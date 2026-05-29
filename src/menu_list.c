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

/*
 * FUN_0043ca40 (970 bytes) — the cursor-navigation engine.  __thiscall
 * taking a direction/action code; the engine samples GetTickCount()
 * internally for the auto-repeat timers — that clock is injected here as
 * `now` so the port stays pure and testable (same convention as
 * title_pace_step / input_poll_consume).
 *
 * `dir` is the menu action enum the latch feeds in:
 *   0 = up/prev    1 = down/next   2 = page-up    3 = page-down
 *   4/6 = axis-held auto-repeat (positive/negative); on each tick past the
 *         per-axis deadline they re-fire as 1 / 0 and re-arm a faster 100ms
 *         repeat (initial press arms a 300ms delay).
 *   5/7 = axis released → clear that axis's repeat deadline.
 *   9 = cancel (latches action=3)   10 = confirm (latches action=4)
 *   8 and anything > 10 = no-op (return 0).
 *
 * The inner dispatch is the indirect jump table `switchdataD_0043ce1c`
 * Ghidra could not recover.  Read out of the image with radare2
 * (`pxw 44 @ 0x43ce1c`), the 11 entries (dir 0..10) resolve to 7 handlers:
 *
 *   dir 0  -> 0x43cb0b (prev)        dir 1  -> 0x43cbd2 (next)
 *   dir 2  -> 0x43cc7c (page-up)     dir 3  -> 0x43cce2 (page-down)
 *   dir 4..8 -> 0x43cdfe (return 0)
 *   dir 9  -> 0x43cae9 (cancel)      dir 10 -> 0x43cafa (confirm)
 *
 * Every field meaning is type-dependent (hdr->type 0 linear-wrap / 2 grid /
 * 3 trailing-page).  Ported branch-for-branch from the decompile; see the
 * per-block provenance comments.  `iVar7` (entry sel2+stride) is captured
 * once at the top and read by the prev/next handlers and the type-3 tail.
 */
int32_t menu_list_nav(menu_ctrl *c, uint32_t dir, uint32_t now)
{
    menu_list_hdr *hdr = c->list;                 /* *(this + 0x174) */
    int32_t local_c = 0;
    int32_t iVar7 = hdr->sel2 + hdr->stride;      /* captured at entry */

    /* ── auto-repeat preprocessing (dir 4..7), GetTickCount → now ── */
    switch (dir) {
    case 4:
        if (hdr->repeat_a == 0) { hdr->repeat_a = now + 300; return 0; }
        if (now < hdr->repeat_a) return 0;        /* unsigned: not yet */
        dir = 1;
        hdr->repeat_a = now + 100;
        break;                                    /* fire as 'next' */
    case 5:
        hdr->repeat_a = 0;
        return 0;
    case 6:
        if (hdr->repeat_b == 0) { hdr->repeat_b = now + 300; return 0; }
        if (hdr->repeat_b <= now) {               /* unsigned */
            dir = 0;
            hdr->repeat_b = now + 100;
            break;                                /* fire as 'prev' */
        }
        return 0;
    case 7:
        hdr->repeat_b = 0;
        return 0;
    }

    if (dir > 10) return 0;                        /* table bound */

    switch (dir) {
    case 0:                                        /* handler 0x43cb0b: prev */
        if (hdr->count < 2) break;
        if (hdr->type == 0) {                      /* linear wrap */
            int32_t v;
            if (hdr->cursor == hdr->sel2) {
                if (iVar7 - 1 <= hdr->count - 1) {
                    local_c = 1;
                    hdr->cursor = iVar7 - 1;
                    break;
                }
                v = hdr->count - 1;
            } else {
                v = hdr->cursor - 1;
            }
            hdr->cursor = v;                       /* LAB_0043cbc2 */
        } else if (hdr->type == 2) {               /* grid */
            int32_t cur = hdr->cursor;
            if (cur > 0 && (cur / hdr->stride) * hdr->stride < cur) {
                local_c = 1;
                hdr->cursor = cur - 1;
                break;
            }
            int32_t t = hdr->stride - 1 + cur;
            if (t <= hdr->count - 1) {
                local_c = 1;
                hdr->cursor = t;
                break;
            }
            hdr->cursor = hdr->count - 1;          /* goto LAB_0043cbc2 */
        } else {                                   /* type 3 */
            int32_t cur = hdr->cursor;
            hdr->cursor = cur - 1;
            if (cur - 1 < 0) {
                local_c = 1;
                hdr->cursor = hdr->count + hdr->cursor;  /* wrap (= count-1) */
                break;
            }
        }
        local_c = 1;
        break;

    case 1:                                        /* handler 0x43cbd2: next */
        if (hdr->count < 2) break;
        if (hdr->type == 0) {                      /* linear wrap */
            int32_t maxi = hdr->count - 1;
            if (iVar7 - 1 <= hdr->count - 1) maxi = iVar7 - 1;
            if (hdr->cursor == maxi) {
                local_c = 1;
                hdr->cursor = hdr->sel2;           /* wrap to page-top */
                break;
            }
            hdr->cursor = hdr->cursor + 1;         /* LAB_0043cc6e */
        } else if (hdr->type == 2) {               /* grid */
            int32_t cur = hdr->cursor;
            if ((hdr->count - 1 <= cur) ||
                ((cur / hdr->stride + 1) * hdr->stride - 1 <= cur)) {
                local_c = 1;
                hdr->cursor = (cur / hdr->stride) * hdr->stride;  /* row start */
                break;
            }
            hdr->cursor = cur + 1;                 /* goto LAB_0043cc6e */
        } else {                                   /* type 3 */
            hdr->cursor = hdr->cursor + 1;
            if (hdr->count <= hdr->cursor) {
                local_c = 1;
                hdr->cursor = hdr->cursor % hdr->count;
                break;
            }
        }
        local_c = 1;
        break;

    case 2:                                        /* handler 0x43cc7c: page-up */
        if (hdr->count > 1 && hdr->stride < hdr->count) {
            int32_t v = hdr->cursor - hdr->stride;
            if (v < 0) {
                int32_t cm1 = hdr->count - 1;
                int32_t w = (cm1 / hdr->stride) * hdr->stride
                            + hdr->cursor % hdr->stride;
                int32_t clamp = (w <= cm1) ? w : cm1;
                if (clamp < 0) {
                    hdr->cursor = 0;
                } else {
                    if (cm1 < w) w = cm1;
                    hdr->cursor = w;
                }
            } else {
                hdr->cursor = v;
            }
            /* LAB_0043cda7: re-page; type-3 drags the cursor with it */
            int32_t top = page_top(hdr);
            if (top != hdr->sel2) {
                hdr->sel2 = top;
                if (hdr->type == 3) hdr->cursor = hdr->sel2;
                return 2;
            }
            return 0;
        }
        break;

    case 3:                                        /* handler 0x43cce2: page-down */
        if (hdr->count > 1 && hdr->stride < hdr->count) {
            int32_t v = hdr->stride + hdr->cursor;
            if (v < hdr->count) {
                hdr->cursor = v;
            } else {
                int32_t cm1 = hdr->count - 1;
                if (v < (cm1 / hdr->stride + 1) * hdr->stride) {
                    hdr->cursor = cm1;
                } else {
                    int32_t r = hdr->cursor % hdr->stride;
                    if (cm1 < r) r = cm1;
                    hdr->cursor = r;
                }
            }
            /* LAB_0043cda7 (shared) */
            int32_t top = page_top(hdr);
            if (top != hdr->sel2) {
                hdr->sel2 = top;
                if (hdr->type == 3) hdr->cursor = hdr->sel2;
                return 2;
            }
            return 0;
        }
        break;

    case 9:                                        /* handler 0x43cae9: cancel */
        local_c = 3;
        c->action = 3;
        break;

    case 10:                                       /* handler 0x43cafa: confirm */
        local_c = 4;
        c->action = 4;
        break;

    default:                                       /* dir 8 (and 4..7) → 0x43cdfe */
        return 0;
    }

    /* ── common tail (cases 0,1,9,10 and page guards that failed) ── */
    if (hdr->type == 2) {                          /* grid: re-page from cursor */
        int32_t top = page_top(hdr);
        if (top != hdr->sel2) {
            hdr->sel2 = top;
            local_c = 2;
        }
    } else if (hdr->type == 3) {                   /* trailing page */
        if (hdr->cursor < hdr->sel2) hdr->sel2 = hdr->cursor;
        if (iVar7 <= hdr->cursor) {
            hdr->sel2 = (hdr->cursor - hdr->stride) + 1;
            return local_c;
        }
    }
    return local_c;
}
