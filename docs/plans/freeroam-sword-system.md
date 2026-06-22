# Plan — the freeroam SWORD / ATTACK system (Z unsheathe + X attack)

> **Status: CHIP 1 DONE (ckpt 156) — the DRAW + sword-out STATE are PORTED + verified bit-exact.**
> Z (ring 9) toggles `sword_out` (`character_resolve_sword`); `arche_sword_clip` plays the UNSHEATHE
> cels 96-103 (~48t) on the 0→1 edge / a reverse SHEATHE stand-in on 1→0, then delegates to the
> walk/idle/pose (sword-out reuses base cels).  VERIFIED off `port-sword.osr` vs `sword-realplay.osr`
> (`draw_probe --res 0x570`): the draw cels' dst W×H byte-identical, durations exact mid-clip (97-102),
> ±2t at the boundary frames (non-lockstep noise).  `PORT-DEBT(sword-quest-gate)` (Z free in the
> errands freeroam) + `(sword-sheathe-cels)` (reverse-draw stand-in).  Host-tested
> (`test_arche_sword_clip` + `test_character_resolve_sword`).  **NEXT = chip 2: the ATTACK** (X,
> neutral 120-127 + 128-132 + 3-combo + directionals) + sword-out WALK + the real SHEATHE — needs the
> CLEAN injected capture (force `weapon+0xd4=2`); see "## ckpt-155 — the attack VARIANT set" below.
>
> **ckpt 158 — BREAKTHROUGH (USER, the game owner): everything proxy-derived about the sword was
> WRONG, three ways.  RE-DO chip 1 against the clean recording.**
> 1. **NO QUEST GATE.**  Z draws the sword on a NORMAL retail run, on a FRESH NEW GAME, *before* any
>    quest progress (USER).  The ckpt-155 "gated on errands quest case 8 / weapon+0xd4=2" was an
>    ARTIFACT — the proxy was silently breaking the Z key in every injected capture.  `PORT-DEBT(sword
>    -quest-gate)` is BOGUS; remove it.
> 2. **OSS_FORCE_SWORD broke Z.**  Clamping `weapon+0xd4=2` every frame (`eh_force_sword`, built on the
>    bogus gate) JAMMED the natural draw — *only* Z failed, every other input worked.  Recording with
>    `OSS_FORCE_SWORD=0` + `OSS_TURBO=0 OSS_LOCKSTEP=0` (no clock virtualization) → Z works.  REMOVE the
>    force hook + its run_proxy plumbing.
> 3. **The sword-OUT form is a SEPARATE BANK: 0xc35b → bank 0x8c = res 0x571 (the "registered but
>    unused" bank!).**  Drawing RE-INSTALLS Arche from bank 0x8b (res 0x570 sword-in) to **bank 0x8c
>    (res 0x571 sword-out)** — res 0x570 VANISHES from the draw stream the instant Z fires (~tick 1807),
>    she reappears on res 0x571.  So the ENTIRE ckpt-156/157 chip-1 model (draw/sheathe/idle on res
>    0x570) was on the WRONG sheet.  The res-0x570 cels 96-103 are the sword-IN sheet's sheathe; the REAL
>    draw is **res 0x571 cels 96→103** (~7t each, ~56t), and the sword-OUT idle/walk are **res 0x571 cels
>    0-2 / 8-15** (blade baked in).
> - **CLEAN GROUND TRUTH (USER real-play, the input recorder WORKS): `/mnt/c/oss-osr/sword2.osr` (978 MB)
>   + `sword2-input.jsonl` (the editable held-trace, scancodes).**  flip≈2.23×tick.  Z-draw @flip 5400
>   = tick 1807.  **The moveset the USER demonstrated (from the input trace):** unholster (Z), walk R/L,
>   RUN R/L (dir double-tap), SLIDE R/L (DN+dir), run-stop with UP (×2), single attack (X), attack-SPAM
>   (repeated single, NO combo), then DIRECTIONAL attacks: facing-dir+X = a HEAVIER attack, immediately
>   opposite-dir+X = a DIFFERENT attack, UP+X = another; DOWN+X = nothing special.  All on res 0x571.
> - **NEXT SESSION (fresh /clear): extract the res-0x571 cels per action** (probe `sword2.osr --res 0x571`
>   at the input-trace ticks: run 16-21?, slide, up-stop, the attack cels + the 3 directional variants),
>   then **re-do the port**: render Arche from bank 0x8c (res 0x571) while `sword_out`; DRAW = res 0x571
>   96-103; idle/walk/run = res 0x571 0-2/8-15/16-21; attacks = res 0x571 120+ (+ directionals).  Remove
>   `eh_force_sword` + `PORT-DEBT(sword-quest-gate)` + `(sword-draw-cels)`/`(sword-out-pose-cels)` (now
>   capturable).  The chip-1 `arche_sword_clip` swap (ckpt 157, 1353612) is moot — rebuild on res 0x571.
>   Tools added this arc (keep): `OSS_INPUT_RECORD` (proxy input recorder), `/tmp/celspan2.py`,
>   `/tmp/flip2tick.py`.
>
> **ckpt 157 — chip-2 capture progress + USER feedback:**
> - **force-sword TOOLING DONE + committed** (`0f07877`): `tools/capture_proxy/engine_hooks.h`
>   `eh_force_sword` writes `weapon+0xd4=2` on Arche each flip (chain `*0x8a9b50→+0x2784→+0x200c→
>   +0x9f4 ent→+0x750 wpn`, off 0xd4; VirtualQuery-guarded; env `OSS_FORCE_SWORD=1`).  VALIDATED:
>   the hook fires (`[force-sword] wrote weapon+0xd4=2 @flip 1118 code=c35a`) + the game enters
>   (sim_tick advances).
> - **BLOCKER (chip-2 inject capture):** a from-scratch injected nav reaches the errands but Arche
>   stays in the LEFT-IDLE (cels 152-155) and never responds to injected Z/X/walk — the
>   **cutscene→freeroam HANDOFF doesn't complete** (control never hands to the freeroam char-AI;
>   ~92% frames carry text).  4 attempts (each fixed a different nav bug: inline-comment pollution,
>   turbo, dropped boot id-3, extra confirms) — the handoff is the real retail-harness fiddliness.
> - **USER GUIDANCE (ckpt 157):** "you can just APPEND INPUTS TO THE RECORDED TRACE.  at sim_tick
>   2413 the dad door dialogue is fully dismissed and the sword still drawn."  → the path is to
>   append attack inputs to the recording's INPUT TRACE (it already reached controllable sword-out
>   freeroam), NOT fight the from-scratch handoff.  **OPEN: no recorded input-trace FILE found**
>   (`sword-realplay.osr` is the draw stream only; the proxy doesn't log real-play input) — need the
>   USER's input trace OR a targeted re-record.
> - **USER OBSERVATION:** the port "draws the sword … but snaps back to no sword after the animation."
>   RESOLVED as the SHEATHE in the `port-sword.osr` DEMO (the demo's Z@frame5900), NOT a render bug:
>   the port's sword-out idle cels (res 1392 fr 0/1/2) are **dhash + blit IDENTICAL** to the recording
>   (and the retail force-capture) — same sprite, so the port renders the sword-out idle exactly as
>   retail.  A no-sheathe demo (`port-sword2.osr`: draw → STAY drawn → walk) is the clean verify.
>   (CAVEAT: the port idle uses cels 0,1,2 = the SAME as the unarmed idle; if the USER confirms the
>   sword is NOT visible in the no-sheathe demo, the sword-out body is a distinct form-0xc35b bank the
>   port doesn't load — RE 0x41f200's 0xc35b install → the sword-out body res — and the dhash match is
>   a same-sheet coincidence to re-examine.)
>
> **Mechanism RESOLVED (ckpt 155) — the sword IS available in the errands, GATED on the
> tutorial quest reaching case 8 (`4dc510:1167` sets `weapon+0xd4=2`).**  USER (game owner) was RIGHT.
> My fresh-new-game captures sat at an early quest state so it was never enabled (ring injection +
> movement both PROVEN working via the id-7 jump control + the walk).  NEXT: reach quest state 8
> (force it / play the tutorial) → Z draws.  See "## ckpt-155 capture 6" below.  Old "BLOCKED/OFF"
> reads SUPERSEDED.  ORIGINAL USER (ckpt 154):
> ckpt-155 GROUND-TRUTH capture DISPROVES this for the first errands: **`weapon+0x466` (the
> sword-capability gate) reads 0 the whole scene**, and confirmed-firing ring injects of
> **Z (id 0x24) and X (id 9) change NOTHING** (cmd4 stays 0, body+0x66 stays 0) — only the
> held-axis walk responds.  So in the moving-in errands the sword is gated OFF and no input
> can enable it (478ba0:337 forces the stance block off when `weapon+0x466`==0; `+0x466` has
> NO writer in the decompile = set at weapon equip / story progression, not by a key).
> **Awaiting USER: which scene has Arche's sword ACTIVE?** (likely a later one — first dungeon /
> after she draws it.)  Until then the sword arc can't be captured.  See "## ckpt-155 capture" below.
> Orient: `CLAUDE.md` → `FRONT.md` → here.  Sibling: `findings/freeroam-pose-commands.md`
> (the U/D-pose + slide), `plans/controllable-arche-faithful.md` (the freeroam base).

## ckpt-155 capture — no reachable SWORD-DRAW path in the moving-in errands
Two captures (host up, `runs/sword/`).  **Capture A** (`sword_fields.json` + `sword-nav.jsonl`,
frame-based on the proven `slide-nav2` retail prefix → errands freeroam): injected ids
`[0x24, 9]` at flips 4567-4867.  **Capture B** (`kb_config_fields.json`): dumped the LIVE keybind
config `*0x8a6e80` to resolve which ring id each key produces.

**THE KEY→ID MAPPING (corrected off the live config dump — the opposite of the first guess):**
| key | DIK | cfg slot | ring id | role (478ba0) |
|-----|-----|----------|---------|---------------|
| **Z** | 0x2c | cfg+0x590 | **9**    | the discrete ATTACK press → cmd4=0xf (478ba0:302, gated `body+0x66`!=0 = sword OUT) |
| **X** | 0x2d | cfg+0x558 | **8**    | the HELD auto-attack (+0x128 → 478ba0:296 → 479f90 → cmd4=0xe) |
| **C** | 0x2e | cfg+0x574 | **7**    | JUMP (478ba0:287) |
| V/Enter | 0x2f/0x1c | cfg+0x5ac/fixed | **0x24** | confirm / dialogue-advance (a UI subsystem, NOT a freeroam char-AI consumer) |
(directions fixed in 46a880: 0xc8→1 up / 0xd0→3 down / 0xcb→2 left / 0xcd→4 right.)

**`weapon+0x466` = "sword currently DRAWN" (toggleable STATE, not a static capability).**
`402a60` (the action-precondition filter) tests it per-action: case `0xf00001` needs `+0x466`==0
(sword-IN actions, e.g. interact), case `0xf00002`/`0xf00003` needs `+0x466`!=0 (sword-OUT actions,
e.g. attack).  So `+0x466`!=0 ⇔ sword out.  It has **NO writer in the decompile** (set via the weapon
template / an aliased path, not a literal `+0x466 =`).

**RESULT — injecting Z (id 9) does NOT draw the sword here:**
- `code` (entity+0x1d4) stays **0xc35a** the whole freeroam (478ba0:463 implies 0xc35b/0xc35c are the
  OTHER Arche forms — likely sword-out; it never changes).  `wpn_466`=0, `body_66_68`=0, `cmd4`=0x0
  through all 3 id-9 presses (flips 4627/4747/4867 — confirmed-firing).  Only the held-RIGHT walks her.
- **EVERY id-9 consumer is in 478ba0, ALL gated on sword-already-OUT** (`body+0x66`!=0) or in the
  mode==1 stance block (gated off for Arche: 478ba0:337 forces mode 2 when `weapon+0x466`==0).  So
  there is **no reachable code path that DRAWS the sword** (flips 0xc35a→sword-out) in this freeroam.
- `body+0x66` is a CHARGE/windup METER, not sword-out (455d80 case-3 accumulates `+= weapon+0x2d8`
  toward cap 1000; 442a70:2206 drains -100/tick in body state 5).  The plan's `+0x66=sword-out` was WRONG.

**Capture B2 (the airtight cross-check — real X=id8 + Z=id9):** held X (scancode 0x2d → id-8
auto-attack) over 3 windows + Z(id9) presses.  **X-held DID fire `cmd4=0xe`** every window (478ba0:296
auto-attack reached — input IS processed) **yet `code` stayed 0xc35a and `wpn_466` stayed 0 throughout**
(distinct code={0xc35a}, wpn_466={0x0}).  So BOTH attack inputs are consumed but produce NO sword draw
/ NO sword-out form — conclusive that the sword subsystem has nothing to act on here.

**INTERPRETATION (needs USER in-game verification):** the binary shows the sword un-drawable in the
moving-in errands — most consistent with Arche NOT having her sword equipped yet this early (the draw
becomes reachable once `weapon+0x466` can flip, i.e. a later scene / after she equips it).  The USER
(game owner) maintains the sword works "here"; if so, the exact in-game step that draws it (a specific
key/precondition) would point to the missing path — OR it's a later scene.  **Re-capture once that's
known** (inject id 9 / id 8 in the scene where Arche has the sword; watch `code` flip 0xc35a→0xc35b).

## The USER ground-truth (engine-quirks #3311, the game owner)
- **Z = unsheathe / sheathe the sword** (a TOGGLE; a ring action).  **Two distinct
  POSE/anim SETS for everything by sword-out vs sword-in** — i.e. a SECOND full cel set
  (walk/idle/crouch/jump…) for the sword-out stance.
- **X = ATTACK when the sword is OUT, INTERACT when not** (e.g. talk to mom).  Different
  attacks by holding FORWARD / UP / DOWN while attacking.

## The command map — RE'd off `0x478ba0` (the char-AI builder; ckpt 154 initial pass)
The builder snapshots `entity+0x14854` cmd block + re-derives each cmd per tick:
- `cmd[0]` `+0x14854` — walk/dash (1/5 L, 2/6 R) — PORTED.
- `cmd[2]` `+0x1485c` — jump (7/8/9) — PORTED.
- `cmd[3]` `+0x14860` — U/D pose (10 down / 0xb up) — PORTED (ckpt 153/154).
- **`cmd[4]` `+0x14864` — ATTACK (0xe / 0xf)** — THIS arc.  Built from:
  - `:296-300` AI/auto attack (`+0x158a4`+0x128 set, 200ms gate) → `0x479f90` + cmd[4]=0xe.
  - **`:301-303` the X-button attack:** `(param_1+0x66 != 0)` [SWORD OUT] AND
    `(param_1+0x68 == 0)` [not mid-swing] AND ring **id 9** (X) in 400ms → cmd[4]=**0xf**.
  - `:459-460` `(param_1+0x66 != 0) || cmd[4]==0xe` → cmd[4]=0xf (normalize).

## The key STATES (to confirm by capture + RE)
- **`param_1+0x66` (s16) = the SWORD-OUT flag** (≠0 ⇒ X attacks; gates the attack arm).
  `param_1` = the body/anim struct (the `0x442a70` `puVar4`?; +0x48/+0x58/+0x66/+0x68 are
  anim/state fields).  CONFIRM the offset + that Z toggles it.
- **`entity+0x750` = the WEAPON/COMBAT struct** (`+0x140` mode, `+0x138[]` attack-slot
  array, `+0x144`, `+0x466`).  `FUN_00479d30(+0x750)`=get stance / `FUN_00479da0(+0x750,n)`
  =set stance (0-4); `FUN_0047a420(dir,flag)` cycles `+0x138[]` attack slots (modular %10);
  `FUN_0047a300`.  This is the combat/weapon state machine.
- **`entity+0x14898` = the stance-arm mode** (`:336`; ==1 enables the 0x18-0x1c stance
  selects; code 0xc35a [Arche] + `+0x750`+0x466==0 forces it to 2 at :337-339).
- **`entity+0x14894` (s16)** — an attack/combo gate (`:435`, `:472`).
- The control scheme `*(*0x8a6e80+0x104)` (0/1/2) branches the bind set (`:322/399/421`).

## The stance / unsheathe arms (`:341-374`, ids 0x18-0x1c)
Five ring binds (ids 0x18,0x19,0x1a,0x1b,0x1c, 200ms) each: if pressed AND current
stance (`0x479d30`) != target AND `0x4071f0(+0x750, +0x750[0x140], n)` ok → set stance n
(`0x479da0`).  Plus `0x47a7f0(0x1d..0x21, …, 5..9)` (`:376-394`) and the 0xa/0xc/0xd +
0xe/0xf arms (`:397-455`) via `0x47a300`/`0x47a420`.  **The Z unsheathe/sheathe toggle is
in here** (likely `0x47a420`/`0x47a300` flipping `+0x66` / the `+0x750` mode) — pin the
exact id + fn by capture.

## OPEN QUESTIONS (resolve capture-first, like the slide)
1. **Which ring id does Z post** for unsheathe/sheathe? (X=id 9 confirmed; Z=? — depends
   on the `*0x8a6e80` keybind + scheme `+0x104`.)  Ground-truth: inject Z's scancode ring
   in the errands freeroam, read which `+0x14864`/`+0x66`/`+0x750` field changes.
2. **What IS `+0x66` (sword-out)** + which fn toggles it — confirm `0x47a420`/`0x47a300`.
3. **The SECOND pose set (sword-out cels):** the bank/frame range for sword-out
   walk/idle/crouch/jump (the USER's "two distinct anims").  Off `retail.osr` res 0x570
   (draw_probe) with the sword drawn — like the ckpt-153b pose cels.
4. **The ATTACK cels + the apply** (`0x442a70`'s cmd[4]=0xe/0xf handling = a new body
   state): the swing anim + hitbox (the hitbox is gameplay, defer; the ANIM is the port).
5. **Directional attacks** (forward/up/down + X) — the variant selection.
6. **X=INTERACT when sword in** (talk to mom) — the non-combat X path (proximity + NPC
   dialogue trigger).  May overlap the door-enter / interact subsystem.

## The PLAN (capture-first, the slide/pose pattern)
1. **Capture the GROUND TRUTH** in the errands freeroam (the proven `ids:[4,4]`+held +
   the field-spec / proxy `.osr`): press Z (find its ring id by scanning), read `+0x66` /
   `+0x750`+0x140 / `+0x14898` per tick → the sword-out toggle.  Then press X (id 9) with
   sword out → cmd[4]=0xf, the apply body state, the swing cels (res 0x570 draw stream).
   Capture sword-out WALK/IDLE/CROUCH cels (the second pose set).
   - Field spec: extend `pose_consts_fields.json` (or a new `sword_fields.json`) with
     `+0x66`, `+0x750`→`+0x140`/`+0x466`, `+0x14894`/`+0x14898`, `cmd4`=+0x14864.
   - Nav: the proven dash-ring2 prefix → freeroam, then Z (scan ids) + X.
2. **Port the STANCE TOGGLE** (`character_resolve_attack`/`_sword` sibling to
   `character_resolve_pose`): Z ring → toggle a `sword_out` field; X ring → cmd[4] when out.
3. **Port the SWORD-OUT POSE SET** (`arche_*_clip` variants keyed on `sword_out`) +
   the ATTACK anim (a new `arche_attack_clip`, the cmd[4] body state in `character_step`).
4. **Verify bit-exact** off a port `.osr` vs retail (the swing cels + the sword-out
   walk/idle, tick-for-tick) — `draw_probe --res 0x570`.
5. **Document** (finding + quirk + ledger) + retire the relevant moveset debt.

## Risks / notes
- BIG arc — likely 2-3 chips (toggle+pose, attack anim, directional + interact).  The
  HITBOX/damage is gameplay (defer / PORT-DEBT); the ANIM + state are the faithful port.
- `0x442a70` is the 12KB shared integrator — RE the STRUCTURE but PIN values by capture
  (the ckpt-117/153 lesson; the cmd[4] body state will reuse stack slots).
- X=interact (sword-in) may need the NPC-proximity + dialogue-trigger subsystem (overlaps
  `char-up-door-probe` / the questline) — scope separately if so.

## ckpt-155 capture C — waited past the unlock (USER hints), STILL no draw
USER hints: (1) input eaten right after the dialogue — wait longer; (2) the draw anim takes ~1s,
give it time before attacking.  Capture C (`sword3-nav.jsonl`): fully dismissed the dialogue,
settled long, then 7 CLEAN single Z(id9) presses frames 5000-5900 (= **sim-ticks 1772-2216**, i.e.
5 of them PAST the ~1897 control-unlock) with ~7s gaps + X(id8)-held attacks after each.  RESULT:
`code` stayed 0xc35a (1801/1801 rows), `wpn_466`=0 throughout, **`body+0x38` only ever walk/idle
(0x0/0x1/0x10001) — NO draw-animation state for any of the 7 Z presses.**  So even with correct key +
post-unlock timing + draw-anim time, the sword never draws.
- CAVEAT: id-9's only field-visible effect (`cmd4=0xf`, 478ba0:302) is itself gated on sword-OUT, so
  the field capture can't directly CONFIRM the injected Z was consumed.  The definitive test is the
  `.osr` DRAW STREAM (does Arche's sprite change?) — the proxy capture, USER-inspectable in the studio.
- Forms 0xc35a/b/c = Arche INSTALL variants (41f200): 0xc35a base, **0xc35b = sword-OUT (registers the
  attack actions 0x27d9/0x27da + sword-out anim)**, 0xc35c another.  Unsheathe = re-install as 0xc35b.
- LEADING HYPOTHESIS: the Frida capture boots a FRESH NEW GAME (anchors newgame_enter 750 → game_enter
  1427) and reaches the moving-in errands as the very FIRST freeroam — most consistent with Arche NOT
  having her sword equipped/enabled that early in a clean run.  NEXT: USER to confirm whether a
  brand-new game has the sword at the first errands or only after a beat; OR an `.osr` visual capture.

## ckpt-155 captures 4-5c — MOVEMENT works; the Z-draw path's weapon state is empty
USER (ckpt 155): sword IS wieldable right after the errands dialogue (same spot as the movement
tests); "inputs maybe not going through" (saw retail idle in the PORT traces — idle there BY DESIGN).
- **Capture 4 (added `wx`/`hvel`):** during a clean held-RIGHT walk, **Arche MOVED 16,680 units**
  (wx 19200→35880, hvel ramped to the 24000 walk cap).  So the frida injection DOES reach retail +
  take effect — the held-axis walk + the X(id8) auto-attack (cmd4=0xe) both fire.  Inputs ARE going
  through.  The issue is the Z-DRAW specifically.
- **The draw path (478ba0:473-481):** when sword-IN, Z(id9) → :323 `479960(0,200,1,9)` sets `local_614`
  (UNGATED) → if `atk94`(+0x14894)!=0 [=2] AND `local_614`, scan the `+0x14874` array (atk94 entries,
  {match@0,cmd@4}) for `entry == weapon+0xd4`; on match, `479de0` queues cmd → **entity+0x14868 (cmd[5])
  type 0xd2**.  THE context-action (draw) mechanism.
- **Capture 5c (DENSE id-9 bursts every 2 frames + the cmd5/wpn_d4 detectors):** `cmd5`=0 the whole
  time (the queue NEVER fired), `code`=0xc35a, `wpn_466`=0, and **`wpn_d4` (weapon+0xd4) = 0** — the
  context-action match key is EMPTY, so the `+0x14874` draw can't match.  Dense bursts ruled out the
  [0,200]ms window-aging hypothesis (or id9 reaches :323 but the match fails on wpn_d4=0).
- **Where it stands:** movement + the X path work via injection, but Z→draw needs `weapon+0xd4` (+ a
  matching `+0x14874` entry) populated, and it reads 0 in this fresh-new-game errands.  Either (a) the
  frida DIRECT-ring-write of id9 doesn't reproduce a NATURAL Z press (the producer 46a880 fills the
  ring + weapon state from the event queue 0x5ba3a0, which the direct write bypasses), or (b) the
  weapon/sword state genuinely isn't equipped in this captured state.  **NEXT (fresh): inject Z via the
  EVENT QUEUE (hook 0x5ba3a0) so the producer processes it naturally — the "improve the tool" fix — OR
  reach the exact in-game state (a save?) where weapon+0xd4 is set.**  The `.osr` visual won't help
  while the same ring injection underdelivers id9.

## ckpt-155 capture 6 — RESOLVED: the sword is gated on the errands QUEST reaching case 8
**THE ANSWER (USER was right — sword IS available in the errands):**
- **Capture 6 (id-7 JUMP control):** injecting id 7 → `cmd2=0x7` fired (43 frames) — **ring injection
  DELIVERS** perfectly (jump + walk both work).  So id-9's no-effect is NOT a delivery problem.
- **The gate:** `weapon+0xd4` (the `+0x14874` context-action match key) reads **0** because the errands
  questline `0x4dc510` hasn't reached **case 8** yet.  **`4dc510:1167` (case 8) sets `weapon+0xd4 = 2`**
  on Arche — THAT enables the sword draw.  The quest var (`FUN_0041e2f0(0x606aa50)`) advances
  2→3→4→5→6→7→8 via `FUN_0041d190(0x606aa50,N,0)` across `4dc510` (errands) + `4d7d80` (arrival/house)
  — the MAIN INTRO STORY PROGRESSION.  Case 0 plays the 3 tutorial lines (incl. the "[Z/X]" line); the
  sword unlocks at case 8, AFTER the tutorial progresses.
- My fresh-new-game captures sat at an early quest state (just past the opening dialogue) → `wpn_d4=0`
  → sword never enabled → Z does nothing.  **Not injection, not "no sword" — a QUEST-STATE gate.**

**NEXT (the path is clear):** reach quest state 8, then Z draws.  Two ways:
1. **Force the state for the capture** (RE): Frida-hook `FUN_0041e2f0(0x606aa50)`→return 8 (or write
   `weapon+0xd4=2`) so case 8 runs → sword enabled → inject Z(id9) → capture the draw + the sword-out
   cels + the attack (res 0x570 draw_probe / the proxy `.osr`).  A new agent capability (return-override
   / mem-write) — the "improve the tool" step.
2. **Play the tutorial** (natural): drive the errands moving-in tasks to advance the quest to 8.  Heavy nav.
For the PORT: the quest system is unported → the sword-enable (`weapon+0xd4=2`) is a `PORT-DEBT(sword-quest-gate)`
stand-in (set it at the equivalent errands point), like the cutscene-coords / ERRANDS_CAST precedents.

## ckpt-155 — THE SWORD CONFIRMED + the cel ground truth (USER's real-play recording)
USER recorded a real-play session under the proxy (visible window, `C:\oss-osr\sword-realplay.osr`,
540 MB, ticks 0-4290, a FRESH NEW GAME so the quest/tutorial state matches the port).  The sword
DRAWS — proven.  All cels are res **0x570** (Arche's bank; NO separate sword sprite — the sword is
baked into the sword-out cels).  Drawcall ground truth (`draw_probe`/the osr.py reader):
- **UNSHEATHE (draw) — cels 96→97→98→99→100→101→102→103, ticks 2156-2205 (~49 ticks ≈ 1 s).**  dst
  widths 41/35/54/67/41/33/32/31 (the sword swinging out).  Matches USER's "~1 s draw anim".  Triggered
  by Z (id 9) once the quest enabled the sword (`weapon+0xd4=2`, errands questline case 8).
- **sword-OUT IDLE — cels 0,1,2,3** at dst (222,336,~28x63), ~14t each (same base idle cels; the sword
  is out but the idle reuses the base pose — no distinct idle cels seen).
- **ATTACK 1 (X, neutral) — cels 120→121→122→123→124→123→122→121→122→123→124→125→126→127→120**, ticks
  2890-3070 (~12t/cel; a swing out 120-124 then a follow-through 125-127).  dst up to 30x72.
- **ATTACK 2 (X, a 2nd/combo or directional) — cels 128→129→130→131→132→…→128**, ticks 3820-4000.
  dst up to 45x54.
- (NOT yet captured: the sword-OUT WALK cels + directional attacks up/down — the USER mostly stood +
  attacked neutral.  Capture later by walking + holding ^/v while attacking with the sword out.)
- The cel 6 blip (ticks 1023-1027) is a sword-IN turn frame (pre-draw), not sword-related.

**WHY my injected captures failed (now fully explained):** the sword needs `weapon+0xd4=2` (errands
quest case 8); my ring-injected runs never advanced the quest there (the USER's natural play did, after
the opening dialogue).  Injection + movement were always fine (id-7 jump fired, walk moved her).

**THE PORT (the path is now concrete):**
1. **Sword toggle** — Z(id9) when `sword_out` is enabled → toggle a `sword_out` state + play the draw
   clip (cels 96-103, ~49t) / a sheathe clip on toggle-off.  Gate behind a `PORT-DEBT(sword-quest-gate)`
   stand-in (set "sword available" at the errands point; the quest system is unported).
2. **Attack** — X(id8) when sword_out → `arche_attack_clip` (cels 120-127 neutral; 128-132 the 2nd) +
   the attack TRAIL vfx (USER: "there's a trail effect").  Directional variants TBD (capture later).
3. **Sword-out pose set** — idle reuses 0-3 (verified); walk/crouch/jump sword-out cels TBD (capture).
4. Verify bit-exact off a port `.osr` vs this recording (draw_probe res 0x570 at the matching cels).

## ckpt-155 — the attack VARIANT set (USER, for next session)
The recording has the draw + 2 attack animations (cels 120-127 + 128-132).  USER's full attack set
(to capture/port NEXT session):
- **Neutral X repeated = a 3-ATTACK COMBO** (3 chained swings; the 2 captured here — 120-127, 128-132 —
  are likely combo hits 1+2; hit 3 not yet captured).
- **Each DIRECTION + X = its own attack animation** (→+X, ↑+X, ↓+X — distinct cel ranges, not captured).
- Plus the sword-OUT WALK/crouch/jump pose set (USER's "second anim set"; only the idle 0-3 seen so far).

**NEXT-SESSION capture plan (no user re-record needed — "synth by modifying the trace"):**
1. **Force the sword-enabled state** so injection can drive it: Frida return-override on
   `FUN_0041e2f0(0x606aa50)`→8 (runs questline case 8 → `weapon+0xd4=2`), OR a mem-write `weapon+0xd4=2`.
   A new agent capability (the "improve the tool" step) — `tools/frida/opensummoners-agent.js`.
2. Then inject input traces: Z (draw) → X×3 (combo) → →+X / ↑+X / ↓+X (directionals) → walk with sword
   out → capture each variant's cels off the port-side or a fresh retail `.osr` (draw_probe res 0x570).
3. Extract the cel IMAGES for 96-103 / 120-132 / the new ranges from the user's OWN game files (the
   faithful asset path, res 0x570) — NOT the recording's pixels.
4. Port: `arche_sword_clip` (draw/sheathe) + `arche_attack_clip` (combo + directional) + the trail vfx +
   the sword-out pose set; `PORT-DEBT(sword-quest-gate)` stand-in.

**GROUND-TRUTH recording:** `C:\oss-osr\sword-realplay.osr` (= `/mnt/c/oss-osr/sword-realplay.osr`,
540 MB, fresh new game, ticks 0-4290; the draw @2156, attacks @2890/@3820).  Keep it for verification;
re-record if lost (the proxy visible-window command in this session's log).
