/*
 * src/glyph_text.h — the SJIS-aware token search of the engine's dynamic-text
 * system (`docs/findings/text-glyph-pipeline.md`).  The cell layout 0x40fa00 and
 * the escape pass are ui_text.c's (grid_cell_text, glyph_recs_set, text_parse).
 *
 * Win32-free, host-testable under the sanitizer.
 */
#ifndef OPENSUMMONERS_GLYPH_TEXT_H
#define OPENSUMMONERS_GLYPH_TEXT_H

#include <stdint.h>

/* ─── FUN_0040fd20 — SJIS-aware substring search ─────────────────────────
 *
 * Find `needle` in `hay` starting at byte offset `start`, advancing the scan
 * by 2 over SJIS lead bytes (`(signed char) < 0`).  Returns the byte offset
 * of the match, `start` for an empty needle, or -1 if not found.  Faithful
 * to 0x40fd20 (the compare is bounded so it never reads past `hay`'s NUL).
 * Used by the escape pass. */
int32_t glyph_token_search(const char *hay, const char *needle, int32_t start);

#endif /* OPENSUMMONERS_GLYPH_TEXT_H */
