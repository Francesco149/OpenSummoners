/*
 * party.{c,h} — the "dramatist" character system: the spawn-time identity
 * resolution that turns a 32-bit character HANDLE into a sprite (code + bank).
 *
 * Engine correspondence (proven: docs/proofs/dramatist-table.md, engine-quirk
 * #91).  The engine identifies a NAMED character (a "dramatist") by a 32-bit
 * handle, not by its actor code.  The EFFECT activator 0x41f200 logs the
 * literal "Get Dramatist Info" at 0x41f200:51, then (:54-69) — when spawned with
 * a non-zero handle — scans the static table DAT_006b6ea8 for that handle and:
 *   - sets the actor's effective CODE (+0x1d4) = the row's code, the ARCHETYPE
 *     that drives 0x41f200's sprite-install switch (gated on code_in==0, since
 *     the 0x41f0e0 spawn path passes the activator's param_4 = the constant 3,
 *     so the row's code only takes over when the caller passed code 0);
 *   - carries the row's BANK (+0x30) as sVar17, which OVERRIDES the archetype's
 *     per-facing default sheet: each archetype case does
 *         if (sVar17 == 0) sVar17 = <facing default>;
 *         0x426d70(0, sVar17, 0);   // install sprite row 0 = bank sVar17
 *
 * So CODE = the archetype (pose/clip set, shared by many NPCs) and BANK = the
 * specific sheet (recolor/variant); the handle picks both.  A dramatist bank of
 * 0 means "no static sheet" — the character's sprite is loaded dynamically by
 * the party/new-game path (the playable leads, e.g. Arche 0xc35a, whose case
 * installs a 4-bank body 0x8b-0x8e + clip 0x62a8c8 the party path must
 * register; that path is unported, so those banks cull until then).
 *
 * This module ports the STATIC def table + the resolution; it is pure and
 * host-tested.  The distinct LIVE-actor handle registry (DAT_008a9b50+0x2790,
 * add 0x555f00 / resolve 0x556eb0 — scans live actors for +0x1d8 == handle, used
 * by the dialogue runner to find a speaker) is a separate, later chip (Phase 3).
 *
 * Win32-free + pure: numeric RE facts only (handle/code/bank).  The display name
 * is a trailing /​* comment *​/ for readability (the proof already publishes the
 * names); the port reads the live table from the user's own exe for the dialogue
 * port, it does not embed name strings.
 */
#ifndef OSS_PARTY_H
#define OSS_PARTY_H

#include <stdint.h>

/*
 * One row of the retail "Get Dramatist Info" table DAT_006b6ea8 (0x34 bytes/row
 * in retail; the port keeps only the three fields the resolver reads).  Generated
 * from the user's own binary by tools/dump_dramatist_table.py — regenerate that
 * table below if it ever drifts (the names are documentation, not asset data).
 */
typedef struct party_dramatist_row {
    uint32_t handle;   /* +0x00 — key (the live table is handle==0 terminated) */
    uint32_t code;     /* +0x04 — archetype / 0x41f200 sprite-switch selector  */
    uint16_t bank;     /* +0x30 — primary sheet (0 => dynamic/party-loaded)    */
} party_dramatist_row;

/*
 * Scan the dramatist table for `handle` (the 0x41f200:54-69 lookup).  Returns
 * the matching row, or NULL when handle==0 or no row matches.
 */
const party_dramatist_row *party_dramatist_find(uint32_t handle);

/*
 * The per-archetype DEFAULT sprite bank — retail's `if (sVar17 == 0) sVar17 =
 * <facing default>` arm in each 0x41f200 case.  `facing_sel` is the activator's
 * param_11 (1/2/3 pick the directional sheet; anything else the base).  Returns
 * the default bank, or 0 for an archetype with no static default (unknown to the
 * RE'd subset below, or a party-loaded lead like Arche 0xc35a).
 *
 * Only the cases read off the decompile are modelled (the town-arrival
 * archetypes + their immediate neighbours); extend as more 0x41f200 cases are
 * RE'd.  PORT-DEBT(dramatist-archetype-defaults).
 */
uint16_t party_archetype_default_bank(uint32_t code, int facing_sel);

/*
 * Port of 0x41f200's spawn-time identity resolution (:54-69 + the archetype
 * default-bank arm) — compute the EFFECTIVE actor code and the sprite BANK the
 * activator installs (0x426d70(0, bank, 0)) for a spawn:
 *
 *   - `handle`     : 0x41f0e0 arg1 (0 => spawned by code, no dramatist).
 *   - `code_in`    : 0x41f0e0 arg2 (the explicit archetype; 0 => take the
 *                    dramatist row's code).
 *   - `facing_sel` : the activator's param_11 (0x41f0e0 arg8) — selects the
 *                    archetype default when there is no dramatist bank override.
 *
 * Resolution (the spawn path, where the activator's param_4 is the constant 3,
 * so the code override is gated on code_in==0, NOT param_4==1):
 *   1. handle != 0 and matched: bank = row.bank (the OVERRIDE); if code_in==0,
 *      code = row.code (the archetype).
 *   2. bank still 0 (no handle, or a dramatist bank of 0): bank =
 *      party_archetype_default_bank(code, facing_sel).
 *
 * Writes *code_out (always) and *bank_out.  Returns 1 when a usable static bank
 * was resolved (*bank_out > 0), 0 otherwise — e.g. a party-loaded lead whose
 * sheet the unported new-game path must register (Arche resolves to code 0xc35a,
 * bank 0: *bank_out is left 0 and the caller defers her to the party band).
 */
int party_resolve_spawn(uint32_t handle, uint32_t code_in, int facing_sel,
                        uint32_t *code_out, uint16_t *bank_out);

/* ══════════════════════════════════════════════════════════════════════════
 * Party band Phase 2 — the party-member STAT record + the room+0x4030 slots.
 *
 * The freeroam HUD (FUN_00494e60; src/hud.c + main.c game_render_hud) reads the
 * leader's HP/MP/level/element-stars/EXP from the member's stat block at
 * member+0x750.  `party_stats` models the SUBSET of that block the HUD reads
 * (RE'd off 494e60 + the number helpers 497b40/497bb0/496970 + the star drawer
 * 498620; full contract in docs/plans/party-band-phase2-hud-data.md).  Field
 * comments are byte offsets relative to member+0x750.  Retires the leader-panel
 * half of PORT-DEBT(hud-party-context): the values come from a real member
 * record populated from the character's level def, not the hardcoded 100/20/1
 * HUD_*_VALUE stand-ins.
 * ══════════════════════════════════════════════════════════════════════════ */

typedef struct party_stats {
    int32_t hp_cur;                       /* +0x54 — current HP                 */
    int32_t hp_base, hp_equip, hp_buff;   /* +0x58 / +0x84 / +0x9c (max = sum)  */
    int32_t mp_cur;                       /* +0x5c — current MP                 */
    int32_t mp_base, mp_equip, mp_buff;   /* +0x60 / +0x88 / +0xa0 (max = sum)  */
    int32_t level_base;                   /* +0xe0                              */
    int32_t level_bonus;                  /* +0xd8 (level = base + bonus)        */
    int32_t star_count;                   /* +0xdc — element-affinity star count */
    int32_t exp_cur, exp_max;             /* +0xe8 / +0xec                      */
} party_stats;

/* max HP/MP = the 3 summed terms (494e60:109/116; 497b40/497bb0); clamped >= 0. */
int party_stat_hp_max(const party_stats *s);
int party_stat_mp_max(const party_stats *s);

/* The settled HP/MP ratio = cur*1000/max(1,max), clamped [0,1000] — the value
 * retail's animator (0x49af40) lerps toward.  At full HP (the errands, no
 * combat) it IS this value with no transient, so the port computes it directly. */
int party_stat_hp_ratio(const party_stats *s);
int party_stat_mp_ratio(const party_stats *s);

/* The displayed CURRENT HP/MP number (port of 0x497b40 / 0x497bb0): when `ratio`
 * equals the true cur ratio return the exact cur, else the interpolated
 * max*ratio/1000 (the mid-lerp readout).  Faithful to retail's number path. */
int party_stat_hp_display(const party_stats *s, int ratio);
int party_stat_mp_display(const party_stats *s, int ratio);

/* level = level_base + level_bonus (494e60:123). */
int party_stat_level(const party_stats *s);

/* ── the room+0x4030 party (8 slots + leader) ──────────────────────────────
 * The retail slot (operator_new(0xeec)) carries active +0x9c4, handle +0x9f0,
 * and a member ptr +0x9f4 -> the member entity (whose +0x750 is the stats).
 * The port collapses that slot->member indirection into an inline member (the
 * slot IS the container here); active==0 / handle==0 marks an empty slot
 * (retail's gate: active==1 && handle != the empty sentinel 0x5f5e168). */
#define PARTY_SLOT_COUNT     8
#define PARTY_HANDLE_EMPTY   0x5f5e168u   /* the "no member" sentinel (4961a0)   */

typedef struct party_member {
    uint32_t    handle;    /* slot +0x9f0 — the dramatist handle (0 = empty)    */
    int         active;    /* slot +0x9c4 == 1                                  */
    party_stats stats;     /* member+0x750 (the HUD-read subset)                */
} party_member;

typedef struct party {
    party_member slots[PARTY_SLOT_COUNT]; /* room+0x4030..+0x404c              */
    int          leader;   /* index of room+0x200c's member; -1 = none          */
} party;

/* The leader member, or NULL when there is no active leader. */
const party_member *party_leader(const party *p);

/* Port of FUN_00426fd0 — fill a member's stats from the (code, level) base-stat
 * table row (base_stat_find, base_stat_table.h).  Sets HP/MP base = cur = row
 * HP/MP (equip/buff terms 0), level_base = row.level (bonus 0), exp_cur = 0,
 * exp_max = row.exp_max, and CLEARS element/star_count (retail 426fd0:101-108
 * zeroes +0xd8/+0xdc — those are set later by the equip subsystem, NOT the base
 * table). */
void party_stats_init(party_stats *s, uint32_t code, int level);

/* Populate `p` for the errands freeroam: Arche (dramatist handle 0x5f5e165 ->
 * code 0xc35a) as the lone level-1 leader in slot 0, her stats from the base-stat
 * table.  Mirrors retail's persistent-party create (FUN_004cc820(0, code 0, lvl 1,
 * handle 0x5f5e165, element 2, active 1) in the FUN_004e59a0 story dispatch). */
void party_init_errands(party *p);

#endif /* OSS_PARTY_H */
