// Ghidra script: parse C header(s) into the current program's Data
// Type Manager.
//
// Headless equivalent of File -> Parse C Source — uses the SAME
// CParserUtils.parseHeaderFiles entry point the GUI invokes, so
// behavior matches the GUI's path.  Earlier versions of this script
// used the lower-level CParser class directly and hit a wedge where
// the FIRST parse of a struct only stubbed it at size=1 and
// subsequent parses on the same name silently no-op'd; switching
// to parseHeaderFiles avoids that path.
//
// Run via:
//   ghidra-analyzeHeadless ghidra/projects <project> \
//       -process <binary> \
//       -noanalysis \
//       -scriptPath tools/ghidra-scripts \
//       -postScript ParseCSource.java <header1> [<header2> ...]
//
// Bonus: parseHeaderFiles INCLUDES a real preprocessor (CPP).  So
// headers can use #include / #define / #pragma / #ifndef guards
// directly — no need to strip them.  Unlike the previous
// CParser-only approach.
//
// @category OpenSummoners

import ghidra.app.script.GhidraScript;
import ghidra.app.util.cparser.C.CParserUtils;
import ghidra.app.util.cparser.C.CParserUtils.CParseResults;
import ghidra.program.model.data.CategoryPath;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeManager;
import ghidra.util.exception.DuplicateNameException;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class ParseCSource extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("ParseCSource: missing header argument(s)");
            printerr("Usage: -postScript ParseCSource.java <header1> [<header2> ...]");
            return;
        }

        DataTypeManager dtm = currentProgram.getDataTypeManager();
        int ok = 0;

        for (String arg : args) {
            Path p = Paths.get(arg);
            if (!Files.isReadable(p)) {
                printerr("  not readable: " + arg);
                continue;
            }

            // Extract struct tag names for the post-parse sanity check.
            // Sanity check exists because earlier attempts using the
            // low-level CParser API silently stubbed structs at size=1.
            // parseHeaderFiles is reportedly free of that wedge but
            // we keep the check for confidence.
            List<String> tagNames;
            try {
                tagNames = extractStructTags(new String(Files.readAllBytes(p)));
            } catch (Exception e) {
                printerr("  read failed: " + arg + " — " + e.getMessage());
                continue;
            }

            // Pre-clean: remove any existing DTM types whose tag names
            // appear in this header.  Without this, repeated parses
            // pick up the cached version and edits to layout don't
            // take effect.  parseHeaderFiles itself handles fresh
            // adds well, but doesn't appear to update existing types.
            for (String tag : tagNames) {
                DataType existing = findTypeByName(dtm, tag);
                while (existing != null) {
                    dtm.remove(existing, monitor);
                    existing = findTypeByName(dtm, tag);  // type may live in
                                                          // multiple category paths
                }
            }
            currentProgram.flushEvents();

            // parseHeaderFiles takes filename arrays — pass our one
            // file as a single-element list.
            //
            // Include path: tools/ghidra-cpp-shim/ has minimal
            // stdint.h / stddef.h / stdbool.h shims so our headers'
            // `#include <stdint.h>` (etc.) resolves under Ghidra's
            // bundled CPP, which has no libc.  The script lives at
            // tools/ghidra-scripts/, so the shim is at ../ghidra-cpp-shim/.
            //
            // Preproc args: strip C11 `_Static_assert(c, m);` (CPP is
            // C89-ish and doesn't know the keyword).  Our headers use
            // _Static_assert purely as a compile-time layout check on
            // the host build; Ghidra never needs to see it.
            Path scriptDir = Paths.get(getSourceFile().getAbsolutePath())
                                  .getParent();
            String shimDir = scriptDir.resolveSibling("ghidra-cpp-shim")
                                      .toAbsolutePath().toString();
            String[] filenames = new String[]{ p.toAbsolutePath().toString() };
            String[] includePaths = new String[]{ shimDir };
            String[] preprocArgs = new String[]{ "-D_Static_assert(c,m)=" };
            DataTypeManager[] openTypes = null;

            try {
                CParseResults res = CParserUtils.parseHeaderFiles(
                    openTypes, filenames, includePaths, preprocArgs,
                    dtm, monitor);

                // parseHeaderFiles puts new types under a category named
                // after the source file (e.g. /screen.h/sgl_screen).
                // TagThiscallFunctions's auto-this resolver only looks
                // at /<tag>, so move each tag's type to the root.
                for (String tag : tagNames) {
                    DataType dt = findTypeByName(dtm, tag);
                    if (dt != null && !"/".equals(dt.getCategoryPath().getPath() + "/")) {
                        try {
                            dt.setCategoryPath(CategoryPath.ROOT);
                        } catch (DuplicateNameException e) {
                            // Name collision at root — leave the type
                            // where parseHeaderFiles put it; rare in
                            // practice since MFC types have distinct names.
                        }
                    }
                }

                // Verify each tag landed at >1 byte.
                StringBuilder sizes = new StringBuilder();
                boolean anyBad = false;
                for (String tag : tagNames) {
                    DataType dt = findTypeByName(dtm, tag);
                    int sz = (dt != null) ? dt.getLength() : -1;
                    if (sizes.length() > 0) sizes.append(", ");
                    sizes.append(tag).append("=").append(sz);
                    if (sz <= 1) anyBad = true;
                }
                String tagInfo = tagNames.isEmpty()
                    ? ""
                    : String.format(" [%s]", sizes);
                if (anyBad) {
                    printerr(String.format(
                        "  parsed %s%s — WARNING: at least one tag is size<=1",
                        p.getFileName(), tagInfo));
                } else {
                    println(String.format(
                        "  parsed %s%s",
                        p.getFileName(), tagInfo));
                }
                ok++;
            } catch (Exception e) {
                printerr("  parse failed: " + arg + " — " + e.getMessage());
            }
        }

        println("\nResult: " + ok + "/" + args.length + " header(s) parsed");
    }

    /**
     * Look up a data type by name across all categories.  Returns the
     * first hit.  Necessary because CParserUtils.parseHeaderFiles
     * places parsed types in a category named after the source file
     * (e.g. `/screen.h/sgl_screen`), not at the root path that
     * dtm.getDataType("/" + name) would find.
     */
    private static DataType findTypeByName(DataTypeManager dtm, String name) {
        Iterator<DataType> it = dtm.getAllDataTypes();
        while (it.hasNext()) {
            DataType d = it.next();
            if (name.equals(d.getName())) return d;
        }
        return null;
    }

    /**
     * Extract struct tag names from a C source.  Matches
     * `struct <name> {` and `typedef struct <name> {` patterns.
     * Used to pre-clean stale DTM entries before re-parse and to
     * sanity-check post-parse sizes.
     */
    private static List<String> extractStructTags(String src) {
        List<String> out = new ArrayList<>();
        Pattern p = Pattern.compile(
            "(?:typedef\\s+)?struct\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\{");
        Matcher m = p.matcher(src);
        while (m.find()) {
            out.add(m.group(1));
        }
        return out;
    }
}
