# Arche's house TURN — the listen→face-father emote (USER studio notes #3-5)

**Status: PORTED + drawcall-faithful (ckpt 146, commit `cfc6a96`).** During the
new-house dialogue Arche turns from her arrival-listening idle to face her father
just before she says "I will, I promise!".  The port's static HOUSE_CAST Arche used
to hold the arrival-idle (cels 152-155) the whole house scene; she now plays the
turn.

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

## The port (commit `cfc6a96`)

- `actor_spawn.c`: `ARCHE_HOUSE_TURN_CLIP` (base 158, delta `{0,-151}` = cels
  {158,7}, dur 4, **one-shot**) + `ARCHE_HOUSE_STAND_IDLE_CLIP` (base 0, delta
  `{0,1,2,1}`, dur 14, loop); accessors `arche_house_turn_clip()` /
  `arche_house_turn_idle_clip()`.
- `cutscene.{c,h}`: a new `CS_ACT_ACTOR_TURN` action + a `cutscene_room.turn_after_line`
  field (house = 5, arrival/errands = -1).  `cutscene_step` emits the action on the
  advance PAST `turn_after_line` (after the lead gate, before the speaker-change
  box handling).  **Fire-and-forget** — it does NOT gate the next line (the house
  cadence already places L6), unlike retail's blocking beat (see the offset note).
- `main.c`: the `CS_ACT_ACTOR_TURN` drain swaps the room-cast Arche (`HOUSE_CAST[0]`,
  guarded on bank `0x8b`) to the turn clip + sets `g_arche_house_turning`; after
  `actor_pool_update(&g_room_cast)`, when the one-shot finishes (`rs->done`) it
  swaps her to the standing idle.  Reset on `reload_room_backdrop`.

## Verified

Fresh `port-turn.osr` vs `retail.osr` (`draw_probe --res 0x570`): the port plays
**158@1586(4t) → 7@1590(4t) → idle 0@1594 → 1@1608** at dst (354,336) — cels,
durations, and position **identical to retail's turn**.

## Residual — the ~7t absolute-tick lag (NOT this chip)

The port's turn starts ~7 ticks LATER than retail (port 158@1586 vs retail 158@1579).
This is the **documented house-dialogue-cadence phase lag** (FRONT ckpt 145's
"~13t cover-START phase offset"), not a turn bug:

- Retail front-loads the turn — it's a *blocking* beat, so L5 closes, Arche turns
  (8t), THEN L6 opens.
- The port's turn is *concurrent* and the house nav is tuned so L6's box **opens**
  at retail's tick (dialogue_timeline within ~1t).  Because the port does not model
  the 8t blocking gap, the nav holds L5 ~6-7t longer to land L6 — so the L5→L6
  advance (which fires the turn) is ~7t late.

The turn is keyed to the advance, so it **auto-aligns** once the house cadence is
made tick-1:1 (the separate phase-pillar follow-up).  Making the turn a blocking
beat now would push L6 + every later house line 8t late unless the nav is rebuilt
(the cadence work), so the concurrent emit is the correct scope here.

## Tests
`tests/test_cutscene.c::test_cutscene_house_turn` (the action fires on L5→L6 and
nowhere else) + `tests/test_actor_spawn.c::test_arche_house_turn_clip` (the clip
cels 158→7→done, the base-0 idle).  1030 host pass.

USER-VERIFY: `osr_view.exe C:\oss-osr\port-turn.osr C:\oss-osr\retail.osr` (the
studio shortcut is loaded with this pair) — scrub the house (port ticks ~1586-1607):
Arche turns to face her father just before "I will, I promise!".
