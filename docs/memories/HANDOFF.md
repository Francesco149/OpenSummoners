# Session handoff — last updated 2026-05-25 (back-buffer attach + 8bpp palette PORTED)

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
- **ZDD wrapper** — **24 functions** covering:
    - ZDD class lifecycle: ctor / dtor / child-release loop /
      cursor-restore / cursor-hide / DDERR log builder /
      DirectDrawCreateEx / SetCooperativeLevel / SetDisplayMode /
      GetDisplayMode / busy_wait_ms / top-level create driver
    - **NEW: 8bpp palette** `zdd_setup_8bit_palette` (FUN_005b8e00,
      157B): GetSystemPaletteEntries + CreatePalette stash on com_b
      + conditional SetPalette on com_a (the ZDD primary display
      surface). Closes the "+0x128 com_a role" open thread —
      `com_a` is the primary display surface (IDirectDrawSurface7)
      that FUN_005b8480 mode 0/3/4 stamps directly on the ZDD.
    - ZDDObject leaf lifecycle: ctor / dtor / pixel-buf release
    - Surface alloc primitives: `zdd_build_surface_desc` (pure
      DDSURFACEDESC2 builder) + `zdd_create_surface` (Win32
      CreateSurface + SetPalette wrapper)
    - Surface-alloc stampers: prefill_desc / stamp_metrics /
      set_color_key + orchestrator (create_surface_pair)
    - Public factory `zdd_object_new` (operator_new + ctor +
      orchestrator + cleanup-on-failure)
    - Clipper attach `zdd_object_attach_clipper`
    - **NEW: back-buffer attach** `zdd_object_attach_backbuffer`
      (FUN_005b9740, 153B): prefill_desc(0,0) + GetAttachedSurface
      vtable[12] with DDSCAPS_BACKBUFFER (|VIDEOMEMORY if force_vm)
      + stamp_metrics(w,h,0,0,w,h) + set_color_key(sentinel).
      Stashes the attached surface in ZDDObject's com_primary slot.

  Pure logic in `src/zdd.c`.  Win32 primitives in `src/zdd_win32.c`.

Total host tests across all modules: **272 pass, 0 fail, 6 skip**
(up from 265; 7 new — 3 attach_backbuffer, 4 setup_8bit_palette).
Cross-build with mingw clean.

**Open RE thread closed this checkpoint**: ZDD `com_a` (+0x128) is
the primary display IDirectDrawSurface7 — FUN_005b8e00's SetPalette
target.  FUN_005b8480 mode 0/3/4 allocates it directly via
`ddraw7->CreateSurface(..., &this->com_a, NULL)`.  Was previously
unpinned ("likely IDirectDrawPalette + IDirectDrawClipper or
similar" — neither, it's the primary surface).

**Ghidra C++ recovery infrastructure** — unchanged.  Kaiju +
`tools/ghidra-scripts/TagThiscallFunctions.java` + headless
`tools/ghidra-tag-and-export.sh`.  26 functions tagged.

Most recent commits (newest first):

- (current) ZDD: port FUN_005b9740 back-buffer attach + FUN_005b8e00 8bpp palette
- `8a9d536` ZDD: port 4 CreateScreen leaf helpers + map mode dispatch
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

With both of FUN_005b8480's surface-helpers ported (`_9740` back-
buffer attach + `_8e00` 8bpp palette), only one dependency remains:
`FUN_005b8a20` (16bpp pixel-format binding, 181B) — and its ECX
identity is ambiguous (open RE thread; not the calling ZDDObject).
We can port FUN_005b8480 NOW and leave the 16bpp branch as a
TODO/stub, since the boot path is mode 0 with bpp=16 calling it.

1. **(recommended) `FUN_005b8480` — internal mode-aware surface-init**
   (1088 bytes).  Stamps params on the ZDD struct (+0x134/+0x140/
   +0x144/+0x164/+0x168 + 7-int rect at +0x148..+0x164).  Calls
   `FUN_005b8040` (our release-children prologue) first, then
   conditionally builds a backbuffer-flavoured DDSURFACEDESC2 and
   calls IDirectDraw7::CreateSurface to stamp ZDD's `com_a`
   (primary display surface) — only when param_4 != 2 (skip in
   Windowed mode).  Dispatches by mode_arg (0/3/4 use back-buffer
   GetAttachedSurface, mode 4 also a CreateSurface variant) to
   allocate 1/2/3 ZDDObjects on the ZDD's three slots (+0x16c
   primary, +0x18 back_obj_a, +0x1c back_obj_b) via our already-
   ported helpers (`zdd_object_ctor` + `zdd_object_attach_backbuffer`
   + `zdd_object_create_surface_pair`).  On success: calls our
   `zdd_object_attach_clipper` and a 16bpp-only `FUN_005b8a20`
   (TODO — pixel-format binding, ECX identity unclear).  Calls
   `zdd_setup_8bit_palette` when bpp==8.  Single-checkpoint port
   now that all leaves except _8a20 are in place.

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
                            back-buffer attach (zdd_object_attach_backbuffer)
                            + 8bpp palette (zdd_setup_8bit_palette)
                            + hide_cursor.  20 pure-logic leaf funcs.
  zdd_win32.c               DirectDrawCreateEx/SetCooperativeLevel +
                            SetDisplayMode + GetDisplayMode +
                            CreateSurface/SetPalette + SetColorKey +
                            CreateClipper/SetClipList/SetClipper +
                            GetAttachedSurface +
                            CreatePalette (from system) +
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
  test_zdd.c                74 tests for ZDD + ZDDObject lifecycle +
                            DDSURFACEDESC2 builder + surface-alloc
                            stampers + orchestrator + factory +
                            clipper attach + hide_cursor +
                            SetDisplayMode + GetDisplayMode +
                            busy_wait_ms + back-buffer attach +
                            8bpp palette setup (2 32-bit-only skips)

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
  +0x148..+0x164, ZDDObject slots: +0x16c/+0x18/+0x1c).  All leaf
  dependencies now ported except `FUN_005b8a20` (16bpp pixel-format
  binding, 181B — ECX identity ambiguous, possibly a global pixel-
  format descriptor not the ZDDObject).  Plan: port the body with
  the 16bpp path stubbed/TODO.  Builds a backbuffer-flavoured
  DDSURFACEDESC2 with dwCaps=0x218 / 0x21 / 0xa18 depending on mode +
  videomem_flag (the primary surface goes directly on the ZDD's
  `com_a` slot).
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
  now fully resolved: `com_a` is the ZDD primary display
  IDirectDrawSurface7 (allocated by FUN_005b8480 mode 0/3/4 via
  `ddraw7->CreateSurface(..., &this->com_a, NULL)`).  `com_b` is
  the IDirectDrawPalette (allocated by `zdd_setup_8bit_palette`
  via `ddraw7->CreatePalette(..., &this->com_b, NULL)`).  The
  clipper attach FUN_005b9520 stores the clipper on the ZDDObject's
  +0xac, NOT the parent ZDD's com_a (so com_a stays the primary).
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
