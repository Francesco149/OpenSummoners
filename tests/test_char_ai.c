/*
 * test_char_ai.c — host tests for the pure pieces of the character AI port:
 * the ring queries (input.c FUN_00479960 / 0x479ca0 / 0x479a10) and the command
 * / party-slot setters (char_ai.c).  The AI itself is verified inside retail by
 * rhost (shadow:char_ai: 8,750 calls / 3 scenarios, full-state equal).
 */
#include "char_ai.h"
#include "input.h"
#include "t.h"

#include <string.h>

typedef struct { input_mgr m; input_event ev[INPUT_RING_LEN]; } ring_fx;

static void ring_init(ring_fx *f)
{
    memset(f, 0, sizeof *f);
    for (int i = 0; i < INPUT_RING_LEN; i++) f->m.ring[i] = &f->ev[i];
}

int test_char_ai_ring_find_window_flag_consume(void)
{
    ring_fx f; ring_init(&f);
    f.ev[5] = (input_event){ 3, 1000, 1 };          /* DOWN pressed at t=1000 */
    f.ev[9] = (input_event){ 3, 1100, 0 };          /* ...released at 1100   */
    T_ASSERT_EQ_I(input_ring_find(&f.m, 1150, 0, 200, 1, 3, 0, 0, NULL, 0), 5);
    T_ASSERT_EQ_I(input_ring_find(&f.m, 1150, 0, 100, 1, 3, 0, 0, NULL, 0), -1);   /* too old */
    T_ASSERT_EQ_I(input_ring_find(&f.m, 1150, 0, 200, 0, 3, 0, 0, NULL, 0), 9);    /* release */
    T_ASSERT_EQ_I(input_ring_find(&f.m, 1150, 0, 200, 1, 5, 3, 0, NULL, 1), 5);    /* 5 -> fwd */
    T_ASSERT_EQ_I(f.ev[5].id, 0);                                                  /* consumed */
    return 0;
}

int test_char_ai_ring_double_needs_two_records(void)
{
    ring_fx f; ring_init(&f);
    int32_t mask[INPUT_RING_LEN] = { 0 };
    f.ev[0] = (input_event){ 4, 1000, 1 };
    T_ASSERT_EQ_I(input_ring_find(&f.m, 1050, 0, 300, 1, 4, 0, 0, mask, 0), 0);
    T_ASSERT_EQ_I(input_ring_find(&f.m, 1050, 0, 300, 1, 4, 0, 0, mask, 0), -1);  /* masked */
    T_ASSERT_EQ_I(mask[0], 1);
    T_ASSERT_EQ_I(input_ring_hit(&f.m, 1050, 0, 300, 1, 6, 0), 0);               /* 6 -> id 0 */
    return 0;
}

int test_char_ai_combo_chain(void)
{
    ring_fx f; ring_init(&f);
    f.ev[0] = (input_event){ 3, 1000, 1 };          /* DOWN */
    f.ev[1] = (input_event){ 4, 1050, 1 };          /* then forward (right) */
    wl_combo c; memset(&c, 0, sizeof c);
    c.ts_mode = 1; c.lo = 0; c.hi = 300; c.hi2 = 300; c.lo2 = 0;
    c.nids = 2; c.ids[0] = 5; c.ids[1] = 3;         /* matched newest-first: fwd, then DOWN */
    T_ASSERT_EQ_I(input_combo_match(&f.m, &c, 1100, 4, 2, 1, 0), 1);
    T_ASSERT_EQ_I(f.ev[0].id, 0);                   /* consumed */
    T_ASSERT_EQ_I(f.ev[1].id, 0);
    return 0;
}

int test_char_ai_cmd_and_slots(void)
{
    wl_cmd c; memset(&c, 0xff, sizeof c);
    cmd_clear(&c);
    T_ASSERT_EQ_I(c.lr, 0); T_ASSERT_EQ_I(c.skill_kind, 0); T_ASSERT_EQ_I(c.skill_ptr, 0);
    cmd_set_skill(&c, 7, 3, 0);
    T_ASSERT_EQ_I(c.skill, 7); T_ASSERT_EQ_I(c.skill_kind, 210); T_ASSERT_EQ_I(c.skill_arg, 3);
    wl_party p; memset(&p, 0, sizeof p);
    p.slot_mode = 2;
    T_ASSERT_EQ_I(party_slot_set(&p, 4), 1);        /* mode 2 -> slot index 1 */
    T_ASSERT_EQ_I(p.slot[1], 4);
    T_ASSERT_EQ_I(party_slot_get(&p), 4);
    return 0;
}
