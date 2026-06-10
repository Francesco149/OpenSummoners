/*
 * src/held_trace.c — port-side deterministic held-axis replay (see held_trace.h).
 *
 * A narrow hand-rolled parser for the harness's `{"frame":N,"keys":[..]}` JSONL,
 * plus a level-replay pump that rebuilds the input manager's axis-held slots
 * each frame.  Self-contained (its own tiny scanner, like input_trace.c) so the
 * working ring-replay module is untouched; the format is fixed and small enough
 * that a JSON library would be overkill.
 */

#include "held_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int held_scancode_slot(int32_t scancode)
{
    switch (scancode) {
    case HELD_DIK_UP:    return 0;
    case HELD_DIK_DOWN:  return 1;
    case HELD_DIK_LEFT:  return 2;
    case HELD_DIK_RIGHT: return 3;
    default:             return -1;
    }
}

void held_trace_free(struct held_trace *t)
{
    if (t == NULL) return;
    free(t->entries);
    t->entries = NULL;
    t->count   = 0;
    t->cap     = 0;
    t->cursor  = 0;
    t->cur_n   = 0;
}

/* ─── tiny scanner over [p, end) ─────────────────────────────────────── */

static void skip_ws(const char **pp, const char *end)
{
    const char *p = *pp;
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        p++;
    *pp = p;
}

static void skip_inline_ws(const char **pp, const char *end)
{
    const char *p = *pp;
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    *pp = p;
}

/* Parse a decimal or 0x-hex integer at *pp into *out.  Returns 1 on success
 * and advances *pp past the digits; 0 if no number is present. */
static int parse_number(const char **pp, const char *end, long *out)
{
    char buf[32];
    const char *p = *pp;
    size_t n = 0;

    skip_inline_ws(&p, end);
    if (p < end && (*p == '-' || *p == '+')) {
        if (n < sizeof buf - 1) buf[n++] = *p;
        p++;
    }
    while (p < end && n < sizeof buf - 1 &&
           ((*p >= '0' && *p <= '9') ||
            (*p == 'x' || *p == 'X') ||
            (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) {
        buf[n++] = *p++;
    }
    if (n == 0) return 0;
    buf[n] = '\0';

    char *endp = NULL;
    long v = strtol(buf, &endp, 0);   /* base 0 → handles 0x prefixes */
    if (endp == buf) return 0;
    *out = v;
    *pp = p;
    return 1;
}

static int expect(const char **pp, const char *end, char c)
{
    skip_inline_ws(pp, end);
    if (*pp < end && **pp == c) { (*pp)++; return 1; }
    return 0;
}

/* Parse a quoted key at *pp and report which one via *kind:
 * 0 = "frame", 1 = "keys", -1 = unknown/error.  Advances past the closing
 * quote.  Returns 1 on a well-formed quoted key, 0 on a malformed token. */
static int parse_key(const char **pp, const char *end, int *kind)
{
    if (!expect(pp, end, '"')) return 0;
    const char *p = *pp;
    const char *start = p;
    while (p < end && *p != '"') p++;
    if (p >= end) return 0;
    size_t klen = (size_t)(p - start);
    p++;                                  /* past closing quote */
    *pp = p;

    if (klen == 5 && memcmp(start, "frame", 5) == 0)     *kind = 0;
    else if (klen == 4 && memcmp(start, "keys", 4) == 0) *kind = 1;
    else                                                 *kind = -1;
    return 1;
}

/* Parse one held key — either a decimal/hex scancode or a quoted direction
 * name ("up"/"down"/"left"/"right") — into *out (a DIK scancode).  Returns 1
 * on success. */
static int parse_held_key(const char **pp, const char *end, long *out)
{
    skip_inline_ws(pp, end);
    if (*pp < end && **pp == '"') {
        (*pp)++;                          /* opening quote */
        const char *s = *pp;
        while (*pp < end && **pp != '"') (*pp)++;
        if (*pp >= end) return 0;
        size_t n = (size_t)(*pp - s);
        (*pp)++;                          /* closing quote */
        if      (n == 2 && memcmp(s, "up",    2) == 0) *out = HELD_DIK_UP;
        else if (n == 4 && memcmp(s, "down",  4) == 0) *out = HELD_DIK_DOWN;
        else if (n == 4 && memcmp(s, "left",  4) == 0) *out = HELD_DIK_LEFT;
        else if (n == 5 && memcmp(s, "right", 5) == 0) *out = HELD_DIK_RIGHT;
        else return 0;                    /* unknown name */
        return 1;
    }
    return parse_number(pp, end, out);
}

/* Parse `[a, b, …]` of scancodes/names into e->keys/n_keys.  Returns 1 on success. */
static int parse_key_array(const char **pp, const char *end,
                           struct held_trace_entry *e)
{
    if (!expect(pp, end, '[')) return 0;
    e->n_keys = 0;
    skip_inline_ws(pp, end);
    if (*pp < end && **pp == ']') { (*pp)++; return 1; }   /* empty array */

    for (;;) {
        long v;
        if (!parse_held_key(pp, end, &v)) return 0;
        if (e->n_keys >= HELD_TRACE_MAX_KEYS) return 0;     /* too many keys */
        e->keys[e->n_keys++] = (int32_t)v;
        skip_inline_ws(pp, end);
        if (*pp < end && **pp == ',') { (*pp)++; continue; }
        if (*pp < end && **pp == ']') { (*pp)++; return 1; }
        return 0;                                            /* malformed */
    }
}

/* Parse one `{"frame":N, "keys":[..]}` object at *pp into *out.  Keys may be in
 * any order; an absent "keys" yields n_keys == 0.  Returns 1 on success. */
static int parse_entry(const char **pp, const char *end,
                       struct held_trace_entry *out)
{
    memset(out, 0, sizeof *out);
    int have_frame = 0;

    if (!expect(pp, end, '{')) return 0;
    skip_inline_ws(pp, end);
    if (*pp < end && **pp == '}') { (*pp)++; return have_frame; }

    for (;;) {
        int kind;
        if (!parse_key(pp, end, &kind)) return 0;
        if (!expect(pp, end, ':')) return 0;

        if (kind == 0) {                                    /* frame */
            long v;
            if (!parse_number(pp, end, &v)) return 0;
            out->frame = (uint32_t)v;
            have_frame = 1;
        } else if (kind == 1) {                             /* keys */
            if (!parse_key_array(pp, end, out)) return 0;
        } else {
            return 0;                                        /* unknown key */
        }

        skip_inline_ws(pp, end);
        if (*pp < end && **pp == ',') { (*pp)++; continue; }
        if (*pp < end && **pp == '}') { (*pp)++; break; }
        return 0;
    }
    return have_frame;
}

static int push_entry(struct held_trace *out, const struct held_trace_entry *e)
{
    if (out->count >= HELD_TRACE_MAX_ENTRIES) return 0;
    if (out->count == out->cap) {
        size_t ncap = out->cap ? out->cap * 2 : 16;
        struct held_trace_entry *n =
            (struct held_trace_entry *)realloc(out->entries, ncap * sizeof *n);
        if (n == NULL) return 0;
        out->entries = n;
        out->cap = ncap;
    }
    out->entries[out->count++] = *e;
    return 1;
}

int held_trace_parse_buf(const char *buf, size_t len, struct held_trace *out)
{
    memset(out, 0, sizeof *out);
    if (buf == NULL) return 0;

    const char *p = buf;
    const char *end = buf + len;
    uint32_t last_frame = 0;
    int have_prev = 0;

    while (p < end) {
        skip_ws(&p, end);
        if (p >= end) break;
        if (*p == '#') {                                    /* comment line */
            while (p < end && *p != '\n') p++;
            continue;
        }
        struct held_trace_entry e;
        if (!parse_entry(&p, end, &e)) return 0;
        if (have_prev && e.frame < last_frame) return 0;    /* out of order */
        last_frame = e.frame;
        have_prev = 1;
        if (!push_entry(out, &e)) return 0;
        while (p < end && *p != '\n') {
            if (*p != ' ' && *p != '\t' && *p != '\r' && *p != ',') return 0;
            p++;
        }
    }
    return 1;
}

int held_trace_load(const char *path, struct held_trace *out)
{
    memset(out, 0, sizeof *out);
    if (path == NULL) return 0;

    FILE *f = fopen(path, "rb");
    if (f == NULL) return 0;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 0; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }

    char *buf = (char *)malloc((size_t)sz + 1);
    if (buf == NULL) { fclose(f); return 0; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = '\0';

    int ok = held_trace_parse_buf(buf, got, out);
    free(buf);
    return ok;
}

void held_trace_replay(struct held_trace *t, uint32_t present_frame,
                       input_mgr *mgr)
{
    if (t == NULL || mgr == NULL) return;

    /* Advance to the latest entry at-or-before this frame (the current LEVEL).
     * Multiple entries due this frame: the last one wins. */
    while (t->cursor < t->count && t->entries[t->cursor].frame <= present_frame) {
        const struct held_trace_entry *e = &t->entries[t->cursor];
        t->cur_n = e->n_keys;
        for (uint16_t i = 0; i < e->n_keys; i++) t->cur_keys[i] = e->keys[i];
        t->cursor++;
    }

    /* Rebuild the four managed direction slots from the current held set,
     * clear-then-set (mirroring the producer 0x46a880). */
    for (int slot = 0; slot < 4; slot++) mgr->axis_held[slot] = 0;
    for (uint16_t i = 0; i < t->cur_n; i++) {
        int slot = held_scancode_slot(t->cur_keys[i]);
        if (slot >= 0) mgr->axis_held[slot] = 1;
    }
}
