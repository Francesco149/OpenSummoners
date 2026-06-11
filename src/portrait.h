/*
 * portrait.{c,h} — the dialogue-PORTRAIT resolver (per-speaker bust selection).
 *
 * The open-air reduction of 0x49d6e0's portrait lookup (the dialogue-line
 * setup).  Retail resolves the bust per line: it reads the SPEAKER actor's
 * head-state (+0x1d8) + the line's face_id (param_8), scans the face table
 * DAT_006b6568 (embedded verbatim in portrait_face_data.c, key
 * [head_state, face_id]), and writes the resulting portrait pool-slot to the
 * box's +0x84.  The render then blits (&DAT_008a760c)[slot] (= the port's
 * g_ar_sprite_slots[slot]).  Each face-table record holds 3 slot VARIANTS,
 * picked by the speaker's anim-state + a box-state flag (0x49d6e0:121-148):
 *   +0x8 (var A) when the speaker body faces 3 (local_110 == 3),
 *   +0xa (var B) the default,
 *   +0xc (var C) the bVar4 box-state path (in_ECX+0x2f0 != 0 && +0x7c in {0,1,2,4}).
 *
 * The port supplies the head-state per speaker (the cutscene cast's +0x1d8,
 * HARNESS-CAPTURED: Arche=100000101, Father=100000211, Mother=100000212 —
 * runs/portrait-gt) + the face_id (the script table).  Pure (no Win32);
 * host-tested (tests/test_portrait.c).
 */
#ifndef OPENSUMMONERS_PORTRAIT_H
#define OPENSUMMONERS_PORTRAIT_H

#include <stdint.h>

/* The variant column (the 0x49d6e0:137-148 branch). */
typedef enum portrait_variant {
    PORTRAIT_VAR_A = 0,   /* +0x8  — speaker body facing == 3 (local_110 == 3) */
    PORTRAIT_VAR_B,       /* +0xa  — the default path                          */
    PORTRAIT_VAR_C        /* +0xc  — the bVar4 box-state path                  */
} portrait_variant;

/* Resolve (head_state, face_id) → the portrait pool-slot (g_ar_sprite_slots
 * index) for the given variant, via the embedded face table.  Returns -1 if
 * the (head, face) pair has no record (retail's no-portrait path, +0x20=1). */
int portrait_resolve(int32_t head_state, int32_t face_id, portrait_variant var);

#endif /* OPENSUMMONERS_PORTRAIT_H */
