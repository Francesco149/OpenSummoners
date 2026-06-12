/* engine_hooks.h — the trace-studio-v2 engine-VA detour layer (M2b).
 *
 * Faithful native port of the Frida agent's VA hooks (opensummoners-agent.js),
 * installed via the INT3+VEH framework in va_detour.h (zero base-fixup — the exe
 * base is the fixed 0x400000).  Each hook mirrors the agent's onEnter:
 *
 *   0x5b8fc0  FLIP_VA      present/flip   → flip++  + lockstep clock advance
 *   0x43d1d0  SIM_TICK_VA  scene easer    → sim_tick++  (the deterministic index,
 *                                           quirk #75; reset to 0 at game_enter)
 *   0x56c070  SPAWN_VA     first sparkle  → ONE-SHOT seed pin DAT_008a4f94
 *   0x564780  newgame_enter anchor
 *   0x56cd20  prologue_enter anchor
 *   0x59f2c0  game_enter   anchor → sim_tick:=0, arm the per-map RNG re-pin
 *   0x41f200  RNG_ANCHOR   EFFECT band activator → per-map seed re-pin (latched)
 *
 * Input injection (0x43c110 ring / 0x5ba520 held leaf) lands in engine_input.h.
 *
 * The seed-pin/anchor logic is the agent's: pin the LCG seed once at the title
 * sparkle, then RE-pin per map at the EFFECT activator so retail's town spawn
 * burst starts from the same seed the port replays from (engine-quirk #86).
 */
#ifndef OSS_ENGINE_HOOKS_H
#define OSS_ENGINE_HOOKS_H

#include <windows.h>

#include "proxy_config.h"
#include "proxy_log.h"
#include "clock.h"
#include "va_detour.h"
#include "osr_writer.h"

/* ── engine VAs (absolute; base 0x400000, relocations stripped) ───────────── */
#define EH_FLIP_VA        0x005b8fc0u
#define EH_SIM_TICK_VA    0x0043d1d0u
#define EH_SPAWN_VA       0x0056c070u
#define EH_SEED_ADDR      0x008a4f94u   /* DAT_008a4f94 — the LCG seed word */
#define EH_NEWGAME_VA     0x00564780u
#define EH_PROLOGUE_VA    0x0056cd20u
#define EH_GAME_ENTER_VA  0x0059f2c0u
#define EH_RNG_ANCHOR_VA  0x0041f200u

/* ── observable counters (read later by the .osr emitter) ─────────────────── */
static volatile LONG g_eh_flip     = 0;
static volatile LONG g_eh_sim_tick = 0;

/* ── seed/rng pin state (mirrors the agent latches) ──────────────────────── */
static int g_eh_seed_pinned     = 0;   /* one-shot: the title sparkle pin */
static int g_eh_rng_anchor_armed = 0;  /* set at game_enter (if seed_pin) */
static int g_eh_rng_anchored    = 0;   /* per-map: first 0x41f200 only */

static DWORD eh_read_seed(void)
{
    return *(volatile DWORD *)EH_SEED_ADDR;
}

/* ── flip: frame boundary + the lockstep clock advance ───────────────────── */
static DWORD g_eh_hb_t0 = 0;   /* real GetTickCount at flip 1 (throughput base) */
static void eh_flip_cb(PCONTEXT ctx)
{
    (void)ctx;
    LONG f = InterlockedIncrement(&g_eh_flip);
    clock_on_flip();                       /* arm + step the lockstep clock */
    /* M3a: one (flip, sim_tick) frame record per present — the tick-join axis.
     * M3c restructures this to FRAMEBEG-at-open / draws / PRESENT-at-flip once
     * the blit detours emit draws between flips. */
    osr_w_framebeg((uint32_t)f, (uint32_t)g_eh_sim_tick, 0);
    osr_w_present(0, 0);
    if (f <= 3)
        proxy_logf("[hook] flip #%ld (sim_tick=%ld)", f, g_eh_sim_tick);
    if (f == 1 && real_GetTickCount) g_eh_hb_t0 = real_GetTickCount();
    if ((f % 2000) == 0 && real_GetTickCount) {
        DWORD dt = real_GetTickCount() - g_eh_hb_t0;
        proxy_logf("[hook] flip %ld @ %lu ms real (%lu fps, sim_tick=%ld)",
                   f, (unsigned long)dt,
                   dt ? (unsigned long)(f * 1000UL / dt) : 0UL, g_eh_sim_tick);
    }
}

/* ── sim-tick: the deterministic frame-of-reference index ────────────────── */
static void eh_sim_tick_cb(PCONTEXT ctx)
{
    (void)ctx;
    InterlockedIncrement(&g_eh_sim_tick);
}

/* ── seed pin: one-shot write at the first title sparkle ─────────────────── */
static void eh_seed_pin_cb(PCONTEXT ctx)
{
    (void)ctx;
    if (g_eh_seed_pinned) { detour_request_disarm(); return; }
    g_eh_seed_pinned = 1;
    if (g_cfg.seed_pin) {
        DWORD before = eh_read_seed();
        *(volatile DWORD *)EH_SEED_ADDR = g_cfg.seed_value;
        osr_w_seed((uint32_t)g_eh_flip, before, g_cfg.seed_value);
        proxy_logf("[hook] seed PINNED @flip %ld: DAT_008a4f94 0x%lx -> 0x%lx",
                   g_eh_flip, (unsigned long)before,
                   (unsigned long)g_cfg.seed_value);
    }
    detour_request_disarm();               /* stop trapping every sparkle */
}

/* ── per-map RNG re-pin at the EFFECT band activator ─────────────────────── */
static void eh_rng_anchor_cb(PCONTEXT ctx)
{
    (void)ctx;
    if (!g_eh_rng_anchor_armed || g_eh_rng_anchored) return;
    g_eh_rng_anchored = 1;
    if (g_cfg.seed_pin) {
        DWORD before = eh_read_seed();
        *(volatile DWORD *)EH_SEED_ADDR = g_cfg.seed_value;
        osr_w_seed((uint32_t)g_eh_flip, before, g_cfg.seed_value);
        proxy_logf("[hook] RNG re-pin @flip %ld (game_enter spawn): "
                   "0x%lx -> 0x%lx", g_eh_flip, (unsigned long)before,
                   (unsigned long)g_cfg.seed_value);
    }
}

/* ── scene anchors (assertions / alignment points) ───────────────────────── */
static void eh_emit_anchor(const char *name)
{
    proxy_logf("[anchor] %s flip=%ld sim_tick=%ld rng=0x%lx",
               name, g_eh_flip, g_eh_sim_tick, (unsigned long)eh_read_seed());
    osr_w_anchor((uint32_t)g_eh_flip, (uint32_t)g_eh_sim_tick,
                 eh_read_seed(), name);
}

static void eh_newgame_cb(PCONTEXT ctx)  { (void)ctx; eh_emit_anchor("newgame_enter"); }
static void eh_prologue_cb(PCONTEXT ctx) { (void)ctx; eh_emit_anchor("prologue_enter"); }

static void eh_game_enter_cb(PCONTEXT ctx)
{
    (void)ctx;
    /* sim_tick counts easer calls SINCE game_enter (quirk #75) — reset so it is a
     * load-anchored, cross-run index the diff joins on. */
    InterlockedExchange(&g_eh_sim_tick, 0);
    /* arm the town SPAWN re-pin: the seed write happens at the first 0x41f200,
     * NOT here (effects mutate the LCG between game_enter and that activator). */
    g_eh_rng_anchor_armed = g_cfg.seed_pin;
    g_eh_rng_anchored = 0;
    eh_emit_anchor("game_enter");
}

/* ── install all of M2b's onEnter hooks ──────────────────────────────────── */
static void engine_hooks_install(void)
{
    detour_init();
    detour_add(EH_FLIP_VA,       eh_flip_cb);
    detour_add(EH_SIM_TICK_VA,   eh_sim_tick_cb);
    detour_add(EH_SPAWN_VA,      eh_seed_pin_cb);
    detour_add(EH_NEWGAME_VA,    eh_newgame_cb);
    detour_add(EH_PROLOGUE_VA,   eh_prologue_cb);
    detour_add(EH_GAME_ENTER_VA, eh_game_enter_cb);
    detour_add(EH_RNG_ANCHOR_VA, eh_rng_anchor_cb);
    proxy_logf("[hook] engine-VA detour layer installed (%d hooks)",
               g_detour_n);
}

#endif /* OSS_ENGINE_HOOKS_H */
