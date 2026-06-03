# In-game intro — the scene past the prologue (map 0x3f2) — PHASE KICKOFF (ckpt 49)

The user opted to **extend the trace in-game** (ckpt 48 decision): spam Z after
the prologue begins, give it ~1 min of frames, then port to match retail.  This
doc is the phase kickoff — the RETAIL golden target + the entry-function map +
the porting plan.  No port code yet; the in-game engine is unported.

## How you reach it (the Z-spam exit)

The prologue cutscene `0x56cd20` exits on the **3rd "beat"** — any fresh key
press (`prologue_stone`: `beats++`, 3rd → exit fade → ~200-tick watchdog →
`PROLOGUE_DONE`).  **Z = the confirm id `0x24` (36)** = a beat (the NORMAL exit);
`0x22` would *abort* to the title instead.  On `PROLOGUE_DONE` retail's boot
driver calls **`0x59ec30(0, 0, 0x3f2)`** (app_flow.c chain: `0x1a → 0x564160 …
0x56cd20 … 0x59ec30(0,0,0x3f2)`), which loads + runs the opening map.

Trace (RETAIL flips, lockstep): Start@615 → down@715/740 → confirm@795 → fade →
prologue_enter@815 → Z beats @850/870/890/910/930 → exit → in-game.  Then Z every
20 flips advances the story dialogue.  Saved: `tests/scenarios/in-game-intro/`.

## The RETAIL golden (runs/tas-ingame-1 — local only, runs/ is gitignored)

Captured under `--lockstep` (240 s, 250 events; anchors subtitle@432 /
newgame@667 / prologue@815, consistent with all prior runs).  Frame-content
sweep (mean brightness / distinct colors):

| flip | content |
|------|---------|
| 900–1050 | **black** (cutscene exit-fade + the post-3rd-beat watchdog + map load) |
| 1100 | transition begins (mean 0.7, 99 colors) |
| **1150** | **first in-game frame — the opening TOWN of Tilelia** (houses, NPCs, hill backdrop, a banner across the top) |
| 1200–1700 | the town scene (mean ~70, 2–5k colors) |
| ~2200–2500 | **story DIALOGUE** — a character portrait (the girl) + an empty dialogue box; Z advances it |
| 3000 | dialogue continues |

So the in-game intro is: **opening town map → intro story dialogue** (Z-advanced).
Montage pushed to llm-feed (frames 1150/1200/1400/1800/2200/2500).

**The game proper renders from ~flip 1150** (retail).  900–1100 is black (the
fade-out + the map resource load — `0x59ec30` logs `"Unload Resource From…
Terminate"` between scenes and loads the new map's banks).

## Entry-function map (what to port)

```
0x59ec30(param1, param2, map=0x3f2)   — scene LOAD/UNLOAD WRAPPER (531 B)
  alloc scene obj (0x18) → *in_ECX
  iVar3 = 0x59f2c0(map, param2, …)    ← THE MAP RUN LOOP (3522 B) — the bulk
  unload resources (0x5a4320 / 0x5bef0e ×3 / 0x43e3a0(2) / 0x5a4440)
  result switch: 4/5 → 0x59ee50(...) → next-scene id (0x16→0x2724, 0x17→0x272e)
                 else → teardown + return
0x59f2c0(map, …)                      — MAP LOOP: zeroes a HUGE stack frame
  (~0xeb1c B: tile arrays 0x800/0x1800/0x800 dwords, 8× entity blocks 0x47/0x108,
   a 0x600 + 0x4120-byte map object) → loads the map → runs the per-frame
   update/render loop → returns a result code (next-scene / exit).
```

`0x59f2c0` is the in-game engine entry: tile map, entities/party, the per-frame
update + the in-game render dispatch (ROADMAP: `0x5a00c0` primary render
dispatch, `0x590230` wave/audio mgr, scene transition/load).  This is
**milestone 2** (the game proper) — a multi-checkpoint port.

## Porting plan (the same loop as title/new-game/prologue)

1. **A game-entry ANCHOR.**  Add a retail-side anchor at `0x59ec30` (or
   `0x59f2c0`) entry + a port-side `enter_game` so `tas_diff` can align the
   in-game frames (the existing anchors stop at prologue_enter).  The cutscene
   exit-fade + load makes the flip offset between commit and first game frame
   variable, so an anchor at the map-loop entry is needed (not a fixed offset).
2. **Wire the port seam.**  `PROLOGUE_DONE` currently re-displays the title
   (`leave_prologue_to_title`).  Route it to an `enter_game` stub that logs the
   `0x59ec30(0,0,0x3f2)` entry — the explicit in-game seam to build behind.
3. **Survey + port `0x59f2c0`** in units (decompose the 3522-B loop): the map
   object init, the resource/bank load for map 0x3f2, the per-frame update, the
   render dispatch — port-and-test each against the golden, smallest visible
   win first (likely the static town backdrop/tilemap render → then entities →
   then the dialogue box).
4. **Diff** each ported piece vs `runs/tas-ingame-1` at the matching tick.

## Open questions for the port

- The map-0x3f2 resource set: which DLL banks + the load path (`0x59f2c0`'s
  loader) — likely needs the deferred `ar_register_*` batches (the in-game
  sprite/tile banks) registered at boot, the way the title banks were (ckpt 26).
- The dialogue system (portrait + textbox + the story script for map 0x3f2) —
  a sub-system of the map loop; the text is likely the glyph pipeline again.
- Whether the intro is scripted (an event track auto-advanced by Z) or free
  movement — the golden shows a banner + dialogue, suggesting a scripted intro.
