/*
 * color_grade.{c,h} — the in-game palette COLOR-GRADE LUT.
 *
 * The retail engine renders the in-game world a touch DARKER and MORE
 * SATURATED than the raw sprite palettes: a 256-entry per-channel tone-curve
 * LUT is applied to every sprite palette's R/G/B bytes before the cel is
 * blitted.  The title / new-game / prologue scenes do NOT apply it (their
 * blits go through the plain frame getter FUN_00418470), which is why those
 * are bit-exact without it; the in-game render paths do:
 *
 *   - FUN_00417c40 (the palette-aware frame getter) — used by the parallax
 *     far-plane producer (FUN_00490cd0 / FUN_00499560).
 *   - FUN_00490f30 (the tilemap walk) — applies the same LUT inline
 *     (490f30.c:75-213) to each tile's palette.
 *
 * Both gate the LUT on  (DAT_008a9510 != 0 || DAT_008a9514 != 1000)  and, when
 * armed, remap every palette channel value v -> LUT[v].
 *
 * THE BUILDER (FUN_00562ea0 @ 0x5639fd-0x563a70, the boot init; verified
 * against a live retail probe of DAT_008a9410 bit-for-bit at the three sampled
 * indices).  Two config gates drive it:
 *   gate1 = *(*DAT_008a6e80 + 0x130)   ("brightness", in-game default 700)
 *   gate2 = *(*DAT_008a6e80 + 0x14c)   ("contrast",   in-game default 850)
 * and for each index i in 0..255:
 *   q      = (i * gate2) / 1000;                       // integer divide
 *   accum  = ( (1000 - gate1) * q
 *            + (1.0 - cos(q * PI/255)) * 127.5 * gate1 ) * 0.001;
 *   LUT[i] = (uint8_t) min(255.0, accum);              // x87 ftol = truncate
 * The doubles 1.0 / 127.5 / 0.001 / 255.0 / (PI, 1/255) are the .rdata
 * constants the builder reads (0x850dc8 / 0x850dc0 / 0x850d98 / 0x850db8 /
 * 0x5cc288, 0x850dd0).  gate1=0 && gate2=1000 yields the identity LUT[i]==i.
 *
 * Live-verified samples (gate1=700, gate2=850):
 *   LUT[0]=0  LUT[64]=35  LUT[128]=100  LUT[192]=175  LUT[255]=233.
 *
 * Win32-free + pure; host-tested in tests/test_color_grade.c.  The town's
 * gates (700/850) are read live; deriving them from the launcher config /
 * god-object (DAT_008a6e80+0x130/+0x14c) is PORT-DEBT(color-grade-gates).
 */
#ifndef OSS_COLOR_GRADE_H
#define OSS_COLOR_GRADE_H

#include <stdint.h>

/* The in-game default gates (live-probed at the town intro; the launcher's
 * brightness/contrast config can override them — PORT-DEBT color-grade-gates). */
#define COLOR_GRADE_TOWN_GATE1 700
#define COLOR_GRADE_TOWN_GATE2 850

/*
 * Build the 256-entry tone-curve LUT for the given gates (FUN_00562ea0's
 * loop, verified bit-exact).  gate1=0 && gate2=1000 => the identity LUT.
 */
void color_grade_build_lut(uint8_t lut[256], int gate1, int gate2);

/*
 * 1 iff this gate pair arms the LUT (matches the engine's
 * `DAT_008a9510 != 0 || DAT_008a9514 != 1000` gate); 0 => identity, skip.
 */
int color_grade_is_active(int gate1, int gate2);

/*
 * Apply the LUT to a packed RGBQUAD palette in place: for each of `n_entries`
 * 4-byte entries, remap the first three bytes (R,G,B) through `lut`, leaving
 * the 4th (reserved/alpha) untouched — exactly FUN_00417c40's per-channel
 * palette remap (417c40.c:259-273).  `palette` must hold n_entries*4 bytes.
 */
void color_grade_apply_palette(uint8_t *palette, int n_entries,
                               const uint8_t lut[256]);

#endif /* OSS_COLOR_GRADE_H */
