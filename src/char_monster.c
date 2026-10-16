/* char_monster.c — the monsters' per-type inits.  See char_monster.h.  Transcribed from the decompile, the store
 * widths from the asm; the inlined setter runs are the char_def.c setters they copy (clips, walk, jump, hit,
 * move, crouch, step, the skill definition). */
#include "char_monster.h"

#include <stddef.h>
#include <string.h>

#include "char_def.h"
#include "char_state.h"      /* entity_set_track 0x426db0 */
#include "engine_calls.h"
#include "rng.h"
#include "room_load.h"       /* char_follow_set 0x41e020 */

#define TRAILER 0x8a9b6cu

static uint16_t ld16(const void *p, uint32_t off) { uint16_t v; memcpy(&v, (const uint8_t *)p + off, 2); return v; }
static int32_t ld32(const void *p, uint32_t off) { int32_t v; memcpy(&v, (const uint8_t *)p + off, 4); return v; }
static void st16(void *p, uint32_t off, uint16_t v) { memcpy((uint8_t *)p + off, &v, 2); }
static void st32(void *p, uint32_t off, int32_t v) { memcpy((uint8_t *)p + off, &v, 4); }
static void stp(void *p, uint32_t off, const void *v) { memcpy((uint8_t *)p + off, &v, sizeof v); }
static const void *D(uint32_t va) { return g_eng.data(va); }
static int32_t mul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

/* a skill with retail's common defaults (class 0xf00000, kind 3, no name) */
static void skill(wl_char *c, int32_t id, uint16_t fae, int32_t b0, int32_t b4, int32_t b8, int32_t bc, int32_t c4)
{
    char_skill_def(c, id, fae, b0, b4, b8, bc, 0xf00000, c4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, D(TRAILER));
}

/* the AI record of learnt skill `id` built in place (see char_monster.h) */
static void ai_rec(wl_char *c, int32_t id, int32_t f08, int32_t f0c, uint16_t f11a, uint16_t f124, uint16_t f126,
                   int32_t x0, int32_t x1, int32_t y0, int32_t y1)
{
    uint8_t *k = char_skill_find(c, (uint16_t)id);
    uint8_t *r = (uint8_t *)c + 0xca28 + 320u * ld16(c, 0x14228);
    if (k == NULL) return;
    stp(r, 0x000, k);
    st32(r, 0x004, id);
    st32(r, 0x008, f08);
    st32(r, 0x00c, f0c);
    st16(r, 0x11a, f11a);
    st16(r, 0x118, 0);
    st16(r, 0x11c, 0);
    st32(r, 0x120, 1000);
    st16(r, 0x124, f124);
    st16(r, 0x126, f126);
    st16(r, 0x010, 0);
    st16(r, 0x094, 0);
    st32(r, 0x128, 0);
    st32(r, 0x12c, 0);
    st32(r, 0x130, 0);
    st32(r, 0x134, 0);
    st32(r, 0x138, 1000);
    st16(r, 0x13c, 0);
    st16(r, 0x13e, 0);
    ai_skill_boxes_add(r, x0, x1, y0, y1, 0, 0, 0, 0);
    st16(c, 0x14228, (uint16_t)(ld16(c, 0x14228) + 1));
}

/* the target-wander block (+0x14240 5, the facing at +0x14244) */
static void wander_reset(wl_char *c)
{
    st32(c, 0x14240, 5);
    st32(c, 0x1424c, 0);
    st32(c, 0x14250, 0);
    st32(c, 0x14254, 0);
    st32(c, 0x14244, c->base.body->facing);
    st16(c, 0x14248, 0);
    st32(c, 0x14258, 0);
}

void ai_rec_hold_set(wl_char *c, const uint8_t *rec, uint16_t v)
{
    for (uint16_t k = 0; k < ld16(c, 0x14228); k++) {
        uint8_t *r = (uint8_t *)c + 0xca28 + 320u * k;
        if (ld32(r, 4) != ld32(rec, 4)) continue;
        int32_t t = mul(300, (int32_t)rng_rand()) / 0x8000;
        st16(r, 0x11c, (uint16_t)(mul(v, t) / 1000 + (int32_t)(700u * v / 1000u)));
    }
}

void char_skill_list_add(wl_char *c, int32_t v)
{
    uint16_t n = ld16(c, 0x158d0);
    if (n >= 8) return;
    st32(c, 0x158d4 + 4u * n, v);
    st16(c, 0x158d0, (uint16_t)(n + 1));
}

void mon_slime(wl_char *c, int16_t row, int16_t trk1, int32_t pw, int32_t pw2)
{
    int crouch = 0, jumps = 0;
    int32_t split = 0;
    entity_set_track(&c->base, 0, row, 0, 1, 0, 0, 0);
    if (trk1 != 0) entity_set_track(&c->base, 1, trk1, 0, 1, 0, -80, -80);
    uint32_t code = (uint32_t)c->base.code;
    if (code > 0xc747u) {
        if (code == 100186) { crouch = 1; jumps = 1; split = 100186; }
    } else if (code == 51015) {
        crouch = 1; jumps = 1;
    } else if (code >= 0xc744u) {
        if (code <= 0xc745u) jumps = 1;
        else { crouch = 1; jumps = 1; split = 100186; }
    }
    char_clips_set(c, D(0x65ab38), D(0x65b330), D(0x65b4c8), 0, 2);
    char_walk_set(c, 8000, 1600, 3200, -1600, -3200, D(0x65acd0));
    char_jump_set(c, -80000, 2000, 8000, 48000, 1600, 3200, D(0x65ae68), D(0x65b000), D(0x65b198), 137);
    char_clips2_set(c, D(0x62a598), D(0x62a598));
    char_hit_set(c, 104, D(0x65ab38), 0, 1, 0);
    char_step_set(c, 3, 139);
    if (crouch) char_crouch_set(c, 2800, D(0x65bb28), D(0x65bcc0), D(0x65be58));
    skill(c, 20, 1, pw, pw2, 0, 1, 1000);
    skill(c, 19, 1, 0, 0, 0, 1, 1000);
    ai_rec(c, 19, 0, 0, 240, 10, 20, 6400, 6400, -6400, 9600);
    char_ai_skill_add(c, 20, 0, 1, -1600, 6400, -1600, 4800, 80, 1000, 5, 10, 0, 0, 0, 0, 0);
    if (jumps && trk1 != 0) {
        skill(c, 21, 1, pw, pw2, 0, 1, 1000);
        skill(c, 22, 1, pw, pw2, 0, 1, 1000);
        char_ai_skill_add(c, 22, 0, 1, 3200, 12800, -6400, 9600, 80, 1000, 5, 10, 0, 0, 0, 0, 0);
        char_ai_skill_add(c, 21, 0, 1, 1600, 12800, -14400, 12800, 80, 1000, 5, 10, 0, 0, 0, 0, 0);
    }
    if (split != 0) {
        char_skill_def(c, 23, 3, 14, 0, 4, 1, 0xf00000, split, 1, 6, 0, 8, 20, 0, 0, 0, 0, 0, 0, 0, 3, D(TRAILER));
        uint8_t *r = char_ai_skill_add(c, 23, 0, 3, 0, 0, 0, 0, 3000, 1000, 0, 0, 0, 0, 0, 0, 0);
        if (r != NULL) ai_rec_hold_set(c, r, 800);
    }
    if (code == 100164) { char_follow_set(c, 6, NULL, 0, 0); char_pace_set(c, 2, 0, 0, 0); }
    else { char_follow_set(c, 5, NULL, 0, 0); char_pace_set(c, 4, 0, 0, 0); }
    char_c880_set(c, 12800, 800, 12800);
    char_combo_add(c, 1, 21, 0, 2, 0, 10, 300, 0, 210, 8, 1, 0, 0, 0);
    char_combo_add(c, 1, 22, 0, 2, 0, 10, 300, 0, 210, 8, 5, 0, 0, 0);
    char_combo_add(c, 1, 20, 0, 0, 1, 0, 400, 0, 0, 8, 0, 0, 0, 0);
    st32(c, 0x15a90, 0);
    st32(c, 0x1590c, 0);
    char_skill_list_add(c, 3);
}

void mon_skill51(wl_char *c, int16_t row)
{
    entity_set_track(&c->base, 0, row, 0, 1, 0, 0, 0);
    char_clips_set(c, D(0x662548), D(0x6626e0), D(0x6626e0), 0, 1);
    char_walk_set(c, 4000, 400, 200, -1600, -3200, D(0x662548));
    char_jump_set(c, -56000, 2000, 8000, 32000, 1600, 3200, D(0x662548), D(0x662548), D(0x662548), 143);
    char_clips2_set(c, D(0x62a400), D(0x62a400));
    char_hit_set(c, 104, D(0x662548), 1, 1, 0);
    char_move_set(c, 1, 24000, 2000, 64000, 8000, 8000, 1, 0);
    skill(c, 51, 1, 0, 0, 0, 0, 1000);
    ai_rec(c, 51, 1, 1, 200, 10, 10, 6400, 9600, -3200, 8000);
    char_ai_skill_add(c, 51, 0, 1, 6400, 9600, 3200, 8000, 200, 1000, 10, 10, 0, 0, 0, 0, 0);
    char_ai_skill_add(c, 51, 3, 1, 0, 9600, 3200, 8000, 200, 1000, 10, 10, 0, 0, 0, 0, 0);
    wander_reset(c);
    char_pace_set(c, 2, 6400, 800, 200);
    char_c880_set(c, 12800, 6400, 12800);
    char_nav_set(c, ld32(c, 0x1f4), 4800, 16000);
}

void mon_skill29(wl_char *c, int32_t code, int32_t a3, int32_t a4)
{
    (void)code;
    char_clips_set(c, D(0x662ba8), D(0x6633a0), D(0x662ed8), 0, 0);
    char_walk_set(c, 12000, 2000, 4000, -2000, -4000, D(0x662d40));
    char_jump_set(c, -80000, 2000, 8000, 48000, 1600, 3200, D(0x662ed8), D(0x663070), D(0x663208), 137);
    char_clips2_set(c, D(0x62a400), D(0x62a400));
    char_hit_set(c, 104, D(0x663538), 0, 1, 0);
    skill(c, 29, 1, a3, a4, 0, 0, 1000);
    skill(c, 30, 1, a3, a4, 0, 1, 1200);
    ai_rec(c, 29, 0, 1, 60, 5, 10, -1600, 6400, 0, 6400);
    char_ai_skill_add(c, 30, 0, 0, 2000, 8000, -4800, 8000, 200, 1000, 10, 20, 0, 0, 0, 0, 0);
    wander_reset(c);
    char_pace_set(c, 2, 0, 800, 0);
}
