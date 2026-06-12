/* clock.h — native turbo/lockstep clock virtualization (port of the Frida
 * agent's installTurboHooks, via IAT patching instead of Interceptor).
 *
 * Model (faithful to opensummoners-agent.js):
 *  - GetTickCount is the engine's cadence source (FUN_005b1030 limiter + ~30
 *    scene fns).  Virtualise it ONLY on the main thread and ONLY after the
 *    message pump is entered (pre-pump init has GetTickCount busy-waits that
 *    livelock under an advancing clock).
 *  - turbo: each main-thread GetTickCount call advances the virtual clock by
 *    turbo_step_ms → the engine thinks lots of time passed → fast simulation.
 *  - lockstep: once armed (first Flip), freeze the clock between Flips (the
 *    flip hook advances it one quantum) so retail renders 1 update/present,
 *    matching the port's fixed timestep; a tiny epsilon creep after a long
 *    flipless stall prevents load-settle busy-waits from hanging.
 *  - Sleep → Sleep(0) (yield, don't block).  WaitMessage → return immediately
 *    on the main thread (the pump must busy-spin since real time is frozen).
 *  - PeekMessage/GetMessage entry sets the pump-entered latch.
 *
 * The Flip hook (lockstep advance) is installed by the VA-detour layer; it
 * calls clock_on_flip().
 */
#ifndef OSS_CLOCK_H
#define OSS_CLOCK_H

#include <windows.h>

#include "iat_hook.h"
#include "proxy_config.h"
#include "proxy_log.h"

/* ── state ───────────────────────────────────────────────────────────────── */
static DWORD  g_main_tid = 0;
static volatile LONG g_pump_entered = 0;
static DWORD  g_virtual_now_ms = 0;
static int    g_clock_virtualised = 0;

/* lockstep */
static int    g_lock_armed = 0;
static DWORD  g_lock_clock_ms = 0;
static DWORD  g_lock_calls_since_flip = 0;
static const DWORD LOCKSTEP_STALL_CALLS = 2000;  /* agent default */
static const DWORD LOCKSTEP_EPSILON_MS  = 1;

/* originals */
typedef DWORD (WINAPI *GetTickCount_t)(void);
typedef void  (WINAPI *Sleep_t)(DWORD);
typedef BOOL  (WINAPI *WaitMessage_t)(void);
typedef BOOL  (WINAPI *PeekMessageA_t)(LPMSG, HWND, UINT, UINT, UINT);
typedef BOOL  (WINAPI *GetMessageA_t)(LPMSG, HWND, UINT, UINT);

static GetTickCount_t  real_GetTickCount;
static Sleep_t         real_Sleep;
static WaitMessage_t   real_WaitMessage;
static PeekMessageA_t  real_PeekMessageA;
static GetMessageA_t   real_GetMessageA;

/* Called by the Flip detour each present (lockstep clock advance).  Used by the
 * VA-detour layer (M2b); marked unused so M2a builds clean without it wired. */
__attribute__((unused))
static void clock_on_flip(void)
{
    if (!g_cfg.lockstep) return;
    if (!g_lock_armed) {
        g_lock_armed = 1;
        g_lock_clock_ms = g_virtual_now_ms;  /* continuity from boot/load */
    }
    g_lock_clock_ms += (DWORD)g_cfg.lockstep_step_ms;
    g_lock_calls_since_flip = 0;
}

/* ── replacements ────────────────────────────────────────────────────────── */
static DWORD WINAPI hook_GetTickCount(void)
{
    DWORD tid = GetCurrentThreadId();
    if (g_main_tid == 0) {
        g_main_tid = tid;
        proxy_logf("[clock] main thread = %lu (via GetTickCount)",
                   (unsigned long)tid);
    }
    if (g_clock_virtualised && tid == g_main_tid && g_pump_entered) {
        if (g_cfg.lockstep && g_lock_armed) {
            if (++g_lock_calls_since_flip > LOCKSTEP_STALL_CALLS)
                g_lock_clock_ms += LOCKSTEP_EPSILON_MS;  /* unstick a stall */
            return g_lock_clock_ms;
        }
        g_virtual_now_ms += (DWORD)g_cfg.turbo_step_ms;
        return g_virtual_now_ms;
    }
    return real_GetTickCount();
}

static void WINAPI hook_Sleep(DWORD ms)
{
    (void)ms;
    real_Sleep(0);  /* yield, never block */
}

static BOOL WINAPI hook_WaitMessage(void)
{
    if (g_main_tid != 0 && GetCurrentThreadId() == g_main_tid)
        return TRUE;  /* pump must busy-spin: real time is frozen */
    return real_WaitMessage();
}

static void note_pump_entered(void)
{
    if (!g_pump_entered) {
        InterlockedExchange(&g_pump_entered, 1);
        proxy_logf("[clock] pump entered → clock virtualization armed");
    }
}

static BOOL WINAPI hook_PeekMessageA(LPMSG m, HWND h, UINT a, UINT b, UINT c)
{
    note_pump_entered();
    return real_PeekMessageA(m, h, a, b, c);
}

static BOOL WINAPI hook_GetMessageA(LPMSG m, HWND h, UINT a, UINT b)
{
    note_pump_entered();
    return real_GetMessageA(m, h, a, b);
}

/* ── install ─────────────────────────────────────────────────────────────── */
static void clock_install(void)
{
    HMODULE exe = GetModuleHandleA(NULL);

    if (g_cfg.turbo) {
        real_GetTickCount =
            (GetTickCount_t)iat_hook(exe, "kernel32.dll", "GetTickCount",
                                     (void *)hook_GetTickCount);
        real_Sleep =
            (Sleep_t)iat_hook(exe, "kernel32.dll", "Sleep",
                              (void *)hook_Sleep);
        real_WaitMessage =
            (WaitMessage_t)iat_hook(exe, "user32.dll", "WaitMessage",
                                    (void *)hook_WaitMessage);
        real_PeekMessageA =
            (PeekMessageA_t)iat_hook(exe, "user32.dll", "PeekMessageA",
                                     (void *)hook_PeekMessageA);
        real_GetMessageA =
            (GetMessageA_t)iat_hook(exe, "user32.dll", "GetMessageA",
                                    (void *)hook_GetMessageA);
        if (real_GetTickCount) g_clock_virtualised = 1;
        /* Fallbacks: if an export wasn't in the IAT we must still be able to
         * call the real thing. */
        if (!real_GetTickCount)
            real_GetTickCount = (GetTickCount_t)GetProcAddress(
                GetModuleHandleA("kernel32.dll"), "GetTickCount");
        if (!real_Sleep)
            real_Sleep = (Sleep_t)GetProcAddress(
                GetModuleHandleA("kernel32.dll"), "Sleep");
        if (!real_WaitMessage)
            real_WaitMessage = (WaitMessage_t)GetProcAddress(
                GetModuleHandleA("user32.dll"), "WaitMessage");
        if (!real_PeekMessageA)
            real_PeekMessageA = (PeekMessageA_t)GetProcAddress(
                GetModuleHandleA("user32.dll"), "PeekMessageA");
        if (!real_GetMessageA)
            real_GetMessageA = (GetMessageA_t)GetProcAddress(
                GetModuleHandleA("user32.dll"), "GetMessageA");
        proxy_logf("[clock] turbo hooks installed (virtualised=%d)",
                   g_clock_virtualised);
    }
}

#endif /* OSS_CLOCK_H */
