/*
 * src/input_trace.h — port-side deterministic input replay.
 *
 * Mirrors the Frida harness's input-injection format (tools/frida_capture.py):
 * a sparse JSONL trace, one `{"frame": N, "ids": [i, j, …]}` line per change,
 * where each entry injects those button ids as fresh presses at the first
 * poll at-or-after Flip frame N.  The harness replays into *retail*'s input
 * ring; this module replays the same trace into the *port*'s input ring (the
 * drive's input_mgr), so a scripted scene walk (e.g. the new-game path in
 * docs/findings/new-game-flow.md) drives the port deterministically — the
 * port-side counterpart needed to capture port frames for the pixel-parity
 * diff against the harness goldens.
 *
 * TICK AXIS (ckpt 134): an entry may instead key on the deterministic SIM-TICK
 * (`{"tick": N, "ids": [...]}`) — the easer 0x43d1d0 call count, the axis the
 * trace-studio tick-join pairs on.  Both the port and retail share it, so a
 * tick-keyed entry fires confirm at the SAME sim-tick on both sides — the
 * MATCHED-CADENCE nav that makes per-tick dialogue compares honest (the port's
 * Flip cadence differs from retail's, so a Flip-keyed confirm cannot align the
 * dialogue; plans/intro-cutscene-1to1.md).  The axis is PER ENTRY, so a single
 * nav MIXES them: a flip-keyed boot prefix (the menus, while sim_tick is still
 * 0) then tick-keyed in-game confirms.  The threshold value lives in
 * `entry.frame` regardless of axis; a single object carrying BOTH keys is
 * ambiguous and fails the parse.  Each axis's thresholds must be non-decreasing
 * among themselves (interleaving the two axes is allowed).
 *
 * The button ids are the engine's poll ids (engine-quirks #42/#43): up = 1,
 * down = 3, confirm = 0x24, abort = 0x22, etc. — exactly the values the trace
 * files already carry.
 *
 * Pure C, Win32-free: the parse + the ring injection are host-testable; the
 * frame source (the present/Flip count) is the caller's job (main.c feeds the
 * drive's present counter).  An "id" is injected by writing the next ring slot
 * round-robin as {id, flag=1, ts=now} — byte-for-byte what the poll
 * (input_poll_consume / input_any_fresh_press) treats as a fresh press.
 */
#ifndef OPENSUMMONERS_INPUT_TRACE_H
#define OPENSUMMONERS_INPUT_TRACE_H

#include <stddef.h>
#include <stdint.h>

#include "input.h"   /* input_mgr — the replay target */

/* Max button ids carried by a single frame entry.  The harness injects a
 * handful per frame; 8 is well past any real trace line. */
#define INPUT_TRACE_MAX_IDS 8

/* Sanity ceiling on entries — a corrupt/runaway file fails loudly instead of
 * OOM-ing.  Far past any real scripted scene walk. */
#define INPUT_TRACE_MAX_ENTRIES (1u << 20)

/* Which counter an entry's threshold (entry.frame) is compared against —
 * PER ENTRY, so a trace may MIX axes (the canonical pattern: a flip-keyed boot
 * prefix that sequences the title/newgame menus while sim_tick is still 0, then
 * tick-keyed in-game confirms — the matched-cadence dialogue nav). */
#define INPUT_TRACE_AXIS_FRAME 0   /* the present/Flip count (default)     */
#define INPUT_TRACE_AXIS_TICK  1   /* the deterministic sim-tick (0x43d1d0) */

struct input_trace_entry {
    uint32_t frame;                        /* threshold: the Flip frame OR the
                                            * sim-tick (per .axis) the ids fire
                                            * at-or-after                     */
    int32_t  ids[INPUT_TRACE_MAX_IDS];     /* button ids to inject        */
    uint16_t n_ids;                        /* count of live ids           */
    uint8_t  axis;                         /* INPUT_TRACE_AXIS_* for THIS entry */
};

struct input_trace {
    struct input_trace_entry *entries;     /* heap; NULL when count == 0  */
    size_t                    count;        /* live entries                */
    size_t                    cap;          /* allocated capacity          */
    size_t                    cursor;       /* next un-injected entry      */
    int                       ring_head;    /* round-robin ring write slot */
};

/* Release the heap table and reset to empty.  Idempotent — safe on a
 * zero-initialised trace and safe to call twice.  MUST be called on any trace
 * passed to the parsers even when they return 0 (a partial table may have
 * been allocated before the malformed line). */
void input_trace_free(struct input_trace *t);

/* Parse a sparse JSONL trace from an in-memory buffer.  Returns 1 on success
 * (fills *out) or 0 if any line fails to parse (out->count is the entries
 * parsed before the failure).  Tolerated: blank lines, leading/trailing
 * whitespace, `# …` comment lines.  Numbers may be decimal or 0x-hex.  Each
 * entry records its axis (the `frame` vs `tick` key); the two axes may be mixed
 * (a flip-keyed prefix then a tick-keyed suffix), but the thresholds within
 * EACH axis must be non-decreasing (out-of-order within an axis fails), and an
 * object carrying both keys fails.  `len` is the byte length; an embedded NUL
 * is tolerated. */
int input_trace_parse_buf(const char *buf, size_t len, struct input_trace *out);

/* Same, reading from `path`.  Returns 0 if the file can't be opened. */
int input_trace_load(const char *path, struct input_trace *out);

/* Inject every not-yet-fired entry whose threshold <= its axis counter into
 * `mgr`'s ring as fresh presses stamped `now`, advancing the internal cursor.
 * The counter is `present_frame` for a FRAME-axis entry or `sim_tick` for a
 * TICK-axis entry.  The cursor is monotonic, so order the file boot-flips then
 * in-game-ticks: a FRAME entry firing during boot (sim_tick still 0) before the
 * TICK entries are reached.  Each id takes the next ring slot round-robin
 * (mgr->ring[head] must point at a real record — the drive's input_mgr
 * satisfies this).  No-op when the trace is empty, fully consumed, or `mgr` is
 * NULL.  Call once per frame, before the scene step's poll. */
void input_trace_replay(struct input_trace *t, uint32_t present_frame,
                        uint32_t sim_tick, input_mgr *mgr, uint32_t now);

#endif /* OPENSUMMONERS_INPUT_TRACE_H */
