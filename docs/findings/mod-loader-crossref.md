# Mod-loader ↔ port cross-reference (audit 2026-11-28)

Source: `../sotes-mod-loader/docs/HANDOFF.md` (+ its `tools/symgen_manual.tsv`), checked claim by claim against
the EN-old IDA export (and the `ense-en` export where SE-only). Verdict: **nothing behaviour-changing** — the
port's rhost-gated code already does what retail does; 9 RECORD errors (docs / struct comments / tool data)
on the port side, 1 wrong claim on the loader side. EN-old VAs unless marked SE.

## Port records that were wrong (fixed in place 2026-11-28)

| # | where | was | is (evidence) |
|---|---|---|---|
| 1 | `party.h`, `sdt-save-format.md`, `freeroam-hud.md` §10 | `+0xd8` level bonus, `+0xdc` element-star count, `+0xe0` combat max | `+0xd8` CURRENT combat level, `+0xdc` MAX combat level (HUD star count), `+0xe0` adventurer level; "Lv" = `+0xd8 + +0xe0` (`0x494e60`: loop to `v12[55]`, `0x498620` frame `16 + (i <= v12[54])`, `"%d"` of `v12[54] + v12[56]`). Cause: SE offsets (= EN-old + 4 from `0xd4` on) applied unshifted. Save-15 live data fits shifted: 5 / 5 / 18, Lv 23 |
| 2 | `CLAUDE.md`, `SE_CODE_MAP.md`, `title-menu-state.md`, `save15-live-stats.md`, trainer | SE VAs `0x437c70`, `0x5e2a10`, `0x92d5bc`, `0x92dd38` as current | OLD SE build (md5 `3fe1bc9f`). The 2026-08-09 Steam SE update (md5 `c382ced3`, the `ense-en` export) moved them: `0x437c70` is mid-function; world pointer `0x92dfd8` (`tools/rhost/rhost.c`) |
| 3 | `save15-live-stats.md` §4 | "no fixed-index party array in SE" | world → room (`+0x3788` SE / `+0x2784` EN-old) → `slots[8]` at room `+0x4030` (loader v2 gate vs HUD, 3 editions; not re-run here) |
| 4 | `SE_CODE_MAP.md` | area records 0x150 + 8-B alloc header, no global reaches the table | 344-B records from table+4, count `+0x56004` (SE `area_find` `0x565a20`); EN-old 336 B, count `+0x54004`, table at world `+0x103c` |
| 5 | `SE_CODE_MAP.md` base-game notes | map object = `*(root+0x1044)` | `+0x1044` = the room loop; the 0x4120-B room = world `+0x2784` (`world_layout.h`) |
| 6 | `asset_register.h` | `ar_sprite_slot +0x3c`, `ar_sound_slot +0x10` = "settings pointer" | the resource HMODULE (`0x4178e0` → `0x5b7800` → `FindResourceA`; `0x563ef0` → `0x5bb740` `FindResourceA(.., "WAVE")`) |
| 7 | `asset_register.h` | `ar_info_entry +0x00` "live marker 1/2", `+0x10` unknown | `+0x00` = mirror frame offset (`0x44ce94`: `ax = [*(sys+0x18e0 + bank*4)]; inc ax`); `+0x10` = dynamic-tint-clone mark (banks 884..908; quirk #141) |
| 8 | crosswalk TSV (`decompiled-ida/crosswalk_enold_ense-en.tsv`) | `0049cd70 ↔ 004981c0`, `00407880 ↔ 005e8f10`, `0041c2f0 ↔ 004154f0` | WRONG pairs (method-order matches, same=0). SE `script_end` = `0x497bd0` (after `script_begin` `0x4979b0`); the other two are a list row-slot accessor / a free loop on SE. Fixes: loader `tools/symgen_manual.tsv:44-48`. Treat same=0 crosswalk rows as leads only |
| 9 | `world_layout.h` room `+0x4024`; `tools/extract/lizsoft_sprite.py` docstring | "script a zone result runs"; "W/H not in the file" | the current AREA id (fed to `area_find`); W/H are in the file (`docs/formats/lizsoft-sprite.md`) |

## Loader-side notes (tell that project)

- WRONG there: "an event script's frame loop calls `room_warp_tick` and drops its result". Single direct caller:
  `room_loop_run` (`0x58fc17`; SE `0x5b6c3d`), which returns the result. The lost warp needs another cause.
- CONFIRMED for it: EN-old drop fields the loader marked "inferred" — `+0xf0` EXP, `+0xf4` coins, `+0xf8` item,
  `+0xfc` chance (ported `char_die`, `char_hit.c`).

## New retail facts from the loader (not yet gated here — LEADS until a port depends on them)

- **Names** (added to `tools/ida/retail_fields.json`): `0x406500` item_take (removes the unstored record with
  the smallest count; `this` needs only `+8` = room), `0x41c390` inn_run (ev, price CP, 1, mode 0, speaker, offer,
  decline, broke, morning, bye), `0x41a160` inn_rest (clears world tick `+0x1c` after), `0x5b14c0` display_restore
  (re-decodes every bank; only from `0x5b12e0` `WM_ACTIVATEAPP`), `0x57b280` sound_registrar, `0x561c90` area_find.
- **Sprites:** slot `+0x34` = per-cell trim records, 24 B {x0, x1, y0, y1, has-opaque, has-key}, count `+0x38`;
  `slot_clone` copies them (stale ones hide cells). Character `+0x48` u16 = track-0 bank (live-swappable); body clip
  `+0x6c`, timer `+0x70`, frame `+0x72`, done `+0x74`. Character `+0x274` = display name (20 B copy of the
  stat-table name); stat table `+0x8a` = English name. Portrait banks: 24-bit sheets, 160×176 cells. Icon sheet =
  DATA 1451 (256×1024, 8-bit), feeds menu + world banks. Pickup sparkle 100143 keeps the item id at `+0x25c`.
- **Area table:** holds rooms with NO map data (999999, the 110/120/130 series; 20 on EN-old) — loading one =
  fatal "The map data is not found.". Record `+0x114 == 2` = town → world `+0x1030`; in a town hits do no damage
  (`char_hit.c` gates on it).
- **Map records:** a character record's x/y = top-left of its sprite cell (`char_init` centres x, feet on the
  cell's bottom row); field A[4] ≠ 0 = facing left. Chest 70220: A[2] id (handle = room×1000 + id when < 1000),
  A[7] item (1/2/3 = copper/silver/gold coins), A[8] count. Perpetuity table = 2048 entries; overflow = fatal
  exit (NOT read in the decompile here).
- **Text / scripts:** `text_wrap` (`0x40e5e0`): a `%n` inside a run of letters joins both halves into one word
  for wrapping. A kind-0 message box with no actor shows nothing and its wait returns at once. Effect wait
  (`+0x20 = 2`) with no effect running returns after ~80 frames whatever `+0x2c`. Book: title[0x28], then 2052-B
  pages {text[2048], u32 end}. Shop stock = 0-terminated int32 id list.
- **Music manager** (`sys+0x2884`): `+0x18` track table (56 B: dev, id), `+0x1c` count, `+0x28` master volume in
  1/100 dB (written by `0x5bbe20` from the config), current id = manager `+4`; a track's u16 `+20` = 2 while
  fading out.
- **Items:** `0x5b1f10` case 10: stat `+0x68` (1..6) gains `+0x6c + rand·+0x70`. Resource ids COLLIDE across
  modules (map DATA 1026 in the exe vs a sheet in `sotesd.dll`).

## Agreement / scope

~30 loader claims AGREE with the port (bank counts + tables, presence classes, room inventory / gold / perpetuity
offsets, checkpoints 10020 / 10030, message-box geometry, choice list, 0xdc stat stride, 736-B item rows, sound
pool, `bgm_pick`, zone / touch pass, 122 `char_init` codes). ~15 are SE-only (344-B area records, SE item caps
80 / 128, the 910000+ generated-gear table, SE arg-count changes, 925 / 929 banks, SE item use `0x5da1f0`).
Not checked: `tools/ennse_voice` still on old-build VAs; SE live measurements (jump heights, carry caps).
