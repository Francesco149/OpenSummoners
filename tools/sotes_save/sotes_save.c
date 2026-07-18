/* sotes_save — see sotes_save.h.  EN-SE savedataNN.sdt reader (libc-only). */
#include "sotes_save.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── key table (VA 0x5fd290, 256-byte permutation P) ───────────────────────────
 * The archive decode (`FUN_005dee40` @0x5def06) does, per body byte i:
 *     t        = (cipher[i] - key) & 0xff
 *     plain[i] = KEYTABLE[t]
 * where KEYTABLE = P^-1 (built by `FUN_005df030`: KEYTABLE[P[j]] = j).  We embed P
 * and invert it once at first use.  key = (header.seed >> 8) & 0xff. */
static const uint8_t KEYSTR[256] = {
    0x37, 0x16, 0x3f, 0x51, 0x7a, 0x59, 0x3a, 0x52, 0x82, 0x2c, 0xdb, 0xb5,
    0x2f, 0x4d, 0x3e, 0xb6, 0x3b, 0x25, 0x43, 0xfd, 0xfa, 0x1e, 0x31, 0x2e,
    0xd9, 0x88, 0x86, 0x4e, 0xa7, 0x93, 0xa9, 0xa6, 0x5a, 0x95, 0x1c, 0x3d,
    0xb4, 0x63, 0x55, 0x4b, 0xba, 0xcc, 0xec, 0x10, 0xcd, 0xd3, 0x3c, 0xef,
    0x29, 0xa0, 0xa4, 0xe2, 0x48, 0x41, 0x64, 0x44, 0x4c, 0x56, 0xdf, 0x23,
    0xcf, 0xad, 0x87, 0x0f, 0x61, 0x46, 0x45, 0x96, 0xb0, 0x00, 0xb9, 0x74,
    0x33, 0xab, 0xde, 0x75, 0x53, 0x68, 0x9a, 0x99, 0x0c, 0x8e, 0x22, 0x1a,
    0x5c, 0x0d, 0x6f, 0x6c, 0x2b, 0xf8, 0x13, 0xc0, 0x24, 0x8a, 0x80, 0x89,
    0x0a, 0xb3, 0x81, 0x85, 0xf3, 0x98, 0xe7, 0x78, 0x49, 0xe1, 0x2a, 0xf7,
    0xae, 0x11, 0x27, 0x01, 0x5f, 0x35, 0x76, 0xdd, 0xa3, 0xaf, 0x50, 0xe6,
    0x8b, 0x7e, 0x91, 0x57, 0xc1, 0xb8, 0xf9, 0x73, 0x4f, 0x58, 0xbf, 0x05,
    0x15, 0xe0, 0x92, 0x36, 0xca, 0x8c, 0x54, 0x47, 0x7d, 0x7b, 0xf1, 0x79,
    0xed, 0x94, 0x9d, 0xc9, 0xfb, 0x6b, 0xc2, 0x42, 0x09, 0x66, 0xf5, 0xd2,
    0xfc, 0xf2, 0x20, 0x19, 0x1b, 0x39, 0xe8, 0x5e, 0x70, 0xa1, 0x7f, 0x9b,
    0x26, 0x62, 0xf0, 0xbe, 0x32, 0xc8, 0x4a, 0xb2, 0xd4, 0xa8, 0xb7, 0xeb,
    0x65, 0x90, 0x17, 0xce, 0xd0, 0x9c, 0x12, 0xd8, 0xaa, 0x04, 0x8f, 0x5b,
    0x2d, 0xb1, 0x6a, 0x07, 0x6e, 0x08, 0x77, 0xc3, 0x18, 0xbd, 0x9f, 0xa2,
    0xf4, 0x97, 0xf6, 0xc5, 0x1d, 0x71, 0xac, 0x14, 0xea, 0x69, 0x34, 0x60,
    0xcb, 0xa5, 0xe9, 0xee, 0x9e, 0x06, 0xc6, 0xe5, 0xff, 0x6d, 0xc4, 0x8d,
    0xe3, 0xd1, 0xbb, 0x38, 0xc7, 0x03, 0x30, 0x40, 0xda, 0xd6, 0x5d, 0xdc,
    0x0e, 0xbc, 0x67, 0x21, 0x1f, 0xe4, 0x02, 0xd7, 0x84, 0x7c, 0x72, 0x28,
    0xfe, 0x83, 0x0b, 0xd5,
};

static uint8_t g_invkey[256];
static int     g_invkey_ready;

static void invkey_init(void) {
    if (g_invkey_ready) return;
    for (int j = 0; j < 256; ++j) g_invkey[KEYSTR[j]] = (uint8_t)j;
    g_invkey_ready = 1;
}

/* ── code -> name ──────────────────────────────────────────────────────────────
 * Playable party codes (base_stat_table.c: 0xc35a/b/c).  A save's roster is exactly
 * these three as the party grows (Arche -> +Sana -> +Stella). */
static const struct { uint32_t code; const char *name; } CHAR_NAMES[] = {
    { 0xc35a, "Arche"  },
    { 0xc35b, "Sana"   },
    { 0xc35c, "Stella" },
};

const char *sotes_char_name(uint32_t code) {
    for (size_t i = 0; i < sizeof CHAR_NAMES / sizeof CHAR_NAMES[0]; ++i)
        if (CHAR_NAMES[i].code == code) return CHAR_NAMES[i].name;
    return "";
}

static uint32_t rd_u32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* ── container decode ──────────────────────────────────────────────────────────*/
uint8_t *sotes_sdt_decode(const uint8_t *file, size_t file_len,
                          sotes_sdt_header *hdr, size_t *body_len) {
    if (!file || file_len < 0x14) return NULL;
    uint32_t prefix = rd_u32(file + 0x00);
    uint32_t magic  = rd_u32(file + 0x04);
    uint32_t bodysz = rd_u32(file + 0x08);
    uint32_t val3   = rd_u32(file + 0x0c);
    uint32_t seed   = rd_u32(file + 0x10);
    if (prefix != 0x10 || magic != SOTES_SDT_MAGIC) return NULL;
    /* body starts at file+4+prefix (== 0x14); must fit. */
    size_t body_off = 4 + (size_t)prefix;
    if (bodysz == 0 || body_off + (size_t)bodysz > file_len) return NULL;

    invkey_init();
    uint8_t key = (uint8_t)((seed >> 8) & 0xff);
    uint8_t *body = (uint8_t *)malloc(bodysz);
    if (!body) return NULL;
    const uint8_t *src = file + body_off;
    for (uint32_t i = 0; i < bodysz; ++i)
        body[i] = g_invkey[(uint8_t)(src[i] - key)];

    if (hdr) {
        hdr->prefix_len = prefix; hdr->magic = magic; hdr->bodysize = bodysz;
        hdr->val3 = val3; hdr->seed = seed; hdr->key = key;
    }
    if (body_len) *body_len = bodysz;
    return body;
}

/* ── summary parse ─────────────────────────────────────────────────────────────
 * Decoded body = a record stream.  Record 0 is [u32 len=0x25c][604-byte metadata]:
 *   metadata+0x228 checksum, +0x22c magic (SOTES_SDT_MAGIC), +0x230 category handle.
 * The metadata block is a fixed 604 bytes, so the PARTY-HEADER GRID (16 u32) always
 * begins right after it (body 0x260).  The playable roster is found by scanning for
 * the known character codes (each appears exactly once); combat_level_max is the u32 right
 * after the code word (== the in-memory stat +0xe0, verified live). */
int sotes_save_parse(const uint8_t *body, size_t body_len, size_t file_size,
                     const sotes_sdt_header *hdr, sotes_save_info *out) {
    if (!out) return -1;
    memset(out, 0, sizeof *out);
    out->file_size = file_size;
    out->body_size = body_len;
    if (hdr) out->hdr = *hdr;
    if (!body || body_len < 4) return -1;

    uint32_t meta_len = rd_u32(body);              /* record-0 length (0x25c)      */
    size_t   meta_off = 4;
    if (meta_len < 0x234 || meta_off + meta_len > body_len) return -1;
    out->ok       = 1;
    out->checksum = rd_u32(body + meta_off + 0x228);
    uint32_t mg   = rd_u32(body + meta_off + 0x22c);
    out->handle   = rd_u32(body + meta_off + 0x230);
    out->valid    = (mg == SOTES_SDT_MAGIC && out->handle == SOTES_SAVE_MAINQUEST);

    /* party-header grid, right after the metadata record */
    size_t ph_off = meta_off + meta_len;
    if (ph_off + 16 * 4 <= body_len) {
        for (int k = 0; k < 16; ++k) out->ph[k] = rd_u32(body + ph_off + (size_t)k * 4);
        out->ph_present = 1;
    }

    /* roster: scan (post-metadata) for each known code; take the first hit whose
     * following combat_level_max is sane (1..99). */
    for (size_t ci = 0; ci < sizeof CHAR_NAMES / sizeof CHAR_NAMES[0]; ++ci) {
        uint32_t code = CHAR_NAMES[ci].code;
        for (size_t o = ph_off; o + 8 <= body_len; o += 4) {
            if (rd_u32(body + o) != code) continue;
            int32_t lv = (int32_t)rd_u32(body + o + 4);
            if (lv < 1 || lv > 99) continue;
            if (out->party_count >= SOTES_MAX_PARTY) break;
            sotes_member *m = &out->party[out->party_count++];
            m->code = code; m->combat_level_max = lv; m->body_off = o;
            snprintf(m->name, sizeof m->name, "%s", CHAR_NAMES[ci].name);
            break;
        }
    }
    return 0;
}

int sotes_save_read(const char *path, sotes_save_info *out) {
    if (!path || !out) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -2;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -3; }
    long sz = ftell(f);
    if (sz <= 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -3; }
    uint8_t *file = (uint8_t *)malloc((size_t)sz);
    if (!file) { fclose(f); return -4; }
    size_t got = fread(file, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(file); return -3; }

    sotes_sdt_header hdr; size_t body_len = 0;
    uint8_t *body = sotes_sdt_decode(file, (size_t)sz, &hdr, &body_len);
    free(file);
    if (!body) { memset(out, 0, sizeof *out); out->file_size = (size_t)sz; return -5; }
    int rc = sotes_save_parse(body, body_len, (size_t)sz, &hdr, out);
    free(body);
    return rc;
}
