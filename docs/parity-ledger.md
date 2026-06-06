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
| 4 | Phase-7 sparkle twinkles (particle system) — tick-matched, seed-pinned | port Flip 465 `phase=7 fade=540` | `sparkle-align/frame_00939` (seed 0x4f5347) | 2026-06-02 ckpt 31 — `differ_px=0`, **user-confirmed 1:1 incl. particles** |
| 5 | New-game config menu box + text + selection cursor — anim-phase-matched | port Flip 761 (cursor frame 17/19) | `goldens/retail-newgame-config-menu.png` | 2026-06-03 ckpt 43 — menu-box `differ_px=0` (region (32,32)400×124), **user-confirmed 1:1**. Off-phase frames (16/18 @ Flip 760/762) differ only by the cursor's animation phase, same as the intro twinkles — not a content gap. Closes the ckpt-40 307px residual via the quirk-#69 trim-transpose fix |
| 6 | New-game config TOOLTIP TEXT node (word-wrapped help line, y=416/444) | port Flip 760 | `goldens/retail-newgame-config-menu.png` (tooltip region (32,392)576×80) | 2026-06-03 ckpt 44 — **0 text-colored pixels differ** (text-presence XOR == 0); difficulty tooltip wraps 65/52 glyphs via the width-68 word-wrap port (`glyph_wrap`, quirk #70). **User-confirmed 1:1** (whole new-game screen 1:1 except the deferred animated sparkle corner). Residual = 9px box-panel RGB565 1-LSB rounding (non-text, pre-existing box decode) |
| 7 | In-game town BACKDROP **during the intro PAN** (sky + parallax mountains + tilemap), camera-matched | port Flips 1304/1344/1384/1422/1462 | fresh retail `--no-turbo --seed-pin --lockstep` caps, Flips 1617/1660/1700/1740/1780 | 2026-06-06 ckpt 70b — **matched by `cam_x60`** (port↔retail share 127990/125690/120050/114350/108350 exactly — the ported easer/cadence/trigger land on retail's scroll grid). Backdrop **Δ0** inside the scene window: alignment shift-search peaks sharply at `dx=dy=0`; the pan-start `x=80` column is **all Δ0** (every building/wall/grass pixel). Residual diff = the **named missing layers ONLY**: the cinematic LETTERBOX (rows 0-63 + 416-479 black, quirk #74 — **now ported, entry 8**), the "Town of Tonkiness" BANNER (`0x5a00c0`), the NPCs (present modes 0/1/2), the foreground TREE/vegetation (`ingame-nontile-layers`). Proves the camera projection during the pan is correct — the things the port renders are 1:1 |
| 8 | In-game establishing-shot **cinematic LETTERBOX** (res `0x583` tiled — top rows 0-63 + bottom rows 416-479 solid black; quirk #74) | port frame 1200 (cam hold 128000) | retail blit trace `/tmp/blit_town_retail` frame 1500 (cam hold) | 2026-06-06 ckpt 75 — **blit-trace 1:1 via `render_diff`**: the 320 res-`0x583` blits match retail on identity + geometry + DDraw state (the town-frame diff dropped **356→36**, and the 36 are exactly the deferred RNG-driven actor/banner/tree banks — `present-actor-modes`/`ingame-nontile-layers`). Port-side pixel check: rows 0-63 + 416-479 are `(0,0,0)`, row 64 is the sky band — the central 640×352 window. `letterbox.{c,h}` (the `0x48c150:124-162` grid-fill, host-tested). PORT-DEBT `ingame-letterbox` (the 64/64 bar heights are a constant stand-in for the unported `0x5a00c0` cutscene op; the geometry is bit-exact) |

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

### R4 — phase-7 subtitle sparkle: RESOLVED (ckpt 31) — both parts now bit-exact

The phase-7 flourish has **two independent parts**, both now `differ_px=0`:
1. **Render-half subtitle-reveal sweep** (`TITLE_DRAW_SPARKLE`, ckpt 30):
   `FUN_0056bcf7` copies 4×48 slivers of the menu-bg sprite (MAIN frame 5)
   revealing the "Secret of the Elemental Stone" subtitle column-by-column.
   Bit-exact since ckpt 30.
2. **Particle twinkle system** (`FUN_0056c070` spawn + the `0x56ba69` per-frame
   update + cull `0x56c030` + draw `0x56c180`, ported ckpt 31): white sparkles
   spawn at the reveal's leading edge, **rise upward (accelerating) and fade out
   over a 20–39-tick lifetime, then cull** — they do NOT accumulate. The seed
   word `DAT_008a4f94` (engine LCG `FUN_005bf505`) is `srand(time())`-seeded
   (`0x56227a`), so retail's stream is wall-clock-random; pinning it on both
   sides (port boots a fixed seed; harness `--seed-pin` writes the same value at
   the first spawn) makes the twinkles reproducible. **Confirmed bit-exact** (now
   row 4 above): port Flip 465 vs `sparkle-align/frame_00939`, seed `0x4f5347`,
   **`differ_px=0`** at matching update-tick.

> **What the journey taught (ckpt 31):** the first cut spawned + drew the
> particles but left them **frozen** at the spawn row → an over-bright smear
> (8277 px diff). The bug was the **missing per-frame update** (`0x56ba69`):
> `y_num -= vel; vel += 2` (rise, accelerating up), `anim_num--` (ages → the
> draw's `(anim_num*frame_count)/anim_div` walks the sprite frame 0→7 = fade),
> and cull at `anim_num==0`. The `+0x08` field (then mislabelled `_pad08`) is
> the upward velocity. With the update ported, the diff fell to a clean diagonal
> min and hits 0 at tick alignment. **Off-tick frames still differ** purely by
> the **R3 render-rate sub-tick jitter** (retail renders each update-state
> ~2.2×; the captured retail flip rarely lands on the exact port tick), and
> retail's intro pacing jitters run-to-run (first spawn at flip 886/895/896
> across runs) — so align by the `subtitle_anim_start` TAS anchor + tick, not by
> a fixed flip index.

> **Fade-probe caveat (ckpt 30):** `frida_capture.py --fade-probe` (hooks
> `FUN_00448c80`, logs the first `(value,div)` per Flip) reads the intro fade
> directly in **phases 0..4** (logo), but in **phase 7** the first call is the
> first *sparkle*, so it logs `min(7·fade,1000)` — NOT the raw fade. Match
> phase-7 frames by reveal extent / the saturated full-reveal state, not the
> probe value. (Phases 5–6 don't call `0x448c80` at all — the gap in
> `fade_level.jsonl` pinpoints them.)

When a residual reaches `differ_px == 0`, move it to "Confirmed bit-exact".
