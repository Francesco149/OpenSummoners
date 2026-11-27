# probe story-postgame-b

run `story-pgb`; accepted 2026-11-27 17:50

check PASS (tools/drive/tasks/story-postgame-b.md)
verify: by the batch (the leaf movie of its chain: `rhost.py legs`)
filed tests/scenarios/story-postgame-b/trace.jsonl (73615 frames)

## driver's report

# story-pgb report: party saved at fountain 1, Saint's Brooch equipped

Run `story-pgb` (from `story-pga`) reached the main goal: the party drank at fountain 1 (450185) and saved to slot 19 at **f73615**, about 34.5k frames after the start at f39084. The Saint's Brooch, 4000 CP and 1000 CP are taken, and the code-room lever was used. Selene was not fought.

- **Gates:** `rd check` **PASS** (step 1 at f39085, step 2 at f43560, no forbid hit). `rd verify` **MATCH** (frame 73615, rng 1d7a1ec0, semv 47b95b0ba3b1bfe2, fb fc78e48868440f3d).
- **End save:** **`53b5f2a895a9`** → `rd new NEXT --from save:53b5f2a895a9`. Slot 19 was overwritten (it held "Korat Village Lv8", 6:36, the shortest play time); slots 1–18 and 24 are untouched. Slot 19 now reads "Labyrith of Night, Lv20 ×3, 11:41".
- **Session:** stopped. No commits.

## Route
| frame | what |
|---|---|
| 39084–39981 | Killed Ice Dragon Pup #10 (needs jump attacks: it hovers about 10k above the floor). Its ice breath freezes Arche (`lpose` 119) and hit Sana down to 23 HP |
| 40446 | Killed Pup #9 |
| 40497–41232 | Sana as leader, 3 × Heal Water II (`n8`): party back to full |
| 41641 | **Rapier → Stella** (Atk 97 → 137; the Lady's Dagger goes back to the bag) |
| 42129 | **Stronggirl Juice → Arche**: Max HP +4 (331 → 335) |
| 42734–43452 | Killed an Org Centurion at the 450410 zone 4 passage |
| 43559 | 450410 z4 → **450420** (entry 4, x 0, y 196000) |
| 44624–44799 | Killed 2 Black Harpies and a Baby Blazemage; chest (40000, 198400) = **Revival Drink EX** |
| 45454–47660 | Back in 450410. I cast Unison (sword drawn, hold X ~110 frames, 10 MP), then boost-jumped from the z4 ledge into the pup passage and from there into the z3 passage (floor ~147k) |
| 47913 | Jumped the floor spikes (x 197–207k) → 450410 z3 → **450420** entry 3 (x 0, y 141600) |
| 48702–50642 | Climbed the stairs into the gated chamber (y ~97–116k). Killed 2 Ice Dragon Pups and an Org Centurion |
| 50902–51165 | Walked right to the shaft (x 208–224k) and dropped while holding L (hugging the left wall). Landed in the middle corridor at (198651, 250400) |
| 51165–54379 | Killed 3 Black Harpies and 2 Org Centurions |
| 54497 | Chest (19200, 259200) = **[Saint's Brooch]** |
| 54839 | Chest (12800, 259200) = **4000 CP** ("40 gold coins") |
| 55425 | **Saint's Brooch → Arche**, replacing the Kitty Ears. Every stat went up: Atk 115 → 117, Def 133 → 135, Spi 47 → 55, Res 59 → 67, Max HP 335 → 343, Max MP 59 → 67 |
| 56272–62168 | Dropped down the shaft to the bottom corridor. Killed a Black Panther, 3 Black Harpies and an Org Centurion |
| 63258 | Chest (155200, 342400) = **1000 CP** |
| 64006 | **Mind Chocolate → Sana** (+127 MP) |
| 64781 | 450420 z5 → 450410 bottom. Jumped the spikes at x 177–196k; killed a Black Panther and 2 Black Harpies |
| 66404 | **Lever** (59200, 339200): "A teleportation trap activates!" → "You triggered a teleport trap!" → **450400** (9600, 39200) at f66831 |
| 67148 | 450400 z1 (124800, 28800) → **450180** (entry 4, x 0) |
| 67947–68818 | Healed with Sana; in the top-corridor monster trap, killed an Org Centurion and a Baby Blazemage |
| 68818–72052 | Killed the enemies near the door (51111, an Org Centurion, a Vile Bat); z5 ↑ → **450185** |
| 72800–73585 | **Fountain 1**: drink (party fully healed), save to slot 19 |

## Party at the end
- **Levels:** Arche, Sana and Stella are all **Lv20**. Stella reached Lv20 in this run and learned Chaser II. HP 343/343, 258/258, 278/278 after the fountain.
- **Arche:** Org Slayer, Iron Armor, Leather Cape, **Saint's Brooch**.
- **Sana:** Star Rod, Scale Armor, Chest Sash, Anti-Poison Ring.
- **Stella:** **Rapier**, Magician's Outfit, Woven Gloves, Celebrity Ring.
- **Money:** 51849 CP.
- **Notable items:** Revival Drink EX ×2, Revival Drink ×2, Mind Chocolate ×1, Magic Candy ×2, Recall Lamp ×2, Better Herb ×3, Org Sword, and the Kitty Ears, Lady's Dagger and Magic Bracelet unequipped.
- **Skipped on purpose:**
  - **Magic Bracelet on Sana:** the preview gave Spi 113 → 133 but Def 97 → 91, and she would lose the Anti-Poison Ring. I cancelled.
  - **450420 chest (131200, 185600):** guarded by an Ice Dragon Pup and 2 Black Panthers in a side room off entry 4. Contents unknown.
  - **450420 top chest (94400, 44800):** probably the **Mystery Juice**, reached through 450410 z2 (the top corridor). The top corridor can't be climbed to from where I was.

## 450420 layout (EN-old, 74 × 112 cells)
- **Exits:** four exits on the left edge back to 450410, at y 19200, 131200, 185600 and 329600.
- **Right shaft:** x ~208–224k, from y ~30k to the bottom. It joins the top corridor, the gated chamber (y ~110k), the middle corridor (via stairs at y ~240k) and the bottom corridor (y 340k). Falling to the shaft bottom lands safely on the bottom corridor.
- **Pits:** floor gaps in the bottom corridor (e.g. x ~66–74k) are pits: a small HP loss and back to the ledge.

| entry | area | what's there |
|---|---|---|
| 3 | small ledge at y 141.6k | stairs up-right into the gated chamber (y 97–116k). Gates 70230 at x 80600 / 144600; 2 Ice Dragon Pups and an Org Centurion. Both gates read state 1 before and after the kills |
| 4 | corridor at y 196k | chest (40000, 198400) Revival Drink EX; side room with chest (131200, 185600), pup and 2 Panthers |
| — | middle corridor (y 250–257k) | chests (19200, 259200) **Saint's Brooch** and (12800, 259200) **4000 CP**; 3 Harpies and 2 Org Centurions |
| 5 | bottom corridor (y 340k) | chest (155200, 342400) **1000 CP**; Panther, 3 Harpies, Org Centurion |
| top (via 450410 z2) | top corridor | spike traps 70430, gate 70230 (58200, 35200), chest (94400, 44800), 2 Panthers |

## Game facts and walkthrough corrections
- **The guide's "next passage right side, drop hugging left wall" is literal:** 450410 z3 → 450420 → gated chamber → walk off into the right shaft holding ← → you land in the Brooch corridor.
- **The 450410 lever works with V while facing RIGHT,** standing at lx ≈ 61612 (just right of the lever's x of 59200). Facing left with V, ↑ or X did nothing. pga's "could not trigger it" was the facing direction. The lever's `+0xc` state stayed 0 even after it fired.
- **Exits from 450410's z4 passage:** the passage floor is at 208.8k. The pup passage (floor 176.8k) and the z3 passage (floor ~147k) need Unison boosts; a plain jump is capped by the passage ceiling.
  - Path that worked from the pup passage (with Unison active): `.*8 RC*24 C*12 .*4 RC*20 .*4 LC*12 RC*32`.
  - The 450410 shaft (x 140–157k) has no ledges. Falling to its bottom does **not** wrap: it is a pit, back to the z4 ledge for −16 HP.
- **The code-room teleport (lever)** → 450400 at (9600, 39200). 450400 z1 (124800, 28800) → 450180 entry 4.
- **450180's top corridor is a monster trap** (gates 70230 at x ~68k / ~126k: an Org Centurion and a Baby Blazemage), with more enemies near the fountain door z5.
- **Fountain 1 flow:** V at the statue → "Water flows from the fountain. Would you like to drink the water?" Yes → "Your health is restored!" → "Would you like to save?" Yes → slot list. The cursor starts on the last-used slot (18).
- **Ice Dragon Pups freeze** (`lpose` 119, about 120+ frames, mashing doesn't help). Leader swap (`ent`) is refused while the target member is being hit.
- **Opening chests:** walking into a chest often opens it (`script` 1) without V.
- **Numpad spells:** `n8` alternates between working and not; every other `n8 .*6 X*80` cast produced no heal.
- **Drawing the sword:** `Z` takes ~15 frames to change `sword`, and `Z*3` toggles more than once. Use one `Z` and wait 20+ frames.
- **Level-up screen:** F1 first shows the pending Level-up Info pages, then the Main Menu. Equipment is changed through Items → item → Equip → "On who?", which shows a stat preview.

## Named state (run-local)
- `m0`–`m2`, `phmin`: party HP.
- `e0`–`e21`, `ex*` / `ey*`: hostile HP and positions per character slot.
- `px0`–`px2`, `py0`–`py2`: party positions.
- `lever = s32(s32(world+0x11e0+4*31)+0xc)`: the 450410 lever's state. It did not change when the lever fired.
- The Sana leader-swap target is `lid` 50011.

## Tool gaps
- **`toward(target - lx, 300)` stalls short of the target** (by about 6000) and oscillates. Plain `R*N` / `L*N` worked.
- **Searches over 180 s go to the background,** and the savestate pool evicted a checkpoint, so a later `ck load` rebuilt the session by a 160-second replay.
- **`rd map` doesn't show which gates are open:** 70230 read state 1 both before and after the trap kills.
- **Nothing reads where a spell cursor points:** there is no named state for which spell is selected.

## Files
- Final frame (party at fountain 1): `/mnt/ephemeral/opensummoners/drive/runs/story-pgb/view/look_f073615.png`
- Save confirmation (slot 19): `/mnt/ephemeral/opensummoners/drive/runs/story-pgb/view/look_f073583.png`
- Saint's Brooch equip preview: `/mnt/ephemeral/opensummoners/drive/runs/story-pgb/view/look_f055394.png`
- 450420 full map: `/mnt/ephemeral/opensummoners/drive/runs/story-pgb/view/map_450420_f43599.png`
