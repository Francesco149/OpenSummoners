/*
 * cutscene.c — the town-intro multi-room dialogue chain (see cutscene.h).
 */
#include "cutscene.h"

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

/* The town-gate family conversation — 0x4d7d80 case 0x334be, the first-run
 * (flag 0x5f76805 == 0) path, decompile lines 33-292.  Ten 0x49d6e0 calls,
 * in order; each row = (speaker dramatist NAME VA, line TEXT VA, face, voice).
 * Text VAs are the 0x4d7d80 pcVar6 string labels; faces are uVar9, voices uVar5
 * (0x3eb..0x3f4, sequential — voice deferred).  All strings live in the user's
 * sotes.exe and are read at runtime by VA (never embedded). */
static const cutscene_line TOWN_ARRIVAL[] = {
    /*  # decompile  speaker  name         text        face  voice  gist */
    /*  1 :105 */ { NAME_FATHER, 0x86d58cu, 0x1e, 0x3eb }, /* "Ahh, here we are at last!…" */
    /*  2 :119 */ { NAME_ARCHE,  0x86d55cu, 0x02, 0x3ec }, /* "Yay, we're finally here!…"  */
    /*  3 :133 */ { NAME_MOTHER, 0x86d500u, 0x1e, 0x3ed }, /* "We haven't been here since…"*/
    /*  4 :147 */ { NAME_ARCHE,  0x86d4c8u, 0x03, 0x3ee }, /* "Yeah! There's people and…"  */
    /*  5 :161 */ { NAME_ARCHE,  0x86d47cu, 0x09, 0x3ef }, /* "Hey, Dad! Our shop is in…"  */
    /*  6 :175 */ { NAME_ARCHE,  0x86d45cu, 0x0d, 0x3f0 }, /* "I wanna see it! Where is it?"*/
    /*  7 :196 */ { NAME_FATHER, 0x86d42cu, 0x1e, 0x3f1 }, /* "Mm-hmm. It's just down the…" */
    /*  8 :210 */ { NAME_ARCHE,  0x86d424u, 0x03, 0x3f2 }, /* "Cool!"                       */
    /*  9 :248 */ { NAME_ARCHE,  0x86d410u, 0x03, 0x3f3 }, /* "Mom! Dad! c'mon!"            */
    /* 10 :262 */ { NAME_MOTHER, 0x86d3d4u, 0x1e, 0x3f4 }, /* "Hmhm. Well, wait up for…"    */
};
#define TOWN_ARRIVAL_COUNT ((int)(sizeof(TOWN_ARRIVAL) / sizeof(TOWN_ARRIVAL[0])))

/* The new-house interior — 0x4d7d80 case 0x334c8, the first-run (flag
 * 0x5f76805 == 0) path, decompile lines 1029-1218.  Eight 0x49d6e0 calls,
 * in order.  One non-dialogue beat (the actor emote 0x401e60(Arche,1) at
 * :1170, between lines 6 and 7) is SKIPPED — PORT-DEBT(cutscene-beat-runner).
 * Voices are all 0 (the house lines are unvoiced).  After line 8 the script
 * stages room 0x334dc (the errands/freeroam) via 0x401d40 and `return 2`. */
static const cutscene_line TOWN_HOUSE[] = {
    /*  # decompile   speaker  name         text        face  voice  gist */
    /*  1 :1093 */ { NAME_ARCHE,  0x86d390u, 0x0d, 0 }, /* "So this is our new house!…"  */
    /*  2 :1107 */ { NAME_ARCHE,  0x86d35cu, 0x0d, 0 }, /* "Hee, there's even an item…"  */
    /*  3 :1121 */ { NAME_MOTHER, 0x86d318u, 0x1e, 0 }, /* "Oh, this is lovely. …your…" */
    /*  4 :1135 */ { NAME_FATHER, 0x86d2d4u, 0x1e, 0 }, /* "Mm-hmm. I'm hoping I can…"   */
    /*  5 :1149 */ { NAME_FATHER, 0x86d294u, 0x1e, 0 }, /* "…helping the townsfolk out…"*/
    /*  6 :1163 */ { NAME_FATHER, 0x86d240u, 0x1e, 0 }, /* "…Well, Arche, I'll be count…"*/
    /*  7 :1184 */ { NAME_ARCHE,  0x86d22cu, 0x03, 0 }, /* "I will, I promise."          */
    /*  8 :1198 */ { NAME_MOTHER, 0x86d1dcu, 0x1e, 0 }, /* "And today, we need your help"*/
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
static int arm_current_line(cutscene *cs)
{
    const cutscene_room *room = &cs->rooms[cs->room_idx];
    const cutscene_line *ln   = &room->script[cs->line_idx];
    const char *text = cs->resolve(ln->text_va);
    if (text == NULL)
        return 0;
    dialogue_arm(cs->box, cs->resolve(ln->name_va), text);
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
    if (rooms == NULL || resolve == NULL || box == NULL || n_rooms <= 0 ||
        rooms[0].script == NULL || rooms[0].n_lines <= 0)
        return;
    if (arm_current_line(cs))   /* room-0 line-0 unresolved → stays disarmed */
        cs->active = 1;
}

int cutscene_step(cutscene *cs, int advance_pressed)
{
    if (cs == NULL || !cs->active)
        return 0;

    dialogue_step(cs->box);     /* pop-in / portrait fade / typewriter, one tick */

    /* DIALOGUE-beat completion (0x439690:1004): Z advances ONLY once the line
     * is fully typed (the "state 2" wait — the arrow shows).  Z while typing is
     * ignored, faithful to the beat-runner (the typewriter auto-advances on its
     * own cadence; there is no player skip). */
    if (advance_pressed && dialogue_awaiting_advance(cs->box)) {
        cs->line_idx++;
        if (cs->line_idx >= cs->rooms[cs->room_idx].n_lines) {
            /* End of this room — COMMIT the next room key (the 0x401d40
             * stage / 0x402030 commit / 0x4d7d80 re-dispatch, modeled as
             * advancing the chain) and arm its line 0. */
            cs->room_idx++;
            cs->line_idx = 0;
        }
        if (cs->room_idx >= cs->n_rooms || !arm_current_line(cs)) {
            cs->active   = 0;
            cs->complete = 1;
            cs->box->active = 0;    /* close the box (the 0x49cd70 teardown) */
            return 1;               /* chain complete → caller hands off control */
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
