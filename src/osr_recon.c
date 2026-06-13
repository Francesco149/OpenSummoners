/*
 * src/osr_recon.c — the ".osr" → BMP reconstructor driver (Trace Studio v2 M4).
 *
 * The STREAM-to-BMP front-end: feed an .osr through the streaming reader
 * (osr_replay.c), compose each frame onto a dest surface via the shared
 * reconstruction core (recon_apply.h — the bit-exact blit/text/blend sinks), and
 * snapshot the wanted frames to recon_<flip>_t<tick>.bmp.  The native scrub
 * viewer (tools/osr_view) drives the SAME core with random access instead.
 *
 * The dest is NOT cleared between frames: retail flips a back-buffer chain, so an
 * empty re-present frame (no draws — retail coalesces, quirk #99) must retain the
 * prior pixels; the dest is cleared once at start and accumulates.
 *
 * KNOWN LIMITATION: mode-2 RECTS src_w/src_h default to the dest extent (osr_blit
 * has no field for them yet) — a scaling rects blit (~1.3% of blits) reconstructs
 * at 1:1.  Capture-side follow-up.  Scene-transition CLEARs (OSR_CLEAR) aren't
 * captured yet, so the dest can carry stale content across a scene boundary until
 * the next full redraw.
 */
#include "osr_recon.h"

#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osr_replay.h"
#include "recon_apply.h"

typedef struct recon_ctx {
    recon_tables rt;
    zdd_object  *dest;
    const char  *out_dir;
    const unsigned *want;
    size_t       n_want;
    uint32_t     cur_flip, cur_tick;
    long         n_frames_out;
    /* M4d --validate: SNAP records (real retail backbuffer pixels) are compared
     * against the just-composed dest; the gate is differ_px==0 on every snap. */
    long         n_snaps, n_snaps_clean;
    long         worst_differ;
    uint32_t     worst_flip;
} recon_ctx;

/* ── BMP snapshot of the dest surface (mirrors main.c capture_primary_to_bmp) ─*/
static int recon_snapshot_bmp(recon_ctx *c, const char *path)
{
    const int W = c->rt.screen_w, H = c->rt.screen_h;
    void *src_hdc = NULL;
    if (!zdd_object_get_dc(c->dest, &src_hdc) || src_hdc == NULL) return 0;

    HDC mem = CreateCompatibleDC((HDC)src_hdc);
    if (mem == NULL) { zdd_object_release_dc(c->dest, src_hdc); return 0; }

    BITMAPINFO bi;
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = W;
    bi.bmiHeader.biHeight      = H;          /* +H = bottom-up = BMP order */
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HBITMAP dib = CreateDIBSection((HDC)src_hdc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    int ok = 0;
    if (dib != NULL && bits != NULL) {
        HGDIOBJ old = SelectObject(mem, dib);
        if (BitBlt(mem, 0, 0, W, H, (HDC)src_hdc, 0, 0, SRCCOPY)) {
            const uint32_t row   = (uint32_t)((W * 3 + 3) & ~3);
            const uint32_t pixsz = row * (uint32_t)H;
            BITMAPFILEHEADER fh;
            memset(&fh, 0, sizeof fh);
            fh.bfType    = 0x4D42;            /* 'BM' */
            fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            fh.bfSize    = fh.bfOffBits + pixsz;
            FILE *f = fopen(path, "wb");
            if (f != NULL) {
                fwrite(&fh, sizeof fh, 1, f);
                fwrite(&bi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, f);
                fwrite(bits, 1, pixsz, f);
                fclose(f);
                ok = 1;
            }
        }
        SelectObject(mem, old);
    }
    if (dib != NULL) DeleteObject(dib);
    DeleteDC(mem);
    zdd_object_release_dc(c->dest, src_hdc);
    return ok;
}

/* ── M4d: write a 24-bit BMP from raw top-down RGB565 rows (the SNAP pixels) ─ */
static int recon_write_bmp_rgb565(const char *path, const uint8_t *pix,
                                  int w, int h, uint32_t pitch)
{
    const uint32_t row   = (uint32_t)((w * 3 + 3) & ~3);
    const uint32_t pixsz = row * (uint32_t)h;
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER bih;
    memset(&fh, 0, sizeof fh);
    memset(&bih, 0, sizeof bih);
    fh.bfType    = 0x4D42;
    fh.bfOffBits = sizeof fh + sizeof bih;
    fh.bfSize    = fh.bfOffBits + pixsz;
    bih.biSize   = sizeof bih;
    bih.biWidth  = w;
    bih.biHeight = h;                       /* +h = bottom-up file order */
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;

    FILE *f = fopen(path, "wb");
    if (f == NULL) return 0;
    fwrite(&fh, sizeof fh, 1, f);
    fwrite(&bih, sizeof bih, 1, f);
    uint8_t *line = malloc(row);
    if (line == NULL) { fclose(f); return 0; }
    for (int y = h - 1; y >= 0; y--) {      /* bottom-up: last snap row first */
        const uint16_t *src = (const uint16_t *)(pix + (size_t)y * pitch);
        memset(line, 0, row);
        for (int x = 0; x < w; x++) {
            uint16_t v = src[x];
            line[x * 3 + 0] = (uint8_t)(((v & 0x001f) << 3) | ((v & 0x001f) >> 2));
            line[x * 3 + 1] = (uint8_t)(((v & 0x07e0) >> 3) | ((v & 0x07e0) >> 9));
            line[x * 3 + 2] = (uint8_t)(((v & 0xf800) >> 8) | (v >> 13));
        }
        fwrite(line, 1, row, f);
    }
    free(line);
    fclose(f);
    return 1;
}

static int recon_want(recon_ctx *c, uint32_t flip)
{
    if (c->n_want == 0) return 1;
    for (size_t i = 0; i < c->n_want; i++)
        if (c->want[i] == flip) return 1;
    return 0;
}

/* ── streaming sink callbacks (thin wrappers over the shared core) ─────────── */
static void rc_header(void *u, const osr_header *h)
{
    recon_ctx *c = u;
    c->rt.screen_w = h->screen_w ? h->screen_w : 640;
    c->rt.screen_h = h->screen_h ? h->screen_h : 480;
}
static void rc_font(void *u, const osr_font *f)  { recon_apply_font(&((recon_ctx *)u)->rt, f); }
static void rc_blend(void *u, const osr_blend *b){ recon_apply_blend(&((recon_ctx *)u)->rt, b); }
static void rc_sheet(void *u, const osr_sheet *s){ recon_apply_sheet(&((recon_ctx *)u)->rt, s); }

static void rc_frame_begin(void *u, const osr_framebeg *fb)
{
    recon_ctx *c = u;
    recon_release_dc(&c->rt);       /* a prior frame must not leave a DC held */
    c->cur_flip = fb->flip;
    c->cur_tick = fb->sim_tick;
    /* draw EVERY frame onto the (accumulating) dest; no per-frame clear (the
     * streaming BMP tool retains prior pixels for empty re-present frames). */
}
static void rc_clear(void *u, const osr_clear *cl)
{
    (void)cl;                       /* proxy-filtered to the backbuffer */
    recon_ctx *c = u;
    recon_release_dc(&c->rt);       /* a held GDI DC blocks the clear's Lock */
    zdd_object_clear(c->dest);
}
static void rc_blit(void *u, const osr_blit *b)
{ recon_ctx *c = u; recon_apply_blit(&c->rt, c->dest, b); }
static void rc_text(void *u, const osr_text *t)
{ recon_ctx *c = u; recon_apply_text(&c->rt, c->dest, t); }

/* ── M4d --validate: a SNAP holds the REAL retail backbuffer pixels for the
 * frame being closed (in-stream just before its PRESENT, after all its draws),
 * so the accumulated dest must match it EXACTLY — differ_px==0 is the gate.
 * On a mismatch, dump the real snapshot + the reconstruction side by side. */
static void rc_snap(void *u, const osr_snap *s)
{
    recon_ctx *c = u;
    c->n_snaps++;
    if (s->pixfmt != OSR_PIXFMT_RGB565) {
        fprintf(stderr, "[validate] flip=%u SKIP: snap pixfmt=%u (want RGB565)\n",
                s->flip, s->pixfmt);
        return;
    }
    recon_release_dc(&c->rt);               /* a held GDI DC blocks the Lock */
    if (!zdd_object_lock(c->dest)) {
        fprintf(stderr, "[validate] flip=%u SKIP: dest Lock failed\n", s->flip);
        return;
    }
    void   *buf = NULL;
    int32_t pitch = 0, height = 0;
    zdd_object_get_locked_info(c->dest, &buf, &pitch, &height);

    int w = s->w < c->rt.screen_w ? s->w : c->rt.screen_w;
    int h = s->h < (int)height    ? s->h : (int)height;
    long differ = 0;
    if (buf != NULL) {
        for (int y = 0; y < h; y++) {
            const uint16_t *real  = (const uint16_t *)(s->bytes + (size_t)y * s->pitch);
            const uint16_t *recon = (const uint16_t *)((const uint8_t *)buf
                                                       + (size_t)y * (size_t)pitch);
            for (int x = 0; x < w; x++)
                if (real[x] != recon[x]) differ++;
        }
    }
    zdd_object_unlock(c->dest);

    if (differ == 0) c->n_snaps_clean++;
    if (differ > c->worst_differ) { c->worst_differ = differ; c->worst_flip = s->flip; }
    fprintf(stderr, "[validate] flip=%u tick=%u differ_px=%ld (%dx%d)\n",
            s->flip, s->sim_tick, differ, w, h);
    if (differ != 0) {
        char path[1200];
        snprintf(path, sizeof path, "%s/real_%05u_t%06u.bmp",
                 c->out_dir, s->flip, s->sim_tick);
        recon_write_bmp_rgb565(path, s->bytes, w, h, s->pitch);
        snprintf(path, sizeof path, "%s/recon_%05u_t%06u.bmp",
                 c->out_dir, c->cur_flip, c->cur_tick);
        recon_snapshot_bmp(c, path);         /* the mismatching reconstruction */
    }
}

static void rc_present(void *u, const osr_present *pr)
{
    (void)pr;
    recon_ctx *c = u;
    recon_release_dc(&c->rt);
    if (!recon_want(c, c->cur_flip)) return;   /* draw all, snapshot wanted */
    char path[1200];
    snprintf(path, sizeof path, "%s/recon_%05u_t%06u.bmp",
             c->out_dir, c->cur_flip, c->cur_tick);
    if (recon_snapshot_bmp(c, path)) c->n_frames_out++;
}

/* ── driver ───────────────────────────────────────────────────────────────── */
int osr_recon_run(zdd *z, zdd_object *dest, const char *out_dir,
                  const char *osr_path,
                  const unsigned *want, size_t n_want)
{
    recon_ctx c;
    memset(&c, 0, sizeof c);
    recon_tables_init(&c.rt, z, 640, 480);
    c.dest = dest;
    c.out_dir = (out_dir && out_dir[0]) ? out_dir : ".";
    c.want = want;
    c.n_want = n_want;

    osr_replay_sink sink;
    memset(&sink, 0, sizeof sink);
    sink.user           = &c;
    sink.on_header      = rc_header;
    sink.on_font        = rc_font;
    sink.on_blend       = rc_blend;
    sink.on_sheet       = rc_sheet;
    sink.on_frame_begin = rc_frame_begin;
    sink.on_clear       = rc_clear;
    sink.on_blit        = rc_blit;
    sink.on_text        = rc_text;
    sink.on_snap        = rc_snap;
    sink.on_present     = rc_present;

    fprintf(stderr, "[opensummoners] osr-replay: streaming %s → %s (want=%zu)\n",
            osr_path, c.out_dir, n_want);
    if (dest != NULL) zdd_object_clear(dest);   /* one initial black canvas */
    int r = osr_replay_file(osr_path, &sink);
    recon_release_dc(&c.rt);
    recon_tables_free(&c.rt);

    fprintf(stderr, "[opensummoners] osr-replay done (rc=%d): %ld frames out, "
            "%ld sheets, %ld alpha-skipped, %ld no-sheet, %ld no-font\n",
            r, c.n_frames_out, c.rt.n_sheets_built, c.rt.n_blit_alpha_skipped,
            c.rt.n_blit_no_sheet, c.rt.n_text_no_font);
    if (c.n_snaps) {
        fprintf(stderr, "[opensummoners] VALIDATE: %ld/%ld snaps differ_px==0",
                c.n_snaps_clean, c.n_snaps);
        if (c.n_snaps_clean != c.n_snaps)
            fprintf(stderr, " — WORST flip=%u differ_px=%ld",
                    c.worst_flip, c.worst_differ);
        fprintf(stderr, "\n");
    }
    return r;
}
