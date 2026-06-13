# Plan — a 1:1 intro cutscene → errands freeroam (the gap punch-list)

> **Status:** IN PROGRESS (set ckpt 133).  **THEME 1 (dialogue TIMING) + the
> MATCHED-CADENCE-NAV prerequisite are DONE (ckpt 134)** — the arrival dialogue (L0-L7)
> now tracks `retail.osr` TICK-FOR-TICK (start/full/advance bit-equal, 314/323 ticks of
> name+body identical).  THEMES 3 (room-transition choreography) and 2 (cast render)
> remain.  This doc is the durable punch-list; the USER drove an `osr_view` pass over
> `port-skip.osr` vs `retail.osr` and dropped 8 crop+text MARKS
> (`C:\oss-osr\osr_notes.jsonl`).  **Until ALL of these are faithful there is no 1:1
> intro → errands.**

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

## ✅ DONE (ckpt 134) — the MATCHED-CADENCE nav (the prerequisite)
Built `tools/trace_studio2/dialogue_timeline.py NAV`: it reads retail's per-line SKIP tick
(the reveal-jump) + ADVANCE tick off `retail.osr` and emits a nav that presses `0x24` at
those exact SIM-TICKS.  Enabled by **tick-keyed input-trace** (`input_trace` entries may key
on `{"tick":N}` not just `{"frame":N}`, axis per-entry so one nav mixes a flip-keyed boot
prefix + tick-keyed in-game confirms — the port's Flip cadence differs from retail's, so
only the shared sim-tick aligns the dialogue).  Regen:
`dialogue_timeline.py NAV /mnt/c/oss-osr/retail.osr runs/cutscene-verify/nav-zspam-ext.jsonl
600 985 0 1100 | grep -E '^[#{]' > runs/cutscene-verify/nav-matched.jsonl`.

---

## ✅ DONE (ckpt 134) — THEME 1, the dialogue progression TIMING

**The port's reveal/skip MECHANICS were ALREADY faithful — note #4 was a CADENCE artifact,
not a reveal-rate bug.**  `dialogue_timeline.py` read the curve off `retail.osr`: retail
types **1 char / 5 ticks** (space 1t) and the skip is an **instant `reveal→total` jump** —
exactly the port's model.  The port "revealed text earlier" (note #4) only because the
`0x24`-SPAM nav skipped every line the instant it armed and raced ahead.  With the matched
nav the arrival dialogue (L0-L7) tracks retail TICK-FOR-TICK.  Two cadence gaps were fixed
(NOT the reveal rate, which was already right):
- **The box re-pop model.**  The port re-armed (full 20-update pop-in) every line; retail
  keeps the box on a SAME-speaker advance (`dialogue_set_text`, gap 1t) and re-opens from
  HALF scale on a SPEAKER CHANGE (`dialogue_reopen`, `DIALOGUE_REOPEN_SCALE=500` →
  ~10-update reopen = retail's advance+11t; vs the first box's ~20-update slide-in).
- **The word-wrap space.**  `dialogue_expand_text` consumed the wrap space; retail renders
  it → keep it (body byte-identical).
- **The speaker-change box OVERLAP (USER studio note, tick 696) — DRAWCALL-EXACT.** Retail
  OVERLAPS the opening NEW box (IN FRONT) over the closing OLD box (BEHIND) on a speaker
  change; the port single-swapped.  USER: no approximation — so `tools/trace_studio2/
  draw_probe.py` (the ordered-drawcall region probe) read retail's EXACT choreography and the
  port reproduces it cell-for-cell: z-order new-in-front (`render_dialogue_box` closing-then-
  main), open spawn 200 +50/update, close -40/update removed <160, old box lingers full until
  the new box passes half (`box->scale > 500`), advance at `advance_tick - 6`; same-speaker
  re-text deferred (`pending_keep`).  Also fixed the advance-boundary residual.
- **VERIFIED drawcall-per-drawcall** (draw_probe, port vs retail.osr): every box-frame cell
  (pos+scale) matches across EVERY arrival speaker change — L0->L1 28/28, L1->L2 28/28,
  L2->L3 29/29, L5->L6 33/33 ticks EXACT; per-tick (name,body) 322/323 (tick 884 = a
  retail-coalesced flip).  The ckpt-134 first cut (curve-fit close + guessed z-order) was
  rejected by the USER → CLAUDE.md no-approximation / drawcall-per-drawcall rule persisted.
- **Note #4** (tick 661: port full line vs retail "A") — CLOSED, it was the spam-nav cadence.

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

## Suggested order
1. ~~The matched-cadence nav (prerequisite)~~ **DONE ckpt 134.**
2. ~~THEME 1 (dialogue timing)~~ **DONE ckpt 134** — mechanics were already faithful; fixed
   the box re-pop cadence + the wrap space → tick-1:1.  (One residual: the advance-boundary
   1-flip clear, USER-verify.)
3. **THEME 3** (room transition) — NEXT.  The big-differ, clearly-missing features: the
   167-tick "Arche runs to the house" beat between arrival L7/L8 (#5, `0x402730`/`0x401e60`,
   gate the room load on it) + the fade-from-black / inverted-iris house-entry reveal
   (#6/#7, the `scene_fade` the port lacks on `reload_room_backdrop`).  The divergence after
   tick 982 (port advances L7→L8 instantly; retail's L8 is at tick 1149).
4. **THEME 2** (cast colour/animation) — the most involved (the party-band render `0x4997b0`
   + butterfly variants).  Now that the dialogue is tick-1:1, the matched nav makes the
   cast/ambient frames at ticks 274-339 (notes #0-#3) honestly comparable.

Cross-refs: `docs/port-debt.md` (butterfly-wander / butterfly-flutter / cutscene-party-chars
/ cutscene-beat-runner / effect-color-variant), `docs/findings/engine-quirks.md`,
`plans/controllable-arche-faithful.md` (the freeroam hand-off, behind this 1:1 bar).
