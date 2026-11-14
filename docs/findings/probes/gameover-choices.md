# probe gameover-choices

run `gameover-choices`; accepted 2026-11-14 01:02

check PASS (tools/drive/tasks/gameover-choices.md)
verify MATCH (+ real device)
filed tests/scenarios/gameover-choices/trace.jsonl (21606 frames)

## driver's report

I got the party wiped in a real fight and looked at both game-over choices, but the task does not pass. `rd check` FAILs on its own forbid, and `rd verify` says MISMATCH.

- **Check:** the game-over screen itself has `world == 0` (the world singleton is freed there). So "back at the title: world == 0 and frame > 6000" fires on the game-over screen (frames 21318–21506, 189 frames). It would fire for any wipe after frame 6000, and a level-4 party cannot be wiped before then. The forbid needs to exclude the game-over screen.
- **Verify:** frame and RNG agree in all three passes (live, headless replay, clean rebuild: 21606, rng 890d11a5), but the semantic hash differs in all three (67b0…, 6dd4…, 3446…). The tool could not collect evidence of why. My guess is the saves the game writes along the way (the defeat save 10030, the options save), but that is not confirmed.

**Run:** gameover-choices, 21606 frames, from end:rh-after-prologue.
- Step 1 (in a room with control) at frame 415; step 2 at frame 4001.
- Checkpoints: field@2689, expert@7762, slimes@8799, sana-off@12684, arche-low@19754, arche-fallen@20128, gameover@21437. The movie ends on the Return To Safety branch.

**Route:**
1. On the title, Continue → slot 6 ("Town of Tonkiness, Arche LV4 / Sana LV4", 2:48, the newest). It loads into the Tonkiness inn, room 210150. Messages shows Day 5 ("No.8 From the Weapon Shop").
2. Out of the inn, then the gate on the far left of 210110 (↑ at x≈15000) leads to field 260110.
3. System → Options → Game & Control: Difficulty 4:Expert, Auto-guard Off, Save and Exit. Difficulty is stored at world+0x2780 (10 Easy, 40 Expert) and took effect after leaving and re-entering the field.
4. Abilities → Sana: Water Shoot, Diamond Dust, Heal Water, Shield and Sleep Mist all set to Don't Use. Her Heal Water had healed Arche for 95, which made a wipe impossible.
5. Left the field and came back (the Green Slime came back at about x 105000, low ground). Pressed F3 (an "ESCAPE" party-order icon appeared) and walked Arche into the slime without attacking. Expert slime hits did 4–10 damage.
6. Frame ~20088: "Arche has fallen..." (seen on screen at 20128). Control passes to Sana automatically (PLY Sana).
7. Sana walked into the slime and fell at 20994. The Game Over screen appeared at about 21437 with "Try Again" and "Return To Safety (-10% Money)", cursor on Try Again.

**What each choice does (both looked at):**
- **Try Again** (from checkpoint gameover): room 210110 at the town gate (x 12800), Arche leading. HP 123/69, which is exactly what the party had when it last came through the field door at 15555–15682; Arche is no longer fallen. Gold 116 is unchanged. The defeat counter went 3→4.
- **Return To Safety** (after loading the checkpoint, D then X): the same room and spot, but **Sana leading** (the leader at the time of the wipe). HP is full (161/100) and gold went 116→105 (−11 = trunc(10%)). Counter 3→4.
- **System → Give Up** (tried earlier and cut from the movie): needs a Yes/No confirm, where the cursor starts on No. The game-over screen then shows **only** Return To Safety, no Try Again. Result: gold 113→102, healed, back at the 210110 gate.

**Decompile:**
- The game-over menu is **sub_59EE50** (0x59ee50). It fades in, builds a menu box with "Try Again" (id 22) if a3 and "Return To Safety (-10% Money)" (id 23) if a2, and returns the picked id.
- Its caller **sub_59EC30** runs the session (sub_59F2C0):
  - Result 4 or 5 means a defeat. If the result is 5 and a retry point exists, Try Again is hidden; this is the Give Up case.
  - Choice 22 reloads save 10020 (the snapshot taken on area entry). Choice 23 loads save 10030.
- On a defeat, sub_59F2C0 (around 0x5a0xxx) checks for a retry point (sub_5A3BF0). If there is one it heals everyone (sub_4E1950), takes 10% of the money (sub_419C00 with −100·gold/1000), increments rm+0x40b0, shows sub_59CFC0 and writes save 10030.
- The "%s has fallen" line comes from status inflict 0x4593e0, status 1.

**State names (defined in this run; checked against the Party Member Status panel and the HUD):**
- `rmo` = s32(world+0x2784): the room/party record.
- `gold` = s32(rmo+0x4014), in CP (113 matched "113CP (Max 6000)").
- `hp0` = s32(s32(rmo+0x4030)+0xa64) = Arche's HP (161/161); `hp1` = s32(s32(rmo+0x4034)+0xa64) = Sana's HP (100). Party slots are rmo+0x4030 + 4·i; slot+0x9c4 == 1 means in the party. Max HP = +0xa68 + +0xa94 + +0xaac.
- `retry` = s32(rmo+0x40d0), the retry room (210110 here).
- `defeats` = s32(rmo+0x40b0), the defeat counter (3 in the save, 4 after a wipe).
- `diff` = s32(world+0x2780): 10 Easy … 40 Expert.
- During game over `world == 0`, so all of these read 0 or −1 there.

**Game facts and guide corrections (EN-old):**
- Main Menu order is Items, Special Items, Status, Abilities, Party Commands, Messages, System.
- System holds Options, Quick-heal, Money Display, Give Up, Return to Title. Give Up's help text: "Cannot be used in safe places such as towns."
- On Game & Control Setup, Right does not change a value. X opens a picker (Game Difficulty: 1:Easy … 4:Expert).
- On the Abilities page, AI members follow book settings; "AI characters will not use spells in book C".
- Try Again does **not** cost money, and it restores the last area-entry snapshot rather than healing. mechanics.md §10's "likely retry from area entry" is about right, but it should say no heal and no money loss.
- The −10% is truncated.
- Green Slimes killed in field 260110 stay dead until you leave and re-enter.
- On Easy with Auto-guard On, a slime did 1 damage in 400 frames, so a wipe needs Expert, no auto-guard and Sana's healing switched off.

**Not done:** I did not test reviving a fallen member (Revival Drink, inn or statue).

**Tool gaps:**
- The game exited ("game-exit") early in one `rd search` (98 frames in), cause unknown.
- I lost a lot of time to sessions being rebuilt by replay (30–50 s each).

---
model claude-opus-5-5, effort high, 152 turns, $2.95, 43 min

