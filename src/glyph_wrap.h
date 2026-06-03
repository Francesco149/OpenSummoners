/*
 * src/glyph_wrap.h — the tooltip text-node word-wrap / justification pass.
 *
 * Ports the layout half of the engine's standalone TEXT NODE (the widget that
 * draws the new-game / options tooltip help string at the bottom of the
 * screen).  Unlike the menu grid (menu_list / glyph_text / glyph_render — a
 * fixed rows×cols cell layout), a text node takes ONE free-form string and
 * greedily word-wraps it into rows at a configured pixel/char width, honoring
 * explicit `%n` line breaks.  Retail builds it in three steps on the node's
 * twin record arrays (a 0xc-byte position record {col@+0, row@+2,
 * linebreak@+8, wraptype@+9} per glyph, parallel to a 0x24-byte glyph-bytes
 * record):
 *
 *   FUN_0040f040  parse  — split the source into glyph records, turning `%n`
 *                          into a per-glyph forced-break flag (`%m<n>%` =
 *                          colour, `%w` = wrap hint; neither used by the
 *                          English tooltips).
 *   FUN_0040e5e0  justify— greedy word-wrap: assign each glyph a (col,row),
 *                          breaking to a new row when the next WORD would
 *                          overflow `width`, or on a forced `%n` break.
 *   FUN_004031c0  commit — copy the wrapped bytes back into the 0x24-byte
 *                          glyph records the renderer walks.
 *
 * This module fuses the parse + justify into one pure pass that emits the
 * per-row text the port draws directly (mirroring how main.c renders the menu
 * grid itself — by position, not via the retail widget-tree walk).  A word is
 *   - an alpha run  [A-Za-z']+  plus one optional trailing {space ! , - . ; ?}
 *   - a digit run   [0-9.,]+    plus one optional trailing space
 *   - any other single glyph (e.g. a lone space or '(')
 * exactly as FUN_0040e5e0 classifies them (sVar3 = 1 / 2 / 0).  The width
 * accumulator and wrap test are faithful to the retail `uVar13`/`param_1`
 * compare.  Verified bit-exact against the captured new-game golden: the
 * difficulty tooltip at width 68 splits 65/52 glyphs across y=416/444
 * (tests/test_glyph_wrap.c).
 *
 * DEFERRED (documented, English never reaches it): the SJIS-lead path
 * (sVar3 == 3) — retail's kinsoku line-break rules over the DAT_008548xx
 * 2-byte punctuation table.  An SJIS lead byte is treated here as a lone
 * single glyph (it still wraps, just not by the Japanese kinsoku rules); port
 * the kinsoku table when a Japanese build needs it.
 *
 * Pure / Win32-free / host-testable.
 */
#ifndef OPENSUMMONERS_GLYPH_WRAP_H
#define OPENSUMMONERS_GLYPH_WRAP_H

/* Caps: the retail node allocates a 0x200-glyph buffer; a tooltip is two short
 * rows.  These bound the result without a heap alloc. */
#define GLYPH_WRAP_MAX_ROWS 16
#define GLYPH_WRAP_MAX_LINE 256

typedef struct glyph_wrap_result {
    int  row_count;                              /* number of rows produced */
    char rows[GLYPH_WRAP_MAX_ROWS][GLYPH_WRAP_MAX_LINE];  /* NUL-terminated  */
} glyph_wrap_result;

/*
 * Word-wrap `src` into `out` at `width` glyph-columns per row (ASCII = 1
 * column/byte, SJIS = 2).  `%n` forces a break before the following glyph;
 * `%m<digits>%` / `%w` escapes are consumed (colour/wrap hints, unused by the
 * tooltips).  Over-long single words are not split (retail keeps a word
 * whole); rows past GLYPH_WRAP_MAX_ROWS or bytes past GLYPH_WRAP_MAX_LINE are
 * dropped (well above any real tooltip).  `out` is fully written (zeroed
 * first).
 */
void glyph_wrap_layout(const char *src, int width, glyph_wrap_result *out);

#endif /* OPENSUMMONERS_GLYPH_WRAP_H */
