# Arche's house TURN — the listen→face-father emote (USER studio notes #3-5)

**Status: PORTED + drawcall-faithful + TICK-1:1 (ckpt 151).** During the new-house
dialogue Arche turns from her arrival-listening idle to face her father just before
she says "I will, I promise!".  The port's static HOUSE_CAST Arche used to hold the
arrival-idle (cels 152-155) the whole house scene; she now plays the turn — and as
of ckpt 151 it fires at retail's exact tick (158@1579 / 7@1583 / 0@1587, verified off
`port-houseturn.osr` vs `retail.osr`).

**ckpt 151 update — the turn is now the BLOCKING beat retail uses, not fire-and-forget.**
ckpt 146 emitted the turn fire-and-forget on the L5→L6 advance and relied on the matched
nav to place L6, which left the turn ~7t LATE (the "residual" below).  The ckpt-150
investigation then DISPROVED this finding's old "auto-aligns once the cadence is tick-1:1"
claim: a port-vs-retail comparison off `port-dash.osr` (a fully matched-cadence nav) showed
the house DIALOGUE was already tick-1:1 (`dialogue_timeline` within ±1t) yet the turn was
STILL 7t late — so the lag was NOT a cadence-phase debt, it was the missing BLOCKING beat.
ckpt 151 ports it faithfully (see "The port", below): the turn is house L6's blocking lead
beat (`CS_BEAT_ACTOR_TURN`), keyed to L5's confirm, and the nav presses L5's confirm at
retail's real confirm tick (1579, the turn onset).  Turn 158@1579 / 7@1583 / 0@1587 ==
retail; house L0-L7 tick-1:1; cover-start 1669 == retail; no arrival regression.

## The trigger (RE'd, not measured)

The town-intro script `0x4d7d80` (house case `0x334c8`) issues a fire-and-forget
actor emote **between L5 and L6**:

```
:1161  L5  "...Well, Arche, I'll be countin' on you."   (Father, 0x5f5e1d3)
:1168  uVar10 = 1;
:1169  uVar3 = FUN_00556eb0(0x5f5e165);                 // Arche's actor handle
:1170  FUN_00401e60(uVar3, uVar10);                      // EMOTE: turn Arche to dir 1
:1182  L6  "I will, I promise!"                          (Arche, 0x5f5e165)
```

`FUN_00401e60(actor, param)` finds/creates the actor's command slot and writes
**command kind 2** (`*(slot+0x15b64)=2`) with **param** (`*(slot+0x15b74)=param_2`,
here `1` = the target facing), then sets the beat type `in_ECX[8]=4` (the case-4
actor-wait, the same beat class the L7→L8 run-off `0x402730` uses).  Command kind 2
is consumed by `0x43e5b0` case 2 = "turn to face dir N" (its sibling setter
`0x406210` is literally *GetHeadLeftRight*).  So in retail the emote is a **blocking
beat**: L5 completes → Arche turns (≈8 ticks) → L6's box opens.

The live actor-turn FSM `0x43e5b0` is unported (PORT-DEBT(cutscene-party-chars) —
the cutscene cast still renders as captured static-cast EFFECT actors), so the port
reproduces the *animation* from the retail draw stream, the same stand-in pattern as
`ARCHE_RUN_CLIP` (ckpt 140).

## The animation (ground truth — retail.osr res 0x570)

Arche (res `0x570`/1392) is **static at screen (354,336)** through the entire house;
only her cel changes.  `draw_probe.py --res 0x570` over the L5→L6 ticks:

| ticks      | cel | note                                  |
|------------|-----|---------------------------------------|
| …–1578     | 152-155 | arrival-listening idle loop       |
| 1579–1582  | **158** | turn windup (4t)                  |
| 1583–1586  | **7**   | turn settle (4t)                  |
| 1587+      | **0,1,2** | the new STANDING idle (base-0 breathe, 14t/cel) |

So the turn is `158(4t) → 7(4t)` then a base-0 standing-idle breathe `0/1/2` — a
different idle pose set than the arrival idle (base 152).

## The port (ckpt 151 — the BLOCKING lead beat)

- `actor_spawn.c`: `ARCHE_HOUSE_TURN_CLIP` (base 158, delta `{0,-151}` = cels
  {158,7}, dur 4, **one-shot**) + `ARCHE_HOUSE_STAND_IDLE_CLIP` (base 0, delta
  `{0,1,2,1}`, dur 14, loop); accessors `arche_house_turn_clip()` /
  `arche_house_turn_idle_clip()`.  (Unchanged since ckpt 146.)
- `cutscene.{c,h}`: a `CS_BEAT_ACTOR_TURN` beat type (issues `CS_ACT_ACTOR_TURN(param=dir)`,
  holds `dur` ticks) + house **L6's lead beat** `HOUSE_L6_LEAD` (`{CS_BEAT_ACTOR_TURN,
  dir=1, dur=HOUSE_TURN_DUR=8}`).  The L5→L6 advance begins the turn beat (the existing
  lead-gate in `cutscene_step`): L6 opens only AFTER it completes, so the turn is keyed to
  L5's confirm — exactly retail's `0x401e60` setting `in_ECX[8]=4` (the actor-WAIT beat the
  thunk `0x439680` pumps).  The lead's `box_hold = HOUSE_TURN_DUR` keeps L5's box UP (full
  text, opaque portrait) through the turn — retail's actor-wait beat does not touch the box —
  then L5 SHRINK-closes as L6 reopens (the speaker-change overlap, quirk #107; the box_hold
  path gates slide+portrait-dissolve to the run-off CAMERA_PAN lead, NOT the turn).  The old
  `cutscene_room.turn_after_line` fire-and-forget field is REMOVED (superseded).
- `main.c`: the `CS_ACT_ACTOR_TURN` drain (unchanged) swaps the room-cast Arche (`HOUSE_CAST[0]`,
  guarded on bank `0x8b`) to the turn clip + sets `g_arche_house_turning`; after
  `actor_pool_update(&g_room_cast)`, when the one-shot finishes (`rs->done`) it swaps her to
  the standing idle.  Reset on `reload_room_backdrop`.
- the NAV: `dialogue_timeline.py NAV … "7:10,15:15,17:10"` — the house L5 (global L15) confirm
  is pressed at its real tick **1579** (the turn onset = advance_tick 1594 − 15), so the turn
  beat fires there; L17 (house exit) at 1668 (the cover confirm).  `runs/cutscene-verify/nav-house-turn.jsonl`.

## Verified (ckpt 151)

Fresh `port-houseturn.osr` vs `retail.osr`:
- **the turn** (`draw_probe --res 0x570`): **158@1579-1582 → 7@1583-1586 → 0@1587** at dst
  (354,336) — cels, durations, position, AND absolute tick **identical to retail's turn**
  (the ~7t lag is GONE).
- **the house dialogue** (`dialogue_timeline`): L0-L4/L6/L7 tick-1:1 (±1t), L5 adv 1598 (retail
  1594, +4t — the box-overlap close), L6 start 1608 (retail 1605), L7 start 1644 (==).
- **the cover-start** (`draw_probe --res 1112`): first fade cells at tick **1669** == retail.
- **no arrival regression**: the run-off shares the box_hold path (gated to CAMERA_PAN) — the
  arrival L0-L9 + the L8 run-off (start 1151) are unchanged.

## Residual (minor) — the L5 box-overlap close

Retail's L5 box closes at 1594 (when L6 passes half); the port's at 1598 (+4t) and L6's box
opens at 1608 (retail 1605, +3t) — a box-OPEN-rate detail of `ARM_OPEN` vs retail's overlap,
the same box-close-animation family as PORT-DEBT(dialogue-runoff-box-slide).  The turn itself
+ the dialogue advances are tick-1:1; this is the closing-box shrink curve, not the turn.

## Follow-up — the ARRIVAL's Father turn (same mechanism, unmodeled)

The arrival case (`0x4d7d80:181-182`, case `0x334be`) issues the SAME emote
`0x401e60(Father 0x5f5e1d3, 1)` between arrival L5 ("I wanna see it!") and L6
("Mm-hmm. It's just down the next street"), so it is also a blocking actor-turn beat.
The port does NOT model it (the arrival is "tick-1:1" because the nav absorbs the gap,
same as the house was pre-ckpt-151).  Whether it's visible enough to need modeling is
UNVERIFIED (the arrival cast is moving/camera-panning, unlike the static house cast) —
if so, it's a clean consistent follow-up: an arrival L6 lead beat with `CS_BEAT_ACTOR_TURN`
on the Father (bank 0xe3) + the Father turn clip, plus the matched-nav L5 confirm lead.
Probe retail.osr for the Father's res cels around the arrival L5→L6 ticks (~890-910) first.

## Tests
`tests/test_cutscene.c::test_cutscene_house_turn` (the L5→L6 advance arms the BLOCKING turn
beat — in_beats, CS_ACT_ACTOR_TURN(1) — and L6 opens only after it; the turn fires nowhere
else) + `tests/test_actor_spawn.c::test_arche_house_turn_clip` (the clip cels 158→7→done, the
base-0 idle).  1045 host pass.

USER-VERIFY: `osr_view.exe C:\oss-osr\port-houseturn.osr C:\oss-osr\retail.osr` (the studio
shortcut is loaded with this pair) — scrub the house (ticks ~1579-1610): Arche turns to face
her father just before "I will, I promise!", now AT retail's tick, with her father's text
bubble still up through the turn.
