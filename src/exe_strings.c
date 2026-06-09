/*
 * exe_strings.c — pure PE32 VA->string resolver.  See exe_strings.h.
 *
 * No Win32: parses the raw PE file bytes (DOS header -> NT headers -> section
 * table), maps the absolute VA to a file offset, and returns the NUL-terminated
 * string there.  Every access is bounds-checked against `len`.
 */
#include "exe_strings.h"

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

const char *pe_string_at(const uint8_t *image, size_t len, uint32_t va)
{
    if (image == NULL || len < 0x40)
        return NULL;
    if (image[0] != 'M' || image[1] != 'Z')              /* IMAGE_DOS_SIGNATURE */
        return NULL;

    uint32_t e_lfanew = rd32(image + 0x3c);
    if ((size_t)e_lfanew + 0x18 > len)                   /* room for PE sig+COFF */
        return NULL;

    const uint8_t *nt = image + e_lfanew;
    if (nt[0] != 'P' || nt[1] != 'E' || nt[2] != 0 || nt[3] != 0)
        return NULL;                                     /* IMAGE_NT_SIGNATURE   */

    uint16_t nsec   = rd16(nt + 6);                      /* NumberOfSections     */
    uint16_t opt_sz = rd16(nt + 0x14);                   /* SizeOfOptionalHeader */
    const uint8_t *opt = nt + 0x18;                      /* OptionalHeader       */
    if ((size_t)(opt - image) + opt_sz > len)
        return NULL;
    if (rd16(opt) != 0x10b)                              /* PE32 (not PE32+)     */
        return NULL;

    uint32_t imagebase = rd32(opt + 0x1c);
    if (va < imagebase)
        return NULL;
    uint32_t rva = va - imagebase;

    const uint8_t *sec = opt + opt_sz;                   /* section table        */
    for (uint16_t i = 0; i < nsec; i++) {
        const uint8_t *s = sec + (size_t)i * 0x28;
        if ((size_t)(s - image) + 0x28 > len)
            return NULL;
        uint32_t vsz    = rd32(s + 8);                   /* VirtualSize          */
        uint32_t vaddr  = rd32(s + 12);                  /* VirtualAddress       */
        uint32_t rawsz  = rd32(s + 16);                  /* SizeOfRawData        */
        uint32_t rawptr = rd32(s + 20);                  /* PointerToRawData     */
        uint32_t span   = (vsz > rawsz) ? vsz : rawsz;   /* the larger window    */
        if (rva >= vaddr && rva < vaddr + span) {
            uint32_t off = rva - vaddr + rawptr;
            if (off >= len)
                return NULL;
            /* Require a NUL terminator within the mapped image. */
            for (size_t k = off; k < len; k++)
                if (image[k] == 0)
                    return (const char *)(image + off);
            return NULL;
        }
    }
    return NULL;
}
