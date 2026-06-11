# Plan — FAITHFUL controllable Arche (the real freeroam scene + live input)

> **Status (ckpt 121): Phase 1 DONE, the dialogue advance DONE, Phase 2 RESHAPED + ACTIVE.**
> The MVP wire is removed; faithful LIVE input (`src/input_live.{c,h}`, the `0x46a880` producer)
> + the town-arrival DIALOGUE ADVANCE (`src/cutscene.{c,h}`, the 10-line Z-advanced family
> conversation) are ported + verified.  **Phase 2 (the control hand-off) turned out ~5× bigger
> than planned — it is a MULTI-ROOM CHAIN, not a few lines — and the USER COMMITTED to the full
> faithful path (ckpt 121).  The next move is HARNESS VERIFICATION (resolve the static-vs-live
> conflict) BEFORE porting** — see "Phase 2 RESHAPED" below.  Phase 3 (animation) stays deferred.
>
> Orient: `CLAUDE.md` → `FRONT.md` → this file.  Sibling plans:
> `plans/movement-system.md` (the mover chips), `plans/dialogue-cutscene.md` (the cutscene
> coroutine), `plans/party-character-system.md` (the party band / leader render).

## Phase 2 RESHAPED (ckpt 121) — the control hand-off is a MULTI-ROOM CHAIN; harness-verify first

The RE (two general-purpose subagent maps of `0x4d7d80` / `0x401d40` / `0x41e070`) found the
player-control hand-off is NOT a few lines past the town-arrival dialogue:

- **`FUN_00401d40(scene_id, p2, p3)` stages a ROOM TRANSITION** — it writes the next room-lookup
  key into the map object `+0x900/+0x904/+0x908` (committed to `+0x4024` by `FUN_00402030`, the
  `+0x4024 = room-key` the in-game-intro proof established); the script then `return 2` (yield +
  transition) and the engine reloads.  It is NOT "fall through to the next `case`".
- **The narrative spine (cross-room):** arrival `0x334be` flag-0 (10 lines, DONE) → load room
  **`0x334c8`** (the new house interior; 8 lines, text VAs 0x86d390..0x86d1dc) → load room
  **`0x334dc`** (morning errands — a SEPARATE dispatcher `FUN_004dc510`, NOT in `4d7d80.c`; this
  is where story-flag `0x5f76805` advances 0→0xd2) → back to **town `0x334be` flag-0xd2** → the
  Sana-walk-home scene (`4d7d80.c:295-481`) → **the control transfer**.
- **The transfer (`4d7d80.c:449-463`, the inlined `0x41e070`/`0x4c6830` idiom):**
  `piVar1 = FUN_00413b20(handle); FUN_004c63a0(piVar1,1)` (release from the cutscene script-band);
  guard `FUN_004cc250(1,piVar1)==0`; then **`*(entity+0x200) = 1`** (the master "player-controlled"
  flag) + `*(entity+0x158a4) = 0` (clear the AI-script), `FUN_0041e180(1)` (clear the cmd ring),
  `FUN_0041e280()` (re-bind DirectInput → `FUN_0054e5c0` to the entity), `FUN_0041dc90()` (recompute
  the party band).  Returns NOT 2 → no room reload; control stays in the town room.  Two LATER
  transfer sites (B `:719-733` flag 0x140 end-of-day; C `:882-896` resume==3 post-school) are not the
  first.  The canonical helpers `0x41e070`/`0x4c6830` do the same flip on the leader slot `+0x200c`.

**THE CONFLICT (resolve FIRST, don't guess — harness):** ckpt-112 observed retail reaching control
via Z-spam with **ONE `game_enter` and NO map reload** ("the inn interior is the same cutaway scene").
The static read implies room transitions (`0x334c8`/`0x334dc`).  Either those keys map to the SAME
town map (the transitions are camera-only scene changes), or the live path is shorter than the static
chain.  **DO FIRST (the next session's opening move):**
1. **Harness-verify the live room/control path.** A Frida field-spec reading, per Flip across a
   Z-spam from `game_enter` to the hand-off: the scene-controller room key `*(*(0x8a9b50+0x2790)…)`
   / the map object `+0x4024`; Arche's entity `+0x200` (the control flag) + `+0x158a4`; the story
   flag `0x5f76805` (via `FUN_0041e2f0`).  This tells us: how many rooms/`game_enter`s actually
   occur, whether the map reloads, and the exact Flip/tick the `+0x200` flips to 1.  (Reuse the
   `runs/freeroam-gt` / ckpt-112 nav; `tools/frida_capture.py --seed-pin --lockstep --no-turbo`.)
2. **THEN port** the actual (possibly-reduced) chain: the room-transition system (`0x401d40` →
   the map reload) IF the live path really reloads; the intervening room scripts (`0x334c8`
   house, the `0x334dc` errands in `FUN_004dc510`) IF on the path; the Sana scene; then the
   `+0x200=1` transfer + wire `character_step` at that transition.  If the live path is same-map
   camera-only, the port is much smaller (extend `cutscene.c` to chain the scene scripts in-place).

**The control MECHANISM is small + clear; reaching retail's exact LOCATION is the arc.**  The
`cutscene.c` sequencer (the 10-line advance) is the foundation to extend; `character_step` is DONE
(bit-exact); the render is the static cutscene-cast slot (Arche slides, no walk-cycle —
PORT-DEBT(cutscene-party-chars), the animation system is Phase 3, out of the un-MVP scope).

## Why (the pivot)

The mover itself is DONE and host-validated **bit-exact** — walk (chip 3a), run/dash
(3b), jump + variable-height + windup (3b), all in `src/character.{c,h}` with embedded
retail-byte tests.  The ckpt-120 MVP wire (`src/main.c`) proved the *seams* exist (Arche's
cutscene-cast EFFECT actor is drivable; `g_game_drive.input.axis_held` feeds
`character_step`; the render-state mirror works), but it is a THROWAWAY scaffold:
- it arms control at a **measured frame** (`CHAR_CONTROL_ARM_FRAMES`=200), not the real
  dialogue-driven hand-off — PORT-DEBT(char-control-trigger);
- it is driven only by the **`held_trace` replay** (the port has NO live keyboard producer
  — `WM_KEYDOWN` is a no-op), so movement can't be exercised interactively;
- it renders Arche via the **static cutscene-cast slot** (idle clip, single bank 0x8b) — no
  walk-cycle, no facing mirror — PORT-DEBT(cutscene-party-chars).

Building the animation system on that scaffold couples it to all three shortcuts.  Instead:
do the faithful base first, then animate on top.  **Phase 2 below REPLACES the MVP wire;
do not extend it.**  (The ckpt-120 commit stays in history as the seam-proving prototype.
It can be reverted for a clean slate — ask the USER; otherwise Phase 2 supersedes it.)

## What's already in hand (don't redo)

- **The mover** — `src/character.{c,h}`: `character_step(c, axis_held[4], jump_held, run)`,
  bit-exact walk/run/jump/windup.  Reads `axis_held[0..3]` (UP/DOWN/LEFT/RIGHT, aligns with
  the input-mgr array A) + `jump_held` (the C button level) + `run` (resolved cmd[0]==5/6).
- **The seams** (proven by the MVP): Arche is `g_effects` code `0xc35a` (cutscene-cast slot,
  bank 0x8b); `game_actor_update` is the sim-tick-gated band-update site; the render-state
  mirror (`world_x/world_y/facing`) drives the sprite.
- **The retail ground truth**: freeroam is REACHED (ckpt 112, `runs/freeroam-gt`); the
  control transfer flips `entity+0x200=1` (`0x41e070`/`0x4c6830`) at the dialogue's "PLAYER!"
  prompt; freeroam movement reads the held-axis array `input-mgr+0x114` (quirk #41/#101) +
  the event ring for jump/dash.  The input producer is `0x46a880`; the keybind config is
  `*0x8a6e80`.

## The three phases (recommended order)

### Phase 1 — FAITHFUL LIVE INPUT (the USER's emphasis: testable live)  ✅ DONE (ckpt 121, `src/input_live.{c,h}`)

Goal: real keyboard drives the port's input manager each frame, so any input-reading
subsystem (the mover, menus) works interactively — alongside the existing deterministic
`held_trace`/`input_trace` replay (which STAYS the capture/parity path; live keyboard is
wall-clock = non-deterministic, so it must NOT be the parity path).

- **The held-axis producer** = port `FUN_0046a880` (RE'd ckpt 113): each frame it fills
  `input_mgr.axis_held[0..3]` (+ action slots `[4..]`) from the DInput keyboard buffer via
  the leaf `0x5ba520` (= `keyboard_state[scancode] & 0x80`).  Port a LIVE source: read the
  real keyboard each frame (`GetKeyboardState`/`GetAsyncKeyState`, or track `WM_KEYDOWN`/
  `WM_KEYUP` in `wnd_proc.c` — currently a no-op) and fill `axis_held` exactly as the
  producer does.  Scancodes: arrows `0xc8/0xd0/0xcb/0xcd`; the action buttons + their slots
  come from the keybind config (below).
- **The event ring producer** — the discrete ring (`input_mgr+0xc`, `{id,ts,flag}`×64;
  read side = `src/input.{c,h}` `input_poll_consume`) is filled on key-DOWN edges.  Port a
  live producer that posts a ring event on each fresh press (jump = id 7, Z-advance = id
  `0x24`, attack, …), so the jump/dash/Z paths fire from real keys (the dash double-tap is
  detected over the ring by `0x479e70` — see Phase 2).
- **The keybind config `*0x8a6e80`** — maps scancodes → action ids + the held-array slots
  (`+0x558`=attack X `0x2d`, `+0x574`=jump C `0x2e`, `+0x510`=run mode, window `+0xf8`).
  Read it (or its defaults) to resolve keys → ids/slots faithfully rather than hardcoding.
- **Wire it into the live loop** (`main_loop_body`'s `g_game_active` branch + the title/
  newgame/prologue drives) so live keyboard fills `g_game_drive.input` (and the others)
  every frame, GATED so it does NOT run when a `--held-trace`/`--input-trace` is active
  (replay wins, determinism preserved).
- **Determinism note**: keep BOTH paths.  Captures/parity = replay (seed-pinned, lockstep).
  Live keyboard = interactive testing only.  Document which is which.
- **Validation**: run the port windowed (NOT `--hide-window`), walk Arche with the arrow
  keys, confirm she moves; the existing `character_step` host tests stay the bit-exact guard.

### Phase 2 — THE REAL CONTROL HAND-OFF (replace the MVP trigger)  ⚠ RESHAPED — see "Phase 2 RESHAPED (ckpt 121)" above; the dialogue chip 4 (the 10-line advance) is DONE (`src/cutscene.{c,h}`), the control transfer is the multi-room chain (harness-verify first)

Goal: Arche becomes controllable when the town-arrival cutscene actually completes — the
genuine freeroam scene — not at a measured frame.

- **Dialogue chip 4** (`plans/dialogue-cutscene.md`): the town script `0x4d7d80` case
  `0x334be` configures beats on the scene-controller; the beat-runner `0x439690` pumps them
  (dialogue line via Z `0x43b980`, camera-at-target, flags, timer).  Port the ~15-line script
  table (Father/Arche/Mother/Sana, text read from the exe by VA like line 1) + the beat-runner
  so the cutscene advances on Z through all beats.  The port already renders the bubble
  (ckpt 104) — extend it to the multi-line, Z-advanced, beat-driven cutscene.
- **The control transfer**: on the cutscene's completing beat, port the `entity+0x200=1`
  flip (`0x41e070`/`0x4c6830`) for Arche (the persistent leader `room_state+0x200c`, code
  `0xc35a`).  Then the band update (`0x46cd70` pass 1) dispatches her to the char AI
  `0x478ba0` + apply `0x442a70` — i.e. wire `character_step` at the REAL `entity+0x200`
  transition (REPLACING the `CHAR_CONTROL_ARM_FRAMES` MVP arm; delete that scaffold).
- **The dash trigger** (retires char-run-trigger): with the live ring (Phase 1), port the
  AI's double-tap detection `0x479e70`/`0x479960` (two same-direction ring events in the
  window `*0x8a6e80+0xf8`, self-sustain while held) so `run` derives from real input.
- **Validation**: drive the port through the cutscene (live Z or `--input-trace`) → control
  transfers at the real prompt → Arche walks/jumps/dashes via live keys.  A NEW trace-studio
  session for the freeroam scene (the USER's "house freeroam" directive).

### Phase 3 — THE ANIMATION / RENDER (build the animation system HERE, on the faithful base)

Goal: Arche faces + walk-animates correctly — the animation system the USER wants, on the
real party-band render (NOT the MVP static slot).

- **The party band `0x4997b0`** (`plans/party-character-system.md`): render Arche as the
  persistent multi-part leader, not the static cutscene-cast member.  Her body banks
  `0x8c`-`0x8e` (registered, unused) + the row-0 `0x8b`.
- **Directional + walk-cycle frames**: resolve the walk/idle/jump animation clips for bank
  0x8b and the facing mirror — the render's `facing==3` selects `frame_base + flip_table[0x8b]`
  (a pre-mirrored FRAME), so register the mirror-frame offset + the directional cels.  This
  retires PORT-DEBT(cutscene-party-chars) (the facing-flip + walk-cycle the MVP couldn't show).
- **Drive the anim from the mover state**: select the clip from `character`'s state
  (idle/walk/run/airborne) + facing, advancing the clip with motion.
- **Validation**: trace-studio / feed — Arche walks with the correct cycle + faces her
  travel direction, vs retail freeroam.

## Also retire (folds in along the way)

- **char-collision-mover** — wire the chip-2 `collision_move_vertical` (`0x54e990`) + the
  horizontal mover `0x54db10` so Arche stops at walls / lands on slopes / hits the town
  ceiling (the held-jump apex clamp) — her first LIVE collision caller.
- **char-walk-tuning** — read `in_ECX[0x565b/c/e]` off the live entity instead of the
  `#define`d constants (per-character tuning).
- **char-input-autorepeat** — the press→latch warmup from the real wall-clock auto-repeat
  (`0x479ca0`) once live input is in.

## Risks / notes

- **Determinism**: live keyboard is wall-clock.  Keep the replay path authoritative for
  captures/parity; live input is interactive-only and must be bypassable.
- **Scope**: Phase 2 (dialogue chip 4) is the biggest piece (cutscene-coroutine RE) — it may
  warrant its own sub-arc.  Phase 1 is the most self-contained + immediately useful.
- **Don't fork the mover**: `character_step` is the faithful reduction; Phases 2-3 feed it
  real inputs + render its state, they don't rewrite it.
