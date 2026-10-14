/* rh_gdi.c — deterministic GDI handle values for retail.
 *
 * Retail keeps GDI object handles in its heap (fonts/brushes/pens of the text + primitive
 * drawers: the type byte shows in the value — 0x0a font, 0x10 brush, 0x30 pen), and GDI
 * numbers handles per session, so ~100 heap dwords differed run-to-run (2026-10-14 det gate,
 * frame 1). Every GDI-handle-RETURNING retail import yields a fake, sequence-numbered handle;
 * every consuming import translates back; SelectObject's return (the previously selected
 * object) maps back to the fake retail gave it. Handles retail never got from us (stock
 * objects, DirectDraw surface DCs) pass through unchanged. */
#include "rhost.h"

#define GFAKE_BASE 0x7D000000u
#define GFAKE_MAX  16384
static HANDLE g_real[GFAKE_MAX];
static uint32_t g_next;
static long g_live;

static HANDLE gf(HANDLE real)
{
    if (!real) return real;
    for (uint32_t k = 0; k < g_next; k++) if (g_real[k] == real) return (HANDLE)(uintptr_t)(GFAKE_BASE + 4u * k);   /* already known */
    if (g_next >= GFAKE_MAX) rh_fatal("rh_gdi: more than %d GDI handles", GFAKE_MAX);
    uint32_t k = g_next++;
    g_real[k] = real; g_live++;
    return (HANDLE)(uintptr_t)(GFAKE_BASE + 4u * k);
}
static HANDLE gr(HANDLE h)
{
    uint32_t v = (uint32_t)(uintptr_t)h;
    if (v < GFAKE_BASE || v >= GFAKE_BASE + 4u * GFAKE_MAX || (v & 3)) return h;
    return g_real[(v - GFAKE_BASE) >> 2];
}
static HANDLE gback(HANDLE real)   /* real → the fake retail knows, if any */
{
    if (!real) return real;
    for (uint32_t k = 0; k < g_next; k++) if (g_real[k] == real) return (HANDLE)(uintptr_t)(GFAKE_BASE + 4u * k);
    return real;
}
static void gforget(HANDLE h)
{
    uint32_t v = (uint32_t)(uintptr_t)h;
    if (v < GFAKE_BASE || v >= GFAKE_BASE + 4u * GFAKE_MAX || (v & 3)) return;
    g_real[(v - GFAKE_BASE) >> 2] = (HANDLE)(uintptr_t)0xDEADDEADu; g_live--;   /* never reused: numbering stays a pure sequence */
}
#define R(h) ((void *)gr((HANDLE)(h)))

static HBRUSH WINAPI g_CreateBrushIndirect(const LOGBRUSH *lb) { return gf(CreateBrushIndirect(lb)); }
static HPEN WINAPI g_CreatePen(int s, int w, COLORREF c) { return gf(CreatePen(s, w, c)); }
static HFONT WINAPI g_CreateFontIndirectA(const LOGFONTA *lf) { return gf(CreateFontIndirectA(lf)); }
static HBITMAP WINAPI g_CreateDIBitmap(HDC dc, const BITMAPINFOHEADER *h, DWORD i, const void *b, const BITMAPINFO *bi, UINT u)
{ return gf(CreateDIBitmap(R(dc), h, i, b, bi, u)); }
static HDC WINAPI g_CreateCompatibleDC(HDC dc) { return gf(CreateCompatibleDC(R(dc))); }
static BOOL WINAPI g_DeleteDC(HDC dc) { BOOL r = DeleteDC(R(dc)); gforget(dc); return r; }
static BOOL WINAPI g_DeleteObject(HGDIOBJ o) { BOOL r = DeleteObject(R(o)); gforget(o); return r; }
static HGDIOBJ WINAPI g_SelectObject(HDC dc, HGDIOBJ o) { return gback(SelectObject(R(dc), R(o))); }
static UINT WINAPI g_GetSystemPaletteEntries(HDC dc, UINT s, UINT n, LPPALETTEENTRY p) { return GetSystemPaletteEntries(R(dc), s, n, p); }
static int WINAPI g_StretchDIBits(HDC dc, int x, int y, int w, int h, int sx, int sy, int sw, int sh, const void *b, const BITMAPINFO *bi, UINT u, DWORD rop)
{ return StretchDIBits(R(dc), x, y, w, h, sx, sy, sw, sh, b, bi, u, rop); }
static BOOL WINAPI g_Polygon(HDC dc, const POINT *p, int n) { return Polygon(R(dc), p, n); }
static BOOL WINAPI g_Polyline(HDC dc, const POINT *p, int n) { return Polyline(R(dc), p, n); }
static BOOL WINAPI g_BitBlt(HDC d, int x, int y, int w, int h, HDC s, int sx, int sy, DWORD rop) { return BitBlt(R(d), x, y, w, h, R(s), sx, sy, rop); }
static int WINAPI g_GetDIBits(HDC dc, HBITMAP bm, UINT s, UINT n, LPVOID b, LPBITMAPINFO bi, UINT u) { return GetDIBits(R(dc), R(bm), s, n, b, bi, u); }
static BOOL WINAPI g_Arc(HDC dc, int a, int b, int c, int d, int e, int f, int g, int h) { return Arc(R(dc), a, b, c, d, e, f, g, h); }
static int WINAPI g_SetBkMode(HDC dc, int m) { return SetBkMode(R(dc), m); }
static COLORREF WINAPI g_SetTextColor(HDC dc, COLORREF c) { return SetTextColor(R(dc), c); }
static BOOL WINAPI g_TextOutA(HDC dc, int x, int y, LPCSTR s, int n) { return TextOutA(R(dc), x, y, s, n); }
static HDC WINAPI g_GetDC(HWND w) { return gf(GetDC(w)); }
static int WINAPI g_ReleaseDC(HWND w, HDC dc) { int r = ReleaseDC(w, R(dc)); gforget(dc); return r; }

void rh_gdi_install(void)
{
    iat_try("CreateBrushIndirect", g_CreateBrushIndirect);
    iat_try("CreatePen", g_CreatePen);
    iat_try("CreateFontIndirectA", g_CreateFontIndirectA);
    iat_try("CreateDIBitmap", g_CreateDIBitmap);
    iat_try("CreateCompatibleDC", g_CreateCompatibleDC);
    iat_try("DeleteDC", g_DeleteDC);
    iat_try("DeleteObject", g_DeleteObject);
    iat_try("SelectObject", g_SelectObject);
    iat_try("GetSystemPaletteEntries", g_GetSystemPaletteEntries);
    iat_try("StretchDIBits", g_StretchDIBits);
    iat_try("Polygon", g_Polygon);
    iat_try("Polyline", g_Polyline);
    iat_try("BitBlt", g_BitBlt);
    iat_try("GetDIBits", g_GetDIBits);
    iat_try("Arc", g_Arc);
    iat_try("SetBkMode", g_SetBkMode);
    iat_try("SetTextColor", g_SetTextColor);
    iat_try("TextOutA", g_TextOutA);
    iat_try("GetDC", g_GetDC);
    iat_try("ReleaseDC", g_ReleaseDC);
}
long rh_gdi_live(void) { return g_live; }
void *rh_gdi_real(void *h) { return gr((HANDLE)h); }
