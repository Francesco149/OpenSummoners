# Session handoff — last updated 2026-05-29 (title scene runner ckpt 2)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 2 just landed** (this session): the `local_28`
**frame-pacing FSM** + the `FUN_005b1030` pump call sites — ported as
`title_pace_*` in `src/title_scene.{c,h}` (`title_pace_step`). A pure
fixed-16 ms-timestep accumulator: each iteration runs the *update* half
(input + the ckpt-1 phase FSM, burning the budget in 16 ms slices) or
the *render* half (draw + Flip), refilling the budget from the real
`GetTickCount` delta (clamped 100 ms) and pumping on the way into
update. `now` is passed in; pump-request + update/render decision are
reported via `title_pace_step_out` (Win32-free, no link deps).
**418 host tests pass, 0 fail, 6 skip (of 424); both cross-build exes
clean.** Decoded byte-for-byte from r2 `0x56b002..0x56b0c8`.

**Resolved the ckpt-1 open thread:** the `E` counter at `[esp+0x5c]`
(r2 showed it; Ghidra dropped it) is a **dead** consecutive-sub-second-
frame tally — full-function disasm scan finds it written-only/never-read,
Ghidra dead-store-eliminated it, and its window anchor `D = local_20` is
read only to gate that dead update. The *entire* `S==1` post-arm is
observably inert → omitted, behaviourally exact. Only the `S==2` arm
(`A = now`) is load-bearing. (See PROGRESS 2026-05-29, quirk #29, and
the per-line provenance in `title_scene.c`.) The pacer also explains the
`--turbo` "splash doesn't animate": frozen clock → budget never refills
past one slice → renders every frame with the phase FSM frozen.

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). **112/1490 touched
  (6.8%), 9.5% of bytes**, 109 host-tested. (Both ckpts left the number
  flat — `FUN_0056aea0` was already "touched"; progress = new refs.)
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness
  cards. Milestone 0 card describes the whole `FUN_0056aea0`.
- `docs/findings/title-scene.md` — the function's full anatomy. Now
  carries the decoded **"Frame-pacing sub-state machine"** section (ckpt 2)
  alongside the phase FSM, jump table, menu + input dispatch, joystick
  lazy-attach.
- `docs/port-frontier.md` — DERIVED "what to port next": 52 portable leaves.
- `docs/audit/subsystem-survey-2026-05-29.json` — raw 22-agent survey.
  **Mine this instead of re-running.**
- `findings/engine-quirks.md` #15–#29.

**Tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

**Structural-parity harness (NEW — offline foundation landed 2026-05-29):**
call-graph diff + mem-watch, mirroring `../openrecet`. How-to:
`docs/parity-harness.md`; design: `docs/plans/parity-harness.md`. Offline
pieces done + tested (`src/call_trace.{c,h}` probe, `tools/call_trace_diff.py`,
`tools/gen_engine_vas.py`, `tools/mem_watch.py` ranking). The agent
(`opensummoners-agent.js`) + `frida_capture.py` call-trace/mem-watch modes +
`tools/bisect_call_trace_vas.py` are code-complete but **need a live
retail-under-Frida run to verify** (the human-verification gate; first move
= run `bisect_call_trace_vas.py`, calibrating its `--boot-threshold`).

NB: only put a `FUN_<va>` token in `src/` for a function you have
actually ported — the ledger generator treats any `FUN_<va>` in src as a
port signal. Reference *unported* callees by bare VA (`0x412c10`), not
`FUN_00412c10`, or you'll inflate the headline.

## Module inventory (8 modules)

Pixel-Drawer (8 fns), Asset-Register (31), Bitmap-Session (8), WndProc
(`FUN_005b12e0`), ZDD wrapper (40+), cs_dispatch, app_pump
(`FUN_005b1030`), **title_scene (`FUN_0056aea0` partial — fade FSM +
pacing FSM)**. Live boot zero DDERR through 10 frames in mode 2. The
drop-in still uses its own minimal `main_loop_body`; `app_pump_frame`
and the two title FSMs are ported but **not yet wired** into a real
scene loop in main.c.

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
Both pure FSMs (fade + pacing) are now done. The remaining work is the
two loop halves the pacer dispatches to: the **update** half (input +
menu) and the **render** half (jump-table draw + Flip).

## Next move (pick one — recommendation first)

1. **(recommended) Knock out the update-half frontier leaves, then
   checkpoint 3 (menu + input).** The default-branch update half (phases
   8/9, asm `0x56b0ce..0x56bf2e`) needs: the menu-controller object
   (`0x412c10` alloc — a **zero-dep leaf, 46 B, ready today**), the input
   poll/latch (`0x43c110` — **leaf, 84 B, ready**; then `0x43ce50`,
   220 B, 1 dep), the slot-populate loop (action IDs `0x1a,0x1c,0x1e,
   0x1d,8` into `local_60->[0x174]/[0x17c]`), and the action switch
   (`0x411390`, 413 B). `0x414080` (63 B) is also a ready leaf. Port the
   leaves first (port-and-test rhythm shrinks the surface), then assemble
   ckpt 3. Models the consume-on-read ring buffer at `in_ECX[1]+0x108`
   and the `param_1` skip-intro early-out (`local_64 < 8`).

2. **Checkpoint 4: the render half (`0x56bb04`) — the path that actually
   draws.** The `PTR_DAT_0056bfa4[local_64]` jump-table call (11 entries,
   7 handlers `0x56bb5c..0x56be85`, already recovered in title-scene.md)
   → per-phase draw bridges → frame-end `FUN_0056c180(...->[0x16c])` +
   "Title Menu - Flipping" log + `FUN_005b8fc0(hWnd)` (the DDraw Flip).
   This is the literal "title menu drew a frame" path but is heavily
   DDraw/object-model-coupled (`DAT_008a93cc`/`DAT_008a93ec`, ZDDObject
   vtable `[0x16c]`), so it's harder to unit-test purely than option 1.

3. **Wire the scene loop into `main.c`** — once update+render exist,
   replace the minimal `main_loop_body` with `title_pace_step` driving
   `app_pump_frame` + `title_fade_step` + the two halves. Defer until
   there's a real consumer (cosmetic until then).

## Open RE threads (see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — milestone 0. Pure FSMs done;
  update + render halves remain (next move above).
- **Input** `FUN_0043c110`/`_43ce50` + DInput `FUN_005ba120` — milestone 1.
  Black box: who calls `GetDeviceState` (vtable `[0x24]`) to fill the
  `+0x108` ring buffer. **Now have the tool to find it:** point
  `tools/mem_watch.py --region <+0x108 addr>:64:input_ring` at it (live gate).
- **Audio ZDM** `FUN_005bab10`/`_5bc150` — milestone 3 (WMF/COM, hard).
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4.
- **Hash-id asset directory** `FUN_00556eb0` — recover the ID→name table
  (have Arche `0x5f5e165`, Sana `0x5f5e166`, Sophia `0x35a4e902` so far).
- God-object `DAT_008a9b50` layout (engine-quirks #15) — model as we go.
- `FUN_00563ef0` wave-load second half — milestone 5 support.
- Frida turbo: add `GetTickCount` + `WaitMessage` hooks to the agent
  (the engine uses `GetTickCount` exclusively; `timeGetTime` hook is a
  no-op here — see PROGRESS). Quirk #29 explains *why* turbo currently
  freezes the splash.

## How to apply

When the user says "continue RE work" (or similar):

1. Read this file first, then `STATUS.md` + `ROADMAP.md`.
2. Pick the recommended next move (or whichever the user redirects to).
3. Work port-and-test style: small unit → tests → commit. Each ported
   function gets a `FUN_XXXXXX` provenance comment (the ledger keys on it)
   and a test spot-checking behaviour vs hand-computed expectations.
4. **Append any engine quirk** you find to `findings/engine-quirks.md`.
5. **Regenerate the derived artifacts** (`gen_port_ledger.py` +
   `gen_frontier.py`) when a port lands.
6. Update THIS file at each meaningful checkpoint; append to PROGRESS.md.
7. **Suggest a `/clear`** at the natural stop point (see AGENT-WORKFLOW
   "Session lifecycle") — the docs are the durable memory, not context.
