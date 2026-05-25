# Session handoff — last updated 2026-05-25 (surface-alloc stampers + orchestrator PORTED)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The harness boots retail headlessly + the engine reaches per-frame
ticks under `--turbo --hide-window`.  Phase 1 surface mapping and
Phase 2 file-format extraction are complete enough to support
methodical port-and-test.  **Five modules ported now:**

- **Pixel-Drawer** — 8 functions including the boot driver (69 fixed
  slots in 5 groups + 4 special-colour writes — see `winmain-and-
  bootstrap.md` "Pixel Drawer slot tables"), 39 host tests passing.
- **Asset-Register** — 31 functions ported including the boot-driver
  wiring (see PROGRESS for the full FUN_0057ca40 breakdown).  Pure
  logic with GDI wrappers split into `asset_register_win32.c`.
- **Bitmap-Session** — 8 functions (7 thiscalls + 1 free function
  FUN_005b7c10).  Pure PE-resource bitmap decoder, Win32-free body.
- **WndProc** — `FUN_005b12e0` (the engine's main game window
  message handler).  9-message dispatch including the load-bearing
  WM_ACTIVATEAPP that owns `DAT_008a952c`.
- **ZDD wrapper** — **16 functions** covering the full DirectDraw 7
  wrapper class lifecycle + ZDDObject leaf lifecycle + the DDSD
  builder + the surface-alloc orchestrator stack:
    - ZDD class: ctor / dtor / child-release loop / cursor-restore /
      DDERR log builder / DirectDrawCreateEx / SetCooperativeLevel /
      top-level create driver
    - ZDDObject leaf: ctor / dtor / pixel-buf release
    - Surface alloc primitives: `zdd_build_surface_desc` (pure
      DDSURFACEDESC2 builder) + `zdd_create_surface` (Win32
      CreateSurface + SetPalette wrapper) — ports FUN_005b8c00
    - **NEW this checkpoint**: surface-alloc stampers + orchestrator:
      - `zdd_object_prefill_desc` (FUN_005b97e0, 66 bytes) — caches
        caps_in / force_videomem_in at +0xcc/+0xd0, zeros the
        embedded DDSD, stamps three self-pointers into DDSD field
        offsets, sets dwSize = 0x7c.
      - `zdd_object_stamp_metrics` (FUN_005b98c0, 73 bytes) —
        10-dword window-fit metric stash across +0x0c/+0x10/+0x14/
        +0x18/+0x1c/+0x20 + +0xb0/+0xb4/+0xb8/+0xbc.
      - `zdd_object_set_color_key` (FUN_005b9830, 138 bytes) —
        0x1ffffff sentinel branch (clears state_flag + short-
        circuits the vtable call) vs real-key branch (stamps
        state_flag = 0x8000, calls IDirectDrawSurface7::SetColorKey
        with DDCKEY_SRCBLT).  Win32 leg in `zdd_win32.c`.
      - `zdd_object_create_surface_pair` (FUN_005b95c0, 110 bytes) —
        pure orchestration over the 4 helpers above.

  Pure logic in `src/zdd.c` (ctor + dtor decision tree + DDERR-to-
  string + log builder + ZDDObject lifecycle + descriptor build +
  stampers + orchestrator).  Win32 primitives (DDraw CreateSurface/
  SetPalette/SetColorKey/Coop/CreateEx, COM Release, ShowCursor,
  OutputDebugStringA, LocalFree) in `src/zdd_win32.c`.  FUN_005b8b40
  (CreateSurfacePair wrapper around operator_new + ctor + orchestrator)
  NOT in this slice; deferred to the next checkpoint.

Total host tests across all five modules: **246 pass, 0 fail, 6
skip** (up from 234; 12 new pure-logic stamper + orchestrator tests).

**Open RE thread closed this checkpoint**: the ZDDObject struct now
has names for 21 of its fields (up from 8 last checkpoint) —
specifically the three self-pointers at +0x00/+0x04/+0x08, the six
metric slots at +0x0c..+0x20, the two colorkey slots at +0x24/+0x28,
the four secondary metrics at +0xb0..+0xbc, and the two cached
create-time args at +0xcc/+0xd0.  Only `embedded_ddsd` (the 124-byte
scratch DDSURFACEDESC2 at +0x30..+0xab) remains opaque.

**Ghidra C++ recovery infrastructure** — unchanged from last
checkpoint.  Kaiju + `tools/ghidra-scripts/TagThiscallFunctions.java`
+ headless `tools/ghidra-tag-and-export.sh`.  26 functions tagged.

Most recent commits (newest first):

- (current) ZDD: port FUN_005b95c0 + 97e0 + 98c0 + 9830 surface-alloc stampers + orchestrator
- `d87b7ea` ZDD: port FUN_005b8c00 DDSURFACEDESC2 builder + CreateSurface
- `19e4e6c` ZDD: port ZDDObject ctor + dtor + LocalFree pixel-buf helper
- `ce6b87e` ZDD: port ctor/dtor + DDERR log helper + DirectDraw init wrappers
- `90de1ba` Pixel-Drawer: port boot-time slot tables (5 groups + 4 special)
- `5377460` Asset-Register: port FUN_0057ca40 6th pass — 9 inline slot-clones
- `63e14bb` Asset-Register: port FUN_0057ca40 5th pass — 94 SS_MGR slot-clones
- `ea08d2a` Asset-Register: port FUN_0057ca40 4th pass — 443 info-entry pool writes
- `76db8f2` RE: recover PTR_DAT_0056bfa4 jumptable (title-menu phase dispatch)
- `dfdb1cf` RE: FUN_0057ca40 tail memcpy loops are intra-pool info-entry copies

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

With the surface-alloc orchestrator landed, the natural next layer up
is `FUN_005b8b40` (CreateSurfacePair) — the small wrapper that
operator_new's a ZDDObject, runs its ctor, then calls the orchestrator.
After that the immediate next layers are the clipper attach and the
mode-dispatch CreateScreen path.

1. **(recommended) `FUN_005b8b40` — CreateSurfacePair wrapper**.
   The next layer up.  Per `docs/findings/ddraw-init.md`, it's
   roughly: `zdo = operator_new(0xd8); zdd_object_ctor(zdo, ...);
   if (!zdd_object_create_surface_pair(zdo, w, h, 0, pixelFmtFlags,
   count, 0, 0, w, h)) { delete zdo; return 0; } *out = zdo; return
   1;`.  Small (likely <100 bytes from the docs).  No new Win32
   primitives needed — everything is already in `zdd.c`/`zdd_win32.c`.
   Pre-req met: zdd_object_create_surface_pair is now callable.

2. **`FUN_005b9520` — Clipper attach** (per ddraw-init.md, 87 bytes).
   Independent of the surface-alloc tree — just creates an
   IDirectDrawClipper, calls SetHWnd, then attaches to a surface.
   Bonus the next time a ZDDObject port lands, since the clipper
   ends up bound to com_primary.

3. **`FUN_00582e90` — mode-dispatch CreateScreen** (3560 bytes).
   The next big consumer of zdd_object_create_surface_pair.  Per
   ddraw-init.md, it switches on state->offset_0x04 (the launcher
   frame style) and dispatches to mode 0..4 each of which builds a
   different surface layout.  Multi-checkpoint; would benefit from a
   structured walk to map out the 5 mode branches before porting.

4. **Wire ar_boot_register_all + pd_boot_init_slots into the drop-in's
   WinMain.** — currently every batch is a module in isolation; the
   functions exist but no real drop-in caller invokes them.  Wiring
   requires the drop-in to own the engine init (currently retail does
   this).  Pre-req: complete enough ZDD/ZDS/ZDM init.  Without those,
   the calls can run with stub pointers but have no observable effect
   because the drop-in's drawing path is still retail's.

5. **`FUN_00586010` palette-draw consumer** — first ported reader of
   the `ar_info_entry` pool.  Big function (1035 lines, 61 unique
   FUN_ callees) — would be a multi-checkpoint port and most callees
   are unported.  Closes the "no consumer reads info-entry fields
   yet" open thread; pins per-prefix flag semantics (the 0/1/2/3
   dispatch) from the consumer side.

6. **`FUN_00563ef0` wave-load half** — defer until we have a reason
   to load sound bytes (i.e. once title scene starts playing audio).
   Big DSound+mmio+resource mock layer for code that is dead at boot.

## Active modules / file layout

```
src/
  main.c                    WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 8 functions (7 leaf primitives +
                            pd_boot_init_slots boot driver for 69+4 slots)
  asset_register.c/h        Asset-register slots — 31 functions
  asset_register_win32.c    GDI primitive wrappers
  bitmap_session.c/h        PE-resource bitmap decoder — 8 functions
  bitmap_session_win32.c    LocalAlloc/Free + FindResource/LoadResource/LockResource
  wnd_proc.c/h              Main game window WndProc — pure dispatch
  wnd_proc_win32.c          DefWindowProcA + ExitProcess + 5 placeholder hooks
  zdd.c/h                   ZDD wrapper — ctor/dtor + DDERR log + create
                            driver + ZDDObject ctor/dtor/pixel-buf-release
                            + DDSD builder + surface-alloc stampers
                            (prefill / metrics / set_color_key) +
                            orchestrator (create_surface_pair).  15 pure-
                            logic leaf functions.
  zdd_win32.c               DirectDrawCreateEx/SetCooperativeLevel +
                            CreateSurface/SetPalette + SetColorKey +
                            ShowCursor/OutputDebugStringA/IUnknown::Release
                            + LocalFree
  Makefile                  single-TU mingw cross-build (-lddraw -ldxguid)

tests/
  Makefile                  host gcc + ASan/UBSan; `make -C tests run`
                            filter by name with `F=<substr>`
  t.h                       T_ASSERT_* macros, 0/1/2 = pass/fail/skip
  test_main.c               X-macro registry; one X(name) per test
  test_pixel_drawer.c       39 tests for Pixel-Drawer
  test_asset_register.c     111 tests for Asset-Register
  test_bitmap_session.c     31 tests for bitmap_session
  test_wnd_proc.c           20 tests for WndProc
  test_zdd.c                48 tests for ZDD + ZDDObject lifecycle +
                            DDSURFACEDESC2 builder + surface-alloc
                            stampers + orchestrator (2 32-bit-only
                            layout skips)

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
  extract/57ca40_inline_clone_table.py
                            regenerator for the 6th-pass inline-clone table
  extract/57ca40_pool_map.py
                            audit tool for FUN_0057ca40 pool-write coverage
```

## Open RE threads (not picked up yet)

- ZDDObject's 124-byte embedded DDSURFACEDESC2 (+0x30..+0xab) is
  still modelled as `uint8_t[]` — typed access requires pulling
  <ddraw.h> into the host build or defining a parallel struct.
  Deferred until a ported function actually reads a DDSD field by
  name (the Lock() path is the candidate).
- ZDDObject's two metric clusters now have field names but the
  *semantic* role of each slot remains uncertain.  The names
  (`metric_0c`/etc, `metric_b0`/etc) reflect byte offsets, not
  meaning.  Likely "src rect TL/BR + dst rect TL/BR" pairs based on
  the orchestrator's argument shape, but no consumer reads them yet.
  Pin once the first Blt/draw caller lands.
- `FUN_005b8b00` (16bpp color-channel shift converter) reads byte
  shift tables off ECX — looks like it expects a "pixel format
  descriptor" object as `this`, NOT the calling ZDDObject.  Needs
  disasm-level confirmation (Ghidra's `in_ECX` annotation is just
  "whatever's in ECX at function entry"; the compiler may have set
  ECX explicitly before the call).  **Now reachable** as a TODO
  inside `zdd_object_set_color_key` — the 16bpp branch is currently
  a no-op (passes raw key through); wire FUN_005b8b00's converted
  output once the descriptor's identity is pinned.  Note: at boot
  the orchestrator hits the sentinel path so this branch is dead
  code right now.
- ZDD's two opaque COM handles (+0x128 `com_a`, +0x12c `com_b`)
  partially resolved: `com_b` is now READ by `zdd_create_surface` as
  a palette and bound via vtable[31] (SetPalette), so it's almost
  certainly `IDirectDrawPalette*`.  `com_a` still unpinned — likely
  `IDirectDrawClipper*` given `FUN_005b9520` attaches a clipper to a
  surface; confirm when its setter lands.
- ZDD's pixel-format hint fields (`pixel_format_mode` at +0x164,
  `pixel_format_bpp` at +0x168) are now READ by zdd_build_surface_desc
  AND zdd_object_set_color_key (the 16bpp branch checks pixel_format_bpp)
  but still WRITTEN by no ported path.  The higher-level mode-dispatch
  FUN_00582e90 is what stamps them during fullscreen-mode init.
- ZDD's `videomem_flag` at +0x134 — same status: read by the builder,
  not yet written by any port.
- `FUN_005bd040` mode 3 / mode 4 LUT formulas have arithmetic whose
  "floor-correction" terms are zero for valid weight ranges but kept
  literally in the port.  Audit if out-of-range weights ever flow.
  Note: the boot driver does pass per-channel weights up to 2000 for
  group-C grey-ramp and special D slots — these reach mode 1/2 LUTs
  (via the slot-level `mode` field) so the audit case is now closer
  to "live", but no rendering consumer reads them yet.
- SS_MGR / W_MGR / GD_MGR boot-pool allocators (DAT_008a8440 / _6ec4
  / _9274) are dependency-of ~30 functions; defer until consumer
  semantics map cleanly.  FUN_0057ca40's SS_MGR clone + inline
  clone subsystems are ported without modeling the singleton itself.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner — read
  via radare2, 11 entries → 7 distinct phase handlers.  See
  `docs/findings/title-scene.md` "Inner scene-phase dispatch" for
  the resolved table.
- `ar_boot_register_all` **and** `pd_boot_init_slots` exist but
  **neither is called from the drop-in's WinMain** — every batch
  is still a module in isolation.  Wiring requires the drop-in to
  actually own the engine init (currently retail does that), which
  means porting the ZDD/ZDS/ZDM device init first.
- `g_ar_info_table[909]` — pool modelled in full and the 443 writes
  inside FUN_0057ca40 are PORTED via `ar_apply_group3_info_events`.
  Open follow-ups (DATA_SET payloads, f_10 semantics, pool index 0)
  — see `docs/findings/0057ca40-rabbit-hole.md`.
- `ar_locale_state` modelling: the locale loop's three globals
  (DAT_008a6e68, _6e70, *DAT_008a6e80+0x1c8) are passed in as a
  struct in our port.  The boot driver port will need to read them
  from the real BSS / launcher-settings record.
- The WndProc port is also a module in isolation — `wp_handle_message`
  is not wired into any RegisterClassExA call yet.
- `FUN_00563ef0` wave-load second half is unported.
- The 5 "deep engine" structs the WndProc port depends on
  (paint_ctx, input_dev, zdm + zdm_entry, input_mgr, log_singleton)
  now have field-offset shells in `src/wnd_proc.h`.
- Locale-table magic / sequence fields: 23 distinct values for the
  +0x00 magic field, +0x04 is per-locale group selector 1..73.
  Revisit when porting the scene loader.

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
