# Driver card: a verified Fortune Summoners trace with `rd`

For a coding agent with a shell (a Claude Code session or subagent, or `tools/drive/agent.sh`). You turn a task — ordered
state predicates, or "reach / exercise X" — into an input movie that a fresh game process replays to the same state.
Everything runs locally and is free; your turns are the cost, so work in big strides: one `play`, `tap` or `search` per
sub-goal, not one per key.

`rd` = `/opt/src/OpenSummoners/rd` (run it from the repo root). The game runs headless (no window); state is numbers
and text, and **pictures are instant** (`rd look`: the frame straight from the session's memory, ~20 ms) — look often.

## The loop

1. **Read the task and what it points to** (`docs/game/` for how the game plays and the route; findings docs, IDA
   names). Say in state terms what each step means before touching the controls.
2. **Create your run:** `rd new <run> --from <start>` (the task's `start:`). Never write into another run.
3. **Per sub-goal: act, then read state.** `rd play` / `rd tap` with `--until EXPR` stop on the frame the predicate turns
   true. For walking / jumping / timing, **search instead of steering**: `rd search <run> --goal EXPR --avoid EXPR --apply`.
4. **When something does not happen, find out why before repeating it:** `rd status`, `rd text` (the strings on screen),
   `rd look` (a PNG — Read it), `rd eval`, `rd ram mark` → act → `rd ram diff`. Never run the same failing thing twice.
5. **Checkpoint before anything you might redo:** `rd ck <run> save NAME`; `load NAME` returns in under 1 ms. A load is a
   step back in time: the movie is cut there. `rd undo` reverts the last state-changing command (8 deep).
6. **Finish:** `rd check <run> TASK.md` PASS (if there is a task file), `rd verify <run>` MATCH, and you have LOOKED at the
   moment the trace is about (`rd look`, `rd strip`). Then report: run name, frames, each step's frame, state you named,
   game facts you learned, what the tool lacked.

One command at a time per run (a run is locked while a command works). Give shell calls a timeout above a search's
`--time-limit` (default 60 s + refine; 300 s is safe).

## Commands

```
rd new R --from fresh | SCENARIO[@F] | RUN[:CK|@F] | end:SCENARIO | save:DIGEST   [--force]
rd play R "SPEC" [--until EXPR] [--avoid EXPR] [--watch EXPR,EXPR] [--every N] [--rows N]
rd tap R "ent" [--n 40] [--gap 8] [--until EXPR] [--text]     # (KEY .*gap) x n: advance dialogue, mash a menu
rd status R        rd text R        rd eval R EXPR [EXPR..] [--at CHECKPOINT | --at-frame F]
rd scan R --from F [--to T] --watch "EXPR,EXPR.." [--every N] [--changes] [--where EXPR]
rd ram R list | read VA [NBYTES] | dwords VA [N] | str VA | define NAME --addr HEX --type s32 | --expr "..." [--status] [--event]
rd ram R mark ; (act) ; rd ram R diff [--lo VA --hi VA] [--save NAME] [--minus NAME] [--show N]
rd ck R save NAME | load NAME | list        rd undo R        rd rewind R --to F
rd search R --goal EXPR [--avoid EXPR] [--cell "(a, b)"] [--actions SPEC ..] [--time-limit S] [--apply] [-j N]
            [--score EXPR [--apply-best] [--greed G]] [--hold N [--hold-actions SPEC ..]]
rd look R [--at F,F]        rd strip R --from F --to F [--every N]      # PNGs → Read them
rd check R TASK.md        rd verify R [--real]        rd lint TASK.md --on RUN
rd endsave R        rd file R NAME        rd stop R        rd ls
```

**Input spec:** tokens separated by spaces. One-letter keys combine: `U D L R` arrows, `X` (attack / talk / examine /
confirm), `C` (jump; cancel; closes dialogue), `Z` (draw / sheathe sword; casters: spell select), `V` (examine only), `F`
(healing item) — `RX` = Right+X, `RC` = run-jump right. Named keys (join with `+`): `ent` (Enter: change character, skip /
advance text, menu confirm), `esc`, `bk` (Backspace: cancel), `f1` (Main Menu) .. `f6`, `n0`..`n9` (numpad spell select),
`pu pd home` (move POV), `sp` (Space). `.` = nothing. Repeat `*N`, group `( … )xN`: `ent .*8 (D .*5)x3 X*2 .*30`. A press is
an edge: release (`.`) between two presses of the same key. Double-tap ← / → to dash: `R . R*20`.

**Inputs that read state:** `{EXPR}*N` holds the mask EXPR evaluates to, re-read before every frame (the movie records the
masks it gave). In EXPR, key letters are masks (`X`, `RC`), `key("ent", "f1")` any key, `toward(dx[, dead])` = R or L along
dx (screen x grows right), `pad(dx, dy)` = the 8-way arrows along (dx, dy) (menus). Walk to x = 90000: `{toward(90000 - lx,
300)}*120`.

**Expressions** (goals, `--until`, `--watch`, `eval`, task steps): Python syntax over named state (`rd ram R list`),
`frame`, `rng`, raw reads `u8(va) s8 u16 s16 u32 s32 f32 f64` (nestable: `s32(s32(0x8a9b50) + 0x1038)`), arithmetic,
comparisons, `and or not`, `x in (1, 2)`, `a if c else b`, `min max abs int sqrt sin cos atan2 hypot pow`, `sum(..)`.
Conventions: the state read is the one AFTER the frame ran; a session at frame F has run F frames since boot.

## Search: how to think

`rd search` keeps one savestate per **cell**, expands rarely-visited cells with random macro-actions until the **goal**
holds, then spends `--refine` seconds shortening the path.
- **Goal:** the step's predicate, or a stepping stone ("a room I have not been in": `room != 210140`).
- **Cell:** what counts as a different situation. Default `(room, lx // 1600, ly // 1600)`. Put the mechanic's state in it —
  a menu cursor, a dialogue line, an enemy's HP, a timer `frame // 20` when nothing moves.
- **Avoid:** every constraint that must survive (`hp < 40`, `room != 210140`).
- **Actions:** short macro-actions. Default: walk R / L (8 and 2 frames), run-jumps `RC*8 LC*8`, a jump `C*12`, attack
  `X .*7`, door / guard `U .*7`, crouch `D*8`, wait `.*8`. Give what the sub-goal needs.
- **Score (`--score EXPR`, higher = closer)** gives direction; `--apply-best` keeps the best cell's path when the goal is not
  reached in time. **Fights: `--hold N`** (an arrival counts only if it survives N more frames) — see tools/drive/README.md.
- `-j N` = N independent searches (N sessions), the shortest path applied.

## Fortune Summoners facts (this workbench)

- **How the game plays and the whole route**: `docs/game/` — `mechanics.md` (controls, magic, party, menus),
  `walkthrough.md` (Day 1 → ending, numbered steps), `bosses.md`, `side-content.md`, `exercise-checklist.md`. Its facts are
  from guides (tagged); confirm in state before relying on one, and report corrections.
- Named state (`tools/drive/vars.json`): `world` (0 until the first room exists), `room` (packed room id: 210110 town
  arrival, 210120 house front, 210140 house interior, 210141 bedroom), `lx ly` (leader position ×100; +240/frame walking),
  `script` (1 = a script owns the game), `dlg` (an open dialogue box), `control` (free roam, no box). Name more as you find
  them (`rd ram R define`), report the useful ones.
- Title: Start / Continue / Bonus Menu / Options / Exit, cursor on Start (`tcur`, `tcount == 5` = the title menu is up;
  later visits remember the last pick; the studio intro replays on every return). `ent` picks.
- **Menus drop input pressed within ~15 frames of opening**: wait ~30 frames after anything opens, ~12 between presses.
  On option pages `C` saves and exits WITHOUT a prompt (Graphics has no Save row). The title cursor glow pulses: a
  picture may show no highlight — read `tcur`. New game: a config box (`Game
  Difficulty`, `Auto-guard`, `Start Game`): `D D ent`. `ent` advances dialogue (a press during typing completes the line).
- In the first house (210140): X talks (Enter does not), C jumps, ↑ enters doors; the front door refuses until the errands.
- Movement [RD field-movement]: walk 240 lx/frame after ~10 frames; dash (`R . R*20`, short first press) 480; slide = ↓
  while dashing; short jump `C*2` ≈ 4800 up, long `C*12` ≈ 8400. Field exits are ↑ markers at the room edge (not
  walk-offs) and refuse while an enemy is attacking; town → field: ↑ at the tower right of the boutique (210110 → 225110).
  X examines unless the sword is drawn (then it slashes); V always examines. In a shop menu `ent` BUYS — mind `gold`.
- **Room exits are event zones** [RD story-barness-caves; `zone_pass` 0x59a1f0]: a zone fires only after 30 ticks with no
  hostile within 56000 × 20000 of the leader (`zrest` counts down 30 → 0; HUD "ESCAPE" icon = a held zone). Kind 1 fires on
  walking in, kinds 2–3 on ↑ while standing idle on the ground. Kill or outrun nearby enemies first; red screen-edge markers
  point at off-screen enemies, not doors. Doors of a room: zone actors `a = s32(world+0x11e0+4k)` with `a+0x274` ≠ 0.
- Push blocks / crates only with the sword SHEATHED (Z); pushing = `lpose` 114, 100 lx/frame.
- Game over: "Return To Safety" → the `retry` room, HP full, money −10 % (truncated).
- The game saves at goddess statues (slot saves `user\savedataNN.sdt`). `rd endsave R` files the run's saves:
  `rd new NEXT --from save:DIGEST` continues from them (Continue on the title). Chain by saves: replays start at boot.
- `--from fresh` = a new install (no saves, default config).

## What not to do

- Do not read other runs' movies or `tests/scenarios/*/trace.jsonl` for the inputs of your task: the trace must be your
  own. Reading `docs/game/`, findings and the decompile is the point.
- Do not kill processes by name; `rd stop R` ends your session. Do not touch `C:\orc-rhost` / other projects' runners.
- Do not commit; report instead. A run that was poked (`w`) is flagged POKED and cannot be filed.
