/*
 * camera_follow.c — FUN_0043d1d0 (the camera ease-to-target) + FUN_0043d340
 * (its screen-shake sub-applier).  The decompiled arithmetic IS the spec; see
 * camera_follow.h + docs/findings/in-game-intro.md "The camera EASER located".
 */
#include "camera_follow.h"

/* FUN_0043d1d0, one axis (X and Y are identical in the decompile). */
void camera_follow_axis(int32_t *cur, int32_t *vel, int32_t tgt,
                        int32_t flag, int32_t cap)
{
    int32_t c = *cur;
    int32_t v = *vel;
    int32_t dist = tgt - c;
    if (dist < 0) dist = -dist;                 /* |tgt - cur| */

    if (v < dist) {                             /* still approaching */
        int32_t extra = 0;
        if (flag != 0 && dist > 16000) extra = (dist - 16000) / 10;  /* far-boost */
        c += (c < tgt) ? (v + extra) : -(v + extra);
        v += 10;                                /* accelerate */
        if (v > cap) v = cap;                   /* clamp to the speed cap */
    } else {                                    /* within one step of target */
        c = tgt;                                /* snap */
        v -= 10;                                /* decelerate */
        if (v < 0) v = 0;
    }
    *cur = c;
    *vel = v;
}

/* FUN_0043d340 — the projector sub-accumulator from a shake block. */
int32_t camera_shake_apply(cam_shake *s, int32_t cur, int32_t max_scroll)
{
    int32_t amp = (s->amp * 0x640) / 1000;      /* amplitude * 1600 / 1000 */

    if (s->active == 0) {                       /* inactive -> no shake */
        s->out = 0;
        return 0;
    }
    if (s->duration > 0) {
        s->counter += 1;
        if (s->duration <= s->counter) {        /* expired */
            s->active = 0;
            s->out = 0;
            return 0;
        }
        if (s->duration - s->counter < 0x14)    /* fade out over the last 20 */
            amp = ((s->duration - s->counter) * amp) / 0x14;
    }

    int32_t lower = -amp;                        /* iVar5 */
    int32_t upper = 0;                           /* iVar2 */
    if (lower + cur < 0) {                       /* clamp lower to the map edge */
        lower = -cur;
        upper = lower + amp;
    }
    upper += amp;
    if (max_scroll < upper + cur)                /* clamp upper to the far edge */
        upper = max_scroll - cur;

    /* envelope ramp: up by 500/frame to 1000, hold, down by 500 to 0 */
    if (s->phase == 0) {
        if (s->envelope < 1000) {
            int32_t e = (int32_t)s->envelope + 500;
            if (e > 1000) e = 1000;
            s->envelope = (uint16_t)e;
        } else {
            s->phase = 1;
        }
    } else if (s->envelope == 0) {
        s->phase = 0;
    } else {
        int32_t e = (int32_t)s->envelope - 500;
        if (e < 0) e = 0;
        s->envelope = (uint16_t)e;
    }

    s->out = (int32_t)(((uint32_t)s->envelope * (uint32_t)(upper - lower)) / 1000) + lower;
    return s->out;
}

/* FUN_0043d1d0 — the once-per-frame follow over both axes + the shake appliers. */
void camera_follow_step(camera_view *v)
{
    camera_follow_axis(&v->cur_x, &v->vel_x, v->tgt_x, v->flag, v->cap);
    camera_follow_axis(&v->cur_y, &v->vel_y, v->tgt_y, v->flag, v->cap);
    v->accum_x = camera_shake_apply(&v->shake_x, v->cur_x, v->map_w - v->vp_w);
    v->accum_y = camera_shake_apply(&v->shake_y, v->cur_y, v->map_h - v->vp_h);
}

/* 439690's target clamp: max(tgt, 0) then min(that, map_ext - viewport).
 * (Decompile: `((int)x<0)-1 & x` = max(x,0); then `if (m <= ext) keep m`.) */
static int32_t camera_clamp_target(int32_t tgt, int32_t max_scroll)
{
    if (tgt < 0) tgt = 0;
    if (tgt > max_scroll) tgt = max_scroll;
    return tgt;
}

/* 0x439690:643-664 — the +0x4c PAN command: set the (clamped) target +
 * speed/flag, leave cur/vel so FUN_0043d1d0 eases there. */
void camera_apply_pan(camera_view *v, int32_t tgt_x, int32_t tgt_y,
                      int32_t speed)
{
    v->tgt_x = camera_clamp_target(tgt_x, v->map_w - v->vp_w);  /* [0x1b] */
    v->tgt_y = camera_clamp_target(tgt_y, v->map_h - v->vp_h);  /* [0x1c] */
    v->cap   = speed;   /* [8]  = +0x20 velocity cap / pan speed */
    v->flag  = 0;       /* [7]  = +0x1c far-boost flag           */
}

/* 0x439690:599-642 — the +0x40 SNAP command: set the (clamped) target,
 * cap=0/flag=0, then jump cur onto the target and zero the follow velocity. */
void camera_apply_snap(camera_view *v, int32_t tgt_x, int32_t tgt_y)
{
    v->tgt_x = camera_clamp_target(tgt_x, v->map_w - v->vp_w);  /* [0x1b] */
    v->tgt_y = camera_clamp_target(tgt_y, v->map_h - v->vp_h);  /* [0x1c] */
    v->cap   = 0;       /* [8] = 0 */
    v->flag  = 0;       /* [7] = 0 */
    v->cur_x = v->tgt_x;   /* +0x60 = +0x6c (jump) */
    v->cur_y = v->tgt_y;   /* +0x5c = +0x70        */
    v->vel_x = 0;          /* +0x08 = 0            */
    v->vel_y = 0;          /* +0x0c = 0            */
}
