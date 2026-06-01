/*
 * src/title_scene.c — intro-phase / menu-fade state machine, ported
 * from the `switch(local_64)` core of FUN_0056aea0 (title scene runner).
 *
 * Pure integer arithmetic; the only outward signals are reported via
 * the title_fade_step_out descriptor.  Per-function provenance and the
 * disassembly-level reasoning live in title_scene.h plus the per-case
 * comments below.
 *
 * Phase table (local_64), as recovered from the switch at 0x56b153 and
 * the per-phase jump-table handlers at 0x56bfa4 (see title-scene.md):
 *
 *   phase  what it animates                              advance when
 *   ----   ------------------------------------------    ---------------------
 *    0     studio-logo fade-in   (fade += 20)            fade reaches 1000
 *    1     hold studio logo      (tick++)                tick reaches 50
 *    2     studio-logo fade-out  (fade -= 20)            fade reaches 0  → seg cue
 *    3     title-logo fade-in    (fade += 10)            fade reaches 1000
 *    4     hold title logo       (tick++)                tick reaches 20
 *    5     "press button" in     (fade += 100, tick++)   tick reaches 40
 *    6     hold "press button"   (fade += 10,  tick++)   tick reaches 120
 *    7     sparkle flourish      (fade += 20, spawn)     tick reaches 90
 *    8     menu fade-in          (fade→1000, menu+=50)   menu_fade reaches 1000 → 9
 *    9     menu fade-out         (menu -= 50)            menu_fade reaches 0    → 8
 *   10     scene fade-out        (fade -= 20)            fade reaches 0  → EXIT
 *
 * Phases 8 and 9 are the steady-state menu (the engine's `default:`
 * switch arm): each frame first ramps the *main* fade to 1000, and only
 * once it is saturated does the menu cross-fade (menu_fade) breathe up
 * (8) and down (9).  The menu input/draw work that also happens every
 * frame in those phases is deferred (see title_scene.h).
 */
#include "title_scene.h"

/* Saturating subtract toward zero: max(x - d, 0).
 *
 * Retail implements this branchlessly at 0x56ba63 (case 2 / case 10
 * fade-out) and 0x56b57c (case 9 menu fade-out) as:
 *     lea  eax, [x - d]
 *     setl bl            ; bl = (x - d) < 0
 *     dec  ebx           ; -1 if >=0, 0 if <0
 *     and  ebx, eax      ; (x-d) if >=0, else 0
 * which is exactly max(x - d, 0) on signed values. */
static int32_t sat_sub(int32_t x, int32_t d)
{
    int32_t v = x - d;
    return v < 0 ? 0 : v;
}

void title_fade_state_init(title_fade_state *s)
{
    s->phase     = 0;
    s->fade      = 0;
    s->tick      = 0;
    s->menu_fade = 0;
}

title_fade_action title_fade_step(title_fade_state *s, title_fade_step_out *fx)
{
    fx->set_next_segment  = 0;
    fx->spawn_sparkle     = 0;
    fx->sparkle_intensity = 0;

    switch (s->phase) {

    /* case 0 @ 0x56b15e — studio-logo fade-in, +20/frame to 1000. */
    case 0:
        if (s->fade < 1000) {
            s->fade += 0x14;
            if (s->fade > 1000) s->fade = 1000;
        } else {
            s->phase = 1;
        }
        return TITLE_FADE_CONTINUE;

    /* case 1 @ 0x56b2c6 — hold studio logo for 50 frames. */
    case 1:
        if (s->tick < 0x32) s->tick++;
        else                s->phase = 2;
        return TITLE_FADE_CONTINUE;

    /* case 2 @ 0x56b2e6 — studio-logo fade-out, -20/frame to 0.  When
     * it reaches 0: reset tick, advance to phase 3, and fire the BGM
     * "SetNextSegment" cue (the 0x5bbc60/_90/_cd0/_20 + 0x5bcb80
     * block at 0x56b2f6, logged "Title Menu - SetNextSegment"). */
    case 2:
        if (s->fade > 0) {
            s->fade = sat_sub(s->fade, 0x14);
        } else {
            s->fade  = 0;
            s->tick  = 0;
            s->phase = 3;
            fx->set_next_segment = 1;
        }
        return TITLE_FADE_CONTINUE;

    /* case 3 @ 0x56b346 — title-logo fade-in, +10/frame to 1000. */
    case 3:
        if (s->fade < 1000) {
            s->fade += 10;
            if (s->fade > 1000) s->fade = 1000;
        } else {
            s->phase = 4;
        }
        return TITLE_FADE_CONTINUE;

    /* case 4 @ 0x56b36e — hold title logo for 20 frames, then reset
     * fade and advance to the "press button" phase. */
    case 4:
        if (s->tick < 0x14) {
            s->tick++;
        } else {
            s->fade  = 0;
            s->phase = 5;
            s->tick  = 0;
        }
        return TITLE_FADE_CONTINUE;

    /* case 5 @ 0x56b39a — "press button" fade-in, +100/frame (cap
     * 1000) while holding 40 frames, then phase 6. */
    case 5:
        if (s->fade < 1000) {
            int32_t v = s->fade + 100;
            s->fade = (v < 0x3e9) ? v : 1000;   /* min(fade+100, 1000) */
        }
        if (s->tick < 0x28) {
            s->tick++;
        } else {
            s->fade  = 0;
            s->phase = 6;
            s->tick  = 0;
        }
        return TITLE_FADE_CONTINUE;

    /* case 6 @ 0x56b46e — hold "press button" + ambient, +10/frame
     * (cap 1000) over 120 frames, then phase 7. */
    case 6:
        if (s->fade < 1000) {
            int32_t v = s->fade + 10;
            s->fade = (v < 0x3e9) ? v : 1000;   /* min(fade+10, 1000) */
        }
        if (s->tick < 0x78) {
            s->tick++;
        } else {
            s->fade  = 0;
            s->phase = 7;
            s->tick  = 0;
        }
        return TITLE_FADE_CONTINUE;

    /* case 7 @ 0x56b4ea — sparkle flourish.  fade += 20; while the
     * post-increment fade is < 850 (0x352) and still < 1001, request a
     * sparkle-particle spawn (retail 0x56c070) at intensity
     * (fade*0xe0)/900 + 0xc0; clamp fade to 1000 once it passes 1000.
     * Holds 90 frames, then hands off to the menu (phase 8). */
    case 7:
        if (s->fade < 1000) {
            s->fade += 0x14;
            if (s->fade < 0x3e9) {
                if (s->fade < 0x352) {
                    fx->spawn_sparkle     = 1;
                    fx->sparkle_intensity = (s->fade * 0xe0) / 900 + 0xc0;
                }
            } else {
                s->fade = 1000;
            }
        }
        if (s->tick < 0x5a) {
            s->tick++;
        } else {
            s->fade  = 0;
            s->phase = 8;
            s->tick  = 0;
        }
        return TITLE_FADE_CONTINUE;

    /* case 10 @ 0x56ba2e — scene fade-out at end of flow.  The menu
     * teardown (local_60/local_54/obj+0x50 reset) is the caller's job
     * (it owns the menu controller); here we only run the fade-out.
     * When fade has reached 0 the scene is done (retail jumps to the
     * cleanup+return path at 0x56bf2e). */
    case 10:
        if (s->fade < 1) return TITLE_FADE_EXIT;
        s->fade = sat_sub(s->fade, 0x14);
        return TITLE_FADE_CONTINUE;

    /* default @ 0x56b546 — phases 8 and 9 (steady-state menu).  Ramp
     * the main fade to 1000 first; only once saturated does the menu
     * cross-fade run: phase 8 ramps menu_fade up by 50 (→ phase 9 at
     * 1000), phase 9 ramps it down by 50 (→ phase 8 at 0).  After the
     * fade math, the caller runs the deferred menu/input update. */
    default:
        if (s->fade < 1000) {
            s->fade += 0x14;
            if (s->fade > 1000) s->fade = 1000;
        } else if (s->phase == 8) {
            if (s->menu_fade < 1000) {
                s->menu_fade += 0x32;
                if (s->menu_fade > 1000) s->menu_fade = 1000;
            } else {
                s->phase = 9;
            }
        } else if (s->phase == 9) {
            if (s->menu_fade < 1) s->phase = 8;
            else                  s->menu_fade = sat_sub(s->menu_fade, 0x32);
        }
        return TITLE_FADE_MENU;
    }
}

/* ─── (A) frame-pacing sub-state machine ─────────────────────────────
 *
 * Port of the `local_28` machine + the pump call sites at the top of
 * FUN_0056aea0's outer loop (radare2 disasm 0x56b002..0x56b0c8, raw
 * stack offsets).  See the title_pace_* block in title_scene.h for the
 * state map, the fixed-timestep-accumulator overview, and the rationale
 * for omitting the dead window/FPS-counter locals (D = local_20 and E
 * = [esp+0x5c]).
 *
 * All budget/anchor arithmetic is unsigned to match the engine's
 * `jbe`/`ja` comparisons (0x56b02d, 0x56b048, 0x56b05d, 0x56b076);
 * GetTickCount wraps mod 2^32 and the `(now - anchor)` deltas are
 * computed in that same modular arithmetic, so a 49.7-day rollover is
 * handled identically to retail.
 */

void title_pace_state_init(title_pace_state *s)
{
    s->sub           = 0;       /* local_28 = 0      (0x56afe0) */
    s->budget        = 0x11;    /* local_30 = 0x11   (0x56afd4) */
    s->tick_anchor   = 0;       /* local_34 = 0      (0x56afd0) */
    s->render_anchor = 0;       /* local_2c = 0      (0x56afdc) */
}

void title_pace_step(title_pace_state *s, uint32_t now, title_pace_step_out *out)
{
    out->pump = 0;

    if (s->sub == 0) {
        /* 0x56b07d — first frame: anchor the pump clock, pump once, and
         * drop straight into the update state. */
        s->tick_anchor = now;                       /* C = now            */
        out->pump      = 1;                          /* FUN_005b1030 @0x56b081 */
        s->sub         = 2;                          /* S = 2  @0x56b086   */
    } else if (s->sub == 1) {
        /* 0x56b051 — refill: add the wall-clock elapsed since the last
         * pump to the budget, clamp to 100 ms, re-anchor, pump, and
         * switch to updating once more than one 16 ms slice is banked. */
        uint32_t b = (s->budget - s->tick_anchor) + now;  /* B += now - C  */
        if (b > 100) b = 100;                             /* min(.,100) @0x56b05d */
        s->tick_anchor = now;                             /* C = now       */
        s->budget      = b;
        out->pump      = 1;                                /* FUN_005b1030 @0x56b071 */
        if (b > 0x10) s->sub = 2;                          /* B>16 → S=2 @0x56b076 */
    } else if (s->sub == 2) {
        /* 0x56b01f — spend the budget in 16 ms slices.  If real time has
         * already outrun the budget, drop straight to render; otherwise
         * burn one slice and render once down to the final slice. */
        if ((now - s->render_anchor) > s->budget) {        /* @0x56b02d (jbe) */
            s->budget = 0;
            s->sub    = 1;
        } else {
            s->budget -= 0x10;                             /* B -= 16        */
            if (s->budget <= 0x10) s->sub = 1;             /* B<=16 → S=1 @0x56b048 (ja) */
        }
    }

    /* Post-transition fix-up (0x56b08f..0x56b0be).  The retail `sub==1`
     * arm here only maintains the dead 1-second-window frame counter
     * (D / E) — omitted, see title_scene.h.  The `sub==2` arm re-anchors
     * the update clock, which is load-bearing: the next update frame
     * measures (now - render_anchor) against the budget at 0x56b02d. */
    if (s->sub == 2) {
        s->render_anchor = now;                            /* A = now @0x56b097 */
    }

    /* Dispatch (0x56b0be): sub==1 jumps to the render half (0x56bb04),
     * sub==2 falls through to the update half (0x56b0ce). */
    out->action = (s->sub == 1) ? TITLE_PACE_RENDER : TITLE_PACE_UPDATE;
}

/* ─── (C) the top-level menu spawn block ─────────────────────────────
 *
 * Port of FUN_0056aea0's default-arm spawn block (disasm 0x56b5cd..0x56b807;
 * see title_scene.h for the framing and the object-nesting it reveals).
 * Composes the already-ported menu primitives — menu_node_build,
 * sel_list_mark_last, obj_pool_acquire, menu_ctrl_build, menu_row_finalize,
 * menu_list_scroll_into_view — into the one-shot menu construction.
 */

/* The title top-level menu's five rows, appended in this order.  Each id is
 * the outer scene-driver state code that entry resolves to (title-scene.md
 * "Input dispatch"): 0x1a, 0x1c, 0x1e (options), 0x1d, 8 (exit). */
static const int32_t TITLE_MENU_ACTIONS[5] = { 0x1a, 0x1c, 0x1e, 0x1d, 8 };

void title_menu_spawn(sel_list *owner, int32_t select_key, title_menu *out)
{
    /* (1) Configure the owner's next free entry as this menu's tree node,
     *     give it one child, bump the count, mark it active, and stash it
     *     (retail local_54).  When the owner is full the node stays NULL —
     *     the degenerate path retail leaves to crash; the title flow never
     *     hits it. */
    menu_node *node = NULL;
    uint16_t count = owner->count;                  /* word [owner+6] */
    if (count < owner->capacity) {                  /* word [owner+4] */
        node = (menu_node *)owner->entries[count];  /* this = entries[count] */
        menu_node_build(node, owner, 0, 0, 100, 100, 1, NULL);  /* 0x40f3e0 */
        owner->count = (uint16_t)(owner->count + 1);            /* inc [owner+6] */
        sel_list_mark_last(owner);                              /* 0x414080 */
        node = (menu_node *)owner->entries[count];  /* re-read (count unchanged) */
    }
    out->node = node;

    /* (2) Acquire the controller — node->children[0].  Retail does this by
     *     reinterpreting the node as an obj_pool and calling obj_pool_acquire
     *     (0x412c10, ECX = node): the node's child array / capacity /
     *     count at +0x48 / +0x4c / +0x4e alias obj_pool.slots / capacity /
     *     count *exactly on the 32-bit target*.  Those two structs' paddings
     *     diverge on the 64-bit host (pointer fields widen to 8 B), so the
     *     reinterpret cast cannot find the pool header; we instead apply
     *     obj_pool_acquire's semantics to the node's own fields — byte-for-
     *     byte identical to the retail cast on 32-bit.  The stamp's owner
     *     write (slot+0) is the load-bearing one: it wires the controller's
     *     input-ready gate (menu_ctrl.sub, also at +0) to the node, whose
     *     +0x54 ramp later gates menu input through menu_list_latch; the
     *     index/+8 stamps land in fields menu_ctrl_build immediately rewrites.
     *     Then lay a 6×1 stride-6 linear-wrap grid on the controller. */
    menu_ctrl *ctrl = NULL;
    if (node != NULL && node->field_4e < node->child_count) {  /* count < capacity */
        pool_slot *slot = (pool_slot *)node->children[node->field_4e];  /* slots[count] */
        slot->owner    = node;                      /* slot+0 = pool (the node) */
        slot->index    = node->field_4e;            /* slot+4 (u16)             */
        slot->field8   = 0;                         /* slot+8                   */
        node->field_4e = (uint16_t)(node->field_4e + 1);  /* count++            */
        ctrl = (menu_ctrl *)slot;
    }
    out->ctrl = ctrl;
    if (ctrl != NULL) {
        menu_ctrl_build(ctrl, 0, 0, 6, 1, 6, 0);    /* 0x40f5c0 */
    }

    /* (3) Append the five fixed rows.  Each: guard count < alloc_a, write
     *     field0=0 / action=id / flag8=1, bump the header count, then
     *     finalize (a no-op on these fresh NULL-pointer cells). */
    for (int a = 0; a < 5; a++) {
        menu_list_hdr *hdr = ctrl->list;            /* [ctrl+0x174] */
        int32_t i = hdr->count;                     /* [hdr+0x10]   */
        if (i < hdr->alloc_a) {                     /* [hdr+0x04]   */
            hdr->count = i + 1;
            ctrl->rows[i].field0 = 0;               /* rows[i]+0 */
            ctrl->rows[i].action = TITLE_MENU_ACTIONS[a];  /* rows[i]+4 */
            ctrl->rows[i].flag8  = 1;               /* rows[i]+8 */
            menu_row_finalize(ctrl, i);             /* 0x411f40 */
        }
    }

    /* (4) Seek the cursor to the first row with field0==0 whose action
     *     matches the saved selection key, then page it into view.  The
     *     index counter advances full-width and is masked to 16 bits both
     *     as the row index and when stored to cursor (harmless < 0x10000). */
    int32_t n = ctrl->list->count;                  /* [hdr+0x10] */
    int32_t idx = 0;
    if (n > 0) {
        uint32_t k = 0;
        do {
            menu_row *row = &ctrl->rows[k];         /* rows + k*0x10 */
            if (row->field0 == 0 && row->action == select_key) {
                ctrl->list->cursor = (int32_t)((uint32_t)idx & 0xffff);  /* [hdr+0x14] */
                menu_list_scroll_into_view(ctrl);   /* 0x4192b0 */
                break;
            }
            idx++;
            k = (uint32_t)idx & 0xffff;
        } while ((int32_t)k < n);
    }
}

void title_menu_teardown(title_menu *m)
{
    if (m->ctrl != NULL) {                          /* if (local_60 != 0) */
        if (m->node != NULL) {
            m->node->field_50 = 0;                  /* *(local_54 + 0x50) = 0 */
        }
        m->ctrl = NULL;                             /* local_60 = 0 */
        m->node = NULL;                             /* local_54 = 0 */
    }
}

/* ─── (D) the per-frame menu input dispatch ──────────────────────────
 *
 * Port of FUN_0056aea0's default-arm per-frame menu update (disasm
 * 0x56b807..0x56ba39; see title_scene.h for the framing and the
 * action↔button mapping caveat).  Composes the ported poll/latch leaves
 * (input_poll_consume, menu_list_latch) into the nav resolution, then the
 * action switch + commit + idle watchdog, routing the unported side effects
 * through hooks.
 */

title_menu_sfx_fn      title_menu_sfx_hook      = NULL;
title_menu_joystick_fn title_menu_joystick_hook = NULL;
title_menu_savedata_fn title_menu_savedata_hook = NULL;
title_menu_watchdog_fn title_menu_watchdog_hook = NULL;

/* The menu's idle-watchdog threshold (retail `0x1193 < local_50`, i.e.
 * local_50 >= 0x1194).  ~75 s at 60 Hz. */
#define TITLE_MENU_WATCHDOG_FRAMES 0x1194

static void title_menu_sfx(int32_t id)
{
    if (title_menu_sfx_hook != NULL) title_menu_sfx_hook(id);  /* 0x411390(id,0,0) */
}

void title_menu_input_step(input_mgr *mgr, menu_ctrl *ctrl, uint32_t now,
                           int32_t watchdog_frames,
                           const title_menu_savedata_list *savedata,
                           title_menu_input_out *out)
{
    out->action        = 0;
    out->enter_phase10 = 0;
    out->set_result    = 0;
    out->result_code   = 0;

    /* (1) Poll the five buttons + the two axis-held syntheses, in retail
     *     order; each hit overwrites `esi` with the latch result, so the
     *     last latch that fires wins (0x56b80f..0x56b8e3). */
    int32_t esi = 0;
    if (input_poll_consume(mgr, now, 2))    esi = menu_list_latch(ctrl, 2, now);
    if (input_poll_consume(mgr, now, 4))    esi = menu_list_latch(ctrl, 3, now);
    if (input_poll_consume(mgr, now, 1))    esi = menu_list_latch(ctrl, 0, now);
    if (esi == 0) {                                  /* 0x56b871 */
        uint32_t dir = (mgr->axis_held_v != 0) ? 6 : 7;
        esi = menu_list_latch(ctrl, dir, now);
    }
    if (input_poll_consume(mgr, now, 3))    esi = menu_list_latch(ctrl, 1, now);
    if (esi == 0) {                                  /* 0x56b8ab */
        uint32_t dir = (mgr->axis_held_h != 0) ? 4 : 5;
        esi = menu_list_latch(ctrl, dir, now);
    }
    if (input_poll_consume(mgr, now, 0x24)) esi = menu_list_latch(ctrl, 9, now);
    out->action = esi;

    /* (2) Action switch (0x56b8ed).  `commit` records whether we reached the
     *     enabled-confirm path that 0x56b93f gates on (esi == 3 with the
     *     selected row enabled); a disabled row plays the denied SFX and
     *     skips straight to the watchdog. */
    int commit = 0;
    switch (esi) {
    case 1:                                          /* cursor moved */
    case 2:                                          /* page scrolled */
        title_menu_sfx(9);
        break;
    case 3: {                                        /* commit (from latch dir 9) */
        menu_row *sel = &ctrl->rows[ctrl->list->cursor];   /* rows[cursor] */
        if (sel->flag8 != 0) {                       /* row enabled → confirm */
            title_menu_sfx(5);
            commit = 1;
        } else {                                     /* row disabled → denied */
            title_menu_sfx(6);
        }
        break;
    }
    case 4:                                          /* cancel SFX (unreachable in title) */
        title_menu_sfx(7);
        break;
    default:                                         /* 0 / nav no-op / mode-2 codes */
        break;
    }

    /* (3) Commit path (0x56b93f, esi == 3 and the row was enabled). */
    if (commit) {
        /* Joystick lazy-attach (0x56b948..0x56b97c). */
        if (title_menu_joystick_hook != NULL) title_menu_joystick_hook();

        /* Save-data notify (0x56b97e..0x56b9e1): unless the selected action is
         * 0x1d, scan the god object's menu-action table for a key match and
         * notify the matched record.  base==0 / count==0 → skip. */
        int32_t act = ctrl->rows[ctrl->list->cursor].action;   /* rows[cursor].action */
        if (act != 0x1d && savedata != NULL && savedata->entries != NULL) {
            for (uint16_t i = 0; i < savedata->count; i++) {
                if (savedata->entries[i].key == act) {
                    if (title_menu_savedata_hook != NULL)
                        title_menu_savedata_hook(savedata->entries[i].arg);  /* 0x41bb80(0x5e,arg) */
                    break;
                }
            }
        }

        /* Commit the transition (0x56b9e3): phase 10, fade 1000, return the
         * selected row's action id (re-read, as retail does). */
        out->enter_phase10 = 1;
        out->set_result    = 1;
        out->result_code   = ctrl->rows[ctrl->list->cursor].action;
    }

    /* (4) Idle watchdog (LAB_0056ba0e): always checked, even with no input. */
    if (watchdog_frames >= TITLE_MENU_WATCHDOG_FRAMES) {
        if (title_menu_watchdog_hook != NULL) title_menu_watchdog_hook();  /* 0x40a5d0(0,0,0,0,1) */
        out->enter_phase10 = 1;
    }
}
