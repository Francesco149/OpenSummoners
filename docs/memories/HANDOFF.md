# Session handoff — last updated 2026-05-25 (CreateScreen mode-dispatch MAPPED + 4 helpers PORTED)

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
- **ZDD wrapper** — **22 functions** covering:
    - ZDD class lifecycle: ctor / dtor / child-release loop /
      cursor-restore / DDERR log builder / DirectDrawCreateEx /
      SetCooperativeLevel / top-level create driver
    - ZDDObject leaf lifecycle: ctor / dtor / pixel-buf release
    - Surface alloc primitives: `zdd_build_surface_desc` (pure
      DDSURFACEDESC2 builder) + `zdd_create_surface` (Win32
      CreateSurface + SetPalette wrapper)
    - Surface-alloc stampers: prefill_desc / stamp_metrics /
      set_color_key + orchestrator (create_surface_pair)
    - Public factory `zdd_object_new` (operator_new + ctor +
      orchestrator + cleanup-on-failure)
    - Clipper attach `zdd_object_attach_clipper`
    - **NEW this checkpoint**: 4 leaf helpers for CreateScreen —
      `zdd_hide_cursor` (FUN_005b8dd0, 33B), `zdd_set_display_mode`
      (FUN_005b8900, 74B), `zdd_get_display_mode` (FUN_005b8950,
      126B), `zdd_busy_wait_ms` (FUN_005b5ac0, 39B).

  Pure logic in `src/zdd.c`.  Win32 primitives in `src/zdd_win32.c`.

Total host tests across all modules: **265 pass, 0 fail, 6 skip**
(up from 256; 9 new — 3 hide_cursor, 2 SetDisplayMode, 3
GetDisplayMode, 1 busy_wait).  Cross-build with mingw clean.

**Documentation landed this checkpoint**: `docs/findings/ddraw-init.md`
section "FUN_00582e90 — mode-dispatch CreateScreen" now has the full
5-mode table (Full / Safe / Windowed / DB Mode / Zoom), the prologue
(release prior DAT_008a6ec0 ZDDObject), the centre-rect math for
mode 4, and 7 fixed error-string mappings.  Mode 0 (640×480, 16bpp)
is the boot path and the only branch that calls our already-ported
`zdd_object_new`.  Modes 1–4 all dispatch through `FUN_005b8480`
(1088 bytes — still unported, the next big piece).

**Open RE thread closed this checkpoint**: `FUN_005b9390` is exactly
our existing `zdd_object_dtor` (same body byte-for-byte).  The pair
`FUN_005b9390 + FUN_005bef0e` in FUN_00582e90's prologue is the same
as `zdd_obj_destroy(&DAT_008a6ec0)`.

**Ghidra C++ recovery infrastructure** — unchanged.  Kaiju +
`tools/ghidra-scripts/TagThiscallFunctions.java` + headless
`tools/ghidra-tag-and-export.sh`.  26 functions tagged.

Most recent commits (newest first):

- (current) ZDD: port 4 CreateScreen leaf helpers + map mode dispatch
- `7f5a001` ZDD: port FUN_005b9520 clipper attach (create + clear + attach)
- `6024a36` ZDD: port FUN_005b8b40 CreateSurfacePair factory + orchestrator returns int
- `1348360` ZDD: port FUN_005b95c0 + 97e0 + 98c0 + 9830 surface-alloc stampers + orchestrator
- `d87b7ea` ZDD: port FUN_005b8c00 DDSURFACEDESC2 builder + CreateSurface
- `19e4e6c` ZDD: port ZDDObject ctor + dtor + LocalFree pixel-buf helper
- `ce6b87e` ZDD: port ctor/dtor + DDERR log helper + DirectDraw init wrappers
- `90de1ba` Pixel-Drawer: port boot-time slot tables (5 groups + 4 special)
- `5377460` Asset-Register: port FUN_0057ca40 6th pass — 9 inline slot-clones

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

With the 5-mode table mapped and 4 leaf helpers in place, the next
strategic piece is the big inner surface-init that all 5 modes
funnel through.

1. **(recommended) `FUN_005b8480` — internal mode-aware surface-init**
   (1088 bytes).  The single load-bearing function FUN_00582e90's
   per-mode branches all call.  Stamps params on the ZDD struct
   (+0x134/+0x140/+0x144/+0x164/+0x168 + a 7-int rect at +0x148..),
   conditionally creates a primary surface via IDirectDraw7::CreateSurface
   with a backbuffer-flavoured DDSURFACEDESC2, dispatches by mode
   to create 1/2/3 ZDDObjects on the ZDD's three slots (+0x16c
   primary, +0x18 back_obj_a, +0x1c back_obj_b), calls
   `FUN_005b9740` (still unported — the "back-buffer-fetch via
   GetAttachedSurface" helper) for mode 0/3 and the first leg of
   mode 4, plus `FUN_005b95c0` (our orchestrator) for the rest.
   On success: calls `FUN_005b9520` (our clipper attach!) and a
   16bpp-only `FUN_005b8a20` (pixel-format binding, unported).
   Multi-checkpoint port — start by mapping the mode tree.

2. **`FUN_00582e90` — outer CreateScreen dispatcher** (3560 bytes).
   Doable in one shot once FUN_005b8480 is in.  Most of the bulk is
   inlined strcpy/strcat error-message construction we can
   collapse to a single helper.

3. **`FUN_00586010` palette-draw consumer** — first ported reader of
   the `ar_info_entry` pool.  Big function (1035 lines, 61 unique
   FUN_ callees) — would be a multi-checkpoint port and most callees
   are unported.  Defers the "consumer pins flag semantics" thread.

4. **`FUN_00563ef0` wave-load half** — defer until we have a reason
   to load sound bytes (i.e. once title scene starts playing audio).
   Big DSound+mmio+resource mock layer for code that is dead at boot.

## Active modules / file layout

```
src/
  main.c                    WinMain shim, single-instance, --hide-window/--frames
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 8 functions
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
                            orchestrator (create_surface_pair) +
                            factory (zdd_object_new) + clipper attach
                            (zdd_object_attach_clipper) +
                            hide_cursor (zdd_hide_cursor).
                            18 pure-logic leaf functions.
  zdd_win32.c               DirectDrawCreateEx/SetCooperativeLevel +
                            SetDisplayMode + GetDisplayMode +
                            CreateSurface/SetPalette + SetColorKey +
                            CreateClipper/SetClipList/SetClipper +
                            ShowCursor/OutputDebugStringA/IUnknown::Release
                            + LocalFree + busy_wait_ms (GetTickCount spin).
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
  test_zdd.c                67 tests for ZDD + ZDDObject lifecycle +
                            DDSURFACEDESC2 builder + surface-alloc
                            stampers + orchestrator + factory +
                            clipper attach + hide_cursor +
                            SetDisplayMode + GetDisplayMode +
                            busy_wait_ms (2 32-bit-only layout skips)

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

- **FUN_005b8480** (1088 bytes) — the recommended next-port.  Mode-
  aware internal surface-init.  Reads/writes a wide swath of the ZDD
  struct (params: +0x134/+0x140/+0x144/+0x164/+0x168, rect-blob:
  +0x148..+0x164, ZDDObject slots: +0x16c/+0x18/+0x1c).  Calls
  `FUN_005b9740` (unported — back-buffer GetAttachedSurface helper)
  and our `FUN_005b95c0` (orchestrator).  Builds a backbuffer-flavoured
  DDSURFACEDESC2 with dwCaps=0x218 / 0x21 / 0xa18 depending on mode +
  videomem_flag.  Calls `FUN_005b8e00` (unported) for 8bpp.  On
  success: `FUN_005b9520` (our clipper attach) + 16bpp-only
  `FUN_005b8a20` (unported — pixel-format binding).
- **FUN_00582e90** (3560 bytes) — outer CreateScreen dispatcher.
  All 5 mode branches mapped in ddraw-init.md.  Body is mostly
  inlined strcpy/strcat error logging (the actual logic per branch
  is ≤ 10 lines).  Port after FUN_005b8480.
- ZDDObject's +0xac field (`com_back`) is dual-role: holds either a
  back-buffer IDirectDrawSurface7 (per the dtor's release order
  comment) OR an IDirectDrawClipper (per FUN_005b9520's stash).  Both
  implement IUnknown so the lifecycle path doesn't care, but field
  naming is misleading.  Consider a rename to `com_attached` or an
  anonymous union once a consumer cares which it is.
- FUN_005b9520's vtable[7] call site passes a pointer to a
  stack-local NULL (in retail: `piStack_40 = NULL;
  clipper->vtable[0x1c](clipper, &piStack_40, 0)`).  Standard
  IDirectDrawClipper has SetClipList at vtable[7] / byte 0x1c —
  ddraw-init.md flags this as ambiguous (could also be SetHWnd at
  vtable[8] = byte 0x20 if Ghidra got the offset wrong).  Our port
  mirrors the literal vtable+0x1c call.  Frida verification
  recommended once the harness runs the clipper-attach path live.
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
  disasm-level confirmation.  **Now reachable** as a TODO
  inside `zdd_object_set_color_key` — the 16bpp branch is currently
  a no-op; wire FUN_005b8b00's converted output once the
  descriptor's identity is pinned.
- ZDD's two opaque COM handles (+0x128 `com_a`, +0x12c `com_b`)
  partially resolved: `com_b` is now READ by `zdd_create_surface` as
  a palette and bound via vtable[31] (SetPalette).  `com_a` still
  unpinned.  The clipper attach FUN_005b9520 stores the clipper on
  the ZDDObject's +0xac, NOT the parent ZDD's com_a.
- ZDD's pixel-format hint fields (`pixel_format_mode` at +0x164,
  `pixel_format_bpp` at +0x168) are now READ by zdd_build_surface_desc
  AND zdd_object_set_color_key but still WRITTEN by no ported path.
  `FUN_005b8480` (the next port) is what stamps them.
- ZDD's `videomem_flag` at +0x134 — same status: read by the builder,
  not yet written by any port.  Also stamped by `FUN_005b8480`.
- `FUN_005bd040` mode 3 / mode 4 LUT formulas have arithmetic whose
  "floor-correction" terms are zero for valid weight ranges but kept
  literally in the port.  Audit if out-of-range weights ever flow.
- SS_MGR / W_MGR / GD_MGR boot-pool allocators (DAT_008a8440 / _6ec4
  / _9274) are dependency-of ~30 functions; defer until consumer
  semantics map cleanly.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner — read
  via radare2, 11 entries → 7 distinct phase handlers.  See
  `docs/findings/title-scene.md` "Inner scene-phase dispatch".
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
