# Plan — FAITHFUL controllable Arche (the real freeroam scene + live input)

> **Status (ckpt 130): Phase 2a — RENDER THE ROOMS — DONE.** The house (DATA 1023) + errands
> (DATA 1025) ROOM BACKDROPS now render: `game_world_room_render_cfg` drives the scene + parallax
> from the active room, the per-room `map_decode` tilesets are ported BIT-EXACT (validated vs a
> retail emit capture), `reload_room_backdrop` swaps the backdrop + camera on the cutscene room-key
> change, and the errands-render crash (an OOB frame read on an under-loaded tileset bank) is fixed
> (the `ar_sprite_slot_frame` f_38 bound).  **NEXT = Phase 2b: the FREEROAM HAND-OFF** — at the
> errands room stop the sequencer + run `character_step` on live `axis_held` (the `+0x200==0`
> char-AI path; the mover is DONE bit-exact).  Carried: load the under-loaded errands tileset banks
> (the render gaps, assetreg-clone-defer); the room CAST (NPCs).  See HANDOFF "ckpt 130".
>
> **(prior, ckpt 123): Phase 1 DONE, dialogue advance DONE, Phase 2 HARNESS-VERIFIED, the
> arrival→house DIALOGUE CHAIN PORTED + verified.**
> The MVP wire is removed; faithful LIVE input (`src/input_live.{c,h}`, the `0x46a880` producer),
> the town-arrival DIALOGUE ADVANCE, and now the **arrival→house multi-room CHAIN**
> (`src/cutscene.{c,h}`, the room-key swap, ckpt 123, commit `daa1f65`) are ported + verified — the
> live chain reaches the errands boundary `0x334dc`.  **The ckpt-122 harness verification (quirk
> #103): a 3-ROOM chain (arrival `0x334be` → house `0x334c8` → errands `0x334dc`, light room-key
> swaps, ONE `game_enter`); player control = `entity+0x200 == 0` (char-AI), NOT `+0x200=1`; the
> errands room IS the freeroam.**  Two findings reshaped the next move (ckpt 123): the errands room
> is a flag-gated QUESTLINE (`0x4dc510`), and NEITHER the house nor errands ROOM is rendered (the
> house lines play over the town backdrop).  **USER decision (ckpt 123): RENDER THE ROOMS FIRST —
> the faithful foundation for a non-fake freeroam.  See "Phase 2a — RENDER THE ROOMS" below.**
> The freeroam hand-off + Phase 3 (animation) follow.
>
> Orient: `CLAUDE.md` → `FRONT.md` → this file.  Sibling plans:
> `plans/movement-system.md` (the mover chips), `plans/dialogue-cutscene.md` (the cutscene
> coroutine), `plans/party-character-system.md` (the party band / leader render).

## Phase 2a — RENDER THE ROOMS (ckpt 123, USER-chosen — the ACTIVE CHIP)

The arrival→house dialogue CHAIN is ported (ckpt 123); the live chain reaches the errands
boundary `0x334dc`.  But neither the house (`0x334c8`) nor errands (`0x334dc`) ROOM is rendered —
the house lines play over the town backdrop — so the freeroam hand-off would put Arche in the
wrong scene (the ckpt-120 fake the USER removed).  **USER decision: render the rooms first** (the
faithful foundation; fixes the house backdrop AND unblocks a non-fake freeroam).

**The render path + resource scheme are ALREADY GENERAL (the big head start, scoped ckpt 123):**
- `load_town_scene(uint16_t scene)` (`main.c:2294`) — `FindResourceA(sotes.exe, scene, "DATA")` →
  `LockResource` → `town_render_load` → parse (`map_data.c`/`0x587970`) + decode (`map_decode.c`/
  `0x587e00`).  Per-room by design; the town hardcodes `load_town_scene(1022)`.
- The port has the FULL ROOM REGISTRY embedded (`world_tables_data.c`: 417 records @ `0x6940c8`,
  0x150 stride) + `game_world_find_room(w, room_key)` (mirrors retail's `0x585ae0` key lookup) +
  `game_map_room_key`.  **CONFIRMED:** ROOM record `+0x0c` dword = `GW_ROOM_SCENE` = the per-room
  DATA resource id (the town room — registry entry [61], id `0x334be`, area `0xd2` — is scene
  **1022**).  So `room_key → find_room → [GW_ROOM_SCENE] → load_town_scene(that)` is the whole
  resource lookup, on data the port ALREADY HAS.

**Tasks (in order):**
1. **The room→scene mapping is CONFIRMED (ckpt 123, registry dump):** house `0x334c8` = entry[62] →
   DATA scene **1023**, errands `0x334dc` = entry[64] → DATA scene **1025** (parent chain town
   `0x334be`[61]/scene 1022 → house → errands, all area `0xd2`).  So `load_town_scene(1023)` = the
   house map, `load_town_scene(1025)` = the errands map.  Both rooms exist with valid sequential
   ids — the arc premise holds.
2. **Drive `load_town_scene` from the active room** — replace the hardcoded `1022` with the active
   room's scene (via `game_map`/`game_world`); confirm the town still renders 1:1 (regression).
3. **Room-keyed RELOAD on the cutscene transition** — when `cutscene_step` commits a new room key
   (`cutscene_room_key` changes), `town_render_free` + `load_town_scene(new room's scene)` so the
   backdrop swaps.  (The cutscene already exposes `cutscene_room_key`.)
4. **Per-room CAMERA** — `MAP_RENDER_CAM_TOWN_3F2` is a town constant (PORT-DEBT
   ingame-camera-snap); each room needs its view origin (the spawn-snap).  Capture/RE per room.
5. **map_decode COVERAGE (the main RISK)** — `0x587e00`'s ported arms are the TOWN tileset; the
   house/errands INTERIORS likely use tile shapes/ids not yet implemented → expect to add decode
   arms (the framework is there; transcribe the needed arms from `587e00.c`).  Validate each room
   bit-exact vs a retail capture (the usual divergence loop).
6. (Deferred to Phase 2b) the room ACTORS/NPCs (house/errands have their own cast — the cutscene
   cast spawn is town-specific) + the errands QUESTLINE (`0x4dc510`) + the freeroam hand-off
   (`character_step` at the errands room) + DROP `+0x200=1` for the first freeroam.

**Verify:** each room renders bit-exact vs a seed-pinned retail capture at the matching room key
(reuse the `runs/control-path-gt` nav to reach each room in retail); push montages to the feed.

## Phase 2 VERIFIED (ckpt 122) — the live path is GROUND TRUTH; porting is unblocked

The ckpt-121 harness verification is DONE (`runs/control-path-gt`, **quirk #103**).  Seed-pinned
`--lockstep --no-turbo` retail, the proven ckpt-112 nav (Z-spam from `game_enter`) extended to flip
8392, under a per-Flip field spec reading `room_state = *(*(0x8a9b50)+0x2784)`.  **Result — both the
static RE and ckpt-112 were HALF-right:**

- **3-ROOM CHAIN, confirmed.**  The committed room key **`room_state+0x4024`** swaps
  **`0x334be` (arrival, flip 1430) → `0x334c8` (house, flip 3661) → `0x334dc` (errands, flip 4270)**,
  staged by `FUN_00401d40(key,…)` (fired @3659/@4268), committed by `FUN_00402030`.  The static
  room sequence is REAL.
- **But it's a LIGHT room-key swap, NOT a full reload.**  ONE `game_enter`; `room_state`, the leader
  slot `+0x200c` (`0xd1dcc58`), the entity (`+0x9f4`, code `0xc35a`), and `+0x158a4` hold CONSTANT
  across both swaps.  So ckpt-112's "no second reload / entities persist" was right; its "same scene"
  was wrong (the room key does change).
- **CONTROL IS `entity+0x200 == 0` (char-AI), NOT `+0x200=1`** (the ckpt-114 polarity open, RESOLVED).
  In the errands room a held-axis walk drove Arche bit-exact (held-RIGHT `wx 19200→73800` facing 1,
  held-LEFT →`14640` facing 3) **with `+0x200`==0 and `+0x158a4` non-null the whole time** — matching
  `0x46cd70`'s dispatch (`+0x200==0` → char AI `0x478ba0` reads the held axis).  The `0x41e070`/
  `0x4c6830` `+0x200=1` setters in `4d7d80:449` are a LATER/different control point (party / end-of-day
  sites B/C), NOT the entry to player movement.
- **The errands room `0x334dc` IS the freeroam** ("PLAYER!" marker + HUD on screen = ckpt-112's
  "PLAYER!@4500", just correctly located in the errands room).  USER-confirmed: "a house with mom and
  dad and you run some errands and there's short dialogue at the start."  Z-spam STALLS there because
  it's gameplay, not dialogue — the stall IS the control boundary.

**The port plan (the verified, no-longer-conditional version):**
1. **Chain the 3 room scripts in `cutscene.c`** (extend the 10-line arrival sequencer): on the arrival's
   completing beat, transition to room `0x334c8` (the house, 8 lines, text VAs 0x86d390..0x86d1dc), then
   to `0x334dc` (the errands scene + its short opening dialogue).  Port the room-key swap mechanism
   (`0x401d40` stage → `0x402030` commit → load the room's script + scene) — it is a LIGHT swap (no full
   teardown), so the port models it as a scene/script change within the live town, NOT a full map reload.
2. **At the errands room, run FREEROAM:** stop the cutscene sequencer and run `character_step` on the
   leader entity reading live `g_game_drive.input.axis_held` (the char-AI `+0x200==0` path) + the live
   keyboard producer (`input_live.c`, Phase 1 — already ported).  This is the faithful replacement for
   the ckpt-120 `CHAR_CONTROL_ARM_FRAMES` MVP arm.  The port does NOT need to model `+0x200` for the
   first freeroam — control = "the errands scene yields to the char AI."
3. **DROP** the "`+0x200=1` transfer" from the first-freeroam port path (it was a static-RE
   mis-attribution; it's a later party/end-of-day mechanic).

**Two refinements stay OPEN (don't block the port):** (a) the LATER `+0x200=1` transfer (after the
errands complete → town flag-0xd2 → Sana scene) — needs a walk-to-trigger nav, a separate capture; (b)
whether the char-AI is actively SUPPRESSED during the arrival/house cutscenes or merely un-fed (held-
input not tested there) — i.e. is the port's "switch to freeroam" a gate or just "stop scripting her".
Both refine the model; neither changes the target: controllable Arche = the errands room `0x334dc`, via
`character_step` on live input.  `character_step` is DONE (bit-exact); the render is the static
cutscene-cast slot (Arche slides, no walk-cycle — PORT-DEBT(cutscene-party-chars), Phase 3).

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
