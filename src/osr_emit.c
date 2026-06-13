/*
 * src/osr_emit.c — the PORT-side ".osr" draw-stream emitter (Trace Studio v2 M5).
 * See osr_emit.h for the sink map and the identity conventions.
 *
 * Pure C / no Win32 (host-linkable): the writer is buffered stdio on the
 * caller's path; surface pixels come through the injected reader.  Single-
 * threaded by construction — every sink runs on the port's render thread.
 */
#include "osr_emit.h"

#include <stdio.h>
#include <string.h>

#include "zdd.h"
#include "render_id.h"

/* ── writer state ─────────────────────────────────────────────────────────── */
static FILE   *g_oe_fp;
static int     g_oe_frame_open;
static uint32_t g_oe_seq;            /* per-frame draw ordinal (BLIT/TEXT/CLEAR) */

/* stats for the close summary */
static long g_oe_n_blits, g_oe_n_sheets, g_oe_n_texts, g_oe_n_clears,
            g_oe_n_fonts, g_oe_n_blends, g_oe_n_frames, g_oe_n_grab_fails;

static const struct zdd_object *g_oe_primary;
static osr_emit_surf_read_fn    g_oe_surf_read;
static osr_emit_surf_done_fn    g_oe_surf_done;

/* The write buffer for fixed-size records (the largest is FONT's 8+64). */
static void oe_write(const uint8_t *buf, size_t n)
{
    if (n != 0 && g_oe_fp != NULL) fwrite(buf, 1, n, g_oe_fp);
}

int osr_emit_is_active(void) { return g_oe_fp != NULL; }

/* ── dest-handle intern (mirrors the proxy's surface_id.h, but the port's
 *    stream only ever emits the primary = handle 1; kept a table anyway so a
 *    future offscreen-dest record costs one slot, not a redesign) ─────────── */
#define OE_DST_SLOTS 64u
static const void *g_oe_dst_key[OE_DST_SLOTS];
static uint32_t    g_oe_dst_handle[OE_DST_SLOTS];
static uint32_t    g_oe_dst_next = 1;

static uint32_t oe_dst_intern(const void *obj)
{
    if (obj == NULL) return 0;
    for (unsigned i = 0; i < OE_DST_SLOTS; i++) {
        if (g_oe_dst_key[i] == obj) return g_oe_dst_handle[i];
        if (g_oe_dst_key[i] == NULL) {
            g_oe_dst_key[i] = obj;
            g_oe_dst_handle[i] = g_oe_dst_next++;
            return g_oe_dst_handle[i];
        }
    }
    return 0;   /* table full — unidentified dest */
}

/* ── sheet-grab cache: zdd_object* → dhash (mirrors sheet_grab.h, including
 *    the ckpt-126 tombstoned eviction so a recycled pointer re-grabs) ──────── */
#define OE_SG_SLOTS 4096u
#define OE_SG_TOMB  ((const void *)1)

typedef struct { const void *key; uint32_t dhash; } oe_sg_entry;
static oe_sg_entry g_oe_sg[OE_SG_SLOTS];

static unsigned oe_sg_slot(const void *p)
{
    uintptr_t x = (uintptr_t)p;
    x ^= x >> 16; x *= 0x9E3779B1u; x ^= x >> 13;
    return (unsigned)(x & (OE_SG_SLOTS - 1u));
}

/* emitted-dhash set: skip re-EMITTING identical content after an evict (the
 * reader dedups by dhash anyway; this keeps the file small). */
static uint32_t g_oe_emitted[OE_SG_SLOTS];

static int oe_emitted_test_and_set(uint32_t dhash)
{
    if (dhash == 0) return 0;
    unsigned h = (unsigned)((dhash * 0x9E3779B1u) & (OE_SG_SLOTS - 1u));
    for (unsigned i = 0; i < OE_SG_SLOTS; i++) {
        unsigned s = (h + i) & (OE_SG_SLOTS - 1u);
        if (g_oe_emitted[s] == dhash) return 1;
        if (g_oe_emitted[s] == 0) { g_oe_emitted[s] = dhash; return 0; }
    }
    return 1;   /* set full — claim "already emitted" (best-effort) */
}

void osr_emit_evict(struct zdd_object *obj)
{
    if (obj == NULL) return;
    unsigned h = oe_sg_slot(obj);
    for (unsigned i = 0; i < OE_SG_SLOTS; i++) {
        unsigned s = (h + i) & (OE_SG_SLOTS - 1u);
        const void *k = g_oe_sg[s].key;
        if (k == obj) { g_oe_sg[s].key = OE_SG_TOMB; g_oe_sg[s].dhash = 0; return; }
        if (k == NULL) return;
    }
}

/* Cap a single SHEET's pixel payload (matches the proxy's SG_MAX_BYTES). */
#define OE_SG_MAX_BYTES (8u * 1024u * 1024u)

static uint8_t oe_pixfmt_of_bitcount(uint16_t bitcount)
{
    if (bitcount == 16) return OSR_PIXFMT_RGB565;
    if (bitcount == 32) return OSR_PIXFMT_XRGB8888;
    if (bitcount == 8)  return OSR_PIXFMT_PAL8;
    return OSR_PIXFMT_UNKNOWN;
}

/* Grab (or recall) the source object's surface pixels → dhash; emit the SHEET
 * on first sighting.  Returns 0 on a failed grab (BLIT.dhash stays 0). */
static uint32_t oe_sheet_capture(struct zdd_object *src,
                                 uint16_t res, uint16_t frame)
{
    if (src == NULL) return 0;

    unsigned h = oe_sg_slot(src);
    unsigned ins = OE_SG_SLOTS;
    for (unsigned i = 0; i < OE_SG_SLOTS; i++) {
        unsigned s = (h + i) & (OE_SG_SLOTS - 1u);
        const void *k = g_oe_sg[s].key;
        if (k == src) return g_oe_sg[s].dhash;     /* cached — the hot path */
        if (k == OE_SG_TOMB) { if (ins == OE_SG_SLOTS) ins = s; continue; }
        if (k == NULL) { if (ins == OE_SG_SLOTS) ins = s; break; }
    }
    if (ins == OE_SG_SLOTS) return 0;              /* cache full */

    osr_emit_surf sf;
    memset(&sf, 0, sizeof sf);
    if (g_oe_surf_read == NULL || !g_oe_surf_read(src, &sf) ||
        sf.pixels == NULL || sf.pitch == 0 || sf.w == 0 || sf.h == 0) {
        /* Cache the miss as dhash 0 so a problem surface isn't re-locked on
         * every blit (the proxy's failed-Lock idiom). */
        g_oe_sg[ins].key = src; g_oe_sg[ins].dhash = 0;
        g_oe_n_grab_fails++;
        return 0;
    }

    size_t bytes = (size_t)sf.pitch * sf.h;
    if (bytes > OE_SG_MAX_BYTES) bytes = OE_SG_MAX_BYTES;

    /* dhash: the proxy's seed order — w:u32, h:u32, bitcount:u16, pixels. */
    uint32_t w32 = sf.w, h32 = sf.h;
    uint16_t bc = sf.bitcount;
    uint32_t seed = render_id_hash_seed(RENDER_ID_FNV1A_SEED, &w32, sizeof w32);
    seed = render_id_hash_seed(seed, &h32, sizeof h32);
    seed = render_id_hash_seed(seed, &bc, sizeof bc);
    uint32_t dhash = render_id_hash_seed(seed, sf.pixels, bytes);

    if (!oe_emitted_test_and_set(dhash)) {
        osr_sheet sh;
        memset(&sh, 0, sizeof sh);
        sh.dhash    = dhash;
        sh.res      = res;
        sh.frame    = frame;
        sh.w        = (uint16_t)sf.w;
        sh.h        = (uint16_t)sf.h;
        sh.pitch    = sf.pitch;
        sh.pixfmt   = oe_pixfmt_of_bitcount(sf.bitcount);
        sh.codec    = 0;     /* raw; miniz is PORT-DEBT(osr-sheet-compression) */
        sh.byte_len = (uint32_t)bytes;
        uint8_t pre[8 + OSR_SHEET_HDR];
        size_t n = osr_enc_sheet_prefix(pre, sizeof pre, &sh);
        oe_write(pre, n);
        if (g_oe_fp != NULL) fwrite(sf.pixels, 1, bytes, g_oe_fp);
        g_oe_n_sheets++;
    }

    if (g_oe_surf_done != NULL) g_oe_surf_done(src);

    g_oe_sg[ins].key = src; g_oe_sg[ins].dhash = dhash;
    return dhash;
}

/* ── BLEND registry: content-dedup'd descriptor → blend_ref ──────────────────
 * LUT byte lengths mirror the proxy's bg_lut_len / zdd_blend_pixel exactly:
 *   mode 0: maxch + 1;  mode 1: (maxch<<5) + maxch + 1;  mode 2: gray_max + 1
 * with maxch = mask>>shift per channel and gray_max = (Σ maxch)/3. */
#define OE_BLEND_CAP 256
static uint32_t g_oe_blend_hash[OE_BLEND_CAP];   /* content hash; ref = idx+1 */
static uint32_t g_oe_blend_next;                 /* number registered */

static uint32_t oe_blend_lut_len(int32_t mode, uint32_t maxch, uint32_t gray_max)
{
    if (mode == 1) return (maxch << 5) + maxch + 1u;
    if (mode == 2) return gray_max + 1u;
    return maxch + 1u;
}

static uint32_t oe_blend_register(const struct zdd_blend_desc *desc)
{
    if (desc == NULL) return 0;

    uint32_t maxch[3], lut_len[3], gray_max = 0;
    for (int i = 0; i < 3; i++) {
        int32_t sh = desc->ch[i].shift;
        maxch[i] = (sh >= 0 && sh < 32) ? (desc->ch[i].mask >> sh) : 0;
        gray_max += maxch[i];
    }
    gray_max /= 3u;
    for (int i = 0; i < 3; i++) {
        lut_len[i] = desc->ch[i].lut != NULL
                   ? oe_blend_lut_len(desc->mode, maxch[i], gray_max) : 0;
        if (lut_len[i] > 8192u) lut_len[i] = 8192u;  /* proxy's BG_LUT_CAP */
    }

    /* content hash over mode + per-channel shift/mask/lut bytes */
    uint32_t ch = render_id_hash_seed(RENDER_ID_FNV1A_SEED,
                                      &desc->mode, sizeof desc->mode);
    for (int i = 0; i < 3; i++) {
        ch = render_id_hash_seed(ch, &desc->ch[i].shift, sizeof desc->ch[i].shift);
        ch = render_id_hash_seed(ch, &desc->ch[i].mask,  sizeof desc->ch[i].mask);
        if (lut_len[i]) ch = render_id_hash_seed(ch, desc->ch[i].lut, lut_len[i]);
    }
    if (ch == 0) ch = 1;

    for (uint32_t i = 0; i < g_oe_blend_next; i++)
        if (g_oe_blend_hash[i] == ch) return i + 1;
    if (g_oe_blend_next >= OE_BLEND_CAP) return 0;

    uint32_t ref = ++g_oe_blend_next;
    g_oe_blend_hash[ref - 1] = ch;

    /* The 3 LUTs are separate engine buffers, so don't bounce them through a
     * scratch concat — write the framed 44-byte prefix, then stream each LUT
     * straight from the descriptor (the SHEET one-copy discipline). */
    uint32_t lut_total = lut_len[0] + lut_len[1] + lut_len[2];
    uint8_t p[8 + OSR_BLEND_HDR];
    osr_put_u32(p, OSR_BLEND);
    osr_put_u32(p + 4, OSR_BLEND_HDR + lut_total);
    uint8_t *q = p + 8;
    osr_put_u32(q + 0, ref);
    osr_put_u32(q + 4, (uint32_t)desc->mode);
    for (int i = 0; i < 3; i++) osr_put_u32(q +  8 + i * 4, (uint32_t)desc->ch[i].shift);
    for (int i = 0; i < 3; i++) osr_put_u32(q + 20 + i * 4, desc->ch[i].mask);
    for (int i = 0; i < 3; i++) osr_put_u32(q + 32 + i * 4, lut_len[i]);
    oe_write(p, sizeof p);
    for (int i = 0; i < 3; i++)
        if (lut_len[i] && g_oe_fp != NULL)
            fwrite(desc->ch[i].lut, 1, lut_len[i], g_oe_fp);
    g_oe_n_blends++;
    return ref;
}

/* ── FONT registry: hfont → font_ref ─────────────────────────────────────── */
#define OE_FONT_CAP 64
static const void *g_oe_font_key[OE_FONT_CAP];   /* HFONT; ref = idx+1 */
static uint32_t    g_oe_font_next;

static uint32_t oe_font_ref_of(const void *hfont)
{
    if (hfont == NULL) return 0;
    for (uint32_t i = 0; i < g_oe_font_next; i++)
        if (g_oe_font_key[i] == hfont) return i + 1;
    return 0;
}

void osr_emit_font_create(void *hfont, const osr_font *f)
{
    if (g_oe_fp == NULL || hfont == NULL || f == NULL) return;
    if (oe_font_ref_of(hfont) != 0) return;          /* already registered */
    if (g_oe_font_next >= OE_FONT_CAP) return;

    uint32_t ref = ++g_oe_font_next;
    g_oe_font_key[ref - 1] = hfont;

    osr_font rec = *f;
    rec.font_ref = ref;
    uint8_t buf[8 + OSR_FONT_PAYLOAD];
    oe_write(buf, osr_enc_font(buf, sizeof buf, &rec));
    g_oe_n_fonts++;
}

/* ── per-HDC GDI shadow (the proxy's engine_gdi.h per-DC state) ────────────── */
#define OE_DC_CAP 8
typedef struct {
    const void *hdc;                /* NULL = free slot */
    const struct zdd_object *obj;   /* the surface the DC came from */
    uint32_t font_ref;
    uint32_t color;
    int32_t  bk_mode;
} oe_dc_state;
static oe_dc_state g_oe_dc[OE_DC_CAP];

static oe_dc_state *oe_dc_find(const void *hdc)
{
    if (hdc == NULL) return NULL;
    for (int i = 0; i < OE_DC_CAP; i++)
        if (g_oe_dc[i].hdc == hdc) return &g_oe_dc[i];
    return NULL;
}

void osr_emit_dc_open(struct zdd_object *obj, void *hdc)
{
    if (g_oe_fp == NULL || hdc == NULL) return;
    oe_dc_state *d = oe_dc_find(hdc);
    if (d == NULL) {
        for (int i = 0; i < OE_DC_CAP; i++)
            if (g_oe_dc[i].hdc == NULL) { d = &g_oe_dc[i]; break; }
        if (d == NULL) return;
    }
    d->hdc      = hdc;
    d->obj      = obj;
    d->font_ref = 0;
    d->color    = 0;       /* GDI defaults: black text… */
    d->bk_mode  = 2;       /* …OPAQUE background */
}

void osr_emit_dc_close(void *hdc)
{
    oe_dc_state *d = oe_dc_find(hdc);
    if (d != NULL) memset(d, 0, sizeof *d);
}

void osr_emit_gdi_select_font(void *hdc, void *hfont)
{
    oe_dc_state *d = oe_dc_find(hdc);
    if (d != NULL) d->font_ref = oe_font_ref_of(hfont);
}

void osr_emit_gdi_color(void *hdc, uint32_t color)
{
    oe_dc_state *d = oe_dc_find(hdc);
    if (d != NULL) d->color = color;
}

void osr_emit_gdi_bkmode(void *hdc, int32_t bk_mode)
{
    oe_dc_state *d = oe_dc_find(hdc);
    if (d != NULL) d->bk_mode = bk_mode;
}

void osr_emit_gdi_text(void *hdc, int32_t x, int32_t y,
                       const char *str, int32_t len)
{
    if (g_oe_fp == NULL || str == NULL || len <= 0) return;
    oe_dc_state *d = oe_dc_find(hdc);
    if (d == NULL || d->obj == NULL || d->obj != g_oe_primary) return;

    osr_text t;
    memset(&t, 0, sizeof t);
    t.seq        = g_oe_seq++;
    t.dst_handle = oe_dst_intern(d->obj);
    t.x          = x;
    t.y          = y;
    t.font_ref   = d->font_ref;
    t.color      = d->color;
    t.bk_mode    = d->bk_mode;
    t.str_len    = (uint32_t)len;
    t.str        = str;
    uint8_t buf[8 + OSR_TEXT_HDR + 512];
    size_t n = osr_enc_text(buf, sizeof buf, &t);
    if (n == 0) {        /* very long string — frame + prefix, stream the rest */
        uint8_t p[8 + OSR_TEXT_HDR];
        osr_put_u32(p, OSR_TEXT);
        osr_put_u32(p + 4, OSR_TEXT_HDR + t.str_len);
        uint8_t *q = p + 8;
        osr_put_u32(q +  0, t.seq);
        osr_put_u32(q +  4, t.dst_handle);
        osr_put_u32(q +  8, (uint32_t)t.x);
        osr_put_u32(q + 12, (uint32_t)t.y);
        osr_put_u32(q + 16, t.font_ref);
        osr_put_u32(q + 20, t.color);
        osr_put_u32(q + 24, (uint32_t)t.bk_mode);
        osr_put_u32(q + 28, t.str_len);
        oe_write(p, sizeof p);
        if (g_oe_fp != NULL) fwrite(str, 1, t.str_len, g_oe_fp);
    } else {
        oe_write(buf, n);
    }
    g_oe_n_texts++;
}

/* ── frame stream ─────────────────────────────────────────────────────────── */
void osr_emit_flip(uint32_t flip, uint32_t sim_tick)
{
    if (g_oe_fp == NULL) return;
    uint8_t buf[32];
    if (g_oe_frame_open) {
        oe_write(buf, osr_enc_present(buf, sizeof buf, 0, 0));
    }
    oe_write(buf, osr_enc_framebeg(buf, sizeof buf, flip, sim_tick, 0));
    g_oe_frame_open = 1;
    g_oe_seq = 0;
    g_oe_n_frames++;
}

void osr_emit_anchor(const char *name, uint32_t flip, uint32_t sim_tick,
                     uint32_t rng)
{
    if (g_oe_fp == NULL) return;
    uint8_t buf[8 + 16 + 64];
    char nm[48];
    if (name == NULL) name = "";
    snprintf(nm, sizeof nm, "%s", name);
    oe_write(buf, osr_enc_anchor(buf, sizeof buf, flip, sim_tick, rng, nm));
}

void osr_emit_seed(uint32_t flip, uint32_t before, uint32_t value)
{
    if (g_oe_fp == NULL) return;
    uint8_t buf[32];
    oe_write(buf, osr_enc_seed(buf, sizeof buf, flip, before, value));
}

/* ── draw sinks ───────────────────────────────────────────────────────────── */
void osr_emit_blit(uint32_t va, uint32_t mode,
                   struct zdd_object *src, struct zdd_object *dest,
                   int32_t dx, int32_t dy, int32_t reqw, int32_t reqh,
                   int32_t sx, int32_t sy, int32_t srcw, int32_t srch,
                   const struct zdd_blend_desc *desc, uint32_t ckey)
{
    if (g_oe_fp == NULL || src == NULL) return;
    if (dest == NULL || (const struct zdd_object *)dest != g_oe_primary)
        return;   /* primary-dest only (mirrors retail's observed stream) */

    render_id rid;
    int named = render_id_lookup(src, &rid);

    osr_blit b;
    memset(&b, 0, sizeof b);
    b.va         = va;
    b.seq        = g_oe_seq++;
    b.res        = named ? rid.resource_id : 0;
    b.frame      = named ? rid.frame : 0;
    b.dhash      = oe_sheet_capture(src, b.res, b.frame);
    b.dst_handle = oe_dst_intern(dest);
    b.dx = dx; b.dy = dy; b.reqw = reqw; b.reqh = reqh;
    b.sx = sx; b.sy = sy;
    b.ow = src->metric_b8; b.oh = src->metric_bc;
    b.ox = src->metric_0c; b.oy = src->metric_10;
    b.state = (uint32_t)src->state_flag;
    b.ckey  = ckey;
    b.bmode = desc != NULL ? desc->mode : -1;
    b.mode  = mode;
    b.blend_ref = desc != NULL ? oe_blend_register(desc) : 0;
    b.srcw = srcw; b.srch = srch;

    uint8_t buf[8 + OSR_BLIT_PAYLOAD];
    oe_write(buf, osr_enc_blit(buf, sizeof buf, &b));
    g_oe_n_blits++;
}

void osr_emit_clear(struct zdd_object *dest, uint32_t value)
{
    if (g_oe_fp == NULL || dest == NULL) return;
    if ((const struct zdd_object *)dest != g_oe_primary) return;
    uint8_t buf[32];
    oe_write(buf, osr_enc_clear(buf, sizeof buf, g_oe_seq++,
                                oe_dst_intern(dest), value));
    g_oe_n_clears++;
}

/* ── lifecycle ────────────────────────────────────────────────────────────── */
void osr_emit_set_primary(const struct zdd_object *primary)
{
    g_oe_primary = primary;
    if (primary != NULL) (void)oe_dst_intern(primary);   /* handle 1 */
}

void osr_emit_set_surf_reader(osr_emit_surf_read_fn read_fn,
                              osr_emit_surf_done_fn done_fn)
{
    g_oe_surf_read = read_fn;
    g_oe_surf_done = done_fn;
}

int osr_emit_open(const char *path, uint32_t seed, uint32_t flags,
                  const char *scenario, uint16_t screen_w, uint16_t screen_h,
                  uint8_t pixfmt)
{
    if (g_oe_fp != NULL || path == NULL) return 0;
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "[osr-emit] fopen('%s') FAILED\n", path);
        return 0;
    }
    setvbuf(fp, NULL, _IOFBF, 1u << 20);

    osr_header h;
    memset(&h, 0, sizeof h);
    h.side     = OSR_SIDE_PORT;
    h.pixfmt   = pixfmt;
    h.screen_w = screen_w;
    h.screen_h = screen_h;
    h.seed     = seed;
    h.flags    = flags;
    if (scenario != NULL)
        snprintf(h.scenario, sizeof h.scenario, "%s", scenario);

    uint8_t buf[OSR_HEADER_SIZE];
    size_t n = osr_header_encode(buf, sizeof buf, &h);
    if (n == 0 || fwrite(buf, 1, n, fp) != n) {
        fclose(fp);
        return 0;
    }
    g_oe_fp = fp;
    g_oe_frame_open = 0;
    g_oe_seq = 0;
    fprintf(stderr, "[osr-emit] writing '%s' (seed=0x%lx scenario='%s')\n",
            path, (unsigned long)seed, h.scenario);
    return 1;
}

void osr_emit_close(void)
{
    if (g_oe_fp == NULL) return;
    uint8_t buf[16];
    if (g_oe_frame_open)
        oe_write(buf, osr_enc_present(buf, sizeof buf, 0, 0));
    fprintf(stderr,
            "[osr-emit] closed: %ld frames, %ld blits, %ld sheets (%ld grab "
            "fails), %ld texts, %ld fonts, %ld blends, %ld clears\n",
            g_oe_n_frames, g_oe_n_blits, g_oe_n_sheets, g_oe_n_grab_fails,
            g_oe_n_texts, g_oe_n_fonts, g_oe_n_blends, g_oe_n_clears);
    fclose(g_oe_fp);
    g_oe_fp = NULL;
}
