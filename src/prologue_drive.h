/*
 * src/prologue_drive.h — the Elemental-Stone cutscene drive (the caller side of
 * FUN_0056cd20's frame loop).
 *
 * prologue_stone.c is the Win32-free model (the per-tick UPDATE state machine +
 * the render-descriptor build).  What was still missing to make the cutscene RUN
 * was the *caller* — the plumbing 0x56cd20's loop owns: own the cutscene state +
 * its input manager, step it one animation tick per frame, and render + present.
 * This module is that caller, factored exactly like title_drive / newgame_drive:
 * the structural loop is pure C and host-tests; the Win32 frame render (the real
 * zdd alpha/keyed blits onto the primary surface) + present arrive through the
 * cfg callbacks the real build (main.c) fills.
 *
 * Simpler than newgame_drive: the cutscene has NO input-gate ramp and NO nav
 * latch — it reads the raw input ring directly (input_poll_consume for the abort
 * 0x22, input_any_fresh_press for the beats), which prologue_stone_update already
 * does.  So the drive just steps the state and renders.
 *
 * Lifecycle:
 *     prologue_drive_init(&d, &cfg);
 *     while (running) {
 *         uint32_t now = GetTickCount();
 *         switch (prologue_drive_step(&d, now)) {
 *         case PROLOGUE_ABORT: ...back to title...;  running = 0; break;
 *         case PROLOGUE_DONE:  ...enter game...;     running = 0; break;
 *         default: break;                            // RUNNING: keep going
 *         }
 *     }
 *     prologue_drive_shutdown(&d);
 */
#ifndef OPENSUMMONERS_PROLOGUE_DRIVE_H
#define OPENSUMMONERS_PROLOGUE_DRIVE_H

#include <stdint.h>

#include "prologue_stone.h"  /* prologue_stone / prologue_status / render        */
#include "input.h"           /* input_mgr / input_event / INPUT_RING_LEN         */

/* What the drive needs to wire the cutscene to the live engine.  Both callbacks
 * are optional (NULL ⇒ no-op), so a headless drive degrades to a faithful all-
 * no-op run that still steps the state + polls input. */
typedef struct prologue_drive_cfg {
    /* Draw one frame: clear the primary to black, then blit the gem / aura /
     * sparkles from prologue_stone_render's draw list.  Receives `user`.
     * NULL ⇒ no render. */
    void (*render)(void *user);

    /* Present the composed frame (retail FUN_005b8fc0 = zdd_present).  NULL ⇒
     * no present.  The real build's thunk also bumps the Flip/present counter
     * the input-trace replay keys on. */
    void (*present)(void *user);

    void *user;        /* opaque, passed to render / present */
} prologue_drive_cfg;

/* The running drive: the cutscene state + its owned input manager + the bound
 * config.  Treat as opaque; read `done` / `result` after a step (and `scene`
 * for the render callback, which reads the cutscene state). */
typedef struct prologue_drive {
    prologue_stone scene;

    input_mgr   input;
    /* Backing store for the input ring (the poll path derefs ring[i] with no
     * NULL guard, so every slot points at an owned idle record — an all-idle
     * ring is the faithful "no input" state). */
    input_event ring_store[INPUT_RING_LEN];

    /* Borrowed cfg callbacks. */
    void (*render)(void *user);
    void (*present)(void *user);
    void *user;

    int             started;   /* init ran                              */
    int             done;      /* a step returned ABORT/DONE            */
    prologue_status result;    /* the outcome (valid once done)         */

    /* Diagnostics (read by the drive's tests + main.c's logging). */
    uint32_t ticks;            /* update ticks run                      */
} prologue_drive;

/* Init the cutscene (prologue_stone_init) + bind the input ring, store the cfg
 * callbacks.  cfg is consumed by value.  Returns 1. */
int prologue_drive_init(prologue_drive *d, const prologue_drive_cfg *cfg);

/* Run one frame: step the cutscene one animation tick (polling the input ring),
 * then render + present while it is still running.  Returns the step outcome:
 *   PROLOGUE_RUNNING → keep looping;
 *   PROLOGUE_ABORT / PROLOGUE_DONE → terminal (d->done set, d->result latched).
 * Idempotent after a terminal step (returns d->result without re-running). */
prologue_status prologue_drive_step(prologue_drive *d, uint32_t now);

/* No owned heap; resets `started`.  Safe on a never-init'd drive. */
void prologue_drive_shutdown(prologue_drive *d);

#endif /* OPENSUMMONERS_PROLOGUE_DRIVE_H */
