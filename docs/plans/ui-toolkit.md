# Plan — the retail UI widget toolkit on the retail layout (0x40d000..0x41a000)

Status: chip 1 DONE 2026-10-16 (`src/ui_core.c`, 24 functions, hybrid 379); chip 2 DONE 2026-10-16
(`src/ui_text.c`, 12 functions, hybrid 391; quirk #148 — the SJIS wrap / ruby paths are JP-only: unexercised).
Chip 3a DONE 2026-10-16: the builders (`src/ui_core.c` +7) + the shop's screens (`src/ui_menu.c`, 6), hybrid 404.
Chip 3b DONE 2026-10-16: the runtime (`src/ui_run.c`, 11: keys, ticks, message update, list input, the menu
frame loop `0x55f190` + pacer as REPLACE-only), hybrid 415; quirk #150.  Chip 3c DONE 2026-10-16: the shop
(`src/ui_menu.c` +7: driver / stock list / "On who?" REPLACE-only, the equip preview + its sprite / stat rows /
member rows shadowed; `src/ui_core.c` +4 leaves), hybrid 429; quirk #151 — the preview's recolour path (an item
with a palette swap table) and the buy confirm are unreached by the corpus.  Shadow note: a function whose
`this` is a caller's stack object can show a false divergence (the shadow restores only below ESP between its
retail and port phases) — confirm with REPLACE (`0x412570` did).  Owner: the porting loop (rhost shadow per function, REPLACE in the hybrid).

## Why
The menus (field / title / shop / status), the message box and the cutscene runner `0x439690` all sit on
one widget toolkit.  The old port reduced pieces of it on its own structs (`menu_list.c`, `glyph_*.c`,
`newgame_*`, `dialogue.c`) — those FUN_ tokens make the ledger count the band as touched, but none of it runs
on the retail layout.  Measured 2026-10-15: the band is 49.6 KB (109 functions), 33.4 KB of it reached by
the corpus; 159 reached functions (~197 KB, incl. the boot / asset loader / title app) are touched only by
old-port modules.

## Object model (from 0x4124d0 / 0x40f3e0 / 0x411c50 / 0x40df40 — VERIFY each offset in the asm: IDA's
names for the runner were 4 bytes off past +0x2e0, quirk #144)
- **Window manager** (room loop +0xe8, sys-side ones too): {+0 window list, u16 +4 cap, u16 +6 count};
  `0x4124d0` takes the next window (`0x40f3e0` init), `0x414080` marks only the last one topmost (+8).
- **Window** (0x84 B): +0 its manager, +4 live, +8 topmost, +0xc..+0x18 the rect, +0x1c, +0x48 control
  list, u16 +0x4c cap, u16 +0x4e count, +0x50, +0x54, +0x58, +0x5c a 0x24-byte style (copied), +0x80.
  `0x40f3e0` frees the old controls (`0x40e0c0` + delete) and allocates `cap` fresh 0x1b0-byte controls.
- **Control** (0x1b0 B): +0 window, u16 +4 index, +8 kind (1 grid / list `0x411c50`, 2 text `0x40df40`),
  +0xc / +0x10 position, +0x14 / +0x18, +0x20, +0x84, +0x140, +0x170 the text node (0x2c), +0x174 the grid
  info (0x24: [1] rows, [2] cols, [4] used rows ...), +0x178 the column cells (36 B: x, ?, w, ?, +0x10..),
  +0x17c the rows (16 B, +0xc → 24-byte cells {text, icon 0x54, gauge 0x20, ...}); `0x40e0c0` (ported:
  `ctrl_geometry_free`) frees them.

## Port order (bottom-up; each chip: shadow per function + the hybrid)
1. allocators / setters / leaves: `0x414080` `0x4124d0` `0x412c10` `0x4173d0` `0x40fdb0` `0x412100`
   `0x40f5a0` `0x412570` `0x40f3e0` `0x40dee0` `0x40df40` `0x411c50` `0x411f40` `0x40e2f0` `0x411ec0`
   `0x414c40` `0x412d30` `0x413760` `0x4192b0` `0x415420`; the inventory / money leaves `0x415930`
   `0x417440` `0x415990` `0x416500`.
2. ~~text~~ DONE: `0x40fa00` (cell text), `0x40f040` / `0x40e5e0` (parse / wrap), `0x4031c0`, `0x43cf30` /
   `0x43cf90`, `0x413ed0`, `0x410610` (the message key table), `0x410560`, `0x411230`, `0x40e360`; reuses
   `glyph_text.c`'s `0x40fd20`.  Slots left: the ruby context test `0x4034f0` (7 KB) + reading writer `0x4051d0`
   (3 KB) — need a JP-text scenario (ense-jp) before porting.
3. the menus.  3a DONE: grid init `0x40f5c0`, grid new `0x4134f0`, row add `0x40f800`, cell icon `0x415860`, title
   `0x412c40`, framed window `0x40f1c0`, page arrows `0x4103d0`; the shop's message window `0x4123a0`, party status
   `0x4129b0` / `0x412df0`, "You have N" `0x415a10`, item fit `0x417480`, sprite set release `0x415350`.  3b DONE: the runtime (`src/ui_run.c`, above).  3c DONE: the shop
   driver `0x415060`, the stock list `0x415460`, "On who?" `0x415f60` (+ `0x4167a0`, the equip preview `0x4169e0`,
   `0x4175a0`, `0x418c90`), the leaves `0x412330` `0x4173f0` `0x412160` `0x411940` `0x4078a0` `0x41a140`.  3d next
   (each needs a scenario that reaches it — USER `rhost.py record`: buy a consumable, sell, equip from the shop): the
   buy confirm `0x415ae0` (+ `0x4190d0` yes/no, `0x413b70` / `0x4140c0` message boxes, `0x412800`), the SELL list
   `0x4192f0` (+ `0x4199b0` `0x419c70` `0x419eb0`), the gear-locked box `0x418ec0`; the input re-poll `0x40fe00`
   (DirectInput) stays a slot.
4. the cutscene runner `0x439690` on top (its other callees: `0x43c650` `0x43d830` `0x43e5b0` …).
Render / DirectDraw (`0x4178e0` `0x417c40` `0x418470` `0x4184a0` `0x4188b0`) stay platform slots.
