# probe story-day11j

run `story-d11j`; accepted 2026-11-27 03:53

check PASS (tools/drive/tasks/story-day11j.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11j/trace.jsonl (75582 frames)

## driver's report

# story-d11j report

**Run `story-d11j`** got through the 7.1 and 7.2 targets: [Mind Chocolate], [Revival Drink EX], [Pitch Black Key] + **MoH 18**, then the water cavern's [Guardian Drop] (used on Arche) and [Best Herb]. **Fountain 1 was not reached.**

The run ends at **f75582** in room **450170**, the junction room between the water cavern and 7.3. Arche leads, is in control, and stands in the top corridor under the raised gate at lx 215816, ly 29600. Unison is still active. That is 31.8k frames past the start (f43827).

**There is no end save.** No statue or fountain was on the route, so the next leg continues from the run: `rd new NEXT --from story-d11j`. The last save is still slot 11 `21d1362d3bba` (White Rose Inn).

**Why it stops here:**
- After the cavern I spent about 4k frames trying for 450180 (the "up past stairs" room). Two searches failed, and one dropped Arche back to the bottom-left of 450170.
- I stepped back to the top-corridor checkpoint (f75582), which is the better place to resume.
- In the top-left corridor there are two Skeletal Corporals (51252, about 430 and 505 HP) and, at the 450180 door (48000, 32000), an Org Centurion (51122, 554) and a 51242 (409). Reaching 450180, then the 7.3 trap chest and fountain 1, would not fit in the remaining ~8k frames.

**Gates:**
- `rd check story-d11j tools/drive/tasks/story-day11j.md`: **PASS** (step 1 at f43828, step 2 at f44792, no forbid hit).
- `rd verify story-d11j`: **MATCH** (frame 75582, rng 380a632b, semv d89d89894333148d, fb 485c225d5c85a930).
- I looked at the end frame: `/mnt/ephemeral/opensummoners/drive/runs/story-d11j/view/look_f075582.png`. The session is stopped.

## Route
| frame | what |
|---|---|
| 44791 | 450110 zone 2 (left, x 28800) → 450120. I pressed ↑ with a tiny search (actions `R*2 R*4 L*2 .*10 U .*10`), because walking there overshoots. |
| 45195 | 450120 zone 1 (124800, 44800) → **450130** |
| 45542–50020 | 450130: stepping on plate 70130 at x ≈ 100800 starts a long scene (Dark Witch Selene petrifies the teachers, explains the barrier). Control returns at f50020. |
| 50301 | 450130 zone 2 (265600) → **450140** (cavern over a pit) |
| 51620–51889 | Unison cast; from the ledge (122788, 58400) a triple jump reaches the floating brick platform (ly 20000) |
| 52529 | Killed one Purple Slime 51015 on the platform |
| 52894 | **[Mind Chocolate]**, chest 70220 at (160000, 22400) |
| 54611–55247 | One pit fall (back to the spike platform at 273844, 23200). Bats killed at the exit; zone 2 (390400, 48000) → **450145** |
| 56482–57339 | 450145: two Org Centurions (51122) |
| 57464 | 450145 zone 2 (288000, 19200) → **450150** |
| 58757 | 450150 top-left: Org Bowman 51111 and Centurion killed |
| 58863 | **[Revival Drink EX]**, chest (11200, 32000) |
| 60514 / 60634 | Two Purple Slimes killed; lower-left zone 2 (28800, 105600) → **450160** |
| 60822 | Unison cast. A triple jump from x ≈ 255000 (`RC*6 C*24 .*4 C*20 .*4 C*8 LC*12 L*30`) lands on the upper ledge (ly 74400). |
| 62348 | Skeletal Corporal 51252 killed by the upper door |
| 62538 | 450160 zone 2 (217600, 76800) → **450165** (the MoH room; the walk in fully heals Arche) |
| 62841 | **[Pitch Black Key]**, chest (78400, 38400) |
| 62991–63842 | **MoH 18**, 70100 at (76000, 24005), reached with a `C*30` jump. The "Congratulations" scene waits for X. HP and MP go to full. |
| 64512 → 66620 | 450160 zone 1 → 450150 entry 2; along the bottom, Centurion and 51241 killed; zone 3 (281600, 102400) → **450170** |
| 67214 | Arche quick-heal (`F`) |
| 68238 | Bowman and Centurion in the bottom spike corridor killed (they were on Sana) |
| 69951–70089 | Sana at 37 HP: switched leader (`ent`), Sana's `F` quick-heal, then `ent ent` back to Arche |
| 70894 | Ghost Warlock 51083 killed; it held the exit |
| 71076 | 450170 zone 4 (297600, 70400) → **450175**, the water cavern |
| 71452 | Sana (`ent`) casts **Mermaid Robe**: `n6`, hold X 90 frames, release, 2 MP |
| 72218 | Purple Slime sitting on the chests killed; **Arche Lv17** (HP 288, learned The Full Moon) |
| 72511 | **[Guardian Drop]**, chest 70220 at (174400, 51200) |
| 72698 | **[Best Herb]**, chest 70220 at (182400, 51200) |
| 73126 | Guardian Drop used on Arche, Resist +1 (now 49) |
| 74332 | Back in 450170; Unison cast at f74490 |
| 75582 | Unison search up into the top corridor (x ≈ 215800). **End.** |

## Party at the end
- **Levels:** Arche **Lv17** (163/288 HP, 41/51 MP; Atk 74, Def 67, Spi 38, Res 49), Sana Lv16 (166/212, 220/279 MP), Stella Lv16 (189/248).
- **Money:** 12814 CP. The money cap reads **110000** after MoH 18.
- **Gear:** unchanged from the start. Arche: Org Slayer, Iron Armor, Leather Cape, Kitty Ears.
- **New items:** Mind Chocolate, Revival Drink EX, Pitch Black Key, Best Herb (plus a "You got Best Herb!" pickup from a drop around f72315). Bag page 6/6 shows Magic Candy ×3, Mind Chocolate, Recall Lamp, Merrin Card, Doll Hand.

## Upgrades taken and skipped
- **Taken:** Guardian Drop on Arche. She is the front-liner taking the hits, and the guide names no one for it.
- **Skipped:**
  - The Full Moon was not assigned in Abilities (frame budget).
  - The Mind Chocolates were not used (the guide gives no stat use for them).
  - There was no shop on this stretch.

## Game facts and walkthrough corrections (EN-old)
- **Room chain:** 450110 → 450120 (zones 2 and 3 both lead there; zone 1 → 450130) → 450130 (the Selene scene fires from plate 70130 at x ≈ 100800) → 450140 → 450145 → 450150.
  - 450150 is a hub: top-left chest = Revival Drink EX; lower-left zone 2 → 450160; bottom-right zone 3 → 450170.
- **450160:** its upper ledge door (217600, 76800) → **450165**, the MoH 18 + Pitch Black Key room. The ledge needs a Unison triple jump from the floor's right end. The floor ends at x 256000, and so does the ledge above it, so drift right first and steer left at the top.
- **450170:** zone 2 (48000, 32000) → 450180 (top-left, guarded); zone 4 (297600, 70400) → **450175**, the water cavern (a dead end). Zone 3 at the top-left shaft → 450160.
- **450140 is a pit room:** the grassy "ground" is background, and the floors are planks / scaffolds (70202), which `rd map` does not draw. The chest is on the floating brick platform; reach it with a Unison triple jump from (122788, 58400).
- **Unison:**
  - The triple jump (C*30, then C*20 twice, `.*4` gaps) rises about 37000 from standing.
  - From the air you get only **2** boosts, so start on flat ground: starting while sliding down a slope wasted one.
  - Unison lasted more than 1000 frames each time.
  - Unison also gives a winged flight pose (lpose 10115).
- **Water cavern:** "Sana / Mermaid Robe" means Sana (as leader) casts Mermaid Robe (spell 6). Then **Arche** walks underwater and **V opens the chests**.
  - Swimming Sana could not open them with V, which matches the d7b open question.
  - Underwater walking slides about 3000 after release.
- **MoH 18:** the money cap shows 110,000, not the 120,000 in the walkthrough.
- **Enemy codes:**

  | code | enemy | HP |
  |---|---|---|
  | 51015 | Purple Slime | ~300 |
  | 51062 | Vile Bat | ~85 |
  | 51052 | slime | ~95 |
  | 51122 | Org Centurion | 530–607 |
  | 51111 | Org Bowman | 289–353 |
  | 51083 | Ghost Warlock | 320–390 |
  | 51252 | Skeletal Corporal | 430–515 (lies as a bone pile until near) |
  | 51241 | unidentified | 256 |
  | 51242 | unidentified | 408–433 |

  Many start dormant (`c+0x1d0` = 0).
- **Leveling:** the level-up info screen opens on F1 when a level-up is pending, and `ent` closes it.

## Named state (run-local)
- `m0`, `m1`, `m2`: party HP. `phmin`: their minimum.
- `e0`–`e13`: HP of hostile character slot N (slot-based, so the enemy behind each name changes per room).

## Tool gaps
- **`rd map` misses floors.** It does not draw plank / one-way floors (70202 and the cavern "ground"), so 450140 looked like an open void and walking "on the ground" fell into the pit.
- **Searches waste frames when they fail.**
  - `--apply-best` fight searches can apply long detours. One spent 2768 frames and dropped Arche to the room bottom.
  - A goal containing `lstate == 0` never held; I dropped it.
- **A low follower makes the search useless.** When a follower is already near the `phmin` threshold, the fight search collapses to "1 cell" with no hint why. Healing first (leader swap + `F`) fixed it.
- **Mashing `ent` in a quiet scene swaps the leader.** `tap ent --until script==0` started in free roam just swaps the leader (undone once).
- **Session restarts are slow.** After idle, `rd ck load` took over 3 minutes (replay).

No repo edits, no commits.
