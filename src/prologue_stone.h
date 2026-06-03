/*
 * src/prologue_stone.h — the Elemental-Stone intro cutscene state model
 * (the Win32-free heart of FUN_0056cd20).
 *
 * FUN_0056cd20 is the timed gem-on-black cutscene that plays between the
 * new-game config menu and the game proper.  The boot driver FUN_00562ea0
 * (case 0x1a, Start) runs it as the second of three blocking calls:
 *
 *     0x564160()      → config menu + ~19-frame dismiss anim   (ported: newgame_*)
 *     FUN_0056cd20()  → THIS cutscene; returns 6 on abort      (this module)
 *     0x59ec30(0,0,0x3f2) → game proper (map 0x3f2)            (deferred: in-game)
 *
 * The cutscene draws ONLY sprites — a glowing Elemental Stone (gem) rising on a
 * black field, a soft aura behind it, and the scrolling story NARRATION.  The
 * narration is NOT GDI text: it is pre-baked into sprite bank 0x448 as a 24-tile
 * strip (6 lines × 4 horizontal tiles), animated as the "caption" grid below —
 * confirmed live (the gem rises while "Elemental Stones: stones imbued with the
 * power of an Elemental Spirit, which grant…" scrolls up).
 *
 * Factored like title_scene (pure) vs title_drive (Win32): this file ports the
 * per-tick UPDATE state machine and the render-descriptor build — pure integer
 * arithmetic over the cutscene's local state — and host-tests them.  The Win32
 * frame pump (the 16ms-step iVar9 timing loop), the real zdd_alpha_blit draws,
 * and the input-ring feed live in prologue_drive.{c,h}.
 *
 * Sprite banks (already registered at boot by ar_register_main_sprites, group 4;
 * pool idx = (retail_BSS_addr − 0x008a7640)/4):
 *   gem      g_ar_sprite_slots[3]  DAT_008a764c  res 0x4a2  144×108  (35-frame loop)
 *   aura     g_ar_sprite_slots[1]  DAT_008a7644  res 0x49f  224×224  (2-frame toggle)
 *   caption  g_ar_sprite_slots[2]  DAT_008a7648  res 0x448  152×40   (24-frame strip)
 *
 * Audio is entangled but every cue is null-guarded (DAT_008a93e4 ZDM music,
 * DAT_008a7608 SP mgr) → skipped pre-audio (milestone 3); the visuals run
 * standalone.  See docs/findings/prologue-stone-intro.md for the full survey.
 */
#ifndef OPENSUMMONERS_PROLOGUE_STONE_H
#define OPENSUMMONERS_PROLOGUE_STONE_H

#include <stdint.h>

#include "input.h"   /* input_mgr — the per-tick abort/beat ring poll */

/* ─── the 6 caption entries (FUN_0056cd20 local_46[]) ─────────────────────
 *
 * Retail packs 6 entries × 6 ushorts on the stack; the live fields are STATE
 * (offset 0), level (offset 2), sub (offset 4), and a 32-bit y (offset 8).
 * Per-tick state machine (each entry):
 *   0 wait    : sub>0 → sub--; sub==0 → state 1   (sub seeded per-entry = stagger)
 *   1 grow    : sub 0→10 then reset + level 0→0x14; level==0x14 → state 2
 *   2 descend : y < 0x2ee1 → state 3
 *   3 shrink  : sub 0→10 then reset + level 0x14→0; level==0 → state 4
 *   4 dead    : (no-op)
 *   states 1..3 also: y -= 0x10
 * On the 3rd beat (exit), each entry is forced toward shrink: state = 4-(state!=0). */
typedef struct prologue_caption {
    uint16_t state;   /* +0  animation state 0..4                              */
    uint16_t level;   /* +2  brightness band 0..0x14 (the ramp index)          */
    uint16_t sub;     /* +4  state-0 wait countdown / states-1,3 0..10 counter */
    int32_t  y;       /* +8  fixed-point descent position (−0x10/tick), /100 px */
} prologue_caption;

#define PROLOGUE_CAPTION_LINES 6

/* ─── the cutscene state (FUN_0056cd20 locals) ───────────────────────────── */
typedef struct prologue_stone {
    int32_t  watchdog;     /* uVar16  0x1130 → −1/tick; 0 = finish; ≤200 after exit */
    int16_t  start_delay;  /* sVar4   10 → counts down before the gem appears       */
    int32_t  sound_cue;    /* local_90 0xed8 → −1/tick; 0 fires a BGM cue (audio)   */
    uint32_t beats;        /* uVar8   fresh key presses; >2 (3rd) → begin exit      */
    int      exiting;      /* bVar5   set on the 3rd beat → gem fade-out + clamp     */

    int16_t  gem_phase;    /* sVar6   0 fade-in / 1 hold / 2 fade-out               */
    int32_t  gem_fade;     /* local_bc 0→600 (alpha); fade-out −1, or −4 when exiting */
    int32_t  hold;         /* local_9c 0→0xc7f dwell in phase 1                      */
    int32_t  rise_pos;     /* local_a0 −4000 → += rise_vel/100 (the gem/aura y)      */
    int32_t  rise_vel;     /* local_88 800 → −10/tick (floored 0) once rise_pos>16000 */

    uint16_t gem_frame;    /* local_94 0..0x22, %0x23 (35-frame gem loop)           */
    uint16_t gem_sub;      /* uVar19  0..6 gate; on wrap bumps gem_frame            */
    uint16_t aura_frame;   /* uVar17  0/1 toggle                                    */
    uint16_t aura_sub;     /* uVar7   0..6 gate; on wrap toggles aura_frame         */

    prologue_caption caption[PROLOGUE_CAPTION_LINES];
} prologue_stone;

/* What one UPDATE tick resolves to. */
typedef enum prologue_status {
    PROLOGUE_RUNNING = 0,  /* keep the cutscene loop going                         */
    PROLOGUE_ABORT,        /* abort (id 0x22) pressed → return 6 (skip to title)   */
    PROLOGUE_DONE,         /* watchdog elapsed (or exit fade finished) → enter game */
} prologue_status;

/* ─── FUN_0056cd20:50-102 — init ─────────────────────────────────────────
 *
 * Reset all state to the cutscene's start values and seed the 6 caption entries
 * (y=32000 default, per-entry `sub` stagger {0x2a8,0x3d4,0x62c,0x758,0x884,0xa46}).
 * Pure; no allocation. */
void prologue_stone_init(prologue_stone *ps);

/* ─── FUN_0056cd20 — one UPDATE tick (the iVar9==2 body) ──────────────────
 *
 * Advance the cutscene one 16ms animation step at wall-clock `now`, polling
 * `in` for the abort (id 0x22, checked first → PROLOGUE_ABORT) and the beat
 * (any fresh press → consume + flush + beats++; 3rd beat begins the exit fade).
 * Returns PROLOGUE_DONE when the watchdog reaches 0 (after the exit clamp this
 * is ~200 ticks past the 3rd beat).  `in` may be NULL (no input → time-only).
 * Faithful to the visual half of 0x56cd20's update; audio cues are no-ops. */
prologue_status prologue_stone_update(prologue_stone *ps, input_mgr *in,
                                      uint32_t now);

/* ─── render descriptor (FUN_0056cd20 iVar9==1 body) ─────────────────────── */
typedef struct prologue_draw {
    int draw;       /* 1 = blit this element this frame                          */
    int frame;      /* sprite frame index                                        */
    int x, y;       /* LOGICAL blit position; the drive adds the decoded sprite's */
                    /* trim offset (+0xc/+0x10) before the real blit             */
    int ramp_idx;   /* alpha-shade ramp index.  gem/caption: RAW ramp_b index    */
                    /* (gem peaks at 16; caption == level, can reach 0x14, where  */
                    /* the drive falls to the keyed blit).  aura: ramp_a index,   */
                    /* pre-clamped to [0,19] (always alpha-blit, no fallback).    */
} prologue_draw;

#define PROLOGUE_CAPTION_DRAWS 24   /* 6 entries × 4 columns                     */

typedef struct prologue_render_out {
    prologue_draw gem;                              /* slot[3], frame gem_frame  */
    prologue_draw aura;                             /* slot[1], frame aura_frame */
    prologue_draw caption[PROLOGUE_CAPTION_DRAWS];  /* slot[2], frame = draw idx */
} prologue_render_out;

/* Build this frame's draw list from the cutscene state.
 *
 * Mirrors the iVar9==1 render branch: nothing draws until start_delay==0; the
 * gem draws when gem_fade≥2, the aura when gem_fade≥1, and each caption draw
 * when its entry's level≠0.  Positions are the retail logical bases (gem 0xf8 /
 * aura 0xd0 / caption col 0x10+col·0x98) plus the per-element y; the caption
 * grid is entry-major (entry e → frames 4e..4e+3 across columns 0..3, all at
 * the entry's y/level).  Pure; out is fully written (draw=0 where idle). */
void prologue_stone_render(const prologue_stone *ps, prologue_render_out *out);

#endif /* OPENSUMMONERS_PROLOGUE_STONE_H */
