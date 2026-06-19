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

## 3. The wall colour (note #11) — a HUE SHIFT (color-grade / palette), deferred

[75,283,30,38] "wrong wall colour".  Recon (osr_prof, tick 1780 vs 1726) shows it is a
clear **hue shift on the wood-plank WALL**: the PORT wall is WARM BROWN, retail's is a
cooler GREENISH-GRAY — while the props (the plant/vase), the sign, and the shelves render
identically.  So it is NOT a tile-frame or sprite difference; it is a **palette / scene
color-grade** difference applied to the wall tiles (the errands scene's grade, or the wall
bank's decoded palette).  Investigate `color_grade.c` (the per-scene grade) + the wall
tile bank's palette decode.  DEFERRED — a color change is very visual + wants USER
confirmation, and it is a subsystem (grade/palette) not a quick tile fix.  Feed: the
PORT(brown)|RETAIL(grey) wall montage.

## 4. The shelf props (notes "missing props in shelves" / "bookshelf missing props") — FIXED (z-order, ckpt 146, `ead9c49`)

[315,343,143,85] + [81,290,102,129] — these were **NOT a missing spawn**.  The port
ALREADY emits the shelf props (the structure band, `res=1026`/`res=1027`, at the exact
retail frames + positions — verified via `draw_probe`).  The bug was **Z-ORDER**: the
ckpt-145 ERRANDS_CAST furniture stand-in (the bookshelf FRAME `res=1023 fr3` @80,288, and
the two shelf UNITS `res=1027 fr9` @320,352 / @384,352) spawned at the **cast layer 13**,
which draws OVER the layer-8 structure props and OCCLUDED them.  The draw pool walks its 27
layers in index order (lower = behind), so layer 13 > layer 8.  Retail draws the
frame/units FIRST (seq 257/261) then the small props ON TOP (seq 268+).

**Fix:** `room_cast_member` gained a per-member `layer` (0 ⇒ the default cast layer 13);
the BACKGROUND furniture (the bookshelf frame + the two shelf units) is set to **layer 7**
(below the layer-8 structure props).  `actor_spawn_room_cast` honours it; the family /
foreground props stay 13.  VERIFIED: the port seq order now matches retail (frame/units
before props) and both flagged shelves recon pixel-matching retail (fully stocked).
+test `errands_cast_zorder`.

LESSON: a "missing" element may be EMITTED-but-OCCLUDED — always check the draw-stream seq
order (the z), not just "is it drawn".  The ckpt-145 furniture stand-in (cast layer 13)
silently regressed the structure-band props behind it.

## Tooling note
`osr_prof.exe` (built `make -C tools/osr_view prof` → `build/osr_prof.exe`) reconstructs
any `.osr` frame headless: `osr_prof.exe <file.win> dump <frame_idx> <out.bmp>`.  Map a
sim_tick → frame index with `osr.stream_frames(path)` (enumerate; the Nth FRAMEBEG = index
N).  This is the "LOOK at it yourself" path used to locate the fire above.
