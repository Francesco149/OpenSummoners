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

/* 22-byte pixel-format color descriptor occupying the first slot of the
 * ZDD struct (+0x00..+0x15).  Stamped by zdd_bind_pixel_format
 * (FUN_005b8a20) when the surface is 16bpp; consumed by zdd_color_convert
 * (FUN_005b8b00) to map RGB888 source values to the surface's native
 * pixel format.  Layout pinned from FUN_005b8a20 byte stores + reads in
 * FUN_005b8b00:
 *
 *   shift_left[3]  (+0x00..+0x02) — left-shift to place each channel
 *                                   into the output word.  For RGB565:
 *                                   R=11, G=5, B=0.
 *   mask_raw[3]    (+0x04..+0x0f) — raw R/G/B bitmasks from the
 *                                   surface's DDPIXELFORMAT (e.g.
 *                                   0xF800/0x07E0/0x001F for RGB565).
 *   shift_right[3] (+0x10..+0x12) — right-shift applied to a source
 *                                   8bpp channel before placement.  3
 *                                   if the channel ends up 5-bit
 *                                   (saturating to 0x1F after the
 *                                   trailing-zero strip), else 2 (the
 *                                   6-bit green case).
 *   mask_lo[3]     (+0x13..+0x15) — low byte of the post-shift mask
 *                                   (significance count).
 *
 * The two trailing bytes (+0x16..+0x17) are unobserved by any ported
 * code — kept as opaque pad so back_obj_a still lands at +0x18. */
typedef struct zdd_color_descriptor {
    uint8_t   shift_left[3];                 /* +0x00..+0x02 */
    uint8_t   _pad03;                        /* +0x03 — never read */
    uint32_t  mask_raw[3];                   /* +0x04..+0x0f */
    uint8_t   shift_right[3];                /* +0x10..+0x12 */
    uint8_t   mask_lo[3];                    /* +0x13..+0x15 */
    uint8_t   _pad16[2];                     /* +0x16..+0x17 — alignment */
} zdd_color_descriptor;

/* Field layout inferred from FUN_005b7f80 (ctor writes), FUN_005b7fe0
 * (dtor reads), and FUN_005b8040 (5-slot release loop).  Size 0x170 is
 * the literal `operator_new(0x170)` argument in FUN_005b7ee0. */
struct zdd {
    /* +0x00..+0x17: pixel-format color descriptor (see typedef above).
     * Stamped by zdd_bind_pixel_format (FUN_005b8a20) on 16bpp boots.
     * Untouched on 8/24/32bpp paths — descriptor bytes stay zero.  Uses
     * 22 logical bytes (+0x00..+0x15); the +0x16/+0x17 alignment tail
     * absorbs the original retail "+0x00..+0x17 unobserved pad" — no
     * known consumer touches them. */
    zdd_color_descriptor color_desc;        /* +0x00..+0x17 */

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

    /* +0x128: primary display IDirectDrawSurface7.  Allocated by
     * FUN_005b8480 mode 0/3/4 via `ddraw7->CreateSurface(..., &com_a,
     * NULL)` — note the surface goes directly on the ZDD wrapper,
     * NOT on a ZDDObject child.  Read by `zdd_setup_8bit_palette`
     * as the target of SetPalette.  Released via IUnknown::Release
     * (vtable+8) in FUN_005b8040's 4th slot.
     *
     * +0x12c: IDirectDrawPalette (8bpp branch only).  Allocated by
     * `zdd_setup_8bit_palette` via `ddraw7->CreatePalette(...,
     * &com_b, NULL)`.  Read by `zdd_create_surface` as the palette
     * to bind to each new ZDDObject surface (vtable[31] SetPalette).
     * Released via IUnknown::Release in FUN_005b8040's 5th slot. */
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

    /* +0x138, +0x13c: zeroed by FUN_005b8480's prologue (the
     * screen-init driver) on every call.  No ported code reads or
     * writes either field outside that zero-stamp; likely a paired
     * (x, y) origin that the windowed-mode path sets elsewhere when
     * positioning the screen inside the desktop.  Names are descriptive
     * placeholders — revise once a consumer pins the role. */
    int32_t       screen_pos_x;              /* +0x138 */
    int32_t       screen_pos_y;              /* +0x13c */

    /* +0x140, +0x144: width / height of the requested screen.  Stamped
     * by FUN_005b8480 from the launcher's chosen resolution (in retail,
     * always 640×480).  Read by no other ported path yet — the
     * orchestrator slots that consume w/h pass them directly as
     * function args. */
    int32_t       screen_width;              /* +0x140 */
    int32_t       screen_height;             /* +0x144 */

    /* +0x148..+0x163: 7-int rect blob (28 bytes).  Stamped by
     * FUN_005b8480 — either copied from caller's param_6 (mode 4 Zoom
     * passes a real 7-int layout: {display_w, display_h, 2, centre_x,
     * centre_y, src_w, src_h}) or zeroed (modes 0/1/2/3 pass NULL).
     * Indices 0/1 are read by FUN_005b8480's mode 4 branch as the
     * back-buffer dimensions; indices 5/6 as the orchestrator's
     * back_obj_b dimensions. */
    int32_t       screen_rect[7];            /* +0x148..+0x163 */

    /* +0x164: surface-init mode arg (the value the launcher's radio
     * picked).  Stamped by FUN_005b8480 from param_4.  Five-state
     * enum: 0=Full / 1=Safe / 2=Windowed / 3=DB Mode / 4=Zoom Mode.
     *
     * Historical alias `pixel_format_mode` — FUN_005b8c00's
     * DDSURFACEDESC2 builder branches on `field == 2`, which doubles as
     * "Windowed launcher mode" AND "DDSD needs an explicit
     * DDPIXELFORMAT block" (only Windowed mode must match the desktop's
     * pixel format explicitly).  Field name preserves the read-site
     * semantics; the write-site (mode_arg) semantics are the primary
     * meaning. */
    int32_t       pixel_format_mode;         /* +0x164 — aka mode_arg */

    /* +0x168: pixel-format bpp hint.  Stamped by FUN_005b8480 from
     * param_3 (8/16/24/32).  Read by FUN_005b8c00's DDSD builder when
     * pixel_format_mode == 2 (Windowed) — branches on bpp to fill the
     * DDPIXELFORMAT masks. */
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
     * same param_5/param_6 the +0x14/+0x18 slot pair holds.  When
     * called from FUN_005b95c0 (the surface-alloc orchestrator), 98c0
     * param_5/_6 are the surface width/height — so metric_b8 = width
     * and metric_bc = height in the boot path.  Read by
     * FUN_005b9520's clipper-attach as the RGNDATA RECT bound. */
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
_Static_assert(offsetof(zdd, color_desc)            == 0x000, "zdd color_desc offset");
_Static_assert(offsetof(zdd_color_descriptor, shift_left)  == 0x000, "color_desc shift_left offset");
_Static_assert(offsetof(zdd_color_descriptor, mask_raw)    == 0x004, "color_desc mask_raw offset");
_Static_assert(offsetof(zdd_color_descriptor, shift_right) == 0x010, "color_desc shift_right offset");
_Static_assert(offsetof(zdd_color_descriptor, mask_lo)     == 0x013, "color_desc mask_lo offset");
_Static_assert(sizeof(zdd_color_descriptor)         == 0x018, "color_desc must be 24 bytes (22 used + 2 align)");
_Static_assert(offsetof(zdd, back_obj_a)            == 0x018, "zdd back_obj_a offset");
_Static_assert(offsetof(zdd, back_obj_b)            == 0x01c, "zdd back_obj_b offset");
_Static_assert(offsetof(zdd, log_buf)               == 0x020, "zdd log_buf offset");
_Static_assert(offsetof(zdd, open_objects)          == 0x120, "zdd open_objects offset");
_Static_assert(offsetof(zdd, ddraw7)                == 0x124, "zdd ddraw7 offset");
_Static_assert(offsetof(zdd, com_a)                 == 0x128, "zdd com_a offset");
_Static_assert(offsetof(zdd, com_b)                 == 0x12c, "zdd com_b offset");
_Static_assert(offsetof(zdd, cursor_state)          == 0x130, "zdd cursor_state offset");
_Static_assert(offsetof(zdd, videomem_flag)         == 0x134, "zdd videomem_flag offset");
_Static_assert(offsetof(zdd, screen_pos_x)          == 0x138, "zdd screen_pos_x offset");
_Static_assert(offsetof(zdd, screen_pos_y)          == 0x13c, "zdd screen_pos_y offset");
_Static_assert(offsetof(zdd, screen_width)          == 0x140, "zdd screen_width offset");
_Static_assert(offsetof(zdd, screen_height)         == 0x144, "zdd screen_height offset");
_Static_assert(offsetof(zdd, screen_rect)           == 0x148, "zdd screen_rect offset");
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
 *                           self->caps_in, p5)) return 0
 *   zdd_object_stamp_metrics(self, p1, p2, p6, p7, width, height)
 *   return zdd_object_set_color_key(self, p4)
 *
 * Return value matches retail's implicit EAX-carry-through:
 *   0  →  CreateSurface failed (or SetColorKey failed on the real-key
 *         branch).  Caller (FUN_005b8b40) tears down the ZDDObject.
 *   1  →  CreateSurface succeeded AND (SetColorKey succeeded OR was
 *         skipped via the 0x1ffffff sentinel).
 * Retail's Ghidra decomp shows the function as `void` but the assembly
 * leaves the last callee's return in EAX — the FUN_005b8b40 caller
 * reads it as int.
 *
 * Args follow retail's 9-param shape.  The unusual bit is that
 * stamp_metrics gets (p1, p2, p6, p7, width, height) — p3/p4/p5 are
 * consumed by earlier calls and don't reach the metric slots.
 * Disassembly evidence: r2 at 0x5b95ff–0x5b9617 — push order eax=p7,
 * ecx=p6, edx=p2, eax=p1 with already-loaded ebx=p8/width and
 * edi=p9/height.
 *
 *   p1, p2 = surface origin coords (stamped at +0x1c/+0x20)
 *   p3     = caps_in (stamped on the ZDDObject at +0xcc by prefill,
 *            re-read into CreateSurface's `caps_base`)
 *   p4     = colorkey (handed to SetColorKey; 0x1ffffff = "none")
 *   p5     = force_videomem hint / back-buffer count per
 *            FUN_005b8b40's caller (see ddraw-init.md).  Consumed by
 *            prefill (+0xd0 force_videomem_in) and CreateSurface's
 *            `force_videomem`; does NOT reach the metric slots.
 *   p6     = secondary origin / pair-coord (stamped at +0x0c metric_0c).
 *   p7     = secondary flag (stamped at +0x10 metric_10).
 *   width, height = surface dimensions; ALSO stamped at +0xb8/+0xbc
 *            (metric_b8/bc) and +0x14/+0x18 (metric_14/18) — the
 *            clipper attach (FUN_005b9520) later reads these as the
 *            RGNDATA RECT bound. */
int  zdd_object_create_surface_pair(zdd_object *self,
                                    int32_t p1, int32_t p2, int32_t p3,
                                    int32_t p4, int32_t p5, int32_t p6,
                                    int32_t p7,
                                    uint32_t width, uint32_t height);

/* FUN_005b8b40 — operator_new(0xd8) + ZDDObject ctor + surface-alloc
 * orchestrator + cleanup-on-failure.  The public factory the engine
 * uses to ask "give me a new ZDDObject bound to a fresh surface".
 *
 *   zdo = calloc(1, sizeof(zdd_object));
 *   if (!zdo) return 0;
 *   zdd_object_ctor(zdo, parent);
 *   if (!zdd_object_create_surface_pair(zdo, w, h, 0, colorkey,
 *                                       count, 0, 0, w, h)) {
 *       zdd_object_dtor(zdo); free(zdo); return 0;
 *   }
 *   *out = zdo; return 1;
 *
 * Note: retail uses `operator_new(0xd8)` (uninitialized) — our port
 * uses calloc for deterministic zero-init.  The subsequent ctor
 * stamps every observable field, so the observable behaviour is
 * identical.
 *
 * Cleanup-on-failure: retail calls FUN_005b9390 (the bare dtor) +
 * FUN_005bef0e (the heap-free primitive).  Our port mirrors with
 * zdd_object_dtor + free.  Note that this is NOT zdd_obj_destroy —
 * the latter is the parent-driven cleanup primitive that takes a
 * **pp; here we own the local variable, so we use dtor + free
 * directly. */
int  zdd_object_new(zdd *parent, zdd_object **out,
                    uint32_t width, uint32_t height,
                    int32_t colorkey, int32_t count);

/* ─── primary surface descriptor + create ────────────────────────── */

/* Sub-set of DDSURFACEDESC2 the primary-surface CreateSurface call
 * fills.  Only three fields vary by mode_arg/videomem_flag — the
 * builder below returns these three; the Win32 leg stamps them into
 * a zeroed DDSURFACEDESC2 with dwSize = 0x7c and feeds it to
 * IDirectDraw7::CreateSurface. */
typedef struct zdd_primary_desc_build {
    /* DDSD dwFlags — 0x21 (DDSD_CAPS | DDSD_BACKBUFFERCOUNT) for the
     * flippable modes (Full / DB / Zoom), 0x01 (DDSD_CAPS) for Safe. */
    uint32_t dwFlags;
    /* ddsCaps.dwCaps — combination of DDSCAPS_PRIMARYSURFACE (0x200) +
     * DDSCAPS_FLIP (0x10) + DDSCAPS_COMPLEX (0x8) for flippable modes
     * (= 0x218), plus DDSCAPS_VIDEOMEMORY (0x800) when mode 0 is paired
     * with videomem_flag (= 0xa18).  Safe mode is DDSCAPS_PRIMARYSURFACE
     * alone (0x200).  Modes 3/4 use 0x218 unconditionally — videomem
     * is honoured at the per-ZDDObject orchestrator layer in those
     * modes, not the primary. */
    uint32_t dwCaps;
    /* dwBackBufferCount — 1 for flippable modes (Full/DB/Zoom), 0 for
     * Safe (the dwFlags omits DDSD_BACKBUFFERCOUNT in that branch but
     * we stash 0 for cleanliness). */
    uint32_t dwBackBufferCount;
} zdd_primary_desc_build;

/* FUN_005b8480 lines 56-86 — pure-logic descriptor build for the
 * primary surface, factored out of the Win32 leg so the per-mode
 * switch is testable on host without ddraw.h.  Branches:
 *   mode_arg == 0 (Full):   {0x21, videomem_flag ? 0xa18 : 0x218, 1}
 *   mode_arg == 1 (Safe):   {0x01, 0x200, 0}
 *   mode_arg == 2 (Wind):   undefined — caller must skip CreateSurface
 *   mode_arg == 3 (DB):     {0x21, 0x218, 1}
 *   mode_arg == 4 (Zoom):   {0x21, 0x218, 1}
 *
 * Mode 2 is handled by the caller (zdd_create_screen) which releases
 * any existing self->com_a and never calls into this builder. */
void zdd_build_primary_surface_desc(int mode_arg, int videomem_flag,
                                    zdd_primary_desc_build *out);

/* FUN_005b8480 — internal mode-aware surface-init.  Pure-logic
 * orchestration that runs the full screen-init sequence for the
 * requested mode:
 *
 *   1. zdd_release_children(self)                       [release prior]
 *   2. stamp params on the ZDD: videomem_flag, screen_pos_x/y=0,
 *      width, height, mode_arg, rect_blob (from opt_rect7 or zeros)
 *   3. mode_arg == 2 (Windowed):
 *        zdd_com_release(&self->com_a)
 *      else:
 *        zdd_build_primary_surface_desc(...) + zdd_create_primary_surface()
 *        → on failure: DDERR logged, return 0
 *   4. stamp pixel_format_bpp = bpp; if (bpp == 8) zdd_setup_8bit_palette()
 *   5. allocate primary_obj (operator_new + ctor)
 *   6. per-mode wiring:
 *        mode 0 (Full):       attach_backbuffer(primary_obj, com_a, w, h, videomem_flag)
 *        mode 1/2 (Safe/Wind): create_surface_pair(primary_obj, w, h, 0,
 *                              sentinel, videomem_flag, 0, 0, w, h)
 *        mode 3 (DB):         alloc back_obj_a + attach_backbuffer to it,
 *                              then create_surface_pair(primary_obj, ...)
 *        mode 4 (Zoom):       alloc back_obj_a + attach_backbuffer (rect[0],
 *                              rect[1], 0), alloc back_obj_b + create_surface_pair
 *                              (rect[5], rect[6], ..., force_vm=1), then
 *                              create_surface_pair(primary_obj, ..., force_vm=1)
 *   7. on success: zdd_object_attach_clipper(primary_obj); if (bpp == 16)
 *      TODO 16bpp pixel-format bind (FUN_005b8a20 unported, ECX identity
 *      ambiguous — flagged in HANDOFF open RE threads).  Return 1.
 *   8. on per-mode failure: release the slot whose orchestrator failed
 *      (matches retail's "release just the latest failure" cleanup
 *      pattern — prior slots leak; the caller exits the process
 *      shortly after this returns 0).
 *
 * Args:
 *   width / height     param_1 / param_2 — requested screen size
 *   bpp                param_3 — 8/16/24/32 (stamped on ZDD)
 *   mode_arg           param_4 — 0..4 (Full/Safe/Windowed/DB/Zoom)
 *   videomem_flag      param_5 — forces VRAM placement (mode 0 only
 *                                 folds into primary; other modes
 *                                 pass through to the orchestrator)
 *   opt_rect7          param_6 — pointer to 7 int32_t rect entries
 *                                 (mode 4 Zoom only); NULL zeroes the
 *                                 ZDD's rect blob
 *
 * Returns 1 on success, 0 on any failure (DDERR logged via self if
 * the failure was a Win32 call). */
int  zdd_create_screen(zdd *self,
                       uint32_t width, uint32_t height,
                       uint32_t bpp, int mode_arg, int videomem_flag,
                       const int32_t *opt_rect7);

/* Allocate (calloc) + in-place ctor a fresh ZDDObject bound to
 * `parent`.  Returns NULL on OOM.  Defensive null-on-OOM matches
 * `zdd_object_new`'s style (retail's operator_new returns NULL on
 * OOM and the subsequent `if (pvVar2 != NULL)` skips the ctor; the
 * caller is then liable for crashing on the next deref — we return
 * NULL all the way out instead). */
zdd_object *zdd_object_alloc_and_ctor(zdd *parent);

/* FUN_005b8e00 — 8bpp palette setup.  Pure-logic orchestration:
 *   1. zdd_create_system_palette(self)   — sample the desktop's
 *                                          current 256-entry palette
 *                                          via GetSystemPaletteEntries
 *                                          and wrap it as a
 *                                          DDPCAPS_8BIT IDirectDrawPalette
 *                                          stashed in self->com_b.
 *      On failure: DDERR logged via self, return 0.
 *   2. if (self->com_a != NULL):
 *        zdd_surface_set_palette(com_a, com_b, self)
 *      Retail's null-check is `if (com_a != 0) SetPalette(...)` — the
 *      primary may legitimately be NULL during ctor-only test paths,
 *      so we mirror it.  On SetPalette failure: DDERR logged + return 0.
 *   3. Return 1.
 *
 * Called by FUN_005b8480 when the requested bpp is 8.  The palette
 * gets bound to the primary display surface that FUN_005b8480's mode
 * 0/3/4 branches just allocated via `ddraw7->CreateSurface(..., &com_a,
 * NULL)`, so com_a is non-NULL by the time this runs in the boot
 * sequence.  Caller (FUN_005b8480) ignores the return value — palette
 * setup is best-effort.
 *
 * Pins the role of self->com_a as the ZDD's primary display surface
 * (IDirectDrawSurface7), a previously-open RE thread. */
int  zdd_setup_8bit_palette(zdd *self);

/* FUN_005b9740 — back-buffer attach via GetAttachedSurface.  Pure-logic
 * orchestration that fetches the IDirectDrawSurface7 the engine
 * Flip()s alongside the primary, stashes it in self->com_primary, and
 * stamps the per-attachment metrics + a sentinel (no-color-key) state.
 * Used by FUN_005b8480 mode 0 (Full), mode 3 (DB Mode), and the first
 * leg of mode 4 (Zoom) — each mode allocates a fresh ZDDObject and
 * binds it to a back-buffer that was implicitly created when the
 * primary was made with a non-zero backBufferCount.
 *
 * Sequence (matches retail byte order):
 *   1. zdd_object_prefill_desc(self, 0, 0)    [caps_in=0, force_vm_in=0
 *                                              — back-buffers don't go
 *                                              through CreateSurface]
 *   2. Build DDSCAPS2 with dwCaps =
 *        DDSCAPS_BACKBUFFER (4)  OR
 *        DDSCAPS_BACKBUFFER | DDSCAPS_VIDEOMEMORY (0x804) when
 *        force_videomem != 0
 *   3. zdd_get_attached_surface(primary, caps, &self->com_primary,
 *                               self->parent)
 *      → on failure: DDERR logged + return 0
 *   4. zdd_object_stamp_metrics(self, width, height, 0, 0, width, height)
 *   5. return zdd_object_set_color_key(self, 0x1ffffff)  [sentinel; always 1]
 *
 * Note: this writes the back-buffer surface into the +0x2c slot
 * (com_primary, the same slot CreateSurface uses for surface objects).
 * The naming "com_primary" reflects the slot role on ZDDObjects that
 * own a CreateSurface result; for ZDDObjects holding a back-buffer
 * attachment, it's the attached surface itself.  Both implement
 * IUnknown so the dtor's release path doesn't care. */
int  zdd_object_attach_backbuffer(zdd_object *self,
                                  void *primary_surface,
                                  uint32_t width, uint32_t height,
                                  int force_videomem);

/* FUN_005b9520 — create + attach an IDirectDrawClipper to this
 * ZDDObject's primary surface.  Pure-logic orchestration over the
 * Win32 primitives below.  Retail sequence (matches byte order):
 *   1. Release any existing com_back via vtable[2] (Release).  Even
 *      though com_back is named for "back-buffer surface", this
 *      function re-purposes the same +0xac slot for the IDirectDrawClipper
 *      — both implement IUnknown so Release works for either role.
 *   2. self->parent->ddraw7->CreateClipper(0, &com_back, NULL)  [vtable[4]]
 *   3. com_back->SetClipList(&NULL_ptr, 0)                       [vtable[7]]
 *      — retail passes the address of a stack-local NULL pointer,
 *        which DDraw reads as an empty/invalid RGNDATA pointer.
 *        ddraw-init.md flags this offset as ambiguous (could be
 *        SetHWnd at vtable[8] depending on the actual asm — needs
 *        Frida verification).  We mirror the literal vtable+0x1c
 *        recovery; semantic role pending Frida-trace confirmation.
 *   4. com_primary->SetClipper(com_back)                          [vtable[28]]
 *
 * No return value (retail is void).  All COM failures are silently
 * dropped at this layer. */
void zdd_object_attach_clipper(zdd_object *self);

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

/* FUN_005b8dd0 — hide the cursor exactly once during a fullscreen
 * mode transition.  Inverse of zdd_restore_cursor_on_release.  Gate
 * on cursor_state == 1 (currently shown): calls zdd_show_cursor(0)
 * and stamps cursor_state back to 0.  Idempotent — does nothing when
 * the cursor was already hidden (cursor_state == 0).  Called by
 * FUN_00582e90 between SetDisplayMode and SetCooperativeLevel in the
 * fullscreen branches (modes 0/1/3/4).
 *
 * Note: retail reads the gate as `*(int*)(self + 0x130) != 0` and the
 * body sets it back to 0 — i.e., the field is "is the cursor currently
 * shown?", so 1→0 on hide and 0→1 on restore.  Mirrors that exactly. */
void zdd_hide_cursor(zdd *self);

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

/* Real build: build a primary-flavoured DDSURFACEDESC2 from `desc`
 * (dwSize=0x7c + dwFlags + ddsCaps.dwCaps + dwBackBufferCount), call
 * IDirectDraw7::CreateSurface vtable[6] / byte 0x18, stash the result
 * in self->com_a.  Logs DDERR via self on failure; returns 0.  Host
 * build: returns g_dd_create_primary_result + stashes the pre-staged
 * handle. */
int  zdd_create_primary_surface(zdd *self,
                                const zdd_primary_desc_build *desc);

/* Real build: GetSystemPaletteEntries(NULL, 0, 256, &entries[256]) +
 * IDirectDraw7::CreatePalette via vtable[5] (byte 0x14) with
 * DDPCAPS_8BIT (4).  Stashes the new IDirectDrawPalette into
 * self->com_b.  On failure: zdd_log_dderr("DirectDraw",
 * "CreatePalette", hr); returns 0.  Host build: returns
 * g_dd_create_palette_result + stashes the pre-staged handle. */
int  zdd_create_system_palette(zdd *self);

/* Real build: IDirectDrawSurface7::SetPalette via vtable[31] (byte
 * 0x7c).  NULL surface or NULL palette returns 0 silently.  On
 * vtable failure: zdd_log_dderr("DirectDrawSurface", "SetPalette",
 * hr) via log_owner; returns 0.  Host build: records the call. */
int  zdd_surface_set_palette(void *surface, void *palette,
                             zdd *log_owner);

/* Real build: IDirectDrawSurface7::GetAttachedSurface via vtable[12]
 * (byte 0x30).  Builds a DDSCAPS2 with caps[0] = `caps_in` (the engine
 * only ever sets the first dword — back-buffer flag + optional
 * videomem) and zero-padded caps[1..3].  On success, *out holds the
 * fetched IDirectDrawSurface7 and 1 is returned.  On failure: logs
 * DDERR via `log_owner` and returns 0.  NULL primary or NULL out
 * returns 0 silently.  Host build: returns g_dd_attached_result + the
 * pre-staged handle. */
int  zdd_get_attached_surface(void *primary, uint32_t caps_in,
                              void **out, zdd *log_owner);

/* Real build: IDirectDraw7::CreateClipper via vtable[4] (byte 0x10).
 * Args (parent->ddraw7, 0 dwFlags, &out, NULL pUnkOuter).  On
 * success, *out holds the new IDirectDrawClipper*.  On failure, *out
 * is set to NULL (defensive; retail leaves it undefined and trusts
 * the call).  Host build: returns the fake clipper from a global. */
void zdd_create_clipper(zdd *parent, void **out_clipper);

/* Real build: IDirectDrawClipper::SetClipList via vtable[7] (byte
 * 0x1c — verified by r2 disasm at 0x5b95a7).  Builds a stack-local
 * RGNDATA = {RGNDATAHEADER{dwSize=0x20, iType=RDH_RECTANGLES,
 * nCount=1, nRgnSize=0x10, rcBound={0,0,w,h}}, RECT{0,0,w,h}} and
 * passes its address to SetClipList — matches retail's layout in
 * FUN_005b9520 exactly.
 *
 * The earlier port used a NULL stack-local pointer here; under
 * DDSCL_NORMAL the clipper then has no cliplist and every subsequent
 * Blt fails with DDERR_NOCLIPLIST (0x887600CD).  Surfaced by the
 * drop-in's mode-2 smoke-present; fixed here.
 *
 * Host build: records the call shape (w, h) so tests can assert. */
void zdd_clipper_set_clip_list_rect(void *clipper,
                                    uint32_t width, uint32_t height);

/* Real build: IDirectDrawSurface7::SetClipper via vtable[28] (byte
 * 0x70).  Attaches `clipper` to `surface`.  NULL on either side is
 * a no-op.  Host build: records the call. */
void zdd_surface_set_clipper(void *surface, void *clipper);

/* FUN_005b8900 — IDirectDraw7::SetDisplayMode via vtable[21] (byte
 * 0x54).  Switches into the requested mode (width, height, bpp,
 * refresh_hz).  Retail's call passes (w, h, bpp, refresh, 0) — the
 * trailing 0 is the dwFlags arg (DDSDM_STANDARDVGAMODE etc., always
 * zero in this engine).  Returns 1 on success, 0 on failure (and
 * logs via zdd_log_dderr through `self` as "DirectDraw.SetDisplayMode").
 * Host build: returns g_dd_setmode_result + records the last call. */
int  zdd_set_display_mode(zdd *self, uint32_t width, uint32_t height,
                          uint32_t bpp, uint32_t refresh_hz);

/* FUN_005b8950 — IDirectDraw7::GetDisplayMode via vtable[12] (byte
 * 0x30).  Builds a stack DDSURFACEDESC2 with dwSize=0x7c and
 * dwFlags=0x41006 (DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_PIXELFORMAT)
 * plus a pre-stamped ddpf header (dwSize=0x20, dwFlags=DDPF_RGB),
 * then asks DDraw to fill the result.  Out-pointer behaviour matches
 * retail's: when each pointer is non-NULL, its slot is written:
 *   out_width  ← ddsd.dwWidth
 *   out_height ← ddsd.dwHeight
 *   out_pitch  ← ddsd.lPitch (DDSD offset 0x10)
 * Returns 1 on success, 0 on failure (silent — no DDERR log; retail's
 * caller logs separately via the "It_failed_in_the_display_mode_ac"
 * fixed-string path).
 *
 * FUN_00582e90's mode 4 (Zoom) calls this with (&w, &h, NULL) to ask
 * for the desktop's current dimensions when the user hasn't pinned an
 * override via DAT_008a6eac/eb0. */
int  zdd_get_display_mode(zdd *self, uint32_t *out_width,
                          uint32_t *out_height, uint32_t *out_pitch);

/* FUN_005b5ac0 — busy-wait spin via GetTickCount.  Polls GetTickCount
 * in a tight loop until elapsed >= ms.  Used by FUN_00582e90 between
 * SetDisplayMode and the surface-create call (mode 0/1/3/4 all pause
 * 2000 ms here) — looks like a "wait for the mode transition to
 * settle" workaround for early-2010s GPU drivers.
 *
 * Real build: actual GetTickCount spin.  Host build: instantly returns
 * + records the ms argument so tests can assert the value without
 * actually sleeping. */
void zdd_busy_wait_ms(uint32_t ms);

/* ─── present-dispatcher trio: FUN_005b94e0 / _9500 / _8fc0 ──────────
 *
 * The "push the offscreen surface to the display" path the engine fires
 * each frame.  Ghidra labels FUN_005b94e0 / _9500 as `paint_ctx::`
 * methods, but the ECX in every live callsite is a `zdd_object*`
 * (specifically `zdd.primary_obj` — see WM_PAINT handler FUN_005b9130
 * and FUN_005b8fc0 case 2), NOT a separate "paint_ctx" class.  We name
 * accordingly.  The wnd_proc.h "paint_ctx" struct is a misnomer for ZDD
 * itself (same +0x16c primary_obj / +0x138..+0x144 / +0x164 offsets);
 * its `+0x2c zdd_device` docstring is incorrect.
 */

/* FUN_005b94e0 — IDirectDrawSurface7::GetDC wrapper.  Reads
 * self->com_primary (+0x2c, verified by r2 disasm at 0x5b94e0) and
 * forwards to the COM call via vtable[17] (byte 0x44).
 *
 *   if (!self->com_primary) return 0;       // *out_hdc NOT written
 *   self->com_primary->GetDC(out_hdc);      // COM-side fills *out_hdc
 *   return 1;                                // retail's `mov eax, 1`
 *
 * Retail forces eax=1 regardless of the underlying COM HRESULT (the
 * literal `mov eax, 1; ret 4` at 0x5b94f5 — we mirror).  If GetDC
 * itself failed, *out_hdc ends up NULL and 1 is still returned; the
 * caller's BitBlt would then no-op rather than error here.  Bizarre
 * contract but faithful to retail. */
int  zdd_object_get_dc(zdd_object *self, void **out_hdc);

/* FUN_005b9500 — IDirectDrawSurface7::ReleaseDC wrapper.  Forwards to
 * vtable[26] (byte 0x68).  No NULL guard in retail (the contract is
 * "only call after a successful zdd_object_get_dc"); the underlying
 * Win32 primitive guards defensively.  Modelled as `void` — retail's
 * eax carries whatever ReleaseDC returned, but no caller reads it. */
void zdd_object_release_dc(zdd_object *self, void *hdc);

/* FUN_005b9a40 — generic "blit `self` onto `dest` at (dest_x, dest_y)".
 * Used by FUN_005b8fc0 mode 4 (Zoom) to composite the zoom intermediate
 * (back_obj_b) onto the back-buffer (back_obj_a) at a centred offset.
 * Also called from many gameplay sites (over a dozen XREFs).
 *
 *   if (!self->com_primary) return 1;          // degenerate-success
 *   dest_rect = {dest_x, dest_y,
 *                dest_x + self->metric_b8, dest_y + self->metric_bc}
 *   src_rect  = self->metric_b0..bc, interpreted as a 4-int RECT
 *               = {0, 0, self->metric_b8, self->metric_bc}
 *               (NOT src's metrics — retail uses *self's* metric_b0..bc
 *               which always hold {0, 0, w, h} for self's own surface)
 *   flags     = self->state_flag | DDBLT_WAIT
 *   return dest->com_primary->Blt(&dest_rect, self->com_primary,
 *                                 &src_rect, flags, NULL)
 *
 * Note the role inversion: `self` is the SOURCE surface; the COM call's
 * receiver is `dest->com_primary` (since Blt is called ON the dest).
 * The arg ordering — `(this=src, dest_obj, dest_x, dest_y)` — is how
 * retail names them, NOT `(this=dest, src_obj, dest_x, dest_y)`.
 *
 * Sanity-check via r2 at 0x5b9a8d (`add ecx, 0xb0`) + the call shape:
 * the +0xb0 source-rect block is on `self`, not on `dest`.  The "early
 * return 1 when self->com_primary is NULL" branch is retail's literal
 * behaviour (a degenerate no-op reading as success — quirky but
 * faithful).  Returns whatever the underlying Blt HRESULT was on the
 * non-degenerate path. */
int  zdd_object_blt_onto(zdd_object *self, zdd_object *dest,
                         int32_t dest_x, int32_t dest_y);

/* FUN_005b9b70 — variant of zdd_object_blt_onto with a non-zero
 * source-rect origin and an extra DDBLT_KEYSRC bit OR'd into the flags.
 * Called from the title-menu scene runner (FUN_0056aea0) when
 * 2 <= local_64 < 4 (studio-logo phases) to draw the logo bitmap onto
 * the primary surface with its color-keyed transparency honored.
 *
 *   if (!self->com_primary) return 1;          // degenerate-success
 *   dest_left   = self->metric_0c + dest_x
 *   dest_top    = self->metric_10 + dest_y
 *   dest_right  = self->metric_b8 + dest_left
 *   dest_bottom = self->metric_bc + dest_top
 *   src_rect    = self->metric_b0..bc = {0, 0, w, h}
 *   flags       = self->state_flag | DDBLT_KEYSRC (0x1000000)
 *   return dest->com_primary->Blt(&dest_rect, self->com_primary,
 *                                 &src_rect, flags, NULL)
 *
 * Key differences vs zdd_object_blt_onto:
 *  - dest origin shifted by self->metric_0c / metric_10 (per-object
 *    placement offset from FUN_005b98c0's metric stash)
 *  - flags carry DDBLT_KEYSRC instead of DDBLT_WAIT, meaning the source
 *    surface's color-key gets honored as transparency.  (Retail's
 *    state_flag carries DDBLT_KEYSRC alone — 0x8000 set by
 *    zdd_object_set_color_key when a non-sentinel key is bound — and
 *    this function OR's it again; effectively the same bit twice, but
 *    we mirror the literal.)
 *
 * Returns the underlying Blt HRESULT (0 success / non-zero error) on
 * the non-degenerate path, 1 on the degenerate self->com_primary==NULL
 * path. */
int  zdd_object_blt_keyed(zdd_object *self, zdd_object *dest,
                          int32_t dest_x, int32_t dest_y);

/* FUN_005b9490 — IDirectDrawSurface7::Lock wrapper.  Reads
 * self->com_primary (+0x2c), calls Lock with the surface's embedded
 * DDSURFACEDESC2 scratch (self->embedded_ddsd at +0x30) as the
 * lpDDSurfaceDesc out-param, dwFlags=1 (DDLOCK_WAIT), hEvent=NULL.
 *
 *   if (!self->com_primary) return 0;
 *   hr = self->com_primary->Lock(NULL, &self->embedded_ddsd, 1, NULL);
 *   if (hr != S_OK) {
 *       zdd_log_dderr(parent, "DirectDrawSurface", "", hr);
 *       return 0;
 *   }
 *   return 1;
 *
 * Retail's `s_DirectDrawSurface_008a4b54` and `DAT_008a4b84` (the empty
 * BSS string) are the log prefixes — same shape as every other DDERR
 * log site.  After a successful Lock, the embedded DDSD's lpSurface /
 * lPitch / dwHeight slots are populated; the three self-pointer fields
 * (ddsd_lpSurface_ref / pitch_ref / height_ref at +0x00..+0x08) point
 * into those slots — so callers read lpSurface as
 * `*(void**)self->ddsd_lpSurface_ref`, pitch as
 * `*(int*)self->ddsd_pitch_ref`, height as
 * `*(int*)self->ddsd_height_ref`.  Returns 1 on success, 0 on Lock
 * failure OR NULL com_primary. */
int  zdd_object_lock(zdd_object *self);

/* FUN_005b94d0 — IDirectDrawSurface7::Unlock wrapper.  Forwards to
 * vtable[32] with lpRect=NULL.  No NULL guard in retail (contract:
 * "only call after a successful zdd_object_lock"); the underlying
 * Win32 primitive guards defensively.  Modelled as void — retail's
 * return value is dropped. */
void zdd_object_unlock(zdd_object *self);

/* FUN_005b9410 — "clear the entire surface to zero".  Locks the
 * surface, computes pixel byte count from the post-Lock DDSD
 * (`pitch * height` from the +0x40 / +0x38 reads on the zdd_object,
 * which alias embedded_ddsd[0x10] = lPitch and embedded_ddsd[0x08] =
 * dwHeight), zero-fills the surface buffer (DWORD chunks then byte
 * remainder), unlocks.  No-op if Lock fails (retail: just skip the
 * fill + Unlock entirely).
 *
 * Called by the title-menu scene runner (FUN_0056aea0) at the start of
 * local_64 == 0 (studio-logo phase 0) — a blank frame before the
 * studio logo fades in.  Acts on the back-buffer surface (back_obj_a)
 * in retail. */
void zdd_object_clear(zdd_object *self);

/* FUN_005b8b00 — 16bpp color-channel converter.  Takes an RGB888-style
 * input value (typically a colorkey passed by retail's higher layers
 * as a literal `0x00RRGGBB`) and packs it into the surface's native
 * pixel layout using the descriptor at self->color_desc:
 *
 *   r_byte = (input >> 16) & 0xff
 *   g_byte = (input >>  8) & 0xff
 *   b_byte = (input >>  0) & 0xff
 *   r_out  = (r_byte >> color_desc.shift_right[0]) << color_desc.shift_left[0]
 *   g_out  = (g_byte >> color_desc.shift_right[1]) << color_desc.shift_left[1]
 *   b_out  = (b_byte >> color_desc.shift_right[2]) << color_desc.shift_left[2]
 *   return r_out | g_out | b_out
 *
 * For an RGB565 surface (R=0xF800 / G=0x07E0 / B=0x001F), this maps
 * (R, G, B) 8-bit channels to a 16-bit packed value with the top
 * 5/6/5 bits placed.  Retail's "& 0x1f" on shift counts is a no-op for
 * the values the descriptor builder writes (always 0..7) but we
 * mirror it (a defensive no-op against undefined shifts).
 *
 * Called by zdd_object_set_color_key's 16bpp branch (FUN_005b9830) and
 * by some scene-runner sites that pack ad-hoc colors. */
uint32_t zdd_color_convert(zdd *self, uint32_t input_rgb);

/* FUN_005b8a20 — pixel-format binding.  Calls GetSurfaceDesc on the
 * supplied surface, reads the embedded DDPIXELFORMAT's R/G/B bitmasks,
 * and stamps self->color_desc with the precomputed shifts that
 * zdd_color_convert needs.  Per channel:
 *
 *   shift_left[ch]  = count of trailing zeros in mask_raw[ch]
 *                     (where the channel sits in the output word)
 *   mask_lo[ch]     = (mask_raw[ch] >> shift_left[ch]) & 0xff
 *                     (significance bits after stripping placement)
 *   shift_right[ch] = 3 if mask_lo == 0x1f, else 2
 *                     (input >> shift to get top 5 or 6 bits)
 *
 * Called from zdd_create_screen's post-success path when
 * pixel_format_bpp == 16 (the only bpp that needs format-aware color
 * conversion).  Returns 1 on success, 0 if GetSurfaceDesc failed.
 *
 * NULL surface returns 0.  Side effect: descriptor bytes are stamped
 * IN PLACE — does not clear non-touched bytes (caller must zero-init
 * if state matters), but the ctor already zero-fills the whole zdd
 * struct. */
int  zdd_bind_pixel_format(zdd *self, void *surface);

/* FUN_005b8fc0 — 5-mode per-frame present dispatcher.  Switches on
 * self->pixel_format_mode (the launcher mode 0..4) and dispatches to
 * one of:
 *
 *   mode 0 (Full):     com_a->Flip(primary_obj->com_primary, DDFLIP_WAIT)
 *   mode 1 (Safe):     com_a->Blt(&self->screen_pos_x_as_RECT,
 *                                 primary_obj->com_primary,
 *                                 &self->screen_pos_x_as_RECT,
 *                                 DDBLT_WAIT, NULL)
 *   mode 2 (Windowed): zdd_object_get_dc(primary_obj, &src_hdc) +
 *                      zdd_desktop_present(src_hdc, screen_pos_x,
 *                                          screen_pos_y, screen_width,
 *                                          screen_height) +
 *                      zdd_object_release_dc(primary_obj, src_hdc)
 *   mode 3 (DB):       back_obj_a->com_primary->Blt(rect,
 *                          primary_obj->com_primary, rect,
 *                          DDBLT_WAIT, NULL)
 *                      THEN com_a->Flip(back_obj_a->com_primary,
 *                                       DDFLIP_WAIT)
 *   mode 4 (Zoom):     FUN_005b8ea0(self, back_obj_b, primary_obj,
 *                                   screen_rect[2])    // SW scaler
 *                      zdd_object_blt_onto(back_obj_b, back_obj_a,
 *                                          screen_rect[3], screen_rect[4])
 *                      THEN com_a->Flip(back_obj_a->com_primary,
 *                                       DDFLIP_WAIT)
 *
 * The mode 3 / mode 4 common tail at retail 0x5b903d is the Flip on
 * `back_obj_a->com_primary`.  Modes 0 / 1 / 2 return directly without
 * the common tail.  Default branch (mode > 4) is a no-op.
 *
 * The RECT used by modes 1 & 3 is read as `&self->screen_pos_x` (4
 * contiguous int32s = {screen_pos_x, screen_pos_y, screen_width,
 * screen_height}, interpreted by DDraw as {left, top, right, bottom}).
 * In fullscreen modes screen_pos_x/y are 0, so this resolves to
 * {0, 0, w, h} — the full surface.  Windowed mode wouldn't fit this
 * RECT convention, but mode 2 doesn't use this builder.
 *
 * NOTE: the software scaler `FUN_005b8ea0` (mode 4 leaf, 285 bytes of
 * 16bpp software upscale + Lock/Unlock via FUN_005b9490/_94d0) is NOT
 * YET PORTED.  Mode 4 in this port skips the scaler and proceeds
 * straight to the blit_onto + Flip — visible artefact when mode 4 runs
 * live (back_obj_b's contents stay frozen from boot, so back_obj_a
 * receives a blank stamp).  Mode 4 isn't the live boot mode (mode 2 is);
 * this surfaces only when the launcher selects Zoom.
 *
 * Returns void; retail callers don't read a return value.  Per-step
 * failures from Win32 primitives are silently dropped here (the Win32
 * leg logs DDERR via OutputDebugStringA, which our wrapper dual-sinks
 * to stderr — see zdd_win32.c). */
void zdd_present(zdd *self);

/* ─── present-dispatcher Win32 primitives ────────────────────────────
 *
 * The pure-logic dispatcher above forwards to these.  Each has a host
 * stub in tests/test_zdd.c so dispatcher tests can assert which leg
 * fired with which args. */

/* IDirectDrawSurface7::Flip via vtable[11] (byte 0x2c — verified by r2
 * disasm at 0x5b904f / 0x5b906d).  `target` may be NULL (lets DDraw
 * pick the next back buffer in a back-buffer chain); in retail it's
 * always the attached child surface's COM pointer.  `flags` is
 * DDFLIP_WAIT (1) in every retail callsite.  Returns 1 on success, 0
 * on failure with DDERR logged via `log_owner`.  NULL surface returns
 * 0 silently. */
int  zdd_surface_flip(void *surface, void *target, uint32_t flags,
                      zdd *log_owner);

/* IDirectDrawSurface7::Blt via vtable[5] (byte 0x14 — verified by r2
 * at 0x5b903a / 0x5b9098 / 0x5b9aa5).  `dest_rect` and `src_rect` are
 * 4-int RECT pointers (left, top, right, bottom); either may be NULL
 * (full surface).  `flags` typically carries DDBLT_WAIT (0x1000000)
 * plus the source surface's state_flag for blits via FUN_005b9a40;
 * we don't add anything implicitly.  `ddbltfx` is currently NULL in
 * every consumer (the DDBLT_COLORFILL path uses a separate primitive
 * — see zdd_surface_blt_color_fill below).  Returns 1 on success, 0
 * on failure with DDERR logged via `log_owner`.  NULL dest returns
 * 0 silently. */
int  zdd_surface_blt(void *dest, const int32_t *dest_rect,
                     void *src,  const int32_t *src_rect,
                     uint32_t flags, zdd *log_owner);

/* Desktop-DC composite: GetDC(NULL) + BitBlt(dest_hdc, ...) + ReleaseDC.
 * Used by FUN_005b8fc0 case 2 to push the windowed-mode offscreen
 * surface to the desktop at the window's screen position.  src_hdc is
 * the HDC returned by zdd_object_get_dc (the surface's GDI DC).
 * Returns 1 on success, 0 if GetDC(NULL) failed.  NULL src_hdc returns
 * 0 silently. */
int  zdd_desktop_present(void *src_hdc, int dest_x, int dest_y,
                         int width, int height);

/* IDirectDrawSurface7::GetSurfaceDesc via vtable[22] (byte 0x58 —
 * verified by r2 disasm of FUN_005b8a20: `call dword [eax + 0x58]`).
 * Caller stamps `ddsd[0] = dwSize = 0x7c` and `ddsd[4] = dwFlags =
 * 0x1000 (DDSD_PIXELFORMAT)` before the call.  On success, the entire
 * 124-byte DDSURFACEDESC2 is filled; bind_pixel_format only reads the
 * embedded DDPIXELFORMAT R/G/B masks at offsets 0x58..0x60 inside the
 * descriptor.  Returns 0 if the call returned non-S_OK; returns 1
 * otherwise.  No DDERR log — retail's FUN_005b8a20 silently drops the
 * failure (returns 0 to its caller, which has no observable consumer
 * in the boot path).  NULL surface or NULL ddsd returns 0. */
int  zdd_surface_get_desc(void *surface, void *ddsd);

/* IDirectDrawSurface7::Lock via vtable[25] (byte 0x64 — verified by r2
 * disasm of FUN_005b9490: `call dword [eax + 0x64]`).  Args mirror
 * retail exactly: (this, lpDestRect=NULL, lpDDSurfaceDesc, dwFlags=1
 * (DDLOCK_WAIT), hEvent=NULL).  The DDSURFACEDESC2 pointed to by
 * `ddsd` MUST have its dwSize stamped before the call — retail leaves
 * this to the caller (FUN_005b9490 hands the surface's embedded
 * scratch DDSD, pre-stamped 0x7c by FUN_005b97e0).  Returns 0 if the
 * underlying Lock returned non-S_OK (DDERR_SURFACELOST etc.); returns
 * the HRESULT in *out_hr so callers can log it via zdd_log_dderr.
 * Returns 1 on success.  NULL surface returns 0 with *out_hr untouched. */
int  zdd_surface_lock(void *surface, void *ddsd, uint32_t flags,
                      int32_t *out_hr);

/* IDirectDrawSurface7::Unlock via vtable[32] (byte 0x80 — verified by
 * r2 disasm of FUN_005b94d0: `call dword [eax + 0x80]`).  Args
 * (this, lpRect=NULL) — retail's FUN_005b94d0 ALWAYS passes NULL for
 * lpRect, meaning "release the entire surface lock".  Return value is
 * ignored by retail.  NULL surface is a silent no-op. */
void zdd_surface_unlock(void *surface);

/* Extract post-Lock DDSD slots (lpSurface, lPitch, dwHeight) from a
 * zdd_object's embedded scratch DDSD.  Retail reads these directly via
 * `*(void**)(this+0x54)` / `*(int*)(this+0x40)` / `*(int*)(this+0x38)`
 * — the DDSD offsets 0x24/0x10/0x08 added to the embedded_ddsd's
 * +0x30 base.  This primitive exists for host portability: in the
 * Win32 build it just reads those offsets directly (pointer is 4
 * bytes, same as retail).  In the host build a test stub publishes
 * canned values so the layout mismatch (8-byte pointers, struct sized
 * to fit them) doesn't bleed into the pure-logic test path.
 *
 * Out pointers may be NULL (skip that slot's write).  Returns 1 if
 * extraction succeeded, 0 if `self` is NULL.  Does NOT itself call
 * Lock — caller must have already called zdd_object_lock. */
int  zdd_object_get_locked_info(zdd_object *self, void **out_buf,
                                int32_t *out_pitch, int32_t *out_height);

/* IDirectDrawSurface7::GetDC via vtable[17] (byte 0x44).  Underlying
 * primitive that zdd_object_get_dc forwards to after dereferencing
 * self->com_primary.  Tests use this directly for raw-COM scenarios;
 * the pure-logic dispatcher uses zdd_object_get_dc. */
int  zdd_surface_get_dc(void *surface, void **out_hdc);

/* IDirectDrawSurface7::ReleaseDC via vtable[26] (byte 0x68).  Pair
 * with zdd_surface_get_dc; underlying primitive that
 * zdd_object_release_dc forwards to.  NULL surface or NULL hdc is a
 * no-op. */
void zdd_surface_release_dc(void *surface, void *hdc);

/* IDirectDrawSurface7::Blt with DDBLT_COLORFILL + DDBLT_WAIT.  Fills
 * the entire surface (NULL lpDestRect) with `fill_value`.  Retained as
 * a separate primitive — the dispatcher's mode 0/1/3 Blt paths use a
 * source surface (not a colorfill), and DDraw's Blt API forces COLORFILL
 * to use lpDDBltFx instead of lpDDSrcSurface.  Kept available for
 * future debugging / smoke paths even though zdd_present no longer
 * calls it. */
int  zdd_surface_blt_color_fill(void *surface, uint32_t fill_value,
                                zdd *log_owner);

/* USER32 BeginPaint wrapper.  Returns the HDC for painting the window
 * and stashes OS-internal paint state in *out_cookie (heap-allocated
 * PAINTSTRUCT in the real build; opaque sentinel in the host stub).
 * The cookie's lifetime spans the begin/end pair — caller MUST pass it
 * unchanged to zdd_window_paint_end on the same hwnd, otherwise the
 * window's update region will not be marked validated and Windows will
 * redeliver WM_PAINT forever.  NULL hwnd returns NULL and writes NULL
 * to *out_cookie.  Retail's FUN_005b9130 doesn't NULL-check the return;
 * we pass it through to the BitBlt and EndPaint primitives, both of
 * which guard defensively. */
void *zdd_window_paint_begin(void *hwnd, void **out_cookie);

/* USER32 EndPaint wrapper.  Pairs with zdd_window_paint_begin: tells
 * the OS the WM_PAINT is consumed (validates the dirty region) and
 * frees the cookie.  NULL hwnd or NULL cookie is silently dropped. */
void zdd_window_paint_end(void *hwnd, void *cookie);

/* GDI BitBlt with SRCCOPY raster op (0xCC0020 — matches the literal
 * push at FUN_005b9130:0x5b9173 and FUN_005b8fc0:0x5b90c7).  Both HDCs
 * are caller-supplied; this primitive does NOT acquire either — unlike
 * zdd_desktop_present which wraps GetDC(NULL) internally.  Used by
 * FUN_005b9130's WM_PAINT path where the dest HDC comes from BeginPaint
 * and the src HDC comes from the DDraw surface via zdd_object_get_dc.
 * NULL dest or NULL src is a silent no-op. */
void zdd_window_blit_copy(void *dest_hdc, int dest_x, int dest_y,
                          int width, int height, void *src_hdc);

/* FUN_005b9130 — WM_PAINT handler for the windowed-mode game surface
 * (151 bytes in retail).  Called by the engine's WndProc dispatch
 * (wp_handle_message via the wp_paint_check hook) when Windows
 * delivers WM_PAINT to the game window after another app uncovers it
 * or the OS otherwise invalidates the client area.
 *
 * Sequence (matches retail FUN_005b9130 step-for-step):
 *
 *   if (self->pixel_format_mode != 2) return 0      // only mode-2 consumes
 *   dest_hdc = zdd_window_paint_begin(hwnd, &cookie)
 *   zdd_object_get_dc(self->primary_obj, &src_hdc)
 *   zdd_window_blit_copy(dest_hdc, screen_pos_x, screen_pos_y,
 *                        screen_width, screen_height, src_hdc)
 *   zdd_object_release_dc(self->primary_obj, src_hdc)
 *   zdd_window_paint_end(hwnd, cookie)
 *   return 1
 *
 * Returns 1 iff the WM_PAINT was consumed — the caller's WndProc must
 * then return 0 to the OS (we own the dirty region's validation via
 * EndPaint).  Returns 0 in three cases: NULL self, mode != 2, or NULL
 * primary_obj — caller must then fall back to DefWindowProcA so
 * Windows validates the update region itself.  Retail's only
 * early-out is the `mode != 2` check; the NULL-self / NULL-primary_obj
 * guards are defensive (retail would crash on the vtable deref inside
 * FUN_005b94e0).
 *
 * QUIRK (retail-faithful): the BitBlt uses screen_pos_x/y/width/height
 * (the same +0x138..+0x144 fields the per-frame present at FUN_005b8fc0
 * case 2 uses) as the destination coordinates inside a window-DC
 * returned by BeginPaint.  In our drop-in, screen_pos_x/y hold the
 * window's client top-left in SCREEN coordinates (kept in sync via
 * main.c's sync_window_position).  But BeginPaint returns an HDC whose
 * origin is (0,0) in CLIENT coordinates — so for a window not at
 * desktop (0,0), the WM_PAINT BitBlt lands at offset (screen_x,
 * screen_y) WITHIN the client area, i.e. outside the visible region.
 * Retail does this too; we mirror.  In practice the per-frame
 * zdd_present at FUN_005b8fc0 fires every frame and re-paints the
 * window correctly via GetDC(NULL) at the same coords on the desktop
 * DC, so this helper's misaligned BitBlt is overwritten before any
 * frame fully composites.  Treating it as a retail quirk worth
 * flagging but not "fixing" — that's a renderer-side decision. */
int zdd_window_paint(zdd *self, void *hwnd);

#endif /* OPENSUMMONERS_ZDD_H */
