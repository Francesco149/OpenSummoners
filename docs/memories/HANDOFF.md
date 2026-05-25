# Session handoff — last updated 2026-05-25 (lost-surface recovery)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Big chip session — landed **four ZDD checkpoints** that fill out most
of the remaining "small leaf" RE work in the present / surface-paint
family.  The drop-in still boots zero-DDERR through 10 frames mode-2
(Windowed) live, but now has the plumbing for:

1. **Per-surface Lock + zero-fill** (zdd_object_lock / _unlock /
   _clear).  Title-scene phase-0 needs this to blank back_obj_a
   before the studio fade-in.
2. **Pixel-format color descriptor + 16bpp conversion** (`zdd_color_descriptor`
   struct at zdd+0x00, `zdd_bind_pixel_format` stamps it post-CreateScreen,
   `zdd_color_convert` packs RGB888 → surface-native).  Wired into
   `zdd_object_set_color_key`'s 16bpp branch.
3. **Color-keyed blit** (`zdd_object_blt_keyed`) — variant of
   `blt_onto` with positioned dest origin (metric_0c/_10) + KEYSRC.
   Used by title-scene studio-logo phases.
4. **Mode-4 software upscaler** (`zdd_object_upscale_16bpp`) plus mode-4
   dispatcher wiring.  Mode 2 (Windowed) is still the live boot mode,
   but Mode 4 (Zoom) no longer has a TODO gap.
5. **Lost-surface recovery** — IsLost + Restore primitives + the
   four-slot dispatchers (`zdd_check_any_surface_lost`,
   `zdd_restore_all_surfaces`) that the post-activate hook
   (FUN_005b14c0, unported) needs.

Plumbing-side: the embedded DDSD's lpSurface slot now reads through a
new `zdd_object_get_locked_info` Win32 primitive so the
4-byte-vs-8-byte pointer mismatch between retail (32-bit) and host
(64-bit) doesn't bleed into pure-logic code.  Test stubs publish
canned values from `g_dd_lock_fill_*` globals.

Six modules ported + wired:

- **Pixel-Drawer** — 8 functions, 39 host tests.
- **Asset-Register** — 31 functions, 111 host tests.
- **Bitmap-Session** — 8 functions, 31 host tests.
- **WndProc** — `FUN_005b12e0`, 20 host tests.
- **ZDD wrapper** — 40+ functions: full DDraw init chain + 5-mode
  CreateScreen + clipper attach with RGNDATA + 5-mode present
  dispatcher (`zdd_present`) + paint_ctx GetDC/ReleaseDC + blt_onto +
  WM_PAINT handler (`zdd_window_paint`) + surface Lock/Unlock/clear
  + color descriptor + bind_pixel_format + color_convert + blt_keyed
  + mode-4 upscaler + lost-surface recovery (IsLost / Restore /
  check_any / restore_all).  **153 host tests** (was 110 last
  checkpoint).
- **cs_dispatch** — `cs_dispatch_create_screen`, 21 host tests.

Total host tests: **372 pass, 0 fail, 6 skip** (was 329/0/6).
Cross-build with mingw clean every checkpoint.

**Live-boot output under harness** (`tools/run-opensummoners.sh
--frames 10 --hide-window`):

```
[opensummoners] init_ddraw: launcher_mode=2 (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)
[opensummoners] init_ddraw: SetCooperativeLevel ok (fullscreen=0)
[opensummoners] init_ddraw: CreateScreen dispatch returned (success path)
[opensummoners] OpenSummoners exiting after 10 frames (160 ms wall)
[launcher] child exited (rc=0)
```

(Still zero DDERR.  The post-CreateScreen `zdd_bind_pixel_format`
call fires silently — no logging on success.)

CLI knobs available on the drop-in (unchanged):
- `--launcher-mode=N` (0/1/2/3/4; default 2)
- `--skip-ddraw` — bypass init entirely
- `--no-present` — skip the per-frame `zdd_present` call

## Active goal

**Get the title menu rendering.**  The full per-frame pipeline +
all surface-paint leaves are ported.  Next is producing real content
for `primary_obj->com_primary` so the windowed BitBlt has something
visible to push — i.e., port the title-menu scene runner
(`FUN_0056aea0`).

The leaves it needs are now ALL ported:
- `zdd_object_clear` (FUN_005b9410) for phase-0 blank.
- `zdd_object_blt_keyed` (FUN_005b9b70) for studio-logo phases 2-4.
- The frame-end Flip path goes through `zdd_present` (already wired).

## Next move (pick one — recommendation first)

1. **(recommended) Port the title-menu scene runner `FUN_0056aea0`.**
   The first *real* drawer that touches `primary_obj`.  Multi-
   checkpoint because of its 3441-byte body + many unported callees
   (`PTR_DAT_0056bfa4` jumptable [inlined inside the runner, not
   separately exported], `FUN_00563ef0` wave-load, `FUN_0056c180`
   title-menu flip helper, `FUN_0056c070` sparkle helper, `FUN_005b1030`
   message-pump frame limiter).  All ZDD leaves it touches are now
   ported.  The remaining gaps are: the asset-pool / menu controller
   side (`FUN_00412c10` menu controller alloc, `FUN_0043c110` input
   poll, `FUN_0043ce50` input action latch).  See
   `docs/findings/title-scene.md`.

2. **Port `FUN_005b1030`** (156-byte main pump / frame waiter).  Small
   chip, requires modelling the app_ctx struct (DAT_008a9b64).  Used
   pervasively by the scene runner.

3. **Port `FUN_005b14c0`** (287-byte post-activate hook).  Wires up
   the new lost-surface dispatchers + the sprite-cache scrub on
   resume.  Requires DAT_008a760c (sprite asset pool) + DAT_008a92b4
   (some object pool) to be modelled — substantial RE work.

4. **`FUN_005a4770` launcher-settings parser** (45 KB).  Replaces the
   `--launcher-mode=N` CLI flag with a real `user/config.dat` read.
   Lower priority than getting pixels.

## Active modules / file layout

```
src/
  main.c                    WinMain shim + DDraw init + per-frame
                            zdd_present + WM_PAINT-driven zdd_window_paint.
                            sync_window_position() keeps screen_pos_x/y
                            in sync (init + WM_MOVE).
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 8 functions
  asset_register.c/h        Asset-register slots — 31 functions
  asset_register_win32.c    GDI primitive wrappers
  bitmap_session.c/h        PE-resource bitmap decoder — 8 functions
  bitmap_session_win32.c    LocalAlloc/Free + FindResource/LoadResource
  wnd_proc.c/h              Main game window WndProc — pure dispatch
  wnd_proc_win32.c          DefWindowProcA + ExitProcess + placeholders
  zdd.c/h                   ZDD wrapper — full ddraw init chain +
                            5-mode CreateScreen + clipper attach with
                            RGNDATA + 5-mode present dispatcher
                            (zdd_present) + paint_ctx GetDC/ReleaseDC
                            + blt_onto + blt_keyed + WM_PAINT handler +
                            **surface Lock/Unlock/clear + color
                            descriptor (zdd_color_descriptor at
                            zdd+0x00, 22 bytes) + bind_pixel_format +
                            color_convert + 16bpp upscaler + lost-
                            surface recovery (is_lost / restore /
                            check_any / restore_all)**.
  zdd_win32.c               DirectDraw7 + IDirectDrawSurface7 +
                            IDirectDrawClipper + IDirectDrawPalette
                            primitive wrappers.  Dual-sinks
                            OutputDebugStringA to stderr.  Includes:
                            Flip + Blt + desktop-BitBlt + window-paint
                            begin/end + caller-supplied-HDC BitBlt +
                            **Lock + Unlock + GetSurfaceDesc +
                            IsLost + Restore + get_locked_info
                            (post-Lock DDSD extractor — handles
                            host/target pointer-size mismatch)**.
  cs_dispatch.c/h           Outer CreateScreen mode dispatcher
                            (FUN_00582e90) + per-mode handlers.
  cs_dispatch_win32.c       Win32 primitives — error log fetch,
                            fatal log, fatal+lasterror, ExitProcess.
  Makefile                  single-TU mingw cross-build (-lddraw -ldxguid)

tests/
  Makefile                  host gcc + ASan/UBSan
  test_*.c                  372 tests across 6 modules
```

## Open RE threads (not picked up yet)

- **`FUN_0056aea0`** (3441 bytes) — title-menu scene runner.  See
  `docs/findings/title-scene.md` for phase breakdown + jumptable
  resolution.  Big port — multi-checkpoint.
- **`FUN_005b1030`** (156 bytes) — main message pump / frame waiter.
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
  still modelled as `uint8_t[]` (kept opaque to avoid pulling
  <ddraw.h> into the host build).
- SS_MGR / W_MGR / GD_MGR boot-pool allocators (gameplay-side, not
  blocking title menu).
- `PTR_DAT_0056bfa4` jumptable — 7 handlers INSIDE FUN_0056aea0 (not
  separate functions; resolved in `docs/findings/title-scene.md`).
- `ar_boot_register_all` and `pd_boot_init_slots` exist but are not
  called from WinMain — every batch is still a module in isolation.
- WndProc port is also a module in isolation.
- `FUN_00563ef0` wave-load second half is unported.
- Locale-table magic / sequence fields: 23 distinct values for the
  +0x00 magic field, +0x04 is per-locale group selector 1..73.
- `cs_engine_name_header` is "" at boot; populated by some unported
  settings init path.

## Resolved this checkpoint (14 chips landed)

- **`FUN_005b9490` `zdd_object_lock`** + Win32 `zdd_surface_lock`
  primitive (vtable[25] / byte 0x64).  DDERR routed through parent's
  log buffer.
- **`FUN_005b94d0` `zdd_object_unlock`** + Win32 `zdd_surface_unlock`
  (vtable[32] / byte 0x80).
- **`FUN_005b9410` `zdd_object_clear`** — Lock + zero-fill + Unlock.
  Reads pitch/height/lpSurface via a new `zdd_object_get_locked_info`
  Win32 primitive (4-byte vs 8-byte pointer mismatch handled).
- **`FUN_005b9b70` `zdd_object_blt_keyed`** — variant of blt_onto with
  positioned dest origin (metric_0c/_10) + DDBLT_KEYSRC.
- **`FUN_005b8a20` `zdd_bind_pixel_format`** + GetSurfaceDesc Win32
  primitive (vtable[22] / byte 0x58).  Stamps a new 22-byte
  `zdd_color_descriptor` struct at the start of the zdd (replacing
  the unobserved `_pad000[0x18]` region).  Wired into
  `zdd_create_screen`'s post-success branch when bpp==16.
- **`FUN_005b8b00` `zdd_color_convert`** — RGB888 → surface-native
  pixel pack.  Wired into `zdd_object_set_color_key`'s 16bpp branch.
- **`FUN_005b8ea0` `zdd_object_upscale_16bpp`** — 285-byte 2x software
  scaler.  Wired into `zdd_present` mode 4 (Zoom).
- **`FUN_005b9ac0` `zdd_object_is_lost`** + Win32 `zdd_surface_is_lost`
  (vtable[24] / byte 0x60).
- **`FUN_005b9ab0` `zdd_object_restore_surface`** + Win32
  `zdd_surface_restore` (vtable[27] / byte 0x6c).
- **`FUN_005b91d0` `zdd_check_any_surface_lost`** — 4-slot dispatcher
  (com_a + primary_obj + back_obj_a + back_obj_b).
- **`FUN_005b9240` `zdd_restore_all_surfaces`** — blanket Restore on
  the same 4 slots.

**Naming-discovery note** (carried from prior checkpoints): Ghidra's
`paint_ctx::` label on FUN_005b94e0/_9500/_9130 is a misnomer — the
ECX in every live callsite is `zdd_object*` (specifically
`zdd.primary_obj`).  The `wnd_proc.h` `paint_ctx` typedef is actually
a `zdd` alias.

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
