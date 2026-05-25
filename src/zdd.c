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

    /* FUN_005b8480 primary-surface caps + flags. */
    Z_DDSD_BACKBUFFERCOUNT  = 0x0020,
    Z_DDSCAPS_COMPLEX       = 0x0008,
    Z_DDSCAPS_FLIP          = 0x0010,
    Z_DDSCAPS_PRIMARYSURFACE = 0x0200,

    /* Sentinel "no color key" value, recognised by FUN_005b9830
     * (zdd_object_set_color_key). */
    Z_COLOR_KEY_NONE        = 0x01ffffff,
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

/* ─── ZDDObject surface-alloc stampers + orchestrator ────────────── */

/* The colorkey-sentinel "no key requested" value passed to
 * zdd_object_set_color_key by callers that don't want a transparent
 * color.  Defined here (rather than in the header) because it's only
 * read inside this TU's set_color_key body — no consumer needs the
 * literal anywhere else.  See docs/findings/ddraw-init.md §
 * "FUN_005b95c0" for the original boot call site (FUN_005b8b40 passes
 * pixelFmtFlags = 0x1ffffff for the primary surface). */
#define Z_COLORKEY_SENTINEL  0x1ffffff

void zdd_object_prefill_desc(zdd_object *self,
                             int32_t caps_in, int32_t force_videomem_in)
{
    self->caps_in            = caps_in;
    self->force_videomem_in  = force_videomem_in;

    memset(self->embedded_ddsd, 0, sizeof(self->embedded_ddsd));

    /* Self-pointers into the embedded DDSD.  In retail, written
     * literally as `*in_ECX = in_ECX + 0x15` etc. — i.e., dword indices
     * 0x15/0x10/0x0e from the start of the object, which are byte
     * offsets +0x54/+0x40/+0x38 — the DDSD lpSurface (DDSD offset 0x24),
     * DDSD lPitch (offset 0x10), and DDSD dwHeight (offset 0x08)
     * sub-fields respectively (DDSD itself starts at object offset
     * +0x30). */
    self->ddsd_lpSurface_ref = &self->embedded_ddsd[0x24];
    self->ddsd_pitch_ref     = &self->embedded_ddsd[0x10];
    self->ddsd_height_ref    = &self->embedded_ddsd[0x08];

    /* DDSD dwSize stamp (the first dword of the embedded DDSD). */
    *(uint32_t *)&self->embedded_ddsd[0x00] = 0x7c;
}

void zdd_object_stamp_metrics(zdd_object *self,
                              int32_t p1, int32_t p2, int32_t p3,
                              int32_t p4, int32_t p5, int32_t p6)
{
    /* Issue order matches FUN_005b98c0 byte-for-byte: post-DDSD zero
     * pair first, then the interleaved metric+param writes. */
    self->metric_b0 = 0;
    self->metric_b4 = 0;
    self->metric_0c = p3;
    self->metric_b8 = p5;
    self->metric_bc = p6;
    self->metric_14 = p5;
    self->metric_18 = p6;
    self->metric_10 = p4;
    self->metric_1c = p1;
    self->metric_20 = p2;
}

int zdd_object_set_color_key(zdd_object *self, int32_t key)
{
    self->colorkey_in = key;

    if (key == Z_COLORKEY_SENTINEL) {
        /* "No color key requested" — stamp the sentinel through and
         * clear state_flag.  No vtable call. */
        self->colorkey_out = Z_COLORKEY_SENTINEL;
        self->state_flag   = 0;
        return 1;
    }

    self->state_flag = 0x8000;

    /* 16bpp branch: retail runs the key through FUN_005b8b00 (a pixel-
     * format channel-shifter that picks 16bpp masks off some descriptor
     * pointer in ECX).  The conversion is currently NOT ported — the
     * descriptor object's identity is still unresolved (see
     * docs/findings/ddraw-init.md open thread "FUN_005b8b00").  For boot
     * the orchestrator's caller (FUN_005b8b40 -> FUN_005b95c0) passes
     * the sentinel, so the sentinel branch above wins and this branch
     * never executes at boot.  Once a real consumer needs it, port
     * FUN_005b8b00 and replace the raw key here with its converted
     * value. */
    if (self->parent != NULL && self->parent->pixel_format_bpp == 16) {
        /* key = zdd_pixel_format_convert_16bpp(self->parent, key); */
        /* TODO: pending FUN_005b8b00. */
    }
    self->colorkey_out = key;

    return zdd_surface_set_color_key(self->com_primary, key, self->parent);
}

int zdd_object_create_surface_pair(zdd_object *self,
                                   int32_t p1, int32_t p2, int32_t p3,
                                   int32_t p4, int32_t p5, int32_t p6,
                                   int32_t p7,
                                   uint32_t width, uint32_t height)
{
    zdd_object_prefill_desc(self, p3, p5);

    /* Pass `self->caps_in` (just stamped to p3 by prefill) — matches
     * retail's `*(undefined4 *)(in_ECX + 0xcc)` re-read.  The dword
     * roundtrip is deliberate in the original; we preserve it. */
    if (!zdd_create_surface(self->parent, &self->com_primary,
                            width, height,
                            (uint32_t)self->caps_in, (int)p5)) {
        return 0;
    }

    /* Retail's FUN_005b95c0 calls FUN_005b98c0 with (p1, p2, p6, p7, p8,
     * p9) — i.e., p3/p4/p5 are NOT stamp_metrics inputs.  p8/p9 are
     * `width`/`height` (the surface dimensions, re-passed positionally
     * by retail).  Confirmed by r2 disasm at 0x5b95ff–0x5b9617: stack
     * pushes are eax=p7, ecx=p6, edx=p2, eax=p1 plus already-loaded
     * ebx=p8/width and edi=p9/height.  Earlier port mistakenly passed
     * (p1..p6) which left metric_b8/bc holding the count-flag instead
     * of the surface dimensions — broke the clipper RGNDATA build that
     * reads metric_b8/bc as a RECT bound (DDERR_NOCLIPLIST at runtime). */
    zdd_object_stamp_metrics(self, p1, p2, p6, p7,
                             (int32_t)width, (int32_t)height);
    /* Return the EAX-carry-through value from the last call — retail's
     * implicit return.  This is the SetColorKey success bit (1 for
     * sentinel branch or successful vtable call, 0 for vtable failure). */
    return zdd_object_set_color_key(self, p4);
}

void zdd_build_primary_surface_desc(int mode_arg, int videomem_flag,
                                    zdd_primary_desc_build *out)
{
    /* Modes 0/3/4 share the flippable-primary shape — Full additionally
     * folds videomem_flag into the caps. */
    if (mode_arg == 0 || (mode_arg >= 3 && mode_arg <= 4)) {
        uint32_t caps = (uint32_t)Z_DDSCAPS_PRIMARYSURFACE
                      | (uint32_t)Z_DDSCAPS_FLIP
                      | (uint32_t)Z_DDSCAPS_COMPLEX;        /* 0x218 */
        if (mode_arg == 0 && videomem_flag != 0) {
            caps |= (uint32_t)Z_DDSCAPS_VIDEOMEMORY;        /* 0xa18 */
        }
        out->dwFlags           = (uint32_t)Z_DDSD_CAPS
                               | (uint32_t)Z_DDSD_BACKBUFFERCOUNT; /* 0x21 */
        out->dwCaps            = caps;
        out->dwBackBufferCount = 1;
        return;
    }

    /* Mode 1 (Safe) — non-flippable primary. */
    out->dwFlags           = (uint32_t)Z_DDSD_CAPS;                 /* 0x01 */
    out->dwCaps            = (uint32_t)Z_DDSCAPS_PRIMARYSURFACE;    /* 0x200 */
    out->dwBackBufferCount = 0;
    /* Mode 2 (Windowed) is caller-side — zdd_create_screen never calls
     * this builder for mode 2.  We deliberately don't add a guard
     * (out-of-contract input shouldn't happen). */
}

zdd_object *zdd_object_alloc_and_ctor(zdd *parent)
{
    zdd_object *o = (zdd_object *)calloc(1, sizeof(zdd_object));
    if (o == NULL) return NULL;
    zdd_object_ctor(o, parent);
    return o;
}

int zdd_create_screen(zdd *self,
                      uint32_t width, uint32_t height,
                      uint32_t bpp, int mode_arg, int videomem_flag,
                      const int32_t *opt_rect7)
{
    /* Step 1: tear down any prior screen state (3 ZDDObject slots +
     * com_a primary surface + com_b palette). */
    zdd_release_children(self);

    /* Step 2: stamp parameters on the ZDD.  Retail issue order is
     * preserved exactly (videomem_flag first, then height, then
     * mode_arg, then the zero pair, then width — Ghidra's mov stream
     * is intercalated for instruction-level scheduling but the
     * post-state is what matters). */
    self->videomem_flag     = videomem_flag;
    self->screen_height     = (int32_t)height;
    self->pixel_format_mode = mode_arg;
    self->screen_pos_x      = 0;
    self->screen_pos_y      = 0;
    self->screen_width      = (int32_t)width;

    /* Step 3: copy or zero the 7-int rect blob. */
    if (opt_rect7 != NULL) {
        memcpy(self->screen_rect, opt_rect7, sizeof(self->screen_rect));
    } else {
        memset(self->screen_rect, 0, sizeof(self->screen_rect));
    }

    /* Step 4: primary surface alloc — gated on mode_arg.  Windowed
     * mode (mode 2) skips CreateSurface entirely and just releases
     * any prior primary; all other modes build a per-mode DDSD and
     * call IDirectDraw7::CreateSurface. */
    if (mode_arg == 2) {
        zdd_com_release(&self->com_a);
    } else {
        zdd_primary_desc_build desc;
        zdd_build_primary_surface_desc(mode_arg, videomem_flag, &desc);
        if (!zdd_create_primary_surface(self, &desc)) {
            return 0;
        }
    }

    /* Step 5: stamp bpp and (conditionally) create the 8bpp palette. */
    self->pixel_format_bpp = (int32_t)bpp;
    if (bpp == 8) {
        zdd_setup_8bit_palette(self);
    }

    /* Step 6: allocate the primary ZDDObject. */
    self->primary_obj = zdd_object_alloc_and_ctor(self);
    if (self->primary_obj == NULL) {
        return 0;
    }

    /* Step 7: per-mode ZDDObject wiring.  `success` carries whether
     * the last orchestrator call returned 1; on 0 we fall through to
     * the cleanup path for the most-recently-allocated ZDDObject
     * slot. */
    int success = 0;
    int cleanup_slot = 0;  /* 0=primary_obj, 1=back_obj_a, 2=back_obj_b */

    if (mode_arg == 0) {
        /* Full — back-buffer attach to primary_obj. */
        success = zdd_object_attach_backbuffer(self->primary_obj,
                                               self->com_a,
                                               width, height,
                                               videomem_flag);
        cleanup_slot = 0;
    } else if (mode_arg == 3) {
        /* DB Mode — alloc back_obj_a + back-buffer attach to it, then
         * orchestrator-create primary_obj. */
        self->back_obj_a = zdd_object_alloc_and_ctor(self);
        if (self->back_obj_a == NULL) {
            cleanup_slot = 0;  /* only primary_obj is live */
        } else if (!zdd_object_attach_backbuffer(self->back_obj_a,
                                                 self->com_a,
                                                 width, height, 0)) {
            cleanup_slot = 1;  /* back_obj_a alloc/attach failed */
        } else {
            success = zdd_object_create_surface_pair(
                self->primary_obj,
                (int32_t)width, (int32_t)height, 0,
                Z_COLOR_KEY_NONE, videomem_flag, 0, 0,
                width, height);
            cleanup_slot = 0;  /* primary_obj orchestrator failed */
        }
    } else if (mode_arg == 4) {
        /* Zoom — alloc back_obj_a + back-buffer attach (rect[0/1] =
         * display size), alloc back_obj_b + orchestrator-create
         * (rect[5/6] = source size), then orchestrator-create
         * primary_obj.  Both create_surface_pair calls force VRAM. */
        self->back_obj_a = zdd_object_alloc_and_ctor(self);
        if (self->back_obj_a == NULL) {
            cleanup_slot = 0;
        } else if (!zdd_object_attach_backbuffer(self->back_obj_a,
                                                 self->com_a,
                                                 (uint32_t)self->screen_rect[0],
                                                 (uint32_t)self->screen_rect[1],
                                                 0)) {
            cleanup_slot = 1;
        } else {
            self->back_obj_b = zdd_object_alloc_and_ctor(self);
            if (self->back_obj_b == NULL) {
                cleanup_slot = 2;
            } else if (!zdd_object_create_surface_pair(
                           self->back_obj_b,
                           self->screen_rect[5], self->screen_rect[6], 0,
                           Z_COLOR_KEY_NONE, 1, 0, 0,
                           (uint32_t)self->screen_rect[5],
                           (uint32_t)self->screen_rect[6])) {
                cleanup_slot = 2;
            } else {
                success = zdd_object_create_surface_pair(
                    self->primary_obj,
                    (int32_t)width, (int32_t)height, 0,
                    Z_COLOR_KEY_NONE, 1, 0, 0,
                    width, height);
                cleanup_slot = 0;
            }
        }
    } else {
        /* Mode 1 (Safe) or Mode 2 (Windowed) — orchestrator-create
         * primary_obj.  Both pass videomem_flag through as force_vm. */
        success = zdd_object_create_surface_pair(
            self->primary_obj,
            (int32_t)width, (int32_t)height, 0,
            Z_COLOR_KEY_NONE, videomem_flag, 0, 0,
            width, height);
        cleanup_slot = 0;
    }

    if (success) {
        /* Bind a clipper to the primary ZDDObject's surface.  Then in
         * 16bpp, retail also calls FUN_005b8a20 (pixel-format channel
         * binding) — currently UNPORTED because the ECX identity is
         * ambiguous (see HANDOFF "FUN_005b8b00 16bpp converter" open
         * thread; FUN_005b8a20 is part of the same family).  The
         * boot-path bpp is 16 so this is a live TODO for visible
         * output; the rest of the screen-init plumbing is correct. */
        zdd_object_attach_clipper(self->primary_obj);
        /* if (bpp == 16) zdd_object_bind_pixel_format(...); TODO */
        return 1;
    }

    /* Failure cleanup — release only the slot that just failed.  This
     * mirrors retail exactly: prior slots leak, but the engine's
     * caller (FUN_00582e90) exits the process via FUN_005bf5db(0)
     * after we return 0, so the leak is moot. */
    switch (cleanup_slot) {
        case 0: zdd_obj_destroy(&self->primary_obj); break;
        case 1: zdd_obj_destroy(&self->back_obj_a);  break;
        case 2: zdd_obj_destroy(&self->back_obj_b);  break;
    }
    return 0;
}

int zdd_setup_8bit_palette(zdd *self)
{
    if (!zdd_create_system_palette(self)) {
        return 0;
    }
    /* Retail explicitly null-checks self->com_a before binding —
     * mirror it.  The primary surface may legitimately be absent
     * during teardown / re-init paths. */
    if (self->com_a != NULL) {
        if (!zdd_surface_set_palette(self->com_a, self->com_b, self)) {
            return 0;
        }
    }
    return 1;
}

int zdd_object_attach_backbuffer(zdd_object *self,
                                 void *primary_surface,
                                 uint32_t width, uint32_t height,
                                 int force_videomem)
{
    /* prefill_desc(0, 0) — back-buffer attachments don't go through
     * CreateSurface, so caps_in / force_videomem_in stay zero on this
     * ZDDObject.  The DDSD zero + self-pointer stamp still runs (for
     * downstream Lock() consumers that read +0x00..+0x08). */
    zdd_object_prefill_desc(self, 0, 0);

    /* DDSCAPS2.dwCaps: DDSCAPS_BACKBUFFER (4), OR'd with
     * DDSCAPS_VIDEOMEMORY (0x800) when caller forces vram. */
    uint32_t caps = force_videomem ? 0x804u : 0x004u;

    if (!zdd_get_attached_surface(primary_surface, caps,
                                  &self->com_primary, self->parent)) {
        return 0;
    }

    /* Retail stamps the same params twice (p1=p5=width, p2=p6=height)
     * — see FUN_005b98c0's slot-to-param mapping in zdd.h. */
    zdd_object_stamp_metrics(self,
                             (int32_t)width, (int32_t)height,
                             0, 0,
                             (int32_t)width, (int32_t)height);

    /* Sentinel color-key — back-buffer surfaces don't get one.  The
     * sentinel branch always returns 1, but we still propagate the
     * return value to mirror retail's EAX-carry-through. */
    return zdd_object_set_color_key(self, 0x1ffffff);
}

void zdd_object_attach_clipper(zdd_object *self)
{
    /* Release any existing com_back COM binding — works whether it
     * was a back-buffer surface or a previously-attached clipper, both
     * implement IUnknown. */
    zdd_com_release(&self->com_back);

    /* Create the new clipper.  The Win32 leg stamps com_back directly
     * (matches retail's CreateClipper(0, &this->com_back, NULL) which
     * stores into the +0xac slot). */
    zdd_create_clipper(self->parent, &self->com_back);

    /* Retail unconditionally calls vtable[7] after CreateClipper even
     * if the create failed.  Our port guards with NULL-checks (more
     * defensive than retail; the observable difference is the absence
     * of a crash on a broken DDraw, not a behavioural divergence on
     * the happy path).
     *
     * The clip-list bounding rect is (0, 0, metric_b8, metric_bc) —
     * the surface width/height stamped by stamp_metrics (see
     * orchestrator docstring in zdd.h).  Retail reads those exact two
     * slots at FUN_005b9520:0x5b9531/0x5b953b. */
    if (self->com_back != NULL) {
        zdd_clipper_set_clip_list_rect(self->com_back,
                                       (uint32_t)self->metric_b8,
                                       (uint32_t)self->metric_bc);
    }

    if (self->com_primary != NULL && self->com_back != NULL) {
        zdd_surface_set_clipper(self->com_primary, self->com_back);
    }
}

int zdd_object_new(zdd *parent, zdd_object **out,
                   uint32_t width, uint32_t height,
                   int32_t colorkey, int32_t count)
{
    /* Retail uses operator_new(0xd8) (uninitialized); calloc here is
     * deterministic and the subsequent ctor stamps every observable
     * field, so the difference is invisible. */
    zdd_object *zdo = (zdd_object *)calloc(1, sizeof(zdd_object));
    if (zdo == NULL) {
        return 0;
    }

    zdd_object_ctor(zdo, parent);

    /* FUN_005b8b40 call shape (per docs/findings/ddraw-init.md §
     * "FUN_005b8b40"):  FUN_005b95c0(w, h, 0, colorkey, count, 0, 0, w, h).
     * The p3 = 0 / p6 = 0 / p7 = 0 slots are caller-fixed at this
     * layer; higher-level dispatch (FUN_00582e90) may provide non-zero
     * values when calling the orchestrator directly for other surface
     * shapes. */
    if (!zdd_object_create_surface_pair(zdo,
            /*p1*/ (int32_t)width, /*p2*/ (int32_t)height, /*p3*/ 0,
            /*p4*/ colorkey, /*p5*/ count, /*p6*/ 0, /*p7*/ 0,
            width, height)) {
        zdd_object_dtor(zdo);
        free(zdo);
        return 0;
    }

    *out = zdo;
    return 1;
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

void zdd_hide_cursor(zdd *self)
{
    /* Retail gates on `cursor_state != 0` (currently shown).  Symmetric
     * inverse of zdd_restore_cursor_on_release.  Once we drop to
     * "hidden" the field stays 0 until that helper restores it on
     * release — so back-to-back hide calls are no-ops. */
    if (self->cursor_state != 0) {
        zdd_show_cursor(0);
        self->cursor_state = 0;
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
