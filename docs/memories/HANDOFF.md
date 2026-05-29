# Session handoff — last updated 2026-05-29 (audit + workflow tooling)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

This checkpoint was a **project-wide audit + tooling setup**, not a code
port. We brought OpenSummoners up to the sibling-project (OpenMare /
OpenLords2) workflow standard and mapped the whole binary so future
sessions never re-analyze the same code.

**New this checkpoint — read these for orientation:**

- `docs/STATUS.md` — 60-second coverage headline (DERIVED).
  Currently **112/1490 engine-proper functions touched (6.8%), 9.5% of
  code bytes**, 109 host-tested.
- `docs/ROADMAP.md` — the semantic layer: **11-milestone order** (title →
  menu → audio → new game → movement → dialogue → battle → save →
  tutorial dungeon), a full **subsystem map** of every address band, and
  **port-readiness cards** for the next 5 forward-path clusters.
- `docs/port-frontier.md` — DERIVED "what to port next": 114 frontier
  functions, **52 zero-dependency leaves portable today**.
- `docs/port-ledger.{md,json}` — DERIVED per-function port status.
- `docs/findings/INDEX.md` — map of the 10 subsystem RE writeups.
- `docs/audit/subsystem-survey-2026-05-29.json` — the raw structured
  result of the 22-agent survey (16 band-maps + 6 scout cards + 136
  quirks). **Mine this instead of re-running the survey.**
- `findings/engine-quirks.md` #15–#27 — the load-bearing/charming quirks
  the survey surfaced (god-object singleton, frame-pump `return 6` quit
  convention, hash-id asset directory *with recovered character names*,
  struct strides, the LCG RNG, WMA-temp-file BGM, …).

**New tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

Regenerates STATUS / ledger / frontier from `src/` `FUN_<va>` provenance
comments + `functions.csv`. `--check` on the ledger gen exits 3 if stale
(pre-commit-ready). The survey workflow is `tools/workflows/subsystem-survey.js`.

## State before the audit (unchanged — still current code state)

Seven modules ported + wired (387 host tests pass, 0 fail, 6 skip;
cross-build clean with mingw). Live boot zero DDERR through 10 frames in
mode 2. The drop-in still uses its own minimal `main_loop_body`;
`app_pump_frame` is ported but not yet wired into main.c's per-frame loop.

- **Pixel-Drawer** (8 fns, 39 tests), **Asset-Register** (31, 111),
  **Bitmap-Session** (8, 31), **WndProc** (`FUN_005b12e0`, 19),
  **ZDD wrapper** (40+, 153), **cs_dispatch** (21), **app_pump**
  (`FUN_005b1030`, 16).

See `docs/STATUS.md` "Current front" and the file layout in PROGRESS.md
2026-05-25 for the full module breakdown.

## Active goal

**Get the title menu rendering** (ROADMAP milestone 0). All prerequisite
"small leaves" are ported. The remaining work is the scene runner itself,
`FUN_0056aea0` (3441 B) — a multi-checkpoint port.

## Next move (pick one — recommendation first)

1. **(recommended) Begin the title-menu scene runner port
   (`FUN_0056aea0`).** Read the fresh card in `ROADMAP.md` (milestone 0)
   and `findings/title-scene.md`. Start with the outer skeleton +
   state-vars (`local_28`, `local_64`, `local_68`, `local_30`) and the
   pump-callsite plumbing. The `PTR_DAT_0056bfa4` jump table needs
   radare2 to recover (Ghidra gave up) — 7 handlers over 11 phase
   indices. Stub unported helpers as TODO panics so the structure lands
   first. Multi-checkpoint by design.

2. **Knock out frontier leaves** (`docs/port-frontier.md`) — 52 portable
   today, several are the title runner's own callees (`0x412c10` menu
   alloc, `0x43c110` input poll, `0x414080`, `0x411f40`). Porting these
   first shrinks the runner's stub surface.

3. **Wire `app_pump_frame` into `main.c`** — small cosmetic chip; defer
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
