# probe combat-moves

run `combat-moves`; accepted 2026-11-14 07:25

check PASS (tools/drive/tasks/combat-moves.md)
verify MATCH (+ real device)
filed tests/scenarios/combat-moves/trace.jsonl (8577 frames)

## driver's report

The run `combat-moves` passes the task check and the replay matches, but only part of the combat checklist got exercised. The movie is 8577 frames; I stopped well short of the 30k frame target to stay within budget. I confirmed 5 Arche moves on an enemy and saw 3 more without hitting anything. Guard, the 5-hit combo, Sana's spells, healing, status effects and level-up were not done.

**Run checks**
- `rd check combat-moves tools/drive/tasks/combat-moves.md`: PASS. Step 1 (control after loading) at frame 605, step 2 (frame > 6000) at frame 6001.
- `rd verify`: MATCH. The live session had drifted by one volatile dword; that dump is kept in `runs/combat-moves/verify-drift-f8577`. A clean replay of all 8577 frames was used.
- Pictures: I looked at frames 5734, 7090 and 7159 (hits on a Mothbee) and 8186 (breaking a pile), among others.

**Route**
- Title (cursor was remembered on Bonus Menu): ↑ to Continue, `ent`, load slot 7 "Barness Village, Arche LV5" at frame 504.
- Starts in room 270200 (a Barness inn interior; the door is at lx about 19000, ↑). Village street is 270120.
- The village's far-left cave (280115) is blocked by a closed gate after about 50000 units, so I went back.
- Dashed right to 270110, took the ↑ exit at the far right (lx about 382000), and reached the field 260170 (Barness road) at frame 5379.
- In 260170 the enemies are Mothbees. The brown round "chestnut" piles are breakable coin objects, not enemies.

**Moves (move ID is the value of `lpose`)**

| Move | Input | ID | What happened |
|---|---|---|---|
| Slash | X | 10101 | Killed Mothbee #1 at frame 6276 (4 → 0 HP) and Mothbee #2 at 7309 (11 → 0) |
| Wide Slash | →+X | 10109 | Hit at 7090 for 18 (47 → 29); picture checked |
| Arcing Slash | ↑+X | 10110 | Hit at 7159 for 18 (29 → 11); picture checked. Earlier ↑+X hits at 5764 and 5800 (37 → 22 → 4), ID not recorded there |
| Combo slash (Slash, then ↑+X) | X, then ↑+X | 10111 (6291) and 10104 (7229) | Two different IDs; I could not tell which one is "Combo Slash". Neither hit |
| Downstab | ↓+X while falling | 10103 | At 7118 a Mothbee hit interrupted it; clean at 8504 with no enemy |
| Thrust | ↓+X on the ground | 10102 | At 8206, no enemy |
| Roll | ↓, release, → | 10108 | At 8540; moves about 9600 units. `D*2 R*2` and `D . R` did not roll; `D*3 DR*6` did |
| Cartwheel, Jumpkick, Wheel Slash | ↓ then ←; →↑X; →↓X | — | My attempts produced Slash or nothing |
| Jump attack | jump, X | — | Not confirmed: an X pressed as the jump started gave 10101 |

All 20 move IDs Arche has: 10101–10118, 10120 and 10122. The table is at character +0x754, one record per 0x300 bytes, ID at +0, count at +0xc754.

**Other checklist items**
- **Manual guard (↑):** not shown. Holding ↑ while idle only gives the look-up state (body state 5). I did not open the Auto-guard option.
- **Combo of 5 or more hits:** not reached; solo Arche's recovery is slow and enemies were few.
- **Switching to Sana and casting:** not possible here. The party is only Arche (plus Chiffon), as Day 6 has Sana sick.
- **F and F2 healing:** both did nothing. The inventory (F1 → Items) has no healing items, only equipment and stat drops (Power/Good Child/Guardian Drop, Mystery Juice).
- **Status effect:** none seen. The Mothbee's yellow sparkle attack only did damage (15, 8, 14 and so on), and `rd text` showed no message.
- **Level-up:** none; I got +19 EXP per Mothbee.
- **Pits:** falling in costs 8 HP and puts you back at the last ledge (seen 4 times; frames 7325, 7791, 7854, 8040).

**Named state (defined in this run)**
- `lparty` = `s32(s32(lslot+0x9f4)+0x750)`: the leader's stat block.
- `lhp` = `s32(lparty+0x54)` (max HP at +0x58) and `lmp` = `+0x5c` (max MP at +0x60). The HUD showed 172/172 and 27/27. A hit of 15 shown on screen took `lhp` from 172 to 157.
- `foe` = `s32(world+0x2774)`: 1 when an enemy is near. It turned 1 when the off-screen enemy marker appeared.
- `lpose` = `u16(lchar+0x48)`: the current move ID.
- Enemies are in `world+0x1160[i]` (32 character slots). Code is at character +0x1d4 (Mothbee 51025, breakable pile 51051, Arche 50010, Chiffon 50013). HP is `s32(s32(char+0x750)+0x54)`; x and y are `s32(s32(char+0x40)+4)` and `+8`.
- `T` (slot index), `ex`, `ey`, `ehp` read the enemy in slot T. Checked: Mothbee slot 6 went 49 → 37 when a 12 appeared on screen.
- Facing is `s32(lchar+0x2c)` (1 right, 3 left); body state is `u16(lchar+0x38)`.
- Not found: the combo counter, and which enemy is currently targeted (`lchar+0x50` read 0). `ram diff` of a hit gave 155k changed dwords, too much noise to use.

**Game facts [RD] (corrections for docs/game)**
- The title remembers the last pick: the cursor started on Bonus Menu.
- An attack key pressed during an attack's recovery is lost, and a jump needs a fresh press. While recovering, C did nothing.
- Wide Slash and Slash lunge forward 1600–3000 units, enough to carry Arche into a pit.
- Being hit knocks Arche back and makes inputs do nothing for about 30 frames.
- F and F2 do nothing silently when you carry no healing item.
- The coin piles in 260170 can be broken for coins (903 → 909).

**Tool gaps**
- Expressions have no `hex()`, no list/sum comprehensions, and no way to loop over the character slots. A "nearest live enemy" or "targeted enemy" built-in variable would have saved most of my steps.
- `rd search --score` with `--goal` accepted an arrival while Arche was in mid-air above a pit, which then cost HP. A "standing on the ground" option for search would fix this.
- `rd scan` replays from boot (12–17 s), which is fine. Saved checkpoints stop working once the movie is cut.
- The status line now shows `gold`, `hp` and `party` names that I did not define. They seem to come from a shared variables update made during the run.

---
model claude-opus-5-5, effort high, 181 turns, $3.40, 32 min

