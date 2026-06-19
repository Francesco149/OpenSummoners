# The errands (freeroam) render gaps — the USER studio notes, RE'd

The USER's `osr_notes.jsonl` (on `port-errands.osr`) flags the remaining errands gaps.
This is the RE of each (off `retail.osr` via `osr_prof` recon + `draw_probe`), so the
ports are clean follow-ups.  The exact USER marks (tick + crop + text):

| tick | crop [x,y,w,h]      | USER note                          |
|------|---------------------|------------------------------------|
| 1726 | [309,154,96,89]     | **missing fire in fireplace**      |
| 1726 | [75,283,30,38]      | **wrong wall colour**              |
| 1726 | [315,343,143,85]    | **missing props in shelves**       |
| 1726 | [81,290,102,129]    | **bookshelf missing props**        |
| 2413 | [0,0,294,92]        | **top left hud**                   |
| 2413 | [0,443,140,37]      | **bottom left hud**                |
| 2413 | [430,433,210,47]    | **bottom right hud**               |
| 2413 | [192,402,49,67]     | door indicator                     |
| 2413 | [176,348,65,74]     | arche idle animation (when still)  |
| 1579-1604 | [369,343,48,71] | arche turns the other way (→ DONE ckpt 146, `findings/arche-house-turn.md`) |

The errands camera is STATIC (no follow) in the opening — screen positions below are
stable across the opening ticks.

## 1. The fireplace FIRE (note #10) — FULLY RE'd, port deferred

The fireplace is at screen ~(309-405, 154-243) — the recess behind the grate at the
base of the brick chimney (upper-center).  PORT shows a BLACK empty recess; RETAIL has
a roaring orange fire.  Recon crop (`osr_prof` tick 1726) confirms it visually.

**The fire is `res=1034`** (PE DATA resource 1034), drawn:
- dst **(329,178), 48x39**, CONSTANT position/size.
- **ALPHA-blended**: primitive `alpha`, `st=0x8000`, **`bmode=1`** (additive flame glow,
  NOT the colorkey blit the furniture uses).
- **frames 0,1,2,3,4,5 looping**, ~**6 sim-ticks/frame** (a 36-tick loop) — measured off
  `draw_probe --res 1034` over retail ticks 1726-1786 (seq=264, the per-frame cycle).
- **The PORT draws 0 res=1034** anywhere (town or errands) — its bank (PE DATA 1034) is
  NOT registered/loaded.

**Why deferred (not a clean autonomous chip):** porting it drawcall-exact needs (a) the
res-1034 sprite bank registered + decoded in the errands load (a new asset-register
bank, adjacent to PORT-DEBT(assetreg-clone-defer)), AND (b) the alpha blit matched to
retail's `st=0x8000`/`bmode=1` (the port HAS an alpha path — `zdd_alpha_blit` + the
`actor` node_alpha — but the exact blend descriptor must match, and the alpha *look*
wants USER visual confirmation, which is deferred).  When ported, the cleanest shape is
an animated, alpha-blended `ERRANDS_CAST`-style member (clip = frames 0-5 dur 6) at
world pos projecting to (329,178), OR via the proper errands CHARACTER band
(PORT-DEBT(errands-cast)).  Feed: the PORT|RETAIL recon comparison.

## 2. The freeroam HUD (notes #13-18) — IDENTIFIED, not ported

The `res=0` draws present in retail's errands but ABSENT in the port are the **freeroam
status HUD** (the gameplay UI), NOT particles:
- **top-left** [0,0,294,92]: the leader portrait + **HP "100 / 100"** + **MP "20 / 20"** +
  **level "1"** + **★★** (the two element stars) + the bar.
- **bottom-left** [0,443,140,37] + **bottom-right** [430,433,210,47]: the lower HUD strips.
- The HUD numbers/glyphs render as `res=0` primitive draws (the small clustered draws that
  "move" frame-to-frame in `draw_probe` are the changing digits/glyphs, not motion).

This is the in-game status panel — a real UI subsystem to port (the party/character stats
exist port-side; the panel LAYOUT + bar/star/number rendering is the work).  Best done
with the USER (the HUD layout is very visual).

## 3. The wall colour (note #11) — wall TINT, not yet RE'd

[75,283,30,38] "wrong wall colour" — a subtle wall tint/shade difference (maxd ~33 per the
ckpt-145 note).  Likely a palette/tint or a tile-variant on the left wall.  Low priority
(small, subtle); RE the specific tile/palette when convenient.

## 4. The shelf props (notes "missing props in shelves" / "bookshelf missing props")

[315,343,143,85] + [81,290,102,129] — more CHARACTER-band objects (the same class as the
counter/bookshelf/clock furniture added ckpt 145, `01dc162`): items sitting ON the shelves
that the port's suppressed-for-non-town character band never spawns.  The clean port is to
extend `ERRANDS_CAST` (actor_spawn.c) with the missing prop codes → banks/frames at their
captured positions (the established stand-in), OR spawn the proper errands character band
(PORT-DEBT(errands-cast)).  Needs: dump the errands character-band layers in those crop
regions, map each code→bank/frame off the retail draw stream, add to `ERRANDS_CAST`.
Colorkey draws (no alpha) — so a clean follow-up to the furniture pattern.

## Tooling note
`osr_prof.exe` (built `make -C tools/osr_view prof` → `build/osr_prof.exe`) reconstructs
any `.osr` frame headless: `osr_prof.exe <file.win> dump <frame_idx> <out.bmp>`.  Map a
sim_tick → frame index with `osr.stream_frames(path)` (enumerate; the Nth FRAMEBEG = index
N).  This is the "LOOK at it yourself" path used to locate the fire above.
