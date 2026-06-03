/*
 * src/newgame_drive.h — the new-game config scene's drive (the caller side of
 * FUN_00564780 case 0x24 / its frame pump FUN_00565d10).
 *
 * newgame_scene.c is the Win32-free run-loop MODEL (the tooltip pick, the
 * pump-result→action dispatch, the value refill); newgame_menu.c is the bit-
 * exact grid builder.  What was still missing to make the scene RUN was the
 * *caller* — the plumbing FUN_00564780's frame pump owns: own the scene + its
 * input manager, ramp the input-ready gate open, poll the buttons each frame
 * and collapse them into the pump result the model dispatches on, and present
 * the rendered frame.  This module is that caller, factored exactly like
 * title_drive vs title_scene: the structural loop is pure C and host-tests; the
 * Win32 frame render (GDI glyph_grid_render onto the primary surface) + present
 * arrive through the cfg callbacks the real build (main.c) fills.
 *
 * Lifecycle (mirrors title_drive):
 *     newgame_drive_init(&d, &cfg);          // build scene, bind input ring
 *     while (running) {
 *         uint32_t now = GetTickCount();
 *         switch (newgame_drive_step(&d, now)) {
 *         case NEWGAME_START: ...begin game...; running = 0; break;
 *         case NEWGAME_BACK:  ...re-display title...; running = 0; break;
 *         default: break;                    // RUNNING / OPEN_PICKER: keep going
 *         }
 *     }
 *     newgame_drive_shutdown(&d);
 *
 * The input pump (newgame_drive_step's poll→latch→collapse) is the Win32-free
 * distillate of FUN_00565d10's per-frame button scan + FUN_0043bca0's id→latch
 * mapping (quirk #65):
 *
 *   button 1  (up)    → menu_list_latch(0)  → 1/2 → NEWGAME_PUMP_MOVE   (0xd)
 *   button 3  (down)  → menu_list_latch(1)  → 1/2 → NEWGAME_PUMP_MOVE   (0xd)
 *   button 2/4(page)  → menu_list_latch(2/3)→ 1/2 → NEWGAME_PUMP_MOVE   (0xd)
 *   button 0x24(confirm/OK) → menu_list_latch(9)  → 3 → NEWGAME_PUMP_CONFIRM(0xc)
 *   button 0x27(back/cancel)→ menu_list_latch(10) → 4 → NEWGAME_PUMP_BACK  (0xb)
 *
 * The latch refuses to act until sub->ready == 1000, so the drive ramps the
 * scene's input gate +NEWGAME_DRIVE_GATE_STEP/frame to 1000 — exactly the
 * title menu's menu_owner_transition_step mode-1 ramp (quirk #34/#59): the menu
 * becomes navigable ~20 frames after it appears.
 *
 * The option picker submenu (0x567ba0) is now wired: a CONFIRM on a kind-0 row
 * yields NEWGAME_OPEN_PICKER, and the drive opens that option's nested value
 * grid (newgame_picker) as a modal SUBMODE — subsequent frames pump input into
 * the picker (its own gate ramp) instead of the parent scene.  On the picker's
 * COMMIT the drive calls newgame_scene_set_option(id, chosen) to re-lay the
 * value cell; on CANCEL the option is left unchanged.  This is the frame-stepped
 * equivalent of retail's blocking modal FUN_00567ba0 call (which suspends the
 * parent's frame loop until it returns 0/0xc).
 *
 * DEFERRED seams (documented for the next unit, NOT modelled here):
 *   - the box widget chrome + the tooltip text node render (the second GDI-text
 *     node at y=416/444): the render callback's job; the scene already computes
 *     the tooltip text (newgame_scene_tooltip) for it.
 *   - axis-held auto-repeat (the title's dirs 4/5/6/7): discrete presses only
 *     here; trace-driven nav needs no repeat, live held-key repeat is a seam.
 */
#ifndef OPENSUMMONERS_NEWGAME_DRIVE_H
#define OPENSUMMONERS_NEWGAME_DRIVE_H

#include <stdint.h>

#include "newgame_scene.h"   /* newgame_scene / newgame_scene_status / dispatch */
#include "newgame_picker.h"  /* the option picker submenu the drive runs        */
#include "input.h"           /* input_mgr / input_event / INPUT_RING_LEN        */

/* The input-gate ramp (the +0x54 ready field +N/frame to 1000).  Matches the
 * title's menu_owner_transition_step mode-1 step (+50/frame ⇒ open ~20 frames
 * after spawn). */
#define NEWGAME_DRIVE_GATE_STEP 50

/* The post-config fade-out frame count (FUN_00564160's loop cap, `0x13 < uVar2`
 * ⇒ at most 0x13+1 = 20 iterations).  Once "Start Game" is committed, retail
 * does NOT enter the gem cutscene on the next flip: FUN_00564160 runs up to 20
 * more frames of the new-game scene fading out (the box node's mode-1 closing
 * alpha ramp) before 0x56cd20.  Porting this loop makes the port spend the same
 * ~20 flips between the commit and the cutscene, so the TAS cutscene anchor
 * offset is CONSTANT rather than drifting.  See newgame_drive_step. */
#define NEWGAME_FADEOUT_FRAMES 20

/* The mode-1 closing alpha-ramp step (0x56c930 field_50==0 arm: field_54 -=
 * 0x28 toward 0, 0x56cb0a).  Distinct from the +0x32 opening step. */
#define NEWGAME_FADEOUT_RAMP_STEP 0x28

/* What the drive needs to wire the scene to the live engine.  All callbacks
 * are optional (NULL ⇒ that side effect no-ops), so a headless drive degrades
 * to a faithful all-no-op run that still pumps input + dispatches. */
typedef struct newgame_drive_cfg {
    /* The starting option values (read per-row into the value column).  NULL ⇒
     * NEWGAME_SETTINGS_DEFAULT (difficulty 10 "1:Easy", auto-guard 1 "On"). */
    const newgame_settings *settings;

    /* Draw one frame: the box chrome + glyph_grid_render of the menu node + the
     * tooltip node, onto the primary surface (retail's generic compositor /
     * 0x48c820 path).  Receives the cfg `user` pointer.  NULL ⇒ no render. */
    void (*render)(void *user);

    /* Present the composed frame (retail FUN_005b8fc0 = zdd_present).  NULL ⇒
     * no present.  The real build's thunk also bumps the Flip/present counter
     * the input-trace replay keys on. */
    void (*present)(void *user);

    void *user;        /* opaque, passed to render / present                   */

    /* Per-frame input-gate ramp step; 0 selects NEWGAME_DRIVE_GATE_STEP. */
    int gate_step;
} newgame_drive_cfg;

/* The running drive: the scene + its owned input manager + the bound config.
 * Treat as opaque; read `done` / `result` after a step (and `scene` for the
 * render callback, which reads scene.node / scene.grid). */
typedef struct newgame_drive {
    newgame_scene scene;

    /* The option picker submenu, active only while `picker_active` is set.  When
     * active the per-frame input pump drives `picker.grid` (its own ramping gate)
     * instead of the parent scene; the render callback overlays it on the menu. */
    newgame_picker picker;
    int            picker_active;

    /* The post-config fade-out submode (FUN_00564160's loop tail).  Set by a
     * Start-Game commit instead of finishing immediately: while `fading` the
     * step ramps the box node's alpha down (NEWGAME_FADEOUT_RAMP_STEP/frame) and
     * re-renders for NEWGAME_FADEOUT_FRAMES frames, then returns NEWGAME_START so
     * the caller enters the cutscene at the same flip offset retail does. */
    int            fading;                 /* fade-out active (post-START)       */
    int32_t        fade_frames;            /* fade frames elapsed (0..FRAMES)    */

    input_mgr     input;

    /* Backing store for the input manager's 64-slot ring.  As in title_drive:
     * the poll path derefs ring[i] with no NULL guard, so every slot points at
     * an owned idle record; an all-idle ring is the faithful "no input" state. */
    input_event   ring_store[INPUT_RING_LEN];

    /* Borrowed cfg callbacks. */
    void (*render)(void *user);
    void (*present)(void *user);
    void *user;
    int   gate_step;

    int                  started;          /* init ran                          */
    int                  done;             /* a step returned START/BACK         */
    newgame_scene_status result;           /* the outcome (valid once done)      */

    /* Diagnostics (read by the drive's tests + main.c's logging). */
    int32_t  last_pump;                    /* last pump code fed to dispatch     */
    uint32_t picker_requests;              /* count of pickers opened            */
    uint32_t picker_commits;               /* count of picker value commits      */
} newgame_drive;

/* Build the scene (newgame_scene_init) + bind the input ring, store the cfg
 * callbacks, and seed the gate ramp step.  cfg is consumed by value.  Always
 * succeeds (no heap beyond newgame_scene_init's container build, which the
 * caller owns); returns 1 for symmetry with title_drive_init. */
int newgame_drive_init(newgame_drive *d, const newgame_drive_cfg *cfg);

/* Run one frame: ramp the input gate, poll+latch+collapse the buttons into a
 * pump result, dispatch it through newgame_scene, then render + present.
 * Returns the dispatch outcome:
 *   NEWGAME_RUNNING / NEWGAME_OPEN_PICKER → keep looping (OPEN_PICKER is the
 *       deferred picker seam: surfaced + counted, value left unchanged);
 *   NEWGAME_BACK → terminal (d->done set, d->result latched).
 *   NEWGAME_START → a Start-Game commit does NOT return START immediately: it
 *       enters the post-config fade-out (returns NEWGAME_RUNNING for the next
 *       NEWGAME_FADEOUT_FRAMES frames, ramping the box alpha down + presenting),
 *       then returns NEWGAME_START on the final fade frame (d->done/result set).
 *       This mirrors FUN_00564160's loop tail so the caller enters the cutscene
 *       at retail's flip offset (constant TAS anchor offset).
 * Idempotent after a terminal step (returns d->result without re-running). */
newgame_scene_status newgame_drive_step(newgame_drive *d, uint32_t now);

/* Free the scene's container (newgame_scene_clear).  Safe on a never-init'd or
 * already-shutdown drive (no double free). */
void newgame_drive_shutdown(newgame_drive *d);

#endif /* OPENSUMMONERS_NEWGAME_DRIVE_H */
