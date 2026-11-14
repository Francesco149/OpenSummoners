# probe bonus-menu

run `bonus-menu`; accepted 2026-11-14 14:44

check PASS (tools/drive/tasks/bonus-menu.md)
verify MATCH (+ real device)
filed tests/scenarios/bonus-menu/trace.jsonl (12459 frames)

## driver's report

Run `bonus-menu` from `end:rh-water-shrine`, 12459 frames. `rd check` PASS (step 1 judged at f1501, step 2 at f5001, forbid never triggered); `rd verify` MATCH (rng 1dd38522). Ends on the title (f12459). I did not open Continue first.

What unlocked it: with the rh-water-shrine saves (Prologue Clear Data slot), both Bonus modes open with no message and no code. On a fresh install they say "You cannot access this menu yet." The Versus help text says: "Available characters will increase as you proceed in Story Mode."

Screens in order:
1. f576, title: the cursor (tcur) was already on Bonus Menu (2), not Start; the title remembers the last pick. `ent` pressed at f606.
2. f687, Bonus Menu: Versus Mode / Auto-Play Mode / Back. The cursor was remembered on Auto-Play. Help lines:
   - Auto-Play: "Auto-Play Mode allows you to watch the computer control all the characters."
   - Versus: "Select your favorite characters and face off in Versus Mode. Available characters will increase as you proceed in Story Mode."
3. f749, Vs. Mode Settings. Rows: Team 1 Character 1 / Character 2 / AI Strength / Damage Handicap, the same four for Team 2, then Stage, Mode, Start. Defaults: Arche Level 3, Don't Participate, 3: Normal, 100%, Behind School, Player vs. AI. The cursor opens on Start ("Begin Vs. Mode.").
   - f931-979, character picker ("Select Character One."), 4 pages turned with L/R, `ent` picks:
     - p1: Arche Level 3, then ????x5
     - p2: Sana Level 4, then ????x5
     - p3: ????x6
     - p4: Don't Participate (greyed for Character 1)
     - I picked Sana Level 4 for Team 1.
   - f1099, AI Strength ("Alter the capabilities of the AI."): 1: Punchin' Bag, 2: Weak, 3: Normal, 4: Strong, 5: Takin' Names.
   - f1164, Damage Handicap (help: "Alter the levels of damage a te[am]... is half damage."): 100% down to 10% in steps of 10.
   - f1281, Stage ("Select the stage to fight in."): only Behind School.
   - f1346, Mode ("Choose either Player vs. AI or AI vs. AI."): those two.
4. f1454, Versus match started: AI vs AI, Sana Lv4 (Team 1) vs Arche Lv3 (Team 2), room 200210, "TIME 00:00:00" timer.
   - Sana is the leader (100 HP); Arche has 140 HP, in character slot world+0x116c with faction 2.
   - Sana mostly healed herself; Arche was still 139/140 at f5131 and later got down to 54 HP. Sana fell at about f7631.
   - The screen read "Sana has fallen...", the timer stopped at 01:35:69, and the game faded out.
   - f8028: straight back to Vs. Mode Settings, no results screen.
   - During the match f1, esc, ent, bk, sp, f2 and f3 did nothing (no menu, control stayed 1). I found no way to quit a match early.
5. f8089: `bk` went back to the Bonus Menu. That press changed the RNG (9fc0b987), like saving options does.
6. f8151, Auto-play Settings: Arche's / Sana's / Stella's Level (Level 3 each), Default Settings, Start ("Begin Auto-Play.").
   - The level picker (f8234, "Select the level of the characters in autoplay mode.") offers No Participant and Level 1-4. I set Arche to Level 4.
   - The cursor wraps (4xU from Start lands on Arche's Level).
7. f8411, Auto-play running: "Fortune Summoners Autoplay Mode / The Game Play by Computer operator only. / - Please Hit Any Key -".
   - Full party of three, all computer-controlled ("COM" tags). Leader Arche Lv4, 160 HP.
   - It moved through rooms 200110 -> 200120 (f9161) -> 200150 (f10661, a cave), killing enemies on the way (gold 0 -> 14, HP 160 -> 128).
   - One X at f10911 ended it within 2 frames and returned to Auto-play Settings.
8. Back to the title: `bk` then Back. The studio intro replayed; the title was up at f12419 (tcount 5, tcur 2).

New state found [RD]: Vs room 200210 (Behind School); Auto-Play rooms 200110 / 200120 / 200150. In both modes, character HP is readable through the card's character-slot layout.

Tool gaps:
- `rd text` cut the Vs. Mode Settings list off after about 10 rows (once at "Arche Le") and did not capture the pop-up pickers; I read those from pictures.
- `rd eval` cannot take list comprehensions, so all character slots cannot be scanned in one expression.
- `grep` refused the IDA export all_ida.c ("not searchable here"), so I could not look up the Vs-mode exit in the code.
- No task-level variable for "inside a Bonus sub-screen" (tcur is -1 there and world is 0).

---
model claude-opus-5-5, effort high, 75 turns, $0.83, 7 min

