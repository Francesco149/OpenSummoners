/*
 * test_entity_move.c — host tests for the entity movers (src/entity_move.c:
 * FUN_0054db10 / FUN_0054e5c0 / FUN_004416c0).  An empty 10x10-cell grid (no
 * tiles) isolates the entity scan; one blocker entity sits in g_world->collide.
 * Ground truth for these paths is rhost (rh-my-play: 22,987 horizontal entity
 * blocks + 4,029 vertical supports, 0 divergences); these pin the shape.
 */
#include "entity_move.h"
#include "map_grid.h"
#include "t.h"

#include <string.h>

typedef struct {
    wl_world   world;
    wl_list    list;
    wl_entity *items[2];
    wl_entity  self, blk;
    wl_body    self_body, blk_body;
} fixture;

/* self: part 0 at (4000, 8000) 1000x2000; blocker: (8000, 8000) 2000x2000, kind `kind`. */
static void setup(fixture *f, int32_t kind)
{
    memset(f, 0, sizeof *f);
    f->world.grid = map_grid_alloc();
    map_grid_set_dims(f->world.grid, 10, 10);
    f->world.collide = &f->list;
    f->list.items = f->items;
    f->items[0] = &f->self; f->items[1] = &f->blk; f->list.count = 2;
    f->self.body = &f->self_body; f->self.nbody = 1;
    f->self_body = (wl_body){ .active = 1, .x = 4000, .y = 8000, .w = 1000, .h = 2000 };
    f->blk.body = &f->blk_body; f->blk.nbody = 1;
    f->blk.coll_mode = 1; f->blk.coll_kind = kind;
    f->blk_body = (wl_body){ .active = 1, .x = 8000, .y = 8000, .w = 2000, .h = 2000 };
    g_world = &f->world; g_world_slope = NULL; g_world_slope_ctx = NULL;
}
static void teardown(fixture *f) { map_grid_free(f->world.grid); g_world = NULL; }

/* walking right into a solid entity: blocked before the tile sweep, nothing moves. */
int test_entity_move_h_blocked_by_solid(void)
{
    fixture f; setup(&f, 3);
    int r = entity_move_horizontal(&f.self, &f.self_body, 3500, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(r, 0);
    T_ASSERT_EQ_I(f.self_body.x, 4000);
    teardown(&f);
    return 0;
}

/* a one-way platform (kind 2) never blocks sideways: the sweep moves the body. */
int test_entity_move_h_platform_passes(void)
{
    fixture f; setup(&f, 2);
    int r = entity_move_horizontal(&f.self, &f.self_body, 3500, 0, 0, 0, 0, 0);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(f.self_body.x, 7500);
    teardown(&f);
    return 0;
}

/* probe mode: same verdict, the body is never written. */
int test_entity_move_h_probe_writes_nothing(void)
{
    fixture f; setup(&f, 2);
    int r = entity_move_horizontal(&f.self, &f.self_body, 1000, 0, 0, 0, 0, 1);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(f.self_body.x, 4000);
    teardown(&f);
    return 0;
}

/* falling onto a platform: the delta shortens to land 1 unit above its top, the
 * result is 0 and the platform is reported as the support. */
int test_entity_move_v_lands_on_platform(void)
{
    fixture f; setup(&f, 2);
    f.blk_body.x = 3000;                          /* under self, X overlap */
    f.blk_body.y = 12000;
    int32_t y = f.self_body.y;
    wl_entity *sup = NULL;
    int r = entity_move_vertical(&f.self, &f.self_body, 2500, 0, 0, &y, &sup);
    T_ASSERT_EQ_I(r, 0);
    T_ASSERT_EQ_I(y, 8000 + (12000 - (8000 + 2000 - 1) - 1));   /* reach = top - bottom - 1 = 2000 */
    T_ASSERT(sup == &f.blk);
    teardown(&f);
    return 0;
}

/* the drop-through flag skips one-way platforms: full fall, no support. */
int test_entity_move_v_pass_skips_platform(void)
{
    fixture f; setup(&f, 2);
    f.blk_body.x = 3000; f.blk_body.y = 12000;
    int32_t y = f.self_body.y;
    wl_entity *sup = &f.self;
    int r = entity_move_vertical(&f.self, &f.self_body, 2500, 1, 0, &y, &sup);
    T_ASSERT_EQ_I(r, 1);
    T_ASSERT_EQ_I(y, 10500);
    T_ASSERT(sup == NULL);
    teardown(&f);
    return 0;
}

/* jumping up into a solid block's underside: 0, nothing written. */
int test_entity_move_v_head_bonk(void)
{
    fixture f; setup(&f, 3);
    f.blk_body.x = 3000; f.blk_body.y = 5000;      /* bottom 6999, above self top 8000 */
    int32_t y = -1;
    wl_entity *sup = &f.self;
    f.self_body.y = 7000;                          /* top 7000 >= block top 5000 */
    int r = entity_move_vertical(&f.self, &f.self_body, -500, 0, 0, &y, &sup);
    T_ASSERT_EQ_I(r, 0);
    T_ASSERT_EQ_I(y, -1);
    T_ASSERT(sup == &f.self);
    teardown(&f);
    return 0;
}

int test_entity_move_faction_relation(void)
{
    T_ASSERT_EQ_I(faction_relation(0, 2), 0);
    T_ASSERT_EQ_I(faction_relation(3, 2), 1);
    T_ASSERT_EQ_I(faction_relation(2, 2), 2);
    T_ASSERT_EQ_I(faction_relation(1, 2), 1);
    return 0;
}
