/*
 * exe_strings_win32.c — the Win32 sink for exe_data_string().  See exe_strings.h.
 *
 * Maps the user's installed sotes.exe (game-dir CWD, the same file
 * load_town_scene opens for resources) read-only as a flat byte image and hands
 * it to the pure pe_string_at().  Reads the user's own file — never embeds story
 * content.  Not compiled into the host tests (those exercise pe_string_at with a
 * synthetic PE).
 */
#include "exe_strings.h"

#include <stdio.h>
#include <windows.h>

static const uint8_t *g_exe_image = NULL;   /* mapped raw file bytes            */
static size_t         g_exe_len   = 0;
static int            g_exe_tried = 0;       /* one-shot: don't retry on failure */

static void map_exe_once(void)
{
    if (g_exe_tried)
        return;
    g_exe_tried = 1;

    HANDLE f = CreateFileA("sotes.exe", GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "exe_strings: CreateFileA(sotes.exe) failed: %lu — "
                "dialogue text unavailable\n", GetLastError());
        return;
    }
    DWORD sz = GetFileSize(f, NULL);
    HANDLE m = CreateFileMappingA(f, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(f);
    if (m == NULL) {
        fprintf(stderr, "exe_strings: CreateFileMapping(sotes.exe) failed: %lu\n",
                GetLastError());
        return;
    }
    void *base = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(m);                 /* the view stays valid after the handle close */
    if (base == NULL) {
        fprintf(stderr, "exe_strings: MapViewOfFile(sotes.exe) failed: %lu\n",
                GetLastError());
        return;
    }
    g_exe_image = (const uint8_t *)base;
    g_exe_len   = (size_t)sz;
}

const char *exe_data_string(uint32_t va)
{
    map_exe_once();
    if (g_exe_image == NULL)
        return NULL;
    return pe_string_at(g_exe_image, g_exe_len, va);
}

const uint8_t *exe_data_bytes(uint32_t va, size_t n)
{
    map_exe_once();
    if (g_exe_image == NULL)
        return NULL;
    return pe_bytes_at(g_exe_image, g_exe_len, va, n);
}
