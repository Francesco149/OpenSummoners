# probe story-day8a

run `story-d8a`; accepted 2026-11-18 08:52

check PASS (tools/drive/tasks/story-day8a.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day8a/trace.jsonl (40663 frames)

## driver's report

# story-d8a: healer save → kidnapping → Org side street → merchant → Org Lair (Tough Drop, Org Slayer) → lair room 350150

**Run:** `story-d8a`, from `save:7722d878b57d` (slot 15). It ends at **f40663**, about 39.9k frames after the load (budget stop). All commands ran with RD_HOST=bunker; nothing touched Windows.

**Gates:** `rd check` **PASS** (step 1 f800, step 2 f3001). `rd verify` **MATCH** at f40663 (rng 33c584d9, semv 1d470c9769a38baf, fb ba8c51307d20e8be). An earlier verify at f31906 also MATCHed.

**End saves:**
- **0ba881e290ac** (f8384): wandering healer save, slot **16**, room 340120. Arche alone, full HP, after the kidnapping (`story` 1310), before the side street. Resume with `rd new NEXT --from save:0ba881e290ac`.
- I reached no other save point. The lair's fountain is past the gargoyle/statue rooms.

**End state:** f40663, room **350150** (Org Lair, the gargoyle/button room off 350125's bottom path). Arche is at the left entry (0, 61600) with 197/197 HP and 31/31 MP. Gold 2085, `story` 1310, `party` 1 (Arche alone; Stella has not appeared yet). Checkpoint `r350150` is at f40663. Map: `/mnt/ephemeral/opensummoners/drive/agents/story-d8a/map_350150_f40663.png`.

## Loading
`tcur` boots on 2, so press U to reach Continue, then ent. The load list opens on page 2 with the cursor already on slot 15 (the newest). One ent loads it. Control comes at f799, room 340120.

## Route (frames)
- f799 340120 → right → f989 **340125**. Myconids and Kobolds hit hard; I fought with the sword drawn. The exit at (188800, 9600) is listed as "walk in" but only fired on **U while idle** with `zrest` 0. f3523 → **340130**: kidnapping cutscene to f5285 (`story` 1310, `party` 1).
- 340130 → 340125: classmates' scene (to f6575) → 340120 (f7183).
- **Healer again** (340120, x≈151600, V while sheathed): 30 CP heal to 197. "Save as well?" Yes, then D to slot 16 → f8384, end-save 0ba881e290ac.
- 340120 ↑ at x 131200 → **340150** (f8631). Path to the exit:
  - The jump is **~15600 high when C is held ~30 frames** (`C*12` ≈ 8400).
  - Path: ground → platform (cols 36–57, rows 16–18) → hanging block (cols 44–45, row 12) → top-left chest ledge (row 10) → upper band → exit (278400, 0).
  - **[Herb]** chest (104000, 28800) at f12108, after killing a Kobold on the ledge.
  - U while idle → f13296 → **340160**.
- 340160: Chiffon scene at the Org Lair door (124800, 67200), f14847. Quick-heal F to full.
- Detour right → 340165 (f15838): traveling merchant (Old Man) plus Item Storage (object 70220 at x 48000 = storage, not a chest).
  - Back via 340165's lower exit → 340160 entry 4: **chest (228800, 70400) = 650 CP** (f17849).
  - Left of it is a pit pocket. Return via 340165's upper exit (entry 3) to the lair door.
- f19389 **350110** (entrance hall: falling platforms, spike trap) → f21292 **350120**.
  - Took zone 3 (x 217600, "2nd stairs") → f23110 **350130**, after killing 2 Kobolds holding the zone.
- 350130: left, drop to the bottom-left door (38400, 80000), after killing an Org and a Kobold → f24813 **350135**.
- 350135 (pulley platforms 70460, pairs 501/502, 503/504, 505/506): search with platform heights in `--cell` found the climb onto the right ledge (ly 36000) → f26356 **350140**.
- 350140 (dead end; spiked pit with moving platforms 70420):
  - Left chest (72000, 32000) = **[Tough Drop]** (f27789). Used on Arche: DEF bonus +0xaa0 0 → 2.
  - Right chest (225600, 32000) = **[Org Slayer]** (f31493). Equipped on Arche: ATK 47 → 57 on the equip panel.
  - Each chest is guarded by 2 Orgs.
- Back 350140 → 350135 → 350130 → 350120 (f37785) → zone 2 x 89600 ("1st stairs") → **350125** (f38879) → bottom path, killing 3 Orgs at the bottom-right exit.
- f40572 **350150**. Objects:
  - spike traps 70430, a pushable 70410 (gargoyle?) at (166400, 53600), wooden door 70215 (186400, 57600, flag 350150501), a button 70230 at (40600, 105600, flag 350150502), and statues 70810;
  - chest 70220 at (190400, 102400), probably the walkthrough's [Power Drop];
  - exit ↑ (12800, 115200) → 350160.
  - **Next leg starts here.**

## Party at end
- **Arche** Lv7, 197/197 HP, 31/31 MP:
  - Gear: **Org Slayer** (new), Thick Leather, Woven Gloves; Accessory empty.
  - Tough Drop eaten (+2 DEF).
- Sana (kidnapped): her gear is in the bag (Iron Staff, Leather Armor, Chest Sash, Green Smock). Stella has not joined.
- Items, from F1 → Items at f28000:
  - Herb ×4, Better Herb ×6 (1 bought here), Revival Drink ×1, Handmade Tart ×3, Magic Candy.
  - Also Merrin Card, Underwear, Kitty Ears, Iron Sword (spare).
  - Some Herbs / Better Herbs were used after that via F.
- Gold 1249 → 2085: +650 chest + drops − 18 (one Better Herb).
- **Upgrades:**
  - Took Org Slayer and Tough Drop.
  - Skipped the merchant's Rondel 1050 (Stella's dagger unknown, she is not in the party), Magic Ring 1600, Shortsword 2800, Bronze Armor 2600.
  - Skipped 350130's left chest pair (poison + Moldy Bread), its bottom-right [Better Herb], and its top-right crates [Herb]: Org-heavy detours of low value.

## Shop: traveling merchant (Old Man, 340165 x≈40000)
Better Herb 18, Venom-Away 7, Revival Drink 350, Shortsword 2800, Iron Staff 350, Rondel 1050, Thick Leather 950, Bronze Armor 2600; page 2: Magic Ring 1600 ("makes the wearer feel more spirited").

Flow: "You need anything?" → Buy/Sell → list. The first `ent` on the list **buys at once**: 1 Better Herb was bought by accident (harmless). C backs out to Buy/Sell, then C closes.

## Game facts / walkthrough corrections
- **340150:** the exit band is reached from the LEFT, via the top-left [Herb] ledge, with full-height jumps (hold C ~30 f, ≈15600 up). The low right alcove is a cul-de-sac, not the way. Below the right side (y > 102400) is a pit (−8/−9 HP, back to the ledge).
- **Walk-in zones** (kind 1) in 340125, 340150, 350135 and 350110 fired only on **U while idle on the ground** with `zrest` 0. Walking into them did nothing.
- **340160:** the 650 CP chest ("drop off right edge → tiny tunnel") is at (228800, 70400) in 340160, reached via 340165's lower exit. The merchant + storage are in 340165.
- **Org Lair rooms (EN-old):**
  - 350110 → 350120. In 350120, zone 2 (x 89600) → 350125 = "1st stairs"; zone 3 (x 217600) → 350130 = "2nd stairs".
  - 350130 (chest pair left x 14400/22400; chest bottom-right x 260800; crates 70303 top-right) → bottom-left door → 350135 (see-saw) → 350140 (Tough Drop left, Org Slayer right; dead end).
  - 350125's bottom path → 350150 (gargoyle / button room).
- **Stella** did not join at the lair entrance or in any room so far. "Arche + Stella" must form later (before the boss?). Unverified.
- **F (quick-heal)** works only while Arche is idle; it is ignored during attack or hit animations. It picks a Better Herb when badly hurt (+110/+126 seen). Herb = 70–100.
- Stat items (F1 → Items → Use → On who?) become a timed cast; the bonus lands ~200 f after the menu closes.
- The Org Slayer kills Orgs much faster than the Iron Sword.

## Named state (run vars)
- `c0..c9`: char slot pointers.
- `f0..f9`: targetable faction-2 flags.
- `h0..h9`: their HP (0 when not targetable).
- `nfoe`: sum of `f*`.
- `p2y p4y p5y`, `q1 q2`: platform y in 350135 / 350140.

A stop condition that sums all ten `h*` exceeds the 598-token limit. Use 2–3 named `hN` per goal and put them in `--cell`.

## Tool gaps
- **Dropping a checkpoint forces a session rebuild that replays the whole movie.** That took 297 s at f21331 on bunker, and it blocks the run. Keep checkpoints instead of dropping them.
- No jump-height hint: the card's "long C*12 ≈ 8400" is not the maximum. Holding C ~30 f gives ≈15600, and searches without it failed.
- Searches starting mid-air "score nothing above the start".
- `toward()` overshoots at speed. Use `--until abs(lx-X) < 400 and lstate == 0`.

## Repo edits
None. No commits.

