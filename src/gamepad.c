/* gamepad.c — the GAMEPAD input producer 0x468b10 (the joystick devices' state -> the input manager's ring records
 * and held axes, the button map from the key config, the dead zones) + 0x46cb40 / 0x46cd10.  See gamepad.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  UNVERIFIED: rhost's DirectInput
 * enumerates no joystick (hermetic); REPLACE-wired, compile-checked. */
#include "gamepad.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include "frame_load.h"
#include "input_dev.h"
#pragma GCC optimize ("no-strict-aliasing")
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#define CFG() ((intptr_t)**(uint8_t ***)((uint8_t *)g_eng.sys + 0x320))
#define SYS32(o) F32(g_eng.sys, (o))
#define SYSB(o) F8(g_eng.sys, (o))   /* the system object's byte at +o (an lvalue: &SYSB(o) is its address) */
static inline int ofsub32_(int32_t a, int32_t b) { int32_t r; return __builtin_sub_overflow(a, b, &r); }
static inline int ofadd32_(int32_t a, int32_t b) { int32_t r; return __builtin_add_overflow(a, b, &r); }
#define __OFSUB__(a, b) ofsub32_((int32_t)(a), (int32_t)(b))
#define __OFADD__(a, b) ofadd32_((int32_t)(a), (int32_t)(b))
#define __CFADD__(a, b) ((uint32_t)(a) + (uint32_t)(b) < (uint32_t)(a))
#define __CFSUB__(a, b) ((uint32_t)(a) < (uint32_t)(b))
#define __SETS__(x) ((int32_t)(x) < 0)
#define LODWORD(x) ((uint32_t)(uint64_t)(x))
#define HIDWORD(x) ((uint32_t)((uint64_t)(x) >> 32))
#define WORD1(x) ((uint16_t)((uint64_t)(x) >> 16))
#define WORD2(x) ((uint16_t)((uint64_t)(x) >> 32))
#define BYTE1(x) ((uint8_t)((uint64_t)(x) >> 8))
#define BYTE2(x) ((uint8_t)((uint64_t)(x) >> 16))
#define HIBYTE(x) ((uint8_t)((uint32_t)(x) >> 24))
#define HIWORD(x) ((uint16_t)((uint32_t)(x) >> 16))
#define SHIDWORD(x) ((int32_t)((uint64_t)(x) >> 32))
#define SLODWORD(x) ((int32_t)(x))
#include <math.h>
/* 0x471080: rounded half away from zero (fcomp 0.0, then fsub / fadd 0.5 and __ftol's truncation) */
static inline int64_t ftol_round_(double a) { return a < 0.0 ? (int64_t)(a - 0.5) : (int64_t)(a + 0.5); }
static inline int32_t abs32(int32_t x) { return x < 0 ? (int32_t)(0u - (uint32_t)x) : x; }
#ifndef __cplusplus
#include <stdbool.h>
#endif
typedef int32_t _DWORD;
typedef uint8_t _UNKNOWN;   /* IDA's untyped byte (casts: `(_UNKNOWN *)p`) */
typedef uint8_t _BOOL1;
typedef uint16_t _BOOL2;
typedef int32_t _BOOL4;
typedef uint16_t _WORD;
typedef uint8_t _BYTE;
typedef uint64_t _QWORD;
#define GAP(p, o, i) F8(p, (o) + (i))   /* IDA's gapN[i]: the byte at +N + i */
#define qmemcpy memcpy   /* IDA's inline memcpy */
static inline void *memset32(void *d, uint32_t v, size_t n) { uint32_t *p = d; while (n--) *p++ = v; return d; }   /* IDA's rep stosd */
typedef char CHAR;
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define SETB1(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff00ffu) | ((uint32_t)(uint8_t)(v) << 8)))
#define SETLODW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffff00000000ull) | (uint32_t)(v)))
#define SETHIDW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffffull) | ((uint64_t)(uint32_t)(v) << 32)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define FU32(p, o) (*(uint32_t *)((uint8_t *)(intptr_t)(p) + (o)))   /* an `unsigned int` load: >> / < / / stay unsigned */
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F64(p, o) (*(int64_t *)((uint8_t *)(intptr_t)(p) + (o)))

static inline intptr_t W_bs_5ba390(intptr_t a0, ...) { return (intptr_t)bs_5ba390((wl_char *)a0); }
static inline intptr_t W_id_5ba3a0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)id_5ba3a0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_id_5ba3b0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)id_5ba3b0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_id_5ba3d0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)id_5ba3d0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_id_5ba3f0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)id_5ba3f0((wl_char *)a0, (int32_t)a1); }

int32_t gp_468b10(wl_char *self, int32_t * a2, int32_t a3);
int32_t gp_46cb40(wl_char *self, uint32_t a2, int32_t a3);
int32_t gp_46cd10(wl_char *self, int32_t a2, int32_t a3, int32_t a4);

/* 0x468b10: int __thiscall sub_468B10(cls_468B10 *this, _DWORD *a2, int a3) */
int32_t gp_468b10(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint32_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint32_t v12 = 0;
    intptr_t v13 = 0;
    int v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    int32_t * v23 = 0;
    int32_t * v24 = 0;
    int32_t * v25 = 0;
    intptr_t v26 = 0;
    int32_t * v27 = 0;
    intptr_t ii = 0;
    intptr_t v29 = 0;
    int32_t * v30 = 0;
    int32_t * v31 = 0;
    intptr_t jj = 0;
    intptr_t v33 = 0;
    int32_t * v34 = 0;
    int32_t * v35 = 0;
    intptr_t kk = 0;
    intptr_t v37 = 0;
    int32_t * v38 = 0;
    int32_t * v39 = 0;
    intptr_t mm = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    int32_t * v46 = 0;
    int32_t * v47 = 0;
    intptr_t nn = 0;
    intptr_t v49 = 0;
    int32_t * v50 = 0;
    int32_t * v51 = 0;
    intptr_t i1 = 0;
    intptr_t v53 = 0;
    int32_t * v54 = 0;
    int32_t * v55 = 0;
    intptr_t i2 = 0;
    intptr_t v57 = 0;
    int32_t * v58 = 0;
    int32_t * v59 = 0;
    intptr_t i3 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    intptr_t v65 = 0;
    int32_t * v66 = 0;
    int32_t * v67 = 0;
    intptr_t i15 = 0;
    intptr_t v69 = 0;
    int32_t * v70 = 0;
    int32_t * v71 = 0;
    intptr_t i4 = 0;
    intptr_t v73 = 0;
    intptr_t v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    intptr_t v77 = 0;
    int32_t * v78 = 0;
    int32_t * v79 = 0;
    intptr_t i5 = 0;
    intptr_t v81 = 0;
    int32_t * v82 = 0;
    int32_t * v83 = 0;
    intptr_t i6 = 0;
    intptr_t v85 = 0;
    intptr_t v86 = 0;
    intptr_t v87 = 0;
    intptr_t v88 = 0;
    intptr_t v89 = 0;
    intptr_t v90 = 0;
    intptr_t v91 = 0;
    int32_t * v92 = 0;
    int32_t * v93 = 0;
    intptr_t i = 0;
    intptr_t v95 = 0;
    int32_t * v96 = 0;
    int32_t * v97 = 0;
    intptr_t j = 0;
    intptr_t v99 = 0;
    intptr_t v100 = 0;
    intptr_t v101 = 0;
    intptr_t v102 = 0;
    intptr_t v103 = 0;
    int32_t * v104 = 0;
    int32_t * v105 = 0;
    intptr_t m = 0;
    intptr_t v107 = 0;
    int32_t * v108 = 0;
    int32_t * v109 = 0;
    intptr_t k = 0;
    intptr_t v111 = 0;
    intptr_t v112 = 0;
    int32_t * v113 = 0;
    int32_t * v114 = 0;
    intptr_t i7 = 0;
    intptr_t v116 = 0;
    intptr_t v117 = 0;
    int32_t * v118 = 0;
    int32_t * v119 = 0;
    intptr_t i8 = 0;
    intptr_t v121 = 0;
    intptr_t v122 = 0;
    int32_t * v123 = 0;
    int32_t * v124 = 0;
    intptr_t i9 = 0;
    intptr_t v126 = 0;
    intptr_t v127 = 0;
    int32_t * v128 = 0;
    int32_t * v129 = 0;
    intptr_t i10 = 0;
    intptr_t v131 = 0;
    intptr_t v132 = 0;
    int32_t * v133 = 0;
    int32_t * v134 = 0;
    intptr_t i11 = 0;
    intptr_t v136 = 0;
    intptr_t v137 = 0;
    int32_t * v138 = 0;
    int32_t * v139 = 0;
    intptr_t i12 = 0;
    intptr_t v141 = 0;
    intptr_t v142 = 0;
    int32_t * v143 = 0;
    int32_t * v144 = 0;
    intptr_t i13 = 0;
    intptr_t v146 = 0;
    intptr_t v147 = 0;
    int32_t * v148 = 0;
    int32_t * v149 = 0;
    intptr_t i14 = 0;
    intptr_t v151 = 0;
    intptr_t v152 = 0;
    intptr_t v153 = 0;
    intptr_t v154 = 0;
    intptr_t v155 = 0;
    intptr_t v156 = 0;
    intptr_t v157 = 0;
    intptr_t v158 = 0;
    intptr_t v159 = 0;
    intptr_t v160 = 0;
    intptr_t v161 = 0;
    intptr_t v162 = 0;
    intptr_t v163 = 0;
    intptr_t v164 = 0;
    intptr_t v165 = 0;
    intptr_t v166 = 0;
    intptr_t v167 = 0;
    intptr_t v168 = 0;
    intptr_t v169 = 0;
    intptr_t v170 = 0;
    intptr_t v171 = 0;
    intptr_t v172 = 0;
    intptr_t v173 = 0;
    intptr_t v174 = 0;
    intptr_t v175 = 0;
    intptr_t v176 = 0;
    intptr_t v177 = 0;
    intptr_t v178 = 0;
    intptr_t v179 = 0;
    intptr_t v180 = 0;
    intptr_t v181 = 0;
    intptr_t v182 = 0;
    intptr_t v183 = 0;
    intptr_t v184 = 0;
    intptr_t v185 = 0;
    intptr_t v186 = 0;
    intptr_t result = 0;
    intptr_t v188 = 0;
    intptr_t v189 = 0;
    intptr_t v190 = 0;
    intptr_t v230 = 0;
    intptr_t n = 0;
    intptr_t v232 = 0;
    intptr_t v233 = 0;
    intptr_t v234 = 0;
    intptr_t v235 = 0;
    intptr_t v236 = 0;
    intptr_t v237 = 0;
    intptr_t v238 = 0;
    intptr_t v239 = 0;
    intptr_t v240 = 0;
    intptr_t v241 = 0;
    intptr_t v242 = 0;
    intptr_t v243 = 0;
    intptr_t v244 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v208__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v207__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v206__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v205__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v204__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v203__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v202__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v201__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v200__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v199__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v198__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v197__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v196__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v195__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v194__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v193__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v192__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v191__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v214__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x18 */
    intptr_t *const v213__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x18 */
    intptr_t *const v212__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x18 */
    intptr_t *const v211__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x18 */
    intptr_t *const v210__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x18 */
    intptr_t *const v209__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x18 */
    intptr_t *const v216__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x14 */
    intptr_t *const v215__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x14 */
    intptr_t *const v218__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x10 */
    intptr_t *const v217__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x10 */
    intptr_t *const v227__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v226__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v225__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v224__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v223__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v222__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v221__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v220__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v219__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v228__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x8 */
    int16_t *const v229__f = (int16_t *)(frame_ + 0x28);   /* ebp-0x4 */

  v3 = CFG();
  (*v228__f) = 0;
  v5 = F32(CFG(), 976);
  v6 = 1000 * F32(CFG(), 1340) / 1000;
  if ( v5 > 0xD8 )
  {
    switch ( v5 )
    {
      case 0x1F4u:
        v7 = a2;
        v8 = a2[14];
        if ( v8 == 0 )
          goto LABEL_33;
        v9 = v8 == 4500;
        goto LABEL_29;
      case 0x1F6u:
        v7 = a2;
        v10 = a2[14];
        if ( v10 == 4500 || v10 == 9000 || v10 == 13500 )
          goto LABEL_33;
        goto LABEL_34;
      case 0x1F8u:
        v7 = a2;
        v11 = a2[14];
        if ( v11 == 13500 || v11 == 18000 || v11 == 22500 )
          goto LABEL_33;
        goto LABEL_34;
      case 0x1FAu:
        v7 = a2;
        v8 = a2[14];
        if ( v8 == 22500 )
          goto LABEL_33;
        v9 = v8 == 27000;
LABEL_29:
        if ( v9 || v8 == 31500 )
          goto LABEL_33;
        goto LABEL_34;
      default:
LABEL_32:
        v7 = a2;
        if ( (*((uint8_t *)a2 + v5 + 72) & 0x80) != 0 )
          goto LABEL_33;
        goto LABEL_34;
    }
  }
  if ( v5 == 216 )
  {
    v7 = a2;
    if ( a2[10] <= 1000 * F32(CFG(), 1340) / -1000 )
LABEL_33:
      (*v228__f) = 1;
  }
  else
  {
    switch ( v5 )
    {
      case 0x6Cu:
        v7 = a2;
        if ( a2[8] >= v6 )
          goto LABEL_33;
        break;
      case 0x70u:
        v7 = a2;
        if ( a2[9] >= v6 )
          goto LABEL_33;
        break;
      case 0x74u:
        v7 = a2;
        if ( a2[10] >= v6 )
          goto LABEL_33;
        break;
      case 0xD0u:
        v7 = a2;
        if ( a2[8] <= 1000 * F32(CFG(), 1340) / -1000 )
          goto LABEL_33;
        break;
      case 0xD4u:
        v7 = a2;
        if ( a2[9] <= 1000 * F32(CFG(), 1340) / -1000 )
          goto LABEL_33;
        break;
      default:
        goto LABEL_32;
    }
  }
LABEL_34:
  if ( F32(v3, 260) != 0 )
  {
    v12 = F32(v3, 724);
    if ( v12 > 0xD8 )
    {
      switch ( v12 )
      {
        case 0x1F4u:
          v13 = *(_DWORD *)&GAP((v7 + (2) * 0x18), 0x0, 8);
          if ( v13 == 0 )
            goto LABEL_67;
          v14 = v13 == 4500;
          goto LABEL_63;
        case 0x1F6u:
          v15 = *(_DWORD *)&GAP((v7 + (2) * 0x18), 0x0, 8);
          if ( v15 == 4500 || v15 == 9000 || v15 == 13500 )
            goto LABEL_67;
          goto LABEL_68;
        case 0x1F8u:
          v16 = *(_DWORD *)&GAP((v7 + (2) * 0x18), 0x0, 8);
          if ( v16 == 13500 || v16 == 18000 || v16 == 22500 )
            goto LABEL_67;
          goto LABEL_68;
        case 0x1FAu:
          v13 = *(_DWORD *)&GAP((v7 + (2) * 0x18), 0x0, 8);
          if ( v13 == 22500 )
            goto LABEL_67;
          v14 = v13 == 27000;
LABEL_63:
          if ( v14 || v13 == 31500 )
            goto LABEL_67;
          goto LABEL_68;
        default:
LABEL_66:
          if ( GAP((v7 + (3) * 0x18), 0x0, v12) < 0 )
            goto LABEL_67;
          goto LABEL_68;
      }
    }
    if ( v12 == 216 )
    {
      if ( *(_DWORD *)&GAP((v7 + (1) * 0x18), 0x0, 16) <= 1000 * F32(CFG(), 1340) / -1000 )
LABEL_67:
        (*v228__f) = 2;
    }
    else
    {
      switch ( v12 )
      {
        case 0x6Cu:
          if ( *(_DWORD *)&GAP((v7 + (1) * 0x18), 0x0, 8) >= v6 )
            goto LABEL_67;
          break;
        case 0x70u:
          if ( *(_DWORD *)&GAP((v7 + (1) * 0x18), 0x0, 12) >= v6 )
            goto LABEL_67;
          break;
        case 0x74u:
          if ( *(_DWORD *)&GAP((v7 + (1) * 0x18), 0x0, 16) >= v6 )
            goto LABEL_67;
          break;
        case 0xD0u:
          if ( *(_DWORD *)&GAP((v7 + (1) * 0x18), 0x0, 8) <= 1000 * F32(CFG(), 1340) / -1000 )
            goto LABEL_67;
          break;
        case 0xD4u:
          if ( *(_DWORD *)&GAP((v7 + (1) * 0x18), 0x0, 12) <= 1000 * F32(CFG(), 1340) / -1000 )
            goto LABEL_67;
          break;
        default:
          goto LABEL_66;
      }
    }
  }
LABEL_68:
  SETLO(v17, 0);
  (*v229__f) = 0;
  if ( W_bs_5ba390((intptr_t)(v7)) != 0 )
  {
    while ( 1 )
    {
      if ( (*v228__f) == 1 )
      {
        v17 = (uint16_t)v17;
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)((uint16_t)v17)) == 4 )
        {
          v87 = W_id_5ba3b0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
          v235 = 0;
          (*v213__f) = 0;
          if ( F32(CFG(), 1296) != 0 )
          {
            v88 = F32(CFG(), 1340);
            v89 = 900 * v88 / 1000;
            if ( F32(c, 0x110) == -1 && v87 > -1000 * v89 / 1000 )
              F32(c, 0x110) = 0;
            if ( F32(c, 0x110) == 1 && v87 < 1000 * v89 / 1000 )
              F32(c, 0x110) = 0;
            if ( F32(c, 0x110) < 0 )
            {
              v90 = v87;
            }
            else
            {
              v90 = v87;
              if ( v87 <= -1000 * v88 / 1000 )
              {
                F32(c, 0x110) = -1;
                v235 = 1;
              }
            }
            if ( F32(c, 0x110) <= 0 && v90 >= 1000 * v88 / 1000 )
            {
              F32(c, 0x110) = 1;
              (*v213__f) = 1;
            }
            if ( v235 != 0 )
            {
LABEL_211:
              v91 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
              v92 = (_DWORD *)F32(c, 0x108);
              v93 = &F32(c, 0x108);
              for ( i = 63; i != 0; --i )
              {
                *v93 = *(v93 - 1);
                --v93;
              }
              F32(c, 0xc) = (int)v92;
              *v92 = 40;
              F32(F32(c, 0xc), 4) = v91;
              F32(F32(c, 0xc), 8) = 1;
            }
            if ( (*v213__f) != 0 )
            {
LABEL_215:
              v95 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
              v96 = (_DWORD *)F32(c, 0x108);
              v97 = &F32(c, 0x108);
              for ( j = 63; j != 0; --j )
              {
                *v97 = *(v97 - 1);
                --v97;
              }
              F32(c, 0xc) = (int)v96;
              *v96 = 41;
              F32(F32(c, 0xc), 4) = v95;
              F32(F32(c, 0xc), 8) = 1;
            }
          }
          else
          {
            if ( v87 <= -500 )
              goto LABEL_211;
            if ( v87 >= 500 )
              goto LABEL_215;
          }
        }
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)((uint16_t)v17)) != 0 )
          goto LABEL_245;
        v99 = W_id_5ba3b0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
        v236 = 0;
        (*v214__f) = 0;
        if ( F32(CFG(), 1296) != 0 )
        {
          v100 = F32(CFG(), 1340);
          v101 = 900 * v100 / 1000;
          if ( F32(c, 0x10c) == -1 && v99 > -1000 * v101 / 1000 )
            F32(c, 0x10c) = 0;
          if ( F32(c, 0x10c) == 1 && v99 < 1000 * v101 / 1000 )
            F32(c, 0x10c) = 0;
          if ( F32(c, 0x10c) < 0 )
          {
            v102 = v99;
          }
          else
          {
            v102 = v99;
            if ( v99 <= -1000 * v100 / 1000 )
            {
              F32(c, 0x10c) = -1;
              v236 = 1;
            }
          }
          if ( F32(c, 0x10c) <= 0 && v102 >= 1000 * v100 / 1000 )
          {
            F32(c, 0x10c) = 1;
            (*v214__f) = 1;
          }
          if ( v236 != 0 )
            goto LABEL_237;
        }
        else
        {
          if ( v99 > -500 )
          {
            if ( v99 < 500 )
              goto LABEL_245;
LABEL_241:
            v107 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v108 = (_DWORD *)F32(c, 0x108);
            v109 = &F32(c, 0x108);
            for ( k = 63; k != 0; --k )
            {
              *v109 = *(v109 - 1);
              --v109;
            }
            F32(c, 0xc) = (int)v108;
            *v108 = 43;
            v85 = F32(c, 0xc);
            v86 = v107;
LABEL_244:
            F32(v85, 4) = v86;
            F32(F32(c, 0xc), 8) = 1;
            goto LABEL_245;
          }
LABEL_237:
          v103 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
          v104 = (_DWORD *)F32(c, 0x108);
          v105 = &F32(c, 0x108);
          for ( m = 63; m != 0; --m )
          {
            *v105 = *(v105 - 1);
            --v105;
          }
          F32(c, 0xc) = (int)v104;
          *v104 = 42;
          F32(F32(c, 0xc), 4) = v103;
          F32(F32(c, 0xc), 8) = 1;
        }
        if ( (*v214__f) != 0 )
          goto LABEL_241;
        goto LABEL_245;
      }
      if ( (*v228__f) != 2 )
      {
        v17 = (uint16_t)v17;
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)((uint16_t)v17)) == 4 )
        {
          v18 = W_id_5ba3b0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
          v230 = 0;
          (*v209__f) = 0;
          (*v217__f) = 0;
          (*v215__f) = 0;
          if ( F32(CFG(), 1296) == 0 )
          {
            if ( v18 <= -500 )
              goto LABEL_89;
            if ( v18 >= 500 )
              goto LABEL_95;
            (*v215__f) = 1;
            goto LABEL_99;
          }
          v19 = F32(CFG(), 1340);
          v20 = 900 * v19 / 1000;
          if ( F32(c, 0x110) == -1 && v18 > -1000 * v20 / 1000 )
          {
            F32(c, 0x110) = 0;
            (*v217__f) = 1;
          }
          if ( F32(c, 0x110) == 1 && v18 < 1000 * v20 / 1000 )
          {
            F32(c, 0x110) = 0;
            (*v215__f) = 1;
          }
          if ( F32(c, 0x110) < 0 )
          {
            v21 = v18;
          }
          else
          {
            v21 = v18;
            if ( v18 <= -1000 * v19 / 1000 )
            {
              F32(c, 0x110) = -1;
              v230 = 1;
            }
          }
          if ( F32(c, 0x110) <= 0 && v21 >= 1000 * v19 / 1000 )
          {
            F32(c, 0x110) = 1;
            (*v209__f) = 1;
          }
          if ( v230 != 0 )
          {
LABEL_89:
            v22 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v23 = (_DWORD *)F32(c, 0x108);
            v24 = &F32(c, 0x108);
            (*v219__f) = v22;
            v25 = &F32(c, 0x108);
            for ( n = 63; n != 0; --n )
            {
              *v25 = *(v25 - 1);
              --v25;
            }
            F32(c, 0xc) = (int)v23;
            *v23 = 1;
            F32(F32(c, 0xc), 4) = (*v219__f);
            F32(F32(c, 0xc), 8) = 1;
            v26 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v27 = (_DWORD *)*v24;
            for ( ii = 63; ii != 0; --ii )
            {
              *v24 = *(v24 - 1);
              --v24;
            }
            F32(c, 0xc) = (int)v27;
            *v27 = 35;
            F32(F32(c, 0xc), 4) = v26;
            F32(F32(c, 0xc), 8) = 1;
          }
          if ( (*v209__f) != 0 )
          {
LABEL_95:
            v29 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v30 = (_DWORD *)F32(c, 0x108);
            v31 = &F32(c, 0x108);
            for ( jj = 63; jj != 0; --jj )
            {
              *v31 = *(v31 - 1);
              --v31;
            }
            F32(c, 0xc) = (int)v30;
            *v30 = 3;
            F32(F32(c, 0xc), 4) = v29;
            F32(F32(c, 0xc), 8) = 1;
          }
          if ( (*v217__f) != 0 )
          {
LABEL_99:
            v33 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v34 = (_DWORD *)F32(c, 0x108);
            v35 = &F32(c, 0x108);
            for ( kk = 63; kk != 0; --kk )
            {
              *v35 = *(v35 - 1);
              --v35;
            }
            F32(c, 0xc) = (int)v34;
            *v34 = 1;
            F32(F32(c, 0xc), 4) = v33;
            F32(F32(c, 0xc), 8) = 0;
          }
          if ( (*v215__f) != 0 )
          {
            v37 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v38 = (_DWORD *)F32(c, 0x108);
            v39 = &F32(c, 0x108);
            for ( mm = 63; mm != 0; --mm )
            {
              *v39 = *(v39 - 1);
              --v39;
            }
            F32(c, 0xc) = (int)v38;
            *v38 = 3;
            F32(F32(c, 0xc), 4) = v37;
            F32(F32(c, 0xc), 8) = 0;
          }
        }
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)((uint16_t)v17)) != 0 )
          goto LABEL_245;
        v41 = W_id_5ba3b0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
        v232 = 0;
        (*v210__f) = 0;
        (*v218__f) = 0;
        (*v216__f) = 0;
        if ( F32(CFG(), 1296) != 0 )
        {
          v42 = F32(CFG(), 1340);
          v43 = 900 * v42 / 1000;
          if ( F32(c, 0x10c) == -1 && v41 > -1000 * v43 / 1000 )
          {
            F32(c, 0x10c) = 0;
            (*v218__f) = 1;
          }
          if ( F32(c, 0x10c) == 1 && v41 < 1000 * v43 / 1000 )
          {
            F32(c, 0x10c) = 0;
            (*v216__f) = 1;
          }
          if ( F32(c, 0x10c) < 0 )
          {
            v44 = v41;
          }
          else
          {
            v44 = v41;
            if ( v41 <= -1000 * v42 / 1000 )
            {
              F32(c, 0x10c) = -1;
              v232 = 1;
            }
          }
          if ( F32(c, 0x10c) <= 0 && v44 >= 1000 * v42 / 1000 )
          {
            F32(c, 0x10c) = 1;
            (*v210__f) = 1;
          }
          if ( v232 != 0 )
          {
LABEL_125:
            v45 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v46 = (_DWORD *)F32(c, 0x108);
            v47 = &F32(c, 0x108);
            for ( nn = 63; nn != 0; --nn )
            {
              *v47 = *(v47 - 1);
              --v47;
            }
            F32(c, 0xc) = (int)v46;
            *v46 = 2;
            F32(F32(c, 0xc), 4) = v45;
            F32(F32(c, 0xc), 8) = 1;
          }
          if ( (*v210__f) != 0 )
          {
LABEL_129:
            v49 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v50 = (_DWORD *)F32(c, 0x108);
            v51 = &F32(c, 0x108);
            for ( i1 = 63; i1 != 0; --i1 )
            {
              *v51 = *(v51 - 1);
              --v51;
            }
            F32(c, 0xc) = (int)v50;
            *v50 = 4;
            F32(F32(c, 0xc), 4) = v49;
            F32(F32(c, 0xc), 8) = 1;
          }
          if ( (*v218__f) != 0 )
          {
LABEL_133:
            v53 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v54 = (_DWORD *)F32(c, 0x108);
            v55 = &F32(c, 0x108);
            for ( i2 = 63; i2 != 0; --i2 )
            {
              *v55 = *(v55 - 1);
              --v55;
            }
            F32(c, 0xc) = (int)v54;
            *v54 = 2;
            F32(F32(c, 0xc), 4) = v53;
            F32(F32(c, 0xc), 8) = 0;
          }
          if ( (*v216__f) != 0 )
          {
            v57 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
            v58 = (_DWORD *)F32(c, 0x108);
            v59 = &F32(c, 0x108);
            for ( i3 = 63; i3 != 0; --i3 )
            {
              *v59 = *(v59 - 1);
              --v59;
            }
            F32(c, 0xc) = (int)v58;
            *v58 = 4;
            F32(F32(c, 0xc), 4) = v57;
            F32(F32(c, 0xc), 8) = 0;
          }
          goto LABEL_245;
        }
        if ( v41 <= -500 )
          goto LABEL_125;
        if ( v41 >= 500 )
          goto LABEL_129;
        (*v216__f) = 1;
        goto LABEL_133;
      }
      v17 = (uint16_t)v17;
      if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)((uint16_t)v17)) != 4 )
        goto LABEL_166;
      v61 = W_id_5ba3b0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
      v233 = 0;
      (*v211__f) = 0;
      if ( F32(CFG(), 1296) != 0 )
        break;
      if ( v61 <= -500 )
        goto LABEL_159;
      if ( v61 >= 500 )
      {
LABEL_163:
        v69 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
        v70 = (_DWORD *)F32(c, 0x108);
        v71 = &F32(c, 0x108);
        for ( i4 = 63; i4 != 0; --i4 )
        {
          *v71 = *(v71 - 1);
          --v71;
        }
        F32(c, 0xc) = (int)v70;
        *v70 = 13;
        F32(F32(c, 0xc), 4) = v69;
        F32(F32(c, 0xc), 8) = 1;
      }
LABEL_166:
      if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)((uint16_t)v17)) != 0 )
        goto LABEL_245;
      v73 = W_id_5ba3b0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
      v234 = 0;
      (*v212__f) = 0;
      if ( F32(CFG(), 1296) != 0 )
      {
        v74 = F32(CFG(), 1340);
        v75 = 900 * v74 / 1000;
        if ( F32(c, 0x10c) == -1 && v73 > -1000 * v75 / 1000 )
          F32(c, 0x10c) = 0;
        if ( F32(c, 0x10c) == 1 && v73 < 1000 * v75 / 1000 )
          F32(c, 0x10c) = 0;
        if ( F32(c, 0x10c) < 0 )
        {
          v76 = v73;
        }
        else
        {
          v76 = v73;
          if ( v73 <= -1000 * v74 / 1000 )
          {
            F32(c, 0x10c) = -1;
            v234 = 1;
          }
        }
        if ( F32(c, 0x10c) <= 0 && v76 >= 1000 * v74 / 1000 )
        {
          F32(c, 0x10c) = 1;
          (*v212__f) = 1;
        }
        if ( v234 != 0 )
        {
LABEL_185:
          v77 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
          v78 = (_DWORD *)F32(c, 0x108);
          v79 = &F32(c, 0x108);
          for ( i5 = 63; i5 != 0; --i5 )
          {
            *v79 = *(v79 - 1);
            --v79;
          }
          F32(c, 0xc) = (int)v78;
          *v78 = 14;
          F32(F32(c, 0xc), 4) = v77;
          F32(F32(c, 0xc), 8) = 1;
        }
        if ( (*v212__f) != 0 )
          goto LABEL_189;
        goto LABEL_245;
      }
      if ( v73 <= -500 )
        goto LABEL_185;
      if ( v73 >= 500 )
      {
LABEL_189:
        v81 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
        v82 = (_DWORD *)F32(c, 0x108);
        v83 = &F32(c, 0x108);
        for ( i6 = 63; i6 != 0; --i6 )
        {
          *v83 = *(v83 - 1);
          --v83;
        }
        F32(c, 0xc) = (int)v82;
        *v82 = 15;
        v85 = F32(c, 0xc);
        v86 = v81;
        goto LABEL_244;
      }
LABEL_245:
      if ( a3 == 0 && W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) != 112 )
      {
        v111 = F32(CFG(), 668);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v111 + 48 )
        {
          (*v220__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v112 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          v113 = (_DWORD *)F32(c, 0x108);
          v114 = &F32(c, 0x108);
          v237 = v112;
          for ( i7 = 63; i7 != 0; --i7 )
          {
            *v114 = *(v114 - 1);
            --v114;
          }
          F32(c, 0xc) = (int)v113;
          *v113 = 7;
          F32(F32(c, 0xc), 4) = v237;
          F32(F32(c, 0xc), 8) = (*v220__f) != 0;
        }
        v116 = F32(CFG(), 696);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v116 + 48 )
        {
          (*v221__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v117 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          v118 = (_DWORD *)F32(c, 0x108);
          v119 = &F32(c, 0x108);
          v238 = v117;
          for ( i8 = 63; i8 != 0; --i8 )
          {
            *v119 = *(v119 - 1);
            --v119;
          }
          F32(c, 0xc) = (int)v118;
          *v118 = 37;
          F32(F32(c, 0xc), 4) = v238;
          F32(F32(c, 0xc), 8) = (*v221__f) != 0;
        }
        v121 = F32(CFG(), 696);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v121 + 48 )
        {
          (*v222__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v122 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          v123 = (_DWORD *)F32(c, 0x108);
          v124 = &F32(c, 0x108);
          v239 = v122;
          for ( i9 = 63; i9 != 0; --i9 )
          {
            *v124 = *(v124 - 1);
            --v124;
          }
          F32(c, 0xc) = (int)v123;
          *v123 = 8;
          F32(F32(c, 0xc), 4) = v239;
          F32(F32(c, 0xc), 8) = (*v222__f) != 0;
        }
        v126 = F32(CFG(), 724);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v126 + 48 )
        {
          (*v223__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v127 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          v128 = (_DWORD *)F32(c, 0x108);
          v129 = &F32(c, 0x108);
          v240 = v127;
          for ( i10 = 63; i10 != 0; --i10 )
          {
            *v129 = *(v129 - 1);
            --v129;
          }
          F32(c, 0xc) = (int)v128;
          *v128 = 9;
          F32(F32(c, 0xc), 4) = v240;
          F32(F32(c, 0xc), 8) = (*v223__f) != 0;
        }
        v131 = F32(CFG(), 724);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v131 + 48 )
        {
          (*v224__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v132 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          v133 = (_DWORD *)F32(c, 0x108);
          v134 = &F32(c, 0x108);
          v241 = v132;
          for ( i11 = 63; i11 != 0; --i11 )
          {
            *v134 = *(v134 - 1);
            --v134;
          }
          F32(c, 0xc) = (int)v133;
          *v133 = 17;
          F32(F32(c, 0xc), 4) = v241;
          F32(F32(c, 0xc), 8) = (*v224__f) != 0;
        }
        v136 = F32(CFG(), 752);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v136 + 48 )
        {
          (*v225__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v137 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          v138 = (_DWORD *)F32(c, 0x108);
          v139 = &F32(c, 0x108);
          v242 = v137;
          for ( i12 = 63; i12 != 0; --i12 )
          {
            *v139 = *(v139 - 1);
            --v139;
          }
          F32(c, 0xc) = (int)v138;
          *v138 = 38;
          F32(F32(c, 0xc), 4) = v242;
          F32(F32(c, 0xc), 8) = (*v225__f) != 0;
        }
        v141 = F32(CFG(), 780);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v141 + 48 )
        {
          (*v226__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v142 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          v143 = (_DWORD *)F32(c, 0x108);
          v144 = &F32(c, 0x108);
          v243 = v142;
          for ( i13 = 63; i13 != 0; --i13 )
          {
            *v144 = *(v144 - 1);
            --v144;
          }
          F32(c, 0xc) = (int)v143;
          *v143 = 20;
          F32(F32(c, 0xc), 4) = v243;
          F32(F32(c, 0xc), 8) = (*v226__f) != 0;
        }
        v146 = F32(CFG(), 808);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v146 + 48 )
        {
          (*v227__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v147 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          v148 = (_DWORD *)F32(c, 0x108);
          v149 = &F32(c, 0x108);
          v244 = v147;
          for ( i14 = 63; i14 != 0; --i14 )
          {
            *v149 = *(v149 - 1);
            --v149;
          }
          F32(c, 0xc) = (int)v148;
          *v148 = 16;
          F32(F32(c, 0xc), 4) = v244;
          F32(F32(c, 0xc), 8) = (*v227__f) != 0;
        }
        v151 = F32(CFG(), 696);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v151 + 48 )
        {
          (*v191__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v152 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 36, v152, (*v191__f));
        }
        v153 = F32(CFG(), 668);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v153 + 48 )
        {
          (*v192__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v154 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 39, v154, (*v192__f));
        }
        v155 = F32(CFG(), 836);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v155 + 48 )
        {
          (*v193__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v156 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 21, v156, (*v193__f));
        }
        v157 = F32(CFG(), 864);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v157 + 48 )
        {
          (*v194__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v158 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 10, v158, (*v194__f));
        }
        v159 = F32(CFG(), 892);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v159 + 48 )
        {
          (*v195__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v160 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 22, v160, (*v195__f));
        }
        v161 = F32(CFG(), 920);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v161 + 48 )
        {
          (*v196__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v162 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 23, v162, (*v196__f));
        }
        v163 = F32(CFG(), 1004);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v163 + 48 )
        {
          (*v197__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v164 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 11, v164, (*v197__f));
        }
        v165 = F32(CFG(), 1032);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v165 + 48 )
        {
          (*v198__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v166 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 24, v166, (*v198__f));
        }
        v167 = F32(CFG(), 1060);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v167 + 48 )
        {
          (*v199__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v168 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 25, v168, (*v199__f));
        }
        v169 = F32(CFG(), 1088);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v169 + 48 )
        {
          (*v200__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v170 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 26, v170, (*v200__f));
        }
        v171 = F32(CFG(), 1116);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v171 + 48 )
        {
          (*v201__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v172 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 27, v172, (*v201__f));
        }
        v173 = F32(CFG(), 1144);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v173 + 48 )
        {
          (*v202__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v174 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 28, v174, (*v202__f));
        }
        v175 = F32(CFG(), 1172);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v175 + 48 )
        {
          (*v203__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v176 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 29, v176, (*v203__f));
        }
        v177 = F32(CFG(), 1200);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v177 + 48 )
        {
          (*v204__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v178 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 30, v178, (*v204__f));
        }
        v179 = F32(CFG(), 1228);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v179 + 48 )
        {
          (*v205__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v180 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 31, v180, (*v205__f));
        }
        v181 = F32(CFG(), 1256);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v181 + 48 )
        {
          (*v206__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v182 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 32, v182, (*v206__f));
        }
        v183 = F32(CFG(), 1284);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v183 + 48 )
        {
          (*v207__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v184 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 33, v184, (*v207__f));
        }
        v185 = F32(CFG(), 948);
        if ( W_id_5ba3a0((intptr_t)(v7), (intptr_t)(v17)) == v185 + 48 )
        {
          (*v208__f) = W_id_5ba3d0((intptr_t)(v7), (intptr_t)(v17));
          v186 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)(v17));
          gp_46cd10(c, 18, v186, (*v208__f));
        }
      }
      if ( (uint16_t)++(*v229__f) >= (unsigned int)W_bs_5ba390((intptr_t)(v7)) )
        goto LABEL_335;
      SETLO(v17, (*v229__f));
    }
    v62 = F32(CFG(), 1340);
    v63 = 900 * v62 / 1000;
    if ( F32(c, 0x110) == -1 && v61 > -1000 * v63 / 1000 )
      F32(c, 0x110) = 0;
    if ( F32(c, 0x110) == 1 && v61 < 1000 * v63 / 1000 )
      F32(c, 0x110) = 0;
    if ( F32(c, 0x110) < 0 )
    {
      v64 = v61;
    }
    else
    {
      v64 = v61;
      if ( v61 <= -1000 * v62 / 1000 )
      {
        F32(c, 0x110) = -1;
        v233 = 1;
      }
    }
    if ( F32(c, 0x110) <= 0 && v64 >= 1000 * v62 / 1000 )
    {
      F32(c, 0x110) = 1;
      (*v211__f) = 1;
    }
    if ( v233 != 0 )
    {
LABEL_159:
      v65 = W_id_5ba3f0((intptr_t)(v7), (intptr_t)((uint16_t)v17));
      v66 = (_DWORD *)F32(c, 0x108);
      v67 = &F32(c, 0x108);
      for ( i15 = 63; i15 != 0; --i15 )
      {
        *v67 = *(v67 - 1);
        --v67;
      }
      F32(c, 0xc) = (int)v66;
      *v66 = 12;
      F32(F32(c, 0xc), 4) = v65;
      F32(F32(c, 0xc), 8) = 1;
    }
    if ( (*v211__f) != 0 )
      goto LABEL_163;
    goto LABEL_166;
  }
LABEL_335:
  if ( gp_46cb40(
         v7,
         F32(CFG(), 668),
         1000 * F32(CFG(), 1340) / 1000) != 0 )
    F32(c, 0x124) = 1;
  if ( gp_46cb40(
         v7,
         F32(CFG(), 696),
         1000 * F32(CFG(), 1340) / 1000) != 0 )
    F32(c, 0x128) = 1;
  if ( gp_46cb40(
         v7,
         F32(CFG(), 780),
         1000 * F32(CFG(), 1340) / 1000) != 0 )
    F32(c, 0x12c) = 1;
  result = (*v228__f);
  if ( (*v228__f) == 0 )
  {
    v188 = CFG();
    if ( F32(CFG(), 1296) != 0 )
    {
      v189 = *(_DWORD *)((uint8_t *)(v7 + (1) * 0x18) + 0x0);
      if ( v189 >= -1000 * F32(v188, 1340) / 1000 )
      {
        if ( v189 > 1000 * F32(v188, 1340) / 1000 )
          F32(c, 0x134) = 1;
      }
      else
      {
        F32(c, 0x130) = 1;
      }
      result = CFG();
      if ( F32(CFG(), 1296) == 2 )
      {
        result = *(_DWORD *)((uint8_t *)(v7 + (1) * 0x18) + 0x0);
        if ( result > -1000 )
        {
          if ( result >= 1000 )
            F32(c, 0x13c) = 1;
        }
        else
        {
          F32(c, 0x138) = 1;
        }
      }
    }
    else
    {
      result = *(_DWORD *)((uint8_t *)(v7 + (1) * 0x18) + 0x0);
      if ( result >= -500 )
      {
        if ( result > 500 )
          F32(c, 0x120) = 1;
      }
      else
      {
        F32(c, 0x11c) = 1;
      }
    }
    v190 = *(_DWORD *)&GAP((v7 + (1) * 0x18), 0x0, 4);
    if ( v190 >= -500 )
    {
      if ( v190 > 500 )
        F32(c, 0x118) = 1;
    }
    else
    {
      F32(c, 0x114) = 1;
    }
  }
  return result;
}

/* 0x46cb40: int __thiscall sub_46CB40(cls_46CB40 *this, unsigned int a2, int a3) */
int32_t gp_46cb40(wl_char *self, uint32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v4 = 0;
    int v5 = 0;
    intptr_t v6 = 0;
    int v7 = 0;
    intptr_t v8 = 0;

  if ( a2 > 0xD8 )
  {
    switch ( a2 )
    {
      case 0x1F4u:
        v4 = F32(c, 0x38);
        if ( v4 == 0 )
          return 1;
        v5 = v4 == 4500;
        goto LABEL_21;
      case 0x1F6u:
        v6 = F32(c, 0x38);
        if ( v6 == 4500 || v6 == 9000 )
          return 1;
        v7 = v6 == 13500;
        break;
      case 0x1F8u:
        v8 = F32(c, 0x38);
        if ( v8 == 13500 || v8 == 18000 )
          return 1;
        v7 = v8 == 22500;
        break;
      case 0x1FAu:
        v4 = F32(c, 0x38);
        if ( v4 == 22500 )
          return 1;
        v5 = v4 == 27000;
LABEL_21:
        if ( v5 )
          return 1;
        v7 = v4 == 31500;
        break;
      default:
        return *((uint8_t *)c + a2 + 72) & 0x80;
    }
    return v7;
  }
  else if ( a2 == 216 )
  {
    return F32(c, 0x28) <= -a3;
  }
  else
  {
    switch ( a2 )
    {
      case 0x6Cu:
        result = F32(c, 0x20) >= a3;
        break;
      case 0x70u:
        result = F32(c, 0x24) >= a3;
        break;
      case 0x74u:
        result = F32(c, 0x28) >= a3;
        break;
      case 0xD0u:
        result = F32(c, 0x20) <= -a3;
        break;
      case 0xD4u:
        result = F32(c, 0x24) <= -a3;
        break;
      default:
        return *((uint8_t *)c + a2 + 72) & 0x80;
    }
  }
  return result;
}

/* 0x46cd10: int __thiscall sub_46CD10(cls_46CD10 *this, int a2, int a3, int a4) */
int32_t gp_46cd10(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    int32_t * v4 = 0;
    intptr_t v5 = 0;
    int32_t * v6 = 0;
    intptr_t result = 0;

  v4 = &F32(c, 0x108);
  v5 = 63;
  v6 = (int *)F32(c, 0x108);
  do
  {
    *v4 = *(v4 - 1);
    --v4;
    --v5;
  }
  while ( v5 != 0 );
  F32(c, 0xc) = (int)v6;
  *v6 = a2;
  F32(F32(c, 0xc), 4) = a3;
  result = a4;
  F32(F32(c, 0xc), 8) = a4 != 0;
  return result;
}
/* unported callees: none */
