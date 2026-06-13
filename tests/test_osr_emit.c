/*
 * tests/test_osr_emit.c — host round-trip tests for src/osr_emit.c (M5).
 *
 * The port-side .osr emitter is driven with synthetic zdd_objects + a canned
 * surface reader, then the file is streamed back through the M4 reader
 * (src/osr_replay.c) and every record class is asserted: frame structure
 * (present-then-framebeg), per-frame seq ordering shared across CLEAR/BLIT/
 * TEXT, the dedup'd SHEET (one per surface content, dhash referenced by the
 * BLITs, re-grab after evict does not re-emit), the primary-dest filter
 * (offscreen blits/text dropped), FONT/BLEND refs, and ANCHOR/SEED.
 *
 * The emitter is a process-global singleton (mirroring call_trace), so the
 * whole lifecycle lives in ONE test body; the no-op safety of calling sinks
 * while closed is checked first.
 */
#include "../src/osr_emit.h"
#include "../src/osr_replay.h"
#include "../src/zdd.h"
#include "../src/render_id.h"
#include "t.h"

/* ── canned surface reader: a 4x2 RGB565 block, pitch 8 ─────────────────────── */
static const uint16_t k_px[8] = {
    0xF800, 0x07E0, 0x001F, 0xFFFF,
    0x0000, 0xF81F, 0x07FF, 0xFFE0,
};
static int  g_read_calls;
static int  g_done_calls;

static int canned_read(struct zdd_object *obj, osr_emit_surf *out)
{
    (void)obj;
    g_read_calls++;
    out->pixels   = k_px;
    out->pitch    = 8;
    out->w        = 4;
    out->h        = 2;
    out->bitcount = 16;
    return 1;
}
static void canned_done(struct zdd_object *obj)
{
    (void)obj;
    g_done_calls++;
}

/* ── counting/recording read-back sink ──────────────────────────────────────── */
#define RT_MAX_BLITS 16
typedef struct {
    osr_header header;
    int n_framebeg, n_present, n_clear, n_blit, n_text, n_sheet, n_font,
        n_blend, n_anchor, n_seed;
    uint32_t framebeg_flip[8], framebeg_tick[8];
    osr_blit blit[RT_MAX_BLITS];
    uint32_t clear_seq, clear_dst;
    uint32_t sheet_dhash; uint16_t sheet_w, sheet_h; uint32_t sheet_pitch;
    uint8_t  sheet_bytes[64]; uint32_t sheet_byte_len;
    osr_font font;
    uint32_t text_seq, text_font_ref, text_color, text_dst;
    int32_t  text_x, text_y, text_bk;
    char     text_str[16];
    uint32_t blend_ref; int32_t blend_mode; uint32_t blend_lut_len[3];
    char     anchor_name[32]; uint32_t anchor_flip, anchor_rng;
    uint32_t seed_value;
} rt_state;

static void rt_header(void *u, const osr_header *h)   { ((rt_state *)u)->header = *h; }
static void rt_framebeg(void *u, const osr_framebeg *fb)
{
    rt_state *s = (rt_state *)u;
    if (s->n_framebeg < 8) {
        s->framebeg_flip[s->n_framebeg] = fb->flip;
        s->framebeg_tick[s->n_framebeg] = fb->sim_tick;
    }
    s->n_framebeg++;
}
static void rt_present(void *u, const osr_present *pr) { (void)pr; ((rt_state *)u)->n_present++; }
static void rt_clear(void *u, const osr_clear *c)
{
    rt_state *s = (rt_state *)u;
    s->clear_seq = c->seq; s->clear_dst = c->dst_handle;
    s->n_clear++;
}
static void rt_blit(void *u, const osr_blit *b)
{
    rt_state *s = (rt_state *)u;
    if (s->n_blit < RT_MAX_BLITS) s->blit[s->n_blit] = *b;
    s->n_blit++;
}
static void rt_text(void *u, const osr_text *t)
{
    rt_state *s = (rt_state *)u;
    s->text_seq = t->seq; s->text_dst = t->dst_handle;
    s->text_x = t->x; s->text_y = t->y;
    s->text_font_ref = t->font_ref; s->text_color = t->color;
    s->text_bk = t->bk_mode;
    uint32_t n = t->str_len < 15 ? t->str_len : 15;
    memcpy(s->text_str, t->str, n); s->text_str[n] = '\0';
    s->n_text++;
}
static void rt_sheet(void *u, const osr_sheet *sh)
{
    rt_state *s = (rt_state *)u;
    s->sheet_dhash = sh->dhash; s->sheet_w = sh->w; s->sheet_h = sh->h;
    s->sheet_pitch = sh->pitch;
    s->sheet_byte_len = sh->byte_len < 64 ? sh->byte_len : 64;
    memcpy(s->sheet_bytes, sh->bytes, s->sheet_byte_len);
    s->n_sheet++;
}
static void rt_font(void *u, const osr_font *f)  { ((rt_state *)u)->font = *f; ((rt_state *)u)->n_font++; }
static void rt_blend(void *u, const osr_blend *b)
{
    rt_state *s = (rt_state *)u;
    s->blend_ref = b->blend_ref; s->blend_mode = b->mode;
    for (int i = 0; i < 3; i++) s->blend_lut_len[i] = b->lut_len[i];
    s->n_blend++;
}
static void rt_anchor(void *u, const osr_anchor *a)
{
    rt_state *s = (rt_state *)u;
    uint32_t n = a->name_len < 31 ? a->name_len : 31;
    memcpy(s->anchor_name, a->name, n); s->anchor_name[n] = '\0';
    s->anchor_flip = a->flip; s->anchor_rng = a->rng;
    s->n_anchor++;
}
static void rt_seed(void *u, const osr_seed *sd)
{
    ((rt_state *)u)->seed_value = sd->value;
    ((rt_state *)u)->n_seed++;
}

/* Sinks must be safe no-ops while the emitter is closed. */
int test_osr_emit_inactive_noop(void)
{
    zdd_object src, prim;
    memset(&src, 0, sizeof src); memset(&prim, 0, sizeof prim);
    T_ASSERT(osr_emit_is_active() == 0);
    osr_emit_flip(1, 0);
    osr_emit_blit(0x5b9b70, 1, &src, &prim, 0, 0, 4, 2, 0, 0, 0, 0, NULL, 0);
    osr_emit_clear(&prim, 0);
    osr_emit_anchor("x", 1, 0, 0);
    osr_emit_seed(1, 0, 0);
    osr_emit_gdi_text((void *)0x1, 0, 0, "a", 1);
    osr_emit_evict(&src);
    osr_emit_close();
    T_ASSERT(osr_emit_is_active() == 0);
    return 0;
}

int test_osr_emit_roundtrip(void)
{
    const char *path = "build/test_osr_emit_rt.osr";

    /* synthetic objects: a primary, a source cel, an offscreen dest */
    static zdd_object prim, src, off;
    memset(&prim, 0, sizeof prim); memset(&src, 0, sizeof src);
    memset(&off, 0, sizeof off);
    src.metric_b8 = 4;  src.metric_bc = 2;       /* source w/h     */
    src.metric_0c = 1;  src.metric_10 = 2;       /* placement orig */
    src.state_flag = 0x8000;                     /* KEYSRC armed   */
    render_id_register(&src, /*res=*/0x456, /*frame=*/3, /*dhash=*/0xdead);

    T_ASSERT(osr_emit_open(path, 0x4f5347, OSR_FLAG_SEED_PIN, "emit-test",
                           640, 480, OSR_PIXFMT_RGB565) == 1);
    T_ASSERT(osr_emit_is_active() == 1);
    osr_emit_set_primary(&prim);
    osr_emit_set_surf_reader(canned_read, canned_done);
    g_read_calls = g_done_calls = 0;

    /* FONT registered at creation (the ar_gdi_create_font chokepoint) */
    void *hfont = (void *)0x1111;
    osr_font fdef;
    memset(&fdef, 0, sizeof fdef);
    fdef.height = 18; fdef.width = 7; fdef.weight = 400; fdef.charset = 1;
    strcpy(fdef.face, "Courier New");
    osr_emit_font_create(hfont, &fdef);
    osr_emit_font_create(hfont, &fdef);          /* dup — must not re-emit */

    /* boot seed pin */
    osr_emit_seed(0, 0x12345678, 0x4f5347);

    /* frame 1: clear, two blits of the same source (one SHEET), an offscreen
     * blit (dropped), GDI text (one kept, one offscreen dropped) */
    osr_emit_flip(1, 0);
    osr_emit_clear(&prim, 0);                                       /* seq 0 */
    osr_emit_blit(0x5b9b70, 1, &src, &prim, 10, 20, 4, 2, 0, 0, 0, 0,
                  NULL, 0xf81f);                                    /* seq 1 */
    osr_emit_blit(0x5b9ae0, 2, &src, &prim, 30, 40, 8, 4, 1, 1, 4, 2,
                  NULL, 0xf81f);                                    /* seq 2 */
    osr_emit_blit(0x5b9b70, 1, &src, &off, 0, 0, 4, 2, 0, 0, 0, 0,
                  NULL, 0);                                /* offscreen drop */

    void *hdc = (void *)0x2222, *hdc_off = (void *)0x3333;
    osr_emit_dc_open(&prim, hdc);
    osr_emit_gdi_bkmode(hdc, 1);
    osr_emit_gdi_select_font(hdc, hfont);
    osr_emit_gdi_color(hdc, 0xfffff0);
    osr_emit_gdi_text(hdc, 100, 200, "Hi", 2);                      /* seq 3 */
    osr_emit_dc_close(hdc);
    osr_emit_dc_open(&off, hdc_off);
    osr_emit_gdi_text(hdc_off, 0, 0, "no", 2);             /* offscreen drop */
    osr_emit_dc_close(hdc_off);

    /* mode-4 alpha: BLEND dedup'd by content (two blits, one record) */
    static const uint8_t lut_r[32], lut_g[64], lut_b[32];
    zdd_blend_desc desc;
    memset(&desc, 0, sizeof desc);
    desc.mode = 0;
    desc.ch[0].shift = 11; desc.ch[0].mask = 0xF800; desc.ch[0].lut = lut_r;
    desc.ch[1].shift = 5;  desc.ch[1].mask = 0x07E0; desc.ch[1].lut = lut_g;
    desc.ch[2].shift = 0;  desc.ch[2].mask = 0x001F; desc.ch[2].lut = lut_b;
    osr_emit_blit(0x5bd550, 4, &src, &prim, 50, 60, 4, 2, 0, 0, 0, 0,
                  &desc, 0xf81f);                                   /* seq 4 */
    osr_emit_blit(0x5bd550, 4, &src, &prim, 51, 61, 4, 2, 0, 0, 0, 0,
                  &desc, 0xf81f);                                   /* seq 5 */

    osr_emit_anchor("game_enter", 1, 0, 0xabcdef);

    /* frame 2: evict the source (dtor) — re-grab happens but identical content
     * must NOT re-emit a SHEET, and the blit keeps the same dhash. */
    osr_emit_flip(2, 1);
    osr_emit_evict(&src);
    osr_emit_blit(0x5b9b70, 1, &src, &prim, 11, 21, 4, 2, 0, 0, 0, 0,
                  NULL, 0xf81f);                                    /* seq 0 */

    osr_emit_close();
    T_ASSERT(osr_emit_is_active() == 0);

    /* the canned reader ran exactly twice: first grab + post-evict re-grab */
    T_ASSERT_EQ_I(g_read_calls, 2);
    T_ASSERT_EQ_I(g_done_calls, 2);

    /* ── stream it back ── */
    rt_state st;
    memset(&st, 0, sizeof st);
    osr_replay_sink sink;
    memset(&sink, 0, sizeof sink);
    sink.user           = &st;
    sink.on_header      = rt_header;
    sink.on_frame_begin = rt_framebeg;
    sink.on_present     = rt_present;
    sink.on_clear       = rt_clear;
    sink.on_blit        = rt_blit;
    sink.on_text        = rt_text;
    sink.on_sheet       = rt_sheet;
    sink.on_font        = rt_font;
    sink.on_blend       = rt_blend;
    sink.on_anchor      = rt_anchor;
    sink.on_seed        = rt_seed;
    T_ASSERT(osr_replay_file(path, &sink) == OSR_REPLAY_OK);

    /* header */
    T_ASSERT_EQ_U(st.header.side, OSR_SIDE_PORT);
    T_ASSERT_EQ_U(st.header.pixfmt, OSR_PIXFMT_RGB565);
    T_ASSERT_EQ_U(st.header.screen_w, 640);
    T_ASSERT_EQ_U(st.header.seed, 0x4f5347);
    T_ASSERT_EQ_U(st.header.flags, OSR_FLAG_SEED_PIN);
    T_ASSERT(strcmp(st.header.scenario, "emit-test") == 0);

    /* frame structure: FRAMEBEG(1) … FRAMEBEG(2) closes 1, close() closes 2 */
    T_ASSERT_EQ_I(st.n_framebeg, 2);
    T_ASSERT_EQ_I(st.n_present, 2);
    T_ASSERT_EQ_U(st.framebeg_flip[0], 1);
    T_ASSERT_EQ_U(st.framebeg_tick[0], 0);
    T_ASSERT_EQ_U(st.framebeg_flip[1], 2);
    T_ASSERT_EQ_U(st.framebeg_tick[1], 1);

    /* the dedup'd SHEET: one record, pixels intact, dhash = the documented
     * seed order (w,h,bitcount then pitch*h bytes) */
    T_ASSERT_EQ_I(st.n_sheet, 1);
    T_ASSERT_EQ_U(st.sheet_w, 4);
    T_ASSERT_EQ_U(st.sheet_h, 2);
    T_ASSERT_EQ_U(st.sheet_pitch, 8);
    T_ASSERT_EQ_U(st.sheet_byte_len, 16);
    T_ASSERT(memcmp(st.sheet_bytes, k_px, 16) == 0);
    {
        uint32_t w = 4, h = 2; uint16_t bc = 16;
        uint32_t seed = render_id_hash_seed(RENDER_ID_FNV1A_SEED, &w, sizeof w);
        seed = render_id_hash_seed(seed, &h, sizeof h);
        seed = render_id_hash_seed(seed, &bc, sizeof bc);
        T_ASSERT_EQ_U(st.sheet_dhash,
                      render_id_hash_seed(seed, k_px, sizeof k_px));
    }

    /* blits: 5 kept (offscreen one dropped); shared seq with CLEAR + TEXT */
    T_ASSERT_EQ_I(st.n_blit, 5);
    T_ASSERT_EQ_I(st.n_clear, 1);
    T_ASSERT_EQ_U(st.clear_seq, 0);
    T_ASSERT_EQ_U(st.clear_dst, 1);          /* primary = handle 1 */
    T_ASSERT_EQ_U(st.blit[0].seq, 1);
    T_ASSERT_EQ_U(st.blit[0].va, 0x5b9b70);
    T_ASSERT_EQ_U(st.blit[0].mode, 1);
    T_ASSERT_EQ_U(st.blit[0].res, 0x456);
    T_ASSERT_EQ_U(st.blit[0].frame, 3);
    T_ASSERT_EQ_U(st.blit[0].dhash, st.sheet_dhash);
    T_ASSERT_EQ_U(st.blit[0].dst_handle, 1);
    T_ASSERT_EQ_I(st.blit[0].dx, 10);
    T_ASSERT_EQ_I(st.blit[0].dy, 20);
    T_ASSERT_EQ_I(st.blit[0].ow, 4);
    T_ASSERT_EQ_I(st.blit[0].oh, 2);
    T_ASSERT_EQ_I(st.blit[0].ox, 1);
    T_ASSERT_EQ_I(st.blit[0].oy, 2);
    T_ASSERT_EQ_U(st.blit[0].state, 0x8000);
    T_ASSERT_EQ_U(st.blit[0].ckey, 0xf81f);
    T_ASSERT_EQ_I(st.blit[0].bmode, -1);
    T_ASSERT_EQ_U(st.blit[0].blend_ref, 0);
    /* mode-2 RECTS carries the source extent */
    T_ASSERT_EQ_U(st.blit[1].seq, 2);
    T_ASSERT_EQ_U(st.blit[1].mode, 2);
    T_ASSERT_EQ_I(st.blit[1].srcw, 4);
    T_ASSERT_EQ_I(st.blit[1].srch, 2);
    /* the two alpha blits share ONE blend record */
    T_ASSERT_EQ_U(st.blit[2].seq, 4);
    T_ASSERT_EQ_U(st.blit[2].mode, 4);
    T_ASSERT_EQ_I(st.blit[2].bmode, 0);
    T_ASSERT_EQ_U(st.blit[2].blend_ref, 1);
    T_ASSERT_EQ_U(st.blit[3].blend_ref, 1);
    T_ASSERT_EQ_I(st.n_blend, 1);
    T_ASSERT_EQ_U(st.blend_ref, 1);
    T_ASSERT_EQ_I(st.blend_mode, 0);
    T_ASSERT_EQ_U(st.blend_lut_len[0], 32);   /* mode 0: (0xF800>>11)+1 */
    T_ASSERT_EQ_U(st.blend_lut_len[1], 64);   /*          (0x07E0>>5)+1 */
    T_ASSERT_EQ_U(st.blend_lut_len[2], 32);   /*          (0x001F>>0)+1 */
    /* frame 2's post-evict blit: seq restarts, dhash unchanged */
    T_ASSERT_EQ_U(st.blit[4].seq, 0);
    T_ASSERT_EQ_U(st.blit[4].dhash, st.sheet_dhash);

    /* GDI text: one record (offscreen DC dropped), per-DC shadow applied */
    T_ASSERT_EQ_I(st.n_text, 1);
    T_ASSERT_EQ_U(st.text_seq, 3);
    T_ASSERT_EQ_U(st.text_dst, 1);
    T_ASSERT_EQ_I(st.text_x, 100);
    T_ASSERT_EQ_I(st.text_y, 200);
    T_ASSERT_EQ_U(st.text_font_ref, 1);
    T_ASSERT_EQ_U(st.text_color, 0xfffff0);
    T_ASSERT_EQ_I(st.text_bk, 1);
    T_ASSERT(strcmp(st.text_str, "Hi") == 0);

    /* FONT: registered once despite the duplicate create */
    T_ASSERT_EQ_I(st.n_font, 1);
    T_ASSERT_EQ_U(st.font.font_ref, 1);
    T_ASSERT_EQ_I(st.font.height, 18);
    T_ASSERT(strcmp(st.font.face, "Courier New") == 0);

    /* ANCHOR + SEED */
    T_ASSERT_EQ_I(st.n_anchor, 1);
    T_ASSERT(strcmp(st.anchor_name, "game_enter") == 0);
    T_ASSERT_EQ_U(st.anchor_flip, 1);
    T_ASSERT_EQ_U(st.anchor_rng, 0xabcdef);
    T_ASSERT_EQ_I(st.n_seed, 1);
    T_ASSERT_EQ_U(st.seed_value, 0x4f5347);

    render_id_forget(&src);
    remove(path);
    return 0;
}
