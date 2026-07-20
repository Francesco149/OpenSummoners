// tools/ennse_voice/ennse_voice.c — EN-SE / JP-SE JP-voice patch (standalone proxy version.dll).
//
// Restores the Japanese DIALOGUE + DELUXE COMBAT voices AND the monster SFX for the retail
// Special Edition, exe untouched, memory-only.  Handles BOTH editions in one DLL (picked by
// host exe name), because they share the same engine bug:
//
//   sotes_en.exe (EN-SE) — the English localizer removed the one line that loads sotesx_s.dll,
//     so the voice-bank global DAT_0092af80 stays null: no dialogue, non-deluxe combat.
//   sotes.exe    (JP-SE) — loads sotesx_s.dll natively, so dialogue + deluxe combat already work.
//
// THE SHARED BUG (findings/ense-voice-combat-init.md, quirk #78): the boot sound-def registrar
// (EN FUN_0059b520 / JP FUN_00599b40, byte-identical) walks the 294-row action table and, in its
// DELUXE branch (taken when the voice bank is set), does `id = deluxe_id[row]; if (id==0||id==0x7fff)
// goto skip` — so the 64 rows with a non-deluxe sound but NO deluxe variant (every MONSTER row:
// harpy 0x790-0x795, ghosts, babymage…) register NOTHING → their SFX go silent.  This fires on JP
// too (USER-confirmed) whenever the voice bank is present when the registrar runs.
//
// TWO memory patches, armed by a DllMain waiter once SteamStub decrypts .text:
//   (1) SEED  — EN only: inline-hook the bank_load wrapper 0x5d8b10 (called by the loader before
//       the registrar) and set 0x92af80 = LoadLibraryA(sotesx_s), so party rows take the deluxe
//       branch + the manager gate builds the dialogue mgr.  JP skips this (bank loads natively).
//   (2) DELUXE-SKIP PATCH — both editions: flip ONE byte in the registrar so the `deluxe_id==0`
//       skip `je <skip>` becomes `je <non-deluxe path>` — those rows then register from sotesd
//       with their non-deluxe id (correct dll).  EN je @0x59cccc, JP je @0x59b2ec; both are
//       `0f 84 83 00 00 00` and both patch the rel32 low byte 0x83->0x36 (redirect delta 0x36).
//       The 0x7fff "skip in deluxe" sentinel is left intact.  Register state at the je (eax=row,
//       edi=sotesd, ebx=0) matches the non-deluxe path's real predecessors — disasm-verified.
//
// Both patches are opcode/prologue-signature gated ⇒ a wrong exe guess or a future build shift
// fails safe (no patch), never corrupts the game.
//
// 17 version.dll exports forwarded to realver.dll via version.def.  INSTALL beside the exe:
// version.dll + realver.dll + sotesx_s.dll.  Log: oss_voice.log.
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

// EN-SE (sotes_en.exe): seed (localizer removed the bank load) + deluxe-skip fix.
#define EN_SEED_HOOK   0x5d8b10u   // bank_load wrapper — seed BEFORE the registrar
#define EN_SEED_RESUME 0x5d8b16u   // 0x5d8b10 + 6 (after the stolen `sub esp,0x710`)
#define EN_REG_JE      0x59ccccu   // registrar deluxe-skip je (-> 0x59cd08 non-deluxe path)
#define EN_BANK_GLOBAL 0x92af80u
#define EN_CLIP_LOADER 0x5e37a0u
#define EN_SOTESP      0x92af78u
#define EN_SOTESD      0x92af88u

// JP-SE (sotes.exe): loads the voice bank natively — needs ONLY the deluxe-skip fix.
#define JP_REG_JE      0x59b2ecu   // registrar deluxe-skip je (-> 0x59b328 non-deluxe path)
#define JP_BANK_GLOBAL 0x926170u
#define JP_CLIP_LOADER 0x5e13d0u
#define JP_SOTESP      0x926168u
#define JP_SOTESD      0x926178u

static uintptr_t g_delta;
static char      g_logpath[MAX_PATH];
static int       g_is_en;                                 // 1 = sotes_en.exe, 0 = sotes.exe (JP)
static uintptr_t A_reg_je, A_bank, A_clip, A_sotesp, A_sotesd;   // resolved per edition at attach
#define AP(x) ((void *)((uintptr_t)(x) + g_delta))

static void vlog(const char *fmt, ...) {
    FILE *f = fopen(g_logpath, "a"); if (!f) return;
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f); fclose(f);
}

// ─── EN seed: set the voice bank pointer on the main thread when the loader calls the bank_load
// wrapper (before FUN_0059b520).  Guarded on the pointer being null → seeds once.  Allocates nothing.
static void __cdecl seed_once(void) {
    void **bankg = (void **)AP(A_bank);
    if (*bankg != 0) return;
    char path[MAX_PATH]; GetModuleFileNameA(NULL, path, MAX_PATH);
    char *bs = strrchr(path, '\\'); if (bs) bs[1] = 0; else path[0] = 0;
    strncat(path, "sotesx_s.dll", MAX_PATH - strlen(path) - 1);
    *bankg = LoadLibraryA(path);                      // == the one line the EN localizer removed
    vlog("[seed] voice bank -> %p  [%s]  (before the registrar → party deluxe + dialogue mgr)", *bankg, path);
}

// ─── deluxe-skip patch (both editions): registrar `je <skip>` -> `je <non-deluxe path>`.  Both
// je's are `0f 84 83 00 00 00`; redirect delta is 0x36 either way, so patch the rel32 low byte
// (A_reg_je + 2) 0x83 -> 0x36.  Opcode-signature gated.
static void install_registrar_patch(void) {
    unsigned char *je = (unsigned char *)AP(A_reg_je);
    if (!(je[0] == 0x0f && je[1] == 0x84 && je[2] == 0x83 && je[3] == 0x00 && je[4] == 0x00 && je[5] == 0x00)) {
        vlog("[reg] je sig mismatch @0x%x (%02x %02x %02x %02x %02x %02x) — no patch",
             A_reg_je, je[0], je[1], je[2], je[3], je[4], je[5]);
        return;
    }
    DWORD old;
    if (!VirtualProtect(je + 2, 1, PAGE_EXECUTE_READWRITE, &old)) { vlog("[reg] VirtualProtect failed"); return; }
    je[2] = 0x36;                              // -> je non-deluxe path (mob SFX register from sotesd)
    VirtualProtect(je + 2, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), je, 6);
    vlog("[reg] deluxe-skip patched @ 0x%x: deluxe_id==0 rows -> non-deluxe path (mob SFX restored)", A_reg_je);
}

// ─── EN seed hook: inline-hook 0x5d8b10, steal the 6-byte `sub esp,0x710` (81 EC 10 07 00 00),
// trampoline pushad/pushfd → seed_once → stolen insn → jmp 0x5d8b16.
static void install_seed_hook(void) {
    unsigned char *fn = (unsigned char *)AP(EN_SEED_HOOK);
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
    { void *slot = p + 4; D(slot); D(AP(EN_SEED_RESUME)); }
    #undef B
    #undef D
    DWORD old;
    if (!VirtualProtect(fn, 6, PAGE_EXECUTE_READWRITE, &old)) { vlog("[hook] VirtualProtect failed"); return; }
    fn[0] = 0xE9; *(int32_t *)(fn + 1) = (int32_t)((uintptr_t)stub - ((uintptr_t)fn + 5));
    fn[5] = 0x90;
    VirtualProtect(fn, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, 6);
    vlog("[hook] seed hook @ 0x%x installed (stub=%p)", EN_SEED_HOOK, stub);
}

#ifdef OSS_VOICE_DIAG
// ─── clip-loader diagnostic (verify the fix): distinct (bank,id) plays.  Expect deluxe combat
// [VOICE_x_s] AND monster ids (harpy 0x790-0x795) [SOTESD] both present.
static volatile uint32_t g_c_ret, g_c_s8, g_c_sc;
static void *g_clip_resume;
static void __cdecl clip_log(void) {
    static uint32_t seen[800]; static int nseen;
    uint32_t bank = g_c_s8, id = g_c_sc, key = bank ^ (id * 2654435761u);
    for (int i = 0; i < nseen; i++) if (seen[i] == key) return;
    if (nseen < 800) seen[nseen++] = key;
    uint32_t voice  = (uint32_t)(uintptr_t)*(void **)AP(A_bank);
    uint32_t sotesp = (uint32_t)(uintptr_t)*(void **)AP(A_sotesp);
    uint32_t sotesd = (uint32_t)(uintptr_t)*(void **)AP(A_sotesd);
    const char *t = bank == voice ? "VOICE_x_s" : bank == sotesp ? "SOTESP" :
                    bank == sotesd ? "SOTESD" : "other";
    vlog("[clip] id=%u(0x%x) bank=%x[%s] ret=%x", id, id, bank, t, g_c_ret);
}
static void install_clip_hook(void) {
    unsigned char *fn = (unsigned char *)AP(A_clip);
    if (fn[0] != 0x6a || fn[1] != 0xff || fn[2] != 0x68) { vlog("[clip] loader sig mismatch — no diag"); return; }
    g_clip_resume = AP(A_clip + 7);
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
    vlog("[clip] diagnostic hook @ 0x%x installed", A_clip);
}
#endif // OSS_VOICE_DIAG

// Waiter: wait for SteamStub to decrypt .text (SteamStub decrypts the whole section at once at the
// OEP), then arm the patches — well before the registrar runs, and (EN) before the loader's first
// wrapper call.  EN keys off the seed-hook prologue (81 EC ...), JP off the registrar je
// (0f 84 83 ...).  Signature-gated ⇒ a future shift fails safe.  version.dll runs before the exe entry.
static DWORD WINAPI waiter_thread(void *unused) {
    (void)unused;
    int i = 0;
    if (g_is_en) {
        unsigned char *fn = (unsigned char *)AP(EN_SEED_HOOK);
        while (!(fn[0] == 0x81 && fn[1] == 0xec && fn[2] == 0x10 && fn[3] == 0x07 && fn[4] == 0x00 && fn[5] == 0x00)) {
            if (++i > 120000) { vlog("[waiter] TIMEOUT: EN .text never decrypted — no patch"); return 0; }
            Sleep(1);
        }
        vlog("[waiter] EN-SE .text decrypted after ~%d ms — arming seed + deluxe-skip patch", i);
        install_registrar_patch();
        install_seed_hook();
    } else {
        unsigned char *je = (unsigned char *)AP(JP_REG_JE);
        while (!(je[0] == 0x0f && je[1] == 0x84 && je[2] == 0x83 && je[3] == 0x00 && je[4] == 0x00 && je[5] == 0x00)) {
            if (++i > 120000) { vlog("[waiter] TIMEOUT: JP .text never decrypted — no patch"); return 0; }
            Sleep(1);
        }
        vlog("[waiter] JP-SE .text decrypted after ~%d ms — arming deluxe-skip patch (bank loads natively)", i);
        install_registrar_patch();
    }
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
        if (StrStrIA_(hn, "sotes_en")) {            // English SE
            g_is_en = 1;
            A_reg_je = EN_REG_JE; A_bank = EN_BANK_GLOBAL; A_clip = EN_CLIP_LOADER;
            A_sotesp = EN_SOTESP; A_sotesd = EN_SOTESD;
        } else if (StrStrIA_(hn, "sotes")) {        // Japanese SE (sotes.exe)
            g_is_en = 0;
            A_reg_je = JP_REG_JE; A_bank = JP_BANK_GLOBAL; A_clip = JP_CLIP_LOADER;
            A_sotesp = JP_SOTESP; A_sotesd = JP_SOTESD;
        } else {
            vlog("[proxy] host '%s' is not a sotes exe — idle", hn); return TRUE;
        }
        g_delta = (uintptr_t)GetModuleHandleA(NULL) - ORIG_BASE;
        vlog("[proxy] attach host=%s edition=%s delta=%p", host, g_is_en ? "EN-SE" : "JP-SE", (void *)g_delta);
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
