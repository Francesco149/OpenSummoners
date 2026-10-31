/*
 * tests/test_glyph_text.c — glyph_token_search (src/glyph_text.c), the
 * SJIS-aware substring search.
 */
#include "t.h"
#include "glyph_text.h"

#include <string.h>

/* ─── glyph_token_search (FUN_0040fd20) ─────────────────────────────── */

int test_glyph_token_search_found(void)
{
    T_ASSERT_EQ_I(glyph_token_search("hello#world", "#", 0), 5);
    T_ASSERT_EQ_I(glyph_token_search("##", "#", 0), 0);
    return 0;
}

int test_glyph_token_search_absent(void)
{
    T_ASSERT_EQ_I(glyph_token_search("hello", "#", 0), -1);
    return 0;
}

int test_glyph_token_search_empty_needle(void)
{
    /* Retail returns `start` for a zero-length needle. */
    T_ASSERT_EQ_I(glyph_token_search("hello", "", 0), 0);
    T_ASSERT_EQ_I(glyph_token_search("hello", "", 3), 3);
    return 0;
}

int test_glyph_token_search_start_offset(void)
{
    /* First '#' at index 1 is skipped; search from 2 finds the one at 3. */
    T_ASSERT_EQ_I(glyph_token_search("a#b#c", "#", 2), 3);
    return 0;
}

int test_glyph_token_search_skips_sjis_trail(void)
{
    /* The 0x23 ('#') byte sits in an SJIS trail position; the scan must
     * skip it (advance 2 over the 0x82 lead) and not report a false hit. */
    T_ASSERT_EQ_I(glyph_token_search("\x82\x23X", "#", 0), -1);
    /* But a genuine '#' after the SJIS pair is found at its real offset. */
    T_ASSERT_EQ_I(glyph_token_search("\x82\x23X#", "#", 0), 3);
    return 0;
}
