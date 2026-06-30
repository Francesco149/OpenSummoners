/*
 * hud.{c,h} — the freeroam STATUS HUD (FUN_00494e60, the in-game overlay).
 *
 * Rendered AFTER the world + dialogue (retail 0x48c150:165 calls 0x494e60
 * x2 after the world layers + fade grid).  Pure C (no Win32): the geometry
 * + number formatting live here (host-testable); the cel/GDI blits are the
 * caller's sink (main.c game_render_hud), like banner.{c,h} / scene_fade.c.
 *
 * This file is the LEADER PANEL (top-left): the HP/MP bars + the HP/MP
 * number text.  The panel frame / portrait / level glyph / item bar / door
 * land in later slices.
 *
 * PORT-DEBT(hud-party-context): the displayed values (HP/MP/level/element
 * stars/items) come from the party subsystem (room+0x4030) which is unported
 * (the port's freeroam Arche is a standalone character mover, not a party
 * slot).  The port stands in the errands leader's values (Arche: HP 100/100,
 * MP 20/20, Lv 1) + a fully-slid-in panel; retire when the party lands.
 */
#ifndef OPENSUMMONERS_HUD_H
#define OPENSUMMONERS_HUD_H

#include <stdint.h>

/* The HP/MP bar source bank (FUN_00498680 reads (&DAT_008a760c)[0x2f]) = the
 * retail unified-pool index 0x2f = res 0x777 (192x16 gradient, type 0),
 * registered by ar_register_palette_ramps.  ar_pool_get_slot(HUD_BAR_POOL_IDX). */
#define HUD_BAR_POOL_IDX   0x2f
/* The orchestrator's bar geometry constants (0x494e60:91-94). */
#define HUD_BAR_WIDTH      0xa8   /* 168 — full bar width (param_7)             */
#define HUD_HP_ROWS        4      /* HP bar rows (498680 param_6)               */
#define HUD_MP_ROWS        3      /* MP bar rows (498820 param_6)               */
#define HUD_HP_SRC_Y       0      /* HP gradient row (param_8 normal)           */
#define HUD_MP_SRC_Y       2      /* MP gradient row                           */
#define HUD_BAR_DEP_SRC_Y  0xe    /* 14 — the depleted gradient row (the alpha) */
/* Panel-relative offsets from the slide x-base / y-base (0x494e60). */
#define HUD_BAR_DX         0x75   /* bar x   = xbase + 0x75 (= 118 when in)     */
#define HUD_HP_BAR_DY      6      /* HP bar y = ybase + 6   (= 7)               */
#define HUD_MP_BAR_DY      0x14   /* MP bar y = ybase + 0x14 (= 21)             */
#define HUD_TEXT_DX        0xd9   /* HP/MP number x = xbase + 0xd9 (= 218)      */
#define HUD_HP_TEXT_DY     4      /* HP number y = ybase + 4  (= 5)             */
#define HUD_MP_TEXT_DY     0x11   /* MP number y = ybase + 0x11 (= 18)          */
#define HUD_TEXT_FONT      2      /* g_ar_gdi_table[2] (Courier New w7 h16)     */
/* The ornate panel FRAME (0x494e60:97 = 0x418470(0) on bank DAT_008a7724) =
 * the retail unified-pool index 70 = res 0x44b (352x96, type 2), drawn keyed
 * AFTER the bars (seq 481) at (xbase-0x20, ybase) = (-31,1).  ar_pool_get_slot. */
#define HUD_FRAME_POOL_IDX 70     /* base-0x760c pool index (-> g_ar slot 57)   */
#define HUD_FRAME_DX       (-0x20)/* frame x = xbase - 0x20 (= -31 when in)     */

/* The panel slide x-base (0x494e60:87): xbase = ((prog*0xb - 11000)*0x20)/1000
 * + 1.  prog 0..1000; == 1 fully slid in (the steady HUD).  ybase is a const 1
 * (0x494e60:86) — exposed as HUD_PANEL_YBASE for the caller. */
#define HUD_PANEL_YBASE    1
int hud_panel_xbase(int slide_progress);

/* The panel SLIDE-IN: progress ramps 0 -> 1000 at +50/sim-tick over 20 ticks
 * (armed at the errands hand-off; the cinematic step 0x499ab0 drives it on
 * retail).  +50 is EXACT — it reproduces the recording's integer xbase
 * sequence (-333,-315,-298,... at prog 50,100,150).  hud_slide_step ramps one
 * sim-tick, capped at 1000.  PORT-DEBT(hud-slide): the trigger + increment
 * stand in for the unported HUD cinematic-step subsystem. */
#define HUD_SLIDE_STEP     50
#define HUD_SLIDE_FULL     1000
int hud_slide_step(int prog);

/* One HP/MP bar row's blit geometry (FUN_00498680 per-row body).
 *   cur/max : the gauge ratio (cur 0..1000, max 1000);
 *   x,y     : the bar origin (xbase+0x75, ybase+6 [HP] / +0x14 [MP]);
 *   width   : full bar width (0xa8);
 *   src_y   : the gradient row (0 HP / 2 MP);
 *   row     : 0..rows-1 (each row steps dst_x -1, dst_y +2 — the italic skew).
 * The FILLED span is a 1:1 rects copy [dst .. dst+filled_w]; the DEPLETED span
 * [dep_x .. dep_x+dep_w] is the alpha companion (src_y = HUD_BAR_DEP_SRC_Y). */
typedef struct {
    int dst_x, dst_y, dst_w, dst_h;   /* filled rect (rects blit)              */
    int src_x, src_y, src_w, src_h;
    int dep_x, dep_w;                 /* depleted span (alpha blit), same y/h   */
} hud_bar_row;
hud_bar_row hud_bar_row_geom(int cur, int max, int x, int y, int width,
                             int src_y, int row);

/* Format an HP/MP gauge "%s / %d" — cur right-justified width 4 (0x495dc0),
 * then max (0x5bf3ee "%s / %d").  E.g. (100,100) -> " 100 / 100",
 * (20,20) -> "  20 / 20".  Writes a NUL-terminated string into buf. */
void hud_format_gauge(int cur, int max, char *buf, int buflen);

#endif /* OPENSUMMONERS_HUD_H */
