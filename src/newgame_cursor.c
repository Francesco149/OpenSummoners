/*
 * src/newgame_cursor.c — the new-game menu selection cursor (see
 * newgame_cursor.h).  Faithful port of FUN_0048d940's type-1 plain-branch arm.
 */

#include <stddef.h>

#include "newgame_cursor.h"

int newgame_cursor_frame_id(int anim_idx)
{
    /* base + frames[idx % count] with the canonical {0,1,2,3} frame list
     * (FUN_0048d940: uVar4 = node+0x2c + node+0x2e[node+0x72]).  The list is
     * the identity 0,1,2,3, so this collapses to base + (idx mod count). */
    int n = NEWGAME_CURSOR_FRAME_COUNT;
    int i = anim_idx % n;
    if (i < 0) i += n;
    return NEWGAME_CURSOR_BASE_FRAME + i;
}

void newgame_cursor_base(int box_x, int box_y,
                         int cursor, int sel2, int pitch,
                         int *out_x, int *out_y)
{
    /* FUN_0048d940 type-1:
     *   x = node+0x7c + node+0xc + parent_x
     *   y = (cursor - sel2) * pitch + node+0x80 + node+0x10 + parent_y
     * collapsed to the golden-calibrated box-relative form (the port has no
     * box-widget sub-node to source +0x7c/+0x80 from). */
    if (out_x != NULL)
        *out_x = box_x + NEWGAME_CURSOR_OFFSET_X;
    if (out_y != NULL)
        *out_y = box_y + NEWGAME_CURSOR_OFFSET_Y + (cursor - sel2) * pitch;
}

void newgame_cursor_render(const newgame_cursor_ops *ops,
                           int box_x, int box_y,
                           int cursor, int sel2, int pitch,
                           int anim_idx)
{
    if (ops == NULL || ops->frame == NULL || ops->blt == NULL)
        return;

    int frame_id = newgame_cursor_frame_id(anim_idx);
    void *f = ops->frame(ops->user, frame_id);
    if (f == NULL)
        return;

    int x, y;
    newgame_cursor_base(box_x, box_y, cursor, sel2, pitch, &x, &y);
    ops->blt(ops->user, f, x, y);
}
