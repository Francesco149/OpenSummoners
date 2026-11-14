# probe main-menu-pages

run `main-menu-pages`; accepted 2026-11-14 01:00

check PASS (tools/drive/tasks/main-menu-pages.md)
verify MATCH (+ real device)
filed tests/scenarios/main-menu-pages/trace.jsonl (4521 frames)

## driver's report

## main-menu-pages: report

I opened every page of the F1 Main Menu, looked at each one, and put back every value I changed. `rd check` **PASS**es, but `rd verify` returned **MISMATCH**. Frame 4521 and RNG abf0fd12 match across all three runs. The semantic hash (semv) differs every time, and it also differs between two clean replays that used no savestates (headless 17a74f6c… vs rebuilt 54acda5f…). So some saved state depends on the host clock, not on my inputs. My best guess is the menu key-repeat deadlines, which engine-quirks.md #150 says come from GetTickCount and live in the grid info at +0x1c/+0x20. I did not confirm this, because `verify` returned no evidence.

**Run:** main-menu-pages, 4521 frames, started with `--from end:rh-after-prologue`.

**Task steps:**
- Step 1 holds at frame 363, after loading slot 6 ("Town of Tonkiness", 2:48, Arche LV4 / Sana LV4, the newest save).
- Step 2 holds at frame 4001.
- The forbid ("back at the title") never fired.

**Where the game is:** Day 5, in a Tonkiness inn (room 210150), at the start of walkthrough Day 5 step 1. The latest letter (No.7, "We Found A Stone! But…") points to the Crests of the Wind at the Shrine of the Wind and Weathervane Tower. Difficulty is 1:Easy.

**Values at start and end (the same):** Arche HP 161/161, MP 27/27; Sana 100/100, 88/88; 113 CP (Max 6000).

### Pages, in order (frame = when I looked)
- **Top menu (403):** Items, Special Items, Status, Abilities, Party Commands, Messages, System, plus a "Party Member Status" side panel.
- **Items**
  - 471, 492: the list, 3 pages.
  - 523, 554: Herb → Use → "On who?" / "Who do you want to use this on?". Confirming on Arche at full HP does nothing and uses up nothing (585, 616).
  - 731, 751, 798: Melshee Chocolate → Discard → "Are you sure you wish to discard the Melshee Chocolate?" Yes/No; I chose No.
  - 819, 840, 882: Z re-sorts the whole bag (it is not a swap); a second Z put the original order back.
- **Equipment (inside Items)**
  - Weapons and armour show Equip/Discard; an equipped costume shows Equip/Remove/Discard.
  - Bronze Sword → "Which character will equip this?" (Arche/Sana/Chiffon/Don't Equip) with a stat preview (1709). It shows "Cannot Equip" for Sana (1735) and Chiffon (1761).
  - I equipped Bronze Sword on Arche (1818), then re-equipped the Iron Sword (1911).
  - Green Smock: "Cannot Equip" for Arche (1999); it is a costume for Sana (preview 2025). I equipped it on Sana (2056), then removed it with Remove (2113, 2157).
  - Leather Armor preview shows Defense 30→35 (2232); I left with Don't Equip (2302).
  - Armor #2 and Accessory are empty and there were no items to test them.
- **Special Items (955):** Old Sword, Mysterious Elemental Stone, Elemental Stone 'Eolamest', Old Key, Stone Tablet, Memory Photo 1.
  - Memory Photo → Use shows the full-screen picture (1122).
  - The Stone Tablet has no Use (1210).
- **Status:** Arche (1283) and Sana (1309). Each page also shows that member's Equipment panel and the Adventure Stats panel (Marks 3/20, Monsters 319, Money Gained 1412, EXP 1993, Party Deaths 3, Longest Combo 9, Max Damage Weapon 18 / Magic 30). Pressing Enter on a member opens a read-only abilities list with descriptions (1340, 1387).
- **Abilities**
  - Character picker (2398).
  - Sana's list (2442). Diamond Dust → "Use In Book A/B/C / Don't Use" (2499); I moved it to Book B, which asks for a slot 1–10 (2543, 2574), then back to Book A slot 1, its original place (2701, 2732).
  - Water Shoot's AI toggle: Use → Don't Use → Use (2776, 2820, 2895).
  - Arche's list, 2 pages (2965).
- **Party Commands (3061):** Magic Usage, Follow Mode, Formation.
  - Magic Usage (3092, 3123): Don't use magic / Heal only / Be conservative / Normal use / Cast like mad.
  - Follow Mode (3209, 3240): 'Follow Me!' / 'Fight Freely!' / 'Stay There!'.
  - Formation grid (3326): I moved Sana right and back (3363, 3384).
- **Messages (3485):** 8 letters from Day 1 to Day 5. I read No.8 (3526) and all 4 pages of No.7 (3557, 3578–3620).
- **System (3716):** Options, Quick-heal, Money Display, Give Up, Return to Title.
  - Options → Game & Control (3757, 3798); I left with Cancel Changes → "Exiting without saving. Are you sure?" (3904).
  - Quick-heal: Enter switches a row directly between Use and Don't Use, with no picker. I switched Minor Wound Recovery off and back on (4020, 4064, 4095).
  - Money Display (4165): "as gold" shows "1GP (Max 60)" (4209); I set it back to copper.
  - Give Up → "You cannot give up here." (4338).
  - Return to Title → Yes/No with No preselected; I declined (4423).
- **Closed (4521):** back in free roam with the HUD; Sana and Chiffon are visible.

### State names I defined
- `gold` = `s32(s32(world+0x2784)+0x4014)`: 113 matches the screen. The cap is room +0x4020 (6000).
- `moneygp` = room +0x4064: 0 means CP, 10 means GP. Checked with `--watch`: it went 0→10 at frame 4179 and back to 0.
- `lhp` = `s32(lslot+0xa64)`, `lmp` = `s32(lslot+0xa6c)`. Their maxima are +0xa68+0xa94+0xaac and +0xa70+0xa98+0xab0; they matched 161 and 27 on screen.
- `mcur` = `s32(0x5025e574)` (Main Menu cursor) and `mcount` = `s32(0x5025e570)` (7 rows). The layout is the same as the title list (+0x10 count, +0x14 cursor). This is a fixed heap address: it held through two session rebuilds of this run, but I found no pointer chain to it. It keeps its last value after the menu closes.

### Corrections for docs/game (EN-old)
- **Menu order:** the top level is Items, Special Items, Status, Abilities, Party Commands, Messages, System.
- **No Equipment page:** equipping happens from Items. There is no separate Adventure Stats entry: the stats sit on the Status page.
- **System menu:** it has Options, Quick-heal, Money Display, Give Up and Return to Title, in that order.
- **Party Commands:** it has Magic Usage, Follow Mode and Formation; there is no "Battle Order" command (the words only appear as a label on the formation grid).
- **Items do stack:** the "x N" on a row is a count per slot (Herb x2/x3, Melshee x2), which contradicts "Items never stack". Herbs still take several rows (x2, x3, x3, x3), so a slot seems to hold at most 3, but I did not confirm the cap.
- **Z in Items** re-sorts the whole bag; it does not re-arrange one item.
- **Chiffon** appears in the equip / Follow Mode / Formation lists but not in the Party Member Status panel.
- **Text glitch:** the Water Shoot description renders "(Air Shot)" as "i Air Shotj" (full-width parentheses are mis-mapped).

### Tool gaps
- I need a way to search memory for a value or pointer, to turn a heap address like `mcur` into a pointer chain.
- `verify` returned no evidence for the semv mismatch, and semv appears to include host-clock state, so menu-heavy traces cannot reach MATCH.
- The session was rebuilt twice during the run (about 40 s each).

---
model claude-opus-5-5, effort high, 187 turns, $3.52, 43 min

