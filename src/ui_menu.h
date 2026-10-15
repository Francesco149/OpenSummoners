/*
 * ui_menu.{c,h} — the menu screens built on the UI toolkit (rhost-verified inside retail), starting with the
 * shop's: its message window, the party status window, the "You have N" column, the class / item fit and
 * the menu's sprite set release.  Plan: docs/plans/ui-toolkit.md (chip 3).
 *
 *   FUN_004123a0  menu_msg_open       FUN_004129b0  menu_party_open     FUN_00412df0  menu_party_fill
 *   FUN_00415a10  menu_have_counts    FUN_00417480  menu_item_fits      FUN_00415350  menu_sprites_free
 *
 * The menu context M (0x34 B, on the caller's stack; set up by 0x415420): +0 the window manager, +4, +8,
 * +0xc the room, +0x10, +0x14 the result, u16 +0x18 the sprite set (sys+0xaac[i]), +0x20 the message window,
 * +0x24 its text control, +0x28 the party status window, +0x30.
 */
#ifndef OSS_UI_MENU_H
#define OSS_UI_MENU_H

#include <stdint.h>

/* the message window (8, 346, 624 x 128) + its text control (64, 24; 68 x 3 glyphs) with the key tokens */
uint8_t *menu_msg_open(uint8_t *m);                                                    /* 0x4123a0 */
/* "Party Member Status": a row per ready member (LV / HP / MP) + the money and its cap */
void menu_party_open(uint8_t *m);                                                      /* 0x4129b0 */
void menu_party_fill(uint8_t *m);                                                      /* 0x412df0 */
/* column 4 of every row of `c`: "You have N" (the unequipped records of the row's item +4) or "" */
intptr_t menu_have_counts(uint8_t *m, uint8_t *c);                                     /* 0x415a10 */
/* whether the member `code` (in the party, active 1 / 2) can use `item` (its kind +0x7c) */
int32_t menu_item_fits(const uint8_t *m, int32_t code, const uint8_t *item);           /* 0x417480 */
/* release the menu's sprite set: every entry's frames (the surface free 0x5b9390), then mark it unloaded */
int32_t menu_sprites_free(uint8_t *m);                                                 /* 0x415350 */

#endif /* OSS_UI_MENU_H */
