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
- **Asset-Register** — 16 functions ported.  Pure logic with GDI
  wrappers split into `asset_register_win32.c` (real build only).
  Latest add is `ar_register_game_sprites` (FUN_0056e190) — the
  biggest sprite batch at boot, 442 single-entry sprite registers
  packed into a 442-entry static const data table.  93 of those are
  retail's "inline" blocks (open-coded register at idx 425..517,
  resource IDs 0x592..0x5fb) and 349 are trailing thiscall
  FUN_005748c0 calls (slot pointers came from radare2 disasm of the
  thiscall ECX setups Ghidra dropped from the C view).  All three
  sprite shapes appear: 0xa0×0xb0 scale=1 type=0,
  0xb0×0x90 scale=1 type=0, and 0x80×0x80 scale=0 type=2.
  `AR_SPRITE_SLOT_COUNT` bumped 64 → 1024 to fit (highest touched
  idx = 863).  No branching, no palette work — purely 442 register
  calls.

Total host tests across both modules: **75 pass, 0 fail, 2 skip** (the
skips are 32-bit-only layout asserts that fire at compile time on the
cross build).

Most recent commits (newest first):

- (pending) Asset-Register: port FUN_0056e190 (ar_register_game_sprites)
- `f42354c` docs: HANDOFF + PROGRESS for FUN_005749b0 checkpoint
- `e26712c` Asset-Register: port FUN_005749b0 (ar_register_main_sprites)
- `a30fc2e` docs: HANDOFF + PROGRESS for FUN_005748c0 checkpoint
- `effb3f2` Asset-Register: port FUN_005748c0 (ar_sprite_slot_register)
- `9cd6873` docs: HANDOFF + PROGRESS for FUN_00579a00 sound batch

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

The asset-register batch continues.  Actual boot-driver call order
(per `docs/decompiled/by-address/562ea0.c` lines 614..624):

```
FUN_00579bd0  (group 1)  ✅ ar_register_fonts          — done
FUN_00579a00  (group 1)  ✅ ar_register_sounds         — done
FUN_0057a330  (group 2)  ← 3919 B asm — heavy palette-ramp work per
                            sprite.  Blocked only by the palette-
                            session trio (FUN_004178e0, _005b5d90,
                            _00491770) + their deeper deps
                            (FUN_005b6e70, _005b7800, _005b6f00,
                            _005b7b90, _005b6e90).  Big — that's a
                            PE-resource decoder for indexed sprites.
FUN_00563ef0×4 sounds    ← partial-ported (init half).  Wave-load
                            second half pulls in DSound + mmio +
                            PE-resource mock layer (FUN_005bb250 →
                            FUN_005bb380/_005bb3d0 / FUN_005bb610
                            mmioOpenA / FUN_005bb740 PE-resource /
                            FUN_005bb040 error log; FUN_005bb2f0 →
                            FUN_005bb5c0 DuplicateSoundBuffer).
                            Boot batch passes load_flag=0 → dead at boot.
FUN_0057ca40  (group 3)  ← 24884 B — Ghidra decompile FAILS (response
                            buffer exceeded).  Will need radare2
                            hand-disasm or chunked Ghidra approach.
FUN_0057b280  (group 3)  ← 955 lines.
FUN_005748c0             ✅ ar_sprite_slot_register   — done (helper)
FUN_005749b0  (group 4)  ✅ ar_register_main_sprites  — done
                            (palette ramp deferred)
FUN_0056e190  (group 5)  ✅ ar_register_game_sprites  — done
                            (442 slots; table-driven).
```

1. **(recommended) CLASS_LIZSOFT_SOTES WndProc** (`FUN_005b12e0`,
   441 bytes / 84 lines).  Removes a Frida hack (the WM_ACTIVATEAPP
   → DAT_008a952c PostMessage workaround).  Pulls in input-Acquire,
   ZDM activate, and a few input-device helpers — moderate scope.
   Independent of the remaining asset-register batches, so it
   unblocks Frida-hack removal without waiting on the palette-session
   trio.

2. **Palette-session trio** (FUN_004178e0 + FUN_00491770 +
   FUN_005b5d90).  FUN_005b5d90 is a 3-byte COLORREF pack — trivial.
   FUN_00491770 copies a 1024-byte palette into `**this+4`.
   FUN_004178e0 is the hard one: opens the sprite's PE-resource
   handle via FUN_005b7800, checks if 8-bit-indexed via FUN_005b6f00,
   conditionally replaces its palette via FUN_005b7b90.  Whole
   PE-resource decoder needed.  Unblocks the deferred palette ramps
   in FUN_005749b0 AND the entire FUN_0057a330 batch.

3. **`FUN_0057b280` partial** — likely another sprite-register style
   batch given its position in the boot driver.  Skim Ghidra decomp
   to confirm shape; if it's another hundreds-of-sprites table, the
   same extraction script used for FUN_0056e190 can be reused
   verbatim.

4. **`FUN_00563ef0` wave-load half** — defer until we have a reason
   to load sound bytes (i.e. once title scene starts playing audio).
   Big DSound+mmio+resource mock layer for code that is dead at boot.

5. **DDraw ZDD wrapper** (`FUN_005b7ee0`, `FUN_005b88c0`, et al).
   Can't be cleanly unit-tested without a DDraw mock layer; verify
   via Frida smoke harness end-to-end.  Unblocks actual rendering.

## Active modules / file layout

```
src/
  main.c                    WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 7 functions, DONE
  asset_register.c/h        Asset-register slots (GDI, sprite, sound) — 15 functions
  asset_register_win32.c    GDI primitive wrappers (CreateFontIndirectA etc.)
  Makefile                  single-TU mingw cross-build

tests/
  Makefile                  host gcc + ASan/UBSan; `make -C tests run`
                            filter by name with `F=<substr>`
  t.h                       T_ASSERT_* macros, 0/1/2 = pass/fail/skip
  test_main.c               X-macro registry; one X(name) per test
  test_pixel_drawer.c       31 tests for Pixel-Drawer
  test_asset_register.c     44 tests for Asset-Register

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
- `ar_register_fonts` + `ar_register_sounds` + `ar_register_main_sprites`
  + `ar_register_game_sprites` are ported but **not yet called from
  the drop-in's boot path** — they're modules in isolation.  Wire
  them in once enough adjacent register batches land that calling
  them actually has a visible effect.
- `FUN_00563ef0` wave-load second half is unported (the `if
  (param_6 != 0 && ...)` branch that allocates DSound buffers).  Boot
  callers all pass `load_flag = 0` so it's dead code at boot, but the
  per-scene asset loads later in the engine will need it.
- FUN_005749b0's palette ramp section (between inline slot 5 and
  slot 9 writes) is documented in the driver but skipped — depends on
  the palette-session trio AND a PE-resource decoder.  The slot at
  idx 0 (DAT_008a7640, id 0x90b from sotesp.dll) HAS been registered;
  only the palette upload step is missing.

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
