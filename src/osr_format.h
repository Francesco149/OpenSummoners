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
 * ANCHOR / SEED); M3b adds BLIT (the 5 source-bearing draw primitives, with the
 * render_id identity but dhash/dst_handle deferred to M3c); M3c adds SHEET (the
 * dedup'd source pixels); M3d adds TEXT + FONT (the GDI TextOut stream).  The
 * remaining records (CLEAR / PALETTE / INPUT) have reserved type ids here and land
 * as the reconstructor needs them.
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
    OSR_INPUT    = 11,  /* injected ring/held input at a flip */
    OSR_BLEND    = 12,  /* dedup'd software-alpha blend descriptor (M4 alpha) */
    OSR_SNAP     = 13   /* REAL backbuffer pixels at a flip (the M4d validate gate) */
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
/* CLEAR — the engine zero-fills its compose surface (FUN_005b9410, scene
 * transitions: newgame menu / prologue / title resets).  An ORDERED draw in the
 * frame's op list (seq shared with BLIT/TEXT); the proxy emits it only for the
 * backbuffer (offscreen panel-sheet clears are filtered out capture-side).
 * value is the fill word (retail always 0). */
typedef struct osr_clear     { uint32_t seq, dst_handle, value;    } osr_clear;
typedef struct osr_anchor {
    uint32_t flip, sim_tick, rng;
    const char *name;        /* not nul-required; len carried separately */
    uint32_t name_len;
} osr_anchor;

/* BLIT — one of the 5 source-bearing draw primitives (M3b).  The field set is
 * tools/render_diff.py's schema so the cross-side draw diff works unchanged:
 *   va        — which primitive (0x5b9a40/b70/ae0/bf0 thiscall, 0x5bd550 cdecl)
 *   seq       — the draw's ordinal WITHIN its frame (reset at each FRAMEBEG)
 *   res/frame — the source cel's load-stable identity (render_id registry)
 *   dhash     — decoded-sheet fingerprint (0 retail-side until M3c grabs pixels)
 *   dst_handle— the dest surface's stable handle (0 until the M3c COM wrap)
 *   dx,dy,reqw,reqh,sx,sy — the RAW call geometry (clip math is a deterministic
 *               bit-exact port from these, so we emit inputs not post-clip rects)
 *   ow,oh,ox,oy — the source cel placement metrics (+0xb8/+0xbc/+0x0c/+0x10)
 *   state     — cel +0xd4 (0x8000 = KEYSRC armed); ckey — bound color key;
 *   bmode     — alpha blend mode (-1 if N/A); mode — the 0..4 primitive index;
 *   blend_ref — for mode-4 alpha, the OSR_BLEND record this blit blends through
 *               (0 = none); the per-channel blend LUT can't be reconstructed from
 *               bmode alone (mode-4 reconstruction needs it).  0 for modes 0..3.
 *   srcw,srch — mode-2 RECTS only: the SOURCE rect extent (the 8-coord call's
 *               args 7/8; src_w != dst_w means a SCALING Blt).  0 = N/A (modes
 *               0/1/3/4 carry no independent source extent).  Appended after
 *               blend_ref; the decoder zero-fills them on a legacy 80-byte
 *               payload so pre-existing captures stay readable. */
typedef struct osr_blit {
    uint32_t va, seq;
    uint16_t res, frame;
    uint32_t dhash, dst_handle;
    int32_t  dx, dy, reqw, reqh, sx, sy;
    int32_t  ow, oh, ox, oy;
    uint32_t state, ckey;
    int32_t  bmode;
    uint32_t mode;
    uint32_t blend_ref;
    int32_t  srcw, srch;
} osr_blit;

#define OSR_BLIT_PAYLOAD        88u  /* the fixed BLIT payload size (osr_enc_blit) */
#define OSR_BLIT_PAYLOAD_LEGACY 80u  /* pre-srcw/srch captures (decoder zero-fills) */

/* SHEET — the dedup'd decoded SOURCE pixels a blit reads from (M3c).  Written
 * ONCE per unique source surface (dedup'd by dhash); BLIT.dhash references it.
 * The replayer (M4) rebuilds a source surface from each SHEET, then replays the
 * frame's BLITs reading sub-rects (sx,sy,reqw,reqh) out of it.
 *   dhash      — FNV-1a of (w,h,pixfmt-seed) + the raw pixel bytes; the sheet_ref
 *   res/frame  — a representative (resource_id, frame) that first referenced it
 *   w,h        — surface dimensions in pixels
 *   pitch      — bytes per row as captured (Lock pitch; may exceed w*bytespp)
 *   pixfmt     — OSR_PIXFMT_* of the captured bytes
 *   codec      — 0 = raw, 1 = miniz/zlib (deferred; raw first per port-debt)
 *   bytes      — pitch*h bytes of the (possibly compressed) surface contents */
typedef struct osr_sheet {
    uint32_t dhash;
    uint16_t res, frame;
    uint16_t w, h;
    uint32_t pitch;
    uint8_t  pixfmt, codec;
    uint32_t byte_len;
    const uint8_t *bytes;   /* not owned; len = byte_len */
} osr_sheet;

#define OSR_SHEET_HDR 24u    /* the fixed-size SHEET payload prefix before bytes */

/* SNAP — the REAL backbuffer pixels at a flip (M4d, the --validate gate).  The
 * capture proxy Locks the engine's dest surface at the flip hook (after the
 * frame's draws, before the present) and records what retail ACTUALLY put on
 * screen; the reconstructor compares its just-composed frame against it and the
 * difference must be differ_px==0 (docs/plans/trace-studio-v2.md M4d).  Sampled
 * (every-N-flips / an explicit flip list), so a capture carries only a handful.
 *   flip      — the label of the frame this snapshot closes (the FRAMEBEG flip;
 *               positioned in-stream just before that frame's PRESENT)
 *   sim_tick  — the sim tick at snapshot time (metadata; join is by position)
 *   w,h,pitch — surface dims + Lock pitch (pitch may exceed w*bytespp)
 *   pixfmt    — OSR_PIXFMT_* of the captured bytes
 *   codec     — 0 = raw
 *   bytes     — pitch*h bytes of surface contents (rows top-down, Lock order) */
typedef struct osr_snap {
    uint32_t flip, sim_tick;
    uint16_t w, h;
    uint32_t pitch;
    uint8_t  pixfmt, codec;
    uint32_t byte_len;
    const uint8_t *bytes;   /* not owned; len = byte_len */
} osr_snap;

#define OSR_SNAP_HDR 24u     /* the fixed-size SNAP payload prefix before bytes */

/* BLEND — a dedup'd software-alpha blend descriptor (M4 alpha).  The engine's
 * mode-4 blit (zdd_blit_orchestrate → zdd_alpha_blit) blends through a
 * zdd_blend_desc: a `mode` (0 remap / 1 src×dst / 2 colorize) + 3 channel
 * records, each carrying a `shift`, a `mask`, and a byte LUT the blend math
 * indexes.  Only `mode` is recoverable from the BLIT record; the LUTs are not —
 * so the reconstructor needs this.  Dedup'd by descriptor identity (the engine's
 * ramp descriptors are persistent globals) and referenced by osr_blit.blend_ref.
 *   blend_ref   — the stable 1-based id (0 = none); osr_blit.blend_ref points here
 *   mode        — desc->mode (0/1/2; the blit's bmode mirrors it)
 *   shift[3]    — per-channel right/left shift (R/G/B; 11/5/0 for RGB565)
 *   mask[3]     — per-channel channel mask (0xF800/0x07E0/0x001F for RGB565)
 *   lut_len[3]  — per-channel LUT byte length (sized to the max index the blend
 *                 math reaches: mode0 = (mask>>shift)+1; mode1 = ((m>>s)<<5)+
 *                 (m>>s)+1; mode2 = Σ(m>>s)/3 + 1 — see zdd_blend_pixel)
 *   lut         — lut_len[0]+lut_len[1]+lut_len[2] bytes, the 3 LUTs concatenated */
typedef struct osr_blend {
    uint32_t blend_ref;
    int32_t  mode;
    int32_t  shift[3];
    uint32_t mask[3];
    uint32_t lut_len[3];
    const uint8_t *lut;     /* not owned; len = lut_len[0]+lut_len[1]+lut_len[2] */
} osr_blend;

#define OSR_BLEND_HDR 44u    /* fixed prefix: 4 + 4 + 3*4 + 3*4 + 3*4 = 44 */

/* FONT — a dedup'd GDI logical font (M3d).  The retail engine builds 8 HFONTs at
 * boot (ar_register_fonts → CreateFontIndirectA) and the replayer (M4) recreates
 * them from these LOGFONTA fields; a TEXT record names its font by font_ref.
 * Mirrors the Win32 LOGFONTA field-for-field but stays Win32-free (plain int/byte)
 * so this header still links into the host suite.  font_ref is a stable 1-based id
 * (0 = none/unknown).  Fixed 64-byte payload. */
typedef struct osr_font {
    uint32_t font_ref;
    int32_t  height, width, escapement, orientation, weight;
    uint8_t  italic, underline, strikeout, charset;
    uint8_t  out_prec, clip_prec, quality, pitch_family;
    char     face[32];      /* LF_FACESIZE; nul-padded face name */
} osr_font;

#define OSR_FONT_PAYLOAD 64u   /* 4 + 5*4 + 8 + 32 */

/* TEXT — one GDI TextOutA op (M3d).  The engine renders all dynamic text + the
 * prologue narration through GDI TextOutA straight onto the backbuffer DC
 * (text-glyph-pipeline.md); the replayer re-issues these on Windows so glyphs are
 * bit-exact.  Variable length (the string trails a fixed 32-byte prefix).
 *   seq        — the draw's ordinal WITHIN its frame, SHARED with BLIT.seq so the
 *                replayer interleaves text and blits in issue order
 *   dst_handle — the target surface's stable handle (the single backbuffer, M3c)
 *   x,y        — TextOutA position
 *   font_ref   — the currently-selected HFONT's font_ref (→ a FONT record)
 *   color      — the current text COLORREF (SetTextColor)
 *   bk_mode    — the current background mode (TRANSPARENT=1 / OPAQUE=2)
 *   str_len    — byte length of the string (TextOutA's explicit count, not NUL)
 *   str        — str_len bytes (SJIS/ASCII as the engine passed them) */
typedef struct osr_text {
    uint32_t seq, dst_handle;
    int32_t  x, y;
    uint32_t font_ref, color;
    int32_t  bk_mode;
    uint32_t str_len;
    const char *str;        /* not owned; len = str_len */
} osr_text;

#define OSR_TEXT_HDR 32u     /* the fixed-size TEXT payload prefix before str */

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
static inline size_t osr_enc_clear(uint8_t *buf, size_t cap,
                                   uint32_t seq, uint32_t dst_handle,
                                   uint32_t value)
{
    uint8_t p[12];
    osr_put_u32(p, seq);
    osr_put_u32(p + 4, dst_handle);
    osr_put_u32(p + 8, value);
    return osr_rec_write(buf, cap, OSR_CLEAR, p, sizeof(p));
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

/* BLIT — a fixed 88-byte payload (OSR_BLIT_PAYLOAD).  Mirrors struct osr_blit
 * field order so the Python reader (tools/trace_studio2/osr.py) can struct.unpack
 * it directly. */
static inline size_t osr_enc_blit(uint8_t *buf, size_t cap, const osr_blit *b)
{
    if (cap < (size_t)8 + OSR_BLIT_PAYLOAD) return 0;
    osr_put_u32(buf, OSR_BLIT);
    osr_put_u32(buf + 4, OSR_BLIT_PAYLOAD);
    uint8_t *p = buf + 8;
    osr_put_u32(p +  0, b->va);
    osr_put_u32(p +  4, b->seq);
    osr_put_u16(p +  8, b->res);
    osr_put_u16(p + 10, b->frame);
    osr_put_u32(p + 12, b->dhash);
    osr_put_u32(p + 16, b->dst_handle);
    osr_put_u32(p + 20, (uint32_t)b->dx);
    osr_put_u32(p + 24, (uint32_t)b->dy);
    osr_put_u32(p + 28, (uint32_t)b->reqw);
    osr_put_u32(p + 32, (uint32_t)b->reqh);
    osr_put_u32(p + 36, (uint32_t)b->sx);
    osr_put_u32(p + 40, (uint32_t)b->sy);
    osr_put_u32(p + 44, (uint32_t)b->ow);
    osr_put_u32(p + 48, (uint32_t)b->oh);
    osr_put_u32(p + 52, (uint32_t)b->ox);
    osr_put_u32(p + 56, (uint32_t)b->oy);
    osr_put_u32(p + 60, b->state);
    osr_put_u32(p + 64, b->ckey);
    osr_put_u32(p + 68, (uint32_t)b->bmode);
    osr_put_u32(p + 72, b->mode);
    osr_put_u32(p + 76, b->blend_ref);
    osr_put_u32(p + 80, (uint32_t)b->srcw);
    osr_put_u32(p + 84, (uint32_t)b->srch);
    return (size_t)8 + OSR_BLIT_PAYLOAD;
}

/* SHEET — write only the framed 8-byte header + the 24-byte prefix (NOT the
 * pixel bytes), with the len field accounting for the full byte_len.  Lets a
 * writer stream large pixel payloads straight into its ring after the prefix
 * (one copy, no temp buffer).  Returns 8 + OSR_SHEET_HDR, or 0 if cap is short. */
static inline size_t osr_enc_sheet_prefix(uint8_t *buf, size_t cap,
                                          const osr_sheet *s)
{
    if (cap < (size_t)8 + OSR_SHEET_HDR) return 0;
    osr_put_u32(buf, OSR_SHEET);
    osr_put_u32(buf + 4, OSR_SHEET_HDR + s->byte_len);
    uint8_t *p = buf + 8;
    osr_put_u32(p +  0, s->dhash);
    osr_put_u16(p +  4, s->res);
    osr_put_u16(p +  6, s->frame);
    osr_put_u16(p +  8, s->w);
    osr_put_u16(p + 10, s->h);
    osr_put_u32(p + 12, s->pitch);
    p[16] = s->pixfmt;
    p[17] = s->codec;
    p[18] = 0; p[19] = 0;            /* reserved */
    osr_put_u32(p + 20, s->byte_len);
    return (size_t)8 + OSR_SHEET_HDR;
}

/* SHEET — a variable-length record: the 24-byte prefix (OSR_SHEET_HDR) followed
 * by byte_len bytes of (possibly compressed) pixels.  Returns total bytes
 * written, or 0 if cap is too small. */
static inline size_t osr_enc_sheet(uint8_t *buf, size_t cap, const osr_sheet *s)
{
    size_t total = (size_t)8 + OSR_SHEET_HDR + s->byte_len;
    if (cap < total) return 0;
    osr_enc_sheet_prefix(buf, cap, s);
    if (s->byte_len && s->bytes)
        memcpy(buf + 8 + OSR_SHEET_HDR, s->bytes, s->byte_len);
    return total;
}

/* SNAP — write only the framed 8-byte header + the 24-byte prefix (NOT the
 * pixel bytes), with the len field accounting for the full byte_len; the writer
 * streams the Lock'd pixels straight into its ring after the prefix (the same
 * one-copy discipline as SHEET).  Returns 8 + OSR_SNAP_HDR, or 0 if cap is short. */
static inline size_t osr_enc_snap_prefix(uint8_t *buf, size_t cap,
                                         const osr_snap *s)
{
    if (cap < (size_t)8 + OSR_SNAP_HDR) return 0;
    osr_put_u32(buf, OSR_SNAP);
    osr_put_u32(buf + 4, OSR_SNAP_HDR + s->byte_len);
    uint8_t *p = buf + 8;
    osr_put_u32(p +  0, s->flip);
    osr_put_u32(p +  4, s->sim_tick);
    osr_put_u16(p +  8, s->w);
    osr_put_u16(p + 10, s->h);
    osr_put_u32(p + 12, s->pitch);
    p[16] = s->pixfmt;
    p[17] = s->codec;
    p[18] = 0; p[19] = 0;            /* reserved */
    osr_put_u32(p + 20, s->byte_len);
    return (size_t)8 + OSR_SNAP_HDR;
}

/* SNAP — a variable-length record: the 24-byte prefix (OSR_SNAP_HDR) followed
 * by byte_len bytes of raw pixels.  Returns total bytes written, or 0 if cap
 * is too small. */
static inline size_t osr_enc_snap(uint8_t *buf, size_t cap, const osr_snap *s)
{
    size_t total = (size_t)8 + OSR_SNAP_HDR + s->byte_len;
    if (cap < total) return 0;
    osr_enc_snap_prefix(buf, cap, s);
    if (s->byte_len && s->bytes)
        memcpy(buf + 8 + OSR_SNAP_HDR, s->bytes, s->byte_len);
    return total;
}

/* BLEND — a variable-length record: the 44-byte prefix (OSR_BLEND_HDR) followed
 * by the 3 concatenated channel LUTs (lut_len[0..2] bytes).  Returns total bytes
 * written, or 0 if cap is too small. */
static inline size_t osr_enc_blend(uint8_t *buf, size_t cap, const osr_blend *b)
{
    uint32_t lut_total = b->lut_len[0] + b->lut_len[1] + b->lut_len[2];
    size_t total = (size_t)8 + OSR_BLEND_HDR + lut_total;
    if (cap < total) return 0;
    osr_put_u32(buf, OSR_BLEND);
    osr_put_u32(buf + 4, OSR_BLEND_HDR + lut_total);
    uint8_t *p = buf + 8;
    osr_put_u32(p +  0, b->blend_ref);
    osr_put_u32(p +  4, (uint32_t)b->mode);
    for (int i = 0; i < 3; i++) osr_put_u32(p +  8 + i * 4, (uint32_t)b->shift[i]);
    for (int i = 0; i < 3; i++) osr_put_u32(p + 20 + i * 4, b->mask[i]);
    for (int i = 0; i < 3; i++) osr_put_u32(p + 32 + i * 4, b->lut_len[i]);
    if (lut_total && b->lut) memcpy(p + OSR_BLEND_HDR, b->lut, lut_total);
    return total;
}

/* FONT — a fixed 64-byte payload mirroring struct osr_font's field order so the
 * Python reader (tools/trace_studio2/osr.py) can struct.unpack it directly. */
static inline size_t osr_enc_font(uint8_t *buf, size_t cap, const osr_font *f)
{
    if (cap < (size_t)8 + OSR_FONT_PAYLOAD) return 0;
    osr_put_u32(buf, OSR_FONT);
    osr_put_u32(buf + 4, OSR_FONT_PAYLOAD);
    uint8_t *p = buf + 8;
    osr_put_u32(p +  0, f->font_ref);
    osr_put_u32(p +  4, (uint32_t)f->height);
    osr_put_u32(p +  8, (uint32_t)f->width);
    osr_put_u32(p + 12, (uint32_t)f->escapement);
    osr_put_u32(p + 16, (uint32_t)f->orientation);
    osr_put_u32(p + 20, (uint32_t)f->weight);
    p[24] = f->italic;   p[25] = f->underline;  p[26] = f->strikeout;  p[27] = f->charset;
    p[28] = f->out_prec; p[29] = f->clip_prec;  p[30] = f->quality;    p[31] = f->pitch_family;
    memcpy(p + 32, f->face, 32);
    return (size_t)8 + OSR_FONT_PAYLOAD;
}

/* TEXT — a variable-length record: the 32-byte prefix (OSR_TEXT_HDR) followed by
 * str_len string bytes.  Returns total bytes written, or 0 if cap is too small. */
static inline size_t osr_enc_text(uint8_t *buf, size_t cap, const osr_text *t)
{
    size_t total = (size_t)8 + OSR_TEXT_HDR + t->str_len;
    if (cap < total) return 0;
    osr_put_u32(buf, OSR_TEXT);
    osr_put_u32(buf + 4, OSR_TEXT_HDR + t->str_len);
    uint8_t *p = buf + 8;
    osr_put_u32(p +  0, t->seq);
    osr_put_u32(p +  4, t->dst_handle);
    osr_put_u32(p +  8, (uint32_t)t->x);
    osr_put_u32(p + 12, (uint32_t)t->y);
    osr_put_u32(p + 16, t->font_ref);
    osr_put_u32(p + 20, t->color);
    osr_put_u32(p + 24, (uint32_t)t->bk_mode);
    osr_put_u32(p + 28, t->str_len);
    if (t->str_len && t->str) memcpy(p + OSR_TEXT_HDR, t->str, t->str_len);
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
static inline int osr_dec_clear(const uint8_t *p, uint32_t len, osr_clear *o)
{
    if (len < 12) return 0;
    o->seq = osr_get_u32(p); o->dst_handle = osr_get_u32(p + 4);
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
static inline int osr_dec_blit(const uint8_t *p, uint32_t len, osr_blit *o)
{
    if (len < OSR_BLIT_PAYLOAD_LEGACY) return 0;
    o->va         = osr_get_u32(p +  0);
    o->seq        = osr_get_u32(p +  4);
    o->res        = osr_get_u16(p +  8);
    o->frame      = osr_get_u16(p + 10);
    o->dhash      = osr_get_u32(p + 12);
    o->dst_handle = osr_get_u32(p + 16);
    o->dx   = (int32_t)osr_get_u32(p + 20);
    o->dy   = (int32_t)osr_get_u32(p + 24);
    o->reqw = (int32_t)osr_get_u32(p + 28);
    o->reqh = (int32_t)osr_get_u32(p + 32);
    o->sx   = (int32_t)osr_get_u32(p + 36);
    o->sy   = (int32_t)osr_get_u32(p + 40);
    o->ow   = (int32_t)osr_get_u32(p + 44);
    o->oh   = (int32_t)osr_get_u32(p + 48);
    o->ox   = (int32_t)osr_get_u32(p + 52);
    o->oy   = (int32_t)osr_get_u32(p + 56);
    o->state = osr_get_u32(p + 60);
    o->ckey  = osr_get_u32(p + 64);
    o->bmode = (int32_t)osr_get_u32(p + 68);
    o->mode  = osr_get_u32(p + 72);
    o->blend_ref = osr_get_u32(p + 76);
    if (len >= OSR_BLIT_PAYLOAD) {
        o->srcw = (int32_t)osr_get_u32(p + 80);
        o->srch = (int32_t)osr_get_u32(p + 84);
    } else {
        o->srcw = 0;            /* legacy capture: source extent not recorded */
        o->srch = 0;
    }
    return 1;
}
static inline int osr_dec_sheet(const uint8_t *p, uint32_t len, osr_sheet *o)
{
    if (len < OSR_SHEET_HDR) return 0;
    o->dhash    = osr_get_u32(p +  0);
    o->res      = osr_get_u16(p +  4);
    o->frame    = osr_get_u16(p +  6);
    o->w        = osr_get_u16(p +  8);
    o->h        = osr_get_u16(p + 10);
    o->pitch    = osr_get_u32(p + 12);
    o->pixfmt   = p[16];
    o->codec    = p[17];
    o->byte_len = osr_get_u32(p + 20);
    if ((size_t)o->byte_len > (size_t)(len - OSR_SHEET_HDR)) return 0;
    o->bytes    = p + OSR_SHEET_HDR;
    return 1;
}
static inline int osr_dec_snap(const uint8_t *p, uint32_t len, osr_snap *o)
{
    if (len < OSR_SNAP_HDR) return 0;
    o->flip     = osr_get_u32(p +  0);
    o->sim_tick = osr_get_u32(p +  4);
    o->w        = osr_get_u16(p +  8);
    o->h        = osr_get_u16(p + 10);
    o->pitch    = osr_get_u32(p + 12);
    o->pixfmt   = p[16];
    o->codec    = p[17];
    o->byte_len = osr_get_u32(p + 20);
    if ((size_t)o->byte_len > (size_t)(len - OSR_SNAP_HDR)) return 0;
    o->bytes    = p + OSR_SNAP_HDR;
    return 1;
}
static inline int osr_dec_blend(const uint8_t *p, uint32_t len, osr_blend *o)
{
    if (len < OSR_BLEND_HDR) return 0;
    o->blend_ref = osr_get_u32(p +  0);
    o->mode      = (int32_t)osr_get_u32(p + 4);
    for (int i = 0; i < 3; i++) o->shift[i]   = (int32_t)osr_get_u32(p +  8 + i * 4);
    for (int i = 0; i < 3; i++) o->mask[i]    = osr_get_u32(p + 20 + i * 4);
    for (int i = 0; i < 3; i++) o->lut_len[i] = osr_get_u32(p + 32 + i * 4);
    uint32_t lut_total = o->lut_len[0] + o->lut_len[1] + o->lut_len[2];
    if ((size_t)lut_total > (size_t)(len - OSR_BLEND_HDR)) return 0;
    o->lut = p + OSR_BLEND_HDR;
    return 1;
}
static inline int osr_dec_font(const uint8_t *p, uint32_t len, osr_font *o)
{
    if (len < OSR_FONT_PAYLOAD) return 0;
    o->font_ref    = osr_get_u32(p +  0);
    o->height      = (int32_t)osr_get_u32(p +  4);
    o->width       = (int32_t)osr_get_u32(p +  8);
    o->escapement  = (int32_t)osr_get_u32(p + 12);
    o->orientation = (int32_t)osr_get_u32(p + 16);
    o->weight      = (int32_t)osr_get_u32(p + 20);
    o->italic = p[24]; o->underline = p[25]; o->strikeout = p[26]; o->charset = p[27];
    o->out_prec = p[28]; o->clip_prec = p[29]; o->quality = p[30]; o->pitch_family = p[31];
    memcpy(o->face, p + 32, 32);
    return 1;
}
static inline int osr_dec_text(const uint8_t *p, uint32_t len, osr_text *o)
{
    if (len < OSR_TEXT_HDR) return 0;
    o->seq        = osr_get_u32(p +  0);
    o->dst_handle = osr_get_u32(p +  4);
    o->x          = (int32_t)osr_get_u32(p +  8);
    o->y          = (int32_t)osr_get_u32(p + 12);
    o->font_ref   = osr_get_u32(p + 16);
    o->color      = osr_get_u32(p + 20);
    o->bk_mode    = (int32_t)osr_get_u32(p + 24);
    o->str_len    = osr_get_u32(p + 28);
    if ((size_t)o->str_len > (size_t)(len - OSR_TEXT_HDR)) return 0;
    o->str        = (const char *)(p + OSR_TEXT_HDR);
    return 1;
}

#endif /* OPENSUMMONERS_OSR_FORMAT_H */
