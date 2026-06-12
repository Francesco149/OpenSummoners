/* proxy_config.h — capture-proxy runtime config, read once from the env.
 *
 * The capture driver sets these before spawning the game.  All have sane
 * defaults so a bare boot (no env) still hides + dismisses + turbos.
 */
#ifndef OSS_PROXY_CONFIG_H
#define OSS_PROXY_CONFIG_H

#include <windows.h>
#include <stdlib.h>

#include "proxy_log.h"

typedef struct {
    int  turbo;            /* OSS_TURBO            (default 1) */
    int  lockstep;         /* OSS_LOCKSTEP         (default 1) */
    int  turbo_step_ms;    /* OSS_TURBO_STEP_MS    (default 17) */
    int  lockstep_step_ms; /* OSS_LOCKSTEP_STEP_MS (default 16) */
    int  hide_window;      /* OSS_HIDE_WINDOW      (default 1) */
    int  dismiss_dialog;   /* OSS_DISMISS_DIALOG   (default 1) */
    int  silent_audio;     /* OSS_SILENT_AUDIO     (default 1) */
    int  seed_pin;         /* OSS_SEED_PIN         (default 1) */
    DWORD seed_value;      /* OSS_SEED_VALUE       (default 0x4f5347) */
} proxy_config;

static proxy_config g_cfg;

static int cfg_env_int(const char *name, int dflt)
{
    char buf[32];
    DWORD n = GetEnvironmentVariableA(name, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) return dflt;
    return (int)strtol(buf, NULL, 0);
}

static void proxy_config_load(void)
{
    g_cfg.turbo            = cfg_env_int("OSS_TURBO", 1);
    g_cfg.lockstep         = cfg_env_int("OSS_LOCKSTEP", 1);
    g_cfg.turbo_step_ms    = cfg_env_int("OSS_TURBO_STEP_MS", 17);
    g_cfg.lockstep_step_ms = cfg_env_int("OSS_LOCKSTEP_STEP_MS", 16);
    g_cfg.hide_window      = cfg_env_int("OSS_HIDE_WINDOW", 1);
    g_cfg.dismiss_dialog   = cfg_env_int("OSS_DISMISS_DIALOG", 1);
    g_cfg.silent_audio     = cfg_env_int("OSS_SILENT_AUDIO", 1);
    g_cfg.seed_pin         = cfg_env_int("OSS_SEED_PIN", 1);
    g_cfg.seed_value       = (DWORD)cfg_env_int("OSS_SEED_VALUE", 0x4f5347);
    proxy_logf("[cfg] turbo=%d lockstep=%d step=%d/%d hide=%d dismiss=%d "
               "silent=%d seed_pin=%d seed=0x%lx",
               g_cfg.turbo, g_cfg.lockstep, g_cfg.turbo_step_ms,
               g_cfg.lockstep_step_ms, g_cfg.hide_window, g_cfg.dismiss_dialog,
               g_cfg.silent_audio, g_cfg.seed_pin,
               (unsigned long)g_cfg.seed_value);
}

#endif /* OSS_PROXY_CONFIG_H */
