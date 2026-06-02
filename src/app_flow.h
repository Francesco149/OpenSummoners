/*
 * src/app_flow.h — the post-title application-flow dispatch.
 *
 * Retail's boot driver FUN_00562ea0 (@ 0x562ea0) ends in an outer
 * `do { } while(true)` loop (decompiled 562ea0.c:658-735) that:
 *
 *   1. runs the title menu (FUN_0056aea0) when DAT_008a6e6c == 0,
 *   2. switches on its return code (the committed menu-action id), and
 *   3. either leaves the loop (Exit), enters a sub-scene (new-game /
 *      continue / options / bonus), or — for any other code — falls through
 *      and the loop re-runs the title to re-display the menu.
 *
 * Re-displaying passes the title runner a NON-ZERO arg (`local_164`, set to 1
 * after the first iteration) — the port's title_scene `skip_intro` flag.  It
 * does NOT auto-skip: the runner restarts at phase 0 and replays the intro
 * logos/sparkles each time (56aea0.c re-enters FUN_0056aea0 with local_64=0).
 * `skip_intro` only *enables a phase-0 skip-press* — the skip-to-menu jump at
 * 56aea0.c:177 is gated on a fresh recent button press (:182), and with arg==0
 * phase 0 ignores presses (only phases 1+ honour them).  So a returning player
 * sees the intro again unless they press a key, just like retail.
 *
 * This module ports the pure result→action mapping of that switch so the
 * categorisation is faithful to retail and unit-testable in isolation.  The
 * Win32-coupled lifecycle (tearing down + rebuilding the title drive,
 * requesting process shutdown) stays in main.c, keyed off this enum.
 *
 * The sub-scene runners themselves are NOT ported yet (they are gated on the
 * glyph/text pipeline + font registration); their entry points are named here
 * by bare VA only, per the port-ledger convention, so they don't inflate the
 * ledger as if ported.
 *
 * Pure integer mapping, no Win32 surface — builds and runs under the host
 * unit suite unchanged.
 */
#ifndef OPENSUMMONERS_APP_FLOW_H
#define OPENSUMMONERS_APP_FLOW_H

#include <stdint.h>

/* The category a title-menu return code dispatches to.  Numbered from 0; the
 * values are an internal contract (not retail constants) — the retail meaning
 * is the title_result that maps here (see app_flow_dispatch). */
typedef enum app_flow_action {
    APP_FLOW_REENTER_TITLE = 0, /* case 0 + default: attract/idle → re-show title   */
    APP_FLOW_NEW_GAME,          /* 0x1a Start  → 0x564160 + 0x56cd20 + 0x59ec30      */
    APP_FLOW_DEMO_START,        /* 0x1b        → 0x59ec30(0x2724,0,0)                 */
    APP_FLOW_CONTINUE,          /* 0x1c        → 0x56a670 (load-game menu)            */
    APP_FLOW_OPTIONS,           /* 0x1d        → 0x40a5d0 + 0x568de0(0) (options)     */
    APP_FLOW_BONUS,             /* 0x1e        → 0x583fe0 (bonus menu)                */
    APP_FLOW_EXIT,              /* 6 / 8       → leave the game loop                  */
    APP_FLOW_EXIT_9,            /* 9           → leave the game loop, code 9          */
} app_flow_action;

/* FUN_00562ea0 (tail switch, 562ea0.c:684-734) — categorise the title runner's
 * return code.  Codes with no explicit case (and case 0) fall to the loop's
 * implicit "re-run the title" behaviour → APP_FLOW_REENTER_TITLE. */
app_flow_action app_flow_dispatch(int32_t title_result);

#endif /* OPENSUMMONERS_APP_FLOW_H */
