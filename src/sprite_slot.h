/*
 * sprite_slot.{c,h} — the SPRITE SLOT helpers (sys+0xaac + 4 x bank, ar_sprite_slot: its entries'
 * frame surfaces + palette buffers), retail layout; rhost-verified inside retail.
 *
 * A slot's entry k holds the frame surfaces (+0, f_38 of them) and a 1 KB palette (+4).  Freeing
 * an entry's frames releases each surface (0x5b9390) and the array; the time-of-day change frees
 * every graded slot's frames (those whose palette descriptor sys+0x18e0 + 4 x bank has a kind) so
 * they rebuild with the new tint.  A swap table (u16 source bank, then 256 destination / 256 source
 * index bytes, 0-terminated) copies entries of the source bank's palette (or the given one) into
 * the given palette.
 *
 *   FUN_00417870  slot_frames_free     FUN_004118b0  slot_frames_free_all
 *   FUN_0056df10  slots_free_graded    FUN_00491770  slot_palette_set
 *   FUN_00417bc0  pal_swap_apply       FUN_0049a050  sprite_slot_ready
 *   FUN_00491740  sprite_slot_tint
 */
#ifndef OSS_SPRITE_SLOT_H
#define OSS_SPRITE_SLOT_H

#include <stdint.h>

#include "asset_register.h"

/* 0x417870 (thiscall slot): free entry k's frame surfaces */
void slot_frames_free(ar_sprite_slot *s, uint16_t k);
/* 0x4118b0 (thiscall slot): ... every entry's */
void slot_frames_free_all(ar_sprite_slot *s);
/* 0x56df10 (thiscall sys): every graded slot's (the palette descriptor has a kind) */
void slots_free_graded(void *sys);
/* 0x491770 (thiscall slot): entry 0's palette = pal (1 KB, allocated on first use) */
void slot_palette_set(ar_sprite_slot *s, const uint8_t *pal);
/* 0x417bc0 (thiscall sys): apply swap table `tbl` to `pal` */
void pal_swap_apply(void *sys, const uint8_t *tbl, uint8_t *pal);
/* 0x49a050 (thiscall slot): its frames are built */
int sprite_slot_ready(const void *slot);
/* 0x491740 (thiscall slot): the per-channel tint (per mille) + the gamma LUT */
void sprite_slot_tint(void *slot, int32_t r, int32_t g, int32_t b, const uint8_t *lut);

#endif /* OSS_SPRITE_SLOT_H */
