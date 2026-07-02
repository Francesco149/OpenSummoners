# Dialogue "runs early" — the port-stairs vs retail-stairs divergence (ckpt 176)

USER studio mark **t1197** (`osr_notes.jsonl`): *"port skipping dialogue early"* — at
sim-tick 1197 retail's box shows the Mother's "wait up for your poor, old, slow parents"
line; the port box is already empty.  FRONT framed it as an **advance-gate** bug ("retail
gates the confirm on text-reveal completion / a per-line min-hold the port lacks").

**That hypothesis is DISPROVEN.  The advance PRESSES already match retail.**  The mark is a
box-close **render linger**, and the residual chain-early is a separate **beat-duration**
gap.  Decomposed below off `port-stairs.osr` | `retail-stairs.osr` (the spam-confirm
re-drive `runs/sync/spam-confirm-nav.jsonl` = one confirm id 0x24 / 14 sim-ticks, 660..1976)
+ the port DLGT dump (env `OSS_DLG_TRACE=1`, per-tick box state at the `cutscene_step` site).

## Ground truth — the arrival advance ticks MATCH (dialogue_timeline)

| line (arrival) | port adv | retail adv | Δ | port hold | retail hold |
|---|---|---|---|---|---|
| L0..L8 | ~= | ~= | ≤±2 | 23-24 | 23-25 |
| **L9 (Mother "wait up")** | **1192→1200** (fixed) | **1200** | **0** (was −8) | 23 | 23 |

The port DLGT proves every confirm's disposition: each line = pop-in confirm EATEN (scale
<1000) → a SKIP confirm (reveal→full) → an ADVANCE confirm.  L9: reopen@1152, pop-in→1168,
slow natural reveal, **skip@1178, advance@1192** — the advance press is on the SAME confirm
(1192) as retail.  L0-L8 advance presses are already tick-1:1 (688,730,772,814,842,870,912,
954,1150).  So there is **no advance-gate divergence**; the FRONT's `0x48cf80`-family accept
gate is faithful (`FUN_0043ce50` state 1=typing→skip / state 2=waiting→advance == the port's
`dialogue_typing`→skip / `dialogue_awaiting_advance`→advance).

## Component 1 — the L9 box RENDER linger (FIXED, ckpt 176)

Both advance L9 at 1192, but retail draws L9's body through tick **1200** (hold 23t) while
the port (`ARRIVAL exit_box_hold = 0`) cleared it at 1192 (hold 15t) — the box vanished 8t
early = the t1197 mark.  **Fix:** `ARRIVAL_EXIT_BOX_HOLD = 8` (`cutscene.c`), the SAME
box-linger mechanism the house exit already uses (`HOUSE_EXIT_BOX_HOLD = 10`) — the box stays
UP showing full text over the exit wait, then shrink-closes.  8 = 1200−1192.

VERIFIED off the fixed port `.osr` (`dialogue_timeline`): L9 `start=1168 full=1177 adv=1200
hold=23t` == retail EXACTLY (was adv=1192).  The chain timing is UNCHANGED — the exit beats
still fire on the 1192 advance; only the box render lingers.  Same measured-box-hold category
as `HOUSE_EXIT_BOX_HOLD` (a render-linger stand-in, not a curve-fit of logic).  Host:
`test_cutscene_transition_fades` updated (box now LINGERS, not closes, after the L9 advance).

## Component 2/3 — the chain EARLY-start (OPEN: beat durations)

The advances match, yet the house + errands still play early — a BEAT-duration gap, not the
dialogue gate:

| boundary | port gap | retail gap | Δ | effect |
|---|---|---|---|---|
| L9-adv(1192) → house L0 start | 145t | 155t | **−10t** | house starts ~10t early |
| house L7-adv → errands L0 start | 73t | 81t | **−8t** | errands starts ~8t early |

Downstream advance offsets (dialogue_timeline): house L0-L7 uniformly ~**−12t** (the −8 L9
carry + minor drift); errands L0-L2 drift to ~**−26t**.  Suspects (all MEASURED stand-ins
calibrated off the OLD `retail.osr`, now ~8-10t short vs `retail-stairs.osr`):
`ARRIVAL_EXIT_WAIT`(10), the two `scene_fade` grid-settle durations
(arrival-exit cover + house-entry reveal), `HOUSE_EXIT` cover settle + the errands-entry
reveal (main.c arms it on chain-complete), and the errands same-speaker advance rate.  These
gate on the fade GRID settling (the real `case-2` gate) — pin the grid settle-rate off
`retail-stairs.osr` res-1112 (probe was inconclusive; needs the right fade res / a per-tick
brightness curve, quirk #99 family) before adjusting.  **NOT curve-fit blind** — RE the fade
performer's per-tick step first.  Deferred to the next dialogue-timing arc.

## Tools added
- `OSS_DLG_TRACE=1` — per-tick `DLGT t= cf= act= sc= rev=/  ib= cl= lg= rd= r= L=` dump at the
  `cutscene_step` site (`main.c`), the definitive port-side dialogue-FSM ground truth.  Gated;
  run via `WSLENV="$WSLENV:OSS_DLG_TRACE"` so it reaches the WSL-launched exe (else the nix
  `OPENSUMMONERS_GAME_DIR` WSLENV is clobbered → the port can't chdir to the game dir → black).
