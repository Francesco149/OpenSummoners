/* blend_grab.h — the trace-studio-v2 capture-proxy ALPHA blend-descriptor grab
 * (M4 alpha).
 *
 * The engine's mode-4 blit (FUN_005bd550 → FUN_005bd680, src/zdd.c
 * zdd_blit_orchestrate → zdd_alpha_blit) blends its source through a
 * __thiscall blend descriptor (zdd_blend_desc, src/zdd.h): a `mode` (0 remap /
 * 1 src×dst / 2 colorize) at +0x00 and three 0x14-byte channel records starting
 * at +0x04, each carrying a `shift` (+0x00), a `mask` (+0x04), and a byte LUT
 * pointer (+0x08) the blend math indexes.  The BLIT record carries only `mode`;
 * the LUTs are what the reconstructor (osr_recon.c) needs — without them mode-4
 * blits (the prologue narration, the town sky/ground, fades) can't be replayed.
 *
 * So the first time we see a blend descriptor (interned by pointer — the engine's
 * ramp descriptors are persistent globals, like the surfaces) we read its mode +
 * 3 channels and grab exactly the LUT bytes the blend math will index (sized per
 * mode, see bg_lut_len), and emit ONE dedup'd OSR_BLEND record.  The alpha BLIT
 * then references it by blend_ref.  Pure pointer interning + a bounded read, no
 * COM, on the engine thread inside the blit detour.
 */
#ifndef OSS_BLEND_GRAB_H
#define OSS_BLEND_GRAB_H

#include <stdint.h>
#include <windows.h>

#include "proxy_log.h"
#include "osr_writer.h"

/* zdd_blend_desc layout (src/zdd.h): mode @+0x00; channel i @ +0x04 + i*0x14,
 * each { shift @+0x00 (i32), mask @+0x04 (u32), lut @+0x08 (ptr) }. */
#define BG_CH_STRIDE  0x14u
#define BG_CH_BASE    0x04u

/* The max LUT byte length the blend math reaches for one channel, given the mode
 * and the channel's max value maxch = mask>>shift (and, for mode 2, the gray sum
 * of all 3 channels).  Mirrors zdd_blend_pixel / zdd_blend_gray_index exactly:
 *   mode 0: idx = s_ch          → maxch + 1
 *   mode 1: idx = (s_ch<<5)+d_ch → (maxch<<5) + maxch + 1
 *   mode 2: idx = (Σ s_ch)/3     → gray_max + 1   (same for every channel) */
static uint32_t bg_lut_len(int mode, uint32_t maxch, uint32_t gray_max)
{
    if (mode == 1) return (maxch << 5) + maxch + 1u;
    if (mode == 2) return gray_max + 1u;
    return maxch + 1u;                 /* mode 0 (and any other) */
}

#define BG_LUT_CAP  8192u              /* per-channel sanity cap on the read */

/* Is [p, p+n) committed + readable?  The blend descriptor can be a STACK object
 * (built per call — so its address is NOT a stable global), while its channel
 * LUTs are static data; VirtualQuery covers both and guards against a wild deref
 * inside the trampoline callback (where a fault would crash). */
static int bg_readable(const void *p, size_t n)
{
    if ((uintptr_t)p < 0x10000u) return 0;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof mbi) == 0) return 0;
    if (mbi.State != MEM_COMMIT) return 0;
    DWORD ok = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ |
               PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY;
    if (!(mbi.Protect & ok)) return 0;
    /* the readable span runs to the end of this region */
    uintptr_t end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    return (uintptr_t)p + n <= end;
}

/* ── content → blend_ref intern (dedup by descriptor CONTENT) ─────────────────
 * The descriptor may be a transient stack object, so we cannot dedup by pointer
 * — we dedup by a content key (mode + per-channel shift/mask/lut-POINTER; the LUT
 * pointers are stable globals that uniquely identify the blend table). */
#define BG_SLOTS 256u

typedef struct { int used; uint32_t key; uint32_t ref; } bg_entry;
static bg_entry  g_bg[BG_SLOTS];
static uint32_t  g_bg_next = 1;        /* ref 0 = "no blend" */

#define BG_FNV1A_SEED  2166136261u
static uint32_t bg_mix(uint32_t h, uint32_t v)
{
    const uint8_t *p = (const uint8_t *)&v;
    for (int i = 0; i < 4; i++) { h ^= p[i]; h *= 16777619u; }
    return h;
}

/* Grab (or recall) a blend descriptor → its stable blend_ref; emit OSR_BLEND on
 * the first sighting of a unique descriptor.  Returns 0 if desc is unreadable. */
static uint32_t blend_capture(const void *desc)
{
    if (!bg_readable(desc, BG_CH_BASE + 3u * BG_CH_STRIDE)) return 0;

    int      mode = *(const int32_t *)desc;
    int32_t  shift[3];
    uint32_t mask[3];
    const uint8_t *lut[3];
    uint32_t maxch[3];
    for (int i = 0; i < 3; i++) {
        const uint8_t *cb = (const uint8_t *)desc + BG_CH_BASE + (unsigned)i * BG_CH_STRIDE;
        shift[i] = *(const int32_t *)(cb + 0);
        mask[i]  = *(const uint32_t *)(cb + 4);
        lut[i]   = *(const uint8_t * const *)(cb + 8);
        maxch[i] = mask[i] >> ((uint32_t)shift[i] & 0x1fu);
    }
    uint32_t gray_max = (maxch[0] + maxch[1] + maxch[2]) / 3u;

    /* Content key: mode + per-channel shift/mask/lut-pointer (the LUT pointers
     * are stable globals, so identical content → identical key, even when the
     * descriptor itself is a transient stack object). */
    uint32_t key = bg_mix(BG_FNV1A_SEED, (uint32_t)mode);
    for (int i = 0; i < 3; i++) {
        key = bg_mix(key, (uint32_t)shift[i]);
        key = bg_mix(key, mask[i]);
        key = bg_mix(key, (uint32_t)(uintptr_t)lut[i]);
    }
    if (key == 0) key = 1;                       /* 0 is the empty-slot sentinel */

    unsigned h = key & (BG_SLOTS - 1u);
    unsigned ins = BG_SLOTS;
    for (unsigned i = 0; i < BG_SLOTS; i++) {
        unsigned s = (h + i) & (BG_SLOTS - 1u);
        if (g_bg[s].used && g_bg[s].key == key) return g_bg[s].ref;  /* cached */
        if (!g_bg[s].used) { ins = s; break; }
    }
    if (ins == BG_SLOTS) return 0;               /* table full */

    uint32_t lut_len[3];
    for (int i = 0; i < 3; i++) {
        uint32_t L = bg_lut_len(mode, maxch[i], gray_max);
        if (L > BG_LUT_CAP) L = BG_LUT_CAP;
        if (!bg_readable(lut[i], L)) L = 0;      /* a bad LUT ptr → empty (skip) */
        lut_len[i] = L;
    }

    /* Pack the 3 LUTs contiguously into a stack buffer for the writer. */
    static uint8_t buf[3u * BG_LUT_CAP];
    uint32_t off = 0;
    for (int i = 0; i < 3; i++) {
        if (lut_len[i]) memcpy(buf + off, lut[i], lut_len[i]);
        off += lut_len[i];
    }

    uint32_t ref = g_bg_next++;
    g_bg[ins].used = 1;
    g_bg[ins].key  = key;
    g_bg[ins].ref  = ref;

    if (ref <= 5)
        proxy_logf("[blend] ref=%lu desc=%p mode=%d lut=%p/%p/%p len=%lu/%lu/%lu",
                   (unsigned long)ref, desc, mode, lut[0], lut[1], lut[2],
                   (unsigned long)lut_len[0], (unsigned long)lut_len[1],
                   (unsigned long)lut_len[2]);

    osr_blend b;
    memset(&b, 0, sizeof b);
    b.blend_ref = ref;
    b.mode = mode;
    for (int i = 0; i < 3; i++) {
        b.shift[i]   = shift[i];
        b.mask[i]    = mask[i];
        b.lut_len[i] = lut_len[i];
    }
    b.lut = buf;
    osr_w_blend(&b);
    return ref;
}

#endif /* OSS_BLEND_GRAB_H */
