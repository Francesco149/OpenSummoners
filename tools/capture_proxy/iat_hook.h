/* iat_hook.h — patch the main exe's Import Address Table.
 *
 * The retail exe imports kernel32!GetTickCount, user32!ShowWindow, etc. by
 * name.  Swapping the IAT slot redirects every call site with zero per-call
 * overhead (no trampoline, no exception) — the right tool for the
 * high-frequency clock/window hooks.  Returns the original function pointer so
 * the replacement can forward.
 *
 * Safe to call from DllMain (pure memory writes via VirtualProtect; no loader
 * calls), provided the target import's DLL is already mapped — kernel32 /
 * user32 / gdi32 always are by the time our proxy's DllMain runs.
 */
#ifndef OSS_IAT_HOOK_H
#define OSS_IAT_HOOK_H

#include <windows.h>
#include <string.h>

#include "proxy_log.h"

static int oss_streqi(const char *a, const char *b)
{
    return lstrcmpiA(a, b) == 0;
}

/* Patch `func` imported from `dll` in `module` (NULL = main exe) to `repl`.
 * Returns the previous slot value (the real function), or NULL if not found. */
static void *iat_hook(HMODULE module, const char *dll, const char *func,
                      void *repl)
{
    if (!module) module = GetModuleHandleA(NULL);
    BYTE *base = (BYTE *)module;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;

    IMAGE_DATA_DIRECTORY imp_dir =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!imp_dir.VirtualAddress) return NULL;

    IMAGE_IMPORT_DESCRIPTOR *imp =
        (IMAGE_IMPORT_DESCRIPTOR *)(base + imp_dir.VirtualAddress);

    for (; imp->Name; ++imp) {
        const char *modname = (const char *)(base + imp->Name);
        if (!oss_streqi(modname, dll)) continue;

        /* OriginalFirstThunk holds the names; FirstThunk is the live IAT.
         * Some linkers null OriginalFirstThunk — fall back to FirstThunk. */
        IMAGE_THUNK_DATA *name_thunk = imp->OriginalFirstThunk
            ? (IMAGE_THUNK_DATA *)(base + imp->OriginalFirstThunk)
            : (IMAGE_THUNK_DATA *)(base + imp->FirstThunk);
        IMAGE_THUNK_DATA *iat_thunk =
            (IMAGE_THUNK_DATA *)(base + imp->FirstThunk);

        for (; name_thunk->u1.AddressOfData; ++name_thunk, ++iat_thunk) {
            if (IMAGE_SNAP_BY_ORDINAL(name_thunk->u1.Ordinal))
                continue;  /* imported by ordinal — no name to match */
            IMAGE_IMPORT_BY_NAME *ibn =
                (IMAGE_IMPORT_BY_NAME *)(base + name_thunk->u1.AddressOfData);
            if (!oss_streqi((const char *)ibn->Name, func)) continue;

            void **slot = (void **)&iat_thunk->u1.Function;
            void *orig = *slot;
            DWORD oldp;
            if (VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &oldp)) {
                *slot = repl;
                VirtualProtect(slot, sizeof(void *), oldp, &oldp);
                proxy_logf("[iat] hooked %s!%s slot=%p orig=%p repl=%p",
                           dll, func, (void *)slot, orig, repl);
                return orig;
            }
            proxy_logf("[iat] VirtualProtect failed for %s!%s", dll, func);
            return NULL;
        }
    }
    proxy_logf("[iat] NOT FOUND: %s!%s", dll, func);
    return NULL;
}

#endif /* OSS_IAT_HOOK_H */
