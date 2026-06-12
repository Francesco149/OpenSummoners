/*
 * tests/test_osr_format.c — host round-trip tests for src/osr_format.h.
 *
 * The .osr format is the shared draw-stream codec written by BOTH the retail
 * capture proxy and the port emitter and read by the Windows reconstructor +
 * the Python studio.  These tests pin the wire layout (so the C and Python
 * decoders can't drift) and prove encode→decode is lossless, including the
 * variable-length ANCHOR record and the truncated-tail recovery a hard-killed
 * capture relies on.
 */
#include "../src/osr_format.h"
#include "t.h"

int test_osr_header_roundtrip(void)
{
    osr_header h = {0};
    h.side = OSR_SIDE_RETAIL;
    h.pixfmt = OSR_PIXFMT_RGB565;
    h.screen_w = 640; h.screen_h = 480;
    h.seed = 0x4f5347;
    h.flags = OSR_FLAG_TURBO | OSR_FLAG_LOCKSTEP | OSR_FLAG_SEED_PIN;
    strcpy(h.scenario, "intro-1");

    uint8_t buf[OSR_HEADER_SIZE];
    T_ASSERT_EQ_U(osr_header_encode(buf, sizeof(buf), &h), OSR_HEADER_SIZE);
    /* magic + version are the cross-tool contract */
    T_ASSERT(buf[0] == 'O' && buf[1] == 'S' && buf[2] == 'R' && buf[3] == '1');
    T_ASSERT_EQ_U(osr_get_u32(buf + 4), OSR_VERSION);

    osr_header g = {0};
    T_ASSERT(osr_header_decode(buf, sizeof(buf), &g) == 1);
    T_ASSERT_EQ_U(g.side, OSR_SIDE_RETAIL);
    T_ASSERT_EQ_U(g.pixfmt, OSR_PIXFMT_RGB565);
    T_ASSERT_EQ_U(g.screen_w, 640);
    T_ASSERT_EQ_U(g.screen_h, 480);
    T_ASSERT_EQ_U(g.seed, 0x4f5347);
    T_ASSERT_EQ_U(g.flags, OSR_FLAG_TURBO | OSR_FLAG_LOCKSTEP | OSR_FLAG_SEED_PIN);
    T_ASSERT(strcmp(g.scenario, "intro-1") == 0);
    return 0;
}

int test_osr_header_rejects_bad_magic(void)
{
    uint8_t buf[OSR_HEADER_SIZE] = {0};
    osr_header g;
    T_ASSERT(osr_header_decode(buf, sizeof(buf), &g) == 0);   /* zeroed = no magic */
    /* too-short buffer */
    T_ASSERT(osr_header_decode(buf, 10, &g) == 0);
    return 0;
}

int test_osr_header_encode_too_small(void)
{
    osr_header h = {0};
    uint8_t buf[10];
    T_ASSERT_EQ_U(osr_header_encode(buf, sizeof(buf), &h), 0);
    return 0;
}

/* Write FRAMEBEG/PRESENT/SEED/ANCHOR records into one buffer, then iterate +
 * decode them back, asserting field-exact recovery and correct framing. */
int test_osr_records_roundtrip(void)
{
    uint8_t buf[256];
    size_t off = 0, n;

    n = osr_enc_framebeg(buf + off, sizeof(buf) - off, 1242, 17, 0); T_ASSERT(n); off += n;
    n = osr_enc_present(buf + off, sizeof(buf) - off, 3, 0xabcd);    T_ASSERT(n); off += n;
    n = osr_enc_seed(buf + off, sizeof(buf) - off, 652, 0x111, 0x4f5347); T_ASSERT(n); off += n;
    n = osr_enc_anchor(buf + off, sizeof(buf) - off, 1242, 0, 0x4f5347, "game_enter");
    T_ASSERT(n); off += n;

    const uint8_t *p = buf, *end = buf + off;
    uint32_t type, plen; const uint8_t *pay;
    int count = 0;

    p = osr_rec_next(p, end, &type, &pay, &plen);
    T_ASSERT(p != NULL); T_ASSERT_EQ_U(type, OSR_FRAMEBEG);
    osr_framebeg fb;
    T_ASSERT(osr_dec_framebeg(pay, plen, &fb));
    T_ASSERT_EQ_U(fb.flip, 1242); T_ASSERT_EQ_U(fb.sim_tick, 17);
    T_ASSERT_EQ_U(fb.anchor_id, 0);
    count++;

    p = osr_rec_next(p, end, &type, &pay, &plen);
    T_ASSERT(p != NULL); T_ASSERT_EQ_U(type, OSR_PRESENT);
    osr_present pr;
    T_ASSERT(osr_dec_present(pay, plen, &pr));
    T_ASSERT_EQ_U(pr.mode, 3); T_ASSERT_EQ_U(pr.src_handle, 0xabcd);
    count++;

    p = osr_rec_next(p, end, &type, &pay, &plen);
    T_ASSERT(p != NULL); T_ASSERT_EQ_U(type, OSR_SEED);
    osr_seed sd;
    T_ASSERT(osr_dec_seed(pay, plen, &sd));
    T_ASSERT_EQ_U(sd.flip, 652); T_ASSERT_EQ_U(sd.before, 0x111);
    T_ASSERT_EQ_U(sd.value, 0x4f5347);
    count++;

    p = osr_rec_next(p, end, &type, &pay, &plen);
    T_ASSERT(p != NULL); T_ASSERT_EQ_U(type, OSR_ANCHOR);
    osr_anchor an;
    T_ASSERT(osr_dec_anchor(pay, plen, &an));
    T_ASSERT_EQ_U(an.flip, 1242); T_ASSERT_EQ_U(an.sim_tick, 0);
    T_ASSERT_EQ_U(an.rng, 0x4f5347);
    T_ASSERT_EQ_U(an.name_len, 10);
    T_ASSERT(memcmp(an.name, "game_enter", 10) == 0);
    count++;

    /* exact end: the next call sees no complete record */
    T_ASSERT(osr_rec_next(p, end, &type, &pay, &plen) == NULL);
    T_ASSERT_EQ_I(count, 4);
    T_ASSERT_EQ_P(p, end);   /* consumed exactly */
    return 0;
}

/* The BLIT record (M3b) pins render_diff.py's draw-stream schema; encode→decode
 * must be byte-exact including the signed-geometry fields and the 76-byte size
 * (the Python reader struct.unpack's the same fixed layout). */
int test_osr_blit_roundtrip(void)
{
    osr_blit b = {0};
    b.va = 0x5b9bf0; b.seq = 7;
    b.res = 0x91b; b.frame = 5;
    b.dhash = 0x77c7a334; b.dst_handle = 0xdead;
    b.dx = 12; b.dy = -34; b.reqw = 4; b.reqh = 48;
    b.sx = -8; b.sy = 16;
    b.ow = 64; b.oh = 4; b.ox = -2; b.oy = -3;
    b.state = 0x8000; b.ckey = 0x1f; b.bmode = -1; b.mode = 3;

    uint8_t buf[8 + OSR_BLIT_PAYLOAD];
    size_t n = osr_enc_blit(buf, sizeof(buf), &b);
    T_ASSERT_EQ_U(n, 8 + OSR_BLIT_PAYLOAD);
    T_ASSERT_EQ_U(OSR_BLIT_PAYLOAD, 76);

    uint32_t type, plen; const uint8_t *pay;
    const uint8_t *p = osr_rec_next(buf, buf + n, &type, &pay, &plen);
    T_ASSERT(p == buf + n);                 /* consumed exactly */
    T_ASSERT_EQ_U(type, OSR_BLIT);
    T_ASSERT_EQ_U(plen, OSR_BLIT_PAYLOAD);

    osr_blit g = {0};
    T_ASSERT(osr_dec_blit(pay, plen, &g));
    T_ASSERT_EQ_U(g.va, 0x5b9bf0); T_ASSERT_EQ_U(g.seq, 7);
    T_ASSERT_EQ_U(g.res, 0x91b);   T_ASSERT_EQ_U(g.frame, 5);
    T_ASSERT_EQ_U(g.dhash, 0x77c7a334); T_ASSERT_EQ_U(g.dst_handle, 0xdead);
    T_ASSERT_EQ_I(g.dx, 12);  T_ASSERT_EQ_I(g.dy, -34);
    T_ASSERT_EQ_I(g.reqw, 4); T_ASSERT_EQ_I(g.reqh, 48);
    T_ASSERT_EQ_I(g.sx, -8);  T_ASSERT_EQ_I(g.sy, 16);
    T_ASSERT_EQ_I(g.ow, 64);  T_ASSERT_EQ_I(g.oh, 4);
    T_ASSERT_EQ_I(g.ox, -2);  T_ASSERT_EQ_I(g.oy, -3);
    T_ASSERT_EQ_U(g.state, 0x8000); T_ASSERT_EQ_U(g.ckey, 0x1f);
    T_ASSERT_EQ_I(g.bmode, -1); T_ASSERT_EQ_U(g.mode, 3);

    osr_blit small = {0};
    T_ASSERT_EQ_U(osr_dec_blit(pay, OSR_BLIT_PAYLOAD - 1, &small), 0);  /* short */
    return 0;
}

/* SHEET — the variable-length dedup'd source-pixel record (M3c).  Verify the
 * 24-byte prefix + the trailing pixel bytes round-trip, framing included (the
 * Python reader struct.unpack's the same <I HH HH I BB xx I> prefix). */
int test_osr_sheet_roundtrip(void)
{
    static const uint8_t pix[7] = { 0xde, 0xad, 0xbe, 0xef, 0x01, 0x02, 0x03 };
    osr_sheet s = {0};
    s.dhash = 0x12345678; s.res = 0x91b; s.frame = 5;
    s.w = 64; s.h = 48; s.pitch = 128;
    s.pixfmt = OSR_PIXFMT_RGB565; s.codec = 0;
    s.byte_len = sizeof(pix); s.bytes = pix;

    uint8_t buf[8 + OSR_SHEET_HDR + sizeof(pix)];
    size_t n = osr_enc_sheet(buf, sizeof(buf), &s);
    T_ASSERT_EQ_U(n, 8 + OSR_SHEET_HDR + sizeof(pix));
    T_ASSERT_EQ_U(OSR_SHEET_HDR, 24);

    uint32_t type, plen; const uint8_t *pay;
    const uint8_t *p = osr_rec_next(buf, buf + n, &type, &pay, &plen);
    T_ASSERT(p == buf + n);                 /* consumed exactly */
    T_ASSERT_EQ_U(type, OSR_SHEET);
    T_ASSERT_EQ_U(plen, OSR_SHEET_HDR + sizeof(pix));

    osr_sheet g = {0};
    T_ASSERT(osr_dec_sheet(pay, plen, &g));
    T_ASSERT_EQ_U(g.dhash, 0x12345678);
    T_ASSERT_EQ_U(g.res, 0x91b); T_ASSERT_EQ_U(g.frame, 5);
    T_ASSERT_EQ_U(g.w, 64); T_ASSERT_EQ_U(g.h, 48); T_ASSERT_EQ_U(g.pitch, 128);
    T_ASSERT_EQ_U(g.pixfmt, OSR_PIXFMT_RGB565); T_ASSERT_EQ_U(g.codec, 0);
    T_ASSERT_EQ_U(g.byte_len, sizeof(pix));
    T_ASSERT(memcmp(g.bytes, pix, sizeof(pix)) == 0);

    /* a header claiming more bytes than the slice holds must be rejected */
    osr_sheet bad = {0};
    T_ASSERT_EQ_U(osr_dec_sheet(pay, OSR_SHEET_HDR - 1, &bad), 0);  /* short hdr */
    return 0;
}

/* FONT — the dedup'd LOGFONTA record (M3d).  Fixed 64-byte payload; the Python
 * reader struct.unpack's the same <I iiiii BBBB BBBB> prefix + 32-byte face. */
int test_osr_font_roundtrip(void)
{
    osr_font f = {0};
    f.font_ref = 3;
    f.height = 18; f.width = 7; f.escapement = 0; f.orientation = 0; f.weight = 400;
    f.italic = 0; f.underline = 0; f.strikeout = 0; f.charset = 128 /* SHIFTJIS */;
    f.out_prec = 3; f.clip_prec = 2; f.quality = 1; f.pitch_family = 0x31;
    strcpy(f.face, "Courier New");

    uint8_t buf[8 + OSR_FONT_PAYLOAD];
    size_t n = osr_enc_font(buf, sizeof(buf), &f);
    T_ASSERT_EQ_U(n, 8 + OSR_FONT_PAYLOAD);
    T_ASSERT_EQ_U(OSR_FONT_PAYLOAD, 64);

    uint32_t type, plen; const uint8_t *pay;
    const uint8_t *p = osr_rec_next(buf, buf + n, &type, &pay, &plen);
    T_ASSERT(p == buf + n);
    T_ASSERT_EQ_U(type, OSR_FONT);
    T_ASSERT_EQ_U(plen, OSR_FONT_PAYLOAD);

    osr_font g = {0};
    T_ASSERT(osr_dec_font(pay, plen, &g));
    T_ASSERT_EQ_U(g.font_ref, 3);
    T_ASSERT_EQ_I(g.height, 18); T_ASSERT_EQ_I(g.width, 7);
    T_ASSERT_EQ_I(g.weight, 400);
    T_ASSERT_EQ_U(g.charset, 128);
    T_ASSERT_EQ_U(g.out_prec, 3); T_ASSERT_EQ_U(g.clip_prec, 2);
    T_ASSERT_EQ_U(g.quality, 1);  T_ASSERT_EQ_U(g.pitch_family, 0x31);
    T_ASSERT(strcmp(g.face, "Courier New") == 0);

    osr_font small = {0};
    T_ASSERT_EQ_U(osr_dec_font(pay, OSR_FONT_PAYLOAD - 1, &small), 0);  /* short */
    return 0;
}

/* TEXT — the variable-length GDI TextOut record (M3d).  Verify the 32-byte prefix
 * (incl. the signed x/y/bk_mode) + the trailing string round-trip, framing too. */
int test_osr_text_roundtrip(void)
{
    static const char s[] = "Ahh";
    osr_text t = {0};
    t.seq = 41; t.dst_handle = 0xbeef;
    t.x = 72; t.y = -3;
    t.font_ref = 3; t.color = 0x3e537d; t.bk_mode = 1 /* TRANSPARENT */;
    t.str_len = (uint32_t)strlen(s); t.str = s;

    uint8_t buf[8 + OSR_TEXT_HDR + sizeof(s)];
    size_t n = osr_enc_text(buf, sizeof(buf), &t);
    T_ASSERT_EQ_U(n, 8 + OSR_TEXT_HDR + strlen(s));
    T_ASSERT_EQ_U(OSR_TEXT_HDR, 32);

    uint32_t type, plen; const uint8_t *pay;
    const uint8_t *p = osr_rec_next(buf, buf + n, &type, &pay, &plen);
    T_ASSERT(p == buf + n);
    T_ASSERT_EQ_U(type, OSR_TEXT);
    T_ASSERT_EQ_U(plen, OSR_TEXT_HDR + strlen(s));

    osr_text g = {0};
    T_ASSERT(osr_dec_text(pay, plen, &g));
    T_ASSERT_EQ_U(g.seq, 41); T_ASSERT_EQ_U(g.dst_handle, 0xbeef);
    T_ASSERT_EQ_I(g.x, 72); T_ASSERT_EQ_I(g.y, -3);
    T_ASSERT_EQ_U(g.font_ref, 3); T_ASSERT_EQ_U(g.color, 0x3e537d);
    T_ASSERT_EQ_I(g.bk_mode, 1);
    T_ASSERT_EQ_U(g.str_len, strlen(s));
    T_ASSERT(memcmp(g.str, s, strlen(s)) == 0);

    /* a header claiming more string bytes than the slice holds must be rejected */
    osr_text bad = {0};
    T_ASSERT_EQ_U(osr_dec_text(pay, OSR_TEXT_HDR - 1, &bad), 0);  /* short hdr */
    return 0;
}

int test_osr_anchor_empty_name(void)
{
    uint8_t buf[64];
    size_t n = osr_enc_anchor(buf, sizeof(buf), 5, 6, 7, NULL);
    T_ASSERT_EQ_U(n, 8 + 16 + 0);
    uint32_t type, plen; const uint8_t *pay;
    T_ASSERT(osr_rec_next(buf, buf + n, &type, &pay, &plen) != NULL);
    T_ASSERT_EQ_U(type, OSR_ANCHOR);
    osr_anchor an;
    T_ASSERT(osr_dec_anchor(pay, plen, &an));
    T_ASSERT_EQ_U(an.name_len, 0);
    T_ASSERT_EQ_U(an.flip, 5); T_ASSERT_EQ_U(an.sim_tick, 6); T_ASSERT_EQ_U(an.rng, 7);
    return 0;
}

/* A capture hard-killed mid-record leaves a truncated tail; the reader must
 * stop cleanly at the last COMPLETE record, never read past the buffer. */
int test_osr_truncated_tail_recovers(void)
{
    uint8_t buf[64];
    size_t off = 0, n;
    n = osr_enc_framebeg(buf + off, sizeof(buf) - off, 1, 1, 0); off += n;
    n = osr_enc_present(buf + off, sizeof(buf) - off, 0, 0);     off += n;
    size_t good = off;
    /* simulate a half-written third record: 8-byte header claims 12 bytes
     * payload but only 4 are present */
    osr_put_u32(buf + off, OSR_SEED);
    osr_put_u32(buf + off + 4, 12);
    off += 8 + 4;   /* only 4 of the 12 payload bytes landed before the kill */

    const uint8_t *p = buf, *end = buf + off;
    uint32_t type, plen; const uint8_t *pay;
    int count = 0;
    while ((p = osr_rec_next(p, end, &type, &pay, &plen)) != NULL) count++;
    T_ASSERT_EQ_I(count, 2);               /* only the two complete records */
    /* the cursor that stopped is at the start of the torn record */
    /* (re-walk to confirm it stops exactly at `good`) */
    p = buf;
    p = osr_rec_next(p, end, &type, &pay, &plen);
    p = osr_rec_next(p, end, &type, &pay, &plen);
    T_ASSERT_EQ_P(p, buf + good);
    T_ASSERT(osr_rec_next(p, end, &type, &pay, &plen) == NULL);
    return 0;
}
