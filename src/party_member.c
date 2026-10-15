/* party_member.c — the party member create / attach.  See party_member.h.  Transcribed from the
 * asm (0x58e170 inlines 0x406440 for its second log line; 0x5b4a10 inlines 0x417bc0 and the
 * per-entry frame frees of 0x4118b0; 0x5b4530's pacing switch is 0x427670's). */
#include "party_member.h"

#include <string.h>

#include "char_def.h"
#include "char_init.h"
#include "char_state.h"
#include "engine_calls.h"
#include "entity_life.h"
#include "npc_ai.h"
#include "sprite_slot.h"

#define IP(p) ((intptr_t)(p))
#define W (*g_eng.world)
#define B(c) ((uint8_t *)(c))
#define TRAILER 0x8a9b6cu

static int32_t  ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void *ldp(const void *p, uint32_t off) { void *v; memcpy(&v, (const uint8_t *)p + off, sizeof v); return v; }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const char *S(uint32_t va) { return (const char *)g_eng.data(va); }
static const void *D(uint32_t va) { return g_eng.data(va); }

static void step_log(uint32_t msg)
{
    if (*g_eng.quiet == 0) g_eng.log_fail_406440(g_eng.dlog, S(msg), S(TRAILER));
}

/* the skill table row `id` (0x6826c0, 768-byte rows; the scan ends at a 0 key) */
static const uint8_t *skill_row(int32_t id)
{
    const uint8_t *t = D(0x6826c0u);
    for (uint16_t i = 0; ld32(t, 768u * i) != 0; i++)
        if (ld32(t, 768u * i) == id) return t + 768u * i;
    return NULL;
}

/* the dramatist row's code for handle `id`, else `dflt` */
static int32_t dramatist_code(int32_t id, int32_t dflt)
{
    const uint8_t *t = D(0x6b6ea8u);
    for (uint16_t i = 0; ld32(t, 52u * i) != 0; i++)
        if (ld32(t, 52u * i) == id) return ld32(t, 52u * i + 4);
    return dflt;
}

int32_t pslot_code(const wl_pslot *s)
{
    return dramatist_code(s->id, s->code);
}

uint8_t *member_skill_at(const uint8_t *st, int32_t cls, uint16_t slot)
{
    uint16_t n = ld16(st, 0x144);
    for (uint16_t i = 0; i < n; i++) {
        const uint8_t *r = st + 0x148 + 12u * i;
        if (ld32(r, 4) != cls || ld16(r, 8) != slot) continue;
        const uint8_t *e = skill_row(ld32(r, 0));
        if (e != NULL && ld32(e, 0xa8) != 0) return (uint8_t *)r;
    }
    return NULL;
}

int char_skill_usable(wl_char *c, const uint8_t *k, int32_t probing)
{
    if (k == NULL) return 0;
    const uint8_t *st = (const uint8_t *)c->party;
    int32_t cls = ld32(k, 0xc0);
    switch (cls) {
    case 0xf00000: break;
    case 0xf00001:
        if (ld16(st, 0x466) != 0) return 0;
        if (party_find(c, 50013, 0, NULL, 0) == NULL) return 0;
        break;
    case 0xf00003:
        if (ld32(st, 0xd4) != 1) return 0;
        /* fall through */
    case 0xf00002:
        if (ld16(st, 0x466) == 0) return 0;
        break;
    default:
        if (cls != ld32(st, 0xd4)) return 0;
        break;
    }
    switch (ld32(k, 0x2f8)) {
    case 1: case 2: return 1;
    case 3: return probing == 0;
    case 0xff: return ld32(c, 0x1f0) != 0;
    default: return 0;
    }
}

/* the class's slot-cursor index: 1 / 2 / 3 -> 0 / 1 / 2, else `cls` itself */
static uint16_t class_idx(const uint8_t *st, int32_t cls)
{
    switch (ld32(st, 0x140)) {
    case 1: return 0;
    case 2: return 1;
    case 3: return 2;
    default: return (uint16_t)cls;
    }
}

int char_skills_refresh(wl_char *c, int32_t cycle)
{
    for (uint16_t i = 0; i < ld16(c, 0xc754); i++) {
        uint8_t *k = B(c) + 0x754 + 768u * i;
        if (ld32(k, 0x2fc) != 0) { st32(k, 0x2f8, 1); continue; }
        st32(k, 0x2f8, 0);
        const uint8_t *st = (const uint8_t *)c->party;
        uint16_t n = ld16(st, 0x144);
        int32_t id = ld32(k, 0);
        for (uint16_t j = 0; j < n; j++)
            if (id == ld32(st, 0x148 + 12u * j)) { st32(k, 0x2f8, ld32(st, 0x14c + 12u * j)); break; }
    }
    if (cycle == 0) return 0;
    uint8_t *st = (uint8_t *)c->party;
    int32_t cls = ld32(st, 0x140);
    uint16_t n = ld16(st, 0x144);
    int found = 0;
    for (uint16_t pass = 0; pass < 2 && !found; pass++) {
        if (pass != 0) cls = cls == 1 ? 2 : cls == 2 ? 3 : cls == 3 ? 1 : cls;
        uint16_t cnt = 0;
        for (uint16_t j = 0; j < n; j++) {
            const uint8_t *r = st + 0x148 + 12u * j;
            if (ld32(r, 4) != cls) continue;
            const uint8_t *e = skill_row(ld32(r, 0));
            if (e != NULL && ld32(e, 0xa8) != 0) cnt++;
        }
        if (cnt != 0) { st32(st, 0x140, cls); found = 1; }
    }
    if (!found) st32(st, 0x140, 1);
    st = (uint8_t *)c->party;
    uint16_t cur = ld16(st, 0x138 + 2u * class_idx(st, cls));
    if (ld16(st, 0x144) != 0) {
        for (uint16_t t = 0; t < 9; t++) {
            if (t != 0) cur = (uint16_t)((cur + 1) % 10);
            const uint8_t *r = member_skill_at((const uint8_t *)c->party, ld32(c->party, 0x140), cur);
            if (r == NULL) continue;
            uint8_t *k = char_skill_find(c, ld16(r, 0));
            if (k != NULL && ld32(k, 0xa8) != 0 && char_skill_usable(c, k, 0)) {
                st = (uint8_t *)c->party;
                uint16_t ix = class_idx(st, cls);
                st16(st, 0x138 + 2u * ix, cur);
                return ix;
            }
        }
    }
    st = (uint8_t *)c->party;
    uint16_t ix = class_idx(st, cls);
    st16(st, 0x138 + 2u * ix, 0);
    return ix;
}

/* banks 884..908 are the runtime clones */
static int clone_bank(uint16_t b) { return b >= 0x374 && b <= 0x38c; }

void member_palette(wl_char *c)
{
    if (ld32(c, 0xc794) == 0) return;
    void *sys = g_eng.sys;
    uint16_t a = ld16(c, 0x48);
    int cloned = clone_bank(a);
    if (!cloned) st16(c, 0x270, a);
    uint16_t base = ld16(c, 0x270);
    if (base == 0) return;
    /* (retail: a stack buffer; 0x4178e0 writes nothing for a bitmap that is not 8 bpp) */
    uint8_t pal[1024] = { 0 };
    g_eng.h4178e0(ldp(sys, 0xaac + 4u * base), IP(pal));
    const uint8_t *slot = ldp(c, 0xc794);
    const uint8_t *room = ldp(slot, 0x9ec);
    int32_t id = ld32(slot, 0x9f0);
    int any = 0;
    for (uint16_t i = 0; i < ld16(room, 0x4010); i++) {
        const uint8_t *r = room + 16u * i;
        if (ld32(r, 0x2014) != id) continue;
        const uint8_t *item = ldp(r, 0x201c);
        if (ld32(item, 0x78) == 0) continue;
        const uint8_t *tbl = ldp(item, 0x2b4);
        if (tbl == NULL) continue;
        pal_swap_apply(sys, tbl, pal);
        any = 1;
    }
    if (!any && !cloned) return;
    for (int j = 0; j < 8; j++) {
        uint8_t *t = B(c) + 0x48 + 20u * j;
        uint16_t b = ld16(t, 0);
        if (b == 0 || t[4] == 0) continue;
        if (!clone_bank(b)) {
            for (uint16_t k = 0x374; k <= 0x38c; k++) {
                uint8_t *d = ldp(sys, 0x18e0 + 4u * k);
                if (ld32(d, 0x10) != 0) continue;
                st32(d, 0x10, 1);
                slot_clone(sys, k, b);
                st16(t, 0, k);
                t[4] = 1;
                st32(t, 0xc, 0);
                st32(t, 0x8, 0);
                st32(t, 0x10, 0);
                break;
            }
            slot_frames_free_all(ldp(sys, 0xaac + 4u * ld16(t, 0)));
        }
        uint16_t nb = ld16(t, 0);
        if (!clone_bank(nb)) continue;
        uint8_t *d = ldp(sys, 0x18e0 + 4u * nb);
        if (ldp(d, 0xc) == NULL) stp(d, 0xc, g_eng.op_new(0x400));
        memcpy(ldp(d, 0xc), pal, 1024);
        ar_sprite_slot *s = ldp(sys, 0xaac + 4u * nb);
        for (uint16_t m = 0; m < s->entry_count; m++) slot_frames_free(s, m);
    }
}

int member_attach(wl_char *c, wl_pslot *s, int32_t mode)
{
    int32_t c838 = ld32(c, 0xc838), reach = ld32(c, 0xc834);
    stp(c, 0xc794, s);
    c->party = (wl_party *)s->stats;
    char_pace_set(c, s->pace, reach, c838, ld32(c, 0xc844));
    st32(c, 0x1dc, mode);
    entity_set_handle(&c->base, s->id);
    if (s->in != NULL) {
        stp(c, 0x158a4, s->in);
        st32(c, 0x200, 0);
    } else {
        st32(c, 0x158a4, 0);
        st32(c, 0x200, 1);
    }
    char_ai_reset(c, 1);
    char_ground_probe(c);
    st32(c, 0xc84c, 0);
    st32(c, 0xc758, 1);
    st32(c, 0xc75c, c->base.body->x);
    st32(c, 0xc760, part_bottom(&c->base, 0));
    if (ld32(c, 0x1d4) == 50010) {
        if (ld32(c->party, 0xd4) == 1) {
            st32(c, 0x158a0, 2);
            char_set_look_up(c, 700, D(0x62e3c0u), D(0x62e558u), 75, 600, 0);
            st16(c, 0xe8, 1);
            stp(c, 0x15914, D(0x62def8u));
            st32(c, 0x15a80, 0);
        } else {
            st32(c, 0x158a0, 0);
            char_set_look_up(c, 500, D(0x62e3c0u), D(0x62e558u), 78, 200, 0);
            st16(c, 0xe8, 0);
            stp(c, 0x15914, D(0x62a8c8u));
            st32(c, 0x15a80, 1);
        }
    }
    st32(c, 0x158fc, 0);
    if (W->f27b4 == 2) {
        st16(c, 0x15900, 0);
        uint32_t k = (uint32_t)pslot_code(ldp(c, 0xc794));
        if (k >= 50010 && k <= 50013) char_member_pose_set(c, 107, 112, 6000);
    } else {
        st16(c, 0x15900, 40);
    }
    st32(c, 0x15904, 0);
    st32(c, 0x1bc, 1);
    member_palette(c);
    int r = char_skills_refresh(c, 0);
    st16(c, 0x1e4, 0);
    return r;
}

wl_char *member_create(wl_roomloop *rl, int32_t unused, const wl_entity *at, wl_pslot *s, int32_t facing, int32_t mode)
{
    (void)unused;
    wl_char *c = NULL;
    for (uint16_t n = 0;;) {
        wl_char *t = W->chars[rl->fcc];
        rl->fcc = (uint16_t)((rl->fcc + 1) & 0x1f);
        if (t->base.alive == 0) { c = t; break; }
        if (++n >= 32) break;
    }
    const wl_body *b = at->body;
    int32_t x = b->x, y = b->h + b->y;
    s->ch = c;
    step_log(0x8a2e9cu);   /* "Create Character Object" */
    int32_t code = dramatist_code(s->id, s->code);
    char_init(c, x, y, 3, 2, 0, code, 0, facing, s->id, 0, 0, 0, 0, 0);
    step_log(0x8a2e84u);   /* "Attach Character Object" (0x406440 inlined) */
    member_attach(c, s, mode);
    return c;
}
