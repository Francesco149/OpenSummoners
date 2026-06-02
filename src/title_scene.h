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
 *   - the joystick lazy-attach block (0x5ba120) on first confirm;
 *   - the local_50 watchdog (forces phase 10 after 0x1194 frames).
 */
#ifndef OPENSUMMONERS_TITLE_SCENE_H
#define OPENSUMMONERS_TITLE_SCENE_H

#include <stdint.h>

#include "menu_list.h"      /* menu_ctrl / menu_node — the spawned controller */
#include "obj_container.h"  /* sel_list — the owning menu-tree entry list     */
#include "input.h"          /* input_mgr — the per-frame menu input poll      */

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

/* ─── the per-frame menu input dispatch (default arm, after the spawn) ──
 *
 * Ported from FUN_0056aea0's per-frame menu update at 0x56b807..0x56ba39 —
 * the last piece of the title scene's *update* half.  Each menu frame, once
 * the menu has been spawned, this:
 *
 *   1. polls the five menu buttons through the (already-ported) ring poll
 *      input_poll_consume and feeds each hit into the (already-ported) action
 *      latch menu_list_latch, in retail order:
 *        button 2  → latch dir 2   button 4  → latch dir 3
 *        button 1  → latch dir 0   button 3  → latch dir 1
 *        button 0x24 → latch dir 9
 *      with two axis-held syntheses interleaved (only when nothing has
 *      latched yet): after buttons 2/4/1, latch dir 6 (mgr->axis_held[0] set)
 *      or 7 (clear); after button 3, latch dir 4 (mgr->axis_held[1] set) or 5
 *      (clear).  The latch's return code (`esi` / retail iVar14) is the
 *      resolved menu action.
 *
 *   2. dispatches on that action code:
 *        1, 2  → a "move" SFX (id 9);
 *        3     → the *commit*: if the selected row is enabled (row.flag8 != 0)
 *                play the confirm SFX (id 5) and proceed; if disabled, play the
 *                denied SFX (id 6) and do nothing else this frame;
 *        4     → a "cancel" SFX (id 7)  [unreachable in the title flow — it
 *                arises from latch dir 10, which the title never sends];
 *        else  → nothing.
 *
 *      NB the action↔meaning mapping is the latch/nav return code, NOT the
 *      button: nav returns 3 for *cancel* (its dir 9) — so the title menu's
 *      "commit" is physically button 0x24.  (The earlier findings note that
 *      called 0x24 "back/cancel" was reading the dir, not the outcome.)
 *
 *   3. on a successful commit (action 3, enabled row): runs the joystick
 *      lazy-attach (the &DAT_008a93dc DInput block), then — when the selected
 *      row's action id is not 0x1d — walks the god object's menu-action table
 *      for an entry whose key matches that action id and notifies it
 *      (0x41bb80(0x5e, entry.arg)); finally latches the result code (the
 *      selected action id) and requests the phase-10 fade-out.
 *
 *   4. unconditionally runs the idle watchdog: if the menu has been up for
 *      >= 0x1194 frames it fires 0x40a5d0(0,0,0,0,1) and forces phase 10.
 *
 * The three still-unported side effects — SFX (0x411390), the joystick
 * attach (0x5ba120/0x5ba290), and the save-data notify (0x41bb80) — plus
 * the watchdog (0x40a5d0) are routed through the observable hooks below
 * (the menu_cell_layout_hook pattern), each a no-op by default, so this
 * assembles and tests now without pulling in audio / DInput / the god object.
 * The save-data *lookup* itself is ported faithfully against a caller-supplied
 * model of the god object's table slice.
 *
 * NOT included here (a separate point in the update half, before the phase
 * switch): the early `0x22`→return-state-6 abort poll at 0x56b14d.  The
 * enclosing scene loop will do that as a plain
 * `if (input_poll_consume(mgr, now, 0x22)) return 6;` before this step. */

/* SFX trigger — retail 0x411390(sfx_id, 0, 0) on the global audio object
 * DAT_008a6b60.  Ids used here: 9 (move), 5 (confirm), 6 (denied), 7 (cancel). */
typedef void (*title_menu_sfx_fn)(int32_t sfx_id);
extern title_menu_sfx_fn title_menu_sfx_hook;

/* Joystick lazy-attach — retail's &DAT_008a93dc 2-slot DInput attach loop
 * (0x5ba120 / input_dev 0x5ba290).  Fired once per commit. */
typedef void (*title_menu_joystick_fn)(void);
extern title_menu_joystick_fn title_menu_joystick_hook;

/* Save-data notify — retail 0x41bb80(0x5e, entry.arg) for the matched
 * menu-action table entry.  `arg` is that entry's +0x00 payload pointer. */
typedef void (*title_menu_savedata_fn)(void *arg);
extern title_menu_savedata_fn title_menu_savedata_hook;

/* Idle watchdog action — retail 0x40a5d0(0,0,0,0,1), fired when the menu
 * times out (>= 0x1194 frames). */
typedef void (*title_menu_watchdog_fn)(void);
extern title_menu_watchdog_fn title_menu_watchdog_hook;

/* One entry of the god object's menu-action table (retail: 8-byte records at
 * `*DAT_008a6e80`->[0xa48]+0x10, count at +0x16).  The commit path searches it
 * for `key == selected action id` and notifies the matched `arg`. */
typedef struct title_menu_savedata_entry {
    void    *arg;   /* +0x00 — payload passed to the notify hook */
    int32_t  key;   /* +0x04 — compared against the selected row's action id */
} title_menu_savedata_entry;   /* 8 B */

/* The table slice the commit path walks.  Pass entries=NULL (or count=0) to
 * model retail's "no table / empty" guard (base or count zero → skip). */
typedef struct title_menu_savedata_list {
    title_menu_savedata_entry *entries;
    uint16_t                   count;
} title_menu_savedata_list;

/* What the enclosing loop must do after one menu-input frame. */
typedef struct title_menu_input_out {
    int32_t action;        /* the latch result code (0 none, 1/2 nav, 3 commit, 4 cancel) */
    int     enter_phase10; /* 1 → caller sets phase=10, fade=1000 (commit or watchdog)    */
    int     set_result;    /* 1 → caller latches result_code into the scene return (local_48) */
    int32_t result_code;   /* the committed row's action id; valid only when set_result    */
} title_menu_input_out;

/* Run one frame of the spawned title menu's input handling.
 *
 *   mgr             the scene's input manager (retail in_ECX[1]) — polled, and
 *                   its axis_held[0]/[1] flags consulted for the synth events.
 *   ctrl            the spawned menu controller (title_menu.ctrl / local_60).
 *   now             this frame's GetTickCount sample (retail DVar8), passed to
 *                   the poll + latch for the consume window / key-repeat clocks.
 *   watchdog_frames the menu's idle-frame counter (retail local_50).
 *   savedata        the god object's menu-action table slice, or NULL to skip
 *                   the post-commit notify lookup.
 *   out             filled with the resulting state-transition request.
 *
 * Faithful to 0x56b807..0x56ba39.  Side effects (SFX / joystick / notify /
 * watchdog) go through the hooks above.  Touches the controller's list state
 * (via the latch) and the matched record only through the notify hook. */
void title_menu_input_step(input_mgr *mgr, menu_ctrl *ctrl, uint32_t now,
                           int32_t watchdog_frames,
                           const title_menu_savedata_list *savedata,
                           title_menu_input_out *out);

/* ─── the render half (the pacer's `sub==1` arm) ─────────────────────
 *
 * Ported from FUN_0056aea0's render branch at 0x56bb04..0x56bf1a — the
 * path the frame pacer dispatches to on a TITLE_PACE_RENDER frame, and
 * the last piece of the title scene.  It draws one frame for the current
 * scene phase and presents it.
 *
 * Control flow (recovered from the raw disasm — Ghidra mis-rendered the
 * jump-table jmp at 0x56bb55 as a call+return; in reality every per-phase
 * handler `jmp`s to the shared frame-end at 0x56bec4):
 *
 *   1. prologue (0x56bb04): phase 0 → a surface reset (0x5b9410); phases
 *      2..3 → a surface clear (0x5b9b70); phase > 10 → skip straight to
 *      the frame-end (nothing to draw);
 *   2. dispatch (0x56bb55): `jmp [phase*4 + 0x56bfa4]`, the 11-entry table
 *      recovered with radare2 → 7 inline handlers:
 *        phase 0,1,2 → studio-logo handler   (0x56bb5c, sprite field +4)
 *        phase 3,4   → title-logo handler    (0x56bbd4, sprite field +8)
 *        phase 5     → "press button" fade-in (0x56bc4d, assets 2,3)
 *        phase 6     → "press button" hold    (0x56bca2, assets 3,4)
 *        phase 7     → sparkle flourish       (0x56bcf7, asset 4 + trail of 5s)
 *        phase 8,9   → top-level menu         (0x56bdb9, assets 5,6 + cursor)
 *        phase 10    → menu fade-out          (0x56be85, reset + asset 6)
 *   3. frame-end (0x56bec4): compose (0x56c180), log "Title Menu -
 *      Flipping" once (gated on the already-flipped flag and the quiet
 *      flag DAT_008a6b54), then Flip (0x5b8fc0) — the documented "title
 *      menu drew a frame" event.
 *
 * Every leaf the handlers call is an unported DDraw / asset-model /
 * object-model bridge (0x5b9410, 0x5b9b70, 0x494e10, 0x418470, 0x56c610,
 * 0x56c4e0, 0x56c580, 0x56c470, 0x56c180, 0x5b8fc0).  Rather than a
 * function pointer per bridge, they are reported as an ordered stream of
 * tagged draw commands through the single sink hook below (no-op by
 * default).  The render half's *purpose* is exactly that ordered draw
 * stream, so the sink is its testable core: the dispatch decision, the
 * per-handler draw sequence, the fade→alpha ramp, the sparkle-trail
 * geometry, and the selected-row cursor placement are all observable in
 * it without pulling in any of the still-black-box draw subsystems. */

/* The kind of draw bridge a command stands for (the retail call it maps
 * to is in the trailing comment). */
typedef enum title_draw_op {
    TITLE_DRAW_SURFACE_RESET = 1, /* 0x5b9410(surface)                          */
    TITLE_DRAW_SURFACE_CLEAR,     /* 0x5b9b70(surface,0,0)                      */
    TITLE_DRAW_LOGO,              /* 0x494e10 — studio/title logo alpha blit    */
    TITLE_DRAW_SPRITE,           /* 0x56c610(obj,surface,asset,0,0) — plain    */
    TITLE_DRAW_SPRITE_LEVEL,     /* 0x56c4e0(obj,surface,asset,level,1000,0,0) */
    TITLE_DRAW_SPARKLE,          /* 0x56c580 — one sparkle of the phase-7 trail */
    TITLE_DRAW_MENU_CURSOR,      /* 0x56c470 — the selected row's highlight     */
    TITLE_DRAW_FRAME_END,        /* 0x56c180(surface) — compose the frame       */
    TITLE_DRAW_LOG_FLIPPING,     /* the "Title Menu - Flipping" log marker       */
    TITLE_DRAW_FLIP,             /* 0x5b8fc0(hWnd) — present (the DDraw Flip)    */
} title_draw_op;

/* One draw command.  Only the fields meaningful for `op` are set; the
 * rest are 0.
 *
 *   asset  TITLE_DRAW_LOGO        → the logo sprite field offset (4 studio,
 *                                   8 title) distinguishing the two logos;
 *          TITLE_DRAW_SURFACE_CLEAR → the source frame index blitted onto the
 *                                   primary: 0 (phase-2..3 background frame[0])
 *                                   or 1/2 (the logo handler's alpha-0 path,
 *                                   frames[1] studio / frames[2] title);
 *          TITLE_DRAW_SPRITE[_LEVEL]/SPARKLE → the 0x418470 asset id;
 *          TITLE_DRAW_MENU_CURSOR → the selected row index (the cursor).
 *   level  TITLE_DRAW_SPRITE_LEVEL → the raw fade level passed (0x56c4e0's
 *                                    4th arg, divisor 1000 is its 5th);
 *          TITLE_DRAW_MENU_CURSOR  → the constant 0x4b0 (1200).
 *   alpha  TITLE_DRAW_LOGO / SPARKLE → the ramp-resolved blend value
 *                                      (title_fade_ramp); 0 ⇒ the clear path.
 *   x      TITLE_DRAW_SPARKLE      → the sparkle's x (192,196,…<416).
 *   y      TITLE_DRAW_MENU_CURSOR  → the row's y (16 + cursor*32).            */
typedef struct title_draw_cmd {
    title_draw_op op;
    int32_t       asset;
    int32_t       level;
    int32_t       alpha;
    int32_t       x;
    int32_t       y;
} title_draw_cmd;

/* The draw sink — receives every command in retail draw order.  NULL by
 * default (all bridges become no-ops, exactly as a headless run that owns
 * no surfaces behaves). */
typedef void (*title_render_sink_fn)(const title_draw_cmd *cmd);
extern title_render_sink_fn title_render_sink_hook;

/* Port of 0x448c80, the fade→alpha ramp the render half blends with.
 *
 *   idx = (value * 20) / divisor       (signed truncating division)
 *   return  (0 <= idx < 20) ? ramp[idx] : 0
 *
 * `ramp` is the engine's 20-dword palette/blend table at 0x8a9308 — which
 * the *static* image leaves all-zero (DDraw/asset init fills it at run
 * time).  Pass NULL (or the all-zero static table) to model an unfilled
 * ramp: every lookup yields 0, so the logo handlers take their surface-
 * clear branch and sparkles draw at alpha 0 — the faithful pre-init
 * behaviour.  Note divisor<=0 returns 0 (retail's `jle` guard at
 * 0x448c86) and idx==20 (value==divisor) also returns 0 (the `>= 0x14`
 * cap), so a fully-saturated fade ramps to 0, not the top entry. */
int32_t title_fade_ramp(int32_t value, int32_t divisor, const uint32_t *ramp);

/* Run one render frame for the current scene phase.
 *
 *   phase           the scene phase (local_64 / title_fade_state.phase).
 *   fade            the main fade ramp (uVar15 / title_fade_state.fade).
 *   ctrl            the spawned menu controller (for the phase 8/9 cursor
 *                   highlight); NULL ⇒ skip the cursor (retail's
 *                   `[esp+0x18]` null check at 0x56be20).
 *   ramp            the 0x8a9308 alpha table (see title_fade_ramp); NULL ⇒
 *                   all-zero.
 *   quiet           the DAT_008a6b54 log-suppress flag (nonzero ⇒ no log).
 *   already_flipped in/out: the bVar3 "have we flipped before" latch — the
 *                   "Flipping" log fires only on the first flip while
 *                   `quiet` is clear.  Set to 1 on return.
 *
 * Emits the frame's draw commands through title_render_sink_hook in exact
 * retail order, ending with TITLE_DRAW_FRAME_END, the optional
 * TITLE_DRAW_LOG_FLIPPING, then TITLE_DRAW_FLIP.  Faithful to
 * 0x56bb04..0x56bf1a. */
void title_render_step(int32_t phase, int32_t fade, menu_ctrl *ctrl,
                       const uint32_t *ramp, int quiet, int *already_flipped);

/* ─── the scene runner (the outer do/while of FUN_0056aea0) ───────────
 *
 * The capstone that composes the ported units into the one running title
 * scene.  Ported from FUN_0056aea0's outer `do { … } while(1)` body — one
 * call to title_scene_step is one iteration of that loop:
 *
 *   1. sample GetTickCount once (the caller passes it as `now`, 0x56b002);
 *   2. title_pace_step decides pump + update/render for this iteration,
 *      and the pump (0x5b1030) fires through a hook when requested;
 *   3. on a TITLE_PACE_RENDER iteration → title_render_step draws the
 *      current phase's frame and presents it (the whole render half);
 *   4. on a TITLE_PACE_UPDATE iteration → the update half:
 *        - the pre-update side effects (0x43e140 + 0x40fe00 + 0x566250(0));
 *        - the 0x22 abort poll (0x56b14d): a hit returns scene result 6;
 *        - the phase switch = title_fade_step (its set_next_segment /
 *          spawn_sparkle effects routed through hooks); on a TITLE_FADE_MENU
 *          frame, title_menu_spawn on first entry then title_menu_input_step
 *          (whose SFX/joystick/notify/watchdog go through the existing
 *          title_menu_*_hook globals); on a phase-10 frame, title_menu_teardown
 *          precedes the fade-out, and TITLE_FADE_EXIT returns the latched result;
 *        - the per-frame tail (LAB_0056ba69): the idle-watchdog counter
 *          increment, the post-update side effect (0x56c930), and the
 *          per-owner-entry update (0x43c2e0, once per owner->count).
 *
 * The scene returns (TITLE_SCENE_DONE) only out of the update half — via the
 * abort poll (result 6) or the phase-10 fade-out completing (result = the
 * committed menu action, or 0 on a watchdog timeout).  The render half never
 * returns; it loops.  (Ghidra rendered the 0x56bb55 jump table as a `call`
 * with a `return`, hiding this — see title-scene.md / engine-quirks #40.)
 *
 * Faithful divergences (documented so the seams stay visible):
 *
 *   - The **skip-splash early-out** (0x56b0e8..0x56b150 — "press any button
 *     during the intro to jump straight to the menu") is now ported (it sits
 *     just after the 0x22 abort poll).  It walks the input-manager ring
 *     (input_any_fresh_press, newest-first) and, on a hit, zeros the fade,
 *     fires the BGM SetNextSegment cue when still before phase 3, flushes the
 *     ring + axis state (input_mgr_reset), and forces phase 8.  The gate
 *     honours `skip_intro` (param_1): with it clear (the headless default) a
 *     press is ignored at phase 0, so a param_1==0 boot still plays the intro
 *     from the start, but a press during phases 1..7 skips the rest.  The one
 *     piece left out is retail's scene-local sparkle-counter reset (var_3eh_2),
 *     which belongs to the deferred sparkle-trail subsystem, not the runner.
 *   - The menu-spawn precondition (a free, allocated owner entry) is the same
 *     as retail's (which has no guard).  Where retail would dereference a NULL
 *     controller after a failed spawn, this guards `ctrl != NULL` before the
 *     input step — a no-op divergence on the guaranteed-valid title flow, but
 *     it keeps a degenerate headless owner from crashing the runner.
 *   - Scene entry/exit engine plumbing (the operator_new allocations, the
 *     0x40a5d0 watchdog reset, 0x56bfd0/0x562a70/0x40fe00 init, and the
 *     0x5aff00/0x56c2b0 exit cleanup) is the caller's job — the runner owns
 *     only the per-frame loop body.  title_scene_init just zeroes the FSM
 *     state and binds the environment.
 */

/* The still-unported per-frame engine calls in the outer loop, routed through
 * one struct so the runner composes the ported units without pulling in the
 * unported subsystems.  Every field is nullable (NULL ⇒ that call is a no-op);
 * a NULL `hooks` argument makes all of them no-ops. */
typedef struct title_scene_hooks {
    void (*pump)(void);                       /* 0x5b1030 — message-pump frame   */
    void (*pre_update)(void);                 /* 0x43e140 + 0x40fe00 + 0x566250(0)*/
    void (*post_update)(void);                /* 0x56c930                         */
    void (*update_entry)(int32_t idx);        /* 0x43c2e0, once per owner entry   */
    void (*set_next_segment)(void);           /* BGM cue (title_fade_step fx)     */
    void (*spawn_sparkle)(int32_t intensity); /* 0x56c070 (title_fade_step fx)    */
} title_scene_hooks;

/* The full per-frame state of a running title scene — the FSM units plus the
 * scene-level locals (watchdog counter local_50, return code local_48, the
 * first-flip latch bVar3) and the bound environment (owner sel_list *in_ECX,
 * input manager in_ECX[1], the saved menu selection key, the alpha ramp, the
 * log-quiet flag, the save-data table slice). */
typedef struct title_scene {
    title_pace_state pace;
    title_fade_state fade;
    title_menu       menu;            /* node + ctrl (spawned on first menu frame) */
    int32_t          watchdog;        /* local_50 — idle-frame counter (caps 0x1194)*/
    int32_t          result;          /* local_48 — the scene's return code         */
    int              already_flipped; /* bVar3 — first-flip "Flipping" log latch     */

    sel_list        *owner;           /* *in_ECX — owning menu-tree entry list       */
    input_mgr       *input;           /* in_ECX[1] — per-frame input poll            */
    int32_t          select_key;      /* *(*DAT_008a6e80+0xa60) — saved menu pick     */
    const uint32_t  *ramp;            /* 0x8a9308 alpha ramp (NULL ⇒ all-zero)        */
    int              quiet;           /* DAT_008a6b54 — log-suppress flag             */
    const title_menu_savedata_list *savedata;  /* commit-path notify table, or NULL  */
    int              skip_intro;      /* param_1 — nonzero ⇒ a press skips the intro  */
                                      /*           even from phase 0 (0x56b10f gate)  */
} title_scene;

/* What one iteration of the runner resolved to. */
typedef enum title_scene_status {
    TITLE_SCENE_RUNNING = 0,   /* keep iterating */
    TITLE_SCENE_DONE    = 1,   /* scene finished — read ts->result for the code */
} title_scene_status;

/* Initialise a fresh title scene: zero the pacing/fade/menu state, the
 * watchdog, the result, and the flip latch, then bind the environment.  Mirrors
 * the entry zeroing at 0x56aec4..0x56aef1 / 0x56affc (the FSM locals only — the
 * engine-object allocations and the init-time engine calls are the caller's). */
void title_scene_init(title_scene *ts, sel_list *owner, input_mgr *input,
                      int32_t select_key, const uint32_t *ramp, int quiet,
                      const title_menu_savedata_list *savedata, int skip_intro);

/* Run one iteration of the outer loop.  `now` is this iteration's GetTickCount
 * sample (the engine reads it once at 0x56b002); `hooks` routes the unported
 * per-frame engine calls (NULL ⇒ all no-op).  Returns TITLE_SCENE_RUNNING to
 * keep going, or TITLE_SCENE_DONE when the scene has finished (ts->result holds
 * the menu-action / abort code the outer driver dispatches on).  Faithful to
 * the outer `do { … } while(1)` of FUN_0056aea0 (see the header note above for
 * the deferred skip-splash early-out). */
title_scene_status title_scene_step(title_scene *ts, uint32_t now,
                                    const title_scene_hooks *hooks);

#endif /* OPENSUMMONERS_TITLE_SCENE_H */
