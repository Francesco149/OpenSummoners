/*
 * tests/test_newgame_box.c — the 9-slice box panel render (src/newgame_box.c),
 * the new-game config chrome (port of FUN_0048cf80's opaque arm).
 *
 * The geometry is ground-truthed live (goldens/retail-newgame-box-cells.jsonl,
 * quirk #67): the menu box is (32,32) 400×124 and the tooltip box (32,392)
 * 576×80, both 32×32-cell 9-slices with frames {tl0,top1,tr2,l3,c4,r5,bl6,b7,
 * br8}.  These tests record every slice blit and assert:
 *   (a) the four corners land at the expected rects with the right frame ids;
 *   (b) the slices TILE-COVER the whole box exactly once — no gap, no overlap,
 *       nothing out of bounds (a coverage grid catches an off-by-one in any of
 *       the corner/edge/remainder placements);
 *   (c) the interior uses the center frame (4) and the border the edge/corner
 *       frames — never the wrong slice.
 */
#include "t.h"
#include "newgame_box.h"

#include <stdint.h>
#include <string.h>

/* ─── recording newgame_box_ops stub ─────────────────────────────────── */
#define MAX_BLITS 512
typedef struct { int id, x, y, w, h; } rec_blit;

static rec_blit g_bl[MAX_BLITS];
static int      g_nbl;

/* frame() returns a tagged handle encoding the slice id (id+1, so 0 is a valid
 * non-NULL handle); blt() decodes it back. */
static void *rec_frame(void *u, int id) { (void)u; return (void *)(intptr_t)(id + 1); }
static void  rec_blt(void *u, void *frame, int x, int y, int w, int h)
{
    (void)u;
    if (g_nbl >= MAX_BLITS) return;
    g_bl[g_nbl].id = (int)(intptr_t)frame - 1;
    g_bl[g_nbl].x = x; g_bl[g_nbl].y = y;
    g_bl[g_nbl].w = w; g_bl[g_nbl].h = h;
    g_nbl++;
}

static const int FRAMES9[9] = {
    NEWGAME_BOX_TL, NEWGAME_BOX_TOP,    NEWGAME_BOX_TR,
    NEWGAME_BOX_L,  NEWGAME_BOX_CENTER, NEWGAME_BOX_R,
    NEWGAME_BOX_BL, NEWGAME_BOX_BOTTOM, NEWGAME_BOX_BR,
};

static void render(int x0, int y0, int w, int h)
{
    g_nbl = 0;
    newgame_box_ops ops = { rec_frame, rec_blt, NULL };
    newgame_box_render(&ops, x0, y0, w, h, FRAMES9, NEWGAME_BOX_CELL);
}

/* Find the single blit whose top-left is exactly (x,y); -1 if none/ambiguous. */
static int find_at(int x, int y)
{
    int hit = -1;
    for (int i = 0; i < g_nbl; i++)
        if (g_bl[i].x == x && g_bl[i].y == y) {
            if (hit != -1) return -2;   /* ambiguous */
            hit = i;
        }
    return hit;
}

/* Coverage grid: paint every blit rect (clamped to the box) into a WxH map of
 * the slice id; assert each cell painted exactly once and check OOB. */
static int g_cov[160][640];   /* [h][w], box-relative; sized for tooltip 576×80 */

static int check_cover(int x0, int y0, int w, int h, const char **err)
{
    static char buf[128];
    memset(g_cov, -1, sizeof g_cov);
    for (int i = 0; i < g_nbl; i++) {
        rec_blit *b = &g_bl[i];
        /* out of bounds? */
        if (b->x < x0 || b->y < y0 || b->x + b->w > x0 + w || b->y + b->h > y0 + h) {
            snprintf(buf, sizeof buf, "blit %d id=%d (%d,%d %dx%d) out of box (%d,%d %dx%d)",
                     i, b->id, b->x, b->y, b->w, b->h, x0, y0, w, h);
            *err = buf; return 0;
        }
        for (int yy = 0; yy < b->h; yy++)
            for (int xx = 0; xx < b->w; xx++) {
                int gx = b->x - x0 + xx, gy = b->y - y0 + yy;
                if (g_cov[gy][gx] != -1) {
                    snprintf(buf, sizeof buf, "overlap at box(%d,%d): id %d and %d",
                             gx, gy, g_cov[gy][gx], b->id);
                    *err = buf; return 0;
                }
                g_cov[gy][gx] = b->id;
            }
    }
    /* every cell covered? */
    for (int gy = 0; gy < h; gy++)
        for (int gx = 0; gx < w; gx++)
            if (g_cov[gy][gx] == -1) {
                snprintf(buf, sizeof buf, "gap at box(%d,%d)", gx, gy);
                *err = buf; return 0;
            }
    return 1;
}

/* corner positions for a box at (x0,y0) wxh, cell 32. */
static int corner_ok(int x0, int y0, int w, int h, int cell)
{
    int idx;
    idx = find_at(x0, y0);                       /* TL */
    if (idx < 0 || g_bl[idx].id != NEWGAME_BOX_TL) return 0;
    idx = find_at((x0 - cell) + w, y0);          /* TR */
    if (idx < 0 || g_bl[idx].id != NEWGAME_BOX_TR) return 0;
    idx = find_at(x0, (y0 - cell) + h);          /* BL */
    if (idx < 0 || g_bl[idx].id != NEWGAME_BOX_BL) return 0;
    idx = find_at((x0 - cell) + w, (y0 - cell) + h);  /* BR */
    if (idx < 0 || g_bl[idx].id != NEWGAME_BOX_BR) return 0;
    return 1;
}

/* ─── tests ──────────────────────────────────────────────────────────── */

/* The menu box (32,32) 400×124 tiles its whole area exactly once. */
int test_newgame_box_menu_covers_exactly(void)
{
    render(32, 32, 400, 124);
    T_ASSERT(g_nbl > 0);
    const char *err = NULL;
    if (!check_cover(32, 32, 400, 124, &err))
        T_FAIL("menu box coverage: %s", err);
    if (!corner_ok(32, 32, 400, 124, 32))
        T_FAIL("menu box corners misplaced or wrong frame");
    return 0;
}

/* The tooltip box (32,392) 576×80 — wider, H exactly 2.5 cells, W a multiple of
 * the cell (no horizontal remainder) — also tiles exactly once. */
int test_newgame_box_tooltip_covers_exactly(void)
{
    render(32, 392, 576, 80);
    T_ASSERT(g_nbl > 0);
    const char *err = NULL;
    if (!check_cover(32, 392, 576, 80, &err))
        T_FAIL("tooltip box coverage: %s", err);
    if (!corner_ok(32, 392, 576, 80, 32))
        T_FAIL("tooltip box corners misplaced or wrong frame");
    return 0;
}

/* Interior pixels use the center frame (4); the 1px border ring uses edge/
 * corner frames (never center). */
int test_newgame_box_center_vs_border(void)
{
    render(32, 32, 400, 124);
    const char *err = NULL;
    if (!check_cover(32, 32, 400, 124, &err))
        T_FAIL("coverage precheck: %s", err);

    /* a deep-interior pixel is the center fill */
    T_ASSERT_EQ_I(g_cov[62][200], NEWGAME_BOX_CENTER);   /* box(200,62) */
    /* top-edge interior pixel is the top frame */
    T_ASSERT_EQ_I(g_cov[5][200], NEWGAME_BOX_TOP);       /* box(200,5)  */
    /* left-edge interior pixel is the left frame */
    T_ASSERT_EQ_I(g_cov[62][5], NEWGAME_BOX_L);          /* box(5,62)   */
    /* the top-left 32×32 cell is the TL corner, never center */
    T_ASSERT_EQ_I(g_cov[0][0], NEWGAME_BOX_TL);
    return 0;
}

/* The guard rejects a box too small for even one corner pair in either axis. */
int test_newgame_box_tiny_guard(void)
{
    render(10, 10, 40, 40);   /* 40 >= 2*32? no; 40>=64 false both → still one axis? */
    /* 40 < 64 in both axes → guard (cell*2<=w || cell*2<=h) is false → no blits */
    T_ASSERT_EQ_I(g_nbl, 0);
    return 0;
}
