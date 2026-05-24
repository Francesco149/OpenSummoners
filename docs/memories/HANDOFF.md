# Session handoff — last updated 2026-05-24

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The harness boots retail headlessly + the engine reaches per-frame
ticks under `--turbo --hide-window`.  Phase 1 surface mapping and
Phase 2 file-format extraction are complete enough to support
methodical port-and-test.  Test harness scaffold landed and the
**Pixel-Drawer module is fully ported** (7 functions, 30 tests
passing on host, 32-bit cross build verifies retail offset parity).

Most recent commits (newest first):

- `592d728` docs: PROGRESS entry for the Pixel-Drawer session
- `aa0e62c` Pixel-Drawer: slot commit + format-mask reader (module complete)
- `bb8c706` Pixel-Drawer: LUT builder (FUN_005bd040) + 10 tests
- `a53c141` Pixel-Drawer leaf primitives + test harness scaffold
- `07088a7` Title-scene runner (FUN_0056aea0) + state-code map
- `a2e5cb0` Lizsoft sprite format (0x425f family): spec + extractor
- `8d6855c` Harness: fix --max-frames cap formula

## Active goal

**Replicate the engine's init sequence in the drop-in until the
title menu actually renders.**  Port modules in dependency order;
each ported function gets unit tests in `tests/test_*.c`.  The
sibling **openrecet** at `/opt/src/openrecet` is the model for
porting style (see its `src/` and `tests/`).  Asset extraction is
explicitly *not* the goal — sprites will load through the engine's
own paths once we replicate them.

## Next move (pick one — recommendation first)

1. **(recommended) Asset-register batch — `FUN_00579bd0` (fonts)**
   first.  Small, populates Pixel-Drawer slots directly, so it
   integrates the work we just finished and exercises the
   Pixel-Drawer module end-to-end.  After fonts, the sprite slot
   registrations (`FUN_0056e190` family) follow naturally.

2. **DDraw ZDD wrapper** (`FUN_005b7ee0`, `FUN_005b88c0`,
   `FUN_005b89d0`, `FUN_005b8c00`, `FUN_005b95c0`, `FUN_005b9520`)
   — substantial, Win32-heavy, can't be cleanly unit-tested in
   pure C without a DDraw mock layer.  Verify via the Frida smoke
   harness end-to-end rather than fine-grained tests.  Unblocks
   actual rendering.

3. **CLASS_LIZSOFT_SOTES WndProc** (`FUN_005b12e0`, 84 lines).
   Small, includes the load-bearing WM_ACTIVATEAPP → DAT_008a952c
   side-effect that our Frida agent currently fakes via
   PostMessage.  Porting it lets the drop-in handle activation
   natively.

## Active modules / file layout

```
src/
  main.c              WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h       MessageBox redirect prologue patch
  pixel_drawer.c/h    ZDPixelDrawer — DONE (5 leaves + LUT builder + commit)
  Makefile            single-TU mingw cross-build → opensummoners{,-debug}.exe

tests/
  Makefile            host gcc + ASan/UBSan; `make -C tests run`
                      filter by name with `F=<substr>`
  t.h                 T_ASSERT_* macros, 0/1/2 = pass/fail/skip
  test_main.c         X-macro registry; one X(name) per test
  test_pixel_drawer.c 31 tests for the Pixel-Drawer module

tools/
  frida_capture.py    headless retail harness driver
  frida/opensummoners-agent.js   Frida agent
  run-retail.sh       single-source-of-truth dev loop
```

## Open RE threads (not picked up yet)

- `FUN_005bd040` mode 3 / mode 4 LUT formulas have arithmetic
  whose "floor-correction" terms are zero for valid weight ranges
  but kept literally in the port.  If we ever see out-of-range
  weights flowing through, audit those branches first.
- The Pixel-Drawer slot-table boot loops (5 fixed-size groups, 69
  total slots at DAT_008a92b8 / DAT_008a9308 / DAT_008a9358 /
  DAT_008a93bc / DAT_008a936c) are documented in
  `docs/findings/winmain-and-bootstrap.md` "Pixel Drawer slot
  tables" but NOT ported yet.  Port them only after at least one
  consumer (asset register) lands — the loops alone are inert.
- SS_MGR / W_MGR / GD_MGR boot-pool allocators
  (DAT_008a8440 / DAT_008a6ec4 / DAT_008a9274) are dependency-of
  ~30 functions; defer until consumer semantics are mapped.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner
  (`FUN_0056aea0`) is Ghidra-flagged as unrecovered.  Read it
  with `radare2 -c 'pxw 0x60 @ 0x56bfa4'` when we want to map
  the title-menu action enum to scene IDs.

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
   append-only history; rewrite HANDOFF as the rolling current
   state.
