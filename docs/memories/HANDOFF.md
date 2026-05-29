# Session handoff — last updated 2026-05-29 (menu input chain, ckpt 4)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 4 just landed** (this session): the entire **menu
input → action chain** the title menu's update half depends on, ported
port-and-test style into a new **`src/menu_list.{c,h}`** (3 logical
commits, 4a/4b/4c):

- **`menu_list_scroll_into_view`** (`FUN_004192b0`, 52 B) — recompute the
  page-top `sel2 = floor(cursor/stride)*stride`, return 1 if it moved.
  Factored its step-search into `page_top()` (reused by the nav engine).
- **`menu_list_nav`** (`FUN_0043ca40`, 970 B) — the cursor-navigation
  engine. Its inner jump table was **Ghidra-unrecovered**; recovered via
  `radare2 -c 'pxw 44 @ 0x43ce1c'` (dir 0..10 → 7 handlers). Ported
  branch-for-branch incl. the three list-type scroll models (0 linear-wrap
  / 2 grid / 3 trailing-page) and the two-rate auto-repeat (300→100 ms).
- **`menu_list_latch`** (`FUN_0043ce50`, 220 B) — the input gate: refuses
  unless `sub->ready==1000 && sub->enabled!=0`, then dispatches mode 1 →
  nav, mode 2 → confirm/message box (two-press reveal-then-dismiss).

This **completes the chain** `input_poll_consume → menu_list_latch →
menu_list_nav` (ckpt 3 did the poll). See `findings/menu-list.md` for the
full picture and the field/handler tables.

**484 host tests pass, 0 fail, 6 skip (of 490)** — 43 new
(6 scroll + 26 nav + 11 latch), every handler/branch/list-type with
hand-derived expectations. Both cross-build exes clean (32-bit
`_Static_assert` on every struct offset holds). New quirks **#32**
(jump-table + per-type field reuse), **#33** (two-rate auto-repeat),
**#34** (1000-ready gate + two-press confirm). Ledger **118/1490 touched
(7.2%), 115 tested**.

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). 118/1490 touched, 9.6%
  of bytes, 115 host-tested.
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness.
- `docs/findings/title-scene.md` — the title runner's full anatomy.
- `docs/findings/menu-list.md` — **NEW**: the menu controller, the three
  ported functions, the recovered jump table, the input→action chain.
- `docs/findings/input.md` — the input ring + poll; the latch/nav "Open"
  items are now resolved (only the ring *producer* remains black-box).
- `docs/port-frontier.md` — DERIVED "what to port next".
- `findings/engine-quirks.md` #15–#34.

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
`FUN_0043ce50`)**. Live boot zero DDERR through 10 frames in mode 2. The
drop-in still uses its own minimal `main_loop_body`; the ported title FSMs
+ the menu chain are **not yet wired** into a real scene loop in main.c.

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
Done: both pure FSMs (fade + pacing), the update half's input poll, the
container leaves, and now the whole menu input→action chain. Remaining:
**assemble the menu-spawn block** (the rest of the update half) and the
**render half** (the path that draws + Flips).

## Next move (pick one — recommendation first)

1. **(recommended) Port the menu-spawn-block leaves, then assemble it.**
   The `0x56aea0` default branch (lines ~385–465) builds the controller on
   first entry and populates 5 menu rows with action IDs `0x1a,0x1c,0x1e,
   0x1d,8` into `local_60->[0x174]/[0x17c]`. It needs three still-unported
   helpers — port-and-test each as its own unit first:
   - **`0x40f3e0`** (434 B, list append) → likely `src/obj_container.c`.
   - **`0x411f40`** (slot finalize).
   - **`0x40f5c0`** (563 B).
   Then assemble the spawn block on top of those + the now-ported
   `obj_pool_acquire` / `sel_list_mark_last` / `menu_list_*`. Models the
   `param_1` skip-intro early-out too. This finishes the update half.

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
  update-half input chain done; menu-spawn assembly + render half remain.
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
