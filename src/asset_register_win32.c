/*
 * src/asset_register_win32.c — GDI primitive wrappers for the real
 * Windows build.  The host test harness provides its own stubs (see
 * tests/test_asset_register.c) and does NOT link this file.
 *
 * Single-TU build picks this up via the src/Makefile wildcard.
 */
#include "asset_register.h"

#include <windows.h>
#include <wingdi.h>
#include <string.h>

/* Retail engine's "font face override" string.  Set by the launcher
 * dialog when the user picks a non-default UI font; copied byte-by-byte
 * into LOGFONTA.lfFaceName by ar_make_font on the CreateFontIndirectA
 * retry path.  We keep it as a writable global so a future launcher
 * port can stash a face name here without touching this file. */
char g_ar_fallback_face_name[LF_FACESIZE] = "";

ar_gdi_handle ar_gdi_create_font(int32_t width, int32_t height,
                                  int italic, const char *face)
{
    LOGFONTA lf;
    memset(&lf, 0, sizeof lf);
    lf.lfWidth   = (LONG)width;
    lf.lfHeight  = (LONG)height;
    lf.lfWeight  = 400;            /* FW_NORMAL — retail literal. */
    lf.lfCharSet = 1;              /* DEFAULT_CHARSET — retail literal. */
    lf.lfItalic  = (BYTE)(italic ? 1 : 0);
    if (face != NULL) {
        strncpy(lf.lfFaceName, face, LF_FACESIZE - 1);
        lf.lfFaceName[LF_FACESIZE - 1] = '\0';
    }
    return (ar_gdi_handle)CreateFontIndirectA(&lf);
}

ar_gdi_handle ar_gdi_create_pen(int style, int32_t width, uint32_t color)
{
    return (ar_gdi_handle)CreatePen(style, (int)width, (COLORREF)color);
}

ar_gdi_handle ar_gdi_create_brush(uint32_t color)
{
    LOGBRUSH lb;
    lb.lbStyle = BS_SOLID;
    lb.lbColor = (COLORREF)color;
    lb.lbHatch = 0;
    return (ar_gdi_handle)CreateBrushIndirect(&lb);
}

void ar_gdi_delete(ar_gdi_handle h)
{
    if (h != NULL) DeleteObject((HGDIOBJ)h);
}

const char *ar_gdi_get_fallback_face_name(void)
{
    return g_ar_fallback_face_name;
}
