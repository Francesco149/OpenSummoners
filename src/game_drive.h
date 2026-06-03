/*
 * src/game_drive.h — the in-game map drive (the caller side of the per-map run
 * loop 0x59f2c0, reached via the wrapper 0x59ec30(0,0,0x3f2)).
 *
 * This is the milestone-2 counterpart of title_drive / newgame_drive /
 * prologue_drive: the Win32-free plumbing that owns the in-game scene state +
 * its input ring, steps it once per presented frame, and renders + presents
 * through cfg callbacks the real build (main.c) fills.
 *
 * SCAFFOLD (ckpt 52).  The in-game ENGINE is not yet ported — 0x59f2c0
 * (world alloc + map load), its per-step driver 0x586010 (6 KB sim/draw),
 * and the render dispatch 0x5a00c0 (13.7 KB) are the multi-checkpoint body
 * still ahead (see docs/findings/in-game-intro.md).  So this drive has no scene
 * MODEL behind it yet: a step renders the faithful map-load frame — the screen
 * is BLACK from game_enter (retail flip ~1092) until the town first renders
 * (~flip 1150) while the engine loads + fades.  Presenting black here is the
 * correct retail entry state (golden runs/tas-ingame-1: flips 900-1100 black),
 * and it replaces the earlier enter_game stub that wrongly re-displayed the
 * title.  When 0x5a00c0 is ported, the render callback grows from a clear-
 * to-black into the real town tilemap/entity/UI walk, and game_drive_step
 * starts returning GAME_EXIT on the engine's scene-transition codes (the
 * 0x59f2c0 return 4/5 → 0x59ec30 map reload 0x2724/0x272e).
 *
 * The structural loop (init / step / shutdown, the input ring wiring) is pure C
 * and host-tests, exactly like the sibling drives.
 *
 * Lifecycle:
 *     game_drive_init(&d, &cfg);
 *     while (running) {
 *         uint32_t now = GetTickCount();
 *         switch (game_drive_step(&d, now)) {
 *         case GAME_EXIT: ...map transition / leave...; running = 0; break;
 *         default: break;                              // GAME_RUNNING: keep going
 *         }
 *     }
 *     game_drive_shutdown(&d);
 */
#ifndef OPENSUMMONERS_GAME_DRIVE_H
#define OPENSUMMONERS_GAME_DRIVE_H

#include <stdint.h>

#include "input.h"           /* input_mgr / input_event / INPUT_RING_LEN */

/* The step outcome.  Mirrors the prologue/newgame status enums; for now a step
 * only ever stays RUNNING (no engine to signal a transition yet).  GAME_EXIT is
 * reserved for the ported engine's scene-transition / leave path. */
typedef enum game_status {
    GAME_RUNNING = 0,   /* keep looping (rendered + presented this frame) */
    GAME_EXIT,          /* the map run loop ended (transition / teardown) */
} game_status;

/* What the drive needs to wire the in-game frame to the live engine.  Both
 * callbacks are optional (NULL ⇒ no-op), so a headless drive degrades to a
 * faithful all-no-op run that still steps + polls input. */
typedef struct game_drive_cfg {
    /* Draw one frame.  Today: clear the primary to black (the map-load frame).
     * When 0x5a00c0 is ported this becomes the town render walk.  Receives
     * `user`.  NULL ⇒ no render. */
    void (*render)(void *user);

    /* Present the composed frame (retail FUN_005b8fc0 = zdd_present).  NULL ⇒
     * no present.  The real build's thunk also bumps the Flip/present counter
     * the input-trace replay keys on. */
    void (*present)(void *user);

    void *user;        /* opaque, passed to render / present */
} game_drive_cfg;

/* The running drive: the in-game input manager + the bound config.  Treat as
 * opaque; read `started` / `ticks` after a step.  A scene/world model field
 * will join `input` here when the engine is ported. */
typedef struct game_drive {
    input_mgr   input;
    /* Backing store for the input ring (the poll path derefs ring[i] with no
     * NULL guard, so every slot points at an owned idle record — an all-idle
     * ring is the faithful "no input" state). */
    input_event ring_store[INPUT_RING_LEN];

    /* Borrowed cfg callbacks. */
    void (*render)(void *user);
    void (*present)(void *user);
    void *user;

    int      started;   /* init ran                 */
    uint32_t ticks;     /* frames stepped           */
} game_drive;

/* Bind the input ring + store the cfg callbacks.  cfg is consumed by value.
 * Returns 1. */
int game_drive_init(game_drive *d, const game_drive_cfg *cfg);

/* Run one frame: render + present the current in-game frame.  Returns:
 *   GAME_RUNNING → keep looping;
 *   GAME_EXIT    → the map run loop ended (not produced until the engine port).
 */
game_status game_drive_step(game_drive *d, uint32_t now);

/* No owned heap; resets `started`.  Safe on a never-init'd drive. */
void game_drive_shutdown(game_drive *d);

#endif /* OPENSUMMONERS_GAME_DRIVE_H */
