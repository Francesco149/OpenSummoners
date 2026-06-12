/* proxy_log.h — tiny file logger for the ddraw proxy.
 *
 * The proxy runs inside the retail process on Windows; we have no console.
 * Log to a file the WSL side can read via /mnt/c.  Path comes from the env
 * var OSS_PROXY_LOG (set by the capture driver), else a temp default.  All
 * logging is best-effort and never throws / never blocks the engine.
 */
#ifndef OSS_PROXY_LOG_H
#define OSS_PROXY_LOG_H

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

static CRITICAL_SECTION g_log_cs;
static int g_log_cs_ready = 0;
static char g_log_path[MAX_PATH] = {0};

static void proxy_log_init(void)
{
    if (g_log_cs_ready) return;
    InitializeCriticalSection(&g_log_cs);
    g_log_cs_ready = 1;
    DWORD n = GetEnvironmentVariableA("OSS_PROXY_LOG", g_log_path,
                                      (DWORD)sizeof(g_log_path));
    if (n == 0 || n >= sizeof(g_log_path)) {
        /* default: <temp>\oss_proxy.log */
        char tmp[MAX_PATH];
        DWORD t = GetTempPathA((DWORD)sizeof(tmp), tmp);
        if (t == 0 || t >= sizeof(tmp))
            lstrcpynA(g_log_path, "C:\\oss_proxy.log", sizeof(g_log_path));
        else {
            lstrcpynA(g_log_path, tmp, sizeof(g_log_path));
            lstrcatA(g_log_path, "oss_proxy.log");
        }
    }
}

static void proxy_logf(const char *fmt, ...)
{
    if (!g_log_cs_ready) proxy_log_init();
    EnterCriticalSection(&g_log_cs);
    FILE *f = fopen(g_log_path, "ab");
    if (f) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fputc('\n', f);
        fclose(f);
    }
    LeaveCriticalSection(&g_log_cs);
}

#endif /* OSS_PROXY_LOG_H */
