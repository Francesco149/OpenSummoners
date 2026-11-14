# probe newgame-config

run `newgame-config`; accepted 2026-11-14 12:18

check PASS (tools/drive/tasks/newgame-config.md)
verify MATCH (+ real device)
filed tests/scenarios/newgame-config/trace.jsonl (6999 frames)

## driver's report

The run `newgame-config` is done: a 6999-frame movie that starts from a fresh install. `rd check` gives PASS (step 1 "a game started" at frame 4487, step 2 "played into the prologue" at frame 5001) and `rd verify` gives MATCH (rng e39b05da). Nothing was poked.

**Answers**
- **Nightmare:** the difficulty picker lists only 1:Easy, 2:Normal, 3:Hard, 4:Expert. 5:Nightmare is not shown at all; it is not listed as locked. The cursor wraps from Expert back to Easy.
- **Hard:** stored as 30. The scale is 10 Easy, 20 Normal (going by the decompile's value list), 30 Hard, 40 Expert.
- **Auto-guard OFF:** stored as 0 (On = 1).
- **Took effect:** in game, Options → Game & Control shows 3:Hard and Auto-guard Off. Leaving through Cancel Changes kept both.

**Each screen**
- **f576:** title menu up (`tcount == 5`). `ent` at about f606; the config box was readable at f707 (picture look_f000707). Defaults: 1:Easy, Auto-guard On.
- **f738:** difficulty picker open (look_f000738). Easy, Normal and Expert were also pictured at f738, f754, f786, and the wrap back to Easy at f807.
- **Help text:** it belongs to the row, not to the entry.
  - Game Difficulty: "Allows you to configure game difficulty. On harder difficulties, enemies are stronger and experience gain more rapid." The same text shows for all four entries.
  - Auto-guard: "Auto-guard will automatically guard against incoming attacks if your character is not engaging in any other action."
  - Start Game: "Confirm options and begin the game. (Options can be altered after the game starts.)"
- **Closing the picker:** `bk` closes it without changing the value (f828).
- **Auto-guard picker (f875):** two rows, "Off" above "On", and it opens on the current value. Changes: Off at f922, On at f1000, Off at f1078.
- **f1188:** 3:Hard picked. At f1220 the box showed 3:Hard / Off / Start Game (look_f001220). Start Game pressed at f1221.
- **f4221:** prologue text on a black screen ("In the country of Scotsholm…"). It needs `ent` presses to go on; with no input the world was still 0 at f4221. The world appeared at f4486 in room 210110.
- **f6440:** first steady free control, in 210140 (the house), as Arche with 100/100 HP (look_f006470). `control` was briefly 1 at f5769 during a black fade in 210110; that was not real control.
- **f6501:** F1. **f6610:** System submenu (Options / Quick-heal / Money Display / Give Up / Return to Title). Its help text says difficulty changes "take effect once you change areas".
- **f6651:** Options. **f6692:** Game & Control (look_f006692): Game Difficulty 3:Hard, Auto-guard Off, Magic Quick Select "Magic Book Select Mode", Dash Input Sensitivity "4 (Semi-Long)", Default Settings, Cancel Changes, Save and Exit. The in-game difficulty picker opens on Hard (f6723) and also has only 4 entries.
- **f6855:** Cancel Changes ("Exit without saving.") asks "Exiting without saving. Are you sure?" with Yes as the default. Yes at about f6856 returned to the Options list (f6896).
- **f6999:** `bk` ×3 back to the field (look_f006999). Values still 30 / 0.

**Values in memory [RD]**
- The config object is `cfg = u32(u32(0x8a6e80))`, 0x20011dd0 in this run. It holds the dash window: `cfg+0xf8` = 800.
- The settings record for row id is at `cfg + 0xc + id*0x1c`:
  - Difficulty = `cfg+0x60`: 10 at f707, 30 after Start.
  - Auto-guard = `cfg+0x7c`: 1 at f707, 0 at f922, 0 after Start.
- The Nightmare unlock flag `cfg+0xaa4` = 0 on a fresh install, which matches the picker having no 5th entry.
- The world copy `diff` = `s32(world+0x2780)` = 30 from the moment the world exists (f4486).
- I defined two vars in this run: `cfgdiff` = `s32(u32(u32(0x8a6e80))+0x60)` and `autoguard` = `s32(u32(u32(0x8a6e80))+0x7c)`. Worth adding to vars.json.
- `rd ram mark` / `diff` across Start was no use: 585k dwords changed in the heap, and in the image range only the rng changed. The settings are written at commit time in the box, not at Start, so comparing against earlier frames (`eval --at`) was what worked.

**Corrections to the docs**
- The `lid` var reads 0 for Arche in the prologue house; slot+4 is 0 there. Don't use `lid != 0` as a "control" test early in the game. I lost one 3150-frame step to this and undid it.
- DRIVER's "C saves and exits" warning holds; the safe exit is the Cancel Changes row, then Yes.

**Tool gaps**
- `rd text` does not capture the entries of the option pickers (difficulty values, the "On" row) or the System submenu items, so pictures were needed.
- On the Game & Control page after a picker closes, `rd text` gives garbled two-letter fragments ("Op", "pt", …) and a truncated "Of".
- `ram diff` has no way to diff only one heap object, such as a range relative to a pointer.

---
model claude-opus-5-5, effort high, 74 turns, $0.84, 5 min

