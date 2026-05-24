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

/* Opaque ZDDObject — owned by the ZDD as primary + 2 back-buffer
 * children.  Real shape lives behind FUN_005b9350 (ZDDObject::ctor,
 * 0xd8-byte struct per docs/findings/ddraw-init.md FUN_005b8b40), which
 * isn't ported yet.  Treated as an opaque heap allocation here. */
typedef struct zdd_object zdd_object;

/* Field layout inferred from FUN_005b7f80 (ctor writes), FUN_005b7fe0
 * (dtor reads), and FUN_005b8040 (5-slot release loop).  Size 0x170 is
 * the literal `operator_new(0x170)` argument in FUN_005b7ee0. */
typedef struct zdd {
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

    /* +0x134..+0x163: unobserved by ZDD-class methods we've decomp'd
     * so far.  Pad. */
    uint8_t       _pad134[0x164 - 0x134];

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
} zdd;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(zdd)                          == 0x170, "zdd must be 0x170 bytes");
_Static_assert(offsetof(zdd, back_obj_a)            == 0x018, "zdd back_obj_a offset");
_Static_assert(offsetof(zdd, back_obj_b)            == 0x01c, "zdd back_obj_b offset");
_Static_assert(offsetof(zdd, log_buf)               == 0x020, "zdd log_buf offset");
_Static_assert(offsetof(zdd, open_objects)          == 0x120, "zdd open_objects offset");
_Static_assert(offsetof(zdd, ddraw7)                == 0x124, "zdd ddraw7 offset");
_Static_assert(offsetof(zdd, com_a)                 == 0x128, "zdd com_a offset");
_Static_assert(offsetof(zdd, com_b)                 == 0x12c, "zdd com_b offset");
_Static_assert(offsetof(zdd, cursor_state)          == 0x130, "zdd cursor_state offset");
_Static_assert(offsetof(zdd, pixel_format_mode)     == 0x164, "zdd pixel_format_mode offset");
_Static_assert(offsetof(zdd, pixel_format_bpp)      == 0x168, "zdd pixel_format_bpp offset");
_Static_assert(offsetof(zdd, primary_obj)           == 0x16c, "zdd primary_obj offset");
#endif

/* ─── pure logic ─────────────────────────────────────────────────── */

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

/* Real build: full ZDDObject teardown (FUN_005b9390 cleanup chain +
 * FUN_005bef0e heap free).  Host build: decrements a counter + zeros
 * *p.  Idempotent on (*p == NULL).
 *
 * Stub-grade in the real build until FUN_005b9390 is ported — for now
 * it just heap-frees, matching the surface API the dtor needs. */
void zdd_obj_destroy(zdd_object **obj_pp);

/* Real build: DirectDrawCreateEx(NULL, &self->ddraw7, &IID_IDirectDraw7,
 * NULL).  On failure, calls zdd_log_dderr(self, "", "DirectDrawCreate",
 * hresult) and returns 0.  Host build: pulls the HRESULT (success or
 * failure) from a test-controlled state.
 *
 * Mirrors FUN_005b88c0. */
int  zdd_directdraw_create_ex(zdd *self);

/* Real build: vtable[20](ddraw7, hwnd, flags) where flags is
 * DDSCL_NORMAL (8) when fullscreen == 0, or
 * DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_ALLOWREBOOT (0x13) otherwise.
 * On failure, calls zdd_log_dderr(self, "DirectDraw", "SetCooperativeLevel",
 * hresult) and returns 0.  Host build: stub controllable from tests.
 *
 * Mirrors FUN_005b89d0. */
int  zdd_set_coop_level(zdd *self, void *hwnd, int fullscreen);

#endif /* OPENSUMMONERS_ZDD_H */
