# Live character stats, adventure record & per-character save records (EN-SE)

Reverse-engineered **live** against `sotes-trainer-oss.exe` (unpacked EN-SE, md5 `3fe1bc9f…`,
ImageBase `0x400000`, load delta 0) with an external `ReadProcessMemory` scan tool, cross-checked to
the pixel against the status-screen screenshots of **save 15** (the "Loch Carudale" save the
sotes-mod-loader `autoload` mod loads). Party: Arche `0xc35a`, Sana `0xc35b`, Stella `0xc35c`.

Ground truth (save 15 status screen):

| member | Lv | HP  | MP  | Attack  | Defense | Spirit | Resist | Adv | Combat | EXP→Lv |
|--------|----|-----|-----|---------|---------|--------|--------|-----|--------|--------|
| Arche  | 23 | 366 | 76  | 194/106 | 166/90  | 66/58  | 87/63  | 18  | 5/5    | 144000 |
| Sana   | 23 | 286 | 359 | 97/57   | 136/61  | 141/99 | 135/77 | 18  | 5/5    | 145000 |
| Stella | 23 | 297 | 347 | 145/77  | 125/67  | 140/98 | 124/69 | 18  | 5/5    | 146000 |

Shared adventure stats: Marks 20/20 · Monsters Defeated 6478 · Money Gained 373243 · EXP Gained
2343496 · Party Deaths 2 · Longest Combo 32 · Max Dmg Weapon 301 (Arche) / Magic 454 (Stella).

---

## 1. Live character stat block  — `stat_block = *(actor + 0x760)`

Reached the way the trainer / mod-loader roster does: a party actor (found in a `render_root` band by
its `+0x1d4` code) → `+0x760` stat_block. This is the DERIVED/effective block the HUD and status screen
read; equipment and buffs are already folded in.

| off    | field                         | verification (Arche / Sana / Stella)                       |
|--------|-------------------------------|------------------------------------------------------------|
| `+0x00`| **name** char[8] (NUL-term)   | `41 72 63 68 65 00…` = "Arche" / "Sana" / "Stella"          |
| `+0x50`| portrait/element descriptor   | (non-zero; `426fd0` writes it — see base decompile)        |
| `+0x54`| **HP current**                | 366 / 286 / 297                                            |
| `+0x58`| HP base                       | 344 (Arche)                                               |
| `+0x5c`| **MP current**                | 76 / 359 / 347                                            |
| `+0x60`| MP base                       | 64 (Arche)                                                |
| `+0x64`| **Attack** (raw base)         | Sana 57 ✓, Stella 77 ✓ (exact); Arche 88 (<shown 106)      |
| `+0x68`| **Defense** (raw base)        | Stella 67 ✓; Sana 59, Arche 81                             |
| `+0x6c`| **Spirit** (raw base)         | Arche 58 ✓; Sana 90, Stella 90                             |
| `+0x70`| **Resist** (raw base)         | Sana 77 ✓; Arche 55, Stella 64                             |
| `+0x74`..`+0x80` | combat-stat equip/buff deltas (partial) | Arche 88/76/8/24                            |
| `+0x84`| HP **equip** bonus            | Arche 8    → 344+8+14 = **366** ✓                          |
| `+0x88`| MP **equip** bonus            | Arche 8    → 64+8+4 = **76** ✓                             |
| `+0x9c`| HP **buff** bonus             | Arche 14                                                  |
| `+0xa0`| MP **buff** bonus             | Arche 4                                                   |
| `+0xd8`| level_bonus                   | 0 (all)                                                   |
| `+0xdc`| star_count                    | 5 (all — == combat level this save, can't disambiguate)   |
| `+0xe0`| **combat_level_max**          | 5   ("Combat Level 5/5")                                  |
| `+0xe4`| **adventurer_level**          | 18  ("Adventurer Level 18")                               |
| `+0xec`| **EXP current**               | 0   ("EXP 0")                                             |
| `+0xf0`| **EXP to next level**         | 144000 / 145000 / 146000 (EXACT)                          |

Solid, unit-verified: **max HP = `+0x58` + `+0x84` + `+0x9c`**, **max MP = `+0x60` + `+0x88` + `+0xa0`**
(the loader's `stat_max()` already does this). name at `+0x00`. exp_cur/exp_max at `+0xec/+0xf0`.

Caveats:
- The 4 combat stats at `+0x64..+0x70` are the **raw base**. The status screen's "second number" (the
  shown base) = raw + armor/passive, so it can read a bit higher than the raw field (Arche shows
  Attack …/106 while `+0x64`=88); the "first number" additionally folds in the weapon. Sana/Stella's
  Attack equals `+0x64` exactly only because of their loadouts. The per-stat equip/buff decomposition
  lives in the `+0x74..+0x80` region — not yet fully split (a follow-up: the exact display formula).
- **Character "Level" (23) is not stored in the block.** It equals combat_level(`+0xe0`=5) +
  adventurer_level(`+0xe4`=18) = 23 in this save. All three members are 5/18/23, so the split is a
  hypothesis (the SUM is what matches). NB: the mod-loader's `mod.game.roster` `level` field currently
  reads `+0xe0` alone → that is the **combat** level (5), not the character level (23).

---

## 2. Adventure-stats struct  (shared playthrough record)

In the loaded-save low-heap block (this run the canonical copy was near `0x1a19f0`; it is a heap addr
that moves per run, and ~5 copies exist — one canonical + save/render mirrors in the `0x1df…`/`0x1e0…`
heap). Offsets are relative to a base such that the three most distinctive fields land at `+0xa4/a8/ac`:

| off    | field                    | save-15 value | note                                   |
|--------|--------------------------|---------------|----------------------------------------|
| `+0x78`| **gold** (candidate)     | 131385        | screen ~131285 GP — verify (±100)      |
| `+0x90`| Marks Gained             | 20            |                                        |
| `+0xa0`| Total Party Deaths       | 2             |                                        |
| `+0xa4`| **Monsters Defeated**    | 6478          | ← anchor                               |
| `+0xa8`| **EXP Gained**           | 2343496       | ← anchor (3 consecutive dwords)        |
| `+0xac`| **Money Gained** (total) | 373243        | ← anchor                               |
| `+0xf8`| area id (candidate)      | 1010          |                                        |
| `+0x100`| Max Damage — Weapon     | 301           |                                        |
| `+0x104`| └ dealer handle         | `0x05f5e165`  | == Arche's actor handle                |
| `+0x108`| Max Damage — Magic      | 454           |                                        |
| `+0x10c`| └ dealer handle         | `0x05f5e167`  | == Stella's actor handle               |
| `+0x110`| Longest Combo           | 32            |                                        |

Fastest way to find the struct: **scan for the trio {6478, 2343496, 373243} as three consecutive
dwords** (`+0xa4/a8/ac`).

---

## 3. Per-character save record  (persisted summary, 0x11c = 284-byte stride)

Also in the low-heap save block: an array of 284-byte records, one per party member, keyed by the
entity code at record `+0x60`. This run: Arche code `0xc35a` @ `0x1a3b5c` → record base `= code − 0x60`.
Fields (relative to record base):

| off    | field                | save-15 (Arche)      |
|--------|----------------------|----------------------|
| `+0x10`| character handle     | `0x05f5e165`         |
| `+0x28`| HP max               | 366                  |
| `+0x30`| MP max               | 76                   |
| `+0x40`| a stat (Spirit-ish)  | 58                   |
| `+0x48`| Adventurer level     | 18                   |
| `+0x4c`| Combat level         | 5                    |
| `+0x54`| EXP to next level    | 144000               |
| `+0x5c`| Marks (candidate)    | 20                   |
| `+0x60`| **entity code**      | `0xc35a` ← scan anchor |
| `+0x64`| combat level         | 5                    |
| `+0x74`| adventurer level     | 18                   |

This record carries HP/MP/levels/exp-to-level but NOT the derived combat stats (Attack/Def/… are
computed into the live stat block). The character handle `0x05f5e16N` is a stable per-character id
(Arche …65, Sana …66, Stella …67) — the same value the adventure struct stores as the "max-damage
dealer" and that appears in per-actor records.

---

## 4. Party actor list — no fixed-index array in SE  (negative result)

`render_root = *0x92dd38`. The 3 party actors ARE consecutive in an entity array at
`render_root + ~0x118c` (this run `0x1df82174`), but that array is the render band and is **mixed with
non-party actors** and ordered by spawn:

```
+0x0  code 0xc404   +0x4  0xc760   +0x8 Arche(0xc35a)  +0xc Sana(0xc35b)  +0x10 Stella(0xc35c)
+0x14 0xc35d (a 4th character-class code!)   +0x18… code 0 (effects)
```

So there is **no dedicated, fixed-index party array** in the SE build (the base-game `map_obj+0x4030`
model does not carry) — confirming the trainer/loader `PORT-DEBT` note. The correct approach is the
loader's existing `roster_bands` scan of the EFFECT (`render_root+0x1160` ×32) + CHARACTER
(`render_root+0x11e0` ×128) bands for the `+0x1d4` code; the full-heap walk stays as the fallback.

Two subtleties confirmed live:
- A party actor's `+0x1d4` code is the **bare** low word `0xc35a` in some scenes but `0x0001c35a`
  (high word set) in others — so `roster_bands`' `code & 0xffff` mask is load-bearing, while the
  heap-scan's exact `code_index()` only matches when the low word is bare (an asymmetry worth keeping).
- **`0xc35d` is a 4th character-class code**, grouped immediately after Stella (`c35a`..`c35d`
  contiguous) — a lead for a possible 4th playable/ally character registry.

### Reachability caveat
The adventure-stats + save-record structs live in a low-heap "loaded save" block with **no stable
pointer into it** that could be pinned (scans for a pointer to the exact struct bases returned 0 hits).
A mod can find them by value-scan but not yet by a stable chain. Leads to pursue: the game-state
singleton `0x92ac68` (read 0 as a raw pointer this run — likely the object base; trace the save-apply
`0x586c60` write path), or the status-screen draw code that reads these fields.
