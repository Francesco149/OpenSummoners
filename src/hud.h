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
 * LEVEL glyph + EXP gauge (1c-2 — findings/freeroam-hud.md §5-6).  The
 * portrait / item bar / door land in later slices.
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

/* EXP gauge (0x494e60:95 → FUN_00498f10): the leader's EXP bar, a single
 * row below the HP/MP bars.  Source = pool idx 0x2e = g_ar_sprite_slots[33]
 * = res 0x44e (HUD_EXP_BANK_SLOT in main.c's grade skip-list).  Position
 * (xbase + 0x8f, ybase + 0x29) = (144,42) when slid in; width 0x68 (104),
 * height 2.  Ground truth sword2.osr tick 2200 seq493-494: the FILLED span
 * is 0-width (Arche's errands EXP = 0: cur=0 and char+0xe8=0, no combat in
 * the errands) — omitted, like the HP/MP bars omit a 0-width depleted
 * (ckpt 167); the DEPLETED span is the full gauge width, drawn via the
 * mode-4 ALPHA path (0x498f10's display-mode branch, NOT the plain rects
 * copy the FILLED span would use), src (0, 14), blend descriptor = the
 * port's g_ramp_b[8] (LUT md5 ed6214bd, matched by content against all 20
 * ramp_a + 20 ramp_b entries — exactly one full match; findings/freeroam-
 * hud.md §6).  PORT-DEBT(hud-party-context): EXP pinned at 0 (Arche's
 * errands value), so only the depleted span ever draws. */
#define HUD_EXP_POOL_IDX   0x2e   /* pool[0x2e] = g_ar_sprite_slots[33] = res 0x44e */
#define HUD_EXP_DX         0x8f   /* exp x = xbase + 0x8f (= 144)            */
#define HUD_EXP_DY         0x29   /* exp y = ybase + 0x29 (= 42)             */
#define HUD_EXP_WIDTH      0x68   /* 104 — full gauge width                  */
#define HUD_EXP_HEIGHT     2
#define HUD_EXP_SRC_Y      14    /* the depleted gradient row               */
#define HUD_EXP_RAMP_B_IDX 8     /* g_ramp_b[8] — the alpha blend descriptor */

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

/* ── slice 2: the ITEM BAR (FUN_004962a0 ×6, bottom-right cluster) ───────
 *
 * Static disasm (ckpt 173; Ghidra's decompile hides all three thiscall/
 * pool reads here, same trap as the ckpt-171/172 portrait hunt) pins THREE
 * keyed blits per slot, all at the SAME (x,y):
 *   1. BG frame (ALWAYS): pool 0x39 (res 0x450), a fixed frame — NOT frame 0
 *      (0x4184a0(this,0)'s "0" is the entry_idx/lazy-decode-once arg, not a
 *      frame index; the actual draw reads the bank's cached info+0x30 =
 *      frames[12] literally).  Ground-truthed by brute-force dhash sweep
 *      (0x494e60's own asm has no frame-select call to read here at all).
 *   2. ICON frame (if param_3 != 0): pool 0x31 == HUD_STAR_POOL_IDX (res
 *      0x44f, the SAME sheet the element stars use) at frame = param_3, a
 *      per-slot game-state-selected mode icon.
 *   3. LABEL frame (ALWAYS): the SAME pool 0x31 bank, frame = slot_index+4
 *      — an F1..F6 key-cap glyph (dhash-confirmed against sword2.osr: 'F1'
 *      .. 'F6' bitmaps at frames 4..9), independent of party/game state.
 * Position: x = slot*0x20 - (hslide*200)/1000 + 0x1b8; y = (1000-vslide)*
 * 0x80/1000 + 0x1bc (FUN_004962a0's own math, param_4=vslide/param_5=hslide).
 *
 * vslide == room+0x3c8 (0x49af40 "hud_party_anim_update": +20/tick
 * toward 1000 while the HUD's room context is active, -20/tick toward 0
 * otherwise — a SEPARATE, slower-paced sibling of the panel's own +0x1b0/
 * hud_slide_step, not the same counter).  Ported as hud_item_slide_step,
 * armed at the SAME control hand-off as the panel (main.c).
 * hslide == room+0x388, the DOOR-PROXIMITY glow ramp (same 0x49af40:
 * +40/tick within range of a tracked exit actor, else -10/-20/tick) — an
 * entirely separate, unported subsystem (the door indicator itself is
 * deferred, findings/freeroam-hud.md "Scoping correction ckpt 172").
 * PORT-DEBT(hud-item-hslide): pinned at 0 (Arche is not near a door in the
 * errands ground truth — sword2.osr tick 2200's item-bar x's are exactly
 * slot*32+440, zero offset), retired when the door subsystem lands.
 *
 * PORT-DEBT(hud-party-context): the 6 ICON frames are the errands leader's
 * captured mode/status values (party subsystem unported), ground-truthed
 * bit-exact off sword2.osr tick 2200 by brute-force dhash sweep of pool
 * 0x31's frames 0..89 (findings/freeroam-hud.md §8):
 *   slot0 <- 0x4961a0() party-scan bool (0/1)       -> observed 0 -> fr 44
 *   slot1 <- room+4->+0x4070 quick-item-bound flag  -> observed 0 -> fr 48
 *   slot2 <- room+0x4050 clamped [0,2]              -> observed 0 -> fr 40
 *   slot3 <- room+0x4054 (switch 0..4)              -> observed 3 -> fr 36
 *   slot4 <- leader char+0x750+0x140 (gated)         -> gate false -> fr 59
 *   slot5 <- room+0x4058 clamped [0,2]              -> observed 0 -> fr 80
 * The LABEL frames need no stand-in (slot_index+4 has no game-state input). */
#define HUD_ITEM_BG_POOL_IDX    0x39    /* pool[0x39] = res 0x450 (48x48 cell) */
#define HUD_ITEM_BG_FRAME       12      /* the bank's one cached frame (+0x30) */
#define HUD_ITEM_ICON_POOL_IDX  HUD_STAR_POOL_IDX /* pool 0x31 = res 0x44f, shared */
#define HUD_ITEM_SLOT_COUNT     6
#define HUD_ITEM_SLOT_DX0       0x1b8   /* slot 0 x at hslide=0 (= 440)        */
#define HUD_ITEM_SLOT_STEP      0x20    /* +32 px per slot                    */
#define HUD_ITEM_SLOT_DY_BASE   0x1bc   /* y at vslide=1000 (= 444)            */
#define HUD_ITEM_LABEL_BASE     4       /* label frame = slot_index + 4        */
#define HUD_ITEM_HSLIDE         0       /* PORT-DEBT(hud-item-hslide): door-glow floor */

/* PORT-DEBT(hud-party-context): the 6 slots' ground-truthed ICON frames
 * (0 would mean "no icon" and skip the draw — never observed in the
 * errands).  Indexed by slot 0..5. */
extern const int HUD_ITEM_ICON_FRAME[HUD_ITEM_SLOT_COUNT];

/* FUN_004962a0's position math, split per axis (both PURE int formulas). */
int hud_item_slot_x(int slot, int hslide);
int hud_item_slot_y(int vslide);

/* The item-bar's OWN slide-in ramp (room+0x3c8): +20/tick toward 1000,
 * capped — the slower sibling of hud_slide_step (+50/tick).  PORT-DEBT
 * (hud-slide): models only the arm-and-rise transient, like hud_slide_step;
 * the retail -20/tick fall-off (room context going inactive) is unexercised
 * in a stable freeroam session. */
#define HUD_ITEM_SLIDE_STEP 20
int hud_item_slide_step(int prog);

/* ── slice 3: the DOOR INDICATOR (FUN_004969b0) ──────────────────────────
 *
 * A full off-screen multi-exit compass-arrow system, NOT a fixed icon
 * (findings/freeroam-hud.md "Scoping correction ckpt 172" + the ckpt-174
 * static-disasm RE): scans a 32-slot EFFECT actor band (DAT_008a9b50+0x1160)
 * for candidates matching a room/zone relation to the reference actor
 * (the errands leader), pre-filters by a 72000x56000 world-space "reach"
 * box, EXCLUDES any candidate already on-screen (only off-screen exits get
 * an arrow), projects the survivor through the camera + clamps to the
 * viewport edge (picking one of 4 EDGE directions), DEDUPES/stacks arrows
 * landing within 5px of each other (up to 20 distinct clusters, 5 stacked
 * per cluster, offset 12px per stack level INTO the screen), bumps to a
 * HIGHLIGHTED frame set when the candidate is the door the leader is
 * standing at, and alpha-fades near the reach-box edge via a 20-entry
 * blend-LUT ramp (g_ramp_b, the SAME table the EXP gauge/sword-trail/
 * particle sky-fade already use — DAT_008a9308, ckpt-163/171 family).
 *
 * Every ECX (thiscall `this`) at the ACTUAL asm call sites was verified by
 * objdump (Ghidra's decompile hides them all, same trap as the ckpt-171/172
 * portrait hunt + the ckpt-173 item-bar wrapper): FUN_0044e640/_e680
 * (candidate center x/y) run with `this` = the CANDIDATE actor; the second
 * FUN_0044e680 call (reference center y) runs with `this` = the REFERENCE
 * (param_2); FUN_004766a0 (on-screen test) runs with `this` = the CAMERA
 * (param_3); the final blit's blend descriptor is `this` = esi (the ramp
 * lookup), NOT the frame's own +0x28 field (that is the *colorkey* arg).
 *
 * Call-site pin (0x494e60:455): FUN_004969b0(dst, hud_ctx+0x24 [reference],
 * DAT_008a9b50+0x104c [[the SAME camera_view/mr_camera room+0x104c object
 * camera_follow/map_render already model]], hud_ctx+0x398 [the highlighted/
 * tracked-door pointer — the SAME field ckpt-173's item-bar hslide reads]).
 *
 * PORT-DEBT(hud-door-actors): the +0x1160 EFFECT-band actor SPAWN is a new,
 * unported subsystem — the errands room's map data (DATA scene, dumped via
 * a throwaway recon pass) has exactly TWO EFFECT-band (50000..59999) object
 * placements (code 50240 @ world (62400,28800) and code 50140 @ world
 * (48000,48000)); across the full errands map (108800x64000, viewport
 * 64000x48000, horizontal-only scroll [0,44800]) BOTH stay inside the
 * viewport at every reachable camera position, so this port's candidate
 * list — built from those two real, data-sourced positions — faithfully
 * produces ZERO visible indicators in the errands, matching sword2.osr's
 * ground truth (a full-session scan found zero res=0x451 blits).  The
 * ALGORITHM below is statically RE-verified bit-exact (disasm-traced, not
 * measured/curve-fit); only the room's FULL exit roster + each door's
 * active/valid/status/zone fields (retail's +0x1d0/body[0]/body[0x7e]/
 * char+0x750+0x44a/+0x1dc, sourced from the still-unread EFFECT activator
 * 0x41f200 + the room-registry reciprocal-exit table in game_world.c's
 * gw_cross_reference) remain to derive — retire when either lands, or when
 * a real-play capture exercises an actual off-screen exit to verify the
 * highlighted/stacked/alpha-fade branches end-to-end (none are reachable
 * in any currently-captured session). */
#define HUD_DOOR_POOL_IDX        0x3a   /* pool[0x3a] = 0x8a76f4 = g_ar_sprite_slots[45] = res 0x451, 64x64 */
#define HUD_DOOR_REACH_X         72000  /* world-space pre-filter half-extent x   */
#define HUD_DOOR_REACH_Y         56000  /* world-space pre-filter half-extent y   */
#define HUD_DOOR_FADE_X_DENOM    32000  /* fade-ratio denominator x               */
#define HUD_DOOR_FADE_Y_DENOM    24000  /* fade-ratio denominator y               */
#define HUD_DOOR_CLUSTER_RADIUS 5       /* dedup bucket match: abs(dx)<5&&abs(dy)<5*/
#define HUD_DOOR_STACK_OFFSET   12      /* px per stacked duplicate, into-screen  */
#define HUD_DOOR_MAX_BUCKETS    20      /* the dedup table's capacity (aiStack_f0)*/
#define HUD_DOOR_RAMP_COUNT     20      /* g_ramp_b / PD_BOOT_GROUP_B_COUNT       */
#define HUD_DOOR_BLIT_OFFSET    0x20    /* the anchor->blit-corner offset (-32)   */

#define HUD_DOOR_EDGE_TOP       0
#define HUD_DOOR_EDGE_RIGHT     1
#define HUD_DOOR_EDGE_BOTTOM    2
#define HUD_DOOR_EDGE_LEFT      3

/* A world-space actor body — the render-state rect FUN_004969b0 reads off
 * `actor+0x40` (retail offsets in comments; x/y are the top-left world pos,
 * *100 fixed point, matching actor_render_state.world_x/y). */
typedef struct {
    int32_t x, y;         /* +0x04 / +0x08 */
    int32_t w, h;          /* +0x0c / +0x10 */
    int32_t baseline;      /* +0x14 — the "feet" reference the Y-center uses */
} hud_door_body;

/* One candidate EFFECT-band actor (iVar7 in the retail loop). */
typedef struct {
    hud_door_body body;
    int32_t     zone;        /* +0x1dc — room/zone relation tag              */
    int         active;      /* +0x1d0 == 1                                  */
    int         body_valid;  /* body[0] == 1                                 */
    int         suppressed;  /* body[0x7e] != 0 => filtered out              */
    int         status;      /* *(char+0x750)+0x44a; must == 0               */
    const void *id;          /* identity for the highlight-pointer compare   */
} hud_door_candidate;

/* The reference actor (param_2 — hud_ctx+0x24, the errands leader). */
typedef struct {
    hud_door_body body;
    int32_t       zone;      /* +0x1dc */
} hud_door_ref;

/* The camera/view (param_3 — DAT_008a9b50+0x104c; maps 1:1 onto the port's
 * existing mr_camera off34/off4c/off5c/off60/off64/off68/off74 fields). */
typedef struct {
    int32_t cam34, cam4c, cam5c, cam60, cam64, cam68, cam74;
} hud_door_camera;

/* One dedup/stack bucket (the retail stack pair {aiStack_f0[], auStack_e8[]},
 * modelled as one struct — same 12-byte stride, only logically split by
 * Ghidra since mixed int/short accesses hit the SAME stack slots). */
typedef struct {
    int32_t x, y;
    int16_t count;
} hud_door_bucket;

/* The scan-wide dedup state — reset once per HUD render pass, threaded
 * through every hud_door_process call for that frame (bucket assignment is
 * ORDER-SENSITIVE, exactly as retail's single-pass 32-actor loop). */
typedef struct {
    hud_door_bucket buckets[HUD_DOOR_MAX_BUCKETS];
    int             n;
} hud_door_dedup;

void hud_door_dedup_reset(hud_door_dedup *d);

/* One computed draw.  visible==0 => no draw for this candidate (filtered,
 * on-screen, or a 6th+ stacked duplicate at the same cluster).
 * frame_index (4..11) indexes the door bank's frame: dir 0-3 (TOP/RIGHT/
 * BOTTOM/LEFT) + 4 when highlighted, + 4 again (retail's unconditional
 * "+4" — the low 4 frames of the bank are unused by this renderer).
 * ramp_idx: -1 => draw OPAQUE (plain keyed blit); 0..19 => alpha-blit
 * through g_ramp_b[ramp_idx] (NULL-checked by the caller, like the EXP
 * gauge). cx/cy is the stacked/clamped anchor — the blit corner is
 * (cx - HUD_DOOR_BLIT_OFFSET, cy - HUD_DOOR_BLIT_OFFSET) (+ the frame cel's
 * own metric_0c/_10 for the alpha path only — FUN_005b9b70 adds it
 * internally, FUN_005bd550/zdd_blit_orchestrate does not; main.c #1706's
 * existing keyed-vs-orchestrate offset split). */
typedef struct {
    int     visible;
    int     frame_index;
    int32_t cx, cy;
    int     ramp_idx;
} hud_door_draw;

/* Port of FUN_004969b0's per-actor body (one candidate against the current
 * dedup state).  Returns 0 normally (check out->visible), or -1 when the
 * dedup table is exhausted mid-scan — retail's bare `return;` ABORTS THE
 * WHOLE 32-actor loop, so the caller must stop processing further
 * candidates for this frame too (not just skip this one). */
int hud_door_process(hud_door_dedup *dedup, const hud_door_ref *ref,
                     const hud_door_camera *cam, const hud_door_candidate *cand,
                     const void *highlight_id, hud_door_draw *out);

#endif /* OPENSUMMONERS_HUD_H */
