/*
 * src/newgame_picker.c — the new-game config scene's option picker submenu.
 * See newgame_picker.h for the interface, the FUN_00567ba0 step map, and the
 * documented reconstructions (the decompiler dropped the picker's __thiscall
 * arg lists; the model rebuilds them from the callees' contracts).
 *
 * Each function is the Win32-free distillate of the corresponding slice of
 * FUN_00567ba0; the input pump (0x565d10) stays in the drive.
 */
#include "newgame_picker.h"

#include <string.h>

#include "glyph_text.h"    /* glyph_cell_layout (FUN_0040fa00) */
#include "newgame_scene.h" /* NEWGAME_PUMP_* result codes      */

/*
 * FUN_00568320 (964 B) — option id → value-code list.  Only the new-game
 * config arms are ported (id 3 difficulty, id 4 auto-guard); the rest of the
 * switch serves the screen/sound/team menus and several arms read god-object
 * globals (stage counts, unlock flags) the new-game scene never touches.
 *
 * id 3 (568320.c:12-26): seeds {10,20,30}, then bVar3 = (unlock != 0) + 4.
 * The two appends are `if (3 < bVar3)` → +40 (always, since bVar3 >= 4) and
 * `if (4 < bVar3)` → +50 (only when unlock != 0 → bVar3 == 5).  So 4 choices
 * locked, 5 unlocked.  id 4 (568320.c:27-41): {0,1}.
 */
int32_t newgame_picker_values(int32_t id, int32_t *out, int32_t difficulty_unlock)
{
    if (id == NEWGAME_OPT_DIFFICULTY) {        /* case 3 */
        int32_t n = 3;
        out[0] = 10;
        out[1] = 20;
        out[2] = 30;
        /* bVar3 = (unlock != 0) + 4; the byte arithmetic is exact in retail. */
        uint8_t lim = (uint8_t)((difficulty_unlock != 0) + 4);
        if (3 < lim) {                          /* always: lim is 4 or 5 */
            out[3] = 40;
            n = 4;
        }
        if (4 < lim) {                          /* unlock only: lim == 5 */
            out[n] = 50;
            return n + 1;
        }
        return n;
    }

    if (id == NEWGAME_OPT_AUTO_GUARD) {        /* case 4 */
        out[0] = 0;
        out[1] = 1;
        return 2;
    }

    return 0;                                   /* unported arms / default */
}

/*
 * FUN_00419900 — seek the cursor to the row matching (kind, id), then page it
 * into view.  The picker calls it as FUN_00419900(0, current_value): find the
 * kind-0 value row whose action == current_value and make it the selection.
 * No-op (cursor stays at row 0) if the value isn't in the list.  Faithful to
 * 0x419900's row search + the sel2 page-top recompute (== menu_list_scroll).
 */
static void picker_seek_value(newgame_picker *p, int32_t value)
{
    menu_ctrl *c = &p->grid;
    for (int32_t r = 0; r < c->list->count; r++) {
        if (c->rows[r].field0 == 0 && c->rows[r].action == value) {
            c->list->cursor = r;
            menu_list_scroll_into_view(c);   /* FUN_004192b0, the +0x18 recompute */
            return;
        }
    }
}

/*
 * newgame_picker_init — FUN_00567ba0 default arm (567ba0.c:29-45 + the seek).
 * Build a 1-column value grid for `option_id`, lay each value's label
 * (newgame_option_value = FUN_00566a80), and seek the cursor to current_value.
 * Returns 0 (run nothing) when the option has no choices, mirroring retail's
 * `if (uVar2 == 0) return 0`.
 */
int newgame_picker_init(newgame_picker *p, int32_t option_id,
                        int32_t current_value, int32_t difficulty_unlock)
{
    char buf[512];   /* retail local_624 — the value-label scratch */

    memset(p, 0, sizeof(*p));
    p->option_id = option_id;
    p->count     = newgame_picker_values(option_id, p->values, difficulty_unlock);
    if (p->count == 0)
        return 0;                               /* FUN_00568320 == 0 → return 0 */

    /* FUN_00411940(this, 0x120,0x80,0x100, 0, 2, …): a single-column value grid
     * box at (288,128) width 256.  The row/column allocations (FUN_00412160's
     * appends) are `count` rows × 1 column, one page (stride = count).  The
     * text inset / position fields feed only the render (an OPEN geometry gate,
     * see the header); nav reads only count/cursor/stride. */
    menu_ctrl_build(&p->grid, /*f_c=*/0x28, /*f_10=*/0x18,
                    /*alloc_a=*/p->count, /*alloc_b=*/1,
                    /*stride=*/p->count, /*type=*/0);

    /* Append one kind-0 row per value, label = its value string.  The value
     * code is stamped as the row's action id so the seek + commit can read it
     * (the reconstructed FUN_00412160/FUN_005657f0 contract, see the header). */
    for (int32_t i = 0; i < p->count; i++) {
        newgame_option_value(option_id, p->values[i], buf);   /* FUN_00566a80 */
        menu_grid_append(&p->grid, /*kind=*/0, /*id=*/p->values[i], buf);
    }

    /* Bind the input gate (built closed; the drive ramps ready 0→1000, like the
     * parent scene) and seed the render node from grid's arrays. */
    p->sub.enabled = 1;
    p->sub.ready   = 0;
    p->grid.sub    = &p->sub;
    p->node.ctrl_list    = p->grid.list;
    p->node.ctrl_entries = p->grid.entries;
    p->node.ctrl_rows    = p->grid.rows;

    /* FUN_00419900(0, current_value): open on the current selection. */
    picker_seek_value(p, current_value);

    p->started = 1;
    return 1;
}

void newgame_picker_clear(newgame_picker *p)
{
    if (p && p->started) {
        menu_ctrl_clear(&p->grid);
        p->started = 0;
    }
}

int32_t newgame_picker_focused_value(const newgame_picker *p)
{
    int32_t r = p->grid.list->cursor;
    return p->grid.rows[r].action;              /* the value code stamped above */
}

/*
 * newgame_picker_dispatch — the picker's nav loop (567ba0.c:237-251):
 *   0xb → return 0          (cancel; the option is left unchanged)
 *   0xc → FUN_005657f0(...) (commit the focused value), return 0xc
 *   0xd → re-iterate        (cursor moved → re-render)
 * any other code re-iterates the loop (retail loops on != 0xb/0xc).
 */
newgame_picker_status newgame_picker_dispatch(newgame_picker *p, int32_t pump_result)
{
    switch (pump_result) {
    case NEWGAME_PUMP_BACK:                     /* 0xb */
        return NEWGAME_PICKER_CANCEL;

    case NEWGAME_PUMP_CONFIRM:                  /* 0xc: FUN_005657f0 commit */
        p->chosen = newgame_picker_focused_value(p);
        return NEWGAME_PICKER_COMMIT;

    case NEWGAME_PUMP_MOVE:                     /* 0xd: re-render */
    default:
        return NEWGAME_PICKER_RUNNING;
    }
}
