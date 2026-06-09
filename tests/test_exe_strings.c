/*
 * tests/test_exe_strings.c — the pure PE32 VA->string resolver (src/exe_strings.c,
 * pe_string_at).  Builds a minimal-but-valid synthetic PE32 in a buffer (DOS
 * header -> PE sig -> COFF -> optional header w/ ImageBase 0x400000 -> one
 * ".data" section vaddr 0x1000 / rawptr 0x200) with two known strings, and
 * asserts the VA->offset math + bounds checks.  No Win32 (exe_strings_win32.c
 * is the real-build sink and is not linked here).
 */
#include "t.h"
#include "exe_strings.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void w16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void w32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;        p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

#define IMG_LEN   0x400
#define IMAGEBASE 0x400000u
#define SEC_VADDR 0x1000u
#define SEC_RAW   0x200u

/* Build the synthetic PE into `buf` (IMG_LEN bytes, pre-zeroed by caller). */
static void build_pe(uint8_t *buf)
{
    buf[0] = 'M'; buf[1] = 'Z';
    w32(buf + 0x3c, 0x80);                 /* e_lfanew -> PE header at 0x80 */

    uint8_t *nt = buf + 0x80;
    nt[0] = 'P'; nt[1] = 'E'; nt[2] = 0; nt[3] = 0;
    w16(nt + 4, 0x14c);                    /* Machine = i386              */
    w16(nt + 6, 1);                        /* NumberOfSections            */
    w16(nt + 0x14, 0x60);                  /* SizeOfOptionalHeader        */

    uint8_t *opt = nt + 0x18;              /* OptionalHeader              */
    w16(opt + 0, 0x10b);                   /* Magic = PE32                */
    w32(opt + 0x1c, IMAGEBASE);            /* ImageBase                   */

    uint8_t *sec = opt + 0x60;             /* section table               */
    memcpy(sec, ".data\0\0", 8);
    w32(sec + 8,  0x1000);                 /* VirtualSize                 */
    w32(sec + 12, SEC_VADDR);              /* VirtualAddress              */
    w32(sec + 16, SEC_RAW);                /* SizeOfRawData               */
    w32(sec + 20, SEC_RAW);                /* PointerToRawData            */

    memcpy(buf + SEC_RAW + 0x00, "Hello, town!", 13);   /* VA 0x401000   */
    memcpy(buf + SEC_RAW + 0x20, "Arche's Father", 15); /* VA 0x401020   */
}

/* VA of a string at section-relative byte `rel` (file offset SEC_RAW+rel). */
static uint32_t va_of(uint32_t rel) { return IMAGEBASE + SEC_VADDR + rel; }

int test_exe_strings_resolve(void)
{
    uint8_t buf[IMG_LEN];
    memset(buf, 0, sizeof buf);
    build_pe(buf);

    const char *s = pe_string_at(buf, sizeof buf, va_of(0x00));
    T_ASSERT(s != NULL);
    T_ASSERT(strcmp(s, "Hello, town!") == 0);

    const char *n = pe_string_at(buf, sizeof buf, va_of(0x20));
    T_ASSERT(n != NULL);
    T_ASSERT(strcmp(n, "Arche's Father") == 0);
    return 0;
}

int test_exe_strings_bounds(void)
{
    uint8_t buf[IMG_LEN];
    memset(buf, 0, sizeof buf);
    build_pe(buf);

    /* VA below ImageBase -> NULL */
    T_ASSERT(pe_string_at(buf, sizeof buf, 0x100) == NULL);
    /* VA in no section (RVA far past the one section's span) -> NULL */
    T_ASSERT(pe_string_at(buf, sizeof buf, IMAGEBASE + 0x900000) == NULL);
    /* VA whose file offset would fall past the image end -> NULL.
     * RVA 0x1fff -> offset SEC_RAW+0xfff = 0x11ff >= IMG_LEN(0x400). */
    T_ASSERT(pe_string_at(buf, sizeof buf, va_of(0xfff)) == NULL);
    return 0;
}

int test_exe_strings_malformed(void)
{
    uint8_t buf[IMG_LEN];
    memset(buf, 0, sizeof buf);
    build_pe(buf);

    T_ASSERT(pe_string_at(NULL, 0, 0x401000) == NULL);
    T_ASSERT(pe_string_at(buf, 4, 0x401000) == NULL);   /* too short */

    buf[0] = 'X';                                        /* break MZ  */
    T_ASSERT(pe_string_at(buf, sizeof buf, 0x401000) == NULL);
    buf[0] = 'M';
    buf[0x80] = 'X';                                     /* break PE  */
    T_ASSERT(pe_string_at(buf, sizeof buf, 0x401000) == NULL);
    return 0;
}

/* A string with no NUL within the image must be rejected (no over-read). */
int test_exe_strings_unterminated(void)
{
    uint8_t buf[IMG_LEN];
    memset(buf, 0xAA, sizeof buf);   /* no zero bytes anywhere */
    build_pe(buf);
    /* re-fill the tail of the section raw data with non-zero so the string at
     * the last reachable offset has no terminator before IMG_LEN. */
    memset(buf + SEC_RAW + 0x40, 0x41, IMG_LEN - (SEC_RAW + 0x40));
    const char *s = pe_string_at(buf, sizeof buf, va_of(0x40));
    T_ASSERT(s == NULL);
    return 0;
}

/* Validate the VA math against a REAL sotes.exe layout (the .data section's
 * alignment differs from the synthetic PE).  Reads the unpacked vendor copy if
 * present; T_SKIP otherwise (CI / fresh checkout has no vendor binary). */
int test_exe_strings_real_exe(void)
{
    const char *path = "../vendor/unpacked/sotes.unpacked.exe";
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        T_SKIP("no %s (vendor binary not extracted)", path);

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); T_SKIP("empty %s", path); }

    uint8_t *img = (uint8_t *)malloc((size_t)n);
    T_ASSERT(img != NULL);
    size_t got = fread(img, 1, (size_t)n, f);
    fclose(f);
    T_ASSERT(got == (size_t)n);

    /* Town-intro line 1 @ VA 0x86d58c (r2: paddr 0x46d58c, .data). */
    const char *s = pe_string_at(img, (size_t)n, 0x86d58c);
    if (s == NULL) { free(img); T_FAIL("VA 0x86d58c did not resolve"); }
    if (strncmp(s, "Ahh, here we are at last!", 24) != 0) {
        char head[40]; snprintf(head, sizeof head, "%.30s", s);
        free(img);
        T_FAIL("VA 0x86d58c = \"%s...\" (want \"Ahh, here we are at last!...\")", head);
    }
    free(img);
    return 0;
}
