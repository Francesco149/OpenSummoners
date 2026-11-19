# probe story-day8i

run `story-d8i`; accepted 2026-11-18 20:17

check PASS (tools/drive/tasks/story-day8i.md) [driver rd check]
verify MATCH (headless); verify --real: REAL MISMATCH ×2 = R12 flake (fb only, frame + RNG equal). Ruled out by A/B: Windows real ×2 + headless ×1 replays (--fb-hash --hash-every 600): all 288 fb samples + end chain fb=1f5e614fc5417262 @f172632 identical.
filed tests/scenarios/story-day8i/trace.jsonl (172512 frames; rd file)

## driver's report

# story-d8i: 370180 block/lever puzzle → 370190 → 370200 BOSS New Age Myconid (won) → 370210 MoH 11 → Recall Lamp → hot spring scene → day-end save → Day 9 school

**Run:** `story-d8i`, created with `RD_HOST=bunker ./rd new story-d8i --from story-d8h`. Start: f131926 in 370180, control, party = 7, Arche leading.
**End:** f172512 in **230140** (school, Day 9 opening; `script` = 1; party = 1, Arche alone; `story` 1410). That is ~40.6k frames.
Every rd command used `RD_HOST=bunker`. Nothing ran on the Windows box. No DISK LOW.

**Gates:**
- `rd check` **PASS**: step 1 at f131927, step 2 at f149393.
- `rd verify` **MATCH** at f172512 (rng 0722dfed, semv 4b19e1f025a1d235, fb 1663a64908c65c8e).

**End save:** **`328360c66c95`** (f172268; filed after the Day 8 → Day 9 save prompt). Continue from it with `rd new NEXT --from save:328360c66c95`.
- ⚠ The day-end "Please choose a slot to save in." prompt came up during an `ent` tap and was answered by `ent`. That is the cursor's default: most likely slot 21 (the last used), overwritten with the default Yes. I did not confirm which slot.
- The previous save 4ce2d13efdf6 (Korat inn) was the only one before this.

## Route (frames)
- **f132182–133056:** two Salamandras (51043, ~170 HP each) at the bottom killed by a `--hold` search.
- **f136262 → 137148:** heals, all from F1 → Items:
  - Handmade Tart → Sana.
  - Hot Springs Bun → Arche (full).
  - A Phoenix test from Stella did not hit the ledge Myconid 13.6k above her, so I undid it.
- **Middle → top region:** searches with a tight HP avoid.
  - The Poison Myconid on the right ledge (165–175k, 73.6k) poisons on contact: avoid it.
  - Stella got poisoned anyway. I used a Bun, then Venom-Away on her.
- **Chest 370180501 at f140286 = 3000 CP.** It did not open while foes were near; the Red Slimes 51013 and the Dragon Pup 51261 had to die first.
- **Block puzzle (walkthrough 7.4 confirmed, with clarifications):**
  - Sheathe the sword (Z). Push the RIGHT block (216000) right off the top ledge at f141874: it falls into the column x 227–240k down to the lower area.
  - Jump over the left block and push it right as well. It lands on top of the first one, giving a stack at x 227200 (tops at 60.8k and 54.4k) at f142720.
  - ⚠ The horizontal blower at the ledge's right wall (vent 70700 at 236800, 20800) blows LEFT along the ledge, about 13 per hit.
  - Drop down. ⚠ The floor left of the stack ends in a pit (respawn at the ledge), so step down carefully.
  - Push the stack right along the lower floor (lpose 114, 100/frame; the stack moves as one). Kill the Salamandra by chest 502 on the way.
  - The search pushed it all the way RIGHT, over **vent 70700 at (294800, 43200) on the right wall** (f146882). That is the "vent" in the walkthrough: with the stack on it the steam in the pocket corridor stops, and the stack is the step up.
  - From the stack top: a dash-jump left plus an air jumpkick (`L . L*8 LC*50 L U X`) reaches the lever ledge.
  - **Lever 370180511 = V at f148209** (state 0 → 1, no message).
- **Back west** to the top region (f149093). Zone 2 (35200, 38400): ↑ → **370190 at f149392**.
- **370190:** cutscene (Myconids bathing). Control at f150645. Walk right to zone 2 → **370200 at f150995**. Second cutscene, control at f151532.
- **BOSS (in 370200, not 370210)**, HP from the start of the fight:

  | Foe | Code | HP |
  |---|---|---|
  | New Age Myconid | 100177 | 1286 |
  | Toxic Myconid | 100176 | 672 |
  | Myconid | 100175 | 716 |

  - Arche fought with `--hold` / score searches. The spore clouds poison and SLEEP Arche (lpose 10104 / 117, hundreds of frames). Cells need `frame//60` or the search stalls on "one cell only".
  - Stella died around f154900, Sana around f155400.
  - A field heal while foes are near is a ~200-frame cast for the leader ("Sweet Sticks-Arche"). Dash left past the Myconid first, then cast (f155300).
  - Toxic down at f155021. **All three down at f158038** (Arche 26 HP, then poisoned to 13 HP; cured with Venom-Away).
  - Surrender scene; control at f160319.
- **370210** (wooden door, zone 2 at 105600 of 370200; f160595):
  - **MoH 11** (object 370210501 at 43200, 23605) by a jump at f160725. **It fully heals AND revives the party.**
  - Then the "Congratulations!" scene.
  - **Money cap → 40,000.**
  - Chest 370210601 = **Better Herb** (the bag now shows ×3).
- **Recall Lamp** (F1 → Items, "Do you wish to teleport someplace safe?" Yes) → **360140** at f162798.
  - Short talk, then the game auto-moves to **360176**: hot spring scene with the Myconids, Ms. Sophia, Stella.
  - Ends "And so, that evening...", `story` 1320 → 1330.
- **310140, Stella alone** (night monologue, "moaning"), `story` 1340 → **day-end save prompt** (saved, see above) → "The Bonus Menu has been expanded." → Day 9 school 230140 (`story` 1410). Stopped there.

## Party at the end
At the MoH (f161983), all three were full: Arche 216/216 HP, 39/39 MP; Sana 149/149, 161/161; Stella 189/189, 132/132.
- **Arche Lv10:** Org Slayer, Thick Leather, Leather Cape, Kitty Ears.
- **Sana Lv10:** Mahogany Staff, Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella Lv10:** Lady's Dagger, Woven Gloves, Salamander Scale.

No gear changed. I did not open the Status page for levels after the boss, so they may have risen.

**Gold:** 6146 CP. The 3000 CP chest plus drops.

**Items:**
- Left: Better Herb ×3, Venom-Away S (1 + 2 doses), Revival Drink, Sweet Sticks ×2, Orange Beret, Merrin Card.
- Used: Recall Lamp, Handmade Tart, all 3 Hot Springs Buns, 2 Better Herbs, 3 Sweet Sticks, 3 Venom-Away doses.

**Upgrades:**
- Taken: none (no shop passed; MoH 11 = level cap +1).
- Skipped: chest 370180502 at (257600, 64000). It would not open with V: there was a foe near (`foe` = 1, a Myconid ~60k away), and I did not want to spend more frames there.

## Game facts / walkthrough corrections
- **The boss fight is in 370200**, triggered by the cutscene on entry. 370210 is the MoH room behind the wooden door on the right.
- **MoH 11 sets the money cap to 40,000** (the walkthrough says 50,000). Picking it up fully heals and revives the party.
- **370180's "push the stack right over the vent"** = the vent on the RIGHT WALL of the lower-right corridor (294800, 43200). Its steam blows left across the lever pocket's entrance.
  - Pushed there, the stack both blocks the steam and is the step up.
  - Lever 511 (V) is on a ledge reached by a dash-jump plus jumpkick from the stack top.
  - Blocks fall from the top ledge through the column x 227–240k.
  - The second block lands on the first, and the stack is pushed as one.
- **Chests do not open with any hostile flagged near** (`foe` = 1), even far off-screen.
- **Myconid spores:** poison (Arche, Stella; Sana showed bubbles despite the Anti-Poison Ring) and sleep. There is no idle MP regeneration (Sana stayed at 1 MP over 300 frames).
- **Field item use:**
  - From the F1 menu, a heal is applied at once when no foe is near.
  - With a foe near it becomes the leader's ~200-frame cast bar ("Item-Target"), even when the target is a follower.
  - A menu opened within ~15 frames of closing another drops the input.
- **Day 8 ends:** hot spring scene (360176) → Stella's evening (310140) → save prompt → Day 9 school (230140). There is no mayor visit to play: the scene covers it.

## Named state (run vars)
- `p0 p1 p2` / `m0 m1 m2`: party HP / MP by party slot. `pmin` is the lowest.
- `e0..e8`: HP of character slots 0–8, or 0 when not targetable. In 370200 the party fills slots 0–3, so e4 = the boss, e5 = Toxic, e6 = Myconid. `bsum` = e4 + e5 + e6.
- `bR bL bRy bLy`: 370180 block x / y, objects in slots 21 and 16 (`s32(s32(world+0x11e0+4k)+0x40)+4 / +8`; y = the block's top).
- `v12 v14 v15 v19` / `s12 s19`: vent `body+0x58` / `body+0xa4`. **Not useful here:** they stay constant at 2 and 3200.
- `moh`: unused.

## Tool gaps
- **`rd map --crop`** takes world units: cell coordinates crash PIL ("cannot write empty image").
- **No mid-search item use.** Fights where the leader must heal (a ~200-frame cast) cannot be searched as one goal.
- **The savestate pool was exhausted** once with ~14 checkpoints. After dropping some, the next command rebuilt the session by replaying (~7 min).
- **`rd scan` replays from boot** (~7.5 min for 275 frames) when the checkpoint is no longer in the movie.
- **`rd tap ent --until control`** walks straight through a day-end save prompt. A `--until` that stops at "Please choose a slot" (a save-screen var) would help.
- **Poison and sleep status are not named vars.**

## Repo edits
None. No commits.
