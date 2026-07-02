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

VISUAL (notes.py render, fixed port | retail-stairs @ t1197): the box + name plate + full text
now render == retail; the t1197 crop differ dropped **46480 → 8553**.  DRAW-VERIFIED the 8553
is a RECON ARTIFACT, NOT a game gap: `draw_probe` @t1197 shows the port and retail draw the
SAME frame — **730 vs 729 draws**, and the portrait region (res 1153 fr1 (8,64,320x320) + res
1004 fr0/1) is BYTE-identical (same res/cel/dst/blend) on both sides.  The one extra port draw
is a capture-side res-ID naming gap (port res=1110 vs retail res=0 on one clipped 32×32 tile at
(64,148) — the known "res=0" proxy ID gap, `res0-ui-banks.md`).  The note1.png tint (port
warmer/tan vs retail pinker, whole-frame) is the RECON decoding the two `.osr` with different
session palettes (a notes.py recon-fidelity limit), not a divergence in the game output.  So
after the fix **t1197 is draw-exact**; no portrait-tint follow-up is warranted.

## Component 2/3 — the chain EARLY-start (RESOLVED, ckpt 179 — it was `ARRIVAL_EXIT_WAIT`)

The advances match, yet the house + errands played early.  **ROOT CAUSE (ground-truthed off
`retail-stairs.osr`): the arrival-exit WAIT was curve-fit to 10 (the "2× anomaly") when it is
the script's literal `0x14=20` mapped 1:1.**  The whole arrival→house→errands chain was shifted
−11t early by that one wrong constant.  **NOT a fade-performer bug** (the FRONT's "settle-rate"
hypothesis is DISPROVEN — see below).  Fix: `ARRIVAL_EXIT_WAIT 10 → 20` (`cutscene.c`).

**How the fade GRID is actually captured** (the linchpin that broke the old analysis): retail's
`scene_fade` cells are ALL `res=0 bmode=1` (the subtract-blend gray mask, res unnamed by the
proxy 0x418470 registry — the "res=0" ID gap, `res0-ui-banks.md`), so there is NO res-1112 vs
opaque split retail-side.  Only COVERAGE (64×4 cells, interior band dy∈[64,416)) is cross-side
comparable — the old plan to "pin the settle-rate off res-1112" could never work.  The clean
signal is the res-1110 dialogue-box open/close + the fade-cell coverage envelope.

**The scene_fade PERFORMER is bit-exact** — cover ramp +80 cells/tick BOTH sides; reveal
recession rate equal.  The finding's earlier "−7t black hold / room-load latency" read was a
CONFOUND of the −11t global WAIT shift (the cover armed 11t early, so the whole cover+hold+reveal
looked short in absolute ticks).  Once the WAIT is 1:1 the house transition is tick-exact with NO
room-load modelling — proving there was no significant house room-load gap; it was ALL the WAIT.

**The "2× anomaly" was a MEASUREMENT ERROR:** ckpt-137 read the cover onset as "first CENTER
alpha" (tick 1234, OLD `retail.osr`).  But retail's arrival cover grows **TOP-DOWN** (a downward
sweep — variant 2, NOT the center-out var-0 stand-in the port forces), so the center is marked
LATE; that lag ≠ the arm.  The true arm = the FIRST cell = tick 1213 = L9-adv(1192)+21 ≈ the
script 20.  The house/L8 WAITs (`0x32=50`) always mapped 1:1 — there was never a per-context 2×.

**Ground-truth ledger** (res-1110 box open/close; `retail-stairs.osr` vs port `spam-confirm-nav`
+ `synth-move2` re-drive):

| boundary | port OLD (WAIT 10) | port FIX (WAIT 20) | retail | Δ fixed |
|---|---|---|---|---|
| arrival cover ARM (first 64×4 cell) | 1202 | 1212 | 1213 | −1t |
| **house L0 box open** | 1320 | 1330 | 1331 | **−1t** |
| **house box close (L7)** | 1636 | 1650 | 1650 | **0t** |
| errands box open | 1679 | 1693 | 1699 | **−6t** |

**RESIDUAL (errands −6t, a separate smaller issue — blocked on the room-load subsystem):** the
house dialogue closes tick-EXACT (1650), so the remaining −6t is entirely in the house→errands
transition.  Measured (interior fade envelope, retail-stairs vs port-waitfix): retail's house
cover is full-black through 1667 then the errands reveal recedes 1668→1689 — a **~20-tick black
HOLD** (the errands room-LOAD under black); the port arms the errands reveal on chain-complete with
NO hold (recedes 1651→1681, −17t at the recede start).  That −17t nets to only −6t at the res-1110
box open because the errands DIALOGUE is deferred to the reveal-recede + the box-open latency.
`HOUSE_EXIT` has NO preceding WAIT (decompile), so this is NOT a WAIT fix like the arrival — it is
the unmodelled errands room-load latency (`PORT-DEBT(cutscene-errands-entry-latency)`).  Adding a
~17-20t black-hold stand-in would match the reveal-recede but OVER-shoot the box open by ~+11t
(the same reveal/box-latency delta that self-compensated in the arrival), so it needs the real
room-load, NOT a curve-fit.  Also: retail-stairs' errands reveal is **edges-in (var 1)** and its
arrival cover is **top-down (var 2)**, but the port forces center-out (var 0) for both — the LCG
variant DRIFTS per capture (`PORT-DEBT(cutscene-fade-variant)`; the OLD retail.osr rolled
center-out, retail-stairs rolls different variants).  Deferred.

## Tools added
- `OSS_DLG_TRACE=1` — per-tick `DLGT t= cf= act= sc= rev=/  ib= cl= lg= rd= r= L=` dump at the
  `cutscene_step` site (`main.c`), the definitive port-side dialogue-FSM ground truth.  Gated;
  run via `WSLENV="$WSLENV:OSS_DLG_TRACE"` so it reaches the WSL-launched exe (else the nix
  `OPENSUMMONERS_GAME_DIR` WSLENV is clobbered → the port can't chdir to the game dir → black).
