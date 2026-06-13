/*
 * tests/test_input_trace.c — host tests for the port-side input replay
 * (src/input_trace.c): the {"frame":N,"ids":[..]} JSONL parser + the
 * ring-injection replay pump.
 *
 * The parser is the load-bearing surface (it ingests harness goldens that
 * drive the port deterministically); the replay pump's contract is "inject
 * each due entry's ids into the ring as fresh presses, once".
 */
#include "t.h"
#include "input_trace.h"

#include <string.h>

/* A 64-slot ring backed by real idle records, like the drive owns. */
static input_event it_ring[INPUT_RING_LEN];
static void mk_input(input_mgr *m)
{
    memset(m, 0, sizeof *m);
    memset(it_ring, 0, sizeof it_ring);
    for (int i = 0; i < INPUT_RING_LEN; i++) m->ring[i] = &it_ring[i];
}

/* ── parse: a well-formed trace ── */
int test_input_trace_parse_basic(void)
{
    const char *buf =
        "{\"frame\":0, \"ids\":[]}\n"
        "{\"frame\":30, \"ids\":[3]}\n"
        "{\"frame\":31, \"ids\":[0x24, 1]}\n";
    struct input_trace t;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf, strlen(buf), &t), 1);
    T_ASSERT_EQ_U(t.count, 3);
    T_ASSERT_EQ_U(t.entries[0].frame, 0);
    T_ASSERT_EQ_U(t.entries[0].n_ids, 0);
    T_ASSERT_EQ_U(t.entries[1].frame, 30);
    T_ASSERT_EQ_U(t.entries[1].n_ids, 1);
    T_ASSERT_EQ_I(t.entries[1].ids[0], 3);
    T_ASSERT_EQ_U(t.entries[2].n_ids, 2);
    T_ASSERT_EQ_I(t.entries[2].ids[0], 0x24);
    T_ASSERT_EQ_I(t.entries[2].ids[1], 1);
    input_trace_free(&t);
    return 0;
}

/* ── parse: blank lines, comments, key order, no "ids" key ── */
int test_input_trace_parse_tolerant(void)
{
    const char *buf =
        "# a comment\n"
        "\n"
        "  {\"ids\":[2,4], \"frame\":5}  \n"   /* ids before frame, trailing ws */
        "{\"frame\":9}\n";                       /* no ids → n_ids 0 */
    struct input_trace t;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf, strlen(buf), &t), 1);
    T_ASSERT_EQ_U(t.count, 2);
    T_ASSERT_EQ_U(t.entries[0].frame, 5);
    T_ASSERT_EQ_U(t.entries[0].n_ids, 2);
    T_ASSERT_EQ_I(t.entries[0].ids[0], 2);
    T_ASSERT_EQ_I(t.entries[0].ids[1], 4);
    T_ASSERT_EQ_U(t.entries[1].frame, 9);
    T_ASSERT_EQ_U(t.entries[1].n_ids, 0);
    input_trace_free(&t);
    return 0;
}

/* ── parse: out-of-order frames fail ── */
int test_input_trace_parse_out_of_order_fails(void)
{
    const char *buf = "{\"frame\":10,\"ids\":[1]}\n{\"frame\":9,\"ids\":[1]}\n";
    struct input_trace t;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf, strlen(buf), &t), 0);
    T_ASSERT_EQ_U(t.count, 1);   /* the first (valid) entry survives */
    input_trace_free(&t);
    return 0;
}

/* ── parse: malformed object fails ── */
int test_input_trace_parse_malformed_fails(void)
{
    const char *buf = "{\"frame\":1,\"ids\":[1,]}\n";   /* trailing comma in array */
    struct input_trace t;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf, strlen(buf), &t), 0);
    input_trace_free(&t);

    const char *buf2 = "{\"frame\":1,\"bogus\":2}\n";   /* unknown key */
    struct input_trace t2;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf2, strlen(buf2), &t2), 0);
    input_trace_free(&t2);
    return 0;
}

/* ── replay: injects each due entry once, at the right frame ── */
int test_input_trace_replay_injects_at_frame(void)
{
    const char *buf =
        "{\"frame\":2, \"ids\":[3]}\n"
        "{\"frame\":4, \"ids\":[0x24]}\n";
    struct input_trace t;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf, strlen(buf), &t), 1);

    input_mgr m; mk_input(&m);

    /* frame 0/1: nothing due.  (sim_tick arg is ignored for a FRAME-axis
     * trace — pass a high value to prove the frame counter drives firing.) */
    input_trace_replay(&t, 0, 999, &m, 1000);
    input_trace_replay(&t, 1, 999, &m, 1000);
    T_ASSERT_EQ_U(t.cursor, 0);
    for (int i = 0; i < INPUT_RING_LEN; i++) T_ASSERT_EQ_I(it_ring[i].id, 0);

    /* frame 2: the first entry fires into ring[0]. */
    input_trace_replay(&t, 2, 0, &m, 2000);
    T_ASSERT_EQ_U(t.cursor, 1);
    T_ASSERT_EQ_I(it_ring[0].id, 3);
    T_ASSERT_EQ_I(it_ring[0].flag, 1);
    T_ASSERT_EQ_U(it_ring[0].ts, 2000);

    /* frame 3: still nothing new (next entry is frame 4). */
    input_trace_replay(&t, 3, 0, &m, 3000);
    T_ASSERT_EQ_U(t.cursor, 1);

    /* frame 5 (>= 4): the second entry fires into ring[1]. */
    input_trace_replay(&t, 5, 0, &m, 5000);
    T_ASSERT_EQ_U(t.cursor, 2);
    T_ASSERT_EQ_I(it_ring[1].id, 0x24);
    T_ASSERT_EQ_U(it_ring[1].ts, 5000);

    /* fully consumed — further calls are no-ops. */
    input_trace_replay(&t, 100, 0, &m, 9000);
    T_ASSERT_EQ_U(t.cursor, 2);
    input_trace_free(&t);
    return 0;
}

/* ── replay: a frame whose entry was skipped past still fires (<=) ── */
int test_input_trace_replay_catches_up(void)
{
    const char *buf =
        "{\"frame\":1, \"ids\":[1]}\n"
        "{\"frame\":2, \"ids\":[3]}\n"
        "{\"frame\":3, \"ids\":[1]}\n";
    struct input_trace t;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf, strlen(buf), &t), 1);

    input_mgr m; mk_input(&m);
    /* jump straight to frame 5: all three entries fire in order. */
    input_trace_replay(&t, 5, 0, &m, 7000);
    T_ASSERT_EQ_U(t.cursor, 3);
    T_ASSERT_EQ_I(it_ring[0].id, 1);
    T_ASSERT_EQ_I(it_ring[1].id, 3);
    T_ASSERT_EQ_I(it_ring[2].id, 1);
    input_trace_free(&t);
    return 0;
}

/* ── replay: NULL mgr / empty trace are no-ops ── */
int test_input_trace_replay_guards(void)
{
    struct input_trace t;
    memset(&t, 0, sizeof t);
    input_trace_replay(&t, 10, 0, NULL, 0);     /* empty + NULL mgr */

    input_mgr m; mk_input(&m);
    input_trace_replay(&t, 10, 0, &m, 0);       /* empty trace */
    T_ASSERT_EQ_U(t.cursor, 0);
    input_trace_free(&t);

    /* double free is safe. */
    input_trace_free(&t);
    return 0;
}

/* ── tick axis: a {"tick":N} trace keys on sim_tick, not the Flip frame ── */
int test_input_trace_tick_axis(void)
{
    const char *buf =
        "{\"tick\":5, \"ids\":[0x24]}\n"
        "{\"tick\":12, \"ids\":[0x24]}\n";
    struct input_trace t;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf, strlen(buf), &t), 1);
    T_ASSERT_EQ_U(t.count, 2);
    T_ASSERT_EQ_U(t.entries[0].axis, INPUT_TRACE_AXIS_TICK);
    T_ASSERT_EQ_U(t.entries[0].frame, 5);   /* threshold lives in .frame */

    input_mgr m; mk_input(&m);

    /* A high FRAME but a low TICK: nothing fires (the trace keys on tick). */
    input_trace_replay(&t, 9999, 4, &m, 1000);
    T_ASSERT_EQ_U(t.cursor, 0);

    /* tick reaches 5: the first entry fires; the FRAME is irrelevant. */
    input_trace_replay(&t, 0, 5, &m, 2000);
    T_ASSERT_EQ_U(t.cursor, 1);
    T_ASSERT_EQ_I(it_ring[0].id, 0x24);
    T_ASSERT_EQ_U(it_ring[0].ts, 2000);

    /* tick 12 fires the second. */
    input_trace_replay(&t, 0, 12, &m, 3000);
    T_ASSERT_EQ_U(t.cursor, 2);
    T_ASSERT_EQ_I(it_ring[1].id, 0x24);
    input_trace_free(&t);
    return 0;
}

/* ── mixed axes: a flip-keyed boot prefix then tick-keyed in-game confirms (the
 *    matched-cadence nav shape) parses + replays per-entry ── */
int test_input_trace_mixed_axis(void)
{
    const char *buf =
        "{\"frame\":700, \"ids\":[0x24]}\n"   /* boot: fires by Flip frame   */
        "{\"tick\":5,    \"ids\":[0x24]}\n";  /* in-game: fires by sim_tick  */
    struct input_trace t;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf, strlen(buf), &t), 1);
    T_ASSERT_EQ_U(t.count, 2);
    T_ASSERT_EQ_U(t.entries[0].axis, INPUT_TRACE_AXIS_FRAME);
    T_ASSERT_EQ_U(t.entries[1].axis, INPUT_TRACE_AXIS_TICK);

    input_mgr m; mk_input(&m);

    /* boot: present_frame 700 reached, sim_tick still 0 → only the flip entry */
    input_trace_replay(&t, 700, 0, &m, 1000);
    T_ASSERT_EQ_U(t.cursor, 1);
    T_ASSERT_EQ_I(it_ring[0].id, 0x24);

    /* the tick entry waits for sim_tick 5 even though the frame keeps climbing */
    input_trace_replay(&t, 9999, 4, &m, 2000);
    T_ASSERT_EQ_U(t.cursor, 1);
    input_trace_replay(&t, 9999, 5, &m, 3000);
    T_ASSERT_EQ_U(t.cursor, 2);
    T_ASSERT_EQ_I(it_ring[1].id, 0x24);
    input_trace_free(&t);

    /* a single object carrying BOTH a frame and a tick key is ambiguous → fail */
    const char *buf2 = "{\"frame\":1,\"tick\":2,\"ids\":[1]}\n";
    struct input_trace t2;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf2, strlen(buf2), &t2), 0);
    input_trace_free(&t2);

    /* out-of-order WITHIN one axis still fails */
    const char *buf3 = "{\"tick\":10,\"ids\":[1]}\n{\"tick\":9,\"ids\":[1]}\n";
    struct input_trace t3;
    T_ASSERT_EQ_I(input_trace_parse_buf(buf3, strlen(buf3), &t3), 0);
    input_trace_free(&t3);
    return 0;
}
