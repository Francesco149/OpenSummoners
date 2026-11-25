# probe story-day11b

run `story-d11b`; accepted 2026-11-25 09:18

check PASS (tools/drive/tasks/story-day11b.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11b/trace.jsonl (608262 frames)

## driver's report

# story-d11b report (Day 11 step 3, north of Chartreux; continued after the OOM restart)

**The Grand Gourmand was not reached.** The run ends at **f608262** in **430120**, the Mountain Cat Inn at the North Chartreux Guardpost. The party is in control and rested to full, leader Arche, party 7, 12726 CP. I saved to **slot 6** and filed it as end save **`31deb5360576`**.
- **Why I stopped:** the run is 61.6k frames past its start (f546689); the task's limit was ~40k.
- **Wrong turn:** the Guardpost is walkthrough step 4 ("first go LEFT"), so I went past wherever the Gourmand is. Candidates are under "Where the Grand Gourmand probably is" below.
- **Report file not written:** the harness refused to let me write `/mnt/ephemeral/opensummoners/drive/agents/story-d11b/report.md` ("subagents should return findings as text"). This message is the full report; the orchestrator should save it to that path.

**Run:** `story-d11b`. I continued the killed driver's movie and did not recreate the run.
- **Host:** bunker. `session.json` already said bunker and rd kept it.
- **Windows box:** I never touched it, so there were no ssh resets, no fallback and no DISK LOW.
- **Restart:** the first `rd status` came back quickly at f589423 in 420150, with every checkpoint listed.

**Gates** (run once each, at the end, f608262):
- `rd check story-d11b tools/drive/tasks/story-day11b.md`: **PASS**. Step 1 at f546690, step 2 at f546911, forbid clear.
- `rd verify story-d11b`: **MATCH**. Frame 608262, rng f4275192, semv cf09d5c65697ee16, fb cb0d6b0490081e45. It took 1854 s.

**Saves:**
- Slot 6 had "Town of Tonkiness, Arche LV4 / Sana LV4, 2:48", the shortest play time among slots 6–23. It is now the Mountain Cat Inn save.
- Slots 1–5 and 24 are untouched. Slot 5 is still the Chartreux 9:02 save (`704e90e24d19`); the earlier driver made no save.
- The next leg can start with `rd new NEXT --from save:31deb5360576` or `--from story-d11b`.

**Last frame:** I looked at it (inn interior, party at full HP): `/mnt/ephemeral/opensummoners/drive/runs/story-d11b/view/look_f608262.png`. I stopped the session with `rd stop`.

## Route

### Part 1: the earlier driver, f546689–589423
Reconstructed from its checkpoints, view PNGs and the end state.
- **f546689** `start`: Chartreux inn.
- **f548184–548943:** weapons store 300220. The stock page shows Battle Sword 18000, Magic Dagger 9700, Scale Armor 4800, Iron Armor 9800, Bronze Gauntlets 2800. **Nothing was bought**: gold stayed 9104.
- **f550151** `north0` (420110), **f553842** `r420120`, **f556166** `r420125`.
- **f558813** `cleit`: the flower-field event gave **[Cleitcaithream]**. I confirmed it in Special Items.
- **f559246** `r420150`, **f561194** `gate`: 420150's gate (70230, handle 420150501) was opened with the lever 70810.
- **f563158–580571**, `r420130` … `chests`: the step 3.1 cave **420130**.
  - Checkpoints in order: vines, vburnt, statue0, dock, boatdock, onboat, boatatdock, leftstop, stairs, prevines2, pickup, stellaleft, plate.
  - Two chests: gold went 9104 → 10902.
  - The [Mystery Juice] is not in the bag now, so it was probably drunk (unverified).
  - Sana went Lv13 → 14 (Diamond Dust II); Stella is also Lv14 now.
- **f581514** `revived`: someone was revived; Revival Drinks went 3 → 2.
- **f585803** `back420120`, **f586061** `r420150b`, **f587018** `mid150`, **f587439** `harpdone`.
- **f589423:** **420150's far-left chest** (handle 420150601) gave **850 CP**. So **420150 is the step 3.3 cave**: ceiling spike trap at x 175600, far-left 850 CP.

### Part 2: me, f589423–608262
1. **f590638:** 420150 zone 2 (↑ at x 83200) leads to **420160**, outdoors. A Myconid and a 51052 held the zone; I killed them with a search scored on their HP. I checked the F1 Items and Special Items menus, then loaded back to drop those frames.
2. **f592881:** 420160's bottom-left walk-in exit (x 0, y 48000) leads to **420170**. Two Harpies held it; the fight took 1087 frames.
3. **f596753:** 420170's high-ledge chest (handle 420170601, at 88000, 12800) gave **[Revival Drink]**, now ×3.
   - The search to reach the ledge cost ~2.5k frames.
   - The chest opened only with **V while standing on its x**.
4. **f598701:** 420170's x 0 exit leads to **420180** (outdoors, signpost). Its exits: zone 1 → 420170, zone 2 top right (252800, 6400) → **420220**, zone 3 left at x 0 → **430110**.
5. **f600193–603039:** a Harpy (405 HP), a Trippy Myconid (311) and a Saber-panther (224) held 420180's left exit.
   - The first fight search applied its best cell with **Sana dead and Stella at 17 HP**. I loaded the checkpoint.
   - I healed: Better Herbs on Arche and Stella, Magic Candy on Sana.
   - I searched again with an avoid on Sana's and Stella's HP too; the fight took 1653 frames.
   - Then the exit did **not** fire on walking: holding ← for 600 frames did nothing. **↑ fired it**, although `rd zones` lists it as "walk/up".
6. **f603672:** **430110 is the North Chartreux Guardpost.**
   - **[Power Drop] not found.** The barrel (70302, handle 430110601, x 69000) did nothing to X or V with the sword sheathed, or to slashes. Its state stayed 0. I gave up after ~1k frames.
   - The jar (70304) broke from a slash and gave nothing visible.
   - I talked to a Little Boy NPC by accident (flavour only).
7. **f605643:** **430120 is the Mountain Cat Inn plus the item shop.**
   - The old lady on the left is the shop. Buy page 1: Better Herb 18, Venom-Away S 40, Life Dew 96, Revival Drink 350, Magic Candy 120, Recall Lamp 500, Magic Bracelet 11000, Orange Smock 125.
   - ⚠ `step.sh` pressed `ent` inside the Buy list and **bought 4 Better Herbs (72 CP)** by accident.
   - The innkeeper is the old man at the right counter (~x 61000; talk facing left). 28 CP a night.
8. **f607099–608262:** Rest (full HP / MP), then "Want to save?" Yes, slot 6, overwrite Yes, then `rd endsave` → `31deb5360576`.

## Party at the end
- **Arche** Lv14, 266 HP / 45 MP. Org Slayer, Scale Armor, Leather Cape, Kitty Ears. The **Blessed Earring is still in the bag, not equipped**: harpy confusion never visibly hurt.
- **Sana** Lv14, 201 / 236. Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv14, 228 / 187. Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
- **Gold:** 12726 CP.
- **Items:** Better Herb ~14, Best Herb, Venom-Away S ×3, Revival Drink ×3, Magic Candy ×4, Recall Lamp.
- **Picked up this leg:** Honey, Ghost Marshmallow ×2, Sweet Sticks, Chicken Leg, Orange Beret, Underwear, a spare Chest Sash, Merrin Card, Doll Hand.
- **Story items:** Wind Crest 3, Password Note, Water of Life, Cleitcaithream, Sophia's letter, **Ultimate Bread (not yet given)**, Memory Photos.

## Purchases and upgrades
- **Bought:** only the 4 accidental Better Herbs.
- **Skipped:**
  - Bronze Gauntlets: the earlier driver left no reason.
  - Iron Armor 9800: became affordable only after the weapons store was behind us.
  - Magic Bracelet 11000 at the Guardpost shop: I did not check its stats.
- **For the next leg:** with ~12.7k CP, consider Iron Armor or the Magic Bracelet.
- **Not found:** [Guardian Drop] and [Life Dew] (the "crates to the top" part of step 3.3). They are probably up 420160's top-left exit (→ 420200) or 420180's top-right exit (→ 420220). Life Dew is also sold at the Guardpost for 96.

## Where the Grand Gourmand probably is
These exits were never explored:
- 420160 zone 2, top left (x 0, y −3200) → **420200**.
- 420180 zone 2, top right → **420220**.

The walkthrough puts the Gourmand and the secret path before the "go LEFT to the Guardpost" step, so it is most likely behind one of these. Next driver: from slot 6, go 430110 → 420180, then try zone 2 → 420220 first.

## Game facts and walkthrough corrections (EN-old)
- **The route north of Chartreux:**
  - 420110, 420120, then 420125 (flower field, Cleitcaithream).
  - 420130 is the 3.1 boat cave.
  - 420150 is the 3.3 cave. Exits: ↑ x 313600 → 420120, ↑ x 83200 → 420160.
  - 420160 → 420170 (bottom left) or 420200 (top left).
  - 420170 holds the Revival Drink ledge chest and exits at x 0 → 420180.
  - 420180 → 420220 (top right) or 430110, the Guardpost (left, needs ↑).
  - 430120 is the inn and shop.
- **"Stairs, left, bottom-left ledge [Revival Drink]"** in the guide is 420150's ↑ exit → 420160 → 420170's outdoor high ledge, not a spot inside the cave.
- **Stun:** `lpose` 10101 looks like a stagger: inputs are ignored until it clears (Arche frozen with a green glow after Pink Slime hits).
- **Object state:** `s32(s32(world+0x11e0+4*SLOT)+0xc)`, with SLOT as printed by `rd zones --all`; a chest goes 0 → 1 when opened.
- **Follower HP:** `s32(s32(s32(world+0x1160+4*SLOT)+0x750)+0x54)`.

## Tool gaps
- **Unsafe fight searches:** `fight.sh` with `--apply-best` watches only the leader's HP and applied a cell with Sana dead. Fight searches need avoids on every member's HP; a built-in `party_hp_min` would help.
- **Expression size limit:** stop conditions are capped at 598 postfix tokens, so a sum of HP over 8 actor slots fails. `fight.sh` needs `SLOTS="a b"`; a built-in `hostile_hp_sum` would remove the need.
- **Exit labels:** the "walk/up" zone label is unreliable; 420180's x 0 exit needed ↑.
- **Hidden menus:** `rd text` cannot see shop or menu lists, so `step.sh` mashed `ent` into a Buy list. A "menu open" flag would let scripts stop.
- **Gate cost at ~608k frames on bunker:** check about 30 min, verify 1854 s.
- **Memory slip:** I briefly started a `nix-shell -p imagemagick` and stopped it after about 2 minutes. Otherwise I ran one rd command at a time.

No repo edits, no commits.
