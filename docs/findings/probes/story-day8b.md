# probe story-day8b

run `story-d8b`; accepted 2026-11-18 08:56

check PASS (tools/drive/tasks/story-day8b.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day8b/trace.jsonl (81525 frames)

## driver's report

# story-d8b: Org Lair gargoyle room → statue puzzle → fountain → Boss Org → Stella joins → MoH 8

**Run:** `story-d8b`, started from `story-d8a` at f40663 in room 350150. It ends at **f81525** in room **350200** (the MoH 8 room), with control and a 3-member party. That is about 40.9k frames past the start, the budget stop. Every command used RD_HOST=bunker; nothing touched the Windows box.

**Gates:**
- `rd check` **PASS**: step 1 at f40664, step 2 at f46764.
- `rd verify` **MATCH** at f81525 (rng 7e462cd4, semv db44570000e6d1b0).
- An earlier verify, at f66935 just after the fountain save, also MATCHed (semv 31ee767d8e4f5abb).

**End saves:**
- **f83f6b01986d** (f66935): the Org Lair fountain (goddess statue) in room 350180, saved to slot **17** (`savedata16.sdt`). Arche is alone there with full HP, before the boss.
- I made no save after the boss. The next save point is further on (Cranbal Valley / Korat).

## Route (frames)

**350150 (gargoyle room)**
- f40940: climbed the shaft to the upper corridor.
- Killed 2 Orgs (51120): f41357, then f41736.
- Pushed the gargoyle (70410), sword sheathed, from x 166400 right onto the weight plate **70215** at x 186400.
  - At gx 186400 the plate stayed 0. The plate is kind 2 and needs the rider centred within 400: plate.x + plate.w/2 − rider.w/2 = 187200.
  - Pushed on to **gx 187000**: plate 1, and the tall door **70230** at (40600, 105600) opened (state 1) at f42443.
  - Correction to the d8a guess: 70215 is the plate and 70230 is the door, not the other way round.
- 💎 Killed Org + 2 Kobolds in the bottom-right room. Chest (190400, 102400) = **[Power Drop]** at f45243, used on Arche ("Attack increased by 1").
  - It took effect at once from the menu, with no cast bar.
- Bottom-left room: killed the Org there. ↑ at the door x 12800 → **350160** at f46802.

**350160 (cave with many Orgs, about 16 foes: Orgs 51110/51120, Kobolds 51030)**
- The top-left door (22400, 51200) is reached from the bottom by going right, then up the middle. The search needed about 3.7k frames. Map: `view/map_350160_f46802.png`.
- Killed the 3 Orgs at the door. ↑ at f52863 → **350162**.

**350162 (platform + statue puzzle)**
- Pieces:
  - The siege tower 70480 (base) with a deck 100144. Its travel range is x 28800..102400.
  - The angel statue 70410 starts on the right ledge.
  - The plate 70215 is on the left ledge at x 16800.
- **The deck height follows the floor:**
  - On the right half (x ≥ ~83200) the deck top is level with the centre and right ledges (y 22400).
  - On the left half it is 6400 lower: deck top 28800, the same as the left ledge's plate.
  - The tower passes UNDER the centre ledge.
- Sequence that works for Arche alone:
  1. Push the tower right to tx 102400. Climb onto the right ledge, jump over the statue, and push it left onto the deck.
  2. Push the tower left. The centre ledge scrapes the statue off onto its right end (sx 88700).
     - The statue now hangs off the edge, so you cannot push it from the right. To fix that, reposition the tower to tx 89600: the deck picks the statue back up at 91000. Then stand on the deck right of the statue and push it left along the centre ledge to sx 68000.
  3. Push the tower left to tx 42400. Climb onto the centre ledge right of the statue and push it off the left end. It drops onto the deck at sx 48000, flush with the deck's right edge, so there is no room to stand right of it.
  4. **Key trick:**
     - Push the tower RIGHT from its left side. The statue jams against the centre ledge's underside (its top is 24800, the ledge bottom 25600), so the deck slides under it. Stop at tx 46500: the statue now sits on the deck's left part.
     - Push the tower far left (tx 28800). The statue is at 30300, which leaves standing room on the deck at 33500..37600.
  5. Stand on the deck right of the statue and push it left onto the plate. At f59544 the plate went to state 1: "In the distance, you can hear the sound of machinery activating."
- This is the walkthrough's sequence, but with step 4 added. A lone Arche needs it.

**Back in 350160**
- The heart platform 70202 (handle 350160502) now shuttles at y 41600, x ≈ 179200..228400.
- Killed an Org, rode the platform across (f62510), and killed the 2 Orgs at the exit (f62977).
- ↑ at x 265600 → **350165** at f63033.

**350165 (tunnel)**
- Went right past Orgs and archers over the moving platforms 70202. Arche dropped to HP 74.
- ↑ at x 252800 → **350170** at f65485.

**350170**
- It has no enemies.
- Doors: zone 2 at x 118400 → 350180 (fountain); zone 3 at x 38400 → 350190 (the fancy boss door between two gargoyle statues 70410).

**350180 (fountain)**
- V at the goddess statue: "Water flows from the fountain. Would you like to drink some?" → Yes heals → "Would you like to save?" → Yes → slot list on page 2 with the cursor on 16. R goes to page 3 (17). ent → "Game saved." This was **f66893**.
- `rd endsave` → **f83f6b01986d**.

**350190 (boss room)**
- A long cutscene runs f67371..f73589:
  - Selene (the "Dark Witch") takes Sana's stone, and the Orgs plan to eat Sana.
  - Arche arrives, then **Stella** appears ("Stella Mayberk, magical genius").
  - The game says "Three on three".
- **BOSS** from f73589:
  - Two boss slots: 100172 (948 HP, the Boss Org) and 100173 (684 HP).
  - Also 51120 and 51110. 100174 (5219 HP) was never targetable (the "pot"?).
- **`party` = 7 from f73589**: Sana and Stella fight beside Arche.
- Boss down at **f77310**. Arche's HP bottomed out at 126.
- Post-fight scene to **f80031**: Stella explains Selene, then joins for the Korat field trip ("I'll stay with you until we reach Korat"). Control returns at f80031 with the 3-member party.
- ↑ at x 57600 → **350200** at f80128.

**350200**
- **MoH 8** (object 70100, (49600, 26815), handle 350200501): touched with a jump at f80215. "You obtained a Mark of Heroism!" and "Congratulations!", and the party healed to full.
- A Level-up Info screen followed: Stella 6 → 7, learned **Phoenix**.
- Control at f81094. End state at f81525.

## Party at end (f81525)
- **Arche**: Lv7, 197/197 HP, 31/31 MP.
  - Gear: Org Slayer, Thick Leather, Woven Gloves, no accessory.
  - Attack +1 from the Power Drop (`atk` 37 → 38).
- **Sana**: Lv6, 112/112 HP, 117/117 MP. Gear not checked; her old gear is still in the bag.
- **Stella**: Lv7, 157/157 HP, 88/88 MP (Atk 30, Def 28, Spirit 44, Resist 21). New ability: Phoenix.
- Gold 2301. Items (from Items page 2 at f45500, before the boss): Herb ×1, Better Herb ×3, Revival Drink ×1, Handmade Tart ×3, Magic Candy ×2.
  - Several quick-heals were used after that, so these counts are probably lower now.

**Upgrades:**
- **Took:** the Power Drop, on Arche.
- **Skipped:**
  - The 350165 left chest ([Better Herb]): guarded by an Org and 2 archers (51024), about 1k frames for one herb.
  - Re-equipping Sana's gear and assigning Stella's abilities: no frames left. **The next leg should do this first:** equip Sana (Iron Staff, Leather Armor, Green Smock, Chest Sash are in the bag) and check Stella's equipment and abilities.
- No shop on this route.

## Named state (run vars, defined via `rd ram define`)
- `c0..c15` / `h0..h15`: character slots and the HP of targetable faction-2 characters.
  - In the boss room the party took slots c0, c1, c3, c8, before the enemies, contrary to "enemies first".
- `gx`, `pst`, `tx`, `sx`, `sy`, `hx`, `hy`, `moh`: object x, y and state read through zone-actor slots, `s32(s32(s32(world+0x11e0+4k)+0x40)+4/+8)`, with state at actor+0xc.
  - The slot numbers are room-specific, so these vars are only valid in the room where they were defined.
- Rider check: the body attach pointer at body+0x284 equals the entity it stands on. Per object, `rider_heavy` is +0x1bc and `rider_kind` +0x1b8.

## Walkthrough corrections / facts
- **350150:** push the gargoyle RIGHT onto the plate by the right wall, and centre it at gx 187000 (600 past the plate's x). It opens the tall door 70230 at the bottom-left, which leads to the 350160 door.
- **350162 solo** needs the "scrape" step (step 4 above). The tower deck changes height with the floor slope, and the tower passes under the centre ledge.
- **Stella joins at the Boss Org fight**, in 350190's cutscene, not at the lair entrance. From f73589 the party is 3.
- The Boss Org fight has 2 large foes (948 + 684 HP) plus 2 Orgs.
- **MoH 8** is in 350200, directly below the boss room via the ↑ door at x 57600.
- The Power Drop applies immediately (no cast bar), unlike d8a's Tough Drop note.
- F (quick-heal) sometimes needed ~200+ frames before the HP landed, and it was cancelled or ignored when a foe was nearby.

## Tool gaps
- **The savestate pool ran out** after about 13 checkpoints. Dropping them cost a 40 s session rebuild.
- `rd eval` rejects tuples and list comprehensions. Listing 16 character slots needs 16 calls.
- No helper to read an object's x / y / state by zone slot. I had to hand-define `s32(s32(s32(world+0x11e0+4k)+0x40)+4)`; a built-in would help.
- **`rd tap ent --text` through long cutscenes costs about 20 frames per line.** The boss cutscenes took about 9k frames, which is most of the run's slack.
- Search output files stay empty until the search ends, so there is no progress to read meanwhile.
- **No `DISK LOW`** was seen.

## Repo edits
None. No commits.

