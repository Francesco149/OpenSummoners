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
#include <ddraw.h>

#include "proxy_config.h"
#include "proxy_log.h"
#include "clock.h"
#include "va_detour.h"
#include "trampoline.h"
#include "osr_writer.h"
#include "render_id.h"
#include "surface_id.h"
#include "engine_pixfmt.h"
#include "sheet_grab.h"
#include "blend_grab.h"

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
#define EH_ZDD_DTOR_VA    0x005b9390u   /* zdd_object dtor — Releases +0xac/+0x2c */
#define EH_ZDD_CLEAR_VA   0x005b9410u   /* zdd_object clear — Lock + zero-fill    */

/* cel (zdd_object) field offsets — confirmed in the blit decompiles + src/zdd.h */
#define CEL_METRIC_0C   0x0cu   /* placement origin x (keyed dst-left bias) */
#define CEL_METRIC_10   0x10u   /* placement origin y                       */
#define CEL_COLORKEY    0x28u   /* bound color key (DDCOLORKEY)             */
#define CEL_METRIC_B8   0xb8u   /* source width                            */
#define CEL_METRIC_BC   0xbcu   /* source height                           */
#define CEL_STATE       0xd4u   /* state flag (0x8000 = KEYSRC armed)       */
#define BLIT_SURF_PTR   0x2cu   /* cel/dest +0x2c = IDirectDrawSurface7* (M3c) */
#define CEL_COM_BACK    0xacu   /* zdd_object +0xac = back/attached surface  */

/* ── observable counters (read later by the .osr emitter) ─────────────────── */
static volatile LONG g_eh_flip     = 0;
static volatile LONG g_eh_sim_tick = 0;

/* ── frame-stream state (M3b: FRAMEBEG-at-open / draws / PRESENT-at-flip) ──── */
static int g_eh_frame_open = 0;    /* a FRAMEBEG has opened the current frame   */
static int g_eh_blit_seq   = 0;    /* draw ordinal within the current frame     */

/* M3d: the single dst (backbuffer) surface handle, learned from the blit path
 * (M3c found exactly one distinct dst surface).  GDI text renders onto that same
 * backbuffer DC, so engine_gdi.h stamps TEXT records with this handle.  Text also
 * SHARES the per-frame draw ordinal (g_eh_blit_seq) so the replayer interleaves
 * text and blits in issue order — both go through eh_next_draw_seq().  Accessors
 * keep engine_gdi.h decoupled from these statics (it is included after us). */
static uint32_t g_eh_backbuffer_handle = 0;
static void    *g_eh_backbuffer_surf   = NULL;  /* M4d: the dest surface ptr */
static uint32_t eh_next_draw_seq(void)     { return (uint32_t)g_eh_blit_seq++; }
static uint32_t eh_backbuffer_handle(void) { return g_eh_backbuffer_handle; }

/* The .osr header is written UNKNOWN/640x480 at file-open (before any surface
 * exists).  The first dest surface we touch is the backbuffer, whose desc gives
 * the real screen pixfmt + dims; correct the header once (the bg writer thread
 * re-stamps offset 0 — osr_w_fixup_header). */
static int g_eh_hdr_fixed = 0;
static void eh_fixup_header_from(void *surf)
{
    if (g_eh_hdr_fixed || !surf) return;
    DDSURFACEDESC2 dd;
    memset(&dd, 0, sizeof(dd));
    dd.dwSize = sizeof(dd);
    if (FAILED(IDirectDrawSurface7_GetSurfaceDesc((LPDIRECTDRAWSURFACE7)surf, &dd)))
        return;
    g_eh_hdr_fixed = 1;
    osr_w_fixup_header(eh_pixfmt_of(&dd.ddpfPixelFormat),
                       (uint16_t)dd.dwWidth, (uint16_t)dd.dwHeight);
    proxy_logf("[hook] header fixup: %lux%lu pixfmt=%u (RGBbits=%lu)",
               dd.dwWidth, dd.dwHeight, eh_pixfmt_of(&dd.ddpfPixelFormat),
               (unsigned long)dd.ddpfPixelFormat.dwRGBBitCount);
}

/* ── seed/rng pin state (mirrors the agent latches) ──────────────────────── */
static int g_eh_seed_pinned     = 0;   /* one-shot: the title sparkle pin */
static int g_eh_rng_anchor_armed = 0;  /* set at game_enter (if seed_pin) */
static int g_eh_rng_anchored    = 0;   /* per-map: first 0x41f200 only */

static DWORD eh_read_seed(void)
{
    return *(volatile DWORD *)EH_SEED_ADDR;
}

/* ── M4d: the real-backbuffer SNAP (the --validate ground truth) ────────────
 * At flip entry the engine has finished the closing frame's draws (blits + GDI
 * text) and is about to present, so the dest surface holds EXACTLY the pixels
 * the reconstructor must reproduce for that frame.  Lock it READONLY (nothing
 * is writing it now) and stream a SNAP record labeled with the closing frame's
 * flip; in-stream it sits just before that frame's PRESENT, so the validating
 * reconstructor compares at exactly that point.  Sampled (every-N / explicit
 * flips) — a snap is ~600 KB, so a capture carries only a handful.
 * Only frames WITH draws are snapped: an empty re-present frame's backbuffer
 * content depends on the flip-chain rotation (quirk #99), which the recon's
 * single accumulating dest intentionally does not model. */
static LONG g_eh_n_snaps = 0;
static int eh_snap_wanted(uint32_t closing_flip)
{
    if (g_cfg.snap_every > 0 &&
        (closing_flip % (uint32_t)g_cfg.snap_every) == 0) return 1;
    for (int i = 0; i < g_cfg.n_snap_flips; i++)
        if (g_cfg.snap_flips[i] == closing_flip) return 1;
    return 0;
}
static void eh_snap_backbuffer(uint32_t closing_flip)
{
    LPDIRECTDRAWSURFACE7 s = (LPDIRECTDRAWSURFACE7)g_eh_backbuffer_surf;
    if (!s) return;
    DDSURFACEDESC2 dd;
    memset(&dd, 0, sizeof(dd));
    dd.dwSize = sizeof(dd);
    if (FAILED(IDirectDrawSurface7_Lock(
            s, NULL, &dd, DDLOCK_READONLY | DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,
            NULL)) || !dd.lpSurface) {
        proxy_logf("[snap] flip %lu: backbuffer Lock FAILED", (unsigned long)closing_flip);
        return;
    }
    size_t bytes = (size_t)dd.lPitch * dd.dwHeight;
    if (bytes <= 8u * 1024u * 1024u) {
        osr_snap sn;
        memset(&sn, 0, sizeof(sn));
        sn.flip     = closing_flip;
        sn.sim_tick = (uint32_t)g_eh_sim_tick;
        sn.w        = (uint16_t)dd.dwWidth;
        sn.h        = (uint16_t)dd.dwHeight;
        sn.pitch    = (uint32_t)dd.lPitch;
        sn.pixfmt   = eh_pixfmt_of(&dd.ddpfPixelFormat);
        sn.codec    = 0;
        sn.byte_len = (uint32_t)bytes;
        sn.bytes    = (const uint8_t *)dd.lpSurface;
        osr_w_snap(&sn);
        LONG t = InterlockedIncrement(&g_eh_n_snaps);
        if (t <= 4 || (t & 15) == 0)
            proxy_logf("[snap] #%ld @flip %lu (%lux%lu, %u KB)", t,
                       (unsigned long)closing_flip, dd.dwWidth, dd.dwHeight,
                       (unsigned)(bytes / 1024u));
    }
    IDirectDrawSurface7_Unlock(s, NULL);
}

/* (ckpt 158: the OSS_FORCE_SWORD hook was REMOVED — it was built on the bogus
 * "errands quest case 8 gates the sword" theory.  In fact Z draws on a normal fresh
 * new game with NO quest progress; clamping weapon+0xd4=2 every flip actually JAMMED
 * the natural draw, so only the Z key failed in injected captures.  The clean
 * recording sword2.osr was made with OSS_FORCE_SWORD=0 + OSS_TURBO=0 OSS_LOCKSTEP=0.) */

/* ── M8 (chase #3): the freeroam player BODY state (wx / hvel) ──────────────
 * Re-drive retail off the RECORDED input with the state pass on, so sync_diff can
 * compare the port's accel ramps wx-vs-wx / vel-vs-vel — CAMERA-INDEPENDENT.  The
 * screen-x residual is camera-hidden (frame-lock-1to1.md "Chase #3 diagnosis"):
 * the walk/dash accel happens at wx>30000 where the follow-camera pins Arche at
 * screen ~270 on both sides, so a real accel divergence only shows on the wx axis.
 *
 * Walk the leader chain to Arche's body (tools/flow/freeroam_mover_fields.json):
 *   p0 = *(0x8a9b50);  p1 = *(p0+0x2784) room_state;  p2 = *(p1+0x200c) leader-slot;
 *   entity = *(p2+0x9f4);  body = *(entity+0x40).
 * Then wx = *(body+4), hvel = *(body+0x28) — the HORIZONTAL accumulator the port's
 * fr_vel mirrors (NB the field-spec's +0x18 "vel" is the VERTICAL one).  The chain
 * is null until a freeroam room is live, so guard EVERY deref (VirtualQuery via
 * bg_readable) — a wild deref inside the flip callback would crash the game. */
#define EH_PLAYER_ROOT_VA 0x008a9b50u
static DWORD eh_player_body(void)   /* 0 until a freeroam room is live */
{
    DWORD p = EH_PLAYER_ROOT_VA;
    static const DWORD hops[5] = { 0u, 0x2784u, 0x200cu, 0x9f4u, 0x40u };
    for (int i = 0; i < 5; i++) {
        DWORD a = p + hops[i];
        if (!bg_readable((const void *)(uintptr_t)a, 4)) return 0;
        p = *(DWORD *)(uintptr_t)a;
        if (!p) return 0;
    }
    return p;                       /* Arche's body struct */
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
    if (g_eh_frame_open) {
        /* M4d: snapshot the real backbuffer for the frame this flip closes
         * (label f-1), positioned just before its PRESENT. */
        uint32_t closing = (uint32_t)(f - 1);
        if (g_eh_blit_seq > 0 && eh_snap_wanted(closing))
            eh_snap_backbuffer(closing);
        osr_w_present(0, 0);
    }
    osr_w_framebeg((uint32_t)f, (uint32_t)g_eh_sim_tick, 0);
    /* M8: the opt-in engine STATE (rng census) right after FRAMEBEG.  rng = the
     * LCG state word read at the flip; retail rngcalls (the per-draw count) is a
     * follow-up — it needs a 0x5bf505 trampoline counter, PORT-DEBT(osr-state-
     * rngcalls-retail).  The port already emits rngcalls; the panel shows it
     * port-side with retail "-" until the proxy counter lands. */
    if (g_cfg.state_on) {
        osr_state_field sf[4];
        memset(sf, 0, sizeof sf);
        uint32_t n = 0;
        snprintf(sf[n].name, sizeof sf[n].name, "rng");
        sf[n].kind = OSR_ST_HEX;
        sf[n].ival = (int64_t)(uint32_t)eh_read_seed();
        n++;
        /* chase #3: the freeroam player body wx/hvel/facing (0 fields until a
         * room is live — the chain derefs are null-guarded; emit only when the
         * body AND its +0x2c are readable so the diff joins wx-vs-wx). */
        DWORD body = eh_player_body();
        if (body && bg_readable((const void *)(uintptr_t)(body + 0x2c), 4)) {
            snprintf(sf[n].name, sizeof sf[n].name, "wx");
            sf[n].kind = OSR_ST_INT;
            sf[n].ival = (int64_t)*(int32_t *)(uintptr_t)(body + 4u);
            n++;
            snprintf(sf[n].name, sizeof sf[n].name, "hvel");
            sf[n].kind = OSR_ST_INT;
            sf[n].ival = (int64_t)*(int32_t *)(uintptr_t)(body + 0x28u);
            n++;
            snprintf(sf[n].name, sizeof sf[n].name, "facing");
            sf[n].kind = OSR_ST_INT;
            sf[n].ival = (int64_t)*(int32_t *)(uintptr_t)(body + 0x2cu);
            n++;
        }
        osr_w_state(sf, n);
    }
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
/* The hot hooks (resolver + 5 blits) are E9-trampolined (trampoline.h), so they
 * take the light `(ecx, entry_esp)` signature, NOT the VEH PCONTEXT: ecx = the
 * thiscall `this`, entry_esp = the function-entry esp (return addr at [esp]). */
static void eh_resolver_cb(DWORD slot, DWORD esp)
{
    (void)esp;
    if (!slot) return;
    DWORD frame = (DWORD)(*(int32_t *)(esp + 4u)) & 0xffffu;
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
/* stack arg i (0-based, past the return address) given the function-entry esp */
#define EH_STK(esp, i) (*(int32_t *)((esp) + 4u + (DWORD)(i) * 4u))

/* dest_arg = the blit's stack arg0 (the paint_ctx); its +0x2c is the dest
 * IDirectDrawSurface7 (uniform across all 5 primitives — see the decompiles). */
static void eh_blit_record(uint32_t va, uint32_t mode, DWORD cel, DWORD dest_arg,
                           int32_t dx, int32_t dy, int32_t reqw, int32_t reqh,
                           int32_t sx, int32_t sy, int32_t bmode, uint32_t ckey,
                           uint32_t blend_ref, int32_t srcw, int32_t srch)
{
    osr_blit b;
    memset(&b, 0, sizeof(b));
    b.va = va;
    b.seq = (uint32_t)g_eh_blit_seq++;
    b.mode = mode;
    b.bmode = bmode;
    b.ckey = ckey;
    b.blend_ref = blend_ref;
    b.dx = dx; b.dy = dy; b.reqw = reqw; b.reqh = reqh; b.sx = sx; b.sy = sy;
    b.srcw = srcw; b.srch = srch;
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
    /* M3c: the dest surface → a stable handle; the first one fixes the header. */
    void *dst_surf = dest_arg ? *(void **)(dest_arg + BLIT_SURF_PTR) : NULL;
    b.dst_handle = surfid_get(dst_surf);
    if (b.dst_handle) {
        g_eh_backbuffer_handle = b.dst_handle;  /* M3d: TEXT dst */
        g_eh_backbuffer_surf   = dst_surf;      /* M4d: the SNAP Lock target */
    }
    if (!g_eh_hdr_fixed) eh_fixup_header_from(dst_surf);
    /* M3c-2: the source surface → its dedup'd SHEET (grabbed once); dhash refs it. */
    void *src_surf = cel ? *(void **)(cel + BLIT_SURF_PTR) : NULL;
    b.dhash = sheet_capture_source(src_surf, res, frame);
    osr_w_blit(&b);
}

/* mode 0/1: __thiscall ECX=cel, stack (dest, x, y); dst extent = cel +0xb8/+0xbc */
static void eh_blt_onto_cb(DWORD cel, DWORD esp)
{
    eh_blit_record(EH_BLT_ONTO_VA, 0, cel, EH_STK(esp, 0),
                   EH_STK(esp, 1), EH_STK(esp, 2),
                   cel ? *(int32_t *)(cel + CEL_METRIC_B8) : 0,
                   cel ? *(int32_t *)(cel + CEL_METRIC_BC) : 0,
                   0, 0, -1, cel ? *(uint32_t *)(cel + CEL_COLORKEY) : 0, 0, 0, 0);
}
static void eh_blt_keyed_cb(DWORD cel, DWORD esp)
{
    eh_blit_record(EH_BLT_KEYED_VA, 1, cel, EH_STK(esp, 0),
                   EH_STK(esp, 1), EH_STK(esp, 2),
                   cel ? *(int32_t *)(cel + CEL_METRIC_B8) : 0,
                   cel ? *(int32_t *)(cel + CEL_METRIC_BC) : 0,
                   0, 0, -1, cel ? *(uint32_t *)(cel + CEL_COLORKEY) : 0, 0, 0, 0);
}
/* mode 2: __thiscall ECX=cel, (dest, dst_x, dst_y, dst_w, dst_h, src_x, src_y,
 * src_w, src_h).  All 8 coords recorded — srcw/srch carry args 7/8 (src_w !=
 * dst_w means a SCALING Blt; the reconstructor passes them to blt_rects). */
static void eh_blt_rects_cb(DWORD cel, DWORD esp)
{
    eh_blit_record(EH_BLT_RECTS_VA, 2, cel, EH_STK(esp, 0),
                   EH_STK(esp, 1), EH_STK(esp, 2), EH_STK(esp, 3), EH_STK(esp, 4),
                   EH_STK(esp, 5), EH_STK(esp, 6), -1,
                   cel ? *(uint32_t *)(cel + CEL_COLORKEY) : 0, 0,
                   EH_STK(esp, 7), EH_STK(esp, 8));
}
/* mode 3: __thiscall ECX=cel, (dest, dst_x, dst_y, width, height, src_x, src_y) — RAW pre-clip */
static void eh_blt_clip_cb(DWORD cel, DWORD esp)
{
    eh_blit_record(EH_BLT_CLIP_VA, 3, cel, EH_STK(esp, 0),
                   EH_STK(esp, 1), EH_STK(esp, 2), EH_STK(esp, 3), EH_STK(esp, 4),
                   EH_STK(esp, 5), EH_STK(esp, 6), -1,
                   cel ? *(uint32_t *)(cel + CEL_COLORKEY) : 0, 0, 0, 0);
}
/* mode 4: __cdecl (dest, cel, dst_x, dst_y, width, height, src_x, src_y, colorkey, gdi_ctx).
 * The blend descriptor is the __thiscall `this` (ecx) forwarded to FUN_005bd680
 * (mode at +0x0); ckey is the explicit colorkey arg, not a cel field.  ecx-as-desc
 * is best-effort (guarded against a wild deref) — verify the blend modes are the
 * valid 0/1/2 (quirk #44) against a real capture; refine in M3c if not. */
static void eh_blt_alpha_cb(DWORD desc_ecx, DWORD esp)
{
    /* PORT-DEBT(osr-alpha-src-grab): mode 4 is a GDI BitBlt+blend over a paint_ctx
     * source (0x5bd550/0x5bd680), so cel+0x2c may not be the blend's true pixel
     * source — the SHEET grab here is best-effort; dst_handle + geometry are exact. */
    DWORD cel  = (DWORD)EH_STK(esp, 1);
    int32_t bmode = (desc_ecx >= 0x10000u) ? *(int32_t *)desc_ecx : -1;
    /* M4 alpha: grab the blend descriptor (mode + per-channel LUTs) → blend_ref;
     * the reconstructor rebuilds the zdd_blend_desc from it.  Dedup'd by ptr. */
    uint32_t blend_ref = blend_capture((const void *)desc_ecx);
    eh_blit_record(EH_BLT_ALPHA_VA, 4, cel, EH_STK(esp, 0),
                   EH_STK(esp, 2), EH_STK(esp, 3), EH_STK(esp, 4), EH_STK(esp, 5),
                   EH_STK(esp, 6), EH_STK(esp, 7), bmode, (uint32_t)EH_STK(esp, 8),
                   blend_ref, 0, 0);
}

/* ── zdd_object dtor → surface-cache eviction (the stale-sheet fix, ckpt 126) ──
 * FUN_005b9390 Releases the object's +0xac (com_back) then +0x2c (com_primary).
 * A LATER surface allocated at a recycled pointer must NOT hit the old
 * ptr→dhash / ptr→handle cache entries — that staleness made house-freeroam
 * blits reference town-era sheets (white dialog panels / the wrong sprite).
 * Evict both pointers at dtor entry (before the Releases zero them).  INT3+VEH
 * is fine here: destruction is a scene-transition burst, not per-frame-hot. */
static LONG g_eh_dtor_evicted = 0;
static void eh_zdd_dtor_cb(PCONTEXT ctx)
{
    DWORD obj = ctx->Ecx;
    if (!obj) return;
    void *prim = *(void **)(obj + BLIT_SURF_PTR);
    void *back = *(void **)(obj + CEL_COM_BACK);
    int n = 0;
    if (prim) { n += sheet_grab_evict(prim); surfid_evict(prim); }
    if (back) { n += sheet_grab_evict(back); surfid_evict(back); }
    /* M4d: never SNAP-Lock a destroyed surface */
    if (prim && prim == g_eh_backbuffer_surf) g_eh_backbuffer_surf = NULL;
    if (back && back == g_eh_backbuffer_surf) g_eh_backbuffer_surf = NULL;
    if (n) {
        LONG t = InterlockedIncrement(&g_eh_dtor_evicted);
        if (t <= 8 || (t & 63) == 0)
            proxy_logf("[hook] zdd dtor evicted blitted-from surface "
                       "#%ld @flip %ld", t, g_eh_flip);
    }
}

/* ── zdd_object clear → an ORDERED CLEAR draw (the flip-800 validate fix) ────
 * FUN_005b9410 Locks +0x2c and zero-fills pitch×height.  The engine clears its
 * compose surface at scene transitions (newgame menu / prologue entry) and the
 * title redraw — WITHOUT this record the reconstructor accumulates stale pixels
 * under any scene that doesn't fully redraw (the menu-over-title artifact).
 * Only the BACKBUFFER clear is a frame op; the engine also zero-fills offscreen
 * panel sheets through the same fn (those are re-grabbed as SHEETs), so filter
 * to the tracked compose surface.  INT3 is fine: ~1/frame at the title, bursts
 * at transitions. */
static LONG g_eh_n_clears = 0;
static void eh_zdd_clear_cb(PCONTEXT ctx)
{
    DWORD obj = ctx->Ecx;
    if (!obj) return;
    void *surf = *(void **)(obj + BLIT_SURF_PTR);
    if (!surf || surf != g_eh_backbuffer_surf) return;
    osr_w_clear(eh_next_draw_seq(), surfid_get(surf), 0);
    LONG t = InterlockedIncrement(&g_eh_n_clears);
    if (t <= 4 || (t & 1023) == 0)
        proxy_logf("[hook] backbuffer CLEAR #%ld @flip %ld", t, g_eh_flip);
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
    detour_add(EH_ZDD_DTOR_VA,   eh_zdd_dtor_cb);   /* sheet/surfid eviction */
    detour_add(EH_ZDD_CLEAR_VA,  eh_zdd_clear_cb);  /* backbuffer CLEAR record */
    /* M3b: the render-id resolver + the 5 blit primitives (the draw stream) are
     * E9-TRAMPOLINED, not INT3'd — they fire hundreds×/frame and the INT3+VEH
     * 2-exceptions/call is the in-game fps wall.  Head bytes are hardcoded from
     * the unpacked exe (instruction-aligned, head_len>=5, no rel jmp/call inside
     * the relocated span — verified by disasm). */
    static const uint8_t HEAD_RESOLVER[5] = { 0x56, 0x8b, 0xf1, 0x8b, 0x06 };
    static const uint8_t HEAD_ONTO[6]  = { 0x8b, 0x51, 0x2c, 0x83, 0xec, 0x10 };
    static const uint8_t HEAD_KEYED[6] = { 0x8b, 0x51, 0x2c, 0x83, 0xec, 0x10 };
    static const uint8_t HEAD_RECTS[6] = { 0x8b, 0x51, 0x2c, 0x83, 0xec, 0x20 };
    static const uint8_t HEAD_CLIP[6]  = { 0x83, 0xec, 0x24, 0x8b, 0x41, 0x2c };
    static const uint8_t HEAD_ALPHA[8] = { 0x83, 0xec, 0x08, 0x56, 0x8b, 0x74, 0x24, 0x34 };
    trampoline_add(EH_RESOLVER_VA,  eh_resolver_cb,  HEAD_RESOLVER, 5);
    trampoline_add(EH_BLT_ONTO_VA,  eh_blt_onto_cb,  HEAD_ONTO,  6);
    trampoline_add(EH_BLT_KEYED_VA, eh_blt_keyed_cb, HEAD_KEYED, 6);
    trampoline_add(EH_BLT_RECTS_VA, eh_blt_rects_cb, HEAD_RECTS, 6);
    trampoline_add(EH_BLT_CLIP_VA,  eh_blt_clip_cb,  HEAD_CLIP,  6);
    trampoline_add(EH_BLT_ALPHA_VA, eh_blt_alpha_cb, HEAD_ALPHA, 8);
    proxy_logf("[hook] engine-VA detour+trampoline layer installed "
               "(%d INT3 + 6 E9)", g_detour_n);
}

#endif /* OSS_ENGINE_HOOKS_H */
