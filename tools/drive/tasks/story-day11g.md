---
name: story-day11g
start: story-d11f
steps:
  - in control in the puzzle: world != 0 and control
  - left the puzzle room: room != 440240 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: finish the pot puzzle (walkthrough Day 11 step 5.7: blocks II, I, III → the magician → seal broken:
Unison), from the end of run `story-d11f`** (`rd new story-d11g --from story-d11f` — the RUN: f741344 in **440240**, the
puzzle room, control, leader **Stella** standing on block I; Arche on 'Stay There!' on the upper-right ledge). Placed:
VII, VIII, IX, IV, V, VI. Left: **I** at (46400, 19200), **II** at (67200, 7600) held by vines, **III** (handle 509?) at
(118400, 67200). Party Lv16/15/15 full HP; 18856 CP. Last save: slot 8 `0c5856a7a482` (the 440220 bed, BEFORE the puzzle;
a block break resets the puzzle). **Read `/mnt/ephemeral/opensummoners/drive/agents/story-d11f/report.md` first** (the
board, the mechanics, every burn tried) and the walkthrough Day 11 step 5.7 (+ the [RD story-d11f] note).

**The vine rule (retail `0x40be30`, ported `src/gimmick.c` `switch_link`):** a vine piece (code 70400) that switches
(burns) then switches every actor in the world list whose handle **+0x1d8** == its link **+0x298** (recursively through
THEIR links); **+0x288** == 0 ⇒ only the first match; **+0x29c** != 0 inverts. State = **+0x00c**. So the burn groups are
fixed map data, not physics: FIRST read, for every 70400 actor in 440240, (body x, y, +0x1d8, +0x298, +0x288, +0x29c,
+0x00c) — the world list is `world+0x2790` (count u16 at +0, items at +4… see `src/world_layout.h` / how story-d11f named
its vine bodies) — and draw the link graph in your report. Then pick the shot that burns II's SIDE pieces without the
chain reaching the piece under II (70000, 14000), push II left 6400 (to x 60800), burn the under piece → II falls to
(60800, 54400). If every side piece's chain includes the under piece, look for the order the guide means ("burn AROUND
(not under) II, push II slightly left, burn under it") — e.g. a hit that reaches only one side, or pushing II while a
chain is still spreading — and say what the graph allows.

**Then (guide):** I off the upper ledge's right edge → its spot; a member on the right balance platform (down), push III
onto it; both girls on the left platform → III level with the left ledge; push III onto the ledge; girls to the right
platform; push III off the left edge → its spot. Use the lever (70216 at (16000, 64000), V facing it) to neaten blocks.
`rd ck save` before every irreversible push / burn. Then talk to the magician → events → **Unison**; back on the 440220
bed: save (`rd endsave`; overwrite the shortest play time, keep slots 1–8 and 24). If frames remain: 5.8 roof → MoH 17
(boost jump), the plank scaffolding chest [Memory Photo 7]. Stop there, at ~40k frames past the start, or at a point you
cannot get past after real attempts (say what blocks you, with an `rd look` PNG path).

**Tools:** `rd zones R --all`; `rd map R --crop`; `rd ck R save/load/drop` (pool ~30 — when full, search prints only an
error) — use `ck load` / `undo`, NOT `rd rewind`, and AVOID `rd strip` / `--at` on this long movie (they replay from boot:
the main session idles out and is rebuilt, ~40 min, every checkpoint lost). Hold standing states with `lstate == 0`.
Follow Mode: F1 → Party Commands (F3 presses can be dropped); an `ent` right after a menu closes can be dropped — check
`lid` after each leader switch. Objects / levers with V facing them. Check every search's `found … N frames` line BEFORE
applying. Searches `-j 1`. Never step scripts through a Yes/No / save prompt. Do NOT allow `f2` in searches. Never wrap
rd in `timeout`. `RD_HOST=bunker` for every rd command; one rd command at a time.

Do not grind; do not read other runs' movies. Gates cost ~40 min each: run `rd check` + `rd verify` ONCE (mandatory; at
the end is fine). Your report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps
with frames, end-save digests, levels + equipment at the end, the vine link graph, the solution that worked, walkthrough
corrections, tool gaps.
