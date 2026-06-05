# The in-game map DATA resource format ("MSD_SOTES_MAPDATA")

## Claim
Each room's **visual map** (tilemap + object layers) is a **PE `DATA` resource embedded
in `sotes.exe`**, keyed by the room's *scene index* (loaded via `FindResourceA(module,
scene & 0xffff, "DATA")`). The opening town (map state `0x3f2` → room 210110, scene
**1022**) is **DATA resource 1022**, **152,936 bytes**, with this exact layout:

```
offset  size                       field
0x00    4                          magic (value 0x30)
0x04    0x30                       header block
0x34    0x34                       maphdr:
          +0x00  char[0x20]          name  = "MSD_SOTES_MAPDATA"
          +0x20  u32                 dim0  (= 88)   tiles X
          +0x24  u32                 dim1  (= 19)   tiles Y
          +0x28  u32                 dim2  (=  3)   planes (z)
          +0x2C  u32                 count (= 86)   layer entries
0x68    dim0*dim1*dim2 * 0x1c       cell array (5016 cells × 28 B),
                                     z-major: idx = (dim1*z + y)*dim0 + x
        per cell (0x1c B): +0x04 tile id (dispatch key), +0x10 shape/orientation
        selector (0..0xc), +0x0c/+0x14/+0x18 placement params, +0x00 co-id, +0x08 aux;
        an empty cell is all-zero.
then    count × { 0x3c-byte layer header (sub-counts at +0x1c/+0x20/+0x24/+0x28,
                  element strides 4 / 0xc / 0x100 / 8) + those four sub-arrays }
```

The whole resource is consumed **exactly** — zero trailing bytes.

## Why this is proven, not guessed
- `src/map_data.c` ports the engine parser `FUN_00587970` and is **host-tested**
  (`tests/test_map_data.c`); the parse is a pure function of the resource bytes.
- `tools/extract/map_data.py` decodes the real resource and **asserts the parse consumes
  the resource with no remainder** (`152936 == 152936`). A wrong layout cannot consume the
  buffer exactly — the assertion is the trust invariant.
- The load path is read directly from the decompile: `FUN_00586010:690` →
  `FUN_00587970(module, room.scene)` → `FUN_005b62a0` = `FindResourceA(module,
  scene & 0xffff, "DATA")` + `LoadResource`/`LockResource`. Full writeup:
  `../findings/in-game-intro.md` ("The RUNTIME MAP DATA"); changelog: PROGRESS ckpt 56.

## Reproduce it yourself
You need your own legal copy of the game. The repo ships no game bytes; the tool below
ships no assets.

1. Unpack the Steam-DRM wrapper once (produces `vendor/unpacked/sotes.unpacked.exe`):
   ```
   ./tools/setup.sh
   ```
2. Decode DATA resource 1022 and print the per-plane occupancy + histograms:
   ```
   nix develop --command python3 tools/extract/map_data.py \
       vendor/unpacked/sotes.unpacked.exe --id 1022 --cells
   ```
3. Hand-check the header bytes (no tooling) — dump the first 0x68 bytes of the resource:
   ```
   nix develop --command python3 tools/extract/map_data.py \
       vendor/unpacked/sotes.unpacked.exe --id 1022 --raw
   ```
   At resource offset `0x34` you will see the ASCII `MSD_SOTES_MAPDATA`, and at `0x54`,
   `0x58`, `0x5C` the little-endian dwords `58 00 00 00` (88), `13 00 00 00` (19),
   `03 00 00 00` (3).

## What you should see
- Resource length **152,936 bytes**; the decoder reports it **consumed the resource
  exactly** (the trust invariant — a wrong format would leave a non-zero remainder or
  overrun).
- Name `MSD_SOTES_MAPDATA`, dims **88 × 19 × 3**, **86** layer entries.
- A coherent town backdrop: of 5,016 cells, **160 populated** — `z=2` the near plane
  (bottom ground/floor strips), `z=0` the far plane (rooftop runs), `z=1` mid details;
  tile ids cluster in the `0x1b58b` family + `0x29ff4`.

## ods status
**Absent.** The community spreadsheet documents struct layouts (Items/Areas/Locations/
Actors/Characters/Abilities/Messages/Mugshot) + an address map + enums, but has **no
sheet for the map DATA resource format**. This proof contributes that format, with a
reproducible exact-consumption check. (See `../ods-crossref.md`.)
