/*
 * letterbox.{c,h} — the opening-town establishing-shot CINEMATIC LETTERBOX.
 *
 * A slice of 0x48c150 (the per-frame world-render driver, @ 0x48c150).
 * After the backdrop present pass (0x48eac0), the driver tiles a single 64x4
 * opaque cel (main sprite-pool slot 41 = PE resource 0x583, registered by
 * ar_register_main_sprites) across the FULL screen width to draw two solid
 * bars — first the BOTTOM bar then the TOP bar:
 *
 *   0x48c150:124-142  in_ECX[0x11] (+0x44) = bottom-bar height -> ret 0x48c48a
 *   0x48c150:143-162  in_ECX[0x12] (+0x48) = top-bar    height -> ret 0x48c4fe
 *
 * During the opening-town intro (map 0x3f2 / room 210110) BOTH heights are 64,
 * giving the quirk-#74 letterbox: solid-black rows 0-63 (top) + 416-479
 * (bottom), leaving the central 640x352 cinematic window.  The bars are stable
 * across the whole establishing shot (hold + scripted pan); they are
 * scene-scoped (absent in settled gameplay).
 *
 * The bar-height fields live on the scene-controller object (in_ECX) and are
 * written by the unported 0x5a00c0 cutscene script; until that lands the port
 * drives them as a constant during the establishing shot (PORT-DEBT
 * ingame-letterbox, parallel to the camera-pan trigger stand-in).
 *
 * Per bar: the height is rounded UP to a multiple of the 4px cel height, and
 * each 4px row is tiled at 64px column pitch — 10 columns, dx in {0,64,…,576}
 * (the inner loop runs while (dx+0x80) < 0x281).  The emitted (x,y) stream is
 * verified bit-exact against the retail blit trace (/tmp/blit_town_retail,
 * flip 1500: 160 bottom-bar blits then 160 top-bar blits).
 *
 * Pure C (no Win32 / ddraw): the cel resolve + blit is the caller's sink.
 */
#ifndef OPENSUMMONERS_LETTERBOX_H
#define OPENSUMMONERS_LETTERBOX_H

/* The opening-town intro bar height (top == bottom == 64; quirk #74). */
#define LETTERBOX_INTRO_BAR 0x40

/* Sink: blit the bound letterbox cel (res 0x583, 64x4, opaque) at screen (x,y).
 * Called once per tiled cel, in the engine's exact order (bottom bar first). */
typedef void (*letterbox_blit_fn)(void *ctx, int x, int y);

/* Tile the letterbox bars (0x48c150:124-162).  top_h / bottom_h are the bar
 * heights in pixels (<=0 = that bar absent).  Emits the bottom bar then the top
 * bar; no-op when blit is NULL. */
void letterbox_render(int top_h, int bottom_h, letterbox_blit_fn blit, void *ctx);

#endif /* OPENSUMMONERS_LETTERBOX_H */
