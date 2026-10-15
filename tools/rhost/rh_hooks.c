/* rh_hooks.c — port functions vs retail on IDENTICAL state, inside the running game.
 *
 *   --hook shadow:NAME[@N]   retail runs (authoritative); the port re-runs on the pre-state
 *                            and every Nth call (default 1) is compared; divergences logged
 *   --hook replace:NAME      the port runs INSTEAD of retail (hybrid mode)
 * Hooks are call-site rewrites (all direct E8 calls to the VA). A port function never calls
 * its own retail VA. Add a function = one wrapper + one table row.
 *
 * Kinds:
 *   RET    pure function of a few words: compare return + the listed state (rand: seed word)
 *   FIELDS compare return + the exact fields the function may write (input_poll: ring ids)
 * Report lines (parsed by rhost.py verify):
 *   hook-result shadow NAME calls=C diverge=D skipped=S [first: ...] */
#include "rhost.h"
#include "rng.h"
#include "input.h"
#include "collision.h"
#include "entity_move.h"
#include "map_data.h"
#include "map_decode.h"
#include "camera_follow.h"
#include "char_ai.h"
#include "char_physics.h"
#include "char_integrate.h"
#include "npc_ai.h"
#include "char_apply.h"
#include "world_tick.h"
#include "entity_fx.h"
#include "actor_update.h"
#include "particle_tick.h"
#include "char_behave.h"
#include "behave_ops.h"
#include "sound_fx.h"
#include "entity_life.h"
#include "char_state.h"
#include "actor_ops.h"
#include "particle_spawn.h"
#include "grid_refresh.h"
#include "particle_init.h"
#include "gimmick.h"
#include "event.h"
#include "room_loop.h"
#include "input_dik.h"
#include "room_load.h"
#include "world_alloc.h"
#include "sprite_slot.h"
#include "device_init.h"
#include "char_def.h"
#include "char_init.h"
#include "party_member.h"
#include "scene_loop.h"
#include "engine_calls.h"

typedef struct {
    const char *name; uint32_t va; void *wrapper;
    int mode;              /* 0 off, 1 shadow, 2 replace */
    uint32_t every;        /* shadow sampling */
    long calls, compared, diverge, skipped; char first[256];
} hook_t;

/* the character definition helpers + the character init (src/char_def.c, src/char_init.c) */
#define CDEF_HOOKS(X) \
    X(H_CD_SKFIND, "char_skill_find", 0x4072c0u) X(H_CD_GPROBE, "char_ground_probe", 0x41e280u) \
    X(H_CD_VSFREE, "voice_set_free", 0x426380u) X(H_CD_VSSTOP, "voice_set_stop", 0x41e440u) \
    X(H_CD_TODOFF, "tod_off", 0x426420u) X(H_CD_GF1C, "grid_f1c", 0x426480u) \
    X(H_CD_JITTER, "stat_jitter", 0x426e00u) X(H_CD_STATS, "char_stats_init", 0x426fd0u) \
    X(H_CD_IDLEI, "char_idle_init", 0x427360u) X(H_CD_IDLEA, "char_idle_add", 0x4273c0u) \
    X(H_CD_MSKILL, "char_member_skill_add", 0x4273f0u) X(H_CD_COMBO, "char_combo_add", 0x427430u) \
    X(H_CD_CLIPS, "char_clips_set", 0x427580u) X(H_CD_CLIPS2, "char_clips2_set", 0x4275f0u) \
    X(H_CD_CARRY, "char_carry_set", 0x427610u) X(H_CD_STEP, "char_step_set", 0x427650u) \
    X(H_CD_PACE, "char_pace_set", 0x427670u) X(H_CD_PACER, "char_pace_raw", 0x427b70u) \
    X(H_CD_C880, "char_c880_set", 0x427c00u) X(H_CD_MOVE, "char_move_set", 0x427c30u) \
    X(H_CD_15A5C, "char_15a5c_set", 0x427c90u) X(H_CD_TURN, "char_turn_set", 0x427cc0u) \
    X(H_CD_WALK, "char_walk_set", 0x427ce0u) X(H_CD_JUMP, "char_jump_set", 0x427d30u) \
    X(H_CD_RUN, "char_run_set", 0x427dc0u) X(H_CD_HIT, "char_hit_set", 0x427df0u) \
    X(H_CD_MPOSE, "char_member_pose_set", 0x427e40u) X(H_CD_CROUCH, "char_crouch_set", 0x427e70u) \
    X(H_CD_NAVSK, "char_nav_skills_set", 0x427eb0u) X(H_CD_APPR, "char_approach_add", 0x427f00u) \
    X(H_CD_SKCODE, "char_skills_by_code", 0x427fb0u) X(H_CD_LEARN, "char_skill_learn", 0x4282f0u) \
    X(H_CD_SKDEF, "char_skill_def", 0x4285f0u) X(H_CD_NAV, "char_nav_set", 0x428780u) \
    X(H_CD_RETGT, "char_retarget_set", 0x4287b0u) X(H_CD_AISK, "char_ai_skill_add", 0x4287d0u) \
    X(H_CD_BOXES, "ai_skill_boxes_add", 0x428950u) X(H_CD_VOICE, "char_voice_init", 0x4289f0u) \
    X(H_CD_NPCDEF, "char_npc_default", 0x431930u) X(H_CD_FORM0, "char_form0_set", 0x431b90u) \
    X(H_CD_PMODE, "char_party_mode_set", 0x431ba0u) X(H_CD_IVOICE, "char_idle_voice_set", 0x431cb0u) \
    X(H_PM_CREATE, "member_create", 0x58e170u) X(H_PM_ATTACH, "member_attach", 0x5b4530u) \
    X(H_PM_CODE, "pslot_code", 0x4094d0u) X(H_PM_PAL, "member_palette", 0x5b4a10u) \
    X(H_PM_SKREF, "char_skills_refresh", 0x406ec0u) X(H_PM_USABLE, "char_skill_usable", 0x407320u) \
    X(H_PM_SKAT, "member_skill_at", 0x4071f0u) X(H_SS_EFREE, "slot_entries_free", 0x417b50u) \
    X(H_SS_CLONE, "slot_clone", 0x4179b0u) X(H_CTRL_FREE, "ctrl_geometry_free", 0x40e0c0u) \
    X(H_SL_OBJ, "scene_obj_init", 0x41a7f0u) X(H_SL_FFIND, "flag_find", 0x4c57f0u) X(H_SL_FADD, "flag_add", 0x4c5830u) \
    X(H_SL_RFLAG, "room_flag_set", 0x560e90u) X(H_SL_PSRESET, "pslot_reset", 0x560e60u) \
    X(H_SL_PDEF, "room_party_defaults", 0x5612b0u) X(H_SL_INALLOC, "room_inputs_alloc", 0x5611d0u) \
    X(H_SL_RDEL, "room_delete", 0x5614e0u) X(H_SL_AFIND, "area_find", 0x561c90u) X(H_SL_ININIT, "input_init", 0x5851e0u) \
    X(H_SL_AENTER, "room_area_enter", 0x59e1a0u) X(H_SL_FCLR, "fill_clear_mapped", 0x5a3bb0u) \
    X(H_SL_HASRET, "room_has_retry", 0x5a3bf0u) X(H_SL_RETPT, "room_retry_point", 0x5a3c10u) \
    X(H_SL_KFIND, "keep_find", 0x5a3de0u) X(H_SL_NEXT, "room_next_set", 0x4e61a0u) X(H_SL_HEAL, "room_party_heal", 0x4e1950u) \
    X(H_SL_GOLD, "room_gold_add", 0x419c00u) X(H_SL_RLNEW, "roomloop_new", 0x5a3c40u) \
    X(H_SL_FROMSC, "room_from_scene", 0x561150u) X(H_SL_ODEL, "obj_delete", 0x5a4460u) X(H_SL_OFREE, "obj_free", 0x5a4480u) \
    X(H_SL_PSIN, "pslot_input_set", 0x4cc5a0u) X(H_SL_KSNAP, "keep_snapshot", 0x5a3e20u) \
    X(H_SL_ENDCLN, "room_end_cleanup", 0x5a3ff0u) X(H_SL_RLPOOLS, "roomloop_pools_free", 0x5a4200u) \
    X(H_SL_RLFREE, "roomloop_free", 0x585f00u) X(H_SL_SEND, "session_end", 0x562ba0u) X(H_SL_XREF, "area_xref", 0x585000u) \
    X(H_SL_FCACHE, "frame_cache_clear", 0x562a70u) X(H_SL_FREAD, "file_read_section", 0x5b6300u) \
    X(H_SL_POBJDEL, "pool_obj_delete", 0x562e60u) X(H_SL_WDEL, "world_delete", 0x562e40u) \
    X(H_SL_TOSC, "room_to_scene", 0x55f790u) X(H_SL_SSTART, "scene_start", 0x59cfc0u) \
    X(H_PA_BYID, "pslot_by_id", 0x413b20u) X(H_PA_DRAM, "dramatist_row", 0x409090u) X(H_PA_ALV, "area_level_row", 0x41dc40u) \
    X(H_PA_HEAL, "pslot_heal", 0x408d40u) X(H_PA_LEVEL, "room_party_level", 0x408dc0u) X(H_PA_INIT, "pslot_init", 0x4c5e00u) \
    X(H_PA_SKL, "member_skills_learn", 0x4c6160u) X(H_PA_GRANT, "room_items_grant", 0x4c6990u) \
    X(H_PA_LEAD, "room_leader_set", 0x4c6830u) X(H_PA_STORY, "room_story_setup", 0x4c5350u) \
    X(H_PA_STORY2, "room_story_setup2", 0x4e59a0u) X(H_PA_SKROW, "skill_row_find", 0x429d00u) \
    X(H_PA_FITS, "item_fits", 0x416420u) X(H_PA_STAT, "pslot_status_set", 0x4e6030u) X(H_PA_REVIVE, "pslot_revive_mark", 0x4c6530u) \
    X(H_PA_ISTATS, "item_stats_add", 0x413a00u) X(H_PA_REFRESH, "room_stats_refresh", 0x413810u) \
    X(H_PA_EQUIP, "room_item_equip", 0x416570u) X(H_PA_DISMISS, "room_party_dismiss", 0x4e6070u) \
    X(H_PA_LEAVE, "room_member_leave", 0x4c63a0u) X(H_PA_JOIN, "room_member_join", 0x4c6550u) \
    X(H_PA_ADD, "room_member_add", 0x4cc820u) X(H_PA_IADD, "room_item_add", 0x415e20u) \
    X(H_PA_SROW, "stat_row_find", 0x4090e0u) X(H_PA_PROF, "char_profile_find", 0x4c6110u) \
    X(H_PA_RECOMP, "stats_recompute", 0x4066d0u) X(H_PFACE, "part_facing", 0x4842c0u) \
    X(H_CINIT, "char_init", 0x41f200u)
#define CDEF_ENUM(id, n, va) id,

static hook_t H[];
enum { H_RAND, H_POLL, H_MOVER_V, H_SWEEP_H, H_ENT_MOVE_H, H_ENT_MOVE_V, H_MAP_DECODE, H_CAM_FOLLOW, H_CAM_SHAKE, H_CHAR_AI, H_CHAR_PHYS, H_CHAR_ANIM, H_CHAR_INT, H_HAZARD, H_NPC_AI, H_CHAR_APPLY, H_WORLD_TICK,
       H_FX_ADD, H_FX_TICK, H_SPLASH, H_SND_RELEASE, H_SND_ALL, H_SND_ONE, H_ACTOR, H_PARTICLE, H_BEHAVE,
       H_STEP_OK, H_PICK_CHAR, H_WALK_TO, H_PACE, H_PART_CX, H_PART_CY,
       H_VP_LOAD, H_VP_TAKE, H_SFX_PLACE, H_SFX_PLAY, H_SFX_AT, H_VOICE_PLACE, H_VOICE_PLAY, H_SND_START,
       H_VOICE_CUE, H_VS_TICK, H_VLINE_STOP, H_VS_SOUNDING,
       H_UNLIST, H_RELEASE, H_ACTOR_REL, H_CHAR_REL, H_SET_COLL, H_CARRY, H_LIST_RM,
       H_ST_STUCK, H_GRID_B, H_PIT, H_RESPAWN, H_MODS, H_AI_RESET, H_SLOWED, H_LOOKUP, H_TARGETABLE, H_SND_LOOP, H_SFX_JIT,
       H_FORCE_ACT, H_GRID_C, H_ADD_HP, H_ADD_MP,
       H_AMBIENT, H_IDLE_VOICE, H_SCATTER, H_SYS_MODE,
       H_CRATE, H_BREAK, H_SLIDE_V, H_RIDERS, H_DUST, H_ANCHOR, H_SPAWN, H_TRACK, H_GRID_REFRESH, H_GRID_MARK,
       H_PINIT, H_ESETUP, H_RIDE, H_RIDE_PART, H_POLAR_DIV, H_POLAR_MILLI, H_FADE_A, H_FADE_B, H_TRACK_ON, H_SND_ALLOC, H_JITTER,
       H_PARTS_FREE, H_SET_ANGLE, H_FX_RESET, H_SKILL_AVAIL, H_PARTY_FIND, H_FLOOR_BELOW,
       H_GPASS, H_GSTEP, H_SWSET, H_SWLINK, H_EVRUN, H_UNLOAD, H_UNLIST_ALL, H_GLOW, H_RECT_HIT, H_CHARS_COUNT, H_CHAR_IN, H_RIDING,
       H_PART_FIND, H_SERIAL, H_SHOT, H_WARP, H_GATHER, H_TPASS, H_TACTOR, H_RFLUSH, H_PROBE_RECT, H_NEAR,
       H_ZPASS, H_ZHIT, H_B96_NEAR, H_RIN, H_RCHAR, H_RACT, H_RANIM, H_RDOWN, H_RLEAD, H_TSOUNDS, H_DIK, H_ENC, H_FOES, H_PFOES, H_PSLOT, H_BGM, H_RLOOP, H_REPOLL,
       H_LROOM, H_BODIES, H_LINKS, H_VPOOLS, H_SHAKECLR, H_VFADE, H_MAPID, H_MAPNPC, H_PSCLR, H_GATE, H_SREADY, H_STINT, H_TSFADE, H_FILL,
       H_WNEW, H_WFREE, H_EFREE, H_AFREE, H_CFREE, H_CBLK, H_WLEAD, H_CSNAP, H_ZRESET, H_QSTATE, H_VREC, H_RSAVE, H_MFREE, H_MDEL, H_PHDEL,
       H_FORM, H_FOLLOW, H_ASTATE, H_PTINT, H_MLOAD,
       H_SFREE, H_SFREEALL, H_SGRADED, H_PSWAP, H_POPULATE, H_SINIT, H_SETH, H_DINIT, H_SNDFREE,
       CDEF_HOOKS(CDEF_ENUM) H_SLOOP, H_N };

static void diverged(hook_t *h, const char *fmt, ...)
{
    h->diverge++;
    if (h->diverge == 1) {
        va_list ap; va_start(ap, fmt); vsnprintf(h->first, sizeof h->first, fmt, ap); va_end(ap);
        rh_log("DIVERGE %s frame %u: %s\n", h->name, g_frame, h->first);
    }
}
static int sample(hook_t *h)
{
    h->calls++;
    if (rh_in_shadow()) return 0;      /* reached from inside a full-state shadow: pass through */
    return h->every <= 1 || h->calls % h->every == 0;
}

/* A shadow call runs retail AND the port on the retail stack: the port leaves its own garbage
 * below ESP, which retail's uninitialized reads (quirk #123) would then see — a pure-retail run
 * never would. Save the dead stack right after the retail call and put it back after the port
 * ran, so the stack is exactly what retail left. */
#define DEAD_BYTES 16384
static uint8_t g_dead[DEAD_BYTES];
static void __attribute__((noinline)) dead_save(uint32_t below)
{ memcpy(g_dead, (void *)(uintptr_t)(below - DEAD_BYTES), DEAD_BYTES); }
static void __attribute__((noinline)) dead_restore(uint32_t below)
{ memcpy((void *)(uintptr_t)(below - DEAD_BYTES), g_dead, DEAD_BYTES); }
#define SP() ({ uint32_t _s; __asm__ volatile("mov %%esp, %0" : "=r"(_s)); _s; })

/* ---- rand (MSVC LCG, FUN_005bf505) vs src/rng.c ---- */
typedef uint32_t (__cdecl *rand_t)(void);
static uint32_t __cdecl w_rand(void)
{
    hook_t *h = &H[H_RAND];
    uint32_t seed = g_ed->rng_seed, s0 = G32(seed);
    if (h->mode == 2) { h->calls++; rng_srand(s0); uint32_t r = rng_rand(); G32(seed) = rng_peek_state(); return r; }
    uint32_t r = ((rand_t)(uintptr_t)h->va)();
    if (sample(h)) {
        uint32_t s1 = G32(seed), sp = SP() - 256;
        dead_save(sp);
        rng_srand(s0); uint32_t p = rng_rand(), ps = rng_peek_state();
        dead_restore(sp);
        h->compared++;
        if (p != r || ps != s1) diverged(h, "seed %08x: retail %u/%08x port %u/%08x", s0, r, s1, p, ps);
    }
    return r;
}

/* ---- input_poll_consume (FUN_0043c110, __thiscall(mgr, now, id)) vs src/input.c ----
 * writes only rec->id of the matching ring record: snapshot all 64 records' ids */
typedef int (__attribute__((thiscall)) *poll_t)(void *, uint32_t, int32_t);
static int __attribute__((thiscall)) w_poll(void *mgr, uint32_t now, int32_t id)
{
    hook_t *h = &H[H_POLL];
    input_mgr *m = mgr;
    if (h->mode == 2) { h->calls++; return input_poll_consume(m, now, id); }
    if (!sample(h)) return ((poll_t)(uintptr_t)h->va)(mgr, now, id);
    int32_t pre[INPUT_RING_LEN], post_r[INPUT_RING_LEN];
    for (int i = 0; i < INPUT_RING_LEN; i++) pre[i] = m->ring[i] ? m->ring[i]->id : 0;
    int r = ((poll_t)(uintptr_t)h->va)(mgr, now, id);
    uint32_t sp = SP() - 256;
    dead_save(sp);
    for (int i = 0; i < INPUT_RING_LEN; i++) { post_r[i] = m->ring[i] ? m->ring[i]->id : 0; if (m->ring[i]) m->ring[i]->id = pre[i]; }
    int p = input_poll_consume(m, now, id);
    dead_restore(sp);
    int bad = -1;
    for (int i = 0; i < INPUT_RING_LEN; i++) {
        int32_t pv = m->ring[i] ? m->ring[i]->id : 0;
        if (pv != post_r[i] && bad < 0) bad = i;
        if (m->ring[i]) m->ring[i]->id = post_r[i];    /* retail stays authoritative */
    }
    h->compared++;
    if (p != r || bad >= 0) diverged(h, "id %d now %u: retail ret %d port ret %d first ring diff slot %d", id, now, r, p, bad);
    return r;
}

/* ---- tile movers (FUN_0054e990 / FUN_0054ded0, thiscall on the grid object) vs src/collision.c ----
 * a slope cell's region-B +0x8 is a pointer to a 32-byte height profile in retail .rdata: the port's
 * resolver callback reads the byte straight out of the mapped image. The __int16 flags are passed
 * through as raw stack dwords (retail reads only the low half). */
static int rh_slope(void *ctx, uint32_t sref, int sub) { (void)ctx; return *(const uint8_t *)(uintptr_t)(sref + (uint32_t)sub); }

typedef int (__attribute__((thiscall)) *mover_v_t)(void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t *, uint32_t);
static int __attribute__((thiscall)) w_mover_v(void *grid, int32_t x, int32_t y, int32_t w, int32_t hh, int32_t delta, int32_t *out_y, uint32_t pass)
{
    hook_t *h = &H[H_MOVER_V];
    phys_box b = { x, y, w, hh, 0 };
    if (h->mode == 2) { h->calls++; return collision_move_vertical(grid, &b, delta, (int16_t)pass != 0, rh_slope, NULL, out_y); }
    if (!sample(h)) return ((mover_v_t)(uintptr_t)h->va)(grid, x, y, w, hh, delta, out_y, pass);
    int32_t pre = *out_y;
    int r = ((mover_v_t)(uintptr_t)h->va)(grid, x, y, w, hh, delta, out_y, pass);
    int32_t post = *out_y, py = pre;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    int p = collision_move_vertical(grid, &b, delta, (int16_t)pass != 0, rh_slope, NULL, &py);
    dead_restore(sp);
    h->compared++;
    if (p != r || py != post)
        diverged(h, "box (%d,%d %dx%d) d=%d pass=%d pre_y=%d: retail %d/y=%d port %d/y=%d", x, y, w, hh, delta, (int16_t)pass, pre, r, post, p, py);
    return r;
}

typedef int (__attribute__((thiscall)) *sweep_h_t)(void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t *, int32_t *, uint32_t);
static int __attribute__((thiscall)) w_sweep_h(void *grid, int32_t x, int32_t w, int32_t top, int32_t bot, int32_t delta,
                                               int32_t sdown, int32_t sup, int32_t *out_x, int32_t *io_y, uint32_t pass)
{
    hook_t *h = &H[H_SWEEP_H];
    if (h->mode == 2) { h->calls++; return collision_sweep_horizontal(grid, x, w, top, bot, delta, sdown, sup, (int16_t)pass != 0, rh_slope, NULL, out_x, io_y); }
    if (!sample(h)) return ((sweep_h_t)(uintptr_t)h->va)(grid, x, w, top, bot, delta, sdown, sup, out_x, io_y, pass);
    int32_t px0 = *out_x, py0 = *io_y;
    int r = ((sweep_h_t)(uintptr_t)h->va)(grid, x, w, top, bot, delta, sdown, sup, out_x, io_y, pass);
    int32_t rx = *out_x, ry = *io_y, px = px0, py = py0;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    int p = collision_sweep_horizontal(grid, x, w, top, bot, delta, sdown, sup, (int16_t)pass != 0, rh_slope, NULL, &px, &py);
    dead_restore(sp);
    h->compared++;
    if (p != r || px != rx || py != ry)
        diverged(h, "x=%d w=%d y=[%d,%d] d=%d down=%d up=%d pass=%d pre=(%d,%d): retail %d/(%d,%d) port %d/(%d,%d)",
                 x, w, top, bot, delta, sdown, sup, (int16_t)pass, px0, py0, r, rx, ry, p, px, py);
    return r;
}

/* ---- entity movers (FUN_0054db10 / FUN_0054e5c0, thiscall on the moving entity) vs src/entity_move.c ----
 * they read the world singleton *(0x8a9b50) (collide list +0x278c, grid +0x1048): the port's g_world is
 * pointed at retail's before each port call. Written state: body x/y (horizontal; probe mode writes only
 * locals), *out_y + *support (vertical). */
#define WORLD_PTR_VA 0x8a9b50u
static void port_world(void) { g_world = *(wl_world **)(uintptr_t)WORLD_PTR_VA; g_world_slope = rh_slope; g_world_slope_ctx = NULL; }

typedef int (__attribute__((thiscall)) *ent_h_t)(void *, wl_body *, int32_t, int32_t, int32_t, uint32_t, uint32_t, int32_t);
static int __attribute__((thiscall)) w_ent_h(void *self, wl_body *b, int32_t delta, int32_t sdown, int32_t sup, uint32_t pass, uint32_t part, int32_t probe)
{
    hook_t *h = &H[H_ENT_MOVE_H];
    if (h->mode == 2) { h->calls++; port_world(); return entity_move_horizontal(self, b, delta, sdown, sup, (int16_t)pass, (uint16_t)part, probe); }
    if (!sample(h)) return ((ent_h_t)(uintptr_t)h->va)(self, b, delta, sdown, sup, pass, part, probe);
    int32_t x0 = b->x, y0 = b->y;
    int r = ((ent_h_t)(uintptr_t)h->va)(self, b, delta, sdown, sup, pass, part, probe);
    int32_t rx = b->x, ry = b->y;
    b->x = x0; b->y = y0;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    port_world();
    int p = entity_move_horizontal(self, b, delta, sdown, sup, (int16_t)pass, (uint16_t)part, probe);
    dead_restore(sp);
    int32_t px = b->x, py = b->y;
    b->x = rx; b->y = ry;
    h->compared++;
    if (p != r || px != rx || py != ry)
        diverged(h, "ent %p part %u d=%d down=%d up=%d pass=%d probe=%d pre=(%d,%d): retail %d/(%d,%d) port %d/(%d,%d)",
                 self, (uint16_t)part, delta, sdown, sup, (int16_t)pass, probe, x0, y0, r, rx, ry, p, px, py);
    return r;
}

typedef int (__attribute__((thiscall)) *ent_v_t)(void *, wl_body *, int32_t, int32_t, uint32_t, int32_t *, wl_entity **);
static int __attribute__((thiscall)) w_ent_v(void *self, wl_body *b, int32_t delta, int32_t pass, uint32_t part, int32_t *out_y, wl_entity **support)
{
    hook_t *h = &H[H_ENT_MOVE_V];
    if (h->mode == 2) { h->calls++; port_world(); return entity_move_vertical(self, b, delta, pass, (uint16_t)part, out_y, support); }
    if (!sample(h)) return ((ent_v_t)(uintptr_t)h->va)(self, b, delta, pass, part, out_y, support);
    int32_t oy0 = *out_y; wl_entity *s0 = support ? *support : NULL;
    int r = ((ent_v_t)(uintptr_t)h->va)(self, b, delta, pass, part, out_y, support);
    int32_t ry = *out_y; wl_entity *rs = support ? *support : NULL;
    *out_y = oy0; if (support) *support = s0;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    port_world();
    int p = entity_move_vertical(self, b, delta, pass, (uint16_t)part, out_y, support);
    dead_restore(sp);
    int32_t py = *out_y; wl_entity *ps = support ? *support : NULL;
    *out_y = ry; if (support) *support = rs;
    h->compared++;
    if (p != r || py != ry || ps != rs)
        diverged(h, "ent %p part %u d=%d pass=%d box (%d,%d %dx%d m%d) pre_y=%d: retail %d/y=%d/sup=%p port %d/y=%d/sup=%p",
                 self, (uint16_t)part, delta, pass, b->x, b->y, b->w, b->h, b->margin, oy0, r, ry, (void *)rs, p, py, (void *)ps);
    return r;
}

/* ---- map decode (FUN_00587e00, thiscall on the grid) vs src/map_decode.c + parallax.c ----
 * The parse (FUN_00587970, this = retail's 0x40-byte map object) is wrapped as a RECORDER of its
 * (hModule, id): the decode wrapper re-reads that DATA resource through rhost's own resource
 * overrides, parses it with map_data_parse, and runs the port's decode on the identical pre-state
 * grid (the previous room's bytes — retail reuses the buffer). The whole 0x2cb044-byte grid is
 * compared; the report breaks the differing bytes down by grid region. */
#define GRID_BYTES 0x2cb044u
static uint32_t g_map_hmod, g_map_id;
/* (the recorder is the map resource load's wrapper, w_mload: both hooks share 0x587970's call sites) */
static int g_mload_sites;
static int __attribute__((thiscall)) w_mload(uint8_t *m, void *hmod, int32_t id);
static void rh_bank_dims(void *ctx, uint16_t bank, int32_t *w, int32_t *h)
{
    (void)ctx;
    uint32_t p = G32(0x8a760cu + 4u * bank);        /* the sprite-bank pool (0x58c910) */
    *w = p ? (int32_t)G32(p + 0x20) : 0; *h = p ? (int32_t)G32(p + 0x24) : 0;
}
static uint8_t *g_grid_pre, *g_grid_ret;
typedef uint32_t (__attribute__((thiscall)) *map_decode_t)(void *, void *, int32_t, int32_t, int32_t);
static uint32_t __attribute__((thiscall)) w_map_decode(void *grid, void *mapobj, int32_t p2, int32_t p3, int32_t p4)
{
    hook_t *h = &H[H_MAP_DECODE];
    if (!g_grid_pre) { g_grid_pre = malloc(GRID_BYTES); g_grid_ret = malloc(GRID_BYTES); }
    memcpy(g_grid_pre, grid, GRID_BYTES);
    uint32_t r = ((map_decode_t)(uintptr_t)h->va)(grid, mapobj, p2, p3, p4);
    if (!sample(h)) return r;
    memcpy(g_grid_ret, grid, GRID_BYTES);
    HRSRC (WINAPI *fr)(HMODULE, LPCSTR, LPCSTR) = iat_lookup("FindResourceA");
    HGLOBAL (WINAPI *lr)(HMODULE, HRSRC) = iat_lookup("LoadResource");
    DWORD (WINAPI *sr)(HMODULE, HRSRC) = iat_lookup("SizeofResource");
    HMODULE hm = (HMODULE)(uintptr_t)g_map_hmod;
    HRSRC rs = fr(hm, MAKEINTRESOURCEA(g_map_id), "DATA");
    const uint8_t *bytes = rs ? LockResource(lr(hm, rs)) : NULL;
    map_data md;
    if (!bytes || map_data_parse(&md, bytes, sr(hm, rs)) != 0) {
        h->skipped++; rh_log("map_decode f%u: map %u: resource/parse failed\n", g_frame, g_map_id); return r;
    }
    memcpy(grid, g_grid_pre, GRID_BYTES);
    uint32_t sp = SP() - 256;
    dead_save(sp);
    map_decode_cfg cfg; map_decode_cfg_init(&cfg, p2, p3, p4);
    map_decode(&md, grid, &cfg, rh_bank_dims, NULL);
    dead_restore(sp);
    map_data_free(&md);
    static const struct { const char *n; uint32_t lo; } RG[] = {
        { "hdr", 0 }, { "A", 0x30 }, { "B", 0x140030 }, { "C", 0x195030 }, { "E", 0x1d1030 }, { "dims", 0x2c1030 }, { "D", 0x2c1040 } };
    long nd[7] = { 0 }, tot = 0; uint32_t first[7];
    const uint8_t *g = grid;
    for (int k = 0; k < 7; k++) {
        uint32_t hi = k < 6 ? RG[k + 1].lo : GRID_BYTES;
        for (uint32_t o = RG[k].lo; o < hi; o++) if (g[o] != g_grid_ret[o]) { if (!nd[k]++) first[k] = o; }
        tot += nd[k];
    }
    char b[512]; int n = 0;
    for (int k = 0; k < 7; k++) if (nd[k])
        n += snprintf(b + n, sizeof b - n, " %s=%ld@%x(r%02x/p%02x)", RG[k].n, nd[k], first[k], g_grid_ret[first[k]], g[first[k]]);
    rh_log("map_decode f%u map %u p2=%d p3=%d p4=%d: %ld bytes differ%s\n", g_frame, g_map_id, p2, p3, p4, tot, n ? b : "");
    h->compared++;
    if (tot) {
        diverged(h, "map %u p2=%d p3=%d p4=%d: %ld bytes differ%s", g_map_id, p2, p3, p4, tot, b);
        if (h->diverge <= 4) {                          /* both grids, for offline diffing */
            char path[MAX_PATH]; FILE *f;
            snprintf(path, sizeof path, "%s\\grid_f%u_retail.bin", O.rundir, g_frame);
            if ((f = fopen(path, "wb"))) { fwrite(g_grid_ret, 1, GRID_BYTES, f); fclose(f); }
            snprintf(path, sizeof path, "%s\\grid_f%u_port.bin", O.rundir, g_frame);
            if ((f = fopen(path, "wb"))) { fwrite(grid, 1, GRID_BYTES, f); fclose(f); }
        }
    }
    memcpy(grid, g_grid_ret, GRID_BYTES);
    return r;
}

/* ---- camera (FUN_0043d1d0 follow step, thiscall on the 0x78-byte view at world+0x104c; FUN_0043d340
 * shake applier, stdcall(block, cur, max)) vs src/camera_follow.c — camera_view is retail layout ---- */
typedef int (__attribute__((thiscall)) *cam_follow_t)(camera_view *);
static int __attribute__((thiscall)) w_cam_follow(camera_view *v)
{
    hook_t *h = &H[H_CAM_FOLLOW];
    if (h->mode == 2) { h->calls++; camera_follow_step(v); return v->shake_y.out; }
    if (!sample(h)) return ((cam_follow_t)(uintptr_t)h->va)(v);
    camera_view pre = *v;
    int r = ((cam_follow_t)(uintptr_t)h->va)(v);
    camera_view ret = *v;
    *v = pre;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    camera_follow_step(v);
    dead_restore(sp);
    camera_view prt = *v;
    *v = ret;
    h->compared++;
    if (memcmp(&prt, &ret, sizeof ret)) {
        int k = 0; while (((uint8_t *)&prt)[k] == ((uint8_t *)&ret)[k]) k++;
        diverged(h, "view +%02x: retail %08x port %08x (cur %d,%d tgt %d,%d vel %d,%d cap %d flag %d)", k & ~3,
                 *(uint32_t *)((uint8_t *)&ret + (k & ~3)), *(uint32_t *)((uint8_t *)&prt + (k & ~3)),
                 pre.cur_x, pre.cur_y, pre.tgt_x, pre.tgt_y, pre.vel_x, pre.vel_y, pre.cap, pre.flag);
    }
    return r;
}
typedef int32_t (__stdcall *cam_shake_t)(cam_shake *, int32_t, int32_t);
static int32_t __stdcall w_cam_shake(cam_shake *b, int32_t cur, int32_t max)
{
    hook_t *h = &H[H_CAM_SHAKE];
    if (h->mode == 2) { h->calls++; return camera_shake_apply(b, cur, max); }
    if (!sample(h)) return ((cam_shake_t)(uintptr_t)h->va)(b, cur, max);
    cam_shake pre = *b;
    int32_t r = ((cam_shake_t)(uintptr_t)h->va)(b, cur, max);
    cam_shake ret = *b;
    *b = pre;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    int32_t p = camera_shake_apply(b, cur, max);
    dead_restore(sp);
    cam_shake prt = *b;
    *b = ret;
    h->compared++;
    if (p != r || memcmp(&prt, &ret, sizeof ret))
        diverged(h, "act %d dur %d ctr %d ph %d env %u amp %d cur %d max %d: retail %d/out %d port %d/out %d",
                 pre.active, pre.duration, pre.counter, pre.phase, pre.envelope, pre.amp, cur, max, r, ret.out, p, prt.out);
    return r;
}

/* ---- character AI (FUN_00478ba0, thiscall(entity, a2, a3)) — FULL-STATE shadow (rh_shadow_ex): the
 * AI writes the command block, the input ring, sounds and calls the integrator 0x442a70 ---- */
typedef int (__attribute__((thiscall)) *char_ai_t)(void *, int32_t, int32_t);
typedef struct { void *self; int32_t a2, a3; int r_ret, r_port; } ai_ctx;
static void ai_retail(void *c) { ai_ctx *x = c; x->r_ret = ((char_ai_t)(uintptr_t)H[H_CHAR_AI].va)(x->self, x->a2, x->a3); }
static void ai_port(void *c)   { ai_ctx *x = c; port_world(); x->r_port = char_ai(x->self, (wl_body *)(uintptr_t)x->a2, x->a3); }
static int __attribute__((thiscall)) w_char_ai(void *self, int32_t a2, int32_t a3)
{
    hook_t *h = &H[H_CHAR_AI];
    if (h->mode == 2) { h->calls++; port_world(); return char_ai(self, (wl_body *)(uintptr_t)a2, a3); }
    if (!sample(h)) return ((char_ai_t)(uintptr_t)h->va)(self, a2, a3);
    ai_ctx x = { self, a2, a3, 0, 0 };
    char why[512], pre[1600]; int pn = 0;
    {   /* the inputs, for the divergence report */
        wl_char *c = self; input_mgr *m = c->input; wl_keycfg *k = **(wl_keycfg ***)(uintptr_t)0x8a6e80u;
        pn += snprintf(pre + pn, sizeof pre - pn, "prev lr%d ud%d jump%d run_mode=%d win=%d diff=%d held", 
                       c->cmd.lr, c->cmd.ud, c->cmd.jump, k->run_mode, k->dash_window, c->difficulty);
        for (int i = 0; i < 11; i++) pn += snprintf(pre + pn, sizeof pre - pn, " %d/%u", m->axis_held[i], (uint32_t)m->axis_held_b[i]);
        pn += snprintf(pre + pn, sizeof pre - pn, " ring");
        for (int i = 0; i < INPUT_RING_LEN && pn < (int)sizeof pre - 40; i++)
            if (m->ring[i]->id) pn += snprintf(pre + pn, sizeof pre - pn, " [%d]%d@%u/%d", i, m->ring[i]->id, m->ring[i]->ts, m->ring[i]->flag);
    }
    int d = rh_shadow_ex("char_ai", ai_retail, ai_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r_ret; }
    h->compared++;
    if (d || x.r_ret != x.r_port) {
        diverged(h, "f%u ret %d/%d %s", g_frame, x.r_ret, x.r_port, d ? why : "");
        if (h->diverge <= 3) rh_log("char_ai divergence f%u vnow=%u: %s\n", g_frame, rh_vnow(), pre);
    }
    return x.r_ret;
}

/* ---- character movement step (FUN_00467bb0, thiscall(char, cmd, body, probe, ctl, part, grounded)) ----
 * writes only the body record (x/y/attach through the movers) and char->air_jumps: those are
 * snapshotted (cheap enough for every call); stray writes are the hybrid corpus gate's job. */
static long g_phys_cov[8], g_phys_ctl[5], g_phys_mode[5], g_phys_probe;
typedef int (__attribute__((thiscall)) *char_phys_t)(wl_char *, wl_cmd *, wl_body *, int32_t, int32_t, int32_t, int32_t);
static int __attribute__((thiscall)) w_char_phys(wl_char *self, wl_cmd *cmd, wl_body *b, int32_t probe, int32_t ctl, int32_t part, int32_t gr)
{
    hook_t *h = &H[H_CHAR_PHYS];
    if (h->mode == 2) { h->calls++; port_world(); return char_physics_step(self, cmd, b, probe, ctl, part, gr); }
    if (!sample(h)) return ((char_phys_t)(uintptr_t)h->va)(self, cmd, b, probe, ctl, part, gr);
    static wl_body pre, ret; int32_t aj0 = self->air_jumps;
    pre = *b;
    g_phys_cov[pre.state < 7 ? pre.state : 7]++; g_phys_ctl[(uint32_t)ctl < 4 ? ctl : 4]++;
    g_phys_mode[(uint32_t)self->move_mode < 4 ? self->move_mode : 4]++; if (probe) g_phys_probe++;
    int r = ((char_phys_t)(uintptr_t)h->va)(self, cmd, b, probe, ctl, part, gr);
    ret = *b; int32_t aj_r = self->air_jumps;
    *b = pre; self->air_jumps = aj0;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    port_world();
    int p = char_physics_step(self, cmd, b, probe, ctl, part, gr);
    dead_restore(sp);
    int bad = memcmp(b, &ret, sizeof ret) ? 1 : 0;
    int k = 0; if (bad) while (((uint8_t *)b)[k] == ((uint8_t *)&ret)[k]) k++;
    uint32_t pv = bad ? *(uint32_t *)((uint8_t *)b + (k & ~3)) : 0, rv = bad ? *(uint32_t *)((uint8_t *)&ret + (k & ~3)) : 0;
    int32_t aj_p = self->air_jumps;
    *b = ret; self->air_jumps = aj_r;
    h->compared++;
    if (bad || aj_p != aj_r || p != r)
        diverged(h, "f%u char %p code %x part %d probe %d ctl %d gr %d cmd lr%d j%d ud%d | body +%03x retail %08x port %08x"
                 " | pre st%u/%u/%u vx%d vy%d x%d y%d facing %d | air_jumps %d/%d ret %d/%d",
                 g_frame, (void *)self, self->base.code, part, probe, ctl, gr, cmd->lr, cmd->jump, cmd->ud, k & ~3, rv, pv,
                 pre.state, pre.sub, pre.ctr, pre.vel_x, pre.vvel, pre.x, pre.y, pre.facing, aj_r, aj_p, r, p);
    return r;
}

/* ---- per-state clip + jump effects (FUN_00466930, thiscall(char, body, probe, grounded)) — full state:
 * sounds, particles and the LCG ---- */
typedef int (__attribute__((thiscall)) *char_anim_t)(wl_char *, wl_body *, int32_t, int32_t);
typedef struct { wl_char *self; wl_body *b; int32_t probe, gr; int r; } anim_ctx;
static void anim_retail(void *c) { anim_ctx *x = c; x->r = ((char_anim_t)(uintptr_t)H[H_CHAR_ANIM].va)(x->self, x->b, x->probe, x->gr); }
static void anim_port(void *c)   { anim_ctx *x = c; port_world(); char_anim_select(x->self, x->b, x->probe, x->gr); }
static int __attribute__((thiscall)) w_char_anim(wl_char *self, wl_body *b, int32_t probe, int32_t gr)
{
    hook_t *h = &H[H_CHAR_ANIM];
    if (h->mode == 2) { h->calls++; port_world(); char_anim_select(self, b, probe, gr); return 0; }
    if (!sample(h)) return ((char_anim_t)(uintptr_t)h->va)(self, b, probe, gr);
    anim_ctx x = { self, b, probe, gr, 0 };
    uint16_t st = b->state, sub = b->sub;
    char why[512];
    int d = rh_shadow_ex("char_anim", anim_retail, anim_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r; }
    h->compared++;
    if (d) diverged(h, "f%u char %p code %x state %u/%u probe %d gr %d: %s", g_frame, (void *)self, self->base.code, st, sub, probe, gr, why);
    return x.r;
}

/* ---- the integrator (FUN_00442a70, thiscall(char, cmd, in, out, probe, part)) — full state ---- */
typedef int (__attribute__((thiscall)) *char_int_t)(wl_char *, wl_cmd *, wl_body *, wl_body *, int32_t, int32_t);
typedef struct { wl_char *self; wl_cmd *cmd; wl_body *in, *out; int32_t probe, part; int r_ret, r_port; } int_ctx;
static void int_retail(void *c) { int_ctx *x = c; x->r_ret = ((char_int_t)(uintptr_t)H[H_CHAR_INT].va)(x->self, x->cmd, x->in, x->out, x->probe, x->part); }
static void int_port(void *c)   { int_ctx *x = c; port_world(); x->r_port = char_integrate(x->self, x->cmd, x->in, x->out, x->probe, x->part); }
static long g_int_special;
static int __attribute__((thiscall)) w_char_int(wl_char *self, wl_cmd *cmd, wl_body *in, wl_body *out, int32_t probe, int32_t part)
{
    hook_t *h = &H[H_CHAR_INT];
    if (h->mode == 2) { h->calls++; port_world(); return char_integrate(self, cmd, in, out, probe, part); }
    if (!sample(h)) return ((char_int_t)(uintptr_t)h->va)(self, cmd, in, out, probe, part);
    if ((in->pose >= 0x75 && in->pose <= 0x7a) || in->pose == 0x81) g_int_special++;
    int_ctx x = { self, cmd, in, out, probe, part, 0, 0 };
    uint16_t pose = in->pose, st = in->state; uint32_t code = (uint32_t)self->base.code; long unp = g_integrate_unported;
    char why[512];
    int d = rh_shadow_ex("char_integrate", int_retail, int_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r_ret; }
    h->compared++;
    if (d || x.r_ret != x.r_port)
        diverged(h, "f%u char %p code %x pose %x state %u probe %d part %d unported+%ld ret %d/%d %s", g_frame, (void *)self, code,
                 pose, st, probe, part, g_integrate_unported - unp, x.r_ret, x.r_port, d ? why : "");
    return x.r_ret;
}

/* ---- hazard probe (FUN_00440f40, thiscall(char, n, body, int *out)) — reads only; writes *out ---- */
typedef int (__attribute__((thiscall)) *hazard_t)(wl_char *, uint32_t, wl_body *, int32_t *);
static int __attribute__((thiscall)) w_hazard(wl_char *self, uint32_t n, wl_body *b, int32_t *out)
{
    hook_t *h = &H[H_HAZARD];
    if (h->mode == 2) { h->calls++; port_world(); return char_hazard_probe(self, (uint16_t)n, b, out); }
    if (!sample(h)) return ((hazard_t)(uintptr_t)h->va)(self, n, b, out);
    int32_t o0 = out ? *out : 0;
    int r = ((hazard_t)(uintptr_t)h->va)(self, n, b, out);
    int32_t ro = out ? *out : 0;
    if (out) *out = o0;
    port_world();
    int p = char_hazard_probe(self, (uint16_t)n, b, out);
    int32_t po = out ? *out : 0;
    if (out) *out = ro;
    h->compared++;
    if (p != r || po != ro) diverged(h, "f%u char %p n %u: retail %d/out %d port %d/out %d", g_frame, (void *)self, (uint16_t)n, r, ro, p, po);
    return r;
}

/* ---- the NPC navigation AI (FUN_0043f880, thiscall + 9 args) — full state (heap scratch, LCG, probes) ---- */
typedef int (__attribute__((thiscall)) *npc_ai_t)(wl_char *, int32_t, int32_t, wl_body *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_char *self; int32_t a2, a3; wl_body *b; int32_t a5, a6, a7, a8, a9, a10; int r_ret, r_port; } npc_ctx;
static void npc_retail(void *c) { npc_ctx *x = c; x->r_ret = ((npc_ai_t)(uintptr_t)H[H_NPC_AI].va)(x->self, x->a2, x->a3, x->b, x->a5, x->a6, x->a7, x->a8, x->a9, x->a10); }
static void npc_port(void *c)   { npc_ctx *x = c; port_world(); x->r_port = npc_navigate(x->self, x->a2, x->a3, x->b, x->a5, x->a6, x->a7, x->a8, x->a9, x->a10); }
static int __attribute__((thiscall)) w_npc_ai(wl_char *self, int32_t a2, int32_t a3, wl_body *b, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10)
{
    hook_t *h = &H[H_NPC_AI];
    if (h->mode == 2) { h->calls++; port_world(); return npc_navigate(self, a2, a3, b, a5, a6, a7, a8, a9, a10); }
    if (!sample(h)) return ((npc_ai_t)(uintptr_t)h->va)(self, a2, a3, b, a5, a6, a7, a8, a9, a10);
    npc_ctx x = { self, a2, a3, b, a5, a6, a7, a8, a9, a10, 0, 0 };
    char why[512];
    int d = rh_shadow_ex("npc_ai", npc_retail, npc_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r_ret; }
    h->compared++;
    if (d || x.r_ret != x.r_port)
        diverged(h, "f%u char %p code %x tx %d arrive %d gr %d calm %d mode %d ty %d norun %d jump %d state %u ret %d/%d %s", g_frame, (void *)self,
                 self->base.code, a2, a3, a5, a6, a7, a8, a9, a10, b->state, x.r_ret, x.r_port, d ? why : "");
    return x.r_ret;
}

/* ---- the character apply pass (FUN_00485fc0, thiscall(char)) — full state ---- */
typedef void (__attribute__((thiscall)) *char_apply_t)(wl_char *);
static void apply_retail(void *c) { ((char_apply_t)(uintptr_t)H[H_CHAR_APPLY].va)(c); }
static void apply_port(void *c)   { port_world(); char_apply(c); }
static void __attribute__((thiscall)) w_char_apply(wl_char *self)
{
    hook_t *h = &H[H_CHAR_APPLY];
    if (h->mode == 2) { h->calls++; port_world(); char_apply(self); return; }
    if (!sample(h)) { ((char_apply_t)(uintptr_t)h->va)(self); return; }
    uint32_t code = (uint32_t)self->base.code; uint16_t pose = self->base.body->pose, st = self->base.body->state;
    char why[512];
    int d = rh_shadow_ex("char_apply", apply_retail, apply_port, self, why, sizeof why);
    if (d < 0) { h->skipped++; return; }
    h->compared++;
    if (d) diverged(h, "f%u char %p code %x pose %x state %u %s", g_frame, (void *)self, code, pose, st, why);
}

/* ---- the entity tick driver (FUN_0046cd70, thiscall(ctl, cut)) — full state ---- */
typedef int (__attribute__((thiscall)) *world_tick_t)(void *, int32_t);
typedef struct { void *ctl; int32_t cut; int r; } tick_ctx;
static void tick_retail(void *c) { tick_ctx *x = c; x->r = ((world_tick_t)(uintptr_t)H[H_WORLD_TICK].va)(x->ctl, x->cut); }
static void tick_port(void *c)   { tick_ctx *x = c; port_world(); world_tick(x->ctl, x->cut); }
static int __attribute__((thiscall)) w_world_tick(void *ctl, int32_t cut)
{
    hook_t *h = &H[H_WORLD_TICK];
    if (h->mode == 2) { h->calls++; port_world(); return world_tick(ctl, cut); }
    if (!sample(h)) return ((world_tick_t)(uintptr_t)h->va)(ctl, cut);
    tick_ctx x = { ctl, cut, 0 };
    char why[512];
    int d = rh_shadow_ex("world_tick", tick_retail, tick_port, &x, why, sizeof why);
    if (d < 0) { h->skipped++; return x.r; }
    h->compared++;
    if (d) diverged(h, "f%u cut %d %s", g_frame, cut, why);
    return x.r;
}

/* ---- effect records + looping-sound channels (src/entity_fx.c) — full state (particles, the sound
 * pools, the LCG). u16 arguments arrive as raw stack dwords. ---- */
static int full_shadow(hook_t *h, const char *name, void (*r)(void *), void (*p)(void *), void *ctx, const char *what)
{
    char why[512];
    int d = rh_shadow_ex(name, r, p, ctx, why, sizeof why);
    if (d < 0) { h->skipped++; return -1; }   /* not compared: the caller must not compare returns */
    h->compared++;
    if (d) diverged(h, "f%u %s %s", g_frame, what, why);
    return d;
}
#define WHAT(e) (snprintf(what, sizeof what, "ent %p code %x", (void *)(e), (unsigned)(e)->code), what)
/* the rare paths (a live effect record, a fading channel) are shadowed on EVERY call; @N samples
 * only the idle calls */
static int sample_rare(hook_t *h, int rare)
{
    if (!rare) return sample(h);
    h->calls++;
    return !rh_in_shadow();
}
static int fx_busy(const wl_entity *e)
{
    for (uint16_t i = 0; i < e->nfx; i++) if (e->fx[i].id != 0) return 1;
    return 0;
}
static int snd_busy(const wl_entity *e, int i)
{
    if (e->snd == NULL) return 0;
    for (int k = i < 0 ? 0 : i; k < (i < 0 ? e->nsnd : i + 1); k++)
        if (e->snd[k].voice != NULL && (e->snd[k].live == 0 || e->snd[k].fading != 0)) return 1;
    return 0;
}

typedef void *(__attribute__((thiscall)) *fx_add_t)(wl_entity *, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *e; int32_t id, rep; uint32_t dur; int32_t f0c, y, a7, uniq; void *rr, *rp; } fxa_ctx;
static void fxa_retail(void *c) { fxa_ctx *x = c; x->rr = ((fx_add_t)(uintptr_t)H[H_FX_ADD].va)(x->e, x->id, x->rep, x->dur, x->f0c, x->y, x->a7, x->uniq); }
static void fxa_port(void *c) { fxa_ctx *x = c; port_world(); x->rp = entity_fx_add(x->e, x->id, x->rep, (int16_t)x->dur, x->f0c, x->y, x->a7, x->uniq); }
static void *__attribute__((thiscall)) w_fx_add(wl_entity *e, int32_t id, int32_t rep, uint32_t dur, int32_t f0c, int32_t y, int32_t a7, int32_t uniq)
{
    hook_t *h = &H[H_FX_ADD];
    if (h->mode == 2) { h->calls++; port_world(); return entity_fx_add(e, id, rep, (int16_t)dur, f0c, y, a7, uniq); }
    if (!sample(h)) return ((fx_add_t)(uintptr_t)h->va)(e, id, rep, dur, f0c, y, a7, uniq);
    fxa_ctx x = { e, id, rep, dur, f0c, y, a7, uniq, 0, 0 };
    char what[96]; snprintf(what, sizeof what, "ent %p id %d rep %d uniq %d", (void *)e, id, rep, uniq);
    if (!full_shadow(h, "fx_add", fxa_retail, fxa_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %p/%p", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef void (__attribute__((thiscall)) *fx_tick_t)(wl_entity *, wl_body *, uint32_t);
typedef struct { wl_entity *e; wl_body *b; uint32_t part; } fxt_ctx;
static void fxt_retail(void *c) { fxt_ctx *x = c; ((fx_tick_t)(uintptr_t)H[H_FX_TICK].va)(x->e, x->b, x->part); }
static void fxt_port(void *c) { fxt_ctx *x = c; port_world(); entity_fx_tick(x->e, x->b, (uint16_t)x->part); }
static void __attribute__((thiscall)) w_fx_tick(wl_entity *e, wl_body *b, uint32_t part)
{
    hook_t *h = &H[H_FX_TICK];
    if (h->mode == 2) { h->calls++; port_world(); entity_fx_tick(e, b, (uint16_t)part); return; }
    if (!sample_rare(h, fx_busy(e))) { ((fx_tick_t)(uintptr_t)h->va)(e, b, part); return; }
    fxt_ctx x = { e, b, part };
    char what[64];
    full_shadow(h, "fx_tick", fxt_retail, fxt_port, &x, WHAT(e));
}

typedef void (__attribute__((thiscall)) *splash_t)(wl_entity *, wl_body *, int32_t, int32_t);
typedef struct { wl_entity *e; wl_body *b; int32_t ws, wc; } spl_ctx;
static void spl_retail(void *c) { spl_ctx *x = c; ((splash_t)(uintptr_t)H[H_SPLASH].va)(x->e, x->b, x->ws, x->wc); }
static void spl_port(void *c) { spl_ctx *x = c; port_world(); entity_splash(x->e, x->b, x->ws, x->wc); }
static void __attribute__((thiscall)) w_splash(wl_entity *e, wl_body *b, int32_t ws, int32_t wc)
{
    hook_t *h = &H[H_SPLASH];
    if (h->mode == 2) { h->calls++; port_world(); entity_splash(e, b, ws, wc); return; }
    if (!sample(h)) { ((splash_t)(uintptr_t)h->va)(e, b, ws, wc); return; }
    spl_ctx x = { e, b, ws, wc };
    char what[64];
    full_shadow(h, "splash", spl_retail, spl_port, &x, WHAT(e));
}

typedef void (__attribute__((thiscall)) *snd_rel_t)(wl_entity *, uint32_t, intptr_t, int32_t);
typedef struct { wl_entity *e; uint32_t i; intptr_t a3; int32_t stop; } srel_ctx;
static void srel_retail(void *c) { srel_ctx *x = c; ((snd_rel_t)(uintptr_t)H[H_SND_RELEASE].va)(x->e, x->i, x->a3, x->stop); }
static void srel_port(void *c) { srel_ctx *x = c; port_world(); entity_snd_release(x->e, (uint16_t)x->i, x->a3, x->stop); }
static void __attribute__((thiscall)) w_snd_release(wl_entity *e, uint32_t i, intptr_t a3, int32_t stop)
{
    hook_t *h = &H[H_SND_RELEASE];
    if (h->mode == 2) { h->calls++; port_world(); entity_snd_release(e, (uint16_t)i, a3, stop); return; }
    if (!sample(h)) { ((snd_rel_t)(uintptr_t)h->va)(e, i, a3, stop); return; }
    srel_ctx x = { e, i, a3, stop };
    char what[64];
    full_shadow(h, "snd_release", srel_retail, srel_port, &x, WHAT(e));
}

typedef void (__attribute__((thiscall)) *snd_all_t)(wl_entity *);
static void sall_retail(void *c) { ((snd_all_t)(uintptr_t)H[H_SND_ALL].va)(c); }
static void sall_port(void *c) { port_world(); entity_snd_update_all(c); }
static void __attribute__((thiscall)) w_snd_all(wl_entity *e)
{
    hook_t *h = &H[H_SND_ALL];
    if (h->mode == 2) { h->calls++; port_world(); entity_snd_update_all(e); return; }
    if (!sample_rare(h, snd_busy(e, -1))) { ((snd_all_t)(uintptr_t)h->va)(e); return; }
    char what[64];
    full_shadow(h, "snd_all", sall_retail, sall_port, e, WHAT(e));
}

typedef void (__attribute__((thiscall)) *snd_one_t)(wl_entity *, uint32_t);
typedef struct { wl_entity *e; uint32_t i; } sone_ctx;
static void sone_retail(void *c) { sone_ctx *x = c; ((snd_one_t)(uintptr_t)H[H_SND_ONE].va)(x->e, x->i); }
static void sone_port(void *c) { sone_ctx *x = c; port_world(); entity_snd_update(x->e, (uint16_t)x->i); }
static void __attribute__((thiscall)) w_snd_one(wl_entity *e, uint32_t i)
{
    hook_t *h = &H[H_SND_ONE];
    if (h->mode == 2) { h->calls++; port_world(); entity_snd_update(e, (uint16_t)i); return; }
    if (!sample_rare(h, snd_busy(e, (uint16_t)i))) { ((snd_one_t)(uintptr_t)h->va)(e, i); return; }
    sone_ctx x = { e, i };
    char what[64];
    full_shadow(h, "snd_one", sone_retail, sone_port, &x, WHAT(e));
}

/* ---- the actor update (FUN_0054f980, thiscall(actor, in, out, probe, part)) — full state ---- */
typedef int (__attribute__((thiscall)) *actor_t)(wl_actor *, wl_body *, wl_body *, int32_t, int32_t);
typedef struct { wl_actor *a; wl_body *in, *out; int32_t probe, part; int rr, rp; } act_ctx;
static void act_retail(void *c) { act_ctx *x = c; x->rr = ((actor_t)(uintptr_t)H[H_ACTOR].va)(x->a, x->in, x->out, x->probe, x->part); }
static void act_port(void *c) { act_ctx *x = c; port_world(); x->rp = actor_update(x->a, x->in, x->out, x->probe, x->part); }
static long g_actor_codes[64]; static int32_t g_actor_code_id[64]; static int g_nactor_codes;
static int __attribute__((thiscall)) w_actor(wl_actor *a, wl_body *in, wl_body *out, int32_t probe, int32_t part)
{
    hook_t *h = &H[H_ACTOR];
    if (h->mode == 2) { h->calls++; port_world(); return actor_update(a, in, out, probe, part); }
    if (!sample(h)) return ((actor_t)(uintptr_t)h->va)(a, in, out, probe, part);
    int32_t code = a->base.code;
    int k = 0;
    while (k < g_nactor_codes && g_actor_code_id[k] != code) k++;
    if (k == g_nactor_codes && k < 64) { g_actor_code_id[k] = code; g_nactor_codes++; }
    if (k < 64) g_actor_codes[k]++;
    act_ctx x = { a, in, out, probe, part, 0, 0 };
    char what[96]; snprintf(what, sizeof what, "actor %p code %d pose %u probe %d part %d", (void *)a, code, in->pose, probe, part);
    if (!full_shadow(h, "actor_update", act_retail, act_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

/* ---- the particle tick (FUN_0046e510, thiscall(particle)) — full state ---- */
typedef void (__attribute__((thiscall)) *particle_t)(wl_particle *);
static void prt_retail(void *c) { ((particle_t)(uintptr_t)H[H_PARTICLE].va)(c); }
static void prt_port(void *c) { port_world(); particle_tick(c); }
static long g_prt_codes[128]; static int32_t g_prt_code_id[128]; static int g_nprt_codes;
static void __attribute__((thiscall)) w_particle(wl_particle *p)
{
    hook_t *h = &H[H_PARTICLE];
    if (h->mode == 2) { h->calls++; port_world(); particle_tick(p); return; }
    int32_t code = p->base.code;
    int k = 0;
    while (k < g_nprt_codes && g_prt_code_id[k] != code) k++;
    if (k == g_nprt_codes && k < 128) { g_prt_code_id[k] = code; g_nprt_codes++; }
    /* the first 64 calls of every code are shadowed, then 1 in N */
    int rare = k < 128 && g_prt_codes[k] < 64;
    if (!sample_rare(h, rare)) { ((particle_t)(uintptr_t)h->va)(p); return; }
    if (k < 128) g_prt_codes[k]++;
    char what[96]; snprintf(what, sizeof what, "particle %p code %d f58 %u ctr %d", (void *)p, code, p->base.body->f58, p->base.body->act_ctr);
    full_shadow(h, "particle_tick", prt_retail, prt_port, p, what);
}

/* ---- the behaviour AI (FUN_0047b990, thiscall(char, cut, body, grounded)) — full state ---- */
typedef int (__attribute__((thiscall)) *behave_t)(wl_char *, int32_t, wl_body *, int32_t);
typedef struct { wl_char *c; int32_t cut; wl_body *b; int32_t gr; int rr, rp; } bh_ctx;
static void bh_retail(void *x_) { bh_ctx *x = x_; x->rr = ((behave_t)(uintptr_t)H[H_BEHAVE].va)(x->c, x->cut, x->b, x->gr); }
static void bh_port(void *x_) { bh_ctx *x = x_; port_world(); x->rp = char_behave(x->c, x->cut, x->b, x->gr); }
static long g_bh_codes[64]; static int32_t g_bh_code_id[64]; static int g_nbh_codes;
static int __attribute__((thiscall)) w_behave(wl_char *c, int32_t cut, wl_body *b, int32_t gr)
{
    hook_t *h = &H[H_BEHAVE];
    if (h->mode == 2) { h->calls++; port_world(); return char_behave(c, cut, b, gr); }
    int32_t code = c->base.code;
    int k = 0;
    while (k < g_nbh_codes && g_bh_code_id[k] != code) k++;
    if (k == g_nbh_codes && k < 64) { g_bh_code_id[k] = code; g_nbh_codes++; }
    int rare = k < 64 && g_bh_codes[k] < 64;
    if (!sample_rare(h, rare)) return ((behave_t)(uintptr_t)h->va)(c, cut, b, gr);
    if (k < 64) g_bh_codes[k]++;
    bh_ctx x = { c, cut, b, gr, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d cut %d state %u f14288 %u think %u", (void *)c, code, cut, c->ai_state, c->f14288, c->think_t);
    if (!full_shadow(h, "char_behave", bh_retail, bh_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

/* ---- the behaviour helpers (src/behave_ops.c) ----
 * step_ok (FUN_0047dbb0) only reads: return.  pick_char (FUN_0047e330: the LCG) / walk_to
 * (FUN_00484c10) / pace (FUN_004857f0): full state + return. u16 arguments arrive as raw dwords. */
typedef int (__attribute__((thiscall)) *step_ok_t)(wl_char *, wl_body *, int32_t, int32_t, int32_t, int32_t, int32_t);
static int __attribute__((thiscall)) w_step_ok(wl_char *c, wl_body *b, int32_t d, int32_t dir, int32_t strict, int32_t deep, int32_t nf)
{
    hook_t *h = &H[H_STEP_OK];
    if (h->mode == 2) { h->calls++; port_world(); return behave_step_ok(c, b, d, dir, strict, deep, nf); }
    int r = ((step_ok_t)(uintptr_t)h->va)(c, b, d, dir, strict, deep, nf);
    if (!sample(h)) return r;
    uint32_t sp = SP() - 256;
    dead_save(sp);
    port_world();
    int p = behave_step_ok(c, b, d, dir, strict, deep, nf);
    dead_restore(sp);
    h->compared++;
    if (p != r) diverged(h, "f%u char %p code %d x %d y %d d %d dir %d %d %d %d mode %d: retail %d port %d", g_frame, (void *)c,
                         c->base.code, b->x, b->y, d, dir, strict, deep, nf, c->move_mode, r, p);
    return r;
}

typedef wl_char *(__attribute__((thiscall)) *pick_t)(wl_char *, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_char *c; int32_t fac, rel; uint32_t part; int32_t reach, crowd, lead, threat; wl_char *rr, *rp; } pick_ctx;
static void pick_retail(void *x_) { pick_ctx *x = x_; x->rr = ((pick_t)(uintptr_t)H[H_PICK_CHAR].va)(x->c, x->fac, x->rel, x->part, x->reach, x->crowd, x->lead, x->threat); }
static void pick_port(void *x_) { pick_ctx *x = x_; port_world(); x->rp = behave_pick_char(x->c, x->fac, x->rel, x->part, x->reach, x->crowd, x->lead, x->threat); }
static wl_char *__attribute__((thiscall)) w_pick(wl_char *c, int32_t fac, int32_t rel, uint32_t part, int32_t reach, int32_t crowd, int32_t lead, int32_t threat)
{
    hook_t *h = &H[H_PICK_CHAR];
    if (h->mode == 2) { h->calls++; port_world(); return behave_pick_char(c, fac, rel, part, reach, crowd, lead, threat); }
    if (!sample(h)) return ((pick_t)(uintptr_t)h->va)(c, fac, rel, part, reach, crowd, lead, threat);
    pick_ctx x = { c, fac, rel, part, reach, crowd, lead, threat, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d fac %d rel %d part %u reach %d crowd %d lead %d threat %d", (void *)c,
                             c->base.code, fac, rel, part, reach, crowd, lead, threat);
    if (!full_shadow(h, "pick_char", pick_retail, pick_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %p/%p", g_frame, what, (void *)x.rr, (void *)x.rp);
    return x.rr;
}

typedef int (__attribute__((thiscall)) *walk_to_t)(wl_char *, wl_body *, int32_t);
typedef struct { wl_char *c; wl_body *b; int32_t gr; int rr, rp; } wt_ctx;
static void wt_retail(void *x_) { wt_ctx *x = x_; x->rr = ((walk_to_t)(uintptr_t)H[H_WALK_TO].va)(x->c, x->b, x->gr); }
static void wt_port(void *x_) { wt_ctx *x = x_; port_world(); x->rp = behave_walk_to(x->c, x->b, x->gr); }
static int __attribute__((thiscall)) w_walk_to(wl_char *c, wl_body *b, int32_t gr)
{
    hook_t *h = &H[H_WALK_TO];
    if (h->mode == 2) { h->calls++; port_world(); return behave_walk_to(c, b, gr); }
    if (!sample_rare(h, c->ai_state == 1 || c->ai_state == 7)) return ((walk_to_t)(uintptr_t)h->va)(c, b, gr);
    wt_ctx x = { c, b, gr, 0, 0 };
    char what[96]; snprintf(what, sizeof what, "char %p code %d state %u x %d to %d", (void *)c, c->base.code, c->ai_state, b->x, c->walk_x);
    if (!full_shadow(h, "walk_to", wt_retail, wt_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef int (__attribute__((thiscall)) *pace_t)(wl_char *, wl_body *, int32_t, uint32_t, int32_t);
typedef struct { wl_char *c; wl_body *b; int32_t gr; uint32_t hold; int32_t chance; int rr, rp; } pace_ctx;
static void pace_retail(void *x_) { pace_ctx *x = x_; x->rr = ((pace_t)(uintptr_t)H[H_PACE].va)(x->c, x->b, x->gr, x->hold, x->chance); }
static void pace_port(void *x_) { pace_ctx *x = x_; port_world(); x->rp = behave_pace(x->c, x->b, x->gr, (int16_t)x->hold, x->chance); }
static int __attribute__((thiscall)) w_pace(wl_char *c, wl_body *b, int32_t gr, uint32_t hold, int32_t chance)
{
    hook_t *h = &H[H_PACE];
    if (h->mode == 2) { h->calls++; port_world(); return behave_pace(c, b, gr, (int16_t)hold, chance); }
    if (!sample_rare(h, c->side_t <= 1)) return ((pace_t)(uintptr_t)h->va)(c, b, gr, hold, chance);
    pace_ctx x = { c, b, gr, hold, chance, 0, 0 };
    char what[96]; snprintf(what, sizeof what, "char %p code %d side %d t %u x %d", (void *)c, c->base.code, c->side, c->side_t, b->x);
    if (!full_shadow(h, "pace", pace_retail, pace_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

/* part centre x / y (FUN_0044e640 / FUN_0044e680): pure, return only */
typedef int32_t (__attribute__((thiscall)) *part_t)(wl_entity *, uint32_t);
static int32_t part_shadow(hook_t *h, int32_t (*port)(const wl_entity *, uint16_t), wl_entity *e, uint32_t i)
{
    if (h->mode == 2) { h->calls++; return port(e, (uint16_t)i); }
    int32_t r = ((part_t)(uintptr_t)h->va)(e, i);
    if (!sample(h)) return r;
    int32_t p = port(e, (uint16_t)i);
    h->compared++;
    if (p != r) diverged(h, "f%u ent %p code %d part %u: retail %d port %d", g_frame, (void *)e, e->code, i, r, p);
    return r;
}
static int32_t __attribute__((thiscall)) w_part_cx(wl_entity *e, uint32_t i) { return part_shadow(&H[H_PART_CX], part_center_x, e, i); }
static int32_t __attribute__((thiscall)) w_part_cy(wl_entity *e, uint32_t i) { return part_shadow(&H[H_PART_CY], part_center_y, e, i); }

/* ---- the sound layer (src/sound_fx.c): full state (the pools, the fake DirectSound buffers, the
 * clock) except sfx_place / voice_place, which compare their outputs. u16 args arrive as raw dwords. ---- */
/* the shape: replace / sample / full shadow (state + return) */
typedef struct { wl_vpool *p; int rr, rp; } vpload_ctx;
static void vpload_retail(void *c) { vpload_ctx *x = c; x->rr = ((int (__attribute__((thiscall)) *)(wl_vpool *))(uintptr_t)H[H_VP_LOAD].va)(x->p); }
static void vpload_port(void *c) { vpload_ctx *x = c; port_world(); x->rp = vpool_load(x->p); }
static int __attribute__((thiscall)) w_vpload(wl_vpool *p)
{
    hook_t *h = &H[H_VP_LOAD];
    if (h->mode == 2) { h->calls++; port_world(); return vpool_load(p); }
    if (!sample(h)) return ((int (__attribute__((thiscall)) *)(wl_vpool *))(uintptr_t)h->va)(p);
    vpload_ctx x = { p, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "pool %p n %u res %u", (void *)p, p->n, p->res);
    if (!full_shadow(h, "vpool_load", vpload_retail, vpload_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, (int)(intptr_t)x.rr, (int)(intptr_t)x.rp);
    return x.rr;
}

typedef struct { wl_vpool *p; int32_t own; wl_voice *rr, *rp; } vptake_ctx;
static void vptake_retail(void *c) { vptake_ctx *x = c; x->rr = ((wl_voice *(__attribute__((thiscall)) *)(wl_vpool *, int32_t))(uintptr_t)H[H_VP_TAKE].va)(x->p, x->own); }
static void vptake_port(void *c) { vptake_ctx *x = c; port_world(); x->rp = vpool_take(x->p, x->own); }
static wl_voice *__attribute__((thiscall)) w_vptake(wl_vpool *p, int32_t own)
{
    hook_t *h = &H[H_VP_TAKE];
    if (h->mode == 2) { h->calls++; port_world(); return vpool_take(p, own); }
    if (!sample(h)) return ((wl_voice *(__attribute__((thiscall)) *)(wl_vpool *, int32_t))(uintptr_t)h->va)(p, own);
    vptake_ctx x = { p, own, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "pool %p n %u next %u owner %d", (void *)p, p->n, p->next, own);
    if (!full_shadow(h, "vpool_take", vptake_retail, vptake_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, (int)(intptr_t)x.rr, (int)(intptr_t)x.rp);
    return x.rr;
}

typedef struct { void *sys; uint32_t id; int32_t vol, freq; int rr, rp; } sfxplay_ctx;
static void sfxplay_retail(void *c) { sfxplay_ctx *x = c; x->rr = ((int (__attribute__((thiscall)) *)(void *, uint32_t, int32_t, int32_t))(uintptr_t)H[H_SFX_PLAY].va)(x->sys, x->id, x->vol, x->freq); }
static void sfxplay_port(void *c) { sfxplay_ctx *x = c; port_world(); x->rp = sfx_play(x->sys, (uint16_t)x->id, x->vol, x->freq); }
static int __attribute__((thiscall)) w_sfxplay(void *sys, uint32_t id, int32_t vol, int32_t freq)
{
    hook_t *h = &H[H_SFX_PLAY];
    if (h->mode == 2) { h->calls++; port_world(); return sfx_play(sys, (uint16_t)id, vol, freq); }
    if (!sample(h)) return ((int (__attribute__((thiscall)) *)(void *, uint32_t, int32_t, int32_t))(uintptr_t)h->va)(sys, id, vol, freq);
    sfxplay_ctx x = { sys, id, vol, freq, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "id %u vol %d freq %d", id & 0xffffu, vol, freq);
    if (!full_shadow(h, "sfx_play", sfxplay_retail, sfxplay_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, (int)(intptr_t)x.rr, (int)(intptr_t)x.rp);
    return x.rr;
}

typedef struct { wl_entity *e; int32_t a2; uint32_t id; int32_t vol, freq, fo; int rr, rp; } sfxat_ctx;
static void sfxat_retail(void *c) { sfxat_ctx *x = c; x->rr = ((int (__attribute__((thiscall)) *)(wl_entity *, int32_t, uint32_t, int32_t, int32_t, int32_t))(uintptr_t)H[H_SFX_AT].va)(x->e, x->a2, x->id, x->vol, x->freq, x->fo); }
static void sfxat_port(void *c) { sfxat_ctx *x = c; port_world(); x->rp = sfx_play_at(x->e, x->a2, (uint16_t)x->id, x->vol, x->freq, x->fo); }
static int __attribute__((thiscall)) w_sfxat(wl_entity *e, int32_t a2, uint32_t id, int32_t vol, int32_t freq, int32_t fo)
{
    hook_t *h = &H[H_SFX_AT];
    if (h->mode == 2) { h->calls++; port_world(); return sfx_play_at(e, a2, (uint16_t)id, vol, freq, fo); }
    if (!sample(h)) return ((int (__attribute__((thiscall)) *)(wl_entity *, int32_t, uint32_t, int32_t, int32_t, int32_t))(uintptr_t)h->va)(e, a2, id, vol, freq, fo);
    sfxat_ctx x = { e, a2, id, vol, freq, fo, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "ent %p code %d id %u vol %d fo %d", (void *)e, e->code, id & 0xffffu, vol, fo);
    if (!full_shadow(h, "sfx_play_at", sfxat_retail, sfxat_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, (int)(intptr_t)x.rr, (int)(intptr_t)x.rp);
    return x.rr;
}

typedef struct { wl_voiceset *vs; uint32_t g; int32_t force, vol, pan, freq, fixed; int rr, rp; } vplay_ctx;
static void vplay_retail(void *c) { vplay_ctx *x = c; x->rr = ((int (__attribute__((thiscall)) *)(wl_voiceset *, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t))(uintptr_t)H[H_VOICE_PLAY].va)(x->vs, x->g, x->force, x->vol, x->pan, x->freq, x->fixed); }
static void vplay_port(void *c) { vplay_ctx *x = c; port_world(); x->rp = voice_play(x->vs, (uint16_t)x->g, x->force, x->vol, x->pan, x->freq, x->fixed); }
static int __attribute__((thiscall)) w_vplay(wl_voiceset *vs, uint32_t g, int32_t force, int32_t vol, int32_t pan, int32_t freq, int32_t fixed)
{
    hook_t *h = &H[H_VOICE_PLAY];
    if (h->mode == 2) { h->calls++; port_world(); return voice_play(vs, (uint16_t)g, force, vol, pan, freq, fixed); }
    if (!sample(h)) return ((int (__attribute__((thiscall)) *)(wl_voiceset *, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t))(uintptr_t)h->va)(vs, g, force, vol, pan, freq, fixed);
    vplay_ctx x = { vs, g, force, vol, pan, freq, fixed, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "voices %p grp %u mode %d", (void *)vs, g & 0xffffu, vs->mode);
    if (!full_shadow(h, "voice_play", vplay_retail, vplay_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, (int)(intptr_t)x.rr, (int)(intptr_t)x.rp);
    return x.rr;
}

typedef int (__attribute__((thiscall)) *sndst_t)(wl_entity *, uint32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *e; uint32_t i; int32_t a3; uint32_t id; int32_t vol, freq, fo, stop; int rr, rp; } sndst_ctx;
static void sndst_retail(void *c) { sndst_ctx *x = c; x->rr = ((sndst_t)(uintptr_t)H[H_SND_START].va)(x->e, x->i, x->a3, x->id, x->vol, x->freq, x->fo, x->stop); }
static void sndst_port(void *c) { sndst_ctx *x = c; port_world(); x->rp = entity_snd_start(x->e, (uint16_t)x->i, x->a3, (uint16_t)x->id, x->vol, x->freq, x->fo, x->stop); }
static int __attribute__((thiscall)) w_snd_start(wl_entity *e, uint32_t i, int32_t a3, uint32_t id, int32_t vol, int32_t freq, int32_t fo, int32_t stop)
{
    hook_t *h = &H[H_SND_START];
    if (h->mode == 2) { h->calls++; port_world(); return entity_snd_start(e, (uint16_t)i, a3, (uint16_t)id, vol, freq, fo, stop); }
    if (!sample(h)) return ((sndst_t)(uintptr_t)h->va)(e, i, a3, id, vol, freq, fo, stop);
    sndst_ctx x = { e, i, a3, id, vol, freq, fo, stop, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "ent %p code %d ch %u id %u fo %d", (void *)e, e->code, i & 0xffffu, id & 0xffffu, fo);
    full_shadow(h, "snd_start", sndst_retail, sndst_port, &x, what);   /* eax on the culls is a register nobody reads */
    return x.rr;
}

/* placement: pure (reads the listener + the entity), compare return + both outputs */
typedef int (__attribute__((thiscall)) *place_t)(wl_entity *, int32_t, int32_t, int32_t, wl_body *, int32_t *, int32_t *);
static int __attribute__((thiscall)) w_sfx_place(wl_entity *e, int32_t base, int32_t fo, int32_t keep, wl_body *b, int32_t *ov, int32_t *op)
{
    hook_t *h = &H[H_SFX_PLACE];
    if (h->mode == 2) { h->calls++; port_world(); return sfx_place(e, base, fo, keep, b, ov, op); }
    int32_t v0 = ov ? *ov : 0, p0 = op ? *op : 0;
    int r = ((place_t)(uintptr_t)h->va)(e, base, fo, keep, b, ov, op);
    if (!sample(h)) return r;
    int32_t rv = ov ? *ov : 0, rp = op ? *op : 0, pv = v0, pp = p0;
    port_world();
    int p = sfx_place(e, base, fo, keep, b, ov ? &pv : NULL, op ? &pp : NULL);
    h->compared++;
    if (p != r || pv != rv || pp != rp)
        diverged(h, "f%u ent %p code %d base %d fo %d keep %d: retail %d %d/%d port %d %d/%d", g_frame, (void *)e, e->code,
                 base, fo, keep, r, rv, rp, p, pv, pp);
    return r;
}
typedef int (__attribute__((thiscall)) *vplace_t)(wl_char *, int32_t *, int32_t *, int32_t *);
static int __attribute__((thiscall)) w_voice_place(wl_char *c, int32_t *ov, int32_t *op, int32_t *of)
{
    hook_t *h = &H[H_VOICE_PLACE];
    if (h->mode == 2) { h->calls++; port_world(); return voice_place(c, ov, op, of); }
    if (!sample(h)) return ((vplace_t)(uintptr_t)h->va)(c, ov, op, of);
    uint32_t seed = G32(g_ed->rng_seed);
    int32_t v0 = ov ? *ov : 0, p0 = op ? *op : 0, f0 = of ? *of : 0;
    int r = ((vplace_t)(uintptr_t)h->va)(c, ov, op, of);
    uint32_t seed_r = G32(g_ed->rng_seed);
    int32_t rv = ov ? *ov : 0, rp = op ? *op : 0, rf = of ? *of : 0, pv = v0, pp = p0, pf = f0;
    G32(g_ed->rng_seed) = seed;
    port_world();
    int p = voice_place(c, ov ? &pv : NULL, op ? &pp : NULL, of ? &pf : NULL);
    uint32_t seed_p = G32(g_ed->rng_seed);
    G32(g_ed->rng_seed) = seed_r;
    h->compared++;
    if (p != r || pv != rv || pp != rp || pf != rf || seed_p != seed_r)
        diverged(h, "f%u char %p code %d: retail %d %d/%d/%d port %d %d/%d/%d", g_frame, (void *)c, c->base.code, r, rv, rp, rf,
                 p, pv, pp, pf);
    return r;
}

/* the voice cue + the voice set's pieces */
typedef int (__attribute__((thiscall)) *vcue_t)(wl_char *, int32_t, uint32_t, int32_t, int32_t, int32_t);
typedef struct { wl_char *c; int32_t a2; uint32_t g; int32_t force, delay, fixed; int rr, rp; } vcue_ctx;
static void vcue_retail(void *x_) { vcue_ctx *x = x_; x->rr = ((vcue_t)(uintptr_t)H[H_VOICE_CUE].va)(x->c, x->a2, x->g, x->force, x->delay, x->fixed); }
static void vcue_port(void *x_) { vcue_ctx *x = x_; port_world(); x->rp = voice_cue(x->c, x->a2, (uint16_t)x->g, x->force, x->delay, x->fixed); }
static int __attribute__((thiscall)) w_vcue(wl_char *c, int32_t a2, uint32_t g, int32_t force, int32_t delay, int32_t fixed)
{
    hook_t *h = &H[H_VOICE_CUE];
    if (h->mode == 2) { h->calls++; port_world(); return voice_cue(c, a2, (uint16_t)g, force, delay, fixed); }
    if (!sample(h)) return ((vcue_t)(uintptr_t)h->va)(c, a2, g, force, delay, fixed);
    vcue_ctx x = { c, a2, g, force, delay, fixed, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d a2 %d grp %u force %d delay %d fixed %d", (void *)c, c->base.code, a2,
                             g & 0xffffu, force, delay, fixed);
    if (!full_shadow(h, "voice_cue", vcue_retail, vcue_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef void (__attribute__((thiscall)) *vstick_t)(wl_voiceset *, int32_t);
typedef struct { wl_voiceset *vs; int32_t dt; } vstick_ctx;
static void vstick_retail(void *x_) { vstick_ctx *x = x_; ((vstick_t)(uintptr_t)H[H_VS_TICK].va)(x->vs, x->dt); }
static void vstick_port(void *x_) { vstick_ctx *x = x_; port_world(); voiceset_tick(x->vs, x->dt); }
static void __attribute__((thiscall)) w_vstick(wl_voiceset *vs, int32_t dt)
{
    hook_t *h = &H[H_VS_TICK];
    if (h->mode == 2) { h->calls++; port_world(); voiceset_tick(vs, dt); return; }
    if (!sample(h)) { ((vstick_t)(uintptr_t)h->va)(vs, dt); return; }
    vstick_ctx x = { vs, dt };
    char what[96]; snprintf(what, sizeof what, "voices %p dt %d", (void *)vs, dt);
    full_shadow(h, "voiceset_tick", vstick_retail, vstick_port, &x, what);
}

typedef void (__stdcall *vlstop_t)(wl_vline *, int32_t);
typedef struct { wl_vline *l; int32_t stop; } vlstop_ctx;
static void vlstop_retail(void *x_) { vlstop_ctx *x = x_; ((vlstop_t)(uintptr_t)H[H_VLINE_STOP].va)(x->l, x->stop); }
static void vlstop_port(void *x_) { vlstop_ctx *x = x_; port_world(); vline_stop(x->l, x->stop); }
static void __stdcall w_vlstop(wl_vline *l, int32_t stop)
{
    hook_t *h = &H[H_VLINE_STOP];
    if (h->mode == 2) { h->calls++; port_world(); vline_stop(l, stop); return; }
    if (!sample(h)) { ((vlstop_t)(uintptr_t)h->va)(l, stop); return; }
    vlstop_ctx x = { l, stop };
    char what[96]; snprintf(what, sizeof what, "line %p voice %p stop %d", (void *)l, (void *)l->voice, stop);
    full_shadow(h, "vline_stop", vlstop_retail, vlstop_port, &x, what);
}

/* pure: compare the return + both outputs */
typedef int (__attribute__((thiscall)) *vssnd_t)(wl_voiceset *, wl_vgroup **, wl_vline **);
static int __attribute__((thiscall)) w_vssnd(wl_voiceset *vs, wl_vgroup **g, wl_vline **l)
{
    hook_t *h = &H[H_VS_SOUNDING];
    if (h->mode == 2) { h->calls++; return voiceset_sounding(vs, g, l); }
    int r = ((vssnd_t)(uintptr_t)h->va)(vs, g, l);
    if (!sample(h)) return r;
    wl_vgroup *pg; wl_vline *pl;
    int p = voiceset_sounding(vs, &pg, &pl);
    h->compared++;
    if (p != r || pg != *g || pl != *l)
        diverged(h, "f%u voices %p: retail %d %p/%p port %d %p/%p", g_frame, (void *)vs, r, (void *)*g, (void *)*l, p, (void *)pg, (void *)pl);
    return r;
}

/* ---- entity list membership + release (src/entity_life.c): full state ---- */
typedef void (__attribute__((thiscall)) *unlist_t)(wl_entity *);
typedef struct { wl_entity *e; } unlist_ctx;
static void unlist_retail(void *c) { unlist_ctx *x = c; ((unlist_t)(uintptr_t)H[H_UNLIST].va)(x->e); }
static void unlist_port(void *c) { unlist_ctx *x = c; port_world(); entity_unlist(x->e); }
static void __attribute__((thiscall)) w_unlist(wl_entity *e)
{
    hook_t *h = &H[H_UNLIST];
    if (h->mode == 2) { h->calls++; port_world(); entity_unlist(e); return; }
    if (!sample(h)) { ((unlist_t)(uintptr_t)h->va)(e); return; }
    unlist_ctx x = { e };
    char what[128]; snprintf(what, sizeof what, "ent %p code %d", (void *)e, e->code);
    full_shadow(h, "entity_unlist", unlist_retail, unlist_port, &x, what);
}
typedef void (__attribute__((thiscall)) *erel_t)(wl_entity *, int32_t);
typedef struct { wl_entity *e; int32_t listed; } erel_ctx;
static void erel_retail(void *c) { erel_ctx *x = c; ((erel_t)(uintptr_t)H[H_RELEASE].va)(x->e, x->listed); }
static void erel_port(void *c) { erel_ctx *x = c; port_world(); entity_release(x->e, x->listed); }
static void __attribute__((thiscall)) w_erel(wl_entity *e, int32_t listed)
{
    hook_t *h = &H[H_RELEASE];
    if (h->mode == 2) { h->calls++; port_world(); entity_release(e, listed); return; }
    if (!sample(h)) { ((erel_t)(uintptr_t)h->va)(e, listed); return; }
    erel_ctx x = { e, listed };
    char what[128]; snprintf(what, sizeof what, "ent %p code %d listed %d", (void *)e, e->code, listed);
    full_shadow(h, "entity_release", erel_retail, erel_port, &x, what);
}
typedef void (__attribute__((thiscall)) *arel_t)(wl_actor *);
typedef struct { wl_actor *a; } arel_ctx;
static void arel_retail(void *c) { arel_ctx *x = c; ((arel_t)(uintptr_t)H[H_ACTOR_REL].va)(x->a); }
static void arel_port(void *c) { arel_ctx *x = c; port_world(); actor_release(x->a); }
static void __attribute__((thiscall)) w_arel(wl_actor *a)
{
    hook_t *h = &H[H_ACTOR_REL];
    if (h->mode == 2) { h->calls++; port_world(); actor_release(a); return; }
    if (!sample(h)) { ((arel_t)(uintptr_t)h->va)(a); return; }
    arel_ctx x = { a };
    char what[128]; snprintf(what, sizeof what, "actor %p code %d", (void *)a, a->base.code);
    full_shadow(h, "actor_release", arel_retail, arel_port, &x, what);
}
typedef void (__attribute__((thiscall)) *crel_t)(wl_char *);
typedef struct { wl_char *c; } crel_ctx;
static void crel_retail(void *c) { crel_ctx *x = c; ((crel_t)(uintptr_t)H[H_CHAR_REL].va)(x->c); }
static void crel_port(void *c) { crel_ctx *x = c; port_world(); char_release(x->c); }
static void __attribute__((thiscall)) w_crel(wl_char *c)
{
    hook_t *h = &H[H_CHAR_REL];
    if (h->mode == 2) { h->calls++; port_world(); char_release(c); return; }
    if (!sample(h)) { ((crel_t)(uintptr_t)h->va)(c); return; }
    crel_ctx x = { c };
    char what[128]; snprintf(what, sizeof what, "char %p code %d", (void *)c, c->base.code);
    full_shadow(h, "char_release", crel_retail, crel_port, &x, what);
}
typedef void (__attribute__((thiscall)) *scoll_t)(wl_entity *, int32_t, int32_t);
typedef struct { wl_entity *e; int32_t mode, kind; } scoll_ctx;
static void scoll_retail(void *c) { scoll_ctx *x = c; ((scoll_t)(uintptr_t)H[H_SET_COLL].va)(x->e, x->mode, x->kind); }
static void scoll_port(void *c) { scoll_ctx *x = c; port_world(); entity_set_collide(x->e, x->mode, x->kind); }
static void __attribute__((thiscall)) w_scoll(wl_entity *e, int32_t mode, int32_t kind)
{
    hook_t *h = &H[H_SET_COLL];
    if (h->mode == 2) { h->calls++; port_world(); entity_set_collide(e, mode, kind); return; }
    if (!sample(h)) { ((scoll_t)(uintptr_t)h->va)(e, mode, kind); return; }
    scoll_ctx x = { e, mode, kind };
    char what[128]; snprintf(what, sizeof what, "ent %p code %d mode %d kind %d", (void *)e, e->code, mode, kind);
    full_shadow(h, "entity_set_collide", scoll_retail, scoll_port, &x, what);
}
typedef void (__attribute__((thiscall)) *carry_t)(wl_entity *, int32_t, int32_t, uint32_t);
typedef struct { wl_entity *e; int32_t dx, dy; uint32_t part; } carry_ctx;
static void carry_retail(void *c) { carry_ctx *x = c; ((carry_t)(uintptr_t)H[H_CARRY].va)(x->e, x->dx, x->dy, x->part); }
static void carry_port(void *c) { carry_ctx *x = c; port_world(); entity_carry_riders(x->e, x->dx, x->dy, (uint16_t)x->part); }
static void __attribute__((thiscall)) w_carry(wl_entity *e, int32_t dx, int32_t dy, uint32_t part)
{
    hook_t *h = &H[H_CARRY];
    if (h->mode == 2) { h->calls++; port_world(); entity_carry_riders(e, dx, dy, (uint16_t)part); return; }
    if (!sample(h)) { ((carry_t)(uintptr_t)h->va)(e, dx, dy, part); return; }
    carry_ctx x = { e, dx, dy, part };
    char what[128]; snprintf(what, sizeof what, "ent %p code %d d %d,%d part %u", (void *)e, e->code, dx, dy, part & 0xffffu);
    full_shadow(h, "entity_carry_riders", carry_retail, carry_port, &x, what);
}
typedef void (__attribute__((thiscall)) *lrm_t)(wl_list *, wl_entity *);
typedef struct { wl_list *l; wl_entity *e; } lrm_ctx;
static void lrm_retail(void *c) { lrm_ctx *x = c; ((lrm_t)(uintptr_t)H[H_LIST_RM].va)(x->l, x->e); }
static void lrm_port(void *c) { lrm_ctx *x = c; port_world(); wl_list_remove(x->l, x->e); }
static void __attribute__((thiscall)) w_lrm(wl_list *l, wl_entity *e)
{
    hook_t *h = &H[H_LIST_RM];
    if (h->mode == 2) { h->calls++; port_world(); wl_list_remove(l, e); return; }
    if (!sample(h)) { ((lrm_t)(uintptr_t)h->va)(l, e); return; }
    lrm_ctx x = { l, e };
    char what[128]; snprintf(what, sizeof what, "list %p n %u ent %p", (void *)l, l->count, (void *)e);
    full_shadow(h, "wl_list_remove", lrm_retail, lrm_port, &x, what);
}

/* ---- per-character state helpers (src/char_state.c) + the jittered one-shot / channel loop ---- */
typedef int (__attribute__((thiscall)) *pit_t)(wl_char *, wl_body *, int32_t *);
typedef struct { wl_char *c; wl_body *b; int32_t *kind; int rr, rp; } pit_ctx;
static void pit_retail(void *c_) { pit_ctx *x = c_; x->rr = ((pit_t)(uintptr_t)H[H_PIT].va)(x->c, x->b, x->kind); }
static void pit_port(void *c_) { pit_ctx *x = c_; port_world(); x->rp = char_pit_check(x->c, x->b, x->kind); }
static int __attribute__((thiscall)) w_pit(wl_char *c, wl_body *b, int32_t *kind)
{
    hook_t *h = &H[H_PIT];
    if (h->mode == 2) { h->calls++; port_world(); return char_pit_check(c, b, kind); }
    if (!sample(h)) { return ((pit_t)(uintptr_t)h->va)(c, b, kind); }
    pit_ctx x = { c, b, kind, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d pit_t %u", (void *)c, c->base.code, c->pit_t);
    if (!full_shadow(h, "char_pit_check", pit_retail, pit_port, &x, what) && x.rr != x.rp) diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *resp_t)(wl_char *, wl_body *);
typedef struct { wl_char *c; wl_body *b; } resp_ctx;
static void resp_retail(void *c_) { resp_ctx *x = c_; ((resp_t)(uintptr_t)H[H_RESPAWN].va)(x->c, x->b); }
static void resp_port(void *c_) { resp_ctx *x = c_; port_world(); char_respawn_mark(x->c, x->b); }
static void __attribute__((thiscall)) w_resp(wl_char *c, wl_body *b)
{
    hook_t *h = &H[H_RESPAWN];
    if (h->mode == 2) { h->calls++; port_world(); char_respawn_mark(c, b); return; }
    if (!sample(h)) { ((resp_t)(uintptr_t)h->va)(c, b); return; }
    resp_ctx x = { c, b };
    char what[128]; snprintf(what, sizeof what, "char %p code %d x %d y %d", (void *)c, c->base.code, b->x, b->y);
    full_shadow(h, "char_respawn_mark", resp_retail, resp_port, &x, what);
    
}
typedef int (__attribute__((thiscall)) *mods_t)(wl_char *);
typedef struct { wl_char *c; int rr, rp; } mods_ctx;
static void mods_retail(void *c_) { mods_ctx *x = c_; x->rr = ((mods_t)(uintptr_t)H[H_MODS].va)(x->c); }
static void mods_port(void *c_) { mods_ctx *x = c_; port_world(); x->rp = char_mods_reset(x->c); }
static int __attribute__((thiscall)) w_mods(wl_char *c)
{
    hook_t *h = &H[H_MODS];
    if (h->mode == 2) { h->calls++; port_world(); return char_mods_reset(c); }
    if (!sample(h)) { return ((mods_t)(uintptr_t)h->va)(c); }
    mods_ctx x = { c, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d", (void *)c, c->base.code);
    if (!full_shadow(h, "char_mods_reset", mods_retail, mods_port, &x, what) && x.rr != x.rp) diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *aires_t)(wl_char *, int32_t);
typedef struct { wl_char *c; int32_t full; } aires_ctx;
static void aires_retail(void *c_) { aires_ctx *x = c_; ((aires_t)(uintptr_t)H[H_AI_RESET].va)(x->c, x->full); }
static void aires_port(void *c_) { aires_ctx *x = c_; port_world(); char_ai_reset(x->c, x->full); }
static void __attribute__((thiscall)) w_aires(wl_char *c, int32_t full)
{
    hook_t *h = &H[H_AI_RESET];
    if (h->mode == 2) { h->calls++; port_world(); char_ai_reset(c, full); return; }
    if (!sample(h)) { ((aires_t)(uintptr_t)h->va)(c, full); return; }
    aires_ctx x = { c, full };
    char what[128]; snprintf(what, sizeof what, "char %p code %d full %d", (void *)c, c->base.code, full);
    full_shadow(h, "char_ai_reset", aires_retail, aires_port, &x, what);
    
}
typedef int (__attribute__((thiscall)) *look_t)(wl_char *, uint32_t, const void *, const void *, uint32_t, int32_t, int32_t);
typedef struct { wl_char *c; uint32_t pose; const void *c0, *c1; uint32_t len; int32_t ch, a7; int rr, rp; } look_ctx;
static void look_retail(void *c_) { look_ctx *x = c_; x->rr = ((look_t)(uintptr_t)H[H_LOOKUP].va)(x->c, x->pose, x->c0, x->c1, x->len, x->ch, x->a7); }
static void look_port(void *c_) { look_ctx *x = c_; port_world(); x->rp = char_set_look_up(x->c, (int16_t)x->pose, x->c0, x->c1, (int16_t)x->len, x->ch, x->a7); }
static int __attribute__((thiscall)) w_look(wl_char *c, uint32_t pose, const void *c0, const void *c1, uint32_t len, int32_t ch, int32_t a7)
{
    hook_t *h = &H[H_LOOKUP];
    if (h->mode == 2) { h->calls++; port_world(); return char_set_look_up(c, (int16_t)pose, c0, c1, (int16_t)len, ch, a7); }
    if (!sample(h)) { return ((look_t)(uintptr_t)h->va)(c, pose, c0, c1, len, ch, a7); }
    look_ctx x = { c, pose, c0, c1, len, ch, a7, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d", (void *)c, c->base.code);
    if (!full_shadow(h, "char_set_look_up", look_retail, look_port, &x, what) && x.rr != x.rp) diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *sloop_t)(wl_entity *, uint32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *e; uint32_t i; int32_t a3; uint32_t id; int32_t vol, freq, fo, stop, period; } sloop_ctx;
static void sloop_retail(void *c_) { sloop_ctx *x = c_; ((sloop_t)(uintptr_t)H[H_SND_LOOP].va)(x->e, x->i, x->a3, x->id, x->vol, x->freq, x->fo, x->stop, x->period); }
static void sloop_port(void *c_) { sloop_ctx *x = c_; port_world(); entity_snd_loop(x->e, (uint16_t)x->i, x->a3, (uint16_t)x->id, x->vol, x->freq, x->fo, x->stop, x->period); }
static void __attribute__((thiscall)) w_sloop(wl_entity *e, uint32_t i, int32_t a3, uint32_t id, int32_t vol, int32_t freq, int32_t fo, int32_t stop, int32_t period)
{
    hook_t *h = &H[H_SND_LOOP];
    if (h->mode == 2) { h->calls++; port_world(); entity_snd_loop(e, (uint16_t)i, a3, (uint16_t)id, vol, freq, fo, stop, period); return; }
    if (!sample(h)) { ((sloop_t)(uintptr_t)h->va)(e, i, a3, id, vol, freq, fo, stop, period); return; }
    sloop_ctx x = { e, i, a3, id, vol, freq, fo, stop, period };
    char what[128]; snprintf(what, sizeof what, "ent %p code %d ch %u id %u", (void *)e, e->code, i & 0xffffu, id & 0xffffu);
    full_shadow(h, "entity_snd_loop", sloop_retail, sloop_port, &x, what);
    
}
typedef int (__attribute__((thiscall)) *sjit_t)(wl_entity *, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *e; uint32_t id; int32_t kind, va, vb, freq, jit; int rr; } sjit_ctx;
static void sjit_retail(void *c_) { sjit_ctx *x = c_; x->rr = ((sjit_t)(uintptr_t)H[H_SFX_JIT].va)(x->e, x->id, x->kind, x->va, x->vb, x->freq, x->jit); }
static void sjit_port(void *c_) { sjit_ctx *x = c_; port_world(); sfx_play_jit(x->e, (uint16_t)x->id, x->kind, x->va, x->vb, x->freq, x->jit); }
/* eax on the culls is a register nobody reads: state only */
static int __attribute__((thiscall)) w_sjit(wl_entity *e, uint32_t id, int32_t kind, int32_t va, int32_t vb, int32_t freq, int32_t jit)
{
    hook_t *h = &H[H_SFX_JIT];
    if (h->mode == 2) { h->calls++; port_world(); return sfx_play_jit(e, (uint16_t)id, kind, va, vb, freq, jit); }
    if (!sample(h)) return ((sjit_t)(uintptr_t)h->va)(e, id, kind, va, vb, freq, jit);
    sjit_ctx x = { e, id, kind, va, vb, freq, jit, 0 };
    char what[128]; snprintf(what, sizeof what, "ent %p code %d id %u kind %d", (void *)e, e->code, id & 0xffffu, kind);
    full_shadow(h, "sfx_play_jit", sjit_retail, sjit_port, &x, what);
    return x.rr;
}
typedef int (__attribute__((thiscall)) *fact_t)(wl_char *, uint32_t, int32_t);
typedef struct { wl_char *c; uint32_t pose; int32_t len; int rr, rp; } fact_ctx;
static void fact_retail(void *c_) { fact_ctx *x = c_; x->rr = ((fact_t)(uintptr_t)H[H_FORCE_ACT].va)(x->c, x->pose, x->len); }
static void fact_port(void *c_) { fact_ctx *x = c_; port_world(); x->rp = char_force_action(x->c, (int16_t)x->pose, x->len); }
static int __attribute__((thiscall)) w_fact(wl_char *c, uint32_t pose, int32_t len)
{
    hook_t *h = &H[H_FORCE_ACT];
    if (h->mode == 2) { h->calls++; port_world(); return char_force_action(c, (int16_t)pose, len); }
    if (!sample(h)) { return ((fact_t)(uintptr_t)h->va)(c, pose, len); }
    fact_ctx x = { c, pose, len, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d pose %u len %d", (void *)c, c->base.code, pose & 0xffffu, len);
    if (!full_shadow(h, "char_force_action", fact_retail, fact_port, &x, what) && x.rr != x.rp) diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef int (__attribute__((thiscall)) *ahp_t)(wl_char *, int32_t, int32_t *, int32_t);
typedef struct { wl_char *c; int32_t d; int32_t *out; int32_t dry; int rr, rp; } ahp_ctx;
static void ahp_retail(void *c_) { ahp_ctx *x = c_; x->rr = ((ahp_t)(uintptr_t)H[H_ADD_HP].va)(x->c, x->d, x->out, x->dry); }
static void ahp_port(void *c_) { ahp_ctx *x = c_; port_world(); x->rp = char_add_hp(x->c, x->d, x->out, x->dry); }
static int __attribute__((thiscall)) w_ahp(wl_char *c, int32_t d, int32_t *out, int32_t dry)
{
    hook_t *h = &H[H_ADD_HP];
    if (h->mode == 2) { h->calls++; port_world(); return char_add_hp(c, d, out, dry); }
    if (!sample(h)) { return ((ahp_t)(uintptr_t)h->va)(c, d, out, dry); }
    ahp_ctx x = { c, d, out, dry, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d d %d dry %d", (void *)c, c->base.code, d, dry);
    if (!full_shadow(h, "char_add_hp", ahp_retail, ahp_port, &x, what) && x.rr != x.rp) diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef int (__attribute__((thiscall)) *amp_t)(wl_char *, int32_t, int32_t *, int32_t);
typedef struct { wl_char *c; int32_t d; int32_t *out; int32_t dry; int rr, rp; } amp_ctx;
static void amp_retail(void *c_) { amp_ctx *x = c_; x->rr = ((amp_t)(uintptr_t)H[H_ADD_MP].va)(x->c, x->d, x->out, x->dry); }
static void amp_port(void *c_) { amp_ctx *x = c_; port_world(); x->rp = char_add_mp(x->c, x->d, x->out, x->dry); }
static int __attribute__((thiscall)) w_amp(wl_char *c, int32_t d, int32_t *out, int32_t dry)
{
    hook_t *h = &H[H_ADD_MP];
    if (h->mode == 2) { h->calls++; port_world(); return char_add_mp(c, d, out, dry); }
    if (!sample(h)) { return ((amp_t)(uintptr_t)h->va)(c, d, out, dry); }
    amp_ctx x = { c, d, out, dry, 0, 0 };
    char what[128]; snprintf(what, sizeof what, "char %p code %d d %d dry %d", (void *)c, c->base.code, d, dry);
    if (!full_shadow(h, "char_add_mp", amp_retail, amp_port, &x, what) && x.rr != x.rp) diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef int (__attribute__((thiscall)) *stuck_t)(wl_char *);
static int __attribute__((thiscall)) w_stuck(wl_char *c)
{
    hook_t *h = &H[H_ST_STUCK];
    if (h->mode == 2) { h->calls++; return char_status_stuck(c); }
    int r = ((stuck_t)(uintptr_t)h->va)(c);
    if (!sample(h)) return r;
    int p = char_status_stuck(c);
    h->compared++;
    if (p != r) diverged(h, "f%u char %p code %d: retail %d port %d", g_frame, (void *)c, c->base.code, r, p);
    return r;
}
typedef int32_t (__attribute__((thiscall)) *gridb_t)(const uint8_t *, int32_t, int32_t);
static int32_t __attribute__((thiscall)) w_gridb(const uint8_t *g, int32_t x, int32_t y)
{
    hook_t *h = &H[H_GRID_B];
    if (h->mode == 2) { h->calls++; return grid_b_word(g, x, y); }
    int32_t r = ((gridb_t)(uintptr_t)h->va)(g, x, y);
    if (!sample(h)) return r;
    int32_t p = grid_b_word(g, x, y);
    h->compared++;
    if (p != r) diverged(h, "f%u x %d y %d: retail %d port %d", g_frame, x, y, r, p);
    return r;
}
typedef int (__stdcall *slowed_t)(wl_body *);
static int __stdcall w_slowed(wl_body *b)
{
    hook_t *h = &H[H_SLOWED];
    if (h->mode == 2) { h->calls++; return body_slowed(b); }
    int r = ((slowed_t)(uintptr_t)h->va)(b);
    if (!sample(h)) return r;
    int p = body_slowed(b);
    h->compared++;
    if (p != r) diverged(h, "f%u body %p: retail %d port %d", g_frame, (void *)b, r, p);
    return r;
}
typedef int (__attribute__((thiscall)) *targ_t)(wl_char *, int32_t, int32_t);
static int __attribute__((thiscall)) w_targ(wl_char *c, int32_t a, int32_t b)
{
    hook_t *h = &H[H_TARGETABLE];
    if (h->mode == 2) { h->calls++; return char_targetable(c, a, b); }
    int r = ((targ_t)(uintptr_t)h->va)(c, a, b);
    if (!sample(h)) return r;
    int p = char_targetable(c, a, b);
    h->compared++;
    if (p != r) diverged(h, "f%u char %p code %d %d %d: retail %d port %d", g_frame, (void *)c, c->base.code, a, b, r, p);
    return r;
}
typedef int32_t (__stdcall *gridc_t)(int32_t, wl_body *);
static int32_t __stdcall w_gridc(int32_t a1, wl_body *b)
{
    hook_t *h = &H[H_GRID_C];
    if (h->mode == 2) { h->calls++; port_world(); return grid_c_at_feet(a1, b); }
    int32_t r = ((gridc_t)(uintptr_t)h->va)(a1, b);
    if (!sample(h)) return r;
    port_world();
    int32_t p = grid_c_at_feet(a1, b);
    h->compared++;
    if (p != r) diverged(h, "f%u body %p: retail %d port %d", g_frame, (void *)b, r, p);
    return r;
}

/* ---- the ambient timers + small helpers ---- */
typedef int (__attribute__((thiscall)) *amb_t)(wl_entity *, wl_body *, int32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                                               uint32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *e; wl_body *b; int32_t probe; uint32_t a[7]; int32_t on_def, on_1, on_24, vol, f0, fv, scene; int rr, rp; } amb_ctx;
static void amb_retail(void *c_)
{
    amb_ctx *x = c_;
    x->rr = ((amb_t)(uintptr_t)H[H_AMBIENT].va)(x->e, x->b, x->probe, x->a[0], x->a[1], x->a[2], x->a[3], x->a[4], x->a[5], x->a[6],
                                                x->on_def, x->on_1, x->on_24, x->vol, x->f0, x->fv, x->scene);
}
static void amb_port(void *c_)
{
    amb_ctx *x = c_;
    port_world();
    x->rp = ambient_sound_timer(x->e, x->b, x->probe, (uint16_t)x->a[0], (uint16_t)x->a[1], (uint16_t)x->a[2], (uint16_t)x->a[3],
                                (uint16_t)x->a[4], (uint16_t)x->a[5], (uint16_t)x->a[6], x->on_def, x->on_1, x->on_24, x->vol, x->f0,
                                x->fv, x->scene);
}
static int __attribute__((thiscall)) w_amb(wl_entity *e, wl_body *b, int32_t probe, uint32_t t0, uint32_t tv, uint32_t s0, uint32_t s1,
                                           uint32_t s2, uint32_t s3, uint32_t ns, int32_t on_def, int32_t on_1, int32_t on_24, int32_t vol,
                                           int32_t f0, int32_t fv, int32_t scene)
{
    hook_t *h = &H[H_AMBIENT];
    amb_ctx x = { e, b, probe, { t0, tv, s0, s1, s2, s3, ns }, on_def, on_1, on_24, vol, f0, fv, scene, 0, 0 };
    if (h->mode == 2) { h->calls++; amb_port(&x); return x.rp; }
    if (!sample_rare(h, probe == 0 && b->act_ctr <= 0)) { amb_retail(&x); return x.rr; }
    char what[96]; snprintf(what, sizeof what, "ent %p code %d ctr %d", (void *)e, e->code, b->act_ctr);
    if (!full_shadow(h, "ambient_timer", amb_retail, amb_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef void (__attribute__((thiscall)) *idlev_t)(wl_char *, int32_t, uint32_t, int32_t, uint32_t);
typedef struct { wl_char *c; int32_t probe; uint32_t grp; int32_t t0; uint32_t tr; } idlev_ctx;
static void idlev_retail(void *c_) { idlev_ctx *x = c_; ((idlev_t)(uintptr_t)H[H_IDLE_VOICE].va)(x->c, x->probe, x->grp, x->t0, x->tr); }
static void idlev_port(void *c_) { idlev_ctx *x = c_; port_world(); char_idle_voice(x->c, x->probe, (uint16_t)x->grp, x->t0, (uint16_t)x->tr); }
static void __attribute__((thiscall)) w_idlev(wl_char *c, int32_t probe, uint32_t grp, int32_t t0, uint32_t tr)
{
    hook_t *h = &H[H_IDLE_VOICE];
    idlev_ctx x = { c, probe, grp, t0, tr };
    if (h->mode == 2) { h->calls++; idlev_port(&x); return; }
    if (!sample_rare(h, probe == 0 && c->idle_voice_on != 0 && c->idle_voice_t <= 1)) { idlev_retail(&x); return; }
    char what[96]; snprintf(what, sizeof what, "char %p code %d t %u", (void *)c, c->base.code, c->idle_voice_t);
    full_shadow(h, "idle_voice", idlev_retail, idlev_port, &x, what);
}

typedef int32_t (__attribute__((thiscall)) *scat_t)(wl_entity *, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *e; int32_t a, b, c, d, rr, rp; } scat_ctx;
static void scat_retail(void *c_) { scat_ctx *x = c_; x->rr = ((scat_t)(uintptr_t)H[H_SCATTER].va)(x->e, x->a, x->b, x->c, x->d); }
static void scat_port(void *c_) { scat_ctx *x = c_; port_world(); x->rp = entity_scatter(x->e, x->a, x->b, x->c, x->d); }
static int32_t __attribute__((thiscall)) w_scat(wl_entity *e, int32_t a, int32_t b, int32_t c, int32_t d)
{
    hook_t *h = &H[H_SCATTER];
    scat_ctx x = { e, a, b, c, d, 0, 0 };
    if (h->mode == 2) { h->calls++; scat_port(&x); return x.rp; }
    if (!sample(h)) { scat_retail(&x); return x.rr; }
    char what[96]; snprintf(what, sizeof what, "ent %p code %d", (void *)e, e->code);
    if (!full_shadow(h, "entity_scatter", scat_retail, scat_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef int32_t (__attribute__((thiscall)) *smode_t)(void *, int32_t, int32_t, int32_t);
static int32_t __attribute__((thiscall)) w_smode(void *sys, int32_t a, int32_t b, int32_t c)
{
    hook_t *h = &H[H_SYS_MODE];
    if (h->mode == 2) { h->calls++; return sys_by_mode(sys, a, b, c); }
    int32_t r = ((smode_t)(uintptr_t)h->va)(sys, a, b, c);
    if (!sample(h)) return r;
    int32_t p = sys_by_mode(sys, a, b, c);
    h->compared++;
    if (p != r) diverged(h, "f%u %d %d %d: retail %d port %d", g_frame, a, b, c, r, p);
    return r;
}

/* ---- the crate cluster (src/actor_ops.c) ---- */
typedef int (__attribute__((thiscall)) *crate_t)(wl_actor *, wl_body *, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                                                 int32_t, int32_t, int32_t, uint32_t);
typedef struct { wl_actor *a; wl_body *b; int32_t probe; uint32_t part; int32_t x0, y0, hold, pushed, speed, brk, slide, ssfx; uint32_t psfx; int rr, rp; } crate_ctx;
static void crate_retail(void *c_)
{
    crate_ctx *x = c_;
    x->rr = ((crate_t)(uintptr_t)H[H_CRATE].va)(x->a, x->b, x->probe, x->part, x->x0, x->y0, x->hold, x->pushed, x->speed, x->brk, x->slide,
                                                x->ssfx, x->psfx);
}
static void crate_port(void *c_)
{
    crate_ctx *x = c_;
    port_world();
    x->rp = actor_crate(x->a, x->b, x->probe, (uint16_t)x->part, x->x0, x->y0, x->hold, x->pushed, x->speed, x->brk, x->slide, x->ssfx,
                        (uint16_t)x->psfx);
}
static int __attribute__((thiscall)) w_crate(wl_actor *a, wl_body *b, int32_t probe, uint32_t part, int32_t x0, int32_t y0, int32_t hold,
                                             int32_t pushed, int32_t speed, int32_t brk, int32_t slide, int32_t ssfx, uint32_t psfx)
{
    hook_t *h = &H[H_CRATE];
    crate_ctx x = { a, b, probe, part, x0, y0, hold, pushed, speed, brk, slide, ssfx, psfx, 0, 0 };
    if (h->mode == 2) { h->calls++; crate_port(&x); return x.rp; }
    if (!sample(h)) { crate_retail(&x); return x.rr; }
    char what[128]; snprintf(what, sizeof what, "actor %p code %d kind %d f58 %u vvel %d", (void *)a, a->base.code, a->kind, b->f58, b->vvel);
    if (!full_shadow(h, "actor_crate", crate_retail, crate_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef int (__attribute__((thiscall)) *slv_t)(wl_actor *, wl_body *, int32_t);
typedef struct { wl_actor *a; wl_body *b; int32_t sp; int rr, rp; } slv_ctx;
static void slv_retail(void *c_) { slv_ctx *x = c_; x->rr = ((slv_t)(uintptr_t)H[H_SLIDE_V].va)(x->a, x->b, x->sp); }
static void slv_port(void *c_) { slv_ctx *x = c_; port_world(); x->rp = actor_slide_v(x->a, x->b, x->sp); }
static int __attribute__((thiscall)) w_slv(wl_actor *a, wl_body *b, int32_t sp)
{
    hook_t *h = &H[H_SLIDE_V];
    slv_ctx x = { a, b, sp, 0, 0 };
    if (h->mode == 2) { h->calls++; slv_port(&x); return x.rp; }
    if (!sample(h)) { slv_retail(&x); return x.rr; }
    char what[96]; snprintf(what, sizeof what, "actor %p code %d y %d dir %d", (void *)a, a->base.code, b->y, b->vdir);
    if (!full_shadow(h, "actor_slide_v", slv_retail, slv_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef void (__attribute__((thiscall)) *brk_t)(wl_actor *, wl_body *);
typedef struct { wl_actor *a; wl_body *b; } brk_ctx;
static void brk_retail(void *c_) { brk_ctx *x = c_; ((brk_t)(uintptr_t)H[H_BREAK].va)(x->a, x->b); }
static void brk_port(void *c_) { brk_ctx *x = c_; port_world(); actor_break(x->a, x->b); }
static void __attribute__((thiscall)) w_brk(wl_actor *a, wl_body *b)
{
    hook_t *h = &H[H_BREAK];
    brk_ctx x = { a, b };
    if (h->mode == 2) { h->calls++; brk_port(&x); return; }
    if (!sample(h)) { brk_retail(&x); return; }
    char what[96]; snprintf(what, sizeof what, "actor %p code %d material %d", (void *)a, a->base.code, a->material);
    full_shadow(h, "actor_break", brk_retail, brk_port, &x, what);
}

/* pure apart from *weight: compare return + weight */
typedef int (__attribute__((thiscall)) *riders_t)(wl_entity *, int32_t, int32_t, int32_t, uint16_t *, int32_t);
static int __attribute__((thiscall)) w_riders(wl_entity *e, int32_t kind, int32_t heavy, int32_t fac, uint16_t *w, int32_t reach)
{
    hook_t *h = &H[H_RIDERS];
    if (h->mode == 2) { h->calls++; port_world(); return entity_count_riders(e, kind, heavy, fac, w, reach); }
    int r = ((riders_t)(uintptr_t)h->va)(e, kind, heavy, fac, w, reach);
    if (!sample(h)) return r;
    uint16_t pw = 0;
    port_world();
    int p = entity_count_riders(e, kind, heavy, fac, w ? &pw : NULL, reach);
    h->compared++;
    if (p != r || (w && pw != *w)) diverged(h, "f%u ent %p code %d: retail %d/%u port %d/%u", g_frame, (void *)e, e->code, r, w ? *w : 0, p, pw);
    return r;
}

typedef void (__attribute__((thiscall)) *dust_t)(wl_entity *, wl_body *, int32_t, uint32_t, int32_t, int32_t);
typedef struct { wl_entity *e; wl_body *b; int32_t a3; uint32_t n; int32_t dx, dy; } dust_ctx;
static void dust_retail(void *c_) { dust_ctx *x = c_; ((dust_t)(uintptr_t)H[H_DUST].va)(x->e, x->b, x->a3, x->n, x->dx, x->dy); }
static void dust_port(void *c_) { dust_ctx *x = c_; port_world(); entity_dust(x->e, x->b, x->a3, x->n, x->dx, x->dy); }
static void __attribute__((thiscall)) w_dust(wl_entity *e, wl_body *b, int32_t a3, uint32_t n, int32_t dx, int32_t dy)
{
    hook_t *h = &H[H_DUST];
    dust_ctx x = { e, b, a3, n, dx, dy };
    if (h->mode == 2) { h->calls++; dust_port(&x); return; }
    if (!sample(h)) { dust_retail(&x); return; }
    char what[96]; snprintf(what, sizeof what, "ent %p code %d n %u", (void *)e, e->code, n & 0xffffu);
    full_shadow(h, "entity_dust", dust_retail, dust_port, &x, what);
}

typedef void (__attribute__((thiscall)) *anchor_t)(wl_entity *, int32_t, int32_t, int32_t, int32_t *, int32_t *);
static void __attribute__((thiscall)) w_anchor(wl_entity *e, int32_t x, int32_t y, int32_t k, int32_t *ox, int32_t *oy)
{
    hook_t *h = &H[H_ANCHOR];
    if (h->mode == 2) { h->calls++; entity_anchor(e, x, y, k, ox, oy); return; }
    int32_t px = *ox, py = *oy;
    ((anchor_t)(uintptr_t)h->va)(e, x, y, k, ox, oy);
    if (!sample(h)) return;
    entity_anchor(e, x, y, k, &px, &py);
    h->compared++;
    if (px != *ox || py != *oy) diverged(h, "f%u ent %p anchor %d: retail %d,%d port %d,%d", g_frame, (void *)e, k, *ox, *oy, px, py);
}

/* ---- the particle spawner (src/particle_spawn.c): full state + return ---- */
typedef wl_entity *(__attribute__((thiscall)) *spawn_t)(wl_entity *, wl_entity *, int32_t, const wl_body *, int32_t, int32_t, int32_t,
                                                        int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *s, *o; int32_t code; const wl_body *b; int32_t a[10]; wl_entity *rr, *rp; } spawn_ctx;
static void spawn_retail(void *c_)
{
    spawn_ctx *x = c_;
    x->rr = ((spawn_t)(uintptr_t)H[H_SPAWN].va)(x->s, x->o, x->code, x->b, x->a[0], x->a[1], x->a[2], x->a[3], x->a[4], x->a[5], x->a[6],
                                                x->a[7], x->a[8], x->a[9]);
}
static void spawn_port(void *c_)
{
    spawn_ctx *x = c_;
    port_world();
    x->rp = particle_spawn(x->s, x->o, x->code, x->b, x->a[0], x->a[1], x->a[2], x->a[3], x->a[4], x->a[5], x->a[6], x->a[7], x->a[8],
                           x->a[9]);
}
static wl_entity *__attribute__((thiscall)) w_spawn(wl_entity *s, wl_entity *o, int32_t code, const wl_body *b, int32_t dx, int32_t dy,
                                                    int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13,
                                                    int32_t a14)
{
    hook_t *h = &H[H_SPAWN];
    spawn_ctx x = { s, o, code, b, { dx, dy, a7, a8, a9, a10, a11, a12, a13, a14 }, 0, 0 };
    if (h->mode == 2) { h->calls++; spawn_port(&x); return x.rp; }
    if (!sample(h)) { spawn_retail(&x); return x.rr; }
    char what[96]; snprintf(what, sizeof what, "ent %p code %d spawns %d", (void *)s, s->code, code);
    if (!full_shadow(h, "particle_spawn", spawn_retail, spawn_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %p/%p", g_frame, what, (void *)x.rr, (void *)x.rp);
    return x.rr;
}

typedef int32_t (__attribute__((thiscall)) *track_t)(wl_entity *, uint32_t, uint32_t, uint32_t, uint32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *e; uint32_t i, a, b, on; int32_t f0c, f08, f10; int rr, rp; } track_ctx;
static void track_retail(void *c_) { track_ctx *x = c_; x->rr = ((track_t)(uintptr_t)H[H_TRACK].va)(x->e, x->i, x->a, x->b, x->on, x->f0c, x->f08, x->f10); }
static void track_port(void *c_)
{
    track_ctx *x = c_;
    port_world();
    x->rp = entity_set_track(x->e, (uint16_t)x->i, (int16_t)x->a, (int16_t)x->b, (uint8_t)x->on, x->f0c, x->f08, x->f10);
}
static int32_t __attribute__((thiscall)) w_track(wl_entity *e, uint32_t i, uint32_t a, uint32_t b, uint32_t on, int32_t f0c, int32_t f08, int32_t f10)
{
    hook_t *h = &H[H_TRACK];
    track_ctx x = { e, i, a, b, on, f0c, f08, f10, 0, 0 };
    if (h->mode == 2) { h->calls++; track_port(&x); return x.rp; }
    if (!sample(h)) { track_retail(&x); return x.rr; }
    char what[96]; snprintf(what, sizeof what, "ent %p code %d i %u", (void *)e, e->code, i & 0xffffu);
    if (!full_shadow(h, "set_track", track_retail, track_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

/* ---- the grid refresh (src/grid_refresh.c): full state ---- */
typedef int32_t (__attribute__((thiscall)) *grefresh_t)(uint8_t *, int32_t);
typedef struct { uint8_t *g; int32_t keep; int32_t rr, rp; } grefresh_ctx;
static void grefresh_retail(void *c_) { grefresh_ctx *x = c_; x->rr = ((grefresh_t)(uintptr_t)H[H_GRID_REFRESH].va)(x->g, x->keep); }
static void grefresh_port(void *c_) { grefresh_ctx *x = c_; port_world(); x->rp = grid_refresh(x->g, x->keep); }
static int32_t __attribute__((thiscall)) w_grefresh(uint8_t *g, int32_t keep)
{
    hook_t *h = &H[H_GRID_REFRESH];
    grefresh_ctx x = { g, keep, 0, 0 };
    if (h->mode == 2) { h->calls++; grefresh_port(&x); return x.rp; }
    if (!sample(h)) { grefresh_retail(&x); return x.rr; }
    char what[64]; snprintf(what, sizeof what, "keep %d", keep);
    if (!full_shadow(h, "grid_refresh", grefresh_retail, grefresh_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *gmark_t)(uint8_t *, int32_t, int32_t, int32_t, int32_t);
typedef struct { uint8_t *g; int32_t x, y, w, hh; } gmark_ctx;
static void gmark_retail(void *c_) { gmark_ctx *x = c_; ((gmark_t)(uintptr_t)H[H_GRID_MARK].va)(x->g, x->x, x->y, x->w, x->hh); }
static void gmark_port(void *c_) { gmark_ctx *x = c_; grid_mark_objects(x->g, x->x, x->y, x->w, x->hh); }
static void __attribute__((thiscall)) w_gmark(uint8_t *g, int32_t x0, int32_t y0, int32_t w, int32_t hh)
{
    hook_t *h = &H[H_GRID_MARK];
    gmark_ctx x = { g, x0, y0, w, hh };
    if (h->mode == 2) { h->calls++; gmark_port(&x); return; }
    if (!sample(h)) { gmark_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "%d,%d %dx%d", x0, y0, w, hh);
    full_shadow(h, "grid_mark", gmark_retail, gmark_port, &x, what);
}

/* ---- the particle init (src/particle_init.c) ---- */
typedef int32_t (__attribute__((thiscall)) *pinit_t)(wl_particle *, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t, wl_entity *,
                                                     int32_t, int32_t, int32_t);
typedef struct { wl_particle *p; int32_t code, x, y, facing; uint32_t delay; int32_t anchor; wl_entity *owner; int32_t angle, prio, fac; int32_t rr, rp; } pinit_ctx;
static void pinit_retail(void *c_)
{
    pinit_ctx *x = c_;
    x->rr = ((pinit_t)(uintptr_t)H[H_PINIT].va)(x->p, x->code, x->x, x->y, x->facing, x->delay, x->anchor, x->owner, x->angle, x->prio, x->fac);
}
static void pinit_port(void *c_)
{
    pinit_ctx *x = c_;
    port_world();
    particle_init(x->p, x->code, x->x, x->y, x->facing, x->delay, x->anchor, x->owner, x->angle, x->prio, x->fac);
}
static long g_pinit_codes[160]; static int32_t g_pinit_code_id[160]; static int g_npinit_codes;
static int32_t __attribute__((thiscall)) w_pinit(wl_particle *p, int32_t code, int32_t x0, int32_t y0, int32_t facing, uint32_t delay,
                                                 int32_t anchor, wl_entity *owner, int32_t angle, int32_t prio, int32_t fac)
{
    hook_t *h = &H[H_PINIT];
    pinit_ctx x = { p, code, x0, y0, facing, delay, anchor, owner, angle, prio, fac, 0, 0 };
    if (h->mode == 2) { h->calls++; pinit_port(&x); return 0; }
    int k = 0;
    while (k < g_npinit_codes && g_pinit_code_id[k] != code) k++;
    if (k == g_npinit_codes && k < 160) { g_pinit_code_id[k] = code; g_npinit_codes++; }
    int rare = k < 160 && g_pinit_codes[k] < 64;
    if (!sample_rare(h, rare)) { pinit_retail(&x); return x.rr; }
    if (k < 160) g_pinit_codes[k]++;
    char what[96]; snprintf(what, sizeof what, "particle %p code %d delay %u anchor %d", (void *)p, code, delay & 0xffffu, anchor);
    full_shadow(h, "particle_init", pinit_retail, pinit_port, &x, what);   /* eax: a register nobody reads */
    return x.rr;
}

typedef void (__attribute__((thiscall)) *esetup_t)(wl_entity *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                                                   uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
typedef struct { wl_entity *e; int32_t a[8]; uint32_t on; int32_t dx, dy, anchor, tint; uint32_t nfx; } esetup_ctx;
static void esetup_retail(void *c_)
{
    esetup_ctx *x = c_;
    ((esetup_t)(uintptr_t)H[H_ESETUP].va)(x->e, x->a[0], x->a[1], x->a[2], x->a[3], x->a[4], x->a[5], x->a[6], x->a[7], x->on, x->dx, x->dy,
                                          x->anchor, x->tint, x->nfx);
}
static void esetup_port(void *c_)
{
    esetup_ctx *x = c_;
    port_world();
    entity_setup(x->e, x->a[0], x->a[1], x->a[2], x->a[3], x->a[4], x->a[5], x->a[6], x->a[7], (uint8_t)x->on, x->dx, x->dy, x->anchor,
                 x->tint, (int16_t)x->nfx);
}
static void __attribute__((thiscall)) w_esetup(wl_entity *e, int32_t kind, int32_t code, int32_t fac, int32_t x0, int32_t y0, int32_t w,
                                               int32_t hh, int32_t facing, uint32_t on, int32_t dx, int32_t dy, int32_t anchor, int32_t tint,
                                               uint32_t nfx)
{
    hook_t *h = &H[H_ESETUP];
    esetup_ctx x = { e, { kind, code, fac, x0, y0, w, hh, facing }, on, dx, dy, anchor, tint, nfx };
    if (h->mode == 2) { h->calls++; esetup_port(&x); return; }
    if (!sample(h)) { esetup_retail(&x); return; }
    char what[96]; snprintf(what, sizeof what, "ent %p code %d anchor %d", (void *)e, code, anchor);
    full_shadow(h, "entity_setup", esetup_retail, esetup_port, &x, what);
}

typedef void (__attribute__((thiscall)) *ride_t)(wl_entity *, wl_entity *);
typedef struct { wl_entity *e, *par; } ride_ctx;
static void ride_retail(void *c_) { ride_ctx *x = c_; ((ride_t)(uintptr_t)H[H_RIDE].va)(x->e, x->par); }
static void ride_port(void *c_) { ride_ctx *x = c_; entity_ride(x->e, x->par); }
static void __attribute__((thiscall)) w_ride(wl_entity *e, wl_entity *par)
{
    hook_t *h = &H[H_RIDE];
    ride_ctx x = { e, par };
    if (h->mode == 2) { h->calls++; ride_port(&x); return; }
    if (!sample(h)) { ride_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "ent %p", (void *)e);
    full_shadow(h, "entity_ride", ride_retail, ride_port, &x, what);
}
typedef void (__attribute__((thiscall)) *ridep_t)(wl_entity *, const wl_body *, uint32_t);
typedef struct { wl_entity *e; const wl_body *b; uint32_t part; } ridep_ctx;
static void ridep_retail(void *c_) { ridep_ctx *x = c_; ((ridep_t)(uintptr_t)H[H_RIDE_PART].va)(x->e, x->b, x->part); }
static void ridep_port(void *c_) { ridep_ctx *x = c_; entity_ride_part(x->e, x->b, (uint16_t)x->part); }
static void __attribute__((thiscall)) w_ridep(wl_entity *e, const wl_body *b, uint32_t part)
{
    hook_t *h = &H[H_RIDE_PART];
    ridep_ctx x = { e, b, part };
    if (h->mode == 2) { h->calls++; ridep_port(&x); return; }
    if (!sample(h)) { ridep_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "ent %p part %u", (void *)e, part & 0xffffu);
    full_shadow(h, "entity_ride_part", ridep_retail, ridep_port, &x, what);
}

/* the x87 polar steps: pure, compare both outputs */
typedef int32_t (__stdcall *pdiv_t)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t *, int32_t *);
static int32_t __stdcall w_pdiv(int32_t r, int32_t ang, int32_t n, int32_t cx, int32_t cy, int32_t *ox, int32_t *oy)
{
    hook_t *h = &H[H_POLAR_DIV];
    if (h->mode == 2) { h->calls++; polar_div(r, ang, n, cx, cy, ox, oy); return 0; }
    int32_t rv = ((pdiv_t)(uintptr_t)h->va)(r, ang, n, cx, cy, ox, oy);
    if (!sample(h)) return rv;
    int32_t px, py;
    polar_div(r, ang, n, cx, cy, &px, &py);
    h->compared++;
    if (px != *ox || py != *oy) diverged(h, "f%u r %d ang %d n %d: retail %d,%d port %d,%d", g_frame, r, ang, n, *ox, *oy, px, py);
    return rv;
}
typedef int32_t (__stdcall *pmil_t)(int32_t, int32_t, int32_t, int32_t, int32_t *, int32_t *);
static int32_t __stdcall w_pmil(int32_t cx, int32_t cy, int32_t r, int32_t ang, int32_t *ox, int32_t *oy)
{
    hook_t *h = &H[H_POLAR_MILLI];
    if (h->mode == 2) { h->calls++; polar_milli(cx, cy, r, ang, ox, oy); return 0; }
    int32_t rv = ((pmil_t)(uintptr_t)h->va)(cx, cy, r, ang, ox, oy);
    if (!sample(h)) return rv;
    int32_t px, py;
    polar_milli(cx, cy, r, ang, &px, &py);
    h->compared++;
    if (px != *ox || py != *oy) diverged(h, "f%u r %d ang %d: retail %d,%d port %d,%d", g_frame, r, ang, *ox, *oy, px, py);
    return rv;
}
typedef int32_t (__stdcall *fade_t)(int32_t, int32_t);
static int32_t fade_shadow(hook_t *h, int32_t (*port)(int32_t, int32_t), int32_t k, int32_t n)
{
    if (h->mode == 2) { h->calls++; return port(k, n); }
    int32_t r = ((fade_t)(uintptr_t)h->va)(k, n);
    if (!sample(h)) return r;
    int32_t p = port(k, n);
    h->compared++;
    if (p != r) diverged(h, "f%u %d/%d: retail %d port %d", g_frame, k, n, r, p);
    return r;
}
static int32_t __stdcall w_fade_a(int32_t k, int32_t n) { return fade_shadow(&H[H_FADE_A], tint_fade_a, k, n); }
static int32_t __stdcall w_fade_b(int32_t k, int32_t n) { return fade_shadow(&H[H_FADE_B], tint_fade_b, k, n); }

typedef int32_t (__attribute__((thiscall)) *trkon_t)(wl_entity *, uint32_t, uint32_t, uint32_t);
typedef struct { wl_entity *e; uint32_t i, a, b; int32_t rr, rp; } trkon_ctx;
static void trkon_retail(void *c_) { trkon_ctx *x = c_; x->rr = ((trkon_t)(uintptr_t)H[H_TRACK_ON].va)(x->e, x->i, x->a, x->b); }
static void trkon_port(void *c_) { trkon_ctx *x = c_; x->rp = entity_track_on(x->e, (uint16_t)x->i, (int16_t)x->a, (int16_t)x->b); }
static int32_t __attribute__((thiscall)) w_trkon(wl_entity *e, uint32_t i, uint32_t a, uint32_t b)
{
    hook_t *h = &H[H_TRACK_ON];
    trkon_ctx x = { e, i, a, b, 0, 0 };
    if (h->mode == 2) { h->calls++; trkon_port(&x); return x.rp; }
    if (!sample(h)) { trkon_retail(&x); return x.rr; }
    char what[64]; snprintf(what, sizeof what, "ent %p i %u", (void *)e, i & 0xffffu);
    if (!full_shadow(h, "track_on", trkon_retail, trkon_port, &x, what) && x.rr != x.rp) diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef int (__attribute__((thiscall)) *salloc_t)(wl_entity *, uint32_t);
typedef struct { wl_entity *e; uint32_t n; int rr, rp; } salloc_ctx;
static void salloc_retail(void *c_) { salloc_ctx *x = c_; x->rr = ((salloc_t)(uintptr_t)H[H_SND_ALLOC].va)(x->e, x->n); }
static void salloc_port(void *c_) { salloc_ctx *x = c_; port_world(); x->rp = entity_snd_alloc(x->e, (uint16_t)x->n); }
static int __attribute__((thiscall)) w_salloc(wl_entity *e, uint32_t n)
{
    hook_t *h = &H[H_SND_ALLOC];
    salloc_ctx x = { e, n, 0, 0 };
    if (h->mode == 2) { h->calls++; salloc_port(&x); return x.rp; }
    if (!sample(h)) { salloc_retail(&x); return x.rr; }
    char what[64]; snprintf(what, sizeof what, "ent %p n %u", (void *)e, n & 0xffffu);
    full_shadow(h, "snd_alloc", salloc_retail, salloc_port, &x, what);
    return x.rr;
}

typedef void (__stdcall *jit_t)(wl_body *);
static void jit_retail(void *c_) { ((jit_t)(uintptr_t)H[H_JITTER].va)(c_); }
static void jit_port(void *c_) { body_clip_jitter(c_); }
static void __stdcall w_jit(wl_body *b)
{
    hook_t *h = &H[H_JITTER];
    if (h->mode == 2) { h->calls++; body_clip_jitter(b); return; }
    if (!sample(h)) { jit_retail(b); return; }
    char what[64]; snprintf(what, sizeof what, "body %p", (void *)b);
    full_shadow(h, "clip_jitter", jit_retail, jit_port, b, what);
}

typedef void *(__attribute__((thiscall)) *pfree_t)(void *, uint32_t);
typedef struct { void *t; uint32_t del; } pfree_ctx;
static void pfree_retail(void *c_) { pfree_ctx *x = c_; ((pfree_t)(uintptr_t)H[H_PARTS_FREE].va)(x->t, x->del); }
static void pfree_port(void *c_) { pfree_ctx *x = c_; parts_free(x->t, (int32_t)(x->del & 0xff)); }
static void *__attribute__((thiscall)) w_pfree(void *t, uint32_t del)
{
    hook_t *h = &H[H_PARTS_FREE];
    pfree_ctx x = { t, del };
    if (h->mode == 2) { h->calls++; pfree_port(&x); return t; }
    if (!sample(h)) { pfree_retail(&x); return t; }
    char what[64]; snprintf(what, sizeof what, "parts %p del %u", t, del & 0xffu);
    full_shadow(h, "parts_free", pfree_retail, pfree_port, &x, what);
    return t;
}

typedef wl_body *(__stdcall *sang_t)(wl_body *, int32_t, int32_t);
static wl_body *__stdcall w_sang(wl_body *b, int32_t a, int32_t m)
{
    hook_t *h = &H[H_SET_ANGLE];
    if (h->mode == 2) { h->calls++; body_set_angle(b, a, m); return b; }
    int32_t pre = b->angle;
    ((sang_t)(uintptr_t)h->va)(b, a, m);
    if (!sample(h)) return b;
    int32_t post = b->angle;
    b->angle = pre;
    body_set_angle(b, a, m);
    h->compared++;
    if (b->angle != post) diverged(h, "f%u a %d m %d facing %d: retail %d port %d", g_frame, a, m, b->facing, post, b->angle);
    b->angle = post;
    return b;
}

typedef int (__attribute__((thiscall)) *fxr_t)(wl_entity *, uint32_t);
typedef struct { wl_entity *e; uint32_t n; } fxr_ctx;
static void fxr_retail(void *c_) { fxr_ctx *x = c_; ((fxr_t)(uintptr_t)H[H_FX_RESET].va)(x->e, x->n); }
static void fxr_port(void *c_) { fxr_ctx *x = c_; entity_fx_reset(x->e, (int16_t)x->n); }
static int __attribute__((thiscall)) w_fxr(wl_entity *e, uint32_t n)
{
    hook_t *h = &H[H_FX_RESET];
    fxr_ctx x = { e, n };
    if (h->mode == 2) { h->calls++; fxr_port(&x); return 0; }
    if (!sample(h)) { fxr_retail(&x); return 0; }
    char what[64]; snprintf(what, sizeof what, "ent %p n %u", (void *)e, n & 0xffffu);
    full_shadow(h, "fx_reset", fxr_retail, fxr_port, &x, what);
    return 0;
}

/* ---- skill availability / party lookup / floor below: pure, return compared ---- */
typedef int (__attribute__((thiscall)) *savail_t)(wl_char *, const wl_skill *, const wl_body *, int32_t, int32_t);
static int __attribute__((thiscall)) w_savail(wl_char *c, const wl_skill *k, const wl_body *b, int32_t gr, int32_t probing)
{
    hook_t *h = &H[H_SKILL_AVAIL];
    if (h->mode == 2) { h->calls++; return char_skill_avail(c, k, b, gr, probing); }
    int r = ((savail_t)(uintptr_t)h->va)(c, k, b, gr, probing);
    if (!sample(h)) return r;
    int p = char_skill_avail(c, k, b, gr, probing);
    h->compared++;
    if (p != r) diverged(h, "f%u char %p skill %d: retail %d port %d", g_frame, (void *)c, k ? k->id : 0, r, p);
    return r;
}
typedef wl_char *(__attribute__((thiscall)) *pfind_t)(const wl_char *, int32_t, uint32_t, uint16_t *, uint32_t);
static wl_char *__attribute__((thiscall)) w_pfind(const wl_char *c, int32_t code, uint32_t pose, uint16_t *f58, uint32_t st)
{
    hook_t *h = &H[H_PARTY_FIND];
    if (h->mode == 2) { h->calls++; return party_find(c, code, (int16_t)pose, f58, (uint16_t)st); }
    uint16_t pre = f58 ? *f58 : 0;
    wl_char *r = ((pfind_t)(uintptr_t)h->va)(c, code, pose, f58, st);
    if (!sample(h)) return r;
    uint16_t rf = f58 ? *f58 : 0, pf = pre;
    wl_char *p = party_find(c, code, (int16_t)pose, f58 ? &pf : NULL, (uint16_t)st);
    h->compared++;
    if (p != r || pf != rf) diverged(h, "f%u code %d pose %u: retail %p/%u port %p/%u", g_frame, code, pose & 0xffffu, (void *)r, rf, (void *)p, pf);
    return r;
}
typedef int32_t (__stdcall *fbelow_t)(const wl_body *, int32_t *);
static int32_t __stdcall w_fbelow(const wl_body *b, int32_t *row)
{
    hook_t *h = &H[H_FLOOR_BELOW];
    if (h->mode == 2) { h->calls++; port_world(); return grid_floor_below(b, row); }
    int32_t pre = row ? *row : 0;
    int32_t r = ((fbelow_t)(uintptr_t)h->va)(b, row);
    if (!sample(h)) return r;
    int32_t rr = row ? *row : 0, pr = pre;
    port_world();
    int32_t p = grid_floor_below(b, row ? &pr : NULL);
    h->compared++;
    if (p != r || pr != rr) diverged(h, "f%u body %p: retail %d/%d port %d/%d", g_frame, (void *)b, r, rr, p, pr);
    return r;
}

/* ---- the gimmick pass (src/gimmick.c) — full state; the pure rect queries compare their return ---- */
typedef int (__attribute__((thiscall)) *gpass_t)(wl_roomloop *, wl_char *);
typedef struct { wl_roomloop *rl; wl_char *pl; int rr, rp; } gpass_ctx;
static void gpass_retail(void *c) { gpass_ctx *x = c; x->rr = ((gpass_t)(uintptr_t)H[H_GPASS].va)(x->rl, x->pl); }
static void gpass_port(void *c) { gpass_ctx *x = c; port_world(); x->rp = gimmick_pass(x->rl, x->pl); }
static int __attribute__((thiscall)) w_gpass(wl_roomloop *rl, wl_char *pl)
{
    hook_t *h = &H[H_GPASS];
    gpass_ctx x = { rl, pl, 0, 0 };
    if (h->mode == 2) { h->calls++; gpass_port(&x); return x.rp; }
    if (!sample(h)) { gpass_retail(&x); return x.rr; }
    char what[64]; snprintf(what, sizeof what, "player %p", (void *)pl);
    if (!full_shadow(h, "gimmick_pass", gpass_retail, gpass_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

/* a gimmick step that will switch something (the trigger condition holds) is shadowed on every
 * call; the idle ones @N */
static int gstep_live(const wl_actor *a, const wl_char *pl)
{
    if (a->hold != 0) return 0;
    if (a->hit != 0) return 1;
    int32_t code = a->base.code, st = a->base.f00c;
    if (code == 70130 || code == 70100) {
        port_world();
        int ov = part_left(&a->base, 0) <= part_right(&pl->base, 0) && part_right(&a->base, 0) > part_left(&pl->base, 0) &&
                 char_part_top((const wl_char *)a, 0) <= part_bottom(&pl->base, 0) && part_bottom(&a->base, 0) > char_part_top(pl, 0);
        return st == 0 ? ov : code == 70130 && st == 1 && !ov;
    }
    return code == 70215 || code == 70600 || code == 70800 || code == 70810 || code == 70830;
}
typedef int (__attribute__((thiscall)) *gstep_t)(wl_actor *, wl_char *);
typedef struct { wl_actor *a; wl_char *pl; int rr, rp; } gstep_ctx;
static void gstep_retail(void *c) { gstep_ctx *x = c; x->rr = ((gstep_t)(uintptr_t)H[H_GSTEP].va)(x->a, x->pl); }
static void gstep_port(void *c) { gstep_ctx *x = c; port_world(); x->rp = gimmick_step(x->a, x->pl); }
static long g_gstep_live;
static int __attribute__((thiscall)) w_gstep(wl_actor *a, wl_char *pl)
{
    hook_t *h = &H[H_GSTEP];
    gstep_ctx x = { a, pl, 0, 0 };
    if (h->mode == 2) { h->calls++; gstep_port(&x); return x.rp; }
    int live = !rh_in_shadow() && gstep_live(a, pl);
    if (!sample_rare(h, live)) { gstep_retail(&x); return x.rr; }
    if (live) g_gstep_live++;
    char what[128]; snprintf(what, sizeof what, "actor %p code %d state %d hit %d", (void *)a, a->base.code, a->base.f00c, a->hit);
    if (!full_shadow(h, "gimmick_step", gstep_retail, gstep_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

typedef int (__attribute__((thiscall)) *swset_t)(wl_actor *, int32_t, int32_t);
typedef struct { wl_actor *a; int32_t v, q; int rr, rp; int link; } swset_ctx;
static void swset_retail(void *c) { swset_ctx *x = c; x->rr = ((swset_t)(uintptr_t)H[x->link ? H_SWLINK : H_SWSET].va)(x->a, x->v, x->q); }
static void swset_port(void *c) { swset_ctx *x = c; port_world(); x->rp = x->link ? switch_link(x->a, x->v, x->q) : switch_set(x->a, x->v, x->q); }
static int sw_hook(int link, wl_actor *a, int32_t v, int32_t q)
{
    hook_t *h = &H[link ? H_SWLINK : H_SWSET];
    swset_ctx x = { a, v, q, 0, 0, link };
    if (h->mode == 2) { h->calls++; swset_port(&x); return x.rp; }
    if (!sample(h)) { swset_retail(&x); return x.rr; }
    char what[128]; snprintf(what, sizeof what, "actor %p code %d state %d -> %d quiet %d", (void *)a, a->base.code, a->base.f00c, v, q);
    if (!full_shadow(h, link ? "switch_link" : "switch_set", swset_retail, swset_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
static int __attribute__((thiscall)) w_swset(wl_actor *a, int32_t v, int32_t q) { return sw_hook(0, a, v, q); }
static int __attribute__((thiscall)) w_swlink(wl_actor *a, int32_t v, int32_t q) { return sw_hook(1, a, v, q); }

/* the event runner and the unload release retail's DirectDraw / DirectSound / GDI objects: a
 * shadow whose port phase reaches one of those releases is aborted (guarded engine calls below)
 * and not compared — the room-change calls are proven by REPLACE (the corpus hash) */
typedef int (__attribute__((thiscall)) *evrun_t)(wl_event *, void *, wl_room *, wl_actor *, int32_t, int32_t, int32_t);
typedef struct { wl_event *ev; void *f00; wl_room *room; wl_actor *a; int32_t a5, f10, mode; int rr, rp; } evrun_ctx;
static void evrun_retail(void *c) { evrun_ctx *x = c; x->rr = ((evrun_t)(uintptr_t)H[H_EVRUN].va)(x->ev, x->f00, x->room, x->a, x->a5, x->f10, x->mode); }
static void evrun_port(void *c) { evrun_ctx *x = c; port_world(); x->rp = event_run(x->ev, x->f00, x->room, x->a, x->a5, x->f10, x->mode); }
static int __attribute__((thiscall)) w_evrun(wl_event *ev, void *f00, wl_room *room, wl_actor *a, int32_t a5, int32_t f10, int32_t mode)
{
    hook_t *h = &H[H_EVRUN];
    evrun_ctx x = { ev, f00, room, a, a5, f10, mode, 0, 0 };
    if (h->mode == 2) { h->calls++; evrun_port(&x); return x.rp; }
    if (!sample(h)) { evrun_retail(&x); return x.rr; }
    char what[96]; snprintf(what, sizeof what, "actor %p code %d mode %d", (void *)a, a ? a->base.code : 0, mode);
    if (!full_shadow(h, "event_run", evrun_retail, evrun_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *unload_t)(void *, uint32_t);
typedef struct { void *sys; uint32_t group; } unload_ctx;
static void unload_retail(void *c) { unload_ctx *x = c; ((unload_t)(uintptr_t)H[H_UNLOAD].va)(x->sys, x->group); }
static void unload_port(void *c) { unload_ctx *x = c; sys_unload(x->sys, (uint16_t)x->group); }
static void __attribute__((thiscall)) w_unload(void *sys, uint32_t group)
{
    hook_t *h = &H[H_UNLOAD];
    unload_ctx x = { sys, group };
    if (h->mode == 2) { h->calls++; unload_port(&x); return; }
    if (!sample(h)) { unload_retail(&x); return; }
    full_shadow(h, "sys_unload", unload_retail, unload_port, &x, "unload");
}

/* engine calls with an external effect: in a shadow's port phase they abort the comparison */
#define GUARD(what) if (rh_shadow_port_phase()) { rh_shadow_abort(what); return 0; }
static int __attribute__((thiscall)) gd_5b9390(void *f) { GUARD("the surface free 0x5b9390") return ((eng_tc0)(uintptr_t)0x5b9390u)(f); }
static int __attribute__((thiscall)) gd_5bb3a0(void *b) { GUARD("the buffer release 0x5bb3a0") return ((eng_tc0)(uintptr_t)0x5bb3a0u)(b); }
static int (__stdcall *r_DeleteObject)(void *);
static int __stdcall gd_delobj(void *o) { GUARD("DeleteObject") return r_DeleteObject(o); }
static int __attribute__((thiscall)) gd_40fe00(void *s) { GUARD("the input re-poll 0x40fe00") return ((eng_tc0)(uintptr_t)0x40fe00u)(s); }
static int __attribute__((thiscall)) gd_5ba4c0(void *d) { GUARD("DirectInput 0x5ba4c0") return ((eng_tc0)(uintptr_t)0x5ba4c0u)(d); }
static int __attribute__((thiscall)) gd_5ba2d0(void *d) { GUARD("DirectInput 0x5ba2d0") return ((eng_tc0)(uintptr_t)0x5ba2d0u)(d); }
static int __attribute__((thiscall)) gd_5ba650(void *d) { GUARD("DirectInput 0x5ba650") return ((eng_tc0)(uintptr_t)0x5ba650u)(d); }
static int __attribute__((thiscall)) gd_40a5d0(void *s, intptr_t id, intptr_t a, intptr_t f, intptr_t fade, intptr_t b)
{ GUARD("the BGM change 0x40a5d0") return ((eng_tc5)(uintptr_t)0x40a5d0u)(s, id, a, f, fade, b); }
static int __attribute__((thiscall)) gd_49bf90(void *d, intptr_t v) { GUARD("the joystick read 0x49bf90") return ((eng_tc1)(uintptr_t)0x49bf90u)(d, v); }
/* the script engine: the room-start (mode 0) and touch (mode 2) scripts drive DirectShow /
 * DirectDraw (BGM, room changes) — running them a second time perturbs the run; the switch /
 * hit scripts (3 / 4) are state-only in the corpus (shadowed both sides, corpus hash unchanged) */
static int __attribute__((thiscall)) gd_40c380(wl_event *ev)
{
    if (ev->mode != 3 && ev->mode != 4) { GUARD("the script engine 0x40c380 (mode != 3/4)") }
    return ((eng_tc0)(uintptr_t)0x40c380u)(ev);
}
#undef GUARD

typedef void (__attribute__((thiscall)) *unlall_t)(wl_actor *);
static void unlall_retail(void *c) { ((unlall_t)(uintptr_t)H[H_UNLIST_ALL].va)(c); }
static void unlall_port(void *c) { port_world(); actor_unlist_all(c); }
static void __attribute__((thiscall)) w_unlall(wl_actor *a)
{
    hook_t *h = &H[H_UNLIST_ALL];
    if (h->mode == 2) { h->calls++; unlall_port(a); return; }
    if (!sample(h)) { unlall_retail(a); return; }
    char what[64];
    full_shadow(h, "actor_unlist_all", unlall_retail, unlall_port, a, WHAT(&a->base));
}

typedef void (__attribute__((thiscall)) *glow_t)(wl_actor *, int32_t, int32_t);
typedef struct { wl_actor *a; int32_t v, burst; } glow_ctx;
static void glow_retail(void *c) { glow_ctx *x = c; ((glow_t)(uintptr_t)H[H_GLOW].va)(x->a, x->v, x->burst); }
static void glow_port(void *c) { glow_ctx *x = c; port_world(); switch_glow(x->a, x->v, x->burst); }
static void __attribute__((thiscall)) w_glow(wl_actor *a, int32_t v, int32_t burst)
{
    hook_t *h = &H[H_GLOW];
    glow_ctx x = { a, v, burst };
    if (h->mode == 2) { h->calls++; glow_port(&x); return; }
    if (!sample(h)) { glow_retail(&x); return; }
    char what[64];
    full_shadow(h, "switch_glow", glow_retail, glow_port, &x, WHAT(&a->base));
}

typedef int (__attribute__((thiscall)) *rhit_t)(const wl_entity *, const int32_t *, uint32_t);
static int __attribute__((thiscall)) w_rhit(const wl_entity *e, const int32_t *r, uint32_t part)
{
    hook_t *h = &H[H_RECT_HIT];
    if (h->mode == 2) { h->calls++; return entity_rect_hit(e, r, (uint16_t)part); }
    int ret = ((rhit_t)(uintptr_t)h->va)(e, r, part);
    if (!sample(h)) return ret;
    int p = entity_rect_hit(e, r, (uint16_t)part);
    h->compared++;
    if (p != ret) diverged(h, "f%u ent %p: retail %d port %d", g_frame, (void *)e, ret, p);
    return ret;
}
typedef uint32_t (__stdcall *ccount_t)(uint32_t, const int32_t *, uint32_t, uint32_t);
static uint32_t __stdcall w_ccount(uint32_t part, const int32_t *r, uint32_t lo, uint32_t hi)
{
    hook_t *h = &H[H_CHARS_COUNT];
    if (h->mode == 2) { h->calls++; port_world(); return chars_in_rect_count((uint16_t)part, r, lo, hi); }
    uint32_t ret = ((ccount_t)(uintptr_t)h->va)(part, r, lo, hi);
    if (!sample(h)) return ret;
    port_world();
    uint16_t p = chars_in_rect_count((uint16_t)part, r, lo, hi);
    h->compared++;
    if (p != (uint16_t)ret) diverged(h, "f%u ids %u..%u: retail %u port %u", g_frame, lo, hi, ret & 0xffffu, p);
    return ret;
}
typedef int (__stdcall *cin_t)(uint32_t, const int32_t *, int32_t);
static int __stdcall w_cin(uint32_t part, const int32_t *r, int32_t leader)
{
    hook_t *h = &H[H_CHAR_IN];
    if (h->mode == 2) { h->calls++; port_world(); return char_in_rect((uint16_t)part, r, leader); }
    int ret = ((cin_t)(uintptr_t)h->va)(part, r, leader);
    if (!sample(h)) return ret;
    port_world();
    int p = char_in_rect((uint16_t)part, r, leader);
    h->compared++;
    if (p != ret) diverged(h, "f%u rect %d,%d %dx%d: retail %d port %d", g_frame, r[0], r[1], r[2], r[3], ret, p);
    return ret;
}
typedef wl_entity *(__attribute__((thiscall)) *riding_t)(const wl_actor *, int32_t, uint16_t *);
static wl_entity *__attribute__((thiscall)) w_riding(const wl_actor *a, int32_t leader, uint16_t *st)
{
    hook_t *h = &H[H_RIDING];
    if (h->mode == 2) { h->calls++; port_world(); return char_riding(a, leader, st); }
    uint16_t pre = st ? *st : 0;
    wl_entity *ret = ((riding_t)(uintptr_t)h->va)(a, leader, st);
    if (!sample(h)) return ret;
    uint16_t rs = st ? *st : 0, ps = pre;
    port_world();
    wl_entity *p = char_riding(a, leader, st ? &ps : NULL);
    h->compared++;
    if (p != ret || ps != rs) diverged(h, "f%u actor %p: retail %p/%u port %p/%u", g_frame, (void *)a, (void *)ret, rs, (void *)p, ps);
    return ret;
}
typedef wl_entity *(__stdcall *pfnd_t)(const wl_entity *, int32_t);
static wl_entity *__stdcall w_pfnd(const wl_entity *owner, int32_t code)
{
    hook_t *h = &H[H_PART_FIND];
    if (h->mode == 2) { h->calls++; port_world(); return particle_find(owner, code); }
    wl_entity *ret = ((pfnd_t)(uintptr_t)h->va)(owner, code);
    if (!sample(h)) return ret;
    port_world();
    wl_entity *p = particle_find(owner, code);
    h->compared++;
    if (p != ret) diverged(h, "f%u owner %p code %d: retail %p port %p", g_frame, (void *)owner, code, (void *)ret, (void *)p);
    return ret;
}
typedef uint32_t (__attribute__((thiscall)) *serial_t)(wl_world *);
typedef struct { wl_world *w; uint32_t rr, rp; } serial_ctx;
static void serial_retail(void *c) { serial_ctx *x = c; x->rr = ((serial_t)(uintptr_t)H[H_SERIAL].va)(x->w); }
static void serial_port(void *c) { serial_ctx *x = c; port_world(); x->rp = world_next_serial(x->w); }
static uint32_t __attribute__((thiscall)) w_serial(wl_world *w)
{
    hook_t *h = &H[H_SERIAL];
    serial_ctx x = { w, 0, 0 };
    if (h->mode == 2) { h->calls++; serial_port(&x); return x.rp; }
    if (!sample(h)) { serial_retail(&x); return x.rr; }
    if (!full_shadow(h, "next_serial", serial_retail, serial_port, &x, "serial") && x.rr != x.rp)
        diverged(h, "f%u ret %u/%u", g_frame, x.rr, x.rp);
    return x.rr;
}
typedef int (__attribute__((thiscall)) *shot_t)(wl_actor *, wl_body *, int32_t, int32_t);
typedef struct { wl_actor *a; wl_body *b; int32_t a3, pw; int rr, rp; } shot_ctx;
static void shot_retail(void *c) { shot_ctx *x = c; x->rr = ((shot_t)(uintptr_t)H[H_SHOT].va)(x->a, x->b, x->a3, x->pw); }
static void shot_port(void *c) { shot_ctx *x = c; port_world(); x->rp = actor_shot(x->a, x->b, x->a3, x->pw); }
static int __attribute__((thiscall)) w_shot(wl_actor *a, wl_body *b, int32_t a3, int32_t pw)
{
    hook_t *h = &H[H_SHOT];
    shot_ctx x = { a, b, a3, pw, 0, 0 };
    if (h->mode == 2) { h->calls++; shot_port(&x); return x.rp; }
    if (!sample(h)) { shot_retail(&x); return x.rr; }
    char what[64];
    if (!full_shadow(h, "actor_shot", shot_retail, shot_port, &x, WHAT(&a->base)) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *warp_t)(wl_actor *, int32_t);
typedef struct { wl_actor *a; int32_t kind; } warp_ctx;
static void warp_retail(void *c) { warp_ctx *x = c; ((warp_t)(uintptr_t)H[H_WARP].va)(x->a, x->kind); }
static void warp_port(void *c) { warp_ctx *x = c; port_world(); warp_request(x->a, x->kind); }
static void __attribute__((thiscall)) w_warp(wl_actor *a, int32_t kind)
{
    hook_t *h = &H[H_WARP];
    warp_ctx x = { a, kind };
    if (h->mode == 2) { h->calls++; warp_port(&x); return; }
    if (!sample(h)) { warp_retail(&x); return; }
    char what[64];
    full_shadow(h, "warp_request", warp_retail, warp_port, &x, WHAT(&a->base));
}
typedef void (__stdcall *gather_t)(const wl_body *);
static void gather_retail(void *c) { ((gather_t)(uintptr_t)H[H_GATHER].va)(c); }
static void gather_port(void *c) { port_world(); party_gather(c); }
static void __stdcall w_gather(const wl_body *b)
{
    hook_t *h = &H[H_GATHER];
    if (h->mode == 2) { h->calls++; gather_port((void *)b); return; }
    if (!sample(h)) { gather_retail((void *)b); return; }
    full_shadow(h, "party_gather", gather_retail, gather_port, (void *)b, "gather");
}

/* ---- the touch pass (src/gimmick.c) ---- */
typedef int (__attribute__((thiscall)) *tpass_t)(wl_roomloop *, wl_char *, int32_t);
typedef struct { wl_roomloop *rl; wl_char *pl; int32_t pressed; int rr, rp; } tpass_ctx;
static void tpass_retail(void *c) { tpass_ctx *x = c; x->rr = ((tpass_t)(uintptr_t)H[H_TPASS].va)(x->rl, x->pl, x->pressed); }
static void tpass_port(void *c) { tpass_ctx *x = c; port_world(); x->rp = touch_pass(x->rl, x->pl, x->pressed); }
static int __attribute__((thiscall)) w_tpass(wl_roomloop *rl, wl_char *pl, int32_t pressed)
{
    hook_t *h = &H[H_TPASS];
    tpass_ctx x = { rl, pl, pressed, 0, 0 };
    if (h->mode == 2) { h->calls++; tpass_port(&x); return x.rp; }
    if (!sample(h)) { tpass_retail(&x); return x.rr; }
    char what[64]; snprintf(what, sizeof what, "player %p pressed %d", (void *)pl, pressed);
    if (!full_shadow(h, "touch_pass", tpass_retail, tpass_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef int (__attribute__((thiscall)) *tact_t)(wl_actor *, int32_t *);
typedef struct { wl_actor *a; int32_t *acted; int32_t fr, fp; int rr, rp; } tact_ctx;
static void tact_retail(void *c) { tact_ctx *x = c; x->rr = ((tact_t)(uintptr_t)H[H_TACTOR].va)(x->a, x->acted); x->fr = *x->acted; }
static void tact_port(void *c) { tact_ctx *x = c; port_world(); x->rp = touch_actor(x->a, x->acted); x->fp = *x->acted; }
static int __attribute__((thiscall)) w_tact(wl_actor *a, int32_t *acted)
{
    hook_t *h = &H[H_TACTOR];
    tact_ctx x = { a, acted, 0, 0, 0, 0 };
    if (h->mode == 2) { h->calls++; tact_port(&x); return x.rp; }
    if (!sample(h)) { tact_retail(&x); return x.rr; }
    char what[64];
    if (!full_shadow(h, "touch_actor", tact_retail, tact_port, &x, WHAT(&a->base)) && (x.rr != x.rp || x.fr != x.fp))
        diverged(h, "f%u %s ret %d/%d acted %d/%d", g_frame, what, x.rr, x.rp, x.fr, x.fp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *rflush_t)(wl_room *);
static void rflush_retail(void *c) { ((rflush_t)(uintptr_t)H[H_RFLUSH].va)(c); }
static void rflush_port(void *c) { port_world(); room_input_flush(c); }
static void __attribute__((thiscall)) w_rflush(wl_room *r)
{
    hook_t *h = &H[H_RFLUSH];
    if (h->mode == 2) { h->calls++; rflush_port(r); return; }
    if (!sample(h)) { rflush_retail(r); return; }
    full_shadow(h, "room_input_flush", rflush_retail, rflush_port, r, "room");
}
/* pure: compare the 4 output words */
typedef void (__stdcall *prect_t)(const wl_body *, const int32_t *, int32_t *, int32_t);
static void __stdcall w_prect(const wl_body *b, const int32_t *p, int32_t *out, int32_t facing)
{
    hook_t *h = &H[H_PROBE_RECT];
    if (h->mode == 2) { h->calls++; body_probe_rect(b, p, out, facing); return; }
    ((prect_t)(uintptr_t)h->va)(b, p, out, facing);
    if (!sample(h)) return;
    int32_t o[4];
    body_probe_rect(b, p, o, facing);
    h->compared++;
    if (memcmp(o, out, sizeof o)) diverged(h, "f%u body %p: retail %d,%d port %d,%d", g_frame, (void *)b, out[0], out[1], o[0], o[1]);
}
typedef int (__attribute__((thiscall)) *near_t)(const wl_entity *, int32_t, int32_t, const wl_entity *, int32_t);
static int __attribute__((thiscall)) w_near(const wl_entity *e, int32_t rx, int32_t ry, const wl_entity *o, int32_t dy)
{
    hook_t *h = &H[H_NEAR];
    if (h->mode == 2) { h->calls++; return entity_near(e, rx, ry, o, dy); }
    int r = ((near_t)(uintptr_t)h->va)(e, rx, ry, o, dy);
    if (!sample(h)) return r;
    int p = entity_near(e, rx, ry, o, dy);
    h->compared++;
    if (p != r) diverged(h, "f%u %p vs %p (%d x %d): retail %d port %d", g_frame, (void *)e, (void *)o, rx, ry, r, p);
    return r;
}

/* ---- the event zones (src/gimmick.c): a tick with a zone under the leader is shadowed every call ---- */
typedef int (__attribute__((thiscall)) *zpass_t)(wl_zonestate *, uint32_t, wl_room *, int32_t *, int32_t *, int32_t *, int32_t);
typedef struct { wl_zonestate *zs; uint32_t now; wl_room *room; int32_t *sc, *arg, *zone; int32_t lock; int rr, rp; } zpass_ctx;
static void zpass_retail(void *c) { zpass_ctx *x = c; x->rr = ((zpass_t)(uintptr_t)H[H_ZPASS].va)(x->zs, x->now, x->room, x->sc, x->arg, x->zone, x->lock); }
static void zpass_port(void *c) { zpass_ctx *x = c; port_world(); x->rp = zone_pass(x->zs, x->now, x->room, x->sc, x->arg, x->zone, x->lock); }
static long g_zone_live;
static int zone_live(const wl_room *room)
{
    const wl_pslot *ld = room->leader;
    if (ld == NULL || ld->ch == NULL) return 0;
    port_world();
    for (uint16_t i = 0; i < 0x80; i++) {
        const wl_actor *a = (const wl_actor *)g_world->actors[i];
        if (a->base.alive != 0 && zone_hit(a, ld->ch)) return 1;
    }
    return 0;
}
static int __attribute__((thiscall)) w_zpass(wl_zonestate *zs, uint32_t now, wl_room *room, int32_t *sc, int32_t *arg, int32_t *zone, int32_t lock)
{
    hook_t *h = &H[H_ZPASS];
    zpass_ctx x = { zs, now, room, sc, arg, zone, lock, 0, 0 };
    if (h->mode == 2) { h->calls++; zpass_port(&x); return x.rp; }
    int live = !rh_in_shadow() && zone_live(room);
    if (!sample_rare(h, live)) { zpass_retail(&x); return x.rr; }
    if (live) g_zone_live++;
    char what[64]; snprintf(what, sizeof what, "zone %p lock %d", (void *)zs->under, lock);
    if (!full_shadow(h, "zone_pass", zpass_retail, zpass_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef int (__attribute__((thiscall)) *zhit_t)(const wl_actor *, const wl_char *);
static int __attribute__((thiscall)) w_zhit(const wl_actor *z, const wl_char *pl)
{
    hook_t *h = &H[H_ZHIT];
    if (h->mode == 2) { h->calls++; return zone_hit(z, pl); }
    int r = ((zhit_t)(uintptr_t)h->va)(z, pl);
    if (!sample(h)) return r;
    int p = zone_hit(z, pl);
    h->compared++;
    if (p != r) diverged(h, "f%u zone %p code %d: retail %d port %d", g_frame, (void *)z, z->base.code, r, p);
    return r;
}
typedef int (__stdcall *b96_t)(int32_t, int32_t);
static int __stdcall w_b96(int32_t x, int32_t y)
{
    hook_t *h = &H[H_B96_NEAR];
    if (h->mode == 2) { h->calls++; port_world(); return band96_near(x, y); }
    int r = ((b96_t)(uintptr_t)h->va)(x, y);
    if (!sample(h)) return r;
    port_world();
    int p = band96_near(x, y);
    h->compared++;
    if (p != r) diverged(h, "f%u (%d,%d): retail %d port %d", g_frame, x, y, r, p);
    return r;
}

/* ---- the room loop's per-tick calls (src/room_loop.c) — full state ---- */
typedef void (__attribute__((thiscall)) *rin_t)(wl_room *);
static void rin_retail(void *c) { ((rin_t)(uintptr_t)H[H_RIN].va)(c); }
static void rin_port(void *c) { port_world(); room_input_update(c); }
static void __attribute__((thiscall)) w_rin(wl_room *r)
{
    hook_t *h = &H[H_RIN];
    if (h->mode == 2) { h->calls++; rin_port(r); return; }
    if (!sample(h)) { rin_retail(r); return; }
    full_shadow(h, "room_input_update", rin_retail, rin_port, r, "room");
}
typedef int (__attribute__((fastcall)) *rchar_t)(wl_char *, void *, int32_t);
typedef struct { wl_char *c; void *w; int32_t cut; int rr, rp; } rchar_ctx;
static void rchar_retail(void *c_) { rchar_ctx *x = c_; x->rr = ((rchar_t)(uintptr_t)H[H_RCHAR].va)(x->c, x->w, x->cut); }
static void rchar_port(void *c_) { rchar_ctx *x = c_; port_world(); x->rp = room_char_step(x->c, x->cut); }
static int __attribute__((fastcall)) w_rchar(wl_char *c, void *w, int32_t cut)
{
    hook_t *h = &H[H_RCHAR];
    rchar_ctx x = { c, w, cut, 0, 0 };
    if (h->mode == 2) { h->calls++; rchar_port(&x); return x.rp; }
    if (!sample(h)) { rchar_retail(&x); return x.rr; }
    char what[96]; snprintf(what, sizeof what, "char %p code %d mode %d cut %d", (void *)c, c->base.code, c->f200, cut);
    if (!full_shadow(h, "room_char_step", rchar_retail, rchar_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *ract_t)(wl_actor *);
static void ract_retail(void *c) { ((ract_t)(uintptr_t)H[H_RACT].va)(c); }
static void ract_port(void *c) { port_world(); room_actor_step(c); }
static void __attribute__((thiscall)) w_ract(wl_actor *a)
{
    hook_t *h = &H[H_RACT];
    if (h->mode == 2) { h->calls++; ract_port(a); return; }
    if (!sample(h)) { ract_retail(a); return; }
    char what[64];
    full_shadow(h, "room_actor_step", ract_retail, ract_port, a, WHAT(&a->base));
}
typedef void (__attribute__((thiscall)) *ranim_t)(uint8_t *);
static void ranim_retail(void *c) { ((ranim_t)(uintptr_t)H[H_RANIM].va)(c); }
static void ranim_port(void *c) { port_world(); room_grid_anim(c); }
static void __attribute__((thiscall)) w_ranim(uint8_t *g)
{
    hook_t *h = &H[H_RANIM];
    if (h->mode == 2) { h->calls++; ranim_port(g); return; }
    if (!sample(h)) { ranim_retail(g); return; }
    full_shadow(h, "room_grid_anim", ranim_retail, ranim_port, g, "grid");
}
typedef int (__stdcall *rdown_t)(const wl_room *);
static int __stdcall w_rdown(const wl_room *r)
{
    hook_t *h = &H[H_RDOWN];
    if (h->mode == 2) { h->calls++; return room_party_down(r); }
    int ret = ((rdown_t)(uintptr_t)h->va)(r);
    if (!sample(h)) return ret;
    int p = room_party_down(r);
    h->compared++;
    if (p != ret) diverged(h, "f%u room %p: retail %d port %d", g_frame, (void *)r, ret, p);
    return ret;
}
typedef void (__attribute__((thiscall)) *rlead_t)(wl_roomloop *, const wl_char *, const wl_char *, int32_t);
typedef struct { wl_roomloop *rl; const wl_char *c, *p; int32_t mode; } rlead_ctx;
static void rlead_retail(void *c_) { rlead_ctx *x = c_; ((rlead_t)(uintptr_t)H[H_RLEAD].va)(x->rl, x->c, x->p, x->mode); }
static void rlead_port(void *c_) { rlead_ctx *x = c_; port_world(); room_lead(x->rl, x->c, x->p, x->mode); }
static void __attribute__((thiscall)) w_rlead(wl_roomloop *rl, const wl_char *c, const wl_char *p, int32_t mode)
{
    hook_t *h = &H[H_RLEAD];
    rlead_ctx x = { rl, c, p, mode };
    if (h->mode == 2) { h->calls++; rlead_port(&x); return; }
    if (!sample(h)) { rlead_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "mode %d lead %d", mode, rl->lead);
    full_shadow(h, "room_lead", rlead_retail, rlead_port, &x, what);
}
typedef void (__attribute__((thiscall)) *tsnd_t)(void *);
static void tsnd_retail(void *c) { ((tsnd_t)(uintptr_t)H[H_TSOUNDS].va)(c); }
static void tsnd_port(void *c) { timed_sounds_tick(c); }
static int tsnd_live(const uint8_t *sys)
{
    const uint8_t *l; memcpy(&l, sys + 0xaa8, sizeof l);
    if (l == NULL) return 0;
    const uint8_t *rec; memcpy(&rec, l + 8, sizeof rec);
    uint16_t n; memcpy(&n, l + 4, 2);
    for (uint16_t i = 0; rec != NULL && i < n; i++) { uint32_t b; memcpy(&b, rec + 24u * i, 4); if (b != 0) return 1; }
    return 0;
}
static void __attribute__((thiscall)) w_tsnd(void *sys)
{
    hook_t *h = &H[H_TSOUNDS];
    if (h->mode == 2) { h->calls++; tsnd_port(sys); return; }
    if (!sample_rare(h, !rh_in_shadow() && tsnd_live(sys))) { tsnd_retail(sys); return; }
    full_shadow(h, "timed_sounds_tick", tsnd_retail, tsnd_port, sys, "sys");
}

/* ---- the keyboard input producer (src/input_dik.c) — full state (ring records + held axes) ---- */
typedef void (__attribute__((thiscall)) *dik_t)(input_mgr *, const void *);
typedef struct { input_mgr *m; const void *kb; } dik_ctx;
static void dik_retail(void *c) { dik_ctx *x = c; ((dik_t)(uintptr_t)H[H_DIK].va)(x->m, x->kb); }
static void dik_port(void *c) { dik_ctx *x = c; port_world(); input_dik_produce(x->m, x->kb); }
static long g_dik_events;
static void __attribute__((thiscall)) w_dik(input_mgr *m, const void *kb)
{
    hook_t *h = &H[H_DIK];
    dik_ctx x = { m, kb };
    if (h->mode == 2) { h->calls++; dik_port(&x); return; }
    uint32_t n; memcpy(&n, (const uint8_t *)kb + 0x10, 4);
    if (!sample_rare(h, !rh_in_shadow() && n != 0)) { dik_retail(&x); return; }
    g_dik_events += n;
    char what[64]; snprintf(what, sizeof what, "events %u", n);
    full_shadow(h, "input_dik_produce", dik_retail, dik_port, &x, what);
}

/* ---- the encounter pass (src/room_loop.c) ---- */
typedef void (__attribute__((thiscall)) *enc_t)(wl_roomloop *, wl_room *, int32_t);
typedef struct { wl_roomloop *rl; wl_room *room; int32_t quiet; } enc_ctx;
static void enc_retail(void *c) { enc_ctx *x = c; ((enc_t)(uintptr_t)H[H_ENC].va)(x->rl, x->room, x->quiet); }
static void enc_port(void *c) { enc_ctx *x = c; port_world(); encounter_tick(x->rl, x->room, x->quiet); }
static void __attribute__((thiscall)) w_enc(wl_roomloop *rl, wl_room *room, int32_t quiet)
{
    hook_t *h = &H[H_ENC];
    enc_ctx x = { rl, room, quiet };
    if (h->mode == 2) { h->calls++; enc_port(&x); return; }
    if (!sample(h)) { enc_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "mode %d quiet %d", *(int32_t *)((uint8_t *)*(void **)(uintptr_t)WORLD_PTR_VA + 0x2770), quiet);
    full_shadow(h, "encounter_tick", enc_retail, enc_port, &x, what);
}
typedef int (__attribute__((thiscall)) *foes_t)(const wl_char *, int32_t, int32_t, int32_t, uint16_t *, int32_t, const wl_char *, int32_t);
static int __attribute__((thiscall)) w_foes(const wl_char *c, int32_t ranged, int32_t rx, int32_t ry, uint16_t *sc, int32_t ai, const wl_char *ref, int32_t lvl)
{
    hook_t *h = &H[H_FOES];
    if (h->mode == 2) { h->calls++; port_world(); return foes_near(c, ranged, rx, ry, sc, ai, ref, lvl); }
    uint16_t pre = sc ? *sc : 0;
    int r = ((foes_t)(uintptr_t)h->va)(c, ranged, rx, ry, sc, ai, ref, lvl);
    if (!sample(h)) return r;
    uint16_t rs = sc ? *sc : 0, ps = pre;
    port_world();
    int p = foes_near(c, ranged, rx, ry, sc ? &ps : NULL, ai, ref, lvl);
    h->compared++;
    if (p != r || ps != rs) diverged(h, "f%u char %p: retail %d/%u port %d/%u", g_frame, (void *)c, r, rs, p, ps);
    return r;
}
typedef int (__stdcall *pfoes_t)(const wl_room *, const wl_char *, uint16_t *, int32_t, int32_t, int32_t, uint32_t, int32_t);
static int __stdcall w_pfoes(const wl_room *room, const wl_char *ref, uint16_t *best, int32_t rx, int32_t ry, int32_t ai, uint32_t min, int32_t lvl)
{
    hook_t *h = &H[H_PFOES];
    if (h->mode == 2) { h->calls++; port_world(); return party_foes_near(room, ref, best, rx, ry, ai, (uint16_t)min, lvl); }
    uint16_t pre = best ? *best : 0;
    int r = ((pfoes_t)(uintptr_t)h->va)(room, ref, best, rx, ry, ai, min, lvl);
    if (!sample(h)) return r;
    uint16_t rb = best ? *best : 0, pb = pre;
    port_world();
    int p = party_foes_near(room, ref, best ? &pb : NULL, rx, ry, ai, (uint16_t)min, lvl);
    h->compared++;
    if (p != r || pb != rb) diverged(h, "f%u: retail %d/%u port %d/%u", g_frame, r, rb, p, pb);
    return r;
}
typedef int (__attribute__((thiscall)) *pslot_t)(const wl_pslot *, int32_t, int32_t);
static int __attribute__((thiscall)) w_pslot(const wl_pslot *s, int32_t any, int32_t inactive)
{
    hook_t *h = &H[H_PSLOT];
    if (h->mode == 2) { h->calls++; return pslot_ok(s, any, inactive); }
    int r = ((pslot_t)(uintptr_t)h->va)(s, any, inactive);
    if (!sample(h)) return r;
    int p = pslot_ok(s, any, inactive);
    h->compared++;
    if (p != r) diverged(h, "f%u slot %p: retail %d port %d", g_frame, (void *)s, r, p);
    return r;
}
typedef uint32_t (__attribute__((thiscall)) *bgm_t)(const wl_roomloop *);
static uint32_t __attribute__((thiscall)) w_bgm(const wl_roomloop *rl)
{
    hook_t *h = &H[H_BGM];
    if (h->mode == 2) { h->calls++; port_world(); return bgm_pick(rl); }
    uint32_t r = ((bgm_t)(uintptr_t)h->va)(rl);
    if (!sample(h)) return r;
    port_world();
    uint16_t p = bgm_pick(rl);
    h->compared++;
    if (p != (uint16_t)r) diverged(h, "f%u: retail %u port %u", g_frame, r & 0xffffu, p);
    return r;
}

/* ---- the room loop (src/room_loop.c): a whole room per call — REPLACE only (the corpus hash) ---- */
typedef int (__attribute__((thiscall)) *rloop_t)(wl_roomloop *);
static int __attribute__((thiscall)) w_rloop(wl_roomloop *rl)
{
    hook_t *h = &H[H_RLOOP];
    h->calls++;
    if (h->mode == 2) { port_world(); return room_loop_run(rl); }
    return ((rloop_t)(uintptr_t)h->va)(rl);
}
typedef void (__attribute__((thiscall)) *rflush2_t)(wl_roomloop *);
static void __attribute__((thiscall)) w_repoll(wl_roomloop *rl)
{
    hook_t *h = &H[H_REPOLL];
    h->calls++;
    if (h->mode == 2) { port_world(); input_repoll_flush(rl); return; }
    ((rflush2_t)(uintptr_t)h->va)(rl);
}

/* ---- the room setup (src/room_load.c): a room per call — REPLACE only (the corpus hash) ---- */
typedef int (__attribute__((thiscall)) *lroom_t)(wl_roomloop *, wl_room *, uint32_t, uint8_t *, uint8_t *, int32_t, int32_t,
                                                const uint8_t *, const uint8_t *, int32_t, int32_t, int32_t, int32_t, uint8_t *,
                                                int32_t, int32_t);
static int __attribute__((thiscall)) w_lroom(wl_roomloop *rl, wl_room *rooms, uint32_t n, uint8_t *areas, uint8_t *ri, int32_t gate,
                                             int32_t a7, const uint8_t *keep, const uint8_t *fill, int32_t fd4, int32_t wide, int32_t f2c,
                                             int32_t f27b4, uint8_t *wait, int32_t f2780, int32_t f105c)
{
    hook_t *h = &H[H_LROOM];
    h->calls++;
    if (h->mode == 2) {
        port_world();
        return load_room(rl, rooms, (uint16_t)n, areas, ri, gate, a7, keep, fill, fd4, wide, f2c, f27b4, wait, f2780, f105c);
    }
    return ((lroom_t)(uintptr_t)h->va)(rl, rooms, n, areas, ri, gate, a7, keep, fill, fd4, wide, f2c, f27b4, wait, f2780, f105c);
}
/* its helpers — full state (heap: the allocator view is part of the shadow) or pure */
typedef void (__attribute__((thiscall)) *bodies_t)(wl_entity *, uint32_t);
typedef struct { wl_entity *e; uint32_t n; } bodies_ctx;
static void bodies_retail(void *c) { bodies_ctx *x = c; ((bodies_t)(uintptr_t)H[H_BODIES].va)(x->e, x->n); }
static void bodies_port(void *c) { bodies_ctx *x = c; port_world(); entity_bodies_alloc(x->e, (uint16_t)x->n); }
static void __attribute__((thiscall)) w_bodies(wl_entity *e, uint32_t n)
{
    hook_t *h = &H[H_BODIES];
    bodies_ctx x = { e, n };
    if (h->mode == 2) { h->calls++; bodies_port(&x); return; }
    if (!sample(h)) { bodies_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "ent %p n %u", (void *)e, n & 0xffffu);
    full_shadow(h, "entity_bodies_alloc", bodies_retail, bodies_port, &x, what);
}
typedef void (__attribute__((thiscall)) *links_t)(wl_char *);
static void links_retail(void *c) { ((links_t)(uintptr_t)H[H_LINKS].va)(c); }
static void links_port(void *c) { port_world(); char_links_clear(c); }
static void __attribute__((thiscall)) w_links(wl_char *c)
{
    hook_t *h = &H[H_LINKS];
    if (h->mode == 2) { h->calls++; links_port(c); return; }
    if (!sample(h)) { links_retail(c); return; }
    full_shadow(h, "char_links_clear", links_retail, links_port, c, "char");
}
typedef void (__attribute__((thiscall)) *vpools_t)(void *);
static void vpools_retail(void *c) { ((vpools_t)(uintptr_t)H[H_VPOOLS].va)(c); }
static void vpools_port(void *c) { port_world(); view_pools_free(c); }
static void __attribute__((thiscall)) w_vpools(void *v)
{
    hook_t *h = &H[H_VPOOLS];
    if (h->mode == 2) { h->calls++; vpools_port(v); return; }
    if (!sample(h)) { vpools_retail(v); return; }
    full_shadow(h, "view_pools_free", vpools_retail, vpools_port, v, "view");
}
typedef void (__stdcall *shclr_t)(void *);
static void shclr_retail(void *c) { ((shclr_t)(uintptr_t)H[H_SHAKECLR].va)(c); }
static void shclr_port(void *c) { shake_clear(c); }
static void __stdcall w_shclr(void *s)
{
    hook_t *h = &H[H_SHAKECLR];
    if (h->mode == 2) { h->calls++; shake_clear(s); return; }
    if (!sample(h)) { shclr_retail(s); return; }
    full_shadow(h, "shake_clear", shclr_retail, shclr_port, s, "shake");
}
typedef void (__attribute__((thiscall)) *vfade_t)(uint8_t *, int32_t);
typedef struct { uint8_t *f; int32_t hold; } vfade_ctx;
static void vfade_retail(void *c) { vfade_ctx *x = c; ((vfade_t)(uintptr_t)H[H_VFADE].va)(x->f, x->hold); }
static void vfade_port(void *c) { vfade_ctx *x = c; volume_fade(x->f, x->hold); }
static void __attribute__((thiscall)) w_vfade(uint8_t *f, int32_t hold)
{
    hook_t *h = &H[H_VFADE];
    vfade_ctx x = { f, hold };
    if (h->mode == 2) { h->calls++; vfade_port(&x); return; }
    if (!sample(h)) { vfade_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "state %d hold %d", *(int32_t *)(f + 0x20), hold);
    full_shadow(h, "volume_fade", vfade_retail, vfade_port, &x, what);
}
typedef int32_t (__attribute__((thiscall)) *mapid_t)(const wl_char *);
static int32_t __attribute__((thiscall)) w_mapid(const wl_char *c)
{
    hook_t *h = &H[H_MAPID];
    if (h->mode == 2) { h->calls++; return char_map_id(c); }
    int32_t r = ((mapid_t)(uintptr_t)h->va)(c);
    if (!sample(h)) return r;
    int32_t p = char_map_id(c);
    h->compared++;
    if (p != r) diverged(h, "f%u char %p: retail %d port %d", g_frame, (void *)c, r, p);
    return r;
}
typedef int (__attribute__((thiscall)) *mapnpc_t)(const wl_char *);
static int __attribute__((thiscall)) w_mapnpc(const wl_char *c)
{
    hook_t *h = &H[H_MAPNPC];
    if (h->mode == 2) { h->calls++; return char_map_npc(c); }
    int r = ((mapnpc_t)(uintptr_t)h->va)(c);
    if (!sample(h)) return r;
    int p = char_map_npc(c);
    h->compared++;
    if (p != r) diverged(h, "f%u char %p: retail %d port %d", g_frame, (void *)c, r, p);
    return r;
}
typedef void (__attribute__((thiscall)) *psclr_t)(wl_pslot *);
static void psclr_retail(void *c) { ((psclr_t)(uintptr_t)H[H_PSCLR].va)(c); }
static void psclr_port(void *c) { pslot_status_clear(c); }
static void __attribute__((thiscall)) w_psclr(wl_pslot *s)
{
    hook_t *h = &H[H_PSCLR];
    if (h->mode == 2) { h->calls++; psclr_port(s); return; }
    if (!sample(h)) { psclr_retail(s); return; }
    full_shadow(h, "pslot_status_clear", psclr_retail, psclr_port, s, "slot");
}
typedef wl_actor *(__stdcall *gate_t)(int32_t);
static wl_actor *__stdcall w_gate(int32_t id)
{
    hook_t *h = &H[H_GATE];
    if (h->mode == 2) { h->calls++; port_world(); return gate_actor(id); }
    wl_actor *r = ((gate_t)(uintptr_t)h->va)(id);
    if (!sample(h)) return r;
    port_world();
    wl_actor *p = gate_actor(id);
    h->compared++;
    if (p != r) diverged(h, "f%u id %d: retail %p port %p", g_frame, id, (void *)r, (void *)p);
    return r;
}
typedef int (__attribute__((thiscall)) *sready_t)(const void *);
static int __attribute__((thiscall)) w_sready(const void *slot)
{
    hook_t *h = &H[H_SREADY];
    if (h->mode == 2) { h->calls++; return sprite_slot_ready(slot); }
    int r = ((sready_t)(uintptr_t)h->va)(slot);
    if (!sample(h)) return r;
    int p = sprite_slot_ready(slot);
    h->compared++;
    if (p != r) diverged(h, "f%u slot %p: retail %d port %d", g_frame, slot, r, p);
    return r;
}
typedef void (__attribute__((thiscall)) *stint_t)(void *, int32_t, int32_t, int32_t, const uint8_t *);
typedef struct { void *s; int32_t r, g, b; const uint8_t *lut; } stint_ctx;
static void stint_retail(void *c) { stint_ctx *x = c; ((stint_t)(uintptr_t)H[H_STINT].va)(x->s, x->r, x->g, x->b, x->lut); }
static void stint_port(void *c) { stint_ctx *x = c; sprite_slot_tint(x->s, x->r, x->g, x->b, x->lut); }
static void __attribute__((thiscall)) w_stint(void *s, int32_t r, int32_t g, int32_t b, const uint8_t *lut)
{
    hook_t *h = &H[H_STINT];
    stint_ctx x = { s, r, g, b, lut };
    if (h->mode == 2) { h->calls++; stint_port(&x); return; }
    if (!sample(h)) { stint_retail(&x); return; }
    full_shadow(h, "sprite_slot_tint", stint_retail, stint_port, &x, "slot");
}
typedef void (__attribute__((thiscall)) *tsfade_t)(void *, int32_t);
typedef struct { void *l; int32_t rate; } tsfade_ctx;
static void tsfade_retail(void *c) { tsfade_ctx *x = c; ((tsfade_t)(uintptr_t)H[H_TSFADE].va)(x->l, x->rate); }
static void tsfade_port(void *c) { tsfade_ctx *x = c; timed_sounds_fade(x->l, x->rate); }
static void __attribute__((thiscall)) w_tsfade(void *l, int32_t rate)
{
    hook_t *h = &H[H_TSFADE];
    tsfade_ctx x = { l, rate };
    if (h->mode == 2) { h->calls++; tsfade_port(&x); return; }
    if (!sample(h)) { tsfade_retail(&x); return; }
    full_shadow(h, "timed_sounds_fade", tsfade_retail, tsfade_port, &x, "list");
}
typedef int (__attribute__((thiscall)) *fill_t)(const uint8_t *, int32_t, int32_t *);
static int __attribute__((thiscall)) w_fill(const uint8_t *t, int32_t id, int32_t *out)
{
    hook_t *h = &H[H_FILL];
    if (h->mode == 2) { h->calls++; return fill_lookup(t, id, out); }
    int32_t o0 = *out;
    int r = ((fill_t)(uintptr_t)h->va)(t, id, out);
    if (!sample(h)) return r;
    int32_t ro = *out, po = o0;
    int p = fill_lookup(t, id, &po);
    h->compared++;
    if (p != r || po != ro) diverged(h, "f%u id %d: retail %d/%d port %d/%d", g_frame, id, r, ro, p, po);
    return r;
}

/* ---- the world's construction / destruction (src/world_alloc.c) + the room setup's leaves — full state ---- */
#define FS1(TAG, NAME, CC, T, PORTCALL)                                                                  \
    typedef void (CC *TAG##_t)(T);                                                                       \
    static void TAG##_retail(void *c) { ((TAG##_t)(uintptr_t)H[H_##TAG].va)((T)c); }                    \
    static void TAG##_port(void *c) { port_world(); PORTCALL((T)c); }                                    \
    static void CC w_##TAG(T x)                                                                          \
    {                                                                                                    \
        hook_t *h = &H[H_##TAG];                                                                         \
        if (h->mode == 2) { h->calls++; TAG##_port(x); return; }                                         \
        if (!sample(h)) { TAG##_retail(x); return; }                                                     \
        char what[48]; snprintf(what, sizeof what, "%p", (void *)x);                                     \
        full_shadow(h, NAME, TAG##_retail, TAG##_port, x, what);                                         \
    }
FS1(WFREE, "world_free", __attribute__((fastcall)), wl_world *, world_free)
FS1(EFREE, "entity_free", __attribute__((thiscall)), wl_entity *, entity_free)
FS1(AFREE, "actor_free", __attribute__((thiscall)), wl_actor *, actor_free)
FS1(CFREE, "char_free", __attribute__((thiscall)), wl_char *, char_free)
FS1(CBLK, "char_block_init", __attribute__((thiscall)), uint8_t *, char_block_init)
FS1(RSAVE, "room_state_save", __stdcall, uint8_t *, room_state_save)
FS1(MFREE, "map_res_free", __attribute__((thiscall)), uint8_t *, map_res_free)
#undef FS1
typedef wl_world *(__attribute__((thiscall)) *wnew_t)(void *);
typedef struct { void *m; void *rr, *rp; } wnew_ctx;
static void wnew_retail(void *c) { wnew_ctx *x = c; x->rr = ((wnew_t)(uintptr_t)H[H_WNEW].va)(x->m); }
static void wnew_port(void *c) { wnew_ctx *x = c; port_world(); x->rp = world_new(x->m); }
static wl_world *__attribute__((thiscall)) w_wnew(void *m)
{
    hook_t *h = &H[H_WNEW];
    wnew_ctx x = { m, 0, 0 };
    if (h->mode == 2) { h->calls++; wnew_port(&x); return x.rp; }
    if (!sample(h)) { wnew_retail(&x); return x.rr; }
    if (!full_shadow(h, "world_new", wnew_retail, wnew_port, &x, "world") && x.rr != x.rp)
        diverged(h, "f%u ret %p/%p", g_frame, x.rr, x.rp);
    return x.rr;
}
typedef wl_char *(__attribute__((thiscall)) *wlead_t)(const wl_world *);
static wl_char *__attribute__((thiscall)) w_wlead(const wl_world *w)
{
    hook_t *h = &H[H_WLEAD];
    if (h->mode == 2) { h->calls++; return world_leader(w); }
    wl_char *r = ((wlead_t)(uintptr_t)h->va)(w);
    if (!sample(h)) return r;
    wl_char *p = world_leader(w);
    h->compared++;
    if (p != r) diverged(h, "f%u: retail %p port %p", g_frame, (void *)r, (void *)p);
    return r;
}
typedef void *(__attribute__((thiscall)) *csnap_t)(wl_roomloop *, int32_t);
typedef struct { wl_roomloop *rl; int32_t snap; void *rr, *rp; } csnap_ctx;
static void csnap_retail(void *c) { csnap_ctx *x = c; x->rr = ((csnap_t)(uintptr_t)H[H_CSNAP].va)(x->rl, x->snap); }
static void csnap_port(void *c) { csnap_ctx *x = c; port_world(); x->rp = camera_snap(x->rl, x->snap); }
static void *__attribute__((thiscall)) w_csnap(wl_roomloop *rl, int32_t snap)
{
    hook_t *h = &H[H_CSNAP];
    csnap_ctx x = { rl, snap, 0, 0 };
    if (h->mode == 2) { h->calls++; csnap_port(&x); return x.rp; }
    if (!sample(h)) { csnap_retail(&x); return x.rr; }
    char what[32]; snprintf(what, sizeof what, "snap %d", snap);
    if (!full_shadow(h, "camera_snap", csnap_retail, csnap_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %p/%p", g_frame, what, x.rr, x.rp);
    return x.rr;
}
typedef void (__attribute__((thiscall)) *zreset_t)(wl_zonestate *, const wl_char *);
typedef struct { wl_zonestate *z; const wl_char *ld; } zreset_ctx;
static void zreset_retail(void *c) { zreset_ctx *x = c; ((zreset_t)(uintptr_t)H[H_ZRESET].va)(x->z, x->ld); }
static void zreset_port(void *c) { zreset_ctx *x = c; port_world(); zones_reset(x->z, x->ld); }
static void __attribute__((thiscall)) w_zreset(wl_zonestate *z, const wl_char *ld)
{
    hook_t *h = &H[H_ZRESET];
    zreset_ctx x = { z, ld };
    if (h->mode == 2) { h->calls++; zreset_port(&x); return; }
    if (!sample(h)) { zreset_retail(&x); return; }
    full_shadow(h, "zones_reset", zreset_retail, zreset_port, &x, "zones");
}
typedef int (__attribute__((thiscall)) *qstate_t)(const wl_room *, const uint8_t *, uint16_t *);
static int __attribute__((thiscall)) w_qstate(const wl_room *r, const uint8_t *rec, uint16_t *out)
{
    hook_t *h = &H[H_QSTATE];
    if (h->mode == 2) { h->calls++; return quest_state(r, rec, out); }
    uint16_t o0 = out ? *out : 0;
    int ret = ((qstate_t)(uintptr_t)h->va)(r, rec, out);
    if (!sample(h)) return ret;
    uint16_t ro = out ? *out : 0, po = o0;
    int p = quest_state(r, rec, out ? &po : NULL);
    h->compared++;
    if (p != ret || po != ro) diverged(h, "f%u rec %p: retail %d/%u port %d/%u", g_frame, (void *)rec, ret, ro, p, po);
    return ret;
}
typedef void (__attribute__((thiscall)) *vrec_t)(uint8_t *, wl_room *, int32_t);
typedef struct { uint8_t *r; wl_room *room; int32_t v; } vrec_ctx;
static void vrec_retail(void *c) { vrec_ctx *x = c; ((vrec_t)(uintptr_t)H[H_VREC].va)(x->r, x->room, x->v); }
static void vrec_port(void *c) { vrec_ctx *x = c; port_world(); view_record_init(x->r, x->room, x->v); }
static void __attribute__((thiscall)) w_vrec(uint8_t *r, wl_room *room, int32_t v)
{
    hook_t *h = &H[H_VREC];
    vrec_ctx x = { r, room, v };
    if (h->mode == 2) { h->calls++; vrec_port(&x); return; }
    if (!sample(h)) { vrec_retail(&x); return; }
    full_shadow(h, "view_record_init", vrec_retail, vrec_port, &x, "view record");
}
typedef void *(__attribute__((thiscall)) *del1_t)(uint8_t *, int32_t);
typedef struct { uint8_t *m; int32_t f; void *rr, *rp; } del1_ctx;
static void mdel_retail(void *c) { del1_ctx *x = c; x->rr = ((del1_t)(uintptr_t)H[H_MDEL].va)(x->m, x->f); }
static void mdel_port(void *c) { del1_ctx *x = c; x->rp = map_res_delete(x->m, x->f); }
static void *__attribute__((thiscall)) w_mdel(uint8_t *m, int32_t f)
{
    hook_t *h = &H[H_MDEL];
    del1_ctx x = { m, f, 0, 0 };
    if (h->mode == 2) { h->calls++; mdel_port(&x); return x.rp; }
    if (!sample(h)) { mdel_retail(&x); return x.rr; }
    if (!full_shadow(h, "map_res_delete", mdel_retail, mdel_port, &x, "map res") && x.rr != x.rp)
        diverged(h, "f%u ret %p/%p", g_frame, x.rr, x.rp);
    return x.rr;
}
static void phdel_retail(void *c) { del1_ctx *x = c; x->rr = ((del1_t)(uintptr_t)H[H_PHDEL].va)(x->m, x->f); }
static void phdel_port(void *c) { del1_ctx *x = c; x->rp = pool_head_delete(x->m, x->f); }
static void *__attribute__((thiscall)) w_phdel(uint8_t *m, int32_t f)
{
    hook_t *h = &H[H_PHDEL];
    del1_ctx x = { m, f, 0, 0 };
    if (h->mode == 2) { h->calls++; phdel_port(&x); return x.rp; }
    if (!sample(h)) { phdel_retail(&x); return x.rr; }
    if (!full_shadow(h, "pool_head_delete", phdel_retail, phdel_port, &x, "pool head") && x.rr != x.rp)
        diverged(h, "f%u ret %p/%p", g_frame, x.rr, x.rp);
    return x.rr;
}

/* ---- the formation, the actor state restore, the palette tint, the map resource load — full state ---- */
typedef void (__attribute__((thiscall)) *form_t)(wl_room *);
static void form_retail(void *c) { ((form_t)(uintptr_t)H[H_FORM].va)(c); }
static void form_port(void *c) { port_world(); room_formation(c); }
static void __attribute__((thiscall)) w_form(wl_room *r)
{
    hook_t *h = &H[H_FORM];
    if (h->mode == 2) { h->calls++; form_port(r); return; }
    if (!sample(h)) { form_retail(r); return; }
    full_shadow(h, "room_formation", form_retail, form_port, r, "room");
}
typedef int (__attribute__((thiscall)) *follow_t)(wl_char *, int32_t, wl_char *, int32_t, int32_t);
typedef struct { wl_char *c; int32_t mode; wl_char *ld; int32_t sd, fd; } follow_ctx;
static void follow_retail(void *c) { follow_ctx *x = c; ((follow_t)(uintptr_t)H[H_FOLLOW].va)(x->c, x->mode, x->ld, x->sd, x->fd); }
static void follow_port(void *c) { follow_ctx *x = c; char_follow_set(x->c, x->mode, x->ld, x->sd, x->fd); }
static int __attribute__((thiscall)) w_follow(wl_char *c, int32_t mode, wl_char *ld, int32_t sd, int32_t fd)
{
    hook_t *h = &H[H_FOLLOW];
    follow_ctx x = { c, mode, ld, sd, fd };
    if (h->mode == 2) { h->calls++; follow_port(&x); return 0; }
    if (!sample(h)) { follow_retail(&x); return 0; }
    full_shadow(h, "char_follow_set", follow_retail, follow_port, &x, "char");
    return 0;
}
typedef void (__attribute__((thiscall)) *astate_t)(wl_actor *, int32_t);
typedef struct { wl_actor *a; int32_t st; } astate_ctx;
static void astate_retail(void *c) { astate_ctx *x = c; ((astate_t)(uintptr_t)H[H_ASTATE].va)(x->a, x->st); }
static void astate_port(void *c) { astate_ctx *x = c; port_world(); actor_state_restore(x->a, x->st); }
static void __attribute__((thiscall)) w_astate(wl_actor *a, int32_t st)
{
    hook_t *h = &H[H_ASTATE];
    astate_ctx x = { a, st };
    if (h->mode == 2) { h->calls++; astate_port(&x); return; }
    if (!sample(h)) { astate_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "actor %p code %d st %d", (void *)a, a->base.code, st);
    full_shadow(h, "actor_state_restore", astate_retail, astate_port, &x, what);
}
typedef void (__stdcall *ptint_t)(uint8_t *, const uint8_t *, int32_t, int32_t, int32_t, int32_t);
typedef struct { uint8_t *d; const uint8_t *s; int32_t r, g, b, gray; } ptint_ctx;
static void ptint_retail(void *c) { ptint_ctx *x = c; ((ptint_t)(uintptr_t)H[H_PTINT].va)(x->d, x->s, x->r, x->g, x->b, x->gray); }
static void ptint_port(void *c) { ptint_ctx *x = c; palette_tint(x->d, x->s, x->r, x->g, x->b, x->gray); }
static void __stdcall w_ptint(uint8_t *d, const uint8_t *src, int32_t r, int32_t g, int32_t b, int32_t gray)
{
    hook_t *h = &H[H_PTINT];
    ptint_ctx x = { d, src, r, g, b, gray };
    if (h->mode == 2) { h->calls++; ptint_port(&x); return; }
    if (!sample(h)) { ptint_retail(&x); return; }
    /* pure in the 3 dst bytes: compare them, not the whole state (it runs 256 times a palette) */
    uint8_t pre[3], ret[3];
    memcpy(pre, d, 3);
    ptint_retail(&x);
    memcpy(ret, d, 3);
    memcpy(d, pre, 3);
    ptint_port(&x);
    h->compared++;
    if (memcmp(ret, d, 3)) diverged(h, "f%u %d/%d/%d gray %d: retail %02x%02x%02x port %02x%02x%02x", g_frame, r, g, b, gray,
                                    ret[0], ret[1], ret[2], d[0], d[1], d[2]);
    memcpy(d, ret, 3);
}
typedef int (__attribute__((thiscall)) *mload_t)(uint8_t *, void *, int32_t);
typedef struct { uint8_t *m; void *hmod; int32_t id; int rr, rp; } mload_ctx;
static void mload_retail(void *c) { mload_ctx *x = c; x->rr = ((mload_t)(uintptr_t)0x587970u)(x->m, x->hmod, x->id); }
static void mload_port(void *c) { mload_ctx *x = c; x->rp = map_res_load(x->m, x->hmod, x->id); }
static int __attribute__((thiscall)) w_mload(uint8_t *m, void *hmod, int32_t id)
{
    g_map_hmod = (uint32_t)(uintptr_t)hmod; g_map_id = (uint32_t)id & 0xffffu;   /* map_decode's shadow re-reads it */
    hook_t *h = &H[H_MLOAD];
    mload_ctx x = { m, hmod, id, 0, 0 };
    if (h->mode == 2) { h->calls++; mload_port(&x); return x.rp; }
    if (h->mode == 0 || !sample(h)) { mload_retail(&x); return x.rr; }
    char what[48]; snprintf(what, sizeof what, "map %u", (uint32_t)id & 0xffffu);
    if (!full_shadow(h, "map_res_load", mload_retail, mload_port, &x, what) && x.rr != x.rp)
        diverged(h, "f%u %s ret %d/%d", g_frame, what, x.rr, x.rp);
    return x.rr;
}

/* ---- the sprite slot helpers (src/sprite_slot.c) — full state (a freed surface aborts the port phase) ---- */
typedef void (__attribute__((thiscall)) *sfree_t)(ar_sprite_slot *, uint32_t);
typedef struct { ar_sprite_slot *s; uint32_t k; } sfree_ctx;
static void sfree_retail(void *c) { sfree_ctx *x = c; ((sfree_t)(uintptr_t)H[H_SFREE].va)(x->s, x->k); }
static void sfree_port(void *c) { sfree_ctx *x = c; slot_frames_free(x->s, (uint16_t)x->k); }
static void __attribute__((thiscall)) w_sfree(ar_sprite_slot *sl, uint32_t k)
{
    hook_t *h = &H[H_SFREE];
    sfree_ctx x = { sl, k };
    if (h->mode == 2) { h->calls++; sfree_port(&x); return; }
    if (!sample(h)) { sfree_retail(&x); return; }
    full_shadow(h, "slot_frames_free", sfree_retail, sfree_port, &x, "slot");
}
typedef void (__attribute__((thiscall)) *sfreeall_t)(ar_sprite_slot *);
static void sfreeall_retail(void *c) { ((sfreeall_t)(uintptr_t)H[H_SFREEALL].va)(c); }
static void sfreeall_port(void *c) { slot_frames_free_all(c); }
static void __attribute__((thiscall)) w_sfreeall(ar_sprite_slot *sl)
{
    hook_t *h = &H[H_SFREEALL];
    if (h->mode == 2) { h->calls++; sfreeall_port(sl); return; }
    if (!sample(h)) { sfreeall_retail(sl); return; }
    full_shadow(h, "slot_frames_free_all", sfreeall_retail, sfreeall_port, sl, "slot");
}
typedef void (__attribute__((thiscall)) *sgraded_t)(void *);
static void sgraded_retail(void *c) { ((sgraded_t)(uintptr_t)H[H_SGRADED].va)(c); }
static void sgraded_port(void *c) { slots_free_graded(c); }
static void __attribute__((thiscall)) w_sgraded(void *sys)
{
    hook_t *h = &H[H_SGRADED];
    if (h->mode == 2) { h->calls++; sgraded_port(sys); return; }
    if (!sample(h)) { sgraded_retail(sys); return; }
    full_shadow(h, "slots_free_graded", sgraded_retail, sgraded_port, sys, "sys");
}
typedef void (__attribute__((thiscall)) *pswap_t)(void *, const uint8_t *, uint8_t *);
typedef struct { void *sys; const uint8_t *t; uint8_t *p; } pswap_ctx;
static void pswap_retail(void *c) { pswap_ctx *x = c; ((pswap_t)(uintptr_t)H[H_PSWAP].va)(x->sys, x->t, x->p); }
static void pswap_port(void *c) { pswap_ctx *x = c; pal_swap_apply(x->sys, x->t, x->p); }
static void __attribute__((thiscall)) w_pswap(void *sys, const uint8_t *t, uint8_t *p)
{
    hook_t *h = &H[H_PSWAP];
    pswap_ctx x = { sys, t, p };
    if (h->mode == 2) { h->calls++; pswap_port(&x); return; }
    if (!sample(h)) { pswap_retail(&x); return; }
    full_shadow(h, "pal_swap_apply", pswap_retail, pswap_port, &x, "palette");
}

/* ---- the room population (src/room_load.c) — full state ---- */
typedef void (__attribute__((thiscall)) *popul_t)(wl_roomloop *, int32_t, const uint8_t *);
typedef struct { wl_roomloop *rl; int32_t map; const uint8_t *m; } popul_ctx;
static void popul_retail(void *c) { popul_ctx *x = c; ((popul_t)(uintptr_t)H[H_POPULATE].va)(x->rl, x->map, x->m); }
static void popul_port(void *c) { popul_ctx *x = c; port_world(); room_populate(x->rl, x->map, x->m); }
static void __attribute__((thiscall)) w_popul(wl_roomloop *rl, int32_t map, const uint8_t *m)
{
    hook_t *h = &H[H_POPULATE];
    popul_ctx x = { rl, map, m };
    if (h->mode == 2) { h->calls++; popul_port(&x); return; }
    if (!sample(h)) { popul_retail(&x); return; }
    char what[48]; snprintf(what, sizeof what, "map %d", map);
    full_shadow(h, "room_populate", popul_retail, popul_port, &x, what);
}

typedef void (__attribute__((thiscall)) *sinit_t)(wl_entity *, int32_t, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t);
typedef struct { wl_entity *e; int32_t x, y, an; uint32_t code; int32_t b, tint, h; } sinit_ctx;
static void sinit_retail(void *c) { sinit_ctx *x = c; ((sinit_t)(uintptr_t)H[H_SINIT].va)(x->e, x->x, x->y, x->an, x->code, x->b, x->tint, x->h); }
static void sinit_port(void *c) { sinit_ctx *x = c; port_world(); structure_init(x->e, x->x, x->y, x->an, x->code, x->b, x->tint, x->h); }
static void __attribute__((thiscall)) w_sinit(wl_entity *e, int32_t x0, int32_t y0, int32_t an, uint32_t code, int32_t b, int32_t tint, int32_t hd)
{
    hook_t *h = &H[H_SINIT];
    sinit_ctx x = { e, x0, y0, an, code, b, tint, hd };
    if (h->mode == 2) { h->calls++; sinit_port(&x); return; }
    if (!sample(h)) { sinit_retail(&x); return; }
    char what[48]; snprintf(what, sizeof what, "code %u", code);
    full_shadow(h, "structure_init", sinit_retail, sinit_port, &x, what);
}
typedef void (__attribute__((thiscall)) *seth_t)(wl_entity *, int32_t);
typedef struct { wl_entity *e; int32_t hd; } seth_ctx;
static void seth_retail(void *c) { seth_ctx *x = c; ((seth_t)(uintptr_t)H[H_SETH].va)(x->e, x->hd); }
static void seth_port(void *c) { seth_ctx *x = c; port_world(); entity_set_handle(x->e, x->hd); }
static void __attribute__((thiscall)) w_seth(wl_entity *e, int32_t hd)
{
    hook_t *h = &H[H_SETH];
    seth_ctx x = { e, hd };
    if (h->mode == 2) { h->calls++; seth_port(&x); return; }
    if (!sample(h)) { seth_retail(&x); return; }
    full_shadow(h, "entity_set_handle", seth_retail, seth_port, &x, "entity");
}

/* ---- the device init (src/device_init.c) — full state, per-code coverage ---- */
typedef void (__attribute__((thiscall)) *dinit_t)(wl_actor *, int32_t, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t,
                                                int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                                                int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, wl_entity *);
typedef struct { wl_actor *a; int32_t v[24]; uint32_t code; wl_entity *f; } dinit_ctx;
static void dinit_retail(void *c)
{
    dinit_ctx *x = c; int32_t *v = x->v;
    ((dinit_t)(uintptr_t)H[H_DINIT].va)(x->a, v[0], v[1], v[2], x->code, v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11],
                                        v[12], v[13], v[14], v[15], v[16], v[17], v[18], v[19], v[20], v[21], v[22], x->f);
}
static void dinit_port(void *c)
{
    dinit_ctx *x = c; int32_t *v = x->v;
    port_world();
    device_init(x->a, v[0], v[1], v[2], x->code, v[3], (int16_t)v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13],
                v[14], v[15], v[16], v[17], v[18], v[19], v[20], v[21], v[22], x->f);
}
#define NDCODES 128
static int32_t g_dcode_id[NDCODES]; static long g_dcodes[NDCODES]; static int g_ndcodes;
static void __attribute__((thiscall)) w_dinit(wl_actor *a, int32_t x0, int32_t y0, int32_t an, uint32_t code, int32_t k6, int32_t trk,
                                              int32_t hd, int32_t zn, int32_t st, int32_t kd, int32_t fc, int32_t ax0, int32_t ax1,
                                              int32_t ay0, int32_t ay1, int32_t p0, int32_t p1, int32_t p2, int32_t p3, int32_t gx,
                                              int32_t gy, int32_t lk, int32_t rd, int32_t mp, wl_entity *f)
{
    hook_t *h = &H[H_DINIT];
    dinit_ctx x = { a, { x0, y0, an, k6, trk, hd, zn, st, kd, fc, ax0, ax1, ay0, ay1, p0, p1, p2, p3, gx, gy, lk, rd, mp }, code, f };
    if (h->mode == 2) { h->calls++; dinit_port(&x); return; }
    if (!sample(h)) { dinit_retail(&x); return; }
    char what[48]; snprintf(what, sizeof what, "code %u", code);
    if (!full_shadow(h, "device_init", dinit_retail, dinit_port, &x, what)) {
        int k; for (k = 0; k < g_ndcodes && g_dcode_id[k] != (int32_t)code; k++) {}
        if (k == g_ndcodes && g_ndcodes < NDCODES) g_dcode_id[g_ndcodes++] = (int32_t)code;
        if (k < NDCODES) g_dcodes[k]++;
    }
}
typedef void (__attribute__((thiscall)) *sndfree_t)(wl_entity *);
static void sndfree_retail(void *c) { ((sndfree_t)(uintptr_t)H[H_SNDFREE].va)(c); }
static void sndfree_port(void *c) { entity_snd_free(c); }
static void __attribute__((thiscall)) w_sndfree(wl_entity *e)
{
    hook_t *h = &H[H_SNDFREE];
    if (h->mode == 2) { h->calls++; sndfree_port(e); return; }
    if (!sample(h)) { sndfree_retail(e); return; }
    full_shadow(h, "entity_snd_free", sndfree_retail, sndfree_port, e, "entity");
}

/* ---- the character definition helpers (src/char_def.c) — full state, generic wrappers: a
 * thiscall / stdcall helper of N stack words; the adapter returns what retail returns where a
 * caller reads it (CMP = compared) ---- */
typedef struct { void *self; intptr_t a[22]; intptr_t rr, rp; } gx_ctx;
#define GP1 intptr_t a1
#define GP2 GP1, intptr_t a2
#define GP3 GP2, intptr_t a3
#define GP4 GP3, intptr_t a4
#define GP5 GP4, intptr_t a5
#define GP6 GP5, intptr_t a6
#define GP7 GP6, intptr_t a7
#define GP8 GP7, intptr_t a8
#define GP9 GP8, intptr_t a9
#define GP10 GP9, intptr_t a10
#define GP11 GP10, intptr_t a11
#define GP12 GP11, intptr_t a12
#define GP13 GP12, intptr_t a13
#define GP14 GP13, intptr_t a14
#define GP15 GP14, intptr_t a15
#define GP16 GP15, intptr_t a16
#define GP17 GP16, intptr_t a17
#define GP18 GP17, intptr_t a18
#define GP19 GP18, intptr_t a19
#define GP20 GP19, intptr_t a20
#define GP21 GP20, intptr_t a21
#define GP22 GP21, intptr_t a22
#define GA1 a1
#define GA2 GA1, a2
#define GA3 GA2, a3
#define GA4 GA3, a4
#define GA5 GA4, a5
#define GA6 GA5, a6
#define GA7 GA6, a7
#define GA8 GA7, a8
#define GA9 GA8, a9
#define GA10 GA9, a10
#define GA11 GA10, a11
#define GA12 GA11, a12
#define GA13 GA12, a13
#define GA14 GA13, a14
#define GA15 GA14, a15
#define GA16 GA15, a16
#define GA17 GA16, a17
#define GA18 GA17, a18
#define GA19 GA18, a19
#define GA20 GA19, a20
#define GA21 GA20, a21
#define GA22 GA21, a22
#define GX1 x->a[0]
#define GX2 GX1, x->a[1]
#define GX3 GX2, x->a[2]
#define GX4 GX3, x->a[3]
#define GX5 GX4, x->a[4]
#define GX6 GX5, x->a[5]
#define GX7 GX6, x->a[6]
#define GX8 GX7, x->a[7]
#define GX9 GX8, x->a[8]
#define GX10 GX9, x->a[9]
#define GX11 GX10, x->a[10]
#define GX12 GX11, x->a[11]
#define GX13 GX12, x->a[12]
#define GX14 GX13, x->a[13]
#define GX15 GX14, x->a[14]
#define GX16 GX15, x->a[15]
#define GX17 GX16, x->a[16]
#define GX18 GX17, x->a[17]
#define GX19 GX18, x->a[18]
#define GX20 GX19, x->a[19]
#define GX21 GX20, x->a[20]
#define GX22 GX21, x->a[21]
#define GX_TAIL(ID, NAME, CMP)                                                                       \
    if (h->mode == 2) { h->calls++; ID##_p(&x); return x.rp; }                                        \
    if (!sample(h)) { ID##_r(&x); return x.rr; }                                                      \
    if (!full_shadow(h, NAME, ID##_r, ID##_p, &x, NAME) && (CMP) && x.rr != x.rp)                   \
        diverged(h, "f%u %s ret %p/%p", g_frame, NAME, (void *)x.rr, (void *)x.rp);                \
    return x.rr;
#define TC_HOOK(ID, NAME, N, CMP, PORT)                                                              \
    typedef intptr_t (__attribute__((thiscall)) *ID##_t)(void *, GP##N);                             \
    static intptr_t ID##_port(void *self, GP##N) { PORT; }                                          \
    static void ID##_r(void *c) { gx_ctx *x = c; x->rr = ((ID##_t)(uintptr_t)H[ID].va)(x->self, GX##N); } \
    static void ID##_p(void *c) { gx_ctx *x = c; port_world(); x->rp = ID##_port(x->self, GX##N); } \
    static intptr_t __attribute__((thiscall)) ID##_w(void *self, GP##N)                              \
    { hook_t *h = &H[ID]; gx_ctx x = { self, { GA##N }, 0, 0 }; GX_TAIL(ID, NAME, CMP) }
#define TC_HOOK0(ID, NAME, CMP, PORT)                                                                \
    typedef intptr_t (__attribute__((thiscall)) *ID##_t)(void *);                                    \
    static intptr_t ID##_port(void *self) { PORT; }                                                 \
    static void ID##_r(void *c) { gx_ctx *x = c; x->rr = ((ID##_t)(uintptr_t)H[ID].va)(x->self); }    \
    static void ID##_p(void *c) { gx_ctx *x = c; port_world(); x->rp = ID##_port(x->self); }         \
    static intptr_t __attribute__((thiscall)) ID##_w(void *self)                                     \
    { hook_t *h = &H[ID]; gx_ctx x = { self, { 0 }, 0, 0 }; GX_TAIL(ID, NAME, CMP) }
#define ST_HOOK(ID, NAME, N, CMP, PORT)                                                              \
    typedef intptr_t (__attribute__((stdcall)) *ID##_t)(GP##N);                                      \
    static intptr_t ID##_port(GP##N) { PORT; }                                                      \
    static void ID##_r(void *c) { gx_ctx *x = c; x->rr = ((ID##_t)(uintptr_t)H[ID].va)(GX##N); }      \
    static void ID##_p(void *c) { gx_ctx *x = c; port_world(); x->rp = ID##_port(GX##N); }           \
    static intptr_t __attribute__((stdcall)) ID##_w(GP##N)                                           \
    { hook_t *h = &H[ID]; gx_ctx x = { 0, { GA##N }, 0, 0 }; GX_TAIL(ID, NAME, CMP) }
#define CP(v) ((const void *)(v))
#define U16(v) ((uint16_t)(v))
TC_HOOK(H_CD_SKFIND, "char_skill_find", 1, 1, return (intptr_t)char_skill_find(self, U16(a1)))
TC_HOOK0(H_CD_GPROBE, "char_ground_probe", 1, return char_ground_probe(self))
TC_HOOK0(H_CD_VSFREE, "voice_set_free", 0, voice_set_free(self); return 0)
TC_HOOK0(H_CD_VSSTOP, "voice_set_stop", 0, voice_set_stop(self); return 0)
TC_HOOK0(H_CD_TODOFF, "tod_off", 1, return tod_off(self))
TC_HOOK0(H_CD_GF1C, "grid_f1c", 1, return grid_f1c(self))
ST_HOOK(H_CD_JITTER, "stat_jitter", 3, 1, return stat_jitter(a1, a2, a3))
ST_HOOK(H_CD_STATS, "char_stats_init", 3, 0, char_stats_init((uint8_t *)a1, a2, a3); return 0)
TC_HOOK(H_CD_IDLEI, "char_idle_init", 3, 1, char_idle_init(self, a1, a2, a3); return ((wl_char *)self)->idle_t)
TC_HOOK(H_CD_IDLEA, "char_idle_add", 1, 0, char_idle_add(self, a1); return 0)
TC_HOOK(H_CD_MSKILL, "char_member_skill_add", 2, 0, char_member_skill_add(self, a1, a2); return 0)
TC_HOOK(H_CD_COMBO, "char_combo_add", 14, 1,
        return (intptr_t)char_combo_add(self, a1, a2, a3, a4, a5, a6, a7, a8, a9, U16(a10), U16(a11), U16(a12), U16(a13), U16(a14)))
TC_HOOK(H_CD_CLIPS, "char_clips_set", 5, 0, char_clips_set(self, CP(a1), CP(a2), CP(a3), a4, a5); return 0)
TC_HOOK(H_CD_CLIPS2, "char_clips2_set", 2, 1, char_clips2_set(self, CP(a1), CP(a2)); return a1)
TC_HOOK(H_CD_CARRY, "char_carry_set", 6, 1, char_carry_set(self, a1, CP(a2), CP(a3), a4, a5, a6); return a5)
TC_HOOK(H_CD_STEP, "char_step_set", 2, 1, char_step_set(self, a1, U16(a2)); return a1)
TC_HOOK(H_CD_PACE, "char_pace_set", 4, 0, char_pace_set(self, a1, a2, a3, a4); return 0)
TC_HOOK(H_CD_PACER, "char_pace_raw", 9, 1,
        char_pace_raw(self, a1, a2, U16(a3), U16(a4), U16(a5), U16(a6), U16(a7), U16(a8), a9); return a9)
TC_HOOK(H_CD_C880, "char_c880_set", 3, 1, char_c880_set(self, a1, a2, a3); return a3)
TC_HOOK(H_CD_MOVE, "char_move_set", 8, 1, char_move_set(self, a1, a2, a3, a4, a5, a6, a7, a8); return a8)
TC_HOOK(H_CD_15A5C, "char_15a5c_set", 4, 1, char_15a5c_set(self, a1, a2, a3, a4); return a3)
TC_HOOK(H_CD_TURN, "char_turn_set", 2, 1, char_turn_set(self, a1, a2); return a1)
TC_HOOK(H_CD_WALK, "char_walk_set", 6, 1, char_walk_set(self, a1, a2, a3, a4, a5, CP(a6)); return a5)
TC_HOOK(H_CD_JUMP, "char_jump_set", 10, 1,
        char_jump_set(self, a1, a2, a3, a4, a5, a6, CP(a7), CP(a8), CP(a9), U16(a10)); return 0)
TC_HOOK(H_CD_RUN, "char_run_set", 2, 1, char_run_set(self, a1, CP(a2)); return a1)
TC_HOOK(H_CD_HIT, "char_hit_set", 5, 1, char_hit_set(self, U16(a1), CP(a2), a3, a4, a5); return 0)
TC_HOOK(H_CD_MPOSE, "char_member_pose_set", 3, 1, char_member_pose_set(self, U16(a1), U16(a2), a3); return a3)
TC_HOOK(H_CD_CROUCH, "char_crouch_set", 4, 1, char_crouch_set(self, a1, CP(a2), CP(a3), CP(a4)); return a3)
TC_HOOK(H_CD_NAVSK, "char_nav_skills_set", 7, 1, char_nav_skills_set(self, a1, a2, a3, a4, a5, a6, a7); return a7)
TC_HOOK(H_CD_APPR, "char_approach_add", 12, 0,
        char_approach_add(self, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, U16(a12)); return 0)
TC_HOOK(H_CD_SKCODE, "char_skills_by_code", 1, 0, char_skills_by_code(self, a1); return 0)
TC_HOOK(H_CD_LEARN, "char_skill_learn", 1, 1, return (intptr_t)char_skill_learn(self, a1))
TC_HOOK(H_CD_SKDEF, "char_skill_def", 22, 1,
        return (intptr_t)char_skill_def(self, a1, U16(a2), a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, U16(a13), a14, a15,
                                        a16, a17, a18, U16(a19), U16(a20), a21, (const char *)a22))
TC_HOOK(H_CD_NAV, "char_nav_set", 3, 1, char_nav_set(self, a1, a2, a3); return a3)
TC_HOOK(H_CD_RETGT, "char_retarget_set", 2, 0, char_retarget_set(self, U16(a1), U16(a2)); return 0)
TC_HOOK(H_CD_AISK, "char_ai_skill_add", 16, 1,
        return (intptr_t)char_ai_skill_add(self, a1, a2, a3, a4, a5, a6, a7, U16(a8), U16(a9), U16(a10), U16(a11), a12, a13,
                                           a14, a15, a16))
ST_HOOK(H_CD_BOXES, "ai_skill_boxes_add", 9, 1, return (intptr_t)ai_skill_boxes_add((uint8_t *)a1, a2, a3, a4, a5, a6, a7, a8, a9))
TC_HOOK(H_CD_VOICE, "char_voice_init", 3, 1, char_voice_init(self, a1, a2, a3); return (intptr_t)self)
TC_HOOK(H_CD_NPCDEF, "char_npc_default", 2, 1, char_npc_default(self, a1, CP(a2)); return 12800)
TC_HOOK(H_CD_FORM0, "char_form0_set", 1, 0, char_form0_set(self, U16(a1)); return 0)
TC_HOOK(H_CD_PMODE, "char_party_mode_set", 1, 0, char_party_mode_set(self, a1); return 0)
TC_HOOK(H_CD_IVOICE, "char_idle_voice_set", 2, 0, char_idle_voice_set(self, a1, U16(a2)); return 0)
/* the party member create / attach (src/party_member.c) + the sprite slot clone (src/sprite_slot.c) */
TC_HOOK(H_PM_CREATE, "member_create", 5, 1, return (intptr_t)member_create(self, a1, CP(a2), (wl_pslot *)a3, a4, a5))
TC_HOOK(H_PM_ATTACH, "member_attach", 2, 1, return member_attach(self, (wl_pslot *)a1, a2))
TC_HOOK0(H_PM_CODE, "pslot_code", 1, return pslot_code(self))
TC_HOOK0(H_PM_PAL, "member_palette", 0, member_palette(self); return 0)
TC_HOOK(H_PM_SKREF, "char_skills_refresh", 1, 1, return char_skills_refresh(self, a1))
TC_HOOK(H_PM_USABLE, "char_skill_usable", 2, 1, return char_skill_usable(self, CP(a1), a2))
ST_HOOK(H_PM_SKAT, "member_skill_at", 3, 1, return (intptr_t)member_skill_at(CP(a1), a2, U16(a3)))
TC_HOOK0(H_SS_EFREE, "slot_entries_free", 0, slot_entries_free(self); return 0)
TC_HOOK(H_SS_CLONE, "slot_clone", 2, 1, return (intptr_t)slot_clone(self, U16(a1), U16(a2)))
TC_HOOK0(H_CTRL_FREE, "ctrl_geometry_free", 0, ctrl_geometry_free(self); return 0)
/* the scene loop's helpers (src/scene_loop.c) */
TC_HOOK0(H_SL_OBJ, "scene_obj_init", 1, return (intptr_t)scene_obj_init(self))
TC_HOOK(H_SL_FFIND, "flag_find", 1, 1, return (intptr_t)flag_find(self, a1))
TC_HOOK(H_SL_FADD, "flag_add", 1, 1, return (intptr_t)flag_add(self, a1))
TC_HOOK(H_SL_RFLAG, "room_flag_set", 2, 1, return (intptr_t)room_flag_set(self, a1, a2))
TC_HOOK0(H_SL_PSRESET, "pslot_reset", 1, pslot_reset(self); return 0)
TC_HOOK0(H_SL_PDEF, "room_party_defaults", 1, room_party_defaults(self); return 0)
TC_HOOK(H_SL_INALLOC, "room_inputs_alloc", 1, 0, room_inputs_alloc(self, U16(a1)); return 0)
TC_HOOK(H_SL_RDEL, "room_delete", 1, 0, room_delete(self, a1); return (intptr_t)self)
TC_HOOK(H_SL_AFIND, "area_find", 1, 1, return (intptr_t)area_find(self, a1))
TC_HOOK(H_SL_ININIT, "input_init", 4, 1, return (intptr_t)input_init(self, U16(a1), a2, a3, a4))
TC_HOOK(H_SL_AENTER, "room_area_enter", 2, 0, room_area_enter(self, a1, (wl_room *)a2); return 0)
TC_HOOK0(H_SL_FCLR, "fill_clear_mapped", 0, fill_clear_mapped(self); return 0)
TC_HOOK0(H_SL_HASRET, "room_has_retry", 1, return room_has_retry(self))
TC_HOOK0(H_SL_RETPT, "room_retry_point", 0, room_retry_point(self); return 0)
TC_HOOK(H_SL_KFIND, "keep_find", 1, 1, return (intptr_t)keep_find(self, a1))
TC_HOOK(H_SL_NEXT, "room_next_set", 3, 1, room_next_set(self, a1, a2, a3); return a3)
TC_HOOK(H_SL_HEAL, "room_party_heal", 1, 0, room_party_heal(self, a1); return 0)
TC_HOOK(H_SL_GOLD, "room_gold_add", 2, 1, return room_gold_add(self, a1, a2))
TC_HOOK0(H_SL_RLNEW, "roomloop_new", 1, return (intptr_t)roomloop_new(self))
TC_HOOK(H_SL_FROMSC, "room_from_scene", 1, 1, room_from_scene(self, CP(a1)); return a1)
TC_HOOK(H_SL_ODEL, "obj_delete", 1, 0, obj_delete(self, a1); return (intptr_t)self)
TC_HOOK0(H_SL_OFREE, "obj_free", 0, obj_free(self); return 0)
TC_HOOK(H_SL_PSIN, "pslot_input_set", 1, 1, return pslot_input_set(self, (void *)a1))
TC_HOOK(H_SL_KSNAP, "keep_snapshot", 1, 0, keep_snapshot(self, (wl_roomloop *)a1); return 0)
TC_HOOK0(H_SL_ENDCLN, "room_end_cleanup", 0, room_end_cleanup(self); return 0)
TC_HOOK0(H_SL_RLPOOLS, "roomloop_pools_free", 0, roomloop_pools_free(self); return 0)
TC_HOOK0(H_SL_RLFREE, "roomloop_free", 0, roomloop_free(self); return 0)
TC_HOOK0(H_SL_SEND, "session_end", 0, session_end(self); return 0)
TC_HOOK(H_SL_XREF, "area_xref", 2, 0, area_xref(self, CP(a1), (uint8_t *)a2); return 0)
TC_HOOK0(H_SL_FCACHE, "frame_cache_clear", 0, frame_cache_clear(self); return 0)
TC_HOOK(H_SL_FREAD, "file_read_section", 3, 0, return file_read_section(self, (void *)a1, a2, (uint32_t)a3))
TC_HOOK(H_SL_POBJDEL, "pool_obj_delete", 1, 0, pool_obj_delete(self, a1); return (intptr_t)self)
TC_HOOK(H_SL_WDEL, "world_delete", 1, 0, world_delete(self, a1); return (intptr_t)self)
TC_HOOK(H_SL_TOSC, "room_to_scene", 1, 0, room_to_scene(self, (uint8_t *)a1); return 0)
ST_HOOK(H_SL_SSTART, "scene_start", 4, 0, return scene_start((uint8_t *)a1, (wl_room *)a2, a3, a4))
/* the party API (src/party_member.c) */
TC_HOOK(H_PA_BYID, "pslot_by_id", 2, 1, return (intptr_t)pslot_by_id(self, a1, a2))
ST_HOOK(H_PA_DRAM, "dramatist_row", 1, 1, return (intptr_t)dramatist_row(a1))
ST_HOOK(H_PA_ALV, "area_level_row", 1, 1, return (intptr_t)area_level_row(a1))
TC_HOOK(H_PA_HEAL, "pslot_heal", 1, 1, pslot_heal(self, a1); return 0)
TC_HOOK(H_PA_LEVEL, "room_party_level", 1, 0, room_party_level(self, a1); return 0)
TC_HOOK(H_PA_INIT, "pslot_init", 6, 0, pslot_init(self, (wl_room *)a1, a2, a3, a4, a5, a6); return 0)
TC_HOOK(H_PA_SKL, "member_skills_learn", 2, 0, member_skills_learn(self, a1, a2); return 0)
TC_HOOK(H_PA_GRANT, "room_items_grant", 1, 0, room_items_grant(self, a1); return 0)
TC_HOOK(H_PA_LEAD, "room_leader_set", 1, 0, room_leader_set(self, (wl_pslot *)a1); return 0)
TC_HOOK0(H_PA_STORY, "room_story_setup", 0, room_story_setup(self); return 0)
TC_HOOK0(H_PA_STORY2, "room_story_setup2", 0, room_story_setup2(self); return 0)
ST_HOOK(H_PA_SKROW, "skill_row_find", 1, 1, return (intptr_t)skill_row_find(a1))
TC_HOOK(H_PA_FITS, "item_fits", 1, 1, return item_fits(self, CP(a1)))
TC_HOOK(H_PA_STAT, "pslot_status_set", 2, 0, pslot_status_set(self, a1, (void *)a2); return 0)
TC_HOOK0(H_PA_REVIVE, "pslot_revive_mark", 0, pslot_revive_mark(self); return 0)
ST_HOOK(H_PA_ISTATS, "item_stats_add", 2, 0, item_stats_add(CP(a1), (uint8_t *)a2); return 0)
TC_HOOK(H_PA_REFRESH, "room_stats_refresh", 1, 0, room_stats_refresh(self, a1); return 0)
TC_HOOK(H_PA_EQUIP, "room_item_equip", 3, 0, room_item_equip(self, U16(a1), a2, a3); return 0)
TC_HOOK0(H_PA_DISMISS, "room_party_dismiss", 0, room_party_dismiss(self); return 0)
TC_HOOK(H_PA_LEAVE, "room_member_leave", 2, 0, room_member_leave(self, (wl_pslot *)a1, a2); return 0)
TC_HOOK(H_PA_JOIN, "room_member_join", 3, 1, return (intptr_t)room_member_join(self, a1, (wl_char *)a2, a3))
TC_HOOK(H_PA_ADD, "room_member_add", 6, 1, return (intptr_t)room_member_add(self, U16(a1), a2, a3, a4, a5, a6))
TC_HOOK(H_PA_IADD, "room_item_add", 5, 1, return room_item_add(self, a1, (uint16_t *)a2, a3, (uint8_t)a4, a5))
ST_HOOK(H_PA_SROW, "stat_row_find", 2, 1, return (intptr_t)stat_row_find(a1, a2))
ST_HOOK(H_PA_PROF, "char_profile_find", 1, 1, return (intptr_t)char_profile_find(a1))
TC_HOOK(H_PA_RECOMP, "stats_recompute", 3, 0, stats_recompute(self, a1, (uint8_t *)a2, CP(a3)); return 0)
TC_HOOK(H_PFACE, "part_facing", 1, 1, return part_facing(self, a1))
/* the scene loop itself — REPLACE only (a whole session per call) */
typedef int32_t (__attribute__((thiscall)) *scloop_t)(uint8_t *, int32_t, uint32_t, int32_t, int32_t *, int32_t);
static int32_t __attribute__((thiscall)) w_scloop(uint8_t *ss, int32_t save, uint32_t sub, int32_t demo, int32_t *out, int32_t start)
{
    hook_t *h = &H[H_SLOOP];
    h->calls++;
    if (h->mode == 2) { port_world(); return scene_loop(ss, save, (uint16_t)sub, demo, out, start); }
    return ((scloop_t)(uintptr_t)h->va)(ss, save, sub, demo, out, start);
}

/* ---- the character init (src/char_init.c) — full state, per-code coverage ---- */
typedef void (__attribute__((thiscall)) *cinit_t)(wl_char *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                                                int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef struct { wl_char *c; int32_t v[14]; } cinit_ctx;
static void cinit_retail(void *p)
{
    cinit_ctx *x = p; int32_t *v = x->v;
    ((cinit_t)(uintptr_t)H[H_CINIT].va)(x->c, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13]);
}
static void cinit_port(void *p)
{
    cinit_ctx *x = p; int32_t *v = x->v;
    port_world();
    char_init(x->c, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13]);
}
#define NCCODES 160
static int32_t g_ccode_id[NCCODES]; static long g_ccodes[NCCODES]; static int g_nccodes;
static void __attribute__((thiscall)) H_CINIT_w(wl_char *c, int32_t x0, int32_t y0, int32_t an, int32_t f8, int32_t fc, int32_t code,
                                                 int32_t a8, int32_t fa, int32_t hd, int32_t a11, int32_t a12, int32_t a13,
                                                 int32_t a14, int32_t a15)
{
    hook_t *h = &H[H_CINIT];
    cinit_ctx x = { c, { x0, y0, an, f8, fc, code, a8, fa, hd, a11, a12, a13, a14, a15 } };
    if (h->mode == 2) { h->calls++; cinit_port(&x); return; }
    if (!sample(h)) { cinit_retail(&x); return; }
    char what[64]; snprintf(what, sizeof what, "code %d handle %d", code, hd);
    if (!full_shadow(h, "char_init", cinit_retail, cinit_port, &x, what)) {
        int k; for (k = 0; k < g_nccodes && g_ccode_id[k] != code; k++) {}
        if (k == g_nccodes && g_nccodes < NCCODES) g_ccode_id[g_nccodes++] = code;
        if (k < NCCODES) g_ccodes[k]++;
    }
}
#define CDEF_ROW(id, n, va) [id] = { n, va, (void *)id##_w, 0, 1, 0, 0, 0, 0, "" },

static hook_t H[H_N] = {
    [H_RAND] = { "rand", 0, (void *)w_rand, 0, 1, 0, 0, 0, 0, "" },
    [H_POLL] = { "input_poll", 0x43c110u, (void *)w_poll, 0, 1, 0, 0, 0, 0, "" },
    [H_MOVER_V] = { "mover_v", 0x54e990u, (void *)w_mover_v, 0, 1, 0, 0, 0, 0, "" },
    [H_SWEEP_H] = { "sweep_h", 0x54ded0u, (void *)w_sweep_h, 0, 1, 0, 0, 0, 0, "" },
    [H_ENT_MOVE_H] = { "ent_move_h", 0x54db10u, (void *)w_ent_h, 0, 1, 0, 0, 0, 0, "" },
    [H_ENT_MOVE_V] = { "ent_move_v", 0x54e5c0u, (void *)w_ent_v, 0, 1, 0, 0, 0, 0, "" },
    [H_MAP_DECODE] = { "map_decode", 0x587e00u, (void *)w_map_decode, 0, 1, 0, 0, 0, 0, "" },
    [H_CAM_FOLLOW] = { "cam_follow", 0x43d1d0u, (void *)w_cam_follow, 0, 1, 0, 0, 0, 0, "" },
    [H_CAM_SHAKE]  = { "cam_shake",  0x43d340u, (void *)w_cam_shake,  0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_AI]    = { "char_ai",    0x478ba0u, (void *)w_char_ai,    0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_PHYS]  = { "char_physics", 0x467bb0u, (void *)w_char_phys, 0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_ANIM]  = { "char_anim",  0x466930u, (void *)w_char_anim,  0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_INT]   = { "char_integrate", 0x442a70u, (void *)w_char_int, 0, 1, 0, 0, 0, 0, "" },
    [H_HAZARD]     = { "hazard_probe", 0x440f40u, (void *)w_hazard, 0, 1, 0, 0, 0, 0, "" },
    [H_NPC_AI]     = { "npc_ai", 0x43f880u, (void *)w_npc_ai, 0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_APPLY] = { "char_apply", 0x485fc0u, (void *)w_char_apply, 0, 1, 0, 0, 0, 0, "" },
    [H_WORLD_TICK] = { "world_tick", 0x46cd70u, (void *)w_world_tick, 0, 1, 0, 0, 0, 0, "" },
    [H_FX_ADD]     = { "fx_add",      0x448cb0u, (void *)w_fx_add,      0, 1, 0, 0, 0, 0, "" },
    [H_FX_TICK]    = { "fx_tick",     0x54d090u, (void *)w_fx_tick,     0, 1, 0, 0, 0, 0, "" },
    [H_SPLASH]     = { "splash",      0x46d7f0u, (void *)w_splash,      0, 1, 0, 0, 0, 0, "" },
    [H_SND_RELEASE] = { "snd_release", 0x4537d0u, (void *)w_snd_release, 0, 1, 0, 0, 0, 0, "" },
    [H_SND_ALL]    = { "snd_all",     0x46d180u, (void *)w_snd_all,     0, 1, 0, 0, 0, 0, "" },
    [H_SND_ONE]    = { "snd_one",     0x46d4a0u, (void *)w_snd_one,     0, 1, 0, 0, 0, 0, "" },
    [H_ACTOR]      = { "actor_update", 0x54f980u, (void *)w_actor,      0, 1, 0, 0, 0, 0, "" },
    [H_PARTICLE]   = { "particle_tick", 0x46e510u, (void *)w_particle,  0, 1, 0, 0, 0, 0, "" },
    [H_BEHAVE]     = { "char_behave", 0x47b990u, (void *)w_behave,      0, 1, 0, 0, 0, 0, "" },
    [H_STEP_OK]    = { "step_ok",     0x47dbb0u, (void *)w_step_ok,     0, 1, 0, 0, 0, 0, "" },
    [H_PICK_CHAR]  = { "pick_char",   0x47e330u, (void *)w_pick,        0, 1, 0, 0, 0, 0, "" },
    [H_WALK_TO]    = { "walk_to",     0x484c10u, (void *)w_walk_to,     0, 1, 0, 0, 0, 0, "" },
    [H_PACE]       = { "pace",        0x4857f0u, (void *)w_pace,        0, 1, 0, 0, 0, 0, "" },
    [H_PART_CX]    = { "part_cx",     0x44e640u, (void *)w_part_cx,     0, 1, 0, 0, 0, 0, "" },
    [H_PART_CY]    = { "part_cy",     0x44e680u, (void *)w_part_cy,     0, 1, 0, 0, 0, 0, "" },
    [H_VP_LOAD]    = { "vpool_load",  0x409e90u, (void *)w_vpload,      0, 1, 0, 0, 0, 0, "" },
    [H_VP_TAKE]    = { "vpool_take",  0x40b710u, (void *)w_vptake,     0, 1, 0, 0, 0, 0, "" },
    [H_SFX_PLACE]  = { "sfx_place",   0x40bb90u, (void *)w_sfx_place,   0, 1, 0, 0, 0, 0, "" },
    [H_SFX_PLAY]   = { "sfx_play",    0x411390u, (void *)w_sfxplay,    0, 1, 0, 0, 0, 0, "" },
    [H_SFX_AT]     = { "sfx_play_at", 0x407ba0u, (void *)w_sfxat,       0, 1, 0, 0, 0, 0, "" },
    [H_VOICE_PLACE] = { "voice_place", 0x40a010u, (void *)w_voice_place, 0, 1, 0, 0, 0, 0, "" },
    [H_VOICE_PLAY] = { "voice_play",  0x4099a0u, (void *)w_vplay,  0, 1, 0, 0, 0, 0, "" },
    [H_SND_START]  = { "snd_start",   0x450ef0u, (void *)w_snd_start,   0, 1, 0, 0, 0, 0, "" },
    [H_VOICE_CUE]  = { "voice_cue",   0x445fd0u, (void *)w_vcue,        0, 1, 0, 0, 0, 0, "" },
    [H_VS_TICK]    = { "voiceset_tick", 0x446400u, (void *)w_vstick,    0, 1, 0, 0, 0, 0, "" },
    [H_VLINE_STOP] = { "vline_stop",  0x4463d0u, (void *)w_vlstop,      0, 1, 0, 0, 0, 0, "" },
    [H_VS_SOUNDING] = { "voiceset_sounding", 0x4464f0u, (void *)w_vssnd, 0, 1, 0, 0, 0, 0, "" },
    [H_UNLIST]     = { "entity_unlist",  0x555fc0u, (void *)w_unlist, 0, 1, 0, 0, 0, 0, "" },
    [H_RELEASE]    = { "entity_release", 0x556180u, (void *)w_erel,   0, 1, 0, 0, 0, 0, "" },
    [H_ACTOR_REL]  = { "actor_release",  0x41ea70u, (void *)w_arel,   0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_REL]   = { "char_release",   0x4467d0u, (void *)w_crel,   0, 1, 0, 0, 0, 0, "" },
    [H_SET_COLL]   = { "set_collide",    0x556380u, (void *)w_scoll,  0, 1, 0, 0, 0, 0, "" },
    [H_CARRY]      = { "carry_riders",   0x5563d0u, (void *)w_carry,  0, 1, 0, 0, 0, 0, "" },
    [H_LIST_RM]    = { "list_remove",    0x40bd90u, (void *)w_lrm,    0, 1, 0, 0, 0, 0, "" },
    [H_ST_STUCK]   = { "status_stuck",   0x485f70u, (void *)w_stuck,  0, 1, 0, 0, 0, 0, "" },
    [H_GRID_B]     = { "grid_b_word",    0x487250u, (void *)w_gridb,  0, 1, 0, 0, 0, 0, "" },
    [H_PIT]        = { "pit_check",      0x487730u, (void *)w_pit,    0, 1, 0, 0, 0, 0, "" },
    [H_RESPAWN]    = { "respawn_mark",   0x487570u, (void *)w_resp,   0, 1, 0, 0, 0, 0, "" },
    [H_MODS]       = { "mods_reset",     0x487530u, (void *)w_mods,   0, 1, 0, 0, 0, 0, "" },
    [H_AI_RESET]   = { "ai_reset",       0x41e180u, (void *)w_aires,  0, 1, 0, 0, 0, 0, "" },
    [H_SLOWED]     = { "body_slowed",    0x4593c0u, (void *)w_slowed, 0, 1, 0, 0, 0, 0, "" },
    [H_LOOKUP]     = { "set_look_up",    0x41ef80u, (void *)w_look,   0, 1, 0, 0, 0, 0, "" },
    [H_TARGETABLE] = { "targetable",     0x40d960u, (void *)w_targ,   0, 1, 0, 0, 0, 0, "" },
    [H_SND_LOOP]   = { "snd_loop",       0x476880u, (void *)w_sloop,  0, 1, 0, 0, 0, 0, "" },
    [H_SFX_JIT]    = { "sfx_play_jit",   0x489280u, (void *)w_sjit,   0, 1, 0, 0, 0, 0, "" },
    [H_FORCE_ACT]  = { "force_action",   0x4874b0u, (void *)w_fact,   0, 1, 0, 0, 0, 0, "" },
    [H_GRID_C]     = { "grid_c_at_feet", 0x4872d0u, (void *)w_gridc,  0, 1, 0, 0, 0, 0, "" },
    [H_ADD_HP]     = { "add_hp",         0x5b54d0u, (void *)w_ahp,    0, 1, 0, 0, 0, 0, "" },
    [H_ADD_MP]     = { "add_mp",         0x5b5580u, (void *)w_amp,    0, 1, 0, 0, 0, 0, "" },
    [H_AMBIENT]    = { "ambient_timer",  0x5531b0u, (void *)w_amb,    0, 1, 0, 0, 0, 0, "" },
    [H_IDLE_VOICE] = { "idle_voice",     0x467380u, (void *)w_idlev,  0, 1, 0, 0, 0, 0, "" },
    [H_SCATTER]    = { "entity_scatter", 0x453960u, (void *)w_scat,   0, 1, 0, 0, 0, 0, "" },
    [H_SYS_MODE]   = { "sys_by_mode",    0x426440u, (void *)w_smode,  0, 1, 0, 0, 0, 0, "" },
    [H_CRATE]      = { "actor_crate",    0x555790u, (void *)w_crate,  0, 1, 0, 0, 0, 0, "" },
    [H_BREAK]      = { "actor_break",    0x555bd0u, (void *)w_brk,    0, 1, 0, 0, 0, 0, "" },
    [H_SLIDE_V]    = { "actor_slide_v",  0x555e90u, (void *)w_slv,    0, 1, 0, 0, 0, 0, "" },
    [H_RIDERS]     = { "count_riders",   0x5564e0u, (void *)w_riders, 0, 1, 0, 0, 0, 0, "" },
    [H_DUST]       = { "entity_dust",    0x40ab80u, (void *)w_dust,   0, 1, 0, 0, 0, 0, "" },
    [H_ANCHOR]     = { "entity_anchor",  0x41ee60u, (void *)w_anchor, 0, 1, 0, 0, 0, 0, "" },
    [H_SPAWN]      = { "particle_spawn", 0x557370u, (void *)w_spawn,  0, 1, 0, 0, 0, 0, "" },
    [H_TRACK]      = { "set_track",      0x426db0u, (void *)w_track,  0, 1, 0, 0, 0, 0, "" },
    [H_GRID_REFRESH] = { "grid_refresh", 0x54c640u, (void *)w_grefresh, 0, 1, 0, 0, 0, 0, "" },
    [H_GRID_MARK]  = { "grid_mark",      0x54c9d0u, (void *)w_gmark,  0, 1, 0, 0, 0, 0, "" },
    [H_PINIT]      = { "particle_init",  0x557550u, (void *)w_pinit,  0, 1, 0, 0, 0, 0, "" },
    [H_ESETUP]     = { "entity_setup",   0x426620u, (void *)w_esetup, 0, 1, 0, 0, 0, 0, "" },
    [H_RIDE]       = { "entity_ride",    0x55cd50u, (void *)w_ride,   0, 1, 0, 0, 0, 0, "" },
    [H_RIDE_PART]  = { "entity_ride_part", 0x476c70u, (void *)w_ridep, 0, 1, 0, 0, 0, 0, "" },
    [H_POLAR_DIV]  = { "polar_div",      0x460e40u, (void *)w_pdiv,   0, 1, 0, 0, 0, 0, "" },
    [H_POLAR_MILLI] = { "polar_milli",   0x55cdb0u, (void *)w_pmil,   0, 1, 0, 0, 0, 0, "" },
    [H_FADE_A]     = { "tint_fade_a",    0x448c80u, (void *)w_fade_a, 0, 1, 0, 0, 0, 0, "" },
    [H_FADE_B]     = { "tint_fade_b",    0x476710u, (void *)w_fade_b, 0, 1, 0, 0, 0, 0, "" },
    [H_TRACK_ON]   = { "track_on",       0x426d70u, (void *)w_trkon,  0, 1, 0, 0, 0, 0, "" },
    [H_SND_ALLOC]  = { "snd_alloc",      0x55cbc0u, (void *)w_salloc, 0, 1, 0, 0, 0, 0, "" },
    [H_JITTER]     = { "clip_jitter",    0x426ec0u, (void *)w_jit,    0, 1, 0, 0, 0, 0, "" },
    [H_PARTS_FREE] = { "parts_free",     0x4272e0u, (void *)w_pfree,  0, 1, 0, 0, 0, 0, "" },
    [H_SET_ANGLE]  = { "set_angle",      0x476d00u, (void *)w_sang,   0, 1, 0, 0, 0, 0, "" },
    [H_FX_RESET]   = { "fx_reset",       0x4265e0u, (void *)w_fxr,    0, 1, 0, 0, 0, 0, "" },
    [H_SKILL_AVAIL] = { "skill_avail",   0x402a60u, (void *)w_savail, 0, 1, 0, 0, 0, 0, "" },
    [H_PARTY_FIND] = { "party_find",     0x5b5990u, (void *)w_pfind,  0, 1, 0, 0, 0, 0, "" },
    [H_FLOOR_BELOW] = { "floor_below",   0x4060d0u, (void *)w_fbelow, 0, 1, 0, 0, 0, 0, "" },
    [H_GPASS]      = { "gimmick_pass",   0x59afd0u, (void *)w_gpass,  0, 1, 0, 0, 0, 0, "" },
    [H_GSTEP]      = { "gimmick_step",   0x59b170u, (void *)w_gstep,  0, 1, 0, 0, 0, 0, "" },
    [H_SWSET]      = { "switch_set",     0x40afe0u, (void *)w_swset,  0, 1, 0, 0, 0, 0, "" },
    [H_SWLINK]     = { "switch_link",    0x40be30u, (void *)w_swlink, 0, 1, 0, 0, 0, 0, "" },
    [H_EVRUN]      = { "event_run",      0x40b8f0u, (void *)w_evrun,  0, 1, 0, 0, 0, 0, "" },
    [H_UNLOAD]     = { "sys_unload",     0x43e3a0u, (void *)w_unload, 0, 1, 0, 0, 0, 0, "" },
    [H_UNLIST_ALL] = { "actor_unlist_all", 0x40c300u, (void *)w_unlall, 0, 1, 0, 0, 0, 0, "" },
    [H_GLOW]       = { "switch_glow",    0x408100u, (void *)w_glow,   0, 1, 0, 0, 0, 0, "" },
    [H_RECT_HIT]   = { "rect_hit",       0x4804b0u, (void *)w_rhit,   0, 1, 0, 0, 0, 0, "" },
    [H_CHARS_COUNT] = { "chars_in_rect_count", 0x556610u, (void *)w_ccount, 0, 1, 0, 0, 0, 0, "" },
    [H_CHAR_IN]    = { "char_in_rect",   0x5567a0u, (void *)w_cin,    0, 1, 0, 0, 0, 0, "" },
    [H_RIDING]     = { "char_riding",    0x5568e0u, (void *)w_riding, 0, 1, 0, 0, 0, 0, "" },
    [H_PART_FIND]  = { "particle_find",  0x556de0u, (void *)w_pfnd,   0, 1, 0, 0, 0, 0, "" },
    [H_SERIAL]     = { "next_serial",    0x40be00u, (void *)w_serial, 0, 1, 0, 0, 0, 0, "" },
    [H_SHOT]       = { "actor_shot",     0x40ac10u, (void *)w_shot,   0, 1, 0, 0, 0, 0, "" },
    [H_WARP]       = { "warp_request",   0x54ecf0u, (void *)w_warp,   0, 1, 0, 0, 0, 0, "" },
    [H_GATHER]     = { "party_gather",   0x54efd0u, (void *)w_gather, 0, 1, 0, 0, 0, 0, "" },
    [H_TPASS]      = { "touch_pass",     0x59bdd0u, (void *)w_tpass,  0, 1, 0, 0, 0, 0, "" },
    [H_TACTOR]     = { "touch_actor",    0x59c240u, (void *)w_tact,   0, 1, 0, 0, 0, 0, "" },
    [H_RFLUSH]     = { "room_input_flush", 0x411560u, (void *)w_rflush, 0, 1, 0, 0, 0, 0, "" },
    [H_PROBE_RECT] = { "probe_rect",     0x4812c0u, (void *)w_prect,  0, 1, 0, 0, 0, 0, "" },
    [H_NEAR]       = { "entity_near",    0x47b7c0u, (void *)w_near,   0, 1, 0, 0, 0, 0, "" },
    [H_ZPASS]      = { "zone_pass",      0x59a1f0u, (void *)w_zpass,  0, 1, 0, 0, 0, 0, "" },
    [H_ZHIT]       = { "zone_hit",       0x59a7c0u, (void *)w_zhit,   0, 1, 0, 0, 0, 0, "" },
    [H_B96_NEAR]   = { "band96_near",    0x59a880u, (void *)w_b96,    0, 1, 0, 0, 0, 0, "" },
    [H_RIN]        = { "room_input_update", 0x468a20u, (void *)w_rin, 0, 1, 0, 0, 0, 0, "" },
    [H_RCHAR]      = { "room_char_step", 0x590410u, (void *)w_rchar,  0, 1, 0, 0, 0, 0, "" },
    [H_RACT]       = { "room_actor_step", 0x58ffd0u, (void *)w_ract,  0, 1, 0, 0, 0, 0, "" },
    [H_RANIM]      = { "room_grid_anim", 0x58ff50u, (void *)w_ranim,  0, 1, 0, 0, 0, 0, "" },
    [H_RDOWN]      = { "room_party_down", 0x59ca10u, (void *)w_rdown, 0, 1, 0, 0, 0, 0, "" },
    [H_RLEAD]      = { "room_lead",      0x59d3f0u, (void *)w_rlead,  0, 1, 0, 0, 0, 0, "" },
    [H_TSOUNDS]    = { "timed_sounds_tick", 0x43e140u, (void *)w_tsnd, 0, 1, 0, 0, 0, 0, "" },
    [H_DIK]        = { "input_dik_produce", 0x46a880u, (void *)w_dik, 0, 1, 0, 0, 0, 0, "" },
    [H_ENC]        = { "encounter_tick", 0x58e780u, (void *)w_enc,    0, 1, 0, 0, 0, 0, "" },
    [H_FOES]       = { "foes_near",      0x58eeb0u, (void *)w_foes,   0, 1, 0, 0, 0, 0, "" },
    [H_PFOES]      = { "party_foes_near", 0x58f0b0u, (void *)w_pfoes, 0, 1, 0, 0, 0, 0, "" },
    [H_PSLOT]      = { "pslot_ok",       0x412db0u, (void *)w_pslot,  0, 1, 0, 0, 0, 0, "" },
    [H_BGM]        = { "bgm_pick",       0x40d9b0u, (void *)w_bgm,    0, 1, 0, 0, 0, 0, "" },
    [H_RLOOP]      = { "room_loop",      0x58f360u, (void *)w_rloop,  0, 1, 0, 0, 0, 0, "" },
    [H_REPOLL]     = { "input_repoll_flush", 0x49be60u, (void *)w_repoll, 0, 1, 0, 0, 0, 0, "" },
    [H_LROOM]      = { "load_room",      0x586010u, (void *)w_lroom,  0, 1, 0, 0, 0, 0, "" },
    [H_BODIES]     = { "entity_bodies_alloc", 0x58cf60u, (void *)w_bodies, 0, 1, 0, 0, 0, 0, "" },
    [H_LINKS]      = { "char_links_clear", 0x58d020u, (void *)w_links, 0, 1, 0, 0, 0, 0, "" },
    [H_VPOOLS]     = { "view_pools_free", 0x587d50u, (void *)w_vpools, 0, 1, 0, 0, 0, 0, "" },
    [H_SHAKECLR]   = { "shake_clear",    0x587d30u, (void *)w_shclr,  0, 1, 0, 0, 0, 0, "" },
    [H_VFADE]      = { "volume_fade",    0x43c9b0u, (void *)w_vfade,  0, 1, 0, 0, 0, 0, "" },
    [H_MAPID]      = { "char_map_id",    0x58cfe0u, (void *)w_mapid,  0, 1, 0, 0, 0, 0, "" },
    [H_MAPNPC]     = { "char_map_npc",   0x58cfc0u, (void *)w_mapnpc, 0, 1, 0, 0, 0, 0, "" },
    [H_PSCLR]      = { "pslot_status_clear", 0x58d060u, (void *)w_psclr, 0, 1, 0, 0, 0, 0, "" },
    [H_GATE]       = { "gate_actor",     0x4022d0u, (void *)w_gate,   0, 1, 0, 0, 0, 0, "" },
    [H_SREADY]     = { "sprite_slot_ready", 0x49a050u, (void *)w_sready, 0, 1, 0, 0, 0, 0, "" },
    [H_STINT]      = { "sprite_slot_tint", 0x491740u, (void *)w_stint, 0, 1, 0, 0, 0, 0, "" },
    [H_TSFADE]     = { "timed_sounds_fade", 0x587ce0u, (void *)w_tsfade, 0, 1, 0, 0, 0, 0, "" },
    [H_FILL]       = { "fill_lookup",    0x587db0u, (void *)w_fill,   0, 1, 0, 0, 0, 0, "" },
    [H_WNEW]       = { "world_new",      0x4017d0u, (void *)w_wnew,   0, 1, 0, 0, 0, 0, "" },
    [H_WFREE]      = { "world_free",     0x54bfb0u, (void *)w_WFREE,  0, 1, 0, 0, 0, 0, "" },
    [H_EFREE]      = { "entity_free",    0x401cb0u, (void *)w_EFREE,  0, 1, 0, 0, 0, 0, "" },
    [H_AFREE]      = { "actor_free",     0x54c2e0u, (void *)w_AFREE,  0, 1, 0, 0, 0, 0, "" },
    [H_CFREE]      = { "char_free",      0x54c3c0u, (void *)w_CFREE,  0, 1, 0, 0, 0, 0, "" },
    [H_CBLK]       = { "char_block_init", 0x401c70u, (void *)w_CBLK,  0, 1, 0, 0, 0, 0, "" },
    [H_WLEAD]      = { "world_leader",   0x54bf90u, (void *)w_wlead,  0, 1, 0, 0, 0, 0, "" },
    [H_CSNAP]      = { "camera_snap",    0x58e6a0u, (void *)w_csnap,  0, 1, 0, 0, 0, 0, "" },
    [H_ZRESET]     = { "zones_reset",    0x58d0b0u, (void *)w_zreset, 0, 1, 0, 0, 0, 0, "" },
    [H_QSTATE]     = { "quest_state",    0x58d3b0u, (void *)w_qstate, 0, 1, 0, 0, 0, 0, "" },
    [H_VREC]       = { "view_record_init", 0x58d210u, (void *)w_vrec, 0, 1, 0, 0, 0, 0, "" },
    [H_RSAVE]      = { "room_state_save", 0x58e330u, (void *)w_RSAVE, 0, 1, 0, 0, 0, 0, "" },
    [H_MFREE]      = { "map_res_free",   0x5878a0u, (void *)w_MFREE,  0, 1, 0, 0, 0, 0, "" },
    [H_MDEL]       = { "map_res_delete", 0x59db70u, (void *)w_mdel,   0, 1, 0, 0, 0, 0, "" },
    [H_PHDEL]      = { "pool_head_delete", 0x562d50u, (void *)w_phdel, 0, 1, 0, 0, 0, 0, "" },
    [H_FORM]       = { "room_formation", 0x41dc90u, (void *)w_form,   0, 1, 0, 0, 0, 0, "" },
    [H_FOLLOW]     = { "char_follow_set", 0x41e020u, (void *)w_follow, 0, 1, 0, 0, 0, 0, "" },
    [H_ASTATE]     = { "actor_state_restore", 0x41e600u, (void *)w_astate, 0, 1, 0, 0, 0, 0, "" },
    [H_PTINT]      = { "palette_tint",   0x4182d0u, (void *)w_ptint,  0, 1, 0, 0, 0, 0, "" },
    [H_MLOAD]      = { "map_res_load",   0x587970u, (void *)w_mload,  0, 1, 0, 0, 0, 0, "" },
    [H_SFREE]      = { "slot_frames_free", 0x417870u, (void *)w_sfree, 0, 1, 0, 0, 0, 0, "" },
    [H_SFREEALL]   = { "slot_frames_free_all", 0x4118b0u, (void *)w_sfreeall, 0, 1, 0, 0, 0, 0, "" },
    [H_SGRADED]    = { "slots_free_graded", 0x56df10u, (void *)w_sgraded, 0, 1, 0, 0, 0, 0, "" },
    [H_PSWAP]      = { "pal_swap_apply", 0x417bc0u, (void *)w_pswap,  0, 1, 0, 0, 0, 0, "" },
    [H_POPULATE]   = { "room_populate",  0x58d460u, (void *)w_popul,  0, 1, 0, 0, 0, 0, "" },
    [H_SINIT]      = { "structure_init", 0x438a60u, (void *)w_sinit,  0, 1, 0, 0, 0, 0, "" },
    [H_SETH]       = { "entity_set_handle", 0x555f00u, (void *)w_seth, 0, 1, 0, 0, 0, 0, "" },
    [H_DINIT]      = { "device_init",    0x431e30u, (void *)w_dinit,  0, 1, 0, 0, 0, 0, "" },
    [H_SNDFREE]    = { "entity_snd_free", 0x426490u, (void *)w_sndfree, 0, 1, 0, 0, 0, 0, "" },
    CDEF_HOOKS(CDEF_ROW)
    [H_SLOOP]      = { "scene_loop", 0x59f2c0u, (void *)w_scloop, 0, 1, 0, 0, 0, 0, "" },
};

static const void *rh_data_identity(uint32_t va) { return (const void *)(uintptr_t)va; }
/* engine_calls.h slots -> retail (EN-old VAs; the roots are retail's globals) */
static void bind_engine_calls(void)
{
    g_eng.tick   = iat_lookup("GetTickCount");          /* rhost's virtual clock override */
    g_eng.sys    = (void *)(uintptr_t)0x8a6b60u;
    g_eng.keycfg = (wl_keycfg ***)(uintptr_t)0x8a6e80u;
    g_eng.log    = (void *)(uintptr_t)0x8a9530u;
    *(void **)&g_eng.integrate_442a70    = (void *)(uintptr_t)0x442a70u;
    *(void **)&g_eng.skill_479f90        = (void *)(uintptr_t)0x479f90u;
    *(void **)&g_eng.skill_47a300        = (void *)(uintptr_t)0x47a300u;
    *(void **)&g_eng.skill_47a420        = (void *)(uintptr_t)0x47a420u;
    *(void **)&g_eng.skill_47a7f0        = (void *)(uintptr_t)0x47a7f0u;
    *(void **)&g_eng.target_ok_47adb0    = (void *)(uintptr_t)0x47adb0u;
    *(void **)&g_eng.dsb_volume_5bb870   = (void *)(uintptr_t)0x5bb870u;
    *(void **)&g_eng.dsb_pan_5bb880      = (void *)(uintptr_t)0x5bb880u;
    *(void **)&g_eng.dsb_freq_5bb890     = (void *)(uintptr_t)0x5bb890u;
    *(void **)&g_eng.dsb_play_5bb8c0     = (void *)(uintptr_t)0x5bb8c0u;
    *(void **)&g_eng.dsb_stop_5bb8e0     = (void *)(uintptr_t)0x5bb8e0u;
    *(void **)&g_eng.dsb_create_5bb250   = (void *)(uintptr_t)0x5bb250u;
    *(void **)&g_eng.dsb_dup_5bb2f0      = (void *)(uintptr_t)0x5bb2f0u;
    *(void **)&g_eng.log_fail_406440     = (void *)(uintptr_t)0x406440u;
    *(void **)&g_eng.dsb_release_5bb3a0  = (void *)gd_5bb3a0;
    *(void **)&g_eng.dbg_out             = iat_lookup("OutputDebugStringA");
    *(void **)&r_DeleteObject            = iat_lookup("DeleteObject");
    *(void **)&g_eng.gdi_delete          = (void *)gd_delobj;
    g_eng.data = rh_data_identity;
    *(void **)&g_eng.op_new    = (void *)(uintptr_t)0x5bf00du;   /* operator new (cdecl) */
    *(void **)&g_eng.op_delete = (void *)(uintptr_t)0x5bef0eu;   /* operator delete (cdecl) */
#define B(n) *(void **)&g_eng.h##n = (void *)(uintptr_t)0x##n##u
    B(467260); B(44a740); B(45a300); B(44bc10); B(44c250); B(44bf60); B(446900);
    B(452020); B(449810); B(4539b0); B(44d8f0); B(448dc0); B(452730); B(457ff0); B(45e830);
    B(455d80); B(44b160); B(4516e0); B(4484b0); B(4512d0); B(44a200); B(44e8d0); B(44f690); B(450810);
    B(44d2e0); B(456d30); B(461790); B(445e50);
#undef B
#define B(n) *(void **)&g_eng.h##n = (void *)(uintptr_t)0x##n##u
    B(5b4d00);
    B(5b5600); B(487dc0);
    B(4878b0); B(4895c0);
   
    B(4710c0); B(46da00);
    B(408b90); B(54f140);
    B(40ac90); B(55cfc0); B(54ed80); B(468b10); B(5bc0c0); B(409520);
    B(5b1030); B(48c150); B(495e40); B(59d160); B(59aa40); B(58ffa0); B(590480); B(590f20); B(590ca0); B(59d540);
    B(591120); B(562a70); B(58ffc0); B(59a910); B(415350); B(590230); B(59c750); B(59ca90); B(59ad60); B(5908a0);
    B(499ab0);
    *(void **)&g_eng.flip_5b8fc0  = g_rh_flip_stub;    /* the virtual clock steps there */
    *(void **)&g_eng.easer_43d1d0 = g_rh_easer_stub;   /* the tick count (+ a camera hook chained behind) */
    B(5ba120); B(5ba290);
    g_eng.tod = (int32_t *)(uintptr_t)0x8a93fcu;
    B(4178e0); B(418470); B(417c40);
    *(void **)&g_eng.find_res = iat_lookup("FindResourceA");
    *(void **)&g_eng.load_res = iat_lookup("LoadResource");
    *(void **)&g_eng.lock_res = iat_lookup("LockResource");
    B(56dc20); B(41a890); B(5bf3ee); B(426110); B(560900); B(5bf5db);
    B(428c00); B(429060); B(429db0); B(42a500); B(42a9a0); B(42b9a0); B(42c130); B(42c510); B(42c970);
    B(42cce0); B(42d3c0); B(42da50); B(42e290); B(42eb20); B(42f840); B(42fbf0); B(42ff00); B(430400);
    B(561d20); B(409150); B(55fdb0); B(41d780); B(4e6520); B(4e61d0); B(41b670); B(4c5af0); B(52fd80); B(5a3870); B(5a00c0);
    B(5a3670); B(56c640); B(5b6060); B(5b60d0); B(5b61b0); B(5b6200); B(5b6bc0); B(5b6340); B(5b64b0);
    B(41cfd0);
    g_eng.world = (wl_world **)(uintptr_t)WORLD_PTR_VA;
    g_eng.quiet = (int32_t *)(uintptr_t)0x8a6b54u;
    g_eng.dlog  = (void *)(uintptr_t)0x8a6620u;
    B(552c70); B(552f60); B(5534d0); B(553520); B(553ec0); B(554570); B(554c40); B(5556d0);
    B(556e50);
    B(47da60); B(482240); B(482460); B(482ac0); B(47de50); B(47f670); B(47f9d0);
    B(481ac0); B(482e10); B(4834f0); B(484ea0); B(4854e0); B(485990); B(47d790); B(481310); B(482880);
    B(47e010); B(47eae0); B(483fb0); B(483de0); B(485350); B(483b50); B(482380); B(484530); B(47e7c0); B(47ecd0); B(441f50); B(407f30);
#undef B
    /* the external-effect calls go through the shadow guards */
    *(void **)&g_eng.h5b9390 = (void *)gd_5b9390;
    *(void **)&g_eng.h40c380 = (void *)gd_40c380;
    *(void **)&g_eng.h40a5d0 = (void *)gd_40a5d0;
    *(void **)&g_eng.h40fe00 = (void *)gd_40fe00;
    *(void **)&g_eng.h5ba4c0 = (void *)gd_5ba4c0;
    *(void **)&g_eng.h5ba2d0 = (void *)gd_5ba2d0;
    *(void **)&g_eng.h5ba650 = (void *)gd_5ba650;
    *(void **)&g_eng.h49bf90 = (void *)gd_49bf90;
}

void rh_hooks_install(void)
{
    if (!strcmp(g_ed->tag, "enold")) bind_engine_calls();
    rng_bind((uint32_t *)(uintptr_t)g_ed->rng_seed);   /* ported code draws from retail's LCG word */
    H[H_RAND].va = g_ed->rand;
    /* input_poll_consume: EN-old 0x43c110; EN-SE 0x4383c0 (crosswalk: masked bytes identical) */
    H[H_POLL].va = !strcmp(g_ed->tag, "enold") ? 0x43c110u : !strcmp(g_ed->tag, "ense-en") ? 0x4383c0u : 0;
    if (strcmp(g_ed->tag, "enold"))   /* EN-old VAs only (SE: crosswalk TODO) */
        H[H_MOVER_V].va = H[H_SWEEP_H].va = H[H_ENT_MOVE_H].va = H[H_ENT_MOVE_V].va = H[H_MAP_DECODE].va =
        H[H_CAM_FOLLOW].va = H[H_CAM_SHAKE].va = H[H_CHAR_AI].va = H[H_CHAR_PHYS].va = H[H_CHAR_ANIM].va = H[H_CHAR_INT].va = H[H_HAZARD].va = H[H_NPC_AI].va = H[H_CHAR_APPLY].va = H[H_WORLD_TICK].va =
        H[H_FX_ADD].va = H[H_FX_TICK].va = H[H_SPLASH].va = H[H_SND_RELEASE].va = H[H_SND_ALL].va = H[H_SND_ONE].va = H[H_ACTOR].va = H[H_PARTICLE].va = H[H_BEHAVE].va =
        H[H_STEP_OK].va = H[H_PICK_CHAR].va = H[H_WALK_TO].va = H[H_PACE].va = H[H_PART_CX].va = H[H_PART_CY].va =
        H[H_VP_LOAD].va = H[H_VP_TAKE].va = H[H_SFX_PLACE].va = H[H_SFX_PLAY].va = H[H_SFX_AT].va = H[H_VOICE_PLACE].va =
        H[H_VOICE_PLAY].va = H[H_SND_START].va =
        H[H_VOICE_CUE].va = H[H_VS_TICK].va = H[H_VLINE_STOP].va = H[H_VS_SOUNDING].va =
        H[H_UNLIST].va = H[H_RELEASE].va = H[H_ACTOR_REL].va = H[H_CHAR_REL].va = H[H_SET_COLL].va = H[H_CARRY].va = H[H_LIST_RM].va =
        H[H_ST_STUCK].va = H[H_GRID_B].va = H[H_PIT].va = H[H_RESPAWN].va = H[H_MODS].va = H[H_AI_RESET].va = H[H_SLOWED].va =
        H[H_LOOKUP].va = H[H_TARGETABLE].va = H[H_SND_LOOP].va = H[H_SFX_JIT].va = H[H_FORCE_ACT].va = H[H_GRID_C].va =
        H[H_ADD_HP].va = H[H_ADD_MP].va =
        H[H_AMBIENT].va = H[H_IDLE_VOICE].va = H[H_SCATTER].va = H[H_SYS_MODE].va =
        H[H_CRATE].va = H[H_BREAK].va = H[H_SLIDE_V].va = H[H_RIDERS].va = H[H_DUST].va = H[H_ANCHOR].va = H[H_SPAWN].va = H[H_TRACK].va = H[H_GRID_REFRESH].va = H[H_GRID_MARK].va =
        H[H_PINIT].va = H[H_ESETUP].va = H[H_RIDE].va = H[H_RIDE_PART].va = H[H_POLAR_DIV].va = H[H_POLAR_MILLI].va = H[H_FADE_A].va =
        H[H_FADE_B].va = H[H_TRACK_ON].va = H[H_SND_ALLOC].va = H[H_JITTER].va = H[H_PARTS_FREE].va = H[H_SET_ANGLE].va = H[H_FX_RESET].va =
        H[H_SKILL_AVAIL].va = H[H_PARTY_FIND].va = H[H_FLOOR_BELOW].va =
        H[H_GPASS].va = H[H_GSTEP].va = H[H_SWSET].va = H[H_SWLINK].va = H[H_EVRUN].va = H[H_UNLOAD].va = H[H_UNLIST_ALL].va =
        H[H_GLOW].va = H[H_RECT_HIT].va = H[H_CHARS_COUNT].va = H[H_CHAR_IN].va = H[H_RIDING].va = H[H_PART_FIND].va =
        H[H_SERIAL].va = H[H_SHOT].va = H[H_WARP].va = H[H_GATHER].va =
        H[H_TPASS].va = H[H_TACTOR].va = H[H_RFLUSH].va = H[H_PROBE_RECT].va = H[H_NEAR].va =
        H[H_ZPASS].va = H[H_ZHIT].va = H[H_B96_NEAR].va =
        H[H_RIN].va = H[H_RCHAR].va = H[H_RACT].va = H[H_RANIM].va = H[H_RDOWN].va = H[H_RLEAD].va = H[H_TSOUNDS].va = H[H_DIK].va =
        H[H_ENC].va = H[H_FOES].va = H[H_PFOES].va = H[H_PSLOT].va = H[H_BGM].va =
        H[H_RLOOP].va = H[H_REPOLL].va =
        H[H_LROOM].va = H[H_BODIES].va = H[H_LINKS].va = H[H_VPOOLS].va = H[H_SHAKECLR].va = H[H_VFADE].va = H[H_MAPID].va =
        H[H_MAPNPC].va = H[H_PSCLR].va = H[H_GATE].va = H[H_SREADY].va = H[H_STINT].va = H[H_TSFADE].va = H[H_FILL].va =
        H[H_WNEW].va = H[H_WFREE].va = H[H_EFREE].va = H[H_AFREE].va = H[H_CFREE].va = H[H_CBLK].va = H[H_WLEAD].va =
        H[H_CSNAP].va = H[H_ZRESET].va = H[H_QSTATE].va = H[H_VREC].va = H[H_RSAVE].va = H[H_MFREE].va = H[H_MDEL].va =
        H[H_PHDEL].va = H[H_FORM].va = H[H_FOLLOW].va = H[H_ASTATE].va = H[H_PTINT].va = H[H_MLOAD].va =
        H[H_SFREE].va = H[H_SFREEALL].va = H[H_SGRADED].va = H[H_PSWAP].va = H[H_POPULATE].va = H[H_SINIT].va = H[H_SETH].va = H[H_DINIT].va = H[H_SNDFREE].va = 0;
    if (strcmp(g_ed->tag, "enold")) for (int i = H_CD_SKFIND; i <= H_SLOOP; i++) H[i].va = 0;
    if (!O.hooks) return;
    char *b = strdup(O.hooks);   /* (a fixed 4 KB buffer silently dropped the tail of a long spec) */
    if (!b) rh_fatal("hook spec: out of memory");
    for (char *t = strtok(b, ","); t; t = strtok(NULL, ",")) {
        char *c = strchr(t, ':'); if (!c) rh_fatal("bad hook spec %s", t);
        *c = 0; const char *mode = t; char *name = c + 1;
        uint32_t every = 1; char *at = strchr(name, '@'); if (at) { *at = 0; every = strtoul(at + 1, 0, 0); }
        hook_t *h = NULL;
        for (int i = 0; i < H_N; i++) if (!strcmp(H[i].name, name)) h = &H[i];
        if (!h) rh_fatal("unknown hook %s", name);
        if (!h->va) { rh_log("hook %s: no VA for edition %s — skipped\n", name, g_ed->tag); continue; }
        h->mode = !strcmp(mode, "shadow") ? 1 : !strcmp(mode, "replace") ? 2 : 0;
        h->every = every;
        if (!h->mode) continue;
        if ((h == &H[H_RLOOP] || h == &H[H_REPOLL] || h == &H[H_LROOM] || h == &H[H_SLOOP]) && h->mode == 1)
            rh_fatal("%s: replace only (it runs a room / repolls DirectInput)", h->name);
        if (h == &H[H_MAP_DECODE]) {
            if (h->mode == 2) rh_fatal("map_decode: shadow only (the port decode needs the recorded resource)");
            if (!g_mload_sites) { rh_log("hook map_decode: parse recorder %d call sites\n", redirect_calls(0x587970u, w_mload)); g_mload_sites = 1; }
        }
        if (h == &H[H_MLOAD] && g_mload_sites) { rh_log("hook %s:%s: sites already on the recorder\n", mode, name); continue; }
        if (h == &H[H_MLOAD]) g_mload_sites = 1;
        int n = redirect_calls_chain(h->va, h->wrapper);
        rh_log("hook %s:%s @%08x every=%u: %d call sites\n", mode, name, h->va, every, n);
        if (!n) rh_fatal("hook %s: no direct call sites", name);
    }
}

void rh_hooks_report(void)
{
    for (int i = 0; i < H_N; i++) {
        hook_t *h = &H[i];
        if (!h->mode) continue;
        if (h->mode == 1)
            rh_log("hook-result shadow %s calls=%ld diverge=%ld skipped=%ld compared=%ld%s%s\n", h->name, h->calls, h->diverge,
                   h->skipped, h->compared, h->diverge ? " first: " : "", h->diverge ? h->first : "");
        else rh_log("hook-result replace %s calls=%ld\n", h->name, h->calls);
    }
    if (H[H_CHAR_INT].mode == 1)
        rh_log("char_integrate: %ld calls delegated (special input pose), %ld entered an unported pose mid-call\n",
               g_int_special, g_integrate_unported);
    if (H[H_BEHAVE].mode == 1)
        for (int k = 0; k < g_nbh_codes; k++) rh_log("char_behave coverage: code %d compared %ld\n", g_bh_code_id[k], g_bh_codes[k]);
    if (H[H_PINIT].mode == 1)
        for (int k = 0; k < g_npinit_codes; k++) rh_log("particle_init coverage: code %d compared %ld\n", g_pinit_code_id[k], g_pinit_codes[k]);
    if (H[H_GSTEP].mode == 1) rh_log("gimmick_step: %ld live (switching) calls shadowed\n", g_gstep_live);
    if (H[H_DINIT].mode == 1)
        for (int k = 0; k < g_ndcodes; k++) rh_log("device_init coverage: code %d compared %ld\n", g_dcode_id[k], g_dcodes[k]);
    if (H[H_CINIT].mode == 1)
        for (int k = 0; k < g_nccodes; k++) rh_log("char_init coverage: code %d compared %ld\n", g_ccode_id[k], g_ccodes[k]);
    if (H[H_DIK].mode == 1) rh_log("input_dik_produce: %ld keyboard events shadowed\n", g_dik_events);
    if (H[H_ZPASS].mode == 1) rh_log("zone_pass: %ld calls with a zone under the leader shadowed\n", g_zone_live);
    if (H[H_PARTICLE].mode == 1)
        for (int k = 0; k < g_nprt_codes; k++) rh_log("particle_tick coverage: code %d compared %ld\n", g_prt_code_id[k], g_prt_codes[k]);
    if (H[H_ACTOR].mode == 1)
        for (int k = 0; k < g_nactor_codes; k++) rh_log("actor_update coverage: code %d calls %ld\n", g_actor_code_id[k], g_actor_codes[k]);
    if (H[H_CHAR_PHYS].mode == 1)
        rh_log("char_physics coverage: state0..6+ %ld %ld %ld %ld %ld %ld %ld %ld | ctl0..3+ %ld %ld %ld %ld %ld | mode0..3+ %ld %ld %ld %ld %ld | probe %ld\n",
               g_phys_cov[0], g_phys_cov[1], g_phys_cov[2], g_phys_cov[3], g_phys_cov[4], g_phys_cov[5], g_phys_cov[6], g_phys_cov[7],
               g_phys_ctl[0], g_phys_ctl[1], g_phys_ctl[2], g_phys_ctl[3], g_phys_ctl[4],
               g_phys_mode[0], g_phys_mode[1], g_phys_mode[2], g_phys_mode[3], g_phys_mode[4], g_phys_probe);
}
