/*
 * tests/test_main.c — unit-test driver for OpenSummoners ports.
 *
 *   make -C tests run             # all tests
 *   ./tests/build/run_tests       # invoke directly (after first build)
 *
 * Outcomes: 0 pass, 1 fail (with message), 2 skip.  Process exit is 0
 * iff failure count is 0.  Skips don't count as failures.
 *
 * Registration: add one X(name) line to TESTS for every test.  The
 * X-macro expands twice — once for the extern decl, once for the
 * dispatch table — so the test name appears exactly once and can't
 * drift from the declaration.
 */
#include "t.h"

typedef int (*test_fn)(void);

struct test_case {
    const char *name;
    test_fn     fn;
};

/* ─── test registry ───────────────────────────────────────────────────
 * One X(n) per test.  Grouped by ported module for readability.
 */
#define TESTS \
    X(pd_channel_mask_to_shift_rgb565_R)                       \
    X(pd_channel_mask_to_shift_rgb565_G)                       \
    X(pd_channel_mask_to_shift_rgb565_B)                       \
    X(pd_channel_mask_to_shift_rgb555)                         \
    X(pd_channel_mask_to_shift_zero_mask)                      \
    X(pd_channel_mask_to_shift_single_bit_at_0)                \
    X(pd_channel_init_zeroes_and_seeds_weight)                 \
    X(pd_channel_free_lut_when_allocated)                      \
    X(pd_channel_free_lut_when_not_allocated)                  \
    X(pd_blend_init_zeroes_state_and_inits_channels)           \
    X(pd_blend_set_color_writes_per_channel_weights)           \
    X(pd_blend_set_color_does_not_touch_other_fields)          \
    X(pd_blend_layout_matches_retail_offsets)                  \

#define X(n) extern int test_##n(void);
TESTS
#undef X

static const struct test_case TESTS_TABLE[] = {
#define X(n) { #n, test_##n },
    TESTS
#undef X
};
static const size_t N_TESTS = sizeof(TESTS_TABLE) / sizeof(TESTS_TABLE[0]);

int main(int argc, char **argv)
{
    const char *filter = (argc >= 2) ? argv[1] : NULL;
    int pass = 0, fail = 0, skip = 0;

    for (size_t i = 0; i < N_TESTS; i++) {
        const struct test_case *t = &TESTS_TABLE[i];
        if (filter && !strstr(t->name, filter)) continue;
        fprintf(stderr, "[%zu/%zu] %s\n", i + 1, N_TESTS, t->name);
        int rc = t->fn();
        if      (rc == 0) pass++;
        else if (rc == 2) skip++;
        else              fail++;
    }

    fprintf(stderr, "\nresult: %d pass, %d fail, %d skip (of %zu)\n",
            pass, fail, skip, N_TESTS);
    return fail == 0 ? 0 : 1;
}
