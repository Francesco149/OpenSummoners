/*
 * test_character.c — the controllable-character WALK reduction (Phase-4 chip 3),
 * validated FIELD-EXACT against Arche's ground-truth per-tick body
 * (runs/mover-caller, ckpt 114: seed-pinned + lockstep held-axis walk; the real
 * body 0xe637b80 committed by 0x485fc0+0x96e -> 0x442a70 case 0x75).
 *
 * The embedded worldX sequences are RETAIL's captured values (not law-derived),
 * so a pass proves the port reproduces the engine's bytes tick-for-tick.
 */
#include "t.h"

#include "character.h"

/* Build an axis-held array: all clear except the named direction (or none). */
static void axis_set(int a[CHAR_AXIS_COUNT], int dir /* -1 L, +1 R, 0 none */)
{
    a[CHAR_AXIS_UP] = a[CHAR_AXIS_DOWN] = a[CHAR_AXIS_LEFT] = a[CHAR_AXIS_RIGHT] = 0;
    if (dir < 0)      a[CHAR_AXIS_LEFT]  = 1;
    else if (dir > 0) a[CHAR_AXIS_RIGHT] = 1;
}

/* RIGHT-walk accelerate -> cap: the captured arche_body.wx from the press tick
 * (runs/mover-caller, ticks 1549..1581, spawn worldX 19200, facing 1).  The first
 * two values are the press->latch warmup (motion starts on the 3rd held tick). */
int test_character_walk_accel(void)
{
    static const int32_t EXPECT[] = {
        19200, 19200, 19216, 19248, 19296, 19360, 19440, 19536, 19648, 19776,
        19920, 20080, 20256, 20448, 20656, 20880, 21120, 21360, 21600, 21840,
        22080, 22320, 22560, 22800, 23040, 23280, 23520, 23760, 24000, 24240,
        24480, 24720, 24960,
    };
    const int N = (int)(sizeof EXPECT / sizeof EXPECT[0]);

    character c;
    character_init(&c, 19200, CHAR_FACE_RIGHT);
    T_ASSERT_EQ_I(c.world_x, 19200);
    T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);

    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, +1);                       /* hold RIGHT */
    for (int i = 0; i < N; i++) {
        character_step(&c, axis);
        T_ASSERT_EQ_I(c.world_x, EXPECT[i]);  /* field-exact vs retail */
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);  /* facing holds through the walk */
    }
    /* At the cap the per-tick step is a steady +240 (dwx = 24000/100). */
    T_ASSERT_EQ_I(c.vel, CHAR_WALK_CAP);
    int32_t before = c.world_x;
    int32_t dwx = character_step(&c, axis);
    T_ASSERT_EQ_I(dwx, 240);
    T_ASSERT_EQ_I(c.world_x, before + 240);
    return 0;
}

/* Release at the cap -> brake to a stop: the captured deceleration dwx sequence
 * (240 cap, then -8/tick: 232,224,..,8,0), facing held the whole glide. */
int test_character_walk_brake(void)
{
    static const int32_t BRAKE_DWX[] = {
        232, 224, 216, 208, 200, 192, 184, 176, 168, 160,
        152, 144, 136, 128, 120, 112, 104,  96,  88,  80,
         72,  64,  56,  48,  40,  32,  24,  16,   8,   0,
    };
    const int N = (int)(sizeof BRAKE_DWX / sizeof BRAKE_DWX[0]);

    character c;
    character_init(&c, 40000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, +1);
    for (int i = 0; i < 60; i++) character_step(&c, axis);  /* reach the cap */
    T_ASSERT_EQ_I(c.vel, CHAR_WALK_CAP);

    axis_set(axis, 0);                        /* release */
    int32_t wx = c.world_x;
    for (int i = 0; i < N; i++) {
        int32_t dwx = character_step(&c, axis);
        T_ASSERT_EQ_I(dwx, BRAKE_DWX[i]);     /* field-exact decel */
        wx += BRAKE_DWX[i];
        T_ASSERT_EQ_I(c.world_x, wx);
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);  /* facing holds to the stop */
    }
    T_ASSERT_EQ_I(c.vel, 0);                   /* stopped */
    /* Held idle stays stopped. */
    T_ASSERT_EQ_I(character_step(&c, axis), 0);
    T_ASSERT_EQ_I(c.world_x, wx);
    return 0;
}

/* LEFT walk from a facing-right rest: the symmetric law (the integrator flips
 * facing 1->3 at v==0, then accelerates -16/tick to the -240 cap). */
int test_character_walk_left_symmetry(void)
{
    character c;
    character_init(&c, 50000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, -1);                        /* hold LEFT */

    /* warmup (CHAR_INPUT_REPEAT_DELAY ticks: cmd not latched -> no motion) */
    for (int i = 0; i < CHAR_INPUT_REPEAT_DELAY - 1; i++) {
        T_ASSERT_EQ_I(character_step(&c, axis), 0);
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);
    }
    /* latch tick: facing != want -> flip to LEFT at rest, still 0 motion */
    T_ASSERT_EQ_I(character_step(&c, axis), 0);
    T_ASSERT_EQ_I(c.facing, CHAR_FACE_LEFT);

    /* now accelerate left: dwx -16,-32,..,-240 (mirror of the right walk) */
    int32_t wx = c.world_x;
    for (int k = 1; k <= 15; k++) {
        int32_t dwx = character_step(&c, axis);
        T_ASSERT_EQ_I(dwx, -16 * k);
        wx += dwx;
        T_ASSERT_EQ_I(c.world_x, wx);
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_LEFT);
    }
    T_ASSERT_EQ_I(c.vel, -CHAR_WALK_CAP);
    /* capped at -240 */
    T_ASSERT_EQ_I(character_step(&c, axis), -240);
    return 0;
}

/* Idle + both-directions-held are no-ops (no net direction, stays at rest). */
int test_character_idle_and_conflict(void)
{
    character c;
    character_init(&c, 12345, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT] = {0, 0, 0, 0};
    for (int i = 0; i < 10; i++) T_ASSERT_EQ_I(character_step(&c, axis), 0);
    T_ASSERT_EQ_I(c.world_x, 12345);

    axis[CHAR_AXIS_LEFT] = axis[CHAR_AXIS_RIGHT] = 1;   /* both held -> idle */
    for (int i = 0; i < 10; i++) T_ASSERT_EQ_I(character_step(&c, axis), 0);
    T_ASSERT_EQ_I(c.world_x, 12345);

    /* NULL guards. */
    T_ASSERT_EQ_I(character_step(NULL, axis), 0);
    T_ASSERT_EQ_I(character_step(&c, NULL), 0);  /* no axis -> brake (already 0) */
    return 0;
}
