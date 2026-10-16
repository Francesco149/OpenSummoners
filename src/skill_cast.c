/* skill_cast.c — FUN_0045a300 and its helpers.  See skill_cast.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (the decompile's control flow kept verbatim: labels, gotos across
 * the cases; IDA's local types, fields through width-typed accessors), then finished by hand against the asm.
 * Retail returns 1 when the cast took the body (the integrator then skips its own handlers). */
#include "skill_cast.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "char_handlers.h"   /* item_effect 0x5b3b00 */
#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }



#include "actor_update.h"
#include "char_attack.h"
#include "char_def.h"
#include "char_hit.h"
#include "char_init.h"
#include "char_integrate.h"
#include "char_state.h"
#include "char_status.h"
#include "entity_fx.h"
#include "particle_init.h"
#include "particle_spawn.h"
#include "sound_fx.h"
#pragma GCC optimize ("no-strict-aliasing")
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#define CFG() ((intptr_t)**(uint8_t ***)((uint8_t *)g_eng.sys + 0x320))
#define SYS32(o) F32(g_eng.sys, (o))
static inline int ofsub32_(int32_t a, int32_t b) { int32_t r; return __builtin_sub_overflow(a, b, &r); }
static inline int ofadd32_(int32_t a, int32_t b) { int32_t r; return __builtin_add_overflow(a, b, &r); }
#define __OFSUB__(a, b) ofsub32_((int32_t)(a), (int32_t)(b))
#define __OFADD__(a, b) ofadd32_((int32_t)(a), (int32_t)(b))
#define __CFADD__(a, b) ((uint32_t)(a) + (uint32_t)(b) < (uint32_t)(a))
#define __CFSUB__(a, b) ((uint32_t)(a) < (uint32_t)(b))
#define __SETS__(x) ((int32_t)(x) < 0)
static inline int32_t abs32(int32_t x) { return x < 0 ? (int32_t)(0u - (uint32_t)x) : x; }
#ifndef __cplusplus
#include <stdbool.h>
#endif
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))

static inline intptr_t W_body_action_reset(intptr_t a0) { body_action_reset((wl_body *)a0); return 0; }
static inline intptr_t W_body_action_start(intptr_t a0, intptr_t a1, intptr_t a2) { return (intptr_t)body_action_start((wl_body *)a0, (int16_t)a1, (int32_t)a2); }
static inline intptr_t W_body_set_clip(intptr_t a0, intptr_t a1) { body_set_clip((wl_body *)a0, (const void *)a1); return 0; }
static inline intptr_t W_char_die(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3) { char_die((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_char_init(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14) { char_init((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14); return 0; }
static inline intptr_t W_char_mag_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3) { return (intptr_t)char_mag_roll((const wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_char_mag_sum(intptr_t a0) { return (intptr_t)char_mag_sum((const wl_char *)a0); }
static inline intptr_t W_char_shot_fire(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13) { return (intptr_t)char_shot_fire((wl_char *)a0, (int32_t)a1, (wl_body *)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int16_t)a7, (int32_t)a8, (const uint8_t *)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_char_skill_roll(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3) { return (intptr_t)char_skill_roll((const wl_char *)a0, (const uint8_t *)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_entity_snd_keep(intptr_t a0, intptr_t a1, intptr_t a2) { entity_snd_keep((wl_entity *)a0, (uint16_t)a1, (intptr_t)a2); return 0; }
static inline intptr_t W_entity_snd_release(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3) { entity_snd_release((wl_entity *)a0, (uint16_t)a1, (intptr_t)a2, (int32_t)a3); return 0; }
static inline intptr_t W_entity_snd_start(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7) { return (intptr_t)entity_snd_start((wl_entity *)a0, (uint16_t)a1, (int32_t)a2, (uint16_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_particle_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13) { return (intptr_t)particle_spawn((wl_entity *)a0, (wl_entity *)a1, (int32_t)a2, (const wl_body *)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13); }
static inline intptr_t W_party_find(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4) { return (intptr_t)party_find((const wl_char *)a0, (int32_t)a1, (int16_t)a2, (uint16_t *)a3, (uint16_t)a4); }
static inline intptr_t W_sfx_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6) { return (intptr_t)sfx_place((const wl_entity *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (const wl_body *)a4, (int32_t *)a5, (int32_t *)a6); }
static inline intptr_t W_shot_spawn(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, intptr_t a13, intptr_t a14, intptr_t a15, intptr_t a16, intptr_t a17, intptr_t a18, intptr_t a19) { return (intptr_t)shot_spawn((void *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (wl_body *)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int16_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12, (int32_t)a13, (int32_t)a14, (int32_t)a15, (int32_t)a16, (int32_t)a17, (int32_t)a18, (int32_t)a19); }
static inline intptr_t W_speed_ramp(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3) { return (intptr_t)speed_ramp((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_stat_jitter(intptr_t a0, intptr_t a1, intptr_t a2) { return (intptr_t)stat_jitter((int32_t)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_tint_fade_a(intptr_t a0, intptr_t a1) { return (intptr_t)tint_fade_a((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_vline_stop(intptr_t a0, intptr_t a1) { vline_stop((wl_vline *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_voice_cue(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5) { return (intptr_t)voice_cue((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_voice_place(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3) { return (intptr_t)voice_place((const wl_char *)a0, (int32_t *)a1, (int32_t *)a2, (int32_t *)a3); }
static inline intptr_t W_voice_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6) { return (intptr_t)voice_play((wl_voiceset *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }
static inline intptr_t W_voiceset_sounding(intptr_t a0, intptr_t a1, intptr_t a2) { return (intptr_t)voiceset_sounding((const wl_voiceset *)a0, (wl_vgroup **)a1, (wl_vline **)a2); }
static inline intptr_t W_vpool_take(intptr_t a0, intptr_t a1) { return (intptr_t)vpool_take((wl_vpool *)a0, (int32_t)a1); }
int32_t skill_cast(wl_char *self, int32_t a2, int32_t errno_decorate, int32_t a4, int32_t a5, int32_t a6, int32_t * a7, uint16_t * a8, int32_t a9);
int32_t skill_charge(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t a6, uint16_t * a7, uint16_t a8, int32_t a9);
int32_t skill_stage(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t a6, uint16_t * a7, uint16_t a8, int16_t a9, int32_t a10);
int32_t skill_mp_pay(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t summon_recall(int32_t a1, int32_t a2);
int32_t summon_room_ok(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t skill_space_ahead(int32_t * a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6);
int32_t summon_init(wl_char *self, int32_t * a2);
int32_t summon_spawn(int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);

/* 0x45a300: int __thiscall sub_45A300( cls_4022A0 *this, int a2, int errno_decorate, int a4, int a5, int a6, _DWORD *a7, _WORD *a8, int a9) */
int32_t skill_cast(wl_char *self, int32_t a2, int32_t errno_decorate, int32_t a4, int32_t a5, int32_t a6, int32_t * a7, uint16_t * a8, int32_t a9)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t result;
    uint16_t * v16;
    int32_t * v17;
    intptr_t v18;
    intptr_t v19;
    intptr_t v20;
    uint16_t v21;
    intptr_t v22;
    intptr_t v23;
    uint16_t * v24;
    intptr_t v25;
    intptr_t v26;
    intptr_t v27;
    uint16_t v28;
    intptr_t v29;
    intptr_t v30;
    int32_t * v31;
    intptr_t * v32;
    intptr_t v33;
    intptr_t v34;
    int16_t v35;
    uint16_t * v36;
    intptr_t v37;
    intptr_t v38;
    intptr_t v39;
    intptr_t v40;
    int16_t v41;
    uint16_t * v42;
    intptr_t v43;
    intptr_t v44;
    intptr_t v45;
    intptr_t v46;
    int16_t v47;
    uint16_t * v48;
    intptr_t v49;
    intptr_t v50;
    intptr_t v51;
    intptr_t v52;
    intptr_t v53;
    intptr_t v54;
    intptr_t v55;
    intptr_t v56;
    intptr_t v57;
    uint16_t * v58;
    int32_t * v59;
    intptr_t v60;
    intptr_t v61;
    uint16_t v62;
    intptr_t v63;
    intptr_t v64;
    int32_t * v65;
    intptr_t v66;
    intptr_t v67;
    intptr_t v68;
    intptr_t v69;
    uint16_t * v70;
    int32_t * v71;
    intptr_t v72;
    intptr_t v73;
    uint16_t v74;
    intptr_t v75;
    intptr_t v76;
    int32_t * v77;
    int32_t * v78;
    int16_t v79;
    int32_t * v80;
    intptr_t v81;
    intptr_t v82;
    intptr_t v83;
    intptr_t v84;
    int16_t v85;
    int16_t v86;
    uint16_t * v87;
    intptr_t v88;
    intptr_t v89;
    uint16_t * v90;
    int32_t * v91;
    intptr_t v92;
    intptr_t v93;
    intptr_t v94;
    uint16_t v95;
    intptr_t v96;
    intptr_t v97;
    int32_t * v98;
    intptr_t * v99;
    intptr_t v100;
    int16_t i;
    uint16_t * v102;
    int32_t * v103;
    intptr_t v104;
    intptr_t v105;
    intptr_t v106;
    uint16_t * v107;
    int32_t * v108;
    intptr_t v109;
    intptr_t v110;
    intptr_t v111;
    intptr_t v112;
    intptr_t v113;
    uint16_t * v114;
    int32_t * v115;
    intptr_t v116;
    intptr_t v117;
    intptr_t v118;
    uint16_t v119;
    intptr_t v120;
    intptr_t v121;
    int32_t * v122;
    intptr_t * v123;
    intptr_t v124;
    int16_t j;
    uint16_t * v126;
    int32_t * v127;
    intptr_t v128;
    intptr_t v129;
    intptr_t v130;
    uint16_t * v131;
    int32_t * v132;
    intptr_t v133;
    intptr_t v134;
    intptr_t v135;
    int16_t v136;
    uint16_t * v137;
    intptr_t v138;
    intptr_t v139;
    intptr_t v140;
    intptr_t v141;
    intptr_t v142;
    uint16_t * v143;
    intptr_t v144;
    intptr_t v145;
    intptr_t v146;
    uint16_t v147;
    intptr_t v148;
    intptr_t v149;
    intptr_t v150;
    intptr_t v151;
    intptr_t v152;
    uint16_t * v153;
    intptr_t v154;
    intptr_t v155;
    uint16_t v156;
    intptr_t v157;
    intptr_t v158;
    uint16_t * v159;
    int32_t * v160;
    intptr_t v161;
    intptr_t v162;
    intptr_t v163;
    uint16_t v164;
    intptr_t v165;
    intptr_t v166;
    int32_t * v167;
    intptr_t * v168;
    intptr_t v169;
    uint16_t * v170;
    int32_t * v171;
    intptr_t v172;
    intptr_t v173;
    intptr_t v174;
    uint16_t v175;
    intptr_t v176;
    intptr_t v177;
    int16_t v178;
    int32_t * v179;
    int32_t * v180;
    intptr_t v181;
    intptr_t v182;
    intptr_t v183;
    int32_t * v184;
    int32_t * v185;
    intptr_t v186;
    intptr_t v187;
    intptr_t v188;
    int32_t * v189;
    int32_t * v190;
    intptr_t v191;
    intptr_t v192;
    intptr_t v193;
    intptr_t v194;
    intptr_t v195;
    intptr_t v196;
    int16_t v197;
    uint16_t * v198;
    intptr_t v199;
    intptr_t v200;
    intptr_t v201;
    int16_t v202;
    uint16_t * v203;
    intptr_t v204;
    intptr_t v205;
    intptr_t v206;
    int16_t v207;
    uint16_t * v208;
    intptr_t v209;
    intptr_t v210;
    int16_t v211;
    uint16_t * v212;
    intptr_t v213;
    intptr_t v214;
    int16_t v215;
    uint16_t * v216;
    intptr_t v217;
    intptr_t v218;
    intptr_t v219;
    intptr_t v220;
    int16_t v221;
    uint16_t * v222;
    intptr_t v223;
    intptr_t v224;
    int16_t v225;
    uint16_t * v226;
    intptr_t v227;
    intptr_t v228;
    intptr_t v229;
    intptr_t v230;
    int16_t v231;
    uint16_t * v232;
    intptr_t v233;
    intptr_t v234;
    intptr_t v235;
    intptr_t v236;
    intptr_t v237;
    intptr_t v238;
    intptr_t v239;
    intptr_t v240;
    intptr_t v241;
    intptr_t v242;
    intptr_t v243;
    intptr_t v244;
    intptr_t v245;
    intptr_t v246;
    intptr_t v247;
    intptr_t v248;
    intptr_t v249;
    intptr_t v250;
    intptr_t v251;
    intptr_t v252;
    intptr_t v253;
    intptr_t v254;
    intptr_t v255;
    intptr_t v256;
    intptr_t v257;
    intptr_t v258;
    intptr_t v259;
    intptr_t v260;
    intptr_t v261;
    intptr_t v262;
    intptr_t v263;
    int16_t v264;
    uint16_t v268;
    _Alignas(4) uint8_t frame_[0x54 + 0x1000];   /* the stack arrays at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v265 = (intptr_t *)(frame_ + 0x0);   /* [5] at ebp-0x54 */
    intptr_t *v266 = (intptr_t *)(frame_ + 0x14);   /* [8] at ebp-0x40 */
    intptr_t *v267 = (intptr_t *)(frame_ + 0x34);   /* [8] at ebp-0x20 */

  v267[0] = 10;
  v266[3] = 10;
  v266[7] = 10;
  v10 = F16(a2, 72);
  v12 = 0;
  v263 = 0;
  v267[1] = 50;
  v267[2] = 30;
  v267[3] = 60;
  v267[4] = 0;
  v267[5] = 40;
  v267[6] = 20;
  v267[7] = 80;
  v266[0] = 0;
  v266[1] = 20;
  v266[2] = 30;
  v266[4] = 20;
  v266[5] = 0;
  v266[6] = 30;
  v265[0] = 1000;
  v265[1] = 12000;
  v265[2] = 10000;
  v265[3] = 4000;
  v265[4] = 7000;
  if ( (unsigned int)v10 <= 0x4E90 )
  {
    if ( v10 == 20112 )
    {
LABEL_603:
      v221 = a9;
      v222 = a8;
      v223 = errno_decorate;
      F32(a4, 0) = 1;
      v263 = skill_stage(c, a2, v223, a5, a7, F32(a2, 76), v222, 0x3Cu, v221, 8);
      if ( v263 != 0 )
      {
        v224 = F16(a2, 88);
        if ( (uint16_t)v224 != 3 && v224 == 6 )
        {
          W_char_shot_fire(
            c,
            100007,
            a2,
            3200,
            0,
            4,
            (int)v223,
            0,
            90,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            0,
            3);
          W_char_shot_fire(
            c,
            100007,
            a2,
            -3200,
            0,
            4,
            (int)v223,
            0,
            90,
            F32(a2, 76),
            1000,
            F32(a2, 44) != 3 ? 3 : 1,
            0,
            3);
          W_voice_cue(c, (int)v223, 0x3Fu, 0, 0, 0);
        }
      }
      return v263;
    }
    if ( v10 <= 10114 )
    {
      if ( v10 != 10114 )
      {
        if ( v10 <= 130 )
        {
          if ( v10 != 130 )
          {
            switch ( v10 )
            {
              case 8:
                goto LABEL_16;
              case 9:
                if ( errno_decorate == 0 )
                {
                  v13 = F32(c, 0x15b54);
                  if ( v13 != 0 && W_voiceset_sounding(v13, &a9, &errno_decorate) != 0 )
                  {
                    v14 = F32(v13, 20);
                    if ( v14 >= 0 )
                    {
                      if ( v14 <= 1 )
                      {
                        W_vline_stop(errno_decorate, 1);
                      }
                      else if ( v14 == 2 )
                      {
                        F32(errno_decorate, 32) = 1;
                      }
                    }
                  }
                }
LABEL_16:
                F32(a4, 0) = 2;
                if ( F16(a2, 88) != 0 )
                {
                  if ( F16(a2, 88) != 1 )
                    return 0;
                }
                else
                {
                  if ( F32(a2, 108) != a8 + 1212 )
                  {
                    F32(a2, 108) = a8 + 1212;
                    F8(a2, 116) = 0;
                    F16(a2, 114) = 0;
                    F16(a2, 112) = 0;
                  }
                  F16(a2, 88) = 1;
                }
                if ( F8(a2, 116) == 0 )
                  return v263;
                F16(a2, 72) = 0;
                F16(a2, 86) = 210;
                F16(a2, 88) = 0;
                F32(a2, 92) = 0;
                F32(a2, 120) = 0;
                F16(a2, 84) = 0;
                F32(a2, 76) = 0;
                F32(a2, 80) = 0;
                F16(a2, 90) = 0;
                F16(a2, 100) = 0;
                result = 0;
                break;
              case 47:
                v41 = a9;
                v42 = a8;
                v43 = errno_decorate;
                F32(a4, 0) = 1;
                v263 = skill_stage(c, a2, v43, a5, a7, F32(a2, 76), v42, 0x50u, v41, 8);
                if ( v263 == 0 )
                  return v263;
                v44 = F16(a2, 88);
                if ( (uint16_t)v44 == 3 || v44 != 6 )
                  return v263;
                W_voice_cue(c, (int)v43, 0x38u, 0, 0, 0);
                v45 = W_char_shot_fire(
                        c,
                        100011,
                        a2,
                        0,
                        0,
                        1,
                        (int)v43,
                        0,
                        0,
                        F32(a2, 76),
                        1000,
                        F32(a2, 44),
                        0,
                        2);
                if ( v45 != 0 )
                  F32(v45, 516) = 10;
                v46 = W_char_shot_fire(
                        c,
                        100011,
                        a2,
                        0,
                        0,
                        1,
                        (int)v43,
                        2,
                        180,
                        F32(a2, 76),
                        1000,
                        F32(a2, 44),
                        1,
                        2);
                if ( v46 == 0 )
                  return v263;
                F32(v46, 516) = 20;
                return v263;
              case 48:
                v35 = a9;
                v36 = a8;
                v37 = errno_decorate;
                F32(a4, 0) = 1;
                v263 = skill_stage(c, a2, v37, a5, a7, F32(a2, 76), v36, 0x50u, v35, 8);
                if ( v263 == 0 )
                  return v263;
                v38 = F16(a2, 88);
                if ( (uint16_t)v38 == 3 || v38 != 6 )
                  return v263;
                v39 = 0;
                W_voice_cue(c, (int)v37, 0x37u, 0, 0, 0);
                W_char_shot_fire(
                  c,
                  100013,
                  a2,
                  0,
                  0,
                  1,
                  (int)v37,
                  0,
                  0,
                  F32(a2, 76),
                  1000,
                  F32(a2, 44),
                  0,
                  2);
                errno_decorate = 0;
                do
                {
                  v40 = W_char_shot_fire(
                          c,
                          100012,
                          a2,
                          0,
                          0,
                          1,
                          (int)v37,
                          0,
                          v39,
                          F32(a2, 76),
                          1000,
                          F32(a2, 44),
                          0,
                          2);
                  if ( v40 != 0 && (uint16_t)errno_decorate != 0 )
                    F32(v40, 504) = 1;
                  v39 += 15;
                  ++errno_decorate;
                }
                while ( (uint16_t)errno_decorate < 0x18u );
                return v263;
              case 127:
                v24 = a8;
                v25 = errno_decorate;
                F32(a4, 0) = 1;
                v26 = skill_charge(c, a2, (int)v25, a5, a7, F32(a2, 76), v24, 0x50u, 8);
                F16(a2, 86) = a9;
                if ( v26 != 0 )
                {
                  if ( F16(a2, 88) == 1 )
                  {
                    W_entity_snd_start(c, 0, (int)v25, 0x51u, 0, -500, 0, 1);
                  }
                  else if ( F16(a2, 88) == 2 )
                  {
                    W_entity_snd_start(c, 1u, (int)v25, 0x50u, 0, 0, 0, 1);
                  }
                }
                v27 = F16(a2, 88);
                if ( (uint16_t)v27 == 1 )
                  goto LABEL_70;
                if ( (uint16_t)v27 > 1u && F16(a2, 88) <= 6u )
                {
                  W_entity_snd_keep(c, 1u, v25);
LABEL_70:
                  W_entity_snd_keep(c, 0, v25);
                }
                v28 = F16(a2, 88);
                if ( v28 != 0 && v28 <= 6u )
                {
                  if ( F32(a2, 24) > 0 )
                    F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
                  v29 = F32(a2, 40);
                  if ( v29 > 0xAFC8 )
                    F32(a2, 40) = W_speed_ramp(v29, 800, -800, 0);
                }
                if ( v25 == 0
                  && F32(CFG(), 148) != 0
                  && F16(a2, 88) >= 2u
                  && F16(a2, 88) <= 3u
                  && F32(c, 0x15a60) != 0 )
                {
                  if ( F16(a2, 102) >= 0x320u )
                    W_particle_spawn(
                      c,
                      (int)c,
                      100095,
                      a2,
                      F32(c, 0x15a64),
                      F32(c, 0x15a68),
                      99,
                      1,
                      0,
                      0,
                      2,
                      1,
                      0,
                      0);
                  if ( F16(a2, 102) >= 0x1F4u )
                  {
                    if ( F32(c, 0xc828) == 0 )
                      F32(c, 0xc828) = W_particle_spawn(
                                           c,
                                           (int)c,
                                           100096,
                                           a2,
                                           F32(c, 0x15a64),
                                           F32(c, 0x15a68),
                                           99,
                                           1,
                                           0,
                                           0,
                                           2,
                                           1,
                                           0,
                                           10);
                    F32(c, 0xc830) = 1;
                  }
                }
                v263 = v26;
                if ( v26 != 0 )
                {
                  v30 = F16(a2, 88);
                  if ( (uint16_t)v30 != 3 && v30 == 6 && v25 == 0 )
                  {
                    v31 = F32(c, 0x40);
                    v32 = W_vpool_take(SYS32(1256), 0);
                    if ( v32 != 0 )
                    {
                      v33 = F32(v32, 0);
                      if ( W_sfx_place(
                             c,
                             F32(CFG(), 360),
                             0,
                             0,
                             v31,
                             &errno_decorate,
                             &a4) != 0 )
                      {
                        g_eng.dsb_volume_5bb870(v33, errno_decorate);
                        g_eng.dsb_pan_5bb880(v33, a4);
                        g_eng.dsb_freq_5bb890(v33, 0);
                        g_eng.dsb_play_5bb8c0(v33);
                      }
                    }
                    F32(c, 0x158b0) = 40;
                    F32(c, 0x158b8) = 40;
                    v34 = F32(c, 0x15b54);
                    F16(c, 0x158a8) = 1;
                    F32(c, 0x158ac) = 0;
                    F32(c, 0x158b4) = 0;
                    if ( v34 != 0
                      && F32(c, 0x15b5c) == 0
                      && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                      && W_voice_place(c, &a9, &a4, &errno_decorate) != 0 )
                    {
                      goto LABEL_100;
                    }
                  }
                }
                return v263;
              case 128:
                v16 = a8;
                v17 = a7;
                v18 = errno_decorate;
                F32(a4, 0) = 1;
                v19 = skill_charge(c, a2, (int)v18, a5, v17, F32(a2, 76), v16, 0x50u, 8);
                errno_decorate = v19;
                F16(a2, 86) = a9;
                if ( v19 != 0 )
                {
                  if ( F16(a2, 88) == 1 )
                  {
                    W_entity_snd_start(c, 0, (int)v18, 0x51u, 0, -500, 0, 1);
                  }
                  else if ( F16(a2, 88) == 2 )
                  {
                    W_entity_snd_start(c, 1u, (int)v18, 0x50u, 0, 0, 0, 1);
                  }
                }
                v20 = F16(a2, 88);
                if ( (uint16_t)v20 == 1 )
                  goto LABEL_33;
                if ( (uint16_t)v20 > 1u && F16(a2, 88) <= 6u )
                {
                  W_entity_snd_keep(c, 1u, v18);
LABEL_33:
                  W_entity_snd_keep(c, 0, v18);
                }
                v21 = F16(a2, 88);
                if ( v21 != 0 && v21 <= 6u )
                {
                  if ( F32(a2, 24) > 0 )
                    F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
                  v22 = F32(a2, 40);
                  if ( v22 > 0xAFC8 )
                    F32(a2, 40) = W_speed_ramp(v22, 800, -800, 0);
                }
                if ( v18 == 0
                  && F32(CFG(), 148) != 0
                  && F16(a2, 88) >= 2u
                  && F16(a2, 88) <= 3u
                  && F32(c, 0x15a60) != 0 )
                {
                  if ( F16(a2, 102) >= 0x320u )
                    W_particle_spawn(
                      c,
                      (int)c,
                      100095,
                      a2,
                      F32(c, 0x15a64),
                      F32(c, 0x15a68),
                      99,
                      1,
                      0,
                      0,
                      2,
                      1,
                      0,
                      0);
                  if ( F16(a2, 102) >= 0x1F4u )
                  {
                    if ( F32(c, 0xc828) == 0 )
                      F32(c, 0xc828) = W_particle_spawn(
                                           c,
                                           (int)c,
                                           100096,
                                           a2,
                                           F32(c, 0x15a64),
                                           F32(c, 0x15a68),
                                           99,
                                           1,
                                           0,
                                           0,
                                           2,
                                           1,
                                           0,
                                           10);
                    F32(c, 0xc830) = 1;
                  }
                }
                v263 = errno_decorate;
                if ( errno_decorate == 0 )
                  return v263;
                v23 = F16(a2, 88);
                if ( (uint16_t)v23 == 3
                  || v23 != 6
                  || v18 != 0
                  || F32(c, 0x15b54) == 0
                  || F32(c, 0x15b5c) != 0
                  || F32(c, 0x15a8c) == 1 && F32(CFG(), 400) == 0 )
                {
                  return v263;
                }
                if ( W_voice_place(c, &a9, &a4, &errno_decorate) == 0 )
                  return v263;
                W_voice_play(F32(c, 0x15b54), 0x28u, 0, a9, a4, errno_decorate, 0);
                return v263;
              default:
                return v263;
            }
            return result;
          }
          v12 = 1;
LABEL_118:
          v47 = a9;
          v48 = a8;
          v49 = errno_decorate;
          F32(a4, 0) = 1;
          v263 = skill_stage(c, a2, v49, a5, a7, F32(a2, 76), v48, 0x50u, v47, 8);
          if ( v263 != 0 )
          {
            v50 = F16(a2, 88);
            if ( (uint16_t)v50 != 3 && v50 == 6 )
            {
              v51 = W_char_shot_fire(
                      c,
                      100008,
                      a2,
                      0,
                      0,
                      1,
                      (int)v49,
                      0,
                      0,
                      F32(a2, 76),
                      1000,
                      F32(a2, 44),
                      0,
                      2);
              if ( v51 != 0 )
              {
                F32(v51, 516) = 10;
                if ( v12 != 0 )
                  F32(v51, 576) = F32(c, 0x15ba0);
              }
              v52 = W_char_shot_fire(
                      c,
                      100008,
                      a2,
                      0,
                      0,
                      1,
                      (int)v49,
                      2,
                      180,
                      F32(a2, 76),
                      1000,
                      F32(a2, 44),
                      1,
                      2);
              if ( v52 != 0 )
              {
                F32(v52, 516) = 20;
                if ( v12 != 0 )
                  F32(v52, 576) = F32(c, 0x15ba0);
              }
              v53 = W_char_shot_fire(
                      c,
                      100008,
                      a2,
                      0,
                      0,
                      1,
                      (int)v49,
                      4,
                      60,
                      F32(a2, 76),
                      1000,
                      F32(a2, 44),
                      1,
                      2);
              if ( v53 != 0 )
              {
                F32(v53, 516) = 30;
                if ( v12 != 0 )
                  F32(v53, 576) = F32(c, 0x15ba0);
              }
              v54 = W_char_shot_fire(
                      c,
                      100008,
                      a2,
                      0,
                      0,
                      1,
                      (int)v49,
                      6,
                      120,
                      F32(a2, 76),
                      1000,
                      F32(a2, 44),
                      1,
                      2);
              if ( v54 != 0 )
              {
                F32(v54, 516) = 40;
                if ( v12 != 0 )
                  F32(v54, 576) = F32(c, 0x15ba0);
              }
              W_voice_cue(c, (int)v49, 0x3Cu, 0, 0, 0);
            }
          }
          return v263;
        }
        if ( v10 <= 139 )
        {
          if ( v10 != 139 )
          {
            switch ( v10 )
            {
              case 134:
              case 135:
                v55 = errno_decorate;
                v56 = F32(a2, 76);
                if ( errno_decorate == 0 && F16(a2, 88) != 0 && F16(a2, 88) <= 3u && v56 != 0 )
                {
                  v57 = F32(v56, 184) - 1;
                  if ( v57 == 0 || v57 == 2 )
                    F32(c, 0xc788) = 1;
                }
                v58 = a8;
                v59 = a7;
                F32(a4, 0) = 1;
                v60 = skill_charge(c, a2, (int)v55, a5, v59, F32(a2, 76), v58, 0x50u, 8);
                F16(a2, 86) = a9;
                if ( v60 != 0 )
                {
                  if ( F16(a2, 88) == 1 )
                  {
                    W_entity_snd_start(c, 0, (int)v55, 0x51u, 0, -500, 0, 1);
                  }
                  else if ( F16(a2, 88) == 2 )
                  {
                    W_entity_snd_start(c, 1u, (int)v55, 0x50u, 0, 0, 0, 1);
                  }
                }
                v61 = F16(a2, 88);
                if ( (uint16_t)v61 == 1 )
                  goto LABEL_153;
                if ( (uint16_t)v61 > 1u && F16(a2, 88) <= 6u )
                {
                  W_entity_snd_keep(c, 1u, v55);
LABEL_153:
                  W_entity_snd_keep(c, 0, v55);
                }
                v62 = F16(a2, 88);
                if ( v62 != 0 && v62 <= 6u )
                {
                  if ( F32(a2, 24) > 0 )
                    F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
                  v63 = F32(a2, 40);
                  if ( v63 > 0xAFC8 )
                    F32(a2, 40) = W_speed_ramp(v63, 800, -800, 0);
                }
                if ( v55 == 0
                  && F32(CFG(), 148) != 0
                  && F16(a2, 88) >= 2u
                  && F16(a2, 88) <= 3u
                  && F32(c, 0x15a60) != 0 )
                {
                  if ( F16(a2, 102) >= 0x320u )
                    W_particle_spawn(
                      c,
                      (int)c,
                      100095,
                      a2,
                      F32(c, 0x15a64),
                      F32(c, 0x15a68),
                      99,
                      1,
                      0,
                      0,
                      2,
                      1,
                      0,
                      0);
                  if ( F16(a2, 102) >= 0x1F4u )
                  {
                    if ( F32(c, 0xc828) == 0 )
                      F32(c, 0xc828) = W_particle_spawn(
                                           c,
                                           (int)c,
                                           100096,
                                           a2,
                                           F32(c, 0x15a64),
                                           F32(c, 0x15a68),
                                           99,
                                           1,
                                           0,
                                           0,
                                           2,
                                           1,
                                           0,
                                           10);
                    F32(c, 0xc830) = 1;
                  }
                }
                v263 = v60;
                if ( v60 == 0 )
                  return v263;
                v64 = F16(a2, 88);
                if ( (uint16_t)v64 == 3 || v64 != 6 )
                  return v263;
                if ( v55 == 0
                  && F32(c, 0x15b54) != 0
                  && F32(c, 0x15b5c) == 0
                  && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                  && W_voice_place(c, &a9, &a4, &errno_decorate) != 0 )
                {
                  W_voice_play(F32(c, 0x15b54), 0x39u, 0, a9, a4, errno_decorate, 0);
                }
                v65 = F32(a2, 76);
                if ( v55 != 0 )
                  return v263;
                if ( !summon_room_ok(c, (int)c, v65[177], v65[178], v65[179]) )
                {
                  if ( F32(c, 0xc794) == 0 )
                    return v263;
                  while ( !summon_room_ok(c, (int)c, v65[177], v65[178], v65[179])
                       && (summon_recall((int)c, v65[177]) != 0 || summon_recall((int)c, 0) != 0) )
                    ;
                }
                if ( skill_space_ahead(a2, 9600, 0, 6400, 6400, &errno_decorate) == 0 )
                {
                  if ( F32(c, 0xc794) == 0 )
                    return v263;
                  errno_decorate = 0;
                }
                v66 = summon_spawn(
                                      v65[177],
                                      a2,
                                      errno_decorate,
                                      0,
                                      4,
                                      0,
                                      F32(a2, 44),
                                      5,
                                      1);
                goto LABEL_488;
              case 136:
              case 137:
              case 138:
                v67 = errno_decorate;
                v68 = F32(a2, 76);
                if ( errno_decorate == 0 && F16(a2, 88) != 0 && F16(a2, 88) <= 3u && v68 != 0 )
                {
                  v69 = F32(v68, 184) - 1;
                  if ( v69 == 0 || v69 == 2 )
                    F32(c, 0xc788) = 1;
                }
                v70 = a8;
                v71 = a7;
                F32(a4, 0) = 1;
                v72 = skill_charge(c, a2, (int)v67, a5, v71, F32(a2, 76), v70, 0x50u, 8);
                F16(a2, 86) = a9;
                if ( v72 != 0 )
                {
                  if ( F16(a2, 88) == 1 )
                  {
                    W_entity_snd_start(c, 0, (int)v67, 0x51u, 0, -500, 0, 1);
                  }
                  else if ( F16(a2, 88) == 2 )
                  {
                    W_entity_snd_start(c, 1u, (int)v67, 0x50u, 0, 0, 0, 1);
                  }
                }
                v73 = F16(a2, 88);
                if ( (uint16_t)v73 == 1 )
                  goto LABEL_207;
                if ( (uint16_t)v73 > 1u && F16(a2, 88) <= 6u )
                {
                  W_entity_snd_keep(c, 1u, v67);
LABEL_207:
                  W_entity_snd_keep(c, 0, v67);
                }
                v74 = F16(a2, 88);
                if ( v74 != 0 && v74 <= 6u )
                {
                  if ( F32(a2, 24) > 0 )
                    F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
                  v75 = F32(a2, 40);
                  if ( v75 > 0xAFC8 )
                    F32(a2, 40) = W_speed_ramp(v75, 800, -800, 0);
                }
                if ( v67 == 0
                  && F32(CFG(), 148) != 0
                  && F16(a2, 88) >= 2u
                  && F16(a2, 88) <= 3u
                  && F32(c, 0x15a60) != 0 )
                {
                  if ( F16(a2, 102) >= 0x320u )
                    W_particle_spawn(
                      c,
                      (int)c,
                      100095,
                      a2,
                      F32(c, 0x15a64),
                      F32(c, 0x15a68),
                      99,
                      1,
                      0,
                      0,
                      2,
                      1,
                      0,
                      0);
                  if ( F16(a2, 102) >= 0x1F4u )
                  {
                    if ( F32(c, 0xc828) == 0 )
                      F32(c, 0xc828) = W_particle_spawn(
                                           c,
                                           (int)c,
                                           100096,
                                           a2,
                                           F32(c, 0x15a64),
                                           F32(c, 0x15a68),
                                           99,
                                           1,
                                           0,
                                           0,
                                           2,
                                           1,
                                           0,
                                           10);
                    F32(c, 0xc830) = 1;
                  }
                }
                v263 = v72;
                if ( v72 == 0 )
                  return v263;
                v76 = F16(a2, 88);
                if ( (uint16_t)v76 == 3 || v76 != 6 )
                  return v263;
                if ( v67 == 0
                  && F32(c, 0x15b54) != 0
                  && F32(c, 0x15b5c) == 0
                  && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                  && W_voice_place(c, &a9, &a4, &errno_decorate) != 0 )
                {
                  W_voice_play(F32(c, 0x15b54), 0x39u, 0, a9, a4, errno_decorate, 0);
                }
                v77 = F32(a2, 76);
                if ( v67 != 0 )
                  return v263;
                if ( !summon_room_ok(c, (int)c, v77[177], v77[178], v77[179]) )
                {
                  if ( F32(c, 0xc794) == 0 )
                    return v263;
                  while ( !summon_room_ok(c, (int)c, v77[177], v77[178], v77[179])
                       && (summon_recall((int)c, v77[177]) != 0 || summon_recall((int)c, 0) != 0) )
                    ;
                }
                if ( skill_space_ahead(a2, 9600, 0, 12800, 6400, &errno_decorate) != 0 )
                  goto LABEL_487;
                if ( F32(c, 0xc794) == 0 )
                  return v263;
                errno_decorate = 0;
                goto LABEL_487;
              default:
                return v263;
            }
          }
LABEL_595:
          v215 = a9;
          v216 = a8;
          v217 = errno_decorate;
          F32(a4, 0) = 1;
          v263 = skill_stage(c, a2, v217, a5, a7, F32(a2, 76), v216, 0x3Cu, v215, 8);
          if ( v263 != 0 )
          {
            v218 = F16(a2, 88);
            if ( (uint16_t)v218 != 3 && v218 == 6 )
            {
              v219 = 0;
              errno_decorate = 64;
              do
              {
                v220 = W_char_shot_fire(
                         c,
                         100017,
                         a2,
                         3200,
                         -6400,
                         4,
                         (int)v217,
                         v219,
                         90,
                         F32(a2, 76),
                         1000,
                         F32(a2, 44),
                         0,
                         2);
                if ( v220 != 0 )
                  F32(v220, 516) = errno_decorate;
                v219 += 5;
                errno_decorate -= 8;
              }
              while ( (uint16_t)v219 < 0x28u );
              W_voice_cue(c, (int)v217, 0x40u, 0, 0, 0);
            }
          }
          return v263;
        }
        if ( v10 != 140 )
        {
          if ( v10 == 10113 )
          {
            if ( W_party_find(c, 50013, 0, 0, 0) == 0 )
            {
              W_body_action_reset(a2);
              return 0;
            }
            v78 = a4;
            v79 = a9;
            v80 = a7;
            v81 = errno_decorate;
            F32(a4, 0) = 1;
            v263 = skill_stage(c, a2, v81, a5, v80, F32(a2, 76), D(0x630378), 0x3Cu, v79, 9);
            if ( v263 != 0 )
            {
              v82 = F16(a2, 88);
              if ( (uint16_t)v82 == 3 )
              {
                W_voice_cue(c, (int)v81, 0x41u, 0, 0, 0);
              }
              else if ( v82 == 6 )
              {
                W_voice_cue(c, (int)v81, 0x42u, 1, 0, 0);
                item_effect((wl_char *)(intptr_t)(c), (int32_t)(a2), (int32_t)((int)v81), (uint16_t *)(intptr_t)(F32(a2, 80)), (int32_t)(F32(a2, 76)), (int32_t)(0), (int16_t)(0));
                if ( F32(c, 0x200) == 0 && v81 == 0 )
                  F32(W + 0x2774, 8) = 1;
              }
            }
            if ( F16(a2, 88) >= 5u )
            {
              F8(a2, 125) = 1;
              F32(a2, 28) = 1000;
              F32(a2, 40) = 5000;
              *v78 = 3;
              return v263;
            }
          }
          return v263;
        }
LABEL_378:
        v113 = 0;
        v268 = 41;
LABEL_386:
        v140 = errno_decorate;
        v141 = F32(a2, 76);
        if ( errno_decorate == 0 && F16(a2, 88) != 0 && F16(a2, 88) <= 3u && v141 != 0 )
        {
          v142 = F32(v141, 184) - 1;
          if ( v142 == 0 || v142 == 2 )
            F32(c, 0xc788) = 1;
        }
        v143 = a8;
        F32(a4, 0) = 1;
        v144 = skill_charge(c, a2, (int)v140, a5, a7, F32(a2, 76), v143, 0x50u, 8);
        errno_decorate = v144;
        F16(a2, 86) = a9;
        if ( v144 != 0 )
        {
          if ( F16(a2, 88) == 1 )
          {
            W_entity_snd_start(c, 0, (int)v140, 0x51u, 0, -500, 0, 1);
          }
          else if ( F16(a2, 88) == 2 )
          {
            W_entity_snd_start(c, 1u, (int)v140, 0x50u, 0, 0, 0, 1);
          }
        }
        v145 = F16(a2, 88);
        if ( (uint16_t)v145 != 1 )
        {
          if ( (uint16_t)v145 <= 1u || F16(a2, 88) > 6u )
          {
LABEL_407:
            v147 = F16(a2, 88);
            if ( v147 != 0 && v147 <= 6u )
            {
              if ( F32(a2, 24) > 0 )
                F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
              v148 = F32(a2, 40);
              if ( v148 > 0xAFC8 )
                F32(a2, 40) = W_speed_ramp(v148, 800, -800, 0);
            }
            if ( v140 == 0
              && F32(CFG(), 148) != 0
              && F16(a2, 88) >= 2u
              && F16(a2, 88) <= 3u
              && F32(c, 0x15a60) != 0 )
            {
              if ( F16(a2, 102) >= 0x320u )
                W_particle_spawn(
                  c,
                  (int)c,
                  100095,
                  a2,
                  F32(c, 0x15a64),
                  F32(c, 0x15a68),
                  99,
                  1,
                  0,
                  0,
                  2,
                  1,
                  0,
                  0);
              if ( F16(a2, 102) >= 0x1F4u )
              {
                if ( F32(c, 0xc828) == 0 )
                  F32(c, 0xc828) = W_particle_spawn(
                                       c,
                                       (int)c,
                                       100096,
                                       a2,
                                       F32(c, 0x15a64),
                                       F32(c, 0x15a68),
                                       99,
                                       1,
                                       0,
                                       0,
                                       2,
                                       1,
                                       0,
                                       10);
                F32(c, 0xc830) = 1;
              }
            }
            v263 = errno_decorate;
            if ( errno_decorate != 0 )
            {
              v149 = F16(a2, 88);
              if ( (uint16_t)v149 != 3 && v149 == 6 )
              {
                item_effect((wl_char *)(intptr_t)(c), (int32_t)(a2), (int32_t)((int)v140), (uint16_t *)(intptr_t)(F32(a2, 80)), (int32_t)(F32(a2, 76)), (int32_t)(v113), (int16_t)(v264));
                if ( v140 == 0
                  && F32(c, 0x15b54) != 0
                  && F32(c, 0x15b5c) == 0
                  && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
                  && W_voice_place(c, &a9, &a4, &errno_decorate) != 0 )
                {
                  W_voice_play(F32(c, 0x15b54), v268, 0, a9, a4, errno_decorate, 0);
                  return v263;
                }
              }
            }
            return v263;
          }
          if ( v140 == 0 )
          {
            v146 = F32(c, 0x1b0);
            if ( v146 != 0 )
            {
              if ( F16(c, 0x1b4) > 1u )
                F32(v146, 56) = 1;
              else
                g_eng.h408b90(D(0x8a9530), D(0x85525c), 0, D(0x8a9b6c));
            }
          }
        }
        W_entity_snd_keep(c, 0, v140);
        goto LABEL_407;
      }
LABEL_267:
      v86 = a9;
      v87 = a8;
      v88 = errno_decorate;
      F32(a4, 0) = 1;
      v263 = skill_stage(c, a2, v88, a5, a7, F32(a2, 76), v87, 0x50u, v86, 8);
      if ( v263 != 0 )
      {
        v89 = F16(a2, 88);
        if ( (uint16_t)v89 == 3 )
        {
          W_voice_cue(c, (int)v88, 0x35u, 0, 0, 0);
        }
        else if ( v89 == 6 )
        {
          W_voice_cue(c, (int)v88, 0x36u, 1, 0, 0);
          W_char_shot_fire(
            c,
            100005,
            a2,
            -3200,
            -9600,
            4,
            (int)v88,
            0,
            90,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            0,
            2);
          return v263;
        }
      }
      return v263;
    }
    if ( v10 > 20106 )
    {
      switch ( v10 )
      {
        case 20107:
          v113 = 0;
          v268 = 38;
          goto LABEL_386;
        case 20108:
          v85 = 2;
          goto LABEL_368;
        case 20109:
          v85 = 3;
          goto LABEL_368;
        case 20110:
          v113 = 0;
          v268 = 49;
          goto LABEL_386;
        case 20111:
          goto LABEL_377;
      }
    }
    if ( v10 == 20106 )
    {
LABEL_384:
      v113 = 100150;
      v264 = 30;
      v268 = 32;
      goto LABEL_386;
    }
    if ( v10 <= 20103 )
    {
      if ( v10 != 20103 )
      {
        v83 = v10 - 10121;
        if ( v83 == 0 )
          goto LABEL_267;
        v84 = v83 - 9980;
        if ( v84 == 0 )
        {
          v85 = 1;
LABEL_368:
          v136 = a9;
          v137 = a8;
          v138 = errno_decorate;
          F32(a4, 0) = 1;
          v263 = skill_stage(c, a2, v138, a5, a7, F32(a2, 76), v137, 0x50u, v136, 8);
          if ( v263 != 0 )
          {
            v139 = F16(a2, 88);
            if ( (uint16_t)v139 != 3 && v139 == 6 )
            {
              W_char_shot_fire(
                c,
                100003,
                a2,
                6000,
                -11000,
                1,
                (int)v138,
                10,
                125,
                F32(a2, 76),
                1000,
                F32(a2, 44),
                0,
                2);
              W_char_shot_fire(
                c,
                100003,
                a2,
                0,
                -13000,
                1,
                (int)v138,
                20,
                125,
                F32(a2, 76),
                1000,
                F32(a2, 44),
                1,
                2);
              W_char_shot_fire(
                c,
                100003,
                a2,
                -6000,
                -10000,
                1,
                (int)v138,
                30,
                125,
                F32(a2, 76),
                1000,
                F32(a2, 44),
                1,
                2);
              if ( (uint16_t)v85 >= 2u )
                W_char_shot_fire(
                  c,
                  100003,
                  a2,
                  12000,
                  -11000,
                  1,
                  (int)v138,
                  40,
                  125,
                  F32(a2, 76),
                  1000,
                  F32(a2, 44),
                  1,
                  2);
              if ( (uint16_t)v85 >= 3u )
                W_char_shot_fire(
                  c,
                  100003,
                  a2,
                  6000,
                  -20000,
                  1,
                  (int)v138,
                  50,
                  125,
                  F32(a2, 76),
                  1000,
                  F32(a2, 44),
                  1,
                  2);
              W_voice_cue(c, (int)v138, 0x21u, 0, 0, 0);
            }
          }
          return v263;
        }
        if ( v84 != 1 )
          return 0;
        goto LABEL_384;
      }
      v90 = a8;
      v91 = a7;
      v92 = errno_decorate;
      F32(a4, 0) = 1;
      v93 = skill_charge(c, a2, (int)v92, a5, v91, F32(a2, 76), v90, 0x50u, 8);
      F16(a2, 86) = a9;
      if ( v93 != 0 )
      {
        if ( F16(a2, 88) == 1 )
        {
          W_entity_snd_start(c, 0, (int)v92, 0x51u, 0, -500, 0, 1);
        }
        else if ( F16(a2, 88) == 2 )
        {
          W_entity_snd_start(c, 1u, (int)v92, 0x50u, 0, 0, 0, 1);
        }
      }
      v94 = F16(a2, 88);
      if ( (uint16_t)v94 != 1 )
      {
        if ( (uint16_t)v94 <= 1u || F16(a2, 88) > 6u )
          goto LABEL_282;
        W_entity_snd_keep(c, 1u, v92);
      }
      W_entity_snd_keep(c, 0, v92);
LABEL_282:
      v95 = F16(a2, 88);
      if ( v95 != 0 && v95 <= 6u )
      {
        if ( F32(a2, 24) > 0 )
          F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
        v96 = F32(a2, 40);
        if ( v96 > 0xAFC8 )
          F32(a2, 40) = W_speed_ramp(v96, 800, -800, 0);
      }
      if ( v92 == 0
        && F32(CFG(), 148) != 0
        && F16(a2, 88) >= 2u
        && F16(a2, 88) <= 3u
        && F32(c, 0x15a60) != 0 )
      {
        if ( F16(a2, 102) >= 0x320u )
          W_particle_spawn(
            c,
            (int)c,
            100095,
            a2,
            F32(c, 0x15a64),
            F32(c, 0x15a68),
            99,
            1,
            0,
            0,
            2,
            1,
            0,
            0);
        if ( F16(a2, 102) >= 0x1F4u )
        {
          if ( F32(c, 0xc828) == 0 )
            F32(c, 0xc828) = W_particle_spawn(
                                 c,
                                 (int)c,
                                 100096,
                                 a2,
                                 F32(c, 0x15a64),
                                 F32(c, 0x15a68),
                                 99,
                                 1,
                                 0,
                                 0,
                                 2,
                                 1,
                                 0,
                                 10);
          F32(c, 0xc830) = 1;
        }
      }
      v263 = v93;
      if ( v93 != 0 )
      {
        v97 = F16(a2, 88);
        if ( (uint16_t)v97 != 3 && v97 == 6 )
        {
          v98 = F32(c, 0x40);
          if ( v92 == 0 )
          {
            v99 = W_vpool_take(SYS32(1248), 0);
            if ( v99 != 0 )
            {
              v100 = F32(v99, 0);
              if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v98, &a4, &a9) != 0 )
              {
                g_eng.dsb_volume_5bb870(v100, a4);
                g_eng.dsb_pan_5bb880(v100, a9);
                g_eng.dsb_freq_5bb890(v100, 0);
                g_eng.dsb_play_5bb8c0(v100);
              }
            }
          }
          for ( i = 10; (uint16_t)i < 0x5Au; i += 10 )
          {
            v102 = F32(a2, 96);
            v103 = F32(a2, 76);
            a9 = F32(a2, 44);
            a4 = 0;
            F8(a2, 127) = 1;
            a8 = v102;
            v104 = rnd();
            a7 = W_char_skill_roll(c, v103, 1000, 1000 * v104 / 0x8000);
            if ( v103[44] == 7 )
            {
              v256 = v103[180];
              v105 = rnd();
              a4 = W_char_mag_roll(c, v103[179], 1000 * v105 / 0x8000, v256);
            }
            v249 = F32(F32(c, 0x750), 216)
                 + F32(F32(c, 0x750), 224)
                 + F32(F32(c, 0x750), 228);
            v242 = F32(c, 0x1f0);
            v235 = a4;
            v106 = W_char_mag_sum(c);
            W_shot_spawn(
              c,
              (int)c,
              F32(c, 0x1dc),
              100018,
              a2,
              2000,
              0,
              1,
              errno_decorate,
              i,
              (int)a7,
              90,
              (int)a8,
              a9,
              v103[44],
              v103[45] + v106,
              v235,
              v242,
              v249,
              2);
            v107 = F32(a2, 96);
            v108 = F32(a2, 76);
            a9 = F32(a2, 44);
            a4 = 0;
            F8(a2, 127) = 1;
            a8 = v107;
            v109 = rnd();
            a7 = W_char_skill_roll(c, v108, 1000, 1000 * v109 / 0x8000);
            if ( v108[44] == 7 )
            {
              v257 = v108[180];
              v110 = rnd();
              a4 = W_char_mag_roll(c, v108[179], 1000 * v110 / 0x8000, v257);
            }
            v250 = F32(F32(c, 0x750), 216)
                 + F32(F32(c, 0x750), 224)
                 + F32(F32(c, 0x750), 228);
            v243 = F32(c, 0x1f0);
            v236 = a4;
            v111 = W_char_mag_sum(c);
            W_shot_spawn(
              c,
              (int)c,
              F32(c, 0x1dc),
              100018,
              a2,
              -2000,
              0,
              1,
              errno_decorate,
              i,
              (int)a7,
              270,
              (int)a8,
              a9,
              v108[44],
              v108[45] + v111,
              v236,
              v243,
              v250,
              2);
          }
          W_voice_cue(c, errno_decorate, 0x33u, 0, 0, 0);
        }
      }
      return v263;
    }
    v112 = v10 - 20104;
    if ( v112 != 0 )
    {
      if ( v112 != 1 )
        return v263;
      v113 = 100153;
      v264 = 20;
      v268 = 36;
      goto LABEL_386;
    }
    v114 = a8;
    v115 = a7;
    v116 = errno_decorate;
    F32(a4, 0) = 1;
    v117 = skill_charge(c, a2, (int)v116, a5, v115, F32(a2, 76), v114, 0x50u, 8);
    F16(a2, 86) = a9;
    if ( v117 != 0 )
    {
      if ( F16(a2, 88) == 1 )
      {
        W_entity_snd_start(c, 0, (int)v116, 0x51u, 0, -500, 0, 1);
      }
      else if ( F16(a2, 88) == 2 )
      {
        W_entity_snd_start(c, 1u, (int)v116, 0x50u, 0, 0, 0, 1);
      }
    }
    v118 = F16(a2, 88);
    if ( (uint16_t)v118 != 1 )
    {
      if ( (uint16_t)v118 <= 1u || F16(a2, 88) > 6u )
      {
LABEL_326:
        v119 = F16(a2, 88);
        if ( v119 != 0 && v119 <= 6u )
        {
          if ( F32(a2, 24) > 0 )
            F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
          v120 = F32(a2, 40);
          if ( v120 > 0xAFC8 )
            F32(a2, 40) = W_speed_ramp(v120, 800, -800, 0);
        }
        if ( v116 == 0
          && F32(CFG(), 148) != 0
          && F16(a2, 88) >= 2u
          && F16(a2, 88) <= 3u
          && F32(c, 0x15a60) != 0 )
        {
          if ( F16(a2, 102) >= 0x320u )
            W_particle_spawn(
              c,
              (int)c,
              100095,
              a2,
              F32(c, 0x15a64),
              F32(c, 0x15a68),
              99,
              1,
              0,
              0,
              2,
              1,
              0,
              0);
          if ( F16(a2, 102) >= 0x1F4u )
          {
            if ( F32(c, 0xc828) == 0 )
              F32(c, 0xc828) = W_particle_spawn(
                                   c,
                                   (int)c,
                                   100096,
                                   a2,
                                   F32(c, 0x15a64),
                                   F32(c, 0x15a68),
                                   99,
                                   1,
                                   0,
                                   0,
                                   2,
                                   1,
                                   0,
                                   10);
            F32(c, 0xc830) = 1;
          }
        }
        v263 = v117;
        if ( v117 != 0 )
        {
          v121 = F16(a2, 88);
          if ( (uint16_t)v121 != 3 && v121 == 6 )
          {
            v122 = F32(c, 0x40);
            if ( v116 == 0 )
            {
              v123 = W_vpool_take(SYS32(1248), 0);
              if ( v123 != 0 )
              {
                v124 = F32(v123, 0);
                if ( W_sfx_place(c, F32(CFG(), 360), 0, 0, v122, &a4, &a9) != 0 )
                {
                  g_eng.dsb_volume_5bb870(v124, a4);
                  g_eng.dsb_pan_5bb880(v124, a9);
                  g_eng.dsb_freq_5bb890(v124, 0);
                  g_eng.dsb_play_5bb8c0(v124);
                }
              }
            }
            for ( j = 10; (uint16_t)j < 0x5Au; j += 10 )
            {
              v126 = F32(a2, 96);
              v127 = F32(a2, 76);
              a9 = F32(a2, 44);
              a4 = 0;
              F8(a2, 127) = 1;
              a8 = v126;
              v128 = rnd();
              a7 = W_char_skill_roll(c, v127, 1000, 1000 * v128 / 0x8000);
              if ( v127[44] == 7 )
              {
                v258 = v127[180];
                v129 = rnd();
                a4 = W_char_mag_roll(c, v127[179], 1000 * v129 / 0x8000, v258);
              }
              v251 = F32(F32(c, 0x750), 216)
                   + F32(F32(c, 0x750), 224)
                   + F32(F32(c, 0x750), 228);
              v244 = F32(c, 0x1f0);
              v237 = a4;
              v130 = W_char_mag_sum(c);
              W_shot_spawn(
                c,
                (int)c,
                F32(c, 0x1dc),
                100020,
                a2,
                2000,
                0,
                1,
                errno_decorate,
                j,
                (int)a7,
                90,
                (int)a8,
                a9,
                v127[44],
                v127[45] + v130,
                v237,
                v244,
                v251,
                2);
              v131 = F32(a2, 96);
              v132 = F32(a2, 76);
              a9 = F32(a2, 44);
              a4 = 0;
              F8(a2, 127) = 1;
              a8 = v131;
              v133 = rnd();
              a7 = W_char_skill_roll(c, v132, 1000, 1000 * v133 / 0x8000);
              if ( v132[44] == 7 )
              {
                v259 = v132[180];
                v134 = rnd();
                a4 = W_char_mag_roll(c, v132[179], 1000 * v134 / 0x8000, v259);
              }
              v252 = F32(F32(c, 0x750), 216)
                   + F32(F32(c, 0x750), 224)
                   + F32(F32(c, 0x750), 228);
              v245 = F32(c, 0x1f0);
              v238 = a4;
              v135 = W_char_mag_sum(c);
              W_shot_spawn(
                c,
                (int)c,
                F32(c, 0x1dc),
                100020,
                a2,
                -2000,
                0,
                1,
                errno_decorate,
                j,
                (int)a7,
                270,
                (int)a8,
                a9,
                v132[44],
                v132[45] + v135,
                v238,
                v245,
                v252,
                2);
            }
            if ( errno_decorate == 0
              && F32(c, 0x15b54) != 0
              && F32(c, 0x15b5c) == 0
              && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
              && W_voice_place(c, &a9, &a4, &errno_decorate) != 0 )
            {
              W_voice_play(F32(c, 0x15b54), 0x25u, 0, a9, a4, errno_decorate, 0);
              return v263;
            }
          }
        }
        return v263;
      }
      W_entity_snd_keep(c, 1u, v116);
    }
    W_entity_snd_keep(c, 0, v116);
    goto LABEL_326;
  }
  switch ( v10 )
  {
    case 20113:
      v113 = 100152;
      v264 = 30;
      v268 = 52;
      goto LABEL_386;
    case 20114:
    case 20129:
      goto LABEL_118;
    case 20115:
      v113 = 0;
      v268 = 59;
      goto LABEL_386;
    case 20116:
      goto LABEL_595;
    case 20117:
      v211 = a9;
      v212 = a8;
      v213 = errno_decorate;
      F32(a4, 0) = 1;
      v263 = skill_stage(c, a2, v213, a5, a7, F32(a2, 76), v212, 0x50u, v211, 8);
      if ( v263 != 0 )
      {
        v214 = F16(a2, 88);
        if ( (uint16_t)v214 != 3 && v214 == 6 )
        {
          W_char_shot_fire(
            c,
            100016,
            a2,
            4000,
            0,
            1,
            (int)v213,
            0,
            90,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            0,
            2);
          W_voice_cue(c, (int)v213, 0x3Eu, 0, 0, 0);
        }
      }
      return v263;
    case 20118:
      v170 = a8;
      v171 = a7;
      v172 = errno_decorate;
      F32(a4, 0) = 1;
      v173 = skill_charge(c, a2, (int)v172, a5, v171, F32(a2, 76), v170, 0x50u, 8);
      F16(a2, 86) = a9;
      if ( v173 != 0 )
      {
        if ( F16(a2, 88) == 1 )
        {
          W_entity_snd_start(c, 0, (int)v172, 0x51u, 0, -500, 0, 1);
        }
        else if ( F16(a2, 88) == 2 )
        {
          W_entity_snd_start(c, 1u, (int)v172, 0x50u, 0, 0, 0, 1);
        }
      }
      v174 = F16(a2, 88);
      if ( (uint16_t)v174 == 1 )
        goto LABEL_539;
      if ( (uint16_t)v174 > 1u && F16(a2, 88) <= 6u )
      {
        W_entity_snd_keep(c, 1u, v172);
LABEL_539:
        W_entity_snd_keep(c, 0, v172);
      }
      v175 = F16(a2, 88);
      if ( v175 != 0 && v175 <= 6u )
      {
        if ( F32(a2, 24) > 0 )
          F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
        v176 = F32(a2, 40);
        if ( v176 > 0xAFC8 )
          F32(a2, 40) = W_speed_ramp(v176, 800, -800, 0);
      }
      if ( v172 == 0
        && F32(CFG(), 148) != 0
        && F16(a2, 88) >= 2u
        && F16(a2, 88) <= 3u
        && F32(c, 0x15a60) != 0 )
      {
        if ( F16(a2, 102) >= 0x320u )
          W_particle_spawn(
            c,
            (int)c,
            100095,
            a2,
            F32(c, 0x15a64),
            F32(c, 0x15a68),
            99,
            1,
            0,
            0,
            2,
            1,
            0,
            0);
        if ( F16(a2, 102) >= 0x1F4u )
        {
          if ( F32(c, 0xc828) == 0 )
            F32(c, 0xc828) = W_particle_spawn(
                                 c,
                                 (int)c,
                                 100096,
                                 a2,
                                 F32(c, 0x15a64),
                                 F32(c, 0x15a68),
                                 99,
                                 1,
                                 0,
                                 0,
                                 2,
                                 1,
                                 0,
                                 10);
          F32(c, 0xc830) = 1;
        }
      }
      v263 = v173;
      if ( v173 == 0 )
        return v263;
      v177 = F16(a2, 88);
      if ( (uint16_t)v177 == 3 || v177 != 6 )
        return v263;
      v178 = 40;
      a4 = 20;
      do
      {
        v179 = F32(a2, 96);
        v180 = F32(a2, 76);
        a8 = F32(a2, 44);
        a9 = 0;
        F8(a2, 127) = 1;
        a7 = v179;
        v181 = rnd();
        a5 = W_char_skill_roll(c, v180, 1000, 1000 * v181 / 0x8000);
        if ( v180[44] == 7 )
        {
          v260 = v180[180];
          v182 = rnd();
          a9 = W_char_mag_roll(c, v180[179], 1000 * v182 / 0x8000, v260);
        }
        v253 = F32(F32(c, 0x750), 216)
             + F32(F32(c, 0x750), 224)
             + F32(F32(c, 0x750), 228);
        v246 = F32(c, 0x1f0);
        v239 = a9;
        v183 = W_char_mag_sum(c);
        W_shot_spawn(
          c,
          (int)c,
          F32(c, 0x1dc),
          100021,
          a2,
          4000,
          800,
          1,
          errno_decorate,
          v178 - 40,
          a5,
          a4 - 20,
          (int)a7,
          (int)a8,
          v180[44],
          v180[45] + v183,
          v239,
          v246,
          v253,
          2);
        v184 = F32(a2, 96);
        v185 = F32(a2, 76);
        a8 = F32(a2, 44);
        a9 = 0;
        F8(a2, 127) = 1;
        a7 = v184;
        v186 = rnd();
        a5 = W_char_skill_roll(c, v185, 1000, 1000 * v186 / 0x8000);
        if ( v185[44] == 7 )
        {
          v261 = v185[180];
          v187 = rnd();
          a9 = W_char_mag_roll(c, v185[179], 1000 * v187 / 0x8000, v261);
        }
        v254 = F32(F32(c, 0x750), 216)
             + F32(F32(c, 0x750), 224)
             + F32(F32(c, 0x750), 228);
        v247 = F32(c, 0x1f0);
        v240 = a9;
        v188 = W_char_mag_sum(c);
        W_shot_spawn(
          c,
          (int)c,
          F32(c, 0x1dc),
          100021,
          a2,
          4000,
          800,
          1,
          errno_decorate,
          v178 - 20,
          a5,
          a4 + 20,
          (int)a7,
          (int)a8,
          v185[44],
          v185[45] + v188,
          v240,
          v247,
          v254,
          2);
        v189 = F32(a2, 96);
        v190 = F32(a2, 76);
        a8 = F32(a2, 44);
        a9 = 0;
        F8(a2, 127) = 1;
        a7 = v189;
        v191 = rnd();
        a5 = W_char_skill_roll(c, v190, 1000, 1000 * v191 / 0x8000);
        if ( v190[44] == 7 )
        {
          v262 = v190[180];
          v192 = rnd();
          a9 = W_char_mag_roll(c, v190[179], 1000 * v192 / 0x8000, v262);
        }
        v255 = F32(F32(c, 0x750), 216)
             + F32(F32(c, 0x750), 224)
             + F32(F32(c, 0x750), 228);
        v248 = F32(c, 0x1f0);
        v241 = a9;
        v193 = W_char_mag_sum(c);
        v194 = v190[45];
        v195 = v190[44];
        v196 = a4;
        W_shot_spawn(
          c,
          (int)c,
          F32(c, 0x1dc),
          100021,
          a2,
          4000,
          800,
          1,
          errno_decorate,
          v178,
          a5,
          a4,
          (int)a7,
          (int)a8,
          v195,
          v194 + v193,
          v241,
          v248,
          v255,
          2);
        v178 += 5;
        a4 = v196 + 60;
      }
      while ( (uint16_t)v178 < 0x46u );
      if ( errno_decorate != 0
        || F32(c, 0x15b54) == 0
        || F32(c, 0x15b5c) != 0
        || F32(c, 0x15a8c) == 1 && F32(CFG(), 400) == 0
        || W_voice_place(c, &a9, &a4, &errno_decorate) == 0 )
      {
        return v263;
      }
      W_voice_play(F32(c, 0x15b54), 0x3Du, 0, a9, a4, errno_decorate, 0);
      return v263;
    case 20119:
LABEL_377:
      v113 = 0;
      v268 = 50;
      goto LABEL_386;
    case 20120:
      v113 = 100153;
      v264 = 20;
      v268 = 35;
      goto LABEL_386;
    case 20121:
      v159 = a8;
      v160 = a7;
      v161 = errno_decorate;
      F32(a4, 0) = 1;
      v162 = skill_charge(c, a2, (int)v161, a5, v160, F32(a2, 76), v159, 0x50u, 8);
      F16(a2, 86) = a9;
      if ( v162 != 0 )
      {
        if ( F16(a2, 88) == 1 )
        {
          W_entity_snd_start(c, 0, (int)v161, 0x51u, 0, -500, 0, 1);
        }
        else if ( F16(a2, 88) == 2 )
        {
          W_entity_snd_start(c, 1u, (int)v161, 0x50u, 0, 0, 0, 1);
        }
      }
      v163 = F16(a2, 88);
      if ( (uint16_t)v163 == 1 )
        goto LABEL_499;
      if ( (uint16_t)v163 > 1u && F16(a2, 88) <= 6u )
      {
        W_entity_snd_keep(c, 1u, v161);
LABEL_499:
        W_entity_snd_keep(c, 0, v161);
      }
      v164 = F16(a2, 88);
      if ( v164 != 0 && v164 <= 6u )
      {
        if ( F32(a2, 24) > 0 )
          F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
        v165 = F32(a2, 40);
        if ( v165 > 0xAFC8 )
          F32(a2, 40) = W_speed_ramp(v165, 800, -800, 0);
      }
      if ( v161 == 0
        && F32(CFG(), 148) != 0
        && F16(a2, 88) >= 2u
        && F16(a2, 88) <= 3u
        && F32(c, 0x15a60) != 0 )
      {
        if ( F16(a2, 102) >= 0x320u )
          W_particle_spawn(
            c,
            (int)c,
            100095,
            a2,
            F32(c, 0x15a64),
            F32(c, 0x15a68),
            99,
            1,
            0,
            0,
            2,
            1,
            0,
            0);
        if ( F16(a2, 102) >= 0x1F4u )
        {
          if ( F32(c, 0xc828) == 0 )
            F32(c, 0xc828) = W_particle_spawn(
                                 c,
                                 (int)c,
                                 100096,
                                 a2,
                                 F32(c, 0x15a64),
                                 F32(c, 0x15a68),
                                 99,
                                 1,
                                 0,
                                 0,
                                 2,
                                 1,
                                 0,
                                 10);
          F32(c, 0xc830) = 1;
        }
      }
      v263 = v162;
      if ( v162 == 0 )
        return v263;
      v166 = F16(a2, 88);
      if ( (uint16_t)v166 == 3 || v166 != 6 || v161 != 0 )
        return v263;
      v167 = F32(c, 0x40);
      v168 = W_vpool_take(SYS32(1256), 0);
      if ( v168 != 0 )
      {
        v169 = F32(v168, 0);
        if ( W_sfx_place(
               c,
               F32(CFG(), 360),
               0,
               0,
               v167,
               &errno_decorate,
               &a4) != 0 )
        {
          g_eng.dsb_volume_5bb870(v169, errno_decorate);
          g_eng.dsb_pan_5bb880(v169, a4);
          g_eng.dsb_freq_5bb890(v169, 0);
          g_eng.dsb_play_5bb8c0(v169);
        }
      }
      item_effect((wl_char *)(intptr_t)(c), (int32_t)(a2), (int32_t)(0), (uint16_t *)(intptr_t)(F32(a2, 80)), (int32_t)(F32(a2, 76)), (int32_t)(0), (int16_t)(0));
      if ( F32(c, 0x15b54) == 0
        || F32(c, 0x15b5c) != 0
        || F32(c, 0x15a8c) == 1 && F32(CFG(), 400) == 0 )
      {
        return v263;
      }
      if ( W_voice_place(c, &a9, &a4, &errno_decorate) == 0 )
        return v263;
LABEL_100:
      W_voice_play(F32(c, 0x15b54), 0x27u, 0, a9, a4, errno_decorate, 0);
      return v263;
    case 20122:
    case 20130:
      v231 = a9;
      v232 = a8;
      v233 = errno_decorate;
      F32(a4, 0) = 1;
      v263 = skill_stage(c, a2, v233, a5, a7, F32(a2, 76), v232, 0x50u, v231, 8);
      if ( v263 != 0 )
      {
        v234 = F16(a2, 88);
        if ( (uint16_t)v234 != 3 && v234 == 6 )
        {
          W_char_shot_fire(
            c,
            100004,
            a2,
            -3200,
            -14400,
            1,
            (int)v233,
            0,
            135,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            0,
            2);
          W_voice_cue(c, (int)v233, 0x22u, 0, 0, 0);
        }
      }
      return v263;
    case 20123:
      v197 = a9;
      v198 = a8;
      F32(a4, 0) = 1;
      v263 = skill_stage(c, a2, errno_decorate, a5, a7, F32(a2, 76), v198, 0x50u, v197, 8);
      if ( v263 != 0 )
      {
        v199 = F16(a2, 88);
        if ( (uint16_t)v199 != 3 && v199 == 6 )
        {
          v200 = 0;
          v201 = 0;
          do
          {
            W_char_shot_fire(
              c,
              100022,
              a2,
              160 * (v266[v201] - 2 * v267[v201]) + 10000,
              32 * (5 * v266[v201] - 750),
              1,
              errno_decorate,
              v200,
              135,
              F32(a2, 76),
              1000,
              F32(a2, 44),
              0,
              2);
            v200 += 5;
            ++v201;
          }
          while ( (uint16_t)v200 < 0x28u );
          W_voice_cue(c, errno_decorate, 0x2Au, 0, 0, 0);
        }
      }
      return v263;
    case 20124:
    case 20136:
      goto LABEL_378;
    case 20125:
      v113 = 100151;
      v264 = 30;
      v268 = 46;
      goto LABEL_386;
    case 20126:
      v225 = a9;
      v226 = a8;
      F32(a4, 0) = 1;
      v263 = skill_stage(c, a2, errno_decorate, a5, a7, F32(a2, 76), v226, 0x50u, v225, 8);
      if ( v263 != 0 )
      {
        v227 = F16(a2, 88);
        if ( (uint16_t)v227 != 3 && v227 == 6 )
        {
          v228 = 0;
          v229 = 90;
          do
          {
            v230 = W_char_shot_fire(
                     c,
                     100002,
                     a2,
                     0,
                     0,
                     1,
                     errno_decorate,
                     v228,
                     v229,
                     F32(a2, 76),
                     1000,
                     F32(a2, 44),
                     0,
                     2);
            if ( v230 != 0 )
              F32(v230, 516) = 800;
            v228 += 2;
            v229 -= 30;
          }
          while ( (uint16_t)v228 < 0x1Au );
          W_voice_cue(c, errno_decorate, 0x2Du, 0, 0, 0);
        }
      }
      return v263;
    case 20127:
      v207 = a9;
      v208 = a8;
      v209 = errno_decorate;
      F32(a4, 0) = 1;
      v263 = skill_stage(c, a2, v209, a5, a7, F32(a2, 76), v208, 0x50u, v207, 8);
      if ( v263 != 0 )
      {
        v210 = F16(a2, 88);
        if ( (uint16_t)v210 != 3 && v210 == 6 )
        {
          W_char_shot_fire(
            c,
            100010,
            a2,
            -6400,
            0,
            1,
            (int)v209,
            0,
            30,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            0,
            2);
          W_char_shot_fire(
            c,
            100010,
            a2,
            -6400,
            0,
            1,
            (int)v209,
            10,
            150,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            1,
            2);
          W_char_shot_fire(
            c,
            100010,
            a2,
            -6400,
            0,
            1,
            (int)v209,
            20,
            10,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            1,
            2);
          W_char_shot_fire(
            c,
            100010,
            a2,
            -6400,
            0,
            1,
            (int)v209,
            30,
            170,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            1,
            2);
          W_char_shot_fire(
            c,
            100010,
            a2,
            -6400,
            -1600,
            1,
            (int)v209,
            40,
            60,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            1,
            2);
          W_char_shot_fire(
            c,
            100010,
            a2,
            -6400,
            1600,
            1,
            (int)v209,
            50,
            120,
            F32(a2, 76),
            1000,
            F32(a2, 44),
            1,
            2);
          W_voice_cue(c, (int)v209, 0x2Fu, 0, 0, 0);
        }
      }
      return v263;
    case 20128:
      goto LABEL_603;
    case 20131:
      v202 = a9;
      v203 = a8;
      F32(a4, 0) = 1;
      v263 = skill_stage(c, a2, errno_decorate, a5, a7, F32(a2, 76), v203, 0x50u, v202, 8);
      if ( v263 != 0 )
      {
        v204 = F16(a2, 88);
        if ( (uint16_t)v204 != 3 && v204 == 6 )
        {
          v205 = 0;
          a4 = (int)v265;
          do
          {
            v206 = F32(a4, 0);
            W_char_shot_fire(
              c,
              100009,
              a2,
              F32(a4, 0),
              3200,
              1,
              errno_decorate,
              v205,
              55,
              F32(a2, 76),
              1000,
              F32(a2, 44),
              1,
              2);
            W_char_shot_fire(
              c,
              100009,
              a2,
              -v206,
              3200,
              1,
              errno_decorate,
              v205 + 7,
              55,
              F32(a2, 76),
              1000,
              F32(a2, 44),
              1,
              2);
            v205 += 15;
            a4 += 4;
          }
          while ( (uint16_t)v205 < 0x4Bu );
          W_voice_cue(c, errno_decorate, 0x2Bu, 0, 0, 0);
        }
      }
      return v263;
    case 20132:
      goto LABEL_384;
    case 20133:
      v113 = 0;
      v268 = 58;
      goto LABEL_386;
    case 20134:
      v113 = 100150;
      v264 = 30;
      v268 = 48;
      goto LABEL_386;
    case 20135:
      v150 = errno_decorate;
      v151 = F32(a2, 76);
      if ( errno_decorate == 0 && F16(a2, 88) != 0 && F16(a2, 88) <= 3u && v151 != 0 )
      {
        v152 = F32(v151, 184) - 1;
        if ( v152 == 0 || v152 == 2 )
          F32(c, 0xc788) = 1;
      }
      v153 = a8;
      F32(a4, 0) = 1;
      v154 = skill_charge(c, a2, (int)v150, a5, a7, F32(a2, 76), v153, 0x50u, 8);
      F16(a2, 86) = a9;
      if ( v154 != 0 )
      {
        if ( F16(a2, 88) == 1 )
        {
          W_entity_snd_start(c, 0, (int)v150, 0x51u, 0, -500, 0, 1);
        }
        else if ( F16(a2, 88) == 2 )
        {
          W_entity_snd_start(c, 1u, (int)v150, 0x50u, 0, 0, 0, 1);
        }
      }
      v155 = F16(a2, 88);
      if ( (uint16_t)v155 == 1 )
        goto LABEL_450;
      if ( (uint16_t)v155 > 1u && F16(a2, 88) <= 6u )
      {
        W_entity_snd_keep(c, 1u, v150);
LABEL_450:
        W_entity_snd_keep(c, 0, v150);
      }
      v156 = F16(a2, 88);
      if ( v156 != 0 && v156 <= 6u )
      {
        if ( F32(a2, 24) > 0 )
          F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
        v157 = F32(a2, 40);
        if ( v157 > 0xAFC8 )
          F32(a2, 40) = W_speed_ramp(v157, 800, -800, 0);
      }
      if ( v150 == 0
        && F32(CFG(), 148) != 0
        && F16(a2, 88) >= 2u
        && F16(a2, 88) <= 3u
        && F32(c, 0x15a60) != 0 )
      {
        if ( F16(a2, 102) >= 0x320u )
          W_particle_spawn(
            c,
            (int)c,
            100095,
            a2,
            F32(c, 0x15a64),
            F32(c, 0x15a68),
            99,
            1,
            0,
            0,
            2,
            1,
            0,
            0);
        if ( F16(a2, 102) >= 0x1F4u )
        {
          if ( F32(c, 0xc828) == 0 )
            F32(c, 0xc828) = W_particle_spawn(
                                 c,
                                 (int)c,
                                 100096,
                                 a2,
                                 F32(c, 0x15a64),
                                 F32(c, 0x15a68),
                                 99,
                                 1,
                                 0,
                                 0,
                                 2,
                                 1,
                                 0,
                                 10);
          F32(c, 0xc830) = 1;
        }
      }
      v263 = v154;
      if ( v154 == 0 )
        return v263;
      v158 = F16(a2, 88);
      if ( (uint16_t)v158 == 3 || v158 != 6 )
        return v263;
      if ( v150 == 0
        && F32(c, 0x15b54) != 0
        && F32(c, 0x15b5c) == 0
        && (F32(c, 0x15a8c) != 1 || F32(CFG(), 400) != 0)
        && W_voice_place(c, &a9, &a4, &errno_decorate) != 0 )
      {
        W_voice_play(F32(c, 0x15b54), 0x2Cu, 0, a9, a4, errno_decorate, 0);
      }
      v77 = F32(a2, 76);
      if ( v150 != 0 )
        return v263;
      if ( !summon_room_ok(c, (int)c, v77[177], v77[178], v77[179]) )
      {
        if ( F32(c, 0xc794) == 0 )
          return v263;
        while ( !summon_room_ok(c, (int)c, v77[177], v77[178], v77[179])
             && (summon_recall((int)c, v77[177]) != 0 || summon_recall((int)c, 0) != 0) )
          ;
      }
      if ( skill_space_ahead(a2, 9600, 0, 12800, 6400, &errno_decorate) == 0 )
      {
        if ( F32(c, 0xc794) == 0 )
          return v263;
        errno_decorate = 0;
      }
LABEL_487:
      v66 = summon_spawn(v77[177], a2, errno_decorate, 0, 4, 0, F32(a2, 44), 5, 0);
LABEL_488:
      if ( v66 == 0 )
        return v263;
      summon_init(v66, c);
      result = v263;
      break;
    default:
      return v263;
  }
  return result;
}

/* 0x459f20: int __thiscall sub_459F20( cls_4022A0 *this, int a2, int a3, int a4, _DWORD *a5, int a6, _WORD *a7, unsigned __int16 a8, int a9) */
int32_t skill_charge(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t a6, uint16_t * a7, uint16_t a8, int32_t a9)
{
    intptr_t c = (intptr_t)self;
    intptr_t v9;
    uint16_t * v10;
    uint16_t * v12;
    intptr_t v13;
    int16_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    uint16_t v19;

  v9 = 0;
  v10 = a7;
  switch ( F16(a2, 88) )
  {
    case 0:
      if ( F32(a6, 724) <= F32(F32(c, 0x750), 92) )
      {
        v10 = a7;
        if ( a7[33] != 0 && F32(a2, 108) != a7 )
        {
          F32(a2, 108) = a7;
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        v9 = 1;
        F16(a2, 88) = 1;
        goto LABEL_8;
      }
      F16(a2, 72) = 0;
      F16(a2, 86) = 210;
      F16(a2, 88) = 0;
      F32(a2, 92) = 0;
      F32(a2, 120) = 0;
      F16(a2, 84) = 0;
      F32(a2, 76) = 0;
      F32(a2, 80) = 0;
      F16(a2, 90) = 0;
      F16(a2, 100) = 0;
      return 0;
    case 1:
LABEL_8:
      if ( F8(a2, 116) == 0 && v10[33] != 0 )
        goto LABEL_15;
      if ( v10[235] != 0 )
      {
        v12 = v10 + 202;
        if ( F32(a2, 108) != v12 )
        {
          F32(a2, 108) = v12;
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
      }
      F16(a2, 88) = 2;
      goto LABEL_14;
    case 2:
      if ( F16(a2, 102) >= 0x64u )
      {
        F16(a2, 88) = 3;
        v9 = 1;
      }
      goto LABEL_23;
    case 3:
LABEL_23:
      if ( F16(a2, 102) >= 0x3E8u )
      {
        if ( a7[437] != 0 && F32(a2, 108) != a7 + 404 )
        {
          F32(a2, 108) = a7 + 404;
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        if ( F8(a2, 104) == 0 )
          *a5 = 0;
      }
      else if ( F8(a2, 104) == 0 )
      {
        v14 = F16(a6, 728) + F16(a2, 102);
        if ( F16(a6, 728) + (unsigned int)F16(a2, 102) > 0x3E8 )
          v14 = 1000;
        F16(a2, 102) = v14;
        *a5 = 0;
      }
      goto LABEL_15;
    case 4:
      if ( a7[639] != 0 && F32(a2, 108) != a7 + 606 )
      {
        F32(a2, 108) = a7 + 606;
        F8(a2, 116) = 0;
        F16(a2, 114) = 0;
        F16(a2, 112) = 0;
      }
      F16(a2, 88) = 5;
      F8(a2, 104) = 1;
      v9 = 1;
      goto LABEL_37;
    case 5:
LABEL_37:
      if ( F8(a2, 116) == 0 && a7[639] != 0 )
        goto LABEL_15;
      v15 = F32(c, 0x750);
      v16 = F32(a6, 724);
      v17 = F32(v15, 92);
      if ( v16 > v17 )
        goto LABEL_19;
      if ( a3 == 0 )
        F32(v15, 92) = v17 - v16;
      if ( a7[841] != 0 && F32(a2, 108) != a7 + 808 )
      {
        F32(a2, 108) = a7 + 808;
        F8(a2, 116) = 0;
        F16(a2, 114) = 0;
        F16(a2, 112) = 0;
      }
      v9 = 1;
      F16(a2, 88) = 6;
LABEL_15:
      if ( F16(a2, 88) >= 4u )
        goto LABEL_59;
      v13 = F32(a4, 16);
      if ( v13 == 15 )
        goto LABEL_19;
      if ( v13 == 14 )
        goto LABEL_59;
      if ( F16(a2, 102) >= 0x3E8u )
      {
        F16(a2, 88) = 4;
        v9 = 1;
LABEL_59:
        v19 = F16(a2, 88);
        if ( v19 >= 4u && v19 <= 6u )
          *a5 = 0;
      }
      else
      {
LABEL_19:
        if ( a9 == 0 )
        {
          F16(a2, 72) = 0;
          F16(a2, 86) = 210;
          F16(a2, 88) = 0;
          F32(a2, 92) = 0;
          F32(a2, 120) = 0;
          F16(a2, 84) = 0;
          F32(a2, 76) = 0;
          F32(a2, 80) = 0;
          F16(a2, 90) = 0;
          F16(a2, 100) = 0;
          return v9;
        }
        if ( (F16(a2, 86) / 0x64u <= 2 || F16(a2, 72) == 0)
          && (uint16_t)a9 != F16(a2, 72) )
        {
          F16(a2, 72) = a9;
          F16(a2, 86) = 210;
          F16(a2, 88) = 0;
          F32(a2, 92) = 0;
          F32(a2, 120) = 0;
          F16(a2, 84) = 0;
          F32(a2, 76) = 0;
          F32(a2, 80) = 0;
          F16(a2, 90) = 0;
          F16(a2, 100) = 0;
          return v9;
        }
      }
      return v9;
    case 6:
      v18 = F32(a2, 92) + 1;
      F32(a2, 92) = v18;
      if ( v18 > a8 )
      {
        if ( a7[1043] != 0 && F32(a2, 108) != a7 + 1010 )
        {
          F32(a2, 108) = a7 + 1010;
          F8(a2, 116) = 0;
          F16(a2, 114) = 0;
          F16(a2, 112) = 0;
        }
        F16(a2, 88) = 7;
LABEL_14:
        v9 = 1;
      }
      goto LABEL_15;
    case 7:
      if ( F8(a2, 116) == 0 && a7[1043] != 0 )
        goto LABEL_15;
      F16(a2, 72) = 0;
      F16(a2, 86) = 210;
      F16(a2, 88) = 0;
      F32(a2, 92) = 0;
      F32(a2, 120) = 0;
      F16(a2, 84) = 0;
      F32(a2, 76) = 0;
      F32(a2, 80) = 0;
      F16(a2, 90) = 0;
      F16(a2, 100) = 0;
      return 1;
    default:
      goto LABEL_15;
  }
}

/* 0x45db20: int __thiscall sub_45DB20( cls_4022A0 *this, int a2, FILE *a3, int a4, _DWORD *a5, int a6, _WORD *a7, unsigned __int16 a8, __int16 a9, int a10) */
int32_t skill_stage(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t * a5, int32_t a6, uint16_t * a7, uint16_t a8, int16_t a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    uint16_t * v11;
    int16_t v12;
    intptr_t v13;
    intptr_t v14;
    uint16_t v15;
    intptr_t ** v16;
    intptr_t v17;
    intptr_t * v18;
    intptr_t v19;
    uint16_t v20;
    intptr_t v21;
    intptr_t v23;
    intptr_t * v24;

  v23 = 0;
  v11 = a7;
  switch ( F16(a2, 88) )
  {
    case 0:
      if ( skill_mp_pay(c, a6, (int)a3, 0) == 0 )
        goto LABEL_3;
      v11 = a7;
      if ( a7[33] != 0 )
        W_body_set_clip(a2, (int)a7);
      F16(a2, 88) = 1;
      v23 = 1;
      goto LABEL_7;
    case 1:
LABEL_7:
      if ( F8(a2, 116) != 0 || v11[33] == 0 )
      {
        if ( v11[235] != 0 )
          W_body_set_clip(a2, (int)(v11 + 202));
        F16(a2, 88) = 2;
        v23 = 1;
      }
      goto LABEL_20;
    case 2:
      if ( F16(a2, 102) >= 0x64u )
      {
        F16(a2, 88) = 3;
        v23 = 1;
      }
      goto LABEL_14;
    case 3:
LABEL_14:
      if ( F16(a2, 102) < 0x3E8u )
      {
        if ( F8(a2, 104) != 0 )
          goto LABEL_20;
        v12 = F16(a6, 728) + F16(a2, 102);
        if ( F16(a6, 728) + (unsigned int)F16(a2, 102) > 0x3E8 )
          v12 = 1000;
        F16(a2, 102) = v12;
        goto LABEL_19;
      }
      if ( a7[437] != 0 )
        W_body_set_clip(a2, (int)(a7 + 404));
      if ( F8(a2, 104) == 0 )
LABEL_19:
        *a5 = 0;
LABEL_20:
      if ( F16(a2, 88) >= 4u )
        goto LABEL_48;
      v13 = F32(a4, 16);
      if ( v13 == 15 )
        goto LABEL_24;
      if ( v13 == 14 )
        goto LABEL_48;
      if ( F16(a2, 102) >= 0x3E8u )
      {
        F16(a2, 88) = 4;
        v23 = 1;
LABEL_48:
        v15 = F16(a2, 88);
        if ( v15 >= 4u && v15 <= 6u )
          *a5 = 0;
      }
      else
      {
LABEL_24:
        if ( a10 != 0 )
          W_body_action_start(a2, a10, 210);
        else
LABEL_3:
          W_body_action_reset(a2);
      }
LABEL_51:
      F16(a2, 86) = a9;
      if ( v23 != 0 )
      {
        if ( F16(a2, 88) == 1 )
        {
          if ( a3 == 0 && F32(c, 0x1b0) != 0 )
          {
            if ( F16(c, 0x1b4) != 0 )
            {
              if ( W_sfx_place(c, 0, 0, 0, F32(c, 0x40), 0, 0) != 0 )
              {
                v16 = F32(c, 0x1b0);
                v17 = SYS32(1192);
                v24 = SYS32(1192);
                if ( F32(v16, 0) != 0 )
                  W_entity_snd_release(c, 0, 0, 1);
                v18 = W_vpool_take(v17, 1);
                if ( v18 != 0 )
                {
                  *v16 = v18;
                  v16[1] = v24;
                  v16[2] = 0;
                  v16[3] = -500;
                  v16[4] = 0;
                  v16[5] = 1;
                  v16[6] = 0;
                  v16[7] = 0;
                  g_eng.dsb_volume_5bb870(F32(v18, 0), -10000);
                  g_eng.dsb_play_5bb8c0(*v18);
                }
              }
            }
            else
            {
              g_eng.h408b90(D(0x8a9530), D(0x85525c), 0, D(0x8a9b6c));
            }
          }
        }
        else if ( F16(a2, 88) == 2 )
        {
          W_entity_snd_start(c, 1u, (int)a3, 0x50u, 0, 0, 0, 1);
        }
      }
      v19 = F16(a2, 88);
      if ( (uint16_t)v19 == 1 )
        goto LABEL_68;
      if ( (uint16_t)v19 > 1u && F16(a2, 88) <= 6u )
      {
        W_entity_snd_keep(c, 1u, a3);
LABEL_68:
        W_entity_snd_keep(c, 0, a3);
      }
      v20 = F16(a2, 88);
      if ( v20 != 0 && v20 <= 6u )
      {
        if ( F32(a2, 24) > 0 )
          F32(a2, 28) = 6000 - 5000 * F16(a2, 102) / 1000;
        v21 = F32(a2, 40);
        if ( v21 > 0xAFC8 )
          F32(a2, 40) = W_speed_ramp(v21, 800, -800, 0);
      }
      if ( a3 == 0
        && F32(CFG(), 148) != 0
        && F16(a2, 88) >= 2u
        && F16(a2, 88) <= 3u
        && F32(c, 0x15a60) != 0 )
      {
        if ( F16(a2, 102) >= 0x320u )
          W_particle_spawn(
            c,
            (int)c,
            100095,
            a2,
            F32(c, 0x15a64),
            F32(c, 0x15a68),
            99,
            1,
            0,
            0,
            2,
            1,
            0,
            0);
        if ( F16(a2, 102) >= 0x1F4u )
        {
          if ( F32(c, 0xc828) == 0 )
            F32(c, 0xc828) = W_particle_spawn(
                                 c,
                                 (int)c,
                                 100096,
                                 a2,
                                 F32(c, 0x15a64),
                                 F32(c, 0x15a68),
                                 99,
                                 1,
                                 0,
                                 0,
                                 2,
                                 1,
                                 0,
                                 10);
          F32(c, 0xc830) = 1;
        }
      }
      return v23;
    case 4:
      if ( a7[639] != 0 )
        W_body_set_clip(a2, (int)(a7 + 606));
      F16(a2, 88) = 5;
      F8(a2, 104) = 1;
      v23 = 1;
      goto LABEL_33;
    case 5:
LABEL_33:
      if ( F8(a2, 116) == 0 && a7[639] != 0 )
        goto LABEL_20;
      if ( skill_mp_pay(c, a6, (int)a3, 1) == 0 )
        goto LABEL_24;
      if ( a7[841] != 0 )
        W_body_set_clip(a2, (int)(a7 + 808));
      v23 = 1;
      F16(a2, 88) = 6;
      goto LABEL_20;
    case 6:
      v14 = F32(a2, 92) + 1;
      F32(a2, 92) = v14;
      if ( v14 > a8 )
      {
        if ( a7[1043] != 0 )
          W_body_set_clip(a2, (int)(a7 + 1010));
        F16(a2, 88) = 7;
        v23 = 1;
      }
      goto LABEL_20;
    case 7:
      if ( F8(a2, 116) == 0 && a7[1043] != 0 )
        goto LABEL_20;
      v23 = 1;
      W_body_action_reset(a2);
      goto LABEL_51;
    default:
      goto LABEL_20;
  }
}

/* 0x45a2c0: int __thiscall sub_45A2C0(cls_4022A0 *this, int a2, int a3, int a4) */
int32_t skill_mp_pay(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;

  v4 = F32(c, 0x750);
  v5 = F32(a2, 724);
  v6 = F32(v4, 92);
  if ( v5 > v6 )
    return 0;
  if ( a3 == 0 && a4 == 1 )
    F32(v4, 92) = v6 - v5;
  return 1;
}

/* 0x456a50: int __stdcall sub_456A50(int a1, int a2) */
int32_t summon_recall(int32_t a1, int32_t a2)
{

    int32_t * v2;
    intptr_t v3;
    char * v4;
    intptr_t i;
    intptr_t v6;
    intptr_t v7;
    int16_t v9;
    intptr_t v10;

  v2 = 0;
  v3 = 0;
  v4 = W + 4448;
  for ( i = 32; i != 0; --i )
  {
    v6 = F32(v4, 0);
    if ( F32(F32(v4, 0), 464) == 1
      && F8(F32(v6, 64), 0) == 1
      && F16(F32(v6, 1872), 1098) == 0
      && F32(v6, 88928) == a1
      && (F32(v6, 468) == a2 || a2 == 0)
      && (v3 < F32(v6, 88264) || v2 == 0) )
    {
      v2 = *v4;
      v3 = F32(v6, 88264);
    }
    v4 += 4;
  }
  if ( v2 == 0 )
    return 0;
  v7 = v2[22174];
  v2[22232] = 0;
  if ( v7 != 0 )
  {
    v9 = F16(v2, 2 * 44348);
    v10 = v2[16];
    if ( (F16(v10, 86) / 0x64u <= 8 || F16(v10, 72) == 0) && v9 != F16(v10, 72) )
    {
      F16(v10, 72) = v9;
      F16(v10, 86) = 800;
      F16(v10, 88) = 0;
      F32(v10, 92) = 0;
      F32(v10, 120) = 0;
      F16(v10, 84) = 0;
      F32(v10, 76) = 0;
      F32(v10, 80) = 0;
      F16(v10, 90) = 0;
      F16(v10, 100) = 0;
    }
    return 1;
  }
  else
  {
    W_char_die((intptr_t)v2, 0, 1, F32(v2[16], 44));   /* thiscall (ecx = v2): IDA read it as fastcall */
    return 1;
  }
}

/* 0x456b60: BOOL __thiscall sub_456B60(cls_4022A0 *this, int a2, int a3, int a4, int a5) */
int32_t summon_room_ok(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5;
    char * v6;
    intptr_t i;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v13;
    intptr_t v14;

  v5 = 0;
  v6 = (W + 0x1030 + 304);
  v13 = 0;
  v14 = 0;
  for ( i = 32; i != 0; --i )
  {
    v8 = F32(v6, 0);
    if ( F32(F32(v6, 0), 464) == 1
      && F8(F32(v8, 64), 0) == 1
      && F16(F32(v8, 1872), 1098) == 0 )
    {
      v9 = F32(a2, 476);
      v10 = F32(v8, 476);
      if ( v9 != 0 && v10 != 0 && v9 != 3 && v10 != 3 && v9 == v10 )
      {
        if ( F32(v8, 468) == a3 || a3 == 0 )
        {
          ++v5;
          if ( F32(v8, 88928) != a2 )
            goto LABEL_16;
          ++v14;
        }
        if ( F32(v8, 88928) == a2 )
          ++v13;
      }
    }
LABEL_16:
    v6 += 4;
  }
  v11 = F32(c, 0x15a6c);
  if ( v11 != 0 && v11 <= v13 )
    return false;
  if ( a5 > 0 && a5 <= v5 )
    return false;
  return a4 <= 0 || a4 > v14;
}

/* 0x45e050: int __stdcall sub_45E050(_DWORD *a1, int a2, int a3, int a4, int a5, _DWORD *a6) */
int32_t skill_space_ahead(int32_t * a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t * a6)
{

    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;
    intptr_t v18;
    uint16_t * v19;
    intptr_t v20;
    intptr_t v22;
    intptr_t v23;
    intptr_t v24;
    intptr_t v25;
    intptr_t v26;
    intptr_t v27;
    intptr_t v28;
    intptr_t v29;
    intptr_t v30;
    intptr_t v31;
    intptr_t v32;

  v25 = 0;
  v22 = 0;
  v24 = 1;
  v6 = a2 - 25600;
  v23 = a2 - 25600;
  if ( __OFSUB__(a2 - 25600, a2 + 25600) )
    goto LABEL_23;
  v27 = a1[11];
  v7 = a3 + a1[2] - a1[4] / 2;
  v8 = a1[1] - a1[3] / 2;
  v26 = v8;
  v29 = a4 / 2;
  v9 = v7 - a5 / 2;
  v10 = a5 / 2 + v7;
  v32 = 0;
  v30 = v9 / 3200;
  v11 = 25600 - a2;
  v31 = v10 / 3200 + 1;
  while ( 1 )
  {
    v12 = v11;
    if ( v27 != 3 )
      v12 = v6;
    v13 = v8 + v12 + v29;
    v14 = (v8 + v12 - v29) / 3200;
    v15 = v13 / 3200 + 1;
    if ( v14 >= 0 )
    {
      v16 = F32(W, 4 * 1042);
      v28 = v16;
      if ( v15 <= F32(v16, 2887728) )
      {
        v17 = v30;
        if ( v30 >= 0 && v31 <= F32(v16, 2887732) )
        {
          if ( v30 >= v31 )
          {
LABEL_17:
            if ( (int)abs32(v32 + a2) > (int)abs32(v11 + a2) || v24 != 0 )
            {
              v25 = v6;
              v32 = v11;
              v22 = 1;
              v24 = 0;
            }
          }
          else
          {
            while ( 1 )
            {
              v18 = v14;
              if ( v14 < v15 )
                break;
LABEL_16:
              if ( ++v17 >= v31 )
                goto LABEL_17;
            }
            v19 = (v16 + 2 * (v17 + (v14 << 7)) + 2887744);
            while ( *v19 != 1 )
            {
              ++v18;
              v19 += 128;
              if ( v18 >= v15 )
              {
                v6 = v23;
                v16 = v28;
                goto LABEL_16;
              }
            }
            v6 = v23;
          }
        }
      }
    }
    v6 += 1600;
    v11 -= 1600;
    v23 = v6;
    if ( v6 >= a2 + 25600 )
      break;
    v8 = v26;
  }
  if ( v22 != 0 )
    v20 = v25;
  else
LABEL_23:
    v20 = 0;
  if ( a6 != 0 )
    *a6 = v20;
  return v22;
}

/* 0x45e4c0: int __thiscall sub_45E4C0(cls_4022A0 *this, _DWORD *a2) */
int32_t summon_init(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3;
    intptr_t v4;
    intptr_t v5;
    intptr_t v6;
    intptr_t v7;
    intptr_t v8;
    intptr_t v9;
    intptr_t v10;
    intptr_t v11;
    intptr_t v12;
    intptr_t result;

  v3 = F32(c, 0x1d4);
  if ( v3 > 0xC835 )
  {
    v5 = v3 - 100186;
    if ( v5 != 0 )
    {
      if ( v5 == 1 )
      {
        F32(c, 0x14240) = 11;
        F32(c, 0x14250) = -8000;
        F32(c, 0x14254) = -8000;
        v6 = F32(c, 0x40);
        F32(c, 0x1424c) = (int)a2;
        F32(c, 0x14244) = F32(v6, 44);
        F16(c, 0x14248) = 0;
        F32(c, 0x14258) = 0;
        W_body_action_start(v6, 5, 800);
        v7 = SYS32(10152);
        F32(c, 0x15a70) = 1;
        F32(c, 0xf4) = v7;
        F32(c, 0xf8) = v7;
        F32(c, 0x15a7c) = 7;
        F32(c, 0x15a74) = 5000;
        F32(c, 0x15a78) = 6;
        v8 = rnd();
        if ( W_stat_jitter(
               (a2[12793] + F32(a2[468], 108) + F32(a2[468], 124) + F32(a2[468], 148)) / 2,
               4000,
               1000 * v8 / 0x8000) < 50
          || (v9 = rnd(),
              W_stat_jitter(
                (a2[12793] + F32(a2[468], 108) + F32(a2[468], 124) + F32(a2[468], 148)) / 2,
                4000,
                1000 * v9 / 0x8000) <= 350) )
        {
          v11 = rnd();
          if ( W_stat_jitter(
                 (a2[12793] + F32(a2[468], 108) + F32(a2[468], 124) + F32(a2[468], 148)) / 2,
                 4000,
                 1000 * v11 / 0x8000) >= 50 )
          {
            v12 = rnd();
            v10 = W_stat_jitter(
                    (a2[12793] + F32(a2[468], 108) + F32(a2[468], 124) + F32(a2[468], 148))
                  / 2,
                    4000,
                    1000 * v12 / 0x8000);
          }
          else
          {
            v10 = 50;
          }
        }
        else
        {
          v10 = 350;
        }
        F32(F32(c, 0x750), 88) = v10;
        F32(F32(c, 0x750), 84) = F32(F32(c, 0x750), 88)
                                          + F32(F32(c, 0x750), 132)
                                          + F32(F32(c, 0x750), 156);
        F16(c, 0x15a84) = 100;
        F16(c, 0x15a86) = 100;
      }
    }
    else
    {
      W_body_action_start(F32(c, 0x40), 24, 800);
    }
  }
  else if ( v3 >= 0xC834 )
  {
LABEL_5:
    W_body_action_start(F32(c, 0x40), 5, 800);
    v4 = W_tint_fade_a(0, 1);
    F32(c, 0xf4) = v4;
    F32(c, 0xf8) = v4;
    F32(c, 0x15a70) = 1;
  }
  else
  {
    switch ( v3 )
    {
      case 0xC789u:
      case 0xC78Au:
      case 0xC78Bu:
        W_body_action_start(F32(c, 0x40), 5, 800);
        break;
      case 0xC794u:
      case 0xC80Du:
        goto LABEL_5;
      default:
        break;
    }
  }
  F32(c, 0x15b60) = (int)a2;
  result = a2[119];
  F32(c, 0x1dc) = result;
  return result;
}

/* 0x55ce70: int __stdcall sub_55CE70(int a1, _DWORD *a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9) */
int32_t summon_spawn(int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9)
{

    intptr_t v10;
    intptr_t v11;
    uint16_t v12;
    intptr_t v13;
    intptr_t v14;
    intptr_t v15;
    intptr_t v16;
    intptr_t v17;

  if ( a6 != 0 )
    return 0;
  v10 = 0;
  v11 = F32(W, 4 * 1041);
  while ( 1 )
  {
    v12 = F16(v11, 204);
    v13 = F32(W, 4 * (v12 + 1112));
    F16(v11, 204) = (v12 + 1) & 0x1F;
    if ( F32(v13, 464) == 0 )
      break;
    if ( (uint16_t)++v10 >= 0x20u )
      return 0;
  }
  if ( v13 == 0 )
    return 0;
  v14 = a3;
  if ( a2[11] == 3 )
    v14 = -a3;
  switch ( a5 )
  {
    case 1:
      v15 = v14 + a2[1] + a2[3] / 2;
      v16 = a4 + a2[2] + a2[4] / 2;
      break;
    case 2:
      v17 = a2[1];
      goto LABEL_16;
    case 3:
      v17 = a2[1] + a2[3] / 2;
LABEL_16:
      v15 = v14 + v17;
      v16 = a4 + a2[2] + a2[5];
      break;
    case 4:
      v15 = v14 + a2[1] + a2[3] / 2;
      v16 = a4 + a2[2] + a2[4];
      break;
    default:
      v16 = a4;
      v15 = v14;
      break;
  }
  W_char_init(v13, v15, v16, a8, 4, 0, a1, 0, a7, 0, 0, 0, a9, 0, 0);
  return v13;
}
/* unported callees: 0x408b90 0x5b3b00 */
