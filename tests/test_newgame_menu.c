/*
 * tests/test_newgame_menu.c — the new-game config menu builder
 * (src/newgame_menu.c).
 *
 * The renderer (glyph_grid_render) is already verified bit-exact against
 * retail's live TextOutA stream (ckpt 36, quirk #63).  The remaining gate was
 * the BUILDER: does the port construct the case-0x24 grid such that, run
 * through that renderer, it emits the SAME stream?  This test closes it.
 *
 * It builds the grid with newgame_config_build, renders the node through a
 * recording glyph_gdi_ops stub at the retail base (x=32, y=32 — the box
 * node's position), and asserts the recorded per-glyph TextOutA draws equal
 * the captured retail golden, draw-for-draw:
 *   tests/scenarios/new-game-through/goldens/retail-newgame-config-textout.jsonl
 * filtered to the menu region (y in 55..113 — excludes the separate tooltip
 * text node at y=416/444, which a different run-loop call renders).
 *
 * The GOLDEN_OPS table below is generated verbatim from that jsonl (129 draws:
 * 3 rows × {col0 label, col1 value} × {shadow-down, shadow-right, main}, with
 * the Start-Game row's empty value column skipped).  If the renderer or
 * builder regresses, the diff names the exact draw.
 */
#include "t.h"
#include "newgame_menu.h"
#include "glyph_render.h"
#include "glyph_text.h"
#include "menu_list.h"

#include <string.h>

/* ─── recording glyph_gdi_ops stub (sized for the full menu stream) ────── */
#define MAX_OPS 256
typedef struct { int x, y; uint32_t color; char str[8]; } rec_op;

static rec_op   g_ops[MAX_OPS];
static int      g_nops;
static uint32_t g_cur_color;

static void rec_select_font(void *u, void *f) { (void)u; (void)f; }
static void rec_set_color(void *u, uint32_t c) { (void)u; g_cur_color = c; }
static void rec_text_out(void *u, int x, int y, const char *s, int len)
{
    (void)u;
    if (g_nops < MAX_OPS) {
        rec_op *o = &g_ops[g_nops++];
        o->x = x; o->y = y; o->color = g_cur_color;
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

/* The captured retail menu stream (generated from the golden jsonl). */
static const rec_op GOLDEN_OPS[] = {
    { 72, 57,0xa8b9ccu,"G"},
    { 79, 57,0xa8b9ccu,"a"},
    { 86, 57,0xa8b9ccu,"m"},
    { 93, 57,0xa8b9ccu,"e"},
    {100, 57,0xa8b9ccu," "},
    {107, 57,0xa8b9ccu,"D"},
    {114, 57,0xa8b9ccu,"i"},
    {121, 57,0xa8b9ccu,"f"},
    {128, 57,0xa8b9ccu,"f"},
    {135, 57,0xa8b9ccu,"i"},
    {142, 57,0xa8b9ccu,"c"},
    {149, 57,0xa8b9ccu,"u"},
    {156, 57,0xa8b9ccu,"l"},
    {163, 57,0xa8b9ccu,"t"},
    {170, 57,0xa8b9ccu,"y"},
    { 73, 56,0xa8b9ccu,"G"},
    { 80, 56,0xa8b9ccu,"a"},
    { 87, 56,0xa8b9ccu,"m"},
    { 94, 56,0xa8b9ccu,"e"},
    {101, 56,0xa8b9ccu," "},
    {108, 56,0xa8b9ccu,"D"},
    {115, 56,0xa8b9ccu,"i"},
    {122, 56,0xa8b9ccu,"f"},
    {129, 56,0xa8b9ccu,"f"},
    {136, 56,0xa8b9ccu,"i"},
    {143, 56,0xa8b9ccu,"c"},
    {150, 56,0xa8b9ccu,"u"},
    {157, 56,0xa8b9ccu,"l"},
    {164, 56,0xa8b9ccu,"t"},
    {171, 56,0xa8b9ccu,"y"},
    { 72, 56,0xf08080u,"G"},
    { 79, 56,0xf08080u,"a"},
    { 86, 56,0xf08080u,"m"},
    { 93, 56,0xf08080u,"e"},
    {100, 56,0xf08080u," "},
    {107, 56,0xf08080u,"D"},
    {114, 56,0xf08080u,"i"},
    {121, 56,0xf08080u,"f"},
    {128, 56,0xf08080u,"f"},
    {135, 56,0xf08080u,"i"},
    {142, 56,0xf08080u,"c"},
    {149, 56,0xf08080u,"u"},
    {156, 56,0xf08080u,"l"},
    {163, 56,0xf08080u,"t"},
    {170, 56,0xf08080u,"y"},
    {232, 57,0xa8b9ccu,"1"},
    {239, 57,0xa8b9ccu,":"},
    {246, 57,0xa8b9ccu,"E"},
    {253, 57,0xa8b9ccu,"a"},
    {260, 57,0xa8b9ccu,"s"},
    {267, 57,0xa8b9ccu,"y"},
    {233, 56,0xa8b9ccu,"1"},
    {240, 56,0xa8b9ccu,":"},
    {247, 56,0xa8b9ccu,"E"},
    {254, 56,0xa8b9ccu,"a"},
    {261, 56,0xa8b9ccu,"s"},
    {268, 56,0xa8b9ccu,"y"},
    {232, 56,0xf08080u,"1"},
    {239, 56,0xf08080u,":"},
    {246, 56,0xf08080u,"E"},
    {253, 56,0xf08080u,"a"},
    {260, 56,0xf08080u,"s"},
    {267, 56,0xf08080u,"y"},
    { 72, 85,0xa8b9ccu,"A"},
    { 79, 85,0xa8b9ccu,"u"},
    { 86, 85,0xa8b9ccu,"t"},
    { 93, 85,0xa8b9ccu,"o"},
    {100, 85,0xa8b9ccu,"-"},
    {107, 85,0xa8b9ccu,"g"},
    {114, 85,0xa8b9ccu,"u"},
    {121, 85,0xa8b9ccu,"a"},
    {128, 85,0xa8b9ccu,"r"},
    {135, 85,0xa8b9ccu,"d"},
    { 73, 84,0xa8b9ccu,"A"},
    { 80, 84,0xa8b9ccu,"u"},
    { 87, 84,0xa8b9ccu,"t"},
    { 94, 84,0xa8b9ccu,"o"},
    {101, 84,0xa8b9ccu,"-"},
    {108, 84,0xa8b9ccu,"g"},
    {115, 84,0xa8b9ccu,"u"},
    {122, 84,0xa8b9ccu,"a"},
    {129, 84,0xa8b9ccu,"r"},
    {136, 84,0xa8b9ccu,"d"},
    { 72, 84,0x3e537du,"A"},
    { 79, 84,0x3e537du,"u"},
    { 86, 84,0x3e537du,"t"},
    { 93, 84,0x3e537du,"o"},
    {100, 84,0x3e537du,"-"},
    {107, 84,0x3e537du,"g"},
    {114, 84,0x3e537du,"u"},
    {121, 84,0x3e537du,"a"},
    {128, 84,0x3e537du,"r"},
    {135, 84,0x3e537du,"d"},
    {232, 85,0xa8b9ccu,"O"},
    {239, 85,0xa8b9ccu,"n"},
    {233, 84,0xa8b9ccu,"O"},
    {240, 84,0xa8b9ccu,"n"},
    {232, 84,0x3e537du,"O"},
    {239, 84,0x3e537du,"n"},
    { 72,113,0xa8b9ccu,"S"},
    { 79,113,0xa8b9ccu,"t"},
    { 86,113,0xa8b9ccu,"a"},
    { 93,113,0xa8b9ccu,"r"},
    {100,113,0xa8b9ccu,"t"},
    {107,113,0xa8b9ccu," "},
    {114,113,0xa8b9ccu,"G"},
    {121,113,0xa8b9ccu,"a"},
    {128,113,0xa8b9ccu,"m"},
    {135,113,0xa8b9ccu,"e"},
    { 73,112,0xa8b9ccu,"S"},
    { 80,112,0xa8b9ccu,"t"},
    { 87,112,0xa8b9ccu,"a"},
    { 94,112,0xa8b9ccu,"r"},
    {101,112,0xa8b9ccu,"t"},
    {108,112,0xa8b9ccu," "},
    {115,112,0xa8b9ccu,"G"},
    {122,112,0xa8b9ccu,"a"},
    {129,112,0xa8b9ccu,"m"},
    {136,112,0xa8b9ccu,"e"},
    { 72,112,0x3e537du,"S"},
    { 79,112,0x3e537du,"t"},
    { 86,112,0x3e537du,"a"},
    { 93,112,0x3e537du,"r"},
    {100,112,0x3e537du,"t"},
    {107,112,0x3e537du," "},
    {114,112,0x3e537du,"G"},
    {121,112,0x3e537du,"a"},
    {128,112,0x3e537du,"m"},
    {135,112,0x3e537du,"e"},
};
#define N_GOLDEN ((int)(sizeof(GOLDEN_OPS) / sizeof(GOLDEN_OPS[0])))

/* Build the menu, render it at the retail base, and diff against the golden. */
int test_newgame_config_matches_retail_stream(void)
{
    menu_ctrl grid;
    menu_node node;
    memset(&grid, 0, sizeof(grid));
    memset(&node, 0, sizeof(node));

    newgame_settings s = NEWGAME_SETTINGS_DEFAULT;   /* difficulty 10, auto-guard 1 */
    newgame_config_build(&grid, &node, &s);

    /* Sanity: 3 rows, the right kinds / ids, the laid-out cell texts. */
    T_ASSERT_EQ_I(grid.list->count, 3);
    T_ASSERT_EQ_I(grid.list->cursor, 0);             /* focus starts on row 0 */
    T_ASSERT_EQ_I(grid.rows[0].field0, 0);           /* option row */
    T_ASSERT_EQ_I(grid.rows[0].action, NEWGAME_OPT_DIFFICULTY);
    T_ASSERT_EQ_I(grid.rows[1].action, NEWGAME_OPT_AUTO_GUARD);
    T_ASSERT_EQ_I(grid.rows[2].field0, 3);           /* action button */
    T_ASSERT_EQ_I(grid.rows[2].action, NEWGAME_OPT_START_GAME);

    glyph_buf *r0c0 = (glyph_buf *)grid.rows[0].cells[0].obj0;
    glyph_buf *r0c1 = (glyph_buf *)grid.rows[0].cells[1].obj0;
    T_ASSERT(r0c0 != NULL && r0c1 != NULL);
    T_ASSERT_EQ_I(r0c0->len, (int)strlen("Game Difficulty"));
    T_ASSERT_EQ_I(r0c1->len, (int)strlen("1:Easy"));
    /* The Start-Game row has no value cell → its column 1 stays empty. */
    T_ASSERT_EQ_P(grid.rows[2].cells[1].obj0, NULL);

    /* Render at the box node's position (param_4/param_5 of FUN_0048e200). */
    g_nops = 0; g_cur_color = 0; memset(g_ops, 0, sizeof(g_ops));
    glyph_gdi_ops ops = recorder();
    void *FONT = (void *)0x1;                         /* opaque HFONT stand-in */
    glyph_grid_render(&node, &ops, /*x=*/32, /*y=*/32, FONT, FONT);

    /* Draw-for-draw equality against the golden. */
    T_ASSERT_EQ_I(g_nops, N_GOLDEN);
    for (int i = 0; i < N_GOLDEN; i++) {
        const rec_op *g = &GOLDEN_OPS[i];
        const rec_op *o = &g_ops[i];
        if (o->x != g->x || o->y != g->y || o->color != g->color ||
            strcmp(o->str, g->str) != 0) {
            T_FAIL("draw %d: got {x=%d y=%d c=0x%06x %s} want {x=%d y=%d c=0x%06x %s}",
                   i, o->x, o->y, o->color, o->str,
                   g->x, g->y, g->color, g->str);
        }
    }

    menu_ctrl_clear(&grid);
    return 0;
}

/* The focused row (row 0) draws its text in the focus colour 0xf08080; the
 * unfocused rows in the normal colour 0x3e537d; every shadow in 0xa8b9cc. */
int test_newgame_config_focus_colours(void)
{
    menu_ctrl grid; menu_node node;
    memset(&grid, 0, sizeof(grid)); memset(&node, 0, sizeof(node));
    newgame_settings s = NEWGAME_SETTINGS_DEFAULT;
    newgame_config_build(&grid, &node, &s);

    g_nops = 0; g_cur_color = 0; memset(g_ops, 0, sizeof(g_ops));
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&node, &ops, 32, 32, (void *)0x1, (void *)0x1);

    int focus = 0, normal = 0, shadow = 0;
    for (int i = 0; i < g_nops; i++) {
        switch (g_ops[i].color) {
        case 0xf08080u: focus++;  break;
        case 0x3e537du: normal++; break;
        case 0xa8b9ccu: shadow++; break;
        default: T_FAIL("unexpected colour 0x%06x at draw %d", g_ops[i].color, i);
        }
    }
    /* Row 0 main glyphs: "Game Difficulty"(15) + "1:Easy"(6) = 21 focused. */
    T_ASSERT_EQ_I(focus, 21);
    /* Rows 1+2 main glyphs: ("Auto-guard"+"On") + "Start Game" = 12 + 10 = 22. */
    T_ASSERT_EQ_I(normal, 22);
    /* Shadow = 2 copies of every main glyph: 2*(21+22) = 86. */
    T_ASSERT_EQ_I(shadow, 86);

    menu_ctrl_clear(&grid);
    return 0;
}

/* Column geometry: col 0 origin x=72, col 1 (value) origin x=232, rows at
 * y=56/84/112 — the inset (field_c=40,field_10=24) + entry[1].pos=0xa0 over
 * the box base (32,32), pitch 28. */
int test_newgame_config_column_geometry(void)
{
    menu_ctrl grid; menu_node node;
    memset(&grid, 0, sizeof(grid)); memset(&node, 0, sizeof(node));
    newgame_settings s = NEWGAME_SETTINGS_DEFAULT;
    newgame_config_build(&grid, &node, &s);

    T_ASSERT_EQ_I(grid.entries[0].pos, 0);
    T_ASSERT_EQ_I(grid.entries[1].pos, 0xa0);
    T_ASSERT_EQ_I(node.field_c, 0x28);
    T_ASSERT_EQ_I(node.field_10, 0x18);
    T_ASSERT_EQ_I(node.field_1ac, 0x1c);

    g_nops = 0; g_cur_color = 0; memset(g_ops, 0, sizeof(g_ops));
    glyph_gdi_ops ops = recorder();
    glyph_grid_render(&node, &ops, 32, 32, (void *)0x1, (void *)0x1);

    /* The very first main-colour glyph of col 0 / col 1 on the focused row. */
    int col0x = -1, col1x = -1;
    for (int i = 0; i < g_nops; i++) {
        if (g_ops[i].y == 56 && g_ops[i].color == 0xf08080u) {
            if (strcmp(g_ops[i].str, "G") == 0 && col0x < 0) col0x = g_ops[i].x;
            if (strcmp(g_ops[i].str, "1") == 0 && col1x < 0) col1x = g_ops[i].x;
        }
    }
    T_ASSERT_EQ_I(col0x, 72);
    T_ASSERT_EQ_I(col1x, 232);

    menu_ctrl_clear(&grid);
    return 0;
}
