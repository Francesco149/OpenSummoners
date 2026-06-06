/*
 * test_letterbox.c — host tests for the establishing-shot cinematic letterbox
 * (src/letterbox.c; the grid-fill slice of FUN_0048c150 @ 0x48c150:124-162).
 *
 * The retail blit trace (/tmp/blit_town_retail, flip 1500) IS the spec: res
 * 0x583 (64x4) tiled as 320 blits = 160 BOTTOM-bar (dy 416..476) then 160
 * TOP-bar (dy 0..60), each row 10 columns dx in {0,64,…,576}.
 */
#include "letterbox.h"
#include "t.h"

#include <string.h>

/* ---- a recording blit sink ------------------------------------------------ */

typedef struct { int x, y; } lb_xy;
typedef struct { lb_xy r[512]; int n; } lb_log;

static void rec_blit(void *ctx, int x, int y)
{
    lb_log *L = (lb_log *)ctx;
    if (L->n < (int)(sizeof L->r / sizeof L->r[0])) {
        L->r[L->n].x = x;
        L->r[L->n].y = y;
    }
    L->n++;
}

/* ---- the opening-town letterbox (64/64) — bit-exact vs retail ------------- */

int test_letterbox_town_intro_grid(void)
{
    lb_log L; memset(&L, 0, sizeof L);
    letterbox_render(LETTERBOX_INTRO_BAR, LETTERBOX_INTRO_BAR, rec_blit, &L);

    /* 10 cols x 16 rows x 2 bars = 320. */
    T_ASSERT_EQ_I(L.n, 320);

    /* Engine order: BOTTOM bar first (dy 416..476), TOP bar second (dy 0..60). */
    T_ASSERT_EQ_I(L.r[0].x, 0);     T_ASSERT_EQ_I(L.r[0].y, 416);
    T_ASSERT_EQ_I(L.r[9].x, 576);   T_ASSERT_EQ_I(L.r[9].y, 416);   /* row end */
    T_ASSERT_EQ_I(L.r[10].x, 0);    T_ASSERT_EQ_I(L.r[10].y, 420);  /* next row */
    T_ASSERT_EQ_I(L.r[159].x, 576); T_ASSERT_EQ_I(L.r[159].y, 476); /* bottom end */
    T_ASSERT_EQ_I(L.r[160].x, 0);   T_ASSERT_EQ_I(L.r[160].y, 0);   /* top start */
    T_ASSERT_EQ_I(L.r[319].x, 576); T_ASSERT_EQ_I(L.r[319].y, 60);  /* top end */

    /* Every row is 10 columns at 64px pitch (0..576); rows step 4px. */
    for (int i = 0; i < L.n; i++) {
        int col = i % 10;
        T_ASSERT_EQ_I(L.r[i].x, col * 64);
    }
    /* Bottom bar covers rows 416..479 (16 rows of 4px); top covers 0..63. */
    for (int i = 0; i < 160; i++) {
        int dy = L.r[i].y;
        T_ASSERT(dy >= 416 && dy <= 476 && (dy - 416) % 4 == 0);
    }
    for (int i = 160; i < 320; i++) {
        int dy = L.r[i].y;
        T_ASSERT(dy >= 0 && dy <= 60 && dy % 4 == 0);
    }
    return 0;
}

/* ---- absent bars ---------------------------------------------------------- */

int test_letterbox_zero_bars(void)
{
    lb_log L; memset(&L, 0, sizeof L);
    letterbox_render(0, 0, rec_blit, &L);
    T_ASSERT_EQ_I(L.n, 0);

    /* one bar only */
    memset(&L, 0, sizeof L);
    letterbox_render(0, LETTERBOX_INTRO_BAR, rec_blit, &L);
    T_ASSERT_EQ_I(L.n, 160);                 /* bottom only */
    T_ASSERT_EQ_I(L.r[0].y, 416);

    memset(&L, 0, sizeof L);
    letterbox_render(LETTERBOX_INTRO_BAR, 0, rec_blit, &L);
    T_ASSERT_EQ_I(L.n, 160);                 /* top only */
    T_ASSERT_EQ_I(L.r[0].y, 0);
    return 0;
}

/* ---- NULL sink is a no-op (no crash) -------------------------------------- */

int test_letterbox_null_sink(void)
{
    letterbox_render(64, 64, NULL, NULL);    /* must not dereference */
    return 0;
}

/* ---- the round-up-to-4 behaviour (a non-multiple-of-4 bar) ----------------
 * Bottom: rounded = roundup4(h), base = 480 - h; rows base..base+rounded-4.
 * Top:    rounded = roundup4(h), base = h - rounded (<=0); rows base..base+rounded-4.
 * For h=63 (rounds to 64 => 16 rows): bottom base = 417 (dy 417..477);
 * top base = 63-64 = -1 (dy -1..59). Faithful to the decompile's arithmetic. */
int test_letterbox_round_up(void)
{
    lb_log L; memset(&L, 0, sizeof L);
    letterbox_render(0, 63, rec_blit, &L);          /* bottom, h=63 */
    T_ASSERT_EQ_I(L.n, 160);                         /* 16 rows still */
    T_ASSERT_EQ_I(L.r[0].y, 417);                    /* 480 - 63 */
    T_ASSERT_EQ_I(L.r[159].y, 417 + 60);             /* base + 15*4 = 477 */

    memset(&L, 0, sizeof L);
    letterbox_render(63, 0, rec_blit, &L);          /* top, h=63 */
    T_ASSERT_EQ_I(L.n, 160);
    T_ASSERT_EQ_I(L.r[0].y, -1);                     /* 63 - 64 */
    T_ASSERT_EQ_I(L.r[159].y, -1 + 60);              /* 59 */
    return 0;
}
