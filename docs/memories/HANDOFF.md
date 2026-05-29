# Session handoff — last updated 2026-05-29 (title-menu spawn block, ckpt 8)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 8 just landed** (this session): the **title-menu spawn block**
(`0x56aea0` default branch, `0x56b5cd..0x56b807`), assembled into
`src/title_scene.{c,h}` as **`title_menu_spawn`** (+ `title_menu_teardown`
for the phase-10 path). It composes already-ported leaves — no new function
is ported, so the ledger is unchanged; the value is the assembly + the
structural finding. The block: configure the owner `sel_list`'s next entry
as the menu's tree node with one child (`menu_node_build`), bump +
`sel_list_mark_last`, acquire that lone child as the controller,
`menu_ctrl_build` its 6×1 stride-6 grid, append the five fixed rows
`0x1a,0x1c,0x1e,0x1d,8` (each `menu_row_finalize`d — a no-op on fresh NULL
cells), then seek the cursor to the row matching the saved selection key and
`menu_list_scroll_into_view`. Returns `{node, ctrl}` (retail `local_54` /
`local_60`) for the per-frame dispatch to drive.

**Headline finding — new quirk #38:** the 0x1b0 menu node wears **four**
overlaid identities — container header, embedded `menu_ctrl`, `sel_entry`,
*and* `obj_pool`. The controller (`local_60`) is the node's lone **child**,
handed out by reinterpreting the node as a pool: retail does
`obj_pool_acquire(node)` (ECX = node, confirmed at `0x56b623 call 0x412c10`
with the node still in ECX). The acquire stamps the child's `+0x00` — the
controller's `menu_ctrl.sub` — with the node pointer, **wiring the
controller's input-ready gate to the node** (the latch reads `node+0x54`
ready / `node+0x04` enabled). Caveat that bit the port: the node's
child-array/count/capacity (`+0x48`/`+0x4e`/`+0x4c`) alias `obj_pool`, and
`node+0x08` aliases `sel_entry.selected` — but **only on the 32-bit target**
(the node's 8-byte `owner` shifts those fields on the 64-bit host). So the
port applies `obj_pool_acquire`'s semantics to the node's own `menu_node`
fields (identical on win32), and the test checks selection through the
`sel_entry` view. **Lesson:** a retail reinterpret-cast between two
primitives ports to the host only if both structs are pointer-free up to the
aliased fields — else replicate the semantics on the real struct.

**509 host tests pass, 0 fail, 6 skip (of 515)** — 5 new (five-row build,
cursor-seek-to-match, no-match keeps cursor 0, teardown clears the node's
`+0x50`, teardown-noop-when-unset; ASan/LSan clean). Both 32-bit cross-builds
clean (`menu_node`/`sel_list`/`pool_slot` offset asserts hold). Ledger
**122/1490 touched (7.5%), 119 tested** (unchanged — assembly, not a new
port).

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
`FUN_0040f3e0`)** + the title-menu **spawn block** (`title_menu_spawn` /
`title_menu_teardown` in `title_scene.c`). Live boot zero DDERR through 10
frames in mode 2. The drop-in still uses its own minimal `main_loop_body`;
the ported title FSMs + the menu chain are **not yet wired** into a real
scene loop in main.c.

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
Done: both pure FSMs (fade + pacing), the update half's input poll, the
container leaves, the whole menu input→action chain (poll/latch/nav as
units), the controller geometry alloc/free, the grid-cell finalizer, the
menu-node builder, and now **the one-shot menu spawn block**. Remaining for
the update half: the **per-frame menu input dispatch** (poll→latch→action
switch→joystick attach, `0x56b8xx..0x56ba0e`) — its leaves
`input_poll_consume`/`menu_list_latch` are ported but it also calls the
**unported** SFX `FUN_00411390` and joystick `FUN_005ba120/_290`, so it needs
hooks. Then the **render half** (draws + Flips).

## Next move (pick one — recommendation first)

1. **(recommended) Assemble the per-frame menu input dispatch** (`0x56aea0`
   default branch after the spawn, decompile lines ~467–573;
   `title-scene.md` "Input dispatch" steps 2–3). Each menu frame:
   - poll the four nav buttons via `input_poll_consume` (down `2`→latch 2,
     right `4`→3, up `1`→0, left `3`→1), with the axis-held synthesis
     (`4|5`/`6|7` from the `ctrl[1]+0x114/+0x118` flags), back `0x24`→9, and
     the early `0x22`→return state 6;
   - `switch(action)`: 1/2 → SFX `FUN_00411390(9,…)`; 3 confirm → if the
     selected row's action is `0x1d` push `(6,…)` else `(5,…)`, and run the
     **joystick lazy-attach** (`FUN_005ba120`/`_290` over `&DAT_008a93dc`);
     4 cancel → `(7,…)`; then on confirm latch `local_48 = selected action`,
     `local_64 = 10`.
   `FUN_00411390` (SFX) and `FUN_005ba120/_290` (DInput pad attach) are
   **unported** — route them through observable hooks (the
   `menu_cell_layout_hook` pattern in `menu_list.c`) so this assembles +
   tests now without pulling in audio/DInput. Drives the `title_menu`
   {node,ctrl} that `title_menu_spawn` returns. **Finishes the update half.**

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
  menu-node builder + the one-shot menu spawn block done; the **per-frame
  menu input dispatch** (needs SFX/joystick hooks) + the render half remain.
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
