/*
 * tests/test_game_drive.c — the in-game map drive (src/game_drive.c, the caller
 * side of FUN_0059f2c0's per-map run loop).
 *
 * The drive owns the in-game input ring, steps once per frame, and renders +
 * presents.  The engine is unported, so a step has no sim/render model behind it
 * yet — it renders the faithful black map-load frame and stays GAME_RUNNING.
 * These tests use recording render/present stubs and assert the loop structure:
 * init binds the ring + cfg, each step renders + presents exactly once and stays
 * RUNNING, and a NULL-callback drive still steps cleanly (headless).
 */
#include "t.h"
#include "game_drive.h"
#include "input.h"

#include <string.h>

static int s_renders;
static int s_presents;
static void rec_render(void *u)  { (void)u; s_renders++; }
static void rec_present(void *u) { (void)u; s_presents++; }

static void drive_init(game_drive *d)
{
    s_renders = s_presents = 0;
    game_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.render  = rec_render;
    cfg.present = rec_present;
    cfg.user    = NULL;
    game_drive_init(d, &cfg);
}

/* Init binds the input ring at the owned idle store and arms the drive. */
int test_game_drive_init(void)
{
    game_drive d;
    drive_init(&d);
    T_ASSERT_EQ_I(d.started, 1);
    T_ASSERT_EQ_U(d.ticks, 0);
    /* every ring slot points at the drive's own backing store (no NULL deref) */
    for (int i = 0; i < INPUT_RING_LEN; i++)
        T_ASSERT_EQ_P(d.input.ring[i], &d.ring_store[i]);
    return 0;
}

/* A step renders + presents exactly once and stays GAME_RUNNING. */
int test_game_drive_runs_and_renders(void)
{
    game_drive d;
    drive_init(&d);
    T_ASSERT_EQ_I(game_drive_step(&d, 1000), GAME_RUNNING);
    T_ASSERT_EQ_I(s_renders, 1);
    T_ASSERT_EQ_I(s_presents, 1);
    T_ASSERT_EQ_U(d.ticks, 1);
    /* keeps running frame after frame (no engine to end the map loop yet) */
    T_ASSERT_EQ_I(game_drive_step(&d, 1016), GAME_RUNNING);
    T_ASSERT_EQ_I(s_renders, 2);
    T_ASSERT_EQ_U(d.ticks, 2);
    return 0;
}

/* A drive with NULL callbacks (headless) still steps cleanly and counts ticks. */
int test_game_drive_headless(void)
{
    game_drive d;
    game_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    game_drive_init(&d, &cfg);
    T_ASSERT_EQ_I(game_drive_step(&d, 1000), GAME_RUNNING);
    T_ASSERT_EQ_U(d.ticks, 1);
    game_drive_shutdown(&d);
    T_ASSERT_EQ_I(d.started, 0);
    return 0;
}
