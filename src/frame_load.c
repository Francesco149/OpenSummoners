/* frame_load.c — the sprite-frame LOADERS and the BITMAP SESSION under them: a sprite slot's palette read 0x4178e0,
 * the frame load 0x4184a0 (free the slot's old frames, the resource bitmap through the bitmap session, the 24-bit
 * tint, then the cut) and the frame cut 0x4188b0 (cells → per-cell trim boxes 0x5b6f80, the depth conversion to the
 * display's 8 / 16 / 24 / 32 bits, one zdd surface per cell 0x5b9280 → 0x5b9630 → 0x5b9910); the bitmap session
 * (the DIB in a LocalAlloc block + its BITMAPINFOHEADER and palette: resource / file loads 0x5b7800 / 0x5b7970 with
 * the scrambled header 0x5b7c10, the depth converters 0x5b7270 / 0x5b7310 / 0x5b74f0, the BMP writer 0x5b7620 +
 * 0x5b7cd0, the getters), the screenshot 0x499850 ("fs_s%d.bmp").  See frame_load.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals).  Hand finish: 0x5b7310's 24-bit loop (IDA reads
 * the scratch ebx as an argument and garbles the loop; the asm's per-pixel shifts instead).  Scripted:
 * tools/ida/finish/frame_load.py.  Unreached in the corpus (compile-checked only): the screenshot 0x499850 + the BMP
 * writer 0x5b7620 / 0x5b7cd0 (retail writes `&Block` — the pointer's own stack bytes — for the scrambled body: kept),
 * the file load 0x5b7970's scrambled branch, the 24 / 32-bit converters 0x5b7270 / 0x5b74f0, the StretchDIBits 0x5b6f30. */
#include "frame_load.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include <windows.h>
#include "asset_register.h"
#include "bitmap_session.h"
#include "render_platform.h"
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

static inline intptr_t W_rp_5b9350(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b9350((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_rp_5b9390(intptr_t a0, ...) { return (intptr_t)rp_5b9390((wl_char *)a0); }
static inline intptr_t W_rp_5b9490(intptr_t a0, ...) { return (intptr_t)rp_5b9490((wl_char *)a0); }
static inline intptr_t W_rp_5b94d0(intptr_t a0, ...) { return (intptr_t)rp_5b94d0((wl_char *)a0); }
static inline intptr_t W_rp_5b94e0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)rp_5b94e0((wl_char *)a0, (intptr_t *)a1); }
static inline intptr_t W_rp_5b9500(intptr_t a0, intptr_t a1, ...) { rp_5b9500((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_rp_5b95c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)rp_5b95c0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_rp_5b97e0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)rp_5b97e0((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_rp_5b98c0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, ...) { return (intptr_t)rp_5b98c0((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6); }

typedef intptr_t (__attribute__((stdcall)) *api_BitBlt_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_BitBlt ((api_BitBlt_t)g_eng.api("BitBlt"))
typedef intptr_t (__attribute__((stdcall)) *api_CloseHandle_t)(intptr_t);
#define API_CloseHandle ((api_CloseHandle_t)g_eng.api("CloseHandle"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateCompatibleDC_t)(intptr_t);
#define API_CreateCompatibleDC ((api_CreateCompatibleDC_t)g_eng.api("CreateCompatibleDC"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateDIBitmap_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_CreateDIBitmap ((api_CreateDIBitmap_t)g_eng.api("CreateDIBitmap"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateFileA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_CreateFileA ((api_CreateFileA_t)g_eng.api("CreateFileA"))
typedef intptr_t (__attribute__((stdcall)) *api_DeleteDC_t)(intptr_t);
#define API_DeleteDC ((api_DeleteDC_t)g_eng.api("DeleteDC"))
typedef intptr_t (__attribute__((stdcall)) *api_DeleteObject_t)(intptr_t);
#define API_DeleteObject ((api_DeleteObject_t)g_eng.api("DeleteObject"))
typedef intptr_t (__attribute__((stdcall)) *api_FindResourceA_t)(intptr_t, intptr_t, intptr_t);
#define API_FindResourceA ((api_FindResourceA_t)g_eng.api("FindResourceA"))
typedef intptr_t (__attribute__((stdcall)) *api_FormatMessageA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_FormatMessageA ((api_FormatMessageA_t)g_eng.api("FormatMessageA"))
typedef intptr_t (__attribute__((stdcall)) *api_GetDC_t)(intptr_t);
#define API_GetDC ((api_GetDC_t)g_eng.api("GetDC"))
typedef intptr_t (__attribute__((stdcall)) *api_GetDIBits_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_GetDIBits ((api_GetDIBits_t)g_eng.api("GetDIBits"))
typedef intptr_t (__attribute__((stdcall)) *api_GetLastError_t)(void);
#define API_GetLastError ((api_GetLastError_t)g_eng.api("GetLastError"))
typedef intptr_t (__attribute__((stdcall)) *api_LoadResource_t)(intptr_t, intptr_t);
#define API_LoadResource ((api_LoadResource_t)g_eng.api("LoadResource"))
typedef intptr_t (__attribute__((stdcall)) *api_LocalAlloc_t)(intptr_t, intptr_t);
#define API_LocalAlloc ((api_LocalAlloc_t)g_eng.api("LocalAlloc"))
typedef intptr_t (__attribute__((stdcall)) *api_LocalFree_t)(intptr_t);
#define API_LocalFree ((api_LocalFree_t)g_eng.api("LocalFree"))
typedef intptr_t (__attribute__((stdcall)) *api_LockResource_t)(intptr_t);
#define API_LockResource ((api_LockResource_t)g_eng.api("LockResource"))
typedef intptr_t (__attribute__((stdcall)) *api_ReadFile_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_ReadFile ((api_ReadFile_t)g_eng.api("ReadFile"))
typedef intptr_t (__attribute__((stdcall)) *api_ReleaseDC_t)(intptr_t, intptr_t);
#define API_ReleaseDC ((api_ReleaseDC_t)g_eng.api("ReleaseDC"))
typedef intptr_t (__attribute__((stdcall)) *api_SelectObject_t)(intptr_t, intptr_t);
#define API_SelectObject ((api_SelectObject_t)g_eng.api("SelectObject"))
typedef intptr_t (__attribute__((stdcall)) *api_StretchDIBits_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_StretchDIBits ((api_StretchDIBits_t)g_eng.api("StretchDIBits"))
typedef intptr_t (__attribute__((stdcall)) *api_WriteFile_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_WriteFile ((api_WriteFile_t)g_eng.api("WriteFile"))
#define CRT_time ((intptr_t (*)())g_eng.retail(0x5bf6dfu))   /* retail's CRT time (cdecl) */
int32_t fl_4178e0(wl_char *self, void * out_palette);
void fl_4184a0(wl_char *self, int32_t a2);
int32_t fl_4188b0(wl_char *self, uint16_t a2, bitmap_session * a3, int32_t a4, int32_t a5, int32_t a6);
void fl_499850(int32_t a1);
int32_t bs_5b6e40(int32_t a1, uint32_t a2);
void bs_5b6e70(wl_char *self);
void bs_5b6e80(wl_char *self);
void bs_5b6e90(wl_char *self);
int32_t bs_5b6eb0(wl_char *self);
int32_t bs_5b6ec0(wl_char *self);
int32_t bs_5b6ee0(wl_char *self);
int32_t bs_5b6ef0(wl_char *self);
int32_t bs_5b6f00(wl_char *self);
void bs_5b6f10(wl_char *self, uint16_t bit_count);
int32_t bs_5b6f30(wl_char *self, int32_t hdc, int32_t xDest, int32_t yDest, int32_t DestWidth, int32_t DestHeight, int32_t xSrc, int32_t a8, int32_t SrcWidth, int32_t SrcHeight);
int32_t bs_5b6f80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t * a7);
int32_t bs_5b71f0(wl_char *self, int32_t width, int32_t height, uint16_t bit_count);
int32_t bs_5b7270(wl_char *self);
int32_t bs_5b7310(wl_char *self, int32_t a2, uint8_t * a3, int32_t a4, int32_t a5);
int32_t bs_5b74f0(wl_char *self, int32_t a2, uint32_t a3);
int32_t bs_5b7620(wl_char *self, char * lpFileName, int32_t a3, int32_t a4);
int32_t bs_5b7800(wl_char *self, void * hModule, uint32_t resource_id, void * resource_type, int32_t compressed_flag);
int32_t bs_5b7970(wl_char *self, char * lpFileName, int32_t a3, int32_t a4);
void bs_5b7b90(wl_char *self, void * dest_palette);
int32_t bs_5b7bd0(wl_char *self, int32_t a2);
int32_t bs_5b7c10(int32_t a1, int32_t a2, uint32_t * a3);
int32_t bs_5b7cd0(int32_t a1, int32_t * a2, int32_t * a3);
int32_t bs_5b9280(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11);
int32_t bs_5b9630(wl_char *self, bitmap_session * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t * a11);
int32_t bs_5b9910(wl_char *self, bitmap_session * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8);
int32_t bs_5ba390(wl_char *self);

/* 0x4178e0: char __thiscall sub_4178E0(ar_sprite_slot *this, void *out_palette) */
int32_t fl_4178e0(wl_char *self, void * out_palette)
{
    intptr_t c = (intptr_t)self;
    uint32_t v3 = 0;
    char v4 = 0;
    intptr_t v6 = 0;
    _Alignas(8) uint8_t frame_[0x45c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    void * *const settings__f = (void * *)(frame_ + 0x0);   /* ebp-0x45c */
    bitmap_session *const v8__f = (bitmap_session *)(frame_ + 0x18);   /* ebp-0x444 */
    intptr_t *const v9__f = (intptr_t *)(frame_ + 0x458);   /* ebp-0x4 */

  bs_5b6e70(&(*v8__f));
  SETLO(v3, ((ar_sprite_slot *)c)->resource_id);
  v4 = 0;
  (*settings__f) = ((ar_sprite_slot *)c)->settings;
  (*v9__f) = 0;
  if ( bs_5b7800(&(*v8__f), (*settings__f), v3, ((char *)D(0x854c98)), 1) != 0 )
  {
    SETLO(v6, bs_5b6f00(&(*v8__f)));
    if ( v6 == 8 )
    {
      bs_5b7b90(&(*v8__f), out_palette);
      v4 = 1;
    }
    bs_5b6e90(&(*v8__f));
    (*v9__f) = -1;
    bs_5b6e80(&(*v8__f));
    return v4;
  }
  else
  {
    (*v9__f) = -1;
    bs_5b6e80(&(*v8__f));
    return 0;
  }
}

/* 0x4184a0: void __thiscall sub_4184A0(cls_4184A0 *this, int a2) */
void fl_4184a0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    uint32_t v3 = 0;
    intptr_t v4 = 0;
    void ** v5 = 0;
    uint32_t i = 0;
    void * v7 = 0;
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
    uint8_t * v18 = 0;
    intptr_t v19 = 0;
    uint32_t LastError = 0;
    uint32_t v21 = 0;
    uint32_t v22 = 0;
    char * v23 = 0;
    char * v24 = 0;
    char * v25 = 0;
    char v26 = 0;
    uint32_t v27 = 0;
    _Alignas(8) uint8_t frame_[0x758 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v34 = (char *)(frame_ + 0x44c);   /* [256] at ebp-0x30c */
    char *Buffer = (char *)(frame_ + 0x54c);   /* [256] at ebp-0x20c */
    char *OutputString = (char *)(frame_ + 0x64c);   /* [256] at ebp-0x10c */
    intptr_t *const v28__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x758 */
    intptr_t *const j__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x754 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x74c */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x748 */
    bitmap_session *const v33__f = (bitmap_session *)(frame_ + 0x14);   /* ebp-0x744 */
    intptr_t *const v37__f = (intptr_t *)(frame_ + 0x754);   /* ebp-0x4 */

  v2 = c;
  bs_5b6e70(&(*v33__f));
  v4 = F32(((uint8_t *)(v2) + 0x0), 0);
  (*v37__f) = 0;
  if ( v4 != 0 )
  {
    v3 = (uint16_t)a2;
    v5 = (void **)(v4 + 8 * (uint16_t)a2);
    if ( *v5 != 0 )
    {
      for ( i = 0; i < *(_DWORD *)&GAP((v2 + (3) * 0x10), 0x0, 8); *((_DWORD *)*v5 + i - 1) = 0 )
      {
        v7 = *((void **)*v5 + i);
        if ( v7 != 0 )
        {
          W_rp_5b9390((intptr_t)(*((intptr_t *)*v5 + i)));
          g_eng.op_delete((void *)v7);
        }
        ++i;
      }
      g_eng.op_delete((void *)*v5);
      *v5 = 0;
    }
  }
  SETLO(v3, *(_WORD *)((uint8_t *)(v2 + (4) * 0x10) + 0x0));
  if ( bs_5b7800(&(*v33__f), (void *)F32(v2 + (3) * 0x10, 0xc), v3, ((char *)D(0x854c98)), 1) == 0 )
    goto LABEL_25;
  v8 = 8 * (uint16_t)a2;
  if ( F32(v8 + F32(((uint8_t *)(v2) + 0x0), 0), 4) != 0 )
  {
    SETLO(v9, bs_5b6f00(&(*v33__f)));
    if ( v9 == 8 )
      bs_5b7bd0(&(*v33__f), F32(v8 + F32(((uint8_t *)(v2) + 0x0), 0), 4));
  }
  if ( *(_DWORD *)&GAP(v2, 0x0, 8) != 0 )
  {
    SETLO(v10, bs_5b6f00(&(*v33__f)));
    if ( v10 == 24 )
    {
      v11 = *(_DWORD *)&GAP((v2 + (1) * 0x10), 0x0, 8);
      v12 = *(_DWORD *)&GAP((v2 + (1) * 0x10), 0x0, 4);
      v13 = *(_DWORD *)((uint8_t *)(v2 + (1) * 0x10) + 0x0);
      (*v32__f) = v11;
      (*v31__f) = F32(c, 0xc);
      (*v28__f) = 0;
      if ( bs_5b6ee0(&(*v33__f)) > 0 )
      {
        v14 = 0;
        do
        {
          v15 = v14 * bs_5b6ef0(&(*v33__f));
          v16 = 0;
          v17 = bs_5b6eb0(&(*v33__f)) + v15;
          for ( (*j__f) = 0; (uint16_t)(*j__f) < bs_5ba390(&(*v33__f)); v16 = (uint16_t)(*j__f) )
          {
            v18 = (uint8_t *)(v16 + v17 + 2 * v16);
            if ( (F32(v18, 0) & 0xFFFFFF) != 0xFF00FF )
            {
              if ( (*v32__f) != 0 )
              {
                *v18 = F8((*v18 + (*v32__f)), 0);
                v18[1] = F8((v18[1] + (*v32__f)), 0);
                v18[2] = F8((v18[2] + (*v32__f)), 0);
              }
              *v18 = v12 * *v18 / 1000;
              v18[1] = v13 * v18[1] / 1000;
              v18[2] = (*v31__f) * v18[2] / 1000;
            }
            ++(*j__f);
          }
          v14 = (uint16_t)++(*v28__f);
        }
        while ( (uint16_t)(*v28__f) < bs_5b6ee0(&(*v33__f)) );
      }
      v2 = c;
    }
  }
  if ( fl_4188b0(
         v2,
         a2,
         &(*v33__f),
         *(_DWORD *)((uint8_t *)(v2 + (2) * 0x10) + 0x0),
         *(_DWORD *)&GAP((v2 + (2) * 0x10), 0x0, 4),
         *(_DWORD *)&GAP((v2 + (2) * 0x10), 0x0, 8)) == 0 )
  {
LABEL_25:
    v19 = *(uint16_t *)((uint8_t *)(v2 + (4) * 0x10) + 0x0);
    *(_DWORD *)&GAP((v2 + (3) * 0x10), 0x0, 8) = 0;
    g_eng.h5bf3ee(v34, "%d", v19);
    LastError = API_GetLastError();
    v21 = LastError;
    if ( LastError != 0 )
    {
      API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0xFFu), (intptr_t)(0));
      g_eng.h5bf3ee(OutputString, "[%08d] %s", v21, Buffer);
      g_eng.log_fail_406440((const char *)D(0x8a9530), OutputString, D(0x8a9b6c));
    }
    F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
    if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
    {
      strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
      v22 = strlen(((char *)D(0x854ca0))) + 1;
      v23 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
      qmemcpy(v23, ((char *)D(0x854ca0)), 4 * (v22 >> 2));
      v25 = ((char *)D(0x854ca0) + (4 * (v22 >> 2)));
      v24 = &v23[4 * (v22 >> 2)];
      v26 = v22;
    }
    else
    {
      v27 = strlen(((char *)D(0x854ca0))) + 1;
      qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854ca0)), 4 * (v27 >> 2));
      v25 = ((char *)D(0x854ca0) + (4 * (v27 >> 2)));
      v24 = &((char *)D(0x8a9530) + 0x404)[4 * (v27 >> 2) - 1024];
      v26 = v27;
    }
    qmemcpy(v24, v25, v26 & 3);
    strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], v34);
    g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x854ca0)), v34);
    g_eng.h5bf5db(0);
  }
  bs_5b6e90(&(*v33__f));
  (*v37__f) = -1;
  bs_5b6e80(&(*v33__f));
}

/* 0x4188b0: int __thiscall sub_4188B0(cls_4188B0 *this, unsigned __int16 a2, bitmap_session *a3, int a4, int a5, int a6) */
int32_t fl_4188b0(wl_char *self, uint16_t a2, bitmap_session * a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    bitmap_session * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v12 = 0;
    intptr_t i = 0;
    int v14 = 0;
    intptr_t v15 = 0;
    intptr_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    intptr_t v22 = 0;
    uint16_t v23 = 0;
    intptr_t v24 = 0;
    intptr_t v25 = 0;
    intptr_t v26 = 0;
    intptr_t v34 = 0;
    intptr_t j = 0;
    bitmap_session * v36 = 0;
    _Alignas(8) uint8_t frame_[0x18 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x18 */
    int32_t * *const v28__f = (int32_t * *)(frame_ + 0x4);   /* ebp-0x14 */
    intptr_t *const v30__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const v29__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x10 */
    intptr_t *const v31__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0xc */
    intptr_t *const v32__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x4 */

  v7 = a3;
  (*v31__f) = 1;
  (*v28__f) = (_DWORD *)(F32(c, 0x0) + 8 * a2);
  if ( a4 == 0 )
    a4 = bs_5ba390(a3);
  if ( a5 == 0 )
    a5 = bs_5b6ee0(a3);
  v8 = bs_5ba390(a3) / a4;
  (*v27__f) = v8;
  v9 = bs_5b6ee0(a3) / a5;
  v10 = v8 * v9;
  (*v32__f) = v9;
  if ( v8 * v9 == 0 )
    return 0;
  F32(c, 0x38) = v10;
  *(*v28__f) = (intptr_t)g_eng.op_new(4 * v10);
  if ( F32(c, 0x34) == 0 && F32(c, 0x30) > 0 && a6 != 0x1FFFFFF )
  {
    v8 = 0;
    F32(c, 0x34) = (int)(intptr_t)g_eng.op_new(24 * F32(c, 0x38));
    if ( v9 != 0 )
    {
      v34 = 0;
      (*v29__f) = v9;
      do
      {
        if ( (*v27__f) != 0 )
        {
          v12 = 0;
          for ( i = (*v27__f); i != 0; --i )
          {
            bs_5b6f80(a3, a6, v12, v34, a4, a5, (int *)(F32(c, 0x34) + 24 * (uint16_t)v8++));
            v12 += a4;
          }
          v7 = a3;
        }
        v14 = (*v29__f) == 1;
        v34 += a5;
        --(*v29__f);
      }
      while ( !v14 );
    }
  }
  v15 = F32(c, 0x1c);
  v16 = F32(v15, 360);
  if ( F32(c, 0x28) != 0x1FFFFFF )
  {
    switch ( v16 )
    {
      case 8:
        bs_5b6f00(v7);
        goto LABEL_42;
      case 16:
        SETLO(v20, bs_5b6f00(v7));
        if ( v20 == 8 || v20 == 24 )
          bs_5b7310(v7, v8, (_BYTE *)v15, a6, 16711935);
        goto LABEL_42;
      case 24:
        SETLO(v21, bs_5b6f00(v7));
        if ( v21 == 8 )
          bs_5b74f0(v7, a6, 0xFF00FFu);
        goto LABEL_42;
      case 32:
        SETLO(v22, bs_5b6f00(v7));
        if ( v22 == 8 )
        {
          bs_5b74f0(v7, a6, 0xFF00FFu);
        }
        else if ( v22 != 24 )
        {
          goto LABEL_42;
        }
        bs_5b7270(v7);
LABEL_42:
        a6 = 16711935;
        goto LABEL_43;
      default:
        goto LABEL_42;
    }
  }
  switch ( v16 )
  {
    case 8:
      bs_5b6f00(v7);
      break;
    case 16:
      SETLO(v17, bs_5b6f00(v7));
      if ( v17 == 8 || v17 == 24 )
        bs_5b7310(v7, v8, (_BYTE *)v15, a6, 0);
      break;
    case 24:
      SETLO(v18, bs_5b6f00(v7));
      if ( v18 == 8 )
        bs_5b74f0(v7, a6, 0);
      break;
    case 32:
      SETLO(v19, bs_5b6f00(v7));
      if ( v19 == 8 )
      {
        bs_5b74f0(v7, a6, 0);
      }
      else if ( v19 != 24 )
      {
        break;
      }
      bs_5b7270(v7);
      break;
    default:
      break;
  }
LABEL_43:
  v23 = 0;
  if ( (*v32__f) != 0 )
  {
    v36 = 0;
    (*v30__f) = (*v32__f);
    do
    {
      if ( (*v27__f) != 0 )
      {
        v24 = 0;
        for ( j = (*v27__f); j != 0; --j )
        {
          v25 = F32(c, 0x34);
          if ( v25 != 0 )
            v26 = v25 + 24 * v23;
          else
            v26 = 0;
          if ( bs_5b9280(
                 F32(c, 0x1c),
                 &(*v33__f),
                 (int)v7,
                 v24,
                 (int)v36,
                 a4,
                 a5,
                 a6,
                 F32(c, 0x2c),
                 F32(c, 0x30),
                 v26) != 0 )
          {
            F32((*(*v28__f) + 4 * v23), 0) = (*v33__f);
          }
          else
          {
            (*v31__f) = 0;
            F32((*(*v28__f) + 4 * v23), 0) = 0;
          }
          ++v23;
          v24 += a4;
        }
      }
      v14 = (*v30__f) == 1;
      v36 = (bitmap_session *)((uint8_t *)v36 + a5);
      --(*v30__f);
    }
    while ( !v14 );
  }
  return (*v31__f);
}

/* 0x499850: void __stdcall sub_499850(int a1) */
void fl_499850(int32_t a1)
{

    intptr_t DC = 0;
    _Alignas(8) uint8_t frame_[0x84c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *FileName = (char *)(frame_ + 0x440);   /* [512] at ebp-0x40c */
    char *Buffer = (char *)(frame_ + 0x640);   /* [512] at ebp-0x20c */
    intptr_t *const hdcSrc__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x84c */
    intptr_t *const Time__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x848 */
    bitmap_session *const pjBits__f = (bitmap_session *)(frame_ + 0x8);   /* ebp-0x844 */
    intptr_t *const hdc__f = (intptr_t *)(frame_ + 0x43c);   /* ebp-0x410 */
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0x848);   /* ebp-0x4 */

  bs_5b6e70(&(*pjBits__f));
  (*v8__f) = 0;
  CRT_time(&(*Time__f));
  bs_5b71f0(&(*pjBits__f), 640, 480, 0x18u);
  (*hdc__f) = API_CreateCompatibleDC((intptr_t)(0));
  if ( (*hdc__f) != 0 )
  {
    DC = API_GetDC((intptr_t)(0));
    if ( DC != 0 )
    {
      (*pjBits__f).f_08 = (uint32_t)API_CreateDIBitmap((intptr_t)(DC), (intptr_t)((const BITMAPINFOHEADER *)&(*pjBits__f).biSize), (intptr_t)(4u), (intptr_t)((*pjBits__f).pixels), (intptr_t)((const BITMAPINFO *)&(*pjBits__f).biSize), (intptr_t)(0));
      if ( (*pjBits__f).f_08 != 0 )
      {
        API_ReleaseDC((intptr_t)(0), (intptr_t)(DC));
        API_SelectObject((intptr_t)((*hdc__f)), (intptr_t)((intptr_t)(*pjBits__f).f_08));
      }
    }
  }
  W_rp_5b94e0((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t *)&(*hdcSrc__f)));
  API_BitBlt((intptr_t)((*hdc__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(640), (intptr_t)(480), (intptr_t)((*hdcSrc__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0xCC0020u));
  W_rp_5b9500((intptr_t)(*(intptr_t *)(SYS32(10348) + 364)), (intptr_t)((intptr_t)(*hdcSrc__f)));
  API_GetDIBits((intptr_t)((*hdc__f)), (intptr_t)((intptr_t)(*pjBits__f).f_08), (intptr_t)(0), (intptr_t)((*pjBits__f).biHeight), (intptr_t)((*pjBits__f).pixels), (intptr_t)((BITMAPINFO *)&(*pjBits__f).biSize), (intptr_t)(0));
  g_eng.h5bf3ee(Buffer, "fs_s%d.bmp", (*Time__f));
  strcpy(FileName, (const char *)&SYSB(259));
  strcat(FileName, ((char *)D(0x854f5c)));
  strcat(FileName, Buffer);
  bs_5b7620(&(*pjBits__f), FileName, 0, 0);
  if ( (*pjBits__f).f_08 == 0 )
  {
    API_DeleteObject((intptr_t)(0));
    (*pjBits__f).f_08 = 0;
  }
  if ( (*hdc__f) == 0 )
  {
    API_DeleteDC((intptr_t)(0));
    (*hdc__f) = 0;
  }
  bs_5b6e90(&(*pjBits__f));
  (*v8__f) = -1;
  bs_5b6e80(&(*pjBits__f));
}

/* 0x5b6e40: int __stdcall sub_5B6E40(int a1, unsigned int a2) */
int32_t bs_5b6e40(int32_t a1, uint32_t a2)
{

    intptr_t result = 0;
    uint32_t i = 0;

  result = 0;
  for ( i = 0; i < a2; ++i )
    result += F8((i + a1), 0);
  return result;
}

/* 0x5b6e70: void __thiscall unknown_libname_2(bitmap_session *this) */
void bs_5b6e70(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  ((bitmap_session *)c)->pixels = 0;
}

/* 0x5b6e80: void __thiscall sub_5B6E80(cls_5B0260 *this) */
void bs_5b6e80(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  bs_5b6e90((bitmap_session *)c);
}

/* 0x5b6e90: void __thiscall sub_5B6E90(bitmap_session *this) */
void bs_5b6e90(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( ((bitmap_session *)c)->pixels != 0 )
  {
    API_LocalFree((intptr_t)(((bitmap_session *)c)->pixels));
    ((bitmap_session *)c)->pixels = 0;
  }
}

/* 0x5b6eb0: int __thiscall sub_5B6EB0(cls_5B6EB0 *this) */
int32_t bs_5b6eb0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return F32(c, 0x0);
}

/* 0x5b6ec0: int __thiscall sub_5B6EC0(_DWORD *this) */
int32_t bs_5b6ec0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return (*((intptr_t *)c)) + (*(((intptr_t *)c) + 5) - 1) * bs_5b6ef0(c);
}

/* 0x5b6ee0: int __thiscall sub_5B6EE0(_DWORD *this) */
int32_t bs_5b6ee0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return *(((intptr_t *)c) + 5);
}

/* 0x5b6ef0: int __thiscall sub_5B6EF0(_DWORD *this) */
int32_t bs_5b6ef0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return *(((intptr_t *)c) + 1);
}

/* 0x5b6f00: unsigned __int16 __thiscall sub_5B6F00(bitmap_session *this) */
int32_t bs_5b6f00(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return ((bitmap_session *)c)->biBitCount;
}

/* 0x5b6f10: void __thiscall sub_5B6F10(bitmap_session *this, unsigned __int16 bit_count) */
void bs_5b6f10(wl_char *self, uint16_t bit_count)
{
    intptr_t c = (intptr_t)self;

  ((bitmap_session *)c)->biBitCount = bit_count;
  ((bitmap_session *)c)->stride = ((bitmap_session *)c)->biWidth * (bit_count >> 3);
}

/* 0x5b6f30: int __thiscall sub_5B6F30( cls_5B6F30 *this, HDC hdc, int xDest, int yDest, int DestWidth, int DestHeight, int xSrc, int a8, int SrcWidth, int SrcHeight) */
int32_t bs_5b6f30(wl_char *self, int32_t hdc, int32_t xDest, int32_t yDest, int32_t DestWidth, int32_t DestHeight, int32_t xSrc, int32_t a8, int32_t SrcWidth, int32_t SrcHeight)
{
    intptr_t c = (intptr_t)self;

  return API_StretchDIBits((intptr_t)(hdc), (intptr_t)(xDest), (intptr_t)(yDest), (intptr_t)(DestWidth), (intptr_t)(DestHeight), (intptr_t)(xSrc), (intptr_t)(F32(c, 0x14) - a8 - SrcHeight), (intptr_t)(SrcWidth), (intptr_t)(SrcHeight), (intptr_t)((const void *)F32(c, 0x0)), (intptr_t)((const BITMAPINFO *)&GAP(c, 0x4, 8)), (intptr_t)(0), (intptr_t)(0xCC0020u));
}

/* 0x5b6f80: int *__thiscall sub_5B6F80(bitmap_session *this, int a2, int a3, int a4, int a5, int a6, int *a7) */
int32_t bs_5b6f80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t * a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v8 = 0;
    intptr_t v10 = 0;
    uint32_t v11 = 0;
    intptr_t i = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    uint32_t v15 = 0;
    uint32_t v16 = 0;
    intptr_t v17 = 0;
    uint32_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v28 = 0;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x1c */
    intptr_t *const v23__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x18 */
    intptr_t *const v24__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x14 */
    intptr_t *const v25__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x10 */
    intptr_t *const v26__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0xc */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8 */

  v8 = 0;
  (*v22__f) = a5;
  (*v23__f) = 0;
  (*v24__f) = a6;
  (*v25__f) = 0;
  (*v27__f) = 0;
  (*v26__f) = 0;
  SETLO(v10, bs_5b6f00(c));
  if ( v10 == 8 )
  {
    v28 = 0;
    if ( a6 > 0 )
    {
      while ( 1 )
      {
        v16 = bs_5b6ec0(c) + a3 - ((bitmap_session *)c)->stride * (v28 + a4);
        v17 = 0;
        if ( a5 <= 0 )
          goto LABEL_48;
        do
        {
          if ( (uint8_t)a2 == F8((v16 + v17), 0) )
          {
            (*v27__f) = 1;
          }
          else
          {
            if ( (*v22__f) >= v17 )
              (*v22__f) = v17;
            v8 = 1;
            (*v26__f) = 1;
          }
          ++v17;
        }
        while ( v17 < a5 );
        if ( v8 != 0 )
        {
          v18 = bs_5b6ec0(c) + a3 - ((bitmap_session *)c)->stride * (v28 + a4);
          v19 = a5 - 1;
          while ( (uint8_t)a2 == F8((v18 + v19), 0) )
          {
            if ( --v19 < 0 )
              goto LABEL_44;
          }
          if ( (*v23__f) <= v19 )
            (*v23__f) = v19;
LABEL_44:
          v20 = v28;
          if ( (*v24__f) >= v28 )
            (*v24__f) = v28;
          if ( (*v25__f) <= v28 )
            (*v25__f) = v28;
        }
        else
        {
LABEL_48:
          v20 = v28;
        }
        v28 = v20 + 1;
        if ( v20 + 1 >= a6 )
          break;
        v8 = 0;
      }
    }
  }
  else if ( v10 == 24 )
  {
    if ( a6 > 0 )
    {
      do
      {
        v11 = 3 * a3 - ((bitmap_session *)c)->stride * (v8 + a4) + bs_5b6ec0(c);
        for ( i = 0; i < a5; ++i )
        {
          if ( F16((char *)&a2, 1) == F16(v11, 1) && (uint8_t)a2 == F8(v11, 0) )
          {
            (*v27__f) = 1;
          }
          else
          {
            if ( (*v22__f) >= i )
              (*v22__f) = i;
            (*v26__f) = 1;
          }
          v11 += 3;
        }
        if ( (*v22__f) < a5 )
        {
          v13 = bs_5b6ec0(c);
          v14 = a5 - 1;
          v15 = v13 + 3 * (a5 + a3) - ((bitmap_session *)c)->stride * (v8 + a4) - 3;
          if ( a5 - 1 >= 0 )
          {
            while ( F16((char *)&a2, 1) == F16(v15, 1) && (uint8_t)a2 == F8(v15, 0) )
            {
              v15 -= 3;
              if ( --v14 < 0 )
                goto LABEL_21;
            }
            if ( (*v23__f) <= v14 )
              (*v23__f) = v14;
          }
LABEL_21:
          if ( (*v24__f) >= v8 )
            (*v24__f) = v8;
          if ( (*v25__f) <= v8 )
            (*v25__f) = v8;
        }
        ++v8;
      }
      while ( v8 < a6 );
    }
  }
  else
  {
    (*v22__f) = 0;
    (*v23__f) = a5 - 1;
    (*v24__f) = 0;
    (*v25__f) = a6 - 1;
    (*v26__f) = 1;
    (*v27__f) = 1;
  }
  *a7 = (*v22__f);
  a7[1] = (*v23__f);
  a7[2] = (*v24__f);
  a7[3] = (*v25__f);
  a7[4] = (*v26__f);
  a7[5] = (*v27__f);
  return a7;
}

/* 0x5b71f0: int __thiscall sub_5B71F0(bitmap_session *this, int width, int height, unsigned __int16 bit_count) */
int32_t bs_5b71f0(wl_char *self, int32_t width, int32_t height, uint16_t bit_count)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *const v7__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0xc */

  ((bitmap_session *)c)->biBitCount = bit_count;
  (*v7__f) = height * width * (bit_count >> 3);
  ((bitmap_session *)c)->biSize = 40;
  ((bitmap_session *)c)->biWidth = width;
  ((bitmap_session *)c)->biHeight = height;
  ((bitmap_session *)c)->biPlanes = 1;
  ((bitmap_session *)c)->biCompression = 0;
  ((bitmap_session *)c)->biSizeImage = (*v7__f);
  ((bitmap_session *)c)->biXPelsPerMeter = 0;
  ((bitmap_session *)c)->biYPelsPerMeter = 0;
  ((bitmap_session *)c)->biClrUsed = 0;
  ((bitmap_session *)c)->biClrImportant = 0;
  v5 = API_LocalAlloc((intptr_t)(0x40u), (intptr_t)((*v7__f)));
  ((bitmap_session *)c)->pixels = v5;
  if ( v5 == 0 )
    return 0;
  bs_5b6f10(c, ((bitmap_session *)c)->biBitCount);
  return 1;
}

/* 0x5b7270: int __thiscall sub_5B7270(bitmap_session *this) */
int32_t bs_5b7270(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    uint32_t v5 = 0;
    intptr_t v6 = 0;
    char * v7 = 0;
    intptr_t v8 = 0;
    char * v9 = 0;
    uint32_t v10 = 0;

  if ( ((bitmap_session *)c)->biBitCount != 24 )
    return 0;
  v3 = 4 * bs_5ba390(c);
  v4 = bs_5b6ef0(c);
  v5 = bs_5b6ee0(c) * v4;
  v6 = bs_5b6ee0(c);
  v7 = (char *)API_LocalAlloc((intptr_t)(0x40u), (intptr_t)(v3 * v6));
  if ( v5 / 3 != 0 )
  {
    v8 = 0;
    v9 = v7 + 2;
    v10 = v5 / 3;
    do
    {
      v8 += 3;
      v9 += 4;
      --v10;
      *(v9 - 6) = *((_BYTE *)((bitmap_session *)c)->pixels + v8 - 3);
      *(v9 - 5) = *((_BYTE *)((bitmap_session *)c)->pixels + v8 - 2);
      *(v9 - 4) = *((_BYTE *)((bitmap_session *)c)->pixels + v8 - 1);
      *(v9 - 3) = 0;
    }
    while ( v10 != 0 );
  }
  API_LocalFree((intptr_t)(((bitmap_session *)c)->pixels));
  ((bitmap_session *)c)->pixels = v7;
  bs_5b6f10(c, 0x20u);
  return 1;
}

/* 0x5b7310: int __userpurge sub_5B7310@<eax>(cls_5B7310 *this@<ecx>, int a2@<ebx>, _BYTE *a3, int a4, int a5) */
int32_t bs_5b7310(wl_char *self, int32_t a2, uint8_t * a3, int32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint32_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v11 = 0;
    uint16_t * v12 = 0;
    uint32_t v13 = 0;
    uint8_t * v15 = 0;
    uint16_t * v16 = 0;
    intptr_t v19 = 0;
    uint16_t * v20 = 0;
    uint16_t * v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    intptr_t v24 = 0;
    uint32_t v25 = 0;
    intptr_t v29 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *const v26__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0xc */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x8 */

  v6 = 2 * bs_5ba390(c);
  v7 = bs_5b6ef0(c);
  v8 = bs_5b6ee0(c) * v7;
  v9 = *((uint16_t *)c + 13);
  if ( (uint16_t)v9 == 8 )
  {
    v19 = bs_5b6ee0(c);
    v20 = API_LocalAlloc((intptr_t)(0x40u), (intptr_t)(v6 * v19));
    (*v27__f) = v20;
    (*v26__f) = 0;
    if ( v8 != 0 )
    {
      v21 = v20;
      do
      {
        v22 = F8((F32(c, 0) + (*v26__f)), 0);
        if ( v22 == a4 )
        {
          v23 = (uint8_t)a5 >> a3[18];
          v24 = BYTE1(a5) >> a3[17];
          v25 = BYTE2(a5);
        }
        else
        {
          v23 = *((uint8_t *)c + 4 * v22 + 52) >> a3[18];
          v24 = *((uint8_t *)c + 4 * v22 + 53) >> a3[17];
          v25 = *((uint8_t *)c + 4 * v22 + 54);
        }
        *v21++ = (v25 >> a3[16] << *a3) | (v23 << a3[2]) | (v24 << a3[1]);
        ++(*v26__f);
      }
      while ( (*v26__f) < v8 );
    }
  }
  else
  {
    if ( v9 != 24 )
      return 0;
    v11 = bs_5b6ee0(c);
    v12 = API_LocalAlloc((intptr_t)(0x40u), (intptr_t)(v6 * v11));
    (*v27__f) = v12;
    v13 = v8 / 3;
    if ( v8 / 3 != 0 )
    {
      /* (hand finish: the asm's loop — per source triple b, g, r: (r >> [a3+16]) << [a3], (g >> [a3+17]) << [a3+1],
       * (b >> [a3+18]) << [a3+2], OR'd, the low word stored; each channel is shifted as a byte, the registers'
       * upper halves (the caller's ebx among them) never reach the stored word) */
      v15 = (uint8_t *)F32(c, 0);
      v16 = v12;
      for ( v29 = v13; v29 != 0; --v29, v15 += 3, ++v16 )
        *v16 = (uint16_t)(((uint32_t)(uint8_t)(v15[2] >> a3[16]) << a3[0])
                          | ((uint32_t)(uint8_t)(v15[1] >> a3[17]) << a3[1])
                          | ((uint32_t)(uint8_t)(v15[0] >> a3[18]) << a3[2]));
    }
  }
  API_LocalFree((intptr_t)(*(intptr_t *)c));
  F32(c, 0) = (*v27__f);
  bs_5b6f10((bitmap_session *)c, 0x10u);
  return 1;
}

/* 0x5b74f0: int __thiscall sub_5B74F0(bitmap_session *this, int a2, unsigned int a3) */
int32_t bs_5b74f0(wl_char *self, int32_t a2, uint32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint8_t * v5 = 0;
    intptr_t v6 = 0;
    intptr_t j = 0;
    uint32_t v8 = 0;
    uint32_t v9 = 0;
    uint32_t v10 = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const i__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0xc */
    uint8_t *const v12__f = (uint8_t *)(frame_ + 0x4);   /* ebp-0x8 */

  if ( ((bitmap_session *)c)->biBitCount != 8 )
    return 0;
  v5 = API_LocalAlloc((intptr_t)(0x40u), (intptr_t)(3 * ((bitmap_session *)c)->biHeight * ((bitmap_session *)c)->biWidth));
  v6 = 0;
  for ( (*i__f) = 0; v6 < (signed int)((bitmap_session *)c)->biHeight; (*i__f) = v6 )
  {
    for ( j = 0; j < (signed int)((bitmap_session *)c)->biWidth; ++j )
    {
      v8 = v6 * ((bitmap_session *)c)->stride;
      (*v12__f) = *((_BYTE *)((bitmap_session *)c)->pixels + v8 + j);
      if ( (*v12__f) == a2 )
      {
        v5[2 * v8 + 2 * j + v8 + j] = a3;
        v5[2 * j + 1 + 2 * v6 * ((bitmap_session *)c)->stride + j + v6 * ((bitmap_session *)c)->stride] = BYTE1(a3);
        v9 = HIWORD(a3);
        v10 = j + v6 * ((bitmap_session *)c)->stride;
      }
      else
      {
        v5[2 * j + 2 * v8 + j + v8] = ((bitmap_session *)c)->palette[4 * (*v12__f)];
        v5[2 * j + 1 + 2 * (*i__f) * ((bitmap_session *)c)->stride + j + (*i__f) * ((bitmap_session *)c)->stride] = ((bitmap_session *)c)->palette[4 * (*v12__f) + 1];
        v6 = (*i__f);
        v10 = j + (*i__f) * ((bitmap_session *)c)->stride;
        SETLB(v9, ((bitmap_session *)c)->palette[4 * (*v12__f) + 2]);
      }
      v5[2 * v10 + 2 + v10] = v9;
    }
    ++v6;
  }
  API_LocalFree((intptr_t)(((bitmap_session *)c)->pixels));
  ((bitmap_session *)c)->pixels = v5;
  bs_5b6f10(c, 0x18u);
  return 1;
}

/* 0x5b7620: BOOL __thiscall sub_5B7620(cls_5B7620 *this, LPCSTR lpFileName, int a3, int a4) */
int32_t bs_5b7620(wl_char *self, char * lpFileName, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    intptr_t result = 0;
    uint32_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t FileA = 0;
    uint32_t v10 = 0;
    intptr_t v11 = 0;
    _Alignas(8) uint8_t frame_[0x474 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *Buffer = (intptr_t *)(frame_ + 0x1c);   /* [278] at ebp-0x458 */
    uint32_t *const NumberOfBytesWritten__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x474 */
    void * *const Block__f = (void * *)(frame_ + 0x4);   /* ebp-0x470 */
    uint32_t *const nNumberOfBytesToWrite__f = (uint32_t *)(frame_ + 0x8);   /* ebp-0x46c */
    int16_t *const v15__f = (int16_t *)(frame_ + 0xc);   /* ebp-0x468 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0xe);   /* ebp-0x466 */
    int16_t *const v17__f = (int16_t *)(frame_ + 0x12);   /* ebp-0x462 */
    int16_t *const v18__f = (int16_t *)(frame_ + 0x14);   /* ebp-0x460 */
    uint32_t *const v19__f = (uint32_t *)(frame_ + 0x16);   /* ebp-0x45e */

  v5 = 0;
  if ( F32(c, 0x0) == 0 )
    return false;
  v7 = F16(c, 0x1a) != 8 ? 40 : 1064;
  v8 = F32(c, 0x20) + v7 + 14;
  (*v15__f) = 19778;
  (*v16__f) = v8;
  (*v17__f) = 0;
  (*v18__f) = 0;
  (*v19__f) = v7 + 14;
  FileA = API_CreateFileA((intptr_t)(lpFileName), (intptr_t)(0x40000000u), (intptr_t)(2u), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(0x80u), (intptr_t)(0));
  if ( FileA == (intptr_t)-1 )
    return false;
  if ( a4 != 0 )
  {
    bs_5b7cd0((int)&GAP(c, 0x8, 4), Buffer, (int *)&(*nNumberOfBytesToWrite__f));
    (*Block__f) = (intptr_t)g_eng.op_new((*nNumberOfBytesToWrite__f));
    result = API_WriteFile((intptr_t)(FileA), (intptr_t)(Buffer), (intptr_t)(0x458u), (intptr_t)(&(*NumberOfBytesWritten__f)), (intptr_t)(0));
    if ( result )
    {
      v10 = (*nNumberOfBytesToWrite__f);
      if ( (*nNumberOfBytesToWrite__f) != 0 )
      {
        v11 = 0;
        do
        {
          ++v5;
          *((uint8_t *)(*Block__f) + v11) = (rnd() << 8) / 0x8000;
          v10 = (*nNumberOfBytesToWrite__f);
          v11 = (uint16_t)v5;
        }
        while ( (uint16_t)v5 < (*nNumberOfBytesToWrite__f) );
      }
      result = API_WriteFile((intptr_t)(FileA), (intptr_t)(&(*Block__f)), (intptr_t)(v10), (intptr_t)(&(*NumberOfBytesWritten__f)), (intptr_t)(0));
      if ( result )
      {
        g_eng.op_delete((void *)(*Block__f));
LABEL_12:
        result = API_WriteFile((intptr_t)(FileA), (intptr_t)((LPCVOID)F32(c, 0x0)), (intptr_t)(F32(c, 0x4) * F32(c, 0x14)), (intptr_t)(&(*NumberOfBytesWritten__f)), (intptr_t)(0));
        if ( result )
        {
          API_CloseHandle((intptr_t)(FileA));
          return true;
        }
      }
    }
  }
  else
  {
    result = API_WriteFile((intptr_t)(FileA), (intptr_t)(&(*v15__f)), (intptr_t)(0xEu), (intptr_t)(&(*NumberOfBytesWritten__f)), (intptr_t)(0));
    if ( result )
    {
      result = API_WriteFile((intptr_t)(FileA), (intptr_t)(&GAP(c, 0x8, 4)), (intptr_t)(v7), (intptr_t)(&(*NumberOfBytesWritten__f)), (intptr_t)(0));
      if ( result )
        goto LABEL_12;
    }
  }
  return result;
}

/* 0x5b7800: int __thiscall sub_5B7800( bitmap_session *this, void *hModule, unsigned int resource_id, void *resource_type, int compressed_flag) */
int32_t bs_5b7800(wl_char *self, void * hModule, uint32_t resource_id, void * resource_type, int32_t compressed_flag)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v7 = 0;
    char * v8 = 0;
    intptr_t biBitCount = 0;
    void * v10 = 0;
    _Alignas(8) uint8_t frame_[0x42c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v12 = (uint8_t *)(frame_ + 0x4);   /* [4] at ebp-0x428 */
    uint16_t *bit_count = (uint16_t *)(frame_ + 0x12);   /* [13] at ebp-0x41a */
    uint8_t *v16 = (uint8_t *)(frame_ + 0x2c);   /* [1024] at ebp-0x400 */
    uint32_t *const v11__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x42c */
    intptr_t *const width__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x424 */
    intptr_t *const height__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x420 */

  bs_5b6e90(c);
  result = (int)API_FindResourceA((intptr_t)((intptr_t)hModule), (intptr_t)((char *)(uint16_t)resource_id), (intptr_t)((char *)resource_type));
  if ( result != 0 )
  {
    result = (int)API_LoadResource((intptr_t)((intptr_t)hModule), (intptr_t)((intptr_t)result));
    if ( result != 0 )
    {
      result = (int)API_LockResource((intptr_t)((intptr_t)result));
      v7 = result;
      if ( result != 0 )
      {
        if ( compressed_flag != 0 )
        {
          result = bs_5b7c10((int)v12, result, &(*v11__f));
          if ( result == 0 )
            return result;
          v8 = (char *)(v7 + (*v11__f) + 1112);
          bs_5b71f0(c, (*width__f), (*height__f), bit_count[0]);
          qmemcpy(((bitmap_session *)c)->palette, v16, sizeof(((bitmap_session *)c)->palette));
        }
        else
        {
          v8 = (char *)(F32(result, 0) + result);
          result = bs_5b71f0(c, F32(result, 4), F32(result, 8), F16(result, 14));
          if ( result == 0 )
            return result;
          biBitCount = ((bitmap_session *)c)->biBitCount;
          if ( (uint16_t)biBitCount == 8 )
          {
            v10 = v8;
            v8 += 1024;
            qmemcpy(((bitmap_session *)c)->palette, v10, sizeof(((bitmap_session *)c)->palette));
          }
          else if ( biBitCount != 24 )
          {
            bs_5b6e90(c);
            return 0;
          }
        }
        qmemcpy(((bitmap_session *)c)->pixels, v8, ((bitmap_session *)c)->stride * ((bitmap_session *)c)->biHeight);
        return 1;
      }
    }
  }
  return result;
}

/* 0x5b7970: int __thiscall sub_5B7970(bitmap_session *this, LPCSTR lpFileName, int a3, int a4) */
int32_t bs_5b7970(wl_char *self, char * lpFileName, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t FileA = 0;
    void * v6 = 0;
    intptr_t v8 = 0;
    intptr_t biBitCount = 0;
    _Alignas(8) uint8_t frame_[0x8c4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint8_t *v13 = (uint8_t *)(frame_ + 0xc);   /* [4] at ebp-0x8b8 */
    uint16_t *v16 = (uint16_t *)(frame_ + 0x1a);   /* [9] at ebp-0x8aa */
    uint8_t *v18 = (uint8_t *)(frame_ + 0x34);   /* [16] at ebp-0x890 */
    uint8_t *v19 = (uint8_t *)(frame_ + 0x44);   /* [4] at ebp-0x880 */
    uint16_t *bit_count = (uint16_t *)(frame_ + 0x52);   /* [13] at ebp-0x872 */
    uint8_t *v23 = (uint8_t *)(frame_ + 0x6c);   /* [1024] at ebp-0x858 */
    uint8_t *Buffer = (uint8_t *)(frame_ + 0x46c);   /* [1112] at ebp-0x458 */
    uint32_t *const NumberOfBytesRead__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x8c4 */
    uint32_t *const nNumberOfBytesToRead__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x8c0 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x8bc */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x8b4 */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x8b0 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x898 */
    intptr_t *const width__f = (intptr_t *)(frame_ + 0x48);   /* ebp-0x87c */
    intptr_t *const height__f = (intptr_t *)(frame_ + 0x4c);   /* ebp-0x878 */

  (*v12__f) = 0;
  bs_5b6e90(c);
  FileA = API_CreateFileA((intptr_t)(lpFileName), (intptr_t)(0x80000000), (intptr_t)(1u), (intptr_t)(0), (intptr_t)(3u), (intptr_t)(0x80u), (intptr_t)(0));
  if ( FileA != (intptr_t)-1 )
  {
    if ( a4 != 0 )
    {
      if ( !API_ReadFile((intptr_t)(FileA), (intptr_t)(Buffer), (intptr_t)(0x458u), (intptr_t)(&(*NumberOfBytesRead__f)), (intptr_t)(0))
        || bs_5b7c10((int)v19, (int)Buffer, &(*nNumberOfBytesToRead__f)) == 0 )
      {
        goto LABEL_22;
      }
      v6 = (intptr_t)g_eng.op_new((*nNumberOfBytesToRead__f));
      if ( !API_ReadFile((intptr_t)(FileA), (intptr_t)(v6), (intptr_t)((*nNumberOfBytesToRead__f)), (intptr_t)(&(*NumberOfBytesRead__f)), (intptr_t)(0)) )
      {
        g_eng.op_delete((void *)v6);
        API_CloseHandle((intptr_t)(FileA));
        return (*v12__f);
      }
      g_eng.op_delete((void *)v6);
      bs_5b71f0(c, (*width__f), (*height__f), bit_count[0]);
      qmemcpy(((bitmap_session *)c)->palette, v23, sizeof(((bitmap_session *)c)->palette));
      v8 = API_ReadFile((intptr_t)(FileA), (intptr_t)(((bitmap_session *)c)->pixels), (intptr_t)(((bitmap_session *)c)->stride * ((bitmap_session *)c)->biHeight), (intptr_t)(&(*NumberOfBytesRead__f)), (intptr_t)(0));
      goto LABEL_20;
    }
    if ( !API_ReadFile((intptr_t)(FileA), (intptr_t)(v18), (intptr_t)(0xEu), (intptr_t)(&(*NumberOfBytesRead__f)), (intptr_t)(0))
      || !API_ReadFile((intptr_t)(FileA), (intptr_t)(v13), (intptr_t)(0x28u), (intptr_t)(&(*NumberOfBytesRead__f)), (intptr_t)(0))
      || bs_5b71f0(c, (*v14__f), (*v15__f), v16[0]) == 0 )
    {
      goto LABEL_22;
    }
    biBitCount = ((bitmap_session *)c)->biBitCount;
    if ( (uint16_t)biBitCount != 8 )
    {
      if ( biBitCount != 24 )
      {
        bs_5b6e90(c);
        API_CloseHandle((intptr_t)(FileA));
        return (*v12__f);
      }
      goto LABEL_19;
    }
    if ( (*v17__f) == 0 || (*v17__f) == 255 )
    {
      if ( API_ReadFile((intptr_t)(FileA), (intptr_t)(((bitmap_session *)c)->palette), (intptr_t)(0x400u), (intptr_t)(&(*NumberOfBytesRead__f)), (intptr_t)(0)) )
      {
LABEL_19:
        v8 = API_ReadFile((intptr_t)(FileA), (intptr_t)(((bitmap_session *)c)->pixels), (intptr_t)(((bitmap_session *)c)->stride * ((bitmap_session *)c)->biHeight), (intptr_t)(&(*NumberOfBytesRead__f)), (intptr_t)(0));
LABEL_20:
        if ( v8 )
          (*v12__f) = 1;
      }
    }
    else if ( API_ReadFile((intptr_t)(FileA), (intptr_t)(((bitmap_session *)c)->palette), (intptr_t)(4 * (*v17__f)), (intptr_t)(&(*NumberOfBytesRead__f)), (intptr_t)(0)) )
    {
      goto LABEL_19;
    }
LABEL_22:
    API_CloseHandle((intptr_t)(FileA));
    return (*v12__f);
  }
  return 0;
}

/* 0x5b7b90: void __thiscall sub_5B7B90(bitmap_session *this, void *dest_palette) */
void bs_5b7b90(wl_char *self, void * dest_palette)
{
    intptr_t c = (intptr_t)self;
    char * v2 = 0;
    uint8_t * v3 = 0;
    intptr_t i = 0;
    char v5 = 0;

  v2 = (uint8_t *)dest_palette + 2;
  v3 = &((bitmap_session *)c)->palette[1];
  for ( i = 256; i != 0; --i )
  {
    v5 = v3[1];
    v3 += 4;
    *(v2 - 2) = v5;
    *(v2 - 1) = *(v3 - 4);
    *v2 = *(v3 - 5);
    v2[1] = 0;
    v2 += 4;
  }
}

/* 0x5b7bd0: _BYTE *__thiscall sub_5B7BD0(cls_5B7BD0 *this, int a2) */
int32_t bs_5b7bd0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    uint8_t * result = 0;
    char v5 = 0;

  v2 = 256;
  v3 = a2 + 2;
  result = (uint8_t *)c + 53;
  do
  {
    v5 = F8((v3 - 2), 0);
    v3 += 4;
    result[1] = v5;
    *result = F8((v3 - 5), 0);
    *(result - 1) = F8((v3 - 4), 0);
    result[2] = 0;
    result += 4;
    --v2;
  }
  while ( v2 != 0 );
  return result;
}

/* 0x5b7c10: int __stdcall sub_5B7C10(int a1, int a2, unsigned int *a3) */
int32_t bs_5b7c10(int32_t a1, int32_t a2, uint32_t * a3)
{

    intptr_t v3 = 0;
    uint32_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  v3 = F32(a2, 1096);
  if ( F32(a2, 1080) - v3 != 10001 )
    return 0;
  v5 = F32(a2, 1104) - v3;
  if ( v5 > 0x80 )
    return 0;
  v6 = F16(a2, 1072) - v3;
  v7 = F32(a2 + 4 * (F32((a2 + 24), 0) - v3), 1056) - v3;
  v8 = F32(a2 + 4 * (F32((a2 + 1088), 0) - v3), 4) - v3;
  F16(a1, 14) = v6;
  F32(a1, 4) = v8;
  F32(a1, 0) = 40;
  F32(a1, 8) = v7;
  F16(a1, 12) = 1;
  F32(a1, 16) = 0;
  F32(a1, 20) = v7 * v8 * (v6 >> 3);
  F32(a1, 24) = 0;
  F32(a1, 28) = 0;
  F32(a1, 32) = 0;
  F32(a1, 36) = 0;
  qmemcpy((void *)(a1 + 40), (const void *)(a2 + 32), 0x400u);
  *a3 = v5;
  return 1;
}

/* 0x5b7cd0: int __stdcall sub_5B7CD0(int a1, int *a2, int *a3) */
int32_t bs_5b7cd0(int32_t a1, int32_t * a2, int32_t * a3)
{

    intptr_t v3 = 0;
    int32_t * v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t result = 0;

  v3 = 0xFFFF * rnd() / 0x8000;
  *a2 = 0xFFFF * rnd() / 0x8000;
  a2[5] = 0xFFFF * rnd() / 0x8000;
  a2[7] = 0xFFFF * rnd() / 0x8000;
  a2[269] = 0xFFFF * rnd() / 0x8000;
  a2[271] = 0xFFFF * rnd() / 0x8000;
  a2[273] = 0xFFFF * rnd() / 0x8000;
  a2[275] = 0xFFFF * rnd() / 0x8000;
  v4 = a2 + 264;
  v5 = 4;
  a2[277] = 0xFFFF * rnd() / 0x8000;
  do
  {
    *(v4 - 263) = 0xFFFF * rnd() / 0x8000;
    *v4++ = 0xFFFF * rnd() / 0x8000;
    --v5;
  }
  while ( v5 != 0 );
  a2[272] = v3 + 4 * rnd() / 0x8000;
  v6 = rnd();
  a2[274] = v3;
  v7 = a2[272] - v3;
  a2[6] = v3 + 4 * v6 / 0x8000;
  a2[v7 + 1] = v3 + F32(a1, 4);
  a2[a2[6] - v3 + 264] = v3 + F32(a1, 8);
  a2[268] = v3 + F16(a1, 14);
  qmemcpy(a2 + 8, (const void *)(a1 + 40), 0x400u);
  a2[270] = v3 + 10001;
  result = 50 * rnd() / 0x8000 + 1;
  a2[276] = result + v3;
  *a3 = result;
  return result;
}

/* 0x5b9280: int __thiscall sub_5B9280( cls_5B7FE0 *this, _DWORD *a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11) */
int32_t bs_5b9280(wl_char *self, int32_t * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11)
{
    intptr_t c = (intptr_t)self;
    intptr_t v12 = 0;
    intptr_t v13 = 0;

  v12 = (intptr_t)g_eng.op_new(0xD8u);
  if ( v12 != 0 )
    v13 = W_rp_5b9350((intptr_t)(v12), (intptr_t)((int)c));
  else
    v13 = 0;
  if ( bs_5b9630(v13, (bitmap_session *)a3, a4, a5, a6, a7, 0, a8, a9, a10, (int *)a11) != 0 )
  {
    *a2 = v13;
    return 1;
  }
  else
  {
    if ( v13 != 0 )
    {
      W_rp_5b9390((intptr_t)(v13));
      g_eng.op_delete((void *)v13);
    }
    return 0;
  }
}

/* 0x5b9630: int __thiscall sub_5B9630( cls_5B95C0 *this, bitmap_session *a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int *a11) */
int32_t bs_5b9630(wl_char *self, bitmap_session * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t * a11)
{
    intptr_t c = (intptr_t)self;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t result = 0;
    int32_t * v18 = 0;

  v12 = a5;
  v13 = a6;
  v14 = 0;
  v15 = 0;
  if ( a11 != 0 && a10 >= 1 )
  {
    v18 = (int *)a11[5];
    if ( a10 >= 2 )
    {
      v14 = *a11;
      v15 = a11[2];
      v12 = a11[1] - v14 + 1 < 0 ? 0 : a11[1] - v14 + 1;
      v13 = a11[3] - v15 + 1 < 0 ? 0 : a11[3] - v15 + 1;
    }
    if ( v18 == 0 )
      a8 = 0x1FFFFFF;
    if ( a11[4] == 0 )
    {
      W_rp_5b97e0((intptr_t)(c), (intptr_t)(0), (intptr_t)(0));
      W_rp_5b98c0((intptr_t)(c), (intptr_t)(a5), (intptr_t)(a6), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0));
      return 1;
    }
  }
  result = W_rp_5b95c0((intptr_t)(c), (intptr_t)(a5), (intptr_t)(a6), (intptr_t)(0), (intptr_t)(a8), (intptr_t)(a9), (intptr_t)(v14), (intptr_t)(v15), (intptr_t)(v12), (intptr_t)(v13));
  if ( result != 0 )
  {
    result = bs_5b9910(c, a2, 0, 0, v12, v13, a3 + v14, a4 + v15);
    if ( result != 0 )
      return 1;
  }
  return result;
}

/* 0x5b9910: int __thiscall sub_5B9910(cls_5B9410 *this, bitmap_session *a2, int a3, int a4, int a5, int a6, int a7, int a8) */
int32_t bs_5b9910(wl_char *self, bitmap_session * a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
{
    intptr_t c = (intptr_t)self;
    uint32_t v9 = 0;
    uint32_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint32_t v13 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    bitmap_session * v16 = 0;
    intptr_t v20 = 0;
    bitmap_session * v21 = 0;
    intptr_t v22 = 0;
    intptr_t v23 = 0;
    _Alignas(8) uint8_t frame_[0x8 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x8 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4 */

  (*v18__f) = bs_5b6ec0(a2);
  SETLO(v9, bs_5b6f00(a2));
  v10 = v9 >> 3;
  v20 = bs_5b6ef0(a2);
  if ( W_rp_5b9490((intptr_t)(c)) == 0 )
    return 0;
  v11 = a7 * v10;
  v12 = a3 * v10;
  v13 = a5 * v10;
  v14 = *(_DWORD *)F32(c, 0x4);
  (*v19__f) = *(_DWORD *)F32(c, 0x0);
  v23 = v12;
  if ( (int)(v13 + v12) > v14 )
    v13 = v14 - v12;
  if ( (int)(v13 + v11) > v20 )
    v13 = v20 - v11;
  memset(*(void **)F32(c, 0x0), 0, v14 * *(_DWORD *)F32(c, 0x8));
  v15 = 0;
  if ( a6 > 0 )
  {
    v16 = (bitmap_session *)((*v18__f) + v11 - a8 * v20);
    v22 = -v20;
    v21 = v16;
    do
    {
      qmemcpy((void *)((*v19__f) + v23 + *(_DWORD *)F32(c, 0x4) * (v15 + a4)), v16, v13);
      ++v15;
      v16 = (bitmap_session *)((uint8_t *)v21 + v22);
      v21 = (bitmap_session *)((uint8_t *)v21 + v22);
    }
    while ( v15 < a6 );
  }
  W_rp_5b94d0((intptr_t)(c));
  return 1;
}

/* 0x5ba390: int __thiscall sub_5BA390(_DWORD *this) */
int32_t bs_5ba390(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return *(((intptr_t *)c) + 4);
}
/* unported callees: none */
