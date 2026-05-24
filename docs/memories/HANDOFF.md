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
- **Asset-Register** — 21 functions ported.  Pure logic with GDI
  wrappers split into `asset_register_win32.c` (real build only).
  Latest adds the two leaf halves of the palette trio:
  - `ar_palette_pack_entry` (FUN_005b5d90) — pack a COLORREF into a
    Win32 PALETTEENTRY.  Independent of any container.
  - `ar_palette_install` (FUN_00491770) — lazy-install a 1024-byte
    palette onto `ar_sprite_slot.entries[0].b`.  Leak-clean against
    the existing destructor.
  Third piece — `FUN_004178e0` (begin palette session, PE-resource
  decoder front-end) — deferred; see `docs/findings/palette-session.md`
  for the rabbit-hole notes and the blocking ECX question.
- **WndProc** — `FUN_005b12e0` (the engine's main game window
  message handler).  9-message dispatch including the load-bearing
  WM_ACTIVATEAPP that owns `DAT_008a952c` (the pump's spin-loop
  exit flag).  Win32-free pure logic + recording-stub-friendly
  hooks for the 5 "deep engine" subsystems we haven't ported yet
  (paint helper, input acquire, ZDM, app pause, post-activate
  scrub) — all are no-op placeholders in `wnd_proc_win32.c`.

Total host tests across all three modules: **117 pass, 0 fail, 3
skip** (the 3 skips are 32-bit-only layout asserts that fire at
compile time on the cross build).

**Ghidra C++ recovery infrastructure** — Kaiju extension installed,
`tools/ghidra-scripts/TagThiscallFunctions.java` applies class-
namespace + `__thiscall` + typed prototype to a batch of functions
headlessly, and `tools/ghidra-tag-and-export.sh` is the one-shot
wrapper.  The 8 asset-register thiscalls are tagged.  See
`docs/findings/cpp-recovery-workflow.md` for the full workflow.

Most recent commits (newest first):

- `6db790d` docs: capture palette-session + PE-resource decoder rabbit hole
- `d3e8a00` Asset-Register: port palette-trio leaves (FUN_005b5d90 + FUN_00491770)
- `811f56c` docs: HANDOFF + PROGRESS for FUN_0057b280-tail checkpoint
- `aec8f15` Asset-Register: port FUN_0057b280 tail (ar_register_locale_sounds)
- `d4198b0` Asset-Register: port ar_register_aux_sounds (FUN_00562ea0:617-620)
- `09c2bfd` docs: capture FUN_0057b280 locale-table layout finding
- `40aabdf` docs: HANDOFF + PROGRESS for FUN_0057b280 checkpoint
- `5814772` Asset-Register: port FUN_0057b280 (ar_register_game_sounds)
- `f96f375` WndProc: port FUN_005b12e0 (wp_handle_message)
- `2a94b02` tools: ghidra-tag-and-export.sh convenience wrapper

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

The palette-trio leaves are done.  The "begin palette session" half
(`FUN_004178e0`) is still deferred — the only sensible way to unblock
it is to first model the bitmap-session struct in Ghidra so the
typed decomp reveals which `this` `FUN_005b7800` actually runs on.

1. **(recommended) Tag the bitmap_session class in Ghidra + port the
   PE-resource decoder.**  Steps:
   a. Create `src/bitmap_session.h` with the inferred struct layout
      from `docs/findings/palette-session.md` (offsets +0x00 pixel
      buffer, +0x04 stride, +0x0c BITMAPINFOHEADER, +0x34 palette).
   b. Add rows to `tools/ghidra-scripts/TagThiscallFunctions.java`
      TAGS array for the 8 bitmap-session thiscalls:
      `FUN_005b71f0`, `FUN_005b6e70`, `FUN_005b6e90`,
      `FUN_005b6f00`, `FUN_005b6f10`, `FUN_005b7800`, `FUN_005b7b90`,
      `FUN_005b7c10`.
   c. Run `./tools/ghidra-tag-and-export.sh` and re-read the
      re-exported decomps.  The typed decomp should reveal whether
      `FUN_005b7800`'s ECX in `FUN_004178e0` is the same `this` or
      a sibling object.
   d. Port the decoder + `FUN_004178e0` + wire the palette ramp in
      `ar_register_main_sprites` (`src/asset_register.c:954`).
      Unblocks indexed-sprite work AND the entire `FUN_0057a330`
      batch (second-biggest sprite-register call at boot).

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
   sprite.  Blocked on the palette-session front half (#1).  Big —
   that's PE-resource decoding for indexed sprites.

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
  asset_register.c/h        Asset-register slots (GDI, sprite, sound, palette) — 21 functions
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
  test_asset_register.c     66 tests for Asset-Register
  test_wnd_proc.c           20 tests for WndProc

tools/
  frida_capture.py          headless retail harness driver
  frida/opensummoners-agent.js   Frida agent
  run-retail.sh             single-source-of-truth dev loop
  ghidra-tag-and-export.sh  one-shot wrapper for Ghidra re-tag + re-export
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
  the palette-session FRONT half (`FUN_004178e0` + PE-resource decoder
  — see `docs/findings/palette-session.md`).  The two LEAF halves
  (`ar_palette_pack_entry`, `ar_palette_install`) are now in place.
  The slot at idx 0 (DAT_008a7640, id 0x90b from sotesp.dll) HAS been
  registered; only the seed + install steps are missing.
- The 5 "deep engine" structs the WndProc port depends on
  (paint context, input device, ZDM, input manager singleton, log
  singleton) are modelled as opaque void* in the port — see
  "Next move" #2 for the formalization track.
- The bitmap_session struct (inferred layout in
  `docs/findings/palette-session.md`) is NOT yet in any header.  It
  needs to be the FIRST step of the recommended #1 next-move so
  Ghidra can type the `FUN_005b78xx` family with `this->field` reads.
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
