/*
 * tests/test_app_flow.c — host tests for the post-title dispatch mapping
 * (app_flow.c, ported from FUN_00562ea0's outer-loop switch, 562ea0.c:684-734).
 *
 * Pins each title-menu return code to its retail-faithful action category:
 *   6,8 → EXIT   9 → EXIT_9   0x1a → NEW_GAME   0x1b → DEMO_START
 *   0x1c → CONTINUE   0x1d → OPTIONS   0x1e → BONUS
 *   0 and every other (unhandled) code → REENTER_TITLE (the loop's fall-through
 *   that re-runs the title).
 */
#include "t.h"
#include "app_flow.h"

int test_app_flow_dispatch_codes(void)
{
    /* The explicit switch cases (562ea0.c). */
    T_ASSERT_EQ_I(app_flow_dispatch(6),    APP_FLOW_EXIT);
    T_ASSERT_EQ_I(app_flow_dispatch(8),    APP_FLOW_EXIT);
    T_ASSERT_EQ_I(app_flow_dispatch(9),    APP_FLOW_EXIT_9);
    T_ASSERT_EQ_I(app_flow_dispatch(0x1a), APP_FLOW_NEW_GAME);
    T_ASSERT_EQ_I(app_flow_dispatch(0x1b), APP_FLOW_DEMO_START);
    T_ASSERT_EQ_I(app_flow_dispatch(0x1c), APP_FLOW_CONTINUE);
    T_ASSERT_EQ_I(app_flow_dispatch(0x1d), APP_FLOW_OPTIONS);
    T_ASSERT_EQ_I(app_flow_dispatch(0x1e), APP_FLOW_BONUS);

    /* case 0 (attract-cycle) collapses to a title re-display. */
    T_ASSERT_EQ_I(app_flow_dispatch(0), APP_FLOW_REENTER_TITLE);

    /* Every unhandled code hits the switch default → re-display the title. */
    T_ASSERT_EQ_I(app_flow_dispatch(1),     APP_FLOW_REENTER_TITLE);
    T_ASSERT_EQ_I(app_flow_dispatch(7),     APP_FLOW_REENTER_TITLE);
    T_ASSERT_EQ_I(app_flow_dispatch(0x19),  APP_FLOW_REENTER_TITLE);
    T_ASSERT_EQ_I(app_flow_dispatch(0x1f),  APP_FLOW_REENTER_TITLE);
    T_ASSERT_EQ_I(app_flow_dispatch(0x99),  APP_FLOW_REENTER_TITLE);
    T_ASSERT_EQ_I(app_flow_dispatch(-1),    APP_FLOW_REENTER_TITLE);

    return 0;
}
