/*
 * src/app_flow.c — post-title application-flow dispatch (see app_flow.h).
 *
 * Ports the result→action mapping of FUN_00562ea0's outer-loop switch
 * (decompiled 562ea0.c:684-734):
 *
 *     switch (unaff_ESI) {            // unaff_ESI = the title runner's return
 *     case 0:    ... attract/demo cycle; loop re-runs the title
 *     case 6:
 *     case 8:    goto LAB_00563e3c;   // return iVar11 — leave the loop
 *     case 9:    iVar11 = 9; goto LAB_00563e3c;
 *     case 0x1a: 0x564160(); ... 0x56cd20(); ... 0x59ec30(0,0,0x3f2);
 *     case 0x1b: 0x59ec30(0x2724,0,0);
 *     case 0x1c: 0x56a670(...); ... 0x59ec30(...);
 *     case 0x1d: 0x40a5d0(...); 0x568de0(0);
 *     case 0x1e: 0x583fe0();
 *     }                              // cases that break → loop re-runs title
 *
 * (Unported sub-scene callees are written as bare VAs above, not FUN_ symbols,
 * so they don't inflate the port ledger — only this tail switch is ported.)
 *
 * Cases 0x1a..0x1e and 0x1b break out of the switch, after which the
 * `do { } while(true)` re-runs the title runner — i.e. once each sub-scene
 * returns (or is cancelled) control comes back to the title.  Only 6/8/9
 * leave the loop.  case 0 (and any unhandled code) does no work and the loop
 * re-runs the title as well, so it collapses to the same re-display action.
 */

#include "app_flow.h"

app_flow_action app_flow_dispatch(int32_t title_result)
{
    switch (title_result) {
    case 6:
    case 8:
        return APP_FLOW_EXIT;
    case 9:
        return APP_FLOW_EXIT_9;
    case 0x1a:
        return APP_FLOW_NEW_GAME;
    case 0x1b:
        return APP_FLOW_DEMO_START;
    case 0x1c:
        return APP_FLOW_CONTINUE;
    case 0x1d:
        return APP_FLOW_OPTIONS;
    case 0x1e:
        return APP_FLOW_BONUS;
    case 0:
    default:
        /* case 0 does an attract-cycle side effect then falls through; every
         * other unhandled code is a no-op.  Both leave the title to be re-run
         * by the outer loop, so they map to the same re-display action. */
        return APP_FLOW_REENTER_TITLE;
    }
}
