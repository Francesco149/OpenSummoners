# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
████████████░░░░░░░░  60.2% touched   (15.7% host-tested, 58.0% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   249 | ported + module covered by the host unit suite       |
| ported      |   686 | reimplemented in src/, no host test for that module  |
| **touched** | **935** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |   889 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1532** below
`0x5bdab0`. The other **292** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1824** non-thunk
functions (of 1824 incl. thunks).

Code-byte coverage (58.0% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **2026-10-14 — rhost is THE porting loop; the MOVEMENT stack is ported and verified inside retail.**
  USER decisions (audit §7): deterministic cadence = rhost 1 update/flip; EN-old first (SE = a later
  delta layer); Frida retail drive RETIRED (legacy, don't extend). Port boot "Now Loading" frames now
  DEFAULT (`--no-boot-loading` = legacy axis; old port-timed traces keep working).
  Corpus: 8 scenarios incl. the USER's `rh-my-play` (15.7k flips: intro → house errands → town → shop;
  10 room loads) + `rh-errands-items` (16.4k flips: errands, item finds, the storage box) + `rh-combat` (57.6k
  flips, the USER 2026-10-16: combat, story advancement, a game over + retry). `rhost.py verify` (shadow, per call; full-state where a function writes widely):
  | function | VA | calls | div |
  |---|---|---:|---:|
  | tile movers | `0x54e990` / `0x54ded0` | 2.50M / 288k | 0 |
  | entity movers (actor scan) | `0x54e5c0` / `0x54db10` | 2.50M / 1.58M | 0 |
  | map decode (whole 2.9 MB grid) | `0x587e00` | 16 loads | 0 |
  | camera follow / shake | `0x43d1d0` / `0x43d340` | 21k / 42k | 0 |
  | character AI (input → cmd) | `0x478ba0` | 8,750 | 0 |
  | movement step (physics) | `0x467bb0` | 1.58M | 0 |
  | per-state clip + jump fx | `0x466930` | 977k | 0 |
  | the INTEGRATOR (dispatcher) | `0x442a70` | 1.03M (64k full-state) | 0 |
  | hazard probe | `0x440f40` | 31k | 0 |
  | NPC navigation AI (+ cluster) | `0x43f880` | 11,491 (all full-state) | 0 |
  | character apply pass | `0x485fc0` | 69,968 (17,492 full-state) | 0 |
  | the per-tick STATUS pass (doom / stun / weak / poison / floor damage, the pose statuses, haste / orbit shots / shields / attack up / guard / auras, the cure + decay) + the FADE, status decay, pending hit, log line, sprite / part descriptors, shot spawn | `0x487dc0` `0x4878b0` (+ `0x48bb30` `0x4891c0` `0x44e6c0` `0x44d160` `0x44d020` `0x55cfc0`) | 143,533 (2,241 full-state: the idle path — no status / fade / shot in the corpus) / 143,533 (2,241) / – / – / – / 1,194,769 (37,334) / – / – | 0 |
  | entity tick driver | `0x46cd70` | 6,877 (3,438 full-state) | 0 |
  | entity effect records (tick / add / splash) | `0x54d090` / `0x448cb0` / `0x46d7f0` | 649k / 218 / 186k (16k full-state) | 0 |
  | looping-sound channels (all / one / release) | `0x46d180` / `0x46d4a0` / `0x4537d0` | 256k / 227k / 36 (11.5k full-state) | 0 |
  | ACTOR update (per-code behaviours) | `0x54f980` | 706k (43.8k full-state) | 0 |
  | PARTICLE tick (96 codes + tail) | `0x46e510` | 438k (28k full-state) | 0 |
  | BEHAVIOUR AI (townsfolk / party / monsters) | `0x47b990` | 122k (32.5k full-state) | 0 |
  | behaviour helpers: step-ahead / target pick / walk-to / pace | `0x47dbb0` / `0x47e330` / `0x484c10` / `0x4857f0` | 3.3k / 2.7k / 8.4k / 224 | 0 |
  | SOUND layer: pool load / take, place, one-shots, voice lines, channel start | `0x409e90` `0x40b710` `0x40bb90` `0x411390` `0x407ba0` `0x40a010` `0x4099a0` `0x450ef0` | 35 / 335 / 27k / 43 / 831 / 299 / 312 / 772 | 0 |
  | voice cue + voice-set clock / find / line stop | `0x445fd0` `0x446400` `0x4464f0` `0x4463d0` (`0x446580`) | 283 / 10.3k / 21 / 829 | 0 |
  | entity lists + release (entity / actor / character), collide-list add, rider carry | `0x555fc0` `0x556180` `0x41ea70` `0x4467d0` `0x556380` `0x5563d0` `0x40bd90` | 5.8k / 27k / 1.7k / 416 / 273 / 87k | 0 |
  | per-character state (HP/MP, status gates, modifier reset, AI reset, respawn mark, pit timer, look-up, grid reads) | `0x5b54d0` `0x5b5580` `0x485f70` `0x40d960` `0x487530` `0x41e180` `0x487570` `0x487730` `0x41ef80` `0x4874b0` `0x4593c0` `0x487250` `0x4872d0` `0x426db0` `0x41ee60` `0x453960` `0x426440` | 143k mods / 122k pit / 88k targetable … | 0 |
  | ambient sound / idle voice timers, jittered one-shot, channel loop | `0x5531b0` `0x467380` `0x489280` `0x476880` | 40k / 5.3k / 216 / 116 | 0 |
  | CRATE (fall / break / push / slide), rider count, dust | `0x555790` `0x555bd0` `0x555e90` `0x5564e0` `0x40ab80` | 86k / 5 / – / – / – | 0 |
  | particle SPAWNER (slot pick + anchor; init `0x557550` still a slot) | `0x557370` | 8.7k | 0 |
  | grid refresh (growing pillars + cached-frame flush) | `0x54c640` / `0x54c9d0` | 16 | 0 |
  | particle INIT (per-code setup, 96 codes) + entity setup / tracks / tints / ride / x87 polar | `0x557550` `0x426620` `0x426d70` `0x448c80` `0x476710` `0x55cd50` `0x476c70` `0x460e40` `0x55cdb0` … | 8.6k (1.1k compared, 7 codes) / 9.5k / 9k / 22k / 465 / – / – / 36 / – | 0 |
  | skill availability, party lookup, floor-below scan | `0x402a60` `0x5b5990` `0x4060d0` | 157k / 8.8k / 13 | 0 |
  | GIMMICK pass (plates / levers / followers / timed plates, hits) + switching + links | `0x59afd0` / `0x59b170` / `0x40afe0` / `0x40be30` | 7.7k (240) / 227k (1,710 full-state, 16 switching) / 9 / 7 | 0 |
  | rect queries, riders, glow, serial, shot, warp, party gather, unlist | `0x4804b0` `0x556610` `0x5567a0` `0x5568e0` `0x556de0` `0x408100` `0x40be00` `0x40ac10` `0x54ecf0` `0x54efd0` `0x40c300` | 175 / – (unreached) | 0 |
  | event runner + resource unload | `0x40b8f0` / `0x43e3a0` | 38 (18 room-start / touch scripts skipped) / 33 (1 compared) | 0 |
  | TOUCH pass (doors / talk: probe rects, nearest script, door actors) + room input flush, probe rect, range check | `0x59bdd0` / `0x59c240` / `0x411560` / `0x4812c0` / `0x47b7c0` | 40 (32 compared, 8 skipped: the door / script calls) / 8 / 138 / 22 / – | 0 |
  | EVENT ZONES (leader in a zone → scene start: walk-in / press-up / hold-up, foe holds) + zone test, band-96 near | `0x59a1f0` / `0x59a7c0` / `0x59a880` | 8.7k (4,474 full-state; 3,868 with a zone under the leader, incl. 8 fires) / 8k / 1 | 0 |
  | ROOM LOOP per-tick calls: input update, character step (probe + AI), actor step, tile-anim phase, party check, camera lead, timed sounds | `0x468a20` `0x590410` `0x58ffd0` `0x58ff50` `0x59ca10` `0x59d3f0` `0x43e140` | 1.4k / 832 / 4.1k / 545 / 8.8k / 545 / 1.5k compared | 0 |
  | the room loop's party chores: warp countdown (+ the party fade-out, the screen dissolve), leader swap (+ zone re-arm), pickups (gold cap / items / the throttled notices), the party tick (low-HP beep, flash fade, queued item use), the quest counts + stand checks, pickup touch, item sparkle, item pick, notice reset | `0x59c750` `0x59aa40` `0x59ca90` `0x5908a0` `0x59a910` `0x49ce70` (+ `0x59acc0` `0x59ad20` `0x59ce60` `0x557060` `0x590660` `0x58ffc0`) | 8,750 (1,093 full-state) / 3 / 8,750 (1,093) / 8,742 (1,092) / – (after the field menu) / 30 / – (no pickups / item uses in the corpus) | 0 |
  | the UI toolkit core (window manager / windows / controls: open, init, take, grid add + rows / columns, text add + metrics, cursor / icon / frame styles, page fit, scroll) + money / inventory / party counts | `0x4124d0` `0x40f3e0` `0x411c50` `0x40df40` `0x411f40` … (24, `docs/plans/ui-toolkit.md` chip 1) | 76 / 68 / 87 / 378 / 445 / 411 / 71 / 69 … (22 of 24 reached) | 0 (`0x412570`: 1 shadow artifact — its object is on the caller's stack, which the shadow does not restore; REPLACE passes) |
  | the UI text layer (cell text + ruby pass, glyph records, %-code parse, word wrap, layout / control text set, the message key-token table: alloc / add / expand / defaults, the box cursor) + the SJIS search | `0x40fa00` `0x4031c0` `0x40f040` `0x40e5e0` `0x413ed0` `0x40e360` `0x43cf30` `0x411230` `0x43cf90` `0x410610` `0x410560` `0x40fd20` | 151 / 74 / 14 / 72 / 58 / 14 / 1 / 969 / 58 / 36 / 34 / 297k (8k compared) — EN text only: the SJIS wrap + ruby paths unreached | 0 |
  | the UI toolkit's builders (grid init / new, row add, cell icon, window title, framed window open, page arrows) + the shop's screens (message window, "Party Member Status" open / fill, "You have N", item fit, sprite set release) | `0x40f5c0` `0x4134f0` `0x40f800` `0x415860` `0x412c40` `0x40f1c0` `0x4103d0` / `0x4123a0` `0x4129b0` `0x412df0` `0x415a10` `0x417480` `0x415350` | 10 / 6 / 13 / 4,487 / 1 / 3 / 1 / 1 / 1 / 1 / 1 / 1 / 7,692 | 0 |
  | the UI RUNTIME: control keys (list cursor + key repeat, message advance), the control / window tick (typewriter, icon / arrow / cursor clocks), the message window update (confirm / cancel / skip, typing blip, auto-advance), the list input handler, jobs check | `0x43ca40` `0x43c920` `0x43ce50` `0x43c650` `0x43c2e0` `0x43c610` `0x43b980` `0x43bca0` `0x43c170` | 2,397 / 115 / 2,637 / 25,730 / 4,433 / 6,842 / 4,445 / 1,234 / – | 0 |
  | the MENU FRAME LOOP every menu runs on (pace, input, window ticks, the world, render + Flip) + its pacer | `0x55f190` / `0x55f550` | REPLACE only: 11 / 39 (rh-my-play) | – |
  | the SHOP: the driver (Buy / Sell choice, input reset, menu context), the stock list, "On who?" (member list + the equip preview loop) | `0x415060` / `0x415460` / `0x415f60` | REPLACE only: 2 / 1 / 1 (rh-my-play; each also replaced alone under retail's callers) | – |
  | the equip preview (worn >> new, price, the stat rows, the sprite pair + its recolour clone) + member rows, stat row, sprite get; the toolkit leaves (list window open, row + text, gauge clamp, cell colour), item lookup, room views mark | `0x4169e0` `0x4167a0` `0x418c90` `0x4175a0` / `0x411940` `0x412160` `0x412330` `0x4173f0` `0x4078a0` `0x41a140` | 1 / 1 / 6 / 2 / 1 / 3 / – / – / 9 / 1 — the recolour path (an item with a swap table) and the buy confirm `0x415ae0` unreached | 0 |
  | the CUTSCENE MESSAGE BOX: balloon placement over the speaker + tail, the fade in / out, the balloon / box styles, the cell-icon alpha; the world → screen projector | `0x49c640` `0x49c910` `0x43e0e0` `0x43e080` `0x43d470` / `0x490b90` | 8,207 / 8,857 / 63 / – / 86 / 16,287 sampled of 8.2M | 0 |
  | the SCREEN STEP every update runs (letterbox, window animations, fader, dissolve, HUD, overlays, notices) + the HUD animation (panels, status icons, leader cross-fade, gold roll, foe gauge, combo, party bars) + its helpers, the view box test | `0x499ab0` / `0x49af40` / `0x49bdf0` `0x496970` `0x49bc40` `0x49bd50` / `0x4766a0` | 2,169 sampled / 4,340 sampled / 479 / 405 / 4,321 / – / – (dissolve: reveal 652, cover 335, all 4 patterns) | 0 |
  | the SCREEN EFFECTS' parts: window open / close animation (fade / slide) + manager drop + control free, the dissolve row patterns (centre-out / edges-in / sweeps), the overlay fades, the notice popup + control reorder / anim set | `0x49a060` `0x49a340` `0x49a2f0` `0x49a470` / `0x49a890` `0x49a740` `0x49aa00` `0x49aae0` / `0x49abd0` `0x49ac80` / `0x49c2f0` `0x41c280` `0x49c620` | 820 sampled / – / 62 / 61 / 577 / 175 / 75 / 160 / – / 1,083 sampled / 2,169 sampled (idle path: no notice in the corpus) | 0 |
  | the SCENE COMMAND RUNNER — every cutscene beat's frame loop (message box build / text / portraits, effects, fades, camera, choices, the cast, voice, skip, wait modes; render + Flip) | `0x439690` | REPLACE only: 200 beats (rh-my-play 120 / rh-intro-errands 38 / rh-errands-walk 42) | – |
  | the CUTSCENE CAST STEP (walk-to with path planning + teleport fallback, face, wait for the action, stance / pose skills) + the path search (DFS over grid regions D / E) + near-x, face key | `0x43e5b0` / `0x43ef20` `0x43f370` / `0x441f00` `0x442600` | 635 / 220 (the DFS inside) / 359 / 139 — walk / face / wait reached; stance, pose, timeout and no-path branches not | 0 |
  | KEYBOARD input producer (DIK events → ring records in retail's test order, held axes) | `0x46a880` | 1,694 compared (every call with events: 1,164 events, title / menus / free roam) | 0 |
  | ENCOUNTER pass (roam / battle / strong-foe modes, foe-near flag, battle linger, room BGM restore) + threat scans, slot check, BGM pick | `0x58e780` / `0x58eeb0` / `0x58f0b0` / `0x412db0` / `0x40d9b0` | 1,471 (every call) / 1.4k / 1.4k / 63k / 1.4k — idle path (no foes in the corpus) | 0 |
  | the ROOM LOOP itself (pacing machine, update, keys / menus, passes, render + Flip) + input re-poll / flush | `0x58f360` / `0x49be60` | REPLACE only: every room of the corpus (rh-my-play 9 runs) | – |
  | the ROOM SETUP `load_room` (new world, pools, gate, map load, palette, NPC entry roll, party, result loop) | `0x586010` | REPLACE only: all 16 room loads of the corpus | – |
  | its helpers: body arrays, char links, draw pools, shake clear, volume fade, map-NPC id / test, slot status, gate actor, slot ready / tint, timed-sound fade, fill lookup | `0x58cf60` `0x58d020` `0x587d50` `0x587d30` `0x43c9b0` `0x58cfe0` `0x58cfc0` `0x58d060` `0x4022d0` `0x49a050` `0x491740` `0x587ce0` `0x587db0` | 1,446 (@16) / 512 / 16 / 32 / 61 / 72 / 72 / 16 / – / 19 / – / – / 78 | 0 |
  | the WORLD's construction / destruction + the entity frees | `0x4017d0` / `0x54bfb0` (REPLACE, in load_room) `0x401cb0` `0x54c2e0` `0x54c3c0` `0x401c70` | 16 / 16 / 1,016 (@16) / 1,664 / 416 / 512 | 0 |
  | room-setup leaves: leader, camera snap, zone reset, quest state, view record, state save, map-resource frees, pool-head free | `0x54bf90` `0x58e6a0` `0x58d0b0` `0x58d3b0` `0x58d210` `0x58e330` `0x5878a0` `0x59db70` `0x562d50` | 8,758 / 8,758 / 10 / 448 / 16 / 13 / 16 / 16 / 6 | 0 |
  | formation + follow set-up, actor state restore, palette tint, map resource load | `0x41dc90` `0x41e020` `0x41e600` `0x4182d0` `0x587970` | 16 / 71 / 452 / 34,304 / 16 | 0 |
  | the map DECODE as port code: the existing grid decoder fed the retail map object + the prologue's engine-side effects | `0x587e00` | REPLACE (in load_room): all 16 room loads | – |
  | sprite-slot helpers: frame frees (entry / all / every graded slot), palette set, swap table | `0x417870` `0x4118b0` `0x56df10` `0x491770` (REPLACE) `0x417bc0` | 13,240 / 3 (REPLACE) / 6 / – / 14 | 0 |
  | the ROOM POPULATION (map objects → characters / effects / devices / structures) | `0x58d460` | 16 (all full-state) | 0 |
  | the STRUCTURE init (box + track by code) + the handle list filing | `0x438a60` / `0x555f00` | 596 / 544 | 0 |
  | the DEVICE init (~95 codes: box, track, collision, steps, zones, lists) + sound-channel reset, step add, zone, link state, night glow, grid mark D, parts copy, sub-device spawn | `0x431e30` (+ `0x426490` `0x438630` `0x438670` `0x4386a0` `0x4386f0` `0x438520` `0x438740` `0x556ef0`) | 425 (34 codes) / 418 | 0 |
  | the CHARACTER INIT (dramatist override, resets, base stats + difficulty, pacing, ~150 codes: Arche / Sana / townsfolk / animals in place, the rest → 18 per-type inits as slots) | `0x41f200` | 103 (all full-state; 17 codes incl. the dramatist path) | 0 |
  | its 42 definition helpers: skill find / learn / define / by code, AI skill records + boxes, approach records, combos, base stats `0x426fd0` + jitter `0x426e00`, voice set init / free, pacing, idle, clips / walk / run / jump / crouch / hit / carry / move / nav setters, party mode, idle voice | `0x4072c0` `0x4282f0` `0x4285f0` `0x427fb0` `0x4287d0` `0x428950` `0x427f00` `0x427430` `0x426fd0` `0x426e00` `0x4289f0` `0x426380` `0x427670` `0x427360` … `0x431cb0` | 320 learn / 288 combo / 224 ai / 206 jitter / 103 stats … (33 of 42 reached; via retail's 0x41f200) | 0 |
  | the party characters' AI skill records (47 / 48 / 134..140 / 10113.. / 20101..20136 by skill: ranges, recharge, hold; the class fix) | `0x42eb20` | 16 | 0 |
  | the PARTY MEMBER create / attach (next free char, init at the gate's feet, the slot's stat block, pacing, brain, respawn) + equipment recolour, skill refresh, skill usable, member skill find, slot code + the sprite-slot clone / entry free | `0x58e170` / `0x5b4530` `0x5b4a10` `0x406ec0` `0x407320` `0x4071f0` `0x4094d0` `0x4179b0` `0x417b50` | 16 / 16 / 16 / 16 / 1,536 / – / – / 564 / 1,182 | 0 |
  | menu / dialog controller geometry free (retail heap; `menu_list.c` keeps the port-heap twin) | `0x40e0c0` | 542 | 0 |
  | the SCENE LOOP (the session: the area / fill / keep tables, the party room, new game / save load, the room loop of load_room with defeat / retry / menu / map-change handling) | `0x59f2c0` | REPLACE only: the whole session in 3 scenarios | – |
  | its helpers: scene object, event flags, area flag / find / cross-reference, slot reset, inputs, keep lists, retry / heal / gold, room loop new / free, room-end cleanup, frame cache | `0x41a7f0` `0x4c57f0` `0x4c5830` `0x560e90` `0x561c90` `0x585000` `0x560e60` `0x5611d0` `0x5851e0` `0x5a3de0` `0x5a3e20` `0x4cc5a0` `0x5a3c40` `0x585f00` `0x5a3ff0` (REPLACE) `0x562a70` … (34) | 1,251 xref / 29 / 24 / 16 … (21 of 34 reached) | 0 |
  | the room → scene snapshot (what a save stores: room fields, play time, items, members, fill table, flags) | `0x55f790` | 11 | 0 |
  | the PARTY API (the story setups by start area, start members from the config, levels, slot init, skills, starting items, leader, join / leave / dismiss, equip + stat refresh, class fit, item add) | `0x4c5350` `0x4e59a0` `0x408dc0` `0x4c5e00` `0x4c6160` `0x4c6990` `0x4c6830` `0x413b20` `0x408d40` … (25) | 6 / 6 / 6 / 3 / 3 / 3 / 3 / 6 / 3 … (11 of 25 reached: the new-game path) | 0 |
  | the member stats recompute (base + items, with an equip-preview item) | `0x4066d0` | 1 | 0 |
  | the scene start (next area / retry point, actor states, room snapshot) + a part's facing | `0x59cfc0` / `0x4842c0` | 8 / 8 | 0 |
  | the party POSE handler (turn-around, hit reactions + voice, stagger, knockdowns, blown-back falls, get-up, revive) + knockdown, status inflict, drift, switch touch, particle live, hitstop | `0x457ff0` (+ `0x459990` `0x4593e0` `0x4678f0` `0x467ae0` `0x459ed0` `0x5b1aa0` `0x459370` `0x54d060` `0x5b4510`) | 1.43M (72,685 full-state: every non-zero pose) / – / – / – / – / – / 414 / – / – / – | 0 |
  | the SCRIPT ENGINE (chapter scripts by area / flag, then the default event: room arrival + BGM + area title, doors, switched furniture / chest / save statue / talk records, time of day) + scene begin / end, room flag, leader, party cast, cast add, scene wait, actor by record | `0x40c380` (+ `0x49cb60` `0x49cd70` `0x41e2f0` `0x402f20` `0x402060` `0x401f50` `0x439680` `0x556eb0`) | 61 (11 full-state; 49 guarded: the BGM / frame loop) / 31 / 30 / 251 / 29 / 13 / 13 / – (REPLACE: 146) / 129; REPLACE: every event of the corpus | 0 |
  | the scene COMMAND builders (message box + voice row, face, weapon stance, talk turn, flag set, camera, fades / waits / pause, area title, door place / spawn / by record, next event, room BGM restart) | `0x49d6e0` `0x4a5ee0` `0x401e60` `0x4a5e20` `0x402d40` `0x406210` `0x41d190` `0x4a5f50` `0x401d40` `0x4a5f30` `0x4a5f90` `0x4cc060` `0x4cc080` `0x4cc0a0` `0x4cc0c0` `0x403100` `0x41ec20` `0x41f0e0` `0x4cbf80` `0x4cca70` | 101 / 34 / 7 / 1 / 13 / 13 / 8 / 18 / 6 / 1 / 5 / 1 / 2 / 2 / 1 / 3 (guarded: the title text) / 1 / 15 / 1 / 2 (guarded: the BGM) — from retail's chapter scripts | 0 |
  | the STORAGE BOX (Item Storage → Deposit / Withdraw: the stored flag, the 128 / 48 limits) + the item list (filters, sort, icons, "E:" wearer, count / sell price, cursor) + the message windows (centred box, paged box + style), the menu text set, party window close, item counts, key-token replace | `0x411610` `0x413130` `0x414cb0` / `0x414490` / `0x413b70` `0x4140c0` `0x414410` `0x412590` `0x4129a0` `0x407880` `0x407900` `0x4139d0` `0x412800` | REPLACE: 2 / 4 / 5; shadow: 13 / 9 / – / – / 23 / REPLACE 2 / 3 / 2 / 2 / 1,056 (rh-errands-items) — the sort toggle, refusals and paged box unreached | 0 |
  | the FINDS (coins / items "You found…", the inventory add, put back) + the furniture search | `0x4081d0` / `0x407970` | REPLACE: 2 (copper coins + an item) / 6 — silver / gold / full / empty / the chest 70220 unreached | – |
  | the story scripts' COMMANDS: cast walk-to (map actor / record / relative), cast clear, the party gather, Yes / No, device spawn, character unload, gifts ("You obtained…"), the script switch | `0x402330` `0x4d64b0` `0x402730` `0x4061d0` `0x405e80` / `0x41a2a0` / `0x431d10` `0x41e360` / `0x407500` / `0x40a810` | 1 / 1 / 4 / 30 / 33 / REPLACE 2 / 18 / 1 / REPLACE 1 / 14 | 0 |
  | the ATTACK pipeline: power rolls (physical / skill) + magic sum, the lunge, a skill's hit boxes (+ twin side hits), the hit box spawn, the target search | `0x4481f0` `0x447f30` `0x4483a0` / `0x447ed0` / `0x447900` / `0x5b15e0` / `0x5b1c50` `0x4478e0` | 3,318 / 801 / 801 / 621 / 8,827 / 4,119 / 554 / 33 — no foe struck yet (`0x5b3890` / `0x5b5600` slots) | 0 |
  | the skills' HELPERS: the slash stages, orbits (x87 polar), the angle wrap, particles + twins, the slide step, the pose set, the afterimage, the crate push | `0x461240` / `0x466700` `0x460ed0` / `0x460de0` / `0x4610e0` / `0x4665d0` / `0x467500` / `0x44ce20` / `0x466e80` | 17,821 / 60 / 12,670 (791) / 36 / 36 / 680 / 4 / 224 / 2.19M (8,539) | 0 |
  | the BOOK reader + the sprite set free | `0x40dbc0` / `0x410300` | REPLACE: 1 / – | – |
  | TAKING A HIT (hit sound, part box, status roll, the damage chain, the number, stagger / flinch / knock-back, worn + inflicted statuses, guard / block / sparks) + the hit COMBO, slot scale, record damage, glyphs, crate knock; the DEATH + drops / pickups / the EXP split (level-ups) / "Exp +%d", part release | `0x4895c0` `0x5b3f00` `0x5b4130` `0x5b4170` `0x48b850` `0x48b8e0` / `0x5b4d00` `0x5b5020` `0x556990` `0x5b5140` `0x5b34d0` `0x525ab0` | 115 (all full-state: the combo, 18 deaths, 55 pickups, the EXP nested) | 0 |
  | a hit BREAKING SHOTS + the shot's end; the FOE GAUGE + its range test | `0x5b3890` `0x478a50` / `0x5b5600` `0x5b58c0` | – (no shot broken yet) / 1,252 | 0 |
  | SANA's skills (50011 / 50012: staff swing, magic shot, spell bursts, spin swing, back-flip, rest, idle) + the skill helpers (magic roll, clip sequence, pose swap, shot fire, particle tint, sprite descriptors, voice priority) | `0x45e830` (+ `0x4483c0` `0x4611c0` `0x461410` `0x447c10` `0x450590` `0x4505c0` `0x409970`) | REPLACE (hybrid) + shadow (wine 0 div to f27.6k) / 95 / 187 | 0 |
  | ARCHE's sword skills (50010; a `tools/ida/decomp2c.py` first pass) + the combo skill lookup | `0x461790` / `0x466750` | REPLACE (hybrid 9/9) | – |
  | the SKILL CASTS (the charge, the stage, the MP payment; summons: recall, the room test, space ahead, init, spawn; a decomp2c first pass) | `0x45a300` (+ `0x459f20` `0x45db20` `0x45a2c0` `0x456a50` `0x456b60` `0x45e050` `0x45e4c0` `0x55ce70`) | REPLACE (hybrid 9/9) | – |
  | the MONSTER inits: slimes, skill-51 / skill-29 types + the AI hold, skill list | `0x42e290` `0x42c970` `0x42fbf0` (+ `0x42ea50` `0x42da20`) | 19 / 4 / 19 | 0 |
  | the SHOTS (per-part tick, init, the 96-band pickups, the room loop's band-64 tick + 30 helpers; decomp2c) | `0x4710c0` `0x55d140` `0x46da00` `0x590230` (+ `0x445ce0`..`0x55f150`) | 562@64 / 13 / 11,481 / 562; helpers 226,320 (`0x476550`) 13,958 12,503 5,292 | 0 |
  | the BEHAVIOUR layer: every slot of the AI tick `0x47b990` + callees (skill pick by range, attack windows, approach / retreat / leash, target checks, the path follower; decomp2c) | `0x47ecd0` `0x47adb0` `0x47ae00` `0x441f50` … (60) | 10,375 / 8,987 / 8,023 / 5,628 / 5,529 / 5,388 / 3,679 … (34 hooks with calls) | 0 |
  | the INTEGRATOR's reached handlers (monsters' skill bodies) + the ITEM / SKILL EFFECT + its value (decomp2c) | `0x455d80` `0x44d2e0` `0x4512d0` `0x4466b0` `0x467260` `0x456c60` / `0x5b3b00` `0x5b28a0` | 112,293@8 / 31,853@8 / 24,751@8 / 4 / 3 / – / 1 / 1 | 0 |
  Hybrid gate (`corpus --check --hybrid`): with all of the above REPLACING retail (641 hooks) (the whole character tick, driver → tile
  movers, + the room loop's passes and per-tick calls, runs as port code), every corpus scenario ends byte-identical
  — and on wine matches pure retail at EVERY 600-frame sample (the sem series, 2026-10-16: catches transient
  divergences a later room reload erases; it took pinning rhost's own footprint — asset DLL slots, host heap,
  hook-spec file, image size; `docs/PROGRESS.md`).
  Stack objects (rhost, 2026-10-16): GX shadows save / restore / compare a window behind every pointer into a
  caller's stack frame (the integrator's body copies, menu contexts) — before, those writes went unchecked.
  Shadow guard (rhost): a port phase that reaches an external effect retail already performed (surface free,
  buffer release, DeleteObject, DirectInput, a mode-0/2 script) aborts its comparison (counted skipped); those
  calls are proven by REPLACE. `corpus --check` with the shadows on stays PASS (no perturbation).
  Infra: `src/world_layout.h` (retail-layout world/entity/char/body/cmd, pinned on 32-bit),
  `src/engine_calls.h` (slots → retail VAs for unported callees, PORT-DEBT(engine-call-slots)),
  `rh_shadow_ex` (full-state shadow; port code may call retail), `rng_bind`, heap zero-on-alloc
  (quirk #130), Windows binary `oss-rhost.exe` (openrecet kills `rhost.exe` by name).
- **Integrator `0x442a70` LANDED** (`src/char_integrate.c`): prologue, action start, poses 0x20/0x68..0x6a,
  the full per-type dispatch (~30 type handlers as slots), tail; special poses 0x75..0x81 delegated to
  retail (PORT-DEBT(integrate-special-poses); none occur in the corpus yet).
- **NPC navigation AI `0x43f880` LANDED** (`src/npc_ai.c`: + `0x4412d0` try-command, `0x441ae0` floor
  ahead, `0x4418e0`, `0x440e40`, `0x441700`, `0x43ee30`, …; scratch bodies from retail's heap via
  `g_eng.op_new`). Hybrid gate with all 12 movement functions replacing retail: PASS.
- **Next (port side):**
  0000. (2026-10-16 evening) the shots, the behaviour layer, the integrator's reached handlers + the item / skill
     effect run as port code; the hybrid gate holds at every 600-frame sample (wine).  Next: the integrator's
     UNREACHED handlers (`0x4539b0` 8.7 KB, `0x456d30`, `0x452730`, … — a recording that reaches them first); the
     scene loop's `0x561560` / `0x561d20` (32 fns, 10 KB); the chapter scripts (`0x4b4d40` … 20+ KB each, the runner
     `cls_401D40` is one of the 13 moved-field classes decomp2c now lays out right) after their ~40 small callees;
     the room loop's menu / game-over flow `0x591120` (131 fns, file I/O + BGM: later).  Loop: decomp2c module →
     wire → hooks → `verify` (shadow; sample hot helpers `@N`: full-state shadows of per-tick helpers crawl) →
     `corpus --check --hybrid` (wine: series; win: end) → `hookbisect` for a breaking hook.
  000. `load_room 0x586010` RUNS AS PORT CODE (`src/room_load.c`, REPLACE-verified; quirk #138), the world it makes /
     frees too (`src/world_alloc.c`), the map decode (`map_decode_room`), the population + device / structure
     inits, and now the CHARACTER INIT `0x41f200` (`src/char_init.c` + 42 helpers `src/char_def.c`; quirk #139).
     The party member create `0x58e170` / attach `0x5b4530` too (`src/party_member.c`; quirk #140).
     Slots left under it: the 17 per-type inits `0x428c00` `0x429060` `0x429db0` `0x42a500` `0x42a9a0` `0x42b9a0`
     `0x42c130` `0x42c510` `0x42c970` `0x42cce0` `0x42d3c0` `0x42da50` `0x42e290` `0x42f840` `0x42fbf0`
     `0x42ff00` `0x430400` (monsters / bosses / party allies; ~30 KB, built from the char_def helpers + 0x41e020 /
     0x426db0 / 0x556380 + a few leaves 0x429d00 0x429d50 0x42b550.. 0x42da20 0x42ea50) — **the corpus reaches NONE
     of them** (no dungeon / monster room: 17 townsfolk / party codes only), so porting them needs a monster-room
     scenario first (USER `rhost.py record`); the palette read `0x4178e0` + frames `0x418470` / `0x417c40`
     (DirectDraw), `0x56dc20` (Now Loading + Flip), `0x41a890`, the joysticks `0x5ba120` / `0x5ba290` (platform). The SCENE LOOP `0x59f2c0` above it runs as port code too
     (`src/scene_loop.c`, quirk #141; its save-load half is unexercised: a save scenario is needed). Its slots: the
     save writer `0x41a890`,
     the save apply `0x561d20`, the demo scene `0x41b670` (+ `0x4c5af0` / `0x52fd80` / `0x5a3870`), the menu
     `0x5a00c0` (13.7 KB), the file reader `0x5b6060`.. (platform). The party API (`src/party_member.c`, quirk
     #142) is ported; its slots: the level-up stats `0x409150`, the cue `0x55fdb0`, the level / status sync `0x41d780`, the start gear `0x4e6520` / `0x4e61d0`. Next: `0x41a890`
     (the save writer, every room start).
  00. the ROOM LOOP `0x58f360` RUNS AS PORT CODE (`room_loop_run`, REPLACE-verified; Flip / the camera
     easer go through rhost's own stubs), with the script engine / scene commands (`src/script_engine.c`,
     `src/scene_cmd.c`), the status pass (`src/char_status.c`) and the party chores (`src/room_tick.c`). Left
     under it as slots: the scene runner `0x59ad60`, the
     menus (`0x59d540` / `0x591120` / `0x590480` / `0x590f20` / `0x590ca0` — on the now-ported UI toolkit), the shots' band `0x590230` (+ `0x4710c0` 20 KB) / band 96 `0x46da00`, the
     render `0x48c150` + Flip (platform), `0x409520` `0x468b10` `0x415350` `0x495e40` `0x59d160` `0x5bc0c0`;
     in the passes: the key use `0x54ed80`, the input re-poll `0x40fe00`, the shot emitter `0x40ac90`, the
     shot init `0x55d140` (8 KB), the fall `0x5b4d00`, the item use `0x5b1f10` + effect value `0x47ae00`;
     under the scripts: the ~65 story scripts (`script_call`; the scene command runner `0x439690` is ported:
     `src/scene_run.c`).
  0. the Arche handlers: the POSE handler `0x457ff0` is ported (`src/arche_pose.c`, quirk #143; the knockdowns /
     status inflict unreached: no combat in the corpus); left: `0x45a300` (14 KB, the skill casts; also 0x446900 /
     0x4484b0), `0x461790` (19 KB, Arche's sword skills 10101..; its helpers 0x4665d0 0x447900 0x5b15e0 0x447ed0
     0x447f30 0x4483a0 0x450590 ...); the apply pass's status tick `0x487dc0` / fade `0x4878b0` are ported; the DirectSound
     calls `0x5bb250`..`0x5bb8e0` + `0x5b9390` stay platform slots (PORT-DEBT(engine-call-slots)).
  1. the band updaters under the tick driver `0x46cd70` (`src/world_tick.c`): ALL ported except shots `0x4710c0` (20 KB)
     + the 96-band `0x46da00` never run in the corpus (no combat yet). Arms ported but NOT yet exercised by
     the corpus (need recordings that reach them): `src/entity_fx.c` water splashes + records
     100150..100157; `src/actor_update.c` codes 70100/70120/70216/70230/70240/70360/70375/70390/70480/
     70520/70521/70840/70850/100138/100142..100145 (+ the slot-only codes); `src/particle_tick.c` every
     code but 100077/100078/100080/100081/100082/100100/100104 (incl. the x87 orbit 100131, which needs
     the port binary on MSVC's 53-bit FPU precision); `src/char_behave.c` the cutscene random walk
     (+0x15ba8) and the monster / party modes. The behaviour helpers behind `0x47b990`'s slots: `src/behave_ops.c` has
     the step-ahead check, the target pick, walk-to / pace / hold / go-home / follow + the small ones;
     the rest (`0x482ac0`..`0x485990`, `0x47ecd0`, the path follower `0x441f50`) are the next layer down —
     the corpus reaches only these four (+ `0x484c10`'s early return, `0x47e330`'s self-centred pick). The actor behaviours behind
     slots: `0x555790` (doors), `0x554c40`, `0x553ec0`, `0x553520`, `0x552c70`, `0x552f60`, `0x554570`,
     `0x54f140`, `0x5556d0`, `0x5531b0` (ambient timers), the channel start `0x450ef0`;
     the type handlers behind the integrator (Arche: `0x45a300`, `0x461790`, `0x45e830`).
  2. SWITCH the port's freeroam drive onto the retail-layout world (`g_world`, wl_char bodies,
     `char_ai` → `char_integrate`) and retire the `character.c` reductions; prove it with
     `--det-clock` portshots/portstate vs rhost on `rh-my-play`.
  3. room transitions (quirk #134): the room loop `0x58f360` (its pacing machine IS the cadence
     model — port it and `--det-clock` gives rhost's 1:1 for free), exits = door actors via
     `0x59afd0` → `0x59b170` (gimmicks), events `0x59a1f0`; `load_room 0x586010` + the scene loop
     `0x59f2c0` switch. The map decode is already byte-exact. A loop function is verified in REPLACE
     mode (corpus/hash), not per-call shadow.
  4. port-side DIK input: the producer `0x46a880` is PORTED (`src/input_dik.c`, retail layout); wire the port
     binary's keyboard path through it (`--rh-trace` = the same DIK traces rhost replays).
- **Loop (per function):** read `decompiled-ida/enold/all_ida.c` (+ asm for conventions/widths) →
  port on world_layout types → wrapper in `tools/rhost/rh_hooks.c` → `rhost.py verify` + `corpus
  --check --hook replace:…` → commit with the evidence line. Coverage of a path: `--trap VA` counts.
- History of the pre-rhost front (ckpt 175–194): `docs/memories/front-archive-ckpt175-194.md`.

> Hand-edited in `docs/FRONT.md`, injected here verbatim so it can't drift.

## Where to read next

- `STATUS.md` (this file) — 60-second orientation.
- `../CLAUDE.md` — the dense auto-loaded entry point (conventions, parity model, paths).
- `FRONT.md` — the hand-edited current front (source of the block above).
- `HANDOFF.md` — rolling current-checkpoint detail (module layout + open threads).
- `parity-model.md` — the multi-pillar model; `parity-ledger.md` — confirmed-1:1 guard.
- `port-debt.md` — synthetic/MVP shortcuts owed back.
- `port-ledger.md` / `.json` — per-function port status (derived).
- `port-frontier.md` — what to port next: unported fns reachable from ported
  code, zero-dep leaves ranked (derived; `tools/gen_frontier.py`).
- `ROADMAP.md` — milestones + subsystem map with difficulty / target module.
- `PROGRESS.md` — dated narrative changelog.
- `findings/INDEX.md` — map of subsystem RE writeups.
- `findings/engine-quirks.md` — the running quirk log (cite in commits).
- `AGENT-WORKFLOW.md` — when to spawn subagents vs stay in the main loop.
- `PLAN.md` — goal, constraints, phased roadmap.
