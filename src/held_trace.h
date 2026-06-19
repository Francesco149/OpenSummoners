/*
 * src/held_trace.h — port-side deterministic HELD-AXIS replay (the LEVEL
 * counterpart of input_trace's edge/ring replay).
 *
 * The engine's per-frame input update 0x46a880 fills the input-manager's
 * axis-held array (input_mgr.axis_held @ +0x114) from the DInput keyboard
 * buffer: for each held key it writes the matching slot = 1 (the leaf query is
 * 0x5ba520, which returns keyboard_state[scancode] & 0x80).  The producer
 * maps the four movement keys to the first four slots (0x46a880:1497-1512):
 *
 *     axis_held[0] (+0x114) = UP    (DIK scancode 0xc8)
 *     axis_held[1] (+0x118) = DOWN  (0xd0)
 *     axis_held[2] (+0x11c) = LEFT  (0xcb)
 *     axis_held[3] (+0x120) = RIGHT (0xcd)
 *
 * These are LEVEL signals — held for as long as the key is down — unlike the
 * ring's one-shot edge presses (input_trace).  The freeroam character mover
 * reads them (engine-quirk #41/#101); the title menu reads slots 0/1 for
 * vertical auto-repeat (title_scene.c).  This is why menu nav (discrete ring)
 * replays via --input-trace but held WALKING needs this separate level path.
 *
 * Replay model: a sparse {"frame":N,"keys":[..]} JSONL — each entry SETS the
 * held set to exactly those keys starting at Flip frame N, persisting until the
 * next entry (an empty keys[] releases all).  held_trace_replay rebuilds the
 * four managed direction slots of mgr->axis_held EVERY frame from the current
 * held set (clear-then-set, mirroring the producer).  Call it once per frame,
 * before the scene step reads the axis array.
 *
 * The harness side that injects the same trace into RETAIL is
 * tools/frida_capture.py --held-trace: it forces the leaf query 0x5ba520 to
 * report the held scancodes as pressed, so the real producer fills +0x114
 * exactly as a physical keypress would (robust to the hidden window's loss of
 * DInput focus).  Both sides consume the identical file.
 *
 * Keys are DIK scancodes (the engine-native unit); the parser also accepts the
 * names "up"/"down"/"left"/"right".  Pure C, Win32-free, host-testable.
 */
#ifndef OPENSUMMONERS_HELD_TRACE_H
#define OPENSUMMONERS_HELD_TRACE_H

#include <stddef.h>
#include <stdint.h>

#include "input.h"   /* input_mgr — the replay target (axis_held @ +0x114) */

/* DInput keyboard scancodes for the movement + action keys the producer maps to
 * axis-held slots (0x46a880).  Slots 0..3 = UP/DOWN/LEFT/RIGHT, 4 = jump (C),
 * 5 = attack (X) — matching input_live.c's KEYMAP.  Exposed so traces/tests can
 * name them (the freeroam jump reads axis_held[4]). */
#define HELD_DIK_UP    0xc8
#define HELD_DIK_DOWN  0xd0
#define HELD_DIK_LEFT  0xcb
#define HELD_DIK_RIGHT 0xcd
#define HELD_DIK_C     0x2e   /* jump  -> axis-held slot 4 (+0x124) */
#define HELD_DIK_X     0x2d   /* attack-> axis-held slot 5 (+0x128) */

/* Max held keys carried by a single frame entry.  A handful at once (e.g. a
 * diagonal = two directions) is the realistic ceiling; 8 is well past it. */
#define HELD_TRACE_MAX_KEYS 8

/* Sanity ceiling on entries — a corrupt/runaway file fails loudly instead of
 * OOM-ing.  Far past any real scripted walk. */
#define HELD_TRACE_MAX_ENTRIES (1u << 20)

struct held_trace_entry {
    uint32_t frame;                        /* Flip frame the held set takes effect */
    int32_t  keys[HELD_TRACE_MAX_KEYS];    /* DIK scancodes held from this frame   */
    uint16_t n_keys;                       /* count of live keys (0 = release all) */
};

struct held_trace {
    struct held_trace_entry *entries;      /* heap; NULL when count == 0  */
    size_t                    count;        /* live entries                */
    size_t                    cap;          /* allocated capacity          */
    size_t                    cursor;       /* next un-applied entry       */
    int32_t  cur_keys[HELD_TRACE_MAX_KEYS]; /* current held set (the level) */
    uint16_t cur_n;                         /* count in cur_keys           */
};

/* Map a DIK scancode to its axis-held slot (UP=0, DOWN=1, LEFT=2, RIGHT=3), or
 * -1 if the scancode is not one of the four movement directions the port
 * models.  (Retail's producer also fills action-button slots 4..; the port
 * mover doesn't read those yet, so the replay ignores them.) */
int held_scancode_slot(int32_t scancode);

/* Release the heap table and reset to empty.  Idempotent — safe on a
 * zero-initialised trace and safe to call twice.  MUST be called on any trace
 * passed to the parsers even when they return 0. */
void held_trace_free(struct held_trace *t);

/* Parse a sparse JSONL trace from an in-memory buffer.  Returns 1 on success
 * (fills *out) or 0 if any line fails to parse (out->count is the entries
 * parsed before the failure).  Tolerated: blank lines, leading/trailing
 * whitespace, `# …` comment lines.  Keys may be decimal/0x-hex scancodes or the
 * quoted names "up"/"down"/"left"/"right".  Frames must be non-decreasing
 * (out-of-order fails).  `len` is the byte length; an embedded NUL is tolerated. */
int held_trace_parse_buf(const char *buf, size_t len, struct held_trace *out);

/* Same, reading from `path`.  Returns 0 if the file can't be opened. */
int held_trace_load(const char *path, struct held_trace *out);

/* Advance the held set to the latest entry whose frame <= `present_frame`, then
 * rebuild mgr->axis_held[0..3] from it (each managed direction slot = 1 if its
 * scancode is in the current held set, else 0).  Call once per frame BEFORE the
 * scene step's axis read.  No-op when the trace is empty or `mgr` is NULL; an
 * all-empty trace still clears the four managed slots (deterministic baseline). */
void held_trace_replay(struct held_trace *t, uint32_t present_frame,
                       input_mgr *mgr);

#endif /* OPENSUMMONERS_HELD_TRACE_H */
