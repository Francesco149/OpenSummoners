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

/* Shared VA -> file-offset resolution (PE32 section walk).  Returns the file
 * offset of `va` or (size_t)-1 if unresolvable. */
static size_t pe_va_to_off(const uint8_t *image, size_t len, uint32_t va)
{
    if (image == NULL || len < 0x40)
        return (size_t)-1;
    if (image[0] != 'M' || image[1] != 'Z')
        return (size_t)-1;

    uint32_t e_lfanew = rd32(image + 0x3c);
    if ((size_t)e_lfanew + 0x18 > len)
        return (size_t)-1;

    const uint8_t *nt = image + e_lfanew;
    if (nt[0] != 'P' || nt[1] != 'E' || nt[2] != 0 || nt[3] != 0)
        return (size_t)-1;

    uint16_t nsec   = rd16(nt + 6);
    uint16_t opt_sz = rd16(nt + 0x14);
    const uint8_t *opt = nt + 0x18;
    if ((size_t)(opt - image) + opt_sz > len)
        return (size_t)-1;
    if (rd16(opt) != 0x10b)
        return (size_t)-1;

    uint32_t imagebase = rd32(opt + 0x1c);
    if (va < imagebase)
        return (size_t)-1;
    uint32_t rva = va - imagebase;

    const uint8_t *sec = opt + opt_sz;
    for (uint16_t i = 0; i < nsec; i++) {
        const uint8_t *s = sec + (size_t)i * 0x28;
        if ((size_t)(s - image) + 0x28 > len)
            return (size_t)-1;
        uint32_t vsz    = rd32(s + 8);
        uint32_t vaddr  = rd32(s + 12);
        uint32_t rawsz  = rd32(s + 16);
        uint32_t rawptr = rd32(s + 20);
        uint32_t span   = (vsz > rawsz) ? vsz : rawsz;
        if (rva >= vaddr && rva < vaddr + span) {
            size_t off = (size_t)(rva - vaddr) + rawptr;
            return off < len ? off : (size_t)-1;
        }
    }
    return (size_t)-1;
}

const uint8_t *pe_bytes_at(const uint8_t *image, size_t len, uint32_t va,
                           size_t n)
{
    size_t off = pe_va_to_off(image, len, va);
    if (off == (size_t)-1 || n > len - off)
        return NULL;
    return image + off;
}

const char *pe_string_at(const uint8_t *image, size_t len, uint32_t va)
{
    size_t off = pe_va_to_off(image, len, va);
    if (off == (size_t)-1)
        return NULL;
    /* Require a NUL terminator within the mapped image. */
    for (size_t k = off; k < len; k++)
        if (image[k] == 0)
            return (const char *)(image + off);
    return NULL;
}
