/* ui_text.c — the UI toolkit's text layer.  See ui_text.h.
 *
 * Transcribed from the asm (IDA's decompile of 0x40e5e0 hides the AX-over-pointer guard below).  Heap
 * blocks go through the engine's operator new (g_eng.op_new) in retail's order. */
#include "ui_text.h"

#include <stddef.h>
#include <string.h>

#include "engine_calls.h"
#include "glyph_text.h"   /* glyph_token_search 0x40fd20 (SJIS-aware search) */

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const void *D(uint32_t va) { return g_eng.data(va); }

/* MSVC6 atol over the digit run the parser collected (no sign / blanks; wraps like retail) */
static int32_t digits_atol(const char *s)
{
    uint32_t v = 0;
    while (*s >= '0' && *s <= '9') v = 10u * v + (uint32_t)(*s++ - '0');
    return (int32_t)v;
}

/* the ruby pass of 0x4031c0 / 0x40fa00 over G's records from `base`: each table word found in `text`
 * (entries of 164 B: the word, one reading per character at +0x10 + 9k, a context test at +0xa0) */
static void ruby_pass(uint8_t *g, const char *text, uint16_t len, uint16_t base)
{
    for (const char *e = D(0x5cd978u); e[0] != 0; e += 164) {
        if (len == 0) continue;
        uint16_t from = 0;
        for (;;) {
            int32_t at = glyph_token_search(text, e, from);
            if (at < 0) break;
            int32_t cond = ld32(e, 0xa0);
            if (cond == 0 || g_eng.h4034f0(text, cond, at, (int32_t)strlen(e)) != 0) {
                int32_t ch = 0;
                for (int32_t i = 0; i < at; i++, ch++)
                    if ((signed char)text[i] < 0) i++;
                if (ld16((uint8_t *)ldp(g, 0) + 36u * (uint32_t)(base + ch), 0x1c) == 0) {
                    int32_t skip = 0, ofs = 0;
                    if (e[0] == '#') {
                        ((uint8_t *)ldp(g, 0))[36u * (uint32_t)(base + ch)] = 0;
                        skip = 2;
                        ((uint8_t *)ldp(g, 0))[36u * (uint32_t)(base + ch + 1)] = 0;
                    }
                    uint32_t b = 0;
                    if (strlen(e) != 0) {
                        for (uint16_t k = 0;; k++) {
                            const char *kana = e + 16 + 9 * k;
                            uint32_t ro = 36u * (uint32_t)(k + base + ch + skip);
                            g_eng.h4051d0((uint8_t *)ldp(g, 0) + ro, kana);
                            st32(ldp(g, 0), ro + 0x20, ofs);
                            int32_t d = (int32_t)(strlen(kana) >> 1) - 2;
                            if (d > 0) ofs += d;
                            if ((signed char)e[(uint16_t)b] < 0) b++;
                            b++;
                            if ((uint16_t)b >= strlen(e)) break;
                        }
                    }
                }
            }
            if ((uint16_t)(at + 1) >= len) break;
            from = (uint16_t)(at + 1);
        }
    }
}

/* one record per glyph (an SJIS lead takes its trail byte) from record `o`; returns the next record */
static uint16_t recs_fill(uint8_t *g, const char *text, uint16_t n, uint16_t o)
{
    for (uint16_t i = 0; i < n; o++, i++) {
        uint8_t *r = (uint8_t *)ldp(g, 0) + 36u * o;
        char c = text[i];
        r[0] = (uint8_t)c;
        if (c < 0) {
            i++;
            r[1] = (uint8_t)text[i];
            r[2] = 0;
        } else {
            r[1] = 0;
        }
        st16(r, 0x1c, 0);
    }
    return o;
}

void glyph_recs_set(uint8_t *g, const char *text, int32_t append)
{
    uint32_t n = (uint32_t)strlen(text);
    uint16_t base;
    if (append != 0) {
        uint16_t len = ld16(g, 0xa);
        if (ld32(g, 4) <= (int32_t)(len + (n & 0xffffu))) return;
        base = ld16(g, 8);
        st16(g, 0xa, (uint16_t)(len + n));
    } else {
        if (ld32(g, 4) <= (int32_t)(n & 0xffffu)) return;
        base = 0;
        st16(g, 0xa, (uint16_t)n);
    }
    st16(g, 8, recs_fill(g, text, (uint16_t)n, base));
    ruby_pass(g, text, (uint16_t)n, base);
}

void grid_cell_text(uint8_t *c, int32_t row, int32_t col, const char *text)
{
    const uint8_t *gi = ldp(c, 0x174);
    if (ld32(gi, 8) <= col || ld32(gi, 0x10) <= row || row < 0) return;
    if (ld32(ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc), 24u * col) == 0) {
        uint8_t *g = g_eng.op_new(0xc);
        if (g != NULL) {
            st32(g, 4, 80);
            stp(g, 0, g_eng.op_new(0xb40));
            st16(g, 8, 0);
            st16(g, 0xa, 0);
        }
        stp(ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc), 24u * col, g);
    }
    uint8_t *g = ldp(ldp((uint8_t *)ldp(c, 0x17c) + 16 * row, 0xc), 24u * col);
    uint32_t n = (uint32_t)strlen(text);
    if (ld32(g, 4) <= (int32_t)(n & 0xffffu)) return;
    st16(g, 0xa, (uint16_t)n);
    st16(g, 8, recs_fill(g, text, (uint16_t)n, 0));
    ruby_pass(g, text, (uint16_t)n, 0);
}

char *text_parse(uint8_t *w, const char *text)
{
    uint32_t n = (uint32_t)strlen(text);
    uint16_t o = 0, gl = 0;
    if ((uint16_t)n != 0) {
        for (uint32_t k = 0; k < (n & 0xffffu); k++) {
            st32(ldp(w, 0), 12u * k + 4, 0);
            ((uint8_t *)ldp(w, 0))[12u * k + 8] = 0;
            ((uint8_t *)ldp(w, 0))[12u * k + 9] = 0;
        }
        uint16_t p = 0;
        do {
            const char *s = text + p;
            if (*s == '%') {
                int d = s[1] - 'm';
                if (d == 0) {                          /* %mN%: the glyph's value */
                    char num[12];
                    uint16_t i = 0;
                    for (;;) {
                        char ch = text[(uint32_t)p + 2];
                        if (ch == '%') { p++; break; }
                        if (ch < '0' || ch > '9' || i >= 10) break;
                        num[i++] = ch;
                        p++;
                    }
                    num[i] = 0;
                    st32(ldp(w, 0), 12u * gl + 4, digits_atol(num));
                    p++;
                } else if (d == 1) {                   /* %n: break before this glyph */
                    p++;
                    ((uint8_t *)ldp(w, 0))[12u * gl + 8] = 1;
                } else if (d == 10) {                  /* %w */
                    p++;
                    ((uint8_t *)ldp(w, 0))[12u * gl + 9] = 5;
                } else {
                    p++;
                }
            } else {
                char *out = ldp(w, 8);
                out[o] = *s;
                if (*s < 0) {
                    o++;
                    p++;
                    out[o] = text[p];
                }
                gl++;
                o++;
            }
            p++;
        } while (p < (uint16_t)n);
    }
    ((char *)ldp(w, 8))[o] = 0;
    return ldp(w, 8);
}

static int letter(signed char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
/* SJIS closing brackets ）」』 */
static int closer(const char *q) { return !strcmp(q, "\x81\x6a") || !strcmp(q, "\x81\x76") || !strcmp(q, "\x81\x78"); }

uint8_t *text_wrap(uint8_t *w, uint16_t width)
{
    uint8_t *g = ldp(w, 0xc);
    uint16_t n = ld16(g, 8);
    if (n == 0) return g;
    uint32_t start = 0, line = 0;
    uint16_t col = 0;
    do {
        const uint8_t *recs = ldp(g, 0);
        uint16_t s = (uint16_t)start;
        uint32_t end = start;
        int cls;
        /* the word: a letter run (+ '), a number (1-9 , .), or SJIS glyphs bound to the punctuation /
         * closing bracket after them; a single other glyph alone.  A trailing blank / mark joins it. */
        signed char c0 = (signed char)recs[36u * s];
        if (c0 < 0) cls = 3;
        else if (letter(c0)) cls = 1;
        else if (c0 >= '0' && c0 <= '9') cls = 2;
        else cls = 0;
        if (cls != 0) {
            uint32_t nx = start + 1;
            while ((uint16_t)nx < n) {
                const char *q = (const char *)recs + 36u * (uint16_t)nx;
                signed char c = q[0];
                if (cls == 2) {
                    if (!((c >= '1' && c <= '9') || c == ',' || c == '.')) {
                        if (c == ' ') end++;
                        break;
                    }
                } else if (cls == 1) {
                    if (!(letter(c) || c == '\'')) {
                        if (c == ' ' || c == '!' || c == ',' || c == '-' || c == '.' || c == ';' || c == '?') end++;
                        break;
                    }
                } else {
                    int go = (int32_t)(uint16_t)nx < (int32_t)n - 1 &&
                             closer((const char *)recs + 36u * (uint16_t)(nx + 1));
                    if (!go)
                        go = !strcmp(q, "\x81\x45") || !strcmp(q, "\x81\x42") || !strcmp(q, "\x81\x41") ||
                             !strcmp(q, "\x81\x49") || !strcmp(q, "\x81\x48") || !strcmp(q, "!") || !strcmp(q, "?");
                    if (!go) break;
                }
                end++;
                nx++;
            }
        }
        /* a word that does not fit the line starts the next one */
        uint32_t wd = 0;
        if (s <= (uint16_t)end) {
            const uint8_t *r = recs + 36u * s;
            for (uint32_t k = (end & 0xffffu) - s + 1; k != 0; k--, r += 36) wd += (signed char)r[0] < 0 ? 2 : 1;
        }
        int broke = 0;
        if (col != 0 && (wd & 0xffffu) + col > width) {
            broke = 1;
            line++;
            col = 0;
        }
        if (s <= (uint16_t)end) {
            uint32_t it = start, idx = s, po = 12u * s, ro = 36u * s;
            do {
                uint8_t *pe = (uint8_t *)ldp(w, 0) + po;
                if (pe[8] != 0 && (broke == 0 || (uint16_t)it > s)) {
                    line++;
                    col = 0;
                }
                st16(pe, 0, col);
                st16((uint8_t *)ldp(w, 0) + po, 2, (uint16_t)line);
                if (cls != 3) {
                    const uint8_t *gg = ldp(w, 0xc);
                    const char *rr = ldp(gg, 0);
                    switch (rr[ro]) {
                    case ' ': ((uint8_t *)ldp(w, 0))[po + 9] = 4; break;
                    case ',': ((uint8_t *)ldp(w, 0))[po + 9] = 2; break;
                    case '!': case '.': case ';': case '?':
                        if ((int32_t)idx < (int32_t)ld16(gg, 8) - 1) {
                            const char *q = rr + 36u * (uint16_t)(it + 1);
                            if (!strcmp(q, "!") || !strcmp(q, "?") || !strcmp(q, ".") || !strcmp(q, ";")) break;
                        }
                        ((uint8_t *)ldp(w, 0))[po + 9] = 3;
                        break;
                    default: break;
                    }
                } else if ((uint16_t)it == (uint16_t)end) {
                    uint8_t *p9 = (uint8_t *)ldp(w, 0) + po + 9;
                    if (*p9 == 0) {
                        const uint8_t *gg = ldp(w, 0xc);
                        const char *rr = ldp(gg, 0);
                        /* retail loads only AX = G's count over p9's address before `dec eax / cmp`, so the
                         * guard is (p9's high half | count) - 1: in practice the record past the last glyph
                         * is read too */
                        int32_t lim = (int32_t)(((uint32_t)(uintptr_t)p9 & 0xffff0000u) | ld16(gg, 8)) - 1;
                        if ((int32_t)idx < lim && closer(rr + 36u * (uint16_t)(it + 1))) {
                            *p9 = 4;
                        } else {
                            const char *q = rr + ro;
                            if (!strcmp(q, "\x81\x45")) *p9 = 1;                      /* ・ */
                            else if (!strcmp(q, "\x81\x42")) *p9 = 3;                 /* 。 */
                            else if (!strcmp(q, "\x81\x41")) *p9 = 2;                 /* 、 */
                            else if (!strcmp(q, "\x81\x49") || !strcmp(q, "\x81\x48") || !strcmp(q, "!") ||
                                     !strcmp(q, "?")) *p9 = 3;                        /* ！？!? */
                            else *p9 = 0;
                        }
                    }
                }
                signed char c = ((const char *)ldp(ldp(w, 0xc), 0))[ro];
                if (c != 0) col = (uint16_t)(col + (c < 0 ? 2 : 1));
                it++;
                idx++;
                po += 12;
                ro += 36;
            } while ((uint16_t)it <= (uint16_t)end);
        }
        start = end + 1;
        g = ldp(w, 0xc);
        n = ld16(g, 8);
    } while ((uint16_t)start < n);
    return g;
}

intptr_t text_layout_set(uint8_t *w, const char *text, uint16_t width)
{
    uint16_t cap = ld16(w, 4);
    if (cap <= strlen(text)) return cap;
    text_parse(w, text);
    glyph_recs_set(ldp(w, 0xc), ldp(w, 8), 0);
    return (intptr_t)text_wrap(w, width);
}

char *key_token_replace(char *s, const uint8_t *e)
{
    char rep[512], tmp[1024];   /* (retail: 512 each) */
    int32_t tl = (int32_t)strlen((const char *)e);
    char *r = NULL;   /* retail's eax (the token byte last compared): no caller reads it */
    if (ld32(e, 0x50) != 0) g_eng.h5bf3ee(rep, "%%m%d%%%s", ld32(e, 0x50), (const char *)e + 0x28);
    else strcpy(rep, (const char *)e + 0x28);
    int32_t rl = (int32_t)strlen(rep), from = 0;
    for (;;) {
        int32_t n = (int32_t)strlen(s), at = from;
        r = NULL;
        if (at >= n) return r;
        for (;;) {
            int32_t k = 0;
            if (tl > 0)
                for (;;) {
                    r = (char *)e + k;
                    if (at + k >= n || s[at + k] != *r) break;
                    if (++k >= tl) break;
                }
            if (k >= tl) break;
            if (++at >= n) return r;
        }
        int32_t m = 0;
        if (at > 0) { memcpy(tmp, s, (size_t)at); m = at; }
        if (rl > 0) { memcpy(tmp + m, rep, (size_t)rl); m += rl; }
        if (at + tl < n) { memcpy(tmp + m, s + at + tl, (size_t)(n - at - tl)); m += n - at - tl; }
        tmp[m] = 0;
        strcpy(s, tmp);
        from = at + rl;
    }
}

/* every key token of the table in `s` → its replacement (0x40e360 inlines 0x412800 per token) */
static void keys_expand(const uint8_t *c, char *s)
{
    for (uint16_t i = 0; (uint32_t)i < (uint32_t)ld32(c, 0x16c); i++)
        key_token_replace(s, (const uint8_t *)ldp(c, 0x164) + 100u * i);
}

intptr_t ctrl_text_set(uint8_t *c, const char *text)
{
    char buf[512];
    const char *src = text;
    if (ldp(c, 0x164) != NULL) {
        strcpy(buf, text);
        keys_expand(c, buf);
        src = buf;
    }
    uint8_t *t = ldp(c, 0x170);
    uint16_t width = ld16(t, 8);
    uint8_t *w = ldp(t, 0);
    intptr_t r = ld16(w, 4);
    if ((uint32_t)r > strlen(src)) {
        char *plain = text_parse(w, src);
        glyph_recs_set(ldp(w, 0xc), plain, 0);
        r = (intptr_t)text_wrap(w, width);
    }
    st16(ldp(c, 0x170), 4, 0);
    st16(ldp(c, 0x170), 6, 0);
    st32(c, 0x1c, 0);
    return r;
}

void *ctrl_keys_alloc(uint8_t *c, int32_t n)
{
    if (ldp(c, 0x164) != NULL) {
        g_eng.op_delete(ldp(c, 0x164));
        stp(c, 0x164, NULL);
    }
    void *t = g_eng.op_new(100u * (uint32_t)n);
    st32(c, 0x168, n);
    stp(c, 0x164, t);
    st32(c, 0x16c, 0);
    return t;
}

int32_t ctrl_key_add(uint8_t *c, const char *tok, const char *rep, int32_t sprite, int16_t frame, int32_t dx, int32_t dy)
{
    if (ldp(c, 0x164) == NULL) return 0;
    uint32_t i = (uint32_t)ld32(c, 0x16c);
    if (i >= (uint32_t)ld32(c, 0x168)) return 0;
    uint8_t *e = (uint8_t *)ldp(c, 0x164) + 100u * i;
    strcpy((char *)e, tok);
    strcpy((char *)e + 0x28, rep);
    if (sprite != 0) {
        st32(e, 0x54, sprite);
        st16(e, 0x58, (uint16_t)frame);
        st32(e, 0x50, (int32_t)i + 10000);
        st32(e, 0x5c, dx);
        st32(e, 0x60, dy);
    } else {
        st32(e, 0x54, 0);
        st32(e, 0x50, 0);
    }
    st32(c, 0x16c, (int32_t)i + 1);
    return 1;
}

char *ctrl_keys_expand(uint8_t *c, const char *text, char *out)
{
    if (ldp(c, 0x164) == NULL) return (char *)text;
    strcpy(out, text);
    keys_expand(c, out);
    return out;
}

int32_t sys_msg_keys(void *sys, int32_t unused, uint8_t *c)
{
    (void)unused;
    /* the key tokens: the confirm / cancel / menu keys (+ "K": as an icon), the arrows, End, F1..F5,
     * the A / S buttons, the +/- markers, then the bare forms (retail VAs: token, replacement) */
    static const struct { uint32_t tok, rep; int16_t frame; int32_t dx, dy; } keys[] = {
        { 0x854994u, 0x854990u, 8, 0, -2 },  { 0x854988u, 0x854990u, 9, 0, -2 },  { 0x854980u, 0x854990u, 10, 0, -2 },
        { 0x854978u, 0x854990u, 0, 0, -2 },  { 0x854970u, 0x854990u, 1, 0, -2 },  { 0x854968u, 0x854990u, 2, 0, -2 },
        { 0x854960u, 0x854990u, 3, 0, -2 },  { 0x854958u, 0x854990u, 4, 0, -4 },  { 0x854950u, 0x854990u, 0, 0, -2 },
        { 0x854948u, 0x854990u, 1, 0, -2 },  { 0x854940u, 0x854990u, 2, 0, -2 },  { 0x854938u, 0x854990u, 3, 0, -2 },
        { 0x854928u, 0x854930u, 16, 0, -2 }, { 0x854920u, 0x854930u, 17, 0, -2 }, { 0x854918u, 0x854930u, 18, 0, -2 },
        { 0x854910u, 0x854930u, 19, 0, -2 }, { 0x854908u, 0x854930u, 20, 0, -2 }, { 0x854900u, 0x854990u, 9, 0, -2 },
        { 0x8548f8u, 0x854990u, 8, 0, -2 },  { 0x8548f0u, 0x854990u, 24, 2, -2 }, { 0x8548e8u, 0x854990u, 25, 2, -2 },
        { 0x8548e4u, 0x854990u, 8, 0, -2 },  { 0x8548e0u, 0x854990u, 9, 0, -2 },  { 0x8548dcu, 0x854990u, 10, 0, -2 },
        { 0x8548d4u, 0x854990u, 4, 0, -4 },  { 0x8548ccu, 0x854930u, 16, 0, -2 }, { 0x8548c4u, 0x854930u, 17, 0, -2 },
        { 0x8548bcu, 0x854930u, 18, 0, -2 }, { 0x8548b4u, 0x854930u, 19, 0, -2 }, { 0x8548acu, 0x854930u, 20, 0, -2 },
        { 0x8548a8u, 0x854990u, 0, 0, -2 },  { 0x8548a4u, 0x854990u, 1, 0, -2 },  { 0x8548a0u, 0x854990u, 3, 0, -2 },
    };
    if (ld32(ldp(ldp(sys, 0x320), 0), 0x254) == 0) {
        st32(c, 0x14, 1);
        st32(c, 0x18, -9);
    }
    st32(ldp(c, 0x170), 0x1c, 20);
    ctrl_keys_alloc(c, 50);
    int32_t r = 0;
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
        r = ctrl_key_add(c, D(keys[i].tok), D(keys[i].rep), ld32(sys, 0xbbc), keys[i].frame, keys[i].dx, keys[i].dy);
    return r;
}

uint8_t *sys_box_cursor(void *sys, const uint8_t *win, uint8_t *c, int32_t dx, int32_t dy)
{
    static const uint16_t frames[4] = { 0, 1, 2, 3 };
    st32(c, 0x20, 1);
    st32(c, 0x28, ld32(sys, 0xb8c));
    st16(c, 0x2c, 20);
    st16(c, 0x6e, 4);
    memcpy(c + 0x2e, frames, sizeof frames);
    st16(c, 0x70, 10);
    st16(c, 0x74, 0);
    st16(c, 0x72, 0);
    st32(c, 0x78, 0);
    st32(c, 0x7c, ld32(win, 0x14) - ld32(c, 0xc) + dx - 32);
    st32(c, 0x24, 0);
    st32(c, 0x80, ld32(win, 0x18) - ld32(c, 0x10) + dy - 32);
    return c;
}
