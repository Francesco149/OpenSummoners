/*
 * tests/test_obj_container.c — host-side tests for src/obj_container.c,
 * the generic container leaves FUN_00412c10 (obj_pool_acquire) and
 * FUN_00414080 (sel_list_mark_last).
 *
 * Pure: each test builds the container in host memory, exercises the op,
 * and checks the slot stamping / selection flags against the values
 * hand-derived from the disassembly (see obj_container.c).
 */
#include "../src/obj_container.h"
#include "t.h"

#include <stdint.h>
#include <string.h>

/* ─── obj_pool_acquire ──────────────────────────────────────────────── */

/* Build a pool over `cap` caller-owned slots. */
static void pool_init(obj_pool *p, pool_slot **slot_ptrs, pool_slot *slots,
                      int cap)
{
    memset(p, 0, sizeof *p);
    for (int i = 0; i < cap; i++) {
        memset(&slots[i], 0xee, sizeof slots[i]);   /* poison to catch writes */
        slot_ptrs[i] = &slots[i];
    }
    p->slots    = slot_ptrs;
    p->capacity = (uint16_t)cap;
    p->count    = 0;
}

int test_pool_acquire_stamps_and_bumps(void)
{
    pool_slot *ptrs[4]; pool_slot slots[4]; obj_pool p;
    pool_init(&p, ptrs, slots, 4);

    pool_slot *s0 = obj_pool_acquire(&p);
    T_ASSERT_EQ_P(s0, &slots[0]);
    T_ASSERT_EQ_P(s0->owner, &p);       /* owner back-pointer */
    T_ASSERT_EQ_I(s0->index, 0);        /* index == prior count */
    T_ASSERT_EQ_I(s0->field8, 0);       /* +8 zeroed */
    T_ASSERT_EQ_I(p.count, 1);          /* count bumped */

    pool_slot *s1 = obj_pool_acquire(&p);
    T_ASSERT_EQ_P(s1, &slots[1]);
    T_ASSERT_EQ_I(s1->index, 1);
    T_ASSERT_EQ_I(p.count, 2);
    return 0;
}

/* The index write is a 16-bit store into dword[1]; the top half (+6)
 * must be left untouched. */
int test_pool_acquire_index_is_16bit_write(void)
{
    pool_slot *ptrs[1]; pool_slot slots[1]; obj_pool p;
    pool_init(&p, ptrs, slots, 1);
    slots[0]._hi6 = 0xABCD;             /* pre-existing high half */

    pool_slot *s = obj_pool_acquire(&p);
    T_ASSERT_EQ_I(s->index, 0);
    T_ASSERT_EQ_U(s->_hi6, 0xABCD);     /* untouched */
    return 0;
}

int test_pool_acquire_exhausts_returns_null(void)
{
    pool_slot *ptrs[2]; pool_slot slots[2]; obj_pool p;
    pool_init(&p, ptrs, slots, 2);

    T_ASSERT(obj_pool_acquire(&p) != 0);
    T_ASSERT(obj_pool_acquire(&p) != 0);
    T_ASSERT_EQ_I(p.count, 2);
    /* pool full: capacity <= count → NULL, count unchanged */
    T_ASSERT_EQ_P(obj_pool_acquire(&p), (void *)0);
    T_ASSERT_EQ_I(p.count, 2);
    return 0;
}

int test_pool_acquire_zero_capacity_is_null(void)
{
    obj_pool p; memset(&p, 0, sizeof p);
    p.capacity = 0; p.count = 0; p.slots = 0;
    T_ASSERT_EQ_P(obj_pool_acquire(&p), (void *)0);
    T_ASSERT_EQ_I(p.count, 0);
    return 0;
}

/* ─── sel_list_mark_last ────────────────────────────────────────────── */

static void sel_init(sel_list *l, sel_entry **ptrs, sel_entry *ents, int n)
{
    memset(l, 0, sizeof *l);
    for (int i = 0; i < n; i++) {
        memset(&ents[i], 0, sizeof ents[i]);
        ents[i].selected = 7;            /* non-0/1 sentinel: must be overwritten */
        ptrs[i] = &ents[i];
    }
    l->entries  = ptrs;
    l->capacity = (uint16_t)n;
    l->count    = (uint16_t)n;
}

int test_sel_mark_last_selects_only_last(void)
{
    sel_entry *ptrs[5]; sel_entry ents[5]; sel_list l;
    sel_init(&l, ptrs, ents, 5);

    sel_list_mark_last(&l);
    for (int i = 0; i < 5; i++)
        T_ASSERT_EQ_I(ents[i].selected, (i == 4) ? 1 : 0);
    return 0;
}

int test_sel_mark_last_single_entry(void)
{
    sel_entry *ptrs[1]; sel_entry ents[1]; sel_list l;
    sel_init(&l, ptrs, ents, 1);

    sel_list_mark_last(&l);
    T_ASSERT_EQ_I(ents[0].selected, 1);
    return 0;
}

int test_sel_mark_last_empty_is_noop(void)
{
    sel_entry *ptrs[1]; sel_entry ents[1]; sel_list l;
    sel_init(&l, ptrs, ents, 1);
    l.count = 0;                          /* empty list */
    ents[0].selected = 7;

    sel_list_mark_last(&l);
    T_ASSERT_EQ_I(ents[0].selected, 7);   /* untouched */
    return 0;
}

/* Idempotence: re-marking after the selection moved (count shrank to 3)
 * clears the old tail and selects the new last. */
int test_sel_mark_last_reselects_new_tail(void)
{
    sel_entry *ptrs[5]; sel_entry ents[5]; sel_list l;
    sel_init(&l, ptrs, ents, 5);
    sel_list_mark_last(&l);               /* entry 4 selected */

    l.count = 3;
    sel_list_mark_last(&l);
    T_ASSERT_EQ_I(ents[0].selected, 0);
    T_ASSERT_EQ_I(ents[1].selected, 0);
    T_ASSERT_EQ_I(ents[2].selected, 1);   /* new last */
    /* entries 3,4 are now outside count → not revisited, keep old value */
    T_ASSERT_EQ_I(ents[4].selected, 1);
    return 0;
}
