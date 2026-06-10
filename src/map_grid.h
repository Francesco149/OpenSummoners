/*
 * map_grid.{c,h} — the in-game RUNTIME render-grid WRITE PRIMITIVES.
 *
 * After the per-room map data is parsed (map_data.{c,h}, FUN_00587970), the
 * engine decodes it into a large flat **runtime render grid** — the buffer the
 * in-game render dispatch (0x5a00c0) later walks to blit the room.  That
 * decode is 0x587e00 (18 KB, a per-tile-id placement dispatch, still
 * unported); this module ports the three small PURE helpers 0x587e00 calls
 * to *write* the grid:
 *
 *   FUN_0054c970  (84 B)   map_grid_clear_cell  — region C, the 0xc-B/cell entry
 *   FUN_0058ca80  (167 B)  map_grid_emit_obj    — region B (0x10/cell) + D (2/cell)
 *   FUN_0058c910  (347 B)  map_grid_emit_tile   — region A, the 4-layer 0x40/cell grid
 *
 * THE RUNTIME GRID OBJECT (0x587e00's `in_ECX`, a flat engine-owned buffer).
 * The grid is addressed by a CELL INDEX with a fixed row pitch of 0x80 (128)
 * cells regardless of the map's actual width:
 *
 *     idx = p1 * 0x80 + p2          (p1 = the dim0/0x2c1030 axis, p2 = dim1)
 *
 * Note this is NOT the map_data z-major cell linearization — the runtime grid is
 * a separate 2-D [0x80-pitch] addressing.  The buffer holds, at fixed byte
 * offsets, a small front header plus four parallel per-cell regions:
 *
 *   byte 0x000030  region A  0x40 B/cell = 4 sub-slots x 0x10 B   (emit_tile)
 *                     sub-slot s of cell c at 0x30 + s*0x10 + c*0x40:
 *                       +0x0 u16 bank id   +0x2 u16   +0x4 u16
 *                       +0x8 i32 dx        +0xc i32 dy
 *   byte 0x140030  region B  0x10 B/cell                           (emit_obj)
 *                       +0x0 u16   +0x4 dword  +0x8 dword  +0xc dword
 *   byte 0x195030  region C  0x0c B/cell                           (clear_cell)
 *                       +0x0 dword  +0x4 dword  +0x8 u16
 *   byte 0x2c1030  i32 dim0           (cols; copied from map descriptor +0x20)
 *   byte 0x2c1034  i32 dim1           (rows; map descriptor +0x24)
 *   byte 0x2c1038  i32 dim0 * 0xc80   (pixel extent along dim0)
 *   byte 0x2c103c  i32 dim1 * 0xc80   (pixel extent along dim1)
 *   byte 0x2c1040  region D  2 B/cell                              (emit_obj)
 *
 * The dim header (0x2c1030..0x2c103c) is written by the 0x587e00 prologue
 * (587e00.c:44-56) and read by all three primitives for their bounds checks;
 * `map_grid_set_dims` ports exactly those four writes so the primitives can be
 * exercised standalone.  Everything else the 18 KB prologue does (the HUD/border
 * sprite-bank selection over the DAT_008a76xx pool, the front header flags) is
 * NOT ported here — it is the engine-coupled body of the rock.
 *
 * PORT NOTES / fidelity boundaries:
 *   - These are faithful byte-for-byte ports of the decompiled arithmetic
 *     (587e00.c references the same offsets via a ushort* base, e.g. in_ECX
 *     index 0x160818 == byte 0x2c1030).  The host buffer is a raw uint8_t* of
 *     MG_GRID_BYTES; offsets are absolute, matching retail.
 *   - Retail does NO buffer-bounds checking (it trusts its own allocation); the
 *     primitives' own (-1 <= coord < dim) guards are ported verbatim, and the
 *     host buffer is sized to cover every region, so no read/write escapes it.
 *   - FUN_0058c910's `param_6 == 0` arm reads an UNINITIALISED local for a
 *     param_3 outside {0,1,2,3} (587e00.c never calls it that way); the port
 *     leaves param_6 = 0 in that unreachable case rather than reproduce UB.
 *   - FUN_0058c910 derives a tile span from a sprite bank's pixel size
 *     (pool[bank]+0x20 width / +0x24 height, rounded up to 32-px tiles).  The
 *     bank pool (&DAT_008a760c) is an engine global; the port takes a
 *     `mg_bank_dims_fn` callback so it stays pure.  A NULL callback yields a
 *     0x0 bank (the span clamps to 0 → no writes), which is the safe degenerate.
 */
#ifndef OSS_MAP_GRID_H
#define OSS_MAP_GRID_H

#include <stddef.h>
#include <stdint.h>

/* Byte offsets of the runtime-grid regions / header (see the table above). */
#define MG_REGION_A    0x000030u   /* tile grid, 0x40 B/cell (4 sub-slots)     */
#define MG_REGION_B    0x140030u   /* object records, 0x10 B/cell              */
#define MG_REGION_C    0x195030u   /* per-cell entry, 0x0c B/cell              */
#define MG_DIM0        0x2c1030u   /* i32 cols                                 */
#define MG_DIM1        0x2c1034u   /* i32 rows                                 */
#define MG_DIM0_PX     0x2c1038u   /* i32 dim0 * 0xc80                         */
#define MG_DIM1_PX     0x2c103cu   /* i32 dim1 * 0xc80                         */
#define MG_REGION_D    0x2c1040u   /* 2 B/cell                                 */

/* Region E — a per-cell "co-id" record (0x30 B/cell) the 0x587e00 decode loop
 * zeroes at the end of each cell pass (587e00.c:3175, `in_ECX[(x*0x80 + 0x9b01
 * + y) * 0x18] = 0` with in_ECX a ushort*, i.e. byte 0x1d1030 + idx*0x30 +0x0).
 * The town dispatch arms never write it; map_decode only clears it.  */
#define MG_REGION_E    0x1d1030u   /* 0x30 B/cell; loop zeroes the +0x0 u16     */
#define MG_REGION_E_STRIDE 0x30u

#define MG_ROW_PITCH   0x80u       /* cells per row in the runtime grid        */
#define MG_PX_PER_DIM  0xc80u      /* pixel extent multiplier (587e00.c:53)    */

/* Buffer size: large enough for every region written by these primitives (the
 * highest is region D, max byte ~0x2c6766 for an 88x19 map) plus the
 * 0x587e00 prologue's front-header flags (~0x2cb044).  The real engine
 * buffer's exact size is fixed by its (unported) allocator. */
#define MG_GRID_BYTES  0x2cc000u

/* Looks up sprite bank `bank_id`'s pixel dimensions (the engine pool entry's
 * +0x20 width / +0x24 height).  Called by map_grid_emit_tile only when the
 * caller passes no explicit span (p9 == p10 == 0) and bank_id != 0. */
typedef void (*mg_bank_dims_fn)(void *ctx, uint16_t bank_id,
                                int32_t *out_w, int32_t *out_h);

/* Allocate / free a zeroed runtime-grid buffer (MG_GRID_BYTES).  Returns NULL
 * on allocation failure. */
uint8_t *map_grid_alloc(void);
void     map_grid_free(uint8_t *grid);

/* Write the four dim-header dwords (0x587e00 prologue, 587e00.c:44-56):
 *   MG_DIM0 = dim0, MG_DIM1 = dim1, MG_DIM0_PX = dim0*0xc80, MG_DIM1_PX = dim1*0xc80.
 * Required before the primitives' bounds checks behave. */
void map_grid_set_dims(uint8_t *grid, int32_t dim0, int32_t dim1);

/* FUN_0054c970 — write region C's 0xc-byte entry for cell (p1,p2), guarded by
 * the *pixel* dims (MG_DIM0_PX / MG_DIM1_PX).  No-op if out of range. */
void map_grid_clear_cell(uint8_t *grid, int32_t p1, int32_t p2,
                         uint32_t v0, uint32_t v4, uint16_t v8);

/* FUN_0058ca80 — fill a p3 x p4 (rows x cols) block starting at cell (p1,p2)
 * into region B (the 0x10-byte record: u16 a, dword d4, dword d8, dword dc) and
 * region D (the 2-byte grid value b).  Each cell guarded by the tile dims
 * (MG_DIM0 / MG_DIM1). */
void map_grid_emit_obj(uint8_t *grid, int32_t p1, int32_t p2,
                       int32_t p3, int32_t p4,
                       uint16_t a, uint16_t b,
                       uint32_t d8, uint32_t d4, uint32_t dc);

/* FUN_0058c910 — place a tile (sprite bank `bank`) at cell (p1,p2), sub-slot
 * `slot` (0..3) of region A.  The footprint is either the explicit span
 * (span_rows, span_cols) when nonzero, or derived from the bank's pixel size
 * via `dims` (rounded up to 32-px tiles) and clamped to the grid.  `a2` is the
 * u16 written at sub-slot +0x2; `flag` (the +0x4 field) defaults from `slot`
 * when 0. */
void map_grid_emit_tile(uint8_t *grid, int32_t p1, int32_t p2, int32_t slot,
                        uint32_t bank, uint16_t a2, int16_t flag,
                        int32_t span_rows, int32_t span_cols,
                        mg_bank_dims_fn dims, void *ctx);

/* ── READ accessors over a BUILT grid (the collision read-side, chip 2) ───────
 * The collision probes (0x441ae0/0x47dbb0) and the tile-mover (0x54e990) index
 * the grid by CELL: idx = col*0x80 + row, where col = worldX/0xc80 (the DIM0 /
 * 0x80-pitch axis) and row = worldY/0xc80 (DIM1).  The cell is 0xc80 = 3200
 * world units (MG_PX_PER_DIM).  These accessors do NO bounds checking — the
 * engine trusts its own dim-clamp before indexing, and the callers (which port
 * that clamp verbatim) pass already-clamped col/row; the MG_GRID_BYTES buffer
 * covers every in-range cell.  Region B carries the per-cell collision CLASS
 * (u16 @+0x0: 1 = slope-test, 10 = solid wall) + the slope-profile reference
 * (u32 @+0x8: an engine .rdata VA — 0 = flat); region D carries the 2-byte flag
 * the directional probes test (b == 1 = wall). */
const uint8_t *map_grid_obj_record(const uint8_t *grid, int32_t col, int32_t row);
uint16_t       map_grid_obj_class (const uint8_t *grid, int32_t col, int32_t row);
uint32_t       map_grid_obj_slope (const uint8_t *grid, int32_t col, int32_t row);
int16_t        map_grid_flag      (const uint8_t *grid, int32_t col, int32_t row);

#endif /* OSS_MAP_GRID_H */
