# Session handoff — last updated 2026-05-25 (DDraw init WIRED into WinMain)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The drop-in's `WinMain` now drives a real DirectDraw boot end-to-end.
`init_ddraw()` (in `src/main.c`) calls:

```
zdd_create               (DirectDrawCreateEx)
zdd_set_coop_level       (SetCooperativeLevel, DDSCL_NORMAL for mode 2)
cs_dispatch_create_screen(launcher_mode=2, zoom=1280x960)   ← the surface
```

`shutdown_ddraw()` runs `zdd_destroy` at process exit.  Six modules
ported + now wired:

- **Pixel-Drawer** — 8 functions, 39 host tests.
- **Asset-Register** — 31 functions, 111 host tests.
- **Bitmap-Session** — 8 functions, 31 host tests.
- **WndProc** — `FUN_005b12e0`, 20 host tests.
- **ZDD wrapper** — 26 functions covering ctor/dtor + DDERR log
  + DirectDrawCreateEx + SetCooperativeLevel + SetDisplayMode +
  GetDisplayMode + busy_wait + clipper attach + back-buffer attach +
  8bpp palette setup + primary-surface DDSD builder +
  `zdd_create_screen` (full 5-mode dispatch).  88 host tests.
- **cs_dispatch** — `cs_dispatch_create_screen` (FUN_00582e90, 3560B
  outer mode dispatcher).  21 host tests.

Total host tests: **307 pass, 0 fail, 6 skip** (unchanged this
checkpoint; this was a wiring change, not a port).  Cross-build with
mingw clean.

**Live-boot output under harness** (`tools/run-opensummoners.sh`):

```
[opensummoners] init_ddraw: launcher_mode=2 (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)
[opensummoners] init_ddraw: SetCooperativeLevel ok (fullscreen=0)
[opensummoners] init_ddraw: CreateScreen dispatch returned (success path)
[opensummoners] OpenSummoners exiting after 120 frames (1920 ms wall)
[launcher] child exited (rc=0)
```

CLI knobs added to the drop-in:
- `--launcher-mode=N` (0/1/2/3/4; default 2) — picks the
  cs_dispatch_create_screen mode.  Until FUN_005a4770 is ported this
  is the only way to drive different modes.
- `--skip-ddraw` — bypass init_ddraw entirely (window-only boot,
  matches prior behaviour).  Useful for harness A/B.

**Open boot-path gaps** (unchanged):
- `FUN_005b8a20` (16bpp pixel-format binding) is still TODO inside
  `zdd_create_screen`'s post-success hook.  May surface as visible-
  output garbage once we actually draw to the surface.
- The launcher settings record parser (FUN_005a4770, 45 KB) is the
  long-term home of the launcher-mode read; for now the drop-in
  hardcodes via `--launcher-mode=N`.

Most recent commits (newest first):

- (current) main: wire cs_dispatch_create_screen into WinMain
  (mode-2 Windowed by default)
- `cfc1242` cs_dispatch: port FUN_00582e90 outer CreateScreen dispatcher
- `8b9e539` ZDD: port FUN_005b8480 CreateScreen body (5-mode dispatch)
- `d415dd5` ZDD: port FUN_005b9740 back-buffer attach + FUN_005b8e00 8bpp palette
- `8a9d536` ZDD: port 4 CreateScreen leaf helpers + map mode dispatch
- `7f5a001` ZDD: port FUN_005b9520 clipper attach (create + clear + attach)
- `6024a36` ZDD: port FUN_005b8b40 CreateSurfacePair factory + orchestrator returns int
- `1348360` ZDD: port FUN_005b95c0 + 97e0 + 98c0 + 9830 surface-alloc stampers + orchestrator
- `d87b7ea` ZDD: port FUN_005b8c00 DDSURFACEDESC2 builder + CreateSurface
- `19e4e6c` ZDD: port ZDDObject ctor + dtor + LocalFree pixel-buf helper

## Active goal

**Get something rendering on the primary surface.**  CreateScreen
now succeeds; the next milestone is observing pixel output (even
something trivial like a BltColorFill).  After that, port the
title-menu scene runner so the game's own draws fire.

## Next move (pick one — recommendation first)

1. **(recommended) Smoke-test: `BltColorFill` the primary surface from
   the main loop.**  Add a small helper that grabs `g_zdd->com_a`
   (mode 2 leaves this NULL but the primary_obj's com_primary holds
   the offscreen surface — verify in code) and `IDirectDrawSurface7::Blt`
   with `DDBLT_COLORFILL` to fill it with red, then `Flip` or `Blt` to
   the visible primary.  Two purposes: (a) confirms the surface chain
   is actually usable, (b) surfaces the 16bpp pixel-format gap if
   `FUN_005b8a20` was needed.  ~50 lines of code, fits in one
   commit.

2. **Port `FUN_005b8a20`** (16bpp pixel-format binding, 181 bytes).
   The one remaining un-ported call inside `zdd_create_screen`.  ECX
   identity ambiguous — investigate via r2 disasm + Frida hook to log
   ECX at first live call.  Defer until smoke (#1) tells us 16bpp
   actually fails without it.

3. **Port the title-menu scene runner `FUN_0056aea0`.**  The "Title
   menu — runs its own pump+tick" function called from the scene
   driver.  Currently unported; first reader of the
   `cs_primary_pair`/`primary_obj` surfaces in a real draw context.
   Likely multi-checkpoint port with many unported callees.

4. **`FUN_005a4770` launcher-settings parser** (45 KB).  Replaces the
   `--launcher-mode=N` CLI flag with a real config.dat read.  Lower
   priority than getting pixels to the screen.

## Active modules / file layout

```
src/
  main.c                    WinMain shim + DDraw init (zdd_create →
                            zdd_set_coop_level → cs_dispatch_create_screen),
                            shutdown_ddraw, --launcher-mode/--skip-ddraw flags
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 8 functions
  asset_register.c/h        Asset-register slots — 31 functions
  asset_register_win32.c    GDI primitive wrappers
  bitmap_session.c/h        PE-resource bitmap decoder — 8 functions
  bitmap_session_win32.c    LocalAlloc/Free + FindResource/LoadResource/LockResource
  wnd_proc.c/h              Main game window WndProc — pure dispatch
  wnd_proc_win32.c          DefWindowProcA + ExitProcess + 5 placeholder hooks
  zdd.c/h                   ZDD wrapper — ctor/dtor + DDERR log +
                            create driver + ZDDObject lifecycle + DDSD
                            builder + surface-alloc stampers + orchestrator
                            + factory + clipper attach + back-buffer attach
                            + 8bpp palette setup + primary-surface DDSD
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
                            cs_exit.
  Makefile                  single-TU mingw cross-build (-lddraw -ldxguid)

tests/
  Makefile                  host gcc + ASan/UBSan; `make -C tests run`
                            filter by name with `F=<substr>`
  test_*.c                  39+111+31+20+88+21 = 310 tests across 6 modules

tools/
  frida_capture.py          headless retail harness driver
  frida/opensummoners-agent.js   Frida agent
  run-retail.sh             single-source-of-truth retail dev loop
  run-opensummoners.sh      drop-in dev loop (build + launcher harness)
  ghidra-tag-and-export.sh  one-shot wrapper for Ghidra re-tag + re-export
  extract/57ca40_*.py       regenerators + audit for FUN_0057ca40 tables
```

## Open RE threads (not picked up yet)

- **FUN_005b8a20** (181 bytes) — 16bpp pixel-format binding.  The
  one un-ported call inside `zdd_create_screen`.  ECX identity
  ambiguous — likely a global pixel-format descriptor, not the
  calling ZDDObject.  TODO comment lives at zdd.c's `zdd_create_screen`
  post-success hook.  Live-boot path (mode 2, bpp 16) currently
  succeeds without it; visible output may differ.
- **Launcher settings record parse** — FUN_005a4770 (45 KB) reads
  `user/config.dat`'s XOR-obfuscated body.  Replaced for now by the
  `--launcher-mode=N` CLI flag on the drop-in.
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
  is still a module in isolation.  (cs_dispatch_create_screen *is*
  wired now — see "Where we are".)
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
