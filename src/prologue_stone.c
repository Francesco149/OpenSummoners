/*
 * src/prologue_stone.c — the Elemental-Stone intro cutscene state model.
 *
 * Port of the visual half of FUN_0056cd20 (2275 bytes): the per-tick UPDATE
 * state machine and the render-descriptor build.  See prologue_stone.h and
 * docs/findings/prologue-stone-intro.md.  Every constant below is from the raw
 * decompile at 0x56cd20 (lines cited inline).  Audio cues are no-ops here (all
 * null-guarded on DAT_008a93e4 / DAT_008a7608 in retail, both null pre-audio).
 */

#include "prologue_stone.h"

/* The cutscene's start state (FUN_0056cd20:50-103 + the per-entry sub stagger
 * at :97-102; the field model is documented in prologue_stone.h). */
void prologue_stone_init(prologue_stone *ps)
{
    /* Per-entry state-0 wait countdown (retail local_46[1]/local_38/.../local_8,
     * the "sub" field) — staggers when each sparkle column begins growing. */
    static const uint16_t sub_seed[PROLOGUE_SPARKLE_ENTRIES] = {
        0x2a8, 0x3d4, 0x62c, 0x758, 0x884, 0xa46,
    };

    ps->watchdog    = 0x1130;   /* uVar16   */
    ps->start_delay = 10;       /* sVar4    */
    ps->sound_cue   = 0xed8;    /* local_90 */
    ps->beats       = 0;        /* uVar8    */
    ps->exiting     = 0;        /* bVar5    */

    ps->gem_phase   = 0;        /* sVar6    */
    ps->gem_fade    = 0;        /* local_bc */
    ps->hold        = 0;        /* local_9c */
    ps->rise_pos    = -4000;    /* local_a0 */
    ps->rise_vel    = 800;      /* local_88 */

    ps->gem_frame   = 0;        /* local_94 */
    ps->gem_sub     = 0;        /* uVar19   */
    ps->aura_frame  = 0;        /* uVar17   */
    ps->aura_sub    = 0;        /* uVar7    */

    for (int i = 0; i < PROLOGUE_SPARKLE_ENTRIES; i++) {
        ps->sparkle[i].state = 0;          /* :89-95 puVar14[-1] = 0          */
        ps->sparkle[i].level = 0;          /* :90    *puVar14 = 0             */
        ps->sparkle[i].sub   = sub_seed[i];/* :97-102 per-entry stagger       */
        ps->sparkle[i].y     = 32000;      /* :92    puVar14[3]=32000,[4]=0   */
    }
}

/* clamp(x, 0, INF) via retail's `((x<0)-1) & x` idiom (max(x,0)). */
static int32_t max0(int32_t x) { return x < 0 ? 0 : x; }

prologue_status prologue_stone_update(prologue_stone *ps, input_mgr *in,
                                      uint32_t now)
{
    /* (1) abort (id 0x22) — checked first every tick (FUN_0056cd20:158-186).
     * FUN_0043c110(now, 0x22) == input_poll_consume. */
    if (in && input_poll_consume(in, now, 0x22))
        return PROLOGUE_ABORT;                       /* local_7c = 6, return 6 */

    /* (2) watchdog (:187-188): at 0 the cutscene completes → enter game. */
    if (ps->watchdog == 0)
        return PROLOGUE_DONE;                        /* return local_7c == 0   */
    ps->watchdog--;

    /* (3) start delay (:189/337): the gem stays hidden for the first 10 ticks. */
    if (ps->start_delay != 0) {
        ps->start_delay--;
        return PROLOGUE_RUNNING;
    }

    /* (4) sound cue countdown (:190-193) — the 0x5bbdb0 call is audio-gated
     * (DAT_008a93e4), a no-op pre-audio; the counter itself still ticks. */
    if (ps->sound_cue != 0)
        ps->sound_cue--;

    /* (5) beat input (:194-240) — only while not yet exiting.  Find the newest
     * fresh press of ANY id; consume it + flush the ring/axes (input_mgr_reset
     * subsumes retail's single-slot consume + the +0x114/+0x140/ring clears),
     * count a beat; the 3rd beat begins the exit. */
    if (!ps->exiting && in && input_any_fresh_press(in, now)) {
        input_mgr_reset(in);
        ps->beats++;                                 /* uVar8++                */
        if (ps->beats > 2) {                         /* :223 2 < uVar8         */
            ps->exiting   = 1;                       /* bVar5 = true           */
            if (ps->watchdog > 200) ps->watchdog = 200;  /* :228-230           */
            ps->gem_phase = 2;                       /* sVar6 = 2 (fade out)   */
            for (int i = 0; i < PROLOGUE_SPARKLE_ENTRIES; i++)
                ps->sparkle[i].state =
                    (uint16_t)(4 - (ps->sparkle[i].state != 0));  /* :234-238 */
        }
    }

    /* (6) gem fade phase machine (:241-263). */
    if (ps->gem_phase == 0) {                        /* fade-in                */
        if (ps->gem_fade < 600) ps->gem_fade++;
        else ps->gem_phase++;
    } else if (ps->gem_phase == 1) {                 /* hold                   */
        if (ps->hold > 0xc7f) ps->gem_phase++;
        else ps->hold++;
    } else if (ps->gem_phase == 2) {                 /* fade-out               */
        if (ps->gem_fade < 1) ps->gem_phase++;
        else ps->gem_fade -= ps->exiting ? 4 : 1;    /* −4 when exiting        */
    }

    /* (7) gem frame (every 7 ticks, %0x23) + aura frame toggle (:264-277). */
    if (ps->gem_sub < 6) ps->gem_sub++;
    else { ps->gem_sub = 0; ps->gem_frame = (uint16_t)((ps->gem_frame + 1) % 0x23); }
    if (ps->aura_sub < 6) ps->aura_sub++;
    else { ps->aura_sub = 0; ps->aura_frame = (uint16_t)(ps->aura_frame == 0); }

    /* (8) gem/aura rise (:278-281): velocity decays once past the top. */
    if (ps->rise_pos > 16000) ps->rise_vel = max0(ps->rise_vel - 10);
    ps->rise_pos += ps->rise_vel / 100;

    /* (9) the 6 sparkle entries (:282-334). */
    for (int i = 0; i < PROLOGUE_SPARKLE_ENTRIES; i++) {
        prologue_sparkle *s = &ps->sparkle[i];
        switch (s->state) {
        case 0:                                      /* wait                   */
            if (s->sub == 0) s->state = 1;
            else s->sub--;
            break;
        case 1:                                      /* grow                   */
            if (s->sub < 10) s->sub++;
            else {
                s->sub = 0;
                if (s->level < 0x14) s->level++;
                else s->state = 2;
            }
            break;
        case 2:                                      /* descend                */
            if (s->y < 0x2ee1) s->state = 3;
            break;
        case 3:                                      /* shrink                 */
            if (s->sub < 10) s->sub++;
            else {
                s->sub = 0;
                if (s->level == 0) s->state = 4;
                else s->level--;
            }
            break;
        default:                                     /* 4 = dead               */
            break;
        }
        if (s->state != 0 && s->state < 4) s->y -= 0x10;   /* :328-330         */
    }

    return PROLOGUE_RUNNING;
}

/* ─── render (FUN_0056cd20:342-415) ──────────────────────────────────────── */
/*
 * Ramp-index convention (recovered from the disasm at 0x56d2c0..0x56d4fc):
 *   - the GEM and SPARKLES blend through ramp_b (DAT_008a9308); their raw index
 *     is emitted verbatim, and the drive applies retail's rule — idx in [0,19]
 *     with ramp_b[idx] != 0 → alpha blit ramp_b[idx], otherwise (idx >= 20 or a
 *     NULL ramp entry) the plain keyed fallback (0x5b9b70).
 *   - the AURA blends through ramp_a (DAT_008a92b8); its index is pre-clamped to
 *     [0,19] here (retail's idx>=20 path loads ramp_a[19] at 0x8a9304, idx<0
 *     loads ramp_a[0]) and the drive always alpha-blits ramp_a[idx] (no keyed
 *     fallback).
 * The drive picks the ramp per element (gem/sparkle → ramp_b, aura → ramp_a)
 * from the struct field, so no per-draw ramp selector is needed.
 */
static int clamp_aura(int idx)   /* ramp_a clamp: [0,19], no keyed fallback */
{
    if (idx < 0) return 0;
    if (idx >= 0x14) return 0x13;
    return idx;
}

void prologue_stone_render(const prologue_stone *ps, prologue_render_out *out)
{
    /* Default everything idle, then fill. */
    out->gem.draw = out->aura.draw = 0;
    for (int k = 0; k < PROLOGUE_SPARKLE_DRAWS; k++) out->sparkle[k].draw = 0;

    /* Render gate: nothing until the start delay elapses (:344 if (sVar4==0)). */
    if (ps->start_delay != 0) return;

    int rise = ps->rise_pos / 100;   /* the shared rise pixels                  */

    /* gem — slot[3] frame gem_frame at (0xf8, rise+0x30); drawn when the fade
     * index is positive (:348-365 / 0x56d2dd-0x56d388).  ramp_idx is the raw
     * ramp_b index ((local_bc*800/1000)*0x14/600); peaks at 16. */
    {
        int iv = (ps->gem_fade * 800) / 1000;        /* :350 iVar15            */
        if (iv > 0) {
            out->gem.draw     = 1;
            out->gem.frame    = ps->gem_frame;
            out->gem.x        = 0xf8;
            out->gem.y        = rise + 0x30;
            out->gem.ramp_idx = (iv * 0x14) / 600;   /* :352, raw (ramp_b)     */
        }
    }

    /* aura — slot[1] frame aura_frame at (0xd0, rise); drawn whenever the gem
     * fade is positive (0x56d38d-0x56d41f).  Blends through ramp_a at index
     * (local_bc*0x14)/600 (= local_bc/30), clamped [0,19]. */
    if (ps->gem_fade > 0) {
        out->aura.draw     = 1;
        out->aura.frame    = ps->aura_frame;
        out->aura.x        = 0xd0;
        out->aura.y        = rise;
        out->aura.ramp_idx = clamp_aura((ps->gem_fade * 0x14) / 600);
    }

    /* sparkles — slot[2], a 6-entry × 4-column grid (:378-414 / 0x56d460-d511).
     * Entry e draws frames 4e..4e+3 at columns 0..3 (x = 0x10 + col·0x98), all
     * sharing the entry's y (= y/100) and ramp_b index (= level).  Drawn while
     * level≠0; the raw index (level, which can reach 0x14) is emitted so the
     * drive falls to the keyed blit at level==0x14, exactly as retail. */
    for (int e = 0; e < PROLOGUE_SPARKLE_ENTRIES; e++) {
        const prologue_sparkle *s = &ps->sparkle[e];
        int sy = s->y / 100;
        for (int col = 0; col < 4; col++) {
            int k = e * 4 + col;                     /* frame == draw index    */
            if (s->level != 0) {
                out->sparkle[k].draw     = 1;
                out->sparkle[k].frame    = k;
                out->sparkle[k].x        = 0x10 + col * 0x98;
                out->sparkle[k].y        = sy;
                out->sparkle[k].ramp_idx = (s->level * 0x14) / 0x14;  /* == level */
            }
        }
    }
}
