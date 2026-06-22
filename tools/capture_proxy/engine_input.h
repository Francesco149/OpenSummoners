/* engine_input.h — deterministic RING input injection (M2b-2).
 *
 * Native port of the Frida agent's input injection (opensummoners-agent.js
 * §"deterministic input injection").  On a hidden, unfocused window DInput
 * produces no events, so the engine's input ring is ours to fill: we hook the
 * poll consumer FUN_0043c110 (thiscall, ecx = the current scene's input-manager)
 * and write synthetic press records into its ring so a scripted trace replays as
 * real button presses — menu nav (id 1/2/3/4 = up/down/left/right) and the
 * dialogue / commit (id 0x24=36).
 *
 * Ring layout (docs/findings/input.md): 64 dword slots at mgr+0x0c..mgr+0x108,
 * each a POINTER to a 3-dword record {id@0, ts@4, flag@8}.  The poll scans
 * newest-first (slot 63→0), matches id within a ~100 ms GetTickCount window
 * (ts MUST be the engine's own cached per-frame `now` = the poll's [esp+4] arg,
 * else the (uint32)(now-ts)<=100 test underflows and the press is dropped), and
 * consumes on read.  We fill the top slots (63, 62, …) so each is "newest".
 *
 * Trace: a text file (OSS_INPUT_TRACE, a Windows path).  One entry per line in
 * the existing JSONL shape `{"frame": N, "ids": [36, 3]}` (a tiny tolerant
 * scanner, NOT a real JSON parser — '#' comment lines skipped).  Each entry
 * fires once, at the first poll at-or-after Flip frame N — flip-keyed exactly
 * like the agent, so the proven runs nav.jsonl traces replay verbatim.
 */
#ifndef OSS_ENGINE_INPUT_H
#define OSS_ENGINE_INPUT_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proxy_log.h"
#include "va_detour.h"
#include "engine_hooks.h"   /* g_eh_flip */

#define EI_INPUT_POLL_VA  0x0043c110u
#define EI_RING_BASE_OFF  0x0c
#define EI_RING_SLOTS     64
#define EI_REC_SIZE       0x0c
#define EI_INJECT_POOL_N  32
#define EI_MAX_ENTRIES    2048
#define EI_MAX_IDS        8

/* Held-axis (LEVEL) injection — the leaf key-state query + its kbstate buffer. */
#define EI_KEYDOWN_VA     0x005ba520u  /* FUN_005ba520 __thiscall(scancode), ecx=device */
#define EI_KBSTATE_OFF    0x18         /* device+0x18 = the 256-byte DInput keyboard state */

typedef struct { DWORD frame; DWORD ids[EI_MAX_IDS]; int nids; } ei_entry;

static ei_entry g_ei_trace[EI_MAX_ENTRIES];
static int   g_ei_n        = 0;
static int   g_ei_i        = 0;          /* cursor into the trace */
static void *g_ei_mgr      = NULL;       /* resolved each poll (ecx) */
static BYTE  g_ei_pool[EI_INJECT_POOL_N * EI_REC_SIZE];
static int   g_ei_pool_i   = 0;
static int   g_ei_injected = 0;

/* Parse one trace line in-place; returns 1 if it yielded an entry. */
static int ei_parse_line(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '#' || *s == '\0' || *s == '\n' || *s == '\r') return 0;

    char *fp = strstr(s, "frame");
    if (!fp) return 0;
    char *colon = strchr(fp, ':');
    if (!colon) return 0;
    DWORD frame = (DWORD)strtoul(colon + 1, NULL, 10);

    if (g_ei_n >= EI_MAX_ENTRIES) return 0;
    ei_entry *e = &g_ei_trace[g_ei_n];
    e->frame = frame;
    e->nids = 0;

    char *ids = strstr(s, "ids");
    if (ids) {
        char *lb = strchr(ids, '[');
        if (lb) {
            char *p = lb + 1;
            while (e->nids < EI_MAX_IDS) {
                while (*p == ' ' || *p == ',') p++;
                if (*p == ']' || *p == '\0') break;
                char *end = NULL;
                long v = strtol(p, &end, 10);
                if (end == p) break;
                e->ids[e->nids++] = (DWORD)v;
                p = end;
            }
        }
    }
    g_ei_n++;
    return 1;
}

static void ei_load_trace(void)
{
    char path[MAX_PATH];
    DWORD n = GetEnvironmentVariableA("OSS_INPUT_TRACE", path, sizeof(path));
    if (n == 0 || n >= sizeof(path)) {
        proxy_logf("[input] no OSS_INPUT_TRACE — injection disabled");
        return;
    }
    FILE *f = fopen(path, "rb");
    if (!f) { proxy_logf("[input] FATAL: fopen(%s) failed", path); return; }
    char line[512];
    while (fgets(line, sizeof(line), f)) ei_parse_line(line);
    fclose(f);
    proxy_logf("[input] loaded %d trace entries from %s", g_ei_n, path);
}

/* Write one fresh press record for `id` into ring slot `slotIdx`. */
static void ei_inject_press(DWORD id, int slotIdx, DWORD ts)
{
    BYTE *rec = g_ei_pool + (g_ei_pool_i % EI_INJECT_POOL_N) * EI_REC_SIZE;
    g_ei_pool_i++;
    *(DWORD *)(rec + 0x00) = id;
    *(DWORD *)(rec + 0x04) = ts;
    *(DWORD *)(rec + 0x08) = 1;
    *(void **)((BYTE *)g_ei_mgr + EI_RING_BASE_OFF + slotIdx * 4) = rec;
}

/* Inject every trace entry at-or-before the current flip, once each. */
static void ei_inject_due(DWORD now)
{
    if (!g_ei_mgr) return;
    LONG flip = g_eh_flip;
    while (g_ei_i < g_ei_n && flip >= (LONG)g_ei_trace[g_ei_i].frame) {
        ei_entry *e = &g_ei_trace[g_ei_i];
        for (int j = 0; j < e->nids; j++) {
            ei_inject_press(e->ids[j], EI_RING_SLOTS - 1 - j, now);
            g_ei_injected++;
        }
        proxy_logf("[input] inject @flip %ld (trace_frame %lu): %d id(s) "
                   "[%lu...] now=%lu", flip, (unsigned long)e->frame, e->nids,
                   e->nids ? (unsigned long)e->ids[0] : 0UL, (unsigned long)now);
        g_ei_i++;
    }
}

/* thiscall(now, button_id): entry stack = [retaddr][now][button_id]; ecx=mgr. */
static void ei_poll_cb(PCONTEXT ctx)
{
    g_ei_mgr = (void *)ctx->Ecx;               /* track every poll (sub-scenes) */
    DWORD now = *(DWORD *)(ctx->Esp + 4);
    ei_inject_due(now);
}

/* ── Held-axis (LEVEL) injection ─────────────────────────────────────────────
 * The freeroam movement (walk / dash / the U/D POSE) reads the per-frame
 * HELD-AXIS array (mgr+0x114 UP / +0x118 DOWN / +0x11c LEFT / +0x120 RIGHT /
 * +0x124 C), which the producer FUN_0046a880 fills by calling the leaf key-state
 * query FUN_005ba520(scancode) once per tracked key (UP 0xc8 -> +0x114, DOWN 0xd0
 * -> +0x118, LEFT 0xcb -> +0x11c, RIGHT 0xcd -> +0x120; ~11 calls/frame).  That
 * leaf returns `device[0x18 + scancode] & 0x80` — the DInput keyboard-state high
 * bit.  On the hidden, unfocused window DInput writes nothing, so we make a key
 * report PRESSED by writing 0x80 into the device's kbstate buffer
 * (device+EI_KBSTATE_OFF+scancode) at the query's ENTRY, just before it reads —
 * exactly the byte a physical keypress sets, so the producer then fills mgr+0x114
 * naturally (engine-quirk #41).  This is the LEVEL counterpart to the discrete
 * RING injection above; the pose/walk need a HELD direction (the ring alone is a
 * one-frame edge).  Mirrors the Frida agent's installHeldAxisInjection
 * (opensummoners-agent.js) but writes the buffer instead of replacing the leaf's
 * return (the proxy's VEH detour is entry-only — no onLeave).
 *
 * Trace: OSS_HELD_TRACE, JSONL of {"frame": N, "keys": [scancode|name,...]} —
 * each entry SETS the held set from Flip frame N on (a LEVEL, persisting until
 * the next entry; [] releases all).  up/down/left/right map to the DIK direction
 * scancodes; raw integers (decimal / 0xhex) pass through (the C button etc.).  A
 * scancode that EVER appears in the trace is "managed": each query writes 0x80
 * when it is currently held and 0x00 when not (a clean release), leaving
 * un-managed scancodes at their real (zero) state. */
static ei_entry g_eih_trace[EI_MAX_ENTRIES];   /* reuse ei_entry: ids[] = scancodes */
static int   g_eih_n         = 0;
static int   g_eih_i         = 0;              /* cursor into the held trace */
static LONG  g_eih_last_flip = -1;             /* last flip the cursor advanced for */
static BYTE  g_eih_managed[256];               /* 1 = scancode appears in the trace */
static BYTE  g_eih_held[256];                  /* 1 = scancode held this frame (level) */

/* ── INPUT RECORD (OSS_INPUT_RECORD=path): log the REAL per-frame held set of the
 * gameplay keys to an editable held-trace JSONL ({"frame":N,"keys":[sc,...]}), so a
 * USER's real-play session yields a replayable + editable input trace (the .osr alone
 * had no input — ckpt 157 USER ask).  The 0x5ba520 leaf query (ecx=device) is read
 * (NOT written) for each tracked scancode; on the flip boundary the just-finished
 * frame's held set is emitted iff it changed.  From the held edges the ring (discrete
 * presses: Z draw, X attack, the dialogue confirm) is DERIVED on replay by the real
 * producer 0x46a880, so the held trace captures everything. */
static FILE *g_rec_file     = NULL;
static DWORD g_rec_device   = 0;               /* captured DInput device ptr (ecx)   */
static LONG  g_rec_cur_flip = -1;              /* the frame currently accumulating   */
static BYTE  g_rec_cur[256];                   /* held set seen this frame           */
static BYTE  g_rec_prev[256];                  /* last EMITTED held set (dedup)      */
static int   g_rec_have_prev = 0;
/* The gameplay scancodes we log (arrows + Z/X/C + Enter); others stay un-tracked. */
static const BYTE g_rec_tracked[] = {
    0xc8 /*up*/, 0xd0 /*down*/, 0xcb /*left*/, 0xcd /*right*/,
    0x2c /*Z*/, 0x2d /*X*/, 0x2e /*C*/, 0x1c /*Enter*/, 0x2f /*V*/,
};
#define REC_NTRACKED ((int)(sizeof(g_rec_tracked)/sizeof(g_rec_tracked[0])))

/* Emit the held set for `flip` if it differs from the last emitted one. */
static void eih_record_emit(LONG flip)
{
    if (!g_rec_file) return;
    int changed = !g_rec_have_prev;
    for (int i = 0; i < REC_NTRACKED && !changed; i++)
        if (g_rec_cur[g_rec_tracked[i]] != g_rec_prev[g_rec_tracked[i]]) changed = 1;
    if (!changed) return;
    char buf[256]; int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "{\"frame\":%ld,\"keys\":[", (long)flip);
    int first = 1;
    for (int i = 0; i < REC_NTRACKED; i++) {
        BYTE sc = g_rec_tracked[i];
        if (g_rec_cur[sc]) { n += snprintf(buf + n, sizeof(buf) - n, "%s%u", first ? "" : ",", sc); first = 0; }
    }
    n += snprintf(buf + n, sizeof(buf) - n, "]}\n");
    fputs(buf, g_rec_file);
    fflush(g_rec_file);                        /* survive an abrupt Stop-Process kill */
    memcpy(g_rec_prev, g_rec_cur, sizeof g_rec_prev);
    g_rec_have_prev = 1;
}

/* On a flip boundary, flush the just-finished frame then start the next. */
static void eih_record_on_query(DWORD device)
{
    if (!g_rec_file) return;
    g_rec_device = device;
    if (g_eh_flip != g_rec_cur_flip) {
        if (g_rec_cur_flip >= 0) eih_record_emit(g_rec_cur_flip);
        g_rec_cur_flip = g_eh_flip;
        memset(g_rec_cur, 0, sizeof g_rec_cur);
    }
}

static void eih_record_init(void)
{
    char path[MAX_PATH];
    DWORD n = GetEnvironmentVariableA("OSS_INPUT_RECORD", path, sizeof(path));
    if (n == 0 || n >= sizeof(path)) return;   /* recording off */
    g_rec_file = fopen(path, "wb");
    if (!g_rec_file) { proxy_logf("[input] FATAL: record fopen(%s) failed", path); return; }
    fputs("# OSS_INPUT_RECORD held-trace (real-play): {\"frame\":N,\"keys\":[scancode,...]}\n",
          g_rec_file);
    proxy_logf("[input] RECORDING real input -> %s (%d tracked keys)", path, REC_NTRACKED);
}

/* up/down/left/right -> DIK scancode (else -1); s,len = the bare (unquoted) name. */
static int eih_name_scancode(const char *s, int len)
{
    if (len == 2 && !strncmp(s, "up",    2)) return 0xc8;
    if (len == 4 && !strncmp(s, "down",  4)) return 0xd0;
    if (len == 4 && !strncmp(s, "left",  4)) return 0xcb;
    if (len == 5 && !strncmp(s, "right", 5)) return 0xcd;
    return -1;
}

/* Parse one held-trace line in-place; returns 1 if it yielded an entry. */
static int eih_parse_line(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '#' || *s == '\0' || *s == '\n' || *s == '\r') return 0;

    char *fp = strstr(s, "frame");
    if (!fp) return 0;
    char *colon = strchr(fp, ':');
    if (!colon) return 0;
    DWORD frame = (DWORD)strtoul(colon + 1, NULL, 10);

    if (g_eih_n >= EI_MAX_ENTRIES) return 0;
    ei_entry *e = &g_eih_trace[g_eih_n];
    e->frame = frame;
    e->nids = 0;

    char *keys = strstr(s, "keys");
    if (keys) {
        char *lb = strchr(keys, '[');
        if (lb) {
            char *p = lb + 1;
            while (e->nids < EI_MAX_IDS) {
                while (*p == ' ' || *p == ',') p++;
                if (*p == ']' || *p == '\0') break;
                int sc = -1;
                if (*p == '"') {
                    char *q = p + 1, *end = strchr(q, '"');
                    if (!end) break;
                    sc = eih_name_scancode(q, (int)(end - q));
                    p = end + 1;
                } else {
                    char *end = NULL;
                    long v = strtol(p, &end, 0);   /* base 0: decimal or 0x-hex */
                    if (end == p) break;
                    sc = (int)(v & 0xff);
                    p = end;
                }
                if (sc >= 0) {
                    e->ids[e->nids++] = (DWORD)sc;
                    g_eih_managed[sc & 0xff] = 1;
                }
            }
        }
    }
    g_eih_n++;
    return 1;
}

static void eih_load_trace(void)
{
    char path[MAX_PATH];
    DWORD n = GetEnvironmentVariableA("OSS_HELD_TRACE", path, sizeof(path));
    if (n == 0 || n >= sizeof(path)) {
        proxy_logf("[input] no OSS_HELD_TRACE — held injection disabled");
        return;
    }
    FILE *f = fopen(path, "rb");
    if (!f) { proxy_logf("[input] FATAL: fopen(%s) failed", path); return; }
    char line[512];
    while (fgets(line, sizeof(line), f)) eih_parse_line(line);
    fclose(f);
    proxy_logf("[input] loaded %d held entries from %s", g_eih_n, path);
}

/* Advance the held cursor to `flip` and set g_eih_held to its key set (a LEVEL,
 * once per flip — like the agent's heldAdvance). */
static void eih_advance(LONG flip)
{
    if (flip == g_eih_last_flip) return;
    g_eih_last_flip = flip;
    while (g_eih_i < g_eih_n && flip >= (LONG)g_eih_trace[g_eih_i].frame) {
        memset(g_eih_held, 0, sizeof g_eih_held);
        ei_entry *e = &g_eih_trace[g_eih_i];
        for (int j = 0; j < e->nids; j++) g_eih_held[e->ids[j] & 0xff] = 1;
        g_eih_i++;
    }
}

/* thiscall(scancode): ecx = the DInput device; [esp+4] = scancode.  Serves BOTH the
 * held-axis injection (write a fake state for managed keys) and INPUT RECORD (read the
 * real state of the queried tracked key) — exclusive in practice (record runs with no
 * injection trace). */
static void eih_keydown_cb(PCONTEXT ctx)
{
    DWORD sc = *(DWORD *)(ctx->Esp + 4) & 0xff;
    if (g_rec_file) {                          /* RECORD: read the real key state */
        eih_record_on_query(ctx->Ecx);         /* flip-boundary flush + device capture */
        if (*(BYTE *)(ctx->Ecx + EI_KBSTATE_OFF + sc) & 0x80) g_rec_cur[sc] = 1;
    }
    if (g_eih_n > 0) {                          /* INJECT: write the trace's held state */
        eih_advance(g_eh_flip);
        if (g_eih_managed[sc])
            *(BYTE *)(ctx->Ecx + EI_KBSTATE_OFF + sc) = g_eih_held[sc] ? 0x80 : 0x00;
    }
}

static void engine_input_install(void)
{
    ei_load_trace();
    eih_load_trace();
    eih_record_init();
    if (g_ei_n > 0) {
        detour_add(EI_INPUT_POLL_VA, ei_poll_cb);
        proxy_logf("[input] ring injection hooked @0x43c110 (%d entries)", g_ei_n);
    }
    /* Install the leaf-query detour for held-axis INJECTION or for RECORDING. */
    if (g_eih_n > 0 || g_rec_file) {
        detour_add(EI_KEYDOWN_VA, eih_keydown_cb);
        proxy_logf("[input] 0x5ba520 hooked (%s%s)",
                   g_eih_n > 0 ? "inject " : "", g_rec_file ? "record" : "");
    }
}

#endif /* OSS_ENGINE_INPUT_H */
