# tools/drive — agents drive the game (`rd`)

An agent authors verified traces on its own: it steps the game live, reads state by name, searches for inputs on
in-process savestates, looks at frames, and files the result as a scenario that retail, the hybrid and the standalone
are then gated against. Human recordings (`rhost.py record`) cover what a player did; this covers what nobody played —
every menu page, every area, failure arms, the whole route.

Method = the Kirby playbook (`/opt/src/kirbys-adventure/tas/PLAYBOOK.md`) as carried furthest by openrecet
(`/opt/src/openrecet/tools/drive/README.md` — the reference this port follows): a trace is a program; a goal is ordered
state predicates; the model names state and designs the search, a local search finds the inputs; done = checked on a
fresh replay + looked at. Agent card: **`DRIVER.md`**. Design + evidence: `docs/findings/headless-devices.md`.

## What the target provides

| level | rhost | figure (wine, maximus, 2026-11-13) |
|---|---|---|
| L0 replay | any trace, real devices or `--headless` (no X, no audio, no Xvfb) | headless 530–610 fps plain; Windows real device ~800 |
| L1 step | `rhost --serve PORT`: frames on demand, memory, predicates, screen text, **PNGs from memory** | ~370 frames/s in a room with write tracking; shot ~20 ms |
| L2 snapshot | `rh_snap.c` savestates at the Flip boundary | restore 0.6 ms |
| search | `explore` in the process (cells, goal, avoid, score, hold), `-j N` sessions | |

Differences from openrecet, all from this engine: the frame boundary is the Flip (50 call sites → the flip coroutine);
input is DirectInput key LEVELS (a 32-bit mask over the keys the game reads: `KEYS` here = `KEYMAP` in rh_serve.c);
DirectDraw is a sysmem 2-D blitter, so headless frames are the real pixels (`fb` equal to the real device's) and pictures
need no replay; saves are a `user\` dir (statue saves), filed under `tests/scenarios/_saves/<digest>/` (gitignored).

## Pieces

| file | what |
|---|---|
| `drive.py` (`./rd`) | the stateless CLI: runs, movies, sessions, expressions, search, pictures, check / verify, filing, gates |
| `DRIVER.md` | the one-page card a driving agent reads |
| `vars.json` | named SotES state shared by all runs (validated entries only) |
| `tasks/*.md` | task files: start, ordered predicates, forbids, prose |
| `agent.py` / `agent.sh` | the API-billed driver (Messages API tool loop, hard budget, log); `agent.sh check` = is there credit |
| `api_check.py` | the credit check (one ~10-token Haiku request; verdict cached 15 min) |
| `../rhost/rh_dev.c` `rh_snap.c` `rh_serve.c` | headless devices, savestates, the command server |
| `../../docs/game/` | how the game plays, the full route, bosses, side content, the exercise checklist |

State on disk: `/mnt/ephemeral/opensummoners/drive/runs/<run>/` — `movie.jsonl` (an rhost trace), `meta.json`
(start, checkpoints, undo), `vars.json`, `session.json` (the live process: a cache), `view/*.png`. Sessions' rhost dirs:
`<wine root>/runs/drive/` (wine) or `C:\oss-rhost\runs\drive\` (Windows). A session ends after 30 idle minutes; the next
command replays the movie.

## Where sessions run (`RD_HOST`)

`auto` (default): the Windows box (`ssh $RHOST_WIN`, e.g. `win-wifi`; native, WMI-started in session 0, admitted by
`Global\oss-rhost-admit` + `queue\procs.txt` + ≤ `RD_WIN_MAX_SESSIONS`) → bunker (when `tools/bunker` is set up for
this project) → wine here (`admit_local`: ≤ `RHOST_MAX_PROCS` rhost processes, ≥ `RHOST_MEM_FLOOR_GB` available).
`RD_HOST=win|bunker|wine` forces one. Headless wine runs use their own prefix (`prefix-nox`), never `wineserver -k`'d by
an Xvfb batch, so sessions and headless batches run side by side.

## A run, end to end

```sh
./rd new t1 --from rh-intro-errands@2900          # replay a scenario's first 2900 frames into a live session
./rd tap t1 ent --n 60 --gap 9 --until "script == 0"
./rd play t1 "R*60" --watch "lx,ly" --every 20
./rd ck t1 save here ; ./rd undo t1
./rd search t1 --goal "room == 210120" --time-limit 30 --apply
./rd look t1 ; ./rd strip t1 --from 2800 --to 3076 --every 92      # Read the PNGs
./rd verify t1 [--real]                           # fresh headless replay = the session (+ real device: frame / RNG / pixels)
./rd accept t1 my-probe --task tools/drive/tasks/my-probe.md       # check → verify --real → file → bless → hybrid → standalone
```

## Campaign recipe (an orchestrating session)

1. **Pick targets from the gaps**: `docs/game/exercise-checklist.md`, `rhost.py coverage` (retail functions no scenario
   reaches), port frontier / PORT-DEBT rows marked unverified. One behaviour per task.
2. **Write the task** (`tasks/NAME.md`): `start:` the latest save that makes it reachable, ordered `steps:` as predicates
   over named state, `forbid:` lines, prose pointing at `docs/game/` sections and findings. Lint it (`rd lint`).
3. **Pick the driver kind automatically:** `tools/drive/agent.sh check` → exit 0 (API credit) =
   `tools/drive/agent.sh tasks/NAME.md --run NAME --budget N` (any number side by side; exit 4 = credit ran out
   mid-run → hand the task to a subagent); otherwise a Claude Code subagent on the session's own model (owner rule:
   no model override) with the brief "read tools/drive/DRIVER.md and tools/drive/tasks/NAME.md; your run is NAME;
   report per the card".
4. **Accept:** `rd accept R NAME --task T`. A hybrid / standalone FAIL is the payoff: port that code by the porting loop
   (CLAUDE.md), re-gate.
5. **Keep what was learned:** names → `vars.json`, game facts → `docs/game/` (corrections tagged [RD]) and DRIVER.md,
   tool gaps → fix the tool. **Chain by saves**: `rd endsave R` → `--from save:DIGEST`.

## Open

See `docs/findings/headless-devices.md` "Open" (headless state_sem words, SE 24-bpp surfaces, session speed, bunker).
