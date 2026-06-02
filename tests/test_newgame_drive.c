/*
 * tests/test_newgame_drive.c — host tests for the new-game config drive
 * (src/newgame_drive.c): the caller that owns the scene + input ring, ramps the
 * input gate open, polls + latches the buttons into a pump result, dispatches
 * it, and renders + presents.
 *
 * The scene model (newgame_scene) and the grid builder (newgame_menu) have
 * their own suites; here we prove the caller wires them together — the gate
 * ramp, the id→latch→pump-result mapping (quirk #65), the terminal outcomes,
 * and clean teardown — with render/present observed through spy callbacks and
 * input injected into the ring the drive owns.
 */
#include "t.h"
#include "newgame_drive.h"
#include "newgame_scene.h"
#include "newgame_menu.h"
#include "menu_list.h"

#include <string.h>

/* ── spies for the cfg callbacks ── */
static int   ng_render_n, ng_present_n;
static void *ng_last_user;
static void  ng_render(void *u)  { ng_render_n++;  ng_last_user = u; }
static void  ng_present(void *u) { ng_present_n++; ng_last_user = u; }
static void  ng_reset_spies(void){ ng_render_n = ng_present_n = 0; ng_last_user = NULL; }

static int ng_user_tag = 0xFEED;

static newgame_drive_cfg ng_basic_cfg(void)
{
    newgame_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.settings  = NULL;          /* defaults: Easy / On */
    cfg.render    = ng_render;
    cfg.present   = ng_present;
    cfg.user      = &ng_user_tag;
    cfg.gate_step = 0;             /* → NEWGAME_DRIVE_GATE_STEP (50) */
    return cfg;
}

/* Inject a fresh press of `button` into ring slot `k` at time `now`. */
static void ng_press(newgame_drive *d, int k, int32_t button, uint32_t now)
{
    d->ring_store[k].id = button; d->ring_store[k].flag = 1; d->ring_store[k].ts = now;
}

/* ── init builds the scene, populates the ring, binds the cfg ── */
int test_newgame_drive_init_builds_and_binds(void)
{
    newgame_drive d;
    newgame_drive_cfg cfg = ng_basic_cfg();
    T_ASSERT_EQ_I(newgame_drive_init(&d, &cfg), 1);

    /* scene built: 3 rows, focus on row 0, the input gate bound but CLOSED. */
    T_ASSERT_EQ_I(d.scene.grid.list->count, 3);
    T_ASSERT_EQ_I(newgame_scene_focused_row(&d.scene), 0);
    T_ASSERT_EQ_P((void *)d.scene.grid.sub, (void *)&d.scene.sub);
    T_ASSERT_EQ_I(d.scene.sub.ready, 0);
    T_ASSERT_EQ_I(d.gate_step, NEWGAME_DRIVE_GATE_STEP);

    /* the ring is fully populated with owned idle records. */
    for (int i = 0; i < INPUT_RING_LEN; i++) {
        T_ASSERT_EQ_P((void *)d.input.ring[i], (void *)&d.ring_store[i]);
        T_ASSERT_EQ_I(d.ring_store[i].id, 0);
    }

    T_ASSERT_EQ_P((void *)d.render, (void *)ng_render);
    T_ASSERT_EQ_P((void *)d.present, (void *)ng_present);

    newgame_drive_shutdown(&d);
    T_ASSERT_EQ_I(d.started, 0);
    return 0;
}

/* ── each step renders + presents and ramps the gate toward 1000 ── */
int test_newgame_drive_step_ramps_gate_and_renders(void)
{
    newgame_drive d;
    newgame_drive_cfg cfg = ng_basic_cfg();
    newgame_drive_init(&d, &cfg);
    ng_reset_spies();

    /* One step: render + present once, gate opens by one step. */
    newgame_scene_status st = newgame_drive_step(&d, 1000);
    T_ASSERT_EQ_I(st, NEWGAME_RUNNING);
    T_ASSERT_EQ_I(ng_render_n, 1);
    T_ASSERT_EQ_I(ng_present_n, 1);
    T_ASSERT_EQ_P(ng_last_user, &ng_user_tag);
    T_ASSERT_EQ_I(d.scene.sub.ready, NEWGAME_DRIVE_GATE_STEP);

    /* Ramp to fully open: 1000/50 = 20 steps total; 19 more from here. */
    for (int i = 0; i < 19; i++) newgame_drive_step(&d, 1000);
    T_ASSERT_EQ_I(d.scene.sub.ready, 1000);
    /* further steps clamp at 1000. */
    newgame_drive_step(&d, 1000);
    T_ASSERT_EQ_I(d.scene.sub.ready, 1000);

    newgame_drive_shutdown(&d);
    return 0;
}

/* ── nav is gated closed until the ramp completes, then DOWN moves the cursor ─ */
int test_newgame_drive_nav_gated_then_moves(void)
{
    newgame_drive d;
    newgame_drive_cfg cfg = ng_basic_cfg();
    newgame_drive_init(&d, &cfg);

    /* Press DOWN (button 3) on the very first frame: gate still closed (ramps
     * to 50 this step), so the cursor must NOT move. */
    ng_press(&d, 10, 3, 1000);
    newgame_scene_status st = newgame_drive_step(&d, 1000);
    T_ASSERT_EQ_I(st, NEWGAME_RUNNING);
    T_ASSERT_EQ_I(newgame_scene_focused_row(&d.scene), 0);
    T_ASSERT_EQ_I(d.last_pump, 0);                 /* gated → no pump */

    /* Open the gate (as ~20 frames of ramp would), then DOWN moves 0→1. */
    d.scene.sub.ready = 1000;
    ng_press(&d, 11, 3, 2000);
    st = newgame_drive_step(&d, 2000);
    T_ASSERT_EQ_I(st, NEWGAME_RUNNING);
    T_ASSERT_EQ_I(d.last_pump, NEWGAME_PUMP_MOVE);
    T_ASSERT_EQ_I(newgame_scene_focused_row(&d.scene), 1);

    /* UP (button 1) moves back 1→0. */
    ng_press(&d, 12, 1, 3000);
    newgame_drive_step(&d, 3000);
    T_ASSERT_EQ_I(newgame_scene_focused_row(&d.scene), 0);

    newgame_drive_shutdown(&d);
    return 0;
}

/* ── confirm (0x24) on the Start-Game row ends the drive with START ── */
int test_newgame_drive_confirm_start_game(void)
{
    newgame_drive d;
    newgame_drive_cfg cfg = ng_basic_cfg();
    newgame_drive_init(&d, &cfg);
    d.scene.sub.ready = 1000;                       /* gate open */

    /* Move the cursor to row 2 (Start Game, kind 3). */
    d.scene.grid.list->cursor = 2;
    T_ASSERT_EQ_I(newgame_scene_focused_action(&d.scene), NEWGAME_OPT_START_GAME);

    ng_press(&d, 5, 0x24, 5000);                    /* confirm button */
    newgame_scene_status st = newgame_drive_step(&d, 5000);

    T_ASSERT_EQ_I(d.last_pump, NEWGAME_PUMP_CONFIRM);
    T_ASSERT_EQ_I(st, NEWGAME_START);
    T_ASSERT_EQ_I(d.done, 1);
    T_ASSERT_EQ_I(d.result, NEWGAME_START);

    /* idempotent after terminal: no re-run, still START. */
    int32_t row_before = newgame_scene_focused_row(&d.scene);
    st = newgame_drive_step(&d, 9999);
    T_ASSERT_EQ_I(st, NEWGAME_START);
    T_ASSERT_EQ_I(newgame_scene_focused_row(&d.scene), row_before);

    newgame_drive_shutdown(&d);
    return 0;
}

/* ── back (0x27) ends the drive with BACK regardless of the focused row ── */
int test_newgame_drive_back_returns_to_title(void)
{
    newgame_drive d;
    newgame_drive_cfg cfg = ng_basic_cfg();
    newgame_drive_init(&d, &cfg);
    d.scene.sub.ready = 1000;

    ng_press(&d, 7, 0x27, 6000);                    /* back/cancel button */
    newgame_scene_status st = newgame_drive_step(&d, 6000);

    T_ASSERT_EQ_I(d.last_pump, NEWGAME_PUMP_BACK);
    T_ASSERT_EQ_I(st, NEWGAME_BACK);
    T_ASSERT_EQ_I(d.done, 1);
    T_ASSERT_EQ_I(d.result, NEWGAME_BACK);

    newgame_drive_shutdown(&d);
    return 0;
}

/* ── confirm on an option row surfaces OPEN_PICKER + keeps running ── */
int test_newgame_drive_confirm_option_opens_picker(void)
{
    newgame_drive d;
    newgame_drive_cfg cfg = ng_basic_cfg();
    newgame_drive_init(&d, &cfg);
    d.scene.sub.ready = 1000;

    d.scene.grid.list->cursor = 0;                  /* Game Difficulty (kind 0) */
    ng_press(&d, 3, 0x24, 7000);
    newgame_scene_status st = newgame_drive_step(&d, 7000);

    T_ASSERT_EQ_I(d.last_pump, NEWGAME_PUMP_CONFIRM);
    T_ASSERT_EQ_I(st, NEWGAME_OPEN_PICKER);
    T_ASSERT_EQ_U(d.picker_requests, 1);
    T_ASSERT_EQ_I(d.done, 0);                        /* not terminal — keeps running */

    newgame_drive_shutdown(&d);
    return 0;
}

/* ── shutdown is safe on a fresh struct and idempotent ── */
int test_newgame_drive_shutdown_safe(void)
{
    newgame_drive d;
    newgame_drive_cfg cfg = ng_basic_cfg();
    newgame_drive_init(&d, &cfg);
    newgame_drive_shutdown(&d);
    newgame_drive_shutdown(&d);                      /* second is a no-op */

    newgame_drive z;
    memset(&z, 0, sizeof z);
    newgame_drive_shutdown(&z);                      /* never-init'd tolerates it */
    return 0;
}
