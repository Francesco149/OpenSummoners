// Ghidra post-script: export a relocation-invariant fingerprint for every
// function, as JSONL, so tools/vamap/match.py can map VAs across the SotES
// editions (JP-SE / EN-SE / EN-old) — a BinDiff-style structural match tuned
// for this near-identical same-compiler family.
//
// Per function one JSON line:
//   va     entry point (hex)
//   name   symbol name (FUN_* if unnamed)
//   size   body byte count
//   nins   instruction count
//   mh     hash of the mnemonic-only sequence               (coarse, very robust)
//   sh     hash of the mnemonic+operand-class+SCALAR seq     (fine; ADDRESS operands
//          masked so relocation differences don't perturb it, but struct offsets /
//          constants — which are stable across editions — are kept)
//   strs   sorted distinct string literals referenced        (strong cross-edition anchors)
//   calls  raw callee entry VAs referenced by CALL           (structural; mapped later)
//   ncall  number of references TO this function             (caller count)
//   sk     bottom-32 KMV sketch of 3-gram instruction shingles (addr-masked) — enables a
//          Jaccard SIMILARITY estimate between any two functions, so match.py can pair
//          relocated / slightly-EDITED functions and score how much each match changed
//   nsh    total distinct shingles (sketch denominator hint)
//
// Run:
//   ghidra-analyzeHeadless ghidra/projects opensummoners -process PROGRAM -noanalysis \
//     -scriptPath tools/ghidra-scripts -postScript ExportFuncFingerprints.java OUT.jsonl
// @category OpenSummoners

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.TreeSet;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.lang.OperandType;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.RefType;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceManager;

public class ExportFuncFingerprints extends GhidraScript {

    private static long fnv1a(String s) {                       // 64-bit FNV-1a
        long h = 0xcbf29ce484222325L;
        for (int i = 0; i < s.length(); i++) {
            h ^= (s.charAt(i) & 0xff);
            h *= 0x100000001b3L;
        }
        return h;
    }

    private static String esc(String s) {
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '"' || c == '\\') b.append('\\').append(c);
            else if (c == '\n') b.append("\\n");
            else if (c == '\r') b.append("\\r");
            else if (c == '\t') b.append("\\t");
            else if (c < 0x20) b.append(' ');
            else b.append(c);
        }
        return b.toString();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) throw new RuntimeException("supply OUT.jsonl path");
        PrintWriter out = new PrintWriter(args[0]);

        Listing listing = currentProgram.getListing();
        ReferenceManager refs = currentProgram.getReferenceManager();
        int count = 0;

        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.isThunk() || f.isExternal()) continue;
            Address entry = f.getEntryPoint();
            StringBuilder mnem = new StringBuilder();     // mnemonics only
            StringBuilder fine = new StringBuilder();     // mnemonics + operand classes
            TreeSet<String> strs = new TreeSet<>();
            List<String> calls = new ArrayList<>();
            List<String> toks = new ArrayList<>();        // per-instruction token (for shingles)
            int nins = 0;

            for (Instruction ins : listing.getInstructions(f.getBody(), true)) {
                nins++;
                String m = ins.getMnemonicString();
                mnem.append(m).append(';');
                StringBuilder itok = new StringBuilder(m);
                for (int i = 0; i < ins.getNumOperands(); i++) {
                    int t = ins.getOperandType(i);
                    itok.append('|');
                    Register r = ins.getRegister(i);
                    if (OperandType.isAddress(t)) {
                        itok.append('A');                 // relocated -> masked
                    } else if (r != null) {
                        itok.append('r').append(r.getName());
                    } else {
                        Scalar sc = ins.getScalar(i);
                        if (sc != null) itok.append('#').append(sc.getValue());  // stable const/offset
                        else itok.append(OperandType.toString(t));
                    }
                }
                fine.append(itok).append(';');
                toks.add(itok.toString());

                // string literals + call targets referenced by this instruction
                for (Reference ref : ins.getReferencesFrom()) {
                    RefType rt = ref.getReferenceType();
                    if (rt.isCall()) {
                        calls.add(ref.getToAddress().toString());
                    } else if (rt.isData() || rt.isRead()) {
                        Data d = listing.getDataAt(ref.getToAddress());
                        if (d != null && d.getValue() instanceof String) {
                            String sv = ((String) d.getValue());
                            if (sv.length() >= 4) strs.add(sv.length() > 48 ? sv.substring(0, 48) : sv);
                        }
                    }
                }
            }

            int ncall = 0;
            for (Reference r : refs.getReferencesTo(entry)) { if (r.getReferenceType().isCall()) ncall++; }

            // bottom-32 KMV sketch over distinct 3-gram instruction shingles (addr-masked)
            java.util.TreeSet<Long> sk = new java.util.TreeSet<>();
            java.util.HashSet<Long> seenSh = new java.util.HashSet<>();
            for (int i = 0; i + 3 <= toks.size(); i++) {
                long hs = fnv1a(toks.get(i) + "" + toks.get(i + 1) + "" + toks.get(i + 2));
                if (seenSh.add(hs)) { sk.add(hs); if (sk.size() > 32) sk.pollLast(); }
            }

            StringBuilder j = new StringBuilder();
            j.append("{\"va\":\"").append(entry.toString()).append("\"");
            j.append(",\"name\":\"").append(esc(f.getName())).append("\"");
            j.append(",\"size\":").append(f.getBody().getNumAddresses());
            j.append(",\"nins\":").append(nins);
            j.append(",\"mh\":\"").append(Long.toHexString(fnv1a(mnem.toString()))).append("\"");
            j.append(",\"sh\":\"").append(Long.toHexString(fnv1a(fine.toString()))).append("\"");
            j.append(",\"ncall\":").append(ncall);
            j.append(",\"nsh\":").append(seenSh.size());
            j.append(",\"sk\":[");
            boolean firstsk = true;
            for (Long h : sk) { if (!firstsk) j.append(','); j.append('"').append(Long.toHexString(h)).append('"'); firstsk = false; }
            j.append("]");
            j.append(",\"strs\":[");
            boolean first = true;
            for (String s : strs) { if (!first) j.append(','); j.append('"').append(esc(s)).append('"'); first = false; }
            j.append("],\"calls\":[");
            first = true;
            for (String c : calls) { if (!first) j.append(','); j.append('"').append(c).append('"'); first = false; }
            j.append("]}");
            out.println(j.toString());
            count++;
        }
        out.close();
        println("[ExportFuncFingerprints] wrote " + count + " functions to " + args[0]);
    }
}
