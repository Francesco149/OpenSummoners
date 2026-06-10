/* map_grid.c — runtime render-grid write primitives.  See map_grid.h. */
#include "map_grid.h"

#include <stdlib.h>
#include <string.h>

/* Typed views into the flat grid at an absolute byte offset. */
static inline int32_t  *mg_i32(uint8_t *g, size_t off) { return (int32_t *)(g + off); }
static inline uint32_t *mg_u32(uint8_t *g, size_t off) { return (uint32_t *)(g + off); }
static inline uint16_t *mg_u16(uint8_t *g, size_t off) { return (uint16_t *)(g + off); }

uint8_t *map_grid_alloc(void)
{
    return (uint8_t *)calloc(MG_GRID_BYTES, 1);
}

void map_grid_free(uint8_t *grid)
{
    free(grid);
}

/* 0x587e00 prologue, 587e00.c:44-56 (the four dim-header writes only). */
void map_grid_set_dims(uint8_t *grid, int32_t dim0, int32_t dim1)
{
    *mg_i32(grid, MG_DIM0)    = dim0;
    *mg_i32(grid, MG_DIM1)    = dim1;
    *mg_i32(grid, MG_DIM0_PX) = dim0 * (int32_t)MG_PX_PER_DIM;
    *mg_i32(grid, MG_DIM1_PX) = dim1 * (int32_t)MG_PX_PER_DIM;
}

/* FUN_0054c970 @ 0x54c970.  Bounds-checked against the *pixel* dims. */
void map_grid_clear_cell(uint8_t *grid, int32_t p1, int32_t p2,
                         uint32_t v0, uint32_t v4, uint16_t v8)
{
    if (p1 >= 0 && p2 >= 0 &&
        p1 < *mg_i32(grid, MG_DIM0_PX) &&
        p2 < *mg_i32(grid, MG_DIM1_PX)) {
        int32_t idx = p1 * (int32_t)MG_ROW_PITCH + p2;
        size_t base = MG_REGION_C + (size_t)idx * 0xc;
        *mg_u32(grid, base + 0x0) = v0;
        *mg_u32(grid, base + 0x4) = v4;
        *mg_u16(grid, base + 0x8) = v8;
    }
}

/* FUN_0058ca80 @ 0x58ca80.  Fill a p3(rows) x p4(cols) block from cell (p1,p2). */
void map_grid_emit_obj(uint8_t *grid, int32_t p1, int32_t p2,
                       int32_t p3, int32_t p4,
                       uint16_t a, uint16_t b,
                       uint32_t d8, uint32_t d4, uint32_t dc)
{
    int32_t col_end = p4 + p2;
    int32_t col;
    for (col = p2; col < col_end; col++) {
        if (p1 >= p3 + p1)            /* p3 <= 0: empty row span */
            continue;
        int32_t r;
        for (r = p1; r < p3 + p1; r++) {
            if (r >= 0 && r < *mg_i32(grid, MG_DIM0) &&
                col >= 0 && col < *mg_i32(grid, MG_DIM1)) {
                int32_t idx  = r * (int32_t)MG_ROW_PITCH + col;
                size_t  recB = MG_REGION_B + (size_t)idx * 0x10;
                *mg_u16(grid, recB + 0x0) = a;
                *mg_u32(grid, recB + 0x4) = d4;
                *mg_u32(grid, recB + 0x8) = d8;
                *mg_u32(grid, recB + 0xc) = dc;
                *mg_u16(grid, MG_REGION_D + (size_t)idx * 2) = b;
            }
        }
    }
}

/* ceil(size / 32), matching 587e00/58c910's signed-safe expression:
 *   iVar = size + 0x1f ;  (iVar + ((iVar >> 31) & 0x1f)) >> 5 */
static inline int32_t mg_tiles32(int32_t size)
{
    int32_t v = size + 0x1f;
    return (int32_t)(v + ((v >> 31) & 0x1f)) >> 5;
}

/* FUN_0058c910 @ 0x58c910. */
void map_grid_emit_tile(uint8_t *grid, int32_t p1, int32_t p2, int32_t slot,
                        uint32_t bank, uint16_t a2, int16_t flag,
                        int32_t span_rows, int32_t span_cols,
                        mg_bank_dims_fn dims, void *ctx)
{
    if (flag == 0) {
        switch (slot) {
        case 0:
        case 1: flag = 2;    break;
        case 2: flag = 0x14; break;
        case 3: flag = 0x15; break;
        default: /* unreachable for valid slots; retail reads UB here */ break;
        }
    }

    int32_t n_rows;   /* iVar4: extent along the p1 (row) axis */
    int32_t n_cols;   /* iVar1: extent along the p2 (col) axis */
    if (span_rows == 0 && span_cols == 0) {
        if ((bank & 0xffff) == 0) {
            n_rows = 1;
            n_cols = 1;
        } else {
            int32_t w = 0, h = 0;
            if (dims)
                dims(ctx, (uint16_t)bank, &w, &h);
            n_rows = mg_tiles32(w);
            int32_t rem = *mg_i32(grid, MG_DIM0) - p1;
            if (n_rows <= rem) rem = n_rows;
            n_rows = rem;

            n_cols = mg_tiles32(h);
            int32_t remc = *mg_i32(grid, MG_DIM1) - p2;
            if (remc < n_cols) n_cols = remc;
        }
    } else {
        n_rows = *mg_i32(grid, MG_DIM0) - p1;
        if (span_rows <= n_rows) n_rows = span_rows;
        n_cols = *mg_i32(grid, MG_DIM1) - p2;
        if (span_cols <= n_cols) n_cols = span_cols;
    }

    for (int32_t dy = 0; dy < n_cols; dy++) {
        if (n_rows <= 0)
            continue;
        for (int32_t dx = 0; dx < n_rows; dx++) {
            if (p2 + dy >= 0 && p1 + dx >= 0) {
                int32_t idx  = (p1 + dx) * (int32_t)MG_ROW_PITCH + (p2 + dy);
                size_t  base = MG_REGION_A + (size_t)slot * 0x10 + (size_t)idx * 0x40;
                *mg_u16(grid, base + 0x0) = (uint16_t)bank;
                *mg_u16(grid, base + 0x2) = a2;
                *mg_u16(grid, base + 0x4) = (uint16_t)flag;
                *mg_i32(grid, base + 0x8) = dx;
                *mg_i32(grid, base + 0xc) = dy;
            }
        }
    }
}

/* ── READ accessors (collision read-side) ─────────────────────────────────────
 * idx = col*0x80 + row; the engine indexes region B as `(col*0x80 + 0x14003 +
 * row)*0x10` (54e990.c:65, a ushort* base = byte 0x140030 + idx*0x10) and
 * region D as `0x2c1040 + idx*2` (441ae0.c:47).  No bounds checking — see the
 * header note. */
const uint8_t *map_grid_obj_record(const uint8_t *grid, int32_t col, int32_t row)
{
    int32_t idx = col * (int32_t)MG_ROW_PITCH + row;
    return grid + MG_REGION_B + (size_t)idx * 0x10;
}

uint16_t map_grid_obj_class(const uint8_t *grid, int32_t col, int32_t row)
{
    uint16_t v; memcpy(&v, map_grid_obj_record(grid, col, row) + 0x0, 2); return v;
}

uint32_t map_grid_obj_slope(const uint8_t *grid, int32_t col, int32_t row)
{
    uint32_t v; memcpy(&v, map_grid_obj_record(grid, col, row) + 0x8, 4); return v;
}

int16_t map_grid_flag(const uint8_t *grid, int32_t col, int32_t row)
{
    int32_t idx = col * (int32_t)MG_ROW_PITCH + row;
    int16_t v; memcpy(&v, grid + MG_REGION_D + (size_t)idx * 2, 2); return v;
}
