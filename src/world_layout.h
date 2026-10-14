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
struct camera_view;

/* One EFFECT RECORD (entity+0x10c, 8 of them, count +0x1ac): 0x448cb0 adds one, 0x54d090
 * runs them each tick (splash / dust / spark particles, one-shot sounds). */
typedef struct wl_fx {
    int32_t  id;              /* +0x00  100146..100158 (0 = free)                         */
    uint16_t dur;             /* +0x04  ticks to live (0 = until removed)                 */
    uint16_t t;               /* +0x06  ticks run                                         */
    uint16_t f08, f0a;        /* +0x08 */
    int32_t  f0c;             /* +0x0c */
    int32_t  y;               /* +0x10  the splash particles' y                           */
} wl_fx;

/* One sound-pool entry: the buffer wrapper (0x5bb870.. take it) + an in-use word. */
typedef struct wl_voice {
    void    *buf;             /* +0x00 */
    int32_t  owner;           /* +0x04  != 0 => taken                                     */
} wl_voice;

/* A sound pool: *(sys + 0x364 + 4*slot) (0x40b710 takes a voice round-robin). */
typedef struct wl_vpool {
    uint16_t  n;              /* +0x00 */
    uint16_t  next;           /* +0x02  where the next search starts                      */
    wl_voice *v;              /* +0x04  0 until loaded (0x409e90)                         */
    void     *dev;            /* +0x08  0 => no sound                                     */
} wl_vpool;

/* One LOOPING-SOUND channel (entity+0x1b0 -> an array of entity+0x1b4): its owner sets
 * `live` each tick it keeps it; 0x46d180 / 0x46d4a0 place it relative to the listener
 * and fade it out (-100 per tick to -5000, then release) once it is no longer live. */
typedef struct wl_sndch {
    wl_voice *voice;          /* +0x00  0 = free                                          */
    void    *pool;            /* +0x04  the wl_vpool the voice came from                  */
    int32_t  vol;             /* +0x08  volume offset                                     */
    int32_t  freq;            /* +0x0c */
    int32_t  falloff;         /* +0x10  0 = -5000, 1 = not positional, -16000 = near only */
    int32_t  live;            /* +0x14 */
    int32_t  fading;          /* +0x18 */
    int32_t  fade;            /* +0x1c  0 .. -5000                                        */
    int32_t  f20;             /* +0x20 */
} wl_sndch;

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
    uint8_t *rects;          /* +0x000  32-byte collision rects (0x445d30 copies them;
                              *          +0x1c u16 = a per-rect cooldown 0x485fc0 ticks)  */
    uint16_t nrects;         /* +0x004 */
    uint8_t  _p006[0x8 - 0x6];
    void    *f08;            /* +0x008  the multi-part frame table (0x445e50)           */
    int32_t  f00c;           /* +0x00c  actors: the switch state (0 off, 1 on, 2..)       */
    struct wl_entity *parent; /* +0x010 particles: the entity they ride (0x46e510)       */
    int32_t  par_dx;         /* +0x014  ... at this offset from its body's top centre    */
    int32_t  par_dy;         /* +0x018 */
    uint8_t  _p01c[0x24 - 0x1c];
    int32_t  f24;            /* +0x024  added to the body y for the listener distance    */
    int32_t  width;          /* +0x028  the body box the integrator stamps each tick    */
    int32_t  height;         /* +0x02c */
    int32_t  dst_x;          /* +0x030  -> body +0x40 */
    int32_t  dst_y;          /* +0x034  -> body +0x44 */
    int32_t  no_entity_coll; /* +0x038  != 0 => the movers skip the entity scan        */
    int32_t  f03c;           /* +0x03c */
    wl_body *body;           /* +0x040  body array                                     */
    uint16_t nbody;          /* +0x044  parts in it                                    */
    uint8_t  _p046[0xe8 - 0x46];
    int32_t  fe8;            /* +0x0e8 */
    uint8_t  _p0ec[0xf4 - 0xec];
    int32_t  tint_f4;        /* +0x0f4 */
    int32_t  tint_f8;        /* +0x0f8 */
    int32_t  f0fc;           /* +0x0fc */
    int32_t  f100;           /* +0x100 */
    int32_t  coll_mode;      /* +0x104  0 none, 1 solid to non-nocollide bodies,       */
                             /*         2 + the faction/state gate (0x4416c0)          */
    int32_t  coll_kind;      /* +0x108  1 side wall, 2 one-way platform, 3 solid block */
    wl_fx    fx[8];          /* +0x10c  effect records                                  */
    uint16_t nfx;            /* +0x1ac */
    uint8_t  _p1ae[0x1b0 - 0x1ae];
    wl_sndch *snd;           /* +0x1b0  looping-sound channels (0 = none)               */
    uint16_t nsnd;           /* +0x1b4 */
    uint8_t  _p1b6[0x1c0 - 0x1b6];
    int32_t  ride_mode;      /* +0x1c0  as a platform: 0/1 only its rider, 3 anyone     */
    uint8_t  _p1c4[0x1d0 - 0x1c4];
    int32_t  alive;          /* +0x1d0  != 0 => targetable (the AI's target cycling)    */
    int32_t  code;           /* +0x1d4  behaviour code (0xc35b/0xc35c: Arche forms)     */
    int32_t  handle;         /* +0x1d8  the id 0x556eb0 resolves (world list +0x2790)   */
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
    uint8_t  _p05c[0x60 - 0x5c];
    int32_t  mp_max;          /* +0x060 */
    uint8_t  _p064[0x84 - 0x64];
    int32_t  hp_bonus1;       /* +0x084 */
    int32_t  mp_bonus1;       /* +0x088 */
    uint8_t  _p08c[0x9c - 0x8c];
    int32_t  hp_bonus2;       /* +0x09c */
    int32_t  mp_bonus2;       /* +0x0a0 */
    uint8_t  _p0a4[0xd4 - 0xa4];
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
    uint8_t  _p16c[0x190 - 0x16c];
    int32_t  voices;          /* +0x190 */
    uint8_t  _p194[0x1a0 - 0x194];
    int32_t  voice_volume;    /* +0x1a0 */
    uint8_t  _p1a4[0x510 - 0x1a4];
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
    uint8_t    _p0000[0x1030];
    int32_t    scene_kind;   /* +0x1030  2 => no fall damage / no yawn                 */
    int32_t    f1034;        /* +0x1034  != 0 => no fall damage                        */
    uint8_t    _p1038[0x1040 - 0x1038];
    uint8_t   *f1040;        /* +0x1040  +0x28 = the effects volume (per mille)        */
    uint8_t    _p1044[0x1048 - 0x1044];
    uint8_t   *grid;         /* +0x1048  the 0x2cb044-byte runtime grid (map_grid.h)   */
    struct camera_view *view; /* +0x104c  the camera/view (camera_follow.h)           */
    uint8_t    _p1050[0x1060 - 0x1050];
    wl_entity *band64[64];   /* +0x1060  0x258-byte entities (shots / projectiles)      */
    struct wl_char *chars[32]; /* +0x1160  the full characters (0x15bb0 B each)        */
    wl_entity *actors[128];  /* +0x11e0  0x2fc-byte actors (townsfolk props, NPC cast)  */
    wl_entity *particles[1024]; /* +0x13e0 0x284-byte particles                         */
    wl_entity *band96[96];   /* +0x23e0  0x20c-byte entities                            */
    uint8_t    _p2560[0x2760 - 0x2560];
    int32_t    f2760;        /* +0x2760  != 0 => no idle anims                         */
    int32_t    f2764;        /* +0x2764  == 1 => (0x47d740) a scene is running          */
    uint8_t    _p2768[0x276c - 0x2768];
    uint16_t   tick15;       /* +0x276c  (0x46cd70: +1 & 0x7fff per tick) */
    uint8_t    _p276e[0x2770 - 0x276e];
    int32_t    f2770;        /* +0x2770  != 0 => (0x47d740) a scene is running          */
    uint8_t    _p2774[0x2788 - 0x2774];
    wl_list   *list_2788;    /* +0x2788 */
    wl_list   *collide;      /* +0x278c  entities the movers test against              */
    wl_list   *list_2790;    /* +0x2790  the actors by handle (0x556eb0)                */
    wl_list   *list_2794;    /* +0x2794 */
    wl_list   *list_2798;    /* +0x2798 */
    int32_t    f279c;        /* +0x279c  (-3 per tick to 0) */
    int32_t    action_serial; /* +0x27a0  bumped (mod 0xffffff) per action start         */
    uint8_t    _p27a4[0x27a8 - 0x27a4];
    int32_t    busy;         /* +0x27a8  != 0 => the ambient emitters hold (scene lock)  */
    uint8_t    _p27ac[0x27b8 - 0x27ac];
} wl_world;

/* A world PARTICLE (world+0x13e0, 1024 slots; 0x284 bytes): the entity base + the per-code
 * motion state 0x46e510 runs. */
typedef struct wl_particle {
    wl_entity  base;
    uint8_t    _p1f8[0x258 - 0x1f8];
    uint16_t   life;           /* +0x258  ticks to live (0 = until killed)               */
    uint8_t    _p25a[0x25c - 0x25a];
    int32_t    f25c;           /* +0x25c  per-code parameter                             */
    int32_t    f260;           /* +0x260 */
    int32_t    f264;           /* +0x264 */
    uint8_t    _p268[0x26c - 0x268];
    uint16_t   age;            /* +0x26c */
    uint8_t    _p26e[0x270 - 0x26e];
    int32_t    snd_on;         /* +0x270  != 0 => its looping channels are placed       */
    wl_entity *owner;          /* +0x274 */
    int32_t    dir_x;          /* +0x278  the motion direction for vel_x (x100000)      */
    int32_t    dir_y;          /* +0x27c */
    uint8_t    _p280[0x284 - 0x280];
} wl_particle;

/* A world ACTOR (world+0x11e0, 128 slots; 0x2fc bytes): the entity base extended with
 * the per-code behaviour state 0x54f980 runs (props, emitters, lifts, doors, birds). */
typedef struct wl_actor {
    wl_entity  base;
    void      *f1f8;           /* +0x1f8  a linked object (0x41ea70 releases it)         */
    wl_entity *follow;         /* +0x1fc  the entity it sticks to (code 100144)          */
    int32_t    follow_dx;      /* +0x200 */
    int32_t    follow_dy;      /* +0x204 */
    struct {
        int32_t     done;      /* +0x0  != 0 => skipped by the switch-off                */
        const void *clip;      /* +0x4  the clip it sets on the body                     */
        uint16_t    sfx;       /* +0x8  the sound it plays                               */
        uint16_t    _p;
    } steps[8];                /* +0x208 */
    uint16_t   nsteps;         /* +0x268 */
    uint8_t    _p26a[0x26c - 0x26a];
    int32_t    grow_max;       /* +0x26c  code 70120: the height it grows to while off   */
    int32_t    grow_min;       /* +0x270  ... shrinks to while on                        */
    uint8_t    _p274[0x27c - 0x274];
    int32_t    goal_x;         /* +0x27c  code 100142 pose 146: where it flies to         */
    int32_t    goal_y;         /* +0x280 */
    int32_t    f284;           /* +0x284 */
    uint8_t    _p288[0x28c - 0x288];
    int32_t    kind;           /* +0x28c */
    uint8_t    _p290[0x298 - 0x290];
    int32_t    link;           /* +0x298  an actor handle (0x556eb0)                     */
    uint8_t    _p29c[0x2a8 - 0x29c];
    void      *f2a8;           /* +0x2a8  a heap buffer it owns                          */
    int32_t    multipart;      /* +0x2ac  != 0 => the parts run too (0x46cd70)           */
    uint8_t    _p2b0[0x2b4 - 0x2b0];
    int32_t    area_x0;        /* +0x2b4  the emitter's span                             */
    int32_t    area_x1;        /* +0x2b8 */
    int32_t    area_y0;        /* +0x2bc */
    int32_t    area_y1;        /* +0x2c0 */
    int32_t    cooldown;       /* +0x2c4 */
    int32_t    use_mover;      /* +0x2c8  != 0 => the ground probe is the full mover     */
    int32_t    step;           /* +0x2cc */
    uint8_t    _p2d0[0x2fc - 0x2d0];
} wl_actor;

#include "input.h"

/* A full CHARACTER (Arche, the party, townsfolk with AI): the entity base extended to
 * 0x15bb0 bytes.  Fields are added as ported functions need them. */
typedef struct wl_char {
    wl_entity  base;
    int32_t    f1f8;           /* +0x001f8  1 => the regen ticks (with faction 2)        */
    uint8_t    _p1fc[0x200 - 0x1fc];
    int32_t    f200;           /* +0x00200  0 player AI, 1 NPC behaviour, >1 idle        */
    uint32_t   flags204;       /* +0x00204  lo16 = the +0x66 timer phase, hi16 = a tick
                                *           counter; 0x30000 / 0x1f0000: no dust          */
    uint8_t    _p208[0x264 - 0x208];
    int32_t    f264;           /* +0x00264  the idle-yawn accumulator                   */
    int32_t    f268;           /* +0x00268  != 0 => intangible (bodies get nocollide)   */
    uint16_t   invuln_a;       /* +0x0026c */
    uint16_t   invuln_b;       /* +0x0026e */
    uint32_t   f270;           /* +0x00270  hi16 counts down                            */
    uint8_t    _p274[0x750 - 0x274];
    wl_party  *party;          /* +0x00750 */
    wl_skill   skills[64];     /* +0x00754 */
    int32_t    n_skills;       /* +0x0c754 */
    int32_t    respawn;        /* +0x0c758  != 0 => falling out respawns at (rx, ry)     */
    int32_t    respawn_x;      /* +0x0c75c */
    int32_t    respawn_y;      /* +0x0c760 */
    int32_t    fc764;          /* +0x0c764 */
    uint8_t    _pc768[0xc76c - 0xc768];
    int32_t    voice_delay;    /* +0x0c76c  ticks until the queued voice line */
    int32_t    voice_id;       /* +0x0c770 */
    int32_t    voice_arg;      /* +0x0c774 */
    int32_t    voice_vol;      /* +0x0c778 */
    int32_t    fc77c;          /* +0x0c77c  the facing at the last 1600-unit step        */
    int32_t    fc780;          /* +0x0c780  the x of the last 1600-unit step             */
    int32_t    fc784;          /* +0x0c784  (-5 per tick outside look-up)                */
    int32_t    fc788;          /* +0x0c788  targeting enabled                           */
    uint16_t   step_delay;     /* +0x0c78c  footstep sound countdown                    */
    uint16_t   step_kind;      /* +0x0c78e */
    uint8_t    _pc790[0xc794 - 0xc790];
    int32_t    fc794;          /* +0x0c794  != 0 => ally proximity / idle anims          */
    struct wl_char *carrier;   /* +0x0c798  (0x442920 releases its hold)                */
    uint8_t    _pc79c[0xc7a4 - 0xc79c];
    struct wl_char *held;      /* +0x0c7a4  the character this one carries              */
    uint8_t    _pc7a8[0xc7e8 - 0xc7a8];
    int32_t    max_air_jumps;  /* +0x0c7e8 */
    int32_t    fc7ec;          /* +0x0c7ec */
    uint8_t    _pc7f0[0xc824 - 0xc7f0];
    int32_t    fc824[2];       /* +0x0c824  linked effects (released when fc82c is 0)    */
    int32_t    fc82c[2];       /* +0x0c82c  (cleared each tick)                          */
    uint8_t    _pc834[0xc848 - 0xc834];
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
    int32_t    stun_on;        /* +0x158a8  with stun_t >= stun_len: the action is cut  */
    int32_t    stun_t;         /* +0x158ac */
    int32_t    stun_len;       /* +0x158b0 */
    uint8_t    _p158b4[0x158bc - 0x158b4];
    int32_t    air_jumps;      /* +0x158bc  jumps used since landing                    */
    uint16_t   regen_t;        /* +0x158c0  regen tick (fires at 1600)                  */
    uint16_t   f158c2;         /* +0x158c2  counts down; clears f158c4 at 0              */
    int32_t    f158c4;         /* +0x158c4 */
    int32_t    leash_t;        /* +0x158c8  ticks bound to f15b60 (saturates 0xffffff)   */
    uint8_t    _p158cc[0x158fc - 0x158cc];
    int32_t    f158fc;         /* +0x158fc  (the hit-reaction helper 0x467260 arg)      */
    uint8_t    _p15900[0x15904 - 0x15900];
    int32_t    f15904;         /* +0x15904  != 0 => pose 0x20 ends by removing itself   */
    int32_t    no_sim;         /* +0x15908  != 0 => every simulated command is accepted  */
    int32_t    no_splash;      /* +0x1590c  != 0 => no water entry/exit splash          */
    uint8_t    _p15910[0x15914 - 0x15910];
    /* ── the per-state clips + effects (0x466930) ── */
    const void *clip_stand;    /* +0x15914 */
    const void *clip_air0;     /* +0x15918 */
    const void *clip_air1;     /* +0x1591c */
    const void *clip_fast;     /* +0x15920  |speed| > 20000 outside the move state      */
    uint8_t    _p15924[0x15934 - 0x15924];
    int32_t    can_carry;      /* +0x15934 */
    uint8_t    _p15938[0x15944 - 0x15938];
    int32_t    f15944;         /* +0x15944  1/2 footstep surfaces, 3 a fixed step sound  */
    int32_t    step_sfx;       /* +0x15948  (mode 3) */
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
    uint8_t    _p159f8[0x15a0a - 0x159f8];
    uint16_t   member_pose;    /* +0x15a0a  the pose that keeps party+0xd4 */
    uint8_t    _p15a0c[0x15a10 - 0x15a0c];
    int32_t    up_assist;      /* +0x15a10  != 0 => look-up / the jump-probe up assist    */
    uint8_t    _p15a14[0x15a18 - 0x15a14];
    const void *clip_up0;      /* +0x15a18 */
    const void *clip_up1;      /* +0x15a1c */
    uint8_t    _p15a20[0x15a24 - 0x15a20];
    int32_t    f15a24;         /* +0x15a24 */
    int32_t    hazard_mode;    /* +0x15a28  1/2/3: which hitbox kinds count (0x440f40)  */
    uint8_t    _p15a2c[0x15a70 - 0x15a2c];
    int32_t    leash_check;    /* +0x15a70 */
    int32_t    leash_max;      /* +0x15a74 */
    int32_t    leash_warp;     /* +0x15a78  (0x4874b0 target) */
    int32_t    fall_warp;      /* +0x15a7c */
    int32_t    f15a80;         /* +0x15a80 */
    uint8_t    _p15a84[0x15a88 - 0x15a84];
    int32_t    voice_vol_bias; /* +0x15a88 */
    int32_t    voice_mode;     /* +0x15a8c */
    int32_t    nav_c_check;    /* +0x15a90  check region-C cells under a drop (nav)      */
    uint8_t    _p15a94[0x15a98 - 0x15a94];
    int32_t    bound_top;      /* +0x15a98  mode 3: -top limit */
    int32_t    bound_bottom;   /* +0x15a9c  mode 3: below the map height */
    int32_t    bound_left;     /* +0x15aa0 */
    int32_t    bound_right;    /* +0x15aa4 */
    uint8_t    _p15aa8[0x15ab0 - 0x15aa8];
    int32_t    f15ab0;         /* +0x15ab0  1 => run 0x4895c0 once */
    uint8_t    _p15ab4[0x15aec - 0x15ab4];
    struct { int32_t id; uint16_t t; uint16_t _p; } recent_hit[8]; /* +0x15aec  hitbox ids already taken */
    uint16_t   idle_n;         /* +0x15b2c  idle animations (the table below) */
    struct { uint16_t a, pose; } idle[4]; /* +0x15b2e */
    uint8_t    _p15b3e[0x15b40 - 0x15b3e];
    int32_t    idle_first;     /* +0x15b40 */
    int32_t    idle_next;      /* +0x15b44 */
    int32_t    idle_jitter;    /* +0x15b48 */
    int32_t    idle_i;         /* +0x15b4c */
    int32_t    idle_t;         /* +0x15b50 */
    void      *voice;          /* +0x15b54  the character's voice object (0x4099a0)      */
    uint8_t    _p15b58[0x15b5c - 0x15b58];
    int32_t    f15b5c;         /* +0x15b5c */
    struct wl_char *leash;     /* +0x15b60 */
    uint8_t    _p15b64[0x15bb0 - 0x15b64];
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
_Static_assert(offsetof(wl_party, mp_bonus2)  == 0x0a0, "party +0xa0");
_Static_assert(offsetof(wl_char, f1f8)        == 0x001f8, "char +0x1f8");
_Static_assert(offsetof(wl_char, f270)        == 0x00270, "char +0x270");
_Static_assert(offsetof(wl_char, respawn_y)   == 0x0c760, "char +0xc760");
_Static_assert(offsetof(wl_char, voice_vol)   == 0x0c778, "char +0xc778");
_Static_assert(offsetof(wl_char, fc794)       == 0x0c794, "char +0xc794");
_Static_assert(offsetof(wl_char, fc82c)       == 0x0c82c, "char +0xc82c");
_Static_assert(offsetof(wl_char, stun_len)    == 0x158b0, "char +0x158b0");
_Static_assert(offsetof(wl_char, leash_t)     == 0x158c8, "char +0x158c8");
_Static_assert(offsetof(wl_char, no_splash)   == 0x1590c, "char +0x1590c");
_Static_assert(offsetof(wl_char, step_sfx)    == 0x15948, "char +0x15948");
_Static_assert(offsetof(wl_char, member_pose) == 0x15a0a, "char +0x15a0a");
_Static_assert(offsetof(wl_char, voice_mode)  == 0x15a8c, "char +0x15a8c");
_Static_assert(offsetof(wl_char, f15ab0)      == 0x15ab0, "char +0x15ab0");
_Static_assert(offsetof(wl_char, idle)        == 0x15b2e, "char idle table");
_Static_assert(offsetof(wl_char, idle_t)      == 0x15b50, "char +0x15b50");
_Static_assert(offsetof(wl_char, leash)       == 0x15b60, "char +0x15b60");
_Static_assert(offsetof(wl_keycfg, voice_volume) == 0x1a0, "keycfg +0x1a0");
_Static_assert(offsetof(wl_world, f1034)      == 0x1034, "world +0x1034");
_Static_assert(offsetof(wl_world, tick15)     == 0x276c, "world +0x276c");
_Static_assert(offsetof(wl_world, f279c)      == 0x279c, "world +0x279c");
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
_Static_assert(offsetof(wl_world, band64)     == 0x1060, "world band64");
_Static_assert(offsetof(wl_world, particles)  == 0x13e0, "world particles");
_Static_assert(offsetof(wl_world, band96)     == 0x23e0, "world band96");
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
_Static_assert(offsetof(wl_entity, f24)      == 0x024, "entity +0x24");
_Static_assert(offsetof(wl_entity, f100)     == 0x100, "entity +0x100");
_Static_assert(offsetof(wl_entity, fx)       == 0x10c, "entity fx records");
_Static_assert(offsetof(wl_entity, nfx)      == 0x1ac, "entity fx count");
_Static_assert(offsetof(wl_entity, snd)      == 0x1b0, "entity sound channels");
_Static_assert(offsetof(wl_entity, nsnd)     == 0x1b4, "entity sound count");
_Static_assert(sizeof(wl_fx)                 == 0x14,  "fx record");
_Static_assert(sizeof(wl_sndch)              == 0x24,  "sound channel");
_Static_assert(offsetof(wl_vpool, dev)       == 0x8,   "pool device");
_Static_assert(offsetof(wl_world, f1040)     == 0x1040, "world +0x1040");
_Static_assert(offsetof(wl_world, view)      == 0x104c, "world view");
_Static_assert(offsetof(wl_world, f2770)     == 0x2770, "world +0x2770");
_Static_assert(offsetof(wl_world, list_2798) == 0x2798, "world +0x2798");
_Static_assert(offsetof(wl_world, busy)      == 0x27a8, "world +0x27a8");
_Static_assert(offsetof(wl_entity, f00c)     == 0x00c, "entity +0xc");
_Static_assert(offsetof(wl_entity, f03c)     == 0x03c, "entity +0x3c");
_Static_assert(offsetof(wl_entity, handle)   == 0x1d8, "entity +0x1d8");
_Static_assert(offsetof(wl_actor, steps)     == 0x208, "actor steps");
_Static_assert(offsetof(wl_actor, nsteps)    == 0x268, "actor step count");
_Static_assert(offsetof(wl_actor, goal_y)    == 0x280, "actor +0x280");
_Static_assert(offsetof(wl_actor, kind)      == 0x28c, "actor +0x28c");
_Static_assert(offsetof(wl_actor, link)      == 0x298, "actor +0x298");
_Static_assert(offsetof(wl_actor, area_x0)   == 0x2b4, "actor +0x2b4");
_Static_assert(offsetof(wl_actor, step)      == 0x2cc, "actor +0x2cc");
_Static_assert(sizeof(wl_actor)              == 0x2fc, "actor is 0x2fc bytes");
_Static_assert(offsetof(wl_entity, par_dy)   == 0x018, "entity +0x18");
_Static_assert(offsetof(wl_particle, life)   == 0x258, "particle life");
_Static_assert(offsetof(wl_particle, age)    == 0x26c, "particle age");
_Static_assert(offsetof(wl_particle, dir_y)  == 0x27c, "particle +0x27c");
_Static_assert(sizeof(wl_particle)           == 0x284, "particle is 0x284 bytes");
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
