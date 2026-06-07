# The in-game map's object-placement layers — the actor SPAWN list

## Claim
Each room's map DATA resource ends with `count` **object-placement layers** (the
opening town, DATA 1022, has **86**).  Each layer's **0x3c-byte header IS one
object's placement record**:

```
hdr+0x04  i32  x     (tile px; the spawn multiplies by 100 -> world x)
hdr+0x08  i32  y     (tile px;                              -> world y)
hdr+0x10  u32  TYPE CODE   <- both the spawn-dispatch key AND the spawned
                              actor's behaviour code (entity field +0x1d4)
hdr+0x18  u16  sub-type / variant (forwarded to the spawn)
hdr+0x00  u32  a small per-instance id (0x0..0x6a here)
```

The room-object pass **`FUN_0058d460`** (called from `FUN_00586010:698`, right
after the map parse) walks these `count` entries and dispatches each by the **range
of its type code** into one of four pre-allocated actor-pool bands off the
room-state god-object `DAT_008a9b50` — scanning the band for a free slot and
aborting with a named `"<kind> Object Count Over"` debug string if it is full:

| type-code range | kind      | band      | spawn fn       |
|-----------------|-----------|-----------|----------------|
| 50000–59999     | EFFECT    | `+0x1160` | `FUN_0041f200` |
| 60000–69999     | STRUCTURE | `+0x2560` | `FUN_00438a60` |
| **70000–79999** | **CHARACTER** | **`+0x11e0`** | **`FUN_00431e30`** |
| 80000–89999     | DEVICE    | `+0x13e0` | `FUN_00557550` |

The **CHARACTER** band (`+0x11e0`) is the one the per-frame renderer
`FUN_00491ae0` walks (engine-quirk #78).  So **the town's static NPC/scenery
actors are precisely the 70000-range object layers**, and a character object's
type code becomes its actor `+0x1d4` behaviour code verbatim
(`FUN_00431e30` sets `actor+0x1d4 = type`).  For DATA 1022 that is **32 character
objects**, and their type codes — with multiplicities — are **identical to the 33
live actor codes captured at the town hold** (flip 1500, `--seed-pin --lockstep`,
engine-quirk #78), the 33rd being one animated actor (code `0x1872d` = 100141,
outside the 70000-range, spawned by a separate path).

This resolves the open ckpt-76 question "the town behaviour codes are never
assigned as literals in the decompile": they are **not constants at all** — they
are read straight out of the map resource's object layers.

## Why this is proven, not guessed
- The dispatch is read directly off the decompile: `FUN_0058d460` reads
  `header[+0x10]` as the type, range-checks it, scans `DAT_008a9b50 + <band>` for a
  free slot, and calls the per-band spawn fn — each branch carrying its own
  `"Effect/Structure/Character/Device Object Count Over"` string.  The character
  spawn `FUN_00431e30` writes `actor+0x1d4 = type`, `actor+0x1d0 = 1` (active),
  `actor+0xfc = 9` (draw layer) — the same fields read live by `FUN_00491ae0`.
- `src/map_data.c` ports the parser (`FUN_00587970`) and is host-tested; the
  object layers are the same array it already exposes (`map_layer.hdr[0x3c]`).
- The byte-level cross-check is a **pure function of the resource bytes**: decoding
  `header[+0x10]` for every layer and histogramming by range reproduces exactly the
  live actor-code census (10× `0x112e6`, 7× `0x111d6`, 3× `0x1129e`, 2× `0x112e2`,
  2× `0x11365`, and `0x112e5`/`0x1129f`/`0x111d9`/`0x111f2`/`0x1136f`/`0x11366`/
  `0x11367`/`0x11370` ×1).  A wrong field offset cannot reproduce the live census.
- Full writeup: `../findings/in-game-intro.md` "The town actor SPAWN".

## Reproduce it yourself
You need your own legal copy of the game; the repo + tool ship no game bytes.

1. Unpack the Steam-DRM wrapper once (`vendor/unpacked/sotes.unpacked.exe`):
   ```
   ./tools/setup.sh
   ```
2. Decode DATA 1022's object layers:
   ```
   nix develop --command python3 tools/extract/map_data.py \
       vendor/unpacked/sotes.unpacked.exe --id 1022 --objects
   ```

## What you should see
- `consumed 152936 / 152936 [EXACT]` — the whole resource parses (trust invariant).
- The band summary: **EFFECT ×15, STRUCTURE ×39, CHARACTER ×32, DEVICE ×0** (86 total).
- A type-code histogram whose **70000-range (CHARACTER)** rows are exactly the 13
  distinct town actor codes above, with those multiplicities.
- The 32 character objects listed with their world (x, y), e.g.
  `layer[1] code=0x111d6 x=2144 y=416`, `layer[49] code=0x112e6 x=624 y=288`.

## ods status
**Absent.** The community spreadsheet has Actors/Characters struct sheets but **no
map object-placement / spawn-list format** and no link from a map's object layer to
the actor band it spawns into. This proof contributes that format + the
type-range → actor-band dispatch, with a reproducible byte-level check against the
live actor census. (See `../ods-crossref.md`.)
