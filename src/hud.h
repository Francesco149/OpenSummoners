/*
 * hud.{c,h} — the freeroam STATUS HUD (FUN_00494e60, the in-game overlay).
 *
 * Rendered AFTER the world + dialogue (retail 0x48c150:165 calls 0x494e60
 * x2 after the world layers + fade grid).  Pure C (no Win32): the geometry
 * + number formatting live here (host-testable); the cel/GDI blits are the
 * caller's sink (main.c game_render_hud), like banner.{c,h} / scene_fade.c.
 *
 * This file is the LEADER PANEL (top-left): the HP/MP bars (slice 1a), the
 * HP/MP number text + panel frame (1b), the element STARS (1c-1), and the
 * LEVEL glyph (1c-2 — now bit-exact once ar_sprite_decode binds the installed
 * ramp palette, findings/freeroam-hud.md §5).  The portrait / item bar / door /
 * EXP gauge land in later slices.
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

/* ── slice 1c-1: element STARS + LEVEL digit (both keyed blits) ──────────
 *
 * Element STARS (0x494e60:100-108): a loop of *(char+0xdc) keyed blits of
 * the icon bank, one per element-affinity star, at
 *   (xbase + 0xba + k*0xd, ybase + 0x1d)   k = 0 .. count-1.
 * The star bank = FUN_00498620's DAT_008a76d0 = unified pool idx 0x31 =
 * res 0x44f (the 32×32 icon sheet); the per-star cel is frame 16 (12×9)
 * — Arche's element.  Both stars are the SAME frame in the errands
 * (ground truth sword2.osr tick 2200 seq 496-497: res 1103 fr 16 at
 * (187,30),(200,30)).  PORT-DEBT(hud-party-context): the count (2) + the
 * element frame (16) stand in for the party member's real affinity. */
#define HUD_STAR_POOL_IDX  0x31   /* DAT_008a76d0 = res 0x44f icon sheet    */
#define HUD_STAR_FRAME     16     /* Arche's element-star cel (12×9)        */
#define HUD_STAR_COUNT     2      /* PORT-DEBT: her 2 affinity stars        */
#define HUD_STAR_DX        0xba   /* first star x = xbase + 0xba (= 187)    */
#define HUD_STAR_STEP      0xd    /* +13 px per star                       */
#define HUD_STAR_DY        0x1d   /* star y = ybase + 0x1d (= 30)           */

/* LEVEL digit (0x494e60:123-124 → 0x495e40): the leader level, drawn
 * as UI-bank glyphs (NOT GDI text) from the small-font atlas = the mgr
 * digit bank *(0x8a6b60+0xaac + 1*4) = unified pool idx 1 = ramp slot 0 =
 * res 0x413.  0x495e40 lays the string out left-to-right: glyph i of
 * char c is atlas frame (c - 0x21), keyed-blit at (base_x + i*advance,
 * base_y); advance = param_4 = 9 (param_7=0 → fixed width).  base =
 * (xbase + 0xa0, ybase + 0x18) = (161,25).  The value = char+0xe0 +
 * char+0xd8 (level base+bonus).  Ground truth seq 526: res0 fr 16 (='1')
 * at (161,25,8,14).  PORT-DEBT(hud-party-context): the value (1). */
#define HUD_LEVEL_POOL_IDX 1      /* pool[1] = ramp 0 = res 0x413 font atlas */
#define HUD_LEVEL_VALUE    1      /* PORT-DEBT: Arche's errands level        */
#define HUD_LEVEL_DX       0xa0   /* level base x = xbase + 0xa0 (= 161)     */
#define HUD_LEVEL_DY       0x18   /* level y      = ybase + 0x18 (= 25)      */
#define HUD_LEVEL_ADVANCE  9      /* per-glyph x step (param_4, param_7=0)   */
/* The atlas frame for a printable glyph (0x495e40: ' ' < c < '{' →
 * frame c - 0x21; ' ' and out-of-range → -1 = a gap, no blit). */
int hud_glyph_frame(char c);

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
