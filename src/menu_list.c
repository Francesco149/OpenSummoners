/*
 * src/menu_list.c — menu-list controller ports.  See menu_list.h for the
 * struct model and framing.
 *
 * FUN_004192b0 (52 bytes), scroll-the-page-into-view:
 *
 *     hdr = this->[0x174];
 *     for (i = 0;
 *          hdr->cursor < i || hdr->stride + i <= hdr->cursor;   // loop cond
 *          i += hdr->stride) { }
 *     if (i != hdr->sel2) { hdr->sel2 = i; return 1; }
 *     return 0;
 *
 * The loop walks `i` up in `stride` steps and stops at the first page-top
 * with  i <= cursor < i + stride — i.e. floor(cursor/stride)*stride.  The
 * compares are signed (`jl`/`jle` in the disasm), so this assumes the
 * caller keeps cursor >= 0 and stride > 0 (retail has no guard either).
 *
 * This same page-top scan recurs verbatim inside the nav engine
 * (LAB_0043cda7), so it is factored out as page_top() here.
 */
#include "menu_list.h"

/* floor(cursor/stride)*stride via the engine's exact step-search loop. */
static int32_t page_top(const menu_list_hdr *hdr)
{
    int32_t i = 0;
    while (hdr->cursor < i || hdr->stride + i <= hdr->cursor) {
        i += hdr->stride;
    }
    return i;
}

int menu_list_scroll_into_view(menu_ctrl *c)
{
    menu_list_hdr *hdr = c->list;          /* iVar1 = *(this + 0x174) */
    int32_t top = page_top(hdr);           /* iVar2 = page-top loop   */
    if (top != hdr->sel2) {                /* iVar1 + 0x18            */
        hdr->sel2 = top;
        return 1;
    }
    return 0;
}
