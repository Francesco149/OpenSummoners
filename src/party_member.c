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
#include "room_load.h"
#include "room_loop.h"
#include "scene_loop.h"
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
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

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

/* ---- the party API ---- */

#define ITEM_TBL 0x6c0cd0u   /* 736-byte item rows: +0 id, u16 +0x64 the default count, +0x78 the equip
                              * slot, +0x7c the class mask kind, +0x2ac the category (< 900: gear),
                              * +0x2b8 the starting owner, +0x2bc granted at start */
#define SLOT(r, i) ((uint8_t *)(r)->slot[i])

static void stats_fill1000(uint8_t *st) { for (int i = 0; i < 12; i++) st32(st, 0xa4 + 4u * i, 1000); }

/* the item row `id` (the scan ends at a 0 key) */
static const uint8_t *item_row(int32_t id)
{
    const uint8_t *t = D(ITEM_TBL);
    for (uint16_t i = 0; ld32(t, 736u * i) != 0; i++)
        if (ld32(t, 736u * i) == id) return t + 736u * i;
    return NULL;
}

/* an item record of the room's inventory (+0x2010, 16 B: id, owner, u16 count, u8 flag, -, def) */
#define ITEM(r, k) ((uint8_t *)(r) + 0x2010 + 16u * (k))

wl_pslot *pslot_by_id(wl_room *r, int32_t id, int32_t passive)
{
    for (int i = 0; i < 8; i++) {
        wl_pslot *s = r->slot[i];
        if (s->id != id) continue;
        if (s->active != 1 && (s->active != 2 || passive == 0)) return NULL;
        return s;
    }
    return NULL;
}

const uint8_t *dramatist_row(int32_t id)
{
    const uint8_t *t = D(0x6b6ea8u);
    for (uint16_t i = 0; ld32(t, 52u * i) != 0; i++)
        if (ld32(t, 52u * i) == id) return t + 52u * i;
    return NULL;
}

const uint8_t *skill_row_find(int32_t id) { return skill_row(id); }

const int32_t *area_level_row(int32_t key)
{
    const int32_t *t = D(0x690b40u);   /* 16-byte rows: key, level, minimum, gold cap; a negative key ends it */
    if (key == 0) return t;
    for (uint16_t i = 1;; i++) {
        if (t[4u * i] == key) return t + 4u * i;
        if (t[4u * i] < 0) return t + 4u * (i - 1u);
    }
}

void pslot_heal(wl_pslot *s, int32_t full)
{
    uint8_t *p = (uint8_t *)s;
    if (full != 0) {
        st32(p, 0xa64, ld32(p, 0xa68) + ld32(p, 0xa94) + ld32(p, 0xaac));
        st32(p, 0xa6c, ld32(p, 0xa70) + ld32(p, 0xa98) + ld32(p, 0xab0));
    } else {
        st32(p, 0xa64, (ld32(p, 0xa68) + ld32(p, 0xa94) + ld32(p, 0xaac)) / 2);
    }
    memset(p + 0xe58, 0, 0x24);
}

/* one member up to the area's level (the active ones play the level-up cue unless `quiet`) */
static void member_level(wl_pslot *s, const int32_t *row, int32_t quiet, int active)
{
    uint8_t *p = (uint8_t *)s;
    int32_t target = row[1], minv = row[2];
    if (target > ld32(p, 0xaf0)) {
        for (int32_t j = target - ld32(p, 0xaf0); j > 0; j--) {
            int32_t x = ld32(p, 0xae8);
            if (x > 0) {
                st32(p, 0xae8, x - 1);
                int32_t t = ld32(p, 0xafc) - 1;
                if (t >= ld32(p, 0xaf8)) t = ld32(p, 0xaf8);
                st32(p, 0xaf8, t);
                st32(p, 0xaf0, ld32(p, 0xaf0) + 1);
                continue;
            }
            int32_t code;
            if (active) {
                if (quiet == 0) {
                    if (ld32(p, 8) == 0) {
                        st32(p, 8, 1);
                        memcpy(p + 0xc, p + 0xa10, 0x4dc);
                    }
                    g_eng.h55fdb0(s);
                }
                st32(p, 0xaf8, 0);
                st32(p, 0xaf0, ld32(p, 0xaf0) + 1);
                const uint8_t *d = dramatist_row(s->id);
                code = d != NULL ? ld32(d, 4) : s->code;
            } else {
                st32(p, 0xaf8, 0);
                st32(p, 0xaf0, ld32(p, 0xaf0) + 1);
                code = pslot_code(s);
            }
            const uint8_t *sr = stat_row_find(code, ld32(p, 0xaf0) + ld32(p, 0xae8));
            if (sr != NULL) g_eng.h409150(s, IP(sr));
            memcpy(p + 0x4e8, p + 0xa10, 0x4dc);
        }
    }
    if (minv > ld32(p, 0xaec)) st32(p, 0xaec, minv);
}

void room_party_level(wl_room *r, int32_t quiet)
{
    const int32_t *row = area_level_row(ld32(r, 0x401c));
    st32(r, 0x4020, row[3]);
    for (int i = 0; i < 8; i++) {
        wl_pslot *s = r->slot[i];
        if (s->active == 1 && s->id != 100000104) member_level(s, row, quiet, 1);
        else if (s->active == 2) member_level(s, row, quiet, 0);
    }
}

void member_skills_learn(wl_pslot *s, int32_t code, int32_t level)
{
    uint8_t *st = s->stats;
    const uint8_t *t = D(0x6907c0u);   /* 16-byte rows: code, skill, level, prerequisite */
    for (uint16_t ri = 0; ld32(t, 16u * ri) != 0; ri++) {
        const uint8_t *row = t + 16u * ri;
        if (ld32(row, 0) != code) continue;
        uint16_t n = ld16(st, 0x144);
        uint16_t k;
        for (k = 0; k < n && ld32(st, 0x148 + 12u * k) != ld32(row, 4); k++) {}
        if (k < n) continue;                         /* known */
        if (ld32(row, 0xc) != 0) {                   /* the prerequisite must be known */
            for (k = 0; k < n && ld32(st, 0x148 + 12u * k) != ld32(row, 0xc); k++) {}
            if (k >= n) continue;
        }
        int32_t lv = ld32(row, 8);
        if (lv > level || lv == 0) continue;
        int32_t skill = ld32(row, 4), cls = 255;
        uint16_t slot = 0;
        const uint8_t *e = skill_row(skill);
        if (e != NULL) {
            if (ld32(e, 0xa8) != 0) {
                uint16_t i;
                for (i = 0; i < 10; i++) {           /* the first class-1 slot no usable skill holds */
                    uint16_t q;
                    for (q = 0; q < ld16(st, 0x144); q++) {
                        if (ld32(st, 0x14c + 12u * q) != 1 || ld16(st, 0x150 + 12u * q) != i) continue;
                        const uint8_t *e2 = skill_row(ld32(st, 0x148 + 12u * q));
                        if (e2 != NULL && ld32(e2, 0xa8) != 0) break;
                    }
                    if (q >= ld16(st, 0x144)) { cls = 1; slot = i; break; }
                }
                if (i >= 10) {                       /* else the first free class-2 slot */
                    uint16_t q;
                    for (q = 0; member_skill_at(st, 2, q) != NULL; q++)
                        if ((uint16_t)(q + 1) >= 10) { q = 0xffff; break; }
                    if (q == 0xffff) cls = 255;
                    else { cls = 2; slot = q; }
                }
            } else {
                cls = 1;
            }
        }
        st32(st, 0x148 + 12u * n, skill);
        st32(st, 0x14c + 12u * n, cls);
        st16(st, 0x150 + 12u * n, slot);
        st16(st, 0x144, (uint16_t)(n + 1));
    }
}

void pslot_init(wl_pslot *s, wl_room *r, int32_t code, int32_t level, int32_t id, int32_t status, int32_t order)
{
    uint8_t *p = (uint8_t *)s;
    s->active = status;
    s->order = order;
    s->room = r;
    s->in = NULL;
    st16(p, 0x9c8, 0);
    st32(p, 8, 0);
    s->ch = NULL;
    s->code = code;
    s->id = id;
    s->follow = 255;
    s->form = 255;
    s->pace = 7;
    st32(p, 0xa08, 1000);
    const uint8_t *d = dramatist_row(id);
    int32_t c = d != NULL ? ld32(d, 4) : code;
    char_stats_init(s->stats, c, level);
    member_skills_learn(s, c, level);
}

/* the inventory has room for gear: fewer than 48 unequipped gear items */
static int gear_room(const wl_room *r)
{
    uint16_t n = ld16(r, 0x4010), c = 0;
    for (uint16_t k = 0; k < n; k++) {
        const uint8_t *it = ITEM(r, k);
        if (it[0xa] == 0 && ld32(ldp(it, 0xc), 0x2ac) < 900) c++;
    }
    return c < 48;
}

int room_item_add(wl_room *r, int32_t item, uint16_t *idx, int32_t count, uint8_t flag, int32_t dry)
{
    uint16_t n = ld16(r, 0x4010);
    if (n >= 0x200) return 0;
    const uint8_t *d = item_row(item);
    if (d == NULL) return 0;
    if (ld32(d, 0x2ac) < 900 && n != 0 && !gear_room(r)) return 0;
    if (dry == 0) {
        uint8_t *it = ITEM(r, n);
        st32(it, 0, item);
        stp(it, 0xc, d);
        st16(it, 8, count > 0 ? (uint16_t)count : ld16(d, 0x64));
        st32(it, 4, 0);
        it[0xa] = flag;
        if (idx != NULL) *idx = n;
        st16(r, 0x4010, (uint16_t)(n + 1));
    }
    return 1;
}

void room_items_grant(wl_room *r, int32_t id)
{
    const uint8_t *t = D(ITEM_TBL);
    for (uint16_t i = 0; ld32(t, 736u * i) != 0; i++) {
        const uint8_t *row = t + 736u * i;
        if (ld32(row, 0x2b8) != id || ld32(row, 0x2bc) == 0) continue;
        int32_t item = ld32(row, 0);
        uint16_t n = ld16(r, 0x4010), have = 0;
        for (uint16_t k = 0; k < n; k++) if (ld32(ITEM(r, k), 0) == item) have++;
        if (have != 0 || n >= 0x200) continue;
        const uint8_t *d = item_row(item);
        if (d == NULL) continue;
        if (ld32(d, 0x2ac) < 900 && n != 0 && !gear_room(r)) continue;
        uint8_t *it = ITEM(r, n);
        st32(it, 0, item);
        stp(it, 0xc, d);
        st16(it, 8, ld16(d, 0x64));
        st32(it, 4, 0);
        it[0xa] = 0;
        st16(r, 0x4010, (uint16_t)(n + 1));
    }
}

void room_leader_set(wl_room *r, wl_pslot *s)
{
    void *in = NULL;
    if (s != NULL) {
        wl_pslot *o = r->leader;
        if (o != NULL) {
            in = o->in;
            o->in = NULL;
            if (o->active == 1 && o->id != 100000104 && o->ch != NULL) {
                wl_char *c = o->ch;
                st32(c, 0x158a4, 0);
                st32(c, 0x200, 1);
                char_ai_reset(c, 1);
                char_ground_probe(c);
            }
        }
        s->in = in;
        if (s->active != 1) { r->leader = s; return; }
        if (s->id != 100000104 && s->ch != NULL) {
            wl_char *c = s->ch;
            if (in != NULL) {
                stp(c, 0x158a4, in);
                st32(c, 0x200, 0);
            } else {
                st32(c, 0x158a4, 0);
                st32(c, 0x200, 1);
            }
            char_ai_reset(c, 1);
            char_ground_probe(c);
        }
    }
    r->leader = s;
}

int item_fits(const wl_pslot *s, const uint8_t *item)
{
    int32_t k = ld32(item, 0x7c);
    switch (s->id) {
    case 100000101: return k == 1 || k == 2 || k == 4 || k == 9;
    case 100000102: return k == 1 || k == 3 || k == 5 || k == 9;
    case 100000103: return k == 1 || k == 3 || k == 6 || k == 9;
    case 100000104: return k == 8;
    default: return 0;
    }
}

void pslot_status_set(wl_pslot *s, int32_t in, void *ch)
{
    if (in != 0) { s->active = 1; s->ch = ch; }
    else { s->active = 2; s->ch = NULL; }
}

void pslot_revive_mark(wl_pslot *s)
{
    uint8_t *p = (uint8_t *)s;
    if (ld16(p, 0xe5a) == 0) return;
    st32(p, 0xa64, 1);
    st16(p, 0xe5a, 0);
}

void item_stats_add(const uint8_t *item, uint8_t *st)
{
    int32_t kind = ld32(item, 0x78);
    if (kind == 0) return;
    if (kind == 1)
        for (uint32_t o = 0; o < 20; o += 4) st32(st, 0x124 + o, ld32(item, 0x98 + o));
    uint16_t n = ld16(st, 0x120);
    st32(st, 0x100 + 4u * n, ld32(item, 0));
    st16(st, 0x120, (uint16_t)(n + 1));
    for (uint32_t o = 0; o < 24; o += 4) st32(st, 0x74 + o, ld32(st, 0x74 + o) + ld32(item, 0x80 + o));
    for (int i = 0; i < 4; i++) {
        uint16_t ix = ld16(item, 0x2c0 + 8u * i);
        if (ix == 0) continue;
        uint32_t o = 0xa4 + 4u * ix;
        st32(st, o, mul(ld32(st, o), ld32(item, 0x2c4 + 8u * i)) / 1000);
    }
}

void stats_recompute(wl_room *r, int32_t id, uint8_t *st, const uint8_t *with)
{
    const uint8_t *m = NULL;
    for (int i = 0; i < 8; i++) if (r->slot[i]->id == id) { m = (const uint8_t *)r->slot[i]; break; }
    if (m == NULL || ld32(m, 0x9c4) != 1) return;
    st32(st, 0x64, ld32(m, 0xa74));
    st32(st, 0x68, ld32(m, 0xa78));
    st32(st, 0x6c, ld32(m, 0xa7c));
    st32(st, 0x70, ld32(m, 0xa80));
    st32(st, 0x8c, ld32(m, 0xa9c));
    st32(st, 0x90, ld32(m, 0xaa0));
    st32(st, 0x94, ld32(m, 0xaa4));
    st32(st, 0x98, ld32(m, 0xaa8));
    for (uint32_t o = 0x74; o <= 0x88; o += 4) st32(st, o, 0);
    st16(st, 0x120, 0);
    for (uint32_t o = 0x124; o <= 0x134; o += 4) st32(st, o, 0);
    stats_fill1000(st);
    for (uint16_t i = 0; i < ld16(r, 0x4010); i++) {
        const uint8_t *it = ITEM(r, i);
        if (ld32(it, 4) != id) continue;
        const uint8_t *d = ldp(it, 0xc);
        int32_t kind = ld32(d, 0x78);
        if (kind != 0 && (with == NULL || ld32(with, 0x78) != kind)) item_stats_add(d, st);
    }
    if (with != NULL && ld32(with, 0x78) != 0) item_stats_add(with, st);
    int32_t hp = ld32(st, 0x58) + ld32(st, 0x9c) + ld32(st, 0x84);
    if (hp >= ld32(st, 0x54)) hp = ld32(st, 0x54);
    st32(st, 0x54, hp);
    int32_t mp = ld32(st, 0x88) + ld32(st, 0x60) + ld32(st, 0xa0);
    if (mp >= ld32(st, 0x5c)) mp = ld32(st, 0x5c);
    st32(st, 0x5c, mp);
}

/* a member's stats from its base and its equipment */
static void member_stats_refresh(wl_room *r, wl_pslot *s)
{
    uint8_t *st = s->stats;
    int32_t id = s->id;
    const uint8_t *m = (const uint8_t *)pslot_by_id(r, id, 0);
    if (m == NULL) return;
    st32(st, 0x64, ld32(m, 0xa74));
    st32(st, 0x68, ld32(m, 0xa78));
    st32(st, 0x6c, ld32(m, 0xa7c));
    st32(st, 0x70, ld32(m, 0xa80));
    st32(st, 0x8c, ld32(m, 0xa9c));
    st32(st, 0x90, ld32(m, 0xaa0));
    st32(st, 0x94, ld32(m, 0xaa4));
    st32(st, 0x98, ld32(m, 0xaa8));
    for (uint32_t o = 0x74; o <= 0x88; o += 4) st32(st, o, 0);
    st16(st, 0x120, 0);
    for (uint32_t o = 0x124; o <= 0x134; o += 4) st32(st, o, 0);
    stats_fill1000(st);
    for (uint16_t i = 0; i < ld16(r, 0x4010); i++) {
        const uint8_t *it = ITEM(r, i);
        if (ld32(it, 4) != id) continue;
        const uint8_t *d = ldp(it, 0xc);
        if (ld32(d, 0x78) != 0) item_stats_add(d, st);
    }
    int32_t hp = ld32(st, 0x58) + ld32(st, 0x9c) + ld32(st, 0x84);
    if (hp >= ld32(st, 0x54)) hp = ld32(st, 0x54);
    st32(st, 0x54, hp);
    int32_t mp = ld32(st, 0x60) + ld32(st, 0xa0) + ld32(st, 0x88);
    if (mp >= ld32(st, 0x5c)) mp = ld32(st, 0x5c);
    st32(st, 0x5c, mp);
}

void room_stats_refresh(wl_room *r, int32_t palette)
{
    for (int i = 0; i < 8; i++) {
        wl_pslot *s = r->slot[i];
        if (s->active == 1 || s->active == 2) member_stats_refresh(r, s);
        if (palette != 0 && r->slot[i]->active == 1 && r->slot[i]->ch != NULL) member_palette(r->slot[i]->ch);
    }
}

/* unequip `id`'s other items of the same equip slot as item record k's */
static void unequip_slot_of(wl_room *r, uint16_t k, int32_t id)
{
    const uint8_t *d = ldp(ITEM(r, k), 0xc);
    for (uint16_t j = 0; j < ld16(r, 0x4010); j++) {
        uint8_t *it = ITEM(r, j);
        if (ld32(it, 4) == id && ld32(ldp(it, 0xc), 0x78) == ld32(d, 0x78)) st32(it, 4, 0);
    }
}

void room_item_equip(wl_room *r, uint16_t k, int32_t id, int32_t refresh)
{
    if (id != 0) unequip_slot_of(r, k, id);
    st32(ITEM(r, k), 4, id);
    if (refresh == 0) return;
    for (int i = 0; i < 8; i++) {
        wl_pslot *s = r->slot[i];
        if (s->active == 1 || s->active == 2) member_stats_refresh(r, s);
        if (r->slot[i]->active == 1 && r->slot[i]->ch != NULL) member_palette(r->slot[i]->ch);
    }
}

/* the member hands its items back (their ids into its +0x9cc list) */
static void member_unequip_all(wl_room *r, wl_pslot *s)
{
    uint8_t *p = (uint8_t *)s;
    st16(p, 0x9c8, 0);
    for (uint16_t k = 0; k < ld16(r, 0x4010); k++) {
        uint8_t *it = ITEM(r, k);
        if (ld32(it, 4) != s->id) continue;
        uint16_t n = ld16(p, 0x9c8);
        st32(p, 0x9cc + 4u * n, ld32(it, 0));
        st16(p, 0x9c8, (uint16_t)(n + 1));
        st32(it, 4, 0);
    }
}

void room_party_dismiss(wl_room *r)
{
    for (int i = 0; i < 8; i++) {
        wl_pslot *s = r->slot[i];
        if (s->active != 1) continue;
        member_unequip_all(r, s);
        for (int j = 0; j < 8; j++) {
            wl_pslot *t = r->slot[j];
            if (pslot_ok(t, 1, 1)) stats_recompute(r, t->id, t->stats, NULL);
            if (pslot_ok(r->slot[j], 1, 0) && r->slot[j]->ch != NULL) member_palette(r->slot[j]->ch);
        }
        s->active = 2;
        s->ch = NULL;
    }
    r->leader = NULL;
}

void room_member_leave(wl_room *r, wl_pslot *s, int32_t check)
{
    member_unequip_all(r, s);
    for (int j = 0; j < 8; j++) {
        wl_pslot *t = r->slot[j];
        if (t->active == 1 || t->active == 2) stats_recompute(r, t->id, t->stats, NULL);
        if (r->slot[j]->active == 1 && r->slot[j]->ch != NULL) member_palette(r->slot[j]->ch);
    }
    if (check != 0) {
        /* nobody else standing: the first other member is marked to revive */
        int alive = 0;
        wl_pslot *first = NULL;
        for (int j = 0; j < 8; j++) {
            wl_pslot *t = r->slot[j];
            if (t->active != 1 || t->id == 100000104 || s->id == t->id) continue;
            if (first == NULL) first = t;
            const wl_char *c = t->ch;
            if (c != NULL && c->base.alive == 1 && c->base.body->active == 1 && ld16(c->party, 0x44a) == 0) alive++;
        }
        if (alive == 0 && first != NULL) pslot_revive_mark(first);
        s->ch = NULL;
        s->active = 2;
    } else {
        s->active = 2;
        s->ch = NULL;
    }
}

/* the area's level row to every member */
static void party_level_apply(wl_room *r, int32_t mode, int all_ok)
{
    const int32_t *row = area_level_row(ld32(r, 0x401c));
    st32(r, 0x4020, row[3]);
    for (int i = 0; i < 8; i++) {
        wl_pslot *t = r->slot[i];
        if (all_ok) {
            if (pslot_ok(t, 0, 0) || pslot_ok(t, 0, 1)) g_eng.h41d780(t, 1, row[1], row[2]);
        } else if (t->active != 1 || t->id == 100000104) {
            if (pslot_ok(t, 0, 1)) g_eng.h41d780(t, 1, row[1], row[2]);
        } else {
            g_eng.h41d780(t, mode, row[1], row[2]);
        }
    }
}

wl_pslot *room_member_join(wl_room *r, int32_t id, wl_char *ch, int32_t mode)
{
    wl_pslot *s = NULL;
    for (int i = 0; i < 8; i++) if (r->slot[i]->id == id) { s = r->slot[i]; break; }
    if (s == NULL || (s->active != 1 && s->active != 2)) return NULL;
    uint8_t *p = (uint8_t *)s;
    for (uint16_t i = 0; i < ld16(p, 0x9c8); i++) {
        int32_t x = ld32(p, 0x9cc + 4u * i);
        for (uint16_t k = 0; k < ld16(r, 0x4010); k++) {
            uint8_t *it = ITEM(r, k);
            if (ld32(it, 0) != x || ld32(it, 4) != 0 || it[0xa] != 0) continue;
            if (!item_fits(s, ldp(it, 0xc))) continue;
            if (id != 0) unequip_slot_of(r, k, id);
            st32(it, 4, id);
            break;
        }
    }
    st16(p, 0x9c8, 0);
    if (ch != NULL) member_attach(ch, s, ld32(r, 0x4018));
    s->active = 1;
    s->ch = ch;
    party_level_apply(r, mode, 0);
    room_stats_refresh(r, 1);
    if (ch != NULL) room_formation(r);
    return s;
}

wl_pslot *room_member_add(wl_room *r, uint16_t idx, int32_t code, int32_t level, int32_t id, int32_t order,
                          int32_t active)
{
    for (int i = 0; i < 8; i++) {
        wl_pslot *t = r->slot[i];
        if (t->id != id) continue;
        if (t->active == 1 || t->active == 2) return NULL;
        break;
    }
    wl_pslot *s = r->slot[idx];
    uint8_t *p = (uint8_t *)s;
    s->active = 2 - (active != 0);
    s->order = order;
    s->code = code;
    s->in = NULL;
    st16(p, 0x9c8, 0);
    st32(p, 8, 0);
    s->ch = NULL;
    s->room = r;
    s->id = id;
    s->follow = 255;
    s->form = 255;
    s->pace = 7;
    st32(p, 0xa08, 1000);
    const uint8_t *d = dramatist_row(id);
    int32_t c = d != NULL ? ld32(d, 4) : s->code;
    char_stats_init(s->stats, c, level);
    member_skills_learn(s, c, level);
    const uint8_t *t = D(ITEM_TBL);
    for (uint16_t i = 0; ld32(t, 736u * i) != 0; i++) {
        const uint8_t *row = t + 736u * i;
        if (ld32(row, 0x2b8) != id || ld32(row, 0x2bc) == 0) continue;
        uint16_t have = 0;
        for (uint16_t k = 0; k < ld16(r, 0x4010); k++) if (ld32(ITEM(r, k), 0) == ld32(row, 0)) have++;
        if (have == 0) room_item_add(r, ld32(row, 0), NULL, 0, 0, 0);
    }
    party_level_apply(r, 1, 1);
    return s;
}

/* the start party member `id` from the config (cfg +on / +lv) in slot `idx` with order `order` */
static void start_member(wl_room *r, const uint8_t *cfg, uint32_t on, uint32_t lv, int idx, int32_t id, int32_t order)
{
    if (ld32(cfg, on) == 0) return;
    int32_t level = ld32(cfg, lv);
    if (pslot_by_id(r, id, 1) == NULL) {
        pslot_init(r->slot[idx], r, 0, level, id, 1, order);
        room_items_grant(r, id);
        room_party_level(r, 1);
    }
    if (r->leader == NULL) room_leader_set(r, pslot_by_id(r, id, 0));
}

static const uint8_t *keycfg(void)
{
    return (const uint8_t *)(intptr_t)ld32((const void *)(intptr_t)ld32(g_eng.sys, 0x320), 0);
}

static void party_heal_ok(wl_room *r)
{
    for (int i = 0; i < 8; i++)
        if (pslot_ok(r->slot[i], 0, 1)) pslot_heal(r->slot[i], 1);
}

void room_story_setup(int32_t *key)
{
    wl_room *r = ldp(key, 4);
    int32_t m = ld32(r, 0x4104);
    if (m == 1) {
        if (key[0] != 0) return;
        st32(r, 0x401c, 1);
        room_party_level(r, 1);
        const uint8_t *cfg = keycfg();
        start_member(r, cfg, 0x804, 0x814, 0, 100000101, 2);
        start_member(r, keycfg(), 0x83c, 0x84c, 2, 100000103, 3);
        start_member(r, keycfg(), 0x820, 0x830, 1, 100000102, 5);
        party_heal_ok(r);
        st32(r, 0x40d0, 0);
    } else if (m == 1010) {
        if (key[0] != 0) return;
        st32(r, 0x401c, 0);
        room_party_level(r, 1);
        int32_t cap = ld32(r, 0x4020), g = ld32(r, 0x4014);
        if (cap > g) {
            st32(r, 0x4014, g + 5);
            st32(r, 0x4014, cap >= g + 5 ? g + 5 : cap);
        }
        if (pslot_by_id(r, 100000101, 1) == NULL) {
            pslot_init(r->slot[0], r, 0, 1, 100000101, 1, 2);
            room_items_grant(r, 100000101);
            room_party_level(r, 1);
        }
        room_leader_set(r, pslot_by_id(r, 100000101, 0));
        room_next_set(r, 210110, 101, 1);
        party_heal_ok(r);
        st32(r, 0x40d0, 0);
    } else if (m == 1020) {
        if (key[0] != 1) return;
        uint8_t *fill = ldp(r, 0x40ac);
        const uint8_t *f = flag_find(fill, 100100101);
        if (f == NULL || ld32(f, 4) != 500) return;
        const uint8_t *f2 = flag_find(fill, 101200101);
        if (f2 != NULL && ld32(f2, 4) != 0 && ld32(f2, 4) != 50) return;
        room_next_set(r, 200115, 102, 1);
        st32(r, 0x40d0, 0);
        for (int i = 0; i < 8; i++) if (pslot_ok(r->slot[i], 1, 0)) room_member_leave(r, r->slot[i], 0);
        room_leader_set(r, NULL);
        party_heal_ok(r);
        uint8_t *g = flag_find(ldp(r, 0x40ac), 100100101);
        if (g == NULL) g = flag_add(ldp(r, 0x40ac), 100100101);
        st32(g, 4, 490);
        st32(g, 8, 0);
        g = flag_find(ldp(r, 0x40ac), 101200101);
        if (g == NULL) g = flag_add(ldp(r, 0x40ac), 101200101);
        st32(g, 4, 5);
        st32(g, 8, 0);
        g_eng.h4c5af0(ldp(r, 0x40ac), 100100102, 20, 0);
        room_member_join(r, 100000101, NULL, 1);
        room_member_join(r, 100000102, NULL, 1);
        room_leader_set(r, pslot_by_id(r, 100000101, 0));
    }
}

/* the member's held items back on it where they fit (+0x9cc list), then it rejoins in place */
static void member_rejoin_items(wl_room *r, int32_t id)
{
    wl_pslot *s = pslot_by_id(r, id, 1);
    if (s == NULL) return;
    uint8_t *p = (uint8_t *)s;
    for (uint16_t i = 0; i < ld16(p, 0x9c8); i++) {
        int32_t x = ld32(p, 0x9cc + 4u * i);
        for (uint16_t k = 0; k < ld16(r, 0x4010); k++) {
            uint8_t *it = ITEM(r, k);
            if (ld32(it, 0) != x || ld32(it, 4) != 0 || it[0xa] != 0 || !item_fits(s, ldp(it, 0xc))) continue;
            room_item_equip(r, k, id, 0);
            break;
        }
    }
    st16(p, 0x9c8, 0);
    pslot_status_set(s, 1, NULL);
    room_party_level(r, 1);
    room_stats_refresh(r, 1);
}

void room_story_setup2(int32_t *key)
{
    wl_room *r = ldp(key, 4);
    int32_t m = ld32(r, 0x4104);
    if (m == 2) {
        if (key[0] != 0) return;
        st32(r, 0x401c, 19);
        room_party_level(r, 1);
        room_member_add(r, 0, 0, 1, 100000101, 2, 1);
        room_member_add(r, 2, 0, 1, 100000103, 3, 1);
        room_member_add(r, 1, 0, 1, 100000102, 5, 1);
        room_leader_set(r, pslot_by_id(r, 100000101, 0));
        g_eng.h4e6520(r, 2, 0);
        g_eng.h4e61d0(r, 4, 0);
        g_eng.h4c5af0(ldp(r, 0x40ac), 100100101, 1500, 0);
        g_eng.h4c5af0(ldp(r, 0x40ac), 102100109, 0, 0);
        g_eng.h4c5af0(ldp(r, 0x40ac), 102410102, 1, 0);
        room_next_set(r, 410230, 1, 1);
        room_party_heal(r, 1);
        st32(r, 0x40d0, 0);
    } else if (m == 1010 && key[0] == 1) {
        const uint8_t *f = flag_find(ldp(r, 0x40ac), 100100101);
        if (f == NULL) return;
        int32_t v = ld32(f, 4);
        if (v == 500) {
            room_next_set(r, 230140, 106, 1);
            st32(r, 0x40d0, 0);
            room_party_dismiss(r);
            room_party_heal(r, 1);
            g_eng.h4c5af0(ldp(r, 0x40ac), 100100101, 1000, 0);
            g_eng.h4c5af0(ldp(r, 0x40ac), 100100102, 20, 0);
            room_member_join(r, 100000101, NULL, 1);
            room_member_join(r, 100000102, NULL, 1);
            room_member_join(r, 100000104, NULL, 1);
            room_leader_set(r, pslot_by_id(r, 100000101, 0));
        } else if (v == 2900) {
            room_next_set(r, 210110, 1, 1);
            st32(r, 0x40d0, 0);
            for (int i = 0; i < 8; i++) if (pslot_ok(r->slot[i], 1, 0)) room_member_leave(r, r->slot[i], 0);
            room_leader_set(r, NULL);
            member_rejoin_items(r, 100000101);
            member_rejoin_items(r, 100000102);
            member_rejoin_items(r, 100000103);
            member_rejoin_items(r, 100000104);
            room_leader_set(r, pslot_by_id(r, 100000101, 0));
            room_party_heal(r, 1);
            g_eng.h4c5af0(ldp(r, 0x40ac), 100100101, 1610, 0);
            g_eng.h4c5af0(ldp(r, 0x40ac), 102100111, 30, 0);
            g_eng.h4c5af0(ldp(r, 0x40ac), 102450103, 0, 0);
            g_eng.h4c5af0(ldp(r, 0x40ac), 100100102, 20, 0);
        }
    }
}
