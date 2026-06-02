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
        uint32_t dir = (mgr->axis_held[0] != 0) ? 6 : 7;   /* +0x114 vertical */
        esi = menu_list_latch(ctrl, dir, now);
    }
    if (input_poll_consume(mgr, now, 3))    esi = menu_list_latch(ctrl, 1, now);
    if (esi == 0) {                                  /* 0x56b8ab */
        uint32_t dir = (mgr->axis_held[1] != 0) ? 4 : 5;   /* +0x118 horizontal */
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

/* ─── (E) the render half ────────────────────────────────────────────
 *
 * Port of FUN_0056aea0's render branch (disasm 0x56bb04..0x56bf1a; see
 * title_scene.h for the framing, the recovered jump table, and the
 * draw-sink rationale).  Drives the prologue gating + the 11-entry
 * jump-table dispatch + the shared frame-end, emitting each unported
 * draw bridge as a tagged command through title_render_sink_hook.
 */

title_render_sink_fn title_render_sink_hook = NULL;

static void title_emit(title_draw_op op, int32_t asset, int32_t level,
                       int32_t alpha, int32_t x, int32_t y)
{
    if (title_render_sink_hook != NULL) {
        title_draw_cmd cmd = { op, asset, level, alpha, x, y };
        title_render_sink_hook(&cmd);
    }
}

/* 0x448c80: idx = (value*20)/divisor; ramp[idx] for 0<=idx<20, else 0. */
int32_t title_fade_ramp(int32_t value, int32_t divisor, const uint32_t *ramp)
{
    int32_t idx;
    if (divisor <= 0) return 0;                 /* 0x448c86 (jle)            */
    idx = (value * 20) / divisor;               /* (value*5)<<2 / divisor    */
    if (idx < 0 || idx >= 0x14) return 0;       /* 0x448c97 / 0x448c9c (>=20)*/
    return ramp != NULL ? (int32_t)ramp[idx] : 0;  /* [idx*4 + 0x8a9308]      */
}

/* The two logo handlers (0x56bb5c studio / 0x56bbd4 title) — identical but
 * for the sprite field offset (+4 vs +8).  fade<=0 draws nothing; otherwise
 * the ramp picks alpha: 0 → a surface clear (logo not yet visible), nonzero
 * → the alpha logo blit. */
static void title_render_logo(int32_t fade, const uint32_t *ramp, int32_t sprite_off)
{
    int32_t alpha;
    if (fade <= 0) return;                       /* 0x56bb86 / 0x56bbff (jle) */
    alpha = title_fade_ramp(fade, 1000, ramp);   /* 0x448c80(fade,1000)       */
    if (alpha == 0)
        /* The alpha-0 path is a plain blt_keyed of the logo frame itself
         * (frames[1] studio / frames[2] title), NOT the phase-2..3
         * background frame[0] — so carry the frame index (sprite_off/4 =
         * 1 or 2) in `asset` so the sink resolves the right sprite.
         * Disasm 0x56bba0 (studio) / 0x56bc19 (title): the je/jne-0 branch
         * blits `esi = entries[0].frames[sprite_off/4]`. */
        title_emit(TITLE_DRAW_SURFACE_CLEAR, sprite_off / 4, 0, 0, 0, 0); /* 0x5b9b70 */
    else
        title_emit(TITLE_DRAW_LOGO, sprite_off, 0, alpha, 0, 0);  /* 0x494e10 */
}

/* The two "press button" handlers (0x56bc4d phase 5 / 0x56bca2 phase 6):
 * a plain sprite then a fade-levelled sprite, over consecutive asset ids. */
static void title_render_pressbtn(int32_t fade, int32_t asset_a, int32_t asset_b)
{
    title_emit(TITLE_DRAW_SPRITE,       asset_a, 0,    0, 0, 0);   /* 0x56c610 */
    title_emit(TITLE_DRAW_SPRITE_LEVEL, asset_b, fade, 0, 0, 0);   /* 0x56c4e0 */
}

/* The sparkle flourish (0x56bcf7, phase 7): a plain sprite (asset 4) then a
 * trailing row of asset-5 sparkles.  The trail starts at level 7*fade and x
 * 192, stepping x by 4 (cap 416) and level down by 100 each sparkle; a
 * sparkle is drawn only while level>0, at alpha = ramp(min(level,1000)). */
static void title_render_sparkle(int32_t fade, const uint32_t *ramp)
{
    int32_t level, x;
    title_emit(TITLE_DRAW_SPRITE, 4, 0, 0, 0, 0);    /* 0x56c610(asset 4)     */
    level = (fade * 7000) / 1000;                    /* 0x56bd2e..0x56bd59    */
    for (x = 0xc0; x < 0x1a0; x += 4) {              /* 192 .. <416, step 4   */
        if (level > 0) {                             /* 0x56bd5b (jle skip)   */
            int32_t v     = level < 1000 ? level : 1000;       /* min(.,1000) */
            int32_t alpha = title_fade_ramp(v, 1000, ramp);    /* 0x448c80    */
            title_emit(TITLE_DRAW_SPARKLE, 5, 0, alpha, x, 0); /* 0x56c580    */
        }
        level -= 0x64;                               /* 0x56bda9 (sub 100)    */
    }
}

/* The menu handler (0x56bdb9, phases 8/9): an optional background sprite
 * (asset 5, only while fading in), the fade-levelled menu sprite (asset 6),
 * and — once fully faded in and a controller exists — the selected row's
 * cursor highlight at y = 16 + cursor*32. */
static void title_render_menu(int32_t fade, menu_ctrl *ctrl)
{
    int32_t cursor, i, y;
    if (fade < 1000)                                  /* 0x56bdb9 (jge skips) */
        title_emit(TITLE_DRAW_SPRITE, 5, 0, 0, 0, 0); /* 0x56c610(asset 5) bg */
    title_emit(TITLE_DRAW_SPRITE_LEVEL, 6, fade, 0, 0, 0);  /* 0x56c4e0(asset 6) */

    if (ctrl == NULL) return;                         /* 0x56be20 (je)        */
    if (fade != 1000) return;                         /* 0x56be28 (jne)       */

    cursor = ctrl->list->cursor;                      /* word [list+0x14]     */
    for (i = 0, y = 0x10; i < 5; i++, y += 0x20) {    /* 0x56be3b loop, 5 rows */
        if (i == cursor)                              /* draw only the cursor row */
            title_emit(TITLE_DRAW_MENU_CURSOR, i, 0x4b0, 0, 0, y);  /* 0x56c470 */
    }
}

/* The menu fade-out (0x56be85, phase 10): a surface reset then the
 * fade-levelled menu sprite (asset 6). */
static void title_render_fadeout(int32_t fade)
{
    title_emit(TITLE_DRAW_SURFACE_RESET, 0, 0, 0, 0, 0);    /* 0x5b9410       */
    title_emit(TITLE_DRAW_SPRITE_LEVEL, 6, fade, 0, 0, 0);  /* 0x56c4e0(asset 6) */
}

void title_render_step(int32_t phase, int32_t fade, menu_ctrl *ctrl,
                       const uint32_t *ramp, int quiet, int *already_flipped)
{
    /* (1) Prologue (0x56bb04).  Phase 0 resets the surface; phases 2..3
     *     clear it; both fall through to the dispatch.  (Phase 0's reset is
     *     at 0x56bbbb, reached by the `je` at the top; the 2..3 clear is the
     *     `1 < phase < 4` block at 0x56bb10.) */
    if (phase == 0)
        title_emit(TITLE_DRAW_SURFACE_RESET, 0, 0, 0, 0, 0);   /* 0x5b9410    */
    else if (phase >= 2 && phase <= 3)
        title_emit(TITLE_DRAW_SURFACE_CLEAR, 0, 0, 0, 0, 0);   /* 0x5b9b70    */

    /* (2) Dispatch (0x56bb4c bounds `cmp phase,0xa; ja` → skip; else the
     *     0x56bb55 jump table).  Phase > 10 draws nothing and falls to the
     *     frame-end. */
    if (phase <= 10) {
        switch (phase) {
        case 0: case 1: case 2:
            title_render_logo(fade, ramp, 4);     break;   /* 0x56bb5c studio */
        case 3: case 4:
            title_render_logo(fade, ramp, 8);     break;   /* 0x56bbd4 title  */
        case 5:  title_render_pressbtn(fade, 2, 3); break; /* 0x56bc4d        */
        case 6:  title_render_pressbtn(fade, 3, 4); break; /* 0x56bca2        */
        case 7:  title_render_sparkle(fade, ramp);  break; /* 0x56bcf7        */
        case 8: case 9:
            title_render_menu(fade, ctrl);        break;   /* 0x56bdb9        */
        case 10: title_render_fadeout(fade);      break;   /* 0x56be85        */
        default: break;                                    /* unreachable     */
        }
    }

    /* (3) Frame-end (0x56bec4): compose, log "Flipping" once, present. */
    title_emit(TITLE_DRAW_FRAME_END, 0, 0, 0, 0, 0);       /* 0x56c180        */
    if (!*already_flipped && !quiet)                       /* 0x56beda/0x56bee9 */
        title_emit(TITLE_DRAW_LOG_FLIPPING, 0, 0, 0, 0, 0);
    title_emit(TITLE_DRAW_FLIP, 0, 0, 0, 0, 0);            /* 0x5b8fc0(hWnd)  */
    *already_flipped = 1;                                  /* 0x56bf12 bVar3=1 */
}

/* ─── (F) the scene runner ────────────────────────────────────────────
 *
 * Port of FUN_0056aea0's outer `do { … } while(1)` body (disasm
 * 0x56b002..0x56ba75; see title_scene.h for the framing + the deferred
 * skip-splash seam).  One call = one loop iteration: pace → (render half) |
 * (update half).  Composes the ported units; every still-unported per-frame
 * engine call goes through the title_scene_hooks struct (the menu-input side
 * effects keep using the existing title_menu_*_hook globals).
 */

void title_scene_init(title_scene *ts, sel_list *owner, input_mgr *input,
                      int32_t select_key, const uint32_t *ramp, int quiet,
                      const title_menu_savedata_list *savedata, int skip_intro)
{
    title_pace_state_init(&ts->pace);   /* sub=0, budget=0x11, anchors=0 (0x56afd0) */
    title_fade_state_init(&ts->fade);   /* phase/fade/tick/menu_fade = 0 (0x56aec4) */
    ts->menu.node       = NULL;         /* local_54 = 0 */
    ts->menu.ctrl       = NULL;         /* local_60 = 0 */
    ts->watchdog        = 0;            /* local_50 = 0 */
    ts->result          = 0;           /* local_48 = 0 */
    ts->already_flipped = 0;           /* bVar3 = false */

    ts->owner      = owner;
    ts->input      = input;
    ts->select_key = select_key;
    ts->ramp       = ramp;
    ts->quiet      = quiet;
    ts->savedata   = savedata;
    ts->skip_intro = skip_intro;       /* param_1 */
}

title_scene_status title_scene_step(title_scene *ts, uint32_t now,
                                    const title_scene_hooks *hooks)
{
    /* (1) Pacing (0x56b002): decide pump + update/render for this iteration. */
    title_pace_step_out pout;
    title_pace_step(&ts->pace, now, &pout);
    if (pout.pump && hooks != NULL && hooks->pump != NULL)
        hooks->pump();                                      /* 0x5b1030 */

    /* (2) Render half (sub==1, 0x56bb04): draw + present, then loop.  Never
     *     exits — the only returns are in the update half below. */
    if (pout.action == TITLE_PACE_RENDER) {
        title_render_step(ts->fade.phase, ts->fade.fade, ts->menu.ctrl,
                          ts->ramp, ts->quiet, &ts->already_flipped);
        return TITLE_SCENE_RUNNING;
    }

    /* (3) Update half (sub==2, 0x56b0ce). */
    if (hooks != NULL && hooks->pre_update != NULL)
        hooks->pre_update();                                /* 0x43e140/0x40fe00/0x566250 */

    /* Abort poll (0x56b14d): a fresh 0x22 press returns scene state 6. */
    if (input_poll_consume(ts->input, now, 0x22)) {
        ts->result = 6;                                     /* local_48 = 6 */
        return TITLE_SCENE_DONE;                            /* → LAB_0056bf2e */
    }

    /* Skip-splash early-out (0x56b0e8..0x56b150): during the intro, a fresh
     * button press jumps straight to the menu.  Gate (0x56b107..0x56b117):
     * phase < 8, and either phase >= 1 or skip_intro (param_1) is set — so at
     * phase 0 the headless default (skip_intro == 0) never skips, but phases
     * 1..7 always honour a press. */
    if (ts->fade.phase < 8 && (ts->fade.phase != 0 || ts->skip_intro)) {
        if (input_any_fresh_press(ts->input, now)) {     /* 0x56b119..0x56b144 */
            ts->fade.fade = 0;                           /* uVar15 = 0 (0x56b18a) */

            /* phase < 3 (0x56b18c): the studio→title BGM hand-off hasn't fired
             * yet, so cue it now — the same SetNextSegment dance title_fade_step
             * runs at the 2→3 transition (0x56b197..0x56b259), condensed here to
             * the one hook. */
            if (ts->fade.phase < 3 && hooks != NULL && hooks->set_next_segment != NULL)
                hooks->set_next_segment();

            input_mgr_reset(ts->input);                  /* 0x56b25e..0x56b29a */
            ts->fade.phase = 8;                          /* local_64 = 8 (0x56b2a0) */
            /* Retail also zeros a scene-local sparkle counter here (var_3eh_2,
             * 0x56b2a8) — that belongs to the still-deferred sparkle-trail
             * subsystem (spawned/advanced outside this runner via the
             * spawn_sparkle hook), so there is nothing to reset in our state. */
        }
    }

    /* The phase switch (0x56b153).  phase > 10 skips it entirely (LAB_0056b14a
     * `if (10 < local_64) goto LAB_0056ba69`). */
    if (ts->fade.phase <= 10) {
        /* case 10 (0x56ba2e) drops the menu *before* the fade-out; idempotent,
         * so safe to call each phase-10 frame. */
        if (ts->fade.phase == 10)
            title_menu_teardown(&ts->menu);

        title_fade_step_out fx;
        title_fade_action act = title_fade_step(&ts->fade, &fx);

        if (fx.set_next_segment && hooks != NULL && hooks->set_next_segment != NULL)
            hooks->set_next_segment();                      /* BGM SetNextSegment cue */
        if (fx.spawn_sparkle && hooks != NULL && hooks->spawn_sparkle != NULL)
            hooks->spawn_sparkle(fx.sparkle_intensity);     /* 0x56c070 */

        if (act == TITLE_FADE_EXIT)
            return TITLE_SCENE_DONE;                         /* phase-10 fade done → ts->result */

        if (act == TITLE_FADE_MENU) {
            /* default arm (phases 8/9): spawn on first entry (local_60 == 0),
             * then run the per-frame menu input dispatch in the same frame. */
            if (ts->menu.ctrl == NULL)
                title_menu_spawn(ts->owner, ts->select_key, &ts->menu);
            if (ts->menu.ctrl != NULL) {                     /* see header: NULL guard */
                title_menu_input_out mout;
                title_menu_input_step(ts->input, ts->menu.ctrl, now,
                                      ts->watchdog, ts->savedata, &mout);
                if (mout.set_result)
                    ts->result = mout.result_code;           /* local_48 = row action */
                if (mout.enter_phase10) {                    /* commit or watchdog */
                    ts->fade.phase = 10;
                    ts->fade.fade  = 1000;
                }
            }
        }
    }

    /* (4) Per-frame tail (LAB_0056ba69), common to every non-exiting update
     *     frame regardless of phase: bump the idle watchdog (capped at 0x1194),
     *     run the post-update side effect, then update each owner entry. */
    if (ts->watchdog < TITLE_MENU_WATCHDOG_FRAMES)
        ts->watchdog++;                                     /* 0x56ba69 */
    if (hooks != NULL && hooks->post_update != NULL)
        hooks->post_update();                               /* 0x56c930 */
    if (ts->owner != NULL && hooks != NULL && hooks->update_entry != NULL) {
        uint16_t cnt = ts->owner->count;                    /* *(short*)(*in_ECX+6) */
        for (uint16_t i = 0; i < cnt; i++)
            hooks->update_entry((int32_t)i);                /* 0x43c2e0 per entry */
    }
    return TITLE_SCENE_RUNNING;
}
