/* asset_load.c — the BOOT asset-group loaders: every sprite slot / sound slot / font / palette the game registers at
 * start (the system object's slot tables): the sprite groups 0x56e190 / 0x57ca40 (~800 slots: resource id, cell size,
 * colour key, cuts, the asset group tag), the sounds 0x5752e0 (+ the slot init 0x563ef0), the UI / HUD banks 0x57b280,
 * the palette ramps 0x57a330, the display set-up 0x582e90 (DirectDraw mode, the primary surface, the zdd objects),
 * the colour tables 0x5749b0 (+ 0x5748c0), the BGM table 0x579bd0 (+ 0x579ec0..0x57a260, 0x582d10), the data files
 * 0x579680 / 0x579740 / 0x579860 / 0x5798f0 / 0x579a00, the input devices 0x583c90, the log line 0x583ee0, the frame
 * text 0x582b80.  See asset_load.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish. */
#include "asset_load.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include <windows.h>
#include <time.h>
#include "asset_register.h"
#include "bitmap_session.h"
#include "file_stream.h"
#include "frame_load.h"
#include "input_dev.h"
#include "render_platform.h"
#include "sound_dev.h"
#include "sprite_slot.h"
#include "title_menu.h"
#include "win_app.h"
#include "zdd_dev.h"
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
typedef uint16_t _WORD;
typedef uint8_t _BYTE;
typedef uint64_t _QWORD;
#define GAP(p, o, i) F8(p, (o) + (i))   /* IDA's gapN[i]: the byte at +N + i */
#define qmemcpy memcpy   /* IDA's inline memcpy */
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

static inline intptr_t W_bs_5b6e70(intptr_t a0, ...) { bs_5b6e70((wl_char *)a0); return 0; }
static inline intptr_t W_bs_5b6e80(intptr_t a0, ...) { bs_5b6e80((wl_char *)a0); return 0; }
static inline intptr_t W_fl_4178e0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fl_4178e0((wl_char *)a0, (void *)a1); }
static inline intptr_t W_fs_5b6060(intptr_t a0, ...) { return (intptr_t)fs_5b6060((wl_char *)a0); }
static inline intptr_t W_fs_5b60d0(intptr_t a0, ...) { return (intptr_t)fs_5b60d0((wl_char *)a0); }
static inline intptr_t W_fs_5b62a0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b62a0((wl_char *)a0, (int32_t)a1, (uint16_t)a2); }
static inline intptr_t W_fs_5b6340(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b6340((wl_char *)a0, (int32_t)a1, (uint32_t)a2); }
static inline intptr_t W_fs_5b64b0(intptr_t a0, ...) { return (intptr_t)fs_5b64b0((wl_char *)a0); }
static inline intptr_t W_id_5ba290(intptr_t a0, ...) { return (intptr_t)id_5ba290((wl_char *)a0); }
static inline intptr_t W_rp_5b8b40(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)rp_5b8b40((wl_char *)a0, (int32_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_rp_5b9390(intptr_t a0, ...) { return (intptr_t)rp_5b9390((wl_char *)a0); }
static inline intptr_t W_rp_5b94e0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b94e0((wl_char *)a0, (intptr_t *)a1); }
static inline intptr_t W_rp_5b9500(intptr_t a0, intptr_t a1, ...) { rp_5b9500((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_sd_5bb250(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, ...) { return (intptr_t)sd_5bb250((wl_char *)a0, (int32_t *)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5); }
static inline intptr_t W_sd_5bb2f0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)sd_5bb2f0((wl_char *)a0, (int32_t *)a1, (int32_t)a2); }
static inline intptr_t W_sd_5bbe20(intptr_t a0, intptr_t a1, ...) { return (intptr_t)sd_5bbe20((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_sd_5bc7c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, intptr_t a10, intptr_t a11, intptr_t a12, ...) { return (intptr_t)sd_5bc7c0((wl_char *)a0, (uint16_t)a1, (int32_t)a2, (uint32_t)a3, (char *)a4, (char *)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9, (int32_t)a10, (int32_t)a11, (int32_t)a12); }
static inline intptr_t W_sd_5bcb30(intptr_t a0, ...) { return (intptr_t)sd_5bcb30((wl_char *)a0); }
static inline intptr_t W_slot_clone(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)slot_clone((void *)a0, (uint16_t)a1, (uint16_t)a2); }
static inline intptr_t W_slot_entries_free(intptr_t a0, ...) { slot_entries_free((ar_sprite_slot *)a0); return 0; }
static inline intptr_t W_slot_palette_set(intptr_t a0, intptr_t a1, ...) { slot_palette_set((ar_sprite_slot *)a0, (const uint8_t *)a1); return 0; }
static inline intptr_t W_tm_562a10(intptr_t a0, ...) { tm_562a10((wl_char *)a0); return 0; }
static inline intptr_t W_tm_56dc20(intptr_t a0, intptr_t a1, intptr_t a2, ...) { tm_56dc20((int32_t)a0, (int32_t)a1, (int32_t)a2); return 0; }
static inline intptr_t W_wm_5b5ac0(intptr_t a0, ...) { return (intptr_t)wm_5b5ac0((uint32_t)a0); }
static inline intptr_t W_wm_5b5d90(intptr_t a0, intptr_t a1, ...) { return (intptr_t)wm_5b5d90((int32_t)a0, (int32_t)a1); }
static inline intptr_t W_wm_5b5dc0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)wm_5b5dc0((int32_t)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_wm_5b5f50(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)wm_5b5f50((uint32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3); }
static inline intptr_t W_zd_5b8480(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)zd_5b8480((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (void *)a6); }
static inline intptr_t W_zd_5b8900(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)zd_5b8900((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }
static inline intptr_t W_zd_5b8950(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, ...) { return (intptr_t)zd_5b8950((wl_char *)a0, (int32_t *)a1, (int32_t *)a2, (int32_t *)a3); }
static inline intptr_t W_zd_5b8dd0(intptr_t a0, ...) { return (intptr_t)zd_5b8dd0((wl_char *)a0); }
static inline intptr_t W_zd_5b9410(intptr_t a0, ...) { return (intptr_t)zd_5b9410((wl_char *)a0); }
typedef intptr_t (__attribute__((thiscall)) *rt_560900)(intptr_t);   /* 0x560900 thiscall: unported, retail's */
#define RC_560900 ((rt_560900)g_eng.retail(0x560900u))
typedef intptr_t (__attribute__((thiscall)) *rt_5b8fc0)(intptr_t, intptr_t);   /* 0x5b8fc0 thiscall: unported, retail's */
#define RC_5b8fc0 ((rt_5b8fc0)g_eng.retail(0x5b8fc0u))
typedef intptr_t (__attribute__((stdcall)) *api_CreateBrushIndirect_t)(intptr_t);
#define API_CreateBrushIndirect ((api_CreateBrushIndirect_t)g_eng.api("CreateBrushIndirect"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateFontIndirectA_t)(intptr_t);
#define API_CreateFontIndirectA ((api_CreateFontIndirectA_t)g_eng.api("CreateFontIndirectA"))
typedef intptr_t (__attribute__((stdcall)) *api_CreatePen_t)(intptr_t, intptr_t, intptr_t);
#define API_CreatePen ((api_CreatePen_t)g_eng.api("CreatePen"))
typedef intptr_t (__attribute__((stdcall)) *api_DeleteObject_t)(intptr_t);
#define API_DeleteObject ((api_DeleteObject_t)g_eng.api("DeleteObject"))
typedef uint32_t (__attribute__((stdcall)) *api_FormatMessageA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_FormatMessageA ((api_FormatMessageA_t)g_eng.api("FormatMessageA"))
typedef intptr_t (__attribute__((stdcall)) *api_FreeLibrary_t)(intptr_t);
#define API_FreeLibrary ((api_FreeLibrary_t)g_eng.api("FreeLibrary"))
typedef uint32_t (__attribute__((stdcall)) *api_GetLastError_t)(void);
#define API_GetLastError ((api_GetLastError_t)g_eng.api("GetLastError"))
typedef intptr_t (__attribute__((stdcall)) *api_GetLocalTime_t)(intptr_t);
#define API_GetLocalTime ((api_GetLocalTime_t)g_eng.api("GetLocalTime"))
typedef uint32_t (__attribute__((stdcall)) *api_GetModuleFileNameA_t)(intptr_t, intptr_t, intptr_t);
#define API_GetModuleFileNameA ((api_GetModuleFileNameA_t)g_eng.api("GetModuleFileNameA"))
typedef intptr_t (__attribute__((stdcall)) *api_GetTempFileNameA_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_GetTempFileNameA ((api_GetTempFileNameA_t)g_eng.api("GetTempFileNameA"))
typedef uint32_t (__attribute__((stdcall)) *api_GetTempPathA_t)(intptr_t, intptr_t);
#define API_GetTempPathA ((api_GetTempPathA_t)g_eng.api("GetTempPathA"))
typedef intptr_t (__attribute__((stdcall)) *api_LoadLibraryA_t)(intptr_t);
#define API_LoadLibraryA ((api_LoadLibraryA_t)g_eng.api("LoadLibraryA"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
typedef intptr_t (__attribute__((stdcall)) *api_SelectObject_t)(intptr_t, intptr_t);
#define API_SelectObject ((api_SelectObject_t)g_eng.api("SelectObject"))
typedef intptr_t (__attribute__((stdcall)) *api_SetBkMode_t)(intptr_t, intptr_t);
#define API_SetBkMode ((api_SetBkMode_t)g_eng.api("SetBkMode"))
typedef uint32_t (__attribute__((stdcall)) *api_SetTextColor_t)(intptr_t, intptr_t);
#define API_SetTextColor ((api_SetTextColor_t)g_eng.api("SetTextColor"))
typedef intptr_t (__attribute__((stdcall)) *api_TextOutA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_TextOutA ((api_TextOutA_t)g_eng.api("TextOutA"))
#define CRT_fclose ((intptr_t (*)())g_eng.retail(0x5bf440u))   /* retail's CRT fclose (cdecl) */
#define CRT_fopen ((intptr_t (*)())g_eng.retail(0x5bf4e8u))   /* retail's CRT fopen (cdecl) */
#define CRT_fprintf ((intptr_t (*)())g_eng.retail(0x5bf496u))   /* retail's CRT fprintf (cdecl) */
#define CRT_localtime ((intptr_t (*)())g_eng.retail(0x5c00ccu))   /* retail's CRT localtime (cdecl) */
#define CRT_time ((intptr_t (*)())g_eng.retail(0x5bf6dfu))   /* retail's CRT time (cdecl) */
int32_t al_56e190(void * zdd, uint16_t group, void * settings);
int32_t al_57ca40(void * zdd, uint16_t group, void * settings);
void al_5752e0(void);
int32_t al_57b280(int32_t a1, uint16_t group, void * settings);
void al_57a330(void * zdd, uint16_t group, void * settings);
int32_t al_582e90(wl_char *self);
int32_t al_5749b0(void * zdd, uint16_t group, void * settings);
int32_t al_5748c0(wl_char *self, void * zdd, void * settings, uint16_t resource_id, uint32_t width, uint32_t height, uint32_t colorkey, uint32_t scale_flag, uint32_t type, uint16_t group);
int32_t al_579680(char * a1);
int32_t al_579740(int32_t hModule, uint16_t a2, int32_t a3, uint32_t a4, int32_t a5, uint32_t nNumberOfBytesToRead);
int32_t al_579860(wl_char *self, char * a2, char * a3);
int32_t al_5798f0(wl_char *self, char * a2, char * a3);
int32_t al_579a00(int32_t a1, uint16_t group, void * settings);
int32_t al_579bd0(intptr_t ecx_, int32_t a1, uint16_t group, int32_t a3);
void al_579ec0(wl_char *self, uint16_t capacity);
int32_t al_579f40(int32_t a1, int32_t a2, int32_t a3);
void al_57a030(wl_char *self, int32_t width, int32_t height, uint32_t family, uint16_t group);
void al_57a1a0(wl_char *self, int32_t width, uint32_t color, uint16_t group, uint16_t capacity);
void al_57a260(wl_char *self, uint32_t color, uint16_t group, uint16_t capacity);
int32_t al_582b80(wl_char *self, ar_sprite_slot * dst);
void al_582d00(wl_char *self);
int32_t al_582d10(uint16_t a1, uint16_t a2, uint16_t a3, uint32_t color, int32_t a5, uint32_t a6, uint16_t a7, uint16_t a8, uint16_t a9);
int32_t al_583c90(wl_char *self, int32_t a2);
int32_t al_583ee0(void);
int32_t al_563ef0(wl_char *self, void * zds, void * settings, uint16_t resource_id, uint16_t count, uint16_t group, int32_t load_flag);

/* 0x56e190: int __stdcall sub_56E190(void *zdd, unsigned __int16 group, void *settings) */
int32_t al_56e190(void * zdd, uint16_t group, void * settings)
{

    intptr_t v3 = 0;
    void * v5 = 0;
    intptr_t v6 = 0;
    int v7 = 0;
    intptr_t v8 = 0;
    void * v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    void * v13 = 0;
    int32_t * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    void * v17 = 0;
    int32_t * v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    void * v21 = 0;
    int32_t * v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    void * v25 = 0;
    int32_t * v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    void * v29 = 0;
    int32_t * v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    void * v33 = 0;
    int32_t * v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    void * v37 = 0;
    int32_t * v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    void * v41 = 0;
    int32_t * v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    void * v45 = 0;
    int32_t * v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    void * v49 = 0;
    int32_t * v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    void * v53 = 0;
    int32_t * v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    void * v57 = 0;
    int32_t * v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    void * v61 = 0;
    int32_t * v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    void * v65 = 0;
    int32_t * v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    void * v69 = 0;
    int32_t * v70 = 0;
    intptr_t v71 = 0;
    intptr_t v72 = 0;
    void * v73 = 0;
    int32_t * v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    void * v77 = 0;
    int32_t * v78 = 0;
    intptr_t v79 = 0;
    intptr_t v80 = 0;
    void * v81 = 0;
    int32_t * v82 = 0;
    intptr_t v83 = 0;
    intptr_t v84 = 0;
    void * v85 = 0;
    int32_t * v86 = 0;
    intptr_t v87 = 0;
    intptr_t v88 = 0;
    void * v89 = 0;
    int32_t * v90 = 0;
    intptr_t v91 = 0;
    intptr_t v92 = 0;
    void * v93 = 0;
    int32_t * v94 = 0;
    intptr_t v95 = 0;
    intptr_t v96 = 0;
    void * v97 = 0;
    int32_t * v98 = 0;
    intptr_t v99 = 0;
    intptr_t v100 = 0;
    void * v101 = 0;
    int32_t * v102 = 0;
    intptr_t v103 = 0;
    intptr_t v104 = 0;
    void * v105 = 0;
    int32_t * v106 = 0;
    intptr_t v107 = 0;
    intptr_t v108 = 0;
    void * v109 = 0;
    int32_t * v110 = 0;
    intptr_t v111 = 0;
    intptr_t v112 = 0;
    void * v113 = 0;
    int32_t * v114 = 0;
    intptr_t v115 = 0;
    intptr_t v116 = 0;
    void * v117 = 0;
    int32_t * v118 = 0;
    intptr_t v119 = 0;
    intptr_t v120 = 0;
    void * v121 = 0;
    int32_t * v122 = 0;
    intptr_t v123 = 0;
    intptr_t v124 = 0;
    void * v125 = 0;
    int32_t * v126 = 0;
    intptr_t v127 = 0;
    intptr_t v128 = 0;
    void * v129 = 0;
    int32_t * v130 = 0;
    intptr_t v131 = 0;
    intptr_t v132 = 0;
    void * v133 = 0;
    int32_t * v134 = 0;
    intptr_t v135 = 0;
    intptr_t v136 = 0;
    void * v137 = 0;
    int32_t * v138 = 0;
    intptr_t v139 = 0;
    intptr_t v140 = 0;
    void * v141 = 0;
    int32_t * v142 = 0;
    intptr_t v143 = 0;
    intptr_t v144 = 0;
    void * v145 = 0;
    int32_t * v146 = 0;
    intptr_t v147 = 0;
    intptr_t v148 = 0;
    void * v149 = 0;
    int32_t * v150 = 0;
    intptr_t v151 = 0;
    intptr_t v152 = 0;
    void * v153 = 0;
    int32_t * v154 = 0;
    intptr_t v155 = 0;
    intptr_t v156 = 0;
    void * v157 = 0;
    int32_t * v158 = 0;
    intptr_t v159 = 0;
    intptr_t v160 = 0;
    void * v161 = 0;
    int32_t * v162 = 0;
    intptr_t v163 = 0;
    intptr_t v164 = 0;
    void * v165 = 0;
    int32_t * v166 = 0;
    intptr_t v167 = 0;
    intptr_t v168 = 0;
    void * v169 = 0;
    int32_t * v170 = 0;
    intptr_t v171 = 0;
    intptr_t v172 = 0;
    void * v173 = 0;
    int32_t * v174 = 0;
    intptr_t v175 = 0;
    intptr_t v176 = 0;
    void * v177 = 0;
    int32_t * v178 = 0;
    intptr_t v179 = 0;
    intptr_t v180 = 0;
    void * v181 = 0;
    int32_t * v182 = 0;
    intptr_t v183 = 0;
    intptr_t v184 = 0;
    void * v185 = 0;
    int32_t * v186 = 0;
    intptr_t v187 = 0;
    intptr_t v188 = 0;
    void * v189 = 0;
    int32_t * v190 = 0;
    intptr_t v191 = 0;
    intptr_t v192 = 0;
    void * v193 = 0;
    int32_t * v194 = 0;
    intptr_t v195 = 0;
    intptr_t v196 = 0;
    void * v197 = 0;
    int32_t * v198 = 0;
    intptr_t v199 = 0;
    intptr_t v200 = 0;
    void * v201 = 0;
    int32_t * v202 = 0;
    intptr_t v203 = 0;
    intptr_t v204 = 0;
    void * v205 = 0;
    int32_t * v206 = 0;
    intptr_t v207 = 0;
    intptr_t v208 = 0;
    void * v209 = 0;
    int32_t * v210 = 0;
    intptr_t v211 = 0;
    intptr_t v212 = 0;
    void * v213 = 0;
    int32_t * v214 = 0;
    intptr_t v215 = 0;
    intptr_t v216 = 0;
    void * v217 = 0;
    int32_t * v218 = 0;
    intptr_t v219 = 0;
    intptr_t v220 = 0;
    void * v221 = 0;
    int32_t * v222 = 0;
    intptr_t v223 = 0;
    intptr_t v224 = 0;
    void * v225 = 0;
    int32_t * v226 = 0;
    intptr_t v227 = 0;
    intptr_t v228 = 0;
    void * v229 = 0;
    int32_t * v230 = 0;
    intptr_t v231 = 0;
    intptr_t v232 = 0;
    void * v233 = 0;
    int32_t * v234 = 0;
    intptr_t v235 = 0;
    intptr_t v236 = 0;
    void * v237 = 0;
    int32_t * v238 = 0;
    intptr_t v239 = 0;
    intptr_t v240 = 0;
    void * v241 = 0;
    int32_t * v242 = 0;
    intptr_t v243 = 0;
    intptr_t v244 = 0;
    void * v245 = 0;
    int32_t * v246 = 0;
    intptr_t v247 = 0;
    intptr_t v248 = 0;
    void * v249 = 0;
    int32_t * v250 = 0;
    intptr_t v251 = 0;
    intptr_t v252 = 0;
    void * v253 = 0;
    int32_t * v254 = 0;
    intptr_t v255 = 0;
    intptr_t v256 = 0;
    void * v257 = 0;
    int32_t * v258 = 0;
    intptr_t v259 = 0;
    intptr_t v260 = 0;
    void * v261 = 0;
    int32_t * v262 = 0;
    intptr_t v263 = 0;
    intptr_t v264 = 0;
    void * v265 = 0;
    int32_t * v266 = 0;
    intptr_t v267 = 0;
    intptr_t v268 = 0;
    void * v269 = 0;
    int32_t * v270 = 0;
    intptr_t v271 = 0;
    intptr_t v272 = 0;
    void * v273 = 0;
    int32_t * v274 = 0;
    intptr_t v275 = 0;
    intptr_t v276 = 0;
    void * v277 = 0;
    int32_t * v278 = 0;
    intptr_t v279 = 0;
    intptr_t v280 = 0;
    void * v281 = 0;
    int32_t * v282 = 0;
    intptr_t v283 = 0;
    intptr_t v284 = 0;
    void * v285 = 0;
    int32_t * v286 = 0;
    intptr_t v287 = 0;
    intptr_t v288 = 0;
    void * v289 = 0;
    int32_t * v290 = 0;
    intptr_t v291 = 0;
    intptr_t v292 = 0;
    void * v293 = 0;
    int32_t * v294 = 0;
    intptr_t v295 = 0;
    intptr_t v296 = 0;
    void * v297 = 0;
    int32_t * v298 = 0;
    intptr_t v299 = 0;
    intptr_t v300 = 0;
    void * v301 = 0;
    int32_t * v302 = 0;
    intptr_t v303 = 0;
    intptr_t v304 = 0;
    void * v305 = 0;
    int32_t * v306 = 0;
    intptr_t v307 = 0;
    intptr_t v308 = 0;
    void * v309 = 0;
    int32_t * v310 = 0;
    intptr_t v311 = 0;
    intptr_t v312 = 0;
    void * v313 = 0;
    int32_t * v314 = 0;
    intptr_t v315 = 0;
    intptr_t v316 = 0;
    void * v317 = 0;
    int32_t * v318 = 0;
    intptr_t v319 = 0;
    intptr_t v320 = 0;
    void * v321 = 0;
    int32_t * v322 = 0;
    intptr_t v323 = 0;
    intptr_t v324 = 0;
    void * v325 = 0;
    int32_t * v326 = 0;
    intptr_t v327 = 0;
    intptr_t v328 = 0;
    void * v329 = 0;
    int32_t * v330 = 0;
    intptr_t v331 = 0;
    intptr_t v332 = 0;
    void * v333 = 0;
    int32_t * v334 = 0;
    intptr_t v335 = 0;
    intptr_t v336 = 0;
    void * v337 = 0;
    int32_t * v338 = 0;
    intptr_t v339 = 0;
    intptr_t v340 = 0;
    void * v341 = 0;
    int32_t * v342 = 0;
    intptr_t v343 = 0;
    intptr_t v344 = 0;
    void * v345 = 0;
    int32_t * v346 = 0;
    intptr_t v347 = 0;
    intptr_t v348 = 0;
    void * v349 = 0;
    int32_t * v350 = 0;
    intptr_t v351 = 0;
    intptr_t v352 = 0;
    void * v353 = 0;
    int32_t * v354 = 0;
    intptr_t v355 = 0;
    intptr_t v356 = 0;
    void * v357 = 0;
    int32_t * v358 = 0;
    intptr_t v359 = 0;
    intptr_t v360 = 0;
    void * v361 = 0;
    int32_t * v362 = 0;
    intptr_t v363 = 0;
    intptr_t v364 = 0;
    void * v365 = 0;
    int32_t * v366 = 0;
    intptr_t v367 = 0;
    intptr_t v368 = 0;
    void * v369 = 0;
    int32_t * v370 = 0;
    intptr_t v371 = 0;
    intptr_t v372 = 0;
    void * v373 = 0;
    int32_t * v374 = 0;
    intptr_t v375 = 0;
    intptr_t v376 = 0;
    int32_t * zdda = 0;
    int32_t * zddb = 0;
    int32_t * zddc = 0;
    int32_t * zddd = 0;
    int32_t * zdde = 0;
    int32_t * zddf = 0;
    int32_t * zddg = 0;
    int32_t * zddh = 0;
    int32_t * zddi = 0;
    int32_t * zddj = 0;
    int32_t * zddk = 0;
    int32_t * zddl = 0;
    int32_t * zddm = 0;
    int32_t * zddn = 0;
    int32_t * zddo = 0;
    int32_t * zddp = 0;
    int32_t * zddq = 0;
    int32_t * zddr = 0;
    int32_t * zdds = 0;
    int32_t * zddt = 0;
    int32_t * zddu = 0;
    int32_t * zddv = 0;
    int32_t * zddw = 0;
    int32_t * zddx = 0;
    int32_t * zddy = 0;
    int32_t * zddz = 0;
    int32_t * zddba = 0;
    int32_t * zddbb = 0;
    int32_t * zddbc = 0;
    int32_t * zddbd = 0;
    int32_t * zddbe = 0;
    int32_t * zddbf = 0;
    int32_t * zddbg = 0;
    int32_t * zddbh = 0;
    int32_t * zddbi = 0;
    int32_t * zddbj = 0;
    int32_t * zddbk = 0;
    int32_t * zddbl = 0;
    int32_t * zddbm = 0;
    int32_t * zddbn = 0;
    int32_t * zddbo = 0;
    int32_t * zddbp = 0;
    int32_t * zddbq = 0;
    int32_t * zddbr = 0;
    int32_t * zddbs = 0;
    int32_t * zddbt = 0;
    int32_t * zddbu = 0;
    int32_t * zddbv = 0;
    int32_t * zddbw = 0;
    int32_t * zddbx = 0;
    int32_t * zddby = 0;
    int32_t * zddbz = 0;
    int32_t * zddca = 0;
    int32_t * zddcb = 0;
    int32_t * zddcc = 0;
    int32_t * zddcd = 0;
    int32_t * zddce = 0;
    int32_t * zddcf = 0;
    int32_t * zddcg = 0;
    int32_t * zddch = 0;
    int32_t * zddci = 0;
    int32_t * zddcj = 0;
    int32_t * zddck = 0;
    int32_t * zddcl = 0;
    int32_t * zddcm = 0;
    int32_t * zddcn = 0;
    int32_t * zddco = 0;
    int32_t * zddcp = 0;
    int32_t * zddcq = 0;
    int32_t * zddcr = 0;
    int32_t * zddcs = 0;
    int32_t * zddct = 0;
    int32_t * zddcu = 0;
    int32_t * zddcv = 0;
    int32_t * zddcw = 0;
    int32_t * zddcx = 0;
    int32_t * zddcy = 0;
    int32_t * zddcz = 0;
    int32_t * zddda = 0;
    int32_t * zdddb = 0;
    int32_t * zdddc = 0;
    int32_t * zdddd = 0;
    int32_t * zddde = 0;
    int32_t * zdddf = 0;
    int32_t * zdddg = 0;
    int32_t * zdddh = 0;
    int32_t * zdddi = 0;
    int32_t * zdddj = 0;
    int32_t * zdddk = 0;
    int32_t * zdddl = 0;
    int32_t * zdddm = 0;
    int32_t * zdddn = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v378__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v3 = SYS32(4484);
  (*v378__f) = SYS32(4484);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4484)));
  F32(v3, 28) = zdd;
  F16(v3, 4) = 1;
  v5 = (intptr_t)g_eng.op_new(8u);
  v6 = 0;
  v7 = F16(v3, 4) == 0;
  F32(v3, 0) = v5;
  if ( !v7 )
  {
    do
    {
      v8 = 8 * (uint16_t)v6++;
      F32((F32(v3, 0) + v8), 0) = 0;
      F32(F32(v3, 0) + v8, 4) = 0;
    }
    while ( (uint16_t)v6 < F16(v3, 4) );
  }
  F32((*v378__f), 8) = 0;
  F32((*v378__f), 24) = 0;
  F32((*v378__f), 60) = settings;
  F16((*v378__f), 64) = 1426;
  F32((*v378__f), 32) = 160;
  F32((*v378__f), 36) = 176;
  F32((*v378__f), 40) = 16711935;
  F32((*v378__f), 44) = 1;
  F32((*v378__f), 48) = 0;
  F16((*v378__f), 66) = group;
  F32((*v378__f), 56) = 0;
  zdda = SYS32(4488);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4488)));
  zdda[7] = zdd;
  F16(zdda, 2 * 2) = 1;
  v9 = (intptr_t)g_eng.op_new(8u);
  v10 = zdda;
  v11 = 0;
  v7 = F16(zdda, 2 * 2) == 0;
  *zdda = v9;
  if ( !v7 )
  {
    do
    {
      v12 = 8 * (uint16_t)v11++;
      F32((*v10 + v12), 0) = 0;
      F32(*zdda + v12, 4) = 0;
      v10 = zdda;
    }
    while ( (uint16_t)v11 < F16(zdda, 2 * 2) );
  }
  zdda[2] = 0;
  zdda[6] = 0;
  zdda[15] = settings;
  F16(zdda, 2 * 32) = 1427;
  zdda[8] = 160;
  zdda[9] = 176;
  zdda[10] = 16711935;
  zdda[11] = 1;
  zdda[12] = 0;
  F16(zdda, 2 * 33) = group;
  zdda[14] = 0;
  zddb = SYS32(4492);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4492)));
  zddb[7] = zdd;
  F16(zddb, 2 * 2) = 1;
  v13 = (intptr_t)g_eng.op_new(8u);
  v14 = zddb;
  v15 = 0;
  v7 = F16(zddb, 2 * 2) == 0;
  *zddb = v13;
  if ( !v7 )
  {
    do
    {
      v16 = 8 * (uint16_t)v15++;
      F32((*v14 + v16), 0) = 0;
      F32(*zddb + v16, 4) = 0;
      v14 = zddb;
    }
    while ( (uint16_t)v15 < F16(zddb, 2 * 2) );
  }
  zddb[2] = 0;
  zddb[6] = 0;
  zddb[15] = settings;
  F16(zddb, 2 * 32) = 1428;
  zddb[8] = 160;
  zddb[9] = 176;
  zddb[10] = 16711935;
  zddb[11] = 1;
  zddb[12] = 0;
  F16(zddb, 2 * 33) = group;
  zddb[14] = 0;
  zddc = SYS32(4496);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4496)));
  zddc[7] = zdd;
  F16(zddc, 2 * 2) = 1;
  v17 = (intptr_t)g_eng.op_new(8u);
  v18 = zddc;
  v19 = 0;
  v7 = F16(zddc, 2 * 2) == 0;
  *zddc = v17;
  if ( !v7 )
  {
    do
    {
      v20 = 8 * (uint16_t)v19++;
      F32((v20 + *v18), 0) = 0;
      F32(v20 + *zddc, 4) = 0;
      v18 = zddc;
    }
    while ( (uint16_t)v19 < F16(zddc, 2 * 2) );
  }
  zddc[2] = 0;
  zddc[6] = 0;
  zddc[15] = settings;
  F16(zddc, 2 * 32) = 1429;
  zddc[8] = 160;
  zddc[9] = 176;
  zddc[10] = 16711935;
  zddc[11] = 1;
  zddc[12] = 0;
  F16(zddc, 2 * 33) = group;
  zddc[14] = 0;
  zddd = SYS32(4500);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4500)));
  zddd[7] = zdd;
  F16(zddd, 2 * 2) = 1;
  v21 = (intptr_t)g_eng.op_new(8u);
  v22 = zddd;
  v23 = 0;
  v7 = F16(zddd, 2 * 2) == 0;
  *zddd = v21;
  if ( !v7 )
  {
    do
    {
      v24 = 8 * (uint16_t)v23++;
      F32((v24 + *v22), 0) = 0;
      F32(v24 + *zddd, 4) = 0;
      v22 = zddd;
    }
    while ( (uint16_t)v23 < F16(zddd, 2 * 2) );
  }
  zddd[2] = 0;
  zddd[6] = 0;
  zddd[15] = settings;
  F16(zddd, 2 * 32) = 1430;
  zddd[8] = 160;
  zddd[9] = 176;
  zddd[10] = 16711935;
  zddd[11] = 1;
  zddd[12] = 0;
  F16(zddd, 2 * 33) = group;
  zddd[14] = 0;
  zdde = SYS32(4504);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4504)));
  zdde[7] = zdd;
  F16(zdde, 2 * 2) = 1;
  v25 = (intptr_t)g_eng.op_new(8u);
  v26 = zdde;
  v27 = 0;
  v7 = F16(zdde, 2 * 2) == 0;
  *zdde = v25;
  if ( !v7 )
  {
    do
    {
      v28 = 8 * (uint16_t)v27++;
      F32((*v26 + v28), 0) = 0;
      F32(*zdde + v28, 4) = 0;
      v26 = zdde;
    }
    while ( (uint16_t)v27 < F16(zdde, 2 * 2) );
  }
  zdde[2] = 0;
  zdde[6] = 0;
  zdde[15] = settings;
  F16(zdde, 2 * 32) = 1431;
  zdde[8] = 160;
  zdde[9] = 176;
  zdde[10] = 16711935;
  zdde[11] = 1;
  zdde[12] = 0;
  F16(zdde, 2 * 33) = group;
  zdde[14] = 0;
  zddf = SYS32(4508);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4508)));
  zddf[7] = zdd;
  F16(zddf, 2 * 2) = 1;
  v29 = (intptr_t)g_eng.op_new(8u);
  v30 = zddf;
  v31 = 0;
  v7 = F16(zddf, 2 * 2) == 0;
  *zddf = v29;
  if ( !v7 )
  {
    do
    {
      v32 = 8 * (uint16_t)v31++;
      F32((*v30 + v32), 0) = 0;
      F32(*zddf + v32, 4) = 0;
      v30 = zddf;
    }
    while ( (uint16_t)v31 < F16(zddf, 2 * 2) );
  }
  zddf[2] = 0;
  zddf[6] = 0;
  zddf[15] = settings;
  F16(zddf, 2 * 32) = 1432;
  zddf[8] = 160;
  zddf[9] = 176;
  zddf[10] = 16711935;
  zddf[11] = 1;
  zddf[12] = 0;
  F16(zddf, 2 * 33) = group;
  zddf[14] = 0;
  zddg = SYS32(4512);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4512)));
  zddg[7] = zdd;
  F16(zddg, 2 * 2) = 1;
  v33 = (intptr_t)g_eng.op_new(8u);
  v34 = zddg;
  v35 = 0;
  v7 = F16(zddg, 2 * 2) == 0;
  *zddg = v33;
  if ( !v7 )
  {
    do
    {
      v36 = 8 * (uint16_t)v35++;
      F32((*v34 + v36), 0) = 0;
      F32(*zddg + v36, 4) = 0;
      v34 = zddg;
    }
    while ( (uint16_t)v35 < F16(zddg, 2 * 2) );
  }
  zddg[2] = 0;
  zddg[6] = 0;
  zddg[15] = settings;
  F16(zddg, 2 * 32) = 1433;
  zddg[8] = 160;
  zddg[9] = 176;
  zddg[10] = 16711935;
  zddg[11] = 1;
  zddg[12] = 0;
  F16(zddg, 2 * 33) = group;
  zddg[14] = 0;
  zddh = SYS32(4516);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4516)));
  zddh[7] = zdd;
  F16(zddh, 2 * 2) = 1;
  v37 = (intptr_t)g_eng.op_new(8u);
  v38 = zddh;
  v39 = 0;
  v7 = F16(zddh, 2 * 2) == 0;
  *zddh = v37;
  if ( !v7 )
  {
    do
    {
      v40 = 8 * (uint16_t)v39++;
      F32((*v38 + v40), 0) = 0;
      F32(*zddh + v40, 4) = 0;
      v38 = zddh;
    }
    while ( (uint16_t)v39 < F16(zddh, 2 * 2) );
  }
  zddh[2] = 0;
  zddh[6] = 0;
  zddh[15] = settings;
  F16(zddh, 2 * 32) = 1434;
  zddh[8] = 160;
  zddh[9] = 176;
  zddh[10] = 16711935;
  zddh[11] = 1;
  zddh[12] = 0;
  F16(zddh, 2 * 33) = group;
  zddh[14] = 0;
  zddi = SYS32(4520);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4520)));
  zddi[7] = zdd;
  F16(zddi, 2 * 2) = 1;
  v41 = (intptr_t)g_eng.op_new(8u);
  v42 = zddi;
  v43 = 0;
  v7 = F16(zddi, 2 * 2) == 0;
  *zddi = v41;
  if ( !v7 )
  {
    do
    {
      v44 = 8 * (uint16_t)v43++;
      F32((*v42 + v44), 0) = 0;
      F32(*zddi + v44, 4) = 0;
      v42 = zddi;
    }
    while ( (uint16_t)v43 < F16(zddi, 2 * 2) );
  }
  zddi[2] = 0;
  zddi[6] = 0;
  zddi[15] = settings;
  F16(zddi, 2 * 32) = 1435;
  zddi[8] = 160;
  zddi[9] = 176;
  zddi[10] = 16711935;
  zddi[11] = 1;
  zddi[12] = 0;
  F16(zddi, 2 * 33) = group;
  zddi[14] = 0;
  zddj = SYS32(4524);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4524)));
  zddj[7] = zdd;
  F16(zddj, 2 * 2) = 1;
  v45 = (intptr_t)g_eng.op_new(8u);
  v46 = zddj;
  v47 = 0;
  v7 = F16(zddj, 2 * 2) == 0;
  *zddj = v45;
  if ( !v7 )
  {
    do
    {
      v48 = 8 * (uint16_t)v47++;
      F32((v48 + *v46), 0) = 0;
      F32(v48 + *zddj, 4) = 0;
      v46 = zddj;
    }
    while ( (uint16_t)v47 < F16(zddj, 2 * 2) );
  }
  zddj[2] = 0;
  zddj[6] = 0;
  zddj[15] = settings;
  F16(zddj, 2 * 32) = 1436;
  zddj[8] = 160;
  zddj[9] = 176;
  zddj[10] = 16711935;
  zddj[11] = 1;
  zddj[12] = 0;
  F16(zddj, 2 * 33) = group;
  zddj[14] = 0;
  zddk = SYS32(4528);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4528)));
  zddk[7] = zdd;
  F16(zddk, 2 * 2) = 1;
  v49 = (intptr_t)g_eng.op_new(8u);
  v50 = zddk;
  v51 = 0;
  v7 = F16(zddk, 2 * 2) == 0;
  *zddk = v49;
  if ( !v7 )
  {
    do
    {
      v52 = 8 * (uint16_t)v51++;
      F32((*v50 + v52), 0) = 0;
      F32(*zddk + v52, 4) = 0;
      v50 = zddk;
    }
    while ( (uint16_t)v51 < F16(zddk, 2 * 2) );
  }
  zddk[2] = 0;
  zddk[6] = 0;
  zddk[15] = settings;
  F16(zddk, 2 * 32) = 1437;
  zddk[8] = 160;
  zddk[9] = 176;
  zddk[10] = 16711935;
  zddk[11] = 1;
  zddk[12] = 0;
  F16(zddk, 2 * 33) = group;
  zddk[14] = 0;
  zddl = SYS32(4532);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4532)));
  zddl[7] = zdd;
  F16(zddl, 2 * 2) = 1;
  v53 = (intptr_t)g_eng.op_new(8u);
  v54 = zddl;
  v55 = 0;
  for ( *zddl = v53; (uint16_t)v55 < F16(zddl, 2 * 2); v54 = zddl )
  {
    v56 = 8 * (uint16_t)v55++;
    F32((*v54 + v56), 0) = 0;
    F32(*zddl + v56, 4) = 0;
  }
  zddl[2] = 0;
  zddl[6] = 0;
  zddl[15] = settings;
  F16(zddl, 2 * 32) = 1438;
  zddl[8] = 160;
  zddl[9] = 176;
  zddl[10] = 16711935;
  zddl[11] = 1;
  zddl[12] = 0;
  F16(zddl, 2 * 33) = group;
  zddl[14] = 0;
  zddm = SYS32(4536);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4536)));
  zddm[7] = zdd;
  F16(zddm, 2 * 2) = 1;
  v57 = (intptr_t)g_eng.op_new(8u);
  v58 = zddm;
  v59 = 0;
  for ( *zddm = v57; (uint16_t)v59 < F16(zddm, 2 * 2); v58 = zddm )
  {
    v60 = 8 * (uint16_t)v59++;
    F32((v60 + *v58), 0) = 0;
    F32(v60 + *zddm, 4) = 0;
  }
  zddm[2] = 0;
  zddm[6] = 0;
  zddm[15] = settings;
  F16(zddm, 2 * 32) = 1439;
  zddm[8] = 160;
  zddm[9] = 176;
  zddm[10] = 16711935;
  zddm[11] = 1;
  zddm[12] = 0;
  F16(zddm, 2 * 33) = group;
  zddm[14] = 0;
  zddn = SYS32(4540);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4540)));
  zddn[7] = zdd;
  F16(zddn, 2 * 2) = 1;
  v61 = (intptr_t)g_eng.op_new(8u);
  v62 = zddn;
  v63 = 0;
  for ( *zddn = v61; (uint16_t)v63 < F16(zddn, 2 * 2); v62 = zddn )
  {
    v64 = 8 * (uint16_t)v63++;
    F32((*v62 + v64), 0) = 0;
    F32(*zddn + v64, 4) = 0;
  }
  zddn[2] = 0;
  zddn[6] = 0;
  zddn[15] = settings;
  F16(zddn, 2 * 32) = 1440;
  zddn[8] = 160;
  zddn[9] = 176;
  zddn[10] = 16711935;
  zddn[11] = 1;
  zddn[12] = 0;
  F16(zddn, 2 * 33) = group;
  zddn[14] = 0;
  zddo = SYS32(4544);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4544)));
  zddo[7] = zdd;
  F16(zddo, 2 * 2) = 1;
  v65 = (intptr_t)g_eng.op_new(8u);
  v66 = zddo;
  v67 = 0;
  for ( *zddo = v65; (uint16_t)v67 < F16(zddo, 2 * 2); v66 = zddo )
  {
    v68 = 8 * (uint16_t)v67++;
    F32((v68 + *v66), 0) = 0;
    F32(v68 + *zddo, 4) = 0;
  }
  zddo[2] = 0;
  zddo[6] = 0;
  zddo[15] = settings;
  F16(zddo, 2 * 32) = 1441;
  zddo[8] = 160;
  zddo[9] = 176;
  zddo[10] = 16711935;
  zddo[11] = 1;
  zddo[12] = 0;
  F16(zddo, 2 * 33) = group;
  zddo[14] = 0;
  zddp = SYS32(4548);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4548)));
  zddp[7] = zdd;
  F16(zddp, 2 * 2) = 1;
  v69 = (intptr_t)g_eng.op_new(8u);
  v70 = zddp;
  v71 = 0;
  for ( *zddp = v69; (uint16_t)v71 < F16(zddp, 2 * 2); v70 = zddp )
  {
    v72 = 8 * (uint16_t)v71++;
    F32((v72 + *v70), 0) = 0;
    F32(v72 + *zddp, 4) = 0;
  }
  zddp[2] = 0;
  zddp[6] = 0;
  zddp[15] = settings;
  F16(zddp, 2 * 32) = 1503;
  zddp[8] = 160;
  zddp[9] = 176;
  zddp[10] = 16711935;
  zddp[11] = 1;
  zddp[12] = 0;
  F16(zddp, 2 * 33) = group;
  zddp[14] = 0;
  zddq = SYS32(4552);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4552)));
  zddq[7] = zdd;
  F16(zddq, 2 * 2) = 1;
  v73 = (intptr_t)g_eng.op_new(8u);
  v74 = zddq;
  v75 = 0;
  for ( *zddq = v73; (uint16_t)v75 < F16(zddq, 2 * 2); v74 = zddq )
  {
    v76 = 8 * (uint16_t)v75++;
    F32((*v74 + v76), 0) = 0;
    F32(*zddq + v76, 4) = 0;
  }
  zddq[2] = 0;
  zddq[6] = 0;
  zddq[15] = settings;
  F16(zddq, 2 * 32) = 1547;
  zddq[8] = 160;
  zddq[9] = 176;
  zddq[10] = 16711935;
  zddq[11] = 1;
  zddq[12] = 0;
  F16(zddq, 2 * 33) = group;
  zddq[14] = 0;
  zddr = SYS32(4556);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4556)));
  zddr[7] = zdd;
  F16(zddr, 2 * 2) = 1;
  v77 = (intptr_t)g_eng.op_new(8u);
  v78 = zddr;
  v79 = 0;
  for ( *zddr = v77; (uint16_t)v79 < F16(zddr, 2 * 2); v78 = zddr )
  {
    v80 = 8 * (uint16_t)v79++;
    F32((v80 + *v78), 0) = 0;
    F32(v80 + *zddr, 4) = 0;
  }
  zddr[2] = 0;
  zddr[6] = 0;
  zddr[15] = settings;
  F16(zddr, 2 * 32) = 1548;
  zddr[8] = 160;
  zddr[9] = 176;
  zddr[10] = 16711935;
  zddr[11] = 1;
  zddr[12] = 0;
  F16(zddr, 2 * 33) = group;
  zddr[14] = 0;
  zdds = SYS32(4560);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4560)));
  zdds[7] = zdd;
  F16(zdds, 2 * 2) = 1;
  v81 = (intptr_t)g_eng.op_new(8u);
  v82 = zdds;
  v83 = 0;
  for ( *zdds = v81; (uint16_t)v83 < F16(zdds, 2 * 2); v82 = zdds )
  {
    v84 = 8 * (uint16_t)v83++;
    F32((v84 + *v82), 0) = 0;
    F32(v84 + *zdds, 4) = 0;
  }
  zdds[2] = 0;
  zdds[6] = 0;
  zdds[15] = settings;
  F16(zdds, 2 * 32) = 1769;
  zdds[8] = 160;
  zdds[9] = 176;
  zdds[10] = 16711935;
  zdds[11] = 1;
  zdds[12] = 0;
  F16(zdds, 2 * 33) = group;
  zdds[14] = 0;
  zddt = SYS32(4564);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4564)));
  zddt[7] = zdd;
  F16(zddt, 2 * 2) = 1;
  v85 = (intptr_t)g_eng.op_new(8u);
  v86 = zddt;
  v87 = 0;
  for ( *zddt = v85; (uint16_t)v87 < F16(zddt, 2 * 2); v86 = zddt )
  {
    v88 = 8 * (uint16_t)v87++;
    F32((*v86 + v88), 0) = 0;
    F32(*zddt + v88, 4) = 0;
  }
  zddt[2] = 0;
  zddt[6] = 0;
  zddt[15] = settings;
  F16(zddt, 2 * 32) = 1770;
  zddt[8] = 160;
  zddt[9] = 176;
  zddt[10] = 16711935;
  zddt[11] = 1;
  zddt[12] = 0;
  F16(zddt, 2 * 33) = group;
  zddt[14] = 0;
  zddu = SYS32(4568);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4568)));
  zddu[7] = zdd;
  F16(zddu, 2 * 2) = 1;
  v89 = (intptr_t)g_eng.op_new(8u);
  v90 = zddu;
  v91 = 0;
  for ( *zddu = v89; (uint16_t)v91 < F16(zddu, 2 * 2); v90 = zddu )
  {
    v92 = 8 * (uint16_t)v91++;
    F32((v92 + *v90), 0) = 0;
    F32(v92 + *zddu, 4) = 0;
  }
  zddu[2] = 0;
  zddu[6] = 0;
  zddu[15] = settings;
  F16(zddu, 2 * 32) = 1442;
  zddu[8] = 160;
  zddu[9] = 176;
  zddu[10] = 16711935;
  zddu[11] = 1;
  zddu[12] = 0;
  F16(zddu, 2 * 33) = group;
  zddu[14] = 0;
  zddv = SYS32(4572);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4572)));
  zddv[7] = zdd;
  F16(zddv, 2 * 2) = 1;
  v93 = (intptr_t)g_eng.op_new(8u);
  v94 = zddv;
  v95 = 0;
  for ( *zddv = v93; (uint16_t)v95 < F16(zddv, 2 * 2); v94 = zddv )
  {
    v96 = 8 * (uint16_t)v95++;
    F32((v96 + *v94), 0) = 0;
    F32(v96 + *zddv, 4) = 0;
  }
  zddv[2] = 0;
  zddv[6] = 0;
  zddv[15] = settings;
  F16(zddv, 2 * 32) = 1443;
  zddv[8] = 160;
  zddv[9] = 176;
  zddv[10] = 16711935;
  zddv[11] = 1;
  zddv[12] = 0;
  F16(zddv, 2 * 33) = group;
  zddv[14] = 0;
  zddw = SYS32(4576);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4576)));
  zddw[7] = zdd;
  F16(zddw, 2 * 2) = 1;
  v97 = (intptr_t)g_eng.op_new(8u);
  v98 = zddw;
  v99 = 0;
  for ( *zddw = v97; (uint16_t)v99 < F16(zddw, 2 * 2); v98 = zddw )
  {
    v100 = 8 * (uint16_t)v99++;
    F32((v100 + *v98), 0) = 0;
    F32(v100 + *zddw, 4) = 0;
  }
  zddw[2] = 0;
  zddw[6] = 0;
  zddw[15] = settings;
  F16(zddw, 2 * 32) = 1444;
  zddw[8] = 160;
  zddw[9] = 176;
  zddw[10] = 16711935;
  zddw[11] = 1;
  zddw[12] = 0;
  F16(zddw, 2 * 33) = group;
  zddw[14] = 0;
  zddx = SYS32(4580);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4580)));
  zddx[7] = zdd;
  F16(zddx, 2 * 2) = 1;
  v101 = (intptr_t)g_eng.op_new(8u);
  v102 = zddx;
  v103 = 0;
  for ( *zddx = v101; (uint16_t)v103 < F16(zddx, 2 * 2); v102 = zddx )
  {
    v104 = 8 * (uint16_t)v103++;
    F32((v104 + *v102), 0) = 0;
    F32(v104 + *zddx, 4) = 0;
  }
  zddx[2] = 0;
  zddx[6] = 0;
  zddx[15] = settings;
  F16(zddx, 2 * 32) = 1445;
  zddx[8] = 160;
  zddx[9] = 176;
  zddx[10] = 16711935;
  zddx[11] = 1;
  zddx[12] = 0;
  F16(zddx, 2 * 33) = group;
  zddx[14] = 0;
  zddy = SYS32(4584);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4584)));
  zddy[7] = zdd;
  F16(zddy, 2 * 2) = 1;
  v105 = (intptr_t)g_eng.op_new(8u);
  v106 = zddy;
  v107 = 0;
  for ( *zddy = v105; (uint16_t)v107 < F16(zddy, 2 * 2); v106 = zddy )
  {
    v108 = 8 * (uint16_t)v107++;
    F32((v108 + *v106), 0) = 0;
    F32(v108 + *zddy, 4) = 0;
  }
  zddy[2] = 0;
  zddy[6] = 0;
  zddy[15] = settings;
  F16(zddy, 2 * 32) = 1446;
  zddy[8] = 160;
  zddy[9] = 176;
  zddy[10] = 16711935;
  zddy[11] = 1;
  zddy[12] = 0;
  F16(zddy, 2 * 33) = group;
  zddy[14] = 0;
  zddz = SYS32(4588);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4588)));
  zddz[7] = zdd;
  F16(zddz, 2 * 2) = 1;
  v109 = (intptr_t)g_eng.op_new(8u);
  v110 = zddz;
  v111 = 0;
  for ( *zddz = v109; (uint16_t)v111 < F16(zddz, 2 * 2); v110 = zddz )
  {
    v112 = 8 * (uint16_t)v111++;
    F32((*v110 + v112), 0) = 0;
    F32(*zddz + v112, 4) = 0;
  }
  zddz[2] = 0;
  zddz[6] = 0;
  zddz[15] = settings;
  F16(zddz, 2 * 32) = 1447;
  zddz[8] = 160;
  zddz[9] = 176;
  zddz[10] = 16711935;
  zddz[11] = 1;
  zddz[12] = 0;
  F16(zddz, 2 * 33) = group;
  zddz[14] = 0;
  zddba = SYS32(4592);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4592)));
  zddba[7] = zdd;
  F16(zddba, 2 * 2) = 1;
  v113 = (intptr_t)g_eng.op_new(8u);
  v114 = zddba;
  v115 = 0;
  for ( *zddba = v113; (uint16_t)v115 < F16(zddba, 2 * 2); v114 = zddba )
  {
    v116 = 8 * (uint16_t)v115++;
    F32((*v114 + v116), 0) = 0;
    F32(*zddba + v116, 4) = 0;
  }
  zddba[2] = 0;
  zddba[6] = 0;
  zddba[15] = settings;
  F16(zddba, 2 * 32) = 1448;
  zddba[8] = 160;
  zddba[9] = 176;
  zddba[10] = 16711935;
  zddba[11] = 1;
  zddba[12] = 0;
  F16(zddba, 2 * 33) = group;
  zddba[14] = 0;
  zddbb = SYS32(4596);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4596)));
  zddbb[7] = zdd;
  F16(zddbb, 2 * 2) = 1;
  v117 = (intptr_t)g_eng.op_new(8u);
  v118 = zddbb;
  v119 = 0;
  for ( *zddbb = v117; (uint16_t)v119 < F16(zddbb, 2 * 2); v118 = zddbb )
  {
    v120 = 8 * (uint16_t)v119++;
    F32((*v118 + v120), 0) = 0;
    F32(*zddbb + v120, 4) = 0;
  }
  zddbb[2] = 0;
  zddbb[6] = 0;
  zddbb[15] = settings;
  F16(zddbb, 2 * 32) = 1449;
  zddbb[8] = 160;
  zddbb[9] = 176;
  zddbb[10] = 16711935;
  zddbb[11] = 1;
  zddbb[12] = 0;
  F16(zddbb, 2 * 33) = group;
  zddbb[14] = 0;
  zddbc = SYS32(4600);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4600)));
  zddbc[7] = zdd;
  F16(zddbc, 2 * 2) = 1;
  v121 = (intptr_t)g_eng.op_new(8u);
  v122 = zddbc;
  v123 = 0;
  for ( *zddbc = v121; (uint16_t)v123 < F16(zddbc, 2 * 2); v122 = zddbc )
  {
    v124 = 8 * (uint16_t)v123++;
    F32((*v122 + v124), 0) = 0;
    F32(*zddbc + v124, 4) = 0;
  }
  zddbc[2] = 0;
  zddbc[6] = 0;
  zddbc[15] = settings;
  F16(zddbc, 2 * 32) = 1450;
  zddbc[8] = 160;
  zddbc[9] = 176;
  zddbc[10] = 16711935;
  zddbc[11] = 1;
  zddbc[12] = 0;
  F16(zddbc, 2 * 33) = group;
  zddbc[14] = 0;
  zddbd = SYS32(4604);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4604)));
  zddbd[7] = zdd;
  F16(zddbd, 2 * 2) = 1;
  v125 = (intptr_t)g_eng.op_new(8u);
  v126 = zddbd;
  v127 = 0;
  for ( *zddbd = v125; (uint16_t)v127 < F16(zddbd, 2 * 2); v126 = zddbd )
  {
    v128 = 8 * (uint16_t)v127++;
    F32((*v126 + v128), 0) = 0;
    F32(*zddbd + v128, 4) = 0;
  }
  zddbd[2] = 0;
  zddbd[6] = 0;
  zddbd[15] = settings;
  F16(zddbd, 2 * 32) = 1451;
  zddbd[8] = 160;
  zddbd[9] = 176;
  zddbd[10] = 16711935;
  zddbd[11] = 1;
  zddbd[12] = 0;
  F16(zddbd, 2 * 33) = group;
  zddbd[14] = 0;
  zddbe = SYS32(4608);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4608)));
  zddbe[7] = zdd;
  F16(zddbe, 2 * 2) = 1;
  v129 = (intptr_t)g_eng.op_new(8u);
  v130 = zddbe;
  v131 = 0;
  for ( *zddbe = v129; (uint16_t)v131 < F16(zddbe, 2 * 2); v130 = zddbe )
  {
    v132 = 8 * (uint16_t)v131++;
    F32((v132 + *v130), 0) = 0;
    F32(v132 + *zddbe, 4) = 0;
  }
  zddbe[2] = 0;
  zddbe[6] = 0;
  zddbe[15] = settings;
  F16(zddbe, 2 * 32) = 1452;
  zddbe[8] = 160;
  zddbe[9] = 176;
  zddbe[10] = 16711935;
  zddbe[11] = 1;
  zddbe[12] = 0;
  F16(zddbe, 2 * 33) = group;
  zddbe[14] = 0;
  zddbf = SYS32(4612);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4612)));
  zddbf[7] = zdd;
  F16(zddbf, 2 * 2) = 1;
  v133 = (intptr_t)g_eng.op_new(8u);
  v134 = zddbf;
  v135 = 0;
  for ( *zddbf = v133; (uint16_t)v135 < F16(zddbf, 2 * 2); v134 = zddbf )
  {
    v136 = 8 * (uint16_t)v135++;
    F32((*v134 + v136), 0) = 0;
    F32(*zddbf + v136, 4) = 0;
  }
  zddbf[2] = 0;
  zddbf[6] = 0;
  zddbf[15] = settings;
  F16(zddbf, 2 * 32) = 1453;
  zddbf[8] = 160;
  zddbf[9] = 176;
  zddbf[10] = 16711935;
  zddbf[11] = 1;
  zddbf[12] = 0;
  F16(zddbf, 2 * 33) = group;
  zddbf[14] = 0;
  zddbg = SYS32(4616);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4616)));
  zddbg[7] = zdd;
  F16(zddbg, 2 * 2) = 1;
  v137 = (intptr_t)g_eng.op_new(8u);
  v138 = zddbg;
  v139 = 0;
  for ( *zddbg = v137; (uint16_t)v139 < F16(zddbg, 2 * 2); v138 = zddbg )
  {
    v140 = 8 * (uint16_t)v139++;
    F32((*v138 + v140), 0) = 0;
    F32(*zddbg + v140, 4) = 0;
  }
  zddbg[2] = 0;
  zddbg[6] = 0;
  zddbg[15] = settings;
  F16(zddbg, 2 * 32) = 1454;
  zddbg[8] = 160;
  zddbg[9] = 176;
  zddbg[10] = 16711935;
  zddbg[11] = 1;
  zddbg[12] = 0;
  F16(zddbg, 2 * 33) = group;
  zddbg[14] = 0;
  zddbh = SYS32(4620);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4620)));
  zddbh[7] = zdd;
  F16(zddbh, 2 * 2) = 1;
  v141 = (intptr_t)g_eng.op_new(8u);
  v142 = zddbh;
  v143 = 0;
  for ( *zddbh = v141; (uint16_t)v143 < F16(zddbh, 2 * 2); v142 = zddbh )
  {
    v144 = 8 * (uint16_t)v143++;
    F32((v144 + *v142), 0) = 0;
    F32(v144 + *zddbh, 4) = 0;
  }
  zddbh[2] = 0;
  zddbh[6] = 0;
  zddbh[15] = settings;
  F16(zddbh, 2 * 32) = 1455;
  zddbh[8] = 160;
  zddbh[9] = 176;
  zddbh[10] = 16711935;
  zddbh[11] = 1;
  zddbh[12] = 0;
  F16(zddbh, 2 * 33) = group;
  zddbh[14] = 0;
  zddbi = SYS32(4624);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4624)));
  zddbi[7] = zdd;
  F16(zddbi, 2 * 2) = 1;
  v145 = (intptr_t)g_eng.op_new(8u);
  v146 = zddbi;
  v147 = 0;
  for ( *zddbi = v145; (uint16_t)v147 < F16(zddbi, 2 * 2); v146 = zddbi )
  {
    v148 = 8 * (uint16_t)v147++;
    F32((*v146 + v148), 0) = 0;
    F32(*zddbi + v148, 4) = 0;
  }
  zddbi[2] = 0;
  zddbi[6] = 0;
  zddbi[15] = settings;
  F16(zddbi, 2 * 32) = 1456;
  zddbi[8] = 160;
  zddbi[9] = 176;
  zddbi[10] = 16711935;
  zddbi[11] = 1;
  zddbi[12] = 0;
  F16(zddbi, 2 * 33) = group;
  zddbi[14] = 0;
  zddbj = SYS32(4628);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4628)));
  zddbj[7] = zdd;
  F16(zddbj, 2 * 2) = 1;
  v149 = (intptr_t)g_eng.op_new(8u);
  v150 = zddbj;
  v151 = 0;
  for ( *zddbj = v149; (uint16_t)v151 < F16(zddbj, 2 * 2); v150 = zddbj )
  {
    v152 = 8 * (uint16_t)v151++;
    F32((*v150 + v152), 0) = 0;
    F32(*zddbj + v152, 4) = 0;
  }
  zddbj[2] = 0;
  zddbj[6] = 0;
  zddbj[15] = settings;
  F16(zddbj, 2 * 32) = 1457;
  zddbj[8] = 160;
  zddbj[9] = 176;
  zddbj[10] = 16711935;
  zddbj[11] = 1;
  zddbj[12] = 0;
  F16(zddbj, 2 * 33) = group;
  zddbj[14] = 0;
  zddbk = SYS32(4632);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4632)));
  zddbk[7] = zdd;
  F16(zddbk, 2 * 2) = 1;
  v153 = (intptr_t)g_eng.op_new(8u);
  v154 = zddbk;
  v155 = 0;
  for ( *zddbk = v153; (uint16_t)v155 < F16(zddbk, 2 * 2); v154 = zddbk )
  {
    v156 = 8 * (uint16_t)v155++;
    F32((v156 + *v154), 0) = 0;
    F32(v156 + *zddbk, 4) = 0;
  }
  zddbk[2] = 0;
  zddbk[6] = 0;
  zddbk[15] = settings;
  F16(zddbk, 2 * 32) = 1504;
  zddbk[8] = 160;
  zddbk[9] = 176;
  zddbk[10] = 16711935;
  zddbk[11] = 1;
  zddbk[12] = 0;
  F16(zddbk, 2 * 33) = group;
  zddbk[14] = 0;
  zddbl = SYS32(4636);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4636)));
  zddbl[7] = zdd;
  F16(zddbl, 2 * 2) = 1;
  v157 = (intptr_t)g_eng.op_new(8u);
  v158 = zddbl;
  v159 = 0;
  for ( *zddbl = v157; (uint16_t)v159 < F16(zddbl, 2 * 2); v158 = zddbl )
  {
    v160 = 8 * (uint16_t)v159++;
    F32((*v158 + v160), 0) = 0;
    F32(*zddbl + v160, 4) = 0;
  }
  zddbl[2] = 0;
  zddbl[6] = 0;
  zddbl[15] = settings;
  F16(zddbl, 2 * 32) = 1549;
  zddbl[8] = 160;
  zddbl[9] = 176;
  zddbl[10] = 16711935;
  zddbl[11] = 1;
  zddbl[12] = 0;
  F16(zddbl, 2 * 33) = group;
  zddbl[14] = 0;
  zddbm = SYS32(4640);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4640)));
  zddbm[7] = zdd;
  F16(zddbm, 2 * 2) = 1;
  v161 = (intptr_t)g_eng.op_new(8u);
  v162 = zddbm;
  v163 = 0;
  for ( *zddbm = v161; (uint16_t)v163 < F16(zddbm, 2 * 2); v162 = zddbm )
  {
    v164 = 8 * (uint16_t)v163++;
    F32((v164 + *v162), 0) = 0;
    F32(v164 + *zddbm, 4) = 0;
  }
  zddbm[2] = 0;
  zddbm[6] = 0;
  zddbm[15] = settings;
  F16(zddbm, 2 * 32) = 1550;
  zddbm[8] = 160;
  zddbm[9] = 176;
  zddbm[10] = 16711935;
  zddbm[11] = 1;
  zddbm[12] = 0;
  F16(zddbm, 2 * 33) = group;
  zddbm[14] = 0;
  zddbn = SYS32(4644);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4644)));
  zddbn[7] = zdd;
  F16(zddbn, 2 * 2) = 1;
  v165 = (intptr_t)g_eng.op_new(8u);
  v166 = zddbn;
  v167 = 0;
  for ( *zddbn = v165; (uint16_t)v167 < F16(zddbn, 2 * 2); v166 = zddbn )
  {
    v168 = 8 * (uint16_t)v167++;
    F32((v168 + *v166), 0) = 0;
    F32(v168 + *zddbn, 4) = 0;
  }
  zddbn[2] = 0;
  zddbn[6] = 0;
  zddbn[15] = settings;
  F16(zddbn, 2 * 32) = 1771;
  zddbn[8] = 160;
  zddbn[9] = 176;
  zddbn[10] = 16711935;
  zddbn[11] = 1;
  zddbn[12] = 0;
  F16(zddbn, 2 * 33) = group;
  zddbn[14] = 0;
  zddbo = SYS32(4648);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4648)));
  zddbo[7] = zdd;
  F16(zddbo, 2 * 2) = 1;
  v169 = (intptr_t)g_eng.op_new(8u);
  v170 = zddbo;
  v171 = 0;
  for ( *zddbo = v169; (uint16_t)v171 < F16(zddbo, 2 * 2); v170 = zddbo )
  {
    v172 = 8 * (uint16_t)v171++;
    F32((*v170 + v172), 0) = 0;
    F32(*zddbo + v172, 4) = 0;
  }
  zddbo[2] = 0;
  zddbo[6] = 0;
  zddbo[15] = settings;
  F16(zddbo, 2 * 32) = 1772;
  zddbo[8] = 160;
  zddbo[9] = 176;
  zddbo[10] = 16711935;
  zddbo[11] = 1;
  zddbo[12] = 0;
  F16(zddbo, 2 * 33) = group;
  zddbo[14] = 0;
  zddbp = SYS32(4652);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4652)));
  zddbp[7] = zdd;
  F16(zddbp, 2 * 2) = 1;
  v173 = (intptr_t)g_eng.op_new(8u);
  v174 = zddbp;
  v175 = 0;
  for ( *zddbp = v173; (uint16_t)v175 < F16(zddbp, 2 * 2); v174 = zddbp )
  {
    v176 = 8 * (uint16_t)v175++;
    F32((v176 + *v174), 0) = 0;
    F32(v176 + *zddbp, 4) = 0;
  }
  zddbp[2] = 0;
  zddbp[6] = 0;
  zddbp[15] = settings;
  F16(zddbp, 2 * 32) = 1823;
  zddbp[8] = 176;
  zddbp[9] = 144;
  zddbp[10] = 16711935;
  zddbp[11] = 1;
  zddbp[12] = 0;
  F16(zddbp, 2 * 33) = group;
  zddbp[14] = 0;
  zddbq = SYS32(4656);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4656)));
  zddbq[7] = zdd;
  F16(zddbq, 2 * 2) = 1;
  v177 = (intptr_t)g_eng.op_new(8u);
  v178 = zddbq;
  v179 = 0;
  for ( *zddbq = v177; (uint16_t)v179 < F16(zddbq, 2 * 2); v178 = zddbq )
  {
    v180 = 8 * (uint16_t)v179++;
    F32((v180 + *v178), 0) = 0;
    F32(v180 + *zddbq, 4) = 0;
  }
  zddbq[2] = 0;
  zddbq[6] = 0;
  zddbq[15] = settings;
  F16(zddbq, 2 * 32) = 1824;
  zddbq[8] = 176;
  zddbq[9] = 144;
  zddbq[10] = 16711935;
  zddbq[11] = 1;
  zddbq[12] = 0;
  F16(zddbq, 2 * 33) = group;
  zddbq[14] = 0;
  zddbr = SYS32(4660);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4660)));
  zddbr[7] = zdd;
  F16(zddbr, 2 * 2) = 1;
  v181 = (intptr_t)g_eng.op_new(8u);
  v182 = zddbr;
  v183 = 0;
  for ( *zddbr = v181; (uint16_t)v183 < F16(zddbr, 2 * 2); v182 = zddbr )
  {
    v184 = 8 * (uint16_t)v183++;
    F32((*v182 + v184), 0) = 0;
    F32(*zddbr + v184, 4) = 0;
  }
  zddbr[2] = 0;
  zddbr[6] = 0;
  zddbr[15] = settings;
  F16(zddbr, 2 * 32) = 1825;
  zddbr[8] = 176;
  zddbr[9] = 144;
  zddbr[10] = 16711935;
  zddbr[11] = 1;
  zddbr[12] = 0;
  F16(zddbr, 2 * 33) = group;
  zddbr[14] = 0;
  zddbs = SYS32(4664);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4664)));
  zddbs[7] = zdd;
  F16(zddbs, 2 * 2) = 1;
  v185 = (intptr_t)g_eng.op_new(8u);
  v186 = zddbs;
  v187 = 0;
  for ( *zddbs = v185; (uint16_t)v187 < F16(zddbs, 2 * 2); v186 = zddbs )
  {
    v188 = 8 * (uint16_t)v187++;
    F32((v188 + *v186), 0) = 0;
    F32(v188 + *zddbs, 4) = 0;
  }
  zddbs[2] = 0;
  zddbs[6] = 0;
  zddbs[15] = settings;
  F16(zddbs, 2 * 32) = 1826;
  zddbs[8] = 176;
  zddbs[9] = 144;
  zddbs[10] = 16711935;
  zddbs[11] = 1;
  zddbs[12] = 0;
  F16(zddbs, 2 * 33) = group;
  zddbs[14] = 0;
  zddbt = SYS32(4668);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4668)));
  zddbt[7] = zdd;
  F16(zddbt, 2 * 2) = 1;
  v189 = (intptr_t)g_eng.op_new(8u);
  v190 = zddbt;
  v191 = 0;
  for ( *zddbt = v189; (uint16_t)v191 < F16(zddbt, 2 * 2); v190 = zddbt )
  {
    v192 = 8 * (uint16_t)v191++;
    F32((v192 + *v190), 0) = 0;
    F32(v192 + *zddbt, 4) = 0;
  }
  zddbt[2] = 0;
  zddbt[6] = 0;
  zddbt[15] = settings;
  F16(zddbt, 2 * 32) = 1827;
  zddbt[8] = 176;
  zddbt[9] = 144;
  zddbt[10] = 16711935;
  zddbt[11] = 1;
  zddbt[12] = 0;
  F16(zddbt, 2 * 33) = group;
  zddbt[14] = 0;
  zddbu = SYS32(4672);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4672)));
  zddbu[7] = zdd;
  F16(zddbu, 2 * 2) = 1;
  v193 = (intptr_t)g_eng.op_new(8u);
  v194 = zddbu;
  v195 = 0;
  for ( *zddbu = v193; (uint16_t)v195 < F16(zddbu, 2 * 2); v194 = zddbu )
  {
    v196 = 8 * (uint16_t)v195++;
    F32((v196 + *v194), 0) = 0;
    F32(v196 + *zddbu, 4) = 0;
  }
  zddbu[2] = 0;
  zddbu[6] = 0;
  zddbu[15] = settings;
  F16(zddbu, 2 * 32) = 1828;
  zddbu[8] = 176;
  zddbu[9] = 144;
  zddbu[10] = 16711935;
  zddbu[11] = 1;
  zddbu[12] = 0;
  F16(zddbu, 2 * 33) = group;
  zddbu[14] = 0;
  zddbv = SYS32(4676);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4676)));
  zddbv[7] = zdd;
  F16(zddbv, 2 * 2) = 1;
  v197 = (intptr_t)g_eng.op_new(8u);
  v198 = zddbv;
  v199 = 0;
  for ( *zddbv = v197; (uint16_t)v199 < F16(zddbv, 2 * 2); v198 = zddbv )
  {
    v200 = 8 * (uint16_t)v199++;
    F32((v200 + *v198), 0) = 0;
    F32(v200 + *zddbv, 4) = 0;
  }
  zddbv[2] = 0;
  zddbv[6] = 0;
  zddbv[15] = settings;
  F16(zddbv, 2 * 32) = 1829;
  zddbv[8] = 176;
  zddbv[9] = 144;
  zddbv[10] = 16711935;
  zddbv[11] = 1;
  zddbv[12] = 0;
  F16(zddbv, 2 * 33) = group;
  zddbv[14] = 0;
  zddbw = SYS32(4680);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4680)));
  zddbw[7] = zdd;
  F16(zddbw, 2 * 2) = 1;
  v201 = (intptr_t)g_eng.op_new(8u);
  v202 = zddbw;
  v203 = 0;
  for ( *zddbw = v201; (uint16_t)v203 < F16(zddbw, 2 * 2); v202 = zddbw )
  {
    v204 = 8 * (uint16_t)v203++;
    F32((v204 + *v202), 0) = 0;
    F32(v204 + *zddbw, 4) = 0;
  }
  zddbw[2] = 0;
  zddbw[6] = 0;
  zddbw[15] = settings;
  F16(zddbw, 2 * 32) = 1830;
  zddbw[8] = 176;
  zddbw[9] = 144;
  zddbw[10] = 16711935;
  zddbw[11] = 1;
  zddbw[12] = 0;
  F16(zddbw, 2 * 33) = group;
  zddbw[14] = 0;
  zddbx = SYS32(4684);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4684)));
  zddbx[7] = zdd;
  F16(zddbx, 2 * 2) = 1;
  v205 = (intptr_t)g_eng.op_new(8u);
  v206 = zddbx;
  v207 = 0;
  for ( *zddbx = v205; (uint16_t)v207 < F16(zddbx, 2 * 2); v206 = zddbx )
  {
    v208 = 8 * (uint16_t)v207++;
    F32((v208 + *v206), 0) = 0;
    F32(v208 + *zddbx, 4) = 0;
  }
  zddbx[2] = 0;
  zddbx[6] = 0;
  zddbx[15] = settings;
  F16(zddbx, 2 * 32) = 1831;
  zddbx[8] = 176;
  zddbx[9] = 144;
  zddbx[10] = 16711935;
  zddbx[11] = 1;
  zddbx[12] = 0;
  F16(zddbx, 2 * 33) = group;
  zddbx[14] = 0;
  zddby = SYS32(4688);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4688)));
  zddby[7] = zdd;
  F16(zddby, 2 * 2) = 1;
  v209 = (intptr_t)g_eng.op_new(8u);
  v210 = zddby;
  v211 = 0;
  for ( *zddby = v209; (uint16_t)v211 < F16(zddby, 2 * 2); v210 = zddby )
  {
    v212 = 8 * (uint16_t)v211++;
    F32((*v210 + v212), 0) = 0;
    F32(*zddby + v212, 4) = 0;
  }
  zddby[2] = 0;
  zddby[6] = 0;
  zddby[15] = settings;
  F16(zddby, 2 * 32) = 1832;
  zddby[8] = 176;
  zddby[9] = 144;
  zddby[10] = 16711935;
  zddby[11] = 1;
  zddby[12] = 0;
  F16(zddby, 2 * 33) = group;
  zddby[14] = 0;
  zddbz = SYS32(4692);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4692)));
  zddbz[7] = zdd;
  F16(zddbz, 2 * 2) = 1;
  v213 = (intptr_t)g_eng.op_new(8u);
  v214 = zddbz;
  v215 = 0;
  for ( *zddbz = v213; (uint16_t)v215 < F16(zddbz, 2 * 2); v214 = zddbz )
  {
    v216 = 8 * (uint16_t)v215++;
    F32((*v214 + v216), 0) = 0;
    F32(*zddbz + v216, 4) = 0;
  }
  zddbz[2] = 0;
  zddbz[6] = 0;
  zddbz[15] = settings;
  F16(zddbz, 2 * 32) = 1833;
  zddbz[8] = 176;
  zddbz[9] = 144;
  zddbz[10] = 16711935;
  zddbz[11] = 1;
  zddbz[12] = 0;
  F16(zddbz, 2 * 33) = group;
  zddbz[14] = 0;
  zddca = SYS32(4696);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4696)));
  zddca[7] = zdd;
  F16(zddca, 2 * 2) = 1;
  v217 = (intptr_t)g_eng.op_new(8u);
  v218 = zddca;
  v219 = 0;
  for ( *zddca = v217; (uint16_t)v219 < F16(zddca, 2 * 2); v218 = zddca )
  {
    v220 = 8 * (uint16_t)v219++;
    F32((*v218 + v220), 0) = 0;
    F32(*zddca + v220, 4) = 0;
  }
  zddca[2] = 0;
  zddca[6] = 0;
  zddca[15] = settings;
  F16(zddca, 2 * 32) = 1834;
  zddca[8] = 176;
  zddca[9] = 144;
  zddca[10] = 16711935;
  zddca[11] = 1;
  zddca[12] = 0;
  F16(zddca, 2 * 33) = group;
  zddca[14] = 0;
  zddcb = SYS32(4700);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4700)));
  zddcb[7] = zdd;
  F16(zddcb, 2 * 2) = 1;
  v221 = (intptr_t)g_eng.op_new(8u);
  v222 = zddcb;
  v223 = 0;
  for ( *zddcb = v221; (uint16_t)v223 < F16(zddcb, 2 * 2); v222 = zddcb )
  {
    v224 = 8 * (uint16_t)v223++;
    F32((*v222 + v224), 0) = 0;
    F32(*zddcb + v224, 4) = 0;
  }
  zddcb[2] = 0;
  zddcb[6] = 0;
  zddcb[15] = settings;
  F16(zddcb, 2 * 32) = 1835;
  zddcb[8] = 176;
  zddcb[9] = 144;
  zddcb[10] = 16711935;
  zddcb[11] = 1;
  zddcb[12] = 0;
  F16(zddcb, 2 * 33) = group;
  zddcb[14] = 0;
  zddcc = SYS32(4704);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4704)));
  zddcc[7] = zdd;
  F16(zddcc, 2 * 2) = 1;
  v225 = (intptr_t)g_eng.op_new(8u);
  v226 = zddcc;
  v227 = 0;
  for ( *zddcc = v225; (uint16_t)v227 < F16(zddcc, 2 * 2); v226 = zddcc )
  {
    v228 = 8 * (uint16_t)v227++;
    F32((v228 + *v226), 0) = 0;
    F32(v228 + *zddcc, 4) = 0;
  }
  zddcc[2] = 0;
  zddcc[6] = 0;
  zddcc[15] = settings;
  F16(zddcc, 2 * 32) = 1836;
  zddcc[8] = 176;
  zddcc[9] = 144;
  zddcc[10] = 16711935;
  zddcc[11] = 1;
  zddcc[12] = 0;
  F16(zddcc, 2 * 33) = group;
  zddcc[14] = 0;
  zddcd = SYS32(4708);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4708)));
  zddcd[7] = zdd;
  F16(zddcd, 2 * 2) = 1;
  v229 = (intptr_t)g_eng.op_new(8u);
  v230 = zddcd;
  v231 = 0;
  for ( *zddcd = v229; (uint16_t)v231 < F16(zddcd, 2 * 2); v230 = zddcd )
  {
    v232 = 8 * (uint16_t)v231++;
    F32((*v230 + v232), 0) = 0;
    F32(*zddcd + v232, 4) = 0;
  }
  zddcd[2] = 0;
  zddcd[6] = 0;
  zddcd[15] = settings;
  F16(zddcd, 2 * 32) = 1837;
  zddcd[8] = 176;
  zddcd[9] = 144;
  zddcd[10] = 16711935;
  zddcd[11] = 1;
  zddcd[12] = 0;
  F16(zddcd, 2 * 33) = group;
  zddcd[14] = 0;
  zddce = SYS32(4712);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4712)));
  zddce[7] = zdd;
  F16(zddce, 2 * 2) = 1;
  v233 = (intptr_t)g_eng.op_new(8u);
  v234 = zddce;
  v235 = 0;
  for ( *zddce = v233; (uint16_t)v235 < F16(zddce, 2 * 2); v234 = zddce )
  {
    v236 = 8 * (uint16_t)v235++;
    F32((*v234 + v236), 0) = 0;
    F32(*zddce + v236, 4) = 0;
  }
  zddce[2] = 0;
  zddce[6] = 0;
  zddce[15] = settings;
  F16(zddce, 2 * 32) = 1838;
  zddce[8] = 176;
  zddce[9] = 144;
  zddce[10] = 16711935;
  zddce[11] = 1;
  zddce[12] = 0;
  F16(zddce, 2 * 33) = group;
  zddce[14] = 0;
  zddcf = SYS32(4716);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4716)));
  zddcf[7] = zdd;
  F16(zddcf, 2 * 2) = 1;
  v237 = (intptr_t)g_eng.op_new(8u);
  v238 = zddcf;
  v239 = 0;
  for ( *zddcf = v237; (uint16_t)v239 < F16(zddcf, 2 * 2); v238 = zddcf )
  {
    v240 = 8 * (uint16_t)v239++;
    F32((v240 + *v238), 0) = 0;
    F32(v240 + *zddcf, 4) = 0;
  }
  zddcf[2] = 0;
  zddcf[6] = 0;
  zddcf[15] = settings;
  F16(zddcf, 2 * 32) = 1839;
  zddcf[8] = 176;
  zddcf[9] = 144;
  zddcf[10] = 16711935;
  zddcf[11] = 1;
  zddcf[12] = 0;
  F16(zddcf, 2 * 33) = group;
  zddcf[14] = 0;
  zddcg = SYS32(4720);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4720)));
  zddcg[7] = zdd;
  F16(zddcg, 2 * 2) = 1;
  v241 = (intptr_t)g_eng.op_new(8u);
  v242 = zddcg;
  v243 = 0;
  for ( *zddcg = v241; (uint16_t)v243 < F16(zddcg, 2 * 2); v242 = zddcg )
  {
    v244 = 8 * (uint16_t)v243++;
    F32((*v242 + v244), 0) = 0;
    F32(*zddcg + v244, 4) = 0;
  }
  zddcg[2] = 0;
  zddcg[6] = 0;
  zddcg[15] = settings;
  F16(zddcg, 2 * 32) = 1840;
  zddcg[8] = 176;
  zddcg[9] = 144;
  zddcg[10] = 16711935;
  zddcg[11] = 1;
  zddcg[12] = 0;
  F16(zddcg, 2 * 33) = group;
  zddcg[14] = 0;
  zddch = SYS32(4724);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4724)));
  zddch[7] = zdd;
  F16(zddch, 2 * 2) = 1;
  v245 = (intptr_t)g_eng.op_new(8u);
  v246 = zddch;
  v247 = 0;
  for ( *zddch = v245; (uint16_t)v247 < F16(zddch, 2 * 2); v246 = zddch )
  {
    v248 = 8 * (uint16_t)v247++;
    F32((*v246 + v248), 0) = 0;
    F32(*zddch + v248, 4) = 0;
  }
  zddch[2] = 0;
  zddch[6] = 0;
  zddch[15] = settings;
  F16(zddch, 2 * 32) = 1841;
  zddch[8] = 176;
  zddch[9] = 144;
  zddch[10] = 16711935;
  zddch[11] = 1;
  zddch[12] = 0;
  F16(zddch, 2 * 33) = group;
  zddch[14] = 0;
  zddci = SYS32(4728);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4728)));
  zddci[7] = zdd;
  F16(zddci, 2 * 2) = 1;
  v249 = (intptr_t)g_eng.op_new(8u);
  v250 = zddci;
  v251 = 0;
  for ( *zddci = v249; (uint16_t)v251 < F16(zddci, 2 * 2); v250 = zddci )
  {
    v252 = 8 * (uint16_t)v251++;
    F32((v252 + *v250), 0) = 0;
    F32(v252 + *zddci, 4) = 0;
  }
  zddci[2] = 0;
  zddci[6] = 0;
  zddci[15] = settings;
  F16(zddci, 2 * 32) = 1842;
  zddci[8] = 176;
  zddci[9] = 144;
  zddci[10] = 16711935;
  zddci[11] = 1;
  zddci[12] = 0;
  F16(zddci, 2 * 33) = group;
  zddci[14] = 0;
  zddcj = SYS32(4732);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4732)));
  zddcj[7] = zdd;
  F16(zddcj, 2 * 2) = 1;
  v253 = (intptr_t)g_eng.op_new(8u);
  v254 = zddcj;
  v255 = 0;
  for ( *zddcj = v253; (uint16_t)v255 < F16(zddcj, 2 * 2); v254 = zddcj )
  {
    v256 = 8 * (uint16_t)v255++;
    F32((*v254 + v256), 0) = 0;
    F32(*zddcj + v256, 4) = 0;
  }
  zddcj[2] = 0;
  zddcj[6] = 0;
  zddcj[15] = settings;
  F16(zddcj, 2 * 32) = 1843;
  zddcj[8] = 176;
  zddcj[9] = 144;
  zddcj[10] = 16711935;
  zddcj[11] = 1;
  zddcj[12] = 0;
  F16(zddcj, 2 * 33) = group;
  zddcj[14] = 0;
  zddck = SYS32(4736);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4736)));
  zddck[7] = zdd;
  F16(zddck, 2 * 2) = 1;
  v257 = (intptr_t)g_eng.op_new(8u);
  v258 = zddck;
  v259 = 0;
  for ( *zddck = v257; (uint16_t)v259 < F16(zddck, 2 * 2); v258 = zddck )
  {
    v260 = 8 * (uint16_t)v259++;
    F32((v260 + *v258), 0) = 0;
    F32(v260 + *zddck, 4) = 0;
  }
  zddck[2] = 0;
  zddck[6] = 0;
  zddck[15] = settings;
  F16(zddck, 2 * 32) = 1505;
  zddck[8] = 160;
  zddck[9] = 176;
  zddck[10] = 16711935;
  zddck[11] = 1;
  zddck[12] = 0;
  F16(zddck, 2 * 33) = group;
  zddck[14] = 0;
  zddcl = SYS32(4740);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4740)));
  zddcl[7] = zdd;
  F16(zddcl, 2 * 2) = 1;
  v261 = (intptr_t)g_eng.op_new(8u);
  v262 = zddcl;
  v263 = 0;
  for ( *zddcl = v261; (uint16_t)v263 < F16(zddcl, 2 * 2); v262 = zddcl )
  {
    v264 = 8 * (uint16_t)v263++;
    F32((v264 + *v262), 0) = 0;
    F32(v264 + *zddcl, 4) = 0;
  }
  zddcl[2] = 0;
  zddcl[6] = 0;
  zddcl[15] = settings;
  F16(zddcl, 2 * 32) = 1506;
  zddcl[8] = 160;
  zddcl[9] = 176;
  zddcl[10] = 16711935;
  zddcl[11] = 1;
  zddcl[12] = 0;
  F16(zddcl, 2 * 33) = group;
  zddcl[14] = 0;
  zddcm = SYS32(4744);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4744)));
  zddcm[7] = zdd;
  F16(zddcm, 2 * 2) = 1;
  v265 = (intptr_t)g_eng.op_new(8u);
  v266 = zddcm;
  v267 = 0;
  for ( *zddcm = v265; (uint16_t)v267 < F16(zddcm, 2 * 2); v266 = zddcm )
  {
    v268 = 8 * (uint16_t)v267++;
    F32((*v266 + v268), 0) = 0;
    F32(*zddcm + v268, 4) = 0;
  }
  zddcm[2] = 0;
  zddcm[6] = 0;
  zddcm[15] = settings;
  F16(zddcm, 2 * 32) = 1507;
  zddcm[8] = 160;
  zddcm[9] = 176;
  zddcm[10] = 16711935;
  zddcm[11] = 1;
  zddcm[12] = 0;
  F16(zddcm, 2 * 33) = group;
  zddcm[14] = 0;
  zddcn = SYS32(4748);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4748)));
  zddcn[7] = zdd;
  F16(zddcn, 2 * 2) = 1;
  v269 = (intptr_t)g_eng.op_new(8u);
  v270 = zddcn;
  v271 = 0;
  for ( *zddcn = v269; (uint16_t)v271 < F16(zddcn, 2 * 2); v270 = zddcn )
  {
    v272 = 8 * (uint16_t)v271++;
    F32((v272 + *v270), 0) = 0;
    F32(v272 + *zddcn, 4) = 0;
  }
  zddcn[2] = 0;
  zddcn[6] = 0;
  zddcn[15] = settings;
  F16(zddcn, 2 * 32) = 1508;
  zddcn[8] = 160;
  zddcn[9] = 176;
  zddcn[10] = 16711935;
  zddcn[11] = 1;
  zddcn[12] = 0;
  F16(zddcn, 2 * 33) = group;
  zddcn[14] = 0;
  zddco = SYS32(4752);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4752)));
  zddco[7] = zdd;
  F16(zddco, 2 * 2) = 1;
  v273 = (intptr_t)g_eng.op_new(8u);
  v274 = zddco;
  v275 = 0;
  for ( *zddco = v273; (uint16_t)v275 < F16(zddco, 2 * 2); v274 = zddco )
  {
    v276 = 8 * (uint16_t)v275++;
    F32((v276 + *v274), 0) = 0;
    F32(v276 + *zddco, 4) = 0;
  }
  zddco[2] = 0;
  zddco[6] = 0;
  zddco[15] = settings;
  F16(zddco, 2 * 32) = 1509;
  zddco[8] = 160;
  zddco[9] = 176;
  zddco[10] = 16711935;
  zddco[11] = 1;
  zddco[12] = 0;
  F16(zddco, 2 * 33) = group;
  zddco[14] = 0;
  zddcp = SYS32(4756);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4756)));
  zddcp[7] = zdd;
  F16(zddcp, 2 * 2) = 1;
  v277 = (intptr_t)g_eng.op_new(8u);
  v278 = zddcp;
  v279 = 0;
  for ( *zddcp = v277; (uint16_t)v279 < F16(zddcp, 2 * 2); v278 = zddcp )
  {
    v280 = 8 * (uint16_t)v279++;
    F32((*v278 + v280), 0) = 0;
    F32(*zddcp + v280, 4) = 0;
  }
  zddcp[2] = 0;
  zddcp[6] = 0;
  zddcp[15] = settings;
  F16(zddcp, 2 * 32) = 1510;
  zddcp[8] = 160;
  zddcp[9] = 176;
  zddcp[10] = 16711935;
  zddcp[11] = 1;
  zddcp[12] = 0;
  F16(zddcp, 2 * 33) = group;
  zddcp[14] = 0;
  zddcq = SYS32(4760);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4760)));
  zddcq[7] = zdd;
  F16(zddcq, 2 * 2) = 1;
  v281 = (intptr_t)g_eng.op_new(8u);
  v282 = zddcq;
  v283 = 0;
  for ( *zddcq = v281; (uint16_t)v283 < F16(zddcq, 2 * 2); v282 = zddcq )
  {
    v284 = 8 * (uint16_t)v283++;
    F32((v284 + *v282), 0) = 0;
    F32(v284 + *zddcq, 4) = 0;
  }
  zddcq[2] = 0;
  zddcq[6] = 0;
  zddcq[15] = settings;
  F16(zddcq, 2 * 32) = 1511;
  zddcq[8] = 160;
  zddcq[9] = 176;
  zddcq[10] = 16711935;
  zddcq[11] = 1;
  zddcq[12] = 0;
  F16(zddcq, 2 * 33) = group;
  zddcq[14] = 0;
  zddcr = SYS32(4764);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4764)));
  zddcr[7] = zdd;
  F16(zddcr, 2 * 2) = 1;
  v285 = (intptr_t)g_eng.op_new(8u);
  v286 = zddcr;
  v287 = 0;
  for ( *zddcr = v285; (uint16_t)v287 < F16(zddcr, 2 * 2); v286 = zddcr )
  {
    v288 = 8 * (uint16_t)v287++;
    F32((v288 + *v286), 0) = 0;
    F32(v288 + *zddcr, 4) = 0;
  }
  zddcr[2] = 0;
  zddcr[6] = 0;
  zddcr[15] = settings;
  F16(zddcr, 2 * 32) = 1512;
  zddcr[8] = 160;
  zddcr[9] = 176;
  zddcr[10] = 16711935;
  zddcr[11] = 1;
  zddcr[12] = 0;
  F16(zddcr, 2 * 33) = group;
  zddcr[14] = 0;
  zddcs = SYS32(4768);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4768)));
  zddcs[7] = zdd;
  F16(zddcs, 2 * 2) = 1;
  v289 = (intptr_t)g_eng.op_new(8u);
  v290 = zddcs;
  v291 = 0;
  for ( *zddcs = v289; (uint16_t)v291 < F16(zddcs, 2 * 2); v290 = zddcs )
  {
    v292 = 8 * (uint16_t)v291++;
    F32((v292 + *v290), 0) = 0;
    F32(v292 + *zddcs, 4) = 0;
  }
  zddcs[2] = 0;
  zddcs[6] = 0;
  zddcs[15] = settings;
  F16(zddcs, 2 * 32) = 1513;
  zddcs[8] = 160;
  zddcs[9] = 176;
  zddcs[10] = 16711935;
  zddcs[11] = 1;
  zddcs[12] = 0;
  F16(zddcs, 2 * 33) = group;
  zddcs[14] = 0;
  zddct = SYS32(4772);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4772)));
  zddct[7] = zdd;
  F16(zddct, 2 * 2) = 1;
  v293 = (intptr_t)g_eng.op_new(8u);
  v294 = zddct;
  v295 = 0;
  for ( *zddct = v293; (uint16_t)v295 < F16(zddct, 2 * 2); v294 = zddct )
  {
    v296 = 8 * (uint16_t)v295++;
    F32((v296 + *v294), 0) = 0;
    F32(v296 + *zddct, 4) = 0;
  }
  zddct[2] = 0;
  zddct[6] = 0;
  zddct[15] = settings;
  F16(zddct, 2 * 32) = 1514;
  zddct[8] = 160;
  zddct[9] = 176;
  zddct[10] = 16711935;
  zddct[11] = 1;
  zddct[12] = 0;
  F16(zddct, 2 * 33) = group;
  zddct[14] = 0;
  zddcu = SYS32(4776);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4776)));
  zddcu[7] = zdd;
  F16(zddcu, 2 * 2) = 1;
  v297 = (intptr_t)g_eng.op_new(8u);
  v298 = zddcu;
  v299 = 0;
  for ( *zddcu = v297; (uint16_t)v299 < F16(zddcu, 2 * 2); v298 = zddcu )
  {
    v300 = 8 * (uint16_t)v299++;
    F32((v300 + *v298), 0) = 0;
    F32(v300 + *zddcu, 4) = 0;
  }
  zddcu[2] = 0;
  zddcu[6] = 0;
  zddcu[15] = settings;
  F16(zddcu, 2 * 32) = 1515;
  zddcu[8] = 160;
  zddcu[9] = 176;
  zddcu[10] = 16711935;
  zddcu[11] = 1;
  zddcu[12] = 0;
  F16(zddcu, 2 * 33) = group;
  zddcu[14] = 0;
  zddcv = SYS32(4780);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4780)));
  zddcv[7] = zdd;
  F16(zddcv, 2 * 2) = 1;
  v301 = (intptr_t)g_eng.op_new(8u);
  v302 = zddcv;
  v303 = 0;
  for ( *zddcv = v301; (uint16_t)v303 < F16(zddcv, 2 * 2); v302 = zddcv )
  {
    v304 = 8 * (uint16_t)v303++;
    F32((v304 + *v302), 0) = 0;
    F32(v304 + *zddcv, 4) = 0;
  }
  zddcv[2] = 0;
  zddcv[6] = 0;
  zddcv[15] = settings;
  F16(zddcv, 2 * 32) = 1516;
  zddcv[8] = 160;
  zddcv[9] = 176;
  zddcv[10] = 16711935;
  zddcv[11] = 1;
  zddcv[12] = 0;
  F16(zddcv, 2 * 33) = group;
  zddcv[14] = 0;
  zddcw = SYS32(4784);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4784)));
  zddcw[7] = zdd;
  F16(zddcw, 2 * 2) = 1;
  v305 = (intptr_t)g_eng.op_new(8u);
  v306 = zddcw;
  v307 = 0;
  for ( *zddcw = v305; (uint16_t)v307 < F16(zddcw, 2 * 2); v306 = zddcw )
  {
    v308 = 8 * (uint16_t)v307++;
    F32((v308 + *v306), 0) = 0;
    F32(v308 + *zddcw, 4) = 0;
  }
  zddcw[2] = 0;
  zddcw[6] = 0;
  zddcw[15] = settings;
  F16(zddcw, 2 * 32) = 1517;
  zddcw[8] = 160;
  zddcw[9] = 176;
  zddcw[10] = 16711935;
  zddcw[11] = 1;
  zddcw[12] = 0;
  F16(zddcw, 2 * 33) = group;
  zddcw[14] = 0;
  zddcx = SYS32(4788);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4788)));
  zddcx[7] = zdd;
  F16(zddcx, 2 * 2) = 1;
  v309 = (intptr_t)g_eng.op_new(8u);
  v310 = zddcx;
  v311 = 0;
  for ( *zddcx = v309; (uint16_t)v311 < F16(zddcx, 2 * 2); v310 = zddcx )
  {
    v312 = 8 * (uint16_t)v311++;
    F32((v312 + *v310), 0) = 0;
    F32(v312 + *zddcx, 4) = 0;
  }
  zddcx[2] = 0;
  zddcx[6] = 0;
  zddcx[15] = settings;
  F16(zddcx, 2 * 32) = 1519;
  zddcx[8] = 160;
  zddcx[9] = 176;
  zddcx[10] = 16711935;
  zddcx[11] = 1;
  zddcx[12] = 0;
  F16(zddcx, 2 * 33) = group;
  zddcx[14] = 0;
  zddcy = SYS32(4792);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4792)));
  zddcy[7] = zdd;
  F16(zddcy, 2 * 2) = 1;
  v313 = (intptr_t)g_eng.op_new(8u);
  v314 = zddcy;
  v315 = 0;
  for ( *zddcy = v313; (uint16_t)v315 < F16(zddcy, 2 * 2); v314 = zddcy )
  {
    v316 = 8 * (uint16_t)v315++;
    F32((*v314 + v316), 0) = 0;
    F32(*zddcy + v316, 4) = 0;
  }
  zddcy[2] = 0;
  zddcy[6] = 0;
  zddcy[15] = settings;
  F16(zddcy, 2 * 32) = 1535;
  zddcy[8] = 160;
  zddcy[9] = 176;
  zddcy[10] = 16711935;
  zddcy[11] = 1;
  zddcy[12] = 0;
  F16(zddcy, 2 * 33) = group;
  zddcy[14] = 0;
  zddcz = SYS32(4796);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4796)));
  zddcz[7] = zdd;
  F16(zddcz, 2 * 2) = 1;
  v317 = (intptr_t)g_eng.op_new(8u);
  v318 = zddcz;
  v319 = 0;
  for ( *zddcz = v317; (uint16_t)v319 < F16(zddcz, 2 * 2); v318 = zddcz )
  {
    v320 = 8 * (uint16_t)v319++;
    F32((*v318 + v320), 0) = 0;
    F32(*zddcz + v320, 4) = 0;
  }
  zddcz[2] = 0;
  zddcz[6] = 0;
  zddcz[15] = settings;
  F16(zddcz, 2 * 32) = 1540;
  zddcz[8] = 160;
  zddcz[9] = 176;
  zddcz[10] = 16711935;
  zddcz[11] = 1;
  zddcz[12] = 0;
  F16(zddcz, 2 * 33) = group;
  zddcz[14] = 0;
  zddda = SYS32(4800);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4800)));
  zddda[7] = zdd;
  F16(zddda, 2 * 2) = 1;
  v321 = (intptr_t)g_eng.op_new(8u);
  v322 = zddda;
  v323 = 0;
  for ( *zddda = v321; (uint16_t)v323 < F16(zddda, 2 * 2); v322 = zddda )
  {
    v324 = 8 * (uint16_t)v323++;
    F32((v324 + *v322), 0) = 0;
    F32(v324 + *zddda, 4) = 0;
  }
  zddda[2] = 0;
  zddda[6] = 0;
  zddda[15] = settings;
  F16(zddda, 2 * 32) = 1541;
  zddda[8] = 160;
  zddda[9] = 176;
  zddda[10] = 16711935;
  zddda[11] = 1;
  zddda[12] = 0;
  F16(zddda, 2 * 33) = group;
  zddda[14] = 0;
  zdddb = SYS32(4804);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4804)));
  zdddb[7] = zdd;
  F16(zdddb, 2 * 2) = 1;
  v325 = (intptr_t)g_eng.op_new(8u);
  v326 = zdddb;
  v327 = 0;
  for ( *zdddb = v325; (uint16_t)v327 < F16(zdddb, 2 * 2); v326 = zdddb )
  {
    v328 = 8 * (uint16_t)v327++;
    F32((*v326 + v328), 0) = 0;
    F32(*zdddb + v328, 4) = 0;
  }
  zdddb[2] = 0;
  zdddb[6] = 0;
  zdddb[15] = settings;
  F16(zdddb, 2 * 32) = 1975;
  zdddb[8] = 160;
  zdddb[9] = 176;
  zdddb[10] = 16711935;
  zdddb[11] = 1;
  zdddb[12] = 0;
  F16(zdddb, 2 * 33) = group;
  zdddb[14] = 0;
  zdddc = SYS32(4808);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4808)));
  zdddc[7] = zdd;
  F16(zdddc, 2 * 2) = 1;
  v329 = (intptr_t)g_eng.op_new(8u);
  v330 = zdddc;
  v331 = 0;
  for ( *zdddc = v329; (uint16_t)v331 < F16(zdddc, 2 * 2); v330 = zdddc )
  {
    v332 = 8 * (uint16_t)v331++;
    F32((*v330 + v332), 0) = 0;
    F32(*zdddc + v332, 4) = 0;
  }
  zdddc[2] = 0;
  zdddc[6] = 0;
  zdddc[15] = settings;
  F16(zdddc, 2 * 32) = 1520;
  zdddc[8] = 160;
  zdddc[9] = 176;
  zdddc[10] = 16711935;
  zdddc[11] = 1;
  zdddc[12] = 0;
  F16(zdddc, 2 * 33) = group;
  zdddc[14] = 0;
  zdddd = SYS32(4812);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4812)));
  zdddd[7] = zdd;
  F16(zdddd, 2 * 2) = 1;
  v333 = (intptr_t)g_eng.op_new(8u);
  v334 = zdddd;
  v335 = 0;
  for ( *zdddd = v333; (uint16_t)v335 < F16(zdddd, 2 * 2); v334 = zdddd )
  {
    v336 = 8 * (uint16_t)v335++;
    F32((v336 + *v334), 0) = 0;
    F32(v336 + *zdddd, 4) = 0;
  }
  zdddd[2] = 0;
  zdddd[6] = 0;
  zdddd[15] = settings;
  F16(zdddd, 2 * 32) = 1521;
  zdddd[8] = 160;
  zdddd[9] = 176;
  zdddd[10] = 16711935;
  zdddd[11] = 1;
  zdddd[12] = 0;
  F16(zdddd, 2 * 33) = group;
  zdddd[14] = 0;
  zddde = SYS32(4816);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4816)));
  zddde[7] = zdd;
  F16(zddde, 2 * 2) = 1;
  v337 = (intptr_t)g_eng.op_new(8u);
  v338 = zddde;
  v339 = 0;
  for ( *zddde = v337; (uint16_t)v339 < F16(zddde, 2 * 2); v338 = zddde )
  {
    v340 = 8 * (uint16_t)v339++;
    F32((*v338 + v340), 0) = 0;
    F32(*zddde + v340, 4) = 0;
  }
  zddde[2] = 0;
  zddde[6] = 0;
  zddde[15] = settings;
  F16(zddde, 2 * 32) = 1522;
  zddde[8] = 160;
  zddde[9] = 176;
  zddde[10] = 16711935;
  zddde[11] = 1;
  zddde[12] = 0;
  F16(zddde, 2 * 33) = group;
  zddde[14] = 0;
  zdddf = SYS32(4820);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4820)));
  zdddf[7] = zdd;
  F16(zdddf, 2 * 2) = 1;
  v341 = (intptr_t)g_eng.op_new(8u);
  v342 = zdddf;
  v343 = 0;
  for ( *zdddf = v341; (uint16_t)v343 < F16(zdddf, 2 * 2); v342 = zdddf )
  {
    v344 = 8 * (uint16_t)v343++;
    F32((*v342 + v344), 0) = 0;
    F32(*zdddf + v344, 4) = 0;
  }
  zdddf[2] = 0;
  zdddf[6] = 0;
  zdddf[15] = settings;
  F16(zdddf, 2 * 32) = 1523;
  zdddf[8] = 160;
  zdddf[9] = 176;
  zdddf[10] = 16711935;
  zdddf[11] = 1;
  zdddf[12] = 0;
  F16(zdddf, 2 * 33) = group;
  zdddf[14] = 0;
  zdddg = SYS32(4824);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4824)));
  zdddg[7] = zdd;
  F16(zdddg, 2 * 2) = 1;
  v345 = (intptr_t)g_eng.op_new(8u);
  v346 = zdddg;
  v347 = 0;
  for ( *zdddg = v345; (uint16_t)v347 < F16(zdddg, 2 * 2); v346 = zdddg )
  {
    v348 = 8 * (uint16_t)v347++;
    F32((v348 + *v346), 0) = 0;
    F32(v348 + *zdddg, 4) = 0;
  }
  zdddg[2] = 0;
  zdddg[6] = 0;
  zdddg[15] = settings;
  F16(zdddg, 2 * 32) = 1524;
  zdddg[8] = 160;
  zdddg[9] = 176;
  zdddg[10] = 16711935;
  zdddg[11] = 1;
  zdddg[12] = 0;
  F16(zdddg, 2 * 33) = group;
  zdddg[14] = 0;
  zdddh = SYS32(4828);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4828)));
  zdddh[7] = zdd;
  F16(zdddh, 2 * 2) = 1;
  v349 = (intptr_t)g_eng.op_new(8u);
  v350 = zdddh;
  v351 = 0;
  for ( *zdddh = v349; (uint16_t)v351 < F16(zdddh, 2 * 2); v350 = zdddh )
  {
    v352 = 8 * (uint16_t)v351++;
    F32((*v350 + v352), 0) = 0;
    F32(*zdddh + v352, 4) = 0;
  }
  zdddh[2] = 0;
  zdddh[6] = 0;
  zdddh[15] = settings;
  F16(zdddh, 2 * 32) = 1525;
  zdddh[8] = 160;
  zdddh[9] = 176;
  zdddh[10] = 16711935;
  zdddh[11] = 1;
  zdddh[12] = 0;
  F16(zdddh, 2 * 33) = group;
  zdddh[14] = 0;
  zdddi = SYS32(4832);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4832)));
  zdddi[7] = zdd;
  F16(zdddi, 2 * 2) = 1;
  v353 = (intptr_t)g_eng.op_new(8u);
  v354 = zdddi;
  v355 = 0;
  for ( *zdddi = v353; (uint16_t)v355 < F16(zdddi, 2 * 2); v354 = zdddi )
  {
    v356 = 8 * (uint16_t)v355++;
    F32((v356 + *v354), 0) = 0;
    F32(v356 + *zdddi, 4) = 0;
  }
  zdddi[2] = 0;
  zdddi[6] = 0;
  zdddi[15] = settings;
  F16(zdddi, 2 * 32) = 1526;
  zdddi[8] = 160;
  zdddi[9] = 176;
  zdddi[10] = 16711935;
  zdddi[11] = 1;
  zdddi[12] = 0;
  F16(zdddi, 2 * 33) = group;
  zdddi[14] = 0;
  zdddj = SYS32(4836);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4836)));
  zdddj[7] = zdd;
  F16(zdddj, 2 * 2) = 1;
  v357 = (intptr_t)g_eng.op_new(8u);
  v358 = zdddj;
  v359 = 0;
  for ( *zdddj = v357; (uint16_t)v359 < F16(zdddj, 2 * 2); v358 = zdddj )
  {
    v360 = 8 * (uint16_t)v359++;
    F32((v360 + *v358), 0) = 0;
    F32(v360 + *zdddj, 4) = 0;
  }
  zdddj[2] = 0;
  zdddj[6] = 0;
  zdddj[15] = settings;
  F16(zdddj, 2 * 32) = 1527;
  zdddj[8] = 160;
  zdddj[9] = 176;
  zdddj[10] = 16711935;
  zdddj[11] = 1;
  zdddj[12] = 0;
  F16(zdddj, 2 * 33) = group;
  zdddj[14] = 0;
  zdddk = SYS32(4840);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4840)));
  zdddk[7] = zdd;
  F16(zdddk, 2 * 2) = 1;
  v361 = (intptr_t)g_eng.op_new(8u);
  v362 = zdddk;
  v363 = 0;
  for ( *zdddk = v361; (uint16_t)v363 < F16(zdddk, 2 * 2); v362 = zdddk )
  {
    v364 = 8 * (uint16_t)v363++;
    F32((*v362 + v364), 0) = 0;
    F32(*zdddk + v364, 4) = 0;
  }
  zdddk[2] = 0;
  zdddk[6] = 0;
  zdddk[15] = settings;
  F16(zdddk, 2 * 32) = 1528;
  zdddk[8] = 160;
  zdddk[9] = 176;
  zdddk[10] = 16711935;
  zdddk[11] = 1;
  zdddk[12] = 0;
  F16(zdddk, 2 * 33) = group;
  zdddk[14] = 0;
  zdddl = SYS32(4844);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4844)));
  zdddl[7] = zdd;
  F16(zdddl, 2 * 2) = 1;
  v365 = (intptr_t)g_eng.op_new(8u);
  v366 = zdddl;
  v367 = 0;
  for ( *zdddl = v365; (uint16_t)v367 < F16(zdddl, 2 * 2); v366 = zdddl )
  {
    v368 = 8 * (uint16_t)v367++;
    F32((v368 + *v366), 0) = 0;
    F32(v368 + *zdddl, 4) = 0;
  }
  zdddl[2] = 0;
  zdddl[6] = 0;
  zdddl[15] = settings;
  F16(zdddl, 2 * 32) = 1529;
  zdddl[8] = 160;
  zdddl[9] = 176;
  zdddl[10] = 16711935;
  zdddl[11] = 1;
  zdddl[12] = 0;
  F16(zdddl, 2 * 33) = group;
  zdddl[14] = 0;
  zdddm = SYS32(4848);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4848)));
  zdddm[7] = zdd;
  F16(zdddm, 2 * 2) = 1;
  v369 = (intptr_t)g_eng.op_new(8u);
  v370 = zdddm;
  v371 = 0;
  for ( *zdddm = v369; (uint16_t)v371 < F16(zdddm, 2 * 2); v370 = zdddm )
  {
    v372 = 8 * (uint16_t)v371++;
    F32((v372 + *v370), 0) = 0;
    F32(v372 + *zdddm, 4) = 0;
  }
  zdddm[2] = 0;
  zdddm[6] = 0;
  zdddm[15] = settings;
  F16(zdddm, 2 * 32) = 1530;
  zdddm[8] = 160;
  zdddm[9] = 176;
  zdddm[10] = 16711935;
  zdddm[11] = 1;
  zdddm[12] = 0;
  F16(zdddm, 2 * 33) = group;
  zdddm[14] = 0;
  zdddn = SYS32(4852);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4852)));
  zdddn[7] = zdd;
  F16(zdddn, 2 * 2) = 1;
  v373 = (intptr_t)g_eng.op_new(8u);
  v374 = zdddn;
  v375 = 0;
  for ( *zdddn = v373; (uint16_t)v375 < F16(zdddn, 2 * 2); v374 = zdddn )
  {
    v376 = 8 * (uint16_t)v375++;
    F32((*v374 + v376), 0) = 0;
    F32(*zdddn + v376, 4) = 0;
  }
  zdddn[2] = 0;
  zdddn[6] = 0;
  zdddn[15] = settings;
  F16(zdddn, 2 * 32) = 1531;
  zdddn[8] = 160;
  zdddn[9] = 176;
  zdddn[10] = 16711935;
  zdddn[11] = 1;
  zdddn[12] = 0;
  F16(zdddn, 2 * 33) = group;
  zdddn[14] = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4856),
    zdd,
    settings,
    0x5FCu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4860),
    zdd,
    settings,
    0x5FEu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4864),
    zdd,
    settings,
    0x600u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4868),
    zdd,
    settings,
    0x606u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4872),
    zdd,
    settings,
    0x607u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4876),
    zdd,
    settings,
    0x7B8u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4880),
    zdd,
    settings,
    0x746u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4884),
    zdd,
    settings,
    0x747u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4888),
    zdd,
    settings,
    0x748u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4892),
    zdd,
    settings,
    0x749u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4896),
    zdd,
    settings,
    0x74Au,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4900),
    zdd,
    settings,
    0x74Bu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4904),
    zdd,
    settings,
    0x74Cu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4908),
    zdd,
    settings,
    0x74Du,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4912),
    zdd,
    settings,
    0x74Eu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4916),
    zdd,
    settings,
    0x74Fu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4920),
    zdd,
    settings,
    0x750u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4924),
    zdd,
    settings,
    0x751u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4928),
    zdd,
    settings,
    0x752u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4932),
    zdd,
    settings,
    0x780u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4936),
    zdd,
    settings,
    0x753u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4940),
    zdd,
    settings,
    0x754u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4944),
    zdd,
    settings,
    0x755u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4948),
    zdd,
    settings,
    0x756u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4952),
    zdd,
    settings,
    0x7B6u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4956),
    zdd,
    settings,
    0x637u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4960),
    zdd,
    settings,
    0x638u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4964),
    zdd,
    settings,
    0x639u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4968),
    zdd,
    settings,
    0x63Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4972),
    zdd,
    settings,
    0x63Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4976),
    zdd,
    settings,
    0x63Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4980),
    zdd,
    settings,
    0x63Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4984),
    zdd,
    settings,
    0x63Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4988),
    zdd,
    settings,
    0x63Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4992),
    zdd,
    settings,
    0x640u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4996),
    zdd,
    settings,
    0x641u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5000),
    zdd,
    settings,
    0x716u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5004),
    zdd,
    settings,
    0x642u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5008),
    zdd,
    settings,
    0x643u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5012),
    zdd,
    settings,
    0x644u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5016),
    zdd,
    settings,
    0x645u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5020),
    zdd,
    settings,
    0x646u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5024),
    zdd,
    settings,
    0x657u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5028),
    zdd,
    settings,
    0x717u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5032),
    zdd,
    settings,
    0x6EDu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5036),
    zdd,
    settings,
    0x647u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5040),
    zdd,
    settings,
    0x648u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5044),
    zdd,
    settings,
    0x649u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5048),
    zdd,
    settings,
    0x64Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5052),
    zdd,
    settings,
    0x64Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5056),
    zdd,
    settings,
    0x64Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5060),
    zdd,
    settings,
    0x64Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5064),
    zdd,
    settings,
    0x64Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5068),
    zdd,
    settings,
    0x64Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5072),
    zdd,
    settings,
    0x650u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5076),
    zdd,
    settings,
    0x651u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5080),
    zdd,
    settings,
    0x718u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5084),
    zdd,
    settings,
    0x652u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5088),
    zdd,
    settings,
    0x653u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5092),
    zdd,
    settings,
    0x654u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5096),
    zdd,
    settings,
    0x655u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5100),
    zdd,
    settings,
    0x656u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5104),
    zdd,
    settings,
    0x658u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5108),
    zdd,
    settings,
    0x719u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5112),
    zdd,
    settings,
    0x6EEu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5116),
    zdd,
    settings,
    0x757u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5120),
    zdd,
    settings,
    0x758u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5124),
    zdd,
    settings,
    0x759u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5128),
    zdd,
    settings,
    0x75Au,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5132),
    zdd,
    settings,
    0x75Bu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5136),
    zdd,
    settings,
    0x75Cu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5140),
    zdd,
    settings,
    0x75Du,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5144),
    zdd,
    settings,
    0x75Eu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5148),
    zdd,
    settings,
    0x75Fu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5152),
    zdd,
    settings,
    0x760u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5156),
    zdd,
    settings,
    0x761u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5160),
    zdd,
    settings,
    0x908u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5164),
    zdd,
    settings,
    0x762u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5168),
    zdd,
    settings,
    0x763u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5172),
    zdd,
    settings,
    0x764u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5176),
    zdd,
    settings,
    0x765u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5180),
    zdd,
    settings,
    0x766u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5184),
    zdd,
    settings,
    0x767u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5188),
    zdd,
    settings,
    0x8C4u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5192),
    zdd,
    settings,
    0x768u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5196),
    zdd,
    settings,
    0x68Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5200),
    zdd,
    settings,
    0x68Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5204),
    zdd,
    settings,
    0x690u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5208),
    zdd,
    settings,
    0x691u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5212),
    zdd,
    settings,
    0x692u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5216),
    zdd,
    settings,
    0x693u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5220),
    zdd,
    settings,
    0x694u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5224),
    zdd,
    settings,
    0x695u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5228),
    zdd,
    settings,
    0x696u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5232),
    zdd,
    settings,
    0x697u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5236),
    zdd,
    settings,
    0x698u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5240),
    zdd,
    settings,
    0x699u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5244),
    zdd,
    settings,
    0x69Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5248),
    zdd,
    settings,
    0x69Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5252),
    zdd,
    settings,
    0x69Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5256),
    zdd,
    settings,
    0x69Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5260),
    zdd,
    settings,
    0x69Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5264),
    zdd,
    settings,
    0x6A9u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5268),
    zdd,
    settings,
    0x67Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5272),
    zdd,
    settings,
    0x67Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5276),
    zdd,
    settings,
    0x67Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5280),
    zdd,
    settings,
    0x680u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5284),
    zdd,
    settings,
    0x681u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5288),
    zdd,
    settings,
    0x682u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5292),
    zdd,
    settings,
    0x683u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5296),
    zdd,
    settings,
    0x684u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5300),
    zdd,
    settings,
    0x685u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5304),
    zdd,
    settings,
    0x686u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5308),
    zdd,
    settings,
    0x687u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5312),
    zdd,
    settings,
    0x688u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5316),
    zdd,
    settings,
    0x689u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5320),
    zdd,
    settings,
    0x68Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5324),
    zdd,
    settings,
    0x68Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5328),
    zdd,
    settings,
    0x68Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5332),
    zdd,
    settings,
    0x68Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5336),
    zdd,
    settings,
    0x6A8u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5340),
    zdd,
    settings,
    0x734u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5344),
    zdd,
    settings,
    0x735u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5348),
    zdd,
    settings,
    0x736u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5352),
    zdd,
    settings,
    0x737u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5356),
    zdd,
    settings,
    0x738u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5360),
    zdd,
    settings,
    0x739u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5364),
    zdd,
    settings,
    0x73Au,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5368),
    zdd,
    settings,
    0x73Bu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5372),
    zdd,
    settings,
    0x73Cu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5376),
    zdd,
    settings,
    0x73Du,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5380),
    zdd,
    settings,
    0x73Eu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5384),
    zdd,
    settings,
    0x73Fu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5388),
    zdd,
    settings,
    0x740u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5392),
    zdd,
    settings,
    0x741u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5396),
    zdd,
    settings,
    0x742u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5400),
    zdd,
    settings,
    0x743u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5404),
    zdd,
    settings,
    0x744u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5408),
    zdd,
    settings,
    0x745u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5412),
    zdd,
    settings,
    0x802u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5416),
    zdd,
    settings,
    0x86Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5420),
    zdd,
    settings,
    0x803u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5424),
    zdd,
    settings,
    0x870u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5428),
    zdd,
    settings,
    0x801u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5432),
    zdd,
    settings,
    0x86Eu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5436),
    zdd,
    settings,
    0x7EFu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5440),
    zdd,
    settings,
    0x82Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5444),
    zdd,
    settings,
    0x82Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5448),
    zdd,
    settings,
    0x82Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5452),
    zdd,
    settings,
    0x82Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5456),
    zdd,
    settings,
    0x891u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5460),
    zdd,
    settings,
    0x892u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5464),
    zdd,
    settings,
    0x7F8u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5468),
    zdd,
    settings,
    0x84Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5472),
    zdd,
    settings,
    0x84Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5476),
    zdd,
    settings,
    0x84Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5480),
    zdd,
    settings,
    0x84Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5484),
    zdd,
    settings,
    0x8A3u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5488),
    zdd,
    settings,
    0x8A4u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5492),
    zdd,
    settings,
    0x7E6u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5496),
    zdd,
    settings,
    0x808u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5500),
    zdd,
    settings,
    0x809u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5504),
    zdd,
    settings,
    0x80Au,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5508),
    zdd,
    settings,
    0x80Bu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5512),
    zdd,
    settings,
    0x87Fu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5516),
    zdd,
    settings,
    0x880u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5520),
    zdd,
    settings,
    0x7F0u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5524),
    zdd,
    settings,
    0x82Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5528),
    zdd,
    settings,
    0x82Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5532),
    zdd,
    settings,
    0x830u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5536),
    zdd,
    settings,
    0x831u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5540),
    zdd,
    settings,
    0x893u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5544),
    zdd,
    settings,
    0x894u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5548),
    zdd,
    settings,
    0x7F9u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5552),
    zdd,
    settings,
    0x850u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5556),
    zdd,
    settings,
    0x851u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5560),
    zdd,
    settings,
    0x852u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5564),
    zdd,
    settings,
    0x853u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5568),
    zdd,
    settings,
    0x8A5u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5572),
    zdd,
    settings,
    0x8A6u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5576),
    zdd,
    settings,
    0x7E7u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5580),
    zdd,
    settings,
    0x80Cu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5584),
    zdd,
    settings,
    0x80Du,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5588),
    zdd,
    settings,
    0x80Eu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5592),
    zdd,
    settings,
    0x80Fu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5596),
    zdd,
    settings,
    0x881u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5600),
    zdd,
    settings,
    0x882u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5604),
    zdd,
    settings,
    0x7F1u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5608),
    zdd,
    settings,
    0x832u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5612),
    zdd,
    settings,
    0x833u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5616),
    zdd,
    settings,
    0x834u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5620),
    zdd,
    settings,
    0x835u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5624),
    zdd,
    settings,
    0x895u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5628),
    zdd,
    settings,
    0x896u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5632),
    zdd,
    settings,
    0x7FAu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5636),
    zdd,
    settings,
    0x854u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5640),
    zdd,
    settings,
    0x855u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5644),
    zdd,
    settings,
    0x856u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5648),
    zdd,
    settings,
    0x857u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5652),
    zdd,
    settings,
    0x8A7u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5656),
    zdd,
    settings,
    0x8A8u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5660),
    zdd,
    settings,
    0x7E8u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5664),
    zdd,
    settings,
    0x810u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5668),
    zdd,
    settings,
    0x811u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5672),
    zdd,
    settings,
    0x812u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5676),
    zdd,
    settings,
    0x813u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5680),
    zdd,
    settings,
    0x883u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5684),
    zdd,
    settings,
    0x884u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5688),
    zdd,
    settings,
    0x7F2u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5692),
    zdd,
    settings,
    0x836u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5696),
    zdd,
    settings,
    0x837u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5700),
    zdd,
    settings,
    0x838u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5704),
    zdd,
    settings,
    0x839u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5708),
    zdd,
    settings,
    0x897u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5712),
    zdd,
    settings,
    0x898u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5716),
    zdd,
    settings,
    0x7FBu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5720),
    zdd,
    settings,
    0x858u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5724),
    zdd,
    settings,
    0x859u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5728),
    zdd,
    settings,
    0x85Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5732),
    zdd,
    settings,
    0x85Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5736),
    zdd,
    settings,
    0x8A9u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5740),
    zdd,
    settings,
    0x8AAu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5744),
    zdd,
    settings,
    0x7E9u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5748),
    zdd,
    settings,
    0x814u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5752),
    zdd,
    settings,
    0x815u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5756),
    zdd,
    settings,
    0x816u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5760),
    zdd,
    settings,
    0x817u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5764),
    zdd,
    settings,
    0x885u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5768),
    zdd,
    settings,
    0x886u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5772),
    zdd,
    settings,
    0x7F3u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5776),
    zdd,
    settings,
    0x83Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5780),
    zdd,
    settings,
    0x83Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5784),
    zdd,
    settings,
    0x83Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5788),
    zdd,
    settings,
    0x83Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5792),
    zdd,
    settings,
    0x899u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5796),
    zdd,
    settings,
    0x89Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5800),
    zdd,
    settings,
    0x7FCu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5804),
    zdd,
    settings,
    0x85Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5808),
    zdd,
    settings,
    0x85Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5812),
    zdd,
    settings,
    0x85Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5816),
    zdd,
    settings,
    0x85Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5820),
    zdd,
    settings,
    0x8ABu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5824),
    zdd,
    settings,
    0x8ACu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5828),
    zdd,
    settings,
    0x7EAu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5832),
    zdd,
    settings,
    0x818u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5836),
    zdd,
    settings,
    0x819u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5840),
    zdd,
    settings,
    0x81Au,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5844),
    zdd,
    settings,
    0x81Bu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5848),
    zdd,
    settings,
    0x887u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5852),
    zdd,
    settings,
    0x888u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5856),
    zdd,
    settings,
    0x7F4u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5860),
    zdd,
    settings,
    0x83Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5864),
    zdd,
    settings,
    0x83Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5868),
    zdd,
    settings,
    0x840u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5872),
    zdd,
    settings,
    0x841u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5876),
    zdd,
    settings,
    0x89Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5880),
    zdd,
    settings,
    0x89Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5884),
    zdd,
    settings,
    0x7FDu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5888),
    zdd,
    settings,
    0x860u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5892),
    zdd,
    settings,
    0x861u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5896),
    zdd,
    settings,
    0x862u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5900),
    zdd,
    settings,
    0x863u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5904),
    zdd,
    settings,
    0x8ADu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5908),
    zdd,
    settings,
    0x8AEu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5912),
    zdd,
    settings,
    0x7EBu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5916),
    zdd,
    settings,
    0x81Cu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5920),
    zdd,
    settings,
    0x81Du,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5924),
    zdd,
    settings,
    0x81Eu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5928),
    zdd,
    settings,
    0x81Fu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5932),
    zdd,
    settings,
    0x889u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5936),
    zdd,
    settings,
    0x88Au,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5940),
    zdd,
    settings,
    0x7F5u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5944),
    zdd,
    settings,
    0x842u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5948),
    zdd,
    settings,
    0x843u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5952),
    zdd,
    settings,
    0x844u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5956),
    zdd,
    settings,
    0x89Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5960),
    zdd,
    settings,
    0x89Eu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5964),
    zdd,
    settings,
    0x7FEu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5968),
    zdd,
    settings,
    0x864u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5972),
    zdd,
    settings,
    0x865u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5976),
    zdd,
    settings,
    0x866u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5980),
    zdd,
    settings,
    0x8AFu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5984),
    zdd,
    settings,
    0x8B0u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5988),
    zdd,
    settings,
    0x7ECu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5992),
    zdd,
    settings,
    0x820u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(5996),
    zdd,
    settings,
    0x821u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6000),
    zdd,
    settings,
    0x822u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6004),
    zdd,
    settings,
    0x88Bu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6008),
    zdd,
    settings,
    0x88Cu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6012),
    zdd,
    settings,
    0x7F6u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6016),
    zdd,
    settings,
    0x845u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6020),
    zdd,
    settings,
    0x846u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6024),
    zdd,
    settings,
    0x847u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6028),
    zdd,
    settings,
    0x848u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6032),
    zdd,
    settings,
    0x89Fu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6036),
    zdd,
    settings,
    0x8A0u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6040),
    zdd,
    settings,
    0x7FFu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6044),
    zdd,
    settings,
    0x867u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6048),
    zdd,
    settings,
    0x868u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6052),
    zdd,
    settings,
    0x869u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6056),
    zdd,
    settings,
    0x86Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6060),
    zdd,
    settings,
    0x8B1u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6064),
    zdd,
    settings,
    0x8B2u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6068),
    zdd,
    settings,
    0x7EDu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6072),
    zdd,
    settings,
    0x823u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6076),
    zdd,
    settings,
    0x824u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6080),
    zdd,
    settings,
    0x825u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6084),
    zdd,
    settings,
    0x826u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6088),
    zdd,
    settings,
    0x88Du,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6092),
    zdd,
    settings,
    0x88Eu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6096),
    zdd,
    settings,
    0x7F7u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6100),
    zdd,
    settings,
    0x849u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6104),
    zdd,
    settings,
    0x84Au,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6108),
    zdd,
    settings,
    0x84Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6112),
    zdd,
    settings,
    0x8A1u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6116),
    zdd,
    settings,
    0x8A2u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6120),
    zdd,
    settings,
    0x800u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6124),
    zdd,
    settings,
    0x86Bu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6128),
    zdd,
    settings,
    0x86Cu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6132),
    zdd,
    settings,
    0x86Du,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6136),
    zdd,
    settings,
    0x8B3u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6140),
    zdd,
    settings,
    0x8B4u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6144),
    zdd,
    settings,
    0x7EEu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6148),
    zdd,
    settings,
    0x827u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6152),
    zdd,
    settings,
    0x828u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6156),
    zdd,
    settings,
    0x829u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6160),
    zdd,
    settings,
    0x88Fu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6164),
    zdd,
    settings,
    0x890u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6168),
    zdd,
    settings,
    0x8D3u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6172),
    zdd,
    settings,
    0x8D4u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6176),
    zdd,
    settings,
    0x8D5u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6180),
    zdd,
    settings,
    0x8D6u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6184),
    zdd,
    settings,
    0x8D7u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6188),
    zdd,
    settings,
    0x8D8u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6192),
    zdd,
    settings,
    0x8D9u,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6196),
    zdd,
    settings,
    0x8DAu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6200),
    zdd,
    settings,
    0x8DBu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6204),
    zdd,
    settings,
    0x8DCu,
    0xA0u,
    0xB0u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6208),
    zdd,
    settings,
    0x8CEu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6212),
    zdd,
    settings,
    0x8CFu,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6216),
    zdd,
    settings,
    0x8D0u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6220),
    zdd,
    settings,
    0x8D1u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6224),
    zdd,
    settings,
    0x8D2u,
    0xB0u,
    0x90u,
    0xFF00FFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6228),
    zdd,
    settings,
    0x580u,
    0x80u,
    0x80u,
    0xFF00FFu,
    0,
    2u,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6236),
    zdd,
    settings,
    0x581u,
    0x80u,
    0x80u,
    0xFF00FFu,
    0,
    2u,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(6232),
    zdd,
    settings,
    0x582u,
    0x80u,
    0x80u,
    0xFF00FFu,
    0,
    2u,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3032),
    zdd,
    settings,
    0x608u,
    0x80u,
    0x80u,
    0xFF00FFu,
    0,
    2u,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3036),
    zdd,
    settings,
    0x609u,
    0x80u,
    0x80u,
    0xFF00FFu,
    0,
    2u,
    group);
  return al_5748c0(
           *(ar_sprite_slot **)&SYSB(3040),
           zdd,
           settings,
           0x60Au,
           0x80u,
           0x80u,
           0xFF00FFu,
           0,
           2u,
           group);
}

/* 0x57ca40: int __stdcall sub_57CA40(void *zdd, unsigned __int16 group, void *settings) */
int32_t al_57ca40(void * zdd, uint16_t group, void * settings)
{

    intptr_t v3 = 0;
    void * v5 = 0;
    intptr_t v6 = 0;
    int v7 = 0;
    intptr_t v8 = 0;
    void * v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    void * v13 = 0;
    int32_t * v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    void * v17 = 0;
    int32_t * v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    void * v21 = 0;
    int32_t * v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    void * v25 = 0;
    int32_t * v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    void * v29 = 0;
    int32_t * v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    void * v33 = 0;
    int32_t * v34 = 0;
    intptr_t v35 = 0;
    intptr_t v36 = 0;
    void * v37 = 0;
    int32_t * v38 = 0;
    intptr_t v39 = 0;
    intptr_t v40 = 0;
    void * v41 = 0;
    int32_t * v42 = 0;
    intptr_t v43 = 0;
    intptr_t v44 = 0;
    void * v45 = 0;
    int32_t * v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    void * v49 = 0;
    int32_t * v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    void * v53 = 0;
    int32_t * v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    void * v57 = 0;
    int32_t * v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    void * v61 = 0;
    int32_t * v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    void * v65 = 0;
    int32_t * v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    void * v69 = 0;
    int32_t * v70 = 0;
    intptr_t v71 = 0;
    intptr_t v72 = 0;
    void * v73 = 0;
    int32_t * v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    void * v77 = 0;
    int32_t * v78 = 0;
    intptr_t v79 = 0;
    intptr_t v80 = 0;
    void * v81 = 0;
    int32_t * v82 = 0;
    intptr_t v83 = 0;
    intptr_t v84 = 0;
    void * v85 = 0;
    int32_t * v86 = 0;
    intptr_t v87 = 0;
    intptr_t v88 = 0;
    void * v89 = 0;
    int32_t * v90 = 0;
    intptr_t v91 = 0;
    intptr_t v92 = 0;
    void * v93 = 0;
    int32_t * v94 = 0;
    intptr_t v95 = 0;
    intptr_t v96 = 0;
    void * v97 = 0;
    int32_t * v98 = 0;
    intptr_t v99 = 0;
    intptr_t v100 = 0;
    void * v101 = 0;
    int32_t * v102 = 0;
    intptr_t v103 = 0;
    intptr_t v104 = 0;
    void * v105 = 0;
    int32_t * v106 = 0;
    intptr_t v107 = 0;
    intptr_t v108 = 0;
    void * v109 = 0;
    int32_t * v110 = 0;
    intptr_t v111 = 0;
    intptr_t v112 = 0;
    void * v113 = 0;
    int32_t * v114 = 0;
    intptr_t v115 = 0;
    intptr_t v116 = 0;
    void * v117 = 0;
    int32_t * v118 = 0;
    intptr_t v119 = 0;
    intptr_t v120 = 0;
    void * v121 = 0;
    int32_t * v122 = 0;
    intptr_t v123 = 0;
    intptr_t v124 = 0;
    void * v125 = 0;
    int32_t * v126 = 0;
    intptr_t v127 = 0;
    intptr_t v128 = 0;
    void * v129 = 0;
    int32_t * v130 = 0;
    intptr_t v131 = 0;
    intptr_t v132 = 0;
    void * v133 = 0;
    int32_t * v134 = 0;
    intptr_t v135 = 0;
    intptr_t v136 = 0;
    void * v137 = 0;
    int32_t * v138 = 0;
    intptr_t v139 = 0;
    intptr_t v140 = 0;
    void * v141 = 0;
    int32_t * v142 = 0;
    intptr_t v143 = 0;
    intptr_t v144 = 0;
    void * v145 = 0;
    int32_t * v146 = 0;
    intptr_t v147 = 0;
    intptr_t v148 = 0;
    void * v149 = 0;
    int32_t * v150 = 0;
    intptr_t v151 = 0;
    intptr_t v152 = 0;
    void * v153 = 0;
    int32_t * v154 = 0;
    intptr_t v155 = 0;
    intptr_t v156 = 0;
    void * v157 = 0;
    int32_t * v158 = 0;
    intptr_t v159 = 0;
    intptr_t v160 = 0;
    void * v161 = 0;
    int32_t * v162 = 0;
    intptr_t v163 = 0;
    intptr_t v164 = 0;
    void * v165 = 0;
    int32_t * v166 = 0;
    intptr_t v167 = 0;
    intptr_t v168 = 0;
    void * v169 = 0;
    int32_t * v170 = 0;
    intptr_t v171 = 0;
    intptr_t v172 = 0;
    void * v173 = 0;
    int32_t * v174 = 0;
    intptr_t v175 = 0;
    intptr_t v176 = 0;
    void * v177 = 0;
    int32_t * v178 = 0;
    intptr_t v179 = 0;
    intptr_t v180 = 0;
    void * v181 = 0;
    int32_t * v182 = 0;
    intptr_t v183 = 0;
    intptr_t v184 = 0;
    void * v185 = 0;
    int32_t * v186 = 0;
    intptr_t v187 = 0;
    intptr_t v188 = 0;
    void * v189 = 0;
    int32_t * v190 = 0;
    intptr_t v191 = 0;
    intptr_t v192 = 0;
    void * v193 = 0;
    int32_t * v194 = 0;
    intptr_t v195 = 0;
    intptr_t v196 = 0;
    void * v197 = 0;
    int32_t * v198 = 0;
    intptr_t v199 = 0;
    intptr_t v200 = 0;
    void * v201 = 0;
    int32_t * v202 = 0;
    intptr_t v203 = 0;
    intptr_t v204 = 0;
    void * v205 = 0;
    int32_t * v206 = 0;
    intptr_t v207 = 0;
    intptr_t v208 = 0;
    void * v209 = 0;
    int32_t * v210 = 0;
    intptr_t v211 = 0;
    intptr_t v212 = 0;
    void * v213 = 0;
    int32_t * v214 = 0;
    intptr_t v215 = 0;
    intptr_t v216 = 0;
    void * v217 = 0;
    int32_t * v218 = 0;
    intptr_t v219 = 0;
    intptr_t v220 = 0;
    void * v221 = 0;
    int32_t * v222 = 0;
    intptr_t v223 = 0;
    intptr_t v224 = 0;
    void * v225 = 0;
    int32_t * v226 = 0;
    intptr_t v227 = 0;
    intptr_t v228 = 0;
    void * v229 = 0;
    int32_t * v230 = 0;
    intptr_t v231 = 0;
    intptr_t v232 = 0;
    void * v233 = 0;
    int32_t * v234 = 0;
    intptr_t v235 = 0;
    intptr_t v236 = 0;
    void * v237 = 0;
    int32_t * v238 = 0;
    intptr_t v239 = 0;
    intptr_t v240 = 0;
    void * v241 = 0;
    int32_t * v242 = 0;
    intptr_t v243 = 0;
    intptr_t v244 = 0;
    void * v245 = 0;
    int32_t * v246 = 0;
    intptr_t v247 = 0;
    intptr_t v248 = 0;
    void * v249 = 0;
    int32_t * v250 = 0;
    intptr_t v251 = 0;
    intptr_t v252 = 0;
    void * v253 = 0;
    int32_t * v254 = 0;
    intptr_t v255 = 0;
    intptr_t v256 = 0;
    void * v257 = 0;
    int32_t * v258 = 0;
    intptr_t v259 = 0;
    intptr_t v260 = 0;
    void * v261 = 0;
    int32_t * v262 = 0;
    intptr_t v263 = 0;
    intptr_t v264 = 0;
    void * v265 = 0;
    int32_t * v266 = 0;
    intptr_t v267 = 0;
    intptr_t v268 = 0;
    void * v269 = 0;
    int32_t * v270 = 0;
    intptr_t v271 = 0;
    intptr_t v272 = 0;
    void * v273 = 0;
    int32_t * v274 = 0;
    intptr_t v275 = 0;
    intptr_t v276 = 0;
    void * v277 = 0;
    int32_t * v278 = 0;
    intptr_t v279 = 0;
    intptr_t v280 = 0;
    void * v281 = 0;
    int32_t * v282 = 0;
    intptr_t v283 = 0;
    intptr_t v284 = 0;
    void * v285 = 0;
    int32_t * v286 = 0;
    intptr_t v287 = 0;
    intptr_t v288 = 0;
    void * v289 = 0;
    int32_t * v290 = 0;
    intptr_t v291 = 0;
    intptr_t v292 = 0;
    void * v293 = 0;
    int32_t * v294 = 0;
    intptr_t v295 = 0;
    intptr_t v296 = 0;
    void * v297 = 0;
    int32_t * v298 = 0;
    intptr_t v299 = 0;
    intptr_t v300 = 0;
    void * v301 = 0;
    int32_t * v302 = 0;
    intptr_t v303 = 0;
    intptr_t v304 = 0;
    void * v305 = 0;
    int32_t * v306 = 0;
    intptr_t v307 = 0;
    intptr_t v308 = 0;
    void * v309 = 0;
    int32_t * v310 = 0;
    intptr_t v311 = 0;
    intptr_t v312 = 0;
    void * v313 = 0;
    int32_t * v314 = 0;
    intptr_t v315 = 0;
    intptr_t v316 = 0;
    void * v317 = 0;
    int32_t * v318 = 0;
    intptr_t v319 = 0;
    intptr_t v320 = 0;
    void * v321 = 0;
    int32_t * v322 = 0;
    intptr_t v323 = 0;
    intptr_t v324 = 0;
    void * v325 = 0;
    int32_t * v326 = 0;
    intptr_t v327 = 0;
    intptr_t v328 = 0;
    void * v329 = 0;
    int32_t * v330 = 0;
    intptr_t v331 = 0;
    intptr_t v332 = 0;
    void * v333 = 0;
    int32_t * v334 = 0;
    intptr_t v335 = 0;
    intptr_t v336 = 0;
    void * v337 = 0;
    int32_t * v338 = 0;
    intptr_t v339 = 0;
    intptr_t v340 = 0;
    void * v341 = 0;
    int32_t * v342 = 0;
    intptr_t v343 = 0;
    intptr_t v344 = 0;
    void * v345 = 0;
    int32_t * v346 = 0;
    intptr_t v347 = 0;
    intptr_t v348 = 0;
    void * v349 = 0;
    int32_t * v350 = 0;
    intptr_t v351 = 0;
    intptr_t v352 = 0;
    void * v353 = 0;
    int32_t * v354 = 0;
    intptr_t v355 = 0;
    intptr_t v356 = 0;
    void * v357 = 0;
    int32_t * v358 = 0;
    intptr_t v359 = 0;
    intptr_t v360 = 0;
    void * v361 = 0;
    int32_t * v362 = 0;
    intptr_t v363 = 0;
    intptr_t v364 = 0;
    void * v365 = 0;
    int32_t * v366 = 0;
    intptr_t v367 = 0;
    intptr_t v368 = 0;
    intptr_t result = 0;
    int32_t * zdda = 0;
    int32_t * zddb = 0;
    int32_t * zddc = 0;
    int32_t * zddd = 0;
    int32_t * zdde = 0;
    int32_t * zddf = 0;
    int32_t * zddg = 0;
    int32_t * zddh = 0;
    int32_t * zddi = 0;
    int32_t * zddj = 0;
    int32_t * zddk = 0;
    int32_t * zddl = 0;
    int32_t * zddm = 0;
    int32_t * zddn = 0;
    int32_t * zddo = 0;
    int32_t * zddp = 0;
    int32_t * zddq = 0;
    int32_t * zddr = 0;
    int32_t * zdds = 0;
    int32_t * zddt = 0;
    int32_t * zddu = 0;
    int32_t * zddv = 0;
    int32_t * zddw = 0;
    int32_t * zddx = 0;
    int32_t * zddy = 0;
    int32_t * zddz = 0;
    int32_t * zddba = 0;
    int32_t * zddbb = 0;
    int32_t * zddbc = 0;
    int32_t * zddbd = 0;
    int32_t * zddbe = 0;
    int32_t * zddbf = 0;
    int32_t * zddbg = 0;
    int32_t * zddbh = 0;
    int32_t * zddbi = 0;
    int32_t * zddbj = 0;
    int32_t * zddbk = 0;
    int32_t * zddbl = 0;
    int32_t * zddbm = 0;
    int32_t * zddbn = 0;
    int32_t * zddbo = 0;
    int32_t * zddbp = 0;
    int32_t * zddbq = 0;
    int32_t * zddbr = 0;
    int32_t * zddbs = 0;
    int32_t * zddbt = 0;
    int32_t * zddbu = 0;
    int32_t * zddbv = 0;
    int32_t * zddbw = 0;
    int32_t * zddbx = 0;
    int32_t * zddby = 0;
    int32_t * zddbz = 0;
    int32_t * zddca = 0;
    int32_t * zddcb = 0;
    int32_t * zddcc = 0;
    int32_t * zddcd = 0;
    int32_t * zddce = 0;
    int32_t * zddcf = 0;
    int32_t * zddcg = 0;
    int32_t * zddch = 0;
    int32_t * zddci = 0;
    int32_t * zddcj = 0;
    int32_t * zddck = 0;
    int32_t * zddcl = 0;
    int32_t * zddcm = 0;
    int32_t * zddcn = 0;
    int32_t * zddco = 0;
    int32_t * zddcp = 0;
    int32_t * zddcq = 0;
    int32_t * zddcr = 0;
    int32_t * zddcs = 0;
    int32_t * zddct = 0;
    int32_t * zddcu = 0;
    int32_t * zddcv = 0;
    int32_t * zddcw = 0;
    int32_t * zddcx = 0;
    int32_t * zddcy = 0;
    int32_t * zddcz = 0;
    int32_t * zddda = 0;
    int32_t * zdddb = 0;
    int32_t * zdddc = 0;
    int32_t * zdddd = 0;
    int32_t * zddde = 0;
    int32_t * zdddf = 0;
    int32_t * zdddg = 0;
    int32_t * zdddh = 0;
    int32_t * zdddi = 0;
    int32_t * zdddj = 0;
    int32_t * zdddk = 0;
    int32_t * zdddl = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v370__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v3 = SYS32(3100);
  (*v370__f) = SYS32(3100);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3100)));
  F32(v3, 28) = zdd;
  F16(v3, 4) = 1;
  v5 = (intptr_t)g_eng.op_new(8u);
  v6 = 0;
  v7 = F16(v3, 4) == 0;
  F32(v3, 0) = v5;
  if ( !v7 )
  {
    do
    {
      v8 = 8 * (uint16_t)v6++;
      F32((F32(v3, 0) + v8), 0) = 0;
      F32(F32(v3, 0) + v8, 4) = 0;
    }
    while ( (uint16_t)v6 < F16(v3, 4) );
  }
  F32((*v370__f), 8) = 0;
  F32((*v370__f), 24) = 0;
  F32((*v370__f), 60) = settings;
  F16((*v370__f), 64) = 1059;
  F32((*v370__f), 32) = 32;
  F32((*v370__f), 36) = 32;
  F32((*v370__f), 40) = 0;
  F32((*v370__f), 44) = 0;
  F32((*v370__f), 48) = 2;
  F16((*v370__f), 66) = group;
  F32((*v370__f), 56) = 0;
  F32(SYS32(6736), 4) = 1;
  zdda = SYS32(3104);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3104)));
  zdda[7] = zdd;
  F16(zdda, 2 * 2) = 1;
  v9 = (intptr_t)g_eng.op_new(8u);
  v10 = zdda;
  v11 = 0;
  v7 = F16(zdda, 2 * 2) == 0;
  *zdda = v9;
  if ( !v7 )
  {
    do
    {
      v12 = 8 * (uint16_t)v11++;
      F32((*v10 + v12), 0) = 0;
      F32(*zdda + v12, 4) = 0;
      v10 = zdda;
    }
    while ( (uint16_t)v11 < F16(zdda, 2 * 2) );
  }
  zdda[2] = 0;
  zdda[6] = 0;
  zdda[15] = settings;
  F16(zdda, 2 * 32) = 1060;
  zdda[8] = 64;
  zdda[9] = 64;
  zdda[10] = 0;
  zdda[11] = 0;
  zdda[12] = 2;
  F16(zdda, 2 * 33) = group;
  zdda[14] = 0;
  F32(SYS32(6740), 4) = 1;
  zddb = SYS32(3124);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3124)));
  zddb[7] = zdd;
  F16(zddb, 2 * 2) = 1;
  v13 = (intptr_t)g_eng.op_new(8u);
  v14 = zddb;
  v15 = 0;
  v7 = F16(zddb, 2 * 2) == 0;
  *zddb = v13;
  if ( !v7 )
  {
    do
    {
      v16 = 8 * (uint16_t)v15++;
      F32((*v14 + v16), 0) = 0;
      F32(*zddb + v16, 4) = 0;
      v14 = zddb;
    }
    while ( (uint16_t)v15 < F16(zddb, 2 * 2) );
  }
  zddb[2] = 0;
  zddb[6] = 0;
  zddb[15] = settings;
  F16(zddb, 2 * 32) = 1075;
  zddb[8] = 64;
  zddb[9] = 64;
  zddb[10] = 0;
  zddb[11] = 0;
  zddb[12] = 2;
  F16(zddb, 2 * 33) = group;
  zddb[14] = 0;
  F32(SYS32(6760), 4) = 1;
  zddc = SYS32(3128);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3128)));
  zddc[7] = zdd;
  F16(zddc, 2 * 2) = 1;
  v17 = (intptr_t)g_eng.op_new(8u);
  v18 = zddc;
  v19 = 0;
  v7 = F16(zddc, 2 * 2) == 0;
  *zddc = v17;
  if ( !v7 )
  {
    do
    {
      v20 = 8 * (uint16_t)v19++;
      F32((v20 + *v18), 0) = 0;
      F32(v20 + *zddc, 4) = 0;
      v18 = zddc;
    }
    while ( (uint16_t)v19 < F16(zddc, 2 * 2) );
  }
  zddc[2] = 0;
  zddc[6] = 0;
  zddc[15] = settings;
  F16(zddc, 2 * 32) = 1076;
  zddc[8] = 128;
  zddc[9] = 128;
  zddc[10] = 0;
  zddc[11] = 0;
  zddc[12] = 2;
  F16(zddc, 2 * 33) = group;
  zddc[14] = 0;
  F32(SYS32(6764), 4) = 1;
  zddd = SYS32(3108);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3108)));
  zddd[7] = zdd;
  F16(zddd, 2 * 2) = 1;
  v21 = (intptr_t)g_eng.op_new(8u);
  v22 = zddd;
  v23 = 0;
  v7 = F16(zddd, 2 * 2) == 0;
  *zddd = v21;
  if ( !v7 )
  {
    do
    {
      v24 = 8 * (uint16_t)v23++;
      F32((v24 + *v22), 0) = 0;
      F32(v24 + *zddd, 4) = 0;
      v22 = zddd;
    }
    while ( (uint16_t)v23 < F16(zddd, 2 * 2) );
  }
  zddd[2] = 0;
  zddd[6] = 0;
  zddd[15] = settings;
  F16(zddd, 2 * 32) = 1702;
  zddd[8] = 192;
  zddd[9] = 160;
  zddd[10] = 0x1FFFFFF;
  zddd[11] = 0;
  zddd[12] = 0;
  F16(zddd, 2 * 33) = group;
  zddd[14] = 0;
  F32(SYS32(6744), 4) = 1;
  zdde = SYS32(3112);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3112)));
  zdde[7] = zdd;
  F16(zdde, 2 * 2) = 1;
  v25 = (intptr_t)g_eng.op_new(8u);
  v26 = zdde;
  v27 = 0;
  v7 = F16(zdde, 2 * 2) == 0;
  *zdde = v25;
  if ( !v7 )
  {
    do
    {
      v28 = 8 * (uint16_t)v27++;
      F32((v28 + *v26), 0) = 0;
      F32(v28 + *zdde, 4) = 0;
      v26 = zdde;
    }
    while ( (uint16_t)v27 < F16(zdde, 2 * 2) );
  }
  zdde[2] = 0;
  zdde[6] = 0;
  zdde[15] = settings;
  F16(zdde, 2 * 32) = 1703;
  zdde[8] = 128;
  zdde[9] = 160;
  zdde[10] = 0x1FFFFFF;
  zdde[11] = 0;
  zdde[12] = 0;
  F16(zdde, 2 * 33) = group;
  zdde[14] = 0;
  F32(SYS32(6748), 4) = 1;
  zddf = SYS32(3116);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3116)));
  zddf[7] = zdd;
  F16(zddf, 2 * 2) = 1;
  v29 = (intptr_t)g_eng.op_new(8u);
  v30 = zddf;
  v31 = 0;
  v7 = F16(zddf, 2 * 2) == 0;
  *zddf = v29;
  if ( !v7 )
  {
    do
    {
      v32 = 8 * (uint16_t)v31++;
      F32((v32 + *v30), 0) = 0;
      F32(v32 + *zddf, 4) = 0;
      v30 = zddf;
    }
    while ( (uint16_t)v31 < F16(zddf, 2 * 2) );
  }
  zddf[2] = 0;
  zddf[6] = 0;
  zddf[15] = settings;
  F16(zddf, 2 * 32) = 1061;
  zddf[8] = 64;
  zddf[9] = 128;
  zddf[10] = 0;
  zddf[11] = 0;
  zddf[12] = 2;
  F16(zddf, 2 * 33) = group;
  zddf[14] = 0;
  F32(SYS32(6752), 4) = 1;
  zddg = SYS32(3120);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3120)));
  zddg[7] = zdd;
  F16(zddg, 2 * 2) = 1;
  v33 = (intptr_t)g_eng.op_new(8u);
  v34 = zddg;
  v35 = 0;
  v7 = F16(zddg, 2 * 2) == 0;
  *zddg = v33;
  if ( !v7 )
  {
    do
    {
      v36 = 8 * (uint16_t)v35++;
      F32((*v34 + v36), 0) = 0;
      F32(*zddg + v36, 4) = 0;
      v34 = zddg;
    }
    while ( (uint16_t)v35 < F16(zddg, 2 * 2) );
  }
  zddg[2] = 0;
  zddg[6] = 0;
  zddg[15] = settings;
  F16(zddg, 2 * 32) = 1080;
  zddg[8] = 32;
  zddg[9] = 48;
  zddg[10] = 0;
  zddg[11] = 1;
  zddg[12] = 0;
  F16(zddg, 2 * 33) = group;
  zddg[14] = 0;
  (*(uint16_t *)SYS32(6756)) = 0;
  F32(SYS32(6756), 4) = 1;
  zddh = SYS32(3132);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3132)));
  zddh[7] = zdd;
  F16(zddh, 2 * 2) = 1;
  v37 = (intptr_t)g_eng.op_new(8u);
  v38 = zddh;
  v39 = 0;
  v7 = F16(zddh, 2 * 2) == 0;
  *zddh = v37;
  if ( !v7 )
  {
    do
    {
      v40 = 8 * (uint16_t)v39++;
      F32((*v38 + v40), 0) = 0;
      F32(*zddh + v40, 4) = 0;
      v38 = zddh;
    }
    while ( (uint16_t)v39 < F16(zddh, 2 * 2) );
  }
  zddh[2] = 0;
  zddh[6] = 0;
  zddh[15] = settings;
  F16(zddh, 2 * 32) = 1052;
  zddh[8] = 32;
  zddh[9] = 32;
  zddh[10] = 0x1FFFFFF;
  zddh[11] = 0;
  zddh[12] = 0;
  F16(zddh, 2 * 33) = group;
  zddh[14] = 0;
  F32(SYS32(6768), 4) = 1;
  zddi = SYS32(3136);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3136)));
  zddi[7] = zdd;
  F16(zddi, 2 * 2) = 1;
  v41 = (intptr_t)g_eng.op_new(8u);
  v42 = zddi;
  v43 = 0;
  v7 = F16(zddi, 2 * 2) == 0;
  *zddi = v41;
  if ( !v7 )
  {
    do
    {
      v44 = 8 * (uint16_t)v43++;
      F32((*v42 + v44), 0) = 0;
      F32(*zddi + v44, 4) = 0;
      v42 = zddi;
    }
    while ( (uint16_t)v43 < F16(zddi, 2 * 2) );
  }
  zddi[2] = 0;
  zddi[6] = 0;
  zddi[15] = settings;
  F16(zddi, 2 * 32) = 2330;
  zddi[8] = 32;
  zddi[9] = 32;
  zddi[10] = 0x1FFFFFF;
  zddi[11] = 0;
  zddi[12] = 0;
  F16(zddi, 2 * 33) = group;
  zddi[14] = 0;
  F32(SYS32(6772), 4) = 1;
  zddj = SYS32(3140);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3140)));
  zddj[7] = zdd;
  F16(zddj, 2 * 2) = 1;
  v45 = (intptr_t)g_eng.op_new(8u);
  v46 = zddj;
  v47 = 0;
  v7 = F16(zddj, 2 * 2) == 0;
  *zddj = v45;
  if ( !v7 )
  {
    do
    {
      v48 = 8 * (uint16_t)v47++;
      F32((v48 + *v46), 0) = 0;
      F32(v48 + *zddj, 4) = 0;
      v46 = zddj;
    }
    while ( (uint16_t)v47 < F16(zddj, 2 * 2) );
  }
  zddj[2] = 0;
  zddj[6] = 0;
  zddj[15] = settings;
  F16(zddj, 2 * 32) = 1055;
  zddj[8] = 64;
  zddj[9] = 64;
  zddj[10] = 0x1FFFFFF;
  zddj[11] = 0;
  zddj[12] = 0;
  F16(zddj, 2 * 33) = group;
  zddj[14] = 0;
  F32(SYS32(6776), 4) = 1;
  zddk = SYS32(3144);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3144)));
  zddk[7] = zdd;
  F16(zddk, 2 * 2) = 1;
  v49 = (intptr_t)g_eng.op_new(8u);
  v50 = zddk;
  v51 = 0;
  for ( *zddk = v49; (uint16_t)v51 < F16(zddk, 2 * 2); v50 = zddk )
  {
    v52 = 8 * (uint16_t)v51++;
    F32((v52 + *v50), 0) = 0;
    F32(v52 + *zddk, 4) = 0;
  }
  zddk[2] = 0;
  zddk[6] = 0;
  zddk[15] = settings;
  F16(zddk, 2 * 32) = 1051;
  zddk[8] = 128;
  zddk[9] = 128;
  zddk[10] = 0;
  zddk[11] = 0;
  zddk[12] = 2;
  F16(zddk, 2 * 33) = group;
  zddk[14] = 0;
  F32(SYS32(6780), 4) = 1;
  zddl = SYS32(3152);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3152)));
  zddl[7] = zdd;
  F16(zddl, 2 * 2) = 1;
  v53 = (intptr_t)g_eng.op_new(8u);
  v54 = zddl;
  v55 = 0;
  for ( *zddl = v53; (uint16_t)v55 < F16(zddl, 2 * 2); v54 = zddl )
  {
    v56 = 8 * (uint16_t)v55++;
    F32((*v54 + v56), 0) = 0;
    F32(*zddl + v56, 4) = 0;
  }
  zddl[2] = 0;
  zddl[6] = 0;
  zddl[15] = settings;
  F16(zddl, 2 * 32) = 1057;
  zddl[8] = 64;
  zddl[9] = 64;
  zddl[10] = 0;
  zddl[11] = 0;
  zddl[12] = 2;
  F16(zddl, 2 * 33) = group;
  zddl[14] = 0;
  F32(SYS32(6788), 4) = 1;
  zddm = SYS32(3148);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3148)));
  zddm[7] = zdd;
  F16(zddm, 2 * 2) = 1;
  v57 = (intptr_t)g_eng.op_new(8u);
  v58 = zddm;
  v59 = 0;
  for ( *zddm = v57; (uint16_t)v59 < F16(zddm, 2 * 2); v58 = zddm )
  {
    v60 = 8 * (uint16_t)v59++;
    F32((v60 + *v58), 0) = 0;
    F32(v60 + *zddm, 4) = 0;
  }
  zddm[2] = 0;
  zddm[6] = 0;
  zddm[15] = settings;
  F16(zddm, 2 * 32) = 1054;
  zddm[8] = 32;
  zddm[9] = 32;
  zddm[10] = 0;
  zddm[11] = 0;
  zddm[12] = 2;
  F16(zddm, 2 * 33) = group;
  zddm[14] = 0;
  F32(SYS32(6784), 4) = 1;
  zddn = SYS32(3156);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3156)));
  zddn[7] = zdd;
  F16(zddn, 2 * 2) = 1;
  v61 = (intptr_t)g_eng.op_new(8u);
  v62 = zddn;
  v63 = 0;
  for ( *zddn = v61; (uint16_t)v63 < F16(zddn, 2 * 2); v62 = zddn )
  {
    v64 = 8 * (uint16_t)v63++;
    F32((v64 + *v62), 0) = 0;
    F32(v64 + *zddn, 4) = 0;
  }
  zddn[2] = 0;
  zddn[6] = 0;
  zddn[15] = settings;
  F16(zddn, 2 * 32) = 1058;
  zddn[8] = 192;
  zddn[9] = 128;
  zddn[10] = 0x1FFFFFF;
  zddn[11] = 1;
  zddn[12] = 0;
  F16(zddn, 2 * 33) = group;
  zddn[14] = 0;
  F32(SYS32(6792), 4) = 1;
  zddo = SYS32(3180);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3180)));
  zddo[7] = zdd;
  F16(zddo, 2 * 2) = 1;
  v65 = (intptr_t)g_eng.op_new(8u);
  v66 = zddo;
  v67 = 0;
  for ( *zddo = v65; (uint16_t)v67 < F16(zddo, 2 * 2); v66 = zddo )
  {
    v68 = 8 * (uint16_t)v67++;
    F32((v68 + *v66), 0) = 0;
    F32(v68 + *zddo, 4) = 0;
  }
  zddo[2] = 0;
  zddo[6] = 0;
  zddo[15] = settings;
  F16(zddo, 2 * 32) = 1765;
  zddo[8] = 64;
  zddo[9] = 64;
  zddo[10] = 0;
  zddo[11] = 0;
  zddo[12] = 2;
  F16(zddo, 2 * 33) = group;
  zddo[14] = 0;
  F32(SYS32(6816), 4) = 1;
  zddp = SYS32(3184);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3184)));
  zddp[7] = zdd;
  F16(zddp, 2 * 2) = 1;
  v69 = (intptr_t)g_eng.op_new(8u);
  v70 = zddp;
  v71 = 0;
  for ( *zddp = v69; (uint16_t)v71 < F16(zddp, 2 * 2); v70 = zddp )
  {
    v72 = 8 * (uint16_t)v71++;
    F32((v72 + *v70), 0) = 0;
    F32(v72 + *zddp, 4) = 0;
  }
  zddp[2] = 0;
  zddp[6] = 0;
  zddp[15] = settings;
  F16(zddp, 2 * 32) = 2319;
  zddp[8] = 64;
  zddp[9] = 64;
  zddp[10] = 0;
  zddp[11] = 0;
  zddp[12] = 2;
  F16(zddp, 2 * 33) = group;
  zddp[14] = 0;
  F32(SYS32(6820), 4) = 1;
  zddq = SYS32(3160);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3160)));
  zddq[7] = zdd;
  F16(zddq, 2 * 2) = 1;
  v73 = (intptr_t)g_eng.op_new(8u);
  v74 = zddq;
  v75 = 0;
  for ( *zddq = v73; (uint16_t)v75 < F16(zddq, 2 * 2); v74 = zddq )
  {
    v76 = 8 * (uint16_t)v75++;
    F32((v76 + *v74), 0) = 0;
    F32(v76 + *zddq, 4) = 0;
  }
  zddq[2] = 0;
  zddq[6] = 0;
  zddq[15] = settings;
  F16(zddq, 2 * 32) = 1960;
  zddq[8] = 32;
  zddq[9] = 32;
  zddq[10] = 0x1FFFFFF;
  zddq[11] = 1;
  zddq[12] = 0;
  F16(zddq, 2 * 33) = group;
  zddq[14] = 0;
  zddr = SYS32(3164);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3164)));
  zddr[7] = zdd;
  F16(zddr, 2 * 2) = 1;
  v77 = (intptr_t)g_eng.op_new(8u);
  v78 = zddr;
  v79 = 0;
  for ( *zddr = v77; (uint16_t)v79 < F16(zddr, 2 * 2); v78 = zddr )
  {
    v80 = 8 * (uint16_t)v79++;
    F32((v80 + *v78), 0) = 0;
    F32(v80 + *zddr, 4) = 0;
  }
  zddr[2] = 0;
  zddr[6] = 0;
  zddr[15] = settings;
  F16(zddr, 2 * 32) = 2055;
  zddr[8] = 32;
  zddr[9] = 32;
  zddr[10] = 0x1FFFFFF;
  zddr[11] = 1;
  zddr[12] = 0;
  F16(zddr, 2 * 33) = group;
  zddr[14] = 0;
  zdds = SYS32(3188);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3188)));
  zdds[7] = zdd;
  F16(zdds, 2 * 2) = 1;
  v81 = (intptr_t)g_eng.op_new(8u);
  v82 = zdds;
  v83 = 0;
  for ( *zdds = v81; (uint16_t)v83 < F16(zdds, 2 * 2); v82 = zdds )
  {
    v84 = 8 * (uint16_t)v83++;
    F32((v84 + *v82), 0) = 0;
    F32(v84 + *zdds, 4) = 0;
  }
  zdds[2] = 0;
  zdds[6] = 0;
  zdds[15] = settings;
  F16(zdds, 2 * 32) = 1635;
  zdds[8] = 32;
  zdds[9] = 32;
  zdds[10] = 0;
  zdds[11] = 0;
  zdds[12] = 2;
  F16(zdds, 2 * 33) = group;
  zdds[14] = 0;
  F32(SYS32(6824), 4) = 1;
  zddt = SYS32(3192);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3192)));
  zddt[7] = zdd;
  F16(zddt, 2 * 2) = 1;
  v85 = (intptr_t)g_eng.op_new(8u);
  v86 = zddt;
  v87 = 0;
  for ( *zddt = v85; (uint16_t)v87 < F16(zddt, 2 * 2); v86 = zddt )
  {
    v88 = 8 * (uint16_t)v87++;
    F32((v88 + *v86), 0) = 0;
    F32(v88 + *zddt, 4) = 0;
  }
  zddt[2] = 0;
  zddt[6] = 0;
  zddt[15] = settings;
  F16(zddt, 2 * 32) = 1973;
  zddt[8] = 32;
  zddt[9] = 32;
  zddt[10] = 0;
  zddt[11] = 0;
  zddt[12] = 2;
  F16(zddt, 2 * 33) = group;
  zddt[14] = 0;
  F32(SYS32(6828), 4) = 1;
  zddu = SYS32(3196);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3196)));
  zddu[7] = zdd;
  F16(zddu, 2 * 2) = 1;
  v89 = (intptr_t)g_eng.op_new(8u);
  v90 = zddu;
  v91 = 0;
  for ( *zddu = v89; (uint16_t)v91 < F16(zddu, 2 * 2); v90 = zddu )
  {
    v92 = 8 * (uint16_t)v91++;
    F32((v92 + *v90), 0) = 0;
    F32(v92 + *zddu, 4) = 0;
  }
  zddu[2] = 0;
  zddu[6] = 0;
  zddu[15] = settings;
  F16(zddu, 2 * 32) = 1636;
  zddu[8] = 64;
  zddu[9] = 64;
  zddu[10] = 0;
  zddu[11] = 0;
  zddu[12] = 2;
  F16(zddu, 2 * 33) = group;
  zddu[14] = 0;
  F32(SYS32(6832), 4) = 1;
  zddv = SYS32(3200);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3200)));
  zddv[7] = zdd;
  F16(zddv, 2 * 2) = 1;
  v93 = (intptr_t)g_eng.op_new(8u);
  v94 = zddv;
  v95 = 0;
  for ( *zddv = v93; (uint16_t)v95 < F16(zddv, 2 * 2); v94 = zddv )
  {
    v96 = 8 * (uint16_t)v95++;
    F32((*v94 + v96), 0) = 0;
    F32(*zddv + v96, 4) = 0;
  }
  zddv[2] = 0;
  zddv[6] = 0;
  zddv[15] = settings;
  F16(zddv, 2 * 32) = 1634;
  zddv[8] = 128;
  zddv[9] = 128;
  zddv[10] = 0;
  zddv[11] = 0;
  zddv[12] = 2;
  F16(zddv, 2 * 33) = group;
  zddv[14] = 0;
  F32(SYS32(6836), 4) = 1;
  zddw = SYS32(3204);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3204)));
  zddw[7] = zdd;
  F16(zddw, 2 * 2) = 1;
  v97 = (intptr_t)g_eng.op_new(8u);
  v98 = zddw;
  v99 = 0;
  for ( *zddw = v97; (uint16_t)v99 < F16(zddw, 2 * 2); v98 = zddw )
  {
    v100 = 8 * (uint16_t)v99++;
    F32((v100 + *v98), 0) = 0;
    F32(v100 + *zddw, 4) = 0;
  }
  zddw[2] = 0;
  zddw[6] = 0;
  zddw[15] = settings;
  F16(zddw, 2 * 32) = 1969;
  zddw[8] = 192;
  zddw[9] = 128;
  zddw[10] = 0;
  zddw[11] = 0;
  zddw[12] = 2;
  F16(zddw, 2 * 33) = group;
  zddw[14] = 0;
  F32(SYS32(6840), 4) = 1;
  zddx = SYS32(3208);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3208)));
  zddx[7] = zdd;
  F16(zddx, 2 * 2) = 1;
  v101 = (intptr_t)g_eng.op_new(8u);
  v102 = zddx;
  v103 = 0;
  for ( *zddx = v101; (uint16_t)v103 < F16(zddx, 2 * 2); v102 = zddx )
  {
    v104 = 8 * (uint16_t)v103++;
    F32((*v102 + v104), 0) = 0;
    F32(*zddx + v104, 4) = 0;
  }
  zddx[2] = 0;
  zddx[6] = 0;
  zddx[15] = settings;
  F16(zddx, 2 * 32) = 1637;
  zddx[8] = 96;
  zddx[9] = 96;
  zddx[10] = 0;
  zddx[11] = 0;
  zddx[12] = 2;
  F16(zddx, 2 * 33) = group;
  zddx[14] = 0;
  F32(SYS32(6844), 4) = 1;
  zddy = SYS32(3168);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3168)));
  zddy[7] = zdd;
  F16(zddy, 2 * 2) = 1;
  v105 = (intptr_t)g_eng.op_new(8u);
  v106 = zddy;
  v107 = 0;
  for ( *zddy = v105; (uint16_t)v107 < F16(zddy, 2 * 2); v106 = zddy )
  {
    v108 = 8 * (uint16_t)v107++;
    F32((*v106 + v108), 0) = 0;
    F32(*zddy + v108, 4) = 0;
  }
  zddy[2] = 0;
  zddy[6] = 0;
  zddy[15] = settings;
  F16(zddy, 2 * 32) = 1970;
  zddy[8] = 32;
  zddy[9] = 32;
  zddy[10] = 0x1FFFFFF;
  zddy[11] = 1;
  zddy[12] = 0;
  F16(zddy, 2 * 33) = group;
  zddy[14] = 0;
  zddz = SYS32(3172);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3172)));
  zddz[7] = zdd;
  F16(zddz, 2 * 2) = 1;
  v109 = (intptr_t)g_eng.op_new(8u);
  v110 = zddz;
  v111 = 0;
  for ( *zddz = v109; (uint16_t)v111 < F16(zddz, 2 * 2); v110 = zddz )
  {
    v112 = 8 * (uint16_t)v111++;
    F32((v112 + *v110), 0) = 0;
    F32(v112 + *zddz, 4) = 0;
  }
  zddz[2] = 0;
  zddz[6] = 0;
  zddz[15] = settings;
  F16(zddz, 2 * 32) = 2007;
  zddz[8] = 32;
  zddz[9] = 32;
  zddz[10] = 0x1FFFFFF;
  zddz[11] = 1;
  zddz[12] = 0;
  F16(zddz, 2 * 33) = group;
  zddz[14] = 0;
  zddba = SYS32(3176);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3176)));
  zddba[7] = zdd;
  F16(zddba, 2 * 2) = 1;
  v113 = (intptr_t)g_eng.op_new(8u);
  v114 = zddba;
  v115 = 0;
  for ( *zddba = v113; (uint16_t)v115 < F16(zddba, 2 * 2); v114 = zddba )
  {
    v116 = 8 * (uint16_t)v115++;
    F32((*v114 + v116), 0) = 0;
    F32(*zddba + v116, 4) = 0;
  }
  zddba[2] = 0;
  zddba[6] = 0;
  zddba[15] = settings;
  F16(zddba, 2 * 32) = 2170;
  zddba[8] = 32;
  zddba[9] = 32;
  zddba[10] = 0x1FFFFFF;
  zddba[11] = 1;
  zddba[12] = 0;
  F16(zddba, 2 * 33) = group;
  zddba[14] = 0;
  zddbb = SYS32(3212);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3212)));
  zddbb[7] = zdd;
  F16(zddbb, 2 * 2) = 1;
  v117 = (intptr_t)g_eng.op_new(8u);
  v118 = zddbb;
  v119 = 0;
  for ( *zddbb = v117; (uint16_t)v119 < F16(zddbb, 2 * 2); v118 = zddbb )
  {
    v120 = 8 * (uint16_t)v119++;
    F32((*v118 + v120), 0) = 0;
    F32(*zddbb + v120, 4) = 0;
  }
  zddbb[2] = 0;
  zddbb[6] = 0;
  zddbb[15] = settings;
  F16(zddbb, 2 * 32) = 2251;
  zddbb[8] = 96;
  zddbb[9] = 96;
  zddbb[10] = 0;
  zddbb[11] = 0;
  zddbb[12] = 2;
  F16(zddbb, 2 * 33) = group;
  zddbb[14] = 0;
  F32(SYS32(6848), 4) = 1;
  zddbc = SYS32(3216);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3216)));
  zddbc[7] = zdd;
  F16(zddbc, 2 * 2) = 1;
  v121 = (intptr_t)g_eng.op_new(8u);
  v122 = zddbc;
  v123 = 0;
  for ( *zddbc = v121; (uint16_t)v123 < F16(zddbc, 2 * 2); v122 = zddbc )
  {
    v124 = 8 * (uint16_t)v123++;
    F32((v124 + *v122), 0) = 0;
    F32(v124 + *zddbc, 4) = 0;
  }
  zddbc[2] = 0;
  zddbc[6] = 0;
  zddbc[15] = settings;
  F16(zddbc, 2 * 32) = 1045;
  zddbc[8] = 64;
  zddbc[9] = 64;
  zddbc[10] = 0x1FFFFFF;
  zddbc[11] = 0;
  zddbc[12] = 0;
  F16(zddbc, 2 * 33) = group;
  zddbc[14] = 0;
  F32(SYS32(6852), 4) = 1;
  zddbd = SYS32(3220);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3220)));
  zddbd[7] = zdd;
  F16(zddbd, 2 * 2) = 1;
  v125 = (intptr_t)g_eng.op_new(8u);
  v126 = zddbd;
  v127 = 0;
  for ( *zddbd = v125; (uint16_t)v127 < F16(zddbd, 2 * 2); v126 = zddbd )
  {
    v128 = 8 * (uint16_t)v127++;
    F32((*v126 + v128), 0) = 0;
    F32(*zddbd + v128, 4) = 0;
  }
  zddbd[2] = 0;
  zddbd[6] = 0;
  zddbd[15] = settings;
  F16(zddbd, 2 * 32) = 1047;
  zddbd[8] = 64;
  zddbd[9] = 64;
  zddbd[10] = 0;
  zddbd[11] = 0;
  zddbd[12] = 2;
  F16(zddbd, 2 * 33) = group;
  zddbd[14] = 0;
  F32(SYS32(6856), 4) = 1;
  zddbe = SYS32(3224);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3224)));
  zddbe[7] = zdd;
  F16(zddbe, 2 * 2) = 1;
  v129 = (intptr_t)g_eng.op_new(8u);
  v130 = zddbe;
  v131 = 0;
  for ( *zddbe = v129; (uint16_t)v131 < F16(zddbe, 2 * 2); v130 = zddbe )
  {
    v132 = 8 * (uint16_t)v131++;
    F32((v132 + *v130), 0) = 0;
    F32(v132 + *zddbe, 4) = 0;
  }
  zddbe[2] = 0;
  zddbe[6] = 0;
  zddbe[15] = settings;
  F16(zddbe, 2 * 32) = 1048;
  zddbe[8] = 64;
  zddbe[9] = 128;
  zddbe[10] = 0;
  zddbe[11] = 0;
  zddbe[12] = 2;
  F16(zddbe, 2 * 33) = group;
  zddbe[14] = 0;
  F32(SYS32(6860), 4) = 1;
  zddbf = SYS32(3228);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3228)));
  zddbf[7] = zdd;
  F16(zddbf, 2 * 2) = 1;
  v133 = (intptr_t)g_eng.op_new(8u);
  v134 = zddbf;
  v135 = 0;
  for ( *zddbf = v133; (uint16_t)v135 < F16(zddbf, 2 * 2); v134 = zddbf )
  {
    v136 = 8 * (uint16_t)v135++;
    F32((v136 + *v134), 0) = 0;
    F32(v136 + *zddbf, 4) = 0;
  }
  zddbf[2] = 0;
  zddbf[6] = 0;
  zddbf[15] = settings;
  F16(zddbf, 2 * 32) = 1044;
  zddbf[8] = 128;
  zddbf[9] = 128;
  zddbf[10] = 0;
  zddbf[11] = 0;
  zddbf[12] = 2;
  F16(zddbf, 2 * 33) = group;
  zddbf[14] = 0;
  F32(SYS32(6864), 4) = 1;
  zddbg = SYS32(3232);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3232)));
  zddbg[7] = zdd;
  F16(zddbg, 2 * 2) = 1;
  v137 = (intptr_t)g_eng.op_new(8u);
  v138 = zddbg;
  v139 = 0;
  for ( *zddbg = v137; (uint16_t)v139 < F16(zddbg, 2 * 2); v138 = zddbg )
  {
    v140 = 8 * (uint16_t)v139++;
    F32((*v138 + v140), 0) = 0;
    F32(*zddbg + v140, 4) = 0;
  }
  zddbg[2] = 0;
  zddbg[6] = 0;
  zddbg[15] = settings;
  F16(zddbg, 2 * 32) = 1049;
  zddbg[8] = 128;
  zddbg[9] = 160;
  zddbg[10] = 0x1FFFFFF;
  zddbg[11] = 0;
  zddbg[12] = 0;
  F16(zddbg, 2 * 33) = group;
  zddbg[14] = 0;
  F32(SYS32(6868), 4) = 1;
  zddbh = SYS32(3236);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3236)));
  zddbh[7] = zdd;
  F16(zddbh, 2 * 2) = 1;
  v141 = (intptr_t)g_eng.op_new(8u);
  v142 = zddbh;
  v143 = 0;
  for ( *zddbh = v141; (uint16_t)v143 < F16(zddbh, 2 * 2); v142 = zddbh )
  {
    v144 = 8 * (uint16_t)v143++;
    F32((v144 + *v142), 0) = 0;
    F32(v144 + *zddbh, 4) = 0;
  }
  zddbh[2] = 0;
  zddbh[6] = 0;
  zddbh[15] = settings;
  F16(zddbh, 2 * 32) = 1046;
  zddbh[8] = 192;
  zddbh[9] = 160;
  zddbh[10] = 0x1FFFFFF;
  zddbh[11] = 1;
  zddbh[12] = 0;
  F16(zddbh, 2 * 33) = group;
  zddbh[14] = 0;
  F32(SYS32(6872), 4) = 1;
  zddbi = SYS32(3240);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3240)));
  zddbi[7] = zdd;
  F16(zddbi, 2 * 2) = 1;
  v145 = (intptr_t)g_eng.op_new(8u);
  v146 = zddbi;
  v147 = 0;
  for ( *zddbi = v145; (uint16_t)v147 < F16(zddbi, 2 * 2); v146 = zddbi )
  {
    v148 = 8 * (uint16_t)v147++;
    F32((v148 + *v146), 0) = 0;
    F32(v148 + *zddbi, 4) = 0;
  }
  zddbi[2] = 0;
  zddbi[6] = 0;
  zddbi[15] = settings;
  F16(zddbi, 2 * 32) = 1757;
  zddbi[8] = 32;
  zddbi[9] = 32;
  zddbi[10] = 0x1FFFFFF;
  zddbi[11] = 1;
  zddbi[12] = 0;
  F16(zddbi, 2 * 33) = group;
  zddbi[14] = 0;
  zddbj = SYS32(3244);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3244)));
  zddbj[7] = zdd;
  F16(zddbj, 2 * 2) = 1;
  v149 = (intptr_t)g_eng.op_new(8u);
  v150 = zddbj;
  v151 = 0;
  for ( *zddbj = v149; (uint16_t)v151 < F16(zddbj, 2 * 2); v150 = zddbj )
  {
    v152 = 8 * (uint16_t)v151++;
    F32((*v150 + v152), 0) = 0;
    F32(*zddbj + v152, 4) = 0;
  }
  zddbj[2] = 0;
  zddbj[6] = 0;
  zddbj[15] = settings;
  F16(zddbj, 2 * 32) = 1758;
  zddbj[8] = 32;
  zddbj[9] = 32;
  zddbj[10] = 0x1FFFFFF;
  zddbj[11] = 1;
  zddbj[12] = 0;
  F16(zddbj, 2 * 33) = group;
  zddbj[14] = 0;
  zddbk = SYS32(3248);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3248)));
  zddbk[7] = zdd;
  F16(zddbk, 2 * 2) = 1;
  v153 = (intptr_t)g_eng.op_new(8u);
  v154 = zddbk;
  v155 = 0;
  for ( *zddbk = v153; (uint16_t)v155 < F16(zddbk, 2 * 2); v154 = zddbk )
  {
    v156 = 8 * (uint16_t)v155++;
    F32((v156 + *v154), 0) = 0;
    F32(v156 + *zddbk, 4) = 0;
  }
  zddbk[2] = 0;
  zddbk[6] = 0;
  zddbk[15] = settings;
  F16(zddbk, 2 * 32) = 1760;
  zddbk[8] = 32;
  zddbk[9] = 32;
  zddbk[10] = 0x1FFFFFF;
  zddbk[11] = 1;
  zddbk[12] = 0;
  F16(zddbk, 2 * 33) = group;
  zddbk[14] = 0;
  zddbl = SYS32(3252);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3252)));
  zddbl[7] = zdd;
  F16(zddbl, 2 * 2) = 1;
  v157 = (intptr_t)g_eng.op_new(8u);
  v158 = zddbl;
  v159 = 0;
  for ( *zddbl = v157; (uint16_t)v159 < F16(zddbl, 2 * 2); v158 = zddbl )
  {
    v160 = 8 * (uint16_t)v159++;
    F32((v160 + *v158), 0) = 0;
    F32(v160 + *zddbl, 4) = 0;
  }
  zddbl[2] = 0;
  zddbl[6] = 0;
  zddbl[15] = settings;
  F16(zddbl, 2 * 32) = 1962;
  zddbl[8] = 32;
  zddbl[9] = 32;
  zddbl[10] = 0x1FFFFFF;
  zddbl[11] = 1;
  zddbl[12] = 0;
  F16(zddbl, 2 * 33) = group;
  zddbl[14] = 0;
  zddbm = SYS32(3256);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3256)));
  zddbm[7] = zdd;
  F16(zddbm, 2 * 2) = 1;
  v161 = (intptr_t)g_eng.op_new(8u);
  v162 = zddbm;
  v163 = 0;
  for ( *zddbm = v161; (uint16_t)v163 < F16(zddbm, 2 * 2); v162 = zddbm )
  {
    v164 = 8 * (uint16_t)v163++;
    F32((v164 + *v162), 0) = 0;
    F32(v164 + *zddbm, 4) = 0;
  }
  zddbm[2] = 0;
  zddbm[6] = 0;
  zddbm[15] = settings;
  F16(zddbm, 2 * 32) = 1050;
  zddbm[8] = 64;
  zddbm[9] = 64;
  zddbm[10] = 0;
  zddbm[11] = 0;
  zddbm[12] = 2;
  F16(zddbm, 2 * 33) = group;
  zddbm[14] = 0;
  F32(SYS32(6892), 4) = 1;
  zddbn = SYS32(3260);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3260)));
  zddbn[7] = zdd;
  F16(zddbn, 2 * 2) = 1;
  v165 = (intptr_t)g_eng.op_new(8u);
  v166 = zddbn;
  v167 = 0;
  for ( *zddbn = v165; (uint16_t)v167 < F16(zddbn, 2 * 2); v166 = zddbn )
  {
    v168 = 8 * (uint16_t)v167++;
    F32((v168 + *v166), 0) = 0;
    F32(v168 + *zddbn, 4) = 0;
  }
  zddbn[2] = 0;
  zddbn[6] = 0;
  zddbn[15] = settings;
  F16(zddbn, 2 * 32) = 1759;
  zddbn[8] = 64;
  zddbn[9] = 64;
  zddbn[10] = 0;
  zddbn[11] = 0;
  zddbn[12] = 2;
  F16(zddbn, 2 * 33) = group;
  zddbn[14] = 0;
  F32(SYS32(6896), 4) = 1;
  zddbo = SYS32(3264);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3264)));
  zddbo[7] = zdd;
  F16(zddbo, 2 * 2) = 1;
  v169 = (intptr_t)g_eng.op_new(8u);
  v170 = zddbo;
  v171 = 0;
  for ( *zddbo = v169; (uint16_t)v171 < F16(zddbo, 2 * 2); v170 = zddbo )
  {
    v172 = 8 * (uint16_t)v171++;
    F32((v172 + *v170), 0) = 0;
    F32(v172 + *zddbo, 4) = 0;
  }
  zddbo[2] = 0;
  zddbo[6] = 0;
  zddbo[15] = settings;
  F16(zddbo, 2 * 32) = 1763;
  zddbo[8] = 64;
  zddbo[9] = 64;
  zddbo[10] = 0;
  zddbo[11] = 0;
  zddbo[12] = 2;
  F16(zddbo, 2 * 33) = group;
  zddbo[14] = 0;
  F32(SYS32(6900), 4) = 1;
  zddbp = SYS32(3268);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3268)));
  zddbp[7] = zdd;
  F16(zddbp, 2 * 2) = 1;
  v173 = (intptr_t)g_eng.op_new(8u);
  v174 = zddbp;
  v175 = 0;
  for ( *zddbp = v173; (uint16_t)v175 < F16(zddbp, 2 * 2); v174 = zddbp )
  {
    v176 = 8 * (uint16_t)v175++;
    F32((v176 + *v174), 0) = 0;
    F32(v176 + *zddbp, 4) = 0;
  }
  zddbp[2] = 0;
  zddbp[6] = 0;
  zddbp[15] = settings;
  F16(zddbp, 2 * 32) = 1963;
  zddbp[8] = 64;
  zddbp[9] = 64;
  zddbp[10] = 0;
  zddbp[11] = 0;
  zddbp[12] = 2;
  F16(zddbp, 2 * 33) = group;
  zddbp[14] = 0;
  F32(SYS32(6904), 4) = 1;
  zddbq = SYS32(3272);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3272)));
  zddbq[7] = zdd;
  F16(zddbq, 2 * 2) = 1;
  v177 = (intptr_t)g_eng.op_new(8u);
  v178 = zddbq;
  v179 = 0;
  for ( *zddbq = v177; (uint16_t)v179 < F16(zddbq, 2 * 2); v178 = zddbq )
  {
    v180 = 8 * (uint16_t)v179++;
    F32((v180 + *v178), 0) = 0;
    F32(v180 + *zddbq, 4) = 0;
  }
  zddbq[2] = 0;
  zddbq[6] = 0;
  zddbq[15] = settings;
  F16(zddbq, 2 * 32) = 1407;
  zddbq[8] = 128;
  zddbq[9] = 128;
  zddbq[10] = 0;
  zddbq[11] = 0;
  zddbq[12] = 2;
  F16(zddbq, 2 * 33) = group;
  zddbq[14] = 0;
  F32(SYS32(6908), 4) = 1;
  zddbr = SYS32(3276);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3276)));
  zddbr[7] = zdd;
  F16(zddbr, 2 * 2) = 1;
  v181 = (intptr_t)g_eng.op_new(8u);
  v182 = zddbr;
  v183 = 0;
  for ( *zddbr = v181; (uint16_t)v183 < F16(zddbr, 2 * 2); v182 = zddbr )
  {
    v184 = 8 * (uint16_t)v183++;
    F32((v184 + *v182), 0) = 0;
    F32(v184 + *zddbr, 4) = 0;
  }
  zddbr[2] = 0;
  zddbr[6] = 0;
  zddbr[15] = settings;
  F16(zddbr, 2 * 32) = 1761;
  zddbr[8] = 128;
  zddbr[9] = 128;
  zddbr[10] = 0;
  zddbr[11] = 0;
  zddbr[12] = 2;
  F16(zddbr, 2 * 33) = group;
  zddbr[14] = 0;
  F32(SYS32(6912), 4) = 1;
  zddbs = SYS32(3280);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3280)));
  zddbs[7] = zdd;
  F16(zddbs, 2 * 2) = 1;
  v185 = (intptr_t)g_eng.op_new(8u);
  v186 = zddbs;
  v187 = 0;
  for ( *zddbs = v185; (uint16_t)v187 < F16(zddbs, 2 * 2); v186 = zddbs )
  {
    v188 = 8 * (uint16_t)v187++;
    F32((v188 + *v186), 0) = 0;
    F32(v188 + *zddbs, 4) = 0;
  }
  zddbs[2] = 0;
  zddbs[6] = 0;
  zddbs[15] = settings;
  F16(zddbs, 2 * 32) = 1762;
  zddbs[8] = 128;
  zddbs[9] = 128;
  zddbs[10] = 0;
  zddbs[11] = 0;
  zddbs[12] = 2;
  F16(zddbs, 2 * 33) = group;
  zddbs[14] = 0;
  F32(SYS32(6916), 4) = 1;
  zddbt = SYS32(3284);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(3284)));
  zddbt[7] = zdd;
  F16(zddbt, 2 * 2) = 1;
  v189 = (intptr_t)g_eng.op_new(8u);
  v190 = zddbt;
  v191 = 0;
  for ( *zddbt = v189; (uint16_t)v191 < F16(zddbt, 2 * 2); v190 = zddbt )
  {
    v192 = 8 * (uint16_t)v191++;
    F32((*v190 + v192), 0) = 0;
    F32(*zddbt + v192, 4) = 0;
  }
  zddbt[2] = 0;
  zddbt[6] = 0;
  zddbt[15] = settings;
  F16(zddbt, 2 * 32) = 1961;
  zddbt[8] = 128;
  zddbt[9] = 128;
  zddbt[10] = 0;
  zddbt[11] = 0;
  zddbt[12] = 2;
  F16(zddbt, 2 * 33) = group;
  zddbt[14] = 0;
  F32(SYS32(6920), 4) = 1;
  zddbu = SYS32(4136);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4136)));
  zddbu[7] = zdd;
  F16(zddbu, 2 * 2) = 1;
  v193 = (intptr_t)g_eng.op_new(8u);
  v194 = zddbu;
  v195 = 0;
  for ( *zddbu = v193; (uint16_t)v195 < F16(zddbu, 2 * 2); v194 = zddbu )
  {
    v196 = 8 * (uint16_t)v195++;
    F32((v196 + *v194), 0) = 0;
    F32(v196 + *zddbu, 4) = 0;
  }
  zddbu[2] = 0;
  zddbu[6] = 0;
  zddbu[15] = settings;
  F16(zddbu, 2 * 32) = 1153;
  zddbu[8] = 320;
  zddbu[9] = 320;
  zddbu[10] = 0;
  zddbu[11] = 0;
  zddbu[12] = 0;
  F16(zddbu, 2 * 33) = group;
  zddbu[14] = 0;
  F32(SYS32(7772), 4) = 1;
  zddbv = SYS32(4140);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4140)));
  zddbv[7] = zdd;
  F16(zddbv, 2 * 2) = 1;
  v197 = (intptr_t)g_eng.op_new(8u);
  v198 = zddbv;
  v199 = 0;
  for ( *zddbv = v197; (uint16_t)v199 < F16(zddbv, 2 * 2); v198 = zddbv )
  {
    v200 = 8 * (uint16_t)v199++;
    F32((*v198 + v200), 0) = 0;
    F32(*zddbv + v200, 4) = 0;
  }
  zddbv[2] = 0;
  zddbv[6] = 0;
  zddbv[15] = settings;
  F16(zddbv, 2 * 32) = 1154;
  zddbv[8] = 352;
  zddbv[9] = 288;
  zddbv[10] = 0;
  zddbv[11] = 0;
  zddbv[12] = 0;
  F16(zddbv, 2 * 33) = group;
  zddbv[14] = 0;
  F32(SYS32(7776), 4) = 1;
  zddbw = SYS32(4144);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4144)));
  zddbw[7] = zdd;
  F16(zddbw, 2 * 2) = 1;
  v201 = (intptr_t)g_eng.op_new(8u);
  v202 = zddbw;
  v203 = 0;
  for ( *zddbw = v201; (uint16_t)v203 < F16(zddbw, 2 * 2); v202 = zddbw )
  {
    v204 = 8 * (uint16_t)v203++;
    F32((*v202 + v204), 0) = 0;
    F32(*zddbw + v204, 4) = 0;
  }
  zddbw[2] = 0;
  zddbw[6] = 0;
  zddbw[15] = settings;
  F16(zddbw, 2 * 32) = 1155;
  zddbw[8] = 160;
  zddbw[9] = 160;
  zddbw[10] = 0;
  zddbw[11] = 0;
  zddbw[12] = 2;
  F16(zddbw, 2 * 33) = group;
  zddbw[14] = 0;
  F32(SYS32(7780), 4) = 1;
  zddbx = SYS32(4148);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4148)));
  zddbx[7] = zdd;
  F16(zddbx, 2 * 2) = 1;
  v205 = (intptr_t)g_eng.op_new(8u);
  v206 = zddbx;
  v207 = 0;
  for ( *zddbx = v205; (uint16_t)v207 < F16(zddbx, 2 * 2); v206 = zddbx )
  {
    v208 = 8 * (uint16_t)v207++;
    F32((v208 + *v206), 0) = 0;
    F32(v208 + *zddbx, 4) = 0;
  }
  zddbx[2] = 0;
  zddbx[6] = 0;
  zddbx[15] = settings;
  F16(zddbx, 2 * 32) = 1957;
  zddbx[8] = 320;
  zddbx[9] = 320;
  zddbx[10] = 0;
  zddbx[11] = 0;
  zddbx[12] = 0;
  F16(zddbx, 2 * 33) = group;
  zddbx[14] = 0;
  F32(SYS32(7784), 4) = 1;
  zddby = SYS32(4152);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4152)));
  zddby[7] = zdd;
  F16(zddby, 2 * 2) = 1;
  v209 = (intptr_t)g_eng.op_new(8u);
  v210 = zddby;
  v211 = 0;
  for ( *zddby = v209; (uint16_t)v211 < F16(zddby, 2 * 2); v210 = zddby )
  {
    v212 = 8 * (uint16_t)v211++;
    F32((*v210 + v212), 0) = 0;
    F32(*zddby + v212, 4) = 0;
  }
  zddby[2] = 0;
  zddby[6] = 0;
  zddby[15] = settings;
  F16(zddby, 2 * 32) = 1063;
  zddby[8] = 80;
  zddby[9] = 80;
  zddby[10] = 0;
  zddby[11] = 0;
  zddby[12] = 2;
  F16(zddby, 2 * 33) = group;
  zddby[14] = 0;
  F32(SYS32(7788), 4) = 1;
  zddbz = SYS32(4160);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4160)));
  zddbz[7] = zdd;
  F16(zddbz, 2 * 2) = 1;
  v213 = (intptr_t)g_eng.op_new(8u);
  v214 = zddbz;
  v215 = 0;
  for ( *zddbz = v213; (uint16_t)v215 < F16(zddbz, 2 * 2); v214 = zddbz )
  {
    v216 = 8 * (uint16_t)v215++;
    F32((*v214 + v216), 0) = 0;
    F32(*zddbz + v216, 4) = 0;
  }
  zddbz[2] = 0;
  zddbz[6] = 0;
  zddbz[15] = settings;
  F16(zddbz, 2 * 32) = 1149;
  zddbz[8] = 96;
  zddbz[9] = 96;
  zddbz[10] = 0;
  zddbz[11] = 0;
  zddbz[12] = 2;
  F16(zddbz, 2 * 33) = group;
  zddbz[14] = 0;
  F32(SYS32(7796), 4) = 1;
  zddca = SYS32(4164);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4164)));
  zddca[7] = zdd;
  F16(zddca, 2 * 2) = 1;
  v217 = (intptr_t)g_eng.op_new(8u);
  v218 = zddca;
  v219 = 0;
  for ( *zddca = v217; (uint16_t)v219 < F16(zddca, 2 * 2); v218 = zddca )
  {
    v220 = 8 * (uint16_t)v219++;
    F32((v220 + *v218), 0) = 0;
    F32(v220 + *zddca, 4) = 0;
  }
  zddca[2] = 0;
  zddca[6] = 0;
  zddca[15] = settings;
  F16(zddca, 2 * 32) = 1148;
  zddca[8] = 80;
  zddca[9] = 48;
  zddca[10] = 0;
  zddca[11] = 0;
  zddca[12] = 2;
  F16(zddca, 2 * 33) = group;
  zddca[14] = 0;
  F32(SYS32(7800), 4) = 1;
  zddcb = SYS32(4156);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4156)));
  zddcb[7] = zdd;
  F16(zddcb, 2 * 2) = 1;
  v221 = (intptr_t)g_eng.op_new(8u);
  v222 = zddcb;
  v223 = 0;
  for ( *zddcb = v221; (uint16_t)v223 < F16(zddcb, 2 * 2); v222 = zddcb )
  {
    v224 = 8 * (uint16_t)v223++;
    F32((*v222 + v224), 0) = 0;
    F32(*zddcb + v224, 4) = 0;
  }
  zddcb[2] = 0;
  zddcb[6] = 0;
  zddcb[15] = settings;
  F16(zddcb, 2 * 32) = 1062;
  zddcb[8] = 48;
  zddcb[9] = 48;
  zddcb[10] = 0;
  zddcb[11] = 0;
  zddcb[12] = 2;
  F16(zddcb, 2 * 33) = group;
  zddcb[14] = 0;
  F32(SYS32(7792), 4) = 1;
  zddcc = SYS32(4212);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4212)));
  zddcc[7] = zdd;
  F16(zddcc, 2 * 2) = 1;
  v225 = (intptr_t)g_eng.op_new(8u);
  v226 = zddcc;
  v227 = 0;
  for ( *zddcc = v225; (uint16_t)v227 < F16(zddcc, 2 * 2); v226 = zddcc )
  {
    v228 = 8 * (uint16_t)v227++;
    F32((v228 + *v226), 0) = 0;
    F32(v228 + *zddcc, 4) = 0;
  }
  zddcc[2] = 0;
  zddcc[6] = 0;
  zddcc[15] = settings;
  F16(zddcc, 2 * 32) = 1064;
  zddcc[8] = 192;
  zddcc[9] = 192;
  zddcc[10] = 0;
  zddcc[11] = 0;
  zddcc[12] = 2;
  F16(zddcc, 2 * 33) = group;
  zddcc[14] = 0;
  F32(SYS32(7848), 4) = 1;
  zddcd = SYS32(4216);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4216)));
  zddcd[7] = zdd;
  F16(zddcd, 2 * 2) = 1;
  v229 = (intptr_t)g_eng.op_new(8u);
  v230 = zddcd;
  v231 = 0;
  for ( *zddcd = v229; (uint16_t)v231 < F16(zddcd, 2 * 2); v230 = zddcd )
  {
    v232 = 8 * (uint16_t)v231++;
    F32((v232 + *v230), 0) = 0;
    F32(v232 + *zddcd, 4) = 0;
  }
  zddcd[2] = 0;
  zddcd[6] = 0;
  zddcd[15] = settings;
  F16(zddcd, 2 * 32) = 1065;
  zddcd[8] = 320;
  zddcd[9] = 256;
  zddcd[10] = 0;
  zddcd[11] = 0;
  zddcd[12] = 2;
  F16(zddcd, 2 * 33) = group;
  zddcd[14] = 0;
  F32(SYS32(7852), 4) = 1;
  zddce = SYS32(4220);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4220)));
  zddce[7] = zdd;
  F16(zddce, 2 * 2) = 1;
  v233 = (intptr_t)g_eng.op_new(8u);
  v234 = zddce;
  v235 = 0;
  for ( *zddce = v233; (uint16_t)v235 < F16(zddce, 2 * 2); v234 = zddce )
  {
    v236 = 8 * (uint16_t)v235++;
    F32((*v234 + v236), 0) = 0;
    F32(*zddce + v236, 4) = 0;
  }
  zddce[2] = 0;
  zddce[6] = 0;
  zddce[15] = settings;
  F16(zddce, 2 * 32) = 1066;
  zddce[8] = 128;
  zddce[9] = 288;
  zddce[10] = 0;
  zddce[11] = 0;
  zddce[12] = 2;
  F16(zddce, 2 * 33) = group;
  zddce[14] = 0;
  F32(SYS32(7856), 4) = 1;
  zddcf = SYS32(4224);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4224)));
  zddcf[7] = zdd;
  F16(zddcf, 2 * 2) = 1;
  v237 = (intptr_t)g_eng.op_new(8u);
  v238 = zddcf;
  v239 = 0;
  for ( *zddcf = v237; (uint16_t)v239 < F16(zddcf, 2 * 2); v238 = zddcf )
  {
    v240 = 8 * (uint16_t)v239++;
    F32((v240 + *v238), 0) = 0;
    F32(v240 + *zddcf, 4) = 0;
  }
  zddcf[2] = 0;
  zddcf[6] = 0;
  zddcf[15] = settings;
  F16(zddcf, 2 * 32) = 1004;
  zddcf[8] = 128;
  zddcf[9] = 192;
  zddcf[10] = 0;
  zddcf[11] = 0;
  zddcf[12] = 2;
  F16(zddcf, 2 * 33) = group;
  zddcf[14] = 0;
  F32(SYS32(7860), 4) = 1;
  zddcg = SYS32(4248);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4248)));
  zddcg[7] = zdd;
  F16(zddcg, 2 * 2) = 1;
  v241 = (intptr_t)g_eng.op_new(8u);
  v242 = zddcg;
  v243 = 0;
  for ( *zddcg = v241; (uint16_t)v243 < F16(zddcg, 2 * 2); v242 = zddcg )
  {
    v244 = 8 * (uint16_t)v243++;
    F32((v244 + *v242), 0) = 0;
    F32(v244 + *zddcg, 4) = 0;
  }
  zddcg[2] = 0;
  zddcg[6] = 0;
  zddcg[15] = settings;
  F16(zddcg, 2 * 32) = 1074;
  zddcg[8] = 64;
  zddcg[9] = 64;
  zddcg[10] = 0;
  zddcg[11] = 0;
  zddcg[12] = 2;
  F16(zddcg, 2 * 33) = group;
  zddcg[14] = 0;
  F32(SYS32(7884), 4) = 2;
  zddch = SYS32(4188);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4188)));
  zddch[7] = zdd;
  F16(zddch, 2 * 2) = 1;
  v245 = (intptr_t)g_eng.op_new(8u);
  v246 = zddch;
  v247 = 0;
  for ( *zddch = v245; (uint16_t)v247 < F16(zddch, 2 * 2); v246 = zddch )
  {
    v248 = 8 * (uint16_t)v247++;
    F32((*v246 + v248), 0) = 0;
    F32(*zddch + v248, 4) = 0;
  }
  zddch[2] = 0;
  zddch[6] = 0;
  zddch[15] = settings;
  F16(zddch, 2 * 32) = 1027;
  zddch[8] = 64;
  zddch[9] = 64;
  zddch[10] = 0;
  zddch[11] = 0;
  zddch[12] = 2;
  F16(zddch, 2 * 33) = group;
  zddch[14] = 0;
  F32(SYS32(7824), 4) = 2;
  zddci = SYS32(4192);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4192)));
  zddci[7] = zdd;
  F16(zddci, 2 * 2) = 1;
  v249 = (intptr_t)g_eng.op_new(8u);
  v250 = zddci;
  v251 = 0;
  for ( *zddci = v249; (uint16_t)v251 < F16(zddci, 2 * 2); v250 = zddci )
  {
    v252 = 8 * (uint16_t)v251++;
    F32((v252 + *v250), 0) = 0;
    F32(v252 + *zddci, 4) = 0;
  }
  zddci[2] = 0;
  zddci[6] = 0;
  zddci[15] = settings;
  F16(zddci, 2 * 32) = 1028;
  zddci[8] = 64;
  zddci[9] = 64;
  zddci[10] = 0;
  zddci[11] = 0;
  zddci[12] = 2;
  F16(zddci, 2 * 33) = group;
  zddci[14] = 0;
  F32(SYS32(7828), 4) = 2;
  zddcj = SYS32(4196);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4196)));
  zddcj[7] = zdd;
  F16(zddcj, 2 * 2) = 1;
  v253 = (intptr_t)g_eng.op_new(8u);
  v254 = zddcj;
  v255 = 0;
  for ( *zddcj = v253; (uint16_t)v255 < F16(zddcj, 2 * 2); v254 = zddcj )
  {
    v256 = 8 * (uint16_t)v255++;
    F32((v256 + *v254), 0) = 0;
    F32(v256 + *zddcj, 4) = 0;
  }
  zddcj[2] = 0;
  zddcj[6] = 0;
  zddcj[15] = settings;
  F16(zddcj, 2 * 32) = 1029;
  zddcj[8] = 80;
  zddcj[9] = 80;
  zddcj[10] = 0;
  zddcj[11] = 0;
  zddcj[12] = 2;
  F16(zddcj, 2 * 33) = group;
  zddcj[14] = 0;
  F32(SYS32(7832), 4) = 2;
  zddck = SYS32(4200);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4200)));
  zddck[7] = zdd;
  F16(zddck, 2 * 2) = 1;
  v257 = (intptr_t)g_eng.op_new(8u);
  v258 = zddck;
  v259 = 0;
  for ( *zddck = v257; (uint16_t)v259 < F16(zddck, 2 * 2); v258 = zddck )
  {
    v260 = 8 * (uint16_t)v259++;
    F32((v260 + *v258), 0) = 0;
    F32(v260 + *zddck, 4) = 0;
  }
  zddck[2] = 0;
  zddck[6] = 0;
  zddck[15] = settings;
  F16(zddck, 2 * 32) = 1023;
  zddck[8] = 128;
  zddck[9] = 128;
  zddck[10] = 0;
  zddck[11] = 0;
  zddck[12] = 2;
  F16(zddck, 2 * 33) = group;
  zddck[14] = 0;
  F32(SYS32(7836), 4) = 2;
  zddcl = SYS32(4204);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4204)));
  zddcl[7] = zdd;
  F16(zddcl, 2 * 2) = 1;
  v261 = (intptr_t)g_eng.op_new(8u);
  v262 = zddcl;
  v263 = 0;
  for ( *zddcl = v261; (uint16_t)v263 < F16(zddcl, 2 * 2); v262 = zddcl )
  {
    v264 = 8 * (uint16_t)v263++;
    F32((v264 + *v262), 0) = 0;
    F32(v264 + *zddcl, 4) = 0;
  }
  zddcl[2] = 0;
  zddcl[6] = 0;
  zddcl[15] = settings;
  F16(zddcl, 2 * 32) = 1024;
  zddcl[8] = 128;
  zddcl[9] = 128;
  zddcl[10] = 0;
  zddcl[11] = 0;
  zddcl[12] = 2;
  F16(zddcl, 2 * 33) = group;
  zddcl[14] = 0;
  F32(SYS32(7840), 4) = 2;
  zddcm = SYS32(4208);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4208)));
  zddcm[7] = zdd;
  F16(zddcm, 2 * 2) = 1;
  v265 = (intptr_t)g_eng.op_new(8u);
  v266 = zddcm;
  v267 = 0;
  for ( *zddcm = v265; (uint16_t)v267 < F16(zddcm, 2 * 2); v266 = zddcm )
  {
    v268 = 8 * (uint16_t)v267++;
    F32((v268 + *v266), 0) = 0;
    F32(v268 + *zddcm, 4) = 0;
  }
  zddcm[2] = 0;
  zddcm[6] = 0;
  zddcm[15] = settings;
  F16(zddcm, 2 * 32) = 1025;
  zddcm[8] = 192;
  zddcm[9] = 192;
  zddcm[10] = 0;
  zddcm[11] = 0;
  zddcm[12] = 2;
  F16(zddcm, 2 * 33) = group;
  zddcm[14] = 0;
  F32(SYS32(7844), 4) = 2;
  zddcn = SYS32(4184);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4184)));
  zddcn[7] = zdd;
  F16(zddcn, 2 * 2) = 1;
  v269 = (intptr_t)g_eng.op_new(8u);
  v270 = zddcn;
  v271 = 0;
  for ( *zddcn = v269; (uint16_t)v271 < F16(zddcn, 2 * 2); v270 = zddcn )
  {
    v272 = 8 * (uint16_t)v271++;
    F32((v272 + *v270), 0) = 0;
    F32(v272 + *zddcn, 4) = 0;
  }
  zddcn[2] = 0;
  zddcn[6] = 0;
  zddcn[15] = settings;
  F16(zddcn, 2 * 32) = 1026;
  zddcn[8] = 40;
  zddcn[9] = 40;
  zddcn[10] = 0;
  zddcn[11] = 0;
  zddcn[12] = 2;
  F16(zddcn, 2 * 33) = group;
  zddcn[14] = 0;
  F32(SYS32(7820), 4) = 2;
  zddco = SYS32(4228);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4228)));
  zddco[7] = zdd;
  F16(zddco, 2 * 2) = 1;
  v273 = (intptr_t)g_eng.op_new(8u);
  v274 = zddco;
  v275 = 0;
  for ( *zddco = v273; (uint16_t)v275 < F16(zddco, 2 * 2); v274 = zddco )
  {
    v276 = 8 * (uint16_t)v275++;
    F32((v276 + *v274), 0) = 0;
    F32(v276 + *zddco, 4) = 0;
  }
  zddco[2] = 0;
  zddco[6] = 0;
  zddco[15] = settings;
  F16(zddco, 2 * 32) = 1078;
  zddco[8] = 64;
  zddco[9] = 128;
  zddco[10] = 0;
  zddco[11] = 0;
  zddco[12] = 2;
  F16(zddco, 2 * 33) = group;
  zddco[14] = 0;
  F32(SYS32(7864), 4) = 1;
  zddcp = SYS32(4232);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4232)));
  zddcp[7] = zdd;
  F16(zddcp, 2 * 2) = 1;
  v277 = (intptr_t)g_eng.op_new(8u);
  v278 = zddcp;
  v279 = 0;
  for ( *zddcp = v277; (uint16_t)v279 < F16(zddcp, 2 * 2); v278 = zddcp )
  {
    v280 = 8 * (uint16_t)v279++;
    F32((v280 + *v278), 0) = 0;
    F32(v280 + *zddcp, 4) = 0;
  }
  zddcp[2] = 0;
  zddcp[6] = 0;
  zddcp[15] = settings;
  F16(zddcp, 2 * 32) = 2234;
  zddcp[8] = 64;
  zddcp[9] = 192;
  zddcp[10] = 0;
  zddcp[11] = 0;
  zddcp[12] = 2;
  F16(zddcp, 2 * 33) = group;
  zddcp[14] = 0;
  F32(SYS32(7868), 4) = 1;
  zddcq = SYS32(4236);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4236)));
  zddcq[7] = zdd;
  F16(zddcq, 2 * 2) = 1;
  v281 = (intptr_t)g_eng.op_new(8u);
  v282 = zddcq;
  v283 = 0;
  for ( *zddcq = v281; (uint16_t)v283 < F16(zddcq, 2 * 2); v282 = zddcq )
  {
    v284 = 8 * (uint16_t)v283++;
    F32((v284 + *v282), 0) = 0;
    F32(v284 + *zddcq, 4) = 0;
  }
  zddcq[2] = 0;
  zddcq[6] = 0;
  zddcq[15] = settings;
  F16(zddcq, 2 * 32) = 2235;
  zddcq[8] = 160;
  zddcq[9] = 192;
  zddcq[10] = 0;
  zddcq[11] = 0;
  zddcq[12] = 2;
  F16(zddcq, 2 * 33) = group;
  zddcq[14] = 0;
  F32(SYS32(7872), 4) = 1;
  zddcr = SYS32(4240);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4240)));
  zddcr[7] = zdd;
  F16(zddcr, 2 * 2) = 1;
  v285 = (intptr_t)g_eng.op_new(8u);
  v286 = zddcr;
  v287 = 0;
  for ( *zddcr = v285; (uint16_t)v287 < F16(zddcr, 2 * 2); v286 = zddcr )
  {
    v288 = 8 * (uint16_t)v287++;
    F32((v288 + *v286), 0) = 0;
    F32(v288 + *zddcr, 4) = 0;
  }
  zddcr[2] = 0;
  zddcr[6] = 0;
  zddcr[15] = settings;
  F16(zddcr, 2 * 32) = 2236;
  zddcr[8] = 128;
  zddcr[9] = 128;
  zddcr[10] = 0;
  zddcr[11] = 0;
  zddcr[12] = 2;
  F16(zddcr, 2 * 33) = group;
  zddcr[14] = 0;
  F32(SYS32(7876), 4) = 1;
  zddcs = SYS32(4244);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4244)));
  zddcs[7] = zdd;
  F16(zddcs, 2 * 2) = 1;
  v289 = (intptr_t)g_eng.op_new(8u);
  v290 = zddcs;
  v291 = 0;
  for ( *zddcs = v289; (uint16_t)v291 < F16(zddcs, 2 * 2); v290 = zddcs )
  {
    v292 = 8 * (uint16_t)v291++;
    F32((v292 + *v290), 0) = 0;
    F32(v292 + *zddcs, 4) = 0;
  }
  zddcs[2] = 0;
  zddcs[6] = 0;
  zddcs[15] = settings;
  F16(zddcs, 2 * 32) = 1079;
  zddcs[8] = 64;
  zddcs[9] = 128;
  zddcs[10] = 0;
  zddcs[11] = 0;
  zddcs[12] = 2;
  F16(zddcs, 2 * 33) = group;
  zddcs[14] = 0;
  F32(SYS32(7880), 4) = 1;
  zddct = SYS32(4252);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4252)));
  zddct[7] = zdd;
  F16(zddct, 2 * 2) = 1;
  v293 = (intptr_t)g_eng.op_new(8u);
  v294 = zddct;
  v295 = 0;
  for ( *zddct = v293; (uint16_t)v295 < F16(zddct, 2 * 2); v294 = zddct )
  {
    v296 = 8 * (uint16_t)v295++;
    F32((*v294 + v296), 0) = 0;
    F32(*zddct + v296, 4) = 0;
  }
  zddct[2] = 0;
  zddct[6] = 0;
  zddct[15] = settings;
  F16(zddct, 2 * 32) = 1073;
  zddct[8] = 64;
  zddct[9] = 32;
  zddct[10] = 0;
  zddct[11] = 0;
  zddct[12] = 2;
  F16(zddct, 2 * 33) = group;
  zddct[14] = 0;
  F32(SYS32(7888), 4) = 2;
  zddcu = SYS32(4256);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4256)));
  zddcu[7] = zdd;
  F16(zddcu, 2 * 2) = 1;
  v297 = (intptr_t)g_eng.op_new(8u);
  v298 = zddcu;
  v299 = 0;
  for ( *zddcu = v297; (uint16_t)v299 < F16(zddcu, 2 * 2); v298 = zddcu )
  {
    v300 = 8 * (uint16_t)v299++;
    F32((*v298 + v300), 0) = 0;
    F32(*zddcu + v300, 4) = 0;
  }
  zddcu[2] = 0;
  zddcu[6] = 0;
  zddcu[15] = settings;
  F16(zddcu, 2 * 32) = 1072;
  zddcu[8] = 64;
  zddcu[9] = 160;
  zddcu[10] = 0;
  zddcu[11] = 0;
  zddcu[12] = 2;
  F16(zddcu, 2 * 33) = group;
  zddcu[14] = 0;
  F32(SYS32(7892), 4) = 2;
  zddcv = SYS32(4260);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4260)));
  zddcv[7] = zdd;
  F16(zddcv, 2 * 2) = 1;
  v301 = (intptr_t)g_eng.op_new(8u);
  v302 = zddcv;
  v303 = 0;
  for ( *zddcv = v301; (uint16_t)v303 < F16(zddcv, 2 * 2); v302 = zddcv )
  {
    v304 = 8 * (uint16_t)v303++;
    F32((v304 + *v302), 0) = 0;
    F32(v304 + *zddcv, 4) = 0;
  }
  zddcv[2] = 0;
  zddcv[6] = 0;
  zddcv[15] = settings;
  F16(zddcv, 2 * 32) = 1071;
  zddcv[8] = 128;
  zddcv[9] = 160;
  zddcv[10] = 0;
  zddcv[11] = 0;
  zddcv[12] = 2;
  F16(zddcv, 2 * 33) = group;
  zddcv[14] = 0;
  F32(SYS32(7896), 4) = 2;
  zddcw = SYS32(4320);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4320)));
  zddcw[7] = zdd;
  F16(zddcw, 2 * 2) = 1;
  v305 = (intptr_t)g_eng.op_new(8u);
  v306 = zddcw;
  v307 = 0;
  for ( *zddcw = v305; (uint16_t)v307 < F16(zddcw, 2 * 2); v306 = zddcw )
  {
    v308 = 8 * (uint16_t)v307++;
    F32((*v306 + v308), 0) = 0;
    F32(*zddcw + v308, 4) = 0;
  }
  zddcw[2] = 0;
  zddcw[6] = 0;
  zddcw[15] = settings;
  F16(zddcw, 2 * 32) = 1697;
  zddcw[8] = 80;
  zddcw[9] = 96;
  zddcw[10] = 0;
  zddcw[11] = 0;
  zddcw[12] = 2;
  F16(zddcw, 2 * 33) = group;
  zddcw[14] = 0;
  F32(SYS32(7956), 4) = 1;
  zddcx = SYS32(4324);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4324)));
  zddcx[7] = zdd;
  F16(zddcx, 2 * 2) = 1;
  v309 = (intptr_t)g_eng.op_new(8u);
  v310 = zddcx;
  v311 = 0;
  for ( *zddcx = v309; (uint16_t)v311 < F16(zddcx, 2 * 2); v310 = zddcx )
  {
    v312 = 8 * (uint16_t)v311++;
    F32((*v310 + v312), 0) = 0;
    F32(*zddcx + v312, 4) = 0;
  }
  zddcx[2] = 0;
  zddcx[6] = 0;
  zddcx[15] = settings;
  F16(zddcx, 2 * 32) = 1699;
  zddcx[8] = 32;
  zddcx[9] = 96;
  zddcx[10] = 0;
  zddcx[11] = 0;
  zddcx[12] = 2;
  F16(zddcx, 2 * 33) = group;
  zddcx[14] = 0;
  F32(SYS32(7960), 4) = 1;
  zddcy = SYS32(4328);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4328)));
  zddcy[7] = zdd;
  F16(zddcy, 2 * 2) = 1;
  v313 = (intptr_t)g_eng.op_new(8u);
  v314 = zddcy;
  v315 = 0;
  for ( *zddcy = v313; (uint16_t)v315 < F16(zddcy, 2 * 2); v314 = zddcy )
  {
    v316 = 8 * (uint16_t)v315++;
    F32((v316 + *v314), 0) = 0;
    F32(v316 + *zddcy, 4) = 0;
  }
  zddcy[2] = 0;
  zddcy[6] = 0;
  zddcy[15] = settings;
  F16(zddcy, 2 * 32) = 1698;
  zddcy[8] = 256;
  zddcy[9] = 256;
  zddcy[10] = 0;
  zddcy[11] = 0;
  zddcy[12] = 2;
  F16(zddcy, 2 * 33) = group;
  zddcy[14] = 0;
  F32(SYS32(7964), 4) = 1;
  zddcz = SYS32(4264);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4264)));
  zddcz[7] = zdd;
  F16(zddcz, 2 * 2) = 1;
  v317 = (intptr_t)g_eng.op_new(8u);
  v318 = zddcz;
  v319 = 0;
  for ( *zddcz = v317; (uint16_t)v319 < F16(zddcz, 2 * 2); v318 = zddcz )
  {
    v320 = 8 * (uint16_t)v319++;
    F32((*v318 + v320), 0) = 0;
    F32(*zddcz + v320, 4) = 0;
  }
  zddcz[2] = 0;
  zddcz[6] = 0;
  zddcz[15] = settings;
  F16(zddcz, 2 * 32) = 1424;
  zddcz[8] = 32;
  zddcz[9] = 32;
  zddcz[10] = 0x1FFFFFF;
  zddcz[11] = 1;
  zddcz[12] = 0;
  F16(zddcz, 2 * 33) = group;
  zddcz[14] = 0;
  al_582b80(*(ar_sprite_slot **)&SYSB(4256), *(ar_sprite_slot **)&SYSB(4268));
  al_582d00(*(ar_info_entry **)&SYSB(7904));
  (*(uint16_t *)SYS32(7904)) = (*(uint16_t *)SYS32(7892));
  F32(SYS32(7904), 4) = F32(SYS32(7892), 4);
  F32(SYS32(7904), 8) = D(0x6752f8);
  al_582b80(*(ar_sprite_slot **)&SYSB(4260), *(ar_sprite_slot **)&SYSB(4272));
  al_582d00(*(ar_info_entry **)&SYSB(7908));
  (*(uint16_t *)SYS32(7908)) = (*(uint16_t *)SYS32(7896));
  F32(SYS32(7908), 4) = F32(SYS32(7896), 4);
  F32(SYS32(7908), 8) = D(0x6752f8);
  zddda = SYS32(4276);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4276)));
  zddda[7] = zdd;
  F16(zddda, 2 * 2) = 1;
  v321 = (intptr_t)g_eng.op_new(8u);
  v322 = zddda;
  v323 = 0;
  for ( *zddda = v321; (uint16_t)v323 < F16(zddda, 2 * 2); v322 = zddda )
  {
    v324 = 8 * (uint16_t)v323++;
    F32((*v322 + v324), 0) = 0;
    F32(*zddda + v324, 4) = 0;
  }
  zddda[2] = 0;
  zddda[6] = 0;
  zddda[15] = settings;
  F16(zddda, 2 * 32) = 1423;
  zddda[8] = 64;
  zddda[9] = 160;
  zddda[10] = 0;
  zddda[11] = 0;
  zddda[12] = 2;
  F16(zddda, 2 * 33) = group;
  zddda[14] = 0;
  F32(SYS32(7912), 4) = 2;
  zdddb = SYS32(4280);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4280)));
  zdddb[7] = zdd;
  F16(zdddb, 2 * 2) = 1;
  v325 = (intptr_t)g_eng.op_new(8u);
  v326 = zdddb;
  v327 = 0;
  for ( *zdddb = v325; (uint16_t)v327 < F16(zdddb, 2 * 2); v326 = zdddb )
  {
    v328 = 8 * (uint16_t)v327++;
    F32((v328 + *v326), 0) = 0;
    F32(v328 + *zdddb, 4) = 0;
  }
  zdddb[2] = 0;
  zdddb[6] = 0;
  zdddb[15] = settings;
  F16(zdddb, 2 * 32) = 1422;
  zdddb[8] = 128;
  zdddb[9] = 160;
  zdddb[10] = 0;
  zdddb[11] = 0;
  zdddb[12] = 2;
  F16(zdddb, 2 * 33) = group;
  zdddb[14] = 0;
  F32(SYS32(7916), 4) = 2;
  zdddc = SYS32(4284);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4284)));
  zdddc[7] = zdd;
  F16(zdddc, 2 * 2) = 1;
  v329 = (intptr_t)g_eng.op_new(8u);
  v330 = zdddc;
  v331 = 0;
  for ( *zdddc = v329; (uint16_t)v331 < F16(zdddc, 2 * 2); v330 = zdddc )
  {
    v332 = 8 * (uint16_t)v331++;
    F32((v332 + *v330), 0) = 0;
    F32(v332 + *zdddc, 4) = 0;
  }
  zdddc[2] = 0;
  zdddc[6] = 0;
  zdddc[15] = settings;
  F16(zdddc, 2 * 32) = 1897;
  zdddc[8] = 64;
  zdddc[9] = 160;
  zdddc[10] = 0;
  zdddc[11] = 0;
  zdddc[12] = 2;
  F16(zdddc, 2 * 33) = group;
  zdddc[14] = 0;
  F32(SYS32(7920), 4) = 2;
  zdddd = SYS32(4288);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4288)));
  zdddd[7] = zdd;
  F16(zdddd, 2 * 2) = 1;
  v333 = (intptr_t)g_eng.op_new(8u);
  v334 = zdddd;
  v335 = 0;
  for ( *zdddd = v333; (uint16_t)v335 < F16(zdddd, 2 * 2); v334 = zdddd )
  {
    v336 = 8 * (uint16_t)v335++;
    F32((v336 + *v334), 0) = 0;
    F32(v336 + *zdddd, 4) = 0;
  }
  zdddd[2] = 0;
  zdddd[6] = 0;
  zdddd[15] = settings;
  F16(zdddd, 2 * 32) = 1898;
  zdddd[8] = 128;
  zdddd[9] = 160;
  zdddd[10] = 0;
  zdddd[11] = 0;
  zdddd[12] = 2;
  F16(zdddd, 2 * 33) = group;
  zdddd[14] = 0;
  F32(SYS32(7924), 4) = 2;
  zddde = SYS32(4292);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4292)));
  zddde[7] = zdd;
  F16(zddde, 2 * 2) = 1;
  v337 = (intptr_t)g_eng.op_new(8u);
  v338 = zddde;
  v339 = 0;
  for ( *zddde = v337; (uint16_t)v339 < F16(zddde, 2 * 2); v338 = zddde )
  {
    v340 = 8 * (uint16_t)v339++;
    F32((v340 + *v338), 0) = 0;
    F32(v340 + *zddde, 4) = 0;
  }
  zddde[2] = 0;
  zddde[6] = 0;
  zddde[15] = settings;
  F16(zddde, 2 * 32) = 1899;
  zddde[8] = 32;
  zddde[9] = 32;
  zddde[10] = 0x1FFFFFF;
  zddde[11] = 1;
  zddde[12] = 0;
  F16(zddde, 2 * 33) = group;
  zddde[14] = 0;
  al_582b80(*(ar_sprite_slot **)&SYSB(4284), *(ar_sprite_slot **)&SYSB(4296));
  al_582d00(*(ar_info_entry **)&SYSB(7932));
  (*(uint16_t *)SYS32(7932)) = (*(uint16_t *)SYS32(7920));
  F32(SYS32(7932), 4) = F32(SYS32(7920), 4);
  F32(SYS32(7932), 8) = D(0x675500);
  al_582b80(*(ar_sprite_slot **)&SYSB(4288), *(ar_sprite_slot **)&SYSB(4300));
  al_582d00(*(ar_info_entry **)&SYSB(7936));
  (*(uint16_t *)SYS32(7936)) = (*(uint16_t *)SYS32(7924));
  F32(SYS32(7936), 4) = F32(SYS32(7924), 4);
  F32(SYS32(7936), 8) = D(0x675500);
  zdddf = SYS32(4304);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4304)));
  zdddf[7] = zdd;
  F16(zdddf, 2 * 2) = 1;
  v341 = (intptr_t)g_eng.op_new(8u);
  v342 = zdddf;
  v343 = 0;
  for ( *zdddf = v341; (uint16_t)v343 < F16(zdddf, 2 * 2); v342 = zdddf )
  {
    v344 = 8 * (uint16_t)v343++;
    F32((v344 + *v342), 0) = 0;
    F32(v344 + *zdddf, 4) = 0;
  }
  zdddf[2] = 0;
  zdddf[6] = 0;
  zdddf[15] = settings;
  F16(zdddf, 2 * 32) = 1718;
  zdddf[8] = 96;
  zdddf[9] = 192;
  zdddf[10] = 0;
  zdddf[11] = 0;
  zdddf[12] = 2;
  F16(zdddf, 2 * 33) = group;
  zdddf[14] = 0;
  F32(SYS32(7940), 4) = 1;
  zdddg = SYS32(4308);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4308)));
  zdddg[7] = zdd;
  F16(zdddg, 2 * 2) = 1;
  v345 = (intptr_t)g_eng.op_new(8u);
  v346 = zdddg;
  v347 = 0;
  for ( *zdddg = v345; (uint16_t)v347 < F16(zdddg, 2 * 2); v346 = zdddg )
  {
    v348 = 8 * (uint16_t)v347++;
    F32((*v346 + v348), 0) = 0;
    F32(*zdddg + v348, 4) = 0;
  }
  zdddg[2] = 0;
  zdddg[6] = 0;
  zdddg[15] = settings;
  F16(zdddg, 2 * 32) = 1933;
  zdddg[8] = 64;
  zdddg[9] = 160;
  zdddg[10] = 0;
  zdddg[11] = 0;
  zdddg[12] = 2;
  F16(zdddg, 2 * 33) = group;
  zdddg[14] = 0;
  F32(SYS32(7944), 4) = 1;
  zdddh = SYS32(4312);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4312)));
  zdddh[7] = zdd;
  F16(zdddh, 2 * 2) = 1;
  v349 = (intptr_t)g_eng.op_new(8u);
  v350 = zdddh;
  v351 = 0;
  for ( *zdddh = v349; (uint16_t)v351 < F16(zdddh, 2 * 2); v350 = zdddh )
  {
    v352 = 8 * (uint16_t)v351++;
    F32((v352 + *v350), 0) = 0;
    F32(v352 + *zdddh, 4) = 0;
  }
  zdddh[2] = 0;
  zdddh[6] = 0;
  zdddh[15] = settings;
  F16(zdddh, 2 * 32) = 1934;
  zdddh[8] = 64;
  zdddh[9] = 160;
  zdddh[10] = 0;
  zdddh[11] = 0;
  zdddh[12] = 2;
  F16(zdddh, 2 * 33) = group;
  zdddh[14] = 0;
  F32(SYS32(7948), 4) = 1;
  zdddi = SYS32(4316);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4316)));
  zdddi[7] = zdd;
  F16(zdddi, 2 * 2) = 1;
  v353 = (intptr_t)g_eng.op_new(8u);
  v354 = zdddi;
  v355 = 0;
  for ( *zdddi = v353; (uint16_t)v355 < F16(zdddi, 2 * 2); v354 = zdddi )
  {
    v356 = 8 * (uint16_t)v355++;
    F32((v356 + *v354), 0) = 0;
    F32(v356 + *zdddi, 4) = 0;
  }
  zdddi[2] = 0;
  zdddi[6] = 0;
  zdddi[15] = settings;
  F16(zdddi, 2 * 32) = 1795;
  zdddi[8] = 64;
  zdddi[9] = 160;
  zdddi[10] = 0;
  zdddi[11] = 0;
  zdddi[12] = 2;
  F16(zdddi, 2 * 33) = group;
  zdddi[14] = 0;
  F32(SYS32(7952), 4) = 1;
  zdddj = SYS32(4332);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4332)));
  zdddj[7] = zdd;
  F16(zdddj, 2 * 2) = 1;
  v357 = (intptr_t)g_eng.op_new(8u);
  v358 = zdddj;
  v359 = 0;
  for ( *zdddj = v357; (uint16_t)v359 < F16(zdddj, 2 * 2); v358 = zdddj )
  {
    v360 = 8 * (uint16_t)v359++;
    F32((v360 + *v358), 0) = 0;
    F32(v360 + *zdddj, 4) = 0;
  }
  zdddj[2] = 0;
  zdddj[6] = 0;
  zdddj[15] = settings;
  F16(zdddj, 2 * 32) = 2231;
  zdddj[8] = 64;
  zdddj[9] = 128;
  zdddj[10] = 0;
  zdddj[11] = 0;
  zdddj[12] = 2;
  F16(zdddj, 2 * 33) = group;
  zdddj[14] = 0;
  F32(SYS32(7968), 4) = 1;
  zdddk = SYS32(4336);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4336)));
  zdddk[7] = zdd;
  F16(zdddk, 2 * 2) = 1;
  v361 = (intptr_t)g_eng.op_new(8u);
  v362 = zdddk;
  v363 = 0;
  for ( *zdddk = v361; (uint16_t)v363 < F16(zdddk, 2 * 2); v362 = zdddk )
  {
    v364 = 8 * (uint16_t)v363++;
    F32((v364 + *v362), 0) = 0;
    F32(v364 + *zdddk, 4) = 0;
  }
  zdddk[2] = 0;
  zdddk[6] = 0;
  zdddk[15] = settings;
  F16(zdddk, 2 * 32) = 2232;
  zdddk[8] = 64;
  zdddk[9] = 192;
  zdddk[10] = 0;
  zdddk[11] = 0;
  zdddk[12] = 2;
  F16(zdddk, 2 * 33) = group;
  zdddk[14] = 0;
  F32(SYS32(7972), 4) = 1;
  zdddl = SYS32(4340);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(4340)));
  zdddl[7] = zdd;
  F16(zdddl, 2 * 2) = 1;
  v365 = (intptr_t)g_eng.op_new(8u);
  v366 = zdddl;
  v367 = 0;
  for ( *zdddl = v365; (uint16_t)v367 < F16(zdddl, 2 * 2); v366 = zdddl )
  {
    v368 = 8 * (uint16_t)v367++;
    F32((v368 + *v366), 0) = 0;
    F32(v368 + *zdddl, 4) = 0;
  }
  zdddl[2] = 0;
  zdddl[6] = 0;
  zdddl[15] = settings;
  F16(zdddl, 2 * 32) = 2233;
  zdddl[8] = 160;
  zdddl[9] = 192;
  zdddl[10] = 0;
  zdddl[11] = 0;
  zdddl[12] = 2;
  F16(zdddl, 2 * 33) = group;
  zdddl[14] = 0;
  F32(SYS32(7976), 4) = 1;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4344), zdd, settings, 0x42Bu, 0x40u, 0xE0u, 0, 0, 1u, group);
  F32(SYS32(7980), 4) = 2;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4348), zdd, settings, 0x42Cu, 0x40u, 0x40u, 0, 0, 1u, group);
  F32(SYS32(7984), 4) = 2;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4352), zdd, settings, 0x42Du, 0xC0u, 0xE0u, 0, 0, 2u, group);
  F32(SYS32(7988), 4) = 2;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4356), zdd, settings, 0x42Eu, 0x80u, 0xE0u, 0, 0, 2u, group);
  F32(SYS32(7992), 4) = 2;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3896), zdd, settings, 0x442u, 0x60u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7532), 4) = 2;
  (*(uint16_t *)SYS32(7532)) = 28;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x124u), (intptr_t)(0x123u));
  F32(SYS32(7536), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x125u), (intptr_t)(0x123u));
  F32(SYS32(7540), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3908), zdd, settings, 0x447u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7544), 4) = 2;
  (*(uint16_t *)SYS32(7544)) = 24;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x127u), (intptr_t)(0x126u));
  F32(SYS32(7548), 8) = D(0x672e68);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x128u), (intptr_t)(0x126u));
  F32(SYS32(7552), 8) = D(0x673070);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3884), zdd, settings, 0x440u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7520), 4) = 2;
  (*(uint16_t *)SYS32(7520)) = 8;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x121u), (intptr_t)(0x120u));
  F32(SYS32(7524), 8) = D(0x672e68);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x122u), (intptr_t)(0x120u));
  F32(SYS32(7528), 8) = D(0x673070);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3920), zdd, settings, 0x441u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7556), 4) = 2;
  (*(uint16_t *)SYS32(7556)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x12Au), (intptr_t)(0x129u));
  F32(SYS32(7560), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3928), zdd, settings, 0x445u, 0x80u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7564), 4) = 2;
  (*(uint16_t *)SYS32(7564)) = 40;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x12Cu), (intptr_t)(0x12Bu));
  F32(SYS32(7568), 8) = D(0x6740b0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3936), zdd, settings, 0x797u, 0x80u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7572), 4) = 2;
  (*(uint16_t *)SYS32(7572)) = 40;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x12Eu), (intptr_t)(0x12Du));
  F32(SYS32(7576), 8) = D(0x6740b0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3944), zdd, settings, 0x538u, 0xF0u, 0xF0u, 0, 1u, 2u, group);
  F32(SYS32(7580), 4) = 2;
  (*(uint16_t *)SYS32(7580)) = 32;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3968), zdd, settings, 0x53Bu, 0x80u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7604), 4) = 2;
  (*(uint16_t *)SYS32(7604)) = 28;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x137u), (intptr_t)(0x135u));
  F32(SYS32(7612), 8) = D(0x6742b8);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x138u), (intptr_t)(0x135u));
  F32(SYS32(7616), 8) = D(0x6744c0);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3972),
    zdd,
    settings,
    0x900u,
    0x20u,
    0x20u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x139u), (intptr_t)(0x135u));
  F32(SYS32(7620), 8) = D(0x6746c8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(4004), zdd, settings, 0x871u, 0x80u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7640), 4) = 2;
  (*(uint16_t *)SYS32(7640)) = 80;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3948), zdd, settings, 0x7C3u, 0x90u, 0x90u, 0, 1u, 2u, group);
  F32(SYS32(7584), 4) = 2;
  (*(uint16_t *)SYS32(7584)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3952), zdd, settings, 0x7C4u, 0xF0u, 0xF0u, 0, 1u, 2u, group);
  F32(SYS32(7588), 4) = 2;
  (*(uint16_t *)SYS32(7588)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3956), zdd, settings, 0x7C5u, 0x90u, 0x90u, 0, 0, 2u, group);
  F32(SYS32(7592), 4) = 2;
  (*(uint16_t *)SYS32(7592)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3960), zdd, settings, 0x7C6u, 0x60u, 0x60u, 0, 0, 2u, group);
  F32(SYS32(7596), 4) = 2;
  (*(uint16_t *)SYS32(7596)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3964), zdd, settings, 0x7C7u, 0x170u, 0x170u, 0, 0, 2u, group);
  F32(SYS32(7600), 4) = 2;
  (*(uint16_t *)SYS32(7600)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4016), zdd, settings, 0x6B1u, 0x20u, 0x20u, 0, 1u, 2u, group);
  F32(SYS32(7652), 4) = 2;
  (*(uint16_t *)SYS32(7652)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4008), zdd, settings, 0x6ADu, 0x60u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7644), 4) = 2;
  (*(uint16_t *)SYS32(7644)) = 32;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x140u), (intptr_t)(0x13Fu));
  F32(SYS32(7648), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3784), zdd, settings, 0x804u, 0xA0u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7420), 4) = 2;
  (*(uint16_t *)SYS32(7420)) = 32;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3780), zdd, settings, 0x444u, 0xA0u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7416), 4) = 2;
  (*(uint16_t *)SYS32(7416)) = 32;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3788), zdd, settings, 0x6DBu, 0xA0u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7424), 4) = 2;
  (*(uint16_t *)SYS32(7424)) = 28;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x109u), (intptr_t)(0x108u));
  F32(SYS32(7428), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3796), zdd, settings, 0x443u, 0xA0u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7432), 4) = 2;
  (*(uint16_t *)SYS32(7432)) = 32;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x10Bu), (intptr_t)(0x10Au));
  F32(SYS32(7436), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(4020), zdd, settings, 0x669u, 0xA0u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7656), 4) = 2;
  (*(uint16_t *)SYS32(7656)) = 36;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x143u), (intptr_t)(0x142u));
  F32(SYS32(7660), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x144u), (intptr_t)(0x142u));
  F32(SYS32(7664), 8) = D(0x674ad8);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x145u), (intptr_t)(0x142u));
  F32(SYS32(7668), 8) = D(0x674ce0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3996), zdd, settings, 0x7D4u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7632), 4) = 2;
  (*(uint16_t *)SYS32(7632)) = 32;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x13Du), (intptr_t)(0x13Cu));
  F32(SYS32(7636), 8) = D(0x673278);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3988), zdd, settings, 0x666u, 0x60u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7624), 4) = 2;
  (*(uint16_t *)SYS32(7624)) = 40;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x13Bu), (intptr_t)(0x13Au));
  F32(SYS32(7628), 8) = D(0x673480);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3864), zdd, settings, 0x7B9u, 0x80u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7500), 4) = 2;
  (*(uint16_t *)SYS32(7500)) = 24;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x11Cu), (intptr_t)(0x11Bu));
  F32(SYS32(7504), 8) = D(0x673a98);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x11Du), (intptr_t)(0x11Bu));
  F32(SYS32(7508), 8) = D(0x673ca0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3876), zdd, settings, 0x8BEu, 0x80u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7512), 4) = 2;
  (*(uint16_t *)SYS32(7512)) = 24;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x11Fu), (intptr_t)(0x11Eu));
  F32(SYS32(7516), 8) = D(0x673ea8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3804), zdd, settings, 0x446u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7440), 4) = 2;
  (*(uint16_t *)SYS32(7440)) = 24;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3808), zdd, settings, 0x6ACu, 0xA0u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7444), 4) = 2;
  (*(uint16_t *)SYS32(7444)) = 8;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x10Eu), (intptr_t)(0x10Cu));
  F32(SYS32(7448), 8) = D(0x672440);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x10Fu), (intptr_t)(0x10Cu));
  F32(SYS32(7452), 8) = D(0x672648);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x110u), (intptr_t)(0x10Du));
  F32(SYS32(7456), 8) = D(0x672648);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x111u), (intptr_t)(0x10Cu));
  F32(SYS32(7460), 8) = D(0x672850);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x112u), (intptr_t)(0x10Du));
  F32(SYS32(7464), 8) = D(0x672850);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x113u), (intptr_t)(0x10Cu));
  F32(SYS32(7468), 8) = D(0x672a58);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x114u), (intptr_t)(0x10Du));
  F32(SYS32(7472), 8) = D(0x672a58);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x115u), (intptr_t)(0x10Cu));
  F32(SYS32(7476), 8) = D(0x672c60);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x116u), (intptr_t)(0x10Du));
  F32(SYS32(7480), 8) = D(0x672c60);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3848), zdd, settings, 0x71Du, 0x80u, 0x80u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(7484)) = 32;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x118u), (intptr_t)(0x117u));
  F32(SYS32(7488), 8) = D(0x673688);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x119u), (intptr_t)(0x117u));
  F32(SYS32(7492), 8) = D(0x673890);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3860), zdd, settings, 0x7ACu, 0xE0u, 0x100u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(7496)) = 12;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4036), zdd, settings, 0x3FAu, 0x20u, 0x20u, 0, 1u, 2u, group);
  F32(SYS32(7672), 4) = 2;
  (*(uint16_t *)SYS32(7672)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4040), zdd, settings, 0x3F4u, 0x30u, 0x30u, 0, 1u, 2u, group);
  F32(SYS32(7676), 4) = 2;
  (*(uint16_t *)SYS32(7676)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4044), zdd, settings, 0x3FCu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7680), 4) = 2;
  (*(uint16_t *)SYS32(7680)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4048), zdd, settings, 0x3FBu, 0x80u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7684), 4) = 2;
  (*(uint16_t *)SYS32(7684)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x14Au), (intptr_t)(0x149u));
  F32(SYS32(7688), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x14Bu), (intptr_t)(0x149u));
  F32(SYS32(7692), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(4060), zdd, settings, 0x3F8u, 0x40u, 0x40u, 0, 1u, 2u, group);
  F32(SYS32(7696), 4) = 2;
  (*(uint16_t *)SYS32(7696)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x14Du), (intptr_t)(0x14Cu));
  F32(SYS32(7700), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(4068), zdd, settings, 0x3F3u, 0x20u, 0x20u, 0, 1u, 2u, group);
  F32(SYS32(7704), 4) = 2;
  (*(uint16_t *)SYS32(7704)) = 5;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x14Fu), (intptr_t)(0x14Eu));
  F32(SYS32(7708), 8) = D(0x6748d0);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4080),
    zdd,
    settings,
    0x3F5u,
    0x20u,
    0x20u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4084),
    zdd,
    settings,
    0x3F6u,
    0x20u,
    0x20u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4088),
    zdd,
    settings,
    0x3F7u,
    0x20u,
    0x20u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4092),
    zdd,
    settings,
    0x6BFu,
    0x20u,
    0x20u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(4096),
    zdd,
    settings,
    0x6BEu,
    0x20u,
    0x20u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(4100), zdd, settings, 0x3FEu, 0x40u, 0x40u, 0, 1u, 2u, group);
  F32(SYS32(7736), 4) = 2;
  (*(uint16_t *)SYS32(7736)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4120), zdd, settings, 0x3FDu, 0x40u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7756), 4) = 2;
  (*(uint16_t *)SYS32(7756)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4124), zdd, settings, 0x6D9u, 0x80u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7760), 4) = 2;
  (*(uint16_t *)SYS32(7760)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4128), zdd, settings, 0x8BDu, 0x60u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7764), 4) = 2;
  (*(uint16_t *)SYS32(7764)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4132), zdd, settings, 0x78Fu, 0xC0u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7768), 4) = 2;
  (*(uint16_t *)SYS32(7768)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4472), zdd, settings, 0x47Fu, 0x28u, 0x28u, 0, 1u, 2u, group);
  F32(SYS32(8108), 4) = 2;
  (*(uint16_t *)SYS32(8108)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4476), zdd, settings, 0x480u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(8112), 4) = 2;
  (*(uint16_t *)SYS32(8112)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4480), zdd, 0, 0x5ABu, 0x20u, 0x20u, 0, 1u, 2u, group);
  F32(SYS32(8116), 4) = 2;
  (*(uint16_t *)SYS32(8116)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4168), zdd, settings, 0x667u, 0x80u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7804), 4) = 2;
  (*(uint16_t *)SYS32(7804)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4172), zdd, settings, 0x6DAu, 0x80u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7808), 4) = 2;
  (*(uint16_t *)SYS32(7808)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4176), zdd, settings, 0x6B5u, 0x80u, 0xE0u, 0, 1u, 2u, group);
  F32(SYS32(7812), 4) = 2;
  (*(uint16_t *)SYS32(7812)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4180), zdd, settings, 0x87Bu, 0x80u, 0xE0u, 0, 1u, 2u, group);
  F32(SYS32(7816), 4) = 2;
  (*(uint16_t *)SYS32(7816)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4112), zdd, settings, 0x6B3u, 0x80u, 0xA0u, 0, 1u, 2u, group);
  F32(SYS32(7748), 4) = 2;
  (*(uint16_t *)SYS32(7748)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4116), zdd, settings, 0x78Cu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7752), 4) = 2;
  (*(uint16_t *)SYS32(7752)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4104), zdd, settings, 0x6D8u, 0x100u, 0x40u, 0, 1u, 2u, group);
  F32(SYS32(7740), 4) = 2;
  (*(uint16_t *)SYS32(7740)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4108), zdd, settings, 0x789u, 0x20u, 0x20u, 0, 1u, 2u, group);
  F32(SYS32(7744), 4) = 2;
  (*(uint16_t *)SYS32(7744)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4360), zdd, settings, 0x43Bu, 0x60u, 0x60u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(7996)) = 1;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4364), zdd, settings, 0x43Cu, 0x30u, 0x30u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8000)) = 8;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4368), zdd, settings, 0x875u, 0x30u, 0x30u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8004)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4372), zdd, settings, 0x878u, 0x40u, 0x40u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8008)) = 24;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4376), zdd, settings, 0x43Eu, 0x40u, 0x40u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8012)) = 40;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4384), zdd, settings, 0x43Fu, 0xA0u, 0xC0u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8020)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4380), zdd, settings, 0x7BDu, 0x40u, 0x40u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8016)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4388), zdd, settings, 0x806u, 0xA0u, 0xA0u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8024)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4392), zdd, settings, 0x874u, 0xA0u, 0xA0u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8028)) = 12;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4396), zdd, settings, 0x704u, 0x40u, 0x40u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8032)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4400), zdd, settings, 0x409u, 0x40u, 0x40u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8036)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4404), zdd, settings, 0x7E5u, 0x40u, 0x40u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8040)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4408), zdd, settings, 0x40Au, 0x40u, 0x40u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8044)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4412), zdd, settings, 0x40Bu, 0x20u, 0x20u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8048)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4416), zdd, settings, 0x539u, 0x20u, 0x20u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8052)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4424), zdd, settings, 0x40Cu, 0x80u, 0x80u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8060)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4420), zdd, settings, 0x8FFu, 0x20u, 0x20u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8056)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4432), zdd, settings, 0x40Du, 0xB0u, 0xB0u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8068)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4428), zdd, settings, 0x40Eu, 0x50u, 0x50u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8064)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4436), zdd, settings, 0x408u, 0x20u, 0x20u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8072)) = 0;
  F32(SYS32(8072), 4) = 1;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4440), zdd, settings, 0x408u, 0x20u, 0x20u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8076)) = 0;
  F32(SYS32(8076), 4) = 2;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4444), zdd, settings, 0x407u, 0x60u, 0x60u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8080)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4448), zdd, settings, 0x591u, 0x90u, 0x90u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8084)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4452), zdd, settings, 0x406u, 0x40u, 0x40u, 0, 1u, 0, group);
  (*(uint16_t *)SYS32(8088)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4456), zdd, settings, 0x410u, 0x80u, 0x60u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8092)) = 12;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4460), zdd, settings, 0x7D0u, 0x50u, 0x60u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8096)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4464), zdd, settings, 0x7D3u, 0xC0u, 0x60u, 0, 1u, 2u, group);
  (*(uint16_t *)SYS32(8100)) = 1;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3288), zdd, 0, 0x570u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6924), 4) = 2;
  (*(uint16_t *)SYS32(6924)) = 152;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3292), zdd, 0, 0x571u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6928), 4) = 2;
  (*(uint16_t *)SYS32(6928)) = 192;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3296), zdd, 0, 0x572u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(6932), 4) = 2;
  (*(uint16_t *)SYS32(6932)) = 8;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3300), zdd, 0, 0x573u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6936), 4) = 2;
  (*(uint16_t *)SYS32(6936)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3304), zdd, 0, 0x574u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6940), 4) = 2;
  (*(uint16_t *)SYS32(6940)) = 136;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3308), zdd, 0, 0x575u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6944), 4) = 2;
  (*(uint16_t *)SYS32(6944)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3312), zdd, 0, 0x576u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6948), 4) = 2;
  (*(uint16_t *)SYS32(6948)) = 160;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3316), zdd, 0, 0x577u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6952), 4) = 2;
  (*(uint16_t *)SYS32(6952)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4076), zdd, 0, 0x57Cu, 0x40u, 0x40u, 0, 1u, 2u, group);
  F32(SYS32(7712), 4) = 2;
  (*(uint16_t *)SYS32(7712)) = 32;
  al_5748c0(*(ar_sprite_slot **)&SYSB(4468), zdd, settings, 0x47Eu, 0x40u, 0x40u, 0, 1u, 2u, group);
  F32(SYS32(8104), 4) = 2;
  (*(uint16_t *)SYS32(8104)) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3744), zdd, settings, 0x79Du, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7380), 4) = 2;
  (*(uint16_t *)SYS32(7380)) = 1;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3724), zdd, settings, 0x79Fu, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7360), 4) = 2;
  (*(uint16_t *)SYS32(7360)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3748), zdd, settings, 0x79Eu, 0x50u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7384), 4) = 2;
  (*(uint16_t *)SYS32(7384)) = 8;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xFFu), (intptr_t)(0xFEu));
  F32(SYS32(7388), 8) = D(0x674ee8);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x100u), (intptr_t)(0xFEu));
  F32(SYS32(7392), 8) = D(0x6750f0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3320), zdd, settings, 0x459u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6956), 4) = 2;
  (*(uint16_t *)SYS32(6956)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x94u), (intptr_t)(0x93u));
  F32(SYS32(6960), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3328), zdd, settings, 0x45Au, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6964), 4) = 2;
  (*(uint16_t *)SYS32(6964)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x96u), (intptr_t)(0x95u));
  F32(SYS32(6968), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3336), zdd, settings, 0x7A3u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6972), 4) = 2;
  (*(uint16_t *)SYS32(6972)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x98u), (intptr_t)(0x97u));
  F32(SYS32(6976), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3344), zdd, settings, 0x45Bu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6980), 4) = 2;
  (*(uint16_t *)SYS32(6980)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x9Au), (intptr_t)(0x99u));
  F32(SYS32(6984), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x9Bu), (intptr_t)(0x99u));
  F32(SYS32(6988), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3356), zdd, settings, 0x45Cu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(6992), 4) = 2;
  (*(uint16_t *)SYS32(6992)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0x9Du), (intptr_t)(0x9Cu));
  F32(SYS32(6996), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3364), zdd, settings, 0x45Du, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7000), 4) = 2;
  (*(uint16_t *)SYS32(7000)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3368), zdd, settings, 0x45Eu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7004), 4) = 2;
  (*(uint16_t *)SYS32(7004)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3372), zdd, settings, 0x45Fu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7008), 4) = 2;
  (*(uint16_t *)SYS32(7008)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3376), zdd, settings, 0x460u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7012), 4) = 2;
  (*(uint16_t *)SYS32(7012)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3380), zdd, settings, 0x786u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7016), 4) = 2;
  (*(uint16_t *)SYS32(7016)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xA3u), (intptr_t)(0xA2u));
  F32(SYS32(7020), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xA4u), (intptr_t)(0xA2u));
  F32(SYS32(7024), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3392), zdd, settings, 0x461u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7028), 4) = 2;
  (*(uint16_t *)SYS32(7028)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xA6u), (intptr_t)(0xA5u));
  F32(SYS32(7032), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xA7u), (intptr_t)(0xA5u));
  F32(SYS32(7036), 8) = D(0x674ad8);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xA8u), (intptr_t)(0xA5u));
  F32(SYS32(7040), 8) = D(0x674ce0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3408), zdd, settings, 0x462u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7044), 4) = 2;
  (*(uint16_t *)SYS32(7044)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xAAu), (intptr_t)(0xA9u));
  F32(SYS32(7048), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xABu), (intptr_t)(0xA9u));
  F32(SYS32(7052), 8) = D(0x674ad8);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xACu), (intptr_t)(0xA9u));
  F32(SYS32(7056), 8) = D(0x674ce0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3424), zdd, settings, 0x463u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7060), 4) = 2;
  (*(uint16_t *)SYS32(7060)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xAEu), (intptr_t)(0xADu));
  F32(SYS32(7064), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xAFu), (intptr_t)(0xADu));
  F32(SYS32(7068), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3460), zdd, settings, 0x8B5u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7096), 4) = 2;
  (*(uint16_t *)SYS32(7096)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xB7u), (intptr_t)(0xB6u));
  F32(SYS32(7100), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3468), zdd, settings, 0x6BBu, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7104), 4) = 2;
  (*(uint16_t *)SYS32(7104)) = 5;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xB9u), (intptr_t)(0xB8u));
  F32(SYS32(7108), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xBAu), (intptr_t)(0xB8u));
  F32(SYS32(7112), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3480), zdd, settings, 0x6BCu, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7116), 4) = 2;
  (*(uint16_t *)SYS32(7116)) = 5;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xBCu), (intptr_t)(0xBBu));
  F32(SYS32(7120), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xBDu), (intptr_t)(0xBBu));
  F32(SYS32(7124), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3492), zdd, settings, 0x6BDu, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7128), 4) = 2;
  (*(uint16_t *)SYS32(7128)) = 5;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xBFu), (intptr_t)(0xBEu));
  F32(SYS32(7132), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xC0u), (intptr_t)(0xBEu));
  F32(SYS32(7136), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3504), zdd, settings, 0x6D7u, 0x50u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7140), 4) = 2;
  (*(uint16_t *)SYS32(7140)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xC2u), (intptr_t)(0xC1u));
  F32(SYS32(7144), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xC3u), (intptr_t)(0xC1u));
  F32(SYS32(7148), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3516), zdd, settings, 0x707u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7152), 4) = 2;
  (*(uint16_t *)SYS32(7152)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xC5u), (intptr_t)(0xC4u));
  F32(SYS32(7156), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3524), zdd, settings, 0x708u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7160), 4) = 2;
  (*(uint16_t *)SYS32(7160)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3528), zdd, settings, 0x709u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7164), 4) = 2;
  (*(uint16_t *)SYS32(7164)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3536), zdd, settings, 0x77Au, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7172), 4) = 2;
  (*(uint16_t *)SYS32(7172)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3540), zdd, settings, 0x77Bu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7176), 4) = 2;
  (*(uint16_t *)SYS32(7176)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3544), zdd, settings, 0x77Cu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7180), 4) = 2;
  (*(uint16_t *)SYS32(7180)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3436), zdd, settings, 0x464u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7072), 4) = 2;
  (*(uint16_t *)SYS32(7072)) = 5;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xB1u), (intptr_t)(0xB0u));
  F32(SYS32(7076), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xB2u), (intptr_t)(0xB0u));
  F32(SYS32(7080), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3448), zdd, settings, 0x465u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7084), 4) = 2;
  (*(uint16_t *)SYS32(7084)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3452), zdd, settings, 0x466u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7088), 4) = 2;
  (*(uint16_t *)SYS32(7088)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3456), zdd, settings, 0x467u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7092), 4) = 2;
  (*(uint16_t *)SYS32(7092)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3548), zdd, settings, 0x468u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7184), 4) = 2;
  (*(uint16_t *)SYS32(7184)) = 5;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xCDu), (intptr_t)(0xCCu));
  F32(SYS32(7188), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xCEu), (intptr_t)(0xCCu));
  F32(SYS32(7192), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3560), zdd, settings, 0x469u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7196), 4) = 2;
  (*(uint16_t *)SYS32(7196)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3564), zdd, settings, 0x46Au, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7200), 4) = 2;
  (*(uint16_t *)SYS32(7200)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xD1u), (intptr_t)(0xD0u));
  F32(SYS32(7204), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3572), zdd, settings, 0x6A5u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7208), 4) = 2;
  (*(uint16_t *)SYS32(7208)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xD3u), (intptr_t)(0xD2u));
  F32(SYS32(7212), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3580), zdd, settings, 0x46Bu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7216), 4) = 2;
  (*(uint16_t *)SYS32(7216)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xD5u), (intptr_t)(0xD4u));
  F32(SYS32(7220), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3588), zdd, settings, 0x46Cu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7224), 4) = 2;
  (*(uint16_t *)SYS32(7224)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3592), zdd, settings, 0x46Du, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7228), 4) = 2;
  (*(uint16_t *)SYS32(7228)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xD8u), (intptr_t)(0xD7u));
  F32(SYS32(7232), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3600), zdd, settings, 0x46Eu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7236), 4) = 2;
  (*(uint16_t *)SYS32(7236)) = 16;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xDAu), (intptr_t)(0xD9u));
  F32(SYS32(7240), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3608), zdd, settings, 0x46Fu, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7244), 4) = 2;
  (*(uint16_t *)SYS32(7244)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3612), zdd, settings, 0x470u, 0x50u, 0x50u, 0, 1u, 2u, group);
  F32(SYS32(7248), 4) = 2;
  (*(uint16_t *)SYS32(7248)) = 16;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3616), zdd, settings, 0x471u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7252), 4) = 2;
  (*(uint16_t *)SYS32(7252)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xDEu), (intptr_t)(0xDDu));
  F32(SYS32(7256), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xDFu), (intptr_t)(0xDDu));
  F32(SYS32(7260), 8) = D(0x674ad8);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xE0u), (intptr_t)(0xDDu));
  F32(SYS32(7264), 8) = D(0x674ce0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3632), zdd, settings, 0x472u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7268), 4) = 2;
  (*(uint16_t *)SYS32(7268)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xE2u), (intptr_t)(0xE1u));
  F32(SYS32(7272), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3640), zdd, settings, 0x473u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7276), 4) = 2;
  (*(uint16_t *)SYS32(7276)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3644), zdd, settings, 0x474u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7280), 4) = 2;
  (*(uint16_t *)SYS32(7280)) = 4;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3648), zdd, settings, 0x475u, 0x50u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7284), 4) = 2;
  (*(uint16_t *)SYS32(7284)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xE6u), (intptr_t)(0xE5u));
  F32(SYS32(7288), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xE7u), (intptr_t)(0xE5u));
  F32(SYS32(7292), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3660), zdd, settings, 0x476u, 0x50u, 0x80u, 0, 1u, 2u, group);
  F32(SYS32(7296), 4) = 2;
  (*(uint16_t *)SYS32(7296)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xE9u), (intptr_t)(0xE8u));
  F32(SYS32(7300), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xEAu), (intptr_t)(0xE8u));
  F32(SYS32(7304), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3672), zdd, settings, 0x477u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7308), 4) = 2;
  (*(uint16_t *)SYS32(7308)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xECu), (intptr_t)(0xEBu));
  F32(SYS32(7312), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xEDu), (intptr_t)(0xEBu));
  F32(SYS32(7316), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3684), zdd, settings, 0x478u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7320), 4) = 2;
  (*(uint16_t *)SYS32(7320)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xEFu), (intptr_t)(0xEEu));
  F32(SYS32(7324), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3692), zdd, settings, 0x479u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7328), 4) = 2;
  (*(uint16_t *)SYS32(7328)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xF1u), (intptr_t)(0xF0u));
  F32(SYS32(7332), 8) = D(0x6748d0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3700), zdd, settings, 0x47Au, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7336), 4) = 2;
  (*(uint16_t *)SYS32(7336)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xF3u), (intptr_t)(0xF2u));
  F32(SYS32(7340), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xF4u), (intptr_t)(0xF2u));
  F32(SYS32(7344), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3712), zdd, settings, 0x784u, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7348), 4) = 2;
  (*(uint16_t *)SYS32(7348)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xF6u), (intptr_t)(0xF5u));
  F32(SYS32(7352), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xF7u), (intptr_t)(0xF5u));
  F32(SYS32(7356), 8) = D(0x674ad8);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3728), zdd, settings, 0x47Bu, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7364), 4) = 2;
  (*(uint16_t *)SYS32(7364)) = 4;
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xFAu), (intptr_t)(0xF9u));
  F32(SYS32(7368), 8) = D(0x6748d0);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xFBu), (intptr_t)(0xF9u));
  F32(SYS32(7372), 8) = D(0x674ad8);
  W_slot_clone((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(0xFCu), (intptr_t)(0xF9u));
  F32(SYS32(7376), 8) = D(0x674ce0);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3532), zdd, settings, 0x70Bu, 0x50u, 0x60u, 0, 1u, 2u, group);
  F32(SYS32(7168), 4) = 2;
  (*(uint16_t *)SYS32(7168)) = 4;
  al_582b80(*(ar_sprite_slot **)&SYSB(3288), *(ar_sprite_slot **)&SYSB(3760));
  qmemcpy(*(void **)&SYSB(7396), *(const void **)&SYSB(6924), 0x14u);
  al_582b80(*(ar_sprite_slot **)&SYSB(3292), *(ar_sprite_slot **)&SYSB(3764));
  qmemcpy(*(void **)&SYSB(7400), *(const void **)&SYSB(6928), 0x14u);
  al_582b80(*(ar_sprite_slot **)&SYSB(3296), *(ar_sprite_slot **)&SYSB(3768));
  qmemcpy(*(void **)&SYSB(7404), *(const void **)&SYSB(6932), 0x14u);
  al_582b80(*(ar_sprite_slot **)&SYSB(3304), *(ar_sprite_slot **)&SYSB(3772));
  qmemcpy(*(void **)&SYSB(7408), *(const void **)&SYSB(6940), 0x14u);
  result = al_582b80(*(ar_sprite_slot **)&SYSB(3312), *(ar_sprite_slot **)&SYSB(3776));
  qmemcpy(*(void **)&SYSB(7412), *(const void **)&SYSB(6948), 0x14u);
  return result;
}

/* 0x5752e0: void sub_5752E0() */
void al_5752e0(void)
{

    uint32_t v0 = 0;
    char * v1 = 0;
    char * v2 = 0;
    char * v3 = 0;
    char v4 = 0;
    uint32_t v5 = 0;
    intptr_t LibraryA = 0;
    intptr_t zdm_ptr = 0;
    uint32_t v8 = 0;
    char * v9 = 0;
    char * v10 = 0;
    char * v11 = 0;
    char v12 = 0;
    uint32_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    uint32_t v16 = 0;
    char * v17 = 0;
    char * v18 = 0;
    char * v19 = 0;
    char v20 = 0;
    uint32_t v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    uint32_t v24 = 0;
    char * v25 = 0;
    char * v26 = 0;
    char * v27 = 0;
    char v28 = 0;
    uint32_t v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    uint32_t v32 = 0;
    char * v33 = 0;
    char * v34 = 0;
    char * v35 = 0;
    char v36 = 0;
    uint32_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    uint32_t v40 = 0;
    char * v41 = 0;
    char * v42 = 0;
    char * v43 = 0;
    char v44 = 0;
    uint32_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    uint32_t v48 = 0;
    char * v49 = 0;
    char * v50 = 0;
    char * v51 = 0;
    char v52 = 0;
    uint32_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    char * v56 = 0;
    intptr_t v57 = 0;
    char * v58 = 0;
    intptr_t v59 = 0;
    char * v60 = 0;
    intptr_t v61 = 0;
    char * v62 = 0;
    intptr_t v63 = 0;
    char * v64 = 0;
    intptr_t v65 = 0;
    char * v66 = 0;
    intptr_t v67 = 0;
    char * v68 = 0;
    intptr_t v69 = 0;
    char * v70 = 0;
    intptr_t v71 = 0;
    char * v72 = 0;
    intptr_t v73 = 0;
    char * v74 = 0;
    intptr_t v75 = 0;
    char * v76 = 0;
    intptr_t v77 = 0;
    char * v78 = 0;
    intptr_t v79 = 0;
    char * v80 = 0;
    intptr_t v81 = 0;
    char * v82 = 0;
    intptr_t v83 = 0;
    char * v84 = 0;
    intptr_t v85 = 0;
    char * v86 = 0;
    intptr_t v87 = 0;
    char * v88 = 0;
    intptr_t v89 = 0;
    char * v90 = 0;
    intptr_t v91 = 0;
    char * v92 = 0;
    intptr_t v93 = 0;
    char * v94 = 0;
    intptr_t v95 = 0;
    char * v96 = 0;
    intptr_t v97 = 0;
    char * v98 = 0;
    intptr_t v99 = 0;
    char * v100 = 0;
    intptr_t v101 = 0;
    char * v102 = 0;
    char * v103 = 0;
    char * v104 = 0;
    char * v105 = 0;
    char * v106 = 0;
    char * v107 = 0;
    char * v108 = 0;
    char * v109 = 0;
    char * v110 = 0;
    char * v111 = 0;
    char * v112 = 0;
    char * v113 = 0;
    char * v114 = 0;
    char * v115 = 0;
    char * v116 = 0;
    char * v117 = 0;
    char * v118 = 0;
    _Alignas(8) uint8_t frame_[0x40c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *LibFileName = (char *)(frame_ + 0x4);   /* [260] at ebp-0x408 */
    char *FileName = (char *)(frame_ + 0x108);   /* [256] at ebp-0x304 */
    char *Buffer = (char *)(frame_ + 0x208);   /* [256] at ebp-0x204 */
    char *TempFileName = (char *)(frame_ + 0x308);   /* [260] at ebp-0x104 */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x40c */

  if ( F32(CFG(), 540) != 1 )
  {
    if ( al_579680(LibFileName) != 0 )
    {
      v0 = strlen(((char *)D(0x8a288c))) + 1;
      v1 = &LibFileName[strlen(LibFileName)];
      qmemcpy(v1, ((char *)D(0x8a288c)), 4 * (v0 >> 2));
      v3 = ((char *)D(0x8a288c) + (4 * (v0 >> 2)));
      v2 = &v1[4 * (v0 >> 2)];
      v4 = v0;
    }
    else
    {
      g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a2850)), D(0x8a9b6c));
      v5 = strlen(((char *)D(0x8a288c))) + 1;
      qmemcpy(LibFileName, ((char *)D(0x8a288c)), 4 * (v5 >> 2));
      v3 = ((char *)D(0x8a288c) + (4 * (v5 >> 2)));
      v2 = &LibFileName[4 * (v5 >> 2)];
      v4 = v5;
    }
    qmemcpy(v2, v3, v4 & 3);
    LibraryA = API_LoadLibraryA((intptr_t)(LibFileName));
    if ( LibraryA == 0 )
      g_eng.h408b90(D(0x8a9530), ((char *)D(0x8a2838)), 0, LibFileName);
    SYS32(792) = LibraryA;
    if ( al_579740(LibraryA, 1039, (int)((char *)D(0x8a282c)), 0x2712u, 1, 8u) != 0 )
    {
      zdm_ptr = SYS32(0x2884);
      if ( SYS32(0x2884) != 0 )
      {
        if ( F32(CFG(), 372) != 0 )
        {
          W_sd_5bbe20((intptr_t)(SYS32(0x2884)), (intptr_t)(F32(CFG(), 388)));
        }
        else
        {
          W_sd_5bcb30((intptr_t)(SYS32(0x2884)));
          F16(D(0x8a93e8), 0) = 0;
        }
        zdm_ptr = SYS32(0x2884);
      }
      if ( zdm_ptr != 0 )
      {
        g_eng.h5bf3ee(Buffer, "1%07d.wma", 1013);
        if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(LibFileName)) != 0 && API_GetTempFileNameA((intptr_t)(LibFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(TempFileName)) != 0 )
        {
          v13 = strlen(TempFileName) + 1;
          qmemcpy(FileName, TempFileName, 4 * (v13 >> 2));
          v11 = &TempFileName[4 * (v13 >> 2)];
          v10 = &FileName[4 * (v13 >> 2)];
          v12 = v13;
        }
        else
        {
          strcpy(FileName, (const char *)&SYSB(514));
          strcat(FileName, ((char *)D(0x854f5c)));
          v8 = strlen(Buffer) + 1;
          v9 = &FileName[strlen(FileName)];
          qmemcpy(v9, Buffer, 4 * (v8 >> 2));
          v11 = &Buffer[4 * (v8 >> 2)];
          v10 = &v9[4 * (v8 >> 2)];
          v12 = v8;
        }
        v14 = *(intptr_t *)&SYSB(792);
        qmemcpy(v10, v11, v12 & 3);
        if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(1u), (intptr_t)(v14), (intptr_t)(0x3F5u), (intptr_t)(FileName), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
          g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
        W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
        v15 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
        W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
        API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
        API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
        API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v15));
        g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 2);
        API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
        W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
        API_DeleteObject((intptr_t)(v15));
        RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
        if ( SYS32(0x2884) != 0 )
        {
          g_eng.h5bf3ee(Buffer, "1%07d.wma", 1003);
          if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
            && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
          {
            v21 = strlen(LibFileName) + 1;
            qmemcpy(FileName, LibFileName, 4 * (v21 >> 2));
            v19 = &LibFileName[4 * (v21 >> 2)];
            v18 = &FileName[4 * (v21 >> 2)];
            v20 = v21;
          }
          else
          {
            strcpy(FileName, (const char *)&SYSB(514));
            strcat(FileName, ((char *)D(0x854f5c)));
            v16 = strlen(Buffer) + 1;
            v17 = &FileName[strlen(FileName)];
            qmemcpy(v17, Buffer, 4 * (v16 >> 2));
            v19 = &Buffer[4 * (v16 >> 2)];
            v18 = &v17[4 * (v16 >> 2)];
            v20 = v16;
          }
          v22 = *(intptr_t *)&SYSB(792);
          qmemcpy(v18, v19, v20 & 3);
          if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x22u), (intptr_t)(v22), (intptr_t)(0x3EBu), (intptr_t)(FileName), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
            g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
          W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
          v23 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
          W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
          API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
          API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
          API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v23));
          g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 4);
          API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
          W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
          API_DeleteObject((intptr_t)(v23));
          RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
          if ( SYS32(0x2884) != 0 )
          {
            g_eng.h5bf3ee(Buffer, "1%07d.wma", 1005);
            if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
              && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
            {
              v29 = strlen(LibFileName) + 1;
              qmemcpy(FileName, LibFileName, 4 * (v29 >> 2));
              v27 = &LibFileName[4 * (v29 >> 2)];
              v26 = &FileName[4 * (v29 >> 2)];
              v28 = v29;
            }
            else
            {
              strcpy(FileName, (const char *)&SYSB(514));
              strcat(FileName, ((char *)D(0x854f5c)));
              v24 = strlen(Buffer) + 1;
              v25 = &FileName[strlen(FileName)];
              qmemcpy(v25, Buffer, 4 * (v24 >> 2));
              v27 = &Buffer[4 * (v24 >> 2)];
              v26 = &v25[4 * (v24 >> 2)];
              v28 = v24;
            }
            v30 = *(intptr_t *)&SYSB(792);
            qmemcpy(v26, v27, v28 & 3);
            if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(2u), (intptr_t)(v30), (intptr_t)(0x3EDu), (intptr_t)(FileName), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
              g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
            W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
            v31 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
            W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
            API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
            API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
            API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v31));
            g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 6);
            API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
            W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
            API_DeleteObject((intptr_t)(v31));
            RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
            if ( SYS32(0x2884) != 0 )
            {
              g_eng.h5bf3ee(Buffer, "1%07d.wma", 1024);
              if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
              {
                v37 = strlen(LibFileName) + 1;
                qmemcpy(FileName, LibFileName, 4 * (v37 >> 2));
                v35 = &LibFileName[4 * (v37 >> 2)];
                v34 = &FileName[4 * (v37 >> 2)];
                v36 = v37;
              }
              else
              {
                strcpy(FileName, (const char *)&SYSB(514));
                strcat(FileName, ((char *)D(0x854f5c)));
                v32 = strlen(Buffer) + 1;
                v33 = &FileName[strlen(FileName)];
                qmemcpy(v33, Buffer, 4 * (v32 >> 2));
                v35 = &Buffer[4 * (v32 >> 2)];
                v34 = &v33[4 * (v32 >> 2)];
                v36 = v32;
              }
              v38 = *(intptr_t *)&SYSB(792);
              qmemcpy(v34, v35, v36 & 3);
              if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(7u), (intptr_t)(v38), (intptr_t)(0x400u), (intptr_t)(FileName), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
                g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
              W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
              v39 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
              W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
              API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
              API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
              API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v39));
              g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 8);
              API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
              W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
              API_DeleteObject((intptr_t)(v39));
              RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
              if ( SYS32(0x2884) != 0 )
              {
                g_eng.h5bf3ee(Buffer, "1%07d.wma", 1022);
                if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                  && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                {
                  v45 = strlen(LibFileName) + 1;
                  qmemcpy(FileName, LibFileName, 4 * (v45 >> 2));
                  v43 = &LibFileName[4 * (v45 >> 2)];
                  v42 = &FileName[4 * (v45 >> 2)];
                  v44 = v45;
                }
                else
                {
                  strcpy(FileName, (const char *)&SYSB(514));
                  strcat(FileName, ((char *)D(0x854f5c)));
                  v40 = strlen(Buffer) + 1;
                  v41 = &FileName[strlen(FileName)];
                  qmemcpy(v41, Buffer, 4 * (v40 >> 2));
                  v43 = &Buffer[4 * (v40 >> 2)];
                  v42 = &v41[4 * (v40 >> 2)];
                  v44 = v40;
                }
                v46 = *(intptr_t *)&SYSB(792);
                qmemcpy(v42, v43, v44 & 3);
                if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0xCu), (intptr_t)(v46), (intptr_t)(0x3FEu), (intptr_t)(FileName), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                  g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                v47 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v47));
                g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 10);
                API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                API_DeleteObject((intptr_t)(v47));
                RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                if ( SYS32(0x2884) != 0 )
                {
                  g_eng.h5bf3ee(Buffer, "1%07d.wma", 1021);
                  if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                    && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                  {
                    v53 = strlen(LibFileName) + 1;
                    qmemcpy(FileName, LibFileName, 4 * (v53 >> 2));
                    v51 = &LibFileName[4 * (v53 >> 2)];
                    v50 = &FileName[4 * (v53 >> 2)];
                    v52 = v53;
                  }
                  else
                  {
                    strcpy(FileName, (const char *)&SYSB(514));
                    strcat(FileName, ((char *)D(0x854f5c)));
                    v48 = strlen(Buffer) + 1;
                    v49 = &FileName[strlen(FileName)];
                    qmemcpy(v49, Buffer, 4 * (v48 >> 2));
                    v51 = &Buffer[4 * (v48 >> 2)];
                    v50 = &v49[4 * (v48 >> 2)];
                    v52 = v48;
                  }
                  v54 = *(intptr_t *)&SYSB(792);
                  qmemcpy(v50, v51, v52 & 3);
                  if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x21u), (intptr_t)(v54), (intptr_t)(0x3FDu), (intptr_t)(FileName), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-500)) == 0 )
                    g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                  W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                  v55 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                  W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                  API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                  API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                  API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v55));
                  g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 12);
                  API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                  W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                  API_DeleteObject((intptr_t)(v55));
                  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                  if ( SYS32(0x2884) != 0 )
                  {
                    g_eng.h5bf3ee(Buffer, "1%07d.wma", 1006);
                    if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                      && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                    {
                      strcpy(FileName, LibFileName);
                      v56 = FileName;
                    }
                    else
                    {
                      v56 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                    }
                    if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x2Du), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3EEu), (intptr_t)(v56), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-300)) == 0 )
                      g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                    W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                    v57 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                    W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                    API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                    API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v57));
                    g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 14);
                    API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                    W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                    API_DeleteObject((intptr_t)(v57));
                    RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                    if ( SYS32(0x2884) != 0 )
                    {
                      g_eng.h5bf3ee(Buffer, "1%07d.wma", 1043);
                      if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                        && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                      {
                        strcpy(FileName, LibFileName);
                        v58 = FileName;
                      }
                      else
                      {
                        v58 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                      }
                      if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x2Eu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x413u), (intptr_t)(v58), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-300)) == 0 )
                        g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                      W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                      v59 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                      W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                      API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                      API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v59));
                      g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 16);
                      API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                      W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                      API_DeleteObject((intptr_t)(v59));
                      RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                      if ( SYS32(0x2884) != 0 )
                      {
                        g_eng.h5bf3ee(Buffer, "1%07d.wma", 1019);
                        if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                          && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                        {
                          strcpy(FileName, LibFileName);
                          v60 = FileName;
                        }
                        else
                        {
                          v60 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                        }
                        if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x2Bu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3FBu), (intptr_t)(v60), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
                          g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                        W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                        v61 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                        W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                        API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                        API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                        API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v61));
                        g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 18);
                        API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                        W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                        API_DeleteObject((intptr_t)(v61));
                        RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                        if ( SYS32(0x2884) != 0 )
                        {
                          g_eng.h5bf3ee(Buffer, "1%07d.wma", 1007);
                          if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                            && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                          {
                            strcpy(FileName, LibFileName);
                            v62 = FileName;
                          }
                          else
                          {
                            v62 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                          }
                          if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0xDu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3EFu), (intptr_t)(v62), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
                            g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                          W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                          v63 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                          W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                          API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                          API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                          API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v63));
                          g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 20);
                          API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                          W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                          API_DeleteObject((intptr_t)(v63));
                          RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                          if ( SYS32(0x2884) != 0 )
                          {
                            g_eng.h5bf3ee(Buffer, "1%07d.wma", 1009);
                            if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                              && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                            {
                              strcpy(FileName, LibFileName);
                              v64 = FileName;
                            }
                            else
                            {
                              v64 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                            }
                            if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0xEu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F1u), (intptr_t)(v64), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
                              g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                            W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                            v65 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                            W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                            API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                            API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                            API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v65));
                            g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 22);
                            API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                            W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                            API_DeleteObject((intptr_t)(v65));
                            RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                            if ( SYS32(0x2884) != 0 )
                            {
                              g_eng.h5bf3ee(Buffer, "1%07d.wma", 1010);
                              if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                              {
                                strcpy(FileName, LibFileName);
                                v66 = FileName;
                              }
                              else
                              {
                                v66 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                              }
                              if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0xFu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F2u), (intptr_t)(v66), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-200)) == 0 )
                                g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                              W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                              v67 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                              W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                              API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                              API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                              API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v67));
                              g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 24);
                              API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                              W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                              API_DeleteObject((intptr_t)(v67));
                              RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                              if ( SYS32(0x2884) != 0 )
                              {
                                g_eng.h5bf3ee(Buffer, "1%07d.wma", 1011);
                                if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                  && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                {
                                  strcpy(FileName, LibFileName);
                                  v68 = FileName;
                                }
                                else
                                {
                                  v68 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                }
                                if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x10u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F3u), (intptr_t)(v68), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
                                  g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                                W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                                v69 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v69));
                                g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 26);
                                API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                API_DeleteObject((intptr_t)(v69));
                                RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                if ( SYS32(0x2884) != 0 )
                                {
                                  g_eng.h5bf3ee(Buffer, "1%07d.wma", 1012);
                                  if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                    && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                  {
                                    strcpy(FileName, LibFileName);
                                    v70 = FileName;
                                  }
                                  else
                                  {
                                    v70 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                  }
                                  if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x11u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F4u), (intptr_t)(v70), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-1000)) == 0 )
                                    g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                                  W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                                  v71 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                  W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                  API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                  API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                  API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v71));
                                  g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 28);
                                  API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                  W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                  API_DeleteObject((intptr_t)(v71));
                                  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                  if ( SYS32(0x2884) != 0 )
                                  {
                                    g_eng.h5bf3ee(Buffer, "1%07d.wma", 1008);
                                    if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                      && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                    {
                                      strcpy(FileName, LibFileName);
                                      v72 = FileName;
                                    }
                                    else
                                    {
                                      v72 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                    }
                                    if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x17u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F0u), (intptr_t)(v72), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
                                      g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                                    W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                                    v73 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                    W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                    API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                    API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v73));
                                    g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 30);
                                    API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                    W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                    API_DeleteObject((intptr_t)(v73));
                                    RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                    if ( SYS32(0x2884) != 0 )
                                    {
                                      g_eng.h5bf3ee(Buffer, "1%07d.wma", 1014);
                                      if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                        && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                      {
                                        strcpy(FileName, LibFileName);
                                        v74 = FileName;
                                      }
                                      else
                                      {
                                        v74 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                      }
                                      if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Bu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F6u), (intptr_t)(v74), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                        g_eng.log_fail_406440((const char *)D(0x8a9530), (SYS32(0x2884) + 0x2c), D(0x8a9b6c));
                                      W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                            , 364)));
                                      v75 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                      W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                      API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                      API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v75));
                                      g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 32);
                                      API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                      W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                      API_DeleteObject((intptr_t)(v75));
                                      RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                      if ( SYS32(0x2884) != 0 )
                                      {
                                        g_eng.h5bf3ee(Buffer, "1%07d.wma", 1020);
                                        if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                          && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                        {
                                          strcpy(FileName, LibFileName);
                                          v76 = FileName;
                                        }
                                        else
                                        {
                                          v76 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                        }
                                        if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Du), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3FCu), (intptr_t)(v76), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                          g_eng.log_fail_406440(
                                            (const char *)D(0x8a9530),
                                            (SYS32(0x2884) + 0x2c),
                                            D(0x8a9b6c));
                                        W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                              , 364)));
                                        v77 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                        W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                        API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                        API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                        API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v77));
                                        g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 34);
                                        API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                        W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                        API_DeleteObject((intptr_t)(v77));
                                        RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                        if ( SYS32(0x2884) != 0 )
                                        {
                                          g_eng.h5bf3ee(Buffer, "1%07d.wma", 1018);
                                          if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                            && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                          {
                                            strcpy(FileName, LibFileName);
                                            v78 = FileName;
                                          }
                                          else
                                          {
                                            v78 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                          }
                                          if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x20u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3FAu), (intptr_t)(v78), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                            g_eng.log_fail_406440(
                                              (const char *)D(0x8a9530),
                                              (SYS32(0x2884) + 0x2c),
                                              D(0x8a9b6c));
                                          W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                , 364)));
                                          v79 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                          W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                          API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                          API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                          API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v79));
                                          g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 36);
                                          API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                          W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                          API_DeleteObject((intptr_t)(v79));
                                          RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                          if ( SYS32(0x2884) != 0 )
                                          {
                                            g_eng.h5bf3ee(Buffer, "1%07d.wma", 1047);
                                            if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                              && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                            {
                                              strcpy(FileName, LibFileName);
                                              v80 = FileName;
                                            }
                                            else
                                            {
                                              v80 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                            }
                                            if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Cu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x417u), (intptr_t)(v80), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                              g_eng.log_fail_406440(
                                                (const char *)D(0x8a9530),
                                                (SYS32(0x2884) + 0x2c),
                                                D(0x8a9b6c));
                                            W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                  , 364)));
                                            v81 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                            W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                            API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                            API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                            API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v81));
                                            g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 38);
                                            API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                            W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                            API_DeleteObject((intptr_t)(v81));
                                            RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                            if ( SYS32(0x2884) != 0 )
                                            {
                                              g_eng.h5bf3ee(Buffer, "1%07d.wma", 1016);
                                              if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                              {
                                                strcpy(FileName, LibFileName);
                                                v82 = FileName;
                                              }
                                              else
                                              {
                                                v82 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                              }
                                              if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Au), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F8u), (intptr_t)(v82), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-300)) == 0 )
                                                g_eng.log_fail_406440(
                                                  (const char *)D(0x8a9530),
                                                  (SYS32(0x2884) + 0x2c),
                                                  D(0x8a9b6c));
                                              W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                    , 364)));
                                              v83 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                              W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                              API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                              API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                              API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v83));
                                              g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 40);
                                              API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                              W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                              API_DeleteObject((intptr_t)(v83));
                                              RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                              if ( SYS32(0x2884) != 0 )
                                              {
                                                g_eng.h5bf3ee(Buffer, "1%07d.wma", 1017);
                                                if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                  && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                {
                                                  strcpy(FileName, LibFileName);
                                                  v84 = FileName;
                                                }
                                                else
                                                {
                                                  v84 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                                }
                                                if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x2Cu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F9u), (intptr_t)(v84), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(400)) == 0 )
                                                  g_eng.log_fail_406440(
                                                    (const char *)D(0x8a9530),
                                                    (SYS32(0x2884) + 0x2c),
                                                    D(0x8a9b6c));
                                                W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                      , 364)));
                                                v85 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v85));
                                                g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 42);
                                                API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                API_DeleteObject((intptr_t)(v85));
                                                RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                if ( SYS32(0x2884) != 0 )
                                                {
                                                  g_eng.h5bf3ee(Buffer, "1%07d.wma", 1015);
                                                  if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                    && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                  {
                                                    strcpy(FileName, LibFileName);
                                                    v86 = FileName;
                                                  }
                                                  else
                                                  {
                                                    v86 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                                  }
                                                  if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Eu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3F7u), (intptr_t)(v86), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                                    g_eng.log_fail_406440(
                                                      (const char *)D(0x8a9530),
                                                      (SYS32(0x2884) + 0x2c),
                                                      D(0x8a9b6c));
                                                  W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                        , 364)));
                                                  v87 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                  W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                  API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                  API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                  API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v87));
                                                  g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 44);
                                                  API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                  W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                  API_DeleteObject((intptr_t)(v87));
                                                  RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                  if ( SYS32(0x2884) != 0 )
                                                  {
                                                    g_eng.h5bf3ee(Buffer, "1%07d.wma", 1004);
                                                    if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                      && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                    {
                                                      strcpy(FileName, LibFileName);
                                                      v88 = FileName;
                                                    }
                                                    else
                                                    {
                                                      v88 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                                    }
                                                    if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x1Fu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3ECu), (intptr_t)(v88), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-500)) == 0 )
                                                      g_eng.log_fail_406440(
                                                        (const char *)D(0x8a9530),
                                                        (SYS32(0x2884) + 0x2c),
                                                        D(0x8a9b6c));
                                                    W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                          , 364)));
                                                    v89 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                    W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                    API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                    API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                    API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v89));
                                                    g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 46);
                                                    API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                    W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                    API_DeleteObject((intptr_t)(v89));
                                                    RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                    if ( SYS32(0x2884) != 0 )
                                                    {
                                                      g_eng.h5bf3ee(Buffer, "1%07d.wma", 1002);
                                                      if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                        && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                      {
                                                        strcpy(FileName, LibFileName);
                                                        v90 = FileName;
                                                      }
                                                      else
                                                      {
                                                        v90 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                                      }
                                                      if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(5u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3EAu), (intptr_t)(v90), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                                        g_eng.log_fail_406440(
                                                          (const char *)D(0x8a9530),
                                                          (SYS32(0x2884) + 0x2c),
                                                          D(0x8a9b6c));
                                                      W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                            , 364)));
                                                      v91 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                      W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                      API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                      API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                      API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v91));
                                                      g_eng.h5bf3ee(LibFileName, "Now Loading % 3d%%", 48);
                                                      API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                      W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                      API_DeleteObject((intptr_t)(v91));
                                                      RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                      if ( SYS32(0x2884) != 0 )
                                                      {
                                                        g_eng.h5bf3ee(Buffer, "1%07d.wma", 1000);
                                                        if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                          && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                        {
                                                          strcpy(FileName, LibFileName);
                                                          v92 = FileName;
                                                        }
                                                        else
                                                        {
                                                          v92 = al_579860((const char *)((intptr_t)g_eng.sys), Buffer, FileName);
                                                        }
                                                        if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x28u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3E8u), (intptr_t)(v92), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                                          g_eng.log_fail_406440(
                                                            (const char *)D(0x8a9530),
                                                            (SYS32(0x2884) + 0x2c),
                                                            D(0x8a9b6c));
                                                        W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                              , 364)));
                                                        v93 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                        W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                        API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                        API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                        API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v93));
                                                        g_eng.h5bf3ee(
                                                          LibFileName,
                                                          "Now Loading % 3d%%",
                                                          50);
                                                        API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                        W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                        API_DeleteObject((intptr_t)(v93));
                                                        RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                        if ( SYS32(0x2884) != 0 )
                                                        {
                                                          g_eng.h5bf3ee(Buffer, "1%07d.wma", 1001);
                                                          if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                            && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                          {
                                                            strcpy(FileName, LibFileName);
                                                            v94 = FileName;
                                                          }
                                                          else
                                                          {
                                                            v94 = al_579860(
                                                                    (const char *)((intptr_t)g_eng.sys),
                                                                    Buffer,
                                                                    FileName);
                                                          }
                                                          if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x29u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x3E9u), (intptr_t)(v94), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                                            g_eng.log_fail_406440(
                                                              (const char *)D(0x8a9530),
                                                              (SYS32(0x2884) + 0x2c),
                                                              D(0x8a9b6c));
                                                          W_zd_5b9410((intptr_t)(F32(SYS32(10348)
                                                                                                , 364)));
                                                          v95 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                          W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                          API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                          API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                          API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v95));
                                                          g_eng.h5bf3ee(
                                                            LibFileName,
                                                            "Now Loading % 3d%%",
                                                            52);
                                                          API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                          W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                          API_DeleteObject((intptr_t)(v95));
                                                          RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                          if ( SYS32(0x2884) != 0 )
                                                          {
                                                            g_eng.h5bf3ee(Buffer, "1%07d.wma", 1029);
                                                            if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                              && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                            {
                                                              strcpy(FileName, LibFileName);
                                                              v96 = FileName;
                                                            }
                                                            else
                                                            {
                                                              v96 = al_579860(
                                                                      (const char *)((intptr_t)g_eng.sys),
                                                                      Buffer,
                                                                      FileName);
                                                            }
                                                            if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x12u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x405u), (intptr_t)(v96), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                                              g_eng.log_fail_406440(
                                                                (const char *)D(0x8a9530),
                                                                (SYS32(0x2884) + 0x2c),
                                                                D(0x8a9b6c));
                                                            W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                                                            v97 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                            W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348)
                                                                            + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                            API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                            API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                            API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v97));
                                                            g_eng.h5bf3ee(
                                                              LibFileName,
                                                              "Now Loading % 3d%%",
                                                              54);
                                                            API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                            W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348)
                                                                            + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                            API_DeleteObject((intptr_t)(v97));
                                                            RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                            if ( SYS32(0x2884) != 0 )
                                                            {
                                                              g_eng.h5bf3ee(Buffer, "1%07d.wma", 1030);
                                                              if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                                && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                              {
                                                                strcpy(FileName, LibFileName);
                                                                v98 = FileName;
                                                              }
                                                              else
                                                              {
                                                                v98 = al_579860(
                                                                        (const char *)((intptr_t)g_eng.sys),
                                                                        Buffer,
                                                                        FileName);
                                                              }
                                                              if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x13u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x406u), (intptr_t)(v98), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-500)) == 0 )
                                                                g_eng.log_fail_406440(
                                                                  (const char *)D(0x8a9530),
                                                                  (SYS32(0x2884) + 0x2c),
                                                                  D(0x8a9b6c));
                                                              W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                                                              v99 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                              W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348)
                                                                              + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                              API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                              API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                              API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v99));
                                                              g_eng.h5bf3ee(
                                                                LibFileName,
                                                                "Now Loading % 3d%%",
                                                                56);
                                                              API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                              W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348)
                                                                              + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                              API_DeleteObject((intptr_t)(v99));
                                                              RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                              if ( SYS32(0x2884) != 0 )
                                                              {
                                                                g_eng.h5bf3ee(Buffer, "1%07d.wma", 1031);
                                                                if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                                  && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                                {
                                                                  strcpy(FileName, LibFileName);
                                                                  v100 = FileName;
                                                                }
                                                                else
                                                                {
                                                                  v100 = al_579860(
                                                                           (const char *)((intptr_t)g_eng.sys),
                                                                           Buffer,
                                                                           FileName);
                                                                }
                                                                if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x14u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x407u), (intptr_t)(v100), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-200)) == 0 )
                                                                  g_eng.log_fail_406440(
                                                                    (const char *)D(0x8a9530),
                                                                    (SYS32(0x2884) + 0x2c),
                                                                    D(0x8a9b6c));
                                                                W_zd_5b9410((intptr_t)(F32(SYS32(10348), 364)));
                                                                v101 = W_wm_5b5dc0((intptr_t)(6), (intptr_t)(12), (intptr_t)(0));
                                                                W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348)
                                                                                + 364)), (intptr_t)((intptr_t *)&(*hdc__f)));
                                                                API_SetTextColor((intptr_t)((*hdc__f)), (intptr_t)((uint32_t)D(0x808080)));
                                                                API_SetBkMode((intptr_t)((*hdc__f)), (intptr_t)(1));
                                                                API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)(v101));
                                                                g_eng.h5bf3ee(
                                                                  LibFileName,
                                                                  "Now Loading % 3d%%",
                                                                  58);
                                                                API_TextOutA((intptr_t)((*hdc__f)), (intptr_t)(320 - ((6 * strlen(LibFileName)) >> 1)), (intptr_t)(456), (intptr_t)(LibFileName), (intptr_t)(strlen(LibFileName)));
                                                                W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348)
                                                                                + 364)), (intptr_t)((intptr_t)(*hdc__f)));
                                                                API_DeleteObject((intptr_t)(v101));
                                                                RC_5b8fc0((intptr_t)(SYS32(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
                                                                if ( SYS32(0x2884) != 0 )
                                                                {
                                                                  g_eng.h5bf3ee(Buffer, "1%07d.wma", 1032);
                                                                  if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                                    && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                                  {
                                                                    strcpy(FileName, LibFileName);
                                                                    v102 = FileName;
                                                                  }
                                                                  else
                                                                  {
                                                                    v102 = al_579860(
                                                                             (const char *)((intptr_t)g_eng.sys),
                                                                             Buffer,
                                                                             FileName);
                                                                  }
                                                                  if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x15u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x408u), (intptr_t)(v102), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                                                    g_eng.log_fail_406440(
                                                                      (const char *)D(0x8a9530),
                                                                      (SYS32(0x2884) + 0x2c),
                                                                      D(0x8a9b6c));
                                                                  W_tm_56dc20((intptr_t)(1), (intptr_t)(30), (intptr_t)(50));
                                                                  if ( SYS32(0x2884) != 0 )
                                                                  {
                                                                    g_eng.h5bf3ee(
                                                                      Buffer,
                                                                      "1%07d.wma",
                                                                      1028);
                                                                    if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(TempFileName)) != 0
                                                                      && API_GetTempFileNameA((intptr_t)(TempFileName), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(LibFileName)) != 0 )
                                                                    {
                                                                      strcpy(FileName, LibFileName);
                                                                      v103 = FileName;
                                                                    }
                                                                    else
                                                                    {
                                                                      v103 = al_579860(
                                                                               (const char *)((intptr_t)g_eng.sys),
                                                                               Buffer,
                                                                               FileName);
                                                                    }
                                                                    if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x16u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x404u), (intptr_t)(v103), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
                                                                      g_eng.log_fail_406440(
                                                                        (const char *)D(0x8a9530),
                                                                        (SYS32(0x2884) + 0x2c),
                                                                        D(0x8a9b6c));
                                                                    W_tm_56dc20((intptr_t)(1), (intptr_t)(31), (intptr_t)(50));
                                                                    if ( SYS32(0x2884) != 0 )
                                                                    {
                                                                      g_eng.h5bf3ee(
                                                                        Buffer,
                                                                        "1%07d.wma",
                                                                        1034);
                                                                      v104 = al_5798f0(
                                                                               ((intptr_t)g_eng.sys),
                                                                               Buffer,
                                                                               FileName);
                                                                      if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(8u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x40Au), (intptr_t)(v104), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                                                        g_eng.log_fail_406440(
                                                                          (const char *)D(0x8a9530),
                                                                          (SYS32(0x2884) + 0x2c),
                                                                          D(0x8a9b6c));
                                                                      W_tm_56dc20((intptr_t)(1), (intptr_t)(32), (intptr_t)(50));
                                                                      if ( SYS32(0x2884) != 0 )
                                                                      {
                                                                        g_eng.h5bf3ee(
                                                                          Buffer,
                                                                          "1%07d.wma",
                                                                          1033);
                                                                        v105 = al_5798f0(
                                                                                 ((intptr_t)g_eng.sys),
                                                                                 Buffer,
                                                                                 FileName);
                                                                        if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(9u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x409u), (intptr_t)(v105), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(400)) == 0 )
                                                                          g_eng.h408b90(
                                                                            D(0x8a9530),
                                                                            (SYS32(0x2884) + 0x2c),
                                                                            0,
                                                                            D(0x8a9b6c));
                                                                        W_tm_56dc20((intptr_t)(1), (intptr_t)(33), (intptr_t)(50));
                                                                        if ( SYS32(0x2884) != 0 )
                                                                        {
                                                                          g_eng.h5bf3ee(
                                                                            Buffer,
                                                                            "1%07d.wma",
                                                                            1035);
                                                                          v106 = al_5798f0(
                                                                                   ((intptr_t)g_eng.sys),
                                                                                   Buffer,
                                                                                   FileName);
                                                                          if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x24u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x40Bu), (intptr_t)(v106), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                                                            g_eng.h408b90(
                                                                              D(0x8a9530),
                                                                              (SYS32(0x2884) + 0x2c),
                                                                              0,
                                                                              D(0x8a9b6c));
                                                                          W_tm_56dc20((intptr_t)(1), (intptr_t)(34), (intptr_t)(50));
                                                                          if ( SYS32(0x2884) != 0 )
                                                                          {
                                                                            g_eng.h5bf3ee(
                                                                              Buffer,
                                                                              "1%07d.wma",
                                                                              1037);
                                                                            v107 = al_5798f0(
                                                                                     ((intptr_t)g_eng.sys),
                                                                                     Buffer,
                                                                                     FileName);
                                                                            if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x25u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x40Du), (intptr_t)(v107), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-400)) == 0 )
                                                                              g_eng.h408b90(
                                                                                D(0x8a9530),
                                                                                (SYS32(0x2884) + 0x2c),
                                                                                0,
                                                                                D(0x8a9b6c));
                                                                            W_tm_56dc20((intptr_t)(1), (intptr_t)(35), (intptr_t)(50));
                                                                            if ( SYS32(0x2884) != 0 )
                                                                            {
                                                                              g_eng.h5bf3ee(
                                                                                Buffer,
                                                                                "1%07d.wma",
                                                                                1038);
                                                                              v108 = al_5798f0(
                                                                                       ((intptr_t)g_eng.sys),
                                                                                       Buffer,
                                                                                       FileName);
                                                                              if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x26u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x40Eu), (intptr_t)(v108), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(400)) == 0 )
                                                                                g_eng.h408b90(
                                                                                  D(0x8a9530),
                                                                                  (SYS32(0x2884) + 0x2c),
                                                                                  0,
                                                                                  D(0x8a9b6c));
                                                                              W_tm_56dc20((intptr_t)(1), (intptr_t)(36), (intptr_t)(50));
                                                                              if ( SYS32(0x2884) != 0 )
                                                                              {
                                                                                g_eng.h5bf3ee(
                                                                                  Buffer,
                                                                                  "1%07d.wma",
                                                                                  1026);
                                                                                v109 = al_5798f0(
                                                                                         ((intptr_t)g_eng.sys),
                                                                                         Buffer,
                                                                                         FileName);
                                                                                if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x27u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x402u), (intptr_t)(v109), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(-200)) == 0 )
                                                                                  g_eng.h408b90(
                                                                                    D(0x8a9530),
                                                                                    (SYS32(0x2884) + 0x2c),
                                                                                    0,
                                                                                    D(0x8a9b6c));
                                                                                W_tm_56dc20((intptr_t)(1), (intptr_t)(37), (intptr_t)(50));
                                                                                if ( SYS32(0x2884) != 0 )
                                                                                {
                                                                                  g_eng.h5bf3ee(
                                                                                    Buffer,
                                                                                    "1%07d.wma",
                                                                                    1025);
                                                                                  v110 = al_5798f0(
                                                                                           ((intptr_t)g_eng.sys),
                                                                                           Buffer,
                                                                                           FileName);
                                                                                  if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x23u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x401u), (intptr_t)(v110), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(600)) == 0 )
                                                                                    g_eng.h408b90(
                                                                                      D(0x8a9530),
                                                                                      (SYS32(0x2884) + 0x2c),
                                                                                      0,
                                                                                      D(0x8a9b6c));
                                                                                  W_tm_56dc20((intptr_t)(1), (intptr_t)(38), (intptr_t)(50));
                                                                                  if ( SYS32(0x2884) != 0 )
                                                                                  {
                                                                                    g_eng.h5bf3ee(
                                                                                      Buffer,
                                                                                      "1%07d.wma",
                                                                                      1027);
                                                                                    v111 = al_5798f0(
                                                                                             ((intptr_t)g_eng.sys),
                                                                                             Buffer,
                                                                                             FileName);
                                                                                    if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(3u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x403u), (intptr_t)(v111), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                                                                      g_eng.h408b90(
                                                                                        D(0x8a9530),
                                                                                        (SYS32(0x2884) + 0x2c),
                                                                                        0,
                                                                                        D(0x8a9b6c));
                                                                                    W_tm_56dc20((intptr_t)(1), (intptr_t)(39), (intptr_t)(50));
                                                                                    if ( SYS32(0x2884) != 0 )
                                                                                    {
                                                                                      g_eng.h5bf3ee(
                                                                                        Buffer,
                                                                                        "1%07d.wma",
                                                                                        1041);
                                                                                      v112 = al_5798f0(
                                                                                               ((intptr_t)g_eng.sys),
                                                                                               Buffer,
                                                                                               FileName);
                                                                                      if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x18u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x411u), (intptr_t)(v112), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                                                                        g_eng.h408b90(
                                                                                          D(0x8a9530),
                                                                                          (SYS32(0x2884) + 0x2c),
                                                                                          0,
                                                                                          D(0x8a9b6c));
                                                                                      W_tm_56dc20((intptr_t)(1), (intptr_t)(40), (intptr_t)(50));
                                                                                      if ( SYS32(0x2884) != 0 )
                                                                                      {
                                                                                        g_eng.h5bf3ee(
                                                                                          Buffer,
                                                                                          "1%07d.wma",
                                                                                          1044);
                                                                                        v113 = al_5798f0(
                                                                                                 ((intptr_t)g_eng.sys),
                                                                                                 Buffer,
                                                                                                 FileName);
                                                                                        if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x19u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x414u), (intptr_t)(v113), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(400)) == 0 )
                                                                                          g_eng.h408b90(
                                                                                            D(0x8a9530),
                                                                                            (SYS32(0x2884) + 0x2c),
                                                                                            0,
                                                                                            D(0x8a9b6c));
                                                                                        W_tm_56dc20((intptr_t)(1), (intptr_t)(41), (intptr_t)(50));
                                                                                        if ( SYS32(0x2884) != 0 )
                                                                                        {
                                                                                          g_eng.h5bf3ee(
                                                                                            Buffer,
                                                                                            "1%07d.wma",
                                                                                            1040);
                                                                                          v114 = al_5798f0(
                                                                                                   ((intptr_t)g_eng.sys),
                                                                                                   Buffer,
                                                                                                   FileName);
                                                                                          if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(4u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x410u), (intptr_t)(v114), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                                                                            g_eng.h408b90(
                                                                                              D(0x8a9530),
                                                                                              (SYS32(0x2884) + 0x2c),
                                                                                              0,
                                                                                              D(0x8a9b6c));
                                                                                          W_tm_56dc20((intptr_t)(1), (intptr_t)(42), (intptr_t)(50));
                                                                                          if ( SYS32(0x2884) != 0 )
                                                                                          {
                                                                                            g_eng.h5bf3ee(
                                                                                              Buffer,
                                                                                              "1%07d.wma",
                                                                                              1042);
                                                                                            v115 = al_5798f0(
                                                                                                     ((intptr_t)g_eng.sys),
                                                                                                     Buffer,
                                                                                                     FileName);
                                                                                            if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(6u), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x412u), (intptr_t)(v115), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(400)) == 0 )
                                                                                              g_eng.h408b90(
                                                                                                D(0x8a9530),
                                                                                                (SYS32(0x2884) + 0x2c),
                                                                                                0,
                                                                                                D(0x8a9b6c));
                                                                                            W_tm_56dc20((intptr_t)(1), (intptr_t)(43), (intptr_t)(50));
                                                                                            if ( SYS32(0x2884) != 0 )
                                                                                            {
                                                                                              g_eng.h5bf3ee(
                                                                                                Buffer,
                                                                                                "1%07d.wma",
                                                                                                1036);
                                                                                              v116 = al_5798f0(((intptr_t)g_eng.sys), Buffer, FileName);
                                                                                              if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0x2Au), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x40Cu), (intptr_t)(v116), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(700)) == 0 )
                                                                                                g_eng.h408b90(
                                                                                                  D(0x8a9530),
                                                                                                  (SYS32(0x2884) + 0x2c),
                                                                                                  0,
                                                                                                  D(0x8a9b6c));
                                                                                              W_tm_56dc20((intptr_t)(1), (intptr_t)(44), (intptr_t)(50));
                                                                                              if ( SYS32(0x2884) != 0 )
                                                                                              {
                                                                                                g_eng.h5bf3ee(
                                                                                                  Buffer,
                                                                                                  "1%07d.wma",
                                                                                                  1045);
                                                                                                v117 = al_5798f0(((intptr_t)g_eng.sys), Buffer, FileName);
                                                                                                if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0xAu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x415u), (intptr_t)(v117), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                                                                                  g_eng.h408b90(
                                                                                                    D(0x8a9530),
                                                                                                    (SYS32(0x2884) + 0x2c),
                                                                                                    0,
                                                                                                    D(0x8a9b6c));
                                                                                                W_tm_56dc20((intptr_t)(1), (intptr_t)(45), (intptr_t)(50));
                                                                                                if ( SYS32(0x2884) != 0 )
                                                                                                {
                                                                                                  g_eng.h5bf3ee(
                                                                                                    Buffer,
                                                                                                    "1%07d.wma",
                                                                                                    1046);
                                                                                                  v118 = al_5798f0(((intptr_t)g_eng.sys), Buffer, FileName);
                                                                                                  if ( W_sd_5bc7c0((intptr_t)(SYS32(0x2884)), (intptr_t)(0xBu), (intptr_t)(*(intptr_t *)&SYSB(792)), (intptr_t)(0x416u), (intptr_t)(v118), (intptr_t)(D(0x8a2800)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(200)) == 0 )
                                                                                                    g_eng.h408b90(D(0x8a9530), (SYS32(0x2884) + 0x2c), 0, D(0x8a9b6c));
                                                                                                  W_tm_56dc20((intptr_t)(1), (intptr_t)(46), (intptr_t)(50));
                                                                                                }
                                                                                              }
                                                                                            }
                                                                                          }
                                                                                        }
                                                                                      }
                                                                                    }
                                                                                  }
                                                                                }
                                                                              }
                                                                            }
                                                                          }
                                                                        }
                                                                      }
                                                                    }
                                                                  }
                                                                }
                                                              }
                                                            }
                                                          }
                                                        }
                                                      }
                                                    }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      if ( SYS32(792) != 0 )
      {
        API_FreeLibrary((intptr_t)(*(intptr_t *)&SYSB(792)));
        SYS32(792) = 0;
      }
      W_tm_56dc20((intptr_t)(1), (intptr_t)(1), (intptr_t)(1));
    }
    else
    {
      g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a2814)), D(0x8a9b6c));
    }
  }
}

/* 0x57b280: unsigned int __stdcall sub_57B280(int a1, unsigned __int16 group, void *settings) */
int32_t al_57b280(int32_t a1, uint16_t group, void * settings)
{

    intptr_t v3 = 0;
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
    intptr_t v14 = 0;
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
    intptr_t v45 = 0;
    intptr_t v46 = 0;
    intptr_t v47 = 0;
    intptr_t v48 = 0;
    intptr_t v49 = 0;
    intptr_t v50 = 0;
    intptr_t v51 = 0;
    intptr_t v52 = 0;
    intptr_t v53 = 0;
    intptr_t v54 = 0;
    intptr_t v55 = 0;
    intptr_t v56 = 0;
    intptr_t v57 = 0;
    intptr_t v58 = 0;
    intptr_t v59 = 0;
    intptr_t v60 = 0;
    intptr_t v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    intptr_t v64 = 0;
    intptr_t v65 = 0;
    intptr_t v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    intptr_t v69 = 0;
    intptr_t v70 = 0;
    intptr_t v71 = 0;
    intptr_t v72 = 0;
    intptr_t v73 = 0;
    intptr_t v74 = 0;
    intptr_t v75 = 0;
    intptr_t v76 = 0;
    intptr_t v77 = 0;
    intptr_t v78 = 0;
    intptr_t v79 = 0;
    intptr_t v80 = 0;
    intptr_t v81 = 0;
    intptr_t v82 = 0;
    intptr_t v83 = 0;
    intptr_t v84 = 0;
    intptr_t v85 = 0;
    intptr_t v86 = 0;
    intptr_t v87 = 0;
    intptr_t v88 = 0;
    intptr_t v89 = 0;
    intptr_t v90 = 0;
    intptr_t v91 = 0;
    intptr_t v92 = 0;
    intptr_t v93 = 0;
    intptr_t v94 = 0;
    intptr_t v95 = 0;
    intptr_t v96 = 0;
    intptr_t v97 = 0;
    intptr_t v98 = 0;
    intptr_t v99 = 0;
    intptr_t v100 = 0;
    intptr_t v101 = 0;
    intptr_t v102 = 0;
    intptr_t v103 = 0;
    intptr_t v104 = 0;
    intptr_t v105 = 0;
    intptr_t v106 = 0;
    intptr_t v107 = 0;
    intptr_t v108 = 0;
    intptr_t v109 = 0;
    intptr_t v110 = 0;
    intptr_t v111 = 0;
    intptr_t v112 = 0;
    intptr_t v113 = 0;
    intptr_t v114 = 0;
    intptr_t v115 = 0;
    intptr_t v116 = 0;
    intptr_t v117 = 0;
    intptr_t v118 = 0;
    intptr_t v119 = 0;
    intptr_t v120 = 0;
    intptr_t v121 = 0;
    intptr_t v122 = 0;
    intptr_t v123 = 0;
    intptr_t v124 = 0;
    intptr_t v125 = 0;
    intptr_t v126 = 0;
    uint32_t result = 0;
    uint16_t v128 = 0;
    void * v129 = 0;
    int16_t v130 = 0;
    _Alignas(8) uint8_t frame_[0x20 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint16_t *const v131__f = (uint16_t *)(frame_ + 0x0);   /* ebp-0x20 */
    uint16_t *const v132__f = (uint16_t *)(frame_ + 0x4);   /* ebp-0x1c */
    uint16_t *const v133__f = (uint16_t *)(frame_ + 0x8);   /* ebp-0x18 */

  v3 = SYS32(924);
  F32(SYS32(924), 8) = SYS32(10356);
  F16(v3, 0) = 8;
  F16(v3, 2) = 0;
  F16(v3, 20) = group;
  F16(v3, 12) = 1299;
  F32(v3, 16) = settings;
  v4 = SYS32(928);
  F32(SYS32(928), 8) = SYS32(10356);
  F16(v4, 0) = 8;
  F16(v4, 2) = 0;
  F16(v4, 20) = group;
  F16(v4, 12) = 1218;
  F32(v4, 16) = settings;
  v5 = SYS32(940);
  F32(SYS32(940), 8) = SYS32(10356);
  F16(v5, 0) = 8;
  F16(v5, 2) = 0;
  F16(v5, 20) = group;
  F16(v5, 12) = 1221;
  F32(v5, 16) = settings;
  v6 = SYS32(936);
  F32(SYS32(936), 8) = SYS32(10356);
  F16(v6, 0) = 4;
  F16(v6, 2) = 0;
  F16(v6, 20) = group;
  F16(v6, 12) = 1220;
  F32(v6, 16) = settings;
  v7 = SYS32(932);
  F32(SYS32(932), 8) = SYS32(10356);
  F16(v7, 0) = 8;
  F16(v7, 2) = 0;
  F16(v7, 20) = group;
  F16(v7, 12) = 1217;
  F32(v7, 16) = settings;
  v8 = SYS32(944);
  F32(SYS32(944), 8) = SYS32(10356);
  F16(v8, 0) = 4;
  F16(v8, 2) = 0;
  F16(v8, 20) = group;
  F16(v8, 12) = 1223;
  F32(v8, 16) = settings;
  v9 = SYS32(948);
  F32(SYS32(948), 8) = SYS32(10356);
  F16(v9, 0) = 4;
  F16(v9, 2) = 0;
  F16(v9, 20) = group;
  F16(v9, 12) = 1219;
  F32(v9, 16) = settings;
  v10 = SYS32(952);
  F32(SYS32(952), 8) = SYS32(10356);
  F16(v10, 0) = 8;
  F16(v10, 2) = 0;
  F16(v10, 20) = group;
  F16(v10, 12) = 1222;
  F32(v10, 16) = settings;
  v11 = SYS32(1008);
  F32(SYS32(1008), 8) = SYS32(10356);
  F16(v11, 0) = 2;
  F16(v11, 2) = 0;
  F16(v11, 20) = group;
  F16(v11, 12) = 2306;
  F32(v11, 16) = settings;
  v12 = SYS32(1012);
  F32(SYS32(1012), 8) = SYS32(10356);
  F16(v12, 0) = 2;
  F16(v12, 2) = 0;
  F16(v12, 20) = group;
  F16(v12, 12) = 2311;
  F32(v12, 16) = settings;
  v13 = SYS32(972);
  F32(SYS32(972), 8) = SYS32(10356);
  F16(v13, 0) = 8;
  F16(v13, 2) = 0;
  F16(v13, 20) = group;
  F16(v13, 12) = 1215;
  F32(v13, 16) = settings;
  v14 = SYS32(976);
  F32(SYS32(976), 8) = SYS32(10356);
  F16(v14, 0) = 4;
  F16(v14, 2) = 0;
  F16(v14, 20) = group;
  F16(v14, 12) = 1216;
  F32(v14, 16) = settings;
  v15 = SYS32(980);
  F32(SYS32(980), 8) = SYS32(10356);
  F16(v15, 0) = 8;
  F16(v15, 2) = 0;
  F16(v15, 20) = group;
  F16(v15, 12) = 1210;
  F32(v15, 16) = settings;
  v16 = SYS32(984);
  F32(SYS32(984), 8) = SYS32(10356);
  F16(v16, 0) = 4;
  F16(v16, 2) = 0;
  F16(v16, 20) = group;
  F16(v16, 12) = 1646;
  F32(v16, 16) = settings;
  v17 = SYS32(988);
  F32(SYS32(988), 8) = SYS32(10356);
  F16(v17, 0) = 2;
  F16(v17, 2) = 0;
  F16(v17, 20) = group;
  F16(v17, 12) = 1696;
  F32(v17, 16) = settings;
  v18 = SYS32(992);
  F32(SYS32(992), 8) = SYS32(10356);
  F16(v18, 0) = 2;
  F16(v18, 2) = 0;
  F16(v18, 20) = group;
  F16(v18, 12) = 1923;
  F32(v18, 16) = settings;
  v19 = SYS32(996);
  F32(SYS32(996), 8) = SYS32(10356);
  F16(v19, 0) = 2;
  F16(v19, 2) = 0;
  F16(v19, 20) = group;
  F16(v19, 12) = 1716;
  F32(v19, 16) = settings;
  v20 = SYS32(1000);
  F32(SYS32(1000), 8) = SYS32(10356);
  F16(v20, 0) = 2;
  F16(v20, 2) = 0;
  F16(v20, 20) = group;
  F16(v20, 12) = 1904;
  F32(v20, 16) = settings;
  v21 = SYS32(1004);
  F32(SYS32(1004), 8) = SYS32(10356);
  F16(v21, 0) = 2;
  F16(v21, 2) = 0;
  F16(v21, 20) = group;
  F16(v21, 12) = 1958;
  F32(v21, 16) = settings;
  v22 = SYS32(1376);
  F32(SYS32(1376), 8) = SYS32(10356);
  F16(v22, 0) = 4;
  F16(v22, 2) = 0;
  F16(v22, 20) = group;
  F16(v22, 12) = 2245;
  F32(v22, 16) = settings;
  v23 = SYS32(1016);
  F32(SYS32(1016), 8) = SYS32(10356);
  F16(v23, 0) = 4;
  F16(v23, 2) = 0;
  F16(v23, 20) = group;
  F16(v23, 12) = 1209;
  F32(v23, 16) = settings;
  v24 = SYS32(1020);
  F32(SYS32(1020), 8) = SYS32(10356);
  F16(v24, 0) = 4;
  F16(v24, 2) = 0;
  F16(v24, 20) = group;
  F16(v24, 12) = 1211;
  F32(v24, 16) = settings;
  v25 = SYS32(1032);
  F32(SYS32(1032), 8) = SYS32(10356);
  F16(v25, 0) = 8;
  F16(v25, 2) = 0;
  F16(v25, 20) = group;
  F16(v25, 12) = 2308;
  F32(v25, 16) = settings;
  v26 = SYS32(1024);
  F32(SYS32(1024), 8) = SYS32(10356);
  F16(v26, 0) = 2;
  F16(v26, 2) = 0;
  F16(v26, 20) = group;
  F16(v26, 12) = 1214;
  F32(v26, 16) = settings;
  v27 = SYS32(1028);
  F32(SYS32(1028), 8) = SYS32(10356);
  F16(v27, 0) = 2;
  F16(v27, 2) = 0;
  F16(v27, 20) = group;
  F16(v27, 12) = 1213;
  F32(v27, 16) = settings;
  v28 = SYS32(1036);
  F32(SYS32(1036), 8) = SYS32(10356);
  F16(v28, 0) = 2;
  F16(v28, 2) = 0;
  F16(v28, 20) = group;
  F16(v28, 12) = 1647;
  F32(v28, 16) = settings;
  v29 = SYS32(1040);
  F32(SYS32(1040), 8) = SYS32(10356);
  F16(v29, 0) = 2;
  F16(v29, 2) = 0;
  F16(v29, 20) = group;
  F16(v29, 12) = 1648;
  F32(v29, 16) = settings;
  v30 = SYS32(1044);
  F32(SYS32(1044), 8) = SYS32(10356);
  F16(v30, 0) = 2;
  F16(v30, 2) = 0;
  F16(v30, 20) = group;
  F16(v30, 12) = 1649;
  F32(v30, 16) = settings;
  v31 = SYS32(1048);
  F32(SYS32(1048), 8) = SYS32(10356);
  F16(v31, 0) = 2;
  F16(v31, 2) = 0;
  F16(v31, 20) = group;
  F16(v31, 12) = 1650;
  F32(v31, 16) = settings;
  v32 = SYS32(1052);
  F32(SYS32(1052), 8) = SYS32(10356);
  F16(v32, 0) = 2;
  F16(v32, 2) = 0;
  F16(v32, 20) = group;
  F16(v32, 12) = 1651;
  F32(v32, 16) = settings;
  v33 = SYS32(1056);
  F32(SYS32(1056), 8) = SYS32(10356);
  F16(v33, 0) = 2;
  F16(v33, 2) = 0;
  F16(v33, 20) = group;
  F16(v33, 12) = 1652;
  F32(v33, 16) = settings;
  v34 = SYS32(1060);
  F32(SYS32(1060), 8) = SYS32(10356);
  F16(v34, 0) = 2;
  F16(v34, 2) = 0;
  F16(v34, 20) = group;
  F16(v34, 12) = 1653;
  F32(v34, 16) = settings;
  v35 = SYS32(1064);
  F32(SYS32(1064), 8) = SYS32(10356);
  F16(v35, 0) = 2;
  F16(v35, 2) = 0;
  F16(v35, 20) = group;
  F16(v35, 12) = 1654;
  F32(v35, 16) = settings;
  v36 = SYS32(1068);
  F32(SYS32(1068), 8) = SYS32(10356);
  F16(v36, 0) = 4;
  F16(v36, 2) = 0;
  F16(v36, 20) = group;
  F16(v36, 12) = 1905;
  F32(v36, 16) = settings;
  v37 = SYS32(1072);
  F32(SYS32(1072), 8) = SYS32(10356);
  F16(v37, 0) = 4;
  F16(v37, 2) = 0;
  F16(v37, 20) = group;
  F16(v37, 12) = 1906;
  F32(v37, 16) = settings;
  v38 = SYS32(1076);
  F32(SYS32(1076), 8) = SYS32(10356);
  F16(v38, 0) = 4;
  F16(v38, 2) = 0;
  F16(v38, 20) = group;
  F16(v38, 12) = 1907;
  F32(v38, 16) = settings;
  v39 = SYS32(1080);
  F32(SYS32(1080), 8) = SYS32(10356);
  F16(v39, 0) = 4;
  F16(v39, 2) = 0;
  F16(v39, 20) = group;
  F16(v39, 12) = 1908;
  F32(v39, 16) = settings;
  v40 = SYS32(1084);
  F32(SYS32(1084), 8) = SYS32(10356);
  F16(v40, 0) = 2;
  F16(v40, 2) = 0;
  F16(v40, 20) = group;
  F16(v40, 12) = 1655;
  F32(v40, 16) = settings;
  v41 = SYS32(1088);
  F32(SYS32(1088), 8) = SYS32(10356);
  F16(v41, 0) = 2;
  F16(v41, 2) = 0;
  F16(v41, 20) = group;
  F16(v41, 12) = 1656;
  F32(v41, 16) = settings;
  v42 = SYS32(1092);
  F32(SYS32(1092), 8) = SYS32(10356);
  F16(v42, 0) = 2;
  F16(v42, 2) = 0;
  F16(v42, 20) = group;
  F16(v42, 12) = 1660;
  F32(v42, 16) = settings;
  v43 = SYS32(1096);
  F32(SYS32(1096), 8) = SYS32(10356);
  F16(v43, 0) = 2;
  F16(v43, 2) = 0;
  F16(v43, 20) = group;
  F16(v43, 12) = 1658;
  F32(v43, 16) = settings;
  v44 = SYS32(1100);
  F32(SYS32(1100), 8) = SYS32(10356);
  F16(v44, 0) = 2;
  F16(v44, 2) = 0;
  F16(v44, 20) = group;
  F16(v44, 12) = 1659;
  F32(v44, 16) = settings;
  v45 = SYS32(1104);
  F32(SYS32(1104), 8) = SYS32(10356);
  F16(v45, 0) = 2;
  F16(v45, 2) = 0;
  F16(v45, 20) = group;
  F16(v45, 12) = 1741;
  F32(v45, 16) = settings;
  v46 = SYS32(1108);
  F32(SYS32(1108), 8) = SYS32(10356);
  F16(v46, 0) = 2;
  F16(v46, 2) = 0;
  F16(v46, 20) = group;
  F16(v46, 12) = 1742;
  F32(v46, 16) = settings;
  v47 = SYS32(1112);
  F32(SYS32(1112), 8) = SYS32(10356);
  F16(v47, 0) = 2;
  F16(v47, 2) = 0;
  F16(v47, 20) = group;
  F16(v47, 12) = 1743;
  F32(v47, 16) = settings;
  v48 = SYS32(1116);
  F32(SYS32(1116), 8) = SYS32(10356);
  F16(v48, 0) = 2;
  F16(v48, 2) = 0;
  F16(v48, 20) = group;
  F16(v48, 12) = 1744;
  F32(v48, 16) = settings;
  v49 = SYS32(1120);
  F32(SYS32(1120), 8) = SYS32(10356);
  F16(v49, 0) = 2;
  F16(v49, 2) = 0;
  F16(v49, 20) = group;
  F16(v49, 12) = 1745;
  F32(v49, 16) = settings;
  v50 = SYS32(1124);
  F32(SYS32(1124), 8) = SYS32(10356);
  F16(v50, 0) = 4;
  F16(v50, 2) = 0;
  F16(v50, 20) = group;
  F16(v50, 12) = 1947;
  F32(v50, 16) = settings;
  v51 = SYS32(1128);
  F32(SYS32(1128), 8) = SYS32(10356);
  F16(v51, 0) = 4;
  F16(v51, 2) = 0;
  F16(v51, 20) = group;
  F16(v51, 12) = 1948;
  F32(v51, 16) = settings;
  v52 = SYS32(1132);
  F32(SYS32(1132), 8) = SYS32(10356);
  F16(v52, 0) = 2;
  F16(v52, 2) = 0;
  F16(v52, 20) = group;
  F16(v52, 12) = 1285;
  F32(v52, 16) = settings;
  v53 = SYS32(1136);
  F32(SYS32(1136), 8) = SYS32(10356);
  F16(v53, 0) = 2;
  F16(v53, 2) = 0;
  F16(v53, 20) = group;
  F16(v53, 12) = 1281;
  F32(v53, 16) = settings;
  v54 = SYS32(1140);
  F32(SYS32(1140), 8) = SYS32(10356);
  F16(v54, 0) = 2;
  F16(v54, 2) = 0;
  F16(v54, 20) = group;
  F16(v54, 12) = 1279;
  F32(v54, 16) = settings;
  v55 = SYS32(1144);
  F32(SYS32(1144), 8) = SYS32(10356);
  F16(v55, 0) = 2;
  F16(v55, 2) = 0;
  F16(v55, 20) = group;
  F16(v55, 12) = 1280;
  F32(v55, 16) = settings;
  v56 = SYS32(1148);
  F32(SYS32(1148), 8) = SYS32(10356);
  F16(v56, 0) = 2;
  F16(v56, 2) = 0;
  F16(v56, 20) = group;
  F16(v56, 12) = 1327;
  F32(v56, 16) = settings;
  v57 = SYS32(1152);
  F32(SYS32(1152), 8) = SYS32(10356);
  F16(v57, 0) = 2;
  F16(v57, 2) = 0;
  F16(v57, 20) = group;
  F16(v57, 12) = 1282;
  F32(v57, 16) = settings;
  v58 = SYS32(1156);
  F32(SYS32(1156), 8) = SYS32(10356);
  F16(v58, 0) = 2;
  F16(v58, 2) = 0;
  F16(v58, 20) = group;
  F16(v58, 12) = 1189;
  F32(v58, 16) = settings;
  v59 = SYS32(1160);
  F32(SYS32(1160), 8) = SYS32(10356);
  F16(v59, 0) = 2;
  F16(v59, 2) = 0;
  F16(v59, 20) = group;
  F16(v59, 12) = 1187;
  F32(v59, 16) = settings;
  v60 = SYS32(1164);
  F32(SYS32(1164), 8) = SYS32(10356);
  F16(v60, 0) = 2;
  F16(v60, 2) = 0;
  F16(v60, 20) = group;
  F16(v60, 12) = 1188;
  F32(v60, 16) = settings;
  v61 = SYS32(1168);
  F32(SYS32(1168), 8) = SYS32(10356);
  F16(v61, 0) = 4;
  F16(v61, 2) = 0;
  F16(v61, 20) = group;
  F16(v61, 12) = 1283;
  F32(v61, 16) = settings;
  v62 = SYS32(1172);
  F32(SYS32(1172), 8) = SYS32(10356);
  F16(v62, 0) = 4;
  F16(v62, 2) = 0;
  F16(v62, 20) = group;
  F16(v62, 12) = 1284;
  F32(v62, 16) = settings;
  v63 = SYS32(1176);
  F32(SYS32(1176), 8) = SYS32(10356);
  F16(v63, 0) = 4;
  F16(v63, 2) = 0;
  F16(v63, 20) = group;
  F16(v63, 12) = 1229;
  F32(v63, 16) = settings;
  v64 = SYS32(1180);
  F32(SYS32(1180), 8) = SYS32(10356);
  F16(v64, 0) = 4;
  F16(v64, 2) = 0;
  F16(v64, 20) = group;
  F16(v64, 12) = 1228;
  F32(v64, 16) = settings;
  v65 = SYS32(1184);
  F32(SYS32(1184), 8) = SYS32(10356);
  F16(v65, 0) = 4;
  F16(v65, 2) = 0;
  F16(v65, 20) = group;
  F16(v65, 12) = 2053;
  F32(v65, 16) = settings;
  v66 = SYS32(1392);
  F32(SYS32(1392), 8) = SYS32(10356);
  F16(v66, 0) = 2;
  F16(v66, 2) = 0;
  F16(v66, 20) = group;
  F16(v66, 12) = 1190;
  F32(v66, 16) = settings;
  v67 = SYS32(1396);
  F32(SYS32(1396), 8) = SYS32(10356);
  F16(v67, 0) = 2;
  F16(v67, 2) = 0;
  F16(v67, 20) = group;
  F16(v67, 12) = 1192;
  F32(v67, 16) = settings;
  v68 = SYS32(1400);
  F32(SYS32(1400), 8) = SYS32(10356);
  F16(v68, 0) = 2;
  F16(v68, 2) = 0;
  F16(v68, 20) = group;
  F16(v68, 12) = 1194;
  F32(v68, 16) = settings;
  v69 = SYS32(1404);
  F32(SYS32(1404), 8) = SYS32(10356);
  F16(v69, 0) = 4;
  F16(v69, 2) = 0;
  F16(v69, 20) = group;
  F16(v69, 12) = 1191;
  F32(v69, 16) = settings;
  v70 = SYS32(1408);
  F32(SYS32(1408), 8) = SYS32(10356);
  F16(v70, 0) = 2;
  F16(v70, 2) = 0;
  F16(v70, 20) = group;
  F16(v70, 12) = 1193;
  F32(v70, 16) = settings;
  v71 = SYS32(1412);
  F32(SYS32(1412), 8) = SYS32(10356);
  F16(v71, 0) = 4;
  F16(v71, 2) = 0;
  F16(v71, 20) = group;
  F16(v71, 12) = 2230;
  F32(v71, 16) = settings;
  v72 = SYS32(1188);
  F32(SYS32(1188), 8) = SYS32(10356);
  F16(v72, 0) = 4;
  F16(v72, 2) = 0;
  F16(v72, 20) = group;
  F16(v72, 12) = 1244;
  F32(v72, 16) = settings;
  v73 = SYS32(1192);
  F32(SYS32(1192), 8) = SYS32(10356);
  F16(v73, 0) = 4;
  F16(v73, 2) = 0;
  F16(v73, 20) = group;
  F16(v73, 12) = 1245;
  F32(v73, 16) = settings;
  v74 = SYS32(1308);
  F32(SYS32(1308), 8) = SYS32(10356);
  F16(v74, 0) = 4;
  F16(v74, 2) = 0;
  F16(v74, 20) = group;
  F16(v74, 12) = 1212;
  F32(v74, 16) = settings;
  v75 = SYS32(1196);
  F32(SYS32(1196), 8) = SYS32(10356);
  F16(v75, 0) = 4;
  F16(v75, 2) = 0;
  F16(v75, 20) = group;
  F16(v75, 12) = 1246;
  F32(v75, 16) = settings;
  v76 = SYS32(1200);
  F32(SYS32(1200), 8) = SYS32(10356);
  F16(v76, 0) = 4;
  F16(v76, 2) = 0;
  F16(v76, 20) = group;
  F16(v76, 12) = 1247;
  F32(v76, 16) = settings;
  v77 = SYS32(1216);
  F32(SYS32(1216), 8) = SYS32(10356);
  F16(v77, 0) = 4;
  F16(v77, 2) = 0;
  F16(v77, 20) = group;
  F16(v77, 12) = 1259;
  F32(v77, 16) = settings;
  v78 = SYS32(1204);
  F32(SYS32(1204), 8) = SYS32(10356);
  F16(v78, 0) = 4;
  F16(v78, 2) = 0;
  F16(v78, 20) = group;
  F16(v78, 12) = 1251;
  F32(v78, 16) = settings;
  v79 = SYS32(1208);
  F32(SYS32(1208), 8) = SYS32(10356);
  F16(v79, 0) = 2;
  F16(v79, 2) = 0;
  F16(v79, 20) = group;
  F16(v79, 12) = 1249;
  F32(v79, 16) = settings;
  v80 = SYS32(1212);
  F32(SYS32(1212), 8) = SYS32(10356);
  F16(v80, 0) = 2;
  F16(v80, 2) = 0;
  F16(v80, 20) = group;
  F16(v80, 12) = 1324;
  F32(v80, 16) = settings;
  v81 = SYS32(1276);
  F32(SYS32(1276), 8) = SYS32(10356);
  F16(v81, 0) = 2;
  F16(v81, 2) = 0;
  F16(v81, 20) = group;
  F16(v81, 12) = 1260;
  F32(v81, 16) = settings;
  v82 = SYS32(1280);
  F32(SYS32(1280), 8) = SYS32(10356);
  F16(v82, 0) = 4;
  F16(v82, 2) = 0;
  F16(v82, 20) = group;
  F16(v82, 12) = 1261;
  F32(v82, 16) = settings;
  v83 = SYS32(1284);
  v84 = SYS32(10356);
  (*(uint16_t *)SYS32(1284)) = 4;
  F32(v83, 8) = v84;
  F16(v83, 2) = 0;
  F16(v83, 20) = group;
  F16(v83, 12) = 1262;
  F32(v83, 16) = settings;
  v85 = SYS32(1288);
  F32(SYS32(1288), 8) = SYS32(10356);
  F16(v85, 0) = 2;
  F16(v85, 2) = 0;
  F16(v85, 20) = group;
  F16(v85, 12) = 1248;
  F32(v85, 16) = settings;
  v86 = SYS32(1292);
  F32(SYS32(1292), 8) = SYS32(10356);
  F16(v86, 0) = 4;
  F16(v86, 2) = 0;
  F16(v86, 20) = group;
  F16(v86, 12) = 1256;
  F32(v86, 16) = settings;
  v87 = SYS32(1296);
  F32(SYS32(1296), 8) = SYS32(10356);
  F16(v87, 0) = 4;
  F16(v87, 2) = 0;
  F16(v87, 20) = group;
  F16(v87, 12) = 2166;
  F32(v87, 16) = settings;
  v88 = SYS32(1300);
  F32(SYS32(1300), 8) = SYS32(10356);
  F16(v88, 0) = 4;
  F16(v88, 2) = 0;
  F16(v88, 20) = group;
  F16(v88, 12) = 2173;
  F32(v88, 16) = settings;
  v89 = SYS32(1304);
  F32(SYS32(1304), 8) = SYS32(10356);
  F16(v89, 0) = 4;
  F16(v89, 2) = 0;
  F16(v89, 20) = group;
  F16(v89, 12) = 2174;
  F32(v89, 16) = settings;
  v90 = SYS32(1312);
  F32(SYS32(1312), 8) = SYS32(10356);
  F16(v90, 0) = 2;
  F16(v90, 2) = 0;
  F16(v90, 20) = group;
  F16(v90, 12) = 1321;
  F32(v90, 16) = settings;
  v91 = SYS32(1316);
  F32(SYS32(1316), 8) = SYS32(10356);
  F16(v91, 0) = 4;
  F16(v91, 2) = 0;
  F16(v91, 20) = group;
  F16(v91, 12) = 1322;
  F32(v91, 16) = settings;
  v92 = SYS32(1220);
  F32(SYS32(1220), 8) = SYS32(10356);
  F16(v92, 0) = 8;
  F16(v92, 2) = 0;
  F16(v92, 20) = group;
  F16(v92, 12) = 1946;
  F32(v92, 16) = settings;
  v93 = SYS32(1320);
  F32(SYS32(1320), 8) = SYS32(10356);
  F16(v93, 0) = 8;
  F16(v93, 2) = 0;
  F16(v93, 20) = group;
  F16(v93, 12) = 1985;
  F32(v93, 16) = settings;
  v94 = SYS32(1224);
  F32(SYS32(1224), 8) = SYS32(10356);
  F16(v94, 0) = 4;
  F16(v94, 2) = 0;
  F16(v94, 20) = group;
  F16(v94, 12) = 1243;
  F32(v94, 16) = settings;
  v95 = SYS32(1228);
  F32(SYS32(1228), 8) = SYS32(10356);
  F16(v95, 0) = 4;
  F16(v95, 2) = 0;
  F16(v95, 20) = group;
  F16(v95, 12) = 1242;
  F32(v95, 16) = settings;
  v96 = SYS32(1252);
  F32(SYS32(1252), 8) = SYS32(10356);
  F16(v96, 0) = 2;
  F16(v96, 2) = 0;
  F16(v96, 20) = group;
  F16(v96, 12) = 1254;
  F32(v96, 16) = settings;
  v97 = SYS32(1248);
  F32(SYS32(1248), 8) = SYS32(10356);
  F16(v97, 0) = 2;
  F16(v97, 2) = 0;
  F16(v97, 20) = group;
  F16(v97, 12) = 1255;
  F32(v97, 16) = settings;
  v98 = SYS32(1268);
  F32(SYS32(1268), 8) = SYS32(10356);
  F16(v98, 0) = 16;
  F16(v98, 2) = 0;
  F16(v98, 20) = group;
  F16(v98, 12) = 1323;
  F32(v98, 16) = settings;
  v99 = SYS32(1272);
  F32(SYS32(1272), 8) = SYS32(10356);
  F16(v99, 0) = 8;
  F16(v99, 2) = 0;
  F16(v99, 20) = group;
  F16(v99, 12) = 1777;
  F32(v99, 16) = settings;
  v100 = SYS32(1256);
  F32(SYS32(1256), 8) = SYS32(10356);
  F16(v100, 0) = 4;
  F16(v100, 2) = 0;
  F16(v100, 20) = group;
  F16(v100, 12) = 2162;
  F32(v100, 16) = settings;
  v101 = SYS32(1260);
  F32(SYS32(1260), 8) = SYS32(10356);
  F16(v101, 0) = 4;
  F16(v101, 2) = 0;
  F16(v101, 20) = group;
  F16(v101, 12) = 2163;
  F32(v101, 16) = settings;
  v102 = SYS32(1264);
  F32(SYS32(1264), 8) = SYS32(10356);
  F16(v102, 0) = 4;
  F16(v102, 2) = 0;
  F16(v102, 20) = group;
  F16(v102, 12) = 1776;
  F32(v102, 16) = settings;
  v103 = SYS32(1232);
  F32(SYS32(1232), 8) = SYS32(10356);
  F16(v103, 0) = 4;
  F16(v103, 2) = 0;
  F16(v103, 20) = group;
  F16(v103, 12) = 1252;
  F32(v103, 16) = settings;
  v104 = SYS32(1236);
  F32(SYS32(1236), 8) = SYS32(10356);
  F16(v104, 0) = 4;
  F16(v104, 2) = 0;
  F16(v104, 20) = group;
  F16(v104, 12) = 1253;
  F32(v104, 16) = settings;
  v105 = SYS32(1240);
  F32(SYS32(1240), 8) = SYS32(10356);
  F16(v105, 0) = 1;
  F16(v105, 2) = 0;
  F16(v105, 20) = group;
  F16(v105, 12) = 1257;
  F32(v105, 16) = settings;
  v106 = SYS32(1244);
  F32(SYS32(1244), 8) = SYS32(10356);
  F16(v106, 0) = 1;
  F16(v106, 2) = 0;
  F16(v106, 20) = group;
  F16(v106, 12) = 1258;
  F32(v106, 16) = settings;
  v107 = SYS32(1328);
  F32(SYS32(1328), 8) = SYS32(10356);
  F16(v107, 0) = 4;
  F16(v107, 2) = 0;
  F16(v107, 20) = group;
  F16(v107, 12) = 1781;
  F32(v107, 16) = settings;
  v108 = SYS32(1324);
  F32(SYS32(1324), 8) = SYS32(10356);
  F16(v108, 0) = 4;
  F16(v108, 2) = 0;
  F16(v108, 20) = group;
  F16(v108, 12) = 1782;
  F32(v108, 16) = settings;
  v109 = SYS32(1332);
  F32(SYS32(1332), 8) = SYS32(10356);
  F16(v109, 0) = 4;
  F16(v109, 2) = 0;
  F16(v109, 20) = group;
  F16(v109, 12) = 1982;
  F32(v109, 16) = settings;
  v110 = SYS32(1336);
  F32(SYS32(1336), 8) = SYS32(10356);
  F16(v110, 0) = 4;
  F16(v110, 2) = 0;
  F16(v110, 20) = group;
  F16(v110, 12) = 1238;
  F32(v110, 16) = settings;
  v111 = SYS32(1340);
  F32(SYS32(1340), 8) = SYS32(10356);
  F16(v111, 0) = 4;
  F16(v111, 2) = 0;
  F16(v111, 20) = group;
  F16(v111, 12) = 1239;
  F32(v111, 16) = settings;
  v112 = SYS32(1344);
  F32(SYS32(1344), 8) = SYS32(10356);
  F16(v112, 0) = 8;
  F16(v112, 2) = 0;
  F16(v112, 20) = group;
  F16(v112, 12) = 1232;
  F32(v112, 16) = settings;
  v113 = SYS32(1348);
  F32(SYS32(1348), 8) = SYS32(10356);
  F16(v113, 0) = 4;
  F16(v113, 2) = 0;
  F16(v113, 20) = group;
  F16(v113, 12) = 1235;
  F32(v113, 16) = settings;
  v114 = SYS32(1352);
  F32(SYS32(1352), 8) = SYS32(10356);
  F16(v114, 0) = 4;
  F16(v114, 2) = 0;
  F16(v114, 20) = group;
  F16(v114, 12) = 1999;
  F32(v114, 16) = settings;
  v115 = SYS32(1356);
  F32(SYS32(1356), 8) = SYS32(10356);
  F16(v115, 0) = 4;
  F16(v115, 2) = 0;
  F16(v115, 20) = group;
  F16(v115, 12) = 1233;
  F32(v115, 16) = settings;
  v116 = SYS32(1360);
  F32(SYS32(1360), 8) = SYS32(10356);
  F16(v116, 0) = 4;
  F16(v116, 2) = 0;
  F16(v116, 20) = group;
  F16(v116, 12) = 1234;
  F32(v116, 16) = settings;
  v117 = SYS32(1364);
  F32(SYS32(1364), 8) = SYS32(10356);
  F16(v117, 0) = 4;
  F16(v117, 2) = 0;
  F16(v117, 20) = group;
  F16(v117, 12) = 1237;
  F32(v117, 16) = settings;
  v118 = SYS32(1368);
  F32(SYS32(1368), 8) = SYS32(10356);
  F16(v118, 0) = 4;
  F16(v118, 2) = 0;
  F16(v118, 20) = group;
  F16(v118, 12) = 1230;
  F32(v118, 16) = settings;
  v119 = SYS32(1372);
  F32(SYS32(1372), 8) = SYS32(10356);
  F16(v119, 0) = 4;
  F16(v119, 2) = 0;
  F16(v119, 20) = group;
  F16(v119, 12) = 1231;
  F32(v119, 16) = settings;
  v120 = SYS32(1388);
  F32(SYS32(1388), 8) = SYS32(10356);
  F16(v120, 0) = 4;
  F16(v120, 2) = 0;
  F16(v120, 20) = group;
  F16(v120, 12) = 1338;
  F32(v120, 16) = settings;
  v121 = SYS32(1380);
  F32(SYS32(1380), 8) = SYS32(10356);
  F16(v121, 0) = 4;
  F16(v121, 2) = 0;
  F16(v121, 20) = group;
  F16(v121, 12) = 1236;
  F32(v121, 16) = settings;
  v122 = SYS32(1384);
  F32(SYS32(1384), 8) = SYS32(10356);
  F16(v122, 0) = 4;
  F16(v122, 2) = 0;
  F16(v122, 20) = group;
  F16(v122, 12) = 1922;
  F32(v122, 16) = settings;
  v123 = SYS32(1416);
  F32(SYS32(1416), 8) = SYS32(10356);
  F16(v123, 0) = 4;
  F16(v123, 2) = 0;
  F16(v123, 20) = group;
  F16(v123, 12) = 1272;
  F32(v123, 16) = settings;
  v124 = SYS32(1424);
  F32(SYS32(1424), 8) = SYS32(10356);
  F16(v124, 0) = 4;
  F16(v124, 2) = 0;
  F16(v124, 20) = group;
  F16(v124, 12) = 1275;
  F32(v124, 16) = settings;
  v125 = SYS32(1428);
  F32(SYS32(1428), 8) = SYS32(10356);
  F16(v125, 0) = 4;
  F16(v125, 2) = 0;
  F16(v125, 20) = group;
  F16(v125, 12) = 1340;
  F32(v125, 16) = settings;
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1440),
    *(void **)&SYSB(10356),
    settings,
    0x4F0u,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1432),
    *(void **)&SYSB(10356),
    settings,
    0x4F1u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1448),
    *(void **)&SYSB(10356),
    settings,
    0x7C8u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1452),
    *(void **)&SYSB(10356),
    settings,
    0x7C9u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1436),
    *(void **)&SYSB(10356),
    settings,
    0x4EFu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1420),
    *(void **)&SYSB(10356),
    settings,
    0x4FAu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1444),
    *(void **)&SYSB(10356),
    settings,
    0x4F9u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1456),
    *(void **)&SYSB(10356),
    settings,
    0x4F7u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1460),
    *(void **)&SYSB(10356),
    settings,
    0x71Cu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1464),
    *(void **)&SYSB(10356),
    settings,
    0x4FCu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1468),
    *(void **)&SYSB(10356),
    settings,
    0x4FDu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1472),
    *(void **)&SYSB(10356),
    settings,
    0x4FEu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1496),
    *(void **)&SYSB(10356),
    settings,
    0x668u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1500),
    *(void **)&SYSB(10356),
    settings,
    0x796u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1504),
    *(void **)&SYSB(10356),
    settings,
    0x66Au,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1476),
    *(void **)&SYSB(10356),
    settings,
    0x6B0u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1480),
    *(void **)&SYSB(10356),
    settings,
    0x7CCu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1484),
    *(void **)&SYSB(10356),
    settings,
    0x7CDu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1488),
    *(void **)&SYSB(10356),
    settings,
    0x7CEu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1696),
    *(void **)&SYSB(10356),
    settings,
    0x8BFu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1700),
    *(void **)&SYSB(10356),
    settings,
    0x8C1u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1732),
    *(void **)&SYSB(10356),
    settings,
    0x511u,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1736),
    *(void **)&SYSB(10356),
    settings,
    0x512u,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1740),
    *(void **)&SYSB(10356),
    settings,
    0x52Du,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1744),
    *(void **)&SYSB(10356),
    settings,
    0x52Eu,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(916),
    *(void **)&SYSB(10356),
    settings,
    0x50Au,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(920),
    *(void **)&SYSB(10356),
    settings,
    0x50Bu,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1748),
    *(void **)&SYSB(10356),
    settings,
    0x4B0u,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1752),
    *(void **)&SYSB(10356),
    settings,
    0x4B1u,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1756),
    *(void **)&SYSB(10356),
    settings,
    0x6E6u,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1760),
    *(void **)&SYSB(10356),
    settings,
    0x6E7u,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1764),
    *(void **)&SYSB(10356),
    settings,
    0x4B2u,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1768),
    *(void **)&SYSB(10356),
    settings,
    0x4B3u,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1772),
    *(void **)&SYSB(10356),
    settings,
    0x4B7u,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1776),
    *(void **)&SYSB(10356),
    settings,
    0x4B4u,
    6u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1828),
    *(void **)&SYSB(10356),
    settings,
    0x4B8u,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1840),
    *(void **)&SYSB(10356),
    settings,
    0x6FBu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1784),
    *(void **)&SYSB(10356),
    settings,
    0x76Eu,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1780),
    *(void **)&SYSB(10356),
    settings,
    0x76Fu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1788),
    *(void **)&SYSB(10356),
    settings,
    0x77Eu,
    0x10u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1844),
    *(void **)&SYSB(10356),
    settings,
    0x78Au,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1792),
    *(void **)&SYSB(10356),
    settings,
    0x7D1u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1796),
    *(void **)&SYSB(10356),
    settings,
    0x8CCu,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1800),
    *(void **)&SYSB(10356),
    settings,
    0x8CDu,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1832),
    *(void **)&SYSB(10356),
    settings,
    0x4ABu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1836),
    *(void **)&SYSB(10356),
    settings,
    0x4ACu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1804),
    *(void **)&SYSB(10356),
    settings,
    0x4B5u,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1808),
    *(void **)&SYSB(10356),
    settings,
    0x4B6u,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1812),
    *(void **)&SYSB(10356),
    settings,
    0x4ADu,
    8u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1816),
    *(void **)&SYSB(10356),
    settings,
    0x4AEu,
    2u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1824),
    *(void **)&SYSB(10356),
    settings,
    0x4AFu,
    4u,
    group,
    0);
  al_563ef0(
    *(ar_sound_slot **)&SYSB(1820),
    *(void **)&SYSB(10356),
    settings,
    0x7BFu,
    8u,
    group,
    0);
  v126 = 0;
  result = 0;
  do
  {
    v128 = ((uint16_t *)D(0x691022))[result / 2];
    if ( v128 == 0 )
      goto LABEL_12;
    if ( ((uint16_t *)D(0x691034))[result / 2] != 0
      && SYS32(784) != 0
      && F32(CFG(), 456) == 0 )
    {
      if ( ((uint16_t *)D(0x691034))[result / 2] == 0x7FFF )
        goto LABEL_12;
      (*v133__f) = group;
      (*v132__f) = ((uint16_t *)D(0x69102c))[result / 2] + 2;
      (*v131__f) = ((uint16_t *)D(0x691034))[result / 2];
      v129 = *(void **)&SYSB(784);
      goto LABEL_10;
    }
    v130 = ((uint16_t *)D(0x69102c))[result / 2];
    (*v133__f) = group;
    if ( ((int32_t *)D(0x691030))[result / 4] == -1 )
    {
      (*v132__f) = v130 + 2;
      (*v131__f) = ((uint16_t *)D(0x691022))[result / 2];
      v129 = *(void **)&SYSB(776);
LABEL_10:
      al_563ef0(
        *(ar_sound_slot **)((uint8_t *)g_eng.sys + (4 * (uint16_t)((uint16_t *)D(0x691020))[result / 2] + 868)),
        *(void **)&SYSB(10356),
        v129,
        (*v131__f),
        (*v132__f),
        (*v133__f),
        0);
      goto LABEL_12;
    }
    al_563ef0(
      *(ar_sound_slot **)((uint8_t *)g_eng.sys + (4 * (uint16_t)((uint16_t *)D(0x691020))[result / 2] + 868)),
      *(void **)&SYSB(10356),
      settings,
      v128,
      v130 + 2,
      group,
      0);
LABEL_12:
    result = 36 * (uint16_t)++v126;
  }
  while ( ((int32_t *)D(0x691018))[result / 4] != 0 );
  return result;
}

/* 0x57a330: void __stdcall ar_run_palette_ramp_57A330(void *zdd, unsigned __int16 group, void *settings) */
void al_57a330(void * zdd, uint16_t group, void * settings)
{

    uint8_t * v3 = 0;
    intptr_t i = 0;
    intptr_t v5 = 0;
    uint8_t * v6 = 0;
    intptr_t v7 = 0;
    uint8_t * v8 = 0;
    intptr_t k = 0;
    intptr_t v10 = 0;
    uint8_t * v11 = 0;
    intptr_t v12 = 0;
    uint8_t * v13 = 0;
    intptr_t n = 0;
    intptr_t v15 = 0;
    uint8_t * v16 = 0;
    intptr_t v17 = 0;
    uint8_t * v18 = 0;
    intptr_t jj = 0;
    intptr_t v20 = 0;
    uint8_t * v21 = 0;
    intptr_t v22 = 0;
    uint8_t * v23 = 0;
    intptr_t mm = 0;
    intptr_t v25 = 0;
    uint8_t * v26 = 0;
    intptr_t v27 = 0;
    uint8_t * v28 = 0;
    intptr_t i1 = 0;
    intptr_t v30 = 0;
    uint8_t * v31 = 0;
    intptr_t v32 = 0;
    uint8_t * v33 = 0;
    intptr_t i3 = 0;
    intptr_t v35 = 0;
    uint8_t * v36 = 0;
    intptr_t v37 = 0;
    uint8_t * v38 = 0;
    intptr_t i5 = 0;
    intptr_t v40 = 0;
    uint8_t * v41 = 0;
    intptr_t v42 = 0;
    uint8_t * v43 = 0;
    intptr_t i7 = 0;
    intptr_t v45 = 0;
    uint8_t * v46 = 0;
    intptr_t v47 = 0;
    uint8_t * v48 = 0;
    intptr_t i9 = 0;
    intptr_t v50 = 0;
    uint8_t * v51 = 0;
    intptr_t v52 = 0;
    uint8_t * v53 = 0;
    intptr_t i11 = 0;
    intptr_t v55 = 0;
    uint8_t * v56 = 0;
    intptr_t v57 = 0;
    uint8_t * v58 = 0;
    intptr_t i13 = 0;
    intptr_t v60 = 0;
    uint8_t * v61 = 0;
    intptr_t v62 = 0;
    intptr_t v63 = 0;
    void * v64 = 0;
    intptr_t v65 = 0;
    int v66 = 0;
    intptr_t v67 = 0;
    intptr_t v68 = 0;
    uint16_t v69 = 0;
    intptr_t v70 = 0;
    _Alignas(8) uint8_t frame_[0x848 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *out_palette = (uint8_t *)(frame_ + 0x4);   /* [4] at ebp-0x844 */
    uint8_t *v85 = (uint8_t *)(frame_ + 0x8);   /* [160] at ebp-0x840 */
    uint8_t *v86 = (uint8_t *)(frame_ + 0xa8);   /* [40] at ebp-0x7a0 */
    uint8_t *v87 = (uint8_t *)(frame_ + 0xd0);   /* [820] at ebp-0x778 */
    intptr_t *const nn__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const m__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const kk__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const ii__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const i8__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const i6__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const i4__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const i2__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const i15__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const i14__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const i12__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    intptr_t *const i10__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x848 */
    bitmap_session *const v88__f = (bitmap_session *)(frame_ + 0x404);   /* ebp-0x444 */
    intptr_t *const v89__f = (intptr_t *)(frame_ + 0x844);   /* ebp-0x4 */

  W_bs_5b6e70((intptr_t)(&(*v88__f)));
  (*v89__f) = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2736),
    zdd,
    *(void **)&SYSB(788),
    0x413u,
    0x18u,
    0x18u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2736)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(4210752));
  v3 = v86;
  for ( i = 10; i != 0; --i )
  {
    W_wm_5b5d90((intptr_t)((int)v3), (intptr_t)(4210752));
    v3 += 4;
  }
  v5 = 1;
  v6 = v87;
  for ( (*j__f) = 20; (*j__f) != 0; --(*j__f) )
  {
    v7 = W_wm_5b5f50((intptr_t)(0x404040u), (intptr_t)(0xFFFFFF), (intptr_t)(v5), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v6), (intptr_t)(v7));
    ++v5;
    v6 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2736)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2740),
    zdd,
    *(void **)&SYSB(788),
    0x412u,
    0x20u,
    0x20u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2740)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(4210752));
  v8 = v86;
  for ( k = 10; k != 0; --k )
  {
    W_wm_5b5d90((intptr_t)((int)v8), (intptr_t)(4210752));
    v8 += 4;
  }
  v10 = 1;
  v11 = v87;
  for ( (*m__f) = 20; (*m__f) != 0; --(*m__f) )
  {
    v12 = W_wm_5b5f50((intptr_t)(0x404040u), (intptr_t)(0xFFFFFF), (intptr_t)(v10), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v11), (intptr_t)(v12));
    ++v10;
    v11 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2740)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2752),
    zdd,
    *(void **)&SYSB(788),
    0x413u,
    0x18u,
    0x18u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2752)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(255));
  v13 = v86;
  for ( n = 10; n != 0; --n )
  {
    W_wm_5b5d90((intptr_t)((int)v13), (intptr_t)((int)D(0x6c6ccc)));
    v13 += 4;
  }
  v15 = 1;
  v16 = v87;
  for ( (*ii__f) = 20; (*ii__f) != 0; --(*ii__f) )
  {
    v17 = W_wm_5b5f50((intptr_t)((unsigned int)D(0x6c6ccc)), (intptr_t)(0xFFFFFF), (intptr_t)(v15), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v16), (intptr_t)(v17));
    ++v15;
    v16 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2752)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2744),
    zdd,
    *(void **)&SYSB(788),
    0x413u,
    0x18u,
    0x18u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2744)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(16711920));
  v18 = v86;
  for ( jj = 10; jj != 0; --jj )
  {
    W_wm_5b5d90((intptr_t)((int)v18), (intptr_t)(13397196));
    v18 += 4;
  }
  v20 = 1;
  v21 = v87;
  for ( (*kk__f) = 20; (*kk__f) != 0; --(*kk__f) )
  {
    v22 = W_wm_5b5f50((intptr_t)(0xCC6CCCu), (intptr_t)(0xFFFFFF), (intptr_t)(v20), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v21), (intptr_t)(v22));
    ++v20;
    v21 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2744)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2760),
    zdd,
    *(void **)&SYSB(788),
    0x413u,
    0x18u,
    0x18u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2760)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(16711680));
  v23 = v86;
  for ( mm = 10; mm != 0; --mm )
  {
    W_wm_5b5d90((intptr_t)((int)v23), (intptr_t)(13397100));
    v23 += 4;
  }
  v25 = 1;
  v26 = v87;
  for ( (*nn__f) = 20; (*nn__f) != 0; --(*nn__f) )
  {
    v27 = W_wm_5b5f50((intptr_t)(0xCC6C6Cu), (intptr_t)(0xFFFFFF), (intptr_t)(v25), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v26), (intptr_t)(v27));
    ++v25;
    v26 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2760)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2756),
    zdd,
    *(void **)&SYSB(788),
    0x412u,
    0x20u,
    0x20u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2756)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(255));
  v28 = v86;
  for ( i1 = 10; i1 != 0; --i1 )
  {
    W_wm_5b5d90((intptr_t)((int)v28), (intptr_t)((int)D(0x6c6ccc)));
    v28 += 4;
  }
  v30 = 1;
  v31 = v87;
  for ( (*i2__f) = 20; (*i2__f) != 0; --(*i2__f) )
  {
    v32 = W_wm_5b5f50((intptr_t)((unsigned int)D(0x6c6ccc)), (intptr_t)(0xFFFFFF), (intptr_t)(v30), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v31), (intptr_t)(v32));
    ++v30;
    v31 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2756)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2748),
    zdd,
    *(void **)&SYSB(788),
    0x412u,
    0x20u,
    0x20u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2748)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(16711920));
  v33 = v86;
  for ( i3 = 10; i3 != 0; --i3 )
  {
    W_wm_5b5d90((intptr_t)((int)v33), (intptr_t)(13397196));
    v33 += 4;
  }
  v35 = 1;
  v36 = v87;
  for ( (*i4__f) = 20; (*i4__f) != 0; --(*i4__f) )
  {
    v37 = W_wm_5b5f50((intptr_t)(0xCC6CCCu), (intptr_t)(0xFFFFFF), (intptr_t)(v35), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v36), (intptr_t)(v37));
    ++v35;
    v36 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2748)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2764),
    zdd,
    *(void **)&SYSB(788),
    0x412u,
    0x20u,
    0x20u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2764)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(0x8000));
  v38 = v86;
  for ( i5 = 10; i5 != 0; --i5 )
  {
    W_wm_5b5d90((intptr_t)((int)v38), (intptr_t)(0x40c040));
    v38 += 4;
  }
  v40 = 1;
  v41 = v87;
  for ( (*i6__f) = 20; (*i6__f) != 0; --(*i6__f) )
  {
    v42 = W_wm_5b5f50((intptr_t)((unsigned int)0x40c040), (intptr_t)(12648384), (intptr_t)(v40), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v41), (intptr_t)(v42));
    ++v40;
    v41 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2764)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2780),
    zdd,
    *(void **)&SYSB(788),
    0x413u,
    0x18u,
    0x18u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2780)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(33023));
  v43 = v86;
  for ( i7 = 10; i7 != 0; --i7 )
  {
    W_wm_5b5d90((intptr_t)((int)v43), (intptr_t)(33023));
    v43 += 4;
  }
  v45 = 1;
  v46 = v87;
  for ( (*i8__f) = 20; (*i8__f) != 0; --(*i8__f) )
  {
    v47 = W_wm_5b5f50((intptr_t)(0x80FFu), (intptr_t)(0xFFFFFF), (intptr_t)(v45), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v46), (intptr_t)(v47));
    ++v45;
    v46 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2780)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2768),
    zdd,
    *(void **)&SYSB(788),
    0x413u,
    0x18u,
    0x18u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2768)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(4210752));
  v48 = v86;
  for ( i9 = 10; i9 != 0; --i9 )
  {
    W_wm_5b5d90((intptr_t)((int)v48), (intptr_t)(49407));
    v48 += 4;
  }
  v50 = 1;
  v51 = v87;
  for ( (*i10__f) = 20; (*i10__f) != 0; --(*i10__f) )
  {
    v52 = W_wm_5b5f50((intptr_t)(0xC0FFu), (intptr_t)(12644607), (intptr_t)(v50), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v51), (intptr_t)(v52));
    ++v50;
    v51 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2768)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2772),
    zdd,
    *(void **)&SYSB(788),
    0x412u,
    0x20u,
    0x20u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2772)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(4210752));
  v53 = v86;
  for ( i11 = 10; i11 != 0; --i11 )
  {
    W_wm_5b5d90((intptr_t)((int)v53), (intptr_t)(49407));
    v53 += 4;
  }
  v55 = 1;
  v56 = v87;
  for ( (*i12__f) = 20; (*i12__f) != 0; --(*i12__f) )
  {
    v57 = W_wm_5b5f50((intptr_t)(0xC0FFu), (intptr_t)(12644607), (intptr_t)(v55), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v56), (intptr_t)(v57));
    ++v55;
    v56 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2772)), (intptr_t)(out_palette));
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2776),
    zdd,
    *(void **)&SYSB(788),
    0x413u,
    0x18u,
    0x18u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2776)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v85), (intptr_t)(4210752));
  v58 = v86;
  for ( i13 = 10; i13 != 0; --i13 )
  {
    W_wm_5b5d90((intptr_t)((int)v58), (intptr_t)(192));
    v58 += 4;
  }
  v60 = 1;
  v61 = v87;
  for ( (*i14__f) = 20; (*i14__f) != 0; --(*i14__f) )
  {
    v62 = W_wm_5b5f50((intptr_t)(0xC0u), (intptr_t)(12632319), (intptr_t)(v60), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)v61), (intptr_t)(v62));
    ++v60;
    v61 += 4;
  }
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2776)), (intptr_t)(out_palette));
  v63 = SYS32(2928);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2928)));
  F32(v63, 28) = zdd;
  F16(v63, 4) = 1;
  v64 = (intptr_t)g_eng.op_new(8u);
  v65 = 0;
  v66 = F16(v63, 4) == 0;
  F32(v63, 0) = v64;
  if ( !v66 )
  {
    do
    {
      v67 = 8 * (uint16_t)v65++;
      F32((v67 + F32(v63, 0)), 0) = 0;
      F32(v67 + F32(v63, 0), 4) = 0;
    }
    while ( (uint16_t)v65 < F16(v63, 4) );
  }
  F32(v63, 32) = 32;
  F32(v63, 36) = 32;
  F32(v63, 8) = 0;
  F32(v63, 24) = 0;
  F32(v63, 60) = settings;
  F16(v63, 64) = 1103;
  F32(v63, 40) = 0;
  F32(v63, 44) = 0;
  F32(v63, 48) = 2;
  F16(v63, 66) = group;
  F32(v63, 56) = 0;
  v68 = SYS32(2936);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2936)));
  F32(v68, 28) = zdd;
  F16(v68, 4) = 1;
  F32(v68, 0) = (intptr_t)g_eng.op_new(8u);
  v69 = 0;
  for ( (*i15__f) = 0; (uint16_t)(*i15__f) < F16(v68, 4); ++(*i15__f) )
  {
    v70 = 8 * v69;
    F32((v70 + F32(v68, 0)), 0) = 0;
    F32(v70 + F32(v68, 0), 4) = 0;
    v69 = (*i15__f) + 1;
  }
  F32(v68, 8) = 0;
  F32(v68, 24) = 0;
  F32(v68, 60) = settings;
  F16(v68, 64) = 1901;
  F32(v68, 32) = 32;
  F32(v68, 36) = 32;
  F32(v68, 40) = 0;
  F32(v68, 44) = 0;
  F32(v68, 48) = 2;
  F16(v68, 66) = group;
  F32(v68, 56) = 0;
  al_5748c0(*(ar_sprite_slot **)&SYSB(2932), zdd, 0, 0x5ABu, 0x20u, 0x20u, 0, 0, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2960), zdd, settings, 0x450u, 0x30u, 0x30u, 0, 0, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2964), zdd, settings, 0x451u, 0x40u, 0x40u, 0, 1u, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2980), zdd, settings, 0x776u, 0xC0u, 0x40u, 0, 0, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3012), zdd, settings, 0x44Bu, 0x160u, 0x60u, 0, 0, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(3016), zdd, settings, 0x778u, 0x60u, 0x40u, 0, 1u, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2988), zdd, settings, 0x44Du, 0x20u, 0x20u, 0, 1u, 0, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2992), zdd, settings, 0x44Au, 0xC0u, 0x40u, 0, 0, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2940), zdd, settings, 0x76Cu, 0x28u, 0x28u, 0, 0, 2u, group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3008),
    zdd,
    settings,
    0x775u,
    0x30u,
    0x10u,
    0xFF00FFu,
    0,
    2u,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3020),
    zdd,
    settings,
    0x601u,
    0x60u,
    0x60u,
    0xFF00FFu,
    1u,
    2u,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3024),
    zdd,
    settings,
    0x602u,
    0x60u,
    0x60u,
    0xFF00FFu,
    1u,
    2u,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3028),
    zdd,
    settings,
    0x603u,
    0x60u,
    0x60u,
    0xFF00FFu,
    1u,
    2u,
    group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2996), zdd, settings, 0x449u, 0x140u, 0x80u, 0, 1u, 2u, group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3000),
    zdd,
    settings,
    0x454u,
    0x190u,
    0x80u,
    0xFF00FFu,
    1u,
    2u,
    group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2944), zdd, settings, 0x458u, 0x40u, 4u, 0x1FFFFFFu, 1u, 0, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2948), zdd, settings, 0x583u, 0x40u, 4u, 0x1FFFFFFu, 0, 0, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2916), zdd, settings, 0x44Eu, 0x80u, 0x10u, 0, 1u, 0, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2920), zdd, settings, 0x777u, 0xC0u, 0x10u, 0, 1u, 0, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2924), zdd, settings, 0x909u, 0x20u, 0xF0u, 0, 1u, 0, group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3044),
    zdd,
    settings,
    0x3E8u,
    0x50u,
    0x160u,
    0x1FFFFFFu,
    0,
    0,
    group);
  F32(SYS32(6680), 4) = 3;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3048),
    zdd,
    settings,
    0x3E9u,
    0x50u,
    0x160u,
    0x1FFFFFFu,
    0,
    0,
    group);
  F32(SYS32(6684), 4) = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3052),
    zdd,
    settings,
    0x7E4u,
    0x50u,
    0x160u,
    0x1FFFFFFu,
    0,
    0,
    group);
  F32(SYS32(6688), 4) = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3056),
    zdd,
    settings,
    0x7B3u,
    0x50u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  F32(SYS32(6692), 4) = 3;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3060),
    zdd,
    settings,
    0x7B4u,
    0x50u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  F32(SYS32(6696), 4) = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3072),
    zdd,
    settings,
    0x3EAu,
    0x50u,
    0x160u,
    0x1FFFFFFu,
    0,
    0,
    group);
  F32(SYS32(6708), 4) = 3;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3076),
    zdd,
    settings,
    0x3EBu,
    0x50u,
    0x160u,
    0x1FFFFFFu,
    0,
    0,
    group);
  F32(SYS32(6712), 4) = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3080),
    zdd,
    settings,
    0x439u,
    0x50u,
    0x140u,
    0xFF00FFu,
    0,
    0,
    group);
  F32(SYS32(6716), 4) = 3;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3084), zdd, settings, 0x43Au, 0x50u, 0x100u, 0, 0, 0, group);
  F32(SYS32(6720), 4) = 3;
  al_5748c0(*(ar_sprite_slot **)&SYSB(3088), zdd, settings, 0x6BAu, 0x50u, 0xC0u, 0, 0, 0, group);
  F32(SYS32(6724), 4) = 3;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3092),
    zdd,
    settings,
    0x6B8u,
    0x50u,
    0x140u,
    0xFF00FFu,
    0,
    0,
    group);
  F32(SYS32(6728), 4) = 3;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3096),
    zdd,
    settings,
    0x6B9u,
    0x50u,
    0x90u,
    0xFF00FFu,
    0,
    0,
    group);
  F32(SYS32(6732), 4) = 3;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3064),
    zdd,
    settings,
    0x879u,
    0x50u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  F32(SYS32(6700), 4) = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(3068),
    zdd,
    settings,
    0x87Cu,
    0x50u,
    0x140u,
    0xFF00FFu,
    0,
    0,
    group);
  (*v89__f) = -1;
  F32(SYS32(6704), 4) = 0;
  W_bs_5b6e80((intptr_t)(&(*v88__f)));
}

/* 0x582e90: int __thiscall sub_582E90(cls_582E90 *this) */
int32_t al_582e90(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    void * v1 = 0;
    intptr_t result = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    uint32_t v6 = 0;
    uint32_t v7 = 0;
    uint32_t v8 = 0;
    char * v9 = 0;
    char * v10 = 0;
    char * v11 = 0;
    char v12 = 0;
    uint32_t v13 = 0;
    uint32_t v14 = 0;
    uint32_t v15 = 0;
    char * v16 = 0;
    char * v17 = 0;
    char * v18 = 0;
    char v19 = 0;
    uint32_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    uint32_t v23 = 0;
    uint32_t v24 = 0;
    uint32_t v25 = 0;
    char * v26 = 0;
    char * v27 = 0;
    char * v28 = 0;
    char v29 = 0;
    uint32_t v30 = 0;
    uint32_t v31 = 0;
    uint32_t v32 = 0;
    char * v33 = 0;
    char * v34 = 0;
    char * v35 = 0;
    char v36 = 0;
    uint32_t v37 = 0;
    uint32_t v38 = 0;
    uint32_t v39 = 0;
    char * v40 = 0;
    char * v41 = 0;
    char * v42 = 0;
    char v43 = 0;
    uint32_t v44 = 0;
    uint32_t LastError = 0;
    uint32_t v46 = 0;
    char * v47 = 0;
    char * v48 = 0;
    char * v49 = 0;
    char v50 = 0;
    uint32_t v51 = 0;
    uint32_t v52 = 0;
    char * v53 = 0;
    char * v54 = 0;
    char * v55 = 0;
    char v56 = 0;
    uint32_t v57 = 0;
    uint32_t v58 = 0;
    char * v59 = 0;
    char * v60 = 0;
    char * v61 = 0;
    char v62 = 0;
    uint32_t v63 = 0;
    uint32_t v64 = 0;
    char * v65 = 0;
    char * v66 = 0;
    char * v67 = 0;
    char v68 = 0;
    uint32_t v69 = 0;
    uint32_t v70 = 0;
    char * v71 = 0;
    char * v72 = 0;
    char * v73 = 0;
    char v74 = 0;
    uint32_t v75 = 0;
    _Alignas(8) uint8_t frame_[0x21c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v76 = (intptr_t *)(frame_ + 0x0);   /* [7] at ebp-0x21c */
    char *Buffer = (char *)(frame_ + 0x1c);   /* [256] at ebp-0x200 */
    char *OutputString = (char *)(frame_ + 0x11c);   /* [256] at ebp-0x100 */

  v1 = *(void **)&SYSB(864);
  if ( SYS32(864) != 0 )
  {
    W_rp_5b9390((intptr_t)(SYS32(864)));
    g_eng.op_delete((void *)v1);
    SYS32(864) = 0;
  }
  result = F32(c, 0x4);
  switch ( result )
  {
    case 0:
      if ( W_zd_5b8900((intptr_t)(SYS32(10348)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(16), (intptr_t)(0)) == 0 )
      {
        LastError = API_GetLastError();
        if ( LastError != 0 )
        {
          API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(OutputString), (intptr_t)(0xFFu), (intptr_t)(0));
          g_eng.h5bf3ee(Buffer, "[%08d] %s", LastError, OutputString);
          g_eng.log_fail_406440((const char *)D(0x8a9530), Buffer, D(0x8a9b6c));
        }
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v46 = strlen(((char *)D(0x8a29f0))) + 1;
          v47 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v47, ((char *)D(0x8a29f0)), 4 * (v46 >> 2));
          v49 = ((char *)D(0x8a29f0) + (4 * (v46 >> 2)));
          v48 = &v47[4 * (v46 >> 2)];
          v50 = v46;
        }
        else
        {
          v51 = strlen(((char *)D(0x8a29f0))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a29f0)), 4 * (v51 >> 2));
          v49 = ((char *)D(0x8a29f0) + (4 * (v51 >> 2)));
          v48 = &((char *)D(0x8a9530) + 0x404)[4 * (v51 >> 2) - 1024];
          v50 = v51;
        }
        qmemcpy(v48, v49, v50 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a29f0)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      W_zd_5b8dd0((intptr_t)(SYS32(10348)));
      W_wm_5b5ac0((intptr_t)(0x7D0u));
      if ( W_zd_5b8480((intptr_t)(SYS32(10348)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(16), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0)) == 0 )
      {
        RC_560900((intptr_t)((const char *)D(0x8a9530)));
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v52 = strlen(((char *)D(0x8a28e8))) + 1;
          v53 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v53, ((char *)D(0x8a28e8)), 4 * (v52 >> 2));
          v55 = ((char *)D(0x8a28e8) + (4 * (v52 >> 2)));
          v54 = &v53[4 * (v52 >> 2)];
          v56 = v52;
        }
        else
        {
          v57 = strlen(((char *)D(0x8a28e8))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a28e8)), 4 * (v57 >> 2));
          v55 = ((char *)D(0x8a28e8) + (4 * (v57 >> 2)));
          v54 = &((char *)D(0x8a9530) + 0x404)[4 * (v57 >> 2) - 1024];
          v56 = v57;
        }
        qmemcpy(v54, v55, v56 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a28e8)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      result = W_rp_5b8b40((intptr_t)(SYS32(10348)), (intptr_t)(&SYSB(864)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(0x1FFFFFF), (intptr_t)(1));
      break;
    case 1:
      if ( W_zd_5b8900((intptr_t)(SYS32(10348)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(16), (intptr_t)(0)) == 0 )
      {
        RC_560900((intptr_t)((const char *)D(0x8a9530)));
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v58 = strlen(((char *)D(0x8a29f0))) + 1;
          v59 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v59, ((char *)D(0x8a29f0)), 4 * (v58 >> 2));
          v61 = ((char *)D(0x8a29f0) + (4 * (v58 >> 2)));
          v60 = &v59[4 * (v58 >> 2)];
          v62 = v58;
        }
        else
        {
          v63 = strlen(((char *)D(0x8a29f0))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a29f0)), 4 * (v63 >> 2));
          v61 = ((char *)D(0x8a29f0) + (4 * (v63 >> 2)));
          v60 = &((char *)D(0x8a9530) + 0x404)[4 * (v63 >> 2) - 1024];
          v62 = v63;
        }
        qmemcpy(v60, v61, v62 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a29f0)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      W_zd_5b8dd0((intptr_t)(SYS32(10348)));
      W_wm_5b5ac0((intptr_t)(0x7D0u));
      result = W_zd_5b8480((intptr_t)(SYS32(10348)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(16), (intptr_t)(0), (intptr_t)(1), (intptr_t)(0));
      if ( result == 0 )
      {
        RC_560900((intptr_t)((const char *)D(0x8a9530)));
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v64 = strlen(((char *)D(0x8a28bc))) + 1;
          v65 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v65, ((char *)D(0x8a28bc)), 4 * (v64 >> 2));
          v67 = ((char *)D(0x8a28bc) + (4 * (v64 >> 2)));
          v66 = &v65[4 * (v64 >> 2)];
          v68 = v64;
        }
        else
        {
          v69 = strlen(((char *)D(0x8a28bc))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a28bc)), 4 * (v69 >> 2));
          v67 = ((char *)D(0x8a28bc) + (4 * (v69 >> 2)));
          v66 = &((char *)D(0x8a9530) + 0x404)[4 * (v69 >> 2) - 1024];
          v68 = v69;
        }
        qmemcpy(v66, v67, v68 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a28bc)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      return result;
    case 2:
      result = W_zd_5b8480((intptr_t)(SYS32(10348)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(16), (intptr_t)(2), (intptr_t)(1), (intptr_t)(0));
      if ( result == 0 )
      {
        v38 = API_GetLastError();
        if ( v38 != 0 )
        {
          API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(v38), (intptr_t)(0), (intptr_t)(OutputString), (intptr_t)(0xFFu), (intptr_t)(0));
          g_eng.h5bf3ee(Buffer, "[%08d] %s", v38, OutputString);
          g_eng.log_fail_406440((const char *)D(0x8a9530), Buffer, D(0x8a9b6c));
        }
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v39 = strlen(((char *)D(0x8a2918))) + 1;
          v40 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v40, ((char *)D(0x8a2918)), 4 * (v39 >> 2));
          v42 = ((char *)D(0x8a2918) + (4 * (v39 >> 2)));
          v41 = &v40[4 * (v39 >> 2)];
          v43 = v39;
        }
        else
        {
          v44 = strlen(((char *)D(0x8a2918))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a2918)), 4 * (v44 >> 2));
          v42 = ((char *)D(0x8a2918) + (4 * (v44 >> 2)));
          v41 = &((char *)D(0x8a9530) + 0x404)[4 * (v44 >> 2) - 1024];
          v43 = v44;
        }
        qmemcpy(v41, v42, v43 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a2918)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      return result;
    case 3:
      if ( W_zd_5b8900((intptr_t)(SYS32(10348)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(16), (intptr_t)(0)) == 0 )
      {
        RC_560900((intptr_t)((const char *)D(0x8a9530)));
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v70 = strlen(((char *)D(0x8a29f0))) + 1;
          v71 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v71, ((char *)D(0x8a29f0)), 4 * (v70 >> 2));
          v73 = ((char *)D(0x8a29f0) + (4 * (v70 >> 2)));
          v72 = &v71[4 * (v70 >> 2)];
          v74 = v70;
        }
        else
        {
          v75 = strlen(((char *)D(0x8a29f0))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a29f0)), 4 * (v75 >> 2));
          v73 = ((char *)D(0x8a29f0) + (4 * (v75 >> 2)));
          v72 = &((char *)D(0x8a9530) + 0x404)[4 * (v75 >> 2) - 1024];
          v74 = v75;
        }
        qmemcpy(v72, v73, v74 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a29f0)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      W_zd_5b8dd0((intptr_t)(SYS32(10348)));
      W_wm_5b5ac0((intptr_t)(0x7D0u));
      result = W_zd_5b8480((intptr_t)(SYS32(10348)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(16), (intptr_t)(3), (intptr_t)(1), (intptr_t)(0));
      if ( result == 0 )
      {
        g_eng.h426110((char *)D(0x8a9530), ((char *)D(0x8a29c8)), 1, D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      return result;
    case 4:
      memset(v76, 0, (7 * sizeof(intptr_t)));
      v4 = SYS32(844);
      if ( SYS32(844) <= 0
        || (v5 = SYS32(848), SYS32(848) <= 0) )
      {
        if ( W_zd_5b8950((intptr_t)(SYS32(10348)), (intptr_t)(v76), (intptr_t)(&v76[1]), (intptr_t)(0)) == 0 )
        {
          v6 = API_GetLastError();
          v7 = v6;
          if ( v6 != 0 )
          {
            API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(v6), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0xFFu), (intptr_t)(0));
            g_eng.h5bf3ee(OutputString, "[%08d] %s", v7, Buffer);
            g_eng.log_fail_406440((const char *)D(0x8a9530), OutputString, D(0x8a9b6c));
          }
          F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
          if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
          {
            strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
            v8 = strlen(((char *)D(0x8a2a18))) + 1;
            v9 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
            qmemcpy(v9, ((char *)D(0x8a2a18)), 4 * (v8 >> 2));
            v11 = ((char *)D(0x8a2a18) + (4 * (v8 >> 2)));
            v10 = &v9[4 * (v8 >> 2)];
            v12 = v8;
          }
          else
          {
            v13 = strlen(((char *)D(0x8a2a18))) + 1;
            qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a2a18)), 4 * (v13 >> 2));
            v11 = ((char *)D(0x8a2a18) + (4 * (v13 >> 2)));
            v10 = &((char *)D(0x8a9530) + 0x404)[4 * (v13 >> 2) - 1024];
            v12 = v13;
          }
          qmemcpy(v10, v11, v12 & 3);
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
          g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a2a18)), D(0x8a9b6c));
          g_eng.h5bf5db(0);
        }
        v5 = v76[1];
        v4 = v76[0];
      }
      else
      {
        v76[0] = SYS32(844);
        v76[1] = SYS32(848);
      }
      if ( v4 < F32(c, 0x14) || v5 < F32(c, 0x18) )
      {
        v31 = API_GetLastError();
        if ( v31 != 0 )
        {
          API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(v31), (intptr_t)(0), (intptr_t)(OutputString), (intptr_t)(0xFFu), (intptr_t)(0));
          g_eng.h5bf3ee(Buffer, "[%08d] %s", v31, OutputString);
          g_eng.log_fail_406440((const char *)D(0x8a9530), Buffer, D(0x8a9b6c));
        }
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v32 = strlen(((char *)D(0x8a2944))) + 1;
          v33 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v33, ((char *)D(0x8a2944)), 4 * (v32 >> 2));
          v35 = ((char *)D(0x8a2944) + (4 * (v32 >> 2)));
          v34 = &v33[4 * (v32 >> 2)];
          v36 = v32;
        }
        else
        {
          v37 = strlen(((char *)D(0x8a2944))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a2944)), 4 * (v37 >> 2));
          v35 = ((char *)D(0x8a2944) + (4 * (v37 >> 2)));
          v34 = &((char *)D(0x8a9530) + 0x404)[4 * (v37 >> 2) - 1024];
          v36 = v37;
        }
        qmemcpy(v34, v35, v36 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a2944)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      if ( W_zd_5b8900((intptr_t)(SYS32(10348)), (intptr_t)(v4), (intptr_t)(v5), (intptr_t)(16), (intptr_t)(0)) == 0 )
      {
        v14 = API_GetLastError();
        if ( v14 != 0 )
        {
          API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(v14), (intptr_t)(0), (intptr_t)(OutputString), (intptr_t)(0xFFu), (intptr_t)(0));
          g_eng.h5bf3ee(Buffer, "[%08d] %s", v14, OutputString);
          g_eng.log_fail_406440((const char *)D(0x8a9530), Buffer, D(0x8a9b6c));
        }
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v15 = strlen(((char *)D(0x8a29f0))) + 1;
          v16 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v16, ((char *)D(0x8a29f0)), 4 * (v15 >> 2));
          v18 = ((char *)D(0x8a29f0) + (4 * (v15 >> 2)));
          v17 = &v16[4 * (v15 >> 2)];
          v19 = v15;
        }
        else
        {
          v20 = strlen(((char *)D(0x8a29f0))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a29f0)), 4 * (v20 >> 2));
          v18 = ((char *)D(0x8a29f0) + (4 * (v20 >> 2)));
          v17 = &((char *)D(0x8a9530) + 0x404)[4 * (v20 >> 2) - 1024];
          v19 = v20;
        }
        qmemcpy(v17, v18, v19 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a29f0)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      W_zd_5b8dd0((intptr_t)(SYS32(10348)));
      W_wm_5b5ac0((intptr_t)(0x7D0u));
      v21 = F32(c, 0x14);
      v22 = F32(c, 0x18);
      v76[5] = v76[0];
      v76[3] = (v76[0] / 2 - v21 / 2) & ((v76[0] / 2 - v21 / 2 < 0) - 1);
      v76[4] = (v76[1] / 2 - v22 / 2) & ((v76[1] / 2 - v22 / 2 < 0) - 1);
      if ( v76[0] >= v21 )
        v76[5] = v21;
      v76[6] = v76[1];
      if ( v76[1] >= v22 )
        v76[6] = v22;
      v76[2] = 2;
      if ( W_zd_5b8480((intptr_t)(SYS32(10348)), (intptr_t)(640), (intptr_t)(480), (intptr_t)(16), (intptr_t)(4), (intptr_t)(1), (intptr_t)(v76)) == 0 )
      {
        v23 = API_GetLastError();
        v24 = v23;
        if ( v23 != 0 )
        {
          API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(v23), (intptr_t)(0), (intptr_t)(OutputString), (intptr_t)(0xFFu), (intptr_t)(0));
          g_eng.h5bf3ee(Buffer, "[%08d] %s", v24, OutputString);
          g_eng.log_fail_406440((const char *)D(0x8a9530), Buffer, D(0x8a9b6c));
        }
        F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
        if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
        {
          strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
          v25 = strlen(((char *)D(0x8a29c8))) + 1;
          v26 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
          qmemcpy(v26, ((char *)D(0x8a29c8)), 4 * (v25 >> 2));
          v28 = ((char *)D(0x8a29c8) + (4 * (v25 >> 2)));
          v27 = &v26[4 * (v25 >> 2)];
          v29 = v25;
        }
        else
        {
          v30 = strlen(((char *)D(0x8a29c8))) + 1;
          qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a29c8)), 4 * (v30 >> 2));
          v28 = ((char *)D(0x8a29c8) + (4 * (v30 >> 2)));
          v27 = &((char *)D(0x8a9530) + 0x404)[4 * (v30 >> 2) - 1024];
          v29 = v30;
        }
        qmemcpy(v27, v28, v29 & 3);
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
        g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a29c8)), D(0x8a9b6c));
        g_eng.h5bf5db(0);
      }
      result = W_zd_5b8dd0((intptr_t)(SYS32(10348)));
      break;
    default:
      return result;
  }
  return result;
}

/* 0x5749b0: int __stdcall sub_5749B0(void *zdd, unsigned __int16 group, void *settings) */
int32_t al_5749b0(void * zdd, uint16_t group, void * settings)
{

    intptr_t v3 = 0;
    void * v4 = 0;
    intptr_t v5 = 0;
    int v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    void * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    void * v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    void * v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    void * v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    void * v25 = 0;
    intptr_t v26 = 0;
    intptr_t v27 = 0;
    intptr_t v28 = 0;
    void * v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    void * v33 = 0;
    intptr_t v34 = 0;
    intptr_t v35 = 0;
    char * v36 = 0;
    intptr_t v37 = 0;
    intptr_t v38 = 0;
    intptr_t v39 = 0;
    uint16_t v40 = 0;
    intptr_t v41 = 0;
    _Alignas(8) uint8_t frame_[0x408 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *out_palette = (uint8_t *)(frame_ + 0x8);   /* [4] at ebp-0x400 */
    uint8_t *v48 = (uint8_t *)(frame_ + 0xc);   /* [160] at ebp-0x3fc */
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x408 */
    char * *const v44__f = (char * *)(frame_ + 0x0);   /* ebp-0x408 */
    intptr_t *const v45__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x404 */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x404 */
    char *const v49__f = (char *)(frame_ + 0xac);   /* ebp-0x35c */
    char *const v50__f = (char *)(frame_ + 0xd4);   /* ebp-0x334 */

  v3 = SYS32(2788);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2788)));
  F32(v3, 28) = zdd;
  F16(v3, 4) = 1;
  v4 = (intptr_t)g_eng.op_new(8u);
  v5 = 0;
  v6 = F16(v3, 4) == 0;
  F32(v3, 0) = v4;
  if ( !v6 )
  {
    do
    {
      v7 = 8 * (uint16_t)v5++;
      F32((v7 + F32(v3, 0)), 0) = 0;
      F32(v7 + F32(v3, 0), 4) = 0;
    }
    while ( (uint16_t)v5 < F16(v3, 4) );
  }
  F32(v3, 44) = 1;
  F32(v3, 8) = 0;
  F32(v3, 24) = 0;
  F32(v3, 60) = settings;
  F16(v3, 64) = 1183;
  F32(v3, 32) = 224;
  F32(v3, 36) = 224;
  F32(v3, 40) = 0;
  F32(v3, 48) = 2;
  F16(v3, 66) = group;
  F32(v3, 56) = 0;
  v8 = SYS32(2792);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2792)));
  F32(v8, 28) = zdd;
  F16(v8, 4) = 1;
  v9 = (intptr_t)g_eng.op_new(8u);
  v10 = 0;
  v6 = F16(v8, 4) == 0;
  F32(v8, 0) = v9;
  if ( !v6 )
  {
    do
    {
      v11 = 8 * (uint16_t)v10++;
      F32((v11 + F32(v8, 0)), 0) = 0;
      F32(v11 + F32(v8, 0), 4) = 0;
    }
    while ( (uint16_t)v10 < F16(v8, 4) );
  }
  F32(v8, 8) = 0;
  F32(v8, 24) = 0;
  F32(v8, 60) = settings;
  F16(v8, 64) = 1096;
  F32(v8, 32) = 152;
  F32(v8, 36) = 40;
  F32(v8, 40) = 0;
  F32(v8, 44) = 1;
  F32(v8, 48) = 2;
  F16(v8, 66) = group;
  F32(v8, 56) = 0;
  v12 = SYS32(2796);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2796)));
  F32(v12, 28) = zdd;
  F16(v12, 4) = 1;
  v13 = (intptr_t)g_eng.op_new(8u);
  v14 = 0;
  v6 = F16(v12, 4) == 0;
  F32(v12, 0) = v13;
  if ( !v6 )
  {
    do
    {
      v15 = 8 * (uint16_t)v14++;
      F32((v15 + F32(v12, 0)), 0) = 0;
      F32(v15 + F32(v12, 0), 4) = 0;
    }
    while ( (uint16_t)v14 < F16(v12, 4) );
  }
  F32(v12, 8) = 0;
  F32(v12, 24) = 0;
  F32(v12, 60) = settings;
  F16(v12, 64) = 1186;
  F32(v12, 32) = 144;
  F32(v12, 36) = 108;
  F32(v12, 40) = 0;
  F32(v12, 44) = 1;
  F32(v12, 48) = 2;
  F16(v12, 66) = group;
  F32(v12, 56) = 0;
  v16 = SYS32(2800);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2800)));
  F32(v16, 28) = zdd;
  F16(v16, 4) = 1;
  v17 = (intptr_t)g_eng.op_new(8u);
  v18 = 0;
  v6 = F16(v16, 4) == 0;
  F32(v16, 0) = v17;
  if ( !v6 )
  {
    do
    {
      v19 = 8 * (uint16_t)v18++;
      F32((v19 + F32(v16, 0)), 0) = 0;
      F32(v19 + F32(v16, 0), 4) = 0;
    }
    while ( (uint16_t)v18 < F16(v16, 4) );
  }
  F32(v16, 8) = 0;
  F32(v16, 24) = 0;
  F32(v16, 60) = settings;
  F16(v16, 64) = 1181;
  F32(v16, 32) = 640;
  F32(v16, 36) = 480;
  F32(v16, 40) = 0x1FFFFFF;
  F32(v16, 44) = 1;
  F32(v16, 48) = 0;
  F16(v16, 66) = group;
  F32(v16, 56) = 0;
  v20 = SYS32(2808);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2808)));
  F32(v20, 28) = zdd;
  F16(v20, 4) = 1;
  v21 = (intptr_t)g_eng.op_new(8u);
  v22 = 0;
  v6 = F16(v20, 4) == 0;
  F32(v20, 0) = v21;
  if ( !v6 )
  {
    do
    {
      v23 = 8 * (uint16_t)v22++;
      F32((v23 + F32(v20, 0)), 0) = 0;
      F32(v23 + F32(v20, 0), 4) = 0;
    }
    while ( (uint16_t)v22 < F16(v20, 4) );
  }
  F32(v20, 8) = 0;
  F32(v20, 24) = 0;
  F32(v20, 60) = settings;
  F16(v20, 64) = 2331;
  F32(v20, 32) = 640;
  F32(v20, 36) = 480;
  F32(v20, 40) = 0x1FFFFFF;
  F32(v20, 44) = 1;
  F32(v20, 48) = 0;
  F16(v20, 66) = group;
  F32(v20, 56) = 0;
  v24 = SYS32(2812);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2812)));
  F32(v24, 28) = zdd;
  F16(v24, 4) = 1;
  v25 = (intptr_t)g_eng.op_new(8u);
  v26 = 0;
  v6 = F16(v24, 4) == 0;
  F32(v24, 0) = v25;
  if ( !v6 )
  {
    do
    {
      v27 = 8 * (uint16_t)v26++;
      F32((F32(v24, 0) + v27), 0) = 0;
      F32(F32(v24, 0) + v27, 4) = 0;
    }
    while ( (uint16_t)v26 < F16(v24, 4) );
  }
  F32(v24, 8) = 0;
  F32(v24, 24) = 0;
  F32(v24, 60) = settings;
  F16(v24, 64) = 2332;
  F32(v24, 32) = 160;
  F32(v24, 36) = 32;
  F32(v24, 40) = 0x1FFFFFF;
  F32(v24, 44) = 1;
  F32(v24, 48) = 0;
  F16(v24, 66) = group;
  F32(v24, 56) = 0;
  v28 = SYS32(2816);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2816)));
  F32(v28, 28) = zdd;
  F16(v28, 4) = 1;
  v29 = (intptr_t)g_eng.op_new(8u);
  v30 = 0;
  v6 = F16(v28, 4) == 0;
  F32(v28, 0) = v29;
  if ( !v6 )
  {
    do
    {
      v31 = 8 * (uint16_t)v30++;
      F32((F32(v28, 0) + v31), 0) = 0;
      F32(F32(v28, 0) + v31, 4) = 0;
    }
    while ( (uint16_t)v30 < F16(v28, 4) );
  }
  F32(v28, 8) = 0;
  F32(v28, 24) = 0;
  F32(v28, 60) = settings;
  F16(v28, 64) = 2333;
  F32(v28, 32) = 32;
  F32(v28, 36) = 32;
  F32(v28, 40) = 0;
  F32(v28, 44) = 1;
  F32(v28, 48) = 2;
  F16(v28, 66) = group;
  F32(v28, 56) = 0;
  v32 = SYS32(2804);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2804)));
  F32(v32, 28) = zdd;
  F16(v32, 4) = 1;
  v33 = (intptr_t)g_eng.op_new(8u);
  v34 = 0;
  v6 = F16(v32, 4) == 0;
  F32(v32, 0) = v33;
  if ( !v6 )
  {
    do
    {
      v35 = 8 * (uint16_t)v34++;
      F32((F32(v32, 0) + v35), 0) = 0;
      F32(F32(v32, 0) + v35, 4) = 0;
    }
    while ( (uint16_t)v34 < F16(v32, 4) );
  }
  F32(v32, 8) = 0;
  F32(v32, 24) = 0;
  F32(v32, 60) = settings;
  F16(v32, 64) = 2323;
  F32(v32, 32) = 640;
  F32(v32, 36) = 480;
  F32(v32, 40) = 0x1FFFFFF;
  F32(v32, 44) = 1;
  F32(v32, 48) = 0;
  F16(v32, 66) = group;
  F32(v32, 56) = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2784),
    zdd,
    *(void **)&SYSB(788),
    0x90Bu,
    0x20u,
    0x20u,
    0,
    0,
    2u,
    group);
  W_fl_4178e0((intptr_t)(*(ar_sprite_slot **)&SYSB(2784)), (intptr_t)(out_palette));
  W_wm_5b5d90((intptr_t)((int)v48), (intptr_t)(0));
  v36 = &(*v49__f);
  for ( (*i__f) = 10; (*i__f) != 0; --(*i__f) )
  {
    W_wm_5b5d90((intptr_t)((int)v36), (intptr_t)(3684408));
    v36 += 4;
  }
  v37 = 1;
  (*v44__f) = &(*v50__f);
  (*v45__f) = 20;
  do
  {
    v38 = W_wm_5b5f50((intptr_t)(0x383838u), (intptr_t)(0xFFFFFF), (intptr_t)(v37), (intptr_t)(20));
    W_wm_5b5d90((intptr_t)((int)(*v44__f)), (intptr_t)(v38));
    ++v37;
    v6 = (*v45__f) == 1;
    (*v44__f) += 4;
    --(*v45__f);
  }
  while ( !v6 );
  W_slot_palette_set((intptr_t)(*(ar_sprite_slot **)&SYSB(2784)), (intptr_t)(out_palette));
  v39 = SYS32(2820);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2820)));
  F32(v39, 28) = zdd;
  F16(v39, 4) = 1;
  F32(v39, 0) = (intptr_t)g_eng.op_new(8u);
  v40 = 0;
  for ( (*j__f) = 0; (uint16_t)(*j__f) < F16(v39, 4); ++(*j__f) )
  {
    v41 = 8 * v40;
    F32((F32(v39, 0) + v41), 0) = 0;
    F32(F32(v39, 0) + v41, 4) = 0;
    v40 = (*j__f) + 1;
  }
  F32(v39, 8) = 0;
  F32(v39, 24) = 0;
  F32(v39, 60) = settings;
  F16(v39, 64) = 2271;
  F32(v39, 32) = 368;
  F32(v39, 36) = 276;
  F32(v39, 40) = 0x1FFFFFF;
  F32(v39, 44) = 1;
  F32(v39, 48) = 0;
  F16(v39, 66) = group;
  F32(v39, 56) = 0;
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2824),
    zdd,
    settings,
    0x8E0u,
    0x170u,
    0x114u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2828),
    zdd,
    settings,
    0x8E1u,
    0x170u,
    0x114u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2832),
    zdd,
    settings,
    0x8E2u,
    0x170u,
    0x114u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2836),
    zdd,
    settings,
    0x8E3u,
    0x170u,
    0x114u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2840),
    zdd,
    settings,
    0x8E4u,
    0x170u,
    0x114u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2844),
    zdd,
    settings,
    0x8E5u,
    0x170u,
    0x114u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2848),
    zdd,
    settings,
    0x90Eu,
    0x170u,
    0x114u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2852),
    zdd,
    settings,
    0x8DEu,
    0x190u,
    0xA0u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2856),
    zdd,
    settings,
    0x919u,
    0x150u,
    0x50u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2860),
    zdd,
    settings,
    0x712u,
    0x160u,
    0xB0u,
    0x1FFFFFFu,
    1u,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2864),
    zdd,
    settings,
    0x6F8u,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2868),
    zdd,
    settings,
    0x6F9u,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2872),
    zdd,
    settings,
    0x715u,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2876),
    zdd,
    settings,
    0x77Fu,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2880),
    zdd,
    settings,
    0x714u,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2884),
    zdd,
    settings,
    0x7D9u,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2888),
    zdd,
    settings,
    0x7A4u,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2892),
    zdd,
    settings,
    0x7D8u,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2896),
    zdd,
    settings,
    0x90Cu,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(
    *(ar_sprite_slot **)&SYSB(2900),
    zdd,
    settings,
    0x90Du,
    0x280u,
    0x1E0u,
    0x1FFFFFFu,
    0,
    0,
    group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2984), zdd, settings, 0x456u, 0x20u, 0x20u, 0, 0, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2968), zdd, settings, 0x453u, 0x40u, 0x40u, 0, 0, 2u, group);
  al_5748c0(*(ar_sprite_slot **)&SYSB(2972), zdd, settings, 0x660u, 0xA0u, 0x20u, 0, 0, 2u, group);
  return al_5748c0(
           *(ar_sprite_slot **)&SYSB(3004),
           zdd,
           settings,
           0x6FAu,
           0x20u,
           0x20u,
           0,
           0,
           2u,
           group);
}

/* 0x5748c0: int __thiscall sub_5748C0( ar_sprite_slot *this, void *zdd, void *settings, unsigned __int16 resource_id, unsigned int width, unsigned int height, unsigned int colorkey, unsigned int scale_flag, unsigned int type, unsigned __int16 group) */
int32_t al_5748c0(wl_char *self, void * zdd, void * settings, uint16_t resource_id, uint32_t width, uint32_t height, uint32_t colorkey, uint32_t scale_flag, uint32_t type, uint16_t group)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v12 = 0;
    ar_sprite_entry * v13 = 0;
    intptr_t v14 = 0;
    int v15 = 0;
    intptr_t v16 = 0;

  if ( ((ar_sprite_slot *)c)->aux_buf != 0 )
  {
    g_eng.op_delete((void *)((ar_sprite_slot *)c)->aux_buf);
    ((ar_sprite_slot *)c)->aux_buf = 0;
  }
  if ( ((ar_sprite_slot *)c)->entries != 0 )
  {
    for ( i = 0; i < ((ar_sprite_slot *)c)->entry_count; ++i )
    {
      v12 = i;
      if ( ((ar_sprite_slot *)c)->entries[v12].b != 0 )
      {
        g_eng.op_delete((void *)((ar_sprite_slot *)c)->entries[v12].b);
        ((ar_sprite_slot *)c)->entries[v12].b = 0;
      }
    }
    g_eng.op_delete((void *)((ar_sprite_slot *)c)->entries);
    ((ar_sprite_slot *)c)->entries = 0;
  }
  ((ar_sprite_slot *)c)->zdd = zdd;
  ((ar_sprite_slot *)c)->entry_count = 1;
  v13 = (ar_sprite_entry *)(intptr_t)g_eng.op_new(8u);
  v14 = 0;
  v15 = ((ar_sprite_slot *)c)->entry_count == 0;
  ((ar_sprite_slot *)c)->entries = v13;
  if ( !v15 )
  {
    do
    {
      v16 = (uint16_t)v14++;
      ((ar_sprite_slot *)c)->entries[v16].frames = 0;
      ((ar_sprite_slot *)c)->entries[v16].b = 0;
    }
    while ( (uint16_t)v14 < ((ar_sprite_slot *)c)->entry_count );
  }
  ((ar_sprite_slot *)c)->settings = settings;
  ((ar_sprite_slot *)c)->resource_id = resource_id;
  ((ar_sprite_slot *)c)->width = width;
  ((ar_sprite_slot *)c)->height = height;
  ((ar_sprite_slot *)c)->colorkey = colorkey;
  ((ar_sprite_slot *)c)->f_08 = 0;
  ((ar_sprite_slot *)c)->f_18 = 0;
  ((ar_sprite_slot *)c)->scale_flag = scale_flag;
  ((ar_sprite_slot *)c)->type = type;
  ((ar_sprite_slot *)c)->group = group;
  ((ar_sprite_slot *)c)->f_38 = 0;
  return 1;
}

/* 0x579680: DWORD __stdcall sub_579680(char *a1) */
int32_t al_579680(char * a1)
{

    uint32_t result = 0;
    intptr_t i = 0;
    _Alignas(8) uint8_t frame_[0x400 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Filename = (char *)(frame_ + 0x0);   /* [1024] at ebp-0x400 */

  Filename[0] = F8(D(0x8a9b6c), 0);
  memset(&Filename[1], 0, 1023);
  strcpy(a1, D(0x8a9b6c));
  result = API_GetModuleFileNameA((intptr_t)(0), (intptr_t)(Filename), (intptr_t)(0x400u));
  if ( result != 0 )
  {
    for ( i = strlen(Filename) - 1; i >= 0; Filename[i--] = 0 )
    {
      if ( Filename[i] == 92 )
        break;
    }
    strcpy(a1, Filename);
    return 1;
  }
  return result;
}

/* 0x579740: int __stdcall sub_579740( HMODULE hModule, unsigned __int16 a2, int a3, unsigned int a4, int a5, DWORD nNumberOfBytesToRead) */
int32_t al_579740(int32_t hModule, uint16_t a2, int32_t a3, uint32_t a4, int32_t a5, uint32_t nNumberOfBytesToRead)
{

    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    _Alignas(8) uint8_t frame_[0x49c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v13 = (uint8_t *)(frame_ + 0x4);   /* [120] at ebp-0x498 */
    intptr_t *v14 = (intptr_t *)(frame_ + 0x7c);   /* [52] at ebp-0x420 */
    uint32_t *const Buffer__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x49c */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x498);   /* ebp-0x4 */

  W_fs_5b6060((intptr_t)(v14));
  v6 = 0;
  (*v15__f) = 0;
  if ( W_fs_5b62a0((intptr_t)(v14), (intptr_t)(hModule), (intptr_t)(a2)) != 0 )
  {
    W_fs_5b6340((intptr_t)(v14), (intptr_t)(&(*Buffer__f)), (intptr_t)(4u));
    W_fs_5b6340((intptr_t)(v14), (intptr_t)(v13), (intptr_t)(nNumberOfBytesToRead));
    v7 = 0;
    if ( nNumberOfBytesToRead != 0 )
    {
      v8 = 0;
      do
      {
        ++v7;
        v13[v8 + 60] = v13[v8] + 65;
        v8 = (uint16_t)v7;
      }
      while ( (uint16_t)v7 < nNumberOfBytesToRead );
    }
    W_fs_5b64b0((intptr_t)(v14));
    if ( (*Buffer__f) >= a4 )
    {
      v9 = 0;
      if ( nNumberOfBytesToRead != 0 )
      {
        v10 = 0;
        while ( v13[v10 + 60] == F8((v10 + a3), 0) )
        {
          v10 = (uint16_t)++v9;
          if ( (uint16_t)v9 >= nNumberOfBytesToRead )
            goto LABEL_10;
        }
      }
      else
      {
LABEL_10:
        v6 = 1;
      }
    }
  }
  (*v15__f) = -1;
  W_fs_5b60d0((intptr_t)(v14));
  return v6;
}

/* 0x579860: char *__thiscall sub_579860(const char *this, const char *a2, char *a3) */
int32_t al_579860(wl_char *self, char * a2, char * a3)
{
    intptr_t c = (intptr_t)self;

  strcpy(a3, c + 514);
  strcat(a3, ((char *)D(0x854f5c)));
  strcat(a3, a2);
  return a3;
}

/* 0x5798f0: char *__thiscall sub_5798F0(cls_5798F0 *this, const char *a2, char *a3) */
int32_t al_5798f0(wl_char *self, char * a2, char * a3)
{
    intptr_t c = (intptr_t)self;

    _Alignas(8) uint8_t frame_[0x208 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [260] at ebp-0x208 */
    char *TempFileName = (char *)(frame_ + 0x104);   /* [260] at ebp-0x104 */

  if ( API_GetTempPathA((intptr_t)(0x104u), (intptr_t)(Buffer)) != 0 && API_GetTempFileNameA((intptr_t)(Buffer), (intptr_t)(((char *)D(0x8a2804))), (intptr_t)(0), (intptr_t)(TempFileName)) != 0 )
    return strcpy(a3, TempFileName);
  strcpy(a3, (uint8_t *)c + 514);
  strcat(a3, ((char *)D(0x854f5c)));
  strcat(a3, a2);
  return a3;
}

/* 0x579a00: int __stdcall sub_579A00(int a1, unsigned __int16 group, void *settings) */
int32_t al_579a00(int32_t a1, uint16_t group, void * settings)
{

    intptr_t v3 = 0;
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

  v3 = SYS32(868);
  F32(SYS32(868), 8) = SYS32(10356);
  F16(v3, 0) = 2;
  F16(v3, 2) = 0;
  F16(v3, 20) = group;
  F16(v3, 12) = 1295;
  F32(v3, 16) = settings;
  v4 = SYS32(872);
  F32(SYS32(872), 8) = SYS32(10356);
  F16(v4, 0) = 2;
  F16(v4, 2) = 0;
  F16(v4, 20) = group;
  F16(v4, 12) = 1294;
  F32(v4, 16) = settings;
  v5 = SYS32(876);
  F32(SYS32(876), 8) = SYS32(10356);
  F16(v5, 0) = 2;
  F16(v5, 2) = 0;
  F16(v5, 20) = group;
  F16(v5, 12) = 1288;
  F32(v5, 16) = settings;
  v6 = SYS32(880);
  F32(SYS32(880), 8) = SYS32(10356);
  F16(v6, 0) = 2;
  F16(v6, 2) = 0;
  F16(v6, 20) = group;
  F16(v6, 12) = 1296;
  F32(v6, 16) = settings;
  v7 = SYS32(884);
  F32(SYS32(884), 8) = SYS32(10356);
  F16(v7, 0) = 2;
  F16(v7, 2) = 0;
  F16(v7, 20) = group;
  F16(v7, 12) = 2307;
  F32(v7, 16) = settings;
  v8 = SYS32(896);
  F32(SYS32(896), 8) = SYS32(10356);
  F16(v8, 0) = 2;
  F16(v8, 2) = 0;
  F16(v8, 20) = group;
  F16(v8, 12) = 1287;
  F32(v8, 16) = settings;
  v9 = SYS32(908);
  F32(SYS32(908), 8) = SYS32(10356);
  F16(v9, 0) = 2;
  F16(v9, 2) = 0;
  F16(v9, 20) = group;
  F16(v9, 12) = 1240;
  F32(v9, 16) = settings;
  v10 = SYS32(912);
  F32(SYS32(912), 8) = SYS32(10356);
  F16(v10, 0) = 2;
  F16(v10, 2) = 0;
  F16(v10, 20) = group;
  F16(v10, 12) = 1241;
  F32(v10, 16) = settings;
  v11 = SYS32(888);
  F32(SYS32(888), 8) = SYS32(10356);
  F16(v11, 0) = 4;
  F16(v11, 2) = 0;
  F16(v11, 20) = group;
  F16(v11, 12) = 1289;
  F32(v11, 16) = settings;
  v12 = SYS32(892);
  F32(SYS32(892), 8) = SYS32(10356);
  F16(v12, 0) = 4;
  F16(v12, 2) = 0;
  F16(v12, 20) = group;
  F16(v12, 12) = 1286;
  F32(v12, 16) = settings;
  v13 = SYS32(904);
  F32(SYS32(904), 8) = SYS32(10356);
  F16(v13, 0) = 4;
  F16(v13, 2) = 0;
  F16(v13, 20) = group;
  F16(v13, 12) = 1293;
  F32(v13, 16) = settings;
  return al_563ef0(
           *(ar_sound_slot **)&SYSB(900),
           *(void **)&SYSB(10356),
           settings,
           0x50Cu,
           4u,
           group,
           0);
}

/* 0x579bd0: HPEN __stdcall sub_579BD0(int a1, unsigned __int16 group, int a3) */
int32_t al_579bd0(intptr_t ecx_, int32_t a1, uint16_t group, int32_t a3)
{

    intptr_t v3 = 0;
    void * v4 = 0;
    intptr_t v5 = 0;
    int v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    void * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t ** v12 = 0;
    ar_gdi_slot * v13 = 0;
    intptr_t ** v14 = 0;
    ar_gdi_slot * v15 = 0;
    intptr_t ** v16 = 0;
    ar_gdi_slot * v17 = 0;
    intptr_t ** v18 = 0;
    ar_gdi_slot * v19 = 0;
    intptr_t ** v20 = 0;
    ar_gdi_slot * v21 = 0;
    intptr_t ** v22 = 0;
    ar_gdi_slot * v23 = 0;
    intptr_t ** v24 = 0;
    ar_gdi_slot * v25 = 0;
    intptr_t ** v26 = 0;
    ar_gdi_slot * v27 = 0;

  v3 = SYS32(2952);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2952)));
  F32(v3, 28) = a1;
  F16(v3, 4) = 1;
  v4 = (intptr_t)g_eng.op_new(8u);
  v5 = 0;
  v6 = F16(v3, 4) == 0;
  F32(v3, 0) = v4;
  if ( !v6 )
  {
    do
    {
      v7 = 8 * (uint16_t)v5++;
      F32((F32(v3, 0) + v7), 0) = 0;
      F32(F32(v3, 0) + v7, 4) = 0;
    }
    while ( (uint16_t)v5 < F16(v3, 4) );
  }
  F32(v3, 60) = a3;
  F32(v3, 8) = 0;
  F32(v3, 24) = 0;
  F16(v3, 64) = 1111;
  F32(v3, 32) = 32;
  F32(v3, 36) = 32;
  F32(v3, 40) = 0;
  F32(v3, 44) = 0;
  F32(v3, 48) = 2;
  F16(v3, 66) = group;
  F32(v3, 56) = 0;
  v8 = SYS32(2956);
  W_slot_entries_free((intptr_t)(*(ar_sprite_slot **)&SYSB(2956)));
  F32(v8, 28) = a1;
  F16(v8, 4) = 1;
  v9 = (intptr_t)g_eng.op_new(8u);
  v10 = 0;
  v6 = F16(v8, 4) == 0;
  F32(v8, 0) = v9;
  if ( !v6 )
  {
    do
    {
      v11 = 8 * (uint16_t)v10++;
      F32((F32(v8, 0) + v11), 0) = 0;
      F32(F32(v8, 0) + v11, 4) = 0;
    }
    while ( (uint16_t)v10 < F16(v8, 4) );
  }
  F32(v8, 8) = 0;
  F32(v8, 24) = 0;
  F32(v8, 60) = a3;
  F16(v8, 64) = 1109;
  F32(v8, 32) = 32;
  F32(v8, 36) = 48;
  F32(v8, 40) = 0;
  F32(v8, 44) = 1;
  F32(v8, 48) = 2;
  F16(v8, 66) = group;
  F32(v8, 56) = 0;
  v12 = *(intptr_t ***)&SYSB(10008);
  v13 = *(ar_gdi_slot **)&SYSB(10008);
  F16(SYS32(10008), 8) = group;
  al_579ec0(v13, 1u);
  **v12 = al_579f40(6, 14, 0);
  v14 = *(intptr_t ***)&SYSB(10012);
  v15 = *(ar_gdi_slot **)&SYSB(10012);
  F16(SYS32(10012), 8) = group;
  al_579ec0(v15, 1u);
  **v14 = al_579f40(7, 16, 0);
  v16 = *(intptr_t ***)&SYSB(10016);
  v17 = *(ar_gdi_slot **)&SYSB(10016);
  F16(SYS32(10016), 8) = group;
  al_579ec0(v17, 1u);
  **v16 = al_579f40(7, 18, 0);
  v18 = *(intptr_t ***)&SYSB(10020);
  v19 = *(ar_gdi_slot **)&SYSB(10020);
  F16(SYS32(10020), 8) = group;
  al_579ec0(v19, 1u);
  **v18 = al_579f40(4, 8, 0);
  v20 = *(intptr_t ***)&SYSB(10024);
  v21 = *(ar_gdi_slot **)&SYSB(10024);
  F16(SYS32(10024), 8) = group;
  al_579ec0(v21, 1u);
  **v20 = al_579f40(7, 18, 2);
  v22 = *(intptr_t ***)&SYSB(10028);
  v23 = *(ar_gdi_slot **)&SYSB(10028);
  F16(SYS32(10028), 8) = group;
  al_579ec0(v23, 1u);
  **v22 = al_579f40(10, 20, 0);
  v24 = *(intptr_t ***)&SYSB(10032);
  v25 = *(ar_gdi_slot **)&SYSB(10032);
  F16(SYS32(10032), 8) = group;
  al_579ec0(v25, 1u);
  **v24 = al_579f40(5, 10, 0);
  v26 = *(intptr_t ***)&SYSB(10036);
  v27 = *(ar_gdi_slot **)&SYSB(10036);
  F16(SYS32(10036), 8) = group;
  al_579ec0(v27, 1u);
  **v26 = al_579f40(8, 16, 0);
  al_57a030(*(ar_gdi_slot **)&SYSB(10040), 4, 8, 0, group);
  al_57a1a0(*(ar_gdi_slot **)&SYSB(10060), 1, 0xFFFFFFu, group, 1u);
  al_57a260(*(ar_gdi_slot **)&SYSB(10064), 0xFFFFFFu, group, 1u);
  al_582d10(0xCu, group, 4u, 0x200020u, (int)D(0x605060), 0xB090BFu, 0xFu, 4u, 1u);
  al_582d10(0xAu, group, 4u, 0x200000u, (int)D(0x804030), 0xCFA090u, 0xFu, 4u, 1u);
  al_582d10(0xBu, group, 4u, 0x30u, 4214912, (uint32_t)D(0x8090bf), 0xFu, 4u, 1u);
  return al_582d10(0xDu, group, 4u, 0x300020u, (int)D(0x804060), 0xB090A0u, 0xFu, 4u, 1u);
}

/* 0x579ec0: void __thiscall sub_579EC0(ar_gdi_slot *this, unsigned __int16 capacity) */
void al_579ec0(wl_char *self, uint16_t capacity)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    intptr_t v4 = 0;

  v3 = 0;
  if ( ((ar_gdi_slot *)c)->array != 0 )
  {
    if ( ((ar_gdi_slot *)c)->capacity != 0 )
    {
      do
      {
        v4 = v3;
        if ( ((ar_gdi_slot *)c)->array[v4] != 0 )
        {
          API_DeleteObject((intptr_t)(((ar_gdi_slot *)c)->array[v4]));
          ((ar_gdi_slot *)c)->array[v4] = 0;
        }
        ++v3;
      }
      while ( v3 < ((ar_gdi_slot *)c)->capacity );
    }
    g_eng.op_delete((void *)((ar_gdi_slot *)c)->array);
    ((ar_gdi_slot *)c)->capacity = 0;
    ((ar_gdi_slot *)c)->count = 0;
    ((ar_gdi_slot *)c)->array = 0;
  }
  ((ar_gdi_slot *)c)->array = (ar_gdi_handle *)(intptr_t)g_eng.op_new(4 * capacity);
  ((ar_gdi_slot *)c)->count = 0;
  ((ar_gdi_slot *)c)->capacity = capacity;
}

/* 0x579f40: HFONT __stdcall sub_579F40(LONG a1, LONG a2, int a3) */
int32_t al_579f40(int32_t a1, int32_t a2, int32_t a3)
{

    char * v3 = 0;
    int32_t * v4 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x3c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    LOGFONTA *const lf__f = (LOGFONTA *)(frame_ + 0x0);   /* ebp-0x3c */

  memset(&(*lf__f), 0, sizeof((*lf__f)));
  (*lf__f).lfWidth = a1;
  (*lf__f).lfHeight = a2;
  (*lf__f).lfWeight = 400;
  (*lf__f).lfCharSet = 1;
  switch ( a3 )
  {
    case 0:
    case 1:
    case 2:
      goto LABEL_3;
    case 3:
      v3 = ((char *)D(0x8a28a0));
      goto LABEL_6;
    case 4:
      v3 = ((char *)D(0x8a2898));
      goto LABEL_6;
    case 5:
      (*lf__f).lfItalic = 1;
LABEL_3:
      v3 = ((char *)D(0x8a28b0));
LABEL_6:
      strcpy((*lf__f).lfFaceName, v3);
      break;
    default:
      break;
  }
  v4 = (intptr_t)g_eng.op_new(4u);
  result = API_CreateFontIndirectA((intptr_t)(&(*lf__f)));
  if ( result == 0 )
  {
    strcpy((*lf__f).lfFaceName, D(0x8a9b6c));
    result = API_CreateFontIndirectA((intptr_t)(&(*lf__f)));
  }
  *v4 = result;
  return result;
}

/* 0x57a030: void __thiscall sub_57A030(ar_gdi_slot *this, int width, int height, unsigned int family, unsigned __int16 group) */
void al_57a030(wl_char *self, int32_t width, int32_t height, uint32_t family, uint16_t group)
{
    intptr_t c = (intptr_t)self;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    char * v8 = 0;
    intptr_t v9 = 0;
    intptr_t * groupa = 0;
    _Alignas(8) uint8_t frame_[0x3c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    LOGFONTA *const lf__f = (LOGFONTA *)(frame_ + 0x0);   /* ebp-0x3c */

  ((ar_gdi_slot *)c)->group = group;
  v6 = 0;
  if ( ((ar_gdi_slot *)c)->array != 0 )
  {
    if ( ((ar_gdi_slot *)c)->capacity != 0 )
    {
      do
      {
        v7 = v6;
        if ( ((ar_gdi_slot *)c)->array[v7] != 0 )
        {
          API_DeleteObject((intptr_t)(((ar_gdi_slot *)c)->array[v7]));
          ((ar_gdi_slot *)c)->array[v7] = 0;
        }
        ++v6;
      }
      while ( v6 < ((ar_gdi_slot *)c)->capacity );
    }
    g_eng.op_delete((void *)((ar_gdi_slot *)c)->array);
    ((ar_gdi_slot *)c)->capacity = 0;
    ((ar_gdi_slot *)c)->count = 0;
    ((ar_gdi_slot *)c)->array = 0;
  }
  ((ar_gdi_slot *)c)->array = (ar_gdi_handle *)(intptr_t)g_eng.op_new(4u);
  ((ar_gdi_slot *)c)->count = 0;
  memset(&(*lf__f), 0, sizeof((*lf__f)));
  (*lf__f).lfHeight = height;
  ((ar_gdi_slot *)c)->capacity = 1;
  (*lf__f).lfWidth = width;
  (*lf__f).lfWeight = 400;
  (*lf__f).lfCharSet = 1;
  switch ( family )
  {
    case 0u:
    case 1u:
    case 2u:
      goto LABEL_9;
    case 3u:
      v8 = ((char *)D(0x8a28a0));
      goto LABEL_12;
    case 4u:
      v8 = ((char *)D(0x8a2898));
      goto LABEL_12;
    case 5u:
      (*lf__f).lfItalic = 1;
LABEL_9:
      v8 = ((char *)D(0x8a28b0));
LABEL_12:
      strcpy((*lf__f).lfFaceName, v8);
      break;
    default:
      break;
  }
  groupa = (intptr_t *)(intptr_t)g_eng.op_new(4u);
  v9 = API_CreateFontIndirectA((intptr_t)(&(*lf__f)));
  if ( v9 == 0 )
  {
    strcpy((*lf__f).lfFaceName, D(0x8a9b6c));
    v9 = API_CreateFontIndirectA((intptr_t)(&(*lf__f)));
  }
  *groupa = v9;
  *((ar_gdi_slot *)c)->array = v9;
}

/* 0x57a1a0: void __thiscall sub_57A1A0( ar_gdi_slot *this, int width, unsigned int color, unsigned __int16 group, unsigned __int16 capacity) */
void al_57a1a0(wl_char *self, int32_t width, uint32_t color, uint16_t group, uint16_t capacity)
{
    intptr_t c = (intptr_t)self;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t * v8 = 0;
    intptr_t Pen = 0;

  v6 = 0;
  ((ar_gdi_slot *)c)->group = group;
  if ( ((ar_gdi_slot *)c)->array != 0 )
  {
    if ( ((ar_gdi_slot *)c)->capacity != 0 )
    {
      do
      {
        v7 = v6;
        if ( ((ar_gdi_slot *)c)->array[v7] != 0 )
        {
          API_DeleteObject((intptr_t)(((ar_gdi_slot *)c)->array[v7]));
          ((ar_gdi_slot *)c)->array[v7] = 0;
        }
        ++v6;
      }
      while ( v6 < ((ar_gdi_slot *)c)->capacity );
    }
    g_eng.op_delete((void *)((ar_gdi_slot *)c)->array);
    ((ar_gdi_slot *)c)->capacity = 0;
    ((ar_gdi_slot *)c)->count = 0;
    ((ar_gdi_slot *)c)->array = 0;
  }
  ((ar_gdi_slot *)c)->array = (ar_gdi_handle *)(intptr_t)g_eng.op_new(4 * capacity);
  ((ar_gdi_slot *)c)->count = 0;
  ((ar_gdi_slot *)c)->capacity = capacity;
  if ( capacity != 0 )
  {
    v8 = (intptr_t *)(intptr_t)g_eng.op_new(4u);
    Pen = API_CreatePen((intptr_t)(0), (intptr_t)(width), (intptr_t)(color));
    *v8 = Pen;
    ((ar_gdi_slot *)c)->array[((ar_gdi_slot *)c)->count++] = Pen;
  }
}

/* 0x57a260: void __thiscall sub_57A260(ar_gdi_slot *this, unsigned int color, unsigned __int16 group, unsigned __int16 capacity) */
void al_57a260(wl_char *self, uint32_t color, uint16_t group, uint16_t capacity)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v6 = 0;
    intptr_t * v7 = 0;
    intptr_t v8 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    LOGBRUSH *const plbrush__f = (LOGBRUSH *)(frame_ + 0x0);   /* ebp-0xc */

  ((ar_gdi_slot *)c)->group = group;
  if ( ((ar_gdi_slot *)c)->array != 0 )
  {
    for ( i = 0; i < ((ar_gdi_slot *)c)->capacity; ++i )
    {
      v6 = i;
      if ( ((ar_gdi_slot *)c)->array[v6] != 0 )
      {
        API_DeleteObject((intptr_t)(((ar_gdi_slot *)c)->array[v6]));
        ((ar_gdi_slot *)c)->array[v6] = 0;
      }
    }
    g_eng.op_delete((void *)((ar_gdi_slot *)c)->array);
    ((ar_gdi_slot *)c)->capacity = 0;
    ((ar_gdi_slot *)c)->count = 0;
    ((ar_gdi_slot *)c)->array = 0;
  }
  ((ar_gdi_slot *)c)->array = (ar_gdi_handle *)(intptr_t)g_eng.op_new(4 * capacity);
  ((ar_gdi_slot *)c)->count = 0;
  ((ar_gdi_slot *)c)->capacity = capacity;
  if ( capacity != 0 )
  {
    (*plbrush__f).lbStyle = 0;
    (*plbrush__f).lbColor = color;
    v7 = (intptr_t *)(intptr_t)g_eng.op_new(4u);
    v8 = API_CreateBrushIndirect((intptr_t)(&(*plbrush__f)));
    *v7 = v8;
    ((ar_gdi_slot *)c)->array[((ar_gdi_slot *)c)->count++] = v8;
  }
}

/* 0x582b80: int __thiscall sub_582B80(ar_sprite_slot *this, ar_sprite_slot *dst) */
int32_t al_582b80(wl_char *self, ar_sprite_slot * dst)
{
    intptr_t c = (intptr_t)self;
    void * aux_buf = 0;
    uint16_t i = 0;
    intptr_t v5 = 0;
    ar_sprite_entry * v6 = 0;
    intptr_t v7 = 0;
    int v8 = 0;
    intptr_t v9 = 0;
    char * v10 = 0;
    uint32_t f_38 = 0;
    intptr_t v12 = 0;
    uint32_t j = 0;
    char * v14 = 0;
    char * v15 = 0;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    void * *const zdd__f = (void * *)(frame_ + 0x0);   /* ebp-0x24 */
    void * *const settings__f = (void * *)(frame_ + 0x4);   /* ebp-0x20 */
    uint16_t *const resource_id__f = (uint16_t *)(frame_ + 0x8);   /* ebp-0x1c */
    uint32_t *const width__f = (uint32_t *)(frame_ + 0xc);   /* ebp-0x18 */
    uint32_t *const height__f = (uint32_t *)(frame_ + 0x10);   /* ebp-0x14 */
    uint32_t *const colorkey__f = (uint32_t *)(frame_ + 0x14);   /* ebp-0x10 */
    uint32_t *const scale_flag__f = (uint32_t *)(frame_ + 0x18);   /* ebp-0xc */
    uint32_t *const type__f = (uint32_t *)(frame_ + 0x1c);   /* ebp-0x8 */
    uint16_t *const group__f = (uint16_t *)(frame_ + 0x20);   /* ebp-0x4 */

  (*group__f) = ((ar_sprite_slot *)c)->group;
  (*type__f) = ((ar_sprite_slot *)c)->type;
  (*colorkey__f) = ((ar_sprite_slot *)c)->colorkey;
  (*scale_flag__f) = ((ar_sprite_slot *)c)->scale_flag;
  (*resource_id__f) = ((ar_sprite_slot *)c)->resource_id;
  aux_buf = dst->aux_buf;
  (*height__f) = ((ar_sprite_slot *)c)->height;
  (*width__f) = ((ar_sprite_slot *)c)->width;
  (*settings__f) = ((ar_sprite_slot *)c)->settings;
  (*zdd__f) = ((ar_sprite_slot *)c)->zdd;
  if ( aux_buf != 0 )
  {
    g_eng.op_delete((void *)aux_buf);
    dst->aux_buf = 0;
  }
  if ( dst->entries != 0 )
  {
    for ( i = 0; i < dst->entry_count; ++i )
    {
      v5 = i;
      if ( dst->entries[v5].b != 0 )
      {
        g_eng.op_delete((void *)dst->entries[v5].b);
        dst->entries[v5].b = 0;
      }
    }
    g_eng.op_delete((void *)dst->entries);
    dst->entries = 0;
  }
  dst->zdd = (*zdd__f);
  dst->entry_count = 1;
  v6 = (ar_sprite_entry *)(intptr_t)g_eng.op_new(8u);
  v7 = 0;
  v8 = dst->entry_count == 0;
  dst->entries = v6;
  if ( !v8 )
  {
    do
    {
      v9 = (uint16_t)v7++;
      dst->entries[v9].frames = 0;
      dst->entries[v9].b = 0;
    }
    while ( (uint16_t)v7 < dst->entry_count );
  }
  dst->settings = (*settings__f);
  dst->resource_id = (*resource_id__f);
  dst->width = (*width__f);
  dst->height = (*height__f);
  dst->colorkey = (*colorkey__f);
  dst->f_08 = 0;
  dst->f_18 = 0;
  dst->scale_flag = (*scale_flag__f);
  dst->type = (*type__f);
  dst->group = (*group__f);
  dst->f_38 = 0;
  v10 = (char *)((ar_sprite_slot *)c)->aux_buf;
  if ( v10 != 0 )
  {
    f_38 = ((ar_sprite_slot *)c)->f_38;
    if ( dst->aux_buf == 0 )
    {
      dst->aux_buf = (intptr_t)g_eng.op_new(24 * f_38);
      if ( f_38 != 0 )
      {
        v12 = 0;
        for ( j = f_38; j != 0; --j )
        {
          v14 = &v10[v12];
          v15 = (char *)dst->aux_buf + v12;
          v12 += 24;
          qmemcpy(v15, v14, 0x18u);
        }
      }
    }
  }
  return 1;
}

/* 0x582d00: void __thiscall sub_582D00(ar_info_entry *this) */
void al_582d00(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  ((ar_info_entry *)c)->marker = 0;
  ((ar_info_entry *)c)->flag = 0;
  ((ar_info_entry *)c)->data = 0;
  ((ar_info_entry *)c)->palette = 0;
}

/* 0x582d10: HPEN __stdcall sub_582D10( unsigned __int16 a1, uint16_t a2, uint16_t a3, COLORREF color, int a5, COLORREF a6, unsigned __int16 a7, unsigned __int16 a8, unsigned __int16 a9) */
int32_t al_582d10(uint16_t a1, uint16_t a2, uint16_t a3, uint32_t color, int32_t a5, uint32_t a6, uint16_t a7, uint16_t a8, uint16_t a9)
{

    ar_gdi_slot * v9 = 0;
    intptr_t v10 = 0;
    intptr_t v12 = 0;
    intptr_t * v13 = 0;
    intptr_t Pen = 0;
    intptr_t v15 = 0;
    intptr_t result = 0;
    uint32_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t * v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    int32_t * v22 = 0;
    intptr_t v23 = 0;
    uint16_t v24 = 0;
    intptr_t v25 = 0;

  v9 = *(ar_gdi_slot **)((uint8_t *)g_eng.sys + (4 * a1 + 10004));
  v10 = 4 * a1 + 9081460;
  v23 = v10;
  v9->group = a2;
  W_tm_562a10((intptr_t)(v9));
  v12 = a3;
  v25 = v12;
  v9->array = (ar_gdi_handle *)(intptr_t)g_eng.op_new(4 * v12);
  v9->count = 0;
  v9->capacity = a3;
  if ( a3 != 0 )
  {
    v13 = (intptr_t *)(intptr_t)g_eng.op_new(4u);
    Pen = API_CreatePen((intptr_t)(0), (intptr_t)(a7), (intptr_t)(color));
    *v13 = Pen;
    v9->array[v9->count++] = Pen;
  }
  v15 = 1;
  result = (intptr_t)(v12 - 1);
  v24 = 1;
  if ( v12 - 1 > 1 )
  {
    do
    {
      v17 = W_wm_5b5f50((intptr_t)(color), (intptr_t)(a5), (intptr_t)(v15), (intptr_t)(v12));
      v18 = F32(v23, 0);
      if ( F16(F32(v23, 0), 6) > F16(F32(v23, 0), 4) )
      {
        v19 = (intptr_t *)(intptr_t)g_eng.op_new(4u);
        v20 = API_CreatePen((intptr_t)(0), (intptr_t)(a7 - v15 * a8), (intptr_t)(v17));
        *v19 = v20;
        v12 = v25;
        F32((F32(v18, 0) + 4 * (uint16_t)(F16(v18, 4))++), 0) = v20;
      }
      result = (intptr_t)(v12 - 1);
      v15 = ++v24;
    }
    while ( v24 < v12 - 1 );
    v10 = v23;
  }
  v21 = F32(v10, 0);
  if ( F16(F32(v10, 0), 6) > F16(F32(v10, 0), 4) )
  {
    v22 = (intptr_t)g_eng.op_new(4u);
    result = API_CreatePen((intptr_t)(0), (intptr_t)(a9), (intptr_t)(a6));
    *v22 = result;
    F32((F32(v21, 0) + 4 * (uint16_t)(F16(v21, 4))++), 0) = result;
  }
  return result;
}

/* 0x583c90: FILE *__thiscall sub_583C90(cls_583C90 *this, FILE *a2) */
int32_t al_583c90(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t * v5 = 0;
    intptr_t i = 0;
    intptr_t v7 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x0);   /* ebp-0x10 */

  result = F32(c, 0x8);
  if ( result != 0 )
  {
    result = a2;
    if ( a2 != 0 )
    {
      if ( SYS32(10360) != 0 )
      {
        if ( F32(D(0x8a6b54), 0) == 0 )
        {
          API_OutputDebugStringA((intptr_t)(((char *)D(0x8a2a74))));
          API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
          API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
          v3 = CRT_fopen(((char *)D(0x8a6620) + 0x404), ((char *)D(0x85456c)));
          if ( v3 != 0 )
          {
            API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
            CRT_fprintf(
              v3,
              "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
              (*SystemTime__f).wYear,
              (*SystemTime__f).wMonth,
              (*SystemTime__f).wDay,
              (*SystemTime__f).wHour,
              (*SystemTime__f).wMinute,
              (*SystemTime__f).wSecond,
              ((char *)D(0x8a2a74)),
              D(0x8a9b6c));
            CRT_fclose(v3);
          }
        }
        W_id_5ba290((intptr_t)(*(intptr_t *)&SYSB(10360)));
        if ( F32(D(0x8a6b54), 0) == 0 )
        {
          API_OutputDebugStringA((intptr_t)(((char *)D(0x8a2a5c))));
          API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
          API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
          v4 = CRT_fopen(((char *)D(0x8a6620) + 0x404), ((char *)D(0x85456c)));
          if ( v4 != 0 )
          {
            API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
            CRT_fprintf(
              v4,
              "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
              (*SystemTime__f).wYear,
              (*SystemTime__f).wMonth,
              (*SystemTime__f).wDay,
              (*SystemTime__f).wHour,
              (*SystemTime__f).wMinute,
              (*SystemTime__f).wSecond,
              ((char *)D(0x8a2a5c)),
              D(0x8a9b6c));
            CRT_fclose(v4);
          }
        }
      }
      v5 = (intptr_t *)&SYSB(10364);
      for ( i = 2; i != 0; --i )
      {
        if ( *v5 != 0 )
          W_id_5ba290((intptr_t)(*v5));
        ++v5;
      }
      result = F32(D(0x8a6b54), 0);
      if ( F32(D(0x8a6b54), 0) == 0 )
      {
        API_OutputDebugStringA((intptr_t)(((char *)D(0x8a2a44))));
        API_OutputDebugStringA((intptr_t)(D(0x8a9b6c)));
        API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
        result = CRT_fopen(((char *)D(0x8a6620) + 0x404), ((char *)D(0x85456c)));
        v7 = result;
        if ( result != 0 )
        {
          API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
          CRT_fprintf(
            v7,
            "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
            (*SystemTime__f).wYear,
            (*SystemTime__f).wMonth,
            (*SystemTime__f).wDay,
            (*SystemTime__f).wHour,
            (*SystemTime__f).wMinute,
            (*SystemTime__f).wSecond,
            ((char *)D(0x8a2a44)),
            D(0x8a9b6c));
          return CRT_fclose(v7);
        }
      }
    }
  }
  return result;
}

/* 0x583ee0: int sub_583EE0() */
int32_t al_583ee0(void)
{

    struct tm * v0 = 0;
    intptr_t tm_mon = 0;
    intptr_t tm_mday = 0;
    uint16_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x4c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v9 = (intptr_t *)(frame_ + 0xc);   /* [16] at ebp-0x40 */
    intptr_t *const Time__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4c */
    intptr_t *const v7__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x48 */
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x44 */

  CRT_time(&(*Time__f));
  v0 = CRT_localtime(&(*Time__f));
  tm_mon = v0->tm_mon;
  tm_mday = v0->tm_mday;
  v9[2] = 3;
  v9[3] = 3;
  v9[8] = 12;
  v9[11] = 12;
  v3 = 0;
  v4 = tm_mon + 1;
  (*v7__f) = 201901;
  (*v8__f) = 2;
  v9[0] = 14;
  v9[1] = 201903;
  v9[4] = 201904;
  v9[5] = 10;
  v9[6] = 31;
  v9[7] = 201902;
  v9[9] = 24;
  v9[10] = 201902;
  v9[12] = 25;
  memset(&v9[13], 0, 12);
  result = 0;
  while ( v4 != F32(((char *)&v9[-1] + result), 0) || tm_mday != F32(((uint8_t *)v9 + result), 0) )
  {
    result = 12 * ++v3;
    if ( F32(((char *)&(*v7__f) + result), 0) == 0 )
    {
      F32(D(0x8a940c), 0) = 0;
      return result;
    }
  }
  result = *(&(*v7__f) + 3 * v3);
  F32(D(0x8a940c), 0) = result;
  return result;
}

/* 0x563ef0: int __thiscall sub_563EF0( ar_sound_slot *this, void *zds, void *settings, unsigned __int16 resource_id, unsigned __int16 count, unsigned __int16 group, int load_flag) */
int32_t al_563ef0(wl_char *self, void * zds, void * settings, uint16_t resource_id, uint16_t count, uint16_t group, int32_t load_flag)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    void * v9 = 0;
    uint32_t v10 = 0;
    uint16_t v11 = 0;
    uint16_t v12 = 0;
    intptr_t v13 = 0;
    _Alignas(8) uint8_t frame_[0x110 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *OutputString = (char *)(frame_ + 0x10);   /* [256] at ebp-0x100 */
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x0);   /* ebp-0x110 */

  result = count;
  ((ar_sound_slot *)c)->group = group;
  ((ar_sound_slot *)c)->resource_id = resource_id;
  ((ar_sound_slot *)c)->settings = settings;
  ((ar_sound_slot *)c)->zds = zds;
  *(_DWORD *)&((ar_sound_slot *)c)->count = count;
  if ( load_flag != 0 && zds != 0 && ((ar_sound_slot *)c)->buffer == 0 )
  {
    v9 = (intptr_t)g_eng.op_new(8 * count);
    v10 = 8 * ((ar_sound_slot *)c)->count;
    ((ar_sound_slot *)c)->buffer = v9;
    memset(v9, 0, v10);
    result = W_sd_5bb250((intptr_t)(((ar_sound_slot *)c)->zds), (intptr_t)((_DWORD *)((ar_sound_slot *)c)->buffer), (intptr_t)((intptr_t)((ar_sound_slot *)c)->settings), (intptr_t)(((ar_sound_slot *)c)->resource_id), (intptr_t)(0), (intptr_t)(0));
    if ( result != 0 )
    {
      v11 = 1;
      if ( ((ar_sound_slot *)c)->count <= 1u )
        return result;
      while ( 1 )
      {
        result = W_sd_5bb2f0((intptr_t)(((ar_sound_slot *)c)->zds), (intptr_t)((_DWORD *)((ar_sound_slot *)c)->buffer + 2 * v11), (intptr_t)(F32(((ar_sound_slot *)c)->buffer, 0)));
        if ( result == 0 )
          break;
        if ( ++v11 >= ((ar_sound_slot *)c)->count )
          return result;
      }
    }
    g_eng.op_delete((void *)((ar_sound_slot *)c)->buffer);
    v12 = ((ar_sound_slot *)c)->resource_id;
    ((ar_sound_slot *)c)->buffer = 0;
    g_eng.h5bf3ee(OutputString, "%d", v12);
    API_OutputDebugStringA((intptr_t)(((char *)D(0x854620))));
    API_OutputDebugStringA((intptr_t)(OutputString));
    API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
    result = (int)CRT_fopen(((char *)D(0x8a9530) + 0x404), ((char *)D(0x85456c)));
    v13 = result;
    if ( result != 0 )
    {
      API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
      CRT_fprintf(
        v13,
        "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
        (*SystemTime__f).wYear,
        (*SystemTime__f).wMonth,
        (*SystemTime__f).wDay,
        (*SystemTime__f).wHour,
        (*SystemTime__f).wMinute,
        (*SystemTime__f).wSecond,
        ((char *)D(0x854620)),
        OutputString);
      return CRT_fclose(v13);
    }
  }
  return result;
}
/* unported callees: 0x560900 0x5b8fc0 */
