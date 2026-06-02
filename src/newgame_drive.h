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
 * DEFERRED seams (documented for the next unit, NOT modelled here):
 *   - the option picker submenu (0x567ba0): a CONFIRM on a kind-0 row yields
 *     NEWGAME_OPEN_PICKER; the drive surfaces it (picker_requests++) but does
 *     not yet open a nested grid — the value stays put.  Once ported, the
 *     picker's commit calls newgame_scene_set_option to re-lay the value cell.
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
#include "input.h"           /* input_mgr / input_event / INPUT_RING_LEN        */

/* The input-gate ramp (the +0x54 ready field +N/frame to 1000).  Matches the
 * title's menu_owner_transition_step mode-1 step (+50/frame ⇒ open ~20 frames
 * after spawn). */
#define NEWGAME_DRIVE_GATE_STEP 50

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
    uint32_t picker_requests;              /* count of OPEN_PICKER outcomes      */
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
 *   NEWGAME_START / NEWGAME_BACK → terminal (d->done set, d->result latched).
 * Idempotent after a terminal step (returns d->result without re-running). */
newgame_scene_status newgame_drive_step(newgame_drive *d, uint32_t now);

/* Free the scene's container (newgame_scene_clear).  Safe on a never-init'd or
 * already-shutdown drive (no double free). */
void newgame_drive_shutdown(newgame_drive *d);

#endif /* OPENSUMMONERS_NEWGAME_DRIVE_H */
