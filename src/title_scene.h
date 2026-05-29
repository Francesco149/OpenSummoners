/*
 * src/title_scene.h — title-menu scene runner port (FUN_0056aea0).
 *
 * FUN_0056aea0 (3441 bytes) is the engine's first perceptually-visible
 * scene: the studio-logo fade-in, the title-logo fade-in, the "press
 * button" prompt, the sparkle flourish, and finally the top-level menu
 * (New Game / Continue / Options / Exit).  The post-launch driver
 * FUN_00562ea0 calls it when DAT_008a6e6c == 0 and dispatches on its
 * return value (the menu-action state code: 6, 8, 0x1a, 0x1c..0x1e).
 *
 * The function is one big `do { … } while(true)` with two interleaved
 * state machines:
 *
 *   (A) a frame-pacing sub-state machine (`local_28`) coupled to the
 *       pump FUN_005b1030 — drives ~60 Hz cadence.  [NOT in this file
 *       yet — checkpoint 2.]
 *
 *   (B) an intro-phase / menu-fade state machine (`local_64`) — the
 *       fade ramps and hold timers that advance studio-logo → title-logo
 *       → press-button → sparkle → menu.  **This file ports (B).**
 *
 * Why (B) first: it is pure integer arithmetic over four state vars
 * (phase, fade, tick, menu_fade), it is the load-bearing "intro
 * animation" logic, and it has no Win32/DDraw/object-model surface — so
 * it ports and tests cleanly in isolation, exactly as app_pump's pump
 * FSM did.  The two genuine side effects the phase machine triggers (a
 * BGM "SetNextSegment" cue at the studio→title hand-off, and the
 * per-frame sparkle-particle spawn in phase 7) are reported back to the
 * caller through a per-frame effects descriptor (title_fade_step_out)
 * rather than called directly, keeping this translation unit free of
 * any dependency on still-unported engine helpers.
 *
 * Ground-truth: the jump table at 0x56bfa4 and the switch dispatch at
 * 0x56bf68 were recovered with radare2 (Ghidra mis-rendered the indirect
 * jump as a call+return).  Every fade constant / threshold below was
 * verified against the raw disassembly at 0x56b153..0x56b5c1 — see
 * docs/findings/title-scene.md and the per-case provenance in
 * title_scene.c.
 *
 * Deferred to later checkpoints (documented here so the seams are
 * visible):
 *   - the local_28 frame-pacing FSM + the FUN_005b1030 pump call sites;
 *   - the phase 8/9 menu-controller spawn (0x412c10) + the
 *     populate-5-slots loop + the input poll/latch (0x43c110 /
 *     0x43ce50) + the action switch (0x411390);
 *   - the render half (jump-table draw handlers) + the frame-end flip
 *     (0x56c180 + FUN_005b8fc0);
 *   - the `param_1` skip-intro path and the consume-on-read ring-buffer
 *     "press a button to skip the splash" early-out (local_64 < 8);
 *   - the joystick lazy-attach block (0x5ba120) on first confirm;
 *   - the local_50 watchdog (forces phase 10 after 0x1194 frames).
 */
#ifndef OPENSUMMONERS_TITLE_SCENE_H
#define OPENSUMMONERS_TITLE_SCENE_H

#include <stdint.h>

/* ─── intro-phase / menu-fade state (the `local_64` machine) ─────────
 *
 * Mirrors four of FUN_0056aea0's stack locals.  All four hold small
 * non-negative magnitudes (0..1000); the engine declares some `uint`
 * but compares them all as signed `(int)`, so we keep them int32_t and
 * the ports below match the signed comparisons byte-for-byte.
 *
 *   phase      local_64  scene phase 0..10 (see the table in title_scene.c)
 *   fade       uVar15    main fade ramp, 0..1000
 *   tick       local_68  per-phase hold-timer (frames within a phase)
 *   menu_fade  local_58  menu cross-fade ramp (phases 8/9), 0..1000
 *
 * A fresh title scene starts every field at 0 (the function zeroes them
 * at entry — 0x56aec4..0x56aef1).  title_fade_state_init does that.
 */
typedef struct title_fade_state {
    int32_t phase;
    int32_t fade;
    int32_t tick;
    int32_t menu_fade;
} title_fade_state;

/* ─── per-frame effects descriptor ───────────────────────────────────
 *
 * title_fade_step fills this with the side-effect requests the phase
 * machine made on this frame.  The caller (the future full
 * title_scene_run) turns these into the actual engine calls; the FSM
 * itself stays pure.
 *
 *   set_next_segment  1 when the studio→title hand-off fired the BGM
 *                     "SetNextSegment" cue this frame (retail: the
 *                     0x5bbc60/_90/_cd0/_20 + 0x5bcb80 block at
 *                     0x56b2f6, logged "Title Menu - SetNextSegment").
 *                     Fires exactly once, on the phase 2→3 transition.
 *   spawn_sparkle     1 when phase 7 spawned a title-screen sparkle
 *                     particle this frame (retail: 0x56c070).
 *   sparkle_intensity 0x56c070's 5th argument when spawn_sparkle==1:
 *                     (fade * 0xe0) / 900 + 0xc0, using the post-increment
 *                     fade value.  Undefined when spawn_sparkle==0.
 */
typedef struct title_fade_step_out {
    int set_next_segment;
    int spawn_sparkle;
    int sparkle_intensity;
} title_fade_step_out;

/* ─── the action a single step resolves to ───────────────────────────
 *
 * What the enclosing per-frame loop must do after the phase machine
 * has updated its state for this frame.
 *
 *   TITLE_FADE_CONTINUE  intro still animating (phases 0..7) or fading
 *                        out (phase 10 with fade still > 0) — keep
 *                        running frames.
 *   TITLE_FADE_MENU      phases 8/9 — the fade math is done for this
 *                        frame; the caller must now run the (deferred)
 *                        menu-controller + input update.
 *   TITLE_FADE_EXIT      phase 10 fade-out completed (fade reached 0) —
 *                        the scene is done; the caller tears down and
 *                        returns its result code.
 */
typedef enum title_fade_action {
    TITLE_FADE_CONTINUE = 0,
    TITLE_FADE_MENU     = 1,
    TITLE_FADE_EXIT     = 2,
} title_fade_action;

/* Zero every field — the entry state of a fresh title scene. */
void title_fade_state_init(title_fade_state *s);

/* Run one frame of the intro-phase / menu-fade machine.
 *
 * Advances *s by exactly one frame's worth of fade/tick arithmetic for
 * the current s->phase, fills *fx with the side effects requested this
 * frame (always cleared first), and returns what the caller must do
 * next.  Pure: touches only *s and *fx.
 *
 * Faithful to the `switch(local_64)` at 0x56b153 (cases 0..7, default
 * = 8/9, case 10).  Phase values outside 0..10 are treated as the
 * default (8/9) branch, matching the engine's `local_64 < 0xb` guard
 * upstream (anything ≥ 11 never reaches the switch in retail).
 */
title_fade_action title_fade_step(title_fade_state *s, title_fade_step_out *fx);

#endif /* OPENSUMMONERS_TITLE_SCENE_H */
