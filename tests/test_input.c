/*
 * tests/test_input.c — host-side tests for src/input.c, the input-event
 * ring poll FUN_0043c110.
 *
 * The unit is pure: each test lays out an input_mgr with explicit event
 * records in the 64-slot ring, polls, and asserts the return value, the
 * consume side effect (matched record's id zeroed), and that nothing
 * else changed.  Expectations are hand-derived from the disassembly at
 * 0x43c110 (see input.c).
 */
#include "../src/input.h"
#include "t.h"

#include <stdint.h>
#include <string.h>

/* Fill every ring slot with a pointer to a single "empty" record so no
 * slot is ever a dangling pointer (retail keeps all 64 populated).  Then
 * individual tests overwrite the slots they care about. */
static void mgr_init(input_mgr *m, input_event *empty)
{
    memset(m, 0, sizeof *m);
    empty->id = 0;            /* id 0 never matches a real button poll */
    empty->ts = 0;
    empty->flag = 0;
    for (int i = 0; i < INPUT_RING_LEN; i++) m->ring[i] = empty;
}

/* ─── layout / record sanity ─────────────────────────────────────────
 * The retail manager+0xc / +0x108 ring offsets are pinned by _Static_assert
 * in input.h on the 32-bit build (pointers are 8 bytes on this host, so the
 * absolute offsets can't be reproduced here).  What we *can* check on the
 * host is the record width the poll reads: id, ts, flag = three dwords. */
int test_input_mgr_ring_offsets(void)
{
    T_ASSERT_EQ_U(sizeof(input_event), 12);   /* id, ts, flag */
    T_ASSERT_EQ_U(INPUT_RING_LEN, 64);
    return 0;
}

/* ─── basic hit ──────────────────────────────────────────────────────
 * A pressed record for the polled id, fresh, returns 1 and is consumed. */
int test_input_poll_hit_consumes(void)
{
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    input_event e = { .id = 2, .ts = 1000, .flag = 1 };
    m.ring[40] = &e;

    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 1);
    T_ASSERT_EQ_I(e.id, 0);            /* consumed */
    T_ASSERT_EQ_U(e.ts, 1000);         /* ts untouched */
    T_ASSERT_EQ_I(e.flag, 1);          /* flag untouched */
    /* second poll for the same id now misses (id was zeroed). */
    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 0);
    return 0;
}

/* ─── id mismatch ────────────────────────────────────────────────── */
int test_input_poll_wrong_id_misses(void)
{
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    input_event e = { .id = 4, .ts = 1000, .flag = 1 };
    m.ring[10] = &e;

    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 0);
    T_ASSERT_EQ_I(e.id, 4);            /* not consumed */
    return 0;
}

/* ─── flag must be exactly 1 ─────────────────────────────────────────
 * flag 0 (released) and flag 2 (some other state) both miss. */
int test_input_poll_flag_must_be_one(void)
{
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    input_event released = { .id = 2, .ts = 1000, .flag = 0 };
    input_event other    = { .id = 2, .ts = 1000, .flag = 2 };

    m.ring[5] = &released;
    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 0);
    T_ASSERT_EQ_I(released.id, 2);

    m.ring[5] = &other;
    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 0);
    T_ASSERT_EQ_I(other.id, 2);
    return 0;
}

/* ─── age window: <= 100 ms accepted, 101 rejected ───────────────────
 * The boundary is inclusive at 100 (retail: `100 < (now-ts)` continues,
 * i.e. accept when now-ts <= 100). */
int test_input_poll_age_boundary(void)
{
    input_mgr m; input_event empty;

    /* exactly 100 ms old → hit */
    mgr_init(&m, &empty);
    input_event e100 = { .id = 2, .ts = 900, .flag = 1 };
    m.ring[0] = &e100;
    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 1);
    T_ASSERT_EQ_I(e100.id, 0);

    /* 101 ms old → miss */
    mgr_init(&m, &empty);
    input_event e101 = { .id = 2, .ts = 899, .flag = 1 };
    m.ring[0] = &e101;
    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 0);
    T_ASSERT_EQ_I(e101.id, 2);
    return 0;
}

/* ─── future timestamp wraps to "too old" via unsigned subtract ──────
 * A record whose ts is after `now` (now - ts underflows) is rejected,
 * exactly as retail's unsigned `ja`. */
int test_input_poll_future_ts_rejected(void)
{
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    input_event e = { .id = 2, .ts = 1001, .flag = 1 };  /* 1 ms ahead */
    m.ring[20] = &e;

    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 0);
    T_ASSERT_EQ_I(e.id, 2);
    return 0;
}

/* ─── GetTickCount rollover: tiny wrapped delta still accepted ───────
 * now just past the 2^32 wrap, ts just before it → real delta 2 ms. */
int test_input_poll_rollover_within_window(void)
{
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    input_event e = { .id = 7, .ts = 0xFFFFFFFFu, .flag = 1 };
    m.ring[30] = &e;

    /* now = 1 → (uint32)(1 - 0xFFFFFFFF) = 2 <= 100 → hit */
    T_ASSERT_EQ_I(input_poll_consume(&m, 1, 7), 1);
    T_ASSERT_EQ_I(e.id, 0);
    return 0;
}

/* ─── scan order: highest matching slot index wins ───────────────────
 * Two matching records; retail scans from index 63 down, so the higher
 * index is consumed and the lower one is left intact. */
int test_input_poll_highest_index_wins(void)
{
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    input_event lo = { .id = 2, .ts = 1000, .flag = 1 };
    input_event hi = { .id = 2, .ts = 1000, .flag = 1 };
    m.ring[10] = &lo;
    m.ring[50] = &hi;

    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 1);
    T_ASSERT_EQ_I(hi.id, 0);     /* higher index consumed */
    T_ASSERT_EQ_I(lo.id, 2);     /* lower index untouched */

    /* next poll consumes the remaining (lower) one */
    T_ASSERT_EQ_I(input_poll_consume(&m, 1000, 2), 1);
    T_ASSERT_EQ_I(lo.id, 0);
    return 0;
}

/* ─── slots 0 and 63 are both in range (off-by-one guards) ──────────── */
int test_input_poll_endpoints_in_range(void)
{
    input_mgr m; input_event empty;

    mgr_init(&m, &empty);
    input_event e0 = { .id = 9, .ts = 500, .flag = 1 };
    m.ring[0] = &e0;
    T_ASSERT_EQ_I(input_poll_consume(&m, 500, 9), 1);
    T_ASSERT_EQ_I(e0.id, 0);

    mgr_init(&m, &empty);
    input_event e63 = { .id = 9, .ts = 500, .flag = 1 };
    m.ring[63] = &e63;
    T_ASSERT_EQ_I(input_poll_consume(&m, 500, 9), 1);
    T_ASSERT_EQ_I(e63.id, 0);
    return 0;
}

/* ─── empty ring (all slots the never-matching record) misses ───────── */
int test_input_poll_empty_ring_misses(void)
{
    input_mgr m; input_event empty; mgr_init(&m, &empty);
    /* poll for id 0 must still miss: empty.flag == 0 fails the flag test
     * even though empty.id == 0 == button_id. */
    T_ASSERT_EQ_I(input_poll_consume(&m, 0, 0), 0);
    T_ASSERT_EQ_I(empty.id, 0);
    return 0;
}
