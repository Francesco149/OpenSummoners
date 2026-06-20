# The errands-scene opening dialogue (ckpt 152)

The short dialogue that plays once the errands scene loads + the player gains control
(USER ckpt 152: "the opening dialogue for the errands scene — starts once it switches
to the errands scene where you have control").  It is the **movement tutorial**.

## What it is (RE'd off `0x4dc510` + retail.osr)

The errands questline script **`FUN_004dc510`** (the case-based scene runner; the dialogue
API `0x4a5ee0` is a thin wrapper → `FUN_0049d6e0`).  Its **entry case** (decompile
lines 1086-1116) plays **three Arche lines**, each a `FUN_0049d6e0(...)` followed by
`FUN_00439680()` (the beat pump — it runs the game loop, so the dialogue + freeroam
control coexist):

| # | line (`:VA`) | text | face |
|---|------|------|------|
| 1 | `:1088` (`0x86f414`) | "So the first floor of our house is the store. Cool!" | 0x02 |
| 2 | `:1102` (`0x86f3e8`) | "Okay, I need to help with the moving-in!" | 0x02 |
| 3 | `:1116` (`0x86f388`) | "To move around, I press [←][→] & [↑][↓], and to talk to people and do stuff, I press [Z/X], right?" | 0x09 |

- The call is `FUN_0049d6e0(0, speaker, text, 0, 1, 1, 0, FACE, 0, voice=0)`.  Param **8**
  (the script's `uVar10`) is the FACE (== the town chain's `uVar9`-face slot, just a
  different decompiler var); speaker = `FUN_00556eb0(0x5f5e165)` = **Arche**; voices 0.
- **`FUN_0049d6e0` IS the SAME dialogue display the town chain uses** (cutscene.c is built
  around it — "Ten 0x49d6e0 calls" arrival, "Eight" house).  So the errands opening is
  just 3 more lines through the existing box system — not a separate dialogue subsystem.

## Drawcall ground truth (retail.osr, ticks 1758-1837)

- Spoken by **Arche, who stands STATIC at the freeroam spawn** (res 0x570 @screen (162,336),
  idle breathing — control is live but the player doesn't move in the recording).
- The box is the SAME widget as the cutscene: a speaker bubble anchored to Arche, but
  **CLAMPED LEFT** (she is near x=32, so `0x49c640`'s `box_x = clamp(.., 0x20, ..) = 32`)
  → the wide bottom box at **(32,192)-(440,304)** with the tail at ~186 pointing down to
  her, the portrait bust (left), the name plate (312,160,127,48), the body (line-count
  distributed), and the advance arrow (400,284).  Name "Arche" @(332,184) color **0x455f7b**;
  body color **0xa8b9cc** main + **0x3e537d** shadow (font 3).
- **Line 3 carries inline button-icon art**: 3× 17x17 res=0 cels (retail seq 536-538) for
  the arrow keys / talk button, marked in the string by `@@<char>` codes.
- Plays AFTER the entry reveal (the fade-from-black recedes ~1725; line 1 first-glyph ~1758).

## The port (ckpt 152, commit `dded4c8`)

- `cutscene.c`: `TOWN_ERRANDS` line table (the 3 lines, faces 2/2/9, box anchored to the
  spawn world 19200,52000) + `ERRANDS_INTRO` 1-room chain + `cutscene_errands_intro()`.
- `main.c`: `g_errands_dlg_pending` set at chain-complete (next to `freeroam_begin()`);
  the deferred-arm check (before `cutscene_step`) re-arms `g_cutscene` with the errands
  chain once `!scene_fade_active` (the reveal receded) — it then renders + advances via the
  existing path while `freeroam_step` keeps control live.  All same-speaker so the box stays
  up across the advances.
- **VERIFIED** off `port-errdlg.osr` vs `retail.osr` (`tools/trace_studio2/dlg_reconstruct.py`;
  the tick-aligned `runs/cutscene-verify/nav-errands-dlg.jsonl` = nav-house-turn + 6 errands
  confirms at retail ticks 1763-1837): all 3 lines render at retail's ticks (L1@1770 /
  L2@1800 / L3@1830), name/box/colors/3-row-layout == retail EXACTLY.

## Residual

- **Line-3 inline button icons** render as raw codes (`@@©`/`@@¨`/`@@X`) — `PORT-DEBT(dialogue-
  arrow-art)`, the inline-art system (shared with the HUD's res=0 UI sprites + the arrival
  book/item glyphs).  Retail draws them as 17x17 sprites.  A shared follow-up.
- Retires the DIALOGUE half of `PORT-DEBT(cutscene-scene-chain)` (the errands reveal fade is
  already a main.c stand-in; the questline proper — quests/flags — stays unported).
