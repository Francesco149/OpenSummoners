# probe story-day8d

run `story-d8d`; accepted 2026-11-18 09:20

check PASS (tools/drive/tasks/story-day8d.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day8d/trace.jsonl (154322 frames)

## driver's report

# story-d8d: MoH 9 cave → Cranbal Valley → Korat Village → the Warm Breeze Inn (save)

**Run:** `story-d8d`, started from `story-d8c` at f114535 in room 345240. I confirmed control (script 0) and a 3-member party (party=7) with `rd look` and `rd status`. It ends at **f154322** in room **360170** (the Korat inn, 2F at the innkeeper), with control. That is about 39.8k frames past the start, so the frame budget is used up. Every rd command used `RD_HOST=bunker`; nothing touched the Windows box. No `DISK LOW` was seen.

**Gates:**
- `rd check` **PASS**: step 1 at f114536, step 2 at f114982.
- `rd verify` **MATCH** at f154322 (rng d5cdec6d, semv ad78f231cf821380, fb 70c4f1d10c30afe1).

**End save:** **b72717552038** (f154322). It was made at the Korat inn: rested (16 CP), then "Want to save your game?" → Yes → **slot 19 "Korat Village"** (Arche / Sana / Stella all Lv8). Slot 18 (d8c's healer save) is untouched. Resume with `rd new NEXT --from save:b72717552038`, or `--from story-d8d`.

## Route (frames)
- **f114982 → 345230.** Exit via the door at (80000, 38400).
- **f115668 → 345210.** Used 345230's top-right walk-in.
- **f116373: [Leather Cape]** from the chest (161600, 140800), bottom right of 345210.
  - Equipped on Arche in place of the Woven Gloves (Def 52 → 54, Res 26 → 32).
  - The Woven Gloves went to Stella (Def 31 → 33).
- **f121279 → 345120.** 345210's bottom band and middle band are not connected on the right. The way up is the shaft at x ≈ 73600, which has wooden jump-through platforms every 12800 (y 131200 / 118400 / 105600 / 92800). Full jumps `C*30` climb it, then the middle band runs east to zone 2 (150400, 89600). Four foes had to die near that door.
- **345120, the lake ("Sana fetches boat"):**
  - The party walks on the lake floor. It has two holes, at x ≈ 224k–244k and 306k–326k; falling in costs HP and puts you back at the pier.
  - Arche wades slowly. With **Sana leading**, dash-jumps (`R . RC*30`) cross both holes: f123387 → f123763.
  - The sunken boat **70600** sits under the right-hand dock at (347200, 121950). Drop through the dock with `D` (f123994) and it rises, then carries the leader toward their facing at about 200/frame. It stops at the left pier (≈ 173000).
  - At the left pier I switched back to Arche and walked onto the boat deck (ly ≈ 116000; f125972). It carried Arche and Sana right. Stella stayed on the dock but rejoined at the next room change.
  - Climbed the right-hand ledges to zone 2 (419200, 32000) → **345220 at f129933**.
- **345220 → 345130 at f131832**, through zone 2 at the left end (28800, 76800).
- **345130:**
  - **[Holy Water]** chest 603 at (228800, 108800): f132176.
  - **Diamond lever** 70216 at (182400, 110400): f132690. It starts the platforms 70202.
  - 345130's upper ledge is **only** reachable from 345220's top-right exit (zone 3 → 345130 entry 2). 345130's zone 1 (bottom, 192000) leads back into 345220's upper-left (28800, 74400): f133446. Then 345220's zone 3 (252800, 44800) → 345130 at f136330, on the ledge (217600, 80800).
  - To reach the top-left island: climb onto the middle rock top (y 36000), then ride the shuttle platform (slot 4, y 41600, x ≈ 67k–102k).
    - **[Holy Water]** from chest 601: f137509.
    - **[Good Child Drop]** from chest 602: f137767. Used on **Sana** at f138417; her Spirit bonus now reads 6.
  - The exit, zone 3 at the top right (240000, 6400), is a walk-in. It is reached via jump-through vine ledges on the cliff at x ≈ 200000, which `rd map` does not show, then the rock top and the right cliff top. A Sabercat sits at the exit and had to be killed. → **340190 at f143509.**
- **340190 → 340200 at f147531** (zone 2, (252800, 19200): a Sabercat and an archer guard it).
- **340200 → 360110 Korat Village at f149186** (↑ at (291200, 32000)).
  - Everyone had been poisoned by Mothbees, and **Sana died of poison** at the door. She came back at 1 HP on the room change.
  - The arrival scene ran: "Korat Village" banner, cold weather, Ms. Sophia and the class. Control came back at **f151925**.
- **Inn = 360170** (Korat's door at x 288000, signs "INN" and "Spa"). The entry scene ran ("let's all go in together"; Stella pushes them), with control back at f152884.
  - The innkeeper (Old Man) is **upstairs** behind the 2F counter (≈ 71600, ly 32800); the stairs are on the left.
  - "Warm Breeze Inn. One night is 16 copper": Rest / Don't Rest / Save. I rested (full heal, 1743 → 1727 CP), then saved to slot 19 at f154254.

## Party at the end (f154322)
- **Arche:** Lv8, 206/206 HP. Org Slayer, Thick Leather, **Leather Cape** (new), no accessory.
- **Sana:** Lv8 (7 → 8 during the run), 136/136 HP, 132 MP. Iron Staff, Leather Armor, Chest Sash, Green Smock. **Good Child Drop** used on her (Spirit 45 total).
- **Stella:** Lv8, 166/166 HP, 110 MP. Rondel, **Woven Gloves** (new). Still no body armour.
- **Gold:** 1727 CP (cap 20000).
- **Items:** about 2 Better Herbs, Holy Water ×2, Magic Candy ×2, Revival Drink, Handmade Tart, Kitty Ears, Iron Sword, Merrin Card. `story` = 1320.

**Upgrades taken:** Leather Cape, Woven Gloves moved to Stella, Good Child Drop, Holy Water ×2.

**Skipped, because the frame budget ran out:**
- The Korat weapon and item stores: no shop lists or prices were seen.
- Body armour for Stella, and a poison cure (Venom-Away).
- The Korat side loot: Hot Water Salts, Mind Chocolate, Inn 2F closet Good Child Drop, and others.

## Not done
- The hot-spring cutscene (the inn's ground-floor door (83200, 54400) → 360175), Mayor → Ellie → Kanon / the Fire Shrine, and the shop lists.
- **Next leg:** shops first. 360110 also has doors at x 89600 (→ 360150), 192000 / 230400 (→ 360160) and 441600 (→ 360120, east).

## Korat room graph (read from the area table)
| From | Exits |
|---|---|
| 360110 | 340200 (west), 360150, 360160 (×2), 360170 (inn), 360120 (east) |
| 360170 (inn) | 360175 (×2) |
| 360175 | 360176 |
| 360120 | 360125, 360130, 360140 |
| 360130 | 360180, 360190, 360200, **380110** |
| 360140 | 370110 (the 370xxx chain runs to 370210, likely the Fire Shrine or the caves) |
| 360200 | 360205 → 999999 (a special exit) |

## Named state (run vars)
- `pa` / `ps` / `pt` and `pmin`: party HP from slot ptrs `room+0x4030/4034/4038 → +0xa64`. These are robust across rooms, unlike d8c's per-room slot vars.
- `ax ay tx ty`: Arche's and Stella's body positions.
- `foes`, and `h0..h31` (foe HP per character slot).
- `chest`, `c601`–`c603`, `lev`: zone-actor states.
- `p4x`: x of zone-actor slot 4's platform. A zone actor's body x is `s32(s32(actor+0x40)+4)`, **not** `actor+4`.
- Scratchpad helpers:
  - `foes.py`: one eval for slot, code, x, y and HP of every live foe.
  - `graph.py`: parses a `rd ram dwords` dump of the area table (`s32(world+0x103c)`; records start at +4, 84 dwords each; zone triples at dword 7) into the whole room graph.

## Game facts and walkthrough corrections
- **51221 is the Poison Mothbee** (not the Vicious Cocorat, as d8c guessed). Its poison drained the whole party and killed Sana (63 HP → 0 in about 350 frames). Carry Venom-Away. Korat's arrival scene cures poison ("Stella is no longer poisoned").
- **The 345120 lake:**
  - Nobody swims. Members walk on the floor, and Sana is fast there.
  - The boat is under the RIGHT dock; drop through the dock (`D`) to board it.
  - The boat goes the way the leader faces.
  - An AI member standing on the dock planks does not board. To bring someone, make them the leader and walk them onto the deck.
- 345130's upper ledge (the way to both the exit and the island chests) is reachable only via 345220's top-right exit. This is the walkthrough's "back in, exit top right".
- The Korat inn is the **Warm Breeze Inn**: 16 CP, innkeeper upstairs, and the save offer comes after resting.

## Tool gaps
- **No `rd graph`:** the room graph had to be dumped by hand, which took 5 minutes.
- **`rd map` does not show jump-through platforms** (345210's shaft, 345130's cliff vines) or the boat / dock collision. Several 200–300 s searches failed because of this.
- `rd text` does not show menu text: the Items list needed `rd look`.
- `--goal` expressions are limited to 598 tokens, so the 32-slot `foes` sum can't be a stop condition.
- Sessions on bunker were slow at times (74–850 ticks/s). One session was rebuilt mid-run (254 s replay), losing the savestates.
- **About 3–4k frames were wasted:**
  - an `--apply-best` path that fell back to the bottom of 345210;
  - the Arche-first attempts at the lake;
  - an extra leg into 345220 and back, because the ledge entry was not obvious;
  - menu fumbling (Status ≠ Equipment: equip goes through Items → Equip → "On who?").

## Repo edits
None. No commits.

