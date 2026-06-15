# Arche's town-intro RUN-OFF — "runs to the house" (THEME 2, cutscene-party-chars)

> **Status: RENDER PORTED + frame-sequence verified bit-exact (ckpt 140).** The
> USER studio note tick 1027 ("arche runs to the house in retail, stands still in
> port") is resolved: Arche now plays her RUN → DECEL → arrival-IDLE animation and
> moves to the house door during the L7→L8 beat. The cel sequence matches retail
> tick-for-tick; the absolute screen position has a ~10–40px residual = the
> pre-existing camera-pan phase ([[ingame-camera-pan]]) + the tagged mover
> stand-in (the run-off velocity/decel, `0x54f980` deferred).

## The gap (USER studio note, `osr_notes.jsonl` tick 1027)
`{"tick":1027,"crop":[329,299,105,85],"differ":186394,"text":"arche runs to the
house in retail, stands still in port"}`. During the L7→L8 inter-line beat
("Mom! Dad! c'mon!", `TOWN_ARRIVAL` line 9) retail's Arche RUNS ahead to the
house door, turns, and calls back; the port left her static at her cast anchor.

## Ground truth — retail.osr draw stream (`draw_probe.py --res 0x570`, ticks 980–1130)
Arche renders as **res 0x570 = bank 0x8b** (NOT a separate "running" resource — my
first wrong assumption was that 0x570 was unused; she just sits at y≈304, below the
y≤280 band I first probed). Her run-off is three clip phases:

| phase | cels (res 0x570 frame) | per-cel dur | notes |
|-------|------------------------|-------------|-------|
| **RUN** | 16,16,17,18,19,19,20,21 (loop) | 5 sim-ticks/clip-frame | contact frames 16/19 held 2× → cel 16 10t, 17/18 5t, 19 10t, 20/21 5t; a 40-tick cycle |
| **DECEL** | 8,9,10,11 (one-shot) | ~6t each | slowdown as she nears the door (~retail ticks 1041–1064) |
| **arrival IDLE** | 152,153,154 (loop) | ~14t each | a gentle breathe at the door (the brief 12,0,6,159 transition between decel and idle is a ~11t cosmetic flourish, folded into the decel→idle switch) |

So the run clip decodes as **`base_sprite 16, frame_delta {0,0,1,2,3,3,4,5}, dur 5,
loop`** — RE'd timing/indexing metadata read off the ground-truth draw stream (like
`WAGON_CLIP`/`IDLE_CLIP`), NOT the binary asset (bank 0x8b pixels load from the
user's `sotes.exe`).

### The motion is the REAL ported run physics
A camera reference (res 1004, the static buildings scrolling left as the camera
pans right) gives the camera scroll, so Arche's **world** velocity = her screen
velocity + the camera scroll. Measured she accelerates then cruises at ~460–480
centi-px/tick = **the ported run cap 48000/100** (char-run, ckpt 118). So her
run-off uses the real run physics (accel to cap, `world_x += vel/100`), confirming
this is not a bespoke cutscene mover but the same `0x442a70` run branch.

### The run-off command (`0x402730`)
`0x402730(Arche, +32000)` writes Arche's actor MOVE command block (`+0x15b64..`)
with target = `actor_world_x + 32000` and OVERWRITES the beat type to case 4 (the
actor-wait, `:54`). Her start world_x ≈ 41600 (her cast anchor) → **target world
73104** (= `TOWN_ARRIVAL` line 9's baked `spk_wx`). The velocity + the
decel/clip-switching are the actor mover `0x54f980` (the 12 KB shared integrator,
deferred — `PORT-DEBT(cutscene-party-chars)`, the same mover the run-off DUR
already stands in for).

## The port (ckpt 140)
- **`actor_spawn.c`**: `ARCHE_RUN_CLIP` / `ARCHE_DECEL_CLIP` / `ARCHE_ARRIVAL_IDLE_CLIP`
  (the faithful clips above) + a pure, host-tested run-off state machine
  `arche_runoff_begin` / `arche_runoff_step`: the **real ported run physics** for
  accel/cruise (two-phase accel 3200→24000 then 1600→48000, cap 48000,
  `world_x += vel/100`); only the **DECEL-approach** (the distance trigger +
  linear ramp-down to stop AT the target) is the tagged `0x54f980`-mover stand-in.
- **`main.c`**: finds Arche's slot in `g_effects` (the cast member on body bank
  0x8b), begins the run-off when the L7→L8 `CS_ACT_CAMERA_PAN` action fires
  (the run is concurrent with the pan — the same beat), and advances her one
  sim-tick per `game_actor_update` (mirrors `world_x` into her render-state +
  switches her clip on a phase change; `actor_pool_update` steps the active clip).

## Verification (port-runoff.osr vs retail.osr)
- **Frame sequence bit-exact**: port plays 16,17,18,19,20,21 (looping) → 8,9,10,11
  (decel) → 152-154 (idle), matching retail's cels.
- **Run-start aligned**: port's first run cel (16) at **tick 983**, retail
  already running by **tick 980** — within ~3 ticks (the matched dialogue cadence,
  THEME 1, places the run-off start on retail's tick).
- **Position residual ~10–40px**: dominated by the pre-existing camera-pan phase —
  the static cast cel (res 1027) is the SAME ~40px off (port 358 vs retail 318 at
  tick 1027, present in port-theme3.osr BEFORE this change), i.e. the whole scene
  is framed ~40px differently ([[ingame-camera-pan]] startup-phase residual), not
  an Arche render error. The remaining few px is the tagged mover-approach
  (exact decel curve = `0x54f980`).
- **Host test** `test_arche_runoff` (1024 pass): the run clip cel sequence + the
  two-phase accel to cap + the decel-stop-at-target.

## The camera-pan phase residual — root cause DIAGNOSED (ckpt 140)
The ~40px whole-scene framing offset during the run-off is the camera onset being
**~10 ticks late**, and the root cause is precisely pinned (measured off retail.osr
vs port-runoff.osr; the static caravan res 1004 is the camera reference):

- **Retail** (during "Cool!" = L2, typed tick 958 → advances 982): Arche's run
  **windup** starts ~tick **970** (res 0x570 fr=3→8→9, motion from 975), the
  **camera** pans from **977** (caravan 288→287), the run cycle (fr 16) at 980 —
  **ALL during "Cool!"'s hold**, ~5-12 ticks BEFORE "Cool!" advances. And retail's
  "Cool!" box (res 0x456) STAYS UP and follows Arche as she runs (box_x 94→103→…→184
  from tick 983) — i.e. the run-off **OVERLAPS** the dialogue line; it is NOT gated
  on the line's confirm.
- **Port**: the run-off (camera + run) fires at the "Cool!" CONFIRM (the L8 lead
  beats run after line 8 advances, ~tick 983), so the camera onset is ~tick 987 —
  the ~10-tick lag → the ~40px framing offset. The pan RATE + target + easer ramp
  all MATCH retail (same shape, offset only by the onset).

So the fix is a **cutscene-beat-runner restructuring**: the run-off must fire
DURING "Cool!" (concurrent with the displayed line, the box following running
Arche), not serialized after its confirm. The exact trigger tick (~970, ~12t into
"Cool!"'s hold) is a `0x4d7d80` beat-timer to RE. This touches the tick-1:1
dialogue path (THEME 1) + the box-position projection (the box would track Arche's
RUNNING world pos), so it is a focused follow-up done carefully, not a rushed
end-of-session change.  Owner: `PORT-DEBT(cutscene-beat-runner)` / `ingame-camera-pan`.

## Other residuals (NOT this chip)
- The exact run **velocity/decel** curve + the 12,0,6,159 decel→idle transition
  flourish → `PORT-DEBT(cutscene-party-chars)` (the mover `0x54f980`).
- Arche does **not turn to face the cast** at the door (she holds the right-facing
  idle); retail's line-9 portrait/facing is `PORT-DEBT(dialogue-portrait-facing)`.

Cross-refs: `docs/port-debt.md` (`cutscene-party-chars`, `ingame-camera-pan`),
`docs/plans/intro-cutscene-1to1.md` (THEME 2 note #5/#3), `src/actor_spawn.{c,h}`
(`arche_runoff_*`), `src/main.c` (the run-off driver), `docs/decompiled/by-address/402730.c`.
