---
name: options-deep
start: end:rh-water-shrine
steps:
  - loaded into a room with control: world != 0 and control
  - worked the options for a while: frame > 6000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Exercise probe: Options — every setting's effect and every error / confirm path (docs/game/exercise-checklist.md §A,
docs/game/mechanics.md §11).** Start from the Day 5 save (Continue → newest slot), so effects show in the field. Open
Options in game (F1 → System → Options). Read `docs/findings/probes/title-options-bonus.md` and `main-menu-pages.md` first.

1. **Graphics Settings:** change Brightness, Contrast, Graphics Quality, Display FPS one at a time, `Save Changes & Exit`,
   and LOOK at the field after each (`rd look`) — say what changed on screen. Put each back afterwards.
2. **Game & Control Setup:** Auto-guard OFF → stand next to an enemy or not (just confirm the setting sticks), Message Speed
   (talk to someone: compare how many frames a line takes), Display Combo Meter, Magic Circle Display, Dash Input
   Sensitivity, Magic Quick Select — change, save, observe if possible, restore.
3. **Keyboard Setup:** rebind one key to another and confirm the game uses it (a move with the new key); clear Jump →
   expect "Jump, Attack, and Magic Select are not bound."; bind a key already used → "This button is already bound.";
   `Reset to Defaults` (confirm prompt) and `Exit Without Saving` (confirm prompt). End with the default bindings.
4. **Sound Settings:** move each slider and save; restore.
5. Gamepad / Analog Gamepad Setup pages: open, look, leave (no pad here).

The game's key settings persist in the config file: the movie must end with every setting restored, and with the default
keyboard bindings (otherwise later scenarios starting from this one would play with your keys). Use the menu pacing in
DRIVER.md (~30 frames after anything opens, ~12 between presses). Do not read other runs' movies. Stop around 20k frames.
Report: run, frames, each setting / message with its frame and what changed on screen, game facts [RD], tool gaps.
