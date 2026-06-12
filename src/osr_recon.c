/*
 * src/osr_recon.c — the ".osr" frame RECONSTRUCTOR sink (Trace Studio v2 M4b/M4c).
 *
 * See osr_recon.h for the contract.  This is the Windows-side replay: it parses
 * the capture via the host-tested streaming reader (osr_replay.c) and turns each
 * record into pixels by REUSING the port's own bit-exact draw sinks —
 *
 *   SHEET → a fresh offscreen DDraw source surface (zdd_object) loaded with the
 *           captured pixels (TOP-DOWN: the capture grabbed them from a DDraw
 *           Lock, not a bottom-up DIB — so we do NOT reuse zdd_object_copy_cell_
 *           pixels, which flips).  Dedup'd + interned by dhash → built once.
 *   FONT  → a GDI HFONT via CreateFontIndirectA, interned by font_ref.
 *   BLIT  → the matching one of the 5 zdd.c primitives (onto/keyed/rects/clipped
 *           /alpha) onto `dest`, with the source object's placement metrics +
 *           colorkey + state_flag stamped from the record so the primitive sees
 *           exactly the inputs retail's cel carried.
 *   TEXT  → SelectObject(font)+SetTextColor+SetBkMode+TextOutA on dest's GDI DC.
 *   PRESENT → snapshot dest to recon_<flip>_t<tick>.bmp (only for wanted frames).
 *
 * Because GDI text and DDraw blits cannot both hold a surface at once, the DC is
 * acquired lazily for a run of TEXT ops and released before the next BLIT (and
 * at PRESENT) — the same GetDC/ReleaseDC dance the real engine does.
 *
 * KNOWN LIMITATIONS (flagged, non-fatal — the --validate gate, M4d, measures
 * their pixel impact):
 *   - mode-4 ALPHA blits are SKIPPED: the captured record carries the blend
 *     MODE but not the per-channel blend LUT (PORT-DEBT(osr-alpha-src-grab) /
 *     osr-recon-alpha), so the software blend can't be reproduced yet.  ~2.4%
 *     of town blits; surfaces as missing pixels until the LUT is captured.
 *   - mode-2 RECTS src_w/src_h default to the dest extent (reqw/reqh): the
 *     capture grabs only 6 of blt_rects' 8 coords (engine_hooks.h
 *     eh_blt_rects_cb), so a SCALING rects blit (rare; 1.3% of blits) is
 *     reconstructed at 1:1.  Capture-side follow-up.
 */
#include "osr_recon.h"

#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osr_replay.h"

/* ── the reconstruction context (the sink's `user`) ───────────────────────── */
#define RECON_SHEET_SLOTS 4096u   /* open-addressing; ~420 live dhashes */
#define RECON_FONT_CAP    64      /* font_ref is a small 1-based id (≤ 9 seen) */

typedef struct recon_ctx {
    zdd        *z;                 /* DDraw god-object (allocs source surfaces) */
    zdd_object *dest;              /* the lockable offscreen dest (primary_obj) */
    const char *out_dir;
    const unsigned *want;          /* wanted FLIP whitelist (NULL/0 = all)      */
    size_t      n_want;

    int         screen_w, screen_h;

    /* dhash → source surface (open addressing, linear probe on dhash) */
    uint32_t    sheet_key[RECON_SHEET_SLOTS];   /* 0 = empty slot              */
    zdd_object *sheet_obj[RECON_SHEET_SLOTS];
    int         n_sheets;

    /* font_ref → HFONT */
    void       *font[RECON_FONT_CAP];

    /* per-frame state */
    int         frame_active;      /* the current frame is wanted → draw it     */
    uint32_t    cur_flip, cur_tick;
    void       *dc;                /* HDC held for a TEXT run, NULL if released  */

    /* stats */
    long        n_frames_out;
    long        n_sheets_built;
    long        n_blit_alpha_skipped;
    long        n_blit_no_sheet;
    long        n_text_no_font;
} recon_ctx;

/* ── sheet table ──────────────────────────────────────────────────────────── */
static zdd_object *recon_find_sheet(recon_ctx *c, uint32_t dhash)
{
    if (dhash == 0) return NULL;
    uint32_t i = dhash & (RECON_SHEET_SLOTS - 1u);
    for (uint32_t n = 0; n < RECON_SHEET_SLOTS; n++) {
        if (c->sheet_key[i] == 0) return NULL;          /* empty → miss        */
        if (c->sheet_key[i] == dhash) return c->sheet_obj[i];
        i = (i + 1u) & (RECON_SHEET_SLOTS - 1u);
    }
    return NULL;
}

static void recon_insert_sheet(recon_ctx *c, uint32_t dhash, zdd_object *obj)
{
    if (dhash == 0) return;
    uint32_t i = dhash & (RECON_SHEET_SLOTS - 1u);
    for (uint32_t n = 0; n < RECON_SHEET_SLOTS; n++) {
        if (c->sheet_key[i] == 0 || c->sheet_key[i] == dhash) {
            c->sheet_key[i] = dhash;
            c->sheet_obj[i] = obj;
            return;
        }
        i = (i + 1u) & (RECON_SHEET_SLOTS - 1u);
    }
}

/* Load a SHEET's captured pixels into `obj`'s freshly-created surface, TOP-DOWN
 * (the capture Lock'd the source surface and copied raw rows — surface order,
 * not a bottom-up DIB).  Clamps each row to min(src_pitch, dst_pitch). */
static void recon_load_pixels(zdd_object *obj, const osr_sheet *s)
{
    if (!zdd_object_lock(obj)) return;
    void   *dst = NULL;
    int32_t dst_pitch = 0, dst_h = 0;
    zdd_object_get_locked_info(obj, &dst, &dst_pitch, &dst_h);
    if (dst != NULL && s->bytes != NULL) {
        int32_t rows = (int32_t)s->h;
        if (rows > dst_h) rows = dst_h;
        size_t  span = (size_t)s->pitch < (size_t)dst_pitch
                     ? (size_t)s->pitch : (size_t)dst_pitch;
        for (int32_t r = 0; r < rows; r++) {
            memcpy((uint8_t *)dst + (size_t)r * (size_t)dst_pitch,
                   s->bytes + (size_t)r * (size_t)s->pitch, span);
        }
    }
    zdd_object_unlock(obj);
}

/* ── DC management (text runs vs blits can't both hold the surface) ────────── */
static void recon_ensure_dc(recon_ctx *c)
{
    if (c->dc == NULL) zdd_object_get_dc(c->dest, &c->dc);
}
static void recon_release_dc(recon_ctx *c)
{
    if (c->dc != NULL) { zdd_object_release_dc(c->dest, c->dc); c->dc = NULL; }
}

/* ── BMP snapshot of the dest surface (mirrors main.c capture_primary_to_bmp) ─*/
static int recon_snapshot_bmp(recon_ctx *c, const char *path)
{
    const int W = c->screen_w, H = c->screen_h;
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

/* ── is this flip wanted? (empty whitelist = all) ─────────────────────────── */
static int recon_want(recon_ctx *c, uint32_t flip)
{
    if (c->n_want == 0) return 1;
    for (size_t i = 0; i < c->n_want; i++)
        if (c->want[i] == flip) return 1;
    return 0;
}

/* ── sink callbacks ───────────────────────────────────────────────────────── */
static void rc_header(void *u, const osr_header *h)
{
    recon_ctx *c = u;
    c->screen_w = h->screen_w ? h->screen_w : 640;
    c->screen_h = h->screen_h ? h->screen_h : 480;
}

static void rc_font(void *u, const osr_font *f)
{
    recon_ctx *c = u;
    if (f->font_ref >= RECON_FONT_CAP) return;
    if (c->font[f->font_ref] != NULL) return;   /* dedup'd; first wins */

    LOGFONTA lf;
    memset(&lf, 0, sizeof lf);
    lf.lfHeight         = f->height;
    lf.lfWidth          = f->width;
    lf.lfEscapement     = f->escapement;
    lf.lfOrientation    = f->orientation;
    lf.lfWeight         = f->weight;
    lf.lfItalic         = f->italic;
    lf.lfUnderline      = f->underline;
    lf.lfStrikeOut      = f->strikeout;
    lf.lfCharSet        = f->charset;
    lf.lfOutPrecision   = f->out_prec;
    lf.lfClipPrecision  = f->clip_prec;
    lf.lfQuality        = f->quality;
    lf.lfPitchAndFamily = f->pitch_family;
    memcpy(lf.lfFaceName, f->face, LF_FACESIZE < 32 ? LF_FACESIZE : 32);
    lf.lfFaceName[LF_FACESIZE - 1] = '\0';
    c->font[f->font_ref] = (void *)CreateFontIndirectA(&lf);
}

static void rc_sheet(void *u, const osr_sheet *s)
{
    recon_ctx *c = u;
    if (recon_find_sheet(c, s->dhash) != NULL) return;   /* already built */

    zdd_object *obj = zdd_object_alloc_and_ctor(c->z);
    if (obj == NULL) return;
    /* Plain offscreen source surface sized to the sheet; no colorkey at build
     * (it is per-blit, stamped in rc_blit).  create_surface_pair args:
     *   p1,p2 origin (1c/20) = 0,0; p3 caps = 0; p4 colorkey = sentinel(none);
     *   p5 force_vm = 0; p6,p7 (0c/10) = 0,0; width,height = sheet w,h. */
    if (!zdd_object_create_surface_pair(obj, 0, 0, 0, 0x1ffffff, 0, 0, 0,
                                        s->w, s->h)) {
        zdd_object_dtor(obj);
        free(obj);
        return;
    }
    recon_load_pixels(obj, s);
    recon_insert_sheet(c, s->dhash, obj);
    c->n_sheets_built++;
}

static void rc_frame_begin(void *u, const osr_framebeg *fb)
{
    recon_ctx *c = u;
    recon_release_dc(c);            /* a prior frame must not leave a DC held */
    c->cur_flip = fb->flip;
    c->cur_tick = fb->sim_tick;
    c->frame_active = recon_want(c, fb->flip);
    if (c->frame_active && c->dest != NULL)
        zdd_object_clear(c->dest);  /* a fresh black canvas to compose onto */
}

/* Stamp the source object's per-cel placement metrics + keying from the BLIT
 * record so the zdd primitive sees the inputs retail's cel carried.  metric_14/
 * _18 (the clip extent blt_clipped reads) always equal metric_b8/_bc (= ow/oh)
 * in retail — zdd_object_stamp_metrics writes both from the same width/height
 * params — so the record's ow/oh is sufficient (no separate capture needed). */
static void recon_stamp_source(recon_ctx *c, zdd_object *s, const osr_blit *b)
{
    s->metric_b8 = b->ow; s->metric_bc = b->oh;
    s->metric_14 = b->ow; s->metric_18 = b->oh;
    s->metric_0c = b->ox; s->metric_10 = b->oy;
    /* metric_b0/_b4 stay 0 (ctor) → blt_onto's src_rect = {0,0,ow,oh}. */
    if (b->state & 0x8000u) {
        /* Keyed: bind the source SRCBLT colorkey directly.  The record's ckey
         * is cel+0x28 = colorkey_OUT — already in the surface's RGB565 encoding
         * (the engine pre-converted it).  So we must NOT go through
         * zdd_object_set_color_key, which re-runs the RGB888→565 conversion and
         * would mangle an already-encoded key (the magenta-leak bug); bind the
         * raw value via the COM primitive instead, and only when it changes. */
        if (s->colorkey_out != (int32_t)b->ckey) {
            s->colorkey_in  = (int32_t)b->ckey;
            s->colorkey_out = (int32_t)b->ckey;
            zdd_surface_set_color_key(s->com_primary, (int32_t)b->ckey, c->z);
        }
    }
    s->state_flag = b->state;       /* match the record's flags exactly */
}

static void rc_blit(void *u, const osr_blit *b)
{
    recon_ctx *c = u;
    if (!c->frame_active) return;
    recon_release_dc(c);            /* can't blit while a GDI DC is held */

    if (b->mode == 4) { c->n_blit_alpha_skipped++; return; }  /* no blend LUT */

    zdd_object *s = recon_find_sheet(c, b->dhash);
    if (s == NULL) { c->n_blit_no_sheet++; return; }
    recon_stamp_source(c, s, b);

    switch (b->mode) {
    case 0: zdd_object_blt_onto(s, c->dest, b->dx, b->dy); break;
    case 1: zdd_object_blt_keyed(s, c->dest, b->dx, b->dy); break;
    case 2: /* src_w/_h default to the dest extent — see file header */
        zdd_object_blt_rects(s, c->dest, b->dx, b->dy, b->reqw, b->reqh,
                             b->sx, b->sy, b->reqw, b->reqh);
        break;
    case 3:
        zdd_object_blt_clipped(s, c->dest, b->dx, b->dy, b->reqw, b->reqh,
                               b->sx, b->sy);
        break;
    default: break;
    }
}

static void rc_text(void *u, const osr_text *t)
{
    recon_ctx *c = u;
    if (!c->frame_active) return;
    recon_ensure_dc(c);
    if (c->dc == NULL) return;
    HDC hdc = (HDC)c->dc;

    if (t->font_ref < RECON_FONT_CAP && c->font[t->font_ref] != NULL)
        SelectObject(hdc, (HGDIOBJ)c->font[t->font_ref]);
    else
        c->n_text_no_font++;
    SetTextColor(hdc, (COLORREF)t->color);
    SetBkMode(hdc, t->bk_mode == 2 ? OPAQUE : TRANSPARENT);
    if (t->str_len)
        TextOutA(hdc, t->x, t->y, t->str, (int)t->str_len);
}

static void rc_present(void *u, const osr_present *pr)
{
    (void)pr;
    recon_ctx *c = u;
    if (!c->frame_active) return;
    recon_release_dc(c);
    char path[1200];
    snprintf(path, sizeof path, "%s/recon_%05u_t%06u.bmp",
             c->out_dir, c->cur_flip, c->cur_tick);
    if (recon_snapshot_bmp(c, path)) c->n_frames_out++;
    c->frame_active = 0;
}

/* ── driver ───────────────────────────────────────────────────────────────── */
int osr_recon_run(zdd *z, zdd_object *dest, const char *out_dir,
                  const char *osr_path,
                  const unsigned *want, size_t n_want)
{
    recon_ctx c;
    memset(&c, 0, sizeof c);
    c.z = z;
    c.dest = dest;
    c.out_dir = (out_dir && out_dir[0]) ? out_dir : ".";
    c.want = want;
    c.n_want = n_want;
    c.screen_w = 640;
    c.screen_h = 480;

    osr_replay_sink sink;
    memset(&sink, 0, sizeof sink);
    sink.user           = &c;
    sink.on_header      = rc_header;
    sink.on_font        = rc_font;
    sink.on_sheet       = rc_sheet;
    sink.on_frame_begin = rc_frame_begin;
    sink.on_blit        = rc_blit;
    sink.on_text        = rc_text;
    sink.on_present     = rc_present;

    fprintf(stderr, "[opensummoners] osr-replay: streaming %s → %s (want=%zu)\n",
            osr_path, c.out_dir, n_want);
    int r = osr_replay_file(osr_path, &sink);
    recon_release_dc(&c);

    /* tear down the surfaces + fonts we created */
    for (uint32_t i = 0; i < RECON_SHEET_SLOTS; i++) {
        if (c.sheet_obj[i] != NULL) {
            zdd_object_dtor(c.sheet_obj[i]);
            free(c.sheet_obj[i]);
        }
    }
    for (int i = 0; i < RECON_FONT_CAP; i++)
        if (c.font[i] != NULL) DeleteObject((HGDIOBJ)c.font[i]);

    fprintf(stderr, "[opensummoners] osr-replay done (rc=%d): %ld frames out, "
            "%ld sheets, %ld alpha-skipped, %ld no-sheet, %ld no-font\n",
            r, c.n_frames_out, c.n_sheets_built, c.n_blit_alpha_skipped,
            c.n_blit_no_sheet, c.n_text_no_font);
    return r;
}
