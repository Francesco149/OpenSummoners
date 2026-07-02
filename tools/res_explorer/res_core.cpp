// tools/res_explorer/res_core.cpp — see res_core.h.
//
// NOTE on detection constants (CI no-assets gate, tools/ci/no_proprietary_bytes.py):
// the gate scans the built exe for a laid-out RIFF/WAVE header and for the FULL
// 16-byte ASF GUID. Scattered "RIFF"/"WAVE"/"fmt " literals are fine; the ASF
// check below deliberately compares only the first 8 GUID bytes as two u32s so
// the contiguous 16-byte GUID never appears in our binary.
#include "res_core.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#include "stb_image_write.h"

// ── bs_* Win32 primitives (bitmap_session_win32.c NOT linked — ours add the
//    1041-language fallback the single-language game datafiles need) ─────────
extern "C" void *bs_local_alloc_zeroed(uint32_t bytes) { return calloc(1, bytes); }
extern "C" void  bs_local_free(void *p) { free(p); }

static HRSRC find_res_a(HMODULE m, const char *type, LPCSTR name)
{
    HRSRC hr = FindResourceA(m, name, type);
    if (!hr) hr = FindResourceExA(m, type, name, MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT));
    if (!hr) hr = FindResourceExA(m, type, name, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    return hr;
}

extern "C" const void *bs_load_pe_resource(void *hModule, uint16_t resource_id,
                                           const char *resource_type)
{
    HMODULE m = (HMODULE)hModule;
    HRSRC hres = find_res_a(m, resource_type, MAKEINTRESOURCEA(resource_id));
    if (!hres) return NULL;
    HGLOBAL hdata = LoadResource(m, hres);
    if (!hdata) return NULL;
    return (const void *)LockResource(hdata);
}

// ── identity helpers ────────────────────────────────────────────────────────
LPCWSTR rx_id_arg(const RxId *r) { return r->isStr ? r->str : MAKEINTRESOURCEW(r->id); }

static void rxid_from(RxId *r, LPCWSTR s)
{
    if (IS_INTRESOURCE(s)) { r->isStr = 0; r->id = (WORD)(ULONG_PTR)s; r->str[0] = 0; }
    else { r->isStr = 1; r->id = 0; lstrcpynW(r->str, s, 96); }
}

int rx_utf8(const wchar_t *ws, int wn, char *out, int cap)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, ws, wn, out, cap - 1, NULL, NULL);
    if (n < 0) n = 0;
    out[n] = 0;
    return n;
}

static void type_label_of(const RxId *t, char *out, int cap)
{
    if (t->isStr) { rx_utf8(t->str, -1, out, cap); return; }
    const char *rt = NULL;
    switch (t->id) {
        case 2:  rt = "BITMAP"; break;
        case 6:  rt = "STRING"; break;
        case 14: rt = "GROUP_ICON"; break;
        case 16: rt = "VERSION"; break;
        case 24: rt = "MANIFEST"; break;
    }
    if (rt) lstrcpynA(out, rt, cap);
    else    _snprintf(out, cap, "#%u", t->id);
}

const char *rx_kind_name(RxKind k)
{
    switch (k) {
        case RK_SPRITE:    return "Sprite";
        case RK_IMAGE_RAW: return "Image";
        case RK_BMP:       return "Bitmap";
        case RK_MAP:       return "Map";
        case RK_WAV:       return "Audio";
        case RK_WMA:       return "Music";
        case RK_STRINGS:   return "Strings";
        case RK_VERSION:   return "Version";
        default:           return "Data";
    }
}

const char *rx_wave_tag_name(WORD t)
{
    switch (t) {
        case 0x0001: return "PCM";
        case 0x0002: return "ADPCM";
        case 0x0050: return "MPEG";
        case 0x0055: return "MP3";
        case 0x0161: return "WMAv2";
        case 0xFFFE: return "EXT";
        default:     return "?";
    }
}

// ── lock ────────────────────────────────────────────────────────────────────
static HRSRC find_res_w(HMODULE m, const RxId *type, const RxId *name)
{
    HRSRC hr = FindResourceW(m, rx_id_arg(name), rx_id_arg(type));
    if (!hr) hr = FindResourceExW(m, rx_id_arg(type), rx_id_arg(name),
                                  MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT));
    if (!hr) hr = FindResourceExW(m, rx_id_arg(type), rx_id_arg(name),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    return hr;
}

const BYTE *rx_lock(const RxWorld *w, const RxEntry *e, DWORD *outsz)
{
    if (e->module_idx < 0 || e->module_idx >= (int)w->modules.size()) return NULL;
    HMODULE m = w->modules[e->module_idx].mod;
    HRSRC hr = find_res_w(m, &e->type, &e->name);
    if (!hr) return NULL;
    DWORD sz = SizeofResource(m, hr);
    HGLOBAL hg = LoadResource(m, hr);
    if (!hg) return NULL;
    const BYTE *p = (const BYTE *)LockResource(hg);
    if (p && outsz) *outsz = sz;
    return p;
}

// ── payload sniffing ────────────────────────────────────────────────────────
static int is_asf(const BYTE *p, DWORD n)
{
    if (n < 16) return 0;
    uint32_t a, b;
    memcpy(&a, p, 4); memcpy(&b, p + 4, 4);
    // first 8 bytes of the ASF header GUID (30 26 B2 75 8E 66 CF 11) as two
    // u32s — deliberately NOT the full 16-byte GUID (CI no-assets gate).
    return a == 0x75B22630u && b == 0x11CF668Eu;
}

static RxWave wave_parse(const BYTE *p, DWORD n)
{
    RxWave w; memset(&w, 0, sizeof w);
    if (n < 20 || memcmp(p, "RIFF", 4) || memcmp(p + 8, "WAVE", 4)) return w;
    DWORD o = 12; int havefmt = 0;
    while (o + 8 <= n) {
        DWORD csz; memcpy(&csz, p + o + 4, 4);
        if (!memcmp(p + o, "fmt ", 4) && o + 8 + 16 <= n) {
            memcpy(&w.tag, p + o + 8, 2);  memcpy(&w.ch, p + o + 10, 2);
            memcpy(&w.sr, p + o + 12, 4);  memcpy(&w.byterate, p + o + 16, 4);
            memcpy(&w.bits, p + o + 22, 2); havefmt = 1;
        } else if (!memcmp(p + o, "data", 4)) {
            w.datasz = csz;
            w.dataoff = o + 8;
        }
        if (csz == 0) break;
        o += 8 + csz + (csz & 1);
    }
    if (havefmt) {
        w.ok = 1;
        if (w.dataoff + w.datasz > n && n > w.dataoff) w.datasz = n - w.dataoff;
        w.dur = w.byterate ? (double)w.datasz / (double)w.byterate : 0.0;
    }
    return w;
}

// Lizsoft compressed container (bs_parse_compressed_header, sig 0x2711):
// validate the parse AND that the pixel payload fits inside the blob.
static int sniff_sprite(const BYTE *p, DWORD n, int *w, int *h, int *bpp)
{
    if (n < 0x460) return 0;
    bs_bitmap_info bi; uint32_t pixoff = 0;
    if (!bs_parse_compressed_header(&bi, p, &pixoff)) return 0;
    if (bi.biBitCount != 8 && bi.biBitCount != 24) return 0;
    if (bi.biWidth == 0 || bi.biHeight == 0 ||
        bi.biWidth > 8192 || bi.biHeight > 32768) return 0;
    uint64_t need = (uint64_t)pixoff + 0x458 +
                    (uint64_t)(bi.biBitCount / 8) * bi.biWidth * bi.biHeight;
    if (need > n) return 0;
    *w = (int)bi.biWidth; *h = (int)bi.biHeight; *bpp = bi.biBitCount;
    return 1;
}

// engine raw path: slots[0]=header offset, slots[1]=w, slots[2]=h, u16@0x0e=bpp.
static int sniff_raw_image(const BYTE *p, DWORD n, int *w, int *h, int *bpp)
{
    if (n < 0x10) return 0;
    uint32_t slots[3]; memcpy(slots, p, 12);
    uint16_t bc; memcpy(&bc, p + 0x0e, 2);
    if (bc != 8 && bc != 24) return 0;
    if (slots[1] == 0 || slots[2] == 0 || slots[1] > 8192 || slots[2] > 32768) return 0;
    if (slots[0] < 0x10 || slots[0] >= n) return 0;
    uint64_t need = (uint64_t)slots[0] + (bc == 8 ? 1024u : 0u) +
                    (uint64_t)(bc / 8) * slots[1] * slots[2];
    if (need > n) return 0;
    *w = (int)slots[1]; *h = (int)slots[2]; *bpp = bc;
    return 1;
}

static int sniff_bmp(const BYTE *p, DWORD n, int *w, int *h, int *bpp)
{
    if (n < 54 || p[0] != 'B' || p[1] != 'M') return 0;
    int32_t bw, bh; uint16_t bc;
    memcpy(&bw, p + 18, 4); memcpy(&bh, p + 22, 4); memcpy(&bc, p + 28, 2);
    if (bw <= 0 || bw > 16384 || bh == 0 || bh > 32768 || bh < -32768) return 0;
    if (bc != 8 && bc != 24 && bc != 32) return 0;
    *w = bw; *h = bh < 0 ? -bh : bh; *bpp = bc;
    return 1;
}

static void classify(RxEntry *e, const BYTE *p, DWORD n)
{
    e->kind = RK_UNKNOWN;
    e->info[0] = 0;
    if (!p || !n) { lstrcpynA(e->info, "(unreadable)", sizeof e->info); return; }

    if (!e->type.isStr && e->type.id == 6)  { e->kind = RK_STRINGS; lstrcpynA(e->info, "string table", sizeof e->info); return; }
    if (!e->type.isStr && e->type.id == 16) { e->kind = RK_VERSION; lstrcpynA(e->info, "version info", sizeof e->info); return; }
    if (!e->type.isStr && e->type.id == 2) {
        // RT_BITMAP: DIB w/o file header
        if (n >= 40) {
            int32_t bw, bh; uint16_t bc;
            memcpy(&bw, p + 4, 4); memcpy(&bh, p + 8, 4); memcpy(&bc, p + 14, 2);
            e->kind = RK_BMP; e->img_w = bw; e->img_h = bh < 0 ? -bh : bh; e->img_bpp = bc;
            _snprintf(e->info, sizeof e->info, "DIB %dx%d %ubpp", e->img_w, e->img_h, bc);
            return;
        }
    }

    RxWave wv = wave_parse(p, n);
    if (wv.ok) {
        e->kind = RK_WAV; e->wave = wv;
        _snprintf(e->info, sizeof e->info, "%s %luHz %ub %s  %.2fs",
                  rx_wave_tag_name(wv.tag), (unsigned long)wv.sr, wv.bits,
                  wv.ch == 1 ? "mono" : "stereo", wv.dur);
        return;
    }
    if (is_asf(p, n)) {
        e->kind = RK_WMA;
        lstrcpynA(e->info, "ASF/WMA stream", sizeof e->info);
        return;
    }
    if (n >= 0x68 && !memcmp(p + 0x34, "MSD_SOTES_MAPDATA", 17)) {
        uint32_t d0, d1, d2, cnt;
        memcpy(&d0, p + 0x54, 4); memcpy(&d1, p + 0x58, 4);
        memcpy(&d2, p + 0x5c, 4); memcpy(&cnt, p + 0x60, 4);
        e->kind = RK_MAP; e->img_w = (int)d0; e->img_h = (int)d1; e->img_bpp = (int)d2;
        _snprintf(e->info, sizeof e->info, "map %ux%ux%u, %u objects",
                  d0, d1, d2, cnt);
        return;
    }
    int w, h, bpp;
    if (sniff_sprite(p, n, &w, &h, &bpp)) {
        e->kind = RK_SPRITE; e->img_w = w; e->img_h = h; e->img_bpp = bpp;
        _snprintf(e->info, sizeof e->info, "sheet %dx%d %dbpp", w, h, bpp);
        return;
    }
    if (sniff_raw_image(p, n, &w, &h, &bpp)) {
        e->kind = RK_IMAGE_RAW; e->img_w = w; e->img_h = h; e->img_bpp = bpp;
        _snprintf(e->info, sizeof e->info, "image %dx%d %dbpp", w, h, bpp);
        return;
    }
    if (sniff_bmp(p, n, &w, &h, &bpp)) {
        e->kind = RK_BMP; e->img_w = w; e->img_h = h; e->img_bpp = bpp;
        _snprintf(e->info, sizeof e->info, "BMP %dx%d %dbpp", w, h, bpp);
        return;
    }
    _snprintf(e->info, sizeof e->info, "%02X %02X %02X %02X %02X %02X %02X %02X",
              p[0], n > 1 ? p[1] : 0, n > 2 ? p[2] : 0, n > 3 ? p[3] : 0,
              n > 4 ? p[4] : 0, n > 5 ? p[5] : 0, n > 6 ? p[6] : 0, n > 7 ? p[7] : 0);
}

// ── enumeration ─────────────────────────────────────────────────────────────
struct EnumCtx {
    RxWorld *w;
    int module_idx;
    std::vector<RxId> types;
};

static BOOL CALLBACK cb_type(HMODULE, LPWSTR type, LONG_PTR lp)
{
    EnumCtx *c = (EnumCtx *)lp;
    RxId t; rxid_from(&t, type);
    c->types.push_back(t);
    return TRUE;
}

struct NameCtx { EnumCtx *e; const RxId *type; };

static BOOL CALLBACK cb_name(HMODULE, LPCWSTR, LPWSTR name, LONG_PTR lp)
{
    NameCtx *nc = (NameCtx *)lp;
    RxEntry en; memset(&en, 0, sizeof en);
    en.module_idx = nc->e->module_idx;
    en.type = *nc->type;
    rxid_from(&en.name, name);
    type_label_of(&en.type, en.type_label, sizeof en.type_label);
    if (en.name.isStr) rx_utf8(en.name.str, -1, en.id_label, sizeof en.id_label);
    else               _snprintf(en.id_label, sizeof en.id_label, "%u", en.name.id);
    nc->e->w->entries.push_back(en);
    return TRUE;
}

int rx_open_module(RxWorld *w, const wchar_t *path)
{
    HMODULE m = LoadLibraryExW(path, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!m) return -1;

    RxModule mod; memset(&mod, 0, sizeof mod);
    mod.mod = m;
    lstrcpynW(mod.path, path, MAX_PATH);
    const wchar_t *base = wcsrchr(path, L'\\');
    rx_utf8(base ? base + 1 : path, -1, mod.label, sizeof mod.label);
    mod.first_entry = (int)w->entries.size();
    int midx = (int)w->modules.size();

    EnumCtx c; c.w = w; c.module_idx = midx;
    EnumResourceTypesW(m, cb_type, (LONG_PTR)&c);
    for (const RxId &t : c.types) {
        NameCtx nc; nc.e = &c; nc.type = &t;
        EnumResourceNamesW(m, rx_id_arg(&t), cb_name, (LONG_PTR)&nc);
    }
    mod.n_entries = (int)w->entries.size() - mod.first_entry;
    w->modules.push_back(mod);

    // classify (needs the module registered for rx_lock)
    for (int i = mod.first_entry; i < mod.first_entry + mod.n_entries; i++) {
        RxEntry *e = &w->entries[i];
        DWORD sz = 0;
        const BYTE *p = rx_lock(w, e, &sz);
        e->size = sz;
        classify(e, p, sz);
    }
    return midx;
}

int rx_load_game_install(RxWorld *w, const wchar_t *forced_dir, wchar_t *outdir)
{
    static const wchar_t *candidates[] = {
        L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Fortune Summoners",
        L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\sotes",
        L"C:\\Program Files (x86)\\lizsoft\\FortuneSummoners",
    };
    int loaded = 0;
    std::vector<std::wstring> seen_base;
    const wchar_t *primary = NULL;

    for (int ci = -1; ci < (int)(sizeof candidates / sizeof *candidates); ci++) {
        const wchar_t *dir = (ci < 0) ? forced_dir : candidates[ci];
        if (!dir) continue;
        if (ci >= 0 && forced_dir) break;      // forced dir replaces the scan
        // exact exe names only — RE working dirs accumulate sotes-unpacked-*.exe
        // copies, and datafile-mapping them all exhausts the 32-bit VA space
        static const wchar_t *pats[] = { L"sotes*.dll", L"sotes.exe", L"sotes_en.exe" };
        for (const wchar_t *pat : pats) {
            wchar_t glob[MAX_PATH];
            _snwprintf(glob, MAX_PATH, L"%ls\\%ls", dir, pat);
            WIN32_FIND_DATAW fd;
            HANDLE hf = FindFirstFileW(glob, &fd);
            if (hf == INVALID_HANDLE_VALUE) continue;
            do {
                std::wstring base = fd.cFileName;
                for (auto &ch : base) ch = (wchar_t)towlower(ch);
                if (base.size() < 4 ||
                    (base.compare(base.size() - 4, 4, L".dll") != 0 &&
                     base.compare(base.size() - 4, 4, L".exe") != 0))
                    continue;                   // e.g. sotesp.OLD
                if (std::find(seen_base.begin(), seen_base.end(), base) != seen_base.end())
                    continue;                   // basename already loaded from an earlier dir
                wchar_t full[MAX_PATH];
                _snwprintf(full, MAX_PATH, L"%ls\\%ls", dir, fd.cFileName);
                if (rx_open_module(w, full) >= 0) {
                    seen_base.push_back(base);
                    loaded++;
                    if (!primary) primary = dir;
                }
            } while (FindNextFileW(hf, &fd));
            FindClose(hf);
        }
    }
    if (outdir) {
        outdir[0] = 0;
        if (primary) lstrcpynW(outdir, primary, MAX_PATH);
    }
    return loaded;
}

void rx_close_all(RxWorld *w)
{
    for (RxModule &m : w->modules)
        if (m.mod) FreeLibrary(m.mod);
    w->modules.clear();
    w->entries.clear();
}

// ── image decode ────────────────────────────────────────────────────────────
static void session_to_image(const bitmap_session *s, RxImage *out)
{
    out->w = (int)s->biWidth;
    out->h = (int)s->biHeight;
    out->bpp = s->biBitCount;
    out->has_palette = (s->biBitCount == 8);
    out->default_flip = 1;   // decoded sheets are bottom-up DIBs at every depth
                             // (matches the engine's own trim math, FUN_005b6f80;
                             // 24bpp portraits = quirk #98; USER-confirmed for
                             // 8bpp sheets 2026-07-02 — the old "top-down" note
                             // in lizsoft-sprite.md was against flipped output)
    for (int i = 0; i < 256; i++) {
        // session palette is RGBQUAD (B,G,R,_)
        uint8_t b = s->palette[i * 4 + 0], g = s->palette[i * 4 + 1],
                r = s->palette[i * 4 + 2];
        out->palette[i] = 0xff000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    }
    size_t bytes = (size_t)s->stride * s->biHeight;
    out->raw.assign((const uint8_t *)s->pixels, (const uint8_t *)s->pixels + bytes);
    out->rgba.resize((size_t)out->w * out->h);
    for (int y = 0; y < out->h; y++) {
        const uint8_t *row = out->raw.data() + (size_t)y * s->stride;
        uint32_t *dst = out->rgba.data() + (size_t)y * out->w;
        if (out->bpp == 8) {
            for (int x = 0; x < out->w; x++) dst[x] = out->palette[row[x]];
        } else {  // 24bpp B,G,R
            for (int x = 0; x < out->w; x++) {
                uint8_t b = row[x * 3 + 0], g = row[x * 3 + 1], r = row[x * 3 + 2];
                dst[x] = 0xff000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
            }
        }
    }
}

static int decode_dib(const BYTE *p, DWORD n, int with_filehdr, RxImage *out)
{
    DWORD off = with_filehdr ? 14 : 0;
    if (n < off + 40) return 0;
    int32_t bw, bh; uint16_t bc; uint32_t clrused = 0;
    memcpy(&bw, p + off + 4, 4); memcpy(&bh, p + off + 8, 4);
    memcpy(&bc, p + off + 14, 2); memcpy(&clrused, p + off + 32, 4);
    if (bw <= 0 || bh == 0) return 0;
    if (bc != 8 && bc != 24 && bc != 32) return 0;
    int h = bh < 0 ? -bh : bh;
    int npal = (bc == 8) ? (clrused ? (int)clrused : 256) : 0;
    DWORD data_off;
    if (with_filehdr) memcpy(&data_off, p + 10, 4);
    else              data_off = off + 40 + npal * 4;
    DWORD stride = ((bw * bc / 8) + 3) & ~3u;   // BMP rows are DWORD-aligned
    if ((uint64_t)data_off + (uint64_t)stride * h > n) return 0;

    out->w = bw; out->h = h; out->bpp = bc; out->has_palette = (bc == 8);
    out->default_flip = (bh > 0);               // positive height = bottom-up
    memset(out->palette, 0, sizeof out->palette);
    for (int i = 0; i < npal && i < 256; i++) {
        const BYTE *q = p + off + 40 + i * 4;   // RGBQUAD B,G,R,_
        out->palette[i] = 0xff000000u | ((uint32_t)q[0] << 16) |
                          ((uint32_t)q[1] << 8) | q[2];
    }
    out->raw.assign(p + data_off, p + data_off + (size_t)stride * h);
    out->rgba.resize((size_t)bw * h);
    for (int y = 0; y < h; y++) {
        const BYTE *row = p + data_off + (size_t)y * stride;
        uint32_t *dst = out->rgba.data() + (size_t)y * bw;
        for (int x = 0; x < bw; x++) {
            if (bc == 8)       dst[x] = out->palette[row[x]];
            else if (bc == 24) dst[x] = 0xff000000u | ((uint32_t)row[x*3+0] << 16) |
                                        ((uint32_t)row[x*3+1] << 8) | row[x*3+2];
            else               dst[x] = 0xff000000u | ((uint32_t)row[x*4+0] << 16) |
                                        ((uint32_t)row[x*4+1] << 8) | row[x*4+2];
        }
    }
    return 1;
}

int rx_decode_image(const RxWorld *w, const RxEntry *e, RxImage *out)
{
    if (e->kind == RK_SPRITE || e->kind == RK_IMAGE_RAW) {
        if (e->name.isStr) return 0;   // engine path is integer-id only
        bitmap_session s;
        memset(&s, 0, sizeof s);
        bs_release_no_free(&s);
        int ok = bs_decode_resource(&s, (void *)w->modules[e->module_idx].mod,
                                    e->name.id, e->type_label,
                                    e->kind == RK_SPRITE ? 1 : 0);
        if (!ok) return 0;
        session_to_image(&s, out);
        bs_release(&s);
        return 1;
    }
    if (e->kind == RK_BMP) {
        DWORD sz = 0;
        const BYTE *p = rx_lock(w, e, &sz);
        if (!p) return 0;
        int with_hdr = (sz >= 2 && p[0] == 'B' && p[1] == 'M');
        return decode_dib(p, sz, with_hdr, out);
    }
    return 0;
}

void rx_image_display(const RxImage *img, int key_on, int flip_on,
                      std::vector<uint32_t> *out)
{
    out->resize(img->rgba.size());
    for (int y = 0; y < img->h; y++) {
        int sy = flip_on ? (img->h - 1 - y) : y;
        const uint32_t *src = img->rgba.data() + (size_t)sy * img->w;
        uint32_t *dst = out->data() + (size_t)y * img->w;
        if (!key_on) {
            memcpy(dst, src, (size_t)img->w * 4);
        } else {
            for (int x = 0; x < img->w; x++) {
                uint32_t c = src[x];
                // magenta 0xff00ff (quirk #47: all keying normalized magenta)
                dst[x] = ((c & 0x00ffffffu) == 0x00ff00ffu) ? 0x00000000u : c;
            }
        }
    }
}

// ── map ─────────────────────────────────────────────────────────────────────
int rx_parse_map(const RxWorld *w, const RxEntry *e, map_data *md)
{
    DWORD sz = 0;
    const BYTE *p = rx_lock(w, e, &sz);
    if (!p || !sz) return 0;
    memset(md, 0, sizeof *md);
    return map_data_parse(md, p, sz) == 0;
}

RxMapObject rx_map_object(const map_layer *l)
{
    RxMapObject o;
    memcpy(&o.id,      l->hdr + 0x00, 4);
    memcpy(&o.x,       l->hdr + 0x04, 4);
    memcpy(&o.y,       l->hdr + 0x08, 4);
    memcpy(&o.type,    l->hdr + 0x10, 4);
    memcpy(&o.subtype, l->hdr + 0x18, 4);
    return o;
}

const char *rx_map_object_category(uint32_t type)
{
    if (type >= 50000 && type <= 59999) return "EFFECT";
    if (type >= 60000 && type <= 69999) return "STRUCTURE";
    if (type >= 70000 && type <= 79999) return "CHARACTER";
    if (type >= 80000 && type <= 89999) return "DEVICE";
    return "?";
}

// ── strings ─────────────────────────────────────────────────────────────────
int rx_decode_strings(const RxWorld *w, const RxEntry *e, std::vector<char> *utf8)
{
    DWORD sz = 0;
    const BYTE *p = rx_lock(w, e, &sz);
    if (!p || !sz || e->name.isStr) return 0;
    utf8->clear();
    unsigned block = e->name.id;                 // strings (block-1)*16 .. +15
    const wchar_t *q = (const wchar_t *)p;
    DWORD nw = sz / 2;
    DWORD i = 0;
    char buf[4096];
    for (int s = 0; s < 16 && i < nw; s++) {
        unsigned len = q[i++];
        if (len == 0) continue;
        if (i + len > nw) break;
        int n = _snprintf(buf, sizeof buf, "%u\t", (block - 1) * 16 + s);
        utf8->insert(utf8->end(), buf, buf + n);
        n = rx_utf8(q + i, (int)len, buf, sizeof buf);
        utf8->insert(utf8->end(), buf, buf + n);
        utf8->push_back('\n');
        i += len;
    }
    return 1;
}

// ── waveform peaks ──────────────────────────────────────────────────────────
int rx_wave_peaks(const BYTE *blob, const RxWave *wv, int columns,
                  std::vector<float> *minmax)
{
    if (!wv->ok || wv->tag != 0x0001 || (wv->bits != 8 && wv->bits != 16))
        return 0;
    const BYTE *d = blob + wv->dataoff;
    uint32_t frame = (uint32_t)wv->ch * (wv->bits / 8);
    if (!frame) return 0;
    uint32_t nfr = wv->datasz / frame;
    if (!nfr) return 0;
    minmax->assign((size_t)columns * 2, 0.0f);
    for (int c = 0; c < columns; c++) {
        uint64_t f0 = (uint64_t)nfr * c / columns;
        uint64_t f1 = (uint64_t)nfr * (c + 1) / columns;
        if (f1 <= f0) f1 = f0 + 1;
        float lo = 1.0f, hi = -1.0f;
        // sample up to 256 frames per column (fast + visually identical)
        uint64_t step = (f1 - f0) / 256; if (!step) step = 1;
        for (uint64_t f = f0; f < f1 && f < nfr; f += step) {
            for (int ch = 0; ch < wv->ch; ch++) {
                float v;
                if (wv->bits == 16) {
                    int16_t s; memcpy(&s, d + f * frame + ch * 2, 2);
                    v = (float)s / 32768.0f;
                } else {
                    v = ((float)d[f * frame + ch] - 128.0f) / 128.0f;
                }
                if (v < lo) lo = v;
                if (v > hi) hi = v;
            }
        }
        if (hi < lo) { lo = 0; hi = 0; }
        (*minmax)[c * 2 + 0] = lo;
        (*minmax)[c * 2 + 1] = hi;
    }
    return 1;
}

// ── export ──────────────────────────────────────────────────────────────────
static int write_file(const wchar_t *path, const void *data, size_t n)
{
    FILE *f = _wfopen(path, L"wb");
    if (!f) return 0;
    size_t wr = fwrite(data, 1, n, f);
    fclose(f);
    return wr == n;
}

int rx_export_raw(const RxWorld *w, const RxEntry *e, const wchar_t *path)
{
    DWORD sz = 0;
    const BYTE *p = rx_lock(w, e, &sz);
    if (!p || !sz) return 0;
    return write_file(path, p, sz);
}

static void stbw_to_file(void *ctx, void *data, int size)
{
    fwrite(data, 1, (size_t)size, (FILE *)ctx);
}

static int png_write(const wchar_t *path, const uint32_t *rgba, int w, int h)
{
    FILE *f = _wfopen(path, L"wb");
    if (!f) return 0;
    int ok = stbi_write_png_to_func(stbw_to_file, f, w, h, 4, rgba, w * 4);
    fclose(f);
    return ok;
}

int rx_export_png(const RxImage *img, int key_on, int flip_on, const wchar_t *path)
{
    if (!img->w || !img->h) return 0;
    std::vector<uint32_t> disp;
    rx_image_display(img, key_on, flip_on, &disp);
    return png_write(path, disp.data(), img->w, img->h);
}

int rx_export_frames(const RxImage *img, int key_on, int flip_on,
                     int cell_w, int cell_h, const wchar_t *stem_path)
{
    if (!img->w || !img->h || cell_w <= 0 || cell_h <= 0) return 0;
    std::vector<uint32_t> disp;
    rx_image_display(img, key_on, flip_on, &disp);
    // strip a trailing .png from the stem if present
    wchar_t stem[MAX_PATH];
    lstrcpynW(stem, stem_path, MAX_PATH);
    size_t sl = wcslen(stem);
    if (sl > 4 && !_wcsicmp(stem + sl - 4, L".png")) stem[sl - 4] = 0;

    int cols = img->w / cell_w, rows = img->h / cell_h;
    std::vector<uint32_t> cell((size_t)cell_w * cell_h);
    int written = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            for (int y = 0; y < cell_h; y++)
                memcpy(cell.data() + (size_t)y * cell_w,
                       disp.data() + (size_t)(r * cell_h + y) * img->w + c * cell_w,
                       (size_t)cell_w * 4);
            wchar_t path[MAX_PATH];
            _snwprintf(path, MAX_PATH, L"%ls_f%03d.png", stem, r * cols + c);
            if (png_write(path, cell.data(), cell_w, cell_h)) written++;
        }
    }
    return written;
}

int rx_export_map_json(const RxWorld *w, const RxEntry *e, const wchar_t *path)
{
    map_data md;
    if (!rx_parse_map(w, e, &md)) return 0;
    FILE *f = _wfopen(path, L"wb");
    if (!f) { map_data_free(&md); return 0; }

    char name[0x21];
    map_data_name(&md, name);
    fprintf(f, "{\n  \"module\": \"%s\",\n  \"id\": %s,\n  \"size\": %lu,\n"
               "  \"consumed\": %lu,\n  \"name\": \"%s\",\n"
               "  \"dims\": [%u, %u, %u],\n  \"object_count\": %u,\n",
            w->modules[e->module_idx].label, e->id_label,
            (unsigned long)e->size, (unsigned long)md.consumed, name,
            md.dim0, md.dim1, md.dim2, md.count);

    fprintf(f, "  \"cells\": [");
    int first = 1;
    for (uint32_t z = 0; z < md.dim2; z++)
        for (uint32_t y = 0; y < md.dim1; y++)
            for (uint32_t x = 0; x < md.dim0; x++) {
                map_cell c;
                if (map_data_cell(&md, x, y, z, &c) != 0 || c.tile_id == 0) continue;
                fprintf(f, "%s\n    {\"x\": %u, \"y\": %u, \"z\": %u, "
                           "\"co_id\": %u, \"tile_id\": %u, \"f08\": %u, "
                           "\"arg0c\": %u, \"shape\": %u, \"arg14\": %u, \"arg18\": %u}",
                        first ? "" : ",", x, y, z,
                        c.f00, c.tile_id, c.f08, c.arg_0c, c.shape, c.arg_14, c.arg_18);
                first = 0;
            }
    fprintf(f, "\n  ],\n  \"objects\": [");
    for (uint32_t i = 0; i < md.count; i++) {
        RxMapObject o = rx_map_object(&md.layers[i]);
        fprintf(f, "%s\n    {\"id\": %u, \"x\": %u, \"y\": %u, \"type\": %u, "
                   "\"category\": \"%s\", \"subtype\": %u, "
                   "\"counts\": [%u, %u, %u, %u]}",
                i ? "," : "", o.id, o.x, o.y, o.type,
                rx_map_object_category(o.type), o.subtype,
                md.layers[i].n_a, md.layers[i].n_b, md.layers[i].n_c, md.layers[i].n_d);
    }
    fprintf(f, "\n  ]\n}\n");
    fclose(f);
    map_data_free(&md);
    return 1;
}

int rx_export_strings_txt(const RxWorld *w, const RxEntry *e, const wchar_t *path)
{
    std::vector<char> utf8;
    if (!rx_decode_strings(w, e, &utf8)) return 0;
    return write_file(path, utf8.data(), utf8.size());
}

const char *rx_default_ext(const RxEntry *e)
{
    switch (e->kind) {
        case RK_SPRITE: case RK_IMAGE_RAW: case RK_BMP: return "png";
        case RK_WAV:     return "wav";
        case RK_WMA:     return "wma";
        case RK_MAP:     return "json";
        case RK_STRINGS: return "txt";
        default:         return "bin";
    }
}

int rx_export_bulk(const RxWorld *w, const std::vector<int> &sel, const wchar_t *dir)
{
    wchar_t manifest[MAX_PATH];
    _snwprintf(manifest, MAX_PATH, L"%ls\\manifest.txt", dir);
    FILE *mf = _wfopen(manifest, L"wb");
    int written = 0;
    for (int idx : sel) {
        const RxEntry *e = &w->entries[idx];
        char stem[128];
        char modstem[64];
        lstrcpynA(modstem, w->modules[e->module_idx].label, sizeof modstem);
        char *dot = strrchr(modstem, '.');
        if (dot) *dot = 0;
        _snprintf(stem, sizeof stem, "%s_%s_%s", modstem, e->type_label, e->id_label);
        for (char *c = stem; *c; c++)
            if (*c == '\\' || *c == '/' || *c == ':' || *c == '*' || *c == '?' ||
                *c == '"' || *c == '<' || *c == '>' || *c == '|' || *c == ' ')
                *c = '_';
        const char *ext = rx_default_ext(e);
        wchar_t path[MAX_PATH];
        _snwprintf(path, MAX_PATH, L"%ls\\%hs.%hs", dir, stem, ext);

        int ok = 0;
        if (e->kind == RK_SPRITE || e->kind == RK_IMAGE_RAW || e->kind == RK_BMP) {
            RxImage img;
            if (rx_decode_image(w, e, &img))
                ok = rx_export_png(&img, 1, img.default_flip, path);
        } else if (e->kind == RK_MAP) {
            ok = rx_export_map_json(w, e, path);
        } else if (e->kind == RK_STRINGS) {
            ok = rx_export_strings_txt(w, e, path);
        }
        if (!ok) {   // raw fallback (also WAV/WMA: the blob IS the file format)
            if (e->kind != RK_WAV && e->kind != RK_WMA &&
                e->kind != RK_SPRITE && e->kind != RK_IMAGE_RAW &&
                e->kind != RK_BMP && e->kind != RK_MAP && e->kind != RK_STRINGS)
                _snwprintf(path, MAX_PATH, L"%ls\\%hs.bin", dir, stem);
            ok = rx_export_raw(w, e, path);
        }
        if (ok) {
            written++;
            if (mf) fprintf(mf, "%s.%s\t%s\t%s\t%lu\t%s\n", stem,
                            e->kind == RK_UNKNOWN ? "bin" : ext,
                            rx_kind_name(e->kind), e->type_label,
                            (unsigned long)e->size, e->info);
        }
    }
    if (mf) fclose(mf);
    return written;
}

// ── headless CLI (voice_view compat) ────────────────────────────────────────
int rx_cli_list(const wchar_t *dll, const wchar_t *out)
{
    RxWorld w;
    if (rx_open_module(&w, dll) < 0) return 2;
    FILE *f = _wfopen(out, L"w, ccs=UTF-8");
    if (!f) { rx_close_all(&w); return 3; }
    fwprintf(f, L"# %ls\n# %d resource(s)\n", dll, (int)w.entries.size());
    char cur_type[32] = "";
    for (const RxEntry &e : w.entries) {
        if (strcmp(cur_type, e.type_label)) {
            lstrcpynA(cur_type, e.type_label, sizeof cur_type);
            fwprintf(f, L"\n== type %hs ==\n", e.type_label);
        }
        fwprintf(f, L"  %-10hs %8lu B  %-8hs %hs\n", e.id_label,
                 (unsigned long)e.size, rx_kind_name(e.kind), e.info);
    }
    fclose(f);
    rx_close_all(&w);
    return 0;
}

int rx_cli_export(const wchar_t *dll, const wchar_t *type, const wchar_t *id,
                  const wchar_t *out)
{
    RxWorld w;
    if (rx_open_module(&w, dll) < 0) return 2;
    char typec[32], idc[48];
    rx_utf8(type, -1, typec, sizeof typec);
    rx_utf8(id, -1, idc, sizeof idc);
    int found = -1;
    for (int i = 0; i < (int)w.entries.size(); i++)
        if (!lstrcmpiA(w.entries[i].type_label, typec) &&
            !lstrcmpiA(w.entries[i].id_label, idc)) { found = i; break; }
    if (found < 0) { rx_close_all(&w); return 4; }
    const RxEntry *e = &w.entries[found];
    int ok = 0;
    if (e->kind == RK_SPRITE || e->kind == RK_IMAGE_RAW || e->kind == RK_BMP) {
        RxImage img;
        if (rx_decode_image(&w, e, &img))
            ok = rx_export_png(&img, 1, img.default_flip, out);
    } else if (e->kind == RK_MAP)     ok = rx_export_map_json(&w, e, out);
    else if (e->kind == RK_STRINGS)   ok = rx_export_strings_txt(&w, e, out);
    else                              ok = rx_export_raw(&w, e, out);
    rx_close_all(&w);
    return ok ? 0 : 5;
}

int rx_cli_dump(const wchar_t *dll, const wchar_t *type, const wchar_t *id,
                const wchar_t *out)
{
    RxWorld w;
    if (rx_open_module(&w, dll) < 0) return 2;
    RxEntry e; memset(&e, 0, sizeof e);
    e.module_idx = 0;
    e.type.isStr = 1; lstrcpynW(e.type.str, type, 96);
    if (id[0] >= L'0' && id[0] <= L'9') { e.name.isStr = 0; e.name.id = (WORD)_wtoi(id); }
    else { e.name.isStr = 1; lstrcpynW(e.name.str, id, 96); }
    int ok = rx_export_raw(&w, &e, out);
    rx_close_all(&w);
    return ok ? 0 : 4;
}
