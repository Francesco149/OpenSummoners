# Session handoff — last updated 2026-05-29 (grid-cell finalizer, ckpt 6)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 6 just landed** (this session): the menu spawn block's
**grid-cell finalizer**, ported into `src/menu_list.{c,h}` as
**`menu_row_finalize`** (`FUN_00411f40`, 444 B). `__thiscall(ctrl, row)`:
walks the row's cell array (bounded by `hdr->alloc_b`) and, per cell,
refreshes whichever sub-objects are present:

- **`obj0`** → re-lays-out its glyph text via `0x40fa00` (the 800-B
  SJIS/colour-escape/font-metric text builder — its **own subsystem, not
  yet ported**; the call routes through an observable hook
  `menu_cell_layout_hook` so the dispatch is testable without it).
- **`obj54`** / **`obj20`** (when `row < hdr->count`) → re-zero their
  modelled fields; `obj20` also recomputes `+0x1c = max(+0x14,
  min(+0x18, 0))`.

**Key correction — new quirk #36:** the decompile reads as a lazy
get-or-create, but the per-sub-object `if (ptr==0) operator_new(...)`
sits under an outer `ptr!=0` guard and is **statically unreachable**
(verified in the disasm at `0x411fbf` / `0x412046`). So the finalizer
**never allocates** — it only re-zeroes sub-objects built elsewhere. The
earlier `findings/menu-list.md` claim that it "lazily operator_new's" the
sub-objects was **wrong** and is now corrected. On the fresh title menu
all cell pointers are NULL → the whole function is a no-op there.

Modelled `menu_cell_obj54` (0x54 B) / `menu_cell_obj20` (0x20 B) in
`menu_list.h`. **499 host tests pass, 0 fail, 6 skip (of 505)** — 6 new
(fresh no-op, obj54 re-zero, obj20 re-zero+clamp, row-outruns-count guard,
obj0 layout-hook dispatch, all-cells iteration; ASan/LSan clean). Both
cross-build exes clean (32-bit `_Static_assert`s on the two new structs
hold). Ledger **121/1490 touched (7.4%), 118 tested** — unchanged, because
`0x411f40` had been provisionally counted via a ckpt-5 header comment using
the `FUN_` token; this port makes that count legitimate.

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). 121/1490 touched, 9.7%
  of bytes, 118 host-tested.
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness.
- `docs/findings/title-scene.md` — the title runner's full anatomy.
- `docs/findings/menu-list.md` — the menu controller: scroll/nav/latch,
  geometry ctor/dtor, **NEW** the grid-cell finalizer, and what remains.
- `docs/findings/input.md` — the input ring + poll; only the ring
  *producer* remains black-box.
- `docs/port-frontier.md` — DERIVED "what to port next".
- `findings/engine-quirks.md` #15–**#36**.

**Tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

**Structural-parity harness (offline foundation landed 2026-05-29):**
call-graph diff + mem-watch, mirroring `../openrecet`. How-to:
`docs/parity-harness.md`; design: `docs/plans/parity-harness.md`. Offline
pieces done + tested. The agent + `frida_capture.py` call-trace/mem-watch
modes + `tools/bisect_call_trace_vas.py` are code-complete but **need a
live retail-under-Frida run to verify** (human-verification gate).

NB: only put a `FUN_<va>` token in `src/` for a function you have
actually ported — the ledger generator treats any `FUN_<va>` in src as a
port signal. Reference *unported* callees by bare VA (`0x40fa00`,
`0x40f3e0`), not `FUN_...`, or you'll inflate the headline. (This bit us
on `0x411f40` at ckpt 5 — now moot since it's ported.)

## Module inventory (11 modules)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper,
cs_dispatch, app_pump, title_scene (`FUN_0056aea0` partial — fade FSM +
pacing FSM), input (`FUN_0043c110`), obj_container (`FUN_00412c10` +
`FUN_00414080`), **menu_list (`FUN_004192b0` + `FUN_0043ca40` +
`FUN_0043ce50` + `FUN_0040f5c0` + `FUN_0040e0c0` + `FUN_00411f40`)**.
Live boot zero DDERR through 10 frames in mode 2. The drop-in still uses
its own minimal `main_loop_body`; the ported title FSMs + the menu chain
are **not yet wired** into a real scene loop in main.c.

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
Done: both pure FSMs (fade + pacing), the update half's input poll, the
container leaves, the whole menu input→action chain, the controller
geometry alloc/free, and now the grid-cell finalizer. Remaining for the
update half: **the menu-item builder (`0x40f3e0`) + the spawn-block
assembly**. Then the **render half** (the path that draws + Flips).

## Next move (pick one — recommendation first)

1. **(recommended) Port the menu-item builder `0x40f3e0` (434 B), then
   assemble the spawn block.** This is the last sub-function the spawn
   block needs. NB it operates on the **page-container** object (`*in_ECX`
   in `0x56aea0`, the god-object's list), **not** the menu controller — so
   it likely belongs in `obj_container`, not `menu_list`; decide placement
   first. It copies a 9-dword config blob into `+0x5c..+0x7c`, seeds scalars,
   frees the old item array (`+0x48`, count u16 `+0x4c`), then allocs N ×
   `0x1b0`-byte items. **Already verified (disasm 0x40f45b):** each `0x1b0`
   item **embeds a full `menu_ctrl` (0x180 B)** — the free loop calls
   `menu_ctrl_clear` on each item — **followed by 0x30 B of display config**
   (colours `0x3e537d`/`0xa8b9cc`/`0xf08080`, label ptrs `&DAT_00677b98`/
   `&DAT_008090a9`, `+0x1ac=0x1c`). See `findings/menu-list.md` "Still
   unported" for the full field map. Needs the page-container struct + the
   `0x1b0` item modelled before porting. Then **assemble the spawn block**
   (`0x56aea0` default branch, lines ~385–465): the `param_1` skip-intro
   early-out, the page-container populate (`0x40f3e0` + `FUN_00414080`),
   `obj_pool_acquire` → `menu_ctrl_build(0,0,6,1,6,0)`, the 5 inline row
   appends (`field0=0`, `action=0x1a/0x1c/0x1e/0x1d/8`, `flag8=1`, bump
   `count`, then `menu_row_finalize` — a no-op on the fresh NULL cells),
   then the cursor-seek + `menu_list_scroll_into_view`. Finishes the
   update half.

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
  update-half input chain + the controller geometry + the grid-cell
  finalizer done; the menu-item builder (`0x40f3e0`) + spawn-block assembly
  + render half remain.
- **`0x40fa00`** the cell text-layout / glyph builder (800 B; SJIS parse,
  `#`-colour escapes, font-metric table; calls `0x40fd20`/`0x4051d0`/
  `0x4034f0`) — its own text subsystem; `menu_row_finalize` calls it via a
  hook until it lands. Only fires for cells with a built `obj0` (not the
  fresh title menu).
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
   recover it in r2 first (`pxw <n> @ <table-va>`), like ckpt 4b. When a
   decompiled branch looks contradictory, **disassemble it** (r2 `pdf`) to
   resolve — that's how ckpt 6 caught the dead-alloc quirk #36.
4. **Append any engine quirk** you find to `findings/engine-quirks.md`.
5. **Regenerate the derived artifacts** (`gen_port_ledger.py` +
   `gen_frontier.py`) when a port lands.
6. Update THIS file at each meaningful checkpoint; append to PROGRESS.md.
7. **Suggest a `/clear`** at the natural stop point — the docs are the
   durable memory, not context.
