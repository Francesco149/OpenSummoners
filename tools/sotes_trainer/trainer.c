// tools/sotes_trainer/trainer.c — standalone EN-SE trainer (injected DLL).
//
// In-process, no Frida.  Injected into sotes_en.exe (unpacked ImageBase 0x400000)
// via build/inject.exe (CreateProcess-SUSPENDED -> remote LoadLibrary -> Resume) or
// a version.dll proxy.  Hosts a localhost line-delimited JSON server (127.0.0.1:7777)
// that is BOTH the optional LLM interface and the stats-UI backend.  See DESIGN.md
// for the discovered mechanics + roadmap.
//
// Build: nix develop --command make -C tools/sotes_trainer  (-> build/sotes_trainer.dll;
//        links the standalone .sdt reader tools/sotes_save/).  See README.md.
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
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include "sotes_save.h"      // standalone .sdt reader (tools/sotes_save/)

// ─── EN-SE addresses (unpacked ImageBase 0x400000) ──────────────────────────
#define ORIG_BASE     0x400000u
#define PLAYER_CODE   0xc35au        // Arche (entity code @ actor+0x1d4)
// actor-base offsets
#define OFF_CODE      0x1d4
#define OFF_HANDLE    0x1d8
// world_x/world_y are a DERIVED per-frame SNAPSHOT (re-committed from the phys-box
// every frame, so writing them is stomped in ~1 frame — the old teleport bug).  The
// AUTHORITATIVE position lives in the collision AABB the actor points to at +0x40.
// RE'd live off the position-commit at VA 0x484554 (HW write-watch on world_x):
//   world_x = box[+0x04]              (direct copy)   -> writing box[+4] STICKS
//   world_y = box[+0x08] + box[+0x10] - 1             (top + height - 1 = bottom)
#define OFF_WORLD_X   0xc76c         // centi-px (px*100) — DERIVED snapshot, read-only
#define OFF_WORLD_Y   0xc770         //                     DERIVED snapshot, read-only
#define OFF_BOX       0x40           // actor -> collision AABB ptr (authoritative pos)
#define BOX_X         0x04           // left  X  (= world_x)                 centi-px
#define BOX_TOP       0x08           // top   Y                              centi-px
#define BOX_W         0x0c           // width                               centi-px
#define BOX_H         0x10           // height (world_y = top + height - 1)  centi-px
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
#define OFF_LEVEL     0xe0           // level_base (NOT the display level — see DESIGN
                                     // "Live findings": display Lv is EXP-derived)
#define OFF_EXP_CUR   0xec           // EXP progress   (SE offset; base-game +0xe8)
#define OFF_EXP_MAX   0xf0           // EXP to next    (cross-ref: 50000 = Arche Lv17)

#define TRAINER_PORT  7777

// ─── save-load chain + safepoint (EN-SE VAs, RE'd — see DESIGN.md session 3) ──
// The 3 load calls touch file IO / the game-state singleton / DirectDraw scene
// teardown, so they MUST run on the ENGINE thread at a per-frame safepoint, NOT the
// socket thread.  We hook the inputPoll (0x437c70, called constantly at the title) and
// drain a queued load there; and hook the title dispatcher (0x581ba0) to capture its
// `this` (= the scene-transition `this` the dispatcher passes to 0x5cb460).
#define VA_INPUTPOLL   0x437c70      // title/gameplay input poll = per-frame safepoint (5 bytes)
#define VA_MENUCTRL    0x4378d0      // generic-menu controller (the save-slot PICKER); 5 bytes
#define VA_DISPATCH    0x581ba0      // title dispatcher (ref only; not hooked)
#define BTN_CONFIRM    0x25          // confirm button id (37)
#define BTN_ROTATE     0x04          // menu rotate/next button id
#define VA_ALLOC       0x5ef121      // cdecl  alloc(size)              -> raw buffer
#define VA_SAVE_LOAD   0x416550      // thiscall(this=S, handle, slot, 0, 0) reads savedataNN.sdt
#define VA_SAVE_APPLY  0x586c60      // thiscall(this=singleton, handle, sel) apply into game state
#define VA_SCENE_TRANS 0x5cb460      // thiscall(this=dispatch, tgt, x, y) deferred scene transition
#define VA_GS_SINGLE   0x92ac68      // game-state singleton (apply's `this`, a static object)
#define VA_SCENE_THIS  0x92dd4c      // global holding the title scene handler `this` (0x5cb460's
                                     // `this`): the caller 0x58113e does `mov [0x92dd4c], ecx`
                                     // right before `call 0x581ba0` — read it, no hook needed.
#define VA_DEMO_JL     0x583866      // attract idle->demo trigger: jl 0x5832e1 -> patch to jmp
#define SAVE_HANDLE_MAIN 0x2738      // Main-Quest save category (loader .sdt branch + apply gate)
#define SAVE_STRUCT_SZ   0xea94      // FUN_00585cf0 allocs this for the transient save struct

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

// ─── engine-function caller ─────────────────────────────────────────────────
// Call fn(args[0..n-1]) with ecx=ecx_this (harmless for cdecl/stdcall — ecx is
// caller-saved; only thiscall reads it).  esp is saved before and restored after
// the call, so ANY calling convention (cdecl/stdcall/thiscall) is safe — a
// stdcall/thiscall `ret N` self-cleanup and a cdecl caller-cleanup both reduce to
// "esp := saved".  O0 keeps ebp a frame pointer so the ebp-relative "m" operands
// stay valid across the arg pushes.  Power primitive: unblocks the load chain.
static uint32_t __attribute__((noinline, optimize("O0")))
call_va(void *fn, uint32_t ecx_this, const uint32_t *args, int n) {
    volatile uint32_t ret = 0;
    __asm__ __volatile__(
        "movl %%esp, %%ebx        \n\t"   // save esp
        "movl %[n], %%edx         \n\t"   // edx = n (index, counts down)
        "1: testl %%edx, %%edx    \n\t"
        "   jz 2f                 \n\t"
        "   decl %%edx            \n\t"
        "   movl %[args], %%eax   \n\t"   // reload args base (ebp-relative, safe)
        "   pushl (%%eax,%%edx,4) \n\t"   // push args[edx]  (right-to-left)
        "   jmp 1b                \n\t"
        "2: movl %[ecx], %%ecx    \n\t"   // ecx = this
        "   call *%[fn]           \n\t"
        "   movl %%ebx, %%esp     \n\t"   // restore esp (convention-agnostic cleanup)
        "   movl %%eax, %[ret]    \n\t"
        : [ret] "=m" (ret)
        : [fn] "m" (fn), [ecx] "m" (ecx_this), [args] "m" (args), [n] "m" (n)
        : "eax", "ebx", "ecx", "edx", "cc", "memory");
    return ret;
}

// Build the player JSON into out. Returns 0 if no player.
static int player_json(char *out, int cap) {
    uintptr_t base = find_player();
    if (!base) { snprintf(out, cap, "null"); return 0; }
    uint32_t wx=0, wy=0, sb=0, hp=0, mp=0, lvl=0, ec=0, em=0;
    rd32((void *)(base + OFF_WORLD_X), &wx);
    rd32((void *)(base + OFF_WORLD_Y), &wy);
    rd32((void *)(base + OFF_STATBLOCK), &sb);
    rd32((void *)(sb + OFF_HP_CUR), &hp);
    rd32((void *)(sb + OFF_MP_CUR), &mp);
    rd32((void *)(sb + OFF_LEVEL), &lvl);
    rd32((void *)(sb + OFF_EXP_CUR), &ec);
    rd32((void *)(sb + OFF_EXP_MAX), &em);
    int hpmax = stat_max(sb, OFF_HP_BASE, OFF_HP_EQUIP, OFF_HP_BUFF);
    int mpmax = stat_max(sb, OFF_MP_BASE, OFF_MP_EQUIP, OFF_MP_BUFF);
    // level_base (0xe0) is NOT the display level (that is EXP-derived — DESIGN);
    // exp_cur/exp_max are exposed so the true level can be looked up from a table.
    snprintf(out, cap,
        "{\"actor\":\"0x%08x\",\"world_x\":%d,\"world_y\":%d,\"stat_block\":\"0x%08x\","
        "\"hp\":%d,\"hp_max\":%d,\"mp\":%d,\"mp_max\":%d,\"level_base\":%d,"
        "\"exp_cur\":%d,\"exp_max\":%d}",
        (unsigned)base, (int)wx, (int)wy, (unsigned)sb,
        (int)hp, hpmax, (int)mp, mpmax, (int)lvl, (int)ec, (int)em);
    return 1;
}

// ─── engine-thread safepoint + save-load chain ──────────────────────────────
// The load calls run from a hook on the inputPoll (a per-frame engine-thread
// safepoint), never the socket thread.  0x581ba0's `this` (the scene object the
// dispatcher hands 0x5cb460) is captured by a second hook.
static volatile uint32_t g_dispatch_this;   // scene `this` read from 0x92dd4c at load time
static volatile int      g_load_pending, g_load_slot, g_load_enter, g_in_safepoint;
static volatile uint32_t g_load_ret;
static char              g_load_log[192];

// ─── menu-drive (the VERIFIED load path): inject input records into the game's own
// menu managers.  Title polls 0x437c70, the save-slot picker polls 0x4378d0; each
// trampoline captures its poll esp+ecx(=manager) so the callback can time the record
// to the poll's own `now` (match window 0x64) and target the live ring.  See DESIGN.md.
static volatile uint32_t g_ti_esp, g_ti_mgr;   // title-poll  (0x437c70) captured esp / manager
static volatile uint32_t g_pk_esp, g_pk_mgr;   // picker-poll (0x4378d0) captured esp / manager
static volatile int      g_md_state;           // 0 idle, 1 want title-confirm, 2 want picker-confirm
static volatile int      g_md_downs;           // picker rotations before confirm (0 = default slot)
static volatile int      g_md_slot = -1;       // target save slot (-1 = default/newest highlight)
static volatile int      g_dlgskip = 1;        // auto-skip dialogue (DEFAULT ON)
// dlgskip auto-advances an OPEN dialogue box hands-free.  It gates on a PASSIVE read of the SE
// dialogue widget (dialogue_box_open below) — NOT the old active 0x24-probe consumption gate.
// WHY the rewrite (session 6): the old gate injected 0x24/0x27 EVERY gameplay poll to *detect*
// a dialogue (watch the probe get consumed).  That active probing ran in freeroam too and — with
// a 1-poll detection lag — leaked a world input on close, AUTO-TRIGGERING the house-exit door
// (the "Dad needs help" gate fires on UP-into-the-door; it does not normally auto-open).  The fix
// per USER: DISTINGUISH an already-open box by READING the widget, and only inject while it is up.
// So in freeroam we now inject NOTHING (no probe) → the door is never auto-triggered.
// The widget lives on the input manager (g_ti_mgr): DLG_BOX_CELL0 (+0x374) = the first content-cell
// ptr (0 ⇒ no box on screen), DLG_BOX_SCALE (+0x3a4) = the pop-in scale (0..1000, 1000 = fully
// open).  The box object itself = *(mgr+0x374) with the layout +0x4 active / +0x54 scale.  RE'd
// live (session 6): freeroam↔dialogue differential off *(0x92dd4c)/the input mgr; box-frame /
// body-text color constants (0x3e537d/0xa8b9cc) located the SE dialogue code in
// vendor/unpacked/editions/sotes-ense-en.exe.  See DESIGN.md "Dialogue-skip — passive gate".
#define DLG_BOX_CELL0   0x374   // input-mgr -> dialogue widget: first content-cell ptr (0 = closed)
#define DLG_BOX_SCALE   0x3a4   // input-mgr -> dialogue widget: pop-in scale (0..1000)
#define DLG_SETTLE_POLLS 120    // hold dlgskip off ~2s after a menu-drive (scene-settle debounce)
static volatile int      g_dlg_btns[6] = { 0, 0, 0, 0, 0, 0 };  // extra reveal-skip ids (0=unused;
                                 // default EMPTY — the reveal-skip id doubles as world UP, so it
                                 // must stay off until the reveal is done via a UI-STATE write,
                                 // per USER "force UI state, not input").  PORT-DEBT(dlgskip-reveal-ui).
static volatile int      g_ng_state;            // new-game drive: 0 idle, 1 rotate-to-Start, 2 confirm
static volatile int      g_ng_rotates;          // title rotations to reach the New Game item
static volatile int      g_ng_btn = 0x04;       // title rotate button (2=up / 4=down)
static volatile int      g_ng_gap, g_ng_cool;   // rotate cadence: 1 rotate every g_ng_gap polls

// Save-slot picker selection (getters FUN_005e8e80/FUN_005e8ea0): the save-list MANAGER
// holds the slot list at *(mgr+0x17c) — 0x10-byte entries { handle@+0, slot@+4 } (slot =
// the savedataNN index) — and the selected index at *( *(mgr+0x174) + 0x14 ).  The MANAGER
// is NOT the 0x4378d0 controller ecx; it is the controller's arg1 (VERIFIED live: poll
// esp+4).  We probe ecx + stack args 1..5 for the object whose +0x17c list's first entry
// has a small savedata slot id (garbage candidates are skipped), then write the index of
// the entry with slot==target.
#define PK_SEL_MODEL  0x174   // manager -> selection-model ptr (+0x14 = selected index)
#define PK_LIST_BASE  0x17c   // manager -> slot-list base ptr  (entries {handle@0, slot@4})
static char         g_pk_diag[128];   // concise pick result, surfaced in the `load` response
static volatile int g_pk_diag_done;
// Point the picker at savedataNN==`target`; returns the chosen list index, or -1 (no write)
// if not found — the caller then loads the default (newest) highlight.  Only writes after a
// matching in-bounds entry in a validated list, so a wrong candidate is a safe no-op.
static int picker_select_slot(uint32_t mgr, uint32_t esp, int target) {
    uint32_t cand[8]; int nc = 0;
    cand[nc++] = mgr;                                              // the controller ecx
    if (esp) for (int a = 1; a <= 5; ++a) { uint32_t v = 0;       // + its stack args 1..5
        if (rd32((void *)(uintptr_t)(esp + (uint32_t)a * 4), &v)) cand[nc++] = v; }
    for (int c = 0; c < nc; ++c) {
        uint32_t sm = 0, lb = 0, s0 = 0;
        if (!rd32((void *)(uintptr_t)(cand[c] + PK_SEL_MODEL), &sm) || !sm) continue;
        if (!rd32((void *)(uintptr_t)(cand[c] + PK_LIST_BASE), &lb) || !lb) continue;
        if (!rd32((void *)(uintptr_t)(lb + 4), &s0) || s0 > 15) continue;   // not the manager
        for (int i = 0; i < 16; ++i) {                            // validated list: find target
            uint32_t sf = 0;
            if (!rd32((void *)(uintptr_t)(lb + (uint32_t)i * 0x10 + 4), &sf)) break;
            if ((int)sf == target) {
                if (mem_writable((void *)(uintptr_t)(sm + 0x14), 4))
                    *(volatile uint32_t *)(uintptr_t)(sm + 0x14) = (uint32_t)i;
                if (!g_pk_diag_done) { snprintf(g_pk_diag, sizeof g_pk_diag, "slot %d -> list index %d", target, i); g_pk_diag_done = 1; }
                return i;
            }
        }
    }
    if (!g_pk_diag_done) { snprintf(g_pk_diag, sizeof g_pk_diag, "slot %d not found in picker (loaded default)", target); g_pk_diag_done = 1; }
    return -1;
}

#define IN_POOL 64
static uint8_t           g_inpool[IN_POOL * 12];
static volatile unsigned g_inpool_idx;
// Write one input record {+0=id,+4=ts,+8=1} into a trainer-owned pool and publish its
// pointer into the manager ring at mgr[0x0c + slot*4] (slot 63 = first polled).  The
// game's poll (0x437c70/0x4378d0) matches +0==polled_id && +8==1 && now-ts<=0x64.
static uint8_t *inject_record(uint32_t mgr, uint32_t id, uint32_t now, int ordinal) {
    if (!mgr || !mem_writable((void *)(uintptr_t)mgr, 0x10c)) return NULL;
    uint8_t *rec = g_inpool + (g_inpool_idx++ % IN_POOL) * 12;
    *(volatile uint32_t *)(rec + 0) = id;
    *(volatile uint32_t *)(rec + 4) = now;
    *(volatile uint32_t *)(rec + 8) = 1;
    int slot = 63 - (ordinal & 63);
    *(volatile uint32_t *)(uintptr_t)(mgr + 0x0c + slot * 4) = (uint32_t)(uintptr_t)rec;
    return rec;   // caller may watch rec[+0]: a poll clears it to 0 on consume (dialogue-active probe)
}

// PASSIVE dialogue-open test: read the SE dialogue widget (embedded in the input mgr) and report
// whether an IN-GAME dialogue box is on screen.  No injection — the non-lagged replacement for the
// old 0x24-probe consumption gate (see the g_dlgskip comment).  Two conditions:
//   (1) cell0 (+0x374) != 0 — the widget's first content-cell ptr (0 in freeroam);
//   (2) scale (+0x3a4) in [1,1000] — the pop-in scale of a real box.
// (2) is what rejects the TITLE: there g_ti_mgr is a DIFFERENT object (the title/menu manager,
// not the in-game input mgr that owns the dialogue widget), where +0x374 holds a menu ptr and
// +0x3a4 aliases a heap pointer (huge, ≫1000).  Freeroam has +0x374==0 && scale==0; only an
// actual dialogue box has both cell0!=0 AND a sane 1..1000 scale.  (Verified live: title
// +0x3a4=0x49600c0, freeroam=0, open=1000.)
static int dialogue_box_open(uint32_t mgr) {
    uint32_t cell0 = 0, scale = 0;
    if (!mgr) return 0;
    if (!rd32((void *)(uintptr_t)(mgr + DLG_BOX_CELL0), &cell0) || !cell0) return 0;
    if (!rd32((void *)(uintptr_t)(mgr + DLG_BOX_SCALE), &scale)) return 0;
    return scale >= 1 && scale <= 1000;
}

// Runs on the ENGINE thread (from the inputPoll safepoint).  Reproduces the retail
// Continue-confirm terminal: alloc a save struct -> load savedataNN.sdt -> apply into
// the game-state singleton -> deferred scene transition.  Every step is logged so a
// failing step is identifiable from the `load` response.
static void do_load_at_safepoint(int slot) {
    void *fn_alloc = AP(VA_ALLOC), *fn_load = AP(VA_SAVE_LOAD),
         *fn_apply = AP(VA_SAVE_APPLY), *fn_trans = AP(VA_SCENE_TRANS);
    if (!mem_readable(fn_alloc, 1) || !mem_readable(fn_load, 1)) {
        snprintf(g_load_log, sizeof g_load_log, "engine fns unreadable"); return; }
    uint32_t sz = SAVE_STRUCT_SZ;
    uint32_t S = call_va(fn_alloc, 0, &sz, 1);                 // cdecl alloc(0xea94)
    if (!S || !mem_writable((void *)(uintptr_t)S, SAVE_STRUCT_SZ)) {
        snprintf(g_load_log, sizeof g_load_log, "alloc(0x%x) failed S=0x%x", SAVE_STRUCT_SZ, S); return; }
    memset((void *)(uintptr_t)S, 0, SAVE_STRUCT_SZ);
    uint32_t la[4] = { SAVE_HANDLE_MAIN, (uint32_t)slot, 0, 0 };
    uint32_t r = call_va(fn_load, S, la, 4);                   // thiscall load -> savedataNN.sdt
    g_load_ret = r;
    if (!r) { snprintf(g_load_log, sizeof g_load_log,
        "416550 ret 0 (savedata%02d.sdt missing/invalid); S=0x%x", slot, S); return; }
    uint32_t aa[2] = { SAVE_HANDLE_MAIN, (uint32_t)slot };
    call_va(fn_apply, (uint32_t)(uintptr_t)AP(VA_GS_SINGLE), aa, 2);   // apply into the singleton
    uint32_t dt = 0; rd32(AP(VA_SCENE_THIS), &dt); g_dispatch_this = dt;
    if (!g_load_enter) {
        snprintf(g_load_log, sizeof g_load_log,
            "slot=%d loaded+applied ret=%u S=0x%x this=0x%x (enter=0: transition skipped)",
            slot, r, S, dt);
        return;
    }
    // EXPERIMENTAL: the raw 0x5cb460 transition still crashes — likely missing the
    // pre-transition setup 0x585cf0 does (validate 0x57f020 / target scene build) or a
    // wrong apply `sel`.  Args match the Start path 5cb460(this,0x2724,0,0); needs the
    // observed real Continue-load args to finish.  See DESIGN.md session 3.
    if (dt && mem_readable((void *)(uintptr_t)dt, 4) && mem_readable(fn_trans, 1)) {
        uint32_t ta[3] = { SAVE_HANDLE_MAIN, 0, 0 };
        call_va(fn_trans, dt, ta, 3);                         // deferred scene transition
        snprintf(g_load_log, sizeof g_load_log,
            "OK slot=%d ret=%u S=0x%x this=0x%x -> transition attempted (EXPERIMENTAL)", slot, r, S, dt);
    } else {
        snprintf(g_load_log, sizeof g_load_log,
            "slot=%d loaded+applied ret=%u S=0x%x but dispatch_this=0x%x invalid - no transition",
            slot, r, S, dt);
    }
}

// Title-poll (0x437c70) callback — runs on the engine thread each poll.  Drains the
// experimental direct-call load, and injects the title confirm for the menu-drive.
static void __attribute__((cdecl)) poll_title_cb(void) {
    if (g_load_pending && !g_in_safepoint) {          // experimental direct chain (loadraw)
        g_in_safepoint = 1; int slot = g_load_slot; g_load_pending = 0;
        do_load_at_safepoint(slot); g_in_safepoint = 0;
    }
    uint32_t now = 0, e = g_ti_esp;
    if (e) rd32((void *)(uintptr_t)(e + 4), &now);        // 0x437c70 arg1 = now
    if (g_md_state == 1 && g_ti_mgr) {                // menu-drive step 1: confirm at the title
        // Re-inject the title confirm EVERY poll until the save-slot picker opens (its poll
        // 0x4378d0 sets g_pk_mgr; the title only polls 0x437c70, so g_pk_mgr==0 until then).
        // A single title confirm is sometimes dropped before the title menu settles — the
        // old one-shot advance-to-2 then hung with the picker never open.
        if (g_pk_mgr) { g_md_state = 2; return; }     // picker up -> picker-confirm step
        inject_record(g_ti_mgr, BTN_CONFIRM, now, 0);
    }
    if (g_ng_state && g_ti_mgr) {                     // new-game drive: rotate to Start, then confirm
        if (g_ng_state == 1) {
            if (g_ng_rotates > 0) {                   // one rotate every g_ng_gap polls (menu settle)
                if (g_ng_cool > 0) g_ng_cool--;
                else { inject_record(g_ti_mgr, (uint32_t)g_ng_btn, now, 3); g_ng_rotates--; g_ng_cool = g_ng_gap; }
            } else g_ng_state = 2;
        } else {                                      // step 2: confirm (re-inject until scene leaves title)
            inject_record(g_ti_mgr, BTN_CONFIRM, now, 3);
        }
    }
    // Scene-settle debounce: a freshly-loaded scene (right after a newgame/load menu-drive) is
    // briefly UNSTABLE — touching the dialogue widget the instant the drive ends faults the game.
    // (PRE-EXISTING: dlgskip-on through `newgame` crashed BOTH this build and the pre-passive-gate
    // one — the transition window, not the gate logic; proven by rebuilding e92abf9.)  Hold dlgskip
    // off for a beat after any drive so the scene settles first.
    static int g_dlg_settle = 0;
    if (g_md_state || g_ng_state) g_dlg_settle = 0;
    else if (g_dlg_settle < 100000)  g_dlg_settle++;
    // Auto-skip dialogue (suppressed while WE drive a menu / until the scene settles).  GATE: only
    // when a box is actually on screen (dialogue_box_open, a passive read of the widget) — so in
    // freeroam we inject NOTHING and can't trigger a world interaction (the door).  When a box IS up,
    // inject the advance ids 0x24/0x27 (dialogue-only, inert in the world) so each box steps hands-
    // free; the reveal-skip ids (g_dlg_btns, default EMPTY) also fire — but ONLY while the box is up,
    // and because the read is non-lagged they stop the instant it closes (no leak on close).
    if (g_dlgskip && g_ti_mgr && !g_md_state && !g_ng_state && g_dlg_settle > DLG_SETTLE_POLLS
        && dialogue_box_open(g_ti_mgr)) {
        inject_record(g_ti_mgr, 0x24, now, 4);                     // advance (dialogue-only)
        inject_record(g_ti_mgr, 0x27, now, 5);                     // advance (dialogue-only)
        for (int i = 0; i < 6; ++i)
            if (g_dlg_btns[i]) inject_record(g_ti_mgr, (uint32_t)g_dlg_btns[i], now, 6 + i);
    }
}

// Picker-poll (0x4378d0) callback — fires only while the save-slot picker is up.  If a
// target slot is set, point the picker's selection at it each poll (idempotent) BEFORE
// confirming; else apply any manual rotations first.  Then keep re-injecting confirm
// every poll until the load fires and the picker goes away (a single early confirm is
// dropped before the cursor settles; the load command ends the drive once find_player()).
static void __attribute__((cdecl)) poll_picker_cb(void) {
    if (g_md_state != 2 || !g_pk_mgr) return;         // menu-drive step 2: navigate + confirm
    uint32_t now = 0, e = g_pk_esp;
    if (e) rd32((void *)(uintptr_t)(e + 8), &now);        // 0x4378d0 arg2 = now
    if (g_md_slot >= 0) picker_select_slot(g_pk_mgr, g_pk_esp, g_md_slot);   // target a specific slot
    else if (g_md_downs > 0) { inject_record(g_pk_mgr, BTN_ROTATE, now, 0); g_md_downs--; return; }
    inject_record(g_pk_mgr, BTN_CONFIRM, now, 0);
}

// ─── minimal inline hooks (runtime-codegen trampolines) ──────────────────────
static uint8_t *g_tramp_p;

static void patch_bytes(void *dst, const void *src, size_t n) {
    DWORD old = 0;
    if (!VirtualProtect(dst, n, PAGE_EXECUTE_READWRITE, &old)) return;
    memcpy(dst, src, n);
    VirtualProtect(dst, n, old, &old);
    FlushInstructionCache(GetCurrentProcess(), dst, n);
}

// Detour `target` (overwriting `savelen` whole-instruction bytes, >=5) to a trampoline
// that: [if cb] pushad/pushfd, 16-align esp, call cb, restore; [if precode] emit it;
// run the saved original bytes; jmp back to target+savelen.  The saved bytes must be
// position-independent (verified: 0x437c70/0x581ba0 prologues are).
static void install_detour(uintptr_t target, int savelen, void *cb,
                           const uint8_t *precb, int precblen) {
    uint8_t *t = g_tramp_p;
    if (precb && precblen) { memcpy(g_tramp_p, precb, (size_t)precblen); g_tramp_p += precblen; }
    if (cb) {
        *g_tramp_p++ = 0x60;                                   // pushad
        *g_tramp_p++ = 0x9c;                                   // pushfd
        *g_tramp_p++ = 0x89; *g_tramp_p++ = 0xe5;              // mov ebp,esp
        *g_tramp_p++ = 0x83; *g_tramp_p++ = 0xe4; *g_tramp_p++ = 0xf0;  // and esp,-16
        *g_tramp_p++ = 0xe8;                                   // call rel32 -> cb
        int32_t rel = (int32_t)((uintptr_t)cb - ((uintptr_t)g_tramp_p + 4));
        memcpy(g_tramp_p, &rel, 4); g_tramp_p += 4;
        *g_tramp_p++ = 0x89; *g_tramp_p++ = 0xec;              // mov esp,ebp
        *g_tramp_p++ = 0x9d;                                   // popfd
        *g_tramp_p++ = 0x61;                                   // popad
    }
    memcpy(g_tramp_p, (void *)target, (size_t)savelen); g_tramp_p += savelen;   // saved prologue
    *g_tramp_p++ = 0xe9;                                       // jmp rel32 -> target+savelen
    int32_t rb = (int32_t)((target + savelen) - ((uintptr_t)g_tramp_p + 4));
    memcpy(g_tramp_p, &rb, 4); g_tramp_p += 4;
    FlushInstructionCache(GetCurrentProcess(), t, (SIZE_T)(g_tramp_p - t));
    uint8_t patch[16];
    patch[0] = 0xe9;                                           // jmp rel32 -> trampoline
    int32_t rt = (int32_t)((uintptr_t)t - (target + 5));
    memcpy(patch + 1, &rt, 4);
    for (int i = 5; i < savelen; ++i) patch[i] = 0x90;        // nop tail
    patch_bytes((void *)target, patch, (size_t)savelen);
}

// Suspend/resume every OTHER thread of this process — closes the race where the engine
// executes a half-written 5-byte detour during install.
static void suspend_others(int suspend) {
    DWORD me = GetCurrentThreadId(), pid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 te; te.dwSize = sizeof te;
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID == pid && te.th32ThreadID != me) {
                HANDLE h = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (h) { if (suspend) SuspendThread(h); else ResumeThread(h); CloseHandle(h); }
            }
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
}

static volatile LONG g_hooks_done;
static void ensure_hooks(void) {
    if (InterlockedCompareExchange(&g_hooks_done, 1, 0) != 0) return;   // install once
    void *base = VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!base) { vlog("[hooks] tramp alloc failed"); g_hooks_done = 0; return; }
    g_tramp_p = (uint8_t *)base;
    // Each poll hook's precb saves the poll esp+ecx(manager) into globals BEFORE pushad, so
    // the cb can read the poll's own `now` and inject into the live ring.  `mov [imm32],esp`
    // = 89 25 <addr>; `mov [imm32],ecx` = 89 0d <addr>.
    uint8_t pt[12] = { 0x89, 0x25, 0,0,0,0, 0x89, 0x0d, 0,0,0,0 };
    uint8_t pp[12] = { 0x89, 0x25, 0,0,0,0, 0x89, 0x0d, 0,0,0,0 };
    uint32_t a;
    a = (uint32_t)(uintptr_t)&g_ti_esp; memcpy(pt + 2, &a, 4);
    a = (uint32_t)(uintptr_t)&g_ti_mgr; memcpy(pt + 8, &a, 4);
    a = (uint32_t)(uintptr_t)&g_pk_esp; memcpy(pp + 2, &a, 4);
    a = (uint32_t)(uintptr_t)&g_pk_mgr; memcpy(pp + 8, &a, 4);
    suspend_others(1);
    // The scene `this` for the (experimental) direct transition is read from 0x92dd4c, not a
    // hook — 0x581ba0 loops internally so its entry hook would not re-fire post-injection.
    install_detour((uintptr_t)AP(VA_INPUTPOLL), 5, (void *)poll_title_cb,  pt, 12);  // 53 8b 5c 24 0c
    install_detour((uintptr_t)AP(VA_MENUCTRL),  5, (void *)poll_picker_cb, pp, 12);  // 51 53 55 8b e9
    suspend_others(0);
    vlog("[hooks] installed: title 0x437c70, picker 0x4378d0, tramp=%p", base);
}

// ─── attract (title idle->demo) freeze ───────────────────────────────────────
static uint8_t     g_demo_orig[6];
static volatile int g_demo_saved;
static void attract_freeze(int on) {
    uint8_t *p = (uint8_t *)AP(VA_DEMO_JL);
    if (!mem_readable(p, 6)) return;
    // 0x583866  jl 0x5832e1  (0f 8c 75 fa ff ff)  ->  jmp 0x5832e1 + nop (rel is base-invariant)
    if (on) {
        if (!g_demo_saved) { memcpy(g_demo_orig, p, 6); g_demo_saved = 1; }
        uint8_t patch[6] = { 0xe9, 0x76, 0xfa, 0xff, 0xff, 0x90 };
        patch_bytes(p, patch, 6);
    } else if (g_demo_saved) {
        patch_bytes(p, g_demo_orig, 6);
    }
}

// ─── save-file inspection (reads savedataNN.sdt via the standalone sotes_save lib) ──
// The game keeps saves beside the exe in user\savedataNN.sdt.  Reading the FILE (not
// engine memory) means the trainer can enumerate/identify EVERY save without loading
// it — so it can pick an appropriate slot to `load`.  sotes_save is dependency-free
// (compiled into the DLL) and reusable outside the trainer (editor / the port).
static void trainer_save_path(int slot, char *out, int cap) {
    char mod[MAX_PATH]; mod[0] = 0;
    GetModuleFileNameA(NULL, mod, MAX_PATH); mod[MAX_PATH - 1] = 0;
    char *slash = strrchr(mod, '\\'); if (slash) *slash = 0;
    snprintf(out, cap, "%s\\user\\savedata%02d.sdt", mod, slot);
}
// Emit one slot's summary as JSON into out (always well-formed, even if absent/invalid).
static void save_json_one(int slot, char *out, int cap) {
    char path[512]; trainer_save_path(slot, path, sizeof path);
    sotes_save_info s;
    int rc = sotes_save_read(path, &s);
    if (rc == -2) { snprintf(out, cap, "{\"slot\":%d,\"present\":false}", slot); return; }
    if (!s.ok)    { snprintf(out, cap, "{\"slot\":%d,\"present\":true,\"valid\":false,\"error\":\"decode\"}", slot); return; }
    int n = snprintf(out, cap,
        "{\"slot\":%d,\"present\":true,\"valid\":%s,\"handle\":%u,\"file_size\":%u,"
        "\"key\":%u,\"checksum\":%u,\"party_count\":%d,\"party\":[",
        slot, s.valid ? "true" : "false", s.handle, (unsigned)s.file_size,
        s.hdr.key, s.checksum, s.party_count);
    for (int i = 0; i < s.party_count && n < cap; ++i)
        n += snprintf(out + n, cap - n, "%s{\"name\":\"%s\",\"code\":%u,\"level_base\":%d}",
                      i ? "," : "", s.party[i].name, s.party[i].code, s.party[i].level_base);
    if (n < cap) n += snprintf(out + n, cap - n, "],\"header_grid\":[");
    for (int k = 0; k < 16 && s.ph_present && n < cap; ++k)
        n += snprintf(out + n, cap - n, "%s%u", k ? "," : "", s.ph[k]);
    if (n < cap) snprintf(out + n, cap - n, "]}");
}

// ─── keep-active (no focus steal) + launcher dismiss + attract-off ───────────
// Three DEFAULT-ON boot behaviors so the trainer works while its own window is in the
// background (the normal case when an agent/UI drives it from another machine):
//   1. KEEP-ACTIVE — DirectDraw pauses the game loop on focus loss (retail halts input +
//      render on WM_ACTIVATEAPP deactivate, src wnd_proc).  We re-post WM_ACTIVATEAPP(TRUE)
//      to the main window (class CLASS_LIZSOFT_SOTES) each tick, so the game keeps running
//      UNFOCUSED — WITHOUT stealing focus (no SetForegroundWindow).  Toggle: `keepactive`.
//   2. LAUNCHER DISMISS — click through the #32770 launcher (Full/Safe/Wind/DB/Zoom) IN-
//      PROCESS (BM_CLICK works in-process, unlike an external click — quirk #3), so the
//      inject.exe ship path is hands-free.
//   3. ATTRACT OFF — freeze the title idle->demo trigger so the title never cycles to the
//      attract demo (which would break the menu-drive load).  Toggle: `attract`.
#define OSS_MAIN_WND_CLASS "CLASS_LIZSOFT_SOTES"
#define WM_ACTIVATEAPP_    0x001C
static volatile int g_launch_clicked;
static volatile int g_keepactive = 1;
static volatile int g_main_wnd_seen;   // the CLASS_LIZSOFT_SOTES window has appeared

static BOOL CALLBACK dismiss_child_cb(HWND hwnd, LPARAM lp) {
    (void)lp;
    char cls[64] = {0}, txt[128] = {0};
    GetClassNameA(hwnd, cls, sizeof cls);
    if (lstrcmpiA(cls, "Button") != 0) return TRUE;
    GetWindowTextA(hwnd, txt, sizeof txt);
    char t[128]; int i = 0;
    for (; txt[i] && i < (int)sizeof t - 1; ++i)
        t[i] = (char)((txt[i] >= 'A' && txt[i] <= 'Z') ? txt[i] + 32 : txt[i]);
    t[i] = 0;
    if (!g_launch_clicked && (strstr(t, "launch") || strstr(t, "start") ||
                              strstr(t, "play") || !strcmp(t, "ok") || !strcmp(t, "&ok"))) {
        PostMessageA(hwnd, BM_CLICK, 0, 0);
        g_launch_clicked = 1;
        vlog("[launcher] clicked '%s'", txt);
    }
    return TRUE;
}
static BOOL CALLBACK keepalive_top_cb(HWND hwnd, LPARAM lp) {
    (void)lp;
    DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) return TRUE;
    if (!g_launch_clicked) EnumChildWindows(hwnd, dismiss_child_cb, 0);   // launcher buttons
    char cls[64] = {0}; GetClassNameA(hwnd, cls, sizeof cls);
    if (lstrcmpiA(cls, OSS_MAIN_WND_CLASS) == 0) {
        g_main_wnd_seen = 1;
        if (g_keepactive) PostMessageA(hwnd, WM_ACTIVATEAPP_, 1, 0);   // keep active, no focus steal
    }
    return TRUE;
}
static void ensure_hooks(void);
static DWORD WINAPI keepalive_thread(void *unused) {
    (void)unused;
    attract_freeze(1);                                    // attract OFF by default
    ensure_hooks();                                       // install title/picker hooks early
    for (;;) { EnumWindows(keepalive_top_cb, 0); Sleep(50); }
    return 0;
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
    if (!strcmp(cmd, "state")) {
        // diagnostic: boot/hook/menu-drive state.  ti_mgr!=0 => the TITLE input poll
        // (0x437c70) is firing (game is at the title); pk_mgr!=0 => the save-slot picker
        // poll (0x4378d0) is firing.  box_open/box_scale = the live dialogue-widget read
        // dlgskip gates on (box_open true ⇒ a dialogue box is on screen).
        uint32_t bsc = 0;
        if (g_ti_mgr) rd32((void *)(uintptr_t)(g_ti_mgr + DLG_BOX_SCALE), &bsc);
        snprintf(out, cap,
            "{\"ok\":true,\"hooks\":%d,\"main_wnd\":%s,\"launch_clicked\":%d,"
            "\"attract_frozen\":%s,\"keepactive\":%s,\"dlgskip\":%s,\"box_open\":%s,\"box_scale\":%u,"
            "\"md_state\":%d,\"md_slot\":%d,"
            "\"ng_state\":%d,\"ti_mgr\":\"0x%08x\",\"pk_mgr\":\"0x%08x\"}",
            (int)g_hooks_done, g_main_wnd_seen ? "true" : "false", g_launch_clicked,
            g_demo_saved ? "true" : "false", g_keepactive ? "true" : "false",
            g_dlgskip ? "true" : "false", dialogue_box_open(g_ti_mgr) ? "true" : "false",
            (unsigned)bsc, g_md_state, g_md_slot,
            g_ng_state, (unsigned)g_ti_mgr, (unsigned)g_pk_mgr);
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
        // Write the AUTHORITATIVE phys-box (actor+0x40 -> box), not the derived
        // world_x/y snapshot.  box[+4]=X sticks; box[+8]=top settles via gravity.
        long long x, y; int rel = json_bool(line, "relative", 0);
        uintptr_t base = find_player();
        if (!base) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no player\"}"); return; }
        uint32_t box = 0;
        if (!rd32((void *)(base + OFF_BOX), &box) || box <= 0x10000 ||
            !mem_readable((void *)(uintptr_t)box, 0x14)) {
            snprintf(out, cap, "{\"ok\":false,\"error\":\"no phys-box\"}"); return; }
        if (json_num(line, "x", &x)) {
            uint32_t cur = 0; rd32((void *)((uintptr_t)box + BOX_X), &cur);
            write_typed((uintptr_t)box + BOX_X, rel ? cur + (uint32_t)(x*100) : (uint32_t)x, "u32");
        }
        if (json_num(line, "y", &y)) {
            uint32_t h = 0, curtop = 0;
            rd32((void *)((uintptr_t)box + BOX_H),   &h);
            rd32((void *)((uintptr_t)box + BOX_TOP), &curtop);
            // absolute y is a world_y target (bottom) -> top = y - height + 1;
            // relative y nudges the top (px), world_y follows.
            uint32_t newtop = rel ? curtop + (uint32_t)(y*100)
                                  : (uint32_t)((int)y - (int)h + 1);
            write_typed((uintptr_t)box + BOX_TOP, newtop, "u32");
        }
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":true,\"box\":\"0x%08x\",\"player\":%s}", box, pj);
        return;
    }
    if (!strcmp(cmd, "box")) {                    // debug: read the player's phys-box
        uintptr_t base = find_player(); uint32_t box = 0;
        if (!base || !rd32((void *)(base + OFF_BOX), &box) || box <= 0x10000) {
            snprintf(out, cap, "{\"ok\":false,\"error\":\"no box\"}"); return; }
        uint32_t bx=0,top=0,w=0,h=0,tag=0;
        rd32((void*)((uintptr_t)box+0),&tag);   rd32((void*)((uintptr_t)box+BOX_X),&bx);
        rd32((void*)((uintptr_t)box+BOX_TOP),&top); rd32((void*)((uintptr_t)box+BOX_W),&w);
        rd32((void*)((uintptr_t)box+BOX_H),&h);
        snprintf(out, cap, "{\"ok\":true,\"box\":\"0x%08x\",\"tag\":\"0x%08x\",\"x\":%d,"
                 "\"top\":%d,\"w\":%d,\"h\":%d,\"world_y\":%d}",
                 box, tag, (int)bx, (int)top, (int)w, (int)h, (int)top+(int)h-1);
        return;
    }
    if (!strcmp(cmd, "unlock_all")) { lock_clear_all(); g_god = 0; snprintf(out, cap, "{\"ok\":true}"); return; }
    if (!strcmp(cmd, "call")) {
        // {"cmd":"call","va":"0x585cf0","a0":..,"a1":..,...,"ecx":"0x..","reloc":true}
        // Calls the engine fn; args a0..a7 (contiguous), ecx=this (thiscall).  EXPERIMENTAL:
        // use for the direct-load chain etc. once the caller state is right (DESIGN).
        long long va, tmp;
        if (!json_num(line, "va", &va)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no va\"}"); return; }
        int reloc = json_bool(line, "reloc", 1);        // module VA -> AP()-relocate by default
        uintptr_t fn = reloc ? (uintptr_t)AP(va) : (uintptr_t)va;
        if (!mem_readable((void *)fn, 1)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"fn unreadable\"}"); return; }
        uint32_t args[8]; int n = 0; char key[4];
        for (int i = 0; i < 8; ++i) {
            snprintf(key, sizeof key, "a%d", i);
            if (!json_num(line, key, &tmp)) break;
            args[n++] = (uint32_t)tmp;
        }
        uint32_t ecx = 0; if (json_num(line, "ecx", &tmp)) ecx = (uint32_t)tmp;
        uint32_t r = call_va((void *)fn, ecx, args, n);
        snprintf(out, cap, "{\"ok\":true,\"ret\":%u,\"ret_hex\":\"0x%08x\",\"fn\":\"0x%08x\",\"argc\":%d}",
                 r, r, (unsigned)fn, n);
        return;
    }

    if (!strcmp(cmd, "saves")) {
        // {"cmd":"saves"} — enumerate + identify EVERY on-disk save (no engine load).
        // Reads user\savedataNN.sdt directly (the standalone sotes_save decoder), so the
        // agent/UI can see what each slot is and pick one to `load`.
        int n = snprintf(out, cap, "{\"ok\":true,\"saves\":[");
        int first = 1;
        for (int slot = 0; slot < 16 && n < cap; ++slot) {
            char path[512]; trainer_save_path(slot, path, sizeof path);
            FILE *f = fopen(path, "rb"); if (!f) continue; fclose(f);
            char sj[640]; save_json_one(slot, sj, sizeof sj);
            n += snprintf(out + n, cap - n, "%s%s", first ? "" : ",", sj); first = 0;
        }
        if (n < cap) snprintf(out + n, cap - n, "]}");
        return;
    }
    if (!strcmp(cmd, "saveinfo")) {
        // {"cmd":"saveinfo","slot":N} — full summary of one slot.
        long long slot;
        if (!json_num(line, "slot", &slot)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no slot\"}"); return; }
        char sj[640]; save_json_one((int)slot, sj, sizeof sj);
        snprintf(out, cap, "{\"ok\":true,\"save\":%s}", sj);
        return;
    }

    if (!strcmp(cmd, "load")) {
        // {"cmd":"load"[,"slot":N][,"downs":N]} — MENU-DRIVE (the game's own load path, no
        // risky direct engine calls): inject confirm at the TITLE (0x437c70) -> Continue ->
        // the save-slot picker; then at the PICKER (0x4378d0) load the target save.
        //   "slot":N  -> select savedataNN via the RE'd picker selection model, then confirm
        //               (any slot; VERIFIED default path, slot-targeting RE'd — see DESIGN).
        //   no slot   -> load the default-highlighted (newest) save (the VERIFIED tower path).
        //   "downs":N -> manual: rotate the picker N times before confirm (fallback nav).
        long long tmp;
        ensure_hooks();
        attract_freeze(1);                               // hold the title (no demo mid-drive)
        g_md_slot  = json_num(line, "slot",  &tmp) ? (int)tmp : -1;
        g_md_downs = json_num(line, "downs", &tmp) ? (int)tmp : 0;
        g_pk_diag[0] = 0; g_pk_diag_done = 0;            // reset the one-shot picker dump
        g_pk_mgr = 0;                                    // clear so poll_title_cb sees the picker OPEN
        g_md_state = 1;                                  // arm: title-confirm -> picker-confirm
        int loaded = 0;                                  // done when the loaded actor appears
        for (int i = 0; i < 1200; ++i) { if (find_player()) { loaded = 1; break; } Sleep(10); }
        g_md_state = 0; g_md_slot = -1;                  // end the drive
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":%s,\"loaded\":%s,\"picker\":\"%s\",\"player\":%s}",
                 loaded ? "true" : "false", loaded ? "true" : "false", g_pk_diag, pj);
        return;
    }
    if (!strcmp(cmd, "newgame")) {
        // {"cmd":"newgame"[,"to":N]} — from the TITLE, rotate N times to the "New Game"/Start
        // item then confirm (the game's own menu).  "to" = title rotations from the default
        // (Continue) to Start (default 1).  With dlgskip on, the intro cutscene skips itself.
        long long tmp;
        ensure_hooks();
        attract_freeze(1);
        g_ng_rotates = json_num(line, "to",  &tmp) ? (int)tmp : 1;   // rotations to New Game
        g_ng_btn     = json_num(line, "btn", &tmp) ? (int)tmp : 0x04; // 2=up / 4=down
        g_ng_gap     = json_num(line, "gap", &tmp) ? (int)tmp : 6;    // polls between rotates
        g_ng_cool    = 4;                                            // small settle before the 1st rotate
        g_ng_state = 1;                                  // arm: rotate-to-Start -> confirm
        int started = 0;                                 // done when the fresh player actor appears
        for (int i = 0; i < 3000; ++i) { if (find_player()) { started = 1; break; } Sleep(10); }
        g_ng_state = 0;
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap, "{\"ok\":%s,\"started\":%s,\"player\":%s}",
                 started ? "true" : "false", started ? "true" : "false", pj);
        return;
    }
    if (!strcmp(cmd, "loadraw")) {
        // {"cmd":"loadraw","slot":N[,"enter":true]} — EXPERIMENTAL direct chain: alloc S ->
        // 416550 load -> 586c60 apply -> (enter) 0x5cb460 transition, on the safepoint.
        // load+apply are verified; the standalone transition CRASHES (needs the picker
        // dispatcher `this` + slot-index arg) — prefer `load`.  See DESIGN.md session 3b.
        long long slot;
        if (!json_num(line, "slot", &slot)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no slot\"}"); return; }
        ensure_hooks();
        g_load_log[0] = 0; g_load_ret = 0;
        g_load_slot = (int)slot;
        g_load_enter = json_bool(line, "enter", 0);   // default: load+apply only (crash-safe)
        g_load_pending = 1;
        for (int i = 0; i < 300 && g_load_pending; ++i) Sleep(10);   // await the safepoint (<100ms at title)
        int ran = !g_load_pending;
        if (!ran) { g_load_pending = 0; snprintf(g_load_log, sizeof g_load_log, "safepoint never fired (game not presenting / not at a scene?)"); }
        char pj[512]; player_json(pj, sizeof pj);
        snprintf(out, cap,
            "{\"ok\":%s,\"ran\":%s,\"ret\":%u,\"dispatch_this\":\"0x%08x\",\"log\":\"%s\",\"player\":%s}",
            ran ? "true" : "false", ran ? "true" : "false", g_load_ret,
            (unsigned)g_dispatch_this, g_load_log, pj);
        return;
    }
    if (!strcmp(cmd, "keepactive")) {
        // {"cmd":"keepactive","on":true|false} — re-post WM_ACTIVATEAPP(TRUE) so the game
        // keeps rendering/updating while unfocused, without stealing focus.  Default ON.
        g_keepactive = json_bool(line, "on", 1);
        snprintf(out, cap, "{\"ok\":true,\"keepactive\":%s}", g_keepactive ? "true" : "false");
        return;
    }
    if (!strcmp(cmd, "dlgskip")) {
        // {"cmd":"dlgskip","on":true|false} — auto-skip dialogue: inject the dialogue advance
        // buttons (0x24/0x27) every gameplay poll, so any dialogue advances the instant it opens.
        // Default ON.  Harmless outside dialogue (those ids are dialogue-only).
        g_dlgskip = json_bool(line, "on", 1);
        snprintf(out, cap, "{\"ok\":true,\"dlgskip\":%s}", g_dlgskip ? "true" : "false");
        return;
    }
    if (!strcmp(cmd, "dlgbtns")) {
        // {"cmd":"dlgbtns","b0":..,"b1":..,...} — set which button ids dlgskip injects each poll
        // (0/absent = clear that slot).  For tuning the reveal-skip button set live.
        long long tmp; char key[4];
        for (int i = 0; i < 6; ++i) {
            snprintf(key, sizeof key, "b%d", i);
            g_dlg_btns[i] = json_num(line, key, &tmp) ? (int)tmp : 0;
        }
        int n = snprintf(out, cap, "{\"ok\":true,\"dlgbtns\":[");
        for (int i = 0; i < 6; ++i) if (g_dlg_btns[i]) n += snprintf(out+n, cap-n, "%s%d", n && out[n-1]!='[' ? "," : "", g_dlg_btns[i]);
        snprintf(out+n, cap-n, "]}");
        return;
    }
    if (!strcmp(cmd, "press")) {
        // {"cmd":"press","btn":N[,"n":count]} — inject button N into the active input mgr `count`
        // times over the next polls (probe: find which button does what in the current context).
        long long btn, cnt = 1;
        if (!json_num(line, "btn", &btn)) { snprintf(out, cap, "{\"ok\":false,\"error\":\"no btn\"}"); return; }
        json_num(line, "n", &cnt);
        uint32_t mgr = g_ti_mgr;
        for (int i = 0; i < (int)cnt && mgr; ++i) {
            uint32_t nowv = 0, e = g_ti_esp; if (e) rd32((void *)(uintptr_t)(e + 4), &nowv);
            inject_record(mgr, (uint32_t)btn, nowv, 8);
            Sleep(16);
        }
        snprintf(out, cap, "{\"ok\":true,\"pressed\":%d,\"n\":%d,\"mgr\":\"0x%08x\"}", (int)btn, (int)cnt, (unsigned)mgr);
        return;
    }
    if (!strcmp(cmd, "attract")) {
        // {"cmd":"attract","freeze":true|false} — patch/unpatch the title idle->demo
        // trigger so the title stays up indefinitely (no engine hooks needed).
        int freeze = json_bool(line, "freeze", 1);
        attract_freeze(freeze);
        snprintf(out, cap, "{\"ok\":true,\"attract_frozen\":%s}", freeze ? "true" : "false");
        return;
    }

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
                char resp[8192];   // large enough for `saves` (all slots) + summaries
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
        CreateThread(NULL, 0, keepalive_thread, NULL, 0, NULL);  // launcher + keep-active + attract-off
    }
    return TRUE;
}
