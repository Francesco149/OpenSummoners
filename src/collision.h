/*
 * collision.{c,h} — the in-game TILE-COLLISION read-side (Phase-4 chip 2).
 *
 * The town's collision grid is already BUILT: map_decode.c (the FUN_00587e00
 * town arms) deposits, per cell, a region-B record (collision class + slope
 * ref) and a region-D flag — the same dispatch that builds the proven-1:1
 * render grid (region A).  This module ports the READ side that consumes it:
 *
 *   collision_move_vertical  FUN_0054e990  the vertical tile-grid mover
 *                                          (gravity / ground / ceiling clamp)
 *
 * The directional AI probes (0x441ae0 / 0x47dbb0) and the horizontal
 * mover land with the controllable character (chip 3), where they get a LIVE
 * grounded mover to validate against; chip 2 ports + host-tests the vertical
 * mover (it is a PURE function of the grid + the body box + a delta — its
 * decompiled `in_ECX` is the grid base, not the actor).
 *
 * SLOPES: a sloped cell's region-B +0x8 holds an engine .rdata VA (the
 * 0x5cc410 / 0x5cc430 height profiles); the mover reads a per-sub-column height
 * byte from it.  The town street is FLAT (every collision cell has slope ref 0),
 * so the resolver is not needed here and is taken as a caller callback — the
 * live resolver for the real profiles is PORT-DEBT(collision-slopes).
 */
#ifndef OSS_COLLISION_H
#define OSS_COLLISION_H

#include <stdint.h>

/* The physics-body box the collision read-side operates on (the actor's body
 * sub-struct fields body+4 / +8 / +0xc / +0x10).  X is the DIM0 axis (the
 * grid's 0x80-pitch / worldX); Y is DIM1 (worldY). */
typedef struct {
    int32_t x;       /* body+4   world X */
    int32_t y;       /* body+8   world Y */
    int32_t width;   /* body+0xc */
    int32_t height;  /* body+0x10 */
} phys_box;

/* Slope-profile resolver.  `slope_ref` is the raw region-B +0x8 u32 (0 = flat,
 * never passed here); `subtile_x` is 0..31 (= 0xc80/100 columns within a cell).
 * Returns the profile's height byte (0..255).  NULL is allowed when no cell in
 * range is sloped (the town). */
typedef int (*coll_slope_fn)(void *ctx, uint32_t slope_ref, int subtile_x);

/* FUN_0054e990 — sweep `box`'s world-Y by `delta` (>=0 down, <0 up) in
 * <=100-unit steps, scanning the X-extent cells against region B (class 10 =
 * solid wall, class 1 = slope surface).  Does NOT mutate `box`; writes the
 * resulting (clamped) world-Y to *out_y (initialised to box->y, so it is always
 * defined — the decompile leaves it to the caller's local on an upward block).
 * Returns 1 if the whole delta was clear, 0 if a wall/slope clamped it.
 * `pass_slopes` = the 54e990 param_7 (moving down, !=0 passes through slope
 * surfaces).  `slope`/`ctx` resolve sloped cells (unused when all in-range
 * cells are flat). */
int collision_move_vertical(const uint8_t *grid, const phys_box *box,
                            int32_t delta, int pass_slopes,
                            coll_slope_fn slope, void *ctx,
                            int32_t *out_y);

#endif /* OSS_COLLISION_H */
