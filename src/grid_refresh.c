/* grid_refresh.c — FUN_0054c640 / FUN_0054c9d0.  See grid_refresh.h.  Transcribed from the asm. */
#include "grid_refresh.h"

#include <string.h>

#include "engine_calls.h"
#include "map_grid.h"
#include "world_layout.h"

static int32_t gi32(const uint8_t *g, size_t off) { int32_t v; memcpy(&v, g + off, 4); return v; }

void grid_mark_objects(uint8_t *grid, int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t rows = h / 3200 + (y % 3200 != 0), cols = w / 3200;
    for (int32_t r = 0; r < rows; r++) {
        int32_t row = y / 3200 + r, col = x / 3200;
        for (int32_t c = cols; c > 0; c--, col++) {
            if (col >= 0 && row >= 0 && col < gi32(grid, MG_DIM0_PX) && row < gi32(grid, MG_DIM1_PX)) {
                int32_t one = 1;
                memcpy(grid + MG_REGION_B + 0xc + 16u * (uint32_t)((col << 7) + row), &one, 4);
            }
        }
    }
}

/* the cached frames at sys+0x2754: { 28-byte records, u16 count, u16 } */
typedef struct frame_rec { int32_t f[6]; void *frame; } frame_rec;
typedef struct frame_list { frame_rec *rec; uint16_t n, n2; } frame_list;

int32_t grid_refresh(uint8_t *grid, int32_t keep)
{
    if (keep == 0) {
        for (int32_t row = 0; row < gi32(grid, MG_DIM1); row++)
            for (int32_t col = 0; col < gi32(grid, MG_DIM0); col++)
                if (col >= 0 && row >= 0 && col < gi32(grid, MG_DIM0_PX) && row < gi32(grid, MG_DIM1_PX)) {
                    uint8_t *c = grid + MG_REGION_C + 12u * (uint32_t)((col << 7) + row);
                    memset(c, 0, 8);
                    memset(c + 8, 0, 2);
                }
        frame_list *l = *(frame_list **)((uint8_t *)g_eng.sys + 0x2754);
        for (uint16_t i = 0; i < l->n; i++) {
            void *fr = l->rec[i].frame;
            if (fr != NULL) {
                g_eng.h5b9390(fr);
                g_eng.op_delete(fr);
                l->rec[i].frame = NULL;
            }
            memset(l->rec[i].f, 0, sizeof l->rec[i].f);
        }
        l->n2 = 0;
    }
    for (int k = 0; k < 128; k++) {
        wl_actor *a = (wl_actor *)g_world->actors[k];
        if (a->base.alive == 0 || a->base.code != 70120) continue;
        const wl_body *b = a->base.body;
        int32_t x = b->x, y = b->y, w = b->w, h = b->h, grow = a->grow_max;
        int32_t kind = a->kind == 1 || a->kind == 3 ? 2 : a->kind == 2 ? 3 : 1;
        int32_t sub = y % 3200 / 100;
        int32_t rows = h / 3200 + (sub != 0), cols = w / 3200;
        for (int32_t r = 0; r < rows; r++)
            for (int32_t c = 0; c < cols; c++) {
                int32_t col = c + x / 3200;
                if (r != 0) {
                    map_grid_clear_cell(grid, col, r + y / 3200, 2, (uint32_t)kind, 0);
                } else if (sub != 0) {
                    map_grid_clear_cell(grid, col, y / 3200, 1, (uint32_t)kind, (uint16_t)(32 - sub));
                } else {
                    map_grid_clear_cell(grid, col, y / 3200 - 1, 1, (uint32_t)kind, 0);
                    map_grid_clear_cell(grid, col, y / 3200, 2, (uint32_t)kind, 0);
                }
            }
        grid_mark_objects(grid, x, h - grow + y, w, grow);
    }
    return 0;
}
