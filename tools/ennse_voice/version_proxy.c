// tools/ennse_voice/version_proxy.c — proxy `version.dll` for the EN-SE JP-voice patch.
//
// sotes_en.exe imports exactly 3 functions from VERSION.dll
// (GetFileVersionInfoSizeA / GetFileVersionInfoA / VerQueryValueA). Dropped in the
// game dir, this shim is loaded by the exe (app-dir wins for non-KnownDLLs), forwards
// those 3 to the REAL system version.dll, and runs our patch code on attach.
//
// Right now it only proves the injection vector (writes a marker on load). The voice
// seed goes in later (load sotesx_s.dll -> set the engine's bank/manager globals).
//
//   nix develop --command i686-w64-mingw32-gcc -shared -O2 -s \
//     -o build/version.dll tools/ennse_voice/version_proxy.c tools/ennse_voice/version.def

#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef DWORD (WINAPI *pGFVISA)(LPCSTR, LPDWORD);
typedef BOOL  (WINAPI *pGFVIA )(LPCSTR, DWORD, DWORD, LPVOID);
typedef BOOL  (WINAPI *pVQVA  )(LPCVOID, LPCSTR, LPVOID *, PUINT);

static pGFVISA r_size;
static pGFVIA  r_get;
static pVQVA   r_vq;

// Exported (see version.def) — forward to the real system version.dll.
DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR f, LPDWORD h)                 { return r_size ? r_size(f, h) : 0; }
BOOL  WINAPI GetFileVersionInfoA(LPCSTR f, DWORD h, DWORD l, LPVOID d)    { return r_get  ? r_get(f, h, l, d) : FALSE; }
BOOL  WINAPI VerQueryValueA(LPCVOID b, LPCSTR s, LPVOID *p, PUINT u)      { return r_vq   ? r_vq(b, s, p, u) : FALSE; }

static void marker(const char *tag) {
    char host[MAX_PATH] = "?"; GetModuleFileNameA(NULL, host, MAX_PATH);
    char self[MAX_PATH] = "?"; GetModuleFileNameA(GetModuleHandleA("version.dll"), self, MAX_PATH);
    FILE *f = fopen("C:\\Users\\headpats\\oss-ver\\PROXY_LOADED.txt", "a");
    if (f) { fprintf(f, "%s\n  host=%s\n  self=%s\n", tag, host, self); fclose(f); }
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID x) {
    (void)x;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        // Load the REAL version.dll from our own dir as "realver.dll" — a renamed copy,
        // so LoadLibrary doesn't base-name-match (and return) THIS module and recurse.
        // (The installer copies the user's own SysWOW64\version.dll -> realver.dll.)
        char rp[MAX_PATH]; GetModuleFileNameA(h, rp, MAX_PATH);
        char *bs = strrchr(rp, '\\'); if (bs) bs[1] = 0; else rp[0] = 0;
        lstrcatA(rp, "realver.dll");
        HMODULE rv = LoadLibraryA(rp);
        r_size = (pGFVISA)GetProcAddress(rv, "GetFileVersionInfoSizeA");
        r_get  = (pGFVIA )GetProcAddress(rv, "GetFileVersionInfoA");
        r_vq   = (pVQVA  )GetProcAddress(rv, "VerQueryValueA");
        marker("ATTACH");
    }
    return TRUE;
}
