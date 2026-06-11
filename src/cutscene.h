/*
 * cutscene.{c,h} — the town-arrival CUTSCENE driver (the dialogue advance).
 *
 * The port-side beat sequence of 0x4d7d80 case 0x334be (the town-gate
 * family conversation) + the beat-runner FUN_00439690.  Retail runs the script
 * as a BLOCKING coroutine — each beat calls 0x439680 (→ 0x439690) which
 * pumps frames until the beat completes, then falls through to the next.  The
 * port's frame loop is non-blocking, so this models the same sequence as a
 * STATE MACHINE stepped once per sim-tick: it drives the dialogue box
 * (dialogue.{c,h}) through the ~N lines, advancing on the Z input (ring id 0x24
 * = the 0x43b980 advance-poll) exactly when the current line is fully typed
 * (dialogue_awaiting_advance — the beat-runner's DIALOGUE-beat completion gate,
 * 0x439690:1004).  When the last line clears, the cutscene completes (the
 * caller then performs the control hand-off).
 *
 * SCOPE (this chip = "the dialogue advance"): the DIALOGUE lines only.  The
 * interleaved non-dialogue beats of the real script — the opening/closing fades
 * (beat type 2), the camera pans (type 3), the wait timers (type 6), and the
 * actor emotes/run-offs (0x401e60 / 0x402730) — stay on the port's
 * existing measured-frame triggers (the camera pan / banner / letterbox /
 * scene-fade, all separately validated) and are NOT sequenced here yet.
 * PORT-DEBT(cutscene-beat-runner): fold the camera/timer/action beats into this
 * sequencer (retiring those measured triggers) so the arrival plays on the
 * engine's schedule.  The town-arrival vignette also CHAINS to scene 0x334c8
 * (return 2) before the real inn control transfer — PORT-DEBT(cutscene-scene-
 * chain): the downstream scenes (0x334c8 → … → the inn "PLAYER!" prompt) are a
 * separate arc; this chip ends at the last arrival line.
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

/* The advance/confirm ring id (the Z key) the DIALOGUE beat polls via 0x43b980
 * — the same id the title menu's confirm reads (input.h).  The caller polls the
 * input ring for it each sim-tick and passes the hit to cutscene_step. */
#define CUTSCENE_ADVANCE_RING_ID 0x24

/* One scripted dialogue line — the args the script passes to FUN_0049d6e0
 * (the dialogue-line setup): the speaker's dramatist NAME and the line TEXT by
 * exe VA, the portrait FACE id, and the VOICE clip id.  name_or_0 in the real
 * call is 0, so the name resolves from the speaker actor's +0x750 (= the
 * dramatist-table name); the port resolves the same string by its VA directly. */
typedef struct cutscene_line {
    uint32_t name_va;   /* dramatist name string VA (DAT_006b6ea8 row +0x08)     */
    uint32_t text_va;   /* line text string VA (0x4d7d80 pcVar6 label)           */
    int16_t  face;      /* portrait face id (uVar9; render detail — deferred)    */
    int16_t  voice;     /* voice clip id (uVar5; 1_%07d.wma — deferred)          */
} cutscene_line;

/* A resolver VA → string (main.c supplies exe_data_string; tests stub it). */
typedef const char *(*cutscene_str_resolver)(uint32_t va);

typedef struct cutscene {
    const cutscene_line  *script;    /* the line table (static; e.g. town arrival) */
    int                   n_lines;
    int                   line_idx;  /* current line (0-based)                     */
    int                   active;    /* the sequence is running                    */
    int                   complete;  /* all lines cleared (one-shot, set once)     */
    cutscene_str_resolver resolve;
    dialogue_box         *box;       /* the box the caller owns + renders          */
} cutscene;

/* The town-gate family conversation (0x4d7d80 case 0x334be, flag 0x5f76805==0,
 * lines 33-292): 10 lines, Father/Arche/Mother.  Returns the static table and
 * its length via *n. */
const cutscene_line *cutscene_town_arrival(int *n);

/* Arm a cutscene: bind the script + resolver + box, reset to line 0, and arm
 * the box with line 0 (resolving its name/text VAs).  Safe no-op if any of
 * script/resolve/box is NULL or n <= 0. */
void cutscene_arm(cutscene *cs, const cutscene_line *script, int n,
                  cutscene_str_resolver resolve, dialogue_box *box);

/* One sim-tick: step the box (pop-in / fade / typewriter), then — if the
 * advance input fired this tick AND the line is fully typed
 * (dialogue_awaiting_advance) — advance to the next line (re-arm the box) or,
 * past the last line, complete (deactivate the box, set complete=1).
 * `advance_pressed` = Z was consumed from the ring this tick (ring id 0x24).
 * Returns 1 on the tick the sequence COMPLETES (the caller fires the control
 * hand-off), else 0. */
int cutscene_step(cutscene *cs, int advance_pressed);

int cutscene_active(const cutscene *cs);
int cutscene_complete(const cutscene *cs);

#endif /* OPENSUMMONERS_CUTSCENE_H */
