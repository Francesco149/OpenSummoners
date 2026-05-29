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

#include <stdlib.h>

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

/*
 * FUN_0043ce50 (220 bytes) — the input-action latch.  Gates the menu's
 * input handling on the sub-object being ready, then dispatches on the
 * controller mode.  `dir` is the menu action enum; `now` is only used on
 * the mode-1 path (forwarded to the nav engine's GetTickCount slot).
 *
 *     sub = this->[0];                              ; [ecx]
 *     if (sub->[0x54] != 1000 || sub->[0x04] == 0) return 0;
 *     if (this->[0x08] == 1) return FUN_0043ca40(dir);     ; mode 1 → nav
 *     if (this->[0x08] == 2) { ...confirm list... }        ; mode 2
 *     return 0;
 *
 * Mode 2 (confirm/message list cl = this->[0x170]):
 *   submode 0 (cl->[0x0c]==0), dir in [8,10]:
 *       cl->[0x18] != 0  → cl->[0x18]=0;            return 6
 *       cl->[0x14] != 0  → this->[0x1c]=8;          return 8
 *       else                                          return 0
 *   submode 1 (cl->[0x0c]==1), dir in [8,10], this->[0x1c] != 8:
 *       cap = (cl->[0]->[0x0c])->[0x08] (u16);  pos = cl->[0x04] (u16)
 *       cap <= pos → this->[0x1c]=8;             return 8   (dismiss)
 *       else  cl->[0x04]=cap; this->[0x1c]=6;    return 6   (reveal-all)
 *
 * (The retail null-check on cl is dead — cl->submode is dereferenced
 * before it, so cl is assumed non-null here, matching the live path.)
 */
int32_t menu_list_latch(menu_ctrl *c, uint32_t dir, uint32_t now)
{
    menu_input_sub *sub = c->sub;                  /* esi = [ecx] */
    if (sub->ready != 1000 || sub->enabled == 0) {
        return 0;
    }

    if (c->mode == 1) {
        return menu_list_nav(c, dir, now);         /* call 0x43ca40 */
    }

    if (c->mode == 2) {
        confirm_list *cl = c->list2;               /* [ecx + 0x170] */
        if (cl->submode == 0) {
            if (dir > 7 && dir < 11) {             /* param in [8,10] */
                if (cl->flag18 != 0) {
                    cl->flag18 = 0;
                    return 6;
                }
                if (cl->flag14 != 0) {
                    c->action = 8;
                    return 8;
                }
            }
            return 0;
        }
        if (cl->submode == 1 && dir > 7 && dir < 11 && c->action != 8) {
            uint16_t cap = cl->src->caprec->cap;
            if (cap <= cl->pos) {                  /* nothing left → dismiss */
                c->action = 8;
                return 8;
            }
            cl->pos = cap;                         /* fast-forward to the end */
            c->action = 6;
            return 6;
        }
        return 0;
    }

    return 0;
}

/*
 * FUN_0040e0c0 (555 bytes) — tear down the controller's geometry.
 *
 * Frees, in retail order, the confirm-list source graph (list2 → src →
 * its owned sub-buffers + caprec), the confirm-list object itself, the
 * owned buffer at +0x164, the per-column metadata array (entries), then
 * every row's cells (each cell owning three lazily-built sub-objects) and
 * the row array, and finally the list header.  Each step is guarded on
 * non-NULL, so it no-ops on a fresh controller; the row/cell loop bounds
 * read alloc_a/alloc_b from the *still-live* header, so the header is
 * freed last.  The retail engine frees through operator delete[]
 * (FUN_005bef0e); free() is the host equivalent.
 *
 * NB the cells[k].obj0 path frees `*obj0` then `obj0` — obj0 points at an
 * object whose first word is itself an owned pointer (built by 0x411f40).
 */
void menu_ctrl_clear(menu_ctrl *c)
{
    /* list2 (+0x170) — the confirm/message source graph. */
    if (c->list2 != NULL && c->list2->src != NULL) {
        confirm_src *src = c->list2->src;          /* piVar1 = **(ecx+0x170) */
        if (src->caprec != NULL) {                 /* piVar1[3] */
            free(src->caprec->owned0);             /* free(*puVar2) — no null check in retail */
            free(src->caprec);                     /* free(puVar2) */
            src->caprec = NULL;
        }
        if (src->owned0 != NULL) { free(src->owned0); src->owned0 = NULL; }  /* piVar1[0] */
        if (src->owned8 != NULL) { free(src->owned8); src->owned8 = NULL; }  /* piVar1[2] */
        free(src);                                 /* free(piVar1) */
        c->list2->src = NULL;                      /* *(ecx+0x170)[0] = 0 */
    }
    if (c->list2 != NULL) { free(c->list2); c->list2 = NULL; }

    /* +0x164 — a single owned buffer. */
    if (c->field_164 != NULL) { free(c->field_164); c->field_164 = NULL; }

    /* entries (+0x178) — the per-column metadata array. */
    if (c->entries != NULL) { free(c->entries); c->entries = NULL; }

    /* rows (+0x17c) — each row's cells (+ sub-objects), then the array. */
    if (c->rows != NULL) {
        menu_list_hdr *hdr = c->list;              /* iVar6 = *(ecx+0x174) */
        if (hdr->alloc_a > 0) {
            for (int32_t r = 0; r < hdr->alloc_a; r++) {
                menu_cell *cells = c->rows[r].cells;
                if (cells != NULL && hdr->alloc_b > 0) {
                    for (int32_t k = 0; k < hdr->alloc_b; k++) {
                        menu_cell *cell = &cells[k];
                        if (cell->obj0 != NULL) {
                            free(*(void **)cell->obj0);  /* *obj0 is owned */
                            free(cell->obj0);
                            cell->obj0 = NULL;
                        }
                        if (cell->obj54 != NULL) { free(cell->obj54); cell->obj54 = NULL; }
                        if (cell->obj20 != NULL) { free(cell->obj20); cell->obj20 = NULL; }
                    }
                }
                if (c->rows[r].cells != NULL) {
                    free(c->rows[r].cells);
                    c->rows[r].cells = NULL;
                }
            }
        }
        free(c->rows);
        c->rows = NULL;
    }

    /* list header (+0x174) — freed last (its dims sized the loops above). */
    if (c->list != NULL) { free(c->list); c->list = NULL; }
}

/*
 * FUN_0040f5c0 (563 bytes) — build the controller's list header + grid.
 *
 * Clears any stale state (it is called on a freshly pool-acquired slot
 * whose pointers may be garbage from a prior occupant), seeds the
 * controller scalars, allocates the 0x24-byte list header, the row array
 * (alloc_a × menu_row) with a per-row cell array (alloc_b × menu_cell),
 * and the per-column metadata array (alloc_b × menu_entry).
 *
 * Param map (the title menu passes 0,0,6,1,6,0):
 *   f_c     → +0x0c        alloc_a → hdr.alloc_a (sizes the row array)
 *   f_10    → +0x10        alloc_b → hdr.alloc_b (sizes cells & entries)
 *   stride  → hdr.stride   type    → hdr.type
 *
 * operator_new → calloc here (see header for the zero-init divergence).
 */
void menu_ctrl_build(menu_ctrl *c, int32_t f_c, int32_t f_10,
                     int32_t alloc_a, int32_t alloc_b,
                     int32_t stride, int32_t type)
{
    menu_ctrl_clear(c);                            /* FUN_0040e0c0 */

    c->field_c   = f_c;                            /* +0x0c = param_1 */
    c->field_10  = f_10;                           /* +0x10 = param_2 */
    c->mode      = 1;                              /* +0x08 = 1 */
    c->field_20  = 0;                              /* +0x20 */
    c->field_84  = 0;                              /* +0x84 */
    c->field_140 = 0;                              /* +0x140 */

    menu_list_hdr *hdr = (menu_list_hdr *)calloc(1, sizeof *hdr); /* operator_new(0x24) */
    c->list      = hdr;
    hdr->sel2    = 0;                              /* +0x18 */
    hdr->cursor  = 0;                              /* +0x14 */
    hdr->alloc_a = alloc_a;                        /* +0x04 = param_3 */
    hdr->alloc_b = alloc_b;                        /* +0x08 = param_4 */
    hdr->stride  = stride;                         /* +0x0c = param_5 */
    hdr->type    = type;                           /* +0x00 = param_6 */
    hdr->repeat_a = 0;                             /* +0x1c */
    hdr->repeat_b = 0;                             /* +0x20 */
    hdr->count   = 0;                              /* +0x10 */

    c->rows    = (menu_row *)calloc((size_t)(alloc_a > 0 ? alloc_a : 0),
                                    sizeof(menu_row));   /* operator_new(alloc_a << 4) */
    c->entries = (menu_entry *)calloc((size_t)(alloc_b > 0 ? alloc_b : 0),
                                      sizeof(menu_entry)); /* operator_new(alloc_b * 0x24) */

    for (int32_t r = 0; r < alloc_a; r++) {
        c->rows[r].field0 = 0;                     /* row[0] */
        c->rows[r].action = 0;                     /* row[4] */
        c->rows[r].cells = (menu_cell *)calloc((size_t)(alloc_b > 0 ? alloc_b : 0),
                                               sizeof(menu_cell)); /* alloc_b * 0x18 */
        for (int32_t k = 0; k < alloc_b; k++) {
            c->rows[r].cells[k].obj0    = NULL;    /* cell[0] */
            c->rows[r].cells[k].obj54   = NULL;    /* cell[4] */
            c->rows[r].cells[k].obj20   = NULL;    /* cell[8] */
            c->rows[r].cells[k].field_c = 0;       /* cell[0xc] */
        }
    }

    for (int32_t e = 0; e < alloc_b; e++) {
        c->entries[e].pos      = e << 5;           /* entry[0] = index*0x20 */
        c->entries[e].field4   = 0;                /* entry[4] */
        c->entries[e].extent   = 0x20;             /* entry[8] */
        c->entries[e].field_c  = 0;                /* entry[0xc] */
        c->entries[e].field_10 = 0;                /* entry[0x10] */
        c->entries[e].field_14 = 0;                /* entry[0x14] */
        c->entries[e].field_18 = 0;                /* entry[0x18] */
        c->entries[e].field_1c = 0;                /* entry[0x1c] */
        c->entries[e].field_20 = 0;                /* entry[0x20] */
    }
}

/* The unported cell text-layout builder (0x40fa00).  See header. */
menu_cell_layout_fn menu_cell_layout_hook = NULL;
const void         *menu_cell_layout_text = NULL;   /* models &DAT_008a9b6c */

/*
 * FUN_00411f40 (444 bytes) — refresh a row's cell sub-objects.
 *
 * __thiscall: this = controller, param_1 = row index.  Walks the row's cell
 * array (bound by the header's alloc_b, the per-row cell count) and, for each
 * cell, refreshes whichever of its three lazily-built sub-objects are present:
 *
 *     hdr = this->[0x174];
 *     for (i = 0; i < hdr->alloc_b; i++) {
 *         cell = this->rows[row].cells[i];
 *         if (cell.obj0)  0x40fa00(row, i, &DAT_008a9b6c);  // re-layout text
 *         if (cell.obj54 && i < alloc_b && row < count) { ...zero fields... }
 *         if (cell.obj20 && i < alloc_b && row < count) {
 *             ...zero fields...
 *             obj20->[0x1c] = max(obj20->[0x14], min(obj20->[0x18], 0));
 *         }
 *     }
 *
 * Dead allocation (quirk #36): retail re-checks `cell.objNN == 0` *inside*
 * each `cell.objNN != 0` block and, if so, operator_new's the sub-object.
 * That inner check is unreachable (same slot, no intervening write — verified
 * against the disasm at 0x411fbf / 0x412046), so this function never
 * allocates; it only re-zeroes already-built sub-objects.  The omitted dead
 * branch mirrors the dead null-check noted in menu_list_latch.
 *
 * The obj20 clamp reads the zeros just written, so field1c resolves to 0
 * here; the arithmetic is ported faithfully (the engine recomputes the field
 * from real values elsewhere).  The `i < alloc_b` re-checks are likewise
 * redundant with the loop bound but kept as written; `row < count` is the
 * meaningful guard (skip when the row index outruns the entry array).
 *
 * The loop counter advances full-width then masks to 16 bits for the cell
 * index (local_8 / uVar8 in the decompile) — harmless below 0x10000 cells.
 */
void menu_row_finalize(menu_ctrl *c, int32_t row)
{
    menu_list_hdr *hdr = c->list;                  /* iVar = *(this+0x174) */
    uint32_t counter = 0;                           /* local_8 — full width */
    uint32_t i;                                     /* uVar8 — cell index   */

    if (hdr->alloc_b <= 0) {                         /* cmp [hdr+8],0; jle ret */
        return;
    }

    i = 0;
    do {
        menu_cell *cell = &c->rows[row].cells[i];   /* rows[row].cells + i*0x18 */

        /* obj0 present → re-lay-out its glyph text (0x40fa00, unported). */
        if (cell->obj0 != NULL) {
            if (menu_cell_layout_hook != NULL) {
                menu_cell_layout_hook(c, row, (int32_t)i, menu_cell_layout_text);
            }
        }

        /* obj54 present and row in range → re-zero its modelled fields. */
        if (cell->obj54 != NULL &&
            (int32_t)i < hdr->alloc_b && row < hdr->count) {
            menu_cell_obj54 *o = (menu_cell_obj54 *)cell->obj54;
            o->field0  = 0;
            o->field4  = 0;
            o->field46 = 0;
            o->field48 = 0;
            o->field4c = 0;
            o->field4a = 0;
            o->field50 = 0;
        }

        /* obj20 present and row in range → re-zero, then recompute the clamp. */
        if (cell->obj20 != NULL &&
            (int32_t)i < hdr->alloc_b && row < hdr->count) {
            menu_cell_obj20 *o = (menu_cell_obj20 *)cell->obj20;
            o->field4  = 0;
            o->field8  = 0;
            o->field0  = 0;
            o->field_c = 0;
            o->field10 = 0;
            o->field14 = 0;
            o->field18 = 0;
            if ((int32_t)i < hdr->alloc_b && row < hdr->count) {
                int32_t hi      = o->field18;             /* obj20[0x18] */
                int32_t lo      = o->field14;             /* obj20[0x14] */
                int32_t clamped = (hi < 0) ? hi : 0;      /* min(field18, 0) */
                o->field1c = (lo > clamped) ? lo : clamped;
            }
        }

        counter++;
        i = counter & 0xffff;
    } while ((int32_t)i < hdr->alloc_b);
}
