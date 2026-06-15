# Arche's town-intro RUN-OFF — "runs to the house" (THEME 2, cutscene-party-chars)

> **Status: RENDER PORTED + the CAMERA-PAN OVERLAP fixed → camera + Arche position
> now BIT-EXACT (ckpt 141).** The USER studio note tick 1027 ("arche runs to the
> house in retail, stands still in port") is resolved (ckpt 140), AND the ckpt-140
> follow-up "the run-off camera-pan is ~10t late (~40px framing offset)" is now
> CLOSED (ckpt 141, "The camera-pan phase residual" section below): the run-off
> fires CONCURRENT with "Cool!" (on its beat completion ~tick 972, not its body-clear
> 982), so off port-runoff2.osr vs retail.osr the **static caravan (res 1004) matches
> retail tick-for-tick** (the camera framing offset is 0px) and **Arche's screen-x +
> run cels match tick-for-tick from 975** (the char-run accel matches retail's windup
> drift to ~1px; the run cycle fr16 lands at tick 980 == retail). Residuals (both
> documented debts, NOT the camera/run): the ~5-tick windup LEAN cels (retail fr 3/8/9
> vs the port's idle = [[cutscene-party-chars]] emote) + the box-close SLIDE
> ([[dialogue-runoff-box-slide]]).

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

## The camera-pan phase residual — DIAGNOSED (ckpt 140) → FIXED (ckpt 141)
The ~40px whole-scene framing offset during the run-off was the camera onset being
late, because the port SERIALIZED the run-off after "Cool!"'s confirm.  RE'd + fixed
(measured off retail.osr vs port-runoff2.osr; the static caravan res 1004 is the
camera reference, dialogue body off `dialogue_timeline.py`):

**The mechanism (the key correction over the ckpt-140 diagnosis).**  "Cool!" (L7,
TOWN_ARRIVAL line 9, full at tick 958) holds, and TWO independent things happen:
- Its dialogue beat COMPLETES at ~tick **972** (full+14) — the script then issues the
  camera command + `0x402730` run-off.  This is PINNED by the camera onset: the camera
  easer `0x43d1d0` accelerates from rest (`v += 10`/tick, **5 ticks to the first
  pixel**), and retail's caravan moves its first pixel at tick **977** ⇒ the command
  fired at 977−5 = **972**.  (The ckpt-140 diagnosis read the onset as a 1-tick latency
  → mis-inferred the fire at 976; it is 5t of accel-from-rest, fire at 972.)
- Its box BODY stays up until tick **982** (full+24) — the box has its own ~24t body
  auto-hold, INDEPENDENT of the beat.  So the run-off OVERLAPS the line's last ~10t,
  and the box keeps showing "Cool!" while the camera + the run play behind it.

Arche: the `0x402730` run command at 972; she accelerates from rest immediately but
plays a forward-lean windup (res 0x570 fr 3/8/9, ticks ~970-979) and only breaks into
her run cycle (fr 16) at tick **980**.

**The port (ckpt 141), all three matched:**
- The matched nav presses "Cool!"'s advance at tick **972** (= advance_tick 982 − 10,
  the run-off lead; `dialogue_timeline.py` `runoff_leads`).  The cutscene fires the L8
  lead beats (camera pan + Arche run) on that advance, but LINGERS the box showing
  "Cool!" for `ARRIVAL_RUNOFF_BOX_HOLD`=11 ticks (through 982/983, its full+24 body
  hold), so THEME 1 is preserved.
- `arche_runoff` accelerates from rest at 972 (the real char-run accel, which matches
  retail's windup drift to ~1px) but HOLDS the run cel idle for `ARCHE_RUNOFF_WINDUP_
  TICKS`=7, so the run cycle (fr 16) lands at tick 980 == retail.
- `ARRIVAL_L8_RUNOFF` bumped 97→108 so L8's first glyph still lands ~1150 (== retail).

**Verified bit-exact off port-runoff2.osr vs retail.osr:** caravan res 1004 position
identical every tick 977-1000 (framing offset 0px); Arche res 0x570 screen-x + run
cels identical every tick 975-1010 (fr16@980, fr17@990, fr18@995, fr19@1000); the
whole arrival→house dialogue chain unchanged (L0-L11 within ~1t).  Recon montage on
the feed (tick 985): the scene is near-zero diff; the only residuals are the two debts
below.  1024 host pass.  Owner CLOSED: `ingame-camera-pan` (the run-off onset).

## Other residuals (NOT this chip)
- The ~5-tick windup LEAN cels (retail fr 3/8/9 ticks ~975-979 vs the port's idle):
  the cast emote, `PORT-DEBT(cutscene-party-chars)`.  Her POSITION matches (the run
  physics) — only the lean cels are unported.
- The box-close SLIDE: after the body clears (982), retail's empty box bubble (tail
  res 0x456) SLIDES right ~7px/tick off-screen, gone tick 1004 — the port shrinks it
  in place instead.  `PORT-DEBT(dialogue-runoff-box-slide)`.
- The exact run **velocity/decel** curve + the 12,0,6,159 decel→idle transition
  flourish → `PORT-DEBT(cutscene-party-chars)` (the mover `0x54f980`).
- Arche does **not turn to face the cast** at the door (she holds the right-facing
  idle); retail's line-9 portrait/facing is `PORT-DEBT(dialogue-portrait-facing)`.

Cross-refs: `docs/port-debt.md` (`cutscene-party-chars`, `ingame-camera-pan`),
`docs/plans/intro-cutscene-1to1.md` (THEME 2 note #5/#3), `src/actor_spawn.{c,h}`
(`arche_runoff_*`), `src/main.c` (the run-off driver), `docs/decompiled/by-address/402730.c`.
