# probe items-use

run `items-use`; accepted 2026-11-14 10:47

check PASS (tools/drive/tasks/items-use.md)
verify MATCH (+ real device)
filed tests/scenarios/items-use/trace.jsonl (10298 frames)

## driver's report

## items-use: report

Every step of the probe was done except step 5: no Recall Lamp or other travel item is in the bag. `rd check` **PASS**es (step 1 at frame 764, step 2 at frame 4001, the forbid never fired) and `rd verify` returned **MATCH** (frame 10298, rng af33d4d3, semv f86aae60…).

**Run:** items-use, **10298 frames**, started with `--from end:story-day6-barness`.
- Title → Continue (`tcur` 1) → slot 7 "Barness Village, Arche LV5, 3:43" was already highlighted → room 270200 (Weathervane Inn) with control at frame 764.
- Arche starts at 172/172 HP, 27/27 MP, 893 CP.

### 1. Permanent stat items (Items → Use → "On who?" → Arche)
I opened Status before and after every item and looked at both pages. Every use changed exactly one dword, which I found with `rd ram mark` → use → `diff` over the slot (lslot 0x20063d80 to +0xb80).

| Item | frame of message | message | Status page | dword changed |
|---|---|---|---|---|
| Power Drop | 1299 | "You used Power Drop. / Arche's Attack increased by 2." | Attack 43/33 → 45/35 (881 → 1408) | slot+0xa9c: 2→4 |
| Good Child Drop #1 | 1729 | "…Arche's Spirit increased by 2." | Spirit 15/15 → 17/17 (1838) | slot+0xaa4: 0→2 |
| Good Child Drop #2 | 2159 | "…Spirit increased by 2." | Spirit → 19/19 (2268) | slot+0xaa4: 2→4 |
| Guardian Drop | 2589 | "…Arche's Resist increased by 1." | Resist 18/18 → 19/19 (2698) | slot+0xaa8: 0→1 |
| Mystery Juice | 3019 | "…Arche's Max MP increased by 4." | MP 27/27 → 27/31; current MP did not rise (3128) | slot+0xab0: 0→4 |

- The rng moves on every use, so the size of each increase may be rolled. That is unconfirmed: I used each item only once with no reroll.
- After a use, the bag closes the gap and the cursor stays on the same row.

### 2. Healing item (Herb)
- **The shop:** it is not in the inn; the inn counter only offers Rest / Don't Rest / Save, 6 CP. It is the "Items" house at the right end of the inn street: in 270120 press ↑ at lx ≈ 260000 → room **270190**, with the Old Man at lx ≈ 34000.
  - He offers "Buy / Sell". Page 1 of the stock: Herb 10, Better Herb 18, Venom-Away 7, Wig 150, Iron Sword 780, Shortsword 2800, Iron Dagger 450, Leather Armor 560.
  - **`ent` in the Buy list buys at once, with no quantity or confirm prompt.** A mash bought one extra Herb: 3 Herbs, 893 → 863 CP.
  - The 3 Herbs took separate rows, each "x 1".
- **At full HP (6784–6825):** Herb → Use → "On who?" → confirming on Arche gives no message. Nothing is consumed, and the "On who?" box stays open.
- **After damage:** I went to field 280110 and let a Cocorat bring Arche to 67 HP, then used Herb from F1 → Items (9991).
  - The menu closes and the use is queued as a **field action**: a HUD cast bar "Herb - Arche" with an F2 CANCEL icon (10204).
  - She keeps taking hits while it charges (67 → 56).
  - At **frame 10298** HP went **56 → 135 (+79)**; the guide says 70–100. The HUD number lags behind (it still showed 63 at that frame).

### 3. Items that "cannot be used"
- **Equipment** (Cloth Armor, 3379) has only Equip / Discard. **Underwear** (3237) has only Discard ("Typical underwear for men.").
- **Special Items** (Old Sword 3673, Wind Crest 1 3766): `ent` does nothing — no submenu and no message.
- Memory Photo 2 → Use shows the full-screen "Meeting Chiffon" picture (3884).

### 4. Discard and Z re-sort
- Underwear → Discard → "Are you sure you wish to discard the Underwear?" with Yes preselected (3278); I chose No.
- Cloth Armor → Discard → Yes (3433) → "You discard the Cloth Armor." (3474).
- **Z re-sort (3536):** the bag order was reversed (Underwear, Bronze Sword, Green Smock, Leather Armor, Iron Sword), and the cursor followed its item. A second Z restored the original order (3567). This was with 5 items; the earlier probe called Z a full re-sort, and here it looked like a reversal.

### 5. Travel items
There is no Recall Lamp or other travel item in Items or Special Items. Special Items holds: Old Sword, Mysterious Elemental Stone, Old Key, Stone Tablet, Wind Crest 1, Memory Photo 1, Memory Photo 2.

### Named state (stat block)
- `s32(s32(lslot+0x9f4)+0x750)` = lslot + 0xa10, so the stat block is inside the party slot.
- Dwords, as offsets from the slot:
  - a64: HP; a68: base max HP 168; a6c: MP; a70: base max MP 27.
  - a74: attack base 31; a78: defense base 31; a7c: spirit 15; a80: resist 18; a84 / a88: WP 10 / AP 10.
  - **a9c: attack bonus; aa4: spirit bonus; aa8: resist bonus; aac: max HP bonus (4); ab0: max MP bonus** — the drop / juice bonus fields.
  - ad8: combat level 3; adc: its maximum 5; ae0: adventurer level 2; ae8: EXP 107; aec: EXP to next level 2000.
- How I validated them: each bonus dword was the only change in the diff, by the amount the message gave, and the Status page moved by the same amount.
- **Inferred, not observed:** aa0 = defense bonus (by position; I had no Tough Drop). Shown base attack = a74 + a9c (31 + 2 = 33 at start, then 35).

### Game facts [RD]
- The Barness general store is 270190, entered from the right end of 270120. It is not inside the inn.
- In the 270110 zone table, zone 6 (→ 270120) is a kind-2 ↑ door at x 134400.
- Heal items used from the menu in the field are timed and cancellable, not instant.
- A heal item on a full-HP target is silently refused and not consumed.
- Herbs bought here did not stack (x1 per row).

### Tool gaps
- `control` reads true while the F1 menu is open, so `--until control` cannot tell the menu from free roam. A `menu_open` name is needed.
- Expressions have no list comprehension, so listing a room's zone actors took several `eval` calls.
- `scan` replayed from boot (40 s) because the checkpoint no longer matched the movie.
- Shop and dialogue text types slowly, so `tap ent` can re-enter the shop and buy again.

---
model claude-opus-5-5, effort high, 187 turns, $3.47, 30 min

