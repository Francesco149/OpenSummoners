/*
 * src/newgame_picker.h — the new-game config scene's option PICKER submenu
 * (the Win32-free model of FUN_00567ba0's default arm).
 *
 * When the run loop's confirm (0xc) lands on a kind-0 option row,
 * FUN_00564780 opens that option's picker: a nested grid listing the option's
 * value choices, with its own 0x565d10 nav loop.  Picking a value commits it
 * (FUN_005657f0 → the settings record) and the parent re-lays that row's value
 * column (already modelled as newgame_scene_set_option); backing out leaves the
 * option unchanged.  newgame_scene_dispatch surfaces NEWGAME_OPEN_PICKER for the
 * focused option; this module is the submenu that request runs.
 *
 * Factored like newgame_scene: the pure state machine (build the value grid,
 * seek the cursor to the current value, nav + commit/cancel) lives here and
 * host-tests; the Win32 frame pump (the real 0x565d10) is the drive's job — the
 * drive feeds the same pump result codes (NEWGAME_PUMP_*) into the picker that
 * it feeds the parent scene.
 *
 * The picker grid (FUN_00567ba0 default arm, 567ba0.c:29-45):
 *   uVar2 = FUN_00568320(id, vals)        — the value-code list + count
 *   if (uVar2 == 0) return 0               — no choices → caller stays put
 *   FUN_00566570(id, buf)                  — (the option's label scratch)
 *   FUN_00411940(this, 0x120,0x80,0x100,…) — a 1-column grid box at (288,128)
 *   for each value: FUN_00566a80(id,val) → FUN_00412160(...)  — append a row
 *   FUN_00419900(0, current_value)         — seek the cursor to the current value
 *   <nav loop>: 0xb → return 0 (cancel); 0xc → FUN_005657f0(commit), return 0xc
 *
 * RECONSTRUCTED (documented, pending a live confirm): the picker's row-append
 * `kind` (FUN_00412160 arg, lost by the decompiler) and the FUN_00419900 /
 * FUN_005657f0 argument lists are not directly readable in 567ba0.c (Ghidra
 * dropped the register/stack args of those __thiscall calls).  They are
 * reconstructed from the callees' own contracts: FUN_00419900(field0,action)
 * seeks the row whose (kind,id) match, so the seek is (0, current_value) over
 * kind-0 value rows; FUN_005657f0(id,value) writes settings[id]=value, so the
 * commit is (option_id, selected_value).  The retail flip counter freezes inside
 * 0x565d10's modal pump, so a live golden of the open picker is not reachable
 * with the current harness — the render geometry (288,128) and these args are
 * an OPEN verification gate (see HANDOFF / docs/findings/new-game-flow.md).
 *
 * Pure / Win32-free: pointer/integer state over the menu_ctrl/menu_node model
 * plus the already-ported nav engine and option-string providers.
 */
#ifndef OPENSUMMONERS_NEWGAME_PICKER_H
#define OPENSUMMONERS_NEWGAME_PICKER_H

#include <stdint.h>

#include "menu_list.h"      /* menu_ctrl / menu_node / menu_input_sub / nav   */
#include "newgame_menu.h"   /* newgame_option_value / settings ids            */

/* The most value choices any new-game option offers: Game Difficulty's
 * unlock-enabled list {10,20,30,40,50}.  Auto-guard uses 2.  The full
 * FUN_00568320 switch goes wider for the screen/sound/team menus (up to 11
 * for the volume sliders), but only ids 3/4 are reachable from case 0x24. */
#define NEWGAME_PICKER_MAX_VALUES 11

/* ─── FUN_00568320 — option id → value-code list ─────────────────────────
 *
 * Fills `out` (>= NEWGAME_PICKER_MAX_VALUES int32) with the option's selectable
 * value codes and returns the count (0 = no picker).  Only the new-game config
 * arms are ported:
 *   id 3 (Game Difficulty): {10,20,30,40} (count 4), or {10,20,30,40,50}
 *        (count 5) when `difficulty_unlock` is non-zero — retail reads the
 *        unlock from *(*DAT_008a6e80 + 0xaa4); passed in to stay pure.
 *   id 4 (Auto-guard):      {0,1} (count 2).
 * Any other id returns 0 (the unported arms belong to the screen/sound/team
 * settings menus, several of which read god-object globals — out of scope for
 * the new-game scene).  Faithful to the id 3 / id 4 arms of 0x568320. */
int32_t newgame_picker_values(int32_t id, int32_t *out, int32_t difficulty_unlock);

/* What one picker dispatch resolves to. */
typedef enum newgame_picker_status {
    NEWGAME_PICKER_RUNNING = 0,  /* keep the picker loop going (re-render)      */
    NEWGAME_PICKER_COMMIT,       /* a value was chosen (newgame_picker_chosen)  */
    NEWGAME_PICKER_CANCEL,       /* backed out (0xb) — leave the option as-is   */
} newgame_picker_status;

/* The picker's owned state.  Like newgame_scene, grid/node alias one 0x1b0
 * object on the 32-bit target; the host splits them.  `sub` is the input gate
 * the drive ramps open (shared with the parent's gate semantics). */
typedef struct newgame_picker {
    menu_ctrl       grid;
    menu_node       node;
    menu_input_sub  sub;
    int32_t         option_id;       /* the option being edited (3 or 4)        */
    int32_t         values[NEWGAME_PICKER_MAX_VALUES]; /* the value-code list   */
    int32_t         count;           /* number of value rows                    */
    int32_t         chosen;          /* committed value (valid on COMMIT)        */
    int             started;
} newgame_picker;

/* Build the value grid for `option_id`, lay each value's label, and seek the
 * cursor to the row carrying `current_value` (FUN_00419900).  `difficulty_unlock`
 * gates difficulty's 4th→5th choice (see newgame_picker_values).  Returns 1 if
 * the picker has at least one choice (it should be run), 0 if FUN_00568320
 * yields none (the caller stays on the menu, as retail's `return 0` does). */
int newgame_picker_init(newgame_picker *p, int32_t option_id,
                        int32_t current_value, int32_t difficulty_unlock);

/* Free the grid's container + laid-out glyph buffers.  Safe on a never-built /
 * already-cleared picker. */
void newgame_picker_clear(newgame_picker *p);

/* The value code on the currently-focused row (grid.list->cursor). */
int32_t newgame_picker_focused_value(const newgame_picker *p);

/* Apply one pump result (NEWGAME_PUMP_*) to the picker and return the outcome.
 * The nav loop (567ba0.c:237-251): 0xb → CANCEL (return 0), 0xc → COMMIT (read
 * the focused value into `chosen`, the FUN_005657f0 commit), 0xd → re-render.
 * Anything else keeps the loop running (faithful: the loop re-iterates on any
 * code that is not 0xb/0xc/0xd). */
newgame_picker_status newgame_picker_dispatch(newgame_picker *p, int32_t pump_result);

#endif /* OPENSUMMONERS_NEWGAME_PICKER_H */
