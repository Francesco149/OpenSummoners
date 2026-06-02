/*
 * src/glyph_render.c — the GDI text renderer.  See glyph_render.h for the
 * interface and docs/findings/text-glyph-pipeline.md for the subsystem map.
 *
 * The three render functions are direct translations of the retail
 * disassembly; the only liberty is routing the three GDI calls through the
 * injected glyph_gdi_ops vtable (so the walk is pure / host-testable) and
 * taking the glyph buffer / menu node through the typed models in
 * glyph_text.h / menu_list.h instead of raw +offset reads.
 */
#include "glyph_render.h"

#include <stddef.h>
#include <string.h>

/*
 * FUN_0048e860 (181 B) — TextOutA each glyph record of one cell.
 *
 * Retail clamps `count` to the buffer (count==0 or an over-long count means
 * "to the end": count = (int16_t)buf->count − start), then walks records
 * from `start`, drawing each record's NUL-terminated `ch` at (x+accum, y)
 * and advancing `accum` by strlen(ch)·advance.  `accum` is masked to 16 bits
 * before being added to x, exactly as retail (`local_28 & 0xffff`).
 */
void glyph_row_draw(const glyph_buf *buf, const glyph_gdi_ops *gdi,
                    int x, int y, int advance, uint16_t start, uint16_t count)
{
    /* count==0 OR buf->count < (count − start) ⇒ draw to the buffer end. */
    if (count == 0 ||
        (int32_t)(uint32_t)buf->count < (int32_t)((uint32_t)count - (uint32_t)start)) {
        count = (uint16_t)((int16_t)buf->count - (int32_t)start);
    }
    if (count == 0) {
        return;
    }

    const glyph_record *recs = (const glyph_record *)buf->records;
    uint32_t accum = 0;
    uint32_t remaining = count;
    do {
        const char *s = recs[start].ch;
        int len = (int)strlen(s);
        gdi->text_out(gdi->user, (int)(accum & 0xffff) + x, y, s, len);
        accum += (uint32_t)len * (uint32_t)advance;
        start++;
        remaining--;
    } while (remaining != 0);
}

/*
 * FUN_0048e6d0 (389 B) — the ruby (furigana) annotation pass.
 *
 * Faithful translation; unexercised by raw text (every record's flag1c is 0
 * after the build pass) and gated by node->field_14 in the grid renderer, so
 * it produces no output for the current scenes.  The ruby substrings are
 * read out of the glyph_record scratch area by the same byte arithmetic
 * retail uses (records + 3 + k·6 and records + (k·3 + 3)·2, k = start·6 + j);
 * that packing is defined by the (still-unported) escape expander.
 */
void glyph_ruby_draw(const glyph_buf *buf, const glyph_gdi_ops *gdi,
                     int x, int y, int advance, uint16_t start, uint16_t count)
{
    if (count == 0 ||
        (int32_t)(uint32_t)buf->count < (int32_t)((uint32_t)count - (uint32_t)start)) {
        count = (uint16_t)((int16_t)buf->count - (int32_t)start);
    }
    if (count == 0) {
        return;
    }

    const char *records = (const char *)buf->records;
    uint32_t accum = 0;
    uint32_t remaining = count;
    do {
        int rec_off = (int)start * 0x24;                 /* record[start] byte offset */
        uint16_t ruby_n = *(const uint16_t *)(records + rec_off + 0x1c); /* flag1c */
        if (ruby_n != 0) {
            uint16_t j = 0;
            do {
                int k = (int)start * 6 + j;
                int base_col = *(const int32_t *)(records + rec_off + 0x20); /* color20 */
                int px = (base_col + j) * advance + (int)(accum & 0xffff) + x;

                const char *s = records + 3 + k * 6;
                gdi->text_out(gdi->user, px, y, s, (int)strlen(s));

                s = records + (k * 3 + 3) * 2;
                gdi->text_out(gdi->user, advance / 2 + px, y - advance / 2,
                              s, (int)strlen(s));

                records = (const char *)buf->records;    /* retail reloads *in_ECX */
                j++;
            } while (j < *(const uint16_t *)(records + rec_off + 0x1c));
        }
        /* advance past the main glyph */
        int len = (int)strlen(records + (int)start * 0x24);
        accum += (uint32_t)len * (uint32_t)advance;
        start++;
        remaining--;
    } while (remaining != 0);
}

/*
 * FUN_0048e200 (1221 B), GDI branch (param_1 != 0) — render the text grid.
 *
 * `node` is the child controller node (0x48c820 passes it as ECX); x/y are
 * the parent's base position (param_4/param_5); font_main/font_shadow are the
 * registered HFONTs (param_6/param_7).  The retail mode==0 (sprite-cell)
 * branch — which keyed-blits font-texture frames + cell sub-widgets through
 * the ZDD instead of TextOutA — is a separate render path (needs sprite
 * decode + the built cell secondary widgets) and is deferred; this is the
 * pure GDI-text path the new-game/options/prologue menus take.
 */
void glyph_grid_render(const menu_node *node, const glyph_gdi_ops *gdi,
                       int x, int y, void *font_main, void *font_shadow)
{
    const menu_list_hdr *hdr     = (const menu_list_hdr *)node->ctrl_list;
    const menu_row      *rows    = (const menu_row *)node->ctrl_rows;
    const menu_entry    *entries = (const menu_entry *)node->ctrl_entries;
    if (hdr == NULL) {
        return;
    }

    int lineH = (int32_t)node->field_1ac;     /* iVar2 — row pitch (0x1c) */
    int row   = hdr->sel2;                     /* iVar13 — first visible row */
    int disp  = 0;                             /* local_8 — 0-based screen row */

    if (row >= hdr->count) {
        return;
    }

    do {
        const menu_row *mrow = &rows[row];
        for (int col = 0; col < hdr->alloc_b; col++) {
            const menu_entry *e = &entries[col];
            const menu_cell  *cell = &mrow->cells[col];
            const glyph_buf  *buf  = (const glyph_buf *)cell->obj0;

            int cx = e->pos + x + node->field_c;
            int cy = e->field4 + node->field_10 + lineH * disp + y;

            if (buf == NULL) {
                continue;
            }

            /* ── colour selection (text colour, secondary colour, shadow?) ── */
            uint32_t text_col, sec_col;
            int draw_shadow = 1;
            if (mrow->flag8 == 0 && hdr->type != 1) {
                /* disabled row: plain text, no drop shadow */
                text_col    = node->label1;            /* +0x194 */
                sec_col     = node->label2;            /* +0x198 */
                draw_shadow = 0;
            } else if (hdr->cursor == row && hdr->type != 1) {
                /* focused row */
                if (e->field_10 == 0) {
                    text_col = node->color2;           /* +0x18c */
                    sec_col  = node->color3;           /* +0x190 */
                } else {
                    text_col = (uint32_t)e->field_1c;  /* entry override +0x1c */
                    sec_col  = (uint32_t)e->field_20;  /* +0x20 */
                }
            } else if (cell->field_c == 0) {
                /* normal row, no per-cell override */
                if (e->field_10 == 0) {
                    text_col = node->color0;           /* +0x180 */
                    sec_col  = node->label0;           /* +0x188 (ruby pass only) */
                } else {
                    text_col = (uint32_t)e->field_14;  /* entry override +0x14 */
                    sec_col  = (uint32_t)e->field_18;  /* +0x18 */
                }
            } else {
                /* per-cell override */
                text_col = (uint32_t)cell->color10;    /* cell +0x10 */
                sec_col  = (uint32_t)cell->color14;    /* cell +0x14 */
            }

            /* ── right-align within the column when entry.field_c == 1 ── */
            if (e->field_c == 1) {
                uint32_t v = (uint32_t)e->extent + (uint32_t)buf->len * (uint32_t)(-7);
                cx += ((int32_t)v < 0) ? 0 : (int32_t)v;   /* max(0, extent − 7·len) */
            }

            /* ── draw: drop shadow (down 1, right 1), then the glyphs ── */
            gdi->select_font(gdi->user, font_main);
            if (draw_shadow) {
                gdi->set_text_color(gdi->user, node->color1);  /* +0x184 */
                glyph_row_draw(buf, gdi, cx,     cy + 1, 7, 0, 0);
                glyph_row_draw(buf, gdi, cx + 1, cy,     7, 0, 0);
            }
            gdi->set_text_color(gdi->user, text_col);
            glyph_row_draw(buf, gdi, cx, cy, 7, 0, 0);

            /* ── optional ruby pass (node->field_14 set) ── */
            if (node->field_14 != 0) {
                gdi->set_text_color(gdi->user, sec_col);
                gdi->select_font(gdi->user, font_shadow);
                glyph_ruby_draw(buf, gdi, cx, node->field_18 + cy, 7, 0, 0);
            }
        }

        disp++;
        if (disp >= hdr->stride) {
            return;
        }
        row++;
    } while (row < hdr->count);
}
