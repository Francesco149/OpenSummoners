/*
 * grid_refresh.{c,h} — the map grid REFRESH FUN_0054c640 (+ FUN_0054c9d0), run by the
 * tick driver when the grid's +0x28 flag is set (retail layout; rhost-verified).
 *
 * `keep` == 0 first clears every region-C cell (retail bounds the cell INDEX by the
 * PIXEL extents, so the check never fails inside the grid) and drops the cached frames
 * of the list at sys+0x2754 (DirectDraw objects: 0x5b9390 + delete, an engine_calls slot).
 * Then every live GROWING PILLAR actor (code 70120) re-stamps its cells: region C
 * (kind by the actor's +0x28c: 1 / 3 -> 2, 2 -> 3, else 1) over its current box, the top
 * row partial (32 - (y mod 3200) / 100) or a cell above it, and region B's object word
 * (+0xc = 1) over its full grown height.  Returns 0 (the loop counter's end; the tick
 * driver reads eax's upper half).
 *
 *   FUN_0054c640  grid_refresh           FUN_0054c9d0  grid_mark_objects
 */
#ifndef OSS_GRID_REFRESH_H
#define OSS_GRID_REFRESH_H

#include <stdint.h>

int32_t grid_refresh(uint8_t *grid, int32_t keep);
void grid_mark_objects(uint8_t *grid, int32_t x, int32_t y, int32_t w, int32_t h);

#endif /* OSS_GRID_REFRESH_H */
