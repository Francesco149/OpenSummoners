/*
 * scene_fade.{c,h} — the SCENE-TRANSITION FADE-GRID (the establishing REVEAL).
 *
 * A self-contained cinematic subsystem: the room-enter iris that opens the
 * static scene from black.  Distinct from the letterbox (letterbox.c, the
 * constant 64px bars) — this is the per-cell black overlay that recedes once,
 * at scene entry, then sits inert (all cells cleared).
 *
 * RE'd from the retail decompile (ckpt 95) + live ground-truth (runs/reveal-grid,
 * the 0x48e920 field spec):
 *   - render   FUN_0048e920 (called from the world driver 0x48c150:175, AFTER
 *              the letterbox bars) -> scene_fade_render
 *   - arm/init 0x439690:555-583 (the frame-FSM scene-transition trigger,
 *              gated on a request flag) -> scene_fade_arm
 *   - update   the inline grid block 0x499ab0:125-177 (the cinematic step,
 *              once/sim-tick) + the iris pattern setters FUN_0049a890 (variant 0,
 *              center-out) / FUN_0049a740 (1, edges-in) / FUN_0049aae0 +
 *              FUN_0049aa00 (2, single sweep) -> scene_fade_step
 *
 * NB the ckpt-90 docs mis-named 0x49af40 "the grid update"; reading it, it
 * is the HUD/portrait/HP-bar animator (walks the party array room+0x4030) — NOT
 * the fade grid.  The real per-cell update is the 0x499ab0 inline loop + the
 * 0x49a8xx pattern setters (corrected ckpt 95).
 *
 * Live params (town establishing reveal, map 0x3f2): W=10, H=120, count=1200,
 * mode=1 (fade-out), speed=1000; variant is RNG-chosen ((rand*3)>>15 in {0,1,2};
 * this capture got 0 = center-out).  The measured envelope (golden video):
 * top/bottom black 240->64 at -8px/sim-tick = mode-1's 2 rows/tick x the 4px row
 * pitch, settling ~sim-tick 25.
 *
 * Pure C (no Win32 / ddraw): the cel resolve + blit is the caller's sink, like
 * letterbox.c.
 */
#ifndef OPENSUMMONERS_SCENE_FADE_H
#define OPENSUMMONERS_SCENE_FADE_H

#include <stdint.h>

/* The grid tiles the full 640x480 screen in 64px x 4px cells (live-confirmed
 * W=10, H=120, count=1200; runs/reveal-grid 0x48e920 header). */
#define SCENE_FADE_W       10
#define SCENE_FADE_H       120
#define SCENE_FADE_COUNT   (SCENE_FADE_W * SCENE_FADE_H)
#define SCENE_FADE_CELL_W  0x40   /* dest x = col << 6 */
#define SCENE_FADE_CELL_H  4      /* dest y = row << 2 */

/* fade mode (request+0x28 at arm) */
#define SCENE_FADE_MODE_OUT 1     /* reveal: opaque -> alpha -> clear (per cell) */
#define SCENE_FADE_MODE_IN  2     /* cover:  clear  -> alpha -> opaque           */

/* One cell (0x48e920 stride 0xc): state 0=opaque / 1=fading / 2=clear. */
typedef struct {
    uint16_t state;
    uint16_t timer;   /* 0..1000; fade progress within state 1 */
    int32_t  col;     /* dest x = col << 6 */
    int32_t  row;     /* dest y = row << 2 */
} scene_fade_cell;

typedef struct {
    scene_fade_cell cells[SCENE_FADE_COUNT];
    int32_t  mode;      /* 1=out reveal, 2=in cover, 0/3=instant (done at once) */
    int32_t  variant;   /* iris shape: 0 center-out / 1 edges-in / 2 sweep      */
    int32_t  speed;     /* fade rate; per-tick step = speed*100/1000            */
    uint16_t radius;    /* iris progress (rows from center / edge), grows /tick */
    int32_t  done;      /* set once every cell is settled (0x499ab0:176)        */
    int32_t  armed;     /* 0 until first arm (count==0 gate, 0x48e920:17)       */
} scene_fade_grid;

/* Arm the grid at scene entry (0x439690:555-583): fill all W*H cells
 * {state=0, timer=0, col, row}, set mode/variant/speed, reset radius+done.
 * `variant` is the RNG draw (rand*3)>>15 in {0,1,2}; the caller consumes the LCG
 * value and passes it in, keeping that draw visible at the wire site. */
void scene_fade_arm(scene_fade_grid *g, int mode, int variant, int speed);

/* One sim-tick update (0x499ab0:125-177 grid block): advance the iris
 * (mark `rows` new rows fading, rows = 2 for mode 1 / 4 for mode 2) + age each
 * fading cell's timer (+step, ->state 2 at 1000).  No-op once done/disarmed. */
void scene_fade_step(scene_fade_grid *g);

/* Render sinks (FUN_0048e920's two blit primitives, after the letterbox):
 *   opaque — the solid black cel res 0x583 (== the letterbox cel) at (x,y).
 *   alpha  — the black cel res 0x458 frame[level], level 1..31 (FUN_005bd550
 *            alpha composite; lower level = more transparent). */
typedef void (*scene_fade_opaque_fn)(void *ctx, int x, int y);
typedef void (*scene_fade_alpha_fn)(void *ctx, int x, int y, int level);

/* Render the grid (FUN_0048e920).  Walks every cell; emits opaque/alpha/skip by
 * mode+state.  No-op when unarmed.  Drawn AFTER the letterbox bars. */
void scene_fade_render(const scene_fade_grid *g,
                       scene_fade_opaque_fn opaque,
                       scene_fade_alpha_fn alpha, void *ctx);

/* True while any cell is still non-clear (the reveal is still painting black) —
 * the wiring uses this to keep rendering / know when to stop. */
int scene_fade_active(const scene_fade_grid *g);

#endif /* OPENSUMMONERS_SCENE_FADE_H */
