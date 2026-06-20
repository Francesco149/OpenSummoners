# The freeroam DASH double-tap trigger (char-run-trigger retired) — ckpt 150

**Status: PORTED + host-verified end-to-end (1045 host pass, +10).**  A USER double-tap
of a direction (tap-tap-hold) now makes freeroam Arche DASH, driven from the live input
event ring — the last un-wired freeroam move.  The dash PHYSICS was already bit-exact
(chip 3b, ckpt 118); this chip resolves the `run` FLAG that feeds it, retiring
`PORT-DEBT(char-run-trigger)`.

## What was missing

`character_step(c, axis_held, jump_held, run)` takes a pre-resolved `run` boolean (the
AI's command `cmd[0] == 5/6`).  `freeroam_step` hardcoded `run = 0`, so the walk + jump
worked on live input but a double-tap never dashed.  The detection that PRODUCES `run` —
the char-AI `0x478ba0`'s double-tap scan over the discrete press ring — was unported.

## The retail logic (RE'd off the decompile — not curve-fit)

`0x478ba0` (the shared char-AI, 3514 bytes) rebuilds the command block `entity+0x14854`
every tick.  The relevant slice (the L/R dash, lines 152-246):

```c
DVar5 = GetTickCount();
// snapshot the PRIOR command block, then reset it this tick:
for (i=0..7) local_608[i] = *(this + 0x14854 + i*4);
*(this + 0x14854) = 0;  /* ... +0x14860 = 0; ... */

// LEFT (id 2, held +0x11c), W = *(*0x8a6e80 + 0xf8):
iVar9 = FUN_00479e70(DVar5, 0, W, W, 2, 2, 0);          // double-tap-LEFT?
if (bVar19 || left_held) {
  if (bVar18 || run_mode/*+0x510*/ != 2) {              // the shipped default branch
    if (local_608[0]==5 || iVar9) *(this+0x14854) = 5;  // dash-left: SUSTAIN or new TAP
    else if (local_608[0]==1 || bVar1) *(this+0x14854) = 1;   // walk-left
  } /* run_mode==2 = a separate hold-to-run scheme */
}
// RIGHT (id 4, held +0x120): the mirror, cmd 6 (dash) / 2 (walk).  Runs SECOND,
// writes the same slot -> wins when both are held.
```

- **`FUN_00479e70`** (the double-tap scan, 275 bytes) finds TWO DISTINCT pressed ring
  records of the direction within the window, using a 64-slot "used" bitmap so a single
  held press (one record) is never a double-tap.  Calls `FUN_00479960` (the windowed
  ring find, 165 bytes) twice; `param_7 == 0` so it does **not** consume the records
  (read-only — they age out of the window naturally).
- **The window** `*(*0x8a6e80 + 0xf8)` is a config field, not a static default (it lives
  in the DInput god-object built at engine init).  **Read LIVE from retail = 800 ms**
  (`runs/dash-window2`, the proven `*0x8a6e80` chain, hooked at the per-Flip `0x5b8fc0`
  at the title).  The same read confirmed `run_mode` `*(*0x8a6e80+0x510) == 0` (≠ 2), so
  the normal double-tap branch above is the active path — the one this ports.
- **Self-sustain + release:** the snapshot/reset means `local_608[0]==5` (the prior
  frame's command) keeps the dash alive while the direction stays held, even after the
  two taps age out of the window; releasing drops `left_held`, so the freshly-reset slot
  stays 0 (the dash ends).  A fresh single press after release is a walk, not a dash.

## The port

- **`input.{c,h}` — `input_dash_double_tap(m, now, dir_id, window)`** — the faithful
  reduction of `FUN_00479e70` + `FUN_00479960`'s scan for the dash invocation
  (`param_2=0, param_3=param_4=window, param_5=param_6=dir, param_7=0`).  Returns 1 iff
  two distinct ring records have `.id == dir_id`, `.flag == 1` (pressed), and
  `(now - .ts) <= window` (unsigned, GetTickCount-wrap safe).  `INPUT_RING_DIR_{LEFT=2,
  RIGHT=4,UP=3,DOWN=1}` name the ids (matching `input_live.c` KEYMAP + the AI).  The live
  producer already posts these on press edges, so no producer change was needed.
- **`character.{c,h}` — `character_resolve_run(c, m, now, axis_held, window)`** — the
  dash-resolution half of `0x478ba0`: snapshot `c->cmd_lr`, reset, detect L/R
  double-taps, resolve with the self-sustain, return `run = (cmd_lr == 5 || 6)`.  A new
  `int16_t cmd_lr` field holds the dash subset of `0x14854` ({0,5,6}); the walk command
  (1/2) + its press-window warmup stay `character_step`'s axis_held domain (the
  `prev==5/6` self-sustain reads identically whether or not 1/2 is tracked).
  `CHAR_DASH_WINDOW_MS = 800` carries the retail-read window with provenance.
- **`main.c` `freeroam_step`** — `run = character_resolve_run(&g_freeroam_char,
  &g_game_drive.input, GetTickCount(), axis, CHAR_DASH_WINDOW_MS)`, fed straight into
  `character_step`.  The ring records its timestamps in `GetTickCount()` ms, so the
  window compare uses the same clock retail's AI does.

## Verification (host, deterministic — the divergence loop's unit end)

10 new tests (all pass):
- `input_dtap_*` (6): two presses in-window → 1; a single held press → 0 (the key
  property — the "used" mask blocks slot reuse); out-of-window → 0; the `<=` window
  boundary is inclusive; release events (flag 0) ignored; other-direction + future-ts
  stale slots rejected; read-only (no consume).
- `character_resolve_run_*` (3) + `character_dash_via_double_tap` (1, END-TO-END): a live
  double-tap-RIGHT-hold driven through `character_resolve_run` → `character_step` reaches
  the RUN cap `48000`; a single held press caps at the WALK cap `24000` and never runs;
  the self-sustain holds the dash after the taps age out, release ends it, both-held lets
  RIGHT override a LEFT dash; NULL guards.

The end-to-end test exercises the SAME two functions `freeroam_step` calls, so it proves
the whole input-ring → run-flag → bit-exact-physics chain, not just the detector.

## Determinism note

The double-tap is inherently a real-time gesture: retail keys it on `GetTickCount()` ms
(the ring ts + the window), and the port matches that exactly.  Neither side is
"deterministic" in window terms — under turbo/uncapped replay the frames fly past so two
presses a few trace-ticks apart land within the 800 ms window (dash fires); at 60 FPS the
gap is real ms.  This is faithful (both sides wall-clock); captures/parity stay on the
seed-pinned replay path, and the unit tests pin the logic with controlled timestamps.

## Residual / debts (NOT this chip)

- `run_mode == 2` (the alternate hold-a-button-to-run control scheme) is unported — inert
  while the shipped config is `run_mode == 0`; folds into `PORT-DEBT(keybind-config)`.
- The walk command latch (`0x14854 == 1/2`) + its press-window auto-repeat (`bVar1`, the
  input-mgr +0x130/+0x148/+0x15c timestamps) stay `PORT-DEBT(char-input-autorepeat)` —
  `character_step`'s own warmup covers the walk; only the DASH needed the ring.
- The walk-cel cadence is still time-based (`PORT-DEBT(char-walk-anim-distance)`); a
  dash should advance the run cels faster — RE that when the distance-locked cel law lands.

## USER-VERIFY (deferred — next session)

The dash is host-proven; a live/visual confirm is deferred.  To see it on-screen: a port
replay that reaches freeroam then double-taps + holds a direction (`--osr-emit`), scrubbed
in the studio — Arche should accelerate to ~2× the walk speed.  (A demo `.osr` + the
studio shortcut is the follow-up artifact.)
