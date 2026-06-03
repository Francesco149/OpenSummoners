/*
 * src/prologue_drive.c — the Elemental-Stone cutscene drive (see prologue_drive.h).
 *
 * The caller side of FUN_0056cd20's frame loop: owns the cutscene state + input
 * manager, steps it one animation tick per presented frame, and renders +
 * presents while it runs.  Pure C / Win32-free — the real blits + present arrive
 * through the cfg callbacks.
 */

#include <string.h>

#include "prologue_drive.h"

int prologue_drive_init(prologue_drive *d, const prologue_drive_cfg *cfg)
{
    memset(d, 0, sizeof *d);

    prologue_stone_init(&d->scene);

    /* Point the 64-slot ring at the drive's owned idle records (the poll path
     * derefs ring[i] unconditionally, exactly as in title_drive/newgame_drive). */
    for (int i = 0; i < INPUT_RING_LEN; i++)
        d->input.ring[i] = &d->ring_store[i];

    d->render  = cfg->render;
    d->present = cfg->present;
    d->user    = cfg->user;

    d->started = 1;
    d->done    = 0;
    d->result  = PROLOGUE_RUNNING;
    d->ticks   = 0;
    return 1;
}

prologue_status prologue_drive_step(prologue_drive *d, uint32_t now)
{
    if (d->done)
        return d->result;                 /* terminal: idempotent */

    /* One animation tick — polls the input ring for abort (0x22) + beats. */
    prologue_status st = prologue_stone_update(&d->scene, &d->input, now);
    d->ticks++;

    if (st == PROLOGUE_RUNNING) {
        /* Compose + present this frame (retail's begin → draw → flip). */
        if (d->render)  d->render(d->user);
        if (d->present) d->present(d->user);
        return PROLOGUE_RUNNING;
    }

    /* Terminal (ABORT / DONE): latch the outcome.  Retail draws/flushes one
     * final frame on the cleanup path (0x56d556); the visual difference is a
     * single frame and the caller tears the scene down immediately, so the
     * drive simply stops here. */
    d->done   = 1;
    d->result = st;
    return st;
}

void prologue_drive_shutdown(prologue_drive *d)
{
    d->started = 0;
}
