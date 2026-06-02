/*
 * src/newgame_menu.c — the new-game config menu builder.  See newgame_menu.h
 * for the interface and docs/findings/new-game-flow.md /
 * docs/findings/text-glyph-pipeline.md for the subsystem map.
 *
 * Each function is a direct translation of the named retail address; the only
 * liberties are the menu_ctrl/menu_node typed model (menu_list.h) in place of
 * raw +offset reads and passing the current option settings in (retail reads
 * them from the settings record at *DAT_008a6e80 + 0xc + id*0x1c).
 */
#include "newgame_menu.h"

#include <string.h>

#include "glyph_text.h"   /* glyph_cell_layout (FUN_0040fa00) */

/*
 * FUN_00412160 (459 B) — append a labelled row to the grid.
 *
 * iVar7 = hdr->count; if it is below the row cap (hdr->alloc_a) the row is
 * appended: count++, the new row's kind/action/flag stamped, the per-column
 * refresh run, and finally the label laid out into column 0.  The refresh
 * loop is byte-for-byte FUN_00411f40's body (re-lay-out obj0, re-zero obj54,
 * re-zero + clamp obj20, each guarded on the sub-object being present and the
 * row in range), so it is delegated to menu_row_finalize; on a fresh append
 * every cell pointer is NULL and the whole refresh is a no-op (quirk #36).
 */
int32_t menu_grid_append(menu_ctrl *c, int32_t kind, int32_t id,
                         const char *label)
{
    menu_list_hdr *hdr = c->list;          /* *(this + 0x174) */
    int32_t row = hdr->count;              /* iVar7 = *(hdr + 0x10) */

    if (row < hdr->alloc_a) {              /* count < row cap (*(hdr + 4)) */
        hdr->count = row + 1;              /* *(hdr + 0x10) = count + 1 */
        menu_row *mr = &c->rows[row];      /* rows + iVar7*0x10 */
        mr->field0 = kind;                 /* row.field0  = kind  (param_2) */
        mr->action = id;                   /* row.action  = id    (param_3) */
        mr->flag8  = 1;                    /* row.flag8   = 1 */

        /* Per-column refresh (FUN_00412160:25-71 == FUN_00411f40).  No-op on
         * a fresh row: every cell sub-object is NULL, so the guarded bodies
         * and their dead operator_new branches (quirk #36) are skipped. */
        if (hdr->alloc_b > 0) {
            menu_row_finalize(c, row);
        }
    } else {
        row = -1;                          /* iVar7 = -1 (row array full) */
    }

    /* Lay out the row label into column 0 (FUN_0040fa00).  On a full grid
     * row == -1 and glyph_cell_layout no-ops on its row >= 0 bounds check. */
    glyph_cell_layout(c, row, 0, label);
    return row;
}

/* String-copy helper standing in for retail's inlined rep-movs copy
 * (FUN_00566570 / FUN_00566a80 each strcpy the resolved label into the
 * caller's buffer).  Buffers are the retail 512/256-byte stack scratch. */
static void copy_label(char *buf, const char *src)
{
    strcpy(buf, src);
}

/*
 * FUN_00566570 (485 B) — option id → label string.  A pure id→string switch;
 * only the new-game config arms are ported (id 3 / id 4).  The default arm
 * copies retail's engine-name buffer &DAT_008a9b6c — modelled as the empty
 * string (the renderer draws nothing for an empty cell).
 */
void newgame_option_label(int32_t id, char *buf)
{
    switch (id) {
    case 3:  copy_label(buf, "Game Difficulty"); break;  /* s_..._008a1380 */
    case 4:  copy_label(buf, "Auto-guard");      break;  /* s_..._008a1374 */
    default: copy_label(buf, "");                break;  /* &DAT_008a9b6c  */
    }
}

/*
 * FUN_00566a80 (2781 B) — option id + current setting → value string.  Only
 * the new-game config arms are ported (id 3 difficulty, id 4 auto-guard).
 * The difficulty values carry clean retail symbols; the On/Off pair are the
 * .rodata DATs (DAT_008a1f44 / PTR_DAT_008a1cb0) the golden confirms render
 * as "On" (the value shown) and "Off" (its toggle counterpart).
 */
void newgame_option_value(int32_t id, int32_t setting, char *buf)
{
    if (id == 3) {                          /* case 3: switch(setting) */
        switch (setting) {
        case 10: copy_label(buf, "1:Easy");      break;  /* s_..._008a1f74 */
        case 20: copy_label(buf, "2:Normal");    break;  /* s_..._008a1f68 */
        case 30: copy_label(buf, "3:Hard");      break;  /* s_..._008a1f60 */
        case 40: copy_label(buf, "4:Expert");    break;  /* s_..._008a1f54 */
        case 50: copy_label(buf, "5:Nightmare"); break;  /* s_..._008a1f48 */
        default: copy_label(buf, "");            break;  /* caseD_13 default */
        }
    } else if (id == 4) {                    /* case 4 */
        switch (setting) {
        case 0:  copy_label(buf, "Off"); break;          /* &PTR_DAT_008a1cb0 */
        case 1:  copy_label(buf, "On");  break;          /* &DAT_008a1f44     */
        default: copy_label(buf, "");    break;
        }
    } else {
        copy_label(buf, "");
    }
}

/*
 * FUN_00566850 (337 B) — option id → tooltip string.  Same shape as
 * FUN_00566570: a pure id→string switch + an inlined rep-movs copy into the
 * caller's buffer, here a strcpy.  Only the new-game config arms are ported;
 * the strings are the full binary literals (the Ghidra symbols truncate).
 */
void newgame_option_tooltip(int32_t id, char *buf)
{
    switch (id) {
    case 3:  /* s_Allows_you_to_configure_game_dif_008a1b88 */
        copy_label(buf,
            "Allows you to configure game difficulty. On harder difficulties, "
            "enemies are stronger and experience gain more rapid.");
        break;
    case 4:  /* s_Auto_guard_will_automatically_gu_008a1b14 */
        copy_label(buf,
            "Auto-guard will automatically guard against incoming attacks if "
            "your character is not engaging in any other action.");
        break;
    default: /* &DAT_008a9b6c — the engine-name buffer (unmodelled → empty) */
        copy_label(buf, "");
        break;
    }
}

/* The display config FUN_0040f3e0 stamps onto every child node (the values
 * menu_list.c's MENU_NODE_COLOR_* carry); the golden confirms the live menu
 * draws with exactly these — normal text 0x3e537d, drop shadow 0xa8b9cc,
 * focused text 0xf08080.  The two label slots hold retail .rodata VAs (a
 * control-coded default label + the empty string) kept verbatim. */
#define NG_COLOR_A   0x3e537du   /* +0x180 normal text          */
#define NG_COLOR_B   0xa8b9ccu   /* +0x184 drop shadow          */
#define NG_COLOR_C   0xf08080u   /* +0x18c/+0x190 focused text  */
#define NG_LABEL0_VA 0x00677b98u /* +0x188 &DAT_00677b98        */
#define NG_LABEL1_VA 0x008090a9u /* +0x194/+0x198 &DAT_008090a9 */

/*
 * FUN_00564780 case 0x24 (+ the grid setup 0x411940 performs) — build the
 * new-game config grid + the render node.  See newgame_menu.h for the step map.
 */
void newgame_config_build(menu_ctrl *grid, menu_node *node,
                          const newgame_settings *settings)
{
    char buf[512];   /* retail local_424 — the label/value scratch buffer */

    /* (1) 0x411940 → menu_ctrl_build(grid, 0x28, 0x18, 3, 2, 3, 0):
     *     a 3-row × 2-col linear (type 0) grid, stride 3 (single page); the
     *     0x28/0x18 land in the node's field_c/field_10 text inset (40, 24). */
    menu_ctrl_build(grid, 0x28, 0x18, /*alloc_a=*/3, /*alloc_b=*/2,
                    /*stride=*/3, /*type=*/0);

    /* (2) case-0x24 entry override: the value column (entry[1]) sits at x
     *     offset 0xa0 (FUN_00564780 case 0x24: `*(entries+0x24) = 0xa0`, the
     *     +0x28/+0x2c/+0x30 zeros).  entry[0] keeps the ctor default pos 0. */
    grid->entries[1].pos     = 0xa0;       /* entries+0x24 */
    grid->entries[1].field4  = 0;          /* entries+0x28 */
    grid->entries[1].extent  = 0;          /* entries+0x2c */
    grid->entries[1].field_c = 0;          /* entries+0x30 */

    /* (3) the three rows (FUN_00564780 case 0x24).  Labels come from
     *     newgame_option_label (FUN_00566570); "Start Game" is a literal. */
    newgame_option_label(NEWGAME_OPT_DIFFICULTY, buf);
    menu_grid_append(grid, 0, NEWGAME_OPT_DIFFICULTY, buf);          /* row 0 */
    newgame_option_label(NEWGAME_OPT_AUTO_GUARD, buf);
    menu_grid_append(grid, 0, NEWGAME_OPT_AUTO_GUARD, buf);          /* row 1 */
    menu_grid_append(grid, 3, NEWGAME_OPT_START_GAME, "Start Game"); /* row 2 */

    /* (4) value fill (the run loop's first block, FUN_00564780:367-385): for
     *     each kind-0 row, lay the option's current value into column 1. */
    for (int32_t r = 0; r < grid->list->count; r++) {
        if (grid->rows[r].field0 == 0) {                /* kind 0 = option row */
            int32_t id = grid->rows[r].action;
            int32_t setting = (id == NEWGAME_OPT_DIFFICULTY) ? settings->difficulty
                            : (id == NEWGAME_OPT_AUTO_GUARD) ? settings->auto_guard
                            : 0;
            newgame_option_value(id, setting, buf);     /* FUN_00566a80 */
            glyph_cell_layout(grid, r, 1, buf);         /* FUN_0040fa00, col 1 */
        }
    }

    /* (5) the render node — the fields 0x411940's menu_node_build (display
     *     config + field_1ac) + menu_ctrl_build (field_c/field_10) leave on the
     *     rendered child.  On-target node and grid are one object; the host
     *     splits them, so mirror grid's container arrays + the inset/colours. */
    node->ctrl_list    = grid->list;
    node->ctrl_entries = grid->entries;
    node->ctrl_rows    = grid->rows;
    node->field_c      = 0x28;             /* +0x0c — text inset x (= 40) */
    node->field_10     = 0x18;             /* +0x10 — text inset y (= 24) */
    node->field_14     = 0;                /* +0x14 — ruby pass off       */
    node->field_18     = 0;                /* +0x18 */
    node->field_1ac    = 0x1c;             /* +0x1ac — row pitch (28)     */
    node->color0 = NG_COLOR_A;             /* +0x180 */
    node->color1 = NG_COLOR_B;             /* +0x184 */
    node->label0 = NG_LABEL0_VA;           /* +0x188 */
    node->color2 = NG_COLOR_C;             /* +0x18c */
    node->color3 = NG_COLOR_C;             /* +0x190 */
    node->label1 = NG_LABEL1_VA;           /* +0x194 */
    node->label2 = NG_LABEL1_VA;           /* +0x198 */
    node->color4 = NG_COLOR_A;             /* +0x19c */
    node->color5 = NG_COLOR_B;             /* +0x1a0 */
}
