/* input_dik.c — FUN_0046a880.  See input_dik.h.
 *
 * Retail spells the ~60 tests out one after another (8.9 KB); every test reads the same
 * pure accessors (0x5ba390 / 0x5ba3a0 / 0x5ba3d0 / 0x5ba3f0 / 0x5ba520 on the polled
 * device memory) and pushes the same record, so the order of the tables below IS the
 * order of retail's tests. */
#include "input_dik.h"

#include <stddef.h>
#include <string.h>

#include "engine_calls.h"

/* the keyboard device wrapper (0x5ba390.. read it) */
typedef struct dik_dev {
    uint8_t         _p00[0x10];
    uint32_t        n;          /* +0x10  buffered events this poll        */
    const uint32_t *ev;         /* +0x14  16 B each: offset, data, time, seq */
    uint8_t         state[256]; /* +0x18  the key states (& 0x80 = down)   */
} dik_dev;
#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(dik_dev, state) == 0x18, "keyboard state");
#endif

typedef struct { uint16_t key; uint8_t id; } dik_map;
/* the arrows: the skill-modifier mode's ids, then the standard ones */
static const dik_map ARROWS_ALT[4] = { { 200, 12 }, { 208, 13 }, { 203, 14 }, { 205, 15 } };
static const dik_map ARROWS[4]     = { { 200, 1 }, { 208, 3 }, { 203, 2 }, { 205, 4 } };
static const dik_map FIXED[] = {
    { 28, 20 }, { 59, 16 }, { 60, 21 }, { 61, 22 }, { 62, 23 }, { 63, 10 }, { 64, 11 },
    { 2, 24 }, { 3, 25 }, { 4, 26 }, { 5, 27 }, { 6, 28 }, { 7, 29 }, { 8, 30 }, { 9, 31 }, { 10, 32 }, { 11, 33 },
    { 28, 36 }, { 1, 39 }, { 14, 39 }, { 200, 35 }, { 197, 18 }, { 87, 19 }, { 66, 34 },
};
/* the bound keys: keycfg record offset (enabled +0, key +0x10) -> id */
static const struct { uint16_t cfg; uint8_t id; } BOUND[] = {
    { 0x548, 8 }, { 0x548, 37 }, { 0x548, 36 }, { 0x564, 7 }, { 0x564, 39 }, { 0x59c, 38 }, { 0x59c, 36 },
    { 0x580, 9 }, { 0x580, 17 }, { 0x5b8, 20 }, { 0x5d4, 16 }, { 0x5f0, 21 }, { 0x60c, 10 }, { 0x628, 22 },
    { 0x644, 23 }, { 0x660, 11 }, { 0x67c, 40 }, { 0x698, 41 }, { 0x6b4, 42 }, { 0x6d0, 43 }, { 0x6ec, 24 },
    { 0x708, 25 }, { 0x724, 26 }, { 0x740, 27 }, { 0x75c, 28 }, { 0x778, 29 }, { 0x794, 30 }, { 0x7b0, 31 },
    { 0x7cc, 32 }, { 0x7e8, 33 },
};

static uint32_t cfg32(uint32_t off) { uint32_t v; memcpy(&v, (const uint8_t *)**g_eng.keycfg + off, 4); return v; }
static int held(const dik_dev *d, uint32_t key) { return (d->state[(uint8_t)key] & 0x80) != 0; }

static void push(input_mgr *m, int32_t id, uint32_t ts, int32_t down)
{
    input_event *r = m->ring[INPUT_RING_LEN - 1];
    for (int k = INPUT_RING_LEN - 1; k != 0; k--) m->ring[k] = m->ring[k - 1];
    m->ring[0] = r;
    r->id = id;
    m->ring[0]->ts = ts;
    m->ring[0]->flag = down != 0;
}

void input_dik_produce(input_mgr *m, const void *kb)
{
    const dik_dev *d = kb;
    int no_alt = !(held(d, 0x38) || held(d, 0xb8));
    int alt_mode = cfg32(0x104) != 0 && cfg32(0x580) != 0 && held(d, cfg32(0x590));
    for (uint32_t i = 0; i < d->n; i++) {
        const uint32_t *e = d->ev + 4 * i;
        const dik_map *ar = alt_mode ? ARROWS_ALT : ARROWS;
        for (int k = 0; k < 4; k++)
            if (e[0] == ar[k].key) push(m, ar[k].id, e[2], (int32_t)(e[1] & 0x80));
        for (size_t k = 0; k < sizeof FIXED / sizeof FIXED[0]; k++)
            if (e[0] == FIXED[k].key) push(m, FIXED[k].id, e[2], (int32_t)(e[1] & 0x80));
        for (size_t k = 0; k < sizeof BOUND / sizeof BOUND[0]; k++)
            if (cfg32(BOUND[k].cfg) != 0 && e[0] == cfg32(BOUND[k].cfg + 0x10))
                push(m, BOUND[k].id, e[2], (int32_t)(e[1] & 0x80));
    }
    if (!alt_mode) {
        if (held(d, 0xc8)) m->axis_held[0] = 1;
        if (held(d, 0xd0)) m->axis_held[1] = 1;
        if (held(d, 0xcb)) m->axis_held[2] = 1;
        if (held(d, 0xcd)) m->axis_held[3] = 1;
    }
    if (cfg32(0x548) != 0 && held(d, cfg32(0x558))) m->axis_held[5] = 1;
    if (cfg32(0x564) != 0 && held(d, cfg32(0x574))) m->axis_held[4] = 1;
    if (cfg32(0x5b8) != 0 && held(d, cfg32(0x5c8))) m->axis_held[6] = 1;
    if (no_alt && held(d, 0x0f)) m->axis_held[6] = 1;
}
