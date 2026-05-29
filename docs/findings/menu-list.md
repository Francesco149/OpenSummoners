# Menu-list controller — cursor nav, scroll, and the action latch

> Milestone 0/1.  The object that drives the title top-level menu (and most
> of the engine's selection UIs).  Three functions ported in checkpoint 4
> (`src/menu_list.{c,h}`: scroll / nav / latch); the controller's **geometry
> allocate/free pair** (`menu_ctrl_build` / `menu_ctrl_clear`) landed in
> checkpoint 5; the **grid-cell finalizer** (`menu_row_finalize`,
> `0x411f40`) landed in checkpoint 6; the **menu-node builder**
> (`menu_node_build`, `0x40f3e0`) landed in checkpoint 7.  What remains of the
> spawn block is just the per-row *populate* (cheap inline appends in
> `0x56aea0`).

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
cells (and each cell's three sub-objects — `obj0` whose `*obj0` is itself
owned, plus the `0x54`/`0x20` objects), the row array, and the header
**last** (its `alloc_a`/`alloc_b` size the free loops).

`menu_cell`'s three pointer slots and the row append are populated later (by
the spawn block + a still-unmapped item-config path); the ctor only NULLs
them.

## The grid-cell finalizer — `menu_row_finalize` (`FUN_00411f40`, 444 B)

`__thiscall(ctrl, row)`.  Walks the row's cell array (bound by the header's
`alloc_b`, the per-row cell count) and, for each cell, refreshes whichever of
its three sub-objects are already present:

- **`obj0`** (`+0x00`) present → re-lay-out its glyph text via `0x40fa00`,
  passing `&DAT_008a9b6c` (the god object's engine-name buffer, god+0x1c;
  see `audio-init.md`).  `0x40fa00` is an 800-B SJIS/colour-escape/font-
  metric text builder — its **own subsystem, not yet ported**; the port
  routes this call through an observable hook (`menu_cell_layout_hook`,
  NULL by default) so the dispatch is testable without pulling in the text
  layer.  The fresh title menu never has a built `obj0`, so this never fires.
- **`obj54`** (`+0x04`) present *and* `row < hdr->count` → re-zero its
  modelled fields (`+0, +4, +0x46, +0x48, +0x4a, +0x4c, +0x50`).
- **`obj20`** (`+0x08`) present *and* `row < hdr->count` → re-zero `+0..+0x18`
  then recompute `+0x1c = max(+0x14, min(+0x18, 0))` (reads the just-written
  zeros, so it settles at 0 here).

**Key correction (quirk #36):** despite the decompile reading as a *lazy
get-or-create*, the finalizer does **not** allocate — the inner
`if (ptr == 0) operator_new(...)` sits under an outer `ptr != 0` guard and is
statically unreachable (same slot, no intervening write; verified at
`0x411fbf` / `0x412046`).  It only re-zeroes sub-objects built elsewhere.  An
earlier draft of this file claimed it "lazily operator_new's" them — that was
wrong.  On the fresh title menu all cell pointers are NULL, so the whole
function is a no-op there.

Modelled `menu_cell_obj54` (0x54 B) / `menu_cell_obj20` (0x20 B) in
`menu_list.h`; the unported text builder is `0x40fa00` (referenced by bare
VA, not `FUN_`).

## The menu-node builder — `menu_node_build` (`FUN_0040f3e0`, 434 B)

`__thiscall(node, owner, f_c, f_10, f_14, f_18, n_children:u16, config)`.
(Re)configures one **menu tree node** and (re)builds its child array.  This
is where the engine's menu structure is revealed as a **tree of uniform
0x1b0-byte nodes** — see quirk #37.  Each node overlays two views on one
buffer: a **container header** (`+0x00..+0x84`, with a child-pointer array at
`+0x48` and its u16 count at `+0x4c`) and an **embedded `menu_ctrl`** at
`+0x00` (so `+0x164..+0x17c` are `field_164/list2/list/entries/rows`) followed
by `0x30 B` of **display config** at `+0x180..+0x1ac`.  The builder:

1. copies the 9-dword `config` blob into `+0x5c..+0x7f` (or zeroes `config[0]`
   when `config == NULL`);
2. stamps the container scalars (`owner@+0`, `+4=1`, `+8=0`, `+0xc=f_c`,
   `+0x10=f_10`, `+0x14=f_14`, `+0x18=f_18`, `+0x80=0`);
3. frees the previous child array — `menu_ctrl_clear` (`0x40e0c0`) on each
   child, then `operator delete` — guided by the *old* `+0x4c` count;
4. sets `+0x4c = n_children`, allocs the `n_children`-wide pointer array, and
   allocs each child node (`operator_new(0x1b0)`), zeroing its embedded ctrl
   (`+0x164/+0x170/+0x174/+0x178/+0x17c`) and seeding its display config
   (`+0x180=+0x19c=0x3e537d`, `+0x184=+0x1a0=0xa8b9cc`, `+0x18c=+0x190=0xf08080`,
   `+0x188=&DAT_00677b98`, `+0x194=+0x198=&DAT_008090a9`, `+0x1ac=0x1c`, and
   `+0x14=+0x18=0`);
5. sets the trailing scalars (`+0x4e=0`, `+0x54=0`, `+0x58=0`, `+0x1c=1`,
   `+0x50=1`).

The title menu calls it `menu_node_build(node, owner, 0,0,100,100,1,NULL)` —
a single child, default config.  The `owner` is a `sel_list` (obj_container.h)
whose entries are these nodes (so `node+0x08` is the `sel_entry` "selected"
flag that `sel_list_mark_last`, `0x414080`, toggles right after).

**Key correction (quirk #37):** Ghidra mis-typed the `__thiscall`, rendering
the ECX `this` *node* as the call's first arg and dropping it; the decompiled
`FUN_0040f3e0(piVar11,0,0,100,100,1,0)` therefore read as "operates on the
page-container `piVar11`".  The disasm (`0x40f3ec mov ebx,ecx`; call site
`0x56b606 mov ecx,[owner->entries + count*4]`) shows `this` is the node and
`piVar11` is `param_1` (the owner).  Earlier drafts of this file and the
HANDOFF called it a "page-container" function — that was off by one and is now
corrected.

Modelled `menu_node` (0x1b0 B) in `menu_list.h`.  Divergences (per
`menu_ctrl_build`'s convention): `operator_new → calloc` (child header bytes
read as zero vs retail garbage); the per-child `menu_ctrl_clear` is
layout-exact on the 32-bit target, and the 64-bit host only ever clears
zeroed children (an all-NULL no-op), matching the title flow which never
rebuilds a populated node.

## The spawn block — `title_menu_spawn` (checkpoint 8)

The `0x56aea0` default-branch spawn block (`0x56b5cd..0x56b807`) — the last
piece of the title scene's *update* half — is now ported in
`src/title_scene.c` as `title_menu_spawn` (+ `title_menu_teardown` for the
phase-10 path).  It composes the menu primitives above into the one-shot
top-level-menu construction; see `findings/title-scene.md` and **quirk #38**
(the node's four overlaid identities and the 32-bit-only `obj_pool`/`sel_entry`
aliases).  In short: configure the owner `sel_list`'s next entry as the menu's
tree node with one child (`menu_node_build`), bump + mark it
(`sel_list_mark_last`), acquire that lone child as the controller (retail:
`obj_pool_acquire(node)` — the node *is* the pool), build its 6×1 stride-6
grid (`menu_ctrl_build`), append the five fixed rows `0x1a,0x1c,0x1e,0x1d,8`
(each `field0=0`/`action=id`/`flag8=1`, bump count, `menu_row_finalize` — a
no-op on the fresh NULL cells), then seek the cursor to the row matching the
saved selection key and `menu_list_scroll_into_view`.

## Still unported (next)

- **`0x40fa00`** (800 B) — the cell text-layout / glyph builder (SJIS parse,
  `#`-colour escapes, font-metric table; calls `0x40fd20`/`0x4051d0`/
  `0x4034f0`).  Its own text subsystem; `menu_row_finalize` calls it via a
  hook until it lands.
- **The input-ring producer** (DInput `GetDeviceState`) — black box,
  milestone-1 mem-watch gate.

## Files referenced

- `docs/decompiled/by-address/4192b0.c`, `43ca40.c`, `43ce50.c`, `40f5c0.c`,
  `40e0c0.c`, `411f40.c` (and `40f3e0.c`, `40fa00.c` for what remains).
- `src/menu_list.{c,h}`, `tests/test_menu_list.c`.
- jump table at `0x43ce1c`; recovered via radare2.
