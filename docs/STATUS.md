# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
█████░░░░░░░░░░░░░░░  26.8% touched   (15.8% host-tested, 24.7% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   250 | ported + module covered by the host unit suite       |
| ported      |   173 | reimplemented in src/, no host test for that module  |
| **touched** | **423** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1401 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1532** below
`0x5bdab0`. The other **292** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1824** non-thunk
functions (of 1824 incl. thunks).

Code-byte coverage (24.7% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

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
  Hybrid gate: with all of the above REPLACING retail (136 hooks) (the whole character tick, driver → tile
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
