/*
 * tests/test_input_live.c — host tests for the live keyboard producer
 * (src/input_live.c, the port of FUN_0046a880).
 *
 * The contract: each frame rebuild axis_held[0..6] from the DIK snapshot
 * (clear-then-set), and post a ring event on each press/release EDGE of a
 * ring-mapped key.  The ring events are verified through the real consumer
 * (input_poll_consume), exactly how the menu/dialogue/dash paths read them.
 */
#include "../src/input_live.h"
#include "../src/input.h"
#include "t.h"

#include <string.h>

/* 64 DISTINCT ring records so each posted event lands in its own slot (retail
 * keeps all 64 populated; the producer overwrites at a rotating head). */
static input_event g_recs[INPUT_RING_LEN];
static void mgr_init(input_mgr *m)
{
    memset(m, 0, sizeof *m);
    memset(g_recs, 0, sizeof g_recs);
    for (int i = 0; i < INPUT_RING_LEN; i++) m->ring[i] = &g_recs[i];
}
static void hold(uint8_t dik[256], uint8_t sc) { dik[sc] = 0x80; }

/* ── axis-fill: arrows → [0..3], C → [4], X → [5]; release clears ── */
int test_input_live_axis_fill(void)
{
    input_mgr m; mgr_init(&m);
    input_live st; input_live_reset(&st);
    uint8_t dik[256];

    memset(dik, 0, sizeof dik);
    hold(dik, DIK_LEFT_ARROW); hold(dik, DIK_C); hold(dik, DIK_X);
    input_live_step(&st, &m, dik, 1000);
    T_ASSERT_EQ_I(m.axis_held[2], 1);   /* LEFT  */
    T_ASSERT_EQ_I(m.axis_held[4], 1);   /* jump  C */
    T_ASSERT_EQ_I(m.axis_held[5], 1);   /* attack X */
    T_ASSERT_EQ_I(m.axis_held[0], 0);
    T_ASSERT_EQ_I(m.axis_held[3], 0);

    memset(dik, 0, sizeof dik);          /* release all */
    input_live_step(&st, &m, dik, 1016);
    T_ASSERT_EQ_I(m.axis_held[2], 0);
    T_ASSERT_EQ_I(m.axis_held[4], 0);
    T_ASSERT_EQ_I(m.axis_held[5], 0);
    return 0;
}

/* ── clear-then-set: pre-dirtied slots are zeroed each frame ── */
int test_input_live_axis_clear_then_set(void)
{
    input_mgr m; mgr_init(&m);
    input_live st; input_live_reset(&st);
    m.axis_held[0] = 9; m.axis_held[5] = 9; m.axis_held[6] = 9;

    uint8_t dik[256]; memset(dik, 0, sizeof dik);
    hold(dik, DIK_RIGHT_ARROW);
    input_live_step(&st, &m, dik, 1000);
    T_ASSERT_EQ_I(m.axis_held[3], 1);   /* RIGHT set   */
    T_ASSERT_EQ_I(m.axis_held[0], 0);   /* stale cleared */
    T_ASSERT_EQ_I(m.axis_held[5], 0);
    T_ASSERT_EQ_I(m.axis_held[6], 0);
    return 0;
}

/* ── ring edge: Z press posts id 0x24; a held key does NOT re-post ── */
int test_input_live_ring_press_once(void)
{
    input_mgr m; mgr_init(&m);
    input_live st; input_live_reset(&st);
    uint8_t dik[256];

    memset(dik, 0, sizeof dik);          /* frame 1: baseline, no events */
    input_live_step(&st, &m, dik, 1000);
    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 0x24), 0);

    hold(dik, DIK_Z);                    /* frame 2: Z down → one press */
    input_live_step(&st, &m, dik, 1016);
    T_ASSERT_EQ_I(input_poll_consume(&m, 1016, 0x24), 1);

    input_live_step(&st, &m, dik, 1032); /* frame 3: Z still held → no edge */
    T_ASSERT_EQ_I(input_poll_consume(&m, 1032, 0x24), 0);

    memset(dik, 0, sizeof dik);          /* frame 4: release → flag-0 (not a press) */
    input_live_step(&st, &m, dik, 1048);
    T_ASSERT_EQ_I(input_poll_consume(&m, 1048, 0x24), 0);
    return 0;
}

/* ── the started gate: a key held on the FIRST step posts no phantom press ── */
int test_input_live_first_frame_no_phantom(void)
{
    input_mgr m; mgr_init(&m);
    input_live st; input_live_reset(&st);
    uint8_t dik[256]; memset(dik, 0, sizeof dik);
    hold(dik, DIK_LEFT_ARROW);

    input_live_step(&st, &m, dik, 1000);            /* first step, LEFT already down */
    T_ASSERT_EQ_I(m.axis_held[2], 1);               /* axis still fills */
    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 0); /* but no ring press */
    return 0;
}

/* ── direction ring ids: LEFT→2, RIGHT→4 (the dash/menu-nav ids) ── */
int test_input_live_direction_ring_ids(void)
{
    input_mgr m; mgr_init(&m);
    input_live st; input_live_reset(&st);
    uint8_t dik[256];

    memset(dik, 0, sizeof dik);                     /* baseline */
    input_live_step(&st, &m, dik, 1000);
    hold(dik, DIK_LEFT_ARROW); hold(dik, DIK_RIGHT_ARROW);
    input_live_step(&st, &m, dik, 1016);
    T_ASSERT_EQ_I(input_poll_consume(&m, 1016, 2), 1);   /* LEFT  */
    T_ASSERT_EQ_I(input_poll_consume(&m, 1016, 4), 1);   /* RIGHT */
    return 0;
}

/* ── guards: NULL pointers are safe no-ops ── */
int test_input_live_guards(void)
{
    input_live st; input_live_reset(&st);
    input_mgr m; mgr_init(&m);
    uint8_t dik[256]; memset(dik, 0, sizeof dik);
    input_live_step(NULL, &m,  dik,  0);
    input_live_step(&st,  NULL, dik,  0);
    input_live_step(&st,  &m,   NULL, 0);
    input_live_reset(NULL);
    return 0;
}
