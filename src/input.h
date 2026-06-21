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

/* ─── the slice of the input-manager object the title path touches ───
 *
 * The poll (FUN_0043c110) only reads the 64-entry pointer ring at
 * manager+0xc.  The title-menu input dispatch additionally reads the
 * axis-held flags at +0x114/+0x118, and the skip-splash early-out
 * (FUN_0056aea0 @ 0x56b25e) flushes a wider swathe — the two parallel
 * 11-dword arrays at +0x114 / +0x140, the +0x10c/+0x110 dwords, the
 * +0x16c half-word, and every ring slot's id.  Mirror all of it at the
 * exact offsets so a test can lay out the bytes the way retail sees them
 * and so the provenance stays legible; the manager's real header
 * (0x00..0x0b) and everything past +0x16d are opaque here.
 *
 *   ring[i]      manager + 0xc + i*4 — pointer to the i-th event record.
 *                ring[63] sits at manager+0x108, where retail's poll/scan
 *                pointer starts (both walk it newest-first, high→low).
 *   axis_held[]  manager + 0x114 (11 dwords) — array A; per-DIRECTION held
 *                flags written each frame by the input producer 0x46a880
 *                from the DInput keyboard buffer (leaf query 0x5ba520 =
 *                keyboard_state[scancode] & 0x80):
 *                  [0] = UP   (DIK 0xc8)   [2] = LEFT  (0xcb)
 *                  [1] = DOWN (0xd0)       [3] = RIGHT (0xcd)
 *                ([4..] are configurable action buttons.)  The title menu reads
 *                slots 0/1 for vertical auto-repeat; the freeroam character
 *                mover reads them for held walking (engine-quirk #41/#101).
 *   axis_held_b[] manager + 0x140 (11 dwords) — array B, parallel to A
 *                (semantics not yet recovered; the skip-splash flush zeros
 *                it alongside A).
 */
typedef struct input_mgr {
    uint8_t      _head[0x0c];                 /* 0x00..0x0b — opaque    */
    input_event *ring[INPUT_RING_LEN];        /* 0x0c..0x108 — 64 ptrs  */
    int32_t      field_10c;                   /* 0x10c — flushed by skip-splash */
    int32_t      field_110;                   /* 0x110 — flushed by skip-splash */
    int32_t      axis_held[11];               /* 0x114..0x13f — array A; [0]=UP [1]=DOWN [2]=LEFT [3]=RIGHT */
    int32_t      axis_held_b[11];             /* 0x140..0x16b — array B (parallel)  */
    uint16_t     field_16c;                   /* 0x16c — flushed by skip-splash */
} input_mgr;

/* axis_held[0] (UP) / axis_held[1] (DOWN) are read by the title-menu input
 * dispatch (0x56aea0 default branch, `[in_ECX[1]+0x114]` / `+0x118`) to
 * synthesise an auto-repeat / release event when no discrete nav button was
 * pressed this frame: axis_held[0] gates the 6 (held) / 7 (released) menu
 * action, and axis_held[1] gates the 4 (held) / 5 (released) one — vertical
 * list auto-repeat off the two vertical directions.  They are the first two of
 * the four direction slots of array A; the producer 0x46a880 fills all
 * four (UP/DOWN/LEFT/RIGHT) plus action buttons [4..] each frame from the
 * DInput keyboard buffer (engine-quirk #41/#101 — no longer a black box), and
 * the skip-splash flush zeros all of A + parallel array B at +0x140.  Only the
 * poll's ring fields are touched by FUN_0043c110. */

/* Pin the retail offsets on the real 32-bit build (where pointers are
 * 4 bytes); the 64-bit host build skips these, exactly as zdd.h does —
 * the poll only indexes m->ring[i], so its behaviour is offset-agnostic
 * and the host model just needs a 64-pointer array. */
#if UINTPTR_MAX == 0xFFFFFFFFu
#include <stddef.h>
_Static_assert(offsetof(input_mgr, ring)                   == 0x0c,  "input ring base offset");
_Static_assert(offsetof(input_mgr, ring[INPUT_RING_LEN-1]) == 0x108, "input ring top offset");
_Static_assert(offsetof(input_mgr, field_10c)              == 0x10c, "input field_10c offset");
_Static_assert(offsetof(input_mgr, field_110)              == 0x110, "input field_110 offset");
_Static_assert(offsetof(input_mgr, axis_held)             == 0x114, "input axis-held A offset");
_Static_assert(offsetof(input_mgr, axis_held[1])         == 0x118, "input axis-held H offset");
_Static_assert(offsetof(input_mgr, axis_held_b)          == 0x140, "input axis-held B offset");
_Static_assert(offsetof(input_mgr, field_16c)             == 0x16c, "input field_16c offset");
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

/* Is there ANY recently-pressed button still in the ring?
 *
 * The skip-splash early-out's scan (0x56b119..0x56b144): walk the ring
 * newest-first (index 63 → 0) and return 1 as soon as a slot is found whose
 * .id != 0, .flag == 1, and (now - .ts) <= 100 (unsigned, so a GetTickCount
 * wrap is handled exactly as retail's `jbe`).  Returns 0 if no slot
 * qualifies.  Unlike input_poll_consume this matches *any* id (it answers
 * "did the player press anything to skip the intro?") and does NOT consume —
 * the caller flushes the whole ring via input_mgr_reset on a hit, which
 * subsumes retail's redundant single-slot zero at 0x56b18f.  Pure read. */
int input_any_fresh_press(const input_mgr *m, uint32_t now);

/* Flush the input manager's ring + axis state.
 *
 * The skip-splash field reset (0x56b25e..0x56b29a): zero every ring slot's
 * id (all 64), both 11-dword axis arrays (A at +0x114, B at +0x140), the
 * +0x10c / +0x110 dwords, and the +0x16c half-word.  Mirrors retail
 * byte-for-byte (every ring slot is assumed to point at a valid record, as
 * retail never null-checks).  Used when the intro is skipped to drop any
 * input accumulated during the splash so the menu starts from a clean slate.
 * (Retail also zeros a scene-local sparkle counter here — that lives outside
 * the input manager and is the title scene's concern, not this flush's.) */
void input_mgr_reset(input_mgr *m);

/* ─── direction ring ids ─────────────────────────────────────────────
 * The discrete press/release events the producer 0x46a880 posts for the
 * four directions (the dash double-tap 0x479e70, the U/D-pose ring find
 * 0x479960, and the menu nav 0x56aea0 all scan these).  Pinned four ways
 * (all agree): the producer's fixed arrow bindings (0xc8/0xd0/0xcb/0xcd →
 * 1/3/2/4, input.md), the title-menu nav (id 1 = up, id 3 = down), and the
 * char-AI 0x478ba0 — the L/R dash reads id 2 for LEFT (held +0x11c) / id 4
 * for RIGHT (+0x120), and the U/D pose (0x478ba0:248-259) reads id 3 for
 * DOWN (held +0x118) / id 1 for UP (held +0x114).  Matches input_live.c
 * KEYMAP (UP→1, DOWN→3).  [Was DOWN=1/UP=3 here — a stale copy-error that
 * never bit because only the L/R dash consumed these; fixed ckpt 153.] */
#define INPUT_RING_DIR_UP    1
#define INPUT_RING_DIR_LEFT  2
#define INPUT_RING_DIR_DOWN  3
#define INPUT_RING_DIR_RIGHT 4

/* Was `dir_id` (a direction ring id) DOUBLE-TAPPED within the last `window`
 * ms?  The dash trigger: a faithful reduction of FUN_00479e70 as the char-AI
 * 0x478ba0 invokes it for the L/R dash (param_2=0, param_3=param_4=window,
 * param_5=param_6=dir, param_7=0 — read-only, no consume).
 *
 * Scans the ring (FUN_00479960) for TWO DISTINCT records with .id == dir_id,
 * .flag == 1 (pressed), and (now - .ts) <= window (unsigned, GetTickCount-wrap
 * safe).  A "used" bitmap forces the two to be different ring slots, so a
 * single held press (one record) does NOT read as a double-tap.  Returns 1
 * iff two such presses exist, else 0.  Does not modify the ring.  `window` is
 * the config double-tap window *(*0x8a6e80+0xf8) (CHAR_DASH_WINDOW_MS). */
int input_dash_double_tap(const input_mgr *m, uint32_t now,
                          int32_t dir_id, uint32_t window);

/* Is there a recent pressed ring event for `dir_id` aged in [lo, hi] ms?
 *
 * FUN_00479960 reduced to the U/D-pose invocation the char-AI 0x478ba0 makes
 * (param_8 == NULL → no "used" bitmap, param_9 == 0 → no consume): scan the
 * ring index 0 UPWARD and return the index of the FIRST record with
 * .id == dir_id, .flag == 1, and (now - .ts) in [lo, hi] (unsigned, so a
 * GetTickCount wrap / future timestamp underflows past `hi` and is rejected),
 * else -1.  Unlike the dash double-tap this matches ONE event (a held press
 * suffices once it has aged past `lo`), and does not modify the ring.  The
 * pose uses lo = 10, hi = 800 (0x478ba0:248/254) — the 10 ms floor is the
 * one-frame input buffer that keeps a just-now press from posing instantly. */
int input_ring_find_recent(const input_mgr *m, uint32_t now,
                           int32_t dir_id, uint32_t lo, uint32_t hi);

#endif /* OPENSUMMONERS_INPUT_H */
