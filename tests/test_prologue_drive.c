/*
 * tests/test_prologue_drive.c — the Elemental-Stone cutscene drive
 * (src/prologue_drive.c, the caller side of FUN_0056cd20's frame loop).
 *
 * The drive owns the cutscene state + input ring, steps one tick per frame, and
 * renders/presents while running.  These tests use recording render/present
 * stubs and assert the loop structure: it renders each running frame, latches
 * the terminal outcome (ABORT on 0x22, DONE on watchdog), and is idempotent
 * after a terminal step.
 */
#include "t.h"
#include "prologue_drive.h"
#include "input.h"

#include <string.h>

static int s_renders;
static int s_presents;
static void rec_render(void *u)  { (void)u; s_renders++; }
static void rec_present(void *u) { (void)u; s_presents++; }

static void drive_init(prologue_drive *d)
{
    s_renders = s_presents = 0;
    prologue_drive_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.render  = rec_render;
    cfg.present = rec_present;
    cfg.user    = NULL;
    prologue_drive_init(d, &cfg);
}

/* A running step renders + presents and stays RUNNING. */
int test_prologue_drive_runs_and_renders(void)
{
    prologue_drive d;
    drive_init(&d);
    T_ASSERT_EQ_I(prologue_drive_step(&d, 1000), PROLOGUE_RUNNING);
    T_ASSERT_EQ_I(s_renders, 1);
    T_ASSERT_EQ_I(s_presents, 1);
    T_ASSERT_EQ_U(d.ticks, 1);
    T_ASSERT_EQ_I(d.done, 0);
    return 0;
}

/* An abort (0x22) press ends the drive with PROLOGUE_ABORT and no extra render. */
int test_prologue_drive_abort(void)
{
    prologue_drive d;
    drive_init(&d);
    input_event e = { .id = 0x22, .ts = 1000, .flag = 1 };
    d.input.ring[40] = &e;
    T_ASSERT_EQ_I(prologue_drive_step(&d, 1000), PROLOGUE_ABORT);
    T_ASSERT_EQ_I(d.done, 1);
    T_ASSERT_EQ_I(d.result, PROLOGUE_ABORT);
    T_ASSERT_EQ_I(s_renders, 0);            /* terminal frame is not drawn here */
    /* idempotent: a further step returns the latched result, no re-run. */
    T_ASSERT_EQ_I(prologue_drive_step(&d, 1000), PROLOGUE_ABORT);
    T_ASSERT_EQ_U(d.ticks, 1);
    return 0;
}

/* With no input, the drive eventually completes (watchdog) with PROLOGUE_DONE. */
int test_prologue_drive_completes(void)
{
    prologue_drive d;
    drive_init(&d);
    prologue_status st = PROLOGUE_RUNNING;
    int guard = 0;
    while (st == PROLOGUE_RUNNING && guard++ < 0x2000)
        st = prologue_drive_step(&d, 1000);
    T_ASSERT_EQ_I(st, PROLOGUE_DONE);
    T_ASSERT_EQ_I(d.done, 1);
    /* every running frame rendered exactly once. */
    T_ASSERT_EQ_I(s_renders, s_presents);
    T_ASSERT_EQ_U((uint32_t)s_renders, d.ticks - 1);  /* the terminal tick didn't render */
    return 0;
}
