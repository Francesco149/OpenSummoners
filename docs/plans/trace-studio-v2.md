# Trace Studio v2 — the native-capture, tick-joined parity studio

> Update (ckpt 159): **TICK-OFFSET phase alignment** added to the dual viewer
> (`osr_view.exe <port> <retail> [offset]`).  The tick-join matches identical sim_ticks,
> which only overlays two SEED-PINNED LOCKSTEP captures; two different play sessions (a
> port replay vs a USER real-play recording — different freeroam entry ⇒ the same action
> lands on different ticks) never aligned, so a movement/sword diff couldn't be scrubbed
> 1:1.  Now the join shifts retail by +offset on the port tick axis; live-nudge with
> `[` / `]` (Shift=10, `\` reset), and the studio-current.txt shortcut can bake an initial
> offset as a 3rd token.  Local alignment (un-synced runs drift, so nudge per region) —
> honest, and the right tool for comparing real-play recordings.  `build_join(port,retail,offset)`
> in `osr_view_imgui.cpp`.
>
> Status (2026-06-13, ckpt 129): **M1..M6 COMPLETE + M7 partial** — this ckpt landed
> the TICK-JOIN STUDIO (M6) AND its drill-in + note hand-off (M7).  M6: both sides'
> `.osr` paired by the deterministic `sim_tick` (the identity join, openrecet E3 —
> honest gaps, NO flip-drift search) via `pair.py` (verdict) + a streaming `osr.py`
> (no OOM on the 1.9 GB capture), and `osr_view` grows a native PORT|RETAIL|DIFF
> three-panel + a diff heat ribbon.  M7: a NOTE/mark system (drag a crop + type →
> `osr_notes.jsonl` → `notes.py` renders the cropped port|retail|diff for the agent —
> the human→agent contract) + a DRAW INSPECTOR (render-up-to-K / draw list / pixel→draw
> pick, openrecet N3).  USER-CONFIRMED the GUI ("studio looks good"); the M7 crop-drag /
> inspector interaction is the open visual-verify.  Verified headless: sim_tick 0
> reconstructs `differ_px==0` port-vs-retail (it's the game_enter TRANSITION frame —
> near-black, the inspector showed a late mid-frame CLEAR; the town is a few ticks in).
> NEXT: RESUME the now-unblocked room-render/freeroam port (the studio exists to iterate
> it); studio polish (survey 4/5/6) is pull-when-needed.  (history below.)  M1..M4:
> M1..M4 COMPLETE — **M4d the `--validate` gate
> LANDED and PASSES 71/71 snaps `differ_px==0`** (real retail backbuffer snapshots
> sampled every 200 flips across boot→title→newgame-menu→prologue→town→dialogue→
> house-freeroam, each compared against the reconstruction; `OSR_SNAP` record +
> proxy flip-hook Lock + the recon's per-snap compare).  The gate's ONE failure
> (flip 800, the newgame menu) root-caused the USER-flagged "menu CLIPPED
> artifact": retail clears the compose surface at scene transitions via
> `FUN_005b9410` and the menu does NOT fully redraw (quirk #105) — now captured
> as `OSR_CLEAR` (ordered draw, proxy-filtered to the backbuffer) and replayed by
> recon + osr_view.  M4: the port's `--osr-replay` mode (`src/osr_replay.{c,h}`
> streaming reader + `src/osr_recon.c` Win32 reconstructor) rebuilds frames 1:1
> through zdd.c blits + real GDI text + BMP snapshots; mode-4 ALPHA captured
> (`OSR_BLEND`).  NEXT: M6 the tick-join studio over osr_view (M5 port emitter DONE ckpt 128).
> (history below.) M1+M2: proxy auto-loads, native
> headless turbo boot, the INT3+VEH engine-VA detour layer, ring input injection →
> seed-pinned lockstep boot to game_enter with all anchors. M3a: the `.osr` format
> (`src/osr_format.h`) + the bg-thread ring writer (`osr_writer.h`) + the cheap
> records (FRAMEBEG/PRESENT/ANCHOR/SEED) + `osr.py`. M3b: the BLIT op stream — the
> 5 blit detours + the cel→(res,frame) registry (`render_id.h`, onEnter-computed) →
> OSR_BLIT records, FULL TURBO (~950 fps, E9-jmp trampoline `50ec26b`). M3c: source
> pixels + surface identity — NO COM vtable wrap needed (the blit decompiles showed
> cel/dest hold a real IDirectDrawSurface7* at +0x2c), so the proxy interns surface
> ptrs (`surface_id.h`) + grabs source pixels from the blit detour (`sheet_grab.h`)
> → SHEET records + BLIT dhash/dst_handle + header pixfmt re-stamp. PROVEN on a
> nav→town capture (dst_handle/dhash 100% set, 496 SHEETs/420 distinct, 912 fps).
> M3d: GDI text → TEXT/FONT — IAT-hooked gdi32!{TextOutA,CreateFontIndirectA,
> SelectObject,SetTextColor,SetBkMode}; the .osr is now a COMPLETE frame
> description. M4 (reconstruct, --osr-replay) is NEXT.
> Built in isolation from v1 (`tools/trace_studio*`, `tools/frida_capture.py`,
> the Frida agent) — none of those are touched until v2 is proven, at which point
> v1 is archived. The USER pulled this forward before porting the freeroam scene
> because v1 capture is too slow/coarse to iterate the render parity at the
> granularity the room-render + freeroam work needs.

## Why v1 is the bottleneck (measured, see the digest in this checkpoint)

v1 captures retail through **Frida**, and every cost sits on the engine thread or
the wire:

1. **Per-frame GDI readback** — `GetDC`/`CreateCompatibleDC`/`CreateDIBSection`/
   `BitBlt`/`SelectObject`/`DeleteObject`/`DeleteDC`, all synchronous, per frame
   (`opensummoners-agent.js` `captureFrame`).
2. **Per-frame Frida `send()` of ~900 KB** (640×480×24bpp) over the WSL2→Windows
   **TCP NAT** (`cutestation.soy:27042`) — one round-trip per frame, then PNG
   (zlib-9) on the Python side.
3. **`--no-turbo` forced for capture** (quirk #29): the GDI surface-readback path
   races the flip under the virtualised clock, so capture runs at ~real wall-clock
   instead of turbo. This is the single biggest throughput cap.
4. **Sparse capture + sticky-drift pairing** — v1 captures selected frames and
   pairs them on the **flip axis** with a ±drift pixel search, which *hunts*
   through content-quiet stretches and absorbs retail's coalesced ticks as ±1–2
   tick error. Converging two sides is hand-work (push/pull the per-side traces).

Net: a capture is minutes of wall-clock for a sparse, flip-paired session, and the
diff granularity is "which frame", not "which draw".

## The v2 thesis (USER-directed, 2026-06-12)

Three radical changes, each removing a whole class of cost:

1. **Capture natively, in-process, with a proxy `ddraw.dll`** — no Frida, no GDI
   round-trip, no TCP. The retail exe imports `DirectDrawCreateEx` from `DDRAW.dll`
   and has **relocations stripped / fixed ImageBase `0x00400000`** (verified:
   `objdump -p` → "relocations stripped", no `DYNAMIC_BASE`). So:
   - A proxy `ddraw.dll` co-located with the exe (we already drop a per-run copy of
     the unpacked exe into the game dir) **auto-loads** — exe-directory is first in
     the DLL search order — with **no injector**.
   - Engine VAs are **absolute and stable** (base never moves) → inline detours at
     hardcoded VAs need zero base-fixup.
   - The proxy **wraps the DirectDraw7 + Surface7 COM vtables**, so it sees every
     `Blt`/`BltFast`/`Lock`/`Flip` and intercepts the draw stream **in-process**.

2. **Capture the DRAW-CALL STREAM, not pixels; reconstruct the frame 1:1 ON
   WINDOWS.** (The USER's explicit direction — and it dissolves the GDI blocker; see
   the fork section.) Per frame, record the ordered list of draw ops (the 5 DDraw
   blits + GDI `TextOut` text + clears/fills + the present) with full state, plus
   the **dedup'd source surfaces** each blit reads from. The framebuffer is *never*
   snapshotted on the hot path — capture just logs small call records, so per-frame
   cost collapses (source sheets are captured once and reused). A separate
   **Windows-side replayer** re-executes the recorded calls to produce the exact
   frame as displayed — because it runs on Windows, real GDI reproduces text
   bit-exact. This is the order-of-magnitude win: the hot path logs calls; pixel
   reconstruction is an off-line batch step.

3. **Join the two sides by the deterministic sim-tick, not by pixel-drift search**
   — capture *all* frames tagged with their sim-tick on both sides under
   seed-pin + lockstep; pairing becomes a hash-join on `sim_tick`. Anchors validate
   the join (RNG state must match). A tick-count divergence IS the bug (phase
   pillar), surfaced — never papered over with drift.

## The capture-vs-replay fork (RESOLVED — USER, 2026-06-12)

The USER's sketch: "capture draw calls, state, gdi etc so we can reconstruct the
frame 1:1; everything is done on the Windows side and WSL just orchestrates it;
Windows is the same machine as this WSL."

My initial objection was that **text/glyphs render via Windows GDI `TextOutA`**
straight onto the backbuffer (`text-glyph-pipeline.md`), outside the 5 blit
primitives, and the Windows font rasterizer can't be reproduced **offline on
Linux**. The USER's correction removes the objection: **the replayer runs on
Windows**, so it calls the *real* GDI — text reconstructs bit-exact. Everything
heavy (capture + reconstruction) is a Windows-native step; WSL only orchestrates
and reviews (it sees the artifacts via `/mnt/c`, same machine).

So v2 records the **call stream + state + GDI + dedup'd source surfaces** and a
Windows replayer reconstructs each frame. We get *both* USER goals — capture an
order of magnitude faster (log calls, not 600 KB/frame of pixels) AND a rich draw
trace for granularity — and parity holds because reconstruction is real Windows
DDraw/GDI, not a Linux reimplementation.

**The replayer = the port binary in a new `--osr-replay` mode.** The port already
*is* a faithful Windows renderer: `zdd.c` reimplements the 5 blits bit-exact
(parity-ledger-verified) and `glyph_render_win32.c` drives real GDI `TextOutA`.
Feeding it an `.osr` and replaying the recorded ops through those exact sinks
reconstructs the frame on Windows with maximum code reuse. **One-time fidelity
check:** reconstruct a retail frame from its `.osr` and compare to a single real
GDI snapshot of that same frame (a `--validate` capture) → must be `differ_px==0`;
thereafter trust reconstruction. (If a blit case isn't yet zdd-faithful, the
reconstructor can fall back to real DirectDraw surfaces — deferred unless the
fidelity check fails.)

**Incremental delivery (USER):** "scrub frame by frame first for the 1:1 pixel
perfect comparison, then add ways to drill into the draw calls and data later."
So the first cut delivers the reconstructed **frame scrub** (port|retail|diff,
tick-joined); the draw-call/data drill-in is layered on after, growing v2 into a
complete replacement for v1.

## Architecture

```
   WINDOWS (same machine; WSL sees it via /mnt/c)              WSL2 (orchestrate)
  ┌─────────────────────────────────────────┐         ┌──────────────────────────┐
  │ sotes.unpacked.exe  (fixed base 0x400000)│         │ tools/trace_studio2/      │
  │   imports DirectDrawCreateEx ───────────┐│         │                          │
  │ ┌─────────────────────────────────────┐ ││         │ capture.py drive + collect│
  │ │ ddraw_proxy.dll  (OUR native C DLL)  │◀┘│ retail  │ pair.py    tick-join      │
  │ │  • forwards real DDRAW exports       │ ─┼─.osr──┐ │ osr.py     reader/decode  │
  │ │  • wraps DDraw7/Surface7 vtables     │ ││       │ │ server.py  :8780 viewer   │
  │ │  • inline detours @ engine VAs       │ ││       │ │ web/       v1 UX reused   │
  │ │  • RECORD draw/GDI/state → ring →    │ ││       │ └──────────────────────────┘
  │ │    bg thread: dedup + miniz → .osr   │ ││       │       ▲ reads reconstructed
  │ └─────────────────────────────────────┘ ││       │       │ PNGs + diff via /mnt/c
  ├─────────────────────────────────────────┤│       ▼
  │ opensummoners.exe --osr <out>  (PORT)    ││  ┌─────────────────────────────────┐
  │   src/osr_emit.c → SAME .osr ───────────────.osr─▶│ opensummoners.exe --osr-replay │
  └─────────────────────────────────────────┘│  │   reuses zdd.c blits + real GDI  │
        capture = log calls (fast)            │  │   → reconstruct frame 1:1 → PNG  │
                                              │  └─────────────────────────────────┘
                                              │     RECONSTRUCT (Windows, off hot path)
```

Two Windows-native steps (capture, reconstruct) + WSL orchestration. The `.osr`
files and reconstructed PNGs live on the Windows FS; WSL reads them via `/mnt/c`.
**No Frida, no TCP, no per-frame pixel transport anywhere.**

### Component 1 — `ddraw_proxy.dll` (native, C, mingw32 — already in the flake)

Built by the **existing** `i686-w64-mingw32-gcc` (single-TU, like the port). Lives
in `tools/capture_proxy/` (source) → `build/ddraw_proxy.dll`. Responsibilities:

- **Export forwarding.** Export `DirectDrawCreateEx` (the only DDRAW import) and
  forward the rest to the real `C:\Windows\SysWOW64\ddraw.dll` (load it explicitly
  by absolute path so we don't recurse into ourselves). mingw `.def` forward-exports
  for all stock DDRAW entry points, a real wrapper only for `DirectDrawCreateEx`.
- **COM vtable wrapping.** On `DirectDrawCreateEx`, return a wrapper `IDirectDraw7`
  whose `CreateSurface` returns wrapper `IDirectDrawSurface7`s. This lets us assign
  each surface a **stable handle** so blit records can name their src/dst surfaces,
  and capture a surface's pixels **once** (the dedup'd SHEET) the first time it is
  blitted FROM (or when its bits change). `Flip`/present-`Blt` → **frame boundary**.
- **The DRAW STREAM is recorded at the inline VA detours**, not the vtable — the
  engine's 5 blit VAs carry the `res`/`frame`/state semantics the digests document
  (the existing `retail_fields.json` mapping). The vtable wrap supplies surface
  identity + the one-time source-pixel grab + the real `DDSURFACEDESC2` (format).
  **GDI text** is captured by hooking `gdi32!TextOutA`/`ExtTextOutA` +
  `SelectObject`/`SetTextColor`/`CreateFontIndirectA` (record font params, string,
  pos, color, target — the replayer re-issues these on Windows → 1:1).
- **Inline detours at fixed engine VAs** (no base math — base is 0x400000):
  | VA | role | v1 source |
  |----|------|-----------|
  | `0x5b8fc0` | present/flip — frame boundary, flip++ | agent Flip hook |
  | `0x43d1d0` | easer — `sim_tick++` (reset at game_enter) | agent sim-tick |
  | `0x56c070` | first sparkle — seed pin + `subtitle_anim_start` | agent anchor |
  | `0x564780`/`0x56cd20`/`0x59f2c0` | newgame/prologue/game_enter anchors | agent |
  | `0x41f200` | town spawn — second seed re-pin | agent rng_anchor |
  | `0x43c110` | ring poll — discrete input inject | agent ring |
  | `0x5ba520` | keyboard leaf — held-axis inject | agent held |
  | `0x5bf4fb`/`DAT_008a4f94` | LCG seed write | agent seed |
  - Inline hook mechanism: a tiny hand-rolled 5-byte `E9 jmp` trampoline (save the
    overwritten head bytes, jmp to our handler, handler does work + runs the saved
    bytes via a relay stub, returns). Engine funcs are stdcall/thiscall with known
    prologues; for onEnter-only hooks (anchors, sim-tick, seed, input) a head-patch
    + relay is enough and avoids a full disassembler. Vendor nothing — ~150 lines.
- **Turbo / lockstep clock** via **IAT patching** of `kernel32!GetTickCount`,
  `winmm!timeGetTime`, `kernel32!Sleep`, `user32!WaitMessage` (same virtual-clock
  model as the agent: per-call advance gated on the pump-entered latch; lockstep =
  freeze between flips, advance one quantum at the flip hook). IAT patch on the
  exe's import table = near-zero per-call cost vs Frida's `Interceptor`.
  **Turbo-safe capture**: because the frame grab is an in-process `memcpy` off the
  surface the engine *just finished drawing* (at the present hook, before Flip), it
  does **not** depend on GDI surface-sync — so quirk #29's "capture ⇒ --no-turbo"
  constraint is GONE. v2 captures at full turbo.
- **The `.osr` writer** (see format below) runs the dedup + compression on a
  **background thread** off a lock-free-ish ring, so the engine thread only pays a
  `memcpy` into the ring per frame.

### Component 2 — port-side `.osr` emitter (`src/osr_emit.{c,h}`)

The port is our code, so no proxy: emit the **same `.osr`** call stream directly.
Hook points already exist (the digest): `drive_present` (FRAMEBEG/PRESENT + flip +
rng), `g_sim_tick_count`, `zdd_emit_blit` (the 5 primitives → BLIT records, with the
source `zdd_object`'s decoded sheet → SHEET, dedup'd by the existing `dhash`),
`glyph_render_win32` TextOut sink (→ TEXT + FONT), `emit_anchor`. This sits beside
the existing BMP path (untouched, v1 keeps working). Same dedup/miniz writer shared
with the proxy (`osr_write.c` is common C compiled into both).

### Component 3 — `.osr` binary format (dedup'd, fast)

One file per side per session. Append-only, recoverable if the run is killed.

```
header   : magic "OSR1", side(port|retail), screen_w, screen_h,
           screen_pixfmt(RGB565|XRGB8888|PAL8), seed, scenario id,
           flags(turbo/lockstep)
stream of records, each: { u32 type, u32 len, payload }
  FRAMEBEG: flip, sim_tick, anchor_id?         ← opens a frame's op list
  CLEAR   : surface_handle, color              ← memset/fill ops
  BLIT    : va, seq, src_sheet_ref, dst_handle, dx,dy,reqw,reqh, sx,sy,
            ow,oh,ox,oy, state, ckey, bmode     ← one of the 5 primitives
  TEXT    : dst_handle, x, y, str, font_ref, color, bk_mode  ← GDI TextOut op
  PRESENT : mode, src_handle                    ← Flip/Blt; closes the frame
  SHEET   : sheet_ref, res_id, frame_idx, dhash, w, h, pixfmt, codec, bytes
                                                ← dedup'd source pixels (once per
                                                  unique dhash; the replay inputs)
  FONT    : font_ref, LOGFONTA fields           ← dedup'd; replayer recreates HFONT
  PALETTE : surface_handle, 256×RGB             ← for PAL8 src/dst
  ANCHOR  : name, flip, sim_tick, rng
  SEED    : flip, before, value
  INPUT   : flip, kind(ring|held), ids/keys
```

**Why this is small + fast to capture**: the hot path only appends tiny op records
(BLIT ≈ 40 B, TEXT ≈ name+string). The bulky pixel data (SHEET) is **dedup'd by
`dhash`** and written **once** — the same decoded sprite sheet is reused across
thousands of frames, so a whole scenario's source pixels are a handful of sheets.
`miniz` (vendored single-file zlib, public-domain) compresses each unique sheet.
No framebuffer is ever copied on the hot path. A 2-minute capture is the op log
(a few MB) + a few compressed sheets (tens of MB), vs v1's per-frame 900 KB sends.

Transport / **storage discipline (USER, 2026-06-12)**: the proxy writes the `.osr`
to **native NTFS on `C:\`** (a Windows-local staging dir, e.g. `%TEMP%\oss-osr\` or
`C:\oss-osr\`) — **never** through the WSL 9p mount (`\\wsl.localhost\…`), which
would put a slow plan9 round-trip on the capture hot path. Same for the
reconstructor's PNG output. WSL only **reads** these via `/mnt/c` (DrvFs) during
orchestration/review — off the hot path. **No live streaming, no Frida, no TCP** —
capture speed is decoupled from the host link entirely. (The proxy's debug log
already follows this: `%TEMP%\oss_proxy.log`, a `C:\` path.)

### Component 3b — the reconstructor (`opensummoners.exe --osr-replay`, Windows)

A new mode of the **port binary** (max reuse of proven, bit-exact sinks). Reads an
`.osr`, rebuilds the source surfaces from SHEET records (into `zdd_object`s) and the
HFONTs from FONT records, then for each frame replays its op list in order —
`CLEAR`/`BLIT` through the existing `zdd.c` primitives, `TEXT` through
`glyph_render_win32.c`'s real GDI `TextOutA`, `PRESENT` snapshots the result — and
writes the reconstructed frame as PNG named `frame_<flip>_t<tick>.png`. Because it
runs on Windows, GDI text is bit-exact with retail. Same binary reconstructs BOTH
sides' `.osr` → a clean apples-to-apples comparison of the two engines' draw
streams. **Fidelity gate:** a `--validate` capture grabs one real GDI snapshot per
side that the reconstructed frame must match `differ_px==0` before we trust replay.

### Component 4 — the studio host (`tools/trace_studio2/`, Python — orchestrate only)

WSL does **no pixel work** — Windows captures and reconstructs; WSL drives and
reviews.

- `osr.py` — `.osr` reader (parse records, resolve refs) for pairing/verdict/draw
  inspection (the reconstructed PNGs come from the Windows reconstructor, read via
  `/mnt/c`).
- `capture.py` — drive: (1) drop exe + `ddraw_proxy.dll` into the game dir, spawn
  retail via WSLInterop (**no Frida**) → retail `.osr`; (2) run the port `--osr` →
  port `.osr`; (3) run `opensummoners.exe --osr-replay` on each `.osr` → the
  reconstructed PNG dirs.
- `pair.py` — **tick-join**: group each side's frames by `sim_tick`, take the last
  flip per tick (the presented frame), inner-join on `sim_tick`. Emit `state.jsonl`
  keyed by ordinal = the joined tick index. Anchors checked (RNG match per anchor);
  tick-set difference reported as the phase-pillar verdict.
- `diff.py` — per-paired-frame `differ_px` (numpy over the reconstructed PNGs) +
  later the **draw-level** diff (reuse `render_diff.py`'s classifier over the two
  draw streams at the joined tick: `[sprite]`/`[decode]`/`[rect]`/`[state]`).
- `server.py` (:8780) — the viewer backend, reusing the v1 web UX verbatim
  (`tools/trace_studio_web` — same scrub/mark/worklist loop, the one thing v1 got
  right per the USER). **First cut: the frame scrub** (port|retail|diff, tick-joined
  reconstructed frames). **Later: a draw-inspector panel** (the draw list for the
  focused tick, click a draw → highlight its dest rect; step draws within a frame).

## Convergence discipline (the "don't hand-sync" system)

The robustness rule set, enforced by the tooling, not by hand:

1. **Both sides seed-pinned** (`OSS_RNG_DEFAULT_SEED = 0x4f5347`; retail proxy
   writes `DAT_008a4f94` at the same two anchors v1 uses).
2. **Both sides lockstep** (1 update / present) so flip↔tick is ~1:1.
3. **Capture ALL frames, tagged with deterministic `sim_tick`.**
4. **Join on `sim_tick`** (hash-join) — never a pixel-drift search. Frame `tick=T`
   on the port pairs with `tick=T` on retail, period.
5. **Anchors are assertions**: at each shared anchor the RNG state must match; the
   join prints a PASS/FAIL per anchor. A mismatch localizes the desync to a segment
   immediately.
6. **A tick-count divergence is THE finding** (the phase pillar) — the verdict
   names the first tick present on one side but not the other (retail coalesces /
   never-presents some ticks — quirk #99), instead of hiding it under drift. That's
   a real lead, surfaced for free.
7. **Re-capture is `--only port`** (retail `.osr` is cached) — the fast fix loop,
   same as v1.

This is what kills the "spend too much time pushing/pulling the trace to sync both
sides" problem: under a pinned seed + lockstep + all-frames + tick-join, the sides
are *already* synchronized by construction; the only question the tooling answers
is "at which tick do the pixels first differ, and which draw caused it."

## Performance plan (and the "is retail bottlenecked in turbo" check)

- **Verify turbo is unthrottled**: audit the proxy's clock hooks so the only paced
  thing is the virtual clock advance; nothing in the proxy sleeps the engine
  thread. The `.osr` bg-writer must never block the engine thread (drop-to-disk
  ring with backpressure = grow the ring, never stall; if disk can't keep up, log a
  dropped-frame count rather than pause).
- **Measure flips/sec** at three stages: (a) proxy loaded, no capture; (b) +draw
  stream; (c) +source-sheet dedup. Target: capture adds < ~10% over uninstrumented
  turbo.
- **Hot-path cost is logging only**: append tiny op records; a source SHEET is
  grabbed + hashed + written **once per unique `dhash`**, not per frame. No
  framebuffer copy, no GDI `BitBlt`, no per-frame PNG, no TCP — all v1 per-frame
  costs are gone. Reconstruction (the pixel work) is a separate Windows batch step,
  off the capture hot path entirely.

## Build / run (ACTUAL — the native flow; the original ":8780 server" plan was
## abandoned for the native osr_view, USER ckpt 126)

```sh
# build the proxy DLL + the native viewer (same toolchain as the port):
nix develop --command make -C tools/capture_proxy      # → build/ddraw_proxy.dll
nix develop --command make -C tools/osr_view           # → build/osr_view.exe

# CAPTURE both sides' .osr (NO Frida; artifacts on native C:\oss-osr\):
tools/capture_proxy/run_proxy.sh <nav>                 # retail → C:\oss-osr\retail.osr
opensummoners.exe --osr-emit C:\oss-osr\port.osr [--osr-scenario NAME]   # port (inside nix develop)

# FULL INTRO → ERRANDS port .osr (to scrub the WHOLE intro side-by-side, ckpt 143):
#   nav = the matched-cadence dialogue nav for ALL 21 lines (arrival+house+errands),
#   regenerated from retail.osr (the "7:10" = the L7→L8 run-off lead, quirk #111).
#   GOTCHA: `nix develop --command … > file` prepends the dev-shell BANNER to stdout
#   → it pollutes the nav → "failed to load input trace".  Strip it with grep '^[#{]'.
nix develop --command python3 tools/trace_studio2/dialogue_timeline.py NAV \
  /mnt/c/oss-osr/retail.osr runs/cutscene-verify/nav-zspam-ext.jsonl 600 1900 0 1100 "7:10" \
  2>/dev/null | grep -E '^[#{]' > runs/cutscene-verify/nav-full-errands.jsonl
OPENSUMMONERS_FRAMES=8000 OPENSUMMONERS_TIMEOUT_MS=300000 tools/run-opensummoners.sh -- \
  --osr-emit 'C:\oss-osr\port-flutter.osr' \
  --input-trace runs/cutscene-verify/nav-full-errands.jsonl --no-frame-limit
#   PATHS: --osr-emit MUST be a native C:\… path (a WSL /mnt/c/… resolves to a
#   \\wsl.localhost UNC that fopen("wb") fails); --input-trace tolerates the repo-
#   relative path (it resolves to the UNC, which fopen("rb") DOES load).  Reaches the
#   errands (cutscene COMPLETE) at sim_tick ~3360; the 8000-flip run covers 0..~3440.

# VERDICT (tick-join PASS/gaps/anchor-RNG, runs from WSL, streams multi-GB files):
nix develop --command python3 tools/trace_studio2/pair.py <port.osr> <retail.osr>

# REVIEW (on Windows): the tick-joined PORT|RETAIL|DIFF scrub + diff ribbon + the
# frame-draw drill + the crop/note marks:
build/osr_view.exe C:\oss-osr\port.osr C:\oss-osr\retail.osr

# READ the USER's marks back (renders the cropped port|retail|diff at each mark):
nix develop --command python3 tools/trace_studio2/notes.py <port.osr> <retail.osr> --render
```

## Isolation + retirement

- v2 lives entirely under `tools/trace_studio2/`, `tools/capture_proxy/`,
  `src/osr_emit.*`, viewer on **:8780**. It does NOT import or modify
  `tools/trace_studio*`, `tools/frida_capture.py`, or the Frida agent.
- v1 stays the working tool until v2 reproduces a known session (e.g. `intro-1`)
  at parity and is faster. Then archive v1 (`tools/archive/`) and flip the docs.

## Build order (incremental — frame scrub first, per USER)

The riskiest/highest-leverage piece is the **proxy DLL on the real game**, so prove
it earliest. Each milestone is independently testable.

- **M1 — proxy loads + forwards. ✓ DONE (ckpt 125).** `ddraw_proxy.dll` forwards all
  DDRAW exports to the real SysWOW64 ddraw and wraps `DirectDrawCreateEx`; the game
  boots normally with it in place (auto-load proven, no regression).
- **M2 — clock + lifecycle. ✓ DONE (ckpt 125).** M2a: IAT turbo/lockstep clock +
  config + harness thread (headless turbo boot to `DirectDrawCreateEx`). M2b: the
  engine-VA detour layer — `va_detour.h` (INT3 + a vectored exception handler, NO
  length-disassembler), `engine_hooks.h` (flip+lockstep / sim-tick / one-shot title
  seed-pin / newgame·prologue·game_enter anchors / per-map RNG re-pin), `engine_input.h`
  (ring injection @0x43c110). PROVEN: seed-pinned lockstep headless turbo boot to
  `game_enter` (newgame@flip652 → prologue@1000 → game_enter@1242, RNG re-pin fires,
  sim_tick climbs ~1:1) at **~790 fps in-game vs v1's ~60fps `--no-turbo` cap**.
  DEFERRED to when freeroam capture needs it: the held-axis leaf inject (`0x5ba520` —
  needs a return-value override, i.e. an onLeave-style hook, not the onEnter framework).
- **M3 — `.osr` capture.** Record the draw/GDI/state stream + dedup'd sources via the
  COM wrap + VA detours + GDI hooks → `.osr` on the Windows FS. Validate the format
  round-trips (`osr.py` reads it). Sliced by risk:
  - **M3a — format + cheap records. ✓ DONE (ckpt 125, `8c42c02`).** `src/osr_format.h`
    (pure-C header-only codec, append-only, truncated-tail recoverable) + the bg-thread
    ring writer (`tools/capture_proxy/osr_writer.h`, the engine thread only locks+memcpy;
    a bg thread drains+fflushes so a hard kill loses ≤1 drain) + the records the boot
    already produces (FRAMEBEG/PRESENT per flip = the tick-join axis; ANCHOR; SEED) +
    `tools/trace_studio2/osr.py`. PROVEN: a real boot writes retail.osr (11585 frames,
    all 3 anchors at the M2b flips, both seed pins) at ~800 fps, no regression.
  - **M3b — the BLIT op stream. ✓ DONE (ckpt 125, `cc63407`).** Detoured the 5 blit VAs
    + the resolver `0x418470` (cel→`(res,frame)`, COMPUTED at onEnter from the decompile
    `cel = *(*(*slot)+frame*4)` — no onLeave needed; `render_id.h`) → BLIT records
    (`render_diff.py`'s 76-B schema). Restructured flip → FRAMEBEG-at-open / draws /
    PRESENT-at-flip + per-frame `seq`. PROVEN: 867k blits / 2377 frames driven to
    game_enter, **89% render-id-named**, all anchors + seed pins; the town establishing
    shot decodes coherently (res=1002 backdrop columns, res=2234 clipped 32×32 sub-tiles
    at the camera scroll, KEYSRC ckey 0xf81f). dhash/dst_handle stay 0 until M3c.
    **PERF FORK RESOLVED — FULL TURBO (measured), both chips landed:** (1) RWX pages
    (`ee55e5b`) — hooked pages made permanently RWX at install so the hot INT3 dance drops
    the per-patch `VirtualProtect`, ~25 → ~56 fps; (2) the E9-jmp trampoline (`50ec26b`,
    `trampoline.h`) — the 6 hot hooks (resolver + 5 blits) become inline 5-byte `E9` jumps
    (per-hook thunk pushad/pushfd→call cb→popad + a relay of the relocated head bytes;
    head bytes hardcoded from the unpacked exe, no length-disassembler), ZERO exceptions/
    hit, ~56 → **~950 fps** (full turbo). Rare hooks stay INT3+VEH. A 30 s run = 29k
    frames / 14.6M blits, geometry byte-identical to the INT3 baseline.
  - **M3c — surface identity + SHEET. ✓ DONE (ckpt 125).** The "risky COM vtable wrap"
    turned out UNNECESSARY: the blit decompiles (`5b9a40.c` etc.) showed each cel/dest holds
    a real `IDirectDrawSurface7*` at `+0x2c` and the engine calls `dest->Blt(&dr, src, &sr,
    flags, NULL)` via vtable +0x14 — so the proxy interns the RAW surface pointers
    (`surface_id.h`: ptr→stable handle) + grabs source pixels straight from the blit detour
    (`sheet_grab.h`: Lock READONLY on first sighting → FNV-1a dhash mirroring the port's
    `asset_register.c` seed order → one dedup'd SHEET per surface ptr), NO surface wrapping
    + NO per-blit COM overhead. `engine_hooks.h` reads the dest surface → `dst_handle` (+ the
    first one re-stamps the header pixfmt/screen from its `DDSURFACEDESC2`, applied at offset 0
    by the bg writer thread) and the src surface → SHEET → BLIT `dhash`. `OSR_SHEET` is a
    variable-length record (`osr_format.h`, `osr_enc_sheet_prefix` streams big payloads with
    one ring copy); `osr.py` decodes it. PROVEN (nav→town): `dst_handle` 100% set (1 distinct =
    backbuffer), `dhash` 100% set, 496 SHEETs / 420 distinct (9.4 MiB raw RGB565), header
    `640×480 RGB565`, all anchors + seed pins byte-identical to M3b, 90% named, **912 fps**
    (<4% over M3b). FOLLOW-UPS (tagged PORT-DEBT, NON-blocking): raw SHEET pixels (miniz
    deferred → `osr-sheet-compression`); the retail dhash won't byte-match the port's
    cross-side (native pitch/pixfmt differ → a legit render_diff `[decode]` signal →
    `osr-sheet-dhash-xside`); the alpha (mode-4) source is a GDI/`paint_ctx` blend so its
    `+0x2c` grab is best-effort.
  - **M3d — GDI text. ✓ DONE (ckpt 125).** The engine renders all dynamic text +
    prologue narration through GDI `TextOutA` straight onto the backbuffer DC
    (outside the 5 blits — text-glyph-pipeline.md / quirk #63).  IAT-patched the
    engine's gdi32 imports (`engine_gdi.h`, via `iat_hook.h` — an IAT swap is a full
    wrapper that SEES the return value, so `CreateFontIndirectA`'s new HFONT needs no
    onLeave framework): `TextOutA` → TEXT records, `CreateFontIndirectA` → dedup'd
    FONT records (LOGFONTA), `SelectObject`/`SetTextColor`/`SetBkMode` track per-HDC
    state (font_ref/color/bk_mode).  TEXT shares the per-frame draw seq with BLIT (so
    the replayer interleaves text + blits) and targets the single backbuffer handle
    (from the blit path).  `OSR_TEXT`/`OSR_FONT` are the codec records (`osr_format.h`
    + round-trip tests); `osr.py` decodes them (+`TEXTS` dump).  PROVEN on a fresh
    nav→game_enter capture: **9 FONT** (Courier New h8..20) + **553k TEXT** records
    (font_ref/dst_handle 100% set, 7 distinct colours), all anchors + both seed pins +
    BLIT/SHEET coverage byte-identical to M3c.  The decoded text matches quirk #63
    ground truth EXACTLY — font ref 3 = Courier New 7×18, per-glyph TextOutA at 7px
    advance, the 3-copy shadow (`0xa8b9cc`/`0xa8b9cc`/main `0x3e537d`), bk TRANSPARENT.
    The `.osr` is now a complete frame description.
- **M4 — reconstruct. ✓ DONE (ckpt 125) except the --validate gate (M4d).**
  `opensummoners.exe --osr-replay <osr> --osr-out <dir> [--osr-replay-frames i,j]`
  rebuilds frames 1:1.  M4a: the STREAMING reader `src/osr_replay.{c,h}` (a 1.5 GB
  capture can't be slurped by the 32-bit port → record-by-record dispatch to an
  `osr_replay_sink`; host-tested + validated against the real capture's counts vs
  osr.py).  M4b+M4c: the Win32 reconstructor `src/osr_recon.c` — SHEET→DDraw source
  surface (TOP-DOWN load), FONT→HFONT, BLIT→the zdd.c primitive (colorkey bound RAW,
  not re-converted — the magenta-leak fix), TEXT→real GDI TextOutA, PRESENT→BMP; the
  dest accumulates with NO per-frame clear (empty re-present frames retain prior
  pixels, quirk #99).  M4-alpha: the mode-4 blend descriptor is captured (`OSR_BLEND`
  + `blend_ref`, `tools/capture_proxy/blend_grab.h` — VirtualQuery-guarded reads +
  content dedup, the descriptor is a HEAP object) so alpha replays via
  `zdd_blit_orchestrate` (0 alpha-skipped).  USER-CONFIRMED the town reconstruction.
  - **M4d ✓ DONE (ckpt 127) — the `--validate` gate PASSES 71/71 snaps
    `differ_px==0`.**  `OSR_SNAP` (24-B prefix + raw Lock pixels, streamed like
    SHEET) records the REAL backbuffer at the flip hook — after the closing
    frame's draws, before its PRESENT, so in-stream it sits just before the
    PRESENT and the recon compares its accumulated dest at exactly that point.
    Sampling: `OSS_OSR_SNAP_EVERY=N` + `OSS_OSR_SNAP_FLIPS=a,b,…`; only frames
    WITH draws are snapped (an empty re-present's backbuffer depends on the
    flip-chain rotation, quirk #99).  The recon's `on_snap` locks the dest,
    counts mismatching RGB565 px, dumps `real_/recon_` BMP pairs on failure.
    **The gate immediately earned its keep:** first run = 67/68 clean, the one
    failure (flip 800) was the USER-flagged menu artifact — root cause: the
    un-captured scene-transition CLEAR (quirk #105, retail's `0x5b9410`
    backbuffer zero-fill; the menu draws only its panels over it, and its
    dialog GROW animation left onion-ring borders in an accumulating recon).
    Fixed by `OSR_CLEAR` (proxy INT3 at `0x5b9410`, filtered to the tracked
    compose surface; replayed as an ORDERED draw by recon + osr_view scrub,
    where a clear-only frame now counts non-empty).  Re-capture → **71/71
    clean** (3 more snaps than before: clear-only frames now qualify).
    Remaining capture gap: none known (mode-2 srcw/srch closed in ckpt 126).
- **M5 — port emitter. ✓ DONE (ckpt 128).** `src/osr_emit.{c,h}` emits the same
  `.osr` from the port's own sinks (`--osr-emit <path> [--osr-scenario <name>]`),
  mirroring the proxy hook map 1:1 — FRAMEBEG/PRESENT at drive_present
  (present-then-framebeg), BLIT at the 5 zdd primitives, per-CEL SHEETs via a
  lock-based surface reader (sheet_grab.h hash shape + dtor eviction), CLEAR,
  mode-4 BLEND (exact LUT sizing), GDI TEXT via a per-HDC shadow, FONT at the
  ar_gdi_create_font chokepoint, ANCHOR/SEED.  Primary-dest filtered
  (dst_handle 1, retail's observed shape).  PROVEN: +2 host round-trip tests;
  a 1500-flip intro-1 run reads back 100% named / 100% dhash+dst; and
  --osr-replay of the port's own .osr rebuilds flips 700/900/1250 vs the port's
  live captures at differ_px==0 (the port stream is self-contained).
- **M6 — studio. ✓ DONE (ckpt 129).** The tick-join + the native PORT|RETAIL|DIFF
  three-panel — the first usable replacement, a frame-by-frame 1:1 scrub on the
  sim_tick axis.  Delivered NATIVELY in `osr_view` (NOT the planned Python `:8780`
  server — the web view is abandoned, ckpt 126 USER directive; the native viewer is
  the only target).
  - **M6a — the JOIN + the streaming reader (`tools/trace_studio2/`).**  `osr.py`
    grew `stream_records`/`stream_frames`/`read_header` (block-buffered, skips the
    bulky BLIT/TEXT/SHEET payloads → streams the 1.9 GB `retail.osr` in ~11 s, no
    OOM — retires the survey's `parse()` OOM debt).  `pair.py` joins both sides by
    `sim_tick` (last flip per tick, honest port-only/retail-only gaps), reporting the
    paired count, the gaps, per-shared-anchor RNG assertions, and the flip-axis drift
    contrast; `--write-pairs` → `pairs.json` (reference only).  VERDICT on
    (port-m5, retail-snap): PASS, 190 paired, anchors RNG-aligned.
  - **M6b — `osr_view.exe <port.osr> <retail.osr>`.**  Opens two scrub sessions,
    builds the tick-join natively (`build_join`, same semantics as `pair.py`, NO
    `pairs.json` dependency), shows PORT | RETAIL | DIFF at the joined tick with a
    tick-indexed scrubber + a precomputed diff HEAT RIBBON (per-paired-tick
    `differ_px`, aggregate worst-per-column, click-to-seek, worst/next-diff nav).
    `diff_image` = amplified cross-side diff (faint silhouette where equal,
    yellow→red where divergent).  Single-file mode unchanged.  Makefile links
    `osr_emit.c` (zdd.c hard-refs its M5 taps).
  - **VERIFIED headless (`osr_prof` dumps, the same `osr_scrub` engine):** join
    indices match `pair.py`; sim_tick 0 reconstructs `differ_px==0` (PIXEL-IDENTICAL
    port vs retail — it's the game_enter TRANSITION frame, near-black; the inspector
    showed a late mid-frame CLEAR wiping a composed town, the town renders a few ticks
    in, e.g. tick 97 `differ_px=264`).  **USER-CONFIRMED the GUI** ("studio looks good").
    **CAVEAT:** port-m5 only reaches tick 191 (a matched-length port capture awaits
    the freeroam port); the 190-paired region is the working demo.
- **M7 — drill-in + the note hand-off. ✓ PARTIAL (ckpt 129).** Done: the NOTE/mark
  system (crop+text → `osr_notes.jsonl` → `notes.py` renders the cropped port|retail|
  diff for the agent — the human→agent contract, openrecet N4) AND the DRAW DRILL
  (osr_scrub `frame_draws`/`render_rgba_upto`/`pick_draw`, openrecet N3).  The drill is
  UNIFIED into the main 3 panels (USER's openrecet-UX ask — NOT a separate window): a
  frame/draw-drill MODE toggle + per-panel show checkboxes; in drill mode ONE K slider
  drives `render_rgba_upto` on BOTH sides synchronously so the DIFF panel shows where the
  two draw SEQUENCES diverge; focus radio + click-to-pick + green selected-draw rect; a
  panel DRAG=crop, CLICK=pick (the InvisibleButton fix also cured the crop-drag
  window-move bug, USER-reported).  Gap panels labelled (not bare black).  Engine verified
  headless (`render_rgba_upto(all)==render_rgba`; clean build-up + pick on port frame 1309;
  cross-side up-to-K diff at tick 112 peaks ~1020px then settles to 380).  USER-CONFIRMED
  the scrub + the note round-trip.  **The DRAW-SEQUENCE MATCH is the faithfulness bar
  (USER):** match the port↔retail draw stream, not just the final pixels — tick 112 already
  shows port 616 vs retail 634 draws.  REMAINING (pull-when-needed, openrecet survey 4/5/6):
  an `.osr` slice tool, a content-addressed capture cache + one orchestrator command, the
  draw-program semantic (`render_diff`) panel that scores the draw-sequence divergence —
  then archive v1.

## Resolved design decisions (USER, 2026-06-12)

- **Capture calls + reconstruct on Windows** (not real-pixel capture, not offline
  replay) — the GDI blocker is void because reconstruction runs on Windows.
- **Frame scrub first**, draw-call/data drill-in later — incremental growth into a
  complete v1 replacement.
- **Proxy scope (REVISED at M3c):** inline-detour the engine VAs only — NO COM vtable
  wrap. The VA detours give the engine's `res`/`frame`/state semantics + clock/seed/
  input/anchors AND (M3c finding) the surface identity for free: each cel/dest holds a
  real `IDirectDrawSurface7*` at `+0x2c`, so the blit detour interns those pointers for
  `dst_handle` + Locks the source for the SHEET, with no surface wrapping and no per-blit
  COM overhead. The god-object surface pointer (`0x8a93cc`→`+0x16c`→`+0x2c`) remains the
  fallback for the `--validate` real-snapshot only.
```

## osr_view — the native ImGui scrubber (ckpt 125 — DONE + one OPEN bug)

`tools/osr_view/` is a native Windows PE (mingw-cross): DDraw+GDI reconstruction
(the C `osr_scrub` engine over the shared `recon_apply` core) + a Dear ImGui / DX11
UI in ONE process.  USER-CONFIRMED fast + good.  Perf (profiled, `osr_prof.exe`):
**~1.4 ms/frame (~720 fps), ~0.3 s open** after three fixes (commit `5118724`):
(1) self-contained render (a SotES non-empty frame is a full redraw — clear+its
draws == accumulated, 0.0% diff on frames 700/1250/5000; empty re-present → last
non-empty); (2) **system-memory surfaces** (`recon_apply` + `osr_scrub`) — a
video-mem Lock readback STALLED ~274 ms/frame on a GPU sync; (3) block-buffered
index (was a 16M-record per-record stdio loop = ~50 s).  Build: `make -C
tools/osr_view` (`IMGUI_DIR` from the flake, `pkgs.imgui`).  GDI fallback:
`make gdi`.  Profiler/frame-dumper: `make prof` → `osr_prof.exe <osr> [n]` (perf)
or `osr_prof.exe <osr> dump <frame_index> <out.bmp>`.

### RESOLVED (ckpt 126) — the HOUSE FREEROAM mis-render was a CAPTURE bug
The USER-flagged defects (white panel "holes" + stray Arche-head fragments on the
house freeroam frame, flip 6390 / tick 5163) were **NOT recon geometry** — the
`.osr` itself referenced the WRONG SHEETS.  Root cause: `sheet_grab.h` cached
ptr→dhash **forever** ("once-per-surface"), but the engine **destroys + reallocates
sheet surfaces at a room swap** (zdd_object dtor `0x5b9390` Releases `+0xac` then
`+0x2c`; eviction bursts observed at flips 652 / 2495 / 2926 / 3065).  A house cel
allocated at a recycled pointer hit the stale cache entry, so its BLITs recorded
the dhash of a town-era surface: the "holes" were a **640×480 100%-WHITE dialog-
panel sheet** (captured at flip 96), the Arche heads were her old bank, plus
extent-overflow garbage (a 128×160 cel reading a 20×20 old sheet).  Diagnosed
offline by a streaming scan cross-checking each blit's dhash against its SHEET
record: **79/509 blits flagged** on the stale capture.

Fix (capture-side, two parts):
1. **dtor eviction** — the proxy INT3-hooks `0x5b9390` and evicts `+0x2c`/`+0xac`
   from BOTH caches (`sheet_grab_evict` / `surfid_evict`, tombstoned open
   addressing); re-grabs of identical content skip re-EMISSION via an
   emitted-dhash set.  A recycled dest pointer also interns a FRESH dst_handle.
2. **mode-2 RECTS src extent closed** (the suspected gap, fixed while the format
   was open): `osr_blit` grew `srcw/srch` (args 7/8 of the 8-coord call; BLIT
   payload 80→88 B), the reconstructor passes them to `blt_rects` (src≠dst extent
   = a scaling Blt).  **Legacy 80-byte captures still decode** (srcw/srch
   zero-fill → the old dest-extent fallback); osr.py mirrors both sizes.

Verified: recaptured the same nav (anchors byte-identical: newgame@652,
game_enter@1242; ~950 fps — the dtor INT3 costs nothing measurable); flip 6390
reconstructs clean (USER-confirmed) — 79→7 flagged blits, all 7 benign
first-referencer res-0 labels (dims match).  Prologue 1200 + town 1250
`differ_px==0` vs the stale capture's reconstruction (the fix perturbed nothing
already-correct).  Also: `run_proxy.sh` now skips `osr.py SUMMARY` above 256 MB —
its non-streaming parse() OOMs on GB captures (it SIGKILLed a tmux session).

~~Still open: M4d `--validate`; the menu CLIPPED artifact; scene-transition
CLEARs.~~ **All three closed in ckpt 127** — the gate landed, passed 71/71, and
its single failure WAS the menu artifact WAS the missing CLEAR (quirk #105).
The planned **draw-inspector panel** + render-up-to-draw-N remain the self-serve
debug path for the next visual defect.  Residual staleness risk (accepted, and
now actively policed by --validate snaps): a surface RE-COMPOSED in place via
Lock/GDI (not through the blit primitives) keeps its first-grab pixels.

## The openrecet v3 survey — the UX/feature roadmap for osr_view (ckpt 126)

Surveyed `/opt/src/openrecet/tools/trace_studio_v3/` (their NATIVE viewer arc, one
generation ahead of ours: plan `openrecet/docs/plans/trace-studio-v3.md`).  Their
pipeline: d3d8 proxy capture (BOTH sides load it) → flat dedup'd container →
content-addressed cache + slice-serve → RESIDENT replay core (open once, render
any frame ~1.4-3 ms) → ImGui/D3D9 three-panel viewer.  We independently converged
on several pieces (native proxy, flat stream, resident recon in the viewer,
~1.4 ms/frame, headless frame dump); the gaps below are the roadmap, ordered by
leverage.  Engine-API specifics (their D3D8 state machine vs our DDraw blits +
GDI text) do NOT transfer; the architecture does.

1. **Draw-level inspection in osr_view (their N3 — the self-serve debug loop).**
   `render_upto(frame, K)` (first K draws), `render_range` (SOLO one draw over a
   clear), a draw-list panel, and **pixel→draw pick** (click → linear-scan the
   prefixes for the draw that last changed that pixel; at our ~1.4 ms/frame a
   pick costs ~150-300 ms).  Our recon already replays a frame's record list —
   the draw cap is a loop bound, NOT new architecture.  This is what makes the
   NEXT visual defect self-diagnosing (this checkpoint's bug took an offline
   capture-scan instead).  Ref: `viewer/viewer.cpp` (draw-step + solo + pick),
   `replay/replay_core.c` (`render_upto`/`render_range`).
2. **Identity JOIN for the M5/M6 port|retail pairing (their E3 — kills the v2
   sync-bug class).**  Pair frames by a STORED key — for them `(anchor,
   occurrence, present-offset)`; for us **sim_tick is already on every FRAMEBEG**
   (a stronger key than their present-count).  Compute the pairing ONCE
   (`pairs.json`-equivalent), render HONEST GAPS (port-only / retail-only spans)
   instead of silently mispairing across load stretches.  Ref: `orv3_sync.py`.
3. **Three-panel port|retail|diff + a diff ribbon (their N2).**  After M5 (the
   port .osr emitter): per-pair pixel metrics (gt8 / meanabs / maxd) precomputed
   at open → a clickable heat ribbon (seek-to-worst-frame).  Our single-panel
   scrubber grows panels, not a rewrite.  Ref: `viewer/viewer.cpp` (live CPU
   diff), `orv3_view.py` (precomputed per-frame metadata).
4. **An .osr SLICE tool (their slice-serve).**  Re-emit a sub-window as a
   STANDALONE container, pulling forward the SHEET/FONT/BLEND records it
   references — a 50-frame repro becomes ~tens of MB instead of the 1.9 GB
   session, and the viewer opens it instantly.  Capture the full extent ONCE,
   slice forever (zero re-drive).  Ref: `orv3.py slice_window()`, `orv3_slice.py`.
5. **A content-addressed capture cache + ONE orchestrator command (their
   orv3_window.py).**  `<scenario> --window X:Y --launch`: reuse the cached
   full-extent capture (keyed on trace-bytes + arm spec; port side staleness =
   exe mtime), slice the window, drive only the MISSING side, join, launch the
   viewer.  Collapses the capture→recon→view loop to one command.  Ref:
   `v3cache.py`, `orv3_window.py`.
6. **A draw-program semantic panel (their material_diff).**  Cross-side per-draw
   verdicts (ALIGNED / BATCHING / DIVERGENT) over the (res,frame)+dhash identity
   — our render_diff.py already computes this offline; fold it into the viewer's
   draw panel once the JOIN exists.  Their HOUSE lesson: identical pixels can
   come from DIFFERENT render programs — pixel diff alone can't see it.  Ref:
   `orv3_draws.py`.
7. **Marks/worklist in the native viewer (their N4 / our v1-parity tail).**  The
   :8779 serve flow's USER-marks → worklist.md remains the hand-off contract;
   port it into osr_view last, once the diff panes exist.

NOT copying: their deferred-snapshot kept-window capture (we capture every frame
at ~950 fps and the full session is the point for scrub); their content-hash-only
resource identity (our (res,frame) registry + dhash + the ckpt-126 dtor eviction
already cover surface reuse); their web view — **ABANDONED upstream (USER): the
NATIVE viewer is the only target**, the web/PNG-bake path reintroduced the
stale-intermediate pain and is not part of this roadmap in any form.
Tooling debt their survey re-confirmed: `osr.py parse()` must grow a STREAMING
iterator (the OOM band-aid in run_proxy.sh caps SUMMARY at 256 MB).
