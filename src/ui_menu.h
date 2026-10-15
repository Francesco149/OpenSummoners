/*
 * ui_menu.{c,h} — the menu screens built on the UI toolkit (rhost-verified inside retail), starting with the
 * shop's: its message window, the party status window, the "You have N" column, the class / item fit and
 * the menu's sprite set release.  Plan: docs/plans/ui-toolkit.md (chip 3).
 *
 *   FUN_004123a0  menu_msg_open       FUN_004129b0  menu_party_open     FUN_00412df0  menu_party_fill
 *   FUN_00415a10  menu_have_counts    FUN_00417480  menu_item_fits      FUN_00415350  menu_sprites_free
 *   FUN_00415060  shop_run            FUN_00415460  shop_buy            FUN_00415f60  shop_who
 *   FUN_004167a0  member_rows_add     FUN_004169e0  equip_panel_fill    FUN_004175a0  member_sprite
 *   FUN_00418c90  stat_row_set
 *
 * The menu context M (0x34 B, on the caller's stack; set up by 0x415420): +0 the window manager, +4, +8,
 * +0xc the room, +0x10, +0x14 the result, u16 +0x18 the sprite set (sys+0xaac[i]), +0x20 the message window,
 * +0x24 its text control, +0x28 the party status window, +0x30 (the sell list's: the runner's +0x18).
 *
 * The shop (a chapter script's 0x415060 on the event runner): the Buy / Sell choice, then Buy = the stock
 * list 0x415460 (an item's description in the message window; gear goes to "On who?" 0x415f60, the rest to
 * the buy confirm 0x415ae0 — a slot, as are the sell list 0x4192f0 and the "can't change gear now" box
 * 0x418ec0).  "On who?" previews the item on the cursor member: the worn item >> the new one, the stats
 * now >> with it (red down / blue up), the sprite now >> recoloured with the item's palette swaps (a clone
 * in the runtime banks 884..908, kept in M +0x18).  Results: 11 cancelled, 12 bought / equipped, 6 the
 * scene ended.
 */
#ifndef OSS_UI_MENU_H
#define OSS_UI_MENU_H

#include <stdint.h>

#include "world_layout.h"

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

/* the shop: `list` = the stock's item ids (0-terminated); a3 / a4 skip the choice: Buy / Sell */
int32_t shop_run(wl_event *ev, const int32_t *list, int32_t a3, int32_t a4);           /* 0x415060 */
int32_t shop_buy(uint8_t *m, const int32_t *list);                                     /* 0x415460 */
/* "On who?" for gear `item` (inventory record `k` when not `priced`; else bought through 0x415ae0) */
int32_t shop_who(uint8_t *m, const uint8_t *item, uint16_t k, int32_t priced);         /* 0x415f60 */
/* a row {1, id} + the name per member in the party (100000104 only with `all`) */
int32_t member_rows_add(const wl_room *r, uint8_t *c, int32_t all);                    /* 0x4167a0 */
/* the equip preview of item `item_id` on member `id` (0: "Don't Equip"); `priced`: its price row */
void equip_panel_fill(uint8_t *m, uint8_t *win, wl_room *room, int32_t id, int32_t item_id, int32_t priced); /* 0x4169e0 */
/* a member's sprite bank / frame / clip (`alt`: the 50010 alternate); with `item` (+0x2b4 a palette swap
 * table) a runtime clone recoloured with the member's gear swaps + the item's */
void member_sprite(uint8_t *m, const uint8_t *s, uint16_t *bank, uint16_t *frame, const uint8_t **clip, int32_t alt,
                   const uint8_t *item);                                               /* 0x4175a0 */
/* a stat row: icon, label, base + cur  >>  base + next (coloured when it changes) */
void stat_row_set(uint8_t *c, uint16_t row, const char *label, int32_t base, int32_t cur, int32_t next,
                  int16_t icon);                                                       /* 0x418c90 */

#endif /* OSS_UI_MENU_H */
