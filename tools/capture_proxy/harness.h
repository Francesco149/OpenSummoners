/* harness.h — the background harness thread (port of the agent's window scan,
 * launcher-dialog auto-dismiss, and silent-audio hooks).
 *
 * A dedicated thread (NOT the engine main thread) periodically:
 *   - hides every top-level window this process owns (SW_HIDE) so the run is
 *     headless;
 *   - posts WM_ACTIVATEAPP(TRUE) to the main game window (class
 *     CLASS_LIZSOFT_SOTES) every scan so the engine's WndProc keeps its
 *     DAT_008a952c "app active" flag set — without it FUN_005b1030's pump loop
 *     never breaks out under a hidden window (quirk #29);
 *   - auto-checks "disable sound" and clicks "launch/OK" on the startup config
 *     dialog so the boot is unattended.
 * Silent audio is an IAT clamp of winmm!waveOutSetVolume to 0.
 *
 * Running on a side thread (not the loader-lock'd DllMain) keeps the Win32 UI
 * calls safe.
 */
#ifndef OSS_HARNESS_H
#define OSS_HARNESS_H

#include <windows.h>

#include "iat_hook.h"
#include "proxy_config.h"
#include "proxy_log.h"

#define OSS_MAIN_WND_CLASS "CLASS_LIZSOFT_SOTES"

static int g_main_window_seen = 0;
static int g_activateapp_posts = 0;
static int g_dialog_launch_clicked = 0;

/* winmm!waveOutSetVolume → forward with volume forced to 0 (engine's "audio
 * ready" gates still see success). */
typedef MMRESULT (WINAPI *waveOutSetVolume_t)(HWAVEOUT, DWORD);
static waveOutSetVolume_t real_waveOutSetVolume;
static MMRESULT WINAPI hook_waveOutSetVolume(HWAVEOUT h, DWORD vol)
{
    (void)vol;
    if (real_waveOutSetVolume) return real_waveOutSetVolume(h, 0);
    return MMSYSERR_NOERROR;
}

static BOOL CALLBACK harness_child_cb(HWND hwnd, LPARAM lp)
{
    (void)lp;
    char cls[64], txt[256];
    GetClassNameA(hwnd, cls, sizeof(cls));
    GetWindowTextA(hwnd, txt, sizeof(txt));
    if (lstrcmpiA(cls, "Button") != 0) return TRUE;

    /* lowercase the text for matching */
    char t[256];
    int i = 0;
    for (; txt[i] && i < (int)sizeof(t) - 1; ++i)
        t[i] = (char)((txt[i] >= 'A' && txt[i] <= 'Z') ? txt[i] + 32 : txt[i]);
    t[i] = 0;

    /* BM_GETCHECK/BM_SETCHECK/BM_CLICK/BST_CHECKED come from winuser.h */
    if (g_cfg.silent_audio &&
        (strstr(t, "disable sound") || strstr(t, "no sound") ||
         strstr(t, "mute"))) {
        if (SendMessageA(hwnd, BM_GETCHECK, 0, 0) != (LRESULT)BST_CHECKED) {
            SendMessageA(hwnd, BM_SETCHECK, BST_CHECKED, 0);
            proxy_logf("[harness] checked 'disable sound' button");
        }
    }
    if (!g_dialog_launch_clicked &&
        (strstr(t, "launch") || strstr(t, "start") || strstr(t, "play") ||
         lstrcmpiA(txt, "ok") == 0 || lstrcmpiA(txt, "&ok") == 0)) {
        PostMessageA(hwnd, BM_CLICK, 0, 0);
        g_dialog_launch_clicked = 1;
        proxy_logf("[harness] clicked launch/OK button ('%s')", txt);
    }
    return TRUE;
}

static BOOL CALLBACK harness_top_cb(HWND hwnd, LPARAM lp)
{
    (void)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) return TRUE;

    char cls[64];
    GetClassNameA(hwnd, cls, sizeof(cls));

    /* dismiss the launcher dialog (buttons live as children) */
    if (g_cfg.dismiss_dialog)
        EnumChildWindows(hwnd, harness_child_cb, 0);

    if (lstrcmpiA(cls, OSS_MAIN_WND_CLASS) == 0) {
        if (!g_main_window_seen) {
            g_main_window_seen = 1;
            proxy_logf("[harness] main game window appeared");
        }
        /* keep the engine's app-active flag set under the hidden window */
        PostMessageA(hwnd, 0x001C /*WM_ACTIVATEAPP*/, 1, 0);
        if (g_activateapp_posts < 3) {
            proxy_logf("[harness] posted WM_ACTIVATEAPP(TRUE)");
            g_activateapp_posts++;
        }
    }

    if (g_cfg.hide_window && IsWindowVisible(hwnd))
        ShowWindow(hwnd, SW_HIDE);

    return TRUE;
}

static DWORD WINAPI harness_thread(LPVOID arg)
{
    (void)arg;
    proxy_logf("[harness] thread started");
    if (g_cfg.silent_audio) {
        real_waveOutSetVolume = (waveOutSetVolume_t)iat_hook(
            GetModuleHandleA(NULL), "winmm.dll", "waveOutSetVolume",
            (void *)hook_waveOutSetVolume);
    }
    for (;;) {
        EnumWindows(harness_top_cb, 0);
        Sleep(g_main_window_seen ? 100 : 8);  /* fast until the window shows */
    }
    return 0;
}

static void harness_start(void)
{
    HANDLE h = CreateThread(NULL, 0, harness_thread, NULL, 0, NULL);
    if (h) CloseHandle(h);
    else proxy_logf("[harness] FATAL: CreateThread failed err=%lu",
                    GetLastError());
}

#endif /* OSS_HARNESS_H */
