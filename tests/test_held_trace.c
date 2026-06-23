/*
 * tests/test_held_trace.c — host tests for the port-side held-axis replay
 * (src/held_trace.c): the {"frame":N,"keys":[..]} JSONL parser (scancodes +
 * direction names) and the LEVEL replay that rebuilds mgr->axis_held each
 * frame.
 *
 * The replay's contract is "the four managed direction slots reflect exactly
 * the current held set, held until the next entry changes it" — the level
 * counterpart of input_trace's one-shot ring presses.
 */
#include "t.h"
#include "held_trace.h"

#include <string.h>

/* ── scancode → slot mapping ── */
int test_held_scancode_slot(void)
{
    T_ASSERT_EQ_I(held_scancode_slot(HELD_DIK_UP),    0);
    T_ASSERT_EQ_I(held_scancode_slot(HELD_DIK_DOWN),  1);
    T_ASSERT_EQ_I(held_scancode_slot(HELD_DIK_LEFT),  2);
    T_ASSERT_EQ_I(held_scancode_slot(HELD_DIK_RIGHT), 3);
    T_ASSERT_EQ_I(held_scancode_slot(0x39),          -1);   /* space — not a dir */
    T_ASSERT_EQ_I(held_scancode_slot(0),             -1);
    return 0;
}

/* ── parse: numeric scancodes, empty array, key order ── */
int test_held_trace_parse_basic(void)
{
    const char *buf =
        "{\"frame\":0, \"keys\":[]}\n"
        "{\"frame\":30, \"keys\":[0xcb]}\n"
        "{\"keys\":[0xc8, 0xcd], \"frame\":31}\n";   /* keys before frame */
    struct held_trace t;
    T_ASSERT_EQ_I(held_trace_parse_buf(buf, strlen(buf), &t), 1);
    T_ASSERT_EQ_U(t.count, 3);
    T_ASSERT_EQ_U(t.entries[0].frame, 0);
    T_ASSERT_EQ_U(t.entries[0].n_keys, 0);
    T_ASSERT_EQ_U(t.entries[1].frame, 30);
    T_ASSERT_EQ_U(t.entries[1].n_keys, 1);
    T_ASSERT_EQ_I(t.entries[1].keys[0], HELD_DIK_LEFT);
    T_ASSERT_EQ_U(t.entries[2].n_keys, 2);
    T_ASSERT_EQ_I(t.entries[2].keys[0], HELD_DIK_UP);
    T_ASSERT_EQ_I(t.entries[2].keys[1], HELD_DIK_RIGHT);
    held_trace_free(&t);
    return 0;
}

/* ── parse: direction names resolve to scancodes; comments/blanks tolerated ── */
int test_held_trace_parse_names(void)
{
    const char *buf =
        "# walk left then up-right\n"
        "\n"
        "  {\"frame\":5, \"keys\":[\"left\"]}  \n"
        "{\"frame\":9, \"keys\":[\"up\",\"right\"]}\n"
        "{\"frame\":12}\n";                        /* no keys → release */
    struct held_trace t;
    T_ASSERT_EQ_I(held_trace_parse_buf(buf, strlen(buf), &t), 1);
    T_ASSERT_EQ_U(t.count, 3);
    T_ASSERT_EQ_U(t.entries[0].frame, 5);
    T_ASSERT_EQ_U(t.entries[0].n_keys, 1);
    T_ASSERT_EQ_I(t.entries[0].keys[0], HELD_DIK_LEFT);
    T_ASSERT_EQ_U(t.entries[1].n_keys, 2);
    T_ASSERT_EQ_I(t.entries[1].keys[0], HELD_DIK_UP);
    T_ASSERT_EQ_I(t.entries[1].keys[1], HELD_DIK_RIGHT);
    T_ASSERT_EQ_U(t.entries[2].n_keys, 0);
    held_trace_free(&t);
    return 0;
}

/* ── parse: out-of-order frames fail (first valid entry survives) ── */
int test_held_trace_parse_out_of_order_fails(void)
{
    const char *buf = "{\"frame\":10,\"keys\":[\"up\"]}\n{\"frame\":9,\"keys\":[\"up\"]}\n";
    struct held_trace t;
    T_ASSERT_EQ_I(held_trace_parse_buf(buf, strlen(buf), &t), 0);
    T_ASSERT_EQ_U(t.count, 1);
    held_trace_free(&t);
    return 0;
}

/* ── parse: malformed inputs fail ── */
int test_held_trace_parse_malformed_fails(void)
{
    /* trailing comma in array */
    const char *a = "{\"frame\":1,\"keys\":[\"up\",]}\n";
    struct held_trace t; T_ASSERT_EQ_I(held_trace_parse_buf(a, strlen(a), &t), 0);
    held_trace_free(&t);

    /* unknown object key */
    const char *b = "{\"frame\":1,\"bogus\":2}\n";
    struct held_trace t2; T_ASSERT_EQ_I(held_trace_parse_buf(b, strlen(b), &t2), 0);
    held_trace_free(&t2);

    /* unknown direction name */
    const char *c = "{\"frame\":1,\"keys\":[\"diagonal\"]}\n";
    struct held_trace t3; T_ASSERT_EQ_I(held_trace_parse_buf(c, strlen(c), &t3), 0);
    held_trace_free(&t3);
    return 0;
}

/* ── replay: the held set is a LEVEL — set, persist across frames, release ── */
int test_held_trace_replay_levels(void)
{
    const char *buf =
        "{\"frame\":0, \"keys\":[]}\n"
        "{\"frame\":2, \"keys\":[\"left\"]}\n"
        "{\"frame\":5, \"keys\":[\"up\",\"right\"]}\n"
        "{\"frame\":8, \"keys\":[]}\n";
    struct held_trace t;
    T_ASSERT_EQ_I(held_trace_parse_buf(buf, strlen(buf), &t), 1);

    input_mgr m; memset(&m, 0, sizeof m);
    /* pre-dirty the managed slots so we prove replay CLEARS them */
    m.axis_held[0] = 7; m.axis_held[1] = 7; m.axis_held[2] = 7; m.axis_held[3] = 7;

    /* frame 0: empty held set → all four slots cleared. */
    held_trace_replay(&t, 0, 0, &m);
    T_ASSERT_EQ_I(m.axis_held[0], 0);
    T_ASSERT_EQ_I(m.axis_held[1], 0);
    T_ASSERT_EQ_I(m.axis_held[2], 0);
    T_ASSERT_EQ_I(m.axis_held[3], 0);

    /* frame 2: LEFT held → slot 2 only. */
    held_trace_replay(&t, 2, 2, &m);
    T_ASSERT_EQ_I(m.axis_held[2], 1);
    T_ASSERT_EQ_I(m.axis_held[0], 0);

    /* frames 3,4: level persists (no new entry) — LEFT still held. */
    held_trace_replay(&t, 3, 3, &m);
    T_ASSERT_EQ_I(m.axis_held[2], 1);
    held_trace_replay(&t, 4, 4, &m);
    T_ASSERT_EQ_I(m.axis_held[2], 1);

    /* frame 5: UP+RIGHT → slots 0 and 3; LEFT (slot 2) cleared. */
    held_trace_replay(&t, 5, 5, &m);
    T_ASSERT_EQ_I(m.axis_held[0], 1);
    T_ASSERT_EQ_I(m.axis_held[3], 1);
    T_ASSERT_EQ_I(m.axis_held[2], 0);
    T_ASSERT_EQ_I(m.axis_held[1], 0);

    /* frame 8: release-all → every managed slot back to 0. */
    held_trace_replay(&t, 8, 8, &m);
    T_ASSERT_EQ_I(m.axis_held[0], 0);
    T_ASSERT_EQ_I(m.axis_held[1], 0);
    T_ASSERT_EQ_I(m.axis_held[2], 0);
    T_ASSERT_EQ_I(m.axis_held[3], 0);

    held_trace_free(&t);
    return 0;
}

/* ── replay: a jump past several entries lands on the latest level ── */
int test_held_trace_replay_catches_up(void)
{
    const char *buf =
        "{\"frame\":1, \"keys\":[\"left\"]}\n"
        "{\"frame\":2, \"keys\":[\"down\"]}\n"
        "{\"frame\":3, \"keys\":[\"right\"]}\n";
    struct held_trace t;
    T_ASSERT_EQ_I(held_trace_parse_buf(buf, strlen(buf), &t), 1);

    input_mgr m; memset(&m, 0, sizeof m);
    /* jump straight to frame 9: only the LAST entry's level (RIGHT) is active. */
    held_trace_replay(&t, 9, 9, &m);
    T_ASSERT_EQ_U(t.cursor, 3);
    T_ASSERT_EQ_I(m.axis_held[3], 1);   /* right */
    T_ASSERT_EQ_I(m.axis_held[2], 0);   /* left  cleared */
    T_ASSERT_EQ_I(m.axis_held[1], 0);   /* down  cleared */
    held_trace_free(&t);
    return 0;
}

/* ── replay: NULL mgr / empty trace are safe no-ops ── */
int test_held_trace_replay_guards(void)
{
    struct held_trace t; memset(&t, 0, sizeof t);
    held_trace_replay(&t, 10, 10, NULL);          /* NULL mgr */

    input_mgr m; memset(&m, 0, sizeof m);
    m.axis_held[2] = 9;
    held_trace_replay(&t, 10, 10, &m);            /* empty trace still clears slots */
    T_ASSERT_EQ_I(m.axis_held[2], 0);
    T_ASSERT_EQ_U(t.cursor, 0);
    held_trace_free(&t);
    held_trace_free(&t);                      /* double free is safe */
    return 0;
}
