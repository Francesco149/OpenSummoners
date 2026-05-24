# Session handoff — last updated 2026-05-24

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The harness boots retail headlessly + the engine reaches per-frame
ticks under `--turbo --hide-window`.  Phase 1 surface mapping and
Phase 2 file-format extraction are complete enough to support
methodical port-and-test.  Two modules are now ported:

- **Pixel-Drawer** — 7 functions, 31 host tests passing.
- **Asset-Register** — 13 functions ported across two boot batches:
  - `FUN_00579bd0` font batch (11 funcs, 24 tests)
  - `FUN_00579a00` sound batch + `FUN_00563ef0` init half (2 funcs,
    4 tests)
  - 28 tests total for this module.  Win32-free pure logic with GDI
    wrappers split into `asset_register_win32.c` (real build only).
  - 32-bit cross build verifies layout parity: 12 B `ar_gdi_slot`,
    0x44 B `ar_sprite_slot`, 0x18 B `ar_sound_slot`.

Total host tests across both modules: **59 pass, 0 fail, 2 skip** (the
skips are 32-bit-only layout asserts that fire at compile time on the
cross build).

Most recent commits (newest first):

- `1535783` Asset-Register: port FUN_00579a00 (sound batch) + 563ef0 init
- `0208135` docs: HANDOFF + PROGRESS for Asset-Register checkpoint
- `c4d2da0` Asset-Register: port FUN_00579bd0 family (boot font batch)
- `32e1915` docs: add HANDOFF.md
- `592d728` docs: PROGRESS entry for the Pixel-Drawer session
- `aa0e62c` Pixel-Drawer: slot commit + format-mask reader

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

The asset-register batch continues.  Boot-driver call order:

```
FUN_00579bd0  ✅ ar_register_fonts        — done
FUN_00579a00  ✅ ar_register_sounds       — done
FUN_0057a330  ← 3919 B asm — big, pulls in 5 new sub-funcs
                (FUN_005748c0, _005b5d90, _004178e0, _005b6e70,
                 _00491770).  Stack uses 1900 B locals — looks like
                a per-frame buffer compositor or atlas builder.
FUN_00563ef0  ← 64 lines — partial-ported (init half only).
                Wave-load second half still needs FUN_005bb250 +
                FUN_005bb2f0 (DirectSound CreateSoundBuffer + Lock).
FUN_0057ca40  ← 24884 B — Ghidra decompile FAILS (response buffer
                exceeded).  Will need radare2 hand-disasm or
                chunked Ghidra approach.
FUN_0057b280  ← 955 lines.
FUN_005749b0  ← 302 lines.
FUN_0056e190  ← 2782 lines — sprite slots, ~hundreds of similar
                blocks per asset-loader.md.  Tedious but mechanical.
```

1. **(recommended) `FUN_00563ef0` wave-load half** — close out the
   already-half-ported function.  Adds DirectSound stubs to the
   test harness (similar to the GDI stubs), needs `FUN_005bb250` +
   `FUN_005bb2f0` ported (small, leaf-ish DSound wrappers).  Small,
   self-contained, and unlocks "boot actually loads audio" replay.

2. **`FUN_005749b0`** (302 lines, less crowded).  Smaller than the
   55b/57c/56e behemoths.  Check the call-graph: probably another
   asset-register variant with its own slot type.

3. **`FUN_0056e190`** (sprite slot mega-register).  Same field-write
   shape as `ar_sprite_slot` (already modelled in this session); the
   work is mostly generating the per-slot (id, w, h, colorkey,
   scale, type, group) entry table by walking the disasm.  Could be
   semi-automated by parsing the asm output.

4. **DDraw ZDD wrapper** (`FUN_005b7ee0`, `FUN_005b88c0`, et al).
   Substantial, Win32-heavy.  Can't be cleanly unit-tested without a
   DDraw mock layer; verify via Frida smoke harness end-to-end.
   Unblocks actual rendering.

5. **CLASS_LIZSOFT_SOTES WndProc** (`FUN_005b12e0`, 84 lines).
   Small, includes the load-bearing WM_ACTIVATEAPP → DAT_008a952c
   side-effect that our Frida agent currently fakes via PostMessage.

## Active modules / file layout

```
src/
  main.c                    WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 7 functions, DONE
  asset_register.c/h        Asset-register slots (GDI, sprite, sound) — 13 functions
  asset_register_win32.c    GDI primitive wrappers (CreateFontIndirectA etc.)
  Makefile                  single-TU mingw cross-build

tests/
  Makefile                  host gcc + ASan/UBSan; `make -C tests run`
                            filter by name with `F=<substr>`
  t.h                       T_ASSERT_* macros, 0/1/2 = pass/fail/skip
  test_main.c               X-macro registry; one X(name) per test
  test_pixel_drawer.c       31 tests for Pixel-Drawer
  test_asset_register.c     28 tests for Asset-Register

tools/
  frida_capture.py          headless retail harness driver
  frida/opensummoners-agent.js   Frida agent
  run-retail.sh             single-source-of-truth dev loop
```

## Open RE threads (not picked up yet)

- `FUN_005bd040` mode 3 / mode 4 LUT formulas have arithmetic whose
  "floor-correction" terms are zero for valid weight ranges but kept
  literally in the port.  Audit if out-of-range weights ever flow.
- The Pixel-Drawer slot-table boot loops (5 fixed-size groups, 69
  total slots at DAT_008a92b8 / _9308 / _9358 / _93bc / _936c) are
  documented in `winmain-and-bootstrap.md` "Pixel Drawer slot tables"
  but NOT ported yet.  Port them once asset-register consumers start
  reading those slots.
- SS_MGR / W_MGR / GD_MGR boot-pool allocators (DAT_008a8440 / _6ec4
  / _9274) are dependency-of ~30 functions; defer until consumer
  semantics map cleanly.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner
  (`FUN_0056aea0`) — Ghidra-flagged unrecovered.  Read with
  `radare2 -c 'pxw 0x60 @ 0x56bfa4'`.
- `ar_register_fonts` + `ar_register_sounds` are ported but **not
  yet called from the drop-in's boot path** — they're modules in
  isolation.  Wire them in once enough adjacent register batches
  land that calling them actually has a visible effect.
- `FUN_00563ef0` wave-load second half is unported (the `if
  (param_6 != 0 && ...)` branch that allocates DSound buffers).  Boot
  callers all pass `load_flag = 0` so it's dead code at boot, but the
  per-scene asset loads later in the engine will need it.

## How to apply

When the user says "continue RE work" (or similar):

1. Read this file first.
2. Glance at `docs/PROGRESS.md` head entry for fuller context.
3. Pick the recommended next move (or whichever the user redirects to).
4. Work in the methodical port-and-test style: small unit → tests →
   commit.  Each ported function gets a clear retail provenance
   comment (`FUN_XXXXXX` reference) and at least one test that
   spot-checks behaviour vs hand-computed expectations.
5. Update THIS file when a meaningful checkpoint lands (module
   completed, direction changed, etc.).  Keep PROGRESS.md as the
   append-only history; rewrite HANDOFF as the rolling current state.
