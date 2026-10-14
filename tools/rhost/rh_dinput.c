/* rh_dinput.c — a fake DirectInput7 for retail sotes: the keyboard is the input script.
 *
 * Retail (ZDI, 0x5b9cf0..0x5ba7e0): DirectInputCreateEx(hinst, 0x700, IID_IDirectInput7A) →
 * EnumDevices(DIDEVTYPE_JOYSTICK, ATTACHEDONLY) into 2 joystick slots → CreateDeviceEx(
 * GUID_SysKeyboard) → SetDataFormat(c_dfDIKeyboard) / SetCooperativeLevel(hwnd, 6) /
 * SetProperty(BUFFERSIZE 32) / Acquire. Per update the scenes poll GetDeviceState(256)
 * (0x5ba4c0 → leaf 0x5ba520 = state[sc] & 0x80, the held axis) and GetDeviceData(16-byte
 * records, 0x5ba2d0 → the input ring producer 0x46a880). Joysticks: GetDeviceState(80).
 *
 * Here nothing touches real DirectInput: the keyboard reports the trace's held LEVEL for the
 * current flip-frame (GetDeviceState), and the level changes at each frame boundary are
 * queued as buffered edge events (GetDeviceData: dwOfs = DIK, dwData 0x80/0, dwTimeStamp =
 * virtual GetTickCount, dwSequence monotonic). No joystick is ever enumerated.
 * --record: the level comes from the real keyboard (GetAsyncKeyState while the game window
 * is foreground), sampled once per frame boundary and written to the trace — the game is fed
 * through the same fake, so a recording replays bit-exactly. */
#include "rhost.h"

#define S_OK_ 0
#define DIERR_NOTFOUND_  ((HRESULT)0x80070002L)
#define E_NOINTERFACE_   ((HRESULT)0x80004002L)
#define E_NOTIMPL_       ((HRESULT)0x80004001L)

static const GUID kGUID_SysKeyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

typedef struct { void **vtbl; LONG refs; } fake_t;

/* ---- keyboard state ---- */
static uint8_t g_level[256];                   /* current frame's held level (0x80 = down) */
typedef struct { DWORD ofs, data, ts, seq; } dod_t;   /* DIDEVICEOBJECTDATA (16 bytes, DX7) */
static dod_t g_q[512]; static int g_qh, g_qn;
static DWORD g_seq;
static long g_gds_calls, g_gdd_calls, g_events;
static int g_kbd_created;
static uint8_t g_vk_of[256];

static void enqueue(DWORD ofs, DWORD data)
{
    if (g_qn == 512) { g_qh = (g_qh + 1) & 511; g_qn--; }   /* overflow: drop oldest */
    dod_t *e = &g_q[(g_qh + g_qn) & 511];
    e->ofs = ofs; e->data = data; e->ts = rh_vnow(); e->seq = ++g_seq;
    g_qn++; g_events++;
}

static void record_sample(uint8_t out[256])
{
    memset(out, 0, 256);
    HWND fg = GetForegroundWindow(); DWORD pid = 0;
    if (fg) GetWindowThreadProcessId(fg, &pid);
    if (pid != GetCurrentProcessId()) return;           /* not focused: nothing held */
    for (int sc = 1; sc < 256; sc++)
        if (g_vk_of[sc] && (GetAsyncKeyState(g_vk_of[sc]) & 0x8000)) out[sc] = 0x80;
}

void rh_dinput_frame(uint32_t frame)
{
    uint8_t next[256];
    if (O.record) { record_sample(next); rh_trace_record(frame, next); }
    else memcpy(next, rh_trace_keys(frame), 256);
    if (!g_kbd_created) { memcpy(g_level, next, 256); return; }
    for (int sc = 0; sc < 256; sc++)
        if ((next[sc] ^ g_level[sc]) & 0x80) {
            enqueue((DWORD)sc, next[sc] & 0x80);
            if (O.log_input) rh_log("input f%u DIK %02x %s\n", frame, sc, next[sc] ? "down" : "up");
        }
    memcpy(g_level, next, 256);
}

/* ---- IUnknown ---- */
static HRESULT WINAPI f_QI(fake_t *s, REFIID r, void **o) { (void)r; *o = s; s->refs++; return S_OK_; }
static ULONG WINAPI f_AddRef(fake_t *s) { return (ULONG)++s->refs; }
static ULONG WINAPI f_Release(fake_t *s) { return (ULONG)(s->refs > 0 ? --s->refs : 0); }
static HRESULT WINAPI f_ok0(fake_t *s) { (void)s; return S_OK_; }
static HRESULT WINAPI f_ok1(fake_t *s, void *a) { (void)s; (void)a; return S_OK_; }
static HRESULT WINAPI f_ok2(fake_t *s, void *a, void *b) { (void)s; (void)a; (void)b; return S_OK_; }
static HRESULT WINAPI f_ok3(fake_t *s, void *a, void *b, void *c) { (void)s; (void)a; (void)b; (void)c; return S_OK_; }
static HRESULT WINAPI f_ok4(fake_t *s, void *a, void *b, void *c, void *d) { (void)s; (void)a; (void)b; (void)c; (void)d; return S_OK_; }
static HRESULT WINAPI f_ni1(fake_t *s, void *a) { (void)s; (void)a; return E_NOTIMPL_; }
static HRESULT WINAPI f_ni2(fake_t *s, void *a, void *b) { (void)s; (void)a; (void)b; return E_NOTIMPL_; }
static HRESULT WINAPI f_ni3(fake_t *s, void *a, void *b, void *c) { (void)s; (void)a; (void)b; (void)c; return E_NOTIMPL_; }
static HRESULT WINAPI f_ni4(fake_t *s, void *a, void *b, void *c, void *d) { (void)s; (void)a; (void)b; (void)c; (void)d; return E_NOTIMPL_; }

/* ---- IDirectInputDevice7A (keyboard) ---- */
static HRESULT WINAPI k_GetDeviceState(fake_t *s, DWORD n, void *buf)
{
    (void)s; g_gds_calls++;
    memset(buf, 0, n);
    memcpy(buf, g_level, n < 256 ? n : 256);
    return S_OK_;
}
static HRESULT WINAPI k_GetDeviceData(fake_t *s, DWORD cb, void *rg, DWORD *inout, DWORD fl)
{
    (void)s; g_gdd_calls++;
    DWORD want = *inout, got = 0;
    int peek = (fl & 1) != 0;   /* DIGDD_PEEK */
    int h = g_qh, n = g_qn;
    while (got < want && n) {
        if (rg) { memset((uint8_t *)rg + got * cb, 0, cb); memcpy((uint8_t *)rg + got * cb, &g_q[h], cb < sizeof(dod_t) ? cb : sizeof(dod_t)); }
        h = (h + 1) & 511; n--; got++;
    }
    if (!peek) { g_qh = h; g_qn = n; }
    *inout = got;
    return S_OK_;
}
static HRESULT WINAPI k_GetCapabilities(fake_t *s, void *caps) { (void)s; (void)caps; return S_OK_; }
static void *g_kbd_vt[29] = {
    f_QI, f_AddRef, f_Release,
    k_GetCapabilities,   /* 3  GetCapabilities */
    f_ni3,               /* 4  EnumObjects */
    f_ni2,               /* 5  GetProperty */
    f_ok2,               /* 6  SetProperty */
    f_ok0,               /* 7  Acquire */
    f_ok0,               /* 8  Unacquire */
    k_GetDeviceState,    /* 9  */
    k_GetDeviceData,     /* 10 */
    f_ok1,               /* 11 SetDataFormat */
    f_ok1,               /* 12 SetEventNotification */
    f_ok2,               /* 13 SetCooperativeLevel */
    f_ni3,               /* 14 GetObjectInfo */
    f_ni1,               /* 15 GetDeviceInfo */
    f_ok2,               /* 16 RunControlPanel */
    f_ok3,               /* 17 Initialize */
    f_ni4,               /* 18 CreateEffect */
    f_ni4,               /* 19 EnumEffects */
    f_ni2,               /* 20 GetEffectInfo */
    f_ni1,               /* 21 GetForceFeedbackState */
    f_ni1,               /* 22 SendForceFeedbackCommand */
    f_ni3,               /* 23 EnumCreatedEffectObjects */
    f_ni1,               /* 24 Escape */
    f_ok0,               /* 25 Poll */
    f_ni4,               /* 26 SendDeviceData */
    f_ni4,               /* 27 EnumEffectsInFile */
    f_ni4,               /* 28 WriteEffectToFile */
};
static fake_t *g_kbd;   /* rh_fixed_alloc'd: retail keeps the pointer */

/* ---- IDirectInput7A ---- */
static HRESULT WINAPI di_EnumDevices(fake_t *s, DWORD type, void *cb, void *ref, DWORD fl)
{ (void)s; (void)cb; (void)ref; rh_log("dinput: EnumDevices(type %lu flags %lx): none (hermetic)\n", (unsigned long)type, (unsigned long)fl); return S_OK_; }
static HRESULT WINAPI di_CreateDeviceEx(fake_t *s, REFGUID g, REFIID iid, void **out, void *outer)
{
    (void)s; (void)iid; (void)outer;
    if (g && !memcmp(g, &kGUID_SysKeyboard, sizeof(GUID))) {
        *out = g_kbd; g_kbd->refs++;
        g_kbd_created = 1;
        rh_log("dinput: keyboard device = fake (%s)\n", O.record ? "fed by the real keyboard, recorded" : "fed by the trace");
        return S_OK_;
    }
    *out = NULL;
    return DIERR_NOTFOUND_;
}
static HRESULT WINAPI di_CreateDevice(fake_t *s, REFGUID g, void **out, void *outer) { return di_CreateDeviceEx(s, g, NULL, out, outer); }
static void *g_di_vt[10] = {
    f_QI, f_AddRef, f_Release,
    di_CreateDevice,     /* 3 */
    di_EnumDevices,      /* 4 */
    f_ni1,               /* 5 GetDeviceStatus */
    f_ok2,               /* 6 RunControlPanel */
    f_ok2,               /* 7 Initialize */
    f_ni3,               /* 8 FindDevice */
    di_CreateDeviceEx,   /* 9 */
};
static fake_t *g_di;

static HRESULT WINAPI my_DirectInputCreateEx(HINSTANCE h, DWORD ver, REFIID iid, void **out, void *outer)
{
    (void)h; (void)iid; (void)outer;
    *out = g_di; g_di->refs++;
    rh_log("dinput: DirectInputCreateEx(ver %lx) = fake\n", (unsigned long)ver);
    return S_OK_;
}

void rh_dinput_install(void)
{
    g_kbd = rh_fixed_alloc(sizeof *g_kbd); g_kbd->vtbl = g_kbd_vt; g_kbd->refs = 1;
    g_di = rh_fixed_alloc(sizeof *g_di); g_di->vtbl = g_di_vt; g_di->refs = 1;
    iat_override("DirectInputCreateEx", my_DirectInputCreateEx);
    if (O.record)
        for (int sc = 1; sc < 256; sc++) {
            UINT vk = sc < 0x80 ? MapVirtualKeyA((UINT)sc, 3 /* MAPVK_VSC_TO_VK_EX */)
                                : MapVirtualKeyA(0xE000u | (UINT)(sc & 0x7f), 3);
            g_vk_of[sc] = (uint8_t)vk;
        }
}

void rh_dinput_report(void)
{
    rh_log("dinput: GetDeviceState=%ld GetDeviceData=%ld events=%ld\n", g_gds_calls, g_gdd_calls, g_events);
}
