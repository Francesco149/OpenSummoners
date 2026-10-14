/* rh_trace.c — the input script (replay) and recorder (human play).
 *
 * Format (.jsonl, one object per line; '#' lines are comments):
 *   {"frame": N, "keys": [DIK, ...]}   from flip-frame N on, EXACTLY these DIK scancodes are
 *                                      held (a LEVEL, sticky until the next entry) — the same
 *                                      schema as the Frida-era held trace / OSS_INPUT_RECORD
 *                                      (up 0xc8 down 0xd0 left 0xcb right 0xcd)
 *   {"savefile": ...} / {"meta": ...}  ignored here (rhost.py stages saves)
 * Frame N = the frames after N flips (frame 0 = boot until the first Flip). The fake
 * keyboard (rh_dinput.c) reports the level via GetDeviceState and synthesizes the matching
 * GetDeviceData edge events at the frame boundary, so record ≡ replay exactly. */
#include "rhost.h"

typedef struct { uint32_t frame; uint8_t keys[256]; } entry_t;
static entry_t *g_e; static int g_n, g_cap, g_cur;
static uint8_t g_none[256];
static FILE *g_rec; static uint8_t g_rec_last[256]; static int g_rec_lines;

static void add(uint32_t frame, const uint8_t *keys)
{
    if (g_n == g_cap) { g_cap = g_cap ? g_cap * 2 : 256; g_e = realloc(g_e, g_cap * sizeof *g_e); }
    g_e[g_n].frame = frame; memcpy(g_e[g_n].keys, keys, 256); g_n++;
}

void rh_trace_init(void)
{
    if (O.record) {
        g_rec = fopen(O.record, "w");
        if (!g_rec) rh_fatal("cannot write %s", O.record);
        fprintf(g_rec, "# rhost record (sotes %s): {\"frame\":N,\"keys\":[DIK..]} held levels per flip\n", g_ed->tag);
        fprintf(g_rec, "{\"frame\":0,\"keys\":[]}\n"); fflush(g_rec);
        return;
    }
    if (!O.trace) return;
    FILE *f = fopen(O.trace, "r");
    if (!f) rh_fatal("cannot read trace %s", O.trace);
    char line[8192]; int ln = 0;
    while (fgets(line, sizeof line, f)) {
        ln++;
        char *p = line; while (*p == ' ' || *p == '\t') p++;
        if (*p != '{') continue;
        char *fr = strstr(p, "\"frame\""), *ks = strstr(p, "\"keys\"");
        if (!fr && strstr(p, "\"ids\"")) rh_fatal("trace line %d: legacy ring \"ids\" — convert with rhost.py (needs the key map)", ln);
        if (!fr || !ks) continue;
        uint32_t frame = strtoul(strchr(fr + 7, ':') + 1, NULL, 0);
        uint8_t k[256] = {0};
        char *b = strchr(ks, '['), *e = b ? strchr(b, ']') : NULL;
        if (!b || !e) rh_fatal("trace line %d: bad keys", ln);
        for (char *q = b + 1; q < e; ) {
            while (q < e && (*q == ' ' || *q == ',')) q++;
            if (q >= e) break;
            char *end; unsigned long v = strtoul(q, &end, 0);
            if (end == q || v > 255) rh_fatal("trace line %d: bad scancode", ln);
            k[v] = 0x80; q = end;
        }
        if (g_n && frame < g_e[g_n - 1].frame) rh_fatal("trace line %d: frame %u goes backwards", ln, frame);
        add(frame, k);
    }
    fclose(f);
    rh_log("trace: %d entries, last frame %u\n", g_n, rh_trace_last_frame());
}

const uint8_t *rh_trace_keys(uint32_t frame)
{
    while (g_cur + 1 < g_n && g_e[g_cur + 1].frame <= frame) g_cur++;
    if (!g_n || g_e[g_cur].frame > frame) return g_none;
    return g_e[g_cur].keys;
}
uint32_t rh_trace_last_frame(void) { return g_n ? g_e[g_n - 1].frame : 0; }

void rh_trace_record(uint32_t frame, const uint8_t keys[256])
{
    if (!g_rec || !memcmp(keys, g_rec_last, 256)) return;
    memcpy(g_rec_last, keys, 256);
    fprintf(g_rec, "{\"frame\":%u,\"keys\":[", frame);
    int first = 1;
    for (int i = 0; i < 256; i++) if (keys[i] & 0x80) { fprintf(g_rec, first ? "%d" : ",%d", i); first = 0; }
    fprintf(g_rec, "]}\n"); fflush(g_rec);
    g_rec_lines++;
}

void rh_trace_finish(void)
{
    if (g_rec) { fprintf(g_rec, "{\"frame\":%u,\"keys\":[],\"end\":1}\n", g_frame); fclose(g_rec); g_rec = NULL; }
}

const char *rh_trace_status(void)
{
    static char b[96];
    if (O.record) snprintf(b, sizeof b, "recorded=%d", g_rec_lines);
    else snprintf(b, sizeof b, "trace_entries=%d complete=%d", g_n, g_n ? (g_frame >= rh_trace_last_frame()) : 1);
    return b;
}
