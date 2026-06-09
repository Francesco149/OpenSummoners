# Plan — the in-game DIALOGUE BOX + town-intro cutscene (Phase 3)

Status: RE complete (ckpt 102), porting begins.  Goal: the town-intro arrival
**dialogue** plays 1:1 (portrait + name + textbox, Z-advanced), which is the gateway
to **controllable Arche** (Phase 4).  Writeup home: `findings/in-game-intro.md`
"The DIALOGUE BOX subsystem".

## Architecture (decompile-verified, ckpt 102)

The town arrival is a **linear cutscene script** run as a blocking coroutine:

```
0x40c380 (scene-script dispatcher)
  └─ 0x4d7d80  case 0x334be  (the TOWN-INTRO script; scene-id @ *(0x8a9b50+0x1038))
       repeat per beat:
         configure a BEAT on the scene-controller object (this/in_ECX)
         FUN_00439680() ── 9-byte thunk ──▶ 0x439690  (the BEAT-RUNNER; BLOCKS)
         if (ret == 6) return 6        // 6 = Escape/abort
```

- **`0x439690`** (8866 B) = the blocking beat-runner.  Per call it (a) on the dirty
  flag, (re)builds the dialogue-box widget tree from the controller's dialogue fields;
  (b) runs a `GetTickCount`-paced frame-pump loop (`while local_274 != 2`, ~16 ms/60 fps)
  that each frame calls **`0x48c150`** (the render driver the port already mirrors for
  letterbox/scene_fade/banner) + `0x5b8fc0` (DDraw flip) + the message pump `0x5b1030`;
  (c) checks the beat's completion condition (switch on beat-type `+0x20`) and returns
  when met.  Returns **6** if Escape is in the input ring (the script aborts).
- **Beat types** (`switch (*(controller+0x20))` @ `0x439690:1128`):
  - `1` = DIALOGUE — completes when advance-input (`0x43b980`, the Z poll) fires.
  - `2` = wait flag `+0x24` clears (e.g. a sub-controller `+0x18` event).
  - `3` = CAMERA reached target (`*(0x8a9b50+0x104c)`: cur scrollX/Y == target).
  - `4` = an entity flag `+0x300[i]` no longer 1.
  - `6` = TIMER `+0x57c` counts down to ≤0 (decremented 1/frame).
  - `7` = portrait-anim done (`in_ECX[0x12] > 0x3f`) or the `+0x57c` timer.
  - `9` = a linked entity's `+0x20` clears.
- **`0x49d6e0`** (590 B) = the DIALOGUE-LINE setup.  Args
  `(type, speaker_actor, text, p4, p5, p6, name_or_0, face_id, p9, voice_id)`:
  - copies `text` → controller `+0x8a`; speaker name (actor `+0x750` when `name==0`) → `+0x28a`;
  - resolves the PORTRAIT id via the face table **`DAT_006b6568`** (records, stride 0x10,
    keyed on `[actor head-state +0x1d8, face_id]`; picks frame `+8`/`+10`/`+12` by the
    actor's anim-state `+0x40→+0x2c`) → controller `+0x84`;
  - sets `+0x2e8 = voice_id` (only if voice enabled), `+0x78 = 1` (dirty), `+0x20 = 1`
    (beat = dialogue).

### The render path (all primitives already in the port)

```
0x48c150 (render driver; letterbox/scene_fade/banner already hang off it)
  └─ 0x48c820  (WIDGET-TREE renderer, 873 B — NOT yet ported)
       ├─ 0x48cf80  → src/newgame_box.c   (9-slice box FRAME, res 0x456)  ✓ ported
       └─ 0x48e200  → src/glyph_render.c   (GDI TextOutA + shadow)         ✓ ported
```

- Box frame = res **`0x456`** (slot `DAT_008a7708`, 0x20×0x20 tiles, 9-patch) — the
  SAME renderer `newgame_box.c` already uses for the new-game panels.
- Text/name = GDI **Courier New** (`0x579f40`, `s_Courier_New_008a28b0`), the dialogue
  font is the 7×16 slot `DAT_008a927c`; per-glyph advance 7 px; drawn with a +1/+1 shadow
  pass then the main pass (`0x48e200`/`0x48e860`).
- Portrait = an actor-derived sprite slot indexed `(&DAT_008a760c)[portrait_id]`, frame
  set via `0x415860` on the box's portrait sub-widget; fade via `0x49c910`.  **Sheet TBD —
  capture.**
- Box position from the speaker's world pos via `0x49c640`; twin/top vs bottom layout by
  controller `+0x2f0`.

### The town-intro script (the data — content stays in the user's exe)

Beats (order), each dialogue line `{speaker_handle, face_id, voice_id, text_VA}`:

| # | speaker (handle)        | face | voice | text @VA   | (gist) |
|---|-------------------------|------|-------|------------|--------|
| 0 | spawn cast + camera/letterbox/banner setup, timers, camera pan (beats 2/6/3/6) |
| 1 | Father `0x5f5e1d3`      | 0x1e | 0x3eb | 0x86d58c   | "Ahh, here we are at last!…" |
| 2 | Arche `0x5f5e165`       | 2    | 0x3ec | 0x86d55c   | "Yay, we're finally here!…" |
| 3 | Mother `0x5f5e1d4`      | 0x1e | 0x3ed | 0x86d500   | "We haven't been here since…" |
| 4 | Arche                   | 3    | 0x3ee | 0x86d4c8   | "Yeah! There's people and shops…" |
| 5 | Arche                   | 9    | 0x3ef | 0x86d47c   | "Hey, Dad! Our shop is in this town…" |
| 6 | Arche                   | 0xd  | 0x3f0 | 0x86d45c   | "I wanna see it! Where is it?" |
| 7 | (Father action `0x401e60`) |   |       |            | turn/emote |
| 8 | Father                  | 0x1e | 0x3f1 | 0x86d42c   | "Mm-hmm. It's just down the next…" |
| 9 | Arche                   | …    | 0x3f2 | 0x86d424   | "Cool!" |
| … | camera/timer beats, then Mom "wait up", then Sana `0x5f5e166` lines, walk-off |

(Voice ids `0x3eb`–`0x3f4` map to `1_%07d.wma` clips — voice deferred.)  **The text
strings are STORY CONTENT in the user's `sotes.exe` `.rdata`; the port reads them from
the user's file at runtime by VA — never embedded in source** (dramatist-table precedent
+ the project legal line).

## Chips (incremental; bit-exact bar each)

1. **Ground truth.** Capture retail at a held dialogue line: box geometry (the 9 tile
   positions + extent), portrait (res+frame+x/y), name + text positions, font, colors.
   `runs/dialogue-*`.  (Task: capture.)
2. **`.rdata` string reader.**  `exe_rdata_string(VA)` → reads from the user's
   `sotes.unpacked`/`sotes.exe` (PE: VA→file-offset via section headers).  Host-tested
   against a couple of known VAs.  Keeps story text out of source.
3. **Static dialogue-box render** (the smallest visible win).  `src/dialogue.{c,h}`:
   compose ONE line (box frame `newgame_box` + name + text `glyph_render` + portrait) at
   the captured geometry; wire into `game_render` after the banner.  Verify `differ_px==0`
   vs the retail capture.  Portrait sheet registered (capture-pinned res).
4. **Typewriter + Z-advance.**  Reveal text char-by-char (interval from `DAT_008a6e80→+0x248`,
   speed-grades ×2/×3/×5); advance the line on Z (the input the port already reads); step
   the box state in the sim-tick block.  Drive the full ~15-line town script (a ported data
   table referencing text by VA).
5. **The beat-runner / cutscene driver** (`0x439690` + `0x4d7d80` as a port-side beat
   sequence).  Replaces the measured-constant fakes — RETIRES PORT-DEBT(banner-trigger),
   ingame-camera-pan trigger, ingame-letterbox source: the script writes the camera target,
   letterbox heights `+0x44/+0x48`, banner arm.  The arrival plays out (camera pan, family
   walk-in via `0x401e60`, dialogue) on the engine's schedule.
6. **Controllable Arche** (Phase 4, separate plan): the party band `0x4997b0` + the movement
   FSM `0x43f880` — after the cutscene ends, control hands to the player.

## Open / to-verify (resolve via the capture, chip 1)

- **Object model.**  `0x439680` tail-calls `0x439690` with no stack arg, so the runner's
  `param_1` (controller) and `in_ECX` (widget) are very likely the SAME `this` pointer
  (Ghidra split a `__thiscall`).  Model it as ONE scene-controller object holding both the
  beat/dialogue state (+0x20/+0x78/+0x7c/+0x84/+0x8a/+0x28a/+0x2e0/+0x2e8/+0x2f0/+0x57c)
  and the widget sub-pointers (+0x54..+0x74, +0x82).  Confirm before chip 5.
- **Portrait sheet.**  Which sprite resource is each speaker's portrait, and how `+0x84`
  (portrait id) + `DAT_008a760c` pick it.  Capture pins it.
- **The face table `DAT_006b6568`** contents (dump to a proof if needed for chip 4).
