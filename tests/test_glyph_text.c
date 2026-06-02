/*
 * tests/test_glyph_text.c — the cell-grid glyph layout builder
 * (src/glyph_text.c).  Covers glyph_token_search (SJIS-aware substring
 * search) and glyph_cell_layout (string → cell.obj0 glyph records).
 */
#include "t.h"
#include "glyph_text.h"

#include <string.h>

/* ─── glyph_token_search (FUN_0040fd20) ─────────────────────────────── */

int test_glyph_token_search_found(void)
{
    T_ASSERT_EQ_I(glyph_token_search("hello#world", "#", 0), 5);
    T_ASSERT_EQ_I(glyph_token_search("##", "#", 0), 0);
    return 0;
}

int test_glyph_token_search_absent(void)
{
    T_ASSERT_EQ_I(glyph_token_search("hello", "#", 0), -1);
    return 0;
}

int test_glyph_token_search_empty_needle(void)
{
    /* Retail returns `start` for a zero-length needle. */
    T_ASSERT_EQ_I(glyph_token_search("hello", "", 0), 0);
    T_ASSERT_EQ_I(glyph_token_search("hello", "", 3), 3);
    return 0;
}

int test_glyph_token_search_start_offset(void)
{
    /* First '#' at index 1 is skipped; search from 2 finds the one at 3. */
    T_ASSERT_EQ_I(glyph_token_search("a#b#c", "#", 2), 3);
    return 0;
}

int test_glyph_token_search_skips_sjis_trail(void)
{
    /* The 0x23 ('#') byte sits in an SJIS trail position; the scan must
     * skip it (advance 2 over the 0x82 lead) and not report a false hit. */
    T_ASSERT_EQ_I(glyph_token_search("\x82\x23X", "#", 0), -1);
    /* But a genuine '#' after the SJIS pair is found at its real offset. */
    T_ASSERT_EQ_I(glyph_token_search("\x82\x23X#", "#", 0), 3);
    return 0;
}

/* ─── glyph_cell_layout (FUN_0040fa00) ──────────────────────────────── */

/* Build a grid with `cols` columns, `rows_cap` row capacity, and pretend
 * `active_rows` rows have been appended (0x40f800 bumps hdr->count; not yet
 * ported, so set it directly to satisfy the layout bounds check). */
static void build_grid(menu_ctrl *c, int rows_cap, int cols, int active_rows)
{
    memset(c, 0, sizeof(*c));
    menu_ctrl_build(c, 0, 0, rows_cap, cols, cols, 0);
    c->list->count = active_rows;
}

int test_glyph_layout_ascii(void)
{
    menu_ctrl c;
    build_grid(&c, 4, 2, 2);

    glyph_cell_layout(&c, 0, 0, "Start");

    glyph_buf *o = (glyph_buf *)c.rows[0].cells[0].obj0;
    T_ASSERT(o != NULL);
    T_ASSERT_EQ_I(o->cap, GLYPH_BUF_CAP);
    T_ASSERT_EQ_U(o->len, 5);
    T_ASSERT_EQ_U(o->count, 5);

    glyph_record *r = (glyph_record *)o->records;
    const char *want = "Start";
    for (int i = 0; i < 5; i++) {
        T_ASSERT_EQ_I(r[i].ch[0], want[i]);
        T_ASSERT_EQ_I(r[i].ch[1], 0);     /* ASCII record is NUL-terminated */
        T_ASSERT_EQ_U(r[i].flag1c, 0);    /* raw glyph, not escape-consumed */
    }

    menu_ctrl_clear(&c);
    return 0;
}

int test_glyph_layout_sjis(void)
{
    menu_ctrl c;
    build_grid(&c, 4, 2, 2);

    /* SJIS 'あ' (0x82 0xa0) then ASCII 'A' → 3 bytes, 2 glyph records. */
    glyph_cell_layout(&c, 1, 0, "\x82\xa0" "A");

    glyph_buf *o = (glyph_buf *)c.rows[1].cells[0].obj0;
    T_ASSERT(o != NULL);
    T_ASSERT_EQ_U(o->len, 3);     /* byte length */
    T_ASSERT_EQ_U(o->count, 2);   /* glyph count */

    glyph_record *r = (glyph_record *)o->records;
    T_ASSERT_EQ_I((unsigned char)r[0].ch[0], 0x82);
    T_ASSERT_EQ_I((unsigned char)r[0].ch[1], 0xa0);
    T_ASSERT_EQ_I(r[0].ch[2], 0);     /* double-byte record NUL-terminated */
    T_ASSERT_EQ_I(r[1].ch[0], 'A');
    T_ASSERT_EQ_I(r[1].ch[1], 0);

    menu_ctrl_clear(&c);
    return 0;
}

int test_glyph_layout_lazy_alloc_then_reuse(void)
{
    menu_ctrl c;
    build_grid(&c, 4, 2, 2);

    T_ASSERT_EQ_P(c.rows[0].cells[0].obj0, NULL);   /* nothing built yet */

    glyph_cell_layout(&c, 0, 0, "Hello");
    glyph_buf *first = (glyph_buf *)c.rows[0].cells[0].obj0;
    T_ASSERT(first != NULL);
    T_ASSERT_EQ_U(first->count, 5);

    /* A second layout reuses the same obj0 buffer and overwrites it. */
    glyph_cell_layout(&c, 0, 0, "Hi");
    T_ASSERT_EQ_P(c.rows[0].cells[0].obj0, first);
    T_ASSERT_EQ_U(first->len, 2);
    T_ASSERT_EQ_U(first->count, 2);

    menu_ctrl_clear(&c);
    return 0;
}

int test_glyph_layout_empty_string(void)
{
    menu_ctrl c;
    build_grid(&c, 4, 2, 2);

    glyph_cell_layout(&c, 0, 0, "");
    glyph_buf *o = (glyph_buf *)c.rows[0].cells[0].obj0;
    T_ASSERT(o != NULL);              /* obj0 still allocated */
    T_ASSERT_EQ_U(o->count, 0);
    T_ASSERT_EQ_U(o->len, 0);

    menu_ctrl_clear(&c);
    return 0;
}

int test_glyph_layout_overlong_drops_text(void)
{
    menu_ctrl c;
    build_grid(&c, 4, 2, 2);

    /* >= cap (0x50) chars: retail allocates obj0 but skips the build. */
    char big[GLYPH_BUF_CAP + 8];
    memset(big, 'x', sizeof(big) - 1);
    big[sizeof(big) - 1] = 0;

    glyph_cell_layout(&c, 0, 0, big);
    glyph_buf *o = (glyph_buf *)c.rows[0].cells[0].obj0;
    T_ASSERT(o != NULL);             /* lazy alloc runs before the length gate */
    T_ASSERT_EQ_U(o->count, 0);      /* but nothing is laid out */
    T_ASSERT_EQ_U(o->len, 0);

    menu_ctrl_clear(&c);
    return 0;
}

int test_glyph_layout_bounds_noop(void)
{
    menu_ctrl c;
    build_grid(&c, 4, 2, 1);   /* only row 0 active */

    /* row >= count → no-op (no alloc on the out-of-range row). */
    glyph_cell_layout(&c, 3, 0, "x");
    T_ASSERT_EQ_P(c.rows[3].cells[0].obj0, NULL);

    /* col >= column-count → no-op (returns before indexing the cell). */
    glyph_cell_layout(&c, 0, 2, "x");
    T_ASSERT_EQ_P(c.rows[0].cells[0].obj0, NULL);

    /* negative row → no-op. */
    glyph_cell_layout(&c, -1, 0, "x");
    T_ASSERT_EQ_P(c.rows[0].cells[0].obj0, NULL);

    menu_ctrl_clear(&c);
    return 0;
}

/* The escape-expansion hook fires once, after the raw pass, with the source
 * byte length. */
static int      g_escape_calls;
static uint16_t g_escape_len;
static glyph_buf *g_escape_obj0;

static void recording_escape_hook(glyph_buf *o, const char *s, uint16_t n)
{
    (void)s;
    g_escape_calls++;
    g_escape_len = n;
    g_escape_obj0 = o;
}

int test_glyph_layout_escape_hook_fires(void)
{
    menu_ctrl c;
    build_grid(&c, 4, 2, 2);

    g_escape_calls = 0;
    g_escape_len = 0;
    g_escape_obj0 = NULL;
    glyph_escape_expand_hook = recording_escape_hook;

    glyph_cell_layout(&c, 0, 0, "abc");

    glyph_escape_expand_hook = NULL;   /* restore default for other tests */

    T_ASSERT_EQ_I(g_escape_calls, 1);
    T_ASSERT_EQ_U(g_escape_len, 3);
    T_ASSERT_EQ_P(g_escape_obj0, c.rows[0].cells[0].obj0);

    menu_ctrl_clear(&c);
    return 0;
}
