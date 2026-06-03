/*
 * src/game_drive.c — the in-game map drive (see game_drive.h).
 *
 * The caller side of 0x59f2c0's per-map run loop: owns the in-game input
 * manager, steps once per presented frame, renders + presents.  Pure C /
 * Win32-free — the real blits + present arrive through the cfg callbacks.
 *
 * SCAFFOLD: the engine (0x59f2c0 / 0x586010 / 0x5a00c0) is unported,
 * so a step has no sim/render model behind it yet — it renders the faithful
 * black map-load frame and stays GAME_RUNNING.  See game_drive.h for the plan.
 */

#include <string.h>

#include "game_drive.h"

int game_drive_init(game_drive *d, const game_drive_cfg *cfg)
{
    memset(d, 0, sizeof *d);

    /* Point the 64-slot ring at the drive's owned idle records (the poll path
     * derefs ring[i] unconditionally, exactly as in the sibling drives). */
    for (int i = 0; i < INPUT_RING_LEN; i++)
        d->input.ring[i] = &d->ring_store[i];

    d->render  = cfg->render;
    d->present = cfg->present;
    d->user    = cfg->user;

    d->started = 1;
    d->ticks   = 0;
    return 1;
}

game_status game_drive_step(game_drive *d, uint32_t now)
{
    (void)now;   /* the ported engine pace clock (map-object +0x4068) is future */

    /* Compose + present this frame (retail's begin → draw → flip).  Until the
     * render dispatch 0x5a00c0 is ported, render() clears to black — the
     * correct map-load state retail shows from game_enter to the first town
     * frame. */
    if (d->render)  d->render(d->user);
    if (d->present) d->present(d->user);
    d->ticks++;

    return GAME_RUNNING;
}

void game_drive_shutdown(game_drive *d)
{
    d->started = 0;
}
