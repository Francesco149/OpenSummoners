/*
 * src/newgame_scene.h — the new-game config scene's run-loop state model
 * (the Win32-free heart of FUN_00564780 case 0x24).
 *
 * newgame_menu.c builds the case-0x24 grid + render node and the renderer
 * (glyph_grid_render) walks it bit-exact (ckpt 37, quirk #64).  What was still
 * missing to make the scene *run* was the loop FUN_00564780 wraps the builder
 * in: pick the focused row's tooltip each frame, pump input, and act on the
 * pump's result (move the cursor / open an option picker / start the game /
 * back out).  This module is that loop, factored exactly like title_scene vs
 * title_drive: the pure state machine lives here and host-tests; the Win32
 * frame pump (the real 0x565d10 — its GDI present + the 0x43bca0 input
 * scan) is the drive's job (newgame_drive, main.c).
 *
 * The pump→action contract (the run-loop switch, 564780.c:597-669) — retail's
 * 0x565d10 collapses every per-frame outcome into one of three codes, and
 * FUN_00564780 dispatches on them:
 *
 *   0xd  cursor moved / page scrolled        → keep running (re-render)
 *   0xc  the confirm/OK button (id 0x24)     → act on the *focused* row:
 *          kind 0 (option) → open that option's picker submenu
 *                            (0x567ba0 default arm) — DEFERRED seam below
 *          kind 3 (0x1e "Start Game")        → 0x568b40(0x1e) is a no-op
 *                            for 0x1e, the action switch falls to case 0x24's
 *                            `goto teardown` → FUN_00564780 returns 0 → the
 *                            caller (0x564160) proceeds to the stone intro
 *   0xb  the back/cancel button (id 0x27)    → local_430!=0 ⇒ local_434=0xb,
 *                            teardown, FUN_00564780 returns 0xb (non-zero) →
 *                            0x564160 sets iVar3=6 → back to the title
 *
 * NB this corrects docs/findings/new-game-flow.md's earlier "id 0x27 = value
 * left/right" guess (flagged there as directionally unverified): in the
 * decompiled chain 0x43bca0 maps button 0x24 → menu_list_latch(9) (the
 * nav engine's "cancel" slot, which the scene reads as confirm → code 3 → 0xc)
 * and button 0x27 → menu_list_latch(10) ("confirm" slot → code 4 → 0xb, the
 * back path).  There is no in-place value toggle — an option's value changes
 * only by confirming into its picker submenu.  The physical-key identity of
 * ids 0x24/0x27 (what FUN_0043c110 reads them as) is the one piece still worth
 * a live Frida confirm; the decompile→code-path mapping above is unambiguous.
 *
 * DEFERRED seams (NOT modelled here, documented for the drive/next unit):
 *   - the option picker submenu (0x567ba0 default arm for id 3/4): a
 *     nested grid + its own 0x565d10 loop.  newgame_scene_dispatch returns
 *     NEWGAME_OPEN_PICKER so the caller can run it; once it commits a value,
 *     newgame_scene_set_option re-lays that row's value column (the run loop's
 *     value-refill block, 564780.c:367-385).
 *   - the box widget tree (0x411940's 0x40f3e0 box + tooltip box) and the
 *     generic compositor render — the drive's responsibility.
 *
 * Pure / Win32-free: pointer/integer state over the menu_ctrl/menu_node model
 * plus the already-ported builder, nav engine, and option-string providers.
 */
#ifndef OPENSUMMONERS_NEWGAME_SCENE_H
#define OPENSUMMONERS_NEWGAME_SCENE_H

#include <stdint.h>

#include "menu_list.h"      /* menu_ctrl / menu_node / menu_input_sub / nav   */
#include "newgame_menu.h"   /* newgame_config_build / newgame_settings        */

/* The pump-result codes 0x565d10 returns and FUN_00564780 dispatches on. */
#define NEWGAME_PUMP_BACK    0x0b   /* back/cancel (id 0x27) → return to title  */
#define NEWGAME_PUMP_CONFIRM 0x0c   /* confirm/OK (id 0x24) → act on focused row */
#define NEWGAME_PUMP_MOVE    0x0d   /* cursor moved / page → re-render          */

/* What one run-loop dispatch resolves to (the scene's externally-visible
 * outcome — the drive turns START/BACK into FUN_00564780's 0 / 0xb return). */
typedef enum newgame_scene_status {
    NEWGAME_RUNNING = 0,    /* keep the menu loop going (re-render this frame) */
    NEWGAME_START,          /* "Start Game" committed → begin the game         */
    NEWGAME_BACK,           /* backed out → re-display the title (result 0xb)  */
    NEWGAME_OPEN_PICKER,    /* confirm on an option row → run its picker submenu
                             * (0x567ba0); pick id is newgame_scene_focused_action */
} newgame_scene_status;

/* The scene's owned state.  `grid`/`node` are the builder's split container +
 * render node (on the 32-bit target they alias one 0x1b0 object; the host
 * splits them exactly as newgame_menu's tests model).  `sub` is the input-
 * ready gate the nav latch consults (the drive ramps sub.ready 0→1000, like
 * the title's menu_owner_transition_step); the model leaves it for the caller
 * to open. */
typedef struct newgame_scene {
    menu_ctrl        grid;
    menu_node        node;
    menu_input_sub   sub;        /* grid.sub points here */
    newgame_settings settings;   /* current option values (mirrors the record) */
    int32_t          param_case; /* the FUN_00564780 case this was built for   */
    int              started;    /* init ran (clear is then owed)              */
} newgame_scene;

/* Build the case-0x24 grid + node (newgame_config_build) and seed the loop
 * state: cursor on row 0, the input gate bound (but closed — sub.ready 0).
 * `settings` defaults to NEWGAME_SETTINGS_DEFAULT when NULL.  After this the
 * node renders bit-exact via glyph_grid_render at base (32,32). */
void newgame_scene_init(newgame_scene *s, const newgame_settings *settings);

/* Free the grid's container + laid-out glyph buffers (menu_ctrl_clear).  Safe
 * on a never-initialised / already-cleared scene. */
void newgame_scene_clear(newgame_scene *s);

/* The focused row (grid.list->cursor) and its kind / action id. */
int32_t newgame_scene_focused_row(const newgame_scene *s);
int32_t newgame_scene_focused_kind(const newgame_scene *s);
int32_t newgame_scene_focused_action(const newgame_scene *s);

/* Resolve the tooltip text for the focused row into `buf` (>= 512 B), exactly
 * as FUN_00564780 does each frame before rendering the tooltip node:
 *   kind 0 (option row)  → newgame_option_tooltip(action)   (FUN_00566850)
 *   kind 3 (action btn)  → the per-action tooltip switch (564780.c:415-595):
 *        0x1b "Save changes and exit."   0x1c "Reset to default settings."
 *        0x1d "Exit without saving."     0x1e + case 0x24 → the Start-Game help
 *   any other            → empty (the unmodelled &DAT_008a9b6c default). */
void newgame_scene_tooltip(const newgame_scene *s, char *buf);

/* Apply one pump result (NEWGAME_PUMP_*) to the scene and return the outcome.
 * This is the body of FUN_00564780's run-loop switch for case 0x24.  MOVE just
 * re-renders; CONFIRM acts on the focused row (option → OPEN_PICKER, Start Game
 * → START); BACK → BACK.  Unknown codes keep the loop running (faithful: the
 * real loop re-iterates on anything that is not 0xb/0xc, and 0xd re-renders). */
newgame_scene_status newgame_scene_dispatch(newgame_scene *s, int32_t pump_result);

/* Commit a new value for option `id` (the effect of its picker submenu, or a
 * default-reset): store it in `settings` and re-lay that row's value column
 * (newgame_option_value → glyph_cell_layout, the run loop's value-refill).
 * No-op if `id` is not one of this menu's option rows.  Returns 1 if a row's
 * value was refreshed, 0 otherwise. */
int newgame_scene_set_option(newgame_scene *s, int32_t id, int32_t setting);

#endif /* OPENSUMMONERS_NEWGAME_SCENE_H */
