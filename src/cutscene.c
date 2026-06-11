/*
 * cutscene.c — the town-arrival dialogue sequence (see cutscene.h).
 */
#include "cutscene.h"

#include <stddef.h>

/* The town-gate family conversation — 0x4d7d80 case 0x334be, the first-run
 * (flag 0x5f76805 == 0) path, decompile lines 33-292.  Ten FUN_0049d6e0 calls,
 * in order; each row = (speaker dramatist NAME VA, line TEXT VA, face, voice).
 *
 * Speaker name VAs are DAT_006b6ea8 dramatist rows (+0x08 inline name), indices
 * 0/4/5 (tools/dump_dramatist_table.py): Arche 0x6b6eb0, Father 0x6b6f80,
 * Mother 0x6b6fb4.  Text VAs are the 0x4d7d80 pcVar6 string labels; faces are
 * uVar9, voices uVar5 (0x3eb..0x3f4, sequential — voice deferred).  All strings
 * live in the user's sotes.exe and are read at runtime by VA (never embedded). */
#define NAME_ARCHE   0x6b6eb0u   /* "Arche"          */
#define NAME_FATHER  0x6b6f80u   /* "Arche's Father" */
#define NAME_MOTHER  0x6b6fb4u   /* "Arche's Mother" */

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

const cutscene_line *cutscene_town_arrival(int *n)
{
    if (n != NULL) *n = TOWN_ARRIVAL_COUNT;
    return TOWN_ARRIVAL;
}

/* Resolve a line's name+text VAs and arm the box.  Returns 1 on success; 0 if
 * the text VA does not resolve (the line stays unshown — the caller treats a
 * line-0 miss as "disarmed" and a later miss as "complete").  A NULL name is
 * tolerated (dialogue_arm copies nothing). */
static int arm_line(cutscene *cs, int idx)
{
    const cutscene_line *ln = &cs->script[idx];
    const char *text = cs->resolve(ln->text_va);
    if (text == NULL)
        return 0;
    dialogue_arm(cs->box, cs->resolve(ln->name_va), text);
    return 1;
}

void cutscene_arm(cutscene *cs, const cutscene_line *script, int n,
                  cutscene_str_resolver resolve, dialogue_box *box)
{
    if (cs == NULL) return;
    cs->script   = script;
    cs->n_lines  = n;
    cs->line_idx = 0;
    cs->active   = 0;
    cs->complete = 0;
    cs->resolve  = resolve;
    cs->box      = box;
    if (script == NULL || resolve == NULL || box == NULL || n <= 0)
        return;
    if (arm_line(cs, 0))     /* line-0 unresolved -> stays disarmed */
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
        if (cs->line_idx >= cs->n_lines || !arm_line(cs, cs->line_idx)) {
            cs->active   = 0;
            cs->complete = 1;
            cs->box->active = 0;    /* close the box (the 0x49cd70 teardown) */
            return 1;               /* completed → caller hands off control  */
        }
    }
    return 0;
}

int cutscene_active(const cutscene *cs)   { return cs != NULL && cs->active; }
int cutscene_complete(const cutscene *cs) { return cs != NULL && cs->complete; }
