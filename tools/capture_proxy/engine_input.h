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

static void engine_input_install(void)
{
    ei_load_trace();
    if (g_ei_n == 0) return;                    /* nothing to inject */
    detour_add(EI_INPUT_POLL_VA, ei_poll_cb);
    proxy_logf("[input] ring injection hooked @0x43c110 (%d entries)", g_ei_n);
}

#endif /* OSS_ENGINE_INPUT_H */
