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
 *   zdd_object_local_free       LocalFree(HLOCAL)
 *   zdd_directdraw_create_ex    DirectDrawCreateEx + DDERR log
 *   zdd_set_coop_level          IDirectDraw7::SetCooperativeLevel
 *
 * Note: zdd_obj_destroy itself is pure logic (FUN_005b9390 cleanup +
 * heap free) and lives in zdd.c — it only needs zdd_object_local_free
 * + zdd_com_release primitives from this file.
 */
#include "zdd.h"

#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
#include <stdlib.h>

void zdd_show_cursor(int show)
{
    ShowCursor(show ? TRUE : FALSE);
}

/* Dual-sink: OutputDebugStringA (so a real DbgView session still sees
 * everything) AND stderr (so the harness picks it up without a DbgView
 * attach).  The retail engine routes every DDERR through this primitive,
 * so this is the only way to see DDraw failure detail under the headless
 * launcher.  Cost is one extra fprintf per log call — negligible. */
void zdd_output_debug_string(const char *s)
{
    if (s == NULL) return;
    OutputDebugStringA(s);
    /* Engine often passes messages without a trailing newline; add one
     * so harness output is line-oriented.  Strings that already end in
     * '\n' just get a blank line — acceptable for diagnostic plumbing. */
    fprintf(stderr, "[ddraw-log] %s\n", s);
    fflush(stderr);
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

/* FUN_005b93e0's LocalFree primitive — wraps the Win32 call.  NULL
 * is a no-op (matches Win32 LocalFree's contract). */
void zdd_object_local_free(void *local_alloc)
{
    if (local_alloc == NULL) return;
    LocalFree((HLOCAL)local_alloc);
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

/* FUN_005b8c00 — full surface create.  Calls into the pure-logic
 * descriptor builder, translates to a real DDSURFACEDESC2, invokes
 * IDirectDraw7::CreateSurface, optionally binds the palette stashed
 * in self->com_b, and logs DDERR on failure.
 *
 * Method-index notes:
 *   IDirectDraw7::CreateSurface     vtable index  6 / byte offset 0x18
 *   IDirectDrawSurface7::SetPalette vtable index 31 / byte offset 0x7c
 * See docs/findings/ddraw-init.md "Vtable cheat-sheet for the harness". */
int zdd_create_surface(zdd *self, void **out_surface,
                       uint32_t width, uint32_t height,
                       uint32_t caps_base, int force_videomem)
{
    zdd_surface_desc_build b;
    zdd_build_surface_desc(self, &b, width, height, caps_base,
                           force_videomem);

    /* Build the real DDSURFACEDESC2.  Zero-init then overlay from the
     * pure-logic descriptor. */
    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize           = sizeof(ddsd);     /* 0x7c — matches Z_DDSD */
    ddsd.dwFlags          = b.dwFlags;
    ddsd.dwHeight         = b.dwHeight;
    ddsd.dwWidth          = b.dwWidth;
    ddsd.ddsCaps.dwCaps   = b.dwCaps;
    if (b.has_pixel_format) {
        ddsd.ddpfPixelFormat.dwSize        = b.ddpf_dwSize;
        ddsd.ddpfPixelFormat.dwFlags       = b.ddpf_dwFlags;
        ddsd.ddpfPixelFormat.dwRGBBitCount = b.ddpf_dwRGBBitCount;
        ddsd.ddpfPixelFormat.dwRBitMask    = b.ddpf_dwRBitMask;
        ddsd.ddpfPixelFormat.dwGBitMask    = b.ddpf_dwGBitMask;
        ddsd.ddpfPixelFormat.dwBBitMask    = b.ddpf_dwBBitMask;
    }

    IDirectDraw7 *dd = (IDirectDraw7 *)self->ddraw7;
    LPDIRECTDRAWSURFACE7 surf = NULL;
    HRESULT hr = dd->lpVtbl->CreateSurface(dd, &ddsd, &surf, NULL);

    /* Retail's success path: if self has a palette stashed (com_b),
     * bind it to the new surface BEFORE returning success.  If
     * CreateSurface failed, fall through to the DDERR log path. */
    if (self->com_b != NULL) {
        if (hr == 0) {
            IDirectDrawPalette *pal = (IDirectDrawPalette *)self->com_b;
            surf->lpVtbl->SetPalette(surf, pal);
            *out_surface = surf;
            return 1;
        }
        /* Failure path falls through. */
    } else if (hr == 0) {
        *out_surface = surf;
        return 1;
    }

    /* CreateSurface failed — log + return 0. */
    zdd_log_dderr(self, "DirectDraw", "CreateSurface", (int32_t)hr);
    return 0;
}

/* FUN_005b8480 (Win32 leg, primary CreateSurface) — build a DDSURFACEDESC2
 * for the primary surface from `desc` (dwFlags + ddsCaps.dwCaps +
 * dwBackBufferCount), invoke IDirectDraw7::CreateSurface vtable[6] /
 * byte 0x18, and stash the result in self->com_a.  The per-mode shape
 * is decided by zdd_build_primary_surface_desc (pure logic in zdd.c)
 * — this leg just translates the build struct to a real DDSURFACEDESC2
 * and makes the call. */
int zdd_create_primary_surface(zdd *self,
                               const zdd_primary_desc_build *desc)
{
    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize             = sizeof(ddsd);       /* 0x7c */
    ddsd.dwFlags            = desc->dwFlags;
    ddsd.ddsCaps.dwCaps     = desc->dwCaps;
    ddsd.dwBackBufferCount  = desc->dwBackBufferCount;

    IDirectDraw7 *dd = (IDirectDraw7 *)self->ddraw7;
    LPDIRECTDRAWSURFACE7 surf = NULL;
    HRESULT hr = dd->lpVtbl->CreateSurface(dd, &ddsd, &surf, NULL);
    if (FAILED(hr)) {
        zdd_log_dderr(self, "DirectDraw", "CreateSurface", (int32_t)hr);
        return 0;
    }
    self->com_a = surf;
    return 1;
}

/* FUN_005b8e00 (Win32 leg, palette half) — sample the desktop's
 * current system palette via GetSystemPaletteEntries and wrap it as
 * an IDirectDrawPalette with DDPCAPS_8BIT.  Stashes the new palette
 * into self->com_b.
 *
 * Retail passes NULL as the HDC (matches the GDI contract: NULL means
 * the *system* palette regardless of any DC).  Argument shape exactly
 * matches retail's `GetSystemPaletteEntries(NULL, 0, 0x100, &local_400)`. */
int zdd_create_system_palette(zdd *self)
{
    PALETTEENTRY entries[256];
    GetSystemPaletteEntries(NULL, 0, 256, entries);

    IDirectDraw7 *dd = (IDirectDraw7 *)self->ddraw7;
    LPDIRECTDRAWPALETTE pal = NULL;
    HRESULT hr = dd->lpVtbl->CreatePalette(dd, DDPCAPS_8BIT, entries,
                                           &pal, NULL);
    if (FAILED(hr)) {
        zdd_log_dderr(self, "DirectDraw", "CreatePalette", (int32_t)hr);
        return 0;
    }
    self->com_b = pal;
    return 1;
}

/* FUN_005b8e00 (Win32 leg, bind half) — IDirectDrawSurface7::SetPalette
 * via vtable[31] / byte 0x7c.  log_owner is the parent ZDD whose
 * log_buf the DDERR builder scribbles to (the surface itself has no
 * log buffer of its own). */
int zdd_surface_set_palette(void *surface, void *palette, zdd *log_owner)
{
    if (surface == NULL || palette == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    LPDIRECTDRAWPALETTE  pal  = (LPDIRECTDRAWPALETTE)palette;
    HRESULT hr = surf->lpVtbl->SetPalette(surf, pal);
    if (FAILED(hr)) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface",
                          "SetPalette", (int32_t)hr);
        }
        return 0;
    }
    return 1;
}

/* FUN_005b9740 (Win32 leg) — IDirectDrawSurface7::GetAttachedSurface
 * via vtable[12] / byte offset 0x30.  Builds a DDSCAPS2 with caps[0]
 * = `caps_in` and zero-padded caps[1..3] (retail's
 * `local_10..local_4` zero block).  Caller chooses the caps mask
 * (DDSCAPS_BACKBUFFER alone, or |DDSCAPS_VIDEOMEMORY when forcing
 * vram). */
int zdd_get_attached_surface(void *primary, uint32_t caps_in,
                             void **out, zdd *log_owner)
{
    if (primary == NULL || out == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)primary;
    DDSCAPS2 caps;
    memset(&caps, 0, sizeof(caps));
    caps.dwCaps = (DWORD)caps_in;
    LPDIRECTDRAWSURFACE7 attached = NULL;
    HRESULT hr = surf->lpVtbl->GetAttachedSurface(surf, &caps, &attached);
    if (FAILED(hr)) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface",
                          "GetAttachedSurface", (int32_t)hr);
        }
        return 0;
    }
    *out = attached;
    return 1;
}

/* IDirectDraw7::CreateClipper via vtable[4] (byte 0x10).  Stores the
 * new IDirectDrawClipper into *out_clipper.  Defensive NULL-on-failure
 * (retail leaves the slot undefined when CreateClipper fails). */
void zdd_create_clipper(zdd *parent, void **out_clipper)
{
    if (out_clipper == NULL) return;
    *out_clipper = NULL;
    if (parent == NULL || parent->ddraw7 == NULL) return;
    IDirectDraw7 *dd = (IDirectDraw7 *)parent->ddraw7;
    LPDIRECTDRAWCLIPPER clipper = NULL;
    HRESULT hr = dd->lpVtbl->CreateClipper(dd, 0, &clipper, NULL);
    if (FAILED(hr)) return;
    *out_clipper = clipper;
}

/* IDirectDrawClipper::SetClipList via vtable[7] (byte 0x1c — verified
 * by r2 disasm at 0x5b95a7).  Builds a one-rect RGNDATA bounding the
 * full surface and hands it to SetClipList.  See zdd.h docstring for
 * the open-issue history. */
void zdd_clipper_set_clip_list_rect(void *clipper,
                                    uint32_t width, uint32_t height)
{
    if (clipper == NULL) return;
    LPDIRECTDRAWCLIPPER cl = (LPDIRECTDRAWCLIPPER)clipper;

    /* RGNDATAHEADER (32B) + a single 16B RECT.  Layout matches the
     * stack-local retail builds at FUN_005b9520:0x5b9555..0x5b9579. */
    struct {
        RGNDATAHEADER hdr;
        RECT          rect;
    } rgn;
    rgn.hdr.dwSize     = sizeof(RGNDATAHEADER);   /* 0x20 */
    rgn.hdr.iType      = RDH_RECTANGLES;          /* 1 */
    rgn.hdr.nCount     = 1;
    rgn.hdr.nRgnSize   = sizeof(RECT);            /* 0x10 */
    rgn.hdr.rcBound.left   = 0;
    rgn.hdr.rcBound.top    = 0;
    rgn.hdr.rcBound.right  = (LONG)width;
    rgn.hdr.rcBound.bottom = (LONG)height;
    rgn.rect.left   = 0;
    rgn.rect.top    = 0;
    rgn.rect.right  = (LONG)width;
    rgn.rect.bottom = (LONG)height;

    cl->lpVtbl->SetClipList(cl, (LPRGNDATA)&rgn, 0);
}

/* IDirectDrawSurface7::SetClipper via vtable[28] (byte 0x70). */
void zdd_surface_set_clipper(void *surface, void *clipper)
{
    if (surface == NULL || clipper == NULL) return;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    LPDIRECTDRAWCLIPPER  cl   = (LPDIRECTDRAWCLIPPER)clipper;
    surf->lpVtbl->SetClipper(surf, cl);
}

/* FUN_005b9830 (Win32 leg) — IDirectDrawSurface7::SetColorKey via
 * vtable[29] (byte offset 0x74) with DDCKEY_SRCBLT (8) flag.  Builds
 * a DDCOLORKEY with both .dwColorSpaceLowValue and
 * .dwColorSpaceHighValue set to `key` — retail's `local_8 = key;
 * local_4 = key` setup.  NULL `surface` returns 0 silently — the
 * orchestrator's path can't actually call this with NULL (we only get
 * here after CreateSurface succeeded), but defensive.
 *
 * `log_owner` is the parent ZDD whose `log_buf` zdd_log_dderr scribbles
 * the DDERR message into.  We need a ZDD to log; the ZDDObject doesn't
 * have its own. */
int zdd_surface_set_color_key(void *surface, int32_t key, zdd *log_owner)
{
    if (surface == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    DDCOLORKEY ck;
    ck.dwColorSpaceLowValue  = (DWORD)key;
    ck.dwColorSpaceHighValue = (DWORD)key;
    HRESULT hr = surf->lpVtbl->SetColorKey(surf, DDCKEY_SRCBLT, &ck);
    if (FAILED(hr)) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface",
                          "SetColorKey", (int32_t)hr);
        }
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

/* FUN_005b8900 — IDirectDraw7::SetDisplayMode via vtable[21] / byte
 * offset 0x54.  Retail passes the 5-arg form: (w, h, bpp, refresh, 0).
 * The trailing 0 is dwFlags (DDSDM_*), always zero. */
int zdd_set_display_mode(zdd *self, uint32_t width, uint32_t height,
                         uint32_t bpp, uint32_t refresh_hz)
{
    IDirectDraw7 *dd = (IDirectDraw7 *)self->ddraw7;
    HRESULT hr = dd->lpVtbl->SetDisplayMode(dd, width, height, bpp,
                                            refresh_hz, 0);
    if (FAILED(hr)) {
        zdd_log_dderr(self, "DirectDraw", "SetDisplayMode",
                      (int32_t)hr);
        return 0;
    }
    return 1;
}

/* FUN_005b8950 — IDirectDraw7::GetDisplayMode via vtable[12] / byte
 * offset 0x30.  Builds a stack DDSURFACEDESC2 with the exact dwFlags
 * pattern retail uses (0x41006 = HEIGHT|WIDTH|PITCH|PIXELFORMAT) and
 * a pre-stamped ddpf header (dwSize=0x20, DDPF_RGB).  No DDERR log on
 * failure — retail's caller chooses the log message based on which of
 * SetDisplayMode / GetDisplayMode failed. */
int zdd_get_display_mode(zdd *self, uint32_t *out_width,
                         uint32_t *out_height, uint32_t *out_pitch)
{
    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize  = sizeof(ddsd);    /* 0x7c */
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;
    ddsd.ddpfPixelFormat.dwSize  = sizeof(ddsd.ddpfPixelFormat);  /* 0x20 */
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;

    IDirectDraw7 *dd = (IDirectDraw7 *)self->ddraw7;
    HRESULT hr = dd->lpVtbl->GetDisplayMode(dd, &ddsd);
    if (FAILED(hr)) return 0;

    if (out_width  != NULL) *out_width  = ddsd.dwWidth;
    if (out_height != NULL) *out_height = ddsd.dwHeight;
    if (out_pitch  != NULL) *out_pitch  = (uint32_t)ddsd.lPitch;
    return 1;
}

/* IDirectDrawSurface7::Blt with DDBLT_COLORFILL + DDBLT_WAIT.  Passes
 * NULL for lpDestRect (fill entire surface) and NULL for lpDDSrcSurface
 * (no source for color fill).  lpDDBltFx carries fill_value in
 * dwFillColor.  See ddraw-init.md vtable cheat-sheet: Blt is method
 * index 5, byte offset 0x14 — but the macro is the standard COM call. */
int zdd_surface_blt_color_fill(void *surface, uint32_t fill_value,
                               zdd *log_owner)
{
    if (surface == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize      = sizeof(fx);
    fx.dwFillColor = (DWORD)fill_value;
    HRESULT hr = surf->lpVtbl->Blt(surf, NULL, NULL, NULL,
                                   DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    if (FAILED(hr)) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface", "Blt(COLORFILL)",
                          (int32_t)hr);
        }
        return 0;
    }
    return 1;
}

/* IDirectDrawSurface7::GetDC via vtable[17] (byte 0x44).  Verified by
 * r2 disasm of FUN_005b94e0 at 0x5b94e0: `call dword [ecx + 0x44]`. */
int zdd_surface_get_dc(void *surface, void **out_hdc)
{
    if (surface == NULL || out_hdc == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    HDC hdc = NULL;
    HRESULT hr = surf->lpVtbl->GetDC(surf, &hdc);
    if (FAILED(hr)) {
        *out_hdc = NULL;
        return 0;
    }
    *out_hdc = (void *)hdc;
    return 1;
}

/* IDirectDrawSurface7::ReleaseDC via vtable[26] (byte 0x68).  Verified
 * by r2 disasm of FUN_005b9500: `call dword [eax + 0x68]`. */
void zdd_surface_release_dc(void *surface, void *hdc)
{
    if (surface == NULL || hdc == NULL) return;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    surf->lpVtbl->ReleaseDC(surf, (HDC)hdc);
}

/* FUN_005b5ac0 — busy-wait spin via GetTickCount.  Polls until the
 * elapsed unsigned-wraparound difference reaches `ms`.  Matches
 * retail's `do { tick = GetTickCount(); } while (tick - start < ms)`
 * shape exactly (unsigned wrap handles 49-day rollover). */
void zdd_busy_wait_ms(uint32_t ms)
{
    DWORD start = GetTickCount();
    while ((DWORD)(GetTickCount() - start) < (DWORD)ms) {
        /* spin */
    }
}

/* IDirectDrawSurface7::Flip via vtable[11] (byte 0x2c).  Verified by
 * r2 disasm of FUN_005b8fc0 at 0x5b904f / 0x5b906d.  Retail passes
 * `target` = the attached back-buffer surface in every callsite and
 * `flags` = 1 (DDFLIP_WAIT). */
int zdd_surface_flip(void *surface, void *target, uint32_t flags,
                     zdd *log_owner)
{
    if (surface == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    LPDIRECTDRAWSURFACE7 tgt  = (LPDIRECTDRAWSURFACE7)target;
    HRESULT hr = surf->lpVtbl->Flip(surf, tgt, (DWORD)flags);
    if (FAILED(hr)) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface", "Flip",
                          (int32_t)hr);
        }
        return 0;
    }
    return 1;
}

/* IDirectDrawSurface7::Blt via vtable[5] (byte 0x14).  Verified by r2
 * disasm at 0x5b903a / 0x5b9098 / 0x5b9aa5.  The 4-int RECT shape is
 * portable across host + Win32 because Windows RECT is also 4 LONGs in
 * the same {left, top, right, bottom} order. */
int zdd_surface_blt(void *dest, const int32_t *dest_rect,
                    void *src,  const int32_t *src_rect,
                    uint32_t flags, zdd *log_owner)
{
    if (dest == NULL) return 0;
    LPDIRECTDRAWSURFACE7 dst_surf = (LPDIRECTDRAWSURFACE7)dest;
    LPDIRECTDRAWSURFACE7 src_surf = (LPDIRECTDRAWSURFACE7)src;
    /* RECT is identical to 4×int32 in {left,top,right,bottom}; cast
     * through a local copy to dodge any const/alignment warts. */
    RECT dr, sr;
    LPRECT pdr = NULL, psr = NULL;
    if (dest_rect != NULL) {
        dr.left   = (LONG)dest_rect[0];
        dr.top    = (LONG)dest_rect[1];
        dr.right  = (LONG)dest_rect[2];
        dr.bottom = (LONG)dest_rect[3];
        pdr = &dr;
    }
    if (src_rect != NULL) {
        sr.left   = (LONG)src_rect[0];
        sr.top    = (LONG)src_rect[1];
        sr.right  = (LONG)src_rect[2];
        sr.bottom = (LONG)src_rect[3];
        psr = &sr;
    }
    HRESULT hr = dst_surf->lpVtbl->Blt(dst_surf, pdr, src_surf, psr,
                                       (DWORD)flags, NULL);
    if (FAILED(hr)) {
        if (log_owner != NULL) {
            zdd_log_dderr(log_owner, "DirectDrawSurface", "Blt",
                          (int32_t)hr);
        }
        return 0;
    }
    return 1;
}

/* Desktop-DC composite: GetDC(NULL) + BitBlt + ReleaseDC(NULL, hdc).
 * Mirrors FUN_005b8fc0 case 2 lines 0x5b90b2..0x5b90f2.  Retail uses
 * `hWnd = NULL` for GetDC (set at the top of FUN_005b8fc0:0x5b8fc5)
 * to get the desktop DC — the surface gets composited into the
 * desktop at (dest_x, dest_y) regardless of which window owns the
 * area.  Caller (main.c) must keep dest_x/dest_y aligned with the
 * window's client-area screen position for the composite to land
 * inside the window. */
int zdd_desktop_present(void *src_hdc, int dest_x, int dest_y,
                        int width, int height)
{
    if (src_hdc == NULL) return 0;
    HDC desktop = GetDC(NULL);
    if (desktop == NULL) return 0;
    /* SRCCOPY = 0x00CC0020 — matches retail's literal push at
     * 0x5b90c7. */
    BitBlt(desktop, dest_x, dest_y, width, height,
           (HDC)src_hdc, 0, 0, SRCCOPY);
    ReleaseDC(NULL, desktop);
    return 1;
}

/* BeginPaint wrapper.  Retail's FUN_005b9130 holds a PAINTSTRUCT on its
 * own stack frame across the begin/blit/end window; we split the begin
 * and end into separate primitives so the pure-logic body can be
 * Win32-free.  PAINTSTRUCT lives on the heap for the duration of the
 * paint session so its address is stable across the two calls.  The
 * cookie returned via *out_cookie is opaque to the caller (it's a
 * malloc'd PAINTSTRUCT*).  The caller MUST hand it back to
 * zdd_window_paint_end on the matching hwnd, otherwise the cookie
 * leaks and the window's update region stays dirty. */
void *zdd_window_paint_begin(void *hwnd, void **out_cookie)
{
    if (out_cookie != NULL) *out_cookie = NULL;
    if (hwnd == NULL) return NULL;
    PAINTSTRUCT *ps = (PAINTSTRUCT *)malloc(sizeof(PAINTSTRUCT));
    if (ps == NULL) return NULL;
    HDC hdc = BeginPaint((HWND)hwnd, ps);
    if (out_cookie != NULL) *out_cookie = ps;
    /* Retail does NOT NULL-check BeginPaint's return (0x5b914b → ebx
     * straight into BitBlt at 0x5b918d); mirror by handing back
     * whatever — NULL or otherwise — to the caller.  Downstream
     * primitives guard defensively. */
    return (void *)hdc;
}

/* EndPaint wrapper.  Frees the cookie allocated by
 * zdd_window_paint_begin and validates the window's update region.
 * NULL hwnd or NULL cookie is a no-op (the heap is left alone, and the
 * window stays dirty — the caller's contract violation is its own to
 * resolve). */
void zdd_window_paint_end(void *hwnd, void *cookie)
{
    if (hwnd == NULL || cookie == NULL) return;
    PAINTSTRUCT *ps = (PAINTSTRUCT *)cookie;
    EndPaint((HWND)hwnd, ps);
    free(ps);
}

/* GDI BitBlt with SRCCOPY.  No HDC acquisition — both HDCs are
 * caller-supplied.  Mirrors the BitBlt at FUN_005b9130:0x5b918e with
 * the same literal rop (0xCC0020 = SRCCOPY).  NULL guards on both HDCs
 * to avoid GDI's silent failure mode (returns FALSE, no diagnostic). */
void zdd_window_blit_copy(void *dest_hdc, int dest_x, int dest_y,
                          int width, int height, void *src_hdc)
{
    if (dest_hdc == NULL || src_hdc == NULL) return;
    BitBlt((HDC)dest_hdc, dest_x, dest_y, width, height,
           (HDC)src_hdc, 0, 0, SRCCOPY);
}

/* IDirectDrawSurface7::GetSurfaceDesc via vtable[22] (byte 0x58).
 * Verified by r2 disasm of FUN_005b8a20: `call dword [eax + 0x58]`.
 * Caller pre-stamps dwSize + dwFlags on the descriptor. */
int zdd_surface_get_desc(void *surface, void *ddsd)
{
    if (surface == NULL || ddsd == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    HRESULT hr = surf->lpVtbl->GetSurfaceDesc(surf,
                                              (LPDDSURFACEDESC2)ddsd);
    return FAILED(hr) ? 0 : 1;
}

/* IDirectDrawSurface7::Lock via vtable[25] (byte 0x64).  Verified by
 * r2 disasm of FUN_005b9490: `call dword [eax + 0x64]`.  Args mirror
 * retail's literal push order: lpDestRect=NULL, lpDDSurfaceDesc=ddsd,
 * dwFlags=flags, hEvent=NULL.  out_hr receives the raw HRESULT so the
 * pure-logic caller can route it through zdd_log_dderr. */
int zdd_surface_lock(void *surface, void *ddsd, uint32_t flags,
                     int32_t *out_hr)
{
    if (surface == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    HRESULT hr = surf->lpVtbl->Lock(surf, NULL, (LPDDSURFACEDESC2)ddsd,
                                    (DWORD)flags, NULL);
    if (out_hr != NULL) *out_hr = (int32_t)hr;
    return FAILED(hr) ? 0 : 1;
}

/* IDirectDrawSurface7::Unlock via vtable[32] (byte 0x80).  Verified
 * by r2 disasm of FUN_005b94d0: `call dword [eax + 0x80]`.  Retail
 * always passes lpRect=NULL ("release entire surface").  Return value
 * dropped — retail's caller doesn't read it. */
void zdd_surface_unlock(void *surface)
{
    if (surface == NULL) return;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    surf->lpVtbl->Unlock(surf, NULL);
}

/* IDirectDrawSurface7::IsLost via vtable[24] (byte 0x60).  Returns
 * the raw HRESULT; the pure-logic caller compares against
 * DDERR_SURFACELOST. */
int32_t zdd_surface_is_lost(void *surface)
{
    if (surface == NULL) return 0;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    HRESULT hr = surf->lpVtbl->IsLost(surf);
    return (int32_t)hr;
}

/* IDirectDrawSurface7::Restore via vtable[27] (byte 0x6c).  Return
 * value dropped — retail discards. */
void zdd_surface_restore(void *surface)
{
    if (surface == NULL) return;
    LPDIRECTDRAWSURFACE7 surf = (LPDIRECTDRAWSURFACE7)surface;
    surf->lpVtbl->Restore(surf);
}

/* Read post-Lock DDSD slots out of the zdd_object's embedded DDSD.
 * On the real (32-bit Win32) build the DDSD layout matches retail
 * exactly — lpSurface is at +0x54 (embedded_ddsd[0x24]), lPitch at
 * +0x40, dwHeight at +0x38.  Direct reads via the documented offsets.
 * Returns 1 if `self` is non-NULL; out pointers may be NULL. */
int zdd_object_get_locked_info(zdd_object *self, void **out_buf,
                               int32_t *out_pitch, int32_t *out_height)
{
    if (self == NULL) return 0;
    if (out_buf != NULL) {
        *out_buf = *(void **)&self->embedded_ddsd[0x24];
    }
    if (out_pitch != NULL) {
        *out_pitch = *(int32_t *)&self->embedded_ddsd[0x10];
    }
    if (out_height != NULL) {
        *out_height = *(int32_t *)&self->embedded_ddsd[0x08];
    }
    return 1;
}
