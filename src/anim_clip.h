/*
 * anim_clip.{c,h} — the in-game ACTOR ANIMATION cycle: the sprite-clip
 * descriptor + the per-sim-tick frame-stepper.
 *
 * THE COUNTER (ckpt 71 determinism work).  Every animating actor behaviour in
 * the per-actor update 0x54f980 runs ONE identical inline frame-stepper.
 * 0x54f980 is called once per render-state entry from the per-tick actor
 * driver 0x46cd70 (itself called once per SIM-TICK from the in-game loop
 * 0x439690) — so the animation advances per sim-tick, NOT per Flip.  That
 * is what makes the actor anim a clean, pinnable per-tick clock (engine-quirk
 * #76; docs/findings/in-game-intro.md "The actor animation cycle").
 *
 * THE STEPPER (verbatim from 0x54f980, e.g. behaviour case 0x112e4 and the
 * ~6 other animating cases — all byte-identical):
 *     seq = rstate[+0x6c];                       // current clip
 *     if (seq && (++rstate[+0x70] >= seq[+0x44])) {   // timer++ >= duration
 *         rstate[+0x72]++;                       // frame++
 *         rstate[+0x70] = 0;                     // reset timer
 *         if (rstate[+0x72] >= seq[+0x42]) {     // ran past the last frame
 *             if (seq[+0x48] == 0)               //   looping clip
 *                 rstate[+0x72] = seq[+0x152];   //     jump to the loop-to frame
 *             else {                             //   one-shot clip
 *                 rstate[+0x70] = 1;             //     hold (timer parked at 1)
 *                 rstate[+0x74] = 1;             //     raise the "finished" flag
 *                 rstate[+0x72] = seq[+0x42]-1;  //     freeze on the last frame
 *             }
 *         }
 *     }
 *
 * THE SET (verbatim from 0x40afe0:235-240 / 0x41e600:25-30 / the
 * 0x54f980 case-0x112f6 arm — the state-transition handlers that point an
 * actor at a new clip): assign rstate[+0x6c] and reset timer/frame/finished —
 * but ONLY when the clip actually changes, so re-asserting the same state lets
 * the cycle keep running.
 *
 * Field offsets below are the retail render-state / clip byte offsets (the
 * canonical names for the flow-trace annotation + struct tagging).  The clip is
 * a real game-data structure, so its layout is pinned with _Static_assert; the
 * anim_state is the slice of the 0x294-byte render-state the stepper touches
 * (the full render-state lands with the entity/spawn system — PORT-DEBT
 * present-actor-modes).
 */
#ifndef OSS_ANIM_CLIP_H
#define OSS_ANIM_CLIP_H

#include <stdint.h>
#include <stddef.h>

/*
 * The animation CLIP descriptor (the actor render-state's +0x6c points at one).
 * A fixed-layout 32-frame sprite clip — confirmed by two witnesses: the stepper
 * 0x54f980 (count/dur/oneshot/loop_to) and the renderer 0x491ae0's
 * case-0x1872d seq reads (base + per-frame sprite delta + per-frame x/y offset).
 *   frame f's sprite id     = base_sprite + frame_delta[f]
 *   frame f's draw offset   = (off_x[f], off_y[f])
 */
#define ANIM_CLIP_MAX_FRAMES 32

typedef struct anim_clip {
    int16_t  base_sprite;                  /* +0x00  base sprite id              */
    int16_t  frame_delta[ANIM_CLIP_MAX_FRAMES]; /* +0x02 per-frame sprite delta  */
    uint16_t frame_count;                  /* +0x42  number of frames            */
    uint16_t frame_dur;                    /* +0x44  sim-ticks per frame         */
    uint16_t _u46;                         /* +0x46  (unread)                    */
    int32_t  oneshot;                      /* +0x48  0 = loop, !=0 = play-once   */
    int32_t  flag_abs;                     /* +0x4c  renderer offset/abs mode    */
    int32_t  off_x[ANIM_CLIP_MAX_FRAMES];  /* +0x50  per-frame X offset          */
    int32_t  off_y[ANIM_CLIP_MAX_FRAMES];  /* +0xd0  per-frame Y offset          */
    uint16_t link;                         /* +0x150 next-clip link (renderer)   */
    uint16_t loop_to;                      /* +0x152 frame to jump to on loop    */
} anim_clip;

_Static_assert(offsetof(anim_clip, frame_count) == 0x42, "clip frame_count @0x42");
_Static_assert(offsetof(anim_clip, frame_dur)   == 0x44, "clip frame_dur @0x44");
_Static_assert(offsetof(anim_clip, oneshot)     == 0x48, "clip oneshot @0x48");
_Static_assert(offsetof(anim_clip, flag_abs)    == 0x4c, "clip flag_abs @0x4c");
_Static_assert(offsetof(anim_clip, off_x)       == 0x50, "clip off_x @0x50");
_Static_assert(offsetof(anim_clip, off_y)       == 0xd0, "clip off_y @0xd0");
_Static_assert(offsetof(anim_clip, link)        == 0x150, "clip link @0x150");
_Static_assert(offsetof(anim_clip, loop_to)     == 0x152, "clip loop_to @0x152");
_Static_assert(sizeof(anim_clip)                == 0x154, "clip is 0x154 bytes");

/*
 * The animation STATE — the slice of the actor render-state (0x294 bytes) the
 * stepper reads/writes.  Retail byte offsets in comments.
 */
typedef struct anim_state {
    const anim_clip *clip;  /* +0x6c — current clip (NULL = inert)               */
    uint16_t timer;         /* +0x70 — sim-ticks elapsed in the current frame    */
    uint16_t frame;         /* +0x72 — current frame index                       */
    uint8_t  done;          /* +0x74 — one-shot finished flag                    */
} anim_state;

/*
 * The per-sim-tick frame-stepper (0x54f980's inline animating idiom).
 * Advances `st` by exactly one sim-tick: NULL clip is a no-op; a looping clip
 * cycles forever; a one-shot clip freezes on its last frame and sets `done`.
 */
void anim_clip_advance(anim_state *st);

/*
 * Point an actor at a clip (the 0x40afe0/0x41e600 transition idiom).
 * Resets timer/frame/done ONLY when the clip changes — re-asserting the same
 * clip leaves the running cycle untouched.
 */
void anim_state_set(anim_state *st, const anim_clip *clip);

/* The current frame's sprite id (base + this frame's delta). */
int32_t anim_clip_sprite(const anim_state *st);

#endif /* OSS_ANIM_CLIP_H */
