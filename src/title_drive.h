/*
 * src/title_drive.h — the title-scene drive (the caller side of FUN_0056aea0).
 *
 * title_scene_step (title_scene.c) is one iteration of FUN_0056aea0's outer
 * do/while; title_sink (title_sink.c) turns its draw-command stream into real
 * ZDD blits.  What was still missing to make "the title scene runs in the real
 * window" was the *caller* — the plumbing FUN_0056aea0's caller FUN_00562ea0
 * owns: allocate the scene's owning object graph (the sel_list menu-tree owner
 * + its input manager), bind the render sink to the live primary surface, and
 * drive the per-frame loop until the scene returns its menu-action code.
 *
 * This module is that caller, factored Win32-free so it host-tests like every
 * other port: the present / log / scene side-effect calls go through callbacks
 * the real build (main.c) fills with zdd_present / log_line / the engine pump,
 * and the host tests fill with spies.  main.c owns only the Win32 thunks and
 * the GetTickCount sampling; everything structural lives here.
 *
 * Lifecycle:
 *     title_drive_init(&d, &cfg);          // alloc owner+node, bind the sink
 *     while (running) {
 *         uint32_t now = GetTickCount();    // caller samples the clock
 *         if (title_drive_step(&d, now) == TITLE_SCENE_DONE) break;
 *     }
 *     // d.result holds the scene's menu-action / abort code
 *     title_drive_shutdown(&d);            // unbind sink, free the object graph
 *
 * The object graph mirrors what FUN_00562ea0 hands FUN_0056aea0: a sel_list
 * whose entry[0] is an allocated 0x1b0 menu-tree node (the slot title_menu_spawn
 * configures on the first menu frame).  We allocate it up front — exactly as
 * retail's caller does via operator_new — so the lazy phase-8 menu spawn has a
 * live node to build on rather than dereferencing NULL.
 *
 * The 8d per-cell surface builder (ar_frame_build_hook) is deliberately NOT
 * installed here: with it NULL every sprite resolves to NULL and the sink draws
 * a cleared + flipped window with no sprites (HANDOFF.md "move B") — the
 * proving-the-loop-live step.  Installing ar_sprite_decode_hook so the banks
 * self-decode once registered is the real build's job (main.c), left out of the
 * drive so the host tests stay free of the asset-pool globals.
 */
#ifndef OPENSUMMONERS_TITLE_DRIVE_H
#define OPENSUMMONERS_TITLE_DRIVE_H

#include <stdint.h>

#include "title_scene.h"   /* title_scene / title_scene_hooks / status        */
#include "title_sink.h"    /* title_sink_ctx / title_render_sink               */
#include "obj_container.h" /* sel_list                                          */
#include "input.h"         /* input_mgr                                         */

/* What the drive needs to wire the scene to the live engine.  All callbacks
 * and the hooks pointer are optional (NULL ⇒ that side effect no-ops), so a
 * headless / pre-DDraw drive degrades to a faithful all-no-op run. */
typedef struct title_drive_cfg {
    /* The blit dest — retail DAT_008a93cc->primary_obj (g_zdd->primary_obj).
     * NULL ⇒ the sink no-ops every draw (faithful pre-DDraw-init behaviour). */
    zdd_object *primary;

    /* TITLE_DRAW_FLIP → present the frame (retail FUN_005b8fc0 = zdd_present).
     * NULL ⇒ no present (the --no-present / headless path). */
    void (*present)(void *user);

    /* TITLE_DRAW_LOG_FLIPPING → the "Title Menu - Flipping" log marker. */
    void (*log_flip)(void *user);

    void *user;                       /* opaque, passed to present / log_flip   */

    /* The 0x8a92b8 / 0x8a9308 alpha ramps the compositor + sprite-level draws
     * blend through.  NULL is fine — unfilled at a cold boot (all-zero ⇒ plain
     * blits), which is the static-image reality until asset init populates them. */
    const zdd_blend_desc *const *ramp_a;   /* 0x8a92b8 — compositor + cursor    */
    const zdd_blend_desc *const *ramp_b;   /* 0x8a9308 — sprite-level + logo    */
    const uint32_t              *fade_ramp;/* 0x8a9308 as the render-step u32 ramp */

    /* The scene's sprite-group display list (TITLE_DRAW_FRAME_END arg).  NULL
     * ⇒ the compose step is skipped (an unmodeled display list composes nothing
     * — faithful; the group is not yet traced, see HANDOFF.md). */
    const title_sprite_group *compose_group;

    /* The still-unported per-frame engine calls in the outer loop (pump /
     * pre / post / per-entry / BGM cue / sparkle).  NULL ⇒ all no-op. */
    const title_scene_hooks *hooks;

    /* title_scene_init args (see title_scene.h). */
    int32_t                         select_key;  /* the saved menu pick         */
    int                             quiet;       /* log-suppress flag           */
    const title_menu_savedata_list *savedata;    /* commit-path notify table    */
    int                             skip_intro;  /* nonzero ⇒ a press skips intro
                                                  *          even from phase 0   */

    /* Owner sel_list capacity (entry-pointer slots).  Only entry[0] is used by
     * the title menu; 0 selects a sensible default (TITLE_DRIVE_OWNER_CAP). */
    int owner_cap;
} title_drive_cfg;

#define TITLE_DRIVE_OWNER_CAP 4

/* The running drive: the scene + its owned object graph + the bound sink ctx.
 * Treat as opaque; read only `done` / `result` after a step. */
typedef struct title_drive {
    title_scene    scene;
    sel_list       owner;
    input_mgr      input;
    title_sink_ctx sink;

    /* Backing store for the input manager's 64-slot ring.  Retail's input
     * manager always points each ring slot at an owned event record; the
     * poll/scan paths (input.c) deref m->ring[i] with no NULL guard, so we
     * own a parallel array of idle records and point the ring at them.  The
     * DInput producer that would fill them is unported + the window is silent,
     * so an all-idle ring is the faithful "no input" state — the intro plays
     * through untouched. */
    input_event    ring_store[INPUT_RING_LEN];

    /* Owned allocations (freed by title_drive_shutdown). */
    sel_entry    **entries;   /* owner.entries — the entry-pointer array        */
    void          *node0;     /* entries[0] — the allocated 0x1b0 menu-tree node */

    int      started;         /* init ran                                       */
    int      done;            /* last step returned TITLE_SCENE_DONE            */
    int32_t  result;          /* the scene's return code (valid once done)      */

    /* Cached for the FRAME_END fade ramp the render step blends through. */
    const uint32_t *fade_ramp;

    /* The scene hooks passed to title_scene_step each frame.  Borrowed — the
     * caller's hooks struct must outlive the drive (main.c uses a static). */
    const title_scene_hooks *scene_hooks_holder;
} title_drive;

/* Allocate the scene's object graph, bind the render sink to cfg, and zero the
 * scene FSM (title_scene_init).  Installs title_render_sink_hook =
 * title_render_sink.  Returns 1 on success, 0 on allocation failure (in which
 * case nothing is bound and no cleanup is owed).  cfg is consumed by value. */
int title_drive_init(title_drive *d, const title_drive_cfg *cfg);

/* Run one iteration of the scene loop with this iteration's clock sample `now`.
 * Returns TITLE_SCENE_RUNNING to keep going or TITLE_SCENE_DONE once the scene
 * finished (d->result then holds the menu-action / abort code).  Idempotent
 * after DONE (returns DONE without re-running the scene). */
title_scene_status title_drive_step(title_drive *d, uint32_t now);

/* Unbind the sink (title_sink_bind(NULL) + title_render_sink_hook = NULL) and
 * free the object graph the menu spawn / drive allocated.  Safe to call on a
 * never-initialised or already-shutdown drive (no double free). */
void title_drive_shutdown(title_drive *d);

#endif /* OPENSUMMONERS_TITLE_DRIVE_H */
