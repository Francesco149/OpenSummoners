/*
 * src/input_live.c — the live keyboard producer (FUN_0046a880).  See input_live.h.
 */
#include "input_live.h"

#include <string.h>

/* The default keybind map (the *0x8a6e80 config's shipped town defaults —
 * PORT-DEBT(keybind-config)).  Each row binds a DIK scancode to:
 *   axis : the held-array A slot it sets while held (input.h order:
 *          0=UP 1=DOWN 2=LEFT 3=RIGHT 4=jump C 5=attack X), or -1 = none;
 *   ring : the discrete ring id it posts on a press/release EDGE, or -1 = none.
 * Directions fill an axis (held walking + the title-menu vertical auto-repeat)
 * AND post a ring id (the 0x479e70 dash double-tap + the 0x56aea0 menu nav,
 * which polls ids 2/4/1/3/0x24).  Jump (C = config +0x574) fills axis[4] (the
 * mover's jump_held) and posts ring id 7 (the 0x442a70 cmd[2]=7 execute).
 *
 * The CONFIRM (the dialogue/menu advance, ring id 0x24) is ENTER or X (USER ckpt
 * 132 — NOT Z), grounded in the 0x46a880 producer: ENTER (0x1c) is a FIXED
 * binding → 0x24 (46a880.c:590-602); X (config +0x558, the attack key) also
 * posts 0x24 (46a880.c:793-808) — so X is attack-held (axis 5) AND confirm.  Z
 * (a config button → ring 9, by elimination) has NO confirm/dialogue role; its
 * gameplay action (sheathe sword) is not in the port's reduced ring set, so it
 * has no row here. */
static const struct { uint8_t dik; int8_t axis; int16_t ring; } KEYMAP[] = {
    { DIK_UP_ARROW,    0, 1    },   /* +0x114, menu/up                       */
    { DIK_DOWN_ARROW,  1, 3    },   /* +0x118, menu/down                     */
    { DIK_LEFT_ARROW,  2, 2    },   /* +0x11c, walk/dash L                   */
    { DIK_RIGHT_ARROW, 3, 4    },   /* +0x120, walk/dash R                   */
    { DIK_C,           4, 7    },   /* +0x124, jump (config +0x574 → ring 7) */
    { DIK_X,           5, 0x24 },   /* +0x128, attack-held + CONFIRM         *
                                     * (config +0x558 → ring 0x24)           */
    { DIK_RETURN,     -1, 0x24 },   /* CONFIRM (the FIXED 0x1c → ring 0x24)  */
};
#define KEYMAP_LEN ((int)(sizeof(KEYMAP) / sizeof(KEYMAP[0])))

/* The axis-A slots the producer writes (+0x114..+0x12c = [0..6]); the flush
 * 0x56a220 zeros all of array A, but only [0..6] are ever set, so clearing the
 * written span is the faithful reduction. */
#define INPUT_LIVE_AXIS_SLOTS 7

void input_live_reset(input_live *st)
{
    if (st == NULL) return;
    memset(st, 0, sizeof *st);
}

/* Post one ring event (0x46a880:1424-1490 head-write, the same primitive
 * input_trace_replay uses): overwrite the cursor slot's record and advance.
 * Retail shifts all 64 entries down one and writes the new record at the head;
 * the consumer (input_poll_consume) scans every slot for the newest matching
 * id within the 100 ms window, so overwriting at a rotating cursor is
 * behaviourally identical (and is exactly what input_trace_replay does). */
static void ring_post(input_mgr *m, uint16_t *head,
                      int32_t id, int32_t flag, uint32_t now)
{
    input_event *rec = m->ring[*head];
    if (rec != NULL) {
        rec->id   = id;
        rec->flag = flag;
        rec->ts   = now;
    }
    *head = (uint16_t)((*head + 1) % INPUT_RING_LEN);
}

void input_live_step(input_live *st, input_mgr *m,
                     const uint8_t *dik, uint32_t now)
{
    if (st == NULL || m == NULL || dik == NULL) return;

    /* (1) axis-fill — clear-then-set [0..6] (the :1497-1538 fill folded with
     *     the :56a220 release flush). */
    for (int i = 0; i < INPUT_LIVE_AXIS_SLOTS; i++)
        m->axis_held[i] = 0;
    for (int k = 0; k < KEYMAP_LEN; k++)
        if (KEYMAP[k].axis >= 0 && (dik[KEYMAP[k].dik] & 0x80))
            m->axis_held[KEYMAP[k].axis] = 1;

    /* (2) ring-fill — post on each press/release EDGE (the :1380-1496 push).
     *     Skipped on the first frame (no valid prev[] yet) so a key already
     *     held at (re-)entry does not synthesise a phantom press. */
    if (st->started) {
        for (int k = 0; k < KEYMAP_LEN; k++) {
            if (KEYMAP[k].ring < 0) continue;
            int now_d = (dik[KEYMAP[k].dik] & 0x80) != 0;
            int was_d = (st->prev[KEYMAP[k].dik] & 0x80) != 0;
            if (now_d != was_d)
                ring_post(m, &st->ring_head, KEYMAP[k].ring, now_d ? 1 : 0, now);
        }
    }

    memcpy(st->prev, dik, sizeof st->prev);
    st->started = 1;
}
