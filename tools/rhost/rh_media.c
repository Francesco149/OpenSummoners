/* rh_media.c — sound on the virtual clock.
 *
 * SE (ZDS, 0x5baed0..): DirectSoundCreate (DSOUND ordinal 1) → SetCooperativeLevel →
 *   primary buffer; per clip CreateSoundBuffer + Lock/memcpy/Unlock, then fire-and-forget
 *   Play/Stop/SetVolume/SetPan/SetFrequency (no GetStatus/GetCurrentPosition reads).
 *   Replay: a silent fake whose buffer memory comes from a fixed-address pool (the Lock
 *   pointers are deterministic). --audible/--record: the real DirectSound.
 * BGM (ZDM, 0x5ba860..0x5bae90 + manager 0x5bb8f0..): DirectShow CLSID_FilterGraph,
 *   RenderFile(<temp>.wma) → IMediaControl Run/Pause/Stop, IMediaSeeking SetPositions /
 *   GetPositions / GetDuration, IBasicAudio put_Volume. The manager LOOPS the track off
 *   GetPositions (0x5bb9b0: pos >= loop_end → Stop, SetPositions(loop_start), Run;
 *   pos >= duration → next) — a wall-clock read that mutates game state. Here the REAL
 *   graph is built (exact GetDuration), but Run/Pause/Stop/GetState/SetPositions/
 *   GetPositions/GetCurrentPosition are patched (in quartz's vtables) onto a per-graph
 *   virtual playback: pos = pos0 + (vnow - t0)·10⁴ (100 ns), clamped to the stop position.
 *   Replay never runs the real graph (silent); --audible/--record also forward to it.
 *   RenderFile resolves the path through the file overlay. */
#include "rhost.h"

const char *rh_files_resolve(const char *path, int want_write, char *out, size_t n);
#define S_OK_ 0
#define E_NOTIMPL_ ((HRESULT)0x80004001L)

/* ---------------------------------------------------------------- fixed pool */
/* Below the 32-bit system-DLL zone (0x60000000.. — 2026-10-16: dinput / dxgi / ddraw / d3d9 got new ASLR
 * bases inside the old 0x60000000 pool once their sections were dropped, no reboot needed) and reserved at
 * startup with the heap arena (rh_media_reserve), so a DLL that loads later relocates around it. */
#define POOL_LO 0x10000000u
#define POOL_SZ 0x0e000000u
static uint32_t g_pool_top, g_pool_free[32];
int rh_media_reserve(void)
{
    if (g_pool_top) return 0;
    if (VirtualAlloc((void *)(uintptr_t)POOL_LO, POOL_SZ, MEM_RESERVE, PAGE_READWRITE) != (void *)(uintptr_t)POOL_LO) return -1;
    g_pool_top = POOL_LO;
    return 0;
}
uint32_t rh_media_pool_used(void) { return g_pool_top ? g_pool_top - POOL_LO : 0; }
int rh_media_in_pool(uint32_t v) { return g_pool_top && v >= POOL_LO && v < POOL_LO + POOL_SZ; }
static void *pool_alloc(uint32_t n)
{
    if (!g_pool_top && rh_media_reserve())
        rh_fatal("rh_media: cannot reserve the sound pool at %08x", POOL_LO);
    int c = 12; while ((1u << c) < n + 16) c++;
    uint32_t p = g_pool_free[c];
    if (p) g_pool_free[c] = *(uint32_t *)(uintptr_t)(p + 4);
    else {
        if (g_pool_top + (1u << c) > POOL_LO + POOL_SZ) rh_fatal("rh_media: sound pool exhausted");
        p = g_pool_top; g_pool_top += 1u << c;
        if (!VirtualAlloc((void *)(uintptr_t)p, 1u << c, MEM_COMMIT, PAGE_READWRITE)) rh_fatal("rh_media: commit failed");
    }
    *(uint32_t *)(uintptr_t)p = (uint32_t)c;
    memset((void *)(uintptr_t)(p + 16), 0, n);
    return (void *)(uintptr_t)(p + 16);
}
static void pool_free(void *q)
{
    if (!q) return;
    uint32_t p = (uint32_t)(uintptr_t)q - 16, c = *(uint32_t *)(uintptr_t)p;
    *(uint32_t *)(uintptr_t)(p + 4) = g_pool_free[c]; g_pool_free[c] = p;
}

/* shadow support: the pool is host memory retail keeps pointers into (buffer objects + their
 * play state), outside the arena snapshot — rh_shadow_ex saves / compares / restores it here */
typedef struct { uint32_t top, freel[32]; uint32_t n, cap; uint8_t *bytes; } media_snap_t;
static media_snap_t *g_snap_cache[8]; static int g_nsnap_cache;   /* reused: a shadow saves 2 per call */
void *rh_media_save(void)
{
    media_snap_t *m = g_nsnap_cache ? g_snap_cache[--g_nsnap_cache] : calloc(1, sizeof *m);
    m->top = g_pool_top; memcpy(m->freel, g_pool_free, sizeof m->freel);
    m->n = g_pool_top ? g_pool_top - POOL_LO : 0;
    if (m->n > m->cap) { free(m->bytes); m->cap = m->n + (1u << 20); m->bytes = malloc(m->cap); }
    if (m->n) memcpy(m->bytes, (void *)(uintptr_t)POOL_LO, m->n);
    return m;
}
void rh_media_restore(const void *p)
{
    const media_snap_t *m = p;
    g_pool_top = m->top; memcpy(g_pool_free, m->freel, sizeof g_pool_free);
    if (m->n) memcpy((void *)(uintptr_t)POOL_LO, m->bytes, m->n);
}
long rh_media_diff(const void *p)        /* differing dwords between the live pool and a snapshot */
{
    const media_snap_t *m = p;
    long d = (g_pool_top != m->top) + (memcmp(g_pool_free, m->freel, sizeof g_pool_free) != 0);
    uint32_t n = m->n < (g_pool_top ? g_pool_top - POOL_LO : 0) ? m->n : (g_pool_top ? g_pool_top - POOL_LO : 0);
    const uint32_t *a = (const uint32_t *)m->bytes, *b = (const uint32_t *)(uintptr_t)POOL_LO;
    for (uint32_t i = 0; i < n / 4; i++) d += a[i] != b[i];
    return d;
}
void rh_media_snap_free(void *p)
{
    media_snap_t *m = p;
    if (!m) return;
    if (g_nsnap_cache < 8) g_snap_cache[g_nsnap_cache++] = m;
    else { free(m->bytes); free(m); }
}

/* ---------------------------------------------------------------- fake DirectSound */
typedef struct { void **vtbl; LONG refs; uint8_t *data; uint32_t bytes; int primary; LONG vol, pan; DWORD freq, playing, flags; void *shared; } dsb_t;
typedef struct { void **vtbl; LONG refs; } ds_t;
static long g_ds_buffers, g_ds_plays;
static void **g_dsb_vt;

static HRESULT WINAPI u_QI(ds_t *s, REFIID r, void **o) { (void)r; *o = s; s->refs++; return S_OK_; }
static ULONG WINAPI u_AddRef(ds_t *s) { return (ULONG)++s->refs; }
static ULONG WINAPI dsb_Release(dsb_t *s)
{
    if (--s->refs > 0) return (ULONG)s->refs;
    if (!s->shared && s->data) pool_free(s->data);
    pool_free(s);
    return 0;
}
static HRESULT WINAPI dsb_GetCaps(dsb_t *s, DWORD *caps) { if (caps && caps[0] >= 20) { caps[1] = s->flags; caps[2] = s->bytes; } return S_OK_; }
static HRESULT WINAPI dsb_GetCurrentPosition(dsb_t *s, DWORD *p, DWORD *w) { (void)s; if (p) *p = 0; if (w) *w = 0; return S_OK_; }
static HRESULT WINAPI dsb_GetFormat(dsb_t *s, void *f, DWORD n, DWORD *w) { (void)s; if (f) memset(f, 0, n); if (w) *w = 0; return S_OK_; }
static HRESULT WINAPI dsb_GetVolume(dsb_t *s, LONG *v) { *v = s->vol; return S_OK_; }
static HRESULT WINAPI dsb_GetPan(dsb_t *s, LONG *v) { *v = s->pan; return S_OK_; }
static HRESULT WINAPI dsb_GetFrequency(dsb_t *s, DWORD *v) { *v = s->freq; return S_OK_; }
static HRESULT WINAPI dsb_GetStatus(dsb_t *s, DWORD *st) { *st = 0; (void)s; return S_OK_; }   /* never "playing": SE are instant */
static HRESULT WINAPI dsb_Initialize(dsb_t *s, void *a, void *b) { (void)s; (void)a; (void)b; return S_OK_; }
static HRESULT WINAPI dsb_Lock(dsb_t *s, DWORD off, DWORD n, void **p1, DWORD *n1, void **p2, DWORD *n2, DWORD fl)
{
    if (fl & 2 /* DSBLOCK_ENTIREBUFFER */) { off = 0; n = s->bytes; }
    if (!s->bytes) { *p1 = NULL; *n1 = 0; if (p2) *p2 = NULL; if (n2) *n2 = 0; return S_OK_; }
    off %= s->bytes; if (n > s->bytes) n = s->bytes;
    uint32_t first = s->bytes - off < n ? s->bytes - off : n;
    *p1 = s->data + off; *n1 = first;
    if (p2) *p2 = n > first ? s->data : NULL;
    if (n2) *n2 = n - first;
    return S_OK_;
}
static HRESULT WINAPI dsb_Play(dsb_t *s, DWORD r1, DWORD pri, DWORD fl) { (void)r1; (void)pri; s->playing = 1 | (fl & 1) << 2; g_ds_plays++; return S_OK_; }
static HRESULT WINAPI dsb_SetCurrentPosition(dsb_t *s, DWORD p) { (void)s; (void)p; return S_OK_; }
static HRESULT WINAPI dsb_SetFormat(dsb_t *s, const void *f) { (void)s; (void)f; return S_OK_; }
static HRESULT WINAPI dsb_SetVolume(dsb_t *s, LONG v) { s->vol = v; return S_OK_; }
static HRESULT WINAPI dsb_SetPan(dsb_t *s, LONG v) { s->pan = v; return S_OK_; }
static HRESULT WINAPI dsb_SetFrequency(dsb_t *s, DWORD v) { s->freq = v; return S_OK_; }
static HRESULT WINAPI dsb_Stop(dsb_t *s) { s->playing = 0; return S_OK_; }
static HRESULT WINAPI dsb_Unlock(dsb_t *s, void *a, DWORD b, void *c, DWORD d) { (void)s; (void)a; (void)b; (void)c; (void)d; return S_OK_; }
static HRESULT WINAPI dsb_Restore(dsb_t *s) { (void)s; return S_OK_; }
static void *g_dsb_vtbl[21] = {
    u_QI, u_AddRef, dsb_Release, dsb_GetCaps, dsb_GetCurrentPosition, dsb_GetFormat, dsb_GetVolume, dsb_GetPan,
    dsb_GetFrequency, dsb_GetStatus, dsb_Initialize, dsb_Lock, dsb_Play, dsb_SetCurrentPosition, dsb_SetFormat,
    dsb_SetVolume, dsb_SetPan, dsb_SetFrequency, dsb_Stop, dsb_Unlock, dsb_Restore,
};
static dsb_t *new_dsb(void)
{
    dsb_t *b = pool_alloc(sizeof *b);
    b->vtbl = g_dsb_vtbl; b->refs = 1; b->freq = 22050;
    g_ds_buffers++;
    return b;
}
static HRESULT WINAPI ds_CreateSoundBuffer(ds_t *s, const DWORD *desc, dsb_t **out, void *outer)
{
    (void)s; (void)outer;
    dsb_t *b = new_dsb();
    b->flags = desc[1]; b->primary = (desc[1] & 1) != 0;
    if (!b->primary && desc[2]) { b->bytes = desc[2]; b->data = pool_alloc(desc[2]); }
    *out = b;
    return S_OK_;
}
static HRESULT WINAPI ds_DuplicateSoundBuffer(ds_t *s, dsb_t *src, dsb_t **out)
{
    (void)s;
    dsb_t *b = new_dsb();
    b->flags = src->flags; b->bytes = src->bytes; b->data = src->data; b->shared = src;
    *out = b; return S_OK_;
}
static HRESULT WINAPI ds_GetCaps(ds_t *s, DWORD *caps) { (void)s; if (caps) memset(caps + 1, 0, caps[0] > 4 ? caps[0] - 4 : 0); return S_OK_; }
static HRESULT WINAPI ds_ok1(ds_t *s, void *a) { (void)s; (void)a; return S_OK_; }
static HRESULT WINAPI ds_ok2(ds_t *s, void *a, void *b) { (void)s; (void)a; (void)b; return S_OK_; }
static HRESULT WINAPI ds_ok0(ds_t *s) { (void)s; return S_OK_; }
static HRESULT WINAPI ds_GetSpeakerConfig(ds_t *s, DWORD *c) { (void)s; *c = 4; return S_OK_; }
static void *g_ds_vtbl[11] = { u_QI, u_AddRef, u_AddRef /* Release: keep alive */, ds_CreateSoundBuffer, ds_GetCaps,
    ds_DuplicateSoundBuffer, ds_ok2, ds_ok0, ds_GetSpeakerConfig, ds_ok1, ds_ok1 };
static ds_t *g_ds;      /* rh_fixed_alloc'd: retail keeps the pointer */
static HRESULT WINAPI my_DirectSoundCreate(const GUID *g, ds_t **out, void *outer)
{
    (void)g; (void)outer;
    static int wrapped;
    if (!wrapped) { wrapped = 1; rh_scrub_wrap_vtbl(g_ds, 11, "fake IDirectSound"); dsb_t tmp = { g_dsb_vtbl, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; rh_scrub_wrap_vtbl(&tmp, 21, "fake IDirectSoundBuffer"); }
    *out = g_ds; g_ds->refs++;
    rh_log("dsound: DirectSoundCreate = silent fake\n");
    return S_OK_;
}

/* ---------------------------------------------------------------- DirectShow BGM */
static const GUID kCLSID_FilterGraph = { 0xE436EBB3, 0x524F, 0x11CE, { 0x9F, 0x53, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70 } };
static const GUID kIID_IUnknown      = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID kIID_IMediaControl = { 0x56A868B1, 0x0AD4, 0x11CE, { 0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70 } };
static const GUID kIID_IMediaSeeking = { 0x36B73880, 0xC2C8, 0x11CF, { 0x8B, 0x46, 0x00, 0x80, 0x5F, 0x6C, 0xEF, 0x60 } };
typedef HRESULT (WINAPI *qi_t)(void *, REFIID, void **);
typedef ULONG (WINAPI *rel_t)(void *);
typedef HRESULT (WINAPI *m0_t)(void *);
typedef HRESULT (WINAPI *rf_t)(void *, LPCWSTR, LPCWSTR);
typedef HRESULT (WINAPI *gs_t)(void *, LONG, LONG *);
typedef HRESULT (WINAPI *sp_t)(void *, LONGLONG *, DWORD, LONGLONG *, DWORD);
typedef HRESULT (WINAPI *gp_t)(void *, LONGLONG *, LONGLONG *);
typedef HRESULT (WINAPI *g1_t)(void *, LONGLONG *);
static rf_t r_RenderFile; static m0_t r_Run, r_Pause, r_Stop; static gs_t r_GetState;
static sp_t r_SetPositions; static gp_t r_GetPositions; static g1_t r_GetCurrentPosition, r_GetDuration;

typedef struct { void *id; int state; LONGLONG pos0, stop, dur; uint32_t t0; int has_dur; } vplay_t;   /* state 0 stop 1 pause 2 run */
static vplay_t g_vp[16]; static int g_nvp;
static long g_bgm_runs, g_bgm_seeks, g_bgm_reads;

static void *identity(void *itf)
{
    void *u = NULL;
    (*(qi_t *)(*(void ***)itf))(itf, &kIID_IUnknown, &u);
    if (u) (*(rel_t *)((*(void ***)u) + 2))(u);
    return u;
}
static vplay_t *vp_of(void *itf)
{
    void *id = identity(itf);
    for (int i = 0; i < g_nvp; i++) if (g_vp[i].id == id) return &g_vp[i];
    if (g_nvp == 16) { memmove(g_vp, g_vp + 1, 15 * sizeof *g_vp); g_nvp = 15; }
    vplay_t *v = &g_vp[g_nvp++]; memset(v, 0, sizeof *v); v->id = id; v->stop = -1;
    return v;
}
static LONGLONG vp_dur(vplay_t *v, void *seek)
{
    if (!v->has_dur && seek) { LONGLONG d = 0; if (r_GetDuration(seek, &d) == 0) { v->dur = d; v->has_dur = 1; } }
    return v->dur;
}
static LONGLONG vp_pos(vplay_t *v)
{
    LONGLONG p = v->pos0;
    if (v->state == 2) p += (LONGLONG)(rh_vnow() - v->t0) * 10000;
    LONGLONG end = v->stop >= 0 ? v->stop : v->has_dur ? v->dur : -1;
    if (end >= 0 && p > end) p = end;
    return p;
}
static HRESULT WINAPI p_Run(void *mc)
{
    vplay_t *v = vp_of(mc);
    if (v->state != 2) { v->t0 = rh_vnow(); v->state = 2; }
    g_bgm_runs++;
    if (O.log_media) rh_log("bgm %p Run pos=%lld (f%u)\n", v->id, (long long)v->pos0, g_frame);
    return O.audible ? r_Run(mc) : S_OK_;
}
static HRESULT WINAPI p_Pause(void *mc)
{
    vplay_t *v = vp_of(mc); v->pos0 = vp_pos(v); v->state = 1;
    return O.audible ? r_Pause(mc) : S_OK_;
}
static HRESULT WINAPI p_Stop(void *mc)
{
    vplay_t *v = vp_of(mc); v->pos0 = vp_pos(v); v->state = 0;
    if (O.log_media) rh_log("bgm %p Stop pos=%lld (f%u)\n", v->id, (long long)v->pos0, g_frame);
    return O.audible ? r_Stop(mc) : S_OK_;
}
static HRESULT WINAPI p_GetState(void *mc, LONG ms, LONG *st) { (void)ms; *st = vp_of(mc)->state; return S_OK_; }
static HRESULT WINAPI p_SetPositions(void *ms, LONGLONG *cur, DWORD cf, LONGLONG *stop, DWORD sf)
{
    vplay_t *v = vp_of(ms);
    vp_dur(v, ms);
    if (cur && (cf & 3)) { LONGLONG p = (cf & 3) == 1 ? *cur : vp_pos(v) + *cur; v->pos0 = p < 0 ? 0 : p; v->t0 = rh_vnow(); }
    if (stop && (sf & 3)) v->stop = (sf & 3) == 1 ? *stop : vp_pos(v) + *stop;
    g_bgm_seeks++;
    if (O.log_media) rh_log("bgm %p SetPositions cur=%lld (f%u)\n", v->id, (long long)v->pos0, g_frame);
    return O.audible ? r_SetPositions(ms, cur, cf, stop, sf) : S_OK_;
}
static HRESULT WINAPI p_GetPositions(void *ms, LONGLONG *cur, LONGLONG *stop)
{
    vplay_t *v = vp_of(ms); vp_dur(v, ms);
    g_bgm_reads++;
    if (cur) *cur = vp_pos(v);
    if (stop) *stop = v->stop >= 0 ? v->stop : v->dur;
    return S_OK_;
}
static HRESULT WINAPI p_GetCurrentPosition(void *ms, LONGLONG *cur) { return p_GetPositions(ms, cur, NULL); }
/* every filter out of a graph (IFilterGraph::EnumFilters 5 -> IEnumFilters::Next 3; RemoveFilter 4) */
static int graph_clear(void *gb)
{
    typedef HRESULT (WINAPI *ef_t)(void *, void **);
    typedef HRESULT (WINAPI *nx_t)(void *, ULONG, void **, ULONG *);
    typedef HRESULT (WINAPI *rf2_t)(void *, void *);
    typedef ULONG (WINAPI *rl_t)(void *);
    void **vt = *(void ***)gb, *en = NULL, *fl[64];
    int n = 0;
    if (((ef_t)vt[5])(gb, &en) < 0 || !en) return -1;
    ULONG got = 0;
    while (n < 64 && ((nx_t)(*(void ***)en)[3])(en, 1, &fl[n], &got) == 0 && got == 1) n++;
    ((rl_t)(*(void ***)en)[2])(en);
    for (int i = 0; i < n; i++) { ((rf2_t)vt[4])(gb, fl[i]); ((rl_t)(*(void ***)fl[i])[2])(fl[i]); }
    return n;
}
static HRESULT WINAPI p_RenderFile(void *gb, LPCWSTR file, LPCWSTR list)
{
    char a[MAX_PATH], buf[MAX_PATH]; WCHAR w[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, file, -1, a, sizeof a, NULL, NULL);
    const char *q = rh_files_resolve(a, 0, buf, sizeof buf);
    MultiByteToWideChar(CP_ACP, 0, q, -1, w, MAX_PATH);
    HRESULT r = r_RenderFile(gb, w, list);
    /* wine's DirectShow (gstreamer) now and then fails a WMA it renders fine on the next try (80040218
     * VFW_E_CANNOT_RENDER under parallel load): retail then logs the error and keeps a different state.  A
     * replay retries — the game sees the outcome every healthy run sees */
    for (int k = 0; r < 0 && k < 8 && !O.audible; k++) {
        int nf = graph_clear(gb);   /* the failed attempt's half-built chain out first: the retry builds what a
                                     * first-time success builds (else the graph — its duration — differs) */
        rh_log("bgm RenderFile %s = %08lx: retry %d (%d filters removed, f%u)\n", q, (unsigned long)r, k + 1, nf, g_frame);
        Sleep(100 * (k + 1));
        r = r_RenderFile(gb, w, list);
    }
    vplay_t *v = vp_of(gb); memset(v, 0, sizeof *v); v->id = identity(gb); v->stop = -1;
    if (O.log_media) rh_log("bgm %p RenderFile %s -> %s = %08lx (f%u)\n", v->id, a, q, (unsigned long)r, g_frame);
    return r;
}

static void *patch_vt(void **vt, int i, void *fn)
{
    void *orig = vt[i];
    if (orig == fn) return NULL;
    DWORD old;
    VirtualProtect(&vt[i], sizeof(void *), PAGE_READWRITE, &old);
    vt[i] = fn;
    VirtualProtect(&vt[i], sizeof(void *), old, &old);
    return orig;
}
typedef HRESULT (WINAPI *cci_t)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID *);
static cci_t r_CoCreateInstance;
static int g_dshow_patched;
static HRESULT WINAPI my_CoCreateInstance(REFCLSID c, LPUNKNOWN outer, DWORD ctx, REFIID iid, LPVOID *out)
{
    HRESULT r = r_CoCreateInstance(c, outer, ctx, iid, out);
    if (r == 0 && c && !memcmp(c, &kCLSID_FilterGraph, sizeof(GUID)) && !g_dshow_patched) {
        void *gb = *out, *mc = NULL, *ms = NULL;
        (*(qi_t *)(*(void ***)gb))(gb, &kIID_IMediaControl, &mc);
        (*(qi_t *)(*(void ***)gb))(gb, &kIID_IMediaSeeking, &ms);
        if (!mc || !ms) rh_fatal("FilterGraph lacks IMediaControl/IMediaSeeking");
        void **vg = *(void ***)gb, **vc = *(void ***)mc, **vs = *(void ***)ms;
        r_RenderFile = (rf_t)patch_vt(vg, 13, (void *)p_RenderFile);
        r_Run = (m0_t)patch_vt(vc, 7, (void *)p_Run);
        r_Pause = (m0_t)patch_vt(vc, 8, (void *)p_Pause);
        r_Stop = (m0_t)patch_vt(vc, 9, (void *)p_Stop);
        r_GetState = (gs_t)patch_vt(vc, 10, (void *)p_GetState);
        r_GetDuration = (g1_t)vs[10];
        r_GetCurrentPosition = (g1_t)patch_vt(vs, 12, (void *)p_GetCurrentPosition);
        r_SetPositions = (sp_t)patch_vt(vs, 14, (void *)p_SetPositions);
        r_GetPositions = (gp_t)patch_vt(vs, 15, (void *)p_GetPositions);
        /* every quartz call (incl. our patched ones, which QI into quartz) through a scrub thunk */
        rh_scrub_wrap_vtbl(gb, 18, "IGraphBuilder");
        rh_scrub_wrap_vtbl(mc, 16, "IMediaControl");
        rh_scrub_wrap_vtbl(ms, 20, "IMediaSeeking");
        void *ba = NULL;
        static const GUID kIID_IBasicAudio = { 0x56A868B3, 0x0AD4, 0x11CE, { 0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70 } };
        (*(qi_t *)(*(void ***)gb))(gb, &kIID_IBasicAudio, &ba);
        if (ba) { rh_scrub_wrap_vtbl(ba, 11, "IBasicAudio"); (*(rel_t *)((*(void ***)ba) + 2))(ba); }
        (*(rel_t *)(vc + 2))(mc); (*(rel_t *)(vs + 2))(ms);
        g_dshow_patched = 1;
        rh_log("dshow: FilterGraph patched (virtual playback clock, %s)\n", O.audible ? "audible" : "silent");
    }
    return r;
}

void rh_media_install(void)
{
    pool_free(pool_alloc(16));     /* reserve the fixed pool NOW, before retail loads its DLLs */
    g_ds = rh_fixed_alloc(sizeof *g_ds); g_ds->vtbl = g_ds_vtbl; g_ds->refs = 1;
    r_CoCreateInstance = (cci_t)iat_lookup("CoCreateInstance");
    if (r_CoCreateInstance) iat_override("CoCreateInstance", my_CoCreateInstance);
    if (!O.audible) {
        if (!iat_override_ord("DSOUND.dll", 1, (void *)my_DirectSoundCreate) && !iat_try("DirectSoundCreate", my_DirectSoundCreate))
            rh_log("dsound: no DirectSoundCreate import found (SE stay real)\n");
    }
    (void)g_dsb_vt;
}
void rh_dinput_report(void);
void rh_media_report(void)
{
    rh_dinput_report();
    rh_log("media: ds_buffers=%ld ds_plays=%ld bgm_runs=%ld bgm_seeks=%ld bgm_pos_reads=%ld pool=+%uK\n",
           g_ds_buffers, g_ds_plays, g_bgm_runs, g_bgm_seeks, g_bgm_reads, g_pool_top ? (g_pool_top - POOL_LO) >> 10 : 0);
}
