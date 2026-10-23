/* retail_map.h — map sotes.unpacked.exe (any edition) at its fixed ImageBase inside a
 * host process WITHOUT running its entry point ("retail as a library").
 * Wine/Windows populate 0x3c0000..0x8b0000 early, so a plain VirtualAlloc at
 * 0x400000 fails. Instead the host links at 0x200000 WITHOUT ASLR
 * (-Wl,--disable-dynamicbase) and carries RETAIL_HOLE_DEFINE, a giant .bss
 * that the PE loader reserves for us; retail_map() then VirtualProtect()s it
 * RWX and copies retail's sections in. */
#pragma once
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Host exes link at a low base (-Wl,--image-base,0x200000) and carry this
 * bss hole so the PE loader reserves the retail range for us before wine (or
 * Windows) puts heaps / mapped files there. */
#define RETAIL_HOLE_DEFINE \
    __attribute__((aligned(4096))) char g_retail_hole[0x4c00000];   /* sotes SizeOfImage 0x3d0f000 (EN-old) .. ~0x46xxxxx (SE) */

/* every byte range a map copied from the FILE (the standalone's audit: none may be code) */
typedef struct { char name[9]; uint32_t va, len, ch; int copied; } retail_range_t;
#define RETAIL_MAX_RANGES 24
typedef struct {
    uint8_t *base;
    uint32_t size;
    uint32_t entry;
    int n_imports_resolved, n_imports_stubbed;
    retail_range_t r[RETAIL_MAX_RANGES]; int nr;   /* the headers + each section: copied, or not (code) */
} retail_image_t;

/* a section that holds or may run code: never copied by a data-only map */
#define RETAIL_SEC_CODE(ch) (((ch) & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE)) != 0)

/* copy the PE headers + every NON-code section of the file at `file` into `base` (each range noted in `out`);
 * code sections are left as `fill` bytes.  The standalone's only way to take bytes from the game's files: a
 * product build (RH_PRODUCT) has no other. */
static void retail_copy_data(const uint8_t *file, uint8_t *base, retail_image_t *out, uint8_t fill)
{
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(file + ((IMAGE_DOS_HEADER *)file)->e_lfanew);
    uint32_t ib = nt->OptionalHeader.ImageBase;
    (void)ib;
    memcpy(base, file, nt->OptionalHeader.SizeOfHeaders);
    out->nr = 0;
    retail_range_t *h = &out->r[out->nr++];
    snprintf(h->name, sizeof h->name, "(hdr)"); h->va = (uint32_t)(uintptr_t)base; h->len = nt->OptionalHeader.SizeOfHeaders;
    h->ch = 0; h->copied = 1;
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections && out->nr < RETAIL_MAX_RANGES; i++) {
        uint32_t raw = sec[i].SizeOfRawData, vs = sec[i].Misc.VirtualSize ? sec[i].Misc.VirtualSize : raw;
        if (raw > vs) raw = vs;
        retail_range_t *r = &out->r[out->nr++];
        memcpy(r->name, sec[i].Name, 8); r->name[8] = 0;
        r->va = (uint32_t)(uintptr_t)base + sec[i].VirtualAddress; r->len = vs; r->ch = sec[i].Characteristics;
        r->copied = !RETAIL_SEC_CODE(sec[i].Characteristics);
        if (r->copied) { if (raw) memcpy(base + sec[i].VirtualAddress, file + sec[i].PointerToRawData, raw); }
        else memset(base + sec[i].VirtualAddress, fill, vs);
    }
}

/* Any import we could not resolve lands here: crash loudly with the slot. */
static void __cdecl retail_unresolved_import(void)
{
    fprintf(stderr, "[retail_map] call through UNRESOLVED import\n");
    ExitProcess(99);
}

/* SteamStub 3.1 (Steam's own sotes.exe): .text encrypted + a .bind stub section, the header's SizeOfImage / entry the
 * stub's.  Every data section and the import directory are the unpacked file's byte for byte, so the standalone (no
 * code mapped) reads the packed file as it is: the header COPY gets the original fields back */
typedef struct { uint32_t packed_size, size, entry_rva; } retail_packed_t;
static const retail_packed_t RETAIL_PACKED[] = { { 0x3d6b000u, 0x3d0f000u, 0x1c0a8fu } };   /* EN-old (2012) */

/* data_only: the standalone's map (retail_copy_data: no code section copied, code ranges INT3) */
static int retail_map(const char *path, retail_image_t *out, int resolve_imports, int data_only)
{
#ifdef RH_PRODUCT
    data_only = 1;   /* the product cannot map retail code */
#endif
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "open %s failed\n", path); return -1; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *file = malloc(n);
    if (fread(file, 1, n, f) != (size_t)n) { fclose(f); return -1; }
    fclose(f);

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)file;
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(file + dos->e_lfanew);
    uint32_t ib = nt->OptionalHeader.ImageBase, sz = nt->OptionalHeader.SizeOfImage;
    uint8_t *base = VirtualAlloc((void *)(uintptr_t)ib, sz, MEM_RESERVE | MEM_COMMIT,
                                 PAGE_EXECUTE_READWRITE);
    if (!base) {
        /* Range already owned by our own image (the RETAIL_HOLE bss): make it RWX. */
        DWORD old;
        if (VirtualProtect((void *)(uintptr_t)ib, sz, PAGE_EXECUTE_READWRITE, &old)) {
            base = (uint8_t *)(uintptr_t)ib;
            memset(base, 0, sz);
        }
    }
    if (base == (uint8_t *)(uintptr_t)ib) {
        /* the whole image must sit inside memory we own: fail loudly if the hole is short */
        MEMORY_BASIC_INFORMATION mi;
        if (!VirtualQuery((void *)(uintptr_t)(ib + sz - 1), &mi, sizeof mi) || mi.State != MEM_COMMIT) {
            fprintf(stderr, "retail_map: image end %08x not inside the host hole\n", ib + sz);
            return -1;
        }
    }
    if (base != (uint8_t *)(uintptr_t)ib) {
        fprintf(stderr, "VirtualAlloc @%08x size %08x failed (got %p, err %lu)\n",
                ib, sz, base, GetLastError());
        return -1;
    }
    int packed = 0;
    IMAGE_SECTION_HEADER *sh = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) packed |= !memcmp(sh[i].Name, ".bind", 6);
    if (packed && !data_only) { fprintf(stderr, "%s: SteamStub-packed (.text encrypted): Steamless it for a retail run\n", path); return -1; }
    if (data_only) {
        retail_copy_data(file, base, out, 0xCC);
    }
#ifndef RH_PRODUCT
    else {   /* rhost's pure / hybrid runs: the whole image, code included */
        memcpy(base, file, nt->OptionalHeader.SizeOfHeaders);
        IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
            uint32_t raw = sec[i].SizeOfRawData;
            if (raw > sec[i].Misc.VirtualSize && sec[i].Misc.VirtualSize) raw = sec[i].Misc.VirtualSize;
            if (raw) memcpy(base + sec[i].VirtualAddress, file + sec[i].PointerToRawData, raw);
        }
        out->nr = 0;
    }
#endif
    if (packed) {   /* (data-only: refused above otherwise) */
        IMAGE_NT_HEADERS32 *mn = (IMAGE_NT_HEADERS32 *)(base + dos->e_lfanew);
        size_t k = 0;
        while (k < sizeof RETAIL_PACKED / sizeof *RETAIL_PACKED && RETAIL_PACKED[k].packed_size != sz) k++;
        if (k == sizeof RETAIL_PACKED / sizeof *RETAIL_PACKED) { fprintf(stderr, "%s: unknown SteamStub build (SizeOfImage %08x)\n", path, sz); return -1; }
        mn->OptionalHeader.SizeOfImage = RETAIL_PACKED[k].size;
        mn->OptionalHeader.AddressOfEntryPoint = RETAIL_PACKED[k].entry_rva;
    }
    out->n_imports_resolved = out->n_imports_stubbed = 0;
    IMAGE_DATA_DIRECTORY *idd = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    for (IMAGE_IMPORT_DESCRIPTOR *d = (void *)(base + idd->VirtualAddress); d->Name; d++) {
        const char *dll = (const char *)(base + d->Name);
        HMODULE h = resolve_imports ? LoadLibraryA(dll) : NULL;
        IMAGE_THUNK_DATA32 *ilt = (void *)(base + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        IMAGE_THUNK_DATA32 *iat = (void *)(base + d->FirstThunk);
        for (; ilt->u1.AddressOfData; ilt++, iat++) {
            FARPROC p = NULL;
            if (h) {
                if (ilt->u1.Ordinal & IMAGE_ORDINAL_FLAG32)
                    p = GetProcAddress(h, (LPCSTR)(uintptr_t)(ilt->u1.Ordinal & 0xffff));
                else
                    p = GetProcAddress(h, (LPCSTR)((IMAGE_IMPORT_BY_NAME *)(base + ilt->u1.AddressOfData))->Name);
            }
            if (p) { iat->u1.Function = (uint32_t)(uintptr_t)p; out->n_imports_resolved++; }
            else   { iat->u1.Function = (uint32_t)(uintptr_t)retail_unresolved_import; out->n_imports_stubbed++; }
        }
    }
    out->base = base; out->size = sz;
    out->entry = ib + nt->OptionalHeader.AddressOfEntryPoint;
    free(file);
    return 0;
}

/* Call a retail function at `va` with no stack args (cdecl/stdcall-0 alike). */
static inline uint32_t retail_call0(uint32_t va) { return ((uint32_t (__cdecl *)(void))(uintptr_t)va)(); }
