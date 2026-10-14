# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
████░░░░░░░░░░░░░░░░  18.7% touched   (15.8% host-tested, 20.9% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   250 | ported + module covered by the host unit suite       |
| ported      |    50 | reimplemented in src/, no host test for that module  |
| **touched** | **300** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1524 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1532** below
`0x5bdab0`. The other **292** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1824** non-thunk
functions (of 1824 incl. thunks).

Code-byte coverage (20.9% of engine-proper bytes) is the truer progress
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
  Hybrid gate: with all of the above REPLACING retail (the whole character tick, driver → tile
  movers, runs as port code), every corpus scenario ends byte-identical.
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
  1. the band updaters under the tick driver `0x46cd70` (`src/world_tick.c`): ALL ported except shots `0x4710c0` (20 KB)
     + the 96-band `0x46da00` never run in the corpus (no combat yet). Arms ported but NOT yet exercised by
     the corpus (need recordings that reach them): `src/entity_fx.c` water splashes + records
     100150..100157; `src/actor_update.c` codes 70100/70120/70216/70230/70240/70360/70375/70390/70480/
     70520/70521/70840/70850/100138/100142..100145 (+ the slot-only codes); `src/particle_tick.c` every
     code but 100077/100078/100080/100081/100082/100100/100104 (incl. the x87 orbit 100131, which needs
     the port binary on MSVC's 53-bit FPU precision); `src/char_behave.c` the cutscene random walk
     (+0x15ba8) and the monster / party modes. The ~40 behaviour helpers behind `0x47b990`'s slots
     (`0x482ac0`..`0x485990`, `0x47ecd0`, `0x47e330`) are the next layer down. The actor behaviours behind
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
  4. port-side DIK input (`--rh-trace` via the producer `0x46a880`, which is itself a shadow target).
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
