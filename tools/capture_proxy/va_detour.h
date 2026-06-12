/* va_detour.h — inline engine-VA hooks via INT3 + a vectored exception handler.
 *
 * The retail exe has a FIXED ImageBase 0x00400000 with relocations stripped, so
 * every engine function lives at a stable absolute VA — no base math.  We hook a
 * function's ENTRY by overwriting its first byte with 0xCC (INT3) and catching
 * the resulting EXCEPTION_BREAKPOINT in a vectored exception handler.  This needs
 * NO length-disassembler (the E9-jmp trampoline approach would, to know how many
 * head bytes to relocate) and NO vendored detour library — ~120 lines.
 *
 * Resume protocol (the classic software-breakpoint single-step dance):
 *   BP at VA  → run the registered callback (it observes/edits the CONTEXT, e.g.
 *               reads thiscall ecx / stack args, writes engine globals) → restore
 *               the original byte, rewind Eip to VA, set the TRAP flag, remember
 *               the VA as "pending re-arm" → CONTINUE_EXECUTION (the CPU now
 *               executes the real first instruction IN PLACE — correct even for
 *               rel call/jmp since the address is unchanged).
 *   #DB (single-step) → re-write 0xCC at the pending VA, clear TF → CONTINUE.
 *
 * A callback may request a PERMANENT disarm (detour_request_disarm) for one-shot
 * hooks (e.g. the title seed-pin fires per-sparkle; disarm after the first) so we
 * pay the exception cost exactly once.
 *
 * Concurrency: the hooked VAs are engine MAIN-THREAD code; the proxy's harness
 * thread only touches Win32 UI.  So a single global "pending re-arm" slot guarded
 * by the thread id is safe (no two threads single-step the same hook at once).
 * The VEH passes through every exception that is not a BP/SS at one of our VAs.
 */
#ifndef OSS_VA_DETOUR_H
#define OSS_VA_DETOUR_H

#include <windows.h>

#include "proxy_log.h"

typedef void (*detour_cb)(PCONTEXT ctx);

typedef struct {
    DWORD     va;
    BYTE      orig;
    detour_cb cb;
    int       armed;
} detour_entry;

#define DETOUR_MAX 32
static detour_entry g_detours[DETOUR_MAX];
static int   g_detour_n = 0;
static PVOID g_detour_veh = NULL;

/* single-step re-arm bookkeeping (main-thread only — see header note) */
static volatile LONG g_detour_pending = 0;   /* VA pending re-arm, 0 = none */
static DWORD g_detour_pending_tid = 0;
/* set by a callback to disarm its hook permanently (one-shot) */
static int   g_detour_disarm_req = 0;

static const DWORD TRAP_FLAG = 0x100;

static void detour_patch_byte(DWORD va, BYTE val)
{
    DWORD old;
    if (VirtualProtect((void *)va, 1, PAGE_EXECUTE_READWRITE, &old)) {
        *(volatile BYTE *)va = val;
        VirtualProtect((void *)va, 1, old, &old);
        FlushInstructionCache(GetCurrentProcess(), (void *)va, 1);
    } else {
        proxy_logf("[detour] VirtualProtect failed @0x%lx", (unsigned long)va);
    }
}

static detour_entry *detour_find(DWORD va)
{
    for (int i = 0; i < g_detour_n; ++i)
        if (g_detours[i].va == va && g_detours[i].armed)
            return &g_detours[i];
    return NULL;
}

/* A registered callback calls this to request its hook be removed after this
 * hit (the original byte is restored permanently, never re-armed). */
static void detour_request_disarm(void) { g_detour_disarm_req = 1; }

static LONG CALLBACK detour_veh(PEXCEPTION_POINTERS ep)
{
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    PCONTEXT ctx = ep->ContextRecord;

    if (code == EXCEPTION_BREAKPOINT) {
        DWORD addr = (DWORD)(ULONG_PTR)ep->ExceptionRecord->ExceptionAddress;
        detour_entry *e = detour_find(addr);
        if (!e) return EXCEPTION_CONTINUE_SEARCH;   /* not ours */

        g_detour_disarm_req = 0;
        e->cb(ctx);

        detour_patch_byte(e->va, e->orig);          /* expose the real byte */
        ctx->Eip = e->va;                           /* rewind over the INT3 */

        if (g_detour_disarm_req) {
            e->armed = 0;                            /* one-shot: gone for good */
            g_detour_disarm_req = 0;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        ctx->EFlags |= TRAP_FLAG;                    /* single-step the real op */
        g_detour_pending = (LONG)e->va;
        g_detour_pending_tid = GetCurrentThreadId();
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (code == EXCEPTION_SINGLE_STEP) {
        if (g_detour_pending &&
            GetCurrentThreadId() == g_detour_pending_tid) {
            detour_patch_byte((DWORD)g_detour_pending, 0xCC);  /* re-arm */
            g_detour_pending = 0;
            ctx->EFlags &= ~TRAP_FLAG;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

/* Register the VEH once.  Call before any detour_add. */
static void detour_init(void)
{
    if (g_detour_veh) return;
    /* first=1 → our handler runs before the engine's own SEH/VEH, so we see the
     * BP/SS before anything else can swallow it. */
    g_detour_veh = AddVectoredExceptionHandler(1, detour_veh);
    if (!g_detour_veh)
        proxy_logf("[detour] FATAL: AddVectoredExceptionHandler failed err=%lu",
                   GetLastError());
}

/* Hook the function at absolute `va`; `cb` runs at its entry (before the
 * prologue → ecx/edx are thiscall this/arg, [esp+4] is the first stack arg). */
static void detour_add(DWORD va, detour_cb cb)
{
    if (!g_detour_veh) detour_init();
    if (g_detour_n >= DETOUR_MAX) {
        proxy_logf("[detour] table full, dropping 0x%lx", (unsigned long)va);
        return;
    }
    detour_entry *e = &g_detours[g_detour_n++];
    e->va = va;
    e->cb = cb;
    e->orig = *(volatile BYTE *)va;
    e->armed = 1;
    detour_patch_byte(va, 0xCC);
    proxy_logf("[detour] hooked 0x%lx (orig=0x%02x)",
               (unsigned long)va, e->orig);
}

#endif /* OSS_VA_DETOUR_H */
