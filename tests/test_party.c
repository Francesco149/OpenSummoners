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
#include "base_stat_table.h"
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

/* ══════════════════════════════════════════════════════════════════════════
 * Party band Phase 2 — the party-member stat accessors + the leader gate.
 * The formulas are RE'd off FUN_00494e60 (:109/:116/:123) + 00497b40/00497bb0
 * (the number path); see docs/plans/party-band-phase2-hud-data.md.
 * ══════════════════════════════════════════════════════════════════════════ */

int test_party_stat_maxes(void)
{
    party_stats s = {0};
    s.hp_base = 90; s.hp_equip = 8; s.hp_buff = 2;    /* 494e60:109 3-term sum */
    s.mp_base = 18; s.mp_equip = 2; s.mp_buff = 0;    /* 494e60:116            */
    T_ASSERT_EQ_I(party_stat_hp_max(&s), 100);
    T_ASSERT_EQ_I(party_stat_mp_max(&s), 20);
    return 0;
}

int test_party_stat_ratio_and_display(void)
{
    party_stats s = {0};
    s.hp_base = 100;                       /* max 100 */
    s.hp_cur  = 100;                       /* full */
    T_ASSERT_EQ_I(party_stat_hp_ratio(&s), 1000);
    /* full HP: the settled ratio yields the exact cur (497b40 exact-match arm) */
    T_ASSERT_EQ_I(party_stat_hp_display(&s, party_stat_hp_ratio(&s)), 100);

    s.hp_cur = 40;                         /* 40/100 -> ratio 400 */
    T_ASSERT_EQ_I(party_stat_hp_ratio(&s), 400);
    T_ASSERT_EQ_I(party_stat_hp_display(&s, 400), 40);   /* exact-match arm       */
    T_ASSERT_EQ_I(party_stat_hp_display(&s, 500), 50);   /* interp (100*500)/1000 */

    /* zero stats: max is the raw 0, the ratio denom clamps at 1 (no div-by-0). */
    party_stats z = {0};
    T_ASSERT_EQ_I(party_stat_hp_max(&z), 0);
    T_ASSERT_EQ_I(party_stat_hp_ratio(&z), 0);

    /* MP mirrors HP (497bb0). */
    party_stats m = {0};
    m.mp_base = 20; m.mp_cur = 20;
    T_ASSERT_EQ_I(party_stat_mp_ratio(&m), 1000);
    T_ASSERT_EQ_I(party_stat_mp_display(&m, party_stat_mp_ratio(&m)), 20);
    return 0;
}

int test_party_stat_level(void)
{
    party_stats s = {0};
    s.level_base = 1; s.level_bonus = 0;                 /* 494e60:123 */
    T_ASSERT_EQ_I(party_stat_level(&s), 1);
    s.level_base = 5; s.level_bonus = 2;
    T_ASSERT_EQ_I(party_stat_level(&s), 7);
    return 0;
}

int test_party_leader_gate(void)
{
    party p = {0};
    p.leader = -1;
    T_ASSERT_EQ_P(party_leader(&p), NULL);               /* no leader */

    p.slots[0].handle = 0x5f5e165u;                      /* Arche */
    p.slots[0].active = 1;
    p.leader = 0;
    T_ASSERT_EQ_P(party_leader(&p), &p.slots[0]);

    p.slots[0].active = 0;                               /* inactive -> gated out */
    T_ASSERT_EQ_P(party_leader(&p), NULL);

    p.slots[0].active = 1;
    p.slots[0].handle = PARTY_HANDLE_EMPTY;              /* sentinel -> gated out */
    T_ASSERT_EQ_P(party_leader(&p), NULL);
    return 0;
}

/* ---- the base-stat table + the FUN_00426fd0 stats init ------------------- */

int test_party_base_stat_find(void)
{
    /* Arche lvl 1 (0xc35a,1): the byte-proven row (HP 100 / MP 20 / EXP 250). */
    const base_stat_row *r = base_stat_find(0x0c35au, 1);
    T_ASSERT(r != NULL);
    T_ASSERT_EQ_U(r->code, 0x0c35au);
    T_ASSERT_EQ_I(r->level, 1);
    T_ASSERT_EQ_I(r->hp, 100);
    T_ASSERT_EQ_I(r->mp, 20);
    T_ASSERT_EQ_I(r->exp_max, 250);

    /* level 0 picks the character's lowest matching row (426fd0's level==0 arm). */
    r = base_stat_find(0x0c35au, 0);
    T_ASSERT_EQ_I(r->level, 1);

    /* lvl 2 = HP 120 (a per-level authored row, not a growth formula). */
    r = base_stat_find(0x0c35au, 2);
    T_ASSERT_EQ_I(r->hp, 120);

    /* an unknown code -> the fallback row 0 (retail &DAT_0067ac58 = code 1). */
    r = base_stat_find(0xdeadu, 1);
    T_ASSERT_EQ_U(r->code, 1u);
    return 0;
}

int test_party_stats_init_arche(void)
{
    party_stats s;
    party_stats_init(&s, 0x0c35au, 1);
    /* HP/MP base = cur = the table max; the equip/buff terms are cleared. */
    T_ASSERT_EQ_I(party_stat_hp_max(&s), 100);
    T_ASSERT_EQ_I(s.hp_cur, 100);
    T_ASSERT_EQ_I(party_stat_mp_max(&s), 20);
    T_ASSERT_EQ_I(s.mp_cur, 20);
    T_ASSERT_EQ_I(s.hp_equip, 0);
    T_ASSERT_EQ_I(s.hp_buff, 0);
    /* level = base(1)+bonus(0); EXP cur 0 / max 250; star_count CLEARED (0xdc). */
    T_ASSERT_EQ_I(party_stat_level(&s), 1);
    T_ASSERT_EQ_I(s.exp_cur, 0);
    T_ASSERT_EQ_I(s.exp_max, 250);
    T_ASSERT_EQ_I(s.star_count, 0);
    return 0;
}

int test_party_init_errands(void)
{
    party p;
    party_init_errands(&p);

    /* Arche is the leader (slot 0), handle 0x5f5e165, active. */
    const party_member *ld = party_leader(&p);
    T_ASSERT(ld != NULL);
    T_ASSERT_EQ_U(ld->handle, 0x5f5e165u);
    T_ASSERT_EQ_I(p.leader, 0);

    /* The HUD leader-panel values, end-to-end: 100/100, 20/20, Lv 1, 2 stars. */
    const party_stats *st = &ld->stats;
    T_ASSERT_EQ_I(party_stat_hp_max(st), 100);
    T_ASSERT_EQ_I(party_stat_hp_display(st, party_stat_hp_ratio(st)), 100);
    T_ASSERT_EQ_I(party_stat_mp_max(st), 20);
    T_ASSERT_EQ_I(party_stat_mp_display(st, party_stat_mp_ratio(st)), 20);
    T_ASSERT_EQ_I(party_stat_level(st), 1);
    T_ASSERT_EQ_I(st->star_count, 2);          /* PORT-DEBT stand-in (equip stone) */

    /* The other 7 slots are empty (inactive). */
    for (int i = 1; i < PARTY_SLOT_COUNT; i++)
        T_ASSERT_EQ_I(p.slots[i].active, 0);
    return 0;
}
