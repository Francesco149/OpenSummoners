# oss_ida_export.py — IDA 9.x headless: (optional) apply repo knowledge + C++ vtable
# recovery, then export pseudocode / disasm / call+data xref index / per-function
# fingerprints for one SotES edition.
#
# Run (Windows, via tools/ida/run_ida.ps1):
#   idat -A -L<log> -Ooss:<out_dir>[;kb=<knowledge.json>][;classes][;novtbl][;noasm] -S<this> <exe|i64>
#
# Outputs (<out_dir>, all derived from the user's binary — never committed):
#   all_ida.c            Hex-Rays pseudocode, every function, "/* ===== name @ 0xVA ..." headers
#   all_ida.asm          disassembly per function (address-prefixed lines)
#   functions_ida.tsv    va size name flirt_lib cc nargs callers callees proto
#   data_xrefs.tsv       target_va target_name func_va insn_va kind(r|w|o)
#   strings_ida.tsv      va len text(escaped) referencing funcs
#   vtables_ida.tsv      vtable_va nslots ctor/xref funcs slot targets (none in sotes: /GR-, no virtuals)
#   classes_ida.tsv      recovered classes (oss_ida_classes.py): methods, instances, size, fields
#   fingerprints.jsonl   one object per function (crosswalk input: masked-bytes hash,
#                        mnemonic hash, cfg shape, string refs, imm consts, callees)
#   export_summary.txt   counts + timings
import os, json, time, hashlib, re
import idaapi, ida_auto, ida_hexrays, ida_funcs, ida_pro, idautils, ida_name, idc
import ida_bytes, ida_ua, ida_segment, ida_gdl, ida_xref, ida_typeinf, ida_nalt, ida_lines

def _opts():
    raw = idaapi.get_plugin_options("oss") or r"C:\oss-ida\out"
    parts = raw.split(";")
    o = {"out": parts[0]}
    for p in parts[1:]:
        if "=" in p:
            k, v = p.split("=", 1); o[k] = v
        elif p:
            o[p] = True
    return o

O = _opts()
OUT = O["out"]
os.makedirs(OUT, exist_ok=True)
LOG = open(os.path.join(OUT, "export_log.txt"), "w", encoding="utf-8")
def log(*a):
    LOG.write(" ".join(str(x) for x in a) + "\n"); LOG.flush()

t0 = time.time()
ida_auto.auto_wait()
t_auto = time.time() - t0
log("auto_wait %.1fs" % t_auto)

def seg_of(ea):
    s = ida_segment.getseg(ea)
    return ida_segment.get_segm_name(s) if s else ""

TEXT = ida_segment.get_segm_by_name(".text")
TEXT_LO, TEXT_HI = (TEXT.start_ea, TEXT.end_ea) if TEXT else (0, 0)
IMG_LO = idaapi.inf_get_min_ea(); IMG_HI = idaapi.inf_get_max_ea()

# ---------------------------------------------------------------- knowledge pack
# kb JSON (tools/ida/gen_knowledge.py): {"decls": "<C text>", "names": {va: name},
#   "protos": {va: "C prototype"}, "cmts": {va: text}, "gtypes": {va: "C type"}}
n_kb = {"decl_err": 0, "names": 0, "protos": 0, "proto_fail": 0, "cmts": 0, "gtypes": 0}
if O.get("kb"):
    kb = json.load(open(O["kb"], encoding="utf-8"))
    if kb.get("decls"):
        n_kb["decl_err"] = ida_typeinf.parse_decls(None, kb["decls"], None, ida_typeinf.HTI_DCL | ida_typeinf.HTI_PAKDEF)
        log("kb decls parsed, errors=%d" % n_kb["decl_err"])
    for va, nm in kb.get("names", {}).items():
        ea = int(va, 16)
        if ida_name.set_name(ea, nm, ida_name.SN_NOWARN | ida_name.SN_NOCHECK | ida_name.SN_FORCE):
            n_kb["names"] += 1
    for va, ty in kb.get("gtypes", {}).items():
        if idc.SetType(int(va, 16), ty):
            n_kb["gtypes"] += 1
    for va, pr in kb.get("protos", {}).items():
        ea = int(va, 16)
        if not ida_funcs.get_func(ea):
            ida_funcs.add_func(ea)
        if idc.SetType(ea, pr):
            n_kb["protos"] += 1
        else:
            n_kb["proto_fail"] += 1; log("proto fail %s: %s" % (va, pr))
    for va, c in kb.get("cmts", {}).items():
        ea = int(va, 16)
        f = ida_funcs.get_func(ea)
        if f and f.start_ea == ea:
            ida_funcs.set_func_cmt(f, c, True)
        else:
            idc.set_cmt(ea, c, 1)
        n_kb["cmts"] += 1
    ida_auto.auto_wait()
    log("kb applied", n_kb)

# ---------------------------------------------------------------- C++ vtable recovery
# /GR- binary (no RTTI): a vtable = a run of >=2 dwords in .rdata each pointing at a
# function start, whose first slot address is written by `mov [reg(+0)], offset` in
# some function (ctor/dtor pattern) or referenced as an immediate.  We name the table
# vtbl_<VA>, type its slots, and name still-anonymous slot targets vf<idx>_<vtblVA>.
vtables = []
if not O.get("novtbl"):
    rdata = ida_segment.get_segm_by_name(".rdata")
    def is_func_start(v):
        f = ida_funcs.get_func(v)
        return f is not None and f.start_ea == v
    if rdata:
        nfp = nfpx = 0
        for a in range(rdata.start_ea, rdata.end_ea - 3, 4):
            if is_func_start(ida_bytes.get_dword(a)):
                nfp += 1
                if any(True for _ in idautils.XrefsTo(a)): nfpx += 1
        log("rdata func-ptr dwords=%d with-xref=%d" % (nfp, nfpx))
        ea = rdata.start_ea
        while ea < rdata.end_ea - 8:
            v = ida_bytes.get_dword(ea)
            if not is_func_start(v):
                ea += 4; continue
            # a vtable starts where some code references this address
            refs = [x for x in idautils.DataRefsTo(ea) if TEXT_LO <= x < TEXT_HI]
            if not refs:
                ea += 4; continue
            slots = []; p = ea
            while p < rdata.end_ea:
                t = ida_bytes.get_dword(p)
                if not is_func_start(t):
                    break
                if p != ea and any(TEXT_LO <= x < TEXT_HI for x in idautils.DataRefsTo(p)):
                    break   # next table begins
                slots.append(t); p += 4
            # ctor/dtor evidence: an instruction `mov dword ptr [reg], offset vtbl`
            writers = []
            for x in refs:
                insn = ida_ua.insn_t()
                if ida_ua.decode_insn(insn, x) and insn.itype == idaapi.NN_mov and insn.ops[0].type in (ida_ua.o_phrase, ida_ua.o_displ):
                    f = ida_funcs.get_func(x)
                    if f: writers.append(f.start_ea)
            if len(slots) >= 2 or writers:
                vtables.append({"va": ea, "slots": slots,
                                "writers": sorted(set(writers)),
                                "refs": sorted({ida_funcs.get_func(x).start_ea for x in refs if ida_funcs.get_func(x)})})
                ea = p
            else:
                ea += 4
    n_vf_named = 0
    for vt in vtables:
        va = vt["va"]
        nm = ida_name.get_name(va)
        is_vt = bool(vt["writers"])   # ctor/dtor stores it into an object: a real vtable
        vt["kind"] = "vtbl" if is_vt else "ftab"
        if not nm or nm.startswith(("off_", "dword_", "unk_")):
            ida_name.set_name(va, "%s_%X" % (vt["kind"], va), ida_name.SN_NOWARN | ida_name.SN_NOCHECK)
        if not is_vt:
            continue
        # slot struct type: vtbl_<VA>_t { void *s0; ... }
        tname = "vtbl_%X_t" % va
        decl = "struct %s {\n%s};\n" % (tname, "".join("  void *vf%X;\n" % (i * 4) for i in range(len(vt["slots"]))))
        if ida_typeinf.parse_decls(None, decl, None, ida_typeinf.HTI_DCL) == 0:
            idc.SetType(va, tname)
        for i, t in enumerate(vt["slots"]):
            tn = ida_name.get_name(t)
            if tn.startswith("sub_"):
                ida_name.set_name(t, "vf%X_%X" % (i * 4, va), ida_name.SN_NOWARN | ida_name.SN_NOCHECK)
                n_vf_named += 1
    log("vtables=%d vf_named=%d" % (len(vtables), n_vf_named))
    ida_auto.auto_wait()

# ---------------------------------------------------------------- C++ class recovery (no RTTI/vtables)
n_classes = 0
if O.get("classes"):
    import sys as _sys
    _sys.path.insert(0, O.get("dir", r"C:\oss-ida"))
    import oss_ida_classes
    ida_hexrays.init_hexrays_plugin()
    keep = {int(v, 16) for v in (kb.get("protos", {}) if O.get("kb") else {})}
    n_classes = oss_ida_classes.build_and_apply(log, os.path.join(OUT, "classes_ida.tsv"), keep)
    ida_auto.auto_wait()
    # the recovery decompiled every method BEFORE giving it its thiscall type: Hex-Rays' cache would hand the
    # export those stale bodies (a caller's call of a now-thiscall callee without its `this`)
    ida_hexrays.clear_cached_cfuncs()

# ---------------------------------------------------------------- helpers
def fn_cc(ea):
    tif = ida_typeinf.tinfo_t()
    if not ida_nalt.get_tinfo(tif, ea):
        if not ida_typeinf.guess_tinfo(tif, ea):
            return "", -1, ""
    try:
        ccv = tif.get_cc() & ida_typeinf.CM_CC_MASK
    except Exception:
        ccv = -1
    cc = {ida_typeinf.CM_CC_CDECL: "cdecl", ida_typeinf.CM_CC_STDCALL: "stdcall",
          ida_typeinf.CM_CC_THISCALL: "thiscall", ida_typeinf.CM_CC_FASTCALL: "fastcall",
          ida_typeinf.CM_CC_PASCAL: "pascal"}.get(ccv, "cc%x" % (ccv & 0xff))
    try:
        nargs = tif.get_nargs()
    except Exception:
        nargs = -1
    return cc, nargs, ida_typeinf.print_tinfo("", 0, 0, ida_typeinf.PRTYPE_1LINE, tif, ida_name.get_name(ea), "") or ""

def esc(s):
    return s.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n").replace("\r", "\\r")

STRS = {}
for s in idautils.Strings():
    try:
        STRS[s.ea] = str(s)
    except Exception:
        pass

def in_image(v):
    return IMG_LO <= v < IMG_HI

def insn_masked_bytes(insn):
    b = bytearray(ida_bytes.get_bytes(insn.ea, insn.size) or b"")
    for op in insn.ops:
        if op.type == ida_ua.o_void:
            break
        addr_like = op.type in (ida_ua.o_mem, ida_ua.o_near, ida_ua.o_far) or \
                    (op.type == ida_ua.o_imm and in_image(op.value)) or \
                    (op.type == ida_ua.o_displ and in_image(op.addr))
        if addr_like:
            off = op.offb if op.type != ida_ua.o_displ else op.offo
            if off and off + 4 <= len(b):
                b[off:off + 4] = b"\0\0\0\0"
            elif op.offb and op.offb < len(b):
                for k in range(op.offb, len(b)):
                    b[k] = 0
    return bytes(b)

# ---------------------------------------------------------------- export
ida_hexrays.init_hexrays_plugin()
nf = 0; fails = []; t1 = time.time()
fc = open(os.path.join(OUT, "all_ida.c"), "w", encoding="utf-8")
fa = None if O.get("noasm") else open(os.path.join(OUT, "all_ida.asm"), "w", encoding="utf-8")
ft = open(os.path.join(OUT, "functions_ida.tsv"), "w", encoding="utf-8")
fx = open(os.path.join(OUT, "data_xrefs.tsv"), "w", encoding="utf-8")
fp = open(os.path.join(OUT, "fingerprints.jsonl"), "w", encoding="utf-8")
ft.write("va\tsize\tname\tflirt_lib\tcc\tnargs\tcallers\tcallees\tproto\n")
fx.write("target_va\ttarget_name\tfunc_va\tinsn_va\tkind\n")
str_refs = {}
for ea in idautils.Functions():
    f = ida_funcs.get_func(ea); name = ida_name.get_name(ea)
    lib = 1 if f.flags & ida_funcs.FUNC_LIB else 0
    thunk = 1 if f.flags & ida_funcs.FUNC_THUNK else 0
    size = f.end_ea - f.start_ea
    callers = sorted({ida_funcs.get_func(x).start_ea for x in idautils.CodeRefsTo(ea, 0) if ida_funcs.get_func(x)})
    callees = set(); mn = []; mbytes = hashlib.sha1(); consts = set(); srefs = set(); grefs = set()
    ninsn = 0
    for item in idautils.FuncItems(ea):
        insn = ida_ua.insn_t()
        if not ida_ua.decode_insn(insn, item):
            continue
        ninsn += 1
        mn.append(insn.itype)
        mbytes.update(insn_masked_bytes(insn))
        for op in insn.ops:
            if op.type == ida_ua.o_void: break
            if op.type == ida_ua.o_imm and not in_image(op.value) and op.value not in (0, 1, 0xffffffff):
                consts.add(op.value & 0xffffffff)
        for x in idautils.CodeRefsFrom(item, 0):
            g = ida_funcs.get_func(x)
            if g and g.start_ea != ea and x == g.start_ea:
                callees.add(x)
        for xr in idautils.XrefsFrom(item, ida_xref.XREF_DATA):
            to = xr.to
            if not in_image(to) or (TEXT_LO <= to < TEXT_HI and ida_funcs.get_func(to)):
                continue
            kind = {ida_xref.dr_R: "r", ida_xref.dr_W: "w"}.get(xr.type, "o")
            fx.write("%08x\t%s\t%08x\t%08x\t%s\n" % (to, ida_name.get_name(to), ea, item, kind))
            if to in STRS:
                srefs.add(STRS[to]); str_refs.setdefault(to, set()).add(ea)
            else:
                grefs.add(to)
        if fa is not None:
            fa.write("%08x  %s\n" % (item, ida_lines.tag_remove(idc.generate_disasm_line(item, 0) or "")))
    cc, nargs, proto = fn_cc(ea)
    ft.write("%08x\t%d\t%s\t%d\t%s\t%d\t%s\t%s\t%s\n" % (ea, size, name, lib, cc, nargs,
             ",".join("%x" % c for c in callers), ",".join("%x" % c for c in sorted(callees)), proto))
    # cfg shape
    nb = ne = 0
    try:
        fcg = ida_gdl.FlowChart(f)
        for bb in fcg:
            nb += 1; ne += sum(1 for _ in bb.succs())
    except Exception:
        pass
    fp.write(json.dumps({"va": ea, "name": name, "size": size, "lib": lib, "thunk": thunk,
        "ninsn": ninsn, "nb": nb, "ne": ne,
        "mhash": hashlib.sha1(bytes(json.dumps(mn), "ascii")).hexdigest()[:16],
        "bhash": mbytes.hexdigest()[:16],
        "callees": sorted(callees), "ncallers": len(callers),
        "strs": sorted(srefs)[:32], "consts": sorted(consts)[:64], "nglob": len(grefs)}) + "\n")
    if fa is not None:
        fa.write("\n")
    fc.write("/* ===== %s @ 0x%08x (%d bytes)%s ===== */\n" % (name, ea, size, " [FLIRT lib]" if lib else ""))
    try:
        fc.write(str(ida_hexrays.decompile(ea)) + "\n\n")
    except Exception as e:
        fails.append(ea); fc.write("/* decompile failed: %r */\n\n" % e)
    nf += 1
for h in (fc, ft, fx, fp):
    h.close()
if fa is not None:
    fa.close()
with open(os.path.join(OUT, "strings_ida.tsv"), "w", encoding="utf-8") as fs:
    fs.write("va\tlen\ttext\tfuncs\n")
    for va in sorted(STRS):
        fs.write("%08x\t%d\t%s\t%s\n" % (va, len(STRS[va]), esc(STRS[va]), ",".join("%x" % x for x in sorted(str_refs.get(va, ())))))
with open(os.path.join(OUT, "vtables_ida.tsv"), "w", encoding="utf-8") as fv:
    fv.write("vtbl_va\tkind\tnslots\twriters\trefs\tslots\n")
    for vt in vtables:
        fv.write("%08x\t%s\t%d\t%s\t%s\t%s\n" % (vt["va"], vt.get("kind", "?"), len(vt["slots"]), ",".join("%x" % x for x in vt["writers"]),
                 ",".join("%x" % x for x in vt["refs"]), ",".join("%x" % x for x in vt["slots"])))
with open(os.path.join(OUT, "export_summary.txt"), "w") as fh:
    fh.write("input=%s md5=%s\n" % (ida_nalt.get_root_filename(), ida_nalt.retrieve_input_file_md5().hex()))
    fh.write("functions=%d failed=%d auto_s=%.1f export_s=%.1f vtables=%d classes=%d kb=%s\n" % (nf, len(fails), t_auto, time.time() - t1, len(vtables), n_classes, json.dumps(n_kb)))
    fh.write("failed_vas=%s\n" % ",".join("%x" % x for x in fails))
log("done")
LOG.close()
ida_pro.qexit(0)
