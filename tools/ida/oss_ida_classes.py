# oss_ida_classes.py — C++ class recovery for SotES in IDA (imported by oss_ida_export.py).
#
# sotes.exe is MSVC6 C++ built /GR- with no virtual methods at all (0 vtables in .rdata; the
# only function tables are CRT init lists), so RTTI/vtable-based recovery (OOAnalyzer, ClassInformer)
# finds nothing. Classes here are "C with methods": __thiscall functions on singletons and heap
# objects. Recovery from the Hex-Rays ctree:
#   1. methods: functions IDA typed __thiscall
#   2. same class: method F calls thiscall G with F's own `this` → union(F, G)
#   3. instances: a call G(&global) → G's class lives AT that global; G(global) → the global
#      holds a pointer to it; `v = operator new(N); G(v)` → G's class is (at least) N bytes
#   4. fields: every `*(T*)(this + off)` / `this[i]` / `this->f` read or write in a method
#   5. per class a struct cls_<VA> (VA = lowest method) with field_<off> of the observed width,
#      applied as the `this` type of every method — Hex-Rays then prints this->field_1C, and the
#      same field reads the same in every method of the class (IDA xrefs to struct members work)
# Classes already typed by the knowledge pack (TagThiscallFunctions: ar_sprite_slot, …) keep
# their hand-made struct. Output: classes_ida.tsv (class, size, methods, instances, fields).
import collections
import ida_funcs, ida_hexrays as hr, ida_name, ida_nalt, ida_typeinf, idautils


def _strip(e):
    while e is not None and e.op in (hr.cot_cast,):
        e = e.x
    return e


class _UF:
    def __init__(self):
        self.p = {}

    def f(self, a):
        self.p.setdefault(a, a)
        while self.p[a] != a:
            self.p[a] = self.p[self.p[a]]
            a = self.p[a]
        return a

    def u(self, a, b):
        ra, rb = self.f(a), self.f(b)
        if ra != rb:
            self.p[max(ra, rb)] = min(ra, rb)


def _is_thiscall(ea):
    tif = ida_typeinf.tinfo_t()
    if not ida_nalt.get_tinfo(tif, ea) and not ida_typeinf.guess_tinfo(tif, ea):
        return False
    try:
        return (tif.get_cc() & ida_typeinf.CM_CC_MASK) == ida_typeinf.CM_CC_THISCALL
    except Exception:
        return False


def recover(log, skip_typed=()):
    methods = [ea for ea in idautils.Functions()
               if not (ida_funcs.get_func(ea).flags & (ida_funcs.FUNC_LIB | ida_funcs.FUNC_THUNK)) and _is_thiscall(ea)]
    mset = set(methods)
    uf = _UF()
    fields = collections.defaultdict(collections.Counter)   # method -> Counter((off, size))
    inst_at = collections.defaultdict(set)                  # method -> {global VA (object at)}
    inst_ptr = collections.defaultdict(set)                 # method -> {global VA (holds pointer)}
    newsz = collections.defaultdict(int)                    # method -> operator new size
    new_names = ("??2@YAPAXI@Z", "operator new")
    nfail = 0
    for ea in methods:
        uf.f(ea)
        try:
            cf = hr.decompile(ea)
        except Exception:
            nfail += 1
            continue
        this_idx = cf.argidx[0] if len(cf.argidx) else -1
        lv = cf.get_lvars()
        newvar = {}

        class V(hr.ctree_visitor_t):
            def __init__(self):
                hr.ctree_visitor_t.__init__(self, hr.CV_FAST)

            def field(self, off, size):
                if 0 <= off < 0x100000 and size in (1, 2, 4, 8, 10):
                    fields[ea][(off, size)] += 1

            def this_off(self, x):
                """x = an address expression; → byte offset from `this`, or None"""
                x = _strip(x)
                if x.op == hr.cot_var and x.v.idx == this_idx:
                    return 0
                if x.op in (hr.cot_add, hr.cot_sub) and _strip(x.x).op == hr.cot_var and _strip(x.x).v.idx == this_idx and x.y.op == hr.cot_num:
                    base = _strip(x.x)
                    scale = 1
                    if base.type.is_ptr():
                        scale = max(1, base.type.get_pointed_object().get_size())
                    n = x.y.numval() * scale
                    return n if x.op == hr.cot_add else -n
                return None

            def visit_expr(self, e):
                if e.op == hr.cot_ptr:
                    off = self.this_off(e.x)
                    if off is not None:
                        self.field(off, e.ptrsize if e.ptrsize > 0 else e.type.get_size())
                elif e.op == hr.cot_idx:
                    b = _strip(e.x)
                    if b.op == hr.cot_var and b.v.idx == this_idx and e.y.op == hr.cot_num and b.type.is_ptr():
                        sz = max(1, b.type.get_pointed_object().get_size())
                        self.field(e.y.numval() * sz, sz)
                elif e.op in (hr.cot_memptr,):
                    b = _strip(e.x)
                    if b.op == hr.cot_var and b.v.idx == this_idx:
                        self.field(e.m, e.type.get_size())
                elif e.op == hr.cot_asg:
                    y = _strip(e.y)
                    if y.op == hr.cot_call and _strip(y.x).op == hr.cot_obj and y.a.size() == 1 and _strip(y.a[0]).op == hr.cot_num:
                        nm = ida_name.get_name(_strip(y.x).obj_ea) or ""
                        if any(k in nm for k in new_names) and _strip(e.x).op == hr.cot_var:
                            newvar[_strip(e.x).v.idx] = _strip(y.a[0]).numval()
                elif e.op == hr.cot_call:
                    c = _strip(e.x)
                    if c.op == hr.cot_obj and c.obj_ea in mset and e.a.size() > 0:
                        g = c.obj_ea
                        a0 = _strip(e.a[0])
                        if a0.op == hr.cot_var and a0.v.idx == this_idx:
                            uf.u(ea, g)
                        elif a0.op == hr.cot_var and a0.v.idx in newvar:
                            newsz[g] = max(newsz[g], newvar[a0.v.idx])
                        elif a0.op == hr.cot_ref and _strip(a0.x).op == hr.cot_obj:
                            inst_at[g].add(_strip(a0.x).obj_ea)
                        elif a0.op == hr.cot_obj:
                            inst_ptr[g].add(a0.obj_ea)
                        elif a0.op == hr.cot_num and a0.numval() > 0x400000:
                            inst_at[g].add(a0.numval())
                return 0
        V().apply_to(cf.body, None)
    # instances shared between methods also join classes
    by_inst = collections.defaultdict(list)
    for m, gs in list(inst_at.items()):
        for g in gs:
            by_inst[("at", g)].append(m)
    for m, gs in list(inst_ptr.items()):
        for g in gs:
            by_inst[("ptr", g)].append(m)
    for k, ms in by_inst.items():
        for m in ms[1:]:
            uf.u(ms[0], m)
    classes = collections.defaultdict(list)
    for m in methods:
        classes[uf.f(m)].append(m)
    log("classes: %d thiscall methods → %d classes (%d decompile failures)" % (len(methods), len(classes), nfail))
    return classes, fields, inst_at, inst_ptr, newsz


def build_and_apply(log, out_tsv, keep_typed):
    classes, fields, inst_at, inst_ptr, newsz = recover(log)
    n_struct = n_applied = n_inst = 0
    rows = []
    for root, ms in sorted(classes.items()):
        ms.sort()
        if any(m in keep_typed for m in ms):
            continue
        fc = collections.Counter()
        for m in ms:
            fc.update(fields[m])
        size = max([o + s for (o, s) in fc] + [max([newsz[m] for m in ms] + [4])])
        # choose one field per offset (most frequent width; skip overlaps)
        best = {}
        for (o, s), n in fc.items():
            if o not in best or n > fc[(o, best[o])]:
                best[o] = s
        decl, cur = [], 0
        for o in sorted(best):
            s = best[o]
            if o < cur:
                continue
            if o > cur:
                decl.append("  char gap%X[%d];" % (cur, o - cur))
            ty = {1: "unsigned char", 2: "unsigned short", 4: "int", 8: "__int64", 10: "long double"}[s]
            decl.append("  %s field_%X;" % (ty, o))
            cur = o + s
        if size > cur:
            decl.append("  char gap%X[%d];" % (cur, size - cur))
        name = "cls_%X" % ms[0]
        text = "struct %s {\n%s\n};\n" % (name, "\n".join(decl) if decl else "  char gap0[4];")
        if ida_typeinf.parse_decls(None, text, None, ida_typeinf.HTI_DCL) != 0:
            log("classes: struct %s failed to parse" % name)
            continue
        n_struct += 1
        st = ida_typeinf.tinfo_t()
        st.get_named_type(None, name)
        pst = ida_typeinf.tinfo_t(); pst.create_ptr(st)
        for m in ms:
            tif = ida_typeinf.tinfo_t()
            if not ida_nalt.get_tinfo(tif, m) and not ida_typeinf.guess_tinfo(tif, m):
                continue
            fi = ida_typeinf.func_type_data_t()
            if not tif.get_func_details(fi) or fi.size() == 0:
                continue
            fi[0].type = pst
            fi[0].name = "this"
            nt = ida_typeinf.tinfo_t()
            if nt.create_func(fi) and ida_typeinf.apply_tinfo(m, nt, ida_typeinf.TINFO_DEFINITE):
                n_applied += 1
        gat = sorted({g for m in ms for g in inst_at[m]})
        gpt = sorted({g for m in ms for g in inst_ptr[m]})
        for g in gpt:
            nm = ida_name.get_name(g)
            if nm.startswith(("dword_", "unk_", "off_")):
                ida_name.set_name(g, "p_%s" % name, ida_name.SN_NOWARN | ida_name.SN_NOCHECK)
            ida_typeinf.apply_tinfo(g, pst, ida_typeinf.TINFO_DEFINITE); n_inst += 1
        for g in gat:
            nm = ida_name.get_name(g)
            if nm.startswith(("dword_", "unk_", "byte_", "word_")):
                ida_name.set_name(g, "o_%s_%X" % (name, g), ida_name.SN_NOWARN | ida_name.SN_NOCHECK)
        rows.append((name, size, ms, gat, gpt, sorted(best.items())))
    with open(out_tsv, "w") as f:
        f.write("class\tsize\tnmethods\tmethods\tinstances_at\tinstances_ptr\tfields(off:width)\n")
        for name, size, ms, gat, gpt, fl in rows:
            f.write("%s\t%d\t%d\t%s\t%s\t%s\t%s\n" % (name, size, len(ms), ",".join("%x" % m for m in ms),
                    ",".join("%x" % g for g in gat), ",".join("%x" % g for g in gpt), ",".join("%x:%d" % (o, s) for o, s in fl)))
    log("classes: %d structs, %d method this-types applied, %d pointer instances typed → %s" % (n_struct, n_applied, n_inst, out_tsv))
    return n_struct
