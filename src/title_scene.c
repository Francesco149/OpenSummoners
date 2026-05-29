/*
 * src/title_scene.c — intro-phase / menu-fade state machine, ported
 * from the `switch(local_64)` core of FUN_0056aea0 (title scene runner).
 *
 * Pure integer arithmetic; the only outward signals are reported via
 * the title_fade_step_out descriptor.  Per-function provenance and the
 * disassembly-level reasoning live in title_scene.h plus the per-case
 * comments below.
 *
 * Phase table (local_64), as recovered from the switch at 0x56b153 and
 * the per-phase jump-table handlers at 0x56bfa4 (see title-scene.md):
 *
 *   phase  what it animates                              advance when
 *   ----   ------------------------------------------    ---------------------
 *    0     studio-logo fade-in   (fade += 20)            fade reaches 1000
 *    1     hold studio logo      (tick++)                tick reaches 50
 *    2     studio-logo fade-out  (fade -= 20)            fade reaches 0  → seg cue
 *    3     title-logo fade-in    (fade += 10)            fade reaches 1000
 *    4     hold title logo       (tick++)                tick reaches 20
 *    5     "press button" in     (fade += 100, tick++)   tick reaches 40
 *    6     hold "press button"   (fade += 10,  tick++)   tick reaches 120
 *    7     sparkle flourish      (fade += 20, spawn)     tick reaches 90
 *    8     menu fade-in          (fade→1000, menu+=50)   menu_fade reaches 1000 → 9
 *    9     menu fade-out         (menu -= 50)            menu_fade reaches 0    → 8
 *   10     scene fade-out        (fade -= 20)            fade reaches 0  → EXIT
 *
 * Phases 8 and 9 are the steady-state menu (the engine's `default:`
 * switch arm): each frame first ramps the *main* fade to 1000, and only
 * once it is saturated does the menu cross-fade (menu_fade) breathe up
 * (8) and down (9).  The menu input/draw work that also happens every
 * frame in those phases is deferred (see title_scene.h).
 */
#include "title_scene.h"

/* Saturating subtract toward zero: max(x - d, 0).
 *
 * Retail implements this branchlessly at 0x56ba63 (case 2 / case 10
 * fade-out) and 0x56b57c (case 9 menu fade-out) as:
 *     lea  eax, [x - d]
 *     setl bl            ; bl = (x - d) < 0
 *     dec  ebx           ; -1 if >=0, 0 if <0
 *     and  ebx, eax      ; (x-d) if >=0, else 0
 * which is exactly max(x - d, 0) on signed values. */
static int32_t sat_sub(int32_t x, int32_t d)
{
    int32_t v = x - d;
    return v < 0 ? 0 : v;
}

void title_fade_state_init(title_fade_state *s)
{
    s->phase     = 0;
    s->fade      = 0;
    s->tick      = 0;
    s->menu_fade = 0;
}

title_fade_action title_fade_step(title_fade_state *s, title_fade_step_out *fx)
{
    fx->set_next_segment  = 0;
    fx->spawn_sparkle     = 0;
    fx->sparkle_intensity = 0;

    switch (s->phase) {

    /* case 0 @ 0x56b15e — studio-logo fade-in, +20/frame to 1000. */
    case 0:
        if (s->fade < 1000) {
            s->fade += 0x14;
            if (s->fade > 1000) s->fade = 1000;
        } else {
            s->phase = 1;
        }
        return TITLE_FADE_CONTINUE;

    /* case 1 @ 0x56b2c6 — hold studio logo for 50 frames. */
    case 1:
        if (s->tick < 0x32) s->tick++;
        else                s->phase = 2;
        return TITLE_FADE_CONTINUE;

    /* case 2 @ 0x56b2e6 — studio-logo fade-out, -20/frame to 0.  When
     * it reaches 0: reset tick, advance to phase 3, and fire the BGM
     * "SetNextSegment" cue (the 0x5bbc60/_90/_cd0/_20 + 0x5bcb80
     * block at 0x56b2f6, logged "Title Menu - SetNextSegment"). */
    case 2:
        if (s->fade > 0) {
            s->fade = sat_sub(s->fade, 0x14);
        } else {
            s->fade  = 0;
            s->tick  = 0;
            s->phase = 3;
            fx->set_next_segment = 1;
        }
        return TITLE_FADE_CONTINUE;

    /* case 3 @ 0x56b346 — title-logo fade-in, +10/frame to 1000. */
    case 3:
        if (s->fade < 1000) {
            s->fade += 10;
            if (s->fade > 1000) s->fade = 1000;
        } else {
            s->phase = 4;
        }
        return TITLE_FADE_CONTINUE;

    /* case 4 @ 0x56b36e — hold title logo for 20 frames, then reset
     * fade and advance to the "press button" phase. */
    case 4:
        if (s->tick < 0x14) {
            s->tick++;
        } else {
            s->fade  = 0;
            s->phase = 5;
            s->tick  = 0;
        }
        return TITLE_FADE_CONTINUE;

    /* case 5 @ 0x56b39a — "press button" fade-in, +100/frame (cap
     * 1000) while holding 40 frames, then phase 6. */
    case 5:
        if (s->fade < 1000) {
            int32_t v = s->fade + 100;
            s->fade = (v < 0x3e9) ? v : 1000;   /* min(fade+100, 1000) */
        }
        if (s->tick < 0x28) {
            s->tick++;
        } else {
            s->fade  = 0;
            s->phase = 6;
            s->tick  = 0;
        }
        return TITLE_FADE_CONTINUE;

    /* case 6 @ 0x56b46e — hold "press button" + ambient, +10/frame
     * (cap 1000) over 120 frames, then phase 7. */
    case 6:
        if (s->fade < 1000) {
            int32_t v = s->fade + 10;
            s->fade = (v < 0x3e9) ? v : 1000;   /* min(fade+10, 1000) */
        }
        if (s->tick < 0x78) {
            s->tick++;
        } else {
            s->fade  = 0;
            s->phase = 7;
            s->tick  = 0;
        }
        return TITLE_FADE_CONTINUE;

    /* case 7 @ 0x56b4ea — sparkle flourish.  fade += 20; while the
     * post-increment fade is < 850 (0x352) and still < 1001, request a
     * sparkle-particle spawn (retail 0x56c070) at intensity
     * (fade*0xe0)/900 + 0xc0; clamp fade to 1000 once it passes 1000.
     * Holds 90 frames, then hands off to the menu (phase 8). */
    case 7:
        if (s->fade < 1000) {
            s->fade += 0x14;
            if (s->fade < 0x3e9) {
                if (s->fade < 0x352) {
                    fx->spawn_sparkle     = 1;
                    fx->sparkle_intensity = (s->fade * 0xe0) / 900 + 0xc0;
                }
            } else {
                s->fade = 1000;
            }
        }
        if (s->tick < 0x5a) {
            s->tick++;
        } else {
            s->fade  = 0;
            s->phase = 8;
            s->tick  = 0;
        }
        return TITLE_FADE_CONTINUE;

    /* case 10 @ 0x56ba2e — scene fade-out at end of flow.  The menu
     * teardown (local_60/local_54/obj+0x50 reset) is the caller's job
     * (it owns the menu controller); here we only run the fade-out.
     * When fade has reached 0 the scene is done (retail jumps to the
     * cleanup+return path at 0x56bf2e). */
    case 10:
        if (s->fade < 1) return TITLE_FADE_EXIT;
        s->fade = sat_sub(s->fade, 0x14);
        return TITLE_FADE_CONTINUE;

    /* default @ 0x56b546 — phases 8 and 9 (steady-state menu).  Ramp
     * the main fade to 1000 first; only once saturated does the menu
     * cross-fade run: phase 8 ramps menu_fade up by 50 (→ phase 9 at
     * 1000), phase 9 ramps it down by 50 (→ phase 8 at 0).  After the
     * fade math, the caller runs the deferred menu/input update. */
    default:
        if (s->fade < 1000) {
            s->fade += 0x14;
            if (s->fade > 1000) s->fade = 1000;
        } else if (s->phase == 8) {
            if (s->menu_fade < 1000) {
                s->menu_fade += 0x32;
                if (s->menu_fade > 1000) s->menu_fade = 1000;
            } else {
                s->phase = 9;
            }
        } else if (s->phase == 9) {
            if (s->menu_fade < 1) s->phase = 8;
            else                  s->menu_fade = sat_sub(s->menu_fade, 0x32);
        }
        return TITLE_FADE_MENU;
    }
}
