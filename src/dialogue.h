/*
 * dialogue.{c,h} — the in-game DIALOGUE BOX (the town-intro speech bubble).
 *
 * A self-contained overlay producer, like banner.{c,h} / scene_fade.{c,h}.  RE'd
 * from the retail decompile + the ckpt-102 ground truth (`runs/dialogue-probe`,
 * `runs/dialogue-portrait`; findings/in-game-intro.md "The DIALOGUE BOX
 * subsystem"; plan docs/plans/dialogue-cutscene.md):
 *
 *   - builder   0x439690:395-514 — on the dirty flag, builds the bubble widget:
 *               the box node (W=0x198/408 with a portrait, H=0x70/112) + cells
 *               for the bubble-TAIL pair (box bank frames 9+10, speaker-anchored
 *               at the box bottom via 0x49c640; 11-14 = the below/twin variants),
 *               the name-TAB cel (0x8a7710 bank = res 0x44a, frame 0 long-name /
 *               1 short @ box(+216,-32)), the NAME text cell (white main +
 *               0x455f7b shadow), TWO portrait cells (cross-fade pair, both @
 *               box(-24,-72)) and the body TEXT grid (cell @ box(+136,+20),
 *               36 chars x 3 rows, line pitch 28).
 *   - position  0x49c640 — box (x,y) from the speaker's world pos (clamped
 *               [0x20, 0x260-W]); for the town line 1 the result is (174,148)
 *               (measured, runs/dialogue-probe box_frame).
 *   - pop-in    0x48c820 node mode +0x1c==1: the 9-slice frame is drawn at
 *               scale +0x54/1000, CENTERED in the full rect; +0x54 += 50 per
 *               widget update (one update per ~2 flips = the sim-tick cadence);
 *               all content cells are GATED until scale==1000 (the
 *               `+0x54 < 1000 && +0x58 == 0` skip).  This is the user-visible
 *               "bubble pop-in" (trace-studio intro-1 mark @2429).
 *   - frame     0x48cf80 — the 9-slice tiler the port already has
 *               (newgame_box_render); box bank = DAT_008a7708 = res 0x456
 *               (32x32 cells, frames 0-8 = tl..br).
 *   - portrait  res 0x7ef (160x176 24bpp BGR bottom-up, magenta key) drawn 1:1
 *               at box(-24,-72) = (150,76); cross-fade 0x49c910: fade += 50 per
 *               update once scale==1000; the NEW cel blends via
 *               ramp_b[(fade*0x14)/500] while fade<500, then holds the last
 *               ramp step until fade==1000 snaps it to the plain keyed blit.
 *   - body text 0x48da70 — per-char records at x = col*7 + cell.x + box.x,
 *               row pitch +0x1ac=28; 3 GDI passes per char: shadow (x,y+1) +
 *               (x+1,y) in +0x184, then main (x,y) in +0x180.  Body colors
 *               0x3e537d main / 0xa8b9cc shadow (measured); name cell white /
 *               0x455f7b (builder 0x439690:464-465).  Font Courier New 7x18
 *               (textout probe).
 *   - typewriter 0x439690:499-505 — interval i = *(*DAT_008a6e80+0x248) widget
 *               updates per char (measured 5 = ~10 flips), grade slots 2i/3i/5i
 *               for pauses; voiced lines override to 6/0x12/0x18/0x24 (unused —
 *               the lockstep probe types at the silent cadence).  Fitted to the
 *               reveal trace: ','/'.'/'!'/'?' pause 3i, a row close adds +i,
 *               a space reveals after 1 update.  PORT-DEBT(dialogue-pause-grades):
 *               the exact char->grade map lives in the unported reveal stepper.
 *   - arrow     0x410560 — the advance arrow rides the text cell at
 *               (+0x7c,+0x80) = box-rel (368,92) = (542,240): frame base
 *               0x14=20 + anim table {0,1,2,3}, one step per 10 updates; the
 *               1px bob is baked in the per-frame cel placement metrics.  It
 *               is HIDDEN while the typewriter runs (0x48d940's +0x174[0]==1
 *               early-out; retail PNGs confirm) and shows once the line waits
 *               for Z.  PORT-DEBT(dialogue-arrow-art): the bank lives on the
 *               widget manager (god+0xb8c) and its module is unresolved — the
 *               probe's res_id 1000 numerically collides with sotesd's
 *               parallax mountain sheet (quirk #92).
 *
 * Pure C (no Win32 / ddraw): the cel resolve + GDI text + blits are the
 * caller's sink (main.c game_render_dialogue), like banner.c / scene_fade.c.
 */
#ifndef OPENSUMMONERS_DIALOGUE_H
#define OPENSUMMONERS_DIALOGUE_H

#include <stdint.h>

/* ── the town line-1 bubble geometry (decompile-derived; see header) ── */
#define DIALOGUE_BOX_X        174   /* 0x49c640 speaker-anchor result (measured) */
#define DIALOGUE_BOX_Y        148
#define DIALOGUE_BOX_W        0x198 /* 408 — 0x439690:407 (the portrait layout)  */
#define DIALOGUE_BOX_H        0x70  /* 112 — 0x439690:445                        */

/* content cells, box-relative (0x439690:445-458 builder constants) */
#define DIALOGUE_PORTRAIT_DX  (-0x18) /* -24 → 150 */
#define DIALOGUE_PORTRAIT_DY  (-0x48) /* -72 →  76 */
/* The bubble-TAIL pair (0x49c640): both at x = clamp(speaker-center box-rel,
 * 0x20, W-0x20) - 0x10; frame 9 (the notch over the border) at y = H-0x20,
 * frame 10 (the spike below) at y = H.  The town line-1 speaker (Father)
 * centers at screen 378 -> box-rel 204 -> x 188.  PORT-DEBT(dialogue-trigger)
 * covers deriving this from the live speaker anchor. */
#define DIALOGUE_TAIL_X       0xbc    /* 188 — speaker-anchored (measured 378)  */
#define DIALOGUE_TAIL_NOTCH_Y (DIALOGUE_BOX_H - 0x20) /* 80  — frame 9          */
#define DIALOGUE_TAIL_SPIKE_Y DIALOGUE_BOX_H          /* 112 — frame 10         */
#define DIALOGUE_TAB_DX       0xd8    /* 216 — long-name tab cell (len > 12)     */
#define DIALOGUE_TAB_DY       (-0x20) /* -32 → (390,116); art offset is in-cel   */
#define DIALOGUE_NAME_DX      0xec    /* 236 → 410 (cell 221 + in-cell 15; the   *
                                       * 15 is the one measured-only offset)     */
#define DIALOGUE_NAME_DY      (-9)    /* → 139 (builder local_234 = -9)          */
#define DIALOGUE_TEXT_DX      0x88    /* 136 → 310 (0x439690:409)                */
#define DIALOGUE_TEXT_DY      0x14    /* 20  → 168 (0x439690:400)                */
#define DIALOGUE_ARROW_DX     0x170   /* 368 → 542: 0x410560 +0x7c (232) + text  *
                                       * cell dx (136)                           */
#define DIALOGUE_ARROW_DY     0x5c    /* 92 → 240: +0x80 (72) + text cell dy (20)*/

#define DIALOGUE_LINE_H       28      /* text cell +0x1ac (row pitch, measured)  */
#define DIALOGUE_ADVANCE      7       /* px per glyph (0x48da70 col*7)           */
#define DIALOGUE_ROW_CHARS    0x24    /* 36 — 0x439690:408 (252px = 36*7)        */
#define DIALOGUE_MAX_ROWS     3       /* 0x439690:402                            */

/* colors (COLORREF 0x00bbggrr) */
#define DIALOGUE_NAME_MAIN    0xffffffu /* 0x439690:464                          */
#define DIALOGUE_NAME_SHADOW  0x455f7bu /* 0x439690:465                          */
#define DIALOGUE_BODY_MAIN    0x3e537du /* textout probe (cell +0x180)           */
#define DIALOGUE_BODY_SHADOW  0xa8b9ccu /* textout probe (cell +0x184)           */

/* animation steps (per widget update = one sim-tick / ~2 flips) */
#define DIALOGUE_SCALE_STEP   50      /* +0x54 pop-in (0->1000 in 20 updates)    */
#define DIALOGUE_FADE_STEP    50      /* portrait cross-fade [0x22]=0x32         */
#define DIALOGUE_ARROW_PERIOD 10      /* 0x410560 +0x70                          */
#define DIALOGUE_TYPE_INTERVAL 5      /* *(*DAT_008a6e80+0x248) (measured)       */

/* sprite banks / frames */
#define DIALOGUE_BOX_FRAME_CORNER 9   /* box bank (res 0x456) extra frames       */
#define DIALOGUE_BOX_FRAME_TAIL   10
#define DIALOGUE_TAB_FRAME_LONG   0   /* tab bank (res 0x44a): name len > 12     */
#define DIALOGUE_TAB_FRAME_SHORT  1
#define DIALOGUE_ARROW_FRAME_BASE 0x14 /* god+0xb8c bank frames 20-23 (module    *
                                        * unresolved — PORT-DEBT dialogue-arrow-art) */

/* exe string VAs (story content stays in the user's sotes.exe — read at
 * runtime via exe_data_string; never embedded) */
#define DIALOGUE_VA_TOWN_LINE1   0x86d58cu /* "Ahh, here we are at last!%n..."   */
#define DIALOGUE_VA_NAME_FATHER  0x6b6f80u /* the dramatist-name region          */

/* ── the box state (mirrors the scene-controller + widget subset) ── */
typedef struct {
    int32_t  active;        /* box exists (armed)                            */
    int32_t  scale;         /* node +0x54: 0..1000 pop-in                    */
    int32_t  portrait_fade; /* 0x49c910 [0x21]: 0..1000 once scale==1000     */
    int32_t  reveal;        /* typewriter: glyph records revealed so far     */
    int32_t  type_timer;    /* updates until the next reveal                 */
    int32_t  arrow_idx;     /* +0x72: 0..3 into the anim table               */
    int32_t  arrow_timer;   /* +0x74: counts to DIALOGUE_ARROW_PERIOD        */
    /* the expanded text (rows split on %n + word wrap at ROW_CHARS) */
    char     rows[DIALOGUE_MAX_ROWS][DIALOGUE_ROW_CHARS + 1];
    int32_t  row_count;
    int32_t  total;         /* total revealable chars across rows            */
    char     name[32];      /* speaker name (an exe string, copied)          */
    int32_t  portrait_slot; /* resolved portrait pool-slot (0x49d6e0 +0x84;  */
                            /* g_ar_sprite_slots index); -1 = no portrait.   */
                            /* Set by the caller per line (portrait.c); reset */
                            /* to -1 by dialogue_arm so a re-arm without a    */
                            /* resolve shows none, faithful to +0x20=1.       */
} dialogue_box;

/* Expand a raw script line into wrapped rows: '%n' forces a row break, longer
 * rows word-wrap at the last space at/before ROW_CHARS (the wrap space is
 * consumed).  Returns the row count (<= DIALOGUE_MAX_ROWS; overflow rows are
 * dropped — PORT-DEBT(dialogue-textwrap): the retail expander 0x43cf90 also
 * handles escapes/ruby and >3-row paging, unneeded for the town line 1). */
int dialogue_expand_text(const char *src,
                         char rows[DIALOGUE_MAX_ROWS][DIALOGUE_ROW_CHARS + 1]);

/* Arm the bubble for one line: expands `text`, copies `name`, zeroes the
 * animation state (scale 0 = pop-in starts on the next update). */
void dialogue_arm(dialogue_box *d, const char *name, const char *text);

/* One widget update (one sim-tick): pop-in scale, then (once scale==1000)
 * portrait fade + arrow anim + typewriter reveal. */
void dialogue_step(dialogue_box *d);

int  dialogue_active(const dialogue_box *d);

/* 1 once the pop-in finished (content cells render; 0x48c820's content gate). */
int  dialogue_content_visible(const dialogue_box *d);

/* 1 once the line is fully typed and the box is waiting for the advance input
 * (the arrow shows) — the beat-runner's DIALOGUE-beat completion gate
 * (0x439690:1004, the FUN_0043b980 Z-poll fires only in this "state 2").  The
 * cutscene driver advances to the next line on Z exactly here. */
int  dialogue_awaiting_advance(const dialogue_box *d);

/* The 9-slice frame rect at the current pop-in scale (0x48c820 mode +0x1c==1:
 * w/h scaled by +0x54/1000, centered in the full rect — integer math). */
void dialogue_scaled_rect(const dialogue_box *d,
                          int *x, int *y, int *w, int *h);

/* The portrait blend for the current fade (0x49c910): -1 = plain keyed blit
 * (fade complete), else the ramp_b index 0..19 (rises over fade 0..499, then
 * HOLDS the last step until fade==1000 — the second fade half only matters for
 * the old cel of a cross-fade, absent on the first line). */
int  dialogue_portrait_ramp_index(const dialogue_box *d);

/* The arrow cel frame for the current anim step (base 20 + table {0,1,2,3}). */
int  dialogue_arrow_frame(const dialogue_box *d);

/* Revealed chars of row `r` under the current typewriter state (the render
 * draws rows[r][0..n)). */
int  dialogue_row_revealed(const dialogue_box *d, int r);

/* The per-char pause grade in updates (fitted to the reveal trace; see the
 * header comment): space 1, ','/'.'/'!'/'?' 3*interval, else interval; a row
 * close adds +interval on top of the closing char's grade. */
int  dialogue_char_grade(char c);

#endif /* OPENSUMMONERS_DIALOGUE_H */
