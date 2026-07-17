/* sotes_save — self-contained reader for Fortune Summoners EN-SE `savedataNN.sdt`.
 *
 * NO deps beyond libc (stdint/stddef/stdlib/string/stdio).  Reusable outside the
 * trainer: a save editor, the port's save subsystem, or headless inspection.
 *
 * Format RE'd off the EN-SE loader `FUN_00416550` + archive class `FUN_005dee40`/
 * `FUN_005df030` (`sotes-ense-en.exe`, ImageBase 0x400000; VA=fileoff+0x400000).
 * See sotes_save.c for the byte-level provenance.  Ground-truth verified against all
 * 8 real saves (magic/handle 8/8; roster matches party growth: slot1 = Arche+Sana).
 *
 * CONTAINER (plaintext prefix, then obfuscated body):
 *   [u32 len=0x10][u32 magic=0x2711][u32 bodysize][u32 val3][u32 seed]  (0x14 bytes)
 *   then `bodysize` obfuscated bytes.  Decode (per byte i of the body):
 *     key      = (seed >> 8) & 0xff
 *     plain[i] = INV_KEYSTR[(cipher[i] - key) & 0xff]
 *   where INV_KEYSTR is the inverse permutation of the 256-byte key table at
 *   VA 0x5fd290 (embedded here).  The decoded body is itself a record stream;
 *   record 0 = a 604-byte metadata block (magic@+0x22c, category handle@+0x230).
 */
#ifndef SOTES_SAVE_H
#define SOTES_SAVE_H

#include <stddef.h>
#include <stdint.h>

#define SOTES_SDT_MAGIC        0x2711u   /* metadata + container magic          */
#define SOTES_SAVE_MAINQUEST   0x2738u   /* category handle: "Main Quest" save  */
#define SOTES_MAX_PARTY        8

/* Container header (the 0x14-byte plaintext prefix). */
typedef struct sotes_sdt_header {
    uint32_t prefix_len;   /* file+0x00, always 0x10 (bytes of header that follow) */
    uint32_t magic;        /* file+0x04, SOTES_SDT_MAGIC                           */
    uint32_t bodysize;     /* file+0x08, decoded-body length                       */
    uint32_t val3;         /* file+0x0c, per-save value (checksum-ish; not verified)*/
    uint32_t seed;         /* file+0x10, cipher seed                               */
    uint8_t  key;          /* derived: (seed >> 8) & 0xff                          */
} sotes_sdt_header;

/* One playable party member found in the roster. */
typedef struct sotes_member {
    uint32_t code;         /* character code (0xc35a Arche / 0xc35b Sana / 0xc35c Stella) */
    char     name[16];     /* resolved display name, "" if code unknown                   */
    int32_t  level_base;   /* body[code_off+4] = stat +0xe0 level_base (NOT display Lv;    */
                           /*   the SE derives the shown level from EXP — see trainer)     */
    size_t   body_off;     /* offset of the code word in the decoded body                 */
} sotes_member;

/* Parsed summary of one save. */
typedef struct sotes_save_info {
    int      ok;           /* header decoded (magic present, body length sane)     */
    int      valid;        /* ok && category handle == Main Quest                  */
    uint32_t handle;       /* category handle @ metadata+0x230                     */
    uint32_t checksum;     /* metadata+0x228 (per-save; additive body sum, unverified)*/
    size_t   file_size;    /* raw .sdt byte size                                   */
    size_t   body_size;    /* decoded body length                                  */
    sotes_sdt_header hdr;

    int          party_count;
    sotes_member party[SOTES_MAX_PARTY];

    /* Raw party-header field grid: 16 u32 at body[metadata_end] (always body 0x260
     * since the metadata block is a fixed 604 bytes).  Meanings are only PARTIALLY
     * RE'd — several fields grow monotonically over a playthrough (candidates:
     * progress / playtime / gold); exposed raw so callers can use/label them without
     * this library fabricating names.  See sotes_save.c "PARTY-HEADER GRID". */
    uint32_t ph[16];
    int      ph_present;   /* the grid was in range and read                       */
} sotes_save_info;

/* Decode a whole .sdt file image to a fresh malloc'd plaintext body.
 * Fills *hdr (if non-NULL) and *body_len (decoded length).
 * Returns the body (caller free()s) or NULL on a malformed/short header. */
uint8_t *sotes_sdt_decode(const uint8_t *file, size_t file_len,
                          sotes_sdt_header *hdr, size_t *body_len);

/* Parse an already-decoded body into a summary.  Returns 0 on success (out->ok set),
 * <0 if the body is too short to hold the metadata block. */
int sotes_save_parse(const uint8_t *body, size_t body_len, size_t file_size,
                     const sotes_sdt_header *hdr, sotes_save_info *out);

/* Convenience: read+decode+parse a path.  Returns 0 ok, <0 on IO/decode error.
 * Frees the decoded body internally (summary only). */
int sotes_save_read(const char *path, sotes_save_info *out);

/* code -> display name; returns "" for an unknown code (never NULL). */
const char *sotes_char_name(uint32_t code);

#endif /* SOTES_SAVE_H */
