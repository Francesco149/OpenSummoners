# Session handoff — last updated 2026-05-25 (WM_PAINT handler ported + wired)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The drop-in now runs the **real per-frame present dispatcher**
(`FUN_005b8fc0` → `zdd_present`) every frame, AND the **real WM_PAINT
handler** (`FUN_005b9130` → `zdd_window_paint`) on demand whenever the
OS invalidates the client area.  The hand-rolled "smoke-present" loop
(BltColorFill + GetDC + BitBlt(window)) is gone, replaced by an exact
port of retail's 5-mode dispatcher.  Mode 2 (Windowed) is the live
boot mode; the harness sees zero DDERR per frame across 10-frame runs.

The trio that landed this checkpoint:

1. **`zdd_object_get_dc`** (FUN_005b94e0, 32 bytes).  Wrapper over
   `IDirectDrawSurface7::GetDC` (vtable[17] / byte 0x44) on a
   ZDDObject's `com_primary`.  Forces eax=1 regardless of underlying
   HRESULT — a retail quirk we mirror.
2. **`zdd_object_release_dc`** (FUN_005b9500, 15 bytes).  Wrapper over
   `IDirectDrawSurface7::ReleaseDC` (vtable[26] / byte 0x68).  No NULL
   guard in retail; our underlying primitive guards defensively.
3. **`zdd_present`** (FUN_005b8fc0, 335 bytes).  5-mode dispatcher,
   switches on `self->pixel_format_mode`:
   - mode 0 (Full): `Flip(com_a, primary_obj->com_primary, DDFLIP_WAIT)`
   - mode 1 (Safe): `com_a->Blt(rect, primary_obj->com_primary, rect, DDBLT_WAIT)`
     where `rect = &self->screen_pos_x` (4-int RECT)
   - mode 2 (Wind): `zdd_object_get_dc(primary_obj, &hdc)` +
     `GetDC(NULL)` + `BitBlt(desktop, screen_pos_x, screen_pos_y,
     screen_width, screen_height, hdc, 0, 0, SRCCOPY)` + `ReleaseDC(NULL)`
     + `zdd_object_release_dc(primary_obj, hdc)`
   - mode 3 (DB): inline Blt from primary_obj→back_obj_a at rect, then
     common Flip(com_a, back_obj_a->com_primary)
   - mode 4 (Zoom): `FUN_005b8ea0`(SW scaler, UNPORTED) +
     `zdd_object_blt_onto(back_obj_b, back_obj_a, rect[3], rect[4])`
     + common Flip
4. **`zdd_object_blt_onto`** (FUN_005b9a40, 112 bytes).  Generic "blit
   self onto dest at (x,y)" used by mode 4.  Note the role inversion:
   `self` is the SOURCE; `dest` receives the Blt vtable call.  Returns
   1 as degenerate-success when `self->com_primary` is NULL (retail's
   literal contract — verified at 0x5b9a4a).
5. New Win32 primitives: `zdd_surface_flip` (vtable[11]/0x2c),
   `zdd_surface_blt` (vtable[5]/0x14), `zdd_desktop_present`
   (GetDC(NULL)+BitBlt+ReleaseDC).

`main.c` changes:
- `present_smoke_frame()` removed; per-frame loop now calls
  `zdd_present(g_zdd)` directly.
- New `sync_window_position()` helper does `ClientToScreen(g_hwnd,
  &pt)` and stamps the result on `g_zdd->screen_pos_x/y`.  Called once
  after `init_ddraw` and again on `WM_MOVE` so mode 2's desktop BitBlt
  tracks the window's actual screen position.
- `--no-smoke-present` flag renamed `--no-present`.
- Smoke-failure counters / log lines removed.
- `WM_PAINT` case added to the minimal `wndproc`: calls
  `zdd_window_paint(g_zdd, hwnd)`; if it returns 1 (consumed) the
  message is eaten, otherwise it falls through to `DefWindowProcA`.
  Wired here directly (not via `wp_handle_message`) because the
  ported WndProc module is still in isolation.

**Naming-discovery clarification**: Ghidra labels FUN_005b94e0 /
_9500 as `paint_ctx::` methods, but the ECX in every live callsite is
a `zdd_object*` (specifically `zdd.primary_obj`) — verified by r2 of
the WM_PAINT handler (FUN_005b9130) and FUN_005b8fc0 case 2.  The
`paint_ctx` typedef in `wnd_proc.h` is actually a `zdd` alias (same
+0x16c primary_obj / +0x138..+0x144 / +0x164 offsets).  Its
`+0x2c zdd_device` field docstring is incorrect — that offset falls
inside `zdd.log_buf` and isn't read by anything.

Seven modules ported + wired:

- **Pixel-Drawer** — 8 functions, 39 host tests.
- **Asset-Register** — 31 functions, 111 host tests.
- **Bitmap-Session** — 8 functions, 31 host tests.
- **WndProc** — `FUN_005b12e0`, 20 host tests.
- **ZDD wrapper** — 31 functions covering ctor/dtor + DDERR log +
  DirectDrawCreateEx + SetCooperativeLevel + SetDisplayMode +
  GetDisplayMode + busy_wait + clipper attach + back-buffer attach +
  8bpp palette setup + primary-surface DDSD builder + `zdd_create_screen`
  (5-mode dispatch) + smoke-present primitives + `zdd_object_get_dc`
  / `zdd_object_release_dc` / `zdd_object_blt_onto` / `zdd_present`
  (5-mode present dispatcher) + **`zdd_window_paint` (FUN_005b9130
  WM_PAINT handler) + Win32 primitives `zdd_window_paint_begin` /
  `zdd_window_paint_end` / `zdd_window_blit_copy`**.  110 host tests.
- **cs_dispatch** — `cs_dispatch_create_screen`, 21 host tests.

Total host tests: **329 pass, 0 fail, 6 skip** (was 324; +5 for the
WM_PAINT handler).  Cross-build with mingw clean.

**Live-boot output under harness** (`tools/run-opensummoners.sh
--frames 10 --hide-window`):

```
[opensummoners] init_ddraw: launcher_mode=2 (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)
[opensummoners] init_ddraw: SetCooperativeLevel ok (fullscreen=0)
[opensummoners] init_ddraw: CreateScreen dispatch returned (success path)
[opensummoners] OpenSummoners exiting after 10 frames (161 ms wall)
[launcher] child exited (rc=0)
```

(Zero DDERR lines.  The dispatcher fires each frame; mode 2's desktop
BitBlt happens silently.  Without renderer content the surface stays
uninitialised — no visible artefact unless the window is visible.)

CLI knobs available on the drop-in:
- `--launcher-mode=N` (0/1/2/3/4; default 2)
- `--skip-ddraw` — bypass init entirely (window-only boot)
- `--no-present` — skip the per-frame `zdd_present` call (DDraw init
  still runs; useful for isolating DDraw init issues from per-frame
  ones).  Replaces the prior `--no-smoke-present` flag.

**Open boot-path gaps** (unchanged from prior checkpoint):
- `FUN_005b8a20` (16bpp pixel-format binding) still TODO in
  `zdd_create_screen`'s post-success hook.  Smoke-present succeeded
  without it; the new dispatcher also doesn't depend on it (the BitBlt
  uses GDI which has its own pixel-format handling).
- `FUN_005a4770` (45 KB launcher settings record parser) replaced
  for now by `--launcher-mode=N` CLI flag.
- `FUN_005b8ea0` (16bpp software upscaler) — mode 4 (Zoom) only.
  Dispatcher currently skips the upscale stage for mode 4 and runs
  straight to `blit_onto` + Flip, which means `back_obj_b` stays
  unchanged from boot (blank stamp).  Mode 4 isn't the live boot
  mode; this TODO bites only on a Zoom-launcher selection.

Most recent commits (newest first):

- (uncommitted) ZDD: port FUN_005b9130 WM_PAINT handler
  (zdd_window_paint) + 3 Win32 primitives + wire into main wndproc
- `26d2f8a` ZDD: port FUN_005b8fc0 5-mode present dispatcher + trio leaves
- `ca1a94e` main + launcher: SetConsoleOutputCP(CP_UTF8)
- `202bd3f` docs: update HANDOFF + PROGRESS for smoke-present + RE fixes
- `5d82301` ZDD: smoke-present mode 2 (Blt+GetDC+BitBlt) + 2 RE bug fixes
- `39efc7f` main: wire cs_dispatch_create_screen into WinMain (mode-2 Windowed)
- `cfc1242` cs_dispatch: port FUN_00582e90 outer CreateScreen dispatcher
- `8b9e539` ZDD: port FUN_005b8480 CreateScreen body (5-mode dispatch)

## Active goal

**Get the title menu rendering.**  The full per-frame pipeline is
ported.  Next is producing real content for `primary_obj->com_primary`
so the windowed BitBlt has something visible to push — i.e., port the
title-menu scene runner (`FUN_0056aea0`).

## Next move (pick one — recommendation first)

1. **(recommended) Port the title-menu scene runner `FUN_0056aea0`.**
   The first *real* drawer that touches `primary_obj`.  Likely
   multi-checkpoint because of its many unported callees
   (`PTR_DAT_0056bfa4` jumptable, `FUN_00563ef0` wave-load).  Once it
   draws *anything* to `com_primary`, mode 2's `zdd_present` will
   composite it into the window — that's the first visible frame.

2. **Port `FUN_005b8ea0`** (16bpp software upscaler, mode 4 only).
   Defer until mode 4 is exercised; mode 2 is the live boot mode.

3. **Port `FUN_005b8a20`** (16bpp pixel-format binding).  ECX
   identity still ambiguous — investigate via r2 disasm + a Frida hook
   that logs ECX at first live call.  Smoke-present and the new
   dispatcher both work without it; defer until colour-channel
   artefacts appear in real content.

4. **`FUN_005a4770` launcher-settings parser** (45 KB).  Replaces the
   `--launcher-mode=N` CLI flag with a real `user/config.dat` read.
   Lower priority than getting pixels.

## Active modules / file layout

```
src/
  main.c                    WinMain shim + DDraw init + per-frame
                            zdd_present + WM_PAINT-driven zdd_window_paint.
                            sync_window_position() keeps screen_pos_x/y
                            in sync (init + WM_MOVE).  CLI: --launcher-mode
                            / --skip-ddraw / --no-present.
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 8 functions
  asset_register.c/h        Asset-register slots — 31 functions
  asset_register_win32.c    GDI primitive wrappers
  bitmap_session.c/h        PE-resource bitmap decoder — 8 functions
  bitmap_session_win32.c    LocalAlloc/Free + FindResource/LoadResource
  wnd_proc.c/h              Main game window WndProc — pure dispatch
  wnd_proc_win32.c          DefWindowProcA + ExitProcess + placeholders
  zdd.c/h                   ZDD wrapper — full ddraw init chain + 5-mode
                            CreateScreen + clipper attach with RGNDATA +
                            5-mode present dispatcher (zdd_present) +
                            paint_ctx GetDC/ReleaseDC + blt_onto +
                            zdd_surface_flip / _blt / desktop_present
                            primitives + **WM_PAINT handler
                            (zdd_window_paint) + BeginPaint/EndPaint/
                            BitBlt primitives**.
  zdd_win32.c               DirectDraw7 + IDirectDrawSurface7 +
                            IDirectDrawClipper + IDirectDrawPalette
                            primitive wrappers.  Dual-sinks
                            OutputDebugStringA to stderr.  Includes:
                            Flip + Blt + desktop-BitBlt + **window-paint
                            begin/end + caller-supplied-HDC BitBlt**.
  cs_dispatch.c/h           Outer CreateScreen mode dispatcher
                            (FUN_00582e90) + per-mode handlers.
  cs_dispatch_win32.c       Win32 primitives — error log fetch,
                            fatal log, fatal+lasterror, ExitProcess.
  Makefile                  single-TU mingw cross-build (-lddraw -ldxguid)

tests/
  Makefile                  host gcc + ASan/UBSan
  test_*.c                  329 tests across 6 modules

tools/
  frida_capture.py          headless retail harness driver
  frida/opensummoners-agent.js   Frida agent
  run-retail.sh             single-source-of-truth retail dev loop
  run-opensummoners.sh      drop-in dev loop (build + launcher harness)
  ghidra-tag-and-export.sh  one-shot wrapper for Ghidra re-tag + re-export
  launcher/                 opensummoners-launcher.exe (job-object
                            supervised child runner)
```

## Open RE threads (not picked up yet)

- **`FUN_005b8a20`** (181 bytes) — 16bpp pixel-format binding.  Last
  un-ported call inside `zdd_create_screen`.  ECX identity ambiguous.
  Dispatcher works without it.
- **`FUN_005b8b00`** (16bpp colorkey channel converter) — TODO at
  `zdd_object_set_color_key`'s 16bpp branch.  Dead at boot (sentinel
  colorkey wins) but live for real scene content.
- **`FUN_005b8ea0`** (16bpp software upscaler, mode 4 only) — 285
  bytes, calls Lock (FUN_005b9490) and Unlock (FUN_005b94d0) on both
  surfaces and copies pixels with integer division.  Mode-4-specific;
  dispatcher skips the upscale and proceeds to blit_onto + Flip.
- **Launcher settings record parse** — `FUN_005a4770` (45 KB).
- ZDDObject's +0xac field (`com_back`) is dual-role: holds either a
  back-buffer `IDirectDrawSurface7` OR an `IDirectDrawClipper`.
- ZDDObject's 124-byte embedded DDSURFACEDESC2 (+0x30..+0xab) is
  still modelled as `uint8_t[]`.
- `FUN_005bd040` mode-3/mode-4 LUT formulas have arithmetic whose
  "floor-correction" terms are zero for valid weight ranges.
- SS_MGR / W_MGR / GD_MGR boot-pool allocators.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner.
- `ar_boot_register_all` and `pd_boot_init_slots` exist but are not
  called from WinMain — every batch is still a module in isolation.
- `g_ar_info_table[909]` — pool modelled in full and PORTED via
  `ar_apply_group3_info_events`.
- WndProc port is also a module in isolation.
- `FUN_00563ef0` wave-load second half is unported.
- Locale-table magic / sequence fields: 23 distinct values for the
  +0x00 magic field, +0x04 is per-locale group selector 1..73.
- `cs_engine_name_header` is "" at boot; populated by some unported
  settings init path.

## Resolved this checkpoint

- **`FUN_005b9130` WM_PAINT handler ported** as `zdd_window_paint`.
  Sequence faithful to retail: `BeginPaint(hwnd) → zdd_object_get_dc
  (primary_obj) → BitBlt(window_hdc, screen_pos_x/y/w/h, src_hdc) →
  zdd_object_release_dc → EndPaint`.  Three new Win32 primitives:
  `zdd_window_paint_begin` (BeginPaint + heap-alloc PAINTSTRUCT for
  cross-call lifetime), `zdd_window_paint_end` (EndPaint + free), and
  `zdd_window_blit_copy` (BitBlt with caller-supplied HDCs, distinct
  from `zdd_desktop_present` which wraps `GetDC(NULL)` internally).
  Wired into main.c's minimal `wndproc` WM_PAINT case (not via
  `wp_handle_message` since the WndProc module is still in isolation).
- **Retail-faithful quirk noted**: WM_PAINT BitBlts into a CLIENT-
  coordinate HDC (from BeginPaint) but uses `screen_pos_x/y` (SCREEN
  coordinates).  For windows not at desktop (0,0), the BitBlt lands
  outside the visible region.  Per-frame `zdd_present` mode 2 covers
  the same damage on the next tick via GetDC(NULL) at the right
  coords, so the misalignment is overwritten before any frame
  fully composites.  See `zdd.h` `zdd_window_paint` doc for the full
  reasoning.

(Prior checkpoint discoveries that informed this one, retained:)
- `paint_ctx::FUN_005b94e0` / `_9500` ECX is `zdd.primary_obj`, not a
  separate class — same applies inside FUN_005b9130 at 0x5b9158.
- `FUN_005b9a40` arg order: `(this=src, dest_obj, dest_x, dest_y)`.
- Mode 2 desktop-DC: `GetDC(NULL)` (hWnd local stamped to 0 at
  prologue), `BitBlt` at `(screen_pos_x, screen_pos_y)`.
- `zdd_surface_flip` vtable byte 0x2c = method 11 = Flip.
- `zdd_surface_blt` byte 0x14 = method 5 = Blt.
- `zdd_surface_get_dc` byte 0x44 = method 17 = GetDC.
- `zdd_surface_release_dc` byte 0x68 = method 26 = ReleaseDC.

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
