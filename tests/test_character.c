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
    character_init(&c, 19200, 52000, CHAR_FACE_RIGHT);
    T_ASSERT_EQ_I(c.world_x, 19200);
    T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);

    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, +1);                       /* hold RIGHT */
    for (int i = 0; i < N; i++) {
        character_step(&c, axis, 0, 0);
        T_ASSERT_EQ_I(c.world_x, EXPECT[i]);  /* field-exact vs retail */
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);  /* facing holds through the walk */
    }
    /* At the cap the per-tick step is a steady +240 (dwx = 24000/100). */
    T_ASSERT_EQ_I(c.vel, CHAR_WALK_CAP);
    int32_t before = c.world_x;
    int32_t dwx = character_step(&c, axis, 0, 0);
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
    character_init(&c, 40000, 52000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, +1);
    for (int i = 0; i < 60; i++) character_step(&c, axis, 0, 0);  /* reach the cap */
    T_ASSERT_EQ_I(c.vel, CHAR_WALK_CAP);

    axis_set(axis, 0);                        /* release */
    int32_t wx = c.world_x;
    for (int i = 0; i < N; i++) {
        int32_t dwx = character_step(&c, axis, 0, 0);
        T_ASSERT_EQ_I(dwx, BRAKE_DWX[i]);     /* field-exact decel */
        wx += BRAKE_DWX[i];
        T_ASSERT_EQ_I(c.world_x, wx);
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);  /* facing holds to the stop */
    }
    T_ASSERT_EQ_I(c.vel, 0);                   /* stopped */
    /* Held idle stays stopped. */
    T_ASSERT_EQ_I(character_step(&c, axis, 0, 0), 0);
    T_ASSERT_EQ_I(c.world_x, wx);
    return 0;
}

/* LEFT walk from a facing-right rest: the symmetric law (the integrator flips
 * facing 1->3 at v==0, then accelerates -16/tick to the -240 cap). */
int test_character_walk_left_symmetry(void)
{
    character c;
    character_init(&c, 50000, 52000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, -1);                        /* hold LEFT */

    /* warmup (CHAR_INPUT_REPEAT_DELAY ticks: cmd not latched -> no motion) */
    for (int i = 0; i < CHAR_INPUT_REPEAT_DELAY - 1; i++) {
        T_ASSERT_EQ_I(character_step(&c, axis, 0, 0), 0);
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);
    }
    /* latch tick: facing != want -> flip to LEFT at rest, still 0 motion */
    T_ASSERT_EQ_I(character_step(&c, axis, 0, 0), 0);
    T_ASSERT_EQ_I(c.facing, CHAR_FACE_LEFT);

    /* now accelerate left: dwx -16,-32,..,-240 (mirror of the right walk) */
    int32_t wx = c.world_x;
    for (int k = 1; k <= 15; k++) {
        int32_t dwx = character_step(&c, axis, 0, 0);
        T_ASSERT_EQ_I(dwx, -16 * k);
        wx += dwx;
        T_ASSERT_EQ_I(c.world_x, wx);
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_LEFT);
    }
    T_ASSERT_EQ_I(c.vel, -CHAR_WALK_CAP);
    /* capped at -240 */
    T_ASSERT_EQ_I(character_step(&c, axis, 0, 0), -240);
    return 0;
}

/* The RUN (dash) ramp (Phase-4 chip 3b): a direction double-tap held -> cmd[0]=6,
 * the higher cap + two-phase accel (0x442a70 case 0x75, the run branch).  Validated
 * FIELD-EXACT against retail's captured per-tick body (runs/runjump-gt/capdash2, ckpt
 * 118: a seed-pinned RING double-tap RIGHT from the town freeroam).  The capture is
 * RIGHT held from the press: 2 warmup ticks, 2 WALK ticks (cmd0=2: hvel 1600,3200),
 * then the double-tap latches cmd0=6 and the run accel kicks in (+3200/tick to 24000,
 * then +1600/tick to the 48000 cap).  The embedded (hvel, worldX) are retail's bytes. */
int test_character_run_ramp(void)
{
    /* per-tick from the press tick (capdash2 t1550..t1578); run latches at tick 4. */
    static const int32_t HVEL[] = {
            0,     0,  1600,  3200,  6400,  9600, 12800, 16000, 19200, 22400,
        25600, 27200, 28800, 30400, 32000, 33600, 35200, 36800, 38400, 40000,
        41600, 43200, 44800, 46400, 48000, 48000, 48000, 48000, 48000,
    };
    static const int32_t WX[] = {
        19200, 19200, 19216, 19248, 19312, 19408, 19536, 19696, 19888, 20112,
        20368, 20640, 20928, 21232, 21552, 21888, 22240, 22608, 22992, 23392,
        23808, 24240, 24688, 25152, 25632, 26112, 26592, 27072, 27552,
    };
    const int N = (int)(sizeof HVEL / sizeof HVEL[0]);
    const int RUN_FROM = 4;                   /* the double-tap latches cmd0=6 here */

    character c;
    character_init(&c, 19200, 52000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, +1);                       /* hold RIGHT the whole dash */

    for (int i = 0; i < N; i++) {
        character_step(&c, axis, /*jump_held=*/0, /*run=*/(i >= RUN_FROM) ? 1 : 0);
        T_ASSERT_EQ_I(c.vel, HVEL[i]);        /* field-exact vs retail (the two-phase ramp) */
        T_ASSERT_EQ_I(c.world_x, WX[i]);
        T_ASSERT_EQ_I(c.facing, CHAR_FACE_RIGHT);
    }
    /* At the run cap the per-tick step is a steady +480 (dwx = 48000/100 = 2x the walk). */
    T_ASSERT_EQ_I(c.vel, CHAR_RUN_CAP);
    int32_t before = c.world_x;
    T_ASSERT_EQ_I(character_step(&c, axis, 0, 1), 480);
    T_ASSERT_EQ_I(c.world_x, before + 480);

    /* Release the dash but KEEP holding the dir (run flag off, axis still RIGHT): the
     * over-cap path decelerates 48000 -> the walk cap 24000 at the BRAKE rate (-800/
     * tick, dwx -8), then holds the walk.  (0x445db0 over-cap, decompile 442a70:1091.) */
    int32_t v = c.vel;                        /* 48000 */
    for (int i = 0; i < 30 && v > CHAR_WALK_CAP; i++) {
        character_step(&c, axis, 0, /*run=*/0);
        v -= CHAR_WALK_BRAKE;                 /* -800/tick toward the walk cap */
        T_ASSERT_EQ_I(c.vel, v);
    }
    T_ASSERT_EQ_I(c.vel, CHAR_WALK_CAP);      /* settled at the walk cap */
    T_ASSERT_EQ_I(character_step(&c, axis, 0, 0), 240);  /* now walking (+240) */
    return 0;
}

/* Idle + both-directions-held are no-ops (no net direction, stays at rest). */
int test_character_idle_and_conflict(void)
{
    character c;
    character_init(&c, 12345, 52000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT] = {0, 0, 0, 0};
    for (int i = 0; i < 10; i++) T_ASSERT_EQ_I(character_step(&c, axis, 0, 0), 0);
    T_ASSERT_EQ_I(c.world_x, 12345);

    axis[CHAR_AXIS_LEFT] = axis[CHAR_AXIS_RIGHT] = 1;   /* both held -> idle */
    for (int i = 0; i < 10; i++) T_ASSERT_EQ_I(character_step(&c, axis, 0, 0), 0);
    T_ASSERT_EQ_I(c.world_x, 12345);

    /* NULL guards. */
    T_ASSERT_EQ_I(character_step(NULL, axis, 0, 0), 0);
    T_ASSERT_EQ_I(character_step(&c, NULL, 0, 0), 0);  /* no axis -> brake (already 0) */
    T_ASSERT_EQ_I(character_step(&c, axis, 0, 1), 0);  /* run flag but no dir -> idle */
    return 0;
}

/* The JUMP arc (Phase-4 chip 3b): the airborne integrator (0x442a70 case 3) is
 * FIELD-EXACT against RETAIL's captured per-tick body (runs/runjump-gt, ckpt 116:
 * a seed-pinned ring-injected jump from the town freeroam, byte-identical across
 * both captured jumps).  This is the SHORT HOP: the ring execute (cmd[2]=7) is a
 * one-tick event so cmd[2]==0 the whole rise -> the FREE rise grav (8000, the
 * in_ECX[0x5669] captured at ckpt 117).  The embedded (vvel, world_y) are retail's
 * bytes, so a pass proves the port reproduces the engine's jump arc tick-for-tick. */
int test_character_jump_arc(void)
{
    static const int32_t VVEL[] = {
        -76000, -68000, -60000, -52000, -44000, -36000, -28000, -20000, -12000, -4000,
          4000,   8000,  12000,  16000,  20000,  24000,  28000,  32000,  36000, 40000,
         44000,  48000,  52000,  56000,  60000,  64000,      0,
    };
    static const int32_t WY[] = {
        51200, 50440, 49760, 49160, 48640, 48200, 47840, 47560, 47360, 47240,
        47200, 47240, 47320, 47440, 47600, 47800, 48040, 48320, 48640, 49000,
        49400, 49840, 50320, 50840, 51400, 52000, 52000,
    };
    const int N = (int)(sizeof VVEL / sizeof VVEL[0]);

    character c;
    character_init(&c, 19200, 52000, CHAR_FACE_RIGHT);  /* grounded at the town wy */
    T_ASSERT_EQ_I(c.airborne, 0);
    T_ASSERT_EQ_I(c.world_y, 52000);

    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, 0);                                  /* no walk -> pure vertical */

    /* The 4-tick launch WINDUP (case 3 sub 0): airborne immediately on the trigger but
     * stationary until the impulse fires.  The capture is a ring TAP (cmd[2]=7 a one-tick
     * event), so the button is pressed only on the edge -> cmd[2]==0 the whole rise ->
     * the FREE rise grav (8000). */
    for (int w = 0; w < CHAR_JUMP_WINDUP_THRESH; w++) {
        character_step(&c, axis, /*jump_held=*/(w == 0) ? 1 : 0, /*run=*/0);
        T_ASSERT_EQ_I(c.airborne, 1);                   /* airborne at once */
        T_ASSERT_EQ_I(c.vvel, 0);                       /* but stationary (windup) */
        T_ASSERT_EQ_I(c.world_y, 52000);
    }

    /* The launch (the 5th tick) + the arc, button released the whole rise. */
    for (int i = 0; i < N; i++) {
        character_step(&c, axis, /*jump_held=*/0, /*run=*/0);
        T_ASSERT_EQ_I(c.vvel, VVEL[i]);                 /* field-exact vs retail */
        T_ASSERT_EQ_I(c.world_y, WY[i]);
    }
    /* Landed: back on the ground, vvel zeroed, airborne cleared. */
    T_ASSERT_EQ_I(c.airborne, 0);
    T_ASSERT_EQ_I(c.world_y, 52000);
    T_ASSERT_EQ_I(c.vvel, 0);
    return 0;
}

/* The jump WINDUP (Phase-4 chip 3b, ckpt 119): the launch-anticipation delay between
 * the jump trigger and the impulse (0x442a70 case 3 sub-state 0, counter > 4).  GROUND-
 * TRUTHED bit-exact from the bstate (body+0x38 = main | sub<<16) field of the ring-
 * injected capture (runs/runjump-gt/capjump-ring2): on the trigger the body enters the
 * airborne state immediately but stays STATIONARY (vvel 0, wy 52000) for exactly 4 sim-
 * ticks (capture flips 4602-4609 = main 3 / sub 0), then the impulse fires on the 5th
 * tick (flip 4610 = main 3 / sub 1 / vvel -76000). */
int test_character_jump_windup(void)
{
    character c;
    character_init(&c, 19200, 52000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, 0);

    /* The trigger tick: the rising edge enters the airborne state THIS tick (sub 0,
     * counter incremented to 1) but the impulse has NOT fired — the body is stationary. */
    int32_t dwx = character_step(&c, axis, /*jump_held=*/1, /*run=*/0);
    T_ASSERT_EQ_I(dwx, 0);
    T_ASSERT_EQ_I(c.airborne, 1);          /* airborne immediately */
    T_ASSERT_EQ_I(c.jump_sub, 0);          /* sub-state 0 = windup */
    T_ASSERT_EQ_I(c.jump_ctr, 1);          /* counter incremented on the entry tick */
    T_ASSERT_EQ_I(c.vvel, 0);              /* no impulse yet */
    T_ASSERT_EQ_I(c.world_y, 52000);       /* stationary */

    /* 3 more stationary windup ticks (counter 2,3,4 — button released, the windup is
     * button-independent: it always completes). */
    for (int w = 2; w <= CHAR_JUMP_WINDUP_THRESH; w++) {
        character_step(&c, axis, /*jump_held=*/0, /*run=*/0);
        T_ASSERT_EQ_I(c.airborne, 1);
        T_ASSERT_EQ_I(c.jump_sub, 0);
        T_ASSERT_EQ_I(c.jump_ctr, w);
        T_ASSERT_EQ_I(c.vvel, 0);
        T_ASSERT_EQ_I(c.world_y, 52000);
    }

    /* The 5th tick: the counter exceeds 4 -> the impulse fires, sub-state advances to 1,
     * worldY takes its first step (-800), vvel reads -76000 (impulse + one fall grav).
     * Matches the capture's flip 4610 exactly. */
    character_step(&c, axis, /*jump_held=*/0, /*run=*/0);
    T_ASSERT_EQ_I(c.jump_sub, 1);          /* launched */
    T_ASSERT_EQ_I(c.jump_ctr, 0);          /* counter reset on launch */
    T_ASSERT_EQ_I(c.vvel, -76000);         /* the launch impulse */
    T_ASSERT_EQ_I(c.world_y, 51200);
    return 0;
}

/* Grounded idle stays put; the jump triggers on the rising edge only (no double-
 * jump while held); and HOLDING the button is the variable-height HIGH jump (the
 * 2000 rise grav -> a strictly higher apex than the short hop's 4800). */
int test_character_jump_edge_and_ground(void)
{
    character c;
    character_init(&c, 30000, 52000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, 0);

    /* Grounded, no jump button: no vertical motion at all. */
    for (int i = 0; i < 5; i++) {
        character_step(&c, axis, 0, 0);
        T_ASSERT_EQ_I(c.airborne, 0);
        T_ASSERT_EQ_I(c.world_y, 52000);
        T_ASSERT_EQ_I(c.vvel, 0);
    }

    /* HOLD the jump the whole time (the floaty high jump, rise grav 2000): launch
     * once on the edge, and — held, never re-pressed — NO double-jump.  Track the
     * apex; loop until it lands (generous cap for the high arc). */
    int launches = 0, lands = 0, was_air = 0;
    int32_t apex = 52000;
    for (int t = 0; t < 400; t++) {
        int before_air = c.airborne;
        character_step(&c, axis, 1, 0);
        if (!before_air && c.airborne) launches++;
        if (was_air && !c.airborne) lands++;
        if (c.world_y < apex) apex = c.world_y;     /* highest point (smaller wy) */
        was_air = c.airborne;
        if (lands == 1) break;
    }
    T_ASSERT_EQ_I(launches, 1);                      /* exactly one jump */
    T_ASSERT_EQ_I(lands, 1);                         /* one landing */
    T_ASSERT_EQ_I(c.airborne, 0);
    T_ASSERT_EQ_I(c.world_y, 52000);
    T_ASSERT(52000 - apex > 4800);                   /* higher than the short hop */

    /* Releasing and re-pressing arms a second jump (a fresh rising edge -> a fresh
     * windup -> the same launch impulse). */
    character_step(&c, axis, 0, 0);                     /* release */
    T_ASSERT_EQ_I(c.airborne, 0);
    character_step(&c, axis, 1, 0);                     /* re-press -> new edge, windup begins */
    T_ASSERT_EQ_I(c.airborne, 1);
    T_ASSERT_EQ_I(c.vvel, 0);                           /* windup: no impulse yet */
    for (int w = 1; w < CHAR_JUMP_WINDUP_THRESH; w++) { /* the rest of the 4-tick windup */
        character_step(&c, axis, 1, 0);
        T_ASSERT_EQ_I(c.vvel, 0);
    }
    character_step(&c, axis, 1, 0);                     /* the 5th tick -> launch */
    T_ASSERT_EQ_I(c.vvel, -76000);                   /* same launch impulse */
    return 0;
}

/* The VARIABLE-HEIGHT held jump RISE, validated FIELD-EXACT against a held-C capture
 * (runs/runjump-gt/capheld, ckpt 117: C held via the leaf -> cmd[2]=8 the whole rise
 * -> the floaty 2000 rise grav).  The embedded (vvel, world_y) are retail's captured
 * bytes for the 16 rise ticks BEFORE the town CEILING clamps the apex (~tick 16, wy
 * 41600 — a collision-mover concern, not jump physics; the flat-ground port has no
 * ceiling so it keeps rising at 2000).  Proves the held branch reproduces retail. */
int test_character_jump_held_rise(void)
{
    static const int32_t VVEL[] = {
        -76000, -74000, -72000, -70000, -68000, -66000, -64000, -62000,
        -60000, -58000, -56000, -54000, -52000, -50000, -48000, -46000,
    };
    static const int32_t WY[] = {
        51200, 50440, 49700, 48980, 48280, 47600, 46940, 46300,
        45680, 45080, 44500, 43940, 43400, 42880, 42380, 41900,
    };
    const int N = (int)(sizeof VVEL / sizeof VVEL[0]);

    character c;
    character_init(&c, 19200, 52000, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT];
    axis_set(axis, 0);

    /* The 4-tick WINDUP, button HELD (the windup is button-independent — stationary). */
    for (int w = 0; w < CHAR_JUMP_WINDUP_THRESH; w++) {
        character_step(&c, axis, /*jump_held=*/1, /*run=*/0);
        T_ASSERT_EQ_I(c.vvel, 0);
        T_ASSERT_EQ_I(c.world_y, 52000);
    }

    /* HOLD the jump every tick -> cmd[2]=8 -> the 2000 rise grav (the high jump). */
    for (int i = 0; i < N; i++) {
        character_step(&c, axis, /*jump_held=*/1, /*run=*/0);
        T_ASSERT_EQ_I(c.vvel, VVEL[i]);              /* field-exact vs the held capture */
        T_ASSERT_EQ_I(c.world_y, WY[i]);
    }
    /* Still rising (the held jump is much taller than the short hop) — the port has
     * no town ceiling, so it keeps going up at 2000 (retail's apex here is a ceiling). */
    T_ASSERT(c.vvel < 0);
    T_ASSERT_EQ_I(c.airborne, 1);
    T_ASSERT(52000 - c.world_y > 4800);              /* already past the short-hop apex */
    return 0;
}

/* ════════════════════════════════════════════════════════════════════
 * character_resolve_run — the dash trigger (0x478ba0 double-tap detection,
 * ckpt 150).  Builds an input_mgr ring, resolves the run flag, and (the
 * integration tests) drives it straight into character_step to prove a live
 * double-tap reaches the run cap via the bit-exact physics.
 * ════════════════════════════════════════════════════════════════════ */
#include "input.h"

/* A ring with all 64 slots pointing at one "empty" record; tests overwrite the
 * slots they care about (mirrors tests/test_input.c mgr_init). */
static void dash_mgr_init(input_mgr *m, input_event *empty)
{
    memset(m, 0, sizeof *m);
    empty->id = 0; empty->ts = 0; empty->flag = 0;
    for (int i = 0; i < INPUT_RING_LEN; i++) m->ring[i] = empty;
}

/* A double-tap held -> run flag set + cmd_lr latched; self-sustains while held;
 * release drops it; a fresh single press after release is a walk, not a dash. */
int test_character_resolve_run_sustain_release(void)
{
    input_mgr m; input_event empty; dash_mgr_init(&m, &empty);
    uint32_t now = 100000;
    input_event a = { .id = INPUT_RING_DIR_LEFT, .ts = now, .flag = 1 };
    input_event b = { .id = INPUT_RING_DIR_LEFT, .ts = now, .flag = 1 };
    m.ring[5] = &a; m.ring[40] = &b;          /* two LEFT presses = a double-tap */

    character c; character_init(&c, 0, 0, CHAR_FACE_LEFT);
    int axis[CHAR_AXIS_COUNT]; axis_set(axis, -1);            /* hold LEFT */

    /* tick 1: the double-tap is in-window -> dash-left engages. */
    T_ASSERT_EQ_I(character_resolve_run(&c, &m, now, axis, CHAR_DASH_WINDOW_MS), 1);
    T_ASSERT_EQ_I(c.cmd_lr, 5);

    /* age the ring past the window: dtap now misses, but prev cmd_lr==5 sustains
     * the dash while LEFT stays held (retail's local_608[0]==5 self-sustain). */
    now += 2000;
    T_ASSERT_EQ_I(character_resolve_run(&c, &m, now, axis, CHAR_DASH_WINDOW_MS), 1);
    T_ASSERT_EQ_I(c.cmd_lr, 5);

    /* release LEFT -> the dash ends (the reset-each-tick slot stays 0). */
    axis_set(axis, 0);
    T_ASSERT_EQ_I(character_resolve_run(&c, &m, now, axis, CHAR_DASH_WINDOW_MS), 0);
    T_ASSERT_EQ_I(c.cmd_lr, 0);

    /* re-hold LEFT with no fresh double-tap (ring stale) -> walk, NOT a dash. */
    axis_set(axis, -1);
    T_ASSERT_EQ_I(character_resolve_run(&c, &m, now, axis, CHAR_DASH_WINDOW_MS), 0);
    T_ASSERT_EQ_I(c.cmd_lr, 0);
    return 0;
}

/* RIGHT runs second and wins on both-held: a sustained LEFT dash is overridden
 * the tick RIGHT is also held (retail writes 0x14854 in the RIGHT block too). */
int test_character_resolve_run_both_held_right_wins(void)
{
    input_mgr m; input_event empty; dash_mgr_init(&m, &empty);
    uint32_t now = 5000;
    input_event a = { .id = INPUT_RING_DIR_LEFT, .ts = now, .flag = 1 };
    input_event b = { .id = INPUT_RING_DIR_LEFT, .ts = now, .flag = 1 };
    m.ring[1] = &a; m.ring[2] = &b;

    character c; character_init(&c, 0, 0, CHAR_FACE_LEFT);
    int axis[CHAR_AXIS_COUNT]; axis_set(axis, -1);
    T_ASSERT_EQ_I(character_resolve_run(&c, &m, now, axis, CHAR_DASH_WINDOW_MS), 1);
    T_ASSERT_EQ_I(c.cmd_lr, 5);                       /* dashing LEFT */

    /* now hold BOTH: RIGHT block runs second, prev==6 is false + no RIGHT dtap
     * -> cmd_lr becomes 0, run drops (matches retail's run-ends-on-both). */
    axis[CHAR_AXIS_RIGHT] = 1;                        /* LEFT still held too */
    T_ASSERT_EQ_I(character_resolve_run(&c, &m, now, axis, CHAR_DASH_WINDOW_MS), 0);
    T_ASSERT_EQ_I(c.cmd_lr, 0);
    return 0;
}

/* END-TO-END: a live double-tap-RIGHT-then-hold drives the run flag into the
 * bit-exact physics and reaches the RUN cap 48000 — the whole input->dash chain.
 * The contrast (a single held press) caps at the WALK cap 24000. */
int test_character_dash_via_double_tap(void)
{
    int axis[CHAR_AXIS_COUNT]; axis_set(axis, +1);   /* hold RIGHT */
    uint32_t now = 200000;

    /* (a) double-tap: two RIGHT presses in the ring -> dash -> RUN cap. */
    {
        input_mgr m; input_event empty; dash_mgr_init(&m, &empty);
        input_event p1 = { .id = INPUT_RING_DIR_RIGHT, .ts = now, .flag = 1 };
        input_event p2 = { .id = INPUT_RING_DIR_RIGHT, .ts = now, .flag = 1 };
        m.ring[10] = &p1; m.ring[20] = &p2;

        character c; character_init(&c, 0, 0, CHAR_FACE_RIGHT);
        int reached_run = 0;
        for (int i = 0; i < 80; i++) {
            int run = character_resolve_run(&c, &m, now, axis, CHAR_DASH_WINDOW_MS);
            character_step(&c, axis, 0, run);
            if (run) reached_run = 1;
        }
        T_ASSERT_EQ_I(reached_run, 1);
        T_ASSERT_EQ_I(c.cmd_lr, 6);                  /* dash-right sustained */
        T_ASSERT_EQ_I(c.vel, CHAR_RUN_CAP);          /* 48000 — only when running */
    }

    /* (b) single held press (no double-tap) -> walk -> WALK cap, never RUN. */
    {
        input_mgr m; input_event empty; dash_mgr_init(&m, &empty);
        input_event p1 = { .id = INPUT_RING_DIR_RIGHT, .ts = now, .flag = 1 };
        m.ring[10] = &p1;                            /* one press = held, not a tap-tap */

        character c; character_init(&c, 0, 0, CHAR_FACE_RIGHT);
        for (int i = 0; i < 80; i++) {
            int run = character_resolve_run(&c, &m, now, axis, CHAR_DASH_WINDOW_MS);
            T_ASSERT_EQ_I(run, 0);
            character_step(&c, axis, 0, run);
        }
        T_ASSERT_EQ_I(c.cmd_lr, 0);
        T_ASSERT_EQ_I(c.vel, CHAR_WALK_CAP);         /* 24000 — walking */
    }
    return 0;
}

/* NULL-safety: no manager / no axis -> no run, no crash. */
int test_character_resolve_run_guards(void)
{
    character c; character_init(&c, 0, 0, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT]; axis_set(axis, +1);
    T_ASSERT_EQ_I(character_resolve_run(NULL, NULL, 0, axis, CHAR_DASH_WINDOW_MS), 0);
    T_ASSERT_EQ_I(character_resolve_run(&c, NULL, 0, axis, CHAR_DASH_WINDOW_MS), 0);
    T_ASSERT_EQ_I(character_resolve_run(&c, NULL, 0, NULL, CHAR_DASH_WINDOW_MS), 0);
    return 0;
}

/* ════════════════════════════════════════════════════════════════════
 * character_resolve_pose — the U/D-pose command (0x478ba0:248-259, ckpt 153).
 * The sibling of resolve_run: a held direction + a recent ring press (or the
 * prev-tick self-sustain) latches cmd[3] = 10 (DOWN) / 0xb (UP).
 * ════════════════════════════════════════════════════════════════════ */

/* DOWN/UP each engage on a held direction + a ring press aged into the window;
 * they self-sustain while held; a fresh hold with a stale ring does NOT pose
 * (the input buffer — only the ring/sustain trigger it); release clears it. */
int test_character_resolve_pose_down_up(void)
{
    input_mgr m; input_event empty; dash_mgr_init(&m, &empty);
    uint32_t now = 100000;
    input_event d = { .id = INPUT_RING_DIR_DOWN, .ts = now - 100, .flag = 1 };
    input_event u = { .id = INPUT_RING_DIR_UP,   .ts = now - 100, .flag = 1 };
    m.ring[7] = &d; m.ring[33] = &u;

    character c; character_init(&c, 0, 0, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT] = {0, 0, 0, 0};

    /* DOWN held + a DOWN ring press aged 100 ms (in [10,800]) -> crouch (10). */
    axis[CHAR_AXIS_DOWN] = 1;
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now, axis), CHAR_POSE_DOWN);
    T_ASSERT_EQ_I(c.cmd_pose, CHAR_POSE_DOWN);

    /* age the ring out of the window (>800): prev==10 sustains while DOWN held. */
    now += 5000;
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now, axis), CHAR_POSE_DOWN);

    /* release DOWN -> the pose clears (reset-each-tick slot stays 0). */
    axis[CHAR_AXIS_DOWN] = 0;
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now, axis), 0);
    T_ASSERT_EQ_I(c.cmd_pose, 0);

    /* re-hold DOWN with the ring stale + no sustain -> the input buffer holds:
     * no pose (needs a fresh ring press or a sustain). */
    axis[CHAR_AXIS_DOWN] = 1;
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now, axis), 0);

    /* UP held + a UP ring press aged 100 ms -> defensive pose (0xb). */
    axis[CHAR_AXIS_DOWN] = 0; axis[CHAR_AXIS_UP] = 1;
    u.ts = now - 100;
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now, axis), CHAR_POSE_UP);
    T_ASSERT_EQ_I(c.cmd_pose, CHAR_POSE_UP);
    return 0;
}

/* The window floor (10 ms): a just-now press (age < 10) does NOT pose; once it
 * ages past the floor it does.  The ceiling (800) drops it again (sans sustain). */
int test_character_resolve_pose_window(void)
{
    input_mgr m; input_event empty; dash_mgr_init(&m, &empty);
    uint32_t now = 50000;
    input_event d = { .id = INPUT_RING_DIR_DOWN, .ts = now, .flag = 1 };
    m.ring[12] = &d;

    character c; character_init(&c, 0, 0, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT] = {0, 0, 0, 0}; axis[CHAR_AXIS_DOWN] = 1;

    /* age 0 (< the 10 ms floor) -> the one-frame buffer suppresses the pose. */
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now, axis), 0);
    /* age exactly 10 -> inclusive lower bound, poses. */
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now + 10, axis), CHAR_POSE_DOWN);
    c.cmd_pose = 0;  /* drop the sustain to isolate the ceiling test */
    /* age exactly 800 -> inclusive upper bound, poses. */
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now + 800, axis), CHAR_POSE_DOWN);
    c.cmd_pose = 0;
    /* age 801 (> ceiling) + no sustain -> misses. */
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now + 801, axis), 0);
    return 0;
}

/* UP runs second and writes the same slot, so it overrides a held DOWN when both
 * are held (retail's :254-259 after :248-253), and NULL inputs are safe. */
int test_character_resolve_pose_up_overrides_down(void)
{
    input_mgr m; input_event empty; dash_mgr_init(&m, &empty);
    uint32_t now = 7000;
    input_event d = { .id = INPUT_RING_DIR_DOWN, .ts = now - 50, .flag = 1 };
    input_event u = { .id = INPUT_RING_DIR_UP,   .ts = now - 50, .flag = 1 };
    m.ring[3] = &d; m.ring[4] = &u;

    character c; character_init(&c, 0, 0, CHAR_FACE_RIGHT);
    int axis[CHAR_AXIS_COUNT] = {0, 0, 0, 0};
    axis[CHAR_AXIS_DOWN] = 1; axis[CHAR_AXIS_UP] = 1;     /* both held */
    T_ASSERT_EQ_I(character_resolve_pose(&c, &m, now, axis), CHAR_POSE_UP);

    /* guards (a FRESH character so there is no prev-pose sustain to mask them):
     * NULL char / NULL mgr / NULL axis all resolve to 0 without a crash.  With a
     * NULL ring the sustain is the only surviving arm, so a fresh c (cmd_pose 0)
     * stays 0. */
    character g; character_init(&g, 0, 0, CHAR_FACE_RIGHT);
    T_ASSERT_EQ_I(character_resolve_pose(NULL, &m, now, axis), 0);
    T_ASSERT_EQ_I(character_resolve_pose(&g, NULL, now, axis), 0);
    T_ASSERT_EQ_I(character_resolve_pose(&g, &m, now, NULL), 0);
    return 0;
}
