// Ghidra script: apply class namespace + __thiscall + signature to a
// fixed list of functions, so the decompile shows `this->field` reads
// matching our C ports.
//
// Run via:
//   ghidra-analyzeHeadless ghidra/projects opensummoners \
//       -process sotes.unpacked.exe \
//       -noanalysis \
//       -scriptPath tools/ghidra-scripts \
//       -postScript TagThiscallFunctions.java
//
// Java rather than Jython because nix Ghidra is built without PyGhidra
// (the .py twin of this script fails with "Python is not available" in
// headless mode; the GUI's Jython runtime is separate).
//
// PREREQUISITES (one-time-per-new-struct):
//   1. Parse C Source (GUI: File -> Parse C Source) on EVERY header
//      that defines a struct named in the TAGS array's `className`
//      column.  Currently:
//         src/asset_register.h   → ar_sprite_slot, ar_gdi_slot,
//                                  ar_sound_slot
//         src/bitmap_session.h   → bitmap_session
//         src/wnd_proc.h         → paint_ctx, input_dev, zdm,
//                                  input_mgr, log_singleton
//      Without a matching struct in the Data Type Manager, the
//      auto-this on the class namespace falls back to `void *` and
//      the decompile improvement is much smaller (the tags still
//      apply the calling convention and namespace, but ECX is typed
//      `void *this` instead of `<struct_name> *this`).
//
//   2. No other instance holds the project's write lock.
//
// IDEMPOTENT: re-running over an already-tagged project is safe.
//
// WHY: see docs/findings/cpp-recovery-workflow.md.
// @category OpenSummoners

import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.app.script.GhidraScript;
import ghidra.app.services.DataTypeManagerService;
import ghidra.app.util.parser.FunctionSignatureParser;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.DataTypeManager;
import ghidra.program.model.data.FunctionDefinition;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.SymbolTable;

public class TagThiscallFunctions extends GhidraScript {

    /**
     * (address, class name, prototype) — one row per ported thiscall.
     *
     * class name MUST match a struct of the same name in the Data Type
     * Manager (Ghidra correlates class namespace -> struct by name when
     * generating the auto-this parameter).
     *
     * Prototype is the explicit-args-only form: no `this`, no
     * `__thiscall` keyword.  Both are added automatically by Ghidra
     * once the function lives in a class namespace with __thiscall
     * convention set.
     *
     * Source of truth for the prototypes is src/asset_register.h.
     * Add a row here whenever a new thiscall is ported.
     */
    private static final Object[][] TAGS = {
        // Asset-Register module
        { 0x005748c0L, "ar_sprite_slot",
            "undefined4 FUN_005748c0(void * zdd, void * settings, ushort resource_id, " +
            "uint width, uint height, uint colorkey, uint scale_flag, uint type, ushort group)" },
        { 0x00417b50L, "ar_sprite_slot",
            "void FUN_00417b50(void)" },
        { 0x00562a10L, "ar_gdi_slot",
            "void FUN_00562a10(void)" },
        { 0x00579ec0L, "ar_gdi_slot",
            "void FUN_00579ec0(ushort capacity)" },
        { 0x0057a030L, "ar_gdi_slot",
            "void FUN_0057a030(int width, int height, uint family, ushort group)" },
        { 0x0057a1a0L, "ar_gdi_slot",
            "void FUN_0057a1a0(int width, uint color, ushort group, ushort capacity)" },
        { 0x0057a260L, "ar_gdi_slot",
            "void FUN_0057a260(uint color, ushort group, ushort capacity)" },
        { 0x00563ef0L, "ar_sound_slot",
            "undefined4 FUN_00563ef0(void * zds, void * settings, ushort resource_id, " +
            "ushort count, ushort group, int load_flag)" },

        // Bitmap-session module (PE-resource bitmap decoder; the `this`
        // is the 0x434-byte struct in src/bitmap_session.h).  Pre-port —
        // only tagged so the typed decomp can resolve the ECX puzzle in
        // FUN_004178e0 (see docs/findings/palette-session.md "The
        // blocking puzzle" section).
        { 0x005b71f0L, "bitmap_session",
            "undefined4 FUN_005b71f0(int width, int height, ushort bit_count)" },
        { 0x005b6e70L, "bitmap_session",
            "void FUN_005b6e70(void)" },
        { 0x005b6e90L, "bitmap_session",
            "void FUN_005b6e90(void)" },
        { 0x005b6f00L, "bitmap_session",
            "ushort FUN_005b6f00(void)" },
        { 0x005b6f10L, "bitmap_session",
            "void FUN_005b6f10(ushort bit_count)" },
        { 0x005b7800L, "bitmap_session",
            "undefined4 FUN_005b7800(void * hModule, uint resource_id, " +
            "void * resource_type, int compressed_flag)" },
        { 0x005b7b90L, "bitmap_session",
            "void FUN_005b7b90(void * dest_palette)" },

        // Palette-session thiscall pair on ar_sprite_slot.  Tagged so
        // the dozen call-sites inside FUN_0057a330 show typed
        // this->field accesses (rather than the bare `FUN_004178e0(buf)`
        // / `FUN_00491770(buf)` calls Ghidra otherwise emits — the ECX
        // setup is dropped from the decompile without these tags).
        { 0x004178e0L, "ar_sprite_slot",
            "bool FUN_004178e0(void * out_palette)" },
        { 0x00491770L, "ar_sprite_slot",
            "void FUN_00491770(void * palette)" },
        // NB: FUN_005b7c10 was listed alongside the bitmap-session
        // helpers in HANDOFF (palette-session next-move), but its
        // decomp shows no in_ECX reads — the destination it writes to
        // (param_1 = caller's stack-local BITMAPINFO) is passed
        // explicitly.  It is a regular __cdecl helper, NOT a
        // __thiscall member function.  Do not tag it as bitmap_session;
        // its job is to populate a transient BITMAPINFO before the
        // caller hands it off to FUN_005b71f0.

        // WndProc-dependency thiscalls.  Each one shows up in the
        // FUN_005b12e0 dispatch (or one of its helpers).  The 5
        // distinct `this` types are pinned in src/wnd_proc.h's
        // "deep-engine struct shapes" section — Parse C Source on
        // that header before running this script.
        //
        // FUN_005b9130 (paint-check / blit-from-backbuffer):
        //   ECX = DAT_008a93cc, a paint_ctx*.  Reads +0x164 state,
        //   +0x138..+0x144 dest rect.  Returns 1 iff the WM_PAINT was
        //   consumed (state==2 path), 0 to fall through to
        //   DefWindowProc.  The HWND arg is the target window the
        //   paint blits onto (g_wp_paint_hwnd in our port).
        { 0x005b9130L, "paint_ctx",
            "undefined4 FUN_005b9130(HWND target)" },

        // FUN_005b94e0 / FUN_005b9500 are vtable trampolines invoked
        // by FUN_005b9130 with `this = parent->back_ctx` (+0x16c) —
        // NOT the parent paint_ctx itself.  Both this->zdd_device
        // (+0x2c) entries get called via vtable[0x44] (begin frame)
        // and vtable[0x68] (end frame).  paint_ctx is recursive
        // (front/back sibling pair) so the same struct type fits:
        // tagging them as paint_ctx is correct, and Ghidra infers the
        // +0x16c indirection at the call sites in FUN_005b9130.
        { 0x005b94e0L, "paint_ctx",
            "undefined4 FUN_005b94e0(HWND * out_src_hdc)" },
        { 0x005b9500L, "paint_ctx",
            "void FUN_005b9500(HWND src_hdc)" },

        // FUN_005ba290 (input device acquire):
        //   ECX = an input_dev*.  Calls *this->dev_obj->vtable[7]
        //   (vtable byte offset 0x1c) — the IDirectInputDevice::Acquire
        //   equivalent.  On success sets this->acquired=1.
        { 0x005ba290L, "input_dev",
            "undefined4 FUN_005ba290(void)" },

        // FUN_005bbd20 (ZDM set-active):
        //   ECX = a zdm*.  Iterates this->entries[0..this->count-1]
        //   and brings each entry to the requested active state by
        //   calling activate/deactivate methods on the entry's
        //   sub-objects.  param_1 is the requested state (0/1).
        { 0x005bbd20L, "zdm",
            "void FUN_005bbd20(int active)" },

        // FUN_0058ffa0 (input manager pause-on-deactivate forwarder):
        //   ECX = the input_mgr singleton at &DAT_008a6b60.  If
        //   this->zdm_ptr (+0x2884) is non-NULL, forwards to
        //   FUN_005bbd20(param_1).  Effectively
        //   "input_mgr.zdm.set_active(arg)" with a NULL-guard.
        { 0x0058ffa0L, "input_mgr",
            "void FUN_0058ffa0(int active)" },

        // FUN_00408b90 (log-singleton emit message):
        //   ECX = the log_singleton at &DAT_008a6620.  Writes the tag
        //   (+ optional GetLastError decoration when param_2!=0) to
        //   OutputDebugString AND to a file opened at this->path
        //   (+0x404).  param_1 = main message LPCSTR, param_2 = "also
        //   decode GetLastError" flag, param_3 = trailing LPCSTR
        //   (always &DAT_008a9b6c — engine's CRLF buffer — at every
        //   call site we've seen).
        { 0x00408b90L, "log_singleton",
            "void FUN_00408b90(LPCSTR msg, int errno_decorate, LPCSTR trailer)" },
    };

    @Override
    public void run() throws Exception {
        SymbolTable st = currentProgram.getSymbolTable();
        DataTypeManager dtm = currentProgram.getDataTypeManager();

        // DataTypeManagerService is null under headless; the parser
        // works without it because our types come from the program's
        // own DTM (after Parse C Source).
        DataTypeManagerService dtms =
            (state != null && state.getTool() != null)
                ? state.getTool().getService(DataTypeManagerService.class)
                : null;
        FunctionSignatureParser parser = new FunctionSignatureParser(dtm, dtms);

        println("Tagging " + TAGS.length + " thiscall functions:");
        int ok = 0;

        for (Object[] row : TAGS) {
            long addrInt = (Long) row[0];
            String className = (String) row[1];
            String prototype = (String) row[2];

            if (tagOne(st, parser, addrInt, className, prototype)) {
                ok++;
            }
        }

        println("\nResult: " + ok + "/" + TAGS.length + " tagged");
    }

    /**
     * Return the class namespace `name` under global, creating it if
     * missing.
     */
    private Namespace getOrCreateClass(SymbolTable st, String name)
            throws Exception {
        Namespace parent = currentProgram.getGlobalNamespace();
        Namespace ns = st.getNamespace(name, parent);
        if (ns != null) {
            return ns;
        }
        return st.createClass(parent, name, SourceType.USER_DEFINED);
    }

    private boolean tagOne(SymbolTable st, FunctionSignatureParser parser,
            long addrInt, String className, String prototype) {
        Address addr = toAddr(addrInt);
        Function func = getFunctionAt(addr);
        if (func == null) {
            printerr(String.format("  no function at 0x%08x — skip", addrInt));
            return false;
        }

        try {
            // Step 1: move into the class namespace.  Auto-this
            // resolves to `<className> *` IFF a struct of the same name
            // exists in the DTM.
            Namespace cls = getOrCreateClass(st, className);
            if (!cls.equals(func.getParentNamespace())) {
                func.setParentNamespace(cls);
            }

            // Step 2: __thiscall.  Setting before the signature apply
            // avoids the "auto params can't be edited" trap.
            if (!"__thiscall".equals(func.getCallingConventionName())) {
                func.setCallingConvention("__thiscall");
            }

            // Step 3: parse + apply the typed prototype.  Same path
            // the GUI's Edit Function Signature dialog uses.
            FunctionDefinition sig = parser.parse(func.getSignature(), prototype);
            ApplyFunctionSignatureCmd cmd = new ApplyFunctionSignatureCmd(
                addr, sig, SourceType.USER_DEFINED);
            if (!cmd.applyTo(currentProgram, monitor)) {
                printerr(String.format("  apply failed for 0x%08x: %s",
                    addrInt, cmd.getStatusMsg()));
                return false;
            }

            println(String.format("  tagged %s::%s @ 0x%08x",
                className, func.getName(), addrInt));
            return true;

        } catch (Exception e) {
            printerr(String.format("  error tagging 0x%08x: %s",
                addrInt, e.getMessage()));
            return false;
        }
    }
}
