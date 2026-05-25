# Session handoff — last updated 2026-05-25 (main pump port)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Small clean chip — ported the 156-byte main message pump
(`FUN_005b1030` → `app_pump_frame`).  This was the small prerequisite
chip the HANDOFF flagged before the big title-menu scene runner port.
With this in place, the runner's two `FUN_005b1030()` callsites have
a real implementation to bind to.

Same checkpoint: did the natural refactor that "ownership of the
app_ctx struct moves to the pump module".  `wp_app_ctx` →
`app_ctx`, `g_wp_app_ctx` → `g_app_ctx`, `g_wp_active_flag` →
`g_app_active_flag`.  The struct + globals moved from `wnd_proc.h`
into a new `src/app_pump.h` (which wnd_proc.h now includes).

Disambiguated the throttle comparison: Ghidra showed `prev - now < 5`
without a sign indicator; the actual asm at `0x5b10b3` is `jae`
(UNSIGNED).  So the throttle re-arms when `(uint32_t)(prev - now) < 5`,
which is true only when `now` is in `[prev - 4, prev]` — basically
"the millisecond clock hasn't ticked yet".  In practice it holds the
throttle until `WM_TIMER` (0x113) clears it; the WndProc's WM_TIMER
handler is what does the clear.

Seven modules ported + wired:

- **Pixel-Drawer** — 8 functions, 39 host tests.
- **Asset-Register** — 31 functions, 111 host tests.
- **Bitmap-Session** — 8 functions, 31 host tests.
- **WndProc** — `FUN_005b12e0`, 19 host tests (was 20; the layout
  test moved to the pump module since the struct moved).
- **ZDD wrapper** — 40+ functions, 153 host tests.
- **cs_dispatch** — `cs_dispatch_create_screen`, 21 host tests.
- **app_pump** — `FUN_005b1030` (`app_pump_frame`), 16 host tests.

Total host tests: **387 pass, 0 fail, 6 skip** (was 372/0/6).
Cross-build with mingw clean every checkpoint.

**Live-boot output under harness** (`tools/run-opensummoners.sh
--frames 10 --hide-window`):

```
[opensummoners] init_ddraw: launcher_mode=2 (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)
[opensummoners] init_ddraw: SetCooperativeLevel ok (fullscreen=0)
[opensummoners] init_ddraw: CreateScreen dispatch returned (success path)
[opensummoners] OpenSummoners exiting after 10 frames (161 ms wall)
[launcher] child exited (rc=0)
```

(Zero DDERR — unchanged.  `app_pump_frame` is NOT wired into the
drop-in's per-frame loop yet; main.c keeps its own minimal
`main_loop_body` until the scene runner ports and naturally calls
the pump retail-style.)

CLI knobs available on the drop-in (unchanged):
- `--launcher-mode=N` (0/1/2/3/4; default 2)
- `--skip-ddraw` — bypass init entirely
- `--no-present` — skip the per-frame `zdd_present` call

## Active goal

**Get the title menu rendering.**  All prerequisite "small leaves"
are now ported.  The remaining work is the scene runner itself
(`FUN_0056aea0`, 3441 bytes) — a multi-checkpoint port whose body
calls into:

- `app_pump_frame()` (✓ ported THIS checkpoint).
- `zdd_object_clear` / `zdd_object_blt_keyed` / `zdd_present`
  (all ✓ ported previously).
- `PTR_DAT_0056bfa4` jumptable — 7 handlers inlined inside the runner.
- `FUN_00563ef0` wave-load — partially decompiled, second half unported.
- `FUN_0056c180` title-menu flip helper — unported.
- `FUN_0056c070` sparkle helper — unported.
- `FUN_00412c10` menu controller allocator — unported.
- `FUN_0043c110` input poll / `FUN_0043ce50` input action latch
  — unported.

See `docs/findings/title-scene.md` for the phase breakdown.

## Next move (pick one — recommendation first)

1. **(recommended) Begin the title-menu scene runner port
   (`FUN_0056aea0`).**  Start with the outer skeleton + state-vars
   (`local_28`, `local_64`, `local_68`, `local_30`) and the
   pump-callsite plumbing.  Stub the unported helpers as TODO
   panics so the structure is in place before each helper gets
   filled in.  Multi-checkpoint by design.

2. **Port `FUN_005b14c0`** (287-byte post-activate hook).  Wires
   up the lost-surface dispatchers (✓ already ported) + the
   sprite-cache scrub on resume.  Requires DAT_008a760c (sprite
   asset pool) + DAT_008a92b4 (some object pool) to be modelled —
   substantial RE work that isn't on the critical path to "title
   menu renders".

3. **`FUN_005a4770` launcher-settings parser** (45 KB).  Replaces
   the `--launcher-mode=N` CLI flag with a real `user/config.dat`
   read.  Lower priority than getting pixels.

4. **Wire `app_pump_frame` into `main.c`.**  Small chip — replace
   the drop-in's hand-rolled `main_loop_body` + `frame_limiter` with
   a single call to the ported pump (gated behind a CLI flag so
   `--frames N` still works for the harness).  Mostly cosmetic until
   the scene runner exists; recommended to defer until then so the
   wire-up has a real consumer to validate against.

## Active modules / file layout

```
src/
  main.c                    WinMain shim + DDraw init + per-frame
                            zdd_present + WM_PAINT-driven zdd_window_paint.
                            sync_window_position() keeps screen_pos_x/y
                            in sync (init + WM_MOVE).  Still uses its
                            own minimal main_loop_body; not yet driven
                            by app_pump_frame.
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 8 functions
  asset_register.c/h        Asset-register slots — 31 functions
  asset_register_win32.c    GDI primitive wrappers
  bitmap_session.c/h        PE-resource bitmap decoder — 8 functions
  bitmap_session_win32.c    LocalAlloc/Free + FindResource/LoadResource
  wnd_proc.c/h              Main game window WndProc — pure dispatch.
                            Includes app_pump.h for app_ctx /
                            g_app_ctx / g_app_active_flag.
  wnd_proc_win32.c          DefWindowProcA + ExitProcess + placeholders
  zdd.c/h                   ZDD wrapper — full ddraw init chain +
                            5-mode CreateScreen + clipper attach with
                            RGNDATA + 5-mode present dispatcher +
                            paint_ctx GetDC/ReleaseDC + blt_onto +
                            blt_keyed + WM_PAINT handler + surface
                            Lock/Unlock/clear + color descriptor +
                            bind_pixel_format + color_convert + 16bpp
                            upscaler + lost-surface recovery.
  zdd_win32.c               DirectDraw7 + IDirectDrawSurface7 +
                            IDirectDrawClipper + IDirectDrawPalette
                            primitive wrappers.
  cs_dispatch.c/h           Outer CreateScreen mode dispatcher
                            (FUN_00582e90) + per-mode handlers.
  cs_dispatch_win32.c       Win32 primitives — error log fetch,
                            fatal log, fatal+lasterror, ExitProcess.
  app_pump.c/h              **NEW.**  Pure-logic port of FUN_005b1030
                            (app_pump_frame).  Owns app_ctx +
                            g_app_ctx + g_app_active_flag.  5 hooks
                            (peek/translate-dispatch/wait/get-tick/
                            request-exit).
  app_pump_win32.c          **NEW.**  Real Win32 backend —
                            PeekMessageA + TranslateMessage +
                            DispatchMessageA + WaitMessage +
                            GetTickCount + exit.
  Makefile                  single-TU mingw cross-build (-lddraw -ldxguid)

tests/
  Makefile                  host gcc + ASan/UBSan
  test_*.c                  387 tests across 7 modules
```

## Open RE threads (not picked up yet)

- **`FUN_0056aea0`** (3441 bytes) — title-menu scene runner.  See
  `docs/findings/title-scene.md` for phase breakdown + jumptable
  resolution.  Big port — multi-checkpoint.  All prerequisite leaves
  ported.
- **`FUN_005b14c0`** (287 bytes) — post-activate hook (sprite-cache
  scrub).
- **`FUN_005bd040`** mode-3/mode-4 LUT formulas — arithmetic with
  "floor-correction" terms zero for valid weight ranges.
- **`FUN_005ba120`** / `FUN_005ba290` — DInput pad lazy-attach (called
  by title-menu first-confirm).
- **`FUN_00412c10`** menu controller allocator.
- **`FUN_0043c110`** input poll.
- **`FUN_0043ce50`** input action latch.
- **Launcher settings record parse** — `FUN_005a4770` (45 KB).
- ZDDObject's +0xac field (`com_back`) is dual-role: holds either a
  back-buffer `IDirectDrawSurface7` OR an `IDirectDrawClipper`.
- ZDDObject's 124-byte embedded DDSURFACEDESC2 (+0x30..+0xab) is
  still modelled as `uint8_t[]`.
- SS_MGR / W_MGR / GD_MGR boot-pool allocators (gameplay-side, not
  blocking title menu).
- `PTR_DAT_0056bfa4` jumptable — 7 handlers INSIDE FUN_0056aea0.
- `ar_boot_register_all` and `pd_boot_init_slots` exist but are not
  called from WinMain — every batch is still a module in isolation.
- WndProc port is also a module in isolation (drop-in uses its own
  minimal wndproc in main.c).
- `FUN_00563ef0` wave-load second half is unported.
- Locale-table magic / sequence fields.
- `cs_engine_name_header` is "" at boot.

## Resolved this checkpoint

- **`FUN_005b1030` `app_pump_frame`** — pure-C port + 5-hook Win32
  backend.  Disambiguated throttle compare to UNSIGNED via asm at
  0x5b10b3 (`jae`).
- **`app_ctx` struct named fields** — `+0x0c limiter_enable`,
  `+0x10 last_tick_ms`, `+0x1c pump_throttle` (was anonymous
  `_pad0c/_pad10/timer` in wp_app_ctx).
- **`wp_app_ctx` / `g_wp_app_ctx` / `g_wp_active_flag` renamed** to
  drop the `wp_` prefix — the struct + globals are shared between
  WndProc and pump now, so the prefix is misleading.  Struct moved
  to `src/app_pump.h`; `wnd_proc.h` includes it.

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
