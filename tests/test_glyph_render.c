/*
 * tests/test_glyph_render.c — the GDI text renderer (src/glyph_render.c).
 *
 * Exercises glyph_grid_render (FUN_0048e200's GDI branch) and, through it,
 * glyph_row_draw (FUN_0048e860) against a recording glyph_gdi_ops stub: the
 * row × column walk, glyph positions, the drop-shadow pre-pass, colour
 * selection (normal / focused / disabled / overrides), the monospace
 * right-align shift, the sel2/stride paging, and the ruby no-op gate.
 *
 * On the 32-bit target the renderer's `this` is a menu_node whose embedded
 * menu_ctrl aliases it at +0x00 (so +0x174..+0x17c name both the container
 * arrays and, beyond +0x180, the node's display config).  The 64-bit host
 * has divergent layouts for the two structs, so these tests keep them
 * separate: a menu_ctrl owns the built container + laid-out text, and a
 * menu_node references its arrays (ctrl_list/entries/rows) and carries the
 * display config the renderer reads.  This mirrors exactly what the renderer
 * sees on-target without aliasing two incompatible layouts.
 */
#include "t.h"
#include "glyph_render.h"
#include "glyph_text.h"
#include "menu_list.h"

#include <string.h>

/* ─── recording glyph_gdi_ops stub ──────────────────────────────────────
 * Records every text_out with the font/colour current at emit time. */
#define MAX_OPS 64
typedef struct {
    int      x, y, len;
    char     str[8];
    uint32_t color;
    void    *font;
} rec_op;

static rec_op   g_ops[MAX_OPS];
static int      g_nops;
static uint32_t g_cur_color;
static void    *g_cur_font;

static void rec_select_font(void *u, void *font)  { (void)u; g_cur_font = font; }
static void rec_set_color(void *u, uint32_t c)     { (void)u; g_cur_color = c; }
static void rec_text_out(void *u, int x, int y, const char *s, int len)
{
    (void)u;
    if (g_nops < MAX_OPS) {
        rec_op *o = &g_ops[g_nops++];
        o->x = x; o->y = y; o->len = len;
        o->color = g_cur_color; o->font = g_cur_font;
        int n = (len < 7) ? len : 7;
        if (n < 0) n = 0;
        memcpy(o->str, s, (size_t)n);
        o->str[n] = 0;
    }
}

static glyph_gdi_ops recorder(void)
{
    glyph_gdi_ops ops;
    ops.select_font    = rec_select_font;
    ops.set_text_color = rec_set_color;
    ops.text_out       = rec_text_out;
    ops.user           = NULL;
    return ops;
}

static void reset_recorder(void)
{
    g_nops = 0; g_cur_color = 0; g_cur_font = NULL;
    memset(g_ops, 0, sizeof(g_ops));
}

/* Sentinel display-config colours (distinct so a recorded op names its
 * source unambiguously). */
#define COL_TEXT_NORMAL   0x00111111u   /* node.color0  +0x180 */
#define COL_SHADOW        0x00222222u   /* node.color1  +0x184 */
#define COL_SEC_NORMAL    0x00333333u   /* node.label0  +0x188 */
#define COL_TEXT_FOCUS    0x00444444u   /* node.color2  +0x18c */
#define COL_SEC_FOCUS     0x00555555u   /* node.color3  +0x190 */
#define COL_TEXT_DISABLED 0x00666666u   /* node.label1  +0x194 */
#define COL_SEC_DISABLED  0x00777777u   /* node.label2  +0x198 */

/* Build a 1-page menu: `c` owns the container (`cols` columns, `active_rows`
 * selectable rows with flag8 = 1, stride = `stride`, type 0 linear); `n` is
 * the render node referencing c's arrays, base at (bx, by), line pitch 0x1c,
 * display config seeded with the sentinels above.  Text is laid into `c`. */
static void build_menu(menu_node *n, menu_ctrl *c, int rows_cap, int cols,
                       int active_rows, int stride, int bx, int by)
{
    memset(c, 0, sizeof(*c));
    menu_ctrl_build(c, 0, 0, rows_cap, cols, stride, /*type=*/0);
    c->list->count = active_rows;
    for (int r = 0; r < active_rows; r++)
        c->rows[r].flag8 = 1;          /* selectable (spawn-block convention) */

    memset(n, 0, sizeof(*n));
    n->ctrl_list    = c->list;
    n->ctrl_entries = c->entries;
    n->ctrl_rows    = c->rows;
    n->field_c   = bx;                 /* x base contribution */
    n->field_10  = by;                 /* y base contribution */
    n->field_14  = 0;                  /* no ruby pass */
    n->field_1ac = 0x1c;               /* row pitch (28) */
    n->color0 = COL_TEXT_NORMAL;
    n->color1 = COL_SHADOW;
    n->label0 = COL_SEC_NORMAL;
    n->color2 = COL_TEXT_FOCUS;
    n->color3 = COL_SEC_FOCUS;
    n->label1 = COL_TEXT_DISABLED;
    n->label2 = COL_SEC_DISABLED;
}

/* ─── glyph_row_draw (FUN_0048e860) standalone ──────────────────────────── */

int test_glyph_row_draw_ascii_advance(void)
{
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 1, 1, 4, 0, 0);
    glyph_cell_layout(&c, 0, 0, "Hi");
    glyph_buf *buf = (glyph_buf *)c.rows[0].cells[0].obj0;
    T_ASSERT(buf != NULL);

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_row_draw(buf, &ops, 100, 50, 7, 0, 0);

    T_ASSERT_EQ_I(g_nops, 2);
    T_ASSERT_EQ_I(g_ops[0].x, 100);  T_ASSERT_EQ_I(g_ops[0].y, 50);
    T_ASSERT(strcmp(g_ops[0].str, "H") == 0);
    T_ASSERT_EQ_I(g_ops[1].x, 107);  T_ASSERT_EQ_I(g_ops[1].y, 50);
    T_ASSERT(strcmp(g_ops[1].str, "i") == 0);

    menu_ctrl_clear(&c);
    return 0;
}

int test_glyph_row_draw_sjis_double_advance(void)
{
    /* An SJIS lead advances the pen by 2·7 = 14 (its record is 2 bytes). */
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 1, 1, 4, 0, 0);
    glyph_cell_layout(&c, 0, 0, "\x82\xa0" "Z");  /* SJIS 'あ' + 'Z' */
    glyph_buf *buf = (glyph_buf *)c.rows[0].cells[0].obj0;

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_row_draw(buf, &ops, 0, 0, 7, 0, 0);

    T_ASSERT_EQ_I(g_nops, 2);
    T_ASSERT_EQ_I(g_ops[0].x, 0);    /* SJIS glyph at 0 */
    T_ASSERT_EQ_I(g_ops[0].len, 2);
    T_ASSERT_EQ_I(g_ops[1].x, 14);   /* 'Z' after the 2-byte glyph: 2·7 */
    T_ASSERT(strcmp(g_ops[1].str, "Z") == 0);

    menu_ctrl_clear(&c);
    return 0;
}

/* ─── glyph_grid_render (FUN_0048e200, GDI branch) ──────────────────────── */

int test_grid_render_normal_row_shadow_and_text(void)
{
    /* One selectable, non-focused row: drop shadow (down 1, right 1) in the
     * shadow colour, then the glyphs in the normal text colour. */
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 1, 1, 4, 10, 20);      /* base (10,20) */
    c.list->cursor = -1;                          /* no row focused → normal colour */
    glyph_cell_layout(&c, 0, 0, "Hi");

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)0xF0, (void *)0xF1);

    /* 2 shadow copies × 2 glyphs + 1 main × 2 glyphs = 6 text_out. */
    T_ASSERT_EQ_I(g_nops, 6);

    /* shadow pass 1: (cx, cy+1) */
    T_ASSERT_EQ_I(g_ops[0].x, 10); T_ASSERT_EQ_I(g_ops[0].y, 21);
    T_ASSERT_EQ_U(g_ops[0].color, COL_SHADOW);
    T_ASSERT_EQ_P(g_ops[0].font, (void *)0xF0);
    T_ASSERT_EQ_I(g_ops[1].x, 17); T_ASSERT_EQ_I(g_ops[1].y, 21);
    /* shadow pass 2: (cx+1, cy) */
    T_ASSERT_EQ_I(g_ops[2].x, 11); T_ASSERT_EQ_I(g_ops[2].y, 20);
    T_ASSERT_EQ_U(g_ops[2].color, COL_SHADOW);
    T_ASSERT_EQ_I(g_ops[3].x, 18); T_ASSERT_EQ_I(g_ops[3].y, 20);
    /* main pass: (cx, cy) in the normal text colour */
    T_ASSERT_EQ_I(g_ops[4].x, 10); T_ASSERT_EQ_I(g_ops[4].y, 20);
    T_ASSERT_EQ_U(g_ops[4].color, COL_TEXT_NORMAL);
    T_ASSERT(strcmp(g_ops[4].str, "H") == 0);
    T_ASSERT_EQ_I(g_ops[5].x, 17); T_ASSERT_EQ_I(g_ops[5].y, 20);
    T_ASSERT(strcmp(g_ops[5].str, "i") == 0);

    menu_ctrl_clear(&c);
    return 0;
}

int test_grid_render_focused_row_uses_focus_colour(void)
{
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 1, 2, 4, 0, 0);
    glyph_cell_layout(&c, 0, 0, "A");
    glyph_cell_layout(&c, 1, 0, "B");
    c.list->cursor = 1;                           /* focus row 1 */

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)1, (void *)2);

    /* Row 0 (not focused): 3 ops, main in normal colour. Row 1 (focused):
     * 3 ops, main in focus colour. main pass is the 3rd op of each block. */
    T_ASSERT_EQ_I(g_nops, 6);
    T_ASSERT(strcmp(g_ops[2].str, "A") == 0);
    T_ASSERT_EQ_U(g_ops[2].color, COL_TEXT_NORMAL);
    T_ASSERT(strcmp(g_ops[5].str, "B") == 0);
    T_ASSERT_EQ_U(g_ops[5].color, COL_TEXT_FOCUS);
    /* Row pitch: row 1's glyphs sit 0x1c below row 0's. */
    T_ASSERT_EQ_I(g_ops[2].y, 0);
    T_ASSERT_EQ_I(g_ops[5].y, 0x1c);

    menu_ctrl_clear(&c);
    return 0;
}

int test_grid_render_disabled_row_no_shadow(void)
{
    /* A non-selectable row (flag8 == 0) draws plain text in the disabled
     * colour with no drop shadow. */
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 1, 1, 4, 0, 0);
    c.rows[0].flag8 = 0;                          /* override: disabled */
    glyph_cell_layout(&c, 0, 0, "X");

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)1, (void *)2);

    T_ASSERT_EQ_I(g_nops, 1);                      /* main pass only */
    T_ASSERT(strcmp(g_ops[0].str, "X") == 0);
    T_ASSERT_EQ_U(g_ops[0].color, COL_TEXT_DISABLED);

    menu_ctrl_clear(&c);
    return 0;
}

int test_grid_render_two_columns_positions(void)
{
    /* Two columns: entry.pos = col·0x20, so column 1 starts 0x20 right. */
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 2, 1, 4, 0, 0);
    glyph_cell_layout(&c, 0, 0, "P");             /* col 0 */
    glyph_cell_layout(&c, 0, 1, "Q");             /* col 1 */

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)1, (void *)2);

    /* 2 columns × (2 shadow + 1 main) = 6 ops; main passes are ops[2], [5]. */
    T_ASSERT_EQ_I(g_nops, 6);
    T_ASSERT(strcmp(g_ops[2].str, "P") == 0); T_ASSERT_EQ_I(g_ops[2].x, 0);
    T_ASSERT(strcmp(g_ops[5].str, "Q") == 0); T_ASSERT_EQ_I(g_ops[5].x, 0x20);

    menu_ctrl_clear(&c);
    return 0;
}

int test_grid_render_sel2_and_stride_paging(void)
{
    /* sel2 = 1 starts the walk at row 1; stride = 1 draws a single row. */
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 8, 1, 4, 1, 0, 0);
    glyph_cell_layout(&c, 0, 0, "0");
    glyph_cell_layout(&c, 1, 0, "1");
    glyph_cell_layout(&c, 2, 0, "2");
    c.list->sel2 = 1;

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)1, (void *)2);

    /* Only row 1 ("1") drawn: 2 shadow + 1 main = 3 ops, and the screen row
     * is 0 (disp resets at sel2), so y = 0. */
    T_ASSERT_EQ_I(g_nops, 3);
    T_ASSERT(strcmp(g_ops[2].str, "1") == 0);
    T_ASSERT_EQ_I(g_ops[2].y, 0);

    menu_ctrl_clear(&c);
    return 0;
}

int test_grid_render_monospace_right_align(void)
{
    /* entry.field_c == 1 right-aligns within entry.extent: shift the column
     * by max(0, extent − 7·byte_len). */
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 1, 1, 4, 10, 0);
    glyph_cell_layout(&c, 0, 0, "Hi");            /* byte_len 2 */
    c.entries[0].field_c = 1;
    c.entries[0].extent  = 50;                    /* 50 − 14 = 36 */

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)1, (void *)2);

    /* base cx = entry.pos(0) + x(0) + field_c(10) = 10; +36 shift = 46. */
    T_ASSERT(strcmp(g_ops[4].str, "H") == 0);
    T_ASSERT_EQ_I(g_ops[4].x, 46);

    menu_ctrl_clear(&c);
    return 0;
}

int test_grid_render_ruby_gate_noop_for_raw_text(void)
{
    /* node->field_14 != 0 enables the ruby pass, but raw text has flag1c == 0
     * on every record, so glyph_ruby_draw emits nothing: the op count matches
     * the no-ruby case (6 for "Hi"). */
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 1, 1, 4, 0, 0);
    n.field_14 = 1;                               /* enable ruby pass */
    glyph_cell_layout(&c, 0, 0, "Hi");

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)1, (void *)2);

    T_ASSERT_EQ_I(g_nops, 6);   /* 2 shadow + 1 main, no ruby output */

    menu_ctrl_clear(&c);
    return 0;
}

int test_grid_render_empty_cell_skipped(void)
{
    /* A cell with no glyph buffer (obj0 NULL) is skipped entirely. */
    menu_node n; menu_ctrl c;
    build_menu(&n, &c, 4, 2, 1, 4, 0, 0);
    glyph_cell_layout(&c, 0, 1, "R");             /* only col 1 has text */

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)1, (void *)2);

    /* col 0 (NULL obj0) skipped; only col 1 drawn: 2 shadow + 1 main = 3. */
    T_ASSERT_EQ_I(g_nops, 3);
    T_ASSERT(strcmp(g_ops[2].str, "R") == 0);

    menu_ctrl_clear(&c);
    return 0;
}

int test_grid_render_null_list_noop(void)
{
    /* No list header → no-op (defensive; retail would deref). */
    menu_node n;
    memset(&n, 0, sizeof(n));
    n.ctrl_list = NULL;

    reset_recorder();
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&n, &ops, 0, 0, (void *)1, (void *)2);

    T_ASSERT_EQ_I(g_nops, 0);
    return 0;
}
