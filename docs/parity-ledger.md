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
| 9 | In-game town **STRUCTURE band** — the foreground **TREE** (res `0x481`) + the fg **hedges/flowerbed** (res `0x426`, layer 15) + bg **decorations** incl. the trellis + fences (res `0x403`) | port frame 1200 (cam hold 128000) | retail field-spec blit trace `/tmp/cast_census` frame 1500 (cam hold) | 2026-06-07 ckpt 83 — **every visible structure cel matches retail on identity + frame + dst position**: tree `(0x481,f0)` @ (496,64) 320×320; the 5 hedges `(0x426,f{0,1,4,5,5})` @ (556,344)/(596,352)/(464,344)/(416,348)/(508,344); the 4 `0x403` props/deco @ (-32,320)/(8,272)/(92,272)/(480,288) — all identical port↔retail (positions cross-checked directly; `render_diff` shows **zero `[rect]`/`[decode]`/`[state]`**). **USER-confirmed 1:1** on the feed ("the decorations are there and positioned 1:1"). Map-driven (`actor_spawn_struct_from_map`, quirk #84): pos = map×100, frame_base = map variant@+0x18, layer from +0x30, bank from the `0x438a60` def table; rendered by `actor_render_static` (the `0x493230` static blit is bit-identical). Residual = the EFFECT townsfolk (`0x493ba0`) + the off-screen `0x4962a0` invisibles (y=572, draw nothing) — next chip |
| 10 | In-game town **EFFECT townsfolk FACING/orientation** — the 7 mirrored villagers (facing 3: `c3be/c3dd/c3e6/c422/c42c/c441/c468`) now face the correct direction | port frame 1200 (cam hold 128000) | live `0x493ba0` `rs_facing` census + a `DAT_008a8440` one-shot read, flips 1450-1600 | 2026-06-07 ckpt 85 — **USER-confirmed on the feed: "npc orientation matches retail yes."** Facing is the deterministic map field `puVar1[4]` (dispatcher `0x58d460:96` → `cVar12 = (puVar1[4]!=0)?3:1` → param_8 → render-state `+0x2c`), NOT RNG; `0x44d160` mirrors the cel (`frame += flip`) + reflects `off_x` at facing==3, `flip = *(s16)(DAT_008a8440[bank])` = the sprite group's frames-per-direction (4/16, read live). Ported via `TOWN_EFFECT_DEFS` facing+flip + `actor_spawn_effect_fill_flip_table` (quirk #85). **Named residual: the idle ANIMATION PHASE** — the townsfolk are still frozen (clip NULL); retail runs the idle clip `0x6290e0` at an RNG start frame (`0x426ec0`), so full per-frame 1:1 awaits the game_enter RNG anchor (idle phase + fountain) |
| 11 | In-game **area-title BANNER** — the "Town of Tonkiness" card (scroll sprite res `0x449` + vines + the area name in Courier New h20 w10, white fill + `0x404040` outline shadow) | port frame 1300 (banner up, cam hold 128000, game_enter+184) | retail `--seed-pin --lockstep --no-turbo` cap `runs/banner-verify/frame_01614` (banner up, cam hold; game_enter+~180) | 2026-06-09 ckpt 101 — **`differ_px=0/36720`** over the whole banner region (scroll + vines + GDI text + sky behind), camera-matched (`cam_x60=128000` both); **USER-confirmed on the feed ("banner looks good").** Producer = `0x494a60` mode 1 (NOT the `0x5a00c0` overlay player — corrects entries #7's attribution); animation = the `0x499ab0` phase machine (fade-in `alpha+=0x14`/sim-tick → hold 400 → fade-out); GDI text composed onto the scroll cel via `zdd_object_get_dc`+`glyph`/`TextOutA` (the live LOGFONT).  `src/banner.{c,h}` (host-tested) + `main.c` wiring; engine-quirk #96.  **Key fix:** the scroll sheet (slot 53/res `0x449`) is decoded UNGRADED — retail binds it via the plain getter `0x418470(0)` (no `0x417c40` grade), so skipping the in-game palette grade for it made the parchment bit-exact (a graded decode rendered it ~10% too dark).  **Named residuals (NOT in the bit-exact window):** the arm trigger + text are measured constants (`banner-trigger`), and only the font-6 length band is ported (`banner-font-table`) |
| 12 | In-game **DIALOGUE BUBBLE, town line 1** — the whole widget: 9-slice bubble (res `0x456`) **incl. the POP-IN scale animation**, speaker-anchored tail pair (frames 9+10), name tab (res `0x44a` f0), "Arche's Father" name (white + `0x455f7b` shadow), the **portrait bust cross-FADE** (res `0x7ef`, ramp_b), and the **TYPEWRITER body text** (Courier New 7×18, `0x3e537d`+`0xa8b9cc`, 5 updates/char incl. the comma 3i pause + space fast-reveal) | trace-studio `intro-1` port frames 2400-2598 (pop @port flip ~2416 = game_enter+1300) | the SAME session's retail capture (pop @retail flip 2737), anchor-segmented pairing, drift −3..−8 | 2026-06-10 ckpt 104 — **the box region pairs `differ_px=0` on 22 of 25 sampled frames across pop-in → fade → typing**; the 3 non-zero frames are 55-62 px = ONE GLYPH at a reveal boundary (the port and the paired retail frame are ±1 tick apart on a coalesced flip pair — the phase pillar / R5 coalescing, self-correcting next frame) — a named, understood delta.  Full-frame residual in the window = the pre-existing town items (fountain/NPC-anim/butterflies, intro-1 marks).  `src/dialogue.{c,h}` (host-tested ×7) + `main.c` wiring; the model from `0x439690` (builder/pump) + `0x48c820` (walk + scale mode) + `0x48cf80` (9-slice) + `0x48da70` (3-pass typewriter text) + `0x49c640` (position/tail) + `0x49c910` (portrait fade — the new cel snaps OPAQUE at fade 500) + `0x410560` (arrow config).  **Key facts:** the UI sheets decode UNGRADED (plain-getter family, like the banner) and the portrait is a 24bpp BMP stored on the 16bpp surface (565 round trip proven exact).  **Named residuals:** `dialogue-trigger` (+1298 measured arm), `dialogue-line-table` (line 1 only, no Z yet), `dialogue-arrow-art` (bank module unresolved; hidden during typing so out-of-window), `dialogue-pause-grades` (fitted), `dialogue-textwrap` (subset) |
| 13 | In-game town **per-tick WORLD at TICK-EQUAL frames** — NPC idle animation (the breathing townsfolk), the **camera PAN across its whole sweep**, the **area-banner fade-in AND fade-out edges**, and the **dialogue pop-in/portrait-fade window**, each compared at FORCED sim-tick equality (`frame_<flip>_t<tick>` both sides, ckpt-105 instrumentation) | trace-studio `intro-1` port (3rd recapture, tick-calibrated triggers) | the SAME session's retail capture, tick-indexed | 2026-06-10 ckpt 105 — **NPC box differ_px=0** at every sampled tick (28-34); **banner box differ_px=0** at fade-in (t60) AND fade-out (t510/525 — closes the intro-1 @2159 mark: it was the 2-tick trigger offset, NOT sampling noise) with the per-present luma sequence matching tick-for-tick (onset t42, drop t493 both sides); **dialogue bubble box differ_px=0** at t650/t660 (pop + portrait fade); **pan full-frame residual 2441/1754 px (t150/t250) = ONLY the named open ensembles** (fountain spray x≈574-630 mid-pan; frozen butterflies x≈85 + chimney smoke at t250 — `butterfly-wander`/`fountain-anchor` debts).  The triggers were recalibrated onto the tick axis (banner 78→**82**, pan 184→**182**, dialogue 1298→**1282**; quirk #99) — the old flip-axis constants carried ±1-8 tick errors absorbed by retail's present-coalescing.  **Method note:** fade dt-probes plateau on the alpha-ramp quantization (~2.5 ticks/index) — the banner's 2-tick error read as 1 until the per-present VALUE sequence was compared |

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

### R5 — town-intro cutscene PAN smoothness: RESOLVED as the presentation-cadence
### (phase) pillar — pan LOGIC measured identical; retail occasionally coalesces 2
### ticks/present (ckpt 103, measured on studio session intro-1)

**Observation (USER, live viewing, 2026-06-09):** retail's arrival pan "consistently
has spikes and is slightly less smooth" than the port's.

**Measurement (phase correlation per consecutive paired frame over the whole pan
window, ordinals 1115–1619 of `intro-1`, lockstep + seed-pinned):**
- **The pan MODEL is identical.** Step histogram port `{−1×11, −2×9, −3×135}` vs
  retail `{−1×11, −2×9, −3×129, −6×3}` — same ease curve, same step timing (every
  2 flips = the sim-tick gate), and the SAME total displacement **434 px** on both
  sides. The first ~30 moving frames match step-for-step at the same ordinals.
- **The residual is pure cadence:** retail, even under the lockstep virtual clock,
  occasionally presents one frame carrying TWO camera steps (−6 between
  *consecutive* retail flips — drift-verified, not a pairing artifact) flanked by
  0-step holds, 3× in this pan. The port (fixed-timestep, 1 update/present, every
  tick rendered) never does. Under the LIVE real clock this same quantization is
  pervasive (display-refresh update banking) — that is exactly the USER-visible
  spikiness. Conversely the port's own duplicate-frame wrinkle shows as (−3,0) vs
  (0,−3) transpositions, absorbed by the studio's sticky matcher.

**Verdict: phase pillar (presentation cadence), zero logic divergence.** On the
sim-tick axis the two pans are identical; `--lockstep` already normalizes retail to
near-port smoothness (3 hiccups/pan vs pervasive live spikes). Per the USER's
"replicate or normalize" framing: **normalized in traces (done — that's the studio
context); replicating retail's flip-level quantization in the port** would mean
porting the real frame-pump's update banking (`0x439690` GetTickCount pacing — the
beat-runner chip already planned for the dialogue cutscene). Decide then whether the
TAS clock should preserve or smooth the quantization; for now the port's
every-tick rendering is the better behavior and the studio pairs through the
difference. (The chase also hardened the matcher: drift hysteresis — move off the
sticky offset only on a ≥5% better match.)

> **ckpt-105 addendum:** the pan now also matches at FORCED tick-equality across
> its whole sweep (the cmd trigger recalibrated to tick 92, `GAME_CAMERA_HOLD_
> FRAMES=182`; the flip-axis 184 was 1 tick late) — full-frame residual at
> tick-equal = the fountain/smoke/butterfly ensembles only (ledger #13).

### R6 — establishing-REVEAL frontier: RESOLVED (ckpt 106) — two confounded
### causes (graded mask cels + the ckpt-105b fence); band differ_px==0 at every
### stamp-equal tick

**Resolution.**  The grid model was always bit-exact — a live per-row grid dump
(the `0x499ab0` `r40..r80` chain fields, `runs/r6-grid` + the port mirror) shows
retail's per-cell `(state, timer)` identical to the port's at every pre-step
boundary over the whole reveal (41 rows × 31 ticks, zero mismatches; the
per-row staircase = `timer(u,d) = 100u+50−50d`, the clean −50/row gradient).
The pixel residual was TWO stacked errors, each masking the other's fix:
1. **Graded mask cels (one 5-bit step weak).**  Retail binds res `0x458` (the
   alpha mask sheet) and res `0x583` (the opaque cel) through the PLAIN getter
   `FUN_004184a0(0)` at `0x48e920:37/66` — UNGRADED, the quirk-#96 family
   (banner scroll / dialogue bubble / name tab).  The port graded them →
   masks one quantization step weaker → frontier rows ~8 RGB bright.  Fixed:
   slots 40/41 joined the grade skip-list.
2. **The ckpt-105b `hold>=2` fence (one tick late).**  Mask-level extraction on
   the fixed cels (s5 = backdrop5 − out5 per pixel, mode over a 64×4 cell)
   shows retail's frame stamped tick u presents the post-update-u grid
   (`s5(a) == a` exactly, a = 0x1f−(timer<<5)/1000), while the fenced port
   presented post-update-(u−1) (`s5_port(a) == index(T−100)` — fits all 11
   probes including both 5-bit wobbles).  The 105b dt-scan that justified the
   fence ran over the GRADED cels — the content error biased the cost surface
   by exactly one tick.  Fence removed; the step runs every sim tick unfenced.

**Verified:** the reveal band (x<400, grid rows 40-80) is **differ_px = 0 at
EVERY stamp-equal tick 2..32** on the recaptured intro-1; at t30 100% of the
remaining whole-frame residual sits in the fountain box (x 428-591, y 222-351)
= R7, smoke contributes 0.  The ckpt-105 "retail already cleared at t13 /
every-4th-row-equal" readings are RETRACTED as measurement artifacts (pixel
ratios inverted through an assumed linear luminance map + cross-stamp frame
pairing); the grid memory dump is the authority.

**Method note (durable):** when a residual survives a state-equality proof,
extract the EFFECTIVE per-index source levels from the frames themselves
(s5 = d5 − out5 against a settled-backdrop reference) before touching model
code — it separates "wrong state" from "wrong cel content" in one shot.

### R7 — FOUNTAIN spray: RESOLVED + USER-CONFIRMED 1:1 (ckpt 107) — anchor +
### band-order + fresh-droplet fade all fixed by RE; the fountain water is bit-exact
### (upper spray differ_px==0, box 4286 → 305), and 100% of the 305 is butterflies

**Three bugs, all fixed by RE (decompile + field-spec + USER-caught fade), drop the
fountain-box residual 4286 → 305 px at stamp-equal tick 30 (the water is then
bit-exact; the 305 remainder is the butterflies):**

1. **Anchor (the bulk).**  `0x557370` mode-1 spawns at `world + (box)/2 + jitter`
   (decompile `0x557370:56-60`).  The new `0x557370` field-spec (`runs/r7-anchor-
   retail`) reads the fountain prop's `+0xc/+0x10` box = (6400,6400), world =
   (176000,41600) — and the PORT's prop world is byte-identical, so the offset is
   the only free variable.  A port|retail water-droplet blit match at tick 30 (27
   droplets each, same cel sheet res 1032 → identical cel offset, same camera
   since R6 background = differ_px 0) pins the EMPIRICAL anchor at **+1600 both
   axes** (= `+0xc/4`).  At the old +1245/no-Y the spray sat high-left; at +1600
   the spray centroid matches and the box differ falls to ~560.  **OPEN 2× gap:**
   the decompile reads `+0xc/2` = +3200, but the rendered spawn matches at +1600 —
   root cause un-RE'd (`0x557550`/`0x426620` water-config second halving, or a
   doubled `+0xc` unit).  The value is validated against pixels; the formula is not
   yet understood end-to-end (PORT-DEBT fountain-anchor).
2. **Band-order one-tick lead.**  `0x46cd70` walks the PARTICLE band `0x13e0`
   (`0x46e510` step) BEFORE the CHARACTER band `0x11e0` (`0x54f980` emit) — so a
   freshly-spawned droplet renders UNSTEPPED for one tick.  The old port stepped
   then emitted (emit-then-step), integrating fresh droplets one tick early.
   `particle_pool_step` moved before the emitters (RNG-free → stream intact).
   Decompile-verified (`46cd70.c:103-169`).
   The 3-way velocity CYCLE was already correct: `54f980:218-285` increments
   `+0x5c` first then `%3`; init 0 yields `(k+1)%3`, matching retail's
   `abs_tick%3==0→case1 / ==1→case2 / ==2→inline-case0` from tick 0 (verified
   `runs/r7-anchor-retail` + `runs/r7-vel-retail`).

3. **Fade — the FRESH-droplet blend (USER-caught).**  The sub_phase-0 droplets
   rendered too BRIGHT (blown-out pink-white) where retail shows them dim/
   transparent.  Cause: `0x557550` case `0x18708` sets the fresh droplet's alpha
   via `FUN_004385c0(DAT_008a9330)` = **ramp_b[10]** (group B, mode 0 / NORMAL
   blend); the step `0x46e510` only overwrites `+0xf4` with `ramp_a[10−sub_phase]`
   (group A, mode 1 / ADD) from sub_phase 1 on.  The port rendered ALL water via
   ramp_a, so the 3 overlapping spawn-cluster droplets ADD-accumulated to white.
   Fix: `particle_pool_render` renders sub_phase 0 via `ramp_b[10]`, sub_phase 1+
   via `ramp_a[10−sub_phase]` (`particle.c`; host test `particle_render_emits`
   extended).  Position + count + sub_phase timing were already 1:1 (per single
   flip 27/6 == 27/6; the earlier "13 vs 6" was a 2-flips-per-tick DOUBLING
   artifact, so `fountain-collide` is a latent debt, NOT observable here).

**The fountain spray is now BIT-EXACT** — upper spray (y222-286) `differ_px == 0`
at stamp-equal t30; box 4286 → 305 (the fade step alone: 560 → 305, upper 154 →
0), and 100% of the 305 remainder is the butterflies (a separate subsystem).

**Remaining ~305 px = the 2 BUTTERFLIES only — PORT-DEBT(butterfly-wander).**  res
`0x3fa` (1018), KEYED blit `0x5b9b70`, at (484,320)/(532,320) inside the box; the
port FREEZES them (movement FSM `0x43f880` unported) while retail's drift.  A
separate subsystem — the fountain is done.  (Chimney smoke contributes 0 in-window,
letterbox-occluded; sky anchor left at 0 — but the `0x557370` spec read the sky
prop box = 3200, contradicting quirk-#88's "+0xc==0", flagged TODO(sky-anchor).)

**REGRESSION GUARD (USER directive — "check that it stays synced"):** the fountain
spray (position + fade) is bit-exact at stamp-equal t30; the watch is `trace_studio
recapture --only port intro-1` + the fountain-box differ (expect ~305 = the 2
butterflies only).  It should approach 0 when `butterfly-wander` (movement FSM)
lands — the fountain water itself (position + fade) is already bit-exact.

### R8 — dialogue typewriter ROW-TRANSITION pauses: the fitted grade model
### mis-distributes the row-close pause (net −3 ticks; OPEN, ckpt 105) — and the
### ckpt-104 "@2463 zero-mean" attribution is RETRACTED

On the tick axis the dialogue machine is rigid: after the arm recalibration the
pop-in/portrait-fade change SEQUENCE is pixel-identical at Δ=0 (ledger #13) and
the steady typewriter cadence is 5 ticks/char on BOTH sides.  The residual is
confined to the row-1→row-2 boundary: retail's change-tick gaps run {5, 14, 5}
where the fitted grades produce {1, 5, 16} (net −3 → row 2 types ~3-5 ticks
early at tick-equal).  The USER's intro-1 mark @2463 ("retail a couple frames
early on the text reveal") was THIS, compounded by the then-8-tick arm error —
the ckpt-104 "zero-mean oscillation / phase pillar" read came off the flip-axis
pairing whose drift absorbed the constant lag, and is **retracted**.  Chase:
RE the reveal stepper's char-class → grade-slot map, esp. the row-close grade
(PORT-DEBT dialogue-pause-grades).

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
