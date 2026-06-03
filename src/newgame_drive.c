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
static int32_t newgame_drive_pump(newgame_drive *d, menu_ctrl *c, uint32_t now)
{
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

/* Ramp an input-ready gate toward 1000 (+gate_step/frame), exactly as the
 * title's menu_owner_transition_step mode-1 arm opens the nav gate (the latch
 * refuses to act until sub.ready == 1000, quirk #34/#59). */
static void ramp_gate(menu_input_sub *sub, int step)
{
    if (sub->ready < 1000) {
        sub->ready += step;
        if (sub->ready > 1000)
            sub->ready = 1000;
    }
}

/* Open the picker for the focused option row (the OPEN_PICKER outcome).  The
 * value list comes from FUN_00568320; the difficulty unlock flag (retail
 * *(*DAT_008a6e80 + 0xaa4)) is left 0 — the port has no save data, so the 5th
 * "5:Nightmare" choice stays locked, matching a fresh new game.  Returns 1 if a
 * picker was opened, 0 if the option has no choices (the menu stays put). */
static int newgame_drive_open_picker(newgame_drive *d)
{
    int32_t id      = newgame_scene_focused_action(&d->scene);
    int32_t current = (id == NEWGAME_OPT_DIFFICULTY) ? d->scene.settings.difficulty
                    : (id == NEWGAME_OPT_AUTO_GUARD) ? d->scene.settings.auto_guard
                    : 0;
    if (!newgame_picker_init(&d->picker, id, current, /*difficulty_unlock=*/0))
        return 0;
    d->picker_active = 1;
    d->picker_requests++;
    return 1;
}

newgame_scene_status newgame_drive_step(newgame_drive *d, uint32_t now)
{
    if (!d->started || d->done)
        return d->result;

    /* ── Picker submode: pump input into the picker's own grid (its ramping
     *    gate), act on its commit/cancel, and keep the parent scene RUNNING.
     *    This is the frame-stepped form of retail's blocking FUN_00567ba0. ── */
    if (d->picker_active) {
        ramp_gate(&d->picker.sub, d->gate_step);
        int32_t pump = newgame_drive_pump(d, &d->picker.grid, now);
        d->last_pump = pump;
        newgame_picker_status ps = newgame_picker_dispatch(&d->picker, pump);
        if (ps == NEWGAME_PICKER_COMMIT) {
            /* FUN_005657f0 commit → the parent's value-refill block: store the
             * value + re-lay that option's value cell (newgame_scene_set_option). */
            newgame_scene_set_option(&d->scene, d->picker.option_id, d->picker.chosen);
            d->picker_commits++;
            newgame_picker_clear(&d->picker);
            d->picker_active = 0;
        } else if (ps == NEWGAME_PICKER_CANCEL) {
            newgame_picker_clear(&d->picker);
            d->picker_active = 0;
        }
        if (d->render != NULL)  d->render(d->user);
        if (d->present != NULL) d->present(d->user);
        return NEWGAME_RUNNING;
    }

    /* (1) Ramp the parent scene's input gate. */
    ramp_gate(&d->scene.sub, d->gate_step);

    /* (2) Poll + latch + collapse this frame's input into a pump result. */
    int32_t pump = newgame_drive_pump(d, &d->scene.grid, now);
    d->last_pump = pump;

    /* (3) Dispatch it through the scene's run-loop switch. */
    newgame_scene_status st = newgame_scene_dispatch(&d->scene, pump);

    /* (4) OPEN_PICKER opens the nested value grid as a modal submode (next
     *     frame the pump drives the picker).  If the option has no choices the
     *     menu stays put (retail's FUN_00567ba0 `return 0`). */
    if (st == NEWGAME_OPEN_PICKER) {
        newgame_drive_open_picker(d);
    }

    /* (5) Render + present the frame (the cursor move from step 2 is now live
     *     in the shared list header the render node reads). */
    if (d->render != NULL)  d->render(d->user);
    if (d->present != NULL) d->present(d->user);

    /* (6) START / BACK are terminal; RUNNING / OPEN_PICKER keep looping. */
    if (st == NEWGAME_START || st == NEWGAME_BACK) {
        d->done   = 1;
        d->result = st;
    }
    return st;
}

void newgame_drive_shutdown(newgame_drive *d)
{
    if (!d->started)
        return;
    if (d->picker_active) {
        newgame_picker_clear(&d->picker);
        d->picker_active = 0;
    }
    newgame_scene_clear(&d->scene);
    d->started = 0;
}
