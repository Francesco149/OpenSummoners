/*
 * world_layout.h — RETAIL-LAYOUT types for the in-game world (the 2026-10-14 rhost
 * methodology: ported functions run on retail's own memory inside rhost, so the
 * structs they touch must have retail's byte layout).  Offsets are pinned with
 * _Static_assert on the 32-bit build; the 64-bit host build keeps the same field
 * NAMES (pointers grow, so offsets differ there — code is offset-agnostic), exactly
 * like input_mgr (input.h).  Add fields as ported functions need them; pad bytes
 * are `_pNNN` (NNN = the hex offset they start at).
 *
 *   wl_world   the world singleton *(0x8a9b50) (0x27b8 bytes, ctor 0x4017d0)
 *   wl_entity  the common ENTITY base every world band shares (smallest band
 *              object is 0x1f8 bytes; the 32 full characters at world+0x1160
 *              are 0x15bb0 bytes and extend it)
 *   wl_body    one 0x294-byte body/render-state record (entity+0x40 points at an
 *              array of entity+0x44 of them; the movers take one by part index)
 *   wl_list    the 0xc-byte entity list objects (world+0x2788 / +0x278c)
 */
#ifndef OSS_WORLD_LAYOUT_H
#define OSS_WORLD_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

#include "collision.h"   /* coll_slope_fn */

struct wl_entity;

typedef struct wl_body {
    uint8_t  active;          /* +0x000  0 => part absent (the movers' gate)            */
    uint8_t  _p001[3];
    int32_t  x;               /* +0x004  world X (px*100)                               */
    int32_t  y;               /* +0x008  world Y                                        */
    int32_t  w;               /* +0x00c  box width                                      */
    int32_t  h;               /* +0x010  box height                                     */
    int32_t  margin;          /* +0x014  top inset: the collision top = y + margin      */
    int32_t  vvel;            /* +0x018  vertical velocity (x100 per tick, + = down)     */
    int32_t  fall_cap;        /* +0x01c  != 0 overrides the fall cap (gravity = it/10)  */
    uint8_t  drop;            /* +0x020  drop-through timer (8 on a DOWN+jump)           */
    uint8_t  _p021[3];
    int32_t  grounded;        /* +0x024  the support probe's result                     */
    int32_t  vel_x;           /* +0x028  horizontal speed (x100 per tick, >= 0)          */
    int32_t  facing;          /* +0x02c  1 right, 3 left                                 */
    uint8_t  _p030[0x38 - 0x30];
    uint16_t state;           /* +0x038  0 stand, 1 move, 2 crouch, 3 jump, 4 air,
                               *          5 look up, 6 slide                              */
    uint16_t sub;             /* +0x03a  the state's sub-phase                          */
    uint16_t ctr;             /* +0x03c  the sub-phase's tick counter                   */
    uint8_t  _p03e[0x40 - 0x3e];
    int32_t  dst_x;           /* +0x040  (entity +0x30, stamped each tick)               */
    int32_t  dst_y;           /* +0x044 */
    uint16_t pose;            /* +0x048  the ACTION (0 none, 2 turn around, 10 skill..)  */
    uint8_t  _p04a[0x4c - 0x4a];
    int32_t  spell;           /* +0x04c  the action's record (the targeting spell)       */
    int32_t  target;          /* +0x050  the targeted character (wl_char *)              */
    uint16_t act_arg;         /* +0x054 */
    uint16_t act_len;         /* +0x056  (200 for the turn-around; 210 = reset)          */
    uint16_t f58;             /* +0x058  the action's phase                             */
    uint16_t act_5a;          /* +0x05a */
    int32_t  act_ctr;         /* +0x05c */
    int32_t  serial;          /* +0x060  the world action serial when it started         */
    uint16_t act_64;          /* +0x064 */
    uint16_t f66;             /* +0x066  != 0 (with f68 == 0) => the use key is live     */
    uint8_t  f68;             /* +0x068 */
    uint8_t  _p069[0x6c - 0x69];
    const void *clip;         /* +0x06c  the animation clip (anim_clip.h)               */
    uint16_t clip_timer;      /* +0x070 */
    uint16_t clip_frame;      /* +0x072 */
    uint8_t  clip_done;       /* +0x074 */
    uint8_t  _p075[0x78 - 0x75];
    int32_t  act_78;          /* +0x078 */
    uint8_t  nocollide;       /* +0x07c  != 0 => ignores / is ignored by entity checks  */
    uint8_t  f7d;             /* +0x07d */
    uint8_t  f7e;             /* +0x07e  (the AI drops a target whose body[0] has it set)*/
    uint8_t  f7f;             /* +0x07f */
    uint16_t nhit;            /* +0x080  attack hitboxes live this tick                  */
    uint8_t  _p082[0x84 - 0x82];
    int32_t  hit[8][16];      /* +0x084  64-byte hitboxes: [3] gate, [4] kind, [5..8] x y w h,
                               *          [15] id (8 of them up to +0x284)                 */
    struct wl_entity *attach; /* +0x284  the entity this body rides/belongs to          */
    int32_t  slow;            /* +0x288  region-C flag at the feet (water: a third cap)  */
    int32_t  c_flag;          /* +0x28c  region-C flag at the body's middle row          */
    void    *frames;          /* +0x290  per-frame sub-records (0x14 B each, 0x4411e0)  */
} wl_body;

typedef struct wl_entity {
    const uint8_t *rects;    /* +0x000  32-byte collision rects (0x445d30 copies them)  */
    uint16_t nrects;         /* +0x004 */
    uint8_t  _p006[0x8 - 0x6];
    void    *f08;            /* +0x008  the multi-part frame table (0x445e50)           */
    uint8_t  _p00c[0x28 - 0xc];
    int32_t  width;          /* +0x028  the body box the integrator stamps each tick    */
    int32_t  height;         /* +0x02c */
    int32_t  dst_x;          /* +0x030  -> body +0x40 */
    int32_t  dst_y;          /* +0x034  -> body +0x44 */
    int32_t  no_entity_coll; /* +0x038  != 0 => the movers skip the entity scan        */
    uint8_t  _p03c[0x40 - 0x3c];
    wl_body *body;           /* +0x040  body array                                     */
    uint16_t nbody;          /* +0x044  parts in it                                    */
    uint8_t  _p046[0xf4 - 0x46];
    int32_t  tint_f4;        /* +0x0f4 */
    int32_t  tint_f8;        /* +0x0f8 */
    uint8_t  _p0fc[0x104 - 0xfc];
    int32_t  coll_mode;      /* +0x104  0 none, 1 solid to non-nocollide bodies,       */
                             /*         2 + the faction/state gate (0x4416c0)          */
    int32_t  coll_kind;      /* +0x108  1 side wall, 2 one-way platform, 3 solid block */
    uint8_t  _p10c[0x1c0 - 0x10c];
    int32_t  ride_mode;      /* +0x1c0  as a platform: 0/1 only its rider, 3 anyone     */
    uint8_t  _p1c4[0x1d0 - 0x1c4];
    int32_t  alive;          /* +0x1d0  != 0 => targetable (the AI's target cycling)    */
    int32_t  code;           /* +0x1d4  behaviour code (0xc35b/0xc35c: Arche forms)     */
    uint8_t  _p1d8[0x1dc - 0x1d8];
    int32_t  faction;        /* +0x1dc  0x4416c0 operand                               */
    uint8_t  _p1e0[0x1f0 - 0x1e0];
    int32_t  coll_off;       /* +0x1f0  != 0 => no mode-2 collision                    */
    uint8_t  _p1f4[0x1f8 - 0x1f4];
} wl_entity;

/* One skill record (character+0x754 + 0x300*i, 64 of them; count at +0xc754). */
typedef struct wl_skill {
    int32_t  id;              /* +0x000 */
    uint8_t  _p004[0xbc - 0x4];
    int32_t  ground;          /* +0x0bc  1 needs ground, 2 needs air, 3 ground + 90% speed */
    int32_t  cls;             /* +0x0c0  0xf00000.. party gates, else the member id      */
    uint8_t  _p0c4[0x2ec - 0xc4];
    int32_t  pose_req;        /* +0x2ec  the pose it chains from (10101 = any of 10115/22) */
    uint16_t pose_lo;         /* +0x2f0 */
    uint16_t pose_hi;         /* +0x2f2 */
    uint8_t  _p2f4[0x2f8 - 0x2f4];
    int32_t  mode;            /* +0x2f8  1/2 always, 3 not while probing, 0xff entity gate */
    uint8_t  _p2fc[0x300 - 0x2fc];
} wl_skill;

/* The 32-byte command block a character's AI builds each tick (entity+0x14854; the
 * integrator 0x442a70 consumes it; 0x411530 clears one). */
typedef struct wl_cmd {
    int32_t  lr;              /* +0x00  1/2 walk L/R, 5/6 run L/R                       */
    int32_t  f04;             /* +0x04                                                  */
    int32_t  jump;            /* +0x08  7 jump, 9 drop through, 8/9 jump-held           */
    int32_t  ud;              /* +0x0c  10 down, 11 up                                  */
    int32_t  act;             /* +0x10  14 attack, 15 use                               */
    uint16_t skill;           /* +0x14                                                  */
    uint16_t skill_arg;       /* +0x16                                                  */
    int32_t  skill_ptr;       /* +0x18                                                  */
    uint16_t skill_kind;      /* +0x1c  210 = combo                                     */
    uint16_t _p1e;
} wl_cmd;

/* The per-character party/skill object (character+0x750). */
typedef struct wl_party {
    uint8_t  _p000[0x54];
    int32_t  hp;              /* +0x054 */
    int32_t  hp_max;          /* +0x058 */
    uint8_t  _p05c[0x84 - 0x5c];
    int32_t  hp_bonus1;       /* +0x084 */
    uint8_t  _p088[0x9c - 0x88];
    int32_t  hp_bonus2;       /* +0x09c */
    uint8_t  _p0a0[0xd4 - 0xa0];
    int32_t  member;          /* +0x0d4                                                 */
    uint8_t  _p0d8[0x138 - 0xd8];
    uint16_t slot[3];         /* +0x138  quick-skill slots (0x479d30 / 0x479da0)        */
    uint8_t  _p13e[0x140 - 0x13e];
    int32_t  slot_mode;       /* +0x140                                                 */
    uint8_t  _p144[0x44a - 0x144];
    uint16_t busy;            /* +0x44a  != 0 => a target is dropped                    */
    uint8_t  _p44c[0x45e - 0x44c];
    uint16_t f45e;            /* +0x45e */
    uint8_t  _p460[0x466 - 0x460];
    uint16_t f466;            /* +0x466                                                 */
} wl_party;

/* The keybind/game config object **(*(0x8a6e80)). */
typedef struct wl_keycfg {
    uint8_t  _p000[0x78];
    int32_t  f078;            /* +0x078  1 => the jump-probe "up" assist                */
    uint8_t  _p07c[0x94 - 0x7c];
    int32_t  effects;         /* +0x094  != 0 => the decorative particle effects         */
    uint8_t  _p098[0xf8 - 0x98];
    int32_t  dash_window;     /* +0x0f8  double-tap window (ms) for the run             */
    uint8_t  _p0fc[0x104 - 0xfc];
    int32_t  skill_keys;      /* +0x104  0/1/2 skill-key layout                         */
    uint8_t  _p108[0x168 - 0x108];
    int32_t  sfx_volume;      /* +0x168  the sound-effect volume base                   */
    uint8_t  _p16c[0x510 - 0x16c];
    int32_t  run_mode;        /* +0x510  2 => the run key toggles                       */
} wl_keycfg;

/* One combo/skill input entry (character+0x148a0 + 0x40*i, 0x479a10's record). */
typedef struct wl_combo {
    int32_t  direct;          /* +0x00  != 0 => 0x479e10 (else 0x479e40)                */
    int32_t  skill;           /* +0x04 */
    int32_t  arg;             /* +0x08 */
    int32_t  ts_mode;         /* +0x0c  0/2 fixed time base, 1 = chain off each press    */
    uint32_t lo, hi;          /* +0x10  first window                                    */
    uint32_t hi2, lo2;        /* +0x18  the later keys' window (hi first)                */
    int32_t  ids[5];          /* +0x20  ring ids (5/6 = forward/back by facing)          */
    uint16_t nids;            /* +0x34 */
    uint16_t _p36;
    int32_t  need_release;    /* +0x38 */
    int32_t  cond;            /* +0x3c  skill availability record (0x402a60), or 0       */
} wl_combo;

typedef struct wl_list {
    uint32_t    _p0;
    wl_entity **items;       /* +0x4 */
    uint16_t    count;       /* +0x8 */
} wl_list;

struct wl_char;
typedef struct wl_world {
    uint8_t    _p0000[0x1048];
    uint8_t   *grid;         /* +0x1048  the 0x2cb044-byte runtime grid (map_grid.h)   */
    uint8_t    _p104c[0x1160 - 0x104c];
    struct wl_char *chars[32]; /* +0x1160  the full characters (0x15bb0 B each)        */
    uint8_t    _p11e0[0x2788 - 0x11e0];
    wl_list   *list_2788;    /* +0x2788 */
    wl_list   *collide;      /* +0x278c  entities the movers test against              */
    uint8_t    _p2790[0x27a0 - 0x2790];
    int32_t    action_serial; /* +0x27a0  bumped (mod 0xffffff) per action start         */
    uint8_t    _p27a4[0x27b8 - 0x27a4];
} wl_world;

#include "input.h"

/* A full CHARACTER (Arche, the party, townsfolk with AI): the entity base extended to
 * 0x15bb0 bytes.  Fields are added as ported functions need them. */
typedef struct wl_char {
    wl_entity  base;
    uint8_t    _p1f8[0x200 - 0x1f8];
    int32_t    f200;           /* +0x00200 */
    uint32_t   flags204;       /* +0x00204  0x30000 / 0x1f0000: no dust particles        */
    uint8_t    _p208[0x750 - 0x208];
    wl_party  *party;          /* +0x00750 */
    wl_skill   skills[64];     /* +0x00754 */
    int32_t    n_skills;       /* +0x0c754 */
    uint8_t    _pc758[0xc764 - 0xc758];
    int32_t    fc764;          /* +0x0c764 */
    uint8_t    _pc768[0xc788 - 0xc768];
    int32_t    fc788;          /* +0x0c788  targeting enabled                           */
    uint8_t    _pc78c[0xc798 - 0xc78c];
    struct wl_char *carrier;   /* +0x0c798  (0x442920 releases its hold)                */
    uint8_t    _pc79c[0xc7a4 - 0xc79c];
    struct wl_char *held;      /* +0x0c7a4  the character this one carries              */
    uint8_t    _pc7a8[0xc7e8 - 0xc7a8];
    int32_t    max_air_jumps;  /* +0x0c7e8 */
    int32_t    fc7ec;          /* +0x0c7ec */
    uint8_t    _pc7f0[0xc848 - 0xc7f0];
    /* ── the NPC navigation tuning (0x43f880) ── */
    int32_t    nav_jump_chance;/* +0x0c848  per-mille chance of a spontaneous look-up/jump */
    int32_t    nav_sim_off;    /* +0x0c84c  != 0 => commands are not simulated first     */
    uint8_t    _pc850[0xc854 - 0xc850];
    int32_t    nav_skill;      /* +0x0c854  the chase skill (action 5)                   */
    int32_t    nav_skill_arg;  /* +0x0c858 */
    int32_t    nav_skill2;     /* +0x0c85c  the ranged skill (action 6)                  */
    int32_t    nav_skill2_arg; /* +0x0c860 */
    int32_t    nav_range_lo;   /* +0x0c864 */
    int32_t    nav_range_hi;   /* +0x0c868 */
    int32_t    nav_chase;      /* +0x0c86c  1 => turn to chase when the target is behind  */
    int32_t    nav_c870;       /* +0x0c870 */
    uint8_t    _pc874[0xc890 - 0xc874];
    int32_t    nav_drop;       /* +0x0c890  the floor drop it will jump down / bridge     */
    int32_t    nav_fall;       /* +0x0c894  the drop it will jump over                    */
    int32_t    nav_rise;       /* +0x0c898  the target height difference it climbs to     */
    uint8_t    _pc89c[0x14234 - 0xc89c];
    uint16_t   f14234;         /* +0x14234  != 0 => immune to the hazard probe          */
    uint8_t    _p14236[0x14238 - 0x14236];
    void      *platform;       /* +0x14238  the entity it rides (nav: stay on it)        */
    uint8_t    _p1423c[0x1427c - 0x1423c];
    struct wl_char *target;    /* +0x1427c */
    uint8_t    _p14280[0x14288 - 0x14280];
    int32_t    f14288;         /* +0x14288  != 0 => simulate even with nav_sim_off       */
    uint8_t    _p1428c[0x14810 - 0x1428c];
    int32_t    boost_speed;    /* +0x14810  != 0 => walk max + 6000                     */
    int32_t    boost_accel;    /* +0x14814  != 0 => accel + 2000, brake - 800            */
    uint8_t    _p14818[0x14854 - 0x14818];
    wl_cmd     cmd;            /* +0x14854 */
    struct { int32_t key, val; } member_skill[4]; /* +0x14874  (count +0x14894)        */
    int32_t    n_member_skill; /* +0x14894 */
    int32_t    skill_input;    /* +0x14898  1 => the skill-slot keys are live           */
    int32_t    n_combo;        /* +0x1489c */
    wl_combo   combo[64];      /* +0x148a0 (ends at +0x158a0) */
    int32_t    difficulty;     /* +0x158a0  0..1 => 10 ms, 2 => 80 ms walk-start delay   */
    input_mgr *input;          /* +0x158a4 */
    uint8_t    _p158a8[0x158bc - 0x158a8];
    int32_t    air_jumps;      /* +0x158bc  jumps used since landing                    */
    uint8_t    _p158c0[0x158fc - 0x158c0];
    int32_t    f158fc;         /* +0x158fc  (the hit-reaction helper 0x467260 arg)      */
    uint8_t    _p15900[0x15904 - 0x15900];
    int32_t    f15904;         /* +0x15904  != 0 => pose 0x20 ends by removing itself   */
    int32_t    no_sim;         /* +0x15908  != 0 => every simulated command is accepted  */
    uint8_t    _p1590c[0x15914 - 0x1590c];
    /* ── the per-state clips + effects (0x466930) ── */
    const void *clip_stand;    /* +0x15914 */
    const void *clip_air0;     /* +0x15918 */
    const void *clip_air1;     /* +0x1591c */
    const void *clip_fast;     /* +0x15920  |speed| > 20000 outside the move state      */
    uint8_t    _p15924[0x15934 - 0x15924];
    int32_t    can_carry;      /* +0x15934 */
    uint8_t    _p15938[0x15944 - 0x15938];
    int32_t    f15944;         /* +0x15944  1 => the water jump splash                   */
    uint8_t    _p15948[0x1594c - 0x15948];
    /* ── the movement tuning (0x467bb0) ── */
    int32_t    move_mode;      /* +0x1594c  0 ground, 1..2 the alternate probe, 3 bounded */
    int32_t    fall_cap;       /* +0x15950 */
    int32_t    gravity;        /* +0x15954 */
    int32_t    slide_cap;      /* +0x15958 */
    int32_t    slide_grav;     /* +0x1595c */
    int32_t    nav_low_bridge; /* +0x15960  != 0 => a drop under 3200 is walked over      */
    int32_t    float_h;        /* +0x15964  > 0: the float threshold over region-D floors */
    uint8_t    _p15968[0x1596c - 0x15968];
    int32_t    walk_max;       /* +0x1596c */
    int32_t    walk_accel;     /* +0x15970 */
    int32_t    run_accel;      /* +0x15974 */
    int32_t    brake;          /* +0x15978  (negative) */
    uint8_t    _p1597c[0x15980 - 0x1597c];
    const void *clip_walk;     /* +0x15980 */
    int32_t    turn_anim;      /* +0x15984  != 0 => a reversal plays the turn action     */
    int32_t    turn_anim_air;  /* +0x15988 */
    int32_t    can_run;        /* +0x1598c */
    int32_t    max_speed;      /* +0x15990  the run max */
    const void *clip_run;      /* +0x15994 */
    int32_t    can_jump;       /* +0x15998 */
    int32_t    jump_impulse;   /* +0x1599c */
    int32_t    rise_grav_held; /* +0x159a0 */
    int32_t    rise_grav_free; /* +0x159a4 */
    int32_t    air_max;        /* +0x159a8  != 0 => air control */
    int32_t    air_accel_fast; /* +0x159ac */
    int32_t    air_accel_slow; /* +0x159b0 */
    const void *clip_jump0;    /* +0x159b4  windup */
    const void *clip_jump1;    /* +0x159b8  rise */
    const void *clip_jump2;    /* +0x159bc  apex */
    int32_t    jump_sfx;       /* +0x159c0  sound slot (0 = none) */
    int32_t    jump_sfx_vol;   /* +0x159c4 */
    int32_t    jump_sfx_freq;  /* +0x159c8 */
    int32_t    jump_sfx_jit;   /* +0x159cc  frequency jitter (x rand / 0x8000) */
    int32_t    jump_sfx_pan;   /* +0x159d0 */
    int32_t    can_crouch;     /* +0x159d4 */
    int32_t    crouch_margin;  /* +0x159d8 */
    const void *clip_crouch0;  /* +0x159dc */
    const void *clip_crouch1;  /* +0x159e0 */
    const void *clip_crouch2;  /* +0x159e4 */
    const void *clip_crawl;    /* +0x159e8  crouch at speed > 20000 */
    uint8_t    _p159ec[0x159f0 - 0x159ec];
    const void *clip_hit;      /* +0x159f0  poses 0x68 / 0x69 */
    const void *clip_hit2;     /* +0x159f4  pose 0x6a */
    uint8_t    _p159f8[0x15a10 - 0x159f8];
    int32_t    up_assist;      /* +0x15a10  != 0 => look-up / the jump-probe up assist    */
    uint8_t    _p15a14[0x15a18 - 0x15a14];
    const void *clip_up0;      /* +0x15a18 */
    const void *clip_up1;      /* +0x15a1c */
    uint8_t    _p15a20[0x15a24 - 0x15a20];
    int32_t    f15a24;         /* +0x15a24 */
    int32_t    hazard_mode;    /* +0x15a28  1/2/3: which hitbox kinds count (0x440f40)  */
    uint8_t    _p15a2c[0x15a90 - 0x15a2c];
    int32_t    nav_c_check;    /* +0x15a90  check region-C cells under a drop (nav)      */
    uint8_t    _p15a94[0x15a98 - 0x15a94];
    int32_t    bound_top;      /* +0x15a98  mode 3: -top limit */
    int32_t    bound_bottom;   /* +0x15a9c  mode 3: below the map height */
    int32_t    bound_left;     /* +0x15aa0 */
    int32_t    bound_right;    /* +0x15aa4 */
    uint8_t    _p15aa8[0x15aec - 0x15aa8];
    struct { int32_t id; uint16_t t; uint16_t _p; } recent_hit[8]; /* +0x15aec  hitbox ids already taken */
    uint8_t    _p15b2c[0x15bb0 - 0x15b2c];
} wl_char;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(wl_char, f200)        == 0x00200, "char +0x200");
_Static_assert(offsetof(wl_char, party)       == 0x00750, "char party");
_Static_assert(offsetof(wl_char, fc788)       == 0x0c788, "char +0xc788");
_Static_assert(offsetof(wl_char, target)      == 0x1427c, "char target");
_Static_assert(offsetof(wl_char, cmd)         == 0x14854, "char cmd");
_Static_assert(offsetof(wl_char, member_skill) == 0x14874, "char member skills");
_Static_assert(offsetof(wl_char, n_combo)     == 0x1489c, "char combo count");
_Static_assert(offsetof(wl_char, combo)       == 0x148a0, "char combos");
_Static_assert(offsetof(wl_char, difficulty)  == 0x158a0, "char +0x158a0");
_Static_assert(offsetof(wl_char, input)       == 0x158a4, "char input");
_Static_assert(offsetof(wl_char, max_speed)   == 0x15990, "char +0x15990");
_Static_assert(offsetof(wl_char, max_air_jumps) == 0x0c7e8, "char +0xc7e8");
_Static_assert(offsetof(wl_char, boost_accel) == 0x14814, "char +0x14814");
_Static_assert(offsetof(wl_char, air_jumps)   == 0x158bc, "char +0x158bc");
_Static_assert(offsetof(wl_char, move_mode)   == 0x1594c, "char +0x1594c");
_Static_assert(offsetof(wl_char, float_h)     == 0x15964, "char +0x15964");
_Static_assert(offsetof(wl_char, brake)       == 0x15978, "char +0x15978");
_Static_assert(offsetof(wl_char, air_accel_slow) == 0x159b0, "char +0x159b0");
_Static_assert(offsetof(wl_char, crouch_margin) == 0x159d8, "char +0x159d8");
_Static_assert(offsetof(wl_char, f15a24)      == 0x15a24, "char +0x15a24");
_Static_assert(offsetof(wl_char, bound_right) == 0x15aa4, "char +0x15aa4");
_Static_assert(offsetof(wl_body, vvel)        == 0x018, "body vvel");
_Static_assert(offsetof(wl_body, grounded)    == 0x024, "body grounded");
_Static_assert(offsetof(wl_body, state)       == 0x038, "body state");
_Static_assert(offsetof(wl_body, act_78)      == 0x078, "body +0x78");
_Static_assert(offsetof(wl_body, slow)        == 0x288, "body +0x288");
_Static_assert(offsetof(wl_body, clip)        == 0x06c, "body clip");
_Static_assert(offsetof(wl_body, clip_done)   == 0x074, "body clip done");
_Static_assert(offsetof(wl_char, flags204)    == 0x00204, "char +0x204");
_Static_assert(offsetof(wl_char, clip_stand)  == 0x15914, "char clip_stand");
_Static_assert(offsetof(wl_char, f15944)      == 0x15944, "char +0x15944");
_Static_assert(offsetof(wl_char, clip_walk)   == 0x15980, "char clip_walk");
_Static_assert(offsetof(wl_char, clip_run)    == 0x15994, "char clip_run");
_Static_assert(offsetof(wl_char, clip_jump0)  == 0x159b4, "char clip_jump0");
_Static_assert(offsetof(wl_char, clip_crawl)  == 0x159e8, "char clip_crawl");
_Static_assert(offsetof(wl_char, clip_up1)    == 0x15a1c, "char clip_up1");
_Static_assert(offsetof(wl_char, skills)      == 0x00754, "char skills");
_Static_assert(offsetof(wl_char, n_skills)    == 0x0c754, "char skill count");
_Static_assert(offsetof(wl_char, carrier)     == 0x0c798, "char +0xc798");
_Static_assert(offsetof(wl_char, held)        == 0x0c7a4, "char +0xc7a4");
_Static_assert(offsetof(wl_char, f158fc)      == 0x158fc, "char +0x158fc");
_Static_assert(offsetof(wl_char, f15904)      == 0x15904, "char +0x15904");
_Static_assert(offsetof(wl_char, can_carry)   == 0x15934, "char +0x15934");
_Static_assert(offsetof(wl_char, clip_hit)    == 0x159f0, "char +0x159f0");
_Static_assert(offsetof(wl_char, f14234)      == 0x14234, "char +0x14234");
_Static_assert(offsetof(wl_char, hazard_mode) == 0x15a28, "char +0x15a28");
_Static_assert(offsetof(wl_char, recent_hit)  == 0x15aec, "char +0x15aec");
_Static_assert(offsetof(wl_char, nav_jump_chance) == 0x0c848, "char +0xc848");
_Static_assert(offsetof(wl_char, nav_rise)    == 0x0c898, "char +0xc898");
_Static_assert(offsetof(wl_char, platform)    == 0x14238, "char +0x14238");
_Static_assert(offsetof(wl_char, f14288)      == 0x14288, "char +0x14288");
_Static_assert(offsetof(wl_char, no_sim)      == 0x15908, "char +0x15908");
_Static_assert(offsetof(wl_char, nav_low_bridge) == 0x15960, "char +0x15960");
_Static_assert(offsetof(wl_char, nav_c_check) == 0x15a90, "char +0x15a90");
_Static_assert(offsetof(wl_party, hp_bonus2)  == 0x09c, "party +0x9c");
_Static_assert(offsetof(wl_party, f45e)       == 0x45e, "party +0x45e");
_Static_assert(sizeof(wl_skill)               == 0x300, "skill record");
_Static_assert(offsetof(wl_skill, mode)       == 0x2f8, "skill mode");
_Static_assert(offsetof(wl_entity, width)     == 0x028, "entity width");
_Static_assert(offsetof(wl_entity, tint_f8)   == 0x0f8, "entity +0xf8");
_Static_assert(offsetof(wl_body, dst_x)       == 0x040, "body +0x40");
_Static_assert(offsetof(wl_body, serial)      == 0x060, "body +0x60");
_Static_assert(offsetof(wl_body, nhit)        == 0x080, "body +0x80");
_Static_assert(offsetof(wl_body, hit)         == 0x084, "body hitboxes");
_Static_assert(offsetof(wl_body, c_flag)      == 0x28c, "body +0x28c");
_Static_assert(offsetof(wl_char, up_assist)   == 0x15a10, "char +0x15a10");
_Static_assert(sizeof(wl_char)                == 0x15bb0, "character is 0x15bb0 bytes");
_Static_assert(sizeof(wl_cmd)                 == 0x20,  "command block");
_Static_assert(sizeof(wl_combo)               == 0x40,  "combo entry");
_Static_assert(offsetof(wl_party, busy)       == 0x44a, "party busy");
_Static_assert(offsetof(wl_party, f466)       == 0x466, "party +0x466");
_Static_assert(offsetof(wl_keycfg, run_mode)  == 0x510, "keycfg run mode");
_Static_assert(offsetof(wl_body, frames)      == 0x290, "body frames");
_Static_assert(offsetof(wl_body, facing)      == 0x02c, "body facing");
_Static_assert(offsetof(wl_body, target)      == 0x050, "body target");
_Static_assert(offsetof(wl_body, f66)         == 0x066, "body +0x66");
_Static_assert(offsetof(wl_entity, code)      == 0x1d4, "entity code");
_Static_assert(offsetof(wl_entity, ride_mode) == 0x1c0, "entity ride mode");
_Static_assert(offsetof(wl_world, chars)      == 0x1160, "world chars");
_Static_assert(offsetof(wl_body, x)         == 0x004, "body x");
_Static_assert(offsetof(wl_body, margin)    == 0x014, "body margin");
_Static_assert(offsetof(wl_body, nocollide) == 0x07c, "body nocollide");
_Static_assert(offsetof(wl_body, attach)    == 0x284, "body attach");
_Static_assert(sizeof(wl_body)              == 0x294, "body is 0x294 bytes");
_Static_assert(offsetof(wl_entity, no_entity_coll) == 0x038, "entity +0x38");
_Static_assert(offsetof(wl_entity, body)     == 0x040, "entity body");
_Static_assert(offsetof(wl_entity, nbody)    == 0x044, "entity nbody");
_Static_assert(offsetof(wl_entity, coll_mode) == 0x104, "entity coll_mode");
_Static_assert(offsetof(wl_entity, coll_kind) == 0x108, "entity coll_kind");
_Static_assert(offsetof(wl_entity, faction)  == 0x1dc, "entity faction");
_Static_assert(offsetof(wl_entity, coll_off) == 0x1f0, "entity coll_off");
_Static_assert(sizeof(wl_entity)             == 0x1f8, "entity base is 0x1f8 bytes");
_Static_assert(offsetof(wl_list, count)      == 0x8,   "list count");
_Static_assert(offsetof(wl_world, grid)      == 0x1048, "world grid");
_Static_assert(offsetof(wl_world, collide)   == 0x278c, "world collide list");
_Static_assert(sizeof(wl_world)              == 0x27b8, "world is 0x27b8 bytes");
#endif

/* The port's *(0x8a9b50) + the slope-profile resolver the tile movers use (a sloped
 * region-B cell's +0x8 is a retail .rdata VA; the port reads it off the user's exe,
 * rhost reads the mapped image). */
extern wl_world     *g_world;
extern coll_slope_fn g_world_slope;
extern void         *g_world_slope_ctx;

#endif /* OSS_WORLD_LAYOUT_H */
