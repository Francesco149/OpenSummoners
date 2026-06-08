/*
 * test_party.c — host tests for src/party.c (the dramatist table DAT_006b6ea8 +
 * the 0x41f200 spawn-time handle->code/bank resolution).
 *
 * Ground truth: docs/proofs/dramatist-table.md (the three-source-agreement proof)
 * + the 0x41f200 archetype cases read off the decompile.  The decisive test is
 * the town-arrival cast resolving to the SAME (code, bank) retail's 0x41f200
 * installs — most importantly Arche's Mother getting her OWN sheet 0xb5 (not the
 * generic map townswoman's 0xa6) and Arche resolving to bank 0 (party-loaded).
 */
#include "party.h"
#include "t.h"

/* ---- the dramatist lookup: known / unknown / zero ------------------------- */

int test_party_dramatist_find(void)
{
    const party_dramatist_row *r;

    /* The arrival handles (proof table). */
    r = party_dramatist_find(0x5f5e165u);                 /* Arche */
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r->code, 0xc35au);
    T_ASSERT_EQ_U(r->bank, 0u);                           /* party-loaded */

    r = party_dramatist_find(0x5f5e1d3u);                 /* Arche's Father */
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r->code, 0xc3dcu);
    T_ASSERT_EQ_U(r->bank, 0xe3u);

    r = party_dramatist_find(0x5f5e1d4u);                 /* Arche's Mother */
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r->code, 0xc440u);
    T_ASSERT_EQ_U(r->bank, 0xb5u);

    r = party_dramatist_find(0x35a4e901u);                /* Dr. Barnard */
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r->code, 0xc3f0u);
    T_ASSERT_EQ_U(r->bank, 0xecu);                        /* his own-handle sheet */

    /* handle 0 is the table terminator -> never a match (0x41f200:54 guard). */
    T_ASSERT_EQ_P(party_dramatist_find(0u), NULL);
    /* an unknown handle misses. */
    T_ASSERT_EQ_P(party_dramatist_find(0xdeadbeefu), NULL);
    return 0;
}

/* ---- the archetype per-facing default bank (the `if (sVar17==0)` arm) ------ */

int test_party_archetype_default_bank(void)
{
    /* 0xc440 "Woman" (Mother archetype), 4-way (:1768): else/1/2/3 = a5/a6/a7/a8. */
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc440u, 0), 0xa5u);
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc440u, 1), 0xa6u);
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc440u, 2), 0xa7u);
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc440u, 3), 0xa8u);

    /* 0xc3f0 Dr. Barnard (:1456): else/1/2 = eb/ec/ed (no facing-3 branch -> base). */
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc3f0u, 0), 0xebu);
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc3f0u, 1), 0xecu);
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc3f0u, 2), 0xedu);
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc3f0u, 3), 0xebu);   /* falls to base */

    /* 0xc3dc Father / man (:1386), 3-way: else/1/2/3 = dd/de/df/e0. */
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc3dcu, 0), 0xddu);
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc3dcu, 3), 0xe0u);

    /* 0xc3dd 2-way (:1402): 1 = e2, else = e1. */
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc3ddu, 1), 0xe2u);
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc3ddu, 0), 0xe1u);

    /* an archetype outside the RE'd subset returns 0. */
    T_ASSERT_EQ_U(party_archetype_default_bank(0xc35au, 0), 0u);  /* Arche: party-loaded */
    T_ASSERT_EQ_U(party_archetype_default_bank(0x12345u, 1), 0u);
    return 0;
}

/* ---- the full spawn resolution: the town-arrival cast (the deliverable) ---- */

int test_party_resolve_arrival_cast(void)
{
    uint32_t code; uint16_t bank; int ok;

    /* Dr. Barnard: 0x41f0e0(handle 0, code 0xc3f0, facing_sel 0).  No handle ->
     * archetype 0xc3f0 default, facing_sel 0 -> base bank 0xeb. */
    ok = party_resolve_spawn(0u, 0xc3f0u, 0, &code, &bank);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(code, 0xc3f0u);
    T_ASSERT_EQ_U(bank, 0xebu);

    /* Arche's Father: 0x41f0e0(handle 0x5f5e1d3, code 0).  Handle resolves ->
     * code 0xc3dc, bank OVERRIDE 0xe3 (NOT the facing default). */
    ok = party_resolve_spawn(0x5f5e1d3u, 0u, 0, &code, &bank);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(code, 0xc3dcu);
    T_ASSERT_EQ_U(bank, 0xe3u);

    /* Arche's Mother: 0x41f0e0(handle 0x5f5e1d4, code 0).  Handle resolves ->
     * code 0xc440 (the "Woman" archetype), bank OVERRIDE 0xb5 — her own sheet,
     * NOT the generic map townswoman's default 0xa6.  THE ckpt-92 fix. */
    ok = party_resolve_spawn(0x5f5e1d4u, 0u, 0, &code, &bank);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(code, 0xc440u);
    T_ASSERT_EQ_U(bank, 0xb5u);
    T_ASSERT(bank != 0xa6u);                 /* != the generic map townswoman */

    /* The generic MAP townswoman (0xc440 by code, no handle, facing 1) keeps the
     * archetype default 0xa6 — distinct from Mom, so both coexist on screen. */
    ok = party_resolve_spawn(0u, 0xc440u, 1, &code, &bank);
    T_ASSERT_EQ_I(ok, 1);
    T_ASSERT_EQ_U(code, 0xc440u);
    T_ASSERT_EQ_U(bank, 0xa6u);

    /* Arche (the party leader): handle resolves -> code 0xc35a, dramatist bank 0,
     * and her archetype has no static default -> bank 0, return 0 (the caller
     * defers her to the party band; her 0x8b-0x8e are party-loaded). */
    ok = party_resolve_spawn(0x5f5e165u, 0u, 0, &code, &bank);
    T_ASSERT_EQ_I(ok, 0);
    T_ASSERT_EQ_U(code, 0xc35au);
    T_ASSERT_EQ_U(bank, 0u);
    return 0;
}
