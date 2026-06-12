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
#include "render_id.h"

/* ── engine VAs (absolute; base 0x400000, relocations stripped) ───────────── */
#define EH_FLIP_VA        0x005b8fc0u
#define EH_SIM_TICK_VA    0x0043d1d0u
#define EH_SPAWN_VA       0x0056c070u
#define EH_SEED_ADDR      0x008a4f94u   /* DAT_008a4f94 — the LCG seed word */
#define EH_NEWGAME_VA     0x00564780u
#define EH_PROLOGUE_VA    0x0056cd20u
#define EH_GAME_ENTER_VA  0x0059f2c0u
#define EH_RNG_ANCHOR_VA  0x0041f200u

/* ── M3b: the render-id resolver + the 5 source-bearing blit primitives ───── */
#define EH_RESOLVER_VA    0x00418470u   /* ar_sprite_slot_frame (cel registry)  */
#define EH_BLT_ONTO_VA    0x005b9a40u   /* mode 0  __thiscall ECX=cel           */
#define EH_BLT_KEYED_VA   0x005b9b70u   /* mode 1  __thiscall ECX=cel           */
#define EH_BLT_RECTS_VA   0x005b9ae0u   /* mode 2  __thiscall ECX=cel           */
#define EH_BLT_CLIP_VA    0x005b9bf0u   /* mode 3  __thiscall ECX=cel           */
#define EH_BLT_ALPHA_VA   0x005bd550u   /* mode 4  __cdecl    cel=arg[1]         */

/* cel (zdd_object) field offsets — confirmed in the blit decompiles + src/zdd.h */
#define CEL_METRIC_0C   0x0cu   /* placement origin x (keyed dst-left bias) */
#define CEL_METRIC_10   0x10u   /* placement origin y                       */
#define CEL_COLORKEY    0x28u   /* bound color key (DDCOLORKEY)             */
#define CEL_METRIC_B8   0xb8u   /* source width                            */
#define CEL_METRIC_BC   0xbcu   /* source height                           */
#define CEL_STATE       0xd4u   /* state flag (0x8000 = KEYSRC armed)       */

/* ── observable counters (read later by the .osr emitter) ─────────────────── */
static volatile LONG g_eh_flip     = 0;
static volatile LONG g_eh_sim_tick = 0;

/* ── frame-stream state (M3b: FRAMEBEG-at-open / draws / PRESENT-at-flip) ──── */
static int g_eh_frame_open = 0;    /* a FRAMEBEG has opened the current frame   */
static int g_eh_blit_seq   = 0;    /* draw ordinal within the current frame     */

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
    /* M3b frame structure: a FRAMEBEG opens a frame, the blit detours stream
     * BLIT records into it, and the NEXT flip closes it with a PRESENT before
     * opening the following frame.  So the draws listed under FRAMEBEG(f) are
     * the ops the engine issued after flip f, presented by flip f+1.  The
     * tick-join uses sim_tick (the deterministic axis), so this one-flip offset
     * between a frame's label and its present is immaterial to pairing; the
     * port emitter (M5) mirrors the same present-then-framebeg order. */
    if (g_eh_frame_open) osr_w_present(0, 0);
    osr_w_framebeg((uint32_t)f, (uint32_t)g_eh_sim_tick, 0);
    g_eh_frame_open = 1;
    g_eh_blit_seq   = 0;                    /* draws restart at 0 each frame */
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

/* ── M3b: the render-id resolver hook ─────────────────────────────────────
 * FUN_00418470 (__thiscall: ECX = bank slot, arg0 = frame): registers the
 * returned cel's identity.  The cel is the resolver's RETURN value, but it is
 * computable AT onEnter from the decompile —
 *   cel = *( *( *(int*)slot ) + (frame & 0xffff) * 4 )
 * — whenever the bank is already decoded (the `*(int*)*in_ECX != 0` guard at
 * 0x418478).  On the very first resolve of a bank the array pointer is still 0
 * (the call lazily decodes it via FUN_004184a0); we skip that one and register
 * the cel on its next resolve (banks decode early, cels persist, and every cel
 * blitted in steady state has been re-resolved with the bank decoded).  This
 * keeps M3b on the proven onEnter-only detour framework — no onLeave needed. */
static void eh_resolver_cb(PCONTEXT ctx)
{
    DWORD slot = ctx->Ecx;
    if (!slot) return;
    DWORD frame = (DWORD)(*(int32_t *)((DWORD)ctx->Esp + 4u)) & 0xffffu;
    DWORD p0 = *(DWORD *)slot;             /* *in_ECX */
    if (!p0) return;
    DWORD frames = *(DWORD *)p0;           /* *(int*)*in_ECX — the cel array */
    if (!frames) return;                   /* undecoded — registers on re-resolve */
    DWORD cel = *(DWORD *)(frames + frame * 4u);
    if (!cel) return;
    uint16_t res = *(uint16_t *)(slot + 0x40u);
    rid_register(cel, res, (uint16_t)frame);
}

/* ── M3b: the 5 source-bearing blit primitives → BLIT records ─────────────── */
/* stack arg i (0-based, past the return address) at a detour's onEnter */
#define EH_STK(ctx, i) (*(int32_t *)((DWORD)(ctx)->Esp + 4u + (DWORD)(i) * 4u))

static void eh_blit_record(uint32_t va, uint32_t mode, DWORD cel,
                           int32_t dx, int32_t dy, int32_t reqw, int32_t reqh,
                           int32_t sx, int32_t sy, int32_t bmode, uint32_t ckey)
{
    osr_blit b;
    memset(&b, 0, sizeof(b));
    b.va = va;
    b.seq = (uint32_t)g_eh_blit_seq++;
    b.mode = mode;
    b.bmode = bmode;
    b.ckey = ckey;
    b.dx = dx; b.dy = dy; b.reqw = reqw; b.reqh = reqh; b.sx = sx; b.sy = sy;
    uint16_t res = 0, frame = 0;
    rid_lookup(cel, &res, &frame);
    b.res = res; b.frame = frame;
    if (cel) {
        b.ow    = *(int32_t  *)(cel + CEL_METRIC_B8);
        b.oh    = *(int32_t  *)(cel + CEL_METRIC_BC);
        b.ox    = *(int32_t  *)(cel + CEL_METRIC_0C);
        b.oy    = *(int32_t  *)(cel + CEL_METRIC_10);
        b.state = *(uint32_t *)(cel + CEL_STATE);
    }
    /* dhash + dst_handle stay 0 retail-side until M3c grabs surface pixels. */
    osr_w_blit(&b);
}

/* mode 0/1: __thiscall ECX=cel, stack (dest, x, y); dst extent = cel +0xb8/+0xbc */
static void eh_blt_onto_cb(PCONTEXT ctx)
{
    DWORD cel = (DWORD)ctx->Ecx;
    eh_blit_record(EH_BLT_ONTO_VA, 0, cel, EH_STK(ctx, 1), EH_STK(ctx, 2),
                   cel ? *(int32_t *)(cel + CEL_METRIC_B8) : 0,
                   cel ? *(int32_t *)(cel + CEL_METRIC_BC) : 0,
                   0, 0, -1, cel ? *(uint32_t *)(cel + CEL_COLORKEY) : 0);
}
static void eh_blt_keyed_cb(PCONTEXT ctx)
{
    DWORD cel = (DWORD)ctx->Ecx;
    eh_blit_record(EH_BLT_KEYED_VA, 1, cel, EH_STK(ctx, 1), EH_STK(ctx, 2),
                   cel ? *(int32_t *)(cel + CEL_METRIC_B8) : 0,
                   cel ? *(int32_t *)(cel + CEL_METRIC_BC) : 0,
                   0, 0, -1, cel ? *(uint32_t *)(cel + CEL_COLORKEY) : 0);
}
/* mode 2: __thiscall ECX=cel, (dest, dst_x, dst_y, dst_w, dst_h, src_x, src_y, …) */
static void eh_blt_rects_cb(PCONTEXT ctx)
{
    DWORD cel = (DWORD)ctx->Ecx;
    eh_blit_record(EH_BLT_RECTS_VA, 2, cel,
                   EH_STK(ctx, 1), EH_STK(ctx, 2), EH_STK(ctx, 3), EH_STK(ctx, 4),
                   EH_STK(ctx, 5), EH_STK(ctx, 6), -1,
                   cel ? *(uint32_t *)(cel + CEL_COLORKEY) : 0);
}
/* mode 3: __thiscall ECX=cel, (dest, dst_x, dst_y, width, height, src_x, src_y) — RAW pre-clip */
static void eh_blt_clip_cb(PCONTEXT ctx)
{
    DWORD cel = (DWORD)ctx->Ecx;
    eh_blit_record(EH_BLT_CLIP_VA, 3, cel,
                   EH_STK(ctx, 1), EH_STK(ctx, 2), EH_STK(ctx, 3), EH_STK(ctx, 4),
                   EH_STK(ctx, 5), EH_STK(ctx, 6), -1,
                   cel ? *(uint32_t *)(cel + CEL_COLORKEY) : 0);
}
/* mode 4: __cdecl (dest, cel, dst_x, dst_y, width, height, src_x, src_y, colorkey, gdi_ctx).
 * The blend descriptor is the __thiscall `this` (ECX) forwarded to FUN_005bd680
 * (mode at +0x0); ckey is the explicit colorkey arg, not a cel field.  ECX-as-desc
 * is best-effort (guarded against a wild deref) — verify the blend modes are the
 * valid 0/1/2 (quirk #44) against a real capture; refine in M3c if not. */
static void eh_blt_alpha_cb(PCONTEXT ctx)
{
    DWORD cel  = (DWORD)EH_STK(ctx, 1);
    DWORD desc = (DWORD)ctx->Ecx;
    int32_t bmode = (desc >= 0x10000u) ? *(int32_t *)desc : -1;
    eh_blit_record(EH_BLT_ALPHA_VA, 4, cel,
                   EH_STK(ctx, 2), EH_STK(ctx, 3), EH_STK(ctx, 4), EH_STK(ctx, 5),
                   EH_STK(ctx, 6), EH_STK(ctx, 7), bmode, (uint32_t)EH_STK(ctx, 8));
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
    /* M3b: the render-id resolver + the 5 blit primitives (the draw stream). */
    detour_add(EH_RESOLVER_VA,   eh_resolver_cb);
    detour_add(EH_BLT_ONTO_VA,   eh_blt_onto_cb);
    detour_add(EH_BLT_KEYED_VA,  eh_blt_keyed_cb);
    detour_add(EH_BLT_RECTS_VA,  eh_blt_rects_cb);
    detour_add(EH_BLT_CLIP_VA,   eh_blt_clip_cb);
    detour_add(EH_BLT_ALPHA_VA,  eh_blt_alpha_cb);
    proxy_logf("[hook] engine-VA detour layer installed (%d hooks)",
               g_detour_n);
}

#endif /* OSS_ENGINE_HOOKS_H */
