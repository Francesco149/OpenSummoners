/* trampoline.h — inline 5-byte E9-jmp hooks for the HOT engine VAs (M3b-perf).
 *
 * The INT3+VEH detour (va_detour.h) is correct + needs no length-disassembler,
 * but it pays TWO exception dispatches per hit — fine for the rare hooks (flip,
 * anchors, seed, input) but crushing for the blit/resolver VAs that fire
 * hundreds of times a frame (the M3b in-game ~25→56 fps wall).  This is the real
 * turbo fix: overwrite a hot VA's first 5 bytes with `E9 <rel32>` to our handler
 * so a hit is a couple of jumps, ZERO exceptions.
 *
 * No length-disassembler: per the plan, each hooked VA's head bytes are HARDCODED
 * at the call site (read once from the unpacked exe; all are simple
 * position-independent prologues — push/mov/sub — with NO rel jmp/call in the
 * relocated span, verified by disasm).  The caller passes the head bytes + the
 * instruction-aligned head_len (>= 5).
 *
 * Per hook we build two stubs in one RX page:
 *   thunk : pushad; pushfd; (compute entry_esp); push entry_esp; push ecx;
 *           call cb; add esp,8; popfd; popad; jmp relay
 *   relay : <the original head_len bytes> ; jmp (va + head_len)
 * The VA is patched to `E9 -> thunk`.  Because we jmp BEFORE the prologue runs,
 * at the thunk esp == the function-entry esp (return addr at [esp], args above),
 * and ecx == the thiscall `this` — exactly what the cb reads.  pushad/pushfd
 * preserve every register + flags across the C call (cdecl already preserves
 * ebx/esi/edi/ebp), so the relay runs the real head in pristine entry state, then
 * jumps back into the function past the head.
 *
 * The cb is the LIGHT signature `void cb(DWORD ecx, DWORD entry_esp)` — the hot
 * callbacks only need the thiscall `this` and the stack-arg base, so we skip the
 * full CONTEXT the VEH path builds.  onEnter-only, permanent (no disarm) — which
 * is all the blit/resolver hooks ever are.
 */
#ifndef OSS_TRAMPOLINE_H
#define OSS_TRAMPOLINE_H

#include <windows.h>
#include <stdint.h>

#include "proxy_log.h"
#include "va_detour.h"   /* detour_make_rwx */

typedef void (*tramp_cb)(DWORD ecx, DWORD entry_esp);

/* One RX arena bump-allocated for all hooks' thunks+relays (each ~40 B). */
#define TRAMP_ARENA 4096u
static uint8_t *g_tramp_arena = NULL;
static size_t   g_tramp_used  = 0;

static uint8_t *tramp__alloc(size_t n)
{
    if (!g_tramp_arena) {
        g_tramp_arena = (uint8_t *)VirtualAlloc(NULL, TRAMP_ARENA,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!g_tramp_arena) {
            proxy_logf("[tramp] FATAL: VirtualAlloc failed err=%lu", GetLastError());
            return NULL;
        }
    }
    if (g_tramp_used + n > TRAMP_ARENA) {
        proxy_logf("[tramp] FATAL: arena exhausted (%zu + %zu)", g_tramp_used, n);
        return NULL;
    }
    uint8_t *p = g_tramp_arena + g_tramp_used;
    g_tramp_used += n;
    return p;
}

/* little-endian rel32 for an `E9`/`E8` at `from` (5-byte instruction) -> `to` */
static void tramp__put_rel32(uint8_t *at, DWORD insn_addr, DWORD target)
{
    int32_t rel = (int32_t)(target - (insn_addr + 5));
    at[0] = (uint8_t)rel;        at[1] = (uint8_t)(rel >> 8);
    at[2] = (uint8_t)(rel >> 16); at[3] = (uint8_t)(rel >> 24);
}

/* Install an E9 hook at `va`.  `head`/`head_len` = the VA's first instructions
 * (instruction-aligned, head_len >= 5, no rel jmp/call inside). */
static void trampoline_add(DWORD va, tramp_cb cb, const uint8_t *head, int head_len)
{
    if (head_len < 5) {
        proxy_logf("[tramp] refuse 0x%lx: head_len %d < 5", (unsigned long)va, head_len);
        return;
    }

    /* ── relay: <head bytes> ; E9 -> (va + head_len) ── */
    uint8_t *relay = tramp__alloc((size_t)head_len + 5);
    if (!relay) return;
    memcpy(relay, head, (size_t)head_len);
    relay[head_len] = 0xE9;
    tramp__put_rel32(relay + head_len + 1, (DWORD)(ULONG_PTR)(relay + head_len),
                     va + (DWORD)head_len);

    /* ── thunk ── */
    uint8_t *t = tramp__alloc(32);
    if (!t) return;
    int i = 0;
    t[i++] = 0x60;                       /* pushad                        */
    t[i++] = 0x9C;                       /* pushfd                        */
    /* lea eax, [esp+0x24]  (entry_esp = esp + 36 after pushad+pushfd)     */
    t[i++] = 0x8D; t[i++] = 0x44; t[i++] = 0x24; t[i++] = 0x24;
    t[i++] = 0x50;                       /* push eax       (arg: entry_esp)*/
    t[i++] = 0x51;                       /* push ecx       (arg: this)     */
    t[i++] = 0xE8;                       /* call cb (cdecl)                */
    tramp__put_rel32(t + i, (DWORD)(ULONG_PTR)(t + i - 1), (DWORD)(ULONG_PTR)cb);
    i += 4;
    t[i++] = 0x83; t[i++] = 0xC4; t[i++] = 0x08;  /* add esp, 8 (pop args) */
    t[i++] = 0x9D;                       /* popfd                         */
    t[i++] = 0x61;                       /* popad                         */
    t[i++] = 0xE9;                       /* jmp relay                     */
    tramp__put_rel32(t + i, (DWORD)(ULONG_PTR)(t + i - 1),
                     (DWORD)(ULONG_PTR)relay);
    i += 4;

    /* ── patch the VA: E9 -> thunk (5 bytes; leftover head bytes are dead) ── */
    detour_make_rwx(va);
    uint8_t patch[5];
    patch[0] = 0xE9;
    tramp__put_rel32(patch + 1, va, (DWORD)(ULONG_PTR)t);
    memcpy((void *)va, patch, 5);
    FlushInstructionCache(GetCurrentProcess(), (void *)va, 5);

    proxy_logf("[tramp] hooked 0x%lx (head_len=%d) -> thunk %p relay %p",
               (unsigned long)va, head_len, (void *)t, (void *)relay);
}

#endif /* OSS_TRAMPOLINE_H */
