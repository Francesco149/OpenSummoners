/*
 * dev_hooks — see dev_hooks.h for rationale.
 *
 * Implementation: classic 5-byte `jmp rel32` prologue patch on
 * user32!MessageBoxA / MessageBoxW.  Works because modern Windows
 * user32 exports start with the canonical hot-patch pad
 * (`8B FF 55 8B EC` = `mov edi, edi; push ebp; mov ebp, esp`), so the
 * first 5 bytes are safely overwritable — we save the originals and
 * restore on shutdown.
 *
 * 32-bit x86 only: rel32 jumps cover the full address space, so we
 * don't need a trampoline-relay for range; and we never call the
 * original (the whole point is to NOT block), so we don't need a
 * relocated copy of the displaced prologue either.  Simpler than
 * Microsoft Detours.
 */

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dev_hooks.h"

static uint8_t g_msgbox_a_save[5];
static uint8_t g_msgbox_w_save[5];
static void   *g_msgbox_a_addr;
static void   *g_msgbox_w_addr;
static int     g_hooks_installed;

/* Distinctive banner.  The leading tag is meant to be unmistakable in
 * agent-mode output — searchable verbatim, can't be mistaken for normal
 * `[opensummoners]` logging.  If you're skimming output and see this, the
 * process was about to block on a modal that we suppressed. */
#define MSGBOX_TAG "[!!! REDIRECTED MESSAGEBOX !!!]"

static void log_messagebox(const char *which, const char *caption,
                           const char *body, unsigned type)
{
    fprintf(stderr,
            "\n"
            "================================================================================\n"
            "%s %s — auto-returning IDOK\n"
            "  caption: %s\n"
            "  body:    %s\n"
            "  type:    0x%x\n"
            "================================================================================\n"
            "\n",
            MSGBOX_TAG, which,
            caption && *caption ? caption : "(empty)",
            body    && *body    ? body    : "(empty)",
            type);
    fflush(stderr);

    /* Also surface via OutputDebugString for DbgView capture. */
    char dbg[1024];
    snprintf(dbg, sizeof(dbg),
             "%s %s caption=\"%s\" body=\"%s\" type=0x%x\n",
             MSGBOX_TAG, which,
             caption ? caption : "",
             body    ? body    : "",
             type);
    OutputDebugStringA(dbg);
}

static int WINAPI hook_MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type)
{
    (void)hwnd;
    log_messagebox("MessageBoxA", caption, text, type);
    return IDOK;
}

static int WINAPI hook_MessageBoxW(HWND hwnd, LPCWSTR text, LPCWSTR caption, UINT type)
{
    (void)hwnd;
    char a_caption[512] = {0};
    char a_text   [1024] = {0};
    if (caption) WideCharToMultiByte(CP_UTF8, 0, caption, -1, a_caption, sizeof(a_caption), NULL, NULL);
    if (text)    WideCharToMultiByte(CP_UTF8, 0, text,    -1, a_text,    sizeof(a_text),    NULL, NULL);
    log_messagebox("MessageBoxW", a_caption, a_text, type);
    return IDOK;
}

/* Overwrite the first 5 bytes of `target` with `jmp rel32 → hook`, after
 * VirtualProtect-ing the page writable.  Saves the original bytes into
 * `saved[5]`. */
static int patch_prologue(void *target, void *hook, uint8_t saved[5], const char *label)
{
    DWORD old_protect;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old_protect)) {
        fprintf(stderr, "[dev_hooks] VirtualProtect(%s @ %p, RWX) failed: %lu\n",
                label, target, GetLastError());
        return 0;
    }
    memcpy(saved, target, 5);
    uint8_t patch[5];
    patch[0] = 0xE9;  /* jmp rel32 */
    int32_t rel = (int32_t)((intptr_t)hook - ((intptr_t)target + 5));
    memcpy(&patch[1], &rel, 4);
    memcpy(target, patch, 5);
    VirtualProtect(target, 5, old_protect, &old_protect);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
    return 1;
}

static void unpatch_prologue(void *target, const uint8_t saved[5])
{
    DWORD old_protect;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old_protect)) return;
    memcpy(target, saved, 5);
    VirtualProtect(target, 5, old_protect, &old_protect);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
}

int dev_hooks_install_messagebox(void)
{
    if (g_hooks_installed) return 1;

    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (!user32) user32 = LoadLibraryA("user32.dll");
    if (!user32) {
        fprintf(stderr, "[dev_hooks] user32.dll not loadable — MessageBox hooks skipped\n");
        return 0;
    }
    g_msgbox_a_addr = (void *)GetProcAddress(user32, "MessageBoxA");
    g_msgbox_w_addr = (void *)GetProcAddress(user32, "MessageBoxW");

    int ok = 1;
    if (g_msgbox_a_addr) {
        if (patch_prologue(g_msgbox_a_addr, (void *)hook_MessageBoxA,
                           g_msgbox_a_save, "MessageBoxA")) {
            fprintf(stderr, "[dev_hooks] hooked MessageBoxA @ %p\n", g_msgbox_a_addr);
        } else ok = 0;
    } else {
        fprintf(stderr, "[dev_hooks] MessageBoxA not exported (?!) — skipping\n");
    }
    if (g_msgbox_w_addr) {
        if (patch_prologue(g_msgbox_w_addr, (void *)hook_MessageBoxW,
                           g_msgbox_w_save, "MessageBoxW")) {
            fprintf(stderr, "[dev_hooks] hooked MessageBoxW @ %p\n", g_msgbox_w_addr);
        } else ok = 0;
    } else {
        fprintf(stderr, "[dev_hooks] MessageBoxW not exported (?!) — skipping\n");
    }
    fflush(stderr);

    g_hooks_installed = 1;
    return ok;
}

void dev_hooks_uninstall_messagebox(void)
{
    if (!g_hooks_installed) return;
    if (g_msgbox_a_addr) unpatch_prologue(g_msgbox_a_addr, g_msgbox_a_save);
    if (g_msgbox_w_addr) unpatch_prologue(g_msgbox_w_addr, g_msgbox_w_save);
    g_hooks_installed = 0;
}
