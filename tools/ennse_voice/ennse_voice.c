// tools/ennse_voice/ennse_voice.c — EN-SE JP-voice patch (a mod_loader mod).
//
// The EN special edition (sotes_en.exe) still contains the ENTIRE voice subsystem
// (dialogue trigger + play + the per-actor COMBAT-voice bake, byte-identical to the JP
// engine); the localizer removed exactly ONE line — the voice-bank load inside the boot
// bank-loader.  Concretely, JP `FUN_005c94f0` line 3951 does
//     DAT_00926170 = bank_load("sotesx_s.dll");     // voice bank HMODULE
// and the EN mirror `FUN_005cb880` simply omits it, so the EN voice-bank global
// `DAT_0092af80` (== JP `DAT_00926170`) stays null forever.  Everything downstream is
// intact but dead, gated on that null:
//   • dialogue voice — boot gate `FUN_00581ba0` builds the voice MANAGER `0x92b76c`
//     only `if (DAT_0092af80 != 0)`, and the dialogue trigger reads the bank live.
//   • COMBAT voice — the per-actor action-table builder `FUN_00423850` bakes each
//     attack action's voice id into the actor ONLY `if (DAT_0092af80 != 0)` AT
//     ACTOR-CREATION TIME; otherwise the id is baked 0 => that actor is permanently
//     silent in combat.  (Arche's attack grunts = sotesx_s clips 2235/2236.)
//
// => The whole fix is to set `DAT_0092af80` to the sotesx_s.dll HMODULE, and to do it
//    EARLY — before any actor is built — exactly where JP loads it.  The OLD build seeded
//    late (a worker thread that waited for the sound device, then bounced onto the main
//    thread via the window's WndProc); that runs AFTER the party actors are built, so
//    dialogue worked but combat voice was baked-silent.  See docs/findings/
//    ense-voice-combat-init.md and docs/plans/ennse-voice-patch.md.
//
// THE HOOK (JP-faithful, deterministic): the EN app-init `FUN_00580ec0` clears the bank
// cluster, calls the bank-loader `FUN_005cb880` (0x580fde), then calls the boot/main-loop
// `FUN_00581ba0` (0x58113e).  We redirect that one CALL at 0x58113e through a tiny thunk
// that runs the removed load (0x92af80 = bank_load("sotesx_s.dll")) and then jumps into
// the real boot.  So the bank is live BEFORE the boot's manager gate and BEFORE the first
// actor — the engine then builds the manager and bakes combat-voice ids through its OWN
// retained code, identical to the JP engine.  We patch process memory only; the exe on
// disk is untouched.  The 0x58113e byte signature (E8 5d 0a 00 00) positively identifies
// this exact EN-SE build, so we never mis-patch the bundled JP `sotes.exe` (which self-
// voices) or an unrecognized build.
//
// Ships as a plain mod DLL loaded by ../mod_loader (the generic proxy version.dll).
// INSTALL (…\steamapps\common\sotes\): version.dll (mod_loader) + realver.dll +
// mods\ennse_voice.dll (this) + sotesx_s.dll (the JP voice bank).  The patch installer
// drops all of it.  `oss_voice.log` (next to the exe) traces each step.
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
// Runtime-relocated via g_delta = actual base - 0x400000 (survives ASLR/rebase).
#define ORIG_BASE      0x400000u
#define VA_BANK_LOAD   0x5d8b10u   // thiscall bank_load(assetmgr, name, a, b) -> bank HMODULE
#define VA_ASSET_MGR   0x92ac68u   // asset-manager object (direct `this`)
#define VA_BANK_GLOBAL 0x92af80u   // void*  voice bank handle  (THE gate; == JP 0x926170)
#define VA_BOOT_CALLSITE 0x58113eu // the `CALL FUN_00581ba0` inside app-init FUN_00580ec0
#define VA_BOOT_FN     0x581ba0u   // boot / main-loop (JP 0x57fe50); builds the mgr via its gate

// legacy late-seed fallback (env OSS_ENNSE_LATE_SEED=1) — kept for A/B + odd builds
#define VA_OPNEW       0x5ef121u   // cdecl  operator_new(size)
#define VA_MGR_INIT    0x584a70u   // thiscall manager_init(mgr, sounddev, count)
#define VA_MGR_GLOBAL  0x92b76cu   // void*  voice manager
#define VA_SOUNDDEV    0x92d5b8u   // void*  sound device (non-null once sound-init ran)

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

// ─── the seed: restore the ONE removed line — load sotesx_s.dll into the voice-bank
// global.  Idempotent.  Called from the early hook stub (main thread, during boot, after
// the bank cluster is loaded and before the manager gate + first actor).  Uses the
// engine's OWN bank_load so the value is byte-for-byte what the engine would have stored.
static void __cdecl set_bank_only(void) {
    void **bankg = (void **)AP(VA_BANK_GLOBAL);
    if (*bankg != 0) { vlog("[hook] bank already set: %p", *bankg); return; }
    bankload_t bank_load = (bankload_t)AP(VA_BANK_LOAD);
    void *bank = bank_load(AP(VA_ASSET_MGR), "sotesx_s.dll", 1, 1);
    *bankg = bank;
    vlog("[hook] restored removed load: 0x92af80 = bank_load(\"sotesx_s.dll\") -> %p "
         "(engine builds mgr + actors bake combat voice)", bank);
}

// ─── the early main-thread hook: repoint the CALL at 0x58113e (app-init -> boot) through
// a stub that runs set_bank_only() first, then jumps into the real boot.  No stolen-byte
// trampoline: redirecting the call keeps the original prologue/args/frame intact, so it is
// calling-convention-agnostic (the stub preserves every register with pushad/pushfd).
static void *g_boot_fn_abs;     // absolute runtime addr of FUN_00581ba0 (the stub's jmp target)

static BOOL install_early_hook(void) {
    unsigned char *cs = (unsigned char *)AP(VA_BOOT_CALLSITE);   // 0x58113e
    int32_t want = (int32_t)(VA_BOOT_FN - (VA_BOOT_CALLSITE + 5));
    int32_t rel;  memcpy(&rel, cs + 1, 4);
    if (cs[0] != 0xE8 || rel != want) {
        vlog("[hook] callsite 0x%x signature mismatch (op=%02x rel=%08x, want E8 rel=%08x)"
             " — not the expected EN-SE build; no patch applied.", VA_BOOT_CALLSITE, cs[0], rel, want);
        return FALSE;
    }
    g_boot_fn_abs = AP(VA_BOOT_FN);

    unsigned char *stub = (unsigned char *)VirtualAlloc(NULL, 64, MEM_COMMIT | MEM_RESERVE,
                                                        PAGE_EXECUTE_READWRITE);
    if (!stub) { vlog("[hook] VirtualAlloc failed"); return FALSE; }
    unsigned char *p = stub;
    *p++ = 0x60;                                                // pushad
    *p++ = 0x9c;                                                // pushfd
    *p++ = 0xb8; *(void **)p = (void *)&set_bank_only; p += 4;  // mov eax, &set_bank_only
    *p++ = 0xff; *p++ = 0xd0;                                   // call eax  (cdecl, no args)
    *p++ = 0x9d;                                                // popfd
    *p++ = 0x61;                                                // popad
    *p++ = 0xff; *p++ = 0x25; *(void **)p = (void *)&g_boot_fn_abs; p += 4;  // jmp [g_boot_fn_abs]

    int32_t newrel = (int32_t)((uintptr_t)stub - ((uintptr_t)cs + 5));
    DWORD old;
    if (!VirtualProtect(cs, 5, PAGE_EXECUTE_READWRITE, &old)) { vlog("[hook] VirtualProtect failed"); return FALSE; }
    memcpy(cs + 1, &newrel, 4);                                 // keep E8; repoint rel32 -> stub
    VirtualProtect(cs, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), cs, 5);
    vlog("[hook] installed: 0x%x CALL -> stub %p -> boot 0x%x  (delta=%p)",
         VA_BOOT_CALLSITE, (void *)stub, VA_BOOT_FN, (void *)g_delta);
    return TRUE;
}

// ─── legacy late-seed fallback (env OSS_ENNSE_LATE_SEED=1) ───────────────────────────────
// The pre-fix mechanism: from a worker thread, wait for the sound device, then run the
// full seed (bank + manager) on the engine MAIN thread via the game window's WndProc.  This
// fixes DIALOGUE voice but is too late for COMBAT voice (actors are already baked-silent).
// Kept only as an A/B toggle and a safety net for builds the early hook doesn't recognize.
#define WM_OSS_SEED (WM_APP + 0x5153)
static WNDPROC g_orig_wndproc;
static HWND    g_game_hwnd;

static void do_seed(void) {
    void **sounddev = (void **)AP(VA_SOUNDDEV);
    void **bankg    = (void **)AP(VA_BANK_GLOBAL);
    void **mgrg     = (void **)AP(VA_MGR_GLOBAL);
    if (*sounddev == 0) { vlog("[seed] do_seed: sound device null — abort"); return; }
    if (*bankg == 0) {
        bankload_t bank_load = (bankload_t)AP(VA_BANK_LOAD);
        *bankg = bank_load(AP(VA_ASSET_MGR), "sotesx_s.dll", 1, 1);
        vlog("[seed] bank_load(\"sotesx_s.dll\") -> %p", *bankg);
    }
    if (*bankg && *mgrg == 0) {
        opnew_t   opnew    = (opnew_t)AP(VA_OPNEW);
        mgrinit_t mgr_init = (mgrinit_t)AP(VA_MGR_INIT);
        void *mgr = opnew(0xc);
        if (mgr) { *(int *)mgr = 0; mgr_init(mgr, *sounddev, 0x10); *mgrg = mgr; vlog("[seed] manager -> %p", mgr); }
    }
    vlog("[seed] DONE bank=%p mgr=%p", *bankg, *mgrg);
}

static LRESULT CALLBACK seed_wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_OSS_SEED) {
        vlog("[seed] running on MAIN thread (WndProc, tid=%lu)", GetCurrentThreadId());
        do_seed();
        SetWindowLongPtrA(h, GWLP_WNDPROC, (LONG_PTR)g_orig_wndproc);
        return 0;
    }
    return CallWindowProcA(g_orig_wndproc, h, msg, wp, lp);
}

static BOOL CALLBACK find_game_wnd(HWND h, LPARAM lp) {
    (void)lp;
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (pid != GetCurrentProcessId()) return TRUE;
    if (!IsWindowVisible(h) || GetWindow(h, GW_OWNER) != NULL) return TRUE;
    char cls[64] = ""; GetClassNameA(h, cls, (int)sizeof cls);
    if (strcmp(cls, "#32770") == 0) return TRUE;
    g_game_hwnd = h; return FALSE;
}

static DWORD WINAPI seed_thread(void *unused) {
    (void)unused;
    void **sounddev = (void **)AP(VA_SOUNDDEV);
    int i = 0;
    while (*sounddev == 0) { if (++i > 3000) { vlog("[seed] TIMEOUT: sound device never up"); return 0; } Sleep(20); }
    Sleep(600);
    for (i = 0; i < 200 && !g_game_hwnd; ++i) { EnumWindows(find_game_wnd, 0); if (!g_game_hwnd) Sleep(25); }
    if (g_game_hwnd) {
        g_orig_wndproc = (WNDPROC)SetWindowLongPtrA(g_game_hwnd, GWLP_WNDPROC, (LONG_PTR)seed_wndproc);
        if (g_orig_wndproc) { PostMessageA(g_game_hwnd, WM_OSS_SEED, 0, 0); return 0; }
    }
    do_seed();   // last resort (worker-thread seed)
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

        // The tools/mod_loader worker LoadLibrarys us during boot, before the engine reaches
        // the 0x58113e call site (it first maps ~260 MB of bank DLLs), so the early hook wins
        // the race by a wide margin.  Two guards keep us safe otherwise:
        //  • callsite signature mismatch => not this EN-SE build (bundled JP sotes.exe self-
        //    voices; an unknown/updated build => don't risk a wrong-address patch): do nothing.
        //  • sound device already up => boot is PAST 0x58113e, the hook can't fire => fall back
        //    to the legacy late seed (dialogue safe; combat only if no actor was built yet).
        unsigned char *cs = (unsigned char *)AP(VA_BOOT_CALLSITE);
        int32_t rel; memcpy(&rel, cs + 1, 4);
        BOOL en_build = (cs[0] == 0xE8 && rel == (int32_t)(VA_BOOT_FN - (VA_BOOT_CALLSITE + 5)));
        void **sounddev = (void **)AP(VA_SOUNDDEV);

        if (GetEnvironmentVariableA("OSS_ENNSE_LATE_SEED", NULL, 0) != 0) {
            vlog("[proxy] OSS_ENNSE_LATE_SEED set — legacy late seed (dialogue only)");
            CreateThread(NULL, 0, seed_thread, NULL, 0, NULL);
        } else if (!en_build) {
            vlog("[proxy] 0x%x not the EN-SE boot call (op=%02x rel=%08x) — no patch applied",
                 VA_BOOT_CALLSITE, cs[0], rel);
        } else if (*sounddev != 0) {
            vlog("[proxy] loaded LATE (sound device up) — early hook would miss; late seed fallback");
            CreateThread(NULL, 0, seed_thread, NULL, 0, NULL);
        } else if (!install_early_hook()) {
            vlog("[proxy] early hook install failed — late seed fallback");
            CreateThread(NULL, 0, seed_thread, NULL, 0, NULL);
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
