/* engine_pixfmt.h — classify a ddraw surface's pixel format → OSR_PIXFMT_* (M3c).
 * Shared by engine_hooks.h (header fixup) and sheet_grab.h (SHEET pixfmt). */
#ifndef OSS_ENGINE_PIXFMT_H
#define OSS_ENGINE_PIXFMT_H

#include <ddraw.h>
#include "osr_format.h"

static uint8_t eh_pixfmt_of(const DDPIXELFORMAT *pf)
{
    if (pf->dwFlags & DDPF_PALETTEINDEXED8) return OSR_PIXFMT_PAL8;
    if (pf->dwFlags & DDPF_RGB) {
        if (pf->dwRGBBitCount == 16) return OSR_PIXFMT_RGB565;   /* the engine's depth */
        if (pf->dwRGBBitCount == 32) return OSR_PIXFMT_XRGB8888;
    }
    return OSR_PIXFMT_UNKNOWN;
}

#endif /* OSS_ENGINE_PIXFMT_H */
