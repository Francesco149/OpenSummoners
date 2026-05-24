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
- **Asset-Register** — **27 functions ported including the boot-driver
  wiring**: `ar_boot_register_all` replays FUN_00562ea0:613-624 in
  retail issue order, plus the slot-register subset of FUN_0057ca40
  (group 3) via `ar_register_group3_sprites`, plus the **NEW**
  `ar_sprite_slot_clone` (FUN_00582b80, a `__thiscall` slot metadata
  clone) and `ar_info_entry_clear` (FUN_00582d00, a 14-byte clear of
  the 16-byte parallel-info-table entry that pinned the `ar_info_entry`
  struct shape).  Pure logic with GDI wrappers split into
  `asset_register_win32.c` (real build only).
- **Bitmap-Session** — 8 functions (7 thiscalls + 1 free function
  FUN_005b7c10).  Pure PE-resource bitmap decoder, Win32-free body.
- **WndProc** — `FUN_005b12e0` (the engine's main game window
  message handler).  9-message dispatch including the load-bearing
  WM_ACTIVATEAPP that owns `DAT_008a952c`.

Total host tests across all four modules: **167 pass, 0 fail, 4
skip** (the 4 skips are 32-bit-only layout asserts that fire at
compile time on the cross build).

**Ghidra C++ recovery infrastructure** — Kaiju extension installed,
`tools/ghidra-scripts/TagThiscallFunctions.java` applies class-
namespace + `__thiscall` + typed prototype to a batch of functions
headlessly, and `tools/ghidra-tag-and-export.sh` is the one-shot
wrapper.  **26 functions tagged now** (10 asset-register including
the 2 new info_entry/clone tags + 7 bitmap_session + 2 palette-session
helpers + 7 WndProc-deps).
Re-exported decomps in `docs/decompiled/` show typed `this->field`
accesses across the family.  **The typed-prototype workflow ALSO
repaired Ghidra's ability to decompile FUN_0057ca40** — what HANDOFF
previously called "Ghidra-fails" decomps cleanly now with the typed
structs in scope.  See `docs/findings/cpp-recovery-workflow.md` for
the full workflow.

**The full pipeline is now headless.**  `tools/ghidra-tag-and-export.sh`
runs in three stages in a single analyzeHeadless session:
ParseCSource.java parses `src/asset_register.h`, `src/bitmap_session.h`,
`src/wnd_proc.h` into the DTM (with a `tools/ghidra-cpp-shim/`
include path that supplies minimal `stdint.h`/`stddef.h`/`stdbool.h`
since Ghidra's bundled CPP has no libc, and `-D_Static_assert(c,m)=`
to strip the C11 keyword), TagThiscallFunctions.java applies the
TAGS, then ExportDecompiledC.java re-exports.  Every typed
`this->field` access lands in the decomp — no more GUI Parse C
Source step.

Most recent commits (newest first):

- (current) Asset-Register: port FUN_00582b80 (slot clone) + FUN_00582d00 (info entry clear)
- `efa18c5` tooling: automate Parse C Source headlessly in tag-and-export wrapper
- `8a3629c` WndProc: correct paint_ctx — add +0x16c back_ctx pointer
- `40dc757` WndProc: model 5 deep-engine struct shapes + tag thiscall deps
- `edbaf19` Asset-Register: port FUN_0057ca40 slot-register subset (ar_register_group3_sprites)
- `39d9602` Asset-Register: wire ar_boot_register_all (FUN_00562ea0:613-624)
- `dcc3d15` Asset-Register: port ar_register_palette_ramps (FUN_0057a330)
- `4f89867` bitmap_session: port the PE-resource decoder + palette-ramp wiring
- `8cb9fd8` RE: resolve bitmap_session ECX puzzle + tag the 7 methods
- `b29ff82` docs: HANDOFF + PROGRESS for palette-trio-leaves checkpoint
- `6db790d` docs: capture palette-session + PE-resource decoder rabbit hole
- `d3e8a00` Asset-Register: port palette-trio leaves (FUN_005b5d90 + FUN_00491770)

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

The 5 deep-engine struct shapes (paint_ctx, input_dev, zdm,
input_mgr, log_singleton) are now defined in `src/wnd_proc.h`, the
7 thiscall functions (FUN_005b9130, FUN_005b94e0, FUN_005b9500,
FUN_005ba290, FUN_005bbd20, FUN_0058ffa0, FUN_00408b90) are tagged,
and the re-export landed typed `this->field` reads across the
family.  Example: `paint_ctx::FUN_005b9130(paint_ctx *this, HWND target)`
now reads `if (this->state == 2) { ... BitBlt(hdc, this->blit_x,
this->blit_y, this->blit_w, this->blit_h, ...); FUN_005b94e0(this->back_ctx, &target); }`.
The WndProc itself reads as a clean class-dispatched function:
`input_mgr::FUN_0058ffa0((input_mgr *)&DAT_008a6b60, 1)`,
`zdm::FUN_005bbd20(DAT_008a93e4, ...)`,
`log_singleton::FUN_00408b90((log_singleton *)&DAT_008a6620, "ActivateInputDevice CP1", 0, &CRLF)`.

1. **(recommended) DDraw ZDD wrapper** (`FUN_005b7ee0`,
   `FUN_005b88c0`, et al).  Can't be cleanly unit-tested without a
   DDraw mock layer; verify via Frida smoke harness end-to-end.
   Unblocks actual rendering AND lets the WndProc's `wp_paint_check`
   hook get a real implementation.  Also unblocked by the paint_ctx
   struct shape — the `+0x2c zdd_device` field connects this
   directly to FUN_005b9130/_94e0/_9500.

2. **Wire ar_boot_register_all into the drop-in's WinMain.** —
   currently every batch is a module in isolation; the function
   exists but no real drop-in caller invokes it.  Wiring requires the
   drop-in to own the engine init (currently retail does this).
   Pre-req: ZDD/ZDS/ZDM init (ports of FUN_005b7ee0, FUN_005b9cf0,
   FUN_005bbb10).  Without those, the call can run with stub pointers
   but has no observable effect because the drop-in's drawing path is
   still retail's.

3. **FUN_0057ca40 deferred subsystems** — pick one of:
   - **Parallel-info-table writes** (~380 writes at retail BSS
     0x008a8578..0x008a8b14).  Requires extending `g_ar_sprite_flags[]`
     from flat-u32 to a ~357-entry **`ar_info_entry *`** array (the
     entry struct itself is now modeled — see ported FUN_00582d00
     port for the 16-byte shape).  Useful once we know what reads from
     it — see "Open RE threads" for the `g_ar_sprite_flags` consumer
     hunt.  The 0057ca40-rabbit-hole.md prefix-table breakdown also
     needs re-classifying: what we called "+4: 1/2 flag" is actually
     marker @+0 (since `ar_info_entry::marker` is +0 and `flag` is +4).
   - **SS_MGR slot-clones** (94 FUN_004179b0 calls) — needs the SS_MGR
     singleton modelled (DAT_008a8440, 0xaac slot table + 0x18e0
     parallel table).
   - **FUN_00582b80/FUN_00582d00 call-cluster wiring** — both
     primitives are now ported (`ar_sprite_slot_clone` +
     `ar_info_entry_clear`); the next step is the 9 clusters in
     FUN_0057ca40 that combine them with the inline template-slot init
     + parallel-table-entry copy.  Needs the parallel-info-table array
     above before it can actually be exercised end-to-end.
   - **FUN_0057ca40 tail 5×20-byte memcpy loops** at `+0xae0`-base —
     another parallel table we haven't named.  Defer until a consumer
     surfaces.
   See `docs/findings/0057ca40-rabbit-hole.md` for the full breakdown.

4. **`FUN_00563ef0` wave-load half** — defer until we have a reason
   to load sound bytes (i.e. once title scene starts playing audio).
   Big DSound+mmio+resource mock layer for code that is dead at boot.

## Active modules / file layout

```
src/
  main.c                    WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 7 functions, DONE
  asset_register.c/h        Asset-register slots (GDI, sprite, sound, palette,
                            BOOT-DRIVER WIRING + group-3 sprites + slot-clone
                            + info-entry clear) — 27 functions
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
  test_asset_register.c     86 tests for Asset-Register (incl. 7 group3_sprites +
                            6 ar_boot_register_all + 5 slot-clone + 2
                            info-entry-clear)
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
  POINTER to an **`ar_info_entry` (16 B)** — struct shape now pinned
  by the FUN_00582d00 port (marker @+0, flag @+4, const-data @+8,
  f_0c @+12).  Modeling needs a refactor: flat-u32 →
  `ar_info_entry *` array of ~357 entries.  See
  `docs/findings/0057ca40-rabbit-hole.md` §4 for the disasm walk and
  the prefix-table re-classification note (what we called "+4: 1/2
  flag" was actually `marker` @+0).  No consumer of the table ported
  yet — both write-side primitives (`ar_sprite_slot_clone` +
  `ar_info_entry_clear`) sit idle awaiting the array landing.
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
- **FUN_0057ca40 deferred subsystems** (see "Next move" #3):
    - ~380 parallel-info-table writes touching 0x008a8578..0x008a8b14
      — entries are now-modeled `ar_info_entry *`, but the table
      indexing + per-prefix semantics still need wiring
    - 94 FUN_004179b0 slot-clone calls (SS_MGR thiscall)
    - **9 FUN_00582b80 + 1 FUN_00582d00 clusters — primitives PORTED
      (`ar_sprite_slot_clone` + `ar_info_entry_clear`), call-cluster
      wiring still pending**
    - 98 const-data-pointer writes into the parallel-info-table
      (target the `ar_info_entry::data` field at +8)

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
