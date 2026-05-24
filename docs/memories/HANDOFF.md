# Session handoff — last updated 2026-05-25 (SS_MGR slot-clones PORTED)

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
- **Asset-Register** — **30 functions ported including the boot-driver
  wiring**: `ar_boot_register_all` replays FUN_00562ea0:613-624 in
  retail issue order, plus the slot-register subset of FUN_0057ca40
  (group 3) via `ar_register_group3_sprites`, plus `ar_sprite_slot_clone`
  (FUN_00582b80, a `__thiscall` slot metadata clone), `ar_info_entry_clear`
  (FUN_00582d00), the 443-event 4th pass (`ar_apply_group3_info_events`),
  **and NEW this checkpoint: the 94 SS_MGR slot-clones (FUN_004179b0)
  via `ar_apply_group3_clones`** — a 94-row static table walked from
  the tail of `ar_register_group3_sprites` (after the info-events
  pass).  Each clone reuses `ar_sprite_slot_clone` for the slot side
  and `ar_info_entry_clear` + marker/flag copy for the info side; the
  unified-pool accessor `ar_pool_get_slot()` maps pool indices 1..12
  → ramp slots, 13..908 → main slots.  Pure logic with GDI wrappers
  split into `asset_register_win32.c` (real build only).
- **Bitmap-Session** — 8 functions (7 thiscalls + 1 free function
  FUN_005b7c10).  Pure PE-resource bitmap decoder, Win32-free body.
- **WndProc** — `FUN_005b12e0` (the engine's main game window
  message handler).  9-message dispatch including the load-bearing
  WM_ACTIVATEAPP that owns `DAT_008a952c`.

Total host tests across all four modules: **187 pass, 0 fail, 4
skip** (up from 176; the 4 skips are 32-bit-only layout asserts
that fire at compile time on the cross build).

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

- (current) Asset-Register: port FUN_004179b0 SS_MGR slot-clones (94 calls)
- `ea08d2a` Asset-Register: port FUN_0057ca40 4th pass — 443 info-entry pool writes
- `76db8f2` RE: recover PTR_DAT_0056bfa4 jumptable (title-menu phase dispatch)
- `dfdb1cf` RE: FUN_0057ca40 tail memcpy loops are intra-pool info-entry copies
- `6a6e87d` RE: FUN_0057ca40 per-call-site pool indexing confirmed (pool[i] == slot[i])
- `2f36d6a` RE: SS_MGR singleton == input_mgr (both at 0x008a6b60)
- `e748f99` Asset-Register: ar_info_entry pool (909 entries) + allocator finding
- `f8344bb` Asset-Register: port FUN_00582b80 (slot clone) + FUN_00582d00 (info entry clear)
- `efa18c5` tooling: automate Parse C Source headlessly in tag-and-export wrapper
- `8a3629c` WndProc: correct paint_ctx — add +0x16c back_ctx pointer
- `40dc757` WndProc: model 5 deep-engine struct shapes + tag thiscall deps
- `edbaf19` Asset-Register: port FUN_0057ca40 slot-register subset (ar_register_group3_sprites)
- `39d9602` Asset-Register: wire ar_boot_register_all (FUN_00562ea0:613-624)
- `dcc3d15` Asset-Register: port ar_register_palette_ramps (FUN_0057a330)
- `4f89867` bitmap_session: port the PE-resource decoder + palette-ramp wiring
- `8cb9fd8` RE: resolve bitmap_session ECX puzzle + tag the 7 methods

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

With the SS_MGR slot-clones landed, FUN_0057ca40's port is now down
to ONE remaining subsystem: the 9 inline-clone clusters that combine
`ar_sprite_slot_clone` with an inline template-slot init.  The
sprite-slot side of those clusters is still missing; the info-entry
side (5 STRUCT_COPY tails + 4 MARKER_COPY / 4 FLAG_COPY pairs) was
already covered by the 4th-pass info-events table.

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

3. **FUN_0057ca40 last deferred subsystem — 9 FUN_00582b80
   call-cluster wiring** — `ar_sprite_slot_clone` is ported; the
   9 clusters in FUN_0057ca40 that combine it with the inline
   template-slot init now have the storage they need.  The remaining
   gap is mapping each cluster's source/target slot-pool pair (the
   info-entry side of these clusters — the struct copies and
   clear-entry pairs — is covered by the 4th pass).  Each cluster
   constructs a stack-local template slot via inline field-writes,
   passes it as ECX to FUN_00582b80 to clone into a target slot, then
   discards the template.  Modeling needs decomp surgery on each
   cluster's open-coded template setup — not table-driven extraction
   like the previous passes.  See `docs/findings/0057ca40-rabbit-hole.md`
   §4 for the cluster structure.

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
                            + info-entry clear + info-entry pool + 4th-pass
                            info-entry writes + 5th-pass SS_MGR clones
                            + unified-pool accessor) — 30 functions
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
  test_asset_register.c     106 tests for Asset-Register (incl. 7 group3_sprites +
                            8 group3_info_events + 11 SS_MGR-clone + 6 ar_boot_register_all +
                            5 slot-clone + 2 info-entry-clear + 1 info-entry-pool)
  test_bitmap_session.c     31 tests for bitmap_session
  test_wnd_proc.c           20 tests for WndProc

tools/
  frida_capture.py          headless retail harness driver
  frida/opensummoners-agent.js   Frida agent
  run-retail.sh             single-source-of-truth dev loop
  ghidra-tag-and-export.sh  one-shot wrapper for Ghidra re-tag + re-export
  extract/57ca40_sprite_table.py
                            regenerator for the group-3 sprite-register table
  extract/57ca40_info_table.py
                            regenerator for the 4th-pass info-event table
  extract/57ca40_clone_table.py
                            regenerator for the 5th-pass SS_MGR clone table
  extract/57ca40_pool_map.py
                            audit tool for FUN_0057ca40 pool-write coverage
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
  semantics map cleanly.  **FUN_0057ca40's SS_MGR clone subsystem is
  now ported** without modeling the singleton itself — the
  `ar_pool_get_slot` accessor sidesteps the `this` pointer because
  SS_MGR == input_mgr and both pool tables are already host globals.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner — read
  this checkpoint via radare2, 11 entries → 7 distinct phase
  handlers.  See `docs/findings/title-scene.md` "Inner scene-phase
  dispatch" for the resolved table.
- `ar_boot_register_all` exists but **is not yet called from the
  drop-in's WinMain** — every batch (and the wiring) is still a
  module in isolation.  Wiring requires the drop-in to actually own
  the engine init (currently retail does that), which means porting
  the ZDD/ZDS/ZDM device init (`FUN_005b7ee0`, `FUN_005b9cf0`,
  `FUN_005bbb10`) first so we have real device pointers to pass in.
- `g_ar_info_table[909]` — pool modelled in full and the 443 writes
  inside FUN_0057ca40 now PORTED via `ar_apply_group3_info_events`
  (see `docs/findings/0057ca40-rabbit-hole.md` §5 for the FUN_00562ea0
  allocator finding and §6 for FUN_00586010 reader / FUN_00587e00
  writer evidence).  Open follow-ups:
    - DATA_SET payloads (98 events) point to retail PE rdata addresses
      (e.g. 0x006748d0) — stored as opaque uintptr_t in the port.  No
      consumer reads them as bytes yet; when FUN_00586010 (palette
      draw with flag dispatch) lands, these will need extracted PE
      bytes — see `docs/findings/0057ca40-rabbit-hole.md` §6.
    - `ar_info_entry::f_10` semantics — zeroed at alloc; no observed
      write or read in any decompiled function.
    - Pool index 0 (retail addr 0x8a760c, one slot before
      `g_ar_sprite_ramp_slots[0]`) is allocated by the same loop but
      has no observed consumer.  May be a sentinel.
      `ar_pool_get_slot(0)` returns NULL so accidental usage faults
      immediately.
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
- **FUN_0057ca40 last deferred subsystem** (see "Next move" #3):
    - 9 FUN_00582b80 cluster wiring — primitives PORTED
      (`ar_sprite_slot_clone` + `ar_info_entry_clear`); 5 STRUCT_COPY
      info-entry tails PORTED in the 4th pass; sprite-slot side of
      the 9 clusters still pending (open-coded template-slot init per
      cluster — not table-extractable like the SS_MGR clones).
    - The FUN_00587e00 const-data-pointer refresh routine is a
      potential CONSUMER of `entry->data` (writes it at runtime); not
      ported yet — first audit on what reads back the rewritten pointer.

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
