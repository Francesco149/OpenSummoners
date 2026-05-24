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
- **Asset-Register (fonts)** — `FUN_00579bd0` family, 11 functions
  ported, 24 new host tests (55 total passing).  Win32 isolation via
  function-pointer-free split into `asset_register.c` (pure logic) +
  `asset_register_win32.c` (GDI primitive wrappers, picked up only by
  the real build's wildcard).  32-bit cross build verifies layout
  parity (12 B `ar_gdi_slot`, 0x44 B `ar_sprite_slot`).

Most recent commits (newest first):

- `c4d2da0` Asset-Register: port FUN_00579bd0 family (boot font batch)
- `32e1915` docs: add HANDOFF.md
- `592d728` docs: PROGRESS entry for the Pixel-Drawer session
- `aa0e62c` Pixel-Drawer: slot commit + format-mask reader
- `bb8c706` Pixel-Drawer: LUT builder (FUN_005bd040) + 10 tests
- `a53c141` Pixel-Drawer leaf primitives + test harness scaffold

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

The asset-register batch continues — the boot driver calls these in
order between Pixel-Drawer setup and "The resource was set":

```
FUN_00579bd0  ✅ ar_register_fonts        — done (this session)
FUN_00579a00  ← 90 lines, NEXT
FUN_0057a330  ← 385 lines
FUN_00563ef0  ← 64 lines — sound banks (DirectSound CreateSoundBuffer)
FUN_0057ca40  ← only 3 lines, trivial
FUN_0057b280  ← 955 lines
FUN_005749b0  ← 302 lines
FUN_0056e190  ← 2782 lines — sprite slots, the big one
```

1. **(recommended) `FUN_00579a00`**: 90 lines, smallest follow-up,
   same shape as `FUN_00579bd0` (more font/sprite registers in the
   same family).  Will reuse the entire `ar_gdi_slot` / `ar_sprite_slot`
   API just landed.

2. **`FUN_00563ef0`** (sound bank loader, 64 lines).  DirectSound
   side — `CreateSoundBuffer` + `Lock` + memcpy + `Unlock` per asset.
   Test strategy similar to GDI: stub the DSound calls, record into
   a per-test log.  See `docs/findings/asset-loader.md` "asset
   register calls" for the 4 IDs (0x4c8..0x4cb) the boot loads.

3. **DDraw ZDD wrapper** (`FUN_005b7ee0`, `FUN_005b88c0`, etc.).
   Substantial, Win32-heavy.  Can't be cleanly unit-tested without a
   DDraw mock layer; verify via Frida smoke harness end-to-end.
   Unblocks actual rendering.

4. **CLASS_LIZSOFT_SOTES WndProc** (`FUN_005b12e0`, 84 lines).
   Small, includes the load-bearing WM_ACTIVATEAPP → DAT_008a952c
   side-effect that our Frida agent currently fakes via PostMessage.

## Active modules / file layout

```
src/
  main.c                    WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 7 functions, DONE
  asset_register.c/h        ZDD asset-register slots + font batch — 11 functions, DONE
  asset_register_win32.c    GDI primitive wrappers (CreateFontIndirectA etc.)
  Makefile                  single-TU mingw cross-build

tests/
  Makefile                  host gcc + ASan/UBSan; `make -C tests run`
                            filter by name with `F=<substr>`
  t.h                       T_ASSERT_* macros, 0/1/2 = pass/fail/skip
  test_main.c               X-macro registry; one X(name) per test
  test_pixel_drawer.c       31 tests for Pixel-Drawer
  test_asset_register.c     24 tests for Asset-Register

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
- `ar_register_fonts` is ported but **not yet called from the
  drop-in's boot path** — it's a module in isolation.  Wire it in
  once enough adjacent register batches land that calling it actually
  has visible effect (`FUN_00579a00` + `FUN_0057a330` neighbours).

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
