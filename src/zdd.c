/*
 * src/zdd.c — pure-logic port of the ZDD ctor/dtor/log path.
 *
 * Win32-free: every external touch (ShowCursor, OutputDebugStringA,
 * IUnknown::Release, ZDDObject teardown, DirectDrawCreateEx,
 * SetCooperativeLevel) goes through a primitive declared in zdd.h
 * and defined in either zdd_win32.c (real build) or
 * tests/test_zdd.c (host build).
 *
 * Provenance: each function block carries the FUN_xxxxxx address it
 * ports.  Strings used in the log builder were verified against the
 * PE data section (r2 pszj at each address) — the comma-instead-of-
 * period quirks in "Warning,exists ZDD errors," and " failed,Error
 * Code " are retail's, not typos.
 */
#include "zdd.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ─── DDSURFACEDESC2 builder ─────────────────────────────────────── */

/* Pixel-format constants — names mirrored to Win32 DDraw macros.
 * Defined locally so this TU compiles host-side without <ddraw.h>. */
enum {
    Z_DDSD_CAPS         = 0x0001,
    Z_DDSD_HEIGHT       = 0x0002,
    Z_DDSD_WIDTH        = 0x0004,
    Z_DDSD_PIXELFORMAT  = 0x1000,

    Z_DDSCAPS_OFFSCREENPLAIN = 0x0040,
    Z_DDSCAPS_VIDEOMEMORY    = 0x0800,

    Z_DDPF_RGB              = 0x0040,
    Z_DDPF_PALETTEINDEXED8  = 0x0020,
};

void zdd_build_surface_desc(const zdd *self, zdd_surface_desc_build *out,
                            uint32_t width, uint32_t height,
                            uint32_t caps_base, int force_videomem)
{
    /* The retail body zeros a 124-byte stack DDSURFACEDESC2 then
     * stamps fields in issue order.  We pre-zero the build struct
     * for the same shape, then overlay. */
    memset(out, 0, sizeof(*out));

    out->dwFlags  = Z_DDSD_CAPS | Z_DDSD_HEIGHT | Z_DDSD_WIDTH;
    out->dwHeight = height;
    out->dwWidth  = width;
    out->dwCaps   = caps_base | Z_DDSCAPS_OFFSCREENPLAIN;

    if (force_videomem != 0 || self->videomem_flag != 0) {
        out->dwCaps |= Z_DDSCAPS_VIDEOMEMORY;
    }

    if (self->pixel_format_mode != 2) {
        return;
    }

    /* Explicit-pixel-format branch. */
    out->dwFlags         |= Z_DDSD_PIXELFORMAT;
    out->has_pixel_format = 1;
    out->ddpf_dwSize      = 0x20;
    out->ddpf_dwFlags     = Z_DDPF_RGB;

    switch (self->pixel_format_bpp) {
    case 8:
        /* 8bpp: PALETTEINDEXED8.  RGBBitCount stays 0 (DDraw infers
         * from palette size).  Masks stay 0. */
        out->ddpf_dwFlags = Z_DDPF_RGB | Z_DDPF_PALETTEINDEXED8;
        break;
    case 16:
        out->ddpf_dwRGBBitCount = 16;
        out->ddpf_dwRBitMask    = 0xF800u;  /* RGB565 */
        out->ddpf_dwGBitMask    = 0x07E0u;
        out->ddpf_dwBBitMask    = 0x001Fu;
        break;
    case 24:
    case 32:
        /* Retail's switch literally lists case 0x18 + case 0x20
         * together — fall-through.  See docs/findings/ddraw-init.md
         * "Engine quirk" note (DDraw ignores the masks at 24/32 bpp
         * but the engine sets them anyway). */
        out->ddpf_dwRGBBitCount = (uint32_t)self->pixel_format_bpp;
        out->ddpf_dwRBitMask    = 0xFF0000u;
        out->ddpf_dwGBitMask    = 0x00FF00u;
        out->ddpf_dwBBitMask    = 0x0000FFu;
        break;
    default:
        /* Any other bpp: leaves dwRGBBitCount = 0 and masks = 0.
         * This case isn't reached at boot but is defensive. */
        break;
    }
}

/* ─── ZDDObject lifecycle ────────────────────────────────────────── */

void zdd_object_ctor(zdd_object *self, zdd *parent)
{
    /* Retail's FUN_005b9350 only writes the 6 lifecycle-relevant
     * fields + bumps parent->open_objects.  Pre-zero the whole struct
     * so the unmodelled regions (embedded DDSD, window-fit metrics)
     * start at a deterministic 0 instead of operator_new garbage. */
    memset(self, 0, sizeof(*self));
    self->parent = parent;
    parent->open_objects++;
}

void zdd_object_release_pixel_buf(zdd_object *self)
{
    if (self->pixel_buf != NULL) {
        zdd_object_local_free(self->pixel_buf);
        self->pixel_buf      = NULL;
        self->pixel_buf_flag = 0;
    }
}

void zdd_object_dtor(zdd_object *self)
{
    /* Retail issue order: pixel buffer first, then com_back (+0xac),
     * then com_primary (+0x2c), then parent counter decrement. */
    zdd_object_release_pixel_buf(self);
    zdd_com_release(&self->com_back);
    zdd_com_release(&self->com_primary);
    if (self->parent != NULL) {
        self->parent->open_objects--;
    }
}

/* zdd_obj_destroy — full ZDDObject teardown + heap free.  Replaces
 * the placeholder that used to just free() the allocation. */
void zdd_obj_destroy(zdd_object **obj_pp)
{
    if (obj_pp == NULL || *obj_pp == NULL) return;
    zdd_object_dtor(*obj_pp);
    free(*obj_pp);
    *obj_pp = NULL;
}

/* ─── ctor ───────────────────────────────────────────────────────── */

/* FUN_005b7f80 — in-place ctor.  Zero-fills the whole struct first
 * (retail leaves pad uninit; we don't, for determinism) then stamps
 * the cursor_state = 1 invariant.  log_buf stays all-zero. */
void zdd_ctor(zdd *self)
{
    memset(self, 0, sizeof(*self));
    self->cursor_state = 1;
}

/* ─── DDERR table ────────────────────────────────────────────────── */

/* HRESULT → DDERR name.  18 entries — the exact set FUN_005b80d0's
 * switch ladder recognises.  Order doesn't affect output (we linear-
 * scan); kept ascending-by-HRESULT here for readability.
 *
 * The literal hex values are HRESULTs as 32-bit unsigned; we compare
 * as int32_t (sign-extended from the source DDERR_* macros).  This is
 * what retail does — the disasm uses signed compares on the negative
 * HRESULT magic numbers (e.g. -0x7789ff1a = 0x887604e6). */
static const struct {
    int32_t     hresult;
    const char *name;
} k_dderr_table[] = {
    /* Windows-codespace errors (re-aliased as DDERR_* by the engine). */
    { (int32_t)0x80070057, "DDERR_INVALIDPARAMS"            }, /* E_INVALIDARG */
    { (int32_t)0x8007000e, "DDERR_OUTOFMEMORY"              }, /* E_OUTOFMEMORY */

    /* DirectDraw codespace (0x88760xxx). */
    { (int32_t)0x88760104, "DDERR_NOOVERLAYHW"              },
    { (int32_t)0x88760233, "DDERR_NODIRECTDRAWHW"           },
    { (int32_t)0x88760234, "DDERR_PRIMARYSURFACEALREADYEXIST" },
    { (int32_t)0x88760235, "DDERR_NOEMULATION"              },
    { (int32_t)0x8876024e, "DDERR_UNSUPPORTEDMODE"          },
    { (int32_t)0x8876024f, "DDERR_NOMIPMAPHW"               },
    { (int32_t)0x88760354, "DDERR_NOZBUFFERHW"              },
    { (int32_t)0x8876037c, "DDERR_OUTOFVIDEOMEMORY"         },
    { (int32_t)0x8876045f, "DDERR_INCOMPATIBLEPRIMARY"      },
    { (int32_t)0x88760464, "DDERR_INVALIDCAPS"              },
    { (int32_t)0x88760482, "DDERR_INVALIDOBJECT"            },
    { (int32_t)0x88760491, "DDERR_INVALIDPIXELFORMAT"       },
    { (int32_t)0x887604b4, "DDERR_NOALPHAHW"                },
    { (int32_t)0x887604d4, "DDERR_NOCOOPERATIVELEVELSET"    },
    { (int32_t)0x887604e1, "DDERR_NOEXCLUSIVEMODE"          },
    { (int32_t)0x887604e6, "DDERR_NOFLIPHW"                 },
};

const char *zdd_dderr_name(int32_t hresult)
{
    size_t n = sizeof(k_dderr_table) / sizeof(k_dderr_table[0]);
    for (size_t i = 0; i < n; i++) {
        if (k_dderr_table[i].hresult == hresult) {
            return k_dderr_table[i].name;
        }
    }
    return NULL;
}

/* ─── log builder ────────────────────────────────────────────────── */

/* Lowercase-hex render of `value` (treated as unsigned 32-bit) into
 * `out`.  Matches FUN_005c0907(.., .., 0x10) — no "0x" prefix,
 * 'a'..'f' digit chars, no zero-padding.  `out` must hold ≥ 9 bytes
 * (8 digits + NUL). */
static void hex32_lower(uint32_t value, char *out)
{
    /* Build digits least-significant-first into a scratch then reverse,
     * the same shape as FUN_005c0934 — except we emit 0 as a single
     * '0' digit (matches retail's do-while). */
    char tmp[16];
    int  n = 0;
    do {
        unsigned d = value % 16u;
        tmp[n++] = (char)((d < 10) ? ('0' + d) : ('a' + d - 10));
        value /= 16u;
    } while (value != 0);
    for (int i = 0; i < n; i++) {
        out[i] = tmp[n - 1 - i];
    }
    out[n] = '\0';
}

/* Bounded strcat — appends `src` to `dst` (which is a NUL-terminated
 * C string fitting in `dst_cap` bytes including the terminator),
 * stopping at the buffer cap.  Always leaves `dst` NUL-terminated.
 *
 * Retail's FUN_005b80d0 is plain strcat — no bound check.  Our buffer
 * is 256 bytes and the longest worst-case message ("Warning,exists
 * ZDD errors,XXXX.SetCooperativeLevel failed,Error Code 88760xxx
 * DDERR_PRIMARYSURFACEALREADYEXIST.\n") fits comfortably under 100
 * chars.  The cap is purely a "we don't trust the inputs" guard. */
static void buf_append(char *dst, size_t dst_cap, const char *src)
{
    if (src == NULL) return;
    size_t len = strlen(dst);
    while (len + 1 < dst_cap && *src != '\0') {
        dst[len++] = *src++;
    }
    dst[len] = '\0';
}

/* FUN_005b80d0 — render + flush DDERR-decorated log message. */
void zdd_log_dderr(zdd *self, const char *prefix1, const char *prefix2,
                   int32_t hresult)
{
    char hex_buf[16];
    const char *dderr = zdd_dderr_name(hresult);

    /* Build into self->log_buf in retail's order (strcpy then 6
     * strcats).  Hex render is unsigned 32-bit. */
    self->log_buf[0] = '\0';
    buf_append(self->log_buf, sizeof(self->log_buf),
               "Warning,exists ZDD errors,");
    buf_append(self->log_buf, sizeof(self->log_buf),
               prefix1 ? prefix1 : "");
    buf_append(self->log_buf, sizeof(self->log_buf), ".");
    buf_append(self->log_buf, sizeof(self->log_buf),
               prefix2 ? prefix2 : "");
    buf_append(self->log_buf, sizeof(self->log_buf),
               " failed,Error Code ");

    hex32_lower((uint32_t)hresult, hex_buf);
    buf_append(self->log_buf, sizeof(self->log_buf), hex_buf);

    if (dderr != NULL) {
        buf_append(self->log_buf, sizeof(self->log_buf), dderr);
    }
    buf_append(self->log_buf, sizeof(self->log_buf), ".\n");

    zdd_output_debug_string(self->log_buf);
}

/* ─── dtor pieces ────────────────────────────────────────────────── */

void zdd_restore_cursor_on_release(zdd *self)
{
    if (self->cursor_state == 0) {
        zdd_show_cursor(1);
        self->cursor_state = 1;
    }
}

void zdd_release_children(zdd *self)
{
    /* Retail issue order — primary first, two back-buffer children
     * next, two opaque COM handles last.  Each primitive is a no-op
     * on NULL and is responsible for zeroing the field. */
    zdd_obj_destroy(&self->primary_obj);
    zdd_obj_destroy(&self->back_obj_a);
    zdd_obj_destroy(&self->back_obj_b);
    zdd_com_release(&self->com_a);
    zdd_com_release(&self->com_b);
}

void zdd_dtor(zdd *self)
{
    zdd_restore_cursor_on_release(self);
    zdd_release_children(self);

    if (self->ddraw7 != NULL) {
        zdd_com_release(&self->ddraw7);
    }

    if (self->open_objects != 0) {
        zdd_output_debug_string("Warning,exists ZDD objects.\n");
    }
    if (self->log_buf[0] != '\0') {
        zdd_output_debug_string(self->log_buf);
    }
}

/* ─── high-level driver ──────────────────────────────────────────── */

int zdd_create(zdd **out)
{
    zdd *p = (zdd *)calloc(1, sizeof(zdd));
    if (p == NULL) {
        return 0;
    }
    zdd_ctor(p);
    if (!zdd_directdraw_create_ex(p)) {
        zdd_dtor(p);
        free(p);
        return 0;
    }
    *out = p;
    return 1;
}

void zdd_destroy(zdd *self)
{
    if (self == NULL) return;
    zdd_dtor(self);
    free(self);
}
