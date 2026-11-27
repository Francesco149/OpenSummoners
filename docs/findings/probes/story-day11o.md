# probe story-day11o

run `story-d11o`; accepted 2026-11-27 17:47

check PASS (tools/drive/tasks/story-day11o.md)
verify: by the batch (the leaf movie of its chain: `rhost.py legs`)
filed tests/scenarios/story-day11o/trace.jsonl (59973 frames)

## driver's report

# story-d11o report: the final leg is done

**Run `story-d11o` beat Dark Witch Selene, played the whole ending and saved the Game Clear Data.** It ends on the title screen at **f59973**, about 59k frames past the start at f886. That is over the ~40k guide: the boss fight ran f20408–f31608, and the ending plus credits took about 28k more frames.

- **Gates:**
  - `rd check`: **PASS**. Step 1 at f887, step 2 at f1526, no forbid hit.
  - `rd verify`: **MATCH** (frame 59973, rng 9cbeb48d, semv 89b5000a2ce65fc4, fb 1e8108bcf513d02a).
- **End save: `9b1a5151b929`**. Slot **18** is now "Game Clear Data". It overwrote the old "Save Point" save, which had the shortest play time (6:23) among the slots I was allowed to replace. Slots 1–17 and 24 are untouched. Next run: `rd new NEXT --from save:9b1a5151b929`.
- **Class president's number: 9792.**
- **Session:** stopped. No repo edits and no commits.

## Route
| frame | what |
|---|---|
| 886 | Load slot 17 (title → Continue → page 3 → slot 17); free roam in 450295 |
| 1525 | 450295 → 450290. Unison cast at f1755 |
| 2703 | 450290 bottom trap killed (51122, 51242, 51111) |
| 3213 | → junction 450270 |
| 6186–6745 | Junction: killed 2× 51052 and 2× 51122 at the stairs, then had to bring a wandering Stella back. **z2 (102400, 73600) needs ↑** → **450300** |
| 7139 | Sana Heal Water II, then drop right |
| 7941 | **Lock 70217 (113600, 60800)**: "You used Pitch Black Key. The Pitch Black Key has broken." The gate pillar is gone |
| 8033–10752 | 4 bridge foes killed (51111, 51262, 51242, 51122 at x 200–235k) |
| 11977 | Unison recast |
| ~15330 | **[Memory Photo 8]**: chest 70220 at (347200, 12800), on top of the high slab |
| 16117 | 450300 z2 (451200, 38400) → **450310** |
| 17605 | → **450320**: cutscene with Selene |
| **20408** | Fight starts (checkpoint `boss0`) |
| **~31608** | **Selene defeated**, EXP +16560. "Victory!" by f32400 |

## Selene fight
- **Her stats:** character slot 4, code **100174**, **5256 HP**, mostly flying at sely ≈ 18–23k while the floor is at ly 32800.
- **What she did:**
  - At the start she stands on the ground casting from a magic circle.
  - A rain of light beams.
  - A big **dark ring** that sweeps through the party for about 40–60 damage per hit. This was the main killer.
  - Summons: a Black Panther and Skeletal Sergeants.
  - Something put Sana to sleep (Zzz).
  - A "DOWN" status icon appeared at the top right.
- **What failed:**
  - Long single searches with Arche leading. The AI members got worn down and the searches ended in doomed states ("1 cell only").
  - F2 quick-heals: each takes ~250 frames, and it used a Best Herb on Arche.
  - Arche-led searches with Sana's AI doing the healing let members drop to about 3 HP.
- **What worked:**
  - **Short Arche search segments**: goal Selene −300 HP; avoid any member under 90; score = −(summons' HP)×2 − Selene's HP + the three members' HP. Actions: move toward her, jump + **The Full Moon** (`C*12 UX`), Wind Thrust, guard, roll.
  - **Alternated with a Sana-led heal phase whenever any member fell under 180**: switch to Sana with `ent`, repeat `n8 .*5 X*80 .*20` until everyone is at 220+, then switch back to Arche.
  - That took her from 3719 to 988 HP in about 4500 frames. Two final pushes from her last 643 HP each allowed a member to drop to 45, then 40.
  - Sana's AI cast Mirror Image on Arche by itself.
  - **Heal-only test:** I set F4 magic usage to heal-only (medpack icon) on a branch I later abandoned, so it is not in the final movie.
- **Damage rate:** ~350–400 HP per 800–1000 frames with Arche leading; the AI party alone does ~300 HP per 1300 frames.
- **Low points in the movie:** no one fell, but Sana dropped to 40 near the end.

## Ending sequence
1. In 450320: Selene's "dimensional nexus" lines, then the unknown voice ("????").
2. The nexus collapses; the party runs to 450310 and falls.
3. Nurse's room 230150: Ms. Remi, the classmates, Ms. Sophia.
4. Home 210140: talk with Mom and Dad.
5. Narration ending "The End (...?)".
6. Credits with world = 0, about 10.5k frames (f43900–54754). I pressed no keys during them.
7. Classroom 230140: the class president gives the number **9792**.
8. "Would you like to save your victory data?" (Yes / No) → save-slot screen → slot 18 → "Overwrite this save?" (Yes) → "Game saved." The slot name becomes "Game Clear Data".
9. "Several bonus menu items have been unlocked." / "The Bonus Menu has been expanded."
10. Studio intro, then the title at f59943.

## Party at the end
- **Levels:** the save list shows Arche Lv19, Sana Lv19, Stella **Lv19**. Stella was already Lv19 in slot 17, not 18 as the d11n report said. I did not re-read the levels after Selene's EXP.
- **Equipment:** unchanged from d11n. No shops or gear on this stretch.
- **Money:** 38973 CP.
- **Items:**
  - Pitch Black Key used up.
  - Memory Photo 8 gained.
  - At least one Best Herb and one or two Better Herbs used (F2).
  - Revival Drinks and Magic Candy not used.
- **Skipped:** a Magic Candy for Arche (she sat at 37/57 MP but had no MP spender besides Unison).

## Game facts and walkthrough corrections (EN-old)
- **450300 (7.10):**
  - **Gargoyles 70410 at x 33600 and 312000 are not save statues.** There is no statue or fountain between fountain 3 and Selene, so there is no pre-boss save.
  - The "drop right" from the starting ledge (y 28800) falls into a pit (HP loss, back to the ledge). A search found a dash/jump down to the lock floor (ly 61600).
  - Lock 70217 at 113600, gate 70230 at 119000.
  - **Brick platforms:** blocks at x 339–352k, 368–381k and 394–406k; the slab x 336–406k carries the chest. The search reached the slab top (ly 10400) with Unison boost jumps; the path was long (~3200 frames).
  - Exit z2 (451200, 38400) on ↑ → 450310.
- **450310:** a gargoyle bridge (more 70410s, not statues). Event zone 101 at x 192000 did not fire. z2 (252800, 19200) → **450320**, where the cutscene starts on entry.
- **Title cursor:** this boot's title remembered **Bonus Menu** (tcur 2). Don't press `ent` blindly at the title.
- **The `dlg` state doesn't flag the pre-fight text boxes.** Use `script`.

## Named state (run-local)
- `m0`–`m2` party HP; `phmin` (status) their minimum.
- `e0`–`e23` hostile HP per slot; `efoe` / `nfoe` totals.
- `sel` = e4 (Selene's HP); `selx` / `sely` her position.
- `chest` (450300 chest state).

## Tool gaps
- **`efoe` (24 terms) is too long for a `--score`**: 1951 tokens against the 598 limit. Use a few slots instead.
- **Boss searches ran at only 50–130 ticks/s**, and long searches drift into doomed states. A shell loop (short search segments plus a scripted heal phase) worked much better than one long search. A built-in "segment + recovery action" mode would help.
- **The savestate pool ran out at ~16 checkpoints;** I dropped old ones.
- **Shell calls are moved to the background after 180 s,** so long `rd search` calls need `run_in_background` plus a wait.
- **No zoomed HUD view:** reading the F4 icon needed an external crop and zoom.

## Files
- Final frame (title): `/mnt/ephemeral/opensummoners/drive/runs/story-d11o/view/look_f059973.png`
- Victory: `/mnt/ephemeral/opensummoners/drive/runs/story-d11o/view/look_f032428.png`
- Selene down: `/mnt/ephemeral/opensummoners/drive/runs/story-d11o/view/look_f031728.png`
