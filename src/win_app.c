/* win_app.c — the application shell under WinMain: the config table + its scrambled file 0x5afb00..0x5afc90, the
 * key=value text file 0x5affe0 / 0x5aff20, the OS / exe version checks 0x5b0280 / 0x5b0450 / 0x5b0390 / 0x5b03e0 /
 * 0x5b5af0, the asset DLL load 0x5b0890, the paths 0x5b0c60 / 0x5b0ce0 / 0x5b0dd0, the launcher dialog 0x5b0d70, the
 * window 0x5b0ee0 / 0x5b01e0, the message PUMP 0x5b1030, the shutdown 0x5b10d0, the WINDOW PROCEDURE 0x5b12e0 (+ the
 * lost-surface restore 0x5b14c0 on activation), the single-instance mutex 0x5b5a60 / 0x5b5a90, the busy wait 0x5b5ac0,
 * the GDI helpers (brush 0x5b5d50, font 0x5b5dc0, outline 0x5b5ef0, colour lerp 0x5b5f50, RGBQUAD 0x5b5d90).  See
 * win_app.h.
 *
 * A FIRST PASS through tools/ida/decomp2c.py (--typed --zero-locals); no hand finish.  The launcher's dialog
 * procedure DialogFunc 0x4013c0 is passed as retail's own (rhost shows no dialog). */
#include "win_app.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine_calls.h"
#include "rng.h"

#define W ((intptr_t)*g_eng.world)
#define D(va) ((intptr_t)g_eng.data(va))


#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "bitmap_session.h"
#include "file_stream.h"
#include "frame_load.h"
#include "input_dev.h"
#include "render_platform.h"
#include "sound_dev.h"
#include "title_menu.h"
#include "win_main.h"
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

static inline intptr_t W_bs_5b6e80(intptr_t a0, ...) { bs_5b6e80((wl_char *)a0); return 0; }
static inline intptr_t W_fl_4184a0(intptr_t a0, intptr_t a1, ...) { fl_4184a0((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_fs_5b6060(intptr_t a0, ...) { return (intptr_t)fs_5b6060((wl_char *)a0); }
static inline intptr_t W_fs_5b60d0(intptr_t a0, ...) { return (intptr_t)fs_5b60d0((wl_char *)a0); }
static inline intptr_t W_fs_5b61b0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fs_5b61b0((wl_char *)a0, (char *)a1); }
static inline intptr_t W_fs_5b6200(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fs_5b6200((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_fs_5b6340(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b6340((wl_char *)a0, (int32_t)a1, (uint32_t)a2); }
static inline intptr_t W_fs_5b64b0(intptr_t a0, ...) { return (intptr_t)fs_5b64b0((wl_char *)a0); }
static inline intptr_t W_fs_5b6580(intptr_t a0, ...) { return (intptr_t)fs_5b6580((wl_char *)a0); }
static inline intptr_t W_fs_5b65a0(intptr_t a0, ...) { return (intptr_t)fs_5b65a0((wl_char *)a0); }
static inline intptr_t W_fs_5b65b0(intptr_t a0, intptr_t a1, intptr_t a2, ...) { return (intptr_t)fs_5b65b0((wl_char *)a0, (int32_t)a1, (int32_t)a2); }
static inline intptr_t W_fs_5b6bc0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fs_5b6bc0((wl_char *)a0, (char *)a1); }
static inline intptr_t W_fs_5b6db0(intptr_t a0, intptr_t a1, ...) { return (intptr_t)fs_5b6db0((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_id_5ba290(intptr_t a0, ...) { return (intptr_t)id_5ba290((wl_char *)a0); }
static inline intptr_t W_rp_5b9390(intptr_t a0, ...) { return (intptr_t)rp_5b9390((wl_char *)a0); }
static inline intptr_t W_sd_5bbd20(intptr_t a0, intptr_t a1, ...) { sd_5bbd20((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_tm_5658c0(intptr_t a0, ...) { return (intptr_t)tm_5658c0((wl_char *)a0); }
static inline intptr_t W_wn_5624c0(intptr_t a0, ...) { wn_5624c0((wl_char *)a0); return 0; }
static inline intptr_t W_wn_58ffa0(intptr_t a0, intptr_t a1, ...) { wn_58ffa0((wl_char *)a0, (int32_t)a1); return 0; }
static inline intptr_t W_wn_5b9ab0(intptr_t a0, ...) { return (intptr_t)wn_5b9ab0((wl_char *)a0); }
static inline intptr_t W_wn_5b9ac0(intptr_t a0, ...) { return (intptr_t)wn_5b9ac0((wl_char *)a0); }
static inline intptr_t W_zd_5b9130(intptr_t a0, intptr_t a1, ...) { return (intptr_t)zd_5b9130((wl_char *)a0, (int32_t)a1); }
static inline intptr_t W_zd_5b91d0(intptr_t a0, ...) { return (intptr_t)zd_5b91d0((wl_char *)a0); }
static inline intptr_t W_zd_5b9240(intptr_t a0, ...) { return (intptr_t)zd_5b9240((wl_char *)a0); }

typedef intptr_t (__attribute__((stdcall)) *api_CloseHandle_t)(intptr_t);
#define API_CloseHandle ((api_CloseHandle_t)g_eng.api("CloseHandle"))
typedef intptr_t (__attribute__((stdcall)) *api_CoUninitialize_t)(void);
#define API_CoUninitialize ((api_CoUninitialize_t)g_eng.api("CoUninitialize"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateBrushIndirect_t)(intptr_t);
#define API_CreateBrushIndirect ((api_CreateBrushIndirect_t)g_eng.api("CreateBrushIndirect"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateFontIndirectA_t)(intptr_t);
#define API_CreateFontIndirectA ((api_CreateFontIndirectA_t)g_eng.api("CreateFontIndirectA"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateMutexA_t)(intptr_t, intptr_t, intptr_t);
#define API_CreateMutexA ((api_CreateMutexA_t)g_eng.api("CreateMutexA"))
typedef intptr_t (__attribute__((stdcall)) *api_CreateWindowExA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_CreateWindowExA ((api_CreateWindowExA_t)g_eng.api("CreateWindowExA"))
typedef intptr_t (__attribute__((stdcall)) *api_DefWindowProcA_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_DefWindowProcA ((api_DefWindowProcA_t)g_eng.api("DefWindowProcA"))
typedef intptr_t (__attribute__((stdcall)) *api_DeleteObject_t)(intptr_t);
#define API_DeleteObject ((api_DeleteObject_t)g_eng.api("DeleteObject"))
typedef intptr_t (__attribute__((stdcall)) *api_DestroyWindow_t)(intptr_t);
#define API_DestroyWindow ((api_DestroyWindow_t)g_eng.api("DestroyWindow"))
typedef intptr_t (__attribute__((stdcall)) *api_DialogBoxParamA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_DialogBoxParamA ((api_DialogBoxParamA_t)g_eng.api("DialogBoxParamA"))
typedef intptr_t (__attribute__((stdcall)) *api_DispatchMessageA_t)(intptr_t);
#define API_DispatchMessageA ((api_DispatchMessageA_t)g_eng.api("DispatchMessageA"))
typedef uint32_t (__attribute__((stdcall)) *api_FormatMessageA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_FormatMessageA ((api_FormatMessageA_t)g_eng.api("FormatMessageA"))
typedef intptr_t (__attribute__((stdcall)) *api_FreeLibrary_t)(intptr_t);
#define API_FreeLibrary ((api_FreeLibrary_t)g_eng.api("FreeLibrary"))
typedef intptr_t (__attribute__((stdcall)) *api_GetFileVersionInfoA_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_GetFileVersionInfoA ((api_GetFileVersionInfoA_t)g_eng.api("GetFileVersionInfoA"))
typedef uint32_t (__attribute__((stdcall)) *api_GetFileVersionInfoSizeA_t)(intptr_t, intptr_t);
#define API_GetFileVersionInfoSizeA ((api_GetFileVersionInfoSizeA_t)g_eng.api("GetFileVersionInfoSizeA"))
typedef uint32_t (__attribute__((stdcall)) *api_GetLastError_t)(void);
#define API_GetLastError ((api_GetLastError_t)g_eng.api("GetLastError"))
typedef uint32_t (__attribute__((stdcall)) *api_GetModuleFileNameA_t)(intptr_t, intptr_t, intptr_t);
#define API_GetModuleFileNameA ((api_GetModuleFileNameA_t)g_eng.api("GetModuleFileNameA"))
typedef uint32_t (__attribute__((stdcall)) *api_GetTickCount_t)(void);
#define API_GetTickCount ((api_GetTickCount_t)g_eng.api("GetTickCount"))
typedef intptr_t (__attribute__((stdcall)) *api_GetVersionExA_t)(intptr_t);
#define API_GetVersionExA ((api_GetVersionExA_t)g_eng.api("GetVersionExA"))
typedef intptr_t (__attribute__((stdcall)) *api_LoadLibraryA_t)(intptr_t);
#define API_LoadLibraryA ((api_LoadLibraryA_t)g_eng.api("LoadLibraryA"))
typedef intptr_t (__attribute__((stdcall)) *api_MessageBoxA_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_MessageBoxA ((api_MessageBoxA_t)g_eng.api("MessageBoxA"))
typedef intptr_t (__attribute__((stdcall)) *api_PeekMessageA_t)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
#define API_PeekMessageA ((api_PeekMessageA_t)g_eng.api("PeekMessageA"))
typedef intptr_t (__attribute__((stdcall)) *api_Polygon_t)(intptr_t, intptr_t, intptr_t);
#define API_Polygon ((api_Polygon_t)g_eng.api("Polygon"))
typedef intptr_t (__attribute__((stdcall)) *api_PostQuitMessage_t)(intptr_t);
#define API_PostQuitMessage ((api_PostQuitMessage_t)g_eng.api("PostQuitMessage"))
typedef intptr_t (__attribute__((stdcall)) *api_ReleaseMutex_t)(intptr_t);
#define API_ReleaseMutex ((api_ReleaseMutex_t)g_eng.api("ReleaseMutex"))
typedef intptr_t (__attribute__((stdcall)) *api_TranslateMessage_t)(intptr_t);
#define API_TranslateMessage ((api_TranslateMessage_t)g_eng.api("TranslateMessage"))
typedef intptr_t (__attribute__((stdcall)) *api_UnregisterClassA_t)(intptr_t, intptr_t);
#define API_UnregisterClassA ((api_UnregisterClassA_t)g_eng.api("UnregisterClassA"))
typedef intptr_t (__attribute__((stdcall)) *api_VerQueryValueA_t)(intptr_t, intptr_t, intptr_t, intptr_t);
#define API_VerQueryValueA ((api_VerQueryValueA_t)g_eng.api("VerQueryValueA"))
typedef intptr_t (__attribute__((stdcall)) *api_WaitMessage_t)(void);
#define API_WaitMessage ((api_WaitMessage_t)g_eng.api("WaitMessage"))
#define CRT_fclose ((intptr_t (*)())g_eng.retail(0x5bf440u))   /* retail's CRT fclose (cdecl) */
#define CRT_fopen ((intptr_t (*)())g_eng.retail(0x5bf4e8u))   /* retail's CRT fopen (cdecl) */
#define CRT_fscanf ((intptr_t (*)())g_eng.retail(0x5c08f1u))   /* retail's CRT fscanf (cdecl) */
#define CRT_fseek ((intptr_t (*)())g_eng.retail(0x5c0865u))   /* retail's CRT fseek (cdecl) */
int32_t wm_5afb00(wl_char *self, char * a2, uint16_t a3, int32_t a4);
int32_t wm_5afb90(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, void * a5);
int32_t wm_5afbf0(wl_char *self, uint16_t a2, int32_t a3, int32_t a4);
int32_t wm_5afc90(wl_char *self);
void wm_5aff00(wl_char *self);
int32_t wm_5aff20(wl_char *self, char * a2, char * a3);
int32_t wm_5affe0(wl_char *self, char * FileName, uint32_t a3);
void wm_5b01e0(wl_char *self, int32_t hInstance);
int32_t wm_5b0260(wl_char *self, char a2);
int32_t wm_5b0280(wl_char *self, OSVERSIONINFOA * lpVersionInformation);
int32_t wm_5b0390(int32_t * a1, int32_t * a2);
int32_t wm_5b03e0(uint16_t * a1, uint16_t * a2);
int32_t wm_5b0450(wl_char *self, char * lptstrFilename, uint16_t * a3);
int32_t wm_5b0890(char * a1, int32_t a2, int32_t a3);
int32_t wm_5b0c60(wl_char *self, char * a2, char * a3);
int32_t wm_5b0ce0(wl_char *self, char * a2, char * a3);
int32_t wm_5b0d70(wl_char *self, int32_t hInstance, char * a3);
int32_t wm_5b0dd0(wl_char *self, char * a2, int32_t a3);
int32_t wm_5b0ee0(wl_char *self, int32_t hInstance);
int32_t wm_5b1030(void);
void wm_5b10d0(void);
int32_t wm_5b12e0(int32_t hWnd, uint32_t Msg, uint32_t wParam, int32_t lParam);
void wm_5b14c0(wl_char *self);
int32_t wm_5b5a60(wl_char *self, char * lpName);
int32_t wm_5b5a90(wl_char *self);
int32_t wm_5b5ac0(uint32_t a1);
int32_t wm_5b5af0(int32_t a1);
int32_t wm_5b5d50(uint32_t a1);
int32_t wm_5b5d90(int32_t a1, int32_t a2);
int32_t wm_5b5dc0(int32_t a1, int32_t a2, int32_t a3);
int32_t wm_5b5ef0(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5);
int32_t wm_5b5f50(uint32_t a1, int32_t a2, int32_t a3, int32_t a4);

/* 0x5afb00: unsigned int __thiscall sub_5AFB00(cls_5AFB00 *this, const char *a2, unsigned __int16 a3, int a4) */
int32_t wm_5afb00(wl_char *self, char * a2, uint16_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    void * v5 = 0;
    uint16_t v6 = 0;
    uint32_t result = 0;

  F16(c, 0x4) = a3;
  v5 = (intptr_t)g_eng.op_new(28 * a3);
  v6 = F16(c, 0x4);
  F32(c, 0x0) = (int)v5;
  memset(v5, 0, 4 * ((28 * (unsigned int)v6) >> 2));
  result = strlen(a2) + 1;
  qmemcpy(((uint8_t *)(c) + 0x6), a2, result);
  F32(c, 4 * 67) = a4;
  return result;
}

/* 0x5afb90: void *__thiscall sub_5AFB90(cls_5AFB90 *this, unsigned __int16 a2, int a3, int a4, void *a5) */
int32_t wm_5afb90(wl_char *self, uint16_t a2, int32_t a3, int32_t a4, void * a5)
{
    intptr_t c = (intptr_t)self;
    intptr_t v5 = 0;
    void * result = 0;

  v5 = F32(c, 0x0) + 28 * a2;
  F32(v5, 0) = a3;
  F32(v5, 4) = a4;
  F32(v5, 8) = a4;
  F32(v5, 12) = a4;
  result = a5;
  F16(v5, 20) = (uint16_t)a5;
  F16(v5, 22) = 0;
  F32(v5, 24) = 0;
  if ( (uint16_t)a5 != 0 )
  {
    result = (intptr_t)g_eng.op_new(8 * (uint16_t)a5);
    F32(v5, 16) = result;
  }
  else
  {
    F32(v5, 16) = 0;
  }
  return result;
}

/* 0x5afbf0: int __thiscall sub_5AFBF0(cls_5AFBF0 *this, unsigned __int16 a2, int a3, int a4) */
int32_t wm_5afbf0(wl_char *self, uint16_t a2, int32_t a3, int32_t a4)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    uint16_t v5 = 0;
    intptr_t v6 = 0;
    int32_t * v8 = 0;
    intptr_t v9 = 0;
    uint16_t v10 = 0;
    uint16_t v11 = 0;

  v4 = F32(c, 0x0);
  v5 = F16(F32(c, 0x0) + 28 * a2, 22);
  v6 = v4 + 28 * a2;
  if ( F16(v6, 20) <= v5 )
    return 0;
  v8 = (int *)(F32(v6, 16) + 8 * v5);
  *v8 = a3;
  v8[1] = a4;
  v9 = F32(v6, 16);
  v10 = ++F16(v6, 22);
  if ( v9 != 0 )
  {
    v11 = 0;
    if ( v10 != 0 )
    {
      while ( F32((v9 + 8 * v11), 0) != F32(v6, 8) )
      {
        if ( ++v11 >= v10 )
          return 1;
      }
      F32(v6, 24) = F32(v9 + 8 * v11, 4);
    }
  }
  return 1;
}

/* 0x5afc90: int __thiscall sub_5AFC90(cls_5AFC90 *this) */
int32_t wm_5afc90(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t v2 = 0;
    intptr_t v3 = 0;
    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t v6 = 0;
    uint16_t v7 = 0;
    intptr_t v8 = 0;
    intptr_t v9 = 0;
    int32_t * v10 = 0;
    intptr_t v11 = 0;
    intptr_t v12 = 0;
    uint16_t v13 = 0;
    uint16_t v14 = 0;
    _Alignas(8) uint8_t frame_[0x45c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    uint32_t *v24 = (uint32_t *)(frame_ + 0x2c);   /* [3] at ebp-0x430 */
    intptr_t *v26 = (intptr_t *)(frame_ + 0x3c);   /* [52] at ebp-0x420 */
    intptr_t *const v16__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x45c */
    intptr_t *const v17__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x44c */
    intptr_t *const v18__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x448 */
    intptr_t *const v19__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x444 */
    intptr_t *const v20__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0x440 */
    intptr_t *const v21__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x43c */
    intptr_t *const v22__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x438 */
    uint32_t *const Buffer__f = (uint32_t *)(frame_ + 0x28);   /* ebp-0x434 */
    uint32_t *const v25__f = (uint32_t *)(frame_ + 0x38);   /* ebp-0x424 */
    intptr_t *const v27__f = (intptr_t *)(frame_ + 0x458);   /* ebp-0x4 */

  W_fs_5b6060((intptr_t)(v26));
  v2 = F32(c, 0x10c);
  v3 = 0;
  v4 = 0;
  (*v27__f) = 0;
  (*v19__f) = 0;
  (*v17__f) = 0;
  v5 = 12195365;
  if ( v2 != 0 )
  {
    v5 = 12198962;
    v6 = (intptr_t)g_eng.op_new(0x124u);
    (*v18__f) = v6;
    SETLB((*v27__f), 1);
    if ( v6 != 0 )
      v3 = W_fs_5b6580((intptr_t)(v6));
    v4 = v3;
    SETLB((*v27__f), 0);
    (*v17__f) = v3;
    W_fs_5b65b0((intptr_t)(v3), (intptr_t)(512), (intptr_t)(1));
    W_fs_5b6db0((intptr_t)(v3), (intptr_t)(F32(c, 0x10c)));
    if ( W_fs_5b6bc0((intptr_t)(v3), (intptr_t)(&GAP(c, 0x4, 2))) == 0 || W_fs_5b6200((intptr_t)(v26), (intptr_t)((int)v3)) == 0 )
      goto LABEL_31;
  }
  else if ( W_fs_5b61b0((intptr_t)(v26), (intptr_t)(&GAP(c, 0x4, 2))) == 0 )
  {
    goto LABEL_31;
  }
  if ( W_fs_5b6340((intptr_t)(v26), (intptr_t)(&(*Buffer__f)), (intptr_t)(4u)) != 0
    && W_fs_5b6340((intptr_t)(v26), (intptr_t)(v24), (intptr_t)((*Buffer__f))) != 0
    && v24[0] == v5
    && v24[1] == 100001 )
  {
    (*v18__f) = 0;
    if ( (*v25__f) != 0 )
    {
      while ( W_fs_5b6340((intptr_t)(v26), (intptr_t)(&(*v21__f)), (intptr_t)(v24[2])) != 0 )
      {
        v7 = F16(((uint8_t *)(c) + 0x4), 0);
        v8 = 0;
        if ( v7 != 0 )
        {
          v9 = F32(c, 0x0);
          while ( 1 )
          {
            v10 = (_DWORD *)(v9 + 28 * (uint16_t)v8);
            if ( *v10 == (*v21__f) )
              break;
            if ( (uint16_t)++v8 >= v7 )
              goto LABEL_28;
          }
          if ( &(*v16__f) != (int *)-28 )
            (*v20__f) = v8;
          if ( v10 != 0 )
          {
            v11 = v9 + 28 * (uint16_t)(*v20__f);
            F32(v11, 8) = (*v22__f);
            v12 = F32(v11, 16);
            if ( v12 != 0 )
            {
              v13 = F16(v11, 22);
              v14 = 0;
              if ( v13 != 0 )
              {
                while ( F32((v12 + 8 * v14), 0) != (*v22__f) )
                {
                  if ( ++v14 >= v13 )
                    goto LABEL_28;
                }
                F32(v11, 24) = F32(v12 + 8 * v14, 4);
              }
            }
          }
        }
LABEL_28:
        (*v18__f) = ((uint8_t *)(*v18__f) + 1);
        if ( (uint16_t)(*v18__f) >= (*v25__f) )
          goto LABEL_29;
      }
    }
    else
    {
LABEL_29:
      W_fs_5b64b0((intptr_t)(v26));
      (*v19__f) = 1;
    }
    v4 = (*v17__f);
  }
LABEL_31:
  W_fs_5b64b0((intptr_t)(v26));
  if ( v4 != 0 )
  {
    W_fs_5b65a0((intptr_t)(v4));
    g_eng.op_delete((void *)v4);
  }
  (*v27__f) = -1;
  W_fs_5b60d0((intptr_t)(v26));
  return (*v19__f);
}

/* 0x5aff00: void __thiscall sub_5AFF00(cls_5AFF00 *this) */
void wm_5aff00(wl_char *self)
{
    intptr_t c = (intptr_t)self;

  if ( F32(c, 0x0) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x0));
    F32(c, 0x0) = 0;
  }
}

/* 0x5aff20: int __thiscall sub_5AFF20(cls_5AFF20 *this, const char *a2, char *a3) */
int32_t wm_5aff20(wl_char *self, char * a2, char * a3)
{
    intptr_t c = (intptr_t)self;
    uint32_t v3 = 0;
    uint32_t v4 = 0;
    char * v5 = 0;

  v3 = F32(c, 0x4);
  v4 = 0;
  if ( v3 != 0 )
  {
    v5 = (const char *)F32(c, 0x0);
    while ( strcmp(v5, a2) != 0 )
    {
      ++v4;
      v5 += 296;
      if ( v4 >= v3 )
        goto LABEL_5;
    }
    strcpy(a3, (const char *)(F32(c, 0x0) + 296 * v4 + 40));
    return 1;
  }
  else
  {
LABEL_5:
    strcpy(a3, D(0x8a9b6c));
    return 0;
  }
}

/* 0x5affe0: int __thiscall sub_5AFFE0(cls_5AFFE0 *this, char *FileName, unsigned int a3) */
int32_t wm_5affe0(wl_char *self, char * FileName, uint32_t a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v4 = 0;
    FILE * v5 = 0;
    FILE * v6 = 0;
    intptr_t v8 = 0;
    uint32_t v9 = 0;
    uint32_t v10 = 0;
    char v11 = 0;
    _Alignas(8) uint8_t frame_[0x408 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char * *const v12__f = (char * *)(frame_ + 0x0);   /* ebp-0x408 */
    FILE * *const v13__f = (FILE * *)(frame_ + 0x4);   /* ebp-0x404 */
    char *const v14__f = (char *)(frame_ + 0x8);   /* ebp-0x400 */

  v4 = 0;
  if ( F32(c, 0x0) != 0 )
  {
    g_eng.op_delete((void *)(void *)F32(c, 0x0));
    F32(c, 0x0) = 0;
  }
  F32(c, 0x4) = 0;
  v5 = CRT_fopen(FileName, ((char *)D(0x8a45cc)));
  v6 = v5;
  (*v13__f) = v5;
  if ( v5 == 0 )
    return 0;
  if ( CRT_fscanf(v5, "%1023s", &(*v14__f)) != -1 )
  {
    do
      ++F32(c, 0x4);
    while ( CRT_fscanf(v6, "%1023s", &(*v14__f)) != -1 );
  }
  if ( a3 != 0 && F32(c, 0x4) > a3 )
  {
    CRT_fclose(v6);
    return 0;
  }
  F32(c, 0x0) = (int)(intptr_t)g_eng.op_new(296 * F32(c, 0x4));
  CRT_fseek(v6, 0, 0);
  if ( CRT_fscanf(v6, "%1023s", &(*v14__f)) != -1 )
  {
    while ( 1 )
    {
      v8 = 0;
      v9 = 0;
      v10 = 0;
      if ( (*v14__f) != 0 )
        break;
LABEL_23:
      v4 += 296;
      if ( CRT_fscanf(v6, "%1023s", &(*v14__f)) == -1 )
        goto LABEL_24;
    }
    (*v12__f) = &(*v14__f);
    while ( 1 )
    {
      v11 = *(*v12__f);
      if ( *(*v12__f) == 61 )
        break;
      if ( v8 != 0 )
        goto LABEL_17;
      if ( v9 < 0x27 )
      {
        ++v9;
        F8((F32(c, 0x0) + v4 + v9 - 1), 0) = v11;
        F8((F32(c, 0x0) + v4 + v9), 0) = 0;
        goto LABEL_21;
      }
LABEL_22:
      if ( *++(*v12__f) == 0 )
        goto LABEL_23;
    }
    if ( v8 == 0 )
    {
      v8 = 1;
      goto LABEL_22;
    }
LABEL_17:
    if ( v10 < 0xFF )
    {
      ++v10;
      F8(F32(c, 0x0) + v4 + v10, 39) = v11;
      F8(F32(c, 0x0) + v4 + v10, 40) = 0;
LABEL_21:
      v6 = (*v13__f);
      goto LABEL_22;
    }
    goto LABEL_22;
  }
LABEL_24:
  CRT_fclose(v6);
  return 1;
}

/* 0x5b01e0: void __thiscall sub_5B01E0(cls_5B01E0 *this, HINSTANCE hInstance) */
void wm_5b01e0(wl_char *self, int32_t hInstance)
{
    intptr_t c = (intptr_t)self;
    void * v3 = 0;

  if ( F32(c, 0x0) != 0 )
  {
    API_DestroyWindow((intptr_t)((intptr_t)F32(c, 0x0)));
    F32(c, 0x0) = 0;
  }
  if ( F32(c, 0x4) != 0 )
  {
    API_DeleteObject((intptr_t)((intptr_t)F32(c, 0x4)));
    F32(c, 0x4) = 0;
  }
  if ( hInstance != 0 && F32(c, 0x214) != 0 )
  {
    API_UnregisterClassA((intptr_t)(((uint8_t *)(c) + 0x8)), (intptr_t)(hInstance));
    F32(c, 0x214) = 0;
  }
  v3 = (void *)F32(c, 0x108);
  if ( v3 != 0 )
  {
    W_bs_5b6e80((intptr_t)(F32(c, 0x108)));
    g_eng.op_delete((void *)v3);
    F32(c, 0x108) = 0;
  }
}

/* 0x5b0260: bitmap_session *__thiscall sub_5B0260(cls_5B0260 *this, char a2) */
int32_t wm_5b0260(wl_char *self, char a2)
{
    intptr_t c = (intptr_t)self;

  W_bs_5b6e80((intptr_t)(c));
  if ( (a2 & 1) != 0 )
    g_eng.op_delete((void *)c);
  return (bitmap_session *)c;
}

/* 0x5b0280: int __thiscall sub_5B0280(cls_5B0280 *this, LPOSVERSIONINFOA lpVersionInformation) */
int32_t wm_5b0280(wl_char *self, OSVERSIONINFOA * lpVersionInformation)
{
    intptr_t c = (intptr_t)self;
    uint32_t LastError = 0;
    _Alignas(8) uint8_t frame_[0x400 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x0);   /* [1024] at ebp-0x400 */

  memset(lpVersionInformation, 0, sizeof(OSVERSIONINFOA));
  lpVersionInformation->dwOSVersionInfoSize = 148;
  if ( API_GetVersionExA((intptr_t)(lpVersionInformation)) )
    return 1;
  strcpy((char *)c, ((char *)D(0x8a45e0)));
  strcat((char *)c, ((char *)D(0x8a45d0)));
  strcat((char *)c, D(0x8a43d8));
  LastError = API_GetLastError();
  API_FormatMessageA((intptr_t)(0x1000u), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0x3FFu), (intptr_t)(0));
  strcat((char *)c, Buffer);
  return 0;
}

/* 0x5b0390: BOOL __stdcall sub_5B0390(_DWORD *a1, _DWORD *a2) */
int32_t wm_5b0390(int32_t * a1, int32_t * a2)
{

    uint32_t v3 = 0;
    uint32_t v4 = 0;

  if ( a1[4] != a2[4] )
    return false;
  v3 = a1[1];
  v4 = a2[1];
  if ( v3 > v4 )
    return false;
  return v3 < v4 || a1[2] <= a2[2];
}

/* 0x5b03e0: BOOL __stdcall sub_5B03E0(_WORD *a1, _WORD *a2) */
int32_t wm_5b03e0(uint16_t * a1, uint16_t * a2)
{

    uint16_t v3 = 0;
    uint16_t v4 = 0;
    uint16_t v5 = 0;
    uint16_t v6 = 0;

  if ( *a1 < *a2 )
    return true;
  if ( *a1 > *a2 )
    return false;
  v3 = a1[1];
  v4 = a2[1];
  if ( v3 < v4 )
    return true;
  if ( v3 > v4 )
    return false;
  v5 = a1[2];
  v6 = a2[2];
  if ( v5 < v6 )
    return true;
  if ( v5 <= v6 )
    return a2[3] >= a1[3];
  return false;
}

/* 0x5b0450: int __thiscall sub_5B0450(cls_5B0450 *this, LPCSTR lptstrFilename, _WORD *a3) */
int32_t wm_5b0450(wl_char *self, char * lptstrFilename, uint16_t * a3)
{
    intptr_t c = (intptr_t)self;
    intptr_t v3 = 0;
    uint32_t FileVersionInfoSizeA = 0;
    uint32_t LastError = 0;
    void * v8 = 0;
    char * v9 = 0;
    uint32_t v10 = 0;
    char * v11 = 0;
    uint32_t v12 = 0;
    uint32_t v13 = 0;
    uint32_t v14 = 0;
    char * v15 = 0;
    uint32_t v16 = 0;
    intptr_t v17 = 0;
    intptr_t v18 = 0;
    _Alignas(8) uint8_t frame_[0x410 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Buffer = (char *)(frame_ + 0x10);   /* [1024] at ebp-0x400 */
    void * *const pBlock__f = (void * *)(frame_ + 0x0);   /* ebp-0x410 */
    uint32_t *const dwLen__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x40c */
    intptr_t *const lpBuffer__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x408 */
    uint32_t *const dwHandle__f = (uint32_t *)(frame_ + 0xc);   /* ebp-0x404 */

  v3 = 0;
  (*dwHandle__f) = 1;
  (*lpBuffer__f) = 0;
  FileVersionInfoSizeA = API_GetFileVersionInfoSizeA((intptr_t)(lptstrFilename), (intptr_t)(&(*dwHandle__f)));
  (*dwLen__f) = FileVersionInfoSizeA;
  if ( FileVersionInfoSizeA == 0 )
  {
    strcpy((char *)c, ((char *)D(0x8a45e0)));
    strcat((char *)c, ((char *)D(0x8a4640)));
    strcat((char *)c, D(0x8a43d8));
    LastError = API_GetLastError();
    API_FormatMessageA((intptr_t)(0x1000u), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0x3FFu), (intptr_t)(0));
    strcat((char *)c, Buffer);
    return 0;
  }
  v8 = (intptr_t)g_eng.op_new(FileVersionInfoSizeA);
  (*pBlock__f) = v8;
  if ( v8 == 0 )
  {
    strcpy((char *)c, ((char *)D(0x8a45e0)));
    v9 = ((char *)D(0x8a4630));
    v10 = strlen(((char *)D(0x8a4630))) + 1;
    v11 = (uint8_t *)c + strlen((const char *)c) + 1;
LABEL_5:
    qmemcpy(v11 - 1, v9, v10);
    goto LABEL_14;
  }
  if ( !API_GetFileVersionInfoA((intptr_t)(lptstrFilename), (intptr_t)(0), (intptr_t)((*dwLen__f)), (intptr_t)(v8)) )
  {
    strcpy((char *)c, ((char *)D(0x8a45e0)));
    strcat((char *)c, ((char *)D(0x8a461c)));
    strcat((char *)c, D(0x8a43d8));
    v12 = API_GetLastError();
    API_FormatMessageA((intptr_t)(0x1000u), (intptr_t)(0), (intptr_t)(v12), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0x3FFu), (intptr_t)(0));
    v9 = Buffer;
    v10 = strlen(Buffer) + 1;
    v11 = (uint8_t *)c + strlen((const char *)c) + 1;
    goto LABEL_5;
  }
  if ( API_VerQueryValueA((intptr_t)((*pBlock__f)), (intptr_t)(((char *)D(0x854f5c))), (intptr_t)(&(*lpBuffer__f)), (intptr_t)(&(*dwLen__f))) )
  {
    if ( (*lpBuffer__f) != 0 )
    {
      v17 = F32((*lpBuffer__f), 4 * 2);
      v18 = F32((*lpBuffer__f), 4 * 3);
      a3[1] = v17;
      *a3 = HIWORD(v17);
      a3[2] = HIWORD(v18);
      a3[3] = v18;
      v3 = 1;
      goto LABEL_14;
    }
    strcpy((char *)c, ((char *)D(0x8a45e0)));
    strcat((char *)c, ((char *)D(0x8a45f8)));
    strcat((char *)c, D(0x8a43d8));
    v16 = API_GetLastError();
    API_FormatMessageA((intptr_t)(0x1000u), (intptr_t)(0), (intptr_t)(v16), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0x3FFu), (intptr_t)(0));
    v14 = strlen(Buffer) + 1;
    v15 = (uint8_t *)c + strlen((const char *)c) + 1;
  }
  else
  {
    strcpy((char *)c, ((char *)D(0x8a45e0)));
    strcat((char *)c, ((char *)D(0x8a460c)));
    strcat((char *)c, D(0x8a43d8));
    v13 = API_GetLastError();
    API_FormatMessageA((intptr_t)(0x1000u), (intptr_t)(0), (intptr_t)(v13), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0x3FFu), (intptr_t)(0));
    v14 = strlen(Buffer) + 1;
    v15 = (uint8_t *)c + strlen((const char *)c) + 1;
  }
  qmemcpy(v15 - 1, Buffer, v14);
LABEL_14:
  if ( (*pBlock__f) != 0 )
    g_eng.op_delete((void *)(*pBlock__f));
  return v3;
}

/* 0x5b0890: HMODULE __stdcall sub_5B0890(const char *a1, int a2, int a3) */
int32_t wm_5b0890(char * a1, int32_t a2, int32_t a3)
{

    intptr_t i = 0;
    uint32_t v4 = 0;
    char * v5 = 0;
    char * v6 = 0;
    char * v7 = 0;
    char v8 = 0;
    intptr_t * v9 = 0;
    uint32_t v10 = 0;
    intptr_t LibraryA = 0;
    intptr_t * v12 = 0;
    uint32_t LastError = 0;
    uint32_t v15 = 0;
    uint32_t v16 = 0;
    char * v17 = 0;
    char * v18 = 0;
    char * v19 = 0;
    char v20 = 0;
    uint32_t v21 = 0;
    _Alignas(8) uint8_t frame_[0x710 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *LibFileName = (char *)(frame_ + 0x10);   /* [256] at ebp-0x700 */
    char *Buffer = (char *)(frame_ + 0x110);   /* [256] at ebp-0x600 */
    char *OutputString = (char *)(frame_ + 0x210);   /* [256] at ebp-0x500 */
    char *Filename = (char *)(frame_ + 0x310);   /* [1024] at ebp-0x400 */
    intptr_t *const SystemTime__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x710 */

  Filename[0] = F8(D(0x8a9b6c), 0);
  memset(&Filename[1], 0, 1023);
  strcpy(LibFileName, D(0x8a9b6c));
  if ( API_GetModuleFileNameA((intptr_t)(0), (intptr_t)(Filename), (intptr_t)(0x400u)) != 0 )
  {
    for ( i = strlen(Filename) - 1; i >= 0; Filename[i--] = 0 )
    {
      if ( Filename[i] == 92 )
        break;
    }
    strcpy(LibFileName, Filename);
    v4 = strlen(a1) + 1;
    v5 = &LibFileName[strlen(LibFileName)];
    qmemcpy(v5, a1, 4 * (v4 >> 2));
    v7 = &a1[4 * (v4 >> 2)];
    v6 = &v5[4 * (v4 >> 2)];
    v8 = v4;
  }
  else
  {
    g_eng.log_fail_406440(g_eng.log, (const char *)(((char *)D(0x8a2850))), (const char *)(D(0x8a9b6c)));
    v10 = strlen(a1) + 1;
    qmemcpy(LibFileName, a1, 4 * (v10 >> 2));
    v7 = &a1[4 * (v10 >> 2)];
    v6 = &LibFileName[4 * (v10 >> 2)];
    v8 = v10;
  }
  qmemcpy(v6, v7, v8 & 3);
  LibraryA = API_LoadLibraryA((intptr_t)(LibFileName));
  if ( a3 == 0 && LibraryA == 0 )
  {
    if ( a2 == 0 )
    {
      LastError = API_GetLastError();
      v15 = LastError;
      if ( LastError != 0 )
      {
        API_FormatMessageA((intptr_t)(0x12FFu), (intptr_t)(0), (intptr_t)(LastError), (intptr_t)(0), (intptr_t)(Buffer), (intptr_t)(0xFFu), (intptr_t)(0));
        g_eng.h5bf3ee(OutputString, "[%08d] %s", v15, Buffer);
        g_eng.log_fail_406440((const char *)D(0x8a9530), OutputString, D(0x8a9b6c));
      }
      F32(((uint8_t *)D(0x8a9530) + 0x000), 0) = 1;
      if ( strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) != 0 )
      {
        strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x854570)));
        v16 = strlen(((char *)D(0x8a2838))) + 1;
        v17 = &((char *)D(0x8a9530) + 0x404)[strlen((const char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]) - 1024];
        qmemcpy(v17, ((char *)D(0x8a2838)), 4 * (v16 >> 2));
        v19 = ((char *)D(0x8a2838) + (4 * (v16 >> 2)));
        v18 = &v17[4 * (v16 >> 2)];
        v20 = v16;
      }
      else
      {
        v21 = strlen(((char *)D(0x8a2838))) + 1;
        qmemcpy(&((uint8_t *)D(0x8a9530) + 0x000)[4], ((char *)D(0x8a2838)), 4 * (v21 >> 2));
        v19 = ((char *)D(0x8a2838) + (4 * (v21 >> 2)));
        v18 = &((char *)D(0x8a9530) + 0x404)[4 * (v21 >> 2) - 1024];
        v20 = v21;
      }
      qmemcpy(v18, v19, v20 & 3);
      strcat((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4], LibFileName);
      g_eng.log_fail_406440((const char *)D(0x8a9530), ((char *)D(0x8a2838)), LibFileName);
      g_eng.h5bf5db(0);
    }
    g_eng.log_fail_406440(g_eng.log, (const char *)(((char *)D(0x8a2838))), (const char *)(LibFileName));
  }
  return LibraryA;
}

/* 0x5b0c60: char *__thiscall sub_5B0C60(cls_5B0C60 *this, const char *a2, char *a3) */
int32_t wm_5b0c60(wl_char *self, char * a2, char * a3)
{
    intptr_t c = (intptr_t)self;

  strcpy(a3, (uint8_t *)c + 4);
  strcat(a3, ((char *)D(0x854f5c)));
  strcat(a3, a2);
  return a3;
}

/* 0x5b0ce0: char *__thiscall sub_5B0CE0(cls_5B0CE0 *this, const char *a2, char *a3) */
int32_t wm_5b0ce0(wl_char *self, char * a2, char * a3)
{
    intptr_t c = (intptr_t)self;

  strcpy(a3, (uint8_t *)c + 259);
  strcat(a3, ((char *)D(0x854f5c)));
  strcat(a3, a2);
  return a3;
}

/* 0x5b0d70: int __thiscall sub_5B0D70(cls_5B0D70 *this, HINSTANCE hInstance, const char *a3) */
int32_t wm_5b0d70(wl_char *self, int32_t hInstance, char * a3)
{
    intptr_t c = (intptr_t)self;

  F32(c, 4 * 65) = hInstance;
  strcpy((uint8_t *)c + 4, a3);
  API_DialogBoxParamA((intptr_t)(hInstance), (intptr_t)((char *)0x2711), (intptr_t)(0), (intptr_t)(((intptr_t)D(0x4013c0) /* lo_4013c0 by address */)), (intptr_t)(0));
  return F32(c, 0x0);
}

/* 0x5b0dd0: char *__thiscall sub_5B0DD0(cls_5B0DD0 *this, char *a2, int a3) */
int32_t wm_5b0dd0(wl_char *self, char * a2, int32_t a3)
{
    intptr_t c = (intptr_t)self;

  strcpy(a2, (uint8_t *)c + 132);
  strcat(a2, ((char *)D(0x8a43dc)));
  strcat(a2, (uint8_t *)c + 212);
  if ( a3 != 0 )
  {
    strcat(a2, D(0x8a43d8));
    strcat(a2, ((char *)D(0x8a43c8)));
    strcat(a2, D(0x8a43c4));
  }
  return a2;
}

/* 0x5b0ee0: HWND __thiscall sub_5B0EE0(cls_5B0EE0 *this, HINSTANCE hInstance) */
int32_t wm_5b0ee0(wl_char *self, int32_t hInstance)
{
    intptr_t c = (intptr_t)self;

    _Alignas(8) uint8_t frame_[0x100 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *WindowName = (char *)(frame_ + 0x0);   /* [256] at ebp-0x100 */

  strcpy(WindowName, (uint8_t *)c + 132);
  strcat(WindowName, ((char *)D(0x8a43dc)));
  strcat(WindowName, (uint8_t *)c + 212);
  strcat(WindowName, D(0x8a43d8));
  strcat(WindowName, ((char *)D(0x8a43c8)));
  strcat(WindowName, D(0x8a43c4));
  return API_CreateWindowExA((intptr_t)(0), (intptr_t)(((char *)D(0x8a4360))), (intptr_t)(WindowName), (intptr_t)(0x80000000), (intptr_t)(0), (intptr_t)(0), (intptr_t)(100), (intptr_t)(100), (intptr_t)(0), (intptr_t)(0), (intptr_t)(hInstance), (intptr_t)(0));
}

/* 0x5b1030: int sub_5B1030() */
int32_t wm_5b1030(void)
{

    intptr_t v0 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0x1c + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    MSG *const Msg__f = (MSG *)(frame_ + 0x0);   /* ebp-0x1c */

  while ( 1 )
  {
    while ( API_PeekMessageA((intptr_t)(&(*Msg__f)), (intptr_t)(0), (intptr_t)(0), (intptr_t)(0), (intptr_t)(1u)) )
    {
      if ( (*Msg__f).message == 18 )
        g_eng.h5bf5db(0);
      API_TranslateMessage((intptr_t)(&(*Msg__f)));
      API_DispatchMessageA((intptr_t)(&(*Msg__f)));
    }
    if ( (*(intptr_t *)D(0x8a952c)) != 0 )
    {
      v0 = (*(int *)D(0x8a9b64));
      if ( F32((char *)(*(int *)D(0x8a9b64)), 28) == 0 )
        break;
    }
    API_WaitMessage();
  }
  result = F32((char *)(*(int *)D(0x8a9b64)), 12);
  if ( result != 0 )
  {
    if ( F32((char *)(*(int *)D(0x8a9b64)), 16) == 0 || F32(v0 + (1) * 0x10, 0x0) - API_GetTickCount() < 5 )
      F32(v0 + (1) * 0x10, 0xc) = 1;
    result = API_GetTickCount();
    F32(v0 + (1) * 0x10, 0x0) = result;
  }
  return result;
}

/* 0x5b10d0: void __cdecl sub_5B10D0() */
void wm_5b10d0(void)
{

    intptr_t v0 = 0;
    void * v1 = 0;
    intptr_t v2 = 0;
    uint16_t v3 = 0;
    _Alignas(8) uint8_t frame_[0x100 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Caption = (char *)(frame_ + 0x0);   /* [256] at ebp-0x100 */

  v0 = (*(int *)D(0x8a9b64));
  if ( (*(int *)D(0x8a9b64)) != 0 )
  {
    W_wn_5624c0((intptr_t)((*(int *)D(0x8a9b64))));
    g_eng.op_delete((void *)v0);
    (*(int *)D(0x8a9b64)) = 0;
  }
  if ( F32(D(0x8a9b68), 0) != 0 )
  {
    v1 = (int32_t *)(intptr_t)F32(D(0x8a9b68), 0);   /* the app object (retail: mov eax, dword_8A9B68) */
    if ( SYS32(788) != 0 )
    {
      API_FreeLibrary((intptr_t)(*(intptr_t *)&SYSB(788)));
      SYS32(788) = 0;
    }
    if ( SYS32(792) != 0 )
    {
      API_FreeLibrary((intptr_t)(*(intptr_t *)&SYSB(792)));
      SYS32(792) = 0;
    }
    if ( SYS32(776) != 0 )
    {
      API_FreeLibrary((intptr_t)(*(intptr_t *)&SYSB(776)));
      SYS32(776) = 0;
    }
    if ( SYS32(780) != 0 )
    {
      API_FreeLibrary((intptr_t)(*(intptr_t *)&SYSB(780)));
      SYS32(780) = 0;
    }
    if ( SYS32(784) != 0 )
    {
      API_FreeLibrary((intptr_t)(*(intptr_t *)&SYSB(784)));
      SYS32(784) = 0;
    }
    if ( SYS32(800) != 0 )
    {
      W_tm_5658c0((intptr_t)(SYS32(800)));
      v2 = SYS32(800);
      if ( SYS32(800) != 0 )
      {
        if ( CFG() != 0 )
        {
          v3 = 0;
          if ( F16(SYS32(800), 4) != 0 )
          {
            do
            {
              if ( F32(F32(v2, 0) + 28 * v3, 16) != 0 )
                g_eng.op_delete((void *)F32(F32(v2, 0), 28 * v3 + 16));
              ++v3;
            }
            while ( v3 < F16(v2, 4) );
          }
          g_eng.op_delete((void *)F32(v2, 0));
          F32(v2, 0) = 0;
        }
        g_eng.op_delete((void *)(void *)v2);
        SYS32(800) = 0;
      }
    }
    API_DeleteObject((intptr_t)(*((intptr_t *)v1 + 57)));
    API_DestroyWindow((intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec))));
    API_UnregisterClassA((intptr_t)(((char *)D(0x8a4360))), (intptr_t)((*(intptr_t *)D(0x8a6b38))));
    wm_5b5a90(D(0x8a6b50));
    strcpy(Caption, (uint8_t *)v1 + 132);
    strcat(Caption, ((char *)D(0x8a43dc)));
    strcat(Caption, (uint8_t *)v1 + 212);
    if ( F32(((uint8_t *)D(0x8a9530) + 0x000), 0) != 0 )
      API_MessageBoxA((intptr_t)(0), (intptr_t)((char *)&((uint8_t *)D(0x8a9530) + 0x000)[4]), (intptr_t)(Caption), (intptr_t)(0x40u));
    API_CoUninitialize();
    g_eng.op_delete((void *)v1);
    F32(D(0x8a9b68), 0) = 0;
  }
  API_PostQuitMessage((intptr_t)(0));
}

/* 0x5b12e0: LRESULT __stdcall sub_5B12E0(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) */
int32_t wm_5b12e0(int32_t hWnd, uint32_t Msg, uint32_t wParam, int32_t lParam)
{

    intptr_t v5 = 0;
    intptr_t v6 = 0;
    intptr_t v7 = 0;
    intptr_t * v8 = 0;
    intptr_t i = 0;

  if ( Msg <= 0x10 )
  {
    if ( Msg != 16 )
    {
      switch ( Msg )
      {
        case 2u:
        case 3u:
        case 5u:
          return 0;
        case 0xFu:
          if ( (*(int *)D(0x8a9b64)) == 0
            || SYS32(10348) == 0
            || W_zd_5b9130((intptr_t)(*(intptr_t *)&SYSB(10348)), (intptr_t)((intptr_t)(*(intptr_t *)D(0x8a93ec)))) == 0 )
          {
            return API_DefWindowProcA((intptr_t)(hWnd), (intptr_t)(Msg), (intptr_t)(wParam), (intptr_t)(lParam));
          }
          return 0;
        default:
          return API_DefWindowProcA((intptr_t)(hWnd), (intptr_t)(Msg), (intptr_t)(wParam), (intptr_t)(lParam));
      }
    }
    g_eng.h5bf5db(0);
  }
  if ( Msg != 28 )
  {
    if ( Msg != 256 )
    {
      if ( Msg != 275 )
        return API_DefWindowProcA((intptr_t)(hWnd), (intptr_t)(Msg), (intptr_t)(wParam), (intptr_t)(lParam));
      if ( (*(int *)D(0x8a9b64)) != 0 )
      {
        F32((char *)(*(int *)D(0x8a9b64)), 28) = 0;
        return 0;
      }
    }
    return 0;
  }
  (*(intptr_t *)D(0x8a952c)) = wParam;
  if ( (*(int *)D(0x8a9b64)) == 0 )
    return 0;
  if ( wParam == 0 )
  {
    if ( F32((char *)(*(int *)D(0x8a9b64)), 8) != 0 )
      W_wn_58ffa0((intptr_t)(((intptr_t)g_eng.sys)), (intptr_t)(1));
    return 0;
  }
  v5 = (*(int *)D(0x8a9b64));
  v6 = true;
  if ( F32((char *)(*(int *)D(0x8a9b64)), 8) == 0 )
    return 0;
  if ( F32((char *)(*(int *)D(0x8a9b64)), 0) != 0 )
  {
    v7 = *(_DWORD *)F32((char *)(*(int *)D(0x8a9b64)), 0);
    if ( v7 != 0 )
      v6 = F32(v7, 24) == 0;
  }
  if ( SYS32(10360) != 0 )
  {
    if ( F32(D(0x8a6b54), 0) == 0 )
      g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a2a74)), 0, D(0x8a9b6c));
    W_id_5ba290((intptr_t)(*(intptr_t *)&SYSB(10360)));
    if ( F32(D(0x8a6b54), 0) == 0 )
      g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a2a5c)), 0, D(0x8a9b6c));
  }
  v8 = (intptr_t *)&SYSB(10364);
  for ( i = 2; i != 0; --i )
  {
    if ( *v8 != 0 )
      W_id_5ba290((intptr_t)(*v8));
    ++v8;
  }
  if ( F32(D(0x8a6b54), 0) == 0 )
    g_eng.h408b90(D(0x8a6620), ((char *)D(0x8a2a44)), 0, D(0x8a9b6c));
  if ( SYS32(0x2884) != 0 )
    W_sd_5bbd20((intptr_t)(SYS32(0x2884)), (intptr_t)(!v6));
  wm_5b14c0(v5);
  return 0;
}

/* 0x5b14c0: void __thiscall sub_5B14C0(cls_5B14C0 *this) */
void wm_5b14c0(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    uint8_t * v1 = 0;
    intptr_t i = 0;
    intptr_t v3 = 0;
    intptr_t j = 0;
    intptr_t v5 = 0;
    uint16_t v6 = 0;
    intptr_t v7 = 0;
    void * v8 = 0;

  if ( F32(c, 0x8) != 0 && W_zd_5b91d0((intptr_t)(SYS32(10348))) )
  {
    W_zd_5b9240((intptr_t)(SYS32(10348)));
    v1 = &SYSB(2732);
    for ( i = 909; i != 0; --i )
    {
      v3 = F32(v1, 0);
      for ( j = 0; (uint16_t)j < F16(v3, 4); ++j )
      {
        if ( F32((F32(v3, 0) + 8 * (uint16_t)j), 0) != 0 )
          W_fl_4184a0((intptr_t)(v3), (intptr_t)(j));
      }
      v1 += 4;
    }
    if ( SYS32(860) != 0 && W_wn_5b9ac0((intptr_t)(SYS32(860))) )
      W_wn_5b9ab0((intptr_t)(SYS32(860)));
    if ( SYS32(864) != 0 && W_wn_5b9ac0((intptr_t)(SYS32(864))) )
      W_wn_5b9ab0((intptr_t)(SYS32(864)));
    v5 = SYS32(10068);
    if ( SYS32(10068) != 0 )
    {
      v6 = 0;
      if ( F16(SYS32(10068), 4) != 0 )
      {
        do
        {
          v7 = 28 * v6;
          v8 = F32(F32(v5, 0), v7 + 24);
          if ( v8 != 0 )
          {
            W_rp_5b9390((intptr_t)(F32(F32(v5, 0), v7 + 24)));
            g_eng.op_delete((void *)v8);
            F32(F32(v5, 0) + v7, 24) = 0;
          }
          ++v6;
          F32((F32(v5, 0) + v7), 0) = 0;
          F32(F32(v5, 0) + v7, 4) = 0;
          F32(F32(v5, 0) + v7, 8) = 0;
          F32(F32(v5, 0) + v7, 12) = 0;
          F32(F32(v5, 0) + v7, 16) = 0;
          F32(F32(v5, 0) + v7, 20) = 0;
        }
        while ( v6 < F16(v5, 4) );
      }
      F16(v5, 6) = 0;
    }
  }
}

/* 0x5b5a60: BOOL __thiscall sub_5B5A60(cls_5B5A60 *this, LPCSTR lpName) */
int32_t wm_5b5a60(wl_char *self, char * lpName)
{
    intptr_t c = (intptr_t)self;

  F32(c, 0x0) = (int)API_CreateMutexA((intptr_t)(0), (intptr_t)(true), (intptr_t)(lpName));
  return API_GetLastError() != 183;
}

/* 0x5b5a90: HANDLE __thiscall sub_5B5A90(cls_5B5A90 *this) */
int32_t wm_5b5a90(wl_char *self)
{
    intptr_t c = (intptr_t)self;
    intptr_t result = 0;

  result = (intptr_t)F32(c, 0x0);
  if ( F32(c, 0x0) != 0 )
  {
    API_ReleaseMutex((intptr_t)((intptr_t)F32(c, 0x0)));
    result = (intptr_t)API_CloseHandle((intptr_t)((intptr_t)F32(c, 0x0)));
    F32(c, 0x0) = 0;
  }
  return result;
}

/* 0x5b5ac0: DWORD __stdcall zdd_busy_wait_ms_5B5AC0(DWORD a1) */
int32_t wm_5b5ac0(uint32_t a1)
{

    uint32_t TickCount = 0;
    uint32_t result = 0;

  TickCount = API_GetTickCount();
  for ( result = API_GetTickCount() - TickCount; result < a1; result = API_GetTickCount() - TickCount )
    ;
  return result;
}

/* 0x5b5af0: int __stdcall sub_5B5AF0(int a1) */
int32_t wm_5b5af0(int32_t a1)
{

    intptr_t FileVersionInfoSizeA = 0;
    void * v2 = 0;
    uint16_t v3 = 0;
    uint16_t v4 = 0;
    intptr_t v5 = 0;
    char v6 = 0;
    uint16_t i = 0;
    intptr_t v8 = 0;
    _Alignas(8) uint8_t frame_[0x414 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    char *Filename = (char *)(frame_ + 0x14);   /* [1024] at ebp-0x400 */
    intptr_t *const lpBuffer__f = (intptr_t *)(frame_ + 0x0);   /* ebp-0x414 */
    uint32_t *const puLen__f = (uint32_t *)(frame_ + 0x4);   /* ebp-0x410 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x40c */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x408 */
    void * *const v14__f = (void * *)(frame_ + 0x10);   /* ebp-0x404 */

  (*v12__f) = 0;
  (*v13__f) = 0;
  (*puLen__f) = 0;
  (*lpBuffer__f) = 0;
  API_GetModuleFileNameA((intptr_t)(0), (intptr_t)(Filename), (intptr_t)(0x400u));
  FileVersionInfoSizeA = API_GetFileVersionInfoSizeA((intptr_t)(Filename), (intptr_t)(0));
  v2 = (intptr_t)g_eng.op_new(FileVersionInfoSizeA);
  (*v14__f) = v2;
  if ( API_GetFileVersionInfoA((intptr_t)(Filename), (intptr_t)(0), (intptr_t)(FileVersionInfoSizeA), (intptr_t)(v2))
    && API_VerQueryValueA((intptr_t)(v2), (intptr_t)(((char *)D(0x8a4778))), (intptr_t)(&(*lpBuffer__f)), (intptr_t)(&(*puLen__f))) )
  {
    strcpy((char *)(a1 + 128), (const char *)(*lpBuffer__f));
    if ( API_VerQueryValueA((intptr_t)(v2), (intptr_t)(((char *)D(0x8a4750))), (intptr_t)(&(*lpBuffer__f)), (intptr_t)(&(*puLen__f))) )
    {
      strcpy((char *)(a1 + 192), (const char *)(*lpBuffer__f));
      if ( API_VerQueryValueA((intptr_t)(v2), (intptr_t)(((char *)D(0x8a4728))), (intptr_t)(&(*lpBuffer__f)), (intptr_t)(&(*puLen__f))) )
      {
        strcpy((char *)a1, (const char *)(*lpBuffer__f));
        if ( API_VerQueryValueA((intptr_t)(v2), (intptr_t)(((char *)D(0x8a4700))), (intptr_t)(&(*lpBuffer__f)), (intptr_t)(&(*puLen__f))) )
        {
          v3 = 0;
          v4 = 0;
          strcpy((char *)(a1 + 64), (const char *)(*lpBuffer__f));
          if ( strlen((const char *)(a1 + 192)) != 0 )
          {
            v5 = 0;
            do
            {
              v6 = F8(v5 + a1, 192);
              if ( v6 == 44 )
              {
                if ( (*v12__f) == 0 )
                {
                  (*v12__f) = 1;
                  F8(v4 + a1, 208) = 46;
LABEL_13:
                  ++v4;
                }
              }
              else if ( v6 != 32 )
              {
                F8(v4 + a1, 208) = v6;
                goto LABEL_13;
              }
              v5 = ++v3;
            }
            while ( v3 < strlen((const char *)(a1 + 192)) );
          }
          F8(v4 + a1, 208) = 0;
          for ( i = strlen((const char *)(a1 + 208)) - 1; ; --i )
          {
            v8 = a1 + i;
            if ( F8(v8, 208) != 48 || F8(v8, 207) == 46 )
              break;
            F8(v8, 208) = 0;
          }
          v2 = (*v14__f);
          (*v13__f) = 1;
        }
      }
    }
  }
  g_eng.op_delete((void *)v2);
  return (*v13__f);
}

/* 0x5b5d50: HBRUSH __stdcall sub_5B5D50(COLORREF a1) */
int32_t wm_5b5d50(uint32_t a1)
{

    int32_t * v1 = 0;
    intptr_t result = 0;
    _Alignas(8) uint8_t frame_[0xc + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    LOGBRUSH *const plbrush__f = (LOGBRUSH *)(frame_ + 0x0);   /* ebp-0xc */

  (*plbrush__f).lbStyle = 0;
  (*plbrush__f).lbColor = a1;
  v1 = (intptr_t)g_eng.op_new(4u);
  result = API_CreateBrushIndirect((intptr_t)(&(*plbrush__f)));
  *v1 = result;
  return result;
}

/* 0x5b5d90: int __stdcall sub_5B5D90(int a1, int a2) */
int32_t wm_5b5d90(int32_t a1, int32_t a2)
{


  F8(a1, 2) = BYTE2(a2);
  F16(a1, 0) = a2;
  F8(a1, 3) = 0;
  return a1;
}

/* 0x5b5dc0: HFONT __stdcall sub_5B5DC0(LONG a1, LONG a2, int a3) */
int32_t wm_5b5dc0(int32_t a1, int32_t a2, int32_t a3)
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

/* 0x5b5ef0: BOOL __stdcall sub_5B5EF0(HDC hdc, LONG a2, LONG a3, int a4, int a5) */
int32_t wm_5b5ef0(int32_t hdc, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{


    _Alignas(8) uint8_t frame_[0x28 + 0x10];   /* the stack locals at their ebp offsets */
    memset(frame_, 0, sizeof frame_);
    POINT *const apt__f = (POINT *)(frame_ + 0x0);   /* ebp-0x28 */
    intptr_t *const v7__f = (intptr_t *)(frame_ + 0x8);   /* ebp-0x20 */
    intptr_t *const v8__f = (intptr_t *)(frame_ + 0xc);   /* ebp-0x1c */
    intptr_t *const v9__f = (intptr_t *)(frame_ + 0x10);   /* ebp-0x18 */
    intptr_t *const v10__f = (intptr_t *)(frame_ + 0x14);   /* ebp-0x14 */
    intptr_t *const v11__f = (intptr_t *)(frame_ + 0x18);   /* ebp-0x10 */
    intptr_t *const v12__f = (intptr_t *)(frame_ + 0x1c);   /* ebp-0xc */
    intptr_t *const v13__f = (intptr_t *)(frame_ + 0x20);   /* ebp-0x8 */
    intptr_t *const v14__f = (intptr_t *)(frame_ + 0x24);   /* ebp-0x4 */

  (*v7__f) = a2 + a4;
  (*v9__f) = a2 + a4;
  (*apt__f).x = a2;
  (*v11__f) = a2;
  (*v13__f) = a2;
  (*apt__f).y = a3;
  (*v8__f) = a3;
  (*v14__f) = a3;
  (*v10__f) = a3 + a5;
  (*v12__f) = a3 + a5;
  return API_Polygon((intptr_t)(hdc), (intptr_t)(&(*apt__f)), (intptr_t)(5));
}

/* 0x5b5f50: int __stdcall sub_5B5F50(unsigned int a1, int a2, int a3, int a4) */
int32_t wm_5b5f50(uint32_t a1, int32_t a2, int32_t a3, int32_t a4)
{

    intptr_t v4 = 0;
    intptr_t v5 = 0;
    intptr_t i = 0;
    intptr_t v7 = 0;

  v4 = 0;
  v5 = 0;
  for ( i = 3; i != 0; --i )
  {
    v7 = ((uint8_t)(a1 >> v5) + a3 * ((uint8_t)(a2 >> v5) - (uint8_t)(a1 >> v5)) / a4) << v5;
    v5 += 8;
    v4 += v7;
  }
  return v4;
}
/* unported callees: none */
