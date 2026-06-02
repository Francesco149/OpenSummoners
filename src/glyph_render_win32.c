/*
 * src/glyph_render_win32.c — the real GDI adapter for the text renderer.
 *
 * Binds glyph_render's injected glyph_gdi_ops vtable to the three Win32 GDI
 * calls 0x48e200/0x48e860/0x48e6d0 make on the back-buffer DC.  The host
 * test harness drives glyph_render with its own recording ops and does NOT
 * link this file (it is picked up only by the src/Makefile wildcard for the
 * real PE build).  See glyph_render.h.
 */
#include "glyph_render.h"

#include <windows.h>

/* `user` is the back-buffer HDC (passed opaquely through glyph_gdi_ops). */
static void win32_select_font(void *user, void *font)
{
    SelectObject((HDC)user, (HGDIOBJ)font);
}

static void win32_set_text_color(void *user, uint32_t color)
{
    SetTextColor((HDC)user, (COLORREF)color);
}

static void win32_text_out(void *user, int x, int y, const char *str, int len)
{
    TextOutA((HDC)user, x, y, str, len);
}

glyph_gdi_ops glyph_gdi_ops_win32(void *hdc)
{
    glyph_gdi_ops ops;
    ops.select_font    = win32_select_font;
    ops.set_text_color = win32_set_text_color;
    ops.text_out       = win32_text_out;
    ops.user           = hdc;
    return ops;
}
