# OpenSummoners — Progress log

Append-only changelog.  Newest entries first.  Each entry: date + heading,
then 1–3 short paragraphs.  Cross-link to `docs/findings/*.md` and
specific commits where relevant.

---

## 2026-07-03 (ckpt 192) — the RNG census is UNBLOCKED (per-sim-tick emit) + it moves the first divergence to TOWN tick 974

- **Tooling unblock.**  The errands family anim-phase census (ckpt 191) was confounded by
  retail's lockstep flip-batching: OSR_STATE emitted per FLIP, so under lockstep one flip
  (advancing >1 sim-tick) SKIPPED ticks and retail's rng stream degraded to a subsequence of
  the true per-tick stream.  Fixed by emitting STATE on the deterministic sim-tick axis: the
  proxy moved the whole state block from `eh_flip_cb` to `eh_sim_tick_cb` (the easer 0x43d1d0),
  each STATE carrying its own `tick`+`flip`; the port added the matching fields at
  `drive_present`.  The easer fires during the sim update (after FRAMEBEG, before the frame's
  blits), so `osr_scrub_frame_state` (the osr_view panel) still resolves each frame unchanged.
  Commit 75cb2e4; 1104 host pass.

- **`rng_seq_diff.py` upgraded to a TICK-FOR-TICK bubble diff.**  With the lockstep confound
  gone both sides' tick axes align 1:1, so a tick-keyed compare is exact and reveals structure
  the subsequence match hid (it broke at the first bubble and called it total).  The new mode
  reports every divergence BUBBLE + whether it self-heals (LCG re-converges = a draw-TIMING
  wobble) or is PERMANENT; the subsequence match is retained as a legacy per-flip fallback.

- **RESULT — the first divergence is TOWN tick 974, not the errands.**  Re-captured both sides
  per-sim-tick (`port-rngcensus2.osr` / `retail-rngcensus2.osr`, same nav, seed-pin).
  Port==retail RNG **tick-for-tick through the town (ticks 1..973)**; **two self-healing bubbles**
  (584-588, 972 — same total draws redistributed) then the **first PERMANENT split at tick 974**
  (port still DRAWING there, rc climbing — a count/timing split, not the port going static).  So
  the ckpt-191 "gap = errands spawn burst" read was a subsequence-match artifact: the TOWN RNG
  splits ~300t earlier and must align first.  Locus: tick 972 = the 162-tick periodic +20-draw
  event (162×6; k=1..5 matched), tick 973 = a +21 draw ⇒ the periodic butterfly/effect consumer
  at k=6.  `findings/errands-rng-census.md` "Census RESULT".  Next: add the retail `rngcalls`
  counter (proxy trampoline 0x5bf505) to read count-vs-timing, then RE the tick-974 consumer.

## 2026-07-03 (ckpt 188) — the errands entry reveal is EDGES-IN, not center-out; the "−6t 9t-hold" was a MISDIAGNOSIS

- **The documented "fix ready" (insert a ~9t room-load black-HOLD) was WRONG.**  Chasing the
  ckpt-187 −6t errands-entry residual, a fresh `retail-stairs.osr` pixel measurement (whole-screen
  mean brightness + a 5-band row profile per sim-tick, `osr_prof` headless recon; independently
  re-measured) DISPROVES the "retail HOLDS full-black ~9t" reading.  Retail's house→errands
  full-black CORE is only **~2t** (t1650-1651); the long "black period" §14c read as a 9t hold is
  actually a SLOW **EDGES-IN reveal** — the top edge lights first (t1652), the bottom follows
  (t1655), both grow INWARD, and the vertical CENTER band clears **LAST** (~t1685-1691).  Row-band
  evidence: retail t1670 `[69,22,0,6,77]`, t1690 `[70,125,66,86,79]` (top→bot; center = last to
  leave 0).  §14c's "top-down" saw the top lead but missed the bottom + center-last.

- **The real bug was the reveal SHAPE (1-line fix).**  The port armed the errands reveal
  **CENTER-OUT (variant 0)** — the center opened FIRST — sourced from the now-DELETED `retail.osr`
  (ckpt 179).  Fixed `main.c`'s `scene_fade_arm(…, /*variant=*/0 → 1)` = **EDGES-IN**, the same shape
  as the port's own `HOUSE_EXIT_COVER_VAR=1` cover (its mirror).  VERIFIED off `port-edgesin.osr`:
  the reveal now lights the edges first + fills the center LAST, matching retail BAND-FOR-BAND at
  aligned phase — port t1770 `[70,125,42,86,79]` ≈ retail t1690 `[70,125,66,86,79]`.  A 9t hold WAS
  tried (188a) + REVERTED (it made the port full-black 13t vs retail's 2t — the wrong mechanism).

- **The "−6t" was ~measurement noise.**  The reveal DURATIONS already matched (~40t both), so the
  errands "opens" (reveal-complete → dialogue arm) at the right relative offset once the shape is
  right; the old −6t came off fuzzy anchors.  RETIRES `cutscene-errands-entry-latency`; folds into
  `cutscene-fade-variant` (⚠ the arrival→house cover/reveal variants came from the SAME dead
  `retail.osr` era — flagged to re-verify vs retail-stairs).  Residual: port full-black ~4t vs
  retail ~2t (the MODE_OUT cell onset-sharpness, ±1-2t slop).  1097 host pass; only `main.c` changed.
  `findings/errands-render-gaps.md §14d`; feed `edgesin_cmp.png` (port|retail edges-in, phase-aligned).

- **ckpt 188b — the arrival→house COVER + house-entry REVEAL had the SAME bug.**  Re-measuring ALL
  FOUR town-intro fades vs retail-stairs found each variant is an INDEPENDENT RNG roll, and 3 of 4
  were forced to the wrong center-out (variant 0) from the dead `retail.osr`.  Corrected in
  `cutscene.c`: the arrival→house COVER is **TOP-DOWN** (`ARRIVAL_EXIT_COVER_VAR` 0→2 — the sky/roof
  darkens first, ground last) and the house-entry REVEAL is **EDGES-IN** (`HOUSE_ENTRY_REVEAL_VAR`
  0→1); the house-exit cover (var 1) was already right.  VERIFIED off `port-ahfix.osr` BAND-FOR-BAND:
  the cover is EXACT (port t1258 `[0,0,0,0,0,2,9,23,35,0]` == retail t1224), the reveal edges-in
  (center last).  Clears the ckpt-188 ⚠ flag; `cutscene-fade-variant` now lists all 4 fades pinned to
  retail-stairs.  1097 host pass; `errands-render-gaps.md §14d.2`; feed `ahfix_cmp.png`.

## 2026-07-03 (ckpt 186) — the errands PARENTS are CHARACTER-band NPCs (not party-band); Father's Z-ORDER fixed

- **Model correction, off the retail-stairs CLAMP draw seq (t2420).**  The FRONT + §12 had the
  errands FAMILY "blocked on the party band 0x4997b0 (a Phase-3 subsystem)".  The retail draw
  ORDER disproves it: the PARENTS render in the CHARACTER band (the 0x1160 pool, `0x48c150:89-96`
  → `0x493ba0(0,slot,0)` param_3=0 — the SAME band as the shop props), interleaved by SLOT index.
  Retail seq: **Father #257** (EARLY, among the props), counter #287, Mother #289.  Only **ARCHE**
  (the leader, room_state+0x200c) is party-band (#290, `0x4997b0` param_2=1, rendered after the
  character band).  Decompile confirms: the parents are persistent handles 0x5f5e1d3/1d4 placed by
  the errands script `0x4dc510` case-7/8 via `0x41ec20(handle,0x65,worldX,0,facing)`; the `+0x4030`
  party-slot loop is Arche/leader bookkeeping (checks `!= 0x5f5e168`), not parent placement.

- **Father's Z-ORDER FIXED (1 field: ERRANDS_CAST layer 13→7).**  Retail draws Father EARLY, so the
  floor items **res1026 fr48 @(24,400)** / **fr51 @(28,444)** (his own res1026 clutter) + the counter
  occlude him; the port had the whole family at L13/frontmost (the ckpt-180b over-correction, which
  fixed Mom-over-the-chair but wrongly pulled Father to the front) → Father's legs drew OVER the
  floor items.  The floor items are modeled in the port's STRUCTURE band at **L8** (interleaved with
  the L8 pile) — a first fix at L8 did NOT clear them (g_room_cast emits after g_structs), which
  proved they're L8; **L7** puts Father below.  VERIFIED off `port-fatherz.osr`: Father seq **#263**
  now BEFORE res1026 fr48/fr51 (#279/#282) + the counter (#315) == retail's Father-behind order;
  clean placement (#258-261 L5 shelf-backs → #262 L6 fire → #263 Father L7 → #265+ L8/L9/L13).

- counter + Arche were already bit-exact @ the clamp; the parents' anim-PHASE (Father fr5 vs retail
  fr6, Mother fr0 vs fr2) stays RNG-blocked (`effect-anim-phase` / 0x426ec0's `rs+0x72 =
  (rand()*count)>>15` idle start-frame — needs the scene RNG census, Phase 2).  **The parents are
  NOT blocked on the party band** — their retire path is the errands scene-script spawn (0x41ec20);
  only ARCHE-as-leader + her multi-part body need 0x4997b0.  1097 host pass; town/house use no
  ERRANDS_CAST (no regression).  `findings/errands-render-gaps.md §13`.

## 2026-07-03 (ckpt 185) — the additive fireplace FIRE is MAP-DRIVEN at LAYER 6: retire its ERRANDS_CAST capture

- **Continuing the errands un-MVP.**  The fireplace FIRE (0x112e4, res1034) was the last ANIMATED
  ERRANDS_CAST member — an additive glow.  Now map-driven from DATA-1025 into the CHARACTER band,
  carrying its additive blend.  Commit 07b2cd2.

- **RE'd off the 0x431e30 fire case** (`docs/decompiled/by-address/431e30.c:739`): install bank 0x1a3
  (0x426d70), the FIRE clip (0x407b80 DAT_00647e58), the additive blend descriptor ramp_a[14] (0x4385c0
  DAT_008a92f0), and **LAYER 6** (0x438610(6)) — NOT the ex-capture's default L13.  The fire is ADDITIVE,
  so the layer genuinely matters (the blit ADDS to what's behind it): retail draws it at L6, BEHIND the
  layer-8 structure grate + mantel.  The ex-ERRANDS_CAST L13 was a stand-in that only looked OK because
  nothing distinguished it at the tested tick.

- **Wiring:** `CHAR_BANK_DEFS` +1 row 0x112e4 → (bank 0x1a3, L6); `actor_spawn_clip_for_code` +fire →
  FIRE_CLIP; new `actor_spawn_alpha_for_code(code)` → 14 for the fire (else 0), wired into
  `actor_spawn_from_map` (`a->node_alpha`).  Removed the fire from ERRANDS_CAST (now 3 members: family +
  counter).  Position is exact by construction: the map world (32000,32000) + dst_base 0 projects to
  (320,160)+pivot = the SAME net screen pos as the ex-fit (32900,33800)+dst(-9,-18) → the fire lands at
  the ckpt-163 dst (329,178) 48×39 unchanged (the fit just split the placement between world and dst).

- **VERIFIED off `port-fire.osr` vs `retail-stairs.osr` at the errands ENTRY** (the fire x=32000 is
  off-screen at the right-edge clamp, so the entry — cam at the left — is the compare window).  `draw_probe
  --res 1034` @t1710: retail seq264 alpha res1034 fr3 @(329,178 48×39) bmode1; port seq262 alpha res1034
  fr4 @(329,178 48×39) bmode1 — identical primitive/res/dst/bmode.  The seq (262 vs 264) is LOW in BOTH =
  layer 6 confirmed (the ex-L13 fire would sit at seq ~518, after the mantel res1098 @517).  The ±1 anim
  frame is the known −6t entry latency.  **osr_prof recon @t2040 (dialogue clear) is PIXEL-IDENTICAL** to
  retail — the fire glows in the hearth behind the grate + mantel (feed `ckpt185 fire PORT | RETAIL`).
  1097 host pass (`test_errands_fire` rewritten for the map spawn).  `findings/errands-render-gaps.md §11`.

---

## 2026-07-03 (ckpt 184) — the errands ANIM props (clock + pot) are MAP-DRIVEN: retire their ERRANDS_CAST capture

- **Continuing the errands un-MVP (USER "session by session").**  §9/ckpt-183 map-drove the STATIC
  shop furniture; this session retires the two ANIMATED props — the pendulum CLOCK (0x112d9) and the
  cooking POT (0x112da) — from the `ERRANDS_CAST` capture.  They now spawn from DATA-1025 into the
  CHARACTER band (`g_actors`) with their anim clip, driven by the SAME per-tick stepper as the town
  wagon.  Commit c234c73.

- **Three edits (`actor_spawn.c` + `main.c`):** (1) `CHAR_BANK_DEFS` +2 rows 0x112d9/0x112da → (bank
  0x16b, layer **9**) — L9 is retail's default-9 for these codes (0x431e30 routes them via `0x426ec0`
  = the clock/pot ANIM-PHASE init, not `0x438610(N)` = a custom layer; the ex-capture used default
  L13).  (2) `actor_spawn_clip_for_code(code)`: a code→clip lookup (clock→CLOCK_CLIP swing {0,1,2,1}
  dur25 → cels 43,44,45,44; pot→POT_CLIP steam {1,2,3,4} dur6 → cels 57..60; else NULL=static), wired
  into `actor_spawn_from_map`.  (3) the non-town per-tick loop now calls `actor_pool_update(&g_actors)`
  (it only ran `actor_pool_update(&g_room_cast)` before — the town's `game_actor_update` doesn't run
  for non-town rooms; the static map props no-op on clip==NULL).  Removed clock/pot from ERRANDS_CAST
  (else double-draw); `actor_spawn_room_cast` 6→4.

- **Map pos is ground truth** (`map_data.py` DATA-1025, the +0x18 variant): clock @map(528,248) var43 →
  world (52800,24800); pot @map(676,296) var56 → (67600,29600) — EXACTLY the ex-ERRANDS_CAST fit, so
  the migration moves nothing.

- **VERIFIED bit-exact vs `retail-stairs.osr` at the CLAMP** (tick 2419-21, both cameras pinned at the
  map's right edge → phase-independent).  `draw_probe --res 1026`: retail pot seq282 fr57 @(228,208
  28×35) + clock seq283 fr45 @(80,160 29×40); the port's `port-clockpot.osr` emits the SAME draw /
  frame / dst AND **seq (z-order 282/283)** — the map band lands them at retail's exact draw-sequence
  slot.  The anim PHASE aligns at tick-equal (both pot=57 clock=45 @t2420, deterministic).  Over ticks
  2420-2470 the port clock swings 45→44→43 (dur25) + pot steams 57→58→59→60 (dur6); an overlap scan
  reports **"nothing over" the pot or clock at every clamp tick** — so the L13→L9 layer change is NO
  regression.  1097 host pass (+`test_errands_clock_pot_mapdriven`).  `findings/errands-render-gaps.md
  §10`; `port-debt.md` errands-cast SHRUNK to fire+family.  Studio pair now `port-clockpot |
  retail-stairs` (`studio-current.txt` updated).

---

## 2026-07-03 (ckpt 183) — the errands CHARACTER band is MAP-DRIVEN: retire the ERRANDS_CAST prop capture (un-MVP)

- **USER: "un-mvp whatever is mvp'd about this scene, session by session, until it's fully
  un-mvp'd."**  The errands shop furniture/shelf/props were a hardcoded `ERRANDS_CAST` capture
  (PORT-DEBT(errands-cast)); the STRUCTURE band was already map-driven.  This session makes the
  CHARACTER band map-driven too — every static prop now derives from DATA-1025.

- **RE (`0x431e30`, the CHARACTER activator's per-case switch → `CHAR_BANK_DEFS` in actor_spawn.c):**
  each visible case installs one sprite row via `0x426d70(0, BANK, variant)`; the draw layer
  (actor+0xfc) is `0x438610(N)` else the default 9 (`0x426ec0` = clock/pot anim-phase init).
  `frame_base = the map variant (+0x18)` — the SAME source the STRUCTURE band uses, and == the town's
  ex-captured frame_base (0x1129e var1 / 0x1129f var2 / 0x112e5 var36, verified), so `TOWN_SPRITE_DEFS`
  folds into the RE'd `CHAR_BANK_DEFS`.  code→(bank,layer): 0x112cf/d3→0x16f L9, **0x112d1→0x16f L5**,
  0x11279→0x16b L9, **0x112db/dc→0x16c L5** (shelf-backs; fr64 = 0x112dc var64 → L5), 0x112a1/a2/
  11274→0x16c L9, 0x1124c→0x156 L9; codes with no install = invisible volumes.  z is purely by layer
  (char L5/9/10 vs struct L8/15 — no shared layer), so no band reorder; this SUBSUMES the ckpt-182 L7
  shelf-back fix (now retail's true L5) AND auto-fixes the deferred fr64.

- **WIRING** ([`f2435c4`], `main.c`): `reload_room_backdrop` spawns `g_actors` from the room map for
  the non-town rooms; `game_actor_walk` renders the CHARACTER band for EVERY room.  `ERRANDS_CAST`
  shrinks to the DEFERRED cast (anim clock/pot, additive fire, family+counter); the map band spawns
  those 4 codes as invisible volumes → no double-draw.

- **VERIFIED** off `port-charband.osr` (camera-aligned flip 5412 vs retail-stairs t2148): the shelf
  pile reconstructs **`differ_px==0`**; full-frame excl-HUD diff 899px (5px BETTER than the ckpt-182
  build — fr64 now correct) = the pre-existing family-pose + Arche walk-phase residuals only.  TOWN
  t800 + HOUSE t1450 are **0px** vs the pre-session build (no regression).  1096 host pass.
  `findings/errands-render-gaps.md §9`; `port-debt.md` errands-cast SHRUNK.  Remaining (next sessions,
  per the USER's "session by session"): the anim props' clip-update, the additive fire map-spawn, the
  family via the party band 0x4997b0.

## 2026-07-03 (ckpt 182) — the upstairs shelf PILE "props missing" (USER note t2148) = a band/layer z-order bug (FIXED)

- **USER note** (`osr_notes` t2148, crop[288,0,65,40] "props missing on shelf on port"): the upstairs
  shelf PILE — colored books + a cream book-stack + a green box — rendered as bare dark wood; retail
  shows the full pile.  NOT the ckpt-181 §7 cull bug (the objects ARE emitted; `frame_diff` shows every
  res1026/res1027 present both sides at matching res/frame/dst — a pure Z-ORDER divergence).

- **ROOT (via `map_data.py` on DATA-1025 + `osr_prof pick`):** the PILE is two STRUCTURE-band objects
  (code `0xec69`→res1026, layer 8: fr13 books @map(408,128), fr34 stack/box @(404,148)); the shelf-BACK
  it sits in is three CHARACTER objects (`0x112dc` var8→res1027 fr8, `0x112db` var14/11→fr14/fr11 @map
  y128).  The port has no map-driven CHARACTER band for non-town rooms, so those shelf-backs are stood
  in by `ERRANDS_CAST` (`g_room_cast`) at the DEFAULT layer 13.  `g_structs` (the L8 pile) renders
  BEFORE `g_room_cast` in `game_actor_walk`, but the pool presents by LAYER → L8 pile presents before
  L13 shelf-back → pile hidden.  Retail: the CHARACTER band (`0x431e30`, pool 0x11e0) is walked BEFORE
  the STRUCTURE band (`0x493230`, pool 0x2560) in the present builder `0x48c150`, both into layer 8, so
  the character shelf-backs land at a LOWER seq (behind); the dispatcher `0x58d460` inserts each band in
  map-object order with NO depth sort (layer = the object's +0x30 flag).

- **FIX** ([`7076054`], `src/actor_spawn.c`): the 3 upstairs shelf-backs res1027 fr8/fr11/fr14 →
  **layer 7** (== the downstairs shelf units fr9 already at 7).  Since the port renders `g_room_cast`
  AFTER `g_structs`, layer 7 (below the L8 pile) is the correct port classification (L8 would still
  draw them over the pile — opposite band order to retail).  Same class as the ckpt-180b family
  z-order fix.  **VERIFIED** off `port-shelffix.osr` at the camera-aligned flip 5412 (res1071 fr9
  dst_x=276 == retail; tick 2148 coalesces flips 5411@281/5412@276 — compare the 276 sub-frame):
  res1027 fr8/fr14 now seq #282/#286 BEFORE res1026 fr34 #300; the pile reconstructs **`differ_px==0`**
  vs retail over x[290,360] y[0,80]; full-frame excl-HUD diff 1531→904 (exactly the 627 pile px
  removed, nothing added — the 904 residual is the pre-existing family-pose + Arche walk-phase gaps).
  1096 host pass.  `findings/errands-render-gaps.md §8`.

- **Open:** res1027 fr64 (a downstairs shelf-back, retail seq 289) left at layer 13 — no prop overlaps
  it at t2148 (no pixel diff), a drawcall-ORDER-only faithfulness item; the map-driven CHARACTER-band
  spawn (still PORT-DEBT(errands-cast)) retires all these ERRANDS_CAST shelf stand-ins.  (Follows the
  ckpt-181 bed cull fix [`3f7af00`], §7 — the sibling upstairs-render fix that landed the same day.)

## 2026-07-02 (ckpt 180) — the errands t2278 "missing pot/cabinet": the POT RENDERS (stale-trace artifact); the CABINET was the real gap (FIXED)

- **The USER mark t2278 (`port-waitfix | retail-stairs`, "missing pot and kitchen cabinet")
  decomposed** off `draw_probe` on retail-stairs / port-waitfix / port-stairs2.  **The POT
  (res1074) is NOT missing — port-waitfix's freeroam walk is STALE-TRACE.**  At t2278 every prop
  sits **+176px right** of retail (4 props measured: pot 552/376, wall-shelf 284/108, counter
  208/32, res1139 232/56) while Arche is screen-CENTRED both sides (272/270) — an X-follow camera
  holding Arche centred while her WORLD pos lags 176px.  The pot renders, shoved off the crop's
  right edge.  **PROOF:** `port-stairs2.osr` (ckpt-177, pre-waitfix, walk 0-div/301t) at t2278
  matches retail-stairs **BIT-FOR-BIT** (pot res1074 fr2 376/376, wall-shelf 108/108, Arche
  270,296).  The ckpt-179 waitfix (`ARRIVAL_EXIT_WAIT` 10→20) shifted the scene phase but the
  tick-keyed held-trace (`synth-stairs`: RIGHT@2050) wasn't re-timed → the walk desyncs ~65-72t
  past the handoff.  **⇒ FREEROAM studio verification must use `port-stairs2 | retail-stairs`, NOT
  port-waitfix (valid only for the DIALOGUE window ≲tick 2000).**

- **The real gap = the RIGHT-side / upstairs props ERRANDS_CAST SYSTEMATICALLY missed (`actor_spawn.c`).**
  It was captured from the STATIC tick-2200 view (only the room's left/centre), so EVERY object with
  world_x ≳ 55000 was dropped.  A full port-vs-retail draw diff at the **camera CLAMP (t2500, both cameras
  pinned at the map's right edge = phase-independent alignment)** surfaces them.  The USER flagged two:
  the **kitchen CABINET** (res1023 fr4 + upstairs hutch fr2) and — on re-check — **the POT** ("right of
  Mom's head": res1026 **fr58** @228,208).  Added **6** ERRANDS_CAST entries (all DATA-1025 CHARACTER
  objects; res→bank = `asset_register` slot+13; world = map layer ×100, validated the map pos AND the
  retail clamp screen agree exactly): cabinet 0x112d1(L18), hutch 0x112d1(L31), POT 0x112da(L27), + 3
  upstairs props 0x11279/0x112d3/0x1124c (banks 0x16f/0x16b/0x156, fr 4/2/58/38/13/4).  Cabinet/hutch
  LAYER 7 (pre-structure, retail seq 223/224); the rest layer 13 (foreground, seq 282-288).  **VERIFIED**
  off a rebuilt walk-driven `port-cabinet.osr` at the clamp: all 6 render at retail's EXACT screen pos +
  dims; feed `pot_crop_cmp.png` (port|retail crop) matches.  1095 host pass.  Still PORT-DEBT(errands-cast)
  — the recurring gap shows the tick-2200-capture approach is fragile ⇒ the proper CHARACTER def-table
  fill (spawn from the MAP, camera-independent) is the priority retire.  Remaining: res1900 fr0 @(8,444)
  (unregistered bank) + Mom's POSE (res1127 fr0 vs fr2).  `findings/errands-render-gaps.md` §6.

- **LESSON:** a studio mark on a PHASE-desynced capture pair reads a real prop as "missing" when it
  is merely DISPLACED — attribute to the PHASE pillar first (`parity-model.md`).

---

## 2026-07-02 (ckpt 179) — the arrival→house dialogue DRIFT FIXED: `ARRIVAL_EXIT_WAIT` 10→20

- **The town-intro cutscene chain played −11t early because ONE constant was curve-fit**
  (the FRONT's open (a2)).  `ARRIVAL_EXIT_WAIT` was 10 (a bogus "2× anomaly / beat-runner
  pumps the exit wait 2×/sim-tick"); it is the script's literal `in_ECX[0x15f]=0x14=20`
  (`0x4d7d80:268`) mapped **1:1**, like the house/L8 `0x32=50` waits.  Fix: 10→20 in
  `cutscene.c`.  1095 host pass.

- **The scene_fade PERFORMER is BIT-EXACT — the FRONT's "settle-rate" hypothesis is
  DISPROVEN.**  Ground-truthed off `retail-stairs.osr` (the `spam-confirm-nav` re-drive):
  the cover ramp (+80 cells/tick) and reveal recession rate match retail tick-for-tick.
  The earlier "−7t room-load black-hold" read was a CONFOUND of the −11t global WAIT shift
  (the cover armed 11t early ⇒ the whole cover+hold+reveal looked short in absolute ticks);
  with the WAIT 1:1 the house transition is tick-exact with NO room-load modelling.

- **The "2× anomaly" was a MEASUREMENT ERROR.**  ckpt-137 read the cover onset as "first
  CENTER alpha" — but retail's arrival cover grows **TOP-DOWN** (a downward sweep, variant 2,
  NOT the center-out var-0 the port forces as the LCG stand-in), so the center is marked
  LATE.  The true arm = the FIRST 64×4 cell = tick 1213 = L9-adv(1192)+21 ≈ 20.  Also pinned:
  retail's fade cells all capture as **res=0 subtract-blend** (the proxy ID gap) — no
  opaque-vs-alpha split retail-side, so only 64×4 COVERAGE + the res-1110 box are
  cross-side comparable (the old "pin the settle-rate off res-1112" plan could never work).

- **Verified** (res-1110 box open/close, `port-waitfix.osr` vs `retail-stairs.osr`):
  house L0 box open −11t→**−1t** (port 1330 / retail 1331); **house box close tick-EXACT
  (1650==1650)**; errands box open −20t→**−6t**.  `findings/dialogue-advance-early.md`
  "Component 2/3 — RESOLVED"; quirk #122.  RESIDUAL: the errands entry −6t (the house→errands
  transition; the errands ENTRY reveal arms ~6t early with no errands room-load latency →
  `PORT-DEBT(cutscene-errands-entry-latency)`).  **USER visual-verify: studio
  `port-waitfix | retail-stairs` @ ~1200-1350; feed `waitfix_cmp.png`.**

---

## 2026-07-02 (ckpt 177) — `char-turn-state` PORTED: the standing TURN-AROUND

- **The from-rest REVERSAL now plays retail's 8-tick pivot** (the ckpt-175 collision
  residual, ex-PORT-DEBT `char-turn-state`).  GROUND TRUTH off `retail-stairs.osr` res
  0x570 (`draw_probe`): the LEFT reversal @ 2950 holds fr 6 (right turn cel) STATIONARY
  4 ticks, then FLIPs facing + walks (fr 7 → +152 fr 159, 4 ticks), then the walk cel
  160 — the walk onset delayed exactly 4 ticks = the −960 wx the old INSTANT flip caused.
  The RE: `0x442a70`'s STATE-1 FSM (`:1011-1090`) requests the turn action
  `FUN_0040a540(body,2,200)` (gated on Arche's `in_ECX[0x5661]=1`/`[0x5662]=0`), which the
  ANIM form-FSM `0x45e830` plays + flips facing at its keyframe; the 8-tick/dur-4 clip
  length is MEASURED off the draw stream (the un-ported turn form), the same measured-clip
  stand-in as `ARCHE_WALK_CLIP`.

- **Port** (`character.c`/`character.h` `turn_ctr`+`turn_frame`+`walk_accel`;
  `actor_spawn.c` `ARCHE_TURN_CLIP` {6,7}; `main.c` freeroam render): the reversal holds
  `CHAR_TURN_HOLD`=4 windup ticks (vel 0, facing held, fr 6), flips + ramps at tick 5, the
  fr-7 cel lingers to tick 8, then the walk clip resumes.  The +152 facing mirror renders
  BOTH directions from the one clip (R→L 6→159, L→R 158→7 == the house cutscene turn).

- **Verified**: host `test_character_turn_around` (SIM law tick-exact, 1088/1088 pass) +
  `draw_probe port-stairs2.osr` (fr 6×4 → 159×4 → 160 == retail) + `state_diff` vs
  retail-stairs (RIGHT walk 0-div/301 ticks — NO regression; LEFT walk ramp SHAPE
  bit-exact, the −960 gap GONE).  RESIDUAL: a constant **1-tick reversal-ONSET phase**
  (−240) — the port latches the reverse press 1 tick before retail's warmup gate (folds
  into `char-input-autorepeat`, NOT the turn windup; within retail's ±1-2 tick coalescing
  slop).  `findings/freeroam-turn-around.md`.  **USER visual-verify: the pivot animation
  (studio `port-stairs2 | retail-stairs` @ ~2950).**

---

## 2026-07-02 (ckpt 176) — dialogue "runs early" decomposed: L9 box-linger FIXED; char-turn RE corrected

- **The USER studio mark t1197 ("port skipping dialogue early") is FIXED — and it was
  NOT the advance-gate the FRONT hypothesised.**  Decomposed off `port-stairs.osr` |
  `retail-stairs.osr` (the spam-confirm re-drive, one 0x24/14t) + a new per-tick port
  `OSS_DLG_TRACE` dump: every arrival advance PRESS is tick-1:1 (L0-L8) and L9 advances
  on the SAME confirm (sim-tick 1192) as retail — the accept gate (`0x43ce50`/`0x43bca0`/
  `0x43b980`, state-1 skip / state-2 advance) is faithful.  The mark is a box RENDER
  linger: retail draws L9's body through tick 1200; the port (`ARRIVAL exit_box_hold=0`)
  cleared it at 1192.  **Fix: `ARRIVAL_EXIT_BOX_HOLD=8`** (the SAME box-linger the house
  exit uses); drawcall-verified L9 adv 1192→1200, hold 23t == retail.  Chain timing
  unchanged (exit beats still fire at the 1192 advance).  `findings/dialogue-advance-early.md`;
  `test_cutscene_transition_fades` updated; 1088/0/6.
- **OPEN (deferred): the chain still plays early** — a BEAT-duration gap (house ~−10t /
  errands ~−26t at the room-transition fades), NOT the dialogue gate.  Needs the
  scene_fade grid settle-rate RE'd off `retail-stairs.osr` (don't curve-fit).  Measured
  tables + suspects documented in the finding.
- **char-turn-state RE CORRECTED:** the ckpt-175 "`0x426f50(body,2)` case-2 sub-FSM"
  pointer was WRONG — `body+0x38==2` is the DOWN/CROUCH (`param_1[3]==10`, already ported
  as the pose).  The real reversal turn is the STATE-1 horizontal command/facing FSM
  `0x442a70:1011-1090` (`local_24` reversal → `body+0x3a`/`+0x3c` timing → the facing
  toggle :1085 / `FUN_0040a540`; ramp `0x445db0` already ported).  Port DEFERRED (deep,
  entangled with the bit-exact walk).  Corrected in `port-debt.md` + `freeroam-collision.md`.

---

## 2026-07-02 (ckpt 175) — COLLISION lands: Arche stops, climbs, and descends the stairs 1:1

- **The freeroam collision movers are ported + wired** (retires the USER-confirmed
  walk-through-stairs divergence, studio note tick 2441 / `PORT-DEBT(char-collision-mover)`).
  `collision_sweep_horizontal` = `FUN_0054ded0` verbatim (≤100-unit steps; STEP-UP stair
  climb; STEP-DOWN floor hug; leading-edge column scan; partial-commit write-through) +
  the `FUN_0054db10` tile-half wrapper; `character_step` restructured to the retail
  `0x442a70` tick order (support probe = a delta=+1 vertical-mover call → `body+0x24`;
  vertical mover landing/ceiling; ledge walk-off → FALL; the worldX commit with
  step_down=1/step_up=1 pinned off the raw call site + live probes).  Slope profiles
  (`0x5cc410/0x5cc430` 32-byte ramps) read live off the user's exe `.rdata`
  (`exe_data_bytes`) — retires `PORT-DEBT(collision-slopes)`.  Full RE:
  `findings/freeroam-collision.md`.  Ghidra's arg numbering is WRONG on both movers —
  raw push order is the ground truth (now annotated in `retail_fields.json`).
- **The `LAB_00589520` "occlusion marks" are invisible COLLISION WALLS** — the
  `0x1b972/77/7c` shape-1/2 blocks write region-B class-10 wall columns (shape 1 =
  1×5 at (x,y..y+4)); proven against retail's live errands grid (probe
  `runs/arche-box/gridcells`).  Ported; retires `PORT-DEBT(decode-occlusion-mark)`
  (the errands LEFT WALL — "walks through the house" — now stops at wx 6400 = retail).
- **Verified bit-exact** (stairs sweep `synth-stairs.jsonl`, seed-pinned, sim-tick axis,
  `state_diff.py` + new wy axis): the whole RIGHT walk incl. the staircase CLIMB
  (wy 52000→36000, the emergent 200/300-unit step-up cadence) has dwx==dwy==0 every
  tick; both sides stop flush at the col-28 wall (wx 87600); descent + left-wall stop
  match.  Sole residual: retail's 4-tick standing TURN-AROUND on reversal →
  `PORT-DEBT(char-turn-state)` (+ narrowed rows `mover-actor-scan`, `char-drop-through`,
  `char-reverse-decel`).  Arche's live-probed body box: w=2000 h=5600 margin=0.
  Tests 1088/0/6.

---

## 2026-07-02 — res_explorer MAP INSPECTOR: engine-true map rendering host-side

- **New Inspector tab for Map resources** (`tools/res_explorer`): runs the PORT'S OWN
  pipeline outside the engine — `map_data_parse` → `map_decode` (FUN_00587e00 dispatch)
  → runtime render grid → `map_render_tile` (FUN_00490f30 geometry) → layer-sorted
  composite against the real sprite banks (`ar_register_main/group3/game_sprites`
  driven with zdd=NULL, settings=<sotesd HMODULE>; `ar_state_init` REQUIRED first —
  the pointer tables start NULL, was the one crash). Frame→cel math (corrected same
  day, USER-caught mangled multi-row banks): ar_sprite_slice's base_y walks from the
  DIB's LAST memory row = the VISUAL TOP ⇒ frame 0 = upright top-left, cel y =
  (f/cols)*ch; node src rect is upright-top-left; dst px = world/100. Town 1022 = 1268 nodes, 1023 = 669,
  both 0 unresolved banks / 0 unported ids — houses/gate/terraces render exactly.
- **Inspection:** click cell → per-plane raw map_cell + region-A sub-slots
  (bank/res/frame/layer + source-cel strip) + region-B/D collision (class/slope VA/
  wall) + region-C blend (MD_BLEND_* named); click object → layer record + PLACEHOLDER
  renderer note (spawn pass 0x58c8c0 unported). Overlays: collision/walls/blend/grid/
  unported-hatch; town-house vs errands cfg combo. Unknown tile ids hatch red + listed
  w/ cell counts (honest coverage for non-town maps).
- **Not ported (stated in-UI):** ramp banks (ar_register_palette_ramps decodes at
  registration, engine-coupled — skipped; unresolved counter catches any use), region-C
  blend DRAW, palette tint 0x4182d0, sky/parallax backdrop, object spawn/render.
- **Object-layer ACTORS render too (same day, USER request):** the spawn pass is the
  port's own — `actor_spawn_struct_from_map` (STRUCTURE, fully map-driven quirk #84),
  `actor_spawn_effect_from_map` (townsfolk def table), `actor_spawn_from_map`
  (CHARACTER props: fountain/barrels) — described per actor by
  `actor_render_describe` (FUN_0044d160) and composited draw_pool-layer-merged with
  the tiles (map_present mode-0 projection, canvas-origin camera). Town 1022 gains
  trees/flower boxes/doors/signs/villagers; 1023 the tree/scarecrow/shutters.
  Object inspect now resolves the REAL sprite (bank/res/frame/layer + cel preview);
  invisible volumes / off-def-table codes say so explicitly. Actors toggle added;
  marker overlay now defaults off.
- **8bpp colorkey is by PALETTE INDEX (same day, USER: black boxes + a "blank" tile
  row):** the engine's display conversion (bs_convert_*, slicer format hook) substitutes
  pixels whose index == slot->colorkey with magenta BEFORE the magenta-keyed blit
  (group3 scenery/tile banks: index 0; sentinel 0x1ffffff = none; bs_convert's exact
  `idx == colorkey` compare — 0xff00ff-valued keys match no index, faithful). The
  inspector now applies the substitution at sheet build; both symptoms were this one
  bug (the "blank" ground-floor row = index-0 pixels rendering black on black).
- **NPC anchor fix (USER: townsfolk one tile low + x off):** actor_emit_part's node
  offset = descriptor off + RENDER-STATE dst_base (the EFFECT band's per-code anchor,
  both axes; 0 for STRUCTURE/CHARACTER — why only the villagers drifted). The
  inspector's node build now adds it; townsfolk stand on the walkway.
- Inspector is the DEFAULT tab for maps (USER-approved after the actor/colorkey/
  anchor fixes); the schematic stays under Preview. `--shot` gained [MODULE:]TYPE:ID
  spec. res_explorer.exe ships in nightly + tagged releases (wired earlier same day).

---

## 2026-07-02 — res_explorer: the full resource explorer (voice_view retired) + README hero refresh

- **`tools/res_explorer/` — every resource type, engine-accurate.** ImGui/DX11 32-bit
  (osr_view build pattern; `-Wl,--large-address-aware` — the DLLs map ~560 MB), reusing
  the PORT's own decoders: `bs_decode_resource`/`bs_parse_compressed_header`
  (sprites; tool supplies `bs_*` primitives w/ 1041-lang fallback, bitmap_session_win32
  NOT linked) + `map_data_parse` (maps). Kinds: Sprite/Image/Bitmap/Map/Audio/Music/
  Strings/Version/Data; unified sortable-filterable table (clipper), per-kind preview
  (sprite: palette+colorkey+grid+px readout, nearest-sampler zoom via draw-callback;
  map: tile-id schematic + object overlay 5xxxx/6xxxx/7xxxx/8xxxx + parses-exactly badge;
  audio: waveform + seekable waveOut, WMA/non-PCM via MCI; hex+info always), export
  PNG/frames/WAV/WMA/JSON/TXT/BIN + bulk w/ manifest. CLI: --list/--dump (voice_view
  compat) + --export (kind-appropriate) + --shot (self-screenshot, used for README pic).
  Threaded install auto-load (fixes the blank-frozen-window sync load; exact-name exe
  glob — stray sotes-unpacked-*.exe had exhausted 32-bit VA ⇒ bad_alloc).
- **Format facts (verified live):** 756/759 sotesd DATA blobs (incl. sibling magics
  0xe225/0x361f/0x3e49 + chunked 1000-1004 = four 640x352 24bpp sheets) decode via
  the ONE compressed path — dims ARE in the self-rebasing header (lizsoft-sprite.md
  corrected). sotes.exe .rsrc maps all classify + parse exactly. Voice bank 1448 PCM.
  8bpp sheets memory-top-down, 24bpp bottom-up (viewer defaults match).
- **CI/release:** workflows + flake `.#ci` (IMGUI_DIR) ship `res_explorer.exe`;
  `tools/voice_view/` REMOVED. Gate clean (ASF check = 8-byte prefix, not the GUID).
- **README refresh (USER):** early/non-playable status up front; hero pics
  (`docs/media/res-explorer.png`, `ennse-voice-install.png`); support-the-author Steam
  links (SE 1381770 rec + 2012 original 203510); JP voiced-release hunt metadata
  (Deluxe boxed 2009-06-18 Jungle ASIN B0026EQRVE / JP-SE CD; sotesx_s.dll sha256 +
  size); patch target pinned (sotes_en.exe sha256 668f7e1a…, 72,529,416 B, app 1381770
  buildid 23890965, seed VAs) in ennse_voice/README.

---

## 2026-07-01 (ckpt 164–174) — frame-lock re-drive foundation closed + freeroam HUD panel slices 1–3

*(Migrated from FRONT.md, which had grown to 2700+ lines; deep detail lives in
`findings/freeroam-hud.md §1-9`, `findings/freeroam-brake-onset.md`, `plans/frame-lock-1to1.md`.)*

- **Capture-proxy foundation fix (164).** Every `run_proxy.sh` retail capture came back
  empty: a bare app-dir `ddraw.dll` drop resolves to `System32\ddraw.dll`, never ours
  (verified via `GetModuleFileName`). Replaced with **injection** — `build/inject.exe`
  (`CreateProcess(SUSPENDED)` → remote `LoadLibraryA(<full path>)` → `ResumeThread`); our
  DLL loads regardless of search order and is live before the main thread, so the engine-VA
  hooks patch the mapped sotes code AND the `#32770` launcher is dismissed in-process
  (`BM_CLICK`). Retail captures are hands-free again. Commit `1a31089`; quirk #3 + CLAUDE.md.

- **Frame-lock warmup "+1" RE'd & fixed (165/166/168).** The walk-accel/brake ±1-tick
  wobble was chased through the input pipeline (`0x46a880` rising-edge ts → `0x478ba0:182`
  11 ms gate, all one frame) and settled by **measurement** — the proxy state pass read the
  engine's own fields (`cmd0`/`lvlR`/`edgeR`) on a clean sim-tick re-drive: retail's walk
  warmup is **exactly 1 idle tick**. The port's `CHAR_INPUT_REPEAT_DELAY` was one too long
  → 3→2, GT tests re-based on `retail-decomp.osr`. This closes the last frame-lock re-drive
  residual; the sim-tick re-drive is now faithful for steady-state, brake, and warmup-
  anchored onsets. (The `sword2.osr` recording is flip-axis ~2.23 flips/tick, so replaying
  *it* aliases ±1t — bit-exact needs the sim-tick re-drive, not the recording; ckpt 167d.)

- **Freeroam HUD — leader panel, slice 1 (167).** New `hud.{c,h}` (pure geometry/format,
  host-tested) + `game_render_hud` (main.c), hooked after the dialogue layer, gated on
  freeroam + control-handed-off. Bit-exact: HP/MP **bars** (res 0x777), **numbers**
  (12-dark + 2-white grid), panel **frame** (res 0x44b). Slide-in (167b/c) armed on the
  control lock→unlock edge, ramps while unlocked (retail hides the HUD during the errands
  opening dialogue, slides it in at the hand-off). `PORT-DEBT(hud-party-context)` +
  `(hud-slide)` stand-ins.

- **HUD data glyphs (169/170/171).** Element **stars** (res 0x44f); **level** digit
  (res 0x413) — which forced RE'ing the ramp custom-palette bind: `FUN_005b7bd0` overwrites
  the session bmiColors from the installed PALETTEENTRY buffer iff `entries[0].b && 8bpp`;
  ported as `bs_install_palette` + a gated call in `ar_sprite_decode`, retiring
  `PORT-DEBT(hud-ramp-palette)`. **EXP** gauge (res 0x44e) — a mode-4 alpha blit whose
  blend LUT was sourced by **content-match** (dumped all 40 boot LUTs + md5 → exactly one
  hit, `g_pd_boot_group_b[8]`). All dhash byte-identical.

- **HUD item bar — slice 2 (173).** `FUN_004962a0` ×6 (the "wrapper" is `0x494e60` inlining
  it), each slot a BG (res 0x450 **frame 12** — the `0` arg is a lazy-decode entry_idx, not a
  frame select) + ICON + LABEL triad off already-registered banks. Frames found by a boot-
  time dhash sweep (`{44,48,40,36,59,80}` icons, `{4..9}` key-cap labels), verified 18/18
  blits identical on a full errands playthrough. Slide rides `room+0x3c8` (+20/tick,
  `PORT-DEBT(hud-item-hslide)`).

- **HUD door indicator — slice 3 (174).** `FUN_004969b0` compass fully RE'd via objdump
  (4 ECX-hiding traps Ghidra all hides): scan the 32-slot `+0x1160` EFFECT band → validity/
  zone gate → 72000×56000 world-reach pre-filter → **off-screen-only** exclude → project +
  clamp-to-edge (TOP/RIGHT/BOTTOM/LEFT) → dedup-stack (20 buckets, <5px cluster, 5-deep,
  12px perpendicular spread) → highlight `+4` → alpha-fade (`g_ramp_b`). Ported pure
  (`hud_door_process`, 5 branch tests); uses already-ported `g_ramp_b` + pool 0x3a (res
  0x451). **`PORT-DEBT(hud-door-actors)`:** the `+0x1160` actor spawn is a whole unported
  subsystem; the errands' 2 map-data EFFECT objects stay on-screen so the port renders 0
  door blits, matching 0 `res=0x451` in `sword2.osr`.

- **Open blocker — the portrait (171/172).** Its bank is read live off `char+0x50` at the
  party leader-match branch (3 exact call sites static-disasm'd), but `hud_ctx+0x1b4`
  (leader_uid) reads **0x0 on every call across every scripted ring-injection replay** —
  Frida INT3, Frida field-spec, and a native proxy probe all agree. The `.osr` from the SAME
  replay shows the portrait rendering, so it's a genuine **replay-fidelity gap** (the
  scripted replay doesn't reproduce what the original human-played session armed), not a
  probe-placement bug. NEXT: find the `+0x1b4` setter (start from `FUN_0048c150`'s init
  path), or drive a live/manual errands play and probe there. `findings/freeroam-hud.md §6-9`.

- **Tooling (172):** `frida_capture.py` + `opensummoners-agent.js` now accept `{"tick":N}`
  trace entries (previously coerced to `frame:0`, firing every tick-keyed press at boot).

## 2026-06-24 (ckpt 163e) — frame-lock: the walk-accel "−4px gap" was a 1-tick replay off-by-one — FIXED

USER directive (the frame-lock foundation, ckpt 163c-d): drive PORT + RETAIL 1:1 frame-by-frame off the
real-play recording (`sword2.osr` + `sword2-input.jsonl`); every divergence is **port debt or a tooling
gap**, never "good enough".  `sync_inputs.py` converts the recording's inputs → tick-axis held-trace + ring
nav (OFFSET 0 by construction), a DETERMINISTIC INPUT CLOCK pins `GetTickCount`→`sim_tick·33ms` in a
replayed freeroam, and `sync_diff.py` reports per-tick body screen-x + a camera proxy.

**Chase #1 — the walk-accel divergence (sword2 tick ~1896, −4px) was NOT physics.**  `feed_input` read
`g_sim_tick_count` BEFORE `game_camera_step` bumps it (which runs AFTER `freeroam_step`), so a
recording-tick-T trace entry drove the port body **labelled T+1** — every replayed input landed one
sim-tick late.  Fix (`main.c feed_input`): anticipate the pending increment, `sim = g_sim_tick_count +
(g_game_active && (g_game_camera_hold & 1)==0)`.  The walk **warmup** (`CHAR_INPUT_REPEAT_DELAY=3` = 2 idle
ticks) is retail-EXACT — confirmed off the recording (held-edge tick 1886 → motion 1888) + the decompile
(`0x478ba0:229` walk latches at `now − press_ts ≥ 0xb` = 11 ms, well inside one 33 ms tick) + the ckpt-118
`capdash2` byte array (`{0,0,1600,…}`).  So no physics change — only the replay timing.

**Verified** off `port-sync2.osr` vs `sword2.osr`: the port walk now starts at tick 1888 == retail (was
1889), **0px at every settled tick** (the residual −2/−3px is the recording's ~2.23-flip/tick aliasing —
the port reproduces retail's smooth ramp, the recording stair-steps it); camera-follow now tracks +0px (was
+2/+4); the dash still fires (`fr_lr=6` @2149); the first real divergence moved 1896 → 2082.  1063 host pass
(no `character.{c,h}` / test change).  **Surfaced** (next chips, `plans/frame-lock-1to1.md` gap table): the
sword DRAW startup latency (retail Z-press 1807 → fr96 render 1810 ≈ 3t; the port draws at press+0) + the
tick-2082 body gap.

---

## 2026-06-22 (ckpt 161) — chip-2b: the DIRECTIONAL sword attacks (FORWARD / DOWN / BACK) — bit-exact

The held direction at the X trigger now picks the swing **variant** (`character_resolve_attack`,
`character.c`): **DOWN** > a held **L/R** (FORWARD if toward facing, BACK if away) > **NEUTRAL**.  RE'd
off the sword-out form install `0x41f200:1181-1201` — each swing is a distinct registered action+variant
template (`FUN_004287d0(0x27d9,var,…)` / `(0x27da,var,…)`) with its own scripted displacement; the
per-frame handler is the 0xc35b-only `0x45e830` (`442a70:357-369`).  The movement mechanism is `0x447ed0`
→ the collision mover `0x54db10`: a **direct world_x step toward facing** (sign-flipped on `facing==3`),
not a velocity.

**FORWARD** lunges +5400 world (+54px) — `character_step` keeps the vel-lock (brake→0) and adds the step,
even-distributed over the 42-tick swing with an exact-integer running sum (magnitude captured;
the per-substate profile = `PORT-DEBT(sword-attack-gameplay)`).  **DOWN** is stationary and returns to
crouch.  **BACK** turns her around: at completion `character_resolve_attack` flips facing 1↔3 + negates
vel — the literal `0x45e830:363-365` (`+0x54==4` branch).  Clips `ARCHE_ATTACK_FORWARD/DOWN/BACK_CLIP`
(`actor_spawn.c`; FORWARD 120-126 dur-6 / DOWN 112-115 [8,6,5,7] / BACK 144-148 [4,4,7,7,5], +192 left).

**VERIFIED** off `port-attack-dir.osr` vs `sword2.osr` (`tools/trace_studio2/sword_cels.py`): all three
fire clean, every swing cel's dst W×H **byte-identical** (incl. the 80×49/80×47/78×54 wide cels),
durations match within the ±1t entry/exit FSM noise; the FORWARD idle moves dst 162→216 (+54px lunge,
camera follows), the BACK idle flips to the left bank 192-194 (turned around), DOWN stays at 162.
+`test_character_attack_directional`; 1058 host pass.  `findings/freeroam-sword-attack.md` "## chip 2b".
**NEXT = chip 2c:** the UP attack (the separate sheet 0x283f) + the attack trail vfx + the slide body 48/49.

## 2026-06-21 (ckpt 153) — the "res=0" UI sprite bank RESOLVED + the dialogue advance indicator ported

**USER-chosen ckpt-152 task: "resolve the res=0 UI sprite bank" (the dialogue advance indicator + the
inline `@@`-code key-cap icons + the freeroam HUD; "load it from the user's files; bank unlocated").**
RESOLVED via Frida ground truth.  1045 host pass.  Full RE: `findings/res0-ui-banks.md`.

A throwaway field spec (`tools/flow/ui_bank_fields.json`) `thischain`-pins the two dialogue widget
builders `FUN_00410560`/`FUN_00411940` (`this`+`0xb8c`/`+0xb88`, dereffing the bank's `+0x3c` HMODULE
and `+0x40` resource id — the args `FUN_005b7800` passes to `FindResourceA`), and `--res-probe` (hooks
`FUN_005b7800`) names the module per load.  **Result: god+`0xb8c` = PE resource `0x455` (sotesd.dll) =
the port's slot 43 `AR_SPR_FONT_TEX_455`, ALREADY loaded from the user's sotesd.dll; god+`0xb88` = res
`0x457`.**  `res_loads.jsonl` (83 distinct loads) proved EVERY UI/HUD/dialogue bank is a sotesd.dll DATA
resource — so `res=0` in the `.osr` is a capture-side identity gap (the proxy only names cels via the
`0x418470` registry), NOT a special module, and there is no legal blocker for any of them.  The ckpt-149
"likely res 1000 in sotesp" guess (quirk #92) was WRONG — retracted.

res 0x455 is a 128×288 / 4×6 / 32×48 / 24-frame atlas (rendered: `atlas_0x455_labeled.png`): the gold
ornament (0-2), tan ◄/► chevrons (4-11), the feather cursor (12-19; 16-19 = the newgame selection
cursor), and **the green BOOK "next" advance indicator at frames 20-23 (base `0x14`)**.  Ported the
indicator in `render_dialogue_box` (`src/main.c`): it blits slot 43 frame `dialogue_arrow_frame(d)` at
box+(368,92), gated on `dialogue_awaiting_advance(d)`, with a `CALL_TRACE_BEGIN(0x48d940)` mirror.
**Verified off `port-arrow.osr` vs `retail.osr`: the book draws at the errands box (400,284) == retail.osr
seq 825 tick 1823, recon pixel-identical (`book_compare.png`).**  Retires the ARROW half of
`PORT-DEBT(dialogue-arrow-art)`.

**The inline `@@`-code key-cap icons (the USER's tick-1823 flag) are ALSO DONE + pixel-verified** (same
checkpoint).  The codes (exe string `0x86f388`: `@@\x81\xa9`←/`@@\x81\xa8`→/`@@X`) render as 17×17 square
blue key-cap buttons; a 279/279-px slide-match of the `.osr` 17×17 cels against the loaded sotesd banks
found them in **PE res `0x6fa` (sotesd.dll) = port slot 55 `AR_SPR_KEYCAP_6FA`** (the keyboard
button-prompt sheet, 256×128 / 32×32 cells: ←=frame 3, →=frame 1, X=frame 9).  Ported a `@@<code>` parser:
`dialogue_keycap_token` + the `dialogue_expand_text` word-wrap count each icon as 3 cells (not its 3-4
source bytes — the row byte-buffer grew to `DIALOGUE_ROW_BYTES`), and `dialogue_body_row_text` skips the
`@@<code>` in the GDI text + blits the slot-55 frame at the body cell.  **Verified off `port-icons3.osr` vs
`retail.osr`: all 3 icons == retail @(336,210)/(378,210)/(224,266); the whole tutorial line is
pixel-identical (`icons_compare.png`).**  Retires `PORT-DEBT(dialogue-arrow-art)` entirely.  RESIDUAL: only
←/→/X are mapped; other `@@` codes (↑/↓/Z/C) are unknown-skipped until a line uses them.  NEXT: the freeroam
movement types (USER directive ckpt 152), then the freeroam HUD.

## 2026-06-21 (ckpt 152) — the errands-scene opening dialogue (the movement tutorial), tick-aligned

**USER directive:** "we're still missing the opening dialogue for the errands scene (starts once
it switches to the errands scene where you have control). port that first, then port and verify
all types of movement."  So the freeroam HUD (which I had begun scoping) was parked; this
checkpoint did the dialogue, and the movement-types task is next.  1045 host pass.

The questline `0x4dc510`'s entry case (decompile :1086-1116) plays **3 Arche lines** via
`FUN_0049d6e0` — **RE-confirmed to be the SAME dialogue display the town chain already drives**
(cutscene.c is built around 0x49d6e0).  So the errands opening is just 3 more lines through the
existing box/typewriter/portrait/advance system, played CONCURRENT with freeroam control
(retail's beat pump 0x439680 runs the game loop while each line waits): "So the first floor of
our house is the store. Cool!" / "Okay, I need to help with the moving-in!" / "To move around,
I press [<>] & [^v], and to talk to people and do stuff, I press [Z/X], right?" (the tutorial).
Faces 0x02/0x02/0x09 (0x49d6e0 param_8); speaker Arche (0x5f5e165); voices 0.

**The port** (`dded4c8`): `cutscene.c` TOWN_ERRANDS 1-room chain + `cutscene_errands_intro()`
(box anchored to Arche's freeroam spawn 19200,52000, clamped left = the wide bottom box retail
shows at (32,192)-(440,304)); `main.c` `g_errands_dlg_pending` arms it once the entry reveal
recedes (retail plays it AFTER the fade-from-black), re-arming `g_cutscene` so it renders +
advances via the existing path while `freeroam_step` keeps control live.  Retires the DIALOGUE
half of `PORT-DEBT(cutscene-scene-chain)`.

**VERIFIED** off `port-errdlg.osr` vs `retail.osr` (the tick-aligned `nav-errands-dlg.jsonl` =
nav-house-turn + 6 errands confirms; `dlg_reconstruct.py`): all 3 lines render at retail's ticks
(L1@1770 / L2@1800 / L3@1830), name "Arche" @(332,184) color 0x455f7b + body @(168,222) color
0xa8b9cc + the line-3 3-row layout == retail EXACTLY.  Recon on the feed; studio →
`port-errdlg.osr | retail.osr`.  RESIDUAL: line-3 inline button icons render as raw codes
(`@@©`/`@@¨`/`@@X`) where retail draws 17x17 sprites = `PORT-DEBT(dialogue-arrow-art)`.
`findings/errands-opening-dialogue.md`.

**ALSO (ckpt 152): the freeroam HUD is fully SCOPED + committed (`ce332c5`) but DEFERRED by USER**
— `findings/freeroam-hud.md` + `tools/trace_studio2/hud_probe.py`: the full drawcall ground truth
(seq 462-536 overlay), the render architecture (`FUN_00494e60` + ~15 sub-renderers), and the
dependency analysis (res=0 UI source sheets + a captured `PORT-DEBT(hud-party-context)` stand-in).
Resume after the movement-types task.

---

## 2026-06-20 (ckpt 151) — the house Arche TURN is the BLOCKING beat (tick-aligned)

**The ckpt-146 ~7t turn lag is RESOLVED; the turn now fires at retail's exact tick.**  1045
host pass.  This is the "(B) house-dialogue-cadence phase fix" the ckpt-150 handoff flagged —
but the diagnosis reframed it.

DIAGNOSIS (off `port-dash.osr`, a fully matched-cadence nav, vs `retail.osr`): the house
DIALOGUE was ALREADY tick-1:1 (`dialogue_timeline` L0-L7 within ±1t) and the cover-start was
already aligned (1669 == retail) — yet the turn was STILL ~7t late.  This **DISPROVED
`findings/arche-house-turn.md`'s old "the turn auto-aligns once the house cadence is tick-1:1"
claim**: the lag was the missing BLOCKING beat, not a cadence-phase debt.  Lesson: a
dialogue-keyed animation can be tick-1:1 in the dialogue yet still off if keyed to the wrong
beat — suspect a missing beat, not a phase debt, and compare port-vs-retail off an existing
matched-nav `.osr` before assuming a cadence problem.

ROOT CAUSE (RE'd, `0x4d7d80:1163-1184`): ckpt 146 emitted the turn FIRE-AND-FORGET on the
L5→L6 advance and let the matched nav place L6, which dragged the turn ~7t late (the nav
delays L5's advance to absorb the missing beat).  But retail's `0x401e60(Arche,1)` sets cmd-2
("turn to dir 1") AND `in_ECX[8]=4` (the actor-WAIT beat), and the thunk `0x439680` PUMPS it
to completion BETWEEN L5 and L6 — a BLOCKING beat, like the L7→L8 run-off's case-4 wait.

THE PORT (`cutscene.{c,h}`): a `CS_BEAT_ACTOR_TURN` beat type + house L6's lead beat
`HOUSE_L6_LEAD` (`{CS_BEAT_ACTOR_TURN, dir=1, dur=8}`).  The L5→L6 advance arms the turn beat
(the existing lead-gate); L6 opens only after it.  `box_hold=8` keeps L5's box up (full text,
opaque portrait) through the turn — retail's actor-wait doesn't touch the box — then L5
shrink-closes as L6 reopens (the speaker-change overlap, quirk #107; the box_hold path now
gates slide+portrait-dissolve to the run-off CAMERA_PAN lead only).  Removed the now-dead
`cutscene_room.turn_after_line` field.  The nav presses the house L5 confirm at retail's tick
1579 (`dialogue_timeline.py NAV … "7:10,15:15,17:10"`).

VERIFIED off `port-houseturn.osr` vs `retail.osr`: the turn (`draw_probe --res 0x570`) =
158@1579-1582 / 7@1583-1586 / 0@1587 at (354,336) == retail (tick-for-tick); house L0-L7
`dialogue_timeline` tick-1:1 (L5 adv +4t = the box-overlap close); cover-start (res 1112)
1669 == retail; arrival L0-L9 + the L8 run-off (start 1151) unchanged (no regression).
`findings/arche-house-turn.md` (rewritten); quirk #109.

---

## 2026-06-20 (ckpt 150) — the freeroam DASH double-tap trigger (char-run-trigger RETIRED)

**The dash TRIGGER is ported + host-verified end-to-end; commit `43a55f1`, 1045 host pass
(+10).**  Autonomous continuation (USER away, verification deferred).  A USER tap-tap-hold
of a direction now makes freeroam Arche DASH — the last un-wired freeroam move (walk + jump
already worked on live input, ckpt 144).  The dash PHYSICS was already bit-exact (chip 3b,
ckpt 118 — cap 48000 + the two-phase accel); what was missing was the run FLAG that feeds
`character_step`.  Retires `PORT-DEBT(char-run-trigger)`.

RE'd off the decompile (not curve-fit).  The char-AI `0x478ba0` resolves the dash command
`entity+0x14854` (`5`/`6` = dash L/R) from the discrete press RING, not the held axis: each
tick it snapshots the prior command, resets, then scans for a direction DOUBLE-TAP via
`FUN_00479e70`/`FUN_00479960` (two distinct pressed ring records of the same id within the
window, a "used" mask blocking slot reuse so a single held press is not a double-tap), and
self-sustains while held (`local_608[0]==5/6`).  The window `*(*0x8a6e80+0xf8)` is a config
field with no static default → **read LIVE from retail = 800 ms** (`runs/dash-window2`, the
proven `*0x8a6e80` chain at the title), with `run_mode` `*(*0x8a6e80+0x510)==0` confirming
the double-tap branch is the active one.

Ported as `input_dash_double_tap` (`input.{c,h}`) + `character_resolve_run` (`character.{c,h}`,
the dash-resolution half of `0x478ba0`, new `cmd_lr` field) + the `freeroam_step` wire
(`main.c`, `GetTickCount` clock to match the ring).  `CHAR_DASH_WINDOW_MS=800` with
provenance.  10 host tests: the detector (two-in-window hit / single-press miss / window
boundary / flag+dir+stale rejects / no-consume) and the resolution + `character_dash_via_
double_tap` — the input-ring → run → RUN-cap-48000 chain through the REAL physics (vs a
single press → WALK cap 24000).  The double-tap is inherently wall-clock (retail keys it on
`GetTickCount`), so it is off the seed-pinned parity path; the unit tests pin the logic with
controlled timestamps.  Ledger +2 (`0x479e70`, `0x479960`; `0x478ba0` stays unported —
only sliced).  Quirk #113; `findings/dash-double-tap-trigger.md`.

**Also BINARY-verified off a port `.osr`** (`port-dash.osr`): drove the replay into freeroam
(`nav-full-errands` + a tick-keyed RIGHT double-tap) and probed Arche (`draw_probe --res
0x570`).  At tick 1866 (the double-tap) her cel flips WALK `0-3` → RUN `16-19`, the sprite
widens 28→40px, and the dst-x step ramps `2.4 → ~5 px/tick` — the run cap (2× walk), the
two-phase accel.  So `freeroam_step → character_resolve_run → character_step` works in the
real exe, not just the unit.  **USER-VERIFY (visual):** click the studio shortcut
(`studio-current.txt` → `port-dash.osr` | `retail.osr`), scrub freeroam ticks ~1840-1887.

## 2026-06-20 (ckpt 149) — dialogue body-text rows are line-count DISTRIBUTED (USER tick 770)

**The dialogue body-text vertical spacing is RE'd + ported bit-exact; commit `a91696f`, 1035
host pass.**  Autonomous continuation (USER away, verification deferred).  USER: at "We haven't
been here since…" (tick 770, a 3-line line) retail shows 3 lines; the port cut it at line 2 and
the spacing was too tall.  ckpt `d16ae1a` had set a CONSTANT row pitch (`LINE_H=36`/`TEXT_DY=29`)
fitted to the **2-line** case, which over-spaces every other line and pushes the 3rd row out of
the box.  USER: "RE the exact logic, don't hardcode from empirical data; it's nuanced logic with
cases like the box sizing/positioning."

RE'd `FUN_0048da70` (@0x48da70, the grid text renderer): the body rows are VERTICALLY DISTRIBUTED
in a fixed 3-row grid — fewer rows ⇒ a larger inter-row gap.  Per-row baseline Y (`:101`):
`Y(r) = box_y + base_y + (r+1)*gap + pitch*r`, with (`:57-88`)
`gap = min(max_gap, ((max_rows-rows)*pitch)/(rows+1))` (0 when full/empty).  Constants RE'd off
the decompile (the retail.osr data only VERIFIES): `base_y=20`/`max_rows=3` (`FUN_0040df40`
params), `max_gap=20` (`FUN_00410610:19` sets records+0x1c=0x14, unconditionally, at builder
`0x439690:469`), `pitch=28` (no static writer; RE-confirmed by the formula's internal consistency
— the SAME value is the 3-row pitch AND the 2-row gap candidate `(1*28)/3=9`).  So 1 row→gap 20
(@box+40); 2 rows→9 (@+29,+66, pitch 37); 3 rows→0 (@+20,+48,+76, pitch 28).  `rows` is the line's
TOTAL count (over ALL grid records, not the typewriter-revealed subset — the layout is fixed when
the text is set; proven: tick 661 shows 'A' alone already at the 3-row offset 20).

Port: `dialogue_body_gap()` + `dialogue_body_row_dy()` (`dialogue.{c,h}`) replace the constant
`box_y + TEXT_DY + r*LINE_H` in `main.c game_render_dialogue`.  VERIFIED off `port-dlgdist.osr`
vs `retail.osr` (`tools/trace_studio2/dlg_text_probe.py`, the body TextOutA row baselines): port
== retail EXACTLY at every arrival line.  The 3-line line renders all 3 rows (feed montage: the
text aligns 1:1; the residual diff is the inline book/item art = PORT-DEBT(dialogue-arrow-art),
not the spacing).  Writeup: `findings/dialogue-body-row-distribution.md`.

## 2026-06-19 (ckpt 148) — the errands WALL-TINT: per-room palette swap, pixel-exact (USER osr_notes #4)

**The errands floor/wall is PORTED + PIXEL-EXACT (`differ_px==0`); commit `05e8742`, 1034
host pass.**  Autonomous continuation (USER away, verification deferred).  The errands floor
rendered a WARM BROWN where retail is a cooler greenish-gray (USER `osr_notes.jsonl` #4,
tick 1726, crop [75,283,30,38]).

RE'd trace-the-code (the dhash census + the decompile), not curve-fit.  The floor banks
0x187/0x188 are clones of the town floor res 0x769/0x76a PIXELS, but carry an info-entry
**+8 = &DAT_00675500** whose first u16 = **0x186**.  **FUN_00417bc0** is the UNIFIED +8
consumer: first u16 == 0 ⇒ remap WITHIN the embedded palette (the NPC colour-variant body-
bank shift, already ported as the equivalent pixel-index remap, ckpt 142); != 0 ⇒ a
CROSS-SLOT swap — copy palette entries from the slot whose POOL index == that u16.  0x186 =
**res 0x76b**, a 32×32 palette-holder whose embedded palette is the cooler errands-floor
colours.  So the SAME floor pixels render warm in the town (own palette) and grey in the
errands (res 0x76b's) — a per-room PALETTE swap.  The port only modelled the within-palette
case (identity for the floor) → no effect → wrong colour.  (This also dissolves the
ckpt-147 "grade ruled out then back in" confusion: the floor IS graded, but retail's grade
runs AFTER a palette swap the port never did — retail 0x490f30's per-bank order is embedded
→ FUN_00417bc0 swap → FUN_004182d0 +4 scale (no-op for the floor) → grade → install.)

Ported `ar_apply_slot_palette_swap` (`asset_register.c`) — the cross-slot half of
FUN_00417bc0: at decode, before the grade (`title_sheet_format`), overwrite the named
palette entries with the source slot's RAW +0x34 palette (`ar_decode_slot_palette_raw`, NOT
the channel-reordered `ar_palette_session_begin`, so src/dst stay in the order the grade +
8bpp→display convert read).  Tables generated by `tools/extract/info_palette_swap_tables.py`
(3 cross-slot tables in the group3 DATA_SET set; all contiguous identity copies, asserted by
the extractor).  +2 host tests.  VERIFIED off `port-walltint.osr` vs `retail.osr`: floor
sheets res 1897 fr4/5/8 + res 1898 fr4/7/11 all `differ_px==0`; the USER wall crop tick 1726
recon `differ_px==0 maxd==0`; town **58/58** + errands **73/73** shared sheets match (no
regression).  `findings/errands-render-gaps.md` §3.  Closes the last autonomously-tractable
errands RENDER gap (the remaining notes — door indicator / HUD / idle-fidget — are the res=0
freeroam UI + the RNG behaviour subsystem, best with the USER).

---

## 2026-06-19 (ckpt 147) — the errands FIREPLACE FIRE, pixel-exact (USER osr_notes #3)

**The fireplace fire is PORTED + PIXEL-EXACT (`differ_px==0`); commits `e320105` (spawn)
+ `ad405b1` (over-grade fix), 1032 host pass.**  Autonomous continuation (USER away,
verification deferred).  The port showed a black recess where retail has a roaring fire.

RE'd trace-the-code (off retail.osr + a dhash census): res **1034**, bank **0x1a3** (the
`ar_pool_get_slot` POOL index = group3 slot 406 + RAMP_COUNT+1; the actor `bank` field is
NOT the slot index — my first try with 0x196 drew nothing).  Frames **0-5 looping at a
uniform dur 6** (the clip's single `frame_dur` +0x44, read off the CLEAN non-coalesced
retail ticks — the apparent 3/9-tick frames were flip-coalescing, quirk #99).  Additive
**`ramp_a[14]`**: its blend descriptor pulled from retail.osr (constant `blend_ref 36`) is
BYTE-IDENTICAL to the port's `g_pd_boot_group_a[14]` (one match of 20 — also confirms the
port's ramp_a == retail's bit-exact).  Ported as an additive `ERRANDS_CAST` member (new
`room_cast_member.alpha` → `actor.node_alpha`; the existing PRESENT_ALPHA path resolves
`g_ramp_a[param8]`); world (32900,33800) + dst_base (-9,-18), dst (329,178) 48x39.  +host
test `errands_fire`.

Then FOUND+FIXED the sheet residual: a frame-matched recon showed the fire SHEET decoded
to different RGB565 pixels (dhash differed while neighbours matched).  The port's global
8bpp colour-grade was OVER-darkening it — retail grades only `0x417c40`-getter sheets
(tiles/sky), not plain-getter EFFECT sheets.  Excluding slot 406 (`FIRE_BANK_SLOT`) from
the grade → fire sheet dhash == retail bit-for-bit, fire-rect recon `differ_px==0`.

Also RE'd the rest of the errands notes (deferred, entry points logged in
`findings/errands-render-gaps.md` §1-3): the **wall-tint (#4)** = res 1897/1898 errands
floor clones decoding differently — over-grade RULED OUT (excluding their slots was a
no-op), a deeper per-room floor decode; the **door-indicator (#5) + HUD (#7-9)** = the
res=0 freeroam UI subsystem (best with USER); the **idle-fidget (#6)** = the deferred RNG
behaviour subsystem `0x54f980`.  Studio shortcut → `port-fire.osr` vs `retail.osr`.

## 2026-06-19 (ckpt 146) — the house Arche TURN (USER studio notes #3-5)

**Arche turns to face her father just before "I will, I promise!" — drawcall-faithful
(commit `cfc6a96`, 1030 host pass).**  Continued the autonomous house/errands punch-list
(the ckpt-145 scoped gap B).  The town-intro script `0x4d7d80` (house case `0x334c8`)
issues a fire-and-forget actor emote `0x401e60(Arche,1)` at `:1170` (between L5 and L6)
= actor command kind 2 ("turn to face dir 1", `0x43e5b0` case 2).  Off `retail.osr`
res `0x570` (Arche, static at screen (354,336) through the house) her cel runs
`158(4t) → 7(4t) →` the base-0 standing idle `0/1/2` (14t) — turning from the arrival-
listening idle (152-155).  The port previously held the arrival-idle the whole scene.

Ported as a new `CS_ACT_ACTOR_TURN` cutscene action (`cutscene_room.turn_after_line`=5)
emitted on the L5→L6 advance; `main.c` plays a one-shot turn clip on the room-cast Arche
(`HOUSE_CAST[0]`, bank 0x8b) and swaps her to the post-turn standing idle on `rs->done`.
Fire-and-forget like retail's emote (does not gate the next line — the house cadence
already places L6); the live turn FSM `0x43e5b0` stays PORT-DEBT(cutscene-party-chars),
the RE'd clip is the same stand-in pattern as `ARCHE_RUN_CLIP` (ckpt 140).

VERIFIED off a fresh `port-turn.osr` vs `retail.osr` (`draw_probe --res 0x570`): cels +
durations + position match retail EXACTLY (port 158@1586 / 7@1590 / idle 0@1594).  The
turn's absolute tick is ~7t later than retail = the documented house-cadence phase lag
(FRONT ckpt 145), NOT a turn bug — it auto-aligns when that phase debt lands (the turn
is keyed to the advance).  +2 host tests.  `findings/arche-house-turn.md`.

**ALSO — the errands shelf/bookshelf props Z-ORDER fix (`ead9c49`).**  The USER's
"bookshelf missing props" / "missing props in shelves" were NOT a missing spawn: the port
EMITS them (structure band, res=1026/1027, the exact retail frames/positions), but the
ckpt-145 ERRANDS_CAST furniture stand-in (the bookshelf frame res=1023 fr3 + the two shelf
units res=1027 fr9) spawned at the CAST LAYER 13, drawing OVER the layer-8 structure props
and occluding them (the draw pool walks its 27 layers low→high = back→front).  Retail draws
the frame/units first (seq 257/261) then the props on top (268+).  Fix: `room_cast_member`
gained a per-member `layer` (0⇒default 13); the background furniture → layer 7.  Verified
off port-turn.osr: the seq order now matches retail + both flagged shelves recon pixel-match
retail (fully stocked).  +1 host test (`errands_cast_zorder`); 1031 host pass.  Found via
the new `build/osr_prof.exe` headless recon (`findings/errands-render-gaps.md` §4).  LESSON:
a "missing" element may be emitted-but-OCCLUDED — check the draw-stream seq (z).

## 2026-06-19 (ckpt 145) — the house→errands TRANSITION FADE + the missing errands FURNITURE (USER studio notes)

**The house now exits with a fade-to-black cover and the errands enters with a fade-from-
black reveal; the missing shop furniture renders (commits `22047fb`…`f99a78f`, 1028 host
pass).**  USER added studio notes to `port-errands.osr` flagging gaps up to the errands.

- **THE FADE (notes #6/#7).**  The house was hard-cutting to the errands.  RE'd off
  `0x4d7d80:1203-1220` (the house EXIT) + the retail.osr full-frame iris: the house EXITs
  with a fade-TO-black COVER (edges-in var 1) — no preceding wait, unlike the arrival —
  then stages the errands key under black (`return 2`); the errands ENTERs with a fade-
  FROM-black REVEAL (center-out var 0, armed by main.c on chain-complete).  Added
  `HOUSE_EXIT` exit-beats (cutscene.c); both variants forced as the existing
  `PORT-DEBT(cutscene-fade-variant)` stand-in.  **Fade-timing fix (`2a464eb`+`6108ea4`):**
  the cover/reveal were ~8t late (nav advanced the last line at the GLYPH-CLEAR, not the
  CONFIRM); fixed to advance at the confirm (1668) + linger the box over the cover
  (`exit_box_hold`=10) → the cover darkens tick-for-tick with retail.  Also cleared a stray
  errands speech bubble (the box_linger box froze in `cs->closing` at chain-complete).

- **THE ERRANDS FURNITURE (notes #8/#9/#18-21, `01dc162`).**  The counter/bookshelf/clock
  were GENUINELY MISSING (first wrongly dismissed as the reveal phase; USER corrected with
  post-reveal notes).  They are CHARACTER-band objects (DATA 1025 codes 0x112cf/d1/d2/d9 in
  the 70000 range) → bank 0x16f/0x16b, frame_base = the layer variant — but the port's
  character band is suppressed for non-town rooms.  Captured into `ERRANDS_CAST` (differ
  13236→11233).  RESIDUAL: finer bookshelf shelf-props + the fireplace FIRE (animated) + a
  subtle wall tint.

- **SCOPED (RE'd, deferred):** the Arche house TURN (done ckpt 146 above) + the house-
  dialogue-cadence phase fix (the ~13t cover-start lag).

## 2026-06-19 (ckpt 144) — the HOUSE/ERRANDS arc: the HOUSE CAST + the FREEROAM HAND-OFF + the ERRANDS tile-frames

**USER directive (away, autonomous): "the errands scene and the scene right before it (house) — get the map
1:1, implement arche's movement, arche+mom+dad are missing on the scene right before errands; synthesize
whatever trace you need."  All three delivered (3 commits, 1027 host pass).**  The USER pre-staged the matched-
cadence 21-line `nav-full-errands.jsonl` (drives the cutscene → errands); diagnosis via the trace-studio v2
draw_probe / recon against `retail.osr`.

- **HOUSE CAST — Arche+Mom+Dad, position bit-exact (`56161d3`).**  The house backdrop + dialogue + portrait
  were already 1:1; the only gap was the missing FAMILY (the port suppresses the town cast band in non-town
  rooms + never repositioned the persistent entities).  Added a per-room cast pool (`g_room_cast`,
  `actor_spawn_room_cast`/`HOUSE_CAST`): the family (banks 0x8b/0xe3/0xb5 — the town cast banks, which PERSIST
  across the map reload, render res 0x570/0x473/0x467) spawns at captured world positions + idle clips,
  rendered + animated only in the non-town rooms.  Positions solved from the town cast's 1:1 projection
  (1 px = 100 world; house cam 89600/3200) → Arche (128000,39200)/Mother (131200,37200)/Father (134400,37200)
  — they land EXACTLY on cutscene.c's house speaker coords (the ckpt-132 box-anchor RE = independent
  cross-check).  draw_probe tick 1340: Arche @354,336 / Mother @386,320 / Father @418,320 == retail; residual
  = the idle-breathe frame PHASE only.

- **FREEROAM HAND-OFF — controllable Arche walks the errands (`5e76e45`).**  On chain-complete the port hands
  off to the bit-exact mover (`freeroam_begin`/`freeroam_step`, `g_freeroam_char`): `character_step` on the
  live `g_game_drive.input.axis_held` (the +0x200==0 char-AI path, quirk #103), REPLACING the removed ckpt-120
  CHAR_CONTROL_ARM_FRAMES MVP (retires PORT-DEBT(char-control-trigger)).  Spawn world (19200,52000) facing
  right (== retail freeroam-walk / control-path-gt; projects to screen (162,336) at the errands cam (0,16000)).
  Walk cels 0-3 (right) / 4-7 (left mirror, flip_table[0x8b]=4), idle 0/1 — RE'd off the freeroam-walk capture's
  celfr.  VERIFIED off `port-freeroam.osr` (--input-trace nav-full-errands + --held-trace a walk; feed_input
  applies BOTH ring + axis): Arche walks RIGHT 162→475px (cels 0-3) then LEFT →181px (cel 6 = the +4 mirror),
  facing flips.  Residuals (debt): time-based walk cels, static camera (`char-walk-anim-distance`).

- **FREEROAM JUMP wired (`5aa1092`).**  jump_held = axis_held[4] (the C-button level, input_live KEYMAP
  DIK_C → axis 4; character_step detects the rising edge + runs the host-validated windup/impulse/variable-
  height arc).  Extended held_trace to replay the action slots (DIK_C→4 / DIK_X→5 + the "jump"/"attack" names)
  so it's autonomously testable.  VERIFIED off port-jump.osr: Arche's dst-Y arcs ground 336→apex 261→land 336
  (a clean ~75px parabola, in place).  Walk + jump both work on live input; run/dash (the double-tap detection)
  is the only un-wired move (char-run-trigger).

- **ERRANDS tile-frames bit-exact (`f570f14`).**  The errands auto-footprint floor/wall tiles drew the correct
  src (footprint dy) but the WRONG FRAME — the port emitted `cfg->scene_frame` (a constant 0), so every
  wall/floor drew its frame-0 placeholder.  ROOT CAUSE (proven, not curve-fit): the per-cell tile VARIANT is
  the cell's **`arg_0c`** (+0xc), NOT scene_frame — the 8 errands 0x1b977 cells' arg_0c (4,5,5,8,5,5,6,7 by
  column) == retail's res 1897 frames EXACTLY; the town/house don't use these tiles (so the scene_frame read
  was untested).  Fixed `map_decode.c` (0x1b97c/72/77 → `c.arg_0c`).  EVERY errands tile-bank's per-frame draw
  count now == retail (res 1897/1898/1072/1073/1074); full-frame differ 143978→90939 (the residual is the
  errands cast + dialogue, not the backdrop).  Reduces PORT-DEBT(decode-prologue).

- **ERRANDS CAST — the family + shop props (`7ea34cb` + `71717df`).**  Extended the room cast to the errands
  (`ERRANDS_CAST`): Father (bank 0xe3) @480,320 + Mother (bank 0xb5) @624,128 + the 10 static shop props/NPCs
  (res 1027 = bank 0x16c, captured static room-cast members — clip NULL, fixed frame, dst_base 0; the
  TOWN_EFFECT_DEFS pattern).  All 14 res-1027 draws now == retail (frame@pos, no missing/extra; was 4/14);
  errands differ 90939→82639.  The errands BACKDROP + CAST are bit-exact; the residual is the errands
  DIALOGUE/HUD only (the questline 0x4dc510, `cutscene-scene-chain`).

**NEXT (task #5):** the errands opening DIALOGUE L18-L20 + questline (0x4dc510, separate dialogue API 0x4a5ee0
— the bulk of the remaining errands diff = the dialogue box+portrait); the freeroam HUD (res 0); freeroam
refinements (run/dash double-tap [char-run-trigger]; camera-FOLLOW — Arche walks off-screen past ~wx 60000;
distance-locked walk cels); the proper actor-sprite-table struct-code fix for the res-1027 props.

## 2026-06-19 (ckpt 143) — the BUTTERFLY VERTICAL FLUTTER (THEME 2 note #2 residual): the case-3 jump FSM + a captured terrain-mover trigger, bit-exact

**The town butterflies now bob up/down matching retail tick-for-tick — THEME 2's vertical-flutter residual is
RESOLVED.**  RE'd "trace the code": the vertical bob is NOT a bespoke sawtooth but the SHARED case-3 "jump" FSM
(`0x442a70`, the SAME airborne sub-FSM `character.c` ports for Arche's jump), with the butterfly's per-archetype
constants read straight off the install (`0x41f200` case `0xe29a` → `0x427d30(0xffff8300=−32000, 1000, 4000)` +
`0x427c30(1, 16000, 2000)`): a 4-tick windup → impulse −32000 (+2000 on the impulse tick ⇒ −30000) → rise grav
+1000 held / +4000 free (variable-height, like Arche's jump) → fall +2000 capped +16000; `worldY += vvel/100`.
1027 host pass (+1 `test_butterfly_flutter`).  `findings/butterfly-flutter.md`, quirk #112.

- **The flap TRIGGER is the shared TERRAIN-AWARE wander mover `0x43f880`** (mode 1, set by `0x427c30` p1=1): it
  scans the collision grid DOWNWARD for the floor (`local_fc = floor_row*0xc80 − body_h − worldY`) and flaps to
  hold ~8000 units of altitude above it (gated `:311` on `8000 ≤ local_fc`).  The irregular 16-38t flap cadence
  emerges from that scan + the every-other-tick AI gate + the `0x4412d0` command-priority dispatch — the SAME
  mover the freeroam controllable-Arche arc needs.  **USER-chosen scope (this ckpt):** port the real PHYSICS now,
  CAPTURE the trigger as `PORT-DEBT(butterfly-flutter-trigger)` — the per-tick `(state3, cmd2_held)` control
  (2 bits/butterfly) baked from a seed-pinned field-capture (`tools/extract/butterfly_flap_ctrl.py` from
  `runs/butterfly-flutter` → `src/butterfly_flap_ctrl.h`), replayed by `life_tick`.  Deterministic + identical
  to retail.osr under the pinned seed; retire when the terrain mover lands.

- **The port** (`butterfly.{c,h}`/`actor_spawn.c`/`main.c`): vertical state + the case-3 physics in
  `butterfly_step` after the horizontal apply; `butterfly_register` takes the spawn worldY + matches a control
  lane by spawn worldX; `main.c` mirrors `b->world_y` into the EFFECT render-state.  **VERIFIED:** the ported
  physics reproduces the captured `vvel` 0 mismatches / 1824 ticks × 4 butterflies; `port-flutter.osr` vs
  `retail.osr` (`draw_probe --res 0x3fa`, ticks 80-360) the butterfly dst-Y matches retail tick-for-tick, and the
  change touched 0 dst-X (X byte-identical 289==289 = the pre-existing `butterfly-bounds-writer` lag).  Plot on
  the feed.  USER-VERIFY: `osr_view.exe C:\oss-osr\port-flutter.osr C:\oss-osr\retail.osr`.

## 2026-06-15 (ckpt 140) — the cast "ARCHE RUNNING" run-off render (THEME 2 note #5/#3): RUN/DECEL/idle clips + run physics, frame-bit-exact

**The town-intro "Arche runs to the house" beat now RENDERS — the USER studio note tick 1027 ("arche runs to
the house in retail, stands still in port") is resolved.**  On the L7→L8 inter-line beat ("Mom! Dad! c'mon!")
Arche plays her RUN cycle → DECEL → arrival-IDLE and moves to the house door, instead of standing static at
her cast anchor.  1024 host pass (+`test_arche_runoff`).  `findings/arche-runoff-render.md`.

- **Ground truth (retail.osr `draw_probe.py --res 0x570`, ticks 980-1130):** Arche renders as res 0x570
  (bank 0x8b).  Three clip phases — **RUN** cels 16,16,17,18,19,19,20,21 (dur 5, loop; contact frames 16/19
  held 2× → a 40-tick cycle), **DECEL** cels 8-11 (~6t, one-shot), **arrival-IDLE** 152-154 (~14t, loop).  So
  the run clip = `base 16, frame_delta {0,0,1,2,3,3,4,5}, dur 5, loop` (RE'd timing metadata, like WAGON_CLIP).
  The run-off command `0x402730(Arche,+32000)` sets target = world 73104 (= L8 spk_wx); a camera reference
  (res 1004) confirmed her world velocity ≈ the ported run cap 48000/100 → she runs with the REAL run physics.
- **The fix:** `actor_spawn.{c,h}` — the 3 clips + a pure, host-tested run-off state machine
  (`arche_runoff_begin`/`arche_runoff_step`: the real two-phase run accel → cap 48000, `world_x += vel/100`;
  only the DECEL-approach is the tagged `0x54f980`-mover stand-in).  `main.c` finds Arche's slot (bank 0x8b),
  begins the run-off on the L7→L8 `CS_ACT_CAMERA_PAN`, and advances her one sim-tick per `game_actor_update`.
- **VERIFIED** off `C:\oss-osr\port-runoff.osr` vs retail.osr: the cel sequence matches retail tick-for-tick;
  run-start aligns (port 983 / retail ≤980, the matched dialogue cadence); the ~40px screen-position residual
  is the PRE-EXISTING camera-pan phase (the static cast cel res 1027 is the same ~40px off, in port-theme3.osr
  too) = `PORT-DEBT(ingame-camera-pan)`, not an Arche render error.  Residuals: the exact velocity/decel curve
  + the 12,0,6,159 transition flourish stay the `0x54f980` mover (`cutscene-party-chars`); Arche doesn't turn
  to face the cast at the door (`dialogue-portrait-facing`).  Port|retail montage on the feed.

## 2026-06-15 (ckpt 138) — the THEME-3 transition FADE punch-list: center-out + box-over-cover + house L0 +8t→0, tick-1:1

**The arrival→house transition FADE now matches retail — the ckpt-137 USER studio residuals (1)-(3) are
CLOSED.**  Commit `cd1c547` (code).  The fix was driven entirely by reading the retail `.osr` DRAW STREAM as
ground truth (`tools/trace_studio2/draw_probe.py` + a per-column scene_fade cell census), then a fresh port
capture verified each point tick-1:1.  1023 host pass (unchanged — no test-shaped logic changed).

- **(1) The fade VARIANT is CENTER-OUT (var 0), not edges-in.**  Read off retail.osr: the arrival-exit COVER
  (tick 1234) and the house-entry REVEAL (tick 1261) both start at the MIDDLE rows (`half`=60) and grow/recede
  OUTWARD at +4 rows/tick — center-out.  The variant is genuinely the RNG draw `(rand*3)>>15` (`0x439690:563`,
  no override), but the port's LCG is **misaligned at these arms**: aligned at game_enter (its establishing
  reveal correctly rolls 0, quirk #94) but drifted by ~tick 1233 because the unported cast
  (`cutscene-party-chars`) consumes the LCG differently in between, so the live draw rolls 1.  `main.c`
  CS_ACT_FADE now forces the beat's `fade_var` (0) as a center-out STAND-IN (keeps the `rng_rand()` draw to
  match retail's per-arm consumption count); the establishing reveal keeps its faithful live draw.
  `PORT-DEBT(cutscene-fade-variant)`.
- **(2) The dialogue box renders IN FRONT of the cover.**  Retail keeps the closing L9 box over the cover —
  the box shrinks out over ~20t from L9's advance (tick 1224), rendered after scene_fade (`draw_probe`:
  BOX>fade through tick 1245).  The port already renders the closing box after the fade; it just needed the
  cover to OVERLAP the close instead of arming after it (fixed by (3)).
- **(3) House L0 +8t — the COVER-ARM TIMING, NOT a hard wipe.**  Retail ages the fade cells too (alpha
  `bmode=1`, ~10t) — the ckpt-137 "al=0 hard wipe" was the proxy's `res=0` on the alpha blit.  The real cause:
  the port armed the cover after the script's literal WAIT 0x14=20t, but off retail.osr the cover arms only
  ~10t after L9 advances (1224→1234).  The exit wait-timer (`+0x57c`, −1/beat-runner call) evidently runs
  ~2×/sim-tick in the EXIT context — yet the house-entry WAIT 0x32=50 maps 1:1 (50t); the 2× exit anomaly is
  not yet pinned in the decompile.  `cutscene.c` calibrates `ARRIVAL_EXIT_WAIT=10` from the draw stream
  (a flip-vs-tick rate detail, quirk #99 family).  At 20 the cover armed 10t late → the closing box had
  vanished (no box over cover) AND house L0 opened +8t late.

**Verified off a fresh port `.osr` vs retail.osr:** cover var-0 onset tick 1235 (retail 1234, +1 from the L9
advance), reveal var-0, the closing box on top of the cover (drawcall z-order), house L0 first-glyph 1370
(retail 1372, was 1380 = +8t) — and house L1/L2/L3 stay tick-exact (1398/1448/1493, nav-anchored).
Port|retail montage (ticks 1240/1245/1275) on the feed; engine-quirk #109 extended.  **USER-VERIFY:**
`osr_view.exe C:\oss-osr\port-theme3.osr C:\oss-osr\retail.osr` (the studio shortcut is loaded with this
pair) — scrub ticks 1224→1300: the box runs ahead, the screen fades to black from the MIDDLE with the bubble
on top, the room swaps under black, the house fades in from the middle.

**Follow-up (same ckpt, USER studio note tick 1293): the cinematic LETTERBOX is now town-arrival-only.**  The
port rendered the establishing-shot letterbox bars in the house too; retail drops them at the arrival→house
room swap (verified off retail.osr: present through the arrival t1190-1245, GONE in the house t1291+).  Gated
`letterbox_render` on `g_loaded_room_key == CUTSCENE_ROOM_ARRIVAL`, so the house + errands render bar-free,
port|retail bit-matching at the top/bottom (montage on the feed).  `PORT-DEBT(ingame-letterbox)` reduced to
just the constant 64px heights (the on/off is now faithful).

**NEXT:** THEME 2 — the cutscene CAST + ambient render (the "Arche running" sprite `cutscene-party-chars`,
the butterfly/NPC colour variants `0x4997b0`), THEN the FREEROAM HAND-OFF.  See `plans/intro-cutscene-1to1.md`.

## 2026-06-14 (ckpt 137) — the arrival→house TRANSITION CHOREOGRAPHY (THEME 3): the non-dialogue beat model lands the transition TICK-1:1

**The town-intro cutscene now plays the interleaved NON-DIALOGUE beats between arrival L7 and the house
dialogue, tracking `retail.osr` tick-for-tick.**  THEME 3 of the intro-cutscene punch-list.  USER
course-correction this ckpt (twice): *the "don't guess" in CLAUDE.md means trace the EXACT retail code that
creates the behaviour and port THAT — don't measure choreography and bake a constant* (memory
`re-not-measure-choreography`).  So every beat is RE'd from the decompile.  Commits `d02f2b7` (beat model +
L7→L8 camera) → `114a807` (the case-4 RE fix) → `351691f` (the fades) → `3903c9d` (run-off=97, tick-1:1).
1023 host pass (+2).  Engine-quirk #109.

**The beat model (`cutscene.{c,h}`, pure C, host-tested):** `0x4d7d80` is a FLAT sequence of ops the
beat-runner `0x439690` pumps — dialogue lines INTERLEAVED with camera pans (`in_ECX[8]=3`), wait timers
(`=6`, `+0x57c`), scene fades (`=2`), and actor run-offs/emotes (`0x402730`/`0x401e60`), each blocking until
it completes.  Added `cutscene_beat` (WAIT / CAMERA_PAN / FADE) + a per-room SPARSE `cutscene_line_lead` map
+ `exit_beats`; `cutscene_step` runs a beat sub-phase (closes the box, runs the beats, then arms the line /
advances the room) and emits a one-shot `cutscene_action` main.c performs against the live camera/scene_fade.

**The three transition beats, RE'd exactly:**
- **L7→L8 "Arche runs ahead to the house":** a FIRE-AND-FORGET camera pan to (28000,12800)@400 — the
  `0x43d1d0` easer (`0x439690:623-641` sets the view tgt then clears `in_ECX[0x13]`; draw-verified −148px
  scroll, settles ~53t) — PLUS the case-4 RUN-OFF wait.  Key RE: `0x402730` does `*(in_ECX+0x20)=4`, OVER-
  WRITING the beat type to 4, so the wait at `:229` is case 4 (`0x439690:1137` waits the 32-slot actor-beat
  pool to clear), NOT case 3 (camera).  The run-off duration (97t = the MEASURED 167t L7adv→L8-first-glyph
  gap − wait50 − the ~20t box pop-in) is the actor MOVE stepper `0x54f980` = the cast (a tagged
  `cutscene-party-chars` stand-in, the same cast the baked run TARGET `spk_wx=73104` does).
- **The arrival EXIT (`:267-291`):** wait20 + a fade-TO-black (cover) that GATES the room-key swap — the
  `0x401d40` stage / `0x402030` commit / `reload_room_backdrop` happen UNDER full black (no early snap).
- **The house ENTRY (`:1056-1080`):** a fade-FROM-black reveal + wait50, before house L0.  The fades RE the
  scene_fade arm `0x439690:555-563`: MODE = `in_ECX[10]` (1 reveal/_OUT, 2 cover/_IN), SPEED = `in_ECX[0xb]`
  = 1000, VARIANT = the LCG `(rand*3)>>15` (main.c draws it at the wire site, one per arm).  A FADE beat
  completes when the live `scene_fade` grid SETTLES (`cutscene_set_fade_active` = retail's case-2 grid-done
  gate), not a guessed dur.

**VERIFIED off a port `.osr` over the extended matched nav (`runs/cutscene-verify/nav-theme3.jsonl`; the nav
generator now applies the #108 −8 overlap only to a quick reopen, not a long beat-gap / room boundary):** the
dialogue timeline lands L8 first-glyph 1150 (retail 1149, +1 = the L7-advance), L9 1190 (==), house L1 1398 /
L2 1448 / L3 1493 (all ==).  **USER-VERIFY:** `osr_view.exe C:\oss-osr\port-theme3.osr C:\oss-osr\retail.osr`.

**Open THEME-3 residuals:** (a) house L0 is +8t — the port's `scene_fade` ages each cell over 10t (alpha) but
retail's transition fade is a HARD WIPE (`al=0` in the draw stream, settles ~16t cover / ~30t reveal); the
cover settles ~8t slow.  A scene_fade follow-up.  (b) The "Arche running" SPRITE (note #5) needs the live cast
(`cutscene-party-chars`).  (c) The RNG fade-VARIANT (iris pattern) alignment needs a retail OSR_STATE capture.

## 2026-06-14 (ckpt 136) — the dialogue PORTRAIT FADE-OUT dissolve: DRAWCALL+LUT-EXACT on every arrival speaker change

On a speaker change the OUTGOING bust now DISSOLVES out via the reverse cross-fade ramp (idx 18→2, the
`ramp_b` LUTs played backwards, then GONE) at its old box anchor while the box is still full — the chip the
ckpt-135 fade-in verification surfaced.  Verified off both seed-pinned `.osr` with a new probe
(`tools/trace_studio2/portrait_fade_probe.py`, the portrait blit's per-tick BLEND ref → ramp idx): the port
matches `retail.osr` **TICK-FOR-TICK on ALL THREE arrival speaker changes** — L0→L1 idx 18,16,..,2 over
ticks [688,696] gone 697; L1→L2 [733,741] gone 742; L2→L3 [778,786] gone 787 (the per-side BLEND ref numbers
differ by a constant — the same shared LUTs — but the IDX is identical).  Recon montage on the feed:
differ_px=0 at the mid/late dissolve ticks, the only residual a 1px ≤1-LSB cross-side sheet sample present on
the OPAQUE pre-dissolve bust too (the standing accepted noise).  1021 host pass (+2).  Engine-quirk #108.

- **The mechanism (engine-quirk #108): retail processes the advance ~2 ticks BEFORE the new box opens.**  It
  arms the OLD box's reverse-ramp dissolve immediately, while the new box's re-pop has a ~2-tick setup
  latency, so the dissolve LEADS the box-frame close by 2.  The coordinated faithful fix:
  (1) the matched nav presses the speaker-change advance 2t early (`dialogue_timeline.py`: `−8` not `−6`);
  (2) `cutscene.c` DELAYS the new box's re-pop 2t (`reopen_delay`/`CUTSCENE_REOPEN_DELAY` — the main box is
  hidden, the closing-box snapshot dissolves, then ARM_REOPEN fires; the box-frame growth stays at
  `advance_tick−6` so the ckpt-134 28/28 box overlap is preserved);
  (3) the closing box runs the reverse ramp — `dialogue_arm_fadeout` (`portrait_fade`=450=idx 18) +
  `dialogue_fadeout_step` (−50/tick → 0), `dialogue_portrait_ramp_index` returns idx 18→2 then
  `DIALOGUE_PORTRAIT_GONE` (-2 = draw nothing; `main.c` skips the portrait blit on it).
- **No new render divergence.**  The fade-IN, the box overlap (quirk #107), the cadence (THEME 1) all stay
  1:1 — the same `portrait_fade_probe.py` confirms the fade-in mapping (idx 0,0,2,..,18) on both sides
  unchanged.  USER-VERIFY: `osr_view.exe C:\oss-osr\port-fadeout.osr C:\oss-osr\retail.osr`.

## 2026-06-13 (ckpt 135) — the dialogue PORTRAIT FADE-IN: drawcall+LUT-exact (the 2-tick idx-0 hold)

The USER's ckpt-134 "portrait slightly less dim at tick 661" note: NOT the ramp formula or the box-open (both
already faithful).  Retail HOLDS the dimmest cross-fade step (ramp_b idx 0) for TWO opening ticks because the
cross-fade state (`0x49c910` `+0x2e`) arms one tick AFTER scale hits 1000 — `0x49c910` returns early WITHOUT
`f += 50` on the first fully-open tick.  Fix: `dialogue_box.fade_armed` gates the `portrait_fade` increment on
a prior arm tick (`dialogue.c` `dialogue_step`).  Verified LUT-byte-identical port↔retail, 13/13 portrait
ticks, every arrival line (commit `9153763`).  Engine-quirk #108; surfaced the FADE-OUT dissolve (ckpt 136).

## 2026-06-13 (ckpt 134) — the dialogue CADENCE is TICK-1:1: the matched-cadence nav + the box re-pop model land THEME 1 of the intro-cutscene punch-list

With a MATCHED-CADENCE nav (tick-keyed confirms at retail's exact dialogue ticks) the port's arrival dialogue
(L0-L7) now tracks `retail.osr` TICK-FOR-TICK — every line's start / full-reveal / advance tick is bit-equal,
and 314/323 ticks of (speaker name, revealed body) are identical.  THEME 1 of `plans/intro-cutscene-1to1.md`
is DONE.  1017 host pass (+5).

- **The high-value diagnosis: the port's reveal/skip MECHANICS were ALREADY faithful.**  A new tool
  `tools/trace_studio2/dialogue_timeline.py` reads the reveal curve straight off `retail.osr` (counting the
  body MAIN glyphs, colour `0x3e537d`, per sim-tick — the cross-side identity, NOT font_ref) and proved retail
  types 1 char / 5 ticks (space 1t) with an instant `reveal→total` skip — EXACTLY the port's model.  So note
  #4 ("port reveals text earlier after X/Enter") was a CADENCE artifact: the `0x24`-SPAM nav skipped every
  line the instant it armed and raced ahead.  Engine-quirk #107.
- **Tick-keyed input-trace (`input_trace.{c,h}`).** An entry may key on the SIM-TICK (`{"tick":N}`) not just
  the Flip frame — the axis the trace-studio tick-join shares, so a tick-keyed confirm fires at the same
  sim-tick on both sides (the port's Flip cadence differs from retail's).  Axis is PER-ENTRY → one nav mixes a
  flip-keyed boot prefix + tick-keyed in-game confirms.  `input_trace_replay` takes `sim_tick`.
- **The box re-pop model (`dialogue.{c,h}` + `cutscene.{c,h}`).** The port re-armed (full 20-update pop-in)
  EVERY line.  Retail keeps the box OPEN across a SAME-speaker advance (`dialogue_set_text`, gap 1t) and
  re-opens from HALF scale on a SPEAKER CHANGE (`dialogue_reopen`, `DIALOGUE_REOPEN_SCALE=500` → ~10-update
  reopen, content at advance+11t = retail).  `cutscene_step` picks the arm mode per advance (OPEN/REOPEN/KEEP
  by speaker `name_va`).  Plus `dialogue_expand_text` now KEEPS the word-wrap space (retail renders it) → the
  body is byte-identical.
- **Verified:** `port-matched.osr` (over `runs/cutscene-verify/nav-matched.jsonl`) L0-L7 start/full/advance
  all bit-equal to `retail.osr`; per-tick (name,body) 314/323.  Commits: tick-keyed input
  (`c5895f9`+`dce9023`), same-speaker keep-box (`14cc140`), reopen+wrap (`e1f3646`), the tool (`552a46c`).
- **The speaker-change box OVERLAP — DRAWCALL-EXACT (USER studio note, tick 696; commits `eb0b3a6` first
  cut, `6e22c1d` the CLAUDE.md no-approximation rule, `dfb5986` draw_probe, `e6f0e66` the exact port).**
  Retail OVERLAPS the opening NEW box (IN FRONT) over the closing OLD box (BEHIND) on a speaker change; the
  port single-swapped.  The first cut was a curve-fit (linear -100/t close + scale-500 reopen + old box in
  front) shipped without reading the trace — the USER rejected it: "no approximation, drawcall per drawcall;
  if probing tools are lacking, improve them."  So I built `tools/trace_studio2/draw_probe.py` (the
  ordered-drawcall region probe — the box scale/pos/z was in the `.osr` all along) and ported retail's EXACT
  choreography: z-order CORRECTED (new in front), open spawn 200 +50/update, close -40/update removed <160,
  old box lingers full until the new box passes half (`box->scale > 500`), advance fires at advance_tick-6.
  **VERIFIED drawcall-per-drawcall** (draw_probe, port vs retail): every box-frame cell matches across all
  arrival speaker changes — L0->L1 28/28, L1->L2 28/28, L2->L3 29/29, L5->L6 33/33 ticks EXACT; per-tick
  (name,body) 322/323 (tick 884 = a retail-coalesced flip).  Persisted the no-approximation /
  drawcall-per-drawcall / improve-the-probe / look-at-it-yourself rule in CLAUDE.md.
- **USER-VERIFY: `osr_view.exe C:\oss-osr\port-matched.osr C:\oss-osr\retail.osr`** — scrub the arrival
  dialogue (ticks 661-982).  NEXT: THEME 3 (the Arche-runs-to-house beat after L7) then THEME 2 (the cast
  render).

## 2026-06-13 (ckpt 133) — the dialogue TYPEWRITER-SKIP ported: the confirm-while-typing desync blocker is closed; the port advances at the press cadence (chain @hold 2571 vs 11365)

The USER-flagged ckpt-132 blocker.  The port modeled only the ADVANCE half of the dialogue confirm, so it
waited out every typewriter while retail skips+advances — the port lagged, the two desynced, and the
dialogue-section 1:1 studio compare was blocked.

- **RE'd the skip from the beat-runner `0x439690:976-1011`.** The dialogue box widget is in state 1 (typing)
  OR state 2 (waiting) — an `if/else-if` (`:978` vs `:1004`), MUTUALLY EXCLUSIVE per tick.  Retail's ONE
  confirm (ENTER/X = ring `0x24`) does exactly one thing: press while TYPING → SKIP (`FUN_0043bca0`'s `0x24`
  poll calls `FUN_0043ce50(9)`→`FUN_0043ca40(9)`, forcing the text machine fully-shown and returning 3; the
  beat-runner reads the 3 to step state 1→2); press while COMPLETE → ADVANCE (`FUN_0043b980`).  The skip
  press is consumed by state 1; it does NOT also advance that tick → ~2 confirms/line, retail's cadence.
  (`FUN_0043ca40`/`FUN_0043ce50` are the SAME generic widget engine the menu ports — `menu_list_nav`/the
  latch; the dialogue box is a separate reduced model.)  Commit `56f270d`.
- **Ported** `dialogue_typing` (content-visible && reveal<total = state 1) + `dialogue_skip_reveal`
  (reveal→total) in `dialogue.{c,h}`; `cutscene_step` now SKIPs (if typing) else ADVANCEs (if awaiting) on
  the confirm — a confirm during the pop-in is eaten with no effect (faithful to `FUN_0043ce50` returning 0
  until scale==1000).  Renamed `advance_pressed`→`confirm_pressed`.  Host test `cutscene_typewriter_skip`.
- **The live CONFIRM is ENTER/X not Z — RE'd the `0x46a880` producer** (the `findings/input.md` "only
  remaining black box", now RESOLVED — commit `3bd9a76`).  The ring WRITER walks the pressed keys and posts
  ring events: FIXED binds (ENTER `0x1c`→`0x24`, ESC/BACKSPACE→`0x27`, arrows→1/3/2/4) + CONFIGURABLE buttons
  from `*0x8a6e80` (each fans out): `+0x558`(X)→{8,0x25,**0x24**}, `+0x574`(C)→{**7**,0x27}, `+0x5ac`→
  {0x26,0x24}, `+0x590`(Z)→{**9**,…}.  So CONFIRM = ENTER (fixed) or X (config +0x558); Z is the `+0x590`
  sheathe button (ring 9, NOT confirm).  Fixed `input_live.c` KEYMAP (ENTER+X→`0x24`, dropped Z→`0x24`); the
  per-key SCANCODE defaults stay `PORT-DEBT(keybind-config)`.
- **Verified.** Host suite 1012 pass (+1).  Over the same confirm nav the chain COMPLETES @hold 2571 vs the
  old advance-only 11365 (4.4× — the port advances at the press cadence, no longer waiting out typewriters).
  A fresh `C:\oss-osr\port-skip.osr` tick-joins `retail.osr` **2027/2042 paired, 3 anchors RNG-aligned** —
  the port stays on retail's tick axis through the dialogue.  Montage on the feed (arrival→house, skip-driven).
  RETIRES `PORT-DEBT(dialogue-typewriter-skip)`.  NEXT: the studio CONTENT 1:1 sweep — a matched-cadence nav
  (the port spam-nav vs retail's nav must press `0x24` at the same sim-ticks for per-line content alignment),
  then drill residual divergences in `osr_view`.

## 2026-06-13 (ckpt 132) — the dialogue BOX POSITION ported faithfully (`0x49c640`): the box anchors to the speaker, 17/18 town lines bit-exact

The last ckpt-131 rendering gap up to the errands.  The port hardcoded the box at (174,148) (the line-1
value); retail computes it per line by `FUN_0049c640` from the SPEAKER's world position projected through
the camera.  USER chose the faithful fix — port the function, not bake the result.

- **Harness-captured EVERY `0x49c640` input from retail first** (`tools/flow/box_pos_inputs_fields.json`;
  `runs/box-pos-inputs`).  Needed a new agent read kind `argchain` (arg-rooted pointer hops, like `thischain`
  from ECX) to reach the speaker's body sub-object `*(*(arg+0x40)+off)`.  The capture PROVED the formula and
  confirmed the port's `mr_camera` IS `0x490b90`'s `in_ECX` (the same `off34/4c/5c/60/64/68/74` the tile walk
  reads), so the projection is a free reuse.  Formula (quirk #106):
  `box_x = clamp((sprite_w/200 - W/2) + scr_x, 0x20, 0x260-W)`,
  `box_y = (metric_14/100 + spk+0x1c) - H - 0x30 + scr_y`, `scr_x = wx/100 - cam_x/100` (two truncating
  divisions), `scr_y = wy/100 - (cam_y + scroll·100)/100`.  Both clamps fire in the real data (box_x 32/200).

- **Ported as `dialogue_box_position`** (pure C in `dialogue.c`, host-tested bit-exact vs all 10 distinct
  captured cases incl. both clamps + the centered fallback).  `cutscene.c` bakes the per-line speaker world
  pos (the cast MOVES — Arche runs ahead at arrival L9) + the per-character body geometry (Arche's head sits
  8px lower → off_1c=-8); these are HARNESS-CAPTURED because the live cast that owns them is
  PORT-DEBT(cutscene-party-chars).  `game_render_dialogue` projects through the LIVE `g_game_camera_mr` each
  frame, so the box tracks the camera (`dialogue_scaled_rect` now takes the anchor).

- **Verified:** a port `--call-trace` (the new `0x49c910` box-position annotation) over the same nav
  reproduced retail's box position for arrival L1-L9 + ALL house lines BIT-EXACT.  The ONE residual —
  arrival L10 Mother, port (62,148) vs retail (32,148) — is because retail's camera panned to 28000 to follow
  Arche running ahead at L9, while the port's static cast keeps the camera at 12800: an upstream-input pillar
  (the camera), downstream of PORT-DEBT(cutscene-party-chars) + the camera-pan debt, NOT the box logic.
  Retires the box half of PORT-DEBT(dialogue-trigger).  1011 host pass (+1).  Fresh port `.osr` at
  `C:\oss-osr\port-boxpos.osr` for `osr_view` vs `retail.osr`; per-speaker-anchor montage on the feed.

---

## 2026-06-13 (ckpt 131) — ROOM-RENDER GAPS CLOSED (errands floor + house/errands props) + the dialogue-portrait BUST bug; the box-POSITION arc is next

USER directive: use trace-studio v2 to find + close the rendering gaps up to the errands, frame-by-frame
1:1, before going further.  Four real bugs found + fixed (the portrait one via the draw-drill the USER
insisted on), all harness-verified; the box-position is the remaining 1:1 gap (USER-chosen the faithful
fix).

- **Errands BOTTOM FLOOR — fixed** (`e8d5c0b`).  The floor tileset bank `0x188` was boot-cloned from the
  wrong source (`0x186` = a 1-frame 32×32 sprite res 0x76b), so its floor tiles (frames 4/7/11 at row
  y=13) were culled by the `ar_sprite_slot_frame` f_38 bound → the whole bottom floor missing.  Retail
  RUNTIME ground truth (frida pool read, `tools/flow/errands_tileset_fields.json`): bank 0x187=res 0x769
  f_38=24, 0x188=res 0x76a f_38=16 — the town (DATA 1022) floor banks 0x184/0x185 CLONED into 0x187/0x188
  (the two area-0xd2 rooms share the floor sheets).  Fixed the inline-clone sources {0x187←0x184},
  {0x188←0x185}; +1 regression test.  Retires the errands-floor half of `assetreg-clone-defer`.

- **House + errands PROPS — rendered** (`1d826c3`).  `game_actor_walk` (the band walk) was gated to the
  arrival room only, and the bands were spawned once in `enter_game` from the TOWN map.  The STRUCTURE band
  is fully MAP-DRIVEN (quirk #84), so `reload_room_backdrop` now re-spawns `g_structs` per room (house→21,
  errands→58 props) and `game_actor_walk` runs for EVERY room (STRUCTURE always; the town-specific
  EFFECT/CHARACTER cast bands stay gated on `room_is_town`).  House renders its flower boxes / "Items" sign;
  errands its shop furniture.  Remaining errands gap: the CHARACTER-band shop items (35) + the room cast —
  per-room sprite tables (`actor-sprite-table`, Phase 2b).

- **Dialogue PORTRAITS — the +13 pool/array offset (EVERY bust was wrong)** (`cbbab94`).  The USER flagged
  "many divergences"; the draw-drill (osr.py BLIT scan + osr_prof recon) showed L3 Mother rendering 176×144
  vs retail 160×176.  Root cause: the portrait render indexed `g_ar_sprite_slots[pslot]` DIRECTLY, but the
  0x49d6e0 face table returns a POOL index, and the pool is offset from the array by the ramp slots
  (`ar_pool_get_slot` = `[i-(RAMP+1)]` = i−13).  So every portrait was shifted +13 → the wrong bust
  resource (right character, wrong expression/variant) AND, for var-B slots, wrong dims.  HARNESS-VERIFIED:
  retail pool 676=res 0x7ef / 704=0x7f9 / 460=0x5a3 (all 160×176) live at the port's slots 663/691/447.
  Fix: resolve via `ar_pool_get_slot(pslot)`.  Verified the port now emits res 0x7ef (L1) / 0x5a3 (L2) /
  0x7f9 (L3) — matching retail's slots EXACTLY — and L3 renders 160×176.

- **Dialogue box POSITION — the remaining 1:1 gap (USER-chose the faithful fix; next chip).**  Retail
  anchors the dialogue box to the SPEAKER's projected screen position (`0x49c640`: project the speaker body
  world pos via `0x490b90`, center the box, clamp), so the box moves to whoever speaks (Father box≈(174,148),
  Arche≈(94,160), Mother≈(62,148)).  The port HARDCODES `DIALOGUE_BOX_X=174/Y=148`.  Ground truth captured:
  `tools/flow/portrait_pos_fields.json` reads the live box obj `+0xc/+0x10` per line (hook 0x49c910,
  param_1=this+0x54, *param_1=box obj).  The arrival cast is largely CLUSTERED/static during the dialogue
  (early vs late frames match) with slight drift, so the fix is: port `0x49c640` over the cast's
  (mostly-static) positions + the projector, then refine with the walk-in cast if the drift matters
  (`cutscene-party-chars`).  NOT YET DONE.

Trace-studio note: the NOTE/mark UI is dual-mode only (single-file scrub has no notes panel) — a studio gap
to build.  The USER flagged frames directly (4103/5165) which worked.

The whole reason trace-studio v2 was pulled forward (ckpt 125): resume the room-render/freeroam port.  The
town-intro cutscene chains arrival → house → errands; previously the house/errands lines played over the
TOWN backdrop because their rooms never loaded (PORT-DEBT(cutscene-room-render)).  This ckpt ports the
per-room map-load so **both the house (DATA 1023) and the errands/freeroam (DATA 1025) ROOM BACKDROPS now
render** — the main-goal room renders its real scene.  5 commits (`c2b1568`..`87bf668`); 1009 host pass
(+5).  House + errands frames on the feed.

**M1 (`c2b1568`) — scene + parallax from the active room.**  `game_world_room_render_cfg(w, key, &scene,
&p2, &p3)` resolves a room key to its DATA scene (`GW_ROOM_SCENE`, the FindResourceA id) + the 0x587e00
prologue params (param_2=room[0x44], param_3≈room[0x43]) from the embedded registry; `town_render_load`
takes the parallax params; `main.c load_room(key)` drives the load.  Byte-verified: arrival 0x334be→1022/(4,1),
house 0x334c8→1023/(4,1), errands 0x334dc→1025/(9,4).

**M2 (`e228150`) — the house + errands map_decode arms, BIT-EXACT to a retail emit capture (the "main
RISK").**  Ported the 14 new FUN_00587e00 tile-id arms the interiors exercise (the 10xxx/112xxx/113xxx/
171xxx/172xxx families, incl. the 113xxx AUTO-FOOTPRINT floor/walls retail inlines as a grid-rectangle loop
= emit_tile span 0/0) + the param_4 tileset-bank prologue (`map_decode_cfg` / `map_decode_cfg_init`).
GROUND TRUTH: a retail capture of the decode emit sequence (frida hooks on the emit primitives
0x58c910/0x58ca80 across the cutscene chain, `runs/room-render-gt`, `tools/flow/map_decode_fields.json`)
cross-referenced with the cell (tile id, shape) histograms (`map_data.py --cells`).  A standalone host probe
decodes the real DATA 1023/1025 with recording emit-stubs — every emit_tile/emit_obj (bank, slot, flag,
count) matches retail EXACTLY (town 178/104 regression, house 111/50, errands 98 obj + 37 captured-arm tiles
+ 78 direct-write 113xxx = the histogram).  Resolved the architecture question: the room swap DOES re-run
0x587e00 per room (3 decodes captured), and param_3 (the scene field local_918) = 0x14 for every town-area
room (normalizes to 0 = the 113xxx tile frame).  Deferred: PORT-DEBT(decode-occlusion-mark) (the 113xxx
shape-1/2 region-B/D culling marks).

**M3 (`c3accc0`) — room-keyed reload + per-room camera.**  `reload_room_backdrop(key)` (town_render_free +
load_room + camera snap) fires on the cutscene room swap (arrival→house) + the chain-complete errands load.
The house/errands SETTLED camera origins are HARNESS-CAPTURED (`tools/flow/room_camera_fields.json` reading
the scene view object `*(*(0x8a9b50)+0x104c)` cur_x/cur_y across the chain — both STATIC): house
(89600,3200), errands (0,16000).  The town cast/effects are suppressed for non-town rooms (`room_is_town`).
Retires PORT-DEBT(cutscene-room-render).

**Tooling (`87fafd5`) — `--no-frame-limit`** uncaps the in-game 60 FPS gate (gated on g_game_active; the
title/menu nav stays capped, since uncapping the whole run desyncs the frame-keyed title nav) so the
~13000-frame cutscene→errands replay captures in ~9 s.

**M4 (`87bf668`) — the errands-render CRASH fixed.**  Rendering DATA 1025 access-violated: an under-loaded
errands tileset bank (PORT-DEBT(assetreg-clone-defer)) made `ar_sprite_slot_frame`'s unbounded
`frames[frame_id]` read OOB → a garbage cel `game_present_blit` dereferenced.  Bound frame_id against
`slot->f_38` (the slice frame count) when known — an under-loaded bank culls the tile (a gap) instead of
crashing; retail's f_38 always covers the index, so no behavior change there.  The errands now renders
coherently (gaps where a bank is under-loaded).

**NEXT:** the FREEROAM HAND-OFF (controllable Arche in the errands room via `character_step` on live input,
the mover is DONE bit-exact); load the under-loaded errands tileset banks (the gaps); the room cast (Phase
2b).  See HANDOFF + `plans/controllable-arche-faithful.md`.

## 2026-06-13 (ckpt 129) — M8: the trace-studio GAME-STATE panel (opt-in OSR_STATE; the RNG census folded in)

The studio gains an engine-state pillar (USER-requested, modeled on openrecet's orv3_state opt-in pass): a
native, extensible record of NAMED once-per-frame engine-state fields, captured opt-in on both sides and
shown in osr_view per joined tick, port-vs-retail, diff-highlighted. The RNG census/survey is its first
content — the live per-tick rng diff supersedes the standalone `rng_tick_diff.py`.

Format (M8a, `src/osr_format.h`): a new OSR_STATE record (=14) — `u32 nfields` then `{name[16], kind
(hex/int/f32), i64 ival, f64 fval}` per field; `osr_enc_state` / `osr_dec_state_field` + u64/f64 helpers.
Generic by construction (the emitter appends whatever it reads; the decoder is field-agnostic), so the field
set grows as engine state is annotated. Port emit: a per-frame accumulator in `osr_emit.c`
(`osr_emit_state_field` push + `osr_emit_state_enable`) flushed as one OSR_STATE after each FRAMEBEG, armed
by `--osr-state`; `main.c` pushes `rng` (the `DAT_008a4f94` mirror `rng_peek_state`) + `rngcalls` (new
cumulative `rng_call_count` in `rng.c`) at the drive_present flip site — add more `osr_emit_state_field`
calls there as annotated. `osr.py` decodes it (`STATES` dump, streamed; a SUMMARY line). Retail (M8b,
`tools/capture_proxy`): opt-in `OSS_OSR_STATE` → the flip hook writes one OSR_STATE with `rng` (eh_read_seed,
a free global read) right after FRAMEBEG; retail `rngcalls` deferred (`PORT-DEBT(osr-state-rngcalls-retail)`
— it needs a 0x5bf505 trampoline counter). Viewer (M8c, `tools/osr_view`): `osr_scrub_frame_state` decodes
a frame's OSR_STATE; an "ENGINE STATE" table (field | port | retail) shows the union of both sides' fields,
each formatted by kind, a port≠retail value highlighted red.

Verified headless: a 150-frame port boot capture emits STATE=150 (rng=0x4f5347 pinned, rngcalls=0 — no RNG
in the title, correct); `osr.py STATES`/`SUMMARY` and `osr_scrub_frame_state` read them; host test
`test_osr_emit_state` (+1, 1003 pass) asserts the codec round-trips all three kinds + the accumulator resets
per frame. `tools/rng_tick_diff.py` archived to `tools/archive/` with a banner (superseded by the panel);
`rng_consumer_census.py` kept (consumer ATTRIBUTION is a separate analysis). CLAUDE.md carries the how-to +
extend-it pointer. Commits `ba0b801` (M8a) + `8da3dcb` (M8c) + `2a6f424` (M8b). OPEN: the GUI panel + the
retail OSS_OSR_STATE capture are USER-verify.

## 2026-06-13 (ckpt 129) — M7: the studio drill-in + the note hand-off (draw inspector + crop/note marks)

The studio becomes self-diagnosing and gains the human→agent channel (USER-requested: "the M7 stuff plus a
simple note system so I can add notes with a crop region to tell you what to look at").

The NOTE/mark system (openrecet N4, our v1 marks/worklist returning natively): in osr_view dual mode the
USER drags a crop rectangle on any panel (frame-space, drawn live on all three) + types a note → it persists
to `osr_notes.jsonl` beside the `.osr` (tick, port/retail flip, crop[x,y,w,h], differ, text); a notes-list
panel seeks/deletes; notes load on startup; keyboard scrub is suppressed while typing.  `tools/trace_studio2/
notes.py` is the agent READ side — it reads the JSONL, resolves each note's tick → per-side frame index via
the same join as `pair.py`, and with `--render` reconstructs the cropped port|retail|diff at that tick
(`osr_prof` dump + PIL crop), `--feed` pushes it.  So a mark says exactly "look HERE at THIS sim_tick" and
round-trips to a precise visual on the agent's side (verified end-to-end headless against a C-format sample).
Gap panels now label "no frame at this tick" instead of bare black (the USER flagged the honest post-tick-191
black port panel).

The DRAW INSPECTOR (openrecet N3, the self-serve "which draw made this pixel" loop): `osr_scrub` grew
`frame_ndraws`/`frame_draws` (the ordered BLIT/TEXT/CLEAR list + human labels), `render_rgba_upto(idx,K)`
(reconstruct applying only the first K draws — watch a frame build), and `pick_draw(idx,px,py)` (which draw
last changed a pixel, one incremental pass).  A second osr_view window exposes them: a PORT/RETAIL radio, an
"up to draw K" slider, the clipped draw list (click a row → render-to-there + highlight its rect), and
click-the-image pixel→draw pick.  Engine verified headless (`render_rgba_upto(all)==render_rgba`; on port
frame 1309 the build-up is clean CLEAR→town→+banner, `pick(200,150)`=draw #615 the banner) — and it earned
its keep immediately: it revealed that the tick-0 game_enter frame is a TRANSITION (near-black, a town
composed then wiped by a late mid-frame scene-transition CLEAR, quirk #105; both sides identical → the
earlier "town establishing shot" label was corrected to "transition frame").  USER-CONFIRMED the studio
("looks good"); the M7 crop-drag / inspector interaction is the open visual-verify.  Commits `953ee74`
(notes) + `b568104` (inspector engine) + `6279274` (inspector GUI); montages on the feed.  NEXT: RESUME the
now-unblocked room-render/freeroam port (the studio exists to iterate it); studio polish (survey 4/5/6) is
pull-when-needed.

## 2026-06-13 (ckpt 129) — M6: the tick-join studio — both sides' `.osr` paired by `sim_tick`, native PORT|RETAIL|DIFF three-panel + diff ribbon

Trace Studio v2 reaches its first usable deliverable: a frame-by-frame 1:1 port-vs-retail scrub on the
parity (sim_tick) axis.  The v1 sync-bug class — pairing on the FLIP axis with a ±drift pixel search that
hunts through content-quiet stretches and absorbs retail's coalesced ticks — is dissolved.  Every FRAMEBEG
already carries the deterministic `sim_tick` (the easer `0x43d1d0` count, reset at game_enter), so pairing
is a JOIN on that stored identity: group each side's frames by tick, take the LAST flip per tick (the
presented state — retail coalesces / re-presents, quirk #99), align on the tick union, and keep honest
port-only / retail-only GAPS rather than silently mispairing (the openrecet E3 model — sim_tick is a
stronger key than their present-count).

M6a (`tools/trace_studio2/`, runnable from WSL): `osr.py` grew a block-buffered streaming iterator
(`stream_records`/`stream_frames`/`read_header`) that yields only the small records and skips the bulky
BLIT/TEXT/SHEET payloads by seek arithmetic — it streams the full **1.9 GB `retail.osr` (37673 frames) in
~11 s with no OOM**, retiring the survey-flagged `parse()` OOM debt.  New `pair.py` joins both sides and
reports the paired count, the honest gaps, per-shared-anchor RNG assertions (the join-validity proof), and
the flip-axis drift contrast; `--write-pairs` → `pairs.json` (reference only).  VERDICT on
(port-m5, retail-snap): **PASS — 190 tick-paired, 2 honest port-only gaps (ticks 41/91), all 3 shared
anchors RNG-aligned**; game_enter lands at port flip 1116 vs retail 1242, so naive same-flip pairing would
silently drift +126 flips (~63 ticks) — the tick-join is immune.

M6b (`tools/osr_view`): `osr_view.exe <port.osr> <retail.osr>` opens two scrub sessions (safe — `zdd_create`
is a clean per-instance alloc; offscreen recon never touches the one window-present global), builds the
tick-join natively from the two frame indices (`build_join`, NO `pairs.json` dependency — the viewer is
self-sufficient), and shows PORT | RETAIL | DIFF at the joined tick with a tick-indexed scrubber + a
precomputed diff HEAT RIBBON (per-paired-tick `differ_px`, aggregate worst-per-column, click-to-seek,
worst/next-diff nav).  `diff_image` amplifies the cross-side diff (faint silhouette where equal, yellow→red
by magnitude where divergent).  Single-file mode unchanged; the Makefile now links `src/osr_emit.c` into all
three osr_view targets (zdd.c hard-refs its M5 taps — fixing a latent M5 link break).  VERIFIED HEADLESS via
`osr_prof` dumps (the same `osr_scrub` engine the GUI wraps): the join indices match `pair.py` exactly, and
**sim_tick 0 — the game_enter town establishing shot — reconstructs `differ_px==0` (PIXEL-IDENTICAL port vs
retail)**, while sim_tick 97 shows `differ_px=264` (0.09%, a small localized divergence the studio surfaces).
1002 host pass (unchanged — tooling only).  Commits `2788ed9` (M6a) + `57260be` (M6b); montages on the feed.
OPEN: GUI visual-verify (USER) — the windowed DX11 app needs a Windows launch.  NEXT: M7 drill-in, or RESUME
the now-unblocked room-render/freeroam port (the studio exists to iterate it).

## 2026-06-13 (ckpt 128) — M5: the port `.osr` emitter — the port writes the proxy's draw stream, self-reconstructs `differ_px==0`

Trace Studio v2's two sides now speak from one codec.  New `src/osr_emit.{c,h}` (pure C, host-linkable,
every sink a cheap no-op without `--osr-emit`) mirrors the retail proxy's hook map exactly: FRAMEBEG/
PRESENT at `drive_present` (the proxy's present-then-framebeg frame structure), BLIT at the 5 zdd
primitives (`zdd_emit_blit` extended to carry dest/blend-desc/srcw+srch), dedup'd per-CEL SHEETs grabbed
through an injected lock-based surface reader (sheet_grab.h's exact hash shape, tombstoned eviction at
the zdd_object dtor — the ckpt-126 stale-sheet lesson), CLEAR at `zdd_object_clear` (quirk #105), mode-4
BLEND with blend_grab.h's per-mode LUT sizing, GDI TEXT through a per-HDC shadow bound at
`zdd_object_get_dc` (glyph ops + dialogue mirrors; the banner-cel compose is filtered — its pixels reach
the file via the composed cel's SHEET), FONT at the `ar_gdi_create_font` chokepoint, and ANCHOR/SEED at
the existing pin sites.  BLIT/CLEAR/TEXT emit only for the PRIMARY dest (dst_handle 1), retail's observed
stream shape.

Verified three ways.  Host: +2 tests (1002 pass / 0 fail) — a full write→`osr_replay` read-back round
trip (frame structure, shared seq, sheet dedup + post-evict no-reemit, dest filter, font/blend refs).
Decode: a 1500-flip intro-1 run emits 316k blits / 173 sheets (0 grab fails) / 18k texts / 38 blends,
and `osr.py` reads it **100% named + 100% dhash/dst coverage** with all 4 anchors at the proven flips.
Ground truth: `--osr-replay` of the port's OWN `.osr` rebuilds flips 700/900/1250 against the port's live
`--capture-frames` BMPs at **`differ_px==0`** — the port stream is self-contained, the same bar M4d set
for retail.  Commit `cc99f3a`; montages on the feed.  NEXT: M6, the sim_tick identity JOIN + the
port|retail|diff scrub in osr_view.

## 2026-06-13 (ckpt 127) — M4d: the `--validate` ground-truth gate lands, passes 71/71 `differ_px==0`, and fixes the menu artifact (`OSR_CLEAR`)

The trace-studio-v2 fidelity gate is real: a new `OSR_SNAP` record carries the REAL retail backbuffer
(the proxy Locks the compose surface at the flip hook, after the closing frame's draws, before its
PRESENT; `OSS_OSR_SNAP_EVERY` / `OSS_OSR_SNAP_FLIPS`), and the reconstructor compares its accumulated
dest at exactly that stream point, dumping `real_/recon_` BMP pairs on mismatch.  First run: **67/68
snaps `differ_px==0`** across boot→title→menu→prologue→town→dialogue→house-freeroam — alpha, GDI text
and the house scene are now proven against retail's actual screen.

The one failure (flip 800) was the USER-flagged "menu CLIPPED artifact", root-caused as the missing
scene-transition CLEAR (**quirk #105**): retail zero-fills the compose surface via `FUN_005b9410`
(per-frame at the title, bursts at transitions) and the newgame-menu scene does NOT fully redraw — the
accumulating recon kept stale title pixels and the menu dialog's grow animation stacked onion-ring
borders.  New `OSR_CLEAR` record (proxy INT3 at `0x5b9410`, filtered to the tracked backbuffer; an
ORDERED draw replayed by the recon and by osr_view's scrub, where a clear-only frame now counts
non-empty).  Re-capture (anchors byte-identical): **71/71 clean**; osr_view renders flip 800 identical
to retail.  1000 host pass; montage on the feed.  Next: M5 the port emitter.

---

## 2026-06-13 (ckpt 126) — the house-freeroam recon bug was the CAPTURE: stale sheet cache fixed (+ mode-2 src extent)

The USER-flagged house-freeroam defects (white panel "holes" + Arche-head fragments) were not recon
geometry — the `.osr` referenced the WRONG SHEETS.  `sheet_grab.h` cached ptr→dhash once-per-surface
forever, but the engine destroys + reallocates sheet surfaces at a room swap (**quirk #104**, zdd dtor
`0x5b9390`); a cel at a recycled pointer recorded a town-era dhash — 79/509 blits on flip 6390, the
"holes" being a 640×480 all-white dialog-panel sheet.  Diagnosed offline with a streaming blit↔SHEET
cross-check over the 1.5 GB capture.

Fix: the proxy INT3-hooks the dtor and evicts `+0x2c`/`+0xac` from the sheet + surfid ptr caches
(tombstoned); while the format was open, the mode-2 RECTS gap closed too — `osr_blit` grew `srcw/srch`
(80→88 B payload, legacy captures still decode zero-filled).  Recaptured the same nav (anchors
byte-identical, ~950 fps): flip 6390 clean (**USER-confirmed**), prologue/town `differ_px==0` vs the
old reconstruction; 999 host pass.  Also `run_proxy.sh` no longer auto-SUMMARYs GB captures (osr.py's
non-streaming parse OOMed the tmux session).  Commits `f39ca4c` `3832e4a` `588605f`; writeup in
`plans/trace-studio-v2.md` "RESOLVED (ckpt 126)".

---

## 2026-06-12 (ckpt 125) — TRACE STUDIO v2 M4: the `.osr` → frames RECONSTRUCTOR (+ the alpha blend capture)

M4 reconstruct lands: the port binary's `--osr-replay` mode rebuilds frames 1:1 from a captured draw stream
through the port's OWN bit-exact sinks (zdd.c blits + real GDI text), and the capture is extended to record
the ALPHA blend descriptor it was missing (the USER's "capture everything we're missing" pass). 5 commits;
998 host pass (+6). USER-CONFIRMED the town reconstruction looks correct.

**M4a — the STREAMING reader** (`src/osr_replay.{c,h}`, `ea38484`). A real capture is 1.5 GB / 16M blits and
the port is a 32-bit process, so the `.osr` CANNOT be slurped — it streams the framed records to an
`osr_replay_sink` visitor (truncated-tail-safe, host-tested). Validated against the REAL capture: a throwaway
host harness's per-type counts match `osr.py` byte-for-byte.

**M4b+M4c — the Win32 RECONSTRUCTOR** (`src/osr_recon.c`, `afb3e64`). SHEET → a DDraw source surface (loaded
TOP-DOWN — the capture grabbed a Lock, not a bottom-up DIB), FONT → an HFONT, BLIT → the matching zdd.c
primitive onto the dest, TEXT → real GDI `TextOutA`, PRESENT → a BMP snapshot. RAN on the real capture (0
no-sheet / 0 no-font). Two fidelity fixes: (1) the record's `ckey` is `colorkey_OUT` (already RGB565), so bind
it RAW via `zdd_surface_set_color_key` — going through `zdd_object_set_color_key` re-runs the RGB888→565
conversion and leaks magenta; (2) NO per-frame clear — retail flips a back-buffer chain, so an empty
re-present frame (quirk #99) must retain the prior pixels (prologue 0%→99.7%); the dest is cleared once at
start and accumulates.

**M4-alpha — capture + replay the blend descriptor** (`d22d456`). The `.osr` recorded only the blend MODE, so
every mode-4 ALPHA blit (~333k: the prologue narration, the town sky/ground, fades) reconstructed as black.
NEW `OSR_BLEND` record (mode + 3 channels {shift, mask, exact-sized LUT — sized to the max index
`zdd_blend_pixel` reaches}) + `osr_blit.blend_ref` (76→80 B); `tools/capture_proxy/blend_grab.h` grabs the
descriptor at the alpha detour. First cut captured ZERO — the descriptor is a HEAP object (≈0x069a…, not a
global), so the global-range filter + ptr-dedup failed; fixed with VirtualQuery-guarded reads (safe for
stack/heap, no fault in the trampoline) + CONTENT dedup (mode + per-channel shift/mask/lut-pointer, the LUTs
being stable globals). Re-captured nav→town: BLEND=38 distinct, 100% of alpha blits referenced, full turbo
(~944 fps). The reconstructor rebuilds a `zdd_blend_desc` → `zdd_blit_orchestrate` (0 alpha-skipped). Town
flip 1250 (USER-CONFIRMED) + prologue 1200 on the feed.

**NEXT — M4d the `--validate` differ_px==0 gate**: a real retail backbuffer snapshot from the proxy → diff vs
reconstruction. It both proves alpha pixel-correctness AND diagnoses the menu-panel CLIPPED-blit artifact the
USER flagged. Remaining capture gaps (flagged): scene-transition CLEARs (`OSR_CLEAR`); mode-2 rects
src_w/src_h. Then M5 (port emitter), M6 (the :8780 tick-join studio). Plan: `plans/trace-studio-v2.md`.

---

## 2026-06-12 (ckpt 125) — TRACE STUDIO v2 M3d: GDI text → TEXT/FONT (the `.osr` is now a COMPLETE frame)

The engine renders ALL dynamic text + the prologue narration through Win32 GDI `TextOutA` straight onto the
backbuffer DC — OUTSIDE the 5 DDraw blit primitives (`docs/findings/text-glyph-pipeline.md`, quirk #63). M3d
captures that last draw class, so the `.osr` is now a complete frame description (blits + sources + surface
identity + text) and M4 reconstruct is unblocked. 992 host pass (+2: `osr_font_roundtrip`, `osr_text_roundtrip`).

**Mechanism — IAT-patch, not a VA detour.** The engine imports `GDI32.dll!{TextOutA, CreateFontIndirectA,
SelectObject, SetTextColor, SetBkMode}` by name (objdump; no `ExtTextOutA`/`SetBkColor`). `engine_gdi.h` (NEW)
swaps those IAT slots via the existing `iat_hook.h`. Unlike the engine-VA INT3/E9 trampolines (onEnter-only),
an IAT swap is a FULL wrapper that calls the real function and SEES ITS RETURN VALUE — exactly what
`CreateFontIndirectA` (the new HFONT) needs, with no onLeave framework (the gap the held-axis leaf was
deferred for). Near-zero per-call cost, and only the engine's own calls are intercepted.

**State model (engine-thread-only, no locking).** A font cache HFONT→font_ref: `CreateFontIndirectA` interns
the returned HFONT and emits ONE dedup'd `FONT` record (the LOGFONTA the M4 replayer recreates). A tiny
per-HDC state table {font_ref, color, bk_mode}: `SelectObject` updates the current font (only when the object
is a KNOWN HFONT — it is also called with bitmaps/pens), `SetTextColor`/`SetBkMode` the colour/mode.
`TextOutA` emits a `TEXT` record with that state + position + string, the SHARED per-frame draw `seq`
(`eh_next_draw_seq`, so the replayer interleaves text and blits in issue order) and the single backbuffer
`dst_handle` (learned from the blit path — M3c found exactly one dst surface). `OSR_TEXT`/`OSR_FONT` are the
codec records in `src/osr_format.h` (host round-trip tested); `osr.py` decodes them (+ a `TEXTS` dump and
font/text coverage in `SUMMARY`).

**PROVEN** (fresh nav→game_enter capture): **9 FONT** records (all Courier New, h 8..20 — the
`ar_register_fonts` boot fonts) + **553k TEXT** records (font_ref 100% set ⇒ every TextOutA had a tracked
selected HFONT; dst_handle 100% set; 7 distinct colours; 12260 frames with text); all 3 anchors + both seed
pins + the BLIT/SHEET coverage byte-identical to M3c (no regression). The decoded text matches quirk #63's
retail ground truth EXACTLY: font ref 3 = Courier New 7×18, per-glyph `TextOutA` at a 7px advance, the 3-copy
shadow (shadow `0xa8b9cc` at (x,y+1) & (x+1,y), main `0x3e537d` at (x,y)), `bk=TRANSPARENT`, `dst=1` — e.g.
the new-game menu label "Allows you t…" laid out glyph-for-glyph at flip 671. Two M3c follow-ups
(`osr-sheet-compression`, `osr-sheet-dhash-xside`) stay tagged PORT-DEBT; neither blocks M4.

---

## 2026-06-12 (ckpt 125) — TRACE STUDIO v2 M3c: SOURCE pixels + surface identity captured (no COM wrap)

M3c gives the draw stream the two things reconstruction (M4) needs beyond geometry: each blit's DEST surface
identity (a stable `dst_handle`) and its SOURCE pixels (a dedup'd `SHEET`). The plan called this "the risky
COM vtable wrap" — but the blit decompiles (`docs/decompiled/by-address/5b9a40.c` etc.) showed every cel/dest
holds a real `IDirectDrawSurface7*` at `+0x2c` and the engine issues `dest->Blt(&dr, src, &sr, flags, 0)` via
vtable +0x14, so the proxy interns those RAW surface pointers and Locks the source straight from the blit
detour — NO surface wrapping, NO per-blit COM overhead, far lower risk than the planned wrap. Two commits;
990 host pass (+1, `osr_sheet_roundtrip`).

**The format (`src/osr_format.h`).** Added the variable-length `OSR_SHEET` record (a 24-byte prefix —
`dhash/res/frame/w/h/pitch/pixfmt/codec/byte_len` — then raw pixel bytes) + `osr_enc_sheet_prefix` so the
writer can stream a large pixel payload into its ring with a single copy. Host round-trip tested.

**The proxy (`tools/capture_proxy/`).** `surface_id.h` (NEW) interns a surface pointer → a small stable
handle. `sheet_grab.h` (NEW) Locks a source surface `DDLOCK_READONLY` on its first sighting, fingerprints it
with FNV-1a mirroring `src/asset_register.c`'s seed order (w/h/bitcount, then pixels), emits ONE dedup'd
`SHEET` per surface pointer, and caches the dhash for the per-blit hot path. `engine_pixfmt.h` (NEW)
classifies a `DDPIXELFORMAT` → `OSR_PIXFMT_*`. `engine_hooks.h` reads the dest surface (`*(void**)(arg0+0x2c)`)
→ `dst_handle` and re-stamps the `.osr` header pixfmt/screen from the first dest's `DDSURFACEDESC2`, and reads
the src surface (`*(void**)(cel+0x2c)`) → `SHEET` → BLIT `dhash`. `osr_writer.h` applies the header re-stamp
at offset 0 from the bg thread (the only non-sequential write). `osr.py` decodes `SHEET` (+ a `SHEETS` dump
and dst/dhash coverage in `SUMMARY`).

**PROVEN** (`m3c-verify` nav→town capture): the header re-stamped to `640×480 RGB565` (was UNKNOWN);
`dst_handle` 100% set with 1 distinct value (= the single backbuffer, correct for a 2D blit compositor); src
`dhash` 100% set; **496 SHEETs / 420 distinct dhash / 9.4 MiB raw RGB565**; all 3 anchors + both seed pins
byte-identical to M3b; 90% render-id-named; **912 fps in-game** (<4% under M3b's 950 — the grab is
once-per-surface, no Lock stall or crash). The captured sheets are coherent: 640×480 backdrop/scroll layers,
a sparkle particle series (22→20→18→14→12→10→8→6), 18× 160×176 portrait busts, 32×32 town tiles, tall 80×N
sprite columns.

**FOLLOW-UPS (PORT-DEBT, none block M3d/M4):** `osr-sheet-compression` (SHEET pixels are raw — miniz
deferred); `osr-sheet-dhash-xside` (the retail dhash mirrors the port's seed SHAPE but is computed over the
native-pitch/pixfmt locked surface, so it won't byte-match the port's packed decode — a legitimate
render_diff `[decode]` signal, with `(resource_id, frame)` remaining the primary cross-side join);
`osr-alpha-src-grab` (mode-4 alpha is a GDI/`paint_ctx` blend, its `+0x2c` source grab best-effort). **NEXT:
M3d** — hook `gdi32!TextOutA`/`ExtTextOutA` + font selection → TEXT/FONT records (the last draw class), then
M4 reconstruct.

---

## 2026-06-12 (ckpt 125) — TRACE STUDIO v2 M3b: native BLIT draw-stream captured (town PROVEN)

M3b records the ORDERED draw-call stream — the 5 source-bearing DDraw blit primitives + the render-id
identity per frame — natively via INT3+VEH detours (no Frida), so an `.osr` now carries the full draw list
per frame keyed to the load-stable `(resource_id, frame)` identity `tools/render_diff.py` aligns on. One
commit (`cc63407`). 989 host pass (+1, `osr_blit_roundtrip`).

**`src/osr_format.h` — the `OSR_BLIT` record.** A fixed 76-byte payload mirroring `render_diff.py`'s schema
(`va/seq`, `res/frame`, `dhash/dst_handle`, `dx,dy,reqw,reqh,sx,sy`, `ow,oh,ox,oy`, `state/ckey/bmode/mode`)
so the cross-side draw diff works unchanged; host-tested encode→decode (the Python reader struct.unpacks the
same layout). `dhash`/`dst_handle` stay 0 retail-side until M3c grabs surface pixels/identity.

**`tools/capture_proxy/render_id.h` (NEW) — the cel→(res,frame) registry.** The retail mirror of
`src/render_id.c` (and the Frida agent's `g_render_id_map`): an open-addressing table populated by the
resolver detour `0x418470`. The cel is the resolver's RETURN value, but it is COMPUTABLE at onEnter from the
decompile — `cel = *( *( *(int*)slot ) + (frame & 0xffff) * 4 )` whenever the bank is already decoded
(`*(int*)*in_ECX != 0`). On a bank's very first resolve the array is still 0 (the call lazily decodes it);
we skip that one and register the cel on its next resolve — banks decode early and cels persist, so steady
state is 89% named. This keeps M3b entirely on the proven onEnter-only `va_detour.h` framework — no onLeave
hook needed (the onLeave/return-value mechanism stays deferred to the held-axis leaf inject).

**`engine_hooks.h` — the 5 blit detours + the restructured flip.** Each blit VA's callback reads the source
cel (ECX for the 4 thiscall primitives; stack arg[1] for the cdecl alpha `0x5bd550`), looks up its
`(res,frame)`, reads the cel placement metrics (+0xb8/+0xbc/+0x0c/+0x10/+0xd4/+0x28), and the call geometry
off the stack per the decompiled arg conventions (RE'd from `docs/decompiled/by-address/5b9a40.c` … cross-
checked against the port's `zdd_emit_blit`). The alpha blend mode is read from the forwarded `0x5bd680`
thiscall `this` in ECX (best-effort, guarded). The flip hook is restructured from M3a's empty
FRAMEBEG+PRESENT-per-flip to FRAMEBEG-at-open / draws / PRESENT-at-flip, with a per-frame draw `seq` reset
at each FRAMEBEG. `osr.py` gains BLIT decode, a `BLITS <flip>` frame-draw-list dump, and per-VA / named%
stats in SUMMARY.

**PROVEN on a real boot** driven to game_enter (nav `runs/proxy-m2b/nav.jsonl`): 867k BLIT records over 2377
frames, 89% render-id-named, all 3 anchors at the exact M2b flips (newgame@652 / prologue@1000 /
game_enter@1242) + both seed pins. The town establishing shot (flip 1250, 1815 blits) decodes coherently —
res=1002 backdrop columns (8×80px = 640 wide), res=1722/1082 KEYSRC parallax layers, res=2234 `clipped`
32×32 sub-tiles drawn at the camera scroll offset (dst x=-32) with the proper src sub-offsets, ckey 0xf81f
(RGB565 magenta) — matching the documented town render path (`findings/ddraw-blit-trace.md`).

**PERF FORK (measured, plan §M3b) — RESOLVED, FULL TURBO.** Title/menu ~2400 fps WITH capture; the in-game
town was **~25 fps** first cut — the INT3+VEH cost (2 exception dispatches/blit + `detour_patch_byte`'s 2×
`VirtualProtect`/`FlushInstructionCache` per patch, ×2 patches/hit) at ~1500 blits/frame. Two chips closed
the gap (both this session):
- **RWX pages (`ee55e5b`):** make each hooked page permanently `PAGE_EXECUTE_READWRITE` once at install
  (`detour_make_rwx`) so the hot INT3 dance is just a byte write + flush — no per-patch `VirtualProtect`.
  ~25 → **~56 fps** (2.2×), integrity unchanged.
- **E9-jmp trampoline (`50ec26b`, USER-chosen, the real fix):** `tools/capture_proxy/trampoline.h` replaces
  the 6 HOT hooks (resolver + 5 blits) with inline 5-byte `E9` jumps — ZERO exceptions/hit. Per hook a
  thunk (`pushad`/`pushfd` → push `entry_esp`+`ecx` → `call cb` → `popfd`/`popad` → `jmp relay`) + a relay
  (the relocated head bytes → `jmp va+head_len`) live in one RX arena; the VA is patched `E9 -> thunk`. No
  length-disassembler — each VA's head bytes are HARDCODED from the unpacked exe (instruction-aligned,
  head_len ≥ 5, no rel jmp/call in the relocated span — disasm-verified). The hot callbacks take the light
  `(ecx, entry_esp)` signature; the rare hooks keep INT3+VEH. ~56 → **~950 fps** (sustained over 28k flips,
  full turbo). A 30 s run now captures 29k frames / 14.6M blits, 91% named, geometry byte-identical to the
  INT3 baseline (the `entry_esp` math is exact). No crash on the real game.

---

## 2026-06-12 (ckpt 125) — TRACE STUDIO v2 M3a: native `.osr` draw-stream writer (cheap records) PROVEN

M3 of the v2 plan begins, sliced by risk. M3a is the lowest-risk, highest-leverage first cut: the shared
`.osr` capture format + the proxy-side background-thread ring writer, emitting the cheap records the boot
already produces (FRAMEBEG / PRESENT / ANCHOR / SEED) — no COM wrap yet, so ZERO game-crash risk. One commit
(`8c42c02`). It proves the format, the writer, and the Windows-FS storage discipline end-to-end on the real
game before the riskier draw-capture pieces land.

**`src/osr_format.h` — the shared codec.** Pure-C, header-only, no Win32 (so it links into the host suite +
both targets). A fixed 64-B header (magic `OSR1`, version, side, pixfmt, screen, seed, flags, scenario) then
a stream of framed `{u32 type, u32 len, payload}` records. Little-endian (x86 only). Append-only +
self-describing: a reader skips an unknown type by len, and a run HARD-KILLED mid-record (the harness
Stop-Processes the game) is still valid up to the last complete record (`osr_rec_next` stops cleanly at a
torn tail). M3a implements FRAMEBEG/PRESENT/ANCHOR/SEED; the bulky/draw record ids (CLEAR/BLIT/TEXT/SHEET/
FONT/PALETTE/INPUT) are reserved for M3b+.

**`tools/capture_proxy/osr_writer.h` — the bg-thread ring.** The engine main thread, inside the INT3+VEH
detour callbacks, only ever locks + memcpy's a small encoded record into a double buffer; a dedicated
background thread swaps the buffers under the lock and drains the inactive half to the `.osr` file every
~30 ms, fflushing after each drain. So disk latency never stalls the engine (the plan's hot-path rule), and
because the harness hard-kills the game (no clean shutdown), whatever is on disk at kill time is a valid
`.osr` up to the last drain (≤30 ms lost). The file lives on native NTFS (`C:\oss-osr\retail.osr`), never
the WSL 9p mount. Records wired: FRAMEBEG+PRESENT at the flip hook (the deterministic tick-join axis), ANCHOR
at the newgame/prologue/game_enter anchors, SEED at the title pin + the per-map re-pin. New config:
`OSS_OSR` / `OSS_OSR_PATH` / `OSS_SCENARIO`; `run_proxy.sh` stages the path and prints the `osr.py` summary.

**`tools/trace_studio2/osr.py` — the reader/validator.** Mirrors `osr_format.h` exactly (kept in lockstep);
`SUMMARY` prints the header + frame count + tick range + anchors + seeds + a torn-tail note, `FRAMES` dumps
per-frame `(flip, sim_tick)`. The M3a round-trip gate.

**PROVEN on a real boot** (`OSS_SCENARIO=intro-1 run_proxy.sh 15 runs/proxy-m2b/nav.jsonl`): `retail.osr` =
417 KB, 11585 frames (flip 1..11585 / sim_tick 0..10358), all 3 anchors at the EXACT M2b flips (newgame@652,
prologue@1000, game_enter@1242), both seed pins (@432 title, @1242 re-pin → 0x4f5347), no torn tail; `osr.py`
round-trips it cleanly. Throughput held at ~800 fps WITH capture (813/804/800 at flips 6k/8k/10k) — NO
regression over the ~790 fps M2 baseline, confirming the design thesis: the hot path logs calls, not pixels.
6 host tests (988 pass, +6): header + record round-trip, bad-magic reject, empty-anchor-name, truncated-tail
recovery.

**NEXT — M3b: the BLIT op stream** (detour the 5 blit VAs + resolver `0x418470` → BLIT records, layout
finalized = `render_diff.py`'s schema; restructure flip → FRAMEBEG-at-open / draws / PRESENT-at-flip). The
known fork: blit VAs fire hundreds×/frame and the INT3+VEH detour pays 2 exceptions/call — first cut on the
proven framework, measure fps, build the plan's E9-jmp trampoline only if prohibitive. Then M3c (COM wrap +
SHEET pixels, the risky piece), M3d (GDI text). Plan + sub-milestones: `docs/plans/trace-studio-v2.md`.

---

## 2026-06-12 (ckpt 125) — TRACE STUDIO v2 M2b: native engine-VA detours + ring input → BOOT TO game_enter

M2 of the v2 plan completes — the native proxy now does everything the Frida agent did to STAND UP a
deterministic boot, with no Frida and no injector. Two commits: M2b-1 (`16f7977`) the detour framework +
observe hooks, M2b-2 (`2ce391c`) ring input injection.

**The detour framework (`tools/capture_proxy/va_detour.h`) — INT3 + a vectored exception handler, no
length-disassembler, no vendored lib (~120 lines).** Patch a function's first byte to 0xCC; a
first-priority VEH catches `EXCEPTION_BREAKPOINT` at our VAs, runs the registered onEnter callback (which
reads thiscall ecx/edx + `[esp+4]` args and writes engine globals off the `CONTEXT`), then resumes via the
classic restore-byte / rewind-Eip / set-trap-flag / single-step / re-arm dance (the real op executes IN
PLACE, correct even for rel call/jmp). A callback can request a permanent one-shot disarm. The fixed base
0x400000 + stripped relocations means zero base math; the VEH passes through every non-ours fault.

**The hooks (`engine_hooks.h`) — faithful port of the agent's VA map:** flip `0x5b8fc0` (flip++ + the
lockstep clock advance + a throughput heartbeat), sim-tick `0x43d1d0` (the deterministic index, reset at
game_enter), one-shot title seed-pin `0x56c070`→`DAT_008a4f94`, the newgame/prologue/game_enter scene
anchors, and the per-map RNG re-pin `0x41f200` (armed at game_enter, latched). **Ring input
(`engine_input.h`):** hook the poll consumer `0x43c110` (thiscall, ecx = the current scene's
input-manager, re-read every poll so the difficulty SUB-menu's distinct manager gets the presses), read the
engine's cached `now` from `[esp+4]`, and write `{id,ts,flag=1}` records into the top ring slots
(63,62,…=newest) of mgr+0x0c. Trace = the existing JSONL nav shape via a tolerant scanner (`OSS_INPUT_TRACE`,
a `C:\` path), flip-keyed, fire-once. `clock.h` grew to install the GetTickCount/pump hooks when turbo OR
lockstep (enabling the agent's --no-turbo --lockstep mode).

**PROVEN live (no Frida):** seed PINNED @flip 432; with a cadence-tolerant nav, newgame_enter @flip 652 →
prologue_enter @flip 1000 → game_enter @flip 1242 (RNG re-pin `0x8fafd240`→`0x4f5347` fires there), then
sim_tick climbs ~1:1 with flips (16773 @flip 18000 = lockstep engaged in-game) at **~790 fps turbo — vs
v1's ~60fps `--no-turbo` capture cap**. The order-of-magnitude throughput win the v2 thesis predicted is
real. NAV LESSON: a nav with EXACT flip frames is calibrated to one boot cadence (the agent's); the proxy's
differs, so the ckpt-122 flip-keyed nav stalled at the submenu — a window-spread nav reaches game_enter
robustly (fine for a boot, where game_enter re-pins the seed). DEFERRED: the held-axis leaf inject
(`0x5ba520` needs a return-value override) until freeroam capture needs it. Launcher
`tools/capture_proxy/run_proxy.sh` deploys → runs → collects → ALWAYS cleans up ddraw.dll (so v1 Frida runs
sharing the game dir never load the proxy). NEXT: M3 — the `.osr` draw-stream capture (DDraw COM vtable
wrap + the 5 blit-VA detours + GDI text hooks + the dedup'd/miniz writer).

---

## 2026-06-12 (ckpt 125) — TRACE STUDIO v2 design + M1: the native capture-proxy vector is PROVEN

The USER asked to rebuild the trace studio in a radically faster/richer way before resuming the freeroam
port: capture the **draw-call stream** (DDraw blits + GDI text + state, dedup'd source surfaces) rather than
pixels, and **reconstruct the frame 1:1 on Windows** (where real GDI is available — this dissolves the
offline-GDI-text blocker). Everything heavy runs Windows-side; WSL only orchestrates + reviews. Full design:
`docs/plans/trace-studio-v2.md` (capture-vs-replay fork RESOLVED with the USER; tick-join convergence
discipline; the `.osr` format; the incremental build order M1→M7; **storage discipline — all hot-path writes
land on native NTFS `C:\`, never the WSL 9p mount**). v2 is built in **isolation** — it does not touch v1
(`tools/trace_studio*`, `tools/frida_capture.py`, the Frida agent); v1 is archived only once v2 reaches
parity.

**M1 LANDED — the proxy `ddraw.dll` auto-loads into the real retail game with NO Frida and NO injector.**
The retail exe imports exactly one DDRAW symbol (`DirectDrawCreateEx`) and has a fixed ImageBase `0x400000`
with **relocations stripped** (verified `objdump -p`), so a proxy `ddraw.dll` dropped next to the exe wins
the DLL search order and loads automatically; later milestones inline-detour engine VAs with zero base-fixup.
`tools/capture_proxy/` (`ddraw_proxy.c` + `.def` undecorated stdcall exports + Makefile, built by the
existing mingw32 → `build/ddraw_proxy.dll`) forwards `DirectDrawCreateEx`/`DirectDrawCreate` to the real
system ddraw (loaded by absolute SysWOW64 path to avoid self-recursion). Live-proven on the real game:
`DLL_PROCESS_ATTACH pid=23032`, process boots clean. Launch mechanism nailed: **PowerShell `Start-Process
-WorkingDirectory`** (no exec bit, no Frida, no Steam).

**M2a LANDED — a fully native, Frida-free, HEADLESS TURBO BOOT.** `tools/capture_proxy/` grew the IAT patcher
(`iat_hook.h`), clock virtualization (`clock.h` — `GetTickCount`/`Sleep`/`WaitMessage`/`PeekMessageA` hooked,
turbo + lockstep model + pump-entered latch, faithful port of the agent's `installTurboHooks`), env-driven
config (`proxy_config.h`), and the harness thread (`harness.h` — window-hide, `WM_ACTIVATEAPP` keep-alive,
launcher-dialog auto-dismiss). Live-proven: config loaded → clock hooks installed → harness dismissed the
dialog (clicked "Launch") → `DirectDrawCreateEx -> hr=0` reached in **~1 s**, then `pump entered → clock
virtualization armed`. **No Frida, headless, turbo.** Two benign gaps: the exe imports neither
`user32!GetMessageA` (uses `PeekMessageA`) nor `winmm!waveOutSetVolume` (audio is DSOUND — silent-audio IAT
clamp is a no-op; TODO proxy DSOUND or find the launcher's sound control). NEXT: M2b — the engine-VA
inline-detour layer (INT3 + a vectored exception handler; no length-disassembler) for the flip counter
(`0x5b8fc0` → lockstep advance), sim-tick (`0x43d1d0`), anchors (`0x56c070`/`0x59f2c0`/…), seed pin
(`DAT_008a4f94`), and input injection (`0x43c110` ring / `0x5ba520` held leaf).

## 2026-06-11 (ckpt 124) — the dialogue PORTRAITS are un-MVP'd: the bust resolves per speaker

A USER-requested side-fix off the ckpt-123 chain landing: the dialogue portrait was hardcoded to
`g_ar_sprite_slots[663]` — which, the RE now shows, is a WRONG character (the face table's head 100000104),
so every speaker (Arche/Father/Mother) showed the same bust.  New `src/portrait.{c,h}` ports the `0x49d6e0`
face-table lookup: `portrait_resolve(head_state, face_id, variant)` scans the embedded `DAT_006b6568` for
the `(head, face)` record → the portrait pool-slot (the `g_ar_sprite_slots`/`DAT_008a760c` index; -1 = no
record = retail's no-portrait path).  `cutscene.c` maps the speaker → head-state + resolves the slot per
line; `dialogue_box` carries `portrait_slot` (reset -1 by `dialogue_arm`); `main.c game_render_dialogue`
blits it with the existing cross-fade.  982 host pass (+4); commit `ce1af81`.

**Ground-truthed by Frida (`runs/portrait-gt`, spec `tools/flow/portrait_face_fields.json` on `0x49d6e0`).**
Two unknowns the decompile couldn't pin: (1) the speaker→head-state key — captured Arche (code 0xc35a) =
100000101, Father (0xc3dc) = 100000211, Mother (0xc440) = 100000212 (constant per character); (2) the
variant column — `bVar4`/var-C (the old MVP's pick) is FALSE for the town dialogue (`in_ECX+0x2f0`==0
captured), and the A (`+0x8`) vs B (`+0xa`) choice is the speaker's DYNAMIC body-facing (`body+0x2c`==3 → A;
read the resolved `+0x84` via a 1-line-lag trick — the same speaker+face flipped A↔B as the cast turned,
tally A=8/B=10).  The port's cutscene cast is STATIC, so it uses B (the default path + plurality).  The
face table is embedded by `tools/extract/portrait_face_table.py` → `src/portrait_face_data.{c,h}` (147
records, the `world_tables_data` precedent — RE'd mapping only, never assets).

**Verified** (replay, `runs/portrait-gt/verify`): the Father (green shirt), Arche (headband girl), and
Mother (brown-haired woman) busts each render correctly per line (montage on the feed).  Retires
PORT-DEBT(dialogue-portrait-per-speaker).

**ALIGNMENT fix (commit `1a527cb`, USER-flagged + USER-CONFIRMED).** The first pass used one variant column
(B), but the face table's 3 variants are DIFFERENT busts/SIZES (Father A=676 160x176 vs B=683 176x144),
picked per line by the speaker's body-facing (`0x49d6e0:143`) — so Father line 1 rendered as the squished
176x144 crop (a misaligned overlay) where retail uses the 160x176 bust.  RE'd the per-line variant for all
18 arrival+house lines by harness-reading the RESOLVED `+0x84` off the beat-runner thunk `0x439680` (no lag,
no loss at the room transition — `runs/portrait-gt` cap4): arrival A,B,B,B,B,B,B,B,A,B / house
A,A,A,A,A,A,B,A, baked into `cutscene_line.pvar`.  Father L1 now matches retail f2980 (port|retail montage
on the feed); USER confirmed the portraits look correct.  `dialogue-portrait-facing` reduced to "`pvar` is
captured data, not yet derived from a live cast facing".  OPEN (deferred, USER): a frame-by-frame
TRACE-STUDIO pass to verify the exact expression/pose per line (the ad-hoc comparison frames were not
sim-tick aligned — the portrait cross-fade blends two busts).  The planned next arc is unchanged: render
the house/errands ROOMS (`plans/controllable-arche-faithful.md` Phase 2a).

---

## 2026-06-11 (ckpt 123) — the town-intro CUTSCENE CHAINS arrival→house: the room-key swap ported + behaviorally verified

The ckpt-122 harness-verified chain (quirk #103) starts landing in code.  `src/cutscene.{c,h}` grew from a
single-script driver to a multi-ROOM sequencer that walks a ROOMS list modeling the room-key swap: retail's
`0x401d40` stages the next key to map+0x900/4/8, `0x402030` commits it to `room_state+0x4024`, and `0x4d7d80`
re-dispatches on the new key (the case `return 2` path).  The port collapses that to advancing a chain — on a
room's last line it commits the next room key (`room_idx++`) and arms its line 0; past the last room it
completes.  Chain: arrival `0x334be` (10 lines) → house `0x334c8` (8 lines) → ENDS at the errands boundary
`0x334dc` (= the freeroam control hand-off point).  978 host pass (+4); commit `daa1f65`.

**The house script** (RE'd from `0x4d7d80` case 0x334c8, decompile lines 1029-1218): 8 dialogue lines
(`0x49d6e0` calls), text VAs 0x86d390..0x86d1dc, all unvoiced.  Speakers are actor ids resolved to dramatist
names — 0x5f5e165=Arche, 0x5f5e1d3=Father, 0x5f5e1d4=Mother — confirmed against the arrival's known speakers
(same `0x556eb0(id)` actors).  Order Arche/Arche/Mother/Father/Father/Father → [emote `0x401e60`, skipped] →
Arche/Mother.  **Verified behaviorally** (`runs/cutscene-verify`): a seed-pinned replay (extended Z-spam)
drove the live chain through all 18 lines and logged "cutscene chain COMPLETE → errands boundary 0x334dc"
@flip 11365; captured house frames render the correct text + name.  Known deferred (tagged): the portrait
stays the Father bust (`dialogue-portrait-per-speaker`); the backdrop is still the town scene — **new
`PORT-DEBT(cutscene-room-render)`** (the room key drives the unported `0x585ae0`/`0x586010` map-load path).

**Reshapes the plan / NEXT (a scope decision pending with the USER).** Reading the errands dispatcher
`0x4dc510` (21 KB) showed `0x334dc` is a flag-gated QUESTLINE (its own dialogue API `0x4a5ee0`, multiple
sub-scenes), not a linear cutscene — the errands room IS gameplay/freeroam.  And neither the house nor errands
ROOM is rendered.  So the freeroam hand-off (`character_step` on live `axis_held`, the `+0x200==0` char-AI
path — the mover + live input are done) is only NON-FAKE once the errands room RENDERS; otherwise Arche walks
over the town backdrop (the ckpt-120 "wrong scene" the USER removed).  Recommendation: the room-render path is
the faithful foundation (fixes the house backdrop AND unblocks a real freeroam); the hand-off follows.  Also
corrected the ledger: `0x439690`/`0x49d6e0` were over-claimed as `FUN_`-ported in the ckpt-121 cutscene.c (the
sequencer reduces a slice + captures args, not a port) → bare VAs (touched 209→207).

---

## 2026-06-11 (ckpt 122) — the control-transfer PATH is HARNESS-VERIFIED: a 3-room chain, the errands room IS the freeroam, control = `+0x200==0`

The ckpt-121 "DO FIRST: harness-verify the live room/control path before porting" step, done — and it
RESOLVES the static-vs-live conflict (both prior readings were half-right) and CORRECTS the porting model.
Pure RE/harness, no port code (974 host pass unchanged).  Method: seed-pinned `--lockstep --no-turbo` retail
drove the proven ckpt-112 nav (Z-spam from `game_enter`, incl the two `id=3` LEFT presses @830/865 that pass
the new-game submenu — a uniform-Z nav stalled before `game_enter` without them) extended to flip 8392, under
a new per-Flip field spec (`tools/flow/control_handoff_fields.json`) reading off
`room_state = *(*(0x8a9b50)+0x2784)`: the committed room key `+0x4024`, the leader `+0x200c`→entity, the
control flag `entity+0x200` + input-mgr `+0x158a4`, the story-flag table `+0x40ac`.  Artifacts:
`runs/control-path-gt/` (gitignored); montage on the feed.  Full writeup: **engine-quirk #103** (corrects #101).

**Findings:** (1) the path IS a 3-ROOM CHAIN — the room key swaps `0x334be` arrival (flip 1430) → `0x334c8`
house (3661) → `0x334dc` errands (4270), staged by `FUN_00401d40` (@3659/@4268), committed by `FUN_00402030`
(the static RE's sequence is real).  (2) But it's a LIGHT room-key swap, NOT a full reload — ONE `game_enter`,
and `room_state`/leader/entity/flag-table pointers hold constant across both swaps (so ckpt-112's "no reload,
entities persist" was right; its "same scene" was wrong).  (3) CONTROL IS `entity+0x200 == 0` (the char-AI
path `0x478ba0`), NOT `+0x200=1` — a held-axis walk in the errands room drove Arche bit-exact (held-RIGHT
`wx 19200→73800` facing 1, held-LEFT →`14640` facing 3) with `+0x200`==0 + `+0x158a4` non-null throughout;
the ckpt-114 polarity open is resolved, and the `0x41e070`/`0x4c6830` `+0x200=1` setters are a later/different
control point.  (4) the errands room `0x334dc` IS the freeroam ("PLAYER!" marker + HUD on screen = ckpt-112's
"PLAYER!@4500", correctly located) — USER-confirmed "a house with mom and dad, run some errands, short dialogue
at the start"; Z-spam stalls there because it's gameplay, and the stall IS the control boundary.

**Next (the active arc — PORT the verified chain):** extend `src/cutscene.{c,h}` to chain the 3 room scripts
via the light room-key swap (`0x401d40`→`0x402030`, no full reload), and at the errands room stop the
sequencer + run `character_step` on live `axis_held` (the `+0x200==0` char-AI path) — the faithful replacement
for the ckpt-120 `CHAR_CONTROL_ARM_FRAMES` MVP arm.  Drop the `+0x200=1` model for the first freeroam.  Plan
corrected: `plans/controllable-arche-faithful.md` "Phase 2 VERIFIED".  Open (don't block): the later
`+0x200=1` transfer (post-errands → Sana, needs a walk-to-trigger nav); whether the char-AI is suppressed
during the arrival/house cutscenes or merely un-fed.

---

## 2026-06-11 (ckpt 121) — UN-MVP'd the movement: MVP wire REMOVED, FAITHFUL live input + the town-arrival DIALOGUE ADVANCE ported

The USER's directive: the ckpt-120 live-wire put control in the wrong scene via a measured trigger — un-MVP
it.  Three of four steps landed (4 commits direct to master; 974 host pass, +11, 0 fail).  **Step 1**
(`42e0fc1`) removed the throwaway scaffold from `src/main.c` (the `g_arche`/`CHAR_CONTROL_ARM_FRAMES`
measured-frame wire); the bit-exact mover `character.{c,h}` is untouched.  **Step 2** (`61a6aaa`) ported
FAITHFUL live keyboard input — new `src/input_live.{c,h}` (the port of the per-frame producer `0x46a880`):
real keys fill `input_mgr.axis_held[0..6]` (clear-then-set) + post discrete ring EDGES each frame; a single
`main.c feed_input` gates REPLAY (the deterministic parity path) vs LIVE (interactive, focus-gated on
`g_app_active_flag`), mutually exclusive so live keys never perturb a capture.  Movement is now testable
with a real keyboard (USER windowed verify pending: title menu navigable with arrows + Z).

**Step 3** (`8d4c096`) ported the town-arrival DIALOGUE ADVANCE — new `src/cutscene.{c,h}`, a port-side
reduction of `0x4d7d80` case 0x334be (lines 33-292) + the beat-runner `0x439690`.  The port was stuck on
dialogue line 1; now it sequences the real 10-line family conversation, Z-advancing Father→Arche→Mother
(each line's text+name read from the user's exe by VA — dramatist rows 0/4/5; Z advances only when the line
is fully typed = the beat-completion gate `0x439690:1004`).  Replay-verified end-to-end
(`runs/cutscene-verify`: game_enter@1116 → cutscene_arm@hold 1283 → f2600 "Ahh…" → f3150 "Yay" → f3700 "We
haven…"; montage on the feed).  Retires PORT-DEBT(dialogue-line-table); new debt dialogue-portrait-per-speaker
(the portrait stays the Father bust — a deferred render detail).

**Step 4 — the full faithful CONTROL TRANSFER — is the active arc (USER-committed).**  RE (two
general-purpose subagent maps of `0x4d7d80`/`0x401d40`/`0x41e070`) found the hand-off is a MULTI-ROOM CHAIN,
not a few lines: `FUN_00401d40` does a ROOM TRANSITION (arrival → load room 0x334c8 house → load room
0x334dc errands via the SEPARATE dispatcher `FUN_004dc510`, which advances story-flag `0x5f76805`→0xd2 →
back to town 0x334be flag-0xd2 → the Sana-walk-home scene → `entity+0x200=1` at `4d7d80.c:449-463`, the
inlined `0x41e070`/`0x4c6830` idiom).  This CONFLICTS with ckpt-112's live "one game_enter, no map reload" —
so the next move is to **harness-verify the live room/control path FIRST** (field-spec the scene-controller
room key `*(0x8a9b50+0x1038)[0]` + Arche's `+0x200` + flag `0x5f76805`, Z-spam retail to the hand-off) before
porting the room-transition system + the intervening rooms.  Plan rewritten:
`docs/plans/controllable-arche-faithful.md`.

---

## 2026-06-11 (ckpt 120) — PHASE-4 chip 3c: the LIVE WIRE — Arche is CONTROLLABLE ON SCREEN (the movement-system milestone)

`character_step` got its FIRST live caller.  The chip-3a/b character mover (walk/run/jump/windup, all
host-validated bit-exact) now drives Arche's rendered sprite live: held-axis input walks her around the
settled town.  `src/main.c` wiring only (963 host pass, unchanged).  USER visual-verify pending — montages
pushed to the feed (`runs/livewire/arche_walk_scene.png` + `arche_walk2_montage.png`).

The wire mirrors the butterfly pattern.  Arche is the cutscene-cast EFFECT actor (code `0xc35a`, resolved
by scanning `g_effects` after `actor_spawn_cutscene_cast` — slot 18, body bank 0x8b, world 41600/45600,
facing 1, idle clip).  At a MEASURED control-transfer frame (`CHAR_CONTROL_ARM_FRAMES`=200 flips past
game_enter; PORT-DEBT(char-control-trigger), since the port's dialogue does not yet drive the real
`entity+0x200=1` hand-off), `game_actor_update` (already sim-tick-gated) `character_init`s `g_arche` from
her settled render pos and each sim-tick runs `character_step(&g_arche, m->axis_held, m->axis_held[4],
/*run=*/0)` (`m`=`g_game_drive.input`; `axis_held[0..3]` align with `CHAR_AXIS_*`, `[4]`=jump C), then
mirrors `world_x/world_y/facing` into her render-state — exactly as `butterfly_step` mirrors into the
EFFECT actors.  The port has no live in-game keyboard producer (WM_KEYDOWN is a no-op), so input is the
`held_trace`/`input_trace` REPLAY — the same deterministic path walk/jump/dash were all validated on.

Verified by capture (`runs/livewire`): drove the port to the settled town (nav `edit.trace.port.jsonl`,
game_enter@1116; the scripted camera pans LEFT from cur_x 128000 to 12800, settling ~flip 2097 with Arche
~45% across frame), then a held-LEFT-then-RIGHT trace — Arche walks left to Barnard (world ~27600) then
back right, smooth accel/decel, no glitch.  KNOWN DEFERRED (render polish, NOT the mover): she SLIDES on
the idle clip (no walk-cycle) and stays RIGHT-FACING when walking left — the render's `facing==3` mirror
selects a pre-mirrored FRAME (`frame_base + flip_table[0x8b]`), and bank 0x8b has no mirror-frame
registered; both are PORT-DEBT(cutscene-party-chars) (the multi-part party-band render).  The `facing`
data is written correctly, so the animated render will mirror with no further wiring.  NEXT (USER's call):
render polish (directional/walk frames + party band, retires cutscene-party-chars) / dialogue chip 4 (the
real hand-off + live ring jump-dash) / collision wiring (real terrain).

---

## 2026-06-11 (ckpt 119) — PHASE-4 chip 3b: the jump WINDUP is PORTED + bit-exact (the 4-tick launch-anticipation delay)

Ported the jump WINDUP into `src/character.{c,h}` and validated it BIT-EXACT — no fresh capture needed,
the EXISTING `runs/runjump-gt/capjump-ring2` already had the ground truth in its `bstate` field.  963
host pass (+1: `character_jump_windup`).  The jump execute enters the airborne state IMMEDIATELY but the
body stays STATIONARY for exactly **4 sim-ticks** (a visible launch crouch, ~8 flips) before the impulse
fires — the launch lag that the earlier arc port collapsed to an instant launch.

The law is decompile-decisive (`0x442a70:834-841`, case 3 sub-state 0): the execute `cmd[2]==7` calls
`0x426f50(body,3)` which sets `body+0x38`=3 (main state), `+0x3a`=0 (sub), `+0x3c`=0 (counter) — a 3-write
setter, so the count is independent of prior state.  Then case 3 sub 0 runs `counter++; if (4 < counter)
{ vvel := in_ECX[0x5667]; sub := 1; counter := 0 }`: counter 0→1 on the entry tick, 1→2, 2→3, 3→4 (all
stationary, vvel 0), then 4→5 (`4<5`) → impulse.  Four stationary windup ticks, launch on the fifth.

The ground truth: `capjump-ring2`'s `bstate` reads `body+0x38` as u32 = main | sub<<16.  Decoding the
first jump's raw flips: **4602-4609 = (main 3, sub 0, vvel 0)** = the 4 windup ticks (8 flips = 4 sim-
ticks; the body updates every 2 flips), then **4610 = (main 3, sub 1, vvel −76000)** = the impulse.  The
earlier `jump_arc.py` keyed on `vvel!=0`, so the windup was invisible to the arc extraction but was always
in the data.  The port added `jump_sub`/`jump_ctr` (mirror `body+0x3a`/`+0x3c`) + the windup branch
(`CHAR_JUMP_WINDUP_THRESH`=4); the real sub-states 1/2/3 (transient/rise/fall anim bookkeeping) collapse
to the port's vvel-sign branch and the main-state-4 landing recovery is subsumed by the flat ground clamp.
`test_character_jump_windup` asserts the entry/windup/launch ticks bit-exact; the short-hop and held-rise
arc tests got the windup prefix and still pass.  This retires the windup half of
PORT-DEBT(char-jump-variable-height) (the remaining half = the town-ceiling apex clamp = char-collision-mover).
Writeup: engine-quirk #102 (the windup bullet).  NEXT: the LIVE wire (chip 3c, the milestone — `character_step`'s
first live caller at the freeroam hand-off → USER visual-verify).

---

## 2026-06-11 (ckpt 118) — PHASE-4 chip 3b: Arche's DASH (run) is PORTED + field-exact, validated bit-exact vs a fresh ring-double-tap capture

Ported the RUN (dash) into `src/character.{c,h}` and validated it BIT-EXACT against retail's per-tick
body (`runs/runjump-gt/capdash2`).  962 host pass (+1: `character_run_ramp`).  The run is a small,
fully-understood delta on the chip-3a walk — the SAME `body+0x28` accumulator and the SAME `0x445db0`
clamp-ramp, differing only in two captured per-entity consts: the cap `in_ECX[0x5664]`=**48000** (dwx
±480 = 2× the walk's ±240) and a TWO-PHASE accel `in_ECX[0x565d]`=**3200** while `|hvel| < 24000` (the
walk cap), then the walk accel 1600 up to 48000.  `character_step` grew a 4th arg `run` (the resolved
cmd[0]==5/6); the existing 6 character tests got `run=0`.

The chip-3b RUN BLOCKER (ckpt 115/116: "the dash double-tap didn't fire") is CLEARED.  `0x478ba0` sets
cmd[0]=5/6 when the AI's `0x479e70` matches two direction ring events (id 2=LEFT/4=RIGHT) in the window
`*(*0x8a6e80+0xf8)`; `0x479e70`/`0x479960` mark found events BY SLOT INDEX (not timestamp), so injecting
`ids:[4,4]` (two id-4 ring events → slots 63/62, same ts) IS a valid double-tap, and the held RIGHT axis
sustains it (`local_608[0]==6` while held).  The detection doesn't consume the events and they linger a
few flips (≪ the window), so the timing is forgiving.

Capture method lesson: the first attempt (`capdash`, a 2-VA field spec + the held-leaf hook) STALLED —
`sim_tick=0` the whole run, freeroam never reached (the heavy hooking tripped the lockstep stall-breaker
/ dialogue desync).  The clean run (`capdash2`) used a LEAN 1-VA `dash_fields.json` + the PROVEN
`runs/freeroam-walk` nav: it reached freeroam and the dash fired (cmd0 2→6, `hvel` 0→48000, `wx`
+480/tick), giving the bit-exact target.  Fewer hooks + a known-good nav when a capture must drive deep
under lockstep.  The run LAW comes from the `0x442a70` case-0x75 run branch (`:998` selects the run accel
while `hvel < param_3`=the walk cap, then reassigns the cap to 48000 at `:1001`) + the live const band;
the brake stays the walk −800 and releasing the dash decays 48000→24000 at −800 via the `0x445db0`
over-cap path.  The double-tap DETECTION is deferred to the live wire → PORT-DEBT(char-run-trigger).
Writeup: engine-quirk #102 (the run-physics bullet); model `docs/plans/movement-system.md` chip 3.

## 2026-06-11 (ckpt 117) — PHASE-4 chip 3b: Arche's JUMP is PORTED + field-exact, and the move-tuning consts are CAPTURED LIVE (resolving a decompile mis-read)

Ported the vertical airborne integrator into `src/character.{c,h}` (the chip-3a walk module), bit-exact
to the ckpt-116 captured SHORT-HOP arc, and CAPTURED the per-entity move-tuning constants live off
Arche's entity to RE the model rather than curve-fit it.  960 host pass (+2: `character_jump_arc`,
`character_jump_edge_and_ground`).  `character_step(c, axis_held, jump_held)` now runs the vertical
integrator every tick alongside the horizontal walk: launch impulse on the jump rising edge, then
`worldY += vvel/100` with ASYMMETRIC + VARIABLE-HEIGHT gravity and a flat ground clamp (the open-air
reduction of `0x442a70` case 3 + the vertical collision mover `0x54e5c0`).  The 27-tick short-hop arc
asserts bit-exact vs retail's captured `(vvel, worldY)` bytes.

The RE was decompile-STRUCTURE + live-CONSTS, not line-by-line.  A first pass read the 12 KB shared
integrator `0x442a70`'s decompile literally and mis-mapped the gravity/terminal to `in_ECX[0x565b]`/
`[0x565e]` — but a live const capture (`tools/flow/jump_consts_fields.json` → `runs/runjump-gt/capconsts`,
reading `in_ECX[idx]` = entity byte `idx*4` @flip 1500) showed those are the WALK cap (24000) and brake
(−800), CONFIRMING the ckpt-115 walk-tuning hypothesis.  The real jump consts: impulse `[0x5667]`=−80000,
rise grav HELD `[0x5668]`=2000 (the floaty high jump), rise grav FREE `[0x5669]`=8000 (the short hop), run
accel `[0x565d]`=3200.  A wider band scan (`capband`, `0x565a..0x566f`) showed the fall grav (4000, arc-
pinned) is NOT a move-tuning field → a global/derived gravity (4000 = 8000/2).  The decompile's
variable-reuse across vertical/horizontal terms is exactly why the project ground-truths values with the
harness; logged as the METHOD LESSON in engine-quirk #102 (amended) + HANDOFF.

The captured arc is the SHORT HOP, confirmed empirically: the ring execute `cmd[2]=7` fires for ONE tick
(frame 4603) then `cmd[2]==0` the whole rise (4610-4630) → the FREE rise grav 8000 (apex at tick 10).  A
held-C jump (`cmd[2]=8`) would use 2000 → a much higher arc; the port models both via `jump_held`.

Then VALIDATED the held high jump bit-exact with a held-C capture (`runs/runjump-gt/capheld`: held C via
the leaf scancode `0x2e` → `cmd[2]=8` the whole rise → grav 2000).  The held RISE matches the port's held
model BIT-EXACT for 16 ticks (`test_character_jump_held_rise`; apex rise 10400 vs the short hop's 4800,
2.2× higher).  The capture revealed two more mechanics: (a) retail's held apex CLAMPS on a town CEILING
(~tick 16, wy≈41600 — the `wy += vvel/100` relation breaks = a vertical collision clamp by `0x54e5c0`),
which the flat-ground port can't model (it keeps rising) → the `char-collision-mover` debt, NOT a
jump-physics gap; (b) a terminal fall velocity of 64000 (the held fall plateaus at 64000 for 8 ticks),
now ported as `CHAR_JUMP_FALL_TERMINAL` (the short hop is unaffected — it reaches exactly 64000 at
landing).  961 host pass.  Debt: PORT-DEBT(char-jump-variable-height) (the rise validated; the ceiling +
the ~7-flip windup remain), PORT-DEBT(char-jump-fall-grav-source) (the 4000 + 64000 sources un-located),
`char-run-jump` renamed `char-run` (jump done; run remains).  NEXT: the dash double-tap; the jump windup;
then the chip-4 live wire (`character_step`'s first live caller) → Arche jumps on screen → USER visual-verify.

---

## 2026-06-11 (ckpt 116) — PHASE-4 chip 3b: the run/jump BLOCKER is RESOLVED — jump/dash are RING-sourced, and Arche's JUMP is captured bit-exact in the TOWN

Resolved the ckpt-115 chip-3b blocker by RE + a live experiment, and **REFUTED** the "dash/jump need a
platforming/dungeon scene" hypothesis.  Pure ground-truth (no port code); 958 host pass (unchanged).
The root cause is a HARNESS-injection gap, not a scene gate: the chip-3b captures pressed C (and the
dash directions) only through the HELD-AXIS leaf `0x5ba520`, which fills the held array and yields the
marker `cmd[2]=8` — but the apply `0x442a70` executes the jump on **`cmd[2]==7`** (`:801`), which
`0x478ba0:287` sets from a discrete **EVENT-RING** match `FUN_00479960(now,0,800,1,7,…)` scanning the
ring `input-mgr+0xc` (64 × `{id,ts,flag}`) for `id==7,flag==1`.  `cmd[2]=8` (the held marker, from
`+0x124`) is consumed nowhere — it is the variable-height hold-to-rise input.  Dash (cmd[0]=5/6) is the
same gap (`FUN_00479e70` direction double-tap in the ring).  Reading `FUN_00479960` (it reads
`in_ECX+0xc`, the same ring `--input-trace` fills for the Z-advance id `0x24`) was the linchpin.

Confirmed empirically with ZERO harness changes: re-captured the town freeroam with the existing nav
(Z-spam to the inn control transfer) + `{"frame":N,"ids":[7]}` jump presses (`runs/runjump-gt/jump-ring.jsonl`,
capture `capjump-ring2`).  Arche jumps — `vvel 0 → −76000`, `wy 52000 → 47200 (apex) → 52000`, lands
clean, **two byte-identical jumps** (deterministic).  Plot pushed to the feed.  The jump arc is the
bit-exact port target: impulse **vvel = −80000**, `wy(t+1)=wy(t)+vvel(t)/100`, **ASYMMETRIC** gravity
(rise decel +8000/tick, fall accel +4000/tick — a floaty fall, ~27 ticks airtime, apex rise 4800),
ground-clamp at `wy=52000`.  The apex/fall branch is the body+0x38==3 airborne sub-FSM (`0x442a70:832-877`,
the `-20000` threshold); consts `in_ECX[0x5667/0x565b/0x565e]`.  engine-quirk #102 amended.

NEXT: (a) PORT the jump (extend `character.{c,h}` with `world_y/vvel` + the airborne integrator,
host-tested vs the captured arc; RE the apex/fall + variable-height first, don't curve-fit).
(b) Capture + port the DASH (two direction ring presses + hold → cmd[0]=5/6, the run cap).  (c) The
chip-4 LIVE wire (Arche walks/jumps on screen → USER visual-verify).

## 2026-06-11 (ckpt 115) — PHASE-4 chip 3a: Arche's freeroam WALK is PORTED + field-exact (`src/character.{c,h}`)

Ported the controllable-character WALK as a field-exact open-air reduction of the AI `0x478ba0`
(held-axis → command block) + the `0x442a70` case-0x75 horizontal integrator — the symmetric
counterpart of butterfly chip 1 (ckpt 110), on the CHARACTER band.  New `src/character.{c,h}` +
`tests/test_character.c` (4 tests); 958 host pass (+4).  No live port caller yet — the payoff (Arche
walking on screen) lands with the chip-4 freeroam hand-off; this chip ports + validates the LOGIC
against the ckpt-114 ground-truth capture (`runs/mover-caller`), exactly like chip 2's collision
read-side ("host-tested only, live payoff at Arche").

The WALK law, fit BIT-EXACT to Arche's real-body (`0xe637b80`) per-tick worldX: the horizontal
velocity accumulator `body+0x28` (NOT `body+0x18` = the vertical/jump vel, 0 the whole flat walk)
ramps via the 150-byte clamp-ramp `0x445db0` — **accel +1600/tick → cap 24000** (dwx = vel/100 ramps
+16/tick: 16,32,…,240) when held, **brake −800/tick → 0** (dwx −8: 240,232,…,8,0) on release; facing
`body+0x2c` holds through the brake-to-stop and flips 1↔3 only at v==0 on an opposite command; worldX
+= vel/100 (a flat reduction of the collision mover `0x54db10`).  `test_character.c` asserts the accel
ramp, the cap sustain, the brake-to-stop, and the LEFT-from-rest symmetry against the **embedded
captured worldX bytes** — a pass proves the port reproduces retail's bytes tick-for-tick.

Annotated `0x478ba0` (the durable `char_ai` field-spec: held axis U/D/L/R, cmd[0], speed-mode, facing)
in `retail_fields.json`.  Reduction tagged 4 PORT-DEBTs: `char-run-jump` (walk only; run cmd 5/6 +
jump cmd[2]/[4] need their scancode RE + capture), `char-input-autorepeat` (the press→latch warmup
constant reproduces the wall-clock auto-repeat), `char-walk-tuning` (accel/cap/brake = the captured
consts; real source `in_ECX[0x565b/c/e]`), `char-collision-mover` (the flat worldX commit + the
untested reverse-decel rate).  NEXT: (a) the run/jump capture + port; (b) the chip-4 hand-off wires
`character_step` live → Arche walks + the collision mover/probes get a live grounded actor.

## 2026-06-11 (ckpt 114) — PHASE-4 chip 3: Arche's FREEROAM MOVER pinned — input→position is AI `0x478ba0` + apply `0x485fc0`→`0x442a70` (pure RE)

With the ckpt-113 held-axis harness driving the walk, `--call-trace`'d the kinematic integrator
`0x442a70` over the walk window with arg/`this` field reads (`tools/flow/freeroam_mover_fields.json`)
and filtered to Arche (`in_ECX+0x1d4==0xc35a` — in_ECX IS the ~90 KB entity).  Every call-trace row
carries `ret_va` (the caller), so this names the mover directly.  Artifacts: `runs/mover-caller/`
(`find_mover.py` + the capture); no port code, 954 host pass unchanged.

The result: freeroam character movement is **TWO layers, both shared with the existing actor
system** — structurally identical to the butterfly, but on the CHARACTER band and with the FULL
integrator.  **AI `FUN_00478ba0`** (the RNG-free character update townsfolk share; counterpart of
the butterfly's `0x47b990`) reads the held axis at `*(entity+0x158a4)+0x114/118/11c/120` (U/D/L/R,
quirk #41 confirmed) and builds the command block `entity+0x14854` (LEFT→`[0]`=1/5, RIGHT→2/6,
DOWN→`[3]`=10, UP→`[3]`=0xb; walk/run via speed-mode `entity+0x158a0`), plus a 4-step collision
look-ahead into STACK scratch (the "shadow" `0x442a70` calls).  **APPLY `FUN_00485fc0+0x96e`**
(`485fc0.c:348`: `FUN_00442a70(in_ECX+0x5215=cmd, body, body, 0,0)`, in-place on `*(entity+0x40)`,
gated `local_2c==0`) is the ONLY committer of Arche's real-body (`0xe637b80`) position — 244 calls,
`body_wx==new_wx`, tracking the walk **19200→40800→44280→24120**, facing 1=right→3=left.  vel
`body+0x18`=0 ⇒ a direct position write; the per-tick step accelerates +16/tick → a ~+240 cap.

This **reconciles engine-quirk #101 finding #3** (ckpt 112's "separate party-leader path, candidates
`0x405e80`/`0x406210`/`0x40c380`"): Arche's AI is indeed not `0x47b990` — it's `0x478ba0` — but the
apply IS the shared band pass, so `0x46cd70` reaches her as an active band actor (it just never reads
the party array `+0x4030`).  The candidate guesses are superseded.  Writeup: engine-quirk #101 final
bullet (amended); plan `plans/movement-system.md` chip 3.  NEXT: trace the caller of
`0x478ba0`/`0x485fc0` for `0xc35a` (band slot vs party path `0x4997b0`); RE the run/jump scancodes
(`0x8a6e80` keybind defaults) → capture run+jump per-tick; then PORT the `0x478ba0` AI + the full
`0x442a70` integrator.

## 2026-06-10 (ckpt 113) — PHASE-4 chip 3: the HELD-AXIS INJECTION HARNESS — Arche WALKS in retail freeroam (the chip-3 blocker closed)

The ckpt-112 ground-truth found that the freeroam mover reads the HELD-AXIS array
(`input-mgr+0x114`), not the discrete event ring — so `--input-trace` (ring presses) drives menus
and the dialogue Z-advance but leaves the controllable leader idle when walking.  This checkpoint
builds the level-injection mode that was the movement ground-truth's blocker, and live-validates it.

RE (ground truth): the producer `0x46a880` (the per-frame global input update) fills array A each
frame from the DInput keyboard buffer via the leaf query `0x5ba520` (= `keyboard_state[scancode] &
0x80`), one slot per key — `+0x114=UP(0xc8) +0x118=DOWN(0xd0) +0x11c=LEFT(0xcb) +0x120=RIGHT(0xcd)`,
action buttons at `[4..]`.  So array A is PER-DIRECTION held booleans, correcting the long-standing
input.h / `0x56aea0` "[0]=vertical/[1]=horizontal" label (engine-quirk #41 confirmed + amended).

Retail (`tools/frida/opensummoners-agent.js` + `frida_capture.py --held-trace`): hook the leaf
`0x5ba520` and force its return to pressed for the injected scancodes — the real producer then fills
`+0x114` exactly as a physical keypress would.  Hooking the leaf rather than writing the array is the
decisive choice: it needs no model of the engine's per-frame clear/produce path (release is
automatic), covers both the array-mediated mover and any direct-query consumer, and survives the
hidden window's loss of DInput focus.  A read-back diagnostic hooks the producer `0x46a880` onLeave
(`{kind:'axis'}`).  Port (`src/held_trace.{c,h}`, NEW, host-tested ×8): the LEVEL counterpart of
`input_trace` — `{"frame":N,"keys":[scancode|name]}` rebuilds `mgr->axis_held[0..3]` every frame;
`main.c` wires `--held-trace` at the 4 drive replay sites (the title menu already consumes it);
`mem_watch` gets it too.  954 host pass (+8).  Commit 776f575.

Live-validated (`runs/freeroam-walk`): drove retail to freeroam (stripped Z-spam so held-axis is the
sole mover) then held RIGHT 4560-4760 / LEFT 4820-5020.  Arche's `arche_body.wx` went **19200→41760**
(RIGHT, facing 1) then **45472→25320** (LEFT, facing flips to 3), walk anim cycling + decel to a
stop — versus the ckpt-112 ring run's static wx=19200.  Walk montage pushed to the feed.  Lead for
pinning the mover: `vel` (body+0x18) reads 0 throughout, so her horizontal speed lives elsewhere.
NEXT: `mem_watch` her `wx` writer under held walk → the chip-3 mover; then walk/run/jump per-tick →
bit-exact target → port.

---

## 2026-06-10 (ckpt 110) — PHASE-4 chip 1 DONE: the butterfly open-air PATROL MOTION ported + FIELD-EXACT (heading 0 mismatches; the butterflies drift left↔right 1:1)

Chip 1 of the entity-movement arc (`plans/movement-system.md`).  `src/butterfly.{c,h}` grew from
the ckpt-98 RNG-consumer stub into the full open-air FSM — a documented REDUCTION of `0x47b990`
(the `0xe29a` heading AI) / `0x43f880` (the move command) / `0x485fc0`→`0x442a70` (the apply
integrator).  Ported: (a) the patrol bounds set at register (`b1=spawn_wx+11200`,
`b3=spawn_wx−8000`); (b) the heading FSM — the two RNG draws/tick (wander range + the 10% flag)
MOVED from the consume-stub into their real use (decrement cooldown `+0x14248`; flip heading
`+0x14244` toward the far bound when `cd==0` AND `(|wx−bound|<0xc81` OR the 10% roll); the
`0x47dbb0` collision term omitted = open-air clear); (c) the horizontal integrator run EVERY tick
(`world_x += hvel` then `hvel` ramps ±10/tick→±100 — the capture's step-before-ramp), apply-wired
into the rendered EFFECT actors via an `effect_slot` link in `main.c`.  Critically the RNG draw
**count/order is UNCHANGED**, so the ckpt-99 settled-town stream stays bit-exact.

**Validated field-exact on the SIM-TICK axis** (`runs/butterfly-fsm/compare.py`): the port emits
a `CALL_TRACE_BEGIN(0x47b990)` mirror per butterfly per tick; diffed vs the ckpt-109 capture, the
HEADING matches retail with **0 mismatches** for all 4 butterflies — every flip tick exact (bf0
[35,85,155,199,243], bf2 [3,47,101,149,189,269], …), which proves the LCG is byte-aligned through
tick 269 (beating ckpt-99's 0-248).  facing ≤1/286; worldX field-exact between reversals (bf0 to
t37, bf3 to t51).  The ONLY worldX residual is a ≤170-unit ≈ ≤2px BOUNDED transient at
turn-arounds — the deferred flap-coupling — which does not accumulate because the exact flips
phase-lock it.  Host `butterfly_motion` (new) + `butterfly_pertick` (unchanged → RNG intact); 940
pass.  Durable annotation `0x47b990` added to `retail_fields.json`.

Deferred (PORT-DEBT, all retire with the full `0x442a70` integrator in chip 2/3):
`butterfly-flutter` (the vertical flutter sawtooth `body+0x18` + the `cmd_2` flap sub-FSM →
worldY bob, + the flap/reversal coupling = the ≤2px residual; worldY currently holds spawn);
`butterfly-bounds-writer` (the +11200/−8000 derivation un-RE'd); per-instance frame_base
multicolor.  NEXT: chip 2 = tile collision (`0x2c1030`/`0x2c1040` grid + `0x4412d0`/`0x47dbb0`
probes) → chip 3 = controllable Arche.

## 2026-06-10 (ckpt 109) — PHASE-4 chip 0 DONE: the butterfly FSM ground-truthed per-tick; the capture resolved BOTH plan open items (the apply pass + the bounds formula)

Chip 0 of the entity-movement arc (`plans/movement-system.md`).  A seed-pinned + lockstep
field-spec capture of `0x47b990` (the EFFECT-band AI) over `game_enter@1434` → sim-tick 285
× the 4 town butterflies (`runs/butterfly-fsm/`; spec `tools/flow/butterfly_capture_fields.json`,
analysis `analyze.py`) is now the **bit-exact target for the chip-1 port**.  It reads worldX/Y,
heading `+0x14244`, facing `body+0x2c`, the patrol bounds `+0x14264/+0x14268`, the cooldown/
gate/flit timers, the wander range, the cmd block `+0x14854`, vel `body+0x18`, and the flap.

The capture resolved BOTH of the plan's open items as bycatch.  **(1) The apply** (was
"unlocated"): `0x46cd70` makes TWO passes over the EFFECT band — pass 1 the AI `0x47b990`
(writes the cmd block, GATED to work ticks), pass 2 **`0x485fc0`** (`46cd70:71`) which calls
the integrator `FUN_00442a70(this+0x14854, body, body)` (`485fc0:348`) on the REAL body, EVERY
tick (capture-confirmed: position moves on both gate phases).  So the AI decision is gated; the
motion glides because the integrator (`vel+=2000` clamp ±16000, worldX step via `0x54e5c0(body,
vel/100)`) carries velocity between decisions.  **(2) The bounds** (the "decompile lie" — no
static writer): dead-constant per butterfly, **`b1 = spawn_wx + 11200`, `b3 = spawn_wx − 8000`**
(center spawn_wx+1600 ± 9600=3 tiles; `8000=+0xc894`, `11200=8000+0xc80`), spawn_wx = the 4 map
worldX.  Chip 1 sets them at register-time; `PORT-DEBT(butterfly-bounds-writer)` for the un-RE'd
spawn derivation (mem_watch deferred — values are bit-exact for the town).

Also pinned: heading `+0x14244` = INTENT, facing `body+0x2c` = actual travel dir (the integrator
flips it on the velocity-sign change, LAGGING heading ~5 ticks); the heading flips on a work tick
when cooldown==0 AND (`|worldX-bound|<0xc81` OR a `0x47dbb0` collision OR a 10% RNG roll); the
flap `rs+0x72`∈{0,1,2} is heading-INDEPENDENT.  `0x47b990` is the AI for 18 town actors (NOT
butterflies-only, correcting a ckpt-108 note).  No port code yet — chip 1 (the FSM + integrator
port + bit-exact validation) is the next session.  939 pass.  (ckpt 107 = R7 fountain 1:1,
ckpt 108 = the arc opening — both in FRONT/HANDOFF; not separately logged here.)

## 2026-06-10 (ckpt 106) — R6 establishing-REVEAL RESOLVED: two stacked causes (graded mask cels + the 105b fence); the frontier band differ_px==0 at every stamp-equal tick; R7 narrowed to the fountain alone

The R6 chase started with the queued recipe and ended somewhere better: a live per-row
grid dump (new `0x499ab0` field-spec chain fields `r40..r80` + a port mirror in main.c)
proved the fade-grid's per-cell `(state,timer)` BIT-IDENTICAL both sides — 41 rows × 31
ticks, staircase `timer=100u+50−50d` — so the pixel residual had to be cel content or
registration.  Per-pixel mask-level extraction (`s5 = backdrop5 − out5`, mode per cell)
against the studio frames showed retail's effective mask level == the frame index
EXACTLY (`s5(a)==a`), while the port's content sat at `index(T−100)` — one aging step
young — AND one 5-bit step weak.  Two causes: (1) the port graded res `0x458`/`0x583`;
retail binds both through the plain getter `0x4184a0(0)` (quirk-#96 family) — slots
40/41 now on the grade skip-list; (2) the ckpt-105b `hold>=2` fence — a misfix whose
dt-scan justification had been computed over the graded cels (biased exactly one tick)
— removed.  After both: the reveal band is differ_px==0 at EVERY stamp-equal tick 2..32,
and 100% of the remaining reveal-window residual is the fountain box (smoke 0) → R7.
quirk #100; parity-ledger R6 (incl. the reusable state-proof → cel-extraction method).

Bycatch: the grid object's +0x20/24/28 ramp is the town's ambient AUDIO fade-in
(0→1000 at +10/tick, ducking ~12 DSound position updaters through the `0x5bb870/80/90`
vol/pan/freq thunks) — pixels never read it; logged for the future sound port (quirk
#100).  Tooling: the studio's port driver must hand the child a PIPE stdout — WSL
interop's exec of a Windows binary fails its vsock handshake when stdout is a regular
file (`UtilAcceptVsock accept4 110`; this wiped intro-1's port side once mid-session —
recaptured).  939 pass; ledger unchanged (bare-VA work).

---

## 2026-06-10 (ckpt 105) — the SIM-TICK AXIS in the trace studio; the whole intro-1 worklist attributed at tick-equality; banner/pan/dialogue triggers recalibrated to differ_px==0; the @2463 "zero-mean" verdict retracted

The marks left on intro-1 resisted flip-axis chasing (the pairing's pixel-driven drift wanders
±3 ticks through content-quiet stretches), so the timestep-determinism rule got tooled
end-to-end: the port now stamps its easer-call count (`g_sim_tick_count`, the exact mirror of
the retail agent's `0x43d1d0` hook) into `--capture-all` BMP names
(`port_frame_<flip>_t<tick>.bmp`), and the studio carries BOTH sides' ticks through pairing →
`state.jsonl` (`port.sim_tick`) → the viewer (red on pair mismatch) → worklist rows.  Marks
are now chased at FORCED tick-equality (`docs/trace-studio.md`).

That attributed every open mark in one pass (parity-ledger #13 + R6/R7/R8; quirk #99):
**@1177 NPC-anim** differ_px=0 at tick-equal (pairing phase, closed); **@218 lizsoft fade** =
R3 boot stretching, the logo differ_px=0 at matched fade across its lifecycle (closed);
**@2159 banner fade-out** = NOT noise but a 2-tick trigger offset (closed by recalibration);
**@1122 reveal** = real ~1-tick frontier lead (R6 open); **@1177 fountain** = real
tick-shift-invariant ensemble offset (R7 open); **@2463 text reveal** = real row-close
pause-grade mismatch (retail {5,14,5} vs fitted {1,5,16}) on top of an 8-tick arm lag — the
ckpt-104 "zero-mean oscillation" attribution was a flip-axis pairing artifact, retracted.

Three measured-constant triggers recalibrated onto the tick axis (flip-axis readings absorb
retail's coalesced ticks; fade dt-probes plateau on the 2.5-tick alpha-ramp quantization —
calibrate off per-present VALUE sequences): banner arm 78→82 (first step t42; both fade edges
differ_px==0 at tick-equal), pan 184→182 (cmd t92/move t93; tick-equal pan residual = the
named fountain/smoke/butterfly clusters only), dialogue 1298→1282 (arm t642/first change t645;
the pop/portrait window differ_px==0).  3 port-side recaptures verified each step.  939 pass.

---

## 2026-06-10 (ckpt 104) — the in-game DIALOGUE BUBBLE: ported + bit-exact in-window (pop-in, tail, tab, name, portrait fade, typewriter); the intro-1 worklist's big mark closed

The first chip worked off the USER's trace-studio worklist (mark @2429 "dialogue bubble pop in
animation" = the ckpt-102 plan's chips 3+4 reveal half).  `src/dialogue.{c,h}` (NEW, pure,
host-tested ×7) + `main.c game_render_dialogue` at the `0x48c150:179` widget-tree position.
The model (engine-quirk #97, decompile + live): the 9-slice bubble (res `0x456`) drawn at the
node SCALE `+0x54` (the pop-in: +50/update, content gated till 1000, centered integer math),
the speaker-anchored bubble-TAIL pair (box-bank frames 9/10 — a first-cut misread placed them
at the box left edge; `0x49c640`'s math puts them at clamp(speaker_center)−16, box bottom),
the name tab (res `0x44a` f0 long-name), the name (white main + `0x455f7b` shadow, 3-pass),
the portrait bust (res `0x7ef`, 24bpp magenta-keyed, 1:1 at (150,76)) cross-faded via ramp_b
with the `0x49c910` SNAP — the incoming cel goes fully opaque at fade 500, not 1000 (round 1
of the studio loop caught the lag) — and the typewriter body rows (Courier 7×18,
`0x3e537d`/`0xa8b9cc`, 5 updates/char, comma 3i, space 1, row close +i — fitted to the
ckpt-102 reveal trace, PORT-DEBT dialogue-pause-grades).  Line 1's text + speaker name are
read from the user's exe by VA (`0x86d58c` / `0x6b6f80`) — story content never in source.

**Verified on trace-studio intro-1 (`recapture --only port`, 2 rounds): the box region pairs
`differ_px==0` on 22/25 sampled frames across pop-in → fade → typing**; the remainder is ONE
GLYPH at a reveal boundary.  The USER's follow-up mark @2463 ("retail a couple frames ahead
on the text reveal") measured zero-mean: the boundaries oscillate ±2 flips (retail ahead
2462/2472/2544, simultaneous 2516-2580, port ahead 2558/2568/2590) = retail's tick-coalescing
under lockstep stepping through the sticky pairing drift (the R5 mechanism) — cadence 1:1,
no logic divergence.  parity-ledger #12.

Format finds (quirk #98): the 24bpp blobs embed a plain BMP whose 1024-byte palette slot is
uninitialized packer memory (XP-era heap pointers shipped in sotesd.dll), and the screen
pixels are the sheet through ONE RGB565 quantize+bit-replicate round trip (16bpp surfaces) —
proven by an exact-match of the raw portrait against the live frame.  Also: res 1000 in
sotesd is a parallax MOUNTAIN plane, NOT the advance arrow the box_cell probe reported —
a quirk-#92 numeric collision; the arrow bank's module is unresolved (PORT-DEBT
dialogue-arrow-art; benign in-window since the arrow is HIDDEN during typing — confirmed on
retail PNGs — and no capture reaches a finished line).  The UI sheets (`0x456`/`0x44a`)
decode UNGRADED (plain-getter family) — the decode-hook grade skip-list gained slots 50/52
(extends `banner-grade`).  +`0x48cf80` flow-trace annotation both sides (the box-frame rect).
939 host tests (+7).  Debt added: dialogue-trigger / dialogue-line-table /
dialogue-arrow-art / dialogue-pause-grades / dialogue-textwrap.

---

## 2026-06-10 (ckpt 103) — the TRACE STUDIO: capture → scrub → mark → worklist → re-capture (openrecet-style), live on the full intro

USER directive: build the trace viewer now (it was Phase-C-gated on a controllable character, but
input injection exists and visual checks are frequent), improve/clean the tooling around it, archive
stale ad-hoc flows.  Built `tools/trace_studio.py` + the `tools/trace_studio/` package + the
`tools/trace_studio_web/` SPA (htm+preact, adapted from openrecet's proven studio), 36 tool checks.
How-to: `docs/trace-studio.md`; architecture: `docs/plans/trace-studio.md`.  3 commits.

**The loop it lands:** `capture in-game-intro` drives BOTH targets concurrently (port:
`run-opensummoners.sh --capture-all` staged to C:/; retail: `frida_capture --no-turbo --lockstep
--seed-pin --capture-frames all --max-flips N`), pairs the two flip axes ANCHOR-SEGMENTED
(boot/subtitle/newgame/prologue/game_enter) with a sticky ±drift best-match (tas_diff's model made
dense — absorbs the port's duplicate-frame wobble), and emits ordinal-named frame trees (same
`frame_<k>.png` = same moment on port/retail/diff), all-intra scrub mp4s, `state.jsonl`, an
anchor-RNG + per-segment bit-exact verdict.  `serve` (:8779) scrubs the three panels in lockstep
with a differ_px ribbon (black = the bit-exact bar), anchor track, per-frame state; the USER flags
divergences as MARKS (note/feature/rng/phase + drag-box → retail|port|white-diff crop thumbnails);
`apply` renders them into `worklist.md` (the Claude hand-off); re-capture (`--only port` = fast
loop vs the cached retail) keeps marks + working-trace edits.

**First live session (`intro-1`, 2598 paired frames over 4 segments) validated the model and
visualized the whole current gap in one artifact:** the prologue segment LOCKS at constant −7 drift
with 192/290 frames differ_px==0 (the −7 measures the per-side anchor→content arm offset); the town
segment hunts at 0/1483 because content genuinely differs every frame — the missing dialogue box
(the ckpt-102 front), frozen butterflies (PORT-DEBT butterfly-wander), pan offsets.  boot/title
redness = the documented R3 render-rate skew; anchor-rng DESYNC before game_enter = expected
(quirk #77 — this scenario's nav skips the title-sparkle seed pin; the verdict explains it inline).

**Harness hardening from the live smokes** (each found by a real failure): every anchor firing now
streams to `<run>/anchors.jsonl`; `--max-flips` stops a capture at a Flip count WITH an agent-side
emit ceiling (the ~900KB/frame firehose otherwise starves the teardown RPC for minutes); leftover
games must be killed THROUGH frida (children of the elevated frida-server give taskkill
Access-denied) + a pre-flight kill (the "Game is already running." boot bail); the WSL-interop
vsock footgun (port launches fail with `accept4 failed 110` from detached contexts).  New
parity-ledger **R5** (USER-observed): the retail cutscene pan shows spikes vs the port's smooth pan
— hypothesis: real-clock phase pillar, should vanish under lockstep; verify with per-frame camera
state in a `--call-trace` session.  Archive sweep: `tools/archive/README.md` (the ad-hoc /tmp
side-by-side video flow → superseded by the studio; the frida_capture probe-flag graveyard marked
for mechanical removal).  Plan B1 (`plans/trace-tooling-phase-b.md`) is superseded by the studio's
capture as the one entry point.

## 2026-06-09 (ckpt 102) — the in-game DIALOGUE BOX subsystem RE'd + the legal text-reader built (foundation for Phase 3 dialogue → controllable Arche)

The directive: dialogue next, leading to controllable Arche after the cutscene + dialogue.  Reverse-
engineered the whole town-arrival dialogue subsystem and laid its foundations (no pixels yet; 2 commits,
932 pass).  Writeup: `findings/in-game-intro.md` "The DIALOGUE BOX subsystem"; chip plan:
`plans/dialogue-cutscene.md`.

**Architecture (decompile-verified).**  The arrival is a *linear cutscene coroutine*: the script
`FUN_004d7d80` case `0x334be` (dispatched by `0x40c380`) configures a "beat" on a scene-controller
object, then calls `FUN_00439680` (a 9-byte thunk → the blocking BEAT-RUNNER `FUN_00439690`, 8866 B)
which pumps frames (`0x48c150` render + `0x5b8fc0` flip + `0x5b1030` pump, `GetTickCount`-paced) until
the beat's completion condition holds — returning 6 on Escape, which unwinds the script.  Dialogue-line
setup `FUN_0049d6e0` writes the controller's fields: text→`+0x8a`, speaker name (actor `+0x750`)→`+0x28a`,
voice→`+0x2e8`, portrait id→`+0x84` (resolved via the face table `DAT_006b6568`, keyed on the actor's
head-state `+0x1d8` + a script face id), dirty `+0x78`, beat `+0x20=1`.  Beat-type switch (`0x439690:1128`):
1 dialogue (advance via the Z poll `0x43b980`), 2 flag, 3 camera-reached-target, 4 entity, 6 timer
`+0x57c`, 7 portrait-anim, 9 entity.  The render goes `0x48c150 → 0x48c820` (widget-tree walk) `→
0x48cf80` (9-slice frame) `+ 0x48e200` (GDI text) — and BOTH primitives are already ported (`newgame_box.c`,
`glyph_render.c`).

**exe_strings — read story text from the user's exe by VA (committed).**  The dialogue text + speaker
names are story content in the retail exe's `.data` (town line 1 @ VA `0x86d58c`: "Ahh, here we are at
last!%nLook, Arche…").  Per the legal line + the dramatist-table precedent, the port must not embed them.
Confirmed the Steam `.bind` DRM leaves `.data` intact (the string is byte-identical at file offset
`0x46d58c` in BOTH the installed packed exe and the unpacked vendor copy), so a plain raw-file read works
— no unpack.  Built `src/exe_strings.{c,h}` (pure `pe_string_at`: PE32 VA→file-offset, fully
bounds-checked) + `exe_strings_win32.c` (`exe_data_string` maps the user's `sotes.exe` once).  5 host
tests incl. a real-exe validation (skips if the vendor binary is absent).

**Ground truth captured** (`runs/dialogue-probe`, `runs/dialogue-portrait` — seed-pinned nav trace, no Z
after game_enter so line 1 appears + waits; PNG `frame_03100` visually confirms): big Father bust portrait
on the left, parchment box on the right with a name header + 2 typing text rows.  Box frame res `0x456`
(9-patch, corners 32×32, node 408×112 @ (174,148), alpha fade-in); name "Arche's Father" Courier New
**7×18** color `0x455dbb` ~(410,139); 2 body rows Courier New 7×18 (typewriter ~1 char/10 flips, main
`0x3e537d` + light outline `0xa8b9cc`); advance arrow res `0x3e8` (animated, ~(542,240)); portrait res
`0x7ef` (160×176, magenta colorkey).  Town script ≈15 lines (Father `0x5f5e1d3` / Arche `0x5f5e165` /
Mother `0x5f5e1d4` / Sana `0x5f5e166`; voices `0x3eb`–`0x3f4`).

**Next:** `src/dialogue.{c,h}` — register res `0x456` + `0x7ef`, compose box + name + 2 text rows +
portrait at the captured geometry, arm via a measured-constant trigger (PORT-DEBT) after the banner,
verify `differ_px==0` vs `runs/dialogue-probe`.  Then typewriter + Z-advance + the full script; then the
beat-runner driver (retires the banner/camera-pan/letterbox measured-constant debts); then Phase 4
(party band `0x4997b0` + movement FSM `0x43f880`) = controllable Arche.

---

## 2026-06-09 (ckpt 99) — the settled-town per-tick RNG stream is bit-exact ALL THE WAY (4 ambient/event timers)

The directive: close PORT-DEBT(fountain-rng-phase) — make the SETTLED-town per-tick LCG stream
bit-exact, not just through the establishing-REVEAL window (ckpt 98).  The ckpt-98 model had 5
"misses" beyond ~tick 33, guessed to be "the ambient timer `0x5531b0` ×3 + 2 unknown".  RE'd them
properly: they are **FOUR self-clocked ambient/event timers** (+ the butterfly re-fire `butterfly.c`
already models), each a clean **unit-decrement** countdown.  Pinned by a seed-pinned timer-state
capture (`runs/ambient-timer`, new `0x5531b0`/`0x467380`/`0x54f980` field-spec entries reading each
one's `+0x5c`/`+0x20c` `cd` directly — plus a reusable `argfield` agent source to read a struct field
off a stack-arg pointer): `0x11370` (`0x5531b0` ambient SOUND) fires tick 33, the wagon `0x1872d`'s
idle-wander (`0x54f980:932`) tick 134, the `0x467380` `0xe2a5` event timer tick 183, `0x1136f`
(`0x5531b0` SOUND) tick 189.  The census's earlier C-values (141/189/33) were **off-by-one** — the
`0x5bf505` `rng_state` field is the state *before* the draw (value = `rval(step(state))`).

**Ported `src/ambient.{c,h}` (NEW)** — four consume-to-advance timers run in the proven `0x46cd70`
band order (`game_actor_update`): EFFECT `butterfly_step → ambient_effect_step` (`0x467380`), then
CHARACTER `fountain → sky → ambient_character_step` (`0x1136f`, `0x11370`, wagon).  Each inits
`(rand*300)>>15` at tick 0 and fires (3-4 draws) when its countdown expires; the drawn values feed
sounds / the wagon wander / an `0xe2a5` sub-effect (none ported) but the COUNTS + TIMING keep the
shared LCG aligned so the fountain/sky particle positions read the same stream as retail.  Replaced
the ckpt-98 blanket 3-draw tick-0 consume.

**Validated 3 ways:** an offline MSVC-LCG replay reproduces the capture's `0x46cd70` onEnter rng for
the FULL 298-tick window (**0/297** transitions); two **live port** runs (frame-window `--call-trace`)
match retail tick-for-tick across ticks 0-248 — through ALL FOUR ambient fires (33/134/183/189,
including the `ambient_effect_step` event timer at 183); host test `ambient_pertick` (922 pass, +1).  **Retires** the RNG residual of fountain-rng-phase + the
RNG half of PORT-DEBT(actor-protagonist-clip) (the wagon idle-wander).  Residual: PORT-DEBT(ambient-event-cd)
(the `0x467380` cd-init = the seed-pinned 184; real source = the unported `0xe2a5` spawn arm `0x431cb0`).
engine-quirk #95 (amended); `findings/in-game-intro.md` "The per-tick RNG stream".

## 2026-06-09 (ckpt 98) — the intro-cutscene PER-TICK RNG STREAM is ported + LIVE bit-exact

The directive was "sweep all the rng gaps in the intro cutscene + fix the debts blocked by this."
Two chips, both ground-truthed against the seed-pinned per-tick census `runs/rng-census-repin` and
validated by replaying the MSVC LCG offline + live.

**ckpt 97 — the room-load SPAWN BURST (engine-quirk #94, commit `b1bc5af`).** The town's first in-game
frame draws a 19-object EFFECT burst — 15 MAP objects (`actor_spawn_effect_from_map`, 171 draws) + **4
SCRIPT cutscene-cast** objects (`0x4d7d80`→`0x41f0e0`→`0x41f200`: Arche `0xc35a` 12 draws via her
`0x427360`, Barnard/Father/Mother 10 each), THEN the establishing-REVEAL iris-variant draw `(rand*3)>>15`.
The port consumed only the 15 map objects, so the iris drew variant 2 (sweep) — pinned to 0. Now
`actor_spawn_cutscene_cast` consumes its 42-draw burst, so the iris VARIANT is DRAWN at the right phase =
**0 (center-out)**, retail's town value. Only the count matters (the LCG state after N steps is value-
independent); proven offline (`0x4f5347`+214 draws = `0x9c2b551d` = retail's tick-0 state — the spawn is
now byte-aligned), host-tested (`actor_spawn_cutscene_iris`), live (`scene_fade_arm … variant=0 … DRAWN`).
Retires the RNG half of PORT-DEBT(scene-fade-rng-phase) (renamed `scene-fade-window`).

**ckpt 98 — the PER-TICK stream (engine-quirk #95, `src/butterfly.{c,h}` NEW).** Resolves the ckpt-73/#77
"the stream desyncs run-to-run even under `--seed-pin`" mystery: under the pin it is fully deterministic
— that was the *port* missing the EFFECT-band draws, not nondeterminism. `0x46cd70` walks the bands; only
two draw RNG in the town: the **4 BUTTERFLIES** (`0x47b990`, called ONLY for update-mode-1 EFFECT actors;
the townsfolk + cast take the RNG-free `0x478ba0`) and the **fountain/sky emitters** (`0x54f980`, already
ported). `butterfly_step` runs the `0xe29a` draw model once/sim-tick BEFORE the emitters — the gate
`0x14232` (work on even ticks), the flit-pick timer `0x14236` (fires every 80 work-ticks = 160 sim-ticks),
and the heading+flag draws — with each butterfly's move-freq `0xc874` captured by the spawn replay. Clean
count model: `tick0=23`, then `6 + 8·[N even] + 8·[N≡5 mod 6]`. **Validated:** the offline LCG replay
reproduces retail's `0x46cd70` onEnter rng for **293/298 ticks** (the 5 misses are named irregulars — the
ambient timer `0x5531b0` ×3, the flit re-fire at tick 162, +2 unknown); host test `butterfly_pertick`
locks the gate/timer/count; and **the LIVE port's per-tick rng matches retail tick-for-tick**
(`0x9c2b551d, 0xb92fc6fa, 0x5c22a348, …` for ticks 0-11). The flit MOTION (`0x43f880`, the 5.5 KB movement
FSM) is deferred consume-to-advance, so the butterflies hold position but the stream stays aligned.
PORT-DEBT(fountain-rng-phase) narrows to the irregular `0x5531b0`; PORT-DEBT(butterfly-wander) is no
longer RNG-blocked (the drift waits on the movement FSM). 921 pass.

---

## 2026-06-09 (ckpt 96) — the town BUTTERFLIES are PORTED (`0xe29a` was never "wandering villagers")

USER (golden review #89) pointed out tiny ~3px butterflies that flit by the flowerbeds at the
settled town — "over the dark wood, below the ARMS/sword sign, above the dog" (retail flips ~2028 +
2138).  Chasing them corrected a mislabel that stood from ckpt 83 through 95: the 4 map **`0xe29a`**
EFFECT objects, called "wandering villagers" all along (the spawn excluded them, consuming only their
RNG), are the **butterflies**.

**The chase** followed the methodology's "RE via a live render trace at the SETTLED town, not the
hold."  No prior capture had hooked the particle band there, so I drove retail to flips 2028/2138
(`--seed-pin --lockstep`, `trace-retail.jsonl`) and captured frames + field-spec traces.  The
particle band (`0x493480`) rendered only the ported `0x18704`+`0x18708`; the EFFECT band
(`0x493ba0`) only townsfolk/cast — the butterfly was in neither known band.  A **blit trace**
(`blt_keyed` res/dx/dy) found it at the butterfly's screen position = **res `0x3fa`, 14×8 cels**; an
**emit trace** (`0x492670` cel_res + ret_va) named the producer `FUN_00493ba0` at world positions
matching the `0xe29a` census **1:1**.  Asset: res `0x3fa` = bank `0x146` (sprite slot 313 = bank−13,
32×32, sotesd.dll DATA — already group3-registered, just unused), clip `0x65ddf0` (decoded: 3-frame
flap, dur 4, looping); two colour variants (yellow + white) = per-instance frame_base 0/4/8/12.

**The port (`src/actor_spawn.c`).**  Added `0xe29a` to `TOWN_EFFECT_DEFS` (bank `0x146`, dst (0,0),
layer 12) + a `BUTTERFLY_CLIP`; the spawn now selects the per-code clip BEFORE the `0x426ec0` phase
draws (the draw COUNT is unchanged, so the shared LCG stays aligned — no townsfolk-phase regression).
`0xe29a` was previously excluded; now it spawns + renders via the existing `actor_render_static`
path.  **919 host tests pass** (`test_actor_spawn` updated: `0xe29a` def found, effect count 2→3,
the butterfly slot + flap clip).  Port blit trace: res `0x3fa` frames 0/1/2 emit on-screen as the
camera pans (frame 1600 @dx 116/180, frame 1850 @dx 491/555) — two yellow butterflies flapping by
the ARMS sign / flowers / dog, matching retail's placement (pushed to the feed).  engine-quirk #93;
`findings/in-game-intro.md` "The town BUTTERFLIES".  PORT-DEBT(butterfly-wander): per-instance
direction/colour (the white variant) + the RNG wander drift (the 5 `0x427670` draws are consumed but
the motion isn't applied) — Phase 2.  **Lesson:** to ID a small/ambient actor, capture the rendered
RESOURCE (blit `res` / emit `cel_res`), not just the actor code+bank.

## 2026-06-08 (ckpt 95) — the establishing REVEAL is PORTED (the center-out vertical iris)

The room-enter iris that opens the static town from black — a self-contained scene-transition
**fade-grid** (`src/scene_fade.{c,h}`, NEW).  A 10×120 grid of 64×4px cells over the screen, each
`state 0 opaque → 1 fading → 2 clear`.  **render** = `FUN_0048e920` (drawn after the letterbox,
`0x48c150:175`); **update** = the INLINE loop `0x499ab0:125-177` + the iris **pattern setters**
`0x49a890` (variant 0 center-out) / `0x49a740` (1 edges-in) / `0x49aae0`+`0x49aa00` (2 sweep);
**arm** = `0x439690:555-583` (`mode=req+0x28`, `variant=(rand*3)>>15`, `speed=req+0x2c`, fill cells).
Live town params (`runs/reveal-grid`, the `0x48e920` field spec): W=10 H=120 count=1200, mode 1,
speed 1000, variant 0.

**Corrects engine-quirk #90.**  The ckpt-90 attribution of the per-cell update to `FUN_0049af40`
was WRONG: reading `0x49af40`, it is the per-frame HUD/portrait/HP-bar animator (walks the 8 party
slots `room+0x4030`, lerps the fill bars + portrait fade timers, returns a counter) — it never
touches a 64×4 grid.  The real update is the `0x499ab0` inline loop + the `0x49a8xx` pattern setters.
The measured −8px/sim-tick = mode-1's **2 rows/tick** × the 4px row pitch (1×/sim-tick), not the
ckpt-90 "`0x49af40` 2×".  Fixed in `engine-quirks.md` #90, `findings/in-game-intro.md`, and
`retail_fields.json` (`0x49af40` renamed `hud_party_anim_update`; `0x48e920`/`0x499ab0` notes
corrected).

**Wired + verified.**  `enter_game` arms the grid (after the spawn burst, mirroring retail's order);
`game_render` steps it once/sim-tick after the camera easer + renders it after `letterbox_render`.
opaque sink = the letterbox cel (res `0x583`); **alpha sink = the true `0x5bd550` composite** of res
`0x458` frame[level] (the per-level GRAY MASK) through the descriptor **`g_pd_boot_group_e[19]`** (=
`*(0x8a93b8)`, the group-E ramp `0x8a936c` [19]: weight 1000, mode-2 subtract-blend = darken the dest
by the source).  The first cut KEYED-blitted res `0x458`, which drew the gray opaquely (USER: "white
outside, black inside, no transparency"); disassembling `0x48e920` (the `0x5bd550` call at `0x48eaa9`)
recovered the thiscall ECX descriptor Ghidra dropped (`runs/reveal-desc` confirmed it's unique vs
ramp_a/b[19] → the group-E ramp the port already builds).  VERIFIED on the composited capture
`port_frame_01160`: the blue town sky shows through and darkens to near-black across the receding edge
(true translucency).  Host-tested (`test_scene_fade.c`, 5 cases); **919 pass** (+5);
ledger 204/199 (+5: the render `0x48e920` + the 4 iris pattern setters; the partial `0x499ab0`/
`0x439690` slices stay bare-VA).  Port blit trace: black tiles 1490 → 650 → 320 over
frames 1118→1200, the center-out iris settling to the 64px letterbox by ~sim-tick 25 (= retail's
240→64).  Real composited frames pushed to the feed → **USER: "the iris looks reasonable."**
PORT-DEBT(scene-fade-rng-phase): the iris variant is RNG but the port's post-spawn LCG phase isn't
retail's (the spawn doesn't consume the full 238-draw burst, ckpt 87), so the drawn variant is wrong
(2/sweep) — pinned to the live town 0; that + the skipped black-load window's start offset are Phase 2.

**BMP capture footgun (USER caught it — "how is in-game capture broken? we capture a video of the
whole intro before").**  It was never broken: I'd passed a WSL `--capture-dir /tmp/…` the Windows exe
can't `fopen`.  The default capture-dir (the game dir after chdir) works; in-game capture is fine.
Added a `fopen`-failure hint in `capture_primary_to_bmp`.

## 2026-06-08 (ckpt 94) — ARCHE RENDERS — the in-game intro cast is COMPLETE (her body banks are EXE-embedded sprites)

Closed the ckpt-93 gap ("all characters except arche").  Arche the party leader now renders her
own sheet — and the heavy Phase-2 "party band" plan turned out to be **unnecessary** for the
arrival scene.  A live census (`runs/cutscene-cast`, the `0x493ba0` field spec) showed Arche
(`0xc35a`) is drawn by the **SAME `0x493ba0` EFFECT path** as the rest of the cast — row0 bank
`0x8b`, clip `0x62a8c8` (decoded byte-identical to the idle clip `0x6290e0`), world (41600, 45600),
dst (−30,−24), facing 1, layer 13.  Her ONLY blocker was bank registration (slot 126 = bank−13 had
no sprite).

A field-spec **chain read** of the live retail slots (`g_ar_sprite_table[idx]` @ `0x8a7640+idx*4` →
slot → `+0x40` resource_id; `runs/arche-res`/`arche-params`, validated against known slots
168/222/338) pinned her 4 banks `0x8b`–`0x8e` (slots 126–129) to resources **`0x570`–`0x573`**
(group 3, type 2, scale 1; dims 80×80/80×80/80×96/80×80).  **The trap (engine-quirk #92):** those
ids are type `WAVE` (sounds) in sotesd.dll but type `DATA` (sprites) in **sotes.exe**'s own `.rsrc`
— a numeric collision that derailed ckpt 90 (its `0x8b→0x4fb` read was the *sound* table).  The
playable-character sheets live in the EXE; retail loads them via `FindResourceA(NULL,…)`.

**Ported.**  New `ar_register_party_exe_sprites` (asset_register.c) registers slots 126–129 with
`settings = g_sotes_exe` — the module `ar_sprite_decode` reads via `FindResource`, so they load
from the user's own `sotes.exe` at runtime, never embedded (USER directive).  Called in `enter_game`
after `load_town_scene` opens the exe datafile.  `actor_spawn_cutscene_cast` (actor_spawn.c) gains
an Arche row: handle `0x5f5e165`, `bank_override 0x8b` (her dramatist bank is 0 → `party_resolve_
spawn` yields 0), her own settled world_y 45600 / dst_y −24.  She renders via `actor_render_static`
(one keyed cel, like the rest of the cast).

**Verified.**  914 host tests pass (no regression); both GUI + debug exes build clean.  Port blit
trace (settled frame 2200): res `0x570` frame 1 emits at screen (258, 304) = world (41600,45600) −
settled cam + dst (−30,−24).  **USER-confirmed on the live port window: "everyone is rendering
correctly now."**  `findings/in-game-intro.md` "ARCHE RENDERS"; quirk #92; ledger 199/194 unchanged
(no function ported — registration data + a spawn-table row).  PORT-DEBT(cutscene-party-chars)
narrowed: the static-cast Arche, not yet the party band `0x4997b0`; her multi-part body `0x8c`–`0x8e`,
the walk-in roll-in, and the live-actor handle registry (dialogue) remain Phase 2/3.

---

## 2026-06-08 (ckpt 93) — the DRAMATIST RESOLVE + arrival-cast spawn PORTED; Arche's Mother (`0xc440`/`0xb5`) renders

Ported the ckpt-92 RE (the dramatist table proof) into the port.  New module **`src/party.{c,h}`**
ports the static "Get Dramatist Info" table `DAT_006b6ea8` (79 `{handle, code, bank}` rows —
numeric facts only; the character names stay in `docs/proofs/dramatist-table.md` + the dump
tool, not embedded as story content) + **`party_resolve_spawn`** (= `0x41f200:54-69`, the
handle→code/bank lookup; the `0x41f0e0` spawn path passes the activator's param_4 = 3, so the
row code overrides only when `code_in==0`, and the row bank `+0x30` overrides the archetype's
facing default) + **`party_archetype_default_bank`** (the per-case `if (sVar17==0)` arm, the
RE'd subset read off the decompile).

**`actor_spawn_cutscene_cast` rewritten** to spawn the arrival family by their RE'd `0x41f0e0`
params and resolve each through the dramatist system: **Dr. Barnard** (by code → `0xeb`),
**Arche's Father** (handle → `0xe3`), **Arche's Mother** (handle → OVERRIDE bank `0xb5`).  Mom's
`0xb5` is registered in group3 (idx 168; the port resolves bank→`g_ar_sprite_slots[bank-13]`),
so she now RENDERS her own sheet — fixing her absence (the port previously showed only the
far-right map townswoman `0xa6`).  Positions = the wagon's settled anchor (41600) + the RE'd
anchor-relative offsets, reproducing the census {Barnard 67200, Father 49600, Mother 38400}
exactly.  The frozen `CUTSCENE_CAST_DEFS` identity snapshot is retired.

The one remaining gap is **Arche the GIRL** (`0xc35a`, dramatist bank 0): she is the party
LEADER (party band `0x4997b0`), her body banks `0x8b`–`0x8e` are loaded by the unported new-game
party sprite path (UNREGISTERED → would cull), deferred to Phase 2.  **914 pass** (+3,
`test_party.c`); ledger 199/194 unchanged (bare-VA slices).  Settled-town port|retail
side-by-side pushed to the feed → **USER-confirmed: "all characters except for arche are there
and positioned correctly."**  PORT-DEBT(cutscene-party-chars) updated; quirk #91 (the RE) stands.
Plan: `docs/plans/party-character-system.md` (Phase 1 port done bar Arche).

---

## 2026-06-07 (ckpt 89) — the SKY-AMBIENT particles (chimney smoke `0x18704`) PORTED + trace-faithful placement + full-intro side-by-side video

Chip 4 of the in-game-intro arc, on the ckpt-88 particle pool/alpha path.  Ported the
town's second particle system — `0x18704`, the **chimney smoke** (emitter `0x112e2`
`0x54f980:150`, clip `0x644b58` 6-frame ONESHOT, layer 6, ramp_b alpha) — in
`src/particle.{c,h}` + `main.c`.  USER-confirmed 1:1 ("smoke looks 1:1"); the USER
independently spotted the same chimney smoke in retail.  **911 pass** (+5); ledger
unchanged (bare-VA slices).

Per the USER directive "tracing to verify correctness is always good", mined the existing
`0x493480` trace + a fresh `runs/sky-facing` field-spec capture, which confirmed the render
path (layer/bank/frame/clip/Y all exact) AND caught two RNG-independent placement bugs now
fixed faithfully: (a) the emitter **anchor** was a HARDCODED `+1600` (USER: "are you
hardcoding the offset?") — the faithful `0x557370` mode-1 anchor is render-state `+0xc/2`,
and the invisible `0x112e2` trigger has `+0xc==0` → anchor 0 (removed the constant); (b) the
particle **facing** is `+0x2c==1` for every particle (capture), so x integrates `+vel_x/100`
(no flip) → the sky drifts LEFT like retail (the port spawned facing 0 → drifted right).
After both, the port sky world X `[51440..113369]` ≈ retail `[50690..114356]`.  The town's
`+0x13e0` band renders only `0x18704`+`0x18708`, both now ported (no particle remainder).
quirk #88; `findings/in-game-intro.md` "The SKY-AMBIENT particles".

Built a USER-requested **full-intro side-by-side video** (frame-matched retail|port across
title→newgame→prologue→town, 64 anchor-aligned pairs; `/tmp/intro_sidebyside.mp4` + feed
montage).  Verdict: title/menu 1:1, prologue aligned, town establishing 1:1.  The one clear
divergence the full sequence surfaces is retail's **"Town of Tonkiness" area banner**
(~retail flip 1600+), MISSING in the port — the `0x5a00c0` scripted-overlay debt
(`ingame-nontile-layers`), now precisely timed; it's the next chip toward whole-scene 1:1.

---

## 2026-06-07 (ckpt 88) — the FOUNTAIN SPRAY particle subsystem RE'd + live-ground-truthed + clips decoded (Chip 3, RE milestone)

Read the engine's particle subsystem end-to-end and ground-truthed the fountain from
the existing ckpt-86 capture; no port code yet (the port is the next chip).

**Architecture (engine-quirk #87).**  The `+0x13e0` DEVICE band is a 1024-slot particle
pool: alloc `0x557370` (round-robin free-slot / evict-oldest, parent-relative position)
→ config `0x557550` (per-code switch; fountain codes install bank `0x1aa`=res `0x408`)
→ per-tick physics `0x46e510` (gravity / integrate / clip / alpha-fade / expire) →
render `0x493480` default arm via the alpha-blit `0x4917b0`.  The emitters are CHARACTER
props the port already spawns (ckpt 79): the fountain `0x112e5` (`0x54f980:218`) emits a
`0x18708` water droplet each primary sim-tick; `0x112e2` (`:150`) emits a `0x18704` sky
particle every 6th tick.

**Live ground truth + clips.**  Mined run A (`runs/rng-census-repin`, `0x493480` 5924
render calls): `0x18708` = fountain water (clip `0x6449c0` base0/count2/dur2/loop {0,1},
frame_base 6, layer 11, a ~158px column at the fountain, falling); `0x18704` = wide sky
ambient (clip `0x644b58` base0/count6/dur20/oneshot, frame_base 8, layer 6).  ~58-69
particles alive/frame.  Clips decoded from the exe (decoder validated vs IDLE_CLIP).

**Parity bar (refines quirk #77).**  Filtered to the actual LCG (`va==0x5bf505`), the
hold stream is regular per-sim-tick (238 spawn + period-6 `[6,14,6,14,14,14]`), consumed
only by per-sim-tick updaters; the render draws no LCG.  In `--lockstep` (1 present/tick)
there is no per-present variance, so under the re-pin the spray is bit-exact portable —
contingent on reproducing the per-tick consumption ORDER (entangled with the co-resident
consumers, the broader Phase 2).  Corrects a first mis-read that wrongly attributed
"render-pass RNG bursts" (those were blit hooks, not the LCG).

Commits: the architecture writeup (`4748468`), the ground truth + clips (`3480b53`),
the status checkpoint (`473488d`).  `findings/in-game-intro.md` "The FOUNTAIN SPRAY";
engine-quirk #87.

**Then PORTED (same session, `2cb8647`) + USER-confirmed "the particles blending looks
correct."**  `src/particle.{c,h}` (NEW): the 1024-slot `+0x13e0` pool (alloc `0x557370`),
the fountain water `0x18708` (config `0x557550` bank `0x1aa` + clip `0x6449c0`, emitter
`0x54f980` case `0x112e5`, step `0x46e510` case `0x18708`), wired into `main.c`
(`game_actor_update`/`game_actor_walk`).  **The ALPHA render was the key fix:** a first
pass rendered the droplets OPAQUE (keyed); tracing the emit (`0x4917b0`) showed retail
blits them MODE-1 (alpha) via `&DAT_008a92e0[-sub_phase]` = `g_ramp_a[10 - sub_phase]`
(`0x8a92e0 = &g_pd_boot_group_a[10]`).  Added the mode-1 alpha present (`map_present`
case 1 + `game_present_blit` PRESENT_ALPHA → `zdd_blit_orchestrate`), so the droplets are
translucent like retail — retiring part of PORT-DEBT(present-actor-modes).  906 pass (+8),
ledger 199/194 unchanged (particle.c = bare-VA slices).  **NEXT (whole-scene 1:1):**
phase-match the particle RNG (PORT-DEBT(fountain-rng-phase), Phase 2 — needs the
co-resident per-tick consumers); the dark establishing-shot top gradient (ckpt 66/67);
the `0x18704` sky-ambient particles.

## 2026-06-07 (ckpt 85) — townsfolk FACING ported + USER-1:1 (a MAP field, NOT RNG); idle phase + fountain pinned to RNG

Phase-2 "matching half", first chip.  RE'd the three ckpt-84 RNG residuals and
found the **facing is RNG-free** — a deterministic map field — while idle-phase +
fountain are genuine LCG consumers.  So the facing landed standalone, USER-confirmed,
with no RNG anchor.

**Facing = the map sub-record `puVar1[4]`.**  The room-object dispatcher
`FUN_0058d460:96` computes `cVar12 = (-(puVar1[4]!=0) & 2) + 1` → 1 (normal) / 3
(mirrored) and forwards it as **param_8** to the EFFECT activator `0x41f200` (`:151`)
and the CHARACTER activator `0x431e30` (`:227`); `0x41f200:861` stores it at
render-state `+0x2c`.  `FUN_0044d160` mirrors only on `facing==3`: cel `frame += flip`,
`off_x = mirror_x - off_x`, where `flip = *(short*)(DAT_008a8440[bank])`.  Confirmed
live: **`0x8a8440` is a pointer array** whose entries deref to heap sprite-group
descriptors; the first short = the group's frames-per-direction (4 or 16 for the town
banks), so the mirrored cel = `frame_base + frames_per_dir`.

**Ground truth** (the `0x493ba0` census + a new `rs_facing` field; a one-shot read of
`DAT_008a8440` via `runs/read_fliptable.py`): of the 11 map townsfolk, **7 face 3**
(`c3be/c3dd/c3e6/c422/c42c/c441/c468`), 4 normal.  **Ported (898 pass, builds clean):**
`TOWN_EFFECT_DEFS` gains `facing`+`flip`; `actor_spawn_effect_fill_flip_table` fills a
bank-indexed stand-in for the global `DAT_008a8440`, wired into every `game_actor_walk`
`actor_render_static` call.  **USER-confirmed on the feed: "npc orientation matches
retail yes."**  PORT-DEBT `effect-sprite-table` extended.  quirk #85;
`findings/in-game-intro.md` "Townsfolk facing is a MAP field".

**The remaining two residuals are RNG** → need the **game_enter RNG anchor**: the
idle PHASE (`FUN_00426ec0`: `rs+0x72 = (rand()*clip.frame_count)>>15` — a random start
frame in the idle clip `0x6290e0`) and the FOUNTAIN SPRAY (band `+0x13e0`/`0x493480`;
`0x41f200`'s 8 rand draws are position-jitter + a `0x427b70` particle sub-spawn, helper
`0x427670` 20 draws, + per-tick `0x47b990`/`0x453960`).  Next: re-pin `DAT_008a4f94` at
`game_enter` both sides → port the spawn RNG consumers in order → 1:1.

## 2026-06-07 (ckpt 84) — the EFFECT townsfolk PORTED (positions USER-1:1); the residual pinned to RNG → Phase 2

Landed the EFFECT band (the standing townsfolk in the square) positioned 1:1 vs
retail, frozen on the idle clip's frame 0 — the wagon/STRUCTURE precedent
(position-first, animate next).  Built directly on the ckpt-83 census.

The RE was census + map cross-ref (no 27 KB switch read).  The render REDUCES to
`actor_render_static`: for a plain townsperson `FUN_00493ba0`'s static arm
(`LAB_004943d7` → `FUN_0044d160` describe → emit loop) emits exactly ONE mode-0
keyed cel — verified vs the hold blit trace (`0x5b9b70` carried `res`+`frame`; 18
keyed blits, one per townsperson; no `0x4917b0` shadow, no `DAT_008a9358`
color-remap fired).  The placement is FULLY MAP-DRIVEN: `world = (map (x,y) − dst)
× 100` where `dst` is the per-code render anchor (verified cel-for-cel against the
census `rs_x`; the +30 world offset cancels the −30 render dst → screen = map −
cam).  The 11 map townsfolk = 10 `0xc3xx` + `0xe2a5`.

Ported (898 pass, +1; commit `aeb7e90`): `actor_spawn_effect_from_map` +
`actor_spawn_effect_def_for_code` (`actor_spawn.c`, PORT-DEBT `effect-sprite-table`
— captured `{code → bank, dst, layer}`); `main.c` `g_effects` walked by
`game_actor_walk` via `actor_render_static` at layer 13.  Ledger 199/194 unchanged.

USER-confirmed: "the NPCs are rendering at the correct positions."  render_diff
(port 1200 ↔ retail 1500): on-screen townsfolk match on resource + position (zero
`[rect]`/`[state]`).  The residual is now PINNED to the **RNG pillar** (USER
directive → Phase 2): (1) townsfolk FACING — some render flipped (`0x44d160`'s
`facing==3` mirror, unset in the port + `flip_table NULL`; facing likely RNG); (2)
townsfolk idle PHASE (frozen frame 0; clip `0x6290e0` + stepper ported, per-actor
start phase staggered/likely RNG); (3) the FOUNTAIN PARTICLE SPRAY — the whole
`+0x13e0` band (`0x493480`, res `0x408`) is missing (USER crop: a purple/blue
sparkle spray + leafy particles; RNG positions).  NEXT: the scene-wide
RNG-consumer census — hook the LCG `FUN_005bf505`/`DAT_008a4f94`, log every draw's
`ret_va`+`rngcalls`+value, enumerate + match consumption order both sides.  Retires
the ckpt-73 defer-all-RNG.  `findings/in-game-intro.md` "The EFFECT townsfolk
PORTED"; quirk #84; PORT-DEBT `effect-sprite-table`/`effect-anim-phase`/`effect-wanderers`.

## 2026-06-07 (ckpt 81) — the caravan's HORSES TROT: the per-tick actor anim wired + bit-verified

Built the per-sim-tick actor UPDATE pass and drove the ckpt-80 wagon's looping
clip with it, so the horses trot instead of freezing on frame 1.  The RE that made
it safe (engine-quirk #82): `FUN_0054f980`'s case-`0x1872d` (`:911-970`) splits into
an **unconditional frame-stepper** (`:911-928`, gated only on the clip; byte-
identical to `anim_clip_advance`, reads no RNG/clock) and a **gated RNG behaviour**
(`:929-970`, breaks out unless primary + the scene-lock `*(0x8a9b50+0x27a8)==0`,
then draws the LCG for idle/wander — the deferred #77 layer).  So the trot is a
pure function of sim-ticks, portable in isolation; the wander stays deferred.

Ported (pure + host-tested): `actor_render_state` gains the anim block `timer`
(+0x70)/`done` (+0x74); **`actor_anim_advance`** (a thin adapter to the single
ported stepper `anim_clip_advance`); **`actor_pool_update`** = the `0x46cd70:123-169`
band walk (advance every active render-state with a clip — the 32 static actors
no-op on clip NULL).  `main.c game_actor_update` runs it on the SAME sim-tick gate
as the camera easer (`hold & 1`), before `camera_follow_step` (retail `0x439690`
order :1108→:1123), with a `CALL_TRACE_BEGIN(0x46cd70)` mirror.

LIVE-VERIFIED at the byte level (port blit trace, settled cam 12800, one 144-Flip
cycle): the wagon (bank `0x175`) is 3 keyed cels **res `0x3ec`** at x 160/288/416;
the body cel steps **5→2→3→4→5** every 36 Flips (18 sim-ticks) while the two fixed
cels hold frames 0/1; the `0x46cd70` mirror reports `advanced:1`/tick.  Correction:
the wagon's render_id is res `0x3ec` (asset_register idx 215), NOT `0x058f` as
ckpt-80 noted (fixed in FRONT/quirk #81).  Montage + full settled frame pushed to
the feed (USER visual confirm).  **896 pass** (+3); ledger 199/194 unchanged
(bare-VA slices).  quirk #82; PORT-DEBT `actor-protagonist-clip` narrowed to the
RNG behaviour + the cutscene roll-in.  Writeup: `findings/in-game-intro.md`
"The horses TROT — the per-tick anim wired".

## 2026-06-07 (ckpt 80) — the town intro `0x1872d` is the arrival HORSE-DRAWN CARAVAN: render arm + spawn RE'd + wired + USER-confirmed

Ported the one animated town actor.  Three commits + the horses fix:

- **Render arm** (`af31c69`): `actor_render_protagonist` = `0x491ae0`'s
  case-`0x1872d` (`0x491ae0:112-192`).  Part 2 (the body) is byte-identical to
  `FUN_0044d160`/`actor_render_describe`; the arm wraps it with two fixed
  bank-`0x175` cels (frame 0 @ x-256, frame 1 @ x-128) → a 3-cel composite at a
  128-px pitch.  Factored `actor_emit_part`/`actor_emit_layer` out of
  `actor_render_static`.  +3 host tests.

- **Spawn, fully RE'd** (`08fd0be`): `0x1872d` is outside the 70000 CHARACTER
  range, so it's NOT a map object.  It's spawned by the town intro cutscene script
  **`FUN_004d7d80`** (`case 0x334be` = room 210110 / area `0xd2`, gated on event
  flags `0x5f76805`/`0x606aa4f`) → **`FUN_00431d10(0, 0x1872d, anchor=0x65,
  x=0x3200, 0, 0)`** (the by-code `+0x11e0` spawn helper) → **`0x431e30`
  case-`0x1872d`** (layer 9, facing 99, clip `&DAT_00671c48`, sprite via
  **`FUN_00426db0(0, 0x175, 0, …)`** — the `+0x48` FILL PRIMITIVE, retiring the
  ckpt-79 "lazy fill not RE'd" unknown).  `actor_spawn_protagonist` + the
  `game_actor_walk` dispatch.  Drove the port through the pan; a with-/without-
  `0x1872d` rebuild diff at the settled camera isolates exactly its pixels = a
  covered WAGON, not a person.  +1 host test.

- **The HORSES fix**: the first render froze the body on frame 0 (redrew the
  wagon-left cel → "cut in half" — USER).  The body is the animated **horses**.
  Decoded the clip `&DAT_00671c48` from the user's exe `.rdata` (file off
  `0x271c48`): base_sprite 2, 4 frames, dur 18, looping, delta {0,1,2,3} → body
  cels 2..5.  Pointed the render-state at a reconstructed `WAGON_CLIP` → the body
  draws sprite 2 = the horses.  **USER-confirmed on the feed: "that definitely
  matches retail."**

**893 pass** (+4 net); ledger **199/194 unchanged** (bare-VA slices of
`0x491ae0`/`0x431e30`; `0x426db0` cited by bare VA).  quirk #81; PORT-DEBT
`actor-protagonist-clip` (horses frozen on frame 2 — the per-tick stepper +
cutscene roll-in deferred).  Writeup: `findings/in-game-intro.md` "The 0x1872d
SPAWN + the arrival WAGON".  (ckpt 78/79 — the spawn byte-proof + the render
census + the minimal CHARACTER spawn — are recorded in `FRONT.md`/`HANDOFF.md`.)

## 2026-06-07 (ckpt 77) — the town ACTOR render side PORTED (FUN_0044d160 + 0x491ae0 default arm + present mode 0)

Ported the render half of ckpt-76's "implement the NPCs" arc — pure + host-tested,
ahead of the spawn RE.  32/33 town actors render through one path: `0x491ae0`'s
behaviour switch has no case for their codes, so they fall through to the **default
arm** (`caseD_11257` → `FUN_0044d160` → emit one node); `map_present` then blits them
as **mode 0** (keyed).  Ported that path bit-exact against the decompile (commit
`0533603`).

**Ported.**  (1) `draw_pool_emit_actor` = **`FUN_00492670`** — the actor analog of
`draw_pool_emit` (`0x4917b0`): the same 0x3c node, but mode = `bool(alpha != 0)` and
alpha lands in the param8 slot; a NULL cel emits nothing.  (2) **`actor_render.{c,h}`
(NEW):** `actor_render_describe` = **`FUN_0044d160`** (the static / animated / mirrored
/ angle sprite descriptor over the per-direction sprite table `actor+0x48`, stride
0x14, indexed by `actor+0xe8`) + `actor_render_static` = the **`0x491ae0` default arm**
(skip flag `+0x284`, layer `+0xfc` + the render-state `+0x284` override, describe, emit).
The actor + render-state are LOGICAL structs (the spawn fills them, like `anim_clip`'s
`anim_state`); only `actor_sprite_row` (0x14) is a pinned data layout.  (3) `map_present`
**MODE 0** (`FUN_0048eac0` case 0): project like a tile but the cull box comes from the
CEL dims (`cel+0x1c/+0x20`) via a new `present_dims_fn` callback → `PRESENT_KEYED`
(`FUN_005b9b70`).  `dims=NULL` keeps the tile-only contract (`town_render_step`).

**Validated.**  The render-state offsets the port models (`world_x@+4`, `world_y@+8`,
`facing@+0x2c`, `clip@+0x6c`, `frame@+0x72`) match the ckpt-76 live-RE'd `0x491ae0`
field spec exactly (`rs_x`/`rs_y`/`rs_kind2c`/`rs_clip`/`rs_frame`) — so the struct
modelling is pinned to live retail data; the logic is host-tested bit-exact vs the
decompile.  **883 pass / 0 fail / 6 skip** (+18).  Ledger **199/194** (+`FUN_0044d160`,
+`FUN_00492670`).  Both GUI builds clean.  PORT-DEBT `present-actor-modes` narrowed
(mode 0 done; modes 1/2 + the wiring blocked on the spawn) + `actor-occlusion`.

**Still open (the next arc — needs the harness, then the human for pixel-verify):** the
**SPAWN** (the `+0x11e0` band activator) is the gating input.  Narrowed: NOT `0x560e60`
(8 party actors) / NOT `0x584710` (refuted) / NOT the static `+0x1d0`+`+0x1d4` writers
(`0x456a50` find-by-code, `0x487dc0` cell/collision); it is the **entity subsystem**
(`0x42eb20`/`0x4282f0`/`0x429060`/27 KB `0x41f200`) processing the **DATA 1022 layer
entries** (86, `map_data` parses them) via `FUN_00587e00`'s layer pass.  Then the
`0x1872d` animated arm (1 actor), then wire the band walk into `game_render` +
`render_diff` vs retail flip 1500.  `findings/in-game-intro.md` "The town ACTOR render
side".

---

## 2026-06-07 (ckpt 76) — the town NPC/actor RENDER PATH RE'd live + the trace tooling hardened

Next task: **implement the NPCs.**  User direction: *consult the runtime trace to
track down the code paths*, and *improve the trace tooling — as solid a foundation as
possible to probe ground truth, pinpoint code paths, and document them for future
traces*.  Did the RE + instrumentation half (the render-side port + spawn + wire follow).

**Tooling (the durable foundation).**  Added the reusable **`thischain`** field source
to the Frida agent (`ctReadField`): like `chain` but ROOTED at the `__thiscall` `this`
(ECX) + pointer hops + `off`, so it reads a field BEHIND a this-pointer (an actor's
render-state at `*(actor+0x40)+off`) — the primitive for probing any entity by its live
`this`.  **Annotated** `0x491ae0` (the actor render entry: behaviour `+0x1d4`, the
`+0x48` per-direction sprite table, render-state pos/clip/frame), `0x560e60` (actor
reset → spawn caller via the auto `ret_va`), and `0x584710` in
`tools/flow/retail_fields.json` — documented for every future trace (commit `16894a5`).

**RE (two seed-pinned/lockstep retail captures of the town hold, flip 1434/1500).**
The MAIN actor band is `DAT_008a9b50+0x11e0` (0x80=128 slots), render-emitted by
`FUN_00491ae0` (the per-frame driver `0x48c150`'s free-roam branch) and updated by
`FUN_0054f980` (the per-tick `0x46cd70`) — one of six bands.  **33 active actors at
flip 1500: 32 STATIC** (render-state clip `+0x6c`==0), **1 ANIMATED** (`+0x1d4`=`0x1872d`,
the protagonist/key NPC).  Crucially **32/33 behaviour codes are NOT explicit
`0x491ae0` cases → they fall to the DEFAULT arm `caseD_11257` → `FUN_0044d160`** (the
static-actor descriptor builder: `actor+0xe8` dir → the `actor+0x48` stride-0x14 sprite
table → cel `(bank, frame_base+facing)` at the render-state world pos) → the emit tail →
`FUN_00492670` (the actor analog of `draw_pool_emit`; node mode 0=keyed / 1=alpha).  So
the behaviour code drives the **AI** (`0x54f980`, RNG motion deferred ckpt-73), NOT the
render — **one function renders nearly the whole town.**  The 36 mode-0 keyed `0x5b9b70`
blits at flip 1500 (res `0x403`/`0x426`/`0x459`/…) are **exactly the ckpt-75 render_diff
residual's named NPC banks**.

**The spawn — narrowed, not yet ported.**  The band is a **pre-allocated 128-slot pool**
(`0x586010:476-506` calls `FUN_0058cf60(0x40)` 0x80× for the main band; `0x58cf60` zeroes
a slot, `+0x1d0=0`), so the per-room "spawn" is an **ACTIVATE + configure** running AFTER
`0x586010`'s `"Init Objects"` marker (`:508`).  The behaviour codes are **data-driven**
(never literal in the decompile) → an **entity-by-id** subsystem (ROADMAP `0x420000`);
the trace ruled out `0x560e60` (= the 8 PARTY actors, `ret_va=0x59f578` inside `0x59f2c0`)
and `0x584710` (never fired).  **NEXT** (one fresh-context arc, ends in pixel verify):
find the `+0x11e0` activator (hook post-"Init Objects"), port the render side
(`FUN_0044d160`+`0x492670`+the default arm), wire `map_present` modes 0/1, diff vs retail
flip 1500.  No C touched (865 pass); engine-quirk #78; `findings/in-game-intro.md`
"The town ACTORS"; PORT-DEBT `present-actor-modes` (the render-emit half lands there).

---

## 2026-06-06 (ckpt 75) — the establishing-shot cinematic LETTERBOX RE'd + ported + blit-trace 1:1

Closed the ckpt-74 next chip — the town frame's single biggest missing layer (the
320 `0x583` blits/frame).  RE'd the producer straight from the captured retail blit
trace (`/tmp/blit_town_retail`): the 320 res-`0x583` blits' return addresses
(`0x8c48a`/`0x8c4fe` + image base = `0x48c48a`/`0x48c4fe`) sit inside
**`FUN_0048c150`** (the per-frame world driver), lines **124-162** — NOT the
`0x5a00c0` overlay the ckpt-74 note hypothesized.  Two grid-fill loops run after the
backdrop present pass: the BOTTOM bar (`in_ECX+0x44` rows, ret `0x48c48a`, dy
416-476) then the TOP bar (`in_ECX+0x48` rows, ret `0x48c4fe`, dy 0-60), each tiling
a 64×4 opaque cel (main sprite-pool slot 41 = res `0x583`) at 64px column pitch (10
cols) and 4px row pitch; both heights 64 → the quirk-#74 letterbox (black rows 0-63
+ 416-479, 640×352 cinematic window).

**Ported** `src/letterbox.{c,h}` (the two loops verbatim; the inner column loop runs
while `(dx+0x80)<0x281`).  4 host tests, incl. the 64/64 town grid asserted
bit-exact against the 320-blit retail trace.  Wired in `main.c`:
`game_letterbox_blit` resolves `&g_ar_sprite_slots[41]` frame 0 →
`zdd_object_blt_onto`, called in `game_render` after `town_render_step`; heights
armed to 64 in `enter_game`.

**Verified** two ways: (1) `render_diff --retail-frame 1500 --port-frame 1200`
dropped the town-frame divergences **356 → 36** — all 320 `0x583` blits now match
retail on identity + geometry + DDraw state, and the 36 remaining are exactly the
deferred RNG-driven actor/banner/tree banks (`present-actor-modes` /
`ingame-nontile-layers`); (2) port frame 1200 pixel check — rows 0-63 + 416-479 are
`(0,0,0)`, row 64 is the sky band.  **USER-CONFIRMED on the feed.**

865 pass / 0 fail / 6 skip (+4).  Ledger 197/192 unchanged (`letterbox.c` is a
bare-VA slice of the unported `0x48c150`).  parity-ledger #8; engine-quirk #74
updated with the proven producer; PORT-DEBT `ingame-letterbox` (the 64/64 heights
stand in for the unported `0x5a00c0` cutscene op that writes the scene-object
`+0x44`/`+0x48`; the grid-fill geometry is bit-exact).  Finding:
`findings/ddraw-blit-trace.md` "The TOWN frame".

## 2026-06-06 (ckpt 73) — actor-band residual PINNED to the RNG pillar; the shared LCG stream is non-deterministic run-to-run even under `--seed-pin`

Ran the ckpt-72 directed live check (the #76 "To confirm").  Drove retail TWICE
(`--seed-pin --lockstep --no-turbo`, same in-game trace), hooking the per-sim-tick
actor-update boundary `FUN_0046cd70` and snapshotting the shared LCG state word
`DAT_008a4f94` there — a new `rng` field in `tools/flow/retail_fields.json`,
tagged with the deterministic `g_sim_tick` (reset at game_enter).  8644 in-game
sim-ticks common to both runs.

**Result: `rng` matches 0/8643 sim-ticks** — the shared stream is at a
different phase at *every* in-game tick despite the pinned seed and the
deterministic sim-tick index.  Smoking gun: at `prologue_enter` BOTH runs are on
the IDENTICAL flip 946 yet rng differs (`0x84654e6f` vs `0xa79a2d6e`) → at the same
flip the engine has drawn a *different number* of LCG values.  Mechanism: a
per-PRESENT consumer × the non-deterministic presents-per-sim-tick count (quirk
#75) desyncs the stream phase, which never re-converges.  (The `a0_clip`/`a0_frame`
actor-anim fields matched 8643/8643 but trivially — main-band slot 0 was inert all
run; `rng` is the real signal.)

Since `FUN_0054f980`'s behaviour cases draw this exact LCG `FUN_005bf505` ~40× per
tick (idle-wait `+0x5c`, the idle→wander branch pick, wander move-offsets →
`FUN_00450ef0` — static two-witness), a divergent stream makes the actors choose
different waits/dirs/positions run-to-run = the #75-addendum ~6.7k-px actor-band
residual.  **Conclusion:** the residual is the RNG pillar (parity-model pillar 3),
and it is NOT closable by the camera's `g_sim_tick` anchor — an RNG-reading
subsystem needs its own RNG anchor (snapshot/restore `DAT_008a4f94` at the
game_enter sim-tick, both sides).  This makes the #75 "anchor each subsystem
separately" decision mandatory for the actor layer; the parity bar for the band is
"data-1:1 given a matched RNG state" (retail-vs-retail isn't observed-1:1 here).
New tool `tools/rng_tick_diff.py`.  No code/test change (a pure RE/harness finding;
854 pass / 0 fail / 6 skip unchanged).  Engine-quirk #77; `findings/in-game-intro.md`.

## 2026-06-06 (ckpt 72) — the actor ANIMATION cycle RE'd + frame-stepper ported (rides the sim-tick clock)

Took the ckpt-71 directed next ("RE the NPC/actor system + its animation cycle,
then pin its counter") to completion. Traced the per-tick UPDATE path —
`FUN_00439690:1108` → `FUN_0046cd70(1)` (once per sim-tick) → `FUN_0054f980` per
actor — which is distinct from the render/emit pass `FUN_0048c150`. Every
animating behaviour in `0x54f980` runs the byte-identical inline frame-stepper on
the render-state anim fields (`+0x6c` clip / `+0x70` timer / `+0x72` frame /
`+0x74` done): `timer++`; at `>=clip.dur` advance the frame and reset; at
`>=clip.count` either loop to `clip.loop_to` or, for a one-shot clip, freeze on the
last frame and raise `done`. The clip `seq` is a fixed **0x154-byte, 32-frame**
descriptor (count@`+0x42`, dur@`+0x44`, oneshot@`+0x48`, loop_to@`+0x152`, base +
per-frame sprite-delta + per-frame x/y offset) — confirmed by two witnesses: the
stepper and the renderer `FUN_00491ae0` case 0x1872d. The clip is (re)set on a
state change by `FUN_0040afe0`/`FUN_0041e600`, reset-on-change only.

**Ported (pure, host-tested bit-exact): `src/anim_clip.{c,h}`** —
`anim_clip_advance` (the stepper), `anim_state_set` (the change-gated set),
`anim_clip_sprite` (base+delta); the `anim_clip` descriptor pins the retail layout
with `_Static_assert`. 8 host tests cover the loop trajectory, one-shot hold, the
duration gate, the NULL guard, the change-gated set, and the sprite id. **854 pass
/ 0 fail / 6 skip** (+6); both GUI builds clean.

**Determinism conclusion (the point of the exercise):** the counter `+0x70/+0x72`
is a pure function of *(sim-ticks since clip-set)* — it reads no GetTickCount, no
Flip index, no RNG — so it is already deterministic under the camera's
`g_sim_tick` anchor (game_enter reset). The actor-anim subsystem needs **no new
pin**, refining the #75-addendum guess that it "reads a counter NOT the camera sim
tick". The leftover #75 ~6.7k-px actor-band residual is therefore a different
pillar — the RNG-driven behaviour (which clip plays / actor position): `0x54f980`'s
idle/wander cases draw the LCG `FUN_005bf505` for random waits + spawn offsets, and
clip-SET timing is downstream. Annotated for the live check (`retail_fields.json`
`0x46cd70`/`0x54f980` → `a0_clip/a0_timer/a0_frame`). Engine-quirk #76;
`findings/in-game-intro.md` "The actor animation cycle".

---

## 2026-06-06 (ckpt 70b) — the pan CADENCE + TRIGGER measured; port matches retail's trajectory

Chased the two synthetic stand-ins from ckpt 70 to ground truth. A retail
field-spec trace (`--seed-pin --lockstep --no-turbo`, the easer `0x43d1d0` + the
Flip `0x5b8fc0` hooked, a contiguous Flip whitelist across the pan) pinned both:
the **trigger** is `game_enter + 184` Flips (Flip 1616 still HOLD
tgt=128000/cap=0, Flip 1617 = PAN tgt=12800/cap=300; game_enter@1433), and the
**cadence** is the easer firing **once per 2 Flips** — the in-game sim
(`0x439690`, which calls the easer at `:1123`) runs at half the Flip rate, so
`cam_x60` is a STEP function (flat between sim ticks, e.g. 127990 held across
Flips 1618-1621), dropping 300 per 2 Flips at cruise.

The port presented once per frame → stepped the easer 2× too often. `main.c
game_camera_step` now gates the sim to every 2nd frame (`hold & 1`), and
`GAME_CAMERA_HOLD_FRAMES` is 184 (even, so the trigger Flip is also a sim tick,
matching retail's Flip 1617 = pan command + first easer tick). Captured the
port's `0x43d1d0` mirror and diffed the distinct-step `cur_x` sequence vs retail:
**identical** — 128000,127990,127970,127940,127900,127850,127790,127720,127640,
127550,127450,127340,… then cruise −300/2flips. So the port passes through the
same scroll positions in the same order; the pan is trajectory-1:1, and the
backdrop is pixel-identical at any Flip where the two share a `cam_x60`.

Residual (PORT-DEBT `ingame-camera-pan`, narrowed): retail's sim accumulator is
wall-clock-paced, so its startup has sub-tick jitter (a 4-Flip plateau at
1618-1621, a double-tick at 1616) a clean fixed 2:1 step can't reproduce → a ~2-3
Flip phase offset mid-pan (≤1 step = 300u ≈ 3px, transient; zero at the hold +
settled ends). Removing it needs the engine pace clock (map-object `+0x4068`) +
the cutscene-script trigger source, both downstream of the in-game sim /
`0x5a00c0` port. No code/test count change (cadence is a `main.c` glue tweak);
both GUI builds clean. Feed: "camera pan CADENCE matched to retail (ckpt 70b)".
Writeup: `findings/in-game-intro.md` "The pan CADENCE + TRIGGER measured".

**PAN backdrop diff — verified pixel-1:1.** Captured fresh retail pan frames
(`--no-turbo --seed-pin --lockstep`, `--capture-frames` + the easer/Flip field
spec) at Flips 1617/1660/1700/1740/1780 with their `cam_x60`
(127990/125690/120050/114350/108350), simulated the port camera to find the port
Flips landing on each (1304/1344/1384/1422/1462), captured those, and diffed each
camera-matched pair. **The backdrop is Δ0**: a horizontal/vertical shift-search
peaks sharply at `dx=dy=0` (no offset bug), and the pan-start `x=80` column is
all `(0,0,0)` difference (every building/wall/grass pixel identical). The
remaining diff is the **named missing layers only** — the exact signal we wanted.
NEW retail ground-truth (quirk #74): the establishing shot is **LETTERBOXED** —
solid-black bars over rows 0-63 (top) + 416-479 (bottom), leaving a 640×352
cinematic window, stable across the pan; this is the "dark top" the user noticed
(with a matching bottom bar), scene-scoped (absent in settled play), likely a
`0x5a00c0` overlay. Parity-ledger entry #7; feed: "PAN DIFF: backdrop is 1:1,
holes = the unported layers". Next chips: the letterbox (quirk #74), the banner +
foreground tree (`0x5a00c0`), the NPC actors (entity/spawn system).

## 2026-06-06 (ckpt 70) — the intro-PAN camera WIRED LIVE; scripted target-setters ported

The ckpt-69 easer is now **driven by a live camera in `main.c`** — the town
backdrop pans. `enter_game` stands up a static `camera_view g_game_camera`
(map extent `dim·0xc80`, 640×480 viewport) and spawn-snaps it with
`camera_apply_snap(128000, 12800)` (the `MAP_RENDER_CAM_TOWN_3F2` origin).
`game_render` steps it each frame via `game_camera_step`: the
`CALL_TRACE_BEGIN(0x43d1d0)` flow-trace mirror (the X-axis easer state read at
onEnter, per `retail_fields.json`) → `camera_follow_step` → `game_camera_to_mr`
projects the view onto the `mr_camera` subset the backdrop walk/parallax read,
so the town renders through the *current* scroll instead of the static const.

Ported the two camera-command arms of the in-game command processor
`FUN_00439690` (referenced by **bare VA** — only the setter slice of the 8866-B
fn is ported): `camera_apply_snap` (`:599-642`, the `+0x40` SNAP command — clamp
target to `[0, map−vp]`, cap=0/flag=0, JUMP cur=tgt, zero vel) and
`camera_apply_pan` (`:643-664`, the `+0x4c` PAN command — clamp + set target /
cap=speed / flag=0, leave cur/vel so the easer eases). Host-tested bit-exact (2
new tests). A hold timer (`GAME_CAMERA_HOLD_FRAMES=183`) fires
`camera_apply_pan(12800, 12800, 300)` at hold-end. **Feed-confirmed:** hold (cam
x=128000) → mid-pan → settled (cam x=12800, town left edge). Also added
`MAP_RENDER_CAM_TOWN_3F2_SETTLED` (x=y=12800) — the determinate settled camera
both sides share for a flip-anchored full-frame diff with no easer in flight.

State: **848 pass / 0 fail / 6 skip** (+2); ledger **197/1490 touched / 192
tested** (unchanged — easer/shake counted ckpt 69, the setters are a bare-VA
slice). Both GUI builds clean. REMAINING (PORT-DEBT `ingame-camera-pan`,
synthetic stand-ins): the pan TRIGGER timing (the 183-frame timer stands in for
the unported `0x5a00c0` cutscene-script op that writes the command) + the easer
step CADENCE (port per-frame vs retail per-sim-tick → per-flip pan rate differs).
Writeup: `findings/in-game-intro.md` "The camera is WIRED LIVE".

## 2026-06-06 (ckpt 69) — the intro-PAN camera easer located (HW watchpoint) + ported

The opening-town "establishing shot" is a **scripted** leftward camera pan, not a
leader-follow: a live camera-field probe (`game_enter@1434`→3600, the extended
`cam_*` target/speed fields) shows the target x snaps to a FIXED **12800** + speed
**300** once at hold-end (~flip 1617); Y never moves. The per-frame easer is
**`FUN_0043d1d0`** (`vel += 10/frame` capped at the speed, `cur ±= vel` toward
target, snap+decel) — called from `0x439690:1123`.

It was **invisible to static search** (dispatched through a heap function pointer),
so it was found with a **hardware watchpoint** on the view's heap `+0x60`: new
`tools/mem_watch.py --watch-chain … --hw` (chain-resolved heap address +
frida-17 per-thread DR watch; MemoryAccessMonitor livelocks on the hot view page).
One run pinned the writer insn `0x43d26d` + the exact value trajectory
127970→12800 (per-tick deltas 30,40,…,300).

Ported pure + host-tested **bit-exact vs the capture**: `src/camera_follow.{c,h}`
(`camera_follow_step`/`camera_follow_axis` + `camera_shake_apply` `0x43d340`), 6
tests → **846 pass / 0 fail / 6 skip**, ledger **197/1490 touched / 192 tested**.
Annotated into the flow trace (`camera_follow_step` @0x43d1d0 + the view fields).
Commits `d3fdc15` (HW-watch tool), `ea40ee4` (port + annotation). Process note:
reinforced that "annotate" = the flow-trace field spec (CLAUDE.md "Annotate as you
RE", commit `90574d3`), a separate lane from thiscall/struct tagging.
PORT-DEBT `ingame-camera-pan`: wire the stepped camera live + the scripted
target-set op. Writeup: `findings/in-game-intro.md` "The camera EASER located".

## 2026-06-05 (ckpt 66) — the in-game PARALLAX far-plane (sky + mountain)

On top of the ckpt-65 wired backdrop, the port now draws the **parallax
far-plane** behind the town tiles — the blue sky band + the mountains the port
previously rendered black.  User-confirmed in-game (port `game_enter@1116`, DATA
1022 town, frame 1200): sky (layer A bank `0x55`) + mountains (layers C/B banks
`0x58`/`0x59`) under the rooftops.

**RE (two independent witnesses).**  The background is drawn by `FUN_00490cd0`
(inline; called FIRST in the per-frame world driver `0x48c150:47`, the free-roam
path) and its twin `0x499100`→`FUN_00499560` (the establishing-shot/special path
via `0x48c6b0`) — both read the SAME 3-layer descriptor from the runtime grid's
front-header (`*(DAT_008a9b50+0x1048)`) via the select+blit pair
`0x417c40`→`0x5b9a40`, and agree byte-for-byte on its layout.  The descriptor is
written by the `0x587e00` PROLOGUE's `switch(param_2=room[0x44])` /
`param_3=room[0x43]`; the town (room 210110, area `0xd2`: A=4, C=1) → case 4 →
layer A bank `0x55`, C `0x58` baseY `0xf8` wrap 8 paraY `0xfa` (0.5×), B `0x59`
baseY `0xe0` wrap 8 paraY 0 (0.25×).  Writeup: `findings/in-game-intro.md` "The
PARALLAX far-plane".

**PORT.**  `src/parallax.{c,h}` (pure, host-tested): `parallax_select` (the
prologue switch), `parallax_render`/`parallax_strip` (`0x490cd0`/`0x499560`
arithmetic transcribed verbatim, incl. the signed magic-div vertical term + the
`[-0x1c,0]` clamp), `parallax_to_grid`/`_from_grid`.  Wired via
`town_render_parallax` (descriptor selected at load, drawn before the tilemap per
`0x48c150` order) and `main.c game_render` (`game_parallax_blit` →
`zdd_object_blt_onto` = `0x5b9a40`).  9 host tests; **836 pass / 0 fail / 6 skip**.
Ledger **193/188** (+2: `0x490cd0`, `0x499560`).  Not `differ_px==0` (the
establishing-shot/zoom camera differs — PORT-DEBT `ingame-establishing-zoom`);
asset+scale verified vs golden 1800 + user.

**Tooling / blocker.**  Added `--parallax-probe` (`frida_capture.py` + agent:
hook `0x490cd0` + its inner `0x417c40`/`0x5b9a40`, dump the descriptor + blits).
It revealed the **saved retail input-trace no longer navigates** under
`--seed-pin --lockstep --no-turbo` — retail sits on the title (no scene anchors;
injects fire but the title menu doesn't act).  So the in-game retail re-drive is
currently broken (title-menu input-injection black box / stale trace); ckpt 66
verified via the port-side drive + the existing golden instead.  The probe is
ready to live-confirm the descriptor once nav is restored.

## 2026-06-05 (ckpt 65) — REAL IN-GAME PIXELS: the town backdrop wired + rendering

The decode → grid → walk → present pipeline (every stage already pure +
host-tested) is now **composed into one per-room scene and WIRED into `main.c`**,
and the port renders the opening **town of Tonkiness backdrop** — the
half-timbered house, the vine trellis, the stone-block walls, ivy + grass —
**the same assets at the matching gameplay scale as the retail golden**
(user-confirmed; cross-checked vs golden flip 1800).

**The composition (pure, host-tested): `src/town_render.{c,h}`** — a thin scene
owning the shared state (parsed `map_data`, the runtime render grid, the 27-layer
`draw_pool`) run in engine order: `town_render_load` = `map_data_parse`
(`0x587970`) + `map_decode` (`0x587e00` town arms); `town_render_step` = the
backdrop slice of the per-frame driver `0x48c150` (`draw_pool_reset` →
`map_render_walk` `0x490f30` → `map_present` `0x48eac0`). 6 host tests
(`tests/test_town_render.c`): a real parse+decode of a minimal in-memory resource
through to the exact present op, plus the resolver-gate / empty / dry / unloaded /
malformed paths. Commit `552801b`.

**The Win32 glue (`main.c`)** — `load_town_scene(1022)` in `enter_game`:
`LoadLibraryExA("sotes.exe", AS_DATAFILE)` opens the EXE's own `.rsrc` (the
engine-time module `DAT_008a6e7c`, distinct from `sotesd.dll`), then
`FindResource`/`Lock`(DATA 1022) + `town_render_load`. **Live-verified the
*packed* `sotes.exe` `.rsrc` is readable at runtime** (Steam-DRM intact — no
Steamless needed): DATA 1022 = 152936 B, "MSD_SOTES_MAPDATA", 88×19×3, 86 layers.
The three engine globals are real callbacks: `game_sprite_resolve`
(`ar_pool_get_slot(bank)` = `&DAT_008a760c[bank]` + `ar_sprite_slot_frame` =
`0x418470`; bank→pool mapping verified — bank `0x62` → register idx 85 → resource
`0x433`, every town bank in the already-booted g5 batch), `game_bank_dims` (slot
width/height), `game_present_blit` (mode-3 CLIPPED → `zdd_object_blt_clipped`).
`game_render` clears black then walks the scene through `MAP_RENDER_CAM_TOWN_3F2`.
Live: `--input-trace` in-game-intro, `game_enter@1116`, 118k nonblack px / 212
colors. Commit `ccb6c89`.

**NOT `differ_px==0` — named residuals, ALL deferred layers (not logic bugs):**
the parallax sky/mountain far-plane + foreground trees + dialogue/caption overlay
(`0x5a00c0`, PORT-DEBT `ingame-nontile-layers`); the NPC actors (present modes
0/1/2, PORT-DEBT `present-actor-modes`); retail's zoomed-OUT intro establishing
shot at the flip-1150 hold that zooms to 1:1 by ~1800 (the camera scale field
wasn't in the ckpt-64 probe; PORT-DEBT `ingame-establishing-zoom` — so port +
golden don't share a camera at any single flip yet, the tiles are confirmed by
asset+scale match vs golden 1800, not a px-exact frame diff); and the per-sprite
palette tint (`render-palette-tint`, the `DAT_008a93fc`/`0x4182d0` ramp — the "bit
more color" retail shows). **827 pass / 0 fail / 6 skip.** Ledger holds
**191/1490 touched / 186 tested** (pure composition, no new `FUN_`). Full writeup:
`findings/in-game-intro.md` "The backdrop pipeline WIRED". **NEXT:** the parallax
far-plane → the actor renderers → the dialogue overlay (build out the in-game
layers on the now-proven tile base).

## 2026-06-05 (ckpt 64) — the camera/view object RE'd + first-frame value live-probed

The last non-wiring blocker for real town pixels was the **camera/view object**
(`cam[0x34..0x74]`), framed through ckpt 63 as a "dynamic-scroll rock with no
clean pure init." That is **refuted**. The camera *is* the view object — one
`operator_new(0x78)` struct allocated by the room-state ctor `FUN_004017d0:187`
and stored at `room_state+0x104c` (byte 0x104c = dword index 0x413). Its
room-entry init is clean: `FUN_00586010:854-872` sets the fixed 640×480 viewport
(`+0x64=64000`, `+0x68=48000`), zeroes the scroll origin (`+0x5c/+0x60/+0x74`),
and the two `FUN_00587d30` calls zero the `+0x24`/`+0x3c` sub-blocks (which hold
`+0x34`/`+0x4c`). Ported as `map_render_camera_init`.

**Live ground truth.** Added a `src:"chain"` field-spec source to the Frida agent
(a global-root pointer-deref: `*(*(0x8a9b50)+0x104c)+off`) + 9 `cam_*` fields to
`tools/flow/retail_fields.json`, then probed the in-game camera at the Flip,
twice, under `--seed-pin --lockstep`. The camera **snaps to `+0x60=128000`
(40 cells) / `+0x5c=12800` (4 cells) by flip 1093 and holds ~83 flips through
~1176** — the opening town first renders ~flip 1150, inside this stable hold —
then runs a **scripted leftward pan** (easing up to ≈ −300/flip). The probed
viewport matches the static init exactly, confirming the 586010 RE. The pan-onset
flip drifts a few flips run-to-run (the R3 render-pace phase pillar), but does not
touch the first-frame camera.

So the opening town's first frame uses a **determinate constant**, ported as
`MAP_RENDER_CAM_TOWN_3F2` (`src/map_render.{c,h}`; visible window cols 39-60 /
rows 3-18 over the 88×19 DATA-1022 grid). Real backdrop pixels now need only the
`main.c` wiring (sprite resolver `0x418470`, EXE-NULL banks `0x570-0x572`, the
`0x586010` sim slice). **DEFERRED** (PORT-DEBT `ingame-camera-snap`): the
spawn-snap + the intro pan (the dynamic-scroll engine). 2 new host tests →
**821 pass / 0 fail / 6 skip**; ledger unchanged at **191/186** (the init is a
slice of the bare-VA-referenced `586010`). Full writeup:
`findings/in-game-intro.md` "The camera/view object".

---

## 2026-06-05 (ckpt 63) — the in-game PRESENT PASS ported (FUN_0048eac0 + FUN_00490b90)

Ported the **consumer** side of the in-game draw pipeline — the pass that turns
the draw list into a blit stream. After the per-frame driver builds the
27-layer draw-node pool (`draw_pool` / `map_render_walk`, ckpts 58-61),
**`FUN_0048eac0`** walks the layers in order (layer index = draw-order key),
projects each node's world position to screen space, culls off-screen nodes, and
dispatches to one of four zdd blit primitives keyed on the node's mode (`+0x18`).

New module `src/map_present.{c,h}`: `map_present_project` is **`FUN_00490b90`**
exactly (the screen-space projector + four-corner viewport cull the mode arms
share); `map_present` is the 27-layer walk + mode dispatch. **Mode 3** — the
static-backdrop TILE path `map_render_walk` emits — is ported in full: project
with the node's own w/h as the cull box, then select the blit by node `+0x14` —
CLIPPED (`FUN_005b9bf0` / `zdd_object_blt_clipped`, the `param8==0` backdrop
path) or ALPHA (`FUN_005bd550`). Both primitives are already ported in `zdd.c`;
the engine cel handle in node `+0x00` becomes the blit `this`, handed to a
`present_blit_fn` sink (the `mr_sprite_fn` pattern) so the walk stays pure.

**This closes the `decode → grid → geometry → draw-list → present` chain** —
every stage from the map DATA resource to the per-node blit op is now a pure,
host-tested unit. Modes 0/1/2 (actor/sprite/scaled draws) are VISITED in
faithful order but deferred (PORT-DEBT `present-actor-modes`; reported via
`out_deferred`, never silently dropped — no ported producer emits them yet, and
their geometry reads engine sprite/paint_ctx internals). What remains for real
town-backdrop pixels: the **camera/view object construction** (`cam[0x34..0x74]`,
a dynamic-scroll rock) and **wiring into `main.c`**. 9 host tests → **819 pass /
0 fail / 6 skip**; both GUI builds clean. Ledger **191/1490 touched / 186
tested** (+2: `0x48eac0`, `0x490b90`). Commit `d6ad4b8`.

---

## 2026-06-05 (ckpt 62) — Phase-B B2: the field-bearing flow trace, built + live-verified

Brought the divergence-chasing loop up to openrecet's standard with the **LOGIC
drill-in** (`docs/plans/trace-tooling-phase-b.md` B2). `src/call_trace.{c,h}`
gains a per-frame `seq` (execution order) stamped on every row + the
`CALL_TRACE_BEGIN/FIELD/END` field-bearing event family (`I32/U32/F32/HEX`, plus
`_STUB`), assembled in a static buffer and fwritten atomically (4 host tests).
`tools/flow_diff.py` (+ `test_flow_diff.py`, 9 tests) aligns the per-frame call
chain by `seq` and names the first `[chain]` (call present on one side only) or
`[data]` (inputs matched, output diverged) divergence; `--field-timeline` is the
per-field state-drift localizer. `load_names` was fixed to key on our
`name,entry,…` `functions.csv` column order (openrecet's keyed on the name and
silently dropped every CSV row).

The Frida agent (`tools/frida/opensummoners-agent.js`) now reads same-named
retail fields per `tools/flow/retail_fields.json` (`src: global|arg|argderef`;
`i32|u32|f32|hex`; `retval` is an onLeave TODO) into an `f:{…}` payload with a
per-Flip `seq` mirroring the port; `frida_capture.py --field-spec[-only]` loads
the spec and auto-hooks its VAs (the bounded field-trace mode), and the batch
writer passes `seq`/`f` through verbatim.

**First probe + first result.** Seeded coverage with `rng` (the LCG word
`DAT_008a4f94`) read at the **Flip `0x5b8fc0`** — live validation exposed that the
title runner `FUN_0056aea0` keeps its do/while loop INTERNAL (onEnter fires once
at scene entry, not per frame), so the Flip is the right shared per-frame VA, not
`0x56aea0` (the port externalized that loop into `title_scene_step`). The trace
immediately confirmed the title-sparkle RNG consumption is **data-1:1**: port and
retail both converge to the identical end state `0x404a0a8f` (same total draws),
with the per-flip divergence attributable to the **R3 title-pace skew** (the phase
pillar — port anchor `subtitle_anim_start` @flip 437 vs retail @897; the sparkle
is compressed into fewer port flips), NOT logic. A textbook data-1:1-vs-observed
call. Commits `587016f`, `1b7c46b`, `b7b2fb2`, `492e445`. 810 pass / 0 fail /
6 skip; both GUI builds clean.

---

## 2026-06-04 (ckpt 60) — static tilemap render-walk located + geometry ported; the `0x5a00c0` model corrected

Surveyed `0x5a00c0` for the ckpt-59-named "read the grid + blit the backdrop"
slice and found the identification was wrong: `0x5a00c0` references **none** of
the render-grid region offsets — it is the scripted-scene **overlay player** (a
3-state `GetTickCount` pace machine + a stack sprite draw-list + a `0x124`-stride
caption-text array drawn through font bank `DAT_008a7640`), i.e. the intro
banner / dialogue / caption layer, not the tilemap.

The town backdrop tilemap is rendered by **`FUN_00490f30`** (2002 B), found by
intersecting the ~30 grid-dim (`0x2c1030`) readers with the bank pool
`DAT_008a760c`.  It is called `FUN_00490f30(view, 1)` with the render grid in
ECX from the per-frame draw walk (`0x48c150:108` / `0x499100:185`, both passing
the view object `*(room_state+0x104c)`).  It computes the visible-cell window
from the view/camera object + grid dims (`490f30.c:40-54`), scans it reading grid
index `col*0x80+row` (the **read-side** confirmation of ckpt-58's fixed `0x80`
row pitch) + region A's 4 sub-slots, and emits one draw node per populated
sub-slot via `0x4917b0` — dest `(col*0xc80, row*0xc80)`, a `0x20×0x20`
source-rect at `(dx*0xc80/100, dy*0xc80/100)`, layer key = region A `+0x4`.

New pure, host-tested **`src/map_render.{c,h}`** ports the GEOMETRY of the walk
(decoupled from the engine draw machinery): `map_render_visible_window`,
`map_render_grid_index` (`col*0x80+row`), and `map_render_tile` (one region-A
sub-slot → draw-node geometry, or 0 for an empty slot).  The grid it reads is
exactly what `map_decode` produced, closing the decode→read loop.  8 host tests
(window cap both branches + negative-origin clamp + the ×100 row term +
writer↔reader agreement via `map_grid_emit_tile`) → **796 pass / 0 fail / 6
skip**.  Ledger **188/1490 touched / 183 tested** (+1: `0x490f30`; the deferred
helpers `0x4917b0`/`0x418470`/`0x417c40`/`0x48c6b0`/`0x4182d0` are referenced by
bare VA, not `FUN_`, so the derived ledger doesn't over-count).  Both GUI builds
compile clean (`map_render.c` in the `src` wildcard, not yet called by `main.c`).
Deferred (the rest of the render rock): the sprite resolve, the palette tint, the
draw-node pool enqueue + zdd blit/present, the region-C blend arms, and the
camera/view object construction.  Full writeup:
`docs/findings/in-game-intro.md` "The static tilemap render walk".  NEXT: the
draw-node + zdd present pipeline (or the view/camera object construction) to turn
the ported geometry into actual backdrop pixels, diff vs `runs/tas-ingame-1`.

---

## 2026-06-04 (ckpt 59) — FUN_00587e00 per-tile-id placement dispatch ported + host-tested

Ported the **arms** of `FUN_00587e00` — the per-cell recipes that decide which
`map_grid_*` write-primitive calls to make for each town tile id, on top of the
ckpt-58 render-grid primitives.  Ground truth first: extended
`tools/extract/map_data.py --cells` with a `(tile id, shape) → count` cross-tab,
which proves the opening town (DATA 1022) exercises **exactly 9 tile ids**
(`0x1b58b`/`8c`/`8d`/`8f`, `0x1b5a0`/`a9`/`aa`/`ab`, `0x29ff4`) across a handful
of shapes — and that all nine arms are pure compositions of the already-ported
primitives (none touch an engine global directly).  Every other id in the 18 KB
dispatch (the `0x1bd82` autotile pre-pass, the HUD/border families, the
decoration switches) is dead code for this map.

New pure, host-tested **`src/map_decode.{c,h}`**: `map_decode_cell` runs one
cell's arm (faithful argument-for-argument transcription of the `587e00.c`
`FUN_0058ca80`/`FUN_0058c910` calls + the shared `LAB_0058c3b9` base tile), and
`map_decode` is the loop body — dim header + region-C pre-clear + z-major
dispatch + the per-cell region-E co-id zero (`587e00.c:3175`).  The two
`0x1b58d` blend pointers (`&DAT_005cc4xx`) are preserved as their retail VAs.
10 new host tests assert the exact region bytes each arm deposits → **788 pass /
0 fail / 6 skip**.  Integration smoke test: decoding the real 88×19×3 map (160
populated cells) hits **0 unhandled ids** and runs ASan-clean.  Both GUI builds
compile clean (`map_decode.c` in the `src` wildcard, not yet called by `main.c`).
Ledger holds at **187/1490 touched / 182 tested** (`0x587e00` was already
name-counted as touched+tested via `map_data.c` since ckpt 56; this is the
genuine dispatch behind that line).  Deferred: the prologue (front-header flags +
HUD/border bank selection + `0x1bd82` autotile) and the trailing layer pass
(`0x58c8c0`/`0x58c8d0`/`0x58cb30`).  Full writeup:
`docs/findings/in-game-intro.md` "The per-tile-id placement dispatch".  NEXT: a
slice of `0x5a00c0` to *read* the decoded grid and blit the town backdrop, diff
vs `runs/tas-ingame-1`.

---

## 2026-06-04 (ckpt 58) — runtime render-grid + its 3 write primitives ported + host-tested

Decoded `FUN_00587e00`'s `in_ECX` as the **runtime render grid** — a ≳2.9 MB
flat engine buffer (the *placed* form of the map that `0x5a00c0` blits, distinct
from the parsed `map_data`), addressed with a fixed row pitch `0x80`
(`idx = p1*0x80 + p2`).  Layout: a front header + four parallel per-cell regions
(A @0x30 = 4 sub-slots×0x10, B @0x140030, C @0x195030, D @0x2c1040) + a dim
header @0x2c1030.  Ported the three small pure write helpers the 18 KB dispatch
calls into `src/map_grid.{c,h}`: `FUN_0054c970`→`map_grid_clear_cell` (region C),
`FUN_0058ca80`→`map_grid_emit_obj` (regions B+D), `FUN_0058c910`→
`map_grid_emit_tile` (region A; bank-derived footprint via a `mg_bank_dims_fn`
callback to stay pure), plus `map_grid_set_dims` for the dim header.  6 host
tests (exact-byte assertions) → 778 pass.  Ledger 187/1490 touched / 182 tested
(+3).  Full writeup: `docs/findings/in-game-intro.md` "The runtime render grid".

---

## 2026-06-04 (ckpt 57) — map cell-record semantics decoded; pure cell accessors ported; FUN_00587e00 surveyed (18 KB rock, not 3 KB)

Opened the next unit, **`FUN_00587e00`** (the map-data → world decoder the handoff
named as NEXT), and found the handoff's "3282 B" estimate was wrong: it is
**18055 B** (`587e00.c`, 3283 decompiled lines) — a multi-checkpoint rock, the
same surprise the ckpt-56 survey hit with `0x586010`/`0x5a00c0`.  Followed the
ckpt-53/56 discipline instead: decode the data it consumes as re-runnable ground
truth + port the small pure unit, defer the rock.

**RE — the map-descriptor object + cell-record semantics.**  Read `FUN_00587970`
(the object it builds: dims `+0x20/24/28`, count `+0x2c`, cells `+0x34`, layer
headers `+0x38`, layer sub-pointer table `+0x3c`) and `FUN_00587e00`'s per-cell
decode loop (`587e00.c:586-601`), which reveals the **0x1c-byte cell record**:
`+0x04` = tile id (the dispatch key), `+0x10` = footprint/orientation selector
(0..0xc), `+0x0c`/`+0x14`/`+0x18` = placement params, `+0x00` = a co-id, `+0x08`
= aux; **empty cell = all-zero**.  Linearization is z-major:
`idx = (dim1*z + y)*dim0 + x`.

**Ground truth — DATA 1022 decoded.**  Extended `tools/extract/map_data.py` with
`--cells`: renders the per-z occupancy grid + tile-id/shape/field histograms.
The opening town = **160 of 5016 cells populated**, a coherent backdrop (z=2 near
plane = the bottom ground strips, z=0 far plane = rooftop runs, z=1 a few mid
details); tile ids cluster in the **`0x1b58b` family** + `0x29ff4`; shapes used
`{0,2,10,11,12,14,15}`.  **All town tile ids are `< 0x1bd82`**, so the giant
`0x1bd82` autotile block + the `0x1d8ab`/`0x1ffbc…`/decoration switches in
`FUN_00587e00` are *dead code for this map* — a real scoping win for the eventual
port (only the generic per-tile-id arms + the emit helpers `0058ca80`/`0058c910`
matter for the town).

**Port (pure, host-tested).**  Added `map_cell` + `map_data_cell(m,x,y,z)` /
`map_data_cell_index` to `src/map_data.{c,h}` — the typed accessor the future
`FUN_00587e00` port + render walk index into.  2 new host tests → **772 pass / 0
fail / 6 skip**.  Ledger **184/1490 unchanged** (no new FUN ported — `587e00`
stays unported; this is data decode + accessors on the already-tested
`0x587970`).  Full writeup: `docs/findings/in-game-intro.md` "The cell record" +
"DATA 1022 decoded" + "`FUN_00587e00` is an 18 KB multi-checkpoint rock".

**NEXT:** port `FUN_00587e00`'s per-tile-id placement arms for the ~9 town tile
ids + the emit helpers (`0058ca80`/`0058c910`) writing the runtime grid, then the
matching slice of `0x5a00c0` to render the backdrop, diff vs `runs/tas-ingame-1`.

---

## 2026-06-04 (ckpt 56) — the runtime MAP-DATA load path + format are resolved; FUN_00587970 is ported + host-tested

Surveying the next unit (`0x586010` sim → `0x5a00c0` render) confirmed both are
multi-checkpoint rocks: `0x586010` (6 KB) is the full engine sim (allocs the
`0x27b8` room-state object `DAT_008a9b50`, creates party actors, loads the map,
runs the event system `0x40b8f0` + per-frame step `0x58f360`); `0x5a00c0`
(13.7 KB) is a self-contained **blocking scripted-scene player** with its own
3-state GetTickCount pace machine, a sprite-descriptor array, a caption/text
line array (0x124 stride), and a full resource-unload teardown.  The tractable
next data-layer unit is the **map-data load** the sim performs.

**Key RE finding — the town backdrop is a PE DATA resource in the EXE, keyed by
scene index.**  `0x586010:675-697` resolves the map via
`FUN_00587970(DAT_008a6e7c, room.scene)`, where `DAT_008a6e7c` is the **main EXE
module handle** (a boot handle slot `0x8a6e68..7c`, *not* sotesd) and `room[3]`
is the **scene index**.  `FUN_00587970` opens with **`FUN_005b62a0`** =
`FindResourceA(module, scene&0xffff, "DATA")` + LoadResource/LockResource, then
copies sequentially with **`FUN_005b6340`** mode 1.  Room 210110's scene = 1022,
so the opening town map = **`FindResourceA(EXE, 1022, "DATA")` = DATA resource
1022 in the EXE** (152,936 B, name **"MSD_SOTES_MAPDATA"**, dims **88×19×3**, 86
layers).

**Refines plan 3a (ckpt 51 was incomplete).**  The ckpt-51 res-probe hooked only
the *sprite* decoder `bs_decode_resource` (`0x5b7800`); `FUN_005b62a0` is a
separate FindResource path it never saw.  "Map layout is compiled-in" holds only
for the ROOM REGISTRY (the `.rdata` room graph / names / scene ids); the per-room
*visual* map (tiles + object layers) **is** a loaded resource, from the EXE,
keyed by scene index.  Pairs with the EXE-NULL banks `0x570-0x572`: the port
loads both from the original `sotes.exe` as a datafile (one `g_sotes_exe`).

**Port (pure, host-tested).**  New **`tools/extract/map_data.py`** decodes any
map DATA resource and asserts it consumes the resource exactly (re-runnable
ground truth, the ckpt-53 pattern).  New **`src/map_data.{c,h}`** ports
`FUN_00587970`'s parse: the caller supplies the locked bytes (FindResource stays
Win32 in `main.c`), and `map_data_parse` decodes magic + 0x30 header + 0x34
maphdr (name/dims/count) + the `dim0*dim1*dim2`×0x1c cell array + `count` layer
entries (each a 0x3c header + four sized sub-arrays) into owned allocations, with
an overrun guard vs `len`.  The `0x1c` cell record + the layer sub-array element
layouts are decoded by the unported `FUN_00587e00` (3282 B, the next unit); this
parser preserves their raw bytes.

4 new host tests (`tests/test_map_data.c`, synthetic blobs) → **770 pass / 0 fail
/ 6 skip** (+4).  Ledger **184/1490** (`0x587970` now *tested*).  Both port
builds (mingw GUI + debug) compile clean; `map_data.c` is in the `src` wildcard
but not yet wired into `main.c`.  Full writeup:
`docs/findings/in-game-intro.md` "The RUNTIME MAP DATA".  **Next:** port
`FUN_00587e00` (the map-data → world decode reading this structure) + the
matching `0x5a00c0` render slice, diff vs `runs/tas-ingame-1`.

---

## 2026-06-03 (ckpt 55) — the in-game MAP OBJECT is ported + host-tested (0x59f2c0 fresh-entry arm + 0x4c5350 room-key resolution)

Ported the **runtime map object** the in-game engine builds on a fresh new-game
entry into new pure, host-tested **`src/game_map.{c,h}`**: the
`in_stack_0000eb2c == 0` arm of **`FUN_0059f2c0`** (lines 160-218) plus the
**`FUN_004c5350`** room-key resolution it calls.  It allocates the
`operator_new(0x4120)` map object (as a zero-initialised `buf[0x4120]`) + the
**8× `operator_new(0xeec)`** actor sub-objects, runs the 8-actor init loop (slot
index at `+0xa0c`, **`FUN_00560e60`** field-zero, per-slot active flag
`map+0x4084+4*i = 1`), writes the header fields (`+0x40a4=1`, `+0x4018=1`,
**`+0x4054=3`**, the `+0x405c..+0x4064` u16 run, the 3 `{1,0}` tail pairs at
`+0x4108` that fill the object exactly to `+0x4120`, the `GetTickCount` stamp at
`+0x4068`), sets `map+0x4104 = 0x3f2`, then `FUN_004c5350`'s `map==0x3f2` arm
writes the **room-lookup key `map+0x4024 = 0x334be`** (+ spawn params
`+0x4028=0x65`/`+0x402c=1`).  `game_map_active_room` then resolves that key via
`game_world_find_room` → **room 210110 "Town of Tonkiness"** (area 0xd2, scene
1022) — the end-to-end opening-room build, on top of the ckpt-54 table layer.

**Fidelity boundaries (documented in the header).**  The map buffer is
zero-initialised (retail's `operator_new` is raw + relies on the explicit writes
+ opaque sub-inits `0x5612b0`/`0x5611d0`/`0x4e59a0`); `map+0x4020` (the `+0x4014`
ramp ceiling) is set by one of those, so the ramp is inert under zero-init and
does not affect the verified room key.  `FUN_004c5350`'s sprite-registry
sub-calls (`0x408dc0`/`0x413b20`/…) and its `0x3fc` arm (gated by unported
save-flag state) are skipped; the `map==1` arm's pure writes are kept.  An
ordering note is preserved: the `map==0 → 0x3f2` default at `0x59f2c0:378-381`
runs *after* `FUN_004c5350`, so a `0`-map fresh entry gets no key from the
resolver — the real path always passes `0x3f2` (`0x59ec30(0,0,0x3f2)`).

6 new host tests (`tests/test_game_map.c`) → **766 pass / 0 fail / 6 skip** (+6).
Ledger **180/1490** (`0x4c5350`, `0x560e60` now *tested*; `0x59f2c0` was already
touched via the ckpt-54 `game_world` header).  Both port builds (mingw GUI +
debug) compile clean — `game_map.c` is pulled into the `src` wildcard but
**not yet called by `main.c`** (it is the world-runtime foundation the unported
sim/render units read, same status as `game_world`).  Full writeup:
`docs/findings/in-game-intro.md` "The MAP OBJECT".

**Next:** a slice of `0x586010` (sim) → `0x5a00c0` (render) that walks this map
object + room 210110 to draw the static town backdrop, diffed vs
`runs/tas-ingame-1` anchored on `game_enter`.

---

## 2026-06-03 (ckpt 54) — the world-table layer is PORTED + host-tested; map 0x3f2 → room 210110 ("Town of Tonkiness") resolved

Ported the data half of the in-game engine's fresh-entry world construction into
new pure, host-tested **`src/game_world.{c,h}`**: the AREA/ROOM registry build
(the zero-terminated copy `0x59f2c0:122-144`), **`FUN_00585000`** (the per-room
cross-reference — part 1 fills area defaults `room[0x43/0x44/0x45/0x50/0x51]`
from the matching AREA entry; part 2 builds the **reciprocal room-transition
exits** by scanning every other room's 0x14 exit slots), and **`FUN_00561c90`**
(the room lookup — linear search of the copied region by `ROOM.id`/dword0).  The
tables ship as generated bytes (`src/world_tables_data.{c,h}`, emitted by a new
`game_world_tables.py --emit-c` from the EXE's `.rdata`).  6 new host tests
(`tests/test_game_world.c`) → **760 pass / 0 fail / 6 skip**.  Ledger
**178/1490** (`0x585000`, `0x561c90` now *tested*).

**KEY RE FINDING — the `0x3f2` → opening-room resolution (corrects ckpt 53).**
The map id `0x3f2` does NOT index a room.  Disassembly of the room loop
(`0x59f2c0` `59fd8b: mov edi,[ebx+0x4024]; push edi; call 0x561c90`) shows the
lookup key is map-object **`+0x4024`**, set by **`FUN_004c5350`** (a jump table on
`*(map+0x4104)`: `map==0x3f2 → 0x4c5516` writes `+0x4024 = 0x334be`,
`+0x4028 = 0x65`, `+0x402c = 1`).  **`0x334be` = decimal 210110** = registry
entry **[61]**: id 210110, area key `0xd2` = **"Town of Tonkiness"**, scene
**1022**.  So the opening map renders room **210110 "Town of Tonkiness"** — NOT
room 110110 "Town of Tolkien" (a *different, later* town; the ckpt-53 "Tolkien"
and earlier "Tilelia" identifications were both wrong).  `+0x4028`/`+0x402c` are
the entry spawn params; seven rooms carry exits targeting 210110 (a town hub).
Host-tested (`game_world_map_3f2_opening_room`).  (Caveat: the *resolution* to
room 210110 is proven from the binary; that this is what renders at golden flip
~1150 — vs an immediate scripted transition — re-confirms when `0x5a00c0` lands.)
Full writeup: `docs/findings/in-game-intro.md` "ROOM lookup".

**Next:** build the map object (`0x59f2c0` fresh-entry arm: the 0x4120 alloc +
field init + the 8 actor slots) on top of this table layer, then a slice of
`0x586010` (sim) + `0x5a00c0` (render) to draw room 210110's static backdrop,
diff vs `runs/tas-ingame-1` anchored on `game_enter`.

---

## 2026-06-03 (ckpt 53) — the static world tables are extracted + decoded (plan 3b groundwork)

The in-game engine's map layout is **compiled-in static data** (confirming plan
3a) — two `.rdata` tables that `0x59f2c0:122-144` copies into the world's
`scene[4]` object on entry: `scene[4][0] = &DAT_00693848` (the AREA name table)
and a copy of every `&DAT_006940c8` entry (the ROOM registry) into
`scene[4][1..]`, cross-referenced by `0x585000`.  Decoded both out of
`vendor/unpacked/sotes.unpacked.exe` and documented the field meanings.

**New committed tool** `tools/extract/game_world_tables.py` (PE VA→file-offset
map; full listing; `--raw N` hex-dumps a room).  Findings:
- **AREA table `&DAT_00693848`** — 0x40-byte stride, zero-terminated, **33
  entries** = Fortune Summoners' areas (`0x6e` "Town of Tolkien", `0x82` Silver
  Dungeon, `0xe6` Minasa-Ratis Magic School, … `0x1c2` Labyrith of Night).  Each
  holds 6 small dwords `0x585000` fans into the room as per-room defaults.
- **ROOM registry `&DAT_006940c8`** — 0x150-byte (0x54-dword) stride, ends at the
  first `dword0==0`; entry `[0]` is a header sentinel (`0xf423f`) → **416 real
  rooms**.  Per-entry fields recovered: `id` (packed, e.g. 110110), `area`→AREA
  name, `scene` (sequential index 1002/1004/…), `parent` (a room id — the
  transition graph), `d9` ordinal, and a **Shift-JIS room name @+0x118** (room
  110110 = "トルーキンの町 １丁目", Town of Tolkien district 1).

**Doc fix:** the opening town is **"Town of Tolkien"** (SJIS トルーキン), not the
earlier unverified "Tilelia" guess.  **Open thread:** the engine's entry map
`0x3f2` (=1010) is NOT any room `scene` value (they jump 1009→1012), so `0x3f2`
is a separate scene-load id the room loop `0x561c90` resolves to room 110110 —
pinning that mapping is the first room-loop-port task.

Pure groundwork (verifiable data extraction + findings doc, no runtime C ported):
753 host tests pass unchanged, ledger 175/1490 unchanged.  Full writeup:
`docs/findings/in-game-intro.md` "The static world tables (plan 3b groundwork,
ckpt 53)".  **Next:** port the `0x59f2c0` fresh-entry arm (world construction)
into a `game_world` model reading these tables, then the `0x3f2`→room resolution,
then a slice of `0x5a00c0` for the static town backdrop.

---

## 2026-06-03 (ckpt 52) — the in-game `game_drive` scaffold is stood up (plan 3b's first step)

`enter_game` no longer re-displays the title — it stands up a **`game_drive`**
(new `src/game_drive.{c,h}`, the milestone-2 counterpart of `prologue_drive`):
it owns the in-game input ring, and `main_loop_body` runs one `game_drive_step`
per presented frame.  The in-game engine (0x59f2c0 world setup + 0x586010 sim +
0x5a00c0 render dispatch) is unported, so a step renders the faithful **black
map-load frame** (`game_render` = `zdd_object_clear`) — the state retail shows
from `game_enter` (flip ~1092) until the town first renders (~flip 1150) while
the engine loads map 0x3f2 + the entry fade runs (golden: flips 900-1100 black).
This replaces the prior stub's wrong title re-display.  `game_status::GAME_EXIT`
is reserved for the engine's scene-transition codes (0x59f2c0 ret 4/5 → 0x59ec30
map reload); a step stays `GAME_RUNNING` for now.

**Verified live** (`trace-port.jsonl`, `--frames 1300`): `game_enter@1116`
rng `0x40d00581` (matches retail `game_enter@1092`); the port runs the game_drive
to frame 1300 without re-displaying the title.  Captures: frame 400 (title,
phase 6) = 307200 nonblack px (title unaffected); frames 1160/1200 (in-game,
`phase=-1`) = fully black (extrema 0) → the early in-game frames now match
retail's black entry window.  3 host tests (`test_game_drive.c`) → 753 pass / 0
fail / 6 skip.  Ledger 175/1490 unchanged (scaffold/seam, no new FUN; unported
engine fns kept as bare VAs).  Commit 7c60b25.

Also recorded (`docs/findings/in-game-intro.md`): the EXE-NULL banks
`0x570-0x572` are confirmed present in `sotes.unpacked.exe`'s `.rsrc`
(`type=DATA`, 387 DATA ids); the port must load them via
`LoadLibraryExA("sotes.exe", LOAD_LIBRARY_AS_DATAFILE)` (not `settings=NULL` —
the port is its own exe), and their registration stays coupled to the
engine-time pool slot indices (deferred with the 0x5a00c0 port).  **Next:** port
the world construction (0x59f2c0 fresh-entry arm) + a slice of 0x5a00c0 for the
static town backdrop, diff vs `runs/tas-ingame-1`.

---

## 2026-06-03 (ckpt 51) — plan 3a RESOLVED: in-game town banks identified (new --res-probe) + the deferred boot batches g2/g3/g5 wired (title still differ_px=0)

Answered milestone-2 plan 3a ("which banks does map 0x3f2 pull, and from where")
with a new ground-truth probe.  **`frida_capture.py --res-probe`** hooks the
generic PE-resource decoder `bs_decode_resource` (`FUN_005b7800`) and logs every
distinct `(module, id, type)` load with its first flip (agent `installResProbe`,
dedup by module|id|type → `res_loads.jsonl`).  Drove retail prologue → Z-spam →
in-game town under `--lockstep` (`tests/scenarios/in-game-intro/trace-retail.jsonl`)
and analysed the loads at `flip >= game_enter@1092`.

**Finding:** the opening town loads **no per-map resource file** — its only loads
are `type="DATA"` sprite-sheet decodes via the SAME path the title uses
(`ar_sprite_decode` 0x4184a0 + the slot palette-load 0x4178e0); `0x586010`/
`0x5a00c0` never `FindResource` a map/tile file.  **Map layout is compiled-in
static data** (the `&DAT_006940c8` registry, "StartArea" tables).  `sotesw.dll`
= WMA music, `sotesp.dll` = 1 ramp — neither holds graphics (the map-id 0x3f2 ↔
sotesw WMA-id collision is a BGM red herring).  The town + intro dialogue decode
**74 distinct sprite banks**: 71 from `sotesd.dll` + 3 EXE-embedded (hModule=NULL).
Cross-referenced to the register tables, they are exactly the deferred boot
batches **g2** (`ar_register_palette_ramps`: ramps + dialogue face portraits) +
**g3** (`ar_register_group3_sprites`: bulk town) + **g5**
(`ar_register_game_sprites`: character sprites) that `init_sprite_banks` had
skipped (g4 was already wired).

**Port change:** `init_sprite_banks` (`src/main.c`) now also registers g2/g3/g5
(all `settings=g_sotesd`), matching retail's `ar_boot_register_all`.  Banks
decode lazily → inert until a `game_drive` renders in-game.  **Verified no
regression:** A/B capture (post-vs-pre binary, port title flips 60/200) →
**differ_px=0** both frames; port boots clean.  750 host tests pass (main.c not
host-linked); ledger unchanged (tooling + boot wiring, no new FUN).  **One
residual → plan 3b:** the EXE-NULL banks `0x570-0x572` (present only in
sotes.exe's own .rsrc; loaded with `hModule=NULL`) are in no ported batch —
register them with `settings=NULL` at engine time.  Full writeup:
`docs/findings/in-game-intro.md` "Resource banks (plan 3a)".

---

## 2026-06-03 (ckpt 50) — in-game seam wired: game_enter anchor (both sides) + PROLOGUE_DONE→enter_game; the engine 0x59f2c0 surveyed/decomposed

Foundational plumbing for milestone 2 (the game proper).  The prologue's NORMAL
exit (3rd beat → `PROLOGUE_DONE`) now routes to a new **`enter_game`** seam in
`src/main.c` instead of bouncing to the title: it emits the **`game_enter`** TAS
anchor (`emit_anchor`) + logs the `0x59ec30(0,0,0x3f2)` entry, then (engine
unported) re-displays the title like the other stubbed sub-scenes.  The
retail-side `SCENE_ANCHORS` in `tools/frida/opensummoners-agent.js` gains
`{ va: 0x59f2c0, name: 'game_enter' }` — the per-map run-loop entry the wrapper
`0x59ec30` calls.

**Verified both sides (live).**  Port: `--input-trace` (new committed
`tests/scenarios/in-game-intro/trace-port.jsonl`) → `newgame_enter@691` →
`prologue_enter@826` → **`game_enter@1116` rng=0x40d00581**.  Retail (Frida,
`--lockstep`, `trace-retail.jsonl`) → **`game_enter@1092` rng=0x40d00581**.  The
RNG stamp **matches exactly** on both sides at the seam → no RNG desync across
prologue→in-game; the anchor absorbs the +24-flip offset for `tas_diff`.

**Engine surveyed** (`docs/findings/in-game-intro.md`): `0x59f2c0` (3522 B) =
world alloc (map object 0x4120 w/ map-id field +0x4104 defaulting to 0x3f2; two
big world buffers 0x5400c/0x7808; the `&DAT_006940c8` 0x54-stride actor/cell
registry) + the per-room loop calling the two giant children — **`0x586010`
(6 KB, the room state-setup + sim/draw step)** and **`0x5a00c0` (13.7 KB, the
in-game RENDER dispatch)**.  `0x5a00c0` reuses the already-ported sprite
primitives (`ar_sprite_decode`/zdd blits/ramps), so the smallest visible win (the
town tilemap) sits on top of existing code — that is the next checkpoint.  No
`src/` host-tested logic changed (750 pass / 0 fail / 6 skip).  Ledger unchanged
(seam + anchor + survey; no new FUN ported yet).

---

## 2026-06-03 (ckpt 47) — the TAS deterministic port↔retail trace-diff system is built and validated; intro 28/28 bit-exact and prologue cutscene content bit-exact through it

Built the determinism stack for frame-for-frame port↔retail diffing.  **Key RE
finding:** retail drives its entire update cadence off **`GetTickCount` deltas**
(no `timeGetTime`/QPC) — every scene loop runs the same 3-state pace machine
spending a time budget in 16 ms update slices.  The old turbo clock advanced the
virtual clock per-CALL, banking several updates per Flip → retail rendered only
~1/N of the update stream, un-diffable vs the port's 1-update/present.

New **`--lockstep`** (agent) freezes the virtual `GetTickCount` between Flips and
banks exactly one update quantum per present (a stall-breaker creep defeats
asset-load busy-waits without polluting the budget) → retail renders 1
update/present like the port.  Verified: subtitle anchor moves flip 73→432, two
runs byte-identical, all consecutive frames distinct (exactly 1:1).  Also proved
retail is already fully deterministic run-to-run under turbo+seed-pin (byte-
identical frames through the dynamic intro + auto-demo across runs with different
wall-clock seeds) — lockstep is for cadence, not determinism.

Bilateral RNG-stamped **anchors** (`subtitle_anim_start`/`newgame_enter`/
`prologue_enter`) align the per-binary flip skew; **`tools/tas_diff.py`** diffs
per-tick `differ_px` with a best-match ±W window (absorbs an occasional port
0-update duplicate present).  **Intro: 28/28 phase-7 frames `differ_px=0`.
Prologue cutscene: 63/64 dense gem-rise frames have a bit-exact retail match →
gem/aura/narration render is frame-for-frame bit-exact** (ckpt-46 eyeball verdict
now quantified).  Two real port gaps surfaced by the anchors (open threads, not
render bugs): the port skips the new-game→prologue transition (`0x564160`/
`0x5642e0`, ~20 retail flips) which also consumes `rand()` → RNG desync at
prologue_enter (port `0x404a0a8f` vs retail `0x40d00581`).  Full writeup:
**`docs/findings/tas-harness.md`**.  Commits `b21c260` (lockstep), `7fc159c`
(anchors + tas_diff).  749 host tests pass (tooling + main.c anchor logging).

---

## 2026-06-03 (ckpt 46) — the ELEMENTAL-STONE PROLOGUE CUTSCENE (`FUN_0056cd20`) is ported, wired into the Start path, and renders live (gem + aura + scrolling narration); user-confirmed visually

Confirming "Start Game" in the new-game menu now runs the gem cutscene — the
**prologue critical path**.  The boot driver (`FUN_00562ea0` case 0x1a) runs
three blocking calls: the config menu (`0x564160`, ported as `newgame_*`), the
**stone intro (`0x56cd20`)**, then the game proper (`0x59ec30`, deferred/in-game).
Surveyed in **`docs/findings/prologue-stone-intro.md`**.

New pure, host-tested **`src/prologue_stone.{c,h}`** (the visual half of
`0x56cd20`): the per-tick UPDATE state machine (start delay, watchdog, gem
fade-in/hold/fade-out, gem-frame `%0x23` + aura toggle every 7 ticks, the rise
curve `local_a0 += local_88/100` with ease-out past 16000, the 6 caption-line
state machines, abort/beat input) + the render-descriptor build.  New
**`src/prologue_drive.{c,h}`** is the Win32-free caller (steps one tick/frame,
renders + presents; simpler than newgame_drive — no input gate, the cutscene
reads the raw ring).  `main.c` `prologue_render` clears to black + blits gem
(slot[3]/0x4a2 via ramp_b) → aura (slot[1]/0x49f via ramp_a) → 24 caption tiles
(slot[2]/0x448 via ramp_b); the new-game START commit calls `enter_prologue`.

**Key head-start:** the gem/aura/caption banks were **already registered at boot**
(`ar_register_main_sprites` group 4), and the alpha blit (`zdd_alpha_blit` =
`0x5bd550`) + ramps (`g_ramp_a`/`g_ramp_b`) were already ported — so the cutscene
needed only the state model + drive + wiring.  The aura's blend ramp (ramp_a, idx
`local_bc/30`) was recovered from the **disasm** (`0x56d38d`); the decompiler had
dropped `FUN_005bd550`'s `__thiscall` ECX = the ramp entry.

**Live finding:** the scrolling **prologue NARRATION is part of `0x56cd20`**, not
the game proper — it is **pre-baked sprite tiles** (bank `0x448` = slot[2], a
24-tile strip = 6 lines × 4 horizontal tiles), the grid the survey first
mislabeled "sparkles" (renamed → `caption` throughout).  `0x56cd20` uses no GDI
text.  **User-confirmed visually** ("that cutscene looks good… on first
inspection it looks right"); montage pushed to llm-feed.

19 host tests (**749 pass / 0 fail / 6 skip**).  Ledger **174/1490 (+1:
`0x56cd20`)**.  Commits `df6fddf` (state model) + `3b20fd6` (drive + wiring) +
`04bbd29` (caption rename + survey correction).  **OPEN gate:** no bit-exact
retail diff yet — capturing a retail golden of the stone intro + `differ_px` is
the next move (mind the possible modal-loop Flip-freeze, picker class).

## 2026-06-03 (ckpt 44) — new-game TOOLTIP TEXT NODE rendered bit-exact: word-wrap port (`FUN_0040e5e0`), 0 text-colored pixels differ

The bottom-of-screen help line is a **standalone word-wrapping text node**
(`this+0x170`), not the menu grid — one free-form string greedily wrapped into
rows.  Ported the layout core **`FUN_0040e5e0`** (justify → per-glyph (col,row))
plus the **`%n`/`%m`/`%w` parse `FUN_0040f040`** as the pure, host-tested
**`src/glyph_wrap.{c,h}`** (quirk **#70**).  A word = alpha `[A-Za-z']+` / digit
`[0-9.,]+` (+ one absorbed trailing `{space ! , - . ; ?}`) / a lone glyph; the
row-width accumulator + wrap test mirror retail's `uVar13`/`param_1` (width = the
`FUN_0040dee0` ctor arg `0x44` = **68 glyph-columns**); `%n` forces a break.  The
SJIS kinsoku path (`sVar3==3`, the `DAT_008548xx` table) is deferred — English
never reaches it.

`newgame_render` (`main.c`) now picks the focused row's help string
(`newgame_scene_tooltip`), wraps it, and draws each row at **(72, 416+r·28)** with
the menu's 2-copy drop shadow (`0xa8b9cc`) + text (`0x3e537d`); the monospace 7px
Courier New means one `TextOutA` per row is pixel-identical to retail's per-glyph
stream.  **Verified LIVE** (port Flip 760 vs `goldens/retail-newgame-config-menu.png`):
the difficulty-row tooltip wraps **65 / 52** glyphs across y=416/444 — the break is
the width-68 word-wrap (the source has no `%n`), reproduced exactly; the tooltip
region has **0 text-colored pixels differing** (a text-presence XOR over the
region is 0).  6 new host tests (`tests/test_glyph_wrap.c`, **720 pass / 0 fail /
6 skip**).  Ledger **168/1490** (+5 touched: `0x40e5e0`,`0x40f040`,`0x4031c0`).
Comparison pushed to llm-feed.

**Open (pre-existing, not the text):** the only residual in the tooltip region is
**9 background pixels off by exactly one RGB565 5-bit step** (R or B by 8, green
exact) — a box-panel sprite-decode rounding (`newgame_box`, bank `0x457`), the same
class as the menu-box delta-8 px (there masked by the deferred sparkle corner).
Hypothesis: an 8→5-bit decode round-vs-truncate in `bs_convert_to_16bpp`.  A
separate sprite-decode investigation; the box was user-accepted at ckpt 40.

---

## 2026-06-03 (ckpt 43) — selection-cursor RENDER BUG FIXED: a transposed trim scan, not a videomem path; new-game menu cursor is now bit-exact (`differ_px=0`)

The ckpt-42 "scale_flag=1 videomem cell-build path" diagnosis was **wrong**.  The
real cause was a **transposed per-cell trim scan** (quirk **#69**):
`bs_trim_opaque_rect` (`FUN_005b6f80`) named its two size params `(height, width)`,
but retail's arg4 is `cell_w` (the inner/column loop + x-range) and arg5 is
`cell_h` (the outer/row loop + y-range); the slicer passes `(cell_w, cell_h)`, so
a **non-square** cell was scanned transposed.  Invisible on the square 32×32 box
bank `0x457` (which is why every square-cell sprite rendered bit-exact), it
scrambled the 32×48 cursor bank `0x455` into a wrong-size, wrong-offset, un-keyed
cell — the live "opaque-black 16×24 rect at x72" the previous session saw.

Found it the cheap way: a new pure host probe **`tools/extract/cursor_trim_probe.c`**
feeds the real `0x455` blob through the port's actual `bs_trim_opaque_rect` and
dumped frame 17 = **44×29** (wrong) → after swapping the param names to
`(width, height)` (body unchanged) → **22×41 @ (4,3)**, matching the live
`--box-probe` exactly.  Fix touches `src/bitmap_session.{c,h}` (param rename) +
`src/asset_register.c` (call-site comment) + the trim tests (square cases
unchanged; the 6×10 `other_depth` test corrected; new regression
`test_trim_8bpp_nonsquare_quirk69` with a 4×8 cell).  **714 pass / 0 fail / 6 skip**.

Flipped `g_newgame_cursor_enable` **ON** and verified LIVE: port Flip 761 vs
`goldens/retail-newgame-config-menu.png` → menu-box **`differ_px=0`** (region
(32,32)400×124 — panel + text + cursor all bit-exact).  Off-phase frames (760/762
= cursor anim frames 16/18) differ only by the breathing cursor's animation phase
(frames 17 and 19 are both 22×41 @ (4,3), the symmetric phase the golden froze) —
the same animation-phase caveat as the intro twinkles, not a content gap.  Closes
the ckpt-40 307px menu-box residual.  Parity-ledger **#5**.  Comparison pushed to
llm-feed.  Ledger 163/1490 (bug-fix, no new FUN).

## 2026-06-03 (ckpt 41) — selection-cursor GEOMETRY ported + validated; sprite BANK unidentified (render gated off, dig harder next session)

Ported `FUN_0048d940`'s type-1 arm — the new-game menu **selection cursor** (the
drooping gold vine over the box top-left, toward the focused row) — into new pure,
host-tested **`src/newgame_cursor.{c,h}`**.  Frame = base(16) + frames{0,1,2,3} →
16-19 (from the reliable `FUN_00411ec0` decomp args).  Blit base x = box_x + 8,
y = box_y - 6 + (cursor-sel2)*pitch(28), from `0x48d940`'s type-1 formula with
`node+0x7c=-32`, `node+0x80=-30` (read from `FUN_00411ec0`).  **Position
VALIDATED**: row-0 base (40,26) matches the live `--box-probe` golden AND
independently derives from the bit-exact text geometry (col0 x=72-32=40, row0
y=56-30=26).  4 new host tests (**713 pass / 0 fail / 6 skip**).  Wired the
adapter + render in `main.c` behind `g_newgame_cursor_enable` (default **OFF**).

**The blocker: the cursor's sprite BANK is unidentified.**  Probing showed the
`--box-probe`'s deref chain (`bank=*(node+0x28); slot=*bank`) reads **garbage** at
`slot+0x20`/`+0x38` for this node type — its `res_id=0x3e8`/`22×41` readouts are
NOT trustworthy (only the position, from node fields, is).  Ruled out by live
experiment: the port's **0x3e8** (slot 65, sotesd) = a 640×352 **background
landscape** (8 frames, dumped+viewed); **0x3e8 absent** from sotesp/sotesw; a
full **24-frame sweep of the sibling box atlas 0x455** (slot 43) at (40,26)
**matches nothing** (its frames are 44×30 feathers/◄►arrows/caduceus/books — but
the golden element is a thin **22×41 drooping stem+bud+soft-shadow**).  Real bank
= `*(god+0xb8c)`, store site not in the decomp grep (only reads).  Per the user
("dig harder next session"), the render stays gated off; next-session leads:
(1) find the `god+0xb8c` store (box-widget god-object sprite-bank init, near the
`+0xb88`=0x457 store; ctors `0x410560`/`0x4103d0`/`0x413760`); (2) a Frida probe
that Locks+dumps the actual blitted frame-surface PIXELS from retail's `0x48d940`.
Comparison pushed to llm-feed (user confirmed the menu render looks good).

---

## 2026-06-03 (ckpt 40) — the new-game config BOX PANEL renders (9-slice chrome); menu box bit-exact bar the deferred sparkle corner

Drew the bordered cream panel behind the new-game menu + tooltip.  **Part 1**:
a new live **`frida_capture.py --box-probe`** (hooks the sprite-cell render
`0x48d940` + the 9-slice renderers `0x48cb90`/`0x48cf80`, gated to the menu flip
window, deduped by node) captured the exact box composition (golden
`goldens/retail-newgame-box-cells.jsonl`, quirk **#67**): the panel is a 9-slice
SPRITE box (`0x48cf80`), bank **PE resource 0x457** (already registered by
`ar_register_fonts` as `AR_SPR_FONT_TEX_457`, 32×32 cells), frames
tl0/top1/tr2/l3/c4/r5/bl6/b7/br8 (center 4 = cream RGB(239,227,214)); two
instances — menu box (32,32)400×124 + tooltip box (32,392)576×80.  A separate
animated sparkle corner (`0x48d940`, bank **0x3e8**, frames 16–19) sits at the
top-left.  Neither bank field (`*(this+0xb88)`/`+0xb8c`) is a literal offset in
the corpus — they're set by an embedded sub-object ctor — so the resource ids
are only knowable live (read off `slot+0x40`).  Harness note: entering the scene
the Flip counter freezes (modal pump `0x565d10`), so flip-gated probes see only
the title→menu transition (flips ~410–422) — which is when the box first renders.

**Part 2**: ported `0x48cf80`'s opaque arm as the pure, host-tested
`src/newgame_box.{c,h}` (the 9-slice tiling walk over a `newgame_box_ops` vtable:
corner→tiled edge→remainder→corner per row; top/full-middle/partial-middle/bottom
rows).  The real blit (`ar_sprite_slot_frame` + `zdd_object_blt_clipped` =
`FUN_005b9bf0`, the keyed clipped blit) is wired in `main.c`; `newgame_render`
now clears the primary → draws both box panels via DDraw → GetDC +
`glyph_grid_render` the menu text on top (replacing the placeholder
`PatBlt(BLACKNESS)`).  **Verified live** (port frame 760): menu box
**differ_px=307/50800 (0.6%)** vs the golden — and all 307 residual pixels are in
the top-left corner (x44–65,y29–69), exactly the deferred sparkle overlay; the
9-slice panel + menu text are bit-exact everywhere else (interior cream
RGB(239,227,214) matches exactly).  Comparison pushed to llm-feed.  4 new host
tests (coverage-grid: slices tile each box exactly once, no gap/overlap/OOB) →
709 pass / 0 fail / 6 skip.  Ledger **161/1490 (9.9%)** (+4: `0x48cf80`,
`0x48d670`, `0x48d3d0` ported; the keyed blit was already in zdd.c).  Commits
`b68c7e2` (probe + ground truth) + `98d78f4` (box render).

Deferred: the sparkle corner (`0x3e8` — bank not yet registered, the 307px
residual), the tooltip TEXT node (y=416/444 word-wrapped — box drawn, text
computed, needs the word-wrap split), the option picker (`0x567ba0`), the box
fade-in (`0x48cf80`'s alpha arm), and the Start→game path.

---

## 2026-06-02 (ckpt 38) — new-game config run-loop MODEL ported (the Win32-free heart of `FUN_00564780` case 0x24); the `0x27` input semantics RE-corrected

Ported the run-loop heart of the new-game config scene into new
`src/newgame_scene.{c,h}`, mirroring the `title_scene` (pure) vs `title_drive`
(Win32) split.  The pure state machine — focused-row **tooltip resolution**, the
**pump-result→action dispatch**, and the **value-refill** — host-tests; the
real per-frame pump (`0x565d10` + the `0x43bca0` input scan), the option picker
submenu (`0x567ba0`), and the box widgets stay in the drive (the next unit).

The pump→action contract (564780.c:597-669): `0x565d10` collapses every frame
into `0xd` (cursor moved → re-render), `0xc` (confirm/OK → act on focused row:
option → open picker, Start Game → begin), or `0xb` (back → return to title).
`newgame_scene_dispatch` is exactly this switch; `newgame_scene_tooltip` is the
per-frame tooltip selection (`newgame_option_tooltip` = `FUN_00566850` for
option rows + the kind-3 action-tooltip switch); `newgame_scene_set_option` is
the picker's value-refill (`FUN_00566a80` → `glyph_cell_layout`).

**RE correction (quirk #65):** new-game-flow.md's earlier "id 0x27 = value
left/right" guess is **wrong**.  The chain `0x565d10`→`0x43bca0`→`menu_list_latch`
nets out to **`0x24` = confirm (`0xc`)** and **`0x27` = back (`0xb`)** — there is
**no in-place value toggle**; an option's value changes only by confirming into
its picker submenu.  Only the physical-key identity of `0x24`/`0x27` is left for
a live Frida confirm.  698 host tests (+4); ledger **155/1490** (+1: `0x566850`).

## 2026-06-02 (ckpt 37) — new-game config menu BUILDER ported: the text pipeline is closed end-to-end (build → render → bit-exact `TextOutA` stream)

Ported the construction half of the new-game ("Start") config scene —
`FUN_00564780` **case 0x24** + the grid setup `FUN_00411940` performs — into the
new `src/newgame_menu.{c,h}`.  This supplies the cells/geometry the (already
bit-exact, quirk #63) GDI renderer walks, so the port now **builds AND renders**
the new-game menu bit-identically to retail.

Run through `glyph_grid_render` at the box base **(x=32, y=32)**, the built grid
emits retail's captured `TextOutA` stream **draw-for-draw**: all **129**
menu-region glyph draws (3 rows × {col0 label, col1 value} × {shadow-down,
shadow-right, main}, the Start-Game row's empty value column skipped) match
`goldens/retail-newgame-config-textout.jsonl` exactly
(`tests/test_newgame_menu.c`).  **Geometry fully reconciled** (the ckpt-36
"builder geometry" TODO): col 0 origin **x=72** (entry[0].pos 0 + base 32 +
`field_c` 40), col 1 **x=232** (case-0x24's `entry[1].pos = 0xa0` override), row
pitch **28** (`node+0x1ac`), rows y=56/84/112; focus row 0 in 0xf08080, others
in 0x3e537d, shadow 0xa8b9cc.

Ported functions: `menu_grid_append` (`FUN_00412160` — a thin row append whose
per-column refresh loop is byte-for-byte `FUN_00411f40`, delegated to
`menu_row_finalize` and a no-op on fresh rows, quirk #36); the option string
providers `newgame_option_label`/`newgame_option_value` (`FUN_00566570`/
`FUN_00566a80` arms id 3/4); and `newgame_config_build` (the case-0x24 sequence).
Unported callees referenced by bare VA (`0x411940`/`0x40f800`/`0x412330`).  The
interactive run loop / nav / value toggle / tooltip node / box widget tree are
**not** ported — the scene is still an `app_flow` stub (re-enters the title);
wiring it as a drive is the next rock (quirk #64, new-game-flow.md).

3 new host tests (691→**694 pass / 0 fail / 6 skip**).  Ledger **154/1490 touched
(9.5%, 148 tested)** (+4: `0x412160`, `0x564780`, `0x566570`, `0x566a80` — the
last three partial).

---

## 2026-06-02 (ckpt 35) — text/glyph pipeline, part 2: the GDI text renderer is ported + host-tested

Ported the **render half** of the cell-grid dynamic-text system into the new
`src/glyph_render.{c,h}` (+ `glyph_render_win32.c`), the visual counterpart of
ckpt 34's build half. Three functions:

- `glyph_row_draw` (`FUN_0048e860`) — the per-glyph `TextOutA` loop; advance is
  **7 px per source byte**, so a 2-byte SJIS record steps 14 px.
- `glyph_ruby_draw` (`FUN_0048e6d0`) — the furigana pass; gated on
  `node->field_14` (== 0 for the basic menus) and a no-op on raw text
  (`flag1c == 0` on every record) — a faithful translation, unexercised for now.
- `glyph_grid_render` (`FUN_0048e200`, GDI branch) — walk rows
  `[sel2 .. sel2+stride)` × columns `[0 .. alloc_b)`, position each cell at
  `(entry.pos + x + node.field_c, entry.field4 + node.field_10 + lineH·dispRow
  + y)`, pick text/shadow `COLORREF`s by selection state (disabled / focused /
  normal / per-entry / per-cell override), apply the monospace right-align shift
  (`max(0, extent − 7·len)`), draw the **2-copy drop shadow** (offsets `(0,+1)`
  and `(+1,0)`) then the glyphs, and optionally the ruby pass.

GDI is reached through an injected `glyph_gdi_ops` vtable
(`select_font`/`set_text_color`/`text_out`) so the walk + colour selection are
**pure and host-testable** with a recording stub; the real back-buffer GDI lands
in `glyph_render_win32.c` (`glyph_gdi_ops_win32`), following the project's
`_win32.c` split (host harness never links it). Modelled `menu_cell` `+0x10`/
`+0x14` as the per-cell colour overrides the renderer reads (were `_pad10`).

**Three findings (quirk #62):** (a) the renderer's `this` is the **child**
controller node while the **parent** supplies the x/y base (`0x48c820` passes
`parent->field_c/field_10`); (b) the drop shadow is two offset copies of the
glyph row in `node+0x184`; (c) `node+0x188`/`+0x194`/`+0x198` hold label
**pointers** that the renderer reinterprets as `COLORREF`s — but only on dead
paths (disabled rows / ruby), so only `+0x180`/`+0x184`/`+0x18c`/`+0x190` are
live colours for the menus. The retail **sprite-cell mode** (`param_1 == 0`, a
ZDD-blit path) is deferred with a documented seam.

11 new host tests (681→**691 pass / 0 fail / 6 skip**). Ledger **150/1490
(9.2%)** (+3 tested: `0x48e200`, `0x48e860`, `0x48e6d0`). Commit: `feat: port the
GDI text renderer … text pipeline part 2 (ckpt 35)`.

**Open verification gate (human / Frida):** the "render a known string, diff vs
retail" pixel check — install the registered font at boot, render offscreen with
`glyph_gdi_ops_win32`, `differ_px`-diff the glyph region. The walk is host-tested
but the pixels are not yet diffed; that needs the live harness. **Next:** wire
`ar_register_fonts` at boot + the offscreen render path → pixel diff, then the
new-game config scene (`0x564780` case 0x24) + row-append `0x40f800`.

---

## 2026-06-02 (ckpt 34) — text/glyph pipeline, part 1: the glyph layout builder is ported + host-tested

Started the **glyph/text pipeline** — the shared gate for every dynamic-text
menu (new-game config, options, save/load) and the prologue narration (the
title top-level menu's labels are baked into a sprite; dynamic text is what
needs this). Surveyed the whole subsystem and wrote
`docs/findings/text-glyph-pipeline.md`.

**Two load-bearing findings (quirk #61):** (a) the engine renders dynamic text
through **Win32 GDI** — `ar_register_fonts` (already ported) builds 8 real
`HFONT`s and the renderer `FUN_0048e200` `TextOutA`s each glyph, so the drop-in
renders text by calling real GDI (no rasteriser to port); and (b) the layout
builder, row-append, and renderer all operate on the **same `menu_ctrl`/
`menu_node`** object already modelled in `menu_list.h` (descriptor `+0x174`,
`entries` `+0x178`, `rows` `+0x17c`, colour config `+0x180`) — the text system
is not a new container, just a builder + a GDI draw hung off the menu object.

Ported the **build half** into the new `src/glyph_text.{c,h}`:
`glyph_token_search` (`FUN_0040fd20`, the SJIS-aware substring search the escape
pass uses) and `glyph_cell_layout` (`FUN_0040fa00`, string → `cell.obj0` glyph
records). The raw split pass — one 2-byte record per SJIS lead, one 1-byte
record per ASCII byte — is faithful; the `#`-colour/control-code escape pass
(`0x4034f0`/`0x4051d0` over the `0x5cd978` table) is routed through a nullable
hook (NULL default = no-op, faithful for the escape-free ASCII/SJIS that covers
every English menu label). Corrected the **swapped Ghidra param names**
(`FUN_0040fa00`'s `param_1`/`param_2` are ROW/COL, recovered from the caller
`0x40f800`). 12 new host tests (680 pass / 0 fail / 6 skip). Ledger
**147/1490 (9.0%)** (+2 tested: `0x40fd20`, `0x40fa00`).

**Next:** the GDI renderer `FUN_0048e200` (+ `0x48e860`/`0x48e6d0`) with a real
HDC + registered font → render a known string offscreen and `differ_px`-diff vs
retail, then the row-append `0x40f800` + the new-game config scene
(`0x564780` case 0x24).

---

## 2026-06-02 (ckpt 33) — post-title dispatch backbone: the title menu is re-enterable, Exit exits

Until now the port treated **any** title-scene completion as a hard shutdown
(`main_loop_body`), so committing a menu row just exited. Retail doesn't:
`FUN_00562ea0`'s outer `do { } while(true)` loop (562ea0.c:684-734) switches on
the title runner's return code and either leaves the loop (Exit) or re-runs the
title to re-display the menu.

Ported the **result→action mapping** of that switch as a pure, Win32-free unit
`app_flow_dispatch` (`src/app_flow.{c,h}`): `6/8 → EXIT`, `9 → EXIT_9`,
`0x1a → NEW_GAME`, `0x1b → DEMO_START`, `0x1c → CONTINUE`, `0x1d → OPTIONS`,
`0x1e → BONUS`, `0`/default → `REENTER_TITLE`. Wired it into `main.c`: Exit sets
`g_shutdown`; every sub-scene arm (all UNPORTED — gated on the glyph/text
pipeline) logs + calls the new `reenter_title()`, which tears down the finished
drive and rebuilds it (skip_intro=1), so the menu loops the way retail's does.

**Verified live** (`--input-trace` + `--menu-trace`): a trace that walks DOWN×4
to **Exit** + confirm → `result=8` → clean `OpenSummoners exiting` (no re-enter);
a trace that confirms on **Start** → `result=26` → `dispatch: … not yet ported
(stub) — re-displaying title` → the drive rebuilds and the menu reappears. The
re-entered title **replays the intro from phase 0** — confirmed by a capture
showing the title art fading back in (quirk **#60**: the `local_164`/`param_1`
re-display arg does NOT skip the intro; it only enables a phase-0 skip-press).
668 host tests pass (+1 `app_flow_dispatch_codes`, 0 fail, 6 skip). Ledger
**145/1490 (8.9%)** unchanged — `0x562ea0` was already referenced/counted across
ported files; this is a partial port of its tail (status now `tested`).

## 2026-06-02 (ckpt 32) — the title menu is INTERACTIVE: injected nav moves the cursor + commits (milestone 1)

Live-validated the `--input-trace` path (the long-deferred ckpt-24 item) and
found the title menu was **dead to input** despite rendering bit-exact: an
injected DOWN never moved the cursor. RE traced it to the **menu-input gate**
(quirk #34): `menu_list_latch` refuses every nav action until `sub->ready`
(== the spawned node's `+0x54` ramp) reaches 1000, and `menu_node_build` zeroes
it — so the gate starts **closed**.

What opens it is the title scene's **post-update** side effect `FUN_0056c930`
(stubbed NULL in the port), NOT the per-entry update `0x43c2e0` (which only
*reads* `+0x54`). `0x56c930` is the menu-node transition updater; its **mode-1**
arm ramps the active node's `+0x54` by **+50/frame to 1000** (the node is built
mode 1, `+0x50`=1). Ported the mode-1 ramp as `menu_owner_transition_step`
(`src/menu_list.c`) — modes 0/2 are submenu-slide paths the title never uses,
documented + deferred — and wired it as the drive's `post_update`
(`src/main.c` `drive_post_update`). Quirk **#59**.

**Verified live** (`--menu-trace`, a new cursor-row-change diagnostic in
`src/title_sink.c`): injected DOWN×4 walks the cursor `0→1→2→3→4`, UP walks it
back, and confirm (`0x24`) on row N returns that row's action id — `result=26`
(`0x1a` Start) on row 0, `result=8` (Exit) on row 4. The cursor's ► arrow +
row-highlight visibly track the selection (pushed a port `Start`-vs-`Options`
capture to llm-feed). 667 host tests pass (+7 ramp tests); ledger **145/1490
(8.9%)** (+1: `0x56c930`). Note the gate (`+0x54`, +50/frame, open ~flip 547)
opens *before* the cursor draws (`fade==1000`, +20/frame, ~flip 577). This is
**milestone 1** (interactive title menu); next is the new-game config submenu.

## 2026-06-02 (ckpt 30) — LOGO + SPARKLE wired; both intro logos BIT-EXACT, subtitle-reveal sweep bit-exact

Wired the last two deferred title render-half arms (intro phases 0–7), the
recommended ckpt-29 next move. The RE collapsed two "hard" problems into reuse
of already-validated paths:

- **LOGO (studio + title).** r2 of `0x56bb5c`/`0x56bbd4` showed the "+4/+8
  container fields" of quirk #40 are just the MAIN-bank `frames[1]`/`frames[2]`
  walk (`*(*slot)` is the frames array `0x418470` indexes), and the handler is
  **bit-identical to the sprite-level wrapper** `0x56c4e0` (same ramp_b, same
  fade<=0/idx>=20/empty→plain-keyed rules; the lone `0x5bd550` a10-global
  difference is pixel-irrelevant). So `title_render_logo` now emits one
  `TITLE_DRAW_SPRITE_LEVEL` (frame 1/2, raw fade). This **fixed a real bug**: the
  old branch keyed on the scene `ramp` param (`fade_ramp`), never populated by
  `main.c`, so logos rendered **opaque, unfaded**. Now they fade through the
  sink's populated `ramp_b`. Quirk **#56**.
- **SPARKLE (phase-7 subtitle reveal).** `0x56bcf7` copies 4×48 slivers of the
  menu-bg sprite (MAIN frame 5) src `(x,416)`→dst `(x,416)`, x 192..<416 step 4,
  alpha from `ramp_b` of `min(7·fade−100·i,1000)` — revealing "Secret of the
  Elemental Stone" column-by-column. The cmd now carries the raw clamped level +
  column (round-trips, unlike the old 64-bit blend-pointer-in-32-bit-field) and
  the sink does the ramp lookup + the `title_draw_sparkle` blit. Quirk **#57**.

**Verified (the R1 fade-matched method).** New `frida_capture.py --fade-probe`
(hooks `0x448c80`, logs the first fade per Flip = the logo fade in phases 0–4).
Matched-fade diffs: **studio logo phase 0 fade 640 → `differ_px=0`; title-art
logo phase 3 fade 820 → `differ_px=0`** (parity-ledger #2/#3, user-confirmed 1:1
on the pushed comparisons). Sparkle at full reveal (fade 1000): the SUMMONERS
logo + subtitle banner match exactly; the only residual (1208 px, 96.6 %
retail-brighter) is retail's **additive sparkle particles** from the separate,
still-deferred `FUN_0056c070` particle spawn (parity-ledger R4 — a noted gap, not
a sweep error). Fade-probe caveat: in phase 7 it logs the first *sparkle* level,
not the raw fade — match by reveal extent there.

650 host tests pass (0 fail, 6 skip; +2 sink sparkle tests, logo scene test
reworked). Ledger 138/1490 unchanged (wiring, no new FUN).

---

## 2026-06-02 (ckpt 29) — R3 intro pacing: diagnosed + fixed (render-rate artifact, not a rush)

**Reframed and resolved R3.** The "port rushes the intro" hypothesis was wrong.
Measured both sides with the real clock — new `frida_capture.py --pace-probe`
(timestamps Flips; generalises `--cursor-probe`) on retail, and a new `pace:`
phase-transition log in `src/main.c` on the port:

- **Retail:** menu (cursor) onset at Flip **1172 @ 9.23 s**, render rate
  **~127 flips/s**, each `menu_fade` value spanning ~2 consecutive flips (pure
  duplicate frames — display refresh > update rate).
- **Port (pre-fix):** menu at Flip 90 @ 9.87 s, ~9 flips/s. **Same wall-clock,
  1/13 the flips.**

So the wall-clock pacing already matched (~9.2 s to menu); the gap was that the
fixed-timestep accumulator (`title_pace_step`) was being driven **one pace-step
per 16 ms-throttled main-loop iteration**, making `now` advance per *update* and
the budget refill run away to ~6 updates/render — the port **dropped ~5/6 of the
intro's fade frames** (rendered 90 of ~528 update ticks; choppy fades).

**Fix (`src/main.c` `main_loop_body`):** drive the pace machine like retail's
tight outer loop — spin pace-steps (updates ~free, detect a present via
`g_present_frame`) until one frame is presented, then `frame_limiter` gates the
presented-frame rate. Validated first in a Python replica of the two FSMs
(`/tmp/pace_sim.py`): ratio → **1.00**, **MISSED=0** menu_fade values. Live: the
phase curve is now the canonical 51/102/153/254/275/316/437/**528**, every fade
value renders, wall-clock unchanged. R1 re-verified post-fix at `menu_fade=750`
(port Flip 594 vs golden 1300) → **differ_px=0** (rendering path untouched).

Flip-index-exact parity with a golden is the capture rig's refresh (~127 Hz) and
is **not portably reproducible**; the distinct-content sequence is, and now
matches. New quirk **#54** (accumulator must keep the engine's call cadence, not
be re-paced by the host frame limiter). Ledger 138/1490 unchanged (driving fix +
instrumentation, no new FUN). 648 host tests pass. **Open:** `LOGO`/`SPARKLE`
arms still unwired, so intro phase 0–7 *content* parity is still gated on those.
**User-reported (next):** hidden game window flickers/bleeds through screen.

## 2026-06-02 (ckpt 28) — R1 CLOSED: title menu bit-exact, cursor pulse RE'd

The title menu now matches retail **`differ_px == 0`** (parity-ledger #1) —
including the selected-row cursor brightness, the last residual (R1, was 955 px).
User-confirmed fully 1:1 bit-identical.

Root cause (RE'd, not eyeballed — per user direction): the cursor's
`level_num` (`FUN_0056c470`'s 3rd arg = `[esp+0x20]`) is **not a constant** —
retail animates it as a triangle wave.  It is `local_58` in `FUN_0056aea0`,
driven by the phase FSM (`56aea0.c`:366-384): phase 8 ramps `+50`/update to
1000, phase 9 ramps `-50`/update to 0, oscillating.  With the fixed
`level_div = 0x4b0` (1200), `idx = (local_58*20)/1200` sweeps **0..16** (peak
16, NOT 19) and the cursor is invisible at the bottom of each breath.  The port
had wired the cursor to a static idx-19 full-add → uniformly over-bright (every
differing pixel was port>retail).  The port *already* computed the value as
`title_fade_state.menu_fade`; it just never passed it to the draw.

Fix: thread `menu_fade` → cursor `level_num` (`title_render_menu` /
`title_render_step` / the `TITLE_DRAW_MENU_CURSOR` sink arm: `cmd->level` =
level_num, `cmd->alpha` = level_div = 0x4b0).  Validated by capturing port
frames (which now log `phase`/`fade`/`menu_fade`) and matching them to retail
goldens captured *with* the new `--cursor-probe` (so each golden's `local_58`
is known), then diffing at equal pulse phase: port Flip 209 (`menu_fade=750`)
vs retail Flip 1300 (`local_58=750`) → 0 px; port 203 (450) vs goldens
1420/1460 (450) → 0 px.  See engine-quirks #52.

Tooling: `frida_capture.py --cursor-probe` (hooks `FUN_0056c470`, logs per-Flip
`level_num`/`level_div` → `cursor_level.jsonl`).  Also **fixed the harness's
default exe** (engine-quirks #53): it spawned the *packed* `sotes.exe`
(0 frames — DRM stall); now spawns the Steamless-unpacked PE co-located in the
game dir (`setup.sh` copies it there; the engine resolves assets by module dir,
not cwd).  648 host tests pass (0 fail, 6 skip); ledger unchanged 138/1490
(wiring, no new FUN).  The port Flip index still ≠ retail's (intro-pacing R3
remains) — but at equal pulse phase the frames are pixel-identical.

## 2026-06-02 (ckpt 24) — port-side input replay (`--input-trace`)

Ported `input_trace.{c,h}` (commit `50b348d`) — the port-side counterpart of
the Frida harness's deterministic input injection. It parses the *same* sparse
`{"frame":N,"ids":[..]}` JSONL the harness writes (`tools/frida_capture.py`) and
replays it into the title-scene drive's input ring as fresh presses
(`{id, flag=1, ts=now}`, round-robin slots), keyed on the present/Flip count.
So a scripted scene walk (e.g. the new-game path in
`docs/findings/new-game-flow.md`) can drive the *port* deterministically — the
port-side half of the replay → port-frame-capture → golden-diff pipeline.

`main.c` gains `--input-trace <file>`: loads the trace once the drive stands up
and injects due entries each iteration before the scene polls; `g_present_frame`
(bumped in `drive_present`) is the Flip-anchored frame axis. No-op without the
title-scene drive. Pure C, host-tested (7 tests, 636 total: 630 pass / 0 fail /
6 skip): parse basic / tolerant (comments, blanks, key-order, missing ids) /
out-of-order / malformed; replay injects-once-at-frame / catch-up / NULL+empty
guards. Both cross-builds clean; ledger unchanged at 131/1490 (port-side
tooling, no retail FUN). Live validation (does an injected press actually skip
the splash / nav the menu) is deferred to next session; the port-frame-capture
+ diff half is still gated on 8d pixels.

## 2026-06-02 (ckpt 23) — 8d's opaque-trim scanner ported; the whole 8d call graph decoded

Ported `bs_trim_opaque_rect` (`FUN_005b6f80`, commit `2372a3f`) — the
trim-metadata builder the per-cell sprite-surface builder (8d, `0x5b9630`) runs
to size each cell: it scans a W×H window of the decoded bottom-up DIB for the
tight bounding box of opaque (non-colour-key) pixels and reports `found_opaque`
/ `found_key` (the builder skips a fully-transparent cell → a metrics-only
ZDDObject, and drops the colour key on a fully-opaque cell → the `0x1ffffff`
sentinel). The leaf helpers `0x5b6f00` (depth) / `0x5b6ec0` (bottom-row) are
one-liners, inlined.

**RE landed a real asymmetry → engine-quirk #48:** the 24bpp scan gates its
y-bounds on the *global* `x_left < W` (so `y_bottom` runs to `H-1` once any
opaque pixel exists anywhere), while the 8bpp scan uses a *per-row* opaque flag
(tight `y_bottom`). The headline test pair trims the same opaque shape at 24bpp
(`y_bottom`=7) vs 8bpp (`y_bottom`=4); +6 host tests (629 total, 623 pass / 0
fail / 6 skip). Ledger 131/1490 (8.1%), 128 tested (+1).

**Also decoded (no port — for the live session):** the rest of 8d's call graph
(`0x5b9280` build = new+ctor+`0x5b9630`+dtor; `0x5b9630` orchestrator =
trim-gate → `create_surface_pair` → `0x5b9910`; `0x5b9910` = Lock(`0x5b9490`) +
clip + zero + per-format copy via the `0x5b7310/_74f0/_7270` + `0x5b7bd0`
converters), with arg mappings, in `docs/findings/sprite-pipeline.md`. The
pure-logic 8d pieces are now all ported; what remains is DDraw-bound +
interdependent (Lock, pixel copy, format converters) — best ported + verified
together in the live session, where a registered bank + the real display depth
let the produced pixels be diffed against the harness goldens. So next session's
8d work is *wire + verify*, not RE-from-scratch.

## 2026-06-02 (ckpt 22) — the title scene is driven from `main.c`: the loop runs live

Wired the milestone-0 capstone: the ported title runner (`FUN_0056aea0` =
`title_scene_step`) is now **driven by the drop-in's `main.c`**, with the render
sink bound to the live primary surface. The scene that was a tested-in-isolation
unit is now the drop-in's actual per-frame loop.

**`src/title_drive.{c,h}` (ckpt 22a, commit `c90f834`)** — the *caller* side of
`FUN_0056aea0`, the plumbing its retail caller `FUN_00562ea0` owns, factored
Win32-free so it host-tests. `title_drive_init` allocates the scene's object
graph (a `sel_list` menu-tree owner + its 0x1b0 `menu_node` at `entry[0]` — the
slot the lazy phase-8 `title_menu_spawn` configures — and the `input_mgr` with a
**fully-populated idle ring**, since the poll/scan paths deref `ring[i]` with no
NULL guard), binds the render sink (`title_sink_ctx` → `title_render_sink_hook`),
and zeroes the FSM via `title_scene_init`. `title_drive_step` runs one
`title_scene_step` and latches the result on `TITLE_SCENE_DONE` (idempotent
after). `title_drive_shutdown` unbinds + frees the graph (mirrors the test
harness's `free_spawn`). **6 host tests, 617 pass / 0 fail / 6 skip** (+6):
init-allocates-and-binds, FLIP→present on a render frame, NULL-primary headless
no-op, abort-poll→DONE + idempotency, menu spawn + clean teardown (LSan),
shutdown safety on never-spawned / uninitialised drives.

**`main.c` wiring (ckpt 22b, commit `d9c75d9`)** — after DDraw init in mode 2,
`init_title_drive` binds the sink to `g_zdd->primary_obj` with `drive_present`
(→ `zdd_present`) and `drive_log_flip` thunks, installs `ar_sprite_decode_hook`
= `ar_sprite_decode` (banks self-decode once registered), and allocates the
drive. `main_loop_body` runs one `title_scene_step` per iteration; a render
iteration presents through the sink's `TITLE_DRAW_FLIP`; scene completion logs
the menu-action result and stops (the outer action dispatch lands when later
scenes are ported). `--no-title-scene` falls back to the legacy minimal present
loop. Both 32-bit cross-builds clean.

**The 8d per-cell surface builder (`0x5b9280`, `ar_frame_build_hook`) is still
NULL**, so every sprite resolves to NULL ⇒ the scene renders a **cleared +
flipped window with no sprites** — HANDOFF "move B", the prove-the-loop-live
state. Alpha ramps (`0x8a92b8`/`0x8a9308`) + the compositor display group are
unfilled/unmodeled at a cold boot, so they pass through as NULL (plain blits /
no compose — faithful). **Wants live verification next session** (Frida
self-serviceable): confirm zero DDERR + a flipping window, then wire 8d for real
sprites. Cadence note: `main.c`'s 16 ms `frame_limiter` throttles one step per
iteration (so ~1 render per ~2 steps); the scene's pacer adapts. Tune against
the live window next session if the visible rate is off. Ledger unchanged at
**130/1490 (8.1%), 127 tested** (the drive composes already-counted functions;
ref-bumps only).

## 2026-06-02 — Sprite-sheet decoder `FUN_004184a0` + slicer `FUN_004188b0`: the genuine pixel source is ported, ckpt 20

Ported the sprite-sheet decoder chip — the `ar_sprite_decode_hook` target that
turns a bank's PE "DATA" resource into the per-frame surface array the frame
getter returns.  The chip shrank dramatically once we found the whole
resource-load + DIB/decompress layer was **already ported** as `bitmap_session`
(`bs_decode_resource` etc.), so this checkpoint is the remaining decode logic:

- **`FUN_004184a0` → `ar_sprite_decode`** (asset_register.c) — entry-0 decoder:
  re-decode cleanup (release old frames + free the array) → `bs_decode_resource`
  (compressed) → gated 24bpp brightness pass → `ar_sprite_slice` → `bs_release`.
  On resource failure the drop-in leaves the bank unloaded (frames NULL, the
  getter's "still undecoded" path) instead of retail's process-exit.
- **`ar_sheet_decode_pixels`** — the pure 24bpp colour transform (the genuine
  new pixel logic, fully host-tested): per-channel `ch * scale / 1000` with an
  optional gamma LUT (`slot->f_18`), magenta `0xff00ff` left untouched, and the
  **reversed** byte→field mapping (byte0·f_14, byte1·f_10, byte2·f_0c) —
  engine-quirks **#46**.  Reads 3 bytes/pixel (not retail's dword) so the last
  pixel never reads past the buffer under ASan.
- **`FUN_004188b0` → `ar_sprite_slice`** — frame-grid geometry
  (`cols=sheet_w/cell_w`, `rows=sheet_h/cell_h`), `slot->f_38=count`, allocate +
  fill `entries[0].frames` via the per-cell surface-builder hook.  The
  builder's colour-key is hardwired to magenta unless the `0x1ffffff`
  sentinel — engine-quirks **#47**.

The DDraw leaf layer stays behind nullable hooks (`ar_frame_build_hook` =
`0x5b9280`, `ar_frame_free_hook` = `FUN_005b9390`; the trim-metadata `0x5b6f80`,
format switch `0x5b7310/_74f0/_7270`, and 8bpp palette `0x5b7bd0` are deferred).
Headless sizes + zero-fills the frames array but leaves surfaces NULL.

**605 host tests (599 pass, 0 fail, 6 skip; +12 this ckpt)**; both 32-bit
cross-builds clean.  Ledger **130/1490 touched (8.1%), 127 tested** (+2:
`FUN_004184a0` + `FUN_004188b0`, both inventoried).  Added a shared
`tests/bs_fixture.h` so the end-to-end decode test can register a synthetic
compressed 24bpp "DATA" resource without duplicating the builder.  Full decode
in `docs/findings/sprite-pipeline.md`.

## 2026-06-02 — Clipped Blt `FUN_005b9bf0` + sparkle wrapper: the whole title compositor/wrapper layer is ported, ckpt 19

Ported the last two render-bridge chips, completing every `TITLE_DRAW_*` arm:

- **`FUN_005b9bf0` → `zdd_object_blt_clipped`** (zdd.c) — the third blt
  sibling: a color-keyed Blt whose source rect is clipped against the src
  object's placement metrics (metric_0c/_10 origin, metric_14/_18 extent)
  with an explicit source sub-origin, the dest rect shifting to compensate
  for any left/top clip.  Pinned from the Ghidra decomp at
  `docs/decompiled/by-address/5b9bf0.c` (the hand stack-trace was ambiguous;
  the decomp made the clip algebra unambiguous).  Reuses the existing
  `zdd_surface_blt` primitive.  Returns 1 (no surface) / 0 (collapsed
  region) / HRESULT like its siblings.

- **`FUN_0056c580` → `title_draw_sparkle`** (title_render.c) — the sparkle/
  trail wrapper, gated directly on a caller-supplied descriptor (not a ramp
  lookup): desc != NULL → `zdd_blit_orchestrate` alpha with an **explicit
  source sub-rect** (the only wrapper that doesn't use src 0,0 + the
  sprite's full metric_14/_18); desc == NULL → `zdd_object_blt_clipped`.
  The alpha path metric-offsets the dest origin; the clipped path passes the
  raw origin (the clip applies the metric internally) — a retail asymmetry
  preserved literally.

**8 new host tests** (6 blt_clipped: null-src, no-clip rect forward,
left/top clip, extent clamp, collapsed→0, null-dest defensive; 2 sparkle:
alpha vs clipped path selection + arg mapping).  **587 host tests pass, 0
fail, 6 skip.**  Both 32-bit cross-builds clean.  Ledger **128/1490 touched
(7.9%), 125 tested** (+1 = blt_clipped; the four 0x56cxxx wrappers are
sub-helper labels — real ports not in functions.csv, so the headline holds).

The compositor + all four wrappers + the three blt primitives behind them
are now ported.  Next render chip = the sprite-sheet **decoder `0x4184a0` +
slicer `0x4188b0`** (the genuine pixel source; needs the sheet binary format
pinned), then the **sink + drive from `main.c`**.  See
`docs/findings/sprite-pipeline.md`.

---

## 2026-06-02 — Compositor `FUN_0056c180` ported into a new render-bridge module, ckpt 17

Ported the per-frame sprite-display-list **compositor `FUN_0056c180`** as
**`title_compositor_draw`** in a NEW module **`src/title_render.{c,h}`** — the
first to bridge both `asset_register.h` (the sprite pool + frame getter) and
`zdd.h` (the blit primitive + surfaces), exactly the home the next render
chips (the wrappers + the sink) need.

The valuable per-entry logic is factored into a pure helper
**`title_compositor_resolve`**: the animation frame-index math
(`p = min((anim_num*frame_count)/anim_div, frame_count-1)`,
`frame = (uint16_t)(frame_base - p + frame_count - 1)`), the asset-pool +
frame-surface lookup (`ar_pool_get_slot` → `ar_sprite_slot_frame`), the
blend-ramp index clamp (`(alpha_level*20)/1000`, clamped to [0,19] — retail's
`[0x8a9304]` default literally `== &ramp[19]`), and the centi-pixel geometry
(`metric_0c + x_num/100`).  All of retail's magic-reciprocal divides
(0x51eb851f → ÷100, 0x10624dd3 → ÷1000, the signed idiv ÷anim_div) are plain
C int division; disasm-verified at 0x56c19b..0x56c28d.

The display-list entry (0x1c B) + group header are modeled as
`title_sprite_entry` / `title_sprite_group` with `_Static_assert`-pinned
offsets.  The 20-entry blend-descriptor ramp `0x8a92b8` is passed in as a
parameter (NULL ⇒ all blits no-op), mirroring the existing `title_fade_ramp`
decoupling — in retail it IS pixel_drawer's `g_pd_boot_group_a` viewed as a
pointer table.  Headless-safety skips: `anim_div == 0` (retail faults) and a
NULL resolved sprite (retail derefs NULL) both drop the entry.

**10 new host tests** (resolve: basic geometry/frame/ramp, min-clamp, u16
frame mask, ramp lo/hi/mid clamp, div-0 invalid, NULL sprite/entry, NULL ramp;
draw: iterate+skip-invalid+arg-forward via a capture hook, NULL/empty group
no-op).  **572 host tests pass, 0 fail, 6 skip.**  Both 32-bit cross-builds
clean.  Ledger **127/1490 touched (7.9%), 124 tested** (+1 = the compositor).

Decode + the ckpt-16 getter: `docs/findings/sprite-pipeline.md` (compositor
section now marked PORTED).

---

## 2026-06-02 — Sprite frame getter `FUN_00418470` ported; the render sink's asset/sprite pipeline mapped, ckpt 16

Scouted move #1 ("build the render sink + drive the runner") and found it is
**gated on an unported asset/sprite subsystem** — every sprite draw resolves a
*frame surface* out of the asset pool `DAT_008a760c[bank_id]`, lazily decoded
from a sprite sheet.  Fully decoded the chain and wrote
**`docs/findings/sprite-pipeline.md`**: pool → `ar_sprite_slot` (the "bank") →
`bank->entries[0].frames[frame]` → `zdd_object*` surface → `zdd_blit_orchestrate`;
the compositor `0x56c180` walks the scene's display list and blits each entry;
the wrappers `0x56c610/_4e0/_470/_580` each resolve one sprite the same way.

**Reuse find:** the "bank" is the **already-pinned `ar_sprite_slot`** (0x44 B)
indexed by the existing `ar_pool_get_slot` — caught myself starting a duplicate
`zdd_sprite_bank` model and reverted to build on `asset_register`.

**Ported:** the frame getter **`FUN_00418470` → `ar_sprite_slot_frame`** in
`asset_register.c` — the two-level `slot->entries[0].frames[id]` lookup with
lazy decode routed through the nullable `ar_sprite_decode_hook` (the decoder
`0x4184a0` is a later chip).  Widened `ar_sprite_entry.a` (opaque `uint32_t`) →
**`void *frames`** to pin its role + make the getter host-testable (still 4 B
on the 32-bit build, so the 8-byte record holds).

**562 host tests pass, 0 fail, 6 skip** (4 new: null slot/entries, loaded-index
without hook, lazy-decode-fires-once, headless-no-hook).  Both 32-bit
cross-builds clean.  Ledger **126/1490 touched (+1), 123 tested (+1)**.  Next:
the compositor `0x56c180` (decoded, wants a new render-bridge module), then the
sheet decoder `0x4184a0`, then the sink + drive from `main.c`.

---

## 2026-06-02 — Blit orchestrator `FUN_005bd550` + `FUN_005b9ae0` ported; complex path proven dead, ckpt 15

Ported the **blit orchestrator `0x5bd550`** (`zdd_blit_orchestrate`) and its
sibling **`0x5b9ae0`** (`zdd_object_blt_rects`) — render task #8 from the ckpt-13
list.  `0x5bd550` is the single chokepoint every title-screen sprite draw funnels
through: the per-frame compositor `0x56c180` and the sprite wrappers
`0x56c470/_4e0/_580` all call it.  It is `__thiscall` on a `zdd_blend_desc`, with
10 stack args (`dest, src, dst_x, dst_y, w, h, src_x, src_y, colorkey, gdi_ctx`).

**Scout result (ckpt-13 move #1, "does the basic title render via the plain
path?"):** answered by disassembling all four sprite wrappers + the compositor.
`0x56c610` (plain) and `0x56c4e0` (leveled, at full brightness — ramp entry 0 per
quirk #40) forward to the already-ported `0x5b9b70` (`zdd_object_blt_keyed`); but
the **cursor `0x56c470` always** and the **per-frame compositor `0x56c180`
always** funnel through `0x5bd550`.  So the static logo+menu-text can draw plain,
but the cursor and the compositor can't — `0x5bd550` was the real next chip, not
a shortcut to move #2.

**`0x5bd550`'s simple path is the only live one.**  An exhaustive write-search of
the image (all `mov [0x8a6ec0], *` encodings) shows `DAT_008a6ec0` — the global
every caller passes as `gdi_ctx` — is **written only to zero**, never to a
surface.  So the complex path (GDI `BitBlt` into a scratch surface + hardware
`Blt` back via `0x5b9ae0`) **never executes**, and `0x5b9ae0` is reachable only
from it → also dead.  New **engine-quirk #45**.  The simple path is just
`zdd_object_lock(dest)` → `zdd_alpha_blit` → `zdd_object_unlock(dest)`, all
already-ported primitives; the complex path is still ported faithfully (new
`zdd_dc_blit` GDI seam in `zdd_win32.c`) but exercised only by a host test.

**558 host tests pass, 0 fail, 6 skip** (5 new: blt_rects null-src/null-dest/
rect-math, orchestrate simple lock→blit→unlock, orchestrate complex GDI
round-trip incl. the 16px min clamp).  Both 32-bit cross-builds clean.  Ledger
**125/1490 touched (+2), 122 tested**.  (Caught the recurring stray-`FUN_`
inflation again: `FUN_0056c180`/`_0056c470` in a docstring bumped the count until
rewritten as bare VAs.)

## 2026-06-02 — Software alpha blitter `FUN_005bd680` ported, ckpt 14

Ported the 1072-byte **software alpha/colorize blitter at `0x5bd680`** — the
heart of the title sprite-draw subsystem and the cleanest first chip of the
milestone-0 render bridges (ckpt-13 render task #7).  It is `__thiscall` on a
small blend descriptor (new `zdd_blend_desc`: `mode` + three `{shift,mask,LUT}`
channel records at `+0x04`/`+0x18`/`+0x2c`).  Three blend modes, all skipping
source pixels equal to the colorkey: **0** `out_ch = lut_ch[(src&mask)>>shift]
<< shift` (1-D remap), **1** `out_ch = lut_ch[(src_lvl<<5)+dst_lvl] << shift`
(2-D src×dst blend, reads the dest pixel), **2** `g = (ch0+ch1+ch2)/3; out_ch =
lut_ch[g] << shift` (colorize).  Split into a pure host-testable core
(`zdd_alpha_blit_pixels`, raw 16bpp buffers) + the retail wrapper
(`zdd_alpha_blit`) that reads dest geometry directly (caller pre-locks dest),
locks/unlocks only the **source**, and no-ops on a failed source Lock — all
mirroring retail (orchestrator `0x5bd550` locks dest first).  Clipping mirrors
retail exactly: right edge clamps to dst stride-in-words, bottom to dst height,
negative origins shift the source and pin dest to 0.

New **engine-quirk #44**: mode 1 hardcodes the src-level stride at 32
(`shl ebp,5`) for every channel, even 6-bit green; literal mirrored, the LUT
layout + descriptor ctor are a later chip.  11 host tests (3 modes, colorkey
skip, LUT transform, both clip axes, invalid-mode no-op, null guards, wrapper
lock/unlock + lock-fail).  **553 pass / 0 fail / 6 skip**; both 32-bit
cross-builds clean.  Ledger **122→123 touched, 119→120 tested**.  Commit
`cd95935`.

Scouted the rest of the alpha subsystem for the next checkpoint: orchestrator
`0x5bd550` (302 B; simple path = lock-dest → `0x5bd680` → unlock-dest, complex
path adds GDI BitBlt + a hardware Blt) calls `0x5b94e0`/`0x5b9500` (already
ported = `zdd_object_get_dc`/`_release_dc`) and the **unported** `0x5b9ae0`
(140 B Blt-with-explicit-rects, sibling of `zdd_object_blt_keyed`; its 9-arg
order is pinned by the `0x5bd550` call site).

---

## 2026-06-02 — Skip-splash early-out ported; update half complete, ckpt 12

Ported `FUN_0056aea0`'s **skip-splash early-out** (`0x56b0e8..0x56b150`, "press
a button during the intro to jump straight to the menu") — the last
documented-deferred slice of the title scene's update half.  It now sits in
`title_scene_step` right after the `0x22` abort poll: scan the input ring for any
fresh press (`input_any_fresh_press`); on a hit, zero the fade, fire the BGM
`SetNextSegment` cue when still before phase 3, flush the ring + axis state
(`input_mgr_reset`), and force phase 8.  The gate honours the scene's new
`skip_intro` field (`param_1`): clear → a press is ignored at phase 0 (a first
boot plays the studio fade in full), set → it skips from phase 0 too; phases 1..7
always skip on a press.  No engine subsystem pulled in — the BGM cue reuses the
existing `set_next_segment` hook.

The flush block (`0x56b25e..0x56b29a`) mapped the input manager past the ring:
the two "axis-held" flags at `+0x114`/`+0x118` the title menu reads are actually
`array_A[0]`/`array_A[1]` of an **11-dword array**, with a parallel array B at
`+0x140`, plus `+0x10c`/`+0x110`/`+0x16c` scalars — all cleared by the skip.
`input_mgr` was extended to model them (offsets pinned by `_Static_assert` on the
32-bit build) and the two `axis_held_v/h` reads became `axis_held[0]/[1]`.  New
finding → **engine-quirks #41** (let the *reset* code, not the read code, reveal a
struct's true array shape).  The one piece left out is retail's scene-local
sparkle-counter reset (`var_3eh_2`), which belongs to the still-deferred
sparkle-trail subsystem (spawned/advanced outside the runner via a hook).

**542 host tests pass, 0 fail, 6 skip (of 548)** — 8 new (the two input
primitives: any-fresh-press match/empty/flag+age, reset flushes all fields; and
the skip-splash path: jump-to-menu with fade reset + ring flush, the BGM cue
gating, the phase-0 `skip_intro` gate both ways, and the no-press no-op).  Both
32-bit cross-builds clean.  Ledger **122/1490 touched (7.5%)** unchanged — the
early-out is a slice of the already-counted `FUN_0056aea0`; its new input helpers
reference the slice by bare VA.  See `findings/input.md` (wider manager model),
`findings/title-scene.md` (skip-splash now ported).

## 2026-06-02 — Parity harness live-verified against retail under Frida, ckpt 12

Ran the structural-parity call-trace harness against **live retail under
Frida** for the first time (Frida is always up + UAC auto-approved on this
host, so the "human-verification gate" is really self-serviceable — noted in
memory).  Result: it **works end-to-end**.  A `--no-turbo` capture with the
*full* 1743-VA candidate set hooked at once booted retail to its title window
and emitted **1.8M call-trace events over 1914 Flip frames with zero
crashes** — so `tools/bisect_call_trace_vas.py` proved **unnecessary** for
this boot path; `engine_vas_frida_safe.json` was written directly from the
full set.

Two hard live findings.  (1) **Everything live must be `--no-turbo`:** turbo
freezes the splash before the engine reaches its message pump (quirk #29) —
a turbo boot reports `msg_count=0` / no Flips, so the bisect's turbo default
made every subset read as a crash (the calibration trap the file header
warned about).  No-turbo boots cleanly (`msg_count` ~750-1000, 1914 frames).
Fixed the bisect to default `--no-turbo`.  (2) Mining the trace for the title
render path **confirms the ported control flow and the next port target
against live retail**: `0x56c180` compose / `0x5b8fc0` Flip / `0x5b1030` pump
fire once per frame; `0x56c930` post-update fires ≈ half as often (validating
the pacing-FSM update/render split); and the **software alpha blitter
`0x5bd680` (+ orchestrator `0x5bd550`) is the hot per-frame draw primitive
(4279 calls)** — the recommended next render-bridge chip.  The call_trace
*diff* is now blocked only on the port side (main.c must drive
`title_scene_step`).  Details + the per-frame table in `docs/parity-harness.md`.

## 2026-06-02 — Title scene runner wired into one orchestrated loop, ckpt 11

Composed the ckpt 1–10 units (the pacing FSM, the fade FSM, the menu spawn,
the per-frame input dispatch, the render step, the teardown) into the one
running title-scene runner — `title_scene_step` / `title_scene_init` in
`src/title_scene.c`, ported from `FUN_0056aea0`'s outer `do { … } while(1)`
body (`0x56b002..0x56ba75`).  One call = one loop iteration: sample
GetTickCount → `title_pace_step` (pump fires through a hook when requested) →
on a `TITLE_PACE_RENDER` iteration `title_render_step` draws+presents and
loops; on a `TITLE_PACE_UPDATE` iteration the update half runs (pre-update
side effects → the `0x22` abort poll → the phase switch = `title_fade_step`,
with `title_menu_spawn` on first menu entry + `title_menu_input_step`, and
`title_menu_teardown` before the phase-10 fade-out → the per-frame tail:
watchdog increment, post-update, per-owner-entry update).

This is the piece "between a pile of units and a running title scene": the
whole milestone-0 control flow now exists as one composable, testable unit.
Every still-unported per-frame engine call (`0x5b1030` pump, `0x43e140` +
`0x40fe00` + `0x566250` pre-update, `0x56c930` post-update, `0x43c2e0`
per-entry, the BGM SetNextSegment cue, `0x56c070` sparkle spawn) is routed
through a nullable `title_scene_hooks` struct; the menu-input side effects
keep using the existing `title_menu_*_hook` globals; the render bridges keep
using `title_render_sink_hook`.  So the runner assembles and tests now without
pulling in audio / DInput / DDraw / the god object.

**Anatomy confirmed while wiring** (folded into `findings/title-scene.md`):
the scene returns *only* out of the update half — the `0x22` abort poll
(result 6) or the phase-10 fade-out completing (result = the committed menu
action, or 0 on a watchdog timeout); the render half never returns, it loops
(reinforcing the ckpt-10 finding that Ghidra's "jump table as call+return"
was an artifact).  The idle watchdog (`local_50`) increments on *every* update
frame across *all* phases — so the ~75 s timeout counts the intro, not just
menu-idle time.  The menu both spawns *and* runs its first input poll on the
same frame, with the input gate still closed (so the first menu frame can't
latch).

**Deferred (documented seam):** the **skip-splash early-out**
(`0x56b0e8..0x56b150`, "press any button during the intro to jump to the
menu") is not ported here — it walks the input-mgr ring directly and fires a
second SetNextSegment cue; it's a separable intro-convenience reading
input-mgr internals the poll port doesn't model yet.  Without it the intro
always plays in full (the headless `param_1==0` default).

**534 host tests pass, 0 fail, 6 skip (of 540)** — 7 new (runner init/bind,
a render iteration presents without touching the update half, the abort poll
returns 6, the full intro walk to the spawned menu, the watchdog forcing the
fade-out exit, the full intro→menu→commit→exit "money path" returning the
selected action 0x1a, and NULL-hooks safety; ASan/UBSan clean).  Both 32-bit
cross-builds clean.  Ledger **122/1490 touched (7.5%), 119 tested**
(unchanged — an extension of the already-counted `FUN_0056aea0`; unported
callees referenced by bare VA).  The commit-path test had to point the
controller's gate (`ctrl->sub`) at a real `menu_input_sub` rather than the
node it aliases on 32-bit — the same host/target struct-padding divergence
quirk #38 documents.

---

## 2026-06-01 — Title scene render half ported, ckpt 10

Ported `FUN_0056aea0`'s **render branch** (`0x56bb04..0x56bf1a`) into
`src/title_scene.c` as **`title_render_step`** — the path the frame pacer
dispatches to on a `sub==1` (`TITLE_PACE_RENDER`) frame.  This is the last
piece of the title scene: with the update half done at ckpt 9, the runner's
whole control flow (fade FSM + pacing FSM + update + render) is now ported.
The fade→alpha helper `0x448c80` is ported alongside as the pure
`title_fade_ramp`.

The render half draws one frame for the current phase: a prologue (phase 0 →
surface reset `0x5b9410`; phases 2–3 → surface clear `0x5b9b70`; phase > 10 →
skip to frame-end), then the **recovered 11-entry jump table** at `0x56bfa4`
dispatching `jmp [phase*4+...]` to one of 7 inline handlers (studio/title-logo
alpha blits, the two "press button" sprite pairs, the sparkle trail, the menu
bg+sprite+cursor, the menu fade-out), then the universal frame-end at
`0x56bec4`: compose `0x56c180` → "Title Menu - Flipping" log (once, unless the
`DAT_008a6b54` quiet flag) → Flip `0x5b8fc0` — the documented "title menu drew
a frame" event.

It is heavily DDraw/asset/object-model-coupled — every leaf the handlers call
(`0x494e10`, `0x418470`, `0x56c610/_4e0/_580/_470`, `0x56c180`, `0x5b8fc0`, …)
is unported.  Per the handoff's guidance, rather than a hook per bridge, they
are reported as an ordered stream of tagged `title_draw_cmd`s through a single
`title_render_sink_hook` (no-op by default) — the render half's *purpose* is
exactly that ordered draw stream, so the sink is its testable core: the
dispatch decision, per-handler draw sequence, fade→alpha ramp, sparkle-trail
geometry, and selected-row cursor placement are all asserted in it without
pulling in any black-box draw subsystem.

**Findings — new quirk #40:** `0x448c80`'s ramp returns **0 at both ends** —
`fade == 1000` lands on the excluded `idx == 20` (`>= 0x14` cap) and returns 0,
*not* the top entry, so a saturated hold composites by a different path than
the ramp; and its 20-dword table at `0x8a9308` reads **all-zero statically**
(DDraw fills it at run time), so a headless port correctly sees alpha 0
everywhere (modelled as a NULL/empty `ramp` input).  Also: the two intro logos
are **container fields at +4/+8**, not `0x418470` assets; and Ghidra's
"call+return" rendering of the `0x56bb55` jump table hid that the 7 handlers
are inline labels all converging on the one frame-end.

This is an extension of the already-counted `FUN_0056aea0`, with all unported
callees referenced by bare VA, so the ledger is **unchanged: 122/1490 touched
(7.5%), 119 tested**.  **9 host tests** (fade-ramp index/clamp, prologue gating,
logo clear-vs-blit, press-button asset pairs, sparkle trail count/geometry,
menu bg+cursor, fade-out, flip-log-once, no-sink safety); **527 pass / 0 fail /
6 skip (of 533)**, ASan/UBSan clean.  Both 32-bit cross-builds clean.

## 2026-06-01 — Title-menu per-frame input dispatch assembled, ckpt 9

Assembled the `0x56aea0` default-branch **per-frame menu input dispatch**
(`0x56b807..0x56ba39`) into `src/title_scene.c` as **`title_menu_input_step`**
— the last piece of the title scene's *update* half (commit `f8f76bc`).  Like
ckpt 8 this is an *assembly* of already-ported leaves (`input_poll_consume`,
`menu_list_latch`), so the ledger is unchanged (**122/1490 touched, 119
tested**); the value is the wiring + the disasm-resolved findings.

The step polls the five menu buttons + two interleaved axis-held syntheses,
feeds each into the latch, then runs the action switch (move / confirm /
denied / cancel SFX), the enabled-row commit (joystick lazy-attach → save-data
table walk + notify → phase-10 + result latch), and the idle watchdog.  The
four unported side effects — SFX `0x411390`, joystick `0x5ba120/_290`, notify
`0x41bb80`, watchdog `0x40a5d0` — route through no-op-by-default hooks (the
`menu_cell_layout_hook` pattern).  The save-data lookup itself is ported
faithfully against a caller-supplied model of the god object's table slice.
The `+0x114/+0x118` axis-held flags were added to `input_mgr` (with 32-bit
offset asserts) — they live in the input manager past the poll ring.

**Findings (the decompile hid both, so resolved against the raw r2 disasm) —
new quirk #39:** the action `switch` keys on the *latch return code*, not the
button, and the engine's cancel-returns-3 / confirm-returns-4 convention
inverts the intuitive reading: the physical **commit button is `0x24`**
(latch dir 9 → nav returns 3 → `case 3`), `case 3` gates on the selected
*row*'s `flag8` (enabled) — **not** on `action == 0x1d` as an earlier summary
claimed (that's a separate, later save-data guard) — and `case 4` (cancel
SFX 7) is **dead** in the title flow (needs latch dir 10, never sent).  Also:
the page dirs (`2`/`3`) are no-ops on the single-page menu (`stride 6 ≥ count
5`), so only up/left navigate.  Corrected the stale "back/cancel" / "0x1d →
SFX 6" notes in `title-scene.md`.

**9 host tests** (nav SFX, page-button no-op, commit enabled/disabled,
save-data match + 0x1d skip, idle frame, axis-held synth, watchdog idle
threshold); **518 pass / 0 fail / 6 skip (of 524)**, ASan/UBSan clean.  Both
32-bit cross-builds clean.  Remaining for the whole title scene: only the
**render half** (`0x56bb04`, jump-table draws + Flip) — the milestone-0 update
half is now complete.

## 2026-05-29 — Title-menu spawn block assembled, ckpt 8

Assembled the `0x56aea0` default-branch **spawn block** (`0x56b5cd..0x56b807`)
into `src/title_scene.c` as **`title_menu_spawn`** (+ `title_menu_teardown`
for the phase-10 path) — the last piece of the title scene's *update* half.
No new function is ported here (it composes already-ported leaves), so the
ledger is unchanged; the value is the assembly + the structural finding.  The
block: configure the owner `sel_list`'s next entry as the menu tree node with
one child (`menu_node_build`), bump + `sel_list_mark_last`, acquire that lone
child as the controller, `menu_ctrl_build` a 6×1 stride-6 grid, append the
five fixed rows `0x1a,0x1c,0x1e,0x1d,8` (each `menu_row_finalize`d — a no-op
on fresh NULL cells), then seek the cursor to the row matching the saved
selection key and `menu_list_scroll_into_view`.

**Headline finding (new quirk #38):** the 0x1b0 menu node wears **four**
overlaid identities — container header, embedded `menu_ctrl`, `sel_entry`,
*and* `obj_pool`.  The controller (`local_60`) is the node's lone **child**,
handed out by reinterpreting the node as a pool: retail does
`obj_pool_acquire(node)` (ECX = node), confirmed in the disasm
(`0x56b623 call 0x412c10` with the node still in ECX from
`[owner->entries + count*4]`).  The acquire stamps the child's `+0x00` —
which is the controller's `menu_ctrl.sub` — with the node pointer, **wiring
the controller's input-ready gate to the node** (the latch reads `node+0x54`
ready / `node+0x04` enabled).  The node's child-array/count/capacity at
`+0x48`/`+0x4e`/`+0x4c` alias `obj_pool`, and `node+0x08` aliases
`sel_entry.selected` — but **only on the 32-bit target**; the node's 8-byte
`owner` pointer shifts those fields on the 64-bit host.  So the port applies
`obj_pool_acquire`'s semantics to the node's own `menu_node` fields (identical
to the cast on win32) and the test checks selection via the `sel_entry` view.

5 new host tests (full five-row build, cursor-seek-to-match, no-match keeps
cursor 0, teardown clears the node's `+0x50` flag, teardown-noop-when-unset;
ASan/LSan clean).  **509 pass / 0 fail / 6 skip (of 515)**; both 32-bit
cross-builds clean (all `menu_node`/`sel_list`/`pool_slot` offset asserts
hold).  Ledger **122/1490 touched (7.5%), 119 tested** (unchanged — assembly,
not a new port).  See `findings/title-scene.md`, `findings/menu-list.md`
"The spawn block", and quirk #38.

---

## 2026-05-29 — Menu-node builder + the menu-tree structure, ckpt 7

Ported `FUN_0040f3e0` (434 B) into `menu_list` as **`menu_node_build`** — the
0x1b0 menu-item / page builder.  It (re)configures one menu node from its
params and (re)builds the node's child-node array, freeing any stale children
via `menu_ctrl_clear` first.  This is the last sub-function the title-menu
spawn block needed; only the cheap inline row appends remain to finish the
update half.

**Headline finding (new quirk #37):** the engine's menus are a **tree of
uniform 0x1b0-byte nodes**, and Ghidra mis-typed this builder's `__thiscall`.
The decompiled call `FUN_0040f3e0(piVar11,0,0,100,100,1,0)` reads as "operates
on `piVar11`", but the disasm (`0x40f3ec mov ebx,ecx`; call site `0x56b606
mov ecx,[owner->entries + count*4]`) shows the ECX `this` is the **node** being
configured and `piVar11` is `param_1` (the owning `sel_list`) — Ghidra dropped
the ECX node and rendered it as the first arg.  So the earlier "page-container"
reading in `findings/menu-list.md` / HANDOFF was off by one, now corrected.

Each node overlays two views on one buffer: a **container header**
(`+0x00..+0x84`, child-pointer array at `+0x48`, u16 count at `+0x4c`) and an
**embedded `menu_ctrl`** at `+0x00` (so `+0x164..+0x17c` are
`field_164/list2/list/entries/rows`) followed by `0x30 B` of **display config**
at `+0x180..+0x1ac` (text/shadow colours `0x3e537d`/`0xa8b9cc`/`0xf08080` +
label VAs `&DAT_00677b98`/`&DAT_008090a9`).  That dual identity is why the
builder tears a stale child down with `menu_ctrl_clear`.  Modelled `menu_node`
(0x1b0) in `menu_list.h`, pinned by guarded 32-bit `_Static_assert`s.

5 new host tests (title call, config-blob copy, per-child display config,
rebuild-frees-old-children under LSan, zero-children).  **504 pass / 0 fail /
6 skip (of 510)**; both cross-builds clean (the new 0x1b0/offset asserts and
the `sizeof(menu_node) >= sizeof(menu_ctrl)` cast-safety assert all hold).
Ledger **122/1490 touched (7.5%), 119 tested**.  See commit `a78073a`,
`findings/menu-list.md`, and quirk #37.

---

## 2026-05-29 — Menu grid-cell finalizer + dead-alloc quirk, ckpt 6

Ported the menu spawn block's **grid-cell finalizer** into `menu_list` as
`menu_row_finalize` (`FUN_00411f40`, 444 B).  `__thiscall(ctrl, row)`: it
walks the row's cell array (bounded by `hdr->alloc_b`) and, per cell,
refreshes whichever lazily-built sub-objects are present — `obj0` (re-lays
its glyph text via `0x40fa00`), `obj54` and `obj20` (re-zeroes their
modelled fields when `row < hdr->count`; `obj20` also recomputes
`+0x1c = max(+0x14, min(+0x18, 0))`).

**The headline finding (new quirk #36):** the decompile reads as a classic
lazy get-or-create, but disassembling `0x411f40` shows the per-sub-object
`if (ptr == 0) operator_new(...)` sits **inside** an outer `ptr != 0`
guard reading the same slot with no intervening write (verified at
`0x411fbf` / `0x412046`).  So the allocation is **statically dead** — the
finalizer never allocates, it only re-zeroes sub-objects built elsewhere.
The earlier `findings/menu-list.md` note that it "lazily operator_new's"
the sub-objects was **wrong** and is corrected.  On the fresh title menu
every cell pointer is NULL, so the whole function is a no-op there.

`0x40fa00` (the cell's 800-B SJIS/colour-escape/font-metric text-layout
builder) is its own subsystem and stays unported; the finalizer's call to
it routes through an observable hook (`menu_cell_layout_hook`) so the
dispatch is testable without pulling in the text layer.  Its string arg
`&DAT_008a9b6c` is the god object's engine-name buffer (god+0x1c).

Modelled `menu_cell_obj54` (0x54 B) / `menu_cell_obj20` (0x20 B) with
guarded `_Static_assert`s; both cross-builds clean.  **6 new host tests**
(fresh no-op, obj54 re-zero, obj20 re-zero+clamp, row-outruns-count guard,
obj0 layout-hook dispatch, all-cells iteration; ASan/LSan clean); **499
pass / 0 fail / 6 skip (of 505)**.  Ledger **121/1490 touched (7.4%), 118
tested** — unchanged, because `0x411f40` had been provisionally counted via
a ckpt-5 header comment using the `FUN_` token; this port legitimises it.
What remains of the update half: the menu-item builder `0x40f3e0` + the
spawn-block assembly.  See `findings/menu-list.md`, commit `1ba5827`.

---

## 2026-05-29 — Menu-controller geometry ctor+dtor, ckpt 5

Ported the menu-spawn block's **allocate/free pair** into `menu_list` as
`menu_ctrl_build` (`FUN_0040f5c0`, 563 B) and `menu_ctrl_clear`
(`FUN_0040e0c0`, 555 B) — one commit, since build calls clear (slots are
recycled from an object pool, not zeroed; the ctor tears down stale state
first — new quirk **#35**).

`menu_ctrl_build` allocates the `0x24` list header plus the controller's
**two parallel geometry arrays**: the row array (`alloc_a` × `menu_row`
0x10, at `+0x17c`) with a per-row cell array (`alloc_b` × `menu_cell`
0x18), and the per-column metadata array (`alloc_b` × `menu_entry` 0x24,
at `+0x178`) stamped `pos=index*0x20` / `extent=0x20`.  `menu_ctrl_clear`
frees it all in retail order (confirm graph → `+0x164` → entries → each
row's cells + their three lazy sub-objects → row array → header **last**,
since its dims size the free loops).

Modelled the controller's extended layout (`menu_row`/`menu_cell`/
`menu_entry` + the ctor-touched scalars `field_c/_10/_20/_84/_140/_164`)
and extended `confirm_src`/`confirm_caprec` with the owned-pointer slots
the teardown frees.  Every new offset pinned by guarded `_Static_assert`;
both cross-builds clean.  `operator_new → calloc` (zero-init divergence
documented: retail leaves `row.flag8` + each cell's trailing 8 bytes
indeterminate; the spawn block always writes `flag8` before reading).

**9 new host tests** (build header/params/grid/entries, fresh-clear no-op,
recycle-rebuild, and the confirm-graph + cell-subobject + `+0x164`
teardowns — ASan/LSan verify no leak/double-free/UAF); **493 pass / 0 fail
/ 6 skip (of 499)**.  Ledger **121/1490 touched (7.4%), 118 tested**.  The
menu controller's geometry is now grounded; what remains of the spawn
block is the cheap inline row-populate and the two lazy cell finalizers
(`0x40f3e0`, `0x411f40`).  See `findings/menu-list.md`, commit `a380457`.

## 2026-05-29 — Menu input→action chain, ckpt 4 (scroll + nav + latch)

Ported the entire **menu input → action chain** the title menu's update
half depends on, into a new **`src/menu_list.{c,h}`** (three logical
commits 4a/4b/4c, port-and-test):

- **4a `menu_list_scroll_into_view`** (`FUN_004192b0`, 52 B) — recompute
  the page-top `sel2 = floor(cursor/stride)*stride`, return 1 if it moved.
  Factored the step-search into `page_top()` (the nav engine reuses it).
- **4b `menu_list_nav`** (`FUN_0043ca40`, 970 B) — the cursor-navigation
  engine. Its inner jump table at `0x43ce1c` was **Ghidra-unrecovered**
  (duplicate targets); recovered with `radare2 -c 'pxw 44 @ 0x43ce1c'` —
  dir 0..10 → 7 handlers (prev/next/page-up/page-down/no-op/cancel/
  confirm). Ported branch-for-branch including the three list-type scroll
  models (0 linear-wrap / 2 grid / 3 trailing-page, whose shared fields
  change meaning per type) and the two-rate auto-repeat (300 ms initial →
  100 ms steady). The internal `GetTickCount` is injected as `now`.
- **4c `menu_list_latch`** (`FUN_0043ce50`, 220 B) — the input gate:
  refuses unless `sub->ready==1000 && sub->enabled!=0`, then dispatches
  mode 1 → nav, mode 2 → confirm/message box (two-press reveal-then-
  dismiss). Modelled the `sub` ready-gate and the `confirm_list`
  `src→caprec→cap` u16 chain.

Together with the ckpt-3 poll this closes `input_poll_consume →
menu_list_latch → menu_list_nav`. Verified the common tail + cancel/
confirm + latch offsets against the r2 disasm at `0x43ccf7` / `0x43cae9`
/ `0x43ce50`. **43 new host tests** (6 scroll + 26 nav + 11 latch), all
hand-derived; **484 pass / 0 fail / 6 skip**. Both cross-builds clean.
New quirks **#32** (jump table + per-type field reuse), **#33** (two-rate
auto-repeat), **#34** (1000-ready gate + two-press confirm). Ledger
**118/1490 touched (7.2%)**. See `findings/menu-list.md`.

Next: the menu-spawn block (port leaves `0x40f3e0`/`0x411f40`/`0x40f5c0`,
then assemble the 5-row populate) to finish the update half, then the
render half (`0x56bb04`). The input-ring **producer** (DInput
`GetDeviceState`) is now the only black box left in the input subsystem.

---

## 2026-05-29 — Title-menu update-half leaves (ckpt 3): input poll + container primitives

Knocked out the pure, zero-dependency leaves the title-menu update half
(`FUN_0056aea0` default branch) depends on, per the HANDOFF "next move" —
port-and-test rhythm to shrink the surface before assembling the menu.

**Input ring poll `FUN_0043c110`** → new `src/input.{c,h}`,
`input_poll_consume` (opens milestone 1).  The read side of the input
manager's 64-entry event ring at `+0x108`: scan newest-first, match
`id + flag==1 + age<=100 ms` (unsigned, rollover-safe), and on a hit zero
the record id (consume-on-read).  10 host tests; see `findings/input.md`
and engine-quirks #30.  This is the consumer end of the ring `mem_watch.py`
is meant to find live (producer still black-box).

**Container leaves `FUN_00412c10` + `FUN_00414080`** → new
`src/obj_container.{c,h}`.  `obj_pool_acquire` (check out the next free
slot from a fixed-capacity pool, stamp owner/index/+8, NULL when full —
note the index is a 16-bit store into a dword, quirk #31) and
`sel_list_mark_last` (single-selection: mark the last list entry, clear the
rest).  Both ~10× across the engine; in the menu-spawn block they run back
to back (append → mark-last → acquire controller).  8 host tests; the
`sel_list` `+4`/`+6` layout is cross-validated by the title-scene caller.

Deferred (still object-model-coupled / unported deps): the action latch
`FUN_0043ce50` + the 970 B cursor-nav engine `FUN_0043ca40` (jump table
unrecovered), and the menu-spawn assembly itself (needs `0x40f3e0`/
`0x40f5c0`/`0x411f40`/`0x4192b0`).  The sound-effect player at `0x411390`
is audio-subsystem-coupled (milestone 3), not the "action switch" the old
HANDOFF guessed.

Two commits.  **441 host tests pass / 0 fail / 6 skip** (of 447), both
cross-build exes clean.  Ledger **113→115 touched (7.0%), 110→112 tested**.

## 2026-05-29 — Structural-parity harness (offline foundation)

Detour from milestone 0 to build the call-graph-diff + mem-watch machinery
that `../openrecet` credits for fast rendering-path convergence, adapted to
SoTE's DirectDraw stack. Design: `docs/plans/parity-harness.md`; how-to:
`docs/parity-harness.md`. Built the pieces that need no live retail (the
"offline foundation"); the live retail-under-Frida runs are the next-session
human-verification gate.

Landed **[offline, all tested]**: `src/call_trace.{c,h}` — the port-side
`CALL_TRACE_ENTER(0xVA)` probe (one null-check when off), wired into
`main.c` (`--call-trace <path>` + `--call-trace-frames`, begin/end frame
brackets, boot-frame-0 bracket) and probed into `zdd_create`/`zdd_present`/
`zdd_window_paint`/`cs_dispatch_create_screen`; 5 new host tests (**423
pass / 0 fail / 6 skip** of 429, both exes clean). `tools/call_trace_diff.py`
— per-frame overlap / retail-only (= port gap) / port-only (= divergence)
diff with `--align-on-first 0xVA` load-skew anchoring; 9 pytest cases.
`tools/gen_engine_vas.py` — `functions.csv` → 1743-VA candidate hook list.
`tools/mem_watch.py` ranking (offline `--analyze-only` verified: a faulting
insn maps to its owning function + port status via the ledger).

Code-complete, **live verification deferred**: `opensummoners-agent.js`
call-trace + mem-watch modes, anchored on a hook of the DDraw Flip
(`FUN_005b8fc0`) for the per-frame boundary — the DirectDraw analog of
openrecet's D3D-Present anchor (frame axis matches the port's per-`zdd_present`
`g_frame_counter`). `frida_capture.py` driver fields + CLI. `mem_watch.py`
capture + `bisect_call_trace_vas.py` (boots retail via `run-retail.sh`,
bisects out Frida-unsafe VAs → `engine_vas_frida_safe.json`; its
`--boot-threshold` needs calibration on the first live boot). First real use
will target the `+0x108` input-ring writer — resolving a standing HANDOFF
black box. Ledger headline unchanged (112/1490); the probes sit in
already-ported files.

---

## 2026-05-29 — Title scene runner, checkpoint 2: frame-pacing FSM

Second code chip of milestone 0.  Ported the `local_28` frame-pacing sub-state
machine + the `FUN_005b1030` pump call sites at the top of `FUN_0056aea0`'s outer
loop, as `title_pace_*` in `src/title_scene.{c,h}` (`title_pace_step`).  It's a
pure fixed-16 ms-timestep accumulator: each iteration it runs the *update* half
(input + the ckpt-1 `local_64` phase FSM) burning the wall-clock budget in 16 ms
slices, or the *render* half (jump-table draw + Flip), refilling the budget from
the real `GetTickCount` delta (clamped 100 ms) and pumping on the way into update.
`now` is passed in (app_pump style); the pump request and update/render decision
are reported via `title_pace_step_out`, so the unit stays Win32-free and
link-dependency-free.  12 new host tests, all green (**418 pass / 0 fail / 6
skip** of 424; both cross-build exes clean).

Decoded byte-for-byte from r2 disasm `0x56b002..0x56b0c8` (raw stack offsets).
**Resolved the open ckpt-1 thread:** the `E` counter at `[esp+0x5c]` (which r2
showed but Ghidra dropped) is a **dead** consecutive-sub-second-frame tally — a
full-function disassembly scan finds it written-only/never-read, and Ghidra
dead-store-eliminated it.  Its window anchor `D = local_20` is read only to gate
that dead update, so the *entire* `S==1` post-arm is observably inert and is
omitted — behaviourally exact.  Only the `S==2` arm (`A = now`) is load-bearing.
The pacer also explains the `--turbo` "splash doesn't animate" symptom: with a
frozen clock the budget never refills past one slice, so after the first update
the loop renders every frame with the phase FSM frozen.  See
`findings/title-scene.md` "Frame-pacing sub-state machine" and engine-quirk #29.
Ledger headline unchanged (112/1490) — `FUN_0056aea0` was already "touched";
progress shows as new provenance refs.

---

## 2026-05-29 — Title scene runner, checkpoint 1: intro-phase/menu-fade FSM

First code chip of milestone 0 (title screen renders).  Ported the pure
arithmetic core of `FUN_0056aea0` (the 3441-byte title scene runner): the
`switch(local_64)` intro-phase / menu-fade state machine, as
`src/title_scene.{c,h}` (`title_fade_step`).  19 host tests, all green
(406 pass / 0 fail / 6 skip total; cross-build clean).

Verified the control flow against raw radare2 disasm before porting — the
`PTR_DAT_0056bfa4` 11-entry phase jump table (`0x56bb5c..0x56be85`, 7 distinct
handlers) and the `switch` table at `0x56bf68` both match `findings/title-scene.md`,
and every fade constant/threshold in cases 0..10 was confirmed against
`0x56b153..0x56b5c1`.  The FSM is kept **side-effect-free**: the two outward
signals (the studio→title BGM "SetNextSegment" cue, and the phase-7 sparkle
spawn at intensity `(fade*0xe0)/900+0xc0`) are reported through a per-frame
`title_fade_step_out` descriptor instead of calling the still-unported engine
helpers — so `title_scene.c` has zero link dependencies and drops straight into
both the host suite and the mingw `.exe`.  New quirk **#28** (single reused
0..1000 fade ramp across 8 phases + the menu "breathing" oscillator).

**Deferred to later checkpoints** (seams documented in `title_scene.h`): the
`local_28` frame-pacing FSM + `FUN_005b1030` pump call sites; the phase-8/9
menu-controller spawn (`0x412c10`) + 5-slot populate + input poll/latch
(`0x43c110`/`0x43ce50`) + action switch (`0x411390`); the render-half jump-table
draw handlers + frame-end flip (`0x56c180` + `FUN_005b8fc0`); the `param_1`
skip-intro + ring-buffer "press to skip splash" early-out; joystick lazy-attach
(`0x5ba120`); the `local_50` watchdog.  Ledger headline unchanged (112 touched —
`FUN_0056aea0` was already counted; the ledger is binary, so partial progress
isn't reflected in the count, only in the new provenance refs).

## 2026-05-29 — Project-wide audit + workflow tooling (no code port)

Brought OpenSummoners up to the sibling-project (OpenMare / OpenLords2)
workflow standard and mapped the entire binary so future sessions don't
re-analyze the same code.  No engine functions ported this checkpoint —
this is infrastructure + intelligence.

**Derived progress tooling** (adapted from OpenMare):
`tools/gen_port_ledger.py` + `tools/gen_frontier.py`.  Engine-proper
boundary pinned to `0x5bdab0` (last engine fn `FUN_005bd680` before the
MSVC CRT tail — import thunks, `operator_new`, `_malloc`, the `entry`
startup, etc.).  Generates `STATUS.md`, `port-ledger.{md,json}`,
`port-frontier.md` from `FUN_<va>` provenance comments in `src/` +
`functions.csv` — idempotent, `--check`-able for a pre-commit hook.
Baseline: **112/1490 engine-proper touched (6.8%), 9.5% of code bytes**,
109 host-tested, 52 portable-today frontier leaves.

**Subsystem-survey workflow** (`tools/workflows/subsystem-survey.js`):
22 read-only `Explore` agents (16 mapping address bands + 6 scouting the
forward path), ~1.9 M tokens.  Output seeded `docs/ROADMAP.md` (11-milestone
order + full subsystem map of every band + 5 port-readiness cards) and
surfaced **136 quirks** — the load-bearing/charming subset went into
`findings/engine-quirks.md` #15–#27 (god-object `DAT_008a9b50`, the
universal frame-pump `return 6` quit convention, the hash-id asset
directory *with recovered character names* — Arche/Sana/Sophia, the LCG
RNG, struct strides 0x294/0x300, WMA-temp-file BGM, lazy gamepad attach,
the "object perpetuity state area" overflow log).  Raw structured result
archived at `docs/audit/subsystem-survey-2026-05-29.json` (mine it; don't
re-run).  This is the sanctioned read-only-Explore carve-out to the
no-subagents rule — recorded in PLAN.md §7 + AGENT-WORKFLOW.md.

**Docs reorg:** new `STATUS.md`, `ROADMAP.md`, `port-ledger.{md,json}`,
`port-frontier.md`, `findings/INDEX.md`.  Updated `AGENT-WORKFLOW.md`
(session-lifecycle checklist; co-author trailer → Opus 4.8), `PLAN.md` §7,
`memories/HANDOFF.md`.  The forward target is unchanged: ROADMAP milestone
0, the title scene runner `FUN_0056aea0`.

---

## 2026-05-25 — Main pump / frame waiter port (FUN_005b1030)

Ported the 156-byte message-pump / frame-waiter
(`FUN_005b1030` → `app_pump_frame`) as a new `app_pump` module.  The
function is the inner gate the scene runner calls twice per loop
iteration; with this in place, porting the title-menu scene runner
no longer has a "missing prerequisite" gap.

Field-named the `app_ctx` struct (was `wp_app_ctx`, opaque pad after
`+0x08`).  The pump touches three previously-unnamed slots:
- `+0x0c limiter_enable` — master frame-limiter on/off.
- `+0x10 last_tick_ms` — `GetTickCount` sample from the previous
  pump exit.
- `+0x1c pump_throttle` — set by the limiter when re-arming, cleared
  by `WM_TIMER` (0x113) — the periodic tick paces the pump.  This is
  the same slot the WndProc already named `timer`; renamed to make
  the limiter coupling explicit.

The throttle re-arm condition pinned to UNSIGNED `prev - now < 5`
(disasm at 0x5b10b3 uses `jae`).  Equivalent to "GetTickCount hasn't
ticked since the previous sample" — a sub-tick spin guard that holds
the throttle until `WM_TIMER` clears it.  16 host tests cover the
limiter boundaries (==4 sets / ==5 skips), first-frame path, master-
disable, WM_QUIT short-circuit, drain-then-exit, and NULL ctx defense.

Refactoring done as part of the port: `wp_app_ctx` → `app_ctx`,
`g_wp_app_ctx` → `g_app_ctx`, `g_wp_active_flag` → `g_app_active_flag`.
The struct moved from `wnd_proc.h` to a new `src/app_pump.h` since
the pump is now the canonical owner; WndProc just `#include`s it.
`wp_state_init` no longer touches the shared globals — tests call
`app_pump_state_init` alongside.

Tests: 372 → 387 pass (+15 net; -1 layout test moved from wp_ to
app_).  Live boot still zero DDERR through 10 frames mode-2.

The pump is NOT yet wired into `main.c`'s per-frame loop — the
drop-in keeps its own minimal `main_loop_body` until the scene
runner ports and calls `app_pump_frame` retail-style.

---

## 2026-05-25 — Surface-paint leaves chip session (14 ports)

Marathon chip session.  Four checkpoints landed all the remaining
"small leaf" ZDD ports the title-menu scene runner needs.  +43 tests
(329 → 372), every commit cross-builds with mingw clean, live boot
still zero DDERR.

**Commit 55708b3** — Surface Lock/Unlock + clear + color descriptor +
keyed blit:
- `zdd_object_lock` (FUN_005b9490, vtable[25]).
- `zdd_object_unlock` (FUN_005b94d0, vtable[32]).
- `zdd_object_clear` (FUN_005b9410) — Lock + zero-fill + Unlock.
  Uses a new `zdd_object_get_locked_info` Win32 primitive to bridge
  the 4-byte-vs-8-byte pointer mismatch between retail and host.
- `zdd_object_blt_keyed` (FUN_005b9b70) — variant of `blt_onto` with
  positioned dest origin + DDBLT_KEYSRC.
- `zdd_bind_pixel_format` (FUN_005b8a20) — GetSurfaceDesc + mask
  extract.  New `zdd_color_descriptor` struct (22 bytes, replaces the
  unobserved `_pad000[0x18]` at zdd+0x00).
- `zdd_color_convert` (FUN_005b8b00) — RGB888 → surface-native pack.
- GetSurfaceDesc / Lock / Unlock Win32 primitives.

**Commit ed82ce6** — Mode-4 upscaler + create-screen wiring:
- `zdd_object_upscale_16bpp` (FUN_005b8ea0, 285B) — 2x software
  scaler.  Faithful to retail's hardcoded outer-stride caveat.
- Wired upscaler into `zdd_present` mode 4 (Zoom).
- Wired `zdd_bind_pixel_format` into `zdd_create_screen`'s 16bpp
  branch.

**Commit 4d6c590** — Wire color_convert into 16bpp set_color_key.
The TODO in `zdd_object_set_color_key`'s 16bpp branch is now an
actual `zdd_color_convert` call — magenta 0x00FF00FF correctly packs
into RGB565 0xF81F.

**Commit 4db9980** — Lost-surface recovery (FUN_005b91d0/_9240/_9ab0/
_9ac0).  Four functions: `zdd_object_is_lost`, `_restore_surface`,
`zdd_check_any_surface_lost`, `zdd_restore_all_surfaces`.  Backed by
two new Win32 primitives (IsLost vtable[24] / Restore vtable[27]).
Needed by the post-activate hook (FUN_005b14c0, unported) and any
later code that handles DDERR_SURFACELOST.

All ZDD leaves the title-menu scene runner needs (`FUN_0056aea0`)
are now ported.  Next checkpoint should start porting the runner
itself.

---

## 2026-05-25 — WM_PAINT handler ported + wired

Ported `FUN_005b9130` — the 151-byte WM_PAINT consumption handler — as
the new `zdd_window_paint(zdd *self, void *hwnd)` in `src/zdd.c`.
Sequence is retail-faithful: `if (mode != 2) return 0;
BeginPaint(hwnd) → zdd_object_get_dc(primary_obj) → BitBlt(window_hdc,
screen_pos_x, screen_pos_y, screen_width, screen_height, src_hdc, 0,
0, SRCCOPY) → zdd_object_release_dc(primary_obj) → EndPaint`.  Returns
1 iff consumed (caller's WndProc must then return 0 to the OS, since
we own the dirty region's validation via EndPaint).

Three new Win32 primitives in `zdd_win32.c`: `zdd_window_paint_begin`
(BeginPaint with a heap-allocated PAINTSTRUCT so its lifetime spans
the separate begin/end calls — the pure-logic body can't hold a
stack-frame PAINTSTRUCT the way retail does), `zdd_window_paint_end`
(EndPaint + free the cookie), and `zdd_window_blit_copy` (GDI BitBlt
with SRCCOPY on caller-supplied HDCs).  The blit primitive is distinct
from `zdd_desktop_present` — the latter wraps `GetDC(NULL)` +
`ReleaseDC(NULL)` internally, while this one trusts the caller to
hand it both HDCs.

Wired into `main.c`'s minimal `wndproc` via a new WM_PAINT case that
calls `zdd_window_paint(g_zdd, hwnd)`; if it returns 1 the message is
consumed, otherwise the case falls through to `DefWindowProcA` which
validates the update region itself.  Wired directly here (not via the
ported `wp_handle_message`) because the WndProc module is still in
isolation — `wnd_proc_win32.c`'s `wp_paint_check` remains a no-op
stub pending the input-subsystem ports needed to wire `wp_handle_message`
into main.

A retail-faithful quirk fell out of porting: the BitBlt uses
`screen_pos_x/y` (the same +0x138/+0x13c fields the per-frame present
dispatcher reads) as destination coordinates, but BeginPaint returns
an HDC whose origin is at the window's CLIENT-area top-left in CLIENT
coordinates.  For a window at e.g. screen (200, 300), the WM_PAINT
BitBlt destination becomes (200, 300) within the client area — off-
screen.  Retail does this too.  In practice the per-frame
`zdd_present` mode 2 fires every frame and re-paints via `GetDC(NULL)`
at the correct screen coords, so this misalignment is overwritten
before any frame fully composites.  See `zdd.h` `zdd_window_paint`
docstring for the full reasoning.

5 new host tests added (110 total in test_zdd.c): NULL-self guard,
non-mode-2 short-circuit (no primitives fire), NULL-primary_obj
defensive guard, mode-2 full sequence (verifies the exact retail
ordering — begin(seq=1) → get_dc(2) → blit(3) → release_dc(4) →
end(5) — plus HDC/cookie threading and coord passing), and a "doesn't
touch present-dispatcher primitives" anti-regression.  329 host tests
pass / 0 fail / 6 skip (was 324/0/6).  Cross-build with mingw clean;
`tools/run-opensummoners.sh --frames 10 --hide-window` still boots
through zero DDERR (hidden window doesn't see WM_PAINT damage; the
visible-window case will exercise the new handler on uncover).

---

## 2026-05-25 — Present dispatcher ported, smoke loop replaced

Ported `FUN_005b8fc0` — the 5-mode per-frame present dispatcher — and
its three leaves (`FUN_005b94e0` GetDC, `FUN_005b9500` ReleaseDC,
`FUN_005b9a40` blit_onto) as the new `zdd_present` /
`zdd_object_get_dc` / `zdd_object_release_dc` / `zdd_object_blt_onto`
functions in `src/zdd.c`.  Added three new Win32 primitives
(`zdd_surface_flip`, `zdd_surface_blt`, `zdd_desktop_present`) in
`zdd_win32.c`.  The drop-in's hand-rolled smoke-present loop
(`present_smoke_frame` in main.c) is removed; main.c now calls
`zdd_present(g_zdd)` directly each frame.

Two non-trivial discoveries fell out:

1. **`paint_ctx::FUN_005b94e0` / `_9500` are misnamed by Ghidra** — the
   ECX in every live callsite is a `zdd_object*` (specifically
   `zdd.primary_obj`), not a separate "paint_ctx" class.  Verified by
   r2 disasm at `0x5b9158` (`mov ecx, [esi + 0x16c]` in the WM_PAINT
   handler `FUN_005b9130`) and `0x5b90a1` (case 2 of `FUN_005b8fc0`).
   The `paint_ctx` typedef in `src/wnd_proc.h` is a misnomer for `zdd`
   itself — same `+0x16c primary_obj` / `+0x138..+0x144` rect /
   `+0x164 pixel_format_mode` offsets; its `+0x2c zdd_device`
   docstring is incorrect (that offset falls inside `zdd.log_buf` and
   isn't read by anything in the WM_PAINT path).

2. **`FUN_005b9a40` arg order**: `(this=src, dest_obj, dest_x, dest_y)`.
   The `this` parameter is the SOURCE surface; the receiver of the
   Blt vtable call is `dest_obj->com_primary`.  Confirmed by tracing
   case 4's push order at `0x5b900b..0x5b9010` against the function
   body's slot usage at `0x5b9a93` (the COM-dereferenced arg slot is
   the middle stack arg, not the first).  Also discovered: r2 names
   stack args by physical-offset (`arg_1ch` ≠ "first C arg") rather
   than by C-arg-index — caused a brief detour.

Mode 2's desktop-DC technique was also a discovery: case 2 uses
`GetDC(NULL)` (the *desktop* DC, set explicitly at function start via
`mov dword [hWnd], 0`) and `BitBlt` at `(screen_pos_x, screen_pos_y)`,
NOT `GetDC(hWnd)`.  This means `screen_pos_x/y` must be the window's
CLIENT-area top-left in *screen* coordinates for the BitBlt to land
inside the window.  Drop-in now keeps these in sync via a new
`sync_window_position()` helper, called once after `init_ddraw` (with
`ClientToScreen(g_hwnd, &pt)`) and again on every `WM_MOVE`.  Retail
must do this from somewhere we haven't traced — `FUN_005b9130` or a
WM_MOVE handler in the WndProc subsystem; deferred for now.

Mode 4 (Zoom) is partially ported: the dispatcher correctly fans
into `zdd_object_blt_onto` + Flip, but the upstream software
upscaler `FUN_005b8ea0` (285-byte 16bpp pixel-copy via Lock/Unlock)
is NOT ported.  Mode 4 in this port skips the upscale stage —
`back_obj_b` stays blank, so the Flip presents `back_obj_a`'s last
contents.  Mode 4 isn't the live boot mode; this only bites on a
Zoom-launcher selection.

`--no-smoke-present` flag renamed to `--no-present`.  Smoke-failure
counters / log lines in main.c removed (we're done with the
hand-rolled diagnostic; the dispatcher's own DDERR logging via
`zdd_log_dderr` → `OutputDebugStringA` → stderr is the new
diagnostic surface).

17 new host tests added (105 total in test_zdd.c): 4 for
`zdd_object_get_dc`, 2 for `zdd_object_release_dc`, 3 for
`zdd_object_blt_onto`, 8 for `zdd_present`'s mode dispatch.  324 host
tests pass / 0 fail / 6 skip (was 307/0/6).  Cross-build with mingw
clean; `tools/run-opensummoners.sh --frames 10 --hide-window` boots
through the dispatcher with zero DDERR.

---

## 2026-05-25 — Mode-2 smoke-present + two RE bug fixes it surfaced

Drop-in `main.c` now runs a per-frame smoke-present in launcher mode 2
(Windowed): `BltColorFill(0xF800)` on the offscreen primary, then
`GetDC` → `BitBlt` → `ReleaseDC` onto the window HDC.  Mirrors the
windowed branch of retail's `FUN_005b8fc0` (the engine's "Title Menu -
Flipping" path) but without porting `paint_ctx` yet.  Counts per-step
failures; the harness sees zero DDERR across all frames.  Gated behind
`--no-smoke-present` so the flag can be flipped off when the real
title-scene runner lands.

The first run of the smoke loop failed every frame with DDERR_NOCLIPLIST
(0x887600CD) — which surfaced two real RE bugs in the prior port:

1. **`zdd_object_create_surface_pair` was passing the wrong args to
   `zdd_object_stamp_metrics`.**  Retail's `FUN_005b95c0` calls
   `FUN_005b98c0` with `(p1, p2, p6, p7, p8/width, p9/height)` — NOT
   `(p1..p6)`.  Confirmed by r2 disasm at `0x5b95ff–0x5b9617` (push
   order on the stack before the call).  The wrong mapping left
   `metric_b8`/`metric_bc` holding the count flag (`1, 0`) instead of
   the surface dimensions (`640, 480`).

2. **`zdd_object_attach_clipper` passed a NULL pointer where retail
   builds a real `RGNDATA` on the stack.**  `FUN_005b9520` builds
   `{RGNDATAHEADER, RECT}` bounding the full surface from
   `self->metric_b8/metric_bc` and hands the struct address to
   `IDirectDrawClipper::SetClipList` (vtable[7] / byte 0x1c —
   confirmed by r2 at `0x5b95a7`, resolving the prior open
   ambiguity).  Renamed the Win32 primitive
   `zdd_clipper_set_clip_list_null` → `_rect` and threaded the
   dimensions from the now-correct metric slots.  Under
   `DDSCL_NORMAL`, an empty cliplist makes every subsequent `Blt` fail
   `NOCLIPLIST`; the fix unblocks all windowed-mode drawing.

DDERR debugging was also helped by dual-sinking
`zdd_output_debug_string` to stderr in `zdd_win32.c` (in addition to
`OutputDebugStringA`).  Without DbgView attached the engine's DDERR
builder output is invisible — the dup makes the harness output
line-oriented and self-contained.

307 host tests pass / 0 fail; mingw cross-build clean; `tools/
run-opensummoners.sh --frames 5` boots and runs the smoke loop with
zero DDraw errors.  See commit `5d82301`.

---

## 2026-05-25 — DDraw init WIRED into drop-in `WinMain` (mode-2 Windowed)

The boot-time graphics init chain is now end-to-end inside the drop-in.
`src/main.c` now calls (after window creation):

1. `zdd_create(&g_zdd)`           — DirectDrawCreateEx (FUN_005b7ee0)
2. `zdd_set_coop_level(g_zdd, hwnd, fullscreen)` — SetCooperativeLevel
   (FUN_005b89d0).  `fullscreen = (launcher_mode != 2)` — only Windowed
   runs DDSCL_NORMAL; the other modes use DDSCL_EXCLUSIVE|FULLSCREEN|
   ALLOWREBOOT.
3. `cs_dispatch_create_screen(g_zdd, launcher_mode, 1280, 960)`
   — the FUN_00582e90 driver that fans into per-mode CreateScreen.
   Zoom-target args (1280×960) mirror retail's `*(int*)(in_ECX +
   0x14/0x18)`.

Cleanup at process exit calls `zdd_destroy(g_zdd)`.  Until the
launcher settings record parser (FUN_005a4770, 45 KB) is ported,
the launcher mode is hardcoded to **2 (Windowed)** — overridable
per-run via `--launcher-mode=N`.  `--skip-ddraw` keeps the prior
"window-only" boot path for harness comparison.

Live boot via `tools/run-opensummoners.sh` runs clean to the success
path:

```
[opensummoners] init_ddraw: launcher_mode=2 (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)
[opensummoners] init_ddraw: SetCooperativeLevel ok (fullscreen=0)
[opensummoners] init_ddraw: CreateScreen dispatch returned (success path)
[opensummoners] OpenSummoners exiting after 120 frames (1920 ms wall)
[launcher] child exited (rc=0)
```

No surface drawing yet — the title menu / scene loop (FUN_0056aea0)
is still unported, so 120 frames of empty main-loop run through with
no visible output.  This is the surface-creation plumbing, not a
renderer.

Tests unchanged at 307/0/6 (no port changes; just wiring).  Cross-build
clean.

Next: port the title-menu scene runner OR a "draw something to the
primary surface" smoke (BltColorFill on com_a) so we can confirm the
surface is actually writable in flight.  The 16bpp pixel-format
binding (FUN_005b8a20) gap may surface here.

---

## 2026-05-25 — `cs_dispatch_create_screen` PORTED (FUN_00582e90, 3560B outer mode dispatcher)

The outer mode-dispatch driver around `zdd_create_screen` lands.
3560 bytes of retail mostly-inlined strcpy/strcat collapse to a
single ~350-line port + a tiny Win32 leg + a setjmp-aware host
harness.  Reaches CreateScreen end-to-end from the launcher mode
arg.

New files:
- `src/cs_dispatch.h` — public driver + module globals
  (`cs_primary_pair`, zoom overrides, log buf, engine-name header).
  Test-only function-pointer hooks for `zdd_create_screen` and
  `zdd_object_new` so the host harness can verify per-mode dispatch
  without configuring the full ZDD surface-create stub chain.
- `src/cs_dispatch.c` — pure-logic dispatch.  Per-mode handlers
  (cs_mode_full / _safe / _windowed / _db / _zoom), error-log
  builder (`cs_log_append_failure` — the inlined strcat dance
  collapsed to one bounded helper), and a `cs_compute_zoom_rect`
  helper for mode 4's centre-rect math.
- `src/cs_dispatch_win32.c` — Win32 primitives
  (`cs_log_get_last_error`, `cs_fatal_log`,
  `cs_fatal_log_with_lasterror`, `cs_exit`) backing the dispatcher
  for the real build via `GetLastError` + `FormatMessageA` +
  `OutputDebugStringA` + `ExitProcess`.

Per-mode behaviour (matches retail):
- **mode 0 (Full)**: SetDisplayMode(640,480,16,0) → hide_cursor →
  busy_wait(2000) → zdd_create_screen(...mode=0, vmem=0) →
  zdd_object_new(&cs_primary_pair, 640, 480, 0x1ffffff, 1).
- **mode 1 (Safe)**: same fullscreen preamble; create_screen with
  mode=1, vmem=1; NO primary-pair alloc.
- **mode 2 (Windowed)**: NO preamble; create_screen with mode=2,
  vmem=1.  Only branch that doesn't touch the display mode.
- **mode 3 (DB)**: fullscreen preamble; create_screen with mode=3,
  vmem=1.  CreateScreen failure uses the FUN_00426110 "with
  lasterror" variant instead of the standard log builder.
- **mode 4 (Zoom)**: resolve desktop dims via
  `cs_zoom_override_*` or `zdd_get_display_mode`; bounds-check
  against launcher zoom target; SetDisplayMode + preamble; compute
  centre-rect blob; create_screen with mode=4, vmem=1, rect.
  Trailing `zdd_hide_cursor` on success (matches retail's literal
  end-of-mode-4 call; idempotent).

Error strings re-verified against `vendor/unpacked/sotes.unpacked.exe`
via `r2 ps @ 0x8a28e8/0x8a28bc/...` at port time.  The "\n" between
log entries (DAT_00854570) is confirmed: it's literally one byte
"\n", not " -- " or " — " as the variable name suggested.

Tests now: **307 pass, 0 fail, 6 skip** (up from 286; 21 new — 4
zoom-rect math, 3 log-builder, 1 prior-pair release, 5 happy-path
per-mode dispatch, 5 fail-path per-mode dispatch, 1 default-mode
no-op, 2 mode-4 GetDisplayMode/exact-match edges).  Cross-build
with mingw clean.

Open RE thread closed: `DAT_008a6ec0` (the primary-pair ZDDObject*)
is now named `cs_primary_pair` and owned by the cs_dispatch module.
`DAT_008a9534`/_9530 (the 0x638-byte rolling error log + dirty
flag) likewise.  `DAT_008a6eac`/_eb0 (zoom-mode display override
globals) modelled as `cs_zoom_override_width`/`_height`.

Remaining gap: `FUN_005b8a20` (16bpp pixel-format binding inside
`zdd_create_screen`'s post-success hook) is the one un-ported call
on the boot path.  ECX identity ambiguous (open RE thread); the
boot path may need this once the harness runs live.

Next: wire `cs_dispatch_create_screen` into the drop-in's WinMain
(currently retail still owns engine init) — the natural next
visible-output checkpoint.

---

## 2026-05-25 — `zdd_create_screen` PORTED (FUN_005b8480, 1088B, full 5-mode dispatch)

The big inner CreateScreen body lands.  Single-checkpoint port over
the leaves that came in this session: release_children prologue
(already had it), primary-surface DDSD builder + CreateSurface (new
Win32 leg), 8bpp palette setup (just ported), back-buffer attach
(just ported), and the existing orchestrator + clipper attach.

Per-mode wiring (mirrors retail exactly):
- **mode 0 (Full)**: CreateSurface primary (flippable, optional
  VRAM) → alloc primary_obj → attach_backbuffer with the videomem
  flag → clipper attach.
- **mode 1 (Safe)**: CreateSurface primary (non-flippable, no
  backbuffer count) → alloc primary_obj → create_surface_pair with
  videomem flag → clipper attach.
- **mode 2 (Windowed)**: SKIP primary CreateSurface (release any
  prior com_a) → alloc primary_obj → create_surface_pair with
  videomem flag → clipper attach.  Same code path as Safe minus
  the primary alloc.
- **mode 3 (DB)**: CreateSurface primary (flippable, no-VRAM-fold
  at the primary layer; videomem honoured at orchestrator) → alloc
  primary_obj → alloc back_obj_a + attach_backbuffer to it (forced
  no-VRAM) → create_surface_pair on primary_obj with videomem flag
  → clipper attach.
- **mode 4 (Zoom)**: like DB but adds a third ZDDObject — back_obj_a
  attaches the display-sized back-buffer (rect[0/1]), back_obj_b
  gets a source-sized orchestrator-created surface (rect[5/6]),
  primary_obj gets a source-sized orchestrator-created surface.
  Both orchestrator calls force VRAM (hardcoded p5=1).

Failure cleanup mirrors retail's "release just the latest failure"
pattern — prior ZDDObject slots leak on per-mode failure since
the caller (FUN_00582e90) exits the process via FUN_005bf5db(0)
shortly after.

**New struct fields**: `zdd_t` now has explicit `screen_pos_x` /
`screen_pos_y` (zeroed slots at +0x138/+0x13c — likely a paired
origin set elsewhere in the windowed-mode path), `screen_width` /
`screen_height` (+0x140/+0x144), and `screen_rect[7]` (+0x148..
+0x163).  `pixel_format_mode` doc updated to clarify the dual
role: it's the launcher's mode_arg (0..4), and FUN_005b8c00's
"== 2" check (Windowed) doubles as "needs explicit DDPIXELFORMAT".

**TODO**: 16bpp pixel-format binding via `FUN_005b8a20` is a one-
line TODO inside the post-success hook.  The boot path is bpp 16
so this gap may matter for visible output.  ECX identity ambiguous
(pixel-format descriptor object, not the calling ZDDObject) —
needs Frida verification at the first live call.

Tests now: **286 pass, 0 fail, 6 skip** (up from 272; 14 new — 5
primary-DDSD builder branch tests, 9 create_screen mode + edge
tests).  Cross-build with mingw clean (the 32-bit cross compile
exercises the new `_Static_assert` offsets for the 6 new fields).

Next: port `FUN_00582e90` (the outer dispatcher) — straight
transcription over `zdd_create_screen` now that the body is in.

---

## 2026-05-25 — back-buffer attach + 8bpp palette setup PORTED (closes 2 FUN_005b8480 leaves)

Two more leaves of `FUN_005b8480` (the big mode-aware surface-init)
land before tackling its body.

**`zdd_object_attach_backbuffer`** (FUN_005b9740, 153 bytes) — the
back-buffer fetch helper used by `FUN_005b8480` mode 0 (Full),
mode 3 (DB Mode), and the first leg of mode 4 (Zoom).  Pure-logic
orchestration over a new Win32 primitive `zdd_get_attached_surface`
that wraps `IDirectDrawSurface7::GetAttachedSurface` (vtable[12] /
byte 0x30).  The Ghidra decomp was misleading — Ghidra lost track
of the stack frame and emitted bogus `unaff_retaddr` params for the
trailing `stamp_metrics` call.  Disassembly via r2 confirmed the
real signature: `(self, primary_surface, width, height,
force_videomem)`.  Sequence: prefill_desc(0,0) → build DDSCAPS2 with
caps[0] = DDSCAPS_BACKBUFFER (4) or |DDSCAPS_VIDEOMEMORY (0x804) →
GetAttachedSurface into `self->com_primary` → stamp_metrics(w, h,
0, 0, w, h) → set_color_key(0x1ffffff sentinel).

**`zdd_setup_8bit_palette`** (FUN_005b8e00, 157 bytes) — 8bpp palette
allocator called by `FUN_005b8480` when bpp == 8.  Pure-logic
orchestration over two new Win32 primitives: `zdd_create_system_palette`
(GetSystemPaletteEntries + IDirectDraw7::CreatePalette vtable[5] /
byte 0x14 with DDPCAPS_8BIT, stash in `self->com_b`), and
`zdd_surface_set_palette` (IDirectDrawSurface7::SetPalette vtable[31]
/ byte 0x7c).  The orchestrator null-checks `self->com_a` before
SetPalette — matches retail exactly.

**Open RE thread closed**: `self->com_a` (ZDD +0x128) is the primary
display IDirectDrawSurface7.  Was previously listed as "Roles
unknown — likely IDirectDrawPalette + IDirectDrawClipper or similar"
in `zdd.h`.  FUN_005b8e00 calls SetPalette on it, and FUN_005b8480
mode 0/3/4 allocates it via `ddraw7->CreateSurface(..., &this+0x128,
NULL)`.  `com_a` doc comment updated to reflect the pinned role.

Tests now: **272 pass, 0 fail, 6 skip** (up from 265/6; 7 new — 3
attach_backbuffer, 4 setup_8bit_palette).  Cross-build clean.

Next: port `FUN_005b8480` itself.  All leaf deps are now in place
except `FUN_005b8a20` (16bpp pixel-format binding, ECX identity
ambiguous — likely a global pixel-format descriptor, not the
calling ZDDObject).  The body can land with the 16bpp branch
stubbed/TODO since the boot path (mode 0, bpp 16) is the
hot one and depends on that final stub being roughed in.

---

## 2026-05-25 — CreateScreen mode dispatch MAPPED + 4 leaf helpers PORTED

Setup checkpoint before tackling the big mode-aware surface-alloc
inside `FUN_00582e90`.  Two things landed:

**Documentation pass** — `docs/findings/ddraw-init.md` "FUN_00582e90
mode-dispatch CreateScreen" section now has the full 5-mode table
filled in (Full / Safe / Windowed / DB Mode / Zoom), the prologue
(release prior DAT_008a6ec0 ZDDObject), the centre-rect math used by
mode 4, and the 7 fixed error-string mappings (which retail's
`s_It_failed_in_CreateScreen___*` strings get logged per branch).
Mode 0 is the boot path (640×480, 16bpp) and the only branch that
calls our already-ported `zdd_object_new` factory.  Modes 1–4 each
dispatch through the still-unported `FUN_005b8480` (1088 bytes —
big internal-mode-aware surface-init that owns the three ZDD
slots at +0x16c/+0x18/+0x1c).

**Four leaf helpers PORTED** — the simple bits that FUN_00582e90's
prologue + each per-mode branch touch (none of which actually do
surface allocation; that's `FUN_005b8480`'s job in the next
checkpoint):

  1. `zdd_hide_cursor` (FUN_005b8dd0, 33 bytes) — inverse of the
     existing `zdd_restore_cursor_on_release`.  Gates on
     `cursor_state == 1` (currently shown) and calls
     `zdd_show_cursor(0)`.  Idempotent on already-hidden.

  2. `zdd_set_display_mode` (FUN_005b8900, 74 bytes) —
     IDirectDraw7::SetDisplayMode wrapper.  Vtable index 21 / byte
     offset 0x54.  Args: (w, h, bpp, refresh, 0=dwFlags).  Logs
     "DirectDraw.SetDisplayMode" DDERR on failure.

  3. `zdd_get_display_mode` (FUN_005b8950, 126 bytes) —
     IDirectDraw7::GetDisplayMode wrapper.  Vtable index 12 / byte
     offset 0x30.  Builds a stack DDSURFACEDESC2 with dwFlags=0x41006
     (HEIGHT|WIDTH|PITCH|PIXELFORMAT) + a pre-stamped ddpf
     (dwSize=0x20, DDPF_RGB).  Returns (w, h, pitch) via out-pointers
     — any of which may be NULL (mode 4 passes pitch as NULL).  No
     DDERR log; retail's caller picks the message itself.

  4. `zdd_busy_wait_ms` (FUN_005b5ac0, 39 bytes) — GetTickCount
     busy-spin.  All fullscreen branches pause 2000 ms between
     SetDisplayMode and surface-create — looks like a "let the mode
     transition settle" workaround.  Host stub instantly returns +
     records the argument so tests don't actually sleep.

**Open RE thread closed**: `FUN_005b9390` is exactly our existing
`zdd_object_dtor` — same body, byte-for-byte.  The pair
`FUN_005b9390 + FUN_005bef0e` in FUN_00582e90's prologue is exactly
`zdd_obj_destroy(&DAT_008a6ec0)`.  No new helper needed for that
slot.

Tests now: **265 pass, 0 fail, 6 skip** (up from 256/6; 9 new — 3
for hide-cursor, 2 for SetDisplayMode, 3 for GetDisplayMode, 1 for
busy-wait).  Cross-build with mingw still clean.

Next: port `FUN_005b8480` itself — the 1088-byte internal mode-
aware surface-init that all 5 branches funnel through.  Once that's
in, FUN_00582e90 becomes a thin dispatcher we can port in one shot.

---

## 2026-05-25 — Clipper attach PORTED (FUN_005b9520)

Closes out the ZDDObject's COM-attach lifecycle.  `zdd_object_attach_clipper`
(FUN_005b9520, 157 bytes per .text) does the canonical "release prior,
create new, configure, attach" dance:

  1. zdd_com_release(&self->com_back)
  2. self->parent->ddraw7->CreateClipper(0, &self->com_back, NULL)   vtable[4]
  3. self->com_back->SetClipList(&stack_NULL, 0)                     vtable[7]
  4. self->com_primary->SetClipper(self->com_back)                   vtable[28]

Pure-logic orchestration in zdd.c over three new Win32 primitives
(zdd_create_clipper, zdd_clipper_set_clip_list_null,
zdd_surface_set_clipper).  Splitting the primitives makes the
sequence verifiable on host: 4 new tests assert call order
(create -> list -> attach), pre-existing-com_back release, and the
defensive null-skip on CreateClipper failure + missing primary.

Two structural notes captured as open RE threads:
- The +0xac field on ZDDObject is dual-role: back-buffer
  IDirectDrawSurface7 for normal surface objects, IDirectDrawClipper
  for clipper-only objects.  Both implement IUnknown so the dtor's
  release path doesn't care, but the field name "com_back" is now
  misleading.
- The vtable[7] (SetClipList) call passes a stack-local NULL pointer
  as the LPRGNDATA, which is invalid input.  ddraw-init.md flagged
  this offset as potentially-mis-decompiled (could be SetHWnd at
  vtable[8]).  Our port mirrors the literal vtable+0x1c recovery;
  Frida verification recommended.

Tests now: **256 pass, 0 fail, 6 skip** (up from 252/6; 4 new host
tests for the clipper-attach call sequence).

---

## 2026-05-25 — CreateSurfacePair factory PORTED (FUN_005b8b40)

Wraps the surface-alloc stack up to the public engine entry point.
`zdd_object_new` (FUN_005b8b40, 184 bytes) does the canonical "allocate
+ ctor + orchestrator + cleanup-on-failure" sequence:

```c
zdd_object *zdo = calloc(1, sizeof(zdd_object));
if (!zdo) return 0;
zdd_object_ctor(zdo, parent);
if (!zdd_object_create_surface_pair(zdo, w, h, 0, colorkey,
                                    count, 0, 0, w, h)) {
    zdd_object_dtor(zdo); free(zdo); return 0;
}
*out = zdo; return 1;
```

Mirrors retail's `operator_new(0xd8) → FUN_005b9350 → FUN_005b95c0 →
on-fail FUN_005b9390 + FUN_005bef0e` byte-for-byte.  We swap operator_new
for calloc (deterministic zero-init; the subsequent ctor stamps every
observable field so the observable behaviour is identical) and the
heap-free primitive for free.

Side change: `zdd_object_create_surface_pair` (the orchestrator) gains
an int return type.  Retail's Ghidra decomp shows it as void but the
assembly leaves the last callee's EAX value as the implicit return;
FUN_005b8b40 reads that as int to decide cleanup.  The mapping:
0 means "CreateSurface failed OR (SetColorKey failed AND key was
non-sentinel)"; 1 means "everything OK or the sentinel path
short-circuited SetColorKey".  All 4 existing orchestrator tests still
pass since none of them was checking the return value.

Tests now: **252 pass, 0 fail, 6 skip** (up from 246/6; 6 new — 3 for
the orchestrator's new return-int behaviour across sentinel-success,
create-fail, and setkey-fail branches; 3 for the factory: happy path,
create-fail teardown, setkey-fail teardown).

---

## 2026-05-25 — Surface-alloc stampers + orchestrator PORTED (4 functions)

Closes out the ZDDObject surface-alloc sub-tree at the leaf level.
Four new functions land in the same `zdd.{c,h}` family:

  - FUN_005b97e0  `zdd_object_prefill_desc`              66 bytes
  - FUN_005b98c0  `zdd_object_stamp_metrics`             73 bytes
  - FUN_005b9830  `zdd_object_set_color_key`            138 bytes
  - FUN_005b95c0  `zdd_object_create_surface_pair`      110 bytes

Pure logic + tests on the host; the Win32 leg adds
`zdd_surface_set_color_key` (IDirectDrawSurface7::SetColorKey via
vtable[29] with DDCKEY_SRCBLT) to `zdd_win32.c`.

The 0x1ffffff colorkey sentinel was confirmed to be load-bearing:
the orchestrator's boot call site passes it (per FUN_005b8b40 →
0x5b95c0), so the SetColorKey vtable call is skipped entirely for
the primary surface alloc and `state_flag` stays 0.  Non-sentinel
keys stamp state_flag = 0x8000 and call through to the vtable.  The
16bpp branch inside FUN_005b9830 is a TODO — it expects to call
FUN_005b8b00 (a channel-shift converter that takes a pixel-format
descriptor in ECX, identity unresolved), but that branch is dead at
boot because the sentinel wins.  Once the descriptor's owner is
pinned, wire FUN_005b8b00's converted output in place of the raw key.

ZDDObject's struct now names 21 fields (up from 8): the three
self-pointers at +0x00..+0x08 (each points at a specific sub-field of
the embedded DDSURFACEDESC2 — lpSurface @ DDSD+0x24, lPitch @
DDSD+0x10, dwHeight @ DDSD+0x08), the six metric slots at +0x0c..
+0x20, the colorkey pair at +0x24/+0x28, the four secondary metrics
at +0xb0..+0xbc, and the two cached create-time args (caps_in,
force_videomem_in) at +0xcc/+0xd0.  Only `embedded_ddsd` (the
124-byte scratch DDSURFACEDESC2 at +0x30..+0xab) remains opaque
`uint8_t[]`.  Field-naming reflects byte offsets, not semantics — the
metric clusters likely hold src/dst rect TL/BR pairs based on the
orchestrator's argument shape, but no consumer reads them yet.

Tests now: **246 pass, 0 fail, 6 skip** (up from 234/6; 12 new
host tests across prefill / metrics / set_color_key / orchestrator
plus a "passes caps from field" test that verifies the deliberate
prefill→roundtrip→CreateSurface read-from-field path retail uses).

---

## 2026-05-25 — DDSURFACEDESC2 builder + CreateSurface PORTED (FUN_005b8c00)

Lands the meatiest "actually create a DDraw surface" function — 372
bytes of DDSURFACEDESC2 construction + vtable-call into
`IDirectDraw7::CreateSurface`.  Split clean: pure-logic descriptor
build (`zdd_build_surface_desc`) in zdd.c, Win32-side surface call
(`zdd_create_surface`) in zdd_win32.c.

Pure-logic dispatch verified by 10 tests covering each pixel-format
branch:

  - dwCaps = (caps_base | OFFSCREENPLAIN), |= VIDEOMEMORY when
    force_videomem OR self->videomem_flag
  - pixel_format_mode != 2 → short-circuits (no DDPF), even if a bpp
    is set
  - pixel_format_mode == 2 → DDPF block with:
      bpp 8:  PALETTEINDEXED8, bitcount=0, masks=0
      bpp 16: RGB, bitcount=16, RGB565 masks
      bpp 24/32: RGB, bitcount=N, masks = 0xFF0000/00FF00/0000FF
        (retail's switch literally falls 24 → 32 — the "engine quirk"
         from docs/findings/ddraw-init.md is preserved literally)
      other bpp: defensive — leaves bitcount and masks at 0

The Win32 wrapper additionally binds a palette (vtable[31] SetPalette)
when self->com_b is non-NULL — confirming the open-RE thread guess
that com_b is likely the IDirectDrawPalette (zdd-init.md hypothesis).
Failure path: zdd_log_dderr("DirectDraw", "CreateSurface", hr) + return 0.

Adds ZDD field `videomem_flag` at +0x134 (formerly pad).  Read by the
descriptor builder, not yet written — the higher-level mode-dispatch
FUN_00582e90 is what stamps it during fullscreen-mode init.

Tests now: **234 pass, 0 fail, 6 skip** (up from 224/6; 10 new
pure-logic descriptor tests).

---

## 2026-05-25 — ZDDObject ctor + dtor + pixel-buf release PORTED

Three more leaf ports landing on top of the ZDD wrapper checkpoint
that closes the "ZDDObject cleanup chain" open thread:

  - FUN_005b9350  `zdd_object_ctor`              50 bytes
  - FUN_005b9390  `zdd_object_dtor`              75 bytes
  - FUN_005b93e0  `zdd_object_release_pixel_buf` 42 bytes

ZDDObject struct shape pinned at 0xd8 bytes with the 6 lifecycle-
pair fields named (`com_primary` at +0x2c, `com_back` at +0xac,
`parent` at +0xc0, `pixel_buf` at +0xc4, `pixel_buf_flag` at +0xc8,
`state_flag` at +0xd4).  The embedded DDSURFACEDESC2 + window-fit
metrics regions (`_pad030[0xac-0x30]` and `_pad0b0[0xc0-0xb0]` +
`_pad0cc[0xd4-0xcc]`) stay as opaque pad until the surface-alloc
helpers (FUN_005b97e0 / _98c0) get ported alongside FUN_005b95c0.

The previously-placeholder `zdd_obj_destroy` in zdd_win32.c gets
replaced — it's now pure logic in zdd.c that walks
`zdd_object_dtor` then heap-frees the allocation.  Only the
`zdd_object_local_free` primitive remains on the Win32 side (wraps
LocalFree).

Release order in `zdd_object_dtor` matches retail's
FUN_005b9390 byte-for-byte: pixel buf first, then com_back (+0xac),
then com_primary (+0x2c), then parent->open_objects decrement.  The
"com_back BEFORE com_primary" order is the load-bearing detail — it
keeps the COM refcount graph clean when com_back is an
IDirectDrawSurface7 fetched via GetAttachedSurface off com_primary
(open thread per docs/findings/ddraw-init.md
"FUN_005b9520 — Clipper attach" notes).

7 new host tests; the two pre-existing release-children + dtor tests
were updated to use real malloc'd ZDDObjects instead of synthetic
pointers (zdd_obj_destroy now dereferences).

Tests now: **224 pass, 0 fail, 6 skip** (up from 217/5; 7 new
including a 32-bit-only zdd_object layout skip).

---

## 2026-05-25 — ZDD wrapper first slice PORTED (8 functions)

First slice of the DirectDraw 7 wrapper class HANDOFF flagged as the
recommended "next move" — the eight leaf functions in
`docs/findings/ddraw-init.md`'s call graph that together cover the
class lifecycle + DDraw init + DDERR error logging.  Lands in
`src/zdd.{c,h}` + `src/zdd_win32.c` + `tests/test_zdd.c`.

Ports (in size-ascending order):

  - FUN_005b8da0  `zdd_restore_cursor_on_release`     33 bytes
  - FUN_005b88c0  `zdd_directdraw_create_ex`          57 bytes
  - FUN_005b89d0  `zdd_set_coop_level`                71 bytes
  - FUN_005b7fe0  `zdd_dtor`                          90 bytes
  - FUN_005b7f80  `zdd_ctor`                          94 bytes
  - FUN_005b8040  `zdd_release_children`             139 bytes
  - FUN_005b7ee0  `zdd_create`                       153 bytes
  - FUN_005b80d0  `zdd_log_dderr`                    826 bytes

Pure-logic split matches the established bitmap_session / wnd_proc
pattern: ctor, dtor decision tree, DDERR-to-string mapping, and the
log-message builder live in zdd.c; the six Win32 primitives
(ShowCursor, OutputDebugStringA, IUnknown::Release via vtable[2],
DirectDrawCreateEx, IDirectDraw7::SetCooperativeLevel via vtable+0x50,
placeholder ZDDObject destroyer) live in zdd_win32.c.  Host tests
exercise the pure logic with controllable stubs.

The DDERR log message format was a small detective exercise: r2
pszj on each of the seven strings the helper concatenates (verified
against `docs/decompiled/by-address/5b80d0.c` and a fresh
`vendor/unpacked/sotes.unpacked.exe` read) showed retail uses commas-
in-place-of-periods in "Warning,exists ZDD errors," and " failed,Error
Code " — not typos, intentional.  The 18-entry HRESULT → DDERR_xxx
table in `k_dderr_table[]` mirrors the switch ladder in FUN_005b80d0
verbatim; format output is fully exercised by 4 host tests covering
known/empty-prefix/unknown-hresult/null-input paths.

Open follow-ups now:
- ZDDObject (`FUN_005b9350` ctor + the FUN_005b9390 cleanup chain) is
  still unported; `zdd_obj_destroy` in zdd_win32.c is a `free()`
  placeholder that will dispatch through the cleanup chain once
  ZDDObject lands.  Host tests don't touch it.
- Vtable indices for the COM Release calls (+0x128 / +0x12c) match
  IUnknown's standard but the semantic role of those two com pointers
  hasn't been pinned — `com_a` / `com_b` are deliberately vague.
  Likely IDirectDrawPalette + IDirectDrawClipper given the surrounding
  code, confirm when their setters land.
- `pixel_format_mode` / `pixel_format_bpp` (+0x164/+0x168) are written
  by paths we haven't ported yet (FUN_005b8c00 reads them when building
  DDSURFACEDESC2 in `mode == 2` paths).  Modelled as fields for size
  correctness; no consumer in this checkpoint.

Tests now: **217 pass, 0 fail, 5 skip** (up from 200 pass / 4 skip;
the new skip is `zdd_layout_matches_retail_offsets`, 32-bit only).
Real mingw build adds `-lddraw -ldxguid`.

---

## 2026-05-25 — Pixel-Drawer boot-time slot tables PORTED

Picks up an open-thread item from HANDOFF (the 5 fixed-size sprite-slot
allocator loops inside `FUN_00562ea0` lines 462-576).  All five groups
(`DAT_008a92b8` ×20, `DAT_008a9308` ×20, `DAT_008a9358` ×5,
`DAT_008a93bc` ×4, `DAT_008a936c` ×20 — total 69 slots) plus the four
special-colour writes that populate group D land in
`src/pixel_drawer.c` as `pd_boot_init_slots(fmt)` + a companion
`pd_boot_release_slots()` for host-test teardown.  All primitives this
calls into (`pd_blend_init`, `pd_blend_set_color`, `pd_blend_commit`)
were already ported in the first Pixel-Drawer pass — the boot driver
is purely orchestration.

Also corrects a finding doc: `winmain-and-bootstrap.md` claimed the
group-D 4 slots were "filled in later by code we haven't yet mapped" —
disassembly of FUN_00562ea0:0x5637f1-0x5638b6 (via radare2) shows the
4 special-colour writes ARE in the same boot phase, just written
inline as 4 explicit `mov ecx, [addr]` thiscalls that Ghidra's source
view collapsed into ambiguous untyped calls.  Targets are
D[0]/D[1]/D[3]/D[2] (in that order; D[2] and D[3] also get
commit_flag=1).  Boot driver replays this exact sequence.

Storage choice: static `PdBlend g_pd_boot_group_*[]` arrays rather
than retail's `PdBlend *DAT_X[]` heap-pointer-arrays.  Retail's slots
are process-lifetime allocations from `operator_new(0x50)` that are
never freed — static storage gives the same observable end-state with
zero malloc and is ASan-quiet under repeated boot.  If a future
consumer ever needs the pointer-array layout, add a parallel
`PdBlend *g_pd_boot_*_ptrs[]` view then.

Tests now: **200 pass, 0 fail, 4 skip** (up from 192).  8 new tests:
per-group weight/mode/state checks (A weight ramp /20 mode 1,
B weight ramp /22 mode 0, C grey-ramp R=G=B = 1100..1740,
E weight ramp /20 mode 2), group D 4 special-colour assignments
including the D[3]→D[2] retail-quirky order, full-coverage check
that every slot in every group commits its RGB masks from `fmt`,
custom-format propagation (RGB555 spot-check), and idempotency
re-run.  The idempotency test caught a real bug — `pd_blend_init`
zeroes channel.lut without freeing it first, which leaks on re-init
of a static slot — fixed by having `pd_boot_init_slots` call
`pd_boot_release_slots` at entry.  This is a host-build concern only
(retail allocates a fresh slot each boot via operator_new, so the
issue doesn't manifest in the real engine).

---

## 2026-05-25 — FUN_0057ca40 6th pass: 9 inline slot-clones PORTED (function functionally complete)

Closes the last deferred subsystem of FUN_0057ca40.  The 9 inline
FUN_00582b80 calls (the ones taking ECX = a `paVar1 = DAT_X` source
slot rather than going through the SS_MGR table) are extracted by
`tools/extract/57ca40_inline_clone_table.py` and replayed in retail
issue order by `ar_apply_group3_inline_clones`, called from the tail
of `ar_register_group3_sprites` after the SS_MGR clone pass.  3
distinct source pool indices (383, 390, 402) — all themselves
populated by the 1st-pass slot-register table — fan out into 9
disjoint targets (257..261, 384..385, 391..392).

No new primitive needed: each replay is just
`ar_sprite_slot_clone(pool[dst], pool[src])`.  The info-entry side
of each cluster (zero + marker/flag-copy + data-ptr for the 4 early
clusters; 20-byte STRUCT_COPY for the 5 late ones) is already
covered by the 4th-pass `ar_apply_group3_info_events` — verified by
re-running the audit tool `57ca40_pool_map.py` (0 orphans across all
443 pool writes).

With this pass landed, FUN_0057ca40's six retail-observable
subsystems (slot register, info events, SS_MGR clones, inline
clones, plus the two thiscall primitives) all replay in the port.
The next consumer of this state is `FUN_00586010` (palette-draw with
flag dispatch — see rabbit-hole §6); porting it will pin the
per-prefix flag semantics from the read side.

Tests now: **192 pass, 0 fail, 4 skip** (up from 187).  5 new tests
cover: target population after register, late-cluster shared-source
metadata propagation, early-cluster metadata propagation, apply
idempotency, src/dst-set disjointness.  Updated 2 existing tests
to reflect the new slot-count expectation (327 → 336).

See `docs/findings/0057ca40-rabbit-hole.md` §4 for the cluster
source/target table.

---

## 2026-05-25 — FUN_0057ca40 5th pass: 94 SS_MGR slot-clones PORTED

Last of the sprite-slot work in FUN_0057ca40.  The 94 FUN_004179b0
calls inside the function are extracted by
`tools/extract/57ca40_clone_table.py` and replayed in retail issue
order by `ar_apply_group3_clones`, called from the tail of
`ar_register_group3_sprites` after the 4th-pass info-events apply.
Total clones: 94 (54 distinct sources, 94 distinct destinations;
sources span main_slot 134..321, destinations span main_slot 135..322,
all within the 233-slot register region populated by the same
function's earlier pass).

The primitive `ar_ss_mgr_clone_slot(dst_pool_idx, src_pool_idx)`
reuses the existing `ar_sprite_slot_clone` (slot-metadata copy) and
`ar_info_entry_clear` (info-entry zero) primitives, since
FUN_004179b0's body is structurally identical to those primitives'
bodies — only the indirection differs.  Modeling sidesteps the SS_MGR
`this` pointer via a new unified-pool accessor `ar_pool_get_slot`
that maps pool indices 1..12 → ramp slots and 13..908 → main slots
(see rabbit-hole §7: SS_MGR == input_mgr at 0x008a6b60, so the host
already owns both tables as globals).

Tests now: **187 pass, 0 fail, 4 skip** (up from 176).  11 new tests
cover: pool accessor on sentinel/ramp/main ranges, primitive-level
clone (metadata propagation, info marker+flag copy, info data/palette
stay null, dst-entries destruction under ASan), table-walker (apply
idempotency, dst pool range, first-clone metadata propagation,
integration with register_group3_sprites).  Integration count test
updated: register pass writes 233 slots + clone pass writes 94 more
= 327 unique populated slots.

The remaining FUN_0057ca40 deferred work is now strictly on the **9
FUN_00582b80 cluster wiring** — open-coded template-slot init per
cluster, not table-extractable.  See HANDOFF "Next move" #3.

---

## 2026-05-25 — FUN_0057ca40 4th pass: 443 info-entry pool writes PORTED

Mechanical follow-on to the per-call-site indexing confirmation:
extracted the full info-entry event stream and replayed it as a
443-row static table walked by `ar_apply_group3_info_events`,
called from the tail of `ar_register_group3_sprites`.  Every write
the function performs to the parallel info-entry pool now lands in
the host model: 138 marker, 194 flag, 98 data-ptr, 5 struct-copy,
4 marker-copy, 4 flag-copy events spanning pool indices 92..437.

Extractor at `tools/extract/57ca40_info_table.py` mirrors the
`57ca40_sprite_table.py` model — re-run after re-export to catch
drift.  It captures 4 short-typed data-ptr writes (lines 2142,
2147, 2286, 2291) that the `57ca40_pool_map.py` audit's regex
missed — taking the real total from 439 to 443.  Sanity check
verifies all dst indices fall in [0, 909); the 5 struct copies'
sources (pool[139..145]) are not produced inside FUN_0057ca40, but
they're alloc-zeroed by the pool allocator and read at zero —
matching retail's allocator-zeroed pool semantics.

DATA_SET payloads (98 events; 25 distinct PE rdata addresses, e.g.
0x006748d0) are stored as opaque uintptr_t markers.  No consumer
reads them as bytes yet — the first FUN_00586010-style palette
draw with flag dispatch will need extracted PE bytes; this port is
observability-only on the data side.  8 new spot-check tests cover
the kinds (FLAG_SET, MARKER_SET, DATA_SET, MARKER_COPY, FLAG_COPY,
STRUCT_COPY) plus the bounded-region invariant (events only touch
pool[92..437]) plus the wiring through `ar_register_group3_sprites`.
Tests now: **176 pass, 0 fail, 4 skip** (up from 168).

The remaining FUN_0057ca40 deferred work is now strictly on the
**sprite-slot side**: 94 SS_MGR clones (FUN_004179b0) plus the 9
inline-clone clusters (FUN_00582b80 sprite-slot ops).  Info-entry
side is closed.

---

## 2026-05-24 — FUN_0057ca40 per-call-site indexing confirmed

Walked all 466 info-entry references inside FUN_0057ca40 and matched
them against slot decls + clone targets in the same function.  The
implicit "pool[i] shadows slot[i]" model is fully confirmed: 0 of
434 pool writes are orphaned (i.e. every info-entry write at retail
BSS `0x8a8440 + i*4` corresponds to a slot at `0x8a760c + i*4`,
where the slot is either declared inline, declared via the helper,
or produced by SS_MGR clone / inline-template clone).  Audit script:
`tools/extract/57ca40_pool_map.py` — rerun after re-export to catch
drift.

The walk also surfaced a Ghidra rendering gotcha for `DAT_008a8XXX`
references: different DAT vars carry different inferred C types
(byte-typed vs short-typed), so source-level offsets like `+2` vs
`+4` can both denote the same disasm byte offset (+4 = flag).
Verified by disasm at 0x57cad7 (byte-typed, mov [eax+4]) vs
0x57cf3d (short-typed, mov [ecx+4] but Ghidra source says +2).
Rabbit-hole §2 rewritten with the correction; the "pad +2..+3 is
never touched" claim from §4 is reaffirmed (no Ghidra +2 source
write actually targets byte +2 in retail).

This unblocks the 434-write port — it's now mechanical (compose
slot-idx → flag/marker/data tuples in retail issue order, replay
into `g_ar_info_table[i]`).  Deferred because no consumer reads the
info-entry pool yet; the first FUN_00586010-style palette draw with
flag-dispatch will need it.

---

## 2026-05-24 — ar_info_entry pool (909 entries) + allocator finding

Followed the "where do the parallel-table pointer slots come from"
thread to its root and unblocked the full pool model.  The allocator
is **FUN_00562ea0:225-253** — a single 909-iteration loop that runs
right before the "SS_MGR_Preparation" log line.  It heap-allocates
two parallel pools side-by-side: a 0x44-byte sprite slot AND a
0x14-byte info entry per index, stored in adjacent BSS pointer
tables at 0x8a760c..0x8a8440 and 0x8a8440..0x8a9270.

That pins three corrections we'd been waving past:

  - **`ar_info_entry` is 20 bytes, not 16.**  The allocator zeros
    five dwords (the last being `+0x10`); the existing `clear`
    routine only touches the first 14 bytes.  Struct + static
    asserts updated.
  - **`+0x0c` is a palette pointer**, not the "f_0c semantics
    unknown" placeholder.  FUN_00586010:755 reads it as
    `*(int ***)(DAT + 0xc)`, uses it directly as the source of a
    256-entry palette modifier loop when non-NULL, or falls back to
    `ar_palette_session_begin` + `FUN_00417bc0(entry->data, ...)`
    when NULL.  Renamed `f_0c` → `palette`.
  - **The "parallel pool" is 909 entries, not ~357.**  Retail BSS
    range 0x8a8578..0x8a8b14 (the rabbit-hole's "extends to ~357
    entries" estimate) is just pool indices 78..437 of the full
    909-entry table.

`g_ar_sprite_flags[14]` (flat uint32) replaced by
`g_ar_info_entries[909]` + `g_ar_info_table[909]`.  `ar_state_init`
wires the table.  `ar_register_palette_ramps` now writes through the
table: `g_ar_info_table[AR_INFO_RAMP_FLAGS_BASE + i]->flag = N`,
matching retail's `*(int *)(DAT_008a85xx + 4) = N` pattern.

One new pool-init test (168 pass / 0 fail / 4 skip, up from 167).
Two existing info-entry tests refactored for the new field name
(`f_0c` → `palette`) and the +0x10 field's leave-untouched
guarantee.  `docs/findings/0057ca40-rabbit-hole.md` extended with
sections 5 (allocator finding) and 6 (FUN_00586010 + FUN_00587e00
consumer evidence).  HANDOFF's "Open RE threads" entries on
g_ar_sprite_flags and the parallel-pool are now obsolete.

---

## 2026-05-24 — FUN_00582b80 (slot clone) + FUN_00582d00 (info entry clear)

Ported the next two functions from FUN_0057ca40's deferred subsystem
list: `ar_sprite_slot_clone` (the `__thiscall` slot metadata clone)
and `ar_info_entry_clear` (a 14-byte clear of a parallel-info-table
entry).  Together they form the "clone-and-detach pair" that appears
9× in FUN_0057ca40 — see `docs/findings/0057ca40-rabbit-hole.md`
section 4 for the disasm walk and the new struct discovery below.

The big finding is **`ar_info_entry`**, the 16-byte parallel-info-
table entry shape that HANDOFF previously called out as "each retail
entry is itself a POINTER to a struct."  Disasm at 0x57fa98 confirms
FUN_00582d00's `this` is loaded from `[0x008a8a40]` — a pointer in
the parallel table — and the writes pin the layout: u16 marker @+0,
pad @+2, u32 flag @+4, const void* data @+8 (later set to PE rdata
pointers like &DAT_006752f8), u32 f_0c @+12.  Struct + static
asserts now live in `src/asset_register.h`.

`ar_sprite_slot_clone` reuses `ar_sprite_slot_destroy` for its
free-old-state prologue, then stamps every metadata field from src
to dst in retail order, allocates a fresh 1-entry `entries[]`, and
deep-copies src's `aux_buf` (24-byte stride entries, count from
src->f_38).  Retail quirk preserved: `dst->f_38` stays at 0 even
when the aux deep-copy runs — the count isn't propagated; we match.

7 new unit tests (167 pass, 0 fail, 4 skip — up from 160).  Both
functions tagged in `TagThiscallFunctions.java` (26 tags now);
parse+tag stage confirmed `ar_info_entry=16` parses cleanly and
both new functions land in their class namespaces.  Module-isolation
still holds: no real caller is ported yet — the functions are
available primitives for the eventual FUN_0057ca40 wiring once
SS_MGR and the parallel-info-table array land.

---

## 2026-05-24 — Headless Parse C Source automation

Ported the `ParseCSource.java` script from sibling OpenMare to
`tools/ghidra-scripts/`, plus a `tools/ghidra-cpp-shim/` directory
with minimal `stdint.h` / `stddef.h` / `stdbool.h` shims for
Ghidra's bundled CPP (it has no libc).  ParseCSource passes the
shim dir as an include path and `-D_Static_assert(c,m)=` to strip
the C11 keyword from our headers, so `src/asset_register.h`,
`src/bitmap_session.h`, and `src/wnd_proc.h` parse cleanly into
Ghidra's DTM in headless mode.

`tools/ghidra-tag-and-export.sh` upgraded to a 3-stage pipeline
running in one analyzeHeadless session: ParseCSource → TagThiscall
Functions → ExportDecompiledC.  Verified end-to-end: all 14 structs
parsed (sizes match — paint_ctx=368, zdm_entry=56, log_singleton=1284,
etc.), 24/24 tags applied, 1768 functions re-exported.

Immediate payoff: the bodies of the 7 WndProc-dep thiscalls now
show typed `this->field` accesses.  paint_ctx::FUN_005b9130 reads
`if (this->state == 2) { BitBlt(hdc, this->blit_x, this->blit_y,
this->blit_w, this->blit_h, ...); FUN_005b94e0(this->back_ctx, ...); }`.
The WndProc itself reads as a clean class-dispatched function
(`input_mgr::FUN_0058ffa0((input_mgr *)&DAT_008a6b60, 1)`,
`zdm::FUN_005bbd20(DAT_008a93e4, ...)`, etc.).  No more
"open Ghidra GUI + Parse C Source" manual step — `nix develop -c
./tools/ghidra-tag-and-export.sh` is the single command for
struct-edit → typed-decomp.

See `docs/findings/cpp-recovery-workflow.md` "Automated parsing +
tagging" section for the full how-to + new-header discipline.

---

## 2026-05-24 — WndProc dependency formalization

Modeled the 5 "deep engine" struct shapes the WndProc reaches through
its 5 thiscall callees, and tagged each callee in Ghidra so its
prototype and class namespace get applied to the decomp.  The structs
live in `src/wnd_proc.h`'s new "deep-engine struct shapes" section
and pin only the offsets observed in the disasm:

  - **paint_ctx** (DAT_008a93cc) — +0x2c zdd_device, +0x138 blit
    rect (x/y/w/h), +0x164 state.  `this` for FUN_005b9130 (the
    BitBlt-from-backbuffer paint helper), FUN_005b94e0 (begin-frame
    vtable trampoline at zdd_device->vtable[0x44]), and FUN_005b9500
    (end-frame at vtable[0x68]).
  - **input_dev** (DAT_008a93d8, DAT_008a93dc[2]) — +0x04 dev_obj
    (vtable[0x1c] = Acquire), +0x08 acquired flag.  `this` for
    FUN_005ba290.
  - **zdm** (DAT_008a93e4) — +0x18 entries pointer, +0x1c count,
    +0x2c inline name string.  `this` for FUN_005bbd20 (the
    multiplexer set-active fan-out).  Per-entry struct **zdm_entry**
    has stride 0x38 with +0x00 dev, +0x08/+0x0c sub-device pointers
    (each with own vtable), +0x20 active, +0x24 state2, +0x28
    8-byte cookie.
  - **input_mgr** (singleton at &DAT_008a6b60) — +0x2884 zdm_ptr.
    `this` for FUN_0058ffa0 (input pause-on-deactivate; just NULL-
    guards and forwards to FUN_005bbd20).
  - **log_singleton** (singleton at &DAT_008a6620) — +0x404 path
    CHAR buffer.  `this` for FUN_00408b90 (the engine's
    OutputDebugString + log-file writer).

The wnd_proc.h externs (`g_wp_paint_check_this`, `g_wp_input_dev_extra`,
`g_wp_input_devs[2]`, `g_wp_zdm`) were upgraded from `void *` to the
typed pointer forms, and `wp_input_acquire`'s parameter became
`input_dev *` accordingly.

7 new tags added to `tools/ghidra-scripts/TagThiscallFunctions.java`
(now 24 total).  Headless tag step verified: 24/24 applied.  The
re-export was kicked off — the 7 new functions now show in the decomp
with class namespace + explicit `this` arg at every call site
(typed-body upgrade requires Parse C Source on wnd_proc.h in the
Ghidra GUI, then re-running the script — see HANDOFF "Next move" #1).

Tests unchanged at **160 pass, 0 fail, 4 skip** — the WndProc test
suite still binds `(void *)0xN` literal addresses into the new typed
globals via implicit `void *` → `T *` conversion.  Cross-build clean.

---

## 2026-05-24 — ar_register_group3_sprites (FUN_0057ca40 partial)

Ported the 233 sprite-slot register operations inside FUN_0057ca40 —
the "Ghidra-fails 24884 B body" from the prior HANDOFF turned out to
decompile cleanly once the typed-struct workflow from the previous
checkpoint was in scope (the `cpp-recovery-workflow` infra silently
fixed it).  3124-line decomp is at `docs/decompiled/by-address/57ca40.c`.

But the body isn't just "register N sprites" — it has FOUR
subsystems.  Only the first is ported here:

  1. **233 sprite-slot registers** (91 inlined + 142 helper-style
     calls).  Slot indices 79..423, all using uniform (zdd, settings,
     group) routing from the caller.  Table-driven through
     `ar_sprite_slot_register`.  **PORTED** as
     `ar_register_group3_sprites` and wired into `ar_boot_register_all`
     between aux_sounds and game_sounds (matches retail issue order).
  2. ~380 parallel-info-table writes (0x008a8578..0x008a8b14).
     **DEFERRED** — needs `g_ar_sprite_flags[]` refactored from flat-u32
     to pointer-to-struct array (~357 entries).
  3. 94 FUN_004179b0 SS_MGR slot-clones.  **DEFERRED** — needs SS_MGR.
  4. 9 FUN_00582b80 + 1 FUN_00582d00 tail.  **DEFERRED**.

See `docs/findings/0057ca40-rabbit-hole.md` for the full breakdown.
Generator at `tools/extract/57ca40_sprite_table.py` — re-run after
re-exporting the decomp to spot drift.

Tests: +7 new tests in `tests/test_asset_register.c` (distinct-slot
canary, group-tag stamping, uniform routing, three spot-checks for
specific entries, no-overlap-with-other-batches assertion).  Plus
the existing `boot_register_all_touches_every_batch_signature_slot`
test now also pins the group-3 spot-check on sprite[79].
**160 pass, 0 fail, 4 skip** (was 153 / 0 / 4).  Cross-build clean.

Asset-register module is now **functionally complete for the title-
scene boot path** — every `ar_register_*` batch that the boot driver
calls is ported, and no ported consumer reads the deferred FUN_0057ca40
state.

---

## 2026-05-24 — ar_boot_register_all wired

The 8 ported `ar_register_*` batches are no longer modules in isolation
— a new `ar_boot_register_all` in `asset_register.c` calls them in
retail issue order, matching FUN_00562ea0:613-624 byte-for-byte modulo
the one un-ported batch (FUN_0057ca40, group 3, 24884 B Ghidra-fails).
This is the "register every asset slot at boot" entry point: pass ZDD,
ZDS, settings, sotesp_module, and a locale state, and every ramp /
sprite / sound / GDI slot the title scene depends on lands populated.

API shape `ar_boot_register_all(zdd, zds, settings, sotesp_module,
locale)` keeps the conceptual settings-vs-sotesp split (in retail both
are the same DAT_008a6e74 pointer at boot; we accept them separately so
unit tests can distinguish "this register batch reads settings" from
"this batch reads sotesp.dll"). `locale == NULL` skips the locale-tail
batch entirely — useful for testing other batches in isolation; retail
always passes a valid struct.

The FUN_0057ca40 gap is marked with an inline comment at the exact
position it'd slot into (between aux_sounds and game_sounds).  No hook
mechanism — once Ghidra-fail is resolved and the function is ported,
the call is dropped in.

Tests: +6 new tests in `tests/test_asset_register.c` covering group-tag
routing, ZDD-vs-ZDS plumbing, the sotesp-module split (idx 0 +
ramp_slots use sotesp; everything else uses settings), locale state
plumbing, NULL-locale skip behaviour, and a "did every batch run"
canary check.  **153 pass, 0 fail, 4 skip** (was 147 / 0 / 4).  The
palette-install side of palette_ramps is a no-op in these tests
because the bs_load_pe_resource stub's resource table starts empty
when asset tests run; the install path is already covered separately
by test_bitmap_session.c's palette_ramps_* tests.

Cross-build clean.

---

## 2026-05-24 — ar_register_palette_ramps (FUN_0057a330) ported

Ported the 3919-byte sprite-batch palette function as
**`ar_register_palette_ramps`** — second-biggest sprite-register call
at boot.  Three observable sections, all wired in this checkpoint:

**12 palette-ramp blocks** — each registers a small (24×24 or 32×32)
type-2 sprite at one of 12 new `g_ar_sprite_ramp_slots[i]` (retail
BSS 0x008a7610..0x008a763c, a 12-pointer table that precedes the
main sprite pool's 0x008a7640 base), runs the same 3-color palette
ramp scheme as `ar_register_main_sprites`
(palette[1]=bg, [41..50]=mid, [51..70]=lerp(mid→fg, i/20)) with
per-ramp colors, then installs.  All 12 share the
`ar_palette_session_begin` / `ar_palette_install` path that landed
in the previous checkpoint — no new decoder code needed; the family
is now reused.  When the resource decoder fails (wrong bit depth
or missing resource) the install is skipped, matching the main
sprites ramp behaviour.

**23 trailing sprite registers** — main-pool indices 33..61 with
mixed icon / panel shapes.  Two of these (idx 36 at retail 0x76d0
and idx 38 at retail 0x76d8) are spelled inline as the
destructor-plus-field-writes pattern in retail; same observable end
state as `ar_sprite_slot_register`, so all 23 flow through the
helper here.  One entry (idx 37 at retail 0x76d4) is the only
register-call in the file that passes `settings=NULL` instead of
the launcher settings — special-cased in the iteration loop.

**14 portrait blocks** — each is a register-call at retail
0x8a7744..0x8a7778 (main pool indices 65..78, portrait/character art
80×{352,480,320,144,400}) followed by a write of a flag value (0 or
3) into a new parallel `g_ar_sprite_flags[]` table (14 entries
modelling the retail BSS region 0x008a8578..0x008a85ac).  The flag's
semantic meaning is unknown — likely a frame-count or facing-direction
override; no consumer is ported yet so we capture just the observable
+4 write into a flat uint32 array (the retail pointer-to-struct
indirection is unmodelled).

Function-level stack-local `bitmap_session` in retail is a vestigial
SEH-protected RAII placeholder — `bs_release_no_free`'d at entry,
`bs_release`'d at exit, never used.  No observable effect; not
modelled.

Tests: +9 new tests in `tests/test_bitmap_session.c` covering all
three sections.  **147 pass, 0 fail, 4 skip** (was 138 / 0 / 4).
Cross-build clean.  Ghidra TAGS array also gained the two thiscall
helpers it uses (FUN_004178e0 / FUN_00491770) so the re-exported
decomp shows typed `this->field` access through the family.

---

## 2026-05-24 — bitmap_session module + palette ramp wired end-to-end

New module **src/bitmap_session.[ch] + src/bitmap_session_win32.c** —
the 7-method `__thiscall` class behind the PE-resource bitmap decoder
in FUN_004178e0's palette-session front half, plus FUN_005b7c10
(compressed-resource header parser, a free function despite living in
the same family).  Lifecycle is entirely stack-managed in
FUN_004178e0 — the bitmap_session is `[esp+8]` over a 0x444-byte
frame.  Win32-free body; `bs_local_alloc_zeroed` / `bs_local_free` /
`bs_load_pe_resource` are externs supplied per build target.

The blocking ECX puzzle from the prior session's deferral (which
`this` does FUN_005b7800 actually run on?) was resolved by r2 disasm
of FUN_004178e0 — every callsite does `lea ecx, [esp+8]` before the
call, confirming the stack-local interpretation.  Outer this
(sprite_slot * in ESI) is read only for the HMODULE+resource_id pair
passed to FindResourceA.  Resource type string is "DATA", not "BMP"
as the prior findings draft assumed.

**ar_palette_session_begin** (FUN_004178e0, ar_sprite_slot method)
lands in asset_register.c — builds the stack session, calls
`bs_decode_resource(..., "DATA", 1)`, emits the BGRA palette into a
caller buffer iff the source was 8bpp.  Then
**ar_register_main_sprites' palette-ramp section** (previously
deferred) is now wired: allocate a 1024-byte buffer, seed via
ar_palette_session_begin from sotesp.dll/0x90b, override palette[1]=0,
palette[41..50]=0x383838, palette[51..70]=lerp(0x383838→0xffffff,
i=1..20 / 20), install onto slot[0] via ar_palette_install.  No-op
when the decoder can't return a palette.

Ghidra workflow improvement: TagThiscallFunctions.java's TAGS array
gained the 7 bitmap_session methods; re-export shows
`__thiscall bitmap_session::FUN_…(bitmap_session *this, …)`
throughout the family and `bitmap_session local_444[1080]` as the
typed stack local in FUN_004178e0.

Tests: +21 new bitmap_session tests (basic state, init/release,
compressed-header signature mismatch + happy path, raw + compressed
decode paths, ar_palette_session_begin BGRA emit + 24bpp skip, and
end-to-end ar_register_main_sprites integration).  **138 pass, 0
fail, 4 skip** (was 117 / 0 / 3 — the new skip is the
bitmap_session layout test, 32-bit-only).  Win32 cross build clean.
Commits: `8cb9fd8` (struct+tags+findings), `4f89867` (port+tests).

---

## 2026-05-24 — Asset-Register: palette-trio leaves (FUN_005b5d90 + FUN_00491770)

Ported the two leaf halves of the FUN_005749b0 palette-ramp trio
that don't need the PE-resource decoder:

**`ar_palette_pack_entry`** (FUN_005b5d90, 33 B) — pack a Win32
`COLORREF` (`0x00BBGGRR`) into one `PALETTEENTRY` (peRed, peGreen,
peBlue, peFlags=0).  Independent of any container.  Used between
the seed step and install step to override or lerp individual
palette entries.

**`ar_palette_install`** (FUN_00491770, 52 B) — lazy-install a
256-entry (1024-byte) palette onto a sprite slot's first entry.
Allocates `s->entries[0].b` on first call; the existing
`ar_sprite_slot_destroy` already frees it iff non-zero, so the
round trip is leak-clean.  The Ghidra decomp's
`*(int *)(*in_ECX + 4)` pattern is `(*this)+4` →
`entries[0].b` of `ar_sprite_entry` — `entries[0].b` is the
owned-pointer half of the entry record.

The third piece — `FUN_004178e0`, "begin palette session" — is NOT
ported.  It needs the whole PE-resource decoder chain
(`FUN_005b7800` + `FUN_005b71f0` + `FUN_005b7c10` + the small
release-helper group + `FUN_005b7b90` RGBA↔BGRA swap).  Blocking
question: which `this` does `FUN_005b7800` actually run on?  The
offsets `+0x3c` / `+0x40` in FUN_004178e0 match `ar_sprite_slot`
(`settings`, `resource_id`), but the bitmap-session layout
FUN_005b7800 needs (pixel buffer at +0x00, palette at +0x34..+0x434)
doesn't fit overlaid on an `ar_sprite_slot` (which has `entries`
at +0x00 and `aux_buf` at +0x34).  Most likely the actual ECX is
reset before the FUN_005b7800 call — the Ghidra decomp drops
__thiscall ECX setups for un-tagged callsites.  Full layout
analysis and resolution path are in
`docs/findings/palette-session.md`.

Tests: +6 (pack basic / top-byte ignore / overwrite; install
lazy-alloc / reuse / destroy round trip).  **117 pass, 0 fail,
3 skip**.  Win32 cross build clean.  Commits: `d3e8a00`, `6db790d`.

---

## 2026-05-24 — Asset-Register: FUN_0057b280 tail (ar_register_locale_sounds + ar_register_aux_sounds)

Closed FUN_0057b280's deferred backlog from the previous checkpoint
— two distinct ports landing in the same session:

**`ar_register_aux_sounds`** — the 4 inline `ar_sound_slot::FUN_00563ef0`
calls the boot driver (`FUN_00562ea0:617-620`) issues between
`FUN_0057a330` and `FUN_0057ca40`.  Hardcoded indices 22..25 with
IDs 0x4cb / 0x4ca / 0x4c8 / 0x4c9 (issue order), count 2 each,
group 2.  Same `ar_sound_slot_init` semantics as the rest of the
sound batches (`load_flag = 0`).  Tests: +3.  Commit: `d4198b0`.

**`ar_register_locale_sounds`** — the conditional locale-table loop
at the tail of retail FUN_0057b280.  Walks the 283-entry rdata
table at `0x00691018` (terminator = first +0x00 == 0 row) and
dispatches into the W_MGR sound pool keyed on three launcher-
settings globals (DAT_008a6e68 / _6e70 / _6e80+0x1c8), now exposed
as an `ar_locale_state` struct.  Two paths:
  - **PATH A (fallback)** when override==0 OR no current locale OR
    launcher flag suppresses it.  Resource id = entry.primary_id;
    settings = (flag==-1) ? locale.fallback : caller's settings.
  - **PATH B (locale override)** otherwise.  override == 0x7fff
    is the skip-when-active sentinel.  Resource id = entry.override;
    settings = locale.current.

Touched indices span 160..464 (267 distinct) — required bumping
`AR_SOUND_SLOT_COUNT` 256 → 512 to fit the 465-entry retail W_MGR
pool (allocated 0x1d1 entries in FUN_00562ea0's SS_MGR_Preparation
block).  Added `AR_SOUND_POOL_COUNT = 465` as the documented exact
retail capacity.

Table data extracted via `r2 px @ 0x691018` + a Python parser on
the resulting hex.  Only the five fields the loop actually reads
are kept in the static const C array; the magic / sequence /
metadata fields are summarised in the table-extraction comment.

Field shape observations from the parsed data (vs the previous
HANDOFF notes that pegged magic as 0xc35a):
- 23 distinct magic values appear in live entries (0xc35a..0xc35d,
  0xc4ae, 0xc754, 0xc756, 0xc760, 0xc77f, 0xc789, 0xc792, 0xc79c,
  0xc80b, 0xc829, 0xc83d, 0xe2a4..0xe2a8, 0x1874e, 0x18755, 0x18759).
  Magic is NEVER read by the loop — likely a zone/area tag for some
  other subsystem.
- field4 (`u32` at +0x04) is a per-locale group selector 1..73 (with
  gaps), monotonic per magic — looks like a "scene_id" the locale
  pre-loader can filter on.
- 15 entries have primary_id == 0 (sentinels skipped by the loop's
  `if (resource_id != 0)` early-out).  15 have override == 0x7fff.
  29 have flag == -1.
- count_add (`i16` at +0x14) is only ever 0 or 2; flag (`i32` at
  +0x18) is only ever 0 or -1; pad16 / field1e are always 0.

Tests: +7 (no-locale path → primary_id + fallback-or-settings,
primary_id==0 skip semantics, launcher_flag forces fallback,
override path under live locale, 0x7fff skip sentinel, coexistence
with `ar_register_game_sounds` at the 160..244 overlap, lazy-load
buffer pointer preservation).  **111 pass, 0 fail, 3 skip**.  Win32
cross build clean.  Commit: `aec8f15`.

---

## 2026-05-24 — Asset-Register: FUN_0057b280 (ar_register_game_sounds)

The "game sounds" boot-register batch — the sixth call in
`FUN_00562ea0`'s asset-register sequence (right after `FUN_0057ca40`,
called as `FUN_0057b280(ZDS, 3, settings)`).  Populates **174
single-slot sound-bank entries** in `g_ar_sound_table[]` covering
pool indices 12..244 (with 59 sparse gaps in that range).  Same
six-field write pattern as `ar_register_sounds` — every entry routes
through `ar_sound_slot_init` since the retail compiler's choice
between inline blocks (122 entries) and `FUN_00563ef0` thiscall
dispatches (52 entries) is observably identical (load_flag=0,
buffer untouched).

The pool-pointer table at `(&DAT_008a6ec4)[i]` only ran out to idx 11
in the previous port (the original AR_SOUND_SLOT_COUNT cap of 12 came
from `ar_register_sounds`).  Bumped `AR_SOUND_SLOT_COUNT` to **256**
(covers FUN_0057b280's max idx 244 with headroom) and renamed the
old 12-cap constant to `AR_SOUND_MAIN_COUNT` so `ar_register_sounds`
still loops over its exact 12-entry roster.  No retail BSS size for
the contiguous pool past 0x008a6ec4 is documented yet; bump again if
a later batch overruns.

Entry data lifted from the Ghidra decomp via a quick regex sweep
(122 `puVar2 = DAT_…; … puVar2[6] = ID;` blocks + 52 thiscall calls);
issue order preserved so any future call-trace test matches without
renormalisation.

**Deferred** — NOT in this port:

1. The 4 inline `FUN_00563ef0` calls the caller (`FUN_00562ea0:617-620`)
   issues at indices 22..25 with group=2 (IDs 0x4c8..0x4cb).  These
   sit between FUN_0057a330 and FUN_0057b280 in the boot sequence and
   write three slots in the "gap" of FUN_0057b280's range; need their
   own tiny helper when the boot driver gets ported.
2. The conditional locale-table loop at the tail of retail
   FUN_0057b280 (walks the 0x24-stride table at `&DAT_00691018`,
   dispatches into the pool keyed on locale state at DAT_008a6e68 /
   _6e70 / _6e80).  This is the language-pack / per-locale sound
   override path — needs reading the structured rdata table at
   0x00691018 and modelling the launcher-settings struct fields the
   branch reads.

Tests: +6 (total-entry-count 174, index range bounds + sample gaps,
field-write spot check across all 5 count buckets {1,2,4,6,8,16},
all-pairs distinct resource IDs, coexistence with `ar_register_sounds`
without group-tag stomping, lazy-load `buffer` preservation on re-
register).  Total **101 pass, 0 fail, 3 skip**.  Win32 cross build
clean.

---

## 2026-05-24 — WndProc: FUN_005b12e0 (wp_handle_message)

Ported the main game window's WndProc — the message handler
RegisterClassExA wires up for the engine's primary window.  The
function is small in code (441 bytes / 84 decomp lines) but
load-bearing: it owns `DAT_008a952c`, the "WM_ACTIVATEAPP wParam
mirror" the engine's outer pump (`FUN_005b1030`) spins waiting for.
The current Frida agent posts a fake `WM_ACTIVATEAPP(TRUE)` to flip
this flag because hidden retail windows don't naturally see the
message from the shell; a correctly-ported WndProc unblocks dropping
that workaround once we own the window registration.

Split into three TUs following the asset-register pattern:

- **`src/wnd_proc.c`** — pure logic, Win32-free.  Decodes the 9
  message classes the dispatch cares about (WM_DESTROY/MOVE/SIZE/
  PAINT/CLOSE/ACTIVATEAPP/KEYDOWN/TIMER + default→DefWindowProc),
  with the WM_ACTIVATEAPP activation half being the meaty branch —
  acquires the "extra" input device (with CP1/CP2 log surround),
  iterates the 2-slot device array, emits the unconditional CP3 log,
  flips the ZDM activation state, then runs the post-activate hook.

- **`src/wnd_proc_win32.c`** — Win32 adapters.  `wp_def_window_proc`
  → DefWindowProcA, `wp_app_exit` → ExitProcess, `wp_log_cp` →
  OutputDebugStringA.  The five "deep engine" hooks (paint_check,
  app_pause, input_acquire, zdm_set_active, post_activate) are
  placeholder no-ops — none of those subsystems are ported yet, but
  swapping each for a real call is a one-line change once they are.

- **`src/wnd_proc.h`** — typedefs Win32 message types as pointer-
  sized integers so the pure logic compiles + tests on Linux.
  Models `wp_app_ctx` with just the fields FUN_005b12e0 reads
  (`f00` head of the device-init pointer chain, `loaded`, `timer`).

The "device init flag" subtlety: retail's activation path computes
`bVar1 = !(ctx->f00 && *ctx->f00 && (*ctx->f00)[+0x18])` then passes
`!bVar1` to ZDM.  I.e. the ZDM arg = "the chain is fully wired".
Disasm at 0x5b13b5..0x5b13c8 + 0x5b1462..0x5b146a confirms this is
literal pointer-deref-pointer-deref + +0x18 read.  Modelled with two
test cases that build the chain explicitly with stack-local int
buffers + a sub-pointer.

Tests: +20 (harmless messages, close→exit, paint short-circuit
combinations, ACTIVATEAPP flag-write semantics, full call-order
spec for the activation path, log-quiet gate, sparse loop, ZDM
arg = init_flag both true and false, timer field-clear, state
reset, layout assert).  Total **95 pass, 0 fail, 3 skip**.  Win32
cross build clean (single-TU mingw picks the new .c files up
automatically).

Not done in this commit: tagging the WndProc's dependency thiscalls
(FUN_005b14c0 / _0058ffa0 / _005ba290 / _005bbd20 / _005b9130 /
_00408b90) in Ghidra.  The script needs each class struct in the
DTM via Parse C Source, and we only modelled one (`wp_app_ctx`) —
the rest are opaque void* in the port.  Defer to a follow-up that
formalizes the input/ZDM/paint-context layouts.

---

## 2026-05-24 — Asset-Register: FUN_0056e190 (ar_register_game_sprites)

The "hundreds of sprites" boot-register batch — the fifth call in
`FUN_00562ea0`'s asset-register sequence (right after
`ar_register_main_sprites`, called as `FUN_0056e190(ZDD, 5, settings)`).
By far the biggest sprite batch at boot: **442 single-entry sprite
registers** packed into a table-driven port that iterates
`ar_sprite_slot_register` once per entry.

The retail decomp is 2782 lines structured as:

- **93 inline blocks** at idx 425..517 (BSS 0x008a7ce4..0x008a7e54)
  — the compiler chose to open-code the destructor + field-write
  sequence rather than emit a call, because the `this` pointer was
  visible as a global.  Resource IDs are sequential 0x592..0x5fb.
  72 use shape (0xa0×0xb0, scale=1, type=0); 21 (resource IDs
  0x71f..0x733, idx 467..487) use (0xb0×0x90, scale=1, type=0).

- **349 trailing FUN_005748c0 thiscalls** with implicit-ECX slot
  pointers that Ghidra dropped from the C view (only the 8
  non-ECX args show up).  Re-extracted the ECX setups via
  `r2 -c 'pD 0x672c @ 0x56e190' | awk '/mov ecx, dword \[0x8a/{last=...} /call 0x5748c0/{print last}'`
  → paired one-to-one with the 349 C-decomp arg lists by file
  order.  Three sprite shapes: 0xa0×0xb0 / 0xb0×0x90 (type=0, scale=1)
  and 0x80×0x80 (type=2, scale=0 — small icon).  Touches 346
  indices in idx 518..863 plus the low-index stragglers at idx 62/63/64
  (resource IDs 0x608/0x609/0x60a, the 0x80×0x80 icon shape).

**Pool refactor**: `AR_SPRITE_SLOT_COUNT` bumped 64 → 1024 to fit
the new high-water-mark (idx 863) with headroom for the remaining
batches (FUN_0057a330, _57ca40, _57b280 likely add a few dozen more
slots).  Retail's contiguous BSS region past 0x8a7640 is plenty
large; storage cost is ~70 KB BSS.

Tests: +6 (inline-block field-map at shape-shift boundaries,
trailing-call shape spot-check across all three shapes + low-idx
stragglers, total slot count = 442, resource-id uniqueness pin
across the whole batch, untouched-indices stay zero, coexistence
with `ar_register_main_sprites`).  Total **75 pass, 0 fail, 2
skip**.  32-bit cross build clean — pool capacity bump verified at
compile time.

---

## 2026-05-24 — Asset-Register: FUN_005749b0 (ar_register_main_sprites)

UI/menu sprite-register batch — the fourth call in `FUN_00562ea0`'s
asset-register sequence (after `FUN_00579bd0`, `FUN_00579a00`, and
the four `FUN_00563ef0` sound-bank loads).  Populates 34 sprite
slots: 9 inline registers, 1 special transient register (idx 0, id
0x90b loaded from sotesp.dll instead of the launcher settings
record), and 24 trailing single-call registers.  Most slots are
640×480 full-screen backgrounds and 368×276 UI panels; the
stragglers at indices 46/47/50/55 are small icons (32×32 / 64×64).

**The ECX-hidden mystery**: the C decomp shows one `FUN_005748c0`
call without an obvious `this` pointer — Ghidra dropped the thiscall
ECX setup.  radare2 disasm at 0x00574e0a reveals
`mov ecx, dword [0x8a7640]` — the slot at DAT_008a7640 (idx 0 in
our unified pool).  Same slot is also the target of the palette
ramp that follows.  So this one slot is BOTH register-populated
AND palette-decorated, while the inline slots and trailing calls
get only the register pass.

**Refactor**: `g_ar_sprite_slots` went from a 2-entry array
(FUN_00579bd0-specific) to a 64-entry unified pool indexed by
`(retail_BSS_addr - 0x008a7640) / 4`.  FUN_00579bd0's two
font-texture slots now live at `AR_SPR_FONT_TEX_457` (= 42) and
`_455` (= 43).  Existing tests updated mechanically.  Future
batches (FUN_0057a330, FUN_0056e190) plug into the same pool —
bumping `AR_SPRITE_SLOT_COUNT` as needed.

**Skipped**: the palette ramp section between the slot-5 and slot-9
inline writes — builds a 256-entry palette via the palette-session
trio (FUN_004178e0 / _005b5d90 / _00491770) and installs it onto
the idx-0 slot.  Documented in the driver docstring; will land
when the palette-session trio + PE-resource decoder do.

Tests: +6 (inline-slots field map, transient sotesp slot, trailing
IDs in index order, untouched indices stay zero, total slot count,
coexistence with `ar_register_fonts`).  Total 69 pass, 0 fail, 2
skip.  32-bit cross build clean.

---

## 2026-05-24 — Asset-Register: FUN_005748c0 (ar_sprite_slot_register)

Exposes the single-entry sprite-slot register as a public helper —
the same shape used by FUN_005749b0, FUN_0057a330, and the hundreds-
of-sprites mega-register FUN_0056e190.  Previously this lived as a
static helper (`ar_sprite_slot_register_init`) inside the module,
parametrized over `entry_count`.  All known retail callers pass
entry_count=1, so the public form hardcodes it — matches FUN_005748c0
exactly (operator_new(8) + 1-entry zero-loop + named field writes).

`ar_register_fonts` now calls the public helper instead.  Field-init
behaviour is unchanged; the test `register_fonts_sprite_slots` still
passes and pins the same slot state.

**Pivot vs handoff recommendation**: deferred the FUN_00563ef0 wave-
load second half.  Per-resource WAVE loading at boot is dead code
(boot batch passes load_flag=0 everywhere), and the dep chain pulls
in DSound vtable mocks + mmio + PE-resource — sizable test scaffolding
for code that doesn't run.  Instead picked the highest-leverage
building block on the title-menu critical path: the per-slot register
that the next three boot-driver calls (5749b0/57a330/56e190) all share.

Tests: +4 (full field-init map, destroy-on-reregister with aux_buf
+ multi-entry array, uint16 truncation, retail call-shape spot check
against FUN_0057a330's first arg list).  Total 63 pass, 0 fail, 2
skip.  32-bit cross build clean — `ar_sprite_slot` still 0x44 B.

---

## 2026-05-24 — Asset-Register: FUN_00579a00 (sound batch)

Second port in the asset-register batch — `FUN_00579a00` registers 12
sound-bank slots at DAT_008a6ec4..6ef0 ("W_MGR" pool).  Adds the
`ar_sound_slot` type (0x18 B; layout asserted) and the matching field-
init helper `ar_sound_slot_init` — which is also the first half of
`FUN_00563ef0` (the boot batch passes `load_flag = 0` so the wave-load
second half is dead code at boot).

The roster is a 12-entry table of (resource_id, count/kind) — 8 kind-2
slots, 4 kind-4 slots, hitting IDs in two ranges (0x506..0x510 and
0x4d8..0x4d9, plus one outlier at 0x903).  Eleven are written inline
in retail; the twelfth (table[8], id 0x50c) dispatches through
FUN_00563ef0 with load_flag=0 — disasm confirms the field-writes are
identical, so the port routes all 12 through the shared helper.

The `buffer` field at +0x04 is the lazy-load "already loaded?"
sentinel.  The init helper deliberately leaves it untouched; the test
`register_sounds_buffer_pointer_preserved` pins this so a future
refactor can't accidentally clobber it.

Tests: +4 (field-init, state clear, full 12-slot roster verification,
buffer preservation).  Total 59 pass, 0 fail, 2 skip.  32-bit cross
build clean.  Cumulative across the session: 13 functions ported into
the Asset-Register module across the FUN_00579bd0 and FUN_00579a00
boot batches.

---

## 2026-05-24 — Asset-Register module: FUN_00579bd0 family

Second ported module lands.  `FUN_00579bd0` is the first asset-register
batch call from the boot driver (after Pixel-Drawer set, before "The
resource was set" log line — see `docs/findings/asset-loader.md`).
Pulls in 11 functions total: the top-level batch, the 9 supporting
slot-setter / GDI-primitive helpers it calls, and the delete-array
thunk underneath everything.

**Module shape (`c4d2da0`)**: two struct types modelling the retail
in-memory slot layouts.  `ar_gdi_slot` (12 B) holds a fixed-capacity
HGDIOBJ array — the shape used by DAT_008a9274[idx] entries 1..15
in the boot batch.  `ar_sprite_slot` (0x44 B) is the sprite-slot
shape from FUN_0056e190 ("hundreds of similar blocks", per asset-
loader.md) — two are touched here (DAT_008a76e8 / _76ec for the font
texture slots).  Layouts asserted with `_Static_assert` blocks live
on the 32-bit cross build.

**Win32 isolation**: `asset_register.c` is pure logic.  The four GDI
primitive wrappers (`ar_gdi_create_font/pen/brush`, `ar_gdi_delete`)
are externs supplied by `asset_register_win32.c` (real build picks
it up via `src/Makefile` wildcard) or by the test harness (recording
stubs that log every call into a per-kind table).  Tests then assert
on call order + arguments without touching real GDI.

**Retail quirks preserved as comments rather than code**: the
`operator_new(4)` leak in `FUN_00579f40` (omitted, ASan-clean and
no observable effect); the asymmetry where `set_font` leaves
`count=0` but `set_pen`/`set_brush`/`set_pen_gradient` bump it;
the middle-loop bound in `FUN_00582d10` that makes gradient
capacities 0/1/2 skip the middle entirely.

**Ghidra recovery gap closed via radare2**: the bottom-block calls
`FUN_0057a030(4,8,0,group)` / a1a0 / a260 had their ECX setup
dropped from the C decompile.  Disasm at `0x579df8 / 0x579e05 /
0x579e1a` shows ECX = `[0x8a9298]` / `[0x8a92ac]` / `[0x8a92b0]`,
which decode to table indices 9, 14, 15.

**Tests**: +24 new (lerp arithmetic incl. descending channels and
alpha skip, GDI destruct order incl. NULL-hole handling, all 7
slot setters individually, `ar_register_fonts` end-to-end on sprite
+ GDI slot indices + call-order verification, layout parity).
Total: 55 pass, 0 fail, 2 skip (skips are the 32-bit-only layout
asserts; they fire at compile time on the cross build).  32-bit
cross build verifies layout parity, both `opensummoners.exe` and
`opensummoners-debug.exe` build clean.

Module is in isolation — not yet wired into the drop-in's boot
path.  Wiring waits until `FUN_00579a00` / `FUN_0057a330` /
`FUN_0056e190` land so calling it has a visible effect.

---

## 2026-05-24 — Test harness scaffold + first ported module (Pixel-Drawer)

Pivoted from "extract assets to spec the format" → "RE the init sequence
and reimplement methodically with tests" (the openrecet model).  Six
commits across one session.

**Harness fix (`8d6855c`)**: the `--max-frames` cap formula in
`frida_capture.py` was `msg_ticks * 250 >= max_frames`, which at the
default `max_frames=600` fired at just 3 emitted batches (~750
drained messages) and pre-empted any `--duration-ms` longer than
~12 s.  Now compares against the true running count carried on each
`msg` event; default bumped to 30000.

**Sprite format spec (`a2e5cb0`)**: archaeology pass on the
`sotesd.dll` DATA blobs spec'd the `0x425f` sprite family layout
(32 B outer magic + 1024 B BGRA palette + 64 B sub-table + 14 B
BMFH-style preamble + 8 B sub-sig + W×H 8bpp pixels).  213 of 759
DATA blobs parse cleanly.  W/H aren't in the file — they come from
`FUN_0056e190`-family registration calls.  Extractor at
`tools/extract/lizsoft_sprite.py`.  User redirected away from
chasing the asset extraction further: "we will load sprites the
same way as retail exe does it" once the init replay catches up.

**Title-scene runner (`07088a7`)**: `FUN_0056aea0` mapped fully.
8-phase intro animation (studio-logo fade → title fade → "press
button" → particle hand-off), pump+frame-budget cadence, the
default-branch menu-action latch via `FUN_0043ce50`, lazy DInput
pad attach on first menu-confirm.  Also extended
`winmin-and-bootstrap.md` with the state-code → next-scene map for
the outer driver's 0/6/8/9/0x1a..0x1e codes, and the Pixel-Drawer
slot-table allocation phase (69 slots in 5 fixed-size groups).
This is the first-rendered-frame bridge from boot done to actual
DDraw Flip.

**Test harness scaffold + Pixel-Drawer leaf primitives (`a53c141`)**:
`tests/{t.h, test_main.c, Makefile}` mirroring openrecet's pattern —
host gcc + ASan/UBSan, X-macro registry, `T_ASSERT_*` macros,
name-filter via `$F`.  Ported the 5 leaf primitives of `FUN_005bd*`:
mask→shift encoder, channel ctor, channel free-LUT, slot ctor, slot
SetColor.  13 tests; layout parity enforced via `_Static_assert`
blocks active on the 32-bit cross build.

**Pixel-Drawer LUT builder (`bb8c706`)**: `FUN_005bd040` (801 B,
four blend formulas + shared-LUT short-circuit).  Modes: 1=add,
2=sub, 3=lerp-variant-A, 4=channel-weight-coupled, default=lerp.
Floor-correction terms preserved literally even where they're
always-zero for valid weights, in case the engine ever passes
out-of-range inputs.  10 new tests with hand-computed expectations.

**Pixel-Drawer slot commit + mask reader (`aa0e62c`)**: `FUN_005bd3d0`
ties the leaf primitives together (free LUTs → resolve format from
PdFormat or RGB565 default → encode masks → resolve slot.state →
rebuild LUTs in R/G/B order with B sharing R-not-G).  The 8-byte
sub-detail that "B can share with R but never with G" preserved
verbatim from the retail call sequence at 5bd456/45f/468.
Pixel-Drawer module is now complete: 7 functions ported, 30 tests
passing on host (1 layout-test host-skip), 32-bit cross build clean.

Status: test harness is established and the first complete module
is in.  Next sessions should look at the remaining boot-driver
phases that have clear consumer relationships — likely the
ZDD wrapper (DDraw surface mgmt — Win32 heavy, needs mock layer or
just port + verify via the smoke harness) or the asset register
batch (`FUN_00579bd0` fonts, `FUN_0056e190` sprite slots et al —
consumes Pixel-Drawer slots so it integrates our work directly).
Skip the SS_MGR/W_MGR/GD_MGR boot pools until we know their
consumer semantics — they're just `operator_new` loops in
isolation.

---

## 2026-05-24 — Phase 1+2 push: audio, asset loader, config.dat, DDraw surface builder

Long unattended session.  Six commits, four new findings docs, two
new extractors.  Highlights:

**Audio + Input init (`docs/findings/audio-init.md`):** corrected the
prior mis-labeling of `FUN_005b9fc0` as "wave audio" — it's actually
the DInput keyboard sub-device, following `FUN_005b9cf0` (ZDI main /
`DirectInputCreateEx` with version `0x0700`).  DSound primary buffer
is created with `DSBCAPS_CTRLVOLUME` so the engine can master-attenuate
via `SetVolume`.  The launcher's "Disable Sound" gates ZDM (music
mgr) init only — DSound still inits either way.  ZDM allocates 50 ×
576-byte voice slots.  DInput is loaded by `Ordinal_1()` (= legacy
`DirectSoundCreate`), not by name — a quirk in `FUN_005bb180`.

**DDraw surface alloc (`docs/findings/ddraw-init.md`):** decompiled
`FUN_005b95c0` + `FUN_005b8c00` — the actual `IDirectDraw7::CreateSurface`
path.  Identified the engine's `0x01FFFFFF` "no color key" sentinel
(was previously mis-read as an "unlimited hint"), the 24bpp→32bpp
case fallthrough in the pixelformat switch, and corrected the
IDirectDrawSurface7 vtable cheat sheet (Lock at offset 0x64 not 0x60,
Unlock at 0x80 not 0x7c — 0x7c is SetPalette).

**Asset loader (`docs/findings/asset-loader.md` + `tools/extract/sotes_resources.py`):**
the three companion DLLs are pure resource-only PEs.  Wrote a
zero-dep PE-resource walker that dumps every entry to `type=<T>/<ID>.bin`
with a manifest.  Real content map:

- **sotesp.dll**: 31 WAVE SFX + signature blob
- **sotesw.dll**: 47 WMA music files (in `DATA` type, despite the
  earlier "MUSICWMA" speculation)
- **sotesd.dll**: 759 DATA blobs (~135 MB) + 436 WAVE SFX (~26 MB)

`FUN_005b6340`'s "kind 2 source" turns out to be a chunked-memory
abstraction (`FUN_005b67c0` spans 676996-byte chunks) — NOT
decompression as initially guessed.  This means sotesd 1000–1004
(each exactly 676996 bytes) is one logical 3.4 MB blob assembled
at boot.  Assets are stored RAW.  Sample DATA inspection shows
Lizsoft sprites have a 32-byte header + 256-entry BGRA palette
+ pixel data.

**Signature integrity checks (new engine-quirk §13):** all three
DLLs carry the same byte-encoded ASCII signature scheme.  Each
DLL has a resource that decodes (byte + `0x41`) to a known string:

| dll          | resource ID  | signature                                                    | min ver |
|--------------|--------------|--------------------------------------------------------------|---------|
| `sotesd.dll` | 0x7DE (2014) | `JFDGGIUABCVJIEKAUYLPOFDEQBVGSKOLJSCKPIFAXMHGYELSDOBFRKVGBAKB` | 0x2713 |
| `sotesw.dll` | 0x40F (1039) | `MUSICWMA`                                                   | 0x2712  |
| `sotesp.dll` | 0x407 (1031) | `FSPATCHR`                                                   | 0x2711  |

Our drop-in port can no-op these — they're integrity seals for the
ship-time DLLs, irrelevant when the user provides their own legit
copies.

**config.dat extractor (`tools/extract/config_dat.py` +
`docs/formats/config-dat.md`):** 16-byte plaintext header + 820-byte
XOR-obfuscated body (key `0x88`, confirmed by abundance of
`88 88 88 88` runs).  Body parses as one leading u32 + 102 `(u32,u32)`
pairs (`field_id`, `value`).  Field-ID semantics TBD but pattern is
clearly a typed key/value store matching the engine's `FUN_005afb90`
schema-registration with 101 fields.

**Harness turbo fixes (`tools/frida/opensummoners-agent.js`):**
GetTickCount virtualization (gated on first PeekMessage entry to
avoid pre-pump init livelock), WaitMessage stub (main-thread only),
Sleep → Sleep(0) (yield not noop, so background threads don't
starve), and PostMessage WM_ACTIVATEAPP(TRUE) to the main game
window as soon as the periodic scan finds it (without this the
pump spins on `DAT_008a952c == 0` forever because hidden windows
don't naturally receive the activation message).  Also corrected
the WndProc/class doc — `0x401210` is `CLASS_LIZSOFT_WAIT` (the
"Please wait." splash), the main game window is
`CLASS_LIZSOFT_SOTES` with WndProc `0x5b12e0`.

Engine quirks file grew from 8 entries to 14, with the most
load-bearing additions being §10 (WM_ACTIVATEAPP gating) and §13
(the three-DLL signature scheme).

Status: phase 1 surface mapping complete.  Phase 2 file-format
extraction started with config.dat and the resource walker.  Next
session is likely the Lizsoft sprite format spec + the chunked
sotesd 1000-1004 blob identification (needs DDraw Lock-hook capture
of a known sprite, then byte-diff against the extracted DATA bytes).

---

## 2026-05-24 — Harness turbo fixes + WndProc-class correction

Phase 1 surface mapping (the previous entry) flagged three TODOs that
this push addressed in `tools/frida/opensummoners-agent.js`:

1. **`GetTickCount` virtualization.**  Replaces `timeGetTime` as the
   simulation clock — Fortune Summoners never imports `timeGetTime`.
   The hook is gated by `g_pump_entered`: pre-pump init has busy-waits
   that livelock if the clock jumps 17 ms per call instead of advancing
   with real wall-clock.  After first `PeekMessageA` entry from main
   thread, the virtualization activates.
2. **`WaitMessage` stub** (main-thread only).  The pump uses
   `WaitMessage` to yield between frames; with virtual clock the OS
   timer never fires, so `WaitMessage` would hang.  Stub returns 1
   immediately on the main thread; background threads keep real OS
   semantics (audio mixer, file I/O may use `WaitMessage` for real
   waits).
3. **`Sleep` → `Sleep(0)`** instead of true no-op.  True-noop starves
   background threads of CPU, and the main thread often polls flags
   set by exactly such threads.  `Sleep(0)` yields the timeslice
   without actually sleeping — fast enough for turbo, correct enough
   for background work.

Discovered in the process: a hidden window never naturally gets
`WM_ACTIVATEAPP` from the OS, and `FUN_005b1030`'s spin loop only
breaks when `DAT_008a952c != 0` — which is set by the WndProc on
`WM_ACTIVATEAPP`.  Fix: agent posts `WM_ACTIVATEAPP(TRUE)` to the
main game window as soon as the periodic scan finds it
(`installPeriodicWindowScan`).  Without this, `msg_ticks` stayed at
0 forever with `--turbo --hide-window`; with it, pump enters and
the engine progresses into per-scene loops.

Also folded in a WndProc-class correction.  The Phase 1 notes claimed
the main game window's WndProc was `0x401210`; that's actually the
**`CLASS_LIZSOFT_WAIT`** ("Please wait." splash) WndProc.  The main
game window uses **`CLASS_LIZSOFT_SOTES`** with WndProc `0x5b12e0`
— a 441-byte handler that includes the load-bearing `WM_ACTIVATEAPP`
case plus `WM_CLOSE → ExitProcess(0)`.  Both classes are registered
in `FUN_005a4770`, sites `0x5a4ca8` and `0x5af314` respectively.
The `0x5b12e0` site does `mov dword [esp+0x50], 0x5b12e0` (lpfnWndProc
slot in WNDCLASSEXA at offset 8) — visible at `0x5af2c7`.

Quirks doc grew §9 (two WndProcs / two classes), §10 (WM_ACTIVATEAPP
as load-bearing pump-unlock), §11 (function-pointer-only callbacks
that Ghidra misses).  Engine-bootstrap doc updated to document both
WndProcs and the harness fix recipe.

Status: harness now reaches per-frame ticks in `--turbo --hide-window`
mode.  Steady-state frame rate still partial — `msg_ticks` reaches
~250 in some runs and 0 in others within a 30 s window, suggesting
init-phase race conditions remain (likely asset loading from
`sotesd.dll` / `sotesw.dll` — Phase 2 work).  Good enough to land as
a checkpoint; remaining bring-up TBD as the asset-loader RE goes.

---

## 2026-05-24 — Bootstrap (Phase 0)

Initial commit run.  Set up the project shape: nix flake with mingw-w64
i686 cross compiler + Ghidra + Frida-tools + Python (pillow/numpy/
sk-image/opencv/construct/rich/frida-python), `.editorconfig`,
`.gitignore`, MIT license, README.

`tools/setup.sh` — symlinks the user's Steam install of Fortune Summoners
into `vendor/original/`, detects Steam DRM by checking for a `.bind`
section in `sotes.exe`, runs Steamless via WSLInterop, and stashes the
unpacked binary in `vendor/unpacked/sotes.unpacked.exe`.  First run:
Steamless identified SteamStub Variant 2.1 and unpacked cleanly.
Original SHA: `7d779f2eb02b3c603857fedbc52be6973ac3b0b2c5c1bc696122ddac89fb9f1b`,
unpacked SHA: `9e032483b9981f73cabb83baca17a734fd9e7c41e114703900d9ee82c7969516`.

`tools/launcher/opensummoners-launcher.exe` — Job-Object supervisor copied
verbatim from OpenMare.  Guarantees no orphaned Windows-side .exes after a
SIGKILL'd WSL run.  Same `--timeout-ms` / `--grace-ms` / `--no-stdin-watch`
flags as the sibling.

`src/main.c` + `src/dev_hooks.c` — WinMain skeleton with the four
drop-in defaults the user asked for from day one:
  1. Auto-cd into `OPENSUMMONERS_GAME_DIR` + `SetDllDirectoryA` to the
     same, so any later `LoadLibrary` resolves game-dir DLLs first.
     `OPENSUMMONERS_GAME_DIR` is exported by the flake's shellHook with
     `WSLENV=…/p` so the .exe sees the Windows-form path.
  2. `user32!MessageBoxA/W` prologue patch that redirects every modal
     to stderr with a distinctive `[!!! REDIRECTED MESSAGEBOX !!!]`
     banner and auto-returns IDOK.  Override with `--show-msgbox`.
  3. Single-instance mutex (`OpenSummoners-SingleInstance`) catches
     stray .exes from previous SIGKILL'd runs.
  4. `--hide-window` / `--frames N` flags for harness/smoke runs.
Single-TU build (per the user's preference), two outputs:
`opensummoners.exe` (GUI subsystem) and `opensummoners-debug.exe`
(console subsystem, stderr surfaces in the launching shell).

`tools/ghidra-headless.sh` + `tools/ghidra-scripts/ExportDecompiledC.java`
— batch decompiles `vendor/unpacked/sotes.unpacked.exe` to
`docs/decompiled/` (gitignored).  Java post-script because nixpkgs'
Ghidra isn't built with PyGhidra.  First-run analysis kicked off in
background while we wrote the rest of Phase 0.

`tools/frida/opensummoners-agent.js` + `tools/frida_capture.py` — Phase A
Frida harness.  Hooks:
  - `MessageBoxA/W` → redirect to `send({kind:"messagebox",...})` +
    auto-IDOK (mirrors the dev_hooks.c hook on the drop-in side).
  - `ShowWindow` / `ShowWindowAsync` / `SetWindowPos(SWP_SHOWWINDOW)`
    → force hidden for HWNDs we tracked from CreateWindowEx returns.
  - `PeekMessage*` / `GetMessage*` onLeave → tick a coarse frame counter.
  - `Sleep` → no-op (turbo).
  - `winmm!timeGetTime` → virtualised clock for the main thread only
    (turbo simulation speedup, not just loop-iteration speedup).
  - `waveOutSetVolume` → clamp 0 (silent audio).  DSound layer deferred
    to Phase B once Ghidra confirms the engine's COM init path.
All flags default ON per the user's instruction ("hidden window with
muted audio running in turbo mode as early as possible").

`tools/run-opensummoners.sh` + `tools/run-retail.sh` — single-source-of-
truth dev-loop recipes so the build / run / launcher / harness flags are
consistent every time.  No re-discovering gotchas per session.

Smoke verification:
  - `run-opensummoners.sh` end-to-end: launcher → debug.exe
    `--hide-window --frames 200` runs in ~3.2 s (16 ms × 200), MessageBox
    hooks both succeed (`@ 745d6e60` / `@ 745d7380`), init_game_dir cd's
    into the Windows-form game path, exit rc=0.
  - Retail smoke under Frida: green.  The frida-server.exe runs on the
    Windows host as `cutestation.soy:27042` (the host's LAN-resolvable
    name; WSL2 NAT doesn't loop back to 127.0.0.1).  Updated the flake's
    default + `frida_capture.py` to match — 127.0.0.1 was the wrong
    default the OpenMare sibling carried forward.

Discoveries (folded into agent code and findings docs as we hit them):
  - **sotes.exe is SteamStub Variant 2.1 packed.**  Spawning the
    on-disk exe outside the Steam process tree trips the DRM check
    (`Steam Error: Application load error P:0000065432`).  Fix:
    `tools/run-retail.sh` copies vendor/unpacked/sotes.unpacked.exe into
    the game dir as `sotes-unpacked-<pid>.exe` per run (needed alongside
    the engine DLLs so Windows DLL search finds sotesp/d/w).
  - **Frida 17.x API surface differs.**  `Module.findExportByName(modName,
    exp)` static method removed → use
    `Process.findModuleByName(name).findExportByName(exp)`.
    `Memory.readUtf8String(ptr)` removed → use `ptr.readUtf8String()`.
    Hooks attached while the process is suspended sit deferred until
    `Interceptor.flush()` — without that, all our installs no-op'd
    silently.  Use `Process.mainModule` instead of name-matching since
    the spawned exe is named per the temp filename.
  - **The engine launcher is a Win32 #32770 modal dialog**, NOT a
    `MessageBox`.  Created by `DialogBoxParamA(hInst, 0x2711, NULL,
    dlgProc=0x004013c0, 0)`.  The dialog manager bypasses public
    `CreateWindowEx` / `ShowWindow` / `SetWindowPos` exports.  We
    initially caught it via a periodic `EnumWindows` scan + force-hide,
    but the OS painted it before our 8 ms scan tick — a brief flash
    appeared on the user's desktop.

Final fix (silent boot achieved 2026-05-24):
  `installDialogBypass()` in `tools/frida/opensummoners-agent.js`
  hooks `DialogBoxParamA` and replaces the engine's DLGPROC (arg 3)
  with a Frida `NativeCallback` wrapper.  On `WM_INITDIALOG`:
    1. Call original handler (loads saved settings into controls).
    2. Force-check Windowed Mode (ctrlId 10020) + Disable Sound
       (ctrlId 10024).
    3. `SendMessage(LaunchBtn, BM_CLICK)` synchronously — the engine's
       IDOK handler reads control state, persists, calls EndDialog.
    4. Return original result.
  Because `EndDialog` has been called before `WM_INITDIALOG` returns,
  the dialog manager skips its post-INITDIALOG ShowWindow step.  User
  confirmed "absolutely nothing" on screen.

Status of the harness:
  - Spawn retail headlessly under Frida → init agent → resume → engine
    boots silently through its launcher → reaches the main game window
    (`CLASS_LIZSOFT_SOTES`) within a few seconds → harness teardown
    via `device.kill(pid)`.
  - msg_ticks stays at 0 in the smoke summary — the engine reaches
    main window creation but doesn't enter its PeekMessage loop in 8 s.
    Probable additional bring-up phases (DirectDraw surface alloc,
    asset loader) gate the main loop; revisit when tracing the boot
    chain.

Ghidra batch decompile finished: 1768 functions written to
`docs/decompiled/` (gitignored).  First useful query already paid off
— `grep DialogBoxParam` immediately pointed at the dialog call site
and DLGPROC address.

Next session — Phase 1 priorities:
  1. Read DLGPROC at `0x004013c0` and its caller to understand
     `gl.cfg` (or wherever settings persist) layout.  This is the
     first thing the engine writes; spec it and we have an extractor.
  2. Find and document `WinMain` + the main loop + frame limiter
     (mirror OpenMare's `winmain-and-bootstrap.md`).
  3. Identify the DirectDraw 7 init path (`DirectDrawCreateEx` →
     `IDirectDraw7::SetCooperativeLevel` → primary surface alloc).

---

## 2026-05-24 — Phase 1 surface mapping (#1)

Three findings docs landed in one session, covering the three
priorities the prior entry queued up.  All entries cross-link, and
`engine-quirks.md` grew four new items folded in along the way.

`docs/findings/launcher-dialog.md` — full reverse of the launcher
DLGPROC at **`0x004013c0`** plus its sibling helper `FUN_00401730`.
Ghidra missed both because they're only reached via function
pointers; disassembled with `radare2 -c 'af; pdf'`.  The proc handles
just `WM_INITDIALOG` and `WM_COMMAND`; click on Launch (`ctrlID 10003`)
sets `DAT_008a9a40 = 1` and scrapes the four radio/checkbox groups
into `DAT_008a9b48/4a/4c/4e` (screen mode / VRAM / quality / disable
sound).  Engine quirk: **radio enums start at 3, not 0** — saved
file values are 3/4/5 per group.  Engine quirk: control `0x272A`
(Zoom 1920×1440) is unconditionally `ShowWindow(SW_HIDE)`'d at
`WM_INITDIALOG` — exists in the dialog resource but the user never
sees it.  Engine quirk: three controls (`0x271C-0x271E`) are
`EnableWindow(false)`'d on every init with no path to re-enable.

`vendor/original/user/config.dat` (840 bytes) is XOR-obfuscated with a
**16-byte plaintext header** (`hdr=16`, `ver=0x2711` matching the
dialog resource id, `data_size=820`, checksum) followed by 824
obfuscated bytes.  Key byte `0x88` — confirmed by the dead-obvious
runs of `88 88 88 88` (zero plaintext).  Format spec deferred to
Phase 2 `docs/formats/config-dat.md` once we wire the extractor.

`docs/findings/winmain-and-bootstrap.md` — full call graph from
`entry @ 0x5c0a8f` through `WinMain @ 0x562210` and the post-launch
driver `FUN_00562ea0`.  Mapped:
  - **WndProc @ 0x401210** (missed by Ghidra — pointer-only ref).
    Only handles `WM_PAINT` (loading-screen text + frame blit);
    everything else delegates to `DefWindowProcA`.  No `WM_CLOSE`
    handler — click-X just destroys the window without `WM_QUIT`,
    hanging the process.
  - **Message pump + frame limiter at `FUN_005b1030`**:
    `PeekMessageA` → if `WM_QUIT` (0x12) → `ExitProcess(0)`;
    `WaitMessage` to block on a `SetTimer(hWnd, 1, 10ms, NULL)`
    that's installed in `FUN_00562ea0`.  Frame-readiness flag at
    `state->[0x1c]` is set when `GetTickCount - last_tick < 5` ms.
  - **Class registration**: `RegisterClassExA` inside the 46 KB
    `FUN_005a4770` at `0x5a4ca8` — `CLASS_LIZSOFT_SOTES`, style
    `CS_HREDRAW|CS_VREDRAW`, WndProc `0x401210`, default arrow cursor.
  - **No global main loop** — each scene function runs its own
    pump+tick loop until it returns a state code to the outer scene
    state-machine in `FUN_00562ea0`.  Scene code = 9 means
    "restart game", caught by WinMain's `do…while`.

Critical insight for the Frida harness: **the engine uses
`GetTickCount` exclusively** — `iiq~timeGetTime` on the unpacked
binary returns nothing; the timeGetTime hook our agent inherited
from openrecet/OpenMare is a no-op here.  We need to add
`GetTickCount` virtualization + a `WaitMessage` stub to actually
achieve turbo speed.  TODO in the agent.

`docs/findings/ddraw-init.md` — DirectDraw 7 init flow:
`FUN_005b7ee0` (ZDD wrapper ctor)  →  `FUN_005b88c0`
(`DirectDrawCreateEx(NULL, &ddraw7, &IID_IDirectDraw7, NULL)` —
IID at `DAT_00850eb0`) → `FUN_005b89d0` (`SetCooperativeLevel`
with `DDSCL_EXCLUSIVE|FULLSCREEN|ALLOWREBOOT = 0x13` in fullscreen
or `DDSCL_NORMAL = 8` windowed) → `FUN_00582e90` (CreateScreen
mode dispatch — calls `FUN_005b8b40` which builds DDSURFACEDESC2
+ `IDirectDraw7::CreateSurface`) → `FUN_005b9520` (clipper create
+ attach to primary surface).  Catalogued the vtable offsets for
`IDirectDraw7` / `IDirectDrawSurface7` / `IDirectDrawClipper` so
the Phase-A `Lock`/`Flip`/`Blt` hooks land at the right offsets.

Two follow-ups recorded in the new docs for the next push:
  - **Decompile `FUN_005b95c0`** (the DDSURFACEDESC2 builder) when
    we move on to the renderer port — easier than chasing the
    46 KB `FUN_005a4770`.
  - **Add `GetTickCount` + `WaitMessage` hooks** to
    `tools/frida/opensummoners-agent.js` so turbo actually works.

Suggest `/clear` before the next subsystem (likely audio/DSound,
the asset loader, or the renderer port).  The Ghidra reads in this
session pulled in a lot of context that the next milestone won't
need.

---

## Checkpoint 13 — TAS framework: retail capture + deterministic input injection

Mirrored openrecet's TAS harness for OpenSummoners. Two new self-serviceable
Frida capabilities, both **validated live against retail**, plus the scenario
layout and the recovered new-game scene flow.

**Frame capture** (`tools/run-retail.sh --no-turbo --capture-frames "…"`):
at the Flip hook, walk the engine god object to the DDraw render-target
surface (`*(0x8a93cc)->[0x16c] paint_ctx ->[0x2c]`), GetDC + BitBlt into a
24bpp top-down DIB (GDI does the RGB565→BGR convert), read the bits, send to
the driver which writes lossless PNG (`runs/<dir>/frames/frame_NNNNN.png`).
Surface chain + vtable indices pinned by r2 disasm of the GetDC wrapper
`FUN_005b94e0` (`mov eax,[ecx+0x2c]; call [[eax]+0x44]`). First validation:
8 frames of the title boot (studio splash → logo → full menu), 640×480, colors
correct.

**Input injection** (`--input-trace <file.jsonl>`): a hidden window makes
DInput silent, so the engine's 64-slot event ring is ours to fill. Hook the
poll consumer `FUN_0043c110` (ecx = current scene's input mgr); per Flip frame,
write synthetic records `{id, ts, flag=1}` into the newest ring slots for the
ids a sparse `{frame, ids}` trace schedules. Validated: a single trace clicks
the entire **NEW GAME** path — title `Start` → difficulty config menu →
DOWN×2 → `Start Game` → confirm → Elemental Stone intro → prologue narration —
fully deterministic, captured frame-for-frame and confirmed visually.

Three findings landed (engine-quirks **#42/#43**; `input.md` corrected):
  - title nav button ids ≠ their latch-dir names: **up = id 1, down = id 3,
    confirm = id 0x24**; ids 2/4 are page up/down (no-ops in single-column
    menus). Diagnosed by hooking `menu_list_latch` (`0x43ce50`) live and
    seeing `ready=1000` yet no cursor move on id 2.
  - the injected record's `ts` must be the engine's **per-frame cached `now`**
    (the poll's first arg), not a fresh `GetTickCount()` — else `(now-ts)`
    underflows the 100 ms recency window and the press is silently dropped.
  - each scene has its **own input-manager instance**; inject into the current
    poll's `ecx`, never a cached one (the difficulty menu uses a different mgr
    than the title, and polls a different id set `0x22,1,3,0x24,0x27`).

New: `docs/plans/tas-framework.md`, `docs/findings/new-game-flow.md`,
`tests/scenarios/{title-idle,new-game-through}/` (openrecet layout).
Visuals pushed to llm-feed (title boot montage; new-game click-through montage).

> ⚠ Always launch retail via `tools/run-retail.sh` (unpacked exe), never
> `frida_capture.py` directly (the default `vendor/original/sotes.exe` is the
> Steam-DRM-packed image → stalls + orphan window).

Gaps for next time: prologue→first-playable-map needs a recorded human trace
(distil to sparse) or RE of the prologue sequencer; port-side
`input_trace.{c,h}` + port frame capture are latent (blocked on milestone-0
rendering). The harness is now the **yardstick** for the render-bridge port
(HANDOFF Next move #1): once the port renders, capture port frames the same way
and diff vs these retail goldens.

---

## 2026-06-02 (ckpt 21) — the title render sink: cmd stream → real ZDD blits

`src/title_sink.{c,h}` (+ `tests/test_title_sink.c`, 13 tests): the runtime
bridge that turns `title_render_step`'s abstract `TITLE_DRAW_*` command stream
(the render half of `FUN_0056aea0`, behind `title_render_sink_hook`) into the
retail render half's actual ZDD calls, against a bound primary surface. This
is the "sink" half of HANDOFF Next move #1 (the drive half is ckpt 22).

**RE landed this ckpt** (render half `0x56bb04..0x56bf1a`, r2): every per-phase
draw resolves its source frame out of ONE of two fixed sprite banks, then blits
onto `DAT_008a93cc->[0x16c]` (= the ZDD `primary_obj` at +0x16c):
- **MAIN bank** = `0x8a7658` = pool slot **19** (`ar_pool_get_slot(19)`; main
  pool base `0x8a7640` + 6·4). Carries the studio/title logos (frames 1/2),
  the press-button sprites (2/3/4), the sparkle (4/5), the menu background +
  menu sprite (5/6). The cmd `asset` is the `ar_sprite_slot_frame` frame id.
- **CURSOR bank** = `0x8a765c` = pool slot **20**. The menu-selection
  highlight; frame id = the selected row index.

Both self-decode via the ported `ar_sprite_slot_frame` chain (ckpt 16/20) —
but stay NULL until the slot is registered with a real "DATA" resource AND the
8d surface builder (`ar_frame_build_hook`) is wired, so every sprite op no-ops
faithfully (the "still-undecoded" path). ⇒ the drive will render a cleared +
flipped window with no sprites yet (move B), giving 8d a frame-diff harness.

**Faithful + host-tested:** `SURFACE_RESET` (→ `zdd_object_clear`),
`SURFACE_CLEAR`/`SPRITE` (keyed blit of `frame(main,asset)`→primary),
`SPRITE_LEVEL` (→ `title_draw_sprite_level`, ramp_b plain/alpha both proven),
`FRAME_END` (→ `title_compositor_draw` of the bound display-list group),
`FLIP` (present cb), `LOG_FLIPPING` (log cb) — the whole intro + menu-background
+ fade-out path.

**Deferred behind ctx callbacks** (no-op default): `LOGO`, `SPARKLE`,
`MENU_CURSOR` — the alpha-ramp draws whose blend-descriptor *pointer* rides the
32-bit `alpha` field of `title_draw_cmd` (can't round-trip on a 64-bit host) or
whose level numerator (`[esp+0x20]`) + fixed src geometry the command drops.
They only fire once the run-time ramp tables (`0x8a92b8`/`0x8a9308`) are
populated, which never happens at a cold headless boot, so deferring costs no
intro/menu-background fidelity. Wired + validated against live goldens in the
drive checkpoint.

**Fidelity fix to `title_render_step`:** `TITLE_DRAW_SURFACE_CLEAR` now carries
the source frame index in `asset` (prologue background = 0; the logo handler's
alpha-0 path = `frames[1]` studio / `frames[2]` title). The alpha-0 logo blit
is `frames[1/2]` (`0x56bba0`/`0x56bc19`), NOT the phase-2..3 background
`frames[0]` — the abstract op was previously lossy across the two.

**617 host tests (611 pass, 0 fail, 6 skip; +12 this ckpt).** Both 32-bit
cross-builds clean. Ledger unchanged at **130/1490 (8.1%), 127 tested** — the
sink bridges already-counted functions (no new `FUN_` port tokens).

---

## ckpt 25 — 8d ported in full (per-cell builder + format converters + slicer) + wired live (2026-06-02)

The genuine sprite pixel source is **ported, host-tested, and wired into the
live drive** in three commits:

**25  (`d82af11`) — 8d core in `src/zdd.c`:**
- `zdd_object_new_cell` (FUN_005b9280) — operator_new + ctor + orchestrate + publish.
- `zdd_object_build_cell` (FUN_005b9630) — trim-gate (count>1 tightens to the
  opaque bbox; found_key==0 drops the colorkey; found_opaque==0 → metrics-only,
  no surface) → `create_surface_pair` → pixel writer.
- `zdd_object_copy_cell_pixels` (FUN_005b9910) — **raw bottom-up byte blit**
  (NOT a format converter — see below); zero-fills the locked dest then row-copies
  the WxH window, clamping the per-row span to dest pitch + source stride.
- Verified against `docs/decompiled/by-address/5b9280.c`, `5b9630.c`, all.c 5b9910.
- 10 host tests. Ledger 131→134.

**25b (`16e3a18`) — format converters in `src/bitmap_session.c`:**
- `bs_convert_to_16bpp` (FUN_005b7310), `bs_convert_8bpp_to_24bpp` (_74f0),
  `bs_convert_24bpp_to_32bpp` (_7270), `bs_load_palette_from` (_7bd0).
- The slicer picks one by display depth ([zdd+0x168]); windowed = 16bpp, so a
  24bpp title sheet packs to RGB565 via the ZDD shift descriptor. 7 host tests.
  Ledger 134→138.

**25c (`03d33c1`) — slicer body + live wiring:**
- `ar_sprite_slice` expanded to the full FUN_004188b0: trim-scan loop (into
  `slot->aux_buf`) + format switch (new `ar_sheet_format_hook`) + trim-aware
  build loop. Verified vs `by-address/4188b0.c`.
- `main.c` adapters bind the three hooks to the live ZDD: `title_frame_build`
  → `zdd_object_new_cell`, `title_frame_free` → `zdd_obj_destroy`,
  `title_sheet_format` → `bs_convert_*` by `g_zdd->pixel_format_bpp`.

**Two corrected findings → engine-quirks #49 (format converters live in the
SLICER, not the pixel writer — the writer is a raw byte copy) and #50 (the
slicer passes (cell_w, cell_h) as the trim scanner's (height, width) args).**

**LIVE (self-serviced):** port boots windowed at **depth=16bpp**, drives the
title scene, flips, **zero DDERR, zero crashes through 900 frames**, clean
60fps. 8d is crash-clean against real DDraw.

**Gap to *visible* sprites (next session, NOT 8d):** the title banks (pool
19/20) are never registered at boot, so `ar_sprite_slot_frame` returns NULL
(entries==NULL) and the decode→slice→8d chain never fires.
`ar_register_main_sprites` exists but needs the **launcher settings record**
(the PE resource source) + sotesp module — a separate
asset-registration/launcher subsystem. Plus the sink's LOGO/SPARKLE/MENU_CURSOR
arms are still deferred no-ops and the cold-boot intro must reach the menu phase.

**647 host tests (0 fail, 6 skip; +17).** Both 32-bit cross-builds clean.
Ledger **138/1490 (8.6%), 135 tested (+7)**.

---

## ckpt 26 (2026-06-02) — THE TITLE SCREEN RENDERS (bank registration + frame capture)

**The payoff checkpoint.** Registering the title sprite banks at boot lit up the
whole ported render pipeline. The drop-in now decodes the real Fortune Summoners
title art from sotesd.dll and blits it to screen.

**The fix** (`src/main.c`, commit `e00718b`): `init_sprite_banks()` —
`LoadLibraryA("sotesd.dll")` + `ar_state_init()` +
`ar_register_main_sprites(g_zdd, /*group=*/4, hSotesd, hSotesd)`, called between
`init_ddraw` and `init_title_drive`. `FreeLibrary` on shutdown. The pool slots
19/20 the sink reads (`AR_SPR_TITLE_MAIN`/`_CURSOR` → `g_ar_sprite_slots[6]`/`[7]`
= ids 0x91b/0x91c) are exactly the ones `ar_register_main_sprites` populates, so
the getter stops short-circuiting to NULL and the decode→slice→8d→blit chain runs.

**RE findings that unblocked it (engine-quirks #51):**
- `bs_decode_resource`/FUN_005b7800 takes the HMODULE **directly** — no `+0x3c`
  record indirection. `settings` is literally an HMODULE.
- That HMODULE is **sotesd.dll** = `DAT_008a6e74` (stored @ 0x5af5fc right after
  its `LoadLibraryA`). The asset-loader doc mapped the three DLL handles to the
  wrong DAT_ slots (said sotesp.dll) — corrected.
- Every title resource (logo 0x49f, bg banks 0x91b/0x91c, slot-0 palette seed
  0x90b) lives in sotesd.dll's `DATA` type; none in sotesp.dll. Slot 0's
  `sotesp_module` arg is the SAME sotesd handle (misnamed param).

**Verified 1:1 against retail goldens** via new port-side frame capture
(`--capture-frames "60,200,…"` → `port_frame_NNNNN.bmp` of the composed primary;
BMP→PNG→read-image). Port frame 60 = full title art + "FORTUNE SUMMONERS" logo;
port frame 200 = the title menu (Start/Continue/Bonus Menu/Options/Exit +
"Secret of the Elemental Stone" + copyright), **pixel-identical to retail
`runs/title-idle/frame_01900.png`**. Frame capture committed `dd4ef08` (roadmap
task #10 core).

**Three remaining fidelity gaps (NEXT layer, not rendering bugs):**
1. Intro pacing — port reaches the menu by Flip ~200; retail still on the
   Lizsoft splash there (~1900 to menu). The pace pump `0x5b1030` is a stubbed
   hook, so phases aren't time-gated. Port it to match retail's timeline.
2. Menu cursor highlight absent — the CURSOR bank (pool 20) is registered + live
   but the `MENU_CURSOR` sink arm is a deferred no-op. Wire it.
3. Lizsoft studio splash not drawn — `LOGO`/`SPARKLE` arms deferred (need the
   runtime alpha ramps `0x8a92b8`/`0x8a9308` populated).

**647 host tests (0 fail, 6 skip).** Ledger **138/1490 (8.6%)** unchanged —
wiring, not a new function port. Added `SINK_RESOLVE_DEBUG` compile-gated probe.

---

## ckpt 27 (2026-06-02) — menu cursor wired via the alpha ramps; bit-exact policy

Wired the title menu's selection cursor (the only visible delta left on the
settled menu).

- **Alpha ramps** (`src/main.c` `init_alpha_ramps`): `pd_boot_init_slots(NULL)`
  (already-ported; NULL fmt ⇒ RGB565 = the 16bpp display) builds the 20+20 blend
  descriptors in `g_pd_boot_group_a/_b`; exposed as the `ramp_a`/`ramp_b`
  pointer tables (`const zdd_blend_desc *` views — `PdBlend` aliases the retail
  blend-descriptor layout `zdd_alpha_blit` reads). Passed into the sink via
  `cfg.ramp_a/_b`.
- **Cursor arm** (`src/title_sink.c`): `TITLE_DRAW_MENU_CURSOR` now resolves the
  CURSOR bank (pool 20) frame at the row index and draws via
  `title_draw_menu_cursor` (alpha through `ramp_a`), replacing the deferred
  no-op callback. Low-risk to the rest of the menu: `compose_group` is NULL
  (FRAME_END no-ops) and SPRITE_LEVEL takes the plain path at full fade.

**Result:** "▶ Start" renders; menu diff vs retail golden 2964 px → **955 px
(0.31%)**. **NOT closed** — per the project's bit-exact bar, the 955px cursor-
edge residual is an OPEN investigation item (parity-ledger R1), with two
hypotheses: (1) the cursor pulses (animated `level_num` = retail's path-dependent
`[esp+0x20]`) so port Flip 200 vs retail Flip 1900 are at different pulse phases
— an intro-pacing/phase problem; (2) our idx-19 full-add over-brightens. Tests
for each recorded in the ledger.

**New durable artifacts:** `docs/parity-ledger.md` (frames confirmed bit-exact
vs open residuals — regression guard), `tools/push_comparison.py` (port|retail
amplified-diff → llm-feed), the `--capture-frames` port frame capture (ckpt 26b).
Policy recorded: **bit-exact (0 differ_px) is the bar; no diff is hand-waved.**

648 host tests pass (0 fail, 6 skip; +1 `sink_menu_cursor_draws_via_ramp`).
Ledger 138/1490 unchanged (wiring).

---

## ckpt 31 (2026-06-02) — phase-7 sparkle particle twinkles: BIT-EXACT (R4 closed, user-confirmed 1:1)

Closed the last title-intro content gap (parity-ledger R4). The `FUN_0056c070`
particle twinkles are now ported end-to-end and verified `differ_px=0` against
retail with the RNG seed pinned on both sides — **user-confirmed 1:1 including
particles**.

**The subsystem (quirk #58)** — four sites in `FUN_0056aea0` over one cap-500
pool (`DAT_008a92b4`):
- **spawn** `0x56c070` (phase 7, while `uVar15 < 0x352`): append a particle at
  the reveal edge with 4 `rand()` draws (x, y, **velocity `+0x08`**, lifetime
  `+0x0c==+0x0e`).
- **update** `0x56ba69` (every update tick): `y_num -= vel; vel += 2` (rise,
  accelerating up), `anim_num != 0 ? anim_num-- : cull` — particles **evaporate
  upwards** and fade (the draw's frame walks 0→7 as `anim_num` counts down).
- **cull** `0x56c030`: swap-remove (`count--; entries[i]=entries[count]`).
- **draw** `0x56c180` (== `title_compositor_draw`, already present): bank `0x15`
  = pool 21 = resource `0x91d`, blend `ramp_a[16]`.

**The bug + fix:** the first cut spawned + drew but **omitted the per-frame
update**, so particles froze at the spawn row and piled up → an over-bright
smear (8277 px diff). The user spotted it ("they stay frozen… in retail they
evaporate upwards"). RE found the inline update at `0x56ba69` (both watchdog
branches reach it → runs every tick) + the cull caller `0x56baae`. The `+0x08`
field — previously dismissed as `_pad08`/"spare" — is the upward velocity.
Porting the update dropped the diff to a clean diagonal and to **`differ_px=0`
at tick alignment** (port Flip 465 vs `sparkle-align/frame_00939`).

**Determinism (user directive: pin the seed both sides).** The engine LCG seed
`DAT_008a4f94` is `srand(time())` (`0x56227a`), so retail's twinkles are
wall-clock-random. Ported the LCG (`src/rng.{c,h}`, `FUN_005bf505`/`_5bf4fb`),
pinned a fixed seed in the port by default (`OSS_RNG_DEFAULT_SEED 0x4f5347`,
`OPENSUMMONERS_RNG_SEED` overrides), and added harness `--seed-pin` (default ON)
that writes the same value into retail at the first spawn. Off-tick frames still
differ purely by the R3 render-rate sub-tick jitter (retail renders each update
~2.2×) + run-to-run intro-pacing jitter (first spawn at flip 886/895/896) — so
align by the new `subtitle_anim_start` TAS anchor + update-tick, never a fixed
flip index.

**New artifacts:** `src/rng.{c,h}`, `src/title_particles.{c,h}` (pool + spawn +
update + cull); `title_sprite_entry._pad08` → `vel`; `update_particles` hook;
`frida_capture.py --seed-pin`/`--seed-value` + the `subtitle_anim_start` anchor
(`installSparkleAnchor`). 660 host tests pass (+10: LCG stream anchored to the
real MSVC sequence, spawn field/order/cap, update rise/age/cull). Ledger
**144/1490 (8.8%)** (+6 real ports: LCG/srand + spawn/update/cull). Commits this
ckpt: `feat: port phase-7 sparkle-particle spawn + LCG` → `wire` → `harness seed
pin` → `fix: per-frame update` → `harness TAS anchor`.

---

## ckpt 36 (2026-06-02) — TEXT RENDERER VERIFIED BIT-EXACT vs retail's live TextOutA stream

Closed the ckpt-35 "render a string, diff" gate. **Part 1** (committed earlier):
wired `ar_register_fonts` at boot (builds the 8 HFONTs) + a `--render-glyph-test`
offscreen-DIB path on the port side. **Part 2** (this checkpoint): verified from
the retail side. New **`frida_capture.py --textout-probe`** hooks
`gdi32!TextOutA`/`ExtTextOutA` and records every glyph draw (x/y/bytes/text-colour/
bk-mode + the selected `LOGFONTA`); drove retail to the **new-game config menu**
(`trace-retimed.jsonl`, Start at flip ~400 — the title auto-demos by ~flip 900, so
the old flip-2050 press missed) and compared its real output to the port renderer.

**Every parameter matches bit-for-bit:** font Courier New **7×18** (port slot 3),
bk mode **TRANSPARENT**, **7 px/glyph** advance, **per-glyph `TextOutA`**, the
**2-copy shadow** `(x+1,y)/(x,y+1)`, colours normal **0x3e537d** / shadow
**0xa8b9cc** / focused **0xf08080**. GDI rasterizes deterministically from an
identical HFONT + identical draw args → the glyph pixels are bit-identical; the
renderer port is correct. The end-to-end stream/pixel diff now only awaits the
new-game menu **builder** (it supplies the cells the renderer walks). Quirk **#63**.

Also found: under the hidden-window turbo harness retail runs **~15 flips/s**
(vs ~127 native) and the title auto-enters a gameplay **demo** that paints a GDI
**debug HUD** via a *different* text routine (a full **3×3 outline**, not the
menu's 2-copy shadow) — documented, not parity-relevant. Goldens saved:
`tests/scenarios/new-game-through/goldens/{retail-newgame-config-menu.png,
retail-newgame-config-textout.jsonl}`. 691 host tests pass (no `src/` change —
verification + tooling). Ledger **150/1490** unchanged. User-confirmed the retail
golden capture looks good.

---

## ckpt 39 (2026-06-03) — THE NEW-GAME CONFIG SCENE RUNS LIVE: visible + interactive (drive ported + wired)

Wired the new-game config scene as a runnable **drive** — the last missing piece
after the builder (ckpt 37), renderer (bit-exact, ckpt 36), and run-loop model
(ckpt 38).  New **`src/newgame_drive.{c,h}`** is the Win32-free caller (the
`FUN_00565d10` / case-0x24 frame pump side), mirroring `title_drive` vs
`title_scene`: it owns the scene + an input manager, ramps the input-ready gate
**+50/frame to 1000** (the title's `menu_owner_transition_step` mode-1 ramp,
quirk #34/#59), polls the buttons each frame and collapses them via
`menu_list_latch` into the pump result the scene dispatches on, then renders +
presents through cfg callbacks.

**Input pump = `0x565d10`'s scan + `0x43bca0`'s id→latch map (quirk #65),
realised:** 1=up/3=down/2,4=page → MOVE (`0xd`); `0x24`=confirm → latch(9) → 3 →
CONFIRM (`0xc`); `0x27`=back → latch(10) → 4 → BACK (`0xb`).  Terminal outcomes
START/BACK latch; OPEN_PICKER (kind-0 confirm) is surfaced + counted (the picker
submenu `0x567ba0` is deferred).  7 new host tests.

**`main.c` wiring:** `app_flow`'s NEW_GAME arm now routes to `enter_newgame`
(was the re-enter-title stub).  `newgame_render` does the per-frame GDI render of
the menu grid onto the primary surface (`glyph_grid_render` at base (32,32),
**Courier New 7×18 = font slot 5** — the LOGFONTA the golden's TextOutA stream
selects, not the title's slot 2).  A `g_newgame_active` branch in
`main_loop_body` runs one `newgame_drive_step` per presented frame.  BACK
(`0x27`, result 0xb) → re-display title; START → stub (stone intro + game proper
unported) → re-display title.

**Verified LIVE** (`--input-trace`, `--capture-frames`): confirm Start on the
title @flip 620 → `result=26` → NEW_GAME → `enter_newgame`; the menu renders
"Game Difficulty 1:Easy / Auto-guard On / Start Game" with row 0 focused; DOWN
moves the focus 0→1 (pixel-sampled: focused text `0xf08080` periwinkle + tan
`0xa8b9cc` shadow — bit-exact colours per ckpt 37, NO R/B swap); `0x27` backs
out → title replays from phase 0.  Port-vs-retail comparison pushed to llm-feed
(the only diffs are the deferred box chrome + tooltip).  Quirk **#66**.

**Deferred seams** (documented): the box widget chrome (`0x411940`→`0x40f3e0`,
plain black fill for now), the tooltip text node (y=416/444, word-wrapped — text
computed, render needs the box/wrap builder), the option picker submenu
(`0x567ba0`), and the Elemental-Stone intro (`0x564160`→`0x5642e0`→`0x59ec30`).

705 host tests pass (0 fail, 6 skip).  Ledger **157/1490 (9.7%) (+2: `0x565d10`,
`0x43bca0` — both partial, the pump's scan + id→latch arms)**.

---

## ckpt 42 — selection-cursor sprite BANK identified + proven (0x455 slot 43 frames 16-19, decoded BOTTOM-UP); a scale_flag=1 render bug remains, gate stays OFF

Resolved the ckpt-41 blocker (the new-game selection cursor's unidentified sprite
bank).  **The bank is `0x455`** (sotesd.dll, slot 43 = `AR_SPR_FONT_TEX_455`),
**frames 16–19** — exactly the bank/slot/frames the ckpt-41 geometry port
(`src/newgame_cursor.{c,h}`) already targeted.

**The ckpt-41 "0x455 sweep matches nothing" was a decode-ORIENTATION error.**
The Lizsoft DATA blob (`runs/extract/sotesd/type=DATA/1109.bin`) carries a
BMP-style preamble and its 8bpp pixels are a **bottom-up** bitmap; the engine
slices cells bottom-up.  The atlas is 128×288 = a 4-col × 6-row grid of 32×48
cells (24 frames).  Read **top-down**, frames 16–19 land on the row-4 ► chevrons
(9×17 — not a vine); read **bottom-up**, frames 16–19 are the drooping gold
feather/quill + soft white shadow.  Their trimmed bounding boxes match the live
`--box-probe` golden **EXACTLY**: frame 17 = 22×41 @ (4,3), 18 = 22×40 @ (4,4),
19 = 22×41 @ (4,3).  New proof tool **`tools/extract/cursor_frame_match.py`**.

The probe's `res_id=0x3e8` (slot+0x40) was a **reused/garbage marker** — PE
resource 0x3e8 is an 80×352 portrait in sotesd, a WMV in sotesw, absent in
sotesp (verified by extracting all three).  The reliable per-frame signal is the
trim size read via `entries[frameSel]→frec+0x14/+0x18`, which the probe also
records.  Quirk **#68**; #67's cursor claim corrected (0x3e8 → 0x455).

**A separate render bug surfaced when I flipped `g_newgame_cursor_enable` ON**
(NOT a bank problem).  Live capture (port frame 760): the cursor blits as an
opaque-black 16×24 rect at x72–87 (golden feather x44–66), differ_px 307→493.
`0x455` is the only registered bank with **`scale_flag=1`** (box 0x457 is 0), so
its cell takes the untested videomem cell-build path (`zdd_object_build_cell`
`videomem` arg → caps 0x804): the slicer computes the wrong trim offset
(base (40,26)+fdx≈32 → x72 vs correct fdx=4 → x44) AND the transparent area fails
to colour-key.  Gate reverted to **OFF**; the scale_flag=1 trim/keying fix is the
next render task (HANDOFF Next move #1a'').

No `src/` logic change (decode-orientation + bank-ID finding + the new tool +
docs).  **713 host tests pass (0 fail, 6 skip)** unchanged.  Ledger unchanged
(no new FUN ported).

---

## ckpt 45 — the new-game OPTION PICKER submenu is ported, wired, rendered, and user-confirmed (FUN_00567ba0; quirk #71)

Confirming on a kind-0 option row (Game Difficulty / Auto-guard) of the
new-game config menu now opens that option's **picker submenu** — the nested
value-grid `FUN_00567ba0` runs.  Until now `newgame_scene_dispatch` surfaced
`NEWGAME_OPEN_PICKER` but the drive only counted it (the value stayed put).

New pure, host-tested **`src/newgame_picker.{c,h}`**:
- **`newgame_picker_values`** (`FUN_00568320`): option id → value-code list.
  id 3 (difficulty) `{10,20,30,40}` / `{..,50}` unlock-gated; id 4 `{0,1}`.
- the run-loop model: build the 1-column value grid (labels from
  `newgame_option_value`), **seek the cursor to the current value**
  (`FUN_00419900`), and nav/commit/cancel via the `NEWGAME_PUMP_*` codes
  (567ba0.c:237-251) → RUNNING / COMMIT(value) / CANCEL.

Wired into **`newgame_drive`** as a frame-stepped modal SUBMODE (the equivalent
of retail's blocking `FUN_00567ba0` call): on a kind-0 CONFIRM the drive opens
the picker and pumps input into `picker.grid` (its own ramping gate) instead of
the parent scene; on COMMIT it calls `newgame_scene_set_option(id, chosen)` (the
parent's value-refill — re-lays the value cell); on CANCEL the option is left
unchanged.  `main.c`'s `newgame_render` draws the picker box (9-slice, (288,128)
w=256) + `glyph_grid_render` of its value rows over the menu when active.

**Verified LIVE** (port trace: Start → confirm Game Difficulty → down → confirm):
the difficulty picker opens showing `{1:Easy,2:Normal,3:Hard,4:Expert}` with the
current value focused; nav moves the highlight to 2:Normal; commit closes the
picker and re-lays the parent value cell **1:Easy → 2:Normal**.  4-frame montage
pushed to llm-feed; **user-confirmed** ("screenshots look good, those menus
render correctly").

**RECONSTRUCTED + OPEN gate (quirk #71):** the picker's `__thiscall` arg lists
(the `FUN_00412160` row kind, `FUN_00419900` seek, `FUN_005657f0` commit) were
dropped by the decompiler and are rebuilt from the callees' contracts.  A live
**retail** golden of the open picker is **unreachable** — the flip counter
freezes in `0x565d10`'s modal pump (quirk #67 caveat), and both the harness's
capture and input injection are flip-keyed.  So the box geometry (288,128/256)
and the reconstructed args are NOT pixel-verified against retail; closing that
gate needs a harness that drives/captures inside the modal pump.

10 new host tests (8 picker + 2 drive flow): **736 total, 730 pass / 0 fail /
6 skip**.  Ledger **173/1490 (10.7%)** (+5: `0x568320`, `0x567ba0`, `0x419900`,
`0x5657f0` ported; value provider already counted).

---

## ckpt 48 — TAS: the new-game→prologue fade-out is ported (cutscene-offset gap CLOSED)

Closed open thread #1 from the TAS harness (ckpt 47): the port skipped retail's
post-config fade-out, so it entered the prologue cutscene 1 flip after the
"Start Game" commit while retail spends ~20 flips, leaving the cutscene fade-in
un-alignable at a single anchor offset (residuals 246–662 px = one fade step).

**RE finding (quirk #72):** `FUN_00564160`, on a Start commit, clears the menu
box node's `+0x50` (564160.c:30) and runs a **≤20-frame fade-out loop**
(`FUN_005642e0` scene update + `FUN_0056c930` mode-1 CLOSING alpha ramp +
`FUN_0043c2e0` per-entry) before `0x56cd20` — exactly **20 presented flips**
under `--lockstep` (confirm@795 → prologue_enter@815).  `FUN_005642e0` early-
breaks (returns 6) only on an abort event (id 0x22); the Start path runs all 20.

**Port:** a fade-out submode in `newgame_drive` (`NEWGAME_FADEOUT_FRAMES=20`).
A Start-Game commit no longer returns `NEWGAME_START` immediately — it sets
`fading`, clears `node.field_50`, seeds `node.field_54` from the open gate, and
for the next 20 frames ramps `field_54` down by `0x28`/frame (the `0x56c930`
mode-1 close), re-renders + presents, then returns `NEWGAME_START` so `main.c`
enters the prologue at retail's offset.

**Verified (tas_diff, `prologue_enter`, port@821 vs retail@815,
runs/tas-retail-gem, window 2):** prologue_enter moved **801→821** (+20 flips,
matching retail's 20-flip fade-out); `prologue_enter` rng still **0x40d00581**
(the fade consumes no `rand()`).  The cutscene **fade-in (ticks 1-28) is now
entirely `differ_px=0`** at a near-constant drift (−2, ±1 lockstep wobble) — the
ckpt-47 fade-in residuals are GONE.  **63/64** dense gem-rise frames bit-exact;
the only non-zero frame is tick 0 (84684 px) = the **pre-existing** entry-frame
issue (port renders the first gem tick while retail's first present is black —
documented at ckpt 47, NOT introduced here).  Comparison pushed to llm-feed.

**STILL OPEN — the fade-out frames' RENDER:** the port re-renders the new-game
menu **opaque** during the 20 fade frames; retail fades the box-panel alpha
(`0x48cf80`'s alpha arm via `0x5bd550`) + the GDI menu text, so those ~20
transition frames are not yet bit-exact.  That fade *render* is the deferred
box-alpha arm (a separate item, needs a retail capture of the fade-out frames to
model the text fade).  This checkpoint closed the **timing/offset** gap.

751 host tests pass (+1 `newgame_drive_fadeout_then_start`).  Ledger
**175/1490** (+1: `0x564160` partial — the transition loop tail; de-inflated
`0x5b6990`/`0x5642e0` to bare VA per the unported-callee convention).

---

## ckpt 49 — IN-GAME PHASE KICKOFF: retail golden of the opening map captured

The ckpt-13 milestone (1:1 title + new-game + prologue) is MET, so per the
ckpt-48 decision the user opted to **extend the trace in-game**: *"yes extend the
trace.  start by just spamming Z after the prologue begins, give the trace ~1
min of frames, then continue matching retail."*

**How you reach it:** the prologue cutscene `0x56cd20` exits on the **3rd beat**
(any fresh press); **Z = confirm id `0x24`** = a beat = the NORMAL exit (`0x22`
would *abort* to the title).  On `PROLOGUE_DONE` retail calls
**`0x59ec30(0,0,0x3f2)`** → loads + runs the opening map.

**RETAIL GOLDEN (`runs/tas-ingame-1`, local only — runs/ gitignored):** captured
under `--lockstep` (anchors subtitle@432 / newgame@667 / prologue@815, as
always).  Flips **900–1100 are black** (cutscene exit-fade + map load); **the
game renders from ~flip 1150** = the **opening TOWN of Tilelia** (houses, NPCs,
a banner), and spamming Z advances into the **story DIALOGUE** (character
portrait + textbox, ~flip 2200–2500).  6-frame montage pushed to llm-feed.

**THE NEXT ROCK — `0x59f2c0` (3522 B), the map run loop.**  `0x59ec30` (531 B)
is the scene LOAD/UNLOAD wrapper; `0x59f2c0(map,…)` is the in-game engine (zeroes
a ~0xeb1c-B stack frame of tile/entity arrays + a 0x4120 map object, loads map
0x3f2, runs the per-frame update + render dispatch `0x5a00c0`).

**Plan (full writeup: `docs/findings/in-game-intro.md`):** (1) add a game-entry
ANCHOR so tas_diff aligns the in-game frames; (2) wire the port seam
(`PROLOGUE_DONE` → `enter_game` stub); (3) port `0x59f2c0` in units vs the golden
(town tilemap → entities → dialogue box), smallest visible win first.

**No port code this checkpoint** — phase kickoff (golden + plan + reproducible
trace `tests/scenarios/in-game-intro/`).  751 host tests still pass; ledger
175/1490 unchanged.

---

## ckpt 178 — 2026-07-02 — ALL-MAPS resource sweep: 587e00 tile dispatch COMPLETE (100% cell coverage) + the region-E placeholder pass + STRUCTURE table done + viewer band naming

**USER ask:** sweep every game resource (esp. maps) with the res_explorer infra;
port unported tile types + whatever the viewer stubs; check the "particle
emitters named character" label.

**Sweep tool `tools/extract/map_sweep.py`:** enumerates all 376
MSD_SOTES_MAPDATA DATA resources in the exe, histograms tile ids + object
codes vs the PORT's own sources (case labels + MD_SIMPLE_ARMS + the def tables,
parsed live so the report can't go stale).  Pre: 23/87 tile ids, **5.0%** of
342,757 populated cells.  Post: **87/87, 100.0%**.

**map_decode.c — the full FUN_00587e00 dispatch** (faithful transcriptions,
emit-for-emit in retail order; digest-assisted read of the 3,282-line decompile
with the parser VALIDATED against the already-ported arms): all interior
families (10xxx/11xxx banks 0x5c-0x6a, 112xxx 0x189-0x18c, 121xxx 0x68/0x69,
131xxx 0x79-0x7e + bank_14, 141xxx/142xxx 0x71-0x76, 151xxx/161xxx/162xxx,
114xxx autotiles 0x193-0x195 incl. the 0x1bd82 staircase collision clusters,
dir6 additions 0x29c16/0x2a008, 999xxx class-1 markers).  Key RE corrections:
emit_tile (58c910) params **9/10 are the explicit spans** (7/8 the unused
pair) — 0x1d8ab/0x22b16 shape-3 use a 1x2 span; 0x1ffc3/c5 = caseD_2724 twin
(d4=2, bank 0x7a); 0x1ffeb = 0x272e twin; 0x1b995 = 0x1b990 w/ bank_20.

**The trailing placeholder pass (587e00.c:3185-3204 -> FUN_0058cb30) PORTED:**
map codes 90010/90011 are not spawns — they fill region-E link-anchor records
(quirk #120).  This is the FRONT-flagged "object-spawn PLACEHOLDER pass" input
for the missing house props (mark t2278) — the spawn CONSUMER is the next link.

**STRUCTURE def table COMPLETE:** 0xf295 -> 0x77; 0xeead -> the runtime bank
*(0x8a9b50+0x27a4) = 0x88/0x89/0x8a per param_4 5/6/8 written by the 587e00
prologue (quirk #121) — the old "runtime value, omitted" mystery resolved.
Every STRUCTURE code any map places now resolves.

**res_explorer:** 9001x band named PLACEHOLDER (was "?") with a live region-E
record decode in the inspect panel; CHARACTER label clarified (the ENGINE's own
band name per the 0x4a2e64 strings — holds props/emitter fixtures too, which
answers the USER's naming question: keep, but explain); param_4 cfg dropdown
grown to 1-8 (mirrors the 0xeead runtime-bank write); marker legend + violet
placeholder swatch.  Verified visually: map 1057 (castle ruins) and 1047 (cave
tunnel, the 13-shape 0x1ffc3 autotile edge) render fully with 0 unported
hatching; town 1022 regression unchanged (1268 tiles + 59 actors) — pushed to
the llm-feed.

Quirks #119-#121.  7 new host tests (region-E fill, spans, stairs, markers);
**1095 pass / 0 fail**.  Ledger 227/1490 touched.

**Still stubbed (unchanged PORT-DEBT):** EFFECT sprite table (75/87 codes off
the captured town set — the 27 KB 0x41f200 switch), CHARACTER sprite table
(87/90 — the lazy 0x40afe0/0x41e600 fill), the decode-prologue header words +
palette-remap installs (PORT-DEBT decode-prologue-header), the region-E spawn
CONSUMER.

---

## ckpt 194 — 2026-07-04 — the town WANDER NPC's per-tick LCG draw PORTED (first permanent town RNG divergence 974 → 1019)

Closes the ckpt-193 diagnosis: the port omitted a grounded town pedestrian's
per-tick `0x43f880:415` "push command 3" wander roll, the first permanent town
RNG split.  New module `src/town_npc.c` (the town analogue of `ambient.c`/
`butterfly.c`): the NPC (retail body `0xe8767d8` @ world 41600,45600) draws the
shared LCG once per sim-tick over the seed-pinned walk window **[972,1077]** (106
ticks), consume-to-advance — the DRAW aligns the stream, the roll RESULT (wander
motion) is not rendered.  Wired FIRST in `game_actor_update`'s RNG section (before
`butterfly_step`), gated `census_tick = g_sim_tick_count + 1` (the actor update
runs before the camera easer bumps the counter).

**Position + window PROVEN off a fresh census randtrace** (`OSS_RAND_TRACE 968-1080`,
seed-pin, same nav): the NPC's `0x440301` draw is at INDEX 0 in ALL 106 walking
ticks — it LEADS the per-tick stream, before the EFFECT-band butterflies.  (The
randtrace's own `g_eh_sim_tick` axis = census − 1, since the actor update precedes
the easer; align by the NPC-onset draw, not the tick number.)  Walk envelope read
straight off the two OSR_STATE captures: retail = port +1/tick contiguous 972..1077,
then 0 (the NPC goes idle for the rest of the town).

**VERIFIED** off `port-npcfix.osr` vs `retail-rngcensus3.osr` (`rng_seq_diff.py`):
the first PERMANENT town divergence moved **974 → 1019** (+45t); ticks 972-978 now
bit-exact + the 972-973 butterfly self-heal fires (the +1 at 972 perturbs the state
so the port's 973 conditional draws retail's 20, not 21).  The 1019 residual is the
SECONDARY `0x489280` ±2 consumer (draws 2 rands at entry, `489280.c:24-25`; fires
census 979/999/1019/1039/1056/1066/1135…, gaps `20,20,20,17,10,69`, continues past
the walk) — the NEXT chip.

`PORT-DEBT(town-wander-npc)`: the window [972,1077] is MEASURED, not yet derived
from the NPC's spawn + idle→walk timer; the NPC's `0x442a70`/`0x43f880` walk AI +
collision + its RENDER (its path 41600→43024 is near Arche's arrival point ⇒ likely
on-screen — flagged for USER visual check) are deferred.  1105 host pass;
`town_npc_pertick` test.  `errands-rng-census.md` "The NPC walk PORTED".
