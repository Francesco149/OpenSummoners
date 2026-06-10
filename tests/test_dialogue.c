/*
 * test_dialogue.c — host tests for the in-game dialogue box (src/dialogue.c).
 *
 * Ground truth: runs/dialogue-probe 20260609-062250 (box_cells.jsonl +
 * textout.jsonl: box pop first drawn at scale 50 @flip 2737, name @2776,
 * 'A' @2777, ~10 flips (5 updates) per char, ',' pause ~28 flips, the
 * '!' row close ~42, spaces ~1-3; arrow frameSel 20->21 @ +20 flips) and the
 * 0x439690/0x48c820/0x49c910/0x410560 decompile constants in dialogue.h.
 */
#include "dialogue.h"
#include "t.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- text expansion ---------------------------------------------------- */

int test_dialogue_expand_break(void)
{
    char rows[DIALOGUE_MAX_ROWS][DIALOGUE_ROW_CHARS + 1];
    int n = dialogue_expand_text("abc%ndef", rows);
    if (n != 2) T_FAIL("rows = %d (want 2)", n);
    if (strcmp(rows[0], "abc") != 0) T_FAIL("row0 = \"%s\"", rows[0]);
    if (strcmp(rows[1], "def") != 0) T_FAIL("row1 = \"%s\"", rows[1]);
    return 0;
}

int test_dialogue_expand_wrap(void)
{
    /* 36-char row limit: a 39-char second segment wraps at the last space,
     * consuming it (the line-1 shape: 25 + %n + 38 -> 25 / 28 / 9). */
    char rows[DIALOGUE_MAX_ROWS][DIALOGUE_ROW_CHARS + 1];
    const char *src =
        "aaaaaaaaaaaaaaaaaaaaaaaaa%n"          /* 25 a's              */
        "bbbbb bbbbbb bbbb bb bbb bbb "        /* 29 (ends in space)  */
        "ccccccccc";                           /* 9                   */
    int n = dialogue_expand_text(src, rows);
    if (n != 3) T_FAIL("rows = %d (want 3)", n);
    if (strlen(rows[0]) != 25) T_FAIL("row0 len = %zu", strlen(rows[0]));
    if (strcmp(rows[1], "bbbbb bbbbbb bbbb bb bbb bbb") != 0)
        T_FAIL("row1 = \"%s\" (wrap space not consumed?)", rows[1]);
    if (strcmp(rows[2], "ccccccccc") != 0) T_FAIL("row2 = \"%s\"", rows[2]);
    return 0;
}

/* The real town line 1 from the user's exe (skip when absent — story text
 * never lives in the repo; same pattern as test_exe_strings_real_exe). */
int test_dialogue_expand_real_line1(void)
{
    const char *path = "../vendor/unpacked/sotes.unpacked.exe";
    FILE *f = fopen(path, "rb");
    if (f == NULL) T_SKIP("no vendor/unpacked exe");
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *img = (unsigned char *)malloc((size_t)n);
    if (img == NULL || fread(img, 1, (size_t)n, f) != (size_t)n) {
        fclose(f); free(img); T_FAIL("read %s", path);
    }
    fclose(f);

    /* .data is mapped at VA = file offset + 0x400000 for this exe (verified
     * in test_exe_strings); read line 1 directly. */
    const char *line1 = (const char *)img + (DIALOGUE_VA_TOWN_LINE1 - 0x400000u);
    char rows[DIALOGUE_MAX_ROWS][DIALOGUE_ROW_CHARS + 1];
    int rc = dialogue_expand_text(line1, rows);
    if (rc != 3) { free(img); T_FAIL("line1 rows = %d (want 3)", rc); }
    /* the textout probe: row 1 = 25 chars ("Ahh, ... last!"), row 2 starts
     * "Look," and fits "...our new" (28), row 3 = "hometown." (9) */
    if (strlen(rows[0]) != 25)
        { free(img); T_FAIL("row0 len = %zu (want 25)", strlen(rows[0])); }
    if (strncmp(rows[1], "Look,", 5) != 0)
        { free(img); T_FAIL("row1 = \"%.10s...\"", rows[1]); }
    if (strlen(rows[1]) != 28)
        { free(img); T_FAIL("row1 len = %zu (want 28)", strlen(rows[1])); }
    if (strlen(rows[2]) != 9)
        { free(img); T_FAIL("row2 len = %zu (want 9)", strlen(rows[2])); }
    free(img);
    return 0;
}

/* ---- pop-in ------------------------------------------------------------ */

int test_dialogue_popin(void)
{
    dialogue_box d;
    dialogue_arm(&d, "Name", "ab");
    if (!dialogue_active(&d)) T_FAIL("not active after arm");
    if (dialogue_content_visible(&d)) T_FAIL("content visible at scale 0");

    /* first update: scale 50 (the probe's first drawn box). */
    dialogue_step(&d);
    if (d.scale != 50) T_FAIL("scale = %d after 1 update (want 50)", d.scale);
    int x, y, w, h;
    dialogue_scaled_rect(&d, &x, &y, &w, &h);
    /* 408*50/1000=20, 112*50/1000=5, centered (integer math) */
    if (w != 20 || h != 5) T_FAIL("scaled w/h = %d/%d (want 20/5)", w, h);
    if (x != DIALOGUE_BOX_X + (DIALOGUE_BOX_W / 2 - 10)) T_FAIL("x = %d", x);
    if (y != DIALOGUE_BOX_Y + (DIALOGUE_BOX_H / 2 - 2)) T_FAIL("y = %d", y);

    /* 19 more updates: scale 1000, content gate opens; the typewriter has
     * not revealed anything yet (the reveal ticks from the NEXT update). */
    for (int i = 0; i < 19; i++)
        dialogue_step(&d);
    if (d.scale != 1000) T_FAIL("scale = %d after 20 updates", d.scale);
    if (!dialogue_content_visible(&d)) T_FAIL("content gate closed at 1000");
    if (d.reveal != 0) T_FAIL("reveal = %d before the first content update", d.reveal);
    dialogue_scaled_rect(&d, &x, &y, &w, &h);
    if (x != DIALOGUE_BOX_X || y != DIALOGUE_BOX_Y ||
        w != DIALOGUE_BOX_W || h != DIALOGUE_BOX_H)
        T_FAIL("full rect = %d,%d %dx%d", x, y, w, h);
    return 0;
}

/* ---- typewriter timing -------------------------------------------------- */

int test_dialogue_reveal_cadence(void)
{
    /* "ab, cd!%nef" — reveal deltas in updates must be:
     *   a @+1 (first), b @+5, ',' @+5, ' ' @+15 (comma pause),
     *   c @+1 (space), d @+5, '!' @+5, e @+20 (3i row close +i), f @+5. */
    dialogue_box d;
    dialogue_arm(&d, "N", "ab, cd!%nef");
    for (int i = 0; i < 20; i++) dialogue_step(&d);   /* pop-in */
    int when[16];
    int prev = 0, u = 0;
    while (d.reveal < d.total && u < 200) {
        dialogue_step(&d);
        u++;
        if (d.reveal != prev) {
            when[prev] = u;          /* char index prev revealed at update u */
            prev = d.reveal;
        }
    }
    if (d.reveal != d.total) T_FAIL("typewriter stalled at %d/%d", d.reveal, d.total);
    static const int want_delta[9] = { 1, 5, 5, 15, 1, 5, 5, 20, 5 };
    for (int i = 0; i < 9; i++) {
        int delta = when[i] - (i > 0 ? when[i - 1] : 0);
        if (delta != want_delta[i])
            T_FAIL("char %d delta = %d (want %d)", i, delta, want_delta[i]);
    }
    /* row reveal split: "ab, cd!" = 7 chars + "ef" = 2 */
    if (dialogue_row_revealed(&d, 0) != 7) T_FAIL("row0 revealed %d", dialogue_row_revealed(&d, 0));
    if (dialogue_row_revealed(&d, 1) != 2) T_FAIL("row1 revealed %d", dialogue_row_revealed(&d, 1));
    return 0;
}

/* ---- arrow + portrait -------------------------------------------------- */

int test_dialogue_arrow_anim(void)
{
    dialogue_box d;
    dialogue_arm(&d, "N", "x");
    for (int i = 0; i < 20; i++) dialogue_step(&d);   /* pop-in */
    if (dialogue_arrow_frame(&d) != 20) T_FAIL("frame %d at gate (want 20)", dialogue_arrow_frame(&d));
    /* one anim step per 10 updates: 20 -> 21 -> 22 -> 23 -> 20 */
    static const int want[5] = { 21, 22, 23, 20, 21 };
    for (int s = 0; s < 5; s++) {
        for (int i = 0; i < 10; i++) dialogue_step(&d);
        if (dialogue_arrow_frame(&d) != want[s])
            T_FAIL("frame %d after %d anim steps (want %d)",
                   dialogue_arrow_frame(&d), s + 1, want[s]);
    }
    return 0;
}

int test_dialogue_portrait_fade(void)
{
    dialogue_box d;
    dialogue_arm(&d, "N", "x");
    for (int i = 0; i < 20; i++) dialogue_step(&d);   /* pop-in */
    if (dialogue_portrait_ramp_index(&d) != 0) T_FAIL("ramp at gate = %d", dialogue_portrait_ramp_index(&d));
    dialogue_step(&d);                                /* fade 50 */
    if (dialogue_portrait_ramp_index(&d) != 2) T_FAIL("ramp(50) = %d (want 2)", dialogue_portrait_ramp_index(&d));
    for (int i = 0; i < 8; i++) dialogue_step(&d);    /* fade 450 */
    if (dialogue_portrait_ramp_index(&d) != 18)
        T_FAIL("ramp(450) = %d (want 18)", dialogue_portrait_ramp_index(&d));
    dialogue_step(&d);                                /* fade 500: idx 20 > 0x13 */
    if (dialogue_portrait_ramp_index(&d) != -1)
        T_FAIL("ramp(500) = %d (want -1: the new cel snaps opaque)",
               dialogue_portrait_ramp_index(&d));
    return 0;
}
