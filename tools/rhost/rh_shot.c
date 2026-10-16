/* rh_shot.c — screenshot retail's back buffer at exact frames (--shot-at F[,F..]).
 * Written as PNG (stored deflate blocks, no zlib dependency) to <rundir>\shot_fNNNNNN.png.
 * (PNG writer from openrecet tools/rhost.) */
#include "rhost.h"
#include <ddraw.h>

static uint32_t crc_tab[256];
static uint32_t crc32_upd(uint32_t c, const uint8_t *p, size_t n)
{
    if (!crc_tab[1]) for (uint32_t i = 0; i < 256; i++) { uint32_t k = i; for (int j = 0; j < 8; j++) k = k & 1 ? 0xedb88320u ^ (k >> 1) : k >> 1; crc_tab[i] = k; }
    for (size_t i = 0; i < n; i++) c = crc_tab[(c ^ p[i]) & 0xff] ^ (c >> 8);
    return c;
}
static void be32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
static void chunk(FILE *f, const char *type, const uint8_t *d, uint32_t n)
{
    uint8_t h[8]; be32(h, n); memcpy(h + 4, type, 4); fwrite(h, 1, 8, f);
    if (n) fwrite(d, 1, n, f);
    uint32_t c = crc32_upd(0xffffffffu, (const uint8_t *)type, 4); c = crc32_upd(c, d, n) ^ 0xffffffffu;
    uint8_t t[4]; be32(t, c); fwrite(t, 1, 4, f);
}
static int write_png(const char *path, const uint8_t *rgb, int w, int h)
{
    size_t raw_n = (size_t)h * (1 + 3 * w);
    uint8_t *raw = malloc(raw_n);
    for (int y = 0; y < h; y++) { raw[y * (1 + 3 * w)] = 0; memcpy(raw + y * (1 + 3 * w) + 1, rgb + (size_t)y * 3 * w, 3 * w); }
    size_t nblk = (raw_n + 65534) / 65535, z_n = 2 + raw_n + nblk * 5 + 4;
    uint8_t *z = malloc(z_n), *o = z;
    *o++ = 0x78; *o++ = 0x01;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < raw_n; i++) { a = (a + raw[i]) % 65521; b = (b + a) % 65521; }
    for (size_t off = 0; off < raw_n; off += 65535) {
        uint32_t len = (uint32_t)(raw_n - off < 65535 ? raw_n - off : 65535);
        *o++ = off + len >= raw_n; *o++ = len & 0xff; *o++ = len >> 8; *o++ = ~len & 0xff; *o++ = (~len >> 8) & 0xff;
        memcpy(o, raw + off, len); o += len;
    }
    be32(o, (b << 16) | a); o += 4;
    FILE *f = fopen(path, "wb");
    if (!f) { free(raw); free(z); return -1; }
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
    fwrite(sig, 1, 8, f);
    uint8_t ih[13]; be32(ih, w); be32(ih + 4, h); ih[8] = 8; ih[9] = 2; ih[10] = ih[11] = ih[12] = 0;
    chunk(f, "IHDR", ih, 13); chunk(f, "IDAT", z, (uint32_t)(o - z)); chunk(f, "IEND", NULL, 0);
    fclose(f); free(raw); free(z);
    return 0;
}

/* The frame is read straight from retail's DirectDraw back buffer at Flip entry (the
 * composed frame, before present) — window placement / DWM / --quiet never matter.
 * zdd (DAT_ g_ed->zdd) +0x16c = back-buffer object, whose +0x2c = IDirectDrawSurface7*
 * (0x5b8fc0: primary->Blt(rect, *(*(this+364)+44), ...)). */
static int mask_shift(DWORD m, int *bits) { int s = 0, b = 0; if (!m) { *bits = 0; return 0; } while (!(m & 1)) { m >>= 1; s++; } while (m & 1) { m >>= 1; b++; } *bits = b; return s; }
static uint8_t chan(uint32_t px, DWORD mask)
{
    int bits, sh = mask_shift(mask, &bits);
    if (!bits) return 0;
    uint32_t v = (px & mask) >> sh;
    return (uint8_t)(bits >= 8 ? v >> (bits - 8) : (v << (8 - bits)) | (v >> (2 * bits - 8 > 0 ? 2 * bits - 8 : 0)));
}
void rh_shot(const char *tag)
{
    uint32_t zdd = G32(g_ed->zdd);
    uint32_t bb = zdd ? G32(zdd + 0x16c) : 0;
    IDirectDrawSurface7 *s = bb ? (IDirectDrawSurface7 *)(uintptr_t)G32(bb + 0x2c) : NULL;
    if (!s) { rh_log("shot %s: no back buffer yet (frame %u)\n", tag, g_frame); return; }
    DDSURFACEDESC2 d; memset(&d, 0, sizeof d); d.dwSize = sizeof d;
    HRESULT hr = IDirectDrawSurface7_Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
    if (hr != DD_OK) { rh_log("shot %s: Lock failed %08lx\n", tag, (unsigned long)hr); return; }
    int w = (int)d.dwWidth, h = (int)d.dwHeight, bpp = (int)d.ddpfPixelFormat.dwRGBBitCount;
    uint8_t *rgb = malloc((size_t)w * h * 3);
    for (int y = 0; y < h; y++) {
        const uint8_t *row = (const uint8_t *)d.lpSurface + (size_t)y * d.lPitch;
        for (int x = 0; x < w; x++) {
            uint32_t px = bpp == 32 ? ((const uint32_t *)row)[x] : bpp == 16 ? ((const uint16_t *)row)[x]
                        : bpp == 24 ? (uint32_t)row[3 * x] | (uint32_t)row[3 * x + 1] << 8 | (uint32_t)row[3 * x + 2] << 16 : 0;
            uint8_t *o = rgb + ((size_t)y * w + x) * 3;
            o[0] = chan(px, d.ddpfPixelFormat.dwRBitMask); o[1] = chan(px, d.ddpfPixelFormat.dwGBitMask); o[2] = chan(px, d.ddpfPixelFormat.dwBBitMask);
        }
    }
    IDirectDrawSurface7_Unlock(s, NULL);
    char path[MAX_PATH]; snprintf(path, sizeof path, "%s\\shot_%s.png", O.rundir, tag);
    write_png(path, rgb, w, h);
    free(rgb);
    rh_log("shot %s %dx%d %dbpp -> %s\n", tag, w, h, bpp, path);
}

/* --fb-hash: the composed back buffer at Flip entry, hashed (pixels only: rows of the visible width, the pitch
 * padding left out) — pure vs hybrid runs then prove the RENDER, not just the state behind it. */
int rh_fb_hash(uint64_t *out)
{
    uint32_t zdd = G32(g_ed->zdd);
    uint32_t bb = zdd ? G32(zdd + 0x16c) : 0;
    IDirectDrawSurface7 *s = bb ? (IDirectDrawSurface7 *)(uintptr_t)G32(bb + 0x2c) : NULL;
    if (!s) return 0;
    DDSURFACEDESC2 d; memset(&d, 0, sizeof d); d.dwSize = sizeof d;
    if (IDirectDrawSurface7_Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) return 0;
    size_t rowb = (size_t)d.dwWidth * (d.ddpfPixelFormat.dwRGBBitCount / 8);
    uint64_t h = 0xcbf29ce484222325ull ^ ((uint64_t)d.dwWidth << 32 | d.dwHeight);
    for (DWORD y = 0; y < d.dwHeight; y++) {
        const uint8_t *row = (const uint8_t *)d.lpSurface + (size_t)y * d.lPitch;
        size_t x = 0;
        for (; x + 8 <= rowb; x += 8) { uint64_t v; memcpy(&v, row + x, 8); h = (h ^ v) * 0x100000001b3ull; h ^= h >> 29; }
        for (; x < rowb; x++) h = (h ^ row[x]) * 0x100000001b3ull;
    }
    IDirectDrawSurface7_Unlock(s, NULL);
    *out = h;
    return 1;
}
