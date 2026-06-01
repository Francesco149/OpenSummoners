# Session handoff — last updated 2026-06-01 (title-menu input dispatch, ckpt 9)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 9 just landed** (this session): the **per-frame menu input
dispatch** (`0x56aea0` default branch, `0x56b807..0x56ba39`), assembled into
`src/title_scene.c` as **`title_menu_input_step`**. This **completes the
update half** of the title scene — only the render half remains. Like ckpt 8
it's an *assembly* of already-ported leaves (`input_poll_consume`,
`menu_list_latch`), so the ledger is unchanged — the value is the wiring +
two disasm-resolved findings. The step: poll the five buttons (+ two
axis-held syntheses) into the latch, run the action switch (move/confirm/
denied/cancel SFX), the enabled-row commit (joystick attach → save-data table
walk + notify → phase-10 + result latch), and the idle watchdog. The four
unported side effects (SFX `0x411390`, joystick `0x5ba120/_290`, notify
`0x41bb80`, watchdog `0x40a5d0`) route through **no-op-by-default hooks**
(the `menu_cell_layout_hook` pattern). The save-data lookup itself is ported
faithfully against a caller-supplied model of the god object's table slice.

**Headline finding — new quirk #39:** the action `switch` keys on the
**latch return code**, not the button, and the engine's cancel-returns-3 /
confirm-returns-4 convention inverts the intuitive reading. The physical
**commit button is `0x24`** (→ latch dir 9 → nav returns 3 → `case 3`);
`case 3` gates on the selected **row**'s `flag8` (enabled), **not** on
`action == 0x1d` (that's a separate, later save-data guard); `case 4`
(cancel SFX 7) is **dead** in the title flow. Also: the page dirs (2/3) are
no-ops on the single-page menu (`stride 6 ≥ count 5`), so only up/left
navigate. The decompile hid the `__thiscall` ECXs and these facts — all were
resolved against the raw r2 disasm (`pdf`). The `+0x114/+0x118` axis-held
flags were added to `input_mgr` (with 32-bit offset asserts); they live in
the input manager past the poll ring.

**518 host tests pass, 0 fail, 6 skip (of 524)** — 9 new (nav move SFX,
page-button no-op, commit enabled/disabled, save-data match + 0x1d skip, idle
frame, axis-held synth, watchdog idle threshold; ASan/LSan clean). Both
32-bit cross-builds clean (the new `input_mgr` axis-offset asserts hold).
Ledger **122/1490 touched (7.5%), 119 tested** (unchanged — assembly, not a
new port).

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). 122/1490 touched, 9.7%
  of bytes, 119 host-tested.
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness.
- `docs/findings/title-scene.md` — the title runner's full anatomy; the
  "Input dispatch" section now marks steps 1–3 ported (ckpt 8 + 9).
- `docs/findings/menu-list.md` — the menu controller: scroll/nav/latch,
  geometry ctor/dtor, the grid-cell finalizer, the menu-node builder + the
  menu-tree structure, and what remains.
- `docs/findings/input.md` — the input ring + poll; only the ring
  *producer* remains black-box (it also fills `+0x114/+0x118`).
- `docs/port-frontier.md` — DERIVED "what to port next".
- `findings/engine-quirks.md` #15–**#39**.

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
port signal. Reference *unported* callees by bare VA (`0x411390`,
`0x5ba120`), not `FUN_...`, or you'll inflate the headline. **(This bit
ckpt 9 — four unported callees written as `FUN_` bumped the headline
122→126 until corrected; always regen the ledger and check the count after
a port.)**

## Module inventory (11 modules)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper,
cs_dispatch, app_pump, title_scene (`FUN_0056aea0` partial — fade FSM +
pacing FSM + the **whole update half**), input (`FUN_0043c110`),
obj_container (`FUN_00412c10` + `FUN_00414080`), **menu_list
(`FUN_004192b0` + `FUN_0043ca40` + `FUN_0043ce50` + `FUN_0040f5c0` +
`FUN_0040e0c0` + `FUN_00411f40` + `FUN_0040f3e0`)** + the title-menu
**spawn block** (`title_menu_spawn` / `title_menu_teardown`) + the
**per-frame input dispatch** (`title_menu_input_step`) in `title_scene.c`.
Live boot zero DDERR through 10 frames in mode 2. The ported title FSMs +
menu chain are **not yet wired** into a real scene loop in main.c (the
drop-in still uses its own minimal `main_loop_body`).

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
The whole **update half is now done**: both pure FSMs (fade + pacing), the
input poll, the container leaves, the full menu input→action chain, the
controller geometry, the grid-cell finalizer, the menu-node builder, the
one-shot menu spawn block, and now the per-frame input dispatch. **Only the
render half remains.**

## Next move (pick one — recommendation first)

1. **(recommended) Checkpoint: the render half (`0x56bb04`).** This is the
   path the pacer's `sub==1` branch dispatches to, and it's the last thing
   between us and a drawn title frame. `PTR_DAT_0056bfa4[local_64]`
   jump-table call (11 entries, 7 handlers, already recovered in
   title-scene.md) → per-phase draw bridges → `FUN_0056c180(...->[0x16c])` +
   "Title Menu - Flipping" log + `FUN_005b8fc0(hWnd)` (the DDraw Flip). It is
   heavily DDraw/object-model-coupled, so harder to unit-test than the update
   half — expect to lean on hooks for the draw bridges and model the
   jump-table dispatch as the testable core (recover the table in r2 first,
   `pxw 44 @ 0x56bfa4`). Start by scouting which of the 7 handlers the title
   phases (0..10) actually reach and what each draws.

2. **Wire the update half into a real scene loop** (`main.c`) — compose
   `title_pace_step` → (`0x22` abort poll → return 6) → `title_fade_step` →
   `title_menu_spawn` (first menu frame) → `title_menu_input_step` into an
   actual `FUN_0056aea0`-shaped runner, install the hooks against the real
   SFX/joystick/god-object, and drive it from the drop-in. This turns the
   pile of tested units into a running title scene (still no draws until the
   render half lands, but the FSM + input become observable live).

3. **Live harness gate** — run `bisect_call_trace_vas.py` /
   `mem_watch.py --region <+0x108>:64:input_ring` under Frida to verify the
   call-trace + mem-watch machinery and catch the input-ring producer (the
   DInput `GetDeviceState` writer, vtable `[0x24]`) — which also fills the
   new `+0x114/+0x118` axis-held flags. Human-in-the-loop.

## Open RE threads (see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — milestone 0. **Update half DONE.**
  Remaining: the **render half** (`0x56bb04`) + wiring it all into a real
  scene loop.
- **`0x40fa00`** the cell text-layout / glyph builder (800 B; SJIS parse,
  `#`-colour escapes, font-metric table; calls `0x40fd20`/`0x4051d0`/
  `0x4034f0`) — its own text subsystem; `menu_row_finalize` calls it via a
  hook until it lands.
- **SFX `0x411390`** + **joystick `0x5ba120/_290`** + **save-data notify
  `0x41bb80`** + **watchdog `0x40a5d0`** — the four side effects
  `title_menu_input_step` now stubs through hooks; port when their
  subsystems come up (audio = milestone 3; DInput pad attach; god-object
  scene dispatch).
- **Input** poll + latch + nav **DONE**. Remaining: the **producer** that
  fills the `+0x108` ring (DInput `GetDeviceState`) and the `+0x114/+0x118`
  axis-held flags — black box; `mem_watch.py` is the tool.
- **Audio ZDM** `FUN_005bab10`/`_5bc150` + SFX player `FUN_00411390` —
  milestone 3 (WMF/COM, hard).
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4.
- **Hash-id asset directory** `FUN_00556eb0` — recover the ID→name table.
- God-object `DAT_008a9b50`/`DAT_008a6e80` layout (engine-quirks #15) —
  model as we go (the save-data table slice at `god->[0xa48]+0x10` is the
  newest piece touched).
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
   `pdf`) to resolve — that's how ckpt 6 caught the dead-alloc, ckpt 7
   caught the mis-typed `this`, and ckpt 9 caught the commit-button /
   latch-return-code inversion (quirk #39).
4. **Append any engine quirk** you find to `findings/engine-quirks.md`.
5. **Regenerate the derived artifacts** (`gen_port_ledger.py` +
   `gen_frontier.py`) when a port lands — and **check the headline count
   didn't move unexpectedly** (a stray `FUN_` for an unported callee inflates
   it; reference unported callees by bare VA).
6. Update THIS file at each meaningful checkpoint; append to PROGRESS.md.
7. **Suggest a `/clear`** at the natural stop point — the docs are the
   durable memory, not context.
