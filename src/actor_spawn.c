/*
 * actor_spawn.c — see actor_spawn.h.  The town CHARACTER-band spawn: the
 * 0x58d460 -> 0x431e30 slice that turns the map's 70000-range object
 * layers into renderable {actor, render-state} pairs, plus the PORT-DEBT
 * visible-code sprite-table stand-in (the lazy 0x40afe0/0x41e600 fill).
 */
#include "actor_spawn.h"
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
 * other town-scenery structure codes for any room.  (0xeead's bank is a runtime
 * value *(0x8a9b50+0x27a4) — omitted; 0xec59..0xec5e have no case.) */
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
};

int actor_spawn_struct_bank_for_code(uint32_t code, uint16_t *bank)
{
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
 * (FUN_00417c40).  Excludes 0xe29a (RNG wanderers) + 0xc35a/0xc3dc/0xc3f0
 * (non-map party/script spawns).
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

/* The fixed per-object RNG draw count 0x41f200 consumes BEFORE its 0x426ec0
 * idle-phase pair, by EFFECT type code (engine-quirk #86, the seed-pinned
 * 0x5bf505 census cross-checked against the decompile):
 *   0x426fd0 (1, the +0xf4 init) + 0x41f200 prologue (7 = 2 position-jitter
 *   :294/:301 + 5 particle-param :326-334) = 8, PLUS the per-type-switch draw:
 *     0xe29a -> 0x427670 case 2 (5 draws; the wandering villagers, :2181)
 *     0xe2a5 -> 0x431cb0     (1 draw; :2272)
 *     all other town effects -> none.
 * (The conditional 0x41f200:2849 draw + the script effects 0xc35a/0xc3dc/0xc3f0
 * fall AFTER all 11 rendered townsfolk in the spawn order, so they do not affect
 * the townsfolk idle phases and are not modelled here.) */
static int effect_prefix_draws(uint32_t code)
{
    int n = 8;                       /* 0x426fd0 (1) + 0x41f200 prologue (7) */
    if (code == 0xe29au) n += 5;     /* 0x427670 case 2 (the wanderers)      */
    else if (code == 0xe2a5u) n += 1;/* 0x431cb0                             */
    return n;
}

int actor_spawn_effect_from_map(actor_spawn_pool *pool, const map_data *md)
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
         * is USED — and only for the rendered townsfolk (the 0xe29a wanderers +
         * unknown codes still consume their draws, they are just not spawned). */
        for (int k = effect_prefix_draws(code); k > 0; k--)
            (void)rng_rand();
        /* 0x426ec0: the idle PHASE.  frame = (rand * clip.frame_count) >> 15,
         * then timer = (rand * clip.frame_dur) >> 15 (the >>15 is /32768; both
         * operands are small + non-negative so it matches retail's signed form).
         * Every town effect carries a clip, so both draws always fire. */
        uint16_t ph_frame = (uint16_t)((rng_rand() * IDLE_CLIP.frame_count) >> 15);
        uint16_t ph_timer = (uint16_t)((rng_rand() * IDLE_CLIP.frame_dur)   >> 15);

        uint16_t bank; int16_t dstx, dsty; uint32_t layer; int16_t facing, flip;
        if (!actor_spawn_effect_def_for_code(code, &bank, &dstx, &dsty, &layer,
                                             &facing, &flip))
            continue;        /* wanderer (0xe29a) / non-map: draws consumed, not spawned */
        (void)flip;                            /* lands in the render flip table */

        if (pool->count >= ACTOR_BAND_SLOTS)   /* "Effect Object Count Over" */
            return -1;

        int slot = pool->count++;
        actor              *a  = &pool->actors[slot];
        actor_render_state *rs = &pool->states[slot];

        /* 0x41f200: behaviour code (+0x1d4), dir 0 (+0xe8), draw layer (+0xfc).
         * Sprite row 0 = {bank, frame_base 0}; the per-frame sprite comes from
         * the idle clip via the anim stepper.  World = (map (x,y) - dst) * 100;
         * the render dst anchor lives in the render-state (+0x40/+0x44), added
         * back at emit. */
        a->code  = code;
        a->dir   = 0;
        a->layer = layer;
        a->sprite_table[0].bank       = bank;
        a->sprite_table[0].frame_base = 0;

        rs->active     = 1;                     /* +0x00 (the *param_1!='\0' gate) */
        rs->world_x    = (hdr_i32(h, HDR_OFF_X) - dstx) * 100;   /* +0x04 */
        rs->world_y    = (hdr_i32(h, HDR_OFF_Y) - dsty) * 100;   /* +0x08 */
        rs->facing     = facing;                /* +0x2c — 1 normal / 3 mirrored */
        rs->dst_base_x = dstx;                  /* +0x40 — the render anchor */
        rs->dst_base_y = dsty;                  /* +0x44 */
        rs->clip       = &IDLE_CLIP;            /* +0x6c — the idle breathing clip */
        rs->timer      = ph_timer;              /* +0x70 — 0x426ec0 start timer */
        rs->frame      = ph_frame;              /* +0x72 — 0x426ec0 start frame */
        rs->done       = 0;                     /* +0x74 */
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
    uint16_t clip_frame;    /* rs +0x72: idle-clip start phase (cosmetic)        */
} CUTSCENE_FAMILY[] = {
    {0,          0xc3f0u, 3, 0, 4, 0,     0x6400,  43600, -20,  0},  /* Dr. Barnard    -> 0xeb */
    {0x5f5e1d3u, 0,       3, 0, 4, 0,     8000,    43600, -20, 13},  /* Arche's Father -> 0xe3 */
    {0x5f5e1d4u, 0,       1, 0, 0, 0,     -3200,   43600, -20,  1},  /* Arche's Mother -> 0xb5 */
    /* Arche the party LEADER: handle 0x5f5e165 -> code 0xc35a, dramatist bank 0
     * (party-loaded), so party_resolve_spawn yields bank 0 -> override to her body
     * bank 0x8b (the 0x41f200 case 0xc35a :899 install).  Her banks 0x8b-0x8e now
     * register from the EXE (ar_register_party_exe_sprites), so 0x8b decodes and
     * she renders.  Settled census pos: x 41600 (= the wagon anchor, x_off 0),
     * y 45600, dst (-30,-24), facing 1, clip 0x62a8c8 (byte-identical to the idle
     * clip).  PORT-DEBT(cutscene-party-chars): rendered as the static-cast member
     * here (not yet via the party band 0x4997b0); the multi-part body banks
     * 0x8c-0x8e + the walk-in roll-in remain Phase 2/3. */
    {0x5f5e165u, 0,       1, 0, 0, 0x8bu, 0,       45600, -24, 15},  /* Arche -> body 0x8b */
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
