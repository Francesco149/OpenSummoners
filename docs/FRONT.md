<!--
  The ONE hand-edited status block.  tools/gen_port_ledger.py injects everything
  between the FRONT:BEGIN/END markers verbatim into docs/STATUS.md's "Current front"
  section, so STATUS can never drift from reality.  Update THIS when the active front
  moves; keep it a 60-second read.  Everything else in STATUS is derived from code.
  Port state / next-move detail belongs here or in HANDOFF.md, NOT in engine-quirks.md.
  Rolling per-checkpoint narrative → PROGRESS.md; deep RE → findings/.  Keep this SHORT.
-->
<!-- FRONT:BEGIN -->
- **Phase — the freeroam sim**, built bit-exact vs errands re-drives. The prior arcs
  (town intro → arrival/house/errands cutscene; the HUD panel ckpt 167–174) render 1:1.
- **Landed ckpt 175 — COLLISION:** the movers `0x54ded0`/`0x54db10` tile halves ported
  (`collision.c`) + `character_step` restructured to the retail `0x442a70` tick order
  (support probe → vertical mover/ledge-fall → ramp → worldX commit); slope ramps
  read live off the user's exe; the `LAB_00589520` "occlusion marks" = invisible
  collision WALLS (map_decode fix). **Arche stops/climbs/descends the errands stairs
  tick-exact** (dwx==dwy==0 through the whole climb; both walls flush) — retires the
  studio-note-2441 divergence + debts `char-collision-mover`/`collision-slopes`/
  `decode-occlusion-mark`. Sole residual: the 4-tick standing TURN-AROUND
  (`char-turn-state`). RE: `findings/freeroam-collision.md`.
- **HUD blocker (parked) — the PORTRAIT.** `hud_ctx+0x1b4` (leader_uid) reads 0x0 on
  every scripted replay (a replay-fidelity gap, not a port bug); resolve via the
  `+0x1b4` setter or a live/manual play. `findings/freeroam-hud.md §6-9`.
- **Next move (USER marks, port-stairs|retail-stairs `osr_notes.jsonl`, ckpt 175):**
  (a) **dialogue ADVANCE-GATE — the port runs EARLY** (mark t1197: retail box shows the
  Mother's "wait up for your poor, old, slow parents" line, the port box is already
  empty under the SAME confirm injections → the whole chain + the house REVEAL plays
  early).  Suspect: retail gates the confirm on text-reveal completion / a per-line
  min-hold the port lacks — RE `0x48cf80`-family box FSM's accept gate; compare
  line-advance ticks port vs retail in the stairs pair.
  (b) **missing house props** (mark t2278: the stove's steaming COOKING POT + the
  kitchen HUTCH with dishes, upstairs) — the unported object-spawn pass `0x58c8c0`
  (the res_explorer PLACEHOLDER objects; mom's pose in-crop differs too — check her
  clip once the props land).
  (c) `char-turn-state` — the 4-tick standing turn-around (`0x426f50(body,2)` case-2
  sub-FSM, `0x442a70:810-830`), the last freeroam-walk residual.
  (d) HUD: door-indicator spawn source / bottom strips; `mover-actor-scan` when
  collidable actors matter.
- **Open PORT-DEBT (this front):** `char-turn-state`, `mover-actor-scan`,
  `char-drop-through`, `char-reverse-decel`; HUD: `hud-party-context`,
  `hud-door-actors`, `hud-slide`/`hud-item-hslide`. See `port-debt.md`.
- **Standing bar:** every divergence is `differ_px==0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic (`parity-model.md`);
  seed-pinned both sides, compared by anchor/tick — never the flip axis.
- **Read next:** changelog → `PROGRESS.md`; deep RE → `findings/` (esp. `freeroam-hud.md`,
  `freeroam-brake-onset.md`); module layout + open threads → `memories/HANDOFF.md`
  (last rewritten ckpt 155 — stale on the HUD arc).
<!-- FRONT:END -->
