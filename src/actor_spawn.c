/*
 * actor_spawn.c — see actor_spawn.h.  The town CHARACTER-band spawn: the
 * 0x58d460 -> 0x431e30 slice that turns the map's 70000-range object
 * layers into renderable {actor, render-state} pairs, plus the PORT-DEBT
 * visible-code sprite-table stand-in (the lazy 0x40afe0/0x41e600 fill).
 */
#include "actor_spawn.h"
#include "butterfly.h"      /* butterfly_register — the per-tick LCG behaviour   */
#include "character.h"      /* CHAR_POSE_DOWN/UP — the U/D-pose clip selector    */
#include "party.h"          /* the dramatist resolve (handle -> code/bank)       */
#include "rng.h"            /* rng_rand — the engine LCG the spawn replays      */

#include <string.h>

/* Layer-header field reads (little-endian, off the 0x3c-byte map_layer.hdr).
 * Offsets from docs/proofs/map-object-layer-format.md (0x58d460's reads). */
static uint32_t hdr_u32(const uint8_t *h, int off)
{
    return (uint32_t)h[off] | ((uint32_t)h[off + 1] << 8) |
           ((uint32_t)h[off + 2] << 16) | ((uint32_t)h[off + 3] << 24);
}
static int32_t hdr_i32(const uint8_t *h, int off) { return (int32_t)hdr_u32(h, off); }

#define HDR_OFF_X       0x04   /* i32 tile-px x (spawn -> world x = x*100)      */
#define HDR_OFF_Y       0x08   /* i32 tile-px y (spawn -> world y = y*100)      */
#define HDR_OFF_CODE    0x10   /* u32 type code (= actor+0x1d4 behaviour)       */
#define HDR_OFF_VARIANT 0x18   /* u16 variant -> STRUCTURE frame_base (0x58d460:269) */
#define HDR_OFF_FGFLAG  0x30   /* i32 foreground flag -> STRUCTURE layer 15 vs 8     */

static uint16_t hdr_u16(const uint8_t *h, int off)
{
    return (uint16_t)((uint16_t)h[off] | ((uint16_t)h[off + 1] << 8));
}

/* PORT-DEBT(actor-sprite-table): the captured visible-code sprite map (retail
 * town hold, flip 1500 — live +0x48 capture via 0x491ae0).  Stands in for the
 * lazy def-table fill keyed by type+state.  Only codes that DRAW appear here;
 * any other CHARACTER code is an invisible volume (bank 0). */
static const struct {
    uint32_t code;
    uint16_t bank;
    int16_t  frame_base;
    uint32_t layer;
} TOWN_SPRITE_DEFS[] = {
    /* bank 0x16c (res 0x403) is the town-OBJECTS sheet — these are static PROPS
     * (e.g. a barrel, a fountain), NOT people-NPCs (USER-confirmed on the feed:
     * the fountain @0x112e5/176000 + a barrel @0x1129e).  The only person in the
     * CHARACTER band is the animated protagonist 0x1872d (bank 0x175, separate). */
    {0x1129eu, 0x16cu,  1u, 9u},   /* prop, frame 1  (x3 in DATA 1022)   */
    {0x1129fu, 0x16cu,  2u, 9u},   /* prop, frame 2  (x1)                */
    {0x112e5u, 0x16cu, 36u, 10u},  /* prop (the fountain), frame 36, layer 10 (x1) */
};

int actor_spawn_sprite_for_code(uint32_t code, uint16_t *bank,
                                int16_t *frame_base, uint32_t *layer)
{
    for (size_t i = 0; i < sizeof TOWN_SPRITE_DEFS / sizeof TOWN_SPRITE_DEFS[0]; i++) {
        if (TOWN_SPRITE_DEFS[i].code == code) {
            if (bank)       *bank       = TOWN_SPRITE_DEFS[i].bank;
            if (frame_base) *frame_base = TOWN_SPRITE_DEFS[i].frame_base;
            if (layer)      *layer      = TOWN_SPRITE_DEFS[i].layer;
            return 1;
        }
    }
    return 0;
}

int actor_spawn_from_map(actor_spawn_pool *pool, const map_data *md)
{
    if (pool == NULL || md == NULL) return -1;
    memset(pool, 0, sizeof *pool);
    if (md->layers == NULL) return 0;

    for (uint32_t li = 0; li < md->count; li++) {
        const uint8_t *h = md->layers[li].hdr;
        uint32_t code = hdr_u32(h, HDR_OFF_CODE);
        if (code < ACTOR_CODE_CHARACTER_LO || code > ACTOR_CODE_CHARACTER_HI)
            continue;                         /* not a CHARACTER object */

        if (pool->count >= ACTOR_BAND_SLOTS)  /* "Character Object Count Over" */
            return -1;

        int slot = pool->count++;
        actor              *a  = &pool->actors[slot];
        actor_render_state *rs = &pool->states[slot];

        /* 0x431e30 defaults: active, behaviour code, draw layer 9, dir 0,
         * the +0x48 table zeroed (already, via memset).  World = (x,y)*100. */
        a->code  = code;
        a->dir   = 0;
        a->layer = 9;        /* actor+0xfc */

        rs->active  = 1;     /* render-state +0x00 (the *param_1!='\0' gate) */
        rs->world_x = hdr_i32(h, HDR_OFF_X) * 100;   /* +0x04 */
        rs->world_y = hdr_i32(h, HDR_OFF_Y) * 100;   /* +0x08 */
        rs->clip    = NULL;  /* +0x6c — static (32/32 char actors, clip 0)  */

        /* The visible codes get their dir-0 sprite row (+ the draw layer the
         * lazy fill set); every other code stays an invisible volume (bank 0,
         * FUN_0044d160 returns 0). */
        uint16_t bank; int16_t frame_base; uint32_t layer;
        if (actor_spawn_sprite_for_code(code, &bank, &frame_base, &layer)) {
            a->sprite_table[0].bank       = bank;
            a->sprite_table[0].frame_base = frame_base;
            a->layer                      = layer;
        }
    }
    return pool->count;
}

/* STRUCTURE code -> sprite bank, RE'd from the activator 0x438a60's per-case
 * switch (each case ends 0x426db0/0x426d70/(via LAB_00439490)
 * 0x426d70(0, BANK, variant)).  This IS the def table (not a capture), so it
 * is not PORT-DEBT.  DATA-1022 uses only 0xec55/0xec60/0xec6a; the rest are the
 * other town-scenery structure codes for any room.  (0xec59..0xec5e have no
 * case.)  0xeead's bank is the runtime u16 *(0x8a9b50+0x27a4), written by the
 * 587e00 prologue per room[0x43]: 0x88/0x89/0x8a for param_4 5/6/8
 * (587e00.c:215-236) — mirrored here by actor_spawn_struct_set_runtime_bank
 * (0 = unset -> the code resolves to no sprite, like an off-table code). */
static uint16_t g_struct_eead_bank = 0;

void actor_spawn_struct_set_runtime_bank(uint16_t bank)
{
    g_struct_eead_bank = bank;
}

static const struct { uint32_t code; uint16_t bank; } STRUCT_BANK_DEFS[] = {
    {0xec55u, 0x15fu},  /* the foreground TREE          (via 0x426db0)        */
    {0xec56u, 0x160u},  /*                              (via 0x426db0)        */
    {0xec57u, 0x161u},
    {0xec58u, 0x162u},
    {0xec5fu, 0x163u},
    {0xec60u, 0x164u},  /* the fg hedges / flowerbed (res 0x426)               */
    {0xec61u, 0x165u},
    {0xec62u, 0x166u},
    {0xec69u, 0x16bu},
    {0xec6au, 0x16cu},  /* the bg decorations (res 0x403, town-objects sheet)  */
    {0xec6bu, 0x16eu},
    {0xec6cu, 0x16fu},
    {0xec6du, 0x16du},
    {0xec6eu, 0x170u},
    {0xec6fu, 0x170u},
    {0xec70u, 0x196u},
    {0xec7cu, 0x18du},
    {0xf295u, 0x77u},   /* 438a60.c:24-27 (via LAB_00439490)                   */
};

int actor_spawn_struct_bank_for_code(uint32_t code, uint16_t *bank)
{
    if (code == 0xeeadu) {              /* 438a60.c:19-22 — the runtime bank */
        if (g_struct_eead_bank == 0)
            return 0;
        if (bank) *bank = g_struct_eead_bank;
        return 1;
    }
    for (size_t i = 0; i < sizeof STRUCT_BANK_DEFS / sizeof STRUCT_BANK_DEFS[0]; i++) {
        if (STRUCT_BANK_DEFS[i].code == code) {
            if (bank) *bank = STRUCT_BANK_DEFS[i].bank;
            return 1;
        }
    }
    return 0;
}

int actor_spawn_struct_from_map(actor_spawn_pool *pool, const map_data *md)
{
    if (pool == NULL || md == NULL) return -1;
    memset(pool, 0, sizeof *pool);
    if (md->layers == NULL) return 0;

    for (uint32_t li = 0; li < md->count; li++) {
        const uint8_t *h = md->layers[li].hdr;
        uint32_t code = hdr_u32(h, HDR_OFF_CODE);
        if (code < ACTOR_CODE_STRUCTURE_LO || code > ACTOR_CODE_STRUCTURE_HI)
            continue;                          /* not a STRUCTURE object */

        uint16_t bank;
        if (!actor_spawn_struct_bank_for_code(code, &bank))
            continue;                          /* 0x438a60 switch default: no sprite */

        if (pool->count >= ACTOR_BAND_SLOTS)   /* "Structure Object Count Over" */
            return -1;

        int slot = pool->count++;
        actor              *a  = &pool->actors[slot];
        actor_render_state *rs = &pool->states[slot];

        /* 0x438a60: behaviour code (+0x1d4), dir 0 (+0xe8), draw layer (+0xfc) =
         * the foreground flag (record +0x30): 1 => 15 (in front of the cast),
         * else 8.  Sprite row 0 = {bank from the def table, frame_base = the
         * record's variant @ +0x18}, x/y offset 0 (the fill primitives pass 0). */
        a->code  = code;
        a->dir   = 0;
        a->layer = (hdr_i32(h, HDR_OFF_FGFLAG) == 1) ? 15u : 8u;
        a->sprite_table[0].bank       = bank;
        a->sprite_table[0].frame_base = (int16_t)hdr_u16(h, HDR_OFF_VARIANT);

        rs->active  = 1;                        /* +0x00 (the *param_1!='\0' gate) */
        rs->world_x = hdr_i32(h, HDR_OFF_X) * 100;   /* +0x04 */
        rs->world_y = hdr_i32(h, HDR_OFF_Y) * 100;   /* +0x08 */
        rs->clip    = NULL;                     /* +0x6c — static (39/39, clip 0) */
    }
    return pool->count;
}

/* PORT-DEBT(effect-sprite-table): the EFFECT townsfolk code -> (bank, dst, layer,
 * facing, flip) map, captured from the retail town-hold census (the 0x493ba0
 * field spec at flips 1450/1500/1600 — code, row0 bank, rs +0x40/+0x44 dst, +0xfc
 * layer, +0x2c facing).  Stands in for 0x41f200's per-type install switch.  All
 * the standing town villagers share the idle clip 0x6290e0 (decoded: base 0, 20
 * frames, dur 14, loop) and draw layer 13; 0xe2a5 has its own anchor.  Banks
 * resolve to the villager sheets res 0x459/0x462/0x46a/0x46b/0x472/0x47b/0x47f
 * (FUN_00417c40).  Now INCLUDES the 4 butterflies (0xe29a, bank 0x146, clip
 * BUTTERFLY_CLIP, layer 12 — corrected ckpt 96 from the "wandering villagers"
 * mis-ID); excludes only 0xc35a/0xc3dc/0xc3f0 (non-map party/script spawns).
 *
 * facing (rs +0x2c): 1 normal / 3 mirrored.  Set by the dispatcher 0x58d460:96
 *   cVar12 = (puVar1[4] != 0) ? 3 : 1 from the MAP sub-record puVar1[4] (NOT RNG;
 *   built by the unported 0x587e00 decoder, so captured here, not yet map-parsed),
 *   forwarded as param_8 to 0x41f200.  FUN_0044d160 mirrors the cel + reflects
 *   off_x when facing==3.
 * flip: the mirror frame offset = *(int16)(DAT_008a8440[bank]) = the sprite group's
 *   frames-per-direction (read live from retail; mirrored cel = frame_base + flip).
 *   Used by actor_render_describe only on the facing==3 arm. */
static const struct {
    uint32_t code;
    uint16_t bank;
    int16_t  dstx, dsty;
    uint32_t layer;
    int16_t  facing;   /* rs +0x2c: 1 normal / 3 mirrored                       */
    int16_t  flip;     /* DAT_008a8440[bank] first short (frames/dir) for ==3   */
} TOWN_EFFECT_DEFS[] = {
    {0xc3beu, 0x0d4u, -30, -24, 13u, 3, 16},
    {0xc3ddu, 0x0e1u, -30, -20, 13u, 3,  4},
    {0xc3e6u, 0x0e5u, -30, -32, 13u, 3,  4},
    {0xc3f2u, 0x0f0u, -30, -20, 13u, 1,  4},
    {0xc404u, 0x0f9u, -30, -20, 13u, 1,  4},
    {0xc422u, 0x093u, -30, -24, 13u, 3, 16},
    {0xc42cu, 0x099u, -30, -24, 13u, 3, 16},
    {0xc440u, 0x0a6u, -30, -20, 13u, 1, 16},
    {0xc441u, 0x0a9u, -30, -20, 13u, 3, 16},
    {0xc468u, 0x0d0u, -30, -20, 13u, 3,  4},
    {0xe2a5u, 0x14cu, -16, -32, 13u, 1, 16},
    /* The 4 BUTTERFLIES (was mis-ID'd as "wandering villagers" through ckpt 95).
     * bank 0x146 -> sprite res 0x3fa (slot 313, 32x32, already group3-registered),
     * dst (0,0), draw layer 12, flap clip BUTTERFLY_CLIP (selected by code in the
     * spawn).  flip 16 = DAT_008a8440[0x146], the MIRROR-cel offset (live-read
     * ckpt 139): the facing==3 render adds +16 (frames 16-31 = the left-facing
     * mirror cels) AND reflects off_x.  So cel = frame_base + 16*(facing==3) + flap
     * where frame_base is the per-instance BASE DIRECTION from the map variant
     * (set below).  facing toggles 1/3 via the heading FSM (butterfly.c).
     * (was flip 4 = the wrong per-dir stride; see findings/butterfly-direction-sprite.md.) */
    {0xe29au, 0x146u,   0,   0, 12u, 1, 16},
};

int actor_spawn_effect_def_for_code(uint32_t code, uint16_t *bank,
                                    int16_t *dstx, int16_t *dsty, uint32_t *layer,
                                    int16_t *facing, int16_t *flip)
{
    for (size_t i = 0; i < sizeof TOWN_EFFECT_DEFS / sizeof TOWN_EFFECT_DEFS[0]; i++) {
        if (TOWN_EFFECT_DEFS[i].code == code) {
            if (bank)   *bank   = TOWN_EFFECT_DEFS[i].bank;
            if (dstx)   *dstx   = TOWN_EFFECT_DEFS[i].dstx;
            if (dsty)   *dsty   = TOWN_EFFECT_DEFS[i].dsty;
            if (layer)  *layer  = TOWN_EFFECT_DEFS[i].layer;
            if (facing) *facing = TOWN_EFFECT_DEFS[i].facing;
            if (flip)   *flip   = TOWN_EFFECT_DEFS[i].flip;
            return 1;
        }
    }
    return 0;
}

/* Fill a bank-indexed mirror/flip table (the port stand-in for retail's global
 * DAT_008a8440) from the town EFFECT defs: table[bank] = the sprite group's
 * frames-per-direction, which FUN_0044d160 adds to the frame on the facing==3
 * arm to pick the mirrored cel.  Only the town villager banks are filled (the
 * only mirrored actors in the scene); all other banks stay 0 (no mirror).
 * Returns the number of entries written. */
int actor_spawn_effect_fill_flip_table(int16_t *table, size_t n)
{
    if (table == NULL) return 0;
    int written = 0;
    for (size_t i = 0; i < sizeof TOWN_EFFECT_DEFS / sizeof TOWN_EFFECT_DEFS[0]; i++) {
        uint16_t bank = TOWN_EFFECT_DEFS[i].bank;
        if (bank < n) { table[bank] = TOWN_EFFECT_DEFS[i].flip; written++; }
    }
    return written;
}

/* The town villagers' shared idle clip — reconstructed from DAT_006290e0 (the
 * clip 0x426fd0 installs for the standing EFFECT townsfolk; decoded from the
 * user's sotes.exe .rdata for analysis): base 0, 20 frames, 14 sim-ticks/frame,
 * LOOPING, per-frame sprite delta {0,1,2,1,0,...} (a slow breathing/sway cycle),
 * zero per-frame offset.  RE'd timing/indexing metadata (the 0x154-B descriptor),
 * not the asset — the sprite PIXELS load from the user's file at runtime.  At
 * spawn, 0x426ec0 randomizes the START phase into this clip (frame in
 * [0,frame_count), timer in [0,frame_dur)); the per-sim-tick stepper
 * (actor_pool_update -> anim_clip_advance) then runs the breathing loop. */
static const anim_clip IDLE_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1, 2, 1, 0, 1, 2, 1, 0, 1,
                     2, 3, 0, 1, 2, 1, 0, 3, 2, 3 },
    .frame_count = 20,
    .frame_dur   = 14,
    .oneshot     = 0,    /* loops */
};

/* The town BUTTERFLY's wing-flap clip — reconstructed from DAT_0065ddf0 (the clip
 * the 0xe29a actor carries, decoded from the user's sotes.exe .rdata): base 0,
 * 3 frames, 4 sim-ticks/frame, LOOPING, per-frame sprite delta {0,1,2}, zero
 * per-frame offset.  A fast 3-cel wing flap (vs the villagers' slow 20-frame
 * breathe).  CORRECTS the long-standing mis-ID: every prior checkpoint called the
 * 4 town 0xe29a EFFECT objects "wandering villagers", but the live settled-town
 * blit + emit census (runs/butterfly-{blits,emit}, retail flips 2028/2138) proves
 * they render sprite res 0x3fa (bank 0x146, slot 313, 32x32) via 0x493ba0 at
 * layer 12 — they are the small yellow + white BUTTERFLIES that flit by the
 * flowerbeds (USER-pinpointed: over the dark wood beam, below the ARMS sign,
 * above the dog).  Like the villagers, 0x426ec0 randomizes the start phase into
 * this clip at spawn (frame in [0,3), timer in [0,4)). */
static const anim_clip BUTTERFLY_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1, 2 },
    .frame_count = 3,
    .frame_dur   = 4,
    .oneshot     = 0,    /* loops */
};

/* The fixed per-object RNG draw count 0x41f200 consumes BEFORE its 0x426ec0
 * idle-phase pair, by EFFECT type code (engine-quirk #86, the seed-pinned
 * 0x5bf505 census cross-checked against the decompile):
 *   0x426fd0 (1, the +0xf4 init) + 0x41f200 prologue (7 = 2 position-jitter
 *   :294/:301 + 5 particle-param :326-334) = 8, PLUS the per-type-switch draw:
 *     0xe29a -> 0x427670 case 2 (5 draws; the wandering BUTTERFLIES, :2181)
 *     0xe2a5 -> 0x431cb0     (1 draw; :2272)
 *     all other town effects -> none.
 * (The conditional 0x41f200:2849 draw + the script effects 0xc35a/0xc3dc/0xc3f0
 * fall AFTER all 11 rendered townsfolk in the spawn order, so they do not affect
 * the townsfolk idle phases and are not modelled here.) */
static int effect_prefix_draws(uint32_t code)
{
    int n = 8;                       /* 0x426fd0 (1) + 0x41f200 prologue (7) */
    if (code == 0xe29au) n += 5;     /* 0x427670 case 2 (the butterflies)    */
    else if (code == 0xe2a5u) n += 1;/* 0x431cb0                             */
    return n;
}

int actor_spawn_effect_from_map(actor_spawn_pool *pool, const map_data *md,
                                struct butterfly_pool *bp)
{
    if (pool == NULL || md == NULL) return -1;
    memset(pool, 0, sizeof *pool);
    if (md->layers == NULL) return 0;

    for (uint32_t li = 0; li < md->count; li++) {
        const uint8_t *h = md->layers[li].hdr;
        uint32_t code = hdr_u32(h, HDR_OFF_CODE);
        if (code < ACTOR_CODE_EFFECT_LO || code > ACTOR_CODE_EFFECT_HI)
            continue;                          /* not an EFFECT object: no spawn RNG */

        /* RNG REPLAY (engine-quirk #86): EVERY map EFFECT object runs 0x41f200's
         * per-object spawn-draw burst in this (map-layer = 0x58d460 dispatch)
         * order, so the shared LCG must advance identically here for the idle
         * PHASE to land 1:1 with retail under the game_enter re-seed (ckpt 86).
         * The prefix draws feed position jitter / particle params the port does
         * not model (the townsfolk positions are map-driven; the fountain is a
         * later chip), so they are consumed-to-advance; only the 0x426ec0 pair
         * is USED — for the rendered townsfolk AND the butterflies (0xe29a); the
         * remaining unknown codes still consume their draws, just not spawned.
         * For a butterfly the LAST prefix draw is the 5th of 0x427670 case 2 =
         * its move-frequency 0xc874 (engine-quirk #95) — captured for the per-tick
         * behaviour butterfly_step. */
        uint32_t last_prefix = 0;
        for (int k = effect_prefix_draws(code); k > 0; k--)
            last_prefix = rng_rand();
        uint16_t wander_freq = (uint16_t)(((last_prefix * 100u) >> 15) + BUTTERFLY_FREQ_BASE);
        /* The actor's anim clip: the butterfly (0xe29a) flaps the 3-frame
         * BUTTERFLY_CLIP; every other town effect breathes the 20-frame IDLE_CLIP.
         * Selected by code BEFORE the phase draws so 0x426ec0 scales the start
         * phase by THIS clip's count/dur — the draw COUNT is 2 either way, so the
         * shared LCG stream stays aligned regardless of which clip is chosen. */
        const anim_clip *clip = (code == 0xe29au) ? &BUTTERFLY_CLIP : &IDLE_CLIP;
        /* 0x426ec0: the idle PHASE.  frame = (rand * clip.frame_count) >> 15,
         * then timer = (rand * clip.frame_dur) >> 15 (the >>15 is /32768; both
         * operands are small + non-negative so it matches retail's signed form).
         * Every town effect carries a clip, so both draws always fire. */
        uint16_t ph_frame = (uint16_t)((rng_rand() * clip->frame_count) >> 15);
        uint16_t ph_timer = (uint16_t)((rng_rand() * clip->frame_dur)   >> 15);

        uint16_t bank; int16_t dstx, dsty; uint32_t layer; int16_t facing, flip;
        if (!actor_spawn_effect_def_for_code(code, &bank, &dstx, &dsty, &layer,
                                             &facing, &flip))
            continue;        /* unmapped effect code: draws consumed, not spawned */
        (void)flip;                            /* lands in the render flip table */

        if (pool->count >= ACTOR_BAND_SLOTS)   /* "Effect Object Count Over" */
            return -1;

        int slot = pool->count++;
        actor              *a  = &pool->actors[slot];
        actor_render_state *rs = &pool->states[slot];

        /* 0x41f200: behaviour code (+0x1d4), dir 0 (+0xe8), draw layer (+0xfc).
         * Sprite row 0 = {bank, frame_base}; the per-frame sprite comes from the
         * clip via the anim stepper.  World = (map (x,y) - dst) * 100; the render
         * dst anchor lives in the render-state (+0x40/+0x44), added back at emit.
         *
         * frame_base: the standing townsfolk hardcode 0 (their install is
         * 0x426d70(0,bank,0)), but the BUTTERFLY (0xe29a) takes it from the map
         * record's VARIANT field (+0x18) — its per-instance BASE DIRECTION (0/4/8/12).
         * Its install is 0x426d70(0,0x146,param_7), param_7 = *(u16)(record+0x18)
         * (dispatcher 0x58d460:151).  Live-verified (findings/butterfly-direction-sprite.md):
         * the rendered cel = frame_base + 16*(facing==3) + flap, so the 4 butterflies
         * each fly a different base pose, mirrored left/right by the facing toggle. */
        a->code  = code;
        a->dir   = 0;
        a->layer = layer;
        a->sprite_table[0].bank       = bank;
        a->sprite_table[0].frame_base =
            (code == 0xe29au) ? (int16_t)hdr_u16(h, HDR_OFF_VARIANT) : 0;

        rs->active     = 1;                     /* +0x00 (the *param_1!='\0' gate) */
        rs->world_x    = (hdr_i32(h, HDR_OFF_X) - dstx) * 100;   /* +0x04 */
        rs->world_y    = (hdr_i32(h, HDR_OFF_Y) - dsty) * 100;   /* +0x08 */
        rs->facing     = facing;                /* +0x2c — 1 normal / 3 mirrored */
        rs->dst_base_x = dstx;                  /* +0x40 — the render anchor */
        rs->dst_base_y = dsty;                  /* +0x44 */
        rs->clip       = clip;                  /* +0x6c — idle breathe / butterfly flap */
        rs->timer      = ph_timer;              /* +0x70 — 0x426ec0 start timer */
        rs->frame      = ph_frame;              /* +0x72 — 0x426ec0 start frame */
        rs->done       = 0;                     /* +0x74 */

        /* Register the butterfly's per-tick behaviour (the EFFECT band's only
         * per-tick LCG consumer) with its captured move frequency, its spawn
         * worldX (-> the patrol bounds), and this actor's slot (so the per-tick
         * motion drives THIS render-state's worldX/facing), IN MAP ORDER. */
        if (code == 0xe29au && bp != NULL)
            butterfly_register(bp, wander_freq, rs->world_x, rs->world_y, slot);
    }
    return pool->count;
}

/* The town-intro cutscene 0x4d7d80 (case 0x334be) spawns the arriving family IN
 * FRONT of the wagon, anchor-relative to it (anchor 0x65) via three 0x41f0e0
 * calls (-> the EFFECT activator 0x41f200).  RE'd verbatim from the decompile
 * (docs/proofs/dramatist-table.md, the 0x41f0e0 arg shape):
 *
 *   0x41f0e0(0,          0xc3f0, 0x65, 0x6400,     0, 3, 0, 0);  // Dr. Barnard (by code)
 *   0x41f0e0(0x5f5e1d3,  0,      0x65, 8000,       0, 3, 0, 0);  // Arche's Father (handle)
 *   0x41f0e0(0x5f5e1d4,  0,      0x65, 0xfffff380, 0, 1, 0, 0);  // Arche's Mother (handle)
 *
 * Each member's CODE + sprite BANK is resolved through the dramatist table
 * DAT_006b6ea8 + the archetype default-bank arm (src/party.c, the 0x41f200:54-69
 * + per-case logic) — NOT a frozen census snapshot:
 *   - Dr. Barnard: handle 0 -> archetype 0xc3f0 default (facing_sel 0 -> base
 *     bank 0xeb).  Renders (0xeb registered in group3).
 *   - Arche's Father: handle 0x5f5e1d3 -> code 0xc3dc, bank OVERRIDE 0xe3.  Renders.
 *   - Arche's Mother: handle 0x5f5e1d4 -> code 0xc440 (the "Woman" archetype),
 *     bank OVERRIDE 0xb5 (NOT the generic map townswoman's facing default 0xa6).
 *     This is the ckpt-92 fix: the port now spawns Mom's OWN 0xb5 sheet, which is
 *     registered in group3 (idx 168), so she renders as the woman the USER sees
 *     on the golden — instead of being absent.
 *
 * Arche the party LEADER (handle 0x5f5e165 -> code 0xc35a) is ALSO rendered here
 * (ckpt 94).  In retail she is the persistent leader (room_state+0x200c), created
 * at new-game and drawn by the party band 0x4997b0; but at the settled town she
 * renders through the SAME 0x493ba0 path as the rest of the cast (live census
 * runs/cutscene-cast: one 0x493ba0 call, row0 bank 0x8b, clip 0x62a8c8, facing 1,
 * pos 41600/45600).  Her only blocker was bank registration: her body banks
 * 0x8b-0x8e are EXE-embedded (res 0x570-0x573 in sotes.exe, see
 * ar_register_party_exe_sprites), unregistered until ckpt 94.  Her dramatist bank
 * is 0, so party_resolve_spawn yields 0 -> the bank_override 0x8b (her 0x41f200
 * case's row-0 install).  PORT-DEBT(cutscene-party-chars): she is the static-cast
 * member, not yet the party-band leader; the multi-part banks 0x8c-0x8e + the
 * walk-in roll-in + the live-actor handle registry (dialogue) remain Phase 2/3.
 *
 * Positions: world x = the wagon's settled anchor (CUTSCENE_WAGON_ANCHOR_X) +
 * each member's anchor-relative x offset (the RE'd 0x41f0e0 arg4); this
 * reproduces the census settled positions exactly (Barnard 67200 / Father 49600
 * / Mother 38400) AND ports the real offsets.  PORT-DEBT(cutscene-party-chars):
 * the wagon-anchor BASE is the settled value (the roll-in is deferred with the
 * wagon clip), and the walk-in DIALOGUE movement (the family animates to these
 * spots) is Phase 3 — these are the settled hold positions. */
#define CUTSCENE_WAGON_ANCHOR_X 41600   /* wagon anchor 0x65 settled world_x      */

static const struct {
    uint32_t handle;        /* 0x41f0e0 arg1 (0 => spawned by code)              */
    uint32_t code_in;       /* 0x41f0e0 arg2 (0 => take the dramatist row's code) */
    int16_t  facing;        /* 0x41f0e0 arg6 -> rs +0x2c (1 normal / 3 mirrored) */
    int16_t  facing_sel;    /* 0x41f0e0 arg8 -> param_11 (archetype default sel) */
    int16_t  flip;          /* DAT_008a8440[resolved bank] for the facing==3 cel */
    uint16_t bank_override;  /* non-0 => use this bank, NOT party_resolve_spawn's
                              * (for the party LEADER Arche, whose dramatist bank
                              * is 0 but whose 0x41f200 case installs body 0x8b)  */
    int32_t  x_off;         /* 0x41f0e0 arg4 -> anchor-relative world x          */
    int32_t  world_y;       /* rs +0x08 settled world_y (census)                 */
    int16_t  dst_y;         /* rs +0x44 render anchor y (census)                 */
    uint16_t clip_frame;    /* rs +0x72: idle-clip start phase (census, cosmetic) */
    uint8_t  spawn_draws;   /* the LCG draws this member's 0x41f200 spawn consumes
                             * (engine-quirk #94, the seed-pinned 0x5bf505 census).
                             * The town room-load draws a 19-object EFFECT burst:
                             * the 15 MAP objects (effect_from_map, 171 draws) THEN
                             * these 4 SCRIPT objects (0x4d7d80 -> 0x41f0e0), each a
                             * 0x426fd0(1)+0x41f200(7)+0x426ec0(2)=10-draw shape.
                             * ARCHE (0xc35a) draws 12: her 0x41f200 case 0xc35a is
                             * the only one that calls 0x427360 (+1) and trips the
                             * conditional 0x41f200:25caa (+1).  In retail she spawns
                             * FIRST of the four (the obj16 12-draw block in the
                             * census); the port consumes in render-array order since
                             * only the TOTAL (42) is load-bearing — it advances the
                             * shared LCG to the post-spawn phase so the establishing
                             * REVEAL's iris-variant draw lands on retail's value. */
} CUTSCENE_FAMILY[] = {
    {0,          0xc3f0u, 3, 0, 4, 0,     0x6400,  43600, -20,  0, 10},  /* Dr. Barnard    -> 0xeb */
    {0x5f5e1d3u, 0,       3, 0, 4, 0,     8000,    43600, -20, 13, 10},  /* Arche's Father -> 0xe3 */
    {0x5f5e1d4u, 0,       1, 0, 0, 0,     -3200,   43600, -20,  1, 10},  /* Arche's Mother -> 0xb5 */
    /* Arche the party LEADER: handle 0x5f5e165 -> code 0xc35a, dramatist bank 0
     * (party-loaded), so party_resolve_spawn yields bank 0 -> override to her body
     * bank 0x8b (the 0x41f200 case 0xc35a :899 install).  Her banks 0x8b-0x8e now
     * register from the EXE (ar_register_party_exe_sprites), so 0x8b decodes and
     * she renders.  Settled census pos: x 41600 (= the wagon anchor, x_off 0),
     * y 45600, dst (-30,-24), facing 1, clip 0x62a8c8 (byte-identical to the idle
     * clip).  PORT-DEBT(cutscene-party-chars): rendered as the static-cast member
     * here (not yet via the party band 0x4997b0); the multi-part body banks
     * 0x8c-0x8e + the walk-in roll-in remain Phase 2/3. */
    {0x5f5e165u, 0,       1, 0, 0, 0x8bu, 0,       45600, -24, 15, 12},  /* Arche -> body 0x8b (0x427360, 12 draws) */
};

/* Append the cutscene arrival family to an already-filled EFFECT pool (g_effects,
 * after actor_spawn_effect_from_map).  Does NOT memset — it extends the pool.
 * Resolves each member's (code, bank) through the dramatist system (party.c) and
 * spawns it anchor-relative to the wagon.  Also writes each facing==3 member's
 * mirror/flip value into `flip_table` (the port stand-in for DAT_008a8440) so the
 * mirrored cel resolves; pass the same table actor_spawn_effect_fill_flip_table
 * filled (NULL to skip).  Returns the number spawned (-1 on bad arg). */
int actor_spawn_cutscene_cast(actor_spawn_pool *pool, int16_t *flip_table, size_t flip_n)
{
    if (pool == NULL) return -1;
    int spawned = 0;
    for (size_t i = 0; i < sizeof CUTSCENE_FAMILY / sizeof CUTSCENE_FAMILY[0]; i++) {
        /* RNG REPLAY (engine-quirk #94): each cutscene member's 0x41f200 spawn
         * consumes its draws on the room-load tick, in the 19-object EFFECT burst
         * AFTER the 15 map objects (actor_spawn_effect_from_map) and BEFORE the
         * establishing REVEAL's iris-variant draw.  Consume them here — like
         * effect_from_map's prefix replay — so the shared LCG reaches retail's
         * post-spawn phase (the values feed each member's idle phase/position,
         * which the port takes from the settled census, so they are consumed-to-
         * advance).  Done before the pool-full guard: retail always spawns all 4
         * (32 EFFECT slots), and the iris depends on the full 42-draw advance. */
        for (int k = CUTSCENE_FAMILY[i].spawn_draws; k > 0; k--)
            (void)rng_rand();

        if (pool->count >= ACTOR_BAND_SLOTS) return spawned;   /* pool full */

        /* 0x41f200:54-69 + the archetype default arm: handle/code -> (code, bank). */
        uint32_t code; uint16_t bank;
        party_resolve_spawn(CUTSCENE_FAMILY[i].handle, CUTSCENE_FAMILY[i].code_in,
                            CUTSCENE_FAMILY[i].facing_sel, &code, &bank);
        /* The party leader Arche resolves to bank 0 (party-loaded sheet); her
         * 0x41f200 case installs body bank 0x8b explicitly -> the override. */
        if (CUTSCENE_FAMILY[i].bank_override != 0)
            bank = CUTSCENE_FAMILY[i].bank_override;

        int slot = pool->count++;
        actor              *a  = &pool->actors[slot];
        actor_render_state *rs = &pool->states[slot];

        a->code  = code;
        a->dir   = 0;
        a->layer = 13u;                         /* EFFECT townsfolk layer */
        a->sprite_table[0].bank       = bank;
        a->sprite_table[0].frame_base = 0;

        rs->active     = 1;
        rs->world_x    = CUTSCENE_WAGON_ANCHOR_X + CUTSCENE_FAMILY[i].x_off;
        rs->world_y    = CUTSCENE_FAMILY[i].world_y;
        rs->facing     = CUTSCENE_FAMILY[i].facing;
        rs->dst_base_x = -30;                   /* the town villager render anchor */
        rs->dst_base_y = CUTSCENE_FAMILY[i].dst_y;
        rs->clip       = &IDLE_CLIP;            /* breathe like the townsfolk (clip 0x6290e0) */
        rs->timer      = 0;
        rs->frame      = CUTSCENE_FAMILY[i].clip_frame;
        rs->done       = 0;

        if (flip_table != NULL && CUTSCENE_FAMILY[i].facing == 3 &&
            bank < flip_n)
            flip_table[bank] = CUTSCENE_FAMILY[i].flip;
        spawned++;
    }
    return spawned;
}

/* ── THEME 2: Arche's RUN-OFF clips + motion (the "Arche runs to the house"
 * beat).  See actor_spawn.h for the RE provenance.  The clips are RE'd cel-
 * sequence + per-frame-duration metadata read off retail.osr's Arche draw stream
 * (res 0x570, draw_probe.py ticks 980-1130) — NOT the binary asset (bank 0x8b
 * pixels load from the user's sotes.exe). */

/* RUN: cels 16,16,17,18,19,19,20,21 (the contact frames 16/19 held 2x), 5 sim-
 * ticks/clip-frame, looping — matches retail tick-for-tick (cel 16 for 10t, 17/18
 * 5t each, 19 10t, 20/21 5t; a 40-tick cycle). */
static const anim_clip ARCHE_RUN_CLIP = {
    .base_sprite = 16,
    .frame_delta = { 0, 0, 1, 2, 3, 3, 4, 5 },
    .frame_count = 8,
    .frame_dur   = 5,
    .oneshot     = 0,    /* loops */
};

/* DECEL: cels 8,9,10,11 as she slows to a stop, 6 sim-ticks/cel, ONE-SHOT (holds
 * on cel 11 = the planted-foot pose).  Measured ~6t/cel over retail ticks
 * ~1040-1064.  The brief 12,0,6,159 transition flourish between decel and the
 * idle is folded into the decel->idle clip switch (a ~11t cosmetic detail,
 * PORT-DEBT(cutscene-party-chars) refinement). */
static const anim_clip ARCHE_DECEL_CLIP = {
    .base_sprite = 8,
    .frame_delta = { 0, 1, 2, 3 },
    .frame_count = 4,
    .frame_dur   = 6,
    .oneshot     = 1,    /* play once, hold on cel 11 */
};

/* ARRIVAL IDLE: cels 152,153,154 (a gentle breathe at the house door), 14 sim-
 * ticks/cel, looping — measured from retail ticks ~1095+ (the 152/153/154 loop). */
static const anim_clip ARCHE_ARRIVAL_IDLE_CLIP = {
    .base_sprite = 152,
    .frame_delta = { 0, 1, 2, 1 },
    .frame_count = 4,
    .frame_dur   = 14,
    .oneshot     = 0,    /* loops */
};

/* ── The house Arche TURN (USER studio notes #3-5).  The town-intro script emote
 * 0x401e60(Arche,1) at 0x4d7d80:1170 fires AFTER house L5 ("...I'll be countin' on
 * you, Arche") advances and BEFORE L6 ("I will, I promise!"): it sets the actor's
 * command kind 2 = "turn to face dir 1" (0x43e5b0 case 2).  Off retail.osr res
 * 0x570 (Arche, static at screen (354,336) through the house) ticks 1579-1587 her
 * cel runs 158(4t) → 7(4t) → the standing idle 0/1/2 (a base-0 14t breathe),
 * turning from her arrival-listening idle (152-155) to face her father.  The port
 * plays this as a ONE-SHOT clip on the room-cast Arche (HOUSE_CAST[0], bank 0x8b),
 * fired by CS_ACT_ACTOR_TURN (cutscene.c) and swapped to the standing idle by
 * main.c when the one-shot finishes (rs->done).  This is RE'd cel-sequence metadata
 * read off the retail draw stream (the same treatment as ARCHE_RUN_CLIP, ckpt 140 —
 * the live actor-turn FSM 0x43e5b0 is PORT-DEBT(cutscene-party-chars)). */
static const anim_clip ARCHE_HOUSE_TURN_CLIP = {
    .base_sprite = 158,
    .frame_delta = { 0, -151 },   /* cels 158 -> 7 (158 - 151 = 7) */
    .frame_count = 2,
    .frame_dur   = 4,
    .oneshot     = 1,             /* play once, hold on cel 7 until the idle swap */
};

/* The house Arche post-turn STANDING idle: a base-0 breathe 0,1,2,1 at 14t/cel
 * (retail.osr res 0x570 ticks 1587+: fr 0/1/2 loop) — the pose she holds facing her
 * father for the rest of the house scene (same shape as the arrival idle, base 0). */
static const anim_clip ARCHE_HOUSE_STAND_IDLE_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1, 2, 1 },
    .frame_count = 4,
    .frame_dur   = 14,
    .oneshot     = 0,             /* loops */
};

const anim_clip *arche_house_turn_clip(void)      { return &ARCHE_HOUSE_TURN_CLIP; }
const anim_clip *arche_house_turn_idle_clip(void) { return &ARCHE_HOUSE_STAND_IDLE_CLIP; }

/* The run physics consts (= the REAL ported char-run law, char.h CHAR_RUN_*; ckpt
 * 118, validated bit-exact).  Duplicated here (not #included) to keep actor_spawn
 * decoupled from character.c — the values are the same captured per-entity tuning. */
#define ARCHE_RUN_CAP        48000   /* = CHAR_RUN_CAP   |vel| cap -> world_x += 480/tick */
#define ARCHE_RUN_ACCEL1      3200   /* = CHAR_RUN_ACCEL (phase 1, while |vel| < walk cap) */
#define ARCHE_RUN_ACCEL2      1600   /* = CHAR_WALK_ACCEL (phase 2, up to the run cap)     */
#define ARCHE_RUN_WALKCAP    24000   /* = CHAR_WALK_CAP (the two-phase accel knee)         */
/* The decel-APPROACH (distance trigger + linear ramp-down to stop AT the target)
 * is the PORT-DEBT(cutscene-party-chars) stand-in for the unported mover 0x54f980:
 * decel over ~the decel clip length (24t) so she plants at the house door. */
#define ARCHE_RUNOFF_DECEL_DIST 6000
#define ARCHE_RUNOFF_DECEL_MINV 2000 /* don't asymptote — finish the last approach */

void arche_runoff_begin(arche_runoff *st, int32_t start_x, int32_t target_x)
{
    if (st == NULL) return;
    st->active   = 1;
    st->phase    = ARCHE_RUNOFF_RUN;
    st->world_x  = start_x;
    st->vel      = 0;
    st->target_x = target_x;
    st->windup   = ARCHE_RUNOFF_WINDUP_TICKS;   /* the run CELS lag the motion this much */
}

const anim_clip *arche_runoff_step(arche_runoff *st)
{
    if (st == NULL || !st->active) return NULL;
    if (st->phase == ARCHE_RUNOFF_ARRIVED)
        return &ARCHE_ARRIVAL_IDLE_CLIP;

    int32_t dist = st->target_x - st->world_x;
    if (dist < 0) dist = 0;

    if (dist <= ARCHE_RUNOFF_DECEL_DIST) {
        /* DECEL approach: vel proportional to remaining distance -> stop at target
         * (the mover-approach stand-in; the run accel/cruise above are real). */
        st->phase = ARCHE_RUNOFF_DECEL;
        st->vel   = (ARCHE_RUN_CAP * dist) / ARCHE_RUNOFF_DECEL_DIST;
        if (st->vel < ARCHE_RUNOFF_DECEL_MINV) st->vel = ARCHE_RUNOFF_DECEL_MINV;
    } else {
        /* RUN: the ported two-phase accel up to the cap. */
        st->phase = ARCHE_RUNOFF_RUN;
        st->vel += (st->vel < ARCHE_RUN_WALKCAP) ? ARCHE_RUN_ACCEL1 : ARCHE_RUN_ACCEL2;
        if (st->vel > ARCHE_RUN_CAP) st->vel = ARCHE_RUN_CAP;
    }

    st->world_x += st->vel / 100;
    if (st->world_x >= st->target_x) {
        st->world_x = st->target_x;
        st->phase   = ARCHE_RUNOFF_ARRIVED;
        return &ARCHE_ARRIVAL_IDLE_CLIP;
    }
    /* The run CELS lag the motion by the windup (she accelerates while retail plays the
     * lean cels; the port shows idle until then — ARCHE_RUNOFF_WINDUP_TICKS, cast-emote
     * debt).  NULL = the caller keeps her current (idle) clip while world_x advances. */
    if (st->windup > 0) { st->windup--; return NULL; }
    return (st->phase == ARCHE_RUNOFF_DECEL) ? &ARCHE_DECEL_CLIP : &ARCHE_RUN_CLIP;
}

/* The caravan's idle clip — reconstructed from &DAT_00671c48 (the clip pointer
 * the 0x431e30 case-0x1872d arm installs; read from the user's sotes.exe .rdata
 * for analysis): base_sprite 2, 4 frames, 18 sim-ticks/frame, LOOPING, per-frame
 * delta {0,1,2,3} so the animated body cel cycles sprite frames 2..5 — the HORSES
 * (USER-confirmed), zero per-frame offset.  This is RE'd timing/indexing metadata
 * (4 shorts), not the binary asset — the sprite PIXELS (bank 0x175) load from the
 * user's file at runtime.  The looping clip is now driven once per sim-tick by
 * actor_pool_update -> actor_anim_advance (the 0x46cd70/0x54f980 stepper), so
 * the body cel cycles sprite frames 2..5 — the horses TROT.  PORT-DEBT
 * (actor-protagonist-clip) narrows to the remaining halves: the RNG-driven
 * behaviour (idle/wander, deferred ckpt 73) and the cutscene's anchor-relative
 * roll-in (the spawn pos is still the settled census const, not 0x431d10's
 * anchor 0x65 / x 0x3200 arrival path). */
static const anim_clip WAGON_CLIP = {
    .base_sprite = 2,
    .frame_delta = { 0, 1, 2, 3 },
    .frame_count = 4,
    .frame_dur   = 18,
    .oneshot     = 0,    /* loops */
};

int actor_spawn_protagonist(actor_spawn_pool *pool, int32_t world_x, int32_t world_y)
{
    if (pool == NULL) return -1;
    if (pool->count >= ACTOR_BAND_SLOTS) return -1;

    int slot = pool->count++;
    actor              *a  = &pool->actors[slot];
    actor_render_state *rs = &pool->states[slot];

    /* 0x431e30 case-0x1872d end state (see actor_spawn.h). */
    a->code  = ACTOR_CODE_PROTAGONIST;   /* +0x1d4 — the 0x491ae0 dispatch key */
    a->dir   = 0;                        /* +0xe8                              */
    a->layer = 9;                        /* +0xfc (in_ECX[0x3f] = 9)           */

    /* 0x426db0(0, 0x175, 0, 1, 0, 0, 0): sprite-table row 0 only. */
    a->sprite_table[0].bank       = (uint16_t)ACTOR_PROT_SPRITE_BANK;
    a->sprite_table[0].frame_base = 0;
    a->sprite_table[0].x_off      = 0;
    a->sprite_table[0].y_off      = 0;
    a->sprite_table[0].mirror_x   = 0;

    rs->active  = 1;                     /* +0x00 */
    rs->world_x = world_x;               /* +0x04 */
    rs->world_y = world_y;               /* +0x08 */
    rs->facing  = ACTOR_PROT_FACING;     /* +0x2c = 99 (not 3 -> not mirrored) */
    rs->clip    = &WAGON_CLIP;           /* +0x6c — the wagon's HORSES animation */
    rs->timer   = 0;                     /* +0x70 — clip-set resets the cycle    */
    rs->frame   = 0;                     /* +0x72 — starts on frame 0 (sprite 2) */
    rs->done    = 0;                     /* +0x74 */
    return slot;
}

int actor_pool_update(actor_spawn_pool *pool)
{
    if (pool == NULL) return 0;
    int advanced = 0;
    /* 0x46cd70:123-169 — walk the active main-band slots; 0x54f980's frame
     * stepper runs on each.  We advance only render-states with a clip (the
     * stepper short-circuits on clip==0 anyway), so the 32 static actors no-op
     * and only the protagonist's horses trot. */
    for (int i = 0; i < pool->count; i++) {
        actor_render_state *rs = &pool->states[i];
        if (rs->active == 0 || rs->clip == NULL)
            continue;
        actor_anim_advance(rs);
        advanced++;
    }
    return advanced;
}

/* ── The per-room CAST (the house/errands family).  In retail the town-arrival
 * family — Arche + her parents — are PERSISTENT entities (the leader
 * room_state+0x200c et al.) carried across the LIGHT room swaps (quirk #103) and
 * repositioned by the cutscene-script actor movers (0x402730/0x402330).  The port
 * renders them as static-cast EFFECT actors at the room's CAPTURED positions + idle
 * clips — the same treatment as the town arrival cast (CUTSCENE_FAMILY) and the
 * townsfolk (TOWN_EFFECT_DEFS).  PORT-DEBT(cutscene-party-chars): the captured
 * positions/clips stand in for the live actor movers (Phase 3).
 *
 * The banks (Arche 0x8b / Father 0xe3 / Mother 0xb5) and their mirror-flip table
 * entries are registered ONCE at game start (ar_register_* + actor_spawn_cutscene_cast)
 * and PERSIST across the map reloads (town_render_free frees only the map scene, not
 * the asset-register sprite slots), so the room cast reuses them directly — the SAME
 * sprites the town renders 1:1 (retail draws the house parents as res 0x467/0x473 and
 * Arche as 0x570, identical to the town arrival cast).
 *
 * World positions: derived from the town arrival cast's 1:1 projection (the port
 * draws the town cast pixel-identical to retail, so its world→screen is ground
 * truth) — 1 screen px = 100 world units, the house settled camera cur=(89600,3200)
 * (room_camera_origin).  Each member keeps its town (dst_base, facing); world_x/y
 * solve the projection to land at retail's house screen positions (retail.osr
 * draw_probe tick 1340: Arche res 0x570 @354,336 / Father res 0x467 @386,320 /
 * Mother res 0x473 @418,320).  Idle clips: Arche the arrival-idle (cels 152-154,
 * her house breathe), the parents the 20-frame breathe (IDLE_CLIP) at frame_base 4
 * (retail house frames 4-7 vs the town's 0-3 — a different idle pose set). */
struct room_cast_member {
    uint16_t  bank;          /* sprite bank (persisted from the town cast)        */
    uint16_t  frame_base;    /* +0x4a sprite-row frame_base                       */
    int32_t   world_x;       /* rs +0x04                                          */
    int32_t   world_y;       /* rs +0x08                                          */
    int16_t   dst_base_x;    /* rs +0x40                                          */
    int16_t   dst_base_y;    /* rs +0x44                                          */
    int16_t   facing;        /* rs +0x2c (1 normal / 3 mirrored)                  */
    const anim_clip *clip;   /* rs +0x6c idle clip                                */
    uint16_t  clip_phase;    /* rs +0x72 idle start frame (cosmetic phase)        */
    uint16_t  layer;         /* a +0xfc draw layer; 0 => the default cast layer   *
                              * 13.  BACKGROUND furniture that the structure-band  *
                              * props sit IN FRONT of (the bookshelf back panel)   *
                              * needs a layer BELOW the structure-bg layer 8 (the  *
                              * draw pool walks layers in index order, lower=behind*
                              * — quirk: ERRANDS_CAST at 13 drew OVER the layer-8   *
                              * props, occluding them; USER "bookshelf missing      *
                              * props").  Retail draws the frame (seq 261) then the *
                              * props (268+).                                       */
    int32_t   alpha;         /* a +0xf4 node_alpha.  0 => opaque colorkey blit (the *
                              * default for every cast member).  Non-zero => an      *
                              * ADDITIVE alpha blit: the value is the ramp_a index   *
                              * (PARTICLE_PARAM8, low byte) the present resolves to   *
                              * g_ramp_a[idx] (mode 1).  The fireplace FIRE is the    *
                              * only errands member that needs it (ramp_a[14], the    *
                              * weight-750 additive glow — see FIRE_CLIP).            */
};

static const struct room_cast_member HOUSE_CAST[] = {
    /* The dramatist banks: 0x8b->Arche (res 0x570), 0xe3->Father (res 0x473=1139),
     * 0xb5->Mother (res 0x467=1127) — confirmed off the live registry (Father is the
     * RIGHTMOST, Mother middle, Arche left).  facing 1 (no mirror) + the parents'
     * frame_base 4 land the idle frames 4-7 retail draws (their 8-frame house pose
     * set; f_38=8 so the cel resolves).  dst_base = the town cast anchor (-30,-20)
     * for the parents, (-30,-24) for Arche.  World positions solve the town cast's
     * 1:1 projection (1 px = 100 world; house cam 89600/3200) to retail's house
     * screen positions (draw_probe tick 1340: Arche @354,336 / Mother @386,320 /
     * Father @418,320).  NOTE the solved world positions land EXACTLY on the
     * cutscene.c house speaker positions (Arche 128000/39200, Mother 131200/37200,
     * Father 134400/37200) — the box-anchor RE captured the same entity world coords
     * (ckpt 132), an independent cross-check that these are the real cast positions. */
    /* bank   fb  world_x  world_y  dbx  dby fac clip                      phase lyr alpha member */
    { 0x8bu,  0,  128000,  39200,  -30, -24, 1, &ARCHE_ARRIVAL_IDLE_CLIP,   0,   0,  0 }, /* Arche  res 0x570 @354,336 */
    { 0xb5u,  4,  131200,  37200,  -30, -20, 1, &IDLE_CLIP,                 2,   0,  0 }, /* Mother res 0x467 @386,320 */
    { 0xe3u,  4,  134400,  37200,  -30, -20, 1, &IDLE_CLIP,                 1,   0,  0 }, /* Father res 0x473 @418,320 */
};

/* The ERRANDS room (0x334dc) FAMILY — Father + Mother (the persistent parents) in
 * their item shop, while Arche (the freeroam player, g_freeroam) runs errands.
 * Same banks as the house (0xe3/0xb5 — persistent), captured world positions solved
 * from the house cast's projection (errands cam 0/16000): retail draw_probe tick
 * 2200 (freeroam) — Father res 0x473 @screen 480,320 -> world (51000,50000), Mother
 * res 0x467 @screen 624,128 -> world (65400,30800, on the upper level, mostly off
 * the right edge).  The shop NPCs (res 1027, ~10 instances) are the broader
 * PORT-DEBT(errands-cast)/actor-sprite-table, not modelled here. */
/* The errands-shop fireplace FIRE (USER osr_notes #3 "missing fire in fireplace",
 * tick 1726, crop [309,154,96,89]).  RE'd off retail.osr (findings/errands-render-
 * gaps.md §1 + the ckpt-147 trace-the-code pass):
 *   - sprite bank 0x1a3 (=419, the resolve POOL index = group3 slot 406 + RAMP_COUNT+1
 *     (=13), ar_pool_get_slot's offset; slot 406 = PE res 0x40a=1034, 64x64, asset_
 *     register.c row {406,0x040a,...}; registered at boot group 3, decoded lazily),
 *   - frames 0..5 LOOPING, a SINGLE uniform per-clip frame_dur — the retail clip
 *     format (0x154-byte descriptor +0x44) carries one duration for every frame,
 *     NOT a per-frame list (anim_clip.h).  The .osr clean (non-coalesced) ticks
 *     bracket it: fr0 1700-1705, fr1 1726-1731, fr4 1744-1749 — six sim-ticks each,
 *     a 36-tick loop.  (The 1736-1739 "gap" is retail flip-coalescing, not a short
 *     frame — quirk #99 — so this is the data duration, not a flip-axis measurement.)
 *   - ADDITIVE alpha: the .osr blit is primitive=alpha, bmode=1, st=0x8000 (KEYSRC),
 *     ckey=0xf81f, and its blend descriptor (blend_ref 36, CONSTANT across all six
 *     frames + every tick) is byte-IDENTICAL to the port's g_ramp_a[14] (mode 1,
 *     weight 750) — proven by a host LUT compare of all 20 ramp_a entries vs the
 *     descriptor extracted from retail.osr (the only FULL match).  So alpha = 14.
 * Drawn at a CONSTANT screen dst (329,178) 48x39 (the recess behind the grate at the
 * base of the brick chimney).  The proper spawn is an un-RE'd map EFFECT (the lazy
 * def-table fill 0x41e600, PORT-DEBT(errands-cast)/actor-sprite-table); captured here
 * as an additive-alpha room-cast member, the same stand-in as the family/shop props
 * above.  World pos = the errands projection (cam 0/16000, 1px=100): (329*100,
 * (178+160)*100) = (32900,33800); dst_base fitted off the port osr. */
static const anim_clip FIRE_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1, 2, 3, 4, 5 },
    .frame_count = 6,
    .frame_dur   = 6,
    .oneshot     = 0,    /* loops */
};

/* The steaming COOKING POT (res1026, variant base 56) — the steam wisps loop the
 * 4 cels 57..60 at 6 sim-ticks/frame (RE'd off retail-stairs res1026 near the pot
 * @clamp: fr 57(6t)->58->59->60->57, a forward loop; cel = frame_base(56)+delta). */
static const anim_clip POT_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 1, 2, 3, 4 },
    .frame_count = 4,
    .frame_dur   = 6,
    .oneshot     = 0,    /* loops */
};

/* The pendulum CLOCK (res1026, variant base 43) — a slow SWING 43<->45 through the
 * centre 44, 25 sim-ticks/frame (RE'd off retail-stairs res1026 @clock: 45(25t)->44
 * ->43->44->45, the swing delta {0,1,2,1}; cel = frame_base(43)+delta). */
static const anim_clip CLOCK_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1, 2, 1 },
    .frame_count = 4,
    .frame_dur   = 25,
    .oneshot     = 0,    /* loops */
};

static const struct room_cast_member ERRANDS_CAST[] = {
    /* The shop FURNITURE — the bookshelf frame, the counter in front of Dad, the
     * wall clock (USER studio notes #8/#9/#18/#19/#20/#21: "still missing" post-
     * reveal).  These are CHARACTER-band objects (DATA 1025 layer codes 0x112cf
     * clock / 0x112d1 bookshelf / 0x112d2 counter, in the 0x111xx-0x112xx / 70000
     * range, NOT structure codes) that resolve to bank 0x16f (res 1023, the
     * house/shop furniture sheet) with frame_base = the layer's VARIANT (+0x18) —
     * confirmed off retail.osr: res 1023 fr 0/2/3/4/6/13 (+ res 1026/1022 props) at the
     * projected positions.  The
     * port's CHARACTER band (g_actors) is SUPPRESSED for non-town rooms, so these
     * never spawn (they'd be invisible volumes even if spawned — the codes aren't in
     * TOWN_SPRITE_DEFS).  Captured here as static room-cast members (the same stand-in
     * as the shop props below).  World = the map layer pos (X*100, Y*100); the room-
     * cast projection (cam 0/16000) lands them at retail's screen positions.
     * PORT-DEBT(errands-cast): the proper fix is to spawn the errands CHARACTER band
     * with frame_base = variant + the visible-furniture code->bank table. */
    /* bank   fb  world_x  world_y  dbx dby fac clip  phase lyr alpha (map code @screen) */
    { 0x16fu,  3,    8000,  44800,    0,  0, 1, NULL,  0, 7, 0 }, /* bookshelf 0x112d1 res1023 fr3 @80,288 — LAYER 7 (behind its layer-8 props) */
    { 0x16fu,  4,   70400,  25600,    0,  0, 1, NULL,  0, 7, 0 }, /* kitchen CABINET 0x112d1 (map layer[18] 704,256) res1023 fr4 @ref 704,96 — the RIGHT-side furniture the static tick-2200 capture missed (USER mark t2278); LAYER 7 (behind its props, retail seq 223 pre-structure) */
    { 0x16fu,  2,   70400,   6400,    0,  0, 1, NULL,  0, 7, 0 }, /* upstairs HUTCH  0x112d1 (map layer[31] 704,64)  res1023 fr2 @ref 704,-96 (mostly off the top edge — the "with dishes, upstairs" piece); LAYER 7 */
    { 0x16fu,  0,   53200,  25600,    0,  0, 1, NULL,  0, 0, 0 }, /* wall shelf 0x112cf res1023 fr0 @532,96 (above the clock) */
    { 0x16bu, 43,   52800,  24800,    0,  0, 1, &CLOCK_CLIP, 0, 0, 0 }, /* pendulum clock 0x112d9 res1026 var43 @528,88 — SWINGS 43<->45 (CLOCK_CLIP) */
    /* More RIGHT-side / upstairs props ERRANDS_CAST originally missed — off-screen in the
     * static tick-2200 capture, revealed once the camera pans right (USER: "the pot is
     * missing, right next to mom's head, to the right of her").  All DATA-1025 CHARACTER
     * objects; res->bank = asset_register slot + 13; world = map layer pos x100 (validated
     * vs retail at the camera clamp t2500: screen == retail exactly).  frame_base = the
     * retail draw's frame (the layer variant).  LAYER 13 (default) = the foreground prop
     * band (retail seq 282-288, over the structure + the layer-7 cabinet). */
    { 0x16bu, 56,   67600,  29600,    0,  0, 1, &POT_CLIP, 0, 0, 0 }, /* the POT 0x112da (map L27 676,296) res1026 var56 @228,208 — right of Mom's head (USER mark); STEAMS 57..60 (POT_CLIP) */
    { 0x16bu, 38,   56000,   8800,    0,  0, 1, NULL,  0, 0, 0 }, /* upstairs prop 0x11279 (map L34 560,88) res1026 fr38 @112,0 */
    { 0x16fu, 13,   60000,   6400,    0,  0, 1, NULL,  0, 0, 0 }, /* upstairs furniture 0x112d3 (map L36 600,64) res1023 fr13 @152,-24 */
    { 0x156u,  4,   83200,  12800,    0,  0, 1, NULL,  0, 0, 0 }, /* upstairs prop 0x1124c (map L97 832,128) res1022 fr4 @384,40 */
    /* bank   fb  world_x  world_y  dbx dby fac clip        phase lyr alpha */
    { 0x1a3u,  0,   32900,  33800,   -9,-18, 1, &FIRE_CLIP, 0, 0, 14 }, /* fireplace FIRE res1034, additive ramp_a[14] @329,178 (dst_base -9,-18 = the 64x64 cel's pivot, fitted; see FIRE_CLIP) */
    /* The shop PROPS/NPCs — bank 0x16c (res 0x403=1027, the prop sheet, 80 frames),
     * STATIC (clip NULL, identical across ticks): the 10 instances retail draws that
     * the port's struct band doesn't (the codes aren't in the struct-bank table —
     * PORT-DEBT(errands-cast)/actor-sprite-table; captured here as static room-cast
     * members instead, the same approach as TOWN_EFFECT_DEFS).  World = the captured
     * retail screen (tick 2200) back through the errands projection (cam 0/16000,
     * 1 px = 100 world); facing 1 + the cel pivot folded into dst_base (fitted). */
    /* bank   fb  world_x  world_y  dbx  dby fac clip  phase   res-1027 frame @screen */
    { 0x16cu,  4,  50000,  32000,    0,   0, 1, NULL,  0, 0, 0 }, /* fr4  @500,160 */
    { 0x16cu,  5,  63200,  32000,    0,   0, 1, NULL,  0, 0, 0 }, /* fr5  @632,160 */
    { 0x16cu,  8,  38400,  12800,    0,   0, 1, NULL,  0, 0, 0 }, /* fr8  @384,-32 */
    { 0x16cu,  9,  32000,  51200,    0,   0, 1, NULL,  0, 7, 0 }, /* fr9  @320,352 — LAYER 7 (shelf unit, behind its layer-8 props) */
    { 0x16cu,  9,  38400,  51200,    0,   0, 1, NULL,  0, 7, 0 }, /* fr9  @384,352 — LAYER 7 (shelf unit, behind its layer-8 props) */
    { 0x16cu, 11,  52800,  12800,    0,   0, 1, NULL,  0, 0, 0 }, /* fr11 @528,-32 */
    { 0x16cu, 14,  45200,  12800,    0,   0, 1, NULL,  0, 0, 0 }, /* fr14 @452,-32 */
    { 0x16cu, 44,  34400,  41600,    0,   0, 1, NULL,  0, 0, 0 }, /* fr44 @344,256 */
    { 0x16cu, 44,  56000,  22400,    0,   0, 1, NULL,  0, 0, 0 }, /* fr44 @560,64  */
    { 0x16cu, 64,  25600,  51200,    0,   0, 1, NULL,  0, 0, 0 }, /* fr64 @256,352 */
    /* The FAMILY (people) are spawned LAST so they render IN FRONT of the shop props
     * (both are layer 13 -> draw order = array order).  RE'd off retail seq at t2500:
     * every res1027/1026 prop draws (252-286) THEN Mom (289) — she is frontmost.  The
     * old order (family BEFORE the shop props) drew a chair (res1027 fr5 @184,232, the
     * 0x112a2 shop prop) OVER Mom (USER mark).  The counter stays AFTER Father (drawn IN
     * FRONT of him).  bank fb world_x world_y dbx dby fac clip phase lyr alpha */
    { 0xe3u,  4,   51000,  50000,  -30, -20, 1, &IDLE_CLIP,   1, 0, 0 }, /* Father res 0x473 @480,320 */
    { 0x16fu,  6,   45600,  44800,    0,   0, 1, NULL,        0, 0, 0 }, /* counter 0x112d2 res1023 fr6 @456,288 (IN FRONT of Father) */
    { 0xb5u,  0,   65400,  30800,  -30, -20, 1, &IDLE_CLIP,   2, 0, 0 }, /* Mother res 0x467 @624,128 */
};

int actor_spawn_room_cast(actor_spawn_pool *pool, uint32_t room_key)
{
    if (pool == NULL) return -1;
    pool->count = 0;
    const struct room_cast_member *cast = NULL;
    size_t n = 0;
    switch (room_key) {
    case 0x334c8u:  /* CUTSCENE_ROOM_HOUSE */
        cast = HOUSE_CAST; n = sizeof HOUSE_CAST / sizeof HOUSE_CAST[0];
        break;
    case 0x334dcu:  /* CUTSCENE_ROOM_ERRANDS — the family (shop NPCs are debt) */
        cast = ERRANDS_CAST; n = sizeof ERRANDS_CAST / sizeof ERRANDS_CAST[0];
        break;
    default:
        return 0;   /* no port-modelled cast for this room */
    }
    for (size_t i = 0; i < n && pool->count < ACTOR_BAND_SLOTS; i++) {
        int slot = pool->count++;
        actor              *a  = &pool->actors[slot];
        actor_render_state *rs = &pool->states[slot];
        memset(a, 0, sizeof *a);
        memset(rs, 0, sizeof *rs);
        a->code  = 0;
        a->dir   = 0;
        a->layer = cast[i].layer ? cast[i].layer : 13u;  /* 0 => the EFFECT cast layer 13 */
        a->node_alpha = cast[i].alpha;     /* +0xf4: 0 opaque, else additive ramp_a idx */
        a->sprite_table[0].bank       = cast[i].bank;
        a->sprite_table[0].frame_base = cast[i].frame_base;
        rs->active     = 1;
        rs->world_x    = cast[i].world_x;
        rs->world_y    = cast[i].world_y;
        rs->facing     = cast[i].facing;
        rs->dst_base_x = cast[i].dst_base_x;
        rs->dst_base_y = cast[i].dst_base_y;
        rs->clip       = cast[i].clip;
        rs->timer      = 0;
        rs->frame      = cast[i].clip_phase;
        rs->done       = 0;
    }
    return (int)pool->count;
}

/* ── Phase 2b: the FREEROAM (controllable Arche) animation clips.  RE'd off the
 * retail freeroam-walk capture (runs/freeroam-walk, the body 0x5b8fc0/code 0xc35a
 * celfr sequence under held-axis injection): in the errands room (the first
 * freeroam) Arche's body bank 0x8b plays —
 *   WALK : cels 0,1,2,3 (facing RIGHT; the LEFT-facing cels 4-7 come from the
 *          facing==3 mirror via flip_table[0x8b]=4) — a 4-cel cycle.
 *   IDLE : cels 0,1 (the slow standing breathe — retail.osr errands idle, res
 *          0x570 fr 0/1 at the post-dialogue hold).
 * The RUN (dash) reuses ARCHE_RUN_CLIP (cels 16-21, the run-off clip).  The walk-
 * cel cadence is time-based (dur 5) here; retail advances it with DISTANCE (the
 * cel speeds up with velocity) — PORT-DEBT(char-walk-anim-distance), refine when
 * the distance-locked cycle is pinned. */
/* RE'd off retail-walk.osr (res 0x570, held RIGHT then idle; ckpt 153b): the freeroam
 * WALK is cels 8-15 (an 8-cel cycle, dur 6) and the IDLE is cels 0,1,2 (a 3-cel
 * breathe, dur 14) — NOT the ckpt-144 stand-ins (walk 0-3 / idle 0-1), which were
 * wrong.  (The ckpt-144 "+4 walk-flip verified 1:1" did not scrutinise the exact
 * cels.)  The walk-cel cadence is still time-based (dur 6); retail advances it with
 * DISTANCE (PORT-DEBT(char-walk-anim-distance)). */
static const anim_clip ARCHE_WALK_CLIP = {
    .base_sprite = 8,
    .frame_delta = { 0, 1, 2, 3, 4, 5, 6, 7 },   /* cels 8-15 */
    .frame_count = 8,
    .frame_dur   = 6,
    .oneshot     = 0,    /* loops 8->15->8 */
};
static const anim_clip ARCHE_FREEROAM_IDLE_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1, 2 },   /* cels 0,1,2 */
    .frame_count = 3,
    .frame_dur   = 14,
    .oneshot     = 0,    /* loops */
};

/* The STANDING TURN-AROUND clip (char-turn-state, ckpt 177): the freeroam pivot
 * cels fr 6/7 on bank 0x8b — the SAME turn poses the house cutscene turn plays
 * (ARCHE_HOUSE_TURN_CLIP renders their mirrors 158 -> 7).  frame 0 = fr 6 (the
 * windup, held facing the OLD dir), frame 1 = fr 7 (held facing the NEW dir); the
 * renderer's +152 facing mirror makes BOTH reversal directions emerge — R->L
 * renders 6 -> 159 (7+152), L->R renders 158 (6+152) -> 7 — matching retail
 * (retail-stairs res 0x570: fr 6 x4 -> 159 x4).  The FRAME is forced each tick from
 * character_turn_frame() (main.c freeroam_step), NOT the clip timer, so the sim
 * windup/flip and the rendered cel stay locked; frame_dur/oneshot are for
 * completeness (the walk clip resumes when turn_frame drops to -1). */
const anim_clip ARCHE_TURN_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 6, 7 },
    .frame_count = 2,
    .frame_dur   = CHAR_TURN_HOLD,
    .oneshot     = 1,
};

/* The flip-table value bank 0x8b needs for the facing==3 (LEFT) mirror.  Bank 0x8b
 * is mirrored with a UNIFORM offset of +152 (RE'd off retail-walk.osr / poseL: the
 * left cels are the right cels + 152 across EVERY freeroam animation — idle 0-2 ->
 * 152-154, walk 8-15 -> 160-167, run 16-21 -> 168-173, crouch 31/32 -> 183/184, up
 * 34/35 -> 186/187).  So the renderer's facing==3 mirror adds 152 and the dedicated
 * left cels emerge — no per-animation left clips needed.  (The ckpt-144 value 4 was
 * wrong; it rendered left walk 4-7 etc. — engine-quirk #114.) */
const int16_t ARCHE_FREEROAM_FLIP = 152;

/* The flip-table value bank 0x8c (sword-OUT, res 0x571) needs for the facing==3
 * (LEFT) mirror.  RE'd off sword2.osr (`/tmp/sword_cels.py`): the sword-out left
 * cels are the right cels + 192 across EVERY action — idle 0-2 -> 192-194, walk
 * 8-15 -> 200-207, run 16-21 -> 208-213, crouch 31/32 -> 223/224.  (The sword-IN
 * bank 0x8b uses +152; 0x8c is a separate, wider sprite group, hence +192.) */
const int16_t ARCHE_SWORD_OUT_FLIP = 192;

const anim_clip *arche_freeroam_clip(int moving, int airborne, int run)
{
    /* Airborne reuses the run/idle pose for now (no RE'd jump-anim cel set yet —
     * PORT-DEBT(char-jump-anim)); position rides the mover's world_y arc. */
    if (run && moving)
        return &ARCHE_RUN_CLIP;
    if (moving)
        return &ARCHE_WALK_CLIP;
    (void)airborne;
    return &ARCHE_FREEROAM_IDLE_CLIP;
}

/* ── The U/D-POSE animation clips (crouch / up-defensive) — RE'd off the retail
 * draw stream (retail-pose.osr / retail-poseL.osr, res 0x570 = bank 0x8b, Arche
 * at the errands freeroam under held-DOWN / held-UP injection; the held-axis
 * injection added to the capture proxy, ckpt 153b).  Both poses are a 3-phase
 * FSM keyed on the body sub-state (body+0x3a; engine-quirk #114): a TRANSITION
 * cel on enter AND exit, holding a steady cel between:
 *   CROUCH (cmd_pose 10, body state 2): enter cel 31 (4t) -> hold 32 -> [release] exit 31 (5t) -> idle
 *   UP     (cmd_pose 0xb, body state 5): enter cel 34 (4t) -> hold 35 -> [release] exit 34 (5t) -> idle
 * These are the RIGHT-facing cels; the LEFT-facing pose emerges from the bank-0x8b
 * +152 mirror (ARCHE_FREEROAM_FLIP) the renderer applies on facing==3 — crouch
 * 31/32 -> 183/184, up 34/35 -> 186/187 — exactly like the walk/idle/run.  So NO
 * dedicated left clips and NO facing override are needed: the pose renders at the
 * CHARACTER facing and the flip does the rest (verified bit-exact, both facings).
 * The enter dur is 5 (not 4): the stepper advances BEFORE the render, so a dur-5
 * frame-0 renders for retail's 4-tick enter.  The EXIT is the transition cel held
 * for ARCHE_POSE_EXIT_TICKS (the arche_pose_anim FSM below). */
static const anim_clip ARCHE_CROUCH_CLIP = {
    .base_sprite = 0, .frame_delta = { 31, 32 }, .frame_count = 2, .frame_dur = 5, .oneshot = 1,
};
static const anim_clip ARCHE_UP_CLIP = {
    .base_sprite = 0, .frame_delta = { 34, 35 }, .frame_count = 2, .frame_dur = 5, .oneshot = 1,
};
static const anim_clip ARCHE_CROUCH_EXIT_CLIP = {
    .base_sprite = 0, .frame_delta = { 31 }, .frame_count = 1, .frame_dur = 1, .oneshot = 1,
};
static const anim_clip ARCHE_UP_EXIT_CLIP = {
    .base_sprite = 0, .frame_delta = { 34 }, .frame_count = 1, .frame_dur = 1, .oneshot = 1,
};

const anim_clip *arche_pose_clip(arche_pose_anim *st, int16_t cmd_pose,
                                 int moving, int airborne, int run)
{
    const anim_clip *want;
    if (cmd_pose == CHAR_POSE_DOWN) {            /* crouch / slide                */
        want = &ARCHE_CROUCH_CLIP;
        st->exit_timer = 0;
    } else if (cmd_pose == CHAR_POSE_UP) {       /* up-defensive                  */
        want = &ARCHE_UP_CLIP;
        st->exit_timer = 0;
    } else {
        /* Not posing.  If a pose was just released, play its EXIT transition cel
         * for ARCHE_POSE_EXIT_TICKS before the walk/idle clip resumes (the draw
         * stream: cel 31/34 holds ~5 ticks on release, then idle).  The LEFT-facing
         * exit emerges from the same +152 flip. */
        if (st->prev_pose != 0) {
            st->exit_kind  = st->prev_pose;
            st->exit_timer = ARCHE_POSE_EXIT_TICKS;
        }
        if (st->exit_timer > 0) {
            st->exit_timer--;
            want = (st->exit_kind == CHAR_POSE_DOWN) ? &ARCHE_CROUCH_EXIT_CLIP
                                                     : &ARCHE_UP_EXIT_CLIP;
        } else {
            want = arche_freeroam_clip(moving, airborne, run);
        }
    }
    st->prev_pose = cmd_pose;
    return want;
}

/* ── The SWORD unsheathe/sheathe clips + FSM (ckpt 155-156; RE-DONE ckpt 159) ──
 * BREAKTHROUGH (USER ckpt 158, re-verified off the clean recording sword2.osr):
 * the sword-OUT form is a SEPARATE BANK.  Drawing RE-installs Arche from bank 0x8b
 * (res 0x570, sword-IN) to bank 0x8c (res 0x571, sword-OUT) — res 0x570 VANISHES
 * from the draw stream the instant Z fires (~tick 1810), she reappears on res 0x571
 * with the blade baked into every cel.  freeroam_step does the bank swap (bank =
 * sword_out ? 0x8c : 0x8b, + flip_table[0x8c]=192); THIS module only picks the cel
 * sequence — which therefore renders on whichever sheet the bank swap selected.
 * Off sword2.osr (`/tmp/sword_cels.py`, tick-aligned to the input recorder):
 *   DRAW   (sword_out 0→1, bank→0x8c): UNSHEATHE res 0x571 cels 96→103, ~7t each
 *     (tick 1810-1865, durs 6,8,7,7,7,7,7,7 ≈ uniform dur-7 oneshot, 56t total),
 *     then falls through to the sword-OUT idle/walk/run/pose (the SAME cel indices
 *     on 0x571 — 0-2 idle / 8-15 walk / 16-21 run / 31-32 crouch / 34-35 up).
 *   SHEATHE(sword_out 1→0, bank→0x8b): res 0x570 cels 96→103 (the OTHER bank!),
 *     durs 96=6t, 97-100=3t, 101/102=9t, 103=12t = 3*{2,1,1,1,1,3,3,4} (tick 3197).
 *     CONFIRMED bit-exact off sword2.osr — ckpt-156 mistook THIS (the res-0x570
 *     sheathe) for the draw, which is why that demo "snapped back to no sword".
 * The LEFT-facing sword-out cels are a UNIFORM +192 mirror on bank 0x8c (idle
 * 0-2→192-194, walk 8-15→200-207, run 16-21→208-213) — ARCHE_SWORD_OUT_FLIP, set
 * in the flip table by freeroam_begin/step (vs +152 for the sword-IN bank 0x8b).
 * base_sprite 96 ⇒ frame_delta = cel - 96. */
static const anim_clip ARCHE_SWORD_DRAW_CLIP = {
    .base_sprite = 96,
    .frame_delta = { 0, 1, 2, 3, 4, 5, 6, 7 },   /* res 0x571 cels 96→103 (out front) */
    .frame_count = 8,
    .frame_dur   = 7,
    .oneshot     = 1,    /* hold on cel 103 -> the sword-OUT idle (bank 0x8c) */
};
static const anim_clip ARCHE_SWORD_SHEATHE_CLIP = {
    .base_sprite = 96,
    .frame_delta = { 0,0, 1, 2, 3, 4, 5,5,5, 6,6,6, 7,7,7,7 },  /* res 0x570 cels 96→103 */
    .frame_count = 16,
    .frame_dur   = 3,
    .oneshot     = 1,    /* hold on cel 103 until the FSM ends -> sword-IN base idle */
};
/* The sword-OUT IDLE: the same cels 0-2 as the sword-in idle but a FASTER cadence —
 * dur 8 (RE'd off sword2.osr ticks 2926-3160: a steady 8t per frame) vs the sword-in
 * idle's dur 14 (a more alert ready-stance with the blade out).  Walk/run/crouch/up
 * share the sword-in clips (same durations, verified), so ONLY the idle needs a sword
 * -out variant; arche_sword_clip swaps it in for the idle case.  (The idle FRAME order
 * — a 0,1,2,1 ping-pong in retail vs this 0,1,2 loop — is the pre-existing
 * PORT-DEBT(char-idle-fidget) shared with the sword-in idle, not a sword concern.) */
static const anim_clip ARCHE_SWORD_OUT_IDLE_CLIP = {
    .base_sprite = 0,
    .frame_delta = { 0, 1, 2 },
    .frame_count = 3,
    .frame_dur   = 8,
    .oneshot     = 0,
};

/* ── The sword-OUT ATTACK swing clips (chip 2a; RE'd off sword2.osr res 0x571) ──
 * The X press (the +0x128 attack level) starts a swing while sword_out + grounded;
 * character_resolve_attack owns the timing (attacking + attack_timer, cleared at the
 * kind's duration = the +0x68 mid-swing lock), this picks the cel sequence keyed on
 * c->attack_kind.  All on bank 0x8c (res 0x571, blade baked in) — the freeroam bank
 * swap already selected it (the swing requires sword_out).  The LEFT-facing swing is
 * the +192 mirror (ARCHE_SWORD_OUT_FLIP) the renderer applies on facing==3.
 *   NEUTRAL (no dir): cels 104->109, dur-6 each = 36t, STATIONARY (sword2.osr ticks
 *   3485-3520: 104@3485 105@3491 106@3497 107@3503 108@3509 109@3515, then idle 0).
 * The dst widens to 64 mid-swing (cels 106/107) from the cel's own footprint — the
 * sprite-bank anchor, not a clip offset (off_x stays 0), so it emerges from res 0x571.
 *
 * chip 2b — the DIRECTIONAL swings (RE'd off sword2.osr res 0x571, tick-aligned to
 * sword2-input.jsonl; durs from `attack_probe.py`).  All on bank 0x8c (res 0x571); the
 * LEFT mirror is the +192 (ARCHE_SWORD_OUT_FLIP) the renderer applies on facing==3, so
 * one right-facing clip serves both — EXCEPT the BACK swing, which renders at the
 * pre-swing facing (the un-mirrored cels) and only flips facing AT COMPLETION
 * (character_resolve_attack), so its post-swing idle lands on the opposite bank.  The
 * uniform frame_dur is 1 where the per-cel durations differ (faked via repeated
 * frame_delta entries, like the sheathe clip), so the clip's total ticks == the kind's
 * CHAR_ATTACK_*_TICKS and the lock + anim end together:
 *   FORWARD 120->126 dur-6 (42t) ticks 3792-3833 — a forward LUNGE (character_step)
 *   DOWN    112->115 [8,6,5,7]=26t ticks 3957-3982 — stationary, returns to crouch
 *   BACK    144->148 [4,4,7,7,5]=27t ticks 4082-4108 — turns around at completion */
static const anim_clip ARCHE_ATTACK_NEUTRAL_CLIP = {
    .base_sprite = 104,
    .frame_delta = { 0, 1, 2, 3, 4, 5 },   /* res 0x571 cels 104->109 (a full swing) */
    .frame_count = 6,
    .frame_dur   = 6,
    .oneshot     = 1,    /* freeze on 109; the character clears `attacking` at 36t   */
};
static const anim_clip ARCHE_ATTACK_FORWARD_CLIP = {
    .base_sprite = 120,
    .frame_delta = { 0, 1, 2, 3, 4, 5, 6 },  /* res 0x571 cels 120->126 */
    .frame_count = 7,
    .frame_dur   = 6,    /* 7 * 6 = 42t = CHAR_ATTACK_FORWARD_TICKS */
    .oneshot     = 1,
};
static const anim_clip ARCHE_ATTACK_DOWN_CLIP = {
    .base_sprite = 112,                      /* res 0x571 cels 112->115, durs [8,6,5,7] */
    .frame_delta = { 0,0,0,0,0,0,0,0, 1,1,1,1,1,1, 2,2,2,2,2, 3,3,3,3,3,3,3 },
    .frame_count = 26,   /* 8+6+5+7 = 26t = CHAR_ATTACK_DOWN_TICKS */
    .frame_dur   = 1,
    .oneshot     = 1,
};
static const anim_clip ARCHE_ATTACK_BACK_CLIP = {
    .base_sprite = 144,                      /* res 0x571 cels 144->148, durs [4,4,7,7,5] */
    .frame_delta = { 0,0,0,0, 1,1,1,1, 2,2,2,2,2,2,2, 3,3,3,3,3,3,3, 4,4,4,4,4 },
    .frame_count = 27,   /* 4+4+7+7+5 = 27t = CHAR_ATTACK_BACK_TICKS */
    .frame_dur   = 1,
    .oneshot     = 1,
};
/* The UP (overhead) attack — chip 2c.  UNLIKE the neutral/directional swings (all on
 * the sword-out bank 0x8c, res 0x571), the up-thrust draws on a SEPARATE BANK 0x8d
 * (res 0x572) — Arche RE-installs from 0x8c to 0x8d the instant it fires (res 0x571
 * VANISHES, res 0x572 takes over), exactly like the Z draw swaps 0x8b->0x8c.  The
 * 0x283f action is registered in the same 0xc35b sword-out form (41f200:1193/1202).
 * freeroam_step does the bank swap (0x8d while attack_kind==UP); THIS clip picks the
 * cel sequence on whatever bank is active.  Off sword2.osr res 0x572 (the up_attack_probe,
 * ticks 3880-3915, RIGHT-facing): cels 0->5, durs [4,4,4,8,8,8] = 36t, dst grows to
 * 36x96 (the thrust extends straight up — the cel's own footprint, off=0), STATIONARY,
 * then back to the res 0x571 sword-out idle.  All durs are multiples of 4, so encode at
 * frame_dur 4 (9 entries, fits ANIM_CLIP_MAX_FRAMES 32) rather than 36 dur-1 entries. */
static const anim_clip ARCHE_ATTACK_UP_CLIP = {
    .base_sprite = 0,                        /* res 0x572 cels 0->5 (bank 0x8d) */
    .frame_delta = { 0, 1, 2, 3,3, 4,4, 5,5 },   /* durs [4,4,4,8,8,8] @ dur-4 */
    .frame_count = 9,
    .frame_dur   = 4,    /* 9 * 4 = 36t = CHAR_ATTACK_UP_TICKS */
    /* The overhead thrust draws RAISED + leaning toward facing — a per-frame draw
     * offset (the renderer's case-0x1872d off_x/off_y, NOT the cel's sheet origin,
     * which is byte-identical port<->retail).  RE'd off sword2.osr dst deltas vs the
     * tick-adjacent stationary up-pose at world_screen (270,336), off_x 0 (the walk/
     * idle/pose baseline): the up-attack renders at y=320 (a uniform -16 RAISE for
     * the overhead pose) and x 286/296 = +16 (cels 0-2,5) / +26 (cels 3-4, the thrust
     * extension leans forward).  off_x is reflected about row->mirror_x on facing==3
     * (the LEFT mirror, residual-unverified with bank 0x8d's +192). */
    .off_x = { 16, 16, 16, 26, 26, 26, 26, 16, 16 },
    .off_y = { -16, -16, -16, -16, -16, -16, -16, -16, -16 },
    .oneshot     = 1,
};

const anim_clip *arche_sword_clip(arche_sword_anim *st, int16_t sword_out,
                                  int attacking, int16_t attack_kind,
                                  arche_pose_anim *pst, int16_t cmd_pose,
                                  int moving, int airborne, int run)
{
    /* Detect the toggle edge: a change in sword_out starts the matching transient.
     * Each transient renders on the DESTINATION bank (the freeroam bank swap follows
     * sword_out): DRAW plays res 0x571 96-103, SHEATHE plays res 0x570 96-103. */
    if (sword_out != st->prev_out) {
        st->phase = (int16_t)(sword_out ? ARCHE_SWORD_PHASE_DRAW
                                        : ARCHE_SWORD_PHASE_SHEATHE);
        st->timer = 0;
    }
    st->prev_out = sword_out;

    if (st->phase == ARCHE_SWORD_PHASE_DRAW) {
        if (st->timer < ARCHE_SWORD_DRAW_TICKS) {
            st->timer = (int16_t)(st->timer + 1);
            return &ARCHE_SWORD_DRAW_CLIP;        /* 96→103: swing the sword out (0x571) */
        }
        st->phase = ARCHE_SWORD_PHASE_NONE;       /* draw done -> sword-OUT idle/walk */
    } else if (st->phase == ARCHE_SWORD_PHASE_SHEATHE) {
        if (st->timer < ARCHE_SWORD_SHEATHE_TICKS) {
            st->timer = (int16_t)(st->timer + 1);
            return &ARCHE_SWORD_SHEATHE_CLIP;     /* 96→103: onto the back (0x570) */
        }
        st->phase = ARCHE_SWORD_PHASE_NONE;       /* sheathe done -> sword-IN base */
    }

    /* The ATTACK swing wins over the pose/walk/idle (the character gates it on
     * sword_out + grounded + not-mid-draw via the 200 ms refractory, and owns the
     * duration).  It does NOT override the draw/sheathe transient above — you finish
     * drawing before you can swing.  Pick the swing's cel sequence by attack_kind;
     * chip 2a is NEUTRAL (104-109), the directionals are chip 2b. */
    if (attacking) {
        switch (attack_kind) {
        case CHAR_ATTACK_FORWARD:
            return &ARCHE_ATTACK_FORWARD_CLIP;
        case CHAR_ATTACK_DOWN:
            return &ARCHE_ATTACK_DOWN_CLIP;
        case CHAR_ATTACK_BACK:
            return &ARCHE_ATTACK_BACK_CLIP;
        case CHAR_ATTACK_UP:        /* overhead thrust, res 0x572 (bank 0x8d swap in freeroam_step) */
            return &ARCHE_ATTACK_UP_CLIP;
        case CHAR_ATTACK_NEUTRAL:
        default:
            return &ARCHE_ATTACK_NEUTRAL_CLIP;
        }
    }

    /* No transient: delegate to the normal walk/idle/pose.  The cel INDICES are the
     * same for both stances (idle 0-2, walk 8-15, run 16-21, crouch/up 31-35); which
     * SHEET they resolve on (res 0x570 sword-in vs 0x571 sword-out, blade baked in) is
     * the freeroam bank swap's job, keyed on the same sword_out.  So sword-out walk
     * draws res 0x571 8-15 automatically — no separate sword-out clips needed.  The ONE
     * exception is the IDLE cadence: swap the dur-8 sword-out idle for the dur-14 base
     * idle (the only clip that differs sword-out; walk/run/pose durations match). */
    const anim_clip *base = arche_pose_clip(pst, cmd_pose, moving, airborne, run);
    if (sword_out && base == &ARCHE_FREEROAM_IDLE_CLIP)
        return &ARCHE_SWORD_OUT_IDLE_CLIP;
    return base;
}
