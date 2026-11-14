# probe combat-party

run `combat-party`; accepted 2026-11-14 08:24

check PASS (tools/drive/tasks/combat-party.md)
verify MATCH (+ real device)
filed tests/scenarios/combat-party/trace.jsonl (7794 frames)

## driver's report

## combat-party: report

The run is `combat-party`, 7,794 frames. `rd check` gives **PASS**: step 1 (control after loading) is at f507 and step 2 (frame > 6000) is at f6001. `rd verify` gives **MATCH** (frame 7794, rng f41227b6, semv 6045b9870b34e385).

I stopped at about 7.8k frames, not 30k, to stay in budget. Most of the checklist is done. Not done: Auto-guard OFF, an interrupted cast, and a level-up.

**Route:** title at f302, where the cursor was remembered on Bonus Menu. ↑ to Continue, then slot 6 "Town of Tonkiness, Arche LV4 / Sana LV4, 2:48". The Leather Armor in the bag is unequipped, so this is the save after the Water Shrine. Play starts in Inn 210150 with 8 Herbs and 113 CP, so I bought nothing. Then the Inn door (↑ at lx≈50000), the town gate (↑ at lx≈14000 in 210110), field **260110** (Green Slimes) at f1450, and **260120** (Mothbees) at f4060. 260130 (the woman and guard) has no enemies. In 260120, enemies came back only on a later room entry, not the first one.

### Arche's moves
These ran with the sword drawn (Z) unless noted. Each was looked at in a picture. `lpose` = u16(lchar+0x48).

| Move | Input (facing right; mirror when facing left) | lpose | Frame, result |
|---|---|---|---|
| Jumpkick | `R U X` (each a fresh 1-frame press, from standing) | **10110** | Hit a slime for 15 and killed it at f3441 (picture: flying kick). Also done unarmed at f1563. |
| Combo Slash | `X .*10 UX` | **10111** | Hit a slime 28→13 at f3402 (picture: upward swing) |
| Jump attack | `C*4..8`, then `X` in the air | 10101 while airborne (lstate 4) | Hit 14 and killed a slime at f3878 |
| Cartwheel | facing left: `D*3 DR*6` (↓ then away from the facing) | **10108** | f4163–4194, moved backward while a Mothbee's sparkle attack went off (picture f4169) |
| Arcing Slash (open space) | `UX` | 10104 | f2217 |
| Wide Slash | `RX` | 10109 | f2263 |
| Slash | `X` | 10101 | f2309 |
| Thrust | `DX` | 10102 | f2371 |
| Snap Thrust | `LX` | 10105 | f2515 |

- Wheel Slash is not available. Arche's Combat Level is 3/5, and her F1 → Abilities list has only: Slash, Thrust, Roll, Cartwheel, Wide Slash, Snap Thrust, Jumpkick, Arcing Slash, Downstab, Combo Slash.
- The in-game command strings: Cartwheel "↓ ▶ ←", Jumpkick "→ ▶ ↑ ▶ X", Combo Slash "after using Slash, ↑+X".

### Other checklist items
- **Guard:**
  - Auto-guard is ON in this save. Arche guarded by herself at f4139 (stance 11, lstate 5) and took 151→147.
  - Manual guard: I held ↑ from f4196 to f4286. Stance stayed 11 and lpose was 117, with no HP loss from hits.
  - Auto-guard OFF was not tried.
- **Combo of 5 or more:** a search reached `combo` 5 at f5202 and 8 at f5273. The meter "8 Hit +5%" was on screen at f5277.
- **Sana (Enter at f5773, lid 50011), two spells:**
  - Diamond Dust: `n1`, hold X for 130 frames, release. Pose 20101, MP 83→80 at about f5960. Cast again at f7640–7780 and it hit a bee (ehps 304→289; picture f7782).
  - Heal Water: `n2`, same input. Pose 20102, MP 80→76, Sana's HP 71→100 at about f6160 (shown as "29"; picture f6163).
  - No cast was interrupted.
- **Healing:**
  - `F` at f5317: an herb icon and a charge bar showed, and HP went 69→161 by about f5347.
  - F2 in combat: a first try at f4666 showed the bar (picture f4678), then Arche fell asleep. The second F2 at f7347 had bees on screen. The "Quick-heal" bar showed (F2 icon reads "CANCEL"), then the herb bar, and the heal landed at about f7597 (147→161, green glow; picture f7600). That is about 250 frames in combat.
- **Status effect:** Mothbee Sleep. `rd text` showed "Arche has fallen asleep." (about f4226) and "Arche woke up." (f4564). A bee hit woke her: HP 147→125.
  - The sleep landed while Arche was guarding with ↑, so guard does not stop it.
  - She fell asleep again at about f4828 and woke on her own at f5112, after about 280 frames.
  - Sana was asleep at the same time.
- **Level-up:** none came. Arche has 187 EXP and the Status screen says EXP to level 1500.

### Named state (all defined in this run)
- `ehp0`..`ehp3` = s32(s32(s32(world+0x1160+4i)+0x750)+0x54): enemy HP by slot. Checked: slime 28→13 with a "15" on screen, and slime 27→16 with an "11".
- Enemy code is at slot +0x1d4: Green Slime 51010, Mothbee 51025, crate 58010.
- `d0`..`d3` = the enemy's x (s32(s32(c+0x40)+4)) minus `lx`, or 9999999 when the enemy is dead.
- `ehps` = the sum of the four enemy HPs.
  - The four enemy slots also catch party members when a room has fewer than four enemies (e.g. Arche in slot 3 in 260120).
  - In 260110, slot 1 (a slime, 25 HP, x 136800) never showed on screen and never moved, so it is probably inactive.
- `combo` = s32(room+0x40e0), where room = s32(world+0x2784). It matched the HUD "8 Hit".
- `lid` = s32(lslot+4): 50010 Arche, 50011 Sana. Checked with Enter.
- `lcode` = s32(s32(lslot+0x9f4)+0x1d4).
- `stance` = s32(s32(lslot+0x9f4)+0x14860): 11 guard, 10 crouch.
- `lstate` = u16(lchar+0x38): 0 standing or attacking, 1 walking, 2 crouching, 3 jump start, 4 airborne, 5 guard / look-up.
- `p0hp` = Arche's HP, `p1mp` = Sana's MP (party slot +0xa64 / +0xa6c).
- Already shared: `hp` / `mp` for the leader.
- Other poses seen: 117 = asleep, 98 / 99 = hit or knocked down, 20101 = Diamond Dust, 20102 = Heal Water.
- I could not find a status-effect field. Stat block +0x8c and +0x9c did not change with Sleep.

### Corrections for docs/game [RD]
- The combat-moves probe has two IDs wrong:
  - **10110 is Jumpkick**, not Arcing Slash. A flying-kick picture shows it.
  - **↑+X alone gives 10104** (Arcing Slash). 10111 is Combo Slash.
  - The probe's "Roll 10108" is the same ID I got for a backward ↓-then-away input (Cartwheel). Roll and Cartwheel may share 10108; I did not test Roll facing a known direction.
- A jump attack is just Slash (10101) done in the air. It hits.
- Mothbee Sleep gets through ↑ guard and lasts about 280 frames unless a hit wakes you. Both members can be asleep at once.
- F2 in combat runs two bars: "Quick-heal", then an herb bar. It takes about 250 frames and is not cancelled by a hit. `F` also shows a charge bar in combat.
- Sana has about 100 frames of recovery after a cast. MP is taken about 27 frames after X is released.
- Mothbees put Arche to sleep within seconds while she stands at the right-edge entrance of 260120.
- A pit fall in 260110 costs 8 HP.

### Tool gaps
- The 598-token limit per stop condition: a "nearest live enemy" name built from 4 slots was rejected. `actors()` is still not mapped for this game, so I steered by hand toward a slot index.
- `scan` threw away saved checkpoints once the movie was cut and replayed from boot (fine, but slow).
- `play --watch` with `--every` hides early rows. `scan --changes` was the useful way to get move transitions.
- There is no named status-effect variable. The sleep was seen only through `rd text` and pictures.

---
model claude-opus-5-5, effort high, 203 turns, $4.20, 38 min

