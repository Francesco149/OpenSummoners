/*
 * src/glyph_text.c — the cell-grid text/glyph layout builder.  See
 * glyph_text.h for the struct model and docs/findings/text-glyph-pipeline.md
 * for the whole-subsystem map.
 */
#include "glyph_text.h"

#include <stdlib.h>
#include <string.h>

/* The escape-expansion pass (0x4034f0/0x4051d0) is its own unported
 * subsystem; NULL = no-op, which is faithful for escape-free text. */
glyph_escape_expand_fn glyph_escape_expand_hook = NULL;

/*
 * FUN_0040fd20 (143 B) — SJIS-aware substring search.
 *
 * Retail computes strlen(hay) (iVar3) and strlen(needle) once, then from
 * position `start` tries to match `needle`, bounding the per-char compare so
 * it never runs past hay's NUL (`start + k < hlen`); a full match returns the
 * current position, an empty needle returns `start`, and each miss advances
 * the position by 1 — or 2 when the current byte is an SJIS lead.  -1 if the
 * scan reaches the end without a match.
 */
int32_t glyph_token_search(const char *hay, const char *needle, int32_t start)
{
    int32_t hlen = (int32_t)strlen(hay);
    int32_t nlen = (int32_t)strlen(needle);

    if (start < hlen) {
        const char *p = hay + start;
        do {
            if (nlen < 1) {            /* empty needle matches at `start` */
                return start;
            }
            int32_t k = 0;
            while (start + k < hlen && p[k] == needle[k]) {
                k++;
                if (nlen <= k) {       /* matched the whole needle */
                    return start;
                }
            }
            if ((signed char)*p < 0) { /* SJIS lead → skip the trail byte */
                start++;
                p++;
            }
            start++;
            p++;
        } while (start < hlen);
    }
    return -1;
}

/* Lazily build a cell's obj0 glyph buffer (FUN_0040fa00:38-51).  Retail
 * operator_new's the 0xc-byte header and a 0xb40 record buffer; we calloc the
 * records (retail leaves them garbage — observable-equivalent, the renderer
 * only reads written fields). */
static glyph_buf *cell_obj0_get_or_make(menu_cell *cell)
{
    glyph_buf *obj0 = (glyph_buf *)cell->obj0;
    if (obj0 == NULL) {
        obj0 = (glyph_buf *)calloc(1, sizeof(glyph_buf));
        if (obj0 != NULL) {
            obj0->cap     = GLYPH_BUF_CAP;
            obj0->records = calloc(GLYPH_BUF_CAP, sizeof(glyph_record));
            obj0->count   = 0;
            obj0->len     = 0;
        }
        cell->obj0 = obj0;
    }
    return obj0;
}

/*
 * FUN_0040fa00 (800 B) — lay out `str` into grid cell (row, col)'s glyph
 * buffer.  Ghidra's first two params are swapped; here `row`/`col` are
 * correct (see the header).
 */
void glyph_cell_layout(menu_ctrl *c, int32_t row, int32_t col, const char *str)
{
    menu_list_hdr *hdr = c->list;

    /* Bounds: col < column-count (alloc_b), 0 <= row < row-count (count). */
    if (!(col < hdr->alloc_b && row < hdr->count && row >= 0)) {
        return;
    }

    menu_cell *cell  = &c->rows[row].cells[col];
    glyph_buf *obj0  = cell_obj0_get_or_make(cell);
    if (obj0 == NULL || obj0->records == NULL) {
        return;
    }

    /* Over-long strings (>= cap) are dropped whole — retail has no
     * truncation; the build block is simply skipped. */
    uint16_t blen = (uint16_t)strlen(str);
    if (blen >= (uint16_t)obj0->cap) {
        return;
    }

    glyph_record *recs = (glyph_record *)obj0->records;
    obj0->len = blen;

    uint16_t ri = 0;            /* record (glyph) index — retail uVar13/uVar12 */
    if (blen != 0) {
        uint16_t bi = 0;        /* byte index into str — retail uVar8 */
        do {
            char b = str[bi];
            glyph_record *r = &recs[ri];
            if ((signed char)b < 0) {   /* SJIS lead → 2-byte glyph */
                bi++;
                r->ch[0] = b;
                r->ch[1] = str[bi];
                r->ch[2] = 0;
            } else {                    /* ASCII → 1-byte glyph */
                r->ch[0] = b;
                r->ch[1] = 0;
            }
            r->flag1c = 0;
            ri++;
            bi++;
        } while (bi < blen);
    }
    obj0->count = ri;

    /* Escape-expansion second pass (0x4034f0/0x4051d0 over the 0x5cd978
     * table) — hooked; NULL default is faithful for escape-free text. */
    if (glyph_escape_expand_hook != NULL) {
        glyph_escape_expand_hook(obj0, str, blen);
    }
}
