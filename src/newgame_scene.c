/*
 * src/newgame_scene.c — the new-game config scene's run-loop state model.
 * See newgame_scene.h for the pump→action contract and the deferred seams.
 *
 * Each function is the Win32-free distillate of the corresponding slice of
 * FUN_00564780 (the case-0x24 run loop); the input pump (0x565d10) and the
 * picker/box widgets stay in the drive.  Provenance is noted per function.
 */
#include "newgame_scene.h"

#include <string.h>

#include "glyph_text.h"   /* glyph_cell_layout (FUN_0040fa00) */

/*
 * newgame_scene_init — FUN_00564780 case 0x24 setup + the loop's initial
 * state.  newgame_config_build does the grid/node construction (0x411940 +
 * the case-0x24 rows + value fill); here we bind the input gate and leave the
 * cursor at row 0 (menu_ctrl_build zeroes hdr->cursor).
 */
void newgame_scene_init(newgame_scene *s, const newgame_settings *settings)
{
    static const newgame_settings defaults = NEWGAME_SETTINGS_DEFAULT;

    memset(s, 0, sizeof(*s));
    s->settings   = settings ? *settings : defaults;
    s->param_case = 0x24;

    newgame_config_build(&s->grid, &s->node, &s->settings);

    /* Bind the input-ready gate the nav latch consults (menu_list_latch reads
     * grid.sub->ready/enabled).  Built closed (ready 0): the drive ramps it to
     * 1000 over ~20 frames, exactly as the title menu's gate opens (quirk #34
     * / #59).  enabled is set now — the live menu_node's input sub is enabled
     * on spawn; only the ready ramp gates nav. */
    s->sub.enabled = 1;
    s->sub.ready   = 0;
    s->grid.sub    = &s->sub;

    s->started = 1;
}

void newgame_scene_clear(newgame_scene *s)
{
    if (s && s->started) {
        menu_ctrl_clear(&s->grid);
        s->started = 0;
    }
}

int32_t newgame_scene_focused_row(const newgame_scene *s)
{
    return s->grid.list->cursor;            /* *(*(this+0x174)+0x14) */
}

int32_t newgame_scene_focused_kind(const newgame_scene *s)
{
    int32_t r = s->grid.list->cursor;
    return s->grid.rows[r].field0;          /* rows[cursor].field0 (kind) */
}

int32_t newgame_scene_focused_action(const newgame_scene *s)
{
    int32_t r = s->grid.list->cursor;
    return s->grid.rows[r].action;          /* rows[cursor].action (id)   */
}

/*
 * newgame_scene_tooltip — the per-frame tooltip resolution (564780.c:411-595).
 * FUN_00564780 reads the focused row; if it is an option row (kind 0) it calls
 * FUN_00566850(action); if it is an action button (kind 3) it selects a fixed
 * help string by action id, with the 0x1e arm further keyed on the menu case.
 * Only the case-0x24-reachable arms carry real text; the rest are faithful
 * arms of the shared switch (they belong to the screen/sound/etc. cases).
 */
void newgame_scene_tooltip(const newgame_scene *s, char *buf)
{
    int32_t kind   = newgame_scene_focused_kind(s);
    int32_t action = newgame_scene_focused_action(s);

    if (kind == 0) {                        /* *piVar4 == 0 → option row */
        newgame_option_tooltip(action, buf);    /* FUN_00566850 */
        return;
    }

    if (kind == 3) {                        /* *piVar4 == 3 → action button */
        switch (action) {
        case 0x1b:  /* s_Save_changes_and_exit__008a0fe0 */
            strcpy(buf, "Save changes and exit.");
            break;
        case 0x1c:  /* s_Reset_to_default_settings__008a1010 */
            strcpy(buf, "Reset to default settings.");
            break;
        case 0x1d:  /* s_Exit_without_saving__008a0ff8 */
            strcpy(buf, "Exit without saving.");
            break;
        case 0x1e:  /* inner switch(param_1) — case 0x24 only here */
            if (s->param_case == 0x24) {
                /* s_Confirm_options_and_begin_the_ga_008a0f88; the %n is the
                 * engine's line-break escape, expanded by the text builder. */
                strcpy(buf,
                       "Confirm options and begin the game."
                       "%n(Options can be altered after the game starts.)");
            } else {
                buf[0] = '\0';
            }
            break;
        default:
            buf[0] = '\0';
            break;
        }
        return;
    }

    buf[0] = '\0';
}

/*
 * newgame_scene_dispatch — FUN_00564780's run-loop switch on the pump result
 * (564780.c:597-669), specialised to case 0x24.  See the header for the full
 * mapping; the kind-3 path is the Start-Game button (0x568b40(0x1e) is a
 * no-op for 0x1e, so the action switch falls straight to the begin-game
 * teardown), and the kind-0 path opens the option's picker submenu.
 */
newgame_scene_status newgame_scene_dispatch(newgame_scene *s, int32_t pump_result)
{
    switch (pump_result) {
    case NEWGAME_PUMP_MOVE:                 /* 0xd: cursor moved → re-render */
        return NEWGAME_RUNNING;

    case NEWGAME_PUMP_BACK:                 /* 0xb: local_434=0xb, teardown  */
        return NEWGAME_BACK;

    case NEWGAME_PUMP_CONFIRM: {            /* 0xc: act on the focused row    */
        int32_t kind = newgame_scene_focused_kind(s);
        if (kind == 0) {
            return NEWGAME_OPEN_PICKER;     /* 0x567ba0 default arm        */
        }
        if (kind == 3) {                    /* 0x568b40(action)==0 for 0x1e */
            if (newgame_scene_focused_action(s) == NEWGAME_OPT_START_GAME) {
                return NEWGAME_START;       /* case 0x24 → begin-game teardown  */
            }
        }
        return NEWGAME_RUNNING;             /* non-startable confirm: stay      */
    }

    default:
        /* Anything else (e.g. a watchdog 0 or an abort 6) re-iterates the loop
         * in retail; the scene keeps running rather than fabricate an outcome. */
        return NEWGAME_RUNNING;
    }
}

/*
 * newgame_scene_set_option — commit a new option value (the picker's effect /
 * a default-reset) and re-lay that row's value column.  This is the run loop's
 * value-refill block (564780.c:367-385): for the kind-0 row carrying `id`, run
 * newgame_option_value(id, setting) → glyph_cell_layout(col 1).
 */
int newgame_scene_set_option(newgame_scene *s, int32_t id, int32_t setting)
{
    char buf[512];   /* retail local_200 — the value scratch buffer */

    if (id == NEWGAME_OPT_DIFFICULTY) {
        s->settings.difficulty = setting;
    } else if (id == NEWGAME_OPT_AUTO_GUARD) {
        s->settings.auto_guard = setting;
    }

    for (int32_t r = 0; r < s->grid.list->count; r++) {
        if (s->grid.rows[r].field0 == 0 && s->grid.rows[r].action == id) {
            newgame_option_value(id, setting, buf);   /* FUN_00566a80 */
            glyph_cell_layout(&s->grid, r, 1, buf);   /* FUN_0040fa00, col 1 */
            return 1;
        }
    }
    return 0;
}
