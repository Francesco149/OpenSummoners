/*
 * src/input.h — input-event ring poll, ported from FUN_0043c110.
 *
 * FUN_0043c110 is the engine's "was this button/key pressed in the last
 * ~100 ms, and if so consume it" query — the read side of the input
 * event ring buffer that lives inside the input-manager object.  It is a
 * __thiscall taking (now, button_id) with the manager in ECX; it scans a
 * fixed 64-entry ring of event-record pointers newest-first, and the
 * first record whose id matches, whose state flag is "pressed", and
 * whose timestamp is within 100 ms of `now` is consumed (its id is
 * zeroed so a later poll in the same frame won't re-match it) and the
 * call returns 1.  No match → 0.
 *
 * This is the primitive the title-menu update half polls every frame for
 * the menu navigation buttons (0x56aea0 default branch: it calls
 * FUN_0043c110(now, 2/4/1/3/0x24) and feeds the hits into the action
 * latch).  It is also the consumer of the `+0x108` ring that the
 * milestone-1 input work and the mem_watch harness are chasing — the
 * thing that fills the ring (the DInput GetDeviceState path) is still a
 * black box; this is the other end of it.
 *
 * Pure: pointer-and-integer arithmetic only, zero engine callees, no
 * Win32 surface.  The 970-byte cursor-navigation engine (0x43ca40) and
 * the action latch on top of it (0x43ce50) are deliberately NOT here —
 * they are object-model-coupled and partly unrecovered; see docs/findings.
 *
 * Ground truth: the disassembly / Ghidra output at 0x43c110 (84 bytes).
 * Every offset below is verified against it; see the per-line provenance
 * in input.c.
 */
#ifndef OPENSUMMONERS_INPUT_H
#define OPENSUMMONERS_INPUT_H

#include <stdint.h>

/* ─── one entry in the input event ring ──────────────────────────────
 *
 * The ring stores *pointers* to these records (the manager owns a fixed
 * pool of them elsewhere).  FUN_0043c110 reads exactly the first three
 * dwords; the engine's real record is wider (later fields are touched by
 * the producer side, not the poll), so only model what the poll reads.
 *
 *   id    record[0] — button / key id.  Compared against the polled id;
 *                      zeroed on a successful poll (consume-on-read).
 *   ts    record[1] — GetTickCount() when the event was recorded.
 *   flag  record[2] — event state; the poll only accepts flag == 1
 *                      ("pressed"/active).
 */
typedef struct input_event {
    int32_t  id;
    uint32_t ts;
    int32_t  flag;
} input_event;

/* Number of slots in the ring.  Retail scans a fixed 0x40 (64) entries
 * (loop counter starts at 0x3f and the slot array spans manager+0xc ..
 * manager+0x108 = 64 dwords). */
#define INPUT_RING_LEN 64

/* ─── the slice of the input-manager object the poll touches ─────────
 *
 * The poll only reads the 64-entry pointer ring at manager+0xc.  Mirror
 * it at the exact offset so a test can lay out the bytes the way retail
 * sees them and so the provenance stays legible; the manager's real
 * header (0x00..0x0b) and everything past the ring are opaque here.
 *
 *   ring[i]  manager + 0xc + i*4  — pointer to the i-th event record.
 *            ring[63] sits at manager+0x108, which is where retail's
 *            scan pointer starts.
 */
typedef struct input_mgr {
    uint8_t      _head[0x0c];                 /* 0x00..0x0b — opaque    */
    input_event *ring[INPUT_RING_LEN];        /* 0x0c..0x108 — 64 ptrs  */
} input_mgr;

/* Pin the retail offsets on the real 32-bit build (where pointers are
 * 4 bytes); the 64-bit host build skips these, exactly as zdd.h does —
 * the poll only indexes m->ring[i], so its behaviour is offset-agnostic
 * and the host model just needs a 64-pointer array. */
#if UINTPTR_MAX == 0xFFFFFFFFu
#include <stddef.h>
_Static_assert(offsetof(input_mgr, ring)                   == 0x0c,  "input ring base offset");
_Static_assert(offsetof(input_mgr, ring[INPUT_RING_LEN-1]) == 0x108, "input ring top offset");
_Static_assert(sizeof(input_event)                         == 0x0c,  "input_event is 3 dwords");
#endif

/* Poll the ring for a recently-pressed `button_id` and consume it.
 *
 * Scans m->ring newest-first (index 63 → 0, i.e. high address → low,
 * matching retail's downward walk from manager+0x108).  The first record
 * with .id == button_id, .flag == 1, and (now - .ts) <= 100 (computed in
 * unsigned 32-bit, so a GetTickCount wrap is handled exactly as retail's
 * `ja` does) is the hit: its .id is set to 0 (consumed) and the function
 * returns 1.  If no slot matches, returns 0 and nothing is modified.
 *
 * Faithful to 0x43c110.  Every ring slot is assumed to point at a valid
 * record, exactly as retail (which never null-checks the slot pointer);
 * the engine keeps all 64 slots populated.  Pure: touches only the
 * matched record's .id. */
int input_poll_consume(input_mgr *m, uint32_t now, int32_t button_id);

#endif /* OPENSUMMONERS_INPUT_H */
