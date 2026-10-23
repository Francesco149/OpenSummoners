/* render_draw.c — the frame RENDER: the draw layer above the platform blitters.  See render_draw.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (IDA's control flow and types, real class layouts, stack locals at
 * their ebp offsets, Win32 calls through retail's import slots, the knowledge pack's structs).  Retail's own
 * (RC_: PORT-DEBT(engine-call-slots)): the platform layer under it (the zdd surface / bitmap session / blitters
 * 0x5b6xxx..0x5bdxxx) and the sprite-frame LOADERS 0x4184a0 / 0x4188b0 / 0x4178e0 / 0x499850 (resource / file
 * reads, the error log). */
#include "render_draw.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "asset_register.h"
#include "camera_follow.h"
#include "char_ai.h"
#include "char_attack.h"
#include "char_hit.h"
#include "char_status.h"
#include "frame_load.h"
#include "hud_view.h"
#include "npc_ai.h"
#include "particle_init.h"
#include "party_member.h"
#include "render_platform.h"
#include "room_load.h"
#include "room_loop.h"
#include "shots.h"
#include "sound_fx.h"
#include "sprite_slot.h"
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
static inline void eh_vec_ctor(intptr_t p, uint32_t size, int32_t n, void *ctor, void *dtor)   /* MSVC's `eh vector */
{                                                                                             /* constructor iterator' */
    typedef void (__attribute__((thiscall)) *c_t)(void *);
    (void)dtor;
    for (int32_t i = 0; i < n; i++) ((c_t)ctor)((void *)(p + (intptr_t)size * i));
}
static inline void eh_vec_dtor(intptr_t p, uint32_t size, int32_t n, void *dtor)   /* MSVC's `eh vector destructor iterator': */
{                                                                                  /* the elements' dtors, last first */
    typedef void (__attribute__((thiscall)) *d_t)(void *);
    p += (intptr_t)size * n;
    while (--n >= 0) { p -= size; ((d_t)dtor)((void *)p); }
}
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

static inline intptr_t W_dmg_glyphs(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)dmg_glyphs((const char *)a0, (uint16_t *)a1, (uint16_t *)a2); }
static inline intptr_t W_dramatist_row(intptr_t a0, ...) { return (intptr_t)dramatist_row((int32_t)a0); }
static inline intptr_t W_entity_part_descs(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)entity_part_descs((const wl_entity *)a0, (const wl_body *)a1, (es_desc *)a2, (uint16_t *)a3); }
static inline intptr_t W_entity_sprite_desc(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)entity_sprite_desc((const wl_entity *)a0, (const wl_body *)a1, (es_desc *)a2); }
static inline intptr_t W_entity_sprite_descs(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)entity_sprite_descs((const wl_entity *)a0, (const wl_body *)a1, (es_desc *)a2, (uint16_t *)a3); }
static inline intptr_t W_fl_4178e0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fl_4178e0((wl_char *)a0, (void *)a1); }
static inline intptr_t W_fl_4184a0(intptr_t a0, intptr_t a1, ...) { fl_4184a0((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_fl_499850(intptr_t a0, ...) { fl_499850((int32_t)a0); return 0; }
static inline intptr_t W_member_skill_at(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)member_skill_at((const uint8_t *)a0, (int32_t)a1, (uint16_t)a2); }
static inline intptr_t W_pal_swap_apply(intptr_t a0, intptr_t a1, intptr_t a2, ...) { pal_swap_apply((void *)a0, (const uint8_t *)a1, (uint8_t *)a2); return 0; }
static inline intptr_t W_palette_tint(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { palette_tint((uint8_t *)a0, (const uint8_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); return 0; }
static inline intptr_t W_part_center_x(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_center_x((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_part_center_y(intptr_t a0, intptr_t a1, ...) { return (intptr_t)part_center_y((const wl_entity *)a0, (uint16_t)a1); }
static inline intptr_t W_party_slot_get(intptr_t a0, ...) { return (intptr_t)party_slot_get((wl_party *)a0); }
static inline intptr_t W_pslot_ok(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)pslot_ok((const wl_pslot *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_rp_5b5eb0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)rp_5b5eb0((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_rp_5b8b40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)rp_5b8b40((wl_char *)a0, (int32_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_rp_5b9450(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b9450((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_rp_5b9490(intptr_t a0, ...) { return (intptr_t)rp_5b9490((wl_char *)a0); }
static inline intptr_t W_rp_5b94d0(intptr_t a0, ...) { return (intptr_t)rp_5b94d0((wl_char *)a0); }
static inline intptr_t W_rp_5b94e0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b94e0((wl_char *)a0, (intptr_t *)a1); }
static inline intptr_t W_rp_5b9500(intptr_t a0, intptr_t a1, ...) { rp_5b9500((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_rp_5b9520(intptr_t a0, ...) { return (intptr_t)rp_5b9520((wl_char *)a0); }
static inline intptr_t W_rp_5b9a40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)rp_5b9a40((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_rp_5b9ae0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)rp_5b9ae0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_rp_5b9b70(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)rp_5b9b70((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_rp_5b9bf0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, ...) { return (intptr_t)rp_5b9bf0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7); }
static inline intptr_t W_rp_5bd550(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, ...) { return (intptr_t)rp_5bd550((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10); }
static inline intptr_t W_rp_5bd680(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)rp_5bd680((wl_char *)a0, (int32_t **)a1, (int32_t **)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_sfx_play(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)sfx_play((void *)a0, (uint16_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_shot_476550(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)shot_476550((int32_t)a0, (int32_t)a1, (int32_t *)a2, (double *)a3); }
static inline intptr_t W_shot_476d30(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)shot_476d30((wl_char *)a0, (int32_t)a1, (uint16_t)a2, (uint16_t)a3, (int32_t)a4); }
static inline intptr_t W_skill_row_find(intptr_t a0, ...) { return (intptr_t)skill_row_find((int32_t)a0); }
static inline intptr_t W_slot_palette_set(intptr_t a0, intptr_t a1, ...) { slot_palette_set((ar_sprite_slot *)a0, (const uint8_t *)a1); return 0; }
static inline intptr_t W_sprite_slot_tint(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { sprite_slot_tint((void *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (const uint8_t *)a4); return 0; }
static inline intptr_t W_stats_mp_ratio(intptr_t a0, ...) { return (intptr_t)stats_mp_ratio((const uint8_t *)a0); }
static inline intptr_t W_tint_fade_a(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tint_fade_a((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_tint_fade_b(intptr_t a0, intptr_t a1, ...) { return (intptr_t)tint_fade_b((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_view_box_visible(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)view_box_visible((const camera_view *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_view_project(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)view_project((const camera_view *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t *)a7, (int32_t *)a8, (int32_t)a9); }

typedef intptr_t (__attribute__((stdcall)) *api_Arc_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_Arc ((api_Arc_t)g_eng.api("Arc"))
typedef intptr_t (__attribute__((stdcall)) *api_Polyline_t)(intptr_t, intptr_t, intptr_t);
#define API_Polyline ((api_Polyline_t)g_eng.api("Polyline"))
typedef intptr_t (__attribute__((stdcall)) *api_SelectObject_t)(intptr_t, intptr_t);
#define API_SelectObject ((api_SelectObject_t)g_eng.api("SelectObject"))
typedef intptr_t (__attribute__((stdcall)) *api_SetBkMode_t)(intptr_t, intptr_t);
#define API_SetBkMode ((api_SetBkMode_t)g_eng.api("SetBkMode"))
typedef uint32_t (__attribute__((stdcall)) *api_SetTextColor_t)(intptr_t, intptr_t);
#define API_SetTextColor ((api_SetTextColor_t)g_eng.api("SetTextColor"))
typedef intptr_t (__attribute__((stdcall)) *api_TextOutA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_TextOutA ((api_TextOutA_t)g_eng.api("TextOutA"))
int32_t rd_48c150(wl_char *self, int32_t a2, int32_t a3, int32_t a4);
int32_t rd_418470(wl_char *self, uint16_t a2);
int32_t rd_48c6b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t * a13);
int32_t rd_48c820(wl_char *self, int32_t a2, int32_t hdcSrc);
int32_t rd_48cb90(wl_char *self, int32_t a2, int32_t hdcSrc);
int32_t rd_48d3d0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int16_t a8, int16_t a9, int16_t a10, int32_t a11, int32_t hdcSrc);
int32_t rd_48cf80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t hdcSrc);
int32_t rd_48d670(int32_t a1, int32_t a2, int16_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t hdcSrc);
int32_t rd_48d710(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t hdcSrc);
int32_t rd_48d940(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t hdcSrc);
int32_t rd_48da70(wl_char *self, int32_t a2, int32_t hdc, int32_t a4, int32_t a5, int32_t h, int32_t a7, int32_t a8);
int32_t rd_48e100(wl_char *self, int32_t hdc, int32_t a3, int32_t a4, int32_t h);
int32_t rd_48e200(wl_char *self, int32_t a2, int32_t a3, int32_t hdc, int32_t a5, int32_t a6, int32_t h, int32_t a8, int32_t hdcSrc);
int32_t rd_48e6d0(wl_char *self, int32_t hdc, int32_t a3, int32_t y, int32_t a5, uint16_t a6, uint16_t a7);
int32_t rd_48e860(wl_char *self, int32_t hdc, int32_t a3, int32_t y, int16_t a5, uint16_t a6, uint16_t a7);
int32_t rd_48e920(wl_char *self, int32_t a2);
void rd_48eac0(wl_char *self, int32_t a2);
int32_t rd_48ef40(wl_char *self, int32_t hdc, int32_t a3, int32_t a4);
double rd_490090(double a1, double a2);
int32_t rd_4900d0(int32_t a1, double a2, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6);
int32_t rd_490160(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rd_4903f0(int32_t a1, int32_t hdc, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
void rd_490860(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint8_t * a8);
int32_t rd_490cd0(wl_char *self, int32_t a2, int32_t * a3);
int32_t rd_417c40(wl_char *self, uint16_t a2, uint16_t a3);
int32_t rd_490f30(wl_char *self, int32_t * a2, int32_t a3);
int32_t rd_4917b0(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9);
int32_t rd_491820(wl_char *self, int32_t a2, char * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11);
int32_t rd_491ae0(wl_char *self, uint16_t a2, int32_t a3);
int32_t rd_4925e0(wl_char *self, uint16_t a2, int32_t * a3, int32_t * a4, int32_t * a5, int32_t * a6);
int32_t rd_492670(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rd_4926f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rd_4927c0(uint16_t a1, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t rd_4928f0(int32_t a1, uint16_t a2, uint16_t a3, int32_t a4, int32_t a5, uint16_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14);
int32_t rd_492a90(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5);
void rd_492c40(wl_char *self, int32_t * a2, int32_t ** a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14);
void rd_492f30(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12);
int32_t rd_492fc0(int32_t a1, int32_t * a2, int32_t a3, int32_t a4);
int32_t rd_493230(int32_t a1, int32_t a2, int32_t a3);
int32_t rd_493480(wl_char *self, int32_t a2, int32_t * a3);
int32_t rd_4937c0(wl_char *self, int32_t a2, int32_t a3);
int32_t rd_493a50(int32_t * a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rd_493ba0(wl_char *self, uint16_t a2, int32_t * a3, int32_t a4);
int32_t rd_494790(wl_char *self, int32_t a2, int32_t * a3);
int32_t rd_494a60(wl_char *self, int32_t a2);
int32_t rd_494e10(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rd_494e60(wl_char *self, int32_t a2);
void rd_43e250(uint16_t a1, int32_t a2, int32_t hdc, int32_t y, int32_t a5, int32_t a6);
int32_t rd_495dc0(int32_t a1, uint16_t a2, char * Buffer);
int32_t rd_495e40(wl_char *self, int32_t a2, char * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t rd_495fe0(wl_char *self, int32_t a2, uint16_t a3, int32_t * a4, int32_t a5, uint16_t a6);
int32_t rd_496170(wl_char *self);
int32_t rd_4961a0(wl_char *self);
int32_t rd_4961e0(wl_char *self, int32_t a2, int32_t a3);
int32_t rd_496240(wl_char *self, int32_t a2, int32_t a3);
int32_t rd_4962a0(int32_t a1, uint16_t a2, uint16_t a3, int32_t a4, int32_t a5);
void rd_4963a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t rd_496560(wl_char *self, int32_t a2, int32_t * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int16_t a12, int32_t * a13);
int32_t rd_498f10(int32_t a1, int32_t a2, int32_t y1, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, uint16_t a10, uint16_t a11, int32_t a12);
int32_t rd_4969b0(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t rd_496ec0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t rd_4975a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t x1, int32_t y1, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11);
void rd_4975e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t rd_497a00(int32_t a1, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rd_497b40(int32_t * a1, int32_t a2);
int32_t rd_497bb0(int32_t * a1, int32_t a2);
void rd_497c20(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
int32_t rd_497f40(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5);
int32_t rd_498620(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t rd_498680(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t y1, int32_t a7, int32_t a8, int32_t a9, int32_t a10, uint16_t a11, int32_t a12);
int32_t rd_498820(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t y1, int32_t a6, int32_t a7, int32_t a8, uint16_t a9);
int32_t rd_498960(int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t rd_499070(uint16_t a1);
void rd_499100(wl_char *self, int32_t a2);
void rd_499560(int32_t a1, int32_t * a2, int32_t a3, int32_t a4);
int32_t rd_499670(wl_char *self, int32_t a2, uint8_t * a3, int32_t a4, uint32_t a5);
int32_t rd_4997b0(int32_t a1, int32_t a2);
int32_t rd_49c170(int32_t a1);

/* 0x48c150: int __thiscall sub_48C150(cls_48C150 *this, int a2, int a3, int a4) */
int32_t rd_48c150(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t j = 0;
    intptr_t v14 = 0;
    intptr_t k = 0;
    intptr_t v16 = 0;
    intptr_t m = 0;
    intptr_t v18 = 0;
    intptr_t n = 0;
    int32_t * v20 = 0;
    intptr_t ii = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t jj = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t field_44 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    int32_t * p_field_124 = 0;
    intptr_t kk = 0;
    intptr_t v42 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0xc);   /* [20] at ebp-0x14 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x20 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1c */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x18 */

  v5 = *(_DWORD *)&GAP(W, 0x1030, 28);
  v6 = 0;
  for ( (*i__f) = c; (uint16_t)v6 < F16(v5, 88); F16((F32(v5, 84) + 8 * v7), 0) = 0 )
    v7 = (uint16_t)v6++;
  memset((void *)(*(_DWORD *)&GAP(W, 0x1030, 24) + 1638448), 0, 0x5000u);
  if ( *(_WORD *)&GAP(c, 0x28, 8) != 0 )
  {
    v8 = SYS32(4 * *(uint16_t *)&GAP(c, 0x28, 8) + 2732);
    if ( **(_DWORD **)((uint8_t *)(v8) + 0x0) == 0 )
      W_fl_4184a0((intptr_t)(v8), (intptr_t)(0));
    W_rp_5b9a40((intptr_t)(***(intptr_t ***)((uint8_t *)(v8) + 0x0)), (intptr_t)(F32(SYS32(10348), 364)), (intptr_t)(0), (intptr_t)(0));
LABEL_65:
    v9 = W;
    goto LABEL_66;
  }
  v9 = W;
  v10 = *(_DWORD *)&GAP(W, 0x1030, 16);
  if ( F32(v10, 12) == 1 )
    goto LABEL_12;
  if ( F32(v10, 12) == 2 )
  {
    if ( F32(v10, 24) != 0 )
      goto LABEL_66;
LABEL_12:
    if ( F32(c, 0x1c) != 0 )
    {
      v11 = rd_48c6b0(
                            SYS32(10068),
                            1003,
                            0,
                            0,
                            0,
                            0,
                            0,
                            SYS32(10348),
                            640,
                            480,
                            0x1FFFFFF,
                            0,
                            &(*v45__f));
      if ( v11 != 0 )
      {
        if ( (*v45__f) == 0 )
        {
          W_rp_5b9520((intptr_t)(v11));
          rd_499100(c, (int)v11);
        }
        W_rp_5b9a40((intptr_t)(v11), (intptr_t)(a2), (intptr_t)(0), (intptr_t)(0));
      }
      else
      {
        F32(c, 0x1c) = 0;
      }
    }
    else
    {
      rd_490cd0(*(intptr_t *)&GAP(W, 0x1030, 24), a2, *(_DWORD **)&GAP(W, 0x1030, 28));
      v12 = W;
      for ( j = 4576; j < 5088; j += 4 )
      {
        v14 = *(intptr_t *)&GAP(v12, 0x0, j);
        if ( F32(v14, 0x1d0) != 0 )
        {
          rd_491ae0(v14, 0, *(_DWORD *)&GAP(v12, 0x1030, 28));
          v12 = W;
        }
      }
      for ( k = 4192; k < 4448; k += 4 )
      {
        v16 = *(_DWORD *)&GAP(v12, 0x0, k);
        if ( F32(v16, 464) != 0 )
        {
          rd_4937c0(v16, 0, *(_DWORD *)&GAP(v12, 0x1030, 28));
          v12 = W;
        }
      }
      for ( m = 5088; m < 9184; m += 4 )
      {
        v18 = *(intptr_t *)&GAP(v12, 0x0, m);
        if ( F32(v18, 0x1d0) != 0 )
        {
          rd_493480(v18, 0, *(_DWORD **)&GAP(v12, 0x1030, 28));
          v12 = W;
        }
      }
      for ( n = 9184; n < 9568; n += 4 )
      {
        v20 = *(_DWORD **)&GAP(v12, 0x0, n);
        if ( v20[116] != 0 )
        {
          rd_492fc0(*(_DWORD *)&GAP(v12, 0x1030, 28), v20, (int)c, *(_DWORD **)&GAP(v12, 0x1030, 28));
          v12 = W;
        }
      }
      for ( ii = 9568; ii < 10080; ii += 4 )
      {
        v22 = *(_DWORD *)&GAP(v12, 0x0, ii);
        v23 = F32(v22, 464);
        if ( v23 != 0 )
        {
          rd_493230(v22, v23, *(_DWORD **)&GAP(v12, 0x1030, 28));
          v12 = W;
        }
      }
      for ( jj = 4448; jj < 4576; jj += 4 )
      {
        v25 = *(intptr_t *)&GAP(v12, 0x0, jj);
        if ( F32(v25, 0x1d0) != 0 )
        {
          rd_493ba0(v25, 0, *(_DWORD **)&GAP(v12, 0x1030, 28), 0);
          v12 = W;
        }
      }
      v26 = F16(c, 0x4) - 1;
      if ( v26 >= 0 )
      {
        v27 = F16(c, 0x4);
        v28 = 16672 * v26;
        do
        {
          rd_4997b0(v28 + F32(c, 0x0), 0);
          rd_4997b0(v28 + F32(c, 0x0), 1);
          v28 -= 16672;
          --v27;
        }
        while ( v27 != 0 );
        v12 = W;
      }
      rd_490f30(*(intptr_t *)&GAP(v12, 0x1030, 24), *(_DWORD **)&GAP(v12, 0x1030, 28), 1);
      rd_48eac0(*(intptr_t *)&GAP(W, 0x1030, 28), a2);
    }
    v29 = *(_DWORD *)&F16(c, 0x44);
    if ( v29 > 0 )
    {
      v30 = rd_418470(SYS32(2948), 0);
      (*v45__f) = 0;
      v31 = 4 * ((v29 + 3) / 4);
      v32 = 480 - v29;
      (*v46__f) = 480 - v29;
      if ( v31 > 0 )
      {
        while ( 1 )
        {
          v33 = 0;
          v34 = (*v45__f) + v32;
          do
          {
            W_rp_5b9a40((intptr_t)(v30), (intptr_t)(a2), (intptr_t)(v33), (intptr_t)(v34));
            v33 += 64;
          }
          while ( v33 + 64 <= 640 );
          (*v45__f) += 4;
          if ( (*v45__f) >= v31 )
            break;
          v32 = (*v46__f);
        }
      }
      c = (*i__f);
    }
    field_44 = F32(c, 0x48);
    if ( field_44 > 0 )
    {
      v36 = rd_418470(SYS32(2948), 0);
      (*v45__f) = 0;
      v37 = 4 * ((field_44 + 3) / 4);
      (*v46__f) = field_44 - v37;
      if ( v37 > 0 )
      {
        do
        {
          v38 = 0;
          v39 = (*v45__f) + (*v46__f);
          do
          {
            W_rp_5b9a40((intptr_t)(v36), (intptr_t)(a2), (intptr_t)(v38), (intptr_t)(v39));
            v38 += 64;
          }
          while ( v38 + 64 <= 640 );
          (*v45__f) += 4;
        }
        while ( (*v45__f) < v37 );
      }
      c = (*i__f);
    }
    p_field_124 = &F32(c, 0x128);
    for ( kk = 2; kk != 0; --kk )
    {
      rd_494e60(p_field_124, *(intptr_t *)(SYS32(10348) + 364));
      p_field_124 += 251;
    }
    if ( a3 != 0 )
      rd_494790(
        &F32(c, 0xf0),
        F32(SYS32(10348), 364),
        *(_DWORD **)&GAP(W, 0x1030, 28));
    goto LABEL_65;
  }
  if ( F32(v10, 12) != 3 )
    goto LABEL_12;
LABEL_66:
  if ( *(_DWORD *)&GAP((v9 + (1) * 0x27a4), 0x0, 16) == 2 )
  {
    v42 = *(_DWORD *)&GAP((v9 + (1) * 0x27a4), 0x0, 12);
    if ( v42 != 0 )
    {
      rd_49c170(F32(v42, 4));
      v9 = W;
    }
  }
  rd_48e920(
    *(intptr_t *)&GAP(v9, 0x1030, 16),
    F32(SYS32(10348), 364));
  rd_494a60(*(intptr_t *)&GAP(c, 0xf4, 40), F32(SYS32(10348), 364));
  rd_494a60(F32(c, 0x120), F32(SYS32(10348), 364));
  rd_494a60(F32(c, 0x124), F32(SYS32(10348), 364));
  rd_48c820(
    F32((uint8_t *)(c), 0xe8),
    *(intptr_t *)(SYS32(10348) + 364),
    *(intptr_t *)&SYSB(864));
  if ( F32(CFG(), 176) == 1 && a4 != 0 )
  {
    g_eng.h5bf3ee(Buffer, "FPS %d", F32(a4, 36));
    rd_491820(
      (intptr_t *)((intptr_t)g_eng.sys),
      F32(SYS32(10348), 364),
      Buffer,
      1u,
      9,
      544,
      460,
      1,
      5,
      0,
      0);
  }
  result = F32(((uint8_t *)(c) + 0x34), 0);
  if ( result != 0 )
  {
    F32(((uint8_t *)(c) + 0x34), 0) = 0;
    W_fl_499850((intptr_t)(a2));
    return W_sfx_play((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(4u), (intptr_t)(0), (intptr_t)(0));
  }
  return result;
}

/* 0x418470: int __thiscall sub_418470(cls_418470 *this, unsigned __int16 a2) */
int32_t rd_418470(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;

  if ( *(_DWORD *)F32(c, 0x0) == 0 )
    W_fl_4184a0((intptr_t)(c), (intptr_t)(0));
  return F32((*(_DWORD *)F32(c, 0x0) + 4 * a2), 0);
}

/* 0x48c6b0: int __thiscall sub_48C6B0( cls_48C6B0 *this, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, _DWORD *a13) */
int32_t rd_48c6b0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t * a13)
{
    intptr_t c = (intptr_t)self;
    uint16_t v14 = 0;
    uint16_t v15 = 0;
    int32_t * v16 = 0;
    uint16_t v17 = 0;
    uint16_t v18 = 0;
    intptr_t v19 = 0;

  v14 = 0;
  v15 = F16(c, 0x6);
  if ( v15 != 0 )
  {
    while ( 1 )
    {
      v16 = (_DWORD *)(F32(c, 0x0) + 28 * v14);
      if ( *v16 == a2 && v16[1] == a3 && v16[2] == a4 && v16[3] == a5 && v16[4] == a6 && v16[5] == a7 && v16[6] != 0 )
        break;
      if ( ++v14 >= v15 )
        goto LABEL_10;
    }
    *a13 = 1;
    return F32(F32(c, 0x0) + 28 * v14, 24);
  }
  else
  {
LABEL_10:
    v17 = 0;
    *a13 = 0;
    v18 = F16(c, 0x6);
    if ( v18 != 0 )
    {
      do
      {
        if ( F32(F32(c, 0x0) + 28 * v17, 24) == 0 )
          break;
        ++v17;
      }
      while ( v17 < v18 );
    }
    if ( v17 < F16(c, 0x4)
      && (v19 = 28 * v17, W_rp_5b8b40((intptr_t)(a8), (intptr_t)((_DWORD *)(v19 + F32(c, 0x0) + 24)), (intptr_t)(a9), (intptr_t)(a10), (intptr_t)(a11), (intptr_t)(a12)) != 0) )
    {
      if ( F16(c, 0x6) <= v17 )
        F16(c, 0x6) = v17 + 1;
      F32((v19 + F32(c, 0x0)), 0) = a2;
      F32(v19 + F32(c, 0x0), 4) = a3;
      F32(v19 + F32(c, 0x0), 8) = a4;
      F32(v19 + F32(c, 0x0), 12) = a5;
      F32(v19 + F32(c, 0x0), 16) = a6;
      F32(v19 + F32(c, 0x0), 20) = a7;
      return F32(v19 + F32(c, 0x0), 24);
    }
    else
    {
      return 0;
    }
  }
}

/* 0x48c820: int __thiscall sub_48C820(cls_48C820 *this, paint_ctx *a2, HDC hdcSrc) */
int32_t rd_48c820(wl_char *self, int32_t a2, int32_t hdcSrc)
{
    intptr_t c = (intptr_t)self;
    intptr_t ** v3 = 0;
    intptr_t ** v4 = 0;
    intptr_t result = 0;
    int v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint32_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t ** v23 = 0;
    intptr_t ** v24 = 0;
    intptr_t v25 = 0;
    int16_t v26 = 0;
    int16_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *const v33__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x2c */
    uint16_t *const v32__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x28 */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x24 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x20 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x1c */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x18 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x14 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x10 */
    intptr_t *const h__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x8 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x4 */

  v3 = *((intptr_t ***)c + 2);
  (*v36__f) = c;
  if ( v3 != 0 )
    (*h__f) = **v3;
  v4 = *((intptr_t ***)c + 3);
  if ( v4 != 0 )
    (*v40__f) = **v4;
  result = 0;
  v6 = F16(c, 2 * 3) == 0;
  (*v39__f) = 0;
  if ( !v6 )
  {
    while ( 1 )
    {
      v7 = F32((F32(c, 0x0) + 4 * (uint16_t)result), 0);
      if ( F32(v7, 4) != 0 )
        break;
LABEL_47:
      result = (*v39__f) + 1;
      if ( (uint16_t)++(*v39__f) >= F16(c, 2 * 3) )
        return result;
    }
    if ( F32(v7, 128) == 0 )
    {
      v8 = F32(v7, 28);
      if ( v8 == 0 )
        goto LABEL_13;
      v9 = v8 - 1;
      if ( v9 == 0 )
      {
        v10 = F32(v7, 84);
        v11 = v10 * F32(v7, 20);
        v12 = (int)((uint64_t)(274877907LL * v10 * F32(v7, 24)) >> 32) >> 6;
        v13 = hdcSrc;
        v14 = a2;
        rd_48cf80(
          v7,
          (int)a2,
          F32(v7, 12) + F32(v7, 20) / 2 - v11 / 1000 / 2,
          F32(v7, 16) + F32(v7, 24) / 2 - (int)((v12 >> 31) + v12) / 2,
          v11 / 1000,
          (v12 >> 31) + v12,
          F32(v7, 124),
          hdcSrc);
        goto LABEL_15;
      }
      if ( v9 == 1 )
LABEL_13:
        rd_48cf80(
          v7,
          (int)a2,
          F32(v7, 12),
          F32(v7, 16),
          F32(v7, 20),
          F32(v7, 24),
          F32(v7, 124),
          hdcSrc);
    }
    v14 = a2;
    v13 = hdcSrc;
LABEL_15:
    if ( F32(v7, 84) >= 1000 || F32(v7, 88) != 0 )
    {
      v6 = F16(v7, 78) == 0;
      (*v43__f) = 1;
      (*v32__f) = 0;
      if ( !v6 )
      {
        do
        {
          v16 = F32(v7, 16);
          v17 = F32(v7, 12);
          v18 = F32((F32(v7, 72) + 4 * (*v32__f)), 0);
          if ( F32(v18, 8) == 1 )
          {
            rd_48e200(v18, 0, (int)v14, 0, v17, v16, 0, 0, v13);
          }
          else if ( F32(v18, 8) == 2 )
          {
            rd_48da70(v18, (int)v14, 0, v17, v16, 0, 0, (int)v13);
          }
          ++(*v32__f);
        }
        while ( (*v32__f) < F16(v7, 78) );
      }
      W_rp_5b94e0((intptr_t)(v14), (intptr_t)((intptr_t *)&(*hdc__f)));
      API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
      (*v33__f) = 0;
      if ( F16(v7, 78) != 0 )
      {
        do
        {
          v19 = (*v40__f);
          v20 = (*h__f);
          v21 = F32(v7, 16);
          (*v34__f) = F32(v7, 12);
          v22 = F32((F32(v7, 72) + 4 * (*v33__f)), 0);
          (*v37__f) = (*v40__f);
          (*v38__f) = (*hdc__f);
          v23 = *(intptr_t ***)(v22 + 420);
          if ( v23 != 0 )
            v20 = **v23;
          v24 = *(intptr_t ***)(v22 + 424);
          if ( v24 != 0 )
          {
            v19 = **v24;
            (*v37__f) = v19;
          }
          if ( F32(v22, 8) == 1 )
          {
            v25 = F32(v22, 372);
            if ( F32(v25, 0) == 2 )
            {
              v28 = F32(v25, 12);
              (*v42__f) = (F32(v25, 16) - 1) / v28 + 1;
              v29 = F32(F32((v22 + 372), 0), 20) / v28;
              F16(v22, 354) = (*v42__f);
              v19 = (*v37__f);
              F16(v22, 352) = v29 + 1;
            }
            else if ( F32(v25, 0) == 3 )
            {
              v26 = F16(v25, 16);
              v27 = F16(v25, 20) + 1;
              F16(v22, 354) = v26;
              F16(v22, 352) = v27;
            }
            rd_48e200(v22, 1, 0, (*v38__f), (*v34__f), v21, v20, v19, hdcSrc);
          }
          else if ( F32(v22, 8) == 2 )
          {
            rd_48da70(v22, 0, (*hdc__f), (*v34__f), v21, v20, v19, 0);
          }
          rd_48e100(v22, (*v38__f), (*v34__f), v21, v20);
          ++(*v33__f);
        }
        while ( (*v33__f) < F16(v7, 78) );
        v14 = a2;
        v13 = hdcSrc;
      }
      W_rp_5b9500((intptr_t)(v14), (intptr_t)((intptr_t)(*hdc__f)));
      v15 = (*v43__f);
    }
    else
    {
      v15 = 0;
    }
    if ( F32(v7, 128) == 1 )
      rd_48cb90(v7, (int)v14, v13);
    if ( v15 != 0 && F32(v7, 8) != 0 )
    {
      v30 = F32((F32(v7, 72) + 4 * F16(v7, 78) - 4), 0);
      if ( v30 != 0 )
      {
        rd_48d940(v30, (int)v14, F32(v7, 12), F32(v7, 16), v13);
        rd_48d710(v30, (int)v14, F32(v7, 12), F32(v7, 16), v13);
      }
    }
    c = (*v36__f);
    goto LABEL_47;
  }
  return result;
}

/* 0x48cb90: int __thiscall sub_48CB90(cls_48CB90 *this, int a2, HDC hdcSrc) */
int32_t rd_48cb90(wl_char *self, int32_t a2, int32_t hdcSrc)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    int v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8 */

  (*v25__f) = F32(c, 0x7c);
  v4 = F32(c, 0x1c);
  if ( v4 == 0 )
  {
LABEL_20:
    v16 = F32(c, 0x74);
    v17 = F32(c, 0x14);
    v18 = F32(c, 0xc);
    v19 = F32(c, 0x10);
    v20 = F32(c, 0x78);
    (*v29__f) = F32(c, 0x18);
    v21 = 2 * v16;
    (*v27__f) = v18;
    (*v31__f) = v16;
    if ( v17 >= 2 * v16 || (result = 2 * v20, (*v29__f) >= 2 * v20) )
    {
      result = F32(c, 0x5c);
      if ( result != 0 )
      {
        if ( v17 < v21 )
        {
          v22 = v21 - v17;
          v17 = v21;
          (*v27__f) = v22 / -2 + v18;
        }
        v23 = (*v29__f);
        if ( (*v29__f) < 2 * v20 )
        {
          (*v29__f) = 2 * v20;
          v19 += (2 * v20 - v23) / -2;
        }
        rd_48d3d0(
          a2,
          (int)&F32(c, 0x5c),
          (*v27__f),
          v19,
          v20,
          v17,
          (*v31__f),
          F16(c, 0x62),
          F16(c, 0x64),
          F16(c, 0x66),
          (*v25__f),
          hdcSrc);
        v24 = (*v29__f) / v20 - 1;
        if ( v24 > 1 )
        {
          (*v33__f) = v20 + v19;
          (*v35__f) = (*v29__f) / v20 - 2;
          do
          {
            rd_48d3d0(
              a2,
              (int)&F32(c, 0x5c),
              (*v27__f),
              (*v33__f),
              v20,
              v17,
              (*v31__f),
              F16(c, 0x68),
              F16(c, 0x6a),
              F16(c, 0x6c),
              (*v25__f),
              hdcSrc);
            v15 = (*v35__f) == 1;
            (*v33__f) += v20;
            --(*v35__f);
          }
          while ( !v15 );
          v24 = (*v29__f) / v20 - 1;
        }
        if ( (*v29__f) % v20 > 0 )
          rd_48d3d0(
            a2,
            (int)&F32(c, 0x5c),
            (*v27__f),
            v19 + v20 * v24,
            (*v29__f) % v20,
            v17,
            (*v31__f),
            F16(c, 0x68),
            F16(c, 0x6a),
            F16(c, 0x6c),
            (*v25__f),
            hdcSrc);
        return rd_48d3d0(
                 a2,
                 (int)&F32(c, 0x5c),
                 (*v27__f),
                 (*v29__f) + v19 - v20,
                 v20,
                 v17,
                 (*v31__f),
                 F16(c, 0x6e),
                 F16(c, 0x70),
                 F16(c, 0x72),
                 (*v25__f),
                 hdcSrc);
      }
    }
    return result;
  }
  v5 = v4 - 1;
  if ( v5 != 0 )
  {
    result = v5 - 1;
    if ( result != 0 )
      return result;
    goto LABEL_20;
  }
  v7 = F32(c, 0x18);
  v8 = F32(c, 0x54);
  (*v28__f) = v8 * F32(c, 0x18) / 1000;
  v9 = v8 * F32(c, 0x14) / 1000;
  (*v30__f) = v9;
  v10 = F32(c, 0x10) + v7 / 2 - (*v28__f) / 2;
  v11 = F32(c, 0x78);
  v12 = F32(c, 0xc) + F32(c, 0x14) / 2 - v9 / 2;
  result = F32(c, 0x74);
  (*v26__f) = result;
  if ( v9 < 2 * result )
  {
    result = (*v28__f);
    if ( (*v28__f) < 2 * v11 )
      return result;
    result = (*v26__f);
  }
  if ( F32(c, 0x5c) != 0 )
  {
    v13 = 2 * result;
    if ( v9 < v13 )
    {
      v12 += (v13 - v9) / -2;
      (*v30__f) = 2 * (*v26__f);
      v9 = 2 * (*v26__f);
    }
    if ( (*v28__f) < 2 * v11 )
    {
      v10 += (2 * v11 - (*v28__f)) / -2;
      (*v28__f) = 2 * v11;
    }
    rd_48d3d0(
      a2,
      (int)&F32(c, 0x5c),
      v12,
      v10,
      v11,
      v9,
      (*v26__f),
      F16(c, 0x62),
      F16(c, 0x64),
      F16(c, 0x66),
      (*v25__f),
      hdcSrc);
    v14 = (*v28__f) / v11 - 1;
    if ( v14 > 1 )
    {
      (*v32__f) = v10 + v11;
      (*v34__f) = (*v28__f) / v11 - 2;
      do
      {
        rd_48d3d0(
          a2,
          (int)&F32(c, 0x5c),
          v12,
          (*v32__f),
          v11,
          (*v30__f),
          (*v26__f),
          F16(c, 0x68),
          F16(c, 0x6a),
          F16(c, 0x6c),
          (*v25__f),
          hdcSrc);
        v15 = (*v34__f) == 1;
        (*v32__f) += v11;
        --(*v34__f);
      }
      while ( !v15 );
      v14 = (*v28__f) / v11 - 1;
    }
    if ( (*v28__f) % v11 > 0 )
      rd_48d3d0(
        a2,
        (int)&F32(c, 0x5c),
        v12,
        v10 + v11 * v14,
        (*v28__f) % v11,
        (*v30__f),
        (*v26__f),
        F16(c, 0x68),
        F16(c, 0x6a),
        F16(c, 0x6c),
        (*v25__f),
        hdcSrc);
    return rd_48d3d0(
             a2,
             (int)&F32(c, 0x5c),
             v12,
             (*v28__f) + v10 - v11,
             v11,
             (*v30__f),
             (*v26__f),
             F16(c, 0x6e),
             F16(c, 0x70),
             F16(c, 0x72),
             (*v25__f),
             hdcSrc);
  }
  return result;
}

/* 0x48d3d0: int __stdcall sub_48D3D0( paint_ctx *a1, int a2, int a3, int a4, int a5, int a6, int a7, __int16 a8, __int16 a9, __int16 a10, cls_5BD550 *a11, HDC hdcSrc) */
int32_t rd_48d3d0(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int16_t a8, int16_t a9, int16_t a10, int32_t a11, int32_t hdcSrc)
{

    intptr_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    uint16_t v21 = 0;
    intptr_t v22 = 0;
    int v23 = 0;
    uint16_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    uint16_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    uint16_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;

  v12 = F32(a2, 0);
  v13 = F16(a2, 4) + a8;
  if ( ***(_DWORD ***)a2 == 0 )
    W_fl_4184a0((intptr_t)(v12), (intptr_t)(0));
  v14 = F32((**(_DWORD **)((uint8_t *)(v12) + 0x0) + 4 * v13), 0);
  if ( a11 != 0 )
  {
    v16 = a4;
    v17 = a7;
    v18 = a3;
    W_rp_5bd550((intptr_t)(a11), (intptr_t)(a1), (intptr_t)(v14), (intptr_t)(a3 + F32(v14, 12)), (intptr_t)(a4 + F32(v14, 16)), (intptr_t)(a7), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v14, 40)), (intptr_t)(hdcSrc));
  }
  else
  {
    v17 = a7;
    v16 = a4;
    W_rp_5b9bf0((intptr_t)(v14), (intptr_t)((int)a1), (intptr_t)(a3), (intptr_t)(a4), (intptr_t)(a7), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0));
    v18 = a3;
  }
  v19 = a6 / v17 - 1;
  if ( v19 > 1 )
  {
    v36 = v17 + v18;
    v33 = a6 / v17 - 2;
    do
    {
      v20 = F32(a2, 0);
      v21 = F16(a2, 4) + a9;
      v35 = F32(a2, 0);
      if ( ***(_DWORD ***)a2 == 0 )
      {
        W_fl_4184a0((intptr_t)(v20), (intptr_t)(0));
        v20 = v35;
      }
      v22 = F32((**(_DWORD **)((uint8_t *)(v20) + 0x0) + 4 * v21), 0);
      if ( a11 != 0 )
        W_rp_5bd550((intptr_t)(a11), (intptr_t)(a1), (intptr_t)(v22), (intptr_t)(v36 + F32(v22, 12)), (intptr_t)(v16 + F32(v22, 16)), (intptr_t)(v17), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v22, 40)), (intptr_t)(hdcSrc));
      else
        W_rp_5b9bf0((intptr_t)(v22), (intptr_t)((int)a1), (intptr_t)(v36), (intptr_t)(v16), (intptr_t)(v17), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0));
      v23 = v33 == 1;
      v36 += v17;
      --v33;
    }
    while ( !v23 );
    v18 = a3;
    v19 = a6 / v17 - 1;
  }
  v32 = a6 % v17;
  if ( a6 % v17 > 0 )
  {
    v37 = v18 + v17 * v19;
    v24 = F16(a2, 4) + a9;
    v25 = F32(a2, 0);
    v39 = F32(a2, 0);
    v34 = v24;
    if ( ***(_DWORD ***)a2 == 0 )
    {
      W_fl_4184a0((intptr_t)(v25), (intptr_t)(0));
      v25 = v39;
      v24 = v34;
    }
    v26 = F32((**(_DWORD **)((uint8_t *)(v25) + 0x0) + 4 * v24), 0);
    if ( a11 != 0 )
      W_rp_5bd550((intptr_t)(a11), (intptr_t)(a1), (intptr_t)(v26), (intptr_t)(v37 + F32(v26, 12)), (intptr_t)(v16 + F32(v26, 16)), (intptr_t)(v32), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v26, 40)), (intptr_t)(hdcSrc));
    else
      W_rp_5b9bf0((intptr_t)(v26), (intptr_t)((int)a1), (intptr_t)(v37), (intptr_t)(v16), (intptr_t)(v32), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0));
  }
  v27 = a6 + v18 - v17;
  v28 = F16(a2, 4) + a10;
  v29 = F32(a2, 0);
  v38 = v27;
  v40 = F32(a2, 0);
  if ( ***(_DWORD ***)a2 == 0 )
  {
    W_fl_4184a0((intptr_t)(v29), (intptr_t)(0));
    v27 = v38;
    v29 = v40;
  }
  v30 = F32((**(_DWORD **)((uint8_t *)(v29) + 0x0) + 4 * v28), 0);
  if ( a11 != 0 )
    return W_rp_5bd550((intptr_t)(a11), (intptr_t)(a1), (intptr_t)(v30), (intptr_t)(v27 + F32(v30, 12)), (intptr_t)(v16 + F32(v30, 16)), (intptr_t)(v17), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v30, 40)), (intptr_t)(hdcSrc));
  else
    return W_rp_5b9bf0((intptr_t)(v30), (intptr_t)((int)a1), (intptr_t)(v27), (intptr_t)(v16), (intptr_t)(v17), (intptr_t)(a5), (intptr_t)(0), (intptr_t)(0));
}

/* 0x48cf80: int __thiscall widget_box_frame_48CF80(cls_48CF80 *this, int a2, int a3, int a4, int a5, int a6, int a7, HDC hdcSrc) */
int32_t rd_48cf80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t hdcSrc)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t result = 0;
    intptr_t v15 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    _Alignas(8) uint8_t frame_[0x48 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *v36 = (int16_t *)(frame_ + 0x34);   /* [2] at ebp-0x14 */
    int16_t *v35 = (int16_t *)(frame_ + 0x34);   /* [2] at ebp-0x14 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x48 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x40 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x24 */
    uint16_t *const v26__f = (uint16_t *)(frame_ + 0x28);   /* ebp-0x20 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x20 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x20 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x1c */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x1c */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x1c */
    uint16_t *const v33__f = (uint16_t *)(frame_ + 0x30);   /* ebp-0x18 */
    uint16_t *const v32__f = (uint16_t *)(frame_ + 0x30);   /* ebp-0x18 */
    uint16_t *const v34__f = (uint16_t *)(frame_ + 0x34);   /* ebp-0x14 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x10 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0xc */
    uint16_t *const v40__f = (uint16_t *)(frame_ + 0x40);   /* ebp-0x8 */
    uint16_t *const v39__f = (uint16_t *)(frame_ + 0x40);   /* ebp-0x8 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x4 */

  v8 = a5;
  v10 = F32(c, 0x74);
  v11 = F32(c, 0x78);
  (*v25__f) = v11;
  v12 = 2 * v10;
  if ( a5 >= 2 * v10 )
  {
    v13 = a6;
  }
  else
  {
    v13 = a6;
    result = 2 * v11;
    if ( a6 < 2 * v11 )
      return result;
  }
  result = F32(c, 0x5c);
  if ( result != 0 )
  {
    if ( a5 < v12 )
    {
      a5 = 2 * v10;
      a3 += (v12 - v8) / -2;
    }
    if ( v13 < 2 * v11 )
    {
      a6 = 2 * v11;
      a4 += (2 * v11 - v13) / -2;
    }
    (*v34__f) = F16(c, 0x66);
    (*v26__f) = F16(c, 0x64);
    v15 = rd_418470(F32(c, 0x5c), F16(c, 0x62) + F16(c, 0x60));
    if ( a7 != 0 )
    {
      (*v23__f) = v11;
      v17 = a2;
      W_rp_5bd550((intptr_t)(a7), (intptr_t)((intptr_t)a2), (intptr_t)(v15), (intptr_t)(a3 + F32(v15, 12)), (intptr_t)(a4 + F32(v15, 16)), (intptr_t)(v10), (intptr_t)((*v23__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v15, 40)), (intptr_t)(hdcSrc));
    }
    else
    {
      (*v24__f) = v11;
      v17 = a2;
      W_rp_5b9bf0((intptr_t)(v15), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(a4), (intptr_t)(v10), (intptr_t)((*v24__f)), (intptr_t)(0), (intptr_t)(0));
    }
    v18 = a5 / v10 - 1;
    v45 = v18;
    if ( v18 > 1 )
    {
      (*v29__f) = a5 / v10 - 2;
      v42 = (intptr_t)(v10 + a3);
      do
      {
        rd_48d670(v17, (int)&F32(c, 0x5c), (*v26__f), (int)v42, a4, v10, (*v25__f), a7, hdcSrc);
        v19 = (*v29__f) == 1;
        v42 = (intptr_t)((uint8_t *)v42 + v10);
        --(*v29__f);
      }
      while ( !v19 );
      v18 = a5 / v10 - 1;
    }
    (*v37__f) = a5 % v10;
    if ( a5 % v10 > 0 )
      rd_48d670(v17, (int)&F32(c, 0x5c), (*v26__f), a3 + v10 * v18, a4, a5 % v10, (*v25__f), a7, hdcSrc);
    (*v38__f) = a3 - v10 + a5;
    rd_48d670(v17, (int)&F32(c, 0x5c), (*v34__f), (*v38__f), a4, v10, (*v25__f), a7, hdcSrc);
    v20 = (*v25__f);
    (*v41__f) = a6 / (*v25__f) - 1;
    if ( (*v41__f) > 1 )
    {
      v43 = (intptr_t)((*v25__f) + a4);
      F32(v35, 0) = a6 / (*v25__f) - 2;
      do
      {
        (*v39__f) = F16(c, 0x6c);
        (*v32__f) = F16(c, 0x6a);
        rd_48d670(v17, (int)&F32(c, 0x5c), F16(c, 0x68), a3, (int)v43, v10, v20, a7, hdcSrc);
        v21 = a5 / v10 - 1;
        if ( v45 > 1 )
        {
          (*v30__f) = a5 / v10 - 2;
          (*v27__f) = v10 + a3;
          do
          {
            rd_48d670(v17, (int)&F32(c, 0x5c), (*v32__f), (*v27__f), (int)v43, v10, (*v25__f), a7, hdcSrc);
            v19 = (*v30__f) == 1;
            (*v27__f) += v10;
            --(*v30__f);
          }
          while ( !v19 );
          v21 = a5 / v10 - 1;
        }
        if ( (*v37__f) > 0 )
          rd_48d670(v17, (int)&F32(c, 0x5c), (*v32__f), a3 + v10 * v21, (int)v43, (*v37__f), (*v25__f), a7, hdcSrc);
        rd_48d670(v17, (int)&F32(c, 0x5c), (*v39__f), (*v38__f), (int)v43, v10, (*v25__f), a7, hdcSrc);
        v19 = F32(v35, 0) == 1;
        v43 = (intptr_t)((uint8_t *)v43 + (*v25__f));
        --F32(v35, 0);
        v20 = (*v25__f);
      }
      while ( !v19 );
    }
    (*v28__f) = a6 % v20;
    if ( a6 % v20 > 0 )
    {
      (*v40__f) = F16(c, 0x6c);
      (*v33__f) = F16(c, 0x6a);
      (*v31__f) = a4 + v20 * (*v41__f);
      rd_48d670(v17, (int)&F32(c, 0x5c), F16(c, 0x68), a3, (*v31__f), v10, a6 % v20, a7, hdcSrc);
      v22 = a5 / v10 - 1;
      if ( v45 > 1 )
      {
        F32(v36, 0) = a5 / v10 - 2;
        v44 = (intptr_t)(v10 + a3);
        do
        {
          rd_48d670(v17, (int)&F32(c, 0x5c), (*v33__f), (int)v44, (*v31__f), v10, (*v28__f), a7, hdcSrc);
          v19 = F32(v36, 0) == 1;
          v44 = (intptr_t)((uint8_t *)v44 + v10);
          --F32(v36, 0);
        }
        while ( !v19 );
        v22 = a5 / v10 - 1;
      }
      if ( (*v37__f) > 0 )
        rd_48d670(v17, (int)&F32(c, 0x5c), (*v33__f), a3 + v10 * v22, (*v31__f), (*v37__f), (*v28__f), a7, hdcSrc);
      rd_48d670(v17, (int)&F32(c, 0x5c), (*v40__f), (*v38__f), (*v31__f), v10, (*v28__f), a7, hdcSrc);
      v20 = (*v25__f);
    }
    return rd_48d3d0(
             v17,
             (int)&F32(c, 0x5c),
             a3,
             a6 + a4 - v20,
             v20,
             a5,
             v10,
             F16(c, 0x6e),
             F16(c, 0x70),
             F16(c, 0x72),
             a7,
             hdcSrc);
  }
  return result;
}

/* 0x48d670: int __stdcall sub_48D670(paint_ctx *a1, int a2, __int16 a3, int a4, int a5, int a6, int a7, cls_5BD550 *a8, HDC hdcSrc) */
int32_t rd_48d670(int32_t a1, int32_t a2, int16_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t hdcSrc)
{

    intptr_t v9 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;

  v9 = F32(a2, 0);
  v10 = a3 + F16(a2, 4);
  if ( ***(_DWORD ***)a2 == 0 )
    W_fl_4184a0((intptr_t)(v9), (intptr_t)(0));
  v11 = F32((**(_DWORD **)((uint8_t *)(v9) + 0x0) + 4 * v10), 0);
  if ( a8 != 0 )
    return W_rp_5bd550((intptr_t)(a8), (intptr_t)(a1), (intptr_t)(v11), (intptr_t)(a4 + F32(v11, 12)), (intptr_t)(a5 + F32(v11, 16)), (intptr_t)(a6), (intptr_t)(a7), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v11, 40)), (intptr_t)(hdcSrc));
  else
    return W_rp_5b9bf0((intptr_t)(v11), (intptr_t)((int)a1), (intptr_t)(a4), (intptr_t)(a5), (intptr_t)(a6), (intptr_t)(a7), (intptr_t)(0), (intptr_t)(0));
}

/* 0x48d710: int *__thiscall sub_48D710(cls_48D710 *this, int a2, int a3, int a4, HDC hdcSrc) */
int32_t rd_48d710(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t hdcSrc)
{
    intptr_t c = (intptr_t)self;
    int32_t * result = 0;
    uint16_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v22 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  result = (int *)F32(c, 0x84);
  if ( result == 0 )
    return result;
  if ( F32(c, 0x8) == 1 )
  {
    result = (int *)F32(c, 0x174);
    if ( *result < 2 || *result > 3 || result[3] >= result[4] )
      return result;
    goto LABEL_16;
  }
  result = (int *)(F32(c, 0x8) - 2);
  if ( F32(c, 0x8) != 2 )
    return result;
  if ( F32(c, 0x140) == 0 )
  {
LABEL_16:
    (*v20__f) = true;
LABEL_6:
    v8 = F16(c, 0x8c);
    v9 = a4 + F32(c, 0xe0);
    v10 = F32(c, 0x88);
    v11 = a3 + F32(c, 0xdc);
    (*v21__f) = F32(c, 0x88);
    if ( v10 != 0 )
    {
      if ( F16(c, 0xce) != 0 )
        v8 += *(_WORD *)&GAP(c, 0x8e, 2 * F16(c, 0xd2));
      if ( **(_DWORD **)((uint8_t *)(v10) + 0x0) == 0 )
      {
        W_fl_4184a0((intptr_t)(v10), (intptr_t)(0));
        v10 = (*v21__f);
      }
      v12 = F32((**(_DWORD **)((uint8_t *)(v10) + 0x0) + 4 * v8), 0);
      v13 = F32(c, 0xd8);
      if ( v13 != 0 )
        W_rp_5bd550((intptr_t)(v13), (intptr_t)((intptr_t)a2), (intptr_t)(v12), (intptr_t)(v11 + F32(v12, 12)), (intptr_t)(v9 + F32(v12, 16)), (intptr_t)(F32(v12, 20)), (intptr_t)(F32(v12, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v12, 40)), (intptr_t)(hdcSrc));
      else
        W_rp_5b9b70((intptr_t)(v12), (intptr_t)(a2), (intptr_t)(v11), (intptr_t)(v9));
    }
    goto LABEL_18;
  }
  v7 = F16(c, 0x160);
  (*v20__f) = v7 < F16(c, 0x162);
  if ( v7 > 1u )
    goto LABEL_6;
LABEL_18:
  result = (int *)(*v20__f);
  if ( (*v20__f) )
  {
    v14 = F16(c, 0xe8);
    v15 = a4 + F32(c, 0x13c);
    v16 = F32(c, 0xe4);
    v17 = a3 + F32(c, 0x138);
    v22 = (int)v16;
    if ( v16 != 0 )
    {
      if ( F16(c, 0x12a) != 0 )
        v14 += *(_WORD *)&GAP(c, 0xea, 2 * F16(c, 0x12e));
      if ( **(_DWORD **)((uint8_t *)(v16) + 0x0) == 0 )
      {
        W_fl_4184a0((intptr_t)(v16), (intptr_t)(0));
        v16 = v22;
      }
      v18 = F32(c, 0x134);
      v19 = F32((**(_DWORD **)((uint8_t *)(v16) + 0x0) + 4 * v14), 0);
      if ( v18 != 0 )
        return (int *)W_rp_5bd550((intptr_t)(v18), (intptr_t)((intptr_t)a2), (intptr_t)(v19), (intptr_t)(v17 + F32(v19, 12)), (intptr_t)(v15 + F32(v19, 16)), (intptr_t)(F32(v19, 20)), (intptr_t)(F32(v19, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v19, 40)), (intptr_t)(hdcSrc));
      else
        return (int *)W_rp_5b9b70((intptr_t)(v19), (intptr_t)(a2), (intptr_t)(v17), (intptr_t)(v15));
    }
  }
  return result;
}

/* 0x48d940: _DWORD *__thiscall sub_48D940(cls_48D940 *this, int a2, int a3, _DWORD *a4, HDC hdcSrc) */
int32_t rd_48d940(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t hdcSrc)
{
    intptr_t c = (intptr_t)self;
    int32_t * result = 0;
    intptr_t v7 = 0;
    char * v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;

  result = (_DWORD *)F32(c, 0x20);
  if ( result != 0 )
  {
    result = (_DWORD *)F32(c, 0x24);
    if ( result == 0 )
    {
      if ( F32(c, 0x8) == 1 )
      {
        result = (_DWORD *)F32(c, 0x174);
        if ( *result == 1 )
          return result;
        v9 = F32(c, 0x1ac) * (result[5] - result[6]);
        result = a4;
        v7 = a3 + F32(c, 0xc) + F32(c, 0x7c);
        v8 = (uint8_t *)a4 + F32(c, 0x10) + F32(c, 0x80) + v9;
      }
      else
      {
        result = (_DWORD *)(F32(c, 0x8) - 2);
        if ( F32(c, 0x8) == 2 )
        {
          if ( F32(c, 0x1c) != 7 )
            return result;
          result = a4;
          v7 = a3 + F32(c, 0xc) + F32(c, 0x7c);
          v8 = (uint8_t *)a4 + F32(c, 0x10) + F32(c, 0x80);
        }
        else
        {
          v7 = a3;
          v8 = (char *)a3;
        }
      }
      v10 = F32(c, 0x28);
      v11 = F16(c, 0x2c);
      v14 = F32(c, 0x28);
      if ( v10 != 0 )
      {
        if ( F16(c, 0x6e) != 0 )
          v11 += *(_WORD *)&GAP(c, 0x2e, 2 * F16(c, 0x72));
        if ( **(_DWORD **)((uint8_t *)(v10) + 0x0) == 0 )
        {
          W_fl_4184a0((intptr_t)(v10), (intptr_t)(0));
          v10 = v14;
        }
        v12 = F32(c, 0x78);
        v13 = F32((**(_DWORD **)((uint8_t *)(v10) + 0x0) + 4 * v11), 0);
        if ( v12 != 0 )
          return (_DWORD *)W_rp_5bd550((intptr_t)(v12), (intptr_t)((intptr_t)a2), (intptr_t)(v13), (intptr_t)(v7 + F32(v13, 12)), (intptr_t)((int)&v8[F32(v13, 16)]), (intptr_t)(F32(v13, 20)), (intptr_t)(F32(v13, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v13, 40)), (intptr_t)(hdcSrc));
        else
          return (_DWORD *)W_rp_5b9b70((intptr_t)(v13), (intptr_t)(a2), (intptr_t)(v7), (intptr_t)((int)v8));
      }
    }
  }
  return result;
}

/* 0x48da70: __int16 __thiscall sub_48DA70(cls_48DA70 *this, int a2, HDC hdc, int a4, int a5, HGDIOBJ h, HGDIOBJ a7, int a8) */
int32_t rd_48da70(wl_char *self, int32_t a2, int32_t hdc, int32_t a4, int32_t a5, int32_t h, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t i = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    uint16_t v15 = 0;
    uint16_t * v16 = 0;
    intptr_t v17 = 0;
    uint16_t v18 = 0;
    uint16_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    uint16_t v23 = 0;
    uint16_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    uint32_t v27 = 0;
    int v28 = 0;
    uint16_t v29 = 0;
    uint16_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    uint32_t v33 = 0;
    int32_t * v34 = 0;
    uint16_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    uint32_t v38 = 0;
    int32_t * v39 = 0;
    uint16_t v40 = 0;
    intptr_t v41 = 0;
    uint16_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t j = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    uint32_t v48 = 0;
    int v49 = 0;
    uint32_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    uint16_t v55 = 0;
    _Alignas(8) uint8_t frame_[0x44 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    int16_t *const v57__f = (int16_t *)(frame_ + 0x0);   /* ebp-0x44 */
    intptr_t *const v58__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x3c */
    int32_t * *const v59__f = (int32_t * *)(frame_ + 0xc);   /* ebp-0x38 */
    uint16_t *const v61__f = (uint16_t *)(frame_ + 0x14);   /* ebp-0x30 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2c */
    uint16_t *const v66__f = (uint16_t *)(frame_ + 0x1c);   /* ebp-0x28 */
    uint16_t *const v65__f = (uint16_t *)(frame_ + 0x1c);   /* ebp-0x28 */
    uint16_t *const v64__f = (uint16_t *)(frame_ + 0x1c);   /* ebp-0x28 */
    uint16_t *const v63__f = (uint16_t *)(frame_ + 0x1c);   /* ebp-0x28 */
    uint16_t *const v70__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0x24 */
    uint16_t *const v69__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0x24 */
    uint16_t *const v68__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0x24 */
    uint16_t *const v67__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0x24 */
    uint16_t *const v71__f = (uint16_t *)(frame_ + 0x24);   /* ebp-0x20 */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x1c */
    int32_t * *const v74__f = (int32_t * *)(frame_ + 0x28);   /* ebp-0x1c */
    int32_t * *const v72__f = (int32_t * *)(frame_ + 0x28);   /* ebp-0x1c */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v79__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x14 */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x10 */
    int32_t * *const v80__f = (int32_t * *)(frame_ + 0x34);   /* ebp-0x10 */
    intptr_t *const v82__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0xc */
    intptr_t *const v83__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x8 */
    uint16_t *const v85__f = (uint16_t *)(frame_ + 0x40);   /* ebp-0x4 */
    intptr_t *const v84__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x4 */

  v8 = c;
  v9 = F32(c, 0x170);
  (*v61__f) = 0;
  v10 = *(_DWORD **)v9;
  (*v59__f) = *(_DWORD **)v9;
  v11 = F32(F32(v9, 0), 12);
  SETLO(i, F16(v11, 8));
  if ( (uint16_t)i != 0 && (a2 == 0 || F32(c, 0x164) != 0) )
  {
    v11 = F32(v9, 12) - 1;
    if ( F32(v9, 12) == 1 )
    {
      SETLO(v11, F16(v9, 4));
      (*v71__f) = v11;
      if ( (uint16_t)v11 == 0 )
        return v11;
      SETLO(v11, F16(*v10 + 12 * (uint16_t)(v11 - 1), 2));
      v13 = F16(v9, 10);
      if ( v13 <= (uint16_t)v11 )
      {
        SETLO(v11, v11 - v13 + 1);
        (*v61__f) = v11;
      }
    }
    else
    {
      (*v71__f) = i;
      (*v61__f) = 0;
    }
    v14 = F32(v9, 28);
    v15 = 0;
    if ( v14 != 0 )
    {
      v16 = (_WORD *)(*v10 + 2);
      for ( i = (uint16_t)i; i != 0; --i )
      {
        if ( v15 < (uint16_t)(*v16 - (*v61__f) + 1) )
          v15 = *v16 - (*v61__f) + 1;
        v16 += 6;
      }
      SETLO(v11, F16(v9, 10));
      if ( (uint16_t)v11 < v15 )
        v15 = F16(v9, 10);
      if ( v15 != 0 )
      {
        (*v62__f) = F32(v9, 28);
        v11 = *(_DWORD *)&GAP((v8 + (1) * 0x174), 0x0, 56) * (F16(v9, 10) - v15) / (v15 + 1);
        if ( v14 >= v11 )
          (*v62__f) = *(_DWORD *)&GAP((v8 + (1) * 0x174), 0x0, 56) * (F16(v9, 10) - v15) / (v15 + 1);
      }
      else
      {
        (*v62__f) = 0;
      }
    }
    else
    {
      (*v62__f) = 0;
    }
    v17 = 0;
    (*v57__f) = 0;
    if ( (*v71__f) != 0 )
    {
      (*v82__f) = 0;
      (*v58__f) = 1;
      do
      {
        v18 = F16(v17 + *(*v59__f), 2);
        v19 = (uint16_t *)(*(*v59__f) + v17);
        if ( v18 < (*v61__f) )
          goto LABEL_56;
        v20 = v18 - (*v61__f);
        if ( v20 >= F16(F32(v8, 0x170), 10) )
          goto LABEL_56;
        v21 = a4 + *(_DWORD *)&GAP(v8, 0x0, 12) + 7 * *v19;
        (*v79__f) = v21;
        v22 = a5 + *(_DWORD *)&GAP(v8, 0x0, 16) + v20 * *(_DWORD *)&GAP((v8 + (1) * 0x174), 0x0, 56) + (*v62__f) * (v20 + 1);
        if ( a2 != 0 )
        {
          (*v84__f) = F32(v19, 4 * 1);
          if ( (*v84__f) != 0 && F32(v8, 0x164) != 0 )
          {
            v50 = *(_DWORD *)&GAP(v8, 0x168, 4);
            v51 = 0;
            if ( v50 != 0 )
            {
              v52 = 0;
              while ( 1 )
              {
                v53 = F32(v8, 0x164) + 100 * v52;
                if ( F32(v53, 80) == (*v84__f) && F32(v53, 84) != 0 )
                  break;
                v52 = (uint16_t)++v51;
                if ( (uint16_t)v51 >= v50 )
                  goto LABEL_56;
              }
              v54 = F32(v53, 84);
              v55 = F16(v53, 88);
              (*v85__f) = v55;
              (*v83__f) = v54;
              if ( **(_DWORD **)((uint8_t *)(v54) + 0x0) == 0 )
              {
                W_fl_4184a0((intptr_t)(v54), (intptr_t)(0));
                v55 = (*v85__f);
                v54 = (*v83__f);
              }
              W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)((uint8_t *)(v54) + 0x0), 4 * v55)), (intptr_t)(a2), (intptr_t)(v21 + F32(v53, 92)), (intptr_t)(v22 + F32(v53, 96)));
            }
          }
          goto LABEL_56;
        }
        API_SelectObject((intptr_t)(hdc), (intptr_t)(h));
        API_SetTextColor((intptr_t)(hdc), (intptr_t)(*(_DWORD *)&GAP((v8 + (1) * 0x174), 0x0, 16)));
        (*v63__f) = 0;
        (*v72__f) = (int *)(*v59__f)[3];
        v23 = F16((*v72__f), 2 * 4);
        if ( (*v58__f) > v23 )
        {
          v24 = v23 - (*v57__f);
          if ( v24 == 0 )
            goto LABEL_32;
        }
        else
        {
          v24 = 1;
        }
        (*v67__f) = (*v57__f);
        v25 = *(*v72__f);
        (*v75__f) = v24;
        do
        {
          v26 = 36 * (*v67__f);
          API_TextOutA((intptr_t)(hdc), (intptr_t)(v21 + (*v63__f)), (intptr_t)(v22 + 1), (intptr_t)((char *)(v26 + v25)), (intptr_t)(strlen((const char *)(v26 + v25))));
          v25 = *(*v72__f);
          v27 = strlen((const char *)(*(*v72__f) + v26)) + 1;
          v28 = (*v75__f) == 1;
          (*v63__f) += 7 * (v27 - 1);
          ++(*v67__f);
          --(*v75__f);
        }
        while ( !v28 );
        v8 = c;
LABEL_32:
        (*v68__f) = 0;
        (*v76__f) = (*v59__f)[3];
        v29 = F16((*v76__f), 8);
        if ( (*v58__f) > v29 )
        {
          v30 = v29 - (*v57__f);
          if ( v30 == 0 )
            goto LABEL_38;
        }
        else
        {
          v30 = 1;
        }
        (*v73__f) = v30;
        v31 = F32((*v76__f), 0);
        (*v64__f) = (*v57__f);
        do
        {
          v32 = 36 * (*v64__f);
          API_TextOutA((intptr_t)(hdc), (intptr_t)((*v68__f) + v21 + 1), (intptr_t)(v22), (intptr_t)((char *)(v32 + v31)), (intptr_t)(strlen((const char *)(v32 + v31))));
          v31 = F32((*v76__f), 0);
          v33 = strlen((const char *)(F32((*v76__f), 0) + v32)) + 1;
          v28 = (*v73__f) == 1;
          (*v68__f) += 7 * (v33 - 1);
          ++(*v64__f);
          --(*v73__f);
        }
        while ( !v28 );
        v8 = c;
LABEL_38:
        API_SetTextColor((intptr_t)(hdc), (intptr_t)(*(_DWORD *)&GAP((v8 + (1) * 0x174), 0x0, 12)));
        (*v69__f) = 0;
        v34 = (int *)(*v59__f)[3];
        (*v80__f) = v34;
        if ( (*v58__f) > *((uint16_t *)v34 + 4) )
        {
          v35 = F16(v34, 2 * 4) - (*v57__f);
          if ( v35 == 0 )
            goto LABEL_44;
        }
        else
        {
          v35 = 1;
        }
        v36 = *v34;
        (*v65__f) = (*v57__f);
        (*v77__f) = v35;
        do
        {
          v37 = 36 * (*v65__f);
          API_TextOutA((intptr_t)(hdc), (intptr_t)(v21 + (*v69__f)), (intptr_t)(v22), (intptr_t)((char *)(v37 + v36)), (intptr_t)(strlen((const char *)(v37 + v36))));
          v36 = *(*v80__f);
          v38 = strlen((const char *)(*(*v80__f) + v37)) + 1;
          v28 = (*v77__f) == 1;
          (*v69__f) += 7 * (v38 - 1);
          ++(*v65__f);
          --(*v77__f);
        }
        while ( !v28 );
        v8 = c;
LABEL_44:
        if ( *(_DWORD *)&GAP(v8, 0x0, 20) != 0 )
        {
          API_SelectObject((intptr_t)(hdc), (intptr_t)(a7));
          API_SetTextColor((intptr_t)(hdc), (intptr_t)(*(_DWORD *)&GAP((v8 + (1) * 0x174), 0x0, 20)));
          (*v78__f) = v22 + *(_DWORD *)&GAP(v8, 0x0, 24);
          v39 = (_DWORD *)(*v59__f)[3];
          (*v74__f) = v39;
          (*v70__f) = 0;
          if ( (*v58__f) > *((uint16_t *)v39 + 4) )
          {
            v40 = F16(v39, 2 * 4) - (*v57__f);
            if ( v40 != 0 )
              goto LABEL_48;
          }
          else
          {
            v40 = 1;
LABEL_48:
            (*v66__f) = (*v57__f);
            (*v81__f) = v40;
            do
            {
              v41 = *v39;
              v42 = 0;
              v43 = 36 * (*v66__f);
              v44 = *v39 + v43;
              if ( F16(v44, 28) != 0 )
              {
                for ( j = 6 * (*v66__f); ; j = 6 * (*v66__f) )
                {
                  v46 = v21 + (*v70__f) + 7 * (v42 + F32(v44, 32));
                  v47 = j + v42;
                  API_TextOutA((intptr_t)(hdc), (intptr_t)(v46), (intptr_t)((*v78__f)), (intptr_t)((char *)(v41 + 6 * v47 + 3)), (intptr_t)(strlen((const char *)(v41 + 6 * v47 + 3))));
                  API_TextOutA((intptr_t)(hdc), (intptr_t)(v46 + 3), (intptr_t)((*v78__f) - 3), (intptr_t)((char *)(*(*v74__f) + 2 * (3 * v47 + 3))), (intptr_t)(strlen((const char *)(*(*v74__f) + 2 * (3 * v47 + 3)))));
                  v41 = *(*v74__f);
                  v43 = 36 * (*v66__f);
                  v21 = (*v79__f);
                  ++v42;
                  v44 = *(*v74__f) + v43;
                  if ( v42 >= F16(v44, 28) )
                    break;
                }
                v39 = (*v74__f);
              }
              v48 = strlen((const char *)(*v39 + v43)) + 1;
              v28 = (*v81__f) == 1;
              (*v70__f) += 7 * (v48 - 1);
              ++(*v66__f);
              --(*v81__f);
            }
            while ( !v28 );
          }
          v8 = c;
        }
LABEL_56:
        SETLO(v11, (*v57__f) + 1);
        v17 = (*v82__f) + 12;
        v49 = (uint16_t)++(*v57__f) < (*v71__f);
        --(*v58__f);
        (*v82__f) += 12;
      }
      while ( v49 );
    }
  }
  return v11;
}

/* 0x48e100: int __thiscall sub_48E100(cls_48E100 *this, HDC hdc, int a3, int a4, HGDIOBJ h) */
int32_t rd_48e100(wl_char *self, int32_t hdc, int32_t a3, int32_t a4, int32_t h)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *String = (char *)(frame_ + 0x0);   /* [20] at ebp-0x14 */

  result = F32(c, 0x140);
  if ( result != 0 )
  {
    g_eng.h5bf3ee(String, ((uint8_t *)(c) + 0x144), F16(c, 0x160), F16(c, 0x162));
    v7 = a3 + F32(c, 0x158);
    v8 = a4 + F32(c, 0x15c);
    API_SelectObject((intptr_t)(hdc), (intptr_t)(h));
    API_SetTextColor((intptr_t)(hdc), (intptr_t)(F32(c, 0x19c)));
    API_TextOutA((intptr_t)(hdc), (intptr_t)(v7), (intptr_t)(v8 + 1), (intptr_t)(String), (intptr_t)(strlen(String)));
    API_TextOutA((intptr_t)(hdc), (intptr_t)(v7 + 1), (intptr_t)(v8), (intptr_t)(String), (intptr_t)(strlen(String)));
    API_SetTextColor((intptr_t)(hdc), (intptr_t)(F32(c, 0x1a0)));
    return API_TextOutA((intptr_t)(hdc), (intptr_t)(v7), (intptr_t)(v8), (intptr_t)(String), (intptr_t)(strlen(String)));
  }
  return result;
}

/* 0x48e200: int __thiscall sub_48E200(cls_48E200 *this, int a2, int a3, HDC hdc, int a5, int a6, HGDIOBJ h, HGDIOBJ a8, HDC hdcSrc) */
int32_t rd_48e200(wl_char *self, int32_t a2, int32_t a3, int32_t hdc, int32_t a5, int32_t a6, int32_t h, int32_t a8, int32_t hdcSrc)
{
    intptr_t c = (intptr_t)self;
    intptr_t v9 = 0;
    intptr_t result = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    int32_t * v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int32_t ** v19 = 0;
    int32_t * v20 = 0;
    uint32_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    char * v28 = 0;
    uint32_t v29 = 0;
    int v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    char * v33 = 0;
    uint32_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    uint16_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    uint16_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    int v50 = 0;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *const color__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x38 */
    uint32_t *const v52__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x30 */
    intptr_t *const ya__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c */
    intptr_t *const y__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c */
    uint16_t *const v58__f = (uint16_t *)(frame_ + 0x10);   /* ebp-0x28 */
    uint16_t *const v57__f = (uint16_t *)(frame_ + 0x10);   /* ebp-0x28 */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x28 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x28 */
    intptr_t *const v56__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x28 */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x24 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x20 */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1c */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x18 */
    intptr_t *const v67__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x14 */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x14 */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x14 */
    intptr_t *const v68__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x10 */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xc */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */

  v9 = c;
  result = 0;
  v11 = (_DWORD *)F32(c, 0x174);
  (*v68__f) = v9;
  v12 = v11[6];
  (*v70__f) = 0;
  (*v63__f) = v12;
  if ( v12 < v11[4] )
  {
    v13 = 16 * v12;
    (*v64__f) = 16 * v12;
    do
    {
      v14 = result * *(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 52);
      v15 = v13 + *(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 4);
      (*v71__f) = v14;
      (*v61__f) = v15;
      (*v69__f) = 0;
      if ( (int)v11[2] > 0 )
      {
        (*v53__f) = 0;
        (*v62__f) = 0;
        while ( 1 )
        {
          v16 = (_DWORD *)((*v62__f) + *(_DWORD *)((uint8_t *)(v9 + (1) * 0x178) + 0x0));
          v17 = *(_DWORD *)&GAP(v9, 0x0, 12) + a5 + *v16;
          (*v65__f) = v17;
          v18 = a6 + v14 + *(_DWORD *)&GAP(v9, 0x0, 16) + v16[1];
          (*y__f) = v18;
          if ( a2 != 0 )
          {
            v19 = (int **)(F32(v15, 12) + (*v53__f));
            v20 = *v19;
            if ( *v19 != 0 )
            {
              (*v56__f) = 1;
              if ( F32((*v61__f), 8) != 0 || *v11 == 1 )
              {
                if ( v11[5] != (*v63__f) || *v11 == 1 )
                {
                  if ( v19[3] != 0 )
                  {
                    (*color__f) = (uint32_t)v19[4];
                    v21 = (uint32_t)v19[5];
                  }
                  else if ( v16[4] != 0 )
                  {
                    (*color__f) = v16[5];
                    v21 = v16[6];
                  }
                  else
                  {
                    (*color__f) = *(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 8);
                    v21 = *(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 16);
                  }
                }
                else if ( v16[4] != 0 )
                {
                  (*color__f) = v16[7];
                  v21 = v16[8];
                }
                else
                {
                  (*color__f) = *(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 20);
                  v21 = *(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 24);
                }
                (*v52__f) = v21;
              }
              else
              {
                (*v56__f) = 0;
                (*color__f) = *(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 28);
                (*v52__f) = *(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 32);
              }
              if ( v16[3] == 1 )
              {
                v23 = v16[2] - 7 * *((uint16_t *)v20 + 5);
                v22 = v23 < 0 ? 0 : v23;
              }
              else
              {
                v22 = 0;
              }
              v24 = hdc;
              v25 = v17 + v22;
              API_SelectObject((intptr_t)(hdc), (intptr_t)(h));
              if ( (*v56__f) != 0 )
              {
                API_SetTextColor((intptr_t)(hdc), (intptr_t)(*(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 12)));
                (*v57__f) = 0;
                if ( F16(v20, 2 * 4) != 0 )
                {
                  v26 = *v20;
                  v27 = 0;
                  (*v66__f) = *((uint16_t *)v20 + 4);
                  do
                  {
                    API_TextOutA((intptr_t)(hdc), (intptr_t)(v25 + (*v57__f)), (intptr_t)((*y__f) + 1), (intptr_t)((char *)(v27 + v26)), (intptr_t)(strlen((const char *)(v27 + v26))));
                    v26 = *v20;
                    v28 = (const char *)(v27 + *v20);
                    v27 += 36;
                    v29 = strlen(v28) + 1;
                    v30 = (*v66__f) == 1;
                    (*v57__f) += 7 * (v29 - 1);
                    --(*v66__f);
                  }
                  while ( !v30 );
                  v24 = hdc;
                  v9 = (*v68__f);
                }
                (*v58__f) = 0;
                if ( F16(v20, 2 * 4) != 0 )
                {
                  v31 = *v20;
                  v32 = 0;
                  (*v67__f) = *((uint16_t *)v20 + 4);
                  do
                  {
                    API_TextOutA((intptr_t)(hdc), (intptr_t)((*v58__f) + v25 + 1), (intptr_t)((*y__f)), (intptr_t)((char *)(v32 + v31)), (intptr_t)(strlen((const char *)(v32 + v31))));
                    v31 = *v20;
                    v33 = (const char *)(v32 + *v20);
                    v32 += 36;
                    v34 = strlen(v33) + 1;
                    v30 = (*v67__f) == 1;
                    (*v58__f) += 7 * (v34 - 1);
                    --(*v67__f);
                  }
                  while ( !v30 );
                  v24 = hdc;
                  v9 = (*v68__f);
                }
              }
              API_SetTextColor((intptr_t)(v24), (intptr_t)((*color__f)));
              rd_48e860(v20, v24, v25, (*y__f), 7, 0, 0);
              if ( *(_DWORD *)&GAP(v9, 0x0, 20) != 0 )
              {
                API_SetTextColor((intptr_t)(v24), (intptr_t)((*v52__f)));
                API_SelectObject((intptr_t)(v24), (intptr_t)(a8));
                rd_48e6d0(v20, v24, v25, (*y__f) + *(_DWORD *)&GAP(v9, 0x0, 24), 7, 0, 0);
              }
            }
          }
          else
          {
            if ( F32(F32((v15 + 12), 0) + (*v53__f), 4) != 0 )
            {
              v35 = F32(F32((*(_DWORD *)&GAP((v9 + (1) * 0x178), 0x0, 4) + (*v64__f) + 12), 0) + (*v53__f), 4);
              v36 = F32(v35, 0);
              v37 = F16(v35, 4);
              (*v59__f) = F32(v35, 0);
              if ( F32(v35, 0) != 0 )
              {
                if ( F16(v35, 70) != 0 )
                  v37 += F16(v35 + 2 * F16(v35, 74), 6);
                if ( **(_DWORD **)((uint8_t *)(v36) + 0x0) == 0 )
                {
                  W_fl_4184a0((intptr_t)(v36), (intptr_t)(0));
                  v36 = (*v59__f);
                  v17 = (*v65__f);
                }
                v38 = F32(v35, 80);
                v39 = F32((**(_DWORD **)((uint8_t *)(v36) + 0x0) + 4 * v37), 0);
                if ( v38 != 0 )
                  W_rp_5bd550((intptr_t)(v38), (intptr_t)((intptr_t)a3), (intptr_t)(v39), (intptr_t)(v17 + F32(v39, 12)), (intptr_t)(v18 + F32(v39, 16)), (intptr_t)(F32(v39, 20)), (intptr_t)(F32(v39, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v39, 40)), (intptr_t)(hdcSrc));
                else
                  W_rp_5b9b70((intptr_t)(v39), (intptr_t)(a3), (intptr_t)(v17), (intptr_t)(v18));
              }
            }
            v40 = F32(F32(((*v61__f) + 12), 0) + (*v53__f), 8);
            if ( v40 != 0 )
            {
              v41 = F32(v40, 4);
              v42 = F16(v40, 8);
              (*v60__f) = v41;
              if ( **(_DWORD **)((uint8_t *)(v41) + 0x0) == 0 )
              {
                W_fl_4184a0((intptr_t)(v41), (intptr_t)(0));
                v41 = (*v60__f);
              }
              (*ya__f) = F32(**(_DWORD **)((uint8_t *)(v41) + 0x0), 4 * v42);
              if ( F32(v40, 0) == 0 )
              {
                v43 = F32(v40, 20);
                v44 = F32(v40, 12);
                v45 = v44 * (v43 + F32(v40, 28));
                v46 = v43 + F32(v40, 24);
                v47 = F32(v40, 16);
                v48 = v45 / v46;
                v49 = v44 - v48;
                if ( v48 > 0 && v47 > 0 )
                  W_rp_5b9bf0((intptr_t)((*ya__f)), (intptr_t)(a3), (intptr_t)((*v65__f)), (intptr_t)(v18), (intptr_t)(v48), (intptr_t)(v47), (intptr_t)(v49), (intptr_t)(0));
              }
            }
          }
          v11 = (_DWORD *)F32(v9, 0x174);
          v50 = ++(*v69__f) < v11[2];
          (*v62__f) += 36;
          (*v53__f) += 24;
          if ( !v50 )
            break;
          v14 = (*v71__f);
          v15 = (*v61__f);
        }
        result = (*v70__f);
        v12 = (*v63__f);
        v13 = (*v64__f);
      }
      v11 = (_DWORD *)F32(v9, 0x174);
      (*v70__f) = ++result;
      if ( result >= v11[3] )
        break;
      ++v12;
      v13 += 16;
      (*v63__f) = v12;
      (*v64__f) = v13;
    }
    while ( v12 < v11[4] );
  }
  return result;
}

/* 0x48e6d0: unsigned __int16 __thiscall glyph_ruby_draw_48E6D0( cls_48E6D0 *this, HDC hdc, int a3, int y, int a5, unsigned __int16 a6, unsigned __int16 a7) */
int32_t rd_48e6d0(wl_char *self, int32_t hdc, int32_t a3, int32_t y, int32_t a5, uint16_t a6, uint16_t a7)
{
    intptr_t c = (intptr_t)self;
    uint16_t result = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t i = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    int v16 = 0;
    uint16_t v20 = 0;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *const v17__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */

  result = a7;
  v8 = c;
  (*v17__f) = 0;
  if ( a7 == 0 || a7 - a6 > *(uint16_t *)&GAP(c, 0x4, 4) )
    result = *(_WORD *)&GAP(c, 0x4, 4) - a6;
  if ( result != 0 )
  {
    v20 = a6;
    (*v19__f) = result;
    do
    {
      v9 = F32(v8, 0x0);
      v10 = 0;
      v11 = 36 * v20;
      v12 = v11 + F32(v8, 0x0);
      if ( F16(v12, 28) != 0 )
      {
        for ( i = 6 * v20; ; i = 6 * v20 )
        {
          v14 = v10 + i;
          v15 = a3 + (*v17__f) + a5 * (v10 + F32(v12, 32));
          API_TextOutA((intptr_t)(hdc), (intptr_t)(v15), (intptr_t)(y), (intptr_t)((char *)(v9 + 6 * v14 + 3)), (intptr_t)(strlen((const char *)(v9 + 6 * v14 + 3))));
          API_TextOutA((intptr_t)(hdc), (intptr_t)(v15 + a5 / 2), (intptr_t)(y - a5 / 2), (intptr_t)((char *)(F32(c, 0x0) + 2 * (3 * v14 + 3))), (intptr_t)(strlen((const char *)(F32(c, 0x0) + 2 * (3 * v14 + 3)))));
          v9 = F32(c, 0x0);
          ++v10;
          v12 = F32(c, 0x0) + 36 * v20;
          if ( v10 >= F16(v12, 28) )
            break;
        }
        v11 = 36 * v20;
        v8 = c;
      }
      (*v17__f) += a5 * strlen((const char *)(F32(v8, 0x0) + v11));
      result = (*v19__f) - 1;
      v16 = (*v19__f) == 1;
      ++v20;
      --(*v19__f);
    }
    while ( !v16 );
  }
  return result;
}

/* 0x48e860: unsigned __int16 __thiscall sub_48E860( cls_48E860 *this, HDC hdc, int a3, int y, __int16 a5, unsigned __int16 a6, unsigned __int16 a7) */
int32_t rd_48e860(wl_char *self, int32_t hdc, int32_t a3, int32_t y, int16_t a5, uint16_t a6, uint16_t a7)
{
    intptr_t c = (intptr_t)self;
    uint16_t result = 0;
    uint16_t v8 = 0;
    uint16_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t i = 0;

  result = a7;
  v8 = a6;
  v9 = 0;
  if ( a7 == 0 || a7 - a6 > F16(c, 0x8) )
    result = F16(c, 0x8) - a6;
  if ( result != 0 )
  {
    v10 = F32(c, 0x0);
    for ( i = result; i != 0; --i )
    {
      v11 = 36 * v8;
      API_TextOutA((intptr_t)(hdc), (intptr_t)(a3 + v9), (intptr_t)(y), (intptr_t)((char *)(v11 + v10)), (intptr_t)(strlen((const char *)(v11 + v10))));
      v10 = F32(c, 0x0);
      v9 += a5 * strlen((const char *)(v11 + F32(c, 0x0)));
      ++v8;
      result = i - 1;
    }
  }
  return result;
}

/* 0x48e920: int __thiscall scene_fade_grid_render_48E920(cls_48E920 *this, int a2) */
int32_t rd_48e920(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t *** v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int32_t ** v17 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */

  result = F32(c, 0xc);
  (*v20__f) = c;
  if ( result != 0 )
  {
    result = 0;
    (*v18__f) = 0;
    if ( *(_WORD *)&GAP(c, 0x4, 4) != 0 )
    {
      SETLO(v3, a2);
      while ( 1 )
      {
        v4 = 3 * (uint16_t)result;
        v5 = F32(F32(c, 0x0) + 4 * v4, 4);
        v6 = F32(F32(c, 0x0) + 4 * v4, 8);
        v7 = (_WORD *)(F32(c, 0x0) + 4 * v4);
        v8 = v5 << 6;
        v9 = 4 * v6;
        v10 = F32(c, 0xc) - 1;
        if ( v10 == 0 )
          goto LABEL_18;
        v11 = v10 - 1;
        if ( v11 != 0 )
          break;
        SETHI(v12, 0);
        if ( *v7 == 0 )
          goto LABEL_21;
        SETLO(v12, *v7);
        v13 = v12 - 1;
        if ( v13 != 0 )
        {
          if ( v13 == 1 )
            goto LABEL_11;
        }
        else
        {
          v3 = 32 * (uint16_t)v7[1] / 1000;
        }
LABEL_23:
        if ( (uint16_t)v3 != 0 )
        {
          if ( (uint16_t)v3 >= 0x20u || F32(CFG(), 148) != 2 )
            goto LABEL_12;
          v17 = *(_DWORD ***)&SYSB(2944);
          (*v19__f) = SYS32(10328);
          if ( ***(_DWORD ***)&SYSB(2944) == 0 )
            W_fl_4184a0((intptr_t)(SYS32(2944)), (intptr_t)(0));
          W_rp_5bd550((intptr_t)((*v19__f)), (intptr_t)((intptr_t)a2), (intptr_t)(F32((**v17 + 4 * (uint16_t)v3), 0)), (intptr_t)(v8), (intptr_t)(v9), (intptr_t)(64), (intptr_t)(4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0x1FFFFFF), (intptr_t)(*(intptr_t *)&SYSB(864)));
        }
LABEL_15:
        c = (*v20__f);
        result = ++(*v18__f);
        if ( (uint16_t)(*v18__f) >= *(_WORD *)&GAP((*v20__f), 0x4, 4) )
          return result;
      }
      if ( v11 != 1 )
        goto LABEL_23;
LABEL_18:
      SETHI(v15, 0);
      if ( *v7 == 0 )
      {
LABEL_11:
        SETLO(v3, 32);
LABEL_12:
        v14 = SYS32(2948);
        if ( ***(_DWORD ***)&SYSB(2948) == 0 )
          W_fl_4184a0((intptr_t)(SYS32(2948)), (intptr_t)(0));
        W_rp_5b9a40((intptr_t)(***v14), (intptr_t)(a2), (intptr_t)(v8), (intptr_t)(v9));
        goto LABEL_15;
      }
      SETLO(v15, *v7);
      v16 = v15 - 1;
      if ( v16 != 0 )
      {
        if ( v16 == 1 )
        {
LABEL_21:
          SETLO(v3, 0);
          goto LABEL_15;
        }
      }
      else
      {
        SETLO(v3, 31 - 32 * (unsigned int)(uint16_t)v7[1] / 0x3E8);
      }
      goto LABEL_23;
    }
  }
  return result;
}

/* 0x48eac0: void __thiscall sub_48EAC0(cls_48EAC0 *this, int a2) */
void rd_48eac0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    int v4 = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint32_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    uint32_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    uint16_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    uint32_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    int v38 = 0;
    _Alignas(8) uint8_t frame_[0x38 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x38 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x34 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x30 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x2c */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x28 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x24 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x20 */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1c */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x18 */
    intptr_t *const v48__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x14 */
    intptr_t *const v49__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x10 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0xc */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x8 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x4 */

  v3 = 0;
  v4 = F16(c, 0x58) == 0;
  (*v46__f) = 0;
  if ( !v4 )
  {
    do
    {
      v5 = F32(c, 0x54);
      v6 = 0;
      (*v52__f) = 8 * (uint16_t)v3;
      v7 = v5 + (*v52__f);
      (*v45__f) = 0;
      if ( F16((v5 + (*v52__f)), 0) != 0 )
      {
        do
        {
          v8 = F32(v7, 4) + 60 * v6;
          switch ( F32(v8, 24) )
          {
            case 0:
              v11 = F32(v8, 0);
              v12 = F32(c, 0x34);
              (*v41__f) = F32(v8, 16);
              (*v47__f) = *(_DWORD *)&GAP(v11, 0x14, 12);
              (*v42__f) = F32(v8, 8);
              v13 = *(_DWORD *)&GAP(v11, 0x14, 8);
              v14 = (int)((uint64_t)(1374389535LL * F32(v8, 4)) >> 32) >> 5;
              (*v44__f) = (v12 + F32(c, 0x60)) / 100;
              v15 = (v14 >> 31) + v14 - (*v44__f);
              v16 = F32(c, 0x5c);
              v17 = F32(v8, 12) + v15;
              v18 = F32(c, 0x4c);
              v19 = 5 * F32(c, 0x74);
              (*v40__f) = v17;
              v20 = (int)((uint64_t)(1374389535LL * (v18 + v16 + 20 * v19)) >> 32) >> 5;
              (*v44__f) = (v20 >> 31) + v20;
              v21 = (*v41__f) + (*v42__f) / 100 - (*v44__f);
              (*v39__f) = v21;
              if ( v17 + v13 >= 0 && v21 + (*v47__f) >= 0 && F32(c, 0x64) / 100 > v17 && F32(c, 0x68) / 100 > v21 )
                W_rp_5b9b70((intptr_t)(v11), (intptr_t)(a2), (intptr_t)(v17), (intptr_t)((*v41__f) + (*v42__f) / 100 - (*v44__f)));
              break;
            case 1:
              v22 = F32(v8, 0);
              if ( W_view_project((intptr_t)(c), (intptr_t)(F32(v8, 4)), (intptr_t)(F32(v8, 8)), (intptr_t)(F32(v8, 12)), (intptr_t)(F32(v8, 16)), (intptr_t)(F32(F32(v8, 0), 28)), (intptr_t)(F32(F32(v8, 0), 32)), (intptr_t)(&(*v40__f)), (intptr_t)(&(*v39__f)), (intptr_t)(0)) )
              {
                W_rp_5bd550((intptr_t)(F32(v8, 20)), (intptr_t)((intptr_t)a2), (intptr_t)((int)v22), (intptr_t)((*v40__f) + F32(v22, 0xc)), (intptr_t)((*v39__f) + F32(v22, 0x10)), (intptr_t)(F32(((uint8_t *)(v22) + 0x14), 0)), (intptr_t)(*(_DWORD *)&GAP(v22, 0x14, 4)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(*(_DWORD *)&GAP(v22, 0x14, 20)), (intptr_t)(*(intptr_t *)&SYSB(864)));
              }
              break;
            case 2:
              v23 = F32(v8, 20);
              SETLO((*v49__f), F16(v8, 36));
              v24 = F16(v8, 28);
              (*v51__f) = v23;
              v25 = F32(v8, 40);
              v26 = 1000 * v24 / F16(v8, 30);
              v27 = F32(v25, 24);
              SETLO((*v48__f), F16(v8, 32));
              (*v41__f) = v27;
              v28 = F32(v8, 4);
              v29 = F32(v8, 8);
              (*v47__f) = v26;
              (*v44__f) = F32(v25, 20);
              v30 = F32(c, 0x60);
              (*v42__f) = (*v44__f) / 2;
              v31 = (int)((uint64_t)(1374389535LL * (v28 - v30 - F32(c, 0x34))) >> 32) >> 5;
              v32 = (v31 >> 31) + v31 - (*v44__f) / 2;
              v33 = 5 * F32(c, 0x74);
              (*v50__f) = (*v41__f) / 2;
              v34 = (v29 - 20 * v33 - F32(c, 0x5c) - F32(c, 0x4c)) / 100 - (*v41__f) / 2 - 8;
              if ( (*v44__f) + v32 >= 0 && v34 + (*v41__f) >= 0 && F32(c, 0x64) / 100 > v32 && F32(c, 0x68) / 100 > v34 )
              {
                W_rp_5b9450((intptr_t)(SYS32(860)), (intptr_t)((*v41__f)));
                W_rp_5b94e0((intptr_t)(*(intptr_t *)&SYSB(860)), (intptr_t)((intptr_t *)&(*hdc__f)));
                F32(v25, 8) = (uint16_t)(*v47__f);
                F32(v25, 12) = (uint16_t)(*v48__f);
                v35 = 4 * (uint16_t)(*v49__f) + 9081460;
                F32(v25, 28) = F32(SYS32(4 * (uint16_t)(*v49__f) + 10004), 0);
                v36 = (*v50__f);
                v37 = (*v42__f);
                F16(v25, 32) = F16(F32(v35, 0), 4);
                rd_48ef40(v25, (*hdc__f), v37, v36);
                W_rp_5b9500((intptr_t)(*(intptr_t *)&SYSB(860)), (intptr_t)((intptr_t)(*hdc__f)));
                W_rp_5bd550((intptr_t)((*v51__f)), (intptr_t)((intptr_t)a2), (intptr_t)(SYS32(860)), (intptr_t)(v32), (intptr_t)(v34), (intptr_t)((*v44__f)), (intptr_t)((*v41__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(*(intptr_t *)&SYSB(864)));
              }
              break;
            case 3:
              v9 = F32(v8, 0);
              if ( W_view_project((intptr_t)(c), (intptr_t)(F32(v8, 4)), (intptr_t)(F32(v8, 8)), (intptr_t)(F32(v8, 12)), (intptr_t)(F32(v8, 16)), (intptr_t)(F32(v8, 52)), (intptr_t)(F32(v8, 56)), (intptr_t)(&(*v40__f)), (intptr_t)(&(*v39__f)), (intptr_t)(0)) )
              {
                v10 = F32(v8, 20);
                if ( v10 != 0 )
                  W_rp_5bd550((intptr_t)(v10), (intptr_t)((intptr_t)a2), (intptr_t)(v9), (intptr_t)((*v40__f)), (intptr_t)((*v39__f)), (intptr_t)(F32(v8, 52)), (intptr_t)(F32(v8, 56)), (intptr_t)(F32(v8, 44)), (intptr_t)(F32(v8, 48)), (intptr_t)(F32(v9, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
                else
                  W_rp_5b9bf0((intptr_t)(v9), (intptr_t)(a2), (intptr_t)((*v40__f)), (intptr_t)((*v39__f)), (intptr_t)(F32(v8, 52)), (intptr_t)(F32(v8, 56)), (intptr_t)(F32(v8, 44)), (intptr_t)(F32(v8, 48)));
              }
              break;
            default:
              break;
          }
          v7 = (*v52__f) + F32(c, 0x54);
          v6 = (*v45__f) + 1;
          v38 = (uint16_t)++(*v45__f) < F16(v7, 0);
        }
        while ( v38 );
        v3 = (*v46__f);
      }
      v38 = (uint16_t)++v3 < F16(c, 0x58);
      (*v46__f) = v3;
    }
    while ( v38 );
  }
}

/* 0x48ef40: int __thiscall sub_48EF40(cls_48EF40 *this, HDC hdc, int a3, int a4) */
int32_t rd_48ef40(wl_char *self, int32_t hdc, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t result = 0;
    intptr_t v7 = 0;
    int v8 = 0;
    int v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t field_E = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    double v18 = 0;
    double v19 = 0;
    double v20 = 0;
    double v21 = 0;
    double v22 = 0;
    double v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t * v33 = 0;
    double v34 = 0;
    double v35 = 0;
    double v36 = 0;
    double v37 = 0;
    double v38 = 0;
    double v39 = 0;
    intptr_t v40 = 0;
    char * v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    char v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    uint32_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    uint32_t v55 = 0;
    uint16_t i = 0;
    intptr_t v58 = 0;
    _Alignas(8) uint8_t frame_[0x1a0 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    POINT *apt = (POINT *)(frame_ + 0x11c);   /* [16] at ebp-0x84 */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1a0 */
    intptr_t * *const v64__f = (intptr_t * *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t * *const v63__f = (intptr_t * *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const n__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const m__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const kk__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const jj__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t *const ii__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x188 */
    intptr_t * *const p_y__f = (intptr_t * *)(frame_ + 0x1c);   /* ebp-0x184 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x184 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x184 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x184 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x184 */
    intptr_t *const v74__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x184 */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x184 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x184 */
    intptr_t *const v85__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x180 */
    intptr_t *const v84__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x180 */
    intptr_t *const v83__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x180 */
    intptr_t *const v82__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x180 */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x180 */
    intptr_t *const v80__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x180 */
    intptr_t *const v79__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x180 */
    intptr_t *const v93__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x17c */
    intptr_t *const v92__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x17c */
    intptr_t *const v91__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x17c */
    intptr_t *const v90__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x17c */
    intptr_t *const v89__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x17c */
    intptr_t *const v88__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x17c */
    intptr_t *const v87__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x17c */
    intptr_t *const v86__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x17c */
    intptr_t *const v99__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v98__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v97__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v96__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v95__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v94__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v103__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v102__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v101__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v100__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x178 */
    intptr_t *const v109__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x174 */
    intptr_t *const v108__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x174 */
    intptr_t *const v106__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x174 */
    intptr_t *const v104__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x174 */
    intptr_t *const k__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x174 */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x174 */
    char * *const v110__f = (char * *)(frame_ + 0x2c);   /* ebp-0x174 */
    intptr_t * *const v118__f = (intptr_t * *)(frame_ + 0x30);   /* ebp-0x170 */
    intptr_t * *const v117__f = (intptr_t * *)(frame_ + 0x30);   /* ebp-0x170 */
    intptr_t * *const v116__f = (intptr_t * *)(frame_ + 0x30);   /* ebp-0x170 */
    intptr_t * *const v115__f = (intptr_t * *)(frame_ + 0x30);   /* ebp-0x170 */
    intptr_t * *const v114__f = (intptr_t * *)(frame_ + 0x30);   /* ebp-0x170 */
    intptr_t *const v119__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x170 */
    intptr_t *const v113__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x170 */
    double *const v112__f = (double *)(frame_ + 0x30);   /* ebp-0x170 */
    double *const v111__f = (double *)(frame_ + 0x30);   /* ebp-0x170 */
    intptr_t *const v121__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x168 */
    intptr_t *const v120__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x168 */
    intptr_t *const v123__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x164 */
    intptr_t *const v122__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x164 */
    intptr_t *const v127__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x160 */
    intptr_t *const v126__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x160 */
    intptr_t *const v124__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x160 */
    double *const v125__f = (double *)(frame_ + 0x40);   /* ebp-0x160 */
    intptr_t *const v128__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x158 */
    intptr_t *const v129__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x154 */
    intptr_t *const v130__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x150 */
    intptr_t *const v131__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x14c */
    double *const v132__f = (double *)(frame_ + 0x58);   /* ebp-0x148 */
    intptr_t *const v133__f = (intptr_t *)(frame_ + 0x60);   /* ebp-0x140 */
    intptr_t *const v134__f = (intptr_t *)(frame_ + 0x64);   /* ebp-0x13c */
    double *const v135__f = (double *)(frame_ + 0x68);   /* ebp-0x138 */
    intptr_t *const v136__f = (intptr_t *)(frame_ + 0x70);   /* ebp-0x130 */
    intptr_t *const v137__f = (intptr_t *)(frame_ + 0x74);   /* ebp-0x12c */
    intptr_t *const v138__f = (intptr_t *)(frame_ + 0x78);   /* ebp-0x128 */
    POINT *const v139__f = (POINT *)(frame_ + 0x7c);   /* ebp-0x124 */
    POINT *const v140__f = (POINT *)(frame_ + 0xcc);   /* ebp-0xd4 */
    intptr_t *const v142__f = (intptr_t *)(frame_ + 0x19c);   /* ebp-0x4 */

  v5 = F32(((uint8_t *)(c) + 0xc), 0);
  (*v133__f) = v5 % 250;
  v7 = v5 % 500;
  result = v5 / 500;
  v8 = F32(c, 0x8) < 500;
  (*v134__f) = v7;
  if ( v8 )
  {
    for ( i = 0; i < *(_WORD *)&GAP(c, 0x18, 8); ++i )
    {
      API_SelectObject((intptr_t)(hdc), (intptr_t)(*(intptr_t *)(*(_DWORD *)&GAP(c, 0x18, 4) + 4 * i)));
      v58 = F32(c, 0x8) * (96 * F32(c, 0x10) / 1000 - 4) / 500;
      result = W_rp_5b5eb0((intptr_t)(hdc), (intptr_t)(a3 - v58), (intptr_t)(a4), (intptr_t)(v58 + a3), (intptr_t)(a4));
    }
  }
  else
  {
    result = 0;
    v9 = *(_WORD *)&GAP(c, 0x18, 8) == 0;
    (*v136__f) = 0;
    if ( !v9 )
    {
      v10 = a3;
      do
      {
        (*v124__f) = (uint16_t)result;
        API_SelectObject((intptr_t)(hdc), (intptr_t)(*(intptr_t *)(*(_DWORD *)&GAP(c, 0x18, 4) + 4 * (uint16_t)result)));
        API_Arc((intptr_t)(hdc), (intptr_t)(v10 - (F32(c, 0x10) << 6) / 1000), (intptr_t)(a4 - (F32(c, 0x10) << 6) / 1000 * (F32(c, 0x8) - 500) / 500), (intptr_t)((F32(c, 0x10) << 6) / 1000 + v10), (intptr_t)((F32(c, 0x10) << 6) / 1000 * (F32(c, 0x8) - 500) / 500 + a4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        v11 = 76 * F32(c, 0x10) / 1000;
        v12 = v11 * (F32(c, 0x8) - 500) / 500;
        API_Arc((intptr_t)(hdc), (intptr_t)(v10 - v11), (intptr_t)(a4 - v12), (intptr_t)(v11 + v10), (intptr_t)(v12 + a4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        API_Arc((intptr_t)(hdc), (intptr_t)(v10 - 92 * F32(c, 0x10) / 1000), (intptr_t)(a4 - 92 * F32(c, 0x10) / 1000 * (F32(c, 0x8) - 500) / 500), (intptr_t)(92 * F32(c, 0x10) / 1000 + v10), (intptr_t)(92 * F32(c, 0x10) / 1000 * (F32(c, 0x8) - 500) / 500 + a4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        v13 = 96 * F32(c, 0x10) / 1000;
        v14 = v13 * (F32(c, 0x8) - 500) / 500;
        API_Arc((intptr_t)(hdc), (intptr_t)(v10 - v13), (intptr_t)(a4 - v14), (intptr_t)(v13 + v10), (intptr_t)(v14 + a4), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
        field_E = F32(c, 0x10);
        (*v60__f) = 92 * field_E / 1000;
        (*v120__f) = 24;
        (*v71__f) = 60 * field_E / 1000;
        (*v104__f) = F32(c, 0x8);
        (*v86__f) = 0;
        (*v94__f) = 0;
        do
        {
          (*v122__f) = 0;
          (*v142__f) = ((int32_t *)D(0x5cc450))[33 * ((*v86__f) % 12) + 32];
          v16 = (*v142__f);
          if ( (*v142__f) > 0 )
          {
            v17 = 0;
            (*v132__f) = (double)((*v94__f) + 360 * (*v133__f) / 250);
            do
            {
              W_shot_476550((intptr_t)(((int32_t *)D(0x5cc450))[33 * ((*v86__f) % 12) + 2 * v17] * (*v71__f) / 100), (intptr_t)((*v71__f) * ((int32_t *)D(0x5cc450))[33 * ((*v86__f) % 12) + 1 + 2 * v17] / 100 - (*v60__f)), (intptr_t)(&(*v137__f)), (intptr_t)(&(*v135__f)));
              (*v135__f) = rd_490090((*v135__f), (*v132__f) * (*(const double *)D(0x8a9a38)));
              rd_4900d0((*v137__f), (*v135__f), a3, 0, &apt[v17].x, &(*v138__f));
              apt[v17].y = a4 + ((*v104__f) - 500) * (*v138__f) / 500;
              v16 = (*v142__f);
              v17 = (uint16_t)++(*v122__f);
            }
            while ( (uint16_t)(*v122__f) < (*v142__f) );
            v10 = a3;
          }
          API_Polyline((intptr_t)(hdc), (intptr_t)(apt), (intptr_t)(v16));
          v9 = (*v120__f) == 1;
          ++(*v86__f);
          (*v94__f) += 15;
          --(*v120__f);
        }
        while ( !v9 );
        if ( F32(c, 0x0) != 0 )
        {
          (*v61__f) = F32(c, 0x10);
          v18 = (double)(360 * (250 - (*v133__f)) / 250) * (*(const double *)D(0x8a9a38));
          v19 = (double)((*v61__f) * F32(((uint8_t *)(c) + 0x24), 0) / 1000);
          v20 = sin(v18) * v19;
          if ( v20 >= 0.0 )
            v21 = v20 + 0.5;
          else
            v21 = v20 - 0.5;
          (*v121__f) = v10 + (int64_t)v21;
          v22 = cos(v18) * v19;
          if ( v22 >= 0.0 )
            v23 = v22 + 0.5;
          else
            v23 = v22 - 0.5;
          v24 = F32(c, 0x8);
          v25 = a4 - (int)((int64_t)v23 * (v24 - 500)) / 500;
          (*v87__f) = ((*v61__f) << 6) / 1000;
          (*v123__f) = 360 * (*v134__f) / 500;
          switch ( F32(c, 0x0) )
          {
            case 1:
              (*v95__f) = v24 - 500;
              (*v79__f) = 360 * (*v134__f) / 500;
              (*v62__f) = 2;
              (*v132__f) = (double)((*v87__f) - 2);
              do
              {
                (*p_y__f) = &(*v139__f).y;
                (*v88__f) = (*v79__f);
                for ( (*j__f) = 4; (*j__f) != 0; --(*j__f) )
                {
                  (*v111__f) = (double)(*v88__f) * (*(const double *)D(0x8a9a38));
                  (*v129__f) = ftol_round_(sin((*v111__f)) * (*v132__f));
                  v26 = ftol_round_(cos((*v111__f)) * (*v132__f));
                  *((*p_y__f) - 1) = (*v129__f) + (*v121__f);
                  (*v131__f) = -v26;
                  (*v88__f) += 120;
                  *(*p_y__f) = v25 + -v26 * (*v95__f) / 500;
                  (*p_y__f) += 2;
                }
                API_Polyline((intptr_t)(hdc), (intptr_t)(&(*v139__f)), (intptr_t)(4));
                v9 = (*v62__f) == 1;
                (*v79__f) += 180;
                --(*v62__f);
              }
              while ( !v9 );
              break;
            case 2:
              (*v96__f) = v24 - 500;
              (*v80__f) = 360 * (*v134__f) / 500;
              (*v63__f) = &(*v139__f).y;
              (*v106__f) = 6;
              (*v112__f) = (double)((*v87__f) - 2);
              do
              {
                (*v132__f) = (double)(*v80__f) * (*(const double *)D(0x8a9a38));
                (*v129__f) = ftol_round_(sin((*v132__f)) * (*v112__f));
                v27 = ftol_round_(cos((*v132__f)) * (*v112__f));
                *((*v63__f) - 1) = (*v129__f) + (*v121__f);
                (*v131__f) = -v27;
                (*v80__f) += 144;
                *(*v63__f) = v25 + -v27 * (*v96__f) / 500;
                (*v63__f) += 2;
                --(*v106__f);
              }
              while ( (*v106__f) != 0 );
              API_Polyline((intptr_t)(hdc), (intptr_t)(&(*v139__f)), (intptr_t)(6));
              break;
            case 3:
              (*v113__f) = 2;
              (*v97__f) = v24 - 500;
              (*v73__f) = (*v87__f) - 2;
              (*v89__f) = 360 * (*v134__f) / 500;
              do
              {
                (*v64__f) = &(*v139__f).y;
                (*v81__f) = (*v89__f);
                for ( (*k__f) = 5; (*k__f) != 0; --(*k__f) )
                {
                  rd_4900d0((*v73__f), (double)(*v81__f) * (*(const double *)D(0x8a9a38)), 0, 0, &(*v129__f), &(*v131__f));
                  v28 = (*v131__f) * (*v97__f);
                  *((*v64__f) - 1) = (*v129__f) + (*v121__f);
                  (*v81__f) += 90;
                  *(*v64__f) = v25 + v28 / 500;
                  (*v64__f) += 2;
                }
                API_Polyline((intptr_t)(hdc), (intptr_t)(&(*v139__f)), (intptr_t)(5));
                v9 = (*v113__f) == 1;
                (*v89__f) += 45;
                --(*v113__f);
              }
              while ( !v9 );
              break;
            case 4:
              (*v98__f) = v24 - 500;
              (*v114__f) = &(*v139__f).y;
              (*v82__f) = 360 * (*v134__f) / 500;
              for ( (*m__f) = 9; (*m__f) != 0; --(*m__f) )
              {
                rd_4900d0((*v87__f) - 2, (double)(*v82__f) * (*(const double *)D(0x8a9a38)), 0, 0, &(*v129__f), &(*v131__f));
                v29 = (*v131__f) * (*v98__f);
                *((*v114__f) - 1) = (*v121__f) + (*v129__f);
                (*v82__f) += 135;
                *(*v114__f) = v25 + v29 / 500;
                (*v114__f) += 2;
              }
              API_Polyline((intptr_t)(hdc), (intptr_t)(&(*v139__f)), (intptr_t)(9));
              break;
            default:
              break;
          }
          if ( (*v124__f) == *(uint16_t *)&GAP(c, 0x18, 8) - 1 )
          {
            v30 = F32(c, 0x8);
            v31 = (F32(c, 0x10) << 6) / 1000;
            switch ( F32(c, 0x0) )
            {
              case 1:
                (*v99__f) = v30 - 500;
                (*v83__f) = (*v123__f);
                (*v108__f) = 2;
                (*v125__f) = (double)(v31 - 2);
                do
                {
                  (*v115__f) = &(*v140__f).y;
                  (*v74__f) = (*v83__f);
                  for ( (*n__f) = 4; (*n__f) != 0; --(*n__f) )
                  {
                    (*v132__f) = (double)(*v74__f) * (*(const double *)D(0x8a9a38));
                    (*v128__f) = ftol_round_(sin((*v132__f)) * (*v125__f));
                    v32 = -(int)ftol_round_(cos((*v132__f)) * (*v125__f));
                    v33 = (*v115__f);
                    (*v130__f) = v32;
                    *((*v115__f) - 1) = (*v128__f) + (*v121__f) + 2;
                    (*v115__f) += 2;
                    (*v74__f) += 120;
                    *v33 = v25 + v32 * (*v99__f) / 500;
                  }
                  API_Polyline((intptr_t)(hdc), (intptr_t)(&(*v140__f)), (intptr_t)(4));
                  v9 = (*v108__f) == 1;
                  (*v83__f) += 180;
                  --(*v108__f);
                }
                while ( !v9 );
                break;
              case 2:
                (*v90__f) = v31 - 2;
                (*v100__f) = v30 - 500;
                (*v116__f) = &(*v140__f).y;
                (*v75__f) = (*v123__f);
                for ( (*ii__f) = 6; (*ii__f) != 0; --(*ii__f) )
                {
                  rd_4900d0((*v90__f), (double)(*v75__f) * (*(const double *)D(0x8a9a38)), 0, 0, &(*v128__f), &(*v130__f));
                  *((*v116__f) - 1) = (*v128__f) + (*v121__f) + 2;
                  (*v75__f) += 144;
                  *(*v116__f) = v25 + (*v130__f) * (*v100__f) / 500;
                  (*v116__f) += 2;
                }
                API_Polyline((intptr_t)(hdc), (intptr_t)(&(*v140__f)), (intptr_t)(6));
                break;
              case 3:
                (*v101__f) = v30 - 500;
                (*v91__f) = v31 - 2;
                (*v84__f) = (*v123__f);
                (*v109__f) = 2;
                do
                {
                  (*v117__f) = &(*v140__f).y;
                  (*v76__f) = (*v84__f);
                  for ( (*jj__f) = 5; (*jj__f) != 0; --(*jj__f) )
                  {
                    rd_4900d0((*v91__f), (double)(*v76__f) * (*(const double *)D(0x8a9a38)), 0, 0, &(*v128__f), &(*v130__f));
                    *((*v117__f) - 1) = (*v128__f) + (*v121__f) + 2;
                    (*v76__f) += 90;
                    *(*v117__f) = v25 + (*v130__f) * (*v101__f) / 500;
                    (*v117__f) += 2;
                  }
                  API_Polyline((intptr_t)(hdc), (intptr_t)(&(*v140__f)), (intptr_t)(5));
                  v9 = (*v109__f) == 1;
                  (*v84__f) += 45;
                  --(*v109__f);
                }
                while ( !v9 );
                break;
              case 4:
                (*v92__f) = v31 - 2;
                (*v102__f) = v30 - 500;
                (*v118__f) = &(*v140__f).y;
                (*v77__f) = (*v123__f);
                for ( (*kk__f) = 9; (*kk__f) != 0; --(*kk__f) )
                {
                  rd_4900d0((*v92__f), (double)(*v77__f) * (*(const double *)D(0x8a9a38)), 0, 0, &(*v128__f), &(*v130__f));
                  *((*v118__f) - 1) = (*v128__f) + (*v121__f) + 2;
                  (*v77__f) += 135;
                  *(*v118__f) = v25 + (*v130__f) * (*v102__f) / 500;
                  (*v118__f) += 2;
                }
                API_Polyline((intptr_t)(hdc), (intptr_t)(&(*v140__f)), (intptr_t)(9));
                break;
              default:
                break;
            }
          }
        }
        if ( F32(c, 0x4) != 0 )
        {
          v34 = (double)(360 * (250 - (*v133__f)) / 250) * (*(const double *)D(0x8a9a38));
          v35 = (double)(F32(c, 0x10) * *(_DWORD *)&GAP(c, 0x24, 4) / 1000);
          v36 = sin(v34) * v35;
          if ( v36 >= 0.0 )
            v37 = v36 + 0.5;
          else
            v37 = v36 - 0.5;
          v10 = a3;
          v38 = cos(v34) * v35;
          if ( v38 >= 0.0 )
            v39 = v38 + 0.5;
          else
            v39 = v38 - 0.5;
          rd_4903f0(
            F32(c, 0x4),
            hdc,
            a3 + (int64_t)v37,
            a4 - (int)((int64_t)v39 * (F32(c, 0x8) - 500)) / 500,
            F32(c, 0x8),
            360 * (*v134__f) / 500,
            40 * F32(c, 0x10) / 1000);
        }
        else
        {
          v10 = a3;
        }
        (*v119__f) = F32(c, 0x8);
        if ( (*v119__f) > 800 )
        {
          v40 = F32(c, 0x10);
          (*v70__f) = 0;
          (*v93__f) = (uint16_t)(abs32(50 - F32(((uint8_t *)(c) + 0xc), 0) % 100) + 20);
          (*v85__f) = (*v93__f) * (40 * v40 / 1000) / 70;
          (*v78__f) = 8 * (*v93__f) / 70;
          (*v126__f) = ((*v119__f) - 800) * (75 * v40 / 1000) / 200 + (*v78__f) - 8;
          if ( GAP(c, 0x24, 8) != 0 )
          {
            v41 = &GAP(c, 0x24, 8);
            (*v103__f) = 360 - 360 * F32(((uint8_t *)(c) + 0xc), 0) / 1000;
            (*v110__f) = &GAP(c, 0x24, 8);
            do
            {
              v42 = *v41;
              v43 = v42 - 65;
              if ( v42 - 65 < 0 || v43 >= 26 )
              {
                v45 = v42 - 49;
                if ( v42 - 49 < 0 || v45 >= 3 )
                  goto LABEL_71;
                v44 = 132 * v45 + 6081152;
              }
              else
              {
                v44 = 132 * v43 + 6081552;
              }
              rd_490160(hdc, v44, v10, a4, (*v85__f), (*v126__f), (*v103__f), (*v119__f));
              v41 = (*v110__f);
LABEL_71:
              v46 = *v41;
              if ( v46 == 32 )
              {
                v47 = (*v103__f) + 8;
              }
              else if ( v46 == 49 || v46 == 73 )
              {
                v47 = (*v103__f) + 4;
              }
              else
              {
                v47 = (*v103__f) + 7;
              }
              (*v103__f) = v47;
              v41 = &GAP(c, 0x24, (uint16_t)++(*v70__f) + 8);
              (*v110__f) = v41;
            }
            while ( *v41 != 0 );
          }
          v48 = F32(c, 0x10);
          (*v59__f) = (*v93__f) * (40 * v48 / 1000) / 70;
          v49 = 75 * v48 / 1000;
          v50 = F32(c, 0x8);
          v51 = (int)((uint64_t)(1374389535LL * (v50 - 800) * v49) >> 32) >> 6;
          rd_490860(
            hdc,
            v10,
            a4,
            240 - 360 * F32(((uint8_t *)(c) + 0xc), 0) / 1000,
            v50,
            (v51 >> 31) + v51 + (*v78__f) - 8,
            (*v59__f),
            &GAP(c, 0x24, 8));
          v52 = F32(c, 0x10);
          (*v127__f) = (*v93__f) * (40 * v52 / 1000);
          v53 = 75 * v52 / 1000;
          v54 = F32(c, 0x8);
          v55 = (int)((uint64_t)(1374389535LL * (v54 - 800) * v53) >> 32) >> 6;
          rd_490860(
            hdc,
            v10,
            a4,
            120 - 360 * F32(((uint8_t *)(c) + 0xc), 0) / 1000,
            v54,
            (v55 >> 31) + v55 + (*v78__f) - 8,
            (*v127__f) / 70,
            &GAP(c, 0x24, 8));
        }
        result = (*v136__f) + 1;
      }
      while ( (uint16_t)++(*v136__f) < *(_WORD *)&GAP(c, 0x18, 8) );
    }
  }
  return result;
}

/* 0x490090: double __stdcall sub_490090(double a1, double a2) */
double rd_490090(double a1, double a2)
{


  return (double)(((int)(int64_t)(a1 / (*(const double *)D(0x8a9a38)) + a2 / (*(const double *)D(0x8a9a38))) % 360 + 360) % 360) * (*(const double *)D(0x8a9a38));
}

/* 0x4900d0: int __stdcall sub_4900D0(int a1, long double a2, int a3, int a4, _DWORD *a5, _DWORD *a6) */
int32_t rd_4900d0(int32_t a1, double a2, int32_t a3, int32_t a4, int32_t * a5, int32_t * a6)
{

    double v6 = 0;
    double v7 = 0;
    double v8 = 0;
    double v9 = 0;
    int64_t v10 = 0;

  v6 = (double)a1;
  v7 = sin(a2) * v6;
  if ( v7 >= 0.0 )
    v8 = v7 + 0.5;
  else
    v8 = v7 - 0.5;
  *a5 = a3 + (int64_t)v8;
  v9 = v6 * cos(a2);
  if ( v9 >= 0.0 )
    v10 = (int64_t)(v9 + 0.5);
  else
    v10 = (int64_t)(v9 - 0.5);
  *a6 = a4 - v10;
  return v10;
}

/* 0x490160: BOOL __stdcall sub_490160(HDC hdc, int a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t rd_490160(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{

    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    double v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    double v14 = 0;
    double v15 = 0;
    double v16 = 0;
    double v17 = 0;
    double v18 = 0;
    double v19 = 0;
    double v20 = 0;
    double v21 = 0;
    double v22 = 0;
    double v23 = 0;
    double v24 = 0;
    _Alignas(8) uint8_t frame_[0x8c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    POINT *apt = (POINT *)(frame_ + 0x8);   /* [16] at ebp-0x84 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8c */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x88);   /* ebp-0x4 */

  v8 = a2;
  v9 = 0;
  (*v26__f) = 0;
  (*v28__f) = F32(a2, 128);
  v10 = (*v28__f);
  if ( (*v28__f) > 0 )
  {
    v11 = (double)a7 * (*(const double *)D(0x8a9a38)) / (*(const double *)D(0x8a9a38));
    while ( 1 )
    {
      v12 = a5 * F32(v8 + 8 * v9, 4) / 100 - a6;
      v13 = a5 * F32((v8 + 8 * v9), 0) / 100;
      if ( v13 != 0 || v12 != 0 )
      {
        if ( (int)abs32(v12) >= (int)abs32(a5 * F32((v8 + 8 * v9), 0) / 100) )
        {
          if ( v12 >= 0 )
          {
            v16 = atan2((double)v13 / (double)v12, 1.0);
            v14 = v13 >= 0 ? (*(const double *)D(0x8a6b48)) - v16 : (*(const double *)D(0x8a6b40)) - v16;
          }
          else
          {
            v14 = atan2((double)v13 / (double)v12, 1.0) * -1.0;
          }
        }
        else
        {
          v15 = atan2((double)v12 / (double)v13, 1.0);
          v14 = v13 >= 0 ? v15 + (*(const double *)D(0x8a6b28)) : v15 + (*(const double *)D(0x8a6618));
        }
      }
      else
      {
        v14 = 0.0;
      }
      v17 = (v14 < (*(const double *)D(0x8a6b28)) || v14 >= (*(const double *)D(0x8a6b48))) && (v14 < (*(const double *)D(0x8a6618)) || v14 >= 0.0)
          ? (double)v12 / cos(v14) * -1.0
          : (double)v13 / sin(v14);
      v18 = v17 >= 0.0 ? v17 + 0.5 : v17 - 0.5;
      v19 = (double)(((int)(int64_t)(v14 / (*(const double *)D(0x8a9a38)) + v11) % 360 + 360) % 360) * (*(const double *)D(0x8a9a38));
      v20 = (double)(int)(int64_t)v18;
      v21 = sin(v19) * v20;
      v22 = v21 >= 0.0 ? v21 + 0.5 : v21 - 0.5;
      apt[v9].x = a3 + (int64_t)v22;
      v23 = cos(v19) * v20;
      v24 = v23 >= 0.0 ? v23 + 0.5 : v23 - 0.5;
      apt[v9].y = a4 - (int)((int64_t)v24 * (a8 - 500)) / 500;
      v10 = (*v28__f);
      v9 = (uint16_t)++(*v26__f);
      if ( (uint16_t)(*v26__f) >= (*v28__f) )
        break;
      v8 = a2;
    }
  }
  return API_Polyline((intptr_t)(hdc), (intptr_t)(apt), (intptr_t)(v10));
}

/* 0x4903f0: int __stdcall sub_4903F0(int a1, HDC hdc, int a3, int a4, int a5, int a6, int a7) */
int32_t rd_4903f0(int32_t a1, int32_t hdc, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{

    intptr_t result = 0;
    intptr_t v8 = 0;
    double v9 = 0;
    intptr_t * p_y = 0;
    intptr_t j = 0;
    double v12 = 0;
    double v13 = 0;
    double v14 = 0;
    double v15 = 0;
    double v16 = 0;
    int v17 = 0;
    double v18 = 0;
    intptr_t * v19 = 0;
    intptr_t k = 0;
    double v21 = 0;
    double v22 = 0;
    double v23 = 0;
    double v24 = 0;
    double v25 = 0;
    intptr_t v26 = 0;
    double v27 = 0;
    intptr_t * v28 = 0;
    intptr_t n = 0;
    double v30 = 0;
    double v31 = 0;
    double v32 = 0;
    double v33 = 0;
    double v34 = 0;
    double v35 = 0;
    intptr_t * v36 = 0;
    intptr_t ii = 0;
    double v38 = 0;
    double v39 = 0;
    double v40 = 0;
    double v41 = 0;
    double v42 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    _Alignas(8) uint8_t frame_[0x58 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    double *const m__f = (double *)(frame_ + 0x0);   /* ebp-0x58 */
    double *const i__f = (double *)(frame_ + 0x0);   /* ebp-0x58 */
    POINT *const apt__f = (POINT *)(frame_ + 0x8);   /* ebp-0x50 */

  result = a1 - 1;
  switch ( a1 )
  {
    case 1:
      v8 = a5 - 500;
      v9 = (double)(a7 - 2);
      v46 = a6;
      v48 = 2;
      for ( (*i__f) = v9; ; v9 = (*i__f) )
      {
        p_y = &(*apt__f).y;
        v50 = v46;
        for ( j = 4; j != 0; --j )
        {
          v12 = (double)v50 * (*(const double *)D(0x8a9a38));
          v13 = sin(v12) * v9;
          if ( v13 >= 0.0 )
            v14 = v13 + 0.5;
          else
            v14 = v13 - 0.5;
          v15 = cos(v12) * v9;
          if ( v15 >= 0.0 )
            v16 = v15 + 0.5;
          else
            v16 = v15 - 0.5;
          *(p_y - 1) = a3 + (int64_t)v14;
          p_y += 2;
          *(p_y - 2) = a4 - (int)((int64_t)v16 * v8) / 500;
          v50 += 120;
        }
        API_Polyline((intptr_t)(hdc), (intptr_t)(&(*apt__f)), (intptr_t)(4));
        result = v48 - 1;
        v17 = v48 == 1;
        v46 += 180;
        --v48;
        if ( v17 )
          break;
      }
      break;
    case 2:
      v18 = (double)(a7 - 2);
      v19 = &(*apt__f).y;
      v51 = a6;
      for ( k = 6; k != 0; --k )
      {
        v21 = (double)v51 * (*(const double *)D(0x8a9a38));
        v22 = sin(v21) * v18;
        if ( v22 >= 0.0 )
          v23 = v22 + 0.5;
        else
          v23 = v22 - 0.5;
        v24 = cos(v21) * v18;
        if ( v24 >= 0.0 )
          v25 = v24 + 0.5;
        else
          v25 = v24 - 0.5;
        v19 += 2;
        *(v19 - 3) = a3 + (int64_t)v23;
        *(v19 - 2) = a4 - (int)((int64_t)v25 * (a5 - 500)) / 500;
        v51 += 144;
      }
      result = API_Polyline((intptr_t)(hdc), (intptr_t)(&(*apt__f)), (intptr_t)(6));
      break;
    case 3:
      v26 = a5 - 500;
      v27 = (double)(a7 - 2);
      v47 = a6;
      v49 = 2;
      for ( (*m__f) = v27; ; v27 = (*m__f) )
      {
        v28 = &(*apt__f).y;
        v52 = v47;
        for ( n = 5; n != 0; --n )
        {
          v30 = (double)v52 * (*(const double *)D(0x8a9a38));
          v31 = sin(v30) * v27;
          if ( v31 >= 0.0 )
            v32 = v31 + 0.5;
          else
            v32 = v31 - 0.5;
          v33 = cos(v30) * v27;
          if ( v33 >= 0.0 )
            v34 = v33 + 0.5;
          else
            v34 = v33 - 0.5;
          *(v28 - 1) = a3 + (int64_t)v32;
          v28 += 2;
          *(v28 - 2) = a4 - (int)((int64_t)v34 * v26) / 500;
          v52 += 90;
        }
        API_Polyline((intptr_t)(hdc), (intptr_t)(&(*apt__f)), (intptr_t)(5));
        result = v49 - 1;
        v17 = v49 == 1;
        v47 += 45;
        --v49;
        if ( v17 )
          break;
      }
      break;
    case 4:
      v35 = (double)(a7 - 2);
      v36 = &(*apt__f).y;
      v53 = a6;
      for ( ii = 9; ii != 0; --ii )
      {
        v38 = (double)v53 * (*(const double *)D(0x8a9a38));
        v39 = sin(v38) * v35;
        if ( v39 >= 0.0 )
          v40 = v39 + 0.5;
        else
          v40 = v39 - 0.5;
        v41 = cos(v38) * v35;
        if ( v41 >= 0.0 )
          v42 = v41 + 0.5;
        else
          v42 = v41 - 0.5;
        v36 += 2;
        *(v36 - 3) = a3 + (int64_t)v40;
        *(v36 - 2) = a4 - (int)((int64_t)v42 * (a5 - 500)) / 500;
        v53 += 135;
      }
      result = API_Polyline((intptr_t)(hdc), (intptr_t)(&(*apt__f)), (intptr_t)(9));
      break;
    default:
      return result;
  }
  return result;
}

/* 0x490860: void __stdcall sub_490860(HDC hdc, int a2, int a3, int a4, int a5, int a6, int a7, _BYTE *a8) */
void rd_490860(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, uint8_t * a8)
{

    uint8_t * v8 = 0;
    char * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    double v18 = 0;
    double v19 = 0;
    double v20 = 0;
    double v21 = 0;
    double v22 = 0;
    double v23 = 0;
    double v24 = 0;
    double v25 = 0;
    double v26 = 0;
    double v27 = 0;
    char v28 = 0;
    intptr_t v29 = 0;
    _Alignas(8) uint8_t frame_[0xb4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    POINT *apt = (POINT *)(frame_ + 0x30);   /* [16] at ebp-0x84 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xb4 */
    double *const v32__f = (double *)(frame_ + 0xc);   /* ebp-0xa8 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xa0 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x9c */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x98 */
    char * *const v36__f = (char * *)(frame_ + 0x20);   /* ebp-0x94 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0xb0);   /* ebp-0x4 */

  v8 = a8;
  (*v35__f) = 0;
  if ( *a8 != 0 )
  {
    v9 = a8;
    (*v36__f) = a8;
    while ( 1 )
    {
      v10 = *v9;
      v11 = v10 - 65;
      if ( v10 - 65 >= 0 && v11 < 26 )
        break;
      v13 = v10 - 49;
      if ( v10 - 49 >= 0 && v13 < 3 )
      {
        v12 = 132 * v13 + 6081152;
        goto LABEL_9;
      }
LABEL_40:
      v28 = *v9;
      if ( *v9 == 32 )
      {
        v29 = a4 + 8;
      }
      else if ( v28 == 49 || v28 == 73 )
      {
        v29 = a4 + 4;
      }
      else
      {
        v29 = a4 + 7;
      }
      a4 = v29;
      v9 = &v8[(uint16_t)++(*v35__f)];
      (*v36__f) = v9;
      if ( *v9 == 0 )
        return;
    }
    v12 = 132 * v11 + 6081552;
LABEL_9:
    v14 = F32(v12, 128);
    (*v34__f) = v12;
    (*v38__f) = v14;
    (*v33__f) = 0;
    if ( v14 > 0 )
    {
      v15 = 0;
      while ( 1 )
      {
        v16 = a7 * F32(v12 + 8 * v15, 4) / 100 - a6;
        v17 = F32((v12 + 8 * v15), 0) * a7 / 100;
        if ( v17 != 0 || v16 != 0 )
        {
          if ( (int)abs32(v16) >= (int)abs32(F32((v12 + 8 * v15), 0) * a7 / 100) )
          {
            if ( v16 >= 0 )
            {
              v20 = atan2((double)v17 / (double)v16, 1.0);
              v19 = v17 >= 0 ? (*(const double *)D(0x8a6b48)) - v20 : (*(const double *)D(0x8a6b40)) - v20;
            }
            else
            {
              v19 = atan2((double)v17 / (double)v16, 1.0) * -1.0;
            }
          }
          else
          {
            v18 = atan2((double)v16 / (double)v17, 1.0);
            v19 = v17 >= 0 ? v18 + (*(const double *)D(0x8a6b28)) : v18 + (*(const double *)D(0x8a6618));
          }
          (*v32__f) = v19;
        }
        else
        {
          (*v32__f) = 0.0;
        }
        v21 = ((*v32__f) < (*(const double *)D(0x8a6b28)) || (*v32__f) >= (*(const double *)D(0x8a6b48))) && ((*v32__f) < (*(const double *)D(0x8a6618)) || (*v32__f) >= 0.0)
            ? (double)v16 / cos((*v32__f)) * -1.0
            : (double)v17 / sin((*v32__f));
        (*v30__f) = ftol_round_(v21);
        v22 = (double)(((int)(int64_t)((double)a4 * (*(const double *)D(0x8a9a38)) / (*(const double *)D(0x8a9a38)) + (*v32__f) / (*(const double *)D(0x8a9a38))) % 360 + 360) % 360)
            * (*(const double *)D(0x8a9a38));
        v23 = (double)(*v30__f);
        v24 = sin(v22) * v23;
        v25 = v24 >= 0.0 ? v24 + 0.5 : v24 - 0.5;
        apt[v15].x = a2 + (int64_t)v25;
        v26 = cos(v22) * v23;
        v27 = v26 >= 0.0 ? v26 + 0.5 : v26 - 0.5;
        apt[v15].y = a3 - (int)((int64_t)v27 * (a5 - 500)) / 500;
        v14 = (*v38__f);
        v15 = (uint16_t)++(*v33__f);
        if ( (uint16_t)(*v33__f) >= (*v38__f) )
          break;
        v12 = (*v34__f);
      }
      v8 = a8;
      v9 = (*v36__f);
    }
    API_Polyline((intptr_t)(hdc), (intptr_t)(apt), (intptr_t)(v14));
    goto LABEL_40;
  }
}

/* 0x490cd0: int __thiscall sub_490CD0(cls_490CD0 *this, int a2, _DWORD *a3) */
int32_t rd_490cd0(wl_char *self, int32_t a2, int32_t * a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t result = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t j = 0;
    intptr_t v17 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  if ( F16(c, 0x0) != 0 )
  {
    v4 = 0;
    v5 = 0;
    do
    {
      v6 = rd_417c40(((intptr_t)g_eng.sys), F16(c, 0x0), v4);
      result = W_rp_5b9a40((intptr_t)(v6), (intptr_t)(a2), (intptr_t)(v5), (intptr_t)(0));
      ++v4;
      v5 += 80;
    }
    while ( v4 < 8u );
  }
  if ( F16(c, 0x10) != 0 )
  {
    v9 = 250 * ((a3[13] + a3[24]) / 100) / 1000 / 80;
    v10 = -(250 * ((a3[13] + a3[24]) / 100) / 1000 % 80);
    v11 = F32(c, 0x18) * ((a3[19] + a3[23] + 100 * a3[29]) / -100) / 1000;
    if ( v11 < -28 )
    {
      v21 = -28;
    }
    else if ( v11 <= 0 )
    {
      v21 = F32(c, 0x18) * ((a3[19] + a3[23] + 100 * a3[29]) / -100) / 1000;
    }
    else
    {
      v21 = 0;
    }
    for ( (*i__f) = 9; (*i__f) != 0; --(*i__f) )
    {
      (*v18__f) = v21 + F16(c, 0x12);
      v12 = rd_417c40(((intptr_t)g_eng.sys), F16(c, 0x10), v9 % F16(c, 0x14));
      W_rp_5b9a40((intptr_t)(v12), (intptr_t)(a2), (intptr_t)(v10), (intptr_t)((*v18__f)));
      v10 += 80;
      ++v9;
      result = (*i__f) - 1;
    }
  }
  if ( F16(c, 0x4) != 0 )
  {
    v13 = -(500 * ((a3[13] + a3[24]) / 100) / 1000 % 80);
    v14 = F32(c, 0xc) * ((a3[19] + a3[23] + 100 * a3[29]) / -100) / 1000;
    if ( v14 < -28 )
    {
      v22 = -28;
    }
    else if ( v14 <= 0 )
    {
      v22 = F32(c, 0xc) * ((a3[19] + a3[23] + 100 * a3[29]) / -100) / 1000;
    }
    else
    {
      v22 = 0;
    }
    v15 = 500 * ((a3[13] + a3[24]) / 100) / 1000 / 80;
    for ( j = 9; j != 0; --j )
    {
      (*v19__f) = v22 + F16(c, 0x6);
      v17 = rd_417c40(((intptr_t)g_eng.sys), F16(c, 0x4), v15 % F16(c, 0x8));
      result = W_rp_5b9a40((intptr_t)(v17), (intptr_t)(a2), (intptr_t)(v13), (intptr_t)((*v19__f)));
      v13 += 80;
      ++v15;
    }
  }
  return result;
}

/* 0x417c40: int __thiscall game_arm_parallax_grade_417C40(_DWORD *this, unsigned __int16 a2, unsigned __int16 a3) */
int32_t rd_417c40(wl_char *self, uint16_t a2, uint16_t a3)
{
    intptr_t c = (intptr_t)self;
    int32_t * v3 = 0;
    int32_t * v4 = 0;
    intptr_t v5 = 0;
    ar_sprite_slot * v6 = 0;
    int32_t * v7 = 0;
    intptr_t v8 = 0;
    uint8_t * v9 = 0;
    uint16_t * v10 = 0;
    char * v11 = 0;
    uint16_t v12 = 0;
    uint8_t v13 = 0;
    intptr_t v14 = 0;
    uint8_t * v15 = 0;
    intptr_t v16 = 0;
    char * v17 = 0;
    intptr_t j = 0;
    char v19 = 0;
    char v20 = 0;
    char v21 = 0;
    uint8_t * v22 = 0;
    char * v23 = 0;
    char * v24 = 0;
    intptr_t k = 0;
    uint32_t v26 = 0;
    char v27 = 0;
    char v28 = 0;
    uint8_t * v29 = 0;
    char * v30 = 0;
    char * v31 = 0;
    intptr_t m = 0;
    char v33 = 0;
    char v34 = 0;
    char v35 = 0;
    uint8_t * v36 = 0;
    intptr_t v37 = 0;
    char * v38 = 0;
    intptr_t n = 0;
    uint32_t v40 = 0;
    char v41 = 0;
    char v42 = 0;
    uint8_t * v43 = 0;
    intptr_t v44 = 0;
    intptr_t jj = 0;
    uint8_t * v46 = 0;
    intptr_t v47 = 0;
    intptr_t ii = 0;
    uint8_t * v49 = 0;
    uint8_t * v50 = 0;
    uint8_t * v51 = 0;
    char * v52 = 0;
    intptr_t i = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    _Alignas(8) uint8_t frame_[0x800 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *out_palette = (intptr_t *)(frame_ + 0x0);   /* [256] at ebp-0x800 */
    uint8_t *v60 = (uint8_t *)(frame_ + 0x401);   /* [1023] at ebp-0x3ff */
    char *const v59__f = (char *)(frame_ + 0x400);   /* ebp-0x400 */

  v3 = c;
  v4 = 0;
  if ( *(((intptr_t *)c) + 2668) != 0 || *(((intptr_t *)c) + 2669) != 1000 )
    v4 = c + 4 * (2604);
  v5 = a2;
  v6 = (ar_sprite_slot *)*(((intptr_t *)c) + a2 + 683);
  if ( v6->entries->frames == 0 )
  {
    v7 = (_DWORD *)v3[a2 + 1592];
    v8 = v7[1];
    if ( v8 != 0 || v7[3] != 0 || v7[2] != 0 )
    {
      if ( v8 != 3 )
      {
        v9 = (uint8_t *)v7[3];
        if ( v9 == 0 )
        {
          if ( v7[2] != 0 )
          {
            W_fl_4178e0((intptr_t)(v6), (intptr_t)(out_palette));
            v10 = F32((v3[a2 + 1592] + 8), 0);
            if ( *v10 != 0 )
            {
              W_fl_4178e0((intptr_t)((ar_sprite_slot *)v3[(uint16_t)*v10 + 683]), (intptr_t)(&(*v59__f)));
              v11 = &(*v59__f);
            }
            else
            {
              v11 = (char *)out_palette;
            }
            v12 = 0;
            do
            {
              v13 = *((uint8_t *)v10 + v12 + 258);
              if ( v13 == 0 )
                break;
              v14 = *((uint8_t *)v10 + v12++ + 2);
              out_palette[v14] = F32(&v11[4 * v13], 0);
            }
            while ( v12 < 0x100u );
          }
          else
          {
            W_fl_4178e0((intptr_t)(v6), (intptr_t)(out_palette));
          }
          v9 = (uint8_t *)out_palette;
        }
        if ( F32(v3[a2 + 1592], 4) != 1 )
        {
          if ( F32(v3[a2 + 1592], 4) != 2 )
          {
LABEL_78:
            if ( v3[2668] != 0 || v3[2669] != 1000 )
            {
              v49 = (_BYTE *)(v60 - v9);
              v50 = v9 + 1;
              v51 = (_BYTE *)(&(*v59__f) - (char *)v9);
              v52 = &(*v59__f);
              for ( i = 256; i != 0; --i )
              {
                v52 += 4;
                v54 = (uint8_t)*(v50 - 1);
                v50 += 4;
                *(v52 - 4) = *((uint8_t *)v3 + v54 + 10416);
                v50[(int32_t)v49 - 4] = *((uint8_t *)v3 + (uint8_t)*(v50 - 3) + 10416);
                v51[(int32_t)v50 - 4] = *((uint8_t *)v3 + (uint8_t)*(v50 - 4) + 10416);
              }
              v9 = (uint8_t *)&(*v59__f);
            }
            v55 = v3[a2 + 683];
            if ( F32(F32(v55, 0), 4) == 0 )
              F32(F32(v55, 0), 4) = (intptr_t)g_eng.op_new(0x400u);
            qmemcpy(F32(F32(v55, 0), 4), v9, 0x400u);
            v5 = a2;
            return rd_418470(v3[v5 + 683], a3);
          }
          if ( v3[2599] == 3 )
          {
            v15 = v9 + 2;
            v16 = ((uint8_t *)out_palette - (uint8_t *)v9);
            v17 = (char *)out_palette;
            for ( j = 256; j != 0; --j )
            {
              if ( 90 * (unsigned int)*v15 / 0x64 <= 0xFF )
                v19 = 90 * (unsigned int)*v15 / 0x64;
              else
                v19 = -1;
              v15[v16] = v19;
              if ( 90 * (unsigned int)*(v15 - 1) / 0x64 <= 0xFF )
                v20 = 90 * (unsigned int)*(v15 - 1) / 0x64;
              else
                v20 = -1;
              v17[1] = v20;
              if ( 90 * (unsigned int)*(v15 - 2) / 0x64 <= 0xFF )
                v21 = 90 * (unsigned int)*(v15 - 2) / 0x64;
              else
                v21 = -1;
              *v17 = v21;
              v15 += 4;
              v17 += 4;
            }
            v3 = c;
            v9 = (uint8_t *)out_palette;
            goto LABEL_78;
          }
        }
        switch ( v3[2599] )
        {
          case 0:
            v22 = v9 + 2;
            v23 = (char *)(((uint8_t *)out_palette - (uint8_t *)v9));
            v24 = (char *)out_palette;
            for ( k = 256; k != 0; --k )
            {
              if ( 100 * (unsigned int)*v22 / 0x64 <= 0xFF )
                v26 = 100 * (unsigned int)*v22 / 0x64;
              else
                SETLB(v26, -1);
              v23[(int32_t)v22] = v26;
              if ( 100 * (unsigned int)*(v22 - 1) / 0x64 <= 0xFF )
                v27 = 100 * (unsigned int)*(v22 - 1) / 0x64;
              else
                v27 = -1;
              v24[1] = v27;
              if ( 100 * (unsigned int)*(v22 - 2) / 0x64 <= 0xFF )
                v28 = 100 * (unsigned int)*(v22 - 2) / 0x64;
              else
                v28 = -1;
              *v24 = v28;
              v22 += 4;
              v24 += 4;
            }
            v3 = c;
            v9 = (uint8_t *)out_palette;
            break;
          case 1:
            v29 = v9 + 2;
            v30 = (char *)(((uint8_t *)out_palette - (uint8_t *)v9));
            v31 = (char *)out_palette;
            for ( m = 256; m != 0; --m )
            {
              if ( 70 * (unsigned int)*v29 / 0x64 <= 0xFF )
                v33 = 70 * (unsigned int)*v29 / 0x64;
              else
                v33 = -1;
              v30[(int32_t)v29] = v33;
              if ( 80 * (unsigned int)*(v29 - 1) / 0x64 <= 0xFF )
                v34 = 80 * (unsigned int)*(v29 - 1) / 0x64;
              else
                v34 = -1;
              v31[1] = v34;
              if ( 100 * (unsigned int)*(v29 - 2) / 0x64 <= 0xFF )
                v35 = 100 * (unsigned int)*(v29 - 2) / 0x64;
              else
                v35 = -1;
              *v31 = v35;
              v29 += 4;
              v31 += 4;
            }
            v3 = c;
            v9 = (uint8_t *)out_palette;
            break;
          case 2:
          case 3:
            v36 = v9 + 2;
            v37 = ((uint8_t *)out_palette - (uint8_t *)v9);
            v38 = (char *)out_palette;
            for ( n = 256; n != 0; --n )
            {
              if ( 80 * (unsigned int)*v36 / 0x64 <= 0xFF )
                v40 = 80 * (unsigned int)*v36 / 0x64;
              else
                SETLB(v40, -1);
              v36[v37] = v40;
              if ( 70 * (unsigned int)*(v36 - 1) / 0x64 <= 0xFF )
                v41 = 70 * (unsigned int)*(v36 - 1) / 0x64;
              else
                v41 = -1;
              v38[1] = v41;
              if ( 70 * (unsigned int)*(v36 - 2) / 0x64 <= 0xFF )
                v42 = 70 * (unsigned int)*(v36 - 2) / 0x64;
              else
                v42 = -1;
              *v38 = v42;
              v36 += 4;
              v38 += 4;
            }
            v3 = c;
            v9 = (uint8_t *)out_palette;
            break;
          case 4:
            v46 = out_palette;
            v47 = v9 - (uint8_t *)out_palette;
            for ( ii = 256; ii != 0; --ii )
            {
              W_palette_tint((intptr_t)(v46), (intptr_t)(&v46[v47]), (intptr_t)(60), (intptr_t)(60), (intptr_t)(75), (intptr_t)(0));
              v46 += 4;
            }
            goto LABEL_77;
          case 5:
            v43 = out_palette;
            v44 = v9 - (uint8_t *)out_palette;
            for ( jj = 256; jj != 0; --jj )
            {
              W_palette_tint((intptr_t)(v43), (intptr_t)(&v43[v44]), (intptr_t)(80), (intptr_t)(80), (intptr_t)(85), (intptr_t)(0));
              v43 += 4;
            }
LABEL_77:
            v9 = (uint8_t *)out_palette;
            break;
          default:
            goto LABEL_78;
        }
        goto LABEL_78;
      }
      switch ( v3[2599] )
      {
        case 0:
          v6->f_08 = 1;
          v6->f_0c = 1000;
          v6->f_10 = 1000;
          v6->f_14 = 1000;
          goto LABEL_90;
        case 1:
          v6->f_08 = 1;
          v6->f_0c = 1000;
          v6->f_10 = 700;
          v6->f_14 = 600;
          goto LABEL_90;
        case 2:
        case 3:
        case 4:
        case 5:
          v6->f_08 = 1;
          v6->f_0c = 300;
          v6->f_10 = 300;
          v6->f_14 = 400;
LABEL_90:
          v6->f_18 = (uint32_t)v4;
          break;
        default:
          return rd_418470(v3[v5 + 683], a3);
      }
    }
  }
  return rd_418470(v3[v5 + 683], a3);
}

/* 0x490f30: int __thiscall sub_490F30(cls_490F30 *this, _DWORD *a2, int a3) */
int32_t rd_490f30(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    uint32_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t result = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    uint16_t * v19 = 0;
    uint16_t v20 = 0;
    uint8_t * v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    int32_t * v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    uint8_t * v27 = 0;
    intptr_t v28 = 0;
    intptr_t i = 0;
    uint8_t * v30 = 0;
    intptr_t v31 = 0;
    intptr_t j = 0;
    uint8_t * v33 = 0;
    intptr_t v34 = 0;
    intptr_t k = 0;
    uint8_t * v36 = 0;
    intptr_t v37 = 0;
    intptr_t m = 0;
    uint8_t * v39 = 0;
    intptr_t v40 = 0;
    intptr_t ii = 0;
    uint8_t * v42 = 0;
    intptr_t v43 = 0;
    intptr_t n = 0;
    uint8_t * v45 = 0;
    uint8_t * v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    uint8_t * v49 = 0;
    uint8_t * v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    int32_t * v55 = 0;
    int v56 = 0;
    uint16_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    intptr_t v65 = 0;
    intptr_t v66 = 0;
    int32_t * v67 = 0;
    int v68 = 0;
    _Alignas(8) uint8_t frame_[0x840 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *out_palette = (uint8_t *)(frame_ + 0x40);   /* [1024] at ebp-0x800 */
    uint8_t *v87 = (uint8_t *)(frame_ + 0x440);   /* [1024] at ebp-0x400 */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x840 */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x840 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x83c */
    intptr_t *const v72__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x838 */
    uint16_t * *const v73__f = (uint16_t * *)(frame_ + 0xc);   /* ebp-0x834 */
    intptr_t *const v74__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x830 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x828 */
    char * *const v77__f = (char * *)(frame_ + 0x1c);   /* ebp-0x824 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x820 */
    uint16_t *const v79__f = (uint16_t *)(frame_ + 0x24);   /* ebp-0x81c */
    intptr_t *const v80__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x81c */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x818 */
    intptr_t *const v82__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x814 */
    intptr_t *const v83__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x810 */
    intptr_t *const v84__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x808 */
    intptr_t *const v85__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x804 */

  v3 = c;
  v4 = a2[13] + a2[24];
  v5 = (v4 / 3200 - 1) & ((v4 / 3200 - 1 < 0) - 1);
  v6 = a2[25];
  (*v81__f) = F32(c, 4 * 721932) - v5;
  if ( (*v81__f) >= v6 / 3200 + 2 )
    (*v81__f) = v6 / 3200 + 2;
  v7 = (a2[19] + a2[23] + 100 * a2[29]) / 3200 - 1;
  v8 = v7 < 0 ? 0 : v7;
  v9 = (int)((uint64_t)(1374389535LL * (int)a2[26]) >> 32) >> 10;
  v10 = v8;
  v11 = *(_DWORD *)((uint8_t *)(v3 + (62776) * 0x2e) + 0x24);
  (*v85__f) = v10;
  v12 = v9 + (v9 >> 31) + 2;
  (*v78__f) = v11 - v10;
  if ( v11 - v10 >= v12 )
    (*v78__f) = v12;
  result = (*v78__f);
  v14 = 0;
  (*v82__f) = 0;
  if ( (*v78__f) <= 0 )
    return result;
  (*v71__f) = 3200 * v10;
  v15 = (*v81__f);
  (*v76__f) = (int)&GAP((v3 + (35618) * 0x2e), 0x0, 160 * v10 + 20 + v5);
  do
  {
    (*v74__f) = 0;
    if ( v15 <= 0 )
      goto LABEL_82;
    v16 = 5 * v5;
    v17 = v14 + (v5 << 7);
    (*v72__f) = 640 * v16;
    v18 = (int)&GAP((v3 + (1) * 0x2e), 0x0, 64 * v17 + 20 + 64 * v10);
    (*v77__f) = &GAP((v3 + (36063) * 0x2e), 0x26, 12 * v17 + 12 * v10);
    (*v73__f) = (uint16_t *)v18;
    do
    {
      v19 = (uint16_t *)(v18 - 18);
      (*v83__f) = 4;
      (*v69__f) = (v18 - 18);
      do
      {
        v20 = *v19;
        if ( *v19 == 0 )
          goto LABEL_60;
        v21 = 0;
        (*v79__f) = v19[1];
        if ( F32(D(0x8a9510), 0) != 0 || F32(D(0x8a9514), 0) != 1000 )
          v21 = ((uint8_t *)D(0x8a9410));
        v22 = v20;
        v23 = SYS32(4 * v20 + 2732);
        if ( **(_DWORD **)((uint8_t *)(v23) + 0x0) == 0 )
        {
          v24 = SYS32(4 * v20 + 6368);
          v25 = v24[1];
          if ( v25 != 0 || v24[3] != 0 || v24[2] != 0 )
          {
            if ( v25 == 3 )
            {
              switch ( F32(D(0x8a93fc), 0) )
              {
                case 0:
                  W_sprite_slot_tint((intptr_t)(v23), (intptr_t)(1000), (intptr_t)(1000), (intptr_t)(1000), (intptr_t)((int)v21));
                  break;
                case 1:
                  W_sprite_slot_tint((intptr_t)(v23), (intptr_t)(1000), (intptr_t)(700), (intptr_t)(600), (intptr_t)((int)v21));
                  break;
                case 2:
                case 3:
                case 4:
                case 5:
                  W_sprite_slot_tint((intptr_t)(v23), (intptr_t)(300), (intptr_t)(300), (intptr_t)(400), (intptr_t)((int)v21));
                  break;
                default:
                  goto LABEL_57;
              }
              goto LABEL_57;
            }
            if ( v24[3] != 0 )
            {
              v26 = v24[3];
            }
            else
            {
              if ( v24[2] != 0 )
              {
                W_fl_4178e0((intptr_t)((ar_sprite_slot *)v23), (intptr_t)(out_palette));
                W_pal_swap_apply((intptr_t)((ar_sprite_slot **)((intptr_t)g_eng.sys)), (intptr_t)(F32(SYS32(4 * v22 + 6368), 8)), (intptr_t)(out_palette));
              }
              else
              {
                W_fl_4178e0((intptr_t)((ar_sprite_slot *)v23), (intptr_t)(out_palette));
              }
              v26 = (int)out_palette;
            }
            if ( F32(SYS32(4 * v22 + 6368), 4) != 1 )
            {
              if ( F32(SYS32(4 * v22 + 6368), 4) != 2 )
              {
LABEL_47:
                if ( F32(D(0x8a9510), 0) != 0 || F32(D(0x8a9514), 0) != 1000 )
                {
                  v45 = (_BYTE *)(v26 + 1);
                  v46 = v87;
                  v47 = -v26;
                  v48 = 256;
                  v49 = &v87[v47 + 1];
                  v50 = &v87[v47];
                  do
                  {
                    v46 += 4;
                    v51 = (uint8_t)*(v45 - 1);
                    v45 += 4;
                    *(v46 - 4) = ((uint8_t *)D(0x8a9410))[v51];
                    v45[(int32_t)v49 - 4] = ((uint8_t *)D(0x8a9410))[(uint8_t)*(v45 - 3)];
                    --v48;
                    v50[(int32_t)v45 - 4] = ((uint8_t *)D(0x8a9410))[(uint8_t)*(v45 - 4)];
                  }
                  while ( v48 != 0 );
                  v26 = (int)v87;
                }
                W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)((uint8_t *)g_eng.sys + (4 * v22 + 2732))), (intptr_t)((void *)v26));
                v19 = (uint16_t *)(*v69__f);
                goto LABEL_57;
              }
              if ( F32(D(0x8a93fc), 0) == 3 )
              {
                v27 = out_palette;
                v28 = v26 - (int32_t)out_palette;
                for ( i = 256; i != 0; --i )
                {
                  W_palette_tint((intptr_t)(v27), (intptr_t)(&v27[v28]), (intptr_t)(90), (intptr_t)(90), (intptr_t)(90), (intptr_t)(0));
                  v27 += 4;
                }
LABEL_46:
                v26 = (int)out_palette;
                goto LABEL_47;
              }
            }
            switch ( F32(D(0x8a93fc), 0) )
            {
              case 0:
                v30 = out_palette;
                v31 = v26 - (int32_t)out_palette;
                for ( j = 256; j != 0; --j )
                {
                  W_palette_tint((intptr_t)(v30), (intptr_t)(&v30[v31]), (intptr_t)(100), (intptr_t)(100), (intptr_t)(100), (intptr_t)(0));
                  v30 += 4;
                }
                goto LABEL_46;
              case 1:
                v33 = out_palette;
                v34 = v26 - (int32_t)out_palette;
                for ( k = 256; k != 0; --k )
                {
                  W_palette_tint((intptr_t)(v33), (intptr_t)(&v33[v34]), (intptr_t)(100), (intptr_t)(80), (intptr_t)(70), (intptr_t)(0));
                  v33 += 4;
                }
                goto LABEL_46;
              case 2:
              case 3:
                v36 = out_palette;
                v37 = v26 - (int32_t)out_palette;
                for ( m = 256; m != 0; --m )
                {
                  W_palette_tint((intptr_t)(v36), (intptr_t)(&v36[v37]), (intptr_t)(70), (intptr_t)(70), (intptr_t)(80), (intptr_t)(0));
                  v36 += 4;
                }
                goto LABEL_46;
              case 4:
                v42 = out_palette;
                v43 = v26 - (int32_t)out_palette;
                for ( n = 256; n != 0; --n )
                {
                  W_palette_tint((intptr_t)(v42), (intptr_t)(&v42[v43]), (intptr_t)(60), (intptr_t)(60), (intptr_t)(75), (intptr_t)(0));
                  v42 += 4;
                }
                goto LABEL_46;
              case 5:
                v39 = out_palette;
                v40 = v26 - (int32_t)out_palette;
                for ( ii = 256; ii != 0; --ii )
                {
                  W_palette_tint((intptr_t)(v39), (intptr_t)(&v39[v40]), (intptr_t)(80), (intptr_t)(80), (intptr_t)(85), (intptr_t)(0));
                  v39 += 4;
                }
                goto LABEL_46;
              default:
                goto LABEL_47;
            }
          }
        }
LABEL_57:
        v52 = rd_418470(SYS32(4 * v22 + 2732), (*v79__f));
        v53 = 3200 * F32(v19, 4 * 3) / 100;
        v54 = 3200 * F32(v19, 4 * 2) / 100;
        if ( v52 != 0 )
        {
          v55 = rd_4917b0(a2, F32((*v69__f), 0x4), 3, v52, (*v72__f), (*v71__f), 0, 0, 0);
          if ( v55 != 0 )
          {
            v55[11] = v54;
            v55[12] = v53;
            v55[13] = 32;
            v55[14] = 32;
          }
        }
LABEL_60:
        v19 = (uint16_t *)((uint8_t *)((*v69__f)) + 0x10);
        v56 = (*v83__f) == 1;
        (*v69__f) = ((uint8_t *)(*v69__f) + 16);
        --(*v83__f);
      }
      while ( !v56 );
      if ( *((*v73__f) - 1) == 0 )
        goto LABEL_80;
      if ( *((int32_t *)(*v77__f) - 1) == 2 )
      {
        v57 = 7;
        (*v70__f) = SYS32(10104);
        goto LABEL_67;
      }
      if ( *((int32_t *)(*v77__f) - 1) == 3 )
      {
        v57 = 14;
        (*v70__f) = SYS32(4 * F16(c, 0x2c) + 10072);
LABEL_67:
        v58 = (int)(*v70__f);
        goto LABEL_68;
      }
      v57 = 0;
      v58 = SYS32(4 * F16(c, 0x2c) + 10072);
      (*v70__f) = v58;
LABEL_68:
      if ( *((int32_t *)(*v77__f) - 2) == 1 )
      {
        v64 = F16((*v77__f), 0) + 16;
        v65 = (*v71__f) + 100 * (32 - F16((*v77__f), 0));
        v66 = rd_417c40(
                ((intptr_t)g_eng.sys),
                0x61u,
                (((v4 / 3200 - 1) & ((v4 / 3200 - 1 < 0) - 1)) + (*v74__f) + F16(c, 0x24)) % 6 + v57 + 1);
        if ( v66 != 0 )
        {
          v67 = rd_4917b0(a2, 0x13u, 3, v66, (*v72__f), v65, 0, -16, v58);
          if ( v67 != 0 )
          {
            v67[13] = 32;
            v67[11] = 0;
            v67[12] = 0;
            v67[14] = v64;
          }
        }
      }
      else if ( *((int32_t *)(*v77__f) - 2) == 2 )
      {
        if ( F8(((*v76__f) + (*v74__f)), 0) != 0 )
        {
          v59 = rd_417c40(((intptr_t)g_eng.sys), 0x61u, v57);
          if ( v59 != 0 )
            rd_4917b0(a2, 0x13u, v58 != 0, v59, (*v72__f), (*v71__f), 0, 0, v58);
        }
        else
        {
          v60 = rd_417c40(((intptr_t)g_eng.sys), 0x61u, v57);
          (*v80__f) = rd_417c40(((intptr_t)g_eng.sys), *((*v73__f) - 1), *(*v73__f));
          v61 = 3200 * F32((*v73__f) + (5), 0) / 100;
          v62 = 3200 * F32((*v73__f) + (3), 0) / 100;
          v63 = rd_48c6b0(
                                SYS32(10068),
                                1001,
                                *((*v73__f) - 1),
                                *(*v73__f),
                                97,
                                v57,
                                v62 + (v61 << 10),
                                SYS32(10348),
                                32,
                                32,
                                0x1FFFFFF,
                                1,
                                &(*v84__f));
          if ( v63 != 0 )
          {
            if ( (*v84__f) == 0 )
            {
              W_rp_5b9bf0((intptr_t)((*v80__f)), (intptr_t)((int)v63), (intptr_t)(0), (intptr_t)(0), (intptr_t)(32), (intptr_t)(32), (intptr_t)(v62), (intptr_t)(v61));
              W_rp_5b9490((intptr_t)(v63));
              W_rp_5bd680((intptr_t)((*v70__f)), (intptr_t)((int **)v63), (intptr_t)((_DWORD **)v60), (intptr_t)(0), (intptr_t)(0), (intptr_t)(32), (intptr_t)(32), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v60, 40)));
              W_rp_5b94d0((intptr_t)(v63));
            }
            rd_4917b0(a2, 0x13u, 0, (int)v63, (*v72__f), (*v71__f), 0, 0, 0);
          }
        }
      }
LABEL_80:
      v15 = (*v81__f);
      v18 = (int)((*v73__f) + 4096);
      v68 = ++(*v74__f) < (*v81__f);
      (*v72__f) += 3200;
      (*v73__f) += 4096;
      (*v77__f) += 1536;
    }
    while ( v68 );
    v14 = (*v82__f);
    v10 = (*v85__f);
    v5 = (v4 / 3200 - 1) & ((v4 / 3200 - 1 < 0) - 1);
    v3 = c;
LABEL_82:
    ++v14;
    (*v71__f) += 3200;
    result = (*v78__f);
    (*v82__f) = v14;
    (*v76__f) += 160;
  }
  while ( v14 < (*v78__f) );
  return result;
}

/* 0x4917b0: _DWORD *__thiscall draw_pool_emit_layered_4917B0( cls_4917B0 *this, unsigned __int16 a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9) */
int32_t rd_4917b0(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9)
{
    intptr_t c = (intptr_t)self;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * result = 0;
    intptr_t v13 = 0;

  v9 = F32(c, 0x54);
  v10 = F16((v9 + 8 * a2), 0);
  v11 = v9 + 8 * a2;
  if ( v10 >= F16(v11, 2) )
    return 0;
  v13 = F32(v11, 4);
  F16(v11, 0) = v10 + 1;
  result = (_DWORD *)(v13 + 60 * v10);
  *result = a4;
  result[1] = a5;
  result[2] = a6;
  result[3] = a7;
  result[4] = a8;
  result[6] = a3;
  result[5] = a9;
  return result;
}

/* 0x491820: int __thiscall sub_491820( HDC *this, paint_ctx *a2, const char *a3, unsigned __int16 a4, int a5, int a6, int a7, int a8, int a9, cls_5BD550 *a10, int a11) */
int32_t rd_491820(wl_char *self, int32_t a2, char * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11)
{
    intptr_t c = (intptr_t)self;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint32_t v14 = 0;
    char v15 = 0;
    intptr_t v16 = 0;
    int16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    uint16_t * v20 = 0;
    uint16_t v21 = 0;
    intptr_t v22 = 0;
    int32_t * v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    uint16_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t i = 0;
    intptr_t v39 = 0;
    uint16_t * j = 0;
    _Alignas(8) uint8_t frame_[0x60 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v37 = (uint16_t *)(frame_ + 0x10);   /* [40] at ebp-0x50 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x60 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x5c */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x54 */

  v11 = 0;
  v12 = 0;
  (*v33__f) = 0;
  (*v36__f) = 0;
  (*v34__f) = 0;
  v13 = 0;
  while ( 1 )
  {
    v14 = strlen(a3) <= 0x28 ? strlen(a3) : 40;
    if ( (uint16_t)v13 >= v14 )
      break;
    v15 = a3[(uint16_t)v13];
    if ( v15 == 32 )
    {
      v16 = (uint16_t)v12++;
      ++v13;
      v37[v16] = 255;
    }
    else
    {
      if ( v15 >= 33 && v15 <= 122 )
      {
        v17 = v15;
        v18 = (uint16_t)v12++;
        v37[v18] = v17 - 33;
      }
      ++v13;
    }
  }
  v19 = a11;
  if ( a11 > 0 && a11 <= 2 )
  {
    if ( (uint16_t)v12 != 0 )
    {
      v20 = v37;
      for ( i = (uint16_t)v12; i != 0; --i )
      {
        v21 = *v20;
        if ( *v20 == 255 )
        {
          v24 = 0;
        }
        else
        {
          v22 = *(((intptr_t *)c) + a4 + 683);
          if ( **(_DWORD **)((uint8_t *)(v22) + 0x0) == 0 )
            W_fl_4184a0((intptr_t)(v22), (intptr_t)(0));
          v23 = F32((uint8_t *)(v22), 0x0);
          v11 = (*v34__f);
          v24 = F32((*v23 + 4 * v21), 0);
        }
        if ( a8 != 0 )
        {
          if ( v24 != 0 )
            v25 = F32(v24, 20);
          else
            v25 = a9;
          v11 += v25;
          (*v34__f) = v11;
        }
        ++v20;
      }
      v19 = a11;
    }
    if ( v19 == 2 )
      (*v34__f) = v11 / 2;
  }
  if ( (uint16_t)v12 != 0 )
  {
    v39 = (uint16_t)v12;
    for ( j = v37; ; ++j )
    {
      v26 = *j;
      if ( *j == 255 )
      {
        v28 = 0;
      }
      else
      {
        v27 = *(((intptr_t *)c) + a4 + 683);
        if ( **(_DWORD **)((uint8_t *)(v27) + 0x0) == 0 )
          W_fl_4184a0((intptr_t)(v27), (intptr_t)(0));
        v28 = F32((**(_DWORD **)((uint8_t *)(v27) + 0x0) + 4 * v26), 0);
        if ( v28 != 0 && a2 != 0 )
        {
          if ( a8 != 0 )
          {
            v29 = -F32(v28, 12);
            (*v36__f) = v29;
          }
          else
          {
            v29 = (*v36__f);
          }
          if ( a10 != 0 )
            W_rp_5bd550((intptr_t)(a10), (intptr_t)(a2), (intptr_t)(v28), (intptr_t)(a6 + (*v33__f) + v29 + F32(v28, 12) - (*v34__f)), (intptr_t)(a7 + F32(v28, 16)), (intptr_t)(F32(v28, 20)), (intptr_t)(F32(v28, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v28, 40)), (intptr_t)(*(((intptr_t *)c) + 216)));
          else
            W_rp_5b9b70((intptr_t)(v28), (intptr_t)((int)a2), (intptr_t)(a6 + (*v33__f) + v29 - (*v34__f)), (intptr_t)(a7));
        }
      }
      if ( a8 == 0 )
        break;
      if ( v28 == 0 )
      {
        v30 = a9 + (*v33__f);
LABEL_49:
        (*v33__f) = v30;
        goto LABEL_50;
      }
      (*v33__f) += F32(v28, 20);
LABEL_50:
      if ( v39-- == 1 )
        return (*v33__f);
    }
    v30 = a5 + (*v33__f);
    goto LABEL_49;
  }
  return (*v33__f);
}

/* 0x491ae0: int __thiscall actor_render_emit_491AE0(cls_4022A0 *this, unsigned __int16 a2, int a3) */
int32_t rd_491ae0(wl_char *self, uint16_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t result = 0;
    intptr_t v8 = 0;
    uint32_t v9 = 0;
    uint16_t v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    uint32_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    uint32_t v26 = 0;
    intptr_t v27 = 0;
    uint16_t v28 = 0;
    uint16_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    uint16_t v32 = 0;
    intptr_t v33 = 0;
    uint16_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    int32_t * v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    int16_t v49 = 0;
    uint16_t * v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    int v57 = 0;
    _Alignas(8) uint8_t frame_[0x1bc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v80 = (uint16_t *)(frame_ + 0x86);   /* [3] at ebp-0x136 */
    intptr_t *const v58__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1bc */
    intptr_t *const v59__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1b8 */
    intptr_t *const v61__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1b4 */
    intptr_t *const v60__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1b4 */
    intptr_t *const v64__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x1b0 */
    intptr_t *const v63__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x1b0 */
    intptr_t *const v62__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x1b0 */
    intptr_t *const v65__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x1ac */
    intptr_t *const v66__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x198 */
    intptr_t *const v67__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x194 */
    intptr_t *const v68__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x190 */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x18c */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x188 */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x184 */
    intptr_t *const v72__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x180 */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x17c */
    intptr_t *const v74__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x178 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x174 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x170 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x7c);   /* ebp-0x140 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x80);   /* ebp-0x13c */
    int16_t *const v79__f = (int16_t *)(frame_ + 0x84);   /* ebp-0x138 */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x8c);   /* ebp-0x130 */
    intptr_t *const v82__f = (intptr_t *)(frame_ + 0x90);   /* ebp-0x12c */
    intptr_t *const v83__f = (intptr_t *)(frame_ + 0x94);   /* ebp-0x128 */
    int16_t *const v84__f = (int16_t *)(frame_ + 0x98);   /* ebp-0x124 */
    int16_t *const v85__f = (int16_t *)(frame_ + 0x9a);   /* ebp-0x122 */
    intptr_t *const v86__f = (intptr_t *)(frame_ + 0x9c);   /* ebp-0x120 */
    intptr_t *const v87__f = (intptr_t *)(frame_ + 0xa0);   /* ebp-0x11c */
    intptr_t *const v88__f = (intptr_t *)(frame_ + 0xa4);   /* ebp-0x118 */
    intptr_t *const v89__f = (intptr_t *)(frame_ + 0xa8);   /* ebp-0x114 */
    int16_t *const v90__f = (int16_t *)(frame_ + 0xac);   /* ebp-0x110 */
    int16_t *const v91__f = (int16_t *)(frame_ + 0xae);   /* ebp-0x10e */
    intptr_t *const v92__f = (intptr_t *)(frame_ + 0xb0);   /* ebp-0x10c */
    intptr_t *const v93__f = (intptr_t *)(frame_ + 0xb4);   /* ebp-0x108 */
    intptr_t *const v94__f = (intptr_t *)(frame_ + 0xb8);   /* ebp-0x104 */
    intptr_t *const v95__f = (intptr_t *)(frame_ + 0xbc);   /* ebp-0x100 */
    int16_t *const v96__f = (int16_t *)(frame_ + 0xc0);   /* ebp-0xfc */
    int16_t *const v97__f = (int16_t *)(frame_ + 0xc2);   /* ebp-0xfa */
    intptr_t *const v98__f = (intptr_t *)(frame_ + 0xc4);   /* ebp-0xf8 */
    intptr_t *const v99__f = (intptr_t *)(frame_ + 0xc8);   /* ebp-0xf4 */
    intptr_t *const v100__f = (intptr_t *)(frame_ + 0xcc);   /* ebp-0xf0 */
    intptr_t *const v101__f = (intptr_t *)(frame_ + 0xd0);   /* ebp-0xec */
    int16_t *const v102__f = (int16_t *)(frame_ + 0xd4);   /* ebp-0xe8 */
    int16_t *const v103__f = (int16_t *)(frame_ + 0xd6);   /* ebp-0xe6 */
    intptr_t *const v104__f = (intptr_t *)(frame_ + 0xd8);   /* ebp-0xe4 */
    intptr_t *const v105__f = (intptr_t *)(frame_ + 0xdc);   /* ebp-0xe0 */

  v4 = F32(c, 0xfc);
  v5 = F32(c, 0x40);
  (*v72__f) = v4;
  v6 = v5 + 660 * a2;
  result = F32(c, 0x284);
  if ( result != 0 )
    return result;
  v8 = F32(v6, 644);
  if ( v8 != 0 )
  {
    v4 = F32(v8, 256);
    (*v72__f) = v4;
  }
  v9 = F32(c, 0x1d4);
  if ( v9 > 0x1131E )
  {
    if ( v9 <= 0x11350 )
    {
      if ( v9 == 70480 )
      {
        v25 = F32(c, 0x1f8);
        if ( v25 != 0 )
          rd_492c40(
            c,
            (_DWORD *)a3,
            (_DWORD **)0x15D,
            9,
            F32(v6, 4),
            F32(F32((v25 + 64), 0), 16) + F32(F32((v25 + 64), 0), 8) - 801,
            F32(v6, 8) - F32(F32((v25 + 64), 0), 16) - F32(F32((v25 + 64), 0), 8) + 801,
            96,
            96,
            5,
            F32(c, 0x30),
            0,
            0,
            1);
        goto LABEL_51;
      }
      v22 = v9 - 70460;
      if ( v22 != 0 )
      {
        if ( v22 != 10 )
        {
LABEL_51:
          result = W_entity_sprite_desc((intptr_t)(c), (intptr_t)(v6), (intptr_t)(&(*v77__f)));
          if ( result == 0 )
            return result;
          v11 = 1;
          goto LABEL_70;
        }
        if ( F32(c, 0x27c) != 0 || F32(c, 0x280) != 0 )
        {
          v23 = F32(c, 0x280);
          (*v66__f) = 1;
        }
        else
        {
          (*v66__f) = 0;
          v23 = 0;
        }
        rd_492c40(
          c,
          (_DWORD *)a3,
          (_DWORD **)0x156,
          37,
          F32(v6, 4) + 1600,
          v23,
          F32(v6, 8) - v23 - 6400,
          64,
          64,
          5,
          F32(c, 0x30),
          0,
          0,
          1);
        v24 = (*v66__f);
      }
      else
      {
        if ( F32(c, 0x27c) != 0 || F32(c, 0x280) != 0 )
        {
          v23 = F32(c, 0x280);
          (*v66__f) = 1;
        }
        else
        {
          (*v66__f) = 0;
          v23 = 0;
        }
        rd_492c40(
          c,
          (_DWORD *)a3,
          (_DWORD **)0x156,
          37,
          F32(v6, 4) + 1600,
          v23,
          F32(v6, 8) - v23 - 8000,
          64,
          64,
          5,
          F32(c, 0x30),
          0,
          0,
          1);
        v24 = (*v66__f);
      }
      if ( v24 != 0 )
        rd_4928f0(a3, 0x156u, 10, F32(v6, 4) + 1600, v23, 5u, 0, 0, 0, 0, 0, 0, 0, 0);
      goto LABEL_51;
    }
    v26 = v9 - 100141;
    if ( v26 == 0 )
    {
      SETLO(v42, F16(c, 0xe8));
      (*v79__f) = 373;
      (*v84__f) = 373;
      memset(v80, 0, (3 * sizeof(uint16_t)));
      result = (uint16_t)v42;
      (*v81__f) = 0;
      (*v77__f) = -256;
      (*v78__f) = 0;
      (*v85__f) = 1;
      (*v87__f) = 0;
      (*v82__f) = -128;
      (*v83__f) = 0;
      (*v86__f) = 0;
      if ( *(_WORD *)&GAP(c, 0x46, 20 * (uint16_t)v42 + 2) == 0 || F8(v6, 0) == 0 )
        return result;
      result = F32(v6, 108);
      if ( result != 0 )
      {
        v43 = F16(v6, 114);
        if ( F16(result + 2 * v43, 2) == 0xFFFF )
          return result;
        (*v66__f) = F32(result, 76);
        (*v88__f) = F32(result + 4 * v43, 80);
        (*v89__f) = F32(result + 4 * v43, 208);
        SETLO(v43, F16(result, 0) + F16(result + 2 * v43, 2));
        SETLO(result, F16(result, 336));
        (*v67__f) = v43;
        if ( (uint16_t)result != 0 )
          v42 = (uint16_t)result != 255 ? result : 0;
        v4 = (*v72__f);
      }
      else
      {
        (*v88__f) = 0;
        (*v89__f) = 0;
        (*v67__f) = 0;
        (*v66__f) = 0;
      }
      v44 = (uint16_t)v42;
      v45 = 5 * (uint16_t)v42;
      v46 = &F32(c, 0x0) + v45;
      SETLO(v45, F16(v46, 2 * 36));
      (*v70__f) = v45;
      v47 = SYS32(4 * (uint16_t)v45 + 6368);
      SETLO(v47, F16(v47, 0));
      (*v69__f) = v47;
      if ( (*v66__f) != 0 )
      {
        SETLO(v47, 0);
      }
      else if ( F32(c, 0xec) != 0 )
      {
        v47 = (uint16_t)((F32(v6, 52) + 360000) / (360000 / F16(c, 0xf0))) % (int)F16(c, 0xf0);
        if ( F32(v6, 44) == 3 )
          SETLO(v47, (*v69__f) + v47);
        v4 = (*v72__f);
      }
      else if ( F32(v6, 44) == 3 )
      {
        (*v88__f) = *(_DWORD *)&GAP(c, 0x4c, 20 * v44 + 4) - (*v88__f);
      }
      else
      {
        SETLO(v47, 0);
        (*v88__f) += v46[21];
      }
      v48 = v46[22];
      (*v73__f) = v4;
      (*v89__f) += v48;
      (*v90__f) = (*v70__f);
      v49 = v47 + F16(v46, 2 * 37);
      (*v93__f) = 0;
      (*v91__f) = (*v67__f) + v49;
      (*v74__f) = v4;
      v11 = 3;
      goto LABEL_70;
    }
    if ( v26 != 2 )
      goto LABEL_51;
    *(_DWORD *)F32(c, 0x2a8) = 2;
    F32(F32(c, 0x2a8), 4) = 2;
    F32(F32(c, 0x2a8), 20) = 160;
    F32(F32(c, 0x2a8), 24) = 160;
    F32(F32(c, 0x2a8), 16) = 750;
    v27 = 20 * F32(v6, 92) / 1600;
    if ( v27 >= 0 )
    {
      if ( v27 >= 20 )
      {
        (*v66__f) = SYS32(10148);
        goto LABEL_46;
      }
    }
    else
    {
      v27 = 0;
    }
    (*v66__f) = SYS32(4 * v27 + 10072);
LABEL_46:
    v28 = a2;
    v29 = a2;
    if ( a2 >= F16(c, 0x44) )
    {
      v29 = 0;
      v28 = 0;
    }
    v30 = F32(c, 0x40);
    (*v68__f) = F32(v30 + 660 * v28, 4);
    (*v69__f) = F32(c, 0x2a8);
    v31 = F32(v30 + 660 * v29, 8) + F32(v30 + 660 * v29, 20);
    v32 = F32(c, 0xfc);
    v33 = F32(a3, 84);
    v34 = F16((v33 + 8 * v32), 0);
    v35 = v33 + 8 * v32;
    if ( v34 < F16(v35, 2) )
    {
      v36 = F32(v35, 4);
      v37 = v34;
      F16(v35, 0) = v34 + 1;
      v38 = (*v68__f);
      v39 = v36 + 60 * v37;
      F32(v39, 0) = 0;
      F32(v39, 4) = v38;
      F32(v39, 8) = v31;
      F32(v39, 12) = 0;
      F32(v39, 16) = 0;
      F32(v39, 24) = 2;
      F32(v39, 20) = 0;
      if ( v39 != 0 )
      {
        F32(v39, 32) = 0;
        v40 = (*v69__f);
        F16(v39, 28) = 1;
        F16(v39, 30) = 1;
        v41 = (*v66__f);
        F16(v39, 36) = 13;
        F32(v39, 40) = v40;
        F32(v39, 20) = v41;
      }
    }
    goto LABEL_51;
  }
  if ( v9 == 70430 )
  {
    if ( F32(c, 0x27c) != 0 || F32(c, 0x280) != 0 )
      v21 = F32(c, 0x280);
    else
      v21 = 0;
    rd_492c40(
      c,
      (_DWORD *)a3,
      (_DWORD **)0x156,
      17,
      F32(v6, 4) + 3200,
      v21,
      F32(v6, 8) - v21,
      64,
      64,
      5,
      F32(c, 0x30),
      0,
      0,
      1);
    goto LABEL_51;
  }
  switch ( v9 )
  {
    case 0x11256u:
      v12 = F32(c, 0x30);
      v13 = F32(v6, 8);
      v14 = F32(v6, 4);
      (*v69__f) = F32(c, 0x34);
      (*v68__f) = v12;
      v13 -= 3200;
      rd_4925e0(SYS32(4100), 0xAu, &(*v67__f), &(*v66__f), &(*v71__f), &(*v70__f));
      rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v14, v13, (*v67__f), (*v66__f), (*v71__f), (*v70__f));
      (*v62__f) = (*v69__f);
      (*v60__f) = (*v68__f);
      v15 = rd_417c40(((intptr_t)g_eng.sys), 0x156u, 0xAu);
      rd_492670(a3, 0xFu, v15, v14, v13, (*v60__f), (*v62__f), 0);
      v16 = F32(c, 0x30);
      v17 = F32(v6, 8);
      v18 = F32(v6, 4);
      (*v66__f) = F32(c, 0x34);
      (*v67__f) = v16;
      v17 += 6400;
      rd_4925e0(SYS32(4100), 0xBu, &(*v71__f), &(*v70__f), &(*v69__f), &(*v68__f));
      rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v18, v17, (*v71__f), (*v70__f), (*v69__f), (*v68__f));
      (*v63__f) = (*v66__f);
      (*v61__f) = (*v67__f);
      v19 = rd_417c40(((intptr_t)g_eng.sys), 0x156u, 0xBu);
      rd_492670(a3, 0xFu, v19, v18, v17, (*v61__f), (*v63__f), 0);
      goto LABEL_51;
    case 0x11260u:
      rd_492a90(c, v5 + 660 * a2, (_DWORD *)a3, 0, 2400);
      goto LABEL_51;
    case 0x112ECu:
      v10 = F16(c, 0x48);
      v80[0] = F16(c, 0x4a);
      (*v79__f) = v10;
      (*v84__f) = v10;
      (*v81__f) = 0;
      (*v77__f) = 0;
      (*v78__f) = 0;
      F32(&v80[1], 0) = 0;
      (*v85__f) = v80[0] + 1;
      (*v87__f) = 0;
      (*v82__f) = 0;
      (*v83__f) = 0;
      (*v86__f) = 0;
      (*v73__f) = 10;
      v11 = 2;
      break;
    case 0x112F6u:
      result = W_entity_sprite_desc((intptr_t)(c), (intptr_t)(v6), (intptr_t)(&(*v77__f)));
      if ( result == 0 )
        return result;
      (*v87__f) = 0;
      (*v82__f) = 256;
      (*v83__f) = 0;
      v80[0] *= 4;
      (*v84__f) = (*v79__f);
      (*v90__f) = (*v79__f);
      (*v96__f) = (*v79__f);
      (*v85__f) = v80[0] + 1;
      (*v91__f) = v80[0] + 2;
      (*v100__f) = 224;
      (*v101__f) = 224;
      (*v86__f) = 0;
      (*v93__f) = 0;
      (*v88__f) = 0;
      (*v89__f) = 256;
      (*v92__f) = 0;
      (*v97__f) = v80[0] + 3;
      (*v99__f) = 0;
      (*v94__f) = 256;
      (*v95__f) = 256;
      (*v98__f) = 0;
      (*v102__f) = 342;
      (*v103__f) = 36;
      (*v105__f) = 0;
      (*v104__f) = 0;
      (*v73__f) = v4;
      (*v74__f) = v4;
      (*v75__f) = v4;
      (*v76__f) = v4;
      v11 = 5;
      break;
    case 0x11314u:
      if ( F32(c, 0x27c) != 0 || F32(c, 0x280) != 0 )
      {
        v20 = F32(c, 0x280);
        (*v66__f) = 1;
      }
      else
      {
        (*v66__f) = 0;
        v20 = F32(*(_DWORD *)&GAP(W, 0x1030, 24), 2887740);
      }
      rd_492c40(
        c,
        (_DWORD *)a3,
        (_DWORD **)0x156,
        17,
        F32(v6, 4),
        F32(v6, 8) + 600,
        v20 - F32(v6, 8) - 600,
        64,
        64,
        5,
        F32(c, 0x30),
        0,
        0,
        0);
      if ( (*v66__f) != 0 )
        rd_4928f0(a3, 0x156u, 11, F32(v6, 4), v20 - 6400, 5u, 0, 0, 0, 0, 0, 0, 0, 0);
      goto LABEL_51;
    default:
      goto LABEL_51;
  }
LABEL_70:
  (*v67__f) = (int)&(*v72__f);
  v50 = v80;
  (*v70__f) = v11;
  do
  {
    v51 = F32((v50 - 5), 0);
    (*v66__f) = F32(c, 0xf4);
    v52 = F32((*v67__f), 0);
    v53 = F32(v6, 4);
    (*v71__f) = F32(v6, 8);
    v54 = F32(v6, 64) + v51;
    v55 = F32(v6, 68) + F32((v50 - 3), 0);
    (*v68__f) = v53;
    (*v69__f) = v55;
    if ( v52 < 19 )
    {
      rd_4927c0(*(v50 - 1), *v50, v53, (*v71__f), v54, v55);
      v53 = (*v68__f);
      v55 = (*v69__f);
    }
    if ( (*v66__f) == 0 )
      (*v66__f) = F32(v50 + (3), 0);
    (*v65__f) = (*v66__f);
    (*v64__f) = v55;
    (*v59__f) = (*v71__f);
    (*v58__f) = v53;
    v56 = rd_417c40(((intptr_t)g_eng.sys), *(v50 - 1), *v50);
    rd_492670(a3, F32((*v67__f), 0), v56, (*v58__f), (*v59__f), v54, (*v64__f), (*v65__f));
    v50 += 10;
    result = (*v70__f) - 1;
    v57 = (*v70__f) == 1;
    (*v67__f) += 4;
    --(*v70__f);
  }
  while ( !v57 );
  return result;
}

/* 0x4925e0: _DWORD *__thiscall sub_4925E0(cls_4925E0 *this, unsigned __int16 a2, _DWORD *a3, _DWORD *a4, _DWORD *a5, _DWORD *a6) */
int32_t rd_4925e0(wl_char *self, uint16_t a2, int32_t * a3, int32_t * a4, int32_t * a5, int32_t * a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t v7 = 0;

  v6 = F32(c, 0x34);
  if ( v6 != 0 )
  {
    v7 = 24 * a2;
    *a3 = F32((v7 + v6), 0);
    *a4 = F32(v7 + F32(c, 0x34), 8);
    *a5 = F32(v7 + F32(c, 0x34), 4) - F32((v7 + F32(c, 0x34)), 0);
    *a6 = F32(F32(c, 0x34) + v7, 12) - F32(F32(c, 0x34) + v7, 8);
    return a6;
  }
  else
  {
    *a3 = 0;
    *a4 = 0;
    *a5 = F32(c, 0x20);
    *a6 = F32(c, 0x24);
    return a5;
  }
}

/* 0x492670: _DWORD *__thiscall draw_pool_emit_actor_492670( cls_492670 *this, unsigned __int16 a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t rd_492670(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    int32_t * result = 0;
    int32_t * v9 = 0;
    uint16_t v10 = 0;
    intptr_t v11 = 0;

  if ( a3 != 0 )
  {
    result = (_DWORD *)F32(c, 0x54);
    v9 = &result[2 * a2];
    v10 = F16(v9, 0);
    if ( F16(v9, 0) < F16(v9, 2 * 1) )
    {
      v11 = v9[1];
      F16(v9, 0) = v10 + 1;
      result = (_DWORD *)(v11 + 60 * v10);
      result[1] = a4;
      result[2] = a5;
      *result = a3;
      result[3] = a6;
      result[4] = a7;
      result[6] = a8 != 0;
      result[5] = a8;
    }
  }
  return result;
}

/* 0x4926f0: int __thiscall sub_4926F0(cls_4926F0 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t rd_4926f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t result = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;

  v7 = a4 + a2 / 100;
  v14 = v7;
  v8 = a5 + a3 / 100;
  result = a7 + v8 + 31;
  v13 = result;
  if ( v8 < result )
  {
    v10 = v7 + a6 + 31;
    do
    {
      if ( v7 < v10 )
      {
        v11 = v8 / 32;
        do
        {
          v12 = v7 / 32;
          if ( v7 / 32 >= 0
            && v12 < F32(c, 4 * 721932)
            && v11 >= 0
            && v11 < F32(c, 4 * 721933)
            && F32(c, 4 * (384 * v12 + 3 * v11 + 414732)) != 0 )
          {
            *((uint8_t *)c + 160 * v11 + v12 + 1638448) = 1;
          }
          v7 += 32;
        }
        while ( v7 < v10 );
        result = v13;
        v7 = v14;
      }
      v8 += 32;
    }
    while ( v8 < result );
  }
  return result;
}

/* 0x4927c0: signed int __stdcall sub_4927C0(unsigned __int16 a1, unsigned __int16 a2, int a3, int a4, int a5, int a6) */
int32_t rd_4927c0(uint16_t a1, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{

    int32_t * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t result = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;

  v6 = SYS32(4 * a1 + 2732);
  v7 = v6[13];
  if ( v7 != 0 )
  {
    v8 = F32((v7 + 24 * a2), 0);
    v9 = F32(v7 + 24 * a2, 8);
    v10 = F32(v7 + 24 * a2, 4) - v8;
    v11 = F32(v7 + 24 * a2, 12) - v9;
  }
  else
  {
    v10 = v6[8];
    v11 = v6[9];
    v8 = 0;
    v9 = 0;
  }
  v12 = *(_DWORD *)&GAP(W, 0x1030, 24);
  v20 = a5 + v8 + a3 / 100;
  result = (unsigned int)((uint64_t)(1374389535LL * a4) >> 32) >> 31;
  v14 = a6 + v9 + a4 / 100;
  v15 = v14 + v11 + 31;
  if ( v14 < v15 )
  {
    result = v20;
    v19 = v20 + v10 + 31;
    do
    {
      v16 = result;
      if ( result < v19 )
      {
        v17 = v14 / 32;
        do
        {
          v18 = v16 / 32;
          if ( v16 / 32 >= 0
            && v18 < F32(v12, 2887728)
            && v17 >= 0
            && v17 < F32(v12, 2887732)
            && F32((v12 + 12 * ((v18 << 7) + v17 + 138244)), 0) != 0 )
          {
            F8(v18 + 160 * v17 + v12, 1638448) = 1;
          }
          v16 += 32;
        }
        while ( v16 < v19 );
        result = v20;
      }
      v14 += 32;
    }
    while ( v14 < v15 );
  }
  return result;
}

/* 0x4928f0: int __stdcall sub_4928F0( int a1, unsigned __int16 a2, unsigned __int16 a3, int a4, int a5, unsigned __int16 a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14) */
int32_t rd_4928f0(int32_t a1, uint16_t a2, uint16_t a3, int32_t a4, int32_t a5, uint16_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14)
{

    int32_t * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int32_t * v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t result = 0;
    intptr_t v22 = 0;
    uint16_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    int32_t * v26 = 0;
    intptr_t v28 = 0;
    uint16_t v29 = 0;
    intptr_t v30 = 0;
    int32_t * v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;

  v14 = SYS32(4 * a2 + 2732);
  v15 = v14[13];
  if ( v15 != 0 )
  {
    v16 = F32(v15 + 24 * a3, 8);
    v17 = (int *)(v15 + 24 * a3);
    v18 = *v17;
    v19 = v17[1] - *v17;
    v20 = v17[3] - v16;
  }
  else
  {
    v19 = v14[8];
    v20 = v14[9];
    v18 = 0;
    v16 = 0;
  }
  rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), a4, a5, v18, v16, v19, v20);
  if ( a10 != 0 )
  {
    result = rd_417c40(((intptr_t)g_eng.sys), a2, a3);
    if ( result != 0 )
    {
      v22 = F32(a1, 84);
      v23 = F16((v22 + 8 * a6), 0);
      v24 = v22 + 8 * a6;
      if ( v23 < F16(v24, 2) )
      {
        v25 = F32(v24, 4);
        F16(v24, 0) = v23 + 1;
        v26 = (_DWORD *)(v25 + 60 * v23);
        *v26 = result;
        result = a8;
        v26[3] = a7;
        v26[1] = a4;
        v26[2] = a5;
        v26[4] = a8;
        v26[6] = 3;
        v26[5] = a9;
        if ( v26 != 0 )
        {
          v26[11] = a11;
          v26[12] = a12;
          v26[13] = a13;
          v26[14] = a14;
          return a13;
        }
      }
    }
  }
  else
  {
    result = rd_417c40(((intptr_t)g_eng.sys), a2, a3);
    v32 = result;
    if ( result != 0 )
    {
      result = a1;
      v33 = a9 != 0;
      v28 = F32(a1, 84);
      v29 = F16((v28 + 8 * a6), 0);
      v30 = v28 + 8 * a6;
      if ( v29 < F16(v30, 2) )
      {
        F16(v30, 0) = v29 + 1;
        v31 = (int *)(F32(v30, 4) + 60 * v29);
        *v31 = v32;
        v31[3] = a7;
        v31[1] = a4;
        v31[2] = a5;
        v31[4] = a8;
        v31[6] = v33;
        v31[5] = a9;
        return v33;
      }
    }
  }
  return result;
}

/* 0x492a90: _DWORD *__thiscall sub_492A90(cls_492A90 *this, int a2, _DWORD *a3, int a4, int a5) */
int32_t rd_492a90(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * result = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x4 */

  v5 = 0;
  if ( F32(c, 0x27c) != 0 || F32(c, 0x280) != 0 )
  {
    v6 = F32(c, 0x280);
    (*v17__f) = 1;
    (*v15__f) = v6;
  }
  else
  {
    (*v17__f) = 0;
    (*v15__f) = F32(*(_DWORD *)&GAP(W, 0x1030, 24), 2887740);
  }
  (*v16__f) = F32(c, 0x30);
  v7 = F32(a2, 8);
  v8 = F32(a2, 4);
  v9 = (*v15__f) - v7 - a5;
  a5 += v7;
  v10 = a4 + v8;
  if ( v9 > 0 )
  {
    if ( v9 - 6400 >= 0 )
    {
      do
      {
        rd_4928f0((int)a3, 0x156u, 0x11u, v10, v5 + a5, 5u, (*v16__f), 0, 0, 0, 0, 0, 0, 0);
        v5 += 6400;
      }
      while ( v5 <= v9 - 6400 );
    }
    v11 = v9 / 100 % 64;
    if ( v11 != 0 )
      rd_4928f0((int)a3, 0x156u, 0x11u, v10, a5 + v5, 5u, (*v16__f), 0, 0, 1, 0, 0, 64, v11);
  }
  result = (_DWORD *)(*v17__f);
  if ( (*v17__f) != 0 )
  {
    v13 = a4 + F32(a2, 4);
    rd_4925e0(SYS32(4100), 0xBu, &(*v17__f), &a5, &a4, &a2);
    rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v13, (*v15__f) - 6400, (*v17__f), a5, a4, a2);
    v14 = rd_417c40(((intptr_t)g_eng.sys), 0x156u, 0xBu);
    return rd_492670(a3, 5u, v14, v13, (*v15__f) - 6400, 0, 0, 0);
  }
  return result;
}

/* 0x492c40: void __thiscall sub_492C40( cls_492C40 *this, _DWORD *a2, _DWORD **a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14) */
void rd_492c40(wl_char *self, int32_t * a2, int32_t ** a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13, int32_t a14)
{
    intptr_t c = (intptr_t)self;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    uint16_t v16 = 0;
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    _Alignas(8) uint8_t frame_[0x44 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x44 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x40 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x38 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x38 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x38 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x34 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x34 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x34 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x30 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x30 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x30 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2c */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2c */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x2c */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x18 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x14 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x10 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0xc */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x8 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x4 */

  (*v43__f) = c;
  v14 = a7;
  if ( a7 <= 0 )
    return;
  v15 = a5;
  v16 = (uint16_t)a3;
  v17 = a4;
  if ( a14 != 0 )
  {
    v18 = a7 - 100 * a9;
    a4 = 100 * a9;
    if ( v18 < 0 )
      goto LABEL_11;
    a3 = (_DWORD **)(4 * (uint16_t)a3 + 9074188);
    a5 = v18 + a6;
    do
    {
      rd_4925e0(*a3, v17, &(*v41__f), &(*v40__f), &(*v39__f), &(*v38__f));
      rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v15, a5, (*v41__f), (*v40__f), (*v39__f), (*v38__f));
      (*v35__f) = a13;
      (*v32__f) = a12;
      (*v29__f) = a11;
      (*v26__f) = a5;
      v19 = rd_417c40(((intptr_t)g_eng.sys), v16, v17);
      rd_492670(a2, a10, v19, v15, (*v26__f), (*v29__f), (*v32__f), (*v35__f));
      v18 -= a4;
      a5 -= a4;
    }
    while ( v18 >= 0 );
  }
  else
  {
    v18 = 0;
    a4 = 100 * a9;
    (*v42__f) = a7 - 100 * a9;
    if ( (*v42__f) < 0 )
      goto LABEL_11;
    a5 = a6;
    a3 = (_DWORD **)(4 * (uint16_t)a3 + 9074188);
    do
    {
      rd_4925e0(*a3, v17, &(*v38__f), &(*v39__f), &(*v40__f), &(*v41__f));
      rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v15, a5, (*v38__f), (*v39__f), (*v40__f), (*v41__f));
      (*v36__f) = a13;
      (*v33__f) = a12;
      (*v30__f) = a11;
      (*v27__f) = a5;
      v20 = rd_417c40(((intptr_t)g_eng.sys), v16, v17);
      rd_492670(a2, a10, v20, v15, (*v27__f), (*v30__f), (*v33__f), (*v36__f));
      v18 += a4;
      a5 += a4;
    }
    while ( v18 <= (*v42__f) );
  }
  v14 = a7;
LABEL_11:
  v21 = v14 / 100 % a9;
  if ( v21 != 0 )
  {
    if ( a14 != 0 )
    {
      a4 = v21 + 1;
      rd_4925e0(SYS32(4 * v16 + 2732), v17, &a3, &a5, &a14, &a7);
      v22 = a6;
      rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v15, a6, (int)a3, a5, a14, a7);
      (*v37__f) = a13;
      (*v34__f) = a4;
      (*v31__f) = a8;
      (*v28__f) = a9 - a4;
      (*v25__f) = a12;
      (*v24__f) = a11;
      v23 = rd_417c40(((intptr_t)g_eng.sys), v16, v17);
      rd_492f30(a2, a10, v23, v15, v22, (*v24__f), (*v25__f), 0, (*v28__f), (*v31__f), (*v34__f), (*v37__f));
    }
    else
    {
      rd_4928f0((int)a2, v16, v17, v15, a6 + v18, a10, a11, a12, a13, 1, 0, 0, a8, v14 / 100 % a9);
    }
  }
}

/* 0x492f30: void __thiscall sub_492F30( cls_492F30 *this, unsigned __int16 a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12) */
void rd_492f30(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12)
{
    intptr_t c = (intptr_t)self;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    int32_t * v16 = 0;

  if ( a3 != 0 )
  {
    v12 = F32(c, 0x54);
    v13 = F16((v12 + 8 * a2), 0);
    v14 = v12 + 8 * a2;
    if ( v13 < F16(v14, 2) )
    {
      v15 = F32(v14, 4);
      F16(v14, 0) = v13 + 1;
      v16 = (int *)(v15 + 60 * v13);
      v16[1] = a4;
      v16[2] = a5;
      v16[3] = a6;
      *v16 = a3;
      v16[4] = a7;
      v16[6] = 3;
      v16[5] = a12;
      if ( v16 != 0 )
      {
        v16[11] = a8;
        v16[12] = a9;
        v16[13] = a10;
        v16[14] = a11;
      }
    }
  }
}

/* 0x492fc0: int __userpurge band_23e0_render_492FC0@<eax>(int a1@<edx>, _DWORD *a2@<ecx>, int a3@<ebx>, cls_4917B0 *a4) */
int32_t rd_492fc0(int32_t a1, int32_t * a2, int32_t a3, int32_t a4)
{

    int32_t * v4 = 0;
    intptr_t v5 = 0;
    intptr_t result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int16_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    _Alignas(8) uint8_t frame_[0x34 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v34 = (uint16_t *)(frame_ + 0x2a);   /* [5] at ebp-0xa */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x30 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x2c */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x28 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x24 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x20 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x1c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x18 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x14 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x10 */

  v4 = a2;
  (*v27__f) = (int)a2;
  v5 = a2[16];
  (*v24__f) = a2[63];
  result = a2[129];
  if ( result != 2 )
  {
    v7 = F32(v5, 644);
    v8 = 0;
    if ( v7 != 0 )
    {
      a1 = F32(v7, 256);
      (*v24__f) = a1;
    }
    SETLO(v9, F16(v4, 2 * 116));
    result = 5 * (uint16_t)v9;
    if ( LOWORD(v4[5 * (uint16_t)v9 + 18]) != 0 && F8(v5, 0) != 0 )
    {
      result = F32(v5, 108);
      if ( result != 0 )
      {
        v10 = F16(v5, 114);
        SETLO(a1, F16(result + 2 * v10, 2));
        if ( (uint16_t)a1 == 0xFFFF )
          return result;
        SETLO(a1, F16(result, 0) + a1);
        (*v25__f) = F32(result, 76);
        a3 = F32(result + 4 * v10, 80);
        v8 = F32(result + 4 * v10, 208);
        SETLO(result, F16(result, 336));
        (*v32__f) = a3;
        (*v33__f) = v8;
        (*v28__f) = a1;
        if ( (uint16_t)result != 0 )
          v9 = (uint16_t)result != 255 ? result : 0;
      }
      else
      {
        (*v32__f) = 0;
        (*v33__f) = 0;
        (*v28__f) = 0;
        (*v25__f) = 0;
      }
      v11 = (uint16_t)v9;
      SETLO(a3, v4[5 * (uint16_t)v9 + 18]);
      v12 = &v4[5 * (uint16_t)v9];
      (*v31__f) = a3;
      v13 = 4 * (uint16_t)a3;
      SETLO((*v26__f), (*(uint16_t *)SYS32(v13 + 6368)));
      if ( (*v25__f) != 0 )
      {
        SETLO(v14, 0);
      }
      else if ( v4[59] != 0 )
      {
        v14 = (uint16_t)((F32(v5, 52) + 360000) / (360000 / *((uint16_t *)v4 + 120)))
            % (int)*((uint16_t *)v4 + 120);
        if ( F32(v5, 44) == 3 )
          SETLO(v14, (*v26__f) + v14);
        v8 = (*v33__f);
        v4 = (_DWORD *)(*v27__f);
      }
      else if ( F32(v5, 44) == 3 )
      {
        (*v32__f) = v4[5 * v11 + 20] - (*v32__f);
        SETLO(v14, (*v26__f));
      }
      else
      {
        (*v32__f) += v12[21];
        SETLO(v14, 0);
      }
      v15 = v4[61];
      v16 = v12[22] + v8;
      v17 = v14 + F16(v12, 2 * 37);
      v18 = F32(v5, 64);
      v19 = F32(v5, 4);
      v34[0] = (*v28__f) + v17;
      v20 = F32(v5, 8);
      v21 = F32(v5, 68);
      (*v28__f) = v20;
      v22 = (*v32__f) + v18;
      v23 = v16 + v21;
      (*v27__f) = v19;
      if ( (*v24__f) < 19 )
      {
        rd_4925e0(SYS32(v13 + 2732), v34[0], &(*v30__f), &(*v29__f), &(*v25__f), &(*v26__f));
        rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), (*v27__f), (*v28__f), v22 + (*v30__f), v23 + (*v29__f), (*v25__f), (*v26__f));
      }
      result = rd_417c40(((intptr_t)g_eng.sys), (*v31__f), v34[0]);
      if ( result != 0 )
        return (int)rd_4917b0(a4, (*v24__f), v15 != 0, result, (*v27__f), (*v28__f), v22, v23, v15);
    }
  }
  return result;
}

/* 0x493230: int __fastcall band_2560_render_493230(int a1, int a2, cls_4917B0 *a3) */
int32_t rd_493230(int32_t a1, int32_t a2, int32_t a3)
{

    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    int16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    uint16_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x34 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v29 = (uint16_t *)(frame_ + 0x1c);   /* [2] at ebp-0x18 */
    uint16_t *v32 = (uint16_t *)(frame_ + 0x2a);   /* [5] at ebp-0xa */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x34 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x30 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x2c */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x28 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x24 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x20 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x1c */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x14 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x10 */

  SETLO(v3, F16(a1, 232));
  v4 = 0;
  v5 = F32(a1, 64);
  result = 5 * (uint16_t)v3;
  if ( F16(a1 + 20 * (uint16_t)v3, 72) != 0 && F8(v5, 0) != 0 )
  {
    result = F32(v5, 108);
    if ( result != 0 )
    {
      v7 = F16(v5, 114);
      SETLO(a2, F16(result + 2 * v7, 2));
      if ( (uint16_t)a2 == 0xFFFF )
        return result;
      SETLO(a2, F16(result, 0) + a2);
      (*v22__f) = F32(result, 76);
      (*v30__f) = F32(result + 4 * v7, 80);
      v4 = F32(result + 4 * v7, 208);
      SETLO(result, F16(result, 336));
      (*v31__f) = v4;
      (*v25__f) = a2;
      if ( (uint16_t)result != 0 )
        v3 = (uint16_t)result != 255 ? result : 0;
    }
    else
    {
      (*v30__f) = 0;
      (*v31__f) = 0;
      (*v25__f) = 0;
      (*v22__f) = 0;
    }
    v8 = 5 * (uint16_t)v3;
    v9 = a1 + 20 * (uint16_t)v3;
    SETLO(v8, F16(v9, 72));
    (*v28__f) = v8;
    v10 = 4 * (uint16_t)v8;
    (*v24__f) = v10;
    SETLO((*v23__f), (*(uint16_t *)SYS32(v10 + 6368)));
    if ( (*v22__f) != 0 )
    {
      SETLO(v11, 0);
    }
    else if ( F32(a1, 236) != 0 )
    {
      v11 = (uint16_t)((F32(v5, 52) + 360000) / (360000 / F16(a1, 240)))
          % (int)F16(a1, 240);
      if ( F32(v5, 44) == 3 )
        SETLO(v11, (*v23__f) + v11);
      v4 = (*v31__f);
      v10 = (*v24__f);
    }
    else if ( F32(v5, 44) == 3 )
    {
      (*v30__f) = F32((a1 + 4 * (5 * (uint16_t)v3 + 20)), 0) - (*v30__f);
      SETLO(v11, (*v23__f));
    }
    else
    {
      SETLO(v11, 0);
      (*v30__f) += F32(v9, 84);
    }
    v12 = F32(v9, 88) + v4;
    v13 = v11 + F16(v9, 74);
    v14 = F32(a1, 244);
    v15 = F32(a1, 252);
    (*v24__f) = F32(v5, 4);
    v16 = F32(v5, 8);
    v32[0] = (*v25__f) + v13;
    v17 = F32(v5, 64);
    v18 = F32(v5, 68);
    (*v25__f) = v16;
    v19 = (*v30__f) + v17;
    v20 = v12 + v18;
    v21 = v32[0];
    F32(v29, 0) = v15;
    if ( v15 < 19 )
    {
      rd_4925e0(SYS32(v10 + 2732), v32[0], &(*v27__f), &(*v26__f), &(*v22__f), &(*v23__f));
      rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), (*v24__f), (*v25__f), v19 + (*v27__f), v20 + (*v26__f), (*v22__f), (*v23__f));
    }
    result = rd_417c40(((intptr_t)g_eng.sys), (*v28__f), v21);
    if ( result != 0 )
      return (int)rd_4917b0(a3, v29[0], v14 != 0, result, (*v24__f), (*v25__f), v19, v20, v14);
  }
  return result;
}

/* 0x493480: char __thiscall band_13e0_render_493480(cls_4022A0 *this, int a2, _DWORD *a3) */
int32_t rd_493480(wl_char *self, int32_t a2, int32_t * a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    int v20 = 0;
    uint16_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    int v29 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    _Alignas(8) uint8_t frame_[0x2c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v38 = (uint16_t *)(frame_ + 0x14);   /* [2] at ebp-0x18 */
    intptr_t *v39 = (intptr_t *)(frame_ + 0x18);   /* [2] at ebp-0x14 */
    uint16_t *v41 = (uint16_t *)(frame_ + 0x22);   /* [3] at ebp-0xa */
    uint16_t *const i__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x2c */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x28 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x24 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x20 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x1c */
    uint16_t *const v40__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0xc */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x4 */

  v4 = F32(c, 0x40) + 660 * a2;
  if ( F32(c, 0x1d4) == 100042 )
  {
    v5 = F16(c, 0x258);
    v11 = F16(c, 0x26c);
    if ( v11 >= v5 - 20 )
    {
      if ( F16(c, 0x26c) < F16(c, 0x258) )
      {
        v17 = *(_DWORD *)&F16(c, 0x1fc);
        v18 = 0;
        (*v37__f) = v17;
        v19 = SYS32(4 * (v5 - v11) + 10152);
        SETLB(v5, F8(v4, 0));
        (*v33__f) = v19;
        v45 = 0;
        if ( F8(v4, 0) != 0 )
        {
          v20 = F16(c, 0x250) == 0;
          (*v34__f) = 0;
          if ( !v20 )
          {
            do
            {
              v21 = *(_WORD *)&GAP(c, 0x1fe, 2 * (uint16_t)v18 + 2);
              if ( v21 == 255 )
              {
                v28 = 60 * v17 / 100;
                SETLB(v5, v28 + v45);
                v45 += v28;
              }
              else
              {
                v22 = rd_417c40(((intptr_t)g_eng.sys), F32(c, 0x1f8), v21);
                v23 = F32(v4, 4);
                v24 = v22;
                v25 = v45 - F32(v22, 12);
                v26 = F32(v4, 8);
                if ( v24 != 0 )
                {
                  F32(v38, 0) = (*v33__f) != 0;
                  rd_4917b0(
                    a3,
                    F32(c, 0xfc),
                    F32(v38, 0),
                    v24,
                    v23,
                    v26,
                    v25,
                    0,
                    (*v33__f));
                }
                v27 = F32(v24, 20);
                v17 = (*v37__f);
                v18 = (*v34__f);
                SETLB(v5, v27 + v45);
                v45 += v27;
              }
              v29 = (uint16_t)++v18 < F16(c, 0x250);
              (*v34__f) = v18;
            }
            while ( v29 );
          }
        }
      }
    }
    else
    {
      SETLB(v5, F8(v4, 0));
      v12 = *(_DWORD *)&F16(c, 0x1fc);
      v13 = 0;
      F32(v38, 0) = v12;
      v44 = 0;
      if ( (uint8_t)v5 != 0 )
      {
        for ( (*i__f) = 0; (*i__f) < F16(c, 0x250); ++(*i__f) )
        {
          v14 = *(_WORD *)&GAP(c, 0x1fe, 2 * (*i__f) + 2);
          if ( v14 == 255 )
          {
            v13 += 60 * v12 / 100;
            v44 = v13;
          }
          else
          {
            v15 = rd_417c40(((intptr_t)g_eng.sys), F32(c, 0x1f8), v14);
            if ( v15 != 0 )
              rd_4917b0(
                a3,
                F32(c, 0xfc),
                0,
                v15,
                F32(v4, 4),
                F32(v4, 8),
                v13 - F32(v15, 12),
                0,
                0);
            v16 = F32(v15, 20);
            v12 = F32(v38, 0);
            v13 = v16 + v44;
            v44 += v16;
          }
          SETLB(v5, (*i__f) + 1);
        }
      }
    }
  }
  else
  {
    v5 = W_entity_sprite_desc((intptr_t)(c), (intptr_t)(v4), (intptr_t)(v39));
    if ( v5 != 0 )
    {
      v6 = F32(c, 0xfc);
      v7 = F32(c, 0xf4);
      (*v31__f) = F32(v4, 4);
      v43 = F32(v4, 8);
      v8 = v39[0] + F32(v4, 64);
      v9 = *(_DWORD *)&F16(c, 0x268);
      v10 = v39[1] + F32(v4, 68);
      F32(v38, 0) = v6;
      if ( v9 != 0 && v6 < 19 )
      {
        rd_4925e0(SYS32(4 * (*v40__f) + 2732), v41[0], &(*v37__f), &(*v36__f), &(*v35__f), &(*v34__f));
        rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), (*v31__f), v43, v8 + (*v37__f), v10 + (*v36__f), (*v35__f), (*v34__f));
      }
      if ( v7 == 0 )
        v7 = (*v42__f);
      v5 = rd_417c40(((intptr_t)g_eng.sys), (*v40__f), v41[0]);
      if ( v5 != 0 )
        SETLB(v5, (uint8_t)rd_4917b0(
                                        a3,
                                        v38[0],
                                        v7 != 0,
                                        v5,
                                        (*v31__f),
                                        v43,
                                        v8,
                                        v10,
                                        v7));
    }
  }
  return v5;
}

/* 0x4937c0: int __thiscall band_1060_render_4937C0(cls_4937C0 *this, int a2, int a3) */
int32_t rd_4937c0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t result = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int16_t v9 = 0;
    char * v10 = 0;
    int16_t v11 = 0;
    intptr_t v12 = 0;
    int16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t i = 0;
    uint16_t v17 = 0;
    intptr_t v18 = 0;
    int32_t * v19 = 0;
    _Alignas(8) uint8_t frame_[0xb8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xb8 */
    int32_t * *const v21__f = (int32_t * *)(frame_ + 0x4);   /* ebp-0xb4 */
    int16_t *const v22__f = (int16_t *)(frame_ + 0x8);   /* ebp-0xb0 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0xa0 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x9c */
    int16_t *const v25__f = (int16_t *)(frame_ + 0x20);   /* ebp-0x98 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x22);   /* ebp-0x96 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x90 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x8c */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x88 */
    int16_t *const v30__f = (int16_t *)(frame_ + 0x34);   /* ebp-0x84 */
    int16_t *const v31__f = (int16_t *)(frame_ + 0x36);   /* ebp-0x82 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x7c */

  v4 = F32(c, 0x40) + 660 * a2;
  SETLO(v5, F16(c, 0xe8));
  (*v21__f) = (_DWORD *)v4;
  result = 5 * (uint16_t)v5;
  if ( *(_WORD *)&GAP(c, 0x44, 20 * (uint16_t)v5 + 4) != 0 && F8(v4, 0) != 0 )
  {
    result = F32(v4, 108);
    if ( result != 0 )
    {
      v7 = F16(v4, 114);
      if ( F16(result + 2 * v7, 2) == 0xFFFF )
        return result;
      (*v20__f) = F32(result, 76);
      v8 = F32(result + 4 * v7, 80);
      (*v23__f) = v8;
      (*v24__f) = F32(result + 4 * v7, 208);
      v9 = F16(result + 2 * v7, 2) + F16(result, 0);
      SETLO(result, F16(result, 336));
      (*v22__f) = v9;
      if ( (uint16_t)result != 0 )
        v5 = (uint16_t)result != 255 ? result : 0;
    }
    else
    {
      v8 = 0;
      v9 = 0;
      (*v23__f) = 0;
      (*v24__f) = 0;
      (*v22__f) = 0;
      (*v20__f) = 0;
    }
    v10 = &GAP(c, 0x0, 20 * (uint16_t)v5);
    v11 = F16(v10, 2 * 36);
    if ( (*v20__f) != 0 )
    {
      SETLO(v5, 0);
    }
    else if ( F32(c, 0xec) != 0 )
    {
      v5 = (uint16_t)(((*v21__f)[13] + 360000) / (360000 / F16(c, 0xf0))) % (int)F16(c, 0xf0);
      if ( (*v21__f)[11] == 3 )
        SETLO(v5, (*(uint16_t *)SYS32(4 * *((uint16_t *)v10 + 36) + 6368)) + v5);
      v9 = (*v22__f);
    }
    else
    {
      if ( (*v21__f)[11] == 3 )
      {
        v8 = *(_DWORD *)&GAP(c, 0x44, 20 * (uint16_t)v5 + 12) - v8;
        SETLO(v5, (*(uint16_t *)SYS32(4 * *((uint16_t *)v10 + 36) + 6368)));
      }
      else
      {
        v8 += F32(v10, 4 * 21);
        SETLO(v5, 0);
      }
      (*v23__f) = v8;
    }
    v12 = F32(v10, 4 * 22) + (*v24__f);
    (*v25__f) = v11;
    (*v24__f) = v12;
    v13 = v5 + F16(v10, 2 * 37);
    (*v27__f) = F32(c, 0xf4);
    v14 = F32(c, 0x1d4);
    SETLO((*v26__f), v9 + v13);
    if ( v14 == 100035 )
    {
      (*v29__f) = v12;
      (*v28__f) = v8;
      (*v31__f) = (*v26__f) + 4;
      v15 = F32(c, 0x40);
      (*v30__f) = v11;
      SETLO(i, 2);
      v17 = F16(v15, 60);
      if ( v17 <= 0xA0u )
      {
        if ( v17 >= 0x78u )
          v18 = 160 - F16(v15, 60);
        else
          v18 = 40;
      }
      else
      {
        v18 = 0;
      }
      (*v32__f) = W_tint_fade_b((intptr_t)(1000 * v18 / 1000), (intptr_t)(40));
    }
    else
    {
      SETLO(i, 1);
    }
    v19 = &(*v23__f);
    for ( i = (uint16_t)i; i != 0; --i )
    {
      result = rd_493a50((*v21__f), a3, F32(c, 0xfc), 1, 0, 0, 0, (int)v19);
      v19 += 5;
    }
  }
  return result;
}

/* 0x493a50: int __stdcall sub_493A50(_DWORD *a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t rd_493a50(int32_t * a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{

    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint16_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int32_t * v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t result = 0;
    uint16_t * v22 = 0;
    uint16_t v23 = 0;
    intptr_t v24 = 0;
    int32_t * v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;

  v8 = a6 + a1[2];
  v9 = a5 + a1[1];
  v10 = a8;
  v11 = F32(a8, 0) + a1[16];
  v12 = a3;
  v26 = F32(a8, 4) + a1[17];
  if ( a4 != 0 && a3 < 19 )
  {
    v13 = SYS32(4 * F16(a8, 8) + 2732);
    v28 = v13;
    v14 = F32(v13, 52);
    if ( v14 != 0 )
    {
      v15 = F32(v14 + 24 * F16(a8, 10), 8);
      v16 = F32(v14 + 24 * F16(a8, 10), 4);
      v17 = (int *)(v14 + 24 * F16(a8, 10));
      v18 = *v17;
      v19 = v16 - *v17;
      v20 = v17[3] - v15;
    }
    else
    {
      v19 = F32(v13, 32);
      v18 = 0;
      v15 = 0;
      v20 = F32(v28, 36);
    }
    rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v9, v8, v11 + v18, v26 + v15, v19, v20);
    v10 = a8;
    v12 = a3;
  }
  if ( a7 == 0 )
    a7 = F32(v10, 16);
  result = rd_417c40(((intptr_t)g_eng.sys), F16(v10, 8), F16(v10, 10));
  v27 = result;
  if ( result != 0 )
  {
    result = F32(a2, 84);
    v22 = (uint16_t *)(result + 8 * v12);
    v23 = *v22;
    if ( *v22 < v22[1] )
    {
      v24 = F32(v22, 4 * 1);
      *v22 = v23 + 1;
      v25 = (int *)(v24 + 60 * v23);
      *v25 = v27;
      v25[4] = v26;
      v25[1] = v9;
      v25[2] = v8;
      v25[3] = v11;
      v25[6] = a7 != 0;
      v25[5] = a7;
      return a7;
    }
  }
  return result;
}

/* 0x493ba0: __int16 __thiscall band_1160_char_render_493BA0(cls_4022A0 *this, unsigned __int16 a2, _DWORD *a3, int a4) */
int32_t rd_493ba0(wl_char *self, uint16_t a2, int32_t * a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    uint32_t v9 = 0;
    int32_t * v10 = 0;
    char * v11 = 0;
    char v12 = 0;
    int32_t * v13 = 0;
    char * v14 = 0;
    uint32_t v15 = 0;
    char * v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    uint16_t v19 = 0;
    uint16_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    int16_t v26 = 0;
    uint16_t v27 = 0;
    uint16_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    intptr_t v41 = 0;
    intptr_t v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    uint16_t * v45 = 0;
    int32_t * v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    uint32_t v49 = 0;
    int16_t v50 = 0;
    uint16_t v51 = 0;
    intptr_t v52 = 0;
    uint16_t * v53 = 0;
    char * v54 = 0;
    intptr_t i = 0;
    intptr_t v56 = 0;
    int32_t * v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    int32_t * v65 = 0;
    intptr_t v66 = 0;
    intptr_t v67 = 0;
    _Alignas(8) uint8_t frame_[0x53c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v92 = (uint16_t *)(frame_ + 0x10);   /* [2] at ebp-0x52c */
    uint16_t *v94 = (uint16_t *)(frame_ + 0x18);   /* [2] at ebp-0x524 */
    intptr_t *v103 = (intptr_t *)(frame_ + 0x3c);   /* [4] at ebp-0x500 */
    intptr_t *v110 = (intptr_t *)(frame_ + 0x2bc);   /* [160] at ebp-0x280 */
    intptr_t *const v74__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x53c */
    intptr_t *const v73__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x53c */
    intptr_t *const v72__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x53c */
    intptr_t *const v71__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x53c */
    intptr_t *const v70__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x53c */
    intptr_t *const v69__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x53c */
    intptr_t *const v80__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x538 */
    intptr_t *const v79__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x538 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x538 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x538 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x538 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x538 */
    intptr_t *const v87__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x534 */
    intptr_t *const v86__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x534 */
    intptr_t *const v85__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x534 */
    intptr_t *const v84__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x534 */
    intptr_t *const v83__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x534 */
    intptr_t *const v82__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x534 */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x534 */
    uint16_t *const j__f = (uint16_t *)(frame_ + 0xc);   /* ebp-0x530 */
    intptr_t *const v90__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x530 */
    intptr_t *const v89__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x530 */
    int16_t *const v88__f = (int16_t *)(frame_ + 0xc);   /* ebp-0x530 */
    intptr_t *const v93__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x528 */
    intptr_t *const v95__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x520 */
    intptr_t *const v96__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x51c */
    intptr_t *const v97__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x518 */
    intptr_t *const v98__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x514 */
    intptr_t *const v99__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x510 */
    intptr_t *const v100__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x50c */
    intptr_t *const v101__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x508 */
    intptr_t *const v102__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x504 */
    char *const v104__f = (char *)(frame_ + 0x4c);   /* ebp-0x4f0 */
    intptr_t *const v105__f = (intptr_t *)(frame_ + 0x50);   /* ebp-0x4ec */
    intptr_t *const v106__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x4e8 */
    uint16_t *const v107__f = (uint16_t *)(frame_ + 0x58);   /* ebp-0x4e4 */
    int16_t *const v108__f = (int16_t *)(frame_ + 0x5a);   /* ebp-0x4e2 */
    intptr_t *const v109__f = (intptr_t *)(frame_ + 0x60);   /* ebp-0x4dc */

  v5 = F32(c, 0x40) + 660 * a2;
  v6 = F32(c, 0xc764);
  if ( v6 != 0 )
    return v6;
  v6 = F32(c, 0xc794);
  if ( a4 != 0 )
  {
    if ( v6 == 0 )
      return v6;
  }
  else if ( v6 != 0 )
  {
    return v6;
  }
  v7 = F32(v5, 644);
  if ( v7 != 0 )
    F32(v92, 0) = F32(v7, 256);
  else
    F32(v92, 0) = F32(c, 0xfc);
  if ( F32(c, 0x1d4) == 50013 )
  {
    if ( F16(v5, 72) == 72 )
    {
      if ( F16(v5, 88) <= 1u )
        goto LABEL_16;
      goto LABEL_15;
    }
    SETLO(v6, F16(v5, 72) - 73);
    if ( F16(v5, 72) == 73 )
      return v6;
    if ( F16(v5, 72) == 74 )
LABEL_15:
      F32(v92, 0) = 23;
  }
LABEL_16:
  if ( F16(v5, 102) == 0 )
    goto LABEL_68;
  F32(v94, 0) = 0;
  v8 = F32(v5, 76);
  if ( v8 != 0 && strlen((const char *)(v8 + 84)) != 0 )
  {
    v9 = strlen((const char *)(v8 + 84)) + 1;
    qmemcpy(&F32(c, 0x23c), (const void *)(v8 + 84), 4 * (v9 >> 2));
    v11 = (const char *)(v8 + 84 + 4 * (v9 >> 2));
    v10 = &F32(c, 0x23c) + (v9 >> 2);
    v12 = v9;
  }
  else
  {
    v13 = &F32(c, 0x23c);
    strcpy((char *)&F32(c, 0x23c), ((char *)D(0x8554d8)));
    switch ( F16(v5, 72) )
    {
      case 0x7F:
        v14 = ((char *)D(0x8554b4));
        break;
      case 0x80:
        v14 = ((char *)D(0x8554c4));
        break;
      case 0x82:
        v14 = ((char *)D(0x8554d0));
        break;
      default:
        goto LABEL_28;
    }
    v15 = strlen(v14) + 1;
    v16 = &v14[v15];
    qmemcpy(v13, &v16[-v15], 4 * (v15 >> 2));
    v11 = &v16[4 * (v15 >> 2) - v15];
    v10 = &v13[v15 >> 2];
    v12 = v15;
  }
  qmemcpy(v10, v11, v12 & 3);
LABEL_28:
  switch ( F32(c, 0x15a5c) )
  {
    case 1:
      (*v88__f) = 10;
      F32(c, 0x210) = 3;
      F32(c, 0x214) = 1;
      goto LABEL_38;
    case 2:
      (*v88__f) = 11;
      F32(c, 0x210) = 4;
      F32(c, 0x214) = 3;
      F32(c, 0x234) = 0;
      F32(c, 0x238) = 20;
      goto LABEL_38;
    case 3:
      (*v88__f) = 12;
      F32(c, 0x210) = 1;
      F32(c, 0x214) = 3;
      F32(c, 0x234) = 10;
      F32(c, 0x238) = -20;
      goto LABEL_38;
    case 4:
      (*v88__f) = 13;
      F32(c, 0x210) = 2;
      F32(c, 0x214) = 2;
      goto LABEL_38;
    case 5:
      (*v88__f) = 10;
      goto LABEL_36;
    case 6:
      (*v88__f) = 11;
      F32(c, 0x210) = 4;
      F32(c, 0x214) = 3;
      F32(c, 0x234) = 0;
      F32(c, 0x238) = 20;
      goto LABEL_37;
    case 7:
      (*v88__f) = 13;
LABEL_36:
      F32(c, 0x210) = 2;
      F32(c, 0x214) = 2;
LABEL_37:
      F32(v94, 0) = 1;
LABEL_38:
      if ( F32(CFG(), 148) == 0 || F32(v94, 0) != 0 )
      {
        F32(c, 0x220) = 750;
        F32(c, 0x224) = 160;
        F32(c, 0x228) = 160;
      }
      else
      {
        F32(c, 0x220) = 1000;
        F32(c, 0x224) = 208;
        F32(c, 0x228) = 208;
      }
      v17 = F32(CFG(), 624);
      if ( v17 == 0 && F16(v5, 102) > 0xFAu )
        F32(v92, 0) = 22;
      SETLO(v17, F16(v5, 102));
      (*v93__f) = v17;
      v18 = 20 * (unsigned int)(uint16_t)v17 / 0x3E8;
      if ( v18 < 20 )
        (*v75__f) = SYS32(4 * v18 + 10072);
      else
        (*v75__f) = SYS32(10148);
      v19 = a2;
      v20 = a2;
      if ( a2 >= F16(c, 0x44) )
      {
        v20 = 0;
        v19 = 0;
      }
      (*v81__f) = F16(c, 0x204);
      v21 = F32(F32(c, 0x40) + 660 * v20, 20)
          + F32(F32(c, 0x40) + 660 * v20, 8)
          + (F32(F32(c, 0x40) + 660 * v20, 16) - F32(F32(c, 0x40) + 660 * v20, 20)) / 2;
      v22 = F32(c, 0x40) + 660 * v19;
      v23 = F32(v22, 4) + F32(v22, 12) / 2;
      v24 = a3[21] + 8 * v92[0];
      if ( F16(v24, 0) < F16(v24, 2) )
      {
        v25 = F32(v24, 4) + 60 * (uint16_t)(F16(v24, 0))++;
        F32(v25, 0) = 0;
        F32(v25, 4) = v23;
        F32(v25, 8) = v21;
        F32(v25, 12) = 0;
        F32(v25, 16) = -8;
        F32(v25, 24) = 2;
        F32(v25, 20) = 0;
        if ( v25 != 0 )
        {
          v26 = (*v93__f);
          F32(v25, 32) = (*v81__f);
          F16(v25, 28) = v26;
          F32(v25, 40) = &F32(c, 0x210);
          F16(v25, 30) = 1000;
          F16(v25, 36) = (*v88__f);
          F32(v25, 20) = (*v75__f);
        }
      }
      v27 = a2;
      if ( F32(v92, 0) < 19 )
      {
        v28 = a2;
        if ( a2 >= F16(c, 0x44) )
        {
          v28 = 0;
          v27 = 0;
        }
        v29 = F32(c, 0x228);
        (*v93__f) = *(_DWORD *)&GAP(W, 0x1030, 24);
        (*v69__f) = F32(c, 0x40) + 660 * v27;
        v30 = 33 * v28;
        (*v82__f) = (F32((*v69__f), 4) + F32((*v69__f), 12) / 2) / 100 - F32(c, 0x224) / 2;
        v31 = (F32(F32(c, 0x40) + 20 * v30, 20)
             + F32(F32(c, 0x40) + 20 * v30, 8)
             + (F32(F32(c, 0x40) + 20 * v30, 16) - F32(F32(c, 0x40) + 20 * v30, 20)) / 2)
            / 100
            - v29 / 2;
        (*v70__f) = v31 + v29 + 31;
        if ( v31 < (*v70__f) )
        {
          v32 = (*v82__f);
          v33 = (*v93__f);
          (*v76__f) = (*v82__f) + F32(c, 0x224) + 31;
          do
          {
            (*v89__f) = v32;
            if ( v32 < (*v76__f) )
            {
              v34 = v31 / 32;
              do
              {
                v35 = (*v89__f) / 32;
                if ( (*v89__f) / 32 >= 0
                  && v35 < F32(v33, 2887728)
                  && v34 >= 0
                  && v34 < F32(v33, 2887732)
                  && F32((v33 + 12 * ((v35 << 7) + v34 + 138244)), 0) != 0 )
                {
                  F8(v35 + 160 * v34 + v33, 1638448) = 1;
                }
                (*v89__f) += 32;
              }
              while ( (*v89__f) < (*v76__f) );
              v32 = (*v82__f);
            }
            v31 += 32;
          }
          while ( v31 < (*v70__f) );
        }
      }
      break;
    default:
      break;
  }
LABEL_68:
  if ( F16(c, 0x158a8) != 0 )
  {
    v36 = F32(c, 0x158ac);
    if ( v36 > 0 )
    {
      (*v93__f) = F32(c, 0x158b0);
      SETLO(v6, (*v93__f));
      if ( v36 < (*v93__f) )
      {
        v6 = W_entity_sprite_descs((intptr_t)(c), (intptr_t)(v5), (intptr_t)((int)v110), (intptr_t)(v92));
        if ( v6 != 0 )
        {
          SETLO(v6, 0);
          (*v83__f) = 0;
          if ( v92[0] != 0 )
          {
            (*v71__f) = v36 < 0 ? 0 : v36;
            do
            {
              v37 = 5 * (uint16_t)v6;
              v38 = rd_417c40(((intptr_t)g_eng.sys), v110[v37 + 2], HIWORD(v110[v37 + 2]));
              (*v96__f) = v110[v37 + 4];
              v39 = v110[v37 + 1];
              v40 = v110[v37];
              v41 = F32(v5, 68) + v39;
              v42 = F32(v38, 28);
              (*v97__f) = F32(v38, 32) - (*v71__f) * F32(v38, 32) / (*v93__f);
              v43 = F32(v5, 64);
              (*v95__f) = v42;
              v44 = v43 + v40;
              (*v90__f) = F32(v5, 8);
              (*v77__f) = F32(v5, 4);
              if ( v38 != 0 )
              {
                v45 = (_WORD *)(a3[21] + 176);
                if ( *v45 < F16(a3[21], 178) )
                {
                  v46 = (int *)(F32(a3[21], 180) + 60 * (uint16_t)(*v45)++);
                  v46[1] = (*v77__f);
                  v46[2] = (*v90__f);
                  v46[4] = v41;
                  v47 = (*v96__f);
                  *v46 = v38;
                  v46[3] = v44;
                  v46[6] = 3;
                  v46[5] = v47;
                  if ( v46 != 0 )
                  {
                    v48 = (*v97__f);
                    v46[11] = 0;
                    v46[12] = 0;
                    v46[13] = (*v95__f);
                    v46[14] = v48;
                  }
                }
              }
              SETLO(v6, ++(*v83__f));
            }
            while ( (uint16_t)(*v83__f) < v92[0] );
          }
        }
      }
      return v6;
    }
  }
  v49 = F32(c, 0x1d4);
  F32(v94, 0) = 1;
  if ( v49 > 0x18748 )
  {
    if ( v49 >= 0x18753 && v49 <= 0x18754 )
    {
      v6 = W_entity_part_descs((intptr_t)(c), (intptr_t)(v5), (intptr_t)((int)v103), (intptr_t)(v94));
      goto LABEL_101;
    }
  }
  else
  {
    if ( v49 < 0x18745 && v49 != 51027 && (v49 <= 0xC833 || v49 > 0xC835)
      || F16(v5, 72) != 3
      || F16(v5, 88) != 2 )
    {
      goto LABEL_100;
    }
    if ( F32(v5, 44) == 3 )
      v50 = (*(uint16_t *)SYS32(4 * F16(c, 0x48) + 6368)) + 1;
    else
      v50 = 1;
    F32(v94, 0) = 2;
    v51 = F16(c, 0x48);
    (*v105__f) = 0;
    (*v106__f) = 0;
    (*v107__f) = v51;
    (*v108__f) = v50;
    v52 = 20 * F32(v5, 92) / 16;
    if ( v52 >= 0 )
    {
      if ( v52 >= 20 )
      {
        (*v109__f) = 0;
        goto LABEL_100;
      }
    }
    else
    {
      v52 = 0;
    }
    (*v109__f) = SYS32(4 * v52 + 10152);
  }
LABEL_100:
  v6 = W_entity_sprite_desc((intptr_t)(c), (intptr_t)(v5), (intptr_t)(v103));
LABEL_101:
  if ( v6 != 0 )
  {
    if ( F32(c, 0x0) != 0 && F32(c, 0x8) != 0 )
    {
      SETLO(v6, F16(c, 0x4));
      if ( (uint16_t)v6 != 0 )
      {
        v53 = (_WORD *)(F32(c, 0x0) + 28);
        (*v84__f) = F16(c, 0x4);
        do
        {
          if ( *v53 != 0 && *((int32_t *)v53 - 1) != 0 && v94[0] != 0 )
          {
            v54 = &(*v104__f);
            for ( i = v94[0]; i != 0; --i )
            {
              if ( *((int32_t *)v54 - 1) == *((int32_t *)v53 - 1) )
                F32(v54, 0) = SYS32(4 * abs32(4 - (*v53 & 7)) + 10232);
              v54 += 20;
            }
          }
          v53 += 16;
          SETLO(v6, --(*v84__f));
        }
        while ( (*v84__f) != 0 );
      }
    }
    for ( (*j__f) = 0; (*j__f) < v94[0]; ++(*j__f) )
    {
      if ( F32(c, 0xc7f4) != 0 )
      {
        (*v78__f) = SYS32(10200);
        v56 = F32(v5, 4) + F32(c, 0xc7f8);
        v57 = &v103[5 * (*j__f)];
        v58 = F32(v5, 68);
        (*v72__f) = F32(v5, 8);
        (*v93__f) = *v57 + F32(v5, 64);
        (*v85__f) = v58 + v57[1];
        if ( F32(v92, 0) < 19 )
        {
          rd_4925e0(
            SYS32(4 * *((uint16_t *)v57 + 4) + 2732),
            F16(v57, 2 * 5),
            &(*v101__f),
            &(*v96__f),
            &(*v95__f),
            &(*v97__f));
          rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v56, (*v72__f), (*v93__f) + (*v101__f), (*v85__f) + (*v96__f), (*v95__f), (*v97__f));
        }
        if ( (*v78__f) == 0 )
          (*v78__f) = v57[4];
        v59 = rd_417c40(((intptr_t)g_eng.sys), F16(v57, 2 * 4), F16(v57, 2 * 5));
        if ( v59 != 0 )
          rd_4917b0(a3, v92[0], (*v78__f) != 0, v59, v56, (*v72__f), (*v93__f), (*v85__f), (*v78__f));
        v60 = F32(v5, 4) - F32(c, 0xc7f8);
        (*v86__f) = F32(v5, 8);
        (*v79__f) = SYS32(10200);
        v61 = F32(v5, 68);
        (*v93__f) = *v57 + F32(v5, 64);
        (*v73__f) = v61 + v57[1];
        if ( F32(v92, 0) < 19 )
        {
          rd_4925e0(
            SYS32(4 * *((uint16_t *)v57 + 4) + 2732),
            F16(v57, 2 * 5),
            &(*v100__f),
            &(*v99__f),
            &(*v98__f),
            &(*v102__f));
          rd_4926f0(*(intptr_t *)&GAP(W, 0x1030, 24), v60, (*v86__f), (*v93__f) + (*v100__f), (*v73__f) + (*v99__f), (*v98__f), (*v102__f));
        }
        if ( (*v79__f) == 0 )
          (*v79__f) = v57[4];
        v62 = rd_417c40(((intptr_t)g_eng.sys), F16(v57, 2 * 4), F16(v57, 2 * 5));
        if ( v62 != 0 )
          rd_4917b0(a3, v92[0], (*v79__f) != 0, v62, v60, (*v86__f), (*v93__f), (*v73__f), (*v79__f));
      }
      v63 = F32(v5, 64);
      (*v80__f) = F32(c, 0xf4);
      (*v87__f) = F32(v5, 4);
      v64 = v103[5 * (*j__f) + 1];
      v65 = &v103[5 * (*j__f)];
      (*v93__f) = F32(v5, 8);
      (*v74__f) = F32(v5, 68) + v64;
      v66 = *v65 + v63;
      if ( F32(v92, 0) < 19 )
        rd_4927c0(F16(v65, 2 * 4), F16(v65, 2 * 5), (*v87__f), (*v93__f), v66, (*v74__f));
      if ( (*v80__f) == 0 )
        (*v80__f) = v65[4];
      v67 = rd_417c40(((intptr_t)g_eng.sys), F16(v65, 2 * 4), F16(v65, 2 * 5));
      rd_492670(a3, v92[0], v67, (*v87__f), (*v93__f), v66, (*v74__f), (*v80__f));
      SETLO(v6, (*j__f) + 1);
    }
  }
  return v6;
}

/* 0x494790: int __thiscall sub_494790(cls_494790 *this, int a2, _DWORD *a3) */
int32_t rd_494790(wl_char *self, int32_t a2, int32_t * a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t result = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    uint32_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    uint16_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    uint16_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v31 = 0;
    int32_t * v32 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  v5 = a3[26];
  v6 = F32(c, 0xc);
  v31 = a3[25] / 100;
  result = 1374389535 * v5;
  (*v30__f) = v5 / 100;
  if ( v6 == 0 )
    return result;
  v8 = (F32(F32((v6 + 64), 0), 4) + F32(F32((v6 + 64), 0), 12) / 2) / 100 - a3[24] / 100;
  v9 = 25 * a3[29];
  v10 = a3[23];
  v11 = SYS32(2960);
  v12 = (int)((uint64_t)(1374389535LL * (v10 + 4 * v9)) >> 32) >> 5;
  v13 = (F32(F32((v6 + 64), 0), 16) + F32(F32((v6 + 64), 0), 8) - 1) / 100 - ((v12 >> 31) + v12);
  v14 = (v8 - 24) & ((v8 - 24 < 0) - 1);
  v15 = F16(c, 0x10);
  (*v29__f) = v14;
  if ( v15 == 2 )
  {
    v16 = ***(_DWORD ***)&SYSB(2960);
    if ( F32(c, 0x20) / 16 % 2 != 0 )
    {
      if ( v16 == 0 )
        W_fl_4184a0((intptr_t)(SYS32(2960)), (intptr_t)(0));
      v17 = F32(**(_DWORD **)((uint8_t *)(v11) + 0x0), 28);
    }
    else
    {
      if ( v16 == 0 )
        W_fl_4184a0((intptr_t)(SYS32(2960)), (intptr_t)(0));
      v17 = F32(**(_DWORD **)((uint8_t *)(v11) + 0x0), 20);
    }
    v18 = v31 - 48;
    if ( v31 - 48 >= v14 )
      v18 = v14;
    W_rp_5b9b70((intptr_t)(v17), (intptr_t)(a2), (intptr_t)(v18), (intptr_t)(v13));
  }
  else if ( v15 == 1 )
  {
    if ( ***(_DWORD ***)&SYSB(2960) == 0 )
      W_fl_4184a0((intptr_t)(SYS32(2960)), (intptr_t)(0));
    v21 = v31 - 48;
    v32 = (_DWORD *)(v31 - 48);
    if ( (int)v32 < v14 )
      v14 = v21;
    W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)((uint8_t *)(v11) + 0x0), 20)), (intptr_t)(a2), (intptr_t)(v14), (intptr_t)(v13));
    v22 = F32(c, 0x18);
    v23 = 40 * v22 / 10000;
    if ( v22 > 0 )
    {
      if ( **(_DWORD **)((uint8_t *)(v11) + 0x0) == 0 )
        W_fl_4184a0((intptr_t)(v11), (intptr_t)(0));
      v24 = v23 + 6;
      v25 = (int)v32;
      if ( (int)v32 >= (*v29__f) )
        v25 = (*v29__f);
      W_rp_5b9bf0((intptr_t)(F32(**(_DWORD **)((uint8_t *)(v11) + 0x0), 24)), (intptr_t)(a2), (intptr_t)(v25), (intptr_t)(v13 - v24 + 48), (intptr_t)(48), (intptr_t)(v24), (intptr_t)(0), (intptr_t)(48 - v24));
    }
    result = F32(c, 0x20) / 16 % 2;
    if ( result == 0 )
    {
      result = F32(c, 0x1c);
      if ( result != 0 )
      {
        if ( **(_DWORD **)((uint8_t *)(v11) + 0x0) == 0 )
          W_fl_4184a0((intptr_t)(v11), (intptr_t)(0));
        v26 = v13 + 48;
        v27 = (*v30__f) - 20;
        if ( (*v30__f) - 20 >= v26 )
          v27 = v26;
        v28 = (int)v32;
        if ( (int)v32 >= (*v29__f) )
          v28 = (*v29__f);
        return W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)((uint8_t *)(v11) + 0x0), 16)), (intptr_t)(a2), (intptr_t)(v28), (intptr_t)(v27));
      }
    }
    return result;
  }
  result = F32(c, 0x28);
  if ( result != 0 )
  {
    v19 = F32(c, 0x20) / 8 % 4;
    if ( **(_DWORD **)((uint8_t *)(v11) + 0x0) == 0 )
      W_fl_4184a0((intptr_t)(v11), (intptr_t)(0));
    v20 = v31 - 48;
    if ( v31 - 48 >= v14 )
      v20 = v14;
    return W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)((uint8_t *)(v11) + 0x0), 4 * v19)), (intptr_t)(a2), (intptr_t)(v20), (intptr_t)(v13));
  }
  return result;
}

/* 0x494a60: int __thiscall sub_494A60(cls_494A60 *this, int a2) */
int32_t rd_494a60(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    uint16_t v4 = 0;
    int32_t ** v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    uint16_t v12 = 0;
    uint16_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const y__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x4 */

  result = F32(c, 4 * 8);
  if ( result != 0 )
  {
    result = F32(c, 0x0) - 1;
    switch ( F32(c, 0x0) )
    {
      case 1:
        if ( ***(_DWORD ***)&SYSB(2996) != 0 )
        {
          (*v27__f) = (intptr_t)rd_418470(SYS32(2996), 0);
          v21 = (*v27__f);
        }
        else
        {
          v10 = F32(c, 4 * 3);
          (*v24__f) = 0;
          v11 = 14;
          v12 = 6;
          v13 = F16(v10, 10);
          (*v26__f) = 7;
          if ( v13 <= 0x1Cu )
          {
            if ( v13 <= 0x16u )
            {
              if ( v13 <= 0xEu )
              {
                if ( v13 > 0xAu )
                  v11 = 12;
              }
              else
              {
                v11 = 10;
                (*v24__f) = 4;
              }
            }
            else
            {
              v11 = 9;
              (*v24__f) = 6;
              v12 = 8;
              (*v26__f) = 9;
            }
          }
          else
          {
            v11 = 8;
            (*v24__f) = 6;
            v12 = 8;
            (*v26__f) = 9;
          }
          (*v29__f) = v11;
          v14 = v11 * v13;
          v15 = 160 - v14 / 2;
          (*v27__f) = (intptr_t)rd_418470(SYS32(2996), 0);
          W_rp_5b94e0((intptr_t)((*v27__f)), (intptr_t)((intptr_t *)&(*hdc__f)));
          API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
          API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)((uint8_t *)g_eng.sys + (4 * v12 + 10004))));
          API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x404040u));
          (*v28__f) = 3;
          (*y__f) = (*v24__f) + 9;
          do
          {
            v16 = -2;
            do
            {
              rd_48e860(F32(c, 4 * (3)), (*hdc__f), v16 + v15, (*y__f), v11, 0, 0);
              ++v16;
            }
            while ( v16 < 2 );
            v17 = (*v28__f) == 1;
            ++(*y__f);
            --(*v28__f);
          }
          while ( !v17 );
          API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
          v18 = (*v24__f) + 10;
          rd_48e860(F32(c, 4 * (3)), (*hdc__f), v15 - 1, (*v24__f) + 10, v11, 0, 0);
          rd_48e860(F32(c, 4 * (3)), (*hdc__f), 160 - v14 / 2, v18, v11, 0, 0);
          API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)((uint8_t *)g_eng.sys + (4 * (uint16_t)(*v26__f) + 10004))));
          API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x404040u));
          v19 = (*v29__f);
          v20 = (*v24__f) + 6;
          rd_48e6d0(F32(c, 4 * (3)), (*hdc__f), v15 + 1, (*v24__f) + 6, (*v29__f), 0, 0);
          API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
          rd_48e6d0(F32(c, 4 * (3)), (*hdc__f), 160 - v14 / 2, v20, v19, 0, 0);
          v21 = (*v27__f);
          W_rp_5b9500((intptr_t)((*v27__f)), (intptr_t)((intptr_t)(*hdc__f)));
        }
        v22 = F32(c, 4 * 2);
        if ( v22 < 1000 && F32(CFG(), 148) != 0 )
          return rd_494e10(
                   (intptr_t *)((intptr_t)g_eng.sys),
                   a2,
                   v21,
                   SYS32(4 * (20 * v22 / 1000) + 10152),
                   160,
                   64,
                   *(_DWORD *)&((uint8_t *)(v21) + 0x000)[40]);
        else
          return W_rp_5b9b70((intptr_t)(v21), (intptr_t)(a2), (intptr_t)(160), (intptr_t)(64));
      case 2:
        v4 = 1;
        goto LABEL_6;
      case 3:
        v4 = 0;
        goto LABEL_6;
      case 4:
        v4 = 2;
LABEL_6:
        v5 = *(_DWORD ***)&SYSB(3000);
        if ( ***(_DWORD ***)&SYSB(3000) == 0 )
          W_fl_4184a0((intptr_t)(SYS32(3000)), (intptr_t)(0));
        v6 = F32((**v5 + 4 * v4), 0);
        if ( F32(CFG(), 148) == 0 )
          goto LABEL_14;
        v7 = 20 * F32(c, 4 * 2) / 1000;
        if ( v7 >= 0 )
        {
          v8 = 20 * F32(c, 4 * 2) / 1000;
          if ( v7 >= 20 )
            goto LABEL_14;
        }
        else
        {
          v8 = 0;
        }
        v9 = SYS32(4 * v8 + 10152);
        if ( v9 != 0 )
          return W_rp_5bd550((intptr_t)(v9), (intptr_t)((intptr_t)a2), (intptr_t)(v6), (intptr_t)(F32(c, 4 * 5) + F32(v6, 12)), (intptr_t)(F32(c, 4 * 6) + F32(v6, 16)), (intptr_t)(F32(v6, 20)), (intptr_t)(F32(v6, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v6, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
LABEL_14:
        result = W_rp_5b9b70((intptr_t)(v6), (intptr_t)(a2), (intptr_t)(F32(c, 4 * 5)), (intptr_t)(F32(c, 4 * 6)));
        break;
      default:
        return result;
    }
  }
  return result;
}

/* 0x494e10: int __thiscall sub_494E10(HDC *this, paint_ctx *a2, _DWORD *a3, cls_5BD550 *a4, int a5, int a6, int a7) */
int32_t rd_494e10(wl_char *self, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;

  return W_rp_5bd550((intptr_t)(a4), (intptr_t)(a2), (intptr_t)((int)a3), (intptr_t)(a5 + a3[3]), (intptr_t)(a6 + a3[4]), (intptr_t)(a3[5]), (intptr_t)(a3[6]), (intptr_t)(0), (intptr_t)(0), (intptr_t)(a7), (intptr_t)(*(((intptr_t *)c) + 216)));
}

/* 0x494e60: int __thiscall sub_494E60(cls_494E60 *this, paint_ctx *a2) */
int32_t rd_494e60(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t result = 0;
    intptr_t v5 = 0;
    intptr_t * v6 = 0;
    int32_t * v7 = 0;
    intptr_t v8 = 0;
    int32_t * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    char * v18 = 0;
    intptr_t v19 = 0;
    char * v20 = 0;
    uint16_t * v21 = 0;
    uint16_t v22 = 0;
    intptr_t v23 = 0;
    uint16_t * v24 = 0;
    uint16_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    intptr_t v29 = 0;
    intptr_t v30 = 0;
    int32_t * v31 = 0;
    int32_t * v32 = 0;
    int32_t * v33 = 0;
    uint16_t v34 = 0;
    uint16_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    uint16_t v40 = 0;
    intptr_t v41 = 0;
    int v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    int32_t * v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    char * v56 = 0;
    intptr_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    intptr_t v65 = 0;
    int16_t v66 = 0;
    int16_t v67 = 0;
    int16_t v68 = 0;
    intptr_t v69 = 0;
    intptr_t v70 = 0;
    int16_t v71 = 0;
    int64_t v72 = 0;
    intptr_t v73 = 0;
    intptr_t v74 = 0;
    _Alignas(8) uint8_t frame_[0x200 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *Buffer = (intptr_t *)(frame_ + 0x60);   /* [10] at ebp-0x1a0 */
    char *v122 = (char *)(frame_ + 0xd8);   /* [40] at ebp-0x128 */
    uint16_t *v123 = (uint16_t *)(frame_ + 0x100);   /* [128] at ebp-0x100 */
    intptr_t *const v75__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x200 */
    intptr_t *const v76__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x1fc */
    intptr_t *const v80__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1f8 */
    intptr_t *const v79__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1f8 */
    intptr_t *const v78__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1f8 */
    intptr_t *const v77__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x1f8 */
    intptr_t *const v85__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1e4 */
    intptr_t *const v84__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1e4 */
    intptr_t *const v83__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1e4 */
    intptr_t *const v81__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x1e4 */
    int16_t *const i__f = (int16_t *)(frame_ + 0x1c);   /* ebp-0x1e4 */
    uint16_t *const v86__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0x1e0 */
    intptr_t *const v88__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x1e0 */
    intptr_t *const v87__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x1e0 */
    intptr_t *const v91__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x1dc */
    intptr_t *const v90__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x1dc */
    int32_t * *const v92__f = (int32_t * *)(frame_ + 0x24);   /* ebp-0x1dc */
    int32_t * *const v89__f = (int32_t * *)(frame_ + 0x24);   /* ebp-0x1dc */
    intptr_t *const v93__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x1d8 */
    intptr_t *const v97__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x1d4 */
    intptr_t *const v96__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x1d4 */
    intptr_t *const v95__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x1d4 */
    intptr_t *const v94__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x1d4 */
    intptr_t *const v99__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x1d0 */
    intptr_t *const v101__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x1d0 */
    intptr_t *const v100__f = (intptr_t *)(frame_ + 0x30);   /* ebp-0x1d0 */
    int32_t * *const v98__f = (int32_t * *)(frame_ + 0x30);   /* ebp-0x1d0 */
    intptr_t *const v104__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x1cc */
    intptr_t *const v103__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x1cc */
    intptr_t *const v102__f = (intptr_t *)(frame_ + 0x34);   /* ebp-0x1cc */
    int16_t *const v105__f = (int16_t *)(frame_ + 0x34);   /* ebp-0x1cc */
    intptr_t *const v106__f = (intptr_t *)(frame_ + 0x38);   /* ebp-0x1c8 */
    int32_t * *const v107__f = (int32_t * *)(frame_ + 0x38);   /* ebp-0x1c8 */
    intptr_t *const v109__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x1c4 */
    intptr_t *const v108__f = (intptr_t *)(frame_ + 0x3c);   /* ebp-0x1c4 */
    intptr_t *const v111__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x1c0 */
    intptr_t *const v110__f = (intptr_t *)(frame_ + 0x40);   /* ebp-0x1c0 */
    intptr_t *const v112__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x1bc */
    intptr_t *const v113__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x1b8 */
    intptr_t *const v114__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x1b4 */
    uint16_t *const v115__f = (uint16_t *)(frame_ + 0x50);   /* ebp-0x1b0 */
    intptr_t *const v116__f = (intptr_t *)(frame_ + 0x54);   /* ebp-0x1ac */
    intptr_t *const v118__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x1a8 */
    intptr_t *const v117__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x1a8 */
    intptr_t *const v119__f = (intptr_t *)(frame_ + 0x5c);   /* ebp-0x1a4 */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x88);   /* ebp-0x178 */

  v3 = 0;
  (*v115__f) = 0;
  result = F32(c, 0x8);
  (*v112__f) = 0;
  (*v116__f) = 0;
  if ( result != 0 )
  {
    v5 = 8;
    v6 = (intptr_t *)(F32(c, 0x4) + 16432);
    do
    {
      if ( W_pslot_ok((intptr_t)(*v6), (intptr_t)(0), (intptr_t)(0)) )
        ++v3;
      ++v6;
      --v5;
    }
    while ( v5 != 0 );
    (*v114__f) = v3;
    if ( v3 > 1 )
      (*v116__f) = 1;
    v7 = (_DWORD *)F32(c, 0x24);
    if ( v7 != 0 )
    {
      v8 = v7[16];
      if ( v7[128] == 0 && v7[12770] != 0 && F32(v8, 76) != 0 )
        (*v112__f) = F32(v8, 80);
      else
        (*v112__f) = 0;
    }
    if ( F32(c, 0x1b4) != 0 )
    {
      (*v108__f) = 16432;
      (*v110__f) = 8;
      while ( 1 )
      {
        v9 = F32(((*v108__f) + F32(c, 0x4)), 0);
        (*v89__f) = v9;
        if ( v9[625] == 1 && v9[636] != 100000104 )
        {
          v10 = v9[637];
          if ( F32(c, 0x1b4) == v10 )
            break;
        }
LABEL_85:
        v42 = (*v110__f) == 1;
        (*v108__f) += 4;
        --(*v110__f);
        if ( v42 )
          goto LABEL_86;
      }
      v11 = F32(c, 0x1b0);
      v12 = F32(v10, 1872);
      if ( v11 == 0 )
      {
LABEL_44:
        (*v117__f) = F32(c, 0x10);
        v28 = (int)(v9 + 644);
        v29 = v9[637];
        (*v90__f) = F32(c, 0x1b0);
        (*v94__f) = v28;
        SETLB((*hdc__f), 0);
        (*v102__f) = SYS32(2928);
        (*v86__f) = 0;
        (*v99__f) = -1;
        if ( (*v90__f) != 0 )
        {
          if ( rd_496170(v29) == 1 )
          {
            for ( (*i__f) = 0; (uint16_t)(*i__f) < 0xAu; ++(*i__f) )
            {
              v31 = (int *)W_member_skill_at((intptr_t)(v28), (intptr_t)(F32(v28, 320)), (intptr_t)((*i__f)));
              v32 = v31;
              if ( v31 != 0 )
              {
                v33 = W_skill_row_find((intptr_t)(*v31));
                if ( v33 != 0 && v33[42] != 0 )
                {
                  v34 = F16(v33, 2 * 86);
                  v35 = F16(v32, 2 * 4);
                  v123[(*v86__f)] = v34;
                  v123[(*v86__f) + 64] = v35;
                  if ( (*i__f) == W_party_slot_get((intptr_t)((*v94__f))) )
                    (*v99__f) = (*v86__f);
                  v28 = (*v94__f);
                  ++(*v86__f);
                }
              }
            }
            switch ( (*v117__f) )
            {
              case 1:
                (*v113__f) = 632 - 32 * (*v86__f);
                v36 = (((*v90__f) - 1000) << 6) / 1000;
                (*v106__f) = v36;
                break;
              case 2:
                v36 = 96;
                (*v106__f) = 96;
                (*v113__f) = 280 * ((*v90__f) - 1000) / 1000 + 10;
                break;
              case 3:
                v36 = 96;
                (*v106__f) = 96;
                (*v113__f) = 280 * (1000 - (*v90__f)) / 1000 + 360;
                break;
              default:
                v36 = (*v106__f);
                break;
            }
            if ( (*v86__f) != 0 )
            {
              v37 = (*v113__f);
              (*v83__f) = 0;
              (*v91__f) = (*v86__f);
              do
              {
                if ( (*v83__f) == (*v99__f) )
                {
                  (*v80__f) = v36 - 4;
                  (*v76__f) = v37;
                  (*v75__f) = a2;
                  (*v95__f) = v36 + 4;
                  v38 = rd_418470(SYS32(2960), 0xBu);
                }
                else
                {
                  (*v95__f) = v36;
                  (*v80__f) = v36 - 8;
                  (*v76__f) = v37;
                  (*v75__f) = a2;
                  v38 = rd_418470(SYS32(2960), 0xAu);
                }
                W_rp_5b9b70((intptr_t)(v38), (intptr_t)((int)(*v75__f)), (intptr_t)((*v76__f)), (intptr_t)((*v80__f)));
                v39 = rd_418470(SYS32(2936), v123[(*v83__f)]);
                W_rp_5b9b70((intptr_t)(v39), (intptr_t)((int)a2), (intptr_t)(v37), (intptr_t)((*v95__f)));
                v40 = v123[(*v83__f) + 64];
                if ( v40 != 255 )
                {
                  v41 = rd_418470((*v102__f), v40 + 22);
                  W_rp_5b9b70((intptr_t)(v41), (intptr_t)((int)a2), (intptr_t)(v37), (intptr_t)(v36 + 1));
                }
                v37 += 32;
                v42 = (*v91__f) == 1;
                ++(*v83__f);
                --(*v91__f);
              }
              while ( !v42 );
            }
            if ( ((uint8_t)((*hdc__f))) != 0 )
              rd_43e250(2u, a2, &(*hdc__f), 32 * (*v86__f) - 6 * strlen((const char *)&(*hdc__f)) + (*v113__f) + 1, v36 + 41, 0);
            v30 = (*v86__f) != 0;
          }
          else
          {
            v30 = false;
          }
        }
        else
        {
          v30 = false;
        }
        v43 = F32(c, 0x1b0);
        if ( v43 != 0 )
        {
          v44 = F32(c, 0x10);
          if ( v44 > 0 && v44 <= 2 )
          {
            (*v100__f) = 64;
            (*v103__f) = (80 * (1000 - v43) / 1000 + 600);
            if ( !v30 )
              (*v100__f) = 8;
            (*v96__f) = 0;
            if ( F32(c, 0x33c) > 0 )
            {
              v45 = 0;
              do
              {
                (*v87__f) = (int)(*v103__f) + 80 * (1000 - *(_DWORD *)&GAP(c, 0x1d4, 20 * v45 + 8)) / 1000;
                (*v84__f) = 32 * *(_DWORD *)&GAP(c, 0x1d4, 20 * v45 + 16) / 1000 + (*v100__f) + 34 * v45;
                v46 = rd_418470(SYS32(2960), 0xDu);
                W_rp_5b9b70((intptr_t)(v46), (intptr_t)((int)a2), (intptr_t)((*v87__f)), (intptr_t)((*v84__f)));
                v47 = rd_418470(
                                      SYS32(2936),
                                      ((uint16_t *)D(0x62875c))[70 * *(uint16_t *)&GAP(c, 0x1d4, 20 * v45)]);
                W_rp_5b9b70((intptr_t)(v47), (intptr_t)((int)a2), (intptr_t)((*v87__f)), (intptr_t)((*v84__f)));
                if ( ((int32_t *)D(0x628774))[35 * *(uint16_t *)&GAP(c, 0x1d4, 20 * v45)] != 0 )
                {
                  v48 = rd_418470(SYS32(2960), 0xEu);
                  W_rp_5b9b70((intptr_t)(v48), (intptr_t)((int)a2), (intptr_t)((*v87__f)), (intptr_t)((*v84__f)));
                  rd_498f10(
                    (int)a2,
                    (*v87__f) + 3,
                    (*v84__f) + 28,
                    1,
                    4,
                    0,
                    *(uint16_t *)&GAP(c, 0x1d4, 20 * v45 + 2),
                    *(uint16_t *)&GAP(c, 0x1d4, 20 * v45 + 4),
                    26,
                    0x2Eu,
                    3,
                    0);
                }
                else
                {
                  v49 = rd_418470(SYS32(2960), 0xFu);
                  W_rp_5b9b70((intptr_t)(v49), (intptr_t)((int)a2), (intptr_t)((*v87__f)), (intptr_t)((*v84__f)));
                }
                v45 = (uint16_t)++(*v96__f);
              }
              while ( (uint16_t)(*v96__f) < F32(c, 0x33c) );
            }
          }
        }
        goto LABEL_85;
      }
      v13 = F32(c, 0x10);
      if ( v13 > 0 )
      {
        if ( v13 <= 2 )
        {
          v14 = 1;
          (*v119__f) = 1;
          (*v93__f) = 32 * (v11 + 2 * (5 * v11 - 5000) - 1000) / 1000 + 1;
LABEL_26:
          rd_498680(
            c,
            (int)a2,
            F32(c, 0x1b8),
            1000,
            (*v93__f) + 117,
            v14 + 6,
            4,
            168,
            0,
            6,
            0x2Fu,
            1);
          rd_498820((int)a2, F32(c, 0x1bc), 1000, (*v93__f) + 117, v14 + 20, 3, 168, 2, 0x2Fu);
          rd_498f10((int)a2, (*v93__f) + 143, v14 + 41, 1, 4, 0, v12[58], v12[59], 104, 0x2Eu, 0, 1);
          v15 = rd_418470(SYS32(3012), 0);
          W_rp_5b9b70((intptr_t)(v15), (intptr_t)((int)a2), (intptr_t)((*v93__f) - 32), (intptr_t)(v14));
          (*v81__f) = 0;
          if ( (int)v12[55] > 0 )
          {
            v16 = 0;
            do
            {
              rd_498620((int)a2, v16 + (*v93__f) + 12 * v16 + 186, v14 + 29, v16 + 1, v12[54]);
              v16 = (uint16_t)++(*v81__f);
            }
            while ( (uint16_t)(*v81__f) < (int)v12[55] );
          }
          (*v77__f) = v12[22] + v12[33] + v12[39];
          v17 = rd_497b40(v12, F32(c, 0x1b8));
          v18 = rd_495dc0(v17, 4u, v122);
          g_eng.h5bf3ee((char *const)Buffer, "%s / %d", v18, (*v77__f));
          rd_43e250(2u, a2, Buffer, (*v93__f) + 217, v14 + 4, 1);
          (*v78__f) = v12[24] + v12[34] + v12[40];
          v19 = rd_497bb0(v12, F32(c, 0x1bc));
          v20 = rd_495dc0(v19, 4u, v122);
          g_eng.h5bf3ee((char *const)Buffer, "%s / %d", v20, (*v78__f));
          rd_43e250(2u, a2, Buffer, (*v93__f) + 217, v14 + 17, 1);
          g_eng.h5bf3ee((char *const)Buffer, "%d", v12[54] + v12[56]);
          rd_495e40(((intptr_t)g_eng.sys), (int)a2, (const char *)Buffer, 1u, 9, (*v93__f) + 160, v14 + 24, 0);
          v21 = (uint16_t *)v12[20];
          if ( v21 != 0 )
          {
            if ( F32(c, 0x1c8) == 2 )
            {
              v22 = v21[4];
            }
            else if ( F32(c, 0x1c8) == 3 )
            {
              v22 = v21[5];
            }
            else
            {
              v22 = v21[3];
            }
            v23 = rd_418470(SYS32(4 * v21[2] + 2732), v22);
            W_rp_5b9b70((intptr_t)(v23), (intptr_t)((int)a2), (intptr_t)((*v93__f)), (intptr_t)(v14));
            if ( F32(c, 0x1d0) > 0 )
            {
              if ( F32(c, 0x1cc) == 2 )
              {
                v24 = (uint16_t *)v12[20];
                v25 = v24[4];
              }
              else
              {
                v24 = (uint16_t *)v12[20];
                if ( F32(c, 0x1cc) == 3 )
                  v25 = v24[5];
                else
                  v25 = v24[3];
              }
              (*v98__f) = (_DWORD *)rd_418470(SYS32(4 * v24[2] + 2732), v25);
              (*v79__f) = (*v98__f)[10];
              v26 = W_tint_fade_a((intptr_t)(F32(c, 0x1d0) - 1), (intptr_t)(1000));
              rd_494e10((intptr_t *)((intptr_t)g_eng.sys), a2, (*v98__f), v26, (*v93__f), v14, (*v79__f));
            }
            v27 = rd_418470(SYS32(3008), F16(v12[20], 20));
            W_rp_5b9a40((intptr_t)(v27), (intptr_t)((int)a2), (intptr_t)((*v93__f) + 91), (intptr_t)(v14 + 28));
          }
          v9 = (*v89__f);
          goto LABEL_44;
        }
        if ( v13 == 3 )
          goto LABEL_44;
      }
      v14 = (*v119__f);
      goto LABEL_26;
    }
LABEL_86:
    (*v109__f) = 16432;
    (*v107__f) = (int *)((uint8_t *)(c) + 0x340);
    (*v97__f) = 8;
    do
    {
      v50 = F32(((*v109__f) + F32(c, 0x4)), 0);
      (*v92__f) = v50;
      if ( v50[625] == 1 && v50[636] != 100000104 )
      {
        v51 = v50[637];
        if ( F32(c, 0x24) == v51 )
        {
          rd_497f40(c, a2, v51, &F32(c, 0x20), F32(c, 0x10));
          v52 = rd_498960(
                  a2,
                  (_DWORD *)v50[637],
                  (int)(v50 + 644),
                  F32(c, 0x20),
                  F32(c, 0x10),
                  F16(c, 0x34) != 0,
                  F32(c, 0x388));
          v53 = F32(c, 0x20);
          (*v104__f) = v52;
          if ( v53 != 0 )
          {
            v54 = F32(c, 0x10);
            if ( v54 > 0 && v54 <= 2 )
            {
              (*v111__f) = 64;
              (*v118__f) = 80 * (1000 - v53) / 1000 + 600;
              if ( (*v104__f) == 0 )
                (*v111__f) = 8;
              (*v101__f) = 0;
              if ( F32(c, 0x1ac) > 0 )
              {
                v55 = 0;
                do
                {
                  v56 = &GAP(c, 0x1e, 20 * v55 + 2);
                  (*v88__f) = (*v118__f) + 80 * (1000 - F32(v56, 4 * 11)) / 1000;
                  (*v85__f) = 32 * F32(v56, 4 * 13) / 1000 + (*v111__f) + 34 * v55;
                  v57 = rd_418470(SYS32(2960), 0xDu);
                  W_rp_5b9b70((intptr_t)(v57), (intptr_t)((int)a2), (intptr_t)((*v88__f)), (intptr_t)((*v85__f)));
                  v58 = rd_418470(
                                        SYS32(2936),
                                        ((uint16_t *)D(0x62875c))[70 * *((uint16_t *)v56 + 18)]);
                  W_rp_5b9b70((intptr_t)(v58), (intptr_t)((int)a2), (intptr_t)((*v88__f)), (intptr_t)((*v85__f)));
                  if ( ((int32_t *)D(0x628774))[35 * *((uint16_t *)v56 + 18)] != 0 )
                  {
                    v59 = rd_418470(SYS32(2960), 0xEu);
                    W_rp_5b9b70((intptr_t)(v59), (intptr_t)((int)a2), (intptr_t)((*v88__f)), (intptr_t)((*v85__f)));
                    rd_498f10(
                      (int)a2,
                      (*v88__f) + 3,
                      (*v85__f) + 28,
                      1,
                      4,
                      0,
                      *((uint16_t *)v56 + 19),
                      *(uint16_t *)&GAP(c, 0x36, 20 * v55 + 18),
                      26,
                      0x2Eu,
                      3,
                      0);
                  }
                  else
                  {
                    v60 = rd_418470(SYS32(2960), 0xFu);
                    W_rp_5b9b70((intptr_t)(v60), (intptr_t)((int)a2), (intptr_t)((*v88__f)), (intptr_t)((*v85__f)));
                  }
                  v55 = (uint16_t)++(*v101__f);
                }
                while ( (uint16_t)(*v101__f) < F32(c, 0x1ac) );
                v50 = (*v92__f);
              }
            }
          }
        }
        if ( (*v116__f) != 0 )
        {
          v61 = *v50 != 0;
          (*v105__f) = F16(F32(v50[637], 1872), 1098);
          v62 = W_dramatist_row((intptr_t)(v50[636]));
          if ( v62 != 0 )
            v63 = F32(v62, 4);
          else
            v63 = v50[1];
          rd_496560(
            c,
            a2,
            v50 + 644,
            (*v115__f),
            (*v114__f),
            F32(c, 0x3c8),
            F32(c, 0x10),
            v63,
            (*v112__f) != 0,
            (*v112__f) == v50[637],
            v61,
            (*v105__f),
            (*v107__f));
        }
        ++(*v115__f);
      }
      v42 = (*v97__f) == 1;
      (*v109__f) += 4;
      (*v107__f) += 2;
      --(*v97__f);
    }
    while ( !v42 );
    rd_497c20(a2, F32(c, 0x4), F32(c, 0x14), F32(c, 0x10));
    rd_4963a0(c, (int)a2, F32(c, 0x3c8), F32(c, 0x10), F32(c, 0x4), (*v116__f));
    v64 = rd_4961a0(F32(c, 0x4));
    rd_4962a0((int)a2, 0, (v64 != 0) + 44, F32(c, 0x3c8), F32(c, 0x388));
    if ( F16(F32(c, 0x4), 16496) != 0 )
      rd_4962a0((int)a2, 1u, 49, F32(c, 0x3c8), F32(c, 0x388));
    else
      rd_4962a0((int)a2, 1u, 48, F32(c, 0x3c8), F32(c, 0x388));
    v65 = rd_496240(F32(c, 0x4), 0, 0) - 1;
    if ( v65 != 0 )
    {
      if ( v65 == 1 )
        v66 = 2;
      else
        v66 = 0;
    }
    else
    {
      v66 = 1;
    }
    v67 = v66 + 40;
    rd_4962a0((int)a2, 2u, v67, F32(c, 0x3c8), F32(c, 0x388));
    switch ( rd_4961e0(F32(c, 0x4), 0, 0) )
    {
      case 0:
        v67 = 0;
        break;
      case 1:
        v67 = 1;
        break;
      case 2:
        v67 = 2;
        break;
      case 3:
        v67 = 3;
        break;
      case 4:
        v67 = 4;
        break;
      default:
        break;
    }
    rd_4962a0((int)a2, 3u, v67 + 33, F32(c, 0x3c8), F32(c, 0x388));
    v68 = 3;
    v69 = F32(F32(c, 0x4), 8204);
    if ( v69 != 0 )
    {
      v70 = F32(v69, 2548);
      if ( v70 != 0 && rd_496170(v70) == 1 )
      {
        switch ( F32(F32((F32(v69, 2548) + 1872), 0), 320) )
        {
          case 1:
            v68 = 0;
            break;
          case 2:
            v68 = 1;
            break;
          case 3:
            v68 = 2;
            break;
          default:
            break;
        }
      }
    }
    rd_4962a0((int)a2, 4u, v68 + 56, F32(c, 0x3c8), F32(c, 0x388));
    if ( F32(F32(c, 0x4), 16472) == 1 )
    {
      v71 = 1;
    }
    else if ( F32(F32(c, 0x4), 16472) == 2 )
    {
      v71 = 2;
    }
    else
    {
      v71 = 0;
    }
    rd_4962a0((int)a2, 5u, v71 + 80, F32(c, 0x3c8), F32(c, 0x388));
    rd_4975e0(c, a2, F32(c, 0x398), F32(c, 0x388), F32(c, 0x10));
    if ( F32(CFG(), 204) == 0 )
      rd_496ec0(c, a2, F32(c, 0x3d8), F32(c, 0x10), (*v114__f));
    rd_4969b0((int)a2, F32(c, 0x24), *(_DWORD **)&GAP(W, 0x1030, 28), (_DWORD *)F32(c, 0x398));
    if ( (*v112__f) != 0 )
    {
      v72 = 50 - F16(c, 0x3cc);
      v73 = W_tint_fade_a((intptr_t)((HIDWORD(v72) ^ v72) - HIDWORD(v72)), (intptr_t)(50));
      rd_495fe0((*v112__f), (int)a2, 0, *(_DWORD **)&GAP(W, 0x1030, 28), v73, 1u);
    }
    result = F32(c, 0x18);
    if ( result != 0 )
    {
      v74 = rd_499070(F16(c, 0x1c));
      return rd_495fe0(F32(c, 0x18), (int)a2, 0, *(_DWORD **)&GAP(W, 0x1030, 28), v74, 0);
    }
  }
  return result;
}

/* 0x43e250: void __stdcall sub_43E250(unsigned __int16 a1, paint_ctx *a2, HDC hdc, int y, int a5, int a6) */
void rd_43e250(uint16_t a1, int32_t a2, int32_t hdc, int32_t y, int32_t a5, int32_t a6)
{

    char * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v12 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *const c_l__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v6 = (const CHAR *)hdc;
  (*c_l__f) = strlen((const char *)hdc);
  if ( W_rp_5b94e0((intptr_t)(a2), (intptr_t)((intptr_t *)&hdc)) != 0 )
  {
    API_SetBkMode((intptr_t)(hdc), (intptr_t)(1));
    API_SelectObject((intptr_t)(hdc), (intptr_t)(***(intptr_t ***)((uint8_t *)g_eng.sys + (4 * a1 + 10004))));
    API_SetTextColor((intptr_t)(hdc), (intptr_t)(0x202020u));
    v7 = y;
    v12 = -1;
    if ( a6 != 0 )
    {
      do
      {
        v8 = -1;
        y = a5 + v12;
        do
        {
          API_TextOutA((intptr_t)(hdc), (intptr_t)(v8 + v7), (intptr_t)(y), (intptr_t)(v6), (intptr_t)((*c_l__f)));
          ++v8;
        }
        while ( v8 <= 2 );
        ++v12;
      }
      while ( v12 <= 1 );
    }
    else
    {
      do
      {
        v9 = -1;
        y = a5 + v12;
        do
        {
          API_TextOutA((intptr_t)(hdc), (intptr_t)(v9 + v7), (intptr_t)(y), (intptr_t)(v6), (intptr_t)((*c_l__f)));
          ++v9;
        }
        while ( v9 <= 1 );
        ++v12;
      }
      while ( v12 <= 1 );
    }
    API_SetTextColor((intptr_t)(hdc), (intptr_t)(0xFFFFFFu));
    v10 = a5;
    API_TextOutA((intptr_t)(hdc), (intptr_t)(v7), (intptr_t)(a5), (intptr_t)(v6), (intptr_t)((*c_l__f)));
    if ( a6 != 0 )
      API_TextOutA((intptr_t)(hdc), (intptr_t)(v7 + 1), (intptr_t)(v10), (intptr_t)(v6), (intptr_t)((*c_l__f)));
    W_rp_5b9500((intptr_t)(a2), (intptr_t)((intptr_t)hdc));
  }
}

/* 0x495dc0: char *__stdcall sub_495DC0(int a1, unsigned __int16 a2, char *Buffer) */
int32_t rd_495dc0(int32_t a1, uint16_t a2, char * Buffer)
{


    _Alignas(8) uint8_t frame_[0x200 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Format = (char *)(frame_ + 0x0);   /* [512] at ebp-0x200 */

  if ( a2 != 0 )
  {
    g_eng.h5bf3ee(Format, "%% %dd", a2);
    g_eng.h5bf3ee(Buffer, Format, a1);
  }
  else
  {
    g_eng.h5bf3ee(Buffer, "%d", a1);
  }
  return Buffer;
}

/* 0x495e40: int __thiscall sub_495E40(_DWORD *this, int a2, const char *a3, unsigned __int16 a4, int a5, int a6, int a7, int a8) */
int32_t rd_495e40(wl_char *self, int32_t a2, char * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint32_t v10 = 0;
    char v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t * v14 = 0;
    uint16_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    int v22 = 0;
    intptr_t i = 0;
    _Alignas(8) uint8_t frame_[0x60 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v27 = (uint16_t *)(frame_ + 0x10);   /* [40] at ebp-0x50 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x60 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x5c */

  v8 = 0;
  (*v24__f) = 0;
  (*v25__f) = 0;
  v9 = 0;
  while ( 1 )
  {
    v10 = strlen(a3) <= 0x28 ? strlen(a3) : 40;
    if ( (uint16_t)v8 >= v10 )
      break;
    v11 = a3[(uint16_t)v8];
    if ( v11 == 32 )
    {
      v12 = (uint16_t)v9++;
      ++v8;
      v27[v12] = 255;
    }
    else
    {
      if ( v11 >= 33 && v11 <= 122 )
      {
        v13 = (uint16_t)v9++;
        v27[v13] = v11 - 33;
      }
      ++v8;
    }
  }
  if ( (uint16_t)v9 != 0 )
  {
    v14 = v27;
    for ( i = (uint16_t)v9; ; --i )
    {
      v15 = *v14;
      if ( *v14 == 255 )
      {
        v17 = 0;
      }
      else
      {
        v16 = *(((intptr_t *)c) + a4 + 683);
        if ( **(_DWORD **)((uint8_t *)(v16) + 0x0) == 0 )
          W_fl_4184a0((intptr_t)(v16), (intptr_t)(0));
        v17 = F32((**(_DWORD **)((uint8_t *)(v16) + 0x0) + 4 * v15), 0);
        if ( v17 != 0 && a2 != 0 )
        {
          if ( a8 != 0 )
          {
            v18 = -F32(v17, 12);
            (*v25__f) = v18;
          }
          else
          {
            v18 = (*v25__f);
          }
          v19 = (*v24__f);
          W_rp_5b9b70((intptr_t)(v17), (intptr_t)(a2), (intptr_t)(a6 + (*v24__f) + v18), (intptr_t)(a7));
          goto LABEL_23;
        }
      }
      v19 = (*v24__f);
LABEL_23:
      if ( a8 != 0 )
      {
        if ( v17 != 0 )
          v20 = F32(v17, 20);
        else
          v20 = 60 * a5 / 100;
      }
      else
      {
        v20 = a5;
      }
      v21 = v20 + v19;
      ++v14;
      v22 = i == 1;
      (*v24__f) = v21;
      if ( v22 )
        return v21;
    }
  }
  return 0;
}

/* 0x495fe0: signed int __thiscall sub_495FE0( cls_495FE0 *this, int a2, unsigned __int16 a3, _DWORD *a4, int a5, unsigned __int16 a6) */
int32_t rd_495fe0(wl_char *self, int32_t a2, uint16_t a3, int32_t * a4, int32_t a5, uint16_t a6)
{
    intptr_t c = (intptr_t)self;
    int32_t ** v6 = 0;
    int32_t * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t result = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;

  v6 = *(_DWORD ***)&SYSB(2964);
  v7 = (_DWORD *)(F32(c, 0x40) + 660 * a3);
  if ( ***(_DWORD ***)&SYSB(2964) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2964)), (intptr_t)(0));
  v8 = F32((**v6 + 4 * a6), 0);
  v9 = v7[1] / 100 + v7[3] / 2 / 100 - (a4[13] + a4[24]) / 100 - 32;
  result = v7[4] / 100 - (a4[19] + a4[23] + 100 * a4[29]) / 100;
  v11 = v7[2] / 100 + result;
  if ( v9 + F32(v8, 28) >= 0 )
  {
    result = v11 + F32(v8, 32);
    if ( result >= 0 )
    {
      result = (unsigned int)((uint64_t)(1374389535LL * (int)a4[25]) >> 32) >> 31;
      if ( a4[25] / 100 > v9 )
      {
        v12 = a4[26];
        result = (unsigned int)((uint64_t)(1374389535LL * v12) >> 32) >> 31;
        if ( v12 / 100 > v11 )
        {
          if ( a5 != 0 )
            return W_rp_5bd550((intptr_t)(a5), (intptr_t)((intptr_t)a2), (intptr_t)(v8), (intptr_t)(v9 + F32(v8, 12)), (intptr_t)(v11 + F32(v8, 16)), (intptr_t)(F32(v8, 20)), (intptr_t)(F32(v8, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v8, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
          else
            return W_rp_5b9b70((intptr_t)(v8), (intptr_t)(a2), (intptr_t)(v9), (intptr_t)(v11));
        }
      }
    }
  }
  return result;
}

/* 0x496170: int __thiscall sub_496170(cls_496170 *this) */
int32_t rd_496170(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = F32(c, 0x14898);
  if ( F32(c, 0x1d4) == 50010 && F16(F32(c, 0x750), 1126) == 0 )
    return 2;
  return result;
}

/* 0x4961a0: int __thiscall sub_4961A0(cls_4961A0 *this) */
int32_t rd_4961a0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t v1 = 0;
    int32_t * v2 = 0;

  v1 = 0;
  while ( 1 )
  {
    v2 = F32(c, 4 * (v1 + 4108));
    if ( v2[625] == 1 && v2[636] != 100000104 && v2[2] != 0 )
      break;
    if ( ++v1 >= 8u )
      return 0;
  }
  return 1;
}

/* 0x4961e0: int __thiscall sub_4961E0(cls_4961E0 *this, int a2, int a3) */
int32_t rd_4961e0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    int32_t * v4 = 0;

  if ( a2 == 0 )
    return F32(c, 0x4054);
  v3 = 0;
  while ( 1 )
  {
    v4 = *(_DWORD **)&GAP(c, 0x0, 4 * v3 + 16432);
    if ( (v4[625] == 1 || v4[625] == 2) && a2 == v4[636] )
      break;
    if ( ++v3 >= 8u )
      return F32(c, 0x4054);
  }
  if ( a3 != 0 && v4[640] == 255 )
    return F32(c, 0x4054);
  else
    return v4[640];
}

/* 0x496240: int __thiscall sub_496240(cls_496240 *this, int a2, int a3) */
int32_t rd_496240(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    int32_t * v4 = 0;

  if ( a2 == 0 )
    return F32(c, 0x4050);
  v3 = 0;
  while ( 1 )
  {
    v4 = *(_DWORD **)&GAP(c, 0x0, 4 * v3 + 16432);
    if ( (v4[625] == 1 || v4[625] == 2) && a2 == v4[636] )
      break;
    if ( ++v3 >= 8u )
      return F32(c, 0x4050);
  }
  if ( a3 != 0 && v4[639] == 255 )
    return F32(c, 0x4050);
  else
    return v4[639];
}

/* 0x4962a0: int __stdcall sub_4962A0(int a1, unsigned __int16 a2, unsigned __int16 a3, int a4, int a5) */
int32_t rd_4962a0(int32_t a1, uint16_t a2, uint16_t a3, int32_t a4, int32_t a5)
{

    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  v5 = SYS32(2960);
  v6 = SYS32(2928);
  v7 = 32 * a2 - 200 * a5 / 1000 + 440;
  v8 = ((1000 - a4) << 7) / 1000 + 444;
  if ( ***(_DWORD ***)&SYSB(2960) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2960)), (intptr_t)(0));
  W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)v5, 48)), (intptr_t)(a1), (intptr_t)(v7), (intptr_t)(v8));
  if ( a3 != 0 )
  {
    if ( **(_DWORD **)((uint8_t *)(v6) + 0x0) == 0 )
      W_fl_4184a0((intptr_t)(v6), (intptr_t)(0));
    W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)((uint8_t *)(v6) + 0x0), 4 * a3)), (intptr_t)(a1), (intptr_t)(v7), (intptr_t)(v8));
  }
  if ( **(_DWORD **)((uint8_t *)(v6) + 0x0) == 0 )
    W_fl_4184a0((intptr_t)(v6), (intptr_t)(0));
  return W_rp_5b9b70((intptr_t)(F32((**(_DWORD **)((uint8_t *)(v6) + 0x0) + 4 * (uint16_t)(a2 + 4)), 0)), (intptr_t)(a1), (intptr_t)(v7), (intptr_t)(v8));
}

/* 0x4963a0: void __thiscall sub_4963A0(cls_4963A0 *this, int a2, int a3, int a4, int a5, int a6) */
void rd_4963a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint32_t v13 = 0;
    intptr_t v14 = 0;
    _Alignas(8) uint8_t frame_[0x28 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [40] at ebp-0x28 */

  if ( a3 != 0 && (a4 < 2 || a4 > 3) )
  {
    v6 = 8;
    v7 = 100 * (1000 - a3) / 1000 + 456;
    if ( a6 != 0 )
      v6 = 136;
    v8 = F32(c, 0x3d4);
    v9 = F32(a5, 16484);
    if ( v9 == 10 )
      v10 = v8 / 100;
    else
      v10 = v8;
    if ( v10 <= 999999 )
    {
      if ( v9 == 10 )
        v11 = v8 / 100;
      else
        v11 = v8;
    }
    else
    {
      v11 = 999999;
    }
    v12 = rd_417c40(((intptr_t)g_eng.sys), 0x34u, v9 != 10 ? 0 : 2);
    if ( v12 != 0 )
      W_rp_5b9b70((intptr_t)(v12), (intptr_t)(a2), (intptr_t)(v6), (intptr_t)(100 * (1000 - a3) / 1000 + 444));
    g_eng.h5bf3ee(Buffer, D(0x8554f8), v11);
    rd_491820((intptr_t *)((intptr_t)g_eng.sys), (intptr_t)a2, Buffer, 1u, 9, v6 + 32, v7, 0, 5, 0, 0);
    if ( F16(c, 0xc) != 0 )
    {
      if ( strlen(Buffer) >= 4 )
        v13 = strlen(Buffer);
      else
        v13 = 4;
      g_eng.h5bf3ee(Buffer, D(0x8554f8), F16(c, 0xc));
      rd_491820((intptr_t *)((intptr_t)g_eng.sys), (intptr_t)a2, Buffer, 1u, 9, v6 + 8 * v13 + 72, v7, 0, 5, 0, 0);
      v14 = rd_417c40(((intptr_t)g_eng.sys), 0x31u, 0x4Au);
      if ( v14 != 0 )
        W_rp_5b9b70((intptr_t)(v14), (intptr_t)(a2), (intptr_t)(v6 + 8 * v13 + 44), (intptr_t)(100 * (1000 - a3) / 1000 + 448));
    }
  }
}

/* 0x496560: unsigned int __thiscall sub_496560( cls_496560 *this, paint_ctx *a2, _DWORD *a3, unsigned __int16 a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, __int16 a12, int *a13) */
int32_t rd_496560(wl_char *self, int32_t a2, int32_t * a3, uint16_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int16_t a12, int32_t * a13)
{
    intptr_t c = (intptr_t)self;
    intptr_t v13 = 0;
    uint32_t result = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    uint16_t v29 = 0;
    intptr_t v30 = 0;
    _Alignas(8) uint8_t frame_[0x24c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *String = (intptr_t *)(frame_ + 0x24);   /* [10] at ebp-0x228 */
    char *Buffer = (char *)(frame_ + 0x4c);   /* [512] at ebp-0x200 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x24c */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x23c */
    intptr_t *const c_l__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x238 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x234 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x230 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x22c */

  (*v35__f) = c;
  v13 = 96 * (1000 - a6);
  result = (unsigned int)((uint64_t)(274877907LL * v13) >> 32) >> 31;
  v15 = v13 / 1000 + 20 * (a4 - a5 + 24);
  (*v36__f) = v15;
  if ( a6 != 0 )
  {
    v16 = a3[20];
    if ( v16 != 0 )
    {
      SETLO((*c_l__f), F16(v16, 20));
      (*v34__f) = SYS32(3008);
      if ( ***(_DWORD ***)&SYSB(3008) == 0 )
        W_fl_4184a0((intptr_t)(SYS32(3008)), (intptr_t)(0));
      v17 = a2;
      W_rp_5b9a40((intptr_t)(F32((**(_DWORD **)(*v34__f) + 4 * (uint16_t)(*c_l__f)), 0)), (intptr_t)((int)a2), (intptr_t)(a7 != 3 ? 22 : 362), (intptr_t)(v15 - 2));
    }
    else
    {
      v17 = a2;
    }
    v18 = SYS32(2916);
    (*c_l__f) = SYS32(2916);
    if ( ***(_DWORD ***)&SYSB(2916) == 0 )
    {
      W_fl_4184a0((intptr_t)(SYS32(2916)), (intptr_t)(0));
      v18 = (*c_l__f);
    }
    W_rp_5b9a40((intptr_t)(F32(**(_DWORD **)v18, 4)), (intptr_t)((int)v17), (intptr_t)(a7 != 3 ? 68 : 408), (intptr_t)(v15));
    if ( 1000 * *a13 / 1000 >= 300 || (v19 = 12, F32(((*v35__f) + (244) * 0x4), 0) / 4 % 2 != 0) )
      v19 = 8;
    rd_498f10((int)v17, a7 != 3 ? 74 : 414, v15 + 2, 2, v19, 0, *a13, 1000, 56, 0x2Eu, 0, 1);
    rd_498f10((int)v17, a7 != 3 ? 72 : 412, v15 + 9, 2, 10, 0, a13[1], 1000, 56, 0x2Eu, 0, 1);
    v20 = a3[22];
    v21 = a3[33] + a3[39];
    v22 = 1;
    if ( v21 + v20 >= 1 )
      v22 = v21 + v20;
    if ( *a13 == 1000 * a3[21] / v22 )
      v23 = a3[21];
    else
      v23 = *a13 * (v21 + v20) / 1000;
    g_eng.h5bf3ee(Buffer, "%% %dd", 4);
    g_eng.h5bf3ee((char *const)String, Buffer, v23);
    (*c_l__f) = strlen((const char *)String);
    if ( W_rp_5b94e0((intptr_t)(v17), (intptr_t)((intptr_t *)&(*hdc__f))) != 0 )
    {
      API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
      API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)&SYSB(10008)));
      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x202020u));
      (*v34__f) = -1;
      do
      {
        v24 = -1;
        v25 = (*v34__f) + (*v36__f) - 3;
        do
        {
          API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v24 + (a7 != 3 ? 20 : 360) + 88), (intptr_t)(v25), (intptr_t)((char *)String), (intptr_t)((*c_l__f)));
          ++v24;
        }
        while ( v24 <= 1 );
        ++(*v34__f);
      }
      while ( (*v34__f) <= 1 );
      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
      API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(a7 != 3 ? 108 : 448), (intptr_t)((*v36__f) - 3), (intptr_t)((char *)String), (intptr_t)((*c_l__f)));
      W_rp_5b9500((intptr_t)(a2), (intptr_t)((intptr_t)(*hdc__f)));
      v17 = a2;
    }
    v26 = a13[1];
    if ( v26 == W_stats_mp_ratio((intptr_t)(a3)) )
      v27 = a3[23];
    else
      v27 = v26 * (a3[24] + a3[34] + a3[40]) / 1000;
    rd_495dc0(v27, 4u, (char *)String);
    v28 = (*v36__f);
    rd_43e250(1u, v17, String, a7 != 3 ? 102 : 442, (*v36__f) + 5, 0);
    result = 0;
    if ( a9 != 0 )
    {
      if ( a10 == 0 )
        return result;
      v29 = 64;
    }
    else if ( a12 != 0 )
    {
      v29 = 65;
    }
    else
    {
      v29 = 66;
      if ( a11 == 0 )
        v29 = 67;
      result = -5;
    }
    (*v31__f) = v28 + result;
    v30 = rd_418470(SYS32(2928), v29);
    return W_rp_5b9b70((intptr_t)(v30), (intptr_t)((int)v17), (intptr_t)(a7 != 3 ? -12 : 328), (intptr_t)((*v31__f)));
  }
  return result;
}

/* 0x498f10: int __stdcall sub_498F10( paint_ctx *a1, int a2, int y1, int a4, int a5, int a6, cls_5B9AE0 *a7, cls_5B9AE0 *a8, int a9, unsigned __int16 a10, unsigned __int16 a11, int a12) */
int32_t rd_498f10(int32_t a1, int32_t a2, int32_t y1, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, uint16_t a10, uint16_t a11, int32_t a12)
{

    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t result = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;

  v12 = (int)a8;
  if ( (int)a8 <= 0 )
    v12 = 1;
  v21 = a9 * a6 / v12;
  v13 = a9 * (int)a7 / v12 - v21;
  v14 = SYS32(4 * a10 + 2732);
  if ( **(_DWORD **)((uint8_t *)(v14) + 0x0) == 0 )
    W_fl_4184a0((intptr_t)(v14), (intptr_t)(0));
  v15 = SYS32(4 * a10 + 2732);
  v23 = F32(**(_DWORD **)((uint8_t *)(v14) + 0x0), 4 * a11);
  if ( **(_DWORD **)((uint8_t *)(v15) + 0x0) == 0 )
    W_fl_4184a0((intptr_t)(v15), (intptr_t)(0));
  v24 = ***(intptr_t ***)((uint8_t *)(v15) + 0x0);
  result = a4;
  if ( (uint16_t)a4 != 0 )
  {
    v18 = a9 - v13;
    v19 = v13 + a2;
    v20 = v21 - v13;
    v25 = v21 - v13;
    v22 = (uint16_t)a4;
    while ( 1 )
    {
      W_rp_5b9ae0((intptr_t)(v23), (intptr_t)((int)a1), (intptr_t)(v19 + v20), (intptr_t)(y1), (intptr_t)(v13), (intptr_t)(2), (intptr_t)(v18), (intptr_t)(a5), (intptr_t)(v13), (intptr_t)(2));
      if ( a12 != 0 )
      {
        if ( F32(CFG(), 148) == 2 )
          W_rp_5bd550((intptr_t)(SYS32(10184)), (intptr_t)(a1), (intptr_t)((int)v24), (intptr_t)(v19), (intptr_t)(y1), (intptr_t)(v18), (intptr_t)(2), (intptr_t)(0), (intptr_t)(14), (intptr_t)(*(_DWORD *)&GAP(v24, 0x0, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
        else
          W_rp_5b9ae0((intptr_t)(v24), (intptr_t)((int)a1), (intptr_t)(v19), (intptr_t)(y1), (intptr_t)(v18), (intptr_t)(2), (intptr_t)(0), (intptr_t)(14), (intptr_t)(v18), (intptr_t)(2));
      }
      y1 += 2;
      --v19;
      result = --v22;
      if ( v22 == 0 )
        break;
      v20 = v25;
    }
  }
  return result;
}

/* 0x4969b0: int __stdcall sub_4969B0(paint_ctx *a1, cls_4022A0 *a2, cls_4766A0 *a3, cls_4022A0 *a4) */
int32_t rd_4969b0(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{

    intptr_t v4 = 0;
    intptr_t result = 0;
    intptr_t v6 = 0;
    uint8_t * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    int32_t * v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    int32_t * v19 = 0;
    int64_t v20 = 0;
    int64_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    uint16_t v29 = 0;
    int64_t v30 = 0;
    intptr_t v31 = 0;
    uint16_t * v32 = 0;
    uint32_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    int32_t ** v36 = 0;
    intptr_t v37 = 0;
    _Alignas(8) uint8_t frame_[0x10c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v48 = (intptr_t *)(frame_ + 0x1c);   /* [60] at ebp-0xf0 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x10c */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x108 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x108 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x108 */
    int16_t *const v42__f = (int16_t *)(frame_ + 0x8);   /* ebp-0x104 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x100 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x100 */
    uint16_t *const v45__f = (uint16_t *)(frame_ + 0x10);   /* ebp-0xfc */
    intptr_t *const v46__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xf8 */
    intptr_t *const v47__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0xf4 */

  v4 = a2;
  result = 0;
  (*v45__f) = 0;
  if ( a2 != 0 )
  {
    (*v46__f) = 0;
    while ( 1 )
    {
      v6 = *(intptr_t *)&GAP(W, 0x1030, 4 * (uint16_t)result + 304);
      (*v47__f) = v6;
      if ( F32(v6, 0x1d0) != 1 )
        goto LABEL_65;
      v7 = (_BYTE *)F32(v6, 0x40);
      if ( *v7 != 1 )
        goto LABEL_65;
      if ( v7[126] != 0 )
        goto LABEL_65;
      if ( F16(F32(v6, 0x750), 1098) != 0 )
        goto LABEL_65;
      v8 = F32(v4, 0x1dc);
      v9 = F32(v6, 0x1dc);
      if ( v8 == 0 || v9 == 0 || v8 != 3 && v9 != 3 && v8 == v9 )
        goto LABEL_65;
      v10 = abs32(F32(F32(v4, 0x40), 4) + F32(F32(v4, 0x40), 12) / 2 - W_part_center_x((intptr_t)(v6), (intptr_t)(0)));
      v11 = W_part_center_y((intptr_t)(v6), (intptr_t)(0));
      v12 = abs32(W_part_center_y((intptr_t)(v4), (intptr_t)(0)) - v11);
      if ( v10 >= 72000
        || v12 >= 56000
        || W_view_box_visible((intptr_t)(a3), (intptr_t)(F32(F32(v6, 0x40), 4)), (intptr_t)(F32(F32(v6, 0x40), 8)), (intptr_t)(F32(F32(v6, 0x40), 12)), (intptr_t)(F32(F32(v6, 0x40), 16)), (intptr_t)(0)) )
      {
        goto LABEL_65;
      }
      v13 = (_DWORD *)F32(v6, 0x40);
      v14 = v13[3];
      (*v43__f) = v13[1];
      v15 = v13[5];
      v16 = v13[4] - v15;
      (*v39__f) = ((*v43__f) + v14 / 2) / 100 - (F32(a3, 0x34) + F32(a3, 0x60)) / 100;
      v17 = v13[2];
      v18 = v16 / 2;
      v19 = (_DWORD *)F32(a2, 0x40);
      (*v38__f) = (v15 + v17 + v16 / 2) / 100 - (F32(a3, 0x4c) + F32(a3, 0x5c) + 100 * F32(a3, 0x74)) / 100;
      v20 = v14 / 2 + (*v43__f) - v19[3] / 2 - v19[1];
      SETHIDW(v20, (int)((uint64_t)(274877907LL * (int)(1000 * (72000 - ((HIDWORD(v20) ^ v20) - HIDWORD(v20))))) >> 32) >> 11);
      (*v44__f) = (HIDWORD(v20) >> 31) + HIDWORD(v20);
      v21 = v15 + v18 + v17 - (v19[4] - v19[5]) / 2 - v19[2] - v19[5];
      v22 = (HIDWORD(v21) ^ v21) - HIDWORD(v21);
      if ( (*v44__f) >= 1000 * (56000 - v22) / 24000 )
        (*v44__f) = 1000 * (56000 - v22) / 24000;
      v23 = (*v39__f);
      v24 = F32(a3, 0x64) / 100;
      v25 = v24;
      if ( v24 >= (*v39__f) )
        v25 = (*v39__f);
      if ( v25 >= 0 )
      {
        if ( v24 >= (*v39__f) )
          goto LABEL_25;
        (*v40__f) = F32(a3, 0x64) / 100;
      }
      else
      {
        (*v40__f) = 0;
      }
      v23 = (*v40__f);
LABEL_25:
      v26 = (*v38__f);
      v27 = F32(a3, 0x68) / 100;
      v28 = v27;
      if ( v27 >= (*v38__f) )
        v28 = (*v38__f);
      v29 = 0;
      if ( v28 < 0 )
      {
        (*v38__f) = 0;
LABEL_31:
        v26 = (*v38__f);
        goto LABEL_32;
      }
      if ( v27 < (*v38__f) )
      {
        (*v38__f) = F32(a3, 0x68) / 100;
        goto LABEL_31;
      }
LABEL_32:
      (*v42__f) = 0;
      if ( v26 == v27 )
        (*v42__f) = 2;
      if ( v23 == v24 )
        (*v42__f) = 1;
      if ( v23 == 0 )
        (*v42__f) = 3;
      if ( (*v45__f) == 0 )
        goto LABEL_45;
      do
      {
        v30 = v48[3 * v29] - v23;
        result = (HIDWORD(v30) ^ v30) - HIDWORD(v30);
        if ( result <= 4 )
        {
          result = abs32(v48[3 * v29 + 1] - v26);
          if ( result <= 4 )
            break;
        }
        ++v29;
      }
      while ( v29 < (*v45__f) );
      if ( v29 >= 0x14u )
        return result;
      if ( v29 < (*v45__f) )
      {
        v31 = 3 * v29;
        SETLO(v48[v31 + 2], (uint16_t)(v48[v31 + 2]) + 1);
        v32 = (uint16_t *)&v48[v31 + 2];
      }
      else
      {
LABEL_45:
        v31 = 3 * v29;
        v32 = (uint16_t *)&v48[v31 + 2];
        ++(*v45__f);
        SETLO(v48[v31 + 2], 0);
      }
      v48[v31] = v23;
      v48[v31 + 1] = v26;
      v33 = *v32;
      if ( v33 <= 4 )
      {
        switch ( (*v42__f) )
        {
          case 0:
            (*v38__f) = v26 + 12 * v33;
            v26 = (*v38__f);
            break;
          case 1:
            (*v41__f) = v23 - 12 * v33;
            goto LABEL_52;
          case 2:
            (*v38__f) = v26 - 12 * v33;
            v26 = (*v38__f);
            break;
          case 3:
            (*v41__f) = v23 + 12 * v33;
LABEL_52:
            v23 = (*v41__f);
            break;
        }
        if ( a4 == (*v47__f) )
          (*v42__f) += 4;
        v34 = 20 * (*v44__f) / 1000;
        if ( v34 < 0 )
        {
          v34 = 0;
          goto LABEL_57;
        }
        if ( v34 < 20 )
LABEL_57:
          v35 = SYS32(4 * v34 + 10152);
        else
          v35 = 0;
        v36 = *(_DWORD ***)&SYSB(2964);
        if ( ***(_DWORD ***)&SYSB(2964) == 0 )
        {
          W_fl_4184a0((intptr_t)(SYS32(2964)), (intptr_t)(0));
          v26 = (*v38__f);
        }
        v37 = F32((**v36 + 4 * (uint16_t)((*v42__f) + 4)), 0);
        if ( v35 != 0 )
          W_rp_5bd550((intptr_t)(v35), (intptr_t)(a1), (intptr_t)(v37), (intptr_t)(F32(v37, 12) + v23 - 32), (intptr_t)(F32(v37, 16) + v26 - 32), (intptr_t)(F32(v37, 20)), (intptr_t)(F32(v37, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v37, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
        else
          W_rp_5b9b70((intptr_t)(v37), (intptr_t)((int)a1), (intptr_t)(v23 - 32), (intptr_t)(v26 - 32));
      }
LABEL_65:
      result = ++(*v46__f);
      if ( (uint16_t)(*v46__f) >= 0x20u )
        return result;
      v4 = a2;
    }
  }
  return result;
}

/* 0x496ec0: int __thiscall sub_496EC0(cls_496EC0 *this, paint_ctx *a2, int a3, int a4, int a5) */
int32_t rd_496ec0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int32_t ** v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    int32_t * v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    uint32_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    char * v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    intptr_t v33 = 0;
    _Alignas(8) uint8_t frame_[0x21c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *String = (char *)(frame_ + 0x1c);   /* [512] at ebp-0x200 */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x21c */
    intptr_t *const c_l__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x218 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x214 */
    intptr_t *const y__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x210 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x20c */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x208 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x204 */

  result = a3;
  (*v40__f) = c;
  if ( a3 == 0 )
    return result;
  if ( a4 > 0 && a4 <= 3 )
  {
    (*v39__f) = 144;
    (*v38__f) = 32 * (3 * a3 - 3000) / 1000 + 8;
  }
  v6 = F32(c, 0x3dc);
  if ( v6 / 500 != 0 )
  {
    if ( v6 / 500 == 1 )
    {
      (*hdc__f) = (intptr_t)6;
      (*c_l__f) = 5;
    }
    else if ( v6 / 500 == 2 )
    {
      (*hdc__f) = (intptr_t)7;
      (*c_l__f) = 6;
    }
    else
    {
      (*hdc__f) = (intptr_t)8;
      (*c_l__f) = 7;
    }
  }
  else
  {
    (*hdc__f) = (intptr_t)5;
    (*c_l__f) = 0;
  }
  if ( F32(c, 0x3d0) / 4 % 2 == 0 )
  {
    if ( F32(F32(c, 0x4), 16612) <= 600 && v6 < 2000 )
      goto LABEL_17;
    (*hdc__f) = (intptr_t)3;
  }
  if ( v6 >= 2000 )
  {
    v7 = 500;
    goto LABEL_19;
  }
LABEL_17:
  v7 = v6 % 500;
LABEL_19:
  (*y__f) = (*v38__f) + 10;
  v8 = v7 < 0 ? 0 : v7;
  if ( v8 > 500 )
    v8 = 500;
  v9 = *(_DWORD ***)&SYSB(2924);
  v10 = 200 * v8 / 500;
  v11 = 200 - v10;
  if ( ***(_DWORD ***)&SYSB(2924) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2924)), (intptr_t)(0));
  (*v36__f) = F32((**v9 + 4 * (uint16_t)(*hdc__f)), 0);
  (*hdc__f) = (intptr_t)rd_418470(SYS32(2924), 1u);
  W_rp_5b9ae0((intptr_t)((*v36__f)), (intptr_t)((int)a2), (intptr_t)((*y__f)), (intptr_t)((*v39__f) + 4 - v10 + 200), (intptr_t)(10), (intptr_t)(v10), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10), (intptr_t)(v10));
  if ( (uint16_t)(*c_l__f) != 0 )
  {
    v12 = rd_418470(SYS32(2924), (*c_l__f));
    W_rp_5b9ae0((intptr_t)(v12), (intptr_t)((int)a2), (intptr_t)((*v38__f) + 10), (intptr_t)((*v39__f) + 4), (intptr_t)(10), (intptr_t)(v11), (intptr_t)(0), (intptr_t)(0), (intptr_t)(10), (intptr_t)(v11));
  }
  else if ( F32(CFG(), 148) == 2 )
  {
    rd_4975a0(
      (intptr_t *)((intptr_t)g_eng.sys),
      (int)a2,
      (int)(*hdc__f),
      SYS32(10184),
      (*v38__f) + 10,
      (*v39__f) + 4,
      10,
      v11,
      0,
      0,
      F32((*hdc__f), 4 * 10));
  }
  v13 = F32(F32((*v40__f), 0x4), 16612) < 0 ? 0 : F32(F32((*v40__f), 0x4), 16612);
  if ( v13 > 1000 )
    v13 = 1000;
  v14 = SYS32(2924);
  v15 = 200 * v13 / 1000;
  v16 = **(_DWORD ***)&SYSB(2924);
  (*c_l__f) = (*v39__f) + 4;
  (*v36__f) = (200 - v15);
  if ( *v16 == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2924)), (intptr_t)(0));
  (*y__f) = F32(**(_DWORD **)v14, 16);
  v17 = rd_418470(SYS32(2924), 1u);
  W_rp_5b9ae0((intptr_t)((*y__f)), (intptr_t)((int)a2), (intptr_t)((*v38__f) + 4), (intptr_t)((*v39__f) + 4 - v15 + 200), (intptr_t)(3), (intptr_t)(v15), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3), (intptr_t)(v15));
  if ( F32(CFG(), 148) == 2 )
    rd_4975a0(
      (intptr_t *)((intptr_t)g_eng.sys),
      (int)a2,
      v17,
      SYS32(10184),
      (*v38__f) + 4,
      (*c_l__f),
      3,
      (int)(*v36__f),
      0,
      0,
      F32(v17, 40));
  v18 = (*v40__f);
  v19 = F32((*v40__f) + (1) * 0x3e0, 0x4);
  if ( v19 > 0 )
  {
    if ( v19 >= 2000 )
      v20 = 500;
    else
      v20 = v19 % 500;
    v21 = SYS32(2928);
    v22 = (int)((uint64_t)(54975581400LL * v20) >> 32) >> 5;
    v23 = ***(_DWORD ***)&SYSB(2928);
    v24 = ((*v39__f) - ((v22 >> 31) + v22) + 200);
    (*v36__f) = v24;
    if ( v23 == 0 )
      W_fl_4184a0((intptr_t)(SYS32(2928)), (intptr_t)(0));
    W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)v21, 200)), (intptr_t)((int)a2), (intptr_t)((*v38__f) + 7), (intptr_t)((int)v24));
    g_eng.h5bf3ee(String, "%d%%", F32((*v40__f) + (1) * 0x3e0, 0x4) / 10);
    v25 = strlen(String);
    v26 = (*v38__f) + 24;
    if ( W_rp_5b94e0((intptr_t)(a2), (intptr_t)((intptr_t *)&(*hdc__f))) != 0 )
    {
      API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
      API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)&SYSB(10012)));
      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x202020u));
      (*c_l__f) = -1;
      do
      {
        v27 = -1;
        (*y__f) = (int)&GAP((*v36__f), 0x0, (*c_l__f) + 6);
        do
        {
          API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v27 + v26), (intptr_t)((*y__f)), (intptr_t)(String), (intptr_t)(v25));
          ++v27;
        }
        while ( v27 <= 1 );
        ++(*c_l__f);
      }
      while ( (*c_l__f) <= 1 );
      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
      API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v26), (intptr_t)((int)&GAP((*v36__f), 0x0, 6)), (intptr_t)(String), (intptr_t)(v25));
      W_rp_5b9500((intptr_t)(a2), (intptr_t)((intptr_t)(*hdc__f)));
      v24 = (*v36__f);
    }
    g_eng.h5bf3ee(String, "%dHit", *(_DWORD *)((uint8_t *)((*v40__f) + (1) * 0x3e0) + 0x8));
    (*c_l__f) = strlen(String);
    (*y__f) = (int)&GAP((v24 + (-1) * 0xd8), 0x30, 160);
    if ( W_rp_5b94e0((intptr_t)(a2), (intptr_t)((intptr_t *)&(*hdc__f))) != 0 )
    {
      API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
      API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)&SYSB(10012)));
      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x202020u));
      (*v36__f) = -1;
      do
      {
        v28 = -1;
        v29 = &GAP((*v36__f), 0x0, (*y__f));
        do
        {
          API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v28 + v26), (intptr_t)((int)v29), (intptr_t)(String), (intptr_t)((*c_l__f)));
          ++v28;
        }
        while ( v28 <= 1 );
        (*v36__f) = ((uint8_t *)(*v36__f) + 1);
      }
      while ( (int)(*v36__f) <= 1 );
      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
      API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v26), (intptr_t)((*y__f)), (intptr_t)(String), (intptr_t)((*c_l__f)));
      W_rp_5b9500((intptr_t)(a2), (intptr_t)((intptr_t)(*hdc__f)));
    }
    v18 = (*v40__f);
  }
  (*y__f) = SYS32(2924);
  if ( ***(_DWORD ***)&SYSB(2924) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2924)), (intptr_t)(0));
  v30 = (*v39__f);
  v31 = (*v38__f);
  W_rp_5b9a40((intptr_t)(***(intptr_t ***)(*y__f)), (intptr_t)((int)a2), (intptr_t)((*v38__f)), (intptr_t)((*v39__f) - 16));
  g_eng.h5bf3ee(String, "%d", *(_DWORD *)((uint8_t *)(v18 + (1) * 0x3e0) + 0x0));
  v32 = rd_495e40(((intptr_t)g_eng.sys), (int)a2, String, 0xAu, 14, v31 - 6, v30 - 21, 1);
  rd_495e40(((intptr_t)g_eng.sys), (int)a2, (const char *)D(0x855508), 9u, 9, v32 + v31 - 6, v30 - 14, 1);
  if ( F32(v18, 0x3d0) / 4 % 2 == 0 && F32(F32(v18, 0x4), 16640) != 0 )
    rd_495e40(((intptr_t)g_eng.sys), (int)a2, ((char *)D(0x855504)), 0xBu, 9, v31 - 2, v30 - 34, 1);
  result = F32(v18, 0x4);
  v33 = F32(result, 16620);
  if ( v33 > 0 )
  {
    g_eng.h5bf3ee(String, "+%d%%", v33 / 10);
    return rd_495e40(((intptr_t)g_eng.sys), (int)a2, String, 1u, 9, v31, v30 + 208, 1);
  }
  return result;
}

/* 0x4975a0: int __thiscall sub_4975A0( HDC *this, paint_ctx *a2, int a3, cls_5BD550 *a4, int x1, int y1, int a7, int a8, int a9, int a10, int a11) */
int32_t rd_4975a0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t x1, int32_t y1, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11)
{
    intptr_t c = (intptr_t)self;

  return W_rp_5bd550((intptr_t)(a4), (intptr_t)(a2), (intptr_t)(a3), (intptr_t)(x1), (intptr_t)(y1), (intptr_t)(a7), (intptr_t)(a8), (intptr_t)(a9), (intptr_t)(a10), (intptr_t)(a11), (intptr_t)(*(((intptr_t *)c) + 216)));
}

/* 0x4975e0: void __thiscall sub_4975E0(cls_4975E0 *this, paint_ctx *a2, int a3, int a4, int a5) */
void rd_4975e0(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    uint16_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    int32_t * v23 = 0;
    int32_t * v24 = 0;
    intptr_t v25 = 0;
    intptr_t *** v26 = 0;
    _Alignas(8) uint8_t frame_[0x296 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *v33 = (uint16_t *)(frame_ + 0x16);   /* [320] at ebp-0x280 */
    uint16_t *const v27__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x296 */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x2);   /* ebp-0x294 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x6);   /* ebp-0x290 */
    int32_t * *const v30__f = (int32_t * *)(frame_ + 0xa);   /* ebp-0x28c */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0xe);   /* ebp-0x288 */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x12);   /* ebp-0x284 */

  v5 = c;
  (*v32__f) = c;
  if ( a4 == 0 )
    return;
  if ( a5 <= 0 )
    goto LABEL_7;
  if ( a5 > 2 )
  {
    if ( a5 == 3 )
      return;
LABEL_7:
    v7 = (*v28__f);
    v6 = (*v29__f);
    goto LABEL_8;
  }
  v6 = 412;
  (*v29__f) = 412;
  v7 = 216 * (1000 - a4) / 1000 + 440;
  (*v28__f) = v7;
LABEL_8:
  v8 = SYS32(3016);
  if ( ***(_DWORD ***)&SYSB(3016) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(3016)), (intptr_t)(0));
  v9 = F32(**(_DWORD **)v8, 12);
  if ( F32(CFG(), 148) == 2 )
    W_rp_5bd550((intptr_t)(SYS32(10192)), (intptr_t)(a2), (intptr_t)(v9), (intptr_t)(F32(v9, 12) + v7 + 96), (intptr_t)(v6 + F32(v9, 16)), (intptr_t)(F32(v9, 20)), (intptr_t)(F32(v9, 24)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v9, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
  else
    W_rp_5b9b70((intptr_t)(v9), (intptr_t)((int)a2), (intptr_t)(v7 + 96), (intptr_t)(v6));
  v10 = (int)F32(v5, (228) * 4);
  if ( 1000 * v10 / 1000 >= 300 || F32((v5 + (244) * 0x4), 0) / 4 % 2 != 0 )
    v11 = 0;
  else
    v11 = 6;
  rd_498f10((int)a2, v7 + 7, v6 + 54, 3, v11, 0, v10, 1000, 112, 0x2Eu, 0, 1);
  if ( a3 != 0 )
  {
    v12 = F32(a3, 64);
    if ( (F16(a3 + 20 * F16(a3, 232), 72) != 0 || F32(a3, 8) != 0)
      && F8(v12, 0) != 0
      && W_entity_sprite_descs((intptr_t)(a3), (intptr_t)(v12), (intptr_t)((int)v33), (intptr_t)(&(*v27__f))) != 0 )
    {
      (*v31__f) = 0;
      if ( (*v27__f) != 0 )
      {
        do
        {
          v13 = 10 * (uint16_t)(*v31__f);
          v14 = rd_418470(SYS32(4 * v33[v13 + 4] + 2732), v33[v13 + 5]);
          v15 = F32(a3, 88308);
          if ( F32(v12, 44) != 1 )
            v15 = -v15;
          (*v30__f) = (_DWORD *)v15;
          v16 = F32(a3, 40) / 2 / 100 - F32(&v33[v13], 0) - v15 - F32(v12, 64) - 26;
          v17 = F32(a3, 44) / 2 / 100
              - F32(&v33[v13 + 2], 0)
              - F32(a3, 88312)
              - F32(v12, 68)
              - 26;
          v18 = F32(v14, 28) - v16;
          v19 = F32(v14, 32) - v17;
          if ( v16 < 0 )
            v16 = 0;
          if ( v17 < 0 )
            v17 = 0;
          if ( v18 > 0 && v19 > 0 )
          {
            if ( v19 > 52 )
              v19 = 52;
            if ( v18 > 52 )
              v18 = 52;
            W_rp_5b9bf0((intptr_t)(v14), (intptr_t)((int)a2), (intptr_t)((*v28__f) + 136), (intptr_t)((*v29__f) + 8), (intptr_t)(v18), (intptr_t)(v19), (intptr_t)(v16), (intptr_t)(v17));
          }
          ++(*v31__f);
        }
        while ( (uint16_t)(*v31__f) < (*v27__f) );
        v7 = (*v28__f);
        v5 = (*v32__f);
      }
    }
    v20 = *(_WORD *)((uint8_t *)(v5 + (241) * 0x4) + 0x0);
    if ( v20 != 0 )
    {
      v21 = 0;
      if ( v20 < 0x14u )
        v21 = W_tint_fade_a((intptr_t)(v20), (intptr_t)(20));
      if ( v20 > 0x50u )
        v21 = W_tint_fade_a((intptr_t)(100 - v20), (intptr_t)(20));
      v22 = F32(a3, 64);
      v23 = *(_DWORD **)&GAP(W, 0x1030, 28);
      v24 = (_DWORD *)rd_418470(SYS32(2964), 2u);
      v25 = F32(v22, 16);
      (*v30__f) = v24;
      rd_497a00((int)a2, v22, v23, v24, F32(v22, 12) / 2 / 100 - 32, v25 / 100, v21);
      v7 = (*v28__f);
    }
    v5 = (*v32__f);
    v6 = (*v29__f);
  }
  v26 = SYS32(2980);
  if ( ***(_DWORD ***)&SYSB(2980) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2980)), (intptr_t)(0));
  W_rp_5b9b70((intptr_t)(***v26), (intptr_t)((int)a2), (intptr_t)(v7), (intptr_t)(v6));
  if ( GAP((v5 + (231) * 0x4), 0x0, 0) != 0 )
    rd_43e250(2u, a2, (intptr_t)(v5 + (231) * 0x4), v7 + 8, v6 + 40, 0);
}

/* 0x497a00: int __stdcall sub_497A00(paint_ctx *a1, int a2, _DWORD *a3, cls_5B9B70 *a4, int a5, int a6, cls_5BD550 *a7) */
int32_t rd_497a00(int32_t a1, int32_t a2, int32_t * a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{

    intptr_t v7 = 0;
    intptr_t v8 = 0;

  v7 = a5 + F32(a2, 4) / 100 - (a3[13] + a3[24]) / 100;
  v8 = a6 + F32(a2, 8) / 100 - (a3[19] + a3[23] + 100 * a3[29]) / 100;
  if ( v7 + *(_DWORD *)&GAP(a4, 0x14, 8) < 0
    || v8 + *(_DWORD *)&GAP(a4, 0x14, 12) < 0
    || a3[25] / 100 <= v7
    || a3[26] / 100 <= v8 )
  {
    return 0;
  }
  if ( a7 != 0 )
    W_rp_5bd550((intptr_t)(a7), (intptr_t)(a1), (intptr_t)((int)a4), (intptr_t)(v7 + F32(a4, 0xc)), (intptr_t)(v8 + F32(a4, 0x10)), (intptr_t)(F32(((uint8_t *)(a4) + 0x14), 0)), (intptr_t)(*(_DWORD *)&GAP(a4, 0x14, 4)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(*(_DWORD *)&GAP(a4, 0x14, 20)), (intptr_t)(*(intptr_t *)&SYSB(864)));
  else
    W_rp_5b9b70((intptr_t)(a4), (intptr_t)((int)a1), (intptr_t)(v7), (intptr_t)(v8));
  return 1;
}

/* 0x497b40: int __stdcall party_stat_hp_display_497B40(_DWORD *a1, int a2) */
int32_t rd_497b40(int32_t * a1, int32_t a2)
{

    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;

  v2 = 1;
  v3 = a1[33] + a1[39];
  v4 = a1[22];
  if ( v3 + v4 >= 1 )
    v2 = v3 + v4;
  if ( a2 == 1000 * a1[21] / v2 )
    return a1[21];
  else
    return a2 * (v3 + v4) / 1000;
}

/* 0x497bb0: int __stdcall party_stat_mp_display_497BB0(_DWORD *a1, int a2) */
int32_t rd_497bb0(int32_t * a1, int32_t a2)
{

    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;

  v2 = 1;
  v3 = a1[34] + a1[40];
  v4 = a1[24];
  if ( v3 + v4 >= 1 )
    v2 = v3 + v4;
  if ( a2 == 1000 * a1[23] / v2 )
    return a1[23];
  else
    return a2 * (v3 + v4) / 1000;
}

/* 0x497c20: void __stdcall sub_497C20(paint_ctx *a1, int a2, int a3, int a4) */
void rd_497c20(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{

    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t *** v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    int v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    _Alignas(8) uint8_t frame_[0x230 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v21 = (uint8_t *)(frame_ + 0x0);   /* [16] at ebp-0x230 */
    intptr_t *Buffer = (intptr_t *)(frame_ + 0x30);   /* [128] at ebp-0x200 */
    intptr_t *const x1__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x220 */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x21c */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x218 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x214 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x210 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x20c */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x28);   /* ebp-0x208 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x204 */

  v4 = 0;
  if ( a3 == 0 )
    return;
  if ( F16(a2, 16496) != 0 )
  {
    if ( F16(a2, 16496) == 1 )
    {
      if ( v21 != (_BYTE *)-48 )
        strcpy((char *)Buffer, ((char *)D(0x85551c)));
      goto LABEL_15;
    }
    if ( F16(a2, 16496) != 2 || v21 == (_BYTE *)-48 )
    {
LABEL_15:
      if ( v21 == (_BYTE *)-20 )
      {
        v4 = (*v23__f);
        (*v25__f) = 1;
      }
      else
      {
        v7 = F32(a2, 16500);
        (*v25__f) = 1;
        v4 = 1000 * v7 / 250;
      }
      goto LABEL_19;
    }
    if ( F16(a2, 16504) >= F16(a2, 16400) )
      v5 = 0;
    else
      v5 = F32(16 * F16(a2, 16504) + a2, 8220);
    v6 = F32(a2, 16508);
    if ( v5 != 0 )
    {
      if ( v6 != 0 )
        g_eng.h5bf3ee((char *const)Buffer, "%s-%s", (const char *)(v5 + 4), (const char *)(v6 + 2576));
      else
        g_eng.h5bf3ee((char *const)Buffer, "%s", (const char *)(v5 + 4));
      goto LABEL_15;
    }
  }
  (*v25__f) = 0;
LABEL_19:
  if ( a4 > 0 && a4 <= 3 )
  {
    (*v28__f) = 48;
    (*v23__f) = 280 * (a3 - 1000) / 1000 + 96;
  }
  v8 = 1000 - v4;
  v9 = (*v28__f) + 15;
  v10 = (*v23__f) + 34;
  v11 = SYS32(2916);
  v12 = 112 * v8 / 1000;
  v13 = ***(_DWORD ***)&SYSB(2916);
  (*v26__f) = v12;
  if ( v13 == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2916)), (intptr_t)(0));
  (*v29__f) = ***v11;
  v14 = rd_418470(SYS32(2916), 0);
  v15 = 112 - (*v26__f);
  (*v24__f) = v10;
  (*x1__f) = v10 + (*v26__f);
  (*v27__f) = 3;
  do
  {
    W_rp_5b9ae0((intptr_t)((*v29__f)), (intptr_t)((int)a1), (intptr_t)((*v24__f)), (intptr_t)(v9), (intptr_t)((*v26__f)), (intptr_t)(2), (intptr_t)(v15), (intptr_t)(4), (intptr_t)((*v26__f)), (intptr_t)(2));
    if ( F32(CFG(), 148) == 2 )
      rd_4975a0(
        (intptr_t *)((intptr_t)g_eng.sys),
        a1,
        v14,
        SYS32(10184),
        (*x1__f),
        v9,
        v15,
        2,
        0,
        14,
        F32(v14, 40));
    else
      W_rp_5b9ae0((intptr_t)(v14), (intptr_t)((int)a1), (intptr_t)((*x1__f)), (intptr_t)(v9), (intptr_t)(v15), (intptr_t)(2), (intptr_t)(0), (intptr_t)(14), (intptr_t)(v15), (intptr_t)(2));
    v9 += 2;
    v16 = (*v27__f) == 1;
    --(*v24__f);
    --(*x1__f);
    --(*v27__f);
  }
  while ( !v16 );
  v17 = SYS32(2916);
  if ( ***(_DWORD ***)&SYSB(2916) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2916)), (intptr_t)(0));
  v18 = (*v28__f);
  v19 = (*v23__f);
  W_rp_5b9a40((intptr_t)(F32(**(_DWORD **)v17, 8)), (intptr_t)((int)a1), (intptr_t)((*v23__f) + 28), (intptr_t)((*v28__f) + 12));
  v20 = SYS32(2928);
  if ( ***(_DWORD ***)&SYSB(2928) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2928)), (intptr_t)(0));
  W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)v20, 48)), (intptr_t)((int)a1), (intptr_t)(v19), (intptr_t)(v18));
  if ( (*v25__f) != 0 )
    rd_43e250(2u, a1, Buffer, v19 + 44, v18 + 8, 0);
}

/* 0x497f40: int __thiscall sub_497F40(cls_497F40 *this, paint_ctx *a2, int a3, int *a4, int a5) */
int32_t rd_497f40(wl_char *self, int32_t a2, int32_t a3, int32_t * a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    int32_t * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t *** v14 = 0;
    intptr_t *** v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    uint16_t v27 = 0;
    intptr_t v28 = 0;
    uint16_t * v29 = 0;
    uint16_t v30 = 0;
    intptr_t v31 = 0;
    int32_t * v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    _Alignas(8) uint8_t frame_[0x26c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *String = (intptr_t *)(frame_ + 0x1c);   /* [10] at ebp-0x250 */
    char *v46 = (char *)(frame_ + 0x44);   /* [40] at ebp-0x228 */
    char *Buffer = (char *)(frame_ + 0x6c);   /* [512] at ebp-0x200 */
    intptr_t *const v36__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x26c */
    uint32_t *const cb__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x268 */
    intptr_t *const ca__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x268 */
    intptr_t *const c_l__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x268 */
    intptr_t *const v40__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x264 */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x260 */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x25c */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x258 */
    intptr_t *const y__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x254 */

  (*v43__f) = c;
  result = *a4;
  v7 = F32(a3, 1872);
  if ( *a4 == 0 )
    return result;
  if ( a5 > 0 )
  {
    if ( a5 <= 2 )
    {
      v8 = 1;
      (*v40__f) = 1;
      (*v36__f) = 32 * (result + 2 * (5 * result - 5000) - 1000) / 1000 + 1;
      goto LABEL_8;
    }
    if ( a5 == 3 )
      return result;
  }
  v8 = (*v40__f);
LABEL_8:
  v9 = a4[2];
  if ( 1000 * v9 / 1000 >= 300 || F32(c, 0x3d0) / 4 % 2 != 0 )
    v10 = 0;
  else
    v10 = 6;
  rd_498f10((int)a2, (*v36__f) + 117, v8 + 6, 4, v10, 0, v9, 1000, 168, 0x2Fu, 0, 1);
  rd_498f10((int)a2, (*v36__f) + 117, v8 + 20, 3, 2, 0, a4[3], 1000, 168, 0x2Fu, 0, 1);
  v11 = v7[59];
  v12 = v7[58];
  if ( v11 <= 0 )
    v11 = 1;
  (*v42__f) = 0 / v11;
  v13 = 104 * v12 / v11 - 0 / v11;
  v14 = *(intptr_t ****)&SYSB(2916);
  if ( ***(_DWORD ***)&SYSB(2916) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2916)), (intptr_t)(0));
  (*hdc__f) = ***v14;
  (*c_l__f) = rd_418470(SYS32(2916), 0);
  W_rp_5b9ae0((intptr_t)((*hdc__f)), (intptr_t)((int)a2), (intptr_t)((*v42__f) + (*v36__f) + 143), (intptr_t)((*v40__f) + 41), (intptr_t)(v13), (intptr_t)(2), (intptr_t)(104 - v13), (intptr_t)(4), (intptr_t)(v13), (intptr_t)(2));
  if ( F32(CFG(), 148) == 2 )
    rd_4975a0(
      (intptr_t *)((intptr_t)g_eng.sys),
      a2,
      (*c_l__f),
      SYS32(10184),
      v13 + (*v36__f) + 143,
      (*v40__f) + 41,
      104 - v13,
      2,
      0,
      14,
      F32((*c_l__f), 40));
  else
    W_rp_5b9ae0((intptr_t)((*c_l__f)), (intptr_t)((int)a2), (intptr_t)(v13 + (*v36__f) + 143), (intptr_t)((*v40__f) + 41), (intptr_t)(104 - v13), (intptr_t)(2), (intptr_t)(0), (intptr_t)(14), (intptr_t)(104 - v13), (intptr_t)(2));
  v15 = SYS32(3012);
  if ( ***(_DWORD ***)&SYSB(3012) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(3012)), (intptr_t)(0));
  W_rp_5b9b70((intptr_t)(***v15), (intptr_t)((int)a2), (intptr_t)((*v36__f) - 32), (intptr_t)((*v40__f)));
  v16 = 0;
  if ( (int)v7[55] > 0 )
  {
    v17 = 0;
    do
    {
      v18 = rd_418470(SYS32(2928), (v17 + 1 <= v7[54]) + 16);
      W_rp_5b9b70((intptr_t)(v18), (intptr_t)((int)a2), (intptr_t)(v17 + (*v36__f) + 12 * v17 + 186), (intptr_t)((*v40__f) + 29));
      v17 = (uint16_t)++v16;
    }
    while ( (uint16_t)v16 < (int)v7[55] );
  }
  v19 = 1;
  v20 = v7[22] + v7[39] + v7[33];
  (*ca__f) = a4[2];
  if ( v20 >= 1 )
    v19 = v7[22] + v7[39] + v7[33];
  v21 = v7[21];
  if ( (*ca__f) != 1000 * v21 / v19 )
    v21 = (*ca__f) * v20 / 1000;
  g_eng.h5bf3ee(Buffer, "%% %dd", 4);
  g_eng.h5bf3ee(v46, Buffer, v21);
  g_eng.h5bf3ee((char *const)String, "%s / %d", v46, v7[22] + v7[39] + v7[33]);
  v22 = (*v36__f) + 217;
  (*cb__f) = strlen((const char *)String);
  if ( W_rp_5b94e0((intptr_t)(a2), (intptr_t)((intptr_t *)&(*hdc__f))) != 0 )
  {
    API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
    API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)&SYSB(10012)));
    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x202020u));
    (*v42__f) = -1;
    do
    {
      v23 = -1;
      (*y__f) = (*v40__f) + 4 + (*v42__f);
      do
      {
        API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v23 + v22), (intptr_t)((*y__f)), (intptr_t)((char *)String), (intptr_t)((*cb__f)));
        ++v23;
      }
      while ( v23 <= 2 );
      ++(*v42__f);
    }
    while ( (*v42__f) <= 1 );
    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
    API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v22), (intptr_t)((*v40__f) + 4), (intptr_t)((char *)String), (intptr_t)((*cb__f)));
    API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)((*v36__f) + 218), (intptr_t)((*v40__f) + 4), (intptr_t)((char *)String), (intptr_t)((*cb__f)));
    W_rp_5b9500((intptr_t)(a2), (intptr_t)((intptr_t)(*hdc__f)));
  }
  v24 = a4[3];
  if ( v24 == W_stats_mp_ratio((intptr_t)(v7)) )
    v25 = v7[23];
  else
    v25 = v24 * (v7[24] + v7[34] + v7[40]) / 1000;
  g_eng.h5bf3ee(Buffer, "%% %dd", 4);
  g_eng.h5bf3ee(v46, Buffer, v25);
  g_eng.h5bf3ee((char *const)String, "%s / %d", v46, v7[24] + v7[34] + v7[40]);
  v26 = (*v40__f);
  rd_43e250(2u, a2, String, v22, (*v40__f) + 17, 1);
  g_eng.h5bf3ee((char *const)String, "%d", v7[54] + v7[56]);
  rd_491820((intptr_t *)((intptr_t)g_eng.sys), a2, (const char *)String, 1u, 9, (*v36__f) + 160, (*v40__f) + 24, 0, 5, 0, 0);
  result = v7[20];
  if ( result != 0 )
  {
    if ( a4[6] == 2 )
    {
      v27 = F16(result, 8);
    }
    else if ( a4[6] == 3 )
    {
      v27 = F16(result, 10);
    }
    else
    {
      v27 = F16(result, 6);
    }
    v28 = SYS32(4 * F16(result, 4) + 2732);
    (*y__f) = (int)v28;
    if ( **(_DWORD **)((uint8_t *)(v28) + 0x0) == 0 )
    {
      W_fl_4184a0((intptr_t)(v28), (intptr_t)(0));
      v28 = (*y__f);
    }
    W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)((uint8_t *)(v28) + 0x0), 4 * v27)), (intptr_t)((int)a2), (intptr_t)((*v36__f)), (intptr_t)((*v40__f)));
    if ( a4[8] > 0 )
    {
      if ( a4[7] == 2 )
      {
        v29 = (uint16_t *)v7[20];
        v30 = v29[4];
      }
      else
      {
        v29 = (uint16_t *)v7[20];
        if ( a4[7] == 3 )
          v30 = v29[5];
        else
          v30 = v29[3];
      }
      v31 = SYS32(4 * v29[2] + 2732);
      if ( **(_DWORD **)((uint8_t *)(v31) + 0x0) == 0 )
        W_fl_4184a0((intptr_t)(v31), (intptr_t)(0));
      v32 = F32(**(_DWORD **)((uint8_t *)(v31) + 0x0), 4 * v30);
      v33 = v32[10];
      v34 = W_tint_fade_a((intptr_t)(a4[8] - 1), (intptr_t)(1000));
      W_rp_5bd550((intptr_t)(v34), (intptr_t)(a2), (intptr_t)((int)v32), (intptr_t)((*v36__f) + v32[3]), (intptr_t)((*v40__f) + v32[4]), (intptr_t)(v32[5]), (intptr_t)(v32[6]), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v33), (intptr_t)(*(intptr_t *)&SYSB(864)));
      v26 = (*v40__f);
    }
    v35 = rd_418470(SYS32(3008), F16(v7[20], 20));
    return W_rp_5b9a40((intptr_t)(v35), (intptr_t)((int)a2), (intptr_t)((*v36__f) + 91), (intptr_t)(v26 + 28));
  }
  return result;
}

/* 0x498620: int __stdcall sub_498620(int a1, int a2, int a3, int a4, int a5) */
int32_t rd_498620(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{

    int32_t ** v5 = 0;

  v5 = *(_DWORD ***)&SYSB(2928);
  if ( ***(_DWORD ***)&SYSB(2928) == 0 )
    W_fl_4184a0((intptr_t)(SYS32(2928)), (intptr_t)(0));
  return W_rp_5b9b70((intptr_t)(F32((**v5 + 4 * (uint16_t)((a4 <= a5) + 16)), 0)), (intptr_t)(a1), (intptr_t)(a2), (intptr_t)(a3));
}

/* 0x498680: int __thiscall hud_bar_row_geom_498680( cls_498680 *this, int a2, int a3, int a4, int a5, int y1, int a7, int a8, int a9, int a10, unsigned __int16 a11, int a12) */
int32_t rd_498680(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t y1, int32_t a7, int32_t a8, int32_t a9, int32_t a10, uint16_t a11, int32_t a12)
{
    intptr_t c = (intptr_t)self;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t result = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;

  v14 = a4;
  if ( 1000 * a3 / a4 >= 300 || F32(c, 0x3d0) / 4 % 2 != 0 && a12 != 0 )
    a10 = a9;
  if ( a4 <= 0 )
    v14 = 1;
  v27 = 0 / v14;
  v15 = a8 * a3 / v14 - 0 / v14;
  v16 = SYS32(4 * a11 + 2732);
  if ( **(_DWORD **)((uint8_t *)(v16) + 0x0) == 0 )
    W_fl_4184a0((intptr_t)(v16), (intptr_t)(0));
  v17 = SYS32(4 * a11 + 2732);
  v25 = ***(intptr_t ***)((uint8_t *)(v16) + 0x0);
  if ( **(_DWORD **)((uint8_t *)(v17) + 0x0) == 0 )
    W_fl_4184a0((intptr_t)(v17), (intptr_t)(0));
  result = a7;
  v24 = ***(intptr_t ***)((uint8_t *)(v17) + 0x0);
  if ( (uint16_t)a7 != 0 )
  {
    v20 = a8 - v15;
    v21 = v15 + a5;
    v22 = v27 - v15;
    v26 = v27 - v15;
    v28 = (uint16_t)a7;
    while ( 1 )
    {
      W_rp_5b9ae0((intptr_t)(v25), (intptr_t)(a2), (intptr_t)(v21 + v22), (intptr_t)(y1), (intptr_t)(v15), (intptr_t)(2), (intptr_t)(v20), (intptr_t)(a10), (intptr_t)(v15), (intptr_t)(2));
      if ( F32(CFG(), 148) == 2 )
        W_rp_5bd550((intptr_t)(SYS32(10184)), (intptr_t)((intptr_t)a2), (intptr_t)((int)v24), (intptr_t)(v21), (intptr_t)(y1), (intptr_t)(v20), (intptr_t)(2), (intptr_t)(0), (intptr_t)(14), (intptr_t)(*(_DWORD *)&GAP(v24, 0x0, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
      else
        W_rp_5b9ae0((intptr_t)(v24), (intptr_t)(a2), (intptr_t)(v21), (intptr_t)(y1), (intptr_t)(v20), (intptr_t)(2), (intptr_t)(0), (intptr_t)(14), (intptr_t)(v20), (intptr_t)(2));
      y1 += 2;
      --v21;
      result = --v28;
      if ( v28 == 0 )
        break;
      v22 = v26;
    }
  }
  return result;
}

/* 0x498820: int __stdcall sub_498820( paint_ctx *a1, int a2, cls_5B9AE0 *a3, int a4, int y1, int a6, int a7, int a8, unsigned __int16 a9) */
int32_t rd_498820(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t y1, int32_t a6, int32_t a7, int32_t a8, uint16_t a9)
{

    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t result = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v9 = (int)a3;
  if ( (int)a3 <= 0 )
    v9 = 1;
  (*v18__f) = 0 / v9;
  v10 = a7 * a2 / v9 - 0 / v9;
  v11 = SYS32(4 * a9 + 2732);
  if ( **(_DWORD **)((uint8_t *)(v11) + 0x0) == 0 )
    W_fl_4184a0((intptr_t)(v11), (intptr_t)(0));
  v12 = SYS32(4 * a9 + 2732);
  v21 = ***(intptr_t ***)((uint8_t *)(v11) + 0x0);
  if ( **(_DWORD **)((uint8_t *)(v12) + 0x0) == 0 )
    W_fl_4184a0((intptr_t)(v12), (intptr_t)(0));
  result = a6;
  v20 = ***(intptr_t ***)((uint8_t *)(v12) + 0x0);
  if ( (uint16_t)a6 != 0 )
  {
    v15 = a7 - v10;
    v16 = a4 + v10;
    v17 = (*v18__f) - v10;
    v19 = (uint16_t)a6;
    while ( 1 )
    {
      W_rp_5b9ae0((intptr_t)(v21), (intptr_t)((int)a1), (intptr_t)(v16 + v17), (intptr_t)(y1), (intptr_t)(v10), (intptr_t)(2), (intptr_t)(v15), (intptr_t)(a8), (intptr_t)(v10), (intptr_t)(2));
      if ( F32(CFG(), 148) == 2 )
        W_rp_5bd550((intptr_t)(SYS32(10184)), (intptr_t)(a1), (intptr_t)((int)v20), (intptr_t)(v16), (intptr_t)(y1), (intptr_t)(v15), (intptr_t)(2), (intptr_t)(0), (intptr_t)(14), (intptr_t)(*(_DWORD *)&GAP(v20, 0x0, 40)), (intptr_t)(*(intptr_t *)&SYSB(864)));
      else
        W_rp_5b9ae0((intptr_t)(v20), (intptr_t)((int)a1), (intptr_t)(v16), (intptr_t)(y1), (intptr_t)(v15), (intptr_t)(2), (intptr_t)(0), (intptr_t)(14), (intptr_t)(v15), (intptr_t)(2));
      y1 += 2;
      --v16;
      result = --v19;
      if ( v19 == 0 )
        break;
      v17 = (*v18__f) - v10;
    }
  }
  return result;
}

/* 0x498960: BOOL __stdcall sub_498960(paint_ctx *a1, _DWORD *a2, int a3, int a4, int a5, int a6, int a7) */
int32_t rd_498960(int32_t a1, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{

    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    uint16_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    int32_t * v17 = 0;
    uint16_t v18 = 0;
    intptr_t v19 = 0;
    int32_t * v20 = 0;
    int16_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    int32_t ** v28 = 0;
    uint16_t v29 = 0;
    int16_t v30 = 0;
    uint16_t v31 = 0;
    int v32 = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    _Alignas(8) uint8_t frame_[0x16c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *String = (char *)(frame_ + 0x1c);   /* [336] at ebp-0x150 */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x16c */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x168 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x164 */
    intptr_t *const y__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x160 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x15c */
    intptr_t *const v42__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x158 */
    intptr_t *const v43__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x154 */

  (*v43__f) = SYS32(2928);
  String[0] = 0;
  (*v39__f) = 0;
  (*v42__f) = -1;
  if ( a4 == 0 )
    return false;
  v8 = a2[21030];
  if ( a2[117] == 50010 && F16(a2[468], 1126) == 0 )
    v8 = 2;
  if ( v8 != 1 )
    return false;
  (*hdc__f) = 0;
  do
  {
    v9 = a3;
    v10 = 0;
    v11 = F16(a3, 324);
    if ( v11 != 0 )
    {
      v12 = F32(a3, 320);
      while ( 1 )
      {
        v13 = v9 + 12 * v10;
        if ( F32(v13, 332) == v12 && F16((v9 + 4 * (3 * v10 + 84)), 0) == (uint16_t)(*hdc__f) )
          break;
LABEL_20:
        if ( ++v10 >= v11 )
          goto LABEL_42;
      }
      v14 = 0;
      v15 = F32(v13, 328);
      v16 = 10101;
      while ( v16 != v15 )
      {
        v16 = ((int32_t *)D(0x6826c0))[192 * ++v14];
        if ( v16 == 0 )
          goto LABEL_19;
      }
      v17 = &((int32_t *)D(0x6826c0))[192 * v14];
      if ( v17 == 0 || v17[42] == 0 )
      {
LABEL_19:
        v9 = a3;
        goto LABEL_20;
      }
      if ( a3 + 12 * v10 != -328 )
      {
        v18 = 0;
        v19 = 10101;
        while ( v19 != F32(a3 + 12 * v10, 328) )
        {
          v19 = ((int32_t *)D(0x6826c0))[192 * ++v18];
          if ( v19 == 0 )
            goto LABEL_42;
        }
        v20 = &((int32_t *)D(0x6826c0))[192 * v18];
        if ( v20 != 0 && v20[42] != 0 )
        {
          v21 = F16(a3 + 12 * v10, 336);
          v22 = (uint16_t)(*v39__f);
          F16(&String[2 * (uint16_t)(*v39__f) + 80], 0) = F16(v20, 2 * 86);
          F16(&String[2 * v22 + 208], 0) = v21;
          switch ( v12 )
          {
            case 1:
              (*v38__f) = 0;
              break;
            case 2:
              (*v38__f) = 1;
              break;
            case 3:
              (*v38__f) = 2;
              break;
            default:
              break;
          }
          if ( (uint16_t)(*hdc__f) == F16(a3 + 2 * (uint16_t)(*v38__f), 312) )
          {
            if ( a6 != 0 )
            {
              if ( v20[181] <= 0 )
                strcpy(String, (uint8_t *)v20 + 4);
              else
                g_eng.h5bf3ee(String, "%s(MP:%d)", (uint8_t *)v20 + 4, v20[181]);
            }
            (*v42__f) = v22;
          }
          ++(*v39__f);
        }
      }
    }
LABEL_42:
    (*hdc__f) = (intptr_t)((uint8_t *)(*hdc__f) + 1);
  }
  while ( (uint16_t)(*hdc__f) < 0xAu );
  switch ( a5 )
  {
    case 1:
      (*v38__f) = 632 - 32 * (uint16_t)(*v39__f);
      v23 = ((a4 - 1000) << 6) / 1000;
      break;
    case 2:
      v23 = 96;
      (*v38__f) = 280 * (a4 - 1000) / 1000 + 10;
      break;
    case 3:
      v23 = 96;
      (*v38__f) = 280 * (1000 - a4) / 1000 + 360;
      break;
    default:
      v23 = (int)(*v43__f);
      break;
  }
  if ( (uint16_t)(*v39__f) != 0 )
  {
    v24 = (*v38__f);
    v25 = 0;
    (*hdc__f) = 0;
    (*y__f) = (uint16_t)(*v39__f);
    while ( 1 )
    {
      if ( v25 == (intptr_t)(*v42__f) )
      {
        v26 = SYS32(2960);
        (*v41__f) = v23 + 4;
        if ( ***(_DWORD ***)&SYSB(2960) == 0 )
          W_fl_4184a0((intptr_t)(SYS32(2960)), (intptr_t)(0));
        W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)v26, 44)), (intptr_t)((int)a1), (intptr_t)(v24), (intptr_t)(v23 - 4));
      }
      else
      {
        v27 = SYS32(2960);
        (*v41__f) = v23;
        if ( ***(_DWORD ***)&SYSB(2960) == 0 )
          W_fl_4184a0((intptr_t)(SYS32(2960)), (intptr_t)(0));
        W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)v27, 40)), (intptr_t)((int)a1), (intptr_t)(v24), (intptr_t)(v23 - 8));
      }
      v28 = *(_DWORD ***)&SYSB(2936);
      v29 = F16(&String[2 * (int32_t)v25 + 80], 0);
      if ( ***(_DWORD ***)&SYSB(2936) == 0 )
        W_fl_4184a0((intptr_t)(SYS32(2936)), (intptr_t)(0));
      W_rp_5b9b70((intptr_t)(F32(**v28, 4 * v29)), (intptr_t)((int)a1), (intptr_t)(v24), (intptr_t)((*v41__f)));
      v30 = F16(&String[2 * (int32_t)(*hdc__f) + 208], 0);
      if ( v30 != 255 )
      {
        v31 = v30 + 22;
        if ( **(_DWORD **)((uint8_t *)((*v43__f)) + 0x0) == 0 )
          W_fl_4184a0((intptr_t)((*v43__f)), (intptr_t)(0));
        W_rp_5b9b70((intptr_t)(F32(**(_DWORD **)((uint8_t *)((*v43__f)) + 0x0), 4 * v31)), (intptr_t)((int)a1), (intptr_t)(v24), (intptr_t)(v23 + 1));
      }
      v24 += 32;
      v32 = (*y__f) == 1;
      (*hdc__f) = (intptr_t)((uint8_t *)(*hdc__f) + 1);
      --(*y__f);
      if ( v32 )
        break;
      v25 = (*hdc__f);
    }
  }
  if ( String[0] != 0 )
  {
    (*y__f) = v23 + 41;
    v33 = 32 * (uint16_t)(*v39__f) - 6 * strlen(String) + (*v38__f) + 1;
    v34 = strlen(String);
    if ( W_rp_5b94e0((intptr_t)(a1), (intptr_t)((intptr_t *)&(*hdc__f))) != 0 )
    {
      API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
      API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(***(intptr_t ***)&SYSB(10012)));
      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0x202020u));
      (*v38__f) = -1;
      do
      {
        v35 = -1;
        v36 = (*v38__f) + (*y__f);
        do
        {
          API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v35 + v33), (intptr_t)(v36), (intptr_t)(String), (intptr_t)(v34));
          ++v35;
        }
        while ( v35 <= 1 );
        ++(*v38__f);
      }
      while ( (*v38__f) <= 1 );
      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)(0xFFFFFFu));
      API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(v33), (intptr_t)((*y__f)), (intptr_t)(String), (intptr_t)(v34));
      W_rp_5b9500((intptr_t)(a1), (intptr_t)((intptr_t)(*hdc__f)));
    }
  }
  return (uint16_t)(*v39__f) != 0;
}

/* 0x499070: int __stdcall sub_499070(unsigned __int16 a1) */
int32_t rd_499070(uint16_t a1)
{

    intptr_t result = 0;
    intptr_t v2 = 0;
    intptr_t v3 = 0;

  result = 0;
  if ( a1 < 0x14u )
  {
    v2 = 20 * a1 / 20;
    if ( v2 < 20 )
      result = SYS32(4 * v2 + 10152);
    else
      result = 0;
  }
  if ( a1 > 0x50u )
  {
    v3 = 20 * (100 - a1) / 20;
    if ( v3 >= 0 )
    {
      if ( v3 < 20 )
        return SYS32(4 * v3 + 10152);
      else
        return 0;
    }
    else
    {
      return SYS32(10152);
    }
  }
  return result;
}

/* 0x499100: void __thiscall sub_499100(cls_499100 *this, int a2) */
void rd_499100(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    int32_t * v2 = 0;
    uint16_t * v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t i = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    intptr_t j = 0;
    intptr_t v14 = 0;
    int32_t * v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    uint8_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    int32_t * v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t n = 0;
    intptr_t v29 = 0;
    int32_t * v30 = 0;
    intptr_t v31 = 0;
    intptr_t ii = 0;
    intptr_t v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    _Alignas(8) uint8_t frame_[0xd4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v47 = (intptr_t *)(frame_ + 0x20);   /* [5] at ebp-0xb4 */
    intptr_t *v48 = (intptr_t *)(frame_ + 0x34);   /* [2] at ebp-0xa0 */
    uint32_t *const v40__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0xd4 */
    intptr_t *const v44__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc0 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc0 */
    intptr_t *const m__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc0 */
    intptr_t *const k__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0xc0 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0xbc */
    int16_t *const v49__f = (int16_t *)(frame_ + 0x3c);   /* ebp-0x98 */
    intptr_t *const v50__f = (intptr_t *)(frame_ + 0x3e);   /* ebp-0x96 */
    intptr_t *const v51__f = (intptr_t *)(frame_ + 0x44);   /* ebp-0x90 */
    intptr_t *const v52__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x8c */
    intptr_t *const v53__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x88 */
    int16_t *const v54__f = (int16_t *)(frame_ + 0x50);   /* ebp-0x84 */
    int16_t *const v55__f = (int16_t *)(frame_ + 0x52);   /* ebp-0x82 */
    intptr_t *const v56__f = (intptr_t *)(frame_ + 0x58);   /* ebp-0x7c */

  v2 = *(_DWORD **)&GAP(W, 0x1030, 28);
  v3 = *(uint16_t **)&GAP(W, 0x1030, 24);
  if ( *v3 != 0 )
  {
    v4 = 0;
    v5 = 0;
    do
    {
      v6 = rd_417c40(((intptr_t)g_eng.sys), *v3, v4);
      W_rp_5b9a40((intptr_t)(v6), (intptr_t)(a2), (intptr_t)(v5), (intptr_t)(0));
      ++v4;
      v5 += 80;
    }
    while ( v4 < 8u );
  }
  rd_499560(a2, v2, (int)(v3 + 8), 250);
  rd_499560(a2, v2, (int)(v3 + 2), 500);
  v7 = W;
  for ( i = 4576; i < 5088; i += 4 )
  {
    v9 = *(intptr_t *)&GAP(v7, 0x0, i);
    if ( F32(v9, 0x1d0) != 0 )
    {
      rd_491ae0(v9, 0, *(_DWORD *)&GAP(v7, 0x1030, 28));
      v7 = W;
    }
  }
  v10 = 4192;
  (*v45__f) = 4192;
  do
  {
    v11 = *(_DWORD *)&GAP(v7, 0x0, v10);
    if ( F32(v11, 464) != 0 )
    {
      v12 = F32(v11, 64);
      (*v41__f) = *(_DWORD *)&GAP(v7, 0x1030, 28);
      SETLO(j, 1);
      if ( W_entity_sprite_desc((intptr_t)(v11), (intptr_t)((int)v12), (intptr_t)(v48)) != 0 )
      {
        v14 = F32(v11, 468);
        (*v51__f) = F32(v11, 244);
        if ( v14 == 100035 )
        {
          (*v52__f) = v48[0];
          (*v53__f) = v48[1];
          SETLO(j, 2);
          (*v54__f) = (*v49__f);
          (*v55__f) = (*v50__f) + 4;
          (*v56__f) = W_shot_476d30((intptr_t)(v11), (intptr_t)(1), (intptr_t)(0x78u), (intptr_t)(0xA0u), (intptr_t)(1000));
        }
        v15 = v48;
        for ( j = (uint16_t)j; j != 0; --j )
        {
          rd_493a50(v12, (*v41__f), F32(v11, 252), 1, 0, 0, 0, (int)v15);
          v15 += 5;
        }
        v10 = (*v45__f);
      }
      v7 = W;
    }
    v10 += 4;
    (*v45__f) = v10;
  }
  while ( v10 < 4448 );
  v16 = 5088;
  for ( (*k__f) = 5088; (*k__f) < 9184; (*k__f) += 4 )
  {
    v17 = *(_DWORD *)&GAP(v7, 0x0, v16);
    if ( F32(v17, 464) == 0 )
      goto LABEL_29;
    v18 = *(_DWORD *)&GAP(v7, 0x1030, 28);
    v19 = F32(v17, 64);
    if ( F32(v17, 468) == 100042 )
    {
      v20 = F16(v17, 620);
      v21 = F16(v17, 600);
      if ( v20 >= v21 - 20 )
      {
        if ( F16(v17, 620) >= F16(v17, 600) )
          goto LABEL_29;
        (*v40__f) = SYS32(4 * (v21 - v20) + 10152);
      }
      else
      {
        (*v40__f) = 0;
      }
      rd_499670(v17, *(_DWORD *)&GAP(v7, 0x1030, 28), v19, F32(v17, 508), (*v40__f));
    }
    else if ( W_entity_sprite_desc((intptr_t)(v17), (intptr_t)((int)v19), (intptr_t)(v47)) != 0 )
    {
      rd_493a50(v19, v18, F32(v17, 252), F32(v17, 616), 0, 0, F32(v17, 244), (int)v47);
    }
    v7 = W;
LABEL_29:
    v16 = (*k__f) + 4;
  }
  v22 = 9184;
  for ( (*m__f) = 9184; (*m__f) < 9568; (*m__f) += 4 )
  {
    v23 = *(intptr_t *)&GAP(v7, 0x0, v22);
    if ( F32(v23, 0x1d0) != 0 )
    {
      v24 = *(_DWORD *)&GAP(v7, 0x1030, 28);
      v25 = (_DWORD *)F32(v23, 0x40);
      v26 = F32(v23, 0xfc);
      if ( F32(v23, 0x204) != 2 )
      {
        v27 = v25[161];
        if ( v27 != 0 )
          v26 = F32(v27, 256);
        if ( W_entity_sprite_desc((intptr_t)(v23), (intptr_t)((int)v25), (intptr_t)(v47)) != 0 )
          rd_493a50(v25, v24, v26, 1, 0, 0, F32(v23, 0xf4), (int)v47);
        v7 = W;
      }
    }
    v22 = (*m__f) + 4;
  }
  for ( n = 9568; n < 10080; n += 4 )
  {
    v29 = *(intptr_t *)&GAP(v7, 0x0, n);
    if ( F32(v29, 0x1d0) != 0 )
    {
      v30 = (_DWORD *)F32(v29, 0x40);
      v31 = *(_DWORD *)&GAP(v7, 0x1030, 28);
      if ( W_entity_sprite_desc((intptr_t)(v29), (intptr_t)((int)v30), (intptr_t)(v47)) != 0 )
        rd_493a50(v30, v31, F32(v29, 0xfc), 1, 0, 0, F32(v29, 0xf4), (int)v47);
      v7 = W;
    }
  }
  for ( ii = 4448; ii < 4576; ii += 4 )
  {
    v33 = *(intptr_t *)&GAP(v7, 0x0, ii);
    if ( F32(v33, 0x1d0) != 0 )
    {
      rd_493ba0(v33, 0, *(_DWORD **)&GAP(v7, 0x1030, 28), 0);
      v7 = W;
    }
  }
  v34 = F16(c, 0x4) - 1;
  if ( v34 >= 0 )
  {
    v35 = 16672 * v34;
    (*v44__f) = F16(c, 0x4);
    do
    {
      v36 = 7;
      v37 = v35 + F32(c, 0x0);
      do
      {
        v38 = F32(v37 + 4 * (uint16_t)v36, 16432);
        if ( W_pslot_ok((intptr_t)(v38), (intptr_t)(1), (intptr_t)(0)) )
        {
          v39 = F32(v38, 2548);
          if ( v39 != 0 && v38 != F32(v37, 8204) )
            rd_493ba0(v39, 0, *(_DWORD **)&GAP(W, 0x1030, 28), 1);
        }
        --v36;
      }
      while ( v36 >= 0 );
      rd_4997b0(v35 + F32(c, 0x0), 1);
      v35 -= 16672;
      --(*v44__f);
    }
    while ( (*v44__f) != 0 );
    v7 = W;
  }
  rd_490f30(*(intptr_t *)&GAP(v7, 0x1030, 24), *(_DWORD **)&GAP(v7, 0x1030, 28), 1);
  rd_48eac0(*(intptr_t *)&GAP(W, 0x1030, 28), a2);
}

/* 0x499560: void __stdcall parallax_strip_499560(int a1, _DWORD *a2, int a3, int a4) */
void rd_499560(int32_t a1, int32_t * a2, int32_t a3, int32_t a4)
{

    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t i = 0;
    intptr_t v10 = 0;
    intptr_t v12 = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x14 */

  if ( F16(a3, 0) != 0 )
  {
    v5 = a2[19];
    v6 = a4 * ((a2[13] + a2[24]) / 100) / 1000 / 80;
    v7 = -(a4 * ((a2[13] + a2[24]) / 100) / 1000 % 80);
    v8 = F32(a3, 8) * ((v5 + a2[23] + 100 * a2[29]) / -100) / 1000;
    if ( v8 < -28 )
    {
      v12 = -28;
    }
    else if ( v8 <= 0 )
    {
      v12 = F32(a3, 8) * ((v5 + a2[23] + 100 * a2[29]) / -100) / 1000;
    }
    else
    {
      v12 = 0;
    }
    for ( i = 9; i != 0; --i )
    {
      (*v11__f) = v12 + F16(a3, 2);
      v10 = rd_417c40(((intptr_t)g_eng.sys), F16(a3, 0), v6 % F16(a3, 4));
      W_rp_5b9a40((intptr_t)(v10), (intptr_t)(a1), (intptr_t)(v7), (intptr_t)((*v11__f)));
      v7 += 80;
      ++v6;
    }
  }
}

/* 0x499670: char __thiscall sub_499670(cls_499670 *this, int a2, _BYTE *a3, int a4, unsigned int a5) */
int32_t rd_499670(wl_char *self, int32_t a2, uint8_t * a3, int32_t a4, uint32_t a5)
{
    intptr_t c = (intptr_t)self;
    uint8_t * v5 = 0;
    uint16_t * v6 = 0;
    uint32_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    uint32_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    uint16_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    uint32_t * v17 = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x14 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x10 */
    uint32_t *const v21__f = (uint32_t *)(frame_ + 0x8);   /* ebp-0xc */
    uint32_t *const v22__f = (uint32_t *)(frame_ + 0xc);   /* ebp-0x8 */

  v5 = a3;
  v6 = (uint16_t *)c;
  SETLB(v7, *a3);
  v8 = 0;
  (*v19__f) = 0;
  if ( *a3 != 0 )
  {
    v9 = 0;
    (*v20__f) = 0;
    if ( F16(c, 0x250) != 0 )
    {
      do
      {
        v10 = v6[(uint16_t)v9 + 256];
        if ( v10 == 255 )
        {
          v7 = (unsigned int)((uint64_t)(82463372100LL * a4) >> 32) >> 31;
          v8 += 60 * a4 / 100;
          (*v19__f) = v8;
        }
        else
        {
          v7 = rd_417c40(((intptr_t)g_eng.sys), v6[252], v10);
          v11 = v8 - F32(v7, 12);
          (*v21__f) = F32(v5, 4 * 1);
          (*v22__f) = F32(v5, 4 * 2);
          if ( v7 != 0 )
          {
            v12 = v6[126];
            v13 = F32(a2, 84);
            v14 = F16((v13 + 8 * v12), 0);
            v15 = v13 + 8 * v12;
            if ( v14 < F16(v15, 2) )
            {
              v16 = F32(v15, 4);
              F16(v15, 0) = v14 + 1;
              v17 = (unsigned int *)(v16 + 60 * v14);
              v6 = (uint16_t *)c;
              v17[1] = (*v21__f);
              v17[2] = (*v22__f);
              *v17 = v7;
              v17[3] = v11;
              v17[4] = 0;
              v17[6] = a5 != 0;
              v17[5] = a5;
            }
          }
          v5 = a3;
          v9 = (*v20__f);
          v8 = F32(v7, 20) + (*v19__f);
          (*v19__f) = v8;
        }
        (*v20__f) = ++v9;
      }
      while ( (uint16_t)v9 < v6[296] );
    }
  }
  return v7;
}

/* 0x4997b0: __int16 __stdcall sub_4997B0(int a1, int a2) */
int32_t rd_4997b0(int32_t a1, int32_t a2)
{

    intptr_t v2 = 0;
    intptr_t i = 0;
    intptr_t v4 = 0;

  if ( a2 != 0 )
  {
    v2 = F32(a1, 8204);
    if ( v2 != 0 && F32(v2, 2500) == 1 )
    {
      v2 = F32(v2, 2548);
      if ( v2 != 0 )
        SETLO(v2, rd_493ba0(v2, 0, *(_DWORD **)&GAP(W, 0x1030, 28), 1));
    }
  }
  else
  {
    for ( i = 7; i >= 0; --i )
    {
      v2 = F32(a1 + 4 * (uint16_t)i, 16432);
      if ( F32(v2, 2500) == 1 )
      {
        v4 = F32(v2, 2548);
        if ( v4 != 0 && v2 != F32(a1, 8204) )
          SETLO(v2, rd_493ba0(v4, 0, *(_DWORD **)&GAP(W, 0x1030, 28), 1));
      }
    }
  }
  return v2;
}

/* 0x49c170: int __stdcall sub_49C170(int a1) */
int32_t rd_49c170(int32_t a1)
{

    intptr_t v1 = 0;
    uint32_t v2 = 0;
    intptr_t v3 = 0;
    uint16_t v4 = 0;
    uint16_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    _Alignas(8) uint8_t frame_[0x7e + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x6);   /* [40] at ebp-0x78 */
    uint16_t *v12 = (uint16_t *)(frame_ + 0x2e);   /* [40] at ebp-0x50 */
    uint16_t *const v9__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x7e */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x2);   /* ebp-0x7c */

  v1 = 0;
  strcpy(Buffer, ((char *)D(0x855558)));
  v2 = strlen(Buffer) + 1;
  v3 = F32(SYS32(10348), 364);
  W_dmg_glyphs((intptr_t)(Buffer), (intptr_t)((int)v12), (intptr_t)(&(*v9__f)));
  v4 = 0;
  for ( (*i__f) = 0; (uint16_t)(*i__f) < (*v9__f); ++(*i__f) )
  {
    v5 = v12[v4];
    if ( v5 == 255
      || (v6 = rd_418470(SYS32(2740), v5), v7 = v6, v6 == 0) )
    {
      v1 += 10;
    }
    else
    {
      if ( v3 != 0 )
        W_rp_5b9b70((intptr_t)(v6), (intptr_t)(v3), (intptr_t)(288 - 17 * (v2 - 1) + v1 - F32(v6, 0xc)), (intptr_t)(120));
      v1 += F32(((uint8_t *)(v7) + 0x14), 0);
    }
    v4 = (*i__f) + 1;
  }
  g_eng.h5bf3ee(Buffer, "%02d:%02d:%02d", a1 / 1000 / 60, a1 / 1000 % 60, a1 / 10 % 100);
  return rd_491820(
           (intptr_t *)((intptr_t)g_eng.sys),
           *(intptr_t *)(SYS32(10348) + 364),
           Buffer,
           2u,
           17,
           288,
           120,
           0,
           10,
           0,
           0);
}
/* unported callees: none */
