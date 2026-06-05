/*
 * camera_follow.{c,h} — the in-game camera EASE-TO-TARGET follow
 * (FUN_0043d1d0) + its screen-shake sub-applier (FUN_0043d340).
 *
 * The view/camera object (the room-state's +0x104c, operator_new(0x78)) holds a
 * CURRENT scroll origin that the engine eases toward a TARGET scroll origin once
 * per frame.  FUN_0043d1d0 is that easer — found ckpt 69 with a hardware
 * watchpoint on *(*(0x8a9b50)+0x104c)+0x60 (it is dispatched through a heap
 * function pointer, so it is invisible to static call-graph search); the single
 * writer insn is 0x43d26d.  It is called once per frame from 0x439690:1123,
 * immediately before FUN_00499ab0 (the shake/HUD update).
 *
 * THE EASE (per axis, verbatim from FUN_0043d1d0):
 *     dist = |tgt - cur|
 *     if (vel < dist) {                       // still approaching
 *         extra = (flag && dist > 16000) ? (dist - 16000)/10 : 0;  // far-boost
 *         cur += (cur < tgt ? +1 : -1) * (vel + extra);
 *         vel  = min(vel + 10, cap);          // accelerate (+10/frame, cap +0x20)
 *     } else {                                // within one step
 *         cur  = tgt;                         // snap onto target
 *         vel  = max(vel - 10, 0);            // decelerate
 *     }
 * The opening-town pan has flag==0 (no far-boost): cur=128000 eases to tgt=12800
 * with vel ramping 10,20,…,290,300 (cap = +0x20 = 300) then cruising — the exact
 * trajectory the HW watchpoint captured (docs/findings/in-game-intro.md "The
 * camera EASER located").
 *
 * Then FUN_0043d340 writes the projector sub-accumulator (view +0x34 / +0x4c)
 * from a screen-shake block + a 0..1000 envelope, clamped to the map.  Inert
 * during the pan (block inactive -> accumulator 0).
 *
 * Field offsets are the retail view-object byte offsets (the canonical names for
 * the flow-trace annotation + the struct tagging).  This port struct is a clean
 * host layout; the fields the easer touches are modelled, the rest documented.
 */
#ifndef OSS_CAMERA_FOLLOW_H
#define OSS_CAMERA_FOLLOW_H

#include <stdint.h>

/*
 * The screen-shake sub-block (view +0x24 for X, +0x3c for Y; 6 dwords).
 * FUN_0043d340's param_1.  `phase`/`envelope` share dword [3] (lo16/hi16).
 */
typedef struct cam_shake {
    int32_t  active;    /* +0x00 [0] — 0 = inactive (forces out = 0)            */
    int32_t  duration;  /* +0x04 [1] — total frames (0 = unbounded)            */
    int32_t  counter;   /* +0x08 [2] — elapsed frames                          */
    int16_t  phase;     /* +0x0c [3].lo16 — envelope ramp dir (0 up / 1 down)  */
    uint16_t envelope;  /* +0x0e [3].hi16 — 0..1000 fade weight                */
    int32_t  out;       /* +0x10 [4] — output accumulator (-> view +0x34/+0x4c)*/
    int32_t  amp;       /* +0x14 [5] — amplitude                               */
} cam_shake;

/*
 * The camera/view fields the follow touches (retail byte offsets in comments).
 */
typedef struct camera_view {
    int32_t  map_w;     /* +0x00 — map pixel width  (dim0*0xc80)               */
    int32_t  map_h;     /* +0x04 — map pixel height (dim1*0xc80)               */
    int32_t  vel_x;     /* +0x08 — X follow velocity (accumulates +10/frame)   */
    int32_t  vel_y;     /* +0x0c — Y follow velocity                           */
    int32_t  flag;      /* +0x1c — far-boost enable (0 for the town pan)       */
    int32_t  cap;       /* +0x20 — velocity cap = pan SPEED (300 for the town) */
    int32_t  cur_y;     /* +0x5c — CURRENT scroll-y origin                     */
    int32_t  cur_x;     /* +0x60 — CURRENT scroll-x origin                     */
    int32_t  vp_w;      /* +0x64 — viewport width  (64000)                     */
    int32_t  vp_h;      /* +0x68 — viewport height (48000)                     */
    int32_t  tgt_x;     /* +0x6c — TARGET scroll-x origin                      */
    int32_t  tgt_y;     /* +0x70 — TARGET scroll-y origin                      */
    int32_t  accum_x;   /* +0x34 — projector sub-accumulator x (shake out)     */
    int32_t  accum_y;   /* +0x4c — projector sub-accumulator y (shake out)     */
    cam_shake shake_x;  /* +0x24 */
    cam_shake shake_y;  /* +0x3c */
} camera_view;

/*
 * One axis of FUN_0043d1d0: ease *cur toward tgt, integrating *vel (accel +10
 * up to cap, decel -10, snap when within one step).  `flag`!=0 adds the
 * far-boost (+ (dist-16000)/10 when dist>16000).  Exposed for direct testing.
 */
void camera_follow_axis(int32_t *cur, int32_t *vel, int32_t tgt,
                        int32_t flag, int32_t cap);

/*
 * FUN_0043d340: compute the projector sub-accumulator from a shake block, the
 * current scroll on this axis, and the max scroll (map - viewport).  Advances
 * the block's envelope/counter and returns the accumulator value (also stored
 * in s->out).  Inactive block (s->active==0) -> 0.
 */
int32_t camera_shake_apply(cam_shake *s, int32_t cur, int32_t max_scroll);

/*
 * FUN_0043d1d0: the once-per-frame camera follow — ease cur_x/cur_y toward
 * tgt_x/tgt_y, then update the projector sub-accumulators via the shake blocks.
 */
void camera_follow_step(camera_view *v);

#endif /* OSS_CAMERA_FOLLOW_H */
