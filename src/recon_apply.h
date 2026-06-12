/*
 * src/recon_apply.h — the shared ".osr" frame-RECONSTRUCTION core (Trace Studio v2).
 *
 * The bit-exact "turn a captured draw record into pixels" logic, factored out of
 * the BMP snapshot tool (osr_recon.c) so the native scrub VIEWER (tools/osr_view)
 * reuses the EXACT same sinks — there is one copy of the subtle blit / colorkey /
 * alpha-blend / GDI-text dispatch, and the two front-ends (stream-to-BMP vs
 * random-access scrub) just drive it differently.
 *
 * A `recon_tables` owns the dedup'd source surfaces (dhash → zdd_object), HFONTs
 * (font_ref → HFONT) and rebuilt blend descriptors (blend_ref → zdd_blend_desc),
 * plus the lazily-held GDI DC.  The apply_* functions compose ops onto a caller-
 * supplied dest zdd_object.  Win32-only (DDraw + GDI), so NOT in the host suite —
 * the record PARSE is host-tested via src/osr_replay.c.
 */
#ifndef OPENSUMMONERS_RECON_APPLY_H
#define OPENSUMMONERS_RECON_APPLY_H

#include <stdint.h>

#include "zdd.h"
#include "osr_format.h"

#define RECON_SHEET_SLOTS 4096u   /* open-addressing; ~420 live dhashes */
#define RECON_FONT_CAP    64      /* font_ref is a small 1-based id (≤ 9 seen) */
#define RECON_BLEND_CAP   512     /* blend_ref is a small dedup'd 1-based id */

typedef struct recon_tables {
    zdd        *z;                 /* DDraw god-object (allocs source surfaces) */
    int         screen_w, screen_h;

    /* dhash → source surface (open addressing, linear probe on dhash) */
    uint32_t    sheet_key[RECON_SHEET_SLOTS];   /* 0 = empty slot */
    zdd_object *sheet_obj[RECON_SHEET_SLOTS];
    long        n_sheets_built;

    /* font_ref → HFONT */
    void       *font[RECON_FONT_CAP];

    /* blend_ref → rebuilt zdd_blend_desc (M4 alpha); blend_lut backs its channels */
    zdd_blend_desc blend[RECON_BLEND_CAP];
    uint8_t       *blend_lut[RECON_BLEND_CAP];
    int            blend_present[RECON_BLEND_CAP];

    /* a GDI DC held for a TEXT run, released before the next blit / at present */
    zdd_object *dc_owner;          /* the surface dc was taken from */
    void       *dc;

    /* diagnostics */
    long        n_blit_alpha_skipped, n_blit_no_sheet, n_text_no_font;
} recon_tables;

/* Init an (already zeroed) tables struct. */
void recon_tables_init(recon_tables *rt, zdd *z, int screen_w, int screen_h);

/* Free every surface / HFONT / blend LUT the tables built. */
void recon_tables_free(recon_tables *rt);

/* Build a source surface / HFONT / blend descriptor from a dedup'd record (the
 * SHEET pixel bytes / TEXT-FONT / BLEND lut are valid only during the call). */
void recon_apply_sheet(recon_tables *rt, const osr_sheet *s);
void recon_apply_font(recon_tables *rt, const osr_font *f);
void recon_apply_blend(recon_tables *rt, const osr_blend *b);

/* Compose one draw op onto `dest`.  apply_blit releases any held DC first
 * (GDI + DDraw can't both hold the surface); apply_text acquires the DC lazily. */
void recon_apply_blit(recon_tables *rt, zdd_object *dest, const osr_blit *b);
void recon_apply_text(recon_tables *rt, zdd_object *dest, const osr_text *t);

/* Release a lazily-held GDI DC (call before reading the dest / snapshotting). */
void recon_release_dc(recon_tables *rt);

#endif /* OPENSUMMONERS_RECON_APPLY_H */
