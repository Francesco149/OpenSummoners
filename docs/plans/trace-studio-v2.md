# Trace Studio v2 — the native-capture, tick-joined parity studio

> Status (2026-06-12): M1+M2+M3a+M3b LANDED. M1+M2: proxy auto-loads, native headless
> turbo boot, the INT3+VEH engine-VA detour layer, ring input injection →
> seed-pinned lockstep boot to game_enter with all anchors. M3a: the `.osr` format
> (`src/osr_format.h`) + the bg-thread ring writer (`osr_writer.h`) + the cheap
> records (FRAMEBEG/PRESENT/ANCHOR/SEED) + `osr.py`. M3b: the BLIT op stream — the
> 5 blit detours + the cel→(res,frame) registry (`render_id.h`, onEnter-computed) →
> OSR_BLIT records. PROVEN on a real boot to game_enter (867k blits, 89% named, all
> anchors). PERF: title ~2400 fps but in-game town ~25 fps (INT3+VEH 2-exceptions/
> blit) → the perf chip (RWX-pages or the E9 trampoline) is the fork resolution.
> M3c (COM wrap + SHEET) is NEXT (or the perf chip first — USER call).
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

## Build / run (provisional)

```sh
# build the proxy DLL (same toolchain as the port):
nix develop --command make -C tools/capture_proxy      # → build/ddraw_proxy.dll

# capture a scenario on both sides into a v2 session (NO Frida):
nix develop --command python3 -m tools.trace_studio2 capture <scenario> --session NAME

# serve the viewer (:8780, isolated from v1's :8779):
nix develop --command python3 -m tools.trace_studio2 serve --session NAME
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
    **PERF FORK RESOLVED (measured):** the INT3+VEH framework handles the volume but at
    ~25 fps in-game (~2 exceptions/blit + `detour_patch_byte`'s `VirtualProtect`/
    `FlushInstructionCache` per patch dominate at ~1500 blits/frame); title ~2400 fps.
    Usable+cached but below turbo → **M3b-perf** chip: (i) leave hooked pages permanently
    RWX + skip the per-patch protect/flush (cheap, ~4 syscalls/blit saved, keeps the 2
    exceptions); (ii) the hand-rolled 5-byte E9-jmp trampoline (removes the exceptions —
    the real turbo fix). Either before or after M3c (USER call).
  - **M3c — COM wrap + SHEET (the risky piece, isolated).** Wrap the DDraw7 + Surface7
    vtables for surface identity (stable dst handles) + the one-time dedup'd source-pixel
    grab → SHEET records (dhash via the render_id FNV-1a, miniz-compressed). Backfills the
    BLIT `dhash`/`dst_handle` and corrects the header pixfmt/screen from `DDSURFACEDESC2`.
  - **M3d — GDI text.** Hook `gdi32!TextOutA`/`ExtTextOutA` + `SelectObject`/
    `CreateFontIndirectA` → TEXT/FONT records.
- **M4 — reconstruct.** `opensummoners.exe --osr-replay` rebuilds frames 1:1; the
  `--validate` fidelity gate (`differ_px==0` vs a real snapshot) passes.
- **M5 — port emitter.** `src/osr_emit.c` emits the same `.osr` from the port.
- **M6 — studio.** tick-join pairing + `:8780` viewer with the v1 scrub UX over the
  reconstructed frames. **This is the first usable replacement — frame-by-frame 1:1
  scrub.**
- **M7+ — drill-in.** Draw-inspector panel, in-frame draw stepping, draw-level
  `render_diff`, marks/worklist — grow v2 to full v1 parity, then archive v1.

## Resolved design decisions (USER, 2026-06-12)

- **Capture calls + reconstruct on Windows** (not real-pixel capture, not offline
  replay) — the GDI blocker is void because reconstruction runs on Windows.
- **Frame scrub first**, draw-call/data drill-in later — incremental growth into a
  complete v1 replacement.
- **Proxy scope:** wrap DDraw COM *and* inline-detour the engine VAs (the COM wrap
  gives surface identity for the draw stream + one-time source grab; the VA detours
  give the engine's `res`/`frame`/state semantics + clock/seed/input/anchors). The
  god-object surface pointer (`0x8a93cc`→`+0x16c`→`+0x2c`) is the fallback for the
  `--validate` real-snapshot only.
```
