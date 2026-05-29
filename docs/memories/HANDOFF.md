# Session handoff — last updated 2026-05-29 (update-half leaves, ckpt 3)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 3 just landed** (this session): the pure, zero-dependency
**leaves the title-menu update half depends on**, ported port-and-test
style to shrink the surface before assembling the menu:

- **`src/input.{c,h}` — `input_poll_consume`** (port of `FUN_0043c110`,
  84 B). The read side of the input-manager's 64-entry event ring at
  `+0x108`: scan newest-first, match `id + flag==1 + age≤100 ms`
  (unsigned → rollover-safe), and on a hit **zero the record id**
  (consume-on-read). Opens milestone 1. See `findings/input.md`.
- **`src/obj_container.{c,h}`** — two generic container leaves used ~10×
  each: **`obj_pool_acquire`** (`FUN_00412c10`, pool checkout: stamp
  owner/index/+8, NULL when full; index is a 16-bit store into a dword)
  and **`sel_list_mark_last`** (`FUN_00414080`, single-selection: mark
  the last list entry, clear the rest). In the menu-spawn block they run
  back to back (append → mark-last → acquire controller).

**441 host tests pass, 0 fail, 6 skip (of 447); both cross-build exes
clean.** New quirks #30 (consume-on-read 100 ms input window) and #31
(16-bit index store). Ledger **115/1490 touched (7.0%), 112 tested**.

**Correction to the old HANDOFF's guess:** `0x411390` is NOT the menu
"action switch" — it's a **sound-effect/wave player** (calls
`FUN_005bb250`/`_5bb2f0`, "Failed Wave Load"), audio-subsystem coupled
(milestone 3). The action latch is `FUN_0043ce50`.

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). **115/1490 touched
  (7.0%), 9.5% of bytes**, 112 host-tested.
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness.
- `docs/findings/title-scene.md` — the function's full anatomy (phase FSM,
  pacing FSM, jump table, menu + input dispatch, joystick lazy-attach).
- `docs/findings/input.md` — **NEW**: the input ring layout, the poll, the
  menu button-id table, and what's still black-box (the producer).
- `docs/port-frontier.md` — DERIVED "what to port next": ~49 portable leaves.
- `docs/audit/subsystem-survey-2026-05-29.json` — raw 22-agent survey.
  **Mine this instead of re-running.**
- `findings/engine-quirks.md` #15–#31.

**Tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

**Structural-parity harness (offline foundation landed 2026-05-29):**
call-graph diff + mem-watch, mirroring `../openrecet`. How-to:
`docs/parity-harness.md`; design: `docs/plans/parity-harness.md`. Offline
pieces done + tested. The agent (`opensummoners-agent.js`) +
`frida_capture.py` call-trace/mem-watch modes + `tools/bisect_call_trace_vas.py`
are code-complete but **need a live retail-under-Frida run to verify**
(a human-verification gate; first move there = run `bisect_call_trace_vas.py`,
calibrating its `--boot-threshold`). The `mem_watch.py --region <+0x108
addr>:64:input_ring` run is now well-motivated — it catches the input-ring
*producer* (the GetDeviceState writer) that `input_poll_consume` reads from.

NB: only put a `FUN_<va>` token in `src/` for a function you have
actually ported — the ledger generator treats any `FUN_<va>` in src as a
port signal. Reference *unported* callees by bare VA (`0x412c10`), not
`FUN_00412c10`, or you'll inflate the headline.

## Module inventory (10 modules)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper,
cs_dispatch, app_pump (`FUN_005b1030`), **title_scene (`FUN_0056aea0`
partial — fade FSM + pacing FSM)**, **input (`FUN_0043c110`)**,
**obj_container (`FUN_00412c10` + `FUN_00414080`)**. Live boot zero
DDERR through 10 frames in mode 2. The drop-in still uses its own minimal
`main_loop_body`; the ported title FSMs + the new leaves are **not yet
wired** into a real scene loop in main.c.

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
Both pure FSMs (fade + pacing) done; the update half's pure leaves (input
poll + container ops) done. Remaining: assemble the menu update half, and
the render half (the path that actually draws + Flips).

## Next move (pick one — recommendation first)

1. **(recommended) Model the menu-list-controller object and port its
   pure leaf, then the cursor-nav engine.** The list header lives at
   `menu_ctrl + 0x174` with fields `+0xc`=page stride, `+0x10`=count,
   `+0x14`=cursor, `+0x18`=scroll-top, `+0x1c/+0x20`=key-repeat timers.
   Start with the clean 52 B leaf **`FUN_004192b0`** (scroll-into-view:
   recompute `+0x18` = `floor(cursor/stride)*stride`, return 1 if it
   moved) → new `src/menu_list.{c,h}`. Then tackle the cursor-nav engine
   **`FUN_0043ca40`** (970 B — but its jump table is Ghidra-unrecovered;
   recover it in r2 first, like the title jump table) and the latch
   **`FUN_0043ce50`** (220 B, calls `_43ca40`) on top. That completes the
   input→action chain the menu needs.

2. **Assemble the menu-spawn block** (`0x56aea0` default branch lines
   385–465): needs the still-unported helpers `0x40f3e0` (434 B, list
   append), `0x40f5c0` (563 B), `0x411f40` (slot finalize), plus the now-
   ported `obj_pool_acquire`/`sel_list_mark_last`/`FUN_004192b0`. Models
   the 5-slot menu populate (action IDs 0x1a,0x1c,0x1e,0x1d,8 into
   `local_60->[0x174]/[0x17c]`) and the `param_1` skip-intro early-out.

3. **Checkpoint 4: the render half (`0x56bb04`)** — the path that draws.
   `PTR_DAT_0056bfa4[local_64]` jump-table call (11 entries, 7 handlers)
   → per-phase draw bridges → `FUN_0056c180(...->[0x16c])` + "Title Menu -
   Flipping" log + `FUN_005b8fc0(hWnd)` (the DDraw Flip). Heavily
   DDraw/object-model-coupled, so harder to unit-test than 1/2.

4. **Live harness gate** — run `bisect_call_trace_vas.py` /
   `mem_watch.py --region <+0x108>:64:input_ring` under Frida to verify
   the call-trace + mem-watch machinery and catch the input-ring producer.
   This is the human-in-the-loop step (needs the game + Frida host).

## Open RE threads (see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — milestone 0. Pure FSMs + update-
  half leaves done; menu assembly + render half remain (next move above).
- **Input** `FUN_0043c110` poll **DONE**. Remaining: the action latch
  `FUN_0043ce50` + cursor-nav `FUN_0043ca40`, and the **producer** that
  fills the `+0x108` ring (DInput `GetDeviceState`, vtable `[0x24]`) —
  black box; `mem_watch.py` is the tool to find it. See `findings/input.md`.
- **Audio ZDM** `FUN_005bab10`/`_5bc150` + the SFX player `FUN_00411390` —
  milestone 3 (WMF/COM, hard).
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4.
- **Hash-id asset directory** `FUN_00556eb0` — recover the ID→name table
  (have Arche `0x5f5e165`, Sana `0x5f5e166`, Sophia `0x35a4e902` so far).
- God-object `DAT_008a9b50` layout (engine-quirks #15) — model as we go.
- `FUN_00563ef0` wave-load second half — milestone 5 support.
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
   `#if UINTPTR_MAX == 0xFFFFFFFFu` (host pointers are 8 B).
4. **Append any engine quirk** you find to `findings/engine-quirks.md`.
5. **Regenerate the derived artifacts** (`gen_port_ledger.py` +
   `gen_frontier.py`) when a port lands.
6. Update THIS file at each meaningful checkpoint; append to PROGRESS.md.
7. **Suggest a `/clear`** at the natural stop point — the docs are the
   durable memory, not context.
