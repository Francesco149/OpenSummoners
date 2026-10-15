/* rhost — run retail Fortune Summoners inside this process, deterministically.
 * Ported from openrecet's tools/rhost (2026-10-13) to sotes.exe (MSVC6, DirectDraw7,
 * DirectInput7, DirectSound + DirectShow BGM, single-threaded, GetTickCount-paced).
 *
 *   rhost.exe --exe sotes.unpacked.exe --game <gamedir> --run <rundir> [options]
 *
 * Determinism (always on unless noted):
 *   clock   GetTickCount = virtual: +step_ms (16) per Flip, frozen between Flips; after
 *           creep_after calls without a Flip it creeps 1 ms/call (busy-wait breaker; a pure
 *           function of the call sequence). GetLocalTime/GetSystemTime/TZ from the same clock.
 *           ⇒ every scene's 16 ms pace machine runs exactly 1 update per present (the port's
 *           cadence — docs/findings/tas-harness.md "lockstep").
 *   pump    SetTimer is fake; WaitMessage plays the WM_TIMER role (clears state+0x1c) and
 *           never blocks; the WndProc sees no OS activation/timer/paint/input messages — one
 *           synthetic WM_ACTIVATEAPP(1) right after the main window is shown.
 *   launch  the launcher dialog is not shown: its globals are set as a Windowed+Launch click.
 *   arena   retail heap = fixed-address deterministic allocator (rh_heap.c)  [--no-arena]
 *   overlay game dir read-only; writes go copy-on-write to the run dir     [--no-overlay]
 *   seed    srand(time(0)) in WinMain → srand(--seed) (default 0x4f5347 = the port's)
 *   input   fake DirectInput7 keyboard fed by the trace (rh_dinput.c); no joysticks
 *   media   BGM (DirectShow) positions on the virtual clock, silent unless --audible/--record;
 *           DirectSound silent fake unless --audible/--record (rh_media.c)
 * Options:
 *   --frames N        stop after N flips (default: trace end + --tail, else 3000)
 *   --trace F         input trace .jsonl ({"frame":N,"keys":[DIK..]} levels; "#" comments)
 *   --record F        HUMAN PLAY: visible, realtime 60 Hz, real keyboard → flat trace F
 *   --dump-at F[,F]   state dump(s) <rundir>\state_fNNNNNN.rhs (also "end")
 *   --shot-at F[,F]   back-buffer PNG(s) <rundir>\shot_fNNNNNN.png; --shot-every N
 *   --hash-every N    log the state hash every N frames
 *   --hook SPEC       shadow:/replace:/off: + name, comma-separated (rh_hooks.c)
 *   --coverage F      first-hit coverage over the hex VAs in F → <rundir>\coverage.txt
 *   --watch VA[,VA]   dwords appended to each status line
 * Output: <rundir>\rhost.log; last line "RESULT ..." (key=value, parsed by rhost.py). */
#include "rhost.h"

rh_opts_t O;
volatile uint32_t g_frame, g_ticks;
FILE *g_log;
static FILE *g_logf;
static char g_gamedir_abs[MAX_PATH];
static int g_finished;
uint32_t RT_TEXT_LO, RT_TEXT_SZ, RT_DATA_LO, RT_DATA_HI, RT_IMG_HI;

/* ---- editions ---- */
static const rh_edition_t EDITIONS[] = {
    { "enold", 0x3d0f000u, 0x5c0a8fu,
      /* flip      easer      srand      rand       rng_seed   state_ptr  active */
      0x5b8fc0u, 0x43d1d0u, 0x5bf4fbu, 0x5bf505u, 0x8a4f94u, 0x8a9b64u, 0x8a952cu,
      /* dlgproc   launch     cfg_screen cfg_sound  zdd        wndproc */
      0x4013c0u, 0x8a9a40u, 0x8a9b48u, 0x8a9b4cu, 0x8a93ccu, 0x5b12e0u,
      /* hwnd_slot  pump_wait_off */
      0x8a93ecu, 0x1cu },
    /* EN Steam SE (sotes_en.exe, buildid 24635888 / 2026-08-10, unpacked sha256 36a0294df405):
     * every VA from tools/ida/crosswalk.py (functions: bytes/mnem/graph matches; globals: xref
     * votes) except flip (0x5e14e0) + cfg_sound (0x92dfd4), read off the SE pseudocode. */
    { "ense-en", 0x4502000u, 0x5f0d7bu,
      0x5e14e0u, 0x438760u, 0x5ef169u, 0x5ef173u, 0x927304u, 0x92dfecu, 0x92d9b0u,
      0x4015a0u, 0x92dec8u, 0x92dfd0u, 0x92dfd4u, 0x92d850u, 0x5d95a0u,
      0x92d870u, 0x20u },
};
const rh_edition_t *g_ed;

void rh_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt); vfprintf(stdout, fmt, ap); va_end(ap); fflush(stdout);
    if (g_logf) { va_start(ap, fmt); vfprintf(g_logf, fmt, ap); va_end(ap); fflush(g_logf); }
}
void rh_fatal(const char *fmt, ...)
{
    char b[512]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    rh_log("FATAL %s\nRESULT status=fatal frame=%u\n", b, g_frame);
    ExitProcess(3);
}

/* ---- IAT + call-site patching (retail image only) ---- */
static IMAGE_IMPORT_DESCRIPTOR *imports(void)
{
    IMAGE_NT_HEADERS32 *nt = (void *)(0x400000 + ((IMAGE_DOS_HEADER *)0x400000)->e_lfanew);
    return (void *)(0x400000 + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
}
static uint32_t *iat_slot(const char *name)
{
    for (IMAGE_IMPORT_DESCRIPTOR *d = imports(); d->Name; d++) {
        IMAGE_THUNK_DATA32 *ilt = (void *)(0x400000 + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        IMAGE_THUNK_DATA32 *iat = (void *)(0x400000 + d->FirstThunk);
        for (; ilt->u1.AddressOfData; ilt++, iat++)
            if (!(ilt->u1.Ordinal & IMAGE_ORDINAL_FLAG32) &&
                !strcmp((char *)((IMAGE_IMPORT_BY_NAME *)(0x400000 + ilt->u1.AddressOfData))->Name, name))
                return (uint32_t *)&iat->u1.Function;
    }
    return NULL;
}
void iat_override(const char *name, void *fn)
{
    uint32_t *s = iat_slot(name);
    if (!s) rh_fatal("iat_override: retail does not import %s", name);
    *s = (uint32_t)(uintptr_t)fn;
}
int iat_try(const char *name, void *fn) { uint32_t *s = iat_slot(name); if (s) *s = (uint32_t)(uintptr_t)fn; return s != NULL; }
void *iat_lookup(const char *name) { uint32_t *s = iat_slot(name); return s ? (void *)(uintptr_t)*s : NULL; }
/* ordinal import (DSOUND!1 = DirectSoundCreate): swap the slot, return the old target */
void *iat_override_ord(const char *dll, int ord, void *fn)
{
    for (IMAGE_IMPORT_DESCRIPTOR *d = imports(); d->Name; d++) {
        if (_stricmp((const char *)(0x400000 + d->Name), dll)) continue;
        IMAGE_THUNK_DATA32 *ilt = (void *)(0x400000 + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        IMAGE_THUNK_DATA32 *iat = (void *)(0x400000 + d->FirstThunk);
        for (; ilt->u1.AddressOfData; ilt++, iat++)
            if ((ilt->u1.Ordinal & IMAGE_ORDINAL_FLAG32) && (int)(ilt->u1.Ordinal & 0xffff) == ord) {
                void *old = (void *)(uintptr_t)iat->u1.Function;
                iat->u1.Function = (uint32_t)(uintptr_t)fn;
                return old;
            }
    }
    return NULL;
}
static uint8_t *g_stub_top;
#define STUB_LO 0x5f000000u
int redirect_calls(uint32_t va, void *hook)
{
    int n = 0;
    for (uint8_t *p = (uint8_t *)(uintptr_t)RT_TEXT_LO, *e = p + RT_TEXT_SZ - 5; p < e; p++) {
        if (*p != 0xE8) continue;
        uint32_t tgt = (uint32_t)(uintptr_t)p + 5 + *(int32_t *)(p + 1);
        if (tgt != va) continue;
        *(int32_t *)(p + 1) = (int32_t)((uint32_t)(uintptr_t)hook - ((uint32_t)(uintptr_t)p + 5));
        n++;
    }
    return n;
}
/* redirect_calls + the call sites already rewritten to one of rhost's own stubs (the easer tick
 * counter, the flip hook): those reach `va` through the stub's tail `jmp va` — chain the hook
 * there. For port hooks only: a stub installer must use redirect_calls (its own fresh stub's
 * tail would otherwise be pointed back at itself). */
int redirect_calls_chain(uint32_t va, void *hook)
{
    int n = redirect_calls(va, hook);
    if (g_stub_top)
        for (uint8_t *p = (uint8_t *)(uintptr_t)STUB_LO; p + 5 <= g_stub_top; p++) {
            if (*p != 0xE9) continue;
            uint32_t tgt = (uint32_t)(uintptr_t)p + 5 + *(int32_t *)(p + 1);
            if (tgt != va) continue;
            *(int32_t *)(p + 1) = (int32_t)((uint32_t)(uintptr_t)hook - ((uint32_t)(uintptr_t)p + 5));
            n++;
        }
    return n;
}

/* ---- call stubs: any calling convention, args untouched ----
 * [inc dword [counter]] [pushad; pushfd; call pre; popfd; popad] jmp orig
 * Lives at a fixed VA (deterministic: retail .text holds rel32s to it). */
void *g_rh_flip_stub, *g_rh_easer_stub;
static void *make_stub(volatile uint32_t *counter, void (*pre)(void), uint32_t orig)
{
    if (!g_stub_top) {
        g_stub_top = VirtualAlloc((void *)(uintptr_t)STUB_LO, 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (g_stub_top != (uint8_t *)(uintptr_t)STUB_LO) rh_fatal("cannot map the stub page at %08x", STUB_LO);
    }
    uint8_t *s = g_stub_top, *p = s;
    if (counter) { *p++ = 0xFF; *p++ = 0x05; *(uint32_t *)p = (uint32_t)(uintptr_t)counter; p += 4; }
    if (pre) {
        *p++ = 0x60; *p++ = 0x9C; *p++ = 0xE8;
        *(int32_t *)p = (int32_t)((uint32_t)(uintptr_t)pre - ((uint32_t)(uintptr_t)p + 4)); p += 4;
        *p++ = 0x9D; *p++ = 0x61;
    }
    *p++ = 0xE9; *(int32_t *)p = (int32_t)(orig - ((uint32_t)(uintptr_t)p + 4)); p += 4;
    g_stub_top = (uint8_t *)(((uintptr_t)p + 15) & ~(uintptr_t)15);
    return s;
}

/* ---- fixed-address host objects ----
 * Anything retail may keep a POINTER to (fake COM objects, the command-line / environment buffers
 * the CRT stores in _acmdln/_aenvptr) lives on a fixed page, so its address — and therefore the
 * state hash — does not move when rhost itself is rebuilt (host .data layout shifts with code). */
#define FIXED_LO 0x5f080000u
#define FIXED_SZ 0x10000u
static uint8_t *g_fixed_top;
void *rh_fixed_alloc(size_t n)
{
    if (!g_fixed_top) {
        g_fixed_top = VirtualAlloc((void *)(uintptr_t)FIXED_LO, FIXED_SZ, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (g_fixed_top != (uint8_t *)(uintptr_t)FIXED_LO) rh_fatal("cannot map the fixed object page at %08x", FIXED_LO);
    }
    uint8_t *p = g_fixed_top;
    g_fixed_top += (n + 15) & ~(size_t)15;
    if (g_fixed_top > (uint8_t *)(uintptr_t)(FIXED_LO + FIXED_SZ)) rh_fatal("fixed object page exhausted");
    return memset(p, 0, n);
}

/* ---- fixed-address main stack ----
 * Retail stores stack addresses in globals (CRT), and the OS places the main stack
 * differently per run, so retail runs on a stack at a fixed VA. TEB bounds are set
 * because SEH dispatch rejects registration records outside them. */
#define MAIN_STACK_LO 0x58000000u
#define MAIN_STACK_HI 0x59000000u
#define STACK_GUARD   0x10000u
static void stacks_init(void)
{
    if (VirtualAlloc((void *)(uintptr_t)MAIN_STACK_LO, MAIN_STACK_HI - MAIN_STACK_LO, MEM_RESERVE, PAGE_READWRITE) != (void *)(uintptr_t)MAIN_STACK_LO)
        rh_fatal("cannot reserve the fixed stack at %08x (err %lu)", MAIN_STACK_LO, GetLastError());
    if (!VirtualAlloc((void *)(uintptr_t)(MAIN_STACK_LO + STACK_GUARD), MAIN_STACK_HI - MAIN_STACK_LO - STACK_GUARD, MEM_COMMIT, PAGE_READWRITE))
        rh_fatal("cannot commit the fixed stack (err %lu)", GetLastError());
}
static uint32_t __attribute__((noinline)) call_on_stack(void *fn, void *arg, uint32_t lo, uint32_t top)
{
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
    void *ob = tib->StackBase, *ol = tib->StackLimit; struct _EXCEPTION_REGISTRATION_RECORD *oe = tib->ExceptionList;
    tib->StackBase = (void *)(uintptr_t)MAIN_STACK_HI; tib->StackLimit = (void *)(uintptr_t)(lo + STACK_GUARD);
    tib->ExceptionList = (void *)(uintptr_t)0xffffffffu;
    uint32_t r;
    __asm__ volatile(
        "movl %%esp, %%esi\n\t"
        "movl %3, %%esp\n\t"
        "pushl %2\n\t"
        "call *%1\n\t"
        "movl %%esi, %%esp\n\t"
        : "=a"(r) : "r"(fn), "r"(arg), "r"(top - 64) : "esi", "ecx", "edx", "memory", "cc");
    tib->StackBase = ob; tib->StackLimit = ol; tib->ExceptionList = oe;
    return r;
}

/* ---- virtual time ---- */
static uint32_t g_vclock = 1000000u, g_calls_since_flip, g_creep_total;
uint32_t rh_vnow(void) { return g_vclock; }
/* the virtual clock's state (a shadowed port run must not perturb the creep counter) */
void rh_clock_save(uint32_t s[3]) { s[0] = g_vclock; s[1] = g_calls_since_flip; s[2] = g_creep_total; }
void rh_clock_restore(const uint32_t s[3]) { g_vclock = s[0]; g_calls_since_flip = s[1]; g_creep_total = s[2]; }
static DWORD WINAPI my_GetTickCount(void)
{
    if (++g_calls_since_flip > O.creep_after) { g_vclock++; g_creep_total++; }
    return g_vclock;
}
static void WINAPI my_GetLocalTime(SYSTEMTIME *t)
{
    uint32_t s = g_vclock / 1000u;
    memset(t, 0, sizeof *t); t->wYear = 2012; t->wMonth = 1; t->wDay = 27; t->wDayOfWeek = 5;
    t->wHour = (WORD)(12 + (s / 3600) % 12); t->wMinute = (WORD)((s / 60) % 60); t->wSecond = (WORD)(s % 60);
    t->wMilliseconds = (WORD)(g_vclock % 1000u);
}
static void WINAPI my_GetSystemTime(SYSTEMTIME *t) { my_GetLocalTime(t); }
static DWORD WINAPI my_GetTimeZoneInformation(TIME_ZONE_INFORMATION *z) { memset(z, 0, sizeof *z); return TIME_ZONE_ID_UNKNOWN; }
static void WINAPI my_Sleep(DWORD ms) { if (O.record && ms) Sleep(ms > 20 ? 20 : ms); }

/* ---- process identity the CRT copies into retail memory at startup ---- */
static char *g_cmdline, *g_envA;
static WCHAR *g_envW;
static LPSTR WINAPI my_GetCommandLineA(void) { return g_cmdline; }
static LPCH WINAPI my_GetEnvironmentStrings(void) { return g_envA; }
static LPWCH WINAPI my_GetEnvironmentStringsW(void) { return g_envW; }
static BOOL WINAPI my_FreeEnvironmentStringsA(LPCH p) { (void)p; return TRUE; }
static BOOL WINAPI my_FreeEnvironmentStringsW(LPWCH p) { (void)p; return TRUE; }
static VOID WINAPI my_GetStartupInfoA(LPSTARTUPINFOA si) { memset(si, 0, sizeof *si); si->cb = sizeof *si; si->wShowWindow = SW_SHOWNORMAL; }
static HANDLE WINAPI my_GetStdHandle(DWORD n) { (void)n; return INVALID_HANDLE_VALUE; }
static HMODULE WINAPI my_GetModuleHandleA(LPCSTR n) { return n ? GetModuleHandleA(n) : (HMODULE)0x400000; }
static DWORD WINAPI my_GetModuleFileNameA(HMODULE h, LPSTR buf, DWORD n)
{
    if (!h || h == (HMODULE)0x400000) { snprintf(buf, n, "%s\\sotes.exe", g_gamedir_abs); return (DWORD)strlen(buf); }
    return GetModuleFileNameA(h, buf, n);
}
/* The CRT keeps the PREVIOUS top-level exception filter (0x5c7c05 → DAT_008a9ef8 "lpfn"), which is
 * rhost's own (mingw) filter — a host code address that moves with every rhost rebuild. Retail gets
 * NULL as "previous" (it only chains to it); its own filter is still installed. */
static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI my_SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER f)
{ SetUnhandledExceptionFilter(f); return NULL; }

/* resources of "the exe" (hModule NULL: the map loader 0x5b62a0 FindResourceA(NULL, id, "DATA"),
 * 59 MB of .rsrc) live in the hand-mapped image at 0x400000, not in rhost.exe */
#define RMOD(h) ((h) ? (h) : (HMODULE)0x400000)
static HRSRC WINAPI my_FindResourceA(HMODULE h, LPCSTR n, LPCSTR t) { return FindResourceA(RMOD(h), n, t); }
static HGLOBAL WINAPI my_LoadResource(HMODULE h, HRSRC r) { return LoadResource(RMOD(h), r); }
static DWORD WINAPI my_SizeofResource(HMODULE h, HRSRC r) { return SizeofResource(RMOD(h), r); }

/* single-instance mutex (sub_5b5a60: CreateMutexA(0,1,name); GetLastError()!=183): unnamed, so
 * parallel rhost runs — and a human playing the real game — never collide */
static HANDLE WINAPI my_CreateMutexA(LPSECURITY_ATTRIBUTES sa, BOOL own, LPCSTR name)
{ (void)name; HANDLE h = CreateMutexA(sa, own, NULL); SetLastError(0); return h; }
static int WINAPI my_MessageBoxA(HWND h, LPCSTR text, LPCSTR cap, UINT t)
{ (void)h; (void)t; rh_log("MessageBoxA [%s] %s (frame %u)\n", cap ? cap : "", text ? text : "", g_frame); return IDOK; }

/* ---- the launcher dialog (template 0x2711, DLGPROC dlgproc): not shown.
 * Its WM_INITDIALOG→Launch round trip on the config-loaded globals is emulated with the
 * Windowed radio selected (DialogFunc 0x4013c0: radios checked only for values 3..5; the
 * HIWORD(cfg_sound) checkbox round-trips to 4 if it was 4 else 3; Launch sets launch_flag=1;
 * EndDialog(hDlg, 0)). */
typedef INT_PTR (WINAPI *dbp_t)(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
static INT_PTR WINAPI my_DialogBoxParamA(HINSTANCE hi, LPCSTR tmpl, HWND parent, DLGPROC proc, LPARAM lp)
{
    if ((uint32_t)(uintptr_t)proc != g_ed->dlgproc) {
        rh_log("DialogBoxParamA(%p, proc %p): not the launcher — returning 0\n", (void *)tmpl, (void *)proc);
        return 0;
    }
    (void)hi; (void)parent; (void)lp;
    uint32_t scr = G32(g_ed->cfg_screen), snd = G32(g_ed->cfg_sound);
    uint16_t vram = (uint16_t)(scr >> 16), slo = (uint16_t)snd, shi = (uint16_t)(snd >> 16);
    shi = shi == 4 ? 4 : 3;
    G32(g_ed->cfg_screen) = 3u | ((uint32_t)vram << 16);      /* Windowed */
    G32(g_ed->cfg_sound) = slo | ((uint32_t)shi << 16);
    G32(g_ed->launch_flag) = 1;
    rh_log("launcher: emulated Windowed+Launch (screen %08x -> %08x, sound %08x -> %08x)\n", scr, G32(g_ed->cfg_screen), snd, G32(g_ed->cfg_sound));
    return 0;
}

/* ---- window + message hermeticity ----
 * The WndProc (0x5b12e0) reacts to WM_ACTIVATEAPP (active flag, input re-acquire, music
 * pause/resume), WM_TIMER (clears the pump's wait flag), WM_PAINT (GDI present in mode 2) and
 * WM_CLOSE (exit). OS-originated activation/timer/paint/input never reach it; the pump's
 * WaitMessage stands in for WM_TIMER; one synthetic WM_ACTIVATEAPP(1) follows ShowWindow. */
static WNDPROC g_wndproc_orig[4]; static ATOM g_atom[4]; static int g_nclass;
static int g_activated;
static LRESULT CALLBACK filter_proc(HWND h, UINT m, WPARAM w, LPARAM l, WNDPROC orig)
{
    switch (m) {
    case WM_ACTIVATEAPP: case WM_ACTIVATE: case WM_TIMER: case WM_SETFOCUS: case WM_KILLFOCUS:
    case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_CHAR:
        return 0;
    case WM_PAINT: { PAINTSTRUCT ps; BeginPaint(h, &ps); EndPaint(h, &ps); return 0; }
    case WM_CLOSE:
        if (O.record) rh_finish("window-closed");
        return 0;
    default:
        return CallWindowProcA(orig, h, m, w, l);
    }
}
static LRESULT CALLBACK filt0(HWND h, UINT m, WPARAM w, LPARAM l) { return filter_proc(h, m, w, l, g_wndproc_orig[0]); }
static LRESULT CALLBACK filt1(HWND h, UINT m, WPARAM w, LPARAM l) { return filter_proc(h, m, w, l, g_wndproc_orig[1]); }
static LRESULT CALLBACK filt2(HWND h, UINT m, WPARAM w, LPARAM l) { return filter_proc(h, m, w, l, g_wndproc_orig[2]); }
static LRESULT CALLBACK filt3(HWND h, UINT m, WPARAM w, LPARAM l) { return filter_proc(h, m, w, l, g_wndproc_orig[3]); }
static WNDPROC g_filt[4] = { filt0, filt1, filt2, filt3 };
static ATOM WINAPI my_RegisterClassExA(const WNDCLASSEXA *wc)
{
    if (g_nclass >= 4) rh_fatal("more than 4 window classes");
    WNDCLASSEXA c = *wc;
    g_wndproc_orig[g_nclass] = c.lpfnWndProc;
    c.lpfnWndProc = g_filt[g_nclass];
    c.hbrBackground = rh_gdi_real(c.hbrBackground);   /* retail made it via the fake-numbered CreateBrushIndirect */
    ATOM a = RegisterClassExA(&c);
    rh_log("window class '%s' wndproc %p (filtered)\n", HIWORD((uintptr_t)c.lpszClassName) ? c.lpszClassName : "#", (void *)g_wndproc_orig[g_nclass]);
    g_atom[g_nclass++] = a;
    return a;
}
static HWND WINAPI my_CreateWindowExA(DWORD ex, LPCSTR cls, LPCSTR name, DWORD st, int x, int y, int w, int h, HWND p, HMENU m, HINSTANCE i, LPVOID lp)
{
    if (O.quiet) { ex |= WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW; x = -8000; y = -8000; }
    return CreateWindowExA(ex, cls, name, st, x, y, w, h, p, m, i, lp);
}
/* retail shows the game window with SetWindowPos(..., SWP_SHOWWINDOW) (0x5a4770 window-mode
 * switch, windowed = mode 2) or ShowWindow; that is where it gets WM_ACTIVATEAPP(1) */
static void maybe_activate(HWND hw)
{
    WNDPROC orig = (WNDPROC)(uintptr_t)GetClassLongPtrA(hw, GCLP_WNDPROC);
    for (int i = 0; i < g_nclass; i++) if (orig == g_filt[i]) orig = g_wndproc_orig[i];
    if ((uint32_t)(uintptr_t)orig == g_ed->wndproc && !g_activated) {
        g_activated = 1;
        rh_log("synthetic WM_ACTIVATEAPP(1) to the game window (frame %u)\n", g_frame);
        CallWindowProcA(orig, hw, WM_ACTIVATEAPP, 1, 0);
    }
}
static BOOL WINAPI my_SetWindowPos(HWND hw, HWND after, int x, int y, int cx, int cy, UINT fl)
{
    UINT f0 = fl;
    if (O.quiet) { x = -8000; y = -8000; fl |= SWP_NOACTIVATE | SWP_NOZORDER; fl &= ~SWP_NOMOVE; after = NULL; }
    BOOL r = SetWindowPos(hw, after, x, y, cx, cy, fl);
    if (f0 & SWP_SHOWWINDOW) maybe_activate(hw);
    return r;
}
static BOOL WINAPI my_ShowWindow(HWND hw, int cmd)
{
    BOOL r = ShowWindow(hw, O.quiet && cmd != SW_HIDE ? SW_SHOWNOACTIVATE : cmd);
    if (cmd != SW_HIDE) maybe_activate(hw);
    return r;
}
static BOOL WINAPI my_SetForegroundWindow(HWND hw) { return O.quiet ? TRUE : SetForegroundWindow(hw); }
static UINT_PTR WINAPI my_SetTimer(HWND hw, UINT_PTR id, UINT ms, TIMERPROC fn) { (void)hw; (void)ms; (void)fn; return id ? id : 1; }
static BOOL WINAPI my_KillTimer(HWND hw, UINT_PTR id) { (void)hw; (void)id; return TRUE; }
/* the pump (EN-old 0x5b1030) waits here while the app is inactive or the wait flag state+0x1c is set;
 * WM_TIMER is what clears that flag in retail — do it now, deterministically */
static uint32_t g_waits_since_flip;
static BOOL WINAPI my_WaitMessage(void)
{
    uint32_t st = G32(g_ed->state_ptr);
    if (st) G32(st + g_ed->pump_wait_off) = 0;
    if (!G32(g_ed->active) && g_waits_since_flip == 0) rh_log("WaitMessage while inactive (frame %u)\n", g_frame);
    if (++g_waits_since_flip > 5000000u) rh_fatal("pump spins in WaitMessage without a Flip (inactive=%d)", !G32(g_ed->active));
    return TRUE;
}

/* ---- coverage: one-shot INT3 at listed entries ---- */
static uint32_t *g_cov_va; static uint8_t *g_cov_orig; static uint32_t *g_cov_hit; static int g_cov_n;
static int cov_find(uint32_t va)
{ int lo = 0, hi = g_cov_n - 1; while (lo <= hi) { int m = (lo + hi) / 2; if (g_cov_va[m] == va) return m; if (g_cov_va[m] < va) lo = m + 1; else hi = m - 1; } return -1; }
static LONG CALLBACK cov_veh(EXCEPTION_POINTERS *e)
{
    if (e->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT) return EXCEPTION_CONTINUE_SEARCH;
    uint32_t a = (uint32_t)(uintptr_t)e->ExceptionRecord->ExceptionAddress;
    int i = cov_find(a);
    if (i < 0) return EXCEPTION_CONTINUE_SEARCH;
    *(volatile uint8_t *)(uintptr_t)a = g_cov_orig[i];
    if (!g_cov_hit[i]) g_cov_hit[i] = g_frame + 1;
    e->ContextRecord->Eip = a;
    return EXCEPTION_CONTINUE_EXECUTION;
}
static int cmpu(const void *a, const void *b) { uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b; return x < y ? -1 : x > y; }
static void cov_install(void)
{
    FILE *f = fopen(O.coverage, "r"); if (!f) rh_fatal("cannot read %s", O.coverage);
    unsigned v; int cap = 4096; g_cov_va = malloc(cap * 4);
    while (fscanf(f, "%x", &v) == 1) {
        if (v < RT_TEXT_LO || v >= RT_TEXT_LO + RT_TEXT_SZ) continue;
        if (g_cov_n == cap) { cap *= 2; g_cov_va = realloc(g_cov_va, cap * 4); }
        g_cov_va[g_cov_n++] = v;
    }
    fclose(f);
    qsort(g_cov_va, g_cov_n, 4, cmpu);
    g_cov_orig = malloc(g_cov_n); g_cov_hit = calloc(g_cov_n, 4);
    AddVectoredExceptionHandler(1, cov_veh);
    for (int i = 0; i < g_cov_n; i++) { g_cov_orig[i] = *(uint8_t *)(uintptr_t)g_cov_va[i]; *(uint8_t *)(uintptr_t)g_cov_va[i] = 0xCC; }
    rh_log("coverage: %d entries armed\n", g_cov_n);
}
static void cov_dump(void)
{
    if (!g_cov_n) return;
    char p[MAX_PATH]; snprintf(p, sizeof p, "%s\\coverage.txt", O.rundir);
    FILE *f = fopen(p, "w"); if (!f) return;
    int hits = 0;
    for (int i = 0; i < g_cov_n; i++) if (g_cov_hit[i]) { fprintf(f, "%08x %u\n", g_cov_va[i], g_cov_hit[i] - 1); hits++; }
    fclose(f);
    rh_log("coverage: %d/%d hit\n", hits, g_cov_n);
}

/* ---- --watch-write VA@FRAME: hardware write watchpoints (DR0..DR3) on the main thread ----
 * Armed at the flip ending frame FRAME through a VEH (the context of a deliberate
 * breakpoint exception gets Dr0-3/Dr7), so every later write to VA logs the writing
 * instruction (EIP after the write), the new value and the retail return-address chain. */
static uint32_t g_wva[4], g_wfr[4], g_wlast[4]; static int g_nw, g_warmed; static long g_whits[4];
#define WATCH_ARM_CODE 0xE0551001u
static void stack_chain(char *b, size_t n, uint32_t esp)
{
    int k = 0; size_t o = 0; b[0] = 0;
    for (int i = 0; i < 512 && k < 6; i++) {
        uint32_t v = ((uint32_t *)(uintptr_t)esp)[i];
        if (v >= RT_TEXT_LO && v < RT_TEXT_LO + RT_TEXT_SZ) { o += snprintf(b + o, n - o, "%s%x", k ? "<" : "", v); k++; }
    }
}
static LONG CALLBACK watch_veh(EXCEPTION_POINTERS *e)
{
    CONTEXT *c = e->ContextRecord;
    if (e->ExceptionRecord->ExceptionCode == WATCH_ARM_CODE) {
        uint32_t dr7 = 0;
        for (int i = 0; i < g_nw; i++) if (g_warmed & (1 << i)) {
            (&c->Dr0)[i] = g_wva[i];
            dr7 |= (1u << (2 * i)) | (1u << (16 + 4 * i)) | (3u << (18 + 4 * i));   /* local enable, write, 4 bytes */
        }
        c->Dr7 = dr7;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (e->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP) return EXCEPTION_CONTINUE_SEARCH;
    uint32_t dr6 = c->Dr6;
    int hit = 0;
    for (int i = 0; i < g_nw; i++) if (dr6 & (1u << i)) {
        hit = 1;
        if (c->Eip >= 0x5f010000u && c->Eip < 0x5f050000u) continue;   /* the scrubber zeroing it */
        uint32_t nv = G32(g_wva[i]);
        int changed = nv != g_wlast[i]; g_wlast[i] = nv;
        if (g_whits[i]++ < 40 || (changed && g_whits[i] < 1000000)) {   /* first 40, then every value change */
            char st[128]; stack_chain(st, sizeof st, c->Esp);
            rh_log("watch %08x f%u eip=%08x val=%08x [%s]\n", g_wva[i], g_frame, c->Eip, G32(g_wva[i]), st);
        }
    }
    if (!hit) return EXCEPTION_CONTINUE_SEARCH;
    c->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}
/* --trap VA[,VA]: INT3 at an instruction; every hit logs the registers (first 50), re-arms by
 * single-stepping over the restored byte */
static uint32_t g_tva[8]; static int g_ntrap; static uint8_t g_torig[8]; static long g_thits[8]; static int g_trearm = -1;
/* --trap-census VA:OFF — a trap that also histograms the dword at [ecx+OFF] per hit (a thiscall's
 * dispatch field: which codes / types reach VA); reported at exit, most frequent first */
#define CENSUS_N 256
static int32_t g_cen_off[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static uint32_t g_cen_val[8][CENSUS_N]; static long g_cen_cnt[8][CENSUS_N]; static int g_cen_n[8]; static long g_cen_over[8];
static void census_add(int t, uint32_t v)
{
    for (int k = 0; k < g_cen_n[t]; k++) if (g_cen_val[t][k] == v) { g_cen_cnt[t][k]++; return; }
    if (g_cen_n[t] < CENSUS_N) { g_cen_val[t][g_cen_n[t]] = v; g_cen_cnt[t][g_cen_n[t]++] = 1; } else g_cen_over[t]++;
}
static void census_report(void)
{
    for (int t = 0; t < g_ntrap; t++) {
        if (g_cen_off[t] < 0) continue;
        for (int a = 0; a < g_cen_n[t]; a++)          /* sort by count, descending */
            for (int b = a + 1; b < g_cen_n[t]; b++)
                if (g_cen_cnt[t][b] > g_cen_cnt[t][a]) {
                    uint32_t v = g_cen_val[t][a]; long c = g_cen_cnt[t][a];
                    g_cen_val[t][a] = g_cen_val[t][b]; g_cen_cnt[t][a] = g_cen_cnt[t][b];
                    g_cen_val[t][b] = v; g_cen_cnt[t][b] = c;
                }
        for (int k = 0; k < g_cen_n[t]; k++)
            rh_log("trap-census %08x +%x value %u (0x%x) hits=%ld\n", g_tva[t], g_cen_off[t], g_cen_val[t][k], g_cen_val[t][k], g_cen_cnt[t][k]);
        if (g_cen_over[t]) rh_log("trap-census %08x +%x overflow hits=%ld\n", g_tva[t], g_cen_off[t], g_cen_over[t]);
    }
}
static LONG CALLBACK trap_veh(EXCEPTION_POINTERS *e)
{
    CONTEXT *c = e->ContextRecord;
    uint32_t a = (uint32_t)(uintptr_t)e->ExceptionRecord->ExceptionAddress;
    if (e->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) {
        for (int i = 0; i < g_ntrap; i++) if (g_tva[i] == a) {
            if (g_cen_off[i] >= 0) census_add(i, *(uint32_t *)(uintptr_t)(c->Ecx + (uint32_t)g_cen_off[i]));
            if (g_thits[i]++ < O.trap_max)
                rh_log("trap %08x f%u ret=%08x eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx\n", a, g_frame,
                       *(uint32_t *)(uintptr_t)c->Esp, c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
            *(volatile uint8_t *)(uintptr_t)a = g_torig[i];
            c->Eip = a; c->EFlags |= 0x100; g_trearm = i;     /* single-step, then re-arm */
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (e->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP && g_trearm >= 0 && !(c->Dr6 & 0xf)) {
        *(volatile uint8_t *)(uintptr_t)g_tva[g_trearm] = 0xCC; g_trearm = -1;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void watch_arm_due(void)
{
    int arm = 0;
    for (int i = 0; i < g_nw; i++) if (!(g_warmed & (1 << i)) && g_wfr[i] == g_frame) { g_warmed |= 1 << i; arm = 1; }
    if (arm) { RaiseException(WATCH_ARM_CODE, 0, 0, NULL); rh_log("watch: armed at frame %u\n", g_frame); }
}

/* ---- uninitialized-read fixes (quirk #123) ----
 * Two gamma-palette builders write bytes 0..2 of 256 stack dwords and copy all 1 KB to the heap,
 * so byte 3 = whatever the stack held. With the system stack that is retail's own (deterministic)
 * data — but it still differs between a pure-retail run and one where a hook's port code ran on
 * the retail stack, which made hybrid/shadow end states incomparable. Byte 3 is not colour: zero
 * it at both sites (identical in every mode). --no-uninit-fix keeps raw retail behaviour. EN-old. */
typedef void (__attribute__((thiscall)) *palcopy_t)(void *, uint8_t *);
static uint32_t g_palcopy_va;
static void __attribute__((thiscall)) w_pal_copy(void *self, uint8_t *src)
{
    for (int i = 0; i < 256; i++) src[4 * i + 3] = 0;
    ((palcopy_t)(uintptr_t)g_palcopy_va)(self, src);
}
/* per edition: A = `call palcopy` sites (palette at the pushed src), B = inline `lea esi,[esp+0x418]`
 * before a 1 KB rep movsd (+ resume VA), C = move-command copy `lea edi,[esi+OFF]` (+ the
 * stack offset of the WORD-written last field's high half, + resume VA) */
typedef struct { const char *tag; uint32_t palcopy, a[4]; uint32_t b, b_resume; uint32_t c, c_off, c_pad, c_resume; uint32_t sbh_alloc; } uninit_t;
static const uninit_t UNINIT[] = {
    { "enold",   0x491770u, { 0x586f79u }, 0x4181dfu, 0x4181e6u, 0x440dd1u, 0x14854u, 0x42u, 0x440dd7u, 0x5c73fdu },
    { "ense-en", 0x48c300u, { 0x5ad66bu, 0x5ad95bu }, 0x41233fu, 0x412346u, 0x43bcccu, 0x14874u, 0x46u, 0x43bcd2u, 0x5f7afau },
};
/* site D (quirk #130): retail reads UNINITIALIZED HEAP — a reused CRT small block keeps the previous
 * owner's bytes, e.g. the 0x54-byte node `new`ed at 0x4158a4 only fills its word array up to its
 * count, so +0x44 can hold the low half of a DirectDraw lpSurface left by a freed DDSURFACEDESC (a
 * system-DLL pointer ⇒ different every run). Zero every block the MSVC6 small-block heap hands out
 * (___sbh_alloc_block: malloc/new + realloc moves; calloc zeroes anyway); rh_heap zeroes the
 * HeapAlloc path (> the SBH threshold) under the same switch. */
typedef void *(__cdecl *sbh_alloc_t)(size_t);
static uint32_t g_sbh_alloc_va;
static void *__cdecl w_sbh_alloc(size_t n)
{
    void *p = ((sbh_alloc_t)(uintptr_t)g_sbh_alloc_va)(n);
    if (p) memset(p, 0, n);
    return p;
}
static int uninit_fix_install(void)
{
    const uninit_t *u = NULL;
    for (size_t i = 0; i < sizeof UNINIT / sizeof *UNINIT; i++) if (!strcmp(UNINIT[i].tag, g_ed->tag)) u = &UNINIT[i];
    if (!u) return 0;
    int n = 0;
    g_palcopy_va = u->palcopy;
    /* site A: EVERY call of the palette copy (15/16 callers; most build the palette with the same
     * 3-bytes-per-entry LUT loop, so byte 3 can never carry meaning in retail) */
    (void)u->a;
    n += redirect_calls(u->palcopy, w_pal_copy);
    g_sbh_alloc_va = u->sbh_alloc;
    n += redirect_calls(u->sbh_alloc, w_sbh_alloc);
    rh_heap_zero_alloc = 1;
    static const uint8_t want_b[7] = { 0x8D, 0xB4, 0x24, 0x18, 0x04, 0x00, 0x00 };   /* lea esi,[esp+0x418] */
    uint8_t *q = (uint8_t *)(uintptr_t)u->b;
    if (!memcmp(q, want_b, 7)) {                            /* site B: inline palette before rep movsd */
        uint8_t *c = g_stub_top, *o = c;
        *o++ = 0x60; *o++ = 0x9C;                                    /* pushad; pushfd */
        *o++ = 0x8D; *o++ = 0xBC; *o++ = 0x24; *(uint32_t *)o = 36 + 0x418 + 3; o += 4;   /* lea edi,[esp+36+0x418+3] */
        *o++ = 0xB9; *(uint32_t *)o = 256; o += 4;                   /* mov ecx,256 */
        uint8_t *loop = o;
        *o++ = 0xC6; *o++ = 0x07; *o++ = 0x00;                       /* mov byte [edi],0 */
        *o++ = 0x83; *o++ = 0xC7; *o++ = 0x04;                       /* add edi,4 */
        *o++ = 0x49;                                                 /* dec ecx */
        *o++ = 0x75; *o = (uint8_t)(loop - (o + 1)); o++;            /* jnz loop */
        *o++ = 0x9D; *o++ = 0x61;                                    /* popfd; popad */
        memcpy(o, want_b, 7); o += 7;
        *o++ = 0xE9; *(int32_t *)o = (int32_t)(u->b_resume - ((uint32_t)(uintptr_t)o + 4)); o += 4;
        g_stub_top = (uint8_t *)(((uintptr_t)o + 15) & ~(uintptr_t)15);
        q[0] = 0xE9; *(int32_t *)(q + 1) = (int32_t)((uint32_t)(uintptr_t)c - (u->b + 5)); q[5] = 0x90; q[6] = 0x90;
        n++;
    }
    uint8_t want_c[6] = { 0x8D, 0xBE };                     /* lea edi,[esi+OFF] */
    *(uint32_t *)(want_c + 2) = u->c_off;
    uint8_t *r = (uint8_t *)(uintptr_t)u->c;
    if (!memcmp(r, want_c, 6)) {                            /* site C: move-command pad before the copy */
        uint8_t *c = g_stub_top, *o = c;
        *o++ = 0x66; *o++ = 0xC7; *o++ = 0x44; *o++ = 0x24; *o++ = (uint8_t)u->c_pad; *o++ = 0x00; *o++ = 0x00;   /* mov word [esp+pad],0 */
        memcpy(o, want_c, 6); o += 6;
        *o++ = 0xE9; *(int32_t *)o = (int32_t)(u->c_resume - ((uint32_t)(uintptr_t)o + 4)); o += 4;
        g_stub_top = (uint8_t *)(((uintptr_t)o + 15) & ~(uintptr_t)15);
        r[0] = 0xE9; *(int32_t *)(r + 1) = (int32_t)((uint32_t)(uintptr_t)c - (u->c + 5)); r[5] = 0x90;
        n++;
    }
    return n;
}

/* ---- engine hooks ---- */
typedef void (__cdecl *srand_t)(unsigned);
static void __cdecl my_srand(unsigned s)
{
    rh_log("srand(%08x) -> pinned %08x (frame %u)\n", s, O.has_seed ? O.seed : s, g_frame);
    ((srand_t)(uintptr_t)g_ed->srand)(O.has_seed ? O.seed : s);
}

static void finish(const char *why);
void rh_finish(const char *why) { finish(why); }
static LARGE_INTEGER g_t0, g_qf;
static int is_dump_frame(uint32_t f) { for (int i = 0; i < O.n_dump; i++) if (O.dump_at[i] == f) return 1; return 0; }
static void dump_now(const char *tag)
{
    char p[MAX_PATH]; snprintf(p, sizeof p, "%s\\state_%s.rhs", O.rundir, tag);
    rh_state_dump(p);
}
static void watch_str(char *wb, size_t n)
{
    size_t wo = 0; wb[0] = 0;
    for (int i = 0; i < O.n_watch; i++) wo += snprintf(wb + wo, n - wo, " %x=%d", O.watch[i], (int)G32(O.watch[i]));
}
/* runs at the ENTRY of every Flip (the frame's back buffer is complete): frame g_frame ends */
static void __cdecl pre_flip(void)
{
    if (g_finished) return;
    if (g_nw) watch_arm_due();
    char t[32]; snprintf(t, sizeof t, "f%06u", g_frame);
    if (is_dump_frame(g_frame)) dump_now(t);
    for (int i = 0; i < O.n_shot; i++) if (O.shot_at[i] == g_frame) rh_shot(t);
    if (O.shot_every && g_frame % O.shot_every == 0) rh_shot(t);
    if (O.log_every && g_frame % O.log_every == 0) {
        uint16_t cw; __asm__ volatile("fnstcw %0" : "=m"(cw));
        char wb[256]; watch_str(wb, sizeof wb);
        rh_log("frame %u tick=%u vclock=%u rng=%08x cw=%04x creep=%u%s\n", g_frame, g_ticks, g_vclock,
               G32(g_ed->rng_seed), cw, g_creep_total, wb);
    }
    if (O.hash_every && g_frame % O.hash_every == 0) {
        rh_hash_t h; rh_state_hash(&h);
        rh_log("hash f%u full=%016llx sem=%016llx masked=%u\n", g_frame, (unsigned long long)h.full, (unsigned long long)h.semantic, h.masked);
    }
    if (g_frame >= O.frames) finish("frames");
    g_frame++;
    g_vclock += O.step_ms; g_calls_since_flip = 0; g_waits_since_flip = 0;
    rh_dinput_frame(g_frame);
    if (O.realtime) {
        LARGE_INTEGER q; QueryPerformanceCounter(&q);
        double due = g_frame * (1000.0 / 60.0), now = (q.QuadPart - g_t0.QuadPart) * 1000.0 / g_qf.QuadPart;
        if (due > now + 1) Sleep((DWORD)(due - now));
    }
    rh_scrub_here();
}

static void finish(const char *why)
{
    if (g_finished) return;
    g_finished = 1;
    rh_trace_finish();
    cov_dump();
    rh_hooks_report();
    for (int i = 0; i < g_ntrap; i++) rh_log("trap-count %08x hits=%ld\n", g_tva[i], g_thits[i]);
    census_report();
    rh_media_report();
    rh_log("scrub: %ld thunks, %llu dirty pages zeroed\n", rh_scrub_thunks(), (unsigned long long)rh_scrub_pages());
    if (is_dump_frame(0xffffffffu)) dump_now("end");
    for (int i = 0; i < O.n_shot; i++) if (O.shot_at[i] == 0xffffffffu) rh_shot("end");
    char hs[256]; rh_heap_stats(hs, sizeof hs);
    rh_log("%s\n", hs);
    if (O.log_ext) rh_state_ext_report();
    rh_hash_t h; rh_state_hash(&h);
    LARGE_INTEGER q; QueryPerformanceCounter(&q);
    double ms = (q.QuadPart - g_t0.QuadPart) * 1000.0 / g_qf.QuadPart;
    rh_log("RESULT status=%s reason=%s frame=%u tick=%u rng=%08x state_full=%016llx state_sem=%016llx masked=%u state_bytes=%u wall_ms=%.0f fps=%.0f creep=%u %s\n",
           !strcmp(why, "fatal") ? "fatal" : !strcmp(why, "game-exit") ? "exit" : "ok", why, g_frame, g_ticks, G32(g_ed->rng_seed), (unsigned long long)h.full,
           (unsigned long long)h.semantic, h.masked, h.bytes, ms, ms > 0 ? g_frame * 1000.0 / ms : 0.0, g_creep_total, rh_trace_status());
    if (O.stall_ms) Sleep(O.stall_ms);
    if (g_logf) fclose(g_logf);
    ExitProcess(0);
}
static VOID WINAPI my_ExitProcess(UINT code) { if (!g_finished) { rh_log("retail ExitProcess(%u) at frame %u\n", code, g_frame); finish("game-exit"); } ExitProcess(code); }

/* ---- CLI ---- */
static void usage(void)
{
    fprintf(stderr, "usage: rhost --exe RETAIL --game DIR --run DIR [--frames N] [--seed S|--no-seed] [--trace F]\n"
                    "  [--record F] [--realtime] [--dump-at F[,F|end]] [--shot-at F[,F|end]] [--shot-every N]\n"
                    "  [--hash-every N] [--log-every N] [--hook SPEC] [--coverage F] [--stall-ms N] [--tail N]\n"
                    "  [--watch VA,..] [--step-ms N] [--creep-after N] [--audible] [--show] [--no-arena] [--no-overlay]\n"
                    "  [--log-files] [--log-heap] [--log-ext] [--log-media] [--log-input]\n");
    ExitProcess(2);
}
static void parse_list(const char *s, uint32_t *out, int *n, int max)
{
    char b[512]; snprintf(b, sizeof b, "%s", s);
    for (char *t = strtok(b, ","); t && *n < max; t = strtok(NULL, ","))
        out[(*n)++] = !strcmp(t, "end") ? 0xffffffffu : strtoul(t, 0, 0);
}
static void parse(int argc, char **argv)
{
    O.seed = 0x4f5347; O.has_seed = 1; O.tail = 120; O.log_every = 600; O.frames = 0; O.quiet = 1;
    O.step_ms = 16; O.creep_after = 2000; O.stall_wall_ms = 60000; O.scrub_kb = 32; O.trap_max = 50;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        #define ARG() (i + 1 < argc ? argv[++i] : (usage(), (char *)0))
        if (!strcmp(a, "--exe")) O.exe = ARG();
        else if (!strcmp(a, "--game")) O.gamedir = ARG();
        else if (!strcmp(a, "--run")) O.rundir = ARG();
        else if (!strcmp(a, "--edition")) O.edition = ARG();
        else if (!strcmp(a, "--frames")) O.frames = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--seed")) { O.seed = strtoul(ARG(), 0, 0); O.has_seed = 1; }
        else if (!strcmp(a, "--no-seed")) O.has_seed = 0;
        else if (!strcmp(a, "--trace")) O.trace = ARG();
        else if (!strcmp(a, "--record")) { O.record = ARG(); O.realtime = 1; O.quiet = 0; O.audible = 1; }
        else if (!strcmp(a, "--realtime")) O.realtime = 1;
        else if (!strcmp(a, "--dump-at")) parse_list(ARG(), O.dump_at, &O.n_dump, 16);
        else if (!strcmp(a, "--shot-at")) parse_list(ARG(), O.shot_at, &O.n_shot, 64);
        else if (!strcmp(a, "--shot-every")) O.shot_every = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--hash-every")) O.hash_every = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--log-every")) O.log_every = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--tail")) O.tail = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--hook")) O.hooks = ARG();
        else if (!strcmp(a, "--coverage")) O.coverage = ARG();
        else if (!strcmp(a, "--stall-ms")) O.stall_ms = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--step-ms")) O.step_ms = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--creep-after")) O.creep_after = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--stall-wall")) O.stall_wall_ms = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--scrub-kb")) O.scrub_kb = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--audible")) O.audible = 1;
        else if (!strcmp(a, "--silent")) O.silent = 1;
        else if (!strcmp(a, "--show")) O.quiet = 0;
        else if (!strcmp(a, "--quiet")) O.quiet = 1;
        else if (!strcmp(a, "--no-arena")) O.no_arena = 1;
        else if (!strcmp(a, "--no-uninit-fix")) O.no_uninit_fix = 1;
        else if (!strcmp(a, "--perturb-dlls")) O.perturb_dlls = 1;
        else if (!strcmp(a, "--no-overlay")) O.no_overlay = 1;
        else if (!strcmp(a, "--log-files")) O.log_files = 1;
        else if (!strcmp(a, "--log-heap")) O.log_heap = 1;
        else if (!strcmp(a, "--log-ext")) O.log_ext = 1;
        else if (!strcmp(a, "--log-media")) O.log_media = 1;
        else if (!strcmp(a, "--log-input")) O.log_input = 1;
        else if (!strcmp(a, "--watch-write")) {
            char b[256]; snprintf(b, sizeof b, "%s", ARG());
            for (char *t = strtok(b, ","); t && g_nw < 4; t = strtok(NULL, ",")) {
                char *at = strchr(t, '@'); g_wfr[g_nw] = at ? strtoul(at + 1, 0, 0) : 0; if (at) *at = 0;
                g_wva[g_nw++] = strtoul(t, 0, 16) & ~3u;
            }
        }
        else if (!strcmp(a, "--trap-max")) O.trap_max = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--trap-census")) {     /* VA:OFF (hex), one per flag; no register log */
            char *t = ARG(), *c = strchr(t, ':');
            if (!c || g_ntrap >= 8) rh_fatal("--trap-census VA:OFF");
            g_cen_off[g_ntrap] = (int32_t)strtoul(c + 1, 0, 16);
            g_tva[g_ntrap++] = strtoul(t, 0, 16);
            O.trap_max = 0;
        }
        else if (!strcmp(a, "--trap")) {
            char b[256]; snprintf(b, sizeof b, "%s", ARG());
            for (char *t = strtok(b, ","); t && g_ntrap < 8; t = strtok(NULL, ",")) g_tva[g_ntrap++] = strtoul(t, 0, 16);
        }
        else if (!strcmp(a, "--watch")) {
            char b[256]; snprintf(b, sizeof b, "%s", ARG());
            for (char *t = strtok(b, ","); t && O.n_watch < 16; t = strtok(NULL, ",")) O.watch[O.n_watch++] = strtoul(t, 0, 16);
        }
        else { fprintf(stderr, "unknown option %s\n", a); usage(); }
        #undef ARG
    }
    if (!O.exe || !O.gamedir || !O.rundir) usage();
    if (O.silent) O.audible = 0;
    static char rec_abs[MAX_PATH];
    if (O.record && !strchr(O.record, ':') && O.record[0] != '\\') { snprintf(rec_abs, sizeof rec_abs, "%s\\%s", O.rundir, O.record); O.record = rec_abs; }
    if (!O.frames) O.frames = O.record ? 0xfffffff0u : O.trace ? 0 : 3000u;
    else if (O.record) { /* explicit --frames with --record: a bounded recording (tests) */ }
}

static void load_volatile(void)
{
    rh_state_volatile_add(g_ed->hwnd_slot);
    char p[MAX_PATH]; snprintf(p, sizeof p, "%s\\volatile.txt", O.rundir);
    FILE *f = fopen(p, "r"); if (!f) return;
    char line[256]; unsigned v; int n = 0;
    while (fgets(line, sizeof line, f)) if (sscanf(line, "%x", &v) == 1) { rh_state_volatile_add(v); n++; }
    fclose(f);
    rh_log("volatile: %d masked addresses\n", n);
}

static void layout_from_pe(void)
{
    IMAGE_NT_HEADERS32 *nt = (void *)(0x400000 + ((IMAGE_DOS_HEADER *)0x400000)->e_lfanew);
    RT_IMG_HI = 0x400000u + nt->OptionalHeader.SizeOfImage;
    IMAGE_SECTION_HEADER *s = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        uint32_t lo = 0x400000u + s[i].VirtualAddress;
        if (!memcmp(s[i].Name, ".text", 6)) { RT_TEXT_LO = lo; RT_TEXT_SZ = s[i].Misc.VirtualSize; }
        if (!memcmp(s[i].Name, ".data", 6)) { RT_DATA_LO = lo; RT_DATA_HI = (lo + s[i].Misc.VirtualSize + 3u) & ~3u; }
    }
    for (size_t i = 0; i < sizeof EDITIONS / sizeof *EDITIONS; i++)
        if (EDITIONS[i].image_size == nt->OptionalHeader.SizeOfImage &&
            EDITIONS[i].entry == 0x400000u + nt->OptionalHeader.AddressOfEntryPoint &&
            (!O.edition || !strcmp(O.edition, EDITIONS[i].tag))) g_ed = &EDITIONS[i];
    if (!g_ed) rh_fatal("unsupported edition (SizeOfImage %08x entry %08x) — add its VAs to EDITIONS",
                        nt->OptionalHeader.SizeOfImage, 0x400000u + nt->OptionalHeader.AddressOfEntryPoint);
}

/* wall-clock watchdog (host thread; reads g_frame only): no Flip for --stall-wall ms → stalled */
static DWORD WINAPI watchdog(LPVOID p)
{
    (void)p;
    uint32_t last = 0xffffffffu; DWORD since = GetTickCount();
    for (;;) {
        Sleep(500);
        if (g_finished) return 0;
        if (g_frame != last) { last = g_frame; since = GetTickCount(); continue; }
        if (GetTickCount() - since > O.stall_wall_ms) {
            rh_log("STALL: no Flip for %u ms wall at frame %u (tick %u)\nRESULT status=stalled reason=wall frame=%u tick=%u rng=%08x\n",
                   O.stall_wall_ms, g_frame, g_ticks, g_frame, g_ticks, G32(g_ed->rng_seed));
            if (g_logf) fflush(g_logf);
            ExitProcess(4);
        }
    }
}

/* host-only overrides that never enter a system DLL: not worth a scrub thunk (hot paths) */
void *rh_scrub_skip[] = { (void *)my_GetTickCount, (void *)my_WaitMessage, (void *)my_GetLocalTime, NULL };

RETAIL_HOLE_DEFINE

int main(int argc, char **argv)
{
    parse(argc, argv);
    QueryPerformanceFrequency(&g_qf); QueryPerformanceCounter(&g_t0);
    CreateDirectoryA(O.rundir, NULL);
    char lp[MAX_PATH]; snprintf(lp, sizeof lp, "%s\\rhost.log", O.rundir);
    g_logf = fopen(lp, "w");
    g_log = stdout;
    if (!O.no_arena && rh_heap_init()) rh_fatal("heap arena reservation failed");
    stacks_init();
    GetFullPathNameA(O.gamedir, sizeof g_gamedir_abs, g_gamedir_abs, NULL);
    for (size_t l = strlen(g_gamedir_abs); l && g_gamedir_abs[l - 1] == '\\'; ) g_gamedir_abs[--l] = 0;
    char exe_abs[MAX_PATH]; GetFullPathNameA(O.exe, sizeof exe_abs, exe_abs, NULL);
    if (!SetCurrentDirectoryA(g_gamedir_abs)) rh_fatal("cannot cd to %s", g_gamedir_abs);
    if (O.perturb_dlls) {
        /* force the DLLs retail's imports pull in to load at DIFFERENT bases than this boot's ASLR
         * slot (probe-load, note the base, free, reserve the range): simulates a reboot for the
         * portable-hash check. Only DLLs not already in the process can move. */
        static const char *dl[] = { "ddraw.dll", "dinput.dll", "dsound.dll", "version.dll", "ole32.dll", "quartz.dll", "winmm.dll", NULL };
        for (int i = 0; dl[i]; i++) {
            if (GetModuleHandleA(dl[i])) { rh_log("perturb: %s already loaded, cannot move\n", dl[i]); continue; }
            HMODULE m = LoadLibraryExA(dl[i], NULL, DONT_RESOLVE_DLL_REFERENCES);
            if (!m) continue;
            MEMORY_BASIC_INFORMATION mi; VirtualQuery(m, &mi, sizeof mi);
            IMAGE_NT_HEADERS32 *nt = (void *)((uint8_t *)m + ((IMAGE_DOS_HEADER *)m)->e_lfanew);
            uint32_t base = (uint32_t)(uintptr_t)m, size = nt->OptionalHeader.SizeOfImage;
            FreeLibrary(m);
            void *r = VirtualAlloc((void *)(uintptr_t)base, size, MEM_RESERVE, PAGE_NOACCESS);
            rh_log("perturb: %s base %08x+%x %s\n", dl[i], base, size, r ? "reserved (will relocate)" : "not reservable");
        }
    }
    retail_image_t img;
    if (retail_map(exe_abs, &img, 1)) rh_fatal("map %s failed", exe_abs);
    layout_from_pe();
    rh_log("rhost: mapped %s edition=%s (imports %d resolved, %d stubbed) entry=%08x text=%08x+%x data=%08x..%08x\n",
           exe_abs, g_ed->tag, img.n_imports_resolved, img.n_imports_stubbed, img.entry, RT_TEXT_LO, RT_TEXT_SZ, RT_DATA_LO, RT_DATA_HI);
    iat_override("GetModuleHandleA", my_GetModuleHandleA);
    iat_override("GetModuleFileNameA", my_GetModuleFileNameA);
    iat_override("FindResourceA", my_FindResourceA);
    iat_try("SetUnhandledExceptionFilter", my_SetUnhandledExceptionFilter);
    iat_override("LoadResource", my_LoadResource);
    iat_override("SizeofResource", my_SizeofResource);
    iat_override("ExitProcess", my_ExitProcess);
    g_cmdline = rh_fixed_alloc(MAX_PATH + 8);
    snprintf(g_cmdline, MAX_PATH + 8, "\"%s\\sotes.exe\"", g_gamedir_abs);
    static const char envA[] = "SystemRoot=C:\\Windows\0\0";
    static const WCHAR envW[] = L"SystemRoot=C:\\Windows\0\0";
    g_envA = rh_fixed_alloc(sizeof envA); memcpy(g_envA, envA, sizeof envA);
    g_envW = rh_fixed_alloc(sizeof envW); memcpy(g_envW, envW, sizeof envW);
    iat_override("GetCommandLineA", my_GetCommandLineA);
    iat_try("GetEnvironmentStrings", my_GetEnvironmentStrings);
    iat_try("GetEnvironmentStringsA", my_GetEnvironmentStrings);
    iat_try("GetEnvironmentStringsW", my_GetEnvironmentStringsW);
    iat_try("FreeEnvironmentStringsA", my_FreeEnvironmentStringsA);
    iat_try("FreeEnvironmentStringsW", my_FreeEnvironmentStringsW);
    iat_override("GetStartupInfoA", my_GetStartupInfoA);
    iat_override("GetStdHandle", my_GetStdHandle);
    iat_override("GetTickCount", my_GetTickCount);
    iat_override("GetLocalTime", my_GetLocalTime);
    iat_override("GetSystemTime", my_GetSystemTime);
    iat_override("GetTimeZoneInformation", my_GetTimeZoneInformation);
    iat_override("Sleep", my_Sleep);
    iat_override("CreateMutexA", my_CreateMutexA);
    iat_override("MessageBoxA", my_MessageBoxA);
    iat_override("DialogBoxParamA", my_DialogBoxParamA);
    iat_override("RegisterClassExA", my_RegisterClassExA);
    iat_override("CreateWindowExA", my_CreateWindowExA);
    iat_override("ShowWindow", my_ShowWindow);
    iat_override("SetWindowPos", my_SetWindowPos);
    iat_override("SetForegroundWindow", my_SetForegroundWindow);
    iat_override("SetTimer", my_SetTimer);
    iat_override("KillTimer", my_KillTimer);
    iat_override("WaitMessage", my_WaitMessage);
    if (!O.no_arena) rh_heap_install();
    if (!O.no_overlay) rh_files_install();
    rh_handles_install();                            /* after files: wraps their overrides */
    rh_gdi_install();
    rh_dinput_install();
    rh_media_install();
    g_rh_flip_stub = make_stub(NULL, pre_flip, g_ed->flip);
    g_rh_easer_stub = make_stub(&g_ticks, NULL, g_ed->easer);
    int nf = redirect_calls(g_ed->flip, g_rh_flip_stub);
    int ne = redirect_calls(g_ed->easer, g_rh_easer_stub);
    int ns = redirect_calls(g_ed->srand, my_srand);
    if (!nf || !ne || ns != 1) rh_fatal("engine call sites flip=%d easer=%d srand=%d (want >0/>0/1)", nf, ne, ns);
    rh_log("hooks: flip %d sites, easer %d, srand %d\n", nf, ne, ns);
    if (!O.no_uninit_fix) rh_log("uninit-fix: %d site(s) patched (quirk #123)\n", uninit_fix_install());
    load_volatile();
    rh_trace_init();
    rh_hooks_install();
    if (O.coverage) cov_install();
    if (g_nw) AddVectoredExceptionHandler(1, watch_veh);
    if (g_ntrap) {
        AddVectoredExceptionHandler(1, trap_veh);
        for (int i = 0; i < g_ntrap; i++) { g_torig[i] = G8(g_tva[i]); G8(g_tva[i]) = 0xCC; }
    }
    rh_scrub_install();
    rh_scrub_iat();                                  /* last: wraps every IAT slot incl. our overrides */
    if (O.trace && !O.frames) O.frames = rh_trace_last_frame() + O.tail;
    rh_log("rhost: seed=%s%08x frames<=%u step=%ums creep_after=%u realtime=%d quiet=%d audible=%d arena=%d overlay=%d\n",
           O.has_seed ? "" : "none/", O.seed, O.frames, O.step_ms, O.creep_after, O.realtime, O.quiet, O.audible, !O.no_arena, !O.no_overlay);
    rh_dinput_frame(0);
    if (!O.record && O.stall_wall_ms) CloseHandle(CreateThread(NULL, 0, watchdog, NULL, 0, NULL));
    call_on_stack((void *)(uintptr_t)img.entry, NULL, MAIN_STACK_LO, MAIN_STACK_HI - 192);   /* CRT entry: never returns */
    finish("entry-returned");
    return 0;
}
