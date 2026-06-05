# ods cross-reference — *SotES Data Formats & Values* (Fortune Summoners Fan Discord)

The Fortune Summoners Fan Discord ([invite](https://discord.gg/N68c7pt)) maintains a
spreadsheet, *SotES Data Formats & Values*, reverse-engineering `sotes.exe`'s data: an
address map + struct layouts (Items, Areas, Locations, Actors, Characters, Abilities,
Messages, Mugshot Data) + enums.

**The ods is a cross-reference, not ground truth, and is never in this repo.** It lives
read-only at `../SotES Data Formats & Values.ods` (uncommitted). Our **function
annotation + tracing is the source of truth** (CLAUDE.md); the ods is a lead we
*independently re-verify at the byte level* before any port depends on it. This file
records where our RE and the ods intersect, so the relationship is explicit and our
contributions back to the community are tracked.

## Auditing the intersection
`tools/ods_audit.py` opens the ods read-only and cross-tabs its Addresses/Structs/Enums
against `docs/decompiled/functions.csv` + the port ledger:
```
nix develop --command python3 tools/ods_audit.py            # default sibling path
nix develop --command python3 tools/ods_audit.py "/path/to/SotES Data Formats & Values.ods"
```
Latest run (2026-06-05): **177** of the ods's 222 documented `.text` function entries
match our decompile's function entries (CONFIRM); 45 are ods-only (mostly tiny leaves our
decompile labels/merges differently); 1591 functions we have that the ods's address map
omits. Data-table bases the ods records: **Item Format `0x2c0cd0`**, **Actor Format
`0x27ad34`**, **Character Format `0x2b6ea8`**. (The auditor is a router — its per-field
offset alignment is approximate where a sheet interleaves raw+decoded columns; the
intersections below are hand-verified.)

## Intersections (where our tested RE meets the ods)

| subsystem | ods sheet | our RE (source of truth) | status |
|-----------|-----------|--------------------------|--------|
| **AREA registry** | Areas | `game_world` (ckpt 54): `world_tables_data` lifts the `.rdata` AREA table; `0x585000` reads it. Record stride **0x40**, `+0x00` ID, `+0x04` Name[0x28], `+0x2C` Background, `+0x30` "is safe?", `+0x34` Palette. Area **`0xD2` "Town of Tonkiness"** @ RVA `0x002939C8`. | **CONFIRM.** Independently reached the same table + the Tonkiness entry — and *corrected* the common "Tolkien" (area `0x6E`) confusion: the opening map resolves to area `0xD2`, not `0x6E`. The ods's `+0x38`/`+0x3C` `?` fields are a **gap** our `0x585000` xref can name (proof target). |
| **ROOM (Location) registry** | Locations | `game_world` (ckpt 54): `0x561c90` lookup + `0x585000` reciprocal-exit build. Exit records start at **`+0x1C`**, **3 dwords each** {Exit Point, Destination, Dest Entry Point}. Room **210110** (`0x334be`) @ RVA `0x00294218`... area `0xD2`, scene 1022. | **CONFIRM** the exit-record stride + the room/area linkage. **Contribution:** how map state `0x3f2` *selects* room 210110 (the `0x4c5350` jump table → `map+0x4024 = 0x334be`) is runtime control flow the ods (a data-format sheet) does not cover — host-tested in `game_map`. |
| **Map DATA resource** | *(absent)* | `map_data` (ckpt 56): the per-room visual map is a PE **DATA resource** keyed by scene index ("MSD_SOTES_MAPDATA", dims 88×19×3); the parser **consumes it exactly** (152,936 B, zero remainder). | **GAP → PROOF.** Entirely absent from the ods. Fully proven (bit-exact consumption). Published: [`proofs/map-data-format.md`](proofs/map-data-format.md). |
| **Engine LCG / RNG** | *(absent)* | `rng` (ckpt 31): the MSVC LCG `0x5bf505` / srand `0x5bf4fb`, state `DAT_008a4f94`. | GAP — candidate proof once a second independent confirmation is captured. |

## Gap fields the ods marks `?` (proof targets, in confidence order)
- **Areas `+0x38` / `+0x3C`** (`?`) — our `0x585000` cross-reference propagates area dwords
  into room defaults `room[0x43/0x44/0x45/0x50/0x51]`; mapping which area field is which
  is within reach of the tested `game_world` port.
- **Actors `+0x20` "Level Cap?"**, **Items `+0xAC` "Rarity?"** — flagged uncertain; provable
  once those subsystems are ported + traced.

## Contributing a proof back
When our tracing 100%-proves something the ods lacks or marks uncertain (never a guess
about an address/format), write a human-verifiable proof under `docs/proofs/` (see
`proofs/README.md`) and add a row above. The proof is for human reproduction; our
annotation/tests remain the machine source of truth.
