# Session handoff — last updated 2026-05-25 (smoke-present working + 2 RE fixes)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

The drop-in now drives a full per-frame **smoke-present** in launcher
mode 2 (Windowed): `BltColorFill(0xF800)` on the offscreen primary
surface, then `GetDC` → `BitBlt` → `ReleaseDC` onto the window HDC.
That mirrors the windowed branch of retail's `FUN_005b8fc0` (engine's
"Title Menu - Flipping" path).  The harness runs five frames with zero
DDERR — the surface chain is verified end-to-end usable.

Two real RE bugs surfaced while wiring smoke-present and are now fixed
in the same commit:

1. `zdd_object_create_surface_pair` was passing the wrong args to
   `zdd_object_stamp_metrics`.  Retail calls 98c0 with
   `(p1, p2, p6, p7, p8/width, p9/height)`, not `(p1..p6)` — confirmed
   by r2 disasm at `0x5b95ff–0x5b9617`.  Fixed.  metric_b8/bc now
   correctly hold width/height (640/480) at boot.
2. `zdd_object_attach_clipper` passed a NULL stack pointer where
   retail builds a real `{RGNDATAHEADER, RECT}` from
   `metric_b8/metric_bc`.  Fixed via primitive rename
   `zdd_clipper_set_clip_list_null` → `_rect`.  Vtable[7] (byte 0x1c)
   confirmed for `SetClipList`, resolving the prior open ambiguity.

`zdd_output_debug_string` now dual-sinks to stderr (in addition to
`OutputDebugStringA`) so DDERR messages are visible in the harness
output without a DbgView attach.

Six modules ported + wired:

- **Pixel-Drawer** — 8 functions, 39 host tests.
- **Asset-Register** — 31 functions, 111 host tests.
- **Bitmap-Session** — 8 functions, 31 host tests.
- **WndProc** — `FUN_005b12e0`, 20 host tests.
- **ZDD wrapper** — 26 functions covering ctor/dtor + DDERR log +
  DirectDrawCreateEx + SetCooperativeLevel + SetDisplayMode +
  GetDisplayMode + busy_wait + clipper attach (now correct RGNDATA) +
  back-buffer attach + 8bpp palette setup + primary-surface DDSD
  builder + `zdd_create_screen` (full 5-mode dispatch) + 3 smoke-present
  primitives (Blt-COLORFILL, GetDC, ReleaseDC).  88 host tests.
- **cs_dispatch** — `cs_dispatch_create_screen`, 21 host tests.

Total host tests: **307 pass, 0 fail, 6 skip** (unchanged; updates to
the orchestrator + clipper tests stayed within their existing slots).
Cross-build with mingw clean.

**Live-boot output under harness** (`tools/run-opensummoners.sh
--frames 5`):

```
[opensummoners] init_ddraw: launcher_mode=2 (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)
[opensummoners] init_ddraw: SetCooperativeLevel ok (fullscreen=0)
[opensummoners] init_ddraw: CreateScreen dispatch returned (success path)
[opensummoners] OpenSummoners exiting after 5 frames (81 ms wall)
[launcher] child exited (rc=0)
```

(Zero DDERR lines; if anything fails it now shows up as
`[ddraw-log] Warning,exists ZDD errors,...`.)

CLI knobs available on the drop-in:
- `--launcher-mode=N` (0/1/2/3/4; default 2)
- `--skip-ddraw` — bypass init entirely (window-only boot)
- `--no-smoke-present` — skip the per-frame Blt+BitBlt (e.g., when
  the real scene runner takes over)

**Open boot-path gaps**:
- `FUN_005b8a20` (16bpp pixel-format binding) still TODO in
  `zdd_create_screen`'s post-success hook.  Smoke-present succeeds
  without it, so it may be cosmetic (channel mask refinement) or it
  may matter once non-trivial content draws.
- `FUN_005a4770` (45 KB launcher settings record parser) replaced
  for now by `--launcher-mode=N` CLI flag.

Most recent commits (newest first):

- (current) `5d82301` ZDD: smoke-present mode 2 (Blt+GetDC+BitBlt) +
  2 RE bug fixes it surfaced
- `39efc7f` main: wire cs_dispatch_create_screen into WinMain
  (mode-2 Windowed)
- `cfc1242` cs_dispatch: port FUN_00582e90 outer CreateScreen dispatcher
- `8b9e539` ZDD: port FUN_005b8480 CreateScreen body (5-mode dispatch)
- `d415dd5` ZDD: port FUN_005b9740 back-buffer attach + FUN_005b8e00 8bpp palette
- `8a9d536` ZDD: port 4 CreateScreen leaf helpers + map mode dispatch
- `7f5a001` ZDD: port FUN_005b9520 clipper attach (create + clear + attach)
- `6024a36` ZDD: port FUN_005b8b40 CreateSurfacePair factory + orchestrator returns int

## Active goal

**Get the title menu rendering.**  The surface chain is now confirmed
working — next step is replacing the placeholder smoke fill with the
engine's real content path.  That means porting the scene runner so
its draws fire into `primary_obj->com_primary` and we let it call its
own Flip/present.

## Next move (pick one — recommendation first)

1. **(recommended) Port `paint_ctx::FUN_005b94e0` / `FUN_005b9500` +
   `FUN_005b8fc0` (present dispatcher) as a proper module.**  Now
   that smoke-present proves the windowed path works, formalise it.
   `paint_ctx::FUN_005b94e0` is GetDC, `_9500` is ReleaseDC — both
   trivial wrappers over `zdd_object->com_primary` (offset 0x2c —
   verified by r2 disasm of `_94e0` at `0x5b94ed`, NOT offset 0 as
   the Ghidra "this->zdd_device" label suggested).  `FUN_005b8fc0`
   is the 5-mode present dispatcher (case 0 = Flip on com_a, case 1
   = Blt com_a, case 2 = paint_ctx-GetDC+BitBlt, case 3 = back-buffer
   blit+Flip, case 4 = two-stage zoom blit).  Once ported, the
   drop-in's smoke loop can be torn out — engine code takes over.

2. **Port the title-menu scene runner `FUN_0056aea0`.**  The first
   *real* drawer that touches `primary_obj`.  Likely multi-checkpoint
   because of its many unported callees (PTR_DAT_0056bfa4 jumptable,
   `FUN_00563ef0` wave-load).  Pair well with #1.

3. **Port `FUN_005b8a20`** (16bpp pixel-format binding, 181 bytes).
   ECX identity still ambiguous — investigate via r2 disasm + a Frida
   hook that logs ECX at first live call.  Smoke-present works without
   it; defer until real content shows colour-channel artefacts.

4. **`FUN_005a4770` launcher-settings parser** (45 KB).  Replaces the
   `--launcher-mode=N` CLI flag with a real `user/config.dat` read.
   Lower priority than getting pixels.

## Active modules / file layout

```
src/
  main.c                    WinMain shim + DDraw init + per-frame
                            present_smoke_frame (Blt+GetDC+BitBlt),
                            --launcher-mode/--skip-ddraw/
                            --no-smoke-present flags
  dev_hooks.c/h             MessageBox redirect prologue patch
  pixel_drawer.c/h          ZDPixelDrawer — 8 functions
  asset_register.c/h        Asset-register slots — 31 functions
  asset_register_win32.c    GDI primitive wrappers
  bitmap_session.c/h        PE-resource bitmap decoder — 8 functions
  bitmap_session_win32.c    LocalAlloc/Free + FindResource/LoadResource
  wnd_proc.c/h              Main game window WndProc — pure dispatch
  wnd_proc_win32.c          DefWindowProcA + ExitProcess + placeholders
  zdd.c/h                   ZDD wrapper — full ddraw init chain + 5-mode
                            CreateScreen + clipper attach with RGNDATA.
                            New: 3 smoke-present primitives
                            (zdd_surface_blt_color_fill,
                            zdd_surface_get_dc, zdd_surface_release_dc).
                            Renamed: zdd_clipper_set_clip_list_null →
                            _rect (now takes width/height).
  zdd_win32.c               DirectDraw7 + IDirectDrawSurface7 +
                            IDirectDrawClipper + IDirectDrawPalette
                            primitive wrappers.  Now dual-sinks
                            OutputDebugStringA to stderr.
  cs_dispatch.c/h           Outer CreateScreen mode dispatcher
                            (FUN_00582e90) + per-mode handlers.
  cs_dispatch_win32.c       Win32 primitives — error log fetch,
                            fatal log, fatal+lasterror, ExitProcess.
  Makefile                  single-TU mingw cross-build (-lddraw -ldxguid)

tests/
  Makefile                  host gcc + ASan/UBSan
  test_*.c                  307 tests across 6 modules

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
  Smoke-present succeeds without it, so likely cosmetic.
- **`FUN_005b8b00`** (16bpp colorkey channel converter) — reads byte
  shift tables off ECX, ECX identity also ambiguous (likely pixel-
  format descriptor, NOT the calling ZDDObject).  TODO at
  `zdd_object_set_color_key`'s 16bpp branch.  Dead at boot (sentinel
  colorkey wins) but live for real scene content.
- **Launcher settings record parse** — `FUN_005a4770` (45 KB) reads
  `user/config.dat`'s XOR-obfuscated body.  Replaced for now by the
  `--launcher-mode=N` CLI flag on the drop-in.
- ZDDObject's +0xac field (`com_back`) is dual-role: holds either a
  back-buffer `IDirectDrawSurface7` OR an `IDirectDrawClipper`.  Both
  implement IUnknown so lifecycle doesn't care, but field naming is
  misleading.
- ZDDObject's 124-byte embedded DDSURFACEDESC2 (+0x30..+0xab) is
  still modelled as `uint8_t[]` — typed access requires `<ddraw.h>`.
- `FUN_005bd040` mode-3/mode-4 LUT formulas have arithmetic whose
  "floor-correction" terms are zero for valid weight ranges.
- SS_MGR / W_MGR / GD_MGR boot-pool allocators (DAT_008a8440 / _6ec4
  / _9274) are dependency-of ~30 functions; defer until consumer
  semantics map cleanly.
- `PTR_DAT_0056bfa4` jumptable inside the title-menu runner.
- `ar_boot_register_all` **and** `pd_boot_init_slots` exist but
  neither is called from the drop-in's WinMain — every batch is still
  a module in isolation.  (cs_dispatch + ddraw init *are* wired.)
- `g_ar_info_table[909]` — pool modelled in full and the 443 writes
  inside FUN_0057ca40 are PORTED via `ar_apply_group3_info_events`.
- The WndProc port is also a module in isolation.
- `FUN_00563ef0` wave-load second half is unported.
- Locale-table magic / sequence fields: 23 distinct values for the
  +0x00 magic field, +0x04 is per-locale group selector 1..73.
- **`cs_engine_name_header`** is "" at boot; populated by some
  unported settings init path.

## Resolved this checkpoint

- ZDDObject +0x00..+0x08 are self-pointers into the embedded DDSD
  (NOT the IDirectDrawSurface7 the Ghidra "this->zdd_device" label
  suggested).  The `paint_ctx::FUN_005b94e0` GetDC wrapper actually
  reads offset 0x2c (com_primary) — verified by r2 disasm at
  `0x5b94e0`: `mov eax, [ecx + 0x2c]`.  Ghidra named the field
  based on usage; the offset disambiguates.
- `FUN_005b9520` SetClipList uses vtable[7] (byte 0x1c) — confirmed
  by r2 at `0x5b95a7`.  NOT vtable[8] SetHWnd as a prior open thread
  hedged.
- `FUN_005b95c0`'s call to `FUN_005b98c0` uses `(p1, p2, p6, p7,
  p8/width, p9/height)`, NOT `(p1..p6)`.  See orchestrator docstring
  in `src/zdd.h` for the full disasm citation.

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
