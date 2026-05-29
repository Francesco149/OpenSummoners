# Session handoff — last updated 2026-05-29 (menu-node builder, ckpt 7)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 7 just landed** (this session): the **menu-node builder**
`FUN_0040f3e0` (434 B), ported into `src/menu_list.{c,h}` as
**`menu_node_build`**. It (re)configures one 0x1b0 menu node and
(re)builds its child-node array (freeing stale children via
`menu_ctrl_clear` first). This was the last sub-function the title-menu
spawn block needed.

**Headline finding — new quirk #37 (a Ghidra trap):** the engine's menus
are a **tree of uniform 0x1b0-byte nodes**, and Ghidra mis-typed this
builder's `__thiscall`. The decompiled call
`FUN_0040f3e0(piVar11,0,0,100,100,1,0)` reads as "operates on the
page-container `piVar11`" — but disassembly proves otherwise:

- prologue `0x40f3ec  mov ebx, ecx` → the function works on ECX;
- call site `0x56b606  mov ecx, [edi+ecx]` (= `owner->entries[count]`) →
  ECX `this` is the **node**, while `0x56b609 push esi` makes `piVar11`
  (the owning `sel_list`) just `param_1`.

So the earlier "page-container, likely obj_container territory" prediction
in HANDOFF/`findings/menu-list.md` was **off by one** and is corrected.
**Lesson:** always confirm a `__thiscall`'s ECX in the disasm (r2 `pdf`)
before trusting the decompile's arg list — same discipline that caught the
ckpt-6 dead-alloc.

Each node overlays two views on one buffer: a **container header**
(`+0x00..+0x84`; child-ptr array `+0x48`, u16 count `+0x4c`) and an
**embedded `menu_ctrl`** at `+0x00` (so `+0x164..+0x17c` are
`field_164/list2/list/entries/rows`) plus `0x30 B` of **display config**
at `+0x180..+0x1ac` (colours + label VAs). That dual identity is why the
builder frees a stale child with `menu_ctrl_clear`. Modelled `menu_node`
(0x1b0) in `menu_list.h`.

**504 host tests pass, 0 fail, 6 skip (of 510)** — 5 new (title call,
config-blob copy, per-child display config, rebuild-frees-old-children
under LSan, zero-children; ASan/LSan clean). Both cross-builds clean (new
32-bit offset asserts + a `sizeof(menu_node) >= sizeof(menu_ctrl)`
cast-safety assert all hold). Ledger **122/1490 touched (7.5%), 119
tested**.

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). 122/1490 touched, 9.7%
  of bytes, 119 host-tested.
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness.
- `docs/findings/title-scene.md` — the title runner's full anatomy.
- `docs/findings/menu-list.md` — the menu controller: scroll/nav/latch,
  geometry ctor/dtor, the grid-cell finalizer, **NEW** the menu-node
  builder + the menu-tree structure, and what remains.
- `docs/findings/input.md` — the input ring + poll; only the ring
  *producer* remains black-box.
- `docs/port-frontier.md` — DERIVED "what to port next".
- `findings/engine-quirks.md` #15–**#37**.

**Tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

**Static disasm:** the canonical unpacked image for r2 is
`vendor/unpacked/sotes.unpacked.exe` (the `vendor/original/sotes.exe`
symlink target is the **packed** Steam build — its `.text` is encrypted, so
r2 reads garbage there). Recipe:
`nix develop --command bash -c "r2 -q -e scr.color=0 -c 'af @ <va>; pdf @ <va>' vendor/unpacked/sotes.unpacked.exe"`.

**Structural-parity harness (offline foundation landed 2026-05-29):**
call-graph diff + mem-watch, mirroring `../openrecet`. How-to:
`docs/parity-harness.md`; design: `docs/plans/parity-harness.md`. Offline
pieces done + tested. The agent + `frida_capture.py` call-trace/mem-watch
modes + `tools/bisect_call_trace_vas.py` are code-complete but **need a
live retail-under-Frida run to verify** (human-verification gate).

NB: only put a `FUN_<va>` token in `src/` for a function you have
actually ported — the ledger generator treats any `FUN_<va>` in src as a
port signal. Reference *unported* callees by bare VA (`0x40fa00`,
`0x40f3e0`), not `FUN_...`, or you'll inflate the headline.

## Module inventory (11 modules)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper,
cs_dispatch, app_pump, title_scene (`FUN_0056aea0` partial — fade FSM +
pacing FSM), input (`FUN_0043c110`), obj_container (`FUN_00412c10` +
`FUN_00414080`), **menu_list (`FUN_004192b0` + `FUN_0043ca40` +
`FUN_0043ce50` + `FUN_0040f5c0` + `FUN_0040e0c0` + `FUN_00411f40` +
`FUN_0040f3e0`)**. Live boot zero DDERR through 10 frames in mode 2. The
drop-in still uses its own minimal `main_loop_body`; the ported title FSMs
+ the menu chain are **not yet wired** into a real scene loop in main.c.

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
Done: both pure FSMs (fade + pacing), the update half's input poll, the
container leaves, the whole menu input→action chain, the controller
geometry alloc/free, the grid-cell finalizer, and now the menu-node
builder. Remaining for the update half: **just the spawn-block assembly**
(cheap inline row appends). Then the **render half** (draws + Flips).

## Next move (pick one — recommendation first)

1. **(recommended) Assemble the spawn block** (`0x56aea0` default branch,
   lines ~385–465) — now that every sub-function it calls is ported. The
   sequence (disasm `0x56b5cd..0x56b6xx`):
   - `piVar11 = owner sel_list = *in_ECX`; if `count(+6) < cap(+4)`:
     `menu_node_build(owner->entries[count], owner, 0,0,100,100,1,NULL)`,
     bump `owner->count(+6)`, `sel_list_mark_last(owner)`, stash the active
     node ptr;
   - `local_60 = obj_pool_acquire()`; if non-NULL
     `menu_ctrl_build(local_60, 0,0,6,1,6,0)`;
   - 5 inline row appends (each: guard `hdr.count < hdr.alloc_a`, write
     `row.field0=0`, `row.action = 0x1a/0x1c/0x1e/0x1d/8`, `row.flag8=1`,
     bump `hdr.count`, then `menu_row_finalize(local_60, idx)` — a no-op on
     the fresh NULL cells);
   - cursor-seek: walk the rows, find the one whose `field0==0` matches the
     god-object key `*(*DAT_008a6e80 + 0xa60)`, set `hdr.cursor`, call
     `menu_list_scroll_into_view`.
   This is mostly inline stores on already-modelled structs — should be a
   clean, well-tested checkpoint that **finishes the update half**. Decide
   where it lives (a `title_scene` helper that drives the menu_list +
   obj_container + sel_list objects, since it's `0x56aea0`'s own code).

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
  update-half input chain + controller geometry + grid-cell finalizer +
  menu-node builder done; only the spawn-block assembly (inline appends) +
  the render half remain.
- **`0x40fa00`** the cell text-layout / glyph builder (800 B; SJIS parse,
  `#`-colour escapes, font-metric table; calls `0x40fd20`/`0x4051d0`/
  `0x4034f0`) — its own text subsystem; `menu_row_finalize` calls it via a
  hook until it lands. Also feeds the menu-node display config (label VAs
  `&DAT_00677b98`/`&DAT_008090a9`).
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
   recover it in r2 first (`pxw <n> @ <table-va>`). When a decompiled branch
   or a `__thiscall` arg list looks contradictory, **disassemble it** (r2
   `pdf`) to resolve — that's how ckpt 6 caught the dead-alloc and ckpt 7
   caught the mis-typed `this`.
4. **Append any engine quirk** you find to `findings/engine-quirks.md`.
5. **Regenerate the derived artifacts** (`gen_port_ledger.py` +
   `gen_frontier.py`) when a port lands.
6. Update THIS file at each meaningful checkpoint; append to PROGRESS.md.
7. **Suggest a `/clear`** at the natural stop point — the docs are the
   durable memory, not context.
