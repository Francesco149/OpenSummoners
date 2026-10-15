/*
 * ui_core.{c,h} — the retail UI widget toolkit's core on the retail layout (window manager, windows,
 * controls, grid / text set-up) + the small inventory / money helpers the menus use (rhost-verified inside
 * retail).  Plan + object model: docs/plans/ui-toolkit.md.
 *
 *   FUN_00414080  win_mgr_topmost     FUN_004124d0  win_mgr_open        FUN_0040f3e0  win_init
 *   FUN_00412c10  win_ctrl_take       FUN_004173d0  win_ctrl_at         FUN_0040fdb0  grid_col_place
 *   FUN_00412100  grid_col_style      FUN_0040f5a0  ui_ref_set          FUN_00412570  ui_link_drop
 *   FUN_0040dee0  win_text_add        FUN_0040df40  ctrl_text_init      FUN_00411c50  win_grid_add
 *   FUN_00411f40  grid_row_reset      FUN_0040e2f0  text_metrics_set    FUN_00411ec0  ctrl_cursor_set
 *   FUN_00414c40  icon_set            FUN_00412d30  sys_frame_style     FUN_00413760  sys_cursor_style
 *   FUN_004192b0  grid_page_fit       FUN_00415420  ui_scroll_init      FUN_00415930  money_text
 *   FUN_00417440  money_value         FUN_00415990  room_item_count     FUN_00416500  room_party_count
 *   FUN_0040f5c0  ctrl_grid_init      FUN_004134f0  win_grid_new        FUN_0040f800  grid_row_add
 *   FUN_00415860  cell_icon_set       FUN_00412c40  win_title_add       FUN_0040f1c0  sys_win_open
 *   FUN_004103d0  sys_page_arrows
 */
#ifndef OSS_UI_CORE_H
#define OSS_UI_CORE_H

#include <stdint.h>

#include "world_layout.h"

/* window manager {+0 windows, u16 +4 cap, u16 +6 count} */
void win_mgr_topmost(uint8_t *mgr);                                                    /* 0x414080 */
uint8_t *win_mgr_open(uint8_t *mgr, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t cap,
                      const void *style);                                              /* 0x4124d0 */
/* a window (0x84 B): its manager, rect, `cap` fresh 0x1b0-byte controls, the 0x24-byte style */
int win_init(uint8_t *win, uint8_t *mgr, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t cap,
             const void *style);                                                       /* 0x40f3e0 */
uint8_t *win_ctrl_take(uint8_t *win);                                                  /* 0x412c10 */
uint8_t *win_ctrl_at(const uint8_t *win, uint16_t i);                                  /* 0x4173d0 */
uint8_t *win_text_add(uint8_t *win, int32_t x, int32_t y, uint16_t w, uint16_t h);     /* 0x40dee0 */
uint8_t *win_grid_add(uint8_t *win, int32_t x, int32_t y, int32_t w, int32_t h);       /* 0x411c50 */
/* controls */
uint8_t *ctrl_text_init(uint8_t *c, int32_t x, int32_t y, uint16_t w, uint16_t h);     /* 0x40df40: the text node */
int32_t grid_col_place(uint8_t *c, int32_t col, int32_t x, int32_t a, int32_t w, int32_t b);   /* 0x40fdb0 */
int32_t grid_col_style(uint8_t *c, int32_t col, int32_t a, int32_t b, int32_t d, int32_t e, int32_t f); /* 0x412100 */
int32_t grid_row_reset(uint8_t *c, int32_t row);                                       /* 0x411f40 */
uint8_t *text_metrics_set(uint8_t *c, int32_t a, int32_t line_h, int32_t d, int32_t e); /* 0x40e2f0 */
int32_t ctrl_cursor_set(uint8_t *c, int32_t sprite, uint16_t size, const uint16_t *frames, uint16_t n,
                        uint16_t rate, int32_t dx, int32_t dy);                        /* 0x411ec0 */
int grid_page_fit(uint8_t *c);                                                         /* 0x4192b0 */
/* a grid control: rows x cols (32-px columns), `page` rows shown; rows {+0 a, +4 b, +8 live, +0xc cells} */
intptr_t ctrl_grid_init(uint8_t *c, int32_t x, int32_t y, int32_t rows, int32_t cols, int32_t page, int32_t a7); /* 0x40f5c0 */
uint8_t *win_grid_new(uint8_t *win, int32_t x, int32_t y, int32_t rows, int32_t cols, int32_t page,
                      int32_t a7);                                                     /* 0x4134f0 */
int32_t grid_row_add(uint8_t *c, int32_t a, int32_t b);                                /* 0x40f800: the row, or -1 */
/* a cell's icon (0x54 B: +0 sprite, u16 +4, u16 +6.. frames, u16 +0x46 count, u16 +0x48 rate) */
intptr_t cell_icon_set(uint8_t *c, int32_t row, int32_t col, int32_t sprite, uint16_t a5, const uint16_t *frames,
                       uint16_t n, uint16_t a8);                                       /* 0x415860 */
/* a window's title: a 1 x 1 grid at (32, 7) */
uint8_t *win_title_add(uint8_t *win, const char *text);                                /* 0x412c40 */
/* open a framed window (sys+0xb88 frames; `plain`: no top corner) + its title (a control more) */
uint8_t *sys_win_open(void *sys, uint8_t *mgr, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t cap,
                      const char *title, int32_t plain);                               /* 0x40f1c0 */
/* a grid's page arrows (sys+0xb8c) at the window's mid-height + the "%d/%d" page counter */
int32_t sys_page_arrows(void *sys, const uint8_t *win, uint8_t *c);                    /* 0x4103d0 */
/* records */
int32_t ui_ref_set(uint8_t *r, int32_t a, uint16_t b);                                 /* 0x40f5a0 */
uint8_t *ui_link_drop(uint8_t *o);                                                     /* 0x412570 */
int32_t icon_set(uint8_t *rec, int32_t sprite, uint16_t a3, const uint16_t *frames, uint16_t n, uint16_t a6); /* 0x414c40 */
int32_t ui_scroll_init(uint8_t *s, int32_t a2, int32_t a3, int32_t a4, int32_t a5);   /* 0x415420 */
/* the system's styles (`sys` = 0x8a6b60) */
uint8_t *sys_frame_style(void *sys, uint8_t *style, int32_t alt);                      /* 0x412d30 */
uint8_t *sys_cursor_style(void *sys, uint8_t *c, int32_t on);                          /* 0x413760 */
/* money (kind 10: in GP = /100, rounded up when `up`; else CP) */
char *money_text(char *buf, int32_t kind, int32_t v, int32_t up);                      /* 0x415930 */
int32_t money_value(int32_t kind, int32_t v, int32_t up);                              /* 0x417440 */
/* the room's inventory records of `item` (unequipped unless `any`); *best the one held fewest */
int32_t room_item_count(const wl_room *r, int32_t item, uint16_t *best, int32_t any);  /* 0x415990 */
/* the active party slots (100000104 only with `all`; `fit`: alive, shown, not fallen) */
int32_t room_party_count(const wl_room *r, int32_t all, int32_t fit);                  /* 0x416500 */

#endif /* OSS_UI_CORE_H */
