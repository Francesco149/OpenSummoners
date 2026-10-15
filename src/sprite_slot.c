/* sprite_slot.c — the sprite slot helpers.  See sprite_slot.h.  Transcribed from the asm. */
#include "sprite_slot.h"

#include <stddef.h>
#include <string.h>

#include "engine_calls.h"

static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static int32_t ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }

static void entry_frames_free(const ar_sprite_slot *s, ar_sprite_entry *e)
{
    if (e->frames == NULL) return;
    for (uint32_t i = 0; i < s->f_38; i++) {
        void *f = ((void **)e->frames)[i];
        if (f != NULL) {
            g_eng.h5b9390(f);
            g_eng.op_delete(f);
        }
        ((void **)e->frames)[i] = NULL;
    }
    g_eng.op_delete(e->frames);
    e->frames = NULL;
}

void slot_frames_free(ar_sprite_slot *s, uint16_t k)
{
    if (s->entries == NULL) return;
    entry_frames_free(s, &s->entries[k]);
}

void slot_frames_free_all(ar_sprite_slot *s)
{
    for (uint16_t k = 0; k < s->entry_count; k++)
        if (s->entries != NULL) entry_frames_free(s, &s->entries[k]);
}

void sprite_set_free(void *sys, uint16_t idx)
{
    slot_frames_free_all(ldp(sys, 0xaacu + 4u * idx));
    uint8_t *st = ldp(sys, 0x18e0u + 4u * idx);
    memcpy(st + 0x10, &(int32_t){ 0 }, 4);   /* unloaded */
}

void slots_free_graded(void *sys)
{
    for (uint32_t k = 0; k < 909; k++) {
        ar_sprite_slot *s = ldp(sys, 0xaac + 4 * k);
        if (s == NULL || ld32(ldp(sys, 0x18e0 + 4 * k), 4) == 0) continue;
        slot_frames_free_all(s);
    }
}

void slot_palette_set(ar_sprite_slot *s, const uint8_t *pal)
{
    if (s->entries->b == NULL) s->entries->b = g_eng.op_new(0x400);
    memcpy(s->entries->b, pal, 0x400);
}

void pal_swap_apply(void *sys, const uint8_t *tbl, uint8_t *pal)
{
    /* the source bank's palette: its bitmap may not be 8 bpp (0x4178e0 then writes nothing and
     * retail copies stack bytes); the port's buffer starts zeroed */
    uint8_t own[1024] = { 0 };
    const uint8_t *src = pal;
    uint16_t bank;
    memcpy(&bank, tbl, 2);
    if (bank != 0) {
        g_eng.h4178e0(ldp(sys, 0xaac + 4u * bank), (intptr_t)own);
        src = own;
    }
    for (uint16_t c = 0; c < 256; c++) {
        uint8_t from = tbl[0x102 + c];
        if (from == 0) break;
        memcpy(pal + 4 * tbl[2 + c], src + 4 * from, 4);
    }
}

int sprite_slot_ready(const void *slot)
{
    return ld32(ldp(slot, 0), 0) != 0;
}

void sprite_slot_tint(void *slot, int32_t r, int32_t g, int32_t b, const uint8_t *lut)
{
    ar_sprite_slot *s = slot;
    s->f_0c = (uint32_t)r;
    s->f_10 = (uint32_t)g;
    s->f_08 = 1;
    s->f_14 = (uint32_t)b;
    stp(slot, 0x18, lut);
}

void slot_entries_free(ar_sprite_slot *s)
{
    if (s->aux_buf != NULL) {
        g_eng.op_delete(s->aux_buf);
        s->aux_buf = NULL;
    }
    if (s->entries == NULL) return;
    for (uint16_t i = 0; i < s->entry_count; i++) {
        if (s->entries[i].b != NULL) {
            g_eng.op_delete(s->entries[i].b);
            s->entries[i].b = NULL;
        }
    }
    g_eng.op_delete(s->entries);
    s->entries = NULL;
}

void *slot_clone(void *sys, uint16_t dst, uint16_t src)
{
    const ar_sprite_slot *f = ldp(sys, 0xaac + 4u * src);
    ar_sprite_slot *t = ldp(sys, 0xaac + 4u * dst);
    void *zdd = f->zdd, *settings = f->settings;
    uint32_t type = f->type, scale = f->scale_flag, h = f->height, ck = f->colorkey, w = f->width;
    uint16_t grp = f->group, res = f->resource_id;
    slot_entries_free(t);
    t->zdd = zdd;
    t->entry_count = 1;
    t->entries = g_eng.op_new(8);
    for (uint16_t i = 0; i < t->entry_count; i++) {
        t->entries[i].frames = NULL;
        t->entries[i].b = NULL;
    }
    t->settings = settings;
    t->resource_id = res;
    t->width = w;
    t->height = h;
    t->colorkey = ck;
    t->scale_flag = scale;
    t->f_08 = 0;
    t->f_18 = 0;
    t->type = type;
    t->group = grp;
    t->f_38 = 0;
    const uint8_t *aux = f->aux_buf;
    if (aux != NULL && t->aux_buf == NULL) {
        uint32_t n = f->f_38;
        t->aux_buf = g_eng.op_new(24 * n);
        for (uint32_t i = 0; i < n; i++) memcpy((uint8_t *)t->aux_buf + 24 * i, aux + 24 * i, 24);
    }
    uint8_t *d = ldp(sys, 0x18e0 + 4u * dst);
    memset(d, 0, 2);
    memset(d + 4, 0, 12);
    const uint8_t *e = ldp(sys, 0x18e0 + 4u * src);
    memcpy(ldp(sys, 0x18e0 + 4u * dst), e, 2);
    e = ldp(sys, 0x18e0 + 4u * src);
    memcpy((uint8_t *)ldp(sys, 0x18e0 + 4u * dst) + 4, e + 4, 4);
    return ldp(sys, 0x18e0 + 4u * src);
}
