/*
 * cutscene.c — the town-intro multi-room dialogue chain (see cutscene.h).
 */
#include "cutscene.h"
#include "portrait.h"

#include <stddef.h>

/* Speaker dramatist NAME VAs (DAT_006b6ea8 rows, +0x08 inline name;
 * tools/dump_dramatist_table.py).  The real script passes name_or_0 == 0 and
 * resolves the name from the speaker ACTOR's +0x750 (= the dramatist name); the
 * port hardcodes the VA, having mapped each actor id to its dramatist row.  The
 * actor-id → name map (0x556eb0(id) → actor) is confirmed against the
 * arrival's known speakers: 0x5f5e165=Arche, 0x5f5e1d3=Father, 0x5f5e1d4=Mother. */
#define NAME_ARCHE   0x6b6eb0u   /* "Arche"          (actor 0x5f5e165) */
#define NAME_FATHER  0x6b6f80u   /* "Arche's Father" (actor 0x5f5e1d3) */
#define NAME_MOTHER  0x6b6fb4u   /* "Arche's Mother" (actor 0x5f5e1d4) */

/* The speaker's head-state (actor +0x1d8) — the face-table key (portrait.c).
 * HARNESS-CAPTURED off 0x49d6e0's speaker arg (runs/portrait-gt): each speaker's
 * head-state is constant across all lines.  The portrait resolves from
 * (head_state, line face) — the per-speaker BUST the MVP couldn't show. */
#define HEAD_ARCHE   100000101
#define HEAD_FATHER  100000211
#define HEAD_MOTHER  100000212

static int32_t speaker_head_state(uint32_t name_va)
{
    switch (name_va) {
    case NAME_ARCHE:  return HEAD_ARCHE;
    case NAME_FATHER: return HEAD_FATHER;
    case NAME_MOTHER: return HEAD_MOTHER;
    default:          return 0;   /* unknown speaker → no portrait record */
    }
}

/* The speaker's body geometry the box-anchor projection reads (0x49c640).
 * HARNESS-CAPTURED per character (runs/box-pos-inputs, the 0x49c640 field-spec):
 * the whole town cast share sprite_w=2000 / metric_14=0 / off_20=32; the child
 * Arche differs in metric_10 (5600 vs the adults' 7600) and off_1c (-8 vs 0 — her
 * head sits lower, so her box drops 8px, e.g. arrival L2 box_y 160 vs Father 148).
 * Captured because the live cast that owns these fields is PORT-DEBT
 * (cutscene-party-chars / cutscene-room-render Phase 2b). */
static const dialogue_speaker_body BODY_ARCHE = { 2000, 5600, 0, -8, 32 };
static const dialogue_speaker_body BODY_ADULT = { 2000, 7600, 0,  0, 32 };

static const dialogue_speaker_body *speaker_body(uint32_t name_va)
{
    return (name_va == NAME_ARCHE) ? &BODY_ARCHE : &BODY_ADULT;
}

/* The face-table variant is PER LINE (the `pvar` column of the script tables).
 * 0x49d6e0 picks one of each record's 3 portrait-slot variants by the speaker's
 * BODY-FACING (0x49d6e0:143, body+0x2c == 3 → A=+0x8, else B=+0xa) + the bVar4
 * box-state (→ C=+0xc).  bVar4 is FALSE for the town dialogue (in_ECX+0x2f0==0,
 * captured), so it is A or B — and the variants are DIFFERENT busts/sizes (e.g.
 * Father A=676 is 160x176 but B=683 is 176x144), so the right one is required
 * for a 1:1 portrait.  The facing is DYNAMIC (the cast turns mid-scene) and the
 * port's cast is STATIC (PORT-DEBT(cutscene-party-chars)), so the per-line
 * variant is BAKED from the harness capture (runs/portrait-gt — read off the
 * beat-runner thunk 0x439680's +0x84, no lag) into each line's `pvar`.
 * PORT-DEBT(dialogue-portrait-facing): when the animated cast lands, pvar will
 * derive from the speaker's live facing instead of this captured bake. */
#define VA PORTRAIT_VAR_A      /* facing==3 (the +0x8 column) */
#define VB PORTRAIT_VAR_B      /* the default (+0xa column)   */

/* The town-gate family conversation — 0x4d7d80 case 0x334be, the first-run
 * (flag 0x5f76805 == 0) path, decompile lines 33-292.  Ten 0x49d6e0 calls,
 * in order; each row = (speaker dramatist NAME VA, line TEXT VA, face, voice).
 * Text VAs are the 0x4d7d80 pcVar6 string labels; faces are uVar9, voices uVar5
 * (0x3eb..0x3f4, sequential — voice deferred).  All strings live in the user's
 * sotes.exe and are read at runtime by VA (never embedded). */
static const cutscene_line TOWN_ARRIVAL[] = {
    /*  # decompile  speaker  name         text       face  voice pvar  wx     wy     gist */
    /*  1 :105 */ { NAME_FATHER, 0x86d58cu, 0x1e, 0x3eb, VA, 49600, 43600 }, /* "Ahh, here we are at last!…" */
    /*  2 :119 */ { NAME_ARCHE,  0x86d55cu, 0x02, 0x3ec, VB, 41600, 45600 }, /* "Yay, we're finally here!…"  */
    /*  3 :133 */ { NAME_MOTHER, 0x86d500u, 0x1e, 0x3ed, VB, 38400, 43600 }, /* "We haven't been here since…"*/
    /*  4 :147 */ { NAME_ARCHE,  0x86d4c8u, 0x03, 0x3ee, VB, 41600, 45600 }, /* "Yeah! There's people and…"  */
    /*  5 :161 */ { NAME_ARCHE,  0x86d47cu, 0x09, 0x3ef, VB, 41600, 45600 }, /* "Hey, Dad! Our shop is in…"  */
    /*  6 :175 */ { NAME_ARCHE,  0x86d45cu, 0x0d, 0x3f0, VB, 41600, 45600 }, /* "I wanna see it! Where is it?"*/
    /*  7 :196 */ { NAME_FATHER, 0x86d42cu, 0x1e, 0x3f1, VB, 49600, 43600 }, /* "Mm-hmm. It's just down the…" */
    /*  8 :210 */ { NAME_ARCHE,  0x86d424u, 0x03, 0x3f2, VB, 41600, 45600 }, /* "Cool!"                       */
    /*  9 :248 */ { NAME_ARCHE,  0x86d410u, 0x03, 0x3f3, VA, 73104, 45600 }, /* "Mom! Dad! c'mon!" (runs ahead)*/
    /* 10 :262 */ { NAME_MOTHER, 0x86d3d4u, 0x1e, 0x3f4, VB, 38400, 43600 }, /* "Hmhm. Well, wait up for…"    */
};
#define TOWN_ARRIVAL_COUNT ((int)(sizeof(TOWN_ARRIVAL) / sizeof(TOWN_ARRIVAL[0])))

/* The new-house interior — 0x4d7d80 case 0x334c8, the first-run (flag
 * 0x5f76805 == 0) path, decompile lines 1029-1218.  Eight 0x49d6e0 calls,
 * in order.  One non-dialogue beat (the actor emote 0x401e60(Arche,1) at
 * :1170, between lines 6 and 7) is SKIPPED — PORT-DEBT(cutscene-beat-runner).
 * Voices are all 0 (the house lines are unvoiced).  After line 8 the script
 * stages room 0x334dc (the errands/freeroam) via 0x401d40 and `return 2`. */
static const cutscene_line TOWN_HOUSE[] = {
    /*  # decompile   speaker  name         text       face  voice pvar  wx      wy     gist */
    /*  1 :1093 */ { NAME_ARCHE,  0x86d390u, 0x0d, 0, VA, 128000, 39200 }, /* "So this is our new house!…"  */
    /*  2 :1107 */ { NAME_ARCHE,  0x86d35cu, 0x0d, 0, VA, 128000, 39200 }, /* "Hee, there's even an item…"  */
    /*  3 :1121 */ { NAME_MOTHER, 0x86d318u, 0x1e, 0, VA, 131200, 37200 }, /* "Oh, this is lovely. …your…" */
    /*  4 :1135 */ { NAME_FATHER, 0x86d2d4u, 0x1e, 0, VA, 134400, 37200 }, /* "Mm-hmm. I'm hoping I can…"   */
    /*  5 :1149 */ { NAME_FATHER, 0x86d294u, 0x1e, 0, VA, 134400, 37200 }, /* "…helping the townsfolk out…"*/
    /*  6 :1163 */ { NAME_FATHER, 0x86d240u, 0x1e, 0, VA, 134400, 37200 }, /* "…Well, Arche, I'll be count…"*/
    /*  7 :1184 */ { NAME_ARCHE,  0x86d22cu, 0x03, 0, VB, 128024, 39200 }, /* "I will, I promise."          */
    /*  8 :1198 */ { NAME_MOTHER, 0x86d1dcu, 0x1e, 0, VA, 131200, 37200 }, /* "And today, we need your help"*/
};
#define TOWN_HOUSE_COUNT ((int)(sizeof(TOWN_HOUSE) / sizeof(TOWN_HOUSE[0])))

/* The room chain: arrival → house.  Each room carries its committed key (the
 * 0x402030 +0x4024 value) so the caller can drive the backdrop + detect the
 * errands boundary.  The chain ends after the house (its retail `return 2`
 * stages 0x334dc, the errands room = the freeroam hand-off). */
static const cutscene_room TOWN_CHAIN[] = {
    { CUTSCENE_ROOM_ARRIVAL, TOWN_ARRIVAL, TOWN_ARRIVAL_COUNT },
    { CUTSCENE_ROOM_HOUSE,   TOWN_HOUSE,   TOWN_HOUSE_COUNT   },
};
#define TOWN_CHAIN_COUNT ((int)(sizeof(TOWN_CHAIN) / sizeof(TOWN_CHAIN[0])))

const cutscene_line *cutscene_town_arrival(int *n)
{
    if (n != NULL) *n = TOWN_ARRIVAL_COUNT;
    return TOWN_ARRIVAL;
}

const cutscene_line *cutscene_town_house(int *n)
{
    if (n != NULL) *n = TOWN_HOUSE_COUNT;
    return TOWN_HOUSE;
}

const cutscene_room *cutscene_town_chain(int *n_rooms)
{
    if (n_rooms != NULL) *n_rooms = TOWN_CHAIN_COUNT;
    return TOWN_CHAIN;
}

/* Resolve rooms[room_idx].script[line_idx]'s name+text VAs and arm the box.
 * Returns 1 on success; 0 if the text VA does not resolve (the caller treats a
 * room-0 line-0 miss as "disarmed" and any later miss as "complete").  A NULL
 * name is tolerated (dialogue_arm copies nothing). */
/* How to bring up the line's box (THEME 1 cadence — see plans/intro-cutscene-1to1.md):
 *   ARM_OPEN   the FIRST line of the conversation: full slide-in (dialogue_arm,
 *              scale 0 -> ~20-update entrance).
 *   ARM_REOPEN a mid-conversation SPEAKER CHANGE: the box was up, so it re-opens
 *              from half scale (dialogue_reopen, ~10-update pop-in = retail's
 *              advance+11t reopen).
 *   ARM_KEEP   the SAME speaker continues: keep the box open, re-text only
 *              (dialogue_set_text, gap 1t). */
enum { ARM_OPEN = 0, ARM_REOPEN = 1, ARM_KEEP = 2 };

static int arm_current_line(cutscene *cs, int mode)
{
    const cutscene_room *room = &cs->rooms[cs->room_idx];
    const cutscene_line *ln   = &room->script[cs->line_idx];
    const char *text = cs->resolve(ln->text_va);
    if (text == NULL)
        return 0;
    if (mode == ARM_KEEP && cs->box->active)
        dialogue_set_text(cs->box, text);          /* same speaker — keep the box   */
    else if (mode == ARM_REOPEN && cs->box->active)
        dialogue_reopen(cs->box, cs->resolve(ln->name_va), text); /* speaker change */
    else
        dialogue_arm(cs->box, cs->resolve(ln->name_va), text);    /* first open     */
    /* Resolve the per-speaker portrait bust (0x49d6e0's face-table lookup): the
     * speaker's head-state + this line's face → the portrait pool-slot.  -1 (no
     * record) leaves the box's reset -1 → no portrait, faithful to +0x20=1. */
    cs->box->portrait_slot = portrait_resolve(speaker_head_state(ln->name_va),
                                              ln->face, (portrait_variant)ln->pvar);
    /* Arm the box-position anchor (0x49c640): the speaker's world pos + its body
     * geometry; the caller projects them through the live camera each frame so
     * the box rides the speaker (dialogue_box_position). */
    cs->box->anchored = 1;
    cs->box->spk_wx   = ln->spk_wx;
    cs->box->spk_wy   = ln->spk_wy;
    cs->box->spk_body = *speaker_body(ln->name_va);
    return 1;
}

void cutscene_arm(cutscene *cs, const cutscene_room *rooms, int n_rooms,
                  cutscene_str_resolver resolve, dialogue_box *box)
{
    if (cs == NULL) return;
    cs->rooms    = rooms;
    cs->n_rooms  = n_rooms;
    cs->room_idx = 0;
    cs->line_idx = 0;
    cs->active   = 0;
    cs->complete = 0;
    cs->resolve  = resolve;
    cs->box      = box;
    cs->closing.active = 0;   /* no box closing until the first speaker change */
    cs->pending_keep   = 0;
    if (rooms == NULL || resolve == NULL || box == NULL || n_rooms <= 0 ||
        rooms[0].script == NULL || rooms[0].n_lines <= 0)
        return;
    if (arm_current_line(cs, ARM_OPEN)) /* room-0 line-0: full slide-in entrance */
        cs->active = 1;
}

int cutscene_step(cutscene *cs, int confirm_pressed)
{
    if (cs == NULL || !cs->active)
        return 0;

    /* Apply a DEFERRED same-speaker re-text at the tick AFTER the advance press:
     * the old line rendered on the press tick (retail clears the body one flip
     * later), and re-texting here — BEFORE dialogue_step — lets the first char
     * reveal this same tick, so the new line's start tick is unchanged. */
    if (cs->pending_keep) {
        cs->pending_keep = 0;
        arm_current_line(cs, ARM_KEEP);
    }
    dialogue_step(cs->box);     /* pop-in / portrait fade / typewriter, one tick */
    /* The OLD box stays FULL (text shown) BEHIND the opening new box until the
     * NEW box has PASSED half-open (scale > 500), THEN it pops OUT (40/update).
     * retail's overlap: at a speaker change the new box opens in front while the
     * old box lingers full, closing the tick AFTER the new box hits 500 (main
     * 500 @t, old still full @t, old closes @t+1 when main=550) — drawcall-exact
     * vs retail.osr (engine-quirk #107). */
    if (cs->box->scale > 500)
        dialogue_close_step(&cs->closing);

    /* The dialogue-interaction input is the CONFIRM action — ENTER or X (the nav
     * injects it as ring id 0x24; Z has NO dialogue role, USER ckpt 132).  Retail's
     * ONE confirm key does BOTH, and they are MUTUALLY EXCLUSIVE per tick — the box
     * widget is in state 1 (typing) OR state 2 (waiting), an if/else-if at
     * 0x439690:978 vs :1004 — so a single press does exactly one of:
     *   - press while TYPING  -> SKIP: complete the reveal instantly.  Retail
     *     FUN_0043bca0's 0x24 poll calls FUN_0043ce50(9) -> FUN_0043ca40(9), which
     *     forces the text machine to fully-shown and returns 3; the beat-runner
     *     reads that to step the box state 1 -> 2.  The press is CONSUMED by the
     *     skip; it does NOT also advance this tick.
     *   - press while COMPLETE -> ADVANCE to the next line (the FUN_0043b980
     *     state-2 poll).
     * So a line takes ~2 confirms (skip, then advance) — the SAME press cadence as
     * retail, which keeps the port tick-aligned through the dialogue (the old
     * advance-only model waited out every typewriter and lagged retail -> desync,
     * the `dialogue-typewriter-skip` blocker, USER ckpt 132).  A confirm during the
     * pop-in (neither typing nor awaiting) is eaten with no effect, faithful to
     * FUN_0043ce50 returning 0 until scale==1000. */
    if (confirm_pressed) {
        if (dialogue_typing(cs->box)) {
            dialogue_skip_reveal(cs->box);  /* SKIP — completes the line, no advance */
        } else if (dialogue_awaiting_advance(cs->box)) {
            /* Remember the line just shown so the next-line arm can tell a
             * same-speaker page advance (keep the box) from a speaker change
             * (close + re-pop) — retail keeps the box on a same-speaker advance
             * (gap 1t) but closes it ~9t on a speaker change (THEME 1). */
            uint32_t prev_name = cs->rooms[cs->room_idx].script[cs->line_idx].name_va;
            int      prev_room = cs->room_idx;
            cs->line_idx++;
            if (cs->line_idx >= cs->rooms[cs->room_idx].n_lines) {
                /* End of this room — COMMIT the next room key (the 0x401d40
                 * stage / 0x402030 commit / 0x4d7d80 re-dispatch, modeled as
                 * advancing the chain) and arm its line 0. */
                cs->room_idx++;
                cs->line_idx = 0;
            }
            /* Keep the box open only for a same-speaker advance WITHIN a room; a
             * speaker change re-opens (fast, half-scale); a room boundary is a
             * full context change handled as a re-open too. */
            int same_speaker = cs->room_idx < cs->n_rooms &&
                               cs->room_idx == prev_room &&
                               cs->rooms[cs->room_idx].script[cs->line_idx].name_va
                                   == prev_name;
            if (same_speaker) {
                /* SAME speaker: keep the box open, but DEFER the re-text to the
                 * next tick — the old line still renders on this advance tick
                 * (retail clears the body one flip after the press).  Never
                 * completes (mid-room). */
                cs->pending_keep = 1;
            } else {
                /* SPEAKER CHANGE (or a room boundary / the chain end) — snapshot
                 * the just-shown box (still full) into the CLOSING box so it pops
                 * OUT (dialogue_close_step) in FRONT while the new box opens
                 * behind; retail overlaps the two during the ~9t transition
                 * (quirk #107).  This also keeps the old box on the advance tick. */
                if (cs->room_idx < cs->n_rooms && cs->box->active)
                    cs->closing = *cs->box;
                if (cs->room_idx >= cs->n_rooms ||
                    !arm_current_line(cs, ARM_REOPEN)) {
                    cs->active   = 0;
                    cs->complete = 1;
                    cs->box->active = 0;  /* close the box (the 0x49cd70 teardown) */
                    return 1;             /* chain complete → caller hands off ctrl */
                }
            }
        }
    }
    return 0;
}

int cutscene_active(const cutscene *cs)   { return cs != NULL && cs->active; }
int cutscene_complete(const cutscene *cs) { return cs != NULL && cs->complete; }

uint32_t cutscene_room_key(const cutscene *cs)
{
    if (cs == NULL || cs->rooms == NULL || cs->room_idx < 0 ||
        cs->room_idx >= cs->n_rooms)
        return 0;
    return cs->rooms[cs->room_idx].room_key;
}

const dialogue_box *cutscene_closing_box(const cutscene *cs)
{
    if (cs == NULL || !cs->closing.active)
        return NULL;
    return &cs->closing;
}
