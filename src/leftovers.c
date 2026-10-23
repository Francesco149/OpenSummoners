/* leftovers.c — the last reached leftovers: the character AI's input-ring helpers 0x479960 (the windowed ring find:
 * double-taps, held presses) / 0x479a10 (the held-attack charge) / 0x479de0 (the command block) — the hand-ported
 * char_ai inlines them, retail's call sites reach these —, the LOGGER 0x406440 (the error flag + message, the dated
 * line appended through retail's CRT; every module's g_eng.log_fail_406440), and the LAUNCHER: its dialog procedure
 * DialogFunc 0x4013c0 (+ 0x401730; rhost shows no dialog), the splash window procedure 0x401210 and helpers 0x401140 /
 * 0x4011b0 / 0x4013a0.  The last three small engine leaves: 0x5a4760 (a two-dword clear returning this: FLIRT's
 * `_ReaderWriterLock` ctor), 0x58c8d0 (entry k's two record addresses: +0x38 + 60k, +0x3c + 16k), 0x54e4f0 (the
 * solid test of the 3200-unit map block at (a2, a3): its type word + height profile).  See leftovers.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals).  Hand finish: 0x54e4f0's cell index —
 * IDA prints `this + 512 * (a2 / 3200) + 4 * (a3 / 2) + 327692`, the asm computes
 * `16 * (128 * (a2 / 3200) + a3 / 3200 + 0x14003)` (regenerate with --keep 0x54e4f0). */
#include "leftovers.h"

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

#include "frame_load.h"
#include "win_app.h"
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

static inline intptr_t W_bs_5b6ee0(intptr_t a0, ...) { return (intptr_t)bs_5b6ee0((wl_char *)a0); }
static inline intptr_t W_bs_5b6f30(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, intptr_t a5, intptr_t a6, intptr_t a7, intptr_t a8, intptr_t a9, ...) { return (intptr_t)bs_5b6f30((wl_char *)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4, (int32_t)a5, (int32_t)a6, (int32_t)a7, (int32_t)a8, (int32_t)a9); }
static inline intptr_t W_bs_5ba390(intptr_t a0, ...) { return (intptr_t)bs_5ba390((wl_char *)a0); }
static inline intptr_t W_wm_5b5ef0(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4, ...) { return (intptr_t)wm_5b5ef0((int32_t)a0, (int32_t)a1, (int32_t)a2, (int32_t)a3, (int32_t)a4); }

typedef intptr_t (__attribute__((stdcall)) *api_CheckDlgButton_t)(intptr_t, intptr_t, intptr_t);
#define API_CheckDlgButton ((api_CheckDlgButton_t)g_eng.api("CheckDlgButton"))
typedef intptr_t (__attribute__((stdcall)) *api_CheckRadioButton_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_CheckRadioButton ((api_CheckRadioButton_t)g_eng.api("CheckRadioButton"))
typedef intptr_t (__attribute__((stdcall)) *api_DefWindowProcA_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_DefWindowProcA ((api_DefWindowProcA_t)g_eng.api("DefWindowProcA"))
typedef intptr_t (__attribute__((stdcall)) *api_EnableWindow_t)(intptr_t, intptr_t);
#define API_EnableWindow ((api_EnableWindow_t)g_eng.api("EnableWindow"))
typedef intptr_t (__attribute__((stdcall)) *api_EndDialog_t)(intptr_t, intptr_t);
#define API_EndDialog ((api_EndDialog_t)g_eng.api("EndDialog"))
typedef intptr_t (__attribute__((stdcall)) *api_GetDC_t)(intptr_t);
#define API_GetDC ((api_GetDC_t)g_eng.api("GetDC"))
typedef intptr_t (__attribute__((stdcall)) *api_GetDlgItem_t)(intptr_t, intptr_t);
#define API_GetDlgItem ((api_GetDlgItem_t)g_eng.api("GetDlgItem"))
typedef intptr_t (__attribute__((stdcall)) *api_GetLocalTime_t)(intptr_t);
#define API_GetLocalTime ((api_GetLocalTime_t)g_eng.api("GetLocalTime"))
typedef intptr_t (__attribute__((stdcall)) *api_IsDlgButtonChecked_t)(intptr_t, intptr_t);
#define API_IsDlgButtonChecked ((api_IsDlgButtonChecked_t)g_eng.api("IsDlgButtonChecked"))
typedef intptr_t (__attribute__((stdcall)) *api_OutputDebugStringA_t)(intptr_t);
#define API_OutputDebugStringA ((api_OutputDebugStringA_t)g_eng.api("OutputDebugStringA"))
typedef intptr_t (__attribute__((stdcall)) *api_ReleaseDC_t)(intptr_t, intptr_t);
#define API_ReleaseDC ((api_ReleaseDC_t)g_eng.api("ReleaseDC"))
typedef intptr_t (__attribute__((stdcall)) *api_SetFocus_t)(intptr_t);
#define API_SetFocus ((api_SetFocus_t)g_eng.api("SetFocus"))
typedef intptr_t (__attribute__((stdcall)) *api_SetForegroundWindow_t)(intptr_t);
#define API_SetForegroundWindow ((api_SetForegroundWindow_t)g_eng.api("SetForegroundWindow"))
typedef uint32_t (__attribute__((stdcall)) *api_SetTextColor_t)(intptr_t, intptr_t);
#define API_SetTextColor ((api_SetTextColor_t)g_eng.api("SetTextColor"))
typedef intptr_t (__attribute__((stdcall)) *api_SetWindowTextA_t)(intptr_t, intptr_t);
#define API_SetWindowTextA ((api_SetWindowTextA_t)g_eng.api("SetWindowTextA"))
typedef intptr_t (__attribute__((stdcall)) *api_ShowWindow_t)(intptr_t, intptr_t);
#define API_ShowWindow ((api_ShowWindow_t)g_eng.api("ShowWindow"))
typedef intptr_t (__attribute__((stdcall)) *api_TextOutA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_TextOutA ((api_TextOutA_t)g_eng.api("TextOutA"))
#define CRT_fclose ((intptr_t (*)())g_eng.retail(0x5bf440u))   /* retail's CRT fclose (cdecl) */
#define CRT_fopen ((intptr_t (*)())g_eng.retail(0x5bf4e8u))   /* retail's CRT fopen (cdecl) */
#define CRT_fprintf ((intptr_t (*)())g_eng.retail(0x5bf496u))   /* retail's CRT fprintf (cdecl) */
int32_t lo_479960(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9, int32_t a10);
int32_t lo_479a10(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7);
int32_t lo_479de0(int32_t a1, int16_t a2, int16_t a3);
int32_t lo_406440(wl_char *self, char * lpOutputString, char * a3);
int32_t lo_401140(void);
int32_t lo_4011b0(void);
int32_t lo_401210(int32_t hWnd, uint32_t Msg, uint32_t wParam, int32_t lParam);
int32_t lo_4013a0(void);
int32_t lo_4013c0(int32_t hDlg, uint32_t a2, uint32_t a3, int32_t a4);
int32_t lo_401730(int32_t hDlg);
int32_t lo_5a4760(wl_char *self);
int32_t lo_58c8d0(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4);
int32_t lo_54e4f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4);

/* 0x479960: int __thiscall sub_479960( cls_479960 *this, int a2, unsigned int a3, unsigned int a4, int a5, int a6, int a7, int a8, _DWORD *a9, int a10) */
int32_t lo_479960(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8, int32_t * a9, int32_t a10)
{
    intptr_t c = (intptr_t)self;
    intptr_t v10 = 0;
    int32_t * v11 = 0;
    intptr_t v12 = 0;
    intptr_t result = 0;
    int32_t * v14 = 0;
    uint32_t v15 = 0;

  v10 = a6;
  if ( a6 == 5 )
  {
    v10 = a7;
  }
  else if ( a6 == 6 )
  {
    goto LABEL_6;
  }
  a8 = v10;
LABEL_6:
  v11 = a9;
  v12 = c + 4 * (3);
  result = 0;
  while ( 1 )
  {
    v14 = (_DWORD *)F32(v12, 0);
    if ( **(_DWORD **)v12 == a8 )
    {
      v15 = a2 - v14[1];
      if ( a3 <= v15 && a4 >= v15 && v14[2] == a5 )
      {
        if ( a9 == 0 )
          goto LABEL_17;
        if ( *v11 == 0 )
          break;
      }
    }
    ++result;
    v12 += 4;
    ++v11;
    if ( result >= 64 )
      return -1;
  }
  a9[result] = 1;
LABEL_17:
  if ( a10 != 0 )
    **((int32_t **)c + result + 3) = 0;
  return result;
}

/* 0x479a10: int __thiscall sub_479A10(cls_479A10 *this, int a2, int a3, int a4, int a5, int a6, int a7) */
int32_t lo_479a10(wl_char *self, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7)
{
    intptr_t c = (intptr_t)self;
    intptr_t v7 = 0;
    intptr_t v9 = 0;
    uint32_t v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    intptr_t v14 = 0;
    intptr_t v15 = 0;
    intptr_t i = 0;
    int32_t * v17 = 0;
    uint32_t v18 = 0;
    intptr_t v19 = 0;
    intptr_t v20 = 0;
    intptr_t v21 = 0;
    int32_t * v22 = 0;
    intptr_t j = 0;
    int32_t * v24 = 0;
    uint32_t v25 = 0;
    uint32_t v26 = 0;
    int v27 = 0;
    intptr_t v28 = 0;
    int32_t * v29 = 0;
    intptr_t v30 = 0;
    intptr_t v31 = 0;
    intptr_t v32 = 0;
    _Alignas(8) uint8_t frame_[0x130 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v42 = (intptr_t *)(frame_ + 0x30);   /* [64] at ebp-0x100 */
    intptr_t *const v33__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x130 */
    intptr_t *const v34__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x120 */
    intptr_t *const v35__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x11c */
    uint32_t *const v36__f = (uint32_t *)(frame_ + 0x18);   /* ebp-0x118 */
    int32_t * *const v37__f = (int32_t * *)(frame_ + 0x1c);   /* ebp-0x114 */
    intptr_t *const v38__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x110 */
    intptr_t *const v39__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x10c */
    uint32_t *const v40__f = (uint32_t *)(frame_ + 0x28);   /* ebp-0x108 */
    intptr_t *const v41__f = (intptr_t *)(frame_ + 0x2c);   /* ebp-0x104 */

  v7 = a2;
  (*v40__f) = F32(a2, 16);
  v9 = F32(a2, 12);
  v10 = F32(a2, 20);
  v11 = a3;
  (*v35__f) = c;
  (*v34__f) = a3;
  (*v36__f) = v10;
  if ( v9 == 0 )
  {
LABEL_6:
    (*v38__f) = 0;
    goto LABEL_7;
  }
  v12 = v9 - 1;
  if ( v12 != 0 )
  {
    if ( v12 != 1 )
      return 0;
    goto LABEL_6;
  }
  (*v38__f) = 1;
LABEL_7:
  memset(v42, 0, (64 * sizeof(intptr_t)));
  if ( F32(a2, 56) != 0 && a7 == 0 )
  {
    if ( F32(a2, 32) == 5 )
    {
      v14 = a4;
    }
    else if ( F32(a2, 32) == 6 )
    {
      v14 = a5;
    }
    else
    {
      v14 = F32(a2, 32);
    }
    v15 = 0;
    for ( i = c + 4 * (3); ; (i += 4) )
    {
      v17 = (_DWORD *)F32(i, 0);
      if ( **(_DWORD **)i == v14 )
      {
        v18 = a3 - v17[1];
        if ( (*v40__f) <= v18 && (*v36__f) >= v18 && v17[2] == 0 )
          break;
      }
      if ( ++v15 >= 64 )
        return 0;
    }
    if ( v15 < 0 )
      return 0;
    if ( (*v38__f) != 0 )
      v11 = F32(F32(((*v35__f) + (v15 + 3) * 0x4), 0), 4) - 1;
    else
      v11 = (*v34__f);
    v7 = a2;
  }
  SETHI(v19, 0);
  (*v34__f) = 0;
  SETLO(v19, F16(v7, 52));
  (*v41__f) = (uint16_t)v19;
  if ( (uint16_t)v19 != 0 )
  {
    (*v37__f) = (int *)(v7 + 32);
    while ( 1 )
    {
      v20 = *(*v37__f);
      if ( *(*v37__f) == 5 )
      {
        v20 = a4;
      }
      else if ( *(*v37__f) == 6 )
      {
        (*v39__f) = a5;
        goto LABEL_34;
      }
      (*v39__f) = v20;
LABEL_34:
      v21 = 0;
      v22 = v42;
      for ( j = (*v35__f) + 4 * 3; ; (j += 4) )
      {
        v24 = (_DWORD *)F32(j, 0);
        if ( **(_DWORD **)j == (*v39__f) )
        {
          v25 = v11 - v24[1];
          if ( (*v40__f) <= v25 && (*v36__f) >= v25 && v24[2] == 1 )
          {
            if ( &(*v33__f) == (int *)-48 )
              goto LABEL_44;
            if ( *v22 == 0 )
              break;
          }
        }
        ++v21;
        ++v22;
        if ( v21 >= 64 )
          return 0;
      }
      v42[v21] = 1;
LABEL_44:
      if ( v21 < 0 )
        return 0;
      if ( (*v38__f) != 0 )
        v11 = F32(F32(((*v35__f) + (v21 + 3) * 0x4), 0), 4) - 1;
      v26 = F32(a2, 28);
      (*v36__f) = F32(a2, 24);
      v19 = (*v41__f);
      v27 = (*v34__f) + 1 < (*v41__f);
      (*v40__f) = v26;
      ++(*v34__f);
      ++(*v37__f);
      if ( !v27 )
      {
        v7 = a2;
        break;
      }
    }
  }
  if ( a6 != 0 )
  {
    v28 = 0;
    if ( v19 > 0 )
    {
      v29 = (int *)(v7 + 32);
      do
      {
        if ( *v29 == 5 )
        {
          v30 = a4;
        }
        else if ( *v29 == 6 )
        {
          v30 = a5;
        }
        else
        {
          v30 = *v29;
        }
        v31 = 64;
        v32 = (*v35__f) + 4 * 3;
        do
        {
          if ( **(_DWORD **)v32 == v30 )
            **(_DWORD **)v32 = 0;
          v32 += 4;
          --v31;
        }
        while ( v31 != 0 );
        ++v28;
        ++v29;
      }
      while ( v28 < F16(a2, 52) );
    }
  }
  return 1;
}

/* 0x479de0: int __stdcall sub_479DE0(int a1, __int16 a2, __int16 a3) */
int32_t lo_479de0(int32_t a1, int16_t a2, int16_t a3)
{


  F16(a1, 20) = a2;
  F16(a1, 28) = a3;
  F16(a1, 22) = 0;
  F32(a1, 24) = 0;
  return a1;
}

/* 0x406440: int __thiscall cs_fail_406440(const char *this, LPCSTR lpOutputString, LPCSTR a3) */
int32_t lo_406440(wl_char *self, char * lpOutputString, char * a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;
    FILE * v5 = 0;
    _Alignas(8) uint8_t frame_[0x10 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    SYSTEMTIME *const SystemTime__f = (SYSTEMTIME *)(frame_ + 0x0);   /* ebp-0x10 */

  API_OutputDebugStringA((intptr_t)(lpOutputString));
  API_OutputDebugStringA((intptr_t)(a3));
  API_OutputDebugStringA((intptr_t)(((char *)D(0x854570))));
  result = (int)CRT_fopen(c + 1028, ((char *)D(0x85456c)));
  v5 = result;
  if ( result != 0 )
  {
    API_GetLocalTime((intptr_t)(&(*SystemTime__f)));
    CRT_fprintf(
      v5,
      "%04d/%02d/%02d %02d:%02d:%02d  %s %s \n",
      (*SystemTime__f).wYear,
      (*SystemTime__f).wMonth,
      (*SystemTime__f).wDay,
      (*SystemTime__f).wHour,
      (*SystemTime__f).wMinute,
      (*SystemTime__f).wSecond,
      lpOutputString,
      a3);
    CRT_fclose(v5);
    return 1;
  }
  return result;
}

/* 0x401140: int sub_401140() */
int32_t lo_401140(void)
{


  strcpy(((char *)D(0x8a9530) + 0x404), ((char *)D(0x854248)));
  F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 0;
  strcpy((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], D(0x8a9b6c));
  return 0;
}

/* 0x4011b0: int sub_4011B0() */
int32_t lo_4011b0(void)
{


  strcpy(((char *)D(0x8a6620) + 0x404), ((char *)D(0x854248)));
  F32(((uint8_t *)D(0x8a6620) + 0x000), 0) = 0;
  strcpy((char *)&((uint8_t *)D(0x8a6620) + 0x000)[4], D(0x8a9b6c));
  return 0;
}

/* 0x401210: LRESULT __stdcall sub_401210(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) */
int32_t lo_401210(int32_t hWnd, uint32_t Msg, uint32_t wParam, int32_t lParam)
{

    intptr_t v4 = 0;
    intptr_t DC = 0;
    int32_t * v6 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    _Alignas(8) uint8_t frame_[0x30 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *String = (char *)(frame_ + 0x20);   /* [16] at ebp-0x10 */
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x30 */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x24 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x20 */

  strcpy(String, "Please wait.");
  v4 = F32(D(0x8a6b3c), 0);
  if ( Msg == 15 )
  {
    DC = API_GetDC((intptr_t)(hWnd));
    v6 = F32(v4, 264);
    v7 = DC;
    if ( v6 != 0 )
    {
      (*v12__f) = W_bs_5b6ee0((intptr_t)(v6));
      (*v11__f) = W_bs_5ba390((intptr_t)(F32(v4, 264)));
      (*v10__f) = W_bs_5b6ee0((intptr_t)(F32(v4, 264)));
      v8 = W_bs_5ba390((intptr_t)(F32(v4, 264)));
      W_bs_5b6f30((intptr_t)(F32(v4, 264)), (intptr_t)(v7), (intptr_t)(0), (intptr_t)(0), (intptr_t)(v8), (intptr_t)((*v10__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)((*v11__f)), (intptr_t)((*v12__f)));
    }
    else
    {
      W_wm_5b5ef0((intptr_t)(DC), (intptr_t)(0), (intptr_t)(0), (intptr_t)(F32(v4, 268) - 1), (intptr_t)(F32(v4, 272) - 1));
      W_wm_5b5ef0((intptr_t)(v7), (intptr_t)(4), (intptr_t)(4), (intptr_t)(F32(v4, 268) - 9), (intptr_t)(F32(v4, 272) - 9));
      API_SetTextColor((intptr_t)(v7), (intptr_t)(0));
      API_TextOutA((intptr_t)(v7), (intptr_t)(80), (intptr_t)(20), (intptr_t)((char *)(v4 + 276)), (intptr_t)(strlen((const char *)(v4 + 276))));
      API_TextOutA((intptr_t)(v7), (intptr_t)(80), (intptr_t)(40), (intptr_t)(String), (intptr_t)(strlen(String)));
    }
    API_ReleaseDC((intptr_t)(hWnd), (intptr_t)(v7));
  }
  return API_DefWindowProcA((intptr_t)(hWnd), (intptr_t)(Msg), (intptr_t)(wParam), (intptr_t)(lParam));
}

/* 0x4013a0: int sub_4013A0() */
int32_t lo_4013a0(void)
{


  (*(intptr_t *)D(0x8a9a40)) = 0;
  (*(intptr_t *)D(0x8a9b48)) = 0;
  (*(intptr_t *)D(0x8a9b4c)) = 0;
  return 0;
}

/* 0x4013c0: INT_PTR __stdcall DialogFunc(HWND hDlg, UINT a2, WPARAM a3, LPARAM a4) */
int32_t lo_4013c0(int32_t hDlg, uint32_t a2, uint32_t a3, int32_t a4)
{

    intptr_t DlgItem = 0;
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

  if ( a2 == 272 )
  {
    API_SetWindowTextA((intptr_t)(hDlg), (intptr_t)(((char *)D(0x8a9a44))));
    DlgItem = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10001));
    API_SetFocus((intptr_t)(DlgItem));
    if ( F16(D(0x8a9b48), 2) == 3 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10015), (intptr_t)(10017), (intptr_t)(10015));
    if ( F16(D(0x8a9b48), 2) == 4 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10015), (intptr_t)(10017), (intptr_t)(10016));
    if ( F16(D(0x8a9b48), 2) == 5 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10015), (intptr_t)(10017), (intptr_t)(10017));
    if ( (uint16_t)(*(intptr_t *)D(0x8a9b48)) == 3 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10020), (intptr_t)(10025), (intptr_t)(10020));
    if ( (uint16_t)(*(intptr_t *)D(0x8a9b48)) == 4 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10020), (intptr_t)(10025), (intptr_t)(10022));
    if ( (uint16_t)(*(intptr_t *)D(0x8a9b48)) == 5 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10020), (intptr_t)(10025), (intptr_t)(10025));
    v6 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10026));
    API_ShowWindow((intptr_t)(v6), (intptr_t)(0));
    if ( (uint16_t)(*(intptr_t *)D(0x8a9b4c)) == 3 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10006), (intptr_t)(10008), (intptr_t)(10006));
    if ( (uint16_t)(*(intptr_t *)D(0x8a9b4c)) == 4 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10006), (intptr_t)(10008), (intptr_t)(10007));
    if ( (uint16_t)(*(intptr_t *)D(0x8a9b4c)) == 5 )
      API_CheckRadioButton((intptr_t)(hDlg), (intptr_t)(10006), (intptr_t)(10008), (intptr_t)(10008));
    API_CheckDlgButton((intptr_t)(hDlg), (intptr_t)(10024), (intptr_t)(F16(D(0x8a9b4c), 2) == 4));
    v7 = API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10022)) == 1;
    v8 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10015));
    API_EnableWindow((intptr_t)(v8), (intptr_t)(v7));
    v9 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10016));
    API_EnableWindow((intptr_t)(v9), (intptr_t)(v7));
    v10 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10017));
    API_EnableWindow((intptr_t)(v10), (intptr_t)(v7));
    v11 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10018));
    API_EnableWindow((intptr_t)(v11), (intptr_t)(v7));
    v12 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10023));
    API_EnableWindow((intptr_t)(v12), (intptr_t)(v7));
    v13 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10012));
    API_EnableWindow((intptr_t)(v13), (intptr_t)(false));
    v14 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10013));
    API_EnableWindow((intptr_t)(v14), (intptr_t)(false));
    v15 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10014));
    API_EnableWindow((intptr_t)(v15), (intptr_t)(false));
    API_SetForegroundWindow((intptr_t)(hDlg));
    return 0;
  }
  if ( a2 != 273 )
    return 0;
  if ( (uint16_t)a3 > 0x2724u )
  {
    if ( (uint16_t)a3 == 10022 || (uint16_t)a3 == 10025 )
      goto LABEL_32;
    return 0;
  }
  if ( (uint16_t)a3 == 10020 )
  {
LABEL_32:
    lo_401730(hDlg);
    return 0;
  }
  if ( (uint16_t)a3 == 2 )
  {
LABEL_10:
    (*(intptr_t *)D(0x8a9a40)) = 0;
    goto LABEL_11;
  }
  if ( (uint16_t)a3 != 10003 )
  {
    if ( (uint16_t)a3 != 10005 )
      return 0;
    goto LABEL_10;
  }
  (*(intptr_t *)D(0x8a9a40)) = 1;
LABEL_11:
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10020)) == 1 )
    F16(D(0x8a9b48), 0) = 3;
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10022)) == 1 )
    F16(D(0x8a9b48), 0) = 4;
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10025)) == 1 )
    F16(D(0x8a9b48), 0) = 5;
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10015)) == 1 )
    F16(D(0x8a9b48), 2) = 3;
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10016)) == 1 )
    F16(D(0x8a9b48), 2) = 4;
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10017)) == 1 )
    F16(D(0x8a9b48), 2) = 5;
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10006)) == 1 )
    F16(D(0x8a9b4c), 0) = 3;
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10007)) == 1 )
    F16(D(0x8a9b4c), 0) = 4;
  if ( API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10008)) == 1 )
    F16(D(0x8a9b4c), 0) = 5;
  F16(D(0x8a9b4c), 2) = 4 - (API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10024)) != 1);
  API_EndDialog((intptr_t)(hDlg), (intptr_t)(0));
  return 1;
}

/* 0x401730: BOOL __stdcall sub_401730(HWND hDlg) */
int32_t lo_401730(int32_t hDlg)
{

    intptr_t v1 = 0;
    intptr_t DlgItem = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;

  v1 = API_IsDlgButtonChecked((intptr_t)(hDlg), (intptr_t)(10022)) == 1;
  DlgItem = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10015));
  API_EnableWindow((intptr_t)(DlgItem), (intptr_t)(v1));
  v3 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10016));
  API_EnableWindow((intptr_t)(v3), (intptr_t)(v1));
  v4 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10017));
  API_EnableWindow((intptr_t)(v4), (intptr_t)(v1));
  v5 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10018));
  API_EnableWindow((intptr_t)(v5), (intptr_t)(v1));
  v6 = API_GetDlgItem((intptr_t)(hDlg), (intptr_t)(10023));
  return API_EnableWindow((intptr_t)(v6), (intptr_t)(v1));
}
/* 0x5a4760: _DWORD __thiscall Concurrency::details::_ReaderWriterLock::_ReaderWriterLock(cls_5A4760 *this) */
int32_t lo_5a4760(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(((uint8_t *)(c) + 0x0), 0) = 0;
  *(_DWORD *)&GAP(c, 0x0, 4) = 0;
  return c;
}

/* 0x58c8d0: int __thiscall sub_58C8D0(cls_58C8D0 *this, int a2, _DWORD *a3, int *a4) */
int32_t lo_58c8d0(wl_char *self, int32_t a2, int32_t * a3, int32_t * a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = a2;
  if ( a3 != 0 )
    *a3 = F32(c, 0x38) + 60 * a2;
  if ( a4 != 0 )
  {
    result = F32(c, 0x3c) + 16 * a2;
    *a4 = result;
  }
  return result;
}

/* 0x54e4f0: int __thiscall sub_54E4F0(cls_54E4F0 *this, int a2, int a3, int a4) */
int32_t lo_54e4f0(wl_char *self, int32_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    uint16_t v8 = 0;
    intptr_t v9 = 0;
    intptr_t v10 = 0;
    intptr_t result = 0;

  v4 = c + 16 * (128 * (a2 / 3200) + a3 / 3200 + 0x14003);   /* (hand finish: IDA prints `4 * (a3 / 2)`; the asm
                                                              * divides a3 by 3200 too — see the module header) */
  v5 = F32(v4, (2) * 4);
  v6 = a3 % 3200 / 100;
  if ( v5 != 0 )
  {
    v7 = 32 - F8((a2 % 3200 / 100 + *(_DWORD *)&v5), 0);
    if ( v6 > v7 )
      v8 = F16(((uint8_t *)(v4) + 0x0), 0);
    else
      v8 = 0;
    v9 = v7 + 1;
  }
  else
  {
    v8 = F16(((uint8_t *)(v4) + 0x0), 0);
    v9 = 0;
  }
  v10 = v8 - 1;
  if ( v10 != 0 )
  {
    if ( v10 == 9 )
      return 0;
  }
  else
  {
    result = a4;
    if ( (int)abs32(v6 - v9) <= 0 )
      return result;
  }
  return 1;
}

/* unported callees: none */
