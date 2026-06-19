# NPC colour variant — the townswoman's palette swap (THEME 2, note #1)

> **Status: RESOLVED + PORTED + bit-exact (ckpt 142).** The townswoman now renders
> her variant colours (blonde / pink dress) **pixel-identical to retail** — the crop
> at tick 274 (137,290 48×89) is `differ_px == 0` (was 1387), full-frame 1518→108 (the
> residual is the pre-existing butterfly/ambient noise, no regression). The fix: apply
> the cloned variant bank's palette-index remap to the 8bpp pixels at decode, AND fix
> the info-event/clone pass order so the remap pointer survives (see "The fix" below).
> Retires `PORT-DEBT(effect-color-variant)`.

## The gap (USER studio note #1)
`osr_notes.jsonl` tick 274, crop (137,290 48×89), differ 1518 (crop differ_px 1387):
"npc color variant gap". Reconstructed port|retail (notes.py --render, on the feed):
the townswoman by the inn renders **brunette in a blue dress** in the port,
**blonde in a pink/red dress** in retail — SAME sprite shape + arms-crossed idle
pose, a different palette (NOT a uniform colour-grade: hair brown→blonde, dress
blue→pink). She is res **0x461** (1121), fr 1, 34×85 — the draw call is **byte-identical**
port↔retail (same res, frame, dst, src, state); only the decoded SHEET pixels differ.

## Which actor (ground truth, port spawn dump)
Booting the port to town with `OSS_DUMP_EFFECTS` over `nav-matched`, the EFFECT
townsfolk + their map fields: **exactly one has `+0x2c` ≠ 0** —
`code=0xc440 x=896 y=416 +0x18=0x0 +0x2c=1`. Every other townsfolk + the 4
butterflies have `+0x2c=0`. So the girl is the generic map **townswoman `0xc440`**
("Woman" archetype, `s_Woman_00855174`) with **colour variant = 1**.

## The mechanism (RE'd from 0x41f200 + 0x58d460 + 0x57ca40 + the .rdata)
1. **The colour variant = `param_11`, the map record field `+0x2c`** (`puVar14[0xb]`).
   The dispatcher `0x58d460:151` passes it as `0x41f200`'s `param_11`; the spawn case
   `0x41f200:1768` (code 0xc440) resolves the sprite **bank** `sVar17` from it:
   `param_11==0 → 0xa5` (base), `1 → 0xa6`, `2 → 0xa7`, `3 → 0xa8`. Then
   `0x426d70(0, sVar17, 0)` stores it at sprite-row `+0x48` (the render bank). Every
   townsperson archetype (`0xc472`/`0xc473`/`0xc486`/… and `0xc440`/`0xc441`/…) has
   this 0/1/2/3 → bank arm. The standing townsfolk pass `param_11=0` (base); only this
   woman is `param_11=1`.
2. **bank → res**: `ar_pool_get_slot(bank)` = `g_ar_sprite_slots[bank-0xd]`. bank 0xa5
   → slot 152 → res 0x461 (the woman BASE sheet, registered in `GROUP3_SPRITES`); bank
   0xa6 → slot 153.
3. **The variant banks are CLONES of the base + a palette-remap pointer** — the retail
   registration `0x57ca40:2870-2875`:
   ```
   FUN_005748c0(slot152, …, 0x461, …)     // bank 0xa5 base, res 0x461
   FUN_004179b0(0xa6, 0xa5); *(info[0xa6]+8) = &DAT_006748d0   // clone + remap, variant 1
   FUN_004179b0(0xa7, 0xa5); *(info[0xa7]+8) = &DAT_00674ad8   //                variant 2
   FUN_004179b0(0xa8, 0xa5); *(info[0xa8]+8) = &DAT_00674ce0   //                variant 3
   ```
   So bank 0xa6 = a clone of the base sheet (same res 0x461, same pixels) + an
   info-entry `+8` pointer to a palette-remap table. The 3 tables `DAT_006748d0/ad8/ce0`
   are SHARED across every woman archetype (0xc441's `0xaa/0xab/0xac` clones reuse the
   same 3 pointers, `:2879-2883`).
4. **The sheet is 8bpp indexed** (port decode probe: res 0x461 `bpp=8 f_08=0`). So the
   variant is a **palette-index remap on the 8bpp pixels**, NOT a brightness/24bpp lut.

## The remap-table format (extracted from the unpacked exe .rdata)
`DAT_006748d0/ad8/ce0` are **520 bytes each, identical except a 48-byte region at
offset 258**. They are an **index→index remap**: a base run maps a 48-colour range to
palette indices **0x20–0x4f**, and the differing region redirects it to:
- variant 1 (`DAT_006748d0`): **0x50–0x7f**
- variant 2 (`DAT_00674ad8`): **0x80–0xaf**
- variant 3 (`DAT_00674ce0`): **0xb0–0xdf**

i.e. the woman sheet's embedded palette holds FOUR 48-colour banks (blue / pink /
… / …) and the remap selects which bank the dress+hair indices read from. (File
offset = `0x1cc000 + (VA − 0x5cc000)`; e.g. `DAT_006748d0` → file `0x2748d0`.)

## What the PORT already has vs what's missing
ALREADY ported (`src/asset_register.c`):
- the clone: `group3_clones[] = {…, {0xa6,0xa5}, {0xa7,0xa5}, {0xa8,0xa5}, …}` (L2870-74).
- the remap pointer: `group3_info_events[] = {…, {DATA_SET,166,0x006748d0}, {…,167,…ad8}, {…,168,…ce0}}`
  (L2871-75), stored on the slot as `slot->data = ev->payload` (the opaque .rdata VA).
- the 8bpp decode + base palette (the woman renders, correctly blue/base).
- the spawn already selects bank 0xa6 for `0xc440` (TOWN_EFFECT_DEFS captured the
  param_11=1 result — correct for THIS town; reading `+0x2c` to generalise is a
  follow-up, not the colour bug).

## The fix (ported ckpt 142)
Two parts, both in `src/asset_register.c`:

**(1) The remap data + the decode consumer.**
- `tools/extract/npc_palette_remap.py` reads the 3 tables out of the user's
  sotes.exe .rdata and emits the flat per-variant `remap[old]=new` (the table is
  TWO parallel 256-byte arrays — half1[k]=source palette index, half2[k]=dest —
  so `remap[half1[k]]=half2[k]`; net: indices 0x20-0x4f → 0x50-0x7f / 0x80-0xaf /
  0xb0-0xdf for v1/v2/v3, identity elsewhere). Baked into `NPC_PALETTE_REMAPS[]`
  keyed by the retail .rdata VA; `ar_npc_palette_remap(va)` is the lookup.
- `ar_sprite_decode`: after `bs_decode_resource`, for an **8bpp** sheet whose
  bank's info-entry `->data` is one of the 3 VAs, rewrite each pixel index through
  the remap **before the slice** — so the SAME (cloned base) sheet slices against
  the embedded palette into the variant colour bank. The bank = the slot's pool
  index (`ar_pool_get_slot` inverse: `(slot - g_ar_sprite_slots) + 0xd`).

**(2) The pass-order bug (the actual blocker).** `FUN_004179b0` (the clone) DOES
clear the dst info-entry's `+8`/`+0xc` (via `in_ECX + 0x18e0`, NOT the global flip
table) and copy marker/flag from src — the port models this faithfully. But retail
issues each clone-dst's DATA_SET **after** its clone (L2870 clone → L2871 data-set),
whereas the port batched **all** info-events then **all** clones — so the clone
WIPED the remap pointer the info pass had set (`info[0xa6]->data` read back 0).
Fix: `ar_reapply_group3_data_events()` re-runs the DATA_SET events once more after
the clones. PROVEN exact: all 98 clone-dst data-sets follow their clone in retail
issue order (0 exceptions), so the data-set is always the last writer; marker/flag
need no re-apply (clone-copied from src, which the first info pass set).

## Verified (ckpt 142)
Port `.osr` over `nav-matched` (`C:\oss-osr\port-npc.osr`) vs `retail.osr`, the
studio crop at tick 274 (137,290 48×89): **`differ_px == 0`** (was 1387) — the woman
renders blonde/pink, pixel-identical. Full-frame 1518→108 (residual = butterflies,
no regression). Host: `npc_palette_remap_lookup` + `reapply_data_events_restores_after_clone`
(1026 pass). The bank PICK (TOWN_EFFECT_DEFS bank 0xa6 for 0xc440) was already the
captured param_11=1 result — correct for this town; reading `+0x2c` to derive it
generally is a small follow-up under `effect-sprite-table` (the whole captured table).

Cross-refs: `docs/port-debt.md` (`effect-color-variant`), `docs/findings/0057ca40-rabbit-hole.md`
(the info table + the deferred palette consumer), `docs/findings/butterfly-direction-sprite.md`
(the SEPARATE +0x18 path — the butterfly took its frame_base from the variant, the
townsfolk DON'T; this is the orthogonal +0x2c colour path), `src/asset_register.c`
(`group3_clones`, `group3_info_events`, `ar_sprite_decode`).
