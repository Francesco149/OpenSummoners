/* rh_serve.c — --serve PORT: drive the game (retail or the standalone) interactively, frame by frame, over a socket.
 * openrecet's rh_live.c protocol (tools/drive: the `rd` CLI) on SotES: the frame boundary is the Flip (the flip
 * coroutine, rhost.c), input is DirectInput key LEVELS (a 32-bit mask over the keys the game reads, KEYMAP below),
 * screen text is what GDI TextOutA drew this frame.
 *
 * rhost becomes a server on 127.0.0.1:PORT (0 = any free port → <run>\serve.port "PORT PID"). One text line in, one
 * line out ("ok ..." / "err ..."). The client is stateless; this process is its warm cache — everything it holds can
 * be rebuilt by replaying the movie.
 *
 *   ping                                   frame / rng / epoch / poked
 *   run M:N[,M:N..] [u=P] [a=P] [s=P,P..] [e=K]
 *                                          hold key mask M (hex) for N frames, segment by segment (M = pID: the mask is
 *                                          program ID's value, evaluated before each frame — input that reads state); stop
 *                                          when program u holds ("until") or a holds ("avoid"); sample the s programs
 *                                          every K frames (default: at the end)
 *   r VA LEN / w VA HEX                    read / poke memory (a poke is not in the movie: `poked` is set)
 *   prog ID tok tok ..                     define RPN program ID (0..255); eval ID,ID..
 *   anchor | snap | restore ID | free ID   savestates (rh_snap.c); an anchor invalidates every id (epoch+1)
 *   diff ID [max=N] [lo=VA] [hi=VA]        dwords that differ from savestate ID (mark → act → diff)
 *   text                                   the TextOutA calls of the last frame (x:y:color:hex, in draw order)
 *   hash | stats | movie | dump PATH | shot PATH   state hashes / counters / the input movie since boot / .rhs dump /
 *                                          the composition surface as a PNG (headless pixels: no replay needed)
 *   explore g=P c=P,P.. [a=P] acts=A|A|.. [t=MS] [refine=MS] [seed=N] [maxlen=N] [sticky=PCT] [apply=1]
 *           [score=P] [greed=G] [hold=N [hacts=A|A..] [hgoal=0]]   (openrecet's search, unchanged)
 *   quit                                   end the run (RESULT line, exit)
 *
 * The movie: every key level the game has seen since boot (mask changes), a persistent list whose head is part of
 * each savestate — a restore rewinds the movie with the state. `movie` returns it; as a trace (rd converts the masks
 * back to DIK lists) replayed in a fresh rhost (any device mode, retail or the standalone) it reaches this state.
 * With --trace the trace is replayed first (to its end or --serve-until F), its levels entering the movie. */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include "rhost.h"
#include <math.h>
#include <stddef.h>
#include <stdarg.h>

/* the keys a mask bit stands for (tools/drive/drive.py KEYS is the same table) */
static const uint8_t KEYMAP[32] = {
    0xc8, 0xd0, 0xcb, 0xcd,             /*  0..3  Up Down Left Right */
    0x2d, 0x2e, 0x2c, 0x2f, 0x21,       /*  4..8  X (attack/talk/confirm) C (jump/cancel) Z (sword/spell select) V (examine) F (heal item) */
    0x1c, 0x01, 0x0e,                   /*  9..11 Enter (change character / skip text / confirm) Esc Backspace */
    0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, /* 12..17 F1 (main menu) .. F6 */
    0x52, 0x4f, 0x50, 0x51, 0x4b, 0x4c, 0x4d, 0x47, 0x48, 0x49,   /* 18..27 Num0..Num9 (spell select) */
    0xc9, 0xd1, 0xc7,                   /* 28..30 PgUp PgDn Home (move POV) */
    0x39,                               /* 31 Space */
};
static uint32_t keys_to_mask(const uint8_t k[256], int *extra)
{
    uint32_t m = 0; *extra = 0;
    for (int sc = 0; sc < 256; sc++) if (k[sc] & 0x80) {
        int b = -1; for (int i = 0; i < 32; i++) if (KEYMAP[i] == sc) b = i;
        if (b >= 0) m |= 1u << b; else (*extra)++;
    }
    return m;
}
static void mask_to_keys(uint32_t m, uint8_t k[256]) { memset(k, 0, 256); for (int i = 0; i < 32; i++) if (m >> i & 1) k[KEYMAP[i]] = 0x80; }

/* ---------------- string builder ---------------- */
typedef struct { char *p; size_t n, cap; } sb_t;
static void sb_put(sb_t *b, const char *s, size_t n)
{
    if (b->n + n + 1 > b->cap) { b->cap = (b->n + n + 1) * 2 + 256; b->p = realloc(b->p, b->cap); }
    memcpy(b->p + b->n, s, n); b->n += n; b->p[b->n] = 0;
}
static void sb_f(sb_t *b, const char *fmt, ...)
{
    char t[512]; va_list ap; va_start(ap, fmt); int n = vsnprintf(t, sizeof t, fmt, ap); va_end(ap);
    if (n > 0) sb_put(b, t, (size_t)(n < (int)sizeof t ? n : (int)sizeof t - 1));
}

/* ---------------- the movie ---------------- */
typedef struct mnode { struct mnode *parent; uint32_t frame, value; uint32_t kind; } mnode_t;   /* kind 0: mask from frame on */
static mnode_t *g_mhead; static uint32_t g_mrec = 0xffffffffu;
static mnode_t *g_mchunk; static uint32_t g_mleft; static uint64_t g_mnodes;
static void movie_add(uint32_t frame, uint32_t value, uint32_t kind)
{
    if (!g_mleft) { g_mchunk = malloc(65536 * sizeof(mnode_t)); g_mleft = 65536; if (!g_mchunk) rh_fatal("movie: out of memory"); }
    mnode_t *n = g_mchunk++; g_mleft--; g_mnodes++;
    n->parent = g_mhead; n->frame = frame; n->value = value; n->kind = kind;
    g_mhead = n;
}
static int g_owns_input, g_poked; static long g_extra_keys;
static uint32_t g_epoch = 1;
int rh_serve_owns_input(void) { return g_owns_input; }
/* the level for the frame about to run (g_frame): into the movie, then into the fake keyboard */
static void live_latch(uint32_t mask)
{
    if (mask != g_mrec) { movie_add(g_frame, mask, 0); g_mrec = mask; }
    uint8_t k[256]; mask_to_keys(mask, k);
    rh_dinput_set(g_frame, k);
}
static mnode_t **movie_list(mnode_t *head, mnode_t *stop, size_t *n)
{
    size_t k = 0;
    for (mnode_t *m = head; m && m != stop; m = m->parent) k++;
    mnode_t **v = malloc((k ? k : 1) * sizeof *v);
    size_t i = k;
    for (mnode_t *m = head; m && m != stop; m = m->parent) v[--i] = m;
    *n = k;
    return v;
}

/* ---------------- screen text: every TextOutA of the frame (rh_gdi.c) ---------------- */
typedef struct { int32_t x, y; uint32_t color; uint16_t len; char s[126]; } trow_t;
#define MAX_TROW 512
static trow_t g_trow[MAX_TROW]; static int g_ntrow;
void rh_serve_text(HDC dc, int x, int y, const char *s, int n)
{
    if (g_ntrow >= MAX_TROW || !s || n <= 0) return;
    trow_t *t = &g_trow[g_ntrow++];
    t->x = x; t->y = y; t->color = (uint32_t)GetTextColor(dc);
    if (n > (int)sizeof t->s) n = (int)sizeof t->s;
    memcpy(t->s, s, (size_t)n); t->len = (uint16_t)n;
}
static void *text_save(void)
{
    size_t n = sizeof(int) + (size_t)g_ntrow * sizeof(trow_t);
    uint8_t *b = malloc(n); memcpy(b, &g_ntrow, sizeof(int)); memcpy(b + sizeof(int), g_trow, (size_t)g_ntrow * sizeof(trow_t));
    return b;
}
static void text_restore(const void *p) { memcpy(&g_ntrow, p, sizeof(int)); memcpy(g_trow, (const uint8_t *)p + sizeof(int), (size_t)g_ntrow * sizeof(trow_t)); }

/* ---------------- programs (postfix) ---------------- */
enum { OP_NUM, OP_R8, OP_R16, OP_R32, OP_S8, OP_S16, OP_S32, OP_F32, OP_F64, OP_FRAME, OP_RNG, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_FDIV,
       OP_MOD, OP_AND, OP_OR, OP_XOR, OP_SHL, OP_SHR, OP_INV, OP_LT, OP_LE, OP_GT, OP_GE, OP_EQ, OP_NE, OP_LAND, OP_LOR, OP_LNOT,
       OP_NEG, OP_ABS, OP_MIN, OP_MAX, OP_SEL, OP_SQRT, OP_SIN, OP_COS, OP_ATAN2, OP_HYPOT, OP_POW };
static const char *const OPN[] = { "", "r8", "r16", "r32", "s8", "s16", "s32", "f32", "f64", "frame", "rng", "+", "-", "*", "/", "//", "%",
                                   "&", "|", "^", "<<", ">>", "~", "<", "<=", ">", ">=", "==", "!=", "and", "or", "not", "neg", "abs",
                                   "min", "max", "sel", "sqrt", "sin", "cos", "atan2", "hypot", "pow" };
typedef struct { uint8_t op; double v; } ins_t;
typedef struct { int n; ins_t *c; } prog_t;
#define NPROG 256
static prog_t g_prog[NPROG];
static long g_bad_reads;

static int mem_ok(uint32_t a, uint32_t n)
{
    if (a >= RT_IMG_LO && a + n <= RT_IMG_HI && a + n >= a) return 1;
    MEMORY_BASIC_INFORMATION mi;
    for (uint32_t p = a & ~0xfffu; p < a + n; ) {
        if (!VirtualQuery((void *)(uintptr_t)p, &mi, sizeof mi) || mi.State != MEM_COMMIT || (mi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return 0;
        uint32_t hi = (uint32_t)(uintptr_t)mi.BaseAddress + (uint32_t)mi.RegionSize;
        if (hi <= p) return 0;
        p = hi;
    }
    return 1;
}
static double vm_eval(const prog_t *p)
{
    double st[64]; int sp = 0;
    for (int i = 0; i < p->n; i++) {
        const ins_t *x = &p->c[i];
        #define A st[sp - 2]
        #define B st[sp - 1]
        #define BIN(expr) do { if (sp < 2) return 0; double r_ = (expr); sp--; st[sp - 1] = r_; } while (0)
        #define I64(v) ((int64_t)(v))
        switch (x->op) {
        case OP_NUM: if (sp < 64) st[sp++] = x->v; break;
        case OP_FRAME: if (sp < 64) st[sp++] = g_frame; break;
        case OP_RNG: if (sp < 64) st[sp++] = G32(g_ed->rng_seed); break;
        case OP_R8: case OP_R16: case OP_R32: case OP_S8: case OP_S16: case OP_S32: case OP_F32: case OP_F64: {
            if (sp < 1) return 0;
            uint32_t a = (uint32_t)(int64_t)st[sp - 1], n = x->op == OP_F64 ? 8 : (x->op == OP_R8 || x->op == OP_S8) ? 1 : (x->op == OP_R16 || x->op == OP_S16) ? 2 : 4;
            if (!mem_ok(a, n)) { g_bad_reads++; st[sp - 1] = 0; break; }
            const void *q = (const void *)(uintptr_t)a;
            st[sp - 1] = x->op == OP_R8 ? *(const uint8_t *)q : x->op == OP_S8 ? *(const int8_t *)q : x->op == OP_R16 ? *(const uint16_t *)q :
                         x->op == OP_S16 ? *(const int16_t *)q : x->op == OP_R32 ? *(const uint32_t *)q : x->op == OP_S32 ? *(const int32_t *)q :
                         x->op == OP_F32 ? (double)*(const float *)q : *(const double *)q;
            break;
        }
        case OP_ADD: BIN(A + B); break;
        case OP_SUB: BIN(A - B); break;
        case OP_MUL: BIN(A * B); break;
        case OP_DIV: BIN(B != 0 ? A / B : 0); break;
        case OP_FDIV: BIN(B != 0 ? floor(A / B) : 0); break;
        case OP_MOD: BIN(B != 0 ? A - floor(A / B) * B : 0); break;
        case OP_AND: BIN((double)(I64(A) & I64(B))); break;
        case OP_OR: BIN((double)(I64(A) | I64(B))); break;
        case OP_XOR: BIN((double)(I64(A) ^ I64(B))); break;
        case OP_SHL: BIN((double)(I64(A) << (I64(B) & 63))); break;
        case OP_SHR: BIN((double)(I64(A) >> (I64(B) & 63))); break;
        case OP_LT: BIN(A < B); break;
        case OP_LE: BIN(A <= B); break;
        case OP_GT: BIN(A > B); break;
        case OP_GE: BIN(A >= B); break;
        case OP_EQ: BIN(A == B); break;
        case OP_NE: BIN(A != B); break;
        case OP_LAND: BIN(A != 0 && B != 0); break;
        case OP_LOR: BIN(A != 0 || B != 0); break;
        case OP_MIN: BIN(A < B ? A : B); break;
        case OP_MAX: BIN(A > B ? A : B); break;
        case OP_INV: if (sp < 1) return 0; st[sp - 1] = (double)(uint32_t)~(uint32_t)I64(st[sp - 1]); break;
        case OP_LNOT: if (sp < 1) return 0; st[sp - 1] = st[sp - 1] == 0; break;
        case OP_NEG: if (sp < 1) return 0; st[sp - 1] = -st[sp - 1]; break;
        case OP_ABS: if (sp < 1) return 0; st[sp - 1] = fabs(st[sp - 1]); break;
        case OP_SQRT: if (sp < 1) return 0; st[sp - 1] = st[sp - 1] > 0 ? sqrt(st[sp - 1]) : 0; break;
        case OP_SIN: if (sp < 1) return 0; st[sp - 1] = sin(st[sp - 1]); break;
        case OP_COS: if (sp < 1) return 0; st[sp - 1] = cos(st[sp - 1]); break;
        case OP_ATAN2: BIN(atan2(A, B)); break;
        case OP_HYPOT: BIN(sqrt(A * A + B * B)); break;
        case OP_POW: BIN((A < 0 && B != floor(B)) || (A == 0 && B < 0) ? 0 : pow(A, B)); break;
        case OP_SEL: if (sp < 3) return 0; { double r = st[sp - 3] != 0 ? st[sp - 2] : st[sp - 1]; sp -= 2; st[sp - 1] = r; } break;
        }
        #undef A
        #undef B
        #undef BIN
        #undef I64
    }
    return sp ? st[sp - 1] : 0;
}
static int prog_define(int id, char **tok, int ntok, sb_t *err)
{
    ins_t *c = malloc((ntok ? ntok : 1) * sizeof *c);
    for (int i = 0; i < ntok; i++) {
        int op = -1;
        for (int k = 1; k < (int)(sizeof OPN / sizeof OPN[0]); k++) if (!strcmp(tok[i], OPN[k])) { op = k; break; }
        if (op < 0) {
            char *e; double v = (tok[i][0] == '0' && (tok[i][1] == 'x' || tok[i][1] == 'X')) ? (double)strtoull(tok[i], &e, 16) : strtod(tok[i], &e);
            if (*e || e == tok[i]) { sb_f(err, "bad token '%s'", tok[i]); free(c); return -1; }
            c[i].op = OP_NUM; c[i].v = v;
        } else { c[i].op = (uint8_t)op; c[i].v = 0; }
    }
    free(g_prog[id].c); g_prog[id].c = c; g_prog[id].n = ntok;
    return 0;
}
static int pid_of(const char *s) { char *e; long v = strtol(s, &e, 10); return (*e || e == s || v < 0 || v >= NPROG || !g_prog[v].c) ? -1 : (int)v; }
static int pid_list(const char *s, int *out, int max)
{
    int n = 0; char b[512]; snprintf(b, sizeof b, "%s", s);
    for (char *t = strtok(b, ","); t; t = strtok(NULL, ",")) { if (n == max) return -1; int p = pid_of(t); if (p < 0) return -1; out[n++] = p; }
    return n;
}

/* ---------------- stepping ---------------- */
typedef struct { uint32_t mask; int16_t prog; uint32_t n; } seg_t;   /* prog >= 0: the mask is that program's value per tick */
static int parse_segs(const char *s, seg_t **out, char sep)
{
    int n = 0, cap = 16; seg_t *v = malloc(cap * sizeof *v);
    while (*s) {
        char *e; unsigned long m = 0; long pg = -1;
        if (*s == 'p') { pg = strtol(s + 1, &e, 10); if (e == s + 1 || pg < 0 || pg >= NPROG || !g_prog[pg].c) { free(v); return -1; } }
        else { m = strtoul(s, &e, 16); if (e == s) { free(v); return -1; } }
        if (*e != ':') { free(v); return -1; }
        s = e + 1;
        unsigned long k = strtoul(s, &e, 10);
        if (e == s || (*e && *e != sep)) { free(v); return -1; }
        if (n == cap) { cap *= 2; v = realloc(v, cap * sizeof *v); }
        v[n].mask = (uint32_t)m; v[n].prog = (int16_t)pg; v[n].n = (uint32_t)k; n++;
        s = *e ? e + 1 : e;
    }
    *out = v;
    return n;
}
static double now_ms(void)
{
    static LARGE_INTEGER f; LARGE_INTEGER t;
    if (!f.QuadPart) QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)f.QuadPart;
}
static void tick(uint32_t mask) { live_latch(mask); g_ntrow = 0; rh_step(); }
static void tick_seg(const seg_t *s) { tick(s->prog >= 0 ? (uint32_t)(int64_t)vm_eval(&g_prog[s->prog]) : s->mask); }

/* ---------------- explore ---------------- */
typedef struct { uint64_t key; int snap; uint32_t frames, chosen, born; mnode_t *head; double v[6], score; } cell_t;
static uint64_t g_xs;
static uint32_t xrand(void) { g_xs ^= g_xs << 13; g_xs ^= g_xs >> 7; g_xs ^= g_xs << 17; return (uint32_t)(g_xs >> 16); }
static double xunit(void) { return xrand() / 4294967296.0; }
static uint64_t cell_key(const int *cp, int nc, double *vals)
{
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < nc; i++) {
        double v = vm_eval(&g_prog[cp[i]]); if (i < 6) vals[i] = v;
        uint64_t u; memcpy(&u, &v, 8); if (v == 0) u = 0;
        h ^= u; h *= 1099511628211ull; h ^= h >> 29;
    }
    return h ? h : 1;
}
static void path_segs(mnode_t *head, mnode_t *root_head, uint32_t root_mask, uint32_t root_frame, uint32_t frames, sb_t *out)
{
    size_t n; mnode_t **v = movie_list(head, root_head, &n);
    uint32_t m = root_mask; uint32_t at = root_frame, end = root_frame + frames; int first = 1;   /* frame labels [at, end): a node's frame = the frame its keys are held for */
    for (size_t i = 0; i <= n; i++) {
        uint32_t f = i < n ? v[i]->frame : end;
        if (i < n && v[i]->kind != 0) continue;
        if (f > end) f = end;
        if (f > at) { sb_f(out, "%s%x:%u", first ? "" : ",", m, f - at); first = 0; at = f; }
        if (i < n) m = v[i]->value;
    }
    free(v);
}
/* score (a program, -1 = none; higher = closer): cells are picked in proportion to e^(greed * rank-normalised
 * score) on top of the visit-count weight, a cell keeps its best-scored arrival (ties: the shorter), and when the
 * goal is never met the reply carries the path to the best-scored cell (bpath) — progress the client can keep. */
/* pool full: drop the savestates of the most-expanded cells (never cell 0 = the root, nor KEEP) */
static long evict_cells(cell_t *cells, int n, const cell_t *keep)
{
    uint64_t sum = 0; int live = 0; long ev = 0;
    for (int i = 1; i < n; i++) if (cells[i].snap) { sum += cells[i].chosen; live++; }
    uint32_t thr = live ? (uint32_t)(sum / (uint64_t)live) : 0;
    for (int i = 1; i < n; i++) if (cells[i].snap && cells[i].chosen >= thr && &cells[i] != keep) { rh_snap_free(cells[i].snap); cells[i].snap = 0; ev++; }
    return ev;
}
/* one hold tail from a goal arrival: 1 = it survives (the state is its end), 0 = it does not (the caller restores) */
typedef struct { int n, goal, avoid, need_goal, nact; seg_t **acts; const int *alen; } hold_t;
static int hold_try(const hold_t *h, int which, uint64_t *ticks)
{
    int s = 0; uint32_t i = 0;
    for (int k = 0; k < h->n; k++) {
        while (i >= h->acts[which][s].n) { i = 0; s = (s + 1) % h->alen[which]; }
        tick_seg(&h->acts[which][s]); i++; (*ticks)++;
        if (h->avoid >= 0 && vm_eval(&g_prog[h->avoid]) != 0) return 0;
    }
    return !h->need_goal || vm_eval(&g_prog[h->goal]) != 0;
}
static void do_explore(int goal, int avoid, const int *cp, int nc, seg_t **acts, const int *alen, int nact, double tlimit, double refine,
                       int maxlen, double sticky, int apply, int score, double greed, const hold_t *hold, sb_t *out)
{
    if (!rh_snap_anchored() || rh_snap_dirty() > 12000) { if (rh_snap_anchor()) { sb_f(out, "err anchor failed"); return; } g_epoch++; }
    int root = rh_snap_take();
    if (!root) { sb_f(out, "err no savestate (pool exhausted?)"); return; }
    mnode_t *root_head = g_mhead; uint32_t root_mask = g_mrec; uint32_t root_frame = g_frame;
    int cap = 4096, n = 0, mapcap = 16384; cell_t *cells = malloc(cap * sizeof *cells); int *map = calloc(mapcap, sizeof *map);
    #define FIND(k, slot) do { slot = (uint32_t)((k) * 11400714819323198485ull >> 40) % (uint32_t)mapcap; \
                               while (map[slot] && cells[map[slot] - 1].key != (k)) slot = (slot + 1) % (uint32_t)mapcap; } while (0)
    double vals[6] = { 0 }; uint64_t k0 = cell_key(cp, nc, vals); uint32_t s0; FIND(k0, s0);
    double sc0 = score >= 0 ? vm_eval(&g_prog[score]) : 0, smin = sc0, smax = sc0;
    cells[0] = (cell_t){ k0, root, 0, 0, 0, root_head, { vals[0], vals[1], vals[2], vals[3], vals[4], vals[5] }, sc0 }; map[s0] = 1; n = 1;
    #define WEIGHT(i) ((1.0 / sqrt(1.0 + cells[i].chosen)) * (it - cells[i].born <= 2 ? 3.0 : 1.0) * \
                       (score >= 0 && smax > smin ? exp(greed * (cells[i].score - smin) / (smax - smin)) : 1.0))
    double t0 = now_ms(), tbest = 0; uint32_t best = 0, it = 0; uint64_t ticks = 0; mnode_t *best_head = NULL; long pruned = 0, evicted = 0, rejected = 0, held = 0; int best_tail = -1;
    uint32_t hn = hold && hold->n > 0 ? (uint32_t)hold->n : 0;
    while (1) {
        double now = now_ms();
        if (!best && now - t0 > tlimit) break;
        if (best && now - tbest > refine) break;
        it++;
        double tw = 0;
        for (int i = 0; i < n; i++) {
            if (!cells[i].snap || (best && cells[i].frames + 1 >= best)) continue;
            tw += WEIGHT(i);
        }
        if (tw <= 0) break;
        double pick = xunit() * tw; int ei = -1;
        for (int i = 0; i < n; i++) {
            if (!cells[i].snap || (best && cells[i].frames + 1 >= best)) continue;
            ei = i; pick -= WEIGHT(i); if (pick <= 0) break;
        }
        cells[ei].chosen++;
        rh_snap_restore(cells[ei].snap);
        uint32_t f = cells[ei].frames; int a = (int)(xrand() % (uint32_t)nact), len = 2 + (int)(xrand() % (uint32_t)(maxlen - 1)), stop = 0;
        for (int k = 0; k < len && !stop; k++) {
            if (k && xunit() > sticky) a = (int)(xrand() % (uint32_t)nact);
            for (int s = 0; s < alen[a] && !stop; s++)
                for (uint32_t i = 0; i < acts[a][s].n && !stop; i++) {
                    tick_seg(&acts[a][s]); f++; ticks++;
                    if (avoid >= 0 && vm_eval(&g_prog[avoid]) != 0) { pruned++; stop = 1; }   /* avoid wins on the goal's frame */
                    else if (vm_eval(&g_prog[goal]) != 0) {
                        stop = 1;
                        if (best && f >= best) break;
                        int tail = -1;
                        if (hn) {                       /* does it survive? savestate the arrival, try each tail */
                            int hs = rh_snap_take();
                            if (!hs) { evicted += evict_cells(cells, n, &cells[ei]); hs = rh_snap_take(); }
                            if (!hs) { rejected++; break; }   /* no room to test it: not a goal */
                            for (int t = 0; t < hold->nact && tail < 0; t++) {
                                if (t) rh_snap_restore(hs);
                                if (hold_try(hold, t, &ticks)) tail = t;
                            }
                            rh_snap_free(hs);
                            if (tail < 0) { rejected++; break; }
                            held++;
                        }
                        best = f; best_head = g_mhead; best_tail = tail; tbest = now_ms();
                    }
                    else if (avoid >= 0 && vm_eval(&g_prog[avoid]) != 0) { pruned++; stop = 1; }
                    else if (best && f + 1 >= best) stop = 1;
                }
            if (stop) break;
            uint64_t key = cell_key(cp, nc, vals); uint32_t slot; FIND(key, slot);
            cell_t *c = map[slot] ? &cells[map[slot] - 1] : NULL;
            double sc = score >= 0 ? vm_eval(&g_prog[score]) : 0;
            if (c && c->snap && !(sc > c->score || (sc == c->score && f < c->frames))) continue;
            int id = rh_snap_take();
            if (!id) {                                  /* pool full: drop the savestates of the most-expanded cells */
                evicted += evict_cells(cells, n, c);
                id = rh_snap_take();
                if (!id) continue;
            }
            if (c) { if (c->snap && c->snap != root) rh_snap_free(c->snap); c->chosen /= 2; }   /* a better-scored arrival may replace the root's cell: its savestate stays */
            else {
                if (n == cap) { cap *= 2; cells = realloc(cells, cap * sizeof *cells); }
                if ((n + 1) * 2 > mapcap) {             /* rehash */
                    mapcap *= 2; free(map); map = calloc(mapcap, sizeof *map);
                    for (int i = 0; i < n; i++) { uint32_t s2; FIND(cells[i].key, s2); map[s2] = i + 1; }
                    FIND(key, slot);
                }
                c = &cells[n++]; map[slot] = n; c->key = key; c->chosen = 0;
                for (int i = 0; i < 6; i++) c->v[i] = vals[i];
            }
            c->snap = id; c->frames = f; c->born = it; c->head = g_mhead; c->score = sc;
            if (sc < smin) smin = sc;
            if (sc > smax) smax = sc;
        }
    }
    double ms = now_ms() - t0;
    rh_snap_restore(root);
    sb_f(out, "ok found=%d frames=%u cells=%d iters=%u ticks=%llu ms=%.0f pruned=%ld evicted=%ld dirty=%u epoch=%u", best != 0, best ? best + hn : 0, n, it,
         (unsigned long long)ticks, ms, pruned, evicted, rh_snap_dirty(), g_epoch);
    if (hn) sb_f(out, " hold=%u held=%ld rejected=%ld", hn, held, rejected);
    if (best && hn) sb_f(out, " arrive=%u tail=%d", best, best_tail);
    if (best) {
        sb_t path = { 0 }; path_segs(best_head, root_head, root_mask, root_frame, best + hn, &path);
        sb_f(out, " path="); sb_put(out, path.p ? path.p : "", path.n);
        if (apply) {
            seg_t *sg; int ns = parse_segs(path.p ? path.p : "", &sg, ',');
            for (int s = 0; s < ns; s++) for (uint32_t i = 0; i < sg[s].n; i++) tick(sg[s].mask);
            if (ns >= 0) free(sg);
            sb_f(out, " applied=1 holds=%d avoid=%d frame=%u", vm_eval(&g_prog[goal]) != 0, avoid >= 0 && vm_eval(&g_prog[avoid]) != 0, g_frame);
        }
        free(path.p);
    } else {                                            /* how far it got: the newest cells */
        sb_f(out, " newest=");
        int shown = 0;
        for (int i = n - 1; i >= 0 && shown < 8; i--, shown++) {
            sb_f(out, "%s%u@", shown ? ";" : "", cells[i].frames);
            for (int k = 0; k < nc && k < 6; k++) sb_f(out, "%s%.6g", k ? "/" : "", cells[i].v[k]);
        }
        if (score >= 0) {                               /* … and the best-scored one, with the way there */
            int bi = 0;
            for (int i = 1; i < n; i++)
                if (cells[i].score > cells[bi].score || (cells[i].score == cells[bi].score && cells[i].frames < cells[bi].frames)) bi = i;
            sb_f(out, " score0=%.9g bscore=%.9g bframes=%u", sc0, cells[bi].score, cells[bi].frames);
            if (cells[bi].frames) {
                sb_t path = { 0 }; path_segs(cells[bi].head, root_head, root_mask, root_frame, cells[bi].frames, &path);
                sb_f(out, " bpath="); sb_put(out, path.p ? path.p : "", path.n); free(path.p);
            }
        }
    }
    for (int i = 0; i < n; i++) if (cells[i].snap && cells[i].snap != root) rh_snap_free(cells[i].snap);
    rh_snap_free(root);
    free(cells); free(map);
    #undef FIND
    #undef WEIGHT
}

/* ---------------- the server ---------------- */
static SOCKET g_ls = INVALID_SOCKET, g_cs = INVALID_SOCKET;
static char g_in[1 << 16]; static int g_in_n;
static void serve_install(void)
{
    WSADATA w; if (WSAStartup(MAKEWORD(2, 2), &w)) rh_fatal("serve: WSAStartup failed");
    g_ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in a; memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = htons((u_short)O.serve_port);
    int one = 1; setsockopt(g_ls, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof one);
    if (g_ls == INVALID_SOCKET || bind(g_ls, (struct sockaddr *)&a, sizeof a) || listen(g_ls, 4))
        rh_fatal("serve: cannot listen on 127.0.0.1:%d (err %d)", O.serve_port, WSAGetLastError());
    int al = sizeof a; getsockname(g_ls, (struct sockaddr *)&a, &al);
    O.serve_port = ntohs(a.sin_port);
    char p[MAX_PATH]; snprintf(p, sizeof p, "%s\\serve.port.tmp", O.rundir);
    char q[MAX_PATH]; snprintf(q, sizeof q, "%s\\serve.port", O.rundir);
    FILE *f = fopen(p, "w"); if (f) { fprintf(f, "%d %lu\n", O.serve_port, (unsigned long)GetCurrentProcessId()); fclose(f); MoveFileExA(p, q, MOVEFILE_REPLACE_EXISTING); }
    rh_snap_register("movie.head", &g_mhead, sizeof g_mhead); rh_snap_register("movie.rec", &g_mrec, sizeof g_mrec);
    rh_snap_module("text", text_save, text_restore, free);
    g_xs = 0x9E3779B97F4A7C15ull;
    rh_log("serve: on 127.0.0.1:%d\n", O.serve_port);
}
/* a session nobody talks to for --serve-idle seconds (default 1800) ends itself: its whole state can be rebuilt */
static int wait_readable(SOCKET s)
{
    fd_set f; FD_ZERO(&f); FD_SET(s, &f);
    struct timeval tv = { (long)(O.serve_idle ? O.serve_idle : 1800), 0 };
    return select(0, &f, NULL, NULL, &tv) > 0;
}
static int read_line(char *out, int cap)
{
    for (;;) {
        for (int i = 0; i < g_in_n; i++)
            if (g_in[i] == '\n') {
                int n = i < cap - 1 ? i : cap - 1;
                memcpy(out, g_in, (size_t)n); out[n] = 0;
                if (n && out[n - 1] == '\r') out[n - 1] = 0;
                memmove(g_in, g_in + i + 1, (size_t)(g_in_n - i - 1)); g_in_n -= i + 1;
                return n;
            }
        if (g_in_n == (int)sizeof g_in) g_in_n = 0;
        if (!wait_readable(g_cs)) rh_finish("serve-idle");
        int r = recv(g_cs, g_in + g_in_n, (int)sizeof g_in - g_in_n, 0);
        if (r <= 0) return -1;
        g_in_n += r;
    }
}
static void send_all(const char *p, size_t n)
{
    while (n) { int r = send(g_cs, p, (int)(n > (1 << 20) ? (1 << 20) : n), 0); if (r <= 0) return; p += r; n -= (size_t)r; }
}
static const char *opt(char **tok, int ntok, const char *key)
{
    size_t kl = strlen(key);
    for (int i = 1; i < ntok; i++) if (!strncmp(tok[i], key, kl) && tok[i][kl] == '=') return tok[i] + kl + 1;
    return NULL;
}
static void status(sb_t *o) { sb_f(o, "frame=%u rng=%08x epoch=%u poked=%d", g_frame, G32(g_ed->rng_seed), g_epoch, g_poked); }
int rh_shot_to(const char *path);
static struct dctx { sb_t *o; long left; } g_dc;
static void diff_cb(uint32_t va, uint32_t was, uint32_t now, void *u) { struct dctx *d = u; if (d->left-- > 0) sb_f(d->o, "%s%x:%x:%x", d->o->p[d->o->n - 1] == '=' ? "" : ",", va, was, now); }

static void command(char *line, sb_t *o)
{
    char *tok[600]; int nt = 0;
    for (char *t = strtok(line, " \t"); t && nt < 600; t = strtok(NULL, " \t")) tok[nt++] = t;
    if (!nt) { sb_f(o, "err empty"); return; }
    const char *c = tok[0];
    if (!strcmp(c, "ping")) { sb_f(o, "ok "); status(o); }
    else if (!strcmp(c, "run")) {
        if (nt < 2) { sb_f(o, "err run M:N[,M:N..]"); return; }
        seg_t *sg; int ns = parse_segs(tok[1], &sg, ',');
        if (ns < 0) { sb_f(o, "err bad segments"); return; }
        const char *su = opt(tok, nt, "u"), *sa = opt(tok, nt, "a"), *ss = opt(tok, nt, "s"), *se = opt(tok, nt, "e");
        int u = su ? pid_of(su) : -1, a = sa ? pid_of(sa) : -1, sp[32], nsp = ss ? pid_list(ss, sp, 32) : 0;
        if ((su && u < 0) || (sa && a < 0) || nsp < 0) { free(sg); sb_f(o, "err undefined program"); return; }
        uint32_t every = se ? (uint32_t)strtoul(se, 0, 10) : 0;
        uint32_t ran = 0; const char *why = "end"; sb_t smp = { 0 };
        for (int s = 0; s < ns && why[0] == 'e'; s++)
            for (uint32_t i = 0; i < sg[s].n; i++) {
                tick_seg(&sg[s]); ran++;
                if (every && ran % every == 0) {
                    sb_f(&smp, "%s%u", smp.n ? ";" : "", g_frame);
                    for (int k = 0; k < nsp; k++) sb_f(&smp, ":%.9g", vm_eval(&g_prog[sp[k]]));
                }
                if (u >= 0 && vm_eval(&g_prog[u]) != 0) { why = "until"; break; }
                if (a >= 0 && vm_eval(&g_prog[a]) != 0) { why = "avoid"; break; }
            }
        free(sg);
        sb_f(o, "ok ran=%u stop=%s ", ran, why); status(o);
        if (nsp) {
            sb_f(o, " now=");
            for (int k = 0; k < nsp; k++) sb_f(o, "%s%.9g", k ? ":" : "", vm_eval(&g_prog[sp[k]]));
            if (every) { sb_f(o, " samples="); sb_put(o, smp.p ? smp.p : "", smp.n); }
        }
        free(smp.p);
    }
    else if (!strcmp(c, "prog")) {
        int id = nt >= 2 ? atoi(tok[1]) : -1;
        if (id < 0 || id >= NPROG) { sb_f(o, "err prog ID(0..%d) tokens", NPROG - 1); return; }
        sb_t e = { 0 };
        if (prog_define(id, tok + 2, nt - 2, &e)) { sb_f(o, "err %s", e.p); free(e.p); return; }
        sb_f(o, "ok id=%d n=%d", id, nt - 2);
    }
    else if (!strcmp(c, "eval")) {
        int ids[64], n = nt >= 2 ? pid_list(tok[1], ids, 64) : -1;
        if (n < 0) { sb_f(o, "err undefined program"); return; }
        long bad0 = g_bad_reads;
        sb_f(o, "ok v=");
        for (int i = 0; i < n; i++) sb_f(o, "%s%.17g", i ? ":" : "", vm_eval(&g_prog[ids[i]]));
        sb_f(o, " badreads=%ld frame=%u", g_bad_reads - bad0, g_frame);
    }
    else if (!strcmp(c, "r")) {
        uint32_t a = nt >= 3 ? (uint32_t)strtoul(tok[1], 0, 16) : 0, n = nt >= 3 ? (uint32_t)strtoul(tok[2], 0, 0) : 0;
        if (!n || n > (4u << 20) || !mem_ok(a, n)) { sb_f(o, "err unreadable %08x+%u", a, n); return; }
        sb_f(o, "ok ");
        char *h = malloc(n * 2 + 1); static const char X[] = "0123456789abcdef";
        for (uint32_t i = 0; i < n; i++) { uint8_t b = *(const uint8_t *)(uintptr_t)(a + i); h[i * 2] = X[b >> 4]; h[i * 2 + 1] = X[b & 15]; }
        sb_put(o, h, n * 2); free(h);
    }
    else if (!strcmp(c, "w")) {
        uint32_t a = nt >= 3 ? (uint32_t)strtoul(tok[1], 0, 16) : 0; size_t hl = nt >= 3 ? strlen(tok[2]) : 0;
        if (!hl || (hl & 1) || !mem_ok(a, (uint32_t)(hl / 2))) { sb_f(o, "err w VA HEX"); return; }
        for (size_t i = 0; i < hl / 2; i++) { unsigned b; sscanf(tok[2] + i * 2, "%2x", &b); *(volatile uint8_t *)(uintptr_t)(a + i) = (uint8_t)b; }
        g_poked = 1;
        sb_f(o, "ok poked=%u", (unsigned)(hl / 2));
    }
    else if (!strcmp(c, "anchor")) { if (rh_snap_anchor()) sb_f(o, "err anchor failed (see rhost.log)"); else { g_epoch++; sb_f(o, "ok epoch=%u", g_epoch); } }
    else if (!strcmp(c, "snap")) {
        if (!rh_snap_anchored()) { if (rh_snap_anchor()) { sb_f(o, "err anchor failed"); return; } g_epoch++; }
        int id = rh_snap_take();
        if (!id) sb_f(o, "err savestate pool exhausted"); else sb_f(o, "ok id=%d frame=%u epoch=%u dirty=%u", id, g_frame, g_epoch, rh_snap_dirty());
    }
    else if (!strcmp(c, "restore")) {
        int id = nt >= 2 ? atoi(tok[1]) : 0;
        if (rh_snap_restore(id)) sb_f(o, "err no savestate %d (epoch %u)", id, g_epoch); else { sb_f(o, "ok "); status(o); }
    }
    else if (!strcmp(c, "free")) { for (int i = 1; i < nt; i++) rh_snap_free(atoi(tok[i])); sb_f(o, "ok"); }
    else if (!strcmp(c, "diff")) {
        int id = nt >= 2 ? atoi(tok[1]) : 0;
        const char *sm = opt(tok, nt, "max"), *slo = opt(tok, nt, "lo"), *shi = opt(tok, nt, "hi");
        g_dc.o = o; g_dc.left = sm ? atol(sm) : 400;
        sb_f(o, "ok d=");
        long n = rh_snap_diff(id, slo ? (uint32_t)strtoul(slo, 0, 16) : 0, shi ? (uint32_t)strtoul(shi, 0, 16) : 0xffffffffu, diff_cb, &g_dc);
        if (n < 0) { o->n = 0; sb_f(o, "err no savestate %d", id); return; }
        sb_f(o, " n=%ld frame=%u", n, g_frame);
    }
    else if (!strcmp(c, "hash")) {
        rh_hash_t h; rh_state_hash(&h);
        sb_f(o, "ok full=%016llx sem=%016llx semv=%016llx dev=%016llx fb=%016llx bytes=%u ", (unsigned long long)h.full, (unsigned long long)h.semantic,
             (unsigned long long)h.semv, (unsigned long long)rh_state_dev_hash(), (unsigned long long)rh_fb_chain(), h.bytes); status(o);
    }
    else if (!strcmp(c, "stats")) { char b[320]; rh_snap_stats(b, sizeof b); sb_f(o, "ok %s movie_nodes=%llu badreads=%ld extra_keys=%ld", b, (unsigned long long)g_mnodes, g_bad_reads, g_extra_keys); }
    else if (!strcmp(c, "movie")) {
        size_t n; mnode_t **v = movie_list(g_mhead, NULL, &n);
        sb_f(o, "ok frame=%u n=%u poked=%d ops=", g_frame, (unsigned)n, g_poked);
        for (size_t i = 0; i < n; i++) sb_f(o, "%sm%u:%x", i ? "," : "", v[i]->frame, v[i]->value);
        free(v);
    }
    else if (!strcmp(c, "text")) {
        sb_f(o, "ok frame=%u n=%d rows=", g_frame, g_ntrow);
        static const char X[] = "0123456789abcdef";
        for (int i = 0; i < g_ntrow; i++) {
            const trow_t *t = &g_trow[i];
            char h[sizeof t->s * 2 + 1];
            for (int k = 0; k < t->len; k++) { uint8_t b = (uint8_t)t->s[k]; h[k * 2] = X[b >> 4]; h[k * 2 + 1] = X[b & 15]; }
            h[t->len * 2] = 0;
            sb_f(o, "%s%d:%d:%06x:", i ? ";" : "", t->x, t->y, t->color);
            sb_put(o, h, (size_t)t->len * 2);
        }
    }
    else if (!strcmp(c, "dump")) { if (nt < 2 || rh_state_dump(tok[1])) sb_f(o, "err dump PATH"); else sb_f(o, "ok"); }
    else if (!strcmp(c, "shot")) { if (nt < 2 || rh_shot_to(tok[1])) sb_f(o, "err shot PATH (no back buffer yet?)"); else sb_f(o, "ok frame=%u", g_frame); }
    else if (!strcmp(c, "explore")) {
        const char *sg = opt(tok, nt, "g"), *sc = opt(tok, nt, "c"), *sa = opt(tok, nt, "a"), *sacts = opt(tok, nt, "acts");
        int g = sg ? pid_of(sg) : -1, a = sa ? pid_of(sa) : -1, cp[16], nc = sc ? pid_list(sc, cp, 16) : -1;
        if (g < 0 || nc <= 0 || (sa && a < 0) || !sacts) { sb_f(o, "err explore g=P c=P,.. acts=A|A.. (undefined program?)"); return; }
        seg_t *acts[64]; int alen[64], nact = 0; char b[4096]; snprintf(b, sizeof b, "%s", sacts);
        for (char *t = strtok(b, "|"); t && nact < 64; t = strtok(NULL, "|")) {
            alen[nact] = parse_segs(t, &acts[nact], '+');
            if (alen[nact] <= 0) { sb_f(o, "err bad action '%s'", t); for (int i = 0; i < nact; i++) free(acts[i]); return; }
            nact++;
        }
        if (!nact) { sb_f(o, "err no actions"); return; }
        const char *shn = opt(tok, nt, "hold"), *sha = opt(tok, nt, "hacts"), *shg = opt(tok, nt, "hgoal");
        seg_t *hacts[64]; int hlen[64], nh = 0;
        hold_t hold = { shn ? atoi(shn) : 0, g, a, shg ? atoi(shg) != 0 : 1, 0, hacts, hlen };
        if (hold.n > 0) {
            char hb[4096]; snprintf(hb, sizeof hb, "%s", sha ? sha : "0:1");
            for (char *t = strtok(hb, "|"); t && nh < 64; t = strtok(NULL, "|")) {
                hlen[nh] = parse_segs(t, &hacts[nh], '+');
                int ok = hlen[nh] > 0; for (int s = 0; ok && s < hlen[nh]; s++) if (!hacts[nh][s].n) ok = 0;
                if (!ok) {
                    sb_f(o, "err bad hold action '%s'", t);
                    if (hlen[nh] > 0) free(hacts[nh]);
                    for (int i = 0; i < nh; i++) free(hacts[i]);
                    for (int i = 0; i < nact; i++) free(acts[i]);
                    return;
                }
                nh++;
            }
            hold.nact = nh;
            if (!nh) { sb_f(o, "err no hold actions"); for (int i = 0; i < nact; i++) free(acts[i]); return; }
        }
        const char *st = opt(tok, nt, "t"), *sr = opt(tok, nt, "refine"), *ssd = opt(tok, nt, "seed"), *sm = opt(tok, nt, "maxlen"), *sk = opt(tok, nt, "sticky"), *sap = opt(tok, nt, "apply");
        if (ssd) { g_xs = 0x9E3779B97F4A7C15ull ^ ((uint64_t)strtoull(ssd, 0, 10) * 0xD1B54A32D192ED03ull); if (!g_xs) g_xs = 1; }
        int maxlen = sm ? atoi(sm) : 8; if (maxlen < 2) maxlen = 2;
        const char *ssc = opt(tok, nt, "score"), *sgr = opt(tok, nt, "greed");
        int scp = ssc ? pid_of(ssc) : -1;
        if (ssc && scp < 0) { sb_f(o, "err explore score=P (undefined program)"); for (int i = 0; i < nact; i++) free(acts[i]); return; }
        do_explore(g, a, cp, nc, acts, alen, nact, st ? atof(st) : 60000, sr ? atof(sr) : 8000, maxlen, sk ? atof(sk) / 100.0 : 0.6, sap && atoi(sap),
                   scp, sgr ? atof(sgr) : 3.0, &hold, o);
        for (int i = 0; i < nact; i++) free(acts[i]);
        for (int i = 0; i < nh; i++) free(hacts[i]);
    }
    else if (!strcmp(c, "quit")) { sb_f(o, "ok bye"); send_all(o->p, o->n); send_all("\n", 1); rh_finish("serve-quit"); }
    else sb_f(o, "err unknown command '%s'", c);
}

/* the host coroutine's loop (rhost.c host_main) once the first Flip is reached */
void rh_serve_main(void)
{
    serve_install();
    /* with --trace: replay it first (its levels go into the movie), to its end or --serve-until F */
    if (O.trace) {
        uint32_t until = O.serve_until ? O.serve_until : 0xffffffffu;
        while (g_frame < until && !rh_trace_done(g_frame)) {
            int extra; uint32_t m = keys_to_mask(rh_trace_keys(g_frame), &extra);
            if (extra) g_extra_keys++;
            g_owns_input = 1;
            if (m != g_mrec) { movie_add(g_frame, m, 0); g_mrec = m; }
            rh_dinput_set(g_frame, rh_trace_keys(g_frame));   /* exactly the trace's keys (a key outside KEYMAP still counts: extra_keys) */
            g_ntrow = 0;
            rh_step();
        }
        rh_log("serve: replayed to frame %u (%s, %ld frame(s) with keys outside the mask)\n", g_frame, rh_trace_status(), g_extra_keys);
    }
    g_owns_input = 1;
    if (g_mrec == 0xffffffffu) { movie_add(g_frame, 0, 0); g_mrec = 0; }
    static char line[1 << 16];
    for (;;) {
        if (g_cs == INVALID_SOCKET) {
            if (!wait_readable(g_ls)) rh_finish("serve-idle");
            g_cs = accept(g_ls, NULL, NULL);
            if (g_cs == INVALID_SOCKET) rh_fatal("serve: accept failed (err %d)", WSAGetLastError());
            int one = 1; setsockopt(g_cs, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
            g_in_n = 0;
        }
        if (read_line(line, sizeof line) < 0) { closesocket(g_cs); g_cs = INVALID_SOCKET; continue; }
        sb_t o = { 0 };
        command(line, &o);
        sb_put(&o, "\n", 1);
        send_all(o.p, o.n);
        free(o.p);
    }
}
