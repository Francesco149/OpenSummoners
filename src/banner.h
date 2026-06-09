/*
 * banner.{c,h} — the AREA-TITLE BANNER (the "Town of Tonkiness" card).
 *
 * A self-contained overlay producer, like scene_fade.{c,h} / letterbox.c.  RE'd
 * from the retail decompile + live ground truth (ckpt 100; engine-quirk #96;
 * findings/in-game-intro.md "The area-title BANNER"):
 *   - renderer  0x494a60 (918 B) — called 3x/frame from the world driver
 *               0x48c150:176-178, ECX = *(view+0x11c/0x120/0x124) (3 banner
 *               slots), AFTER the scene-fade reveal grid 0x48e920.  Only slot0
 *               is the area card; the other two stay disabled in the town.
 *   - animation 0x499ab0 — the cinematic step (once/sim-tick = every 2
 *               flips; the SAME fn that advances the scene_fade grid): the
 *               mode-1 phase machine 0 compose -> 1 fade-in -> 2 hold -> 3
 *               fade-out.
 *
 * mode 1 (the only ported mode) = the scroll SPRITE (res 0x449, slot 53 / the
 * 0x8a7714 bank, registered by ar_register_palette_ramps) with the area name
 * GDI-composed onto it ONCE, blitted at a hard-coded (160,64); alpha<1000 ->
 * alpha blit (0x494e10 / 0x5bd550, ramp_b), alpha==1000 -> keyed blit
 * (0x5b9b70).  modes 2/3/4 (the separate sprite-pool 0x8a7718 banner) are
 * unused for the town and not ported.
 *
 * Pure C (no Win32 / ddraw): the cel resolve + GDI text compose + blit are the
 * caller's sink (main.c game_render_banner), like scene_fade.c.
 */
#ifndef OPENSUMMONERS_BANNER_H
#define OPENSUMMONERS_BANNER_H

#include <stdint.h>

#define BANNER_MODE_TEXT   1       /* in_ECX[0]: GDI-text card (the area title) */
#define BANNER_DST_X       0xa0    /* 160 — hard-coded blit x (0x494a60)    */
#define BANNER_DST_Y       0x40    /* 64                                        */
#define BANNER_HOLD_DUR    400     /* in_ECX[4] (live)                          */
#define BANNER_ALPHA_STEP  0x14    /* +-20 / sim-tick (0x499ab0)            */
#define BANNER_ALPHA_MAX   1000

/* The banner state object (mirrors the retail object at *(view+0x11c)). */
typedef struct {
    int32_t  mode;       /* [0]   : 1 = GDI-text card (2/3/4 sprite, unported) */
    uint16_t phase;      /* [1].lo: 0 compose / 1 fade-in / 2 hold / 3 fade-out */
    uint16_t hold_ctr;   /* [1].hi: counts to hold_dur in phase 2              */
    int32_t  alpha;      /* [2]   : 0..1000                                    */
    int32_t  hold_dur;   /* [4]   : frames to hold before fade-out (400)       */
    int32_t  enable;     /* [8]   : on/off                                     */
    int32_t  composed;   /* port  : area name GDI-composed onto the cel yet?   */
    char     text[64];   /* the area name ("Town of Tonkiness")               */
} area_banner;

/* Arm the area-title card: mode=1, phase=0, alpha=0, hold_ctr=0, enable=1.
 * The retail arm (the scene script) also sets the text + hold_dur; the port
 * passes them here. */
void banner_arm(area_banner *b, const char *text, int32_t hold_dur);

/* One sim-tick update (0x499ab0 mode-1 phase machine).  No-op when off.
 * phase 0 falls through to the phase-1 fade-in on the same tick (as retail). */
void banner_step(area_banner *b);

/* True while the card should render (enable && mode==1). */
int  banner_active(const area_banner *b);

/* The GDI text layout (0x494a60 length->advance/y math), pure.  x is
 * centred on 160 by the string length; y carries a small length-keyed offset.
 * The town name "Town of Tonkiness" (len 17) -> advance 10, y_off 4, x_base 75
 * (matching the live textout: shadow x 73-76, y 13-15). */
typedef struct {
    int len;       /* strlen                                                  */
    int advance;   /* iVar4 — per-char x step used for centring               */
    int y_off;     /* local_18 — extra y offset                               */
    int x_base;    /* iVar7 = 160 - (len*advance)/2 — the white main-draw x   */
} banner_layout;
banner_layout banner_text_layout(const char *text);

/* alpha (0..1000) -> ramp_b index (0..19) for the fade-in alpha blit, or -1
 * meaning "fully opaque -> keyed blit" (0x494a60: idx = alpha*0x14/1000;
 * idx > 0x13 => keyed).  The caller still keyed-blits when ramp_b[idx]==NULL. */
int banner_alpha_ramp_index(int32_t alpha);

#endif /* OPENSUMMONERS_BANNER_H */
