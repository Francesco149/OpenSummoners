// tools/res_explorer/res_core.h — resource model, classification, decode, export.
// Win32-only, no ImGui: the app (res_explorer.cpp) sits on top of this; the
// headless CLI (--list/--dump) uses it directly.
//
// Decoders reused from the port: src/bitmap_session.{c,h} (the engine's own
// raw + compressed sprite decode, FUN_005b7800/FUN_005b7c10) and
// src/map_data.{c,h} (FUN_00587970). This TU supplies the three bs_* Win32
// primitives (bitmap_session_win32.c is NOT linked) so resource lookup gets
// the 1041-language fallback the single-language game datafiles need.
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <vector>

// the src headers use C11 _Static_assert; map it to the C++ keyword
#ifndef _Static_assert
#define _Static_assert static_assert
#endif
extern "C" {
#include "bitmap_session.h"
#include "map_data.h"
}

// ── identity ────────────────────────────────────────────────────────────────
struct RxId { int isStr; wchar_t str[96]; WORD id; };
LPCWSTR rx_id_arg(const RxId *r);

enum RxKind {
    RK_UNKNOWN = 0,
    RK_SPRITE,      // Lizsoft compressed container (0x2711 self-rebasing header)
    RK_IMAGE_RAW,   // engine raw path: [hdr_off, w, h] + bpp@0x0e
    RK_BMP,         // plain BITMAPFILEHEADER blob
    RK_MAP,         // MSD_SOTES_MAPDATA
    RK_WAV,         // RIFF/WAVE (PCM or compressed tag)
    RK_WMA,         // ASF/WMA stream (BGM)
    RK_STRINGS,     // RT_STRING table
    RK_VERSION,     // RT_VERSION
    RK__COUNT
};
const char *rx_kind_name(RxKind k);

struct RxWave {
    int   ok;
    WORD  tag, ch, bits;
    DWORD sr, byterate, datasz, dataoff;   // dataoff = offset of PCM bytes in blob
    double dur;
};
const char *rx_wave_tag_name(WORD t);

struct RxEntry {
    int    module_idx;
    RxId   type, name;
    DWORD  size;
    RxKind kind;
    // quick classification facts (valid per kind)
    int    img_w, img_h, img_bpp;   // sprite/image/bmp
    RxWave wave;                    // wav
    char   type_label[32];
    char   id_label[48];
    char   info[96];                // table Info column
};

struct RxModule {
    HMODULE mod;
    wchar_t path[MAX_PATH];
    char    label[64];              // "sotesd.dll"
    int     first_entry, n_entries;
};

// ── the loaded world ────────────────────────────────────────────────────────
struct RxWorld {
    std::vector<RxModule> modules;
    std::vector<RxEntry>  entries;
};

// Open one PE (dll or exe) as a datafile, enumerate + classify every resource.
// Returns module index or -1.
int rx_open_module(RxWorld *w, const wchar_t *path);
// Auto-detect a game install (Steam Fortune Summoners / Steam sotes / lizsoft),
// load every sotes*.exe / sotes*.dll found. Returns #modules loaded; outdir
// (optional, MAX_PATH) gets the folder used.
int rx_load_game_install(RxWorld *w, const wchar_t *forced_dir, wchar_t *outdir);
void rx_close_all(RxWorld *w);

// Lock an entry's bytes (valid while the module stays loaded).
const BYTE *rx_lock(const RxWorld *w, const RxEntry *e, DWORD *outsz);

// ── decoded image ───────────────────────────────────────────────────────────
struct RxImage {
    int w = 0, h = 0, bpp = 0;
    int has_palette = 0;
    int default_flip = 0;                 // 1 = memory rows are bottom-up
    std::vector<uint32_t> rgba;           // memory-row order, no colorkey applied
    std::vector<uint8_t>  raw;            // 8bpp indices / 24bpp BGR (packed stride)
    uint32_t palette[256] = {0};          // RGBA (A=255)
};
// Decode a sprite/image/bmp entry. Returns 0 on failure.
int rx_decode_image(const RxWorld *w, const RxEntry *e, RxImage *out);
// Apply colorkey(+flip) → display buffer (same w*h). Magenta 0xff00ff keys out
// (engine quirk #47: all keying is normalized magenta).
void rx_image_display(const RxImage *img, int key_on, int flip_on,
                      std::vector<uint32_t> *out);

// ── map ─────────────────────────────────────────────────────────────────────
// Parse an RK_MAP entry (caller owns/frees md via map_data_free).
int rx_parse_map(const RxWorld *w, const RxEntry *e, map_data *md);
// Object facts read out of a layer header (see docs/proofs/map-object-layer-format.md).
struct RxMapObject { uint32_t id, x, y, type, subtype; };
RxMapObject rx_map_object(const map_layer *l);
const char *rx_map_object_category(uint32_t type);  // EFFECT/STRUCTURE/CHARACTER/DEVICE/?

// ── strings ─────────────────────────────────────────────────────────────────
// Decode an RT_STRING block into UTF-8 lines "id<TAB>text\n".
int rx_decode_strings(const RxWorld *w, const RxEntry *e, std::vector<char> *utf8);

// ── waveform peaks (PCM only) ───────────────────────────────────────────────
// columns pairs of (min,max) in [-1,1]; returns 0 if not PCM 8/16-bit.
int rx_wave_peaks(const BYTE *blob, const RxWave *wv, int columns,
                  std::vector<float> *minmax);

// ── export ──────────────────────────────────────────────────────────────────
int rx_export_raw(const RxWorld *w, const RxEntry *e, const wchar_t *path);
int rx_export_png(const RxImage *img, int key_on, int flip_on, const wchar_t *path);
// Per-frame PNGs on a cell grid; names <stem>_fNNN.png. Returns #written.
int rx_export_frames(const RxImage *img, int key_on, int flip_on,
                     int cell_w, int cell_h, const wchar_t *stem_path);
int rx_export_map_json(const RxWorld *w, const RxEntry *e, const wchar_t *path);
int rx_export_strings_txt(const RxWorld *w, const RxEntry *e, const wchar_t *path);
// Bulk: export every entry in `sel` to dir with per-kind default format +
// manifest.txt. Returns #files written.
int rx_export_bulk(const RxWorld *w, const std::vector<int> &sel,
                   const wchar_t *dir);
// The bulk/default extension for a kind ("png", "wav", ...).
const char *rx_default_ext(const RxEntry *e);

// ── headless CLI (voice_view compat) ────────────────────────────────────────
int rx_cli_list(const wchar_t *dll, const wchar_t *out);
int rx_cli_dump(const wchar_t *dll, const wchar_t *type, const wchar_t *id,
                const wchar_t *out);
// like --dump but in the kind-appropriate format (png/json/txt/raw).
int rx_cli_export(const wchar_t *dll, const wchar_t *type, const wchar_t *id,
                  const wchar_t *out);

// utf16 → utf8 helper (returns bytes written excl. NUL).
int rx_utf8(const wchar_t *ws, int wn, char *out, int cap);
