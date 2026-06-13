#define _GNU_SOURCE   /* fmemopen — feature-gated under -std=c11 */
/*
 * tests/test_osr_replay.c — host tests for the .osr streaming reader
 * (src/osr_replay.c, Trace Studio v2 M4).
 *
 * The reconstructor cannot slurp a 600 MB capture into a 32-bit process, so it
 * STREAMS the framed record stream and dispatches each record to a visitor.
 * These tests build a synthetic .osr in memory (via the osr_format.h encoders),
 * stream it through a recording sink, and assert: every record kind is decoded
 * and dispatched, the variable-length payloads (SHEET pixels / TEXT string)
 * survive intact, the per-frame draw ORDER is preserved (the seq the replayer
 * interleaves blits + text on), and a TRUNCATED tail (hard-killed capture) stops
 * cleanly without dropping the records that DID complete.
 */
#include "../src/osr_replay.h"
#include "t.h"

#include <stdlib.h>
#include <string.h>

/* ── a recording sink: counts each kind + keeps the last/important values ──── */
#define REC_OPS_CAP 32
typedef struct rec_sink {
    int n_header, n_framebeg, n_present, n_blit, n_text, n_sheet, n_font,
        n_anchor, n_seed, n_snap, n_clear;
    osr_header   last_header;
    osr_framebeg last_framebeg;
    osr_blit     last_blit;
    osr_font     last_font;
    osr_seed     last_seed;
    /* SHEET: copy the pixel bytes out (they're only valid during the call) */
    uint32_t sheet_dhash;
    uint16_t sheet_w, sheet_h;
    uint8_t  sheet_pix[64];
    uint32_t sheet_pix_len;
    /* TEXT: copy the string out */
    char     text_str[64];
    uint32_t text_str_len;
    uint32_t text_seq, text_color;
    int32_t  text_x, text_y;
    /* SNAP: copy the pixel bytes out (only valid during the call) */
    uint32_t snap_flip;
    uint16_t snap_w, snap_h;
    uint8_t  snap_pix[64];
    uint32_t snap_pix_len;
    /* ANCHOR: copy the name */
    char     anchor_name[32];
    /* the seq of every blit + text op, in dispatch order (the replay order) */
    uint32_t op_seq[REC_OPS_CAP];
    int      n_ops;
} rec_sink;

static void r_header(void *u, const osr_header *h)
{ rec_sink *r = u; r->n_header++; r->last_header = *h; }
static void r_framebeg(void *u, const osr_framebeg *fb)
{ rec_sink *r = u; r->n_framebeg++; r->last_framebeg = *fb; }
static void r_present(void *u, const osr_present *pr)
{ (void)pr; rec_sink *r = u; r->n_present++; }
static void r_blit(void *u, const osr_blit *b)
{
    rec_sink *r = u; r->n_blit++; r->last_blit = *b;
    if (r->n_ops < REC_OPS_CAP) r->op_seq[r->n_ops++] = b->seq;
}
static void r_text(void *u, const osr_text *t)
{
    rec_sink *r = u; r->n_text++;
    r->text_seq = t->seq; r->text_x = t->x; r->text_y = t->y;
    r->text_color = t->color;
    r->text_str_len = t->str_len < sizeof r->text_str ? t->str_len
                                                      : (uint32_t)(sizeof r->text_str - 1);
    memcpy(r->text_str, t->str, r->text_str_len);
    r->text_str[r->text_str_len] = '\0';
    if (r->n_ops < REC_OPS_CAP) r->op_seq[r->n_ops++] = t->seq;
}
static void r_sheet(void *u, const osr_sheet *s)
{
    rec_sink *r = u; r->n_sheet++;
    r->sheet_dhash = s->dhash; r->sheet_w = s->w; r->sheet_h = s->h;
    r->sheet_pix_len = s->byte_len < sizeof r->sheet_pix ? s->byte_len
                                                        : (uint32_t)sizeof r->sheet_pix;
    if (s->bytes) memcpy(r->sheet_pix, s->bytes, r->sheet_pix_len);
}
static void r_clear(void *u, const osr_clear *cl)
{
    rec_sink *r = u; r->n_clear++;
    if (r->n_ops < REC_OPS_CAP) r->op_seq[r->n_ops++] = cl->seq;
}
static void r_snap(void *u, const osr_snap *s)
{
    rec_sink *r = u; r->n_snap++;
    r->snap_flip = s->flip; r->snap_w = s->w; r->snap_h = s->h;
    r->snap_pix_len = s->byte_len < sizeof r->snap_pix ? s->byte_len
                                                       : (uint32_t)sizeof r->snap_pix;
    if (s->bytes) memcpy(r->snap_pix, s->bytes, r->snap_pix_len);
}
static void r_font(void *u, const osr_font *f)
{ rec_sink *r = u; r->n_font++; r->last_font = *f; }
static void r_anchor(void *u, const osr_anchor *a)
{
    rec_sink *r = u; r->n_anchor++;
    uint32_t n = a->name_len < sizeof r->anchor_name ? a->name_len
                                                    : (uint32_t)(sizeof r->anchor_name - 1);
    memcpy(r->anchor_name, a->name, n);
    r->anchor_name[n] = '\0';
}
static void r_seed(void *u, const osr_seed *s)
{ rec_sink *r = u; r->n_seed++; r->last_seed = *s; }

static osr_replay_sink make_rec_sink(rec_sink *r)
{
    memset(r, 0, sizeof *r);
    osr_replay_sink s = {0};
    s.user = r;
    s.on_header = r_header;       s.on_frame_begin = r_framebeg;
    s.on_present = r_present;     s.on_blit = r_blit;
    s.on_text = r_text;           s.on_sheet = r_sheet;
    s.on_snap = r_snap;           s.on_clear = r_clear;
    s.on_font = r_font;           s.on_anchor = r_anchor;
    s.on_seed = r_seed;
    return s;
}

/* Build one representative .osr (header + a font + a sheet + one frame whose op
 * list is SHEET-ref'd blit, text, blit — present) into `out`; returns its len. */
static size_t build_sample_osr(uint8_t *out, size_t cap)
{
    size_t off = 0;
    osr_header h = {0};
    h.side = OSR_SIDE_RETAIL; h.pixfmt = OSR_PIXFMT_RGB565;
    h.screen_w = 640; h.screen_h = 480; h.seed = 0x4f5347;
    h.flags = OSR_FLAG_TURBO | OSR_FLAG_LOCKSTEP | OSR_FLAG_SEED_PIN;
    strcpy(h.scenario, "intro-1");
    off += osr_header_encode(out + off, cap - off, &h);

    off += osr_enc_seed(out + off, cap - off, /*flip*/0, /*before*/0xdead, 0x4f5347);

    osr_font f = {0};
    f.font_ref = 3; f.height = 18; f.weight = 400; f.charset = 128;
    strcpy(f.face, "Courier New");
    off += osr_enc_font(out + off, cap - off, &f);

    /* a small 2x2 RGB565 sheet (8 bytes) */
    uint8_t pix[8] = { 0x11,0x22, 0x33,0x44, 0x55,0x66, 0x77,0x88 };
    osr_sheet sh = {0};
    sh.dhash = 0xABCD1234; sh.res = 1002; sh.frame = 5; sh.w = 2; sh.h = 2;
    sh.pitch = 4; sh.pixfmt = OSR_PIXFMT_RGB565; sh.codec = 0;
    sh.byte_len = 8; sh.bytes = pix;
    off += osr_enc_sheet(out + off, cap - off, &sh);

    off += osr_enc_framebeg(out + off, cap - off, /*flip*/1250, /*tick*/8, /*anchor*/0);

    /* the frame opens with an ORDERED backbuffer CLEAR (seq 0, shared axis) */
    off += osr_enc_clear(out + off, cap - off, /*seq*/0, /*dst_handle*/1, 0);

    osr_blit b = {0};
    b.va = 0x5b9a40; b.seq = 1; b.res = 1002; b.frame = 5; b.dhash = 0xABCD1234;
    b.dst_handle = 1; b.dx = 100; b.dy = 50; b.reqw = 2; b.reqh = 2;
    b.ow = 2; b.oh = 2; b.state = 0x8000; b.ckey = 0xf81f; b.bmode = -1; b.mode = 0;
    off += osr_enc_blit(out + off, cap - off, &b);

    osr_text t = {0};
    t.seq = 2; t.dst_handle = 1; t.x = 64; t.y = 200; t.font_ref = 3;
    t.color = 0x3e537d; t.bk_mode = 1;
    const char *str = "Arche";
    t.str_len = (uint32_t)strlen(str); t.str = str;
    off += osr_enc_text(out + off, cap - off, &t);

    b.seq = 3; b.dx = 120; b.mode = 1; b.va = 0x5b9b70;
    off += osr_enc_blit(out + off, cap - off, &b);

    /* the real-backbuffer SNAP sits just before the frame's PRESENT (M4d) */
    uint8_t snap_pix[8] = { 0xa0,0xa1, 0xb0,0xb1, 0xc0,0xc1, 0xd0,0xd1 };
    osr_snap sn = {0};
    sn.flip = 1250; sn.sim_tick = 8; sn.w = 2; sn.h = 2; sn.pitch = 4;
    sn.pixfmt = OSR_PIXFMT_RGB565; sn.codec = 0;
    sn.byte_len = 8; sn.bytes = snap_pix;
    off += osr_enc_snap(out + off, cap - off, &sn);

    off += osr_enc_present(out + off, cap - off, /*mode*/0, /*src_handle*/1);

    off += osr_enc_anchor(out + off, cap - off, 1250, 8, 0x4f5347, "game_enter");
    return off;
}

int test_osr_replay_dispatches_all_kinds(void)
{
    uint8_t osr[1024];
    size_t len = build_sample_osr(osr, sizeof osr);
    T_ASSERT(len > OSR_HEADER_SIZE);

    FILE *f = fmemopen(osr, len, "rb");
    T_ASSERT(f != NULL);
    rec_sink r;
    osr_replay_sink sink = make_rec_sink(&r);
    T_ASSERT_EQ_I(osr_replay_stream(f, &sink), OSR_REPLAY_OK);
    fclose(f);

    T_ASSERT_EQ_I(r.n_header, 1);
    T_ASSERT_EQ_I(r.n_seed, 1);
    T_ASSERT_EQ_I(r.n_font, 1);
    T_ASSERT_EQ_I(r.n_sheet, 1);
    T_ASSERT_EQ_I(r.n_framebeg, 1);
    T_ASSERT_EQ_I(r.n_blit, 2);
    T_ASSERT_EQ_I(r.n_text, 1);
    T_ASSERT_EQ_I(r.n_present, 1);
    T_ASSERT_EQ_I(r.n_anchor, 1);
    T_ASSERT_EQ_I(r.n_snap, 1);
    T_ASSERT_EQ_I(r.n_clear, 1);

    /* header survived */
    T_ASSERT_EQ_U(r.last_header.screen_w, 640);
    T_ASSERT_EQ_U(r.last_header.pixfmt, OSR_PIXFMT_RGB565);
    T_ASSERT(strcmp(r.last_header.scenario, "intro-1") == 0);
    /* frame label */
    T_ASSERT_EQ_U(r.last_framebeg.flip, 1250);
    T_ASSERT_EQ_U(r.last_framebeg.sim_tick, 8);
    /* font */
    T_ASSERT_EQ_I(r.last_font.font_ref, 3);
    T_ASSERT(strcmp(r.last_font.face, "Courier New") == 0);
    /* seed */
    T_ASSERT_EQ_U(r.last_seed.before, 0xdead);
    T_ASSERT_EQ_U(r.last_seed.value, 0x4f5347);
    /* anchor name */
    T_ASSERT(strcmp(r.anchor_name, "game_enter") == 0);
    return 0;
}

int test_osr_replay_sheet_pixels_intact(void)
{
    uint8_t osr[1024];
    size_t len = build_sample_osr(osr, sizeof osr);
    FILE *f = fmemopen(osr, len, "rb");
    rec_sink r;
    osr_replay_sink sink = make_rec_sink(&r);
    T_ASSERT_EQ_I(osr_replay_stream(f, &sink), OSR_REPLAY_OK);
    fclose(f);

    T_ASSERT_EQ_U(r.sheet_dhash, 0xABCD1234);
    T_ASSERT_EQ_U(r.sheet_w, 2);
    T_ASSERT_EQ_U(r.sheet_h, 2);
    T_ASSERT_EQ_U(r.sheet_pix_len, 8);
    uint8_t expect[8] = { 0x11,0x22, 0x33,0x44, 0x55,0x66, 0x77,0x88 };
    T_ASSERT_MEM_EQ(r.sheet_pix, expect, 8);

    /* the SNAP record's pixels survive too (the M4d validate gate reads them) */
    T_ASSERT_EQ_U(r.snap_flip, 1250);
    T_ASSERT_EQ_U(r.snap_w, 2); T_ASSERT_EQ_U(r.snap_h, 2);
    T_ASSERT_EQ_U(r.snap_pix_len, 8);
    uint8_t snap_expect[8] = { 0xa0,0xa1, 0xb0,0xb1, 0xc0,0xc1, 0xd0,0xd1 };
    T_ASSERT_MEM_EQ(r.snap_pix, snap_expect, 8);
    return 0;
}

int test_osr_replay_preserves_draw_order(void)
{
    uint8_t osr[1024];
    size_t len = build_sample_osr(osr, sizeof osr);
    FILE *f = fmemopen(osr, len, "rb");
    rec_sink r;
    osr_replay_sink sink = make_rec_sink(&r);
    T_ASSERT_EQ_I(osr_replay_stream(f, &sink), OSR_REPLAY_OK);
    fclose(f);

    /* The frame issued clear(seq0), blit(seq1), text(seq2), blit(seq3) — the
     * replayer must see them in that exact dispatch order (seq interleaves). */
    T_ASSERT_EQ_I(r.n_ops, 4);
    T_ASSERT_EQ_U(r.op_seq[0], 0);
    T_ASSERT_EQ_U(r.op_seq[1], 1);
    T_ASSERT_EQ_U(r.op_seq[2], 2);
    T_ASSERT_EQ_U(r.op_seq[3], 3);
    /* the text record's fields */
    T_ASSERT_EQ_U(r.text_seq, 2);
    T_ASSERT_EQ_I(r.text_x, 64);
    T_ASSERT_EQ_I(r.text_y, 200);
    T_ASSERT_EQ_U(r.text_color, 0x3e537d);
    T_ASSERT(strcmp(r.text_str, "Arche") == 0);
    return 0;
}

int test_osr_replay_truncated_tail_recovers(void)
{
    uint8_t osr[1024];
    size_t len = build_sample_osr(osr, sizeof osr);
    /* Cut 4 bytes off the end — the trailing ANCHOR is now a truncated payload.
     * Everything BEFORE it must still be delivered and the stream must end
     * cleanly (a hard-killed capture is valid up to its last full record). */
    T_ASSERT(len > 4);
    FILE *f = fmemopen(osr, len - 4, "rb");
    rec_sink r;
    osr_replay_sink sink = make_rec_sink(&r);
    T_ASSERT_EQ_I(osr_replay_stream(f, &sink), OSR_REPLAY_OK);
    fclose(f);

    /* the present (just before the anchor) and all draws survived */
    T_ASSERT_EQ_I(r.n_present, 1);
    T_ASSERT_EQ_I(r.n_blit, 2);
    T_ASSERT_EQ_I(r.n_text, 1);
    /* the truncated anchor was dropped (not delivered, no crash) */
    T_ASSERT_EQ_I(r.n_anchor, 0);
    return 0;
}

int test_osr_replay_rejects_bad_header(void)
{
    uint8_t junk[OSR_HEADER_SIZE];
    memset(junk, 0, sizeof junk);
    junk[0] = 'X';                          /* bad magic */
    FILE *f = fmemopen(junk, sizeof junk, "rb");
    rec_sink r;
    osr_replay_sink sink = make_rec_sink(&r);
    T_ASSERT_EQ_I(osr_replay_stream(f, &sink), OSR_REPLAY_ERR_HEADER);
    fclose(f);
    T_ASSERT_EQ_I(r.n_header, 0);

    /* a file shorter than a header is also a header error */
    f = fmemopen(junk, 10, "rb");
    sink = make_rec_sink(&r);
    T_ASSERT_EQ_I(osr_replay_stream(f, &sink), OSR_REPLAY_ERR_HEADER);
    fclose(f);
    return 0;
}

int test_osr_replay_null_callbacks_skipped(void)
{
    /* An all-NULL sink (every callback NULL) must still walk the whole stream
     * to the end without dereferencing a NULL callback. */
    uint8_t osr[1024];
    size_t len = build_sample_osr(osr, sizeof osr);
    FILE *f = fmemopen(osr, len, "rb");
    T_ASSERT(f != NULL);
    osr_replay_sink sink = {0};      /* user + all callbacks NULL */
    T_ASSERT_EQ_I(osr_replay_stream(f, &sink), OSR_REPLAY_OK);
    fclose(f);

    /* a NULL sink pointer is a clean OPEN error, not a crash */
    f = fmemopen(osr, len, "rb");
    T_ASSERT_EQ_I(osr_replay_stream(f, NULL), OSR_REPLAY_ERR_OPEN);
    fclose(f);
    return 0;
}
