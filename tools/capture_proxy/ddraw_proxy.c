/* ddraw_proxy.c — OpenSummoners trace-studio-v2 capture proxy (M1 skeleton).
 *
 * A proxy ddraw.dll dropped next to the retail sotes(.unpacked).exe.  The exe
 * imports exactly one symbol from DDRAW.dll — DirectDrawCreateEx — and has a
 * fixed ImageBase 0x00400000 with relocations stripped (no ASLR), so:
 *   - this DLL auto-loads (exe-dir precedes the system dir in the search order),
 *     no injector needed;
 *   - later milestones inline-detour engine VAs with zero base-fixup.
 *
 * M1 scope (this file): load + forward only.  Prove the proxy loads into the
 * real game and DirectDrawCreateEx forwards to the system ddraw without
 * regressing the boot.  COM vtable wrapping, the engine-VA detours, the clock
 * hooks and the .osr writer land in later milestones (see
 * docs/plans/trace-studio-v2.md).
 *
 * Build: tools/capture_proxy/Makefile -> build/ddraw_proxy.dll (renamed to
 * ddraw.dll when dropped into the game dir).
 */
#include <windows.h>
#include <ddraw.h>

#include "proxy_log.h"
#include "proxy_config.h"
#include "iat_hook.h"
#include "clock.h"
#include "harness.h"

static LONG g_init_done = 0;

/* One-time proxy init: config + clock IAT hooks + the harness thread.  Run
 * from DllMain (PROCESS_ATTACH) — IAT patches are pure memory writes (loader-
 * lock safe); CreateThread is safe here with DisableThreadLibraryCalls + no
 * wait.  The clock hooks must be live before the engine's pre-DirectDraw
 * busy-waits + the launcher dialog, so DllMain is the right (earliest) place. */
static void proxy_init_once(void)
{
    if (InterlockedExchange(&g_init_done, 1)) return;
    proxy_config_load();
    clock_install();
    harness_start();
    proxy_logf("[proxy] init complete");
}

/* The real system ddraw.dll, resolved lazily by absolute path so we never
 * recurse into ourselves (a bare LoadLibrary("ddraw.dll") would find THIS
 * proxy first, since the exe dir wins the search order). */
static HMODULE g_real_ddraw = NULL;

typedef HRESULT (WINAPI *DirectDrawCreateEx_t)(GUID *, LPVOID *, REFIID,
                                               IUnknown *);
typedef HRESULT (WINAPI *DirectDrawCreate_t)(GUID *, LPDIRECTDRAW *,
                                             IUnknown *);

static HMODULE real_ddraw(void)
{
    if (g_real_ddraw) return g_real_ddraw;
    char path[MAX_PATH];
    UINT n = GetSystemDirectoryA(path, (UINT)sizeof(path));
    /* GetSystemDirectory returns ...\System32; the WoW64 file-system
     * redirector maps a 32-bit process's System32 access to SysWOW64, so
     * this loads the 32-bit ddraw.dll — the right bitness for sotes.exe. */
    if (n == 0 || n >= sizeof(path) - 12) {
        lstrcpynA(path, "C:\\Windows\\System32", sizeof(path));
    }
    lstrcatA(path, "\\ddraw.dll");
    g_real_ddraw = LoadLibraryA(path);
    if (!g_real_ddraw)
        proxy_logf("[proxy] FATAL: LoadLibrary(%s) failed err=%lu",
                   path, GetLastError());
    else
        proxy_logf("[proxy] real ddraw loaded: %s", path);
    return g_real_ddraw;
}

/* Exported as the undecorated "DirectDrawCreateEx" via ddraw_proxy.def. */
HRESULT WINAPI
DirectDrawCreateEx(GUID *lpGUID, LPVOID *lplpDD, REFIID iid,
                   IUnknown *pUnkOuter)
{
    HMODULE m = real_ddraw();
    if (!m) return E_FAIL;
    DirectDrawCreateEx_t fn =
        (DirectDrawCreateEx_t)GetProcAddress(m, "DirectDrawCreateEx");
    if (!fn) {
        proxy_logf("[proxy] FATAL: GetProcAddress(DirectDrawCreateEx) failed");
        return E_FAIL;
    }
    HRESULT hr = fn(lpGUID, lplpDD, iid, pUnkOuter);
    proxy_logf("[proxy] DirectDrawCreateEx -> hr=0x%08lx dd=%p",
               (unsigned long)hr, lplpDD ? *lplpDD : NULL);
    /* M3: wrap *lplpDD's vtable here to intercept CreateSurface/Flip/Blt. */
    return hr;
}

/* The game only imports DirectDrawCreateEx, but export DirectDrawCreate too
 * so the proxy is a drop-in for anything that pokes the classic entry. */
HRESULT WINAPI
DirectDrawCreate(GUID *lpGUID, LPDIRECTDRAW *lplpDD, IUnknown *pUnkOuter)
{
    HMODULE m = real_ddraw();
    if (!m) return E_FAIL;
    DirectDrawCreate_t fn =
        (DirectDrawCreate_t)GetProcAddress(m, "DirectDrawCreate");
    if (!fn) return E_FAIL;
    return fn(lpGUID, lplpDD, pUnkOuter);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)inst; (void)reserved;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        proxy_log_init();
        proxy_logf("[proxy] DLL_PROCESS_ATTACH pid=%lu",
                   (unsigned long)GetCurrentProcessId());
        DisableThreadLibraryCalls(inst);
        proxy_init_once();
        break;
    case DLL_PROCESS_DETACH:
        proxy_logf("[proxy] DLL_PROCESS_DETACH");
        break;
    }
    return TRUE;
}
