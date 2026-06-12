/*
 * src/osr_format.h — the OpenSummoners ".osr" draw-stream record format.
 *
 * Trace Studio v2 (docs/plans/trace-studio-v2.md §Component 3) captures the
 * ORDERED DRAW-CALL STREAM of a frame — the 5 DDraw blits + GDI text + clears +
 * the present, plus the dedup'd SOURCE surfaces each blit reads from — instead of
 * snapshotting pixels.  A Windows-side replayer (the port binary's --osr-replay
 * mode, M4) re-executes the recorded ops to reconstruct each frame 1:1.  Both the
 * retail capture proxy (tools/capture_proxy) and the port emitter (src/osr_emit.c,
 * M5) write this SAME format; this header is the single shared codec so the two
 * sides — and the Python reader tools/trace_studio2/osr.py, which mirrors it — can
 * never drift.
 *
 * Design: pure C, header-only, NO Win32 / ddraw.h, so it links into the host unit
 * suite (test_osr_format.c) and the cross build alike.  Little-endian (x86 only,
 * both targets).  Append-only + self-describing: every record is framed
 * {u32 type, u32 len, payload[len]} so a reader can skip an unknown type by len,
 * and a run KILLED mid-capture (the harness hard-kills the game) is still valid up
 * to the last fully-written record.
 *
 * M3a implements the cheap records the boot already produces (FRAMEBEG / PRESENT /
 * ANCHOR / SEED); the bulky / draw records (CLEAR / BLIT / TEXT / SHEET / FONT /
 * PALETTE / INPUT) have reserved type ids here and land with the COM-wrap + blit
 * detours (M3b+).
 */
#ifndef OPENSUMMONERS_OSR_FORMAT_H
#define OPENSUMMONERS_OSR_FORMAT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ── file header (fixed 64 bytes, written once at file open) ───────────────── */
#define OSR_MAGIC0 'O'
#define OSR_MAGIC1 'S'
#define OSR_MAGIC2 'R'
#define OSR_MAGIC3 '1'
#define OSR_VERSION   1u
#define OSR_HEADER_SIZE 64u

/* side */
#define OSR_SIDE_PORT   0u
#define OSR_SIDE_RETAIL 1u

/* screen_pixfmt */
#define OSR_PIXFMT_UNKNOWN  0u
#define OSR_PIXFMT_RGB565   1u
#define OSR_PIXFMT_XRGB8888 2u
#define OSR_PIXFMT_PAL8     3u

/* flags bits */
#define OSR_FLAG_TURBO     0x1u
#define OSR_FLAG_LOCKSTEP  0x2u
#define OSR_FLAG_SEED_PIN  0x4u

/* ── record types (the framed stream after the header) ────────────────────── */
enum osr_rec_type {
    OSR_FRAMEBEG = 1,   /* opens a frame's op list */
    OSR_PRESENT  = 2,   /* Flip/Blt — closes the frame */
    OSR_ANCHOR   = 3,   /* scene/phase boundary assertion (name + flip/tick/rng) */
    OSR_SEED     = 4,   /* LCG seed pin / re-pin */
    /* ── M3b+ (reserved) ── */
    OSR_CLEAR    = 5,   /* memset/fill of a surface */
    OSR_BLIT     = 6,   /* one of the 5 blit primitives */
    OSR_TEXT     = 7,   /* GDI TextOut op */
    OSR_SHEET    = 8,   /* dedup'd decoded source pixels (once per unique dhash) */
    OSR_FONT     = 9,   /* dedup'd LOGFONT for the replayer's HFONT */
    OSR_PALETTE  = 10,  /* 256-entry palette for a PAL8 surface */
    OSR_INPUT    = 11   /* injected ring/held input at a flip */
};

/* ── little-endian put/get (x86; byte-wise so unaligned is fine) ──────────── */
static inline void osr_put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;        p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline void osr_put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static inline uint32_t osr_get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline uint16_t osr_get_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* ── header struct + codec ─────────────────────────────────────────────────
 * scenario is a nul-padded fixed field so the header stays a fixed size. */
typedef struct osr_header {
    uint8_t  side;          /* OSR_SIDE_* */
    uint8_t  pixfmt;        /* OSR_PIXFMT_* */
    uint16_t screen_w;
    uint16_t screen_h;
    uint32_t seed;
    uint32_t flags;         /* OSR_FLAG_* */
    char     scenario[40];  /* nul-padded */
} osr_header;

/* Encode the 64-byte header into buf (>= OSR_HEADER_SIZE).  Returns bytes
 * written, or 0 if cap is too small. */
static inline size_t osr_header_encode(uint8_t *buf, size_t cap,
                                       const osr_header *h)
{
    if (cap < OSR_HEADER_SIZE) return 0;
    memset(buf, 0, OSR_HEADER_SIZE);
    buf[0] = OSR_MAGIC0; buf[1] = OSR_MAGIC1;
    buf[2] = OSR_MAGIC2; buf[3] = OSR_MAGIC3;
    osr_put_u32(buf + 4, OSR_VERSION);
    buf[8] = h->side;
    buf[9] = h->pixfmt;
    osr_put_u16(buf + 10, h->screen_w);
    osr_put_u16(buf + 12, h->screen_h);
    /* buf+14: u16 reserved (0) */
    osr_put_u32(buf + 16, h->seed);
    osr_put_u32(buf + 20, h->flags);
    /* scenario: copy at most 39 chars, leave a nul terminator. */
    {
        size_t n = strlen(h->scenario);
        if (n > 39) n = 39;
        memcpy(buf + 24, h->scenario, n);
    }
    return OSR_HEADER_SIZE;
}

/* Decode + validate the 64-byte header.  Returns 1 on success (magic+version
 * match), 0 otherwise. */
static inline int osr_header_decode(const uint8_t *buf, size_t len,
                                    osr_header *out)
{
    if (len < OSR_HEADER_SIZE) return 0;
    if (buf[0] != OSR_MAGIC0 || buf[1] != OSR_MAGIC1 ||
        buf[2] != OSR_MAGIC2 || buf[3] != OSR_MAGIC3) return 0;
    if (osr_get_u32(buf + 4) != OSR_VERSION) return 0;
    out->side     = buf[8];
    out->pixfmt   = buf[9];
    out->screen_w = osr_get_u16(buf + 10);
    out->screen_h = osr_get_u16(buf + 12);
    out->seed     = osr_get_u32(buf + 16);
    out->flags    = osr_get_u32(buf + 20);
    memset(out->scenario, 0, sizeof(out->scenario));
    memcpy(out->scenario, buf + 24, 39);
    out->scenario[39] = '\0';
    return 1;
}

/* ── generic framed-record helpers ──────────────────────────────────────────
 * A record is [u32 type][u32 len][payload].  osr_rec_begin writes the 8-byte
 * frame given an already-known payload length; the caller then appends the
 * payload.  For convenience the typed encoders below do both. */

/* Write a full framed record (header + payload) into buf.  Returns the total
 * bytes written (8 + payload_len) or 0 if cap is too small. */
static inline size_t osr_rec_write(uint8_t *buf, size_t cap, uint32_t type,
                                   const uint8_t *payload, uint32_t payload_len)
{
    size_t total = (size_t)8 + payload_len;
    if (cap < total) return 0;
    osr_put_u32(buf, type);
    osr_put_u32(buf + 4, payload_len);
    if (payload_len) memcpy(buf + 8, payload, payload_len);
    return total;
}

/* Iterate records.  Given a cursor p and the buffer end, decode one record:
 * sets the out type/payload/plen and returns the cursor PAST it, or NULL if
 * there is not a complete record left (truncated tail from a killed capture). */
static inline const uint8_t *osr_rec_next(const uint8_t *p, const uint8_t *end,
                                          uint32_t *type, const uint8_t **payload,
                                          uint32_t *plen)
{
    if (end - p < 8) return NULL;
    uint32_t t = osr_get_u32(p);
    uint32_t l = osr_get_u32(p + 4);
    if ((size_t)(end - (p + 8)) < l) return NULL;   /* truncated payload */
    *type = t;
    *plen = l;
    *payload = p + 8;
    return p + 8 + l;
}

/* ── typed payload structs ─────────────────────────────────────────────────*/
typedef struct osr_framebeg { uint32_t flip, sim_tick, anchor_id; } osr_framebeg;
typedef struct osr_present   { uint32_t mode, src_handle;          } osr_present;
typedef struct osr_seed      { uint32_t flip, before, value;       } osr_seed;
typedef struct osr_anchor {
    uint32_t flip, sim_tick, rng;
    const char *name;        /* not nul-required; len carried separately */
    uint32_t name_len;
} osr_anchor;

/* ── typed encoders (write a complete framed record) ──────────────────────── */
static inline size_t osr_enc_framebeg(uint8_t *buf, size_t cap,
                                      uint32_t flip, uint32_t sim_tick,
                                      uint32_t anchor_id)
{
    uint8_t p[12];
    osr_put_u32(p, flip);
    osr_put_u32(p + 4, sim_tick);
    osr_put_u32(p + 8, anchor_id);
    return osr_rec_write(buf, cap, OSR_FRAMEBEG, p, sizeof(p));
}
static inline size_t osr_enc_present(uint8_t *buf, size_t cap,
                                     uint32_t mode, uint32_t src_handle)
{
    uint8_t p[8];
    osr_put_u32(p, mode);
    osr_put_u32(p + 4, src_handle);
    return osr_rec_write(buf, cap, OSR_PRESENT, p, sizeof(p));
}
static inline size_t osr_enc_seed(uint8_t *buf, size_t cap,
                                  uint32_t flip, uint32_t before, uint32_t value)
{
    uint8_t p[12];
    osr_put_u32(p, flip);
    osr_put_u32(p + 4, before);
    osr_put_u32(p + 8, value);
    return osr_rec_write(buf, cap, OSR_SEED, p, sizeof(p));
}
/* ANCHOR payload: u32 flip, sim_tick, rng, name_len, char name[name_len]. */
static inline size_t osr_enc_anchor(uint8_t *buf, size_t cap,
                                    uint32_t flip, uint32_t sim_tick, uint32_t rng,
                                    const char *name)
{
    uint32_t nl = name ? (uint32_t)strlen(name) : 0u;
    size_t total = (size_t)8 + 16 + nl;
    if (cap < total) return 0;
    osr_put_u32(buf, OSR_ANCHOR);
    osr_put_u32(buf + 4, (uint32_t)(16 + nl));
    osr_put_u32(buf + 8, flip);
    osr_put_u32(buf + 12, sim_tick);
    osr_put_u32(buf + 16, rng);
    osr_put_u32(buf + 20, nl);
    if (nl) memcpy(buf + 24, name, nl);
    return total;
}

/* ── typed payload decoders (operate on the payload slice from osr_rec_next) ─*/
static inline int osr_dec_framebeg(const uint8_t *p, uint32_t len, osr_framebeg *o)
{
    if (len < 12) return 0;
    o->flip = osr_get_u32(p); o->sim_tick = osr_get_u32(p + 4);
    o->anchor_id = osr_get_u32(p + 8);
    return 1;
}
static inline int osr_dec_present(const uint8_t *p, uint32_t len, osr_present *o)
{
    if (len < 8) return 0;
    o->mode = osr_get_u32(p); o->src_handle = osr_get_u32(p + 4);
    return 1;
}
static inline int osr_dec_seed(const uint8_t *p, uint32_t len, osr_seed *o)
{
    if (len < 12) return 0;
    o->flip = osr_get_u32(p); o->before = osr_get_u32(p + 4);
    o->value = osr_get_u32(p + 8);
    return 1;
}
static inline int osr_dec_anchor(const uint8_t *p, uint32_t len, osr_anchor *o)
{
    if (len < 16) return 0;
    o->flip = osr_get_u32(p); o->sim_tick = osr_get_u32(p + 4);
    o->rng = osr_get_u32(p + 8);
    o->name_len = osr_get_u32(p + 12);
    if ((size_t)o->name_len > (size_t)(len - 16)) return 0;
    o->name = (const char *)(p + 16);
    return 1;
}

#endif /* OPENSUMMONERS_OSR_FORMAT_H */
