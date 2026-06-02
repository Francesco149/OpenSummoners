/*
 * src/newgame_drive.c — the new-game config scene's drive (see newgame_drive.h).
 *
 * The caller side of FUN_00564780 case 0x24 / its frame pump FUN_00565d10:
 * owns the scene + input manager, ramps the input-ready gate open, polls the
 * buttons each frame and collapses them into the pump result newgame_scene
 * dispatches on, then renders + presents.  Pure C / Win32-free — the render /
 * present side effects arrive through the cfg callbacks.
 */

#include <string.h>

#include "newgame_drive.h"
#include "menu_list.h"     /* menu_list_latch — the nav/latch the pump feeds */

int newgame_drive_init(newgame_drive *d, const newgame_drive_cfg *cfg)
{
    memset(d, 0, sizeof *d);

    /* Build the scene (grid + node + bound, closed input gate). */
    newgame_scene_init(&d->scene, cfg->settings);

    /* A fresh, zeroed input manager with its 64-slot ring pointed at the
     * drive's owned idle records (the poll path derefs ring[i] unconditionally,
     * exactly as in title_drive). */
    for (int i = 0; i < INPUT_RING_LEN; i++)
        d->input.ring[i] = &d->ring_store[i];

    d->render    = cfg->render;
    d->present   = cfg->present;
    d->user      = cfg->user;
    d->gate_step = cfg->gate_step > 0 ? cfg->gate_step : NEWGAME_DRIVE_GATE_STEP;

    d->started         = 1;
    d->done            = 0;
    d->result          = NEWGAME_RUNNING;
    d->last_pump       = 0;
    d->picker_requests = 0;
    return 1;
}

/* The per-frame button scan (FUN_00565d10) + id→latch map (FUN_0043bca0,
 * quirk #65), collapsed to a pump result.  "Last latch wins" mirrors the
 * title's per-frame poll order (0x56b80f..0x56b8e3); with trace-injected
 * discrete presses only one button fires per frame anyway.  Returns one of
 * NEWGAME_PUMP_* or 0 (no actionable input this frame). */
static int32_t newgame_drive_pump(newgame_drive *d, uint32_t now)
{
    menu_ctrl *c = &d->scene.grid;
    int32_t r = 0;                                  /* latch result (esi) */

    if (input_poll_consume(&d->input, now, 2))    r = menu_list_latch(c, 2, now);
    if (input_poll_consume(&d->input, now, 4))    r = menu_list_latch(c, 3, now);
    if (input_poll_consume(&d->input, now, 1))    r = menu_list_latch(c, 0, now);
    if (input_poll_consume(&d->input, now, 3))    r = menu_list_latch(c, 1, now);
    if (input_poll_consume(&d->input, now, 0x24)) r = menu_list_latch(c, 9, now);
    if (input_poll_consume(&d->input, now, 0x27)) r = menu_list_latch(c, 10, now);

    switch (r) {
    case 1:                                          /* cursor moved   */
    case 2:  return NEWGAME_PUMP_MOVE;               /* page scrolled  */
    case 3:  return NEWGAME_PUMP_CONFIRM;            /* 0x24 → confirm */
    case 4:  return NEWGAME_PUMP_BACK;               /* 0x27 → back    */
    default: return 0;                               /* no-op          */
    }
}

newgame_scene_status newgame_drive_step(newgame_drive *d, uint32_t now)
{
    if (!d->started || d->done)
        return d->result;

    /* (1) Ramp the input-ready gate toward 1000 (+gate_step/frame), exactly as
     *     the title's menu_owner_transition_step mode-1 arm opens the nav gate
     *     (the latch refuses to act until sub.ready == 1000, quirk #34/#59). */
    if (d->scene.sub.ready < 1000) {
        d->scene.sub.ready += d->gate_step;
        if (d->scene.sub.ready > 1000)
            d->scene.sub.ready = 1000;
    }

    /* (2) Poll + latch + collapse this frame's input into a pump result. */
    int32_t pump = newgame_drive_pump(d, now);
    d->last_pump = pump;

    /* (3) Dispatch it through the scene's run-loop switch. */
    newgame_scene_status st = newgame_scene_dispatch(&d->scene, pump);

    /* (4) Render + present the frame (the cursor move from step 2 is now live
     *     in the shared list header the render node reads). */
    if (d->render != NULL)  d->render(d->user);
    if (d->present != NULL) d->present(d->user);

    /* (5) Resolve the outcome.  OPEN_PICKER is the deferred submenu seam: count
     *     it and keep running (the option value is left unchanged until the
     *     picker is ported).  START / BACK are terminal. */
    if (st == NEWGAME_OPEN_PICKER) {
        d->picker_requests++;
    } else if (st == NEWGAME_START || st == NEWGAME_BACK) {
        d->done   = 1;
        d->result = st;
    }
    return st;
}

void newgame_drive_shutdown(newgame_drive *d)
{
    if (!d->started)
        return;
    newgame_scene_clear(&d->scene);
    d->started = 0;
}
