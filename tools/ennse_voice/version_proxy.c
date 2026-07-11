// tools/ennse_voice/version_proxy.c — EN-SE JP-voice patch (proxy version.dll).
//
// The EN special edition (sotes_en.exe) still contains the ENTIRE voice subsystem
// (trigger + play, byte-identical to the JP engine); the localizer only removed the
// LOADER (LoadLibrary "sotesx_s.dll" + bank-handle store). The play path is intact
// but dead — gated on two globals that are never set:
//     voice-bank handle  DAT_0092af80   (never written -> null)
//     voice-manager      DAT_0092b76c   (created only if the bank is non-null)
// So this DLL just RE-SEEDS those two globals with the engine's OWN functions, then
// the engine plays voice using its own per-line mapping (read from the byte-identical
// sotesd.dll). See docs/plans/ennse-voice-patch.md.
//
// Deploys as a proxy version.dll. sotes_en.exe imports version.dll (and queries some
// exports dynamically), so ALL 17 exports are forwarded to realver.dll (a renamed copy
// of the user's own SysWOW64\version.dll) via version.def; this .c only runs the seed.
//
// INSTALL (…\steamapps\common\sotes\): version.dll (this) + realver.dll (copy of
// SysWOW64\version.dll) + sotesx_s.dll (the JP voice bank). Then launch normally.
//
//   nix develop --command i686-w64-mingw32-gcc -shared -O2 -s \
//     -o build/version.dll tools/ennse_voice/version_proxy.c tools/ennse_voice/version.def

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

static const char *StrStrIA_(const char *hay, const char *needle);  // defined below

// ─── EN engine (sotes_en.exe) addresses — unpacked ImageBase 0x400000 ────────
// Runtime-relocated via g_delta = actual base - 0x400000 (survives ASLR/rebase).
#define ORIG_BASE      0x400000u
#define VA_BANK_LOAD   0x5d8b10u   // thiscall bank_load(assetmgr, name, a, b) -> bank
#define VA_ASSET_MGR   0x92ac68u   // asset-manager object (direct `this`)
#define VA_OPNEW       0x5ef121u   // cdecl  operator_new(size)
#define VA_MGR_INIT    0x584a70u   // thiscall manager_init(mgr, sounddev, count)
#define VA_BANK_GLOBAL 0x92af80u   // void*  voice bank handle  (seed target #1)
#define VA_MGR_GLOBAL  0x92b76cu   // void*  voice manager      (seed target #2)
#define VA_SOUNDDEV    0x92d5b8u   // void*  sound device (non-null once sound-init ran)

// ── Fix A: monster combat-SE drop (docs/findings/ense-voice-monster-se-drop.md) ──
// Once the voice bank (0x92af80) is set, the SFX registrar 0x59cc8c takes its voice
// branch and SKIPS every sound-def with voice_id==0 — all MONSTER sound sets (unvoiced
// by design) — with no SE fallback, silencing them (Ghost Warlock/Black Harpy/Babymage…).
// Retarget its two `voice_id==0 / 0x7fff -> skip` jumps to the SE branch 0x59cd08 so those
// defs fall back to their SE clip (byte-identical to stock bank-null behaviour).
#define VA_SFX_JE1     0x59ccceu   // rel32 LSB of `je 0x59cd55` @0x59cccc : 0x83 -> 0x36
#define VA_SFX_JE2     0x59ccd8u   // rel8       of `je 0x59cd55` @0x59ccd7 : 0x7c -> 0x2f
#define VA_GATE_OBJ    0x92af98u   // combat-voice gate = (*(*0x92af98))[0x238]  (log only)

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

// Patch one code byte, GUARDED on its expected current value, so a different Steam build
// (different addresses → different bytes) is never corrupted.  Returns 1 on success.
static int patch_code_byte(uintptr_t va, unsigned char expect, unsigned char to) {
    unsigned char *p = (unsigned char *)AP(va);
    DWORD old;
    if (*p != expect) { vlog("[sfxfix] SKIP @%p: have 0x%02x want 0x%02x (build drift?)", (void *)p, *p, expect); return 0; }
    if (!VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old)) { vlog("[sfxfix] VirtualProtect failed @%p", (void *)p); return 0; }
    *p = to;
    VirtualProtect(p, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 1);
    return 1;
}

// Retarget the SFX registrar's two voice_id==0 skips to the SE branch (0x59cd08), so
// unvoiced (monster) sound-defs register their SE clip instead of being dropped.
static void patch_sfx_se_fallback(void) {
    int a = patch_code_byte(VA_SFX_JE1, 0x83, 0x36);
    int b = patch_code_byte(VA_SFX_JE2, 0x7c, 0x2f);
    vlog("[sfxfix] SE-fallback %s (je1=%d je2=%d) — monster combat SE %s",
         (a && b) ? "APPLIED" : "PARTIAL/SKIPPED", a, b, (a && b) ? "restored" : "may stay silent");
}

// Informational: log the combat-voice gate (*(*0x92af98))[0x238] at title.  Fix A is
// robust to any value; this just confirms the routing (0 = voice mode).  It may read
// differently at battle time if the gate turns out to be dynamic.
static void log_combat_voice_gate(void) {
    char *P = *(char **)AP(VA_GATE_OBJ);          // value in global 0x92af98
    char *Q = P ? *(char **)P : 0;                // Q = P[0]
    if (!Q) { vlog("[gate] combat-voice gate unavailable (sound not up yet)"); return; }
    vlog("[gate] combat-voice Q[0x238]=%d at title (0 = voice mode)", *(int *)(Q + 0x238));
}

// Seed the two dead globals so the engine's own (intact) voice code comes alive.
static DWORD WINAPI seed_thread(void *unused) {
    (void)unused;
    void **sounddev = (void **)AP(VA_SOUNDDEV);
    void **bankg    = (void **)AP(VA_BANK_GLOBAL);
    void **mgrg     = (void **)AP(VA_MGR_GLOBAL);

    // Wait for sound-init (device global non-null), then settle at the title screen
    // (main thread idle, asset-manager quiescent) before touching engine state.
    int i = 0;
    while (*sounddev == 0) { if (++i > 3000) { vlog("[seed] TIMEOUT: sound device never came up"); return 0; } Sleep(20); }
    Sleep(600);
    vlog("[seed] sound device up=%p delta=%p", *sounddev, (void *)g_delta);

    if (*bankg == 0) {
        bankload_t bank_load = (bankload_t)AP(VA_BANK_LOAD);
        void *bank = bank_load(AP(VA_ASSET_MGR), "sotesx_s.dll", 1, 1);   // (1,1) = the JP voice-bank args
        *bankg = bank;
        vlog("[seed] bank_load(\"sotesx_s.dll\") -> %p", bank);
    } else {
        vlog("[seed] bank already set: %p", *bankg);
    }

    if (*bankg && *mgrg == 0) {
        opnew_t   opnew    = (opnew_t)AP(VA_OPNEW);
        mgrinit_t mgr_init = (mgrinit_t)AP(VA_MGR_INIT);
        void *mgr = opnew(0xc);
        if (mgr) { *(int *)mgr = 0; mgr_init(mgr, *sounddev, 0x10); *mgrg = mgr; vlog("[seed] manager created -> %p", mgr); }
        else vlog("[seed] operator_new(0xc) FAILED");
    } else if (*mgrg) {
        vlog("[seed] manager already set: %p", *mgrg);
    }
    patch_sfx_se_fallback();     // restore SE for unvoiced (monster) defs the bank load would drop
    log_combat_voice_gate();     // informational — confirm the voice/SE routing
    vlog("[seed] DONE bank=%p mgr=%p  (dialogue+party voice on voiced lines; monster SE via sfxfix)", *bankg, *mgrg);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID x) {
    (void)x;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        // our own dir (game dir) -> place the log next to us
        char dir[MAX_PATH]; GetModuleFileNameA(h, dir, MAX_PATH);
        char *bs = strrchr(dir, '\\'); if (bs) bs[1] = 0; else dir[0] = 0;
        _snprintf(g_logpath, MAX_PATH, "%soss_voice.log", dir);

        char host[MAX_PATH] = ""; GetModuleFileNameA(NULL, host, MAX_PATH);
        vlog("[proxy] attach host=%s", host);
        // Only seed inside the game exe (this shim could be dropped elsewhere).
        char *slash = strrchr(host, '\\'); const char *hn = slash ? slash + 1 : host;
        if (StrStrIA_(hn, "sotes")) {
            g_delta = (uintptr_t)GetModuleHandleA(NULL) - ORIG_BASE;
            CreateThread(NULL, 0, seed_thread, NULL, 0, NULL);
        } else {
            vlog("[proxy] host name has no 'sotes' — seed skipped");
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
