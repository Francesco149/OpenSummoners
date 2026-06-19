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

/* ── THEME 3, the L7→L8 inter-line beat ("Arche runs ahead to the house").
 * RE'd from 0x4d7d80:215-235 (the script) + the beat-runner 0x439690 (the gates),
 * traced exactly — NOT curve-fit (engine-quirk #109):
 *   1. set the CAMERA command: in_ECX[0x13]=1, tgt (in_ECX[0x14]/[0x15])=
 *      (28000,12800), cap (in_ECX[0x16])=400, beat type in_ECX[8]=3.
 *   2. 0x402730(Arche, +32000): arms Arche's MOVE beat (run to body+32000 = world
 *      73104 = L8's baked spk_wx) AND OVERWRITES in_ECX[8]=4 (0x402730:54).
 *   3. 0x439680 → case 4 (NOT case 3 — the camera is not waited on): 0x439690:1137
 *      loops the 32-slot actor-beat pool, DONE only when no slot is in active-move
 *      substate (+0x300==1) — i.e. it WAITS FOR THE ARCHE RUN-OFF to finish.
 *   4. WAIT timer 0x32=50 (in_ECX[8]=6, the +0x57c timer), then L8.
 *
 * So the CAMERA is a FIRE-AND-FORGET command: 0x439690:623-641 sets the view target
 * then CLEARS in_ECX[0x13] (one-shot), and the 0x43d1d0 easer pans CONCURRENTLY.
 * The port issues it via camera_apply_pan(28000,12800,400) — the real ported easer
 * (verified off the draw stream: a town tile's dst.x slid -148px over ~982→1031 =
 * 12800→~28000 at -4px/tick cruise, settling ~53t, then held).
 *
 * The GATE is the case-4 RUN-OFF wait — Arche running 32000 units via the actor MOVE
 * stepper (0x46cd70 -> 0x54f980).  That stepper is the cast (PORT-DEBT(cutscene-party-
 * chars), unported), so its duration is a clearly-tagged cast-debt STAND-IN (the same
 * cast the baked run TARGET spk_wx=73104 stands in for), NOT a curve-fit rate.  The
 * retail L7adv→L8-first-glyph gap is MEASURED 167t = run-off + wait50 + the box
 * POP-IN (~20t — L8 opens fresh like L0, and the port reproduces L0-L7's pop-in 1:1,
 * THEME 1).  So the run-off = 167 − 50 − 20 = 97t; the box pop-in is added by
 * dialogue_step AFTER the beats.  Modeled as one beat: CAMERA_PAN issues the concurrent
 * pan command + carries the case-4 run-off wait as its `dur`; then WAIT 50, then L8. */
#define ARRIVAL_L8_PAN_X    28000
#define ARRIVAL_L8_PAN_Y    12800
#define ARRIVAL_L8_PAN_SPD  400
#define ARRIVAL_L8_RUNOFF  108     /* case-4 actor-wait: the Arche run-off (0x54f980)   *
                                    * completion, a cast-debt stand-in.  The lead now    *
                                    * fires on "Cool!"'s beat-COMPLETION (~tick 972, ~10t*
                                    * before its box clears, see ARRIVAL_RUNOFF_BOX_HOLD),*
                                    * so this absorbed +11 vs the old 97 to keep L8 first-*
                                    * glyph at ~1150 (= 972 confirm + 108 + wait50 + ~20  *
                                    * pop)                                               */
#define ARRIVAL_L8_WAIT     50     /* in_ECX[0x15f]=0x32, the script timer (ported)     */
/* The CONCURRENT box hold for the run-off (note #5): retail issues the camera pan +
 * the 0x402730 run-off the instant "Cool!"'s dialogue beat returns.  The camera onset
 * PINS that instant: the easer accelerates from rest (0x43d1d0 v+=10/tick → 5 ticks to
 * the first pixel) and retail.osr's static caravan (res 1004) moves its first pixel at
 * tick 977 — so retail FIRED at ~972, while "Cool!"'s body still holds through tick 982
 * (= its own full+24 box auto-hold).  So the lead beats fire on the advance (tick 972),
 * but "Cool!"'s box stays UP showing its full text for this many MORE ticks (the pan +
 * the run play BEHIND it), then cuts.  11 = the box renders ticks 972..982/983 (its
 * retail body span; the port carries a ~1t coalescing residual). */
#define ARRIVAL_RUNOFF_BOX_HOLD 11
static const cutscene_beat ARRIVAL_L8_LEAD[] = {
    /* type             fade_mode fade_var pan_x  pan_y  param  dur(=case-4 run-off wait) */
    { CS_BEAT_CAMERA_PAN, 0, 0, ARRIVAL_L8_PAN_X, ARRIVAL_L8_PAN_Y,
      ARRIVAL_L8_PAN_SPD, ARRIVAL_L8_RUNOFF },
    { CS_BEAT_WAIT,       0, 0, 0, 0, 0, ARRIVAL_L8_WAIT },
};
#define ARRIVAL_L8_LEAD_N ((int)(sizeof(ARRIVAL_L8_LEAD)/sizeof(ARRIVAL_L8_LEAD[0])))

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
    /*  9 :248 */ { NAME_ARCHE,  0x86d410u, 0x03, 0x3f3, VA, 73104, 45600 }, /* "Mom! Dad! c'mon!"
                   * runs ahead — the L7→L8 camera-pan + run-gated gap (ARRIVAL_LEADS) leads it */
    /* 10 :262 */ { NAME_MOTHER, 0x86d3d4u, 0x1e, 0x3f4, VB, 38400, 43600 }, /* "Hmhm. Well, wait up for…"    */
};
#define TOWN_ARRIVAL_COUNT ((int)(sizeof(TOWN_ARRIVAL) / sizeof(TOWN_ARRIVAL[0])))

/* The arrival room's inter-line beats (THEME 3): L8 ("Mom! Dad! c'mon!") is
 * preceded by the camera pan + run-gated gap (ARRIVAL_L8_LEAD). */
static const cutscene_line_lead ARRIVAL_LEADS[] = {
    { /*line_idx=*/8, ARRIVAL_L8_LEAD, ARRIVAL_L8_LEAD_N, ARRIVAL_RUNOFF_BOX_HOLD },
};
#define ARRIVAL_LEADS_N ((int)(sizeof(ARRIVAL_LEADS) / sizeof(ARRIVAL_LEADS[0])))

/* ── THEME 3, the arrival→house ROOM-TRANSITION fades (notes #6/#7).  RE'd from
 * the scene_fade ARM site 0x439690:555-563 (engine-quirk #109): the fade beat
 * (in_ECX[8]=2) arms the iris grid (*(0x8a9b50)+0x1040) with MODE = in_ECX[10]
 * (1 = reveal/from-black = SCENE_FADE_MODE_OUT, 2 = cover/to-black = _IN), SPEED =
 * in_ECX[0xb] = 1000, and a VARIANT drawn from the LCG (FUN_005bf505, (rand*3)>>15
 * in {0,1,2}) — the SAME arm the establishing town reveal already uses (validated).
 * The caller (main.c) draws the RNG variant + arms the live grid on CS_ACT_FADE; the
 * FADE beat then waits for the grid to SETTLE (cutscene_set_fade_active = retail's
 * case-2 done gate).  These mode values mirror scene_fade.h, carried opaquely. */
#define CS_FADE_REVEAL 1   /* SCENE_FADE_MODE_OUT — fade FROM black (house entry, #7) */
#define CS_FADE_COVER  2   /* SCENE_FADE_MODE_IN  — fade TO black   (arrival exit, #6) */
#define CS_FADE_SPEED  1000               /* in_ECX[0xb] (ported scene_fade speed)    */
#define CS_FADE_CAP    120                /* a generous safety cap (the grid settles  *
                                           * in ~15-30t; the real gate is the grid)   */

/* The arrival room's EXIT beats (0x4d7d80:267-291): after L9 ("Hmhm! …") advances,
 * a WAIT (in_ECX[0x15f]=0x14) then a fade-TO-black (COVER, fade_var 0 = CENTER-OUT,
 * the LCG stand-in, see main.c) BEFORE the house room key is staged (the 0x401d40
 * stage / 0x402030 commit happen under full black, no early snap).
 *
 * The COVER overlaps the box CLOSE: on L9's advance the box snapshots into `closing`
 * and shrinks out (~21t, rendered IN FRONT of the fade — render order puts the
 * dialogue after scene_fade), and retail keeps that closing box over the early cover
 * (draw_probe: BOX>fade through retail.osr tick 1245, the box gone ~1246 while the
 * cover runs to full black ~1259).
 *
 * The pre-cover WAIT is 10 sim-ticks, NOT the script's literal 0x14=20: MEASURED off
 * retail.osr the cover arms ~10t after L9 advances (the box's name tab — its dialogue-
 * mode marker — vanishes tick 1224/1225 = the advance; the cover's first center alpha
 * is tick 1234).  The script's 0x14 wait-timer (the +0x57c beat decremented 1/beat-
 * runner call, 0x439690:1083) thus elapses in ~10 sim-ticks in this exit context — the
 * beat-runner evidently pumps the exit wait ~2×/sim-tick (a flip-vs-tick rate detail,
 * quirk #99 family; not yet fully pinned, calibrated from the draw stream).  At 20 the
 * port armed the cover at advance+20, after the closing box had already shrunk away
 * (no box over the cover) AND landed house L0 ~+8t late (the chain shifted; L1+ stay
 * matched, they are nav-anchored) — both the ckpt-137 residual.  The reveal/house
 * WAIT 0x32=50 maps 1:1 (50t, verified: house L1/L2/L3 tick-exact), so only the exit
 * wait is the 2× anomaly. */
#define ARRIVAL_EXIT_WAIT 10     /* 0x14 elapses ~10 sim-ticks here (retail.osr) */
static const cutscene_beat ARRIVAL_EXIT[] = {
    /* type           fade_mode      fade_var pan_x pan_y param        dur */
    { CS_BEAT_WAIT,   0,             0, 0, 0, 0,            ARRIVAL_EXIT_WAIT },
    { CS_BEAT_FADE,   CS_FADE_COVER, 0, 0, 0, CS_FADE_SPEED, CS_FADE_CAP },
};
#define ARRIVAL_EXIT_N ((int)(sizeof(ARRIVAL_EXIT) / sizeof(ARRIVAL_EXIT[0])))

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

/* The house room's ENTRY beats (0x4d7d80:1056-1080): the house case re-dispatches
 * (after the arrival staged its key + the room reloaded UNDER the exit fade-to-black)
 * and OPENS with a fade-FROM-black (reveal — the SAME arm as the establishing town
 * reveal, in_ECX[10]=1) then a WAIT 0x32=50, before house L0.  Modeled as line 0's
 * lead beats.  MEASURED off retail.osr: the reveal recedes the black ~1261→1291, the
 * room is house from there. */
static const cutscene_beat HOUSE_L0_LEAD[] = {
    /* type           fade_mode       fade_var pan_x pan_y param         dur */
    { CS_BEAT_FADE,   CS_FADE_REVEAL, 0, 0, 0, CS_FADE_SPEED, CS_FADE_CAP },
    { CS_BEAT_WAIT,   0,              0, 0, 0, 0,             50 },
};
#define HOUSE_L0_LEAD_N ((int)(sizeof(HOUSE_L0_LEAD) / sizeof(HOUSE_L0_LEAD[0])))

/* The house room's lead-beat map (THEME 3): line 0 opens with the fade-from-black. */
static const cutscene_line_lead HOUSE_LEADS[] = {
    { /*line_idx=*/0, HOUSE_L0_LEAD, HOUSE_L0_LEAD_N, /*box_hold=*/0 },
};
#define HOUSE_LEADS_N ((int)(sizeof(HOUSE_LEADS) / sizeof(HOUSE_LEADS[0])))

/* The house room's EXIT beats (0x4d7d80:1203-1220): after line 8 ("And today, we
 * need your help…") advances, a fade-TO-black (COVER) — then the errands room key
 * 0x334dc is staged (0x401d40) UNDER full black and `return 2`.  UNLIKE the arrival
 * exit, the house exit has NO preceding WAIT: the script arms the cover immediately
 * after line 8's pump (decompile :1199→:1203, no in_ECX[0x15f]/[8]=6 wait beat).
 *
 * The COVER VARIANT is EDGES-IN (variant 1): retail.osr (seed-pinned) shows the
 * black growing from the TOP+BOTTOM edges toward the middle — the family silhouetted
 * in the shrinking middle band (full-frame dump ticks 1678-1690, scene_fade.c
 * sf_pattern_edges_in).  This is a DIFFERENT iris than the arrival→house cover's
 * center-out (var 0): each fade arm draws its OWN LCG variant ((rand*3)>>15) and the
 * two arms' pinned draws differ.  Forced as the cast-debt STAND-IN
 * (PORT-DEBT(cutscene-fade-variant)) — the live LCG drifts (the unported cast
 * consumes it between arms), so the retail-observed variant is forced until the cast
 * lands; the CS_ACT_FADE handler still draws one rng_rand() per arm (count-faithful).
 * The errands ENTRY reveal (the matching fade-FROM-black, center-out var 0) is armed
 * by main.c on chain-complete — the errands script (0x4dc510) that owns it is not
 * yet ported (PORT-DEBT(cutscene-scene-chain)). */
#define HOUSE_EXIT_COVER_VAR 1   /* edges-in (retail.osr full-frame dump)            */
/* The LAST house line's box lingers showing its full text OVER the cover for this
 * many sim-ticks before closing: retail confirms L7 at ~tick 1670 (which fires the
 * cover), but L7's glyphs don't clear until ~1678 — the box stays up over the
 * darkening scene ~8t (measured off retail.osr's brightness curve + the box-glyph
 * timeline; the nav advances L17 at the confirm 1670, NOT the glyph-clear 1678). */
#define HOUSE_EXIT_BOX_HOLD  8
static const cutscene_beat HOUSE_EXIT[] = {
    /* type           fade_mode      fade_var              pan_x pan_y param          dur */
    { CS_BEAT_FADE,   CS_FADE_COVER, HOUSE_EXIT_COVER_VAR, 0, 0, CS_FADE_SPEED, CS_FADE_CAP },
};
#define HOUSE_EXIT_N ((int)(sizeof(HOUSE_EXIT) / sizeof(HOUSE_EXIT[0])))

/* The room chain: arrival → house.  Each room carries its committed key (the
 * 0x402030 +0x4024 value) so the caller can drive the backdrop + detect the
 * errands boundary.  The chain ends after the house (its retail `return 2`
 * stages 0x334dc, the errands room = the freeroam hand-off).  THEME 3: the arrival
 * EXITs with a wait + fade-to-black (so the room key stages under black), and the
 * house line 0 ENTERs with a fade-from-black reveal.  THEME B (the house→errands
 * transition): the house also EXITs with a fade-to-black COVER (HOUSE_EXIT, edges-in
 * var 1) before the errands key stages; main.c arms the matching errands-entry REVEAL
 * (center-out var 0) on chain-complete. */
static const cutscene_room TOWN_CHAIN[] = {
    /* room_key            script        n_lines            leads          n_leads          exit          n_exit        exit_box_hold */
    { CUTSCENE_ROOM_ARRIVAL, TOWN_ARRIVAL, TOWN_ARRIVAL_COUNT, ARRIVAL_LEADS, ARRIVAL_LEADS_N, ARRIVAL_EXIT, ARRIVAL_EXIT_N, 0 },
    { CUTSCENE_ROOM_HOUSE,   TOWN_HOUSE,   TOWN_HOUSE_COUNT,   HOUSE_LEADS,   HOUSE_LEADS_N,   HOUSE_EXIT,   HOUSE_EXIT_N,   HOUSE_EXIT_BOX_HOLD },
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
    else if (mode == ARM_REOPEN)
        dialogue_reopen(cs->box, cs->resolve(ln->name_va), text); /* speaker change *
                                * (the box is HIDDEN during the re-pop latency, so   *
                                * REOPEN does not gate on cs->box->active — quirk     *
                                * #108; dialogue_reopen re-arms from the spawn scale) */
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

/* ── THEME 3: the non-dialogue BEAT phase (camera pan / wait / fade between or
 * after dialogue lines — the interleaved 0x4d7d80 ops the beat-runner pumps).
 * While a beat list runs the dialogue/confirm path is short-circuited and the
 * main box is closed; the OLD box snapshot in `closing` shrinks out.  Each beat
 * issues a one-shot action (the caller performs it) then holds `dur` sim-ticks. */

/* Look up line `line_idx`'s LEAD in a room's sparse leads map (the full struct, so
 * the caller can read box_hold), or NULL if the line has none. */
static const cutscene_line_lead *cs_find_lead(const cutscene_room *room, int line_idx)
{
    for (int i = 0; i < room->n_leads; i++)
        if (room->leads[i].line_idx == line_idx)
            return &room->leads[i];
    return NULL;
}

/* Look up line `line_idx`'s LEAD beats; returns the beat list (and sets *n_out) or
 * NULL if the line has none. */
static const cutscene_beat *cs_line_lead(const cutscene_room *room, int line_idx,
                                         int *n_out)
{
    const cutscene_line_lead *l = cs_find_lead(room, line_idx);
    if (l != NULL) {
        if (n_out != NULL) *n_out = l->n_beats;
        return l->beats;
    }
    if (n_out != NULL) *n_out = 0;
    return NULL;
}

/* Make beat[beat_idx] current: issue its one-shot action + set its hold timer. */
static void cs_arm_beat(cutscene *cs)
{
    const cutscene_beat *b = &cs->beats[cs->beat_idx];
    cs->beat_timer = b->dur;
    if (b->type == CS_BEAT_CAMERA_PAN) {
        cs->action.kind = CS_ACT_CAMERA_PAN;
        cs->action.a = b->pan_x;
        cs->action.b = b->pan_y;
        cs->action.c = b->param;
    } else if (b->type == CS_BEAT_FADE) {
        cs->action.kind = CS_ACT_FADE;
        cs->action.a = b->fade_mode;
        cs->action.b = b->fade_var;
        cs->action.c = b->param;
        cs->fade_seen = 0;   /* not yet observed the grid active (1-tick arm latency) */
    }
    /* CS_BEAT_WAIT issues no action — it is a pure hold (the +0x57c timer). */
}

/* Snapshot the just-shown box into the CLOSING box so it shrinks out during the
 * gap, and hide the main box (no dialogue runs while the beats play). */
static void cs_close_box_into_closing(cutscene *cs)
{
    if (cs->box != NULL && cs->box->active) {
        cs->closing = *cs->box;
        cs->box->active = 0;
    }
}

/* Begin running a beat list (a line's lead beats or a room's exit beats). */
static void cs_begin_beats(cutscene *cs, const cutscene_beat *beats, int n,
                           int exit_beats)
{
    cs->in_beats   = 1;
    cs->beats_exit = exit_beats;
    cs->beats      = beats;
    cs->n_beats    = n;
    cs->beat_idx   = 0;
    if (n > 0)
        cs_arm_beat(cs);          /* issue beat 0 now (this same tick) */
}

/* Enter the current (room_idx, line_idx) line: if it carries LEAD beats, run
 * them first; otherwise arm it with a fresh open (the box was closed). */
static int cs_enter_line(cutscene *cs)
{
    int nlead = 0;
    const cutscene_beat *lead =
        cs_line_lead(&cs->rooms[cs->room_idx], cs->line_idx, &nlead);
    if (lead != NULL && nlead > 0) {
        cs_begin_beats(cs, lead, nlead, /*exit=*/0);
        return 0;
    }
    arm_current_line(cs, ARM_OPEN);
    return 0;
}

/* A beat list ran out: arm the pending line (lead) or stage the next room (exit).
 * Returns 1 if the chain completed (ran past the last room). */
static int cs_finish_beats(cutscene *cs)
{
    cs->in_beats = 0;
    if (cs->beats_exit) {
        /* room EXIT beats done → COMMIT the next room key (the caller reloads the
         * backdrop off cutscene_room_key) and enter its line 0. */
        cs->room_idx++;
        cs->line_idx = 0;
        if (cs->room_idx >= cs->n_rooms ||
            cs->resolve(cs->rooms[cs->room_idx].script[0].text_va) == NULL) {
            cs->active   = 0;
            cs->complete = 1;
            if (cs->box != NULL) cs->box->active = 0;
            return 1;
        }
        return cs_enter_line(cs);
    }
    /* line LEAD beats done → open the pending line fresh (it was closed during
     * the gap, so a full slide-in like the conversation's first line). */
    arm_current_line(cs, ARM_OPEN);
    return 0;
}

/* One sim-tick of the beat phase: shrink the closing box, advance the current beat,
 * and move to the next (or finish the list) when it completes.  A FADE beat
 * completes when the scene_fade grid SETTLES (retail's case-2 gate); a WAIT /
 * CAMERA_PAN beat completes when its hold timer expires. */
static int cs_step_beats(cutscene *cs)
{
    /* The CONCURRENT run-off lead (note #5): "Cool!"'s box stays UP showing its full
     * text (re-projected through the live camera each frame, so it rides the pan) while
     * the camera + Arche's run play BEHIND it, then closes when its body auto-hold
     * (full+24, tick ~982) elapses — retail issues the run-off on the beat completion
     * ~10t before that (the camera onset, tick 977, pins it).  When the hold expires the
     * box hands to the normal close (snapshot -> shrink), so a box is present through the
     * close exactly as for any line.
     * PORT-DEBT(dialogue-runoff-box-slide): retail's "Cool!" close is a SLIDE-OUT — the
     * body clears at 982 and the EMPTY frame (bubble tail res 0x456) slides right ~7px/
     * tick off-screen over ticks 983-1003 (gone 1004), NOT the shrink-in-place the port
     * does (with the text still on it).  Matching that slide is a box-close-animation
     * follow-up; the camera/run timing (this chip) is independent of it. */
    if (cs->box_linger > 0) {
        cs->box_linger--;
        if (cs->box_linger == 0)
            cs_close_box_into_closing(cs);   /* hand to the normal close (shrink-out) */
    }
    if (cs->closing.active)
        dialogue_close_step(&cs->closing);   /* the OLD box shrinks out */

    const cutscene_beat *b =
        (cs->beat_idx < cs->n_beats) ? &cs->beats[cs->beat_idx] : NULL;
    int done = 0;

    if (b != NULL && b->type == CS_BEAT_FADE) {
        /* wait for the grid the caller drives: once it has gone active and then
         * settled, the beat is done (retail's grid `done` flag clears the +0x24
         * trigger that case 2 polls).  `dur` is a SAFETY cap (a headless run with
         * no fade performer would otherwise stall). */
        if (cs->fade_active)
            cs->fade_seen = 1;
        if (cs->fade_seen && !cs->fade_active)
            done = 1;
        if (cs->beat_timer > 0 && --cs->beat_timer == 0)
            done = 1;                        /* safety cap */
    } else {
        if (cs->beat_timer > 0)
            cs->beat_timer--;
        if (cs->beat_timer <= 0)
            done = 1;
    }

    if (!done)
        return 0;
    cs->beat_idx++;
    if (cs->beat_idx < cs->n_beats) {
        cs_arm_beat(cs);
        return 0;
    }
    return cs_finish_beats(cs);
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
    cs->reopen_delay   = 0;
    cs->in_beats       = 0;   /* no non-dialogue beat list running yet         */
    cs->beats_exit     = 0;
    cs->beats          = NULL;
    cs->n_beats        = 0;
    cs->beat_idx       = 0;
    cs->beat_timer     = 0;
    cs->box_linger     = 0;
    cs->action.kind    = CS_ACT_NONE;
    cs->fade_active    = 0;
    cs->fade_seen      = 0;
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

    /* THEME 3: while a non-dialogue BEAT list is running (a camera pan / wait /
     * fade between or after lines), drive it instead of the dialogue path — the
     * beats are AUTOMATIC, so the confirm is ignored (retail's beat-runner eats
     * presses during a non-dialogue beat).  The main box is closed; the closing
     * snapshot shrinks out. */
    if (cs->in_beats)
        return cs_step_beats(cs);

    /* Apply a DEFERRED same-speaker re-text at the tick AFTER the advance press:
     * the old line rendered on the press tick (retail clears the body one flip
     * later), and re-texting here — BEFORE dialogue_step — lets the first char
     * reveal this same tick, so the new line's start tick is unchanged. */
    if (cs->pending_keep) {
        cs->pending_keep = 0;
        arm_current_line(cs, ARM_KEEP);
    }
    dialogue_step(cs->box);     /* pop-in / portrait fade / typewriter (no-op while *
                                 * the main box is hidden during the re-pop latency)*/

    /* Re-pop the new box AFTER the speaker-change latency (engine-quirk #108):
     * retail processes the advance ~2 ticks before the new box opens.  The press
     * armed the OLD box's portrait dissolve + hid the main box; here, once the
     * countdown expires, ARM_REOPEN spawns the new box.  ARM_REOPEN runs AFTER
     * dialogue_step so the box's FIRST render is the spawn scale (like an
     * immediate reopen), landing the box-frame growth at advance_tick−6 (the
     * ckpt-134 28/28 overlap match) two ticks behind the dissolve start. */
    if (cs->reopen_delay > 0 && --cs->reopen_delay == 0)
        arm_current_line(cs, ARM_REOPEN);

    /* The CLOSING (old) box: dissolve its portrait out via the reverse ramp EVERY
     * tick from the speaker change (idx 18→2, then gone), and pop the FRAME out
     * (40/update) once the new box has PASSED half-open (scale > 500).  The
     * portrait fade LEADS the frame close by ~2 ticks (the latency above), so by
     * the time the frame starts shrinking the bust has dissolved to gone —
     * drawcall+LUT-exact vs retail.osr (engine-quirk #108; the #107 overlap is
     * unchanged: the old box lingers full behind the opening new box). */
    if (cs->closing.active) {
        dialogue_fadeout_step(&cs->closing);
        if (cs->box->active && cs->box->scale > 500)
            dialogue_close_step(&cs->closing);
    }

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
            const cutscene_room *room = &cs->rooms[cs->room_idx];
            int last_in_room = (cs->line_idx + 1 >= room->n_lines);

            /* THEME 3: a room with EXIT beats (the arrival's wait + fade-to-black
             * before the house key is staged, note #6) runs them on the LAST
             * line's advance — close the box and hand to the beat phase;
             * cs_finish_beats commits the next room key when they complete. */
            if (last_in_room && room->exit_beats != NULL && room->n_exit > 0) {
                /* THEME B (house exit): retail keeps the LAST line's box UP showing
                 * its full text OVER the darkening cover for ~8t (the confirm fires
                 * the cover at ~1670, but the box's glyphs don't clear until ~1678 —
                 * the box lingers over the cover, then closes), so LINGER it like the
                 * run-off box_hold instead of closing immediately.  cs_step_beats
                 * decrements box_linger and closes the box when it expires. */
                if (room->exit_box_hold > 0)
                    cs->box_linger = room->exit_box_hold;
                else
                    cs_close_box_into_closing(cs);
                cs_begin_beats(cs, room->exit_beats, room->n_exit, /*exit=*/1);
                return 0;
            }

            cs->line_idx++;
            if (cs->line_idx >= room->n_lines) {
                /* End of this room — COMMIT the next room key (the 0x401d40
                 * stage / 0x402030 commit / 0x4d7d80 re-dispatch, modeled as
                 * advancing the chain) and arm its line 0. */
                cs->room_idx++;
                cs->line_idx = 0;
            }

            /* THEME 3: a non-dialogue GAP before the NEXT line (the L7→L8 "Arche
             * runs ahead" camera pan + wait, note #5) — run the lead beats, then
             * OPEN the next line fresh.  Checked before the same-speaker keep/reopen
             * because the gap interrupts the dialogue regardless of who speaks next.
             * A CONCURRENT lead (box_hold > 0, the run-off) keeps the just-shown box
             * UP showing its full text while the camera pan + Arche's run play behind
             * it — retail fires the run-off on this advance (the beat completion) but
             * the box holds ~7t more (its full+24 auto-hold) then cuts.  A normal lead
             * (box_hold 0) closes the box immediately (snapshot -> shrink). */
            if (cs->room_idx < cs->n_rooms) {
                const cutscene_line_lead *l =
                    cs_find_lead(&cs->rooms[cs->room_idx], cs->line_idx);
                if (l != NULL && l->n_beats > 0) {
                    if (l->box_hold > 0)
                        cs->box_linger = l->box_hold;   /* keep the box up (run-off) */
                    else
                        cs_close_box_into_closing(cs);  /* normal: snapshot + shrink  */
                    cs_begin_beats(cs, l->beats, l->n_beats, /*exit=*/0);
                    return 0;
                }
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
                /* SPEAKER CHANGE (or a room boundary / the chain end).  The chain
                 * end (ran off the last room, or the next line's text VA does not
                 * resolve) closes the box immediately and hands off control. */
                if (cs->room_idx >= cs->n_rooms ||
                    cs->resolve(cs->rooms[cs->room_idx]
                                    .script[cs->line_idx].text_va) == NULL) {
                    cs->active   = 0;
                    cs->complete = 1;
                    cs->box->active = 0;  /* close the box (the 0x49cd70 teardown) */
                    return 1;             /* chain complete → caller hands off ctrl */
                }
                /* Snapshot the just-shown box (still full) into the CLOSING box and
                 * arm its reverse-ramp portrait DISSOLVE (idx 18→2, engine-quirk
                 * #108) — it pops OUT in FRONT while the new box opens behind
                 * (quirk #107 overlap).  HIDE the main box and DELAY its re-pop
                 * CUTSCENE_REOPEN_DELAY ticks: retail processes the advance ~2t
                 * before the new box opens, so the dissolve leads the box-frame by
                 * 2 while the box-frame transition stays at advance_tick−6 (the
                 * 28/28 match).  The reopen latency fires ARM_REOPEN above. */
                if (cs->box->active)
                    cs->closing = *cs->box;
                dialogue_arm_fadeout(&cs->closing);
                cs->box->active  = 0;                     /* hidden during the latency */
                cs->reopen_delay = CUTSCENE_REOPEN_DELAY;
            }
        }
    }
    return 0;
}

int cutscene_active(const cutscene *cs)   { return cs != NULL && cs->active; }
int cutscene_complete(const cutscene *cs) { return cs != NULL && cs->complete; }
int cutscene_in_beats(const cutscene *cs) { return cs != NULL && cs->in_beats; }

void cutscene_set_fade_active(cutscene *cs, int active)
{
    if (cs != NULL)
        cs->fade_active = active;
}

int cutscene_take_action(cutscene *cs, cutscene_action *out)
{
    if (cs == NULL || cs->action.kind == CS_ACT_NONE)
        return 0;
    if (out != NULL)
        *out = cs->action;
    cs->action.kind = CS_ACT_NONE;   /* one-shot: drained */
    return 1;
}

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
