/*
 * src/newgame_menu.h — the new-game config menu BUILDER.
 *
 * Ports the construction half of the new-game ("Start") config scene: the
 * code that builds the cell grid the (already-ported, bit-exact) GDI text
 * renderer walks.  Once the port builds these cells, glyph_grid_render emits
 * the SAME per-glyph TextOutA stream retail does, so the end-to-end
 * stream/pixel diff for the menu is trivially zero — the last gap the text
 * pipeline was gating (verification plan step 4, text-glyph-pipeline.md).
 *
 * The scene this targets is FUN_00564780 case 0x24 (the "Start Game" config
 * menu), reached by pressing confirm on the title's Start row.  Its grid:
 *
 *     Game Difficulty            1:Easy        (row 0, focused)
 *     Auto-guard                 On            (row 1)
 *     Start Game                               (row 2, an action button)
 *
 * Ground truth: tests/scenarios/new-game-through/goldens/ — the captured
 * retail menu (PNG) + its full per-glyph TextOutA stream (jsonl), recorded
 * with frida_capture.py --textout-probe (ckpt 36, quirk #63).
 *
 * Ported functions (one per address):
 *   - FUN_00412160  menu_grid_append      (append a labelled grid row)
 *   - FUN_00566570  newgame_option_label  (option id → label string; 3/4)
 *   - FUN_00566a80  newgame_option_value  (option id + setting → value; 3/4)
 *   - FUN_00564780  newgame_config_build  (case 0x24 grid-build sequence)
 *
 * Win32-free: the build is pure structure manipulation over the menu_ctrl /
 * menu_node model (menu_list.h) and the glyph layout builder (glyph_text.h).
 * The interactive run loop, value toggles, tooltip text node, and the box
 * widget tree (0x411940's geometry/title sub-nodes) are NOT ported here
 * — this is the grid the renderer needs, host-tested against the golden.
 */
#ifndef OPENSUMMONERS_NEWGAME_MENU_H
#define OPENSUMMONERS_NEWGAME_MENU_H

#include <stdint.h>

#include "menu_list.h"

/* ─── FUN_00412160 — append a labelled row to the grid ───────────────────
 *
 * __thiscall(c); params (kind, id, label).  Appends one row to the grid the
 * controller's list header describes: bumps the row count, stamps the new
 * row's kind (→ row.field0), action id (→ row.action) and selectable flag
 * (→ row.flag8 = 1), refreshes the row's cell sub-objects, and lays `label`
 * out into the new row's column 0 (FUN_0040fa00).  Returns the new row index,
 * or -1 if the row array is full (the final layout then no-ops on the row<0
 * bounds check).
 *
 * `kind` selects how the run loop treats the row: 0 = an option row (a label
 * in col 0 + a value the build fills into col 1), 3 = an action button (col 0
 * label only, e.g. "Start Game").
 *
 * The per-column refresh (FUN_00412160:25-71) is exactly FUN_00411f40's body
 * (re-lay-out obj0, re-zero obj54, re-zero + clamp obj20), so it is delegated
 * to the already-ported menu_row_finalize.  On a fresh append every cell
 * sub-object is NULL, so that refresh is a no-op (its guarded bodies and their
 * dead inner operator_new — quirk #36 — are all skipped); it only does work
 * when a populated row is rebuilt.  Faithful to 0x412160.
 *
 * (The thiscall twin 0x40f800 — row append without a label arg, used by
 * the screen-settings menu case 0x20 — is the same body with the obj20 clamp
 * inlined instead of via 0x412330; case 0x24 uses only this one.) */
int32_t menu_grid_append(menu_ctrl *c, int32_t kind, int32_t id,
                         const char *label);

/* ─── FUN_00566570 — option id → label string ────────────────────────────
 *
 * Copies the option's label into `buf` (caller provides >= 256 bytes, as
 * retail's 512-byte stack buffer).  A pure id→string switch; only the arms
 * the new-game config menu uses are ported (id 3 = "Game Difficulty", id 4 =
 * "Auto-guard"); any other id copies the engine-name buffer stand-in (retail
 * &DAT_008a9b6c) — modelled as the empty string here, matching the renderer's
 * "nothing to draw" outcome.  Faithful to the relevant arms of 0x566570. */
void newgame_option_label(int32_t id, char *buf);

/* ─── FUN_00566a80 — option id + current setting → value string ──────────
 *
 * Copies the option's *current value* label into `buf`.  `setting` is the
 * stored option value retail reads from the settings record
 * (*DAT_008a6e80 + 0xc + id*0x1c).  Only the new-game config arms are ported:
 *   id 3 (Game Difficulty): 10→"1:Easy" 20→"2:Normal" 30→"3:Hard"
 *                           40→"4:Expert" 50→"5:Nightmare"
 *   id 4 (Auto-guard):      0→"Off" 1→"On"
 * Any other (id, setting) yields the empty string (the unmodelled default
 * arm).  Faithful to the id 3 / id 4 arms of 0x566a80; the difficulty strings
 * carry clean retail symbols, the On/Off pair are the .rodata DATs the golden
 * confirms render as "On" (value shown) / "Off" (its toggle counterpart). */
void newgame_option_value(int32_t id, int32_t setting, char *buf);

/* ─── FUN_00566850 — option id → tooltip (description) string ────────────
 *
 * Copies the option's help/tooltip text into `buf` (caller provides >= 256
 * bytes, as retail's 512-byte stack scratch).  A pure id→string switch run by
 * the config run loop every frame for the *focused* option row (the second
 * GDI-text node at y=416/444).  Only the new-game config arms are ported
 * (id 3 = game-difficulty help, id 4 = auto-guard help); any other id copies
 * the engine-name buffer stand-in (retail &DAT_008a9b6c) — modelled as the
 * empty string, matching the renderer's "nothing to draw" outcome.  The
 * difficulty arm's text embeds no escape; faithful to the id 3 / id 4 arms of
 * 0x566850.  (The renderer word-wraps the single source string across the
 * tooltip box; the golden's two TextOutA lines reconstruct to this literal.) */
void newgame_option_tooltip(int32_t id, char *buf);

/* The option ids the case-0x24 menu rows carry (FUN_00564780 case 0x24). */
#define NEWGAME_OPT_DIFFICULTY 0x03   /* row 0 — "Game Difficulty" */
#define NEWGAME_OPT_AUTO_GUARD 0x04   /* row 1 — "Auto-guard"      */
#define NEWGAME_OPT_START_GAME 0x1e   /* row 2 — "Start Game" (kind 3 button) */

/* The current option settings the value column reflects.  In retail these are
 * read per-row from the settings record (*DAT_008a6e80 + 0xc + id*0x1c); here
 * they are passed in so the build stays pure.  The captured golden was taken
 * at the menu's defaults: difficulty 10 ("1:Easy"), auto-guard 1 ("On"). */
typedef struct newgame_settings {
    int32_t difficulty;   /* option id 3 — 10 = Easy (default) */
    int32_t auto_guard;   /* option id 4 — 1 = On  (default)   */
} newgame_settings;

/* The retail menu defaults (what the golden was captured at). */
#define NEWGAME_SETTINGS_DEFAULT { 10, 1 }

/* ─── FUN_00564780 case 0x24 — build the new-game config grid ─────────────
 *
 * Reconstructs the case-0x24 menu grid into `grid` and stamps `node` with the
 * display config the renderer reads, so glyph_grid_render(node, …, x=32, y=32)
 * emits exactly retail's TextOutA stream.  The sequence mirrors retail:
 *
 *   1. 0x411940 → menu_ctrl_build(grid, 0x28, 0x18, alloc_a=3, alloc_b=2,
 *      stride=3, type=0): a 3-row × 2-col linear grid; the 0x28/0x18 become
 *      the node's field_c/field_10 text-inset (40,24).
 *   2. case-0x24 entry override: the value column's entry[1].pos = 0xa0 (x
 *      offset 160) — FUN_00564780 case 0x24's `entries+0x24 = 0xa0` (and the
 *      +0x28/+0x2c/+0x30 zeros).
 *   3. three labelled rows (menu_grid_append): (0, id 3, "Game Difficulty"),
 *      (0, id 4, "Auto-guard"), (3, id 0x1e, "Start Game").
 *   4. value fill (the run loop's first block): for every kind-0 row, lay the
 *      option's current value (newgame_option_value) into column 1.
 *
 * The node fields (display colours, field_c/field_10, field_1ac row pitch)
 * are the values 0x411940's menu_node_build + menu_ctrl_build leave on the
 * rendered child node; on the 32-bit target `node` and `grid` are one 0x1b0
 * allocation, on the 64-bit host they are split (their layouts diverge past
 * +0x174) exactly as the renderer's other host tests model the aliasing.
 *
 * Pure / Win32-free.  After it returns, `grid` owns the container + laid-out
 * glyph buffers (free via menu_ctrl_clear) and `node` references grid's arrays.
 */
void newgame_config_build(menu_ctrl *grid, menu_node *node,
                          const newgame_settings *settings);

#endif /* OPENSUMMONERS_NEWGAME_MENU_H */
