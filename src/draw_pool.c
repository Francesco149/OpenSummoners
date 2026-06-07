/*
 * draw_pool.c — see draw_pool.h.  Faithful port of FUN_004917b0 and the
 * 27-layer draw-node table 0x586010 builds.
 */
#include "draw_pool.h"

#include <stdlib.h>
#include <string.h>

/*
 * Per-layer capacities, transcribed verbatim from 0x586010:510-650 (the
 * `*(view->+0x54 + slot*8 + 2) = CAP` stores).  Indexed by layer = slot/8.
 * Slot 0 is only zero-initialised by the prologue loop and never assigned an
 * array, so its cap stays 0.
 */
const uint16_t draw_pool_default_caps[DRAW_POOL_LAYERS] = {
    0x000,  /*  0  (zero-init, no array)            */
    0x080,  /*  1  operator_new(0x1e00)             */
    0x1b8,  /*  2  operator_new(0x6720)             */
    0x1b8,  /*  3  operator_new(0x6720)             */
    0x080,  /*  4  operator_new(0x1e00)             */
    0x080,  /*  5  operator_new(0x1e00)             */
    0x400,  /*  6  operator_new(0xf000)             */
    0x060,  /*  7  operator_new(0x1680)             */
    0x080,  /*  8  operator_new(0x1e00)             */
    0x0e0,  /*  9  operator_new(0x3480)             */
    0x094,  /* 10  operator_new(0x22b0)             */
    0x400,  /* 11  operator_new(0xf000)             */
    0x060,  /* 12  operator_new(0x1680)             */
    0x060,  /* 13  operator_new(0x1680)             */
    0x060,  /* 14  operator_new(0x1680)             */
    0x080,  /* 15  operator_new(0x1e00)             */
    0x040,  /* 16  operator_new(0xf00)              */
    0x400,  /* 17  operator_new(0xf000)             */
    0x080,  /* 18  operator_new(0x1e00)             */
    0x1b8,  /* 19  operator_new(0x6720)             */
    0x0c8,  /* 20  operator_new(12000)              */
    0x1b8,  /* 21  operator_new(0x6720)             */
    0x060,  /* 22  operator_new(0x1680)             */
    0x060,  /* 23  operator_new(0x1680)             */
    0x040,  /* 24  operator_new(0xf00)              */
    0x400,  /* 25  operator_new(0xf000)             */
    0x400,  /* 26  operator_new(0xf000)             */
};

int draw_pool_init(draw_pool *p)
{
    memset(p, 0, sizeof(*p));
    for (unsigned i = 0; i < DRAW_POOL_LAYERS; i++) {
        uint16_t cap = draw_pool_default_caps[i];
        p->layers[i].count = 0;
        p->layers[i].cap   = cap;
        if (cap == 0) {
            p->layers[i].nodes = NULL;   /* slot 0 has no array */
            continue;
        }
        p->layers[i].nodes = calloc(cap, sizeof(draw_node));
        if (!p->layers[i].nodes) {
            draw_pool_free(p);
            return -1;
        }
    }
    return 0;
}

void draw_pool_reset(draw_pool *p)
{
    for (unsigned i = 0; i < DRAW_POOL_LAYERS; i++)
        p->layers[i].count = 0;
}

void draw_pool_free(draw_pool *p)
{
    for (unsigned i = 0; i < DRAW_POOL_LAYERS; i++)
        free(p->layers[i].nodes);
    memset(p, 0, sizeof(*p));
}

draw_node *draw_pool_emit(draw_pool *p, uint32_t layer_key, uint32_t mode,
                          uint32_t sprite, int32_t dst_x, int32_t dst_y,
                          uint32_t p6, uint32_t p7, uint32_t p8)
{
    uint32_t idx = layer_key & 0xffffu;
    if (idx >= DRAW_POOL_LAYERS)
        return NULL;

    draw_layer *L = &p->layers[idx];
    /* FUN_004917b0: `if ((ushort)cap <= count) return 0;` */
    if (L->cap <= L->count)
        return NULL;

    draw_node *n = &L->nodes[L->count];
    L->count++;

    /* The six dwords FUN_004917b0 stores (puVar2[0..6], skipping the src rect
     * the caller stamps afterward). */
    n->sprite = sprite;   /* puVar2[0] = param_3 */
    n->dst_x  = dst_x;    /* puVar2[1] = param_4 */
    n->dst_y  = dst_y;    /* puVar2[2] = param_5 */
    n->param6 = p6;       /* puVar2[3] = param_6 */
    n->param7 = p7;       /* puVar2[4] = param_7 */
    n->param8 = p8;       /* puVar2[5] = param_8 */
    n->mode   = mode;     /* puVar2[6] = param_2 */
    return n;
}

draw_node *draw_pool_emit_actor(draw_pool *p, uint32_t layer_key, uint32_t cel,
                                int32_t world_x, int32_t world_y,
                                int32_t off_x, int32_t off_y, uint32_t alpha)
{
    /* FUN_00492670:12 — `if (param_2 != 0)`: a NULL cel emits nothing. */
    if (cel == 0)
        return NULL;

    /* The node bytes 492670 stores, expressed through draw_pool_emit:
     *   node[0]=cel  [1]=world_x  [2]=world_y  [3]=off_x  [4]=off_y
     *   node[5]=alpha (param8)    node[6]=mode = (alpha != 0). */
    return draw_pool_emit(p, layer_key, (alpha != 0u) ? 1u : 0u,
                          cel, world_x, world_y,
                          (uint32_t)off_x, (uint32_t)off_y, alpha);
}
