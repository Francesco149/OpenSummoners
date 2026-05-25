# Session handoff — last updated 2026-05-25 (cs_dispatch_create_screen PORTED)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The harness boots retail headlessly + the engine reaches per-frame
ticks under `--turbo --hide-window`.  Phase 1 surface mapping and
Phase 2 file-format extraction are complete enough to support
methodical port-and-test.  **Six modules ported now:**

- **Pixel-Drawer** — 8 functions, 39 host tests.
- **Asset-Register** — 31 functions, 111 host tests.
- **Bitmap-Session** — 8 functions, 31 host tests.
- **WndProc** — `FUN_005b12e0`, 20 host tests.
- **ZDD wrapper** — **26 functions** covering ctor/dtor + DDERR log
  + DirectDrawCreateEx + SetCooperativeLevel + SetDisplayMode +
  GetDisplayMode + busy_wait + clipper attach + back-buffer attach
  + 8bpp palette setup + primary-surface DDSD builder +
  `zdd_create_screen` (full 5-mode dispatch).  88 host tests.
- **NEW: cs_dispatch** — `cs_dispatch_create_screen` (FUN_00582e90,
  3560B outer mode dispatcher).  Per-mode driver around
  `zdd_create_screen` with the correct preamble for each launcher
  mode + the centre-rect math for Zoom + the shared error-log
  builder.  21 host tests.

Total host tests across all modules: **307 pass, 0 fail, 6 skip**
(up from 286; 21 new — see PROGRESS.md head entry for the
breakdown).  Cross-build with mingw clean.

**Open RE threads closed this checkpoint**:
- `DAT_008a6ec0` → `cs_primary_pair` (mode-0 ZDDObject*; module global)
- `DAT_008a9534` / `DAT_008a9530` → `cs_log_buf` (0x638 bytes) /
  `cs_log_dirty` (flag); module globals
- `DAT_008a6eac` / `DAT_008a6eb0` → `cs_zoom_override_width` /
  `cs_zoom_override_height` (mode-4 display-mode overrides)
- `DAT_008a9b6c` → `cs_engine_name_header` (the engine-name log
  header; empty at boot, populated by an unported settings path)

**Remaining gap**: 16bpp pixel-format binding via `FUN_005b8a20`
(181B) is a TODO inside `zdd_create_screen`'s post-success hook.
The 16bpp boot path (mode 0, bpp 16) returns success without the
binding — visible output may need this once the harness runs live.
ECX identity is ambiguous (open RE thread).

**Ghidra C++ recovery infrastructure** — unchanged.

Most recent commits (newest first):

- (current) cs_dispatch: port FUN_00582e90 outer CreateScreen
  dispatcher (5-mode driver around zdd_create_screen)
- `8b9e539` ZDD: port FUN_005b8480 CreateScreen body (5-mode dispatch)
- `d415dd5` ZDD: port FUN_005b9740 back-buffer attach + FUN_005b8e00 8bpp palette
- `8a9d536` ZDD: port 4 CreateScreen leaf helpers + map mode dispatch
- `7f5a001` ZDD: port FUN_005b9520 clipper attach (create + clear + attach)
- `6024a36` ZDD: port FUN_005b8b40 CreateSurfacePair factory + orchestrator returns int
- `1348360` ZDD: port FUN_005b95c0 + 97e0 + 98c0 + 9830 surface-alloc stampers + orchestrator
- `d87b7ea` ZDD: port FUN_005b8c00 DDSURFACEDESC2 builder + CreateSurface
- `19e4e6c` ZDD: port ZDDObject ctor + dtor + LocalFree pixel-buf helper
- `ce6b87e` ZDD: port ctor/dtor + DDERR log helper + DirectDraw init wrappers

## Active goal

**Replicate the engine's init sequence in the drop-in until the title
menu actually renders.**  Port modules in dependency order; each
ported function gets unit tests in `tests/test_*.c`.  The sibling
**openrecet** at `/opt/src/openrecet` is the model for porting style.

## Next move (pick one — recommendation first)

The boot-time graphics init chain is now end-to-end pure-logic
ported through to the CreateScreen mode dispatch.  The natural
next checkpoint is one of: (a) wire what's ported into the drop-in
to actually run a live boot through it, OR (b) port the remaining
inner gap (`FUN_005b8a20`).

1. **(recommended) Wire `cs_dispatch_create_screen` into the
   drop-in's `WinMain`**.  Currently `src/main.c` only does the
   single-instance + window-management plumbing; the engine init
   is still owned by retail under the harness.  This is the
   visible-output checkpoint we've been building toward — once
   wired, `--hide-window --frames N` should drive the dispatcher
   through to a real `IDirectDraw7::CreateSurface` and the title
   menu should start rendering.  Blocking work: parse the launcher
   settings record (FUN_005a4770's read path — 45 KB function;
   needs r2 or chopped re-decomp) OR hardcode mode=2 (Windowed)
   for the first wiring pass.  Hardcoded mode=2 is the lowest-risk
   first step.

2. **`FUN_005b8a20` — 16bpp pixel-format binding** (181 bytes).
   The one remaining un-ported call inside `zdd_create_screen`.
   ECX identity is ambiguous — the function reads byte-shift tables
   off ECX that look like a "pixel format descriptor" object, NOT
   the calling ZDDObject.  Investigation: r2 disasm + Frida hook to
   log the ECX value at the first live call.  Port is ~30 lines
   once identity is pinned.  Better to defer until live-boot tells
   us 16bpp actually fails without it.

3. **`FUN_00586010` palette-draw consumer** — first ported reader
   of the `ar_info_entry` pool.  Big function (1035 lines, 61
   unique FUN_ callees) — multi-checkpoint port with mostly
   unported callees.  Defer until consumer semantics matter.

4. **`FUN_00563ef0` wave-load half** — defer until we have a reason
   to load sound bytes (title scene audio).

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
                            driver + ZDDObject lifecycle + DDSD builder +
                            surface-alloc stampers + orchestrator +
                            factory + clipper attach + back-buffer attach +
                            8bpp palette setup + primary-surface DDSD
                            builder + zdd_create_screen + leaf helpers.
                            23 pure-logic functions.
  zdd_win32.c               DirectDraw7 + IDirectDrawSurface7 +
                            IDirectDrawClipper + IDirectDrawPalette
                            primitive wrappers, ShowCursor,
                            OutputDebugStringA, IUnknown::Release,
                            LocalFree, busy_wait_ms.
  cs_dispatch.c/h           Outer CreateScreen mode dispatcher
                            (FUN_00582e90) + per-mode handlers + log
                            builder + centre-rect math.  Module-owned
                            globals: cs_primary_pair, cs_zoom_override_*,
                            cs_log_buf, cs_log_dirty, cs_engine_name_header.
  cs_dispatch_win32.c       Win32 primitives — cs_log_get_last_error,
                            cs_fatal_log, cs_fatal_log_with_lasterror,
                            cs_exit.  Backed by GetLastError +
                            FormatMessageA + OutputDebugStringA +
                            ExitProcess.
  Makefile                  single-TU mingw cross-build (-lddraw -ldxguid)

tests/
  Makefile                  host gcc + ASan/UBSan; `make -C tests run`
                            filter by name with `F=<substr>`
  t.h                       T_ASSERT_* macros, 0/1/2 = pass/fail/skip
  test_main.c               X-macro registry; one X(name) per test
  test_pixel_drawer.c       39 tests
  test_asset_register.c     111 tests
  test_bitmap_session.c     31 tests
  test_wnd_proc.c           20 tests
  test_zdd.c                88 tests
  test_cs_dispatch.c        21 tests — setjmp-based cs_exit catcher +
                            recorder hooks for zdd_create_screen +
                            zdd_object_new; covers all 5 modes happy +
                            fail paths + centre-rect math + log
                            builder + prior-pair release prologue.

tools/
  frida_capture.py          headless retail harness driver
  frida/opensummoners-agent.js   Frida agent
  run-retail.sh             single-source-of-truth dev loop
  ghidra-tag-and-export.sh  one-shot wrapper for Ghidra re-tag + re-export
  extract/57ca40_*.py       regenerators + audit for FUN_0057ca40 tables
```

## Open RE threads (not picked up yet)

- **FUN_005b8a20** (181 bytes) — 16bpp pixel-format binding.  The
  one un-ported call inside `zdd_create_screen`.  ECX identity
  ambiguous — likely a global pixel-format descriptor, not the
  calling ZDDObject.  TODO comment lives at zdd.c's `zdd_create_screen`
  post-success hook.
- **Launcher settings record parse** — FUN_005a4770 (45 KB) reads
  `user/config.dat`'s XOR-obfuscated body.  We need this (or a
  hardcoded mode=2 stub) to drive `cs_dispatch_create_screen` from
  the drop-in's WinMain.
- ZDDObject's +0xac field (`com_back`) is dual-role: holds either a
  back-buffer IDirectDrawSurface7 OR an IDirectDrawClipper.  Both
  implement IUnknown so lifecycle doesn't care, but field naming is
  misleading.
- FUN_005b9520's vtable[7] call site passes a pointer to a
  stack-local NULL — could be SetClipList (vtable[7] = byte 0x1c) or
  SetHWnd (vtable[8] = byte 0x20) depending on the actual asm.
  Frida verification recommended.
- ZDDObject's 124-byte embedded DDSURFACEDESC2 (+0x30..+0xab) is
  still modelled as `uint8_t[]` — typed access requires <ddraw.h>.
- ZDDObject's two metric clusters now have field names but the
  *semantic* role of each slot remains uncertain.
- `FUN_005b8b00` (16bpp color-channel shift converter) reads byte
  shift tables off ECX — looks like it expects a "pixel format
  descriptor" object, NOT the calling ZDDObject.  TODO at
  `zdd_object_set_color_key`'s 16bpp branch.
- `FUN_005bd040` mode 3 / mode 4 LUT formulas have arithmetic whose
  "floor-correction" terms are zero for valid weight ranges.
- SS_MGR / W_MGR / GD_MGR boot-pool allocators (DAT_008a8440 / _6ec4
  / _9274) are dependency-of ~30 functions; defer until consumer
  semantics map cleanly.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner.
- `ar_boot_register_all` **and** `pd_boot_init_slots` exist but
  **neither is called from the drop-in's WinMain** — every batch
  is still a module in isolation.  Same applies to
  `cs_dispatch_create_screen` (just landed; not wired into WinMain
  yet — see "Next move" recommendation #1).
- `g_ar_info_table[909]` — pool modelled in full and the 443 writes
  inside FUN_0057ca40 are PORTED via `ar_apply_group3_info_events`.
- `ar_locale_state` modelling: the locale loop's three globals
  (DAT_008a6e68, _6e70, *DAT_008a6e80+0x1c8) are passed in as a
  struct in our port.
- The WndProc port is also a module in isolation.
- `FUN_00563ef0` wave-load second half is unported.
- The 5 "deep engine" structs the WndProc port depends on now have
  field-offset shells in `src/wnd_proc.h`.
- Locale-table magic / sequence fields: 23 distinct values for the
  +0x00 magic field, +0x04 is per-locale group selector 1..73.
- **cs_engine_name_header** is "" at boot; populated by some
  unported settings init path.  When ported, that path's output
  should feed `cs_engine_name_header` via assignment (the buffer
  is `char *` pointer-into-BSS at retail).

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
