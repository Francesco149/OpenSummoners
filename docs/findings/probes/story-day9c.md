# probe story-day9c

run `story-d9c`; accepted 2026-11-19 04:24

check PASS (tools/drive/tasks/story-day9c.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day9c/trace.jsonl (264278 frames)

## driver's report

# story-d9c: Chartreux Catacombs 4.1–4.3 → Crypt Keys, Scale Armor (Stella), 3 floor buttons → MoH 12

**Run:** `story-d9c` (`rd new --from story-d9b`, all rd via `RD_HOST=bunker`; no Windows box; no DISK LOW).
Start f234522 (400110, control, party 7). **End f264278** in **400146** (MoH room), control, Arche leading, sword sheathed.
29.8k frames used of the 40k budget. Stopped after MoH 12, as the task says.

**Gates:** `rd check` **PASS** (step 1 f234523, step 2 f236980). `rd verify` **MATCH** at f264278 (rng 32bcaf0c, semv 024291ee83da099e, fb cf9897a3e9f1d285).

**End saves:** none. There was no save point on the route (400110–400146), so no `rd endsave`. The last save is still `a2a16253c9bb` (White Rose Inn slot 22). The next leg must continue from the run (`--from story-d9c`).

## Route (frames)
- **400110**
  - f234522–235409: the 2 Ghosts died to a `-fhp` score search (`--hold 30`).
  - The chest is **70220 at (65600, 32000)**, not 70230. Opening it gave **[Crypt Key]** (f236273).
  - 70230 at (114200) is the **gate** (pillar bars). 70217 at (108800) is its **keyhole** (V: "You have no key which matches this keyhole!"). V with the key → "You used Crypt Key. The Crypt Key has broken." Gate state 0→1 at f236573.
  - Zone 2 (182400, U) → **400120** at f236979.
- **400120** (big room)
  - From the top door: run-jump right over the shaft top into the upper-right corridor at y≈39200.
  - Ghosts: one fight search there.
  - The **vines at x 228–235k**: Stella leads. LX / DX burn the low pieces; **`U*3 UX UX`** (hold ↑, then ↑+X) burns the high ones. All 5 were burned by f238810.
  - Zone 2 (272000, press up; ghosts held it) → **400130** at f239992.
- **400130**
  - Drop down the shaft hugging left → ledge (y 48800) → chest **70220 at (24000, 51200)** = **[Crypt Key]** (f241232). Ghosts at the chest were fought first.
  - Down to zone 2 (19200, 92800, "left door"; ghosts held it) → **400120 bottom-left** at f242872.
- **400120 bottom**
  - Search right along the bottom corridor (bats / snakes / ghosts) to zone 4 (304000, 80000; walk-in, but the HUD shows ↑) → **400125** at f244914.
  - Zone 4 was held by a Ghost the sword could not hurt. **Stella's Phoenix (`n3`, hold X ~150 f)** cleared the hold (zrest 30→0).
  - The "chest" seen at x≈292000 was an **enemy drop box**: V → "You got Ghost Marshmallow!".
- **400125**
  - Keyhole **70217 (91200, 104000)** → gate **70230 (98200, 99200)** opened at f245527. These are the walkthrough's "spikes; use key" part; I saw no spikes.
  - Up via the right shaft (green one-way ledges at x≈155–158k):
    - Chest 70220 (142400, 44800) = **[Mystery Juice]** (f248795). A Ghost Mage (51082) and ghosts were here: Arche fell to 2 HP once, so I undid that and re-searched with an hp avoid.
    - Chest 70220 (38400, 32000) = **[Scale Armor]** (f250111).
  - Menu: equipped **Scale Armor on Stella** (Defense 35→73, f250644). Used **Mystery Juice on Sana** (Max MP 161→176 shown, f250839).
  - Zone 2 (153600, 89600) → **400140** at f251851.
- **400140**
  - The "lever" is **70216 at (153600, 46400)**, a wall switch beside gate 70230 (159000). V standing at lx≈156700 facing left flipped it: lever 0→1, gate 0→1 at f253725. X / jump-slashes did not trigger it.
  - Down the right slope to the lower area. Two **Skeletal Soldiers (51027)** were killed by a search (f255899).
  - Stella burned the stair vines 70400 (x 78–88k) going down-left. Six of eight burned; the two upper ones (78000/98800, 82800/97200) do not block the path.
  - The pot **70304 at (6600, 116800)** has to be slashed with the sword DRAWN. It gave **1200 CP** (f257082).
  - Zone 3 (12800, 118400; ghosts held it) → **400145** at f258292.
- **400145** (the 3 buttons)
  - The plates are **70215 at x 13600 / 45600 / 64800** (stand ≈ +1600) and the gate is 70230 at 88600.
  - Main Menu → Party Commands → Follow Mode: set All Members **and** each member to 'Stay There!'. The per-member rows were 'Follow Me!', which overrides the party setting.
  - Arche stood on plate 1, Sana on plate 2 (`ent` swaps leader), Stella on plate 3. The gate opened at f259835 and **stays open after stepping off**.
  - I restored Follow Mode to Follow Me! for all.
  - 2 Ghosts + 1 Skeletal Soldier on the right side were killed (1863 frames). Then the door at zone 2 (131200) → **400146** at f262868.
- **400146**
  - Chest 70220 (84800, 35200) = **[Crypt Key]** (f263289).
  - Jumping (`C*30`) into **70100 at (83200, 20415)** = **Mark of Heroism (MoH 12)** at f263381. Full heal, then a long "Congratulations!" scene. Control returned at f264278.

## Party at the end (f264278)
- **Arche** Lv11, 225/225 HP, 39 MP. Org Slayer, Thick Leather, Leather Cape, Kitty Ears.
- **Sana** Lv11, 158/158 HP, MP max 176. Mahogany Staff, Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv11, 195/195 HP, 140 MP max. Lady's Dagger, **Scale Armor**, Woven Gloves, Salamander Scale. New ability: Blade.
- **7793 CP.**
- Items: Better Herb ×8 (one used by quick-heal), Magic Candy ×4, Recall Lamp, Revival Drink, Venom-Away S ×3, Sweet Sticks ×2, Ghost Marshmallow, Crypt Key ×1 (from the MoH room), Merrin Card. Spares: Iron Staff, Rondel, Chest Sash, Underwear, Orange Beret.

## Upgrades taken / skipped
- Taken: Scale Armor → Stella; Mystery Juice → Sana; MoH 12; 1200 CP pot.
- **Skipped: the 400120 Recall Lamp chest** (70220 at (27200, 70400), left side). It is a detour through a bat / snake / ghost corridor, and a Recall Lamp is already in the bag.
- No shops were on the route.

## Game facts / walkthrough corrections
- 400110: the Crypt Key chest is 70220 at x 65600 (left, near the ghosts). 70230 = barred gate; 70217 = keyhole (V uses the key; the key breaks).
- Catacomb object codes: **70220 = chest** (so the manor 70220 "Item Storage" reading in d9b may be wrong), 70230 = gate, 70217 = keyhole, 70216 = wall lever (V), 70215 = floor plate, 70304 = pot, 70400 = dead vine piece, 70100 = MoH, 70102 / 70101 = door zones.
- Object state address: `s32(s32(world+0x11e0+4*slot)+0xc)` (the `rd zones` "state").
- 4.2 "spikes" were not seen in 400125. The path was keyhole → gate → right shaft up. Mystery Juice is the right chest, Scale Armor the upper-left chest.
- 4.3 "Down, lever" = the 400140 wall switch next to the gate in the upper-middle area. You must flip it to reach the lower areas.
- Ghosts (51081) are often immune to Arche's sword for stretches (no damage). Stella's Phoenix and the AI casters killed most of them. Ghost Mages (51082) sleep / confuse: Arche's controls went reversed or random for ~150 frames.
- A **Ghost drop box** looks like a chest; V picks it up.
- F (quick-heal) starts a cast bar that hits interrupt / outpace. Arche went 67 → 2 HP during it.
- Level-up Info pops up on the next F1 (one page per member) before the Main Menu.

## Named state (this run)
- `p0 p1 p2`: party HP per slot.
- `pmin`: min of the three.
- `nfoe`: targetable hostiles in actor slots 0–7.
- `fhp`: summed HP of targetable hostiles in slots 0–7. Note: enemies can sit in slots up to ~12, and the party follows them.
- `g5`, `near`, `sk`: specific foe-slot HP sums.
- `gate`, `lever`: 400140 object states (slots 19 / 20).

## Tool gaps / notes
- **`rd scan` replays from boot** in a fresh session: 56 frames took 1164 s. Avoid it for short look-backs.
- **The bunker session died repeatedly.** `ck save` after idle triggered ~560 s rebuilds ("no live savestate"), three times.
- `rd text` does not show stacked menu windows: it showed the Main Menu while the Equip / "On who?" boxes were open. Always `rd look` in menus.
- Momentum: after a dash, `R*n --until` overshoots 3–7k. Stopping exactly on a chest needs short taps plus a settle.
- No var for actor slot count, so enemy HP sums must be written per slot (an 8-slot `sum` is near the 598-token limit).

## Repo edits
None. No commits.

