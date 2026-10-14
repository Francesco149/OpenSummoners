/* particle_spawn.c — FUN_00557370.  See particle_spawn.h.  Transcribed from the asm. */
#include "particle_spawn.h"

#include <stddef.h>
#include <string.h>

#include "engine_calls.h"
#include "entity_life.h"
#include "particle_init.h"

static uint16_t *cursor(void) { return (uint16_t *)(g_world->f1044 + 0xce); }

wl_entity *particle_spawn(wl_entity *self, wl_entity *owner, int32_t code, const wl_body *b, int32_t dx, int32_t dy,
                          int32_t a7, int32_t anchor, int32_t a9, int32_t a10, int32_t a11, int32_t mirror, int32_t a13,
                          int32_t prio)
{
    if (a9 != 0) return NULL;
    uint16_t start = *cursor();
    wl_entity *p = NULL;
    for (uint16_t tries = 0; tries < 0x400; tries++) {
        uint16_t i = *cursor();
        wl_entity *q = g_world->particles[i];
        *cursor() = (uint16_t)((i + 1) & 0x3ff);
        if (q->alive == 0) { p = q; break; }
    }
    if (p == NULL) {
        int32_t best = 10;
        uint16_t i = start;
        for (int n = 0x400; n != 0; n--) {
            wl_particle *q = (wl_particle *)g_world->particles[i];
            i = (uint16_t)((i + 1) & 0x3ff);
            if (q->prio < prio && (p == NULL || best > q->prio)) {
                p = &q->base;
                best = q->prio;
            }
        }
        if (p == NULL) return NULL;
        entity_release(p, 1);
    }
    if (mirror != 0 && b->facing == 3) dx = -dx;
    int32_t x, y;
    switch (anchor) {
    case 1: x = b->x + b->w / 2 + dx; y = b->y + b->h / 2 + dy; break;
    case 2: x = b->x + dx; y = b->margin + b->y + dy; break;
    case 3: x = b->x + b->w / 2 + dx; y = b->margin + b->y + dy; break;
    case 4: x = b->x + b->w / 2 + dx; y = b->h + b->y + dy; break;
    default: x = dx; y = dy; break;
    }
    particle_init((wl_particle *)p, code, x, y, a7, (uint32_t)a10, a11, owner, a13, prio, self->faction);
    return p;
}
