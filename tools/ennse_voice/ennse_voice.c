// tools/ennse_voice/ennse_voice.c — EN-SE JP-voice patch (standalone proxy version.dll).
//
// Adds the Japanese DIALOGUE + DELUXE COMBAT voices (sotesx_s.dll) to the retail English
// special edition (sotes_en.exe), exe untouched, memory-only.  The EN localizer removed the
// voice-bank load from the audio-bank loader, leaving the voice-bank global DAT_0092af80 null.
//
// TWO memory patches, both armed by a DllMain waiter once SteamStub decrypts .text:
//
// (1) SEED — set DAT_0092af80 = LoadLibraryA("sotesx_s.dll") EARLY, before the boot sound-def
//     registrar FUN_0059b520 runs, so party-combat rows take its DELUXE branch (voice grunts)
//     and the manager gate `if(0x92af80!=0){build voice mgr}` builds the dialogue manager.  We
//     inline-hook the bank_load wrapper FUN_005d8b10 (0x5d8b10, called directly by the loader on
//     the main thread) and seed on its first call.  Allocates nothing (engine builds the mgr).
//
// (2) DELUXE-SKIP PATCH — fix the SE registrar bug (findings/ense-voice-combat-init.md, quirk
//     #78): FUN_0059b520's DELUXE branch does `id = DAT_0065b104[row]; if (id==0||id==0x7fff)
//     goto 0x59cd55 (skip)` — so the 64 rows with a non-deluxe sound but NO deluxe variant (every
//     MONSTER row: harpy key 0xc789 ids 0x790-0x795, ghosts, babymage…) register NOTHING once the
//     bank is set → mob SFX silent.  The party rows are fine (they have a deluxe id); only the
//     deluxe_id==0 rows wrongly skip.  We flip ONE byte at 0x59ccce so that `je 0x59cd55` (skip)
//     becomes `je 0x59cd08` (the NON-deluxe path) — those rows then register from sotesd with
//     their non-deluxe id (correct DLL), exactly like the bank-null case.  Register state at the
//     patched je (eax=row off, edi=sotesd, ebx=0) equals the non-deluxe path's real predecessors,
//     verified by disasm.  The 0x7fff sentinel still skips (intentional deluxe suppression).
//
// Result: deluxe combat (party deluxe branch) + JP dialogue (manager gate) + mob SFX (monster
// rows routed to the non-deluxe/sotesd path).  Setting a deluxe_id on the monster rows instead
// would send them to the voice DLL (bank=sotesx_s), which lacks monster sounds — hence the
// path-redirect, not an id fill.  Both patches are prologue/opcode-signature gated → fail safe.
//
// 17 version.dll exports forwarded to realver.dll via version.def.  INSTALL
// (…\steamapps\common\sotes\): version.dll + realver.dll + sotesx_s.dll.  Log: oss_voice.log.
//   nix develop --command make -C tools/ennse_voice     # -> build/version.dll
//   (-DOSS_VOICE_DIAG re-enables the clip-loader (bank,id) log to verify the fix.)

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

static const char *StrStrIA_(const char *hay, const char *needle);

#define ORIG_BASE      0x400000u
#define VA_SEED_HOOK   0x5d8b10u   // bank_load wrapper — seed BEFORE FUN_0059b520 (party->deluxe)
#define VA_SEED_RESUME 0x5d8b16u   // 0x5d8b10 + 6 (after the stolen `sub esp,0x710`)
#define VA_REG_JE      0x59ccccu   // FUN_0059b520 `je 0x59cd55` (skip deluxe_id==0): 0f 84 83 00 00 00
#define VA_BANK_GLOBAL 0x92af80u   // voice bank HMODULE — null in EN = the removed load line
#define VA_CLIP_LOADER 0x5e37a0u   // clip loader (bank,id)->DSound — diagnostic only

// Bank cluster: 0x92af78 sotesp · af7c MD · af80 VOICE(target) · af84 extra · af88 sotesd · af94 sotesd_en.

static uintptr_t g_delta;
static char      g_logpath[MAX_PATH];
#define AP(x) ((void *)((uintptr_t)(x) + g_delta))

static void vlog(const char *fmt, ...) {
    FILE *f = fopen(g_logpath, "a"); if (!f) return;
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f); fclose(f);
}

// ─── seed: set ONLY the voice bank pointer, on the main thread, when the loader calls the
// bank_load wrapper (before FUN_0059b520).  Guarded on the pointer being null → seeds once.
static void __cdecl seed_once(void) {
    void **bankg = (void **)AP(VA_BANK_GLOBAL);
    if (*bankg != 0) return;
    char path[MAX_PATH]; GetModuleFileNameA(NULL, path, MAX_PATH);
    char *bs = strrchr(path, '\\'); if (bs) bs[1] = 0; else path[0] = 0;
    strncat(path, "sotesx_s.dll", MAX_PATH - strlen(path) - 1);
    *bankg = LoadLibraryA(path);                      // == the one line the EN localizer removed
    vlog("[seed] voice bank -> %p  [%s]  (before FUN_0059b520 → party deluxe + dialogue mgr)", *bankg, path);
}

// Inline-hook 0x5d8b10: steal the 6-byte prologue `sub esp,0x710` (81 EC 10 07 00 00, position-
// independent), trampoline pushad/pushfd → seed_once → stolen insn → jmp 0x5d8b16.
static void install_seed_hook(void) {
    unsigned char *fn = (unsigned char *)AP(VA_SEED_HOOK);
    unsigned char *stub = (unsigned char *)VirtualAlloc(NULL, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!stub) { vlog("[hook] VirtualAlloc failed — no seed"); return; }
    unsigned char *p = stub;
    #define B(x) (*p++ = (unsigned char)(x))
    #define D(x) (*(void **)p = (void *)(x), p += 4)
    B(0x60); B(0x9c);                          // pushad; pushfd
    B(0xb8); D(&seed_once); B(0xff); B(0xd0);  // mov eax,seed_once; call eax
    B(0x9d); B(0x61);                          // popfd; popad
    memcpy(p, fn, 6); p += 6;                  // stolen: sub esp,0x710 (live copy)
    B(0xff); B(0x25);                          // jmp dword ptr [slot]
    { void *slot = p + 4; D(slot); D(AP(VA_SEED_RESUME)); }
    #undef B
    #undef D
    DWORD old;
    if (!VirtualProtect(fn, 6, PAGE_EXECUTE_READWRITE, &old)) { vlog("[hook] VirtualProtect failed"); return; }
    fn[0] = 0xE9; *(int32_t *)(fn + 1) = (int32_t)((uintptr_t)stub - ((uintptr_t)fn + 5));
    fn[5] = 0x90;
    VirtualProtect(fn, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, 6);
    vlog("[hook] seed hook @ 0x%x installed (stub=%p)", VA_SEED_HOOK, stub);
}

// ─── deluxe-skip patch: FUN_0059b520 `je 0x59cd55` (skip if deluxe_id==0) -> `je 0x59cd08`
// (non-deluxe path).  Redirect target = 0x59cd08 - (0x59cccc + 6) = 0x36; only the low rel32
// byte at 0x59ccce changes (0x83 -> 0x36).  Opcode-signature gated (0f 84 83 00 00 00).
static void install_registrar_patch(void) {
    unsigned char *je = (unsigned char *)AP(VA_REG_JE);
    if (!(je[0] == 0x0f && je[1] == 0x84 && je[2] == 0x83 && je[3] == 0x00 && je[4] == 0x00 && je[5] == 0x00)) {
        vlog("[reg] je sig mismatch (%02x %02x %02x %02x %02x %02x) — no patch",
             je[0], je[1], je[2], je[3], je[4], je[5]);
        return;
    }
    DWORD old;
    if (!VirtualProtect(je + 2, 1, PAGE_EXECUTE_READWRITE, &old)) { vlog("[reg] VirtualProtect failed"); return; }
    je[2] = 0x36;                              // je 0x59cd55 (skip) -> je 0x59cd08 (non-deluxe path)
    VirtualProtect(je + 2, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), je, 6);
    vlog("[reg] deluxe-skip patched @ 0x%x: deluxe_id==0 rows -> non-deluxe path (mob SFX from sotesd)", VA_REG_JE);
}

#ifdef OSS_VOICE_DIAG
// ─── clip-loader diagnostic (verify the fix): distinct (bank,id) plays.  Expect deluxe combat
// 2235/2236 [VOICE_x_s] AND monster ids (harpy 0x790-0x795) [SOTESD] both present.
static volatile uint32_t g_c_ret, g_c_s8, g_c_sc;
static void *g_clip_resume;
static void __cdecl clip_log(void) {
    static uint32_t seen[800]; static int nseen;
    uint32_t bank = g_c_s8, id = g_c_sc, key = bank ^ (id * 2654435761u);
    for (int i = 0; i < nseen; i++) if (seen[i] == key) return;
    if (nseen < 800) seen[nseen++] = key;
    uint32_t voice  = (uint32_t)(uintptr_t)*(void **)AP(VA_BANK_GLOBAL);
    uint32_t sotesp = (uint32_t)(uintptr_t)*(void **)AP(0x92af78u);
    uint32_t sotesd = (uint32_t)(uintptr_t)*(void **)AP(0x92af88u);
    const char *t = bank == voice ? "VOICE_x_s" : bank == sotesp ? "SOTESP" :
                    bank == sotesd ? "SOTESD" : "other";
    vlog("[clip] id=%u(0x%x) bank=%x[%s] ret=%x", id, id, bank, t, g_c_ret);
}
static void install_clip_hook(void) {
    unsigned char *fn = (unsigned char *)AP(VA_CLIP_LOADER);
    if (fn[0] != 0x6a || fn[1] != 0xff || fn[2] != 0x68) { vlog("[clip] loader sig mismatch — no diag"); return; }
    g_clip_resume = AP(VA_CLIP_LOADER + 7);
    unsigned char *stub = (unsigned char *)VirtualAlloc(NULL, 96, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!stub) return;
    unsigned char *p = stub;
    #define B(x) (*p++ = (unsigned char)(x))
    #define D(x) (*(void **)p = (void *)(x), p += 4)
    B(0x8b); B(0x04); B(0x24); B(0xa3); D(&g_c_ret);
    B(0x8b); B(0x44); B(0x24); B(0x08); B(0xa3); D(&g_c_s8);
    B(0x8b); B(0x44); B(0x24); B(0x0c); B(0xa3); D(&g_c_sc);
    B(0x60); B(0x9c); B(0xb8); D(&clip_log); B(0xff); B(0xd0); B(0x9d); B(0x61);
    memcpy(p, fn, 7); p += 7;
    B(0xff); B(0x25); D(&g_clip_resume);
    #undef B
    #undef D
    DWORD old;
    if (!VirtualProtect(fn, 7, PAGE_EXECUTE_READWRITE, &old)) return;
    fn[0] = 0xE9; *(int32_t *)(fn + 1) = (int32_t)((uintptr_t)stub - ((uintptr_t)fn + 5));
    fn[5] = 0x90; fn[6] = 0x90;
    VirtualProtect(fn, 7, old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, 7);
    vlog("[clip] diagnostic hook @ 0x%x installed", VA_CLIP_LOADER);
}
#endif // OSS_VOICE_DIAG

// Waiter: wait for SteamStub to decrypt .text (seed-hook prologue == 81 EC 10 07 00 00), then
// arm BOTH patches (whole .text decrypts at once) — well before the loader's first wrapper call
// and long before FUN_0059b520.  version.dll runs before the exe entry.
static DWORD WINAPI waiter_thread(void *unused) {
    (void)unused;
    unsigned char *fn = (unsigned char *)AP(VA_SEED_HOOK);
    int i = 0;
    while (!(fn[0] == 0x81 && fn[1] == 0xec && fn[2] == 0x10 &&
             fn[3] == 0x07 && fn[4] == 0x00 && fn[5] == 0x00)) {
        if (++i > 120000) { vlog("[waiter] TIMEOUT: .text never decrypted — no patch"); return 0; }
        Sleep(1);
    }
    vlog("[waiter] .text decrypted after ~%d ms — arming seed + deluxe-skip patch", i);
    install_registrar_patch();     // fix the mob-skip (before FUN_0059b520 runs)
    install_seed_hook();           // set the bank (before FUN_0059b520 runs)
#ifdef OSS_VOICE_DIAG
    install_clip_hook();
#endif
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID x) {
    (void)x;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        char dir[MAX_PATH]; GetModuleFileNameA(h, dir, MAX_PATH);
        char *bs = strrchr(dir, '\\'); if (bs) bs[1] = 0; else dir[0] = 0;
        _snprintf(g_logpath, MAX_PATH, "%soss_voice.log", dir);
        char host[MAX_PATH] = ""; GetModuleFileNameA(NULL, host, MAX_PATH);
        char *slash = strrchr(host, '\\'); const char *hn = slash ? slash + 1 : host;
        if (!StrStrIA_(hn, "sotes")) { vlog("[proxy] host '%s' has no 'sotes' — idle", hn); return TRUE; }
        g_delta = (uintptr_t)GetModuleHandleA(NULL) - ORIG_BASE;
        vlog("[proxy] attach host=%s delta=%p (early seed + deluxe-skip patch)", host, (void *)g_delta);
        CreateThread(NULL, 0, waiter_thread, NULL, 0, NULL);
    }
    return TRUE;
}

static const char *StrStrIA_(const char *hay, const char *needle) {
    if (!hay || !needle) return NULL;
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n && (tolower((unsigned char)*h) == tolower((unsigned char)*n))) { h++; n++; }
        if (!*n) return hay;
    }
    return NULL;
}
