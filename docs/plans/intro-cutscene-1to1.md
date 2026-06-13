# Plan — a 1:1 intro cutscene → errands freeroam (the gap punch-list)

> **Status:** OPEN (set ckpt 133).  The dialogue TYPEWRITER-SKIP landed (the port can
> now keep up with retail's dialogue cadence), which UNBLOCKED a real frame-by-frame
> studio compare of the intro — and that compare surfaced the remaining gaps below.
> The USER drove an `osr_view` pass over `port-skip.osr` vs `retail.osr` and dropped 8
> crop+text MARKS (`C:\oss-osr\osr_notes.jsonl`, rendered to the feed + `note_render/`).
> **Until ALL of these are faithful there is no 1:1 intro → errands.**  This doc is the
> durable punch-list; tackle it next session.

## How to reproduce the compare
- Port capture: `C:\oss-osr\port-skip.osr` (the ckpt-133 skip binary over
  `runs/cutscene-verify/nav-zspam-ext.jsonl`).  Re-capture:
  `tools/run-opensummoners.sh -- --no-frame-limit --frames 5200 --input-trace
  runs/cutscene-verify/nav-zspam-ext.jsonl --osr-emit 'C:\oss-osr\port-skip.osr'`.
- Retail: `C:\oss-osr\retail.osr`.
- Studio: `osr_view.exe C:\oss-osr\port-skip.osr C:\oss-osr\retail.osr`.
- The marks: `nix develop --command python3 tools/trace_studio2/notes.py
  /mnt/c/oss-osr/port-skip.osr /mnt/c/oss-osr/retail.osr /mnt/c/oss-osr/osr_notes.jsonl
  --render --feed`.

## A prerequisite for ALL per-line/per-tick compares — the MATCHED-CADENCE nav
The port nav is a `0x24`-SPAM and retail.osr was driven by the proxy's own nav, so the
two progress through the dialogue at DIFFERENT speeds — the port runs AHEAD (e.g. at tick
997 the port is already in the house while retail is still in the arrival scene).  The
`sim_tick` timelines PAIR (2027/2042, `pair.py`), but the CONTENT won't be tick-1:1 until
both sides press confirm `0x24` at the SAME sim-ticks.  **Build a matched nav** (derive
retail's per-line advance ticks from `retail.osr`'s dialogue-text changes → a port nav
that presses `0x24` at those ticks; or drive both sides from one tick-keyed nav).  Several
gaps below are partly "the port is ahead" artifacts that this collapses — but each names a
REAL missing feature underneath, so they stand on their own.

---

## THEME 1 — the dialogue SKIP / progression TIMING (refine the ckpt-133 skip)

**The skip is an improvement but NOT 1:1 — retail is a little SLOWER than the port** (USER
ckpt 133).  The port's `dialogue_skip_reveal` jumps `reveal → total` INSTANTLY; retail's
appears to fill over a tick or two (and/or the per-line start/advance is gated differently).

- **Note #4 — tick 661 (port flip 2437 / retail 1898), crop (296,157 231×90), differ 19282.**
  "port reveals text earlier than retail after X/Enter."  The render shows the PORT line 1
  FULLY revealed ("Ahh, here we are at last! Look, Arche. This is our new hometown.") while
  RETAIL shows only **"A"** (just started typing).  So the port reaches the fully-shown
  state much earlier.
- **Likely cause (RE next):** (a) retail's skip is NOT an instant `reveal→total` —
  `FUN_0043ce50(9)`→`FUN_0043ca40(9)` sets the text-machine state to 3 (fully-shown) but the
  per-tick filler `FUN_0043c650` may paint the remaining glyphs over a tick or two; and/or
  (b) the per-line START / pop-in / advance gating differs from the port's
  `DIALOGUE_ARM_FRAMES` + cadence.  **Probe retail** (don't guess): hook the reveal counter
  / text-machine `+0x18`/`+0x1c` per tick under a CONFIRM-driven nav, capture the exact
  reveal-vs-tick curve for a skipped line, and match `dialogue_step` / `dialogue_skip_reveal`
  to it.  Then verify with the matched nav.
- **Owner:** new (refines this session's `dialogue_typing`/`dialogue_skip_reveal`).

## THEME 2 — the cutscene CAST + ambient render (colour variants + animation)

The arrival scene cast (Father/Arche/townsfolk) + the butterflies render with wrong
colour variants / wrong (static) animation frames.  Mostly KNOWN debts, now visually pinned.

- **Note #0 — tick 274, crop (221,305 52×52), differ 1518 (crop differ_px 131).**
  "butterflies color gap."  → `PORT-DEBT(butterfly-wander)` residual: the per-instance
  sprite **frame_base 0/4/8/12** (the white variant + directions) isn't selected — all
  butterflies render one variant (ckpt-110 known; RE the emit `0x492670` frame_base ↔
  instance/heading map).
- **Note #1 — tick 274, crop (137,290 48×89), differ 1518 (crop differ_px 1387).**
  "npc color variant gap."  → a townsfolk/cast actor renders the wrong COLOUR variant.
  Map to `PORT-DEBT(effect-color-variant)` / the actor sprite frame_base selector; confirm
  which actor + which variant index retail uses at this tick.
- **Note #2 — tick 307, crop (351,304 50×47), differ 1557 (crop differ_px 170).**
  "butterfly movement different."  → `PORT-DEBT(butterfly-flutter)` (the vertical flutter +
  flap/reversal coupling — the ≤2px transient) OR a heading-phase mismatch at this tick.
  Check the butterfly heading/flutter state at tick 307 vs retail.
- **Note #3 — tick 339, crop (76,285 55×94), differ 2522 (crop differ_px 820).**
  "different NPC animation frame."  → `PORT-DEBT(cutscene-party-chars)`: the cutscene cast
  is STATIC (no walk-cycle/breathing), so an NPC sits on a different frame than retail's
  animated one.  The big one — the multi-part party-band render `0x4997b0`.

## THEME 3 — the arrival→house ROOM-TRANSITION CHOREOGRAPHY (NEW detail)

Retail plays a CHOREOGRAPHED transition between the arrival dialogue and the house room;
the port does an instant key-swap SNAP.  Maps to `PORT-DEBT(cutscene-beat-runner)` (the
non-dialogue beats: actor run-offs + fades) but the specifics below are newly pinned.

- **Note #5 — tick 997 (port flip 3109 / retail 2233), crop (305,283 88×110), differ 200515.**
  "port snaps to house, retail shows arche running to the house (not player controller,
  scripted cutscene)."  The render: PORT = the house interior (already swapped); RETAIL =
  the arrival scene with Father + Arche by the wagon (Arche about to run to the house).  →
  retail has a **scripted "Arche runs to the house" beat** (the actor run-off `0x402730`/
  `0x401e60` family) and the room load is GATED on it; the port commits the room key
  instantly.  Port the run-off beat + gate the `reload_room_backdrop` on it.
- **Note #6 — tick 1239, crop (336,153 166×195), differ 210989.**
  "retail has a similar reveal effect as the intro town pan but inverted."  → a reveal/iris
  effect (the `scene_fade` / reveal-grid family, like the establishing-shot iris) on house
  ENTRY, INVERTED (closing/opening the opposite way).
- **Note #7 — tick 1268, crop (185,130 258×201), differ 220375.**
  "reveal effect fades back from black to house (same effect as intro town pan beginning)."
  The render: PORT = the house at FULL brightness (+ an already-popped dialogue box);
  RETAIL = the house mostly BLACK, fading in from the top.  → a **fade-from-black reveal**
  on house room entry that the port lacks (the same `scene_fade` the intro town pan uses;
  the port's `reload_room_backdrop` shows the new room instantly at full brightness).
- **Owner:** `PORT-DEBT(cutscene-beat-runner)` (fades type 2 + actor run beats) — these
  notes are the concrete spec for the arrival→house segment of it.

---

## Suggested order (next session)
1. **The matched-cadence nav** (the prerequisite) — collapses the "port is ahead" half of
   #4/#5/#7 and makes every per-line compare honest.
2. **THEME 1** (skip timing) — small, refines fresh code, and removes the dominant dialogue
   drift; probe retail's reveal curve.
3. **THEME 3** (room transition) — the big-differ, clearly-missing features (Arche-runs +
   house fade-in); ports a chunk of `cutscene-beat-runner`.
4. **THEME 2** (cast colour/animation) — the cutscene-party-chars render + butterfly
   variants; the most involved (the animated party-band render `0x4997b0`).

Cross-refs: `docs/port-debt.md` (butterfly-wander / butterfly-flutter / cutscene-party-chars
/ cutscene-beat-runner / effect-color-variant), `docs/findings/engine-quirks.md`,
`plans/controllable-arche-faithful.md` (the freeroam hand-off, behind this 1:1 bar).
