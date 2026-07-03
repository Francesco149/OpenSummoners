<!--
  The ONE hand-edited status block.  tools/gen_port_ledger.py injects everything
  between the FRONT:BEGIN/END markers verbatim into docs/STATUS.md's "Current front"
  section, so STATUS can never drift from reality.  Update THIS when the active front
  moves; keep it a 60-second read.  Everything else in STATUS is derived from code.
  Port state / next-move detail belongs here or in HANDOFF.md, NOT in engine-quirks.md.
  Rolling per-checkpoint narrative → PROGRESS.md; deep RE → findings/.  Keep this SHORT.
-->
<!-- FRONT:BEGIN -->
- **Phase — the freeroam sim**, built bit-exact vs errands re-drives. The prior arcs
  (town intro → arrival/house/errands cutscene; the HUD panel ckpt 167–174) render 1:1.
- **Landed ckpt 175 — COLLISION:** the movers `0x54ded0`/`0x54db10` tile halves ported
  (`collision.c`) + `character_step` restructured to the retail `0x442a70` tick order
  (support probe → vertical mover/ledge-fall → ramp → worldX commit); slope ramps
  read live off the user's exe; the `LAB_00589520` "occlusion marks" = invisible
  collision WALLS (map_decode fix). **Arche stops/climbs/descends the errands stairs
  tick-exact** (dwx==dwy==0 through the whole climb; both walls flush) — retires the
  studio-note-2441 divergence + debts `char-collision-mover`/`collision-slopes`/
  `decode-occlusion-mark`. The 4-tick standing TURN-AROUND (`char-turn-state`) is
  now PORTED (ckpt 177 below). RE: `findings/freeroam-collision.md`.
- **HUD blocker (parked) — the PORTRAIT.** `hud_ctx+0x1b4` (leader_uid) reads 0x0 on
  every scripted replay (a replay-fidelity gap, not a port bug); resolve via the
  `+0x1b4` setter or a live/manual play. `findings/freeroam-hud.md §6-9`.
- **Landed ckpt 176 — dialogue mark t1197 FIXED + char-turn RE corrected.** (a1) The
  "port skips dialogue early" mark is NOT an advance-gate bug — the advance PRESSES
  already match retail (port DLGT + dialogue_timeline off the stairs pair).  It was a
  box RENDER linger: retail keeps the arrival L9 box up through tick 1200, the port
  cleared it at 1192 → `ARRIVAL_EXIT_BOX_HOLD=8` (== the house-exit box-hold pattern),
  drawcall-verified L9 adv 1192→1200 == retail.  `findings/dialogue-advance-early.md`.
  (c) char-turn RE CORRECTED: the ckpt-175 "case-2" pointer was WRONG (that's the
  DOWN/crouch, already ported); the real reversal turn is the STATE-1 horizontal FSM
  `0x442a70:1011-1090`.  See `port-debt.md`.
- **Landed ckpt 177 — `char-turn-state` PORTED (retires the collision residual).**
  The from-rest REVERSAL now plays retail's 8-tick pivot: `CHAR_TURN_HOLD=4`
  STATIONARY windup ticks (facing HELD, the fr-6 turn cel) → flip facing + walk ramp
  (fr-7 → +152 fr-159 lingers 4 render ticks) → walk (fr 160).  `character.c`
  `turn_ctr`/`turn_frame` + `ARCHE_TURN_CLIP` {6,7} (both dirs via the +152 mirror,
  == the house turn 158→7).  GROUND TRUTH retail-stairs res 0x570 (fr 6×4 → 159×4 →
  160); `draw_probe port-stairs2` == retail, `state_diff` RIGHT 0-div/301t (no regress)
  + LEFT ramp-shape bit-exact (the −960 gap GONE).  RESIDUAL: a constant 1-tick
  reversal-ONSET phase (−240) — the port latches the reverse press 1t before retail's
  warmup gate (→ `char-input-autorepeat`, not the turn), within retail's ±1-2t
  coalescing slop.  RE: `findings/freeroam-turn-around.md`.  **USER: verify the pivot
  animation — studio shortcut = `port-stairs2 | retail-stairs` @ tick ~2950.**
- **Landed ckpt 178 — ALL-MAPS SWEEP: the 587e00 tile dispatch is COMPLETE.**
  `tools/extract/map_sweep.py` (all 376 map resources vs the port's own sources):
  tile-id cell coverage 5.0% → **100.0%** (87/87 ids; every family transcribed
  emit-for-emit; quirks #119-#121).  The trailing `FUN_0058cb30` placeholder pass
  is PORTED (map codes 90010/90011 → region-E link anchors — NOT spawns).
  STRUCTURE def table complete (0xf295→0x77; 0xeead→the runtime bank 0x88/89/8a
  per param_4 5/6/8, RE'd off the 587e00 prologue).  res_explorer: PLACEHOLDER
  band named + region-E inspect; CHARACTER label kept (the ENGINE's own band
  name — holds props/emitters too, answers the USER naming question); param_4
  cfg 1-8.  Maps 1057/1047 render fully (feed); town regression clean.  1095
  host tests.  Still stubbed: EFFECT/CHARACTER sprite tables beyond the town
  captures, the decode-prologue header/palette installs, the region-E spawn
  CONSUMER (next link for (b) below).
- **Landed ckpt 179 — the arrival→house dialogue DRIFT FIXED (`ARRIVAL_EXIT_WAIT` 10→20).**
  The chain played −11t early because the arrival-exit WAIT was curve-fit to 10 (a bogus
  "2× anomaly") when it's the script's `0x14=20` mapped **1:1**.  The scene_fade PERFORMER
  is bit-exact (settle rates match) — the "settle-rate" hypothesis is DISPROVEN; the old
  "room-load black-hold" read was a CONFOUND of the −11t global shift.  The "2×" was a
  measurement error: ckpt-137 timed "first CENTER alpha" but retail's cover grows TOP-DOWN
  (sweep, not center-out), so the center lags the arm.  GROUND TRUTH off `retail-stairs.osr`
  (res-1110 box + 64×4 fade coverage; retail fades capture as res=0 subtract-blend, no
  alpha split): house L0 box open −11t→**−1t**, **house box close now tick-EXACT (1650)**,
  errands −20t→−6t.  `findings/dialogue-advance-early.md` "Component 2/3 RESOLVED"; quirk
  #122; 1095 host pass.  **USER: verify the transition — studio `port-waitfix | retail-stairs`
  @ ~1200-1350 (the arrival→house cover/reveal); feed `waitfix_cmp.png`.**
- **Landed ckpt 180 — the errands t2278 mark DIAGNOSED + the kitchen CABINET FIXED.**  The
  "missing POT" is a `port-waitfix` STALE-TRACE artifact: the pot (res1074) renders BIT-EXACT
  in the walk-aligned `port-stairs2` vs retail-stairs @t2278 (proof: port-stairs2==retail
  bit-for-bit — pot 376/376, wall-shelf 108/108, Arche 270,296).  The ckpt-179 waitfix
  desynced port-waitfix's tick-keyed freeroam walk (+176px camera lag, 4 props measured) — so
  **the freeroam studio pair is `port-stairs2 | retail-stairs`, NOT port-waitfix** (waitfix is
  valid only for the dialogue window ≲tick 2000).  The real gap = the RIGHT-side/upstairs props
  ERRANDS_CAST systematically missed (captured from the static tick-2200 LEFT/centre view): the USER
  flagged the kitchen CABINET (res1023 fr4 + hutch fr2) AND the POT (res1026 fr58 @228,208, "right of
  Mom's head").  FIXED as **6** ERRANDS_CAST entries (cabinet/hutch/pot + 3 upstairs props, DATA-1025
  CHARACTER codes 0x112d1/0x112da/0x11279/0x112d3/0x1124c; banks via slot+13; world = map ×100) —
  VERIFIED at the clamp t2500: all 6 render at retail's EXACT pos+dims (feed `pot_crop_cmp.png`).
  **ROOM PROPS NOW VERIFIED COMPLETE** — a map↔retail cross-ref confirms ERRANDS_CAST covers ALL 20 of
  the map's visible CHARACTER objects; a port↔retail draw diff at t1800 AND the t2500 clamp shows NO
  missing prop either side.  Also PORTED (ckpt 180b): the **pot STEAMS** (POT_CLIP 57→60 dur6) + the
  **clock pendulum SWINGS** (CLOCK_CLIP 43↔45 dur25), both == retail.  Remaining (NOT props): the HUD
  (res1900 = the bottom-left HUD strip, NOT a room object; + the blank portrait blocker + strips), Mom's
  pose (res1127 fr0 vs fr2), the clock/pot anim start-phase seed.  The runtime map-driven spawn (retiring
  the ERRANDS_CAST capture) is now pure cleanup, blocked on the char-band z-order (no draw_pool Y-sort);
  the code→bank table + variant model are proven + ready.  `errands-render-gaps.md` §6.  **USER: studio
  `port-cabinet | retail-stairs`, scrub the CLAMP (~t2400+) — the pot should steam, the clock swing.**
- **Landed ckpt 181 — the upstairs-BED "renders empty until fully in frame" = a mode-0 CULL bug (FIXED).**
  The ckpt-180c "res=1071 fr6 bottom-strip DECODE bug" hypothesis was WRONG: res=1071 fr6 is the
  window-WALL (sheet byte-IDENTICAL port↔retail); the BED is a keyed ACTOR **res=1023 fr13** (in
  ERRANDS_CAST at world 60000,6400) that the port SPAWNS but CULLED at t2158 (drew fine at t2325).
  ROOT: `map_present` mode-0 cull box read the cel's CONTENT dims (`metric_b8/bc`) not retail's CANVAS
  dims (`+0x1c/+0x20` = `metric_1c/20`, `FUN_0048eac0` case-0).  For a trimmed cel with a vertical
  pivot (bed: pivot y=68, content h=60, canvas h=128) at dst_y=-96, content-h culls (-36<0) but
  canvas-h keeps it (+32) and the keyed blit's pivot pokes the content on-screen.  FIX: `game_cel_dims`
  → `metric_1c/20` (bit-exact w/ retail's cull).  VERIFIED off `port-bedfix.osr`: bed now emits
  `res=1023 fr13 @(464,-96)` == retail, reconstructs **`differ_px==0`**; `frame_diff` old-vs-new = 0
  removed / +9 added (the bed + 2 upstairs chests + 6 props, all wrongly-culled).  New tools:
  `sheet_export.py`, `blits_at/region_draws/frame_diff.py`, `osr_prof pick`.  1096 host pass.
  `findings/errands-render-gaps.md §7`.  **USER: studio `port-bedfix | retail-stairs` @ ~t2158 — the
  upstairs bed (top-right, white) should now be there; scrub up to t2325 as Arche climbs.**
- **Landed ckpt 182 — the upstairs shelf PILE "props missing" (USER note t2148) = a BAND/LAYER z-order
  bug (FIXED).**  The pile (books res1026 fr13 + stack/box fr34) is a STRUCTURE-band object (layer 8);
  the shelf-BACK it sits in (res1027 fr8/fr11/fr14) is a CHARACTER object stood in by ERRANDS_CAST at
  the DEFAULT layer 13.  `g_structs` renders before `g_room_cast`, so the L8 pile presented BEHIND the
  L13 shelf-back → hidden.  Retail draws the shelf-back behind (CHARACTER band walked before STRUCTURE
  in `0x48c150`, both layer 8; retail seq 282-284 < the pile 304).  FIX: the 3 upstairs shelf-backs →
  **layer 7** (== the downstairs fr9 shelf units already at 7).  VERIFIED off `port-shelffix.osr`
  (camera-aligned flip 5412): res1027 fr8/fr14 now seq #282/#286 BEFORE res1026 fr34 #300; pile
  reconstructs **`differ_px==0`** vs retail (x[290,360]); full-frame excl-HUD 1531→904 (only the 627
  pile px removed).  1096 host pass.  Open: fr64 (a downstairs shelf-back, seq 289) is drawcall-order-
  only (no overlap at t2148); the map-driven CHARACTER band retires all these.  `errands-render-gaps.md
  §8`.  **USER: click the studio shortcut (`port-shelffix | retail-stairs`) @ t2148 — the shelf pile
  (books/stack/box, upper-left, right of the HUD) should now be there.**
- **Landed ckpt 183 — the errands CHARACTER band is MAP-DRIVEN (retires the ERRANDS_CAST prop
  capture; USER "un-mvp the scene, session by session").**  The shop furniture/shelf/props now
  spawn from DATA-1025 via `actor_spawn_from_map` + `CHAR_BANK_DEFS` (code→(bank,layer) RE'd from
  the activator 0x431e30: install 0x426d70(0,BANK,variant); layer 0x438610(N) else default 9),
  with frame_base = the map variant (+0x18) — the SAME source the STRUCTURE band uses.  Shelf-BACK
  units 0x112db/dc + bookshelf 0x112d1 → **L5** (behind the L8 pile), rest L9 — SUBSUMES the
  ckpt-182 L7 fix (now exactly retail's L5) AND auto-fixes fr64.  Wired: `reload_room_backdrop`
  spawns g_actors for non-town; `game_actor_walk` renders the CHARACTER band every room.  VERIFIED
  off `port-charband.osr`: pile **differ_px==0**, full-frame excl-HUD 899px (5px BETTER than the L7
  build) = the known family-pose + Arche-phase residuals; TOWN t800 + HOUSE t1450 **0px** vs the
  pre-session build (no regression).  1096 host pass.  `errands-render-gaps.md §9`; `port-debt.md`
  errands-cast SHRUNK.  Still ERRANDS_CAST (next sessions): anim clock/pot (clip-update), additive
  fire, family+counter (party band).  **USER: click the studio shortcut (`port-charband |
  retail-stairs`) @ t2148 — the whole errands shelf/furniture is now map-derived, pixel-identical.**
- **Landed ckpt 184 — the errands ANIM props (clock + pot) are MAP-DRIVEN (retires their ERRANDS_CAST
  capture).**  The pendulum clock 0x112d9 / cooking pot 0x112da now spawn from DATA-1025 into the
  CHARACTER band (`g_actors`) with their clip: `CHAR_BANK_DEFS` +2 rows (bank 0x16b, **L9** = retail's
  default-9 via the `0x426ec0` anim-phase init, NOT `0x438610`), `actor_spawn_clip_for_code` (clock
  SWING {0,1,2,1} dur25 → 43,44,45,44 / pot STEAM {1,2,3,4} dur6 → 57..60), and the non-town per-tick
  loop now runs `actor_pool_update(&g_actors)` (was room_cast only).  Map pos == the ex-fit (clock
  52800,24800 / pot 67600,29600), nothing moves.  **VERIFIED bit-exact vs retail-stairs @ the CLAMP
  t2419-21** (both cameras pinned): pot seq282 fr57 @(228,208 28×35) + clock seq283 fr45 @(80,160
  29×40) == retail on draw/frame/dst AND z-order seq; anim phase aligns tick-equal; overlap scan
  "nothing over" either @ every clamp tick (L13→L9 no regress).  `actor_spawn_room_cast` 6→4.  1097
  host pass.  `errands-render-gaps.md §10`.  **USER: click the studio shortcut (`port-clockpot |
  retail-stairs`) @ the CLAMP (~t2420+) — the clock swings, the pot steams, both now map-derived.**
- **Landed ckpt 185 — the additive fireplace FIRE (0x112e4) is MAP-DRIVEN at LAYER 6 (retires the last
  ANIM ERRANDS_CAST member).**  RE'd off the 0x431e30 fire case (431e30.c:739): bank 0x1a3, FIRE_CLIP
  (0x407b80), additive ramp_a[14] (0x4385c0 DAT_008a92f0), **LAYER 6** (0x438610(6) — NOT the ex-capture's
  default L13; the ADDITIVE z genuinely matters, the fire must sit BEHIND the L8 grate/mantel).  Wired:
  `CHAR_BANK_DEFS` +1 row, `actor_spawn_clip_for_code` +fire, new `actor_spawn_alpha_for_code`
  (fire→14).  Pos exact by construction: map (32000,32000)+dst0 = the ex-fit's net (329,178).  The fire
  x=32000 is off-screen at the right-edge CLAMP → verify at the errands ENTRY (cam left).  **VERIFIED off
  `port-fire.osr` vs retail-stairs @t1710:** alpha res1034 @(329,178) bmode1, seq 262 vs retail 264 (both
  LOW = layer 6, not the ex-L13's ~518); osr_prof recon @t2040 **PIXEL-IDENTICAL** to retail (fire glows
  behind the grate+mantel; feed `ckpt185 fire PORT | RETAIL`).  ERRANDS_CAST → 3 (family+counter). 1097
  host pass.  `errands-render-gaps.md §11`.  **USER: feed `ckpt185 fire PORT | RETAIL` — proper fireplace.**
- **Landed ckpt 186 — the errands PARENTS are CHARACTER-band NPCs (NOT party-band); Father's Z-ORDER FIXED.**
  The retail-stairs CLAMP seq (t2420) DISPROVES the "family blocked on the party band 0x4997b0" model: the
  PARENTS (Father 0xe3 res1139 / Mother 0xb5 res1127) render in the CHARACTER band (0x1160 pool,
  `0x48c150:89-96` → `0x493ba0` param_3=0, interleaved with the shop props by SLOT-INDEX) — retail draws
  FATHER EARLY (#257, BEHIND the L8-structure floor items res1026 fr48/fr51 + the counter he overlaps),
  counter #287, Mother #289; only ARCHE (leader, +0x200c) is party-band (#290, `0x4997b0` param_2=1).  The
  port had the whole family at L13/frontmost (the ckpt-180b over-correction) → Father drew his legs OVER the
  floor items.  FIX: Father's ERRANDS_CAST layer 13→**7** (below the L8-structure floor-items he overlaps;
  L8 didn't clear them — g_room_cast emits after g_structs — which PROVES they're L8).  VERIFIED off
  `port-fatherz.osr`: Father seq #263 now BEFORE res1026 fr48/fr51 (#279/#282) + the counter (#315) == retail's
  Father-behind order; clean placement (#258-261 L5 shelf-backs → #262 L6 fire → #263 Father L7 → #265+ L8/L9).
  counter + Arche already bit-exact; Mother/Father anim-PHASE (fr0 vs fr2 / fr5 vs fr6) stays RNG-blocked
  (`effect-anim-phase`/0x426ec0).  1097 host pass; town/house use no ERRANDS_CAST (no regress).  Provenance:
  the parents are persistent handles 0x5f5e1d3/1d4 placed by the errands script (0x4dc510 case-7/8 → 0x41ec20),
  so the retire path is that SCENE-SCRIPT spawn, NOT the party band.  `errands-render-gaps.md §13`.  **USER:
  click the studio shortcut (`port-fatherz | retail-stairs` @ the CLAMP t2420, now CAMERA-ALIGNED via the fixed
  Z-spam recipe below): Father the shopkeeper renders BEHIND the counter/register/vase == retail.  Feed
  `ckpt186: Father z-order VERIFIED camera-aligned` — the port|retail DIFF is ONLY the known residuals (Father's
  breathe anim-PHASE fr5-vs-fr6, RNG-blocked + the HUD item-bar stand-in), NO z-order artifact.**
- **Landed ckpt 187 — the house→errands TRANSITION diagnosed: backdrop renders 1:1, the "drift" is a STALE NAV
  (retires `cutscene-room-render`).**  (a) `cutscene-room-render` was STALE + WRONG — the house (DATA scene 1023)
  + errands (1025) BACKDROPS DO load/render (`main.c:3806` room-key swap): frame-verified port house-dialogue
  @t1600 == retail-stairs @t1445 (IDENTICAL house-EXTERIOR "Liens" backdrop + family + portrait; 0x334c8 is the
  EXTERIOR, not an interior — the debt's "plays over TOWN backdrop / multi-floor interior" both false).  Feed
  `house dialogue backdrop renders 1:1`.  (b) the ckpt-186 OPEN "port drifts behind" = a STALE-NAV artifact:
  `nav-full-errands`'s confirms were keyed to `retail.osr` (**deleted** — only retail-stairs/retail-decomp remain),
  whose schedule differed from retail-stairs by ~8-90t.  (c) retail's transition ENVELOPE measured (read as a
  ~9t black-HOLD → **SUPERSEDED ckpt 188: that was a MISREAD of the slow EDGES-IN reveal, not a room-load hold;
  the real fix was the reveal SHAPE, see the ckpt-188 bullet**).  A re-keyed nav still won't lockstep (port ~8t
  speaker-change reopen + L7/L9/L17 beat gaps → needs beat-aware placement); Z-spam stays the clamp recipe.
  `errands-render-gaps.md §14`; `cutscene-room-render` RETIRED.  New: `osr_prof dump` headless frame recon +
  `dialogue_timeline` re-keying.
- **Landed ckpt 188 — the errands ENTRY reveal is EDGES-IN, not center-out (the "−6t hold" was a
  MISDIAGNOSIS; retires `cutscene-errands-entry-latency`).**  A fresh `retail-stairs.osr` pixel
  measurement (whole-screen mean + row-band per sim-tick, `osr_prof` recon) DISPROVES the ckpt-187
  "insert a ~9t room-load black-HOLD" plan: retail's full-black CORE is only **~2t** (t1650-1651);
  the long "black period" §14c read as a 9t hold is the SLOW **EDGES-IN reveal** keeping the vertical
  CENTER dark while the edges clear (top+bottom light first ~t1652-1655, center fills **LAST**
  ~t1685-91 — NOT top-down, NOT center-out).  The port's REAL bug = the reveal SHAPE: it armed
  **CENTER-OUT (variant 0)**, sourced from the now-DELETED `retail.osr` (ckpt 179).  FIX = one line,
  `main.c` `scene_fade_arm(…, /*variant=*/1)` (EDGES-IN, == the port's own `HOUSE_EXIT` cover, its
  mirror): the port reveal now matches retail BAND-FOR-BAND at aligned phase (port t1770
  `[70,125,42,86,79]` ≈ retail t1690 `[70,125,66,86,79]`; center-band last to clear both).  A 9t
  hold was TRIED (188a) + REVERTED — it made the port full-black 13t vs retail's 2t (wrong
  mechanism).  Reveal DURATIONS already matched (~40t) ⇒ the fabled "−6t" was ~measurement noise off
  the old fuzzy anchors, not a missing load latency.  Folds into `cutscene-fade-variant` (**⚠ re-
  verify the arrival→house cover/reveal variants vs retail-stairs — same dead-capture era**).  1097
  host pass; `errands-render-gaps.md §14d`.  **USER: feed `edgesin_cmp.png` (port|retail edges-in,
  phase-aligned) — the errands now opens edges-first / center-LAST == retail.**
- **Landed ckpt 188b — the arrival→house COVER + house-entry REVEAL had the SAME bug (that ⚠ flag,
  now CLEARED).**  Re-measured ALL FOUR town-intro fades vs retail-stairs: each variant is an
  INDEPENDENT RNG roll, and 3 of 4 were forced to the wrong center-out (variant 0) from the dead
  `retail.osr`.  Corrected in `cutscene.c`: (1) arrival→house COVER = **TOP-DOWN** (`ARRIVAL_EXIT_
  COVER_VAR` 0→2), (2) house-entry REVEAL = **EDGES-IN** (`HOUSE_ENTRY_REVEAL_VAR` 0→1); (3) the
  house-exit cover (var 1 edges-in) was already right; (4) the errands reveal (0→1) = ckpt 188.
  VERIFIED off `port-ahfix.osr` BAND-FOR-BAND — the cover is EXACT (port t1258 `[0,0,0,0,0,2,9,23,35,0]`
  == retail t1224; top darkens first), the reveal is edges-in (center last).  1097 host pass;
  `errands-render-gaps.md §14d.2`.  **USER: feed `ahfix_cmp.png` (port|retail cover+reveal) — the
  arrival darkens top-down, the house opens edges-in == retail.**
- **⚠ TOOLING (ckpt 186): the freeroam CLAMP capture recipe — DIAGNOSED + a WORKING recipe.**  `nav-full-errands`
  alone leaves Arche IDLE at spawn (never walks → camera stays world-left, NOT the clamp).  ROOT CAUSE (logged +
  confirmed): `freeroam_begin` DOES fire and the 3-line errands opening dialogue DOES arm, but `nav-full-errands`'s
  L18-L20 confirms (@t1763-1837, retail-tick-keyed, authored Jun 19) are STALE for the current build — the dialogue
  never completes, so `cutscene_active` stays true and `freeroam_step`'s control gate (`g_errands_dlg_pending ||
  cutscene_active`) never releases → Arche is locked.  **FIX (VERIFIED end-to-end — Arche walks to the right-edge
  CLAMP, camera pins == retail: `port-fatherz.osr` @t2420 has Father @(32,392) #257 / counter (8,360) / Mother
  (176,200) / Arche (398,248) all == retail-stairs, and the excl-HUD pixel-diff is just the anim-phase; scene
  reconstructs camera-aligned):**  Arche walks, fr8 @t2000, camera scrolls
  right):** a MONOTONIC errands Z-spam.  Regenerate `runs/cutscene-verify/nav-errands-spam.jsonl` (gitignored, so
  it must be regenerated) = `nav-full-errands`'s boot frame-lines + its tick-lines ≤1668 (arrival+house) + a dense
  `{"tick":T,"ids":[36]}` spam every 10 ticks over 1700-1900 (the dialogue window; keep spam ticks MONOTONIC after
  the last kept ≤1668 tick, else the whole trace fails to parse).  Drive with `--held-trace runs/sync/hold-right-
  clamp.jsonl` (`{"tick":1650,"keys":[205]}` = hold RIGHT).  (2 spam captures were externally KILLED mid-write ~t2000
  — re-run if the clamp frame isn't reached.)  SEPARATE observed gap **RESOLVED (ckpt 187): the drift is a
  STALE-NAV artifact, NOT a port bug.**  `nav-full-errands`'s confirm ticks were extracted from **`retail.osr`**
  — a capture that **no longer exists** (only retail-stairs + retail-decomp remain); its advance schedule differed
  from retail-stairs by ~8-90t, so the port dialogue advanced at the WRONG ticks → apparent drift.  The house
  BACKDROP is FINE: frame-verified port @t1600 == retail-stairs @t1445 (identical house-EXTERIOR backdrop — retires
  `cutscene-room-render`, §14).  A re-keyed `nav-errands-stairs.jsonl` (skip@full/adv@adv from retail-stairs)
  still won't lockstep — the port's ~8t speaker-change reopen + the L7/L9/L17 BEAT gaps need beat-aware confirm
  placement; the Z-spam stays THE clamp recipe.  `findings/errands-render-gaps.md §14`.
- **Next move (finish the errands un-MVP, session by session):** (1)(2) DONE ckpt 184/185 (clock/pot, fire).
  (3) **RESCOPED by ckpt 186** — the errands PARENTS are character-band NPCs (z-order now fixed); the LAST
  errands stand-ins are the parents' anim-PHASE (RNG-blocked, `effect-anim-phase`/0x426ec0 — needs the scene
  RNG census, Phase 2) + their POSITION provenance (the `0x41ec20` scene-script spawn stand-in).  Only
  ARCHE-as-leader + her multi-part body still need the party band `0x4997b0` (`cutscene-party-chars`; leader
  path + `0x402730/0x402330` movers) — Arche is already the freeroam char, so this is provenance, not a
  visible gap.  (4) **DONE ckpt 188 — the errands entry reveal is EDGES-IN, not center-out;
  `cutscene-errands-entry-latency` RETIRED (the "−6t = a skipped ~9t hold" was a MISDIAGNOSIS — no
  such hold; the real bug was the reveal SHAPE, see the ckpt-188 bullet above).**  (5) the HUD
  party-context (blocked on the party subsystem).
  THEN the older items below:
  (a2') **errands entry — RESOLVED ckpt 188 (the "−6t 9t-hold" was a misdiagnosis).**  A fresh
  retail-stairs pixel measurement DISPROVES the ckpt-187 "insert a ~9t black-hold" plan: retail's
  full-black core is only ~2t; the "9t black period" was the SLOW EDGES-IN reveal (center clears
  LAST).  The port armed CENTER-OUT (variant 0, from the deleted retail.osr) — FIXED to variant 1
  (edges-in), matching retail band-for-band.  Reveal durations already matched (~40t) ⇒ the "−6t"
  was ~noise.  `errands-render-gaps.md §14d`; folds into `cutscene-fade-variant`.
  (b) **house props (mark t2278) — DIAGNOSED + the cabinet FIXED (ckpt 180).**  The POT
  (res1074) is NOT missing: it renders BIT-EXACT in the walk-aligned `port-stairs2` vs
  retail-stairs @t2278 — the "missing" was a `port-waitfix` STALE-TRACE artifact (the
  ckpt-179 waitfix desynced the tick-keyed `synth-stairs` held-walk → +176px camera phase
  lag).  The real gap = the KITCHEN CABINET (res1023 fr4 @280,96 + fr2 upstairs hutch),
  errands-map (DATA 1025) CHARACTER code 0x112d1 map layer[18]/[31] — FIXED as 2 ERRANDS_CAST
  entries (world 70400,25600 / 70400,6400; verified 172px offset == map).  (region-E is
  WAYPOINTS, not props — the old "prop spawn consumer" framing was wrong.)  **⇒ freeroam
  studio pair = `port-stairs2 | retail-stairs`, NOT port-waitfix (stale past ~tick 2000).**
  `findings/errands-render-gaps.md` §6.  Mom's pose (§12): NOT a wrong clip — retail Mom breathes
  res1127 fr2→fr3, the port animates the SAME clip at a ~1-frame PHASE offset (dst identical when
  frames match), a phase residual tied to the −6t entry latency + the family clip_phase, resolved by
  the party band, not a frame fix.
  (c) `char-turn-state` — **DONE ckpt 177** (`findings/freeroam-turn-around.md`).
  (d) HUD: door-indicator spawn source / bottom strips; `mover-actor-scan` when
  collidable actors matter.
- **Open PORT-DEBT (this front):** `mover-actor-scan`,
  `char-drop-through`, `char-reverse-decel`, `char-input-autorepeat` (the residual
  1-tick reversal onset); HUD: `hud-party-context`,
  `hud-door-actors`, `hud-slide`/`hud-item-hslide`. See `port-debt.md`.
- **Standing bar:** every divergence is `differ_px==0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic (`parity-model.md`);
  seed-pinned both sides, compared by anchor/tick — never the flip axis.
- **Read next:** changelog → `PROGRESS.md`; deep RE → `findings/` (esp. `freeroam-hud.md`,
  `freeroam-brake-onset.md`); module layout + open threads → `memories/HANDOFF.md`
  (last rewritten ckpt 155 — stale on the HUD arc).
<!-- FRONT:END -->
