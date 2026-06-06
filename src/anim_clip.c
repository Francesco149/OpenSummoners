/*
 * anim_clip.c — the actor animation cycle (see anim_clip.h for the full RE).
 *
 * Provenance: 0x54f980 (the per-actor update; the inline frame-stepper that
 * every animating behaviour runs) + 0x40afe0 / 0x41e600 (the
 * state-transition clip-set idiom).  Called once per sim-tick via 0x46cd70.
 */
#include "anim_clip.h"

/* 0x54f980's inline frame-stepper — verbatim, one sim-tick. */
void anim_clip_advance(anim_state *st)
{
    const anim_clip *c = st->clip;
    if (c == NULL) {
        return;
    }
    /* timer++ is evaluated unconditionally (retail folds it into the &&). */
    st->timer = (uint16_t)(st->timer + 1);
    if (c->frame_dur <= st->timer) {
        st->frame = (uint16_t)(st->frame + 1);
        st->timer = 0;
        if (c->frame_count <= st->frame) {
            if (c->oneshot == 0) {
                st->frame = c->loop_to;                 /* looping clip */
            } else {
                st->timer = 1;                          /* one-shot: hold */
                st->done  = 1;
                st->frame = (uint16_t)(c->frame_count - 1);
            }
        }
    }
}

/* 0x40afe0:235-240 — assign + reset, but only when the clip changes. */
void anim_state_set(anim_state *st, const anim_clip *clip)
{
    if (st->clip != clip) {
        st->clip  = clip;
        st->done  = 0;
        st->frame = 0;
        st->timer = 0;
    }
}

/* 0x491ae0 case 0x1872d: frame sprite = base + this frame's delta. */
int32_t anim_clip_sprite(const anim_state *st)
{
    const anim_clip *c = st->clip;
    if (c == NULL || st->frame >= ANIM_CLIP_MAX_FRAMES) {
        return 0;
    }
    return (int32_t)c->base_sprite + (int32_t)c->frame_delta[st->frame];
}
