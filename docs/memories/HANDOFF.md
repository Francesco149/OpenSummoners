# Session handoff — last updated 2026-05-29 (title scene runner ckpt 1)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

Mid-way through **milestone 0 (title screen renders)** — the
multi-checkpoint port of `FUN_0056aea0` (3441 B title scene runner).

**Checkpoint 1 just landed** (this session): the pure intro-phase /
menu-fade state machine — the `switch(local_64)` core — is ported as
`src/title_scene.{c,h}` + `tests/test_title_scene.c` (19 tests). It's a
side-effect-free `title_fade_step()` that advances (phase, fade, tick,
menu_fade) one frame and reports BGM-cue / sparkle-spawn requests via a
`title_fade_step_out` descriptor (no link deps on unported helpers).
**406 host tests pass, 0 fail, 6 skip; cross-build clean.** See
PROGRESS.md 2026-05-29, quirk #28, and the per-case provenance in
`title_scene.c` (verified against radare2 disasm `0x56b153..0x56b5c1`).

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). **112/1490 touched
  (6.8%), 9.5% of bytes**, 109 host-tested. (Binary ledger: the title
  runner was already "touched", so checkpoint 1 didn't move the number —
  progress shows only as new provenance refs.)
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness
  cards. Milestone 0 card describes the whole `FUN_0056aea0`.
- `docs/findings/title-scene.md` — the function's full anatomy: the two
  interleaved FSMs, the recovered `PTR_DAT_0056bfa4` jump table, the menu
  + input dispatch, the joystick lazy-attach.
- `docs/port-frontier.md` — DERIVED "what to port next": 52 portable leaves.
- `docs/audit/subsystem-survey-2026-05-29.json` — raw 22-agent survey.
  **Mine this instead of re-running.**
- `findings/engine-quirks.md` #15–#28.

**Tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

NB: only put a `FUN_<va>` token in `src/` for a function you have
actually ported — the ledger generator treats any `FUN_<va>` in src as a
port signal. Reference *unported* callees by bare VA (`0x412c10`), not
`FUN_00412c10`, or you'll inflate the headline (learned this checkpoint).

## Module inventory (8 modules now)

Pixel-Drawer (8 fns), Asset-Register (31), Bitmap-Session (8), WndProc
(`FUN_005b12e0`), ZDD wrapper (40+), cs_dispatch, app_pump
(`FUN_005b1030`), **title_scene (`FUN_0056aea0` partial — fade FSM only)**.
Live boot zero DDERR through 10 frames in mode 2. The drop-in still uses
its own minimal `main_loop_body`; `app_pump_frame` ported but not yet
wired into main.c's per-frame loop.

## Active goal

**Finish `FUN_0056aea0` so the title screen draws a frame** (milestone 0).
Checkpoint 1 (fade FSM) is done. The remaining checkpoints wire the rest
of the function around it.

## Next move (pick one — recommendation first)

1. **(recommended) Checkpoint 2: the `local_28` frame-pacing FSM + the
   outer loop skeleton.** Port the pump-coupled pacing sub-state machine
   (asm `0x56b002..0x56b0c8`, decoded with raw stack offsets via
   `e asm.sub.var=false`) that calls `app_pump_frame` (`FUN_005b1030`,
   ported) and decides each iteration whether to run the *update* half
   (`S==2` → falls through to the input + `switch(local_64)` phase FSM,
   already ported) or the *render* half (`S==1` → jumps to `0x56bb04`,
   the jump-table draw + flip). Build it as a pure `title_pace_step` with
   the GetTickCount value passed in (app_pump-style). **Decoded state
   machine** (S = sub-state at `[esp+0x50]`; now = GetTickCount; B =
   budget `[esp+0x48]`; A `[esp+0x4c]`; C `[esp+0x44]`; D `[esp+0x58]`;
   E `[esp+0x5c]`):
     - `S==0`: C=now; pump(); S=2.
     - `S==1`: B=min((B−C)+now,100); C=now; pump(); if B>16 → S=2 (else stays 1).
     - `S==2`: if (now−A)>B → {B=0; S=1}; else {B−=16; if B≤16 → S=1}.
     - post: if S==2 → A=now; if S==1 → if (now−D)≤1000 → E++ else {E=0; D=now}.
     - dispatch: S==1 → render(`0x56bb04`); S==2 → update (fall through).
   ⚠️ **Divergence resolved:** Ghidra (lines 129–136) dropped the E
   counter (`[esp+0x5c]` inc/reset) entirely — **r2 is authoritative**.
   Still TODO for ckpt 2: confirm whether E is ever *read* (dead vs live)
   — couldn't pin it this session because esp shifts under arg pushes.
   If dead, omit with a note; if live, model it.

2. **Checkpoint 3: the menu + input default branch (phases 8/9).** Needs
   the menu-controller object model (`0x412c10` alloc, the `+0x174`/`+0x17c`
   slot arrays) + input poll/latch (`0x43c110`/`0x43ce50`) + action switch
   (`0x411390`). Several are frontier leaves — port them first to shrink
   the surface. Models the consume-on-read ring buffer at `in_ECX[1]+0x108`.

3. **Knock out frontier leaves** (`docs/port-frontier.md`) — the title
   runner's own callees (`0x412c10`, `0x43c110`, `0x414080`, `0x411f40`)
   are zero-dep leaves; porting them first feeds checkpoint 3.

4. **Wire `app_pump_frame` into `main.c`** — small cosmetic chip; defer
   until the scene runner gives it a real consumer.

## Open RE threads (now mapped — see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — milestone 0, card in ROADMAP.
- **Input** `FUN_0043c110`/`_43ce50` + DInput `FUN_005ba120` — milestone 1.
  Black box: who calls `GetDeviceState` (vtable `[0x24]`) to fill the
  `+0x108` ring buffer — Frida-hook to find it.
- **Audio ZDM** `FUN_005bab10`/`_5bc150` — milestone 3 (WMF/COM, hard).
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4.
- **Hash-id asset directory** `FUN_00556eb0` — recover the ID→name table
  (have Arche `0x5f5e165`, Sana `0x5f5e166`, Sophia `0x35a4e902` so far).
- God-object `DAT_008a9b50` layout (engine-quirks #15) — model as we go.
- `FUN_00563ef0` wave-load second half — milestone 5 support.

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
