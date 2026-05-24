# C++ class recovery workflow

`sotes.exe` is a 32-bit MSVC C++ binary compiled with **`/GR-` (no RTTI)
and effectively no exception tables**.  Evidence (verified May 2026):

- `operator_new` / `operator delete[]` calls are everywhere
  (`FUN_005bef0e` is the `operator delete[]` thunk we already named).
- `__thiscall` convention — ECX = `this` — is the dominant call shape.
  See the 349 trailing calls in `FUN_0056e190` for a concrete example
  where Ghidra dropped the implicit-ECX setup from the C view.
- `"pure virtual function call"` string in `.rdata` (MSVCRT
  `_purecall`) — confirms virtual dispatch is present.
- **No `.?AV…` / `.?AU…` RTTI strings, no `__CxxFrameHandler`**
  exception unwind tables.  Confirmed via `strings | grep`.

This combination is awkward for the standard Ghidra C++ recovery tools,
all of which lean on RTTI as their primary signal.  This doc captures
what we have set up and the manual workflow that gives the best
real-world payoff on this binary.

## What we evaluated and rejected

- **Astrelsky's Ghidra-Cpp-Class-Analyzer** (the most-cited plugin) —
  **archived October 2023**, last release supports Ghidra 10.2.x only.
  Won't load on our Ghidra 12.0.4.  Community forks (Fancy2209 et al.)
  have commits as recent as 2026 but no released artifacts — source
  build only, against an upstream the maintainers themselves declared
  EOL.  Also: its primary recovery signal IS RTTI, which we don't have.

## What we installed

### CERT Kaiju

Successor to the OOAnalyzer Ghidra plugin.  Maintained by the CERTCC
Pharos team, supports Ghidra 12.0.x and 12.1.x with pre-built zips.

- **Path on disk**: `~/.config/ghidra/ghidra_12.0.4_NIX/Extensions/kaiju/`
- **Source**: <https://github.com/CERTCC/kaiju>
- **Release used**: tag `260515` (2026-05-15), file
  `ghidra_12.0.4_PUBLIC_20260515_kaiju.zip`
- **First-launch activation**: open Ghidra, then
  File → Configure → Miscellaneous → tick "CERT Kaiju" and any of its
  sub-plugins you want, then restart Ghidra.

Features Kaiju brings:

| Feature                          | Useful here? | Why                                  |
|----------------------------------|--------------|--------------------------------------|
| OOAnalyzer JSON importer         | Eventually   | Requires running Pharos first (deferred — needs Docker) |
| Function fingerprint matching    | Maybe        | If we find a related opensource project with shared C++ classes |
| Disasm improvements / patching   | No           | Out of scope for our pure-port workflow |
| Bookmarking & note tools         | Mild         | Nice-to-have for tracking what's been hand-tagged |

The big OOAnalyzer-style class recovery requires also installing CMU
SEI's Pharos (`cmu-sei/pharos`).  Pharos has a Dockerfile; on this
Windows + WSL2 host either Docker Desktop or native `docker.io`
inside WSL works.  **Deferred until we hit a real vtable dispatch we
can't tag manually.**

## The manual workflow that actually carries weight

For our use case — porting individual functions while preserving
behaviour — the highest-leverage move is to **tag each thiscall
function and define its `this` type once** in Ghidra.  This restores
the dropped ECX setups across all callers without needing a class
analyzer.

### Workflow

For each function we've identified as `__thiscall` (i.e. the first
ECX-from-DAT pattern we see in radare2):

1. **Define the `this` struct as a Ghidra Data Type** if not already.
   Window → Data Type Manager → right-click the program folder →
   "New" → Structure.  Match the C struct in `src/asset_register.h`
   field-by-field (offsets matter; padding bytes that the static
   asserts pin should also appear in the Ghidra struct).
2. **Open the function in the Listing view**, right-click its
   signature → "Edit Function Signature".
3. Change **Calling Convention** to `__thiscall`.
4. Add (or edit) the first parameter to be `<struct_name> *this`.
5. Re-decompile the function and its callers — the ECX setups should
   now appear as `this->field` accesses, and call sites that pass
   slot pointers via ECX should now show the slot pointer as the
   first argument.

### Concrete test target

`FUN_005748c0` (the per-sprite-slot register, already ported as
`ar_sprite_slot_register`):

- Currently in Ghidra: 9 typed args + `int *in_ECX` placeholder
  (see `docs/decompiled/by-address/5748c0.c`).  Every read on
  `in_ECX[...]` corresponds to a field of `ar_sprite_slot`.
- After tagging:
  - Function signature becomes `void __thiscall
    FUN_005748c0(ar_sprite_slot *this, void *zdd, void *settings,
    uint16 resource_id, ...)`.
  - The 349 trailing calls in `FUN_0056e190` should re-decompile
    with the slot pointer as the first arg — eliminating the
    radare2 disasm step we needed last session.

### Functions worth tagging now

These have already been ported and have a known `this` type in
`src/asset_register.h`:

| FUN_      | Calling-convention `this` type    | Notes                            |
|-----------|-----------------------------------|----------------------------------|
| `FUN_005748c0` | `ar_sprite_slot *`           | Per-slot sprite register         |
| `FUN_00417b50` | `ar_sprite_slot *`           | Sprite-slot destructor           |
| `FUN_00562a10` | `ar_gdi_slot *`              | GDI-slot destructor              |
| `FUN_00579ec0` | `ar_gdi_slot *`              | GDI-slot reset                   |
| `FUN_0057a030` | `ar_gdi_slot *`              | Install one HFONT                |
| `FUN_0057a1a0` | `ar_gdi_slot *`              | Install one HPEN                 |
| `FUN_0057a260` | `ar_gdi_slot *`              | Install one HBRUSH               |
| `FUN_00563ef0` | `ar_sound_slot *`            | Sound-slot init / lazy-load      |

Doing all eight in one pass means every subsequent boot-driver port
benefits — `FUN_0057a330`, `FUN_0057ca40`, and `FUN_0057b280` are all
called via thiscalls that share these `this` types.

## When we actually hit a vtable

We're not there yet — every port so far has been pure data slots.
The first vtables will likely show up when we touch ZDD / ZDS / ZDM
(DirectDraw / DirectSound / DirectInput Manager) class methods.

When that day arrives, the order of operations:

1. **Identify the vtable** — search for `mov [esi/edi/this], offset
   DAT_XXX` followed by indirect calls of the form `call [eax + N]`
   in the same basic block.  That `DAT_XXX` is the vtable; the
   indirect-call offsets index into it.
2. **Define a function-pointer struct** for the vtable in Ghidra
   (one entry per slot, named by what the dispatch site does).
3. **Stamp the vtable Data Type** at `DAT_XXX`.
4. **Add a `vftable *` field at offset 0 of the class struct** so
   `this->vftable->method()` resolves cleanly.
5. **If we're swamped**: install Pharos + run OOAnalyzer against the
   exe to JSON, import via Kaiju's OOAnalyzer JSON importer.

## Automated tagging — `TagThiscallFunctions.java`

For tagging many functions at once, edit the `TAGS` array in
`tools/ghidra-scripts/TagThiscallFunctions.java`, then run:

```bash
# 1. Save + close Ghidra (headless needs the project lock)
# 2. Apply the tags
nix develop -c ghidra-analyzeHeadless ghidra/projects opensummoners \
    -process sotes.unpacked.exe \
    -noanalysis \
    -scriptPath tools/ghidra-scripts \
    -postScript TagThiscallFunctions.java

# 3. Re-export the C dump so the new this->field accesses show up
nix develop -c ./tools/ghidra-headless.sh
```

The Java port exists because Ghidra-on-nix is built without PyGhidra,
so the equivalent `.py` script fails with "Python is not available"
under headless.  A `.py` twin lives next to the `.java` for reference
and works from the GUI Script Manager if you ever want it there.

### Batching guidance

The tagging step is effectively instant.  The re-export takes ~3 min
because it re-decompiles all ~1768 functions from scratch.  So:

- **Accumulate edits to the `TAGS` table before re-exporting** — when
  porting a new module, tag every thiscall in that module in one
  session, then re-export once.
- **Don't re-export to spot-check a single function** — if you're
  iterating on a tag, open Ghidra GUI, see the change live, then
  batch up several changes and re-export only when you're done.

If we ever need cheaper iteration, the obvious enhancement is adding a
function-address filter argument to `ExportDecompiledC.java` so we can
re-export just N functions instead of all 1768.

## Gotchas worth remembering

- **Decompiler "Response buffer size exceeded"** on `FUN_0056e190`
  after tagging `FUN_005748c0` as `__thiscall` — Ghidra's default
  per-function decomp output cap (50 MB) is too small once each
  trailing call grows by one explicit arg.  Fix:
  Edit → Tool Options → Decompiler → Analysis → bump
  **"Payload Limit"** to 200 (or 500) MB.  This is permanent across
  the project once set.

- **Settings dir on Nix Ghidra is `~/.config/ghidra/...`**, NOT the
  upstream NSA convention `~/.ghidra/...`.  Extensions dropped in
  the latter are invisible to Ghidra.

## Provenance / verification log

- 2026-05-24: Kaiju 260515 installed to
  `~/.config/ghidra/ghidra_12.0.4_NIX/Extensions/kaiju/` and the
  "CERT Kaiju" extension appeared in File → Install Extensions on
  next launch (verified by user in GUI).
- 2026-05-24: Manual `__thiscall` workflow validated on
  `FUN_005748c0`.  After tagging + retyping `this` as
  `ar_sprite_slot *`, the function body decompiles as
  `this->entries`, `this->aux_buf`, `this->resource_id` etc., and
  `FUN_0056e190`'s 349 trailing calls now show their slot pointer
  (`DAT_008a7e58`, `DAT_008a7e5c`, …) as the first argument —
  exactly the data that previously required radare2 disasm to
  recover.  Required bumping the decompiler payload limit (see
  Gotchas above).
- 2026-05-24: `TagThiscallFunctions.java` ran headlessly against the
  full 8-function asset-register batch.  All 8 tagged in one shot;
  re-exported decomps show typed `this->field` accesses throughout.
  Side wins from the typed pointers:
  - `FUN_00562a10` now shows `DeleteObject(this->array[uVar1])`
    (the `ar_gdi_handle` carries the Win32 HGDIOBJ alias through).
  - `FUN_00563ef0` wave-load second half — previously opaque, listed
    in HANDOFF as a deferred port — is now readable enough to port
    when needed (typed `this->buffer`, `this->settings`,
    `this->resource_id` reads visible into the FUN_005bb250 dispatch).
  - `ExportDecompiledC.java` payload limit bumped 50 → 500 MB so
    `FUN_0056e190` decompiles end-to-end.
