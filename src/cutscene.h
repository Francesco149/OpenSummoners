/*
 * cutscene.{c,h} — the town-intro CUTSCENE driver (the multi-room dialogue chain).
 *
 * The port-side beat sequence of 0x4d7d80 (the town-intro script) + the
 * beat-runner 0x439690.  Retail runs the script as a BLOCKING coroutine —
 * each beat calls 0x439680 (→ 0x439690) which pumps frames until the beat
 * completes, then falls through to the next.  The port's frame loop is
 * non-blocking, so this models the same sequence as a STATE MACHINE stepped once
 * per sim-tick: it drives the dialogue box (dialogue.{c,h}) through the lines,
 * advancing on the Z input (ring id 0x24 = the 0x43b980 advance-poll) exactly
 * when the current line is fully typed (dialogue_awaiting_advance — the
 * beat-runner's DIALOGUE-beat completion gate, 0x439690:1004).
 *
 * MULTI-ROOM CHAIN (ckpt 123): the town intro is a CHAIN of rooms, not one
 * scene.  0x4d7d80 dispatches on the committed room key (room_state+0x4024); a
 * room's closing beat STAGES the next room via 0x401d40 (writes the next key
 * to map+0x900/0x904/0x908) and 0x402030 COMMITS it (→ room_state+0x4024),
 * re-dispatching 0x4d7d80 on the new key (the case `return 2` path).  The
 * harness-verified chain (quirk #103, runs/control-path-gt):
 *     0x334be arrival (10 lines) → 0x334c8 house (8 lines) → 0x334dc errands.
 * It is a LIGHT key swap (one game_enter; entities/room_state persist), so the
 * port models it as advancing a ROOMS LIST — when a room's lines complete, the
 * sequencer commits the next room key and arms its line 0.  This chip chains
 * 0x334be → 0x334c8; the chain ends at the ERRANDS BOUNDARY (0x334dc), which is
 * the freeroam control hand-off point (the errands room IS the freeroam — the
 * caller starts character_step there; see plans/controllable-arche-faithful.md).
 *
 * SCOPE (this chip = "the dialogue chain"): the DIALOGUE lines + the room-key
 * swap (CONTROL FLOW).  Two pieces stay deferred:
 *   - PORT-DEBT(cutscene-beat-runner): the interleaved non-dialogue beats — the
 *     fades (type 2), camera pans (type 3), wait timers (type 6), actor
 *     emotes/run-offs (0x401e60 / 0x402730) — stay on the port's measured-frame
 *     triggers (camera pan / banner / letterbox / scene-fade, separately
 *     validated) and are NOT sequenced here yet.
 *   - PORT-DEBT(cutscene-room-render): the house (0x334c8) and errands (0x334dc)
 *     ROOM BACKDROPS are not rendered — the room key drives an unported
 *     map-load path (the 0x585ae0/0x586010 family, 14 consumers of +0x4024), so
 *     the house lines currently play over the town-arrival backdrop.  The
 *     errands questline (the separate dispatcher 0x4dc510) is a later arc.
 *
 * The line text + speaker names are STORY CONTENT in the user's sotes.exe — the
 * script table holds only their VAs; the strings are read at runtime via a
 * resolver the caller supplies (main.c: exe_data_string).  Never embedded in
 * source (the dramatist-table precedent + the project legal line).
 *
 * Pure (no Win32): host-tested with a stub resolver (tests/test_cutscene.c).
 */
#ifndef OPENSUMMONERS_CUTSCENE_H
#define OPENSUMMONERS_CUTSCENE_H

#include <stdint.h>
#include "dialogue.h"

/* The CONFIRM ring id the DIALOGUE beat polls via 0x43b980 — the same id the
 * title menu's confirm reads (input.h).  The physical key is ENTER or X (NOT Z —
 * Z has no dialogue role, USER ckpt 132; the key→ring map is the 0x46a880
 * producer).  The caller polls the input ring for it each sim-tick and passes
 * the hit to cutscene_step, which SKIPS or ADVANCES with it (see cutscene_step). */
#define CUTSCENE_ADVANCE_RING_ID 0x24

/* The speaker-change re-pop LATENCY (engine-quirk #108): retail processes a
 * speaker-change advance ~2 ticks BEFORE the new box opens — it arms the OLD
 * box's reverse-ramp portrait dissolve immediately, but the new box's re-pop has
 * a ~2-tick setup latency.  So the driver presses 2 ticks early (the matched nav
 * uses advance_tick−8, not −6) and DELAYS the new box's dialogue_reopen by this
 * many ticks, keeping the box-frame transition at advance_tick−6 (the ckpt-134
 * 28/28 overlap match) while the portrait fade-out leads it by 2. */
#define CUTSCENE_REOPEN_DELAY 2

/* The committed room keys (room_state+0x4024) the town-intro chain swaps through
 * (0x4d7d80 switch cases; staged by 0x401d40, committed by 0x402030). */
#define CUTSCENE_ROOM_ARRIVAL 0x334beu  /* the town-gate arrival (10 lines)        */
#define CUTSCENE_ROOM_HOUSE   0x334c8u  /* the new-house interior (8 lines)        */
#define CUTSCENE_ROOM_ERRANDS 0x334dcu  /* the errands room = the freeroam (gamepl.)*/

/* ── The interleaved NON-DIALOGUE beats (THEME 3, the arrival→house transition).
 * 0x4d7d80 is a flat sequence of ops the beat-runner 0x439690 pumps: dialogue
 * lines (0x49d6e0) AND non-dialogue beats — camera pans (in_ECX[8]=3), wait
 * timers (in_ECX[8]=6, +0x57c), and scene-transition fades (in_ECX[8]=2).  Each
 * non-dialogue beat runs to completion AUTOMATICALLY (no confirm), then the
 * script falls through to the next.  The port models them as a per-line LEAD
 * sequence (beats that run before a line shows, after the previous line's
 * advance) + a per-room EXIT sequence (beats after the last line, before the
 * room key is staged).  cutscene.c stays pure C: the beat issues a one-shot
 * cutscene_action the caller (main.c) performs against the live camera /
 * scene_fade (those modules are pure C too, but kept at the main.c seam).
 *
 * Faithfulness: the CAMERA pan is the real 0x43d1d0 easer (camera_apply_pan,
 * settles ~53t for 12800→28000 @cap400, MEASURED off retail.osr); the WAIT is the
 * +0x57c timer; the FADE is the scene_fade reveal grid.  The one cast-coupled
 * piece is the L7→L8 gap's TOTAL length: the gating beat is the Arche RUN-off
 * (0x402730, the actor mover in PORT-DEBT(cutscene-party-chars)), so the camera
 * beat's `dur` carries the run-gated completion (MEASURED 167t L7adv→L8start,
 * the same cast-debt family as the already-baked run TARGET spk_wx=73104). */
typedef enum {
    CS_BEAT_WAIT = 0,    /* hold `dur` sim-ticks (in_ECX[8]=6 timer +0x57c)        */
    CS_BEAT_CAMERA_PAN,  /* issue the FIRE-AND-FORGET camera command (camera_apply_*
                          * pan(pan_x,pan_y,param); 0x439690:623-641 sets the view  *
                          * tgt then clears the flag — the 0x43d1d0 easer pans       *
                          * CONCURRENTLY).  `dur` holds the SEPARATE wait the script  *
                          * pairs with the pan (for L7→L8: the case-4 run-off wait,   *
                          * 0x402730 — a cast-debt stand-in, see quirk #109)          */
    CS_BEAT_FADE,        /* issue scene_fade_arm(fade_mode,fade_var,param); hold   *
                          * `dur` ticks until the iris grid settles (in_ECX[8]=2)  */
} cutscene_beat_type;

typedef struct cutscene_beat {
    int16_t type;        /* cutscene_beat_type                                     */
    int16_t fade_mode;   /* FADE: scene_fade mode (SCENE_FADE_MODE_OUT / _IN)      */
    int16_t fade_var;    /* FADE: iris variant 0/1/2 (center-out/edges-in/sweep)   */
    int32_t pan_x, pan_y;/* CAMERA_PAN target scroll origin                        */
    int32_t param;       /* CAMERA_PAN speed (cap) / FADE speed                    */
    int32_t dur;         /* sim-ticks to HOLD after issuing the action            */
} cutscene_beat;

/* A one-shot action the caller performs on the cutscene's behalf (cutscene.c is
 * pure C; the camera + scene_fade live at the main.c seam).  Set when a beat
 * issues; the caller drains it with cutscene_take_action each tick. */
typedef enum {
    CS_ACT_NONE = 0,
    CS_ACT_CAMERA_PAN,   /* a=tgt_x  b=tgt_y  c=speed  -> camera_apply_pan         */
    CS_ACT_FADE,         /* a=mode   b=variant c=speed -> scene_fade_arm           */
} cutscene_action_kind;

typedef struct cutscene_action {
    int     kind;        /* cutscene_action_kind (CS_ACT_NONE when drained)        */
    int32_t a, b, c;     /* see the kind comments above                           */
} cutscene_action;

/* One scripted dialogue line — the args the script passes to 0x49d6e0
 * (the dialogue-line setup): the speaker's dramatist NAME and the line TEXT by
 * exe VA, the portrait FACE id, and the VOICE clip id.  name_or_0 in the real
 * call is 0, so the name resolves from the speaker actor's +0x750 (= the
 * dramatist-table name); the port resolves the same string by its VA directly. */
typedef struct cutscene_line {
    uint32_t name_va;   /* dramatist name string VA (DAT_006b6ea8 row +0x08)     */
    uint32_t text_va;   /* line text string VA (0x4d7d80 pcVar6 label)           */
    int16_t  face;      /* portrait face id (uVar9 → the face-table key)         */
    int16_t  voice;     /* voice clip id (uVar5; 1_%07d.wma — deferred)          */
    int16_t  pvar;      /* portrait face-table VARIANT (portrait_variant): the   */
                        /* speaker's body-facing at this line (0x49d6e0:143,     */
                        /* facing==3 → A else B), HARNESS-CAPTURED per line       */
                        /* (runs/portrait-gt) — the variants are different busts/ */
                        /* sizes, so this picks the 1:1 slot.  See portrait.h.    */
    int32_t  spk_wx;    /* the speaker's WORLD pos this line (0x49c640 anchors    */
    int32_t  spk_wy;    /* the box to its projection).  HARNESS-CAPTURED          */
                        /* (runs/box-pos-inputs); the cast moves (Arche runs      */
                        /* ahead at arrival L9), so it is per-line, not per-actor. */
} cutscene_line;

/* THEME 3: a per-room map "line N is preceded by these non-dialogue beats" —
 * the interleaved camera pan / wait / fade that runs after line N-1's advance,
 * before line N shows (e.g. arrival L8's "Arche runs ahead" camera-pan gap).
 * Kept SPARSE + separate from cutscene_line so the dialogue-content rows stay
 * focused on story content (text/speaker/portrait), not choreography. */
typedef struct cutscene_line_lead {
    int                  line_idx;  /* the line this beat list precedes (0-based) */
    const cutscene_beat *beats;
    int                  n_beats;
    /* The RUN-OFF box hold (note #5): for a CONCURRENT lead (the L7→L8 "Arche runs
     * ahead" gap), the preceding line's dialogue beat completes ~box_hold ticks
     * BEFORE the box visibly clears — retail issues the camera pan + the 0x402730
     * run-off the moment the beat returns (the camera onset PROVES it: retail.osr
     * the static caravan pans from tick 977, while "Cool!"'s body holds through
     * 982 = its own full+24 box auto-hold).  So the lead beats fire on the advance,
     * but the preceding line's box stays UP showing its full text for box_hold more
     * ticks (the camera pan + the run play BEHIND it), then cuts.  0 = the normal
     * close (the box goes to `closing` and shrinks out, e.g. the house entry fade,
     * which runs AFTER the room swap with the box already gone). */
    int                  box_hold;
} cutscene_line_lead;

/* A resolver VA → string (main.c supplies exe_data_string; tests stub it). */
typedef const char *(*cutscene_str_resolver)(uint32_t va);

/* One ROOM in the town-intro chain: a committed room key (the 0x402030 +0x4024
 * value) + its dialogue line table.  The sequencer plays the rooms in order,
 * advancing to the next when the current room's lines complete (the port's
 * model of the retail stage/commit/re-dispatch room swap). */
typedef struct cutscene_room {
    uint32_t                  room_key;  /* committed +0x4024 key (CUTSCENE_ROOM_*) */
    const cutscene_line      *script;    /* the room's line table                   */
    int                       n_lines;
    const cutscene_line_lead *leads;     /* SPARSE per-line lead-beat map (THEME 3:  *
                                          * the inter-line camera pan / wait / fade) */
    int                       n_leads;
    const cutscene_beat      *exit_beats;/* non-dialogue beats after the LAST line   *
                                          * advances, BEFORE the next room key is     *
                                          * staged (the arrival's wait + fade-to-     *
                                          * black before the house swap); NULL/0=none*/
    int                       n_exit;
} cutscene_room;

typedef struct cutscene {
    const cutscene_room  *rooms;     /* the room chain (arrival → house → …)       */
    int                   n_rooms;
    int                   room_idx;  /* current room (0-based)                     */
    int                   line_idx;  /* current line within the room (0-based)     */
    int                   active;    /* the chain is running                       */
    int                   complete;  /* the whole chain cleared (one-shot)         */
    cutscene_str_resolver resolve;
    dialogue_box         *box;       /* the box the caller owns + renders          */
    dialogue_box          closing;   /* the OLD box popping OUT during a speaker    *
                                      * change (rendered IN FRONT of `box` opening  *
                                      * behind it); active only mid-transition      */
    int                   pending_keep; /* a SAME-speaker advance defers its re-text *
                                      * to the NEXT tick's start, so the old line    *
                                      * still renders on the advance tick (retail    *
                                      * clears the body one flip after the press)    */
    int                   reopen_delay; /* a SPEAKER-CHANGE advance counts down this  *
                                      * many ticks (CUTSCENE_REOPEN_DELAY) before     *
                                      * the new box re-pops — the main box is hidden  *
                                      * meanwhile while the OLD box's portrait         *
                                      * dissolves out (engine-quirk #108)             */
    /* ── The non-dialogue BEAT sub-state (THEME 3).  While `in_beats`, cutscene_step
     * runs the active beat list (a line's `lead` or a room's `exit_beats`) once per
     * tick instead of the dialogue/confirm path; the main box is closed (the OLD box
     * snapshot in `closing` shrinks out).  When the list completes the sequencer arms
     * the pending line (lead) or advances the room (exit). */
    int                   in_beats;    /* a beat list is running                     */
    int                   beats_exit;  /* the list is a room's exit beats (vs lead)  */
    const cutscene_beat  *beats;       /* the active beat list                       */
    int                   n_beats;
    int                   beat_idx;     /* current beat within the list              */
    int                   beat_timer;   /* sim-ticks left on the current beat        */
    int                   box_linger;   /* a CONCURRENT lead (run-off): ticks the     *
                                         * preceding line's box stays UP (full text,   *
                                         * tracking the camera) after its advance while *
                                         * the lead beats play behind it, before it    *
                                         * cuts (cutscene_line_lead.box_hold; note #5) */
    cutscene_action       action;      /* pending one-shot for the caller to perform */
    /* A FADE beat waits for the scene_fade GRID to settle (retail's case-2 gate =
     * the grid `done` flag, 0x439690:1125), not a fixed timer.  The caller feeds the
     * live grid state each tick via cutscene_set_fade_active; the beat completes once
     * it has SEEN the grid active and then go inactive (`fade_seen` handles the 1-tick
     * arm latency — main.c arms the grid AFTER cutscene_step). */
    int                   fade_active; /* the live scene_fade grid is still painting   */
    int                   fade_seen;   /* the current FADE beat has observed it active  */
} cutscene;

/* The town-gate family conversation (0x4d7d80 case 0x334be, flag 0x5f76805==0,
 * lines 33-292): 10 lines, Father/Arche/Mother.  Returns the line table + *n. */
const cutscene_line *cutscene_town_arrival(int *n);

/* The new-house interior conversation (0x4d7d80 case 0x334c8, flag==0, lines
 * 1029-1218): 8 lines, Arche/Mother/Father.  Returns the line table + *n. */
const cutscene_line *cutscene_town_house(int *n);

/* The town-intro ROOM CHAIN: arrival (0x334be) → house (0x334c8).  The chain
 * completes at the errands boundary (0x334dc).  Returns the room table + *n. */
const cutscene_room *cutscene_town_chain(int *n_rooms);

/* Arm a cutscene: bind the room chain + resolver + box, reset to room 0 / line
 * 0, and arm the box with room-0 line-0 (resolving its name/text VAs).  Safe
 * no-op if any of rooms/resolve/box is NULL or n_rooms <= 0 (or the first line
 * does not resolve). */
void cutscene_arm(cutscene *cs, const cutscene_room *rooms, int n_rooms,
                  cutscene_str_resolver resolve, dialogue_box *box);

/* One sim-tick: step the box (pop-in / fade / typewriter), then — if the CONFIRM
 * input fired this tick — do exactly ONE of (mutually exclusive, 0x439690 state
 * 1 vs 2): SKIP the typewriter if the line is still revealing (dialogue_typing →
 * complete the reveal, no advance), else ADVANCE if the line is fully typed
 * (dialogue_awaiting_advance) — at a room's last line COMMIT the next room key
 * (advance room_idx) and arm its line 0; past the last room, complete
 * (deactivate the box, set complete=1).  So a line takes ~2 confirms (skip, then
 * advance), matching retail's press cadence (keeps the port tick-aligned).
 * `confirm_pressed` = a confirm (ring id 0x24, ENTER/X) was consumed from the
 * ring this tick.  Returns 1 on the tick the chain COMPLETES (the caller fires
 * the control hand-off at the errands boundary), else 0. */
int cutscene_step(cutscene *cs, int confirm_pressed);

int cutscene_active(const cutscene *cs);
int cutscene_complete(const cutscene *cs);

/* Drain the pending one-shot beat action (THEME 3): if a beat issued a camera
 * pan or scene fade this tick, copy it into *out, clear it, and return 1; else
 * return 0.  The caller performs it against the live camera / scene_fade (which
 * cutscene.c, being pure C, does not touch).  Call once per tick AFTER
 * cutscene_step (and, for a fade that rides a room swap, after reloading the new
 * room so the fade grid paints over the new backdrop). */
int cutscene_take_action(cutscene *cs, cutscene_action *out);

/* True while the sequencer is running a non-dialogue beat list (a camera pan /
 * wait / fade between or after lines) rather than awaiting a dialogue confirm —
 * the caller can use it to suppress the advance arrow / confirm polling. */
int cutscene_in_beats(const cutscene *cs);

/* Feed the live scene_fade grid state (scene_fade_active) each tick BEFORE
 * cutscene_step, so a FADE beat completes exactly when the grid settles (retail's
 * case-2 gate).  No-op effect when no fade beat is running. */
void cutscene_set_fade_active(cutscene *cs, int active);

/* The current committed room key (CUTSCENE_ROOM_*), or 0 if inactive/invalid —
 * the caller reads it to drive the (deferred) room backdrop + to know when the
 * chain has reached the errands boundary. */
uint32_t cutscene_room_key(const cutscene *cs);

/* The OLD box popping OUT during a speaker-change transition, or NULL when none
 * is closing.  The caller renders it IN FRONT of the main box (`cs->box`), which
 * is opening behind it — retail's overlap (engine-quirks #107).  Valid only
 * while it is closing (a few ticks after a speaker-change advance). */
const dialogue_box *cutscene_closing_box(const cutscene *cs);

#endif /* OPENSUMMONERS_CUTSCENE_H */
