# Title-menu scene runner — `FUN_0056aea0`

The post-launch driver `FUN_00562ea0` ends in a state-machine loop
whose first call (when `DAT_008a6e6c == 0`) goes to `FUN_0056aea0` —
the **title-menu scene runner**.  Its return value is the state code
the outer driver dispatches on (0, 6, 8, 9, 0x1a..0x1e — see
`docs/findings/winmain-and-bootstrap.md` "Scene driver").

This is the **first scene** the user perceptually sees: studio splash,
title logo fade-in, "press button" prompt, top-level menu (New Game /
Continue / Options / Exit), and outer joystick/save-data plumbing
lazy-init.  Mapping it gives us the bridge between init being done
and the engine actually drawing pixels through DDraw.

## Shape

3441-byte function; one outer `do { … } while(1)` with three nested
state variables:

| local var      | meaning                                                   |
|----------------|-----------------------------------------------------------|
| `local_28`     | pump / pacing sub-state (0 = first frame, 1 = render-ready, 2 = updating) — see "Frame-pacing sub-state machine" below |
| `local_64`     | **scene phase** (the inner switch dispatch — 0..7, 10, …) |
| `local_68`     | per-phase tick counter (frame count within the phase)     |
| `local_30`     | tick-budget remaining for the current frame (ms)          |
| `local_34/2c`  | wall-clock anchors via `GetTickCount`                     |
| `local_60`     | menu-controller pointer (lazily loaded in the default branch) |
| `param_1`      | "skip intro" flag — when non-zero the engine still draws frame 0 of `local_64` but proceeds into the menu without running the splash phases |

The pump call `FUN_005b1030()` (the documented frame limiter — see
`winmain-and-bootstrap.md` "Main message pump") is invoked at lines
103 and 113, once per loop iteration depending on the sub-state.
The `0x10` (16 ms) and `0x11` (17 ms) constants on `local_30` are the
~60 Hz frame budget.

## Frame-pacing sub-state machine (`local_28`)

Decoded byte-for-byte from the r2 disasm at `0x56b002..0x56b0c8`
(`e asm.sub.var=false`).  This is a **fixed-16 ms-timestep accumulator**:
it runs the *update* half (input + the `local_64` phase FSM) as many
16 ms slices as the banked wall-clock budget allows, then runs the
*render* half (jump-table draw + Flip) once and refills the budget.

State (raw esp displacement at the loop top, where esp is stable):

| var | retail local / slot | meaning |
|-----|---------------------|---------|
| `S` | `local_28` `[esp+0x50]` | sub-state: 0 = first frame, 1 = render-ready, 2 = updating |
| `B` | `local_30` `[esp+0x48]` | frame budget in ms (init `0x11` = 17) |
| `C` | `local_34` `[esp+0x44]` | `GetTickCount` at the last pump |
| `A` | `local_2c` `[esp+0x4c]` | `GetTickCount` at the last update entry |
| `now` | `ebp` | `GetTickCount` sampled once per iteration (`0x56b002`) |

Per-iteration logic (`now` = this frame's `GetTickCount`):

```
S==0:  C = now;  pump();  S = 2.                          (0x56b07d)
S==1:  B = min((B - C) + now, 100);  C = now;  pump();
       if (B > 16) S = 2.            (else stay render)   (0x56b051)
S==2:  if ((now - A) > B)  { B = 0;  S = 1; }
       else { B -= 16;  if (B <= 16) S = 1; }             (0x56b01f)
post:  if (S == 2)  A = now;         (S==1 arm is dead — see below) (0x56b08f)
disp:  S==1 → render (jump 0x56bb04);  S==2 → update (fall to 0x56b0ce). (0x56b0be)
```

All compares are **unsigned** (`jbe`/`ja`), and the `(now - anchor)`
deltas are modular `uint32`, so the 49.7-day `GetTickCount` rollover is
handled the same as retail.  The pump fires only on the `S==0` and
`S==1` paths (entering update), never on a pure update slice.

**Dead locals (omitted from the port).**  The `S==1` arm of the
post-block (`0x56b09d..0x56b0ba`) maintains a 1-second-window frame
counter `E = [esp+0x5c]` (incremented while `now − D ≤ 1000`, reset with
its window anchor `D = local_20 [esp+0x58]` otherwise) — an FPS / uptime
tally.  A full-function disassembly scan finds `E` **written only, never
read**; Ghidra dead-store-eliminates `E`; and `D`'s only read just gates
that dead update.  So the whole `S==1` post-arm is observably inert and
is dropped — behaviourally exact.  Only the `S==2` arm (`A = now`) is
load-bearing (the next update slice tests `now − A` against `B`).
**Turbo consequence:** with a frozen clock the budget never refills past
one slice, so after the first update the loop renders every frame with
the phase FSM frozen — the documented `--turbo` "splash doesn't animate".

Ported as the pure `title_pace_step` in `src/title_scene.c`
(checkpoint 2); see also `findings/engine-quirks.md` #29.

## Inner scene-phase dispatch (`local_64`)

Each phase is a piece of the intro / title animation, advanced when a
fade ramp (`uVar15`, range 0..1000) or a tick counter (`local_68`)
crosses a per-phase threshold.

| phase | duration                | what it does                                                          |
|-------|-------------------------|-----------------------------------------------------------------------|
| 0     | fade 0→1000 by +20/tick | studio-logo fade-in                                                   |
| 1     | tick 0→50               | hold studio logo                                                      |
| 2     | fade 1000→0             | studio-logo fade-out; logs `Title Menu - SetNextSegment`              |
| 3     | fade 0→1000 by +10/tick | title-logo fade-in                                                    |
| 4     | tick 0→20               | hold title logo (short)                                               |
| 5     | fade 0→1000 by +100/tick + tick 0→40 | "Press button" animation in                            |
| 6     | fade 0→1000 by +10/tick + tick 0→120 | hold "Press button" + ambient                          |
| 7     | fade 0→1000 by +20/tick; calls `FUN_0056c070(0x15, 0, 8, 800, …, 0x10, 0x18, 0x14, 0x14)` while `uVar15 < 0x352` (850/1000) | spawns title-screen particles / sparkles, then hands off to phase 8 (menu) |
| 8/9   | the *default* branch    | top-level menu — handles input, draws menu items, sub-menu transitions; phase 9 fades volume/brightness for transitions |
| 10    | end-of-flow gate        | (after phase ≥ 10) jumps to `LAB_0056ba69` which returns to the dispatch table at `PTR_DAT_0056bfa4[local_64]` for menu-action resolution |

After phase ≥ 10 the function jumps to `LAB_0056bb55` which dispatches
through an indirect-jumptable `PTR_DAT_0056bfa4[local_64]`.  Ghidra
couldn't recover it ("too many branches" — table has duplicate
targets) but `radare2 -c 'pxw 0x2c @ 0x56bfa4'` reads it cleanly.
The table is 11 dwords (one per `local_64` phase 0..10) → 7 distinct
handler addresses:

| local_64 | handler addr | meaning                                            |
|----------|--------------|----------------------------------------------------|
| 0, 1, 2  | `0x0056bb5c` | studio-logo phases — shared early-intro handler    |
| 3, 4     | `0x0056bbd4` | title-logo phases — shared logo handler            |
| 5        | `0x0056bc4d` | "press button" fade-in                             |
| 6        | `0x0056bca2` | "press button" hold + ambient                      |
| 7        | `0x0056bcf7` | sparkle-spawn-then-handoff                         |
| 8, 9     | `0x0056bdb9` | menu phases — top-level menu / fade-out transition |
| 10       | `0x0056be85` | end-of-flow gate (post-menu)                       |

Note that the call site is `(*(code *)(&PTR_DAT_0056bfa4)[local_64])();`
— it's a **call** through the table (function-pointer dispatch, not a
fallthrough switch).  Each handler is a small bridge routine that runs
the phase-specific drawing then returns.  Phase counts in the table
(11) match the `local_64 < 0xb` bounds check at line 152.

Plain return values:

- `local_64 == 0` → `FUN_005b9410()` first (resets something), then
  `PTR_DAT_0056bfa4[0]()` is called.
- `2 ≤ local_64 < 4` → before the dispatch, runs
  `FUN_005b9b70(DAT_008a93cc->[0x16c], 0, 0)` — touches the primary
  surface (vtable offset 0x16c is past the standard surface vtable;
  this is likely a ZDDObject method, not a raw DDraw call).
- `local_64 ≥ 11` → calls `FUN_0056c180(...->[0x16c])` then logs
  `"Title Menu - Flipping"`, then `FUN_005b8fc0(hWnd)` — these are
  the **frame-end Flip** path the title screen takes; this is the
  hot path for "title menu drew a frame".

## Input dispatch (default branch / post-intro menu)

> **Ported (checkpoint 8):** step 1 below — the one-shot menu spawn — is
> now `title_menu_spawn` / `title_menu_teardown` in `src/title_scene.c`.
> It revealed that `local_60` is the menu *node*'s lone child, acquired by
> reinterpreting the node as an `obj_pool`; see `findings/menu-list.md`
> "The spawn block" and **engine-quirks #38**.  Steps 2–3 (the per-frame
> input poll/latch and the action switch) remain part of the update half.

Once we're in the default branch (after phase 7 hands off), each frame:

1. Spawn the menu-controller (`local_60 = FUN_00412c10()`) on first
   entry; populate 5 menu-item slots in `local_60->[0x174]/[0x17c]`
   with action IDs `0x1a, 0x1c, 0x1e, 0x1d, 8`:

   | slot idx | action_id | likely meaning                       |
   |----------|-----------|--------------------------------------|
   | 0        | 0x1a      | (state code `0x1a` from outer driver — see below) |
   | 1        | 0x1c      | load-save submenu (state `0x1c`)     |
   | 2        | 0x1e      | options (state `0x1e`)               |
   | 3        | 0x1d      | (state `0x1d`)                       |
   | 4        | 8         | exit (state 8 returns from driver)   |

   The order matches the outer driver's `switch(next)` cases.

2. Poll buttons via `FUN_0043c110(now, btn_id)`:
   - `0x22` (34) → returns state 6 (the outer driver also exits on 6)
   - `2, 4` → "down" / "right" menu navigation, calls `FUN_0043ce50(2/3)`
   - `1` → "up" → `FUN_0043ce50(0)`
   - `3` → "left" → `FUN_0043ce50(1)`
   - `0x24` (36) → `FUN_0043ce50(9)` = back / cancel
   - If no button: synthesise `FUN_0043ce50(4|5)` or `(6|7)` based on
     `iVar7 + 0x114/0x118` flags (probably axis-held states for
     keyboard players).

   `FUN_0043ce50(N)` is the **input-action latch** — N is the menu
   action enum.

3. Final `switch(iVar14)` (iVar14 = the resolved action):
   - 1, 2 → `FUN_00411390(9, …)` (commit)
   - 3 → check selected slot's action_id; if 0x1d → push `FUN_00411390(6, …)`; else `FUN_00411390(5, …)` (confirm)
   - 4 → `FUN_00411390(7, …)` (cancel)
   - On `iVar14 == 3` (confirm), additionally runs the **joystick
     lazy-init block**: walks the 2-slot `&DAT_008a93dc` array,
     for each empty slot calls `FUN_005ba120(&slot, hWnd, idx)` →
     if non-null, `FUN_005ba290()`.  These are DInput pad devices
     attached on-demand the first time the user actually picks a menu
     entry.  Pre-attaching them at boot was avoided to keep the
     "no joystick? no problem" boot path clean.

## Notes for the drop-in port

1. **The intro animation is ~7 phases of fade ramps and tick holds**
   driven by `FUN_005b1030`'s frame-budget machinery.  Phases 0-7
   together take roughly: 50 + 20 + 40 + 120 + 90 ≈ 320 ticks ≈ 5.3 s
   real time at 60 Hz.  In `--turbo` mode this runs faster than the
   asset I/O — see the `--turbo` analysis in PROGRESS.md (the splash
   appears not to render because turbo skips the timed phases while
   I/O still takes real wall-clock).
2. **Frame-end happens in phase ≥ 11** via `FUN_0056c180` +
   `FUN_005b8fc0(hWnd)` (the "Flipping" log marker).  Hooking either
   gives us a clean "title menu frame committed" event.  `FUN_005b8fc0`
   is presumably the call site for `IDirectDrawSurface7::Flip`
   (vtable offset 0x2C, method 11 — see `ddraw-init.md`).
3. **The menu-action enum we observed (`0x1a, 0x1c, 0x1e, 0x1d, 8`)**
   maps directly to the outer driver's `switch(next)`.  So the
   chain is: title menu picks an entry → outer driver dispatches by
   the action's state code → the corresponding "next scene" runs
   (e.g. `0x1c` triggers the save-data list scene via `FUN_0056a670`).
4. **Joystick attach is lazy** — the engine does *not* enumerate
   DInput pads at boot.  First menu-confirm (action `iVar14 == 3`)
   attaches up to 2 pads.  For the harness, this means a headless run
   that never reaches "press start" will never call `FUN_005ba120`;
   that's the expected behaviour and not a bug.

## Files referenced

- `docs/decompiled/by-address/56aea0.c` — full function (3441 B).
- `docs/decompiled/by-address/562ea0.c` — caller (post-launch driver).
- `PTR_DAT_0056bfa4` — 11-entry phase-handler dispatch table at
  0x56bfa4 (44 bytes), recovered via radare2 — see "Inner scene-phase
  dispatch" above for the resolved targets.
