/*
 * test_char_physics.c — host tests for the movement step (src/char_physics.c,
 * FUN_00467bb0).  Ground truth is rhost (shadow:char_physics: 1,582,325 calls /
 * 3 scenarios, 0 divergences); these pin the walk ramp and the jump windup on an
 * empty grid with Arche-like tuning.
 */
#include "char_physics.h"
#include "entity_move.h"
#include "map_grid.h"
#include "t.h"

#include <stdlib.h>
#include <string.h>

typedef struct { wl_world w; wl_list l; wl_entity *items[1]; wl_char *c; wl_body body; } phys_fx;

static void phys_setup(phys_fx *f)
{
    memset(f, 0, sizeof *f);
    f->w.grid = map_grid_alloc();
    map_grid_set_dims(f->w.grid, 40, 20);
    f->w.collide = &f->l; f->l.items = f->items; f->l.count = 0;
    f->c = calloc(1, sizeof *f->c);
    f->c->base.body = &f->body; f->c->base.nbody = 1;
    f->c->walk_max = 24000; f->c->walk_accel = 1600; f->c->brake = -800; f->c->max_speed = 48000;
    f->c->gravity = 4000; f->c->fall_cap = 76000; f->c->can_jump = 1; f->c->jump_impulse = -76000;
    f->c->rise_grav_held = 3000; f->c->rise_grav_free = 8000;
    f->body = (wl_body){ .active = 1, .x = 10000, .y = 10000, .w = 2000, .h = 5600, .facing = 1 };
    g_world = &f->w; g_world_slope = NULL;
}
static void phys_free(phys_fx *f) { map_grid_free(f->w.grid); free(f->c); g_world = NULL; }

int test_char_physics_walk_ramp(void)
{
    phys_fx f; phys_setup(&f);
    wl_cmd cmd; memset(&cmd, 0, sizeof cmd); cmd.lr = 2;          /* walk right, facing right */
    for (int i = 0; i < 20; i++) char_physics_step(f.c, &cmd, &f.body, 0, 0, 0, 1);
    T_ASSERT_EQ_I(f.body.vel_x, 24000);                           /* capped at walk_max */
    T_ASSERT_EQ_I(f.body.state, 1);                               /* moving */
    cmd.lr = 0;
    char_physics_step(f.c, &cmd, &f.body, 0, 0, 0, 1);
    T_ASSERT_EQ_I(f.body.vel_x, 23200);                           /* the -800 brake */
    phys_free(&f);
    return 0;
}

int test_char_physics_jump_windup(void)
{
    phys_fx f; phys_setup(&f);
    wl_cmd cmd; memset(&cmd, 0, sizeof cmd); cmd.jump = 7;
    char_physics_step(f.c, &cmd, &f.body, 0, 0, 0, 1);
    T_ASSERT_EQ_I(f.body.state, 3);                               /* jump, windup */
    int32_t y0 = f.body.y;
    for (int i = 0; i < 3; i++) char_physics_step(f.c, &cmd, &f.body, 0, 0, 0, 1);
    T_ASSERT_EQ_I(f.body.sub, 0);
    T_ASSERT_EQ_I(f.body.y, y0);                                  /* stationary through the windup */
    char_physics_step(f.c, &cmd, &f.body, 0, 0, 0, 1);            /* the 5th tick: impulse + move */
    T_ASSERT_EQ_I(f.body.sub, 1);
    T_ASSERT_EQ_I(f.body.y, y0 - 760);                            /* -76000 / 100 at once */
    phys_free(&f);
    return 0;
}
