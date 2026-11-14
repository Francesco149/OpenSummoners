# probe town-services

run `town-services`; accepted 2026-11-14 04:24

check PASS (tools/drive/tasks/town-services.md)
verify MATCH (+ real device)
filed tests/scenarios/town-services/trace.jsonl (16146 frames)

## driver's report

**Run `town-services`: Tonkiness town services probe (Day 5 save)**

The task check passes, but `rd verify` gave **MISMATCH**. Frame and RNG match (16146 / aa07b3d3). The state hash does not: two clean replays disagree on one "volatile-addr" dword. My guess (not proven) is a host-clock word, such as the list key-repeat GetTickCount deadlines (grid info +0x1c / +0x20, findings #150) or the save timestamp. The verify-drift-f16146 folder only holds the word count, not the address.

- **Run:** 16146 frames. `rd check … town-services.md` gives PASS (step 1 at frame 850, step 2 at frame 4001).
- **End save:** `save:32e02afc6bad`. The inn save went to slot 7 (savedata06).
- **Start:** Continue, newest slot 6 ("Town of Tonkiness 2:48, Arche LV4 / Sana LV4"). I started inside the Inn (room 210150) with control at frame 849.
- **Story position:** Messages #08 "From the Weapon Shop" (Day 5). That is walkthrough Day 5, step 1, back in Tonkiness.
- **Money:** 113 CP at the start, 164 at the end. The probe spent 3 + 155 + 38 + 20 + 5 = 221 and earned 190 + 77 + 5 = 272. Inventory went from 28 records to 29.

**Steps (frame: what happened; I looked at the picture for each)**
- **f1053–2230, Inn:** talked to the Innkeeper, rested (113 → 110 CP), then "Did you have a good rest, dear? Want to save…" Yes → saved to slot 7.
- **f2838–4682, weapon store (210160), Swordswoman lessons:** on Day 5 the menu has only Unblockable Attacks / Combos / All Done. I opened both topics; All Done closes at once.
- **f4923–6184, weapon store, owner:**
  - Buy Bronze Sword → "On who?" (Arche / Sana / Chiffon / Don't Equip) → "You don't have enough money."
  - Sell: Good Child Drop → "Selling Good Child Drop for 81CP. This is a rare item, really sell it?" (No is the default); I declined.
  - Sold Bronze Sword for 190 (110 → 300), bought Mattress Beater for 155 (→ 145).
- **f7184–8390, boutique (210170):** bought Green Clothes for 38 (→ 107), sold Mattress Beater for 77 (→ 184).
- **f12597–13474, home (210140), capsule machine:** "Should I try it?" Yes → −20 CP → "Aww, all I won was a Dudely Eraser...", then "Should I try again?" → No.
- **f13601–14624, Dad's shop:** the menu is Buy / Sell / Help Out.
  - Bought a Mentama Drop for 5. There was no buy confirm for a non-gear item.
  - Sold the Dudely Eraser for 5.
  - Help Out: "would you be willing to find a Walnut Chocolate for me?…" → Arche "Okay!" (accepted).
- **f15832–16146, Arche's room (top floor of 210140), storage chest:** "Item Storage" menu (Deposit Items / Withdraw Items / Close Box). Deposited Green Clothes, withdrew it, then got "You don't have anything stored."

**Not reached**
- "You have nothing to sell." — I carry items.
- The inn's not-enough-money line — I had too much money to reach it without grinding.
- A statue or fountain — none in town; the inn save covers saving.
- No other NPC with a menu was probed.

**State names (defined in this run)**
- **`gold`** = `s32(s32(world+0x2784)+0x4014)`. Address from findings #149. It read 113 on the F1 screen and followed every buy and sell.
- **`invn`** = `u16(s32(world+0x2784)+0x4010)`, the number of inventory records (#142). It goes up or down by 1 on each buy, sell or capsule prize. Storing an item does not change it.
- **`nstored`** = sum of `u8(room+0x201a+16k)` for k = 0..31, the stored flag. Validated with `ram diff`: withdrawing cleared room+0x21ca (record 27, Green Clothes), and the value went 1 → 0. It only covers the first 32 records.
- **`choice` / `ccur`** (`s32(dlg-0x3c)==1000`, `s32(0x502b0034)`) are heap addresses that are **not reliable**. They gave false negatives on the weapon shop's Buy/Sell and showed ccur=0 while No was highlighted. Do not reuse them. The event runner (R+0x584 choices, #154) is not `world+0x1044`; I could not find R.

**Game facts and guide corrections (EN-old)**
- Tonkiness layout:
  - 210110 is one long street: town gate at lx 0, Inn door around lx 50000–77000, Arms around lx 114000, fountain, stairs, boutique around lx 258000.
  - At lx ≈ 265600 a tower door (press ↑) leads out to the fields (225110).
  - **Home is reached by pressing ↑ under the wooden vine arch at lx ≈ 151000** (→ 210120 house front). No red arrow is shown there; the second arch near the boutique (I tried lx ≈ 208700) did nothing. The guide does not say this.
  - From 210120, Home (Dad's "Items" shop) is through the door at lx ≈ 121000 (→ 210140).
  - 210141 is the parents' bedroom over the storage room, not Arche's room; Arche's room with the chest is the top floor of 210140 (chest at lx ≈ 87500, ly 13600).
- Walking speed is about 240 per frame after a short ramp. `toward()` oscillation turns into a dash.
- Sell prices are per stack (Herb x2 = 10, x3 = 15). Sell price is half the buy price (Bronze 380 → 190).
- The capsule machine re-asks "Should I try again?" with Yes as the default. Mashing Enter spends another 20 CP; I undid one such accidental retry.
- Chiffon is in the party and appears in "On who?".
- After cancelling out of a shop list, the owner's line re-types, and an Enter pressed in the first ~30 frames is lost.

**Tool gaps**
- `ram diff` has no value filter; a "changed to N" filter would have found gold at once.
- Sessions were rebuilt very often (20–30 s replays), and each rebuild wipes `ram mark`.
- `rd tap` has no text predicate (stop when a string is on screen). There is no robust named state for "a choice / list menu is open" or its cursor.

---
model claude-opus-5-5, effort high, 337 turns, $7.58, 145 min

