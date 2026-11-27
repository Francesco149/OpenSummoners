# probe story-day11k

run `story-d11k`; accepted 2026-11-27 04:12

check PASS (tools/drive/tasks/story-day11k.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11k/trace.jsonl (103883 frames)

## driver's report

# story-d11k report

Run `story-d11k` got further than the task asked. It reached fountain 1 (saved), beat the boss (Skeletal Sergeant ×2), took the [Pitch Black Key] and **MoH 19**, then went on through the bottom-left locked gate to **fountain 2**, where it ends on a save.

- **Frames:** f75582 → **f103883** (28.3k frames used).
- **End position:** room 450225 at the fountain 2 statue (lx 69000, ly 36000). Party at full HP, in control, Arche leading, sword sheathed.
- **End saves:**
  - Fountain 1: `c8e12563b90f` at f81731, slot 12.
  - Fountain 2 (final): **`e50b69b68a6f`** at f103883, slot 13 "Labyrith of Night 10:35". Next leg: `rd new NEXT --from save:e50b69b68a6f`.
  - Slots 1–11 and 24 are untouched. Slot 12 had the shortest play time (5:28) and slot 13 the next shortest (5:38).
- **Gates:**
  - `rd check`: **PASS** (step 1 at f75583, step 2 at f78175, no forbid hit).
  - `rd verify`: **MATCH** (frame 103883, rng 981c1d36, semv da478d85f0ab3262, fb 9e33340b1ed3acea).
  - End frame I looked at: `/mnt/ephemeral/opensummoners/drive/runs/story-d11k/view/look_f103883.png`. The session is stopped.

## Route
| frame | what |
|---|---|
| 75814 | Better Herb on Arche. Out of combat, a herb is instant. |
| 76235 | Two Skeletal Corporals (the bone piles in the 450170 top corridor) killed |
| 76492 | 450170 top corridor gap (x 99k–131k) crossed with a run-jump |
| 77846 | Baby Blazemage 51242 killed; it held the exit. Org Centurion killed earlier (dropped [Org Sword]). **Sana Lv17** (Mirror Image) |
| 78173 | 450170 zone 2 → **450180** |
| 79114 | Up the left shaft to the top corridor |
| 80746 | Zone 5 (166400, 38400), the wooden door → **450185** = **fountain 1** (statue 70410 at 68800) |
| 81731 | Drank (full HP/MP), **saved slot 12**, end save `c8e12563b90f` |
| 82990 | Back in 450180: top corridor right to zone 2 (304000, 19200) → **450190** |
| 84278 | Run-jump over the 450190 middle gap (x 128k–152k) |
| 84536 | **Locked door** opened: lock 70217 at 193600, examined with V facing it, uses the [Pitch Black Key]. Gate 70230 at 200600 |
| 85462 | 450190 zone 3 (236800, 32000) → **450200**, the big pit room |
| 86357 | Unison cast; a triple jump from the left ledge lands on the central tower (ly 23200) |
| 87015–87211 | Org Centurion on the tower killed. **Lever** 70216 at (139200, 22400) pulled with V; it lowers plank 70202 at 113600 |
| 88086 | Rollback point. Life Dew (out of combat: instant, all three to full), then Unison again |
| 88996 | Fancy doors, zone 2 (150400, 25600) → **450210** = **BOSS** |
| 93698 | **Both Skeletal Sergeants dead** |
| 94932 | 450210 zone 2 (102400) → **450215** |
| 95402 | **[Pitch Black Key]** from chest (36800, 35200) |
| 95415 | **MoH 19**, 70100 at (33600, 18905), reached with a jump. Full heal. Money cap now 120000 |
| 97206 | Back in 450200; dropped to the bottom-left corridor |
| 98377 | Bowman, Centurion and Corporal killed |
| 98610 | **Bottom-left locked gate** opened: lock at 19200, key used. Zone 3 (x 0, y 80000) → **450220** |
| 98954 | Life Dew (instant) |
| 99928–102171 | 450220: Centurion and Blazemage in the top corridor, then two Centurions at the bottom, killed |
| 102750 | 450220 zone 4 (252800, 96000) → **450225** |
| 103883 | **Fountain 2**: drank, **saved slot 13**, end save `e50b69b68a6f`. **End.** |

## Boss notes (Skeletal Sergeant ×2, code 100167)
- Each has about 3050 HP and they start on the flanks. They hit followers hard.
- **First attempt (f88391, no Unison):**
  - Damage was slow: about 2000 HP in 1800 frames.
  - Sana fell to 30–36 and Stella to 27, and Arche's Unison cast was interrupted.
  - I rolled back to the doors.
- **What worked:**
  - Life Dew out of combat to fill all three, then entering the room.
  - Casting Unison at the start of the fight, found with a small search (goal `mp <= 31`).
  - Fight searches scored `-(e4+e5) + 3*min(phmin,150)`, with avoid `phmin < 40`. A tighter avoid (70 or 90) collapsed the search to one cell before big hits.
  - Leader-swap quick-heal macros were included in the actions.
  - The kill took about 4700 frames.
  - Bosses were seen asleep (Zz) mid-fight, probably from a party spell.

## Party at the end
- **Arche Lv17:** 288/288 HP, 51/51 MP. Org Slayer, Iron Armor, Leather Cape, Kitty Ears.
- **Sana Lv17:** 225 HP, 289 MP. Learned Mirror Image. Rune Staff, Scale Armor, Chest Sash, Anti-Poison Ring.
- **Stella Lv17:** 256 HP, 236 MP.
- **Money:** 20209 CP, cap 120000.
- **Healing items left:** both Life Dews and 3 Better Herbs used. Left: Better Herb 1, Best Herb about 4, Revival Drink 3, Revival Drink EX, Magic Candy 3, Mind Chocolate, Recall Lamp.
- **New:** [Org Sword] (a Help Out item). Both [Pitch Black Key]s were used on locks.

## Upgrades taken and skipped
- **Taken:** none needed. The Full Moon was already set to "Use" (Abilities page 3). No new equipment was found.
- **Skipped (frame budget, and they were not on the 🔑 path):**
  - The 7.3 side loot: 2500 CP and [Broken Lamp], a Help Out item. This is probably the 450190 bottom-right chest (238400, 89600).
  - The 450220 top chest (145600, 25600). The guide marks it as a teleport trap.
  - The 450200 step-5 chests (232000, 25600), (321600, 32000) and (475200, 44800): Stronggirl Juice, Recall Lamp, Magician's Outfit. These need the stone-block rides and are **worth doing next leg**, from fountain 2.

## Walkthrough corrections and game facts (EN-old)
- **Room chain:**
  - 450170 z2 → 450180.
  - 450180: z5 (166400, 38400) wooden door → 450185 = fountain 1. z2/z3 at the right edge → 450190. z4 at the far top-left → 450400 (unvisited).
  - 450190: lock at 193600 → zone 3 → 450200.
  - 450200 tower: lever 70216 plus the fancy doors (zone 2) → 450210 boss → zone 2 → 450215, with the key chest and MoH 19 (70100 at 33600, 18905).
  - 450200 bottom-left gate (lock 70217 at 19200, 89600; gate 70230 at 13400) → 450220.
  - 450220: z4 (252800, 96000) → **450225 = fountain 2** (statue 70410 at 68800). Other 450220 zones: z2 (67200, 115200) → 450230, z3 (204800, 118400) → 450240.
- **The 450220 "70410" objects at (192000, 117600) and (220800, 117600) are gargoyle statues, not fountains.** These are the step-6 "push right gargoyle left" pair. The fountain object shares code 70410, so check the handle: fountain handles were 450185501 and 450225501.
- The 7.4 boss is only reached via a **Unison boost from the left ledge** of 450200 onto the central tower. The walkthrough's "stairs down" was not needed.
- **MoH 19:** the money cap shows 120000, and so far the in-game cap is 10,000 below each figure in the walkthrough. The "Congratulations!" screen was dismissed by **ent**; X presses were ignored. That differs from d11j's note.
- **Statue saves are free:** drink (heals HP/MP) → Yes, then save → Yes → slot.
- **Followers who fall into a lower area** only hold the exit zone through foes near them. Waiting for them to clear the foes let the door fire.
- **Life Dew:** "restore 160–200 HP to all party members". Out of combat it is instant, but the menu then shows one message box per character, advanced with **X** (ent does nothing). Until they are dismissed, field input looks frozen. That cost me a false "input ignored" chase at the doors.
- **Unison** lasted about 2000 frames. While it is active, holding X does not recast.
- **Enemy codes:** 100167 Skeletal Sergeant (about 3050 HP); 51242 = Baby Blazemage (about 410–480 HP). 51242 was listed as unidentified in d11j.

## Named state (run-local)
- `m0`–`m2`: party HP. `phmin`: their minimum.
- `px0`–`px2` / `py0`–`py2`: party positions.
- `e0`–`e17`: HP of hostile character slot N.

## Tool gaps
- **Search ignores the savestate pool.** After the boss checkpoints, every `ck save` and search failed with "savestate pool exhausted". It listed checkpoints `boss`…`boss5` as live, but `ck drop` refused those names with "no checkpoint" after a `ck load` had dropped them. Only `rd stop` plus a 326 s replay cleared them.
- **The "one cell only" hint is misleading.** It appeared when:
  - an open menu message box swallowed input;
  - the cell lx values came from another room's coordinates (cells 10–17 on the 450180 door);
  - a dormant enemy could not be damaged.
- **The `toward()` hold can trigger a dash.** It drifts into a dash with momentum, and with dash momentum a held L still moved Arche right and into a pit (450200).
- **Pressing ↑ on the door counts as a fight path.** A fight search "won" by entering 450185 through the door. Add `room != R` to the avoid of every in-room search.

No repo edits and no commits.
