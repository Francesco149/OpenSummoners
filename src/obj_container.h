/*
 * src/obj_container.h — two small generic engine container primitives,
 * ported from FUN_00412c10 and FUN_00414080.
 *
 * Both are __thiscall leaf helpers (zero engine callees) used all over
 * the engine — the title-menu update half (FUN_0056aea0 default branch)
 * is just one of ~10 call sites each.  They land here together because
 * they are the pure container leaves that the menu-spawn block depends
 * on; the menu assembly itself is deferred (it also needs 0x40f3e0 /
 * 0x40f5c0 / 0x411f40 / 0x4192b0, all still unported).  They operate on
 * two *distinct* container objects — keep the layouts separate.
 *
 *   FUN_00412c10  obj_pool_acquire     — check out the next free slot
 *                                        from a fixed-capacity object
 *                                        pool, stamping it with its
 *                                        owner + index.  Returns NULL
 *                                        when the pool is exhausted.
 *   FUN_00414080  sel_list_mark_last   — single-selection: mark the last
 *                                        entry of a list selected (flag
 *                                        1) and every earlier entry not
 *                                        (flag 0).
 *
 * In the menu-spawn block these run back to back: a freshly-appended
 * list entry is marked selected (sel_list_mark_last), then the menu
 * controller object is checked out of its pool (obj_pool_acquire).
 *
 * Pure: pointer/integer arithmetic only, no Win32 surface.  Ground truth
 * is the disassembly at 0x412c10 (46 B) and 0x414080 (63 B); every
 * offset is verified there and pinned by guarded _Static_assert below.
 */
#ifndef OPENSUMMONERS_OBJ_CONTAINER_H
#define OPENSUMMONERS_OBJ_CONTAINER_H

#include <stdint.h>
#include <stddef.h>

/* ─── object pool (FUN_00412c10) ─────────────────────────────────────
 *
 * A pool of pre-allocated slot objects handed out in order.  The pool
 * object holds a pointer array of all slots plus a capacity/count pair;
 * acquire returns slots[count] and bumps count, or NULL once count has
 * reached capacity.  Each handed-out slot is stamped with a back-pointer
 * to its pool and its own index, and its +8 dword is zeroed.
 */
typedef struct pool_slot {
    void    *owner;     /* +0x00 — set to the owning pool on acquire     */
    uint16_t index;     /* +0x04 — slot's index in the pool (low u16)    */
    uint16_t _hi6;      /* +0x06 — high half of dword[1]; NOT touched    */
    int32_t  field8;    /* +0x08 — zeroed on acquire                     */
} pool_slot;

typedef struct obj_pool {
    uint8_t     _head[0x48];   /* 0x00..0x47 — opaque                    */
    pool_slot **slots;         /* 0x48 — array of pre-allocated slot ptrs */
    uint16_t    capacity;      /* 0x4c — total slots                     */
    uint16_t    count;         /* 0x4e — slots currently in use          */
} obj_pool;

/* Check out the next free slot.  Returns NULL if count >= capacity;
 * otherwise stamps slots[count] with {owner = p, index = count, +8 = 0},
 * increments count, and returns it.  Faithful to 0x412c10. */
pool_slot *obj_pool_acquire(obj_pool *p);

/* ─── single-select list (FUN_00414080) ─────────────────────────────
 *
 * A list whose entries each carry a "selected" flag at +8.  Marking the
 * last entry selected (and clearing the rest) is the engine's idiom for
 * "the most recently appended item is the active one" — exactly what the
 * menu code does right after appending a menu row.
 */
typedef struct sel_entry {
    uint8_t _head[8];   /* +0x00..+0x07 — opaque                         */
    int32_t selected;   /* +0x08 — 1 on the last entry, 0 otherwise      */
} sel_entry;

typedef struct sel_list {
    sel_entry **entries;   /* +0x00 — array of entry pointers            */
    uint16_t    capacity;  /* +0x04 — total capacity (read by callers)   */
    uint16_t    count;     /* +0x06 — number of live entries             */
} sel_list;

/* Mark the last entry of `l` selected and every earlier entry not.
 * No-op on an empty list.  Faithful to 0x414080 (which re-reads count
 * each iteration — preserved, though count is invariant here). */
void sel_list_mark_last(sel_list *l);

/* Pin retail offsets on the 32-bit build (4-byte pointers); the 64-bit
 * host skips these since the structs index by field name, not address —
 * same convention as zdd.h / input.h. */
#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(offsetof(pool_slot, owner)  == 0x00, "pool_slot.owner");
_Static_assert(offsetof(pool_slot, index)  == 0x04, "pool_slot.index");
_Static_assert(offsetof(pool_slot, field8) == 0x08, "pool_slot.field8");
_Static_assert(offsetof(obj_pool, slots)    == 0x48, "obj_pool.slots");
_Static_assert(offsetof(obj_pool, capacity) == 0x4c, "obj_pool.capacity");
_Static_assert(offsetof(obj_pool, count)    == 0x4e, "obj_pool.count");
_Static_assert(offsetof(sel_entry, selected) == 0x08, "sel_entry.selected");
_Static_assert(offsetof(sel_list, entries)   == 0x00, "sel_list.entries");
_Static_assert(offsetof(sel_list, capacity)  == 0x04, "sel_list.capacity");
_Static_assert(offsetof(sel_list, count)     == 0x06, "sel_list.count");
#endif

#endif /* OPENSUMMONERS_OBJ_CONTAINER_H */
