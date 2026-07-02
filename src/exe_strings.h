/*
 * exe_strings.{c,h} — read static STRINGS out of the user's own sotes.exe at
 * runtime, by virtual address.
 *
 * The cutscene/dialogue STORY TEXT (and the dramatist NAMES) live in the retail
 * exe's .data section as plain NUL-terminated ASCII referenced from .rdata by VA
 * (e.g. line 1 of the town intro is at VA 0x86d58c: "Ahh, here we are at
 * last!%nLook, Arche. This is our new hometown.").  Per the project legal line +
 * the dramatist-table precedent (docs/proofs/dramatist-table.md), the port must
 * NOT embed game story content as source — it reads it from the user's own file.
 *
 * The Steam .bind DRM only encrypts the entry-point/code path; .data is
 * untouched (verified: the string is byte-identical at file offset 0x46d58c in
 * BOTH the installed packed sotes.exe and vendor/unpacked).  So a plain raw-file
 * read of the user's installed sotes.exe yields the text — no unpack needed.
 *
 * Win32-free split (mirrors glyph_render / cs_dispatch): pe_string_at() is pure
 * PE32 parsing over a byte buffer (host-tested with a synthetic PE);
 * exe_data_string() (exe_strings_win32.c) maps the user's sotes.exe and calls it.
 */
#ifndef OPENSUMMONERS_EXE_STRINGS_H
#define OPENSUMMONERS_EXE_STRINGS_H

#include <stddef.h>
#include <stdint.h>

/* Resolve `va` (an absolute virtual address, e.g. 0x86d58c) to a pointer into
 * the raw PE image `image` (the exe's file bytes, `len` long) and return the
 * NUL-terminated ASCII string there, or NULL if: image is malformed / not PE32,
 * the VA falls in no section, the computed file offset is out of range, or the
 * string has no terminator within the image.  All reads are bounded by `len`
 * (ASan-clean).  The returned pointer is valid while `image` stays mapped.
 *
 * Pure (no Win32) — the host-testable core. */
const char *pe_string_at(const uint8_t *image, size_t len, uint32_t va);

/* Resolve `va` to a pointer into `image` guaranteeing `n` readable bytes, or
 * NULL.  Same section walk as pe_string_at, for fixed-size static TABLES the
 * engine references by VA (e.g. the 0x5cc410/0x5cc430 slope height-profile
 * ramps the collision grid's region-B +0x8 points at).  Pure (no Win32). */
const uint8_t *pe_bytes_at(const uint8_t *image, size_t len, uint32_t va,
                           size_t n);

/* Map the user's installed sotes.exe (game-dir CWD) once and return the static
 * string at virtual address `va`, or NULL if the file can't be mapped or the VA
 * doesn't resolve.  The mapping persists for the process lifetime (released by
 * engine shutdown).  Backed by exe_strings_win32.c in the real build. */
const char *exe_data_string(uint32_t va);

/* Same mapping, `n` bytes at `va` (NULL if unmapped/out of range).  Used by
 * the live slope-profile resolver (collision.h coll_slope_fn): retail reads
 * the height byte straight off its own .rdata; the port reads the identical
 * byte off the user's installed sotes.exe. */
const uint8_t *exe_data_bytes(uint32_t va, size_t n);

#endif /* OPENSUMMONERS_EXE_STRINGS_H */
