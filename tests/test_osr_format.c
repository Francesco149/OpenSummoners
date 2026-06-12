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
