/*
 * src/input_live.{c,h} — the LIVE keyboard input producer (FUN_0046a880).
 *
 * The faithful port of the per-frame input producer 0x46a880 (RE'd ckpt 113,
 * engine-quirk #41/#101): each frame it reads the DInput keyboard buffer (via
 * the leaf 0x5ba520 = keyboard_state[scancode] & 0x80) and
 *   (1) fills the per-DIRECTION held array A (input_mgr.axis_held[0..6]) from
 *       the arrows (hardcoded scancodes, 0x46a880:1497-1514) + the action
 *       buttons (config scancodes, :1515-1538);
 *   (2) posts a discrete event into the 64-slot ring on each key EDGE
 *       (0x46a880's :1380-1496 push loop — the SAME ring input_trace_replay
 *       injects into, so the menu/dialogue/dash consumers fire from real keys).
 * Retail's producer never clears array A inline; the per-frame flush 0x56a220
 * does.  The port folds both into one clear-then-set pass here (the held_trace
 * replay does the same — src/held_trace.c).
 *
 * This is the INTERACTIVE path: it lets the player drive the port with a real
 * keyboard.  It is DELIBERATELY mutually exclusive with the deterministic
 * --held-trace/--input-trace REPLAY (which stays the capture/parity path —
 * live keyboard is wall-clock, hence non-deterministic).  main.c gates it:
 * replay active OR app not focused -> live producer skipped (see feed_input).
 *
 * Pure (no Win32): the caller hands in the 256-byte DIK-indexed keyboard
 * snapshot (bit 0x80 = held), exactly the buffer the leaf 0x5ba520 reads;
 * main.c fills it from GetAsyncKeyState on the real build.  Host-tested
 * (tests/test_input_live.c).
 *
 * PORT-DEBT(keybind-config): the scancodes/ring-ids below are the shipped
 * DEFAULTS; the real source is the keybind config *0x8a6e80 (arrows hardcoded;
 * jump @+0x574, attack @+0x558, run-mode @+0x510).  Re-bindable keys need that
 * struct ported.
 */
#ifndef OPENSUMMONERS_INPUT_LIVE_H
#define OPENSUMMONERS_INPUT_LIVE_H

#include <stdint.h>
#include "input.h"

/* DIK scancodes the leaf 0x5ba520 indexes (the DInput keyboard buffer). */
#define DIK_UP_ARROW     0xc8
#define DIK_DOWN_ARROW   0xd0
#define DIK_LEFT_ARROW   0xcb
#define DIK_RIGHT_ARROW  0xcd
#define DIK_Z            0x2c   /* advance / confirm / sword sheathe          */
#define DIK_X            0x2d   /* attack / interact   (config +0x558 default) */
#define DIK_C            0x2e   /* jump                (config +0x574 default) */

/* The live producer's per-frame state: the previous DIK snapshot (for edge
 * detection — the ring push is edge-driven) + the ring write cursor (parallels
 * input_trace's ring_head; the consumer scans all 64 slots so write order is
 * what matters, not absolute slot). */
typedef struct input_live {
    uint8_t  prev[256];   /* last frame's DIK-down snapshot (bit 0x80)        */
    uint16_t ring_head;   /* next ring slot to overwrite                      */
    int      started;     /* prev[] valid — suppress spurious frame-0 edges   */
} input_live;

/* Zero the producer state (call on (re-)entry, like input_trace_free's reset). */
void input_live_reset(input_live *st);

/* One producer frame (= 0x46a880):
 *   - rebuild axis_held[0..6] from `dik` (clear-then-set; the :1497-1538 fill
 *     folded with the :56a220 flush);
 *   - for each ring-mapped key whose held state CHANGED vs the previous frame,
 *     post {id, flag = pressed?1:0, ts = now} into the ring (the :1380-1496
 *     push; identical shape to input_trace_replay's injection).
 * `dik[sc] & 0x80` = scancode sc held.  No-op if any pointer is NULL. */
void input_live_step(input_live *st, input_mgr *m,
                     const uint8_t *dik, uint32_t now);

#endif /* OPENSUMMONERS_INPUT_LIVE_H */
