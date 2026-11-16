---
name: story-day7c
start: story-d7b
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: the Carudale Tunnels → Chartreux (Day 7), from the end of run `story-d7b`** (accepted later as `story-day7b`; start: that run's movie
replayed — f339205 in **330120** (lx 66992, ly 61600, inside ↑ zone 3 → 330125, which waits until the nearby Lead
Merkid-class foes (53–124 HP) are dead: `zrest` 30), **Arche (Lv6 = adv 3 + combat 3, "MAX: 8", Marks 5/20, 121/188 HP,
Iron Sword + Leather Armor; Armor #2 / Accessory empty) + Sana (Lv5, 77/104 HP, Iron Staff + Cloth Armor; Heal Water)**;
2325 CP; ~3 Herbs, Magic Candy, Guardian Drop; **no Revival Drink**; last save = Tonkiness inn `d89ccc7f7e38` — **nothing
saved since: save at the first statue**). Read `/mnt/ephemeral/opensummoners/drive/agents/story-d7b/report.md` and the walkthrough's Day 7 section
([RD story-d7b] notes) first.

**Tools:** `rd zones R [--all]` on entering each room; `rd map R [--crop X0,Y0,X1,Y1] [--scale N]` when a route is
unclear; `rd ck R drop NAME` frees a checkpoint's savestate (the pool is finite — drop old ones). A search that ends
`found: false` loses its best path unless you pass `--apply-best`.

Next, per the walkthrough (Day 7 step 4): clear 330120's foes → 330125 → the Carudale Tunnels: jump the water pits, drop
into the shallows; ⚠ left chest = poison; right → **MoH 7** + pull the lever → sun-symbol route → 💎 [Magic Candy],
[Better Herb]×3 → 3 tall platforms → Sana: [Life Dew], [Cloth Gloves] → the boat → outside: [Mystery Juice], [Magic
Candy] → **Chartreux City** (sweet sticks: Day 7's goal).

**Upgrades (DRIVER.md "keep the party strong") — this leg's list:**
- **MoH 7** — take it; report the Status page's "MAX" / Marks after it (MAX = adventurer level + combat cap; Marks → cap).
- **[Cloth Gloves]** → whoever's Armor #2 / Accessory slot fits (Arche's are empty). **[Better Herb]×3** — keep for fights.
- **Heal with Sana's Heal Water** between rooms (cheap); keep ≥ 2 Herbs for emergencies; no Revival Drink — don't let
  Sana drop in deep water.
- **Chartreux:** the weapons store (top drawer [Power Drop] → Arche) — buy the best sword / armor the ~2300+ CP allows
  (Arche first: she has the empty slots; Leather Armor for Sana if affordable); restock Better Herbs / a Revival Drink at
  the item shop. Report each store's list + prices.

1. `rd new <run> --from story-d7b` replays that run's movie into a live session. Confirm: `rd look`, `rd status`.
2. Follow the walkthrough's 🔑 steps, as far as you can. Save at every statue / fountain / inn / home save and run
   `rd endsave <run>` after each; note each digest.
3. Fights: `--hold` searches (DRIVER.md); keep an avoid on each member's HP. Heal before bosses.
4. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**; then go on; verify again before `finish()`
   if the budget allows.
5. Name the state you need (`rd ram R define`).

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~339k),
or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run,
frames, the route steps reached with their frames, the end-save digests, each character's level + equipment at the end
and the upgrades taken / skipped (with why), shop lists + prices seen, named state, walkthrough corrections, tool gaps.
