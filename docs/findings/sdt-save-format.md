# `savedataNN.sdt` save format (EN-SE) — container codec + summary layout

RE'd off `sotes-ense-en.exe` (unpacked, ImageBase 0x400000; VA = fileoff + 0x400000).
Verified against all 8 real saves (`…/steamapps/common/sotes/user/savedataNN.sdt`).
Reference impl: **`tools/sotes_save/`** (dep-free lib + `sotes_save_dump` CLI); the trainer
(`tools/sotes_trainer`) compiles it in for its `saves`/`saveinfo`/`load slot:N` commands.

Applies to Main-Quest saves (`.sdt` **and** `.bak`); `wmapNNNNN.sdt` (world-map state) use
a **different** container (rejected by the magic check). The port + a save editor both need
this — it is the authoritative serialization, not a guess.

## Container (the crypto — fully RE'd, this is the hard part)

File = a plaintext 0x14-byte prefix, then an obfuscated body:

```
+0x00  u32  prefix_len   = 0x10   (bytes of header that follow the len word)
+0x04  u32  magic        = 0x2711
+0x08  u32  bodysize             (decoded body length; 20 + bodysize == file size)
+0x0c  u32  val3                 (per-save; checksum-ish, not yet pinned)
+0x10  u32  seed                 (cipher seed)
+0x14  …    bodysize obfuscated bytes
```

Decode (loader `FUN_00416550` → archive class `FUN_005dee40` @0x5def06), per body byte i:

```
key      = (seed >> 8) & 0xff
plain[i] = INV_KEYSTR[ (cipher[i] - key) & 0xff ]
```

`INV_KEYSTR` = the inverse permutation of the 256-byte key table `P` at **VA 0x5fd290**
(`FUN_005df030` builds `KEYTABLE[P[j]] = j` from it; the archive-reader trio 0x581000 keys
all three memobj slots with the same `P`). `P` is a verified permutation of 0..255; embedded
in `sotes_save.c`. ENCODE (editor/writer): `cipher[i] = (P[plain[i]] + key) & 0xff`; `seed`
is free as long as `key` derives from it.

The reader is a 3-mode archive class (mode 1 `fopen`, mode 2 in-memory, mode 3 raw
`ReadFile`); the `.sdt` main-quest path loads the whole file into a memobj (`0x5dee40`,
decoding there) and serves decoded slices via `0x5deb70`. Header magic/seed are read from
the PLAINTEXT prefix; only the body is obfuscated.

## Decoded body = a record stream

Record 0 is `[u32 len=0x25c][604-byte metadata]` (the loader's first read). Past it the
stream mixes length-prefixed and FIXED-size reads (a nested C++ object graph: per-member
0x11c blocks at `S+0x28a8 + j*0x11c` with sub-arrays, then inventory lists, then a
0x10c-byte entity table) — do NOT walk it as flat `[len][payload]` past record 0.

### Metadata block (604 bytes, record 0) — mostly heap noise, clean tail
memcpy'd C++ object: live heap pointers + leftover heap strings (e.g. `" to state STARTED "`).
Stable/meaningful fields only in the tail:

```
meta+0x228  u32  checksum   (per-save; additive body-sum candidate, unverified)
meta+0x22c  u32  magic      = 0x2711
meta+0x230  u32  handle     = 0x2738  ("Main Quest" category; loader gate + apply gate)
```

(`handle` filename map, loader switch: `1`→`savedata%02d.bak`, default→`savedata%02d.sdt`,
`0x2724`/`0x272e`→bonus/"Disk" saves; `0x2738`==Main Quest is the only value that both reads
`.sdt` and applies at `FUN_00586c60`.)

### Party-header grid + roster (the summary)
The metadata block is a **fixed 604 bytes**, so a 16×u32 party-header grid always begins at
**body 0x260**. Several grid fields grow monotonically over a playthrough (candidates:
progress / playtime / gold) — exposed RAW by `sotes_save` (`info.ph[16]`), deliberately
UNLABELED: not yet pinned, and this project does not ship guessed field names.

The playable **roster** is recovered by scanning the decoded body (post-metadata) for the
known character codes — each appears exactly once:

```
0xc35a Arche   0xc35b Sana   0xc35c Stella     (base_stat_table.c)
body[code_off + 4]  = level_base  (== in-memory stat +0xe0; NOT the display level —
                                    the SE derives the shown level from EXP)
```

VERIFIED: magic+handle 8/8 saves; roster matches party growth (slot1 = Arche+Sana only, no
Stella; the earliest/smallest save); `level_base` 3 (early) → 5 (Lv17 tower) matches the
live stat-block read.

## Open / next (not blocking the summary)
- Pin the party-header grid fields (gold / playtime / progress) — needs a live gold/playtime
  read to anchor an offset, or RE of the save-select menu draw. Exposed raw meanwhile.
- Display level: `exp_max` sits at `body[code_off-0xc]` (slot7 Arche = 50000 = the Lv17
  table value); a (code,exp_max)→level table lookup would yield the shown level. The SE stat
  table isn't dumped yet (only the base game's `base_stat_table.c`); deferred.
- `val3` (header +0x0c) and `meta+0x228` checksum semantics — needed for a re-save/editor
  that the game will re-load without a validation failure.
