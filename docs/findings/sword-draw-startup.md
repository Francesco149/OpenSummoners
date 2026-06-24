# The sword DRAW/SHEATHE startup latency (frame-lock chase #2, ckpt 163f)

**Gap (frame-lock loop):** the port engaged the sword-out form the INSTANT Z resolved
(press+0); retail engages it ~3-4 ticks later.  Earliest divergence in the sword2.osr
freeroam span — a clean RENDER-timing offset (not body/movement).

## Ground truth (sword2.osr, `sword_cels.py`, FRAMEBEG axis)
DRAW (Z@nav-tick 1807):
- res 0x570 sword-IN idle (fr0) holds through **1809**; res 0x571 sword-OUT **fr96
  first renders at 1810** (= +3 ticks past the press).  Port (pre-fix): swapped at
  **1806** — i.e. **4 FRAMEBEG-ticks early** (its toggle = press+0, plus the 1-tick
  BLIT-vs-input label skew the FRAMEBEG axis carries).
SHEATHE (Z@nav-tick 3194): res 0x571 idle through 3196; res 0x570 **fr96 at 3197** —
same ~+3t latency.  So the latency is symmetric (draw == sheathe).

## Mechanism (RE — why the latency, not curve-fit)
Z (ring 9) does NOT flip the stance on the press tick.  It QUEUES a context-action:
- `478ba0:477` → `FUN_00479de0(entity+0x14854, action, 0xd2)` writes the cmd block:
  `+0x14868` = the action value, `+0x14870` = type **0xd2** (a queued-command record;
  `+0x1486a/+0x1486c` = 0, its working timer/phase).
- The per-tick integrator `442a70` dispatches on the CURRENT form (`entity+0x1d4`):
  case **0xc35a** (sword-IN) → `FUN_0045a300(.., &DAT_0062f868, 0xd2)`; case **0xc35b**
  (sword-OUT) → `45a300(.., &DAT_00636a58, 0xd2)` then `45e830`.  `0x45a300` is a 14 KB
  ACTION-EXECUTION state machine (fields +0x48 state / +0x58 phase / +0x56 type / +0x5c
  timer …) that runs the draw/sheathe action's STARTUP phase before re-installing Arche
  to the other form (`41f200`: 0xc35a↔0xc35b = bank 0x8b↔0x8c = res 0x570↔0x571).
- So the form swaps — and the new sheet's fr96 first appears — only AFTER the action
  FSM startup (~3t); through it Arche holds the current stance's idle.

The port models animations via direct `anim_clip`s and does NOT model the 45a300 action
FSM at all, so the startup latency does not emerge.  `PORT-DEBT(sword-draw-startup)`: we
reproduce it as a deferred-toggle timer (a stand-in for the unported FSM, same pattern as
butterfly-flutter-trigger / sword-attack-trail), value calibrated to the recording.

## The fix (`character.{c,h}` `character_resolve_sword`)
A Z press latches the toggle TARGET + a `CHAR_SWORD_DRAW_STARTUP` (=4) countdown
(`sword_pending`/`_timer`/`_target`); `sword_out` flips when it elapses.  The bank swap
(`freeroam_step`) AND the `arche_sword_clip` draw/sheathe edge both key off `sword_out`,
so deferring it shifts both in lockstep — Arche stays sword-in (res 0x570 idle) through
the startup, exactly as retail.  Presses during a pending startup are ignored (retail
can't re-queue mid-startup; the recording never double-presses in the window).  No quest
gate (ckpt 159).  The value 4 lands the draw at the recording's FRAMEBEG onset (1810);
the underlying retail latency is ~3t, the +1 absorbs the FRAMEBEG BLIT-vs-input skew.

## Verified bit-aligned (port-sync3.osr vs sword2.osr, `sword_cels.py`)
- DRAW: res 0x570 idle through **1809**, fr96 at **1810** (== retail); every cel's dst
  W×H byte-identical (31x61,36x59,35x59,32x62,51x54,44x62,31x68,30x68); fr103 ends 1865,
  idle resumes 1866 — both sides.  ±1 mid-cel boundary = pre-existing flip-aliasing
  (the pre-fix port carried the same noise, shifted 4t early), not introduced here.
- SHEATHE: fr96 at **3197** (== retail), cels + durs match (the dst x=254 vs 270 is the
  downstream body-position residual — a SEPARATE movement gap, not the sheathe).
- NO body/camera regression: `sword_out` doesn't drive `character_step`; the draw window
  body-x is +0px every tick, first divergence still 1923 (the walk aliasing), unchanged.
- Host: `test_character_resolve_sword` rewritten for the deferred semantics; 1063 pass.

## Residual / next
- The exact 3-vs-4 split is FRAMEBEG-label noise; calibrated to the recording is faithful
  for a stand-in.  A real port of the 45a300 action FSM would make it emerge (and pin 3).
- Next frame-lock chases (sync_diff over port-sync3.osr): the sustained body gaps at
  ~2085 (brake, likely flip-aliasing), the dashes ~2385/~2655 (+35/+58px — the big real
  movement gaps), the end-state camera +16 (half-tile) offset.  See `plans/frame-lock-1to1.md`.
