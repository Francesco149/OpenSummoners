// tools/sotes_trainer/trainer.c — standalone EN-SE trainer (injected DLL).
//
// In-process, no Frida.  Injected into sotes_en.exe (unpacked ImageBase 0x400000)
// via build/inject.exe (CreateProcess-SUSPENDED -> remote LoadLibrary -> Resume) or
// a version.dll proxy.  Hosts a localhost line-delimited JSON server (127.0.0.1:7777)
// that is BOTH the optional LLM interface and the stats-UI backend.  See DESIGN.md
// for the discovered mechanics + roadmap.
//
// Build: i686-w64-mingw32-gcc -shared -O2 -s -o build/sotes_trainer.dll
//        tools/sotes_trainer/trainer.c -lws2_32   (run inside nix develop)
//
// Protocol: one JSON object per line in, one per line out.  Examples:
//   {"cmd":"ping"}                          -> {"ok":true,"pong":true,"delta":...}
//   {"cmd":"player"}                        -> {"ok":true,"player":{...}}   (or null)
//   {"cmd":"read","addr":"0x92af80","type":"u32"}
//   {"cmd":"write","addr":"0x...","value":1,"type":"u32"}
//   {"cmd":"setstat","which":"hp","value":999,"lock":true}
//   {"cmd":"god","on":true}                 -> freeze hp+mp at max every tick
//   {"cmd":"teleport","x":50000,"y":40000}  -> write world_x/world_y (centi-px)

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

// ─── EN-SE addresses (unpacked ImageBase 0x400000) ──────────────────────────
#define ORIG_BASE     0x400000u
#define PLAYER_CODE   0xc35au        // Arche (entity code @ actor+0x1d4)
// actor-base offsets
#define OFF_CODE      0x1d4
#define OFF_HANDLE    0x1d8
#define OFF_WORLD_X   0xc76c         // centi-px (px*100)
#define OFF_WORLD_Y   0xc770
#define OFF_STATBLOCK 0x760          // actor -> stat_block ptr
// stat_block offsets
#define OFF_HP_CUR    0x54
#define OFF_HP_BASE   0x58
#define OFF_HP_EQUIP  0x84
#define OFF_HP_BUFF   0x9c
#define OFF_MP_CUR    0x5c
#define OFF_MP_BASE   0x60
#define OFF_MP_EQUIP  0x88
#define OFF_MP_BUFF   0xa0
#define OFF_LEVEL     0xe0

#define TRAINER_PORT  7777

static uintptr_t g_delta;            // actual base - ORIG_BASE (ASLR-safe)
static char      g_logpath[MAX_PATH];
#define AP(x) ((void *)((uintptr_t)(x) + g_delta))

static void vlog(const char *fmt, ...) {
    FILE *f = fopen(g_logpath, "a"); if (!f) return;
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f); fclose(f);
}

// ─── safe memory access (VirtualQuery-guarded) ──────────────────────────────
static int mem_readable(const void *p, size_t n) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof mbi) == 0) return 0;
    if (mbi.State != MEM_COMMIT) return 0;
    DWORD prot = mbi.Protect & 0xff;
    if (prot == PAGE_NOACCESS || (mbi.Protect & PAGE_GUARD)) return 0;
    uintptr_t end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    return (uintptr_t)p + n <= end;
}
static int mem_writable(const void *p, size_t n) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof mbi) == 0) return 0;
    if (mbi.State != MEM_COMMIT) return 0;
    DWORD w = mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY |
                             PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
    if (!w || (mbi.Protect & PAGE_GUARD)) return 0;
    uintptr_t end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    return (uintptr_t)p + n <= end;
}
static int rd32(const void *p, uint32_t *out) {
    if (!mem_readable(p, 4)) return 0;
    *out = *(volatile uint32_t *)p;
    return 1;
}

// ─── player-actor anchor (scan for the 0xc35a actor; cache + revalidate) ─────
static uintptr_t g_player;   // cached actor base (0 = unknown)

static int stat_max(uintptr_t sb, int base_off, int equip_off, int buff_off);
// Strong validation: code 0xc35a is common enough to collide, so cross-check that
// the whole stat block + world coords are mutually player-sane (rejects garbage
// false positives).  TODO(anchor): a ctor hook on 0x419b00 would be collision-proof.
static int actor_valid(uintptr_t base) {
    uint32_t code, sb, lvl, hpc, mpc, wx, wy;
    if (!rd32((void *)(base + OFF_CODE), &code) || code != PLAYER_CODE) return 0;
    if (!rd32((void *)(base + OFF_STATBLOCK), &sb) || !mem_readable((void *)sb, 0x200)) return 0;
    if (!rd32((void *)(sb + OFF_LEVEL), &lvl) || lvl < 1 || lvl > 99) return 0;
    int hpmax = stat_max(sb, OFF_HP_BASE, OFF_HP_EQUIP, OFF_HP_BUFF);
    int mpmax = stat_max(sb, OFF_MP_BASE, OFF_MP_EQUIP, OFF_MP_BUFF);
    if (hpmax < 1 || hpmax > 99999) return 0;
    if (mpmax < 0 || mpmax > 9999) return 0;
    if (!rd32((void *)(sb + OFF_HP_CUR), &hpc) || (int)hpc < 0 || (int)hpc > hpmax) return 0;
    if (!rd32((void *)(sb + OFF_MP_CUR), &mpc) || (int)mpc < 0 || (int)mpc > mpmax) return 0;
    if (!rd32((void *)(base + OFF_WORLD_X), &wx) || !rd32((void *)(base + OFF_WORLD_Y), &wy)) return 0;
    if (abs((int)wx) > 50000000 || abs((int)wy) > 50000000) return 0;   // centi-px sanity
    return 1;
}
static uintptr_t find_player(void) {
    if (g_player && actor_valid(g_player)) return g_player;
    g_player = 0;
    uint8_t *addr = 0;
    MEMORY_BASIC_INFORMATION mbi;
    while (VirtualQuery(addr, &mbi, sizeof mbi)) {
        uint8_t *next = (uint8_t *)mbi.BaseAddress + mbi.RegionSize;
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE &&
            (mbi.Protect & PAGE_READWRITE) && !(mbi.Protect & PAGE_GUARD) &&
            mbi.RegionSize <= 0x4000000) {
            uint32_t *p = (uint32_t *)mbi.BaseAddress;
            uint32_t *e = (uint32_t *)(next - 4);
            for (; p <= e; ++p) {
                if (*p != PLAYER_CODE) continue;
                uintptr_t base = (uintptr_t)p - OFF_CODE;
                if (actor_valid(base)) { g_player = base; return base; }
            }
        }
        if (next <= addr) break;
        addr = next;
    }
    return 0;
}

static int stat_max(uintptr_t sb, int base_off, int equip_off, int buff_off) {
    uint32_t b = 0, e = 0, f = 0;
    rd32((void *)(sb + base_off), &b);
    rd32((void *)(sb + equip_off), &e);
    rd32((void *)(sb + buff_off), &f);
    int m = (int)b + (int)e + (int)f;
    return m < 0 ? 0 : m;
}

// ─── freeze table (applied every tick) ──────────────────────────────────────
static volatile int g_god;                 // freeze hp+mp at max
#define MAX_LOCKS 16
static struct { int used; uintptr_t addr; uint32_t val; } g_locks[MAX_LOCKS];
static CRITICAL_SECTION g_lock_cs;

static void lock_set(uintptr_t addr, uint32_t val) {
    EnterCriticalSection(&g_lock_cs);
    int free = -1;
    for (int i = 0; i < MAX_LOCKS; ++i) {
        if (g_locks[i].used && g_locks[i].addr == addr) { g_locks[i].val = val; LeaveCriticalSection(&g_lock_cs); return; }
        if (!g_locks[i].used && free < 0) free = i;
    }
    if (free >= 0) { g_locks[free].used = 1; g_locks[free].addr = addr; g_locks[free].val = val; }
    LeaveCriticalSection(&g_lock_cs);
}
static void lock_clear_all(void) {
    EnterCriticalSection(&g_lock_cs);
    for (int i = 0; i < MAX_LOCKS; ++i) g_locks[i].used = 0;
    LeaveCriticalSection(&g_lock_cs);
}

static DWORD WINAPI tick_thread(void *unused) {
    (void)unused;
    for (;;) {
        Sleep(50);
        if (g_god) {
            uintptr_t base = find_player();
            if (base) {
                uint32_t sb = 0;
                if (rd32((void *)(base + OFF_STATBLOCK), &sb)) {
                    int hpmax = stat_max(sb, OFF_HP_BASE, OFF_HP_EQUIP, OFF_HP_BUFF);
                    int mpmax = stat_max(sb, OFF_MP_BASE, OFF_MP_EQUIP, OFF_MP_BUFF);
                    if (mem_writable((void *)(sb + OFF_HP_CUR), 4)) *(uint32_t *)(sb + OFF_HP_CUR) = hpmax;
                    if (mem_writable((void *)(sb + OFF_MP_CUR), 4)) *(uint32_t *)(sb + OFF_MP_CUR) = mpmax;
                }
            }
        }
        EnterCriticalSection(&g_lock_cs);
        for (int i = 0; i < MAX_LOCKS; ++i)
            if (g_locks[i].used && mem_writable((void *)g_locks[i].addr, 4))
                *(uint32_t *)g_locks[i].addr = g_locks[i].val;
        LeaveCriticalSection(&g_lock_cs);
    }
    return 0;
}

// ─── tiny JSON helpers (line protocol) ──────────────────────────────────────
// Extract a string value for "key". Returns len (0 if absent). Not a full parser.
static int json_str(const char *s, const char *key, char *out, int cap) {
    char pat[64]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(s, pat); if (!p) return 0;
    p = strchr(p + strlen(pat), ':'); if (!p) return 0;
    ++p; while (*p == ' ' || *p == '\t') ++p;
    if (*p != '"') return 0;
    ++p; int n = 0;
    while (*p && *p != '"' && n < cap - 1) out[n++] = *p++;
    out[n] = 0; return n;
}
// Extract a numeric value (dec or 0x hex) for "key". Returns 1 if found.
static int json_num(const char *s, const char *key, long long *out) {
    char pat[64]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(s, pat); if (!p) return 0;
    p = strchr(p + strlen(pat), ':'); if (!p) return 0;
    ++p; while (*p == ' ' || *p == '\t') ++p;
    if (*p == '"') ++p;                 // allow "0x.." quoted
    int base = 10;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) base = 16;
    *out = strtoll(p, NULL, base); return 1;
}
static int json_bool(const char *s, const char *key, int dflt) {
    char pat[64]; snprintf(pat, sizeof pat, "\"%s\"", key);
    const char *p = strstr(s, pat); if (!p) return dflt;
    p = strchr(p + strlen(pat), ':'); if (!p) return dflt;
    ++p; while (*p == ' ' || *p == '\t') ++p;
    return strncmp(p, "true", 4) == 0 || *p == '1';
}

static uint32_t read_typed(uintptr_t a, const char *ty) {
    if (!strcmp(ty, "u8"))  { uint32_t v = 0; if (mem_readable((void*)a,1)) v = *(uint8_t*)a;  return v; }
    if (!strcmp(ty, "u16")) { uint32_t v = 0; if (mem_readable((void*)a,2)) v = *(uint16_t*)a; return v; }
    uint32_t v = 0; rd32((void *)a, &v); return v;
}
static void write_typed(uintptr_t a, uint32_t v, const char *ty) {
    if (!strcmp(ty, "u8"))  { if (mem_writable((void*)a,1)) *(uint8_t*)a  = (uint8_t)v;  return; }
    if (!strcmp(ty, "u16")) { if (mem_writable((void*)a,2)) *(uint16_t*)a = (uint16_t)v; return; }
    if (mem_writable((void *)a, 4)) *(uint32_t *)a = v;
}

// Build the player JSON into out. Returns 0 if no player.
static int player_json(char *out, int cap) {
    uintptr_t base = find_player();
    if (!base) { snprintf(out, cap, "null"); return 0; }
    uint32_t wx=0, wy=0, sb=0, hp=0, mp=0, lvl=0;
    rd32((void *)(base + OFF_WORLD_X), &wx);
    rd32((void *)(base + OFF_WORLD_Y), &wy);
    rd32((void *)(base + OFF_STATBLOCK), &sb);
    rd32((void *)(sb + OFF_HP_CUR), &hp);
    rd32((void *)(sb + OFF_MP_CUR), &mp);
    rd32((void *)(sb + OFF_LEVEL), &lvl);
    int hpmax = stat_max(sb, OFF_HP_BASE, OFF_HP_EQUIP, OFF_HP_BUFF);
    int mpmax = stat_max(sb, OFF_MP_BASE, OFF_MP_EQUIP, OFF_MP_BUFF);
    snprintf(out, cap,
        "{\"actor\":\"0x%08x\",\"world_x\":%d,\"world_y\":%d,\"stat_block\":\"0x%08x\","
        "\"hp\":%d,\"hp_max\":%d,\"mp\":%d,\"mp_max\":%d,\"level\":%d}",
        (unsigned)base, (int)wx, (int)wy, (unsigned)sb,
        (int)hp, hpmax, (int)mp, mpmax, (int)lvl);
    return 1;
}

// ─── command dispatch ───────────────────────────────────────────────────────
static void handle_line(const char *line, char *out, int cap) {
    char cmd[32] = {0};
    if (!json_str(line, "cmd", cmd, sizeof cmd)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no cmd\"}"); return; }

    if (!strcmp(cmd, "ping")) {
        snprintf(out, cap, "{\"ok\":true,\"pong\":true,\"delta\":\"0x%x\",\"base\":\"0x%x\"}",
                 (unsigned)g_delta, (unsigned)(ORIG_BASE + g_delta));
        return;
    }
    if (!strcmp(cmd, "player")) {
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":true,\"player\":%s}", pj);
        return;
    }
    if (!strcmp(cmd, "read")) {
        long long a; char ty[8] = "u32";
        if (!json_num(line, "addr", &a)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no addr\"}"); return; }
        json_str(line, "type", ty, sizeof ty);
        int reloc = json_bool(line, "va", 0);           // va:true -> AP() relocation
        uintptr_t addr = reloc ? (uintptr_t)AP(a) : (uintptr_t)a;
        snprintf(out, cap, "{\"ok\":true,\"value\":%u,\"addr\":\"0x%08x\"}",
                 read_typed(addr, ty), (unsigned)addr);
        return;
    }
    if (!strcmp(cmd, "write")) {
        long long a, v; char ty[8] = "u32";
        if (!json_num(line, "addr", &a) || !json_num(line, "value", &v)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"addr/value\"}"); return; }
        json_str(line, "type", ty, sizeof ty);
        int reloc = json_bool(line, "va", 0);
        uintptr_t addr = reloc ? (uintptr_t)AP(a) : (uintptr_t)a;
        write_typed(addr, (uint32_t)v, ty);
        snprintf(out, cap, "{\"ok\":true,\"wrote\":%u,\"addr\":\"0x%08x\"}", (uint32_t)v, (unsigned)addr);
        return;
    }
    if (!strcmp(cmd, "god")) {
        g_god = json_bool(line, "on", 1);
        snprintf(out, cap, "{\"ok\":true,\"god\":%s}", g_god ? "true" : "false");
        return;
    }
    if (!strcmp(cmd, "setstat")) {
        char which[8] = {0}; long long v;
        if (!json_str(line, "which", which, sizeof which) || !json_num(line, "value", &v)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"which/value\"}"); return; }
        uintptr_t base = find_player(); uint32_t sb = 0;
        if (!base || !rd32((void *)(base + OFF_STATBLOCK), &sb)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no player\"}"); return; }
        int off = -1;
        if (!strcmp(which, "hp")) off = OFF_HP_CUR; else if (!strcmp(which, "hp_max")) off = OFF_HP_BASE;
        else if (!strcmp(which, "mp")) off = OFF_MP_CUR; else if (!strcmp(which, "mp_max")) off = OFF_MP_BASE;
        else if (!strcmp(which, "level")) off = OFF_LEVEL;
        if (off < 0) { snprintf(out, cap, "{\"ok\":false,\"error\":\"bad which\"}"); return; }
        write_typed(sb + off, (uint32_t)v, "u32");
        if (json_bool(line, "lock", 0)) lock_set(sb + off, (uint32_t)v);
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":true,\"player\":%s}", pj);
        return;
    }
    if (!strcmp(cmd, "teleport")) {
        long long x, y; int rel = json_bool(line, "relative", 0);
        uintptr_t base = find_player();
        if (!base) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no player\"}"); return; }
        if (json_num(line, "x", &x)) {
            uint32_t cur = 0; rd32((void *)(base + OFF_WORLD_X), &cur);
            write_typed(base + OFF_WORLD_X, rel ? cur + (uint32_t)(x*100) : (uint32_t)x, "u32");
        }
        if (json_num(line, "y", &y)) {
            uint32_t cur = 0; rd32((void *)(base + OFF_WORLD_Y), &cur);
            write_typed(base + OFF_WORLD_Y, rel ? cur + (uint32_t)(y*100) : (uint32_t)y, "u32");
        }
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":true,\"player\":%s}", pj);
        return;
    }
    if (!strcmp(cmd, "unlock_all")) { lock_clear_all(); g_god = 0; snprintf(out, cap, "{\"ok\":true}"); return; }

    snprintf(out, cap, "{\"ok\":false,\"error\":\"unknown cmd\"}");
}

// ─── localhost line-JSON server ─────────────────────────────────────────────
static DWORD WINAPI server_thread(void *unused) {
    (void)unused;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { vlog("[srv] WSAStartup failed"); return 0; }
    SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ls == INVALID_SOCKET) { vlog("[srv] socket failed"); return 0; }
    BOOL yes = TRUE; setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof yes);
    struct sockaddr_in sa; memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; sa.sin_port = htons(TRAINER_PORT);
    // Bind all interfaces: the dev/LLM driver runs in WSL2 and reaches the Windows
    // host over the LAN (cutestation.soy), which loopback would block.  A SHIPPED
    // build should default to INADDR_LOOPBACK (UI is same-host) + make this a config.
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(ls, (struct sockaddr *)&sa, sizeof sa) != 0) { vlog("[srv] bind %d failed", TRAINER_PORT); closesocket(ls); return 0; }
    if (listen(ls, 4) != 0) { vlog("[srv] listen failed"); closesocket(ls); return 0; }
    vlog("[srv] listening on 127.0.0.1:%d", TRAINER_PORT);

    for (;;) {
        SOCKET cs = accept(ls, NULL, NULL);
        if (cs == INVALID_SOCKET) { Sleep(50); continue; }
        vlog("[srv] client connected");
        char buf[4096]; int used = 0;
        for (;;) {
            int n = recv(cs, buf + used, (int)sizeof buf - 1 - used, 0);
            if (n <= 0) break;
            used += n; buf[used] = 0;
            char *nl;
            while ((nl = memchr(buf, '\n', used)) != NULL) {
                *nl = 0;
                char resp[1024];
                handle_line(buf, resp, sizeof resp);
                int rl = (int)strlen(resp); resp[rl++] = '\n';
                send(cs, resp, rl, 0);
                int rest = used - (int)(nl + 1 - buf);
                memmove(buf, nl + 1, rest); used = rest; buf[used] = 0;
            }
            if (used >= (int)sizeof buf - 1) used = 0;   // overlong line: drop
        }
        closesocket(cs);
        vlog("[srv] client disconnected");
    }
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) {
    (void)r;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        g_delta = (uintptr_t)GetModuleHandleA(NULL) - ORIG_BASE;
        GetModuleFileNameA(NULL, g_logpath, MAX_PATH);
        char *slash = strrchr(g_logpath, '\\');
        strcpy(slash ? slash + 1 : g_logpath, "oss_trainer.log");
        InitializeCriticalSection(&g_lock_cs);
        vlog("[trainer] attach: base=%p delta=%p port=%d",
             GetModuleHandleA(NULL), (void *)g_delta, TRAINER_PORT);
        CreateThread(NULL, 0, tick_thread, NULL, 0, NULL);
        CreateThread(NULL, 0, server_thread, NULL, 0, NULL);
    }
    return TRUE;
}
