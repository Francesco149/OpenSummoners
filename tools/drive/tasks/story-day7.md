---
name: story-day7
start: story-herb-boss
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: Day 6's end and Day 7 (Sweet Sticks), from the end of `story-herb-boss`** (start: that scenario's
movie replayed — f271179, **Arche (Lv6, cap 9 after MoH 6, 73/188 HP, Iron Sword + Leather Armor) + Stella (Lv5,
63/145 HP, Cloth Armor)** in **320165**, the Baby Mage just killed (`bhp` 0, script 0; 120 idle frames showed no
dialogue yet); Revival Drink, Magic Candy, Good Child Drop, Guardian Drop, Mystery Juice — **no healing items left**;
2141 CP; last save = the 320160 goddess statue, slot 11, digest `3b7de61d6d91` (after MoH 6)). Read
`docs/findings/probes/story-herb-boss.md` and the walkthrough's [RD story-herb-cave] / Day 7 sections first.

**Tools:** `rd zones R [--all]` on entering each room (exits → rooms); `rd map R [--crop X0,Y0,X1,Y1] [--scale N]`
when a route is unclear; `rd ck R drop NAME` frees a checkpoint's savestate (the last driver exhausted the pool).

Next, per the walkthrough:
- **Day 6 end (step 11.7):** watch for the post-boss event; 320165 zone 2 ↑ (22400, 38400) → 320170: chest [Topah
  Herb] → leave → Sana's house cutscene → save (`rd endsave`).
- **Day 7:** school (talk to **Colm**: Chartreux sweet sticks) → Tonkiness: Mom 30 CP, Sana's mom 30 CP → leave town
  LEFT → lake street → **BOSS: Lead Kobold** (+ Snivelly Kobold: kill Snivelly first; `docs/game/bosses.md`) → Loch
  Carudale: Sana dives, 💎 left chest [Mermaid Scale], old man by the boat → **Carudale Tunnels**: **MoH 7** (cap 10)
  + the lever → sun-symbol route → 💎 [Magic Candy], [Better Herb]×3 → 3 tall platforms → Sana: [Life Dew], [Cloth
  Gloves] → the boat → outside: [Mystery Juice], [Magic Candy] → **Chartreux City**.

**Upgrades (DRIVER.md "keep the party strong") — this leg's list:**
- **Restock healing before the Lead Kobold:** the party has none. Tonkiness item shop (Herb 10, Better Herb 18 — check
  prices) on the way out of town: buy ~6 Better Herbs.
- **Equip Sana when she rejoins** (Day 7): check her gear; the Green Smock in the inventory fits her ("she can wear
  smocks / berets"); the Good Child Drop → Sana (the guide's main caster).
- **[Cloth Gloves]** in the Carudale Tunnels → whoever's Armor #2 / Accessory slot fits (Arche's are empty).
- **MoH 7** (cap 10) in the tunnels — take it.
- **Chartreux weapons store** (now open; top drawer [Power Drop] → Arche): buy the best sword / armor the money allows
  (~2200+ CP by then; report the store's list + prices).

1. `rd new <run> --from story-herb-boss` replays that movie into a live session. Confirm: `rd look`, `rd status`.
2. Follow the walkthrough's 🔑 steps, as far as you can. Save at every statue / fountain / inn / home save and run
   `rd endsave <run>` after each; note each digest.
3. Fights: `--hold` searches (DRIVER.md); keep an avoid on each member's HP. Heal before bosses.
4. **Run `rd check` and `rd verify` when you reach ~$6 spent**, then go on; verify again before `finish()` if the
   budget allows.
5. Name the state you need (`rd ram R define`): the Lead Kobold's HP, Sana's HP slot.

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~271k),
or at a point you cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run,
frames, the route steps reached with their frames, the end-save digests, each character's level + equipment at the end
and the upgrades taken / skipped (with why), shop lists + prices seen, named state, walkthrough corrections, tool gaps.
