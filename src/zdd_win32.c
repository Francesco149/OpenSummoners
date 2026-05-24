/*
 * src/zdd_win32.c — Win32 primitives backing the ZDD port.
 *
 * Supplies the primitive set declared at the bottom of zdd.h for the
 * real (mingw) build.  The host test harness provides its own
 * recording stubs in tests/test_zdd.c and does NOT link this file.
 *
 * Real-build primitive map:
 *   zdd_show_cursor             ShowCursor(BOOL)
 *   zdd_output_debug_string     OutputDebugStringA
 *   zdd_com_release             (*IUnknown)->Release via vtable[2]
 *   zdd_obj_destroy             free() — placeholder until ZDDObject
 *                                 cleanup (FUN_005b9390) is ported
 *   zdd_directdraw_create_ex    DirectDrawCreateEx + DDERR log
 *   zdd_set_coop_level          IDirectDraw7::SetCooperativeLevel
 *
 * The ZDDObject release path in retail (FUN_005b9390 + FUN_005bef0e)
 * walks an unported cleanup chain — for now we just heap-free the
 * child pointer.  Wire to the full cleanup chain when ZDDObject lands.
 */
#include "zdd.h"

#include <windows.h>
#include <ddraw.h>

void zdd_show_cursor(int show)
{
    ShowCursor(show ? TRUE : FALSE);
}

void zdd_output_debug_string(const char *s)
{
    if (s == NULL) return;
    OutputDebugStringA(s);
}

/* Standard IUnknown::Release call: vtable index 2 = byte offset 0x08
 * on a 32-bit x86 COM vtable.  Pointer-of-pointer to support the
 * "release-and-clear" idiom used everywhere in the retail dtor. */
void zdd_com_release(void **iunknown_pp)
{
    if (iunknown_pp == NULL || *iunknown_pp == NULL) return;
    IUnknown *iu = (IUnknown *)*iunknown_pp;
    iu->lpVtbl->Release(iu);
    *iunknown_pp = NULL;
}

/* Placeholder ZDDObject destroyer — see header comment above.  Once
 * FUN_005b9390 (the per-ZDDObject cleanup chain) is ported, dispatch
 * through it before the heap free. */
void zdd_obj_destroy(zdd_object **obj_pp)
{
    if (obj_pp == NULL || *obj_pp == NULL) return;
    free(*obj_pp);
    *obj_pp = NULL;
}

/* FUN_005b88c0 — DirectDrawCreateEx with the engine's exact args.
 * IID_IDirectDraw7 comes from the system header (DAT_00850eb0 is the
 * matching in-binary copy at retail, but the mingw header has the
 * same 16-byte literal so we use the macro). */
int zdd_directdraw_create_ex(zdd *self)
{
    HRESULT hr = DirectDrawCreateEx(NULL,
                                    (LPVOID *)&self->ddraw7,
                                    &IID_IDirectDraw7,
                                    NULL);
    if (FAILED(hr)) {
        /* Retail passes DAT_008a9b6c (empty BSS string) as prefix1
         * for this call site.  Mirror exactly. */
        zdd_log_dderr(self, "", "DirectDrawCreate", (int32_t)hr);
        return 0;
    }
    return 1;
}

/* FUN_005b89d0 — IDirectDraw7::SetCooperativeLevel.  Flags:
 *   fullscreen == 0  →  DDSCL_NORMAL (0x08)
 *   fullscreen != 0  →  DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN |
 *                       DDSCL_ALLOWREBOOT  (0x01 | 0x02 | 0x10 = 0x13)
 *
 * Vtable byte offset 0x50 = method index 20 — verified against the
 * IDirectDraw7 cheat-sheet in docs/findings/ddraw-init.md. */
int zdd_set_coop_level(zdd *self, void *hwnd, int fullscreen)
{
    IDirectDraw7 *dd = (IDirectDraw7 *)self->ddraw7;
    DWORD flags = fullscreen
        ? (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT)
        :  DDSCL_NORMAL;
    HRESULT hr = dd->lpVtbl->SetCooperativeLevel(dd, (HWND)hwnd, flags);
    if (FAILED(hr)) {
        zdd_log_dderr(self, "DirectDraw", "SetCooperativeLevel",
                      (int32_t)hr);
        return 0;
    }
    return 1;
}
