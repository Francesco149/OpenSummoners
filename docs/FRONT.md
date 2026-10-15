<!--
  The ONE hand-edited status block.  tools/gen_port_ledger.py injects everything
  between the FRONT:BEGIN/END markers verbatim into docs/STATUS.md's "Current front"
  section, so STATUS can never drift from reality.  Update THIS when the active front
  moves; keep it a 60-second read.  Everything else in STATUS is derived from code.
  Port state / next-move detail belongs here or in HANDOFF.md, NOT in engine-quirks.md.
  Rolling per-checkpoint narrative → PROGRESS.md; deep RE → findings/.  Keep this SHORT.
-->
<!-- FRONT:BEGIN -->
- **2026-10-14 — rhost is THE porting loop; the MOVEMENT stack is ported and verified inside retail.**
  USER decisions (audit §7): deterministic cadence = rhost 1 update/flip; EN-old first (SE = a later
  delta layer); Frida retail drive RETIRED (legacy, don't extend). Port boot "Now Loading" frames now
  DEFAULT (`--no-boot-loading` = legacy axis; old port-timed traces keep working).
  Corpus: 7 scenarios incl. the USER's `rh-my-play` (15.7k flips: intro → house errands → town → shop;
  10 room loads). `rhost.py verify` (shadow, per call; full-state where a function writes widely):
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
  | the PARTY MEMBER create / attach (next free char, init at the gate's feet, the slot's stat block, pacing, brain, respawn) + equipment recolour, skill refresh, skill usable, member skill find, slot code + the sprite-slot clone / entry free | `0x58e170` / `0x5b4530` `0x5b4a10` `0x406ec0` `0x407320` `0x4071f0` `0x4094d0` `0x4179b0` `0x417b50` | 16 / 16 / 16 / 16 / 1,536 / – / – / 564 / 1,182 | 0 |
  | menu / dialog controller geometry free (retail heap; `menu_list.c` keeps the port-heap twin) | `0x40e0c0` | 542 | 0 |
  | the SCENE LOOP (the session: the area / fill / keep tables, the party room, new game / save load, the room loop of load_room with defeat / retry / menu / map-change handling) | `0x59f2c0` | REPLACE only: the whole session in 3 scenarios | – |
  | its helpers: scene object, event flags, area flag / find / cross-reference, slot reset, inputs, keep lists, retry / heal / gold, room loop new / free, room-end cleanup, frame cache | `0x41a7f0` `0x4c57f0` `0x4c5830` `0x560e90` `0x561c90` `0x585000` `0x560e60` `0x5611d0` `0x5851e0` `0x5a3de0` `0x5a3e20` `0x4cc5a0` `0x5a3c40` `0x585f00` `0x5a3ff0` (REPLACE) `0x562a70` … (34) | 1,251 xref / 29 / 24 / 16 … (21 of 34 reached) | 0 |
  | the room → scene snapshot (what a save stores: room fields, play time, items, members, fill table, flags) | `0x55f790` | 11 | 0 |
  | the PARTY API (the story setups by start area, start members from the config, levels, slot init, skills, starting items, leader, join / leave / dismiss, equip + stat refresh, class fit, item add) | `0x4c5350` `0x4e59a0` `0x408dc0` `0x4c5e00` `0x4c6160` `0x4c6990` `0x4c6830` `0x413b20` `0x408d40` … (25) | 6 / 6 / 6 / 3 / 3 / 3 / 3 / 6 / 3 … (11 of 25 reached: the new-game path) | 0 |
  | the member stats recompute (base + items, with an equip-preview item) | `0x4066d0` | 1 | 0 |
  | the scene start (next area / retry point, actor states, room snapshot) + a part's facing | `0x59cfc0` / `0x4842c0` | 8 / 8 | 0 |
  Hybrid gate: with all of the above REPLACING retail (295 hooks) (the whole character tick, driver → tile
  movers, + the room loop's passes and per-tick calls, runs as port code), every corpus scenario ends byte-identical.
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
  000. `load_room 0x586010` RUNS AS PORT CODE (`src/room_load.c`, REPLACE-verified; quirk #138), the world it makes /
     frees too (`src/world_alloc.c`), the map decode (`map_decode_room`), the population + device / structure
     inits, and now the CHARACTER INIT `0x41f200` (`src/char_init.c` + 42 helpers `src/char_def.c`; quirk #139).
     The party member create `0x58e170` / attach `0x5b4530` too (`src/party_member.c`; quirk #140).
     Slots left under it: the 18 per-type inits `0x428c00` `0x429060` `0x429db0` `0x42a500` `0x42a9a0` `0x42b9a0`
     `0x42c130` `0x42c510` `0x42c970` `0x42cce0` `0x42d3c0` `0x42da50` `0x42e290` `0x42eb20` `0x42f840` `0x42fbf0`
     `0x42ff00` `0x430400` (monsters / bosses / party allies; ~30 KB, built from the char_def helpers + 0x41e020 /
     0x426db0 / 0x556380 + a few leaves 0x429d00 0x429d50 0x42b550.. 0x42da20 0x42ea50) — **the corpus reaches NONE
     of them** (no dungeon / monster room: 17 townsfolk / party codes only), so porting them needs a monster-room
     scenario first (USER `rhost.py record`); the palette read `0x4178e0` + frames `0x418470` / `0x417c40`
     (DirectDraw), `0x56dc20` (Now Loading + Flip), `0x41a890`, the room-start script `0x40c380`, the
     joysticks `0x5ba120` / `0x5ba290` (platform). The SCENE LOOP `0x59f2c0` above it runs as port code too
     (`src/scene_loop.c`, quirk #141; its save-load half is unexercised: a save scenario is needed). Its slots: the
     save writer `0x41a890`,
     the save apply `0x561d20`, the demo scene `0x41b670` (+ `0x4c5af0` / `0x52fd80` / `0x5a3870`), the menu
     `0x5a00c0` (13.7 KB), the file reader `0x5b6060`.. (platform). The party API (`src/party_member.c`, quirk
     #142) is ported; its slots: the level-up stats `0x409150`, the cue `0x55fdb0`, the level / status sync `0x41d780`, the start gear `0x4e6520` / `0x4e61d0`. Next: `0x41a890`
     (the save writer, every room start).
  00. the ROOM LOOP `0x58f360` RUNS AS PORT CODE (`room_loop_run`, REPLACE-verified; Flip / the camera
     easer go through rhost's own stubs). Left under it as slots: the pickups `0x59ca90` (+ the world
     message log world+4 / +0x1024, "You got %s!"), the warp countdown `0x59c750`, the leader swap
     `0x59aa40`, the scene runner `0x59ad60` / start `0x59cfc0` (+ `0x41a7f0`), `0x5908a0`, the camera
     snap `0x58e6a0`, the screen effects `0x499ab0`, the menus (`0x59d540` / `0x591120` / `0x590480` /
     `0x590f20` / `0x590ca0`), the shots `0x590230` / band 96 `0x46da00`, the render `0x48c150` + Flip
     (platform). Next: `load_room 0x586010` (the room setup around the loop) and the scene loop
     `0x59f2c0`, so the port binary can run free roam on the retail-layout world (item 2).
     Slots left in the passes: the script engine `0x40c380` (5 KB), the key use `0x54ed80`, the input
     re-poll `0x40fe00`, the shot emitters `0x40ac90` / `0x55cfc0`.
  0. the Arche handlers `0x457ff0` (4.6 KB),
     `0x45a300` (14 KB), `0x461790` (19 KB) and the apply-pass `0x487dc0` (5 KB) / `0x4878b0`; the DirectSound
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
     the type handlers behind the integrator (Arche: `0x457ff0`, `0x45a300`, `0x45e830`).
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
<!-- FRONT:END -->
