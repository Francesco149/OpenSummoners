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
 *       pump FUN_005b1030 — drives ~60 Hz cadence.  **This file now
 *       ports (A) too (checkpoint 2): see the title_pace_* API below.**
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

#include "menu_list.h"      /* menu_ctrl / menu_node — the spawned controller */
#include "obj_container.h"  /* sel_list — the owning menu-tree entry list     */

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

/* ─── (A) frame-pacing sub-state machine (the `local_28` machine) ─────
 *
 * The outer `do { … } while(1)` of FUN_0056aea0 is paced by a tiny
 * three-state machine (`local_28`, values 0/1/2) coupled to the message
 * pump FUN_005b1030 (ported as app_pump_frame).  It is a fixed-16 ms-
 * timestep accumulator: each loop iteration it either runs the *update*
 * half (input + the local_64 phase FSM — TITLE_PACE_UPDATE) or the
 * *render* half (jump-table draw + Flip — TITLE_PACE_RENDER), pumping
 * the OS queue at each transition into the update state.  It burns the
 * accumulated wall-clock budget in 16 ms slices on update frames, then
 * renders once the budget is spent and refills it on render frames from
 * the real elapsed time (capped at 100 ms so a stall can't stack up an
 * unbounded catch-up burst).
 *
 * Decoded byte-for-byte from the radare2 disasm at 0x56b002..0x56b0c8
 * (raw stack offsets, `e asm.sub.var=false`).  State (the trailing
 * comment is the retail local + its raw esp displacement at the loop
 * top, where esp is stable):
 *
 *   sub            the 0/1/2 sub-state itself        local_28  [esp+0x50]
 *   budget         frame-time budget in ms (init 17) local_30  [esp+0x48]
 *   tick_anchor    GetTickCount at the last pump (C) local_34  [esp+0x44]
 *   render_anchor  GetTickCount at last update (A)   local_2c  [esp+0x4c]
 *
 * ⚠ Two retail locals are deliberately OMITTED as vestigial — the whole
 *   `sub==1` arm of the post-transition block (asm 0x56b09d..0x56b0ba)
 *   is observably dead:
 *     - E `[esp+0x5c]`: a per-frame counter the block increments while
 *       <1000 ms have elapsed since D and resets otherwise — i.e. a
 *       consecutive-sub-second-frame tally (an FPS / uptime counter).
 *       A full-function disassembly scan finds it WRITTEN ONLY, never
 *       read; Ghidra's decompiler dead-store-eliminated it entirely.
 *     - D = `local_20` `[esp+0x58]`: the 1-second-window anchor E is
 *       measured against.  Ghidra keeps D, but its *only* read (the
 *       `1000 < now - D` test) gates a branch whose sole effect is
 *       updating D itself and E — both dead — so the entire block
 *       produces no change to any live state (sub-transitions, pump
 *       calls, or the render/update dispatch).
 *   Dropping them is behaviourally exact; see title_scene.c.  The
 *   `sub==2` arm of that post-block (A = now) IS load-bearing and is
 *   kept (the next update frame measures now − A against the budget).
 */

/* sub-state values — mirror local_28 exactly (0 = first frame, 1 =
 * render-ready, 2 = updating). */
enum { TITLE_PACE_SUB_INIT = 0, TITLE_PACE_SUB_RENDER = 1, TITLE_PACE_SUB_UPDATE = 2 };

typedef struct title_pace_state {
    int32_t  sub;            /* local_28 */
    uint32_t budget;         /* local_30 — ms; compared unsigned in retail */
    uint32_t tick_anchor;    /* local_34 (C) — GetTickCount at last pump  */
    uint32_t render_anchor;  /* local_2c (A) — GetTickCount at last update */
} title_pace_state;

/* What the enclosing loop must do this frame once pacing has resolved. */
typedef enum title_pace_action {
    TITLE_PACE_UPDATE = 0,   /* sub==2 → run input + the local_64 phase FSM */
    TITLE_PACE_RENDER = 1,   /* sub==1 → run the jump-table draw + Flip     */
} title_pace_action;

/* Per-step output: whether to pump and what half to run. */
typedef struct title_pace_step_out {
    int               pump;   /* 1 ⇒ caller must call app_pump_frame() this step */
    title_pace_action action; /* TITLE_PACE_UPDATE or TITLE_PACE_RENDER          */
} title_pace_step_out;

/* Entry state of the pacing machine: sub=0, budget=0x11, anchors=0.
 * (Retail also seeds the dead window anchor D from GetTickCount at
 * 0x56affc; irrelevant here since D is dropped.) */
void title_pace_state_init(title_pace_state *s);

/* Advance the pacing machine one loop iteration.
 *
 * `now` is this iteration's GetTickCount sample (the engine reads it
 * once at the top of the loop, 0x56b002).  Fills *out with the pump
 * request and the update/render decision for this frame.  Pure: touches
 * only *s and *out.  Faithful to 0x56b002..0x56b0c8. */
void title_pace_step(title_pace_state *s, uint32_t now, title_pace_step_out *out);

/* ─── the top-level menu spawn block (the `local_64`==8/9 default arm) ──
 *
 * The first time the title scene reaches the menu phase (local_60 == 0) it
 * builds the top-level menu in one shot.  Ported from the spawn block at
 * 0x56b5cd..0x56b807 — the last piece of FUN_0056aea0's *update* half.
 *
 * The block reveals how the menu objects nest.  The scene's `this` carries
 * an owning *sel_list* (FUN_0056aea0's `*in_ECX`) whose entries are 0x1b0
 * menu-tree nodes.  Spawning the menu:
 *
 *   1. configures the owner's next free entry as this menu's tree node,
 *      giving it a single child (menu_node_build, 0x40f3e0), bumps the
 *      owner count, and marks the new entry active (sel_list_mark_last);
 *   2. acquires the controller — which is that node's lone *child*: the
 *      node's child array (node+0x48/+0x4c/+0x4e) doubles as an obj_pool,
 *      and obj_pool_acquire (0x412c10, ECX = the node) hands out
 *      children[0].  menu_ctrl_build (0x40f5c0) then lays a 6×1 stride-6
 *      linear-wrap grid on its embedded controller;
 *   3. appends the five fixed rows — action ids 0x1a, 0x1c, 0x1e, 0x1d, 8
 *      (each writes field0=0 / action=id / flag8=1, bumps the header count,
 *      and finalizes the row, a no-op on the fresh NULL-pointer cells);
 *   4. seeks the cursor to the row whose field0==0 and action matches the
 *      saved selection key (retail `*(*DAT_008a6e80 + 0xa60)`, the god
 *      object's last-picked menu code — injected here as select_key) and
 *      pages it into view (menu_list_scroll_into_view).
 *
 * Both the controller (local_60) and the configured node (local_54) are
 * returned in *out for the caller to drive on later frames and tear down.
 *
 * Precondition (as in retail, which has no guards on either): the owner has
 * a free entry slot whose pointer is an allocated 0x1b0 node, and the node
 * yields its child (always true right after menu_node_build with one
 * child) — otherwise the retail code dereferences NULL exactly as a faithful
 * port would.  The title flow guarantees both. */
typedef struct title_menu {
    menu_node *node;   /* local_54 — the owner sel_list entry configured here */
    menu_ctrl *ctrl;   /* local_60 — the child controller built on it          */
} title_menu;

void title_menu_spawn(sel_list *owner, int32_t select_key, title_menu *out);

/* The phase-10 menu teardown (0x56ba2e): drop the controller/node handles
 * and clear the node's +0x50 "active" flag.  Frees nothing — the controller
 * is a pool slot and the node tree is owned by the sel_list, both released
 * elsewhere.  No-op when no menu is live (ctrl == NULL). */
void title_menu_teardown(title_menu *m);

#endif /* OPENSUMMONERS_TITLE_SCENE_H */
