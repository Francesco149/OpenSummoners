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

#endif /* OSS_PARTY_H */
