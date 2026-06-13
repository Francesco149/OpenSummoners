/*
 * src/recon_apply.c — the shared ".osr" frame-reconstruction core (see the .h).
 *
 * Moved verbatim (behaviour-preserving) out of osr_recon.c so the BMP tool and
 * the native scrub viewer share ONE copy of the blit / colorkey / alpha-blend /
 * GDI-text dispatch.
 */
#include "recon_apply.h"

#include <windows.h>
#include <ddraw.h>     /* DDSCAPS_SYSTEMMEMORY */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ── sheet table ──────────────────────────────────────────────────────────── */
static zdd_object *recon_find_sheet(recon_tables *rt, uint32_t dhash)
{
    if (dhash == 0) return NULL;
    uint32_t i = dhash & (RECON_SHEET_SLOTS - 1u);
    for (uint32_t n = 0; n < RECON_SHEET_SLOTS; n++) {
        if (rt->sheet_key[i] == 0) return NULL;
        if (rt->sheet_key[i] == dhash) return rt->sheet_obj[i];
        i = (i + 1u) & (RECON_SHEET_SLOTS - 1u);
    }
    return NULL;
}

static void recon_insert_sheet(recon_tables *rt, uint32_t dhash, zdd_object *obj)
{
    if (dhash == 0) return;
    uint32_t i = dhash & (RECON_SHEET_SLOTS - 1u);
    for (uint32_t n = 0; n < RECON_SHEET_SLOTS; n++) {
        if (rt->sheet_key[i] == 0 || rt->sheet_key[i] == dhash) {
            rt->sheet_key[i] = dhash;
            rt->sheet_obj[i] = obj;
            return;
        }
        i = (i + 1u) & (RECON_SHEET_SLOTS - 1u);
    }
}

/* Load a SHEET's captured pixels into `obj`'s surface, TOP-DOWN (the capture
 * Lock'd the source surface — surface order, not a bottom-up DIB).  Clamps each
 * row to min(src_pitch, dst_pitch). */
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
        for (int32_t r = 0; r < rows; r++)
            memcpy((uint8_t *)dst + (size_t)r * (size_t)dst_pitch,
                   s->bytes + (size_t)r * (size_t)s->pitch, span);
    }
    zdd_object_unlock(obj);
}

/* ── DC management (text runs vs blits can't both hold the surface) ────────── */
static void recon_ensure_dc(recon_tables *rt, zdd_object *dest)
{
    if (rt->dc != NULL && rt->dc_owner != dest) recon_release_dc(rt);
    if (rt->dc == NULL) {
        zdd_object_get_dc(dest, &rt->dc);
        rt->dc_owner = dest;
    }
}
void recon_release_dc(recon_tables *rt)
{
    if (rt->dc != NULL && rt->dc_owner != NULL)
        zdd_object_release_dc(rt->dc_owner, rt->dc);
    rt->dc = NULL;
    rt->dc_owner = NULL;
}

/* ── lifecycle ────────────────────────────────────────────────────────────── */
void recon_tables_init(recon_tables *rt, zdd *z, int screen_w, int screen_h)
{
    rt->z = z;
    rt->screen_w = screen_w ? screen_w : 640;
    rt->screen_h = screen_h ? screen_h : 480;
}

void recon_tables_free(recon_tables *rt)
{
    recon_release_dc(rt);
    for (uint32_t i = 0; i < RECON_SHEET_SLOTS; i++) {
        if (rt->sheet_obj[i] != NULL) {
            zdd_object_dtor(rt->sheet_obj[i]);
            free(rt->sheet_obj[i]);
            rt->sheet_obj[i] = NULL;
        }
    }
    for (int i = 0; i < RECON_FONT_CAP; i++)
        if (rt->font[i] != NULL) { DeleteObject((HGDIOBJ)rt->font[i]); rt->font[i] = NULL; }
    for (int i = 0; i < RECON_BLEND_CAP; i++)
        if (rt->blend_lut[i] != NULL) { free(rt->blend_lut[i]); rt->blend_lut[i] = NULL; }
}

/* ── dedup'd record builders ──────────────────────────────────────────────── */
void recon_apply_sheet(recon_tables *rt, const osr_sheet *s)
{
    if (recon_find_sheet(rt, s->dhash) != NULL) return;   /* already built */

    zdd_object *obj = zdd_object_alloc_and_ctor(rt->z);
    if (obj == NULL) return;
    /* SYSTEM-memory offscreen source surface (no colorkey at build — per-blit).
     * System memory is decisive for the viewer: a VIDEO-memory surface Lock'd
     * for readback after blitting stalls on a GPU→CPU sync (~270 ms/frame under
     * WSLg); system memory Locks as a direct CPU pointer.  Keeping the sources
     * system-side too makes every blit a CPU copy (no cross-memory transfer). */
    if (!zdd_object_create_surface_pair(obj, 0, 0, DDSCAPS_SYSTEMMEMORY,
                                        0x1ffffff, 0, 0, 0, s->w, s->h)) {
        zdd_object_dtor(obj);
        free(obj);
        return;
    }
    recon_load_pixels(obj, s);
    recon_insert_sheet(rt, s->dhash, obj);
    rt->n_sheets_built++;
}

void recon_apply_font(recon_tables *rt, const osr_font *f)
{
    if (f->font_ref >= RECON_FONT_CAP) return;
    if (rt->font[f->font_ref] != NULL) return;   /* dedup'd; first wins */

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
    rt->font[f->font_ref] = (void *)CreateFontIndirectA(&lf);
}

void recon_apply_blend(recon_tables *rt, const osr_blend *b)
{
    if (b->blend_ref == 0 || b->blend_ref >= RECON_BLEND_CAP) return;
    if (rt->blend_present[b->blend_ref]) return;   /* dedup'd; first wins */

    uint32_t total = b->lut_len[0] + b->lut_len[1] + b->lut_len[2];
    uint8_t *store = (uint8_t *)malloc(total ? total : 1);
    if (store == NULL) return;
    if (total && b->lut) memcpy(store, b->lut, total);

    zdd_blend_desc *d = &rt->blend[b->blend_ref];
    memset(d, 0, sizeof *d);
    d->mode = b->mode;
    uint32_t off = 0;
    for (int i = 0; i < 3; i++) {
        d->ch[i].shift = b->shift[i];
        d->ch[i].mask  = b->mask[i];
        d->ch[i].lut   = store + off;
        off += b->lut_len[i];
    }
    rt->blend_lut[b->blend_ref] = store;
    rt->blend_present[b->blend_ref] = 1;
}

/* Stamp the source object's per-cel placement metrics + keying from the BLIT
 * record so the zdd primitive sees the inputs retail's cel carried.  metric_14/
 * _18 (the clip extent blt_clipped reads) always equal metric_b8/_bc (= ow/oh)
 * in retail (stamp_metrics writes both from the same w/h), so the record's
 * ow/oh is sufficient. */
static void recon_stamp_source(recon_tables *rt, zdd_object *s, const osr_blit *b)
{
    s->metric_b8 = b->ow; s->metric_bc = b->oh;
    s->metric_14 = b->ow; s->metric_18 = b->oh;
    s->metric_0c = b->ox; s->metric_10 = b->oy;
    if (b->state & 0x8000u) {
        /* keyed: bind the source SRCBLT colorkey directly.  The record's ckey is
         * cel+0x28 = colorkey_OUT — ALREADY in the surface's RGB565 encoding —
         * so do NOT go through zdd_object_set_color_key (it re-runs the RGB888→565
         * conversion → the magenta-leak); bind the raw value, only when changed. */
        if (s->colorkey_out != (int32_t)b->ckey) {
            s->colorkey_in  = (int32_t)b->ckey;
            s->colorkey_out = (int32_t)b->ckey;
            zdd_surface_set_color_key(s->com_primary, (int32_t)b->ckey, rt->z);
        }
    }
    s->state_flag = b->state;
}

void recon_apply_blit(recon_tables *rt, zdd_object *dest, const osr_blit *b)
{
    recon_release_dc(rt);            /* can't blit while a GDI DC is held */

    zdd_object *s = recon_find_sheet(rt, b->dhash);
    if (s == NULL) { rt->n_blit_no_sheet++; return; }
    recon_stamp_source(rt, s, b);

    switch (b->mode) {
    case 0: zdd_object_blt_onto(s, dest, b->dx, b->dy); break;
    case 1: zdd_object_blt_keyed(s, dest, b->dx, b->dy); break;
    case 2: /* srcw/srch carry the 8-coord call's source extent; a LEGACY
             * 80-byte capture zero-fills them — fall back to the dest extent
             * (non-scaling assumption) so old captures still reconstruct. */
        zdd_object_blt_rects(s, dest, b->dx, b->dy, b->reqw, b->reqh,
                             b->sx, b->sy,
                             b->srcw ? b->srcw : b->reqw,
                             b->srch ? b->srch : b->reqh);
        break;
    case 3:
        zdd_object_blt_clipped(s, dest, b->dx, b->dy, b->reqw, b->reqh,
                               b->sx, b->sy);
        break;
    case 4: {
        if (b->blend_ref == 0 || b->blend_ref >= RECON_BLEND_CAP ||
            !rt->blend_present[b->blend_ref]) {
            rt->n_blit_alpha_skipped++;
            break;
        }
        zdd_blit_orchestrate(&rt->blend[b->blend_ref], dest, s,
                             b->dx, b->dy, b->reqw, b->reqh, b->sx, b->sy,
                             (int32_t)b->ckey, NULL);
        break;
    }
    default: break;
    }
}

void recon_apply_text(recon_tables *rt, zdd_object *dest, const osr_text *t)
{
    recon_ensure_dc(rt, dest);
    if (rt->dc == NULL) return;
    HDC hdc = (HDC)rt->dc;

    if (t->font_ref < RECON_FONT_CAP && rt->font[t->font_ref] != NULL)
        SelectObject(hdc, (HGDIOBJ)rt->font[t->font_ref]);
    else
        rt->n_text_no_font++;
    SetTextColor(hdc, (COLORREF)t->color);
    SetBkMode(hdc, t->bk_mode == 2 ? OPAQUE : TRANSPARENT);
    if (t->str_len)
        TextOutA(hdc, t->x, t->y, t->str, (int)t->str_len);
}
