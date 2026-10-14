/* rh_scrub.c — keep system-DLL stack garbage out of retail state.
 *
 * Retail reads uninitialized stack: e.g. 0x586010 / 0x417c40 build a 256-entry palette on the
 * stack writing only bytes 0..2 of each dword (gamma LUT 0x8a9410, loops 0x586f32 / 0x4181a7)
 * and 0x491770 / 0x418216 copy all 1 KB into the heap — byte 3 is whatever an earlier call
 * left there. When that earlier call was into DirectDraw/GDI/quartz/kernel32, the leftovers
 * differ per run (2026-10-14 det gate: 87 + 22 heap dwords differing in byte 3 only).
 *
 * SYSTEM STACK: every external call retail makes on the MAIN thread — through its IAT and
 * through the DirectDraw7 / DirectShow / fake-DirectSound COM vtables — runs on a separate
 * stack [SYS_LO, SYS_TOP) inside the same TEB stack bounds: the thunk copies 16 argument
 * dwords there, calls the target with its return address = post, and post restores retail's
 * ESP + the callee's own pop count (stdcall/cdecl alike) and returns to retail. DLL code never
 * touches the retail stack, so retail's uninitialized reads only ever see retail's own
 * (deterministic) data. Callbacks into retail during an external call (WndProc from
 * DispatchMessage) run on the system stack; their nested external calls go direct.
 * Our own host code that runs on the retail stack (pre_flip) zeroes below itself
 * (rh_scrub_here). --scrub-kb 0 disables everything (raw retail behavior).
 *
 *   thunk:  if (tid != main || esp < RETAIL_LO) jmp target
 *           save ret/esp; copy [esp+4..+68) → SYS_TOP-64; push post; esp = sys; jmp target
 *   post:   ecx = esp - esp_at_call (4 + callee pops); esp = retail_esp + ecx; jmp saved ret */
#include "rhost.h"
#include <ddraw.h>

static volatile uint32_t g_jmp, g_main_tid;
static uint8_t *g_code, *g_post;
static long g_nthunks;
#define CODE_LO 0x5f010000u

static uint8_t *emit32(uint8_t *p, uint32_t v) { *(uint32_t *)p = v; return p + 4; }

/* zero the dead stack downward from hi, page by page, until two consecutive pages are already
 * clean (the stack starts all-zero and every scrub restores that, so garbage is one contiguous
 * run below the call site — adaptive depth: shallow calls cost ~1 page check) */
#define STACK_FLOOR 0x58010000u
static uint64_t g_scrubbed_pages;
static void __attribute__((noinline, used)) scrub_pages(uint32_t hi, uint32_t floor)
{
    hi &= ~3u;
    int clean = 0;
    while (hi > floor && clean < 2) {
        uint32_t lo = (hi - 1) & ~0xfffu;
        if (lo < floor) lo = floor;
        volatile uint32_t *p = (volatile uint32_t *)(uintptr_t)lo, *e = (volatile uint32_t *)(uintptr_t)hi;
        int dirty = 0;
        for (volatile uint32_t *q = p; q < e; q++) if (*q) { dirty = 1; break; }
        if (dirty) { memset((void *)(uintptr_t)lo, 0, hi - lo); clean = 0; g_scrubbed_pages++; }
        else clean++;
        hi = lo;
    }
}
static volatile uint32_t g_save_esp;
#define SYS_TOP   0x58700000u
_Static_assert(SYS_TOP < 0x58800000u, "system stack below the retail stack");       /* system stack [STACK_FLOOR, SYS_TOP); guard gap below RETAIL_LO */
#define RETAIL_LO 0x58800000u       /* retail's own stack [RETAIL_LO, 0x59000000) */
static volatile uint32_t g_depth, g_ret[64], g_resp[64], g_scall[64];

static void build_post(void)
{
    uint8_t *p = g_post = g_code;
    /* mov ecx,[g_depth]; dec ecx; mov [g_depth],ecx */
    *p++ = 0x8B; *p++ = 0x0D; p = emit32(p, (uint32_t)(uintptr_t)&g_depth);
    *p++ = 0x49;
    *p++ = 0x89; *p++ = 0x0D; p = emit32(p, (uint32_t)(uintptr_t)&g_depth);
    /* push eax (on the sys stack, below anything live) */
    *p++ = 0x50;
    /* mov eax, esp; add eax, 4  → esp as it was on return */
    *p++ = 0x89; *p++ = 0xE0;
    *p++ = 0x83; *p++ = 0xC0; *p++ = 0x04;
    /* sub eax, [g_scall + ecx*4] → 4 + callee pops */
    *p++ = 0x2B; *p++ = 0x04; *p++ = 0x8D; p = emit32(p, (uint32_t)(uintptr_t)g_scall);
    /* add eax, [g_resp + ecx*4] → retail esp after the call */
    *p++ = 0x03; *p++ = 0x04; *p++ = 0x8D; p = emit32(p, (uint32_t)(uintptr_t)g_resp);
    /* mov [g_save_esp], eax */
    *p++ = 0xA3; p = emit32(p, (uint32_t)(uintptr_t)&g_save_esp);
    /* mov eax,[g_ret + ecx*4]; mov [g_jmp], eax */
    *p++ = 0x8B; *p++ = 0x04; *p++ = 0x8D; p = emit32(p, (uint32_t)(uintptr_t)g_ret);
    *p++ = 0xA3; p = emit32(p, (uint32_t)(uintptr_t)&g_jmp);
    /* pop eax; mov esp,[g_save_esp]; jmp [g_jmp] */
    *p++ = 0x58;
    *p++ = 0x8B; *p++ = 0x25; p = emit32(p, (uint32_t)(uintptr_t)&g_save_esp);
    *p++ = 0xFF; *p++ = 0x25; p = emit32(p, (uint32_t)(uintptr_t)&g_jmp);
    g_code = (uint8_t *)(((uintptr_t)p + 15) & ~(uintptr_t)15);
}

static void *thunk(uint32_t target)
{
    if (!g_code) {
        g_code = VirtualAlloc((void *)(uintptr_t)CODE_LO, 0x40000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (g_code != (uint8_t *)(uintptr_t)CODE_LO) rh_fatal("rh_scrub: cannot map thunks at %08x", CODE_LO);
        build_post();
    }
    if (g_code + 96 > (uint8_t *)(uintptr_t)(CODE_LO + 0x40000)) rh_fatal("rh_scrub: thunk space exhausted");
    uint8_t *s = g_code, *p = s;
    /* push eax; mov eax, fs:[0x24]; cmp eax, main; jne direct_pop */
    *p++ = 0x50;
    *p++ = 0x64; *p++ = 0xA1; p = emit32(p, 0x24);
    *p++ = 0x3D; p = emit32(p, g_main_tid);
    *p++ = 0x75; uint8_t *j1 = p++;
    /* cmp esp, RETAIL_LO; jb direct_pop   (already on the system stack: nested) */
    *p++ = 0x81; *p++ = 0xFC; p = emit32(p, RETAIL_LO);
    *p++ = 0x72; uint8_t *j2 = p++;
    /* push ecx; push esi; push edi   (retail stack, above-esp writes only) */
    *p++ = 0x51; *p++ = 0x56; *p++ = 0x57;
    /* mov ecx,[g_depth]; mov eax,[esp+16] (ret); mov [g_ret+ecx*4],eax */
    *p++ = 0x8B; *p++ = 0x0D; p = emit32(p, (uint32_t)(uintptr_t)&g_depth);
    *p++ = 0x8B; *p++ = 0x44; *p++ = 0x24; *p++ = 0x10;
    *p++ = 0x89; *p++ = 0x04; *p++ = 0x8D; p = emit32(p, (uint32_t)(uintptr_t)g_ret);
    /* lea eax,[esp+16]; mov [g_resp+ecx*4],eax   (retail esp at entry: points at the ret addr) */
    *p++ = 0x8D; *p++ = 0x44; *p++ = 0x24; *p++ = 0x10;
    *p++ = 0x89; *p++ = 0x04; *p++ = 0x8D; p = emit32(p, (uint32_t)(uintptr_t)g_resp);
    /* mov dword [g_scall+ecx*4], SYS_TOP-68 ; inc ecx; mov [g_depth],ecx */
    *p++ = 0xC7; *p++ = 0x04; *p++ = 0x8D; p = emit32(p, (uint32_t)(uintptr_t)g_scall); p = emit32(p, SYS_TOP - 68);
    *p++ = 0x41;
    *p++ = 0x89; *p++ = 0x0D; p = emit32(p, (uint32_t)(uintptr_t)&g_depth);
    /* copy 16 dwords: lea esi,[esp+20]; mov edi, SYS_TOP-64; mov ecx,16; cld; rep movsd */
    *p++ = 0x8D; *p++ = 0x74; *p++ = 0x24; *p++ = 0x14;
    *p++ = 0xBF; p = emit32(p, SYS_TOP - 64);
    *p++ = 0xB9; p = emit32(p, 16);
    *p++ = 0xFC;
    *p++ = 0xF3; *p++ = 0xA5;
    /* mov dword [SYS_TOP-68], post */
    *p++ = 0xC7; *p++ = 0x05; p = emit32(p, SYS_TOP - 68); p = emit32(p, (uint32_t)(uintptr_t)g_post);
    /* pop edi; pop esi; pop ecx; pop eax; mov esp, SYS_TOP-68; jmp target */
    *p++ = 0x5F; *p++ = 0x5E; *p++ = 0x59; *p++ = 0x58;
    *p++ = 0xBC; p = emit32(p, SYS_TOP - 68);
    *p++ = 0xE9; p = emit32(p, target - ((uint32_t)(uintptr_t)p + 4));
    /* direct_pop: pop eax; jmp target */
    uint8_t *dp = p;
    *j1 = (uint8_t)(dp - (j1 + 1)); *j2 = (uint8_t)(dp - (j2 + 1));
    *p++ = 0x58;
    *p++ = 0xE9; p = emit32(p, target - ((uint32_t)(uintptr_t)p + 4));
    g_code = (uint8_t *)(((uintptr_t)p + 15) & ~(uintptr_t)15);
    g_nthunks++;
    return s;
}

static int is_thunk(void *p) { uint32_t v = (uint32_t)(uintptr_t)p; return v >= CODE_LO && v < CODE_LO + 0x40000; }

/* wrap every IAT slot (after all overrides are installed) */
void rh_scrub_iat(void)
{
    if (!O.scrub_kb) return;
    g_main_tid = GetCurrentThreadId();
    IMAGE_NT_HEADERS32 *nt = (void *)(0x400000 + ((IMAGE_DOS_HEADER *)0x400000)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *d = (void *)(0x400000 + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    extern void *rh_scrub_skip[];
    int n = 0;
    for (; d->Name; d++) {
        IMAGE_THUNK_DATA32 *iat = (void *)(0x400000 + d->FirstThunk);
        for (; iat->u1.Function; iat++) {
            void *f = (void *)(uintptr_t)iat->u1.Function;
            int skip = 0;
            for (int i = 0; rh_scrub_skip[i]; i++) if (rh_scrub_skip[i] == f) skip = 1;
            if (skip || is_thunk(f)) continue;
            iat->u1.Function = (uint32_t)(uintptr_t)thunk((uint32_t)(uintptr_t)f);
            n++;
        }
    }
    rh_log("scrub: %d IAT slots wrapped (external calls run on the system stack %08x..%08x)\n", n, STACK_FLOOR, SYS_TOP);
}

/* DirectDraw7 COM: wrap every vtable slot, in place, of each interface retail obtains */
void rh_scrub_wrap_vtbl(void *obj, int nslots, const char *what);
static void wrap_vtbl(void *obj, int nslots, const char *what) { rh_scrub_wrap_vtbl(obj, nslots, what); }
void rh_scrub_wrap_vtbl(void *obj, int nslots, const char *what)
{
    if (!O.scrub_kb) return;
    void **vt = *(void ***)obj;
    if (is_thunk(vt[3])) return;                       /* already wrapped (shared vtable) */
    DWORD old;
    VirtualProtect(vt, nslots * sizeof(void *), PAGE_READWRITE, &old);
    for (int i = 0; i < nslots; i++) if (!is_thunk(vt[i])) vt[i] = thunk((uint32_t)(uintptr_t)vt[i]);
    VirtualProtect(vt, nslots * sizeof(void *), old, &old);
    rh_log("scrub: %s vtable %p wrapped (%d slots)\n", what, (void *)vt, nslots);
}
typedef HRESULT (WINAPI *ddce_t)(GUID *, void **, REFIID, IUnknown *);
static ddce_t r_DirectDrawCreateEx;
typedef HRESULT (WINAPI *cs_t)(void *, DDSURFACEDESC2 *, IDirectDrawSurface7 **, IUnknown *);
typedef HRESULT (WINAPI *gas_t)(void *, DDSCAPS2 *, IDirectDrawSurface7 **);
typedef HRESULT (WINAPI *cc_t)(void *, DWORD, IDirectDrawClipper **, IUnknown *);
typedef HRESULT (WINAPI *cp_t)(void *, DWORD, LPPALETTEENTRY, IDirectDrawPalette **, IUnknown *);
static cs_t r_CreateSurface; static gas_t r_GetAttachedSurface; static cc_t r_CreateClipper; static cp_t r_CreatePalette;
static void surf_seen(IDirectDrawSurface7 *s);
static HRESULT WINAPI w_CreateSurface(void *dd, DDSURFACEDESC2 *d, IDirectDrawSurface7 **out, IUnknown *o)
{
    HRESULT r = r_CreateSurface(dd, d, out, o);
    if (r == DD_OK && out && *out) surf_seen(*out);
    return r;
}
static HRESULT WINAPI w_GetAttachedSurface(void *s, DDSCAPS2 *c, IDirectDrawSurface7 **out)
{
    HRESULT r = r_GetAttachedSurface(s, c, out);
    if (r == DD_OK && out && *out) surf_seen(*out);
    return r;
}
static HRESULT WINAPI w_CreateClipper(void *dd, DWORD f, IDirectDrawClipper **out, IUnknown *o)
{
    HRESULT r = r_CreateClipper(dd, f, out, o);
    if (r == DD_OK && out && *out) wrap_vtbl(*out, 9, "IDirectDrawClipper");
    return r;
}
static HRESULT WINAPI w_CreatePalette(void *dd, DWORD f, LPPALETTEENTRY e, IDirectDrawPalette **out, IUnknown *o)
{
    HRESULT r = r_CreatePalette(dd, f, e, out, o);
    if (r == DD_OK && out && *out) wrap_vtbl(*out, 7, "IDirectDrawPalette");
    return r;
}
static void patch_slot(void **vt, int i, void *fn, void **orig)
{
    DWORD old; VirtualProtect(&vt[i], 4, PAGE_READWRITE, &old);
    *orig = vt[i]; vt[i] = fn;
    VirtualProtect(&vt[i], 4, old, &old);
}
static void surf_seen(IDirectDrawSurface7 *s)
{
    void **vt = *(void ***)s;
    if (is_thunk(vt[3])) return;
    patch_slot(vt, 12, (void *)w_GetAttachedSurface, (void **)&r_GetAttachedSurface);   /* GetAttachedSurface */
    wrap_vtbl(s, 49, "IDirectDrawSurface7");
}
static HRESULT WINAPI my_DirectDrawCreateEx(GUID *g, void **out, REFIID iid, IUnknown *o)
{
    HRESULT r = r_DirectDrawCreateEx(g, out, iid, o);
    if (r == DD_OK && out && *out) {
        void **vt = *(void ***)*out;
        patch_slot(vt, 6, (void *)w_CreateSurface, (void **)&r_CreateSurface);      /* CreateSurface */
        patch_slot(vt, 4, (void *)w_CreateClipper, (void **)&r_CreateClipper);      /* CreateClipper */
        patch_slot(vt, 5, (void *)w_CreatePalette, (void **)&r_CreatePalette);      /* CreatePalette */
        wrap_vtbl(*out, 30, "IDirectDraw7");
    }
    return r;
}
void rh_scrub_install(void)
{
    if (!O.scrub_kb) return;
    r_DirectDrawCreateEx = (ddce_t)iat_lookup("DirectDrawCreateEx");
    iat_override("DirectDrawCreateEx", my_DirectDrawCreateEx);
}
long rh_scrub_thunks(void) { return g_nthunks; }
/* our own host code on the retail stack (pre_flip: logging, dumps, shots — msvcrt heap
 * pointers, system calls) leaves garbage too: zero below the caller's frame */
void __attribute__((noinline)) rh_scrub_here(void)
{
    if (!O.scrub_kb) return;
    uint32_t sp; __asm__ volatile("mov %%esp, %0" : "=r"(sp));
    scrub_pages(sp - 512, 0x58800000u);   /* below this frame and scrub_pages' own, retail stack only */
}
uint64_t rh_scrub_pages(void) { return g_scrubbed_pages; }
