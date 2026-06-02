# Parity ledger — frames/scenes confirmed pixel-1:1 vs retail

A running list of port renders that have been **visually diffed against the
retail golden and confirmed identical** (bit-identical, or identical modulo a
named, understood delta). This is a **regression guard**: once something is on
this list, later work must not silently break it. Re-verify entries with
`--capture-frames` + `tools/push_comparison.py` (port|retail amplified diff)
after changes that touch the relevant render path.

> How an entry earns its place: capture the port frame (`--capture-frames`),
> diff against the retail golden in `runs/<scenario>/frames/` via
> `tools/push_comparison.py --port … --retail …`, eyeball the amplified diff,
> and record the `differ_px` / `mean|abs|` numbers + any understood residual
> delta. "1:1" means the diff is empty OR fully explained by a listed,
> intended-to-be-fixed gap.

## Method / reference

- Port capture: `opensummoners(-debug).exe --capture-frames "N,…"` →
  `<gamedir>/port_frame_NNNNN.bmp` (Flip-indexed). Convert BMP→PNG (PIL).
- Retail goldens: `runs/title-idle/frames/frame_*.png` etc. (also Flip-indexed,
  captured by the Frida harness — see `docs/parity-harness.md`).
- Diff + push: `tools/push_comparison.py --port P.png --retail R.png …`.
- NB the port and retail **Flip indices differ by a constant render-rate
  factor** (~2.2×), NOT a timeline rush — see R3 below (ckpt 29): the port's
  *wall-clock* pacing matches retail (~9.2 s to the menu), but retail renders
  ~127 flips/s (each scene state duplicated ~2.2×) while the port renders one
  flip per scene state. So compare a port Flip against the retail Flip showing
  the *same scene state* (phase/fade/menu_fade), not the same index. Record both.

> **The bar is `differ_px == 0`.** A frame is only "confirmed 1:1" when the
> amplified diff is empty. Any residual — however small — is an OPEN item below
> with concrete hypotheses, never hand-waved as "close enough" ([[feedback_bit_exact]]).

## Confirmed bit-exact (differ_px == 0)

| # | scene / frame | port cap | retail golden | verified |
|---|---------------|----------|---------------|----------|
| 1 | Title menu (idle, settled) — phase-matched | port Flip 209 `menu_fade=750` | `cursor-match/frame_01300` `local_58=750` | 2026-06-02 ckpt 28 — `differ_px=0` |
| 1b| Title menu (idle, settled) — phase-matched | port Flip 203 `menu_fade=450` | `cursor-match/frame_01420` & `_01460` `local_58=450` | 2026-06-02 ckpt 28 — `differ_px=0` |
| 2 | Studio logo (phase 0, fade-in) — fade-matched | port Flip 30 `phase=0 fade=640` | `fade-match/…/frame_00065` (fade-probe: fade=640) | 2026-06-02 ckpt 30 — `differ_px=0` |
| 3 | Title-art logo (phase 3, fade-in) — fade-matched | port Flip 235 `phase=3 fade=820` | `fade-match/…/frame_00480` (fade-probe: fade=820) | 2026-06-02 ckpt 30 — `differ_px=0` |

> **R1 CLOSED (ckpt 28).** The residual was the **cursor pulse**. Retail
> animates the cursor `level_num` (`[esp+0x20]`) as a triangle wave — `local_58`
> in `FUN_0056aea0`, driven by phases 8↔9: phase 8 ramps `+50/update` to 1000,
> phase 9 ramps `-50/update` to 0 (`docs/decompiled/by-address/56aea0.c`
> :366-384). `level_div` is the constant `0x4b0` (1200), so `idx=(local_58*20)
> /1200` peaks at **16** (NOT 19) and breathes to 0 (invisible). The port had
> wired the cursor to a static idx-19 full-add, so it was uniformly **over-
> bright** (every differing pixel had port>retail). Method that nailed it (NOT
> eyeballing — RE'd the code + measured retail):
> 1. `frida_capture.py --cursor-probe` hooks `FUN_0056c470`, logs per-frame
>    `level_num`/`level_div` → showed the 0→1000→0 step-50 triangle + the call
>    site `0x56be79`.
> 2. Read `FUN_0056aea0`'s phase FSM → `local_58` is the source; the port
>    already computed it as `title_fade_state.menu_fade` but never passed it.
> 3. Wired `menu_fade`→cursor `level_num` (`title_render_menu`/`title_sink`),
>    captured port frames (which log `menu_fade`), matched to retail goldens
>    captured WITH `--cursor-probe` (so each golden's `local_58` is known), and
>    diffed at equal pulse phase → `differ_px=0`.
>
> The port Flip index ≠ retail Flip index (a render-rate factor, R3 below — NOT
> a timeline rush). But at equal pulse phase the frames are pixel-identical,
> which proves the render path is 100% correct.

## Open residuals — under investigation (NOT yet 1:1)

_(R1 moved to Confirmed bit-exact above.)_

### R3 — intro pacing: RESOLVED as a render-rate artifact + frame-drop bug (ckpt 29)

The "port rushes the intro" framing was **wrong**. Measured both sides
(`frida_capture.py --pace-probe`/`--cursor-probe` on retail; `pace:` phase log
in `src/main.c` on the port, both with the real clock):

| | menu (phase 8) onset | wall-clock to menu | render rate |
|---|---|---|---|
| **Retail** | Flip 1172 (first cursor draw, fade saturated) | **9.23 s** | ~127 flips/s |
| **Port (pre-fix)** | Flip 90 | 9.87 s | ~9 flips/s |
| **Port (ckpt 29 fix)** | Flip ~528 (phase-8 entry) / ~578 (cursor) | **~9.2 s** | ~60 flips/s |

So the **wall-clock pacing already matched** (~9.2 s both). What differed:
retail renders at its display refresh (~127 Hz here) and **duplicates each
scene state ~2.2×** (cursor probe: each `menu_fade` value spans ~2 consecutive
flips); the port's pace machine (`title_pace_step`) was being driven **one
pace-step per 16 ms-throttled main-loop iteration**, which made the fixed-
timestep accumulator's budget refill (`b += now − anchor`) run away to ~6
updates per render — so the port **DROPPED ~5/6 of the intro's fade frames**
(rendered 90 of ~528 update ticks) and the fades were choppy.

**Fix (ckpt 29, `src/main.c`):** drive the pace machine like retail's tight
outer loop — spin pace-steps (updates are ~free) until one *present*, then
`frame_limiter` gates the presented-frame rate. Result: **1 update per render,
every fade value rendered** (phase curve now the canonical 51/102/153/254/275/
316/437/528, MISSED=0 menu_fade values), wall-clock matches retail.

**Flip-index-exact** matching to a specific golden is the capture rig's refresh
(~127 Hz) and is **not portably reproducible**; the meaningful, achievable
target is the *distinct-content sequence* (every scene state rendered in order),
which now matches. R1 re-verified post-fix at `menu_fade=750`: **differ_px=0**.

### R4 — phase-7 subtitle sparkle: reveal sweep BIT-EXACT; only the deferred particle overlay remains (ckpt 30)

The phase-7 flourish has **two independent parts**:
1. **Render-half subtitle-reveal sweep** (`TITLE_DRAW_SPARKLE`, wired ckpt 30):
   `FUN_0056bcf7` copies 4×48 vertical slivers of the menu-bg sprite (MAIN
   frame 5) at src (x,416)→dst (x,416), x stepping 192..<416 by 4, alpha from
   `ramp_b` (opaque once `min(7·fade−100·i,1000)` saturates). This reveals the
   "Secret of the Elemental Stone" subtitle column-by-column. **Verified
   bit-exact**: at fade 1000 (full reveal) the SUMMONERS logo + subtitle banner
   + art match the retail golden exactly outside the particle region.
2. **Update-half particle spawn** (`FUN_0056c070`, STILL DEFERRED) — additive
   white sparkle twinkles scattered over the lower art. This is a separate
   subsystem the port stubs (see HANDOFF "Open RE threads" / `title_scene_hooks`).

The residual at fade 1000 (port Flip 500 vs `sparkle-match/…/frame_01000`):
**1208 px, 96.6% retail-brighter** (white dots) — i.e. *entirely* retail's
missing particle twinkles, not a reveal-sweep error. Closing R4 fully needs the
`0x56c070` particle system ported.

> **Fade-probe caveat (ckpt 30):** `frida_capture.py --fade-probe` (hooks
> `FUN_00448c80`, logs the first `(value,div)` per Flip) reads the intro fade
> directly in **phases 0..4** (logo), but in **phase 7** the first call is the
> first *sparkle*, so it logs `min(7·fade,1000)` — NOT the raw fade. Match
> phase-7 frames by reveal extent / the saturated full-reveal state, not the
> probe value. (Phases 5–6 don't call `0x448c80` at all — the gap in
> `fade_level.jsonl` pinpoints them.)

When a residual reaches `differ_px == 0`, move it to "Confirmed bit-exact".
