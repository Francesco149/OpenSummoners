/* rh_dev.c — --headless: hermetic devices for SotES (retail and the standalone alike).
 *
 * The device surface SotES really uses was measured with `rhost --dev-census` on the Windows real device
 * (rh-combat + rh-my-play + rh-water-shrine, 2026-11-13; docs/findings/headless-devices.md):
 *   DirectDraw7 — a system-memory blitter only: every surface is OFFSCREENPLAIN|SYSTEMMEMORY 16-bpp 565
 *     (no primary, no Flip, no palette); Blt with DDBLT_WAIT or DDBLT_WAIT|DDBLT_KEYSRC (key 0xF81F), never
 *     stretched / colour-filled / self; Lock(NULL, DDLOCK_WAIT) whole-surface; GetDC/ReleaseDC on the 640x480
 *     composition surface (~5.7 a frame: the GDI text layer); one clipper per window surface.  The present is GDI
 *     (GetDC(window) + BitBlt).
 *   DirectShow — the BGM graph: RenderFile, Run/Pause/Stop, SetPositions/GetPositions/GetDuration, put_Volume.
 * Here every one of those is a fake whose state lives in the DEV region 0xE8000000.. (deterministic allocator,
 * metadata in-band) so a run needs no display, no audio device, no codec — wine's null driver or Windows session 0
 * — and ALL device state is plain memory a savestate can copy.
 *
 * Exactness (gated: `rhost.py headless-check` = headless vs real-device on Windows, frame / RNG / state / every
 * presented frame's pixels):
 *   - Lock pitch = round_up(width*2, 8); the desc's flags 0x100f (+DDSD_CKSRCBLT once a src key is set).
 *   - Blt: an empty rect (right <= left or bottom <= top, dest or source) = DDERR_INVALIDRECT (the only failure
 *     retail meets: 32k zero-width rects in 73k frames); with a clipper the dest rect is clipped to the surface and
 *     the source offset by the same amounts; without one an out-of-surface rect is DDERR_INVALIDRECT.
 *   - KEYSRC: a source pixel equal to the key (low..high) is skipped.
 *   - BGM duration = the ASF File Properties' PlayDuration - Preroll*10^4 (= quartz's GetDuration on every track);
 *     the position runs on the virtual clock exactly as rh_media's patched quartz does.
 *   - GetDC hands back a DC in its default state every time (census: bk OPAQUE, text black, bk white, stock font and
 *     pen, align 0 on all 211,677 real GetDCs), so no DC state survives a ReleaseDC (nor needs a savestate).
 *   - GetDC: large surfaces live in a page-file section mapped at a fixed address with GDI's DIB section over the
 *     SAME pages (no copies); GDI's writes do not fault into savestate tracking, so ReleaseDC marks the range.
 *
 * Method tables come from the mingw headers (gen_dev.py → rh_dev_gen.h): a counting S_OK stub for every method not
 * written here; the ones retail really called are listed at exit ("dev: defaulted ..."). */
#define CINTERFACE
#define COBJMACROS
#include "rhost.h"
#include <ddraw.h>
#include <strmif.h>
#include <control.h>

/* ================= DEV region: deterministic allocator, metadata in-band (openrecet's rh_dev.c) ================= */
#define DEV_MAGIC 0x56454452u   /* "RDEV" */
#define DEV_NCLS 48
#define DIB_SLOTS 96            /* 96 x 640 KB from RH_DIB_LO = up to 0xf7c00000 */
typedef struct {
    uint32_t magic, top, live, bytes, nalloc, nfree, bigfree, pad;
    uint32_t fl[DEV_NCLS];
    uint32_t dib_used[DIB_SLOTS / 32], dib_hw;   /* section slots: in use / ever mapped high-water (allocator state) */
} devhdr_t;
typedef struct { uint32_t cap, cls, magic, req; } dblk_t;
typedef struct { uint32_t next, size; } dspan_t;
#define DH ((devhdr_t *)(uintptr_t)RH_DEV_LO)
#define DBLK_LIVE 0xD0B10C4Bu
#define DCLS_BIG 0xFFFFu
static uint32_t g_dev_committed;

static void dev_commit(uint32_t top)
{
    if (top <= g_dev_committed) return;
    uint32_t want = (top + 0xfffffu) & ~0xfffffu;
    if (want > RH_DEV_HI) rh_fatal("rh_dev: DEV region exhausted (top %08x)", top);
    if (!VirtualAlloc((void *)(uintptr_t)g_dev_committed, want - g_dev_committed, MEM_COMMIT, PAGE_READWRITE))
        rh_fatal("rh_dev: commit %08x..%08x failed (err %lu)", g_dev_committed, want, GetLastError());
    rh_dev_committed_hook(g_dev_committed, want);
    g_dev_committed = want;
}
static uint32_t dev_bump(uint32_t sz)
{
    uint32_t p = DH->top;
    dev_commit(p + sz);
    DH->top = p + sz;
    return p;
}
static uint32_t dev_class(uint32_t n, uint32_t *cap)
{
    if (n <= 256) { uint32_t c = n ? (n + 15u) & ~15u : 16u; *cap = c; return c / 16u - 1u; }
    int k = 31 - __builtin_clz(n - 1);
    uint32_t step = 1u << (k - 2), c = (n + step - 1) & ~(step - 1);
    *cap = c;
    return 16u + (uint32_t)(k - 8) * 4u + (c / step - 5u);
}
static void *dev_alloc(uint32_t n)
{
    dblk_t *b;
    if (n <= 0x10000u) {
        uint32_t cap, cls = dev_class(n, &cap), p = DH->fl[cls];
        if (p) { b = (dblk_t *)(uintptr_t)p; DH->fl[cls] = *(uint32_t *)(b + 1); }
        else { b = (dblk_t *)(uintptr_t)dev_bump(cap + sizeof *b); b->cap = cap; b->cls = cls; }
    } else {
        uint32_t chunk = (n + sizeof *b + 0xfffu) & ~0xfffu, *link = &DH->bigfree, p = 0;
        for (uint32_t s = *link; s; link = &((dspan_t *)(uintptr_t)s)->next, s = *link) {
            dspan_t *sp = (dspan_t *)(uintptr_t)s;
            if (sp->size < chunk) continue;
            if (sp->size == chunk) *link = sp->next;
            else { dspan_t *rest = (dspan_t *)(uintptr_t)(s + chunk); rest->next = sp->next; rest->size = sp->size - chunk; *link = s + chunk; }
            p = s; break;
        }
        if (!p) p = dev_bump(chunk);
        b = (dblk_t *)(uintptr_t)p; b->cap = chunk - sizeof *b; b->cls = DCLS_BIG;
    }
    b->magic = DBLK_LIVE; b->req = n;
    memset(b + 1, 0, b->cap);
    DH->live++; DH->bytes += b->cap; DH->nalloc++;
    return b + 1;
}
static void dev_free(void *ptr)
{
    if (!ptr) return;
    dblk_t *b = (dblk_t *)ptr - 1;
    if (b->magic != DBLK_LIVE) { rh_log("rh_dev: free of a dead block %p (f%u)\n", ptr, g_frame); return; }
    b->magic = 0; DH->live--; DH->bytes -= b->cap; DH->nfree++;
    if (b->cls != DCLS_BIG) { *(uint32_t *)(b + 1) = DH->fl[b->cls]; DH->fl[b->cls] = (uint32_t)(uintptr_t)b; return; }
    uint32_t lo = (uint32_t)(uintptr_t)b, sz = b->cap + sizeof *b, *link = &DH->bigfree, prev = 0;
    while (*link && *link < lo) { prev = *link; link = &((dspan_t *)(uintptr_t)*link)->next; }
    dspan_t *n = (dspan_t *)(uintptr_t)lo; n->next = *link; n->size = sz; *link = lo;
    if (n->next && lo + n->size == n->next) { dspan_t *r = (dspan_t *)(uintptr_t)n->next; n->size += r->size; n->next = r->next; }
    if (prev) { dspan_t *l = (dspan_t *)(uintptr_t)prev; if (prev + l->size == lo) { l->size += n->size; l->next = n->next; } }
}
int rh_dev_range(uint32_t *lo, uint32_t *hi)
{
    if (!O.headless || !g_dev_committed) return 0;
    *lo = RH_DEV_LO; *hi = (DH->top + 0xfffu) & ~0xfffu;
    return 1;
}

/* ================= section slots: surface pixels GDI can draw into without a copy =================
 * Slot i = a 640 KB page-file section mapped at RH_DIB_LO + i*RH_DIB_SLOT (fixed, so retail's pointers into it are
 * run-independent) + a DIB section over the same section + a memory DC selecting it.  Mappings / DCs are
 * PROCESS state (host side, never in DEV): a slot once mapped stays mapped; which slots are in use is DEV state. */
typedef struct { HANDLE sec; HDC dc; HBITMAP bmp, old; uint32_t w, h; int saved; } dibslot_t;
static dibslot_t g_dib[DIB_SLOTS];
#define SLOT_VA(i) (RH_DIB_LO + (uint32_t)(i) * RH_DIB_SLOT)
static int slot_alloc(void)
{
    for (int i = 0; i < DIB_SLOTS; i++)
        if (!(DH->dib_used[i / 32] >> (i % 32) & 1)) { DH->dib_used[i / 32] |= 1u << (i % 32); if ((uint32_t)i + 1 > DH->dib_hw) DH->dib_hw = i + 1; return i; }
    return -1;
}
static void slot_free(int i) { DH->dib_used[i / 32] &= ~(1u << (i % 32)); }
static void slot_map(int i)
{
    dibslot_t *s = &g_dib[i];
    if (s->sec) return;
    s->sec = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, RH_DIB_SLOT, NULL);
    if (!s->sec) rh_fatal("rh_dev: CreateFileMapping (slot %d) failed %lu", i, GetLastError());
    if (MapViewOfFileEx(s->sec, FILE_MAP_ALL_ACCESS, 0, 0, RH_DIB_SLOT, (void *)(uintptr_t)SLOT_VA(i)) != (void *)(uintptr_t)SLOT_VA(i))
        rh_fatal("rh_dev: cannot map section slot %d at %08x (err %lu)", i, SLOT_VA(i), GetLastError());
    rh_dev_committed_hook(SLOT_VA(i), SLOT_VA(i) + RH_DIB_SLOT);
}
/* the slot's DC for a w x h 565 surface (re-made when the surface's shape differs from the last one there) */
static HDC slot_dc(int i, uint32_t w, uint32_t h)
{
    dibslot_t *s = &g_dib[i];
    if (s->dc && s->w == w && s->h == h) return s->dc;
    if (s->dc) { SelectObject(s->dc, s->old); DeleteObject(s->bmp); DeleteDC(s->dc); s->dc = NULL; }
    struct { BITMAPINFOHEADER h; DWORD m[3]; } bi = { { sizeof(BITMAPINFOHEADER), (LONG)w, -(LONG)h, 1, 16, BI_BITFIELDS, 0, 0, 0, 0, 0 }, { 0xf800, 0x07e0, 0x001f } };
    void *bits = NULL;
    s->bmp = CreateDIBSection(NULL, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, s->sec, 0);
    if (!s->bmp) rh_fatal("rh_dev: CreateDIBSection over slot %d (%ux%u) failed %lu", i, w, h, GetLastError());
    s->dc = CreateCompatibleDC(NULL);
    s->old = SelectObject(s->dc, s->bmp);
    s->w = w; s->h = h; s->saved = SaveDC(s->dc);
    return s->dc;
}

/* ================= objects ================= */
extern void *vt_IDirectDraw7[], *vt_IDirectDrawSurface7[], *vt_IDirectDrawClipper[], *vt_IGraphBuilder[], *vt_IMediaSeeking[],
            *vt_IMediaControl[], *vt_IBasicAudio[];
typedef struct { void **vt; uint32_t ref; } obj_t;
typedef struct clip clip_t;
typedef struct {
    obj_t o;
    uint32_t w, h, pitch, size, caps, bpp, rmask, gmask, bmask;   /* bpp: EN-old 16 (565) always; SE also makes 24-bpp surfaces */
    uint8_t *bits;
    int32_t slot;                   /* section slot, -1 = DEV heap */
    uint32_t ckflags, cklo, ckhi;   /* DDCKEY_SRCBLT key */
    clip_t *clip;
    uint32_t dc_out;                /* GetDC outstanding */
} surf_t;
#define CLIP_RECTS 8
struct clip { obj_t o; uint32_t has_hwnd, nrect; RECT r[CLIP_RECTS]; };   /* the HWND itself is process state: not kept here */
static obj_t *g_dd;

static void *obj_new(void **vt, uint32_t size) { obj_t *o = dev_alloc(size); o->vt = vt; o->ref = 1; return o; }
#define SIMPLE_UNK(I) \
    static HRESULT WINAPI I##_x_QueryInterface(I *This, REFIID riid, void **ppv) { (void)riid; if (!ppv) return E_POINTER; ((obj_t *)This)->ref++; *ppv = This; return S_OK; } \
    static ULONG WINAPI I##_x_AddRef(I *This) { return ++((obj_t *)This)->ref; } \
    static ULONG WINAPI I##_x_Release(I *This) { obj_t *o = (obj_t *)This; uint32_t r = --o->ref; if (!r) dev_free(o); return r; }

static long g_n_blt, g_n_blt_bad, g_n_lock, g_n_getdc, g_n_surf, g_n_unsup;
static void unsupported(const char *what, uint32_t a, uint32_t b)
{
    if (g_n_unsup++ < 20) rh_log("dev: UNSUPPORTED %s %08x %08x (frame %u) — the census did not see this: extend rh_dev.c\n", what, a, b, g_frame);
}

/* ---- surfaces ---- */
static void surf_desc(const surf_t *s, DDSURFACEDESC2 *d, int with_bits)
{
    DWORD sz = d->dwSize;
    memset(d, 0, sizeof *d);
    d->dwSize = sz ? sz : sizeof *d;
    d->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | (s->ckflags & DDCKEY_SRCBLT ? DDSD_CKSRCBLT : 0);
    d->dwHeight = s->h; d->dwWidth = s->w; d->lPitch = (LONG)s->pitch;
    d->ddsCaps.dwCaps = s->caps;
    d->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT); d->ddpfPixelFormat.dwFlags = DDPF_RGB; d->ddpfPixelFormat.dwRGBBitCount = s->bpp;
    d->ddpfPixelFormat.dwRBitMask = s->rmask; d->ddpfPixelFormat.dwGBitMask = s->gmask; d->ddpfPixelFormat.dwBBitMask = s->bmask;
    if (s->ckflags & DDCKEY_SRCBLT) { d->ddckCKSrcBlt.dwColorSpaceLowValue = s->cklo; d->ddckCKSrcBlt.dwColorSpaceHighValue = s->ckhi; }
    if (with_bits) d->lpSurface = s->bits;
}
static void hostdc_drop(surf_t *s);
static void surf_destroy(surf_t *s)
{
    hostdc_drop(s);
    if (s->clip && --s->clip->o.ref == 0) dev_free(s->clip);
    if (s->slot >= 0) slot_free(s->slot); else dev_free(s->bits);
    dev_free(s);
}
static HRESULT WINAPI IDirectDrawSurface7_x_QueryInterface(IDirectDrawSurface7 *This, REFIID riid, void **ppv)
{ (void)riid; if (!ppv) return E_POINTER; ((obj_t *)This)->ref++; *ppv = This; return S_OK; }
static ULONG WINAPI IDirectDrawSurface7_x_AddRef(IDirectDrawSurface7 *This) { return ++((obj_t *)This)->ref; }
static ULONG WINAPI IDirectDrawSurface7_x_Release(IDirectDrawSurface7 *This)
{
    surf_t *s = (surf_t *)This; uint32_t r = --s->o.ref;
    if (!r) surf_destroy(s);
    return r;
}
static void blit(surf_t *d, RECT D, surf_t *s, RECT S, DWORD fl);
static HRESULT WINAPI IDirectDrawSurface7_x_Blt(IDirectDrawSurface7 *This, LPRECT dr_, LPDIRECTDRAWSURFACE7 src_, LPRECT sr_, DWORD fl, LPDDBLTFX fx)
{
    surf_t *d = (surf_t *)This, *s = (surf_t *)src_;
    (void)fx;
    g_n_blt++;
    if (!s || (fl & ~(DWORD)(DDBLT_WAIT | DDBLT_KEYSRC))) { unsupported("Blt flags/no-source", fl, (uint32_t)(uintptr_t)s); return DDERR_UNSUPPORTED; }
    RECT D = dr_ ? *dr_ : (RECT){ 0, 0, (LONG)d->w, (LONG)d->h }, S = sr_ ? *sr_ : (RECT){ 0, 0, (LONG)s->w, (LONG)s->h };
    if (D.right <= D.left || D.bottom <= D.top || S.right <= S.left || S.bottom <= S.top) { g_n_blt_bad++; return DDERR_INVALIDRECT; }
    if (S.left < 0 || S.top < 0 || S.right > (LONG)s->w || S.bottom > (LONG)s->h) { g_n_blt_bad++; return DDERR_INVALIDRECT; }
    if ((fl & DDBLT_KEYSRC) && !(s->ckflags & DDCKEY_SRCBLT)) { unsupported("Blt KEYSRC without a key", 0, 0); return DDERR_NOCOLORKEY; }
    if (D.right - D.left != S.right - S.left || D.bottom - D.top != S.bottom - S.top) { unsupported("Blt stretch", (uint32_t)(D.right - D.left), (uint32_t)(S.right - S.left)); return DDERR_UNSUPPORTED; }
    if (!d->clip) {
        if (D.left < 0 || D.top < 0 || D.right > (LONG)d->w || D.bottom > (LONG)d->h) { g_n_blt_bad++; return DDERR_INVALIDRECT; }
        blit(d, D, s, S, fl);
        return DD_OK;
    }
    /* a clipper: the dest rect cut by each clip rect (no list = the surface), the source moved with it */
    RECT whole = { 0, 0, (LONG)d->w, (LONG)d->h };
    uint32_t n = d->clip->nrect ? d->clip->nrect : 1;
    for (uint32_t i = 0; i < n; i++) {
        RECT c = d->clip->nrect ? d->clip->r[i] : whole, P = D, Q = S;
        if (c.left < 0) c.left = 0;
        if (c.top < 0) c.top = 0;
        if (c.right > (LONG)d->w) c.right = (LONG)d->w;
        if (c.bottom > (LONG)d->h) c.bottom = (LONG)d->h;
        if (P.left < c.left) { Q.left += c.left - P.left; P.left = c.left; }
        if (P.top < c.top) { Q.top += c.top - P.top; P.top = c.top; }
        if (P.right > c.right) P.right = c.right;
        if (P.bottom > c.bottom) P.bottom = c.bottom;
        if (P.right > P.left && P.bottom > P.top) blit(d, P, s, Q, fl);
    }
    return DD_OK;
}
static void blit(surf_t *d, RECT D, surf_t *s, RECT S, DWORD fl)
{
    int w = D.right - D.left, h = D.bottom - D.top, B = (int)s->bpp / 8;
    const uint8_t *sp = s->bits + (size_t)S.top * s->pitch + (size_t)S.left * B;
    uint8_t *dp = d->bits + (size_t)D.top * d->pitch + (size_t)D.left * B;
    if (!(fl & DDBLT_KEYSRC)) {
        for (int y = 0; y < h; y++, sp += s->pitch, dp += d->pitch) memmove(dp, sp, (size_t)w * B);
    } else if (B == 2) {
        uint16_t lo = (uint16_t)s->cklo, hi = (uint16_t)s->ckhi;
        for (int y = 0; y < h; y++, sp += s->pitch, dp += d->pitch) {
            const uint16_t *a = (const uint16_t *)sp; uint16_t *b = (uint16_t *)dp;
            if (lo == hi) { for (int x = 0; x < w; x++) if (a[x] != lo) b[x] = a[x]; }
            else for (int x = 0; x < w; x++) if (a[x] < lo || a[x] > hi) b[x] = a[x];
        }
    } else {
        uint32_t lo = s->cklo, hi = s->ckhi, m = B == 4 ? 0xffffffffu : 0xffffffu;
        for (int y = 0; y < h; y++, sp += s->pitch, dp += d->pitch)
            for (int x = 0; x < w; x++) {
                uint32_t v = 0; memcpy(&v, sp + x * B, (size_t)B); v &= m;
                if (v < lo || v > hi) memcpy(dp + x * B, &v, (size_t)B);
            }
    }
}
static HRESULT WINAPI IDirectDrawSurface7_x_Lock(IDirectDrawSurface7 *This, LPRECT rc, LPDDSURFACEDESC2 d, DWORD fl, HANDLE e)
{
    surf_t *s = (surf_t *)This; (void)fl; (void)e;
    g_n_lock++;
    if (!d) return DDERR_INVALIDPARAMS;
    surf_desc(s, d, 1);
    if (rc) d->lpSurface = s->bits + (size_t)rc->top * s->pitch + (size_t)rc->left * (s->bpp / 8);
    return DD_OK;
}
static HRESULT WINAPI IDirectDrawSurface7_x_Unlock(IDirectDrawSurface7 *This, LPRECT rc) { (void)This; (void)rc; return DD_OK; }
static HRESULT WINAPI IDirectDrawSurface7_x_GetSurfaceDesc(IDirectDrawSurface7 *This, LPDDSURFACEDESC2 d) { if (!d) return DDERR_INVALIDPARAMS; surf_desc((surf_t *)This, d, 0); return DD_OK; }
static HRESULT WINAPI IDirectDrawSurface7_x_GetPixelFormat(IDirectDrawSurface7 *This, LPDDPIXELFORMAT pf)
{ DDSURFACEDESC2 d = { .dwSize = sizeof d }; surf_desc((surf_t *)This, &d, 0); *pf = d.ddpfPixelFormat; return DD_OK; }
static HRESULT WINAPI IDirectDrawSurface7_x_GetCaps(IDirectDrawSurface7 *This, LPDDSCAPS2 c) { memset(c, 0, sizeof *c); c->dwCaps = ((surf_t *)This)->caps; return DD_OK; }
static HRESULT WINAPI IDirectDrawSurface7_x_SetColorKey(IDirectDrawSurface7 *This, DWORD fl, LPDDCOLORKEY k)
{
    surf_t *s = (surf_t *)This;
    if (fl != DDCKEY_SRCBLT) { unsupported("SetColorKey flags", fl, 0); return DDERR_UNSUPPORTED; }
    if (k) { s->ckflags |= DDCKEY_SRCBLT; s->cklo = k->dwColorSpaceLowValue; s->ckhi = k->dwColorSpaceHighValue; }
    else s->ckflags &= ~(uint32_t)DDCKEY_SRCBLT;
    return DD_OK;
}
static HRESULT WINAPI IDirectDrawSurface7_x_GetColorKey(IDirectDrawSurface7 *This, DWORD fl, LPDDCOLORKEY k)
{
    surf_t *s = (surf_t *)This;
    if (fl != DDCKEY_SRCBLT || !(s->ckflags & DDCKEY_SRCBLT)) return DDERR_NOCOLORKEY;
    k->dwColorSpaceLowValue = s->cklo; k->dwColorSpaceHighValue = s->ckhi;
    return DD_OK;
}
static HRESULT WINAPI IDirectDrawSurface7_x_SetClipper(IDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER c)
{
    surf_t *s = (surf_t *)This;
    if (c) ((clip_t *)c)->o.ref++;
    if (s->clip && --s->clip->o.ref == 0) dev_free(s->clip);
    s->clip = (clip_t *)c;
    return DD_OK;
}
static HRESULT WINAPI IDirectDrawSurface7_x_GetClipper(IDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER *c)
{
    surf_t *s = (surf_t *)This;
    if (!s->clip) { *c = NULL; return DDERR_NOCLIPPERATTACHED; }
    s->clip->o.ref++; *c = (LPDIRECTDRAWCLIPPER)s->clip; return DD_OK;
}
static HRESULT WINAPI IDirectDrawSurface7_x_IsLost(IDirectDrawSurface7 *This) { (void)This; return DD_OK; }
static HRESULT WINAPI IDirectDrawSurface7_x_Restore(IDirectDrawSurface7 *This) { (void)This; return DD_OK; }
/* GetDC: a section slot's DC (pixels shared, nothing copied) or, for a surface on the DEV heap, a host DIB the
 * pixels are copied into and back out of (the census saw only 640x480 surfaces here, which are all slots). */
typedef struct { surf_t *s; HDC dc; HBITMAP bmp, old; void *bits; uint32_t w, h; int saved; } hostdc_t;
static hostdc_t g_hdc[8];
static HRESULT WINAPI IDirectDrawSurface7_x_GetDC(IDirectDrawSurface7 *This, HDC *out)
{
    surf_t *s = (surf_t *)This;
    g_n_getdc++;
    if (s->dc_out) return DDERR_DCALREADYCREATED;
    HDC dc;
    if (s->slot >= 0) {
        rh_dev_gdi_wrote(SLOT_VA(s->slot), SLOT_VA(s->slot) + s->size);
        dc = slot_dc(s->slot, s->w, s->h);
        RestoreDC(dc, g_dib[s->slot].saved); g_dib[s->slot].saved = SaveDC(dc);   /* real DirectDraw: a default-state DC every time */
    } else {
        hostdc_t *h = NULL;
        for (int i = 0; i < 8 && !h; i++) if (g_hdc[i].s == s && g_hdc[i].w == s->w && g_hdc[i].h == s->h) h = &g_hdc[i];
        for (int i = 0; i < 8 && !h; i++) if (!g_hdc[i].s) h = &g_hdc[i];
        for (int i = 0; i < 8 && !h; i++) if (!g_hdc[i].s->dc_out) { g_hdc[i].s = NULL; h = &g_hdc[i]; }   /* evict an idle one */
        if (!h) { unsupported("GetDC: more than 8 heap-surface DCs out at once", s->w, s->h); return DDERR_CANTCREATEDC; }
        if (h->s != s || !h->dc) {
            if (h->dc) { SelectObject(h->dc, h->old); DeleteObject(h->bmp); DeleteDC(h->dc); }
            struct { BITMAPINFOHEADER hd; DWORD m[3]; } bi = { { sizeof(BITMAPINFOHEADER), (LONG)s->w, -(LONG)s->h, 1, (WORD)s->bpp, s->bpp == 16 ? BI_BITFIELDS : BI_RGB, 0, 0, 0, 0, 0 }, { s->rmask, s->gmask, s->bmask } };
            h->bmp = CreateDIBSection(NULL, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &h->bits, NULL, 0);
            h->dc = CreateCompatibleDC(NULL); h->old = SelectObject(h->dc, h->bmp); h->s = s; h->w = s->w; h->h = s->h; h->saved = SaveDC(h->dc);
        }
        uint32_t rb = s->w * (s->bpp / 8), dp = (rb + 3) & ~3u;
        for (uint32_t y = 0; y < s->h; y++) memcpy((uint8_t *)h->bits + y * dp, s->bits + y * s->pitch, rb);
        RestoreDC(h->dc, h->saved); h->saved = SaveDC(h->dc);
        dc = h->dc;
    }
    s->dc_out = 1;
    *out = dc;
    return DD_OK;
}
static void hostdc_drop(surf_t *s)
{
    for (int i = 0; i < 8; i++) if (g_hdc[i].s == s) g_hdc[i].s = NULL;   /* its DC / DIB are reused for the next surface */
}
static HRESULT WINAPI IDirectDrawSurface7_x_ReleaseDC(IDirectDrawSurface7 *This, HDC dc)
{
    surf_t *s = (surf_t *)This; (void)dc;
    if (!s->dc_out) return DDERR_NODC;
    s->dc_out = 0;
    GdiFlush();
    if (s->slot >= 0) rh_dev_gdi_wrote(SLOT_VA(s->slot), SLOT_VA(s->slot) + s->size);
    else for (int i = 0; i < 8; i++) if (g_hdc[i].s == s) {
        uint32_t rb = s->w * (s->bpp / 8), dp = (rb + 3) & ~3u;
        for (uint32_t y = 0; y < s->h; y++) memcpy(s->bits + y * s->pitch, (uint8_t *)g_hdc[i].bits + y * dp, rb);
    }
    return DD_OK;
}

/* ---- clipper ---- */
SIMPLE_UNK(IDirectDrawClipper)
static HRESULT WINAPI IDirectDrawClipper_x_SetHWnd(IDirectDrawClipper *This, DWORD fl, HWND w) { (void)fl; ((clip_t *)This)->has_hwnd = w != NULL; return DD_OK; }
static HRESULT WINAPI IDirectDrawClipper_x_SetClipList(IDirectDrawClipper *This, LPRGNDATA rd, DWORD fl)
{
    clip_t *c = (clip_t *)This; (void)fl;
    if (!rd) { c->nrect = 0; return DD_OK; }
    if (rd->rdh.nCount > CLIP_RECTS) { unsupported("SetClipList rects", rd->rdh.nCount, 0); return DDERR_UNSUPPORTED; }
    c->nrect = rd->rdh.nCount;
    memcpy(c->r, rd->Buffer, c->nrect * sizeof(RECT));
    if (O.log_ddraw) rh_log("dev: SetClipList %u rect(s), first %ld,%ld,%ld,%ld (f%u)\n", c->nrect, c->r[0].left, c->r[0].top, c->r[0].right, c->r[0].bottom, g_frame);
    return DD_OK;
}
static HRESULT WINAPI IDirectDrawClipper_x_GetHWnd(IDirectDrawClipper *This, HWND *w) { *w = ((clip_t *)This)->has_hwnd ? rh_game_hwnd() : NULL; return DD_OK; }

/* ---- the DirectDraw object ---- */
static HRESULT WINAPI IDirectDraw7_x_QueryInterface(IDirectDraw7 *This, REFIID riid, void **ppv) { (void)riid; if (!ppv) return E_POINTER; ((obj_t *)This)->ref++; *ppv = This; return S_OK; }
static ULONG WINAPI IDirectDraw7_x_AddRef(IDirectDraw7 *This) { return ++((obj_t *)This)->ref; }
static ULONG WINAPI IDirectDraw7_x_Release(IDirectDraw7 *This) { obj_t *o = (obj_t *)This; return o->ref ? --o->ref : 0; }
static HRESULT WINAPI IDirectDraw7_x_SetCooperativeLevel(IDirectDraw7 *This, HWND w, DWORD fl)
{ (void)This; (void)w; if (fl & DDSCL_FULLSCREEN) unsupported("SetCooperativeLevel fullscreen", fl, 0); return DD_OK; }
static HRESULT WINAPI IDirectDraw7_x_CreateClipper(IDirectDraw7 *This, DWORD fl, LPDIRECTDRAWCLIPPER *out, IUnknown *outer)
{ (void)This; (void)fl; (void)outer; *out = obj_new(vt_IDirectDrawClipper, sizeof(clip_t)); return DD_OK; }
static HRESULT WINAPI IDirectDraw7_x_CreateSurface(IDirectDraw7 *This, LPDDSURFACEDESC2 d, LPDIRECTDRAWSURFACE7 *out, IUnknown *outer)
{
    (void)This; (void)outer;
    if (!d || !out) return DDERR_INVALIDPARAMS;
    DWORD need = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    if ((d->dwFlags & need) != need || (d->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) || !d->dwWidth || !d->dwHeight) {
        unsupported("CreateSurface desc", d->dwFlags, d->ddsCaps.dwCaps);
        return DDERR_INVALIDPARAMS;
    }
    uint32_t bpp = 16, rm = 0xf800, gm = 0x07e0, bm = 0x001f;   /* no format given: the census never saw one; 565 */
    if (d->dwFlags & DDSD_PIXELFORMAT) {
        bpp = d->ddpfPixelFormat.dwRGBBitCount; rm = d->ddpfPixelFormat.dwRBitMask; gm = d->ddpfPixelFormat.dwGBitMask; bm = d->ddpfPixelFormat.dwBBitMask;
        if (bpp != 16 && bpp != 24 && bpp != 32) { unsupported("CreateSurface pixel format", bpp, rm); return DDERR_INVALIDPIXELFORMAT; }
    }
    surf_t *s = obj_new(vt_IDirectDrawSurface7, sizeof *s);
    s->bpp = bpp; s->rmask = rm; s->gmask = gm; s->bmask = bm;
    s->w = d->dwWidth; s->h = d->dwHeight; s->pitch = (s->w * (bpp / 8) + 7) & ~7u; s->size = s->pitch * s->h;
    s->caps = d->ddsCaps.dwCaps;
    s->slot = -1;
    /* a slot when GDI could draw into it without a copy: the DIB's DWORD-aligned pitch equals DirectDraw's */
    if (bpp == 16 && s->size >= 0x20000u && s->size <= RH_DIB_SLOT && ((s->w * 2 + 3) & ~3u) == s->pitch) {
        int i = slot_alloc();
        if (i >= 0) {
            slot_map(i); s->slot = i; s->bits = (uint8_t *)(uintptr_t)SLOT_VA(i);
            memset(s->bits, 0, s->size); rh_dev_gdi_wrote(SLOT_VA(i), SLOT_VA(i) + s->size);
        }
    }
    if (s->slot < 0) s->bits = dev_alloc(s->size);
    if (d->dwFlags & DDSD_CKSRCBLT) { s->ckflags = DDCKEY_SRCBLT; s->cklo = d->ddckCKSrcBlt.dwColorSpaceLowValue; s->ckhi = d->ddckCKSrcBlt.dwColorSpaceHighValue; }
    g_n_surf++;
    *out = (LPDIRECTDRAWSURFACE7)s;
    return DD_OK;
}
static HRESULT WINAPI IDirectDraw7_x_GetDisplayMode(IDirectDraw7 *This, LPDDSURFACEDESC2 d)
{
    (void)This; DWORD sz = d->dwSize; memset(d, 0, sizeof *d); d->dwSize = sz;
    d->dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
    d->dwWidth = RH_REF_DESK_W; d->dwHeight = RH_REF_DESK_H; d->lPitch = RH_REF_DESK_W * 4; d->dwRefreshRate = 60;
    d->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT); d->ddpfPixelFormat.dwFlags = DDPF_RGB; d->ddpfPixelFormat.dwRGBBitCount = 32;
    d->ddpfPixelFormat.dwRBitMask = 0xff0000; d->ddpfPixelFormat.dwGBitMask = 0xff00; d->ddpfPixelFormat.dwBBitMask = 0xff;
    return DD_OK;
}
static HRESULT WINAPI IDirectDraw7_x_RestoreAllSurfaces(IDirectDraw7 *This) { (void)This; return DD_OK; }
static HRESULT WINAPI IDirectDraw7_x_TestCooperativeLevel(IDirectDraw7 *This) { (void)This; return DD_OK; }
static HRESULT WINAPI my_DirectDrawCreateEx(GUID *g, void **out, REFIID iid, IUnknown *outer)
{
    (void)g; (void)iid; (void)outer;
    if (!g_dd) {
        g_dd = obj_new(vt_IDirectDraw7, sizeof(obj_t));
        g_dd->ref = 0x7ffffff;   /* never freed (retail Releases it at exit) */
    }
    g_dd->ref++;
    *out = g_dd;
    return DD_OK;
}

/* ================= DirectShow: the BGM graph ================= */
static const GUID k_IUnknown      = { 0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID k_IGraphBuilder = { 0x56a868a9, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID k_IFilterGraph  = { 0x56a8689f, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID k_IMediaControl = { 0x56a868b1, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID k_IBasicAudio   = { 0x56a868b3, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
static const GUID k_IMediaSeeking = { 0x36b73880, 0xc2c8, 0x11cf, { 0x8b, 0x46, 0x00, 0x80, 0x5f, 0x6c, 0xef, 0x60 } };
static const GUID k_FilterGraph   = { 0xe436ebb3, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
typedef struct graph graph_t;
typedef struct { void **vt; graph_t *g; } tear_t;
struct graph {
    tear_t gb, mc, ms, ba;          /* gb first: the graph's identity */
    uint32_t ref, state, rendered;  /* state 0 stop 1 pause 2 run */
    int64_t pos0, stop, dur;
    uint32_t t0;
    int32_t volume;
};
#define G_(This) (((tear_t *)(This))->g)
static HRESULT graph_qi(graph_t *g, REFIID r, void **ppv)
{
    if (!ppv) return E_POINTER;
    void *o = !memcmp(r, &k_IUnknown, 16) || !memcmp(r, &k_IGraphBuilder, 16) || !memcmp(r, &k_IFilterGraph, 16) ? (void *)&g->gb
            : !memcmp(r, &k_IMediaControl, 16) ? (void *)&g->mc : !memcmp(r, &k_IMediaSeeking, 16) ? (void *)&g->ms
            : !memcmp(r, &k_IBasicAudio, 16) ? (void *)&g->ba : NULL;
    if (!o) { *ppv = NULL; unsupported("FilterGraph QueryInterface", ((const uint32_t *)r)[0], 0); return E_NOINTERFACE; }
    g->ref++; *ppv = o; return S_OK;
}
static ULONG graph_release(graph_t *g) { uint32_t r = --g->ref; if (!r) dev_free(g); return r; }
#define TEAR_UNK(I) \
    static HRESULT WINAPI I##_x_QueryInterface(I *This, REFIID r, void **ppv) { return graph_qi(G_(This), r, ppv); } \
    static ULONG WINAPI I##_x_AddRef(I *This) { return ++G_(This)->ref; } \
    static ULONG WINAPI I##_x_Release(I *This) { return graph_release(G_(This)); }
TEAR_UNK(IGraphBuilder)
TEAR_UNK(IMediaControl)
TEAR_UNK(IMediaSeeking)
TEAR_UNK(IBasicAudio)
static int64_t g_pos(graph_t *g)
{
    int64_t p = g->pos0;
    if (g->state == 2) p += (int64_t)(rh_vnow() - g->t0) * 10000;
    int64_t end = g->stop >= 0 ? g->stop : g->rendered ? g->dur : -1;
    if (end >= 0 && p > end) p = end;
    return p;
}
/* the ASF File Properties Object: duration = PlayDuration - Preroll(ms)*10^4 (100 ns), as quartz reports it */
static int asf_duration(const char *path, int64_t *dur)
{
    static const uint8_t FP[16] = { 0xa1, 0xdc, 0xab, 0x8c, 0x47, 0xa9, 0xcf, 0x11, 0x8e, 0xe4, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 };
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return -1;
    static uint8_t b[65536]; DWORD n = 0;
    ReadFile(f, b, sizeof b, &n, NULL); CloseHandle(f);
    for (DWORD i = 0; i + 104 <= n; i++) if (!memcmp(b + i, FP, 16)) {
        uint64_t play, pre; memcpy(&play, b + i + 64, 8); memcpy(&pre, b + i + 80, 8);
        *dur = (int64_t)play - (int64_t)pre * 10000;
        return 0;
    }
    return -1;
}
const char *rh_files_resolve(const char *path, int want_write, char *out, size_t n);
static HRESULT WINAPI IGraphBuilder_x_RenderFile(IGraphBuilder *This, LPCWSTR file, LPCWSTR list)
{
    (void)list;
    graph_t *g = G_(This);
    char a[MAX_PATH], buf[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, file, -1, a, sizeof a, NULL, NULL);
    const char *q = rh_files_resolve(a, 0, buf, sizeof buf);
    int64_t d;
    if (asf_duration(q, &d)) { rh_log("dev: bgm RenderFile %s: no ASF header (f%u)\n", q, g_frame); return (HRESULT)0x80040218L; }
    g->state = 0; g->pos0 = 0; g->stop = -1; g->dur = d; g->rendered = 1; g->t0 = 0;
    if (O.log_media) rh_log("dev: bgm RenderFile %s duration %lld (f%u)\n", q, (long long)d, g_frame);
    return S_OK;
}
static HRESULT WINAPI IMediaControl_x_Run(IMediaControl *This) { graph_t *g = G_(This); if (g->state != 2) { g->t0 = rh_vnow(); g->state = 2; } return S_OK; }
static HRESULT WINAPI IMediaControl_x_Pause(IMediaControl *This) { graph_t *g = G_(This); g->pos0 = g_pos(g); g->state = 1; return S_OK; }
static HRESULT WINAPI IMediaControl_x_Stop(IMediaControl *This) { graph_t *g = G_(This); g->pos0 = g_pos(g); g->state = 0; return S_OK; }
static HRESULT WINAPI IMediaControl_x_GetState(IMediaControl *This, LONG ms, OAFilterState *st) { (void)ms; *st = (OAFilterState)G_(This)->state; return S_OK; }
static HRESULT WINAPI IMediaSeeking_x_GetDuration(IMediaSeeking *This, LONGLONG *d) { graph_t *g = G_(This); if (!g->rendered) return E_FAIL; *d = g->dur; return S_OK; }
static HRESULT WINAPI IMediaSeeking_x_SetTimeFormat(IMediaSeeking *This, const GUID *f) { (void)This; (void)f; return S_OK; }
static HRESULT WINAPI IMediaSeeking_x_SetPositions(IMediaSeeking *This, LONGLONG *cur, DWORD cf, LONGLONG *stop, DWORD sf)
{
    graph_t *g = G_(This);
    if (cur && (cf & 3)) { int64_t p = (cf & 3) == 1 ? *cur : g_pos(g) + *cur; g->pos0 = p < 0 ? 0 : p; g->t0 = rh_vnow(); }
    if (stop && (sf & 3)) g->stop = (sf & 3) == 1 ? *stop : g_pos(g) + *stop;
    return S_OK;
}
static HRESULT WINAPI IMediaSeeking_x_GetPositions(IMediaSeeking *This, LONGLONG *cur, LONGLONG *stop)
{
    graph_t *g = G_(This);
    if (cur) *cur = g_pos(g);
    if (stop) *stop = g->stop >= 0 ? g->stop : g->dur;
    return S_OK;
}
static HRESULT WINAPI IMediaSeeking_x_GetCurrentPosition(IMediaSeeking *This, LONGLONG *cur) { *cur = g_pos(G_(This)); return S_OK; }
static HRESULT WINAPI IBasicAudio_x_put_Volume(IBasicAudio *This, LONG v) { G_(This)->volume = v; return S_OK; }
static HRESULT WINAPI IBasicAudio_x_get_Volume(IBasicAudio *This, LONG *v) { *v = G_(This)->volume; return S_OK; }

typedef HRESULT (WINAPI *cci_t)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID *);
static cci_t r_CoCreateInstance;
static HRESULT WINAPI my_CoCreateInstance(REFCLSID cls, LPUNKNOWN outer, DWORD ctx, REFIID iid, LPVOID *out)
{
    if (cls && !memcmp(cls, &k_FilterGraph, 16)) {
        graph_t *g = dev_alloc(sizeof *g);
        g->gb.vt = vt_IGraphBuilder; g->mc.vt = vt_IMediaControl; g->ms.vt = vt_IMediaSeeking; g->ba.vt = vt_IBasicAudio;
        g->gb.g = g->mc.g = g->ms.g = g->ba.g = g;
        g->stop = -1;
        HRESULT r = graph_qi(g, iid, out);
        if (r) dev_free(g);
        return r;
    }
    unsupported("CoCreateInstance clsid", cls ? ((const uint32_t *)cls)[0] : 0, 0);
    return r_CoCreateInstance(cls, outer, ctx, iid, out);
}

/* ================= report / install ================= */
#include "rh_dev_gen.h"
static void dev_note(int i) { g_note_calls[i]++; }
void rh_dev_report(void)
{
    if (!O.headless) return;
    char b[2048]; size_t o = 0; int n = 0;
    for (int i = 0; i < DEV_NOTE_COUNT; i++)
        if (g_note_calls[i]) { n++; if (o < sizeof b - 96) o += snprintf(b + o, sizeof b - o, " %s×%u", g_note_names[i], g_note_calls[i]); }
    rh_log("dev: defaulted (S_OK stub) methods called: %d%s\n", n, n ? b : "");
    rh_log("dev: surfaces=%ld blt=%ld (invalid-rect %ld) lock=%ld getdc=%ld unsupported=%ld; region top=+%uK live=%u blocks %uK; dib slots used-hw=%u\n",
           g_n_surf, g_n_blt, g_n_blt_bad, g_n_lock, g_n_getdc, g_n_unsup, (DH->top - RH_DEV_LO) >> 10, DH->live, DH->bytes >> 10, DH->dib_hw);
}
void rh_dev_install(void)
{
    if (VirtualAlloc((void *)(uintptr_t)RH_DEV_LO, RH_DEV_HI - RH_DEV_LO, MEM_RESERVE, PAGE_READWRITE) != (void *)(uintptr_t)RH_DEV_LO) {
        for (uint32_t a = 0x60000000u; a < 0xfff00000u; ) {
            MEMORY_BASIC_INFORMATION mi;
            if (!VirtualQuery((void *)(uintptr_t)a, &mi, sizeof mi)) { rh_log("vq %08x: fails\n", a); break; }
            rh_log("vq %08x+%08lx state %lx\n", (uint32_t)(uintptr_t)mi.BaseAddress, (unsigned long)mi.RegionSize, mi.State);
            a = (uint32_t)(uintptr_t)mi.BaseAddress + (uint32_t)mi.RegionSize; if (!mi.RegionSize) break;
        }
        rh_fatal("rh_dev: cannot reserve the DEV region %08x..%08x (err %lu)", RH_DEV_LO, RH_DEV_HI, GetLastError());
    }
    g_dev_committed = RH_DEV_LO;
    dev_commit(RH_DEV_LO + 0x1000);
    DH->magic = DEV_MAGIC; DH->top = RH_DEV_LO + 0x1000;
    iat_override("DirectDrawCreateEx", my_DirectDrawCreateEx);
    r_CoCreateInstance = (cci_t)iat_lookup("CoCreateInstance");
    iat_override("CoCreateInstance", my_CoCreateInstance);
    rh_log("dev: headless devices (DirectDraw7 sysmem 565 blitter, DirectShow BGM on the virtual clock) — no display, no audio\n");
}
/* rh_scrub wraps the fake vtables like the real ones' (every device call on the system stack, as on the real device) */
void rh_dev_scrub(void)
{
    static obj_t dd = { vt_IDirectDraw7, 1 }, sf = { vt_IDirectDrawSurface7, 1 }, cl = { vt_IDirectDrawClipper, 1 };
    static tear_t gb = { vt_IGraphBuilder, 0 }, mc = { vt_IMediaControl, 0 }, ms = { vt_IMediaSeeking, 0 }, ba = { vt_IBasicAudio, 0 };
    rh_scrub_wrap_vtbl(&dd, 30, "fake IDirectDraw7");
    rh_scrub_wrap_vtbl(&sf, 49, "fake IDirectDrawSurface7");
    rh_scrub_wrap_vtbl(&cl, 9, "fake IDirectDrawClipper");
    rh_scrub_wrap_vtbl(&gb, 18, "fake IGraphBuilder");
    rh_scrub_wrap_vtbl(&ms, 20, "fake IMediaSeeking");
    rh_scrub_wrap_vtbl(&mc, 16, "fake IMediaControl");
    rh_scrub_wrap_vtbl(&ba, 11, "fake IBasicAudio");
}

/* savestates: the DEV region + every mapped section slot are tracked memory; GDI's writes into a slot go through
 * GDI's own view of the section (no write fault), so a slot is touched (dirty + writable) before its DC is handed out */
void rh_dev_committed_hook(uint32_t lo, uint32_t hi) { rh_snap_committed(lo, hi); }
void rh_dev_gdi_wrote(uint32_t lo, uint32_t hi) { rh_snap_touch((void *)(uintptr_t)lo, hi - lo); }
int rh_dev_slot_ranges(uint32_t (*out)[2], int max)
{
    int n = 0;
    if (!O.headless || !g_dev_committed) return 0;
    for (uint32_t i = 0; i < DH->dib_hw && n < max; i++)
        if (g_dib[i].sec) { out[n][0] = SLOT_VA(i); out[n][1] = SLOT_VA(i) + RH_DIB_SLOT; n++; }   /* one per view: a protect cannot span two */
    return n;
}
/* the device's memory, hashed (DEV + the slots in use): the savestate gate compares it */
uint64_t rh_state_dev_hash(void)
{
    if (!O.headless || !g_dev_committed) return 0;
    uint64_t h = 1469598103934665603ull;
    uint32_t hi = (DH->top + 0xfffu) & ~0xfffu;
    for (const uint32_t *p = (const uint32_t *)(uintptr_t)RH_DEV_LO; p < (const uint32_t *)(uintptr_t)hi; p++) { h ^= *p; h *= 1099511628211ull; }
    for (int i = 0; i < DIB_SLOTS; i++)
        if (DH->dib_used[i / 32] >> (i % 32) & 1)
            for (const uint32_t *p = (const uint32_t *)(uintptr_t)SLOT_VA(i); p < (const uint32_t *)(uintptr_t)(SLOT_VA(i) + RH_DIB_SLOT); p++) { h ^= *p; h *= 1099511628211ull; }
    return h;
}
