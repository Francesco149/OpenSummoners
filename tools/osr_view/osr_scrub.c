/*
 * tools/osr_view/osr_scrub.c — the .osr reconstruction + scrub engine (C).
 *
 * Owns a DDraw device + the shared recon_apply tables + a frame INDEX (byte-
 * offset/flip/tick per frame) + a lazy KEYFRAME cache (RGB565 dest snapshots).
 * render_rgba(N) restores the nearest keyframe and replays only (keyframe..N]'s
 * ops (re-read from their offsets); a forward step is one frame.  The dest
 * ACCUMULATES (no per-frame clear — empty re-present frames retain prior pixels,
 * quirk #99).  The C++ ImGui viewer drives this through osr_scrub.h.
 */
#include "osr_scrub.h"

#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zdd.h"
#include "osr_format.h"
#include "recon_apply.h"

#define KF_INTERVAL 120          /* keyframe every N frames (≤N-frame replay seek) */

typedef struct { long long off; uint32_t flip, tick; } frame_idx;

struct osr_scrub {
    zdd        *z;
    zdd_object *dest;
    recon_tables rt;
    int         w, h;

    FILE       *f;
    frame_idx  *frames;
    int         nframes;

    uint16_t  **kf;              /* kf[k] = dest RGB565 at frame k*KF_INTERVAL */
    int         nkf;
    int         rendered;        /* which frame the dest currently holds (-2 none) */

    uint8_t    *buf;             /* reusable record read buffer */
    size_t      bufcap;
};

static int buf_ensure(osr_scrub *s, uint32_t n)
{
    if (n <= s->bufcap) return 1;
    uint8_t *nb = (uint8_t *)realloc(s->buf, n);
    if (!nb) return 0;
    s->buf = nb; s->bufcap = n;
    return 1;
}

/* ── dest RGB565 snapshot / restore (tight w*h) ───────────────────────────── */
static uint16_t *dest_snapshot(osr_scrub *s)
{
    if (!zdd_object_lock(s->dest)) return NULL;
    void *buf = NULL; int32_t pitch = 0, hh = 0;
    zdd_object_get_locked_info(s->dest, &buf, &pitch, &hh);
    uint16_t *out = (uint16_t *)malloc((size_t)s->w * s->h * 2);
    if (out && buf)
        for (int r = 0; r < s->h; r++)
            memcpy(out + (size_t)r * s->w, (uint8_t *)buf + (size_t)r * pitch,
                   (size_t)s->w * 2);
    zdd_object_unlock(s->dest);
    return out;
}
static void dest_restore(osr_scrub *s, const uint16_t *kf)
{
    if (!zdd_object_lock(s->dest)) return;
    void *buf = NULL; int32_t pitch = 0, hh = 0;
    zdd_object_get_locked_info(s->dest, &buf, &pitch, &hh);
    if (buf)
        for (int r = 0; r < s->h; r++)
            memcpy((uint8_t *)buf + (size_t)r * pitch, kf + (size_t)r * s->w,
                   (size_t)s->w * 2);
    zdd_object_unlock(s->dest);
}

/* Apply frame n's ops onto the dest (assumes the dest holds frame n-1). */
static void apply_frame(osr_scrub *s, int n)
{
    if (_fseeki64(s->f, s->frames[n].off, SEEK_SET) != 0) return;
    uint8_t hdr[8];
    while (fread(hdr, 1, 8, s->f) == 8) {
        uint32_t t = osr_get_u32(hdr), len = osr_get_u32(hdr + 4);
        if (!buf_ensure(s, len ? len : 1)) break;
        if (len && fread(s->buf, 1, len, s->f) != len) break;
        if (t == OSR_FRAMEBEG) continue;
        if (t == OSR_PRESENT)  break;
        if (t == OSR_BLIT) {
            osr_blit b;
            if (osr_dec_blit(s->buf, len, &b)) recon_apply_blit(&s->rt, s->dest, &b);
        } else if (t == OSR_TEXT) {
            osr_text x;
            if (osr_dec_text(s->buf, len, &x)) recon_apply_text(&s->rt, s->dest, &x);
        }
    }
    recon_release_dc(&s->rt);
}

static void maybe_keyframe(osr_scrub *s, int fr)
{
    if (fr % KF_INTERVAL != 0) return;
    int k = fr / KF_INTERVAL;
    if (k >= s->nkf || s->kf[k] != NULL) return;
    s->kf[k] = dest_snapshot(s);
}

static void render(osr_scrub *s, int n)
{
    if (n < 0) n = 0;
    if (n >= s->nframes) n = s->nframes - 1;
    if (n == s->rendered) return;

    if (n == s->rendered + 1) {
        apply_frame(s, n);
        maybe_keyframe(s, n);
        s->rendered = n;
        return;
    }
    int k = n / KF_INTERVAL, start;
    while (k > 0 && (k >= s->nkf || s->kf[k] == NULL)) k--;
    if (k > 0 && k < s->nkf && s->kf[k] != NULL) {
        dest_restore(s, s->kf[k]);
        start = k * KF_INTERVAL + 1;
    } else {
        zdd_object_clear(s->dest);
        start = 0;
    }
    for (int fr = start; fr <= n; fr++) {
        apply_frame(s, fr);
        maybe_keyframe(s, fr);
    }
    s->rendered = n;
}

/* ── index pass: frame offsets + load all dedup'd tables ──────────────────── */
static int build_index(osr_scrub *s, const char *path)
{
    s->f = fopen(path, "rb");
    if (!s->f) return 0;

    uint8_t hb[OSR_HEADER_SIZE];
    if (fread(hb, 1, OSR_HEADER_SIZE, s->f) != OSR_HEADER_SIZE) return 0;
    osr_header h;
    if (!osr_header_decode(hb, OSR_HEADER_SIZE, &h)) return 0;
    s->w = h.screen_w ? h.screen_w : 640;
    s->h = h.screen_h ? h.screen_h : 480;

    /* First a quick count pass would need a second read; instead grow the index
     * + the keyframe array as we go.  This single pass ALSO reconstructs every
     * frame onto the dest and snapshots a keyframe every KF_INTERVAL frames — so
     * priming is folded into the index read (one pass, ~one file read), and after
     * open every seek replays ≤ KF_INTERVAL frames (scrubbing never freezes). */
    int cap = 4096;
    s->frames = (frame_idx *)malloc((size_t)cap * sizeof *s->frames);
    s->nkf = 4096;
    s->kf = (uint16_t **)calloc((size_t)s->nkf, sizeof *s->kf);
    zdd_object_clear(s->dest);

    long long off = OSR_HEADER_SIZE;
    uint8_t hdr[8];
    uint32_t pflip = 0, ptick = 0;
    long long poff = -1;
    while (fread(hdr, 1, 8, s->f) == 8) {
        uint32_t t = osr_get_u32(hdr), len = osr_get_u32(hdr + 4);
        if (!buf_ensure(s, len ? len : 1)) break;
        if (len && fread(s->buf, 1, len, s->f) != len) break;
        if (t == OSR_FRAMEBEG) {
            osr_framebeg fb;
            if (osr_dec_framebeg(s->buf, len, &fb)) { poff = off; pflip = fb.flip; ptick = fb.sim_tick; }
            recon_release_dc(&s->rt);
        } else if (t == OSR_BLIT) {
            osr_blit b; if (osr_dec_blit(s->buf, len, &b)) recon_apply_blit(&s->rt, s->dest, &b);
        } else if (t == OSR_TEXT) {
            osr_text x; if (osr_dec_text(s->buf, len, &x)) recon_apply_text(&s->rt, s->dest, &x);
        } else if (t == OSR_PRESENT) {
            recon_release_dc(&s->rt);
            if (poff >= 0) {
                if (s->nframes >= cap) {
                    cap *= 2;
                    s->frames = (frame_idx *)realloc(s->frames, (size_t)cap * sizeof *s->frames);
                }
                s->frames[s->nframes].off = poff;
                s->frames[s->nframes].flip = pflip;
                s->frames[s->nframes].tick = ptick;
                int F = s->nframes;
                s->nframes++;
                poff = -1;
                if (F % KF_INTERVAL == 0) {
                    int k = F / KF_INTERVAL;
                    if (k >= s->nkf) {
                        int nn = s->nkf * 2;
                        s->kf = (uint16_t **)realloc(s->kf, (size_t)nn * sizeof *s->kf);
                        memset(s->kf + s->nkf, 0, (size_t)(nn - s->nkf) * sizeof *s->kf);
                        s->nkf = nn;
                    }
                    if (s->kf[k] == NULL) s->kf[k] = dest_snapshot(s);
                }
            }
        } else if (t == OSR_SHEET) {
            osr_sheet sh; if (osr_dec_sheet(s->buf, len, &sh)) recon_apply_sheet(&s->rt, &sh);
        } else if (t == OSR_FONT) {
            osr_font fo; if (osr_dec_font(s->buf, len, &fo)) recon_apply_font(&s->rt, &fo);
        } else if (t == OSR_BLEND) {
            osr_blend bl; if (osr_dec_blend(s->buf, len, &bl)) recon_apply_blend(&s->rt, &bl);
        }
        off += 8 + len;
    }
    s->rendered = s->nframes - 1;   /* the dest holds the last frame after priming */
    return s->nframes > 0;
}

/* ── public API ───────────────────────────────────────────────────────────── */
osr_scrub *osr_scrub_open(void *hwnd, const char *osr_path)
{
    osr_scrub *s = (osr_scrub *)calloc(1, sizeof *s);
    if (!s) return NULL;
    s->rendered = -2;
    s->w = 640; s->h = 480;

    if (!zdd_create(&s->z)) { free(s); return NULL; }
    zdd_set_coop_level(s->z, hwnd, 0);
    if (!zdd_create_screen(s->z, 640, 480, 16, 2, 0, NULL)) {
        zdd_destroy(s->z); free(s); return NULL;
    }
    s->dest = s->z->primary_obj;
    recon_tables_init(&s->rt, s->z, 640, 480);

    if (!build_index(s, osr_path)) { osr_scrub_close(s); return NULL; }
    return s;   /* the index pass already primed the dest + all keyframes */
}

void osr_scrub_close(osr_scrub *s)
{
    if (!s) return;
    recon_tables_free(&s->rt);
    if (s->z) zdd_destroy(s->z);
    if (s->f) fclose(s->f);
    for (int i = 0; i < s->nkf; i++) free(s->kf[i]);
    free(s->kf);
    free(s->frames);
    free(s->buf);
    free(s);
}

int  osr_scrub_frame_count(const osr_scrub *s) { return s ? s->nframes : 0; }
int  osr_scrub_width(const osr_scrub *s)       { return s ? s->w : 0; }
int  osr_scrub_height(const osr_scrub *s)      { return s ? s->h : 0; }

void osr_scrub_frame_info(const osr_scrub *s, int idx, uint32_t *flip, uint32_t *tick)
{
    if (!s || idx < 0 || idx >= s->nframes) { if (flip) *flip = 0; if (tick) *tick = 0; return; }
    if (flip) *flip = s->frames[idx].flip;
    if (tick) *tick = s->frames[idx].tick;
}

int osr_scrub_render_rgba(osr_scrub *s, int idx, uint32_t *out)
{
    if (!s || !out) return 0;
    render(s, idx);
    if (!zdd_object_lock(s->dest)) return 0;
    void *buf = NULL; int32_t pitch = 0, hh = 0;
    zdd_object_get_locked_info(s->dest, &buf, &pitch, &hh);
    if (buf) {
        for (int y = 0; y < s->h; y++) {
            const uint16_t *src = (const uint16_t *)((uint8_t *)buf + (size_t)y * pitch);
            uint32_t *dst = out + (size_t)y * s->w;
            for (int x = 0; x < s->w; x++) {
                uint16_t p = src[x];
                uint32_t r = (uint32_t)((p >> 11) & 0x1f);
                uint32_t g = (uint32_t)((p >>  5) & 0x3f);
                uint32_t b = (uint32_t)( p        & 0x1f);
                r = (r << 3) | (r >> 2);
                g = (g << 2) | (g >> 4);
                b = (b << 3) | (b >> 2);
                dst[x] = 0xff000000u | (b << 16) | (g << 8) | r;  /* RGBA8 (DX R8G8B8A8) */
            }
        }
    }
    zdd_object_unlock(s->dest);
    return 1;
}
