# Session handoff — last updated 2026-05-29 (menu-controller geometry, ckpt 5)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 5 just landed** (this session): the menu-spawn block's
**allocate/free pair**, ported into `src/menu_list.{c,h}` as one commit
(build calls clear, so they're inseparable):

- **`menu_ctrl_build`** (`FUN_0040f5c0`, 563 B) — the controller geometry
  constructor. Allocates the `0x24` list header + **two parallel arrays**:
  the row array (`alloc_a`×`menu_row` 0x10 at `+0x17c`) with a per-row cell
  array (`alloc_b`×`menu_cell` 0x18), and the per-column metadata array
  (`alloc_b`×`menu_entry` 0x24 at `+0x178`, stamped `pos=index*0x20`,
  `extent=0x20`). Seeds controller scalars (`mode=1`, `field_c/_10` etc.).
- **`menu_ctrl_clear`** (`FUN_0040e0c0`, 555 B) — the matching teardown.
  Frees in retail order: confirm graph (`list2→src→{owned0,owned8,
  caprec→owned0}`) → `+0x164` → entries → each row's cells (+ their three
  lazy sub-objects) → row array → **header last** (its `alloc_a`/`alloc_b`
  size the free loops). No-ops on a fresh controller.

This **grounds the controller's full geometry layout** (quirk #35: slots
are recycled from a pool, so the ctor self-clears first). Modelled
`menu_row`/`menu_cell`/`menu_entry` + the ctor scalars; extended
`confirm_src`/`confirm_caprec` with the owned-ptr slots the teardown frees.
See `findings/menu-list.md` "The controller geometry".

**493 host tests pass, 0 fail, 6 skip (of 499)** — 9 new (build
header/params/grid/entries, fresh-clear no-op, recycle-rebuild, confirm-
graph + cell-subobject + `+0x164` teardowns; ASan/LSan verify no leak/
double-free/UAF). Both cross-build exes clean (32-bit `_Static_assert` on
every new struct offset holds). New quirk **#35** (ctor self-clears a
recycled slot; `alloc_b` sizes both cells and entries). Ledger **121/1490
touched (7.4%), 118 tested**.

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). 121/1490 touched, 9.7%
  of bytes, 118 host-tested.
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness.
- `docs/findings/title-scene.md` — the title runner's full anatomy.
- `docs/findings/menu-list.md` — the menu controller: scroll/nav/latch +
  **NEW** the geometry ctor/dtor and what remains of the spawn block.
- `docs/findings/input.md` — the input ring + poll; only the ring
  *producer* remains black-box.
- `docs/port-frontier.md` — DERIVED "what to port next".
- `findings/engine-quirks.md` #15–#35.

**Tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

**Structural-parity harness (offline foundation landed 2026-05-29):**
call-graph diff + mem-watch, mirroring `../openrecet`. How-to:
`docs/parity-harness.md`; design: `docs/plans/parity-harness.md`. Offline
pieces done + tested. The agent + `frida_capture.py` call-trace/mem-watch
modes + `tools/bisect_call_trace_vas.py` are code-complete but **need a
live retail-under-Frida run to verify** (human-verification gate). The
`mem_watch.py --region <+0x108 addr>:64:input_ring` run now catches the
input-ring **producer** — the last black box in the input subsystem.

NB: only put a `FUN_<va>` token in `src/` for a function you have
actually ported — the ledger generator treats any `FUN_<va>` in src as a
port signal. Reference *unported* callees by bare VA (`0x40f3e0`), not
`FUN_0040f3e0`, or you'll inflate the headline.

## Module inventory (11 modules)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper,
cs_dispatch, app_pump, title_scene (`FUN_0056aea0` partial — fade FSM +
pacing FSM), input (`FUN_0043c110`), obj_container (`FUN_00412c10` +
`FUN_00414080`), **menu_list (`FUN_004192b0` + `FUN_0043ca40` +
`FUN_0043ce50` + `FUN_0040f5c0` + `FUN_0040e0c0`)**. Live boot zero DDERR
through 10 frames in mode 2. The drop-in still uses its own minimal
`main_loop_body`; the ported title FSMs + the menu chain are **not yet
wired** into a real scene loop in main.c.

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
Done: both pure FSMs (fade + pacing), the update half's input poll, the
container leaves, the whole menu input→action chain, and now the menu
controller's geometry alloc/free. Remaining: **the two lazy cell
finalizers + the spawn-block populate** (rest of the update half) and the
**render half** (the path that draws + Flips).

## Next move (pick one — recommendation first)

1. **(recommended) Port the two cell finalizers, then assemble the spawn
   block.** With the controller geometry now modelled (`menu_row`/
   `menu_cell`/`menu_entry` in `menu_list.h`), the rest of the update half
   is within reach:
   - **`0x411f40`** (444 B) — the grid-cell finalizer. Per cell, lazily
     `operator_new`s the `0x54` object (→ `cell.obj54`) and the `0x20`
     object (→ `cell.obj20`), zero-inits each, then calls `0x40fa00`; also
     clamps a field on the `0x20` object (`[+0x1c]=max([+0x14],[+0x18])`).
     **Start here** — its cell-struct touches are already modelled; you
     just need the `0x54` / `0x20` sub-object layouts + `0x40fa00`.
   - **`0x40f3e0`** (434 B) — the menu-item builder. NB it operates on the
     **page-container** object (`*in_ECX` in `0x56aea0`, the god-object's
     list), **not** the menu controller: copies a 9-dword config blob into
     `[0x17..0x1f]`, frees old items (via `0x40e0c0` + free), then allocs
     N×`0x1b0`-byte items with ~20 magic fields (colors `0xf08080`, ptrs
     `&DAT_00677b98`/`&DAT_008090a9`). Needs the `0x1b0` item struct.
   - Then **assemble the spawn block** (`0x56aea0` default branch, lines
     ~385–465): the `param_1` skip-intro early-out, the page-container
     populate (`0x40f3e0` + `FUN_00414080`), `obj_pool_acquire` → 
     `menu_ctrl_build(0,0,6,1,6,0)`, the 5 inline row appends
     (`field0=0`, `action=0x1a/0x1c/0x1e/0x1d/8`, `flag8=1`, bump
     `count`, `0x411f40`), then the cursor-seek + `menu_list_scroll_into_view`.
     Finishes the update half.

2. **Checkpoint: the render half (`0x56bb04`)** — the path that draws.
   `PTR_DAT_0056bfa4[local_64]` jump-table call (11 entries, 7 handlers,
   already recovered in title-scene.md) → per-phase draw bridges →
   `FUN_0056c180(...->[0x16c])` + "Title Menu - Flipping" log +
   `FUN_005b8fc0(hWnd)` (the DDraw Flip). Heavily DDraw/object-model-
   coupled, so harder to unit-test than the update half.

3. **Live harness gate** — run `bisect_call_trace_vas.py` /
   `mem_watch.py --region <+0x108>:64:input_ring` under Frida to verify the
   call-trace + mem-watch machinery and catch the input-ring producer (the
   DInput `GetDeviceState` writer, vtable `[0x24]`). Human-in-the-loop.

## Open RE threads (see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — milestone 0. Pure FSMs + the full
  update-half input chain + the controller geometry done; the two lazy cell
  finalizers + spawn-block assembly + render half remain.
- **Input** poll + latch + nav **DONE**. Remaining: the **producer** that
  fills the `+0x108` ring (DInput `GetDeviceState`) — black box;
  `mem_watch.py` is the tool. See `findings/input.md` / `menu-list.md`.
- **Audio ZDM** `FUN_005bab10`/`_5bc150` + SFX player `FUN_00411390` —
  milestone 3 (WMF/COM, hard).
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4.
- **Hash-id asset directory** `FUN_00556eb0` — recover the ID→name table.
- God-object `DAT_008a9b50` layout (engine-quirks #15) — model as we go.
- Frida turbo: add `GetTickCount` + `WaitMessage` hooks to the agent
  (quirk #29 explains why turbo currently freezes the splash).

## How to apply

When the user says "continue RE work" (or similar):

1. Read this file first, then `STATUS.md` + `ROADMAP.md`.
2. Pick the recommended next move (or whichever the user redirects to).
3. Work port-and-test style: small unit → tests → commit. Each ported
   function gets a `FUN_XXXXXX` provenance comment (the ledger keys on it)
   and a test spot-checking behaviour vs hand-computed expectations. Pin
   retail struct offsets via `_Static_assert` guarded by
   `#if UINTPTR_MAX == 0xFFFFFFFFu`. For a Ghidra-unrecovered jump table,
   recover it in r2 first (`pxw <n> @ <table-va>`), like ckpt 4b.
4. **Append any engine quirk** you find to `findings/engine-quirks.md`.
5. **Regenerate the derived artifacts** (`gen_port_ledger.py` +
   `gen_frontier.py`) when a port lands.
6. Update THIS file at each meaningful checkpoint; append to PROGRESS.md.
7. **Suggest a `/clear`** at the natural stop point — the docs are the
   durable memory, not context.
