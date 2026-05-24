# Session handoff — last updated 2026-05-24

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The harness boots retail headlessly + the engine reaches per-frame
ticks under `--turbo --hide-window`.  Phase 1 surface mapping and
Phase 2 file-format extraction are complete enough to support
methodical port-and-test.  Three modules are now ported:

- **Pixel-Drawer** — 7 functions, 31 host tests passing.
- **Asset-Register** — 19 functions ported.  Pure logic with GDI
  wrappers split into `asset_register_win32.c` (real build only).
  Latest adds close out the FUN_0057b280 backlog:
  - `ar_register_aux_sounds` — the 4 inline `FUN_00563ef0` calls
    the boot driver issues at `FUN_00562ea0:617-620` (indices
    22..25, group 2, IDs 0x4cb / 0x4ca / 0x4c8 / 0x4c9).
  - `ar_register_locale_sounds` — the conditional locale-table
    loop at the tail of retail FUN_0057b280 (walks the 283-entry
    rdata table at `0x00691018` keyed on an `ar_locale_state`
    struct the boot driver will populate from
    DAT_008a6e68 / _6e70 / _6e80+0x1c8).  Touched indices span
    160..464; pool capacity bumped 256 → 512 to fit the retail
    465-slot W_MGR allocation (`AR_SOUND_POOL_COUNT = 465`).
- **WndProc** — `FUN_005b12e0` (the engine's main game window
  message handler).  9-message dispatch including the load-bearing
  WM_ACTIVATEAPP that owns `DAT_008a952c` (the pump's spin-loop
  exit flag).  Win32-free pure logic + recording-stub-friendly
  hooks for the 5 "deep engine" subsystems we haven't ported yet
  (paint helper, input acquire, ZDM, app pause, post-activate
  scrub) — all are no-op placeholders in `wnd_proc_win32.c`.

Total host tests across all three modules: **111 pass, 0 fail, 3
skip** (the 3 skips are 32-bit-only layout asserts that fire at
compile time on the cross build).

**Ghidra C++ recovery infrastructure** — Kaiju extension installed,
`tools/ghidra-scripts/TagThiscallFunctions.java` applies class-
namespace + `__thiscall` + typed prototype to a batch of functions
headlessly, and `tools/ghidra-tag-and-export.sh` is the one-shot
wrapper.  The 8 asset-register thiscalls are tagged.  See
`docs/findings/cpp-recovery-workflow.md` for the full workflow.

Most recent commits (newest first):

- `aec8f15` Asset-Register: port FUN_0057b280 tail (ar_register_locale_sounds)
- `d4198b0` Asset-Register: port ar_register_aux_sounds (FUN_00562ea0:617-620)
- (pending) docs: HANDOFF + PROGRESS for FUN_0057b280-tail checkpoint
- `09c2bfd` docs: capture FUN_0057b280 locale-table layout finding
- `40aabdf` docs: HANDOFF + PROGRESS for FUN_0057b280 checkpoint
- `5814772` Asset-Register: port FUN_0057b280 (ar_register_game_sounds)
- `f96f375` WndProc: port FUN_005b12e0 (wp_handle_message)
- `2a94b02` tools: ghidra-tag-and-export.sh convenience wrapper
- `b8de62b` tools: TagThiscallFunctions script + bump decomp payload limit
- `fc71279` docs: C++ class-recovery workflow + Kaiju extension

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

The FUN_0057b280 sound-batch backlog is now fully ported.  Next-
biggest unblocked items:

1. **(recommended) Palette-session trio** (FUN_004178e0 +
   FUN_00491770 + FUN_005b5d90).  FUN_005b5d90 is a 3-byte COLORREF
   pack — trivial.  FUN_00491770 copies a 1024-byte palette into
   `**this+4`.  FUN_004178e0 is the hard one: opens the sprite's
   PE-resource handle via FUN_005b7800 (359 B, calls FUN_005b71f0 /
   FUN_005b7c10 for the actual decode), checks if 8-bit-indexed via
   FUN_005b6f00, conditionally replaces its palette via FUN_005b7b90.
   Whole PE-resource decoder needed (FUN_005b7800 + FUN_005b71f0 +
   FUN_005b7c10) — this is the biggest blocker for indexed-sprite
   work.  Unblocks the deferred palette ramps in FUN_005749b0 AND
   the entire FUN_0057a330 batch (the second-biggest sprite-
   register call at boot).

2. **WndProc dependency formalization** — model the layouts of the
   5 "deep engine" structs (paint context with +0x164 state and
   +0x138 blit rect, input device with +0x04 vtable + +0x08
   acquired flag, ZDM with +0x18 device array + +0x1c count, input
   manager singleton with +0x2884 ZDM pointer, log singleton with
   +0x404 file handle).  Once each struct is in a header AND added
   to TagThiscallFunctions.java's TAGS, run
   `./tools/ghidra-tag-and-export.sh` to get `this->field` reads
   in the decomp.  Pre-req for actually porting any of those
   subsystems beyond the no-op stubs.

3. **DDraw ZDD wrapper** (`FUN_005b7ee0`, `FUN_005b88c0`, et al).
   Can't be cleanly unit-tested without a DDraw mock layer; verify
   via Frida smoke harness end-to-end.  Unblocks actual rendering
   AND lets the WndProc's `wp_paint_check` hook get a real
   implementation.

4. **`FUN_0057a330`** (3919 B) — heavy palette-ramp work per
   sprite.  Blocked on the palette-session trio (#1).  Big — that's
   a PE-resource decoder for indexed sprites.

5. **`FUN_0057ca40`** (24884 B) — Ghidra decompile FAILS (response
   buffer exceeded).  Will need radare2 hand-disasm or chunked
   Ghidra approach.

6. **`FUN_00563ef0` wave-load half** — defer until we have a reason
   to load sound bytes (i.e. once title scene starts playing audio).
   Big DSound+mmio+resource mock layer for code that is dead at boot.

## Active modules / file layout

```
src/
  main.c                    WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 7 functions, DONE
  asset_register.c/h        Asset-register slots (GDI, sprite, sound) — 19 functions
  asset_register_win32.c    GDI primitive wrappers (CreateFontIndirectA etc.)
  wnd_proc.c/h              Main game window WndProc — pure dispatch
  wnd_proc_win32.c          DefWindowProcA + ExitProcess + 5 placeholder hooks
  Makefile                  single-TU mingw cross-build

tests/
  Makefile                  host gcc + ASan/UBSan; `make -C tests run`
                            filter by name with `F=<substr>`
  t.h                       T_ASSERT_* macros, 0/1/2 = pass/fail/skip
  test_main.c               X-macro registry; one X(name) per test
  test_pixel_drawer.c       31 tests for Pixel-Drawer
  test_asset_register.c     60 tests for Asset-Register
  test_wnd_proc.c           20 tests for WndProc

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
  + `ar_register_game_sprites` + `ar_register_game_sounds` +
  `ar_register_aux_sounds` + `ar_register_locale_sounds` are all
  ported but **not yet called from the drop-in's boot path** —
  they're modules in isolation.  Wire them in once enough adjacent
  register batches land that calling them actually has a visible
  effect.  `ar_register_locale_sounds` ALSO needs the boot driver
  to populate an `ar_locale_state` from the launcher-settings
  globals — see next bullet.
- `ar_locale_state` modelling: the locale loop's three globals
  (DAT_008a6e68, _6e70, *DAT_008a6e80+0x1c8) are passed in as a
  struct in our port.  The boot driver port will need to read them
  from the real BSS / launcher-settings record.  DAT_008a6e80 is a
  pointer-to-pointer-to-launcher-settings; the loop reads
  `*(int*)(*DAT_008a6e80 + 0x1c8)`.  Field 0x1c8 of the launcher-
  settings struct is unmodelled — its specific semantics are
  unknown (likely a "force-default-language" override toggle).
- The WndProc port is also a module in isolation — `wp_handle_message`
  is not wired into any RegisterClassExA call yet.  Wiring requires
  the drop-in to actually own the main game window registration
  (currently retail does that during its init), which means porting
  the WinMain → CreateWindow → message-pump scaffold first.
- `FUN_00563ef0` wave-load second half is unported (the `if
  (param_6 != 0 && ...)` branch that allocates DSound buffers).  Boot
  callers all pass `load_flag = 0` so it's dead code at boot, but the
  per-scene asset loads later in the engine will need it.
- FUN_005749b0's palette ramp section (between inline slot 5 and
  slot 9 writes) is documented in the driver but skipped — depends on
  the palette-session trio AND a PE-resource decoder.  The slot at
  idx 0 (DAT_008a7640, id 0x90b from sotesp.dll) HAS been registered;
  only the palette upload step is missing.
- The 5 "deep engine" structs the WndProc port depends on
  (paint context, input device, ZDM, input manager singleton, log
  singleton) are modelled as opaque void* in the port — see
  "Next move" #2 for the formalization track.
- Locale-table magic / sequence fields: the +0x00 magic field has
  23 distinct values (0xc35a..0xc35d, 0xc4ae, 0xc7xx family,
  0xc8xx family, 0xe2a4..0xe2a8, 0x1874e..0x18759) — the loop only
  uses it as a non-zero "live" marker, but it's probably a
  zone/area tag some OTHER subsystem reads.  Field 0x04 is a
  per-locale group selector 1..73 (with gaps) that's monotonic
  per magic — looks like a "scene_id" the locale pre-loader may
  filter on.  Both fields are extracted into the comment above
  the `locale_sounds[]` array but not retained in our entry
  struct.  Revisit when porting the scene loader.

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
