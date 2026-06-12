/* engine_gdi.h — the trace-studio-v2 capture-proxy GDI TEXT capture (M3d).
 *
 * The retail engine renders ALL dynamic text + the prologue narration through
 * Win32 GDI: it SelectObject's one of the 8 boot-built HFONTs into the backbuffer
 * DC, SetTextColor/SetBkMode, then TextOutA's each glyph straight onto the
 * backbuffer — OUTSIDE the 5 DDraw blit primitives (text-glyph-pipeline.md, quirk
 * #62/#63).  So to make the .osr a COMPLETE frame description the proxy must also
 * record that text stream; the M4 replayer re-issues these on Windows so the
 * glyphs reconstruct bit-exact (real GDI both sides).
 *
 * Mechanism: IAT-patch the engine's gdi32 imports (iat_hook.h).  Unlike the engine
 * VAs (INT3/E9 trampolines, onEnter-only), an IAT swap gives us a full wrapper that
 * calls the real function and SEES ITS RETURN VALUE — exactly what CreateFontIndirectA
 * (the new HFONT) needs, with no onLeave framework.  Near-zero per-call cost (a slot
 * swap), and we intercept only the engine's own calls.  Verified imports (objdump):
 * GDI32.dll!{TextOutA,CreateFontIndirectA,SelectObject,SetTextColor,SetBkMode}; the
 * engine uses TextOutA (no ExtTextOutA) and SetBkMode (no SetBkColor).
 *
 * State model (engine-thread-only, like sheet_grab's cache — no locking):
 *   - a font cache HFONT→font_ref: CreateFontIndirectA interns the returned HFONT
 *     and emits ONE dedup'd FONT record (the LOGFONTA the replayer recreates);
 *   - a tiny per-HDC state table {font_ref, color, bk_mode}: SelectObject updates
 *     the current font (only when the selected object is a KNOWN HFONT — it is also
 *     called with bitmaps/pens), SetTextColor/SetBkMode update the colour/mode;
 *   - TextOutA emits a TEXT record with that state, the position, the string, the
 *     shared per-frame draw seq (eh_next_draw_seq), and the backbuffer dst handle
 *     (eh_backbuffer_handle — the single dst surface from the blit path, M3c).
 *
 * MUST be #included AFTER engine_hooks.h (it uses eh_next_draw_seq /
 * eh_backbuffer_handle) and installed after engine_hooks_install().
 */
#ifndef OSS_ENGINE_GDI_H
#define OSS_ENGINE_GDI_H

#include <windows.h>

#include "proxy_log.h"
#include "iat_hook.h"
#include "osr_writer.h"

/* ── font cache: HFONT → stable 1-based font_ref (dedup; FONT on first sight) ── */
#define EG_FONT_SLOTS 64u
typedef struct { HFONT key; uint32_t ref; } eg_font_ent;
static eg_font_ent g_eg_fonts[EG_FONT_SLOTS];
static uint32_t    g_eg_font_next = 1;          /* 0 = none/unknown */

static uint32_t eg_font_ref_of(HFONT f)
{
    if (!f) return 0;
    for (unsigned i = 0; i < EG_FONT_SLOTS; i++)
        if (g_eg_fonts[i].key == f) return g_eg_fonts[i].ref;
    return 0;                                    /* not a font we created */
}

/* Intern a newly created HFONT → a fresh font_ref (or its existing one if a handle
 * value got recycled).  Returns 0 if the table is full. */
static uint32_t eg_font_intern(HFONT f)
{
    for (unsigned i = 0; i < EG_FONT_SLOTS; i++) {
        if (g_eg_fonts[i].key == f)   return g_eg_fonts[i].ref;
        if (g_eg_fonts[i].key == NULL) {
            uint32_t r = g_eg_font_next++;
            g_eg_fonts[i].key = f; g_eg_fonts[i].ref = r;
            return r;
        }
    }
    return 0;
}

/* ── per-HDC text state ────────────────────────────────────────────────────── */
#define EG_HDC_SLOTS 16u
typedef struct {
    HDC      hdc;
    uint32_t font_ref;
    uint32_t color;
    int32_t  bk_mode;
} eg_hdc_state;
static eg_hdc_state g_eg_hdc[EG_HDC_SLOTS];

static eg_hdc_state *eg_hdc_get(HDC hdc)
{
    eg_hdc_state *freeslot = NULL;
    for (unsigned i = 0; i < EG_HDC_SLOTS; i++) {
        if (g_eg_hdc[i].hdc == hdc) return &g_eg_hdc[i];
        if (!g_eg_hdc[i].hdc && !freeslot) freeslot = &g_eg_hdc[i];
    }
    if (!freeslot) freeslot = &g_eg_hdc[0];      /* recycle: DCs are very few */
    freeslot->hdc = hdc;
    freeslot->font_ref = 0;
    freeslot->color = 0;                         /* GDI default text colour: black */
    freeslot->bk_mode = OPAQUE;                  /* GDI default bk mode: OPAQUE (2) */
    return freeslot;
}

/* ── the real (pre-hook) gdi32 entry points ───────────────────────────────── */
typedef BOOL     (WINAPI *TextOutA_t)(HDC, int, int, LPCSTR, int);
typedef HFONT    (WINAPI *CreateFontIndirectA_t)(const LOGFONTA *);
typedef HGDIOBJ  (WINAPI *SelectObject_t)(HDC, HGDIOBJ);
typedef COLORREF (WINAPI *SetTextColor_t)(HDC, COLORREF);
typedef int      (WINAPI *SetBkMode_t)(HDC, int);

static TextOutA_t            real_TextOutA;
static CreateFontIndirectA_t real_CreateFontIndirectA;
static SelectObject_t        real_SelectObject;
static SetTextColor_t        real_SetTextColor;
static SetBkMode_t           real_SetBkMode;

/* ── wrappers (call the real fn, record the relevant state/op) ─────────────── */
#define EG_TEXT_MAX 480u   /* matches osr_w_text's stack buffer cap */

static HFONT WINAPI eg_CreateFontIndirectA(const LOGFONTA *lf)
{
    HFONT f = real_CreateFontIndirectA(lf);
    if (f && lf) {
        uint32_t ref = eg_font_intern(f);
        if (ref) {
            osr_font of;
            memset(&of, 0, sizeof(of));
            of.font_ref    = ref;
            of.height      = lf->lfHeight;
            of.width       = lf->lfWidth;
            of.escapement  = lf->lfEscapement;
            of.orientation = lf->lfOrientation;
            of.weight      = lf->lfWeight;
            of.italic      = lf->lfItalic;
            of.underline   = lf->lfUnderline;
            of.strikeout   = lf->lfStrikeOut;
            of.charset     = lf->lfCharSet;
            of.out_prec    = lf->lfOutPrecision;
            of.clip_prec   = lf->lfClipPrecision;
            of.quality     = lf->lfQuality;
            of.pitch_family= lf->lfPitchAndFamily;
            memcpy(of.face, lf->lfFaceName, 32);   /* LF_FACESIZE; nul-padded */
            osr_w_font(&of);
            proxy_logf("[gdi] font ref=%u '%s' h=%ld w=%ld wt=%ld cs=%u",
                       ref, lf->lfFaceName, (long)lf->lfHeight,
                       (long)lf->lfWidth, (long)lf->lfWeight,
                       (unsigned)lf->lfCharSet);
        }
    }
    return f;
}

static HGDIOBJ WINAPI eg_SelectObject(HDC hdc, HGDIOBJ obj)
{
    /* SelectObject is also used for bitmaps/pens/brushes/regions — only treat it
     * as a font-select when the object is one we created (a known HFONT). */
    uint32_t ref = eg_font_ref_of((HFONT)obj);
    if (ref) eg_hdc_get(hdc)->font_ref = ref;
    return real_SelectObject(hdc, obj);
}

static COLORREF WINAPI eg_SetTextColor(HDC hdc, COLORREF c)
{
    eg_hdc_get(hdc)->color = (uint32_t)c;
    return real_SetTextColor(hdc, c);
}

static int WINAPI eg_SetBkMode(HDC hdc, int mode)
{
    eg_hdc_get(hdc)->bk_mode = mode;
    return real_SetBkMode(hdc, mode);
}

static BOOL WINAPI eg_TextOutA(HDC hdc, int x, int y, LPCSTR str, int c)
{
    if (str && c > 0) {
        uint32_t n = (uint32_t)c;
        if (n > EG_TEXT_MAX) n = EG_TEXT_MAX;
        eg_hdc_state *st = eg_hdc_get(hdc);
        osr_text t;
        memset(&t, 0, sizeof(t));
        t.seq        = eh_next_draw_seq();         /* shared with BLIT.seq */
        t.dst_handle = eh_backbuffer_handle();     /* the single backbuffer */
        t.x = x; t.y = y;
        t.font_ref   = st->font_ref;
        t.color      = st->color;
        t.bk_mode    = st->bk_mode;
        t.str_len    = n;
        t.str        = str;
        osr_w_text(&t);
    }
    return real_TextOutA(hdc, x, y, str, c);
}

/* ── install the GDI text hooks (after engine_hooks_install) ──────────────── */
static void engine_gdi_install(void)
{
    HMODULE exe = GetModuleHandleA(NULL);
    real_TextOutA = (TextOutA_t)
        iat_hook(exe, "gdi32.dll", "TextOutA", (void *)eg_TextOutA);
    real_CreateFontIndirectA = (CreateFontIndirectA_t)
        iat_hook(exe, "gdi32.dll", "CreateFontIndirectA", (void *)eg_CreateFontIndirectA);
    real_SelectObject = (SelectObject_t)
        iat_hook(exe, "gdi32.dll", "SelectObject", (void *)eg_SelectObject);
    real_SetTextColor = (SetTextColor_t)
        iat_hook(exe, "gdi32.dll", "SetTextColor", (void *)eg_SetTextColor);
    real_SetBkMode = (SetBkMode_t)
        iat_hook(exe, "gdi32.dll", "SetBkMode", (void *)eg_SetBkMode);
    proxy_logf("[gdi] GDI text hooks installed (TextOutA=%s font=%s select=%s "
               "color=%s bkmode=%s)",
               real_TextOutA ? "ok" : "MISS",
               real_CreateFontIndirectA ? "ok" : "MISS",
               real_SelectObject ? "ok" : "MISS",
               real_SetTextColor ? "ok" : "MISS",
               real_SetBkMode ? "ok" : "MISS");
}

#endif /* OSS_ENGINE_GDI_H */
