# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
████████████████████  99.9% touched   (15.7% host-tested, 100.0% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   268 | ported + module covered by the host unit suite       |
| ported      |  1290 | reimplemented in src/, no host test for that module  |
| **touched** | **1558** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |   266 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1532** below
`0x5bdab0`. The other **292** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1824** non-thunk
functions (of 1824 incl. thunks).

Code-byte coverage (100.0% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **2026-10-14 — rhost is THE porting loop; the MOVEMENT stack is ported and verified inside retail.**
  USER decisions (audit §7): deterministic cadence = rhost 1 update/flip; EN-old first (SE = a later
  delta layer); Frida retail drive RETIRED (legacy, don't extend). Port boot "Now Loading" frames now
  DEFAULT (`--no-boot-loading` = legacy axis; old port-timed traces keep working).
  Corpus: 11 scenarios (+ `rh-title-exit` 2026-10-24: title → Exit, the game-exit path) incl. the USER's `rh-early-game` (137.7k flips, 2026-10-23: more of the story, a statue save,
  selling at the shop, the main menu's Status / Items / Abilities / Party Commands) + `rh-my-play` (15.7k flips: intro → house errands → town → shop;
  10 room loads) + `rh-errands-items` (16.4k flips: errands, item finds, the storage box) + `rh-combat` (57.6k
  flips, the USER 2026-10-16: combat on the way to school, a party ally fighting, a statue save, the main menu, a game
  over + retry). **Save chains (2026-10-17):** `rhost.py record NEXT --from PREV` continues a play from PREV's final
  save (re-derived from PREV's trace on every replay; nothing committed but the trace). `rhost.py verify` (shadow, per call; full-state where a function writes widely):
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
  | the CHAPTER SCRIPTS the corpus reaches (35; decomp2c, callees bound to their ports; `0x49f7c0` on since 2026-10-23: its book record) + the later chapters' STORY SCRIPTS (`src/story_scripts.c`, 48 incl. helpers, UNVERIFIED: unreached) | `0x4d7d80` `0x4dc510` … `0x4c45a0` (~240 KB) | REPLACE (nested frame loops) | – |
  | the chapter scripts' file / sound / DirectX-free callees (scene / script commands, flag table, runner getters; decomp2c; the scripts now call them directly) | `0x401d80` `0x402030` `0x4020a0` … `0x5b4330` (50) | 91 shadow calls over 11 functions (`0x41efe0` 17, `0x4a63d0` 64, …); 39 unreached = UNVERIFIED (header lists them) | 0 |
  | the TITLE and its menus (decomp2c): the title drive, splash + title menu, New Game options, Options, the load screen, sparkles / compositor | `0x562ea0` `0x56aea0` `0x564780` `0x568de0` `0x566a80` … (59) | REPLACE (30 entry hooks; title / options / new-game / load flows of the corpus) | – |
  | the render's PLATFORM layer (decomp2c): surface DC get / release, lock / unlock, the blits (onto / keyed / clipped / rects), the software ALPHA blender `0x5bd680`, the surface cache (create / free / color key), Polyline | `0x5b9b70` `0x5b9bf0` `0x5bd550` `0x5bd680` `0x5b94e0` `0x5b8b40` … (23) | REPLACE (20 entry hooks: every blit of the corpus; the render gate) | – |
  | the frame RENDER (decomp2c `--typed`): the per-frame render `0x48c150` (room / scene / menu), its bands (characters, actors, effects), the actor / character emitters, the message box, the HUD (party panels, gauges, notices), glyph text, the sprite-frame resolver `0x418470` + palette grade `0x417c40`; the zdd / bitmap / blit platform layer + the frame loaders stay retail's | `0x48c150` `0x491ae0` `0x493ba0` `0x494e60` `0x48c820` `0x417c40` `0x418470` … (78) | REPLACE (11 entry hooks; the render gate: pixel-identical every frame) | – |
  | the room loop's MENUS (decomp2c): the main menu, Messages (list, one message, the line clip `0x5984c0`), System, the party / item hotkey menus, the menu context, the scene runner | `0x591120` `0x597aa0` `0x5985f0` `0x5984c0` `0x596d60` `0x590480` `0x590f20` `0x590ca0` `0x59d540` `0x59ad60` … (13) | REPLACE (9; rh-combat: menu 4, scene runner 4,206; retail's own entries 0 hits) + shadow `0x591590` 4 / `0x5984c0` 40 / `0x598460` 2 / `0x56c430` 1 | 0 |
  | SAVING / LOADING (decomp2c): the statue dialog, the slot list (24 rows: Save Point / LV / time / date), the WRITER (room-start snapshots + the slot file), the READER, the snapshot prep, the APPLY, the file name | `0x41a430` `0x41bd20` `0x41bf60` `0x561560` `0x41a890` `0x41b670` `0x55f620` `0x561d20` `0x560530` `0x41cfd0` `0x561bf0` `0x561cf0` (12) | REPLACE (7; writer 40 in 5 scenarios, the reader's 49 row / slot reads now port code) + shadow `0x55f620` 1 / `0x561cf0` 25 / `0x560530` 2 / `0x561bf0` 1 / `0x41cfd0` 1 / `room_to_scene` 70 (ret compared) | 0 |
  Hybrid gate (`corpus --check --hybrid`): with all of the above REPLACING retail (811 hooks; port→port calls through engine slots / RC_ reach the replacing ports too) (the whole character tick, driver → tile
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
  the full per-type dispatch (~30 type handlers as slots), tail; the special poses 0x75..0x81 too
  (`src/char_special.c`, 2026-10-23; pose 0x78 reached: 54 calls, 0 div).
- **NPC navigation AI `0x43f880` LANDED** (`src/npc_ai.c`: + `0x4412d0` try-command, `0x441ae0` floor
  ahead, `0x4418e0`, `0x440e40`, `0x441700`, `0x43ee30`, …; scratch bodies from retail's heap via
  `g_eng.op_new`). Hybrid gate with all 12 movement functions replacing retail: PASS.
- **THE STANDALONE (2026-10-23 late night, `docs/standalone.md`):** the port runs with NO retail code —
  `build/rhost/opensummoners-standalone.exe` (the product: drop it in the game folder, stock Steam `sotes.exe` works,
  real platform) and `rhost.exe --standalone` (deterministic: the corpus gate).  Data sections only (code range = INT3
  + 1,360 `jmp` thunks into port code), asset DLLs as data, the port CRT `src/msvc_crt.c` (the game's 16 CRT entry
  points + `start`).  `corpus --check --standalone` = PURE RETAIL's frame / tick / RNG / every frame's pixels: PASS
  9/9 Windows + wine (state hashes: PORT-DEBT(crt-heap)).  Proof: every run writes `standalone_audit.txt` (refuses to
  run if it fails); `tools/standalone/prove_no_retail_code.py` re-derives it from the files (image / binary / source /
  audit).  Survey: no script / bytecode format in the exe (`findings/exe-data-interpreted.md`).
  **Real Windows (2026-10-24):** the USER's launch stopped at "It failed in the initialization of ZDI" (real dinput
  rejects the hand-mapped HINSTANCE 0x400000; wine doesn't check) → fixed in live mode; fullscreen fails as RETAIL's
  own does on Windows 11 (quirk #177) → the product relaunches itself under `DWM8And16BitMitigation`.  Windowed +
  fullscreen boot to the title on the USER's box.  The exit routine `wm_5b10d0` read a global's address for its value
  (decomp2c rule, fixed at the source); new scenario `rh-title-exit` covers the game exit.
- **Next (port side):**
  0000000000. The USER plays the standalone: any INT3 stop names a retail VA the play reached with no port (log +
  message box) → port it, add its row.  Then: PORT-DEBT(crt-heap) (MSVC6's SBH from the decompile → standalone state
  hashes = retail's: the full corpus gate), the format docs the survey owes (text markup, MPED2DT, anim clip 0x194,
  sotesd res 1986).
  000000000. (2026-10-23 night, no new traces) WHAT RETAIL CODE STILL RUNS under the full hybrid: `rhost.py coverage
     --hybrid` (INT3 on every retail entry while all ports replace retail) found 51 game functions / 18 KB — now 4 / 79
     BYTES, all CRT internals (srand behind rhost's seed stub, 0x5c071e / 0x5c7c05 / 0x5c8d47); the rest of what runs
     is the MSVC CRT (FLIRT).  Closed: the integrator's SPECIAL POSES (`src/char_special.c`, quirk #174; the corpus
     reaches pose 0x78: shadow 54 / 54 full-state, 0 div — the old shadow had compared retail with retail there), the
     shadow GUARDS now call through the replacing ports (17 functions ran retail's body via a guarded slot), the wait
     window's WndProc 0x401210 + the CRT's static initializers (`src/static_init.c`, by address).  Hybrid gate: 1,338
     hooks, PASS 10/10 Windows + wine.  The drop-in's remaining gap is the CRT + the platform (rhost's determinism
     layer: clock, DirectInput, files) — not game code.
  00000000. (2026-10-23 evening) the USER's `rh-early-game` joins the corpus: it reaches 82 functions no scenario did
     (47 KB: the main menu's screens, the item use 0x5b1f10, the shop's Sell 0x4192f0, the start gear 0x4e6520, …) —
     all already port code, UNVERIFIED until now.  First hybrid run: an AV at f32682 in Abilities 0x5956c0 (an IDA
     register-reuse misread, HANDOFF); fixed, the hybrid gate PASSES on it (Windows: end state + every frame's
     pixels; wine: the 600-frame series).  9 newly reached shadow hooks: 1,299 calls, 0 div.  The new slots made
     `state_sem` unstable at its end ⇒ `det --calibrate` (+1,229 volatile slots) + every scenario re-blessed.
     Still unreached: the level-up screen 0x5915c0, the later chapters' story scripts, char_types / char_handlers2
     (new monsters), actor_behave, sjis_text, gamepad, the ending.
  0000000. (2026-10-23, an overnight run, no new traces) the PLATFORM LAYER and the program TOP run as port code, and
     every unreached group outside the story scripts is ported: `frame_load` (frame loaders + bitmap session),
     `file_stream`, `input_dev` (DirectInput), `sound_dev` (DirectSound + the DirectShow music manager), `zdd_dev`
     (DirectDraw + the FLIP: rhost's flip stub retargets its jmp), `win_app` (the pump, the window, config),
     `asset_load` (the boot asset groups), `win_main` (_WinMain + WinMain: under REPLACE the whole program runs from
     the port's top), `leftovers` (the AI's ring helpers, the logger, the launcher); UNVERIFIED (unreached) batches:
     `menu_screens` / `menu_helpers` (Status / Items / Abilities / Party Commands, the item use), `char_types` (14
     per-type inits), `char_handlers2` (18 integrator handlers), `actor_behave`, `sjis_text`, `gamepad`, `ending`,
     `title_extra` (Options pages, the attract demo, the opening; the input re-poll 0x40fe00 reached), `engine_rest`.
     Hybrid gate: 1,271 hooks, PASS 9/9 Windows + wine.  Ledger: 94.9% of engine functions touched, 76% of bytes.
     REGENERATED: the older decomp2c modules re-converted with today's converter (`tools/ida/decomp2c_regen.py`,
     hand finishes kept; their RC_ calls now bind to the new ports; latent bugs — `*(T *)v->field` precedence,
     unsigned Win32 returns, offset-only arrays, class-pointer `++` in for headers, `trailer`'s byte — fixed in
     place); hybrid PASS 9/9 Windows + wine.  The later chapters' STORY SCRIPTS 0x4e1d10..0x546670 are port code
     too (`src/story_scripts.c`, UNVERIFIED: unreached), and chapter script 0x49f7c0 is on again (its argument to
     the book reader is retail's book RECORD, not a host copy of the title).  Ledger: 98.0% of engine functions
     touched, 100.0% of code bytes; frontier = 3 small leaves.  rhost: by-address functions (window
     procedures, atexit, callbacks, jmp-thunk targets) take an ENTRY PATCH under REPLACE (game memory keeps retail's
     address); 3 stack-address slots joined `volatile.enold.txt` (re-blessed).
  000000. (2026-10-17 afternoon) coverage over the corpus (`rhost.py coverage`): 1,068 of 1,550 game functions reached;
     reached but unported (rhost modules, engine-proper) ~120 KB outside the platform layer — the corpus ALREADY
     reaches monsters, a party ally's AI, the menus, a statue save and a game over (rh-combat), so these are all
     verifiable now.  Done today: the room loop's menus (`src/field_menu.c`) + saving / loading (`src/save_game.c`).
     Left, by size: the boot asset group loaders 0x56e190 / 0x57ca40 / 0x5752e0 / 0x57b280 / 0x57a330 / 0x582e90 /
     0x5749b0 (~85 KB, every scenario; DirectDraw surfaces + resource reads), WinMain's init 0x5a4770 (46 KB, platform
     heavy), the script callees 0x408730 (gifts) / 0x409520 / 0x40a340 / 0x41a160 / 0x41d4a0 / 0x408b90 / 0x40a5d0
     (BGM), the AI skill pick 0x479f90, the frame loaders 0x4184a0 / 0x4188b0, the input re-poll 0x40fe00.  (0x54e4f0
     and 0x58c8c0 / 0x58c8d0 are inlined in the hand ports of the tile mover / map decode.)  NOT the menu: 0x5a00c0 is
     the ENDING CREDITS ("Produced by" …), unreachable until the game is beaten.  decomp2c now resolves a call's ecx
     from the caller's asm (`lea ecx, [esp+…+var_N]` → that local) — the "caller's this" guess put the scene object
     in the reader's file calls; `tools/ida/decomp2c_drift.py` lists committed functions today's decomp2c would emit
     differently — run 2026-10-17: 25 functions in 9 modules, all benign (callees now bound to their new ports instead
     of RC_, the hand finishes, `*v = x` spelling) except 0x4d64f0's table address (fixed, 42f286c); the ecx resolver
     changes no committed call site.  Also ported since: the AI skill pick 0x479f90 (`src/ai_skill.c`) and the script
     engine's I/O callees (`src/script_io.c`: gifts, voice pick, the logger 0x408b90, the BGM change 0x40a5d0).
     Deferred: the frame loaders 0x4184a0 / 0x4188b0 (they call 0x5b7310, an IDA __userpurge with an ebx argument —
     needs a register-call thunk in decomp2c; 5 such functions in the binary).
     **What a new recording would open up** (unreached + unported engine code, ~627 KB): the later chapters' story
     scripts (0x525e70 40 KB, 0x507e70, 0x51a1d0, 0x5402e0 … ~400 KB: the field trip, Stella …), the main menu's other
     screens (Status 0x592190 / 0x592850, Items 0x593f60, Abilities 0x596b50, Party Commands 0x598d00, the Options pages
     0x569220, the level-up screen 0x5915c0), monster / boss types rh-combat never meets (the integrator's handlers
     0x4539b0 / 0x456d30 / 0x452730 / 0x446900 …, 14 per-type inits 0x428c00 ..), a gamepad (0x468b10, 7 KB).
     Chain them with `record NEXT --from rh-combat` (rh-combat ends after a statue save).
  00000. (2026-10-17) the frame RENDER runs as port code (`src/render_draw.c`, 78 functions; 11 REPLACE hooks):
     pure ≡ hybrid in pixels on every presented frame (the render gate).  Every decomp2c module regenerated from a
     fresh IDA export (callers had lost their callee's `this`); its platform layer too (`src/render_platform.c`: every
     blit).  Next: the frame loaders 0x4184a0 / 0x4188b0 / 0x4178e0 / 0x499850 + the bitmap session 0x5b6xxx;  the
     title runs as port code too (`src/title_menu.c`); next on that side: the asset group loaders (0x56e190 0x57ca40
     0x5752e0 0x57b280: ~75 KB, boot-time); a targeted (sampled) shadow verify of the regenerated modules; `_DWORD` is unsigned in IDA but
     signed in decomp2c's prologue (check before relying on `>>` / `<` of a `_DWORD` load).
  0000. (2026-10-16 evening) the shots, the behaviour layer, the integrator's reached handlers + the item / skill
     effect + 34 chapter scripts run as port code; the hybrid gate holds at every 600-frame sample (wine).  (Was
     open: chapter script `0x49f7c0` (errands f7835) — CLOSED 2026-10-23, see item 0000000.)  Next: the integrator's
     UNREACHED handlers (`0x4539b0` 8.7 KB, `0x456d30`, `0x452730`, … — a recording that reaches them first); the
     chapter scripts' I/O-touching callees (`0x41a430` `0x41c390` `0x408730` `0x40a5d0` `0x41d4a0` …, still `RC_`); the scene loop's `0x561560` /
     `0x561d20` (file I/O underneath: keep the overlay in mind);
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
     0x426db0 / 0x556380 + a few leaves 0x429d00 0x429d50 0x42b550.. 0x42da20 0x42ea50) — rh-combat reaches three
     (0x42e290 0x42c970 0x42fbf0, ported); the other 14 are monster / boss / ally types no recording meets yet (the
     coverage run says which); the palette read `0x4178e0` + frames `0x418470` / `0x417c40`
     (DirectDraw), `0x56dc20` (Now Loading + Flip), the joysticks `0x5ba120` / `0x5ba290` (platform). The SCENE LOOP `0x59f2c0` above it runs as port code too
     (`src/scene_loop.c`, quirk #141; its save-load half: rh-combat reaches one apply `0x561d20`; the title's Continue
     enters the corpus with the first chained `--from` scenario). Its slots: the demo scene blocks `0x4c5af0` / `0x52fd80` / `0x5a3870`, the file stream `0x5b6060`..
     (platform); the save writer / reader / apply are port code (`src/save_game.c`, quirks #171 / #172). The party API (`src/party_member.c`, quirk
     #142) is ported; its slots: the level-up stats `0x409150`, the cue `0x55fdb0`, the level / status sync `0x41d780`, the start gear `0x4e6520` / `0x4e61d0`.
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
