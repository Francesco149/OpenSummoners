/* file_stream.c — the engine's FILE layer: the file object (cls_41A890: a resource view 0x5b62a0, a memory bank
 * 0x5b6200 / 0x5b6250 or a disk file 0x5b60e0 / 0x5b61b0 behind one read 0x5b6340 / write 0x5b6410 / close 0x5b64b0,
 * the FormatMessage error text 0x5b64f0) and the MEMORY BANK under it (cls_5B6520: a block list grown on write
 * 0x5b66f0, the read / write cursors 0x5b67c0 / 0x5b6890 / 0x5b68f0, the bank file save 0x5b6990 / load 0x5b6bc0 —
 * magic 10001, the byte-sum checksum 0x5b6e40, the substitution scramble table 0x5b6db0 + the random key).  The save
 * game's room snapshots and slot files, the map / data resources, config.dat.  See file_stream.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish. */
#include "file_stream.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))
static intptr_t rnd(void) { return (intptr_t)rng_rand(); }


#include "frame_load.h"
#include "frame_load.h"
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

static inline intptr_t W_bs_5b6e40(intptr_t a0, intptr_t a1, ...) { return (intptr_t)bs_5b6e40((int32_t)a0, (uint32_t)a1); }

typedef intptr_t (__attribute__((stdcall)) *api_CloseHandle_t)(intptr_t);
#define API_CloseHandle ((api_CloseHandle_t)g_eng.api("CloseHandle"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateFileA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_CreateFileA ((api_CreateFileA_t)g_eng.api("CreateFileA"))
typedef intptr_t (__attribute__((stdcall)) *api_FindResourceA_t)(intptr_t, intptr_t, intptr_t);
#define API_FindResourceA ((api_FindResourceA_t)g_eng.api("FindResourceA"))
typedef intptr_t (__attribute__((stdcall)) *api_FormatMessageA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_FormatMessageA ((api_FormatMessageA_t)g_eng.api("FormatMessageA"))
typedef intptr_t (__attribute__((stdcall)) *api_GetLastError_t)(void);
#define API_GetLastError ((api_GetLastError_t)g_eng.api("GetLastError"))
typedef intptr_t (__attribute__((stdcall)) *api_LoadResource_t)(intptr_t, intptr_t);
#define API_LoadResource ((api_LoadResource_t)g_eng.api("LoadResource"))
typedef intptr_t (__attribute__((stdcall)) *api_LockResource_t)(intptr_t);
#define API_LockResource ((api_LockResource_t)g_eng.api("LockResource"))
typedef intptr_t (__attribute__((stdcall)) *api_ReadFile_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_ReadFile ((api_ReadFile_t)g_eng.api("ReadFile"))
typedef intptr_t (__attribute__((stdcall)) *api_Sleep_t)(intptr_t);
#define API_Sleep ((api_Sleep_t)g_eng.api("Sleep"))
typedef intptr_t (__attribute__((stdcall)) *api_WriteFile_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_WriteFile ((api_WriteFile_t)g_eng.api("WriteFile"))
int32_t fs_5b6060(wl_char *self);
int32_t fs_5b60d0(wl_char *self);
int32_t fs_5b60e0(wl_char *self, char * lpFileName, int32_t a3);
int32_t fs_5b61b0(wl_char *self, char * lpFileName);
int32_t fs_5b6200(wl_char *self, int32_t a2);
int32_t fs_5b6250(wl_char *self, int32_t a2);
int32_t fs_5b62a0(wl_char *self, int32_t hModule, uint16_t a3);
int32_t fs_5b6340(wl_char *self, int32_t lpBuffer, uint32_t nNumberOfBytesToRead);
int32_t fs_5b6410(wl_char *self, int32_t lpBuffer, uint32_t nNumberOfBytesToWrite);
int32_t fs_5b64b0(wl_char *self);
int32_t fs_5b64e0(wl_char *self);
int32_t fs_5b64f0(wl_char *self);
int32_t fs_5b6520(wl_char *self, char * a2);
int32_t fs_5b6580(wl_char *self);
int32_t fs_5b65a0(wl_char *self);
int32_t fs_5b65b0(wl_char *self, int32_t a2, int32_t a3);
int32_t fs_5b6630(wl_char *self);
int32_t fs_5b6650(wl_char *self);
int32_t fs_5b66a0(wl_char *self);
int32_t fs_5b66b0(wl_char *self);
int32_t fs_5b66f0(wl_char *self, int32_t a2);
int32_t fs_5b67c0(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5);
int32_t fs_5b6890(wl_char *self, int32_t a2, uint32_t a3);
int32_t fs_5b68f0(wl_char *self, int32_t a2, uint32_t a3);
void fs_5b6940(wl_char *self);
int32_t fs_5b6990(wl_char *self, char * lpFileName, int32_t a3);
int32_t fs_5b6bc0(wl_char *self, char * lpFileName);
int32_t fs_5b6db0(wl_char *self, int32_t a2);

/* 0x5b6060: char *__thiscall sub_5B6060(cls_5B6060 *this) */
int32_t fs_5b6060(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  fs_5b64e0(c);
  strcpy((uint8_t *)c + 20, D(0x8a9b6c));
  return (char *)c;
}

/* 0x5b60d0: int __thiscall sub_5B60D0(cls_5B60D0 *this) */
int32_t fs_5b60d0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  return fs_5b64b0(c);
}

/* 0x5b60e0: int __thiscall sub_5B60E0(cls_41A890 *this, LPCSTR lpFileName, int a3) */
int32_t fs_5b60e0(wl_char *self, char * lpFileName, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t FileA = 0;
    intptr_t v7 = 0;
    char * lpFileNamea = 0;
    intptr_t v10 = 0;

  FileA = API_CreateFileA((intptr_t)(lpFileName), (intptr_t)(0x40000000u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(0x80u), (intptr_t)(0));
  F32(c, 0xc) = (int)FileA;
  if ( FileA == (intptr_t)-1 )
  {
    if ( a3 <= 0 || API_GetLastError() != 32 || (lpFileNamea = 0, v10 = a3 / 100, a3 / 100 <= 0) )
    {
LABEL_8:
      fs_5b64f0(c);
      return 0;
    }
    while ( 1 )
    {
      API_Sleep((intptr_t)(0x64u));
      v7 = API_CreateFileA((intptr_t)(lpFileName), (intptr_t)(0x40000000u), (intptr_t)(0), (intptr_t)(0), (intptr_t)(2u), (intptr_t)(0x80u), (intptr_t)(0));
      F32(c, 0xc) = (int)v7;
      if ( v7 != (intptr_t)-1 )
        break;
      if ( API_GetLastError() == 32 && (int)++lpFileNamea < v10 )
        continue;
      goto LABEL_8;
    }
  }
  F32(c, 0x0) = 4;
  return 1;
}

/* 0x5b61b0: int __thiscall sub_5B61B0(cls_41A890 *this, LPCSTR lpFileName) */
int32_t fs_5b61b0(wl_char *self, char * lpFileName)
{
    intptr_t c = (intptr_t)self;
    intptr_t FileA = 0;

  FileA = API_CreateFileA((intptr_t)(lpFileName), (intptr_t)(0x80000000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(3u), (intptr_t)(0x80u), (intptr_t)(0));
  F32(c, 0xc) = (int)FileA;
  if ( FileA == (intptr_t)-1 )
  {
    fs_5b64f0(c);
    return 0;
  }
  else
  {
    F32(c, 0x0) = 3;
    return 1;
  }
}

/* 0x5b6200: int __thiscall sub_5B6200(cls_5B6200 *this, int a2) */
int32_t fs_5b6200(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  F32(c, 4 * 4) = a2;
  F32(c, 0x0) = 2;
  if ( fs_5b6650(a2) != 0 )
    return 1;
  strcpy((uint8_t *)c + 20, (const char *)(F32(c, 4 * 4) + 36));
  return 0;
}

/* 0x5b6250: int __thiscall sub_5B6250(cls_5B6250 *this, int a2) */
int32_t fs_5b6250(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;

  F32(c, 4 * 4) = a2;
  F32(c, 0x0) = 2;
  if ( fs_5b66b0(a2) != 0 )
    return 1;
  strcpy((uint8_t *)c + 20, (const char *)(F32(c, 4 * 4) + 36));
  return 0;
}

/* 0x5b62a0: int __thiscall sub_5B62A0(cls_5B62A0 *this, HMODULE hModule, unsigned __int16 a3) */
int32_t fs_5b62a0(wl_char *self, int32_t hModule, uint16_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = (int)API_FindResourceA((intptr_t)(hModule), (intptr_t)((char *)a3), (intptr_t)("DATA"));
  if ( result != 0 )
  {
    result = (int)API_LoadResource((intptr_t)(hModule), (intptr_t)((intptr_t)result));
    if ( result != 0 )
    {
      result = (int)API_LockResource((intptr_t)((intptr_t)result));
      F32(c, 0x4) = result;
      if ( result != 0 )
      {
        F32(c, 0x8) = result;
        F32(c, 0x0) = 1;
        return 1;
      }
    }
  }
  return result;
}

/* 0x5b6340: int __thiscall sub_5B6340(cls_41A890 *this, LPVOID lpBuffer, DWORD nNumberOfBytesToRead) */
int32_t fs_5b6340(wl_char *self, int32_t lpBuffer, uint32_t nNumberOfBytesToRead)
{
    intptr_t c = (intptr_t)self;
    uint32_t v4 = 0;
    uint32_t v6 = 0;

  switch ( F32(c, 0x0) )
  {
    case 1:
      v6 = nNumberOfBytesToRead;
      qmemcpy(lpBuffer, *(const void **)&GAP(c, 0x6, 2), nNumberOfBytesToRead);
      *(_DWORD *)&GAP(c, 0x6, 2) += v6;
      break;
    case 2:
      if ( fs_5b68f0(F32((uint8_t *)(c), 0x10), (int)lpBuffer, nNumberOfBytesToRead) == 0 )
      {
        strcpy(&GAP(c, 0x10, 4), (const char *)(F32(((uint8_t *)(c) + 0x10), 0) + 36));
        return 0;
      }
      break;
    case 3:
      v4 = nNumberOfBytesToRead;
      if ( !API_ReadFile((intptr_t)((intptr_t)F32(c, 0xc)), (intptr_t)(lpBuffer), (intptr_t)(nNumberOfBytesToRead), (intptr_t)(&nNumberOfBytesToRead), (intptr_t)(0))
        || nNumberOfBytesToRead != v4 )
      {
        fs_5b64f0(c);
        return 0;
      }
      break;
    default:
      break;
  }
  return 1;
}

/* 0x5b6410: int __thiscall sub_5B6410(cls_41A890 *this, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite) */
int32_t fs_5b6410(wl_char *self, int32_t lpBuffer, uint32_t nNumberOfBytesToWrite)
{
    intptr_t c = (intptr_t)self;
    uint32_t v4 = 0;

  if ( F32(c, 0x0) == 2 )
  {
    if ( fs_5b6890(F32((uint8_t *)(c), 0x10), (int)lpBuffer, nNumberOfBytesToWrite) == 0 )
    {
      strcpy(&GAP(c, 0x10, 4), (const char *)(F32(((uint8_t *)(c) + 0x10), 0) + 36));
      return 0;
    }
  }
  else if ( F32(c, 0x0) == 4 )
  {
    v4 = nNumberOfBytesToWrite;
    if ( !API_WriteFile((intptr_t)((intptr_t)F32(c, 0xc)), (intptr_t)(lpBuffer), (intptr_t)(nNumberOfBytesToWrite), (intptr_t)(&nNumberOfBytesToWrite), (intptr_t)(0))
      || nNumberOfBytesToWrite != v4 )
    {
      fs_5b64f0(c);
      return 0;
    }
  }
  return 1;
}

/* 0x5b64b0: int __thiscall sub_5B64B0(cls_5B64B0 *this) */
int32_t fs_5b64b0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x0) != 1 )
  {
    if ( F32(c, 0x0) != 2 )
    {
      API_CloseHandle((intptr_t)((intptr_t)F32(c, 0xc)));
      return fs_5b64e0(c);
    }
    fs_5b6630(F32(c, 0x10));
  }
  return fs_5b64e0(c);
}

/* 0x5b64e0: int __thiscall sub_5B64E0(cls_5B64B0 *this) */
int32_t fs_5b64e0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x0) = 0;
  F32(c, 0x4) = 0;
  F32(c, 0x8) = 0;
  F32(c, 0xc) = 0;
  return 0;
}

/* 0x5b64f0: DWORD __thiscall sub_5B64F0(cls_41A890 *this) */
int32_t fs_5b64f0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint32_t LastError = 0;

  LastError = API_GetLastError();
  return API_FormatMessageA((intptr_t)(0x1000u), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(&GAP(c, 0x10, 4)), (intptr_t)(0x3FFu), (intptr_t)(0));
}

/* 0x5b6520: int __thiscall sub_5B6520(cls_5B6520 *this, const char *a2) */
int32_t fs_5b6520(wl_char *self, char * a2)
{
    intptr_t c = (intptr_t)self;

  strcpy((uint8_t *)c + 36, ((char *)D(0x8a47a4)));
  strcat((uint8_t *)c + 36, a2);
  return 0;
}

/* 0x5b6580: _DWORD *__thiscall sub_5B6580(cls_5B6580 *this) */
int32_t fs_5b6580(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x0) = 0;
  F32(c, 0x4) = 0;
  F32(c, 0x8) = 0;
  GAP(c, 0x24, 0) = 0;
  F32(c, 0x18) = 0;
  F32(c, 0xc) = 0;
  F32(c, 0x20) = 0;
  F32(c, 0x1c) = 0;
  return &F32(c, 0x0);
}

/* 0x5b65a0: int __thiscall sub_5B65A0(cls_5B65A0 *this) */
int32_t fs_5b65a0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  fs_5b6940(c);
  return result;
}

/* 0x5b65b0: int __thiscall sub_5B65B0(cls_5B6520 *this, int a2, int a3) */
int32_t fs_5b65b0(wl_char *self, int32_t a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    void * v4 = 0;
    intptr_t v6 = 0;

  F32(c, 0x4) = a2;
  F32(c, 0x8) = a3;
  v4 = (intptr_t)g_eng.op_new(4 * a3);
  F32(c, 0x0) = (int)v4;
  if ( v4 == 0 )
  {
LABEL_2:
    fs_5b6520(c, ((char *)D(0x8a47b8)));
    return 0;
  }
  v6 = 0;
  if ( F32(c, 0x8) != 0 )
  {
    do
    {
      F32((F32(c, 0x0) + 4 * v6), 0) = (intptr_t)g_eng.op_new(F32(c, 0x4));
      if ( F32((F32(c, 0x0) + 4 * v6), 0) == 0 )
        goto LABEL_2;
    }
    while ( (unsigned int)++v6 < F32(c, 0x8) );
  }
  F32(c, 0x14) = 0;
  fs_5b6db0(c, 0);
  return 1;
}

/* 0x5b6630: int __thiscall sub_5B6630(cls_5B6520 *this) */
int32_t fs_5b6630(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x14) == 0 )
    return 0;
  F32(c, 0x14) = 0;
  return 1;
}

/* 0x5b6650: int __thiscall sub_5B6650(cls_5B6520 *this) */
int32_t fs_5b6650(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x0) != 0 )
  {
    if ( F32(c, 0x14) != 0 )
    {
      fs_5b6520(c, ((uint8_t *)D(0x8a47f8)));
      return 0;
    }
    else if ( F32(c, 0x18) != 0 )
    {
      F32(c, 0x10) = 0;
      F32(c, 0x14) = 1;
      return 1;
    }
    else
    {
      fs_5b6520(c, ((uint8_t *)D(0x8a47d8)));
      return 0;
    }
  }
  else
  {
    fs_5b6520(c, ((uint8_t *)D(0x8a4820)));
    return 0;
  }
}

/* 0x5b66a0: int __thiscall sub_5B66A0(cls_5B6520 *this) */
int32_t fs_5b66a0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x18) = 0;
  F32(c, 0xc) = 0;
  return 1;
}

/* 0x5b66b0: int __thiscall sub_5B66B0(cls_5B6520 *this) */
int32_t fs_5b66b0(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x0) != 0 )
  {
    if ( F32(c, 0x14) != 0 )
    {
      fs_5b6520(c, ((uint8_t *)D(0x8a47f8)));
      return 0;
    }
    else
    {
      F32(c, 0x14) = 2;
      F32(c, 0xc) = 0;
      F32(c, 0x18) = 1;
      return 1;
    }
  }
  else
  {
    fs_5b6520(c, ((uint8_t *)D(0x8a4820)));
    return 0;
  }
}

/* 0x5b66f0: int __thiscall sub_5B66F0(cls_5B6520 *this, int a2) */
int32_t fs_5b66f0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint32_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    uint32_t v6 = 0;
    intptr_t v7 = 0;
    uint32_t v8 = 0;
    int32_t * v9 = 0;
    uint32_t i = 0;
    uint32_t v12 = 0;
    int32_t * v13 = 0;
    void * v14 = 0;
    intptr_t v15 = 0;

  v3 = F32(c, 0x4);
  v4 = F32(c, 0x8);
  v5 = F32(c, 0xc);
  if ( v5 + a2 > v4 * v3 )
  {
    v6 = a2 + v5 - v4 * v3;
    v7 = v6 / v3;
    if ( v6 % v3 != 0 )
      ++v7;
    v8 = v7 + v4;
    v9 = (intptr_t)g_eng.op_new(4 * v8);
    v15 = (int)v9;
    if ( v9 == 0 )
    {
LABEL_5:
      fs_5b6520(c, ((char *)D(0x8a4844)));
      return 0;
    }
    for ( i = 0; i < F32(c, 0x8); v9[i - 1] = F32((F32(c, 0x0) + 4 * i - 4), 0) )
      ++i;
    v12 = F32(c, 0x8);
    if ( v12 < v8 )
    {
      v13 = &v9[v12];
      do
      {
        v14 = (intptr_t)g_eng.op_new(F32(c, 0x4));
        *v13 = v14;
        if ( v14 == 0 )
          goto LABEL_5;
        ++v12;
        ++v13;
      }
      while ( v12 < v8 );
    }
    g_eng.op_delete((void *)(void *)F32(c, 0x0));
    F32(c, 0x0) = v15;
    F32(c, 0x8) = v8;
  }
  return 1;
}

/* 0x5b67c0: unsigned int __thiscall sub_5B67C0(cls_5B6520 *this, int a2, unsigned int a3, unsigned int a4, int a5) */
int32_t fs_5b67c0(wl_char *self, int32_t a2, uint32_t a3, uint32_t a4, int32_t a5)
{
    intptr_t c = (intptr_t)self;
    uint32_t v7 = 0;
    intptr_t v8 = 0;
    uint32_t result = 0;
    uint32_t v10 = 0;
    uint32_t v11 = 0;
    intptr_t v12 = 0;
    void * v13 = 0;
    uint32_t v15 = 0;
    uint32_t i = 0;
    _Alignas(8) uint8_t frame_[0x4 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x4 */

  v7 = F32(c, 0x4);
  v8 = 0;
  (*v14__f) = 0;
  v15 = a3 / v7;
  result = a3 / v7;
  v10 = a3 % v7;
  v11 = v7 - a3 % v7;
  if ( a4 != 0 )
  {
    v12 = 4 * v15;
    result = a4;
    for ( i = 4 * v15; ; v12 = i )
    {
      if ( v11 < result )
        result = v11;
      if ( a5 != 0 )
      {
        v13 = (const void *)(v8 + a2);
        v8 = (*v14__f);
        qmemcpy((void *)(v10 + F32((v12 + F32(c, 0x0)), 0)), v13, result);
      }
      else
      {
        qmemcpy((void *)(a2 + v8), (const void *)(v10 + F32((v12 + F32(c, 0x0)), 0)), result);
      }
      v11 = F32(c, 0x4);
      v8 += result;
      v10 = 0;
      a4 -= result;
      (*v14__f) = v8;
      i += 4;
      if ( a4 == 0 )
        break;
      result = a4;
    }
  }
  return result;
}

/* 0x5b6890: int __thiscall sub_5B6890(cls_5B6520 *this, int a2, unsigned int a3) */
int32_t fs_5b6890(wl_char *self, int32_t a2, uint32_t a3)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x14) == 2 )
  {
    if ( fs_5b66f0(c, a3) != 0 )
    {
      fs_5b67c0(c, a2, F32(c, 0xc), a3, 1);
      F32(c, 0xc) += a3;
      return 1;
    }
    else
    {
      fs_5b66a0(c);
      return 0;
    }
  }
  else
  {
    fs_5b6520(c, ((uint8_t *)D(0x8a4864)));
    return 0;
  }
}

/* 0x5b68f0: int __thiscall sub_5B68F0(cls_5B6520 *this, int a2, unsigned int a3) */
int32_t fs_5b68f0(wl_char *self, int32_t a2, uint32_t a3)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x14) == 1 )
  {
    fs_5b67c0(c, a2, F32(c, 0x10), a3, 0);
    F32(c, 0x10) += a3;
    return 1;
  }
  else
  {
    fs_5b6520(c, ((uint8_t *)D(0x8a4894)));
    return 0;
  }
}

/* 0x5b6940: void __thiscall sub_5B6940(cls_5B6940 *this) */
void fs_5b6940(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint32_t i = 0;
    void * v3 = 0;

  if ( F32(c, 0x0) != 0 )
  {
    for ( i = 0; i < F32(c, 0x8); ++i )
      g_eng.op_delete((void *)F32(F32(c, 0x0), 4 * i));
    g_eng.op_delete((void *)(void *)F32(c, 0x0));
    F32(c, 0x0) = 0;
    v3 = (void *)F32(c, 0x20);
    if ( v3 != 0 )
      g_eng.op_delete((void *)v3);
  }
}

/* 0x5b6990: int __thiscall sub_5B6990(cls_5B6520 *this, LPCSTR lpFileName, int a3) */
int32_t fs_5b6990(wl_char *self, char * lpFileName, int32_t a3)
{
    intptr_t c = (intptr_t)self;
    uint32_t v4 = 0;
    uint8_t * v5 = 0;
    intptr_t v7 = 0;
    intptr_t v8 = 0;
    int v9 = 0;
    intptr_t v10 = 0;
    uint32_t i = 0;
    _Alignas(8) uint8_t frame_[0x434 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    intptr_t *v17 = (intptr_t *)(frame_ + 0x14);   /* [5] at ebp-0x420 */
    char *v18 = (char *)(frame_ + 0x28);   /* [1024] at ebp-0x40c */
    uint32_t *const Buffer__f = (uint32_t *)(frame_ + 0x0);   /* ebp-0x434 */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x4);   /* ebp-0x430 */
    uint32_t *const nNumberOfBytesToWrite__f = (uint32_t *)(frame_ + 0x8);   /* ebp-0x42c */
    intptr_t *const v15__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x428 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x424 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x430);   /* ebp-0x4 */

  fs_5b6060(v17);
  (*v19__f) = 0;
  if ( fs_5b60e0(v17, lpFileName, a3) == 0 )
  {
    fs_5b6520(c, v18);
LABEL_8:
    (*v19__f) = -1;
    fs_5b60d0(v17);
    return 0;
  }
  (*Buffer__f) = 16;
  fs_5b6410(v17, &(*Buffer__f), 4u);
  v4 = F32(c, 0xc);
  (*v13__f) = 10001;
  (*nNumberOfBytesToWrite__f) = v4;
  if ( v4 != 0 )
  {
    v5 = (intptr_t)g_eng.op_new(v4);
    if ( v5 == 0 )
    {
      fs_5b6520(c, ((char *)D(0x8a48c4)));
      goto LABEL_8;
    }
    if ( fs_5b6650(c) == 0 || fs_5b68f0(c, (int)v5, (*nNumberOfBytesToWrite__f)) == 0 )
      goto LABEL_8;
    fs_5b6630(c);
    (*v15__f) = W_bs_5b6e40((intptr_t)((int)v5), (intptr_t)((*nNumberOfBytesToWrite__f)));
  }
  else
  {
    v5 = (_BYTE *)(*Buffer__f);
  }
  if ( F32(c, 0x1c) != 0 )
  {
    v7 = (0x7FFF * rnd() / 0x8000) << 16;
    v8 = 0x7FFF * rnd() / 0x8000;
    v9 = v8 + v7 == 0;
    v10 = v8 + v7;
    (*v16__f) = v10;
    if ( v9 )
    {
      SETB1(v10, 0);
      (*v16__f) = 1;
    }
    for ( i = 0; i < (*nNumberOfBytesToWrite__f); ++i )
      v5[i] = BYTE1(v10) + F8((F32(c, 0x1c) + (uint8_t)v5[i]), 0);
  }
  else
  {
    (*v16__f) = 0;
  }
  fs_5b6410(v17, &(*v13__f), (*Buffer__f));
  if ( (*nNumberOfBytesToWrite__f) != 0 )
  {
    fs_5b6410(v17, v5, (*nNumberOfBytesToWrite__f));
    g_eng.op_delete((void *)v5);
  }
  fs_5b64b0(v17);
  (*v19__f) = -1;
  fs_5b60d0(v17);
  return 1;
}

/* 0x5b6bc0: int __thiscall sub_5B6BC0(cls_5B6520 *this, LPCSTR lpFileName) */
int32_t fs_5b6bc0(wl_char *self, char * lpFileName)
{
    intptr_t c = (intptr_t)self;
    uint8_t * v3 = 0;
    uint8_t * v4 = 0;
    uint32_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;
    intptr_t v8 = 0;
    _Alignas(8) uint8_t frame_[0x434 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *v10 = (char *)(frame_ + 0x0);   /* [4] at ebp-0x434 */
    intptr_t *v15 = (intptr_t *)(frame_ + 0x14);   /* [5] at ebp-0x420 */
    char *v16 = (char *)(frame_ + 0x28);   /* [1024] at ebp-0x40c */
    uint32_t *const nNumberOfBytesToRead__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x430 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x42c */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x428 */
    uint32_t *const Buffer__f = (uint32_t *)(frame_ + 0x10);   /* ebp-0x424 */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x430);   /* ebp-0x4 */

  fs_5b6060(v15);
  (*v17__f) = 0;
  if ( fs_5b61b0(v15, lpFileName) == 0 )
  {
    fs_5b6520(c, v16);
LABEL_17:
    (*v17__f) = -1;
    fs_5b60d0(v15);
    return 0;
  }
  fs_5b6340(v15, &(*Buffer__f), 4u);
  fs_5b6340(v15, v10, (*Buffer__f));
  if ( (*nNumberOfBytesToRead__f) != 0 )
  {
    v3 = (intptr_t)g_eng.op_new((*nNumberOfBytesToRead__f));
    v4 = v3;
    if ( v3 == 0 )
    {
      fs_5b6520(c, ((char *)D(0x8a48c4)));
      goto LABEL_17;
    }
    fs_5b6340(v15, v3, (*nNumberOfBytesToRead__f));
    if ( F32(c, 0x20) != 0 && (*v13__f) != 0 )
    {
      v5 = 0;
      if ( (*nNumberOfBytesToRead__f) != 0 )
      {
        v6 = BYTE1((*v13__f));
        do
        {
          v7 = (uint8_t)(v4[v5] - v6);
          if ( (uint8_t)v4[v5] - v6 + 256 < 0 )
            v7 = ((uint8_t)(v7 - 1) | 0xFF00) + 1;
          v4[v5++] = F8((v7 + F32(c, 0x20)), 0);
        }
        while ( v5 < (*nNumberOfBytesToRead__f) );
      }
    }
    v8 = W_bs_5b6e40((intptr_t)((int)v4), (intptr_t)((*nNumberOfBytesToRead__f)));
    if ( v8 != (*v12__f) )
    {
      fs_5b6520(c, ((uint8_t *)D(0x8a48e0)));
      g_eng.op_delete((void *)v4);
      goto LABEL_17;
    }
    if ( fs_5b66b0(c) == 0 || fs_5b6890(c, (int)v4, (*nNumberOfBytesToRead__f)) == 0 )
      goto LABEL_17;
    fs_5b6630(c);
    g_eng.op_delete((void *)v4);
  }
  fs_5b64b0(v15);
  (*v17__f) = -1;
  fs_5b60d0(v15);
  return 1;
}

/* 0x5b6db0: int __thiscall sub_5B6DB0(cls_5B6520 *this, int a2) */
int32_t fs_5b6db0(wl_char *self, int32_t a2)
{
    intptr_t c = (intptr_t)self;
    uint16_t v3 = 0;
    uint8_t * v4 = 0;
    int16_t v5 = 0;
    int16_t i = 0;
    intptr_t v7 = 0;

  F32(c, 0x1c) = a2;
  if ( a2 == 0 )
    return 1;
  if ( F32(c, 0x20) == 0 )
    F32(c, 0x20) = (int)(intptr_t)g_eng.op_new(0x100u);
  v3 = 0;
  while ( 1 )
  {
    v4 = (uint8_t *)F32(c, 0x1c);
    v5 = 256;
    for ( i = 0; (uint16_t)i < 0x100u; ++i )
    {
      if ( v3 == *v4 )
        v5 = i;
      ++v4;
    }
    if ( (uint16_t)v5 >= 0x100u )
      break;
    v7 = v3++;
    F8((v7 + F32(c, 0x20)), 0) = v5;
    if ( v3 >= 0x100u )
      return 1;
  }
  fs_5b6520(c, ((char *)D(0x8a48fc)));
  return 0;
}
/* unported callees: none */
