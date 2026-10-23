/* sound_dev.c — the SOUND layer: DirectSound 0x5baed0 / 0x5bb180 (the primary buffer), the sound buffers (create
 * from a WAVE resource / file 0x5bb250 → 0x5bb3d0 with the RIFF walkers 0x5bb740 / 0x5bb610, duplicate 0x5bb2f0,
 * release 0x5bb3a0; volume / pan / frequency / play / stop / status 0x5bb870..0x5bb8e0), the MUSIC manager (DirectShow:
 * the track object 0x5ba9a0 / 0x5bab10 (FilterGraph, IMediaControl / IMediaSeeking / IBasicAudio), run / pause / stop,
 * positions, volume 0x5bad20..0x5bae90; the BGM table 0x5bbeb0, the track load 0x5bc7c0 (the WMA resource extracted to
 * a temp file 0x5bc150), play 0x5bcb80, the fades + loop points 0x5bb9b0 / 0x5bc0c0, stop 0x5bce40, the master volume
 * 0x5bbe20 / 0x5bcfb0, the activation pause 0x5bbd20) + their OutputDebugString error logs.  See sound_dev.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals).  Hand finish: 0x5bb3d0's stack arguments and
 * its Unlock / SetVolume arities (IDA mistook a mid-body `push esi` for an argument), 0x5bbd20 (zdm shells).  Scripted:
 * tools/ida/finish/sound_dev.py. */
#include "sound_dev.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))


#include <windows.h>
#include <stdio.h>
#include <time.h>


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


typedef intptr_t (*rt_5c0907)(intptr_t, intptr_t, intptr_t);   /* 0x5c0907 cdecl: unported, retail's */
#define RC_5c0907 ((rt_5c0907)g_eng.retail(0x5c0907u))
typedef intptr_t (__attribute__((stdcall)) *api_CloseHandle_t)(intptr_t);
#define API_CloseHandle ((api_CloseHandle_t)g_eng.api("CloseHandle"))
typedef intptr_t (__attribute__((stdcall)) *api_CoCreateInstance_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_CoCreateInstance ((api_CoCreateInstance_t)g_eng.api("CoCreateInstance"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateFileA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_CreateFileA ((api_CreateFileA_t)g_eng.api("CreateFileA"))
typedef intptr_t (__attribute__((stdcall)) *api_DeleteFileA_t)(intptr_t);
#define API_DeleteFileA ((api_DeleteFileA_t)g_eng.api("DeleteFileA"))
typedef intptr_t (__attribute__((stdcall)) *api_DirectSoundCreate_t)(intptr_t, intptr_t, intptr_t);
#define API_DirectSoundCreate ((api_DirectSoundCreate_t)g_eng.api("DirectSoundCreate"))
typedef intptr_t (__attribute__((stdcall)) *api_FindResourceA_t)(intptr_t, intptr_t, intptr_t);
#define API_FindResourceA ((api_FindResourceA_t)g_eng.api("FindResourceA"))
typedef intptr_t (__attribute__((stdcall)) *api_GlobalAlloc_t)(intptr_t, intptr_t);
#define API_GlobalAlloc ((api_GlobalAlloc_t)g_eng.api("GlobalAlloc"))
typedef intptr_t (__attribute__((stdcall)) *api_GlobalFree_t)(intptr_t);
#define API_GlobalFree ((api_GlobalFree_t)g_eng.api("GlobalFree"))
typedef intptr_t (__attribute__((stdcall)) *api_GlobalLock_t)(intptr_t);
#define API_GlobalLock ((api_GlobalLock_t)g_eng.api("GlobalLock"))
typedef intptr_t (__attribute__((stdcall)) *api_GlobalUnlock_t)(intptr_t);
#define API_GlobalUnlock ((api_GlobalUnlock_t)g_eng.api("GlobalUnlock"))
typedef intptr_t (__attribute__((stdcall)) *api_LoadResource_t)(intptr_t, intptr_t);
#define API_LoadResource ((api_LoadResource_t)g_eng.api("LoadResource"))
typedef intptr_t (__attribute__((stdcall)) *api_LockResource_t)(intptr_t);
#define API_LockResource ((api_LockResource_t)g_eng.api("LockResource"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
typedef uint32_t (__attribute__((stdcall)) *api_SizeofResource_t)(intptr_t, intptr_t);
#define API_SizeofResource ((api_SizeofResource_t)g_eng.api("SizeofResource"))
typedef intptr_t (__attribute__((stdcall)) *api_WriteFile_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_WriteFile ((api_WriteFile_t)g_eng.api("WriteFile"))
typedef uint32_t (__attribute__((stdcall)) *api_mmioClose_t)(intptr_t, intptr_t);
#define API_mmioClose ((api_mmioClose_t)g_eng.api("mmioClose"))
typedef uint32_t (__attribute__((stdcall)) *api_mmioDescend_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_mmioDescend ((api_mmioDescend_t)g_eng.api("mmioDescend"))
typedef intptr_t (__attribute__((stdcall)) *api_mmioOpenA_t)(intptr_t, intptr_t, intptr_t);
#define API_mmioOpenA ((api_mmioOpenA_t)g_eng.api("mmioOpenA"))
typedef intptr_t (__attribute__((stdcall)) *api_mmioRead_t)(intptr_t, intptr_t, intptr_t);
#define API_mmioRead ((api_mmioRead_t)g_eng.api("mmioRead"))
int32_t sd_5ba740(char ** a1);
int32_t sd_5ba7e0(wl_char *self);
void sd_5ba820(wl_char *self);
int32_t sd_5ba850(void);
void sd_5ba860(wl_char *self, char * a2, char * a3, int32_t Value);
int32_t sd_5ba9a0(wl_char *self, int32_t * a2, char * Source);
int32_t sd_5baa60(wl_char *self, int32_t * a2);
int32_t sd_5baa80(wl_char *self);
int32_t sd_5baaa0(wl_char *self, char * Source);
int32_t sd_5bab10(wl_char *self);
int32_t sd_5bad20(wl_char *self);
int32_t sd_5bad40(wl_char *self);
int32_t sd_5bad60(wl_char *self);
int32_t sd_5bad80(wl_char *self);
int32_t sd_5badd0(wl_char *self, int32_t a2);
int32_t sd_5bae00(wl_char *self, int32_t a2);
int32_t sd_5bae30(wl_char *self, int32_t a2, int32_t a3);
int64_t sd_5bae70(wl_char *self);
int64_t sd_5bae90(wl_char *self);
int32_t sd_5baec0(wl_char *self);
int32_t sd_5baed0(wl_char *self, char ** a2, int32_t a3);
int32_t sd_5bafa0(wl_char *self);
void sd_5bafe0(wl_char *self);
void sd_5bb040(wl_char *self, char * a2, char * a3, int32_t Value);
int32_t sd_5bb180(wl_char *self, int32_t a2);
int32_t sd_5bb250(wl_char *self, int32_t * a2, int32_t hModule, int32_t a4, int32_t a5, int32_t a6);
int32_t sd_5bb2f0(wl_char *self, int32_t * a2, int32_t a3);
int32_t sd_5bb380(wl_char *self, int32_t a2);
int32_t sd_5bb3a0(wl_char *self);
int32_t sd_5bb3d0(int32_t * a1, char * pszFileName, int32_t hModule, uint16_t a5, int32_t a6, char ** a8, int32_t a9);
int32_t sd_5bb5c0(wl_char *self, int32_t * a2, int32_t a3);
int32_t sd_5bb610(char * pszFileName, char * pch, int32_t * a3, intptr_t * a4);
int32_t sd_5bb740(int32_t hModule, uint16_t a2, int32_t * a3, int32_t * a4, uint32_t * a5);
int32_t sd_5bb870(wl_char *self, int32_t a2);
int32_t sd_5bb880(wl_char *self, int32_t a2);
int32_t sd_5bb890(wl_char *self, int32_t a2);
int32_t sd_5bb8c0(wl_char *self);
int32_t sd_5bb8e0(wl_char *self);
int32_t sd_5bb8f0(wl_char *self, int32_t a2);
int32_t sd_5bb930(wl_char *self, int32_t a2);
int32_t sd_5bb9b0(wl_char *self, int32_t a2, int32_t a3);
int32_t sd_5bbb10(wl_char *self);
void sd_5bbb50(wl_char *self);
int32_t sd_5bbc20(wl_char *self);
int32_t sd_5bbc60(wl_char *self, uint16_t a2);
int32_t sd_5bbc90(wl_char *self, uint16_t a2);
void sd_5bbcd0(wl_char *self, int32_t a2);
void sd_5bbd20(wl_char *self, int32_t active);
int32_t sd_5bbdb0(wl_char *self, int32_t a2, int32_t a3);
int32_t sd_5bbe20(wl_char *self, int32_t a2);
int32_t sd_5bbeb0(wl_char *self, int16_t a2, int16_t a3);
void sd_5bc0c0(wl_char *self);
int32_t sd_5bc150(wl_char *self, int32_t hModule, uint32_t NumberOfBytesWritten, char * lpFileName, char * lpType);
int32_t sd_5bc7c0(wl_char *self, uint16_t a2, int32_t hModule, uint32_t NumberOfBytesWritten, char * lpFileName, char * a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13);
int32_t sd_5bcb30(wl_char *self);
int32_t sd_5bcb80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6);
int32_t sd_5bcdf0(wl_char *self, uint16_t a2);
int32_t sd_5bce40(wl_char *self, int32_t a2);
int32_t sd_5bce80(wl_char *self, uint16_t a2);
int32_t sd_5bceb0(wl_char *self, int32_t a2);
int32_t sd_5bcf30(wl_char *self, uint16_t a2);
int32_t sd_5bcfa0(wl_char *self);
int32_t sd_5bcfb0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);

/* 0x5ba740: int __stdcall sub_5BA740(char **a1) */
int32_t sd_5ba740(char ** a1)
{

    intptr_t v1 = 0;
    char * v2 = 0;

  v1 = (intptr_t)g_eng.op_new(0x104u);
  if ( v1 != 0 )
    v2 = sd_5ba7e0(v1);
  else
    v2 = 0;
  if ( sd_5ba850() != 0 )
  {
    *a1 = v2;
    return 1;
  }
  else
  {
    if ( v2 != 0 )
    {
      sd_5ba820(v2);
      g_eng.op_delete((void *)v2);
    }
    return 0;
  }
}

/* 0x5ba7e0: char *__thiscall sub_5BA7E0(cls_5BA7E0 *this) */
int32_t sd_5ba7e0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x0) = 0;
  strcpy(((uint8_t *)(c) + 0x4), D(0x8a9b6c));
  return (char *)c;
}

/* 0x5ba820: void __thiscall sub_5BA820(cls_5BA820 *this) */
void sd_5ba820(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x0) != 0 )
    API_OutputDebugStringA((intptr_t)(((char *)D(0x8a4c54))));
  if ( strlen((uint8_t *)c + 4) != 0 )
    API_OutputDebugStringA((intptr_t)((char *)c + 4));
}

/* 0x5ba850: int sub_5BA850() */
int32_t sd_5ba850(void)
{


  return 1;
}

/* 0x5ba860: void __thiscall sub_5BA860(cls_5BA860 *this, const char *a2, const char *a3, int Value) */
void sd_5ba860(wl_char *self, char * a2, char * a3, int32_t Value)
{
    intptr_t c = (intptr_t)self;
    int32_t * v4 = 0;
    _Alignas(8) uint8_t frame_[0x40 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [64] at ebp-0x40 */

  v4 = &F32(c, 0x4);
  strcpy((char *)&F32(c, 0x4), ((char *)D(0x8a4c74)));
  strcat((char *)&F32(c, 0x4), a2);
  strcat((char *)&F32(c, 0x4), D(0x85486c));
  strcat((char *)&F32(c, 0x4), a3);
  strcat((char *)&F32(c, 0x4), ((char *)D(0x8a4938)));
  RC_5c0907((intptr_t)(Value), (intptr_t)(Buffer), (intptr_t)(16));
  strcat((char *)v4, Buffer);
  strcat((char *)v4, D(0x8a4934));
  API_OutputDebugStringA((intptr_t)((char *)v4));
}

/* 0x5ba9a0: int __thiscall sub_5BA9A0(cls_5BA860 *this, _DWORD *a2, char *Source) */
int32_t sd_5ba9a0(wl_char *self, int32_t * a2, char * Source)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t result = 0;

  v4 = (intptr_t)g_eng.op_new(0x338u);
  if ( v4 != 0 )
    v5 = sd_5baa60(v4, c);
  else
    v5 = 0;
  result = sd_5baaa0(v5, Source);
  if ( result != 0 )
  {
    *a2 = v5;
  }
  else
  {
    sd_5ba860(c, ((char *)D(0x8a4c90)), ((char *)D(0x8a4c9c)), 0);
    if ( v5 != 0 )
    {
      sd_5baa80(v5);
      g_eng.op_delete((void *)v5);
    }
    return 0;
  }
  return result;
}

/* 0x5baa60: _DWORD *__thiscall sub_5BAA60(cls_5BA860 *this, _DWORD *a2) */
int32_t sd_5baa60(wl_char *self, int32_t * a2)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x4) = 0;
  F32(c, 0x8) = 0;
  F32(c, 0xc) = 0;
  F32(c, 0x10) = 0;
  F32(c, 0x0) = (int)a2;
  ++*a2;
  return &F32(c, 0x0);
}

/* 0x5baa80: int __thiscall sub_5BAA80(cls_5BAA80 *this) */
int32_t sd_5baa80(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    int32_t * v2 = 0;
    intptr_t result = 0;

  sd_5bad80(c);
  v2 = (_DWORD *)F32(c, 0x0);
  result = *v2 - 1;
  *v2 = result;
  return result;
}

/* 0x5baaa0: int __thiscall sub_5BAAA0(cls_5BAAA0 *this, char *Source) */
int32_t sd_5baaa0(wl_char *self, char * Source)
{
    intptr_t c = (intptr_t)self;

  strcpy((uint8_t *)c + 560, Source);
  if ( (int)mbstowcs((wchar_t *)c + 20, Source, 0x104u) <= 0 )
    return 0;
  F32(c, 0x20) = 0;
  F32(c, 0x18) = 0;
  F32(c, 0x24) = 0;
  F32(c, 0x1c) = 0;
  return 1;
}

/* 0x5bab10: int __thiscall sub_5BAB10(cls_5BAB10 *this) */
int32_t sd_5bab10(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    int32_t * v2 = 0;
    intptr_t v4 = 0;
    char * v5 = 0;
    _Alignas(8) uint8_t frame_[0x300 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v6 = (char *)(frame_ + 0x0);   /* [256] at ebp-0x300 */
    char *v7 = (char *)(frame_ + 0x100);   /* [512] at ebp-0x200 */

  v2 = &F32(c, 0x4);
  if ( F32(c, 0x4) != 0 )
    sd_5bad80(c);
  if ( API_CoCreateInstance((intptr_t)(D(0x850f58)), (intptr_t)(0), (intptr_t)(3u), (intptr_t)(D(0x850f08)), (intptr_t)((intptr_t *)v2)) < 0 )
  {
    sd_5ba860(F32(c, 0x0), D(0x8a9b6c), ((char *)D(0x8a4d40)), 0);
    return 0;
  }
  (**(void (__stdcall ***)(int, void *, char *))*v2)(*v2, D(0x850f38), ((uint8_t *)(c) + 0x8));
  (**(void (__stdcall ***)(int, void *, int *))*v2)(*v2, D(0x850f18), &F32(c, 0xc));
  (**(void (__stdcall ***)(int, void *, void *))*v2)(*v2, D(0x850f28), c + 16 * (1));
  (*(void (__stdcall **)(int, void *))(*(_DWORD *)F32(c, 0xc) + 36))(F32(c, 0xc), D(0x850f48));
  v4 = (*(int (__stdcall **)(int, char *, _DWORD))(*(_DWORD *)*v2 + 52))(*v2, (uint8_t *)c + 40, 0);
  if ( v4 > -2147220891 )
  {
    if ( v4 == -2147024882 )
    {
      v5 = ((char *)D(0x8a4ce4));
      goto LABEL_17;
    }
    if ( v4 == 0 )
      return 1;
    goto LABEL_14;
  }
  if ( v4 == -2147220891 )
  {
    v5 = ((char *)D(0x8a4cfc));
    goto LABEL_17;
  }
  if ( v4 == -2147220970 )
  {
    v5 = ((char *)D(0x8a4d18));
    goto LABEL_17;
  }
  if ( v4 != -2147220928 )
  {
LABEL_14:
    v5 = ((char *)D(0x8a4cf4));
    goto LABEL_17;
  }
  v5 = ((char *)D(0x8a4d28));
LABEL_17:
  strcpy(v6, v5);
  strcpy(v7, ((char *)D(0x8a4cd0)));
  strcat(v7, v6);
  strcat(v7, D(0x8a4ccc));
  strcat(v7, (uint8_t *)c + 560);
  sd_5ba860(F32(c, 0x0), ((char *)D(0x8a4cbc)), v7, v4);
  return 0;
}

/* 0x5bad20: BOOL __thiscall sub_5BAD20(cls_5BAD20 *this) */
int32_t sd_5bad20(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;

  v1 = F32(c, 0x8);
  return v1 != 0 && (*(int (__stdcall **)(int))(F32(v1, 0) + 28))(v1) == 0;
}

/* 0x5bad40: BOOL __thiscall sub_5BAD40(cls_5BAD40 *this) */
int32_t sd_5bad40(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;

  v1 = F32(c, 0x8);
  return v1 != 0 && (*(int (__stdcall **)(int))(F32(v1, 0) + 36))(v1) == 0;
}

/* 0x5bad60: BOOL __thiscall sub_5BAD60(cls_5BAD60 *this) */
int32_t sd_5bad60(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v1 = 0;

  v1 = F32(c, 0x8);
  return v1 != 0 && (*(int (__stdcall **)(int))(F32(v1, 0) + 32))(v1) == 0;
}

/* 0x5bad80: int __thiscall sub_5BAD80(cls_5BAD80 *this) */
int32_t sd_5bad80(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t result = 0;

  v2 = F32(c, 0x8);
  if ( v2 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v2, 0) + 8))(v2);
    F32(c, 0x8) = 0;
  }
  v3 = F32(c, 0xc);
  if ( v3 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v3, 0) + 8))(F32(c, 0xc));
    F32(c, 0xc) = 0;
  }
  v4 = F32(c, 0x10);
  if ( v4 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v4, 0) + 8))(F32(c, 0x10));
    F32(c, 0x10) = 0;
  }
  result = F32(c, 0x4);
  if ( result != 0 )
  {
    result = (*(int (__stdcall **)(int))(F32(result, 0) + 8))(F32(c, 0x4));
    F32(c, 0x4) = 0;
  }
  return result;
}

/* 0x5badd0: BOOL __thiscall sub_5BADD0(cls_5BADD0 *this, int a2) */
int32_t sd_5badd0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;

  v2 = F32(c, 0x10);
  return v2 != 0 && (*(int (__stdcall **)(int, int))(F32(v2, 0) + 28))(v2, a2) == 0;
}

/* 0x5bae00: BOOL __thiscall sub_5BAE00(cls_5BAE00 *this, int a2) */
int32_t sd_5bae00(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;

  v2 = F32(c, 0x10);
  return v2 != 0 && (*(int (__stdcall **)(int, int))(F32(v2, 0) + 32))(v2, a2) == 0;
}

/* 0x5bae30: int __thiscall sub_5BAE30(cls_5BAE30 *this, int a2, int a3) */
int32_t sd_5bae30(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;

  v4 = F32(c, 0xc);
  if ( v4 == 0 || (*(int (__stdcall **)(int, int *, int, _DWORD, _DWORD))(F32(v4, 0) + 56))(v4, &a2, 1, 0, 0) != 0 )
    return 0;
  v5 = a3;
  F32(c, 0x20) = a2;
  F32(c, 0x24) = v5;
  return 1;
}

/* 0x5bae70: __int64 __thiscall sub_5BAE70(cls_5BAE70 *this) */
int64_t sd_5bae70(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;

  v2 = F32(c, 0xc);
  if ( v2 != 0 )
    (*(void (__stdcall **)(int, int64_t *, _DWORD))(F32(v2, 0) + 60))(v2, &F64(c, 0x20), 0);
  return F64(c, 0x20);
}

/* 0x5bae90: __int64 __thiscall sub_5BAE90(cls_5BAE90 *this) */
int64_t sd_5bae90(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    int64_t * v1 = 0;
    intptr_t v2 = 0;

  v1 = &F64(c, 0x18);
  if ( F64(c, 0x18) == 0 )
  {
    v2 = F32(c, 0xc);
    if ( v2 != 0 )
      (*(void (__stdcall **)(int, int64_t *))(F32(v2, 0) + 40))(v2, v1);
  }
  return *v1;
}

/* 0x5baec0: BOOL __thiscall sub_5BAEC0(cls_5BAEC0 *this) */
int32_t sd_5baec0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return F32(c, 0x4) != 0;
}

/* 0x5baed0: int __thiscall sub_5BAED0(cls_5BAED0 *this, char **a2, int a3) */
int32_t sd_5baed0(wl_char *self, char ** a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;

  v4 = (intptr_t)g_eng.op_new(0x10Cu);
  v5 = 0;
  if ( v4 != 0 )
    v5 = sd_5bafa0(v4);
  if ( sd_5bb180(v5, (intptr_t)a3) != 0 )
  {
    strcpy((char *)c, &GAP(v5, 0x4, 4));
    if ( v5 != 0 )
    {
      sd_5bafe0(v5);
      g_eng.op_delete((void *)v5);
    }
    return 0;
  }
  else
  {
    *a2 = (char *)v5;
    return 1;
  }
}

/* 0x5bafa0: void *__thiscall sub_5BAFA0(cls_5BAFA0 *this) */
int32_t sd_5bafa0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x0) = 0;
  F32(((uint8_t *)(c) + 0x4), 0) = 0;
  *(_DWORD *)&GAP(c, 0x4, 260) = 0;
  strcpy(&GAP(c, 0x4, 4), D(0x8a9b6c));
  return c;
}

/* 0x5bafe0: void __thiscall sub_5BAFE0(cls_5BAFE0 *this) */
void sd_5bafe0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;

  v2 = F32(((uint8_t *)(c) + 0x4), 0);
  if ( v2 != 0 )
  {
    (*(void (__stdcall **)(int))(F32(v2, 0) + 8))(v2);
    F32(((uint8_t *)(c) + 0x4), 0) = 0;
  }
  if ( F32(c, 0x0) != 0 )
  {
    (*(void (__stdcall **)(int))(*(_DWORD *)F32(c, 0x0) + 8))(F32(c, 0x0));
    F32(c, 0x0) = 0;
  }
  if ( F32(c, 4 * 66) != 0 )
    API_OutputDebugStringA((intptr_t)(((char *)D(0x8a4d54))));
  if ( strlen(&GAP(c, 0x4, 4)) != 0 )
    API_OutputDebugStringA((intptr_t)(&GAP(c, 0x4, 4)));
}

/* 0x5bb040: void __thiscall sub_5BB040(cls_5BB040 *this, const char *a2, const char *a3, int Value) */
void sd_5bb040(wl_char *self, char * a2, char * a3, int32_t Value)
{
    intptr_t c = (intptr_t)self;
    char * v4 = 0;
    _Alignas(8) uint8_t frame_[0x40 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [64] at ebp-0x40 */

  v4 = (char *)(c + 4 * (2));
  strcpy((uint8_t *)c + 8, ((char *)D(0x8a4d74)));
  strcat((uint8_t *)c + 8, a2);
  strcat((uint8_t *)c + 8, D(0x85486c));
  strcat((uint8_t *)c + 8, a3);
  strcat((uint8_t *)c + 8, ((char *)D(0x8a4938)));
  RC_5c0907((intptr_t)(Value), (intptr_t)(Buffer), (intptr_t)(16));
  strcat(v4, Buffer);
  strcat(v4, D(0x8a4934));
  API_OutputDebugStringA((intptr_t)(v4));
}

/* 0x5bb180: HRESULT __thiscall sub_5BB180(cls_5BB040 *this, HWND a2) */
int32_t sd_5bb180(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    _Alignas(8) uint8_t frame_[0x24 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v11 = (intptr_t *)(frame_ + 0x0);   /* [9] at ebp-0x24 */

  v3 = API_DirectSoundCreate((intptr_t)(0), (intptr_t)((void * *)c), (intptr_t)(0));
  v4 = v3;
  if ( v3 != 0 )
  {
    sd_5bb040(c, ((char *)D(0x8a4db4)), ((char *)D(0x8a4dc0)), v3);
    return v4;
  }
  else
  {
    v6 = (*(int (__stdcall **)(int, HWND, int))(*(_DWORD *)F32(c, 0x0) + 24))(F32(c, 0x0), a2, 1);
    v7 = v6;
    if ( v6 != 0 )
    {
      sd_5bb040(c, ((char *)D(0x8a4db4)), ((char *)D(0x8a4b40)), v6);
      return v7;
    }
    else
    {
      memset(v11, 0, (9 * sizeof(intptr_t)));
      v11[2] = 0;
      v11[4] = 0;
      v8 = F32(c, 0x0);
      v11[0] = 36;
      v11[1] = 129;
      v9 = (*(int (__stdcall **)(int, _DWORD *, void *, _DWORD))(F32(v8, 0) + 12))(v8, v11, c + 4 * (1), 0);
      v10 = v9;
      if ( v9 != 0 )
      {
        sd_5bb040(c, ((char *)D(0x8a4db4)), ((char *)D(0x8a4d90)), v9);
        return v10;
      }
      else
      {
        return 0;
      }
    }
  }
}

/* 0x5bb250: int __thiscall sub_5BB250(cls_5BB250 *this, _DWORD *a2, HMODULE hModule, int a4, int a5, int a6) */
int32_t sd_5bb250(wl_char *self, int32_t * a2, int32_t hModule, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    int32_t * v8 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x14 */

  v7 = (intptr_t)g_eng.op_new(0xCu);
  if ( v7 != 0 )
    v8 = sd_5bb380(v7, (int)c);
  else
    v8 = 0;
  result = sd_5bb3d0(v8, 0, hModule, a4, 1, (char **)a5, a6);
  if ( result != 0 )
    *a2 = v8;
  return result;
}

/* 0x5bb2f0: int __thiscall sub_5BB2F0(cls_5BB250 *this, _DWORD *a2, int a3) */
int32_t sd_5bb2f0(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t result = 0;

  v4 = (intptr_t)g_eng.op_new(0xCu);
  if ( v4 != 0 )
    v5 = sd_5bb380(v4, (int)c);
  else
    v5 = 0;
  result = sd_5bb5c0(v5, (_DWORD *)a3, F32(a3, 4));
  if ( result != 0 )
  {
    *a2 = v5;
    return 1;
  }
  return result;
}

/* 0x5bb380: _DWORD *__thiscall sub_5BB380(cls_5BB250 *this, int a2) */
int32_t sd_5bb380(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x0) = 0;
  F32(c, 0x8) = a2;
  ++F32(a2, 264);
  return &F32(c, 0x0);
}

/* 0x5bb3a0: int __thiscall sub_5BB3A0(cls_562B50 *this) */
int32_t sd_5bb3a0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t result = 0;

  if ( F32(c, 0x0) != 0 )
  {
    (*(void (__stdcall **)(int))(*(_DWORD *)F32(c, 0x0) + 8))(F32(c, 0x0));
    F32(c, 0x0) = 0;
  }
  v2 = F32(c, 0x8);
  result = F32(v2, 264) - 1;
  F32(v2, 264) = result;
  return result;
}

/* 0x5bb3d0: int __userpurge sub_5BB3D0@<eax>( _DWORD *a1@<ecx>, int a2@<esi>, LPSTR pszFileName, HMODULE hModule, unsigned __int16 a5, int a6, int a7, char **a8, int a9) */
/* (hand finish: 6 stack arguments — IDA's esi input and its a7 are the mid-body `push esi`, see the module header) */
int32_t sd_5bb3d0(int32_t * a1, char * pszFileName, int32_t hModule, uint16_t a5, int32_t a6, char ** a8, int32_t a9)
{

    intptr_t result = 0;
    intptr_t v11 = 0;
    char * v12 = 0;
    intptr_t v13 = 0;
    char v14 = 0;
    uint32_t v15 = 0;
    intptr_t v16 = 0;
    _Alignas(8) uint8_t frame_[0x50 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *pch = (char *)(frame_ + 0x18);   /* [20] at ebp-0x38 */
    intptr_t *v25 = (intptr_t *)(frame_ + 0x2c);   /* [9] at ebp-0x24 */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x50 */
    intptr_t *const hMem__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x4c */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x48 */
    char * *const v21__f = (char * *)(frame_ + 0xc);   /* ebp-0x44 */
    uint32_t *const v22__f = (uint32_t *)(frame_ + 0x10);   /* ebp-0x40 */
    void * *const v23__f = (void * *)(frame_ + 0x14);   /* ebp-0x3c */

  memset(v25, 0, (9 * sizeof(intptr_t)));
  (*hMem__f) = 0;
  (*v18__f) = 0;
  (*v23__f) = 0;
  (*v20__f) = 0;
  (*v21__f) = 0;
  (*v22__f) = 0;
  v25[0] = 36;
  v25[1] = 226;
  if ( a6 != 0 )
  {
    result = sd_5bb740(hModule, a5, &v25[4], &(*v18__f), &v25[2]);
    if ( result == 0 )
      return result;
  }
  else
  {
    result = (int)sd_5bb610(pszFileName, pch, &(*hMem__f), &v25[2]);
    if ( result == 0 )
      return result;
    v25[4] = pch;
  }
  v11 = (*(int (__stdcall **)(_DWORD, _DWORD *, _DWORD *, _DWORD))(**(_DWORD **)a1[2] + 12))(
          *(_DWORD *)a1[2],
          v25,
          a1,
          0);
  if ( v11 != 0 )
  {
    sd_5bb040(a1[2], ((char *)D(0x8a4db4)), ((char *)D(0x8a4dd4)), v11);
    return 0;
  }
  else
  {
    if ( (*hMem__f) != 0 )
      (*v18__f) = (int)API_GlobalLock((intptr_t)((*hMem__f)));
    if ( (*(int (__stdcall **)(_DWORD, _DWORD, _DWORD, void **, HGLOBAL *, char **, unsigned int *, _DWORD))(*(_DWORD *)*a1 + 44))(
           *a1,
           0,
           v25[2],
           &(*v23__f),
           &(*v20__f),
           &(*v21__f),
           &(*v22__f),
           0) != 0 )
    {
      return 0;
    }
    else
    {
      qmemcpy((*v23__f), (const void *)(*v18__f), (unsigned int)(*v20__f));
      v12 = (*v21__f);
      if ( (*v21__f) != 0 )
      {
        v13 = (*v18__f);
        v14 = (*v22__f);
        v15 = (*v22__f) >> 2;
        qmemcpy((*v21__f), (const void *)(*v18__f), 4 * ((*v22__f) >> 2));
        qmemcpy(&v12[4 * v15], (const void *)(v13 + 4 * v15), v14 & 3);
        v12 = (*v21__f);
      }
      (*(void (__stdcall **)(_DWORD, void *, HGLOBAL, char *, unsigned int))(*(_DWORD *)*a1 + 76))(   /* Unlock */
        *a1,
        (*v23__f),
        (*v20__f),
        v12,
        (*v22__f));
      /* (hand finish: IDA's labels here are 4 bytes off — see the module header) */
      if ( (*hMem__f) != 0 )
      {
        API_GlobalUnlock((intptr_t)((*v18__f)));
        API_GlobalFree((intptr_t)((*hMem__f)));
        (*hMem__f) = 0;
      }
      (*v18__f) = 0;
      if ( a8 != 0 )
        *a8 = (char *)(*v20__f);
      v16 = v25[4];
      if ( a9 != 0 )
      {
        F32(a9, 0) = *(_DWORD *)v25[5];
        F32(a9, 4) = F32(v16, 4);
        F32(a9, 8) = F32(v16, 8);
        F32(a9, 12) = F32(v16, 12);
        F16(a9, 16) = F16(v16, 16);
      }
      a1[1] = F32(v16, 4);
      (*(void (__stdcall **)(int32_t, int32_t))(*(_DWORD *)*a1 + 60))(*a1, 0);   /* SetVolume(0) */
      return 1;
    }
  }
}

/* 0x5bb5c0: int __thiscall sub_5BB5C0(cls_5BB5C0 *this, _DWORD *a2, int a3) */
int32_t sd_5bb5c0(wl_char *self, int32_t * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;

  v4 = (*(int (__stdcall **)(_DWORD, _DWORD, void *))(**(_DWORD **)F32(c, 0x8) + 20))(
         *(_DWORD *)F32(c, 0x8),
         *a2,
         c);
  if ( v4 != 0 )
  {
    sd_5bb040(F32(c, 0x8), ((char *)D(0x8a4db4)), ((char *)D(0x8a4df4)), v4);
    return 0;
  }
  else
  {
    F32(c, 0x4) = a3;
    return 1;
  }
}

/* 0x5bb610: HMMIO __stdcall sub_5BB610(LPSTR pszFileName, HPSTR pch, _DWORD *a3, LONG *a4) */
int32_t sd_5bb610(char * pszFileName, char * pch, int32_t * a3, intptr_t * a4)
{

    intptr_t result = 0;
    intptr_t v5 = 0;
    char * v6 = 0;
    intptr_t cksize = 0;
    intptr_t v8 = 0;
    char * v9 = 0;
    _Alignas(8) uint8_t frame_[0x28 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    MMCKINFO *const v10__f = (MMCKINFO *)(frame_ + 0x0);   /* ebp-0x28 */
    MMCKINFO *const pmmcki__f = (MMCKINFO *)(frame_ + 0x14);   /* ebp-0x14 */

  result = API_mmioOpenA((intptr_t)(pszFileName), (intptr_t)(0), (intptr_t)(0));
  v5 = result;
  if ( result != 0 )
  {
    API_mmioDescend((intptr_t)(result), (intptr_t)(&(*pmmcki__f)), (intptr_t)(0), (intptr_t)(0));
    if ( (*pmmcki__f).ckid == 1179011410 )
    {
      if ( (*pmmcki__f).fccType == 1163280727 )
      {
        while ( 1 )
        {
          while ( 1 )
          {
            API_mmioDescend((intptr_t)(v5), (intptr_t)(&(*v10__f)), (intptr_t)(0), (intptr_t)(0));
            if ( (*v10__f).ckid != 544501094 )
              break;
            if ( (*v10__f).cksize != 16 && (*v10__f).cksize != 18 )
              return 0;
            API_mmioRead((intptr_t)(v5), (intptr_t)(pch), (intptr_t)((*v10__f).cksize));
          }
          if ( (*v10__f).ckid == 1635017060 )
            break;
          if ( (*v10__f).ckid != 1952670054 )
            return 0;
          v6 = (char *)(intptr_t)g_eng.op_new((*v10__f).cksize);
          API_mmioRead((intptr_t)(v5), (intptr_t)(v6), (intptr_t)((*v10__f).cksize));
          g_eng.op_delete((void *)v6);
        }
        cksize = (*v10__f).cksize;
        if ( (*v10__f).cksize == 0 )
          return 0;
        v8 = API_GlobalAlloc((intptr_t)(2u), (intptr_t)((*v10__f).cksize));
        *a3 = v8;
        v9 = (char *)API_GlobalLock((intptr_t)(v8));
        API_mmioRead((intptr_t)(v5), (intptr_t)(v9), (intptr_t)(cksize));
        API_GlobalUnlock((intptr_t)(v9));
        if ( a4 != 0 )
          *a4 = cksize;
        API_mmioClose((intptr_t)(v5), (intptr_t)(0));
        return (intptr_t)1;
      }
      else
      {
        return 0;
      }
    }
    else
    {
      return 0;
    }
  }
  return result;
}

/* 0x5bb740: int __stdcall sub_5BB740(HMODULE hModule, unsigned __int16 a2, _DWORD *a3, _DWORD *a4, unsigned int *a5) */
int32_t sd_5bb740(int32_t hModule, uint16_t a2, int32_t * a3, int32_t * a4, uint32_t * a5)
{

    intptr_t ResourceA = 0;
    intptr_t Resource = 0;
    int32_t * v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    int32_t * v12 = 0;
    uint32_t v13 = 0;
    intptr_t v14 = 0;
    uint32_t v15 = 0;
    int32_t * v16 = 0;

  if ( a3 != 0 )
    *a3 = 0;
  if ( a4 != 0 )
    *a4 = 0;
  if ( a5 != 0 )
    *a5 = 0;
  ResourceA = API_FindResourceA((intptr_t)(hModule), (intptr_t)((char *)a2), (intptr_t)("WAVE"));
  if ( ResourceA == 0 )
    return 0;
  Resource = API_LoadResource((intptr_t)(hModule), (intptr_t)(ResourceA));
  if ( Resource == 0 )
    return 0;
  v7 = (int *)API_LockResource((intptr_t)(Resource));
  if ( v7 == 0 )
    return 0;
  v8 = *v7;
  v9 = v7[1];
  v10 = v7 + 2;
  v11 = *v10;
  v12 = v10 + 1;
  if ( v8 != 1179011410 )
    return 0;
  if ( v11 != 1163280727 )
    return 0;
  v13 = (unsigned int)v12 + v9 - 4;
  if ( (unsigned int)v12 >= v13 )
    return 0;
  while ( 1 )
  {
    v14 = *v12;
    v15 = v12[1];
    v16 = v12 + 2;
    if ( v14 == 544501094 )
      break;
    if ( v14 == 1635017060 && (a4 != 0 && *a4 == 0 || a5 != 0 && *a5 == 0) )
    {
      if ( a4 != 0 )
        *a4 = v16;
      if ( a5 != 0 )
        *a5 = v15;
      if ( a3 == 0 || *a3 != 0 )
        return 1;
    }
LABEL_33:
    v12 = (_DWORD *)((uint8_t *)v16 + ((v15 + 1) & 0xFFFFFFFE));
    if ( (unsigned int)v12 >= v13 )
      return 0;
  }
  if ( a3 == 0 || *a3 != 0 )
    goto LABEL_33;
  if ( v15 < 0xE )
    return 0;
  *a3 = v16;
  if ( a4 != 0 && *a4 == 0 )
    goto LABEL_33;
  if ( a5 != 0 && *a5 == 0 )
    goto LABEL_33;
  return 1;
}

/* 0x5bb870: int __thiscall sub_5BB870(cls_5BB870 *this, int a2) */
int32_t sd_5bb870(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  return (*(int (__stdcall **)(int, int))(*(_DWORD *)F32(c, 0x0) + 60))(F32(c, 0x0), a2);
}

/* 0x5bb880: int __thiscall sub_5BB880(cls_5BB880 *this, int a2) */
int32_t sd_5bb880(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  return (*(int (__stdcall **)(int, int))(*(_DWORD *)F32(c, 0x0) + 64))(F32(c, 0x0), a2);
}

/* 0x5bb890: int __thiscall sub_5BB890(cls_5BB890 *this, int a2) */
int32_t sd_5bb890(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  return (*(int (__stdcall **)(int, unsigned int))(*(_DWORD *)F32(c, 0x0) + 68))(
           F32(c, 0x0),
           F32(c, 0x4) * (a2 + 10000) / 0x2710u);
}

/* 0x5bb8c0: int __thiscall sub_5BB8C0(cls_5BB8C0 *this) */
int32_t sd_5bb8c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  (*(void (__stdcall **)(int, _DWORD))(*(_DWORD *)F32(c, 0x0) + 52))(F32(c, 0x0), 0);
  (*(void (__stdcall **)(int, _DWORD, _DWORD, _DWORD))(*(_DWORD *)F32(c, 0x0) + 48))(F32(c, 0x0), 0, 0, 0);
  return 0;
}

/* 0x5bb8e0: int __thiscall sub_5BB8E0(cls_5BB8E0 *this) */
int32_t sd_5bb8e0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return (*(int (__stdcall **)(int))(*(_DWORD *)F32(c, 0x0) + 72))(F32(c, 0x0));
}

/* 0x5bb8f0: int __thiscall sub_5BB8F0(cls_5BB8F0 *this, int a2) */
int32_t sd_5bb8f0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t result = 0;

  v3 = F32(c, 0x20);
  result = F32((v3 + 576 * (uint16_t)a2), 0);
  if ( result != 0 )
  {
    sd_5bad80(F32((v3 + 576 * (uint16_t)a2), 0));
    return sd_5bce80(c, a2);
  }
  return result;
}

/* 0x5bb930: int __thiscall sub_5BB930(cls_5BB930 *this, int a2) */
int32_t sd_5bb930(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;

  result = (uint16_t)a2;
  v4 = F32(c, 0x20) + 576 * (uint16_t)a2;
  v5 = F32(v4, 40);
  if ( v5 != 0 )
  {
    if ( F16(v5, 20) == 1 )
      F32(v5, 28) = -5000;
    else
      F32(v5, 28) = F32(c, 0x28) + F32(v4, 44);
    sd_5bae30(F32(v4, 0), F32(v5, 48), F32(v5, 52));
    sd_5badd0(F32(v4, 0), F32(v5, 28) > 0 ? 0 : F32(v5, 28));
    sd_5bad20(F32(v4, 0));
    return sd_5bcf30(c, a2);
  }
  return result;
}

/* 0x5bb9b0: __int16 __thiscall sub_5BB9B0(cls_5BB930 *this, int a2, int a3) */
int32_t sd_5bb9b0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    int64_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;
    intptr_t v14 = 0;

  v4 = F32(c, 0x20) + 576 * (uint16_t)a2;
  v5 = F32(v4, 40);
  if ( F32(v5, 12) == 0 )
  {
LABEL_5:
    if ( F32(v5, 8) != 0 )
    {
      if ( sd_5bae70(F32(v4, 0)) >= *(_QWORD *)(v4 + 32) )
      {
        sd_5bad40(F32(v4, 0));
        sd_5bae30(F32(v4, 0), F32(v4, 24), F32(v4, 28));
        sd_5bad20(F32(v4, 0));
      }
    }
    else if ( F16(v5, 20) == 0 )
    {
      v8 = sd_5bae90(F32(v4, 0));
      if ( sd_5bae70(F32(v4, 0)) >= v8 )
        sd_5bbdb0(c, a2, 20);
    }
    v7 = F16(v5, 20);
    if ( (uint16_t)v7 != 0 )
    {
      v9 = v7 - 1;
      if ( v9 == 0 )
      {
        v12 = F32(c, 0x28) + F32(v4, 44);
        v13 = F32(v5, 28);
        if ( v13 < v12 )
          F32(v5, 28) = v13 + F32(v5, 24);
        if ( F32(v5, 28) >= v12 )
        {
          F32(v5, 28) = v12;
          F16(v5, 20) = 0;
        }
        v11 = F32(c, 0x4);
        v14 = F32(v5, 28);
        goto LABEL_26;
      }
      v7 = v9 - 1;
      if ( v7 != 0 )
        return v7;
      v10 = F32(v5, 28);
      if ( v10 > -5000 )
        F32(v5, 28) = v10 - F32(v5, 24);
      v11 = F32(v5, 28);
      if ( v11 <= -5000 )
      {
        SETLO(v7, sd_5bce40(c, a2));
        F16(v5, 20) = 0;
        return v7;
      }
    }
    else
    {
      if ( a3 == 0 )
        return v7;
      v11 = F32(v5, 28);
    }
    v14 = F32(c, 0x4);
LABEL_26:
    SETLO(v7, sd_5badd0(F32(v4, 0), v14 + v11 > 0 ? 0 : v14 + v11));
    return v7;
  }
  v6 = F32(v5, 16);
  if ( v6 <= 0 )
  {
    sd_5bb930(c, a2);
    F32(v5, 12) = 0;
    goto LABEL_5;
  }
  v7 = v6 - 1;
  F32(v5, 16) = v7;
  return v7;
}

/* 0x5bbb10: void *__thiscall sub_5BBB10(cls_5BBB10 *this) */
int32_t sd_5bbb10(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  *(_DWORD *)&GAP(c, 0x4, 28) = 0;
  F32(c, 0x0) = 0;
  *(_DWORD *)&GAP(c, 0x4, 20) = 0;
  strcpy(&GAP(c, 0x4, 40), D(0x8a9b6c));
  return c;
}

/* 0x5bbb50: void __thiscall sub_5BBB50(cls_5BBB50 *this) */
void sd_5bbb50(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    void * v5 = 0;
    void * v6 = 0;

  if ( F32(c, 0x20) != 0 )
  {
    for ( i = 0; i < F16(c, 0x24); ++i )
    {
      v3 = F32(c, 0x20);
      v4 = 576 * i;
      v5 = F32(v4, v3);
      if ( v5 != 0 )
      {
        sd_5baa80(F32(v4, v3));
        g_eng.op_delete((void *)v5);
        if ( strlen((const char *)(v4 + F32(c, 0x20) + 54)) != 0 )
          API_DeleteFileA((intptr_t)((char *)(v4 + F32(c, 0x20) + 54)));
        if ( strlen((const char *)(v4 + F32(c, 0x20) + 314)) != 0 )
          API_DeleteFileA((intptr_t)((char *)(v4 + F32(c, 0x20) + 314)));
      }
    }
    g_eng.op_delete((void *)(void *)F32(c, 0x20));
  }
  g_eng.op_delete((void *)(void *)F32(c, 0x18));
  v6 = (void *)F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    sd_5ba820(F32(c, 0x0));
    g_eng.op_delete((void *)v6);
  }
  if ( strlen((uint8_t *)c + 44) != 0 )
    API_OutputDebugStringA((intptr_t)((char *)c + 44));
}

/* 0x5bbc20: int __thiscall sub_5BBC20(cls_5BBC20 *this) */
int32_t sd_5bbc20(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint16_t v1 = 0;
    uint16_t v2 = 0;
    intptr_t v3 = 0;

  v1 = F16(c, 0x1c);
  v2 = 0;
  if ( v1 == 0 )
    return 0;
  v3 = F32(c, 0x18);
  while ( F32((v3 + 56 * v2), 0) == 0 )
  {
    if ( ++v2 >= v1 )
      return 0;
  }
  return 1;
}

/* 0x5bbc60: BOOL __thiscall sub_5BBC60(cls_5BBC60 *this, unsigned __int16 a2) */
int32_t sd_5bbc60(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;
    int32_t * v2 = 0;

  v2 = (_DWORD *)(F32(c, 0x20) + 576 * a2);
  return *v2 != 0 && v2[10] != 0;
}

/* 0x5bbc90: int __thiscall sub_5BBC90(cls_5BBC90 *this, unsigned __int16 a2) */
int32_t sd_5bbc90(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;
    int32_t * v2 = 0;
    intptr_t result = 0;

  v2 = (_DWORD *)(F32(c, 0x20) + 576 * a2);
  if ( *v2 == 0 )
    return 0;
  result = v2[10];
  if ( result != 0 )
    return F16(result, 20) == 2;
  return result;
}

/* 0x5bbcd0: void __thiscall sub_5BBCD0(cls_5BBCD0 *this, int a2) */
void sd_5bbcd0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v4 = 0;

  for ( i = 0; i < F16(c, 0x1c); ++i )
  {
    v4 = F32(c, 0x18) + 56 * i;
    if ( F32(v4, 0) != 0 )
    {
      SETLO(v4, F16(v4, 4));
      sd_5bbdb0(c, v4, a2);
    }
  }
}

/* 0x5bbd20: void __thiscall sub_5BBD20(zdm *this, int active) */
/* (hand finish, from the asm: zdm +0x18 entries (56 bytes: +0 dev, +4 track, +0x20 active, +0x28 cookie), +0x1c count,
 * +0x20 the 576-byte tracks) */
void sd_5bbd20(wl_char *self, int32_t active)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v4 = 0;
    intptr_t * v5 = 0;

  for ( i = 0; i < F16(c, 0x1c); ++i )
  {
    v4 = F32(c, 0x18) + 56 * i;
    if ( F32(v4, 0) != 0 )
    {
      v5 = (intptr_t *)(F32(c, 0x20) + 576 * F16(v4, 4));
      if ( F32(v4, 0x20) != active )
      {
        if ( active != 0 )
        {
          F64(v4, 0x28) = sd_5bae70(*v5);
          sd_5bad40(*v5);
        }
        else
        {
          sd_5bae30(*v5, F32(v4, 0x28), F32(v4, 0x2c));
          sd_5bad20(*v5);
        }
        F32(v4, 0x20) = active;
      }
    }
  }
}

/* 0x5bbdb0: int __thiscall sub_5BBDB0(cls_5BBDB0 *this, int a2, int a3) */
int32_t sd_5bbdb0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    int64_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;

  SETLODW(v4, (uint16_t)a2);
  v5 = F32(c, 0x20) + 576 * (uint16_t)a2;
  v6 = F32(v5, 40);
  if ( F32(v5, 0) != 0 && v6 != 0 )
  {
    if ( sd_5bae00(F32(v5, 0), v6 + 28) )
    {
      F16(v6, 20) = 2;
      F32(v6, 24) = a3;
      v4 = sd_5bae70(F32(v5, 0));
      *(_QWORD *)(v5 + 16) = v4;
    }
    else
    {
      SETLODW(v4, sd_5bce40(c, a2));
    }
  }
  return v4;
}

/* 0x5bbe20: int __thiscall sub_5BBE20(cls_5BBE20 *this, int a2) */
int32_t sd_5bbe20(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    uint16_t v4 = 0;
    int v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;

  result = a2;
  v4 = 0;
  v5 = F16(c, 0x1c) == 0;
  F32(c, 0x28) = a2;
  if ( !v5 )
  {
    do
    {
      v6 = F32(c, 0x18);
      result = F32((v6 + 56 * v4), 0);
      v7 = v6 + 56 * v4;
      if ( result != 0 )
      {
        result = F32(c, 0x20) + 576 * F16(v7, 4);
        if ( F16(v7, 20) == 0
          || F16(v7, 20) > 2u
          || F32(v7, 28) >= F32(c, 0x28) + F32(result, 44) )
        {
          v8 = F32(result, 44) + F32(c, 0x28);
          result = sd_5badd0(F32(result, 0), v8 > 0 ? 0 : v8);
        }
      }
      ++v4;
    }
    while ( v4 < F16(c, 0x1c) );
  }
  return result;
}

/* 0x5bbeb0: int __thiscall sub_5BBEB0(cls_5BBEB0 *this, __int16 a2, __int16 a3) */
int32_t sd_5bbeb0(wl_char *self, int16_t a2, int16_t a3)
{
    intptr_t c = (intptr_t)self;
    char * v4 = 0;
    void * v6 = 0;
    void * v7 = 0;
    intptr_t v8 = 0;
    int v9 = 0;
    intptr_t v10 = 0;
    void * v11 = 0;
    intptr_t v12 = 0;
    intptr_t v13 = 0;

  if ( sd_5ba740((char **)c) != 0 )
  {
    v6 = (void *)F32(c, 0x20);
    F16(((uint8_t *)(c) + 0x24), 0) = a2;
    if ( v6 != 0 )
      g_eng.op_delete((void *)v6);
    v7 = (intptr_t)g_eng.op_new(576 * F16(((uint8_t *)(c) + 0x24), 0));
    v8 = 0;
    v9 = F16(((uint8_t *)(c) + 0x24), 0) == 0;
    F32(c, 0x20) = (int)v7;
    if ( !v9 )
    {
      do
      {
        v10 = 576 * (uint16_t)v8++;
        F32((v10 + F32(c, 0x20)), 0) = 0;
      }
      while ( (uint16_t)v8 < F16(((uint8_t *)(c) + 0x24), 0) );
    }
    F16(((uint8_t *)(c) + 0x1c), 0) = a3;
    if ( F32(c, 0x18) != 0 )
      g_eng.op_delete((void *)(void *)F32(c, 0x18));
    v11 = (intptr_t)g_eng.op_new(56 * F16(((uint8_t *)(c) + 0x1c), 0));
    v12 = 0;
    v9 = F16(((uint8_t *)(c) + 0x1c), 0) == 0;
    F32(c, 0x18) = (int)v11;
    if ( !v9 )
    {
      do
      {
        v13 = 7 * (uint16_t)v12++;
        F32((F32(c, 0x18) + 8 * v13), 0) = 0;
      }
      while ( (uint16_t)v12 < F16(((uint8_t *)(c) + 0x1c), 0) );
    }
    F32(c, 0x28) = 0;
    F32(c, 0x4) = 0;
    F32(c, 0x8) = 0;
    F32(c, 0xc) = 0;
    F32(c, 0x10) = 0;
    F32(c, 0x14) = 0;
    return 1;
  }
  else
  {
    v4 = (const char *)(F32(c, 0x0) + 4);
    strcpy(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e48)));
    strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e3c)));
    strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e28)));
    strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e1c)));
    strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e0c)));
    strcat(((uint8_t *)(c) + 0x2c), v4);
    strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x854570)));
    API_OutputDebugStringA((intptr_t)(((uint8_t *)(c) + 0x2c)));
    return 0;
  }
}

/* 0x5bc0c0: void __thiscall sub_5BC0C0(cls_5BB930 *this) */
void sd_5bc0c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t i = 0;
    intptr_t v8 = 0;

  v2 = 0;
  v3 = F32(c, 0x8);
  v4 = F32(c, 0x4);
  if ( v3 == v4 )
  {
    F32(c, 0x14) = 0;
  }
  else
  {
    v5 = F32(c, 0x10);
    if ( v5 != 0 )
    {
      F32(c, 0x10) = v5 - 1;
    }
    else
    {
      v6 = F32(c, 0xc);
      v2 = 1;
      F32(c, 0x14) = 1;
      if ( v6 <= (int)abs32(v3 - v4) )
      {
        if ( v3 <= v4 )
          F32(c, 0x4) = v4 - v6;
        else
          F32(c, 0x4) = v4 + v6;
      }
      else
      {
        F32(c, 0x4) = v3;
      }
    }
  }
  for ( i = 0; i < F16(c, 0x1c); ++i )
  {
    v8 = F32(c, 0x18) + 56 * i;
    if ( F32(v8, 0) != 0 )
    {
      SETLO(v8, F16(v8, 4));
      sd_5bb9b0(c, v8, v2);
    }
  }
}

/* 0x5bc150: int __thiscall sub_5BC150( cls_5BC150 *this, HMODULE hModule, DWORD NumberOfBytesWritten, LPCSTR lpFileName, CHAR *lpType) */
int32_t sd_5bc150(wl_char *self, int32_t hModule, uint32_t NumberOfBytesWritten, char * lpFileName, char * lpType)
{
    intptr_t c = (intptr_t)self;
    intptr_t ResourceA = 0;
    intptr_t v7 = 0;
    uint32_t v8 = 0;
    char * v9 = 0;
    char * v10 = 0;
    intptr_t FileA = 0;
    uint32_t v12 = 0;
    void * v13 = 0;
    _Alignas(8) uint8_t frame_[0x14 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char * *const v15__f = (char * *)(frame_ + 0x0);   /* ebp-0x14 */

  ResourceA = API_FindResourceA((intptr_t)(hModule), (intptr_t)((char *)(uint16_t)NumberOfBytesWritten), (intptr_t)(lpType));
  v7 = ResourceA;
  if ( ResourceA == 0 )
  {
    strcpy((uint8_t *)c + 44, ((char *)D(0x8a4e48)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e3c)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4f30)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e1c)));
    strcat((uint8_t *)c + 44, ((uint8_t *)D(0x8a4f14)));
    (*v15__f) = (uint8_t *)c + 44;
    strcat((uint8_t *)c + 44, D(0x8a9b6c));
    v8 = strlen(((char *)D(0x854570))) + 1;
    v9 = (uint8_t *)c + strlen((uint8_t *)c + 44) + 45;
LABEL_11:
    qmemcpy(v9 - 1, ((char *)D(0x854570)), v8);
    API_OutputDebugStringA((intptr_t)((*v15__f)));
    return 0;
  }
  lpType = (CHAR *)API_LoadResource((intptr_t)(hModule), (intptr_t)(ResourceA));
  if ( lpType == 0 )
  {
    strcpy((uint8_t *)c + 44, ((char *)D(0x8a4e48)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e3c)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4f30)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e1c)));
    strcat((uint8_t *)c + 44, ((uint8_t *)D(0x8a4ef4)));
    (*v15__f) = (uint8_t *)c + 44;
    strcat((uint8_t *)c + 44, D(0x8a9b6c));
    v8 = strlen(((char *)D(0x854570))) + 1;
    v9 = (uint8_t *)c + strlen((uint8_t *)c + 44) + 45;
    goto LABEL_11;
  }
  v10 = lpFileName;
  FileA = API_CreateFileA((intptr_t)(lpFileName), (intptr_t)(0x40000000u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(0x80u), (intptr_t)(0));
  if ( FileA == (intptr_t)-1 )
  {
    strcpy((uint8_t *)c + 44, ((char *)D(0x8a4e48)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e3c)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4f30)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e1c)));
    strcat((uint8_t *)c + 44, ((uint8_t *)D(0x8a4ec8)));
    (*v15__f) = (uint8_t *)c + 44;
    strcat((uint8_t *)c + 44, v10);
    v8 = strlen(((char *)D(0x854570))) + 1;
    v9 = (uint8_t *)c + strlen((uint8_t *)c + 44) + 45;
    goto LABEL_11;
  }
  v12 = API_SizeofResource((intptr_t)(hModule), (intptr_t)(v7));
  v13 = API_LockResource((intptr_t)(lpType));
  if ( !API_WriteFile((intptr_t)(FileA), (intptr_t)(v13), (intptr_t)(v12), (intptr_t)(&NumberOfBytesWritten), (intptr_t)(0)) )
  {
    strcpy((uint8_t *)c + 44, ((char *)D(0x8a4e48)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e3c)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4f30)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e1c)));
    strcat((uint8_t *)c + 44, ((uint8_t *)D(0x8a4ea0)));
    (*v15__f) = (uint8_t *)c + 44;
    strcat((uint8_t *)c + 44, v10);
    v8 = strlen(((char *)D(0x854570))) + 1;
    v9 = (uint8_t *)c + strlen((uint8_t *)c + 44) + 45;
    goto LABEL_11;
  }
  if ( NumberOfBytesWritten != v12 )
  {
    strcpy((uint8_t *)c + 44, ((char *)D(0x8a4e48)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e3c)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4f30)));
    strcat((uint8_t *)c + 44, ((char *)D(0x8a4e1c)));
    strcat((uint8_t *)c + 44, ((uint8_t *)D(0x8a4e70)));
    (*v15__f) = (uint8_t *)c + 44;
    strcat((uint8_t *)c + 44, v10);
    v8 = strlen(((char *)D(0x854570))) + 1;
    v9 = (uint8_t *)c + strlen((uint8_t *)c + 44) + 45;
    goto LABEL_11;
  }
  API_CloseHandle((intptr_t)(FileA));
  return 1;
}

/* 0x5bc7c0: int __thiscall sub_5BC7C0( cls_5BC150 *this, unsigned __int16 a2, HMODULE hModule, DWORD NumberOfBytesWritten, LPCSTR lpFileName, const char *a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13) */
int32_t sd_5bc7c0(wl_char *self, uint16_t a2, int32_t hModule, uint32_t NumberOfBytesWritten, char * lpFileName, char * a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12, int32_t a13)
{
    intptr_t c = (intptr_t)self;
    intptr_t v13 = 0;
    intptr_t v14 = 0;
    uint16_t v15 = 0;
    intptr_t v16 = 0;
    char * v17 = 0;
    _Alignas(8) uint8_t frame_[0x108 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *FileName = (char *)(frame_ + 0x4);   /* [260] at ebp-0x104 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x108 */

  v13 = F32(c, 0x20) + 576 * a2;
  v14 = F32(v13, 0);
  if ( F32(v13, 0) != 0 )
  {
    sd_5baa80(F32(v13, 0));
    g_eng.op_delete((void *)v14);
  }
  strcpy(FileName, lpFileName);
  strcpy((char *)(v13 + 54), D(0x8a9b6c));
  strcpy((char *)(v13 + 314), D(0x8a9b6c));
  if ( (uint16_t)NumberOfBytesWritten != 0 )
  {
    v15 = 0;
    if ( strlen(lpFileName) != 3 )
    {
      v16 = 0;
      do
      {
        ++v15;
        FileName[v16] = lpFileName[v16];
        v16 = v15;
      }
      while ( v15 < strlen(lpFileName) - 3 );
    }
    FileName[v15] = 0;
    strcat(FileName, a6);
    if ( sd_5bc150(c, hModule, NumberOfBytesWritten, FileName, ((char *)D(0x854c98))) == 0 )
      goto LABEL_11;
    strcpy((char *)(v13 + 54), lpFileName);
    strcpy((char *)(v13 + 314), FileName);
  }
  if ( sd_5ba9a0(F32(c, 0x0), (_DWORD *)v13, FileName) != 0 )
  {
    F32(v13, 24) = a9;
    F32(v13, 28) = a10;
    F32(v13, 44) = a13;
    F32(v13, 32) = a11;
    F32(v13, 8) = a7;
    F32(v13, 36) = a12;
    F32(v13, 16) = a7;
    F32(v13, 12) = a8;
    F32(v13, 40) = 0;
    F32(v13, 20) = a8;
    F32(v13, 48) = 0;
    return 1;
  }
  v17 = (uint8_t *)c + 44;
  (*v20__f) = (F32(c, 0x0) + 4);
  strcpy(v17, ((char *)D(0x8a4e48)));
  strcat(v17, ((char *)D(0x8a4e3c)));
  strcat(v17, ((char *)D(0x8a4f54)));
  strcat(v17, ((char *)D(0x8a4e1c)));
  strcat(v17, ((char *)D(0x8a4f40)));
  strcat(v17, (const char *)(*v20__f));
  strcat(v17, ((char *)D(0x854570)));
  API_OutputDebugStringA((intptr_t)(v17));
LABEL_11:
  if ( (uint16_t)NumberOfBytesWritten != 0 )
    API_DeleteFileA((intptr_t)(lpFileName));
  return 0;
}

/* 0x5bcb30: _DWORD *__thiscall sub_5BCB30(cls_5BB8F0 *this) */
int32_t sd_5bcb30(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t i = 0;
    int32_t * result = 0;
    intptr_t v4 = 0;

  for ( i = 0; (uint16_t)i < F16(c, 0x24); result[12] = 0 )
  {
    sd_5bb8f0(c, i);
    result = (_DWORD *)(F32(c, 0x20) + 576 * (uint16_t)i++);
    v4 = result[3];
    result[4] = result[2];
    result[10] = 0;
    result[5] = v4;
  }
  return result;
}

/* 0x5bcb80: int __thiscall sub_5BCB80(cls_5BB930 *this, int a2, int a3, int a4, int a5, int a6) */
int32_t sd_5bcb80(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    uint16_t v11 = 0;
    uint16_t v12 = 0;
    intptr_t v13 = 0;
    char * v15 = 0;
    intptr_t v16 = 0;

  v7 = F32(c, 0x20);
  v8 = 576 * (uint16_t)a2;
  v9 = F32(v8 + v7, 40);
  v10 = v8 + v7;
  if ( F32((v8 + v7), 0) != 0 )
  {
    if ( v9 != 0 )
      goto LABEL_9;
    v11 = F16(c, 0x1c);
    v12 = 0;
    if ( v11 != 0 )
    {
      v13 = F32(c, 0x18);
      while ( F32((v13 + 56 * v12), 0) != 0 )
      {
        if ( ++v12 >= v11 )
          return 1;
      }
      v9 = v13 + 56 * v12;
      if ( v9 != 0 )
      {
LABEL_9:
        if ( F32(v10, 0) != 0 )
        {
          if ( sd_5bcdf0(c, a2) == 0 )
          {
            v15 = (const char *)(F32(c, 0x0) + 4);
            strcpy(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e48)));
            strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e3c)));
            strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4f64)));
            strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e1c)));
            strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x8a4e0c)));
            strcat(((uint8_t *)(c) + 0x2c), v15);
            strcat(((uint8_t *)(c) + 0x2c), ((char *)D(0x854570)));
            API_OutputDebugStringA((intptr_t)(((uint8_t *)(c) + 0x2c)));
            return 0;
          }
          if ( *(_QWORD *)(v10 + 32) == 0 && a3 != 0 )
            *(_QWORD *)(v10 + 32) = sd_5bae90(F32(v10, 0));
          F32(v10, 40) = v9;
          F32(v9, 8) = a3;
          F32(v9, 0) = 1;
          F16(v9, 4) = a2;
          F16(v9, 20) = 0;
          F32(v9, 32) = 0;
          if ( a6 != 0 )
          {
            F32(v9, 48) = F32(v10, 16);
            v16 = F32(v10, 20);
          }
          else
          {
            F32(v9, 48) = F32(v10, 8);
            v16 = F32(v10, 12);
          }
          F32(v9, 52) = v16;
          if ( a5 > 0 )
          {
            F16(v9, 20) = 1;
            F32(v9, 24) = a5;
          }
          if ( a4 > 0 )
          {
            F32(v9, 12) = 1;
            F32(v9, 16) = a4;
            return 1;
          }
          F32(v9, 12) = 0;
          sd_5bb930(c, a2);
        }
      }
    }
  }
  return 1;
}

/* 0x5bcdf0: int __thiscall sub_5BCDF0(cls_5BCDF0 *this, unsigned __int16 a2) */
int32_t sd_5bcdf0(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t * v2 = 0;
    intptr_t result = 0;

  v2 = (intptr_t *)(F32(c, 0x20) + 576 * a2);
  if ( *v2 == 0 )
    return 0;
  if ( sd_5baec0(*v2) )
    return 1;
  result = sd_5bab10(*v2);
  if ( result != 0 )
    return 1;
  return result;
}

/* 0x5bce40: int __thiscall sub_5BCE40(cls_5BCE40 *this, int a2) */
int32_t sd_5bce40(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    intptr_t result = 0;

  v3 = F32(c, 0x20);
  result = F32((v3 + 576 * (uint16_t)a2), 0);
  if ( result != 0 )
  {
    sd_5bad60(F32((v3 + 576 * (uint16_t)a2), 0));
    sd_5bceb0(c, a2);
    return sd_5bce80(c, a2);
  }
  return result;
}

/* 0x5bce80: int __thiscall sub_5BCE80(cls_5BCE80 *this, unsigned __int16 a2) */
int32_t sd_5bce80(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    int32_t * v3 = 0;

  result = F32(c, 0x20) + 576 * a2;
  v3 = F32(result, 40);
  if ( v3 != 0 )
  {
    *v3 = 0;
    F32(result, 40) = 0;
  }
  return result;
}

/* 0x5bceb0: int __thiscall sub_5BCEB0(cls_5BB8F0 *this, int a2) */
int32_t sd_5bceb0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v4 = 0;
    intptr_t i = 0;

  result = (uint16_t)a2;
  v4 = F32(c, 0x20) + 576 * (uint16_t)a2;
  if ( F32(v4, 48) != 0 )
    result = sd_5bcf30(c, a2);
  for ( i = 0; (uint16_t)i < F16(c, 0x24); ++i )
  {
    result = F32(c, 0x20) + 576 * (uint16_t)i;
    if ( F32(result, 48) != 0 && F32(result, 0) != 0 && ++F16(result, 52) > 2u )
    {
      F32(result, 48) = 0;
      result = sd_5bb8f0(c, i);
    }
  }
  F16(v4, 52) = 0;
  F32(v4, 48) = 1;
  return result;
}

/* 0x5bcf30: int __thiscall sub_5BCF30(cls_5BCF30 *this, unsigned __int16 a2) */
int32_t sd_5bcf30(wl_char *self, uint16_t a2)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    uint16_t v3 = 0;
    uint16_t v4 = 0;
    uint16_t v5 = 0;

  result = F32(c, 0x20) + 576 * a2;
  if ( F32(result, 48) != 0 )
  {
    v3 = F16(result, 52);
    v4 = 0;
    for ( F32(result, 48) = 0; v4 < F16(c, 0x24); ++v4 )
    {
      result = F32(c, 0x20) + 576 * v4;
      if ( F32(result, 48) != 0 && F32(result, 0) != 0 )
      {
        v5 = F16(result, 52);
        if ( v5 > v3 )
          F16(result, 52) = v5 - 1;
      }
    }
  }
  return result;
}

/* 0x5bcfa0: int __thiscall sub_5BCFA0(cls_5BCFA0 *this) */
int32_t sd_5bcfa0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return F32(c, 0x8);
}

/* 0x5bcfb0: int __thiscall sub_5BCFB0(cls_5BCFB0 *this, int a2, int a3, int a4) */
int32_t sd_5bcfb0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    intptr_t v5 = 0;

  result = a3;
  v5 = F32(c, 0x14);
  F32(c, 0x8) = a2;
  F32(c, 0xc) = a3;
  if ( v5 != 0 )
    F32(c, 0x10) = 0;
  else
    F32(c, 0x10) = a4;
  if ( a3 == 0 )
    F32(c, 0x4) = a2;
  return result;
}
/* unported callees: 0x5c0907 */
