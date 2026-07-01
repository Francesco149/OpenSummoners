<!--
  The ONE hand-edited status block.  tools/gen_port_ledger.py injects everything
  between the FRONT:BEGIN/END markers verbatim into docs/STATUS.md's "Current front"
  section, so STATUS can never drift from reality.  Update THIS when the active front
  moves; keep it a 60-second read.  Everything else in STATUS is derived from code.
  Port state / next-move detail belongs here or in HANDOFF.md, NOT in engine-quirks.md.
  Rolling per-checkpoint narrative → PROGRESS.md; deep RE → findings/.  Keep this SHORT.
-->
<!-- FRONT:BEGIN -->
- **Phase — the freeroam HUD panel**, built slice-by-slice bit-exact vs the errands
  freeroam real-play (`sword2.osr`). The prior arc (town intro → arrival/house/errands
  cutscene) renders 1:1; the frame-lock **sim-tick re-drive** foundation is closed
  (ckpt 168 — retail walk warmup = 1 idle tick, `CHAR_INPUT_REPEAT_DELAY` 3→2).
- **Landed (ckpt 167–174, all dhash byte-identical):** leader panel — HP/MP bars +
  numbers + frame + slide-in; element **stars**; **level** digit (+ the ramp custom-
  palette bind `bs_install_palette`/`FUN_005b7bd0`); **EXP** gauge; the 6-slot **item
  bar** (slice 2); the **door-indicator** compass algorithm (slice 3). `hud.{c,h}` +
  `game_render_hud`. Detail: `findings/freeroam-hud.md §1-9`.
- **Open blocker — the PORTRAIT.** Its bank is read live off `char+0x50` at the party
  leader-match branch, but `hud_ctx+0x1b4` (leader_uid) reads **0x0 on every call across
  every scripted replay** (Frida INT3 / field-spec / native probe all agree) — a
  **replay-fidelity gap** (the ring-injection replay doesn't arm what a live human
  session does), NOT a port bug. Resolve by finding the `+0x1b4` setter, or by probing a
  **live/manual** errands play. `findings/freeroam-hud.md §6-9`.
- **Next move:** (a) the door-indicator's `+0x1160` EFFECT-actor **spawn position source**
  (activator `0x41f200` / `game_world.c` exit-table) → retires `PORT-DEBT(hud-door-actors)`;
  (b) bottom-left "quick item" strip (`0x497c20`) → bottom-right combat cluster
  (`0x4975e0`); (c) retire `PORT-DEBT(hud-party-context)` when the party subsystem lands.
- **Open HUD PORT-DEBT:** `hud-party-context` (values + portrait), `hud-door-actors`
  (exit spawn), `hud-slide` / `hud-item-hslide` (slide ramps). See `port-debt.md`.
- **Standing bar:** every divergence is `differ_px==0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic (`parity-model.md`);
  seed-pinned both sides, compared by anchor/tick — never the flip axis.
- **Read next:** changelog → `PROGRESS.md`; deep RE → `findings/` (esp. `freeroam-hud.md`,
  `freeroam-brake-onset.md`); module layout + open threads → `memories/HANDOFF.md`
  (last rewritten ckpt 155 — stale on the HUD arc).
<!-- FRONT:END -->
