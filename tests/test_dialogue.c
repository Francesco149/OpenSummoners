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
    /* 36-char row limit: a 38-char second segment wraps at the last space,
     * KEEPING it on the wrapped row (retail renders the trailing space — the
     * line-1 shape: 25 + %n -> 25 / "...bbb " (29, with the space) / 9). */
    char rows[DIALOGUE_MAX_ROWS][DIALOGUE_ROW_CHARS + 1];
    const char *src =
        "aaaaaaaaaaaaaaaaaaaaaaaaa%n"          /* 25 a's              */
        "bbbbb bbbbbb bbbb bb bbb bbb "        /* 29 (ends in space)  */
        "ccccccccc";                           /* 9                   */
    int n = dialogue_expand_text(src, rows);
    if (n != 3) T_FAIL("rows = %d (want 3)", n);
    if (strlen(rows[0]) != 25) T_FAIL("row0 len = %zu", strlen(rows[0]));
    if (strcmp(rows[1], "bbbbb bbbbbb bbbb bb bbb bbb ") != 0)
        T_FAIL("row1 = \"%s\" (wrap space should be KEPT)", rows[1]);
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
    /* row 1 = 25 chars ("Ahh, ... last!"), row 2 = "Look, ... our new " (29 —
     * the trailing wrap space is KEPT, matching retail.osr's y=196 row), row 3 =
     * "hometown." (9). */
    if (strlen(rows[0]) != 25)
        { free(img); T_FAIL("row0 len = %zu (want 25)", strlen(rows[0])); }
    if (strncmp(rows[1], "Look,", 5) != 0)
        { free(img); T_FAIL("row1 = \"%.10s...\"", rows[1]); }
    if (strlen(rows[1]) != 29)
        { free(img); T_FAIL("row1 len = %zu (want 29)", strlen(rows[1])); }
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
    dialogue_scaled_rect(&d, DIALOGUE_BOX_X, DIALOGUE_BOX_Y, &x, &y, &w, &h);
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
    dialogue_scaled_rect(&d, DIALOGUE_BOX_X, DIALOGUE_BOX_Y, &x, &y, &w, &h);
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
    for (int i = 0; i < 20; i++) dialogue_step(&d);   /* pop-in -> scale==1000 */
    /* The cross-fade holds idx 0 for TWO opening ticks (drawcall-exact vs
     * retail.osr L0 ticks 660,661): the pop-in completion tick, then the ARM
     * tick (fade_armed 0->1, no counter advance), THEN the ramp starts. */
    if (dialogue_portrait_ramp_index(&d) != 0)
        T_FAIL("ramp at open tick = %d (want 0)", dialogue_portrait_ramp_index(&d));
    dialogue_step(&d);                                /* arm tick: still idx 0 */
    if (dialogue_portrait_ramp_index(&d) != 0)
        T_FAIL("ramp at arm tick = %d (want 0, the 2-tick hold)",
               dialogue_portrait_ramp_index(&d));
    dialogue_step(&d);                                /* fade 50 -> idx 2 */
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

/* ---- box position (0x49c640 anchor) ------------------------------------ */

int test_dialogue_box_position_town(void)
{
    /* The captured speaker body geometry (runs/box-pos-inputs, cutscene.c):
     * {sprite_w, metric_10, metric_14, off_1c, off_20}. */
    const dialogue_speaker_body ARCHE = { 2000, 5600, 0, -8, 32 };
    const dialogue_speaker_body ADULT = { 2000, 7600, 0,  0, 32 };

    /* Every distinct (speaker, world, camera) -> box GROUND TRUTH the 0x49c640
     * field-spec captured across the arrival + house + errands lines.  The box
     * = clamp(speaker-center - W/2) anchored above the head; the L9/house/errands
     * cases exercise BOTH clamps (32 = left 0x20, 200 = right 0x260-W). */
    struct { const dialogue_speaker_body *b; int32_t wx, wy, cx, cy; int ex, ey; } C[] = {
        { &ADULT,  49600, 43600, 12800, 12800, 174, 148 }, /* arrival L1 Father  */
        { &ARCHE,  41600, 45600, 12800, 12800,  94, 160 }, /* arrival L2 Arche   */
        { &ADULT,  38400, 43600, 12800, 12800,  62, 148 }, /* arrival L3 Mother  */
        { &ARCHE,  73104, 45600, 28000, 12800, 200, 160 }, /* arrival L9 (clamp R)*/
        { &ADULT,  38400, 43600, 28000, 12800,  32, 148 }, /* arrival L10 (clamp L)*/
        { &ARCHE, 128000, 39200, 89600,  3200, 190, 192 }, /* house L1 Arche     */
        { &ADULT, 131200, 37200, 89600,  3200, 200, 180 }, /* house L3 Mother(clmp)*/
        { &ADULT, 134400, 37200, 89600,  3200, 200, 180 }, /* house L4 Father(clmp)*/
        { &ARCHE, 128024, 39200, 89600,  3200, 190, 192 }, /* house L7 Arche     */
        { &ARCHE,  19200, 52000,     0, 16000,  32, 192 }, /* errands (clamp L)  */
    };
    for (size_t i = 0; i < sizeof C / sizeof C[0]; i++) {
        dialogue_box d;
        memset(&d, 0, sizeof d);
        d.anchored = 1;
        d.spk_wx   = C[i].wx;
        d.spk_wy   = C[i].wy;
        d.spk_body = *C[i].b;
        int bx, by;
        dialogue_box_position(&d, DIALOGUE_BOX_W, DIALOGUE_BOX_H,
                              C[i].cx, C[i].cy, 0, &bx, &by);
        if (bx != C[i].ex || by != C[i].ey)
            T_FAIL("case %zu: box=(%d,%d) want (%d,%d)", i, bx, by, C[i].ex, C[i].ey);
    }

    /* anchored==0 -> the centered fallback (0x49c640 param_6==0): x=(640-W)/2, y=80. */
    dialogue_box dc;
    memset(&dc, 0, sizeof dc);
    int cx, cy;
    dialogue_box_position(&dc, DIALOGUE_BOX_W, DIALOGUE_BOX_H, 99999, 99999, 0, &cx, &cy);
    if (cx != (0x280 - DIALOGUE_BOX_W) / 2 || cy != 0x50)
        T_FAIL("centered box=(%d,%d) want (%d,80)", cx, cy, (0x280 - DIALOGUE_BOX_W) / 2);
    return 0;
}

/* ── set_text: a same-speaker page advance keeps the box open + resets ONLY the
 *    typewriter (no pop-in) — retail's 1-tick same-speaker reset (THEME 1) ── */
int test_dialogue_set_text(void)
{
    dialogue_box d;
    dialogue_arm(&d, "Arche", "Hello there");
    for (int i = 0; i < 25; i++)               /* run the pop-in + a few chars */
        dialogue_step(&d);
    T_ASSERT_EQ_I(d.scale, 1000);
    T_ASSERT(d.reveal > 0);

    dialogue_set_text(&d, "Bye");
    T_ASSERT_EQ_I(d.active, 1);
    T_ASSERT_EQ_I(d.scale, 1000);              /* box stays open — no re-pop    */
    T_ASSERT_EQ_I(d.reveal, 0);                /* typewriter reset              */
    T_ASSERT_EQ_I(d.total, 3);                 /* "Bye"                         */
    T_ASSERT(strcmp(d.name, "Arche") == 0);    /* name persists                 */
    T_ASSERT_EQ_I(dialogue_content_visible(&d), 1);
    T_ASSERT_EQ_I(dialogue_typing(&d), 1);     /* immediately typing (no gate)  */

    /* the body reveals from the very next step (no 20-update pop-in wait) */
    dialogue_step(&d);
    T_ASSERT_EQ_I(d.reveal, 1);

    /* set_text on an inactive box is a no-op */
    dialogue_box e;
    memset(&e, 0, sizeof e);
    dialogue_set_text(&e, "x");
    T_ASSERT_EQ_I(e.active, 0);
    T_ASSERT_EQ_I(e.total, 0);
    return 0;
}

/* ── reopen: a box opens from spawn scale 200 (+50/update) → content gates in 16
 *    updates; the first box (cold arm, scale 0) takes 20 (engine-quirk #107) ── */
int test_dialogue_reopen(void)
{
    dialogue_box d;
    dialogue_reopen(&d, "Arche's Father", "Hi");
    T_ASSERT_EQ_I(d.active, 1);
    T_ASSERT_EQ_I(d.scale, DIALOGUE_OPEN_SCALE0);    /* spawn scale 200 */
    T_ASSERT(strcmp(d.name, "Arche's Father") == 0);
    T_ASSERT_EQ_I(dialogue_content_visible(&d), 0);  /* gated by the pop-in */

    /* (1000-200)/50 = 16 updates of +50 to reach full, then content gates */
    int steps = 0;
    while (!dialogue_content_visible(&d) && steps < 100) {
        dialogue_step(&d);
        steps++;
    }
    T_ASSERT_EQ_I(steps, 16);

    /* the first box (cold arm, scale 0) takes 20 updates */
    dialogue_box a;
    dialogue_arm(&a, "Arche", "Hi");
    int asteps = 0;
    while (!dialogue_content_visible(&a) && asteps < 100) {
        dialogue_step(&a);
        asteps++;
    }
    T_ASSERT_EQ_I(asteps, 20);
    return 0;
}

/* ── close_step: the pop-OUT shrinks the box -40/update, gates content off
 *    immediately, and removes it once scale < DIALOGUE_CLOSE_MIN (160) — the OLD
 *    box disappearing behind the opening new box on a speaker change ── */
int test_dialogue_close_step(void)
{
    dialogue_box d;
    dialogue_arm(&d, "Arche", "Hello");
    for (int i = 0; i < 25; i++)                  /* open + type to full */
        dialogue_step(&d);
    T_ASSERT_EQ_I(d.scale, 1000);
    T_ASSERT_EQ_I(dialogue_content_visible(&d), 1);

    /* one close step: scale -40, content gates OFF (scale < 1000), box still
     * active (the shrinking frame keeps rendering) */
    dialogue_close_step(&d);
    T_ASSERT_EQ_I(d.scale, 1000 - DIALOGUE_CLOSE_STEP);   /* 960 */
    T_ASSERT_EQ_I(dialogue_content_visible(&d), 0);
    T_ASSERT_EQ_I(d.active, 1);

    /* -40/update from 1000; removed once scale < 160 (last active scale 160 at
     * step 21, removed on step 22 = scale 120) */
    int steps = 1;
    while (d.active && steps < 100) {
        dialogue_close_step(&d);
        steps++;
    }
    T_ASSERT_EQ_I(steps, 22);

    /* no-op on an inactive box */
    dialogue_close_step(&d);
    T_ASSERT_EQ_I(d.active, 0);
    return 0;
}
