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

/* Numbering (2026-10-15): SLOTS like GDI's own handle table, not a never-reused sequence —
 * retail does a GetDC/ReleaseDC pair every flip, so the pure sequence (cap 16384) died at flip
 * ~16.3k in any longer recording. A freed slot is reused LIFO; the fake's high byte is the
 * slot's GENERATION (bumped on free, skipping 0x00 and 0x7E = rh_handles.c's kernel fakes), so
 * a stale fake (retail using a handle after DeleteObject/ReleaseDC) still translates to an
 * invalid handle, as on real GDI, never to the slot's next object. Fakes keep bits 16..23 zero
 * (GDI object type 0, which no real GDI handle carries: stock objects set bit 23, DCs are type
 * 1), so a passed-through real handle is never mistaken for one. First use of slot s =
 * 0x7D000000 + 4s (the old numbering). Every value is a pure function of retail's
 * create/delete sequence. */
#define GSLOTS 16384                     /* objects alive at once (retail: ~30) */
static HANDLE g_real[GSLOTS];            /* NULL = free */
static uint8_t g_gen[GSLOTS];
static uint16_t g_free[GSLOTS];
static uint32_t g_nfree, g_nslot;        /* g_nslot = high-water mark of live objects */
static uint32_t g_issued;
static long g_live;
enum { GK_BRUSH, GK_PEN, GK_FONT, GK_DIB, GK_CDC, GK_GETDC, GK_N };
static uint32_t g_made[GK_N];            /* fresh fakes per import (diagnostic: who churns handles) */
static int g_kind = GK_N;

static HANDLE mk(uint32_t s) { return (HANDLE)(uintptr_t)(((uint32_t)g_gen[s] << 24) | (s << 2)); }
static int slot_of(HANDLE h)             /* -1: not one of our fakes (any generation) */
{
    uint32_t v = (uint32_t)(uintptr_t)h;
    if ((v & 0x00FF0003u) || !(v >> 24)) return -1;
    uint32_t s = (v & 0xFFFFu) >> 2;
    return s < g_nslot ? (int)s : -1;
}
static int live_slot(HANDLE h)           /* the slot h names, if h is its CURRENT fake */
{
    int s = slot_of(h);
    return s >= 0 && g_real[s] && g_gen[s] == (uint32_t)(uintptr_t)h >> 24 ? s : -1;
}
static int find_real(HANDLE real)
{
    for (uint32_t s = 0; s < g_nslot; s++) if (g_real[s] == real) return (int)s;
    return -1;
}
static HANDLE gf(HANDLE real)
{
    if (!real) return real;
    int s = find_real(real);
    if (s >= 0) return mk(s);           /* already known (e.g. a class DC handed out twice) */
    if (g_nfree) s = g_free[--g_nfree];
    else {
        if (g_nslot >= GSLOTS) rh_fatal("rh_gdi: more than %d live GDI objects", GSLOTS);
        s = g_nslot++; g_gen[s] = 0x7D;
    }
    g_real[s] = real; g_live++; g_issued++;
    if (g_kind < GK_N) g_made[g_kind]++;
    return mk(s);
}
static HANDLE gr(HANDLE h)
{
    if (slot_of(h) < 0) return h;
    int s = live_slot(h);
    return s >= 0 ? g_real[s] : (HANDLE)(uintptr_t)0xDEADDEADu;   /* stale: fails like a deleted handle */
}
static HANDLE gback(HANDLE real)         /* real → the fake retail knows, if any */
{
    if (!real) return real;
    int s = find_real(real);
    return s >= 0 ? mk(s) : real;
}
static void gforget(HANDLE h)
{
    int s = live_slot(h);
    if (s < 0) return;
    g_real[s] = NULL; g_live--;
    do g_gen[s]++; while (g_gen[s] == 0x00 || g_gen[s] == 0x7E);
    g_free[g_nfree++] = (uint16_t)s;
}
#define R(h) ((void *)gr((HANDLE)(h)))

static HANDLE gfk(HANDLE real, int kind) { g_kind = kind; HANDLE f = gf(real); g_kind = GK_N; return f; }
static HBRUSH WINAPI g_CreateBrushIndirect(const LOGBRUSH *lb) { return gfk(CreateBrushIndirect(lb), GK_BRUSH); }
static HPEN WINAPI g_CreatePen(int s, int w, COLORREF c) { return gfk(CreatePen(s, w, c), GK_PEN); }
static HFONT WINAPI g_CreateFontIndirectA(const LOGFONTA *lf) { return gfk(CreateFontIndirectA(lf), GK_FONT); }
static HBITMAP WINAPI g_CreateDIBitmap(HDC dc, const BITMAPINFOHEADER *h, DWORD i, const void *b, const BITMAPINFO *bi, UINT u)
{ return gfk(CreateDIBitmap(R(dc), h, i, b, bi, u), GK_DIB); }
static HDC WINAPI g_CreateCompatibleDC(HDC dc) { return gfk(CreateCompatibleDC(R(dc)), GK_CDC); }
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
static HDC WINAPI g_GetDC(HWND w) { return gfk(GetDC(w), GK_GETDC); }
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
void rh_gdi_stats(char *b, int n)
{
    snprintf(b, n, "kh=%u/%ld gdi=%u/%ld/hw%u(brush %u pen %u font %u dib %u cdc %u getdc %u)", rh_handles_issued(), rh_handles_live(), g_issued, g_live, g_nslot,
             g_made[GK_BRUSH], g_made[GK_PEN], g_made[GK_FONT], g_made[GK_DIB], g_made[GK_CDC], g_made[GK_GETDC]);
}
void *rh_gdi_real(void *h) { return gr((HANDLE)h); }
