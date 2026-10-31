/*
 * src/glyph_text.c — the SJIS-aware token search of the text pipeline (the cell
 * layout 0x40fa00 is ui_text's grid_cell_text).  See
 * docs/findings/text-glyph-pipeline.md for the whole-subsystem map.
 */
#include "glyph_text.h"

#include <string.h>

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
