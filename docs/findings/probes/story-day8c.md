# probe story-day8c

run `story-d8c`; accepted 2026-11-18 09:05

check PASS (tools/drive/tasks/story-day8c.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day8c/trace.jsonl (114535 frames)

## driver's report

# story-d8c: Org Lair (back out) → merchant → healer save → Cranbal Valley → MoH 9

**Run:** `story-d8c`, started from `story-d8b` at f81525 in room 350200, with control and a 3-member party (both confirmed with rd look and rd status). It ends at **f114535** in room **345240** (the MoH 9 cave), with control, about 33k frames past the start. Every rd command used `RD_HOST=bunker`; nothing touched the Windows box. No `DISK LOW` was seen.

**Gates:**
- `rd check` **PASS** (movie at f114167): step 1 at f81526, step 2 at f82172.
- `rd verify` **MATCH** at f114535 (rng 59b106d1, semv 6e632dcd988cd1e2, fb 457c57dbbb25f38a).

**End save:** **43128e19cdfa** (f103743). It was made at the wandering healer in 340120 ("Female Mage", 30 CP heal + "save as well?" → Yes), into **slot 18** ("Save Point", Arche Lv8 / Sana Lv7 / Stella Lv7). There is no save point between there and the end; MoH 9 comes after that save.

## The big correction: the lair has no forward exit
The walkthrough's "Right → Cranbal Valley" after MoH 8 is wrong for EN-old. 350200 (MoH 8) is a dead end, and so are 350190 and 350180.

I dumped the whole room/door graph from memory to check this. The area table is `s32(world+0x103c)`, holding 336-byte room records: room id at +0, zone table at +0x1c, 20 × 12 bytes (zone id, destination, entry).
- The lair (350xxx) connects to the outside only through **350110 ↔ 340160**.
- **Cranbal Valley is 345xxx**, entered from **340130's right exit (zone 2, (201600, 9600)) → 345110**. That is the road where Sana was kidnapped.
- The valley's own exits: 345110 → 345130 (entry 4) → 340190 → 340200 → **360110**, which is probably Korat (360xxx/370xxx/380xxx follow).
- So you must walk back out of the whole lair: 350170 → 350165 → 350160 → 350150 → 350125 → 350120 → 350110 → 340160 → 340150 → 340120 → 340125 → 340130 → 345110.
- The full graph dump is in the scratchpad parse; `rd zones` only shows the current room. Worth adding as a tool.

## Route (frames)
**First, gear and skills at 350200 (f81525–81910)**
- Sana was already fully equipped: Iron Staff, Leather Armor, Chest Sash, Green Smock.
- Stella had **nothing equipped** in any slot. Her abilities were fine: Stab, Fire Handling, Leap Back use on; Fire Wall, Chaser, Phoenix in Book A.

**Back out of the lair**
- 350190 at f82216.
- **Selene cutscene on entering 350170** (f82756–83317). The Dark Witch muses about the "second field trip".
- Fountain room 350180 visited; it is a dead end.
- 350165 at f84553: killed 2 Orgs 51110, rode the moving platform 70202 over the pit, killed 2 Orgs 51120. Arche reached **Lv8** in this room.
- ↑ at x 118400 → 350160 at f86709.
- 350160: search down to the bottom-left door, killed 2 Kobolds → 350150 at f88902.
- 350150: climbed the shaft and killed the Org and Kobold at the walk-in (0, 51200) → 350125 at f91337.
- 350125: killed the Org by the door → 350120 at f92968.
- 350120: killed the Org at the top-left door (19200, 25600) → 350110 at f93960. The entry hall has no foes.
- Left the lair → 340160 at f95561.

**Merchant detour (340165)**
- Entered at f96115 via 340160's upper-right walk-in.
- **Bought:** Better Herb ×8 (18 each) and the **Rondel** (1050), equipped on Stella (Attack 30 → 42). Gold went from 2486 to 1292.
- Stock, page 1: Better Herb 18, Venom-Away 7, Revival Drink 350, Shortsword 2800, Iron Staff 350, Rondel 1050, Thick Leather 950, Bronze Armor 2600. I did not open page 2.

**340160 back up to its exit**
- The lair-door hollow is walled on the left at x 108800, with a ceiling: a full jump peaks at ly 48180.
- The way out is the **upper band**: climb at the right side to ly ≈ 20000 (a jump-through ledge drawn as a cliff), go left along it, then onto the far-left ledge (x < 20000, ly 32800), then the walk-in (0, 22400).
- → 340150 at f101561.

**340150 to the healer**
- Went down-left to the walk-in (0, 48000) → 340120 at f102638.
- Healer (sword sheathed, V): heal + save, f103743.

**On to the valley**
- 340125 at f103877, then 340130 at f105015.
- **345110 Cranbal Valley at f105988.** Foes: Sabercat 51211 (about 190 HP), 51221 (probably the Vicious Cocorat, about 115–130 HP).
- Bottom-left door (22400, 121600) → 345210 at f108583. Foes there: 51072, 51024 archers, 51221, 51012.
- 345210: killed the 2 archers by the bottom-left walk-in (0, 115200) → 345230 at f111223.
- 345230: killed 2 Sabercats and a 51221 at the bottom-left door (22400, 70400) → 345240 at f112920.
- **MoH 9** (object 70100 at (27200, 14015)): touched at **f113203**. "You obtained a Mark of Heroism!", then the party was healed and the money cap rose to 20000. Control returned at f114167.
- The F1 menu then showed the pending Level-up Info:
  - Sana 6 → 7: HP 131, MP 125, learned **Turbid Squall**.
  - Stella 7 → 8.

## Party at the end (f114535)
- **Arche:** Lv8, 206/206 HP, 32/32 MP. Gear: Org Slayer, Thick Leather, Woven Gloves, no accessory.
- **Sana:** Lv7, 131/131 HP, 125/125 MP. Gear: Iron Staff, Leather Armor, Chest Sash, Green Smock. Turbid Squall is new and was left at its default book assignment.
- **Stella:** Lv8, 166/166 HP, 110/110 MP. Gear: **Rondel**; no armour or accessory.
- **Gold:** 1445 CP (cap 20000).
- **Items:** about Better Herb ×6–8 (quick-heal used some), Handmade Tart, Revival Drink ×1, Magic Candy ×2, Kitty Ears, Iron Sword, Merrin Card.
- **Upgrades skipped:**
  - Thick Leather 950 and Bronze Armor 2600 for Stella: I kept the gold for Korat's shops.
  - The 345210 [Leather Cape] chest: time.
  - The diamond lever and the [Holy Water] / [Good Child Drop] chests: not reached.

## Not done
- The rest of Cranbal Valley, Korat, the inn save and the hot-spring / Mayor / Ellie chain were not reached.
- **Next leg:** from MoH 9 go back 345240 → 345230 (top-right walk-in (150400, 6400)) → 345210. Take zone 2 at (150400, 89600) → 345120 → 345220 → 345130 → 340190 → 340200 → 360110 (Korat?).
- Resume with `rd new NEXT --from story-d8c` (the f114535 end), or from save 43128e19cdfa (the healer, before MoH 9).

## Named state (run vars)
- `foes` (eval-only sum): the count of targetable faction-2 characters.
- `ha`, `hs`, `ht`: the party's HP, using per-room slot indexes.
- `hN`: a foe's HP when it is targetable, else 0.
- `moh`: the state of zone-actor slot 0 (`s32(s32(world+0x11e0)+0xc)`).
- Helper script `foes.sh` (scratchpad): for each live slot it lists faction, code, x, y and HP in one eval. All the slot-listing gaps from d8b are solved by passing about 128 exprs to `rd eval`.

## Game facts and walkthrough corrections
- The lair exits only via 350110 → 340160. Cranbal Valley = 340130's right exit → 345110. MoH 9 = 345240, via 345210's bottom-left walk-in → 345230's bottom-left door. The walkthrough calls it the "bottom-left cave", and that holds.
- Selene has a short scene the first time you re-enter 350170 after the boss.
- Walking has about 3000 units of slide after release. Running stops overshoot by up to 12000. Aim short of a door, wait, then press ↑.
- `--goal` on a position often lands **mid-jump**: the next `.*N` falls. Add `--hold` or check that ly is stable.
- The healer's save prompt and the "Game saved" message need key presses. Mashing `ent` in my loop also opened the main menu once; the only cost was a few frames.

## Tool gaps
- **No room-graph command.** The whole connectivity is in memory (area table, `world+0x103c`, 336-byte records). An `rd graph` would have saved this run's first ~10 minutes and the wrong-direction guess.
- **rd map does not show jump-through ledges.** In 340160 the upper "cliff" ledges are jump-through and invisible in the map, so the searches kept failing. That cost about 5k frames: three failed `--apply-best` searches of 1–2k frames each in 340160 / 350150.
- `--apply-best` with a distance score often keeps a path that fell or wandered. I undid those, but some frames remain in the movie, from cases where I noticed late.
- **Search speed on bunker** varied from 43 to 1290 ticks/s. One session was rebuilt mid-run, which lost the savestates.

## Repo edits
None. No commits.

