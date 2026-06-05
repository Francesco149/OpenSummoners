/*
 * parallax.{c,h} — the in-game PARALLAX far-plane (sky / mountain backdrop).
 *
 * The full-screen background drawn BEHIND the tilemap (golden flip 1800: a blue
 * sky band + a hazy mountain ridge upper-left — the region the tile-only port
 * renders black).  It is NOT a backdrop tile (region A of the runtime grid) and
 * NOT a 0x5a00c0 overlay slice: it has a dedicated producer drawn first in the
 * per-frame world-render driver 0x48c150, before the actor emitters and the
 * tilemap walk FUN_00490f30.
 *
 * Engine correspondence (two producers, identical output — a mutual cross-check):
 *   - FUN_00490cd0 (603 B)  — the INLINE renderer, 0x48c150:47 (the
 *     free-roam / in_ECX[7]==0 path).
 *   - 0x499100 (1118 B) -> FUN_00499560 (271 B) — the helper renderer, the
 *     0x48c150 else branch (in_ECX[7]!=0, via 0x48c6b0) = the
 *     establishing-shot / special path.  0x499100's layer-A loop is
 *     byte-identical to 0x490cd0's; 0x499560 is 0x490cd0's B/C-layer math
 *     extracted to a helper.  parallax_strip() ports 0x499560 verbatim and
 *     parallax_render() composes it the way both producers do (A, then B, then C).
 *
 * THE DESCRIPTOR is the runtime-grid FRONT-HEADER (*(DAT_008a9b50+0x1048), the
 * first 0x1a bytes of the grid map_decode builds).  It is written by the
 * 0x587e00 PROLOGUE's switch on param_2 (= room[0x44]) / param_3 (= room[0x43]) —
 * the front-header bank selection the map_decode port otherwise defers.
 * parallax_select() ports that switch (the parallax-relevant arms); the renderer
 * reads the same fields both engine producers do:
 *
 *   grid byte  field                 used by
 *   0x00 u16   layer A bank          8-tile top strip, frame 0..7 @ x=i*0x50,y=0
 *   0x04 u16   layer C bank          9 tiles, parallax-X 0.5  (factor 500)
 *   0x06 u16   layer C base Y
 *   0x08 u16   layer C wrap          frame = (col0+i) % wrap
 *   0x0c i32   layer C parallax-Y    vertical-parallax factor (0 = none)
 *   0x10 u16   layer B bank          9 tiles, parallax-X 0.25 (factor 0xfa)
 *   0x12 u16   layer B base Y
 *   0x14 u16   layer B wrap
 *   0x18 i32   layer B parallax-Y
 *
 * The TOWN (room 210110, area 0xd2 -> param_2=4 -> switch case 4, param_3=1):
 *   A bank 0x55; C bank 0x58 baseY 0xf8 wrap 8 paraY 0xfa; B bank 0x59 baseY
 *   0xe0 wrap 8 paraY 0.  (Two-witness static RE + area-table derivation;
 *   docs/findings/in-game-intro.md "The PARALLAX far-plane".)
 *
 * Win32-free: the cel select + blit (0x417c40 -> FUN_005b9a40) come in as a
 * parallax_blit_fn callback (the mr_sprite_fn pattern), so the unit is pure and
 * host-tests with synthetic descriptors / cameras.
 */
#ifndef OSS_PARALLAX_H
#define OSS_PARALLAX_H

#include <stdint.h>

#include "map_render.h"   /* mr_camera (the scroll/view fields) */

/* Runtime-grid front-header byte offsets the descriptor occupies (the bytes
 * 0x490cd0 / 0x499560 read; written by parallax_select -> parallax_to_grid). */
#define PX_A_BANK   0x00u
#define PX_C_BANK   0x04u
#define PX_C_BASEY  0x06u
#define PX_C_WRAP   0x08u
#define PX_C_PARAY  0x0cu
#define PX_B_BANK   0x10u
#define PX_B_BASEY  0x12u
#define PX_B_WRAP   0x14u
#define PX_B_PARAY  0x18u

/* One scrolling parallax layer (B or C).  bank == 0 => the layer is absent. */
typedef struct parallax_layer {
    uint16_t bank;
    uint16_t base_y;
    uint16_t wrap;     /* frame wrap (the engine sets 8 when the layer exists) */
    int32_t  para_y;   /* vertical-parallax factor (0 = no vertical parallax)  */
} parallax_layer;

/* The 3-layer parallax descriptor (the grid front-header, decoded). */
typedef struct parallax_desc {
    uint16_t       a_bank;  /* top strip: 8 tiles, no parallax, y = 0          */
    parallax_layer c;       /* 0.5x  horizontal (factor 500)                   */
    parallax_layer b;       /* 0.25x horizontal (factor 0xfa)                  */
} parallax_desc;

/* Per-tile sink: select sprite-bank `bank` frame `frame` and blit it at screen
 * (x, y).  The Win32 layer maps this to 0x417c40(bank,frame) (set current
 * cel into ECX) -> FUN_005b9a40(surface, x, y) (blit it). */
typedef void (*parallax_blit_fn)(void *ctx, uint16_t bank, int32_t frame,
                                 int32_t x, int32_t y);

/*
 * Port of the 0x587e00 prologue's parallax-bank selection (587e00.c:64-196):
 * normalise `param_3` (10/0x28->1, 0x32->2, 0x3c->4, 0x3d->5, else->0), then
 * switch on `param_2` to fill `out` with the per-room layer banks / geometry.
 * Rooms whose param_2 selects no parallax (case 3 / default — the special-render
 * rooms that set in_ECX[0xe]=1) yield an all-zero descriptor.  The HUD/border
 * and autotile writes the same switch performs are out of scope (still deferred);
 * this ports ONLY the parallax fields the two renderers consume.
 */
void parallax_select(int32_t param_2, int32_t param_3, parallax_desc *out);

/* Write / read the descriptor into / from a runtime grid's front-header (the
 * exact bytes the engine producers read at *(DAT_008a9b50+0x1048)).  `grid` must
 * be at least PX_B_PARAY+4 bytes (any map_grid_alloc buffer is). */
void parallax_to_grid(uint8_t *grid, const parallax_desc *d);
void parallax_from_grid(const uint8_t *grid, parallax_desc *out);

/*
 * Render the parallax far-plane through `cam` (FUN_00490cd0 / 0x499100):
 * draw layer A (8 fixed tiles), then layer B (0.25x), then layer C (0.5x), each
 * a horizontal strip whose column offset + frame come from the camera scroll and
 * whose vertical offset is the clamped vertical parallax.  Emits every tile via
 * `blit` (NULL = a dry count).  Returns the number of tiles emitted.
 *
 * Drawn FIRST in the frame (behind the tilemap), matching 0x48c150:47.
 */
int parallax_render(const parallax_desc *d, const mr_camera *cam,
                    parallax_blit_fn blit, void *ctx);

#endif /* OSS_PARALLAX_H */
