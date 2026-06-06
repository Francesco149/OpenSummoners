/*
 * src/render_id.h — stable cross-side identity for blitted sprites.
 *
 * The DDraw blit trace (the render-stream drill-in, docs/plans/
 * trace-tooling-phase-b.md B3) serialises each blit's SOURCE as a raw
 * zdd_object* / IDirectDrawSurface7* pointer — allocation-dependent, so it
 * differs between the port and retail (and between runs).  To diff the blit
 * stream cross-side we need a name for the source that is the SAME in both
 * binaries.  This is the SotES analog of openrecet's `tex_name` registry
 * (src/d3d_tex_names.c): map each cel pointer to the LOAD-STABLE identity it
 * was resolved from.
 *
 * The asset-anchored identity here is `(resource_id, frame)`:
 *   - resource_id — the PE "DATA" resource the sprite bank decoded from
 *     (ar_sprite_slot +0x40).  Identical across both binaries because both
 *     read the SAME embedded resources from the SAME DLLs.
 *   - frame       — the frame index within the bank (the second arg to the
 *     universal resolver FUN_00418470 / ar_sprite_slot_frame).
 * Both sides register at that ONE resolver chokepoint, so every cel — tiles,
 * parallax, prologue, menu, actors — is tagged uniformly.
 *
 * IMPROVEMENT over openrecet's name-only scheme (and the reason to do it for a
 * SOFTWARE blitter): `dhash`, a content fingerprint of the DECODED sheet
 * pixels.  openrecet's textures live in GPU memory, so hashing them is
 * expensive and it identifies textures by name only — it can tell you the WRONG
 * sprite is bound, but not that the RIGHT sprite decoded to the WRONG pixels
 * (a palette-grade / 24bpp-decode divergence — exactly the recurring SotES
 * residual class, ckpt 67/68).  Our pixels sit in a CPU buffer at decode time,
 * so a per-bank FNV-1a fingerprint is nearly free.  Carried on every blit, it
 * lets render_diff classify a divergence as "right sprite, wrong decode"
 * distinct from "wrong sprite" / "wrong rect" / "wrong blend state".
 *
 * Pure C, no Win32 / ddraw.h — keyed on `const void*` so it links into the host
 * test suite (test_render_id.c) and the cross build alike.
 */
#ifndef OPENSUMMONERS_RENDER_ID_H
#define OPENSUMMONERS_RENDER_ID_H

#include <stddef.h>
#include <stdint.h>

/* The stable identity of one resolved sprite cel. */
typedef struct render_id {
    uint16_t resource_id;   /* PE resource the bank decoded from (cross-side anchor) */
    uint16_t frame;         /* frame index within the bank */
    uint32_t dhash;         /* decoded-sheet content fingerprint (0 = unknown) */
} render_id;

/* ── cel-pointer → render_id registry ──────────────────────────────────────
 * Open-addressing (linear-probe) hash table over the cel pointer, modelled on
 * openrecet's d3d_tex_names.c (TOMBSTONE on forget so a recycled pointer never
 * carries a stale identity).  register() overwrites an existing entry. */
void render_id_register(const void *cel, uint16_t resource_id,
                        uint16_t frame, uint32_t dhash);
int  render_id_lookup(const void *cel, render_id *out);   /* 1 = found, 0 = unknown */
void render_id_forget(const void *cel);
void render_id_reset(void);                               /* clear the whole table */

/* ── per-bank decoded-sheet fingerprint, keyed by resource_id ───────────────
 * Computed once when a bank's sheet is decoded; looked up at resolve time so
 * each cel's render_id carries its bank's dhash.  Separate from the cel table
 * because many cels (frames) share one decoded sheet. */
void     render_id_set_sheet_hash(uint16_t resource_id, uint32_t dhash);
uint32_t render_id_sheet_hash(uint16_t resource_id);      /* 0 = unknown */
void     render_id_reset_sheets(void);

/* ── FNV-1a (32-bit) over a byte buffer — the decoded-sheet fingerprint ──────
 * render_id_hash is the one-shot form; render_id_hash_seed lets a caller fold
 * dimensions/pitch into the seed so two sheets with identical bytes but
 * different geometry hash apart. */
uint32_t render_id_hash(const void *data, size_t len);
uint32_t render_id_hash_seed(uint32_t seed, const void *data, size_t len);

#define RENDER_ID_FNV1A_SEED 2166136261u

#endif /* OPENSUMMONERS_RENDER_ID_H */
