/*
 * map_data.{c,h} — the in-game RUNTIME map-data parser (FUN_00587970).
 *
 * The in-game engine's per-step setup (FUN_00586010) loads the active room's
 * VISUAL map — the tilemap cell grid + a list of object/layer entries — from a
 * PE **DATA resource** keyed by the room's *scene index* (the ROOM registry's
 * dword[3]).  The opening town (map 0x3f2 -> room 210110, scene 1022) is DATA
 * resource 1022 (152,936 bytes), sourced from the **main EXE** module
 * (DAT_008a6e7c), name field "MSD_SOTES_MAPDATA".
 *
 * Load path in retail:
 *   FUN_00586010:690  FUN_00587970(module, room.scene)        the parser (here)
 *     FUN_00587970:72   FUN_005b62a0(module, scene)            the opener:
 *       FUN_005b62a0       FindResourceA(module, scene&0xffff, "DATA")
 *                          LoadResource + LockResource         -> in-mem stream
 *     FUN_00587970         FUN_005b6340(dst, n) x N            sequential copy
 *                          (mode 1: memcpy from the locked resource, advancing)
 *
 * This module ports the PARSE (the FUN_005b6340 mode-1 read sequence + the
 * operator_new layout) as pure C: the caller supplies the already-locked
 * resource bytes (FindResource/LockResource is Win32 and stays in main.c), and
 * `map_data_parse` decodes them into owned host allocations.  It is the data
 * foundation the (still unported) FUN_00587e00 map decoder + the FUN_005a00c0
 * render walk read.
 *
 * Resource format (exactly the FUN_00587970 read sequence from offset 0):
 *   [0x00:0x04]  magic    dword                       (observed 0x30)
 *   [0x04:0x34]  header   0x30 bytes (opaque)
 *   [0x34:0x68]  maphdr   0x34 bytes:
 *                  +0x00 char[0x20] name              (space-padded ASCII)
 *                  +0x20 dword dim0                   (observed 88)
 *                  +0x24 dword dim1                   (observed 19)
 *                  +0x28 dword dim2                   (observed 3)
 *                  +0x2c dword count                  (layer-entry count = 86)
 *   [0x68:..]    cells    dim0*dim1*dim2 cells, 0x1c bytes each
 *   then `count` layer entries, each:
 *                  0x3c-byte layer header, sub-array counts at
 *                    +0x1c (stride 4), +0x20 (stride 0xc),
 *                    +0x24 (stride 0x100), +0x28 (stride 8)
 *                  immediately followed by those four sub-arrays.
 * A well-formed map consumes the resource EXACTLY (verified for DATA 1022 by
 * tools/extract/map_data.py: consumed == 152936).
 *
 * PORT NOTES / fidelity boundaries:
 *   - Retail's FUN_005b6340 does NO bounds-checking (it trusts the bundled
 *     resource).  This port adds an overrun guard against `len` and returns -1
 *     rather than reading past the buffer — a host-safety hardening; on a
 *     well-formed map it is inert.
 *   - The FUN_00587970 prologue that FREES a previously-parsed map (the engine
 *     re-uses one descriptor across room loads) maps to calling
 *     map_data_free()/parse() again on the same `map_data`.
 *   - The 0x1c-byte cell record and the four layer sub-array element layouts
 *     are decoded by the (unported) FUN_00587e00; this parser preserves their
 *     raw bytes for that next unit.
 */
#ifndef OSS_MAP_DATA_H
#define OSS_MAP_DATA_H

#include <stddef.h>
#include <stdint.h>

#define MD_CELL_SIZE   0x1cu   /* bytes per tilemap cell                       */
#define MD_LAYER_HDR   0x3cu   /* bytes per layer-entry header                 */

/* One decoded tilemap cell (the 0x1c-byte record).  Field semantics are read
 * out of the (still-unported, 18 KB) consumer FUN_00587e00's per-cell decode
 * loop (587e00.c:586-601): it reads cell+0x04 as the tile-id switch key, +0x10
 * as the footprint/orientation selector, and +0x0c/+0x14/+0x18 as the placement
 * params it forwards to the (unported) tile-emit helpers 0x58c910 / 0x58ca80. In
 * the opening town (DATA 1022) +0x14/+0x18 are always 0; +0x00 is a second id
 * set on exactly the same cells as the tile-id (a co-id).  An EMPTY cell has
 * tile_id == 0. */
typedef struct map_cell {
    uint32_t f00;       /* +0x00  co-id (set together with tile_id)            */
    uint32_t tile_id;   /* +0x04  FUN_00587e00 switch key (0 == empty cell)    */
    uint32_t f08;       /* +0x08  aux selector (bank/animation)                */
    uint32_t arg_0c;    /* +0x0c  uVar23 (low u16 forwarded as a sprite index) */
    uint32_t shape;     /* +0x10  footprint/orientation selector (0..0xc)      */
    uint32_t arg_14;    /* +0x14  uVar25 placement param (0 in DATA 1022)      */
    uint32_t arg_18;    /* +0x18  uVar21 placement param (0 in DATA 1022)      */
} map_cell;

/* One layer/object entry: the 0x3c header verbatim plus its four sub-arrays,
 * sized by the header dwords at +0x1c/+0x20/+0x24/+0x28 (strides 4/0xc/0x100/8). */
typedef struct map_layer {
    uint8_t  hdr[MD_LAYER_HDR];
    uint32_t n_a, n_b, n_c, n_d;   /* counts: hdr +0x1c/+0x20/+0x24/+0x28      */
    uint8_t *a;   /* n_a * 4    bytes (hdr+0x1c)                               */
    uint8_t *b;   /* n_b * 0xc  bytes (hdr+0x20)                               */
    uint8_t *c;   /* n_c * 0x100 bytes (hdr+0x24)                             */
    uint8_t *d;   /* n_d * 8    bytes (hdr+0x28)                               */
} map_layer;

typedef struct map_data {
    uint32_t magic;            /* [0x00:0x04]                                  */
    uint8_t  header[0x30];     /* [0x04:0x34] opaque header block              */
    uint8_t  maphdr[0x34];     /* [0x34:0x68] name + dims + count              */
    uint32_t dim0, dim1, dim2; /* maphdr +0x20/+0x24/+0x28                     */
    uint32_t count;            /* maphdr +0x2c (layer-entry count)             */
    uint8_t *cells;            /* dim0*dim1*dim2 * 0x1c bytes (owned)          */
    size_t   cells_len;
    map_layer *layers;         /* `count` entries (owned)                      */
    size_t   consumed;         /* bytes read from the resource                 */
} map_data;

/* Parse `len` bytes of a locked map-data resource into `m` (zeroed first).
 * Returns 0 on success, -1 on a malformed/truncated resource or allocation
 * failure.  On success m->consumed == len for a well-formed map.  Release with
 * map_data_free. */
int  map_data_parse(map_data *m, const uint8_t *buf, size_t len);
void map_data_free(map_data *m);

/* The map name (maphdr +0x00, 0x20 bytes, space/NUL trimmed) copied into `out`
 * (size >= 0x21).  Returns `out`. */
char *map_data_name(const map_data *m, char out[0x21]);

/* The linear index of cell (x,y,z) in the cell array, using FUN_00587e00's
 * z-major linearization (587e00.c:595):
 *     idx = (dim1*z + y) * dim0 + x
 * where dim0 = width (cols), dim1 = height (rows), dim2 = planes.  No bounds
 * check (callers guard via the dims). */
size_t map_data_cell_index(const map_data *m, uint32_t x, uint32_t y, uint32_t z);

/* Decode the cell at (x,y,z) into `out`.  Returns 0 on success, -1 if (x,y,z)
 * is out of range or the map has no cell array.  An empty cell decodes to all
 * zero (out->tile_id == 0). */
int map_data_cell(const map_data *m, uint32_t x, uint32_t y, uint32_t z,
                  map_cell *out);

#endif /* OSS_MAP_DATA_H */
