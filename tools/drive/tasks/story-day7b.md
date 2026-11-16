---
name: story-day7b
start: story-day7
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: Day 7 (Sweet Sticks) from the Weathervane road, from the end of `story-day7`** (start: that
scenario's movie replayed — f314753 in **260180** (lx 8048, ly 29600), **Arche (Lv6, Status "MAX: 8", Marks 5/20,
123/188 HP, Iron Sword + Leather Armor) + Sana (Lv4, ~60/100 HP, 88 MP, Iron Staff)**, sword drawn, **2 hostiles alive**
(slots 1–2: ~42 + 39 HP — a Mothbee 51025 and a 51011); 1957 CP; 8 Herbs, Revival Drink, Magic Candy, Guardian Drop;
last save = Tonkiness inn, digest `d89ccc7f7e38`). Read `docs/findings/probes/story-day7.md` and the walkthrough's Day 7
section ([RD story-d7] notes) first.

**Tools:** `rd zones R [--all]` on entering each room; `rd map R [--crop X0,Y0,X1,Y1] [--scale N]` when a route is
unclear; `rd ck R drop NAME` frees a checkpoint's savestate (the pool is finite — drop old ones).

Next, per the walkthrough (Day 7 steps 3–5): finish the 260180 fight → 260180 zone 2 (walk-in x 0) → 260190 (sign:
Weathervane / Loch Carudale) → lake street → crate → past Weathervane Tower → couple attacked → **BOSS: Lead Kobold**
(+ Snivelly Kobold: kill Snivelly first; `docs/game/bosses.md`) → **Loch Carudale**: Sana dives, 💎 left chest [Mermaid
Scale], old man by the boat → **Carudale Tunnels**: **MoH 7** + the lever → sun-symbol route → 💎 [Magic Candy], [Better
Herb]×3 → 3 tall platforms → Sana: [Life Dew], [Cloth Gloves] → the boat → outside: [Mystery Juice], [Magic Candy] →
**Chartreux City**.

**Upgrades (DRIVER.md "keep the party strong") — this leg's list:**
- **Right away:** equip the **Cloth Armor** (in the inventory, unequipped) on **Sana** — she has no armor. Free.
- **Check Arche's cap:** the Status page shows "MAX: 8" after MoH 6 (the last driver expected 9). Report what the MoH
  count / cap really is after MoH 7.
- **Heal before the Lead Kobold** (Herbs ×8; F2 quick-heal). Use the Revival Drink only on a downed ally.
- **MoH 7** in the tunnels — take it. **[Cloth Gloves]** → whoever's Armor #2 / Accessory slot fits (Arche's are empty).
  **[Mermaid Scale]**: read its description; equip if it is gear.
- **Chartreux weapons store** (top drawer [Power Drop] → Arche): buy the best sword / armor the money allows; consider
  Leather Armor for Sana if nothing better. Report the store's list + prices.

1. `rd new <run> --from story-day7` replays that movie into a live session. Confirm: `rd look`, `rd status`.
2. Follow the walkthrough's 🔑 steps, as far as you can. Save at every statue / fountain / inn / home save and run
   `rd endsave <run>` after each; note each digest.
3. Fights: `--hold` searches (DRIVER.md); keep an avoid on each member's HP. Heal before bosses.
4. **Run `rd check` and `rd verify` when you reach ~$6 spent — this is mandatory** (the last driver skipped it); then go
   on; verify again before `finish()` if the budget allows.
5. Name the state you need (`rd ram R define`): the Lead Kobold's + Snivelly's HP.

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~315k),
or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run,
frames, the route steps reached with their frames, the end-save digests, each character's level + equipment at the end
and the upgrades taken / skipped (with why), shop lists + prices seen, named state, walkthrough corrections, tool gaps.
