// tools/ennse_voice/ennse_voice.c — EN-SE JP-voice patch (a mod_loader mod).
//
// The EN special edition (sotes_en.exe) still contains the ENTIRE voice subsystem
// (trigger + play, byte-identical to the JP engine); the localizer only removed the
// LOADER (LoadLibrary "sotesx_s.dll" + bank-handle store). The play path is intact
// but dead — gated on two globals that are never set:
//     voice-bank handle  DAT_0092af80   (never written -> null)
//     voice-manager      DAT_0092b76c   (created only if the bank is non-null)
// So this mod just RE-SEEDS those two globals with the engine's OWN functions, then
// the engine plays voice using its own per-line mapping (read from the byte-identical
// sotesd.dll). See docs/plans/ennse-voice-patch.md.
//
// Ships as a plain mod DLL loaded by ../mod_loader (the generic proxy version.dll).  The
// loader forwards the real version.dll and LoadLibrarys every mods\*.dll — this mod is
// mods\ennse_voice.dll; it only runs the seed (no export forwarding of its own anymore).
//
// INSTALL (…\steamapps\common\sotes\): version.dll (mod_loader) + realver.dll (copy of
// SysWOW64\version.dll) + mods\ennse_voice.dll (this) + sotesx_s.dll (the JP voice bank).
// The patch's one-liner installs all of it.  Then launch normally.
//
//   nix develop --command make -C tools/ennse_voice     # -> build/ennse_voice.dll

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
    vlog("[seed] DONE bank=%p mgr=%p  (voice should now play on voiced lines)", *bankg, *mgrg);
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
