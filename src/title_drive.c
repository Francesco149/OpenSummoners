/*
 * src/title_drive.c — the title-scene drive (see title_drive.h).
 *
 * The caller side of FUN_0056aea0: owns the scene object graph FUN_00562ea0
 * hands the runner (a sel_list menu-tree owner + its input manager), binds the
 * render sink to the live primary surface, and drives title_scene_step until
 * the scene returns its menu-action code.  Pure C / Win32-free — the present /
 * log / engine-pump side effects all arrive through the cfg callbacks.
 */

#include <stdlib.h>
#include <string.h>

#include "title_drive.h"
#include "menu_list.h"     /* menu_node (entry[0] shape) / menu_ctrl_clear */

int title_drive_init(title_drive *d, const title_drive_cfg *cfg)
{
    int cap;

    memset(d, 0, sizeof *d);

    /* The owner sel_list: a pointer array whose entry[0] is an allocated
     * 0x1b0 menu-tree node — the slot the lazy phase-8 title_menu_spawn
     * configures (retail's caller allocates it via operator_new up front; we
     * mirror that so the spawn has a live node rather than a NULL deref). */
    cap = cfg->owner_cap > 0 ? cfg->owner_cap : TITLE_DRIVE_OWNER_CAP;
    d->entries = (sel_entry **)calloc((size_t)cap, sizeof(sel_entry *));
    if (d->entries == NULL)
        return 0;
    d->node0 = calloc(1, sizeof(menu_node));
    if (d->node0 == NULL) {
        free(d->entries);
        d->entries = NULL;
        return 0;
    }
    d->entries[0]    = (sel_entry *)d->node0;
    d->owner.entries = d->entries;
    d->owner.capacity = (uint16_t)cap;
    d->owner.count    = 0;

    /* A fresh, zeroed input manager with its 64-slot ring pointed at the
     * drive's owned idle records.  The poll/scan paths deref ring[i] with no
     * NULL guard (retail's ring is always populated), so every slot must point
     * at a real record; all-idle = the faithful "no input" state. */
    memset(&d->input, 0, sizeof d->input);
    memset(d->ring_store, 0, sizeof d->ring_store);
    for (int i = 0; i < INPUT_RING_LEN; i++)
        d->input.ring[i] = &d->ring_store[i];

    /* Zero the scene FSM + bind the environment (0x56aec4..0x56aef1 / 0x56affc). */
    title_scene_init(&d->scene, &d->owner, &d->input,
                     cfg->select_key, cfg->fade_ramp, cfg->quiet,
                     cfg->savedata, cfg->skip_intro);
    d->fade_ramp = cfg->fade_ramp;

    /* Bind the render sink to the live surface + callbacks, then install it as
     * the scene's draw target.  ramp_a/ramp_b/compose_group carry through; the
     * deferred LOGO/SPARKLE/MENU_CURSOR draws stay NULL (no-op) for this drive. */
    d->sink = (title_sink_ctx){
        .primary       = cfg->primary,
        .ramp_a        = cfg->ramp_a,
        .ramp_b        = cfg->ramp_b,
        .compose_group = cfg->compose_group,
        .present       = cfg->present,
        .log_flip      = cfg->log_flip,
        .draw_logo     = NULL,
        .draw_sparkle  = NULL,
        .draw_cursor   = NULL,
        .user          = cfg->user,
    };
    title_sink_bind(&d->sink);
    title_render_sink_hook = title_render_sink;

    /* Stash the hooks for the per-frame step (copied pointer; the hooks struct
     * itself is the caller's, must outlive the drive — main.c uses a static). */
    d->scene_hooks_holder = cfg->hooks;

    d->started = 1;
    d->done    = 0;
    d->result  = 0;
    return 1;
}

title_scene_status title_drive_step(title_drive *d, uint32_t now)
{
    title_scene_status st;

    if (!d->started || d->done)
        return TITLE_SCENE_DONE;

    st = title_scene_step(&d->scene, now, d->scene_hooks_holder);
    if (st == TITLE_SCENE_DONE) {
        d->done   = 1;
        d->result = d->scene.result;
    }
    return st;
}

void title_drive_shutdown(title_drive *d)
{
    if (!d->started)
        return;

    /* Drop the sink first so no stray draw can touch a freed surface. */
    title_render_sink_hook = NULL;
    title_sink_bind(NULL);

    /* Free what the phase-8 menu spawn allocated, mirroring the test harness's
     * free_spawn: the controller buffer (children[0]) + the child-pointer
     * array, then the node (entry[0]) + the entry array.  menu_ctrl_clear
     * releases the controller's owned sub-buffers (list / rows / cells). */
    if (d->scene.menu.ctrl != NULL)
        menu_ctrl_clear(d->scene.menu.ctrl);
    if (d->scene.menu.node != NULL) {
        free(d->scene.menu.node->children[0]);  /* the controller buffer */
        free(d->scene.menu.node->children);     /* the child-pointer array */
    }
    free(d->node0);    /* entries[0] — the menu-tree node */
    free(d->entries);

    d->node0   = NULL;
    d->entries = NULL;
    d->started = 0;
}
