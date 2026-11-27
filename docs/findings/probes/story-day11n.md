# probe story-day11n

run `story-d11n`; accepted 2026-11-27 05:07

check PASS (tools/drive/tasks/story-day11n.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11n/trace.jsonl (79660 frames)

## driver's report

# story-d11n report

**Run `story-d11n` reached 7.9 and 7.8.** It took **MoH 20** and saved at **fountain 3**. It solved the crate puzzle and took the **[Pitch Black Key]**. Then it went back to fountain 3 and saved again. The run ends on that save, at **f79660**: 42.4k frames past the start (f37295), about 2.4k over the ~40k guide, because of the walk back to save.

- **Gates:**
  - `rd check`: **PASS**. Step 1 at f37296; step 2 at f43516 (450260 → 450270); no forbid hit.
  - `rd verify`: **MATCH** (frame 79660, rng 47b5f0bd, semv 80ccf22311bb17a4, fb f9bced14164c3fcc).
- **End saves:**
  - Fountain 3 + MoH 20 (slot 16, f52885): `911d37b70f67`.
  - **Final, with the Pitch Black Key** (slot 17, f79599): **`da496577f7be`**. Next leg: `rd new NEXT --from save:da496577f7be`, or `--from story-d11n`.
- **Final frame I looked at:** `/mnt/ephemeral/opensummoners/drive/runs/story-d11n/view/look_f079660.png` (party at the fountain 3 statue). The session is stopped.

## Route
| frame | what |
|---|---|
| 37573 | Magic Candy → Arche (+42 MP, 57/57) |
| 38425–38673 | Walked left to the middle shaft (x ≈ 118–134k; floor spikes 70245/70246 cost about 64 HP); climbed to the top corridor (ly 29600) |
| 39634 | An Ice Dragon came out past the gate and hit Arche during her ~45-frame sword draw, knocking her into the shaft. Redone: drew the sword at the shaft bottom first, then climbed |
| 38902–40193 | Both Ice Dragons killed with Wind Thrust (Unison was still on from d11m) |
| 40654–40990 | Sana Heal Water II ×2 |
| 41283 | Unison cast |
| 43256 | Exit guards killed (51241 + 2 Centurions) |
| **43515** | **450260 z2 → 450270**, the 4-way junction |
| 45033 | Junction z6 (bottom right) → **450290** |
| 47358 | 450290 bottom trap killed (51122, 51242, 51111 between gates 64600 / 128600) |
| 50007 | Right shaft up → z3 (201600, 54400) → **450295** |
| **~50420** | **MoH 20**: 70100 at (59200, 24835); a jump touches it |
| **52885** | **Fountain 3** (statue at x 158400): drink → save → **slot 16** (Save Point 5:57 was the shortest play time) |
| 54248 | Back to the junction via 450290's bottom (cleared, no respawn that time) |
| 56916 | Junction z4 (bottom left) → **450280** |
| 58617 | 450280 bottom trap killed (Ice Dragon + Centurion, gates 58200 / 122200) |
| 60195–61601 | Up the left shaft; upper-corridor 51242 + 51111 killed |
| **61842** | Wooden door z3 (89600, 44800) → **450285**, the crate puzzle room |
| 67858 | Crate on the button; gate opens |
| **~68170** | **[Pitch Black Key]**, chest at (110400, 32000) |
| 70444 | 450285 → 450280 → top-right z1 → junction (top left) |
| 73643 | Junction → 450290. Its bottom trap **respawned** this time; killed again by f76662 |
| 77666 | → 450295 |
| **79599** | Fountain 3: drink → save → **slot 17** (Org Lair 6:13, the shortest) |

## Crate puzzle (450285)
**Layout:**
- Room floor at ly 45600, door z1 at x 16000.
- Crate 70205 starts at (12800, 12800) on the top-left ledge (stand height ly 13600).
- Two hanging cages 70460: L at x 32000 (spans to 41600) and R at x 64000 (spans to 73600). Object x is the left edge.
- Button 70215 at (84000, 35200) on the upper right floor (ly 29600).
- Gate 70230 at 98200; chest 70220 at 110400.
- State is at object +0xc. The button needs the crate: Arche standing on it does nothing.

**How the platforms move (they compare how many bodies each cage holds):**
- **Equal → both at the middle** (y 35200). There each cage's surface is ly 29600, level with the right floor.
- **Uneven → the heavier cage goes to the bottom**, its surface level with the room floor, and the other goes to the top (surface ly 13600, level with the ledge).
- The crate counts as one member.

**Constraints:**
- **A crate dropped from a cage at the middle (~16000 fall) becomes unusable.** It vanished from the picture and could no longer be pushed, so I loaded a checkpoint. Only push it off a cage that is at floor level.
- On R the crate's left edge must be ≤ 67200. Further right, the right floor's slab stops it rising and the cage goes up without it.

**Solution I used:**
1. Sana on R (Stay). L rises to ledge level. Arche (Unison) gets onto the ledge and pushes the crate right onto L.
2. Arche on L, left of the crate. L sinks to floor level (once a second member stepped off R). She pushes the crate right along the floor onto R (at floor level, Sana on it); I stopped at crx 65600, then later pushed it to 67200.
3. Arche boosts onto L at the top: 1v1, middle. Sana jumps onto L; Stella walks on. Arche boosts onto R west of the crate, at the top. That is 2v2, so both go to the middle.
4. Push the crate right onto the right floor and the button. The gate opens; open the chest with V.

**Follow mode:** F3 cycles it, and the HUD F3 icon shows an anchor when it is 'Stay There!'. My first F3 press (with Sana leading) did not keep the other non-leaders in place (Arche still followed Stella); only after a second press did the anchor show and they stayed.

## Party at the end
- **Arche Lv19**, 322/322 HP, 57/57 MP. Gear as before (Org Slayer, Iron Armor, Leather Cape, Kitty Ears).
- **Sana Lv19**, 240/240 HP. Star Rod.
- **Stella Lv18**, 274/274 HP. Magician's Outfit, Celebrity Ring.
- **Money:** 30789 CP. Cap now 120,000 (shown before MoH 20; not re-read after it).
- **Marks of Heroism:** 20 taken.
- **Key items:** Pitch Black Key ×1 (for 7.10's lock).
- **Consumables:** Magic Candy one used, others kept.
- **Skipped:** no shops on this stretch, and no new gear found.

## Game facts and walkthrough corrections (EN-old)
- **450270 (junction):**
  - z1 (102400, 35200) → 450260.
  - **z2 (102400, 73600) lower stairs → 450300**: the 7.10 / final path, not entered.
  - z3 / z4 (left, top / bottom) → 450280.
  - z5 / z6 (right, top / bottom) → 450290.
  - The zone kind is "walk/up", but z6 fired only on **↑**, after the stranded followers caught up.
- **"Bottom-right path"** = z6 → 450290 → bottom trap → right shaft → z3 (201600, 54400) → **450295**: MoH 20 plus fountain-3 statue 70410 (handle 450295501). The room has no enemies.
- **"Bottom-left path"** = z4 → 450280 → bottom trap → left shaft → upper corridor → wooden door z3 (89600, 44800) → **450285**, the puzzle room.
- **450280 / 450290 are mirror rooms.** Each has a 2-gate bottom monster trap and an upper corridor with Ice Dragons and Centurions. In 450290 the cleared trap monsters stayed dead on one re-entry and respawned on a later one.
- **Unison** lasted over ~3400 frames each time I checked (aura still on). Holding X while it is active does not recast it (no MP spent); a tap of X is **Wind Thrust** (lpose 10115).
- **Arche's sword draw takes ~45 frames.** She is hittable throughout, so draw before approaching flying enemies.
- **Ice Dragons are not held by their trap gates:** they fly out to the shaft.

## Named state (run-local)
- `m0`–`m2`: party HP. `phmin`: their minimum.
- `e0`–`e23`: hostile HP per slot. `nfoe`: number of live hostiles.
- `px0`–`px2`, `py0`–`py2`: party positions. `x0`–`x16`: foe x.
- 450285 only: `plx ply prx pry` (cage positions), `crx cry` (crate), `btn gate chest` (object +0xc state).

## Tool gaps and pitfalls
- **A loop of `play ent` calls pressed through the save screen** and overwrote slot 15 (which had to be kept). I undid it five steps back, and slot 15 was back to the original 10:46. Never batch `ent` through Yes/No or save screens.
- **`sum()` doesn't take comprehensions** in `rd` expressions.
- **zsh doesn't split words**, so multiple `rd eval` expressions must be passed as an array.
- **`--at` evals take minutes.**
- **The savestate pool filled up after ~20 checkpoints**; drop old ones.
- **Fight searches run at 80–250 ticks/s**, so they need 150–280 s.
- **A follower stranded mid-room holds the exit** even when zrest = 0. `rd zones` shows it.

No repo edits and no commits.
