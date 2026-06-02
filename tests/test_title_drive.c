/*
 * tests/test_title_drive.c — host tests for the title-scene drive
 * (title_drive.c): the caller that owns the scene object graph, binds the
 * render sink, and runs title_scene_step until the scene returns.
 *
 * These exercise the drive's orchestration — allocation of the owner + node +
 * input ring, sink binding, the loop/stop contract, and clean teardown — with
 * the present / log side effects observed through spy callbacks.  The scene FSM
 * and the sink mapping each have their own test suites; here we prove the
 * caller wires them together correctly.
 */
#include "t.h"
#include "title_drive.h"
#include "menu_list.h"

#include <string.h>

/* ── spies for the cfg callbacks ── */
static int dr_present_n, dr_logflip_n;
static void *dr_last_user;
static void dr_present(void *user)  { dr_present_n++; dr_last_user = user; }
static void dr_logflip(void *user)  { dr_logflip_n++; dr_last_user = user; }
static void dr_reset_spies(void) { dr_present_n = dr_logflip_n = 0; dr_last_user = NULL; }

static int dr_user_tag = 0xC0DE;

/* A guaranteed-UPDATE pace (sub==2, zero elapsed, fat budget) so exactly one
 * title_fade_step runs this step — mirrors test_title_scene.c's ts_update. */
static void dr_force_update(title_drive *d, uint32_t now)
{
    d->scene.pace.sub           = TITLE_PACE_SUB_UPDATE;
    d->scene.pace.budget        = 0x40;
    d->scene.pace.tick_anchor   = 0;
    d->scene.pace.render_anchor = now;
}

/* A pace that resolves to RENDER with no pump (the scene-test recipe): sub==2,
 * 0 ms elapsed, budget 17 → burn one slice → 1 <= 16 → sub=1 (render). */
static void dr_force_render(title_drive *d)
{
    d->scene.pace = (title_pace_state){ .sub = 2, .budget = 0x11,
                                        .tick_anchor = 900, .render_anchor = 1000 };
}

static title_drive_cfg dr_basic_cfg(zdd_object *primary)
{
    title_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.primary    = primary;
    cfg.present    = dr_present;
    cfg.log_flip   = dr_logflip;
    cfg.user       = &dr_user_tag;
    cfg.select_key = 0x1e;
    cfg.quiet      = 0;
    cfg.skip_intro = 0;
    return cfg;
}

/* ── init allocates the object graph + binds the sink ── */
int test_title_drive_init_allocates_and_binds(void)
{
    title_drive d;
    title_drive_cfg cfg = dr_basic_cfg(NULL);

    title_render_sink_hook = NULL;
    T_ASSERT_EQ_I(title_drive_init(&d, &cfg), 1);

    /* owner: entry[0] is the allocated node, capacity defaulted, count 0. */
    T_ASSERT(d.entries != NULL);
    T_ASSERT(d.node0 != NULL);
    T_ASSERT_EQ_P((void *)d.owner.entries, (void *)d.entries);
    T_ASSERT_EQ_P((void *)d.owner.entries[0], d.node0);
    T_ASSERT_EQ_U(d.owner.capacity, TITLE_DRIVE_OWNER_CAP);
    T_ASSERT_EQ_U(d.owner.count, 0);

    /* the scene bound to our owner + input, with the init args carried through. */
    T_ASSERT_EQ_P(d.scene.owner, &d.owner);
    T_ASSERT_EQ_P(d.scene.input, &d.input);
    T_ASSERT_EQ_I(d.scene.select_key, 0x1e);

    /* the ring is fully populated (every slot points at an owned idle record). */
    for (int i = 0; i < INPUT_RING_LEN; i++) {
        T_ASSERT_EQ_P((void *)d.input.ring[i], (void *)&d.ring_store[i]);
        T_ASSERT_EQ_I(d.ring_store[i].id, 0);
    }

    /* the sink is installed + bound to our ctx. */
    T_ASSERT_EQ_P((void *)title_render_sink_hook, (void *)title_render_sink);
    T_ASSERT_EQ_P(d.sink.present, dr_present);
    T_ASSERT_EQ_P(d.sink.user, &dr_user_tag);

    title_drive_shutdown(&d);
    /* shutdown unbinds. */
    T_ASSERT_EQ_P((void *)title_render_sink_hook, NULL);
    return 0;
}

/* ── a render iteration routes FLIP → the present callback ── */
int test_title_drive_step_presents_on_render(void)
{
    zdd_object primary;
    memset(&primary, 0, sizeof primary);
    title_drive d;
    title_drive_cfg cfg = dr_basic_cfg(&primary);   /* non-NULL primary ⇒ FLIP fires */
    title_drive_init(&d, &cfg);

    d.scene.fade.phase = 5;          /* press-button handler draws a frame */
    d.scene.fade.fade  = 400;
    dr_force_render(&d);
    dr_reset_spies();

    title_scene_status st = title_drive_step(&d, 1000);

    T_ASSERT_EQ_I(st, TITLE_SCENE_RUNNING);
    T_ASSERT_EQ_I(dr_present_n, 1);          /* TITLE_DRAW_FLIP → present  */
    T_ASSERT_EQ_P(dr_last_user, &dr_user_tag);
    T_ASSERT_EQ_I(d.scene.already_flipped, 1);

    title_drive_shutdown(&d);
    return 0;
}

/* ── with a NULL primary every draw (and FLIP) no-ops — faithful headless ── */
int test_title_drive_step_null_primary_no_present(void)
{
    title_drive d;
    title_drive_cfg cfg = dr_basic_cfg(NULL);   /* NULL primary */
    title_drive_init(&d, &cfg);

    d.scene.fade.phase = 5;
    dr_force_render(&d);
    dr_reset_spies();

    title_scene_status st = title_drive_step(&d, 1000);
    T_ASSERT_EQ_I(st, TITLE_SCENE_RUNNING);
    T_ASSERT_EQ_I(dr_present_n, 0);          /* sink early-outs on NULL primary */

    title_drive_shutdown(&d);
    return 0;
}

/* ── the abort poll (a fresh 0x22 press) ends the scene with result 6 ── */
int test_title_drive_step_abort_returns_done(void)
{
    title_drive d;
    title_drive_cfg cfg = dr_basic_cfg(NULL);
    title_drive_init(&d, &cfg);

    /* inject a fresh abort press into the ring the drive owns. */
    d.ring_store[30].id = 0x22; d.ring_store[30].flag = 1; d.ring_store[30].ts = 5000;
    dr_force_update(&d, 5000);

    title_scene_status st = title_drive_step(&d, 5000);

    T_ASSERT_EQ_I(st, TITLE_SCENE_DONE);
    T_ASSERT_EQ_I(d.done, 1);
    T_ASSERT_EQ_I(d.result, 6);

    /* idempotent after DONE: no re-run, still DONE. */
    int32_t before = d.scene.fade.phase;
    st = title_drive_step(&d, 9999);
    T_ASSERT_EQ_I(st, TITLE_SCENE_DONE);
    T_ASSERT_EQ_I(d.scene.fade.phase, before);

    title_drive_shutdown(&d);
    return 0;
}

/* ── reaching the menu phase spawns the controller; shutdown frees it clean ──
 *
 * Drives one update frame at phase 8 (the default/menu arm) so title_menu_spawn
 * builds the controller on the owner's node, then tears the drive down.  Run
 * under ASan/LSan: a leak or double-free in the spawn/teardown bookkeeping
 * fails the test. */
int test_title_drive_menu_spawn_then_shutdown_clean(void)
{
    title_drive d;
    title_drive_cfg cfg = dr_basic_cfg(NULL);
    cfg.select_key = 0x1a;
    title_drive_init(&d, &cfg);

    d.scene.fade.phase = 8;          /* menu arm → spawn on first entry */
    dr_force_update(&d, 7000);

    title_scene_status st = title_drive_step(&d, 7000);
    T_ASSERT_EQ_I(st, TITLE_SCENE_RUNNING);

    /* the menu spawned: controller is the owner node's lone child. */
    T_ASSERT(d.scene.menu.ctrl != NULL);
    T_ASSERT(d.scene.menu.node != NULL);
    T_ASSERT_EQ_U(d.owner.count, 1);

    title_drive_shutdown(&d);        /* frees ctrl + node children + entries */
    T_ASSERT_EQ_P(d.entries, NULL);
    T_ASSERT_EQ_I(d.started, 0);
    return 0;
}

/* ── shutdown is safe on a never-spawned drive and on a fresh struct ── */
int test_title_drive_shutdown_no_spawn_and_uninit(void)
{
    title_drive d;
    title_drive_cfg cfg = dr_basic_cfg(NULL);
    title_drive_init(&d, &cfg);
    title_drive_shutdown(&d);        /* never reached the menu → frees node + entries */

    /* second shutdown is a no-op (started cleared). */
    title_drive_shutdown(&d);

    /* a freshly-zeroed (never-init'd) drive tolerates shutdown. */
    title_drive z;
    memset(&z, 0, sizeof z);
    title_drive_shutdown(&z);
    return 0;
}
