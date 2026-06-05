<!--
  The ONE hand-edited status block.  tools/gen_port_ledger.py injects everything
  between the FRONT:BEGIN/END markers verbatim into docs/STATUS.md's "Current front"
  section, so STATUS can never drift from reality.  Update THIS when the active front
  moves; keep it a 60-second read.  Everything else in STATUS is derived from code.
  Port state / next-move detail belongs here or in HANDOFF.md, NOT in engine-quirks.md.
-->
<!-- FRONT:BEGIN -->
- **Phase:** Phase 4–5 — porting the **in-game town backdrop** render path toward a trace
  that plays 1:1 pixel-perfect frame by frame on both sides. Milestone map: `ROADMAP.md`.
  Mechanical next chip: `port-frontier.md`.
- **Where we are (ckpt 67): in-game COLOR-GRADE LUT ported — backdrop TILES now `differ_px==0`.**
  Drove the menu→in-game nav trace, diffed the port's town vs a fresh new-trace retail hold
  golden, and chased the first divergence. Two big results:
  1. **The "establishing shot" is a leftward PAN, not a zoom** (overturns the ckpt-65/66
     framing). Live-probed flips 1440–2100: viewport `+0x64/+0x68` and shear `+0x74` are
     **constant**; only `+0x60` pans (128000 hold → 59450). Free-roam render path every frame
     (`0x490cd0` fires; the offscreen/special `0x499100`/`0x48c6b0` never does); projector
     `0x490b90` has no scale. Port's static `MAP_RENDER_CAM_TOWN_3F2` aligns with the golden at
     **dx=0, same scale**. PORT-DEBT `ingame-establishing-zoom` **retired**.
  2. **The missing colour = an in-game per-channel tone-curve LUT** (`DAT_008a9410`,
     `FUN_00562ea0`'s cosine builder; applied by `0x417c40` parallax + `0x490f30` tiles; the
     title/menu use the plain `0x418470`, so they stay bit-exact). NOT the per-sprite tint
     (`DAT_008a93fc==0`, identity). Builder **verified bit-exact** vs a live `DAT_008a9410`
     probe (`LUT[64]=35/128=100/192=175`); gates live-probed `gate1=700 gate2=850`. Ported
     pure + host-tested (`src/color_grade.{c,h}`) and wired in `main.c` at the 8bpp sheet
     conversion (palette-time, in-game-scoped): **the half-timber wall `(173,170,140)` and ivy
     `(107,105,74)` now match retail exactly.**
  **840 pass / 0 fail / 6 skip** (+4 color_grade). Ledger **194/1490 touched / 189 tested**
  (+1: the `0x417c40` LUT slice is now host-tested). Both GUI builds clean.
- **NOT a full `differ_px==0` frame yet — named residuals** (NOT logic bugs): the **24bpp
  parallax far-plane** (sky/mountain banks `0x55`/`0x58`/`0x59` are 24bpp → no palette → the
  8bpp grade skips them, so the sky still renders un-graded/too-bright; retail grades 24bpp via
  a TBD path AND the port's 24bpp decode is itself brighter — PORT-DEBT `render-palette-tint`,
  now sharpened to the 24bpp half); the **NPC actors** (present modes 0/1/2, blocked on the
  entity/spawn system — PORT-DEBT `present-actor-modes`); the **foreground tree** + **"Town of
  Tonkiness" banner** (`0x5a00c0`, PORT-DEBT `ingame-nontile-layers`); the intro **pan** itself
  (`ingame-camera-snap`).
- **Next move:** the **24bpp parallax colour** (where retail grades 24bpp banks + reconcile the
  port's 24bpp→16bpp decode brightness) to finish the sky, then the **foreground tree/banner**
  (`0x5a00c0`) and the **actor renderers** (need the entity/spawn system first). With the camera
  proven (pan, dx=0) and the tile colour bit-exact, a flip-anchored backdrop diff at the hold is
  now meaningful. Full writeup: `findings/in-game-intro.md` "The in-game COLOR-GRADE LUT".
- **Tooling front:** **Phase-B B2 (the field-bearing flow trace) LANDED + live-verified**
  (`docs/plans/trace-tooling-phase-b.md`): `call_trace` carries `seq` + `CALL_TRACE_BEGIN/FIELD/END`;
  the Frida agent reads same-named retail fields per `tools/flow/retail_fields.json` (now incl.
  the `cam_*` camera chain + the ckpt-67 `tint`/`lutgate1/2`/`lut*` colour-grade probes);
  `tools/flow_diff.py` names the first `[chain]`/`[data]` divergence. Remaining Phase B: **B1**
  unified `scenario-test.py`, **B3** DDraw blit-command trace + `render_diff.py`.
- **Standing bar:** every divergence is `differ_px == 0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic
  (`parity-model.md`); seed-pinned both sides, compared by anchor/tick.
<!-- FRONT:END -->
