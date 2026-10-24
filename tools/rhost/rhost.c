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
 *   --standalone      NO retail code: only the data sections are mapped (the code range is INT3 + a jmp thunk into
 *                     the port at every replaced function), the port CRT (src/msvc_crt.c) is the entry; the hook
 *                     set defaults to the built-in one (g_sa_spec); <rundir>\standalone_audit.txt proves it
 *   --live            the real platform (clock, DirectInput, DirectSound, window, launcher, saves in --game) in place
 *                     of the deterministic stand-ins: play, not replay (with --standalone: the playable port);
 *                     --no-launcher skips the launcher dialog (Windowed + Launch, as every rhost run)
 *   --screen N        the emulated launcher's Screen Mode: 3 Windowed (default), 4 Fullscreen, 5 Zoom 1280x960
 *   --no-compat-layer (product) no self-relaunch under DWM8And16BitMitigation (then fullscreen fails as retail's does)
 * Output: <rundir>\rhost.log; last line "RESULT ..." (key=value, parsed by rhost.py). */
#include "rhost.h"
#include "msvc_crt.h"

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
    if (O.live) MessageBoxA(NULL, b, "OpenSummoners", MB_OK | MB_ICONERROR);
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
    /* the imports retail takes by ORDINAL, by their export name (port code asks by name: g_eng.api) */
    static const struct { const char *name, *dll; int ord; } ORD[] = { { "DirectSoundCreate", "DSOUND.dll", 1 } };
    for (IMAGE_IMPORT_DESCRIPTOR *d = imports(); d->Name; d++) {
        IMAGE_THUNK_DATA32 *ilt = (void *)(0x400000 + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        IMAGE_THUNK_DATA32 *iat = (void *)(0x400000 + d->FirstThunk);
        for (; ilt->u1.AddressOfData; ilt++, iat++) {
            if (!(ilt->u1.Ordinal & IMAGE_ORDINAL_FLAG32)) {
                if (!strcmp((char *)((IMAGE_IMPORT_BY_NAME *)(0x400000 + ilt->u1.AddressOfData))->Name, name))
                    return (uint32_t *)&iat->u1.Function;
                continue;
            }
            for (size_t k = 0; k < sizeof ORD / sizeof ORD[0]; k++)
                if (!strcmp(ORD[k].name, name) && !_stricmp((const char *)(0x400000 + d->Name), ORD[k].dll) &&
                    (int)(ilt->u1.Ordinal & 0xffff) == ORD[k].ord)
                    return (uint32_t *)&iat->u1.Function;
        }
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
/* the Flip's REPLACE (zd_5b8fc0): rhost's stub owns every Flip call site (pre_flip, then `jmp 0x5b8fc0`) — retarget
 * that jmp (pushad 60 / pushfd 9C / call E8 rel32 / popfd 9D / popad 61 / jmp E9 rel32: the jmp at +9) */
void rh_flip_retarget(void *fn)
{
    uint8_t *s = g_rh_flip_stub;
    if (!s || s[0] != 0x60 || s[9] != 0xE9) rh_fatal("flip stub: unexpected layout");
    *(int32_t *)(s + 10) = (int32_t)((uint32_t)(uintptr_t)fn - ((uint32_t)(uintptr_t)s + 14));
}
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
/* ---- the game's asset DLLs (sotesd / sotesw / sotesp, SE's sotesd_en / sotesx_*): resource-only for retail
 * (no GetProcAddress on them), all preferring 0x10000000 without ASLR, so the loader relocated them to wherever
 * rhost's own allocations left room: their HMODULEs (in .data and in resource records) moved with rhost's size
 * and with the hook set.  rhost loads, in load order, a COPY whose ImageBase is a fixed 16 MB slot of a range
 * reserved at startup (<game>\rhost-assets\, written once; the loader then needs no relocation).  Wine ignores
 * a requested base for SEC_IMAGE views, so a patched preferred base is the portable pin.  FreeLibrary keeps it
 * loaded (a reload returns the same handle). */
#define ASSET_LO 0x80000000u
#define ASSET_HI 0xb0000000u
static uint32_t g_asset_top;
static struct { char name[64]; uint32_t base; HMODULE h; } g_assets[16];
static int g_nassets;
static int asset_reserve(void)
{
    g_asset_top = ASSET_LO;
    return VirtualAlloc((void *)(uintptr_t)ASSET_LO, ASSET_HI - ASSET_LO, MEM_RESERVE, PAGE_NOACCESS) == (void *)(uintptr_t)ASSET_LO ? 0 : -1;
}
static const char *base_name(const char *p) { const char *b = p; for (; *p; p++) if (*p == '\\' || *p == '/') b = p + 1; return b; }
static HMODULE asset_find(const char *n)
{
    for (int i = 0; i < g_nassets; i++) if (!_stricmp(g_assets[i].name, base_name(n))) return g_assets[i].h;
    return NULL;
}
/* the PE header's ImageBase / SizeOfImage of a DLL file */
static int pe_base(const char *path, uint32_t *base, uint32_t *size, uint32_t *off)
{
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return -1;
    uint8_t h[4096]; DWORD got = 0;
    int ok = ReadFile(f, h, sizeof h, &got, NULL) && got >= 0x400;
    CloseHandle(f);
    if (!ok) return -1;
    uint32_t pe = *(uint32_t *)(h + 0x3c);
    if (pe + 24 + 60 > got || memcmp(h + pe, "PE\0\0", 4)) return -1;
    *off = pe + 24 + 28; *base = *(uint32_t *)(h + *off); *size = *(uint32_t *)(h + pe + 24 + 56);
    return 0;
}
/* rebase a DLL file in place (editbin /rebase): every HIGHLOW fixup + the ImageBase move by the delta.  Patched
 * through a file mapping (no host heap growth: the run that writes the copy keeps the same layout) */
static int asset_rebase(const char *path, uint32_t want)
{
    HANDLE f = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return -1;
    DWORD len = GetFileSize(f, NULL);
    HANDLE m = CreateFileMappingA(f, NULL, PAGE_READWRITE, 0, 0, NULL);
    uint8_t *b = m ? MapViewOfFile(m, FILE_MAP_WRITE, 0, 0, 0) : NULL;
    int rc = -1;
    if (b && len > 0x400) {
        IMAGE_NT_HEADERS32 *nt = (void *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
        IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
        uint32_t delta = want - nt->OptionalHeader.ImageBase, nsec = nt->FileHeader.NumberOfSections;
        IMAGE_DATA_DIRECTORY rd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        #define RVA2OFF(rva, out) do { (out) = 0; for (uint32_t k_ = 0; k_ < nsec; k_++)                          \
            if ((rva) >= sec[k_].VirtualAddress && (rva) < sec[k_].VirtualAddress + sec[k_].SizeOfRawData)           \
                { (out) = sec[k_].PointerToRawData + (rva) - sec[k_].VirtualAddress; break; } } while (0)
        uint32_t ro; RVA2OFF(rd.VirtualAddress, ro);
        rc = rd.Size && ro ? 0 : -1;
        for (uint32_t at = 0; !rc && at + 8 <= rd.Size; ) {
            IMAGE_BASE_RELOCATION *br = (void *)(b + ro + at);
            if (br->SizeOfBlock < 8) break;
            uint16_t *e = (uint16_t *)(br + 1);
            for (uint32_t i = 0; i < (br->SizeOfBlock - 8) / 2; i++) {
                uint32_t t = e[i] >> 12, fo;
                if (t == IMAGE_REL_BASED_ABSOLUTE) continue;
                if (t != IMAGE_REL_BASED_HIGHLOW) { rc = -1; break; }
                RVA2OFF(br->VirtualAddress + (e[i] & 0xfffu), fo);
                if (!fo || fo + 4 > len) { rc = -1; break; }
                *(uint32_t *)(b + fo) += delta;
            }
            at += br->SizeOfBlock;
        }
        #undef RVA2OFF
        if (!rc) nt->OptionalHeader.ImageBase = want;
    }
    if (b) UnmapViewOfFile(b);
    if (m) CloseHandle(m);
    CloseHandle(f);
    return rc;
}
/* ---- --standalone: the audit of every byte taken from the game's files ---- */
static char g_audit[MAX_PATH];
static void sa_audit(const char *fmt, ...)
{
    if (!g_audit[0]) return;
    FILE *f = fopen(g_audit, "a");
    if (!f) return;
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fclose(f);
}
static void sa_audit_ranges(const char *who, const retail_image_t *im)
{
    for (int i = 0; i < im->nr; i++) {
        const retail_range_t *r = &im->r[i];
        sa_audit("range %s %-8s %08x %08x ch=%08x %s\n", who, r->name, r->va, r->len, r->ch, r->copied ? "copied" : "CODE-not-copied");
    }
}
/* a PE file's headers + data sections into `base` through a read-only file view (retail_copy_data) */
static int map_file_data(const char *path, uint8_t *base, retail_image_t *rec)
{
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return -1;
    HANDLE m = CreateFileMappingA(f, NULL, PAGE_READONLY, 0, 0, NULL);
    const uint8_t *v = m ? MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0) : NULL;
    if (v) retail_copy_data(v, base, rec, 0xCC);
    if (v) UnmapViewOfFile(v);
    if (m) CloseHandle(m);
    CloseHandle(f);
    return v ? 0 : -1;
}
static HMODULE WINAPI my_LoadLibraryA(LPCSTR n)
{
    char src[MAX_PATH], dir[MAX_PATH], dst[MAX_PATH], tmp[MAX_PATH];
    if (!n) return LoadLibraryA(n);
    HMODULE h = asset_find(n);
    if (h) return h;
    snprintf(src, sizeof src, "%s\\%s", g_gamedir_abs, base_name(n));
    uint32_t base, size, off;
    if (g_nassets == (int)(sizeof g_assets / sizeof *g_assets) || pe_base(src, &base, &size, &off))
        return LoadLibraryA(n);   /* a system DLL */
    uint32_t want = g_asset_top, next = (want + size + 0xffffffu) & ~0xffffffu;   /* 16 MB slots */
    if (next >= ASSET_HI) rh_fatal("asset %s: %x bytes do not fit the slot range", src, size);
    if (O.standalone) {   /* its data sections at the slot, from a read-only view of the file: no DllMain, no code */
        if (!VirtualFree((void *)(uintptr_t)g_asset_top, 0, MEM_RELEASE)) rh_fatal("asset %s: cannot release the slot range", src);
        uint8_t *base = VirtualAlloc((void *)(uintptr_t)want, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (base != (uint8_t *)(uintptr_t)want) rh_fatal("asset %s: cannot map the slot %08x (%lu)", src, want, GetLastError());
        retail_image_t rec;
        if (map_file_data(src, base, &rec)) rh_fatal("asset %s: cannot read", src);
        DWORD old; VirtualProtect(base, size, PAGE_READONLY, &old);   /* as the loader maps .rsrc */
        if (!VirtualAlloc((void *)(uintptr_t)next, ASSET_HI - next, MEM_RESERVE, PAGE_NOACCESS))
            rh_fatal("asset %s: cannot re-reserve %08x..%08x", src, next, ASSET_HI);
        sa_audit_ranges(base_name(n), &rec);
        snprintf(g_assets[g_nassets].name, sizeof g_assets[0].name, "%s", base_name(n));
        g_assets[g_nassets].base = want; g_assets[g_nassets++].h = (HMODULE)base;
        rh_log("asset: %s data sections at %08x+%x (no code)\n", base_name(n), want, size);
        g_asset_top = next;
        return (HMODULE)base;
    }
    snprintf(dir, sizeof dir, "%s\\rhost-assets", g_gamedir_abs);
    snprintf(dst, sizeof dst, "%s\\%s", dir, base_name(n));
    uint32_t have = 0, hs, ho;
    if (pe_base(dst, &have, &hs, &ho) || have != want) {   /* (re)write the copy: ImageBase = the slot */
        CreateDirectoryA(dir, NULL);
        snprintf(tmp, sizeof tmp, "%s.%lu.tmp", dst, GetCurrentProcessId());
        if (!CopyFileA(src, tmp, FALSE)) rh_fatal("asset %s: copy failed (%lu)", src, GetLastError());
        if (asset_rebase(tmp, want)) rh_fatal("asset %s: cannot rebase the copy", tmp);
        if (!MoveFileExA(tmp, dst, MOVEFILE_REPLACE_EXISTING)) {   /* a parallel run may have won the race */
            DeleteFileA(tmp);
            if (pe_base(dst, &have, &hs, &ho) || have != want) rh_fatal("asset %s: cannot install the copy (%lu)", dst, GetLastError());
        }
        rh_log("asset: wrote %s (ImageBase %08x)\n", dst, want);
    }
    if (!VirtualFree((void *)(uintptr_t)g_asset_top, 0, MEM_RELEASE)) rh_fatal("asset %s: cannot release the slot range", dst);
    h = LoadLibraryA(dst);
    if (!VirtualAlloc((void *)(uintptr_t)next, ASSET_HI - next, MEM_RESERVE, PAGE_NOACCESS))
        rh_fatal("asset %s: cannot re-reserve %08x..%08x", dst, next, ASSET_HI);
    if ((uint32_t)(uintptr_t)h != want) rh_fatal("asset %s: loaded at %p, not the slot %08x (%lu)", dst, (void *)h, want, GetLastError());
    snprintf(g_assets[g_nassets].name, sizeof g_assets[0].name, "%s", base_name(n));
    g_assets[g_nassets].base = want; g_assets[g_nassets++].h = h;
    rh_log("asset: %s loaded at %08x+%x (fixed slot)\n", base_name(n), want, size);
    g_asset_top = next;
    return h;
}
int rh_asset_in(uint32_t v) { return v >= ASSET_LO && v < ASSET_HI; }
static BOOL WINAPI my_FreeLibrary(HMODULE h)
{
    uint32_t v = (uint32_t)(uintptr_t)h;
    if (v >= ASSET_LO && v < ASSET_HI) return TRUE;   /* kept mapped: a reload gets the same handle */
    return FreeLibrary(h);
}
static HMODULE WINAPI my_GetModuleHandleA(LPCSTR n)
{
    if (!n) return (HMODULE)0x400000;
    HMODULE h = asset_find(n);
    return h ? h : GetModuleHandleA(n);
}
static DWORD WINAPI my_GetModuleFileNameA(HMODULE h, LPSTR buf, DWORD n)
{
    if (!h || h == (HMODULE)0x400000) { snprintf(buf, n, "%s\\sotes.exe", g_gamedir_abs); return (DWORD)strlen(buf); }
    for (int i = 0; i < g_nassets; i++)   /* the asset copies report the original file */
        if (h == g_assets[i].h) { snprintf(buf, n, "%s\\%s", g_gamedir_abs, g_assets[i].name); return (DWORD)strlen(buf); }
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
    G32(g_ed->cfg_screen) = (uint32_t)O.screen | ((uint32_t)vram << 16);   /* 3 Windowed (--screen) */
    G32(g_ed->cfg_sound) = slo | ((uint32_t)shi << 16);
    G32(g_ed->launch_flag) = 1;
    rh_log("launcher: emulated Windowed+Launch (screen %08x -> %08x, sound %08x -> %08x)\n", scr, G32(g_ed->cfg_screen), snd, G32(g_ed->cfg_sound));
    return 0;
}

/* --live: the real launcher from the mapped image's template 0x2711 (the port's DLGPROC 0x4013c0 through its thunk);
 * should the system refuse the hand-mapped module, the emulated Windowed+Launch */
static INT_PTR WINAPI live_DialogBoxParamA(HINSTANCE hi, LPCSTR tmpl, HWND parent, DLGPROC proc, LPARAM lp)
{
    INT_PTR r = DialogBoxParamA(hi, tmpl, parent, proc, lp);
    if (r == -1 && (uint32_t)(uintptr_t)proc == g_ed->dlgproc) {
        rh_log("launcher: DialogBoxParamA failed (%lu) — emulating Windowed+Launch\n", GetLastError());
        return my_DialogBoxParamA(hi, tmpl, parent, proc, lp);
    }
    return r;
}

/* --live: real DirectInput validates its HINSTANCE (dinput's hresValidInstanceVer: GetModuleFileName) — the hand-mapped
 * 0x400000 is no loader module → 0x80070126 "The specified module could not be found" → retail's exit(0) "It failed in
 * the initialization of ZDI" (0x562ea0).  wine's dinput does not check.  The host exe's own handle stands in. */
typedef HRESULT (WINAPI *dice_t)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
static dice_t g_real_dice;
static HRESULT WINAPI live_DirectInputCreateEx(HINSTANCE hi, DWORD ver, REFIID iid, LPVOID *out, LPUNKNOWN outer)
{
    if (hi == (HINSTANCE)0x400000) hi = GetModuleHandleA(NULL);
    HRESULT r = g_real_dice(hi, ver, iid, out, outer);
    if (r) rh_log("DirectInputCreateEx(%p, %#lx) failed %08lx\n", (void *)hi, ver, (unsigned long)r);
    return r;
}

/* --live: the game's own diagnostics (ZDD/ZDI error text 0x5b80d0 → OutputDebugStringA; fatal boxes) into rhost.log too */
static void WINAPI live_OutputDebugStringA(LPCSTR s) { rh_log("ods: %s\n", s ? s : ""); OutputDebugStringA(s); }
static int WINAPI live_MessageBoxA(HWND h, LPCSTR text, LPCSTR cap, UINT t)
{ rh_log("MessageBoxA [%s] %s (frame %u)\n", cap ? cap : "", text ? text : "", g_frame); return MessageBoxA(h, text, cap, t); }

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
static WNDPROC g_game_proc;   /* the game window class's own procedure (retail's 0x5b12e0 or the port's) */
static ATOM WINAPI my_RegisterClassExA(const WNDCLASSEXA *wc)
{
    if (g_nclass >= 4) rh_fatal("more than 4 window classes");
    WNDCLASSEXA c = *wc;
    g_wndproc_orig[g_nclass] = c.lpfnWndProc;
    if ((uint32_t)(uintptr_t)c.lpfnWndProc == g_ed->wndproc ||   /* the game window's class: retail's procedure, or the
                                                                   * port's (a ported WinMain registers its own) */
        (HIWORD((uintptr_t)c.lpszClassName) && !strcmp(c.lpszClassName, "CLASS_LIZSOFT_SOTES")))
        g_game_proc = c.lpfnWndProc;
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
    if (orig && orig == g_game_proc && !g_activated) {
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
            int inwin = !O.trap_f1 || (g_frame >= O.trap_f0 && g_frame <= O.trap_f1);
            if (inwin && g_thits[i]++ < O.trap_max) {
                const uint32_t *sp = (const uint32_t *)(uintptr_t)c->Esp;   /* [esp] = return address, then the stack args */
                rh_log("trap %08x f%u ret=%08x eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx"
                       " args %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", a, g_frame,
                       sp[0], c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp,
                       sp[1], sp[2], sp[3], sp[4], sp[5], sp[6], sp[7], sp[8], sp[9], sp[10], sp[11]);
            }
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
typedef struct { uint32_t va, size, tail; } frame_zero_t;   /* tail: bytes after a short `sub esp,imm8` moved too */
typedef struct { const char *tag; uint32_t palcopy, a[4]; uint32_t b, b_resume; uint32_t c, c_off, c_pad, c_resume; uint32_t sbh_alloc;
                 frame_zero_t e[4]; } uninit_t;
/* site E (quirk #161): a function's `sub esp,N` (81 EC imm32, or 83 EC imm8 + `tail` bytes of the next PIC
 * instructions, moved into the stub) → the same + zero the N-byte frame.  0x47ecd0 (the AI's skill pick) writes
 * its never-set [ebp-4] through an out pointer when no record is found (a stale stack address; quirk #168).  The save writer
 * 0x41a890 writes 520 never-filled header bytes from its frame into every snapshot / save file (a real HDC left
 * there by an earlier GetDC caller ⇒ different every run: rh-combat's game-over retry). */
static const uninit_t UNINIT[] = {
    { "enold",   0x491770u, { 0x586f79u }, 0x4181dfu, 0x4181e6u, 0x440dd1u, 0x14854u, 0x42u, 0x440dd7u, 0x5c73fdu,
      { { 0x41a8a5u, 0xb90u, 0 }, { 0x47ecd0u, 0x54u, 4 } } },
    { "ense-en", 0x48c300u, { 0x5ad66bu, 0x5ad95bu }, 0x41233fu, 0x412346u, 0x43bcccu, 0x14874u, 0x46u, 0x43bcd2u, 0x5f7afau,
      { { 0x4155e5u, 0xba0u, 0 }, { 0x47b7d0u, 0x54u, 4 } } },
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
    for (int k = 0; k < 4 && u->e[k].va; k++) {             /* site E: zero a whole stack frame at entry */
        uint8_t *q = (uint8_t *)(uintptr_t)u->e[k].va;
        int lng = q[0] == 0x81 && q[1] == 0xEC && *(uint32_t *)(q + 2) == u->e[k].size && !u->e[k].tail;
        int sht = q[0] == 0x83 && q[1] == 0xEC && q[2] == u->e[k].size && u->e[k].tail >= 2;
        if (!lng && !sht) { rh_log("uninit-fix: site E %08x mismatch\n", u->e[k].va); continue; }
        uint32_t sub = lng ? 6 : 3;
        uint8_t *c = g_stub_top, *o = c;
        memcpy(o, q, sub); o += sub;                                 /* sub esp,N */
        *o++ = 0x60; *o++ = 0x9C;                                    /* pushad; pushfd */
        *o++ = 0x8D; *o++ = 0x7C; *o++ = 0x24; *o++ = 36;            /* lea edi,[esp+36] */
        *o++ = 0xB9; *(uint32_t *)o = u->e[k].size / 4; o += 4;      /* mov ecx,N/4 */
        *o++ = 0x31; *o++ = 0xC0;                                    /* xor eax,eax */
        *o++ = 0xFC; *o++ = 0xF3; *o++ = 0xAB;                       /* cld; rep stosd */
        *o++ = 0x9D; *o++ = 0x61;                                    /* popfd; popad */
        memcpy(o, q + sub, u->e[k].tail); o += u->e[k].tail;         /* the moved instruction(s) */
        uint32_t back = sub + u->e[k].tail;
        *o++ = 0xE9; *(int32_t *)o = (int32_t)(u->e[k].va + back - ((uint32_t)(uintptr_t)o + 4)); o += 4;
        g_stub_top = (uint8_t *)(((uintptr_t)o + 15) & ~(uintptr_t)15);
        q[0] = 0xE9; *(int32_t *)(q + 1) = (int32_t)((uint32_t)(uintptr_t)c - (u->e[k].va + 5));
        for (uint32_t z = 5; z < back; z++) q[z] = 0x90;
        n++;
    }
    return n;
}

/* ---- engine hooks ---- */
typedef void (__cdecl *srand_t)(unsigned);
void __cdecl my_srand(unsigned s)
{
    rh_log("srand(%08x) -> pinned %08x (frame %u)\n", s, O.has_seed ? O.seed : s, g_frame);
    if (O.standalone) crt_srand(O.has_seed ? O.seed : s);   /* the port CRT's (retail's srand is not mapped) */
    else ((srand_t)(uintptr_t)g_ed->srand)(O.has_seed ? O.seed : s);
}

static void finish(const char *why);
void rh_finish(const char *why) { finish(why); }
static LARGE_INTEGER g_t0, g_qf;
/* the address-space map, one line per allocation (base, size, type, committed KB): two runs' maps diff to the
 * first allocation whose placement differs (what shifted a system DLL's heap) */
static void vmmap_log(void)
{
    uint32_t a = 0x10000u;
    rh_log("vmmap f%u:\n", g_frame);
    while (a < 0xfffe0000u) {
        MEMORY_BASIC_INFORMATION mi;
        if (!VirtualQuery((void *)(uintptr_t)a, &mi, sizeof mi)) break;
        uint32_t lo = (uint32_t)(uintptr_t)mi.BaseAddress, sz = (uint32_t)mi.RegionSize;
        if (mi.State != MEM_FREE) {
            uint32_t ab = (uint32_t)(uintptr_t)mi.AllocationBase, end = lo + sz, com = mi.State == MEM_COMMIT ? sz : 0;
            for (;;) {   /* coalesce the allocation's regions */
                MEMORY_BASIC_INFORMATION m2;
                if (end >= 0xfffe0000u || !VirtualQuery((void *)(uintptr_t)end, &m2, sizeof m2) ||
                    (uint32_t)(uintptr_t)m2.AllocationBase != ab || m2.State == MEM_FREE) break;
                if (m2.State == MEM_COMMIT) com += (uint32_t)m2.RegionSize;
                end += (uint32_t)m2.RegionSize;
            }
            rh_log("  vm %08x..%08x %-7s %6uK committed\n", ab, end,
                   mi.Type == MEM_IMAGE ? "image" : mi.Type == MEM_MAPPED ? "mapped" : "private", com >> 10);
            a = end;
        } else a = lo + sz;
        if (a <= lo) break;
    }
}
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
static uint64_t g_fb_chain = 0xcbf29ce484222325ull; static uint32_t g_fb_n;   /* --fb-hash */
/* every access violation: where, the registers, the ebp chain (first 8; the run's own handling continues) */
static LONG CALLBACK crash_veh(EXCEPTION_POINTERS *e)
{
    static int n;
    if (e->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || n++ >= 8) return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT *c = e->ContextRecord;
    char b[160]; int o = 0; uint32_t *fp = (uint32_t *)(uintptr_t)c->Ebp;
    uintptr_t lo = c->Esp, hi = c->Esp + 0x100000;   /* the frame walk stays on this stack (no probing reads) */
    for (int i = 0; i < 8 && (uintptr_t)fp >= lo && (uintptr_t)fp + 8 <= hi && !((uintptr_t)fp & 3); i++, fp = (uint32_t *)(uintptr_t)fp[0])
        o += snprintf(b + o, sizeof b - o, " %08x", fp[1]);
    rh_log("AV f%u at %08x (%s %08lx) eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx esp=%08lx ebp=%08lx callers%s\n",
           g_frame, (unsigned)(uintptr_t)e->ExceptionRecord->ExceptionAddress,
           e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read", (unsigned long)e->ExceptionRecord->ExceptionInformation[1],
           c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Esp, c->Ebp, b);
    return EXCEPTION_CONTINUE_SEARCH;
}

/* runs at the ENTRY of every Flip (the frame's back buffer is complete): frame g_frame ends */
static void __cdecl pre_flip(void)
{
    if (g_finished) return;
    if (g_nw) watch_arm_due();
    char t[32]; snprintf(t, sizeof t, "f%06u", g_frame);
    if (is_dump_frame(g_frame)) dump_now(t);
    for (int i = 0; i < O.n_vmmap; i++) if (O.vmmap_at[i] == g_frame) vmmap_log();
    for (int i = 0; i < O.n_shot; i++) if (O.shot_at[i] == g_frame) rh_shot(t);
    if (O.shot_every && g_frame % O.shot_every == 0) rh_shot(t);
    if (O.log_every && g_frame % O.log_every == 0) {
        uint16_t cw; __asm__ volatile("fnstcw %0" : "=m"(cw));
        char wb[256]; watch_str(wb, sizeof wb);
        char gb[128]; rh_gdi_stats(gb, sizeof gb);
        rh_log("frame %u tick=%u vclock=%u rng=%08x cw=%04x creep=%u %s%s\n", g_frame, g_ticks, g_vclock,
               G32(g_ed->rng_seed), cw, g_creep_total, gb, wb);
    }
    if (O.fb_hash) {
        uint64_t fh;
        if (rh_fb_hash(&fh)) { g_fb_chain = (g_fb_chain ^ fh) * 0x100000001b3ull; g_fb_n++; }
    }
    if (O.hash_every && g_frame % O.hash_every == 0) {
        rh_hash_t h; rh_state_hash(&h);
        char fb[48] = "";
        if (O.fb_hash) snprintf(fb, sizeof fb, " fb=%016llx", (unsigned long long)g_fb_chain);
        rh_log("hash f%u full=%016llx sem=%016llx masked=%u semv=%016llx%s\n", g_frame, (unsigned long long)h.full,
               (unsigned long long)h.semantic, h.masked, (unsigned long long)h.semv, fb);
    }
    if (g_frame >= O.frames) finish("frames");
    g_frame++;
    g_vclock += O.step_ms; g_calls_since_flip = 0; g_waits_since_flip = 0;
    if (!O.live) rh_dinput_frame(g_frame);
    if (O.realtime) {
        LARGE_INTEGER q; QueryPerformanceCounter(&q);
        double due = g_frame * (1000.0 / 60.0), now = (q.QuadPart - g_t0.QuadPart) * 1000.0 / g_qf.QuadPart;
        if (due > now + 1) Sleep((DWORD)(due - now));
    }
    if (!O.live) rh_scrub_here();
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
    rh_log("%s sound_pool=%uKB\n", hs, rh_media_pool_used() >> 10);
    if (O.log_ext) rh_state_ext_report();
    rh_hash_t h; rh_state_hash(&h);
    LARGE_INTEGER q; QueryPerformanceCounter(&q);
    double ms = (q.QuadPart - g_t0.QuadPart) * 1000.0 / g_qf.QuadPart;
    char fb[64] = "";
    if (O.fb_hash) snprintf(fb, sizeof fb, " fb=%016llx fb_frames=%u", (unsigned long long)g_fb_chain, g_fb_n);
    rh_log("RESULT status=%s reason=%s frame=%u tick=%u rng=%08x state_full=%016llx state_sem=%016llx state_semv=%016llx masked=%u state_bytes=%u wall_ms=%.0f fps=%.0f creep=%u %s%s\n",
           !strcmp(why, "fatal") ? "fatal" : !strcmp(why, "game-exit") ? "exit" : "ok", why, g_frame, g_ticks, G32(g_ed->rng_seed), (unsigned long long)h.full,
           (unsigned long long)h.semantic, (unsigned long long)h.semv, h.masked, h.bytes, ms, ms > 0 ? g_frame * 1000.0 / ms : 0.0, g_creep_total, rh_trace_status(), fb);
    if (O.stall_ms) Sleep(O.stall_ms);
    if (g_logf) fclose(g_logf);
    ExitProcess(0);
}
static VOID WINAPI my_ExitProcess(UINT code) { if (!g_finished) { rh_log("retail ExitProcess(%u) at frame %u\n", code, g_frame); finish("game-exit"); } ExitProcess(code); }

/* ---- --standalone: the trap for retail code, the seal, the audit ---- */
static LONG CALLBACK sa_veh(EXCEPTION_POINTERS *e)
{
    uint32_t code = e->ExceptionRecord->ExceptionCode, at = (uint32_t)(uintptr_t)e->ExceptionRecord->ExceptionAddress;
    if (code == EXCEPTION_BREAKPOINT && at >= RT_TEXT_LO && at < RT_TEXT_LO + RT_TEXT_SZ)
        rh_fatal("standalone: the game reached retail code at %08x (no port for it yet; return address %08x) frame %u",
                 at, *(uint32_t *)(uintptr_t)e->ContextRecord->Esp, g_frame);
    if (code == EXCEPTION_ACCESS_VIOLATION && e->ExceptionRecord->ExceptionInformation[0] == 8 &&
        ((at >= RT_IMG_LO && at < RT_IMG_HI) || rh_asset_in(at)))
        rh_fatal("standalone: execution of mapped game DATA at %08x (DEP) frame %u", at, g_frame);
    return EXCEPTION_CONTINUE_SEARCH;
}
/* the image is sealed before the first instruction of port code runs: the code range (INT3 + the thunks) execute /
 * read, everything taken from the file read / write and never executable; then the audit re-derives every claim from
 * memory and from the file, and a failed audit refuses to run */
static void sa_seal(const char *exe_abs, const retail_image_t *img)
{
    DWORD old;
    uint32_t map_hi = RT_IMG_LO + img->size;   /* (a packed exe's .bind lies past RT_IMG_HI) */
    if (!VirtualProtect((void *)(uintptr_t)RT_IMG_LO, map_hi - RT_IMG_LO, PAGE_READWRITE, &old) ||
        !VirtualProtect((void *)(uintptr_t)RT_TEXT_LO, RT_TEXT_SZ, PAGE_EXECUTE_READ, &old))
        rh_fatal("standalone: cannot seal the image (%lu)", GetLastError());
    typedef BOOL (WINAPI *sdep_t)(DWORD);
    typedef BOOL (WINAPI *gdep_t)(HANDLE, LPDWORD, PBOOL);
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    sdep_t sdep = (sdep_t)(void *)GetProcAddress(k32, "SetProcessDEPPolicy");
    gdep_t gdep = (gdep_t)(void *)GetProcAddress(k32, "GetProcessDEPPolicy");
    BOOL set = sdep ? sdep(1 /* PROCESS_DEP_ENABLE */) : FALSE;
    DWORD depf = 0; BOOL perm = FALSE;
    if (gdep) gdep(GetCurrentProcess(), &depf, &perm);

    snprintf(g_audit, sizeof g_audit, "%s\\standalone_audit.txt", O.rundir);
    DeleteFileA(g_audit);
    sa_audit("# OpenSummoners standalone audit: every byte taken from the game's files, and the code range\n");
    sa_audit("exe %s image %08x..%08x edition %s\n", exe_abs, RT_IMG_LO, RT_IMG_HI, g_ed->tag);
    sa_audit_ranges("sotes.exe", img);
    int bad = 0;
    for (int i = 0; i < img->nr; i++)
        if (img->r[i].copied && RETAIL_SEC_CODE(img->r[i].ch)) { sa_audit("FAIL copied a code section %s\n", img->r[i].name); bad++; }
    /* the code range: INT3 everywhere but the thunks */
    const rh_thunk_t *t; int nt_ = rh_hooks_thunks(&t);
    uint8_t *mark = VirtualAlloc(NULL, RT_TEXT_SZ, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    for (int i = 0; i < nt_; i++) {
        const uint8_t *e = (const uint8_t *)(uintptr_t)t[i].va;
        int32_t rel = (int32_t)((uint32_t)(uintptr_t)t[i].target - (t[i].va + 5));
        if (e[0] != 0xE9 || *(const int32_t *)(e + 1) != rel) { sa_audit("FAIL thunk %08x %s altered\n", t[i].va, t[i].name); bad++; }
        for (int k = 0; k < 5; k++) mark[t[i].va - RT_TEXT_LO + k] = 1;
        sa_audit("thunk %08x -> %08x %s\n", t[i].va, (uint32_t)(uintptr_t)t[i].target, t[i].name);
    }
    uint32_t int3 = 0, other = 0, notcopied = 0;
    for (int r = 0; r < img->nr; r++) {   /* every section not copied (.text; a packed exe's .bind): INT3 but the thunks */
        if (img->r[r].copied) continue;
        const uint8_t *code = (const uint8_t *)(uintptr_t)img->r[r].va;
        notcopied += img->r[r].len;
        for (uint32_t i = 0; i < img->r[r].len; i++) {
            uint32_t va = img->r[r].va + i;
            if (va >= RT_TEXT_LO && va < RT_TEXT_LO + RT_TEXT_SZ && mark[va - RT_TEXT_LO]) continue;
            if (code[i] == 0xCC) int3++;
            else if (other++ < 16) sa_audit("FAIL code byte %08x = %02x\n", va, code[i]);
        }
    }
    VirtualFree(mark, 0, MEM_RELEASE);
    bad += other != 0;
    /* against the file: no 16-byte window of retail's .text (all-INT3 windows aside) is present */
    uint32_t windows = 0, same = 0;
    HANDLE f = CreateFileA(exe_abs, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE m = f != INVALID_HANDLE_VALUE ? CreateFileMappingA(f, NULL, PAGE_READONLY, 0, 0, NULL) : NULL;
    const uint8_t *v = m ? MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0) : NULL;
    if (v) {
        IMAGE_NT_HEADERS32 *fn = (void *)(v + ((IMAGE_DOS_HEADER *)v)->e_lfanew);
        IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(fn);
        for (int i = 0; i < fn->FileHeader.NumberOfSections; i++) {
            if (!RETAIL_SEC_CODE(sec[i].Characteristics)) continue;
            const uint8_t *ft = v + sec[i].PointerToRawData, *mt = (const uint8_t *)(uintptr_t)(0x400000u + sec[i].VirtualAddress);
            uint32_t n = sec[i].SizeOfRawData < sec[i].Misc.VirtualSize ? sec[i].SizeOfRawData : sec[i].Misc.VirtualSize;
            for (uint32_t o = 0; o + 16 <= n; o += 16) {
                int all_cc = 1;
                for (int k = 0; k < 16; k++) if (mt[o + k] != 0xCC) { all_cc = 0; break; }
                if (all_cc) continue;
                windows++;
                if (!memcmp(mt + o, ft + o, 16)) same++;
            }
        }
        UnmapViewOfFile(v);
    } else { sa_audit("FAIL cannot re-read %s\n", exe_abs); bad++; }
    if (m) CloseHandle(m);
    if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
    bad += same != 0;
    sa_audit("code_range %08x..%08x bytes %u (all sections not copied: %u) thunks %d (%d bytes) int3 %u other %u\n",
             RT_TEXT_LO, RT_TEXT_LO + RT_TEXT_SZ, RT_TEXT_SZ, notcopied, nt_, 5 * nt_, int3, other);
    sa_audit("retail_text_windows_compared %u identical %u\n", windows, same);
    /* the protections: nothing executable in the image but the code range */
    uint32_t a = RT_IMG_LO, xout = 0;
    while (a < map_hi) {
        MEMORY_BASIC_INFORMATION mi;
        if (!VirtualQuery((void *)(uintptr_t)a, &mi, sizeof mi)) break;
        uint32_t lo = (uint32_t)(uintptr_t)mi.BaseAddress, hi = lo + (uint32_t)mi.RegionSize;
        if (hi > map_hi) hi = map_hi;
        int x = (mi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
        sa_audit("protect %08x..%08x %04lx%s\n", lo, hi, (unsigned long)mi.Protect, x ? " exec" : "");
        if (x && (lo < RT_TEXT_LO || hi > ((RT_TEXT_LO + RT_TEXT_SZ + 0xfffu) & ~0xfffu))) xout++;
        a = hi;
    }
    bad += xout != 0;
    sa_audit("exec_outside_code_range %u\n", xout);
    sa_audit("dep set=%d flags=%lx permanent=%d\n", set, (unsigned long)depf, perm);
    sa_audit("verdict %s\n", bad ? "FAIL" : "OK");
    rh_log("standalone: %d thunks, code range %u bytes (%u INT3, %u other), retail .text windows identical %u/%u, "
           "exec outside the code range %u, DEP %s — audit %s (%s)\n", nt_, RT_TEXT_SZ, int3, other, same, windows, xout,
           depf & 1 ? "on" : "off", bad ? "FAIL" : "OK", g_audit);
    if (bad) rh_fatal("standalone: the audit failed (%s)", g_audit);
}

/* ---- CLI ---- */
static void usage(void)
{
    fprintf(stderr, "usage: rhost --exe RETAIL --game DIR --run DIR [--standalone] [--live] [--frames N] [--seed S|--no-seed] [--trace F]\n"
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
    O.step_ms = 16; O.creep_after = 2000; O.stall_wall_ms = 60000; O.scrub_kb = 32; O.trap_max = 50; O.screen = 3;
    int seed_given = 0;
#ifdef RH_PRODUCT
    O.standalone = O.live = 1;   /* the product: the port alone, on the real platform */
#endif
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        #define ARG() (i + 1 < argc ? argv[++i] : (usage(), (char *)0))
        if (!strcmp(a, "--exe")) O.exe = ARG();
        else if (!strcmp(a, "--game")) O.gamedir = ARG();
        else if (!strcmp(a, "--run")) O.rundir = ARG();
        else if (!strcmp(a, "--edition")) O.edition = ARG();
        else if (!strcmp(a, "--frames")) O.frames = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--seed")) { O.seed = strtoul(ARG(), 0, 0); O.has_seed = 1; seed_given = 1; }
        else if (!strcmp(a, "--standalone")) O.standalone = 1;
        else if (!strcmp(a, "--live")) O.live = 1;
        else if (!strcmp(a, "--no-launcher")) O.no_launcher = 1;
        else if (!strcmp(a, "--screen")) O.screen = (int)strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--no-compat-layer")) O.no_compat_layer = 1;
        else if (!strcmp(a, "--no-seed")) O.has_seed = 0;
        else if (!strcmp(a, "--trace")) O.trace = ARG();
        else if (!strcmp(a, "--record")) { O.record = ARG(); O.realtime = 1; O.quiet = 0; O.audible = 1; }
        else if (!strcmp(a, "--realtime")) O.realtime = 1;
        else if (!strcmp(a, "--dump-at")) parse_list(ARG(), O.dump_at, &O.n_dump, 128);
        else if (!strcmp(a, "--vmmap-at")) parse_list(ARG(), O.vmmap_at, &O.n_vmmap, 16);
        else if (!strcmp(a, "--bgm-fail")) O.bgm_fail = (int)strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--shot-at")) parse_list(ARG(), O.shot_at, &O.n_shot, 64);
        else if (!strcmp(a, "--shot-every")) O.shot_every = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--fb-hash")) O.fb_hash = 1;
        else if (!strcmp(a, "--hash-every")) O.hash_every = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--log-every")) O.log_every = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--tail")) O.tail = strtoul(ARG(), 0, 0);
        else if (!strcmp(a, "--hook")) O.hooks = ARG();   /* "@path": the spec from a file (read in rh_hooks_install) */
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
        else if (!strcmp(a, "--heap-trace")) { const char *v = ARG(); O.htrace_f0 = strtoul(v, 0, 0); const char *d = strchr(v, '-'); O.htrace_f1 = d ? strtoul(d + 1, 0, 0) : O.htrace_f0; }
        else if (!strcmp(a, "--trap-frames")) { const char *v = ARG(); O.trap_f0 = strtoul(v, 0, 0); const char *d = strchr(v, '-'); O.trap_f1 = d ? strtoul(d + 1, 0, 0) : O.trap_f0; }
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
#ifdef RH_PRODUCT
    /* defaults: the game is the folder this exe sits in; the retail exe the unpacked one if present */
    static char gdir[MAX_PATH], exe[MAX_PATH], run[MAX_PATH];
    if (!O.gamedir) {
        GetModuleFileNameA(NULL, gdir, sizeof gdir);
        char *sl = strrchr(gdir, '\\'); if (sl) *sl = 0;
        O.gamedir = gdir;
    }
    if (!O.exe) {
        snprintf(exe, sizeof exe, "%s\\sotes.unpacked.exe", O.gamedir);
        if (GetFileAttributesA(exe) == INVALID_FILE_ATTRIBUTES) snprintf(exe, sizeof exe, "%s\\sotes.exe", O.gamedir);
        O.exe = exe;
    }
    if (!O.rundir) { snprintf(run, sizeof run, "%s\\opensummoners-standalone", O.gamedir); O.rundir = run; }
#endif
    if (!O.exe || !O.gamedir || !O.rundir) usage();
    if (O.live) {   /* a person plays: visible, audible, unpinned (retail seeds srand(time(0))), unbounded */
        O.quiet = 0; O.audible = 1; O.realtime = 0;
        if (!seed_given) O.has_seed = 0;
        if (!O.frames) O.frames = 0xfffffff0u;
        if (O.record || O.trace) { fprintf(stderr, "--live plays the real keyboard: no --trace / --record\n"); usage(); }
    }
    if (O.screen < 3 || O.screen > 5) { fprintf(stderr, "--screen 3|4|5\n"); usage(); }
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

#ifdef RH_PRODUCT
/* Windows 8+ has no native 8/16-bit display modes: retail's fullscreen SetDisplayMode(640,480,16) (0x582e90) returns
 * DDERR_UNSUPPORTED → "It failed in the display mode setting" (retail sotes.exe itself on Windows 11, 2026-10-24) unless
 * the process runs under the compatibility layer DWM8And16BitMitigation (what Windows' "Reduced color mode" setting
 * applies; with it retail reaches the title menu).  Layers apply at process creation: the product relaunches itself
 * once with the layer added to __COMPAT_LAYER (--no-compat-layer: not). */
static void compat_relaunch(void)
{
    static const char want[] = "DWM8And16BitMitigation";
    char cur[512] = "", nv[600], self[MAX_PATH], cmd[32768];
    DWORD n = GetEnvironmentVariableA("__COMPAT_LAYER", cur, sizeof cur);
    if (n >= sizeof cur || strstr(cur, want) || GetEnvironmentVariableA("OSS_COMPAT_RELAUNCHED", NULL, 0)) return;
    snprintf(nv, sizeof nv, "%s%s%s", cur, *cur ? " " : "", want);
    snprintf(cmd, sizeof cmd, "%s", GetCommandLineA());
    if (!GetModuleFileNameA(NULL, self, sizeof self) || !SetEnvironmentVariableA("__COMPAT_LAYER", nv) ||
        !SetEnvironmentVariableA("OSS_COMPAT_RELAUNCHED", "1")) return;
    STARTUPINFOA si = { .cb = sizeof si }; PROCESS_INFORMATION pi;
    if (!CreateProcessA(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {   /* run here, without the layer */
        SetEnvironmentVariableA("__COMPAT_LAYER", n ? cur : NULL);
        return;
    }
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0; GetExitCodeProcess(pi.hProcess, &code);
    ExitProcess(code);
}
#endif

int main(int argc, char **argv)
{
    parse(argc, argv);
#ifdef RH_PRODUCT
    if (!O.no_compat_layer) compat_relaunch();
#endif
    QueryPerformanceFrequency(&g_qf); QueryPerformanceCounter(&g_t0);
    CreateDirectoryA(O.rundir, NULL);
    char lp[MAX_PATH]; snprintf(lp, sizeof lp, "%s\\rhost.log", O.rundir);
    g_logf = fopen(lp, "w");
    g_log = stdout;
#ifdef RH_PRODUCT
    { char cl[512] = ""; GetEnvironmentVariableA("__COMPAT_LAYER", cl, sizeof cl); rh_log("product: __COMPAT_LAYER=%s\n", cl); }
#endif
    if (!O.live && !O.no_arena && rh_heap_init()) rh_fatal("heap arena reservation failed");
    if (!O.live && rh_media_reserve()) rh_fatal("rh_media: cannot reserve the sound pool");
    if (asset_reserve()) rh_fatal("cannot reserve the asset DLL range %08x..%08x", ASSET_LO, ASSET_HI);
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
    if (O.standalone && (O.coverage || g_ntrap)) rh_fatal("--standalone maps no retail code: nothing to cover / trap");
    if (retail_map(exe_abs, &img, 1, O.standalone)) rh_fatal("map %s failed", exe_abs);
    layout_from_pe();
    if (O.standalone && strcmp(g_ed->tag, "enold")) rh_fatal("--standalone: EN-old only (the port's edition)");
    rh_log("rhost: mapped %s edition=%s (imports %d resolved, %d stubbed) entry=%08x text=%08x+%x data=%08x..%08x\n",
           exe_abs, g_ed->tag, img.n_imports_resolved, img.n_imports_stubbed, img.entry, RT_TEXT_LO, RT_TEXT_SZ, RT_DATA_LO, RT_DATA_HI);
    iat_override("GetModuleHandleA", my_GetModuleHandleA);
    iat_override("LoadLibraryA", my_LoadLibraryA);
    iat_override("FreeLibrary", my_FreeLibrary);
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
    if (O.live) {   /* the real platform: only the launcher keeps a fallback */
        iat_override("DialogBoxParamA", O.no_launcher ? my_DialogBoxParamA : live_DialogBoxParamA);
        g_real_dice = (dice_t)iat_lookup("DirectInputCreateEx");
        iat_override("DirectInputCreateEx", live_DirectInputCreateEx);
        iat_try("OutputDebugStringA", live_OutputDebugStringA);
        iat_override("MessageBoxA", live_MessageBoxA);
        goto platform_done;
    }
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
platform_done:
    g_rh_flip_stub = make_stub(NULL, pre_flip, g_ed->flip);
    g_rh_easer_stub = make_stub(&g_ticks, NULL, g_ed->easer);
    if (!O.standalone) {   /* (standalone: no call sites — the port reaches these through its slots / rh_retail) */
        int nf = redirect_calls(g_ed->flip, g_rh_flip_stub);
        int ne = redirect_calls(g_ed->easer, g_rh_easer_stub);
        int ns = redirect_calls(g_ed->srand, my_srand);
        if (!nf || !ne || ns != 1) rh_fatal("engine call sites flip=%d easer=%d srand=%d (want >0/>0/1)", nf, ne, ns);
        rh_log("hooks: flip %d sites, easer %d, srand %d\n", nf, ne, ns);
        if (!O.no_uninit_fix) rh_log("uninit-fix: %d site(s) patched (quirk #123)\n", uninit_fix_install());
    } else if (!O.no_uninit_fix) rh_heap_zero_alloc = 1;   /* site D on the arena (the port CRT zeroes its own blocks) */
    load_volatile();
    rh_trace_init();
    rh_hooks_install();
    if (O.coverage) cov_install();
    AddVectoredExceptionHandler(0, crash_veh);   /* last in line: the trap / watch handlers see theirs first */
    if (g_nw) AddVectoredExceptionHandler(1, watch_veh);
    if (g_ntrap) {
        AddVectoredExceptionHandler(1, trap_veh);
        for (int i = 0; i < g_ntrap; i++) { g_torig[i] = G8(g_tva[i]); G8(g_tva[i]) = 0xCC; }
    }
    if (O.standalone) AddVectoredExceptionHandler(1, sa_veh);
    if (!O.live) {
        rh_scrub_install();
        rh_scrub_iat();                              /* last: wraps every IAT slot incl. our overrides */
    }
    if (O.standalone) sa_seal(exe_abs, &img);
    if (O.trace && !O.frames) O.frames = rh_trace_last_frame() + O.tail;
    rh_log("rhost: seed=%s%08x frames<=%u step=%ums creep_after=%u realtime=%d quiet=%d audible=%d arena=%d overlay=%d standalone=%d live=%d\n",
           O.has_seed ? "" : "none/", O.seed, O.frames, O.step_ms, O.creep_after, O.realtime, O.quiet, O.audible,
           !O.live && !O.no_arena, !O.live && !O.no_overlay, O.standalone, O.live);
    if (!O.live) rh_dinput_frame(0);
    if (!O.record && !O.live && O.stall_wall_ms) CloseHandle(CreateThread(NULL, 0, watchdog, NULL, 0, NULL));
    /* the CRT entry (retail's `start`, or the port's crt_start): never returns */
    call_on_stack(O.standalone ? (void *)crt_start : (void *)(uintptr_t)img.entry, NULL, MAIN_STACK_LO, MAIN_STACK_HI - 192);
    finish("entry-returned");
    return 0;
}
