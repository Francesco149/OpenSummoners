# probe story-stella-crest

run `story-sc`; accepted 2026-11-15 20:47

check PASS (tools/drive/tasks/story-stella-crest.md)
verify MATCH (+ real device)
filed tests/scenarios/story-stella-crest/trace.jsonl (174304 frames)

## driver's report

## story-sc: Stella got the Wind Crest, Arche beat the Stella boss and has Wind Crest 2, and the party is on its way out of the dungeon. Unchecked and unverified: the budget ran out first.

**Status:** I did not run `rd check` or `rd verify`, so the trace is unconfirmed. The run is at **f174304**, in room **290230**, with Arche and Stella together (`party` = 5). Arche has HP 68/172. A script was running when I stopped (script=1, just after a room change). Both task steps should hold (control in a room after loading, room ≠ 0 with frame > 3000), but `rd check` has not confirmed it. The forbid (back at the title) never happened. Next: `rd check story-sc tools/drive/tasks/story-stella-crest.md` and `rd verify story-sc`.

**End-save digests:** none. No goddess statue or inn was on this stretch, so there was no save and no `rd endsave`. The last save is still the Weathervane Inn, slot 9, digest `0ef7e721d142`.

**Checkpoints:** start, fight1, healed, r290240, lever, leverdone, mid, mid2, exitwait, healed2, r290160, k290170, preboss, boss, crest2, back240.

### Route reached (frames)
- **f153656:** start, in 290230 with Stella at 30/107 HP.
- **f154299 → f154387:** killed 2 Cocorats and the Killer Bat with fight searches (Fire Wall plus ↑+X).
- **f154585:** right chest = **Magic Candy**.
- **f154794:** left chest = **Better Herb**. The guide says "Better Herb ×3"; in EN-old it is a single herb.
- **f155071:** Better Herb from F1 → Items healed instantly, to full.
- **f155526:** the archway stairs at lx ≈ 94600 (↑) → **290240**.
- **f156367:** killed a Yellow Slime there; **Stella reached Lv4**.
- **f157586:** killed the Kobold guarding the lever.
- **≈f157700:** **clover lever** flipped with **V** (examine) at lx ≈ 17600, ly 42400; ↑ did nothing. It raises the wooden gate on the upper floor.
- **f159446:** got past the slimes on the upper ledge.
- **f159958:** at the top-left exit (x 0, ly 13600), but the exit was held by 2 Yellow Slimes. Used a Better Herb (a cast bar, because enemies were near).
- **f161004:** after the fight search → **290160**, the talking doors.
- **f161342–161913:** the riddle scene. Stella answers "twelve" on her own; there is no choice to make.
- **f161914:** doors → **290170**, then a Kobold fight.
- **f162934:** → **290180**, the crest pedestal.
- **f163112–163834:** "the second Crest of the Wind fell into Stella's hands". Control returns to **Arche** (Lv5, 172 HP, Iron Sword, Leather Armor) at the 290160 doors.
- **f164189:** ↑ at the doors starts the boss scene in 290170.
- **f166458:** **BOSS: Stella** starts.
- **f169211:** her HP reaches 0, found by a fight search with `--hold 30` (2753 frames). Arche was left with 78 HP.
- **f169481:** the post-fight script starts, about 270 frames after her HP hit 0.
- **f170825:** "You obtained Wind Crest 2."
- **f172107:** control returns with Stella in the party; `story` = 1140, up from 1130.
- **f172753:** the crest-room chest in 290180 (lx ≈ 75000) = **Better Herb**, again a single one.
- **f173189:** back to 290160.
- **f173393:** 290160's right zone → 290240, now full of respawned enemies.
- **f174304:** after a search → **290230**, heading out the Stella way.

**Not reached:** the dungeon exit, the boys, the doctor, the Barness herbalist and the Herb Farm Cave. The Moldy Bread chest was skipped: it is optional and sits in 290240 at (139200, 89600), behind enemies.

### Game facts and walkthrough corrections (EN-old)
- **The Stella boss has 334 max HP** (character code 59993), not the 256 in bosses.md (difficulty is Easy). A Kobold (72 HP) stays in the room during the fight.
- **The clover lever (code 70216) works with V/examine, not ↑.** Code 70810 is the follower gate it raises.
- **290240's actors:**
  - Exits: zone 1 = the stairs back to 290230 at (32000, 89600); zone 2 = the top-left exit at x 0, y 3200–19200 (kind 1, walk in), which leads to 290160.
  - The chest (code 70220) is at (139200, 89600).
  - The lever is at (17600, 41600).
- **290160's zones:** zone 1 at the left edge (x 0, toward 290155, Arche's old route); zone 2 = the doors at x 51200 (↑); zone 3 = the right edge at x 112000, toward 290240 (Stella's route).
- **290180** has only one exit, zone 1 on the left; the pedestal is event zone 101. The only way out is back through 290160.
- **The "talking doors" riddle needs no input from the player** on Stella's visit.
- **"Better Herb ×3" in both chests is a single Better Herb.**
- **The menu sometimes drops an `ent` sent 30 frames after F1;** I had to press it again.
- **Stella's state after the boss:** the party panel showed her at 122/57 (HP/MP), i.e. full, after the scene. The tap log that ended the scene had shown 12 and 5, so check her HP before relying on her.
- **Spare gear:** a spare Iron Sword, Leather Armor, Cloth Armor, Bronze Sword and Green Smock are in the shared inventory. Arche's Armor #2 and Accessory slots are empty.

### Named state (this run)
- `e0`–`e5`, `e7`, `e8`: HP of each enemy slot (in use and faction 2).
- `ehp`: e0..e5 summed. It misses slots 6 and up; in 290240, slot 7 was the slime holding the exit.
- `s0x`, `s8x`: x positions of slots 0 and 8.
- Enemy and boss codes: 51030 Kobold, 51012 Yellow Slime, 59993 boss Stella.
- To list a room's zones, pack each one into a single number: `zone*1e8 + x/100*1e4 + y/100`, with the actor at `world+0x11e0+4k`.

### Party now
- **Arche:** Lv5 (cap 7), HP 68/172, Iron Sword, Leather Armor.
- **Stella:** Lv4.
- **Items:** about 1 Better Herb (net: one from each chest, two used), 1 Herb, 1 Magic Candy.
- **Money:** 1131 CP.
- **Skipped upgrades:** none were bought, since no shop was passed. The Barness store's Shortsword costs 2800, more than we have.

### Tool gaps
- **`ck load` replays from boot** (about 186 s), so a load is not cheap.
- **Fight searches are slow** (100–500 ticks/s) and their paths cost a lot of HP.
- **`sum()` takes no comprehensions,** so an enemy-HP total has to be written out slot by slot.
- **There is no built-in zone or door listing.**
- **A search that finds the goal without `--apply` does not apply its path.** I had to replay it by hand with `rd play`.

---
model claude-opus-5-5, effort high, 183 turns, $8.70, 140 min

