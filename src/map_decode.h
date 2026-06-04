/*
 * map_decode.{c,h} — the per-tile-id placement DISPATCH of the in-game map
 * decoder FUN_00587e00 (the 18 KB map-data -> runtime-render-grid pass).
 *
 * After the per-room map data is parsed (map_data.c, FUN_00587970), the engine
 * walks every cell and, keyed on the cell's tile-id (+0x04) and shape selector
 * (+0x10), issues a recipe of write-primitive calls (map_grid.c) that paint the
 * runtime render grid the in-game render dispatch (0x5a00c0) later blits.  This
 * module ports that per-cell dispatch.
 *
 * SCOPE — the opening town (DATA 1022) only.  Ground truth from
 * tools/extract/map_data.py --cells: the town map uses exactly NINE tile ids,
 * all pure compositions of the ported map_grid primitives:
 *
 *   id        shapes used        what it places (587e00.c arm)
 *   0x1b58b   0, 2               an obj block + a base tile (bank 0x62)
 *   0x1b58c   0, 2               same arm as 0x1b58b
 *   0x1b58d   2                  a 6-call obj cluster + base tile (bank 0x63)
 *   0x1b58f   0,10,11,12,15      optional fg tile (bank 0x17a) + base (0x176)
 *   0x1b5a0   0                  one base tile (bank 0x17b, flag 0xa)
 *   0x1b5a9   0                  one base tile (bank 0x172)
 *   0x1b5aa   0                  one base tile (bank 0x173, z-dependent slot)
 *   0x1b5ab   0                  a tile (bank 0x174) + a 2x9 obj block
 *   0x29ff4   0, 14              optional fg tile (bank 0x17a) + base (0x177)
 *
 * Every OTHER tile id in FUN_00587e00 (the 0x1bd82 autotile block, the
 * 0x1d8ab/0x1ffbc decoration switches, the HUD/border families, the inline
 * grid-write arms that read engine globals DAT_008a76xx/DAT_008a7bfc) is DEAD
 * CODE for this map and is NOT ported here (ckpt-57 scoping win).  A cell with
 * an unhandled id is a no-op (only its region-E co-id slot is cleared, matching
 * the retail fall-through to switchD_005887cc_caseD_271d).
 *
 * NOT ported (the engine-coupled body of the rock, deferred per the
 * ckpt-53/56/57 discipline):
 *   - the 0x587e00 PROLOGUE (587e00.c:34-571): the front-header flag writes, the
 *     HUD/border sprite-bank selection over the DAT_008a76xx pool, and the
 *     0x1bd82 autotile pre-pass.  map_decode performs only the prologue's four
 *     dim-header writes (via map_grid_set_dims) so the primitives' bounds checks
 *     behave; the rest of the front header stays zero.
 *   - the trailing LAYER pass (587e00.c:3185-3204): the 0x58c8c0 / 0x58c8d0 /
 *     0x58cb30 helpers over the 0x15f9a/0x15f9b object entries.
 *
 * PORT NOTES / fidelity boundaries:
 *   - The two emit_obj arms of 0x1b58d store an engine .rdata pointer
 *     (&DAT_005cc410 / &DAT_005cc430, a blend descriptor) into region B's +0x8
 *     dword.  Retail writes the absolute address there; the port writes the same
 *     VA verbatim (MD_BLEND_5cc410 / MD_BLEND_5cc430) so the grid bytes match.
 *     The render port will translate these when it consumes region B.
 *   - The bank-derived tile footprint (map_grid_emit_tile with no explicit span)
 *     needs the sprite-bank pixel-size pool, an engine global; the caller passes
 *     it through the mg_bank_dims_fn callback (NULL -> degenerate 0x0 footprint).
 */
#ifndef OSS_MAP_DECODE_H
#define OSS_MAP_DECODE_H

#include "map_data.h"
#include "map_grid.h"

/* Engine .rdata blend-descriptor addresses written verbatim into region B +0x8
 * by the 0x1b58d arm (587e00.c:2317.. / 2328..).  Preserved as the retail VA. */
#define MD_BLEND_5cc410  0x005cc410u
#define MD_BLEND_5cc430  0x005cc430u

/* Dispatch one cell at (x,y,z): read its record from `m` and issue the
 * map_grid_* recipe for its tile id (no-op for an empty / unhandled cell).
 * Does NOT clear the cell's region-E slot (map_decode does that per the loop).
 * `dims`/`ctx` supply sprite-bank pixel sizes for bank-derived footprints. */
void map_decode_cell(const map_data *m, uint8_t *grid,
                     uint32_t x, uint32_t y, uint32_t z,
                     mg_bank_dims_fn dims, void *ctx);

/* Decode a whole parsed map into a runtime render grid (FUN_00587e00's body
 * minus the deferred prologue + layer pass): write the dim header, pre-clear
 * region C over dim0 x dim1, then for every cell (z-major) run map_decode_cell
 * and zero the cell's region-E co-id slot.  `grid` must be MG_GRID_BYTES (e.g.
 * from map_grid_alloc) and is assumed zeroed. */
void map_decode(const map_data *m, uint8_t *grid,
                mg_bank_dims_fn dims, void *ctx);

#endif /* OSS_MAP_DECODE_H */
