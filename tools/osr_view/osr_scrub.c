/*
 * tools/osr_view/osr_scrub.c — the .osr reconstruction + scrub engine (C).
 *
 * Owns a DDraw device + the shared recon_apply tables + a frame INDEX (byte-
 * offset / flip / tick / draw-count per frame).  Rendering is SELF-CONTAINED
 * (openrecet-style): a SotES non-empty frame is a full redraw, so frame N is
 * reconstructed in isolation — CLEAR the dest, then replay only frame N's own
 * draws (~1.5-3 ms).  Proven byte-identical to accumulated rendering (frames
 * 700/1250/5000 differ 0.0%).  No keyframes, no upfront prime.  An EMPTY frame
 * (a re-present — retail flips without redrawing, quirk #99) shows the last
 * non-empty frame ≤ N.
 *
 * Open does ONE read-only pass over the .osr to build the frame index + load
 * every dedup'd source surface / font / blend (no rendering), then any frame is
 * an instant clear+replay.  The C++ ImGui viewer drives this through osr_scrub.h.
 */
#include "osr_scrub.h"

#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ddraw.h>     /* DDSCAPS_SYSTEMMEMORY */

#include "zdd.h"
#include "osr_format.h"
#include "recon_apply.h"

typedef struct { long long off; uint32_t flip, tick, ndraws; } frame_idx;

struct osr_scrub {
    zdd        *z;
    zdd_object *dest;
    recon_tables rt;
    int         w, h;

    FILE       *f;
    frame_idx  *frames;
    int         nframes;
    int         rendered;        /* which (non-empty) frame the dest holds (-2 none) */

    uint8_t    *buf;             /* reusable record read buffer */
    size_t      bufcap;

    /* profiling accumulators (ms) */
    double      prof_index_ms, prof_clear_ms, prof_replay_ms, prof_readback_ms;
    long        prof_renders;
};

static double g_qpc_freq = 0.0;
static double qpc_now_ms(void)
{
    LARGE_INTEGER c;
    if (g_qpc_freq == 0.0) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); g_qpc_freq = (double)f.QuadPart; }
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / g_qpc_freq;
}

static int buf_ensure(osr_scrub *s, uint32_t n)
{
    if (n <= s->bufcap) return 1;
    uint8_t *nb = (uint8_t *)realloc(s->buf, n);
    if (!nb) return 0;
    s->buf = nb; s->bufcap = n;
    return 1;
}

/* Replay frame n's own draws onto the (already-cleared) dest — self-contained. */
static void replay_frame(osr_scrub *s, int n)
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

/* Resolve frame n → the frame actually drawn (n itself, or the last non-empty
 * frame ≤ n for a re-present), and render it self-contained. */
static void render(osr_scrub *s, int n)
{
    if (n < 0) n = 0;
    if (n >= s->nframes) n = s->nframes - 1;
    int m = n;
    while (m > 0 && s->frames[m].ndraws == 0) m--;
    if (m == s->rendered) return;          /* already on screen */
    double t0 = qpc_now_ms();
    zdd_object_clear(s->dest);
    double t1 = qpc_now_ms();
    replay_frame(s, m);
    double t2 = qpc_now_ms();
    s->prof_clear_ms  += t1 - t0;
    s->prof_replay_ms += t2 - t1;
    s->prof_renders++;
    s->rendered = m;
}

/* ── index pass: frame offsets/draw-counts + load all dedup'd tables ──────── */
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

    int cap = 4096;
    s->frames = (frame_idx *)malloc((size_t)cap * sizeof *s->frames);

    /* BLOCK-BUFFERED scan: the capture has 16M+ records, so a per-record
     * fread()/fseek() loop is ~50 s of pure stdio overhead.  Read the file in
     * big blocks and parse records from memory (pointer arithmetic, no per-record
     * syscall); BLIT/TEXT payloads (the bulk) are skipped, never copied. */
    #define IDX_BLOCK (4u * 1024u * 1024u)   /* > the largest SHEET (~600 KB) */
    uint8_t *B = (uint8_t *)malloc(IDX_BLOCK);
    if (!B) return 0;
    size_t fill = 0, pos = 0;                 /* B[pos..fill) is unparsed */

    long long off = OSR_HEADER_SIZE;
    uint32_t pflip = 0, ptick = 0, pndraws = 0;
    long long poff = -1;

    for (;;) {
        if (fill - pos < 8) {                 /* refill for a record header */
            memmove(B, B + pos, fill - pos);
            fill -= pos; pos = 0;
            fill += fread(B + fill, 1, IDX_BLOCK - fill, s->f);
            if (fill < 8) break;              /* EOF / torn tail */
        }
        uint32_t t   = osr_get_u32(B + pos);
        uint32_t len = osr_get_u32(B + pos + 4);

        if (t == OSR_BLIT || t == OSR_TEXT) {
            pndraws++;
            size_t avail = fill - pos - 8;
            if (avail >= len) {
                pos += 8 + len;
            } else {                          /* payload runs past the block */
                pos = fill;
                _fseeki64(s->f, (long long)(len - avail), SEEK_CUR);
            }
            off += 8 + len;
            continue;
        }

        /* a record we DECODE — get its payload contiguous (in B, or copied out
         * for the rare record bigger than what's buffered). */
        const uint8_t *pay;
        if (fill - pos >= (size_t)8 + len) {
            pay = B + pos + 8;
        } else {
            if (len + 8 <= IDX_BLOCK) {        /* refill so it fits in the block */
                memmove(B, B + pos, fill - pos);
                fill -= pos; pos = 0;
                fill += fread(B + fill, 1, IDX_BLOCK - fill, s->f);
            }
            if (fill - pos >= (size_t)8 + len) {
                pay = B + pos + 8;
            } else {                          /* huge record: assemble in s->buf */
                if (!buf_ensure(s, len ? len : 1)) break;
                size_t have = (fill - pos > 8) ? (fill - pos - 8) : 0;
                if (have) memcpy(s->buf, B + pos + 8, have);
                if (len > have && fread(s->buf + have, 1, len - have, s->f) != len - have) break;
                pos = fill;
                pay = s->buf;
            }
        }

        if (t == OSR_FRAMEBEG) {
            osr_framebeg fb;
            if (osr_dec_framebeg(pay, len, &fb)) { poff = off; pflip = fb.flip; ptick = fb.sim_tick; }
            pndraws = 0;
        } else if (t == OSR_PRESENT) {
            if (poff >= 0) {
                if (s->nframes >= cap) {
                    cap *= 2;
                    s->frames = (frame_idx *)realloc(s->frames, (size_t)cap * sizeof *s->frames);
                }
                s->frames[s->nframes].off = poff;
                s->frames[s->nframes].flip = pflip;
                s->frames[s->nframes].tick = ptick;
                s->frames[s->nframes].ndraws = pndraws;
                s->nframes++;
                poff = -1;
            }
        } else if (t == OSR_SHEET) {
            osr_sheet sh; if (osr_dec_sheet(pay, len, &sh)) recon_apply_sheet(&s->rt, &sh);
        } else if (t == OSR_FONT) {
            osr_font fo; if (osr_dec_font(pay, len, &fo)) recon_apply_font(&s->rt, &fo);
        } else if (t == OSR_BLEND) {
            osr_blend bl; if (osr_dec_blend(pay, len, &bl)) recon_apply_blend(&s->rt, &bl);
        }
        if (pay == B + pos + 8) pos += 8 + len;   /* (else already consumed) */
        off += 8 + len;
    }
    free(B);
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
    recon_tables_init(&s->rt, s->z, 640, 480);

    /* A dedicated SYSTEM-memory dest (not the video-memory primary_obj): the
     * per-frame readback Lock is then a direct CPU read, not a GPU→CPU stall
     * (the 274 ms/frame the profiler found).  Freed in osr_scrub_close. */
    s->dest = zdd_object_alloc_and_ctor(s->z);
    if (!s->dest ||
        !zdd_object_create_surface_pair(s->dest, 0, 0, DDSCAPS_SYSTEMMEMORY,
                                        0x1ffffff, 0, 0, 0, 640, 480)) {
        osr_scrub_close(s); return NULL;
    }

    double ti0 = qpc_now_ms();
    if (!build_index(s, osr_path)) { osr_scrub_close(s); return NULL; }
    s->prof_index_ms = qpc_now_ms() - ti0;
    zdd_object_clear(s->dest);
    return s;
}

void osr_scrub_close(osr_scrub *s)
{
    if (!s) return;
    recon_tables_free(&s->rt);
    if (s->dest) { zdd_object_dtor(s->dest); free(s->dest); s->dest = NULL; }
    if (s->z) zdd_destroy(s->z);
    if (s->f) fclose(s->f);
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
    double tr0 = qpc_now_ms();
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
    s->prof_readback_ms += qpc_now_ms() - tr0;
    return 1;
}

void osr_scrub_prof(const osr_scrub *s, double *index_ms, double *clear_ms,
                    double *replay_ms, double *readback_ms, long *renders)
{
    if (!s) return;
    if (index_ms)    *index_ms    = s->prof_index_ms;
    if (clear_ms)    *clear_ms    = s->prof_clear_ms;
    if (replay_ms)   *replay_ms   = s->prof_replay_ms;
    if (readback_ms) *readback_ms = s->prof_readback_ms;
    if (renders)     *renders     = s->prof_renders;
}
