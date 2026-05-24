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
// PREREQUISITES (one-time per project):
//   1. Parse C Source on src/asset_register.h (GUI: File -> Parse C
//      Source) so that `ar_sprite_slot`, `ar_gdi_slot`, and
//      `ar_sound_slot` exist in the Data Type Manager.  Without these
//      structs, the auto-this on the class namespace falls back to
//      `void *` and the decompile improvement is much smaller.
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
