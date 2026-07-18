// tools/mod_loader/mod_loader.c — generic Fortune Summoners mod loader (proxy version.dll).
//
// The game exe imports version.dll, so Windows loads THIS in its place.  We forward all 17
// version.dll exports to realver.dll (a renamed copy of the user's own SysWOW64\version.dll,
// via version.def) so the exe's imports resolve, then LoadLibrary every *.dll dropped into
// the game's `mods\` folder.  Each mod (the EN-SE trainer, the JP voice patch, ...) is a
// plain DLL with its own DllMain — the loader is GENERIC and carries NO game addresses, so a
// game update never breaks it (only the individual mods pin addresses).
//
// This is the same proven proxy-version.dll injection the JP voice patch used, generalized:
// instead of ONE hard-coded patch it loads a folder of mods.  A bare/static `version.dll`
// resolution loads System32\version.dll, so the drop-in must be named version.dll AND sit
// beside the exe (both true here).
//
// INSTALL (…\steamapps\common\sotes\): version.dll (this) + realver.dll (copy of
// SysWOW64\version.dll) + mods\<yourmod>.dll.  Launch normally.  Log: oss_modloader.log.
//
//   nix develop --command make -C tools/mod_loader     # -> build/version.dll

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

static char g_gamedir[MAX_PATH];   // our own dir (= the game dir), WITH a trailing backslash
static char g_logpath[MAX_PATH];

static void mlog(const char *fmt, ...) {
    FILE *f = fopen(g_logpath, "a"); if (!f) return;
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f); fclose(f);
}

// tiny case-insensitive substring (avoid linking shlwapi for StrStrIA)
static const char *istr(const char *hay, const char *needle) {
    if (!hay || !needle) return NULL;
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) { h++; n++; }
        if (!*n) return hay;
    }
    return NULL;
}

// Load every *.dll in <gamedir>\mods\.  Runs on a worker thread, NEVER in DllMain: calling
// LoadLibrary from DllMain risks a loader-lock deadlock.  Spawned from DLL_PROCESS_ATTACH,
// this thread only starts once the loader lock frees (after the exe's static imports resolve),
// so the mods come up right as the game finishes bootstrapping — early enough for hooks, late
// enough to be safe.  Each mod's own DllMain runs on this thread as it loads.
static DWORD WINAPI loader_thread(void *unused) {
    (void)unused;
    char pat[MAX_PATH];
    _snprintf(pat, MAX_PATH, "%smods\\*.dll", g_gamedir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        mlog("[loader] no mods to load (missing/empty %smods\\)", g_gamedir);
        return 0;
    }
    int n = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        char full[MAX_PATH];
        _snprintf(full, MAX_PATH, "%smods\\%s", g_gamedir, fd.cFileName);
        // LOAD_WITH_ALTERED_SEARCH_PATH: search the MOD's own dir for its dependencies, so a
        // mod can ship co-located libs in mods\ without polluting the game dir.
        HMODULE m = LoadLibraryExA(full, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (m) { mlog("[loader] loaded mods\\%s -> %p", fd.cFileName, (void *)m); ++n; }
        else   { mlog("[loader] FAILED mods\\%s (err %lu)", fd.cFileName, GetLastError()); }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    mlog("[loader] done: %d mod(s) loaded from %smods\\", n, g_gamedir);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID x) {
    (void)x;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        // our own path (we sit beside the exe as version.dll) -> the game dir (trailing '\')
        GetModuleFileNameA(h, g_gamedir, MAX_PATH); g_gamedir[MAX_PATH - 1] = 0;
        char *bs = strrchr(g_gamedir, '\\');
        if (bs) bs[1] = 0; else g_gamedir[0] = 0;
        _snprintf(g_logpath, MAX_PATH, "%soss_modloader.log", g_gamedir);

        char host[MAX_PATH] = ""; GetModuleFileNameA(NULL, host, MAX_PATH);
        mlog("[loader] attach host=%s dir=%s", host, g_gamedir);
        // Only load inside the game exe (this shim could be dropped beside another app).
        char *slash = strrchr(host, '\\'); const char *hn = slash ? slash + 1 : host;
        if (istr(hn, "sotes")) {
            CreateThread(NULL, 0, loader_thread, NULL, 0, NULL);
        } else {
            mlog("[loader] host name has no 'sotes' — mod load skipped");
        }
    }
    return TRUE;
}
