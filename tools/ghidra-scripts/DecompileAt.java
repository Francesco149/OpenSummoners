// Ghidra post-script: decompile only the function(s) containing given VAs.
//
// This is deliberately stdout-only so exact-edition investigations can query
// a handful of runtime addresses without overwriting the canonical bulk dump.
//
// Run via:
//   ghidra-analyzeHeadless ... -process PROGRAM -noanalysis \
//       -scriptPath tools/ghidra-scripts \
//       -postScript DecompileAt.java 0x401000 [0x...]
// @category OpenSummoners

import java.util.HashSet;
import java.util.Set;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.util.task.ConsoleTaskMonitor;

public class DecompileAt extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            throw new RuntimeException("DecompileAt: supply at least one VA");
        }

        DecompileOptions options = new DecompileOptions();
        options.setMaxPayloadMBytes(500);
        DecompInterface decompiler = new DecompInterface();
        decompiler.setOptions(options);
        decompiler.openProgram(currentProgram);

        FunctionManager functions = currentProgram.getFunctionManager();
        ConsoleTaskMonitor task = new ConsoleTaskMonitor();
        Set<Address> emitted = new HashSet<>();

        for (String text : args) {
            long value = Long.decode(text);
            Address address = toAddr(value);
            Function function = functions.getFunctionContaining(address);
            if (function == null) {
                println("[DecompileAt] no function contains " + address);
                continue;
            }
            if (!emitted.add(function.getEntryPoint())) {
                continue;
            }

            println("\n/* ===== " + function.getName() + " @ " +
                    function.getEntryPoint() + " (contains " + address +
                    ", " + function.getBody().getNumAddresses() + " bytes) ===== */");
            DecompileResults result = decompiler.decompileFunction(function, 120, task);
            if (result != null && result.decompileCompleted()) {
                println(result.getDecompiledFunction().getC());
            } else {
                String error = result == null ? "null result" : result.getErrorMessage();
                println("// decompile failed: " + error);
            }
        }
        decompiler.dispose();
    }
}
