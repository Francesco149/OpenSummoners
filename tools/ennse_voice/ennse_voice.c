// tools/ennse_voice/ennse_voice.c — EN-SE JP-voice patch (a tools/mod_loader mod).
//
// The EN special edition (sotes_en.exe) still contains the ENTIRE voice subsystem
// (dialogue trigger + play + the per-actor COMBAT-voice bake, byte-identical to the JP
// engine); the localizer removed exactly ONE line — the voice-bank load — so the voice-bank
// global `DAT_0092af80` stays null forever and the whole subsystem is intact but dead.  We
// revive it with the engine's OWN functions:
//     0x92af80 = bank_load("sotesx_s.dll")           // the DLL HMODULE (the "gate")
//     0x92b76c = new; manager_init(sounddev, 0x10)    // the voice manager (dialogue)
//
// Two consumers, two needs (docs/findings/ense-voice-combat-init.md):
//   • dialogue voice — the trigger reads the bank+manager LIVE per line.  The manager must
//     be OUR do_seed manager: seeding the bank before the engine's boot gate makes the gate
//     build a manager that (observed) does NOT voice dialogue, so we seed the bank AFTER that
//     gate (the game loop) => the gate saw a null bank + skipped, and our proven manager wins.
//   • COMBAT voice (Arche's grunts = sotesx_s clips 2235/2236) — the per-actor action-table
//     builder `FUN_00423850` bakes each attack's voice id ONLY `if (0x92af80 != 0)` AT
//     ACTOR-CREATION; else 0 => permanently silent.  We set the bank before the first party
//     actor (frame 0 of the game loop) AND hook the builder itself, so every actor bakes.
//
// TRIGGER — proxy the engine's own message pump by IAT-hooking its import PeekMessageA
// (IAT slot 0x5fd20c): once the loader/SteamStub has resolved that slot we overwrite it with
// our my_peek.  When the engine pumps its first frame (main/engine thread, AFTER SteamStub
// decrypts .text, AFTER sound init, BEFORE any party actor) my_peek runs do_seed + installs
// the actor-builder hook, then tail-calls the real PeekMessageA.  An IAT hook is just a
// pointer swap — prologue-independent, so it works on Win10/11 where system DLLs no longer
// carry the `mov edi,edi` hot-patch prologue.  The slot resolves at the SteamStub OEP, long
// before the first pump (which is after the ~260 MB bank load + window + sound init), so the
// tiny thread that waits for the slot to resolve has a wide margin; the actual seed is driven
// deterministically by the engine's own pump.
//
// Both seed paths run on the MAIN (engine) thread => do_seed's operator_new serializes with
// the engine's allocator (never the old worker-thread heap race that dropped monster SFX).
// Process memory only; the exe on disk is untouched.  Loaded as mods\ennse_voice.dll by
// tools/mod_loader (proxy version.dll).  Log: oss_voice.log beside the exe.
//
//   nix develop --command make -C tools/ennse_voice     # -> build/ennse_voice.dll

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

static const char *StrStrIA_(const char *hay, const char *needle);  // defined below

// ─── EN engine (sotes_en.exe / sotes-ense-en) addresses — unpacked ImageBase 0x400000 ──
#define ORIG_BASE       0x400000u
#define VA_BANK_LOAD    0x5d8b10u   // thiscall bank_load(assetmgr, name, a, b) -> bank HMODULE
#define VA_ASSET_MGR    0x92ac68u   // asset-manager object (direct `this`)
#define VA_OPNEW        0x5ef121u   // cdecl  operator_new(size)
#define VA_MGR_INIT     0x584a70u   // thiscall manager_init(mgr, sounddev, count)
#define VA_BANK_GLOBAL  0x92af80u   // void*  voice bank handle   (== JP 0x926170)
#define VA_MGR_GLOBAL   0x92b76cu   // void*  voice manager       (== JP 0x926958)
#define VA_SOUNDDEV     0x92d5b8u   // void*  sound device (non-null once sound-init ran)
#define VA_ACTOR_BUILD  0x423850u   // FUN_00423850 — per-actor action-table builder (combat bake)
#define VA_IAT_PEEK     0x5fd20cu   // IAT slot for USER32!PeekMessageA (the engine's pump)
// decrypted prologue of the actor builder: sub esp,8 ; push ebx ; push ebp   (5 bytes)
static const unsigned char ACTOR_SIG[5] = { 0x83, 0xec, 0x08, 0x53, 0x55 };

typedef BOOL(WINAPI *peek_t)(LPMSG, HWND, UINT, UINT, UINT);
static peek_t g_real_peek;

static uintptr_t g_delta;
static char      g_logpath[MAX_PATH];

typedef void *(__attribute__((thiscall)) *bankload_t)(void *, const char *, int, int);
typedef void *(__cdecl                    *opnew_t)(size_t);
typedef void  (__attribute__((thiscall)) *mgrinit_t)(void *, void *, int);

#define AP(x) ((void *)((uintptr_t)(x) + g_delta))

static void vlog(const char *fmt, ...) {
    FILE *f = fopen(g_logpath, "a"); if (!f) return;
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f); fclose(f);
}

// ─── the seed: revive the two dead globals with the engine's OWN functions.  Idempotent;
// bank FIRST (combat only needs the bank) then the manager once the sound device is up
// (dialogue).  MUST run on the engine MAIN thread (the pump hook + the actor hook are both
// main-thread) so operator_new serializes with the engine's allocator.
static void do_seed(void) {
    void **sounddev = (void **)AP(VA_SOUNDDEV);
    void **bankg    = (void **)AP(VA_BANK_GLOBAL);
    void **mgrg     = (void **)AP(VA_MGR_GLOBAL);

    if (*bankg == 0) {
        bankload_t bank_load = (bankload_t)AP(VA_BANK_LOAD);
        *bankg = bank_load(AP(VA_ASSET_MGR), "sotesx_s.dll", 1, 1);
        vlog("[seed] bank_load(\"sotesx_s.dll\") -> %p  (tid=%lu)", *bankg, GetCurrentThreadId());
    }
    if (*bankg != 0 && *sounddev != 0 && *mgrg == 0) {
        opnew_t   opnew    = (opnew_t)AP(VA_OPNEW);
        mgrinit_t mgr_init = (mgrinit_t)AP(VA_MGR_INIT);
        void *mgr = opnew(0xc);
        if (mgr) { *(int *)mgr = 0; mgr_init(mgr, *sounddev, 0x10); *mgrg = mgr;
                   vlog("[seed] manager -> %p  (dialogue live)", mgr); }
    }
}

// ─── generic inline "run cb() before target" hook.  Steals `steal` prologue bytes (caller
// verifies they are whole instructions), redirects target -> a self-contained RWX stub that
// preserves all registers, calls cb(), replays the stolen bytes, and jumps back.
static BOOL hook_before(void *target, void (*cb)(void), int steal) {
    unsigned char *t = (unsigned char *)target;
    unsigned char *stub = (unsigned char *)VirtualAlloc(NULL, 96, MEM_COMMIT | MEM_RESERVE,
                                                        PAGE_EXECUTE_READWRITE);
    if (!stub) return FALSE;
    unsigned char *p = stub;
    *p++ = 0x60; *p++ = 0x9c;                                   // pushad; pushfd
    *p++ = 0xb8; *(void **)p = (void *)cb; p += 4;              // mov eax, cb
    *p++ = 0xff; *p++ = 0xd0;                                   // call eax
    *p++ = 0x9d; *p++ = 0x61;                                   // popfd; popad
    memcpy(p, t, steal); p += steal;                           // stolen prologue
    unsigned char *jmp = p;
    *p++ = 0xff; *p++ = 0x25; *(void **)p = (void *)(jmp + 6); p += 4;  // jmp [jmp+6]
    *(void **)p = (void *)(t + steal);                         // resume slot = target+steal

    DWORD old;
    if (!VirtualProtect(t, steal, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
    t[0] = 0xE9; *(int32_t *)(t + 1) = (int32_t)((uintptr_t)stub - ((uintptr_t)t + 5));
    for (int i = 5; i < steal; i++) t[i] = 0x90;               // NOP any spare stolen bytes
    VirtualProtect(t, steal, old, &old);
    FlushInstructionCache(GetCurrentProcess(), t, steal);
    return TRUE;
}

// install the actor-builder hook once .text is decrypted; do_seed runs at each actor build,
// setting the bank before that actor's voice-id bake (bulletproofs combat).
static int g_actor_hooked;
static void install_actor_hook(void) {
    if (g_actor_hooked) return;
    unsigned char *fn = (unsigned char *)AP(VA_ACTOR_BUILD);
    if (memcmp(fn, ACTOR_SIG, sizeof ACTOR_SIG) != 0) return;  // .text not decrypted yet
    g_actor_hooked = 1;
    if (hook_before(fn, do_seed, 5))
        vlog("[hook] actor-builder hook @ 0x%x installed", VA_ACTOR_BUILD);
    else
        vlog("[hook] actor-builder hook @ 0x%x FAILED", VA_ACTOR_BUILD);
}

// ─── the pump proxy: runs on the engine main thread every frame.  First time we are past
// .text decryption + sound init, seed (bank+manager) and arm the actor hook; thereafter a
// couple of cheap null-checks.
static void pump_cb(void) {
    static int in_pump;                    // main-thread reentrancy guard (do_seed loads a DLL)
    if (in_pump) return;
    in_pump = 1;
    do_seed();             // idempotent: bank now, manager once the sound device is up
    install_actor_hook();  // idempotent: once .text is decrypted
    in_pump = 0;
}

// our replacement for USER32!PeekMessageA in the engine's IAT — runs the seed on the engine
// thread, then tail-calls the real PeekMessageA (fetched directly, not via the hooked slot).
static BOOL WINAPI my_peek(LPMSG msg, HWND hwnd, UINT lo, UINT hi, UINT rm) {
    pump_cb();
    return g_real_peek(msg, hwnd, lo, hi, rm);
}

// wait for the loader/SteamStub to resolve the PeekMessageA IAT slot, then swap in my_peek.
// Wide margin: the slot resolves at the OEP, the first pump is much later (post bank load).
static DWORD WINAPI iat_thread(void *unused) {
    (void)unused;
    void **slot = (void **)AP(VA_IAT_PEEK);
    int i = 0;
    while (*slot != (void *)g_real_peek) {
        if (++i > 6000) { vlog("[proxy] TIMEOUT: PeekMessageA IAT slot never resolved (*slot=%p)", *slot); return 0; }
        Sleep(5);
    }
    DWORD old;
    if (VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &old)) {
        *slot = (void *)&my_peek;
        VirtualProtect(slot, sizeof(void *), old, &old);
        vlog("[proxy] IAT pump hook: 0x%x PeekMessageA -> my_peek (after %d ms)", VA_IAT_PEEK, i * 5);
    } else vlog("[proxy] VirtualProtect on IAT slot 0x%x failed", VA_IAT_PEEK);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID x) {
    (void)x;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        char dir[MAX_PATH]; GetModuleFileNameA(h, dir, MAX_PATH);
        char *bs = strrchr(dir, '\\'); if (bs) bs[1] = 0; else dir[0] = 0;
        _snprintf(g_logpath, MAX_PATH, "%soss_voice.log", dir);

        char host[MAX_PATH] = ""; GetModuleFileNameA(NULL, host, MAX_PATH);
        char *slash = strrchr(host, '\\'); const char *hn = slash ? slash + 1 : host;
        if (!StrStrIA_(hn, "sotes")) { vlog("[proxy] host '%s' has no 'sotes' — skip", hn); return TRUE; }

        g_delta = (uintptr_t)GetModuleHandleA(NULL) - ORIG_BASE;
        vlog("[proxy] attach host=%s delta=%p", host, (void *)g_delta);

        // Proxy the engine's message pump via its PeekMessageA IAT slot.
        HMODULE u32 = GetModuleHandleA("user32.dll");
        g_real_peek = u32 ? (peek_t)GetProcAddress(u32, "PeekMessageA") : NULL;
        if (!g_real_peek) { vlog("[proxy] PeekMessageA not resolvable — no patch"); return TRUE; }
        void **slot = (void **)AP(VA_IAT_PEEK);
        if (*slot == (void *)g_real_peek) {          // already resolved (loader) — hook now
            DWORD old;
            if (VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &old)) {
                *slot = (void *)&my_peek;
                VirtualProtect(slot, sizeof(void *), old, &old);
                vlog("[proxy] IAT pump hook: 0x%x PeekMessageA -> my_peek (immediate)", VA_IAT_PEEK);
            }
        } else {                                     // SteamStub resolves it later — wait for it
            vlog("[proxy] PeekMessageA IAT slot not yet resolved (*slot=%p) — arming waiter", *slot);
            CreateThread(NULL, 0, iat_thread, NULL, 0, NULL);
        }
    }
    return TRUE;
}

// tiny case-insensitive substring (avoid linking shlwapi for StrStrIA)
static const char *StrStrIA_(const char *hay, const char *needle) {
    if (!hay || !needle) return NULL;
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n && (tolower((unsigned char)*h) == tolower((unsigned char)*n))) { h++; n++; }
        if (!*n) return hay;
    }
    return NULL;
}
