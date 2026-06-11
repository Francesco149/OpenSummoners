/*
 * tests/test_portrait.c — host tests for the dialogue-portrait resolver
 * (src/portrait.c): the face-table lookup (head_state, face_id) -> portrait
 * pool-slot, across the 3 anim-state variants.
 *
 * The expected slots are the retail face table DAT_006b6568 (embedded in
 * portrait_face_data.c) for the town-arrival speakers, with the head-states
 * harness-captured (runs/portrait-gt): Arche=100000101, Father=100000211,
 * Mother=100000212.  Variant B (+0xa, the default path) is the town-dialogue
 * column; A (+0x8) and C (+0xc) are checked too.
 */
#include "../src/portrait.h"
#include "t.h"

/* ── variant B (the default / town-dialogue column) per speaker+face ── */
int test_portrait_resolve_variant_b(void)
{
    /* Arche (head 100000101) */
    T_ASSERT_EQ_I(portrait_resolve(100000101, 0x02, PORTRAIT_VAR_B), 460);
    T_ASSERT_EQ_I(portrait_resolve(100000101, 0x03, PORTRAIT_VAR_B), 461);
    T_ASSERT_EQ_I(portrait_resolve(100000101, 0x09, PORTRAIT_VAR_B), 467);
    T_ASSERT_EQ_I(portrait_resolve(100000101, 0x0d, PORTRAIT_VAR_B), 471);
    /* Father (head 100000211) + Mother (head 100000212), neutral face 0x1e */
    T_ASSERT_EQ_I(portrait_resolve(100000211, 0x1e, PORTRAIT_VAR_B), 683);
    T_ASSERT_EQ_I(portrait_resolve(100000212, 0x1e, PORTRAIT_VAR_B), 704);
    return 0;
}

/* ── the A and C variant columns of the same records ── */
int test_portrait_resolve_variants_ac(void)
{
    T_ASSERT_EQ_I(portrait_resolve(100000101, 0x02, PORTRAIT_VAR_A), 439);
    T_ASSERT_EQ_I(portrait_resolve(100000101, 0x02, PORTRAIT_VAR_C), 481);
    T_ASSERT_EQ_I(portrait_resolve(100000211, 0x1e, PORTRAIT_VAR_A), 676);
    T_ASSERT_EQ_I(portrait_resolve(100000211, 0x1e, PORTRAIT_VAR_C), 690);
    return 0;
}

/* ── different speakers / faces resolve to DIFFERENT slots (the un-MVP: the
 *    bust is no longer one hardcoded slot) ── */
int test_portrait_resolve_distinct(void)
{
    int arche  = portrait_resolve(100000101, 0x02, PORTRAIT_VAR_B);
    int father = portrait_resolve(100000211, 0x1e, PORTRAIT_VAR_B);
    int mother = portrait_resolve(100000212, 0x1e, PORTRAIT_VAR_B);
    T_ASSERT(arche != father);
    T_ASSERT(father != mother);
    T_ASSERT(arche != mother);
    /* the old hardcoded MVP slot (663 = head 100000104) is NONE of the real
     * town speakers — the bug the un-MVP fixes */
    T_ASSERT(arche != 663 && father != 663 && mother != 663);
    return 0;
}

/* ── no record -> -1 (retail's no-portrait path, 0x49d6e0:132-134) ── */
int test_portrait_resolve_missing(void)
{
    T_ASSERT_EQ_I(portrait_resolve(999999, 0x1e, PORTRAIT_VAR_B), -1);  /* bad head */
    T_ASSERT_EQ_I(portrait_resolve(100000101, 0x77, PORTRAIT_VAR_B), -1); /* bad face */
    return 0;
}
