# Menu-list controller — cursor nav, scroll, and the action latch

> Milestone 0/1.  The object that drives the title top-level menu (and most
> of the engine's selection UIs).  Three functions ported in checkpoint 4
> (`src/menu_list.{c,h}`: scroll / nav / latch); the controller's **geometry
> allocate/free pair** (`menu_ctrl_build` / `menu_ctrl_clear`) landed in
> checkpoint 5.  What remains of the spawn block is the per-row *populate*
> (cheap inline appends in `0x56aea0`) plus the two lazy cell finalizers
> (`0x40f3e0` menu-item builder, `0x411f40` grid-cell finalizer).

## The objects

The menu controller (`menu_ctrl`, the `this`/ECX of all three functions)
points at a **list header** at `+0x174` and, for confirm boxes, a separate
**confirm list** at `+0x170`.  Only the touched fields are modelled.

### Controller (`menu_ctrl`)

| off    | field   | meaning                                                    |
|--------|---------|------------------------------------------------------------|
| `+0x00`| `sub`   | input "ready" gate sub-object (latch reads `+0x04`,`+0x54`) |
| `+0x08`| `mode`  | latch dispatch: 1 = cursor-nav list, 2 = confirm box       |
| `+0x1c`| `action`| last resolved action code (latched by nav / latch)         |
| `+0x170`| `list2`| the type-2 confirm/message list state                      |
| `+0x174`| `list` | pointer to the list header below                           |

### List header (`menu_list_hdr`, `*(ctrl+0x174)`)

| off    | field      | meaning                                                |
|--------|------------|--------------------------------------------------------|
| `+0x00`| `type`     | scroll model: 0 linear-wrap / 2 grid / 3 trailing-page |
| `+0x0c`| `stride`   | page / row stride (entries per page-row)               |
| `+0x10`| `count`    | total entry count                                      |
| `+0x14`| `cursor`   | current selection index                                |
| `+0x18`| `sel2`     | page-top / wrap-anchor (meaning is type-dependent)     |
| `+0x1c`| `repeat_a` | axis-A auto-repeat deadline (GetTickCount domain)      |
| `+0x20`| `repeat_b` | axis-B auto-repeat deadline                            |

`sel2`'s type-dependent reuse is quirk #32.

## The three ported functions

### `FUN_004192b0` → `menu_list_scroll_into_view` (52 B)

Recompute `sel2 = floor(cursor/stride)*stride` (the page-top containing the
cursor) via a step-search loop; return 1 if it moved.  The same loop recurs
inside the nav engine, so it is factored out as `page_top()`.  Assumes
`stride > 0` and `cursor >= 0` (retail has no guard — the loop would spin).

### `FUN_0043ca40` → `menu_list_nav` (970 B)

The cursor-navigation engine: maps a direction/auto-repeat code to a cursor
move, page scroll, or cancel/confirm latch.  Return code: 0 none, 1 moved,
2 page scrolled, 3 cancel, 4 confirm.

`dir` enum (matches the latch args the title menu feeds in — see
`input.md`): 0 prev, 1 next, 2 page-up, 3 page-down, 4/6 axis-held
auto-repeat (positive/negative, see quirk #33), 5/7 axis-released, 9
cancel, 10 confirm; 8 and >10 are no-ops.

The inner dispatch is the **Ghidra-unrecovered jump table** at `0x43ce1c`,
read out of the image with `radare2 -c 'pxw 44 @ 0x43ce1c'`:

| dir   | handler   | role                          |
|-------|-----------|-------------------------------|
| 0     | `0x43cb0b`| prev                          |
| 1     | `0x43cbd2`| next                          |
| 2     | `0x43cc7c`| page-up                       |
| 3     | `0x43cce2`| page-down                     |
| 4..8  | `0x43cdfe`| no-op (return 0)              |
| 9     | `0x43cae9`| cancel (action=3)             |
| 10    | `0x43cafa`| confirm (action=4)            |

The page-up/down handlers and the type-2 common tail share the `page_top()`
reflow at `LAB_0043cda7`.  `iVar7` (entry `sel2 + stride`) is captured once
at the top and read by the prev/next handlers and the type-3 tail.

### `FUN_0043ce50` → `menu_list_latch` (220 B)

The gate in front of menu input.  Returns 0 unless `sub->ready == 1000 &&
sub->enabled != 0` (quirk #34).  Then:

- **mode 1** → forward to `menu_list_nav`.  This is the path the title
  top-level menu takes: poll (`input_poll_consume`) → latch → nav.
- **mode 2** → drive the confirm/message list (`list2`) directly:
  - submode 0: flag-driven ack (pending flag `+0x18` set → clear, return 6;
    else content flag `+0x14` set → latch action=8, return 8).
  - submode 1: reveal-then-dismiss scrolling message (two-press, quirk #34).
    `cap` is `(list2->[0] → [+0x0c] → [+8])` as u16; `pos` is `list2->[0x04]`
    u16.

## The input → action chain (now complete)

```
input_poll_consume(now, btn)   read the +0x108 event ring (FUN_0043c110)
        │  hit → button id
        ▼
menu_list_latch(dir, now)       gate on "ready", dispatch by mode (FUN_0043ce50)
        │  mode 1
        ▼
menu_list_nav(dir, now)         move cursor / scroll / latch (FUN_0043ca40)
        │
        ▼
ctrl->action  +  return code    consumed by the title scene's switch(iVar14)
```

The title scene maps button ids → latch `dir` (input.md "Button ids"):
down 0x02→2, right 0x04→3, up 0x01→0, left 0x03→1, back 0x24→9, with the
axis-held synthesis feeding 4/5/6/7.

## The controller geometry — `menu_ctrl_build` / `menu_ctrl_clear`

`FUN_0040f5c0` (563 B) builds the controller's selectable grid; `FUN_0040e0c0`
(555 B) tears it down.  The ctor opens by calling the dtor (slots are
recycled, not zeroed — quirk #35), then allocates the `0x24` list header and
**two parallel arrays** plus a per-row cell array:

| ctrl off | array      | element            | count   | per-element init                          |
|----------|------------|--------------------|---------|-------------------------------------------|
| `+0x17c` | **rows**   | `menu_row` (0x10)  | `alloc_a` (hdr+4) | `field0=0`, `action=0`, `cells=`↓ (flag8 left indeterminate) |
| (per row)| **cells**  | `menu_cell` (0x18) | `alloc_b` (hdr+8) | three ptr slots NULL, `field_c=0`         |
| `+0x178` | **entries**| `menu_entry` (0x24)| `alloc_b` (hdr+8) | `pos=index*0x20`, `extent=0x20`, rest 0   |

Note `alloc_b` sizes **both** the per-column entry array and every row's cell
array, while `alloc_a` sizes only the row array.  The title menu passes
`(f_c=0, f_10=0, alloc_a=6, alloc_b=1, stride=6, type=0)` — up to 6 rows × 1
cell, one column entry, linear-wrap.

The dtor frees in retail order: confirm graph (`list2 → src → {owned0,
owned8, caprec→owned0}`), the `+0x164` buffer, `entries`, then each row's
cells (and each cell's three lazily-built sub-objects — `obj0` whose `*obj0`
is itself owned, plus the `0x54`/`0x20` objects built by `0x411f40`), the row
array, and the header **last** (its `alloc_a`/`alloc_b` size the free loops).

`menu_cell`'s three pointer slots and the row append are populated later (by
the spawn block + the still-unported finalizers below); the ctor only NULLs
them.

## Still unported (next)

- **The spawn block's populate half** (`0x56aea0` default branch, after
  `menu_ctrl_build`): append 5 rows with action IDs `0x1a,0x1c,0x1e,0x1d,8`
  (each writes `row.field0=0`, `row.action=id`, `row.flag8=1`, bumps
  `hdr.count`, then calls `0x411f40`), then seek the row whose `field0==0`
  matches a god-object key, set the cursor, and `menu_list_scroll_into_view`.
  The appends are cheap inline stores; the work is in the two finalizers.
- **`0x40f3e0`** (434 B) — the menu-item builder: copies a 9-dword config
  blob, frees old items (via `0x40e0c0` + free), allocs N×`0x1b0`-byte items
  with ~20 magic fields (colors `0xf08080`, `&DAT_00677b98`).  Needs the
  `0x1b0` item struct modelled.  (Called on the *page-container* object, not
  the menu controller.)
- **`0x411f40`** (444 B) — the grid-cell finalizer: per cell, lazily
  `operator_new`s the `0x54` (`cell.obj54`) and `0x20` (`cell.obj20`)
  sub-objects and calls `0x40fa00`.  Needs those two sub-object structs.
- **The input-ring producer** (DInput `GetDeviceState`) — black box,
  milestone-1 mem-watch gate.

## Files referenced

- `docs/decompiled/by-address/4192b0.c`, `43ca40.c`, `43ce50.c`, `40f5c0.c`,
  `40e0c0.c` (and `40f3e0.c`, `411f40.c` for the unported finalizers).
- `src/menu_list.{c,h}`, `tests/test_menu_list.c`.
- jump table at `0x43ce1c`; recovered via radare2.
