# Session handoff — last updated 2026-05-24

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The harness boots retail headlessly + the engine reaches per-frame
ticks under `--turbo --hide-window`.  Phase 1 surface mapping and
Phase 2 file-format extraction are complete enough to support
methodical port-and-test.  **Four modules ported now:**

- **Pixel-Drawer** — 7 functions, 31 host tests passing.
- **Asset-Register** — **25 functions ported including the boot-driver
  wiring**: `ar_boot_register_all` replays FUN_00562ea0:613-624 in
  retail issue order — and **NOW INCLUDES** the slot-register subset
  of FUN_0057ca40 (group 3) via `ar_register_group3_sprites`.  Pure
  logic with GDI wrappers split into `asset_register_win32.c` (real
  build only).
- **Bitmap-Session** — 8 functions (7 thiscalls + 1 free function
  FUN_005b7c10).  Pure PE-resource bitmap decoder, Win32-free body.
- **WndProc** — `FUN_005b12e0` (the engine's main game window
  message handler).  9-message dispatch including the load-bearing
  WM_ACTIVATEAPP that owns `DAT_008a952c`.

Total host tests across all four modules: **160 pass, 0 fail, 4
skip** (the 4 skips are 32-bit-only layout asserts that fire at
compile time on the cross build).

**Ghidra C++ recovery infrastructure** — Kaiju extension installed,
`tools/ghidra-scripts/TagThiscallFunctions.java` applies class-
namespace + `__thiscall` + typed prototype to a batch of functions
headlessly, and `tools/ghidra-tag-and-export.sh` is the one-shot
wrapper.  **24 functions tagged now** (8 asset-register + 7
bitmap_session + 2 palette-session helpers + **7 WndProc-deps**).
Re-exported decomps in `docs/decompiled/` show typed `this->field`
accesses across the family.  **The typed-prototype workflow ALSO
repaired Ghidra's ability to decompile FUN_0057ca40** — what HANDOFF
previously called "Ghidra-fails" decomps cleanly now with the typed
structs in scope.  See `docs/findings/cpp-recovery-workflow.md` for
the full workflow.

The 7 new WndProc-dep tags (paint_ctx/input_dev/zdm/input_mgr/
log_singleton) currently fall back to `void *this` in the bodies
because the new struct definitions in `src/wnd_proc.h` haven't been
parsed into Ghidra's DTM yet.  Call sites already show the explicit
`this` arg though; full upgrade to typed `this->field` reads
requires a one-time GUI step (see "Next move" #1 below).

Most recent commits (newest first):

- (current) WndProc: model 5 deep-engine struct shapes + tag thiscall deps
- `edbaf19` Asset-Register: port FUN_0057ca40 slot-register subset (ar_register_group3_sprites)
- `39d9602` Asset-Register: wire ar_boot_register_all (FUN_00562ea0:613-624)
- `dcc3d15` Asset-Register: port ar_register_palette_ramps (FUN_0057a330)
- `4f89867` bitmap_session: port the PE-resource decoder + palette-ramp wiring
- `8cb9fd8` RE: resolve bitmap_session ECX puzzle + tag the 7 methods
- `b29ff82` docs: HANDOFF + PROGRESS for palette-trio-leaves checkpoint
- `6db790d` docs: capture palette-session + PE-resource decoder rabbit hole
- `d3e8a00` Asset-Register: port palette-trio leaves (FUN_005b5d90 + FUN_00491770)
- `811f56c` docs: HANDOFF + PROGRESS for FUN_0057b280-tail checkpoint

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

The 5 deep-engine struct shapes (paint_ctx, input_dev, zdm,
input_mgr, log_singleton) are now defined in `src/wnd_proc.h` and
the corresponding 7 thiscall functions (FUN_005b9130, FUN_005b94e0,
FUN_005b9500, FUN_005ba290, FUN_005bbd20, FUN_0058ffa0, FUN_00408b90)
are tagged with class namespace + `__thiscall` + typed prototype.
Re-exported decomps show the call sites with explicit `this` args.
The struct *bodies* still decompile as `*(int *)(this + 0xN)`-style
casts because Ghidra's DTM doesn't have the struct definitions yet
— the typed prototype only types the `this` arg, not the deref reads.

1. **(recommended) Parse C Source on wnd_proc.h in Ghidra GUI** —
   one-time manual step (~30s):
   - Open Ghidra GUI on the opensummoners project.
   - File → Parse C Source → add `src/wnd_proc.h` to the source file
     list (and ensure `src/asset_register.h` + `src/bitmap_session.h`
     are still there from prior parses; the dialog remembers them).
   - "Parse to Program" so the 5 new structs (paint_ctx, input_dev,
     zdm, zdm_entry, input_mgr, log_singleton) land in the program's
     DTM.
   - Close Ghidra.
   - Re-run `nix develop -c ./tools/ghidra-tag-and-export.sh` — the
     7 WndProc-dep tags will then re-apply with typed `this`, and
     the re-exported decomp will show e.g.
     `paint_ctx::FUN_005b9130(paint_ctx *this, HWND target) {
        if (this->state == 2) { ... BitBlt(hdc, this->blit_x, ...); }
      }`.
   - Confirms the offset model is right.

   See `docs/findings/cpp-recovery-workflow.md` PREREQUISITES section
   for the general pattern.

2. **DDraw ZDD wrapper** (`FUN_005b7ee0`, `FUN_005b88c0`, et al).
   Can't be cleanly unit-tested without a DDraw mock layer; verify
   via Frida smoke harness end-to-end.  Unblocks actual rendering
   AND lets the WndProc's `wp_paint_check` hook get a real
   implementation.  Now also unblocked by the paint_ctx struct
   shape — the `+0x2c zdd_device` field connects this directly to
   FUN_005b9130/_94e0/_9500.

3. **Wire ar_boot_register_all into the drop-in's WinMain.** —
   currently every batch is a module in isolation; the function
   exists but no real drop-in caller invokes it.  Wiring requires the
   drop-in to own the engine init (currently retail does this).
   Pre-req: ZDD/ZDS/ZDM init (ports of FUN_005b7ee0, FUN_005b9cf0,
   FUN_005bbb10).  Without those, the call can run with stub pointers
   but has no observable effect because the drop-in's drawing path is
   still retail's.

4. **FUN_0057ca40 deferred subsystems** — pick one of:
   - **Parallel-info-table writes** (~380 writes at retail BSS
     0x008a8578..0x008a8b14).  Requires extending `g_ar_sprite_flags[]`
     from flat-u32 to a ~357-entry pointer-to-struct array.  Useful
     once we know what reads from it — see "Open RE threads" for the
     `g_ar_sprite_flags` consumer hunt.
   - **SS_MGR slot-clones** (94 FUN_004179b0 calls) — needs the SS_MGR
     singleton modelled (DAT_008a8440, 0xaac slot table + 0x18e0
     parallel table).
   - **FUN_00582b80 tail** (9 calls + 5×20-byte memcpy tail) — a
     `__thiscall` clone-from-this-slot helper.  Smaller scope but
     depends on the slot-clone semantics being modelled.
   See `docs/findings/0057ca40-rabbit-hole.md` for the full breakdown.

5. **`FUN_00563ef0` wave-load half** — defer until we have a reason
   to load sound bytes (i.e. once title scene starts playing audio).
   Big DSound+mmio+resource mock layer for code that is dead at boot.

## Active modules / file layout

```
src/
  main.c                    WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 7 functions, DONE
  asset_register.c/h        Asset-register slots (GDI, sprite, sound, palette,
                            BOOT-DRIVER WIRING + group-3 sprites) — 25 functions
  asset_register_win32.c    GDI primitive wrappers (CreateFontIndirectA etc.)
  bitmap_session.c/h        PE-resource bitmap decoder (the ar_palette_session_begin backend) — 8 functions
  bitmap_session_win32.c    LocalAlloc/Free + FindResource/LoadResource/LockResource wrappers
  wnd_proc.c/h              Main game window WndProc — pure dispatch
  wnd_proc_win32.c          DefWindowProcA + ExitProcess + 5 placeholder hooks
  Makefile                  single-TU mingw cross-build

tests/
  Makefile                  host gcc + ASan/UBSan; `make -C tests run`
                            filter by name with `F=<substr>`
  t.h                       T_ASSERT_* macros, 0/1/2 = pass/fail/skip
  test_main.c               X-macro registry; one X(name) per test
  test_pixel_drawer.c       31 tests for Pixel-Drawer
  test_asset_register.c     79 tests for Asset-Register (incl. 7 group3_sprites +
                            6 ar_boot_register_all)
  test_bitmap_session.c     31 tests for bitmap_session
  test_wnd_proc.c           20 tests for WndProc

tools/
  frida_capture.py          headless retail harness driver
  frida/opensummoners-agent.js   Frida agent
  run-retail.sh             single-source-of-truth dev loop
  ghidra-tag-and-export.sh  one-shot wrapper for Ghidra re-tag + re-export
  extract/57ca40_sprite_table.py
                            regenerator for the group-3 sprite table
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
  semantics map cleanly.  **FUN_0057ca40's deferred clone subsystem
  needs SS_MGR modelled** — first concrete consumer.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner
  (`FUN_0056aea0`) — Ghidra-flagged unrecovered.  Read with
  `radare2 -c 'pxw 0x60 @ 0x56bfa4'`.
- `ar_boot_register_all` exists but **is not yet called from the
  drop-in's WinMain** — every batch (and the wiring) is still a
  module in isolation.  Wiring requires the drop-in to actually own
  the engine init (currently retail does that), which means porting
  the ZDD/ZDS/ZDM device init (`FUN_005b7ee0`, `FUN_005b9cf0`,
  `FUN_005bbb10`) first so we have real device pointers to pass in.
- `g_ar_sprite_flags[]` — modelled as a flat 14-entry uint32 array
  (retail BSS 0x008a8578..0x008a85ac).  In retail the table is much
  bigger: FUN_0057ca40's deferred parallel-table writes show it
  extends to 0x008a8b14 (~357 entries) AND each entry is itself a
  POINTER to a struct with at least +0/+4/+8 fields (the values 1 vs
  2 in the writes suggest distinct flag semantics per slot).  Modeling
  this needs a refactor: flat-u32 → pointer-to-struct array.  See
  `docs/findings/0057ca40-rabbit-hole.md` for the full breakdown of
  observed writes.  No consumer ported yet.
- `ar_locale_state` modelling: the locale loop's three globals
  (DAT_008a6e68, _6e70, *DAT_008a6e80+0x1c8) are passed in as a
  struct in our port.  The boot driver port will need to read them
  from the real BSS / launcher-settings record.
- The WndProc port is also a module in isolation — `wp_handle_message`
  is not wired into any RegisterClassExA call yet.  Wiring requires
  the drop-in to actually own the main game window registration.
- `FUN_00563ef0` wave-load second half is unported (the `if
  (param_6 != 0 && ...)` branch that allocates DSound buffers).  Boot
  callers all pass `load_flag = 0` so it's dead code at boot, but the
  per-scene asset loads later in the engine will need it.
- The 5 "deep engine" structs the WndProc port depends on
  (paint_ctx, input_dev, zdm + zdm_entry, input_mgr, log_singleton)
  now have field-offset shells in `src/wnd_proc.h`'s
  "deep-engine struct shapes" section.  Only the offsets the
  WndProc + its 5 thiscall callees touch are pinned; everything
  else is opaque padding.  Once any of these subsystems gets a
  real port, the struct moves to its own header (likely renamed
  without coupling to wnd_proc.h).
- Locale-table magic / sequence fields: the +0x00 magic field has
  23 distinct values (0xc35a..0xc35d, 0xc4ae, 0xc7xx family,
  0xc8xx family, 0xe2a4..0xe2a8, 0x1874e..0x18759) — the loop only
  uses it as a non-zero "live" marker, but it's probably a
  zone/area tag some OTHER subsystem reads.  Field 0x04 is a
  per-locale group selector 1..73 (with gaps) that's monotonic
  per magic — looks like a "scene_id" the locale pre-loader may
  filter on.  Revisit when porting the scene loader.
- **FUN_0057ca40 deferred subsystems** (see "Next move" #4):
    - ~380 parallel-info-table writes touching 0x008a8578..0x008a8b14
    - 94 FUN_004179b0 slot-clone calls (SS_MGR thiscall)
    - 9 FUN_00582b80 + 1 FUN_00582d00 tail calls
    - 98 const-data-pointer writes into the parallel-info-table

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
