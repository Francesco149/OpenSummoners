<!--
  The ONE hand-edited status block.  tools/gen_port_ledger.py injects everything
  between the FRONT:BEGIN/END markers verbatim into docs/STATUS.md's "Current front"
  section, so STATUS can never drift from reality.  Update THIS when the active front
  moves; keep it a 60-second read.  Everything else in STATUS is derived from code.
  Port state / next-move detail belongs here or in HANDOFF.md, NOT in engine-quirks.md.
-->
<!-- FRONT:BEGIN -->
- **Phase:** Phase 4–5 — porting the **in-game town backdrop** render path toward a trace
  that plays 1:1 pixel-perfect frame by frame on both sides. Milestone map: `ROADMAP.md`.
  Mechanical next chip: `port-frontier.md`.
- **LATEST (ckpt 91b): the PARTY-character system is SCOPED + PLANNED (`docs/plans/
  party-character-system.md`, USER-approved) and the cast is GROUND-TRUTHED + USER-confirmed.
  There is NO decode bug — res `0x477` is the MAN (rendered correctly); the woman (Arche's
  mom) + Arche are MISSING party characters whose sheets the port never loads. No port code
  ported yet (RE + planning; 911 pass unchanged).**
  1. **Runtime `bank = registration_idx + 13`** (PROVEN: the bit-exact tree is bank `0x15f`=351,
     res `0x481`@idx 338). This corrected the ckpt-90 `bank=idx` reads. (ckpt 91's *further*
     claim "woman=`0xc3f0`, decode bug" was itself WRONG — a cross-run flip-misalignment
     artifact; superseded here.)
  2. **The cast, USER-confirmed (settled-state aligned, `findings/in-game-intro.md` "DEFINITIVE
     (ckpt 91b)"):** `0xc3f0` bank `0xeb`→res `0x477` = **the MAN right of the horses, renders
     CORRECTLY**; `0xc3dc`→res `0x473` + others = townsmen; **`0xc35a` bank `0x8b`→idx 126
     (UNREGISTERED → CULLS) sits at center where Arche + the woman (mom) stand** — ckpt-90 was
     RIGHT. NO decode bug: sotesd.dll res `0x477` is the man (port + offline decode agree);
     it's the only sprite source (sotesp.dll lacks it; the EXE's res 1143 is `MPED2DT` map data).
  3. **So the woman + Arche are missing because the party/character system that loads their
     sheets is unported** — exactly the approved plan. **NEXT = Phase 1** (the dramatist/handle
     registry `0x556eb0`/`DAT_008a9b50+0x2790` + per-character sprite loading) → Phase 2 (party
     band `0x4997b0` + the rich `0x493ba0` arms) → Phase 3 (walk-in + dialogue `0x49d6e0` +
     `0x5a00c0` overlay). **EXE-embedded sheets (if any) must be extracted from the user's
     `sotes.exe` at runtime or cached in `%APPDATA%` — never embedded (USER directive).**
- **Prior (ckpt 90): two golden-review gaps chased — the establishing REVEAL is RE'd
  (a fade-grid, NOT the letterbox) and the town-intro cutscene NPCs are PORTED; the woman +
  little girl are PLAYER-PARTY characters, render path now SCOPED (PARTLY WRONG — see ckpt 91).**
  1. **The establishing REVEAL = a per-cell FADE-GRID transition, not the letterbox bars
     (committed, quirk #90).** Pixel envelope: top/bottom black ramps ~240→64 at −8px/sim-tick.
     **Refuted by live field capture** — both `0x499ab0`'s and the grid-fill `0x48c150`'s bar
     heights read **constant 64** the whole reveal (scroll 0). The real producer is **`0x48e920`**
     (a 64×4 per-cell black-tile fade-grid, the center-out iris; `ret_va 0x48e9c3` emits
     ~1010→0 tiles), rendered from `0x48c150:175`, updated 2×/tick by **`0x49af40`** from the
     cinematic step `0x499ab0`. Explains the long-open ckpt-66/67 "dark top gradient". PORT:
     unported (the reveal chip — port `0x49af40`+`0x48e920`+trigger). `findings/in-game-intro.md`
     "The establishing REVEAL is a per-cell FADE-GRID".
  2. **The town-intro cutscene NPCs (in front of the wagon) PORTED — `actor_spawn_cutscene_cast`
     (committed).** USER flagged 4 missing characters at the pan end. RE'd the spawn (cutscene
     `0x4d7d80` → anchor-relative `0x41f0e0` → `0x41f200`, positioned vs the wagon's anchor
     `0x65`) + captured the settled census (`runs/cutscene-cast`). Ported the 3 EFFECT spawns
     to `g_effects`; **`0xc3dc`+`0xc3f0` (banks `0xe3`/`0xeb`) RENDER** (the 2 NPCs near the
     horse), facing fixed with the in-scene `DAT_008a8440` flip read (=4). **`0xc35a` (the
     woman) CULLS + the little girl is absent — both are PLAYER-PARTY characters.** 911 pass.
  3. **The PARTY render path SCOPED (next arc, USER-chosen).** The party renderer **`0x4997b0`**
     (150 B) just iterates the 8 party actors (`room_state+0x4030`, reset by `0x560e60`) and
     renders each via **`0x493ba0`** — the renderer the port ALREADY reuses. The woman is a
     keyed blit **res `0x477`**; the little girl (the controllable-character actor) renders via
     a richer path (no keyed blit). **Blocker = bank registration** (`game_sprites[]`), with a
     mismatch to resolve (census bank `0x8b`→res `0x4fb` ≠ the rendered `0x477`; `0xc35a`'s
     `+0x48` is a party indirection). **NEXT:** find the bank for res `0x477` → spawn the woman;
     RE the party spawn + wire `0x4997b0` for the girl/protagonist (gateway to the
     controllable-character milestone, Phase C). `findings/in-game-intro.md` "The PARTY-character
     render path is SCOPED". PORT-DEBT(cutscene-party-chars).
- **Prior (ckpt 89): the SKY-AMBIENT particles (`0x18704` = CHIMNEY SMOKE) are PORTED +
  USER-1:1, and the placement is now TRACE-FAITHFUL (anchor + facing fixed from retail).**
  Chip 4: the town's second particle system, built on the ckpt-88 pool/alpha path.
  **PORTED (`src/particle.{c,h}`):** emitter `0x112e2` (`0x54f980:150`, spawns 1 every 6th
  tick), config `0x557550:630` (bank `0x1aa` frame 8, clip `0x644b58` = 6-frame **ONESHOT**
  decoded from the exe, layer 6, `0x453960` velocity scatter), step `0x46e510:683` (vel_y
  decel→-5000, integrate, **expire on the oneshot done flag**, ramp_b fade), and the **ramp_b
  alpha** path (`game_present_blit` decodes `param8 = (ramp_sel<<8)|idx` → ramp_a water /
  **ramp_b** `0x8a9308` sky). **WIRED (`main.c`):** finds both `0x112e2` props, emits each
  sim-tick into the shared `g_fountain_pp`. **USER-confirmed "smoke looks 1:1"** + the USER
  independently spotted the same chimney smoke in retail. **TRACE VERIFICATION (USER directive)
  caught + fixed 2 RNG-independent bugs:** (a) **anchor** — I'd HARDCODED +1600; the faithful
  `0x557370` mode-1 anchor is render-state +0xc/2, and the invisible `0x112e2` trigger has
  +0xc==0 → **anchor 0** (spawn at the prop's exact world pos); removed the constant. (b)
  **facing** — `runs/sky-facing` shows every particle has +0x2c==**1** → x `+= +vel_x/100`
  (no flip) → the sky **drifts LEFT** (matching retail); the port spawned facing 0 → drifted
  right; fixed `particle_spawn_{water,sky}` to facing 1. After both: port sky world X
  `[51440..113369]` ≈ retail `[50690..114356]`, Y matching. **911 pass** (+5), ledger
  unchanged. quirk #88; `findings/in-game-intro.md` "The SKY-AMBIENT particles". **The town's
  `+0x13e0` band now renders BOTH its codes (`0x18704`+`0x18708`) — no particle remainder.**
- **Full-intro side-by-side VIDEO (ckpt 89, USER-requested).** Frame-matched (anchor-aligned)
  retail|port across title→newgame→prologue→town, 64 pairs (`/tmp/intro_sidebyside.mp4` +
  a feed montage). **title/menu 1:1, prologue aligned, town establishing 1:1** (backdrop +
  fountain + decorations + townsfolk + chimney smoke all match). The one clear divergence the
  sequence surfaces: retail's **"Town of Tonkiness" area banner** (~retail flip 1600+) is
  MISSING in the port = the `0x5a00c0` scripted-overlay debt (PORT-DEBT `ingame-nontile-layers`;
  a TIMED element, absent at the hold — consistent with ckpt 82). **NEXT — the USER's
  golden-video review (ckpt 89) flagged 5 concrete items (quirk #89):** (1) the **establishing
  REVEAL** — a VERTICAL FADE opening from the MIDDLE of the screen outward (CONFIRMED off the
  golden; this IS the long-open "dark top gradient" of ckpt 66/67 — a vertical-iris reveal, not
  a static tint; the port jumps straight to the letterboxed scene). (2) the **`0x5a00c0`
  banner/scripted overlay** ("Town of Tonkiness", timed ~retail flip 1600+). (3) **ground
  BUTTERFLIES** by the flowerbeds at the SETTLED town (~flip 2150) — likely a `0x557550` "leaf"
  particle code (`0x18707`/`0x18709`) the hold-only census missed; RE via a render trace at the
  settled town. (4) the Start-Game menu **SCALE transition** (scales IN on appear + OUT on confirm; port pops
  it in/out). (5) **phase-match
  the particle RNG** (PORT-DEBT `fountain-rng-phase`, Phase 2). PORT bug to verify: the
  menu-cursor pulse looked fast (dev-build frame rate? — TODO).
- **Prior (ckpt 88): the FOUNTAIN SPRAY (`0x18708`) is PORTED + USER-confirmed** — the
  particle subsystem RE'd (1024-slot `+0x13e0` pool, alloc `0x557370` / config `0x557550` /
  step `0x46e510` / `0x493480` alpha render); translucent water via ramp_a
  (`g_ramp_a[10-sub_phase]`). `src/particle.{c,h}` NEW; quirk #87.
- **Prior (ckpt 87): the townsfolk IDLE ANIMATION PHASE is PORTED — they now breathe
  from a per-actor RNG start frame instead of frozen on frame 0.** Builds on the ckpt-86
  anchor; the first user-visible payoff of the spawn-RNG arc. **The model (engine-quirk
  #86, decompile- + census-verified):** for each of the 15 map EFFECT objects in
  dispatch (layer) order, `0x41f200` consumes `8 + extra` draws (`0x426fd0`(1) + prologue
  (7); extra = 5 for the 4 `0xe29a` wanderers via `0x427670`, 1 for `0xe2a5` via
  `0x431cb0`, else 0) then `0x426ec0`'s 2 idle-phase draws (`frame=(rand*20)>>15`,
  `timer=(rand*14)>>15`). All 11 rendered townsfolk fall in the first 15 effects (the 4
  script effects spawn after), so they are unaffected by the rest. **PORTED:**
  `actor_spawn_effect_from_map` replays the per-object draws in map order (consume-to-
  advance; only the `0x426ec0` pair is used, only for the rendered townsfolk — the
  wanderers/unknowns still consume their draws), embeds the shared idle clip `IDLE_CLIP`
  (= `0x6290e0`: base 0, 20f, dur 14, looping, delta {0,1,2,1,…} — decoded from the exe),
  and sets `rs->clip`/`frame`/`timer`; `game_actor_update` now also advances `g_effects`
  per sim-tick (RNG-free `anim_clip_advance`) so they breathe in lockstep. **Verified:**
  host test locks the replay to a reference LCG (frame/timer per slot); the draw model is
  census-verified (counts 134/38/20/19) + decompile-verified (the `0xe29a`/`0xe2a5`
  cases); offline from `0x4f5347` the 11 start frames are {1,17,17,17,3,14,4,16,18,12,10}.
  **898 pass, ledger 199/194 unchanged** (bare-VA slice of `0x41f200`). quirk #86;
  `findings/in-game-intro.md` "The town SPAWN RNG anchor" + the idle-phase note.
  **VALIDATION PENDING (not yet differ_px==0):** a bit-exact cross-check of the port's
  `+0x72` per townsperson vs retail (a `0x426ec0` onLeave capture, or render_diff at a
  matched sim-tick) — the chain is complete but the live pixel/`+0x72` diff is the next
  step. **THEN Chip 3 — the FOUNTAIN SPRAY** (band `+0x13e0`/`0x493480`; the `0x427b70`/
  `0x427670`(20) particle init + per-tick `0x47b990`/`0x453960`).
- **Prior (ckpt 86): the town SPAWN RNG ANCHOR is LANDED + LIVE-VERIFIED — the keystone
  for the two remaining RNG residuals (idle PHASE + fountain SPRAY).** Phase-2 matching
  half, the foundational chip (no visual change yet — that lands when the spawn consumers
  are ported, Chip 2). The title→town RNG is non-deterministic run-to-run even under the
  boot seed-pin (quirk #77; game_enter seed was `0x46fe3f46` this run vs `0x83600390`
  last), so the town SPAWN draws started from an unpredictable phase. **FIX:** re-pin
  `DAT_008a4f94` on BOTH sides at the spawn start. **Ground truth (engine-quirk #86, the
  seed-pinned `0x5bf505` census `runs/rng-census-repin`):** the town-LOAD frame draws a
  fixed **238-draw burst over 19 EFFECT objects** (`0x58d460`→`0x41f200`, map order; the
  port renders 11 but ALL 19 consume RNG). Per object: `0x426fd0`(1)+`0x41f200`(7 jitter
  +particle-params)+optional `0x427670`(5, 4 objects)+`0x426ec0`(2 idle frame/timer).
  **The re-pin point is the FIRST `0x41f200`, NOT game_enter** — a pre-spawn one-off
  `0x4c5e00`(1 draw) sits between them (so a game_enter pin would desync the port by one
  draw). **Agent:** `installRngAnchor()` arms at the game_enter anchor, writes the seed at
  the first `0x41f200` onEnter (`rng_anchor` event). **Port:** `game_rng_seed()` helper +
  `rng_srand` re-seed at `enter_game` top (faithful — all pre-effect-spawn code is
  RNG-free). **VERIFIED LIVE:** `re-pinned @ frame 1419: 0x71cc78f1 -> 0x004f5347`
  (`before` ≠ game_enter seed ⇒ the intervening draw is real); spawn draw counts
  byte-identical pre/post (134/38/20/19) ⇒ values reset, control flow untouched. **898
  pass, ledger 199/194 unchanged** (harness+seam, no fn ported). quirk #86;
  `findings/in-game-intro.md` "The town SPAWN RNG anchor". **NEXT (Chip 2):** port
  `0x41f200`'s 19-object RNG consumption in order, give the 11 townsfolk the idle clip
  `0x6290e0` + set `+0x72`/`+0x70` from the aligned `0x426ec0` draws → idle phases 1:1;
  then the fountain (Chip 3).
- **Prior (ckpt 85): townsfolk FACING is PORTED + USER-1:1 — it's a deterministic MAP
  field, NOT RNG (corrects the ckpt-84 guess).** Phase-2 matching half, first chip. RE'd
  the three ckpt-84 RNG residuals: **facing is RNG-FREE** — the dispatcher `0x58d460:96`
  computes `cVar12 = (puVar1[4]!=0)?3:1` from the map sub-record `puVar1[4]` and forwards it
  as **param_8** to `0x41f200`/`0x431e30` → render-state `+0x2c`; `0x44d160` mirrors the cel
  (`frame += flip`) + reflects `off_x` only on `facing==3`, where **`flip = *(s16)(DAT_008a8440
  [bank])` = the sprite group's frames-per-direction** (`0x8a8440` confirmed live a POINTER
  array → heap descriptors; 4 or 16 for the town banks). Live census (the `0x493ba0` spec +
  a new `rs_facing` field + a one-shot read of `DAT_008a8440`): of the 11 map townsfolk **7
  are facing 3** (`c3be/c3dd/c3e6/c422/c42c/c441/c468`), 4 normal. **PORTED:** `TOWN_EFFECT_
  DEFS` gains `facing`+`flip`; `actor_spawn_effect_fill_flip_table` fills the bank-indexed
  stand-in for the global `DAT_008a8440`, wired into every `game_actor_walk` render call;
  **898 pass**, builds clean. **USER-confirmed: "npc orientation matches retail yes."** quirk
  #85; `findings/in-game-intro.md` "Townsfolk facing is a MAP field". (Townsfolk still
  frozen-frame — the idle anim PHASE is RNG, next.) PORT-DEBT `effect-sprite-table` extended.
  **THE REMAINING TWO RESIDUALS ARE RNG → need the game_enter RNG ANCHOR:** (1) **idle PHASE**
  — `0x426ec0` sets `rs+0x72 = (rand()*clip.frame_count)>>15` (every townsperson runs clip
  `0x6290e0` at a random start frame); (2) **the FOUNTAIN SPRAY** (band `+0x13e0`/`0x493480`)
  — `0x41f200`'s 8 rand draws are position-jitter (`0x426e00` `+0x58`/`+0x60`) + a particle
  sub-spawn (`0x427b70`); helper `0x427670` (20 draws) + per-tick `0x47b990`/`0x453960` drive
  the spray. **NEXT:** re-pin `DAT_008a4f94` at `game_enter` both sides → port the spawn RNG
  consumers in order → idle phase + fountain land 1:1.
- **Prior (ckpt 84): the EFFECT townsfolk are PORTED — positions USER-confirmed 1:1; the
  residual is now PINNED to the RNG pillar (Phase 2 begins).** Landed the EFFECT band (the
  standing villagers in the square) positioned 1:1, frozen on the idle clip's frame 0 (the
  wagon/STRUCTURE precedent). **The render REUSES `actor_render_static`** — for a plain
  townsperson `0x493ba0`'s static arm reduces to the ported describe (`0x44d160`) + emit
  (`0x492670`): exactly ONE mode-0 keyed cel each (verified vs the hold blit trace — no
  `0x4917b0` shadow, no `0x8a9358` color-remap). **Placement FULLY MAP-DRIVEN:** `world =
  (map (x,y) − dst) × 100` (the +30 world offset cancels the −30 render dst → screen = map −
  cam; derived cel-for-cel vs the census `rs_x`). The 11 map townsfolk = 10 `0xc3xx` + `0xe2a5`.
  PORTED `actor_spawn_effect_from_map` (`g_effects`, walked at layer 13) + the captured def
  table (PORT-DEBT `effect-sprite-table`). **898 pass** (+1); ledger 199/194 unchanged
  (bare-VA slices of `0x41f200`/`0x493ba0`). **USER-confirmed: "the NPCs are rendering at the
  correct positions."** quirk #84; `findings/in-game-intro.md` "The EFFECT townsfolk PORTED".
  **THE RNG RESIDUAL (USER directive — pivot to Phase 2):** the scene is NOT yet 1:1 every
  frame because of THREE RNG-driven elements: (1) **townsfolk FACING** — some render flipped
  (`0x44d160`'s `facing==3` mirror; the port spawns facing 0 + `flip_table NULL` → no mirror;
  facing is likely an RNG draw at spawn); (2) **townsfolk idle PHASE** — frozen frame 0; the
  clip `0x6290e0` (20f dur 14) + stepper are ported but the per-actor START phase is staggered
  (likely RNG); (3) **the FOUNTAIN PARTICLE SPRAY** — the entire `+0x13e0` band (`0x493480`,
  res `0x408`) is MISSING (USER pointed out the purple/blue spray + leafy particles); RNG
  positions. PORT-DEBT `effect-anim-phase`/`effect-wanderers`.
- **RNG-CONSUMER CENSUS DONE (ckpt 84) — integrated into the flow trace.** Per the USER
  ("integrate, not a bespoke probe"): added `0x5bf505` (the LCG) as a `retail_fields.json`
  entry (the auto `ret_va` names the consumer site) + `tools/rng_consumer_census.py`. Captured
  1142 town-scene draws (game_enter@1434), cross-checked vs the decompile (`0x41f200` has
  exactly 8 rand calls = the 8 sites). **SPAWN (room-load):** `FUN_0041f200` (the EFFECT
  activator, 134/8 sites = townsfolk facing+phase) + helpers `0x426ec0`/`0x427670`/`0x426fd0`.
  **HOLD (per-tick):** `FUN_0054f980` (behaviour/wander 425), `FUN_0047b990` (the `+0x1160`
  EFFECT update = fountain particles/wander 320), `FUN_00453960` (154). Retires the ckpt-73
  defer-all-RNG. `findings/in-game-intro.md` "The scene-wide RNG-consumer census". **NEXT = the
  MATCHING half:** RE each consumer's draws (facing/phase/particles) → an RNG ANCHOR at
  game_enter both sides (re-seed the town scene; the port can't replay the whole boot RNG chain)
  → annotate producers (`rngcalls` + port `CALL_TRACE_BEGIN`) + `flow_diff` → port in order → 1:1.
- **Prior (ckpt 83): the establishing-hold CAST is PINNED to FOUR map-object bands — the
  Phase-1 producer map is complete (no port yet, RE+census milestone).** Resolves ckpt-82's
  "pin each cast cel to its actor" via a field-spec **band census** (`retail_fields.json`
  gained the 5 non-main band render entries `0x4937c0`/`0x493480`/`0x492fc0`/`0x493230`/
  `0x493ba0` + emit prims `0x492670`/`0x4917b0` with `renderid` on the cel). The driver
  `0x48c150` runs 8 emit passes → one present `0x48eac0`; the cel↔producer tie is at EMIT
  (the node carries no back-ref). **The 18 visible keyed cels = 4 bands, all DATA-1022 map
  objects:** **`+0x2560` STRUCTURE (`0x493230`, single-cel)** = the **TREE** (`0xec55` bank
  `0x15f`→res `0x481`) + bg decorations (`0xec6a`→`0x16c`/`0x403`) + fg hedges (`0xec60`→
  `0x164`/`0x426`, layer 15); **`+0x1160` EFFECT (`0x493ba0`, multi-part)** = the townsfolk
  (10 distinct `0xc3xx` + `0xe29a`×4 + `0xe2a5`); **`+0x11e0` CHARACTER (`0x491ae0`)** =
  collision volumes + props + wagon (**already ported**); **`+0x13e0` (`0x493480`)** = bank
  `0x1aa`→res `0x408` particles (alpha, not keyed — deferred). **STRUCTURE is fully
  map-driven:** pos = map (x,y)×100, **frame_base = map variant@+0x18** (verified
  cel-for-cel). EFFECT townsfolk map 1:1 by code/count + a deterministic spawn offset
  (≈+3000 x, from `0x41f200`); `0xc35a`/`0xc3dc`/`0xc3f0` are script/party-spawned (not in
  the map). **Hold = 16 static townsfolk + 39 static structure objects (only the anim frame
  steps, det. per #76) + 4 wandering `0xe29a` (RNG, Phase 2).** Corrects the docs' "the cast
  is the +0x11e0 band" model; the tree is STRUCTURE `0xec55`, not a banner/`0x5a00c0`/tile.
  quirk #84; `findings/in-game-intro.md` "The establishing-hold cast is FOUR map-object
  bands".
- **LANDED (ckpt 83b): the STRUCTURE band is PORTED + USER-confirmed 1:1.** RE'd the
  STRUCTURE activator `0x438a60` (per-code bank def table) + dispatcher `0x58d460`
  (layer = record +0x30 ? 15 : 8, frame_base = variant +0x18 — all verified vs census).
  PORTED `actor_spawn_struct_from_map` (60000-range, fully map-driven) + the bank table
  `actor_spawn_struct_bank_for_code`; the render REUSES `actor_render_static` (the
  `0x493230` static single-cel blit is bit-identical), wired via `game_actor_walk`
  (g_structs) at the structure layers 8/15. **Live: 39 objects spawned + emitted; tree bank
  0x15f registered.** **render_diff/position-verified BIT-EXACT:** tree `(0x481,f0)`@(496,64)
  320×320, the 5 hedges `(0x426)`, the 4 `0x403` props/deco — all identical port↔retail,
  zero `[rect]`/`[decode]`/`[state]`. **USER-confirmed on the feed: "the decorations are
  there and positioned 1:1".** parity-ledger #9; **897 pass** (+1, `actor_spawn_struct`);
  ledger 199/194 unchanged (bare-VA slices). **NEXT (Phase 1 cont.):** the EFFECT townsfolk
  — the multi-part char renderer `0x493ba0` (built on the ported `0x44d160`) + the
  `0x41f200` spawn (map pos + the ≈+3000 x offset); 16 static land at a matched sim-tick,
  the 4 wandering `0xe29a` need Phase 2 (RNG). Then the `0x4962a0` invisibles + the `0x13e0`
  particles. Also pending: byte-confirm the wagon via `render_diff` `(res 0x3ec, frame)`.
  Artifacts (`/tmp`): `cast_census/`, `port_hold_trace.jsonl`, `retail_hold_1500.png`.
- **Prior (ckpt 81): the caravan's HORSES now TROT — the per-tick actor anim is wired +
  BIT-VERIFIED live.** Read `0x54f980` case-`0x1872d` (`:911-970`): its two halves are
  separable (quirk #82) — **`:911-928` is the frame-stepper, UNCONDITIONAL** (gated only on
  clip `+0x6c`, byte-identical to `anim_clip_advance`, reads no RNG/clock → the horses ALWAYS
  trot), and **`:929-970` is the behaviour**, which `break`s out unless primary AND the global
  scene-lock `*(0x8a9b50+0x27a8)==0`, then draws the LCG for idle/wander (the deferred RNG
  layer #77). So the trot is portable in isolation. PORTED: `actor_render_state` gains the anim
  block `timer`(+0x70)/`done`(+0x74); **`actor_anim_advance`** (a thin adapter to the single
  ported stepper `anim_clip_advance`); **`actor_pool_update`** = the `0x46cd70:123-169` band
  walk (advance every active render-state with a clip — static actors no-op); `main.c
  game_actor_update` runs it on the SAME sim-tick gate as the camera easer (`hold & 1`), BEFORE
  `camera_follow_step` (retail `0x439690` order :1108→:1123), with a `CALL_TRACE_BEGIN(0x46cd70)`
  mirror. **LIVE (port blit trace, settled cam 12800, one 144-Flip cycle):** the wagon is **3
  keyed cels res `0x3ec`** (corrects ckpt-80's mis-noted `0x058f`) at x160/288/416; the body
  cel (x416) steps **5→2→3→4→5** every 36 Flips while the two fixed cels hold frames 0/1;
  `0x46cd70` mirror reports `advanced:1`/tick. **USER-confirmed** (horses idle subtly — ear
  flicks; the wagon is PARKED, not moving — "which is how it's supposed to be"; so `WAGON_CLIP`
  is an IDLE loop, not locomotion — quirk #82). **896 pass** (+3); ledger 199/194 unchanged
  (bare-VA slices). quirk #82; PORT-DEBT `actor-protagonist-clip` narrowed to the RNG behaviour
  + the cutscene roll-in. `findings/in-game-intro.md` "The horses TROT". **NEXT (corrected
  ckpt 82):** the code-adjacent "siblings" `0x1872e`/`0x1872f`/`0x18730` are **OUT-OF-SCENE**
  — `0x1872e`←`0x539e80` room 410240 (area 410), `0x1872f`←`0x5034b0` room 230110 (area 230),
  `0x18730` = child of non-town char `0x11350`; the town script `0x4d7d80` spawns ONLY the
  wagon. Code-adjacency ≠ same scene (quirk #83).
- **GROUND TRUTH (ckpt 82): the hold residual is the CHARACTER CAST + foreground TREE, NOT a
  "banner".** Re-captured the retail blit trace + a PNG at the scene-LOCKED establishing hold
  (flip 1500, cam 128000). The 108 keyed blits = **54 visible via present `0x48eac0`
  (`ret_va 0x48ecc2`)** — res `0x481` 320×320 = the foreground TREE + ~5-7 multi-part townsfolk
  CHARACTERS (banks `0x426`×5/`0x459`/`0x462`/`0x46a`/`0x46b`/`0x472`/`0x47b`) + props — and **54
  INVISIBLE via `0x4962a0`** parked at y=572 (off-screen, NO render-id; a HUD?). **No "Town of
  Tonkiness" banner blit exists at the hold** (zero `0x5a00c0`-range `ret_va`s → the docs' banner
  + the `0x5a00c0`-overlay producer are BOTH refuted, like the letterbox was). The cast is NOT
  the main map-object band (bank `0x16c`/`0x175`, off-screen-left at cam 128000) — it's the 8
  PARTY actors (`0x59f2c0`→`0x560e60`, `ret_va 0x59f578`) and/or a scene-actor band. Per quirk
  #82 the hold is scene-locked ⇒ deterministic. `findings/in-game-intro.md` "The hold residual
  is the CHARACTER CAST + foreground tree". **USER DIRECTIVE: get the intro scene 1:1 on EVERY
  frame, THEN pinpoint + port every RNG consumer in the scene 1:1.** That decomposes exactly:
  **Phase 1 (this residual)** = the character/multi-part static render (spawn + sprite tables +
  poses → the locked establishing frames go differ_px==0; generalise the `0x491ae0` arm past the
  wagon + the lazy `+0x48` fill, PORT-DEBT `actor-sprite-table`); **Phase 2** = every in-scene
  LCG consumer (`0x54f980:929+` wander, …), matched by rng+rngcalls both sides. **NEXT:** pin
  each cast cel to its actor (annotate the emit `0x492670`/the band-walk feeding `0x48eac0` with
  the actor code, recapture) → RE the cast spawn → port the multi-part render → verify. Also
  pending: byte-confirm the wagon via `render_diff` keyed on `(res 0x3ec, frame)`.
- **Prior (ckpt 80): the town intro `0x1872d` is PORTED + SPAWN-RE'd + WIRED + LIVE-VERIFIED —
  and it's the arrival WAGON, not "the protagonist" (corrects #79/#80).** Three parts: (1) **the
  render arm** — `actor_render_protagonist` ports `0x491ae0`'s case-`0x1872d` (a 3-cel composite;
  part 2 is byte-identical to `FUN_0044d160`/`actor_render_describe`, wrapped with two fixed
  bank-`0x175` cels @ x-256/x-128). (2) **the SPAWN, fully RE'd:** `0x1872d` is NOT a map object
  (code outside 70000) — it's spawned by the **town intro cutscene script `FUN_004d7d80`** (`case
  0x334be`=room 210110 / area `0xd2`, gated on event flags `0x5f76805`/`0x606aa4f`) →
  **`FUN_00431d10(0,0x1872d,anchor=0x65,x=0x3200,0,0)`** (the by-code `+0x11e0` spawn helper:
  free-slot scan + anchor-relative placement) → **`0x431e30` case-`0x1872d`** which installs sprite
  row 0 via **`FUN_00426db0(0,0x175,0,…)`** (the long-missing `+0x48` FILL PRIMITIVE, now RE'd —
  retires part of `actor-sprite-table`), clip `&DAT_00671c48`, layer 9, facing 99. (3) **WIRED +
  LIVE-VERIFIED:** `actor_spawn_protagonist` + the `game_actor_walk` dispatch → the port logs
  `8 nodes from 33 actors (bank 0x175 registered)`; a with-`0x1872d` vs no-`0x1872d` rebuild diff at
  the settled camera (cam 12800) **isolates exactly its pixels = a horse-drawn CARAVAN** (bbox
  x180-543), NOT a person → **`0x1872d` is the town intro arrival CARRIAGE** (**USER-CONFIRMED on the
  feed: "that definitely matches retail"**). The 3-cel composite = wagon-left | wagon-body |
  **HORSES**: the first render froze the body on frame 0 (redrew the wagon-left cel → "cut in half"),
  so decoded the clip **`&DAT_00671c48`** from the user's exe (`base_sprite 2, 4 frames, looping,
  delta {0,1,2,3}` → body cels 2..5 = the horses) and pointed the render-state at a reconstructed
  `WAGON_CLIP` → the body now draws sprite 2 = the horses. **893 pass** (+4); ledger 199/194 unchanged
  (bare-VA slices). quirk #81; PORT-DEBT `actor-protagonist-clip` (the horses are FROZEN on frame 2 —
  the per-tick stepper that trots them + the cutscene roll-in are deferred).
  `findings/in-game-intro.md` "The 0x1872d SPAWN + the arrival WAGON". **NEXT:** the per-tick anim
  (trot the horses) + the scripted roll-in; then the caravan's siblings `0x1872e`/`0x1872f` (likely
  the characters — spawned by `0x539e80`/`0x5034b0`); byte-confirm via `render_diff` (res `0x3ec` —
  ckpt 81 corrects the `0x058f` here from the live blit trace).
- **Prior (ckpt 79): the town CHARACTER band is RE'd, SPAWNED, RENDERED + WIRED — and it's
  mostly PROPS, not NPCs (USER-confirmed live).** Per the methodology ("capture each slot's
  `+0x48` live"), extended the `0x491ae0` field spec + captured every active `+0x11e0` actor at the
  town hold (flip 1480/1500/1520, `--seed-pin --lockstep`). **Census (corrects #78/#79):** of 33
  main-band actors, **27 are INVISIBLE** (all-zero `+0x48` → self-skip; collision/trigger/spawn
  volumes — the `0x111d6`/`0x112e6`/… physics-body codes), and **only 6 DRAW** — `0x1129e`×3 /
  `0x1129f` / `0x112e5` are **static PROPS** (bank `0x16c` = town-objects sheet res `0x403`: a
  barrel, the fountain — NOT people), + **`0x1872d` the animated protagonist** (bank `0x175`, the
  one PERSON; OUTSIDE the 70000 range = a SEPARATE spawn; needs the `0x491ae0` multi-part arm —
  **the bulk of the 36-blit residual**). Corrections landed: `0x426620` **ZEROES** `+0x48` (its
  `type*0x80+0x21c04` is the **collision-grid** lookup #79 misnamed); the sprite table fills
  **LAZILY** (`0x40afe0`/`0x41e600`, type-keyed def table, un-RE'd); and the prop offset from
  `map_x*100` is **DETERMINISTIC per-code (NOT RNG)** — the fountain `0x112e5` is `+0/+0` and
  matches retail exactly. **PORTED + WIRED + LIVE-VERIFIED:** `src/actor_spawn.{c,h}`
  (`actor_spawn_from_map`: 32 CHARACTER objects → `{actor,render-state}` at `(x,y)*100`, the 3
  prop rows from the captured stand-in, PORT-DEBT `actor-sprite-table`) + `town_render_step_ex`
  actor seam + `main.c` (`game_actor_walk` → `actor_render_static`, `game_cel_dims` cull,
  `game_present_blit` `PRESENT_KEYED` → `zdd_object_blt_keyed`). The port logs `5/32 actors
  emitted (bank 0x16c registered)` and the props render at the correct spots (USER-confirmed on
  the feed). **889 pass** (+6); ledger 199/194. quirk #80; `findings/in-game-intro.md` "The town
  actor RENDER CENSUS". **NEXT:** the `0x1872d` protagonist multi-part animated arm (the actual
  person + the bulk of the 36 residual) as its own arc; then `render_diff` vs retail flip 1500.
- **Prior (ckpt 78): the town actor SPAWN is RE'd + BYTE-VERIFIED — no live drive needed**
  (unblocks the ckpt-77 ported renderer; docs-only, 883 pass unchanged). **The chain**
  (corrects the ckpt-76 guess `0x42eb20`/"`0x587e00` layer pass"): `0x586010:698` →
  **`FUN_0058d460`** (room object-population pass) → **`FUN_00431e30`** (character activator).
  `0x58d460` walks the map's **86 object-placement layers** (`mapobj+0x38` headers, `+0x10`=type
  code, `+0x04/+0x08`=x/y) and dispatches each by **type RANGE** into four pre-alloc bands off
  `DAT_008a9b50` (EFFECT 50k→`+0x1160`, STRUCTURE 60k→`+0x2560`, **CHARACTER 70k→`+0x11e0` via
  `0x431e30`**, DEVICE 80k→`+0x13e0`; each a `"<kind> Object Count Over"`-guarded free-slot scan).
  `0x431e30` (thiscall, ECX=slot) per-type switch: `+0x1d0=1`/`+0x1d4=type`/`+0xfc=9`/`+0xe8=0`,
  zeroes `+0x48` sprite table, stores world (x,y); a helper (`0x426620`) installs the sprite from a
  def table (`type*0x80+0x21c04`). **The proof** (resolves "codes never assigned as constants"):
  the town codes ARE the map object type fields — `map_data.py --objects` on DATA 1022 → 15 effect
  + 39 structure + **32 character** + 0 device; the 32 char codes + multiplicities are IDENTICAL to
  the ckpt-76 live census (0x112e6 ×10, 0x111d6 ×7, …), with world positions. The 33rd live actor =
  the 1 animated NPC (`0x1872d`, separate path). `docs/proofs/map-object-layer-format.md`; quirk #79.
  **NEXT (the port):** the **code → `+0x48` sprite-table** mapping (the only datum not in the map —
  `0x431e30`'s def-table install; RE the 13 town codes OR capture each slot's `+0x48` live) → minimal
  spawn (32 objects from `map_data` → render-state pos + sprite + dir + layer 9) → wire into
  `game_render` → `render_diff` vs retail flip 1500 (the 36-blit residual drops) → human pixel-verify.
  `findings/in-game-intro.md` "The town actor SPAWN".
- **Prior (ckpt 77): the town ACTOR RENDER SIDE is PORTED + host-tested** (the default arm
  that draws 32/33 town actors), ahead of the spawn. Pure, no harness.
  **Ported (commit `0533603`):** `draw_pool_emit_actor` = `FUN_00492670` (the actor analog
  of `draw_pool_emit`; node mode = `bool(alpha!=0)`); **`actor_render.{c,h}` (NEW)** —
  `actor_render_describe` = `FUN_0044d160` (the static/animated/mirrored/angle sprite
  descriptor over the per-direction table `actor+0x48`) + `actor_render_static` = the
  `0x491ae0` **default arm** (`caseD_11257`: 32/33 town actors hit it); `map_present`
  **MODE 0** (the opaque-actor keyed path `FUN_005b9b70`, cull dims from the cel via a new
  `present_dims_fn`). actor + render-state are LOGICAL structs (the spawn fills them);
  `actor_sprite_row` (0x14) pinned. **Validated:** the render-state offsets match the ckpt-76
  live `0x491ae0` field spec exactly (`rs_x`/`rs_y`/`rs_clip`/`rs_frame` = +4/+8/+0x6c/+0x72);
  logic host-tested bit-exact. **883 pass** (+18); ledger 199/194. **NEXT (the gating arc —
  needs the harness, then the human for pixel-verify):** the **SPAWN** (the `+0x11e0` band
  activator — NOT `0x560e60`/`0x584710`; it's the entity subsystem `0x42eb20`/`0x4282f0`/…
  over the DATA 1022 layer entries) → the `0x1872d` animated arm (the 1 key NPC) → **wire**
  the band walk into `game_render` → `render_diff` vs retail flip 1500 (the 36-blit residual
  drops). PORT-DEBT `present-actor-modes` (narrowed: mode 0 done, wiring blocked on spawn) +
  `actor-occlusion`. `findings/in-game-intro.md` "The town ACTOR render side".
- **Prior (ckpt 76): the town NPC/actor RENDER PATH is RE'd LIVE + the trace tooling hardened.**
  User: "implement the NPCs / consult the runtime trace / harden + document the trace tooling."
  Did the RE + instrumentation half (render-side port follows). **Tooling:** added the reusable
  **`thischain`** field source (ECX-rooted pointer hops — probes any actor by its live `__thiscall`
  `this`) + **annotated** `0x491ae0` (actor render entry), `0x560e60`, `0x584710` in
  `retail_fields.json`. **RE (live, retail town hold flip 1500):** the MAIN actor band is
  `DAT_008a9b50+0x11e0` (0x80 slots, rendered by `0x491ae0`, updated by `0x54f980`; one of six
  bands `0x48c150`/`0x46cd70` walk). **33 active actors: 32 STATIC** (clip==0), **1 animated**
  (`0x1872d`). **32/33 behaviour codes fall through to `0x491ae0`'s DEFAULT arm → `FUN_0044d160`**
  (the static-actor descriptor) → `0x492670` emit into the draw_pool as **mode 0/1** (= the
  deferred PORT-DEBT `present-actor-modes`).  The code drives the AI (`0x54f980`), NOT the render —
  **one function draws the town**.  Render banks res `0x403`/`0x426`/… = the ckpt-75 36-divergence
  residual.  **Band is a PRE-ALLOCATED 128-slot pool** (`0x586010:487` `FUN_0058cf60(0x40)`×128);
  the per-room **spawn = ACTIVATE+configure**, running after `0x586010`'s `"Init Objects"` marker —
  a **data-driven entity-by-id** subsystem (codes never literal; NOT `0x560e60`=8 party / NOT
  `0x584710`).  **NEXT:** find the `+0x11e0` activator (hook post-"Init Objects"), then port the
  render side (`FUN_0044d160`+`0x492670`+present 0/1) + wire + pixel-verify vs retail flip 1500.
  865 pass (no C touched); engine-quirk #78; `findings/in-game-intro.md` "The town ACTORS".
- **Prior (ckpt 75): the establishing-shot cinematic LETTERBOX is PORTED + blit-trace 1:1.**
  RE'd the producer from the captured retail blit trace: it's NOT the `0x5a00c0` overlay but
  **`0x48c150:124-162`** (the per-frame world driver), two grid-fill loops that tile a 64×4
  opaque cel (res **`0x583`** = main-pool slot 41) across the screen — BOTTOM bar
  (`in_ECX+0x44` rows, ret `0x48c48a`, dy 416-476) then TOP bar (`in_ECX+0x48` rows, ret
  `0x48c4fe`, dy 0-60), both 64 → the quirk-#74 black bars (rows 0-63 + 416-479), 640×352
  cinematic window. **Ported `letterbox.{c,h}`** (the grid-fill, host-tested 4 + bit-exact
  vs the trace) + `main.c game_letterbox_blit` (resolves slot 41 frame 0, `zdd_object_blt_onto`,
  drawn after the present). **Re-diff: the town-frame divergences dropped 356→36** — all 320
  `0x583` blits now match retail on identity+geometry+state; the 36 left are exactly the
  deferred RNG-driven actor/banner/tree banks. **Port frame pixel-verified** (rows 0-63 +
  416-479 `(0,0,0)`, row 64 = sky), USER-CONFIRMED on the feed. 865 pass; parity-ledger #8;
  PORT-DEBT `ingame-letterbox` (the 64/64 heights stand in for the unported `0x5a00c0` op
  that writes `+0x44`/`+0x48` — the geometry is bit-exact). **Next chip: the "Town of
  Tonkiness" BANNER + foreground TREE (`0x5a00c0` overlay player).**
- **Where we are (ckpt 73): the actor-band residual is PINNED to the RNG pillar — and the
  shared LCG stream is NON-DETERMINISTIC run-to-run EVEN UNDER `--seed-pin`.** Ran the ckpt-72
  directed live check: drove retail **twice** (`--seed-pin --lockstep --no-turbo`, same
  in-game trace), snapshotting the LCG state `DAT_008a4f94` at the per-sim-tick actor-update
  boundary (`0x46cd70`, new `rng` field). **Result: `rng` matches 0/8643 in-game
  sim-ticks** — the stream is at a different phase every tick despite the pinned seed and the
  deterministic sim-tick index. **Smoking gun:** at `prologue_enter` BOTH runs are on the
  IDENTICAL flip 946 yet rng differs (`0x84654e6f` vs `0xa79a2d6e`) → at the same flip the
  engine has drawn a *different number* of LCG values. Mechanism: a consumer draws per-PRESENT
  and the presents-per-sim-tick count is non-deterministic (quirk #75), so the stream phase
  desyncs and never re-converges. Since `0x54f980`'s behaviour cases draw this exact LCG
  (`FUN_005bf505`, ~40 sites: idle waits `+0x5c`, the idle→wander branch pick, move offsets →
  `0x450ef0`), the actors pick different waits/dirs/positions run-to-run = the #75 ~6.7k-px
  band. **CONCLUSION:** an RNG-reading subsystem needs its OWN **RNG anchor** (snapshot/restore
  `DAT_008a4f94` at the game_enter sim-tick, both sides) — the camera's `g_sim_tick` anchor is
  insufficient for it (works only because the camera reads no RNG). Parity bar for the actor
  band = "data-1:1 given a matched RNG state" (retail-vs-retail isn't observed-1:1 here).
  (The `a0_clip/a0_frame` fields matched 8643/8643 but TRIVIALLY — main-band slot 0 was inert
  the whole run; the `rng` divergence is the real result.) Tool: `tools/rng_tick_diff.py`.
  Engine-quirk #77; `in-game-intro.md`. **DIRECTION (user): defer ALL RNG-order parity**
  until every in-scene RNG consumer is RE'd, then match consumption order (rng+`rngcalls`
  both sides — the flow trace now carries `rngcalls`, the unified consumption signal,
  openrecet-style; commit `4c587c0`). **Next chips = implement the scene's VISUAL elements**
  (LETTERBOX #74 → `0x5a00c0` banner/tree → NPC render/spawn); RNG behaviour parity comes after.
- **TOWN FRAME DIFFED via the new blit trace (ckpt 74) — the port's backdrop is PIXEL-FAITHFUL;
  the gaps are MISSING layers, pinpointed.** render_diff (hold: port 1200 ↔ retail 1500, both
  cam=128000) → 606 retail / 250 port blits, **356 divergences ALL `[sprite]` (missing), ZERO
  `[rect]`/`[decode]`/`[state]`, ZERO port-extra** — every port blit matched retail on identity
  + geometry + state. The missing draws: **320× bank `0x583`** (a 64×4 full-screen grid, frame 0,
  deterministic, `dy=416` = the letterbox row → the **establishing-shot cinematic overlay**,
  quirk #74 — **PORTED ckpt 75, see LATEST above; the 320 blits now match retail**) + ~36 actor/overlay banks
  (`0x426`/`0x403`/… NPCs + tree + banner — RNG-driven, accepted-divergent until the scene RNG is
  RE'd). `findings/ddraw-blit-trace.md` "The TOWN frame".
- **Prior (ckpt 72): the ACTOR ANIMATION cycle RE'd + the frame-stepper ported — rides the
  sim-tick clock, no separate counter.** The per-tick UPDATE pass (`0x439690:1108`→`0x46cd70`
  once/tick→`0x54f980` per actor) runs one byte-identical inline stepper on the render-state
  anim fields (`+0x6c` clip/`+0x70` timer/`+0x72` frame/`+0x74` done): `timer++`; at `>=clip.dur`
  → `frame++`,`timer=0`; at `>=clip.count` → loop or one-shot hold. Clip = a fixed **0x154-B
  32-frame** descriptor, (re)set on STATE CHANGE (`0x40afe0`/`0x41e600`). **PORTED (host-tested
  bit-exact, 854 pass): `src/anim_clip.{c,h}`.** The stepper reads no GetTickCount/Flip/RNG → it
  is deterministic under the camera's `g_sim_tick` anchor; ckpt 73 then proved the leftover band
  diff is the RNG-driven BEHAVIOUR, not the stepper. Engine-quirk #76.
- **Prior (ckpt 71): TIMESTEP DETERMINISM established — the SIM-TICK is the only
  valid frame-of-reference; the "house off by 3px" was FLIP-misalignment, not a bug.**
  The in-game sim is a wall-clock GetTickCount frame-limiter (`FUN_00439690:776-859`): one
  logical sim tick per outer iteration (easer `0x43d1d0` once at `:1123`) but a VARIABLE
  number of Flips per tick → **the Flip index is non-deterministic run-to-run** (two identical
  retail runs disagree on 47-86% of flips by ≤3px; `--lockstep-epsilon-ms 0` is worse, so it's
  intrinsic, not the epsilon). The **sim-tick index** (easer-call count) is bit-identical.
  The user's whole-foreground 3px trail (background Δ0) is the signature of flip-misalignment
  — the 0.5×/0.25× parallax hides the same camera offset the 1× foreground exposes; the tile
  math is provably identical at equal `cam_x60`. **FIX (committed):** the agent counts easer
  calls (`g_sim_tick`), tags every captured frame (`frame_<flip>_t<simtick>.png` + manifest)
  + call-trace event, and RESETS the counter at the `game_enter` scene-load anchor (synchronize
  at every non-deterministic load) → cross-run deterministic (99 ticks, 0 cam-mismatches; pan
  starts at tick 92 both runs). `tools/sim_tick_diff.py` matches two run-dirs by sim_tick/cam
  (dx=0) vs flip (the 3px trail). Engine-quirk #75; `in-game-intro.md` "Timestep determinism".
  **DECISION (user):** anchor each subsystem for determinism rather than a global timestep
  hack (fallback if it gets unwieldy). The camera is synced (sim-tick); the actor anim cycle is
  now RE'd + ported (ckpt 72 above — it rides the same sim-tick clock, no new pin needed). The
  intra-tick-identical observation is now explained: the stepper reads no flip/clock/RNG.
  Standing rule: never diff on the Flip index — anchor on the sim tick. NB `--turbo` is NOT faster in-game (Frida/LAN overhead dominates, not Sleep) and
  breaks the no-turbo-timed input traces; use `--no-turbo --lockstep` until traces are re-timed.
- **Prior (ckpt 70): the intro-PAN camera is WIRED LIVE — the town now pans.**
  `main.c game_render` steps a live `camera_view` each frame (`camera_follow_step` =
  `FUN_0043d1d0`, with the `CALL_TRACE_BEGIN(0x43d1d0)` flow-trace mirror) and projects the
  backdrop through its *current* scroll instead of the static const. `enter_game`
  spawn-snaps it (`camera_apply_snap` → cur=tgt=128000/12800); a hold timer fires the
  scripted pan (`camera_apply_pan` → tgt=12800/12800, speed 300) at hold-end. The two
  target-setters are bit-exact ports of `0x439690`'s SNAP/PAN command arms (`:599-664`),
  host-tested (clamp to `[0, map-vp]`; snap-jumps-cur / pan-keeps-cur). **Visually confirmed
  on the feed:** hold (cam x=128000) → mid-pan → settled (cam x=12800, town left edge).
  **848 pass / 0 fail / 6 skip** (+2). Also added `MAP_RENDER_CAM_TOWN_3F2_SETTLED` (x=y=12800).
- **CADENCE + TRIGGER measured → the pan is now TRAJECTORY-1:1 (ckpt 70b).** A retail
  field-spec trace (`--seed-pin --lockstep --no-turbo`, easer `0x43d1d0` + Flip hooked,
  contiguous Flip whitelist) pinned both stand-ins to ground truth: the easer fires **once
  per 2 Flips** (the sim runs at half the Flip rate; `cam_x60` is a STEP function, −300/2flips
  at cruise) and the pan command fires at **`game_enter + 184` Flips** (Flip 1616 HOLD, 1617
  PAN). `game_camera_step` now gates the sim to every 2nd frame (`hold & 1`), trigger at
  `GAME_CAMERA_HOLD_FRAMES=184`. **The port now passes through the IDENTICAL `cam_x60`
  sequence as retail** (128000,127990,127970,127940,…,cruise −300/2flips — verified by
  diffing the captured `0x43d1d0` mirror). **RESIDUAL (PORT-DEBT `ingame-camera-pan`):** a
  ~2-3 Flip startup-jitter PHASE offset (retail's sim accumulator is wall-clock-paced — a
  4-Flip plateau at 1618-1621 a clean 2:1 step can't reproduce; ≤1 step ≈ 3px, transient,
  zero at hold+settled) + the cutscene-script TRIGGER source — both downstream of the
  in-game sim / `0x5a00c0` port.
- **Methodology (reinforced ckpt 69):** "annotate" = the **flow-trace field spec**
  (`retail_fields.json` named functions+fields + port `CALL_TRACE_BEGIN` mirrors) — a CORE
  step of finishing any RE/port; thiscall/struct tagging is a SEPARATE static-readability
  lane. Never an ad-hoc symbol-rename. (CLAUDE.md "Annotate as you RE".)
- **Prior (ckpt 68): 24bpp parallax LUT grade LANDED — sky colour USER-CONFIRMED.**
  Found retail grades the 24bpp sky/mountain banks (`0x55`/`0x58`/`0x59`) at **DECODE**, not via
  the palette path (`0x417c40` early-exits to the plain getter when a bank has no palette): its
  **flag-3 branch** (the 24bpp case) stamps the slot's brightness descriptor (`f_08=1`, scales
  1000 = tint case 0, `f_18`=tone-curve LUT) before the getter, and the lazy `ar_sprite_decode`
  runs `ar_sheet_decode_pixels` (already ported, quirk #46). The port's parallax sink used the
  plain getter so never stamped it → sky decoded raw/too-bright. **Fix:** `game_arm_parallax_grade`
  in `main.c` replicates the stamp in `game_parallax_blit`. Verified: raw sky `(66,150,255)`→LUT
  →565 = `(33,125,239)`; **blue `239` matches retail's main sky band exactly**, and the user
  confirmed the grade looks correct on the feed. (The old finding's raw `(132,186,255)`/retail
  `(103,165,231)` numbers were wrong — actual raw is `(66,150,255)`.) **OPEN (deferred):** a
  "dark top gradient" the user sees in the establishing-scene frame (but not in settled gameplay)
  — likely a **per-scene CINEMATIC effect tied to the establishing shot**, to be confirmed by
  probing ground truth alongside the intro PAN RE.
- **Prior (ckpt 67):** backdrop TILES `differ_px==0` via the 8bpp palette grade (`color_grade.{c,h}`);
  the "establishing shot" proven a leftward **PAN not a zoom** (only `+0x60` moves; dx=0, same
  scale; `MAP_RENDER_CAM_TOWN_3F2`). **840 pass / 0 fail / 6 skip.** Ledger **194/1490 touched /
  189 tested**. Both GUI builds clean.
- **NOT a full `differ_px==0` frame yet — named residuals** (NOT logic bugs): the **NPC actors**
  (present modes 0/1/2, blocked on the entity/spawn system — PORT-DEBT `present-actor-modes`); the
  **foreground tree** + **"Town of Tonkiness" banner** (`0x5a00c0`, PORT-DEBT `ingame-nontile-layers`);
  the intro **pan** is wired + cadence/trigger-matched (ckpt 70b) — it passes through retail's
  exact `cam_x60` sequence; residual is a ~2-3 Flip startup-jitter PHASE (PORT-DEBT
  `ingame-camera-pan`), zero at the hold + settled ends (`MAP_RENDER_CAM_TOWN_3F2_SETTLED`, x=y=12800).
- **PAN BACKDROP DIFF DONE — verified pixel-1:1 (ckpt 70b).** Captured fresh retail pan frames
  (`--no-turbo --seed-pin --lockstep`) + their `cam_x60`, matched port frames by `cam_x60` (port
  Flips 1304/1344/1384/1422/1462 ↔ retail 1617/1660/1700/1740/1780, shared cam 127990/125690/
  120050/114350/108350), and diffed: the **backdrop is Δ0** (shift-search peaks sharply at
  `dx=dy=0`; pan-start `x=80` column all Δ0). The remaining diff is the **named missing layers
  ONLY** — exactly the signal we wanted. NEW retail ground-truth (quirk #74): the establishing
  shot is **LETTERBOXED** (solid-black bars rows 0-63 + 416-479, a 640×352 cinematic window; the
  "dark top" the user saw, with a matching bottom bar). Parity-ledger entry #7.
- **Next move (the named layers, simplest first):** (a) the **cinematic LETTERBOX** (quirk #74)
  — **DONE ckpt 75** (`letterbox.{c,h}`, the `0x48c150:124-162` grid-fill; 356→36 diff); (b) the **"Town of Tonkiness" banner** + **foreground tree/veg**
  (`0x5a00c0` scripted-scene overlay player — also where the pan TRIGGER source lives, closing
  `ingame-nontile-layers` + the trigger half of `ingame-camera-pan`); (c) the **actor renderers**
  (present modes 0/1/2, need the entity/spawn system first). Writeups: `findings/in-game-intro.md`
  "The pan CADENCE + TRIGGER measured" + the diff verification; quirk #74.
- **Tooling front (ckpt 74): the DDraw BLIT TRACE landed + cross-side-verified — we now have
  the two-drill-in coverage the user asked for.** `render_diff` names the wrong DRAW (and how:
  `[sprite]`/`[decode]`/`[rect]`/`[state]`); `flow_diff` names the wrong LOGIC. B3 (`docs/plans/
  trace-tooling-phase-b.md`, `findings/ddraw-blit-trace.md`): `src/render_id.{c,h}` is the
  cross-side identity — a cel→`(resource_id, frame)` registry (openrecet's `tex_name` trick:
  drop the alloc-dependent pointer, key on the load-stable asset name) **plus `dhash`**, an
  FNV-1a fingerprint of the DECODED sheet pixels (the improvement over openrecet's name-only
  scheme — a software blitter has the pixels in CPU mem at decode, so it catches RIGHT sprite /
  WRONG decode, the palette/24bpp residual class). Port emits at the 5 blit primitives
  (`zdd_emit_blit`); the Frida agent mirrors it (resolver `0x418470` registry + the new
  `renderid`/`thisderef` field sources, each auto-installing — no ad-hoc flag). **LIVE-VERIFIED:**
  retail emits the IDENTICAL `resource_id` (0x91b) as the port for the title background; ECX/arg
  reads correct; render_diff named all 59 title-phase blits by identity. Next layers: retail-side
  decode-hash (so `[decode]` fires cross-side), the cdecl `0x5bd550` retail spec, a same-scene
  aligned in-game diff. AGENT-WORKFLOW.md trimmed to subagent-only (process audit). 813 pass.
- **Prior tooling: Phase-B B2 (the field-bearing flow trace) LANDED + live-verified**
  (`docs/plans/trace-tooling-phase-b.md`): `call_trace` carries `seq` + `CALL_TRACE_BEGIN/FIELD/END`;
  the Frida agent reads same-named retail fields per `tools/flow/retail_fields.json` (now incl.
  the `cam_*` camera chain + the ckpt-67 `tint`/`lutgate1/2`/`lut*` colour-grade probes +
  the ckpt-69 `camera_follow_step` producer entry);
  `tools/flow_diff.py` names the first `[chain]`/`[data]` divergence. Remaining Phase B: **B1**
  unified `scenario-test.py`, **B3** DDraw blit-command trace + `render_diff.py`.
  **`mem_watch.py` (ckpt 69):** now resolves **chain heap addresses**
  (`--watch-chain ROOTVA:HOPS:OFF:SIZE[:LABEL[:ARM_AT_FLIP]]`) + a **`--hw` hardware
  watchpoint** (frida-17 per-thread DR) — the fitting tool for a hot heap field (found the
  camera easer through its heap fn-pointer dispatch in one run).
- **Standing bar:** every divergence is `differ_px == 0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic
  (`parity-model.md`); seed-pinned both sides, compared by anchor/tick.
<!-- FRONT:END -->
