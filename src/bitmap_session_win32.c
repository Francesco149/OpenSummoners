/*
 * src/bitmap_session_win32.c — Win32 primitives for the real Windows
 * build.  The host test harness provides its own stubs (see
 * tests/test_bitmap_session.c) and does NOT link this file.
 *
 * Single-TU build picks this up via the src/Makefile wildcard.
 */
#include "bitmap_session.h"

#include <windows.h>

void *bs_local_alloc_zeroed(uint32_t bytes)
{
    /* LMEM_ZEROINIT — matches FUN_005b71f0's `LocalAlloc(0x40, …)`
     * literal (0x40 == LMEM_ZEROINIT). */
    return (void *)LocalAlloc(LMEM_ZEROINIT, (SIZE_T)bytes);
}

void bs_local_free(void *p)
{
    if (p != NULL) {
        LocalFree((HLOCAL)p);
    }
}

const void *bs_load_pe_resource(void *hModule, uint16_t resource_id,
                                 const char *resource_type)
{
    HRSRC hres = FindResourceA((HMODULE)hModule,
                               MAKEINTRESOURCEA(resource_id),
                               resource_type);
    if (hres == NULL) return NULL;
    HGLOBAL hdata = LoadResource((HMODULE)hModule, hres);
    if (hdata == NULL) return NULL;
    return (const void *)LockResource(hdata);
}
