# probe options-deep

run `options-deep`; accepted 2026-11-14 10:03

check PASS (tools/drive/tasks/options-deep.md)
verify MATCH (+ real device)
filed tests/scenarios/options-deep/trace.jsonl (12008 frames)

## driver's report

## options-deep: report

I exercised every Options page from the Day 5 save and put every setting back. Two results to note first:
- **"This button is already bound." never appeared.** On the keyboard page, binding a key that is already in use swaps the two bindings instead.
- **The starting Message Speed is 5 (Fast), not the default 3.** I restored it to 5. The Graphics page's "Default Settings" row would set it to 3, so I did not use that row.

**Run:** `options-deep`, started with `--from end:rh-water-shrine`, 12008 frames.
- `rd check`: **PASS**. Step 1 holds at frame 889 and step 2 at 6001. The forbid ("back at the title") never fired.
- `rd verify`: **MATCH** at frame 12008, rng a9a6b5b1.
- **Ending state:** in the field at house 210140. Keyboard bindings are the defaults (C jumps again, frame 11946). Every option is back at its starting value; final picture at 12008.

### Loading
- Continue opened the load screen with the cursor already on slot 7, "Town of Tonkiness 3:25, Arche LV4". That slot has the longest play time, but its date stamp is not the newest.
- I loaded it at frame 257. A wake-up line ("Mmmmm! That was a good sleep!") plays, then free control at 888 in 210140, Arche alone.
- Each visit takes F1 → down ×6 to System → Options. The Main Menu cursor resets to Items every time F1 opens it.

### Graphics ("Options: Screen Settings")
Every value is chosen from a pop-up list opened with Enter; ←/→ do nothing. `C` saves and exits, and saving draws the RNG (visible as an rng change). Starting values: High / 7 / 7 / 5 Fast / Yes / No / No.

| Setting changed | Saved and looked at | What changed on screen |
|---|---|---|
| Brightness 10 | 1403 | Only slightly brighter; hard to see |
| Brightness 0 | 2054 | Field clearly darker. The HUD and menus are not affected |
| Brightness back to 7, Contrast 0 | 2718 | Washed out and flat: pale sky, dull greens |
| Contrast back to 7, Graphics Quality Low | 3304 | No visible difference in this still room |
| Quality back to High, Display FPS Yes | 3864 | "FPS 30" appears at the bottom right |
| Display FPS No | 4336 | The counter is gone |
| Display Combo Meter No, Magic Circle Display Yes | set at 7132 | Not observable: no enemies here, and Arche has no spells |

Combo Meter and Magic Circle went back to Yes / No at 11675.

**Message Speed:** I timed the same line, Mom "Mom, I'm off to school!", counting frames from when the box opened. `rd text` shows the part typed so far.

| Speed | Box opened | +30 frames | +70 frames | Line complete by |
|---|---|---|---|---|
| 5 (Fast) | 5314 | "Mom," | "Mom, I'm off to sch" | +130 |
| 1 (Slow) | 6358 | "Mo" | "Mom, I'" | +170 |

Speed 5 was restored at 7132.

### Game & Control
- **Auto-guard Off:** saved about frame 6300. It still read Off when I reopened the page at 7264. I did not test it against an enemy.
- **Magic Quick Select:** the list is Magic Book Select Mode / Direct Magic Select Mode / Don't Use. I set Direct; there was nothing to observe without spells.
- **Dash Input Sensitivity:** tested with `R .*14 R*16` (a 14-frame gap between the two taps).

| Sensitivity | Frames | Result |
|---|---|---|
| 1 (Short) | 7626 | Walks, about 170 lx per frame while speeding up |
| 1 (Short), 8-frame gap | 7620 | Still dashes |
| 4 (restored) | 8342 | Dashes, about 360 lx per frame |

All four rows were back to their starting values at 8138.

### Keyboard Setup
There are 3 pages; ←/→ change page. Page 3 holds Default Settings, Cancel Changes and Save and Exit.
- **8734:** Jump ← X swapped the two keys (Jump = X, Attack = C).
- **8786:** Jump ← Num1. Select Spell 1 took the leftover X (seen at 9049).
- Pressing Jump's own key cleared it ("----"). Save and Exit then showed **"Jump, Attack, and Magic Select are not bound."** at 8997.
- **9127:** Default Settings asks **"Resetting to defaults. Are you sure?"** (Yes/No). Yes put back exactly the starting bindings (9200).
- **9252:** Open Menu ← V also swapped (Examine became "----").
- **9338:** Cancel Changes asks **"Exiting without saving. Are you sure?"** Yes threw the change away.
- **9452:** Jump ← Spacebar, saved at 9616. After that:
  - C no longer backs out of menus (9647); Space does.
  - In the field, `C*12` does nothing (9822) and `sp*12` jumps, top of the jump at ly 24380 (frame 9842).
- **Restore:** Default Settings → Yes, then Save and Exit, around 10294–10330.

### Sound, Gamepad, closing
- **Sound:** I set Music Off, Sound 3, Voice 1, Message 5 (10841) and saved. They were still there on reopening (10942). I restored 3 / 5 / 3 / 3 at 11346 and saved.
- **Gamepad Setup and Analog Gamepad Setup:** greyed. The cursor can sit on them and they show their help text ("Set up gamepad button layout." / "...analog gamepads connected to your PC."), but Enter does nothing (11776, 11820).
- **Closing:** it takes 4 C presses to get from an option page back to the field. X acts as confirm in menus: an early X reopened System → Options → Game & Control by accident.

### Game facts [RD]
- Brightness and Contrast change only the field picture; the HUD and menu windows are drawn unchanged.
- The FPS counter reads 30.
- Keyboard rebinding never refuses a duplicate key. It swaps, even onto an unbound row, so "This button is already bound." is probably a gamepad-only message (untested, no pad).
- The Jump key is also the menu-cancel key.
- Talking to Mom worked from lx 61944 while she stood around lx 65700; it failed while Arche was still sliding.

### Tool gaps
- `control` reads 1 while the F1 menu is open; a menu-open variable is needed.
- `ram diff` cannot filter by value (for example "changed 7 → 10"), and its output stops at 200 lines, so I could not find where the settings are stored.
- `rd text` shows a line's typed-so-far part, which was useful, but there is no expression for "the line has finished typing."
- Once, `{toward(68500 - lx, 300)}` left Arche further right than she started, possibly because of dash momentum; not investigated.

---
model claude-opus-5-5, effort high, 185 turns, $3.36, 21 min

