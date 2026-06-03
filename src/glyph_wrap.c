/*
 * src/glyph_wrap.c — the tooltip text-node word-wrap (FUN_0040e5e0 ASCII path
 * + the FUN_0040f040 `%n` parse).  See glyph_wrap.h for the subsystem map.
 */
#include "glyph_wrap.h"

#include <string.h>

/* The retail node buffer is 0x200 glyphs; a tooltip is far shorter. */
#define WGLYPH_CAP 0x200

typedef struct {
    char b[2];      /* glyph bytes (1 ASCII, 2 SJIS) */
    int  n;         /* byte count (1 or 2) */
    int  brk;       /* forced line break BEFORE this glyph (from a preceding %n) */
} wglyph;

/* ── FUN_0040f040 (the relevant arm): split `src` into glyph records, folding
 *    the `%n` line-break escape into a per-glyph flag.  `%m<digits>%` (colour)
 *    and `%w` (wrap hint) are consumed; an unknown `%X` drops both bytes — the
 *    faithful default arm.  Returns the glyph count. */
static int wrap_parse(const char *src, wglyph *g)
{
    int count = 0;
    int pending_brk = 0;            /* retail sets pos[next].linebreak on %n */

    for (size_t i = 0; src[i] != '\0' && count < WGLYPH_CAP; ) {
        if (src[i] == '%') {
            char e = src[i + 1];
            if (e == 'n') {                 /* %n → break before the next glyph */
                pending_brk = 1;
                i += 2;
                continue;
            }
            if (e == 'm') {                 /* %m<digits>% → colour, consume it */
                i += 2;
                while (src[i] != '\0' && src[i] != '%') i++;
                if (src[i] == '%') i++;
                continue;
            }
            if (e == 'w') {                 /* %w → wrap hint, consume */
                i += 2;
                continue;
            }
            if (e == '\0') { i++; continue; }
            i += 2;                          /* unknown %X → drop both bytes */
            continue;
        }

        wglyph *w = &g[count++];
        w->brk = pending_brk;
        pending_brk = 0;
        if ((signed char)src[i] < 0 && src[i + 1] != '\0') {   /* SJIS lead */
            w->b[0] = src[i];
            w->b[1] = src[i + 1];
            w->n = 2;
            i += 2;
        } else {
            w->b[0] = src[i];
            w->b[1] = 0;
            w->n = 1;
            i += 1;
        }
    }
    return count;
}

/* Classify a glyph's lead byte the way FUN_0040e5e0 does: 1 = alpha word,
 * 2 = digit word, 3 = SJIS lead (deferred), 0 = lone other glyph. */
static int wrap_class(char c)
{
    if ((signed char)c < 0)                              return 3;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return 1;
    if (c >= '0' && c <= '9')                            return 2;
    return 0;
}

/* Append a glyph's bytes to row `r`, NUL-terminating; drops on overflow. */
static void wrap_emit(glyph_wrap_result *out, int r, const wglyph *w)
{
    if (r < 0 || r >= GLYPH_WRAP_MAX_ROWS) return;
    size_t len = strlen(out->rows[r]);
    if (len + (size_t)w->n + 1 > GLYPH_WRAP_MAX_LINE) return;
    out->rows[r][len] = w->b[0];
    if (w->n == 2) out->rows[r][len + 1] = w->b[1];
    out->rows[r][len + w->n] = '\0';
}

void glyph_wrap_layout(const char *src, int width, glyph_wrap_result *out)
{
    static wglyph g[WGLYPH_CAP];   /* host-only, single-threaded; avoids a VLA */

    memset(out, 0, sizeof(*out));
    if (src == NULL || src[0] == '\0') return;

    int gcount = wrap_parse(src, g);
    if (gcount == 0) return;

    int row     = 0;               /* sVar4 — current row index            */
    int row_w   = 0;               /* uVar13 — current row width (columns) */
    int emitted = 0;

    int wi = 0;
    while (wi < gcount) {
        /* ── word extent (uVar16): a run by char-class + one absorbed trailer ── */
        int cls  = wrap_class(g[wi].b[0]);
        int wend = wi;
        if (cls == 1 || cls == 2) {
            int j = wi + 1;
            while (j < gcount) {
                char c = g[j].b[0];
                int belongs = (cls == 2)
                    ? ((c >= '0' && c <= '9') || c == ',' || c == '.')
                    : ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '\'');
                if (!belongs) {
                    int absorb = (cls == 2)
                        ? (c == ' ')
                        : (c == ' ' || c == '!' || c == ',' || c == '-' ||
                           c == '.' || c == ';' || c == '?');
                    if (absorb) wend = j;   /* uVar16++ — trailing punct/space */
                    break;
                }
                wend = j;                   /* uVar16++ — glyph belongs to word */
                j++;
            }
        }
        /* cls 0 (lone glyph) and cls 3 (SJIS lead — kinsoku deferred) keep
         * wend == wi: a single-glyph word. */

        /* ── word width (uVar12): sum of glyph byte-widths ── */
        int word_w = 0;
        for (int k = wi; k <= wend; k++) word_w += g[k].n;

        /* ── wrap test (bVar20): break before this word if the row is non-empty
         *    and adding the word would overflow `width`. ── */
        int wrapped = 0;
        if (!(row_w == 0 || (word_w + row_w) <= width)) {
            wrapped = 1;
            row++;
            row_w = 0;
        }

        /* ── assign each glyph its (col,row); honor forced `%n` breaks ── */
        int col = row_w;            /* uVar15 starts at the row's width so far */
        for (int k = wi; k <= wend; k++) {
            if (g[k].brk && (!wrapped || wi < k)) {   /* forced break (not a 2nd) */
                row++;
                col   = 0;
                row_w = 0;
            }
            wrap_emit(out, row, &g[k]);
            emitted = 1;
            col   += g[k].n;        /* col = running width within the row */
            row_w  = col;
        }

        wi = wend + 1;
    }

    out->row_count = emitted ? (row + 1) : 0;
}
