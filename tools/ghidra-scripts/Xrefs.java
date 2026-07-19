// Ghidra post-script: for each given VA, print the containing function and all
// references TO that function's entry point (callers), with the calling function
// name + call-site address.  Reusable for cross-version RE (who calls X?).
//
// Run via:
//   ghidra-analyzeHeadless ... -process PROGRAM -noanalysis \
//       -scriptPath tools/ghidra-scripts \
//       -postScript Xrefs.java 0x401000 [0x...]
// @category OpenSummoners

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceManager;

public class Xrefs extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            throw new RuntimeException("Xrefs: supply at least one VA");
        }
        FunctionManager functions = currentProgram.getFunctionManager();
        ReferenceManager refs = currentProgram.getReferenceManager();

        for (String text : args) {
            Address address = toAddr(Long.decode(text));
            Function target = functions.getFunctionContaining(address);
            Address entry = target != null ? target.getEntryPoint() : address;
            String tname = target != null ? target.getName() : "(none)";
            println("\n/* ===== xrefs to " + tname + " @ " + entry +
                    " (query " + address + ") ===== */");
            int n = 0;
            for (Reference r : refs.getReferencesTo(entry)) {
                Address from = r.getFromAddress();
                Function caller = functions.getFunctionContaining(from);
                String cname = caller != null ? caller.getName() : "(none)";
                Address centry = caller != null ? caller.getEntryPoint() : null;
                println("  " + r.getReferenceType() + "  from " + from +
                        "  in " + cname + (centry != null ? " @ " + centry : ""));
                n++;
            }
            if (n == 0) println("  (no references)");
            println("  total: " + n);
        }
    }
}
