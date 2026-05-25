/*
 * src/zdd.h — ZDD (DirectDraw 7 wrapper) port.
 *
 * Ports the leaf functions of the "ZDDCore" class that owns the
 * IDirectDraw7 device + per-surface ZDDObject children:
 *
 *   - FUN_005b7f80  zdd_ctor                     in-place ctor, zeros
 *                                                 fields + empty log buf
 *   - FUN_005b7fe0  zdd_dtor                     release ddraw7 + children
 *   - FUN_005b8040  zdd_release_children          inner "drop the COM kids"
 *                                                 piece (5 slots in order:
 *                                                 +0x16c, +0x18, +0x1c,
 *                                                 +0x128, +0x12c)
 *   - FUN_005b8da0  zdd_restore_cursor_on_release ShowCursor restore if
 *                                                 the engine had hidden
 *                                                 it (cursor_state == 0)
 *   - FUN_005b80d0  zdd_log_dderr                builds + flushes a
 *                                                 DDERR-decorated log
 *                                                 message into self->log_buf
 *   - FUN_005b88c0  zdd_directdraw_create_ex     DirectDrawCreateEx wrapper
 *   - FUN_005b89d0  zdd_set_coop_level           SetCooperativeLevel wrapper
 *   - FUN_005b7ee0  zdd_create                   top-level "new ZDD + init"
 *                                                 driver
 *   - FUN_005b97e0  zdd_object_prefill_desc      ZDDObject DDSD pre-fill +
 *                                                 self-pointer stamp (also
 *                                                 caches caps_in / force_vm_in)
 *   - FUN_005b98c0  zdd_object_stamp_metrics     ZDDObject window-fit
 *                                                 metric stash (10 slots)
 *   - FUN_005b9830  zdd_object_set_color_key     ZDDObject SetColorKey
 *                                                 wrapper (0x1ffffff sentinel
 *                                                 means "no color key")
 *   - FUN_005b95c0  zdd_object_create_surface_pair
 *                                                 surface-alloc orchestrator
 *                                                 over the 4 helpers above
 *
 * Provenance: see `docs/findings/ddraw-init.md` for the call graph
 * + vtable cheat-sheet.  Per-string lookups confirmed against the PE
 * data section (r2 pszj at each DAT_xxx address — see PROGRESS notes
 * for the table).
 *
 * Split style mirrors bitmap_session.h / wnd_proc.h: pure logic in
 * zdd.c, Win32 primitives extern'd here and defined in zdd_win32.c
 * (real build) or test_zdd.c (host build).  No <windows.h> in this
 * header — COM interface pointers are `void *` so the host can
 * compile and the real build casts at the wrapper boundary.
 */
#ifndef OPENSUMMONERS_ZDD_H
#define OPENSUMMONERS_ZDD_H

#include <stddef.h>
#include <stdint.h>

/* Forward declarations so the struct definitions below can refer to
 * each other.  The full ZDDObject shape sits further down — only the
 * fields the leaf lifecycle (FUN_005b9350 / _9390 / _93e0) touches
 * are pinned; everything else is opaque pad to preserve the 0xd8
 * size literal-stamped in operator_new arguments. */
typedef struct zdd        zdd;
typedef struct zdd_object zdd_object;

/* Field layout inferred from FUN_005b7f80 (ctor writes), FUN_005b7fe0
 * (dtor reads), and FUN_005b8040 (5-slot release loop).  Size 0x170 is
 * the literal `operator_new(0x170)` argument in FUN_005b7ee0. */
struct zdd {
    /* +0x00..+0x17: not observed in any decompiled ZDD-class method.
     * Likely the C++ vtable pointer (+0x00) plus unidentified fields
     * — kept as opaque pad so total size matches retail. */
    uint8_t       _pad000[0x18];

    /* +0x18: child ZDDObject pointer.  Released in FUN_005b8040's
     * second slot (FUN_005b9390 cleanup + FUN_005bef0e heap free).
     * Role within the front/back/whatever triple is not yet pinned —
     * see open-RE thread in HANDOFF.md when ZDDObject lands. */
    zdd_object   *back_obj_a;                /* +0x18 */

    /* +0x1c: third child ZDDObject pointer.  Released in FUN_005b8040's
     * third slot.  Same opaque shape as back_obj_a. */
    zdd_object   *back_obj_b;                /* +0x1c */

    /* +0x20..+0x11f: 256-byte scratch buffer for log messages.
     * FUN_005b7f80 inits to "" (1-byte strcpy of DAT_008a9b6c, the
     * empty BSS string).  FUN_005b80d0 strcpy's a prefix then strcat's
     * the rest in-place + OutputDebugStringA's it.  FUN_005b7fe0 (dtor)
     * flushes one final time iff log_buf[0] != 0.
     *
     * Note: retail's ctor only writes the first byte (the '\0'); the
     * remaining 255 bytes are whatever operator_new returned.  Our port
     * zero-inits the whole buffer in zdd_ctor — same observable
     * behaviour from the C-string layer's perspective. */
    char          log_buf[0x100];            /* +0x20..+0x11f */

    /* +0x120: "open objects" counter.  Incremented by the (not-yet-
     * ported) surface-create paths in FUN_005b95c0 et al; dtor warns
     * via OutputDebugStringA if non-zero at destruction time. */
    int32_t       open_objects;              /* +0x120 */

    /* +0x124: IDirectDraw7 interface pointer.  Owned — Release'd by
     * zdd_dtor.  Set by zdd_directdraw_create_ex. */
    void         *ddraw7;                    /* +0x124 */

    /* +0x128, +0x12c: two more COM interface pointers released via the
     * IUnknown::Release path (vtable+8).  Roles unknown — likely
     * IDirectDrawPalette + IDirectDrawClipper or similar, set by
     * later-bound calls.  Released in FUN_005b8040's 4th and 5th
     * slots. */
    void         *com_a;                     /* +0x128 */
    void         *com_b;                     /* +0x12c */

    /* +0x130: cursor visibility state.  ctor inits to 1 ("shown").
     * FUN_005b8da0 calls ShowCursor(TRUE) + sets back to 1 IFF the
     * field is currently 0 — meaning some other (unported) path
     * flips it to 0 when entering fullscreen / hiding cursor for
     * gameplay, and the dtor restores it. */
    int32_t       cursor_state;              /* +0x130 */

    /* +0x134: "force video memory" flag.  FUN_005b8c00 OR's
     * DDSCAPS_VIDEOMEMORY (0x800) into ddsCaps.dwCaps when this is
     * non-zero OR when its caller-supplied param_5 (force_videomem)
     * is non-zero.  Not written by anything we've ported yet — read
     * but unwritten until the higher-level mode-dispatch
     * (FUN_00582e90) lands. */
    int32_t       videomem_flag;             /* +0x134 */

    /* +0x138..+0x163: unobserved by ZDD-class methods we've decomp'd
     * so far.  Pad. */
    uint8_t       _pad138[0x164 - 0x138];

    /* +0x164, +0x168: pixel-format hints used by FUN_005b8c00 when
     * building a DDSURFACEDESC2.  Mode 2 means "explicit pixel format";
     * bpp is then 8/16/24/32.  See docs/findings/ddraw-init.md
     * "FUN_005b8c00" section.  Not written by anything we've ported
     * yet — declared here only to keep struct size correct. */
    int32_t       pixel_format_mode;         /* +0x164 */
    int32_t       pixel_format_bpp;          /* +0x168 */

    /* +0x16c: the primary ZDDObject (the screen).  Released first in
     * FUN_005b8040's 5-slot loop.  Set by the mode-dispatch
     * CreateScreen path (FUN_00582e90 → FUN_005b8b40). */
    zdd_object   *primary_obj;               /* +0x16c */
};

/* Field layout inferred from FUN_005b9350 (ctor writes 6 fields +
 * parent->open_objects bump), FUN_005b9390 (dtor releases 2 COM
 * children + decrements parent->open_objects), FUN_005b93e0
 * (pixel-buffer release: LocalFree + clear two fields), FUN_005b97e0
 * (DDSD pre-fill: 3 self-pointers, 124-byte zero, dwSize stamp, 2
 * caller-arg caches at +0xcc/+0xd0), FUN_005b98c0 (metric stash: 10
 * dwords spread across the pre-DDSD and post-DDSD metric regions),
 * and FUN_005b9830 (colorkey storage at +0x24/+0x28 + state_flag at
 * +0xd4).  Size 0xd8 is inferred from the operator_new arg in
 * FUN_005b8b40's invocation (per docs/findings/ddraw-init.md). */
struct zdd_object {
    /* +0x00..+0x08: three self-pointers stamped by FUN_005b97e0 into
     * specific sub-fields of the embedded DDSURFACEDESC2.  Likely a
     * vestigial "scratch table" so the Lock() path can grab
     * lpSurface/pitch/height without re-walking the DDSD layout:
     *   ddsd_lpSurface_ref → &embedded_ddsd[0x24]  (DDSD lpSurface)
     *   ddsd_pitch_ref     → &embedded_ddsd[0x10]  (DDSD lPitch)
     *   ddsd_height_ref    → &embedded_ddsd[0x08]  (DDSD dwHeight) */
    void     *ddsd_lpSurface_ref;            /* +0x00 */
    void     *ddsd_pitch_ref;                /* +0x04 */
    void     *ddsd_height_ref;               /* +0x08 */

    /* +0x0c..+0x23: six dwords stamped by FUN_005b98c0 from the
     * orchestrator's caller params (window-fit metrics).  The +0x0c
     * slot is also the byte 97e0 sets to DDSD-dwSize-0x7c — 98c0 runs
     * after FUN_005b8c00 has already consumed the DDSD, so it
     * overwrites the dwSize stamp without ill effect.  Slot-to-98c0-
     * param mapping (98c0 itself takes params in (p1,p2,p3,p4,p5,p6)
     * order, but writes them out of order):
     *   metric_0c ← 98c0 param_3   (also the 0x7c dwSize stamp from 97e0)
     *   metric_10 ← 98c0 param_4
     *   metric_14 ← 98c0 param_5
     *   metric_18 ← 98c0 param_6
     *   metric_1c ← 98c0 param_1
     *   metric_20 ← 98c0 param_2 */
    int32_t   metric_0c;                     /* +0x0c */
    int32_t   metric_10;                     /* +0x10 */
    int32_t   metric_14;                     /* +0x14 */
    int32_t   metric_18;                     /* +0x18 */
    int32_t   metric_1c;                     /* +0x1c */
    int32_t   metric_20;                     /* +0x20 */

    /* +0x24, +0x28: colorkey storage stamped by FUN_005b9830.
     *   colorkey_in  ← raw value the orchestrator passes (0x1ffffff
     *                  sentinel = "no color key requested")
     *   colorkey_out ← post-conversion value (the 16bpp path runs
     *                  the channel-shifter FUN_005b8b00 first;
     *                  other bpp the values are identical).  Also the
     *                  value handed to IDirectDrawSurface7::SetColorKey
     *                  as the DDCOLORKEY.dwColorSpaceLowValue +
     *                  dwColorSpaceHighValue (retail uses the same
     *                  dword for both halves of the colorkey range). */
    int32_t   colorkey_in;                   /* +0x24 */
    int32_t   colorkey_out;                  /* +0x28 */

    /* +0x2c: primary IDirectDrawSurface7 (or similar IUnknown).
     * Owned — released via vtable[2] in zdd_object_dtor.  Also the
     * out-slot CreateSurface writes into (orchestrator passes
     * &com_primary as the surface-out pointer). */
    void     *com_primary;                   /* +0x2c */

    /* +0x30..+0xab: embedded scratch DDSURFACEDESC2 (124 bytes).
     * Zeroed by FUN_005b97e0's 31-dword loop; dwSize stamped 0x7c
     * (the first byte).  Built up by FUN_005b8c00 before CreateSurface
     * actually runs.  Kept opaque (uint8_t[]) so we don't pull
     * <ddraw.h> into the host build. */
    uint8_t   embedded_ddsd[0xac - 0x30];

    /* +0xac: secondary IDirectDrawSurface7 (the attached back-buffer
     * the engine fetches via GetAttachedSurface).  Owned — released
     * via vtable[2] in zdd_object_dtor BEFORE com_primary. */
    void     *com_back;                      /* +0xac */

    /* +0xb0..+0xbf: secondary metric region stamped by FUN_005b98c0
     * (post-DDSD).  +0xb0/+0xb4 always zero, +0xb8/+0xbc carry the
     * same param_5/param_6 the +0x14/+0x18 slot pair holds. */
    int32_t   metric_b0;                     /* +0xb0 */
    int32_t   metric_b4;                     /* +0xb4 */
    int32_t   metric_b8;                     /* +0xb8 */
    int32_t   metric_bc;                     /* +0xbc */

    /* +0xc0: back-pointer to the owning ZDD.  Set by ctor; dtor
     * uses it to decrement parent->open_objects.  Borrowed (no
     * release responsibility). */
    zdd      *parent;                        /* +0xc0 */

    /* +0xc4: HLOCAL pixel buffer (LocalAlloc-managed).  Freed by
     * FUN_005b93e0 via LocalFree on dtor.  NULL when no system-memory
     * surface is attached — set elsewhere by the (unported) surface-
     * alloc path. */
    void     *pixel_buf;                     /* +0xc4 */

    /* +0xc8: flag cleared together with pixel_buf on LocalFree.
     * Semantic role uncertain — likely "pixel_buf is system-memory
     * owned" so a future re-alloc knows to release the prior one. */
    uint32_t  pixel_buf_flag;                /* +0xc8 */

    /* +0xcc, +0xd0: cached create-time args stamped by FUN_005b97e0
     * from the orchestrator's caller args.
     *   caps_in           ← 97e0 param_1.  Re-read by the orchestrator
     *                       as the caps_base for FUN_005b8c00's
     *                       CreateSurface call (so the call uses the
     *                       value that's now on the ZDDObject, not the
     *                       raw orchestrator param — a small but
     *                       deliberate roundtrip in retail).
     *   force_videomem_in ← 97e0 param_2.  Stored for symmetry; the
     *                       orchestrator separately passes its own
     *                       param_5 as force_videomem to 8c00.  Likely
     *                       used by later surface-restore paths. */
    int32_t   caps_in;                       /* +0xcc */
    int32_t   force_videomem_in;             /* +0xd0 */

    /* +0xd4: state flag.  Ctor clears to 0; FUN_005b9830 sets to
     * 0x8000 when a non-sentinel color key is bound (sentinel path
     * clears back to 0). */
    uint32_t  state_flag;                    /* +0xd4 */
};

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(zdd_object)                       == 0x0d8, "zdd_object must be 0xd8 bytes");
_Static_assert(offsetof(zdd_object, ddsd_lpSurface_ref) == 0x000, "zdd_object ddsd_lpSurface_ref offset");
_Static_assert(offsetof(zdd_object, ddsd_pitch_ref)     == 0x004, "zdd_object ddsd_pitch_ref offset");
_Static_assert(offsetof(zdd_object, ddsd_height_ref)    == 0x008, "zdd_object ddsd_height_ref offset");
_Static_assert(offsetof(zdd_object, metric_0c)          == 0x00c, "zdd_object metric_0c offset");
_Static_assert(offsetof(zdd_object, metric_10)          == 0x010, "zdd_object metric_10 offset");
_Static_assert(offsetof(zdd_object, metric_14)          == 0x014, "zdd_object metric_14 offset");
_Static_assert(offsetof(zdd_object, metric_18)          == 0x018, "zdd_object metric_18 offset");
_Static_assert(offsetof(zdd_object, metric_1c)          == 0x01c, "zdd_object metric_1c offset");
_Static_assert(offsetof(zdd_object, metric_20)          == 0x020, "zdd_object metric_20 offset");
_Static_assert(offsetof(zdd_object, colorkey_in)        == 0x024, "zdd_object colorkey_in offset");
_Static_assert(offsetof(zdd_object, colorkey_out)       == 0x028, "zdd_object colorkey_out offset");
_Static_assert(offsetof(zdd_object, com_primary)        == 0x02c, "zdd_object com_primary offset");
_Static_assert(offsetof(zdd_object, embedded_ddsd)      == 0x030, "zdd_object embedded_ddsd offset");
_Static_assert(offsetof(zdd_object, com_back)           == 0x0ac, "zdd_object com_back offset");
_Static_assert(offsetof(zdd_object, metric_b0)          == 0x0b0, "zdd_object metric_b0 offset");
_Static_assert(offsetof(zdd_object, metric_b4)          == 0x0b4, "zdd_object metric_b4 offset");
_Static_assert(offsetof(zdd_object, metric_b8)          == 0x0b8, "zdd_object metric_b8 offset");
_Static_assert(offsetof(zdd_object, metric_bc)          == 0x0bc, "zdd_object metric_bc offset");
_Static_assert(offsetof(zdd_object, parent)             == 0x0c0, "zdd_object parent offset");
_Static_assert(offsetof(zdd_object, pixel_buf)          == 0x0c4, "zdd_object pixel_buf offset");
_Static_assert(offsetof(zdd_object, pixel_buf_flag)     == 0x0c8, "zdd_object pixel_buf_flag offset");
_Static_assert(offsetof(zdd_object, caps_in)            == 0x0cc, "zdd_object caps_in offset");
_Static_assert(offsetof(zdd_object, force_videomem_in)  == 0x0d0, "zdd_object force_videomem_in offset");
_Static_assert(offsetof(zdd_object, state_flag)         == 0x0d4, "zdd_object state_flag offset");
_Static_assert(sizeof(zdd)                          == 0x170, "zdd must be 0x170 bytes");
_Static_assert(offsetof(zdd, back_obj_a)            == 0x018, "zdd back_obj_a offset");
_Static_assert(offsetof(zdd, back_obj_b)            == 0x01c, "zdd back_obj_b offset");
_Static_assert(offsetof(zdd, log_buf)               == 0x020, "zdd log_buf offset");
_Static_assert(offsetof(zdd, open_objects)          == 0x120, "zdd open_objects offset");
_Static_assert(offsetof(zdd, ddraw7)                == 0x124, "zdd ddraw7 offset");
_Static_assert(offsetof(zdd, com_a)                 == 0x128, "zdd com_a offset");
_Static_assert(offsetof(zdd, com_b)                 == 0x12c, "zdd com_b offset");
_Static_assert(offsetof(zdd, cursor_state)          == 0x130, "zdd cursor_state offset");
_Static_assert(offsetof(zdd, videomem_flag)         == 0x134, "zdd videomem_flag offset");
_Static_assert(offsetof(zdd, pixel_format_mode)     == 0x164, "zdd pixel_format_mode offset");
_Static_assert(offsetof(zdd, pixel_format_bpp)      == 0x168, "zdd pixel_format_bpp offset");
_Static_assert(offsetof(zdd, primary_obj)           == 0x16c, "zdd primary_obj offset");
#endif

/* ─── DDSURFACEDESC2 builder ─────────────────────────────────────── */

/* Subset of DDSURFACEDESC2 that the engine actually fills.  Modelled
 * as a plain C struct so the descriptor build is testable on host
 * without dragging in <ddraw.h>.  Win32 wrappers translate to the
 * real DDSURFACEDESC2 + DDPIXELFORMAT shapes.
 *
 * Field meanings + bit constants pinned by docs/findings/ddraw-init.md
 * "FUN_005b8c00" section.  We use plain numbers in the struct (the
 * macro names like DDSD_CAPS / DDSCAPS_OFFSCREENPLAIN are Win32-side
 * only). */
typedef struct zdd_surface_desc_build {
    /* dwFlags bitmask passed to CreateSurface.  Always carries
     * 0x07 = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH; additionally 0x1000 =
     * DDSD_PIXELFORMAT when has_pixel_format != 0. */
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;

    /* ddsCaps.dwCaps — caller's caps OR'd with 0x40 = DDSCAPS_OFFSCREENPLAIN
     * and (conditionally) 0x800 = DDSCAPS_VIDEOMEMORY. */
    uint32_t dwCaps;

    /* When non-zero, the pixel-format block below is populated and
     * dwFlags carries DDSD_PIXELFORMAT.  When zero, the pixel-format
     * fields are all 0 (DDraw will pick a format that matches the
     * current display mode). */
    int      has_pixel_format;

    /* ddpf_*: DDPIXELFORMAT subset (only fields the engine sets).
     * dwSize is always 0x20 when has_pixel_format != 0.
     * dwFlags is 0x40 = DDPF_RGB (or 0x60 = DDPF_RGB|DDPF_PALETTEINDEXED8
     * for the 8bpp branch).
     * dwRGBBitCount, dwRBitMask, dwGBitMask, dwBBitMask are 0 for the
     * 8bpp branch (DDraw infers from palette size) and populated for
     * 16/24/32. */
    uint32_t ddpf_dwSize;
    uint32_t ddpf_dwFlags;
    uint32_t ddpf_dwRGBBitCount;
    uint32_t ddpf_dwRBitMask;
    uint32_t ddpf_dwGBitMask;
    uint32_t ddpf_dwBBitMask;
} zdd_surface_desc_build;

/* FUN_005b8c00 lines 1-50 — the pure-logic descriptor build.  Reads
 * self->videomem_flag / pixel_format_mode / pixel_format_bpp and
 * folds caller params (caps_base, force_videomem, width, height) into
 * a zdd_surface_desc_build.
 *
 * Algorithm:
 *   out->dwFlags = 0x07
 *   out->dwHeight = height; out->dwWidth = width
 *   out->dwCaps = caps_base | 0x40
 *   if (force_videomem || self->videomem_flag) out->dwCaps |= 0x800
 *
 *   if (self->pixel_format_mode != 2):
 *     out->has_pixel_format = 0; ddpf_* = 0
 *   else:
 *     out->dwFlags |= 0x1000
 *     out->has_pixel_format = 1
 *     out->ddpf_dwSize  = 0x20
 *     out->ddpf_dwFlags = 0x40  (overridden to 0x60 in 8bpp branch)
 *     switch (self->pixel_format_bpp):
 *       case 8:  ddpf_dwFlags = 0x60
 *       case 16: dwRGBBitCount=16, masks = 0xF800/0x07E0/0x001F (RGB565)
 *       case 24: dwRGBBitCount=24, masks = 0xFF0000/0xFF00/0xFF
 *       case 32: dwRGBBitCount=32, masks = 0xFF0000/0xFF00/0xFF
 *       default: leave ddpf_dwRGBBitCount = 0, masks = 0
 *   Note: 24-bpp falls through to 32-bpp's mask values (retail's
 *   switch literally lists `case 0x18: case 0x20:` together).  See
 *   docs/findings/ddraw-init.md "Engine quirk".
 */
void zdd_build_surface_desc(const zdd *self, zdd_surface_desc_build *out,
                            uint32_t width, uint32_t height,
                            uint32_t caps_base, int force_videomem);

/* ─── ZDDObject lifecycle (pure logic) ───────────────────────────── */

/* FUN_005b9350 — in-place ctor.  Stamps the back-pointer to the
 * owning ZDD, clears the lifecycle-pair fields (com_primary, com_back,
 * pixel_buf, pixel_buf_flag, state_flag), and bumps
 * parent->open_objects.  Other fields (the embedded DDSD + window-fit
 * metrics) are NOT cleared by this ctor — the surface-alloc path
 * (FUN_005b97e0/_98c0) writes them later.  Our port pre-zeros the
 * whole struct via memset for determinism; the observable behaviour
 * is identical from the lifecycle-pair's perspective. */
void zdd_object_ctor(zdd_object *self, zdd *parent);

/* FUN_005b93e0 — release the pixel buffer (LocalAlloc-managed).  If
 * pixel_buf != NULL: LocalFree it, NULL it, clear pixel_buf_flag.
 * Idempotent on NULL.  Wraps the LocalFree call via the
 * zdd_object_local_free primitive so host tests can intercept. */
void zdd_object_release_pixel_buf(zdd_object *self);

/* FUN_005b9390 — dtor.  Sequence (matches retail):
 *   1. zdd_object_release_pixel_buf(self)
 *   2. if (com_back)    Release(com_back);    com_back    = NULL;
 *   3. if (com_primary) Release(com_primary); com_primary = NULL;
 *   4. parent->open_objects--
 *
 * Note the release order: com_back BEFORE com_primary — retail's
 * FUN_005b9390 reads +0xac first then +0x2c.  Likely because com_back
 * is the IDirectDrawSurface7 the engine GetAttachedSurface'd off the
 * primary; releasing the attached child first keeps the COM refcount
 * graph clean. */
void zdd_object_dtor(zdd_object *self);

/* ─── ZDDObject surface-alloc stampers + orchestrator ────────────── */

/* FUN_005b97e0 — pre-fill helper.  Pure logic; called by the
 * orchestrator BEFORE FUN_005b8c00 (CreateSurface).  Stamps:
 *   - +0xcc           = caps_in           (caller's caps_base, re-read
 *                                          by orchestrator when calling
 *                                          CreateSurface)
 *   - +0xd0           = force_videomem_in (caller's force_videomem)
 *   - +0x30..+0xab    = zeroed (124-byte embedded DDSURFACEDESC2)
 *   - +0x00,+0x04,+0x08 = self-pointers into embedded_ddsd[0x24/0x10/0x08]
 *   - embedded_ddsd[0] = 0x7c (DDSD dwSize)
 *
 * Issue order matches retail (caps_in / force_videomem_in stamps first,
 * then DDSD zero loop, then self-pointer + dwSize stamps). */
void zdd_object_prefill_desc(zdd_object *self,
                             int32_t caps_in, int32_t force_videomem_in);

/* FUN_005b98c0 — metric-stamp helper.  Pure logic; called by the
 * orchestrator AFTER FUN_005b8c00 succeeds.  10 dword writes in
 * retail issue order:
 *   +0xb0 = 0,  +0xb4 = 0,  +0x0c = p3,  +0xb8 = p5,  +0xbc = p6,
 *   +0x14 = p5, +0x18 = p6, +0x10 = p4,  +0x1c = p1,  +0x20 = p2.
 *
 * Param-to-slot mapping is preserved exactly — retail picks an unusual
 * write order that doesn't match the parameter order, and we follow it
 * literally. */
void zdd_object_stamp_metrics(zdd_object *self,
                              int32_t p1, int32_t p2, int32_t p3,
                              int32_t p4, int32_t p5, int32_t p6);

/* FUN_005b9830 — color-key wrapper.  Two paths:
 *   key == 0x1ffffff (sentinel):
 *     - colorkey_in  = sentinel
 *     - colorkey_out = sentinel
 *     - state_flag   = 0
 *     - returns 1 (no SetColorKey call issued)
 *   key != sentinel:
 *     - colorkey_in  = raw key
 *     - state_flag   = 0x8000
 *     - if (parent->pixel_format_bpp == 16): key = pixel_format_convert(key)
 *         (currently NOT ported — pending FUN_005b8b00; we skip the
 *          conversion when the 16bpp branch fires.  Flagged by
 *          docs/findings/ddraw-init.md "FUN_005b8b00" open thread.)
 *     - colorkey_out = key (post-conversion)
 *     - calls zdd_surface_set_color_key(com_primary, key)
 *     - on failure: zdd_log_dderr(parent, "DirectDrawSurface",
 *                                 "SetColorKey", hr) + returns 0
 *     - on success: returns 1
 *
 * NOTE: in the 16bpp branch, retail uses param_1 = FUN_005b8b00(param_1)
 * — i.e., overwrites the key with the converted value.  Our port
 * currently skips that conversion (returns raw key).  Once FUN_005b8b00
 * lands, wire it in here.  For the orchestrator at boot (mode 0,
 * pixel_format_bpp == 16, key = 0x1ffffff) the sentinel branch wins, so
 * the missing conversion is dead code right now. */
int  zdd_object_set_color_key(zdd_object *self, int32_t key);

/* FUN_005b95c0 — surface-alloc orchestrator.  Pure orchestration over
 * the 4 helpers above:
 *
 *   zdd_object_prefill_desc(self, p3, p5)
 *   if (!zdd_create_surface(self->parent, &self->com_primary,
 *                           width, height,
 *                           self->caps_in, p5)) return
 *   zdd_object_stamp_metrics(self, p1, p2, p3, p4, p5, p6)
 *   zdd_object_set_color_key(self, p4)
 *
 * Args follow retail's 9-param shape (p7 is captured but unused — retail
 * builds it but never reads it; we accept it for ABI symmetry):
 *   p1, p2 = window rect TL coords (probably; stamped at +0x1c/+0x20)
 *   p3     = caps_in (also stamped at +0x0c)
 *   p4     = colorkey (handed to SetColorKey; 0x1ffffff = "none")
 *   p5     = force_videomem hint (also "back-buffer count" per
 *            FUN_005b8b40's caller — see ddraw-init.md)
 *   p6     = unknown (stamped at +0x18/+0xbc)
 *   p7     = unused
 *   width, height = surface dimensions */
void zdd_object_create_surface_pair(zdd_object *self,
                                    int32_t p1, int32_t p2, int32_t p3,
                                    int32_t p4, int32_t p5, int32_t p6,
                                    int32_t p7,
                                    uint32_t width, uint32_t height);

/* ─── ZDD pure logic ─────────────────────────────────────────────── */

/* FUN_005b7f80 — in-place ctor.  Issue order (matches retail exactly):
 *   cursor_state   = 1
 *   ddraw7         = NULL
 *   com_a          = NULL
 *   primary_obj    = NULL
 *   back_obj_a     = NULL
 *   back_obj_b     = NULL
 *   com_b          = NULL
 *   open_objects   = 0
 *   log_buf[0]     = '\0'  (retail does inlined strcpy of DAT_008a9b6c)
 *
 * Other fields (pad and pixel_format_*) are zeroed wholesale on entry
 * for determinism — retail leaves them whatever operator_new yielded,
 * which in practice is unobservable until those fields actually get
 * read. */
void zdd_ctor(zdd *self);

/* FUN_005b8da0 — restore cursor visibility if some other (still-
 * unported) path hid it.  Idempotent — does nothing when
 * cursor_state == 1.  When cursor_state == 0, calls zdd_show_cursor(1)
 * and stamps cursor_state back to 1. */
void zdd_restore_cursor_on_release(zdd *self);

/* FUN_005b8040 — drop the 5 child COM/heap objects.  Issue order
 * matches retail:
 *   1. primary_obj  (zdd_obj_destroy)
 *   2. back_obj_a   (zdd_obj_destroy)
 *   3. back_obj_b   (zdd_obj_destroy)
 *   4. com_a        (zdd_com_release)
 *   5. com_b        (zdd_com_release)
 *
 * Each slot is no-op when NULL; the primitive is responsible for
 * zeroing the field afterwards (we pass by-pointer). */
void zdd_release_children(zdd *self);

/* FUN_005b7fe0 — full destructor.  Sequence:
 *   1. zdd_restore_cursor_on_release(self)
 *   2. zdd_release_children(self)
 *   3. if (ddraw7) IUnknown::Release(ddraw7); ddraw7 = NULL;
 *   4. if (open_objects != 0) OutputDebugStringA("Warning,exists ZDD objects.\n")
 *   5. if (log_buf[0] != '\0') OutputDebugStringA(log_buf)
 *
 * After this returns the struct is in the same state as right after
 * zdd_ctor — the caller frees the allocation itself (zdd_destroy
 * does the dtor+free combo). */
void zdd_dtor(zdd *self);

/* FUN_005b80d0 — render a DDERR-decorated log message into
 * self->log_buf and flush via OutputDebugStringA.  Output format,
 * verified against the PE data section:
 *
 *   "Warning,exists ZDD errors," + prefix1 + "." + prefix2 +
 *   " failed,Error Code " + hex(hresult) + dderr_name + ".\n"
 *
 * where:
 *   - prefix1 / prefix2 are caller-supplied tags (NULL treated as "")
 *   - hex(hresult) is lowercase hex of the HRESULT *as an unsigned
 *     32-bit value*, no leading "0x" — matches FUN_005c0907's behaviour
 *     when called with base 0x10
 *   - dderr_name is "DDERR_INVALIDOBJECT" (etc.) if the HRESULT
 *     matches one of the engine's hard-coded entries; otherwise empty.
 *
 * The buffer remains populated after the call (retail doesn't clear
 * it).  Used by FUN_005b88c0 (DirectDrawCreate fail) and FUN_005b89d0
 * (SetCooperativeLevel fail). */
void zdd_log_dderr(zdd *self, const char *prefix1, const char *prefix2,
                   int32_t hresult);

/* Pure-logic helper that backs zdd_log_dderr — returns the canonical
 * DDERR_XXX name for `hresult`, or NULL when the HRESULT is not in
 * the engine's hard-coded table.  Order and coverage matches the
 * exact set the engine recognises (see FUN_005b80d0's switch ladder
 * — 18 entries).  Caller does NOT own the string. */
const char *zdd_dderr_name(int32_t hresult);

/* ─── high-level driver ──────────────────────────────────────────── */

/* FUN_005b7ee0 — allocate a fresh ZDD on the heap, run zdd_ctor,
 * run zdd_directdraw_create_ex.  On success: stores the new ZDD*
 * into *out and returns 1.  On DirectDrawCreateEx failure: tears the
 * ZDD down (zdd_dtor + heap free) and returns 0 with *out unchanged.
 *
 * Note: retail's `operator_new(0x170)` does NOT zero-init.  Our
 * zdd_create uses zeroed allocation so debugging is easier; the
 * subsequent ctor overwrites every observable field anyway. */
int  zdd_create(zdd **out);

/* zdd_dtor + heap free.  Idempotent on NULL.  Inverse of zdd_create. */
void zdd_destroy(zdd *self);

/* ─── Win32 primitives — defined per build target ────────────────── */

/* Real build (zdd_win32.c): ShowCursor(BOOL).  Host build
 * (test_zdd.c): records the call. */
void zdd_show_cursor(int show);

/* Real build: OutputDebugStringA.  Host build: records into a
 * test-controlled capture buffer. */
void zdd_output_debug_string(const char *s);

/* Real build: `if (*p) { (*(IUnknown*)*p)->Release(*p); *p = NULL; }`
 * via the standard vtable[2] call.  Host build: decrements a release
 * counter + zeros *p.  Idempotent on (*p == NULL). */
void zdd_com_release(void **iunknown_pp);

/* Real + host build: full ZDDObject teardown — runs zdd_object_dtor
 * (the FUN_005b9390 cleanup chain) then heap-free's the allocation.
 * Idempotent on (*p == NULL).  Zeros *p so callers can safely chain
 * the same field through another path. */
void zdd_obj_destroy(zdd_object **obj_pp);

/* Real build: LocalFree(HLOCAL).  Host build: tracks the call for
 * leak assertions.  NULL is a no-op. */
void zdd_object_local_free(void *local_alloc);

/* Real build: DirectDrawCreateEx(NULL, &self->ddraw7, &IID_IDirectDraw7,
 * NULL).  On failure, calls zdd_log_dderr(self, "", "DirectDrawCreate",
 * hresult) and returns 0.  Host build: pulls the HRESULT (success or
 * failure) from a test-controlled state.
 *
 * Mirrors FUN_005b88c0. */
int  zdd_directdraw_create_ex(zdd *self);

/* FUN_005b8c00 — full surface-create driver.  Real build:
 *   1. zdd_build_surface_desc(...) into a stack descriptor
 *   2. translate descriptor to a real DDSURFACEDESC2
 *   3. IDirectDraw7::CreateSurface (vtable[6] / byte offset 0x18)
 *   4. on success AND self->com_b (palette) != NULL:
 *        IDirectDrawSurface7::SetPalette (vtable[31] / byte offset 0x7c)
 *   5. on failure: zdd_log_dderr("DirectDraw", "CreateSurface", hr); return 0
 *   6. on success: return 1
 *
 * Host build: returns 1 + stashes a fake surface pointer when a
 * test-controlled g_create_surface_result is non-zero, else logs
 * via the DDERR path and returns 0.  Used by FUN_005b95c0 (unported)
 * and by tests of the descriptor-build dispatch. */
int  zdd_create_surface(zdd *self, void **out_surface,
                        uint32_t width, uint32_t height,
                        uint32_t caps_base, int force_videomem);

/* Real build: vtable[20](ddraw7, hwnd, flags) where flags is
 * DDSCL_NORMAL (8) when fullscreen == 0, or
 * DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_ALLOWREBOOT (0x13) otherwise.
 * On failure, calls zdd_log_dderr(self, "DirectDraw", "SetCooperativeLevel",
 * hresult) and returns 0.  Host build: stub controllable from tests.
 *
 * Mirrors FUN_005b89d0. */
int  zdd_set_coop_level(zdd *self, void *hwnd, int fullscreen);

/* Real build: IDirectDrawSurface7::SetColorKey via vtable[29] (byte
 * offset 0x74) with DDCKEY_SRCBLT (8) flag.  Builds a DDCOLORKEY
 * with .dwColorSpaceLowValue == .dwColorSpaceHighValue == key — matches
 * retail's `local_8 = key; local_4 = key` stack setup.  Returns 1 on
 * success, 0 on failure (logs via zdd_log_dderr through `log_owner`,
 * which is the parent ZDD so the DDERR builder has a log_buf to use).
 * NULL surface returns 0 without logging.  Host build: returns
 * g_dd_setcolorkey_result + records the call. */
int  zdd_surface_set_color_key(void *surface, int32_t key, zdd *log_owner);

#endif /* OPENSUMMONERS_ZDD_H */
