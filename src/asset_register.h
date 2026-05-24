/*
 * src/asset_register.h — Asset-register module ("boot driver" batch).
 *
 * Ports the FUN_00579bd0 family that the engine's outer driver
 * (FUN_00562ea0) calls right after the Pixel-Drawer is set up.  This
 * is the first batch of asset-slot registrations: 8 GDI font handles,
 * 2 sprite slots (font textures), 4 brush/pen gradients.  See
 * `docs/findings/asset-loader.md` "Asset register calls in the boot
 * driver" for the surrounding context.
 *
 * Ported functions (one per address):
 *   - FUN_005bef0e   ar_xfree              (operator delete[] thunk)
 *   - FUN_005b5f50   ar_color_lerp         (per-channel COLORREF lerp)
 *   - FUN_00417b50   ar_sprite_slot_destroy
 *   - FUN_00562a10   ar_gdi_slot_destroy
 *   - FUN_00579ec0   ar_gdi_slot_reset
 *   - FUN_00579f40   ar_make_font          (LOGFONTA + CreateFontIndirectA)
 *   - FUN_0057a030   ar_gdi_slot_set_font
 *   - FUN_0057a1a0   ar_gdi_slot_set_pen
 *   - FUN_0057a260   ar_gdi_slot_set_brush
 *   - FUN_00582d10   ar_gdi_slot_set_pen_gradient
 *   - FUN_00579bd0   ar_register_fonts     (top-level driver call)
 *
 * Two slot families live in this module:
 *
 *   ar_gdi_slot     — Type B, 12 B, holds a fixed-capacity array of
 *                     HGDIOBJ handles (HFONT / HPEN / HBRUSH).  All
 *                     the asset-register slots at DAT_008a9274[idx]
 *                     use this shape.  The retail engine's individual
 *                     globals (DAT_008a9278..0x9294) are entries 1..8
 *                     of that table.
 *
 *   ar_sprite_slot  — Type A, 0x44 B, the sprite-registration shape
 *                     used by FUN_0056e190 et al.  Two are touched
 *                     here (DAT_008a76e8, _76ec) — they reserve font
 *                     texture pages at sprite IDs 0x457 / 0x455.
 *
 * Win32-free: this header doesn't drag in `<windows.h>`.  The GDI
 * primitive wrappers (`ar_gdi_create_font` etc.) are declared here
 * but DEFINED in either `asset_register_win32.c` (real build) or in
 * the test harness (recording stubs).  Pure logic is in
 * `asset_register.c`.
 */
#ifndef OPENSUMMONERS_ASSET_REGISTER_H
#define OPENSUMMONERS_ASSET_REGISTER_H

#include <stddef.h>
#include <stdint.h>

/* Opaque GDI handle.  In Win32 builds these alias HFONT / HPEN /
 * HBRUSH; in tests they're whatever the stub returns. */
typedef void *ar_gdi_handle;

/* ─── ar_gdi_slot — Type B (12 B) ────────────────────────────────── */

typedef struct ar_gdi_slot {
    /* +0x00 (4B): array of `capacity` HGDIOBJ entries.  Owned by us
     * (operator_new); freed via ar_gdi_slot_destroy.  NULL when the
     * slot has not been allocated yet. */
    ar_gdi_handle *array;
    /* +0x04 (2B): "count" — number of entries the slot's setter wrote
     * into `array`.  set_font leaves this at 0 (one entry but the
     * counter is never bumped — matches retail FUN_0057a030); set_pen
     * and set_brush bump it after the first entry, and the gradient
     * setter bumps it for each pen written. */
    uint16_t       count;
    /* +0x06 (2B): capacity — total number of entries `array` can hold. */
    uint16_t       capacity;
    /* +0x08 (2B): asset-group tag stamped by each setter.  Same uint16
     * the boot driver passes down (typically `param_2` of
     * FUN_00579bd0). */
    uint16_t       group;
    /* +0x0A (2B): pad — present so the next field would land 4-aligned;
     * no observed writes from RE. */
    uint16_t       _pad0a;
} ar_gdi_slot;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(ar_gdi_slot) == 12, "ar_gdi_slot must be 12 bytes");
_Static_assert(offsetof(ar_gdi_slot, count)    == 4,  "ar_gdi_slot count offset");
_Static_assert(offsetof(ar_gdi_slot, capacity) == 6,  "ar_gdi_slot capacity offset");
_Static_assert(offsetof(ar_gdi_slot, group)    == 8,  "ar_gdi_slot group offset");
#endif

/* ─── ar_sprite_slot — Type A (0x44 B) ───────────────────────────── */

/* One entry of the sprite slot's `entries` array.  The destructor
 * frees `b` (offset +4) iff non-zero; `a` is never freed.  The retail
 * register-time write only zeros both halves — entries are populated
 * later, presumably on first sprite draw. */
typedef struct ar_sprite_entry {
    uint32_t a;   /* +0x00 */
    void    *b;   /* +0x04 — owned pointer; ar_sprite_slot_destroy frees */
} ar_sprite_entry;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(ar_sprite_entry) == 8, "ar_sprite_entry must be 8 bytes");
#endif

typedef struct ar_sprite_slot {
    /* +0x00 (4B): owned entries array; ar_sprite_slot_destroy frees it.
     * count entries of 8 B each.  NULL on a destroyed slot. */
    ar_sprite_entry *entries;
    /* +0x04 (2B): entry count (low half of the dword). */
    uint16_t  entry_count;
    /* +0x06 (2B): pad — never touched at register time. */
    uint16_t  _pad06;
    /* +0x08 (4B): cleared to 0 by the register. */
    uint32_t  f_08;
    /* +0x0c (4B): not touched by the registers we model. */
    uint32_t  f_0c;
    /* +0x10 (4B): not touched. */
    uint32_t  f_10;
    /* +0x14 (4B): not touched. */
    uint32_t  f_14;
    /* +0x18 (4B): cleared to 0. */
    uint32_t  f_18;
    /* +0x1c (4B): "ZDD" pointer (the DDraw wrapper instance).  The
     * boot driver passes this in as `param_1`. */
    void     *zdd;
    /* +0x20 (4B): sprite width in pixels. */
    uint32_t  width;
    /* +0x24 (4B): sprite height in pixels. */
    uint32_t  height;
    /* +0x28 (4B): colorkey (transparent-pixel COLORREF; 0 = opaque). */
    uint32_t  colorkey;
    /* +0x2c (4B): scale flag (1 = enable bilinear-ish path; see
     * winmain-and-bootstrap.md). */
    uint32_t  scale_flag;
    /* +0x30 (4B): sprite type code (2 = font texture per the registers
     * here; sprite atlas types differ in FUN_0056e190's hundreds of
     * blocks). */
    uint32_t  type;
    /* +0x34 (4B): owned aux buffer (some kind of cached decode buffer);
     * ar_sprite_slot_destroy frees it iff non-zero.  Not written by
     * the register — pre-existing state from prior destruction or BSS. */
    void     *aux_buf;
    /* +0x38 (4B): cleared to 0. */
    uint32_t  f_38;
    /* +0x3c (4B): settings pointer (the launcher's settings record;
     * sotes.exe passes it through every batch). */
    void     *settings;
    /* +0x40 (2B): PE resource ID (uint16). */
    uint16_t  resource_id;
    /* +0x42 (2B): asset-group tag stamped by each register. */
    uint16_t  group;
} ar_sprite_slot;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(ar_sprite_slot) == 0x44,        "ar_sprite_slot must be 0x44 bytes");
_Static_assert(offsetof(ar_sprite_slot, entry_count) == 0x04, "sprite entry_count offset");
_Static_assert(offsetof(ar_sprite_slot, zdd)         == 0x1c, "sprite zdd offset");
_Static_assert(offsetof(ar_sprite_slot, width)       == 0x20, "sprite width offset");
_Static_assert(offsetof(ar_sprite_slot, height)      == 0x24, "sprite height offset");
_Static_assert(offsetof(ar_sprite_slot, aux_buf)     == 0x34, "sprite aux_buf offset");
_Static_assert(offsetof(ar_sprite_slot, settings)    == 0x3c, "sprite settings offset");
_Static_assert(offsetof(ar_sprite_slot, resource_id) == 0x40, "sprite resource_id offset");
_Static_assert(offsetof(ar_sprite_slot, group)       == 0x42, "sprite group offset");
#endif

/* ─── globals — mirror retail BSS slot tables ────────────────────── */

/* g_ar_sprite_slots[0] mirrors DAT_008a76e8 (font texture id 0x457).
 * g_ar_sprite_slots[1] mirrors DAT_008a76ec (font texture id 0x455). */
extern ar_sprite_slot g_ar_sprite_slots[2];

/* The retail table is at &DAT_008a9274.  Index 0 lives there; index 1
 * is DAT_008a9278; ...; index 13 (FUN_00582d10 max in this batch) is
 * DAT_008a92a8.  We over-allocate to 32 for headroom — the rest of
 * the asset-register batch (FUN_0057ca40, FUN_0057b280, etc.) will
 * touch more entries. */
#define AR_GDI_SLOT_COUNT 32
extern ar_gdi_slot  g_ar_gdi_slots[AR_GDI_SLOT_COUNT];
extern ar_gdi_slot *g_ar_gdi_table[AR_GDI_SLOT_COUNT];   /* table[i] == &slots[i] post-init */

/* Initialise the global slot tables: zeros all slot bodies and wires
 * g_ar_gdi_table[i] = &g_ar_gdi_slots[i].  Must run before any
 * ar_register_* call. */
void ar_state_init(void);

/* ─── leaf helpers ───────────────────────────────────────────────── */

/* FUN_005bef0e — operator delete[] thunk (just calls free in our port). */
void ar_xfree(void *p);

/* FUN_005b5f50 — linear-interpolate two COLORREFs per channel.
 * Returns `src + (dst - src) * num / denom`, channelwise.
 * The retail body iterates the three low bytes (shifts 0, 8, 16);
 * the alpha byte at shift 24 is untouched (left at 0). */
uint32_t ar_color_lerp(uint32_t src, uint32_t dst, int32_t num, int32_t denom);

/* FUN_00417b50 — sprite slot destructor.
 * Frees `aux_buf` if non-zero, frees each entry's owned `b` pointer,
 * then frees the `entries` array.  Leaves the rest of the slot
 * untouched (the register pass that follows overwrites those
 * fields). */
void ar_sprite_slot_destroy(ar_sprite_slot *s);

/* FUN_00562a10 — destroy a GDI slot.
 * DeleteObject's every non-null handle in `array`, frees `array`,
 * resets count/capacity to 0.  Safe to call on a fresh BSS-zero slot. */
void ar_gdi_slot_destroy(ar_gdi_slot *s);

/* FUN_00579ec0 — destroy + (re)allocate the slot's handle array.
 * After return: array is a zero-filled buffer of `capacity` HGDIOBJ
 * entries, count=0, capacity=capacity.  group is left untouched. */
void ar_gdi_slot_reset(ar_gdi_slot *s, uint16_t capacity);

/* FUN_00579f40 — build one HFONT.
 *
 *   width, height: passed straight to LOGFONTA.lfWidth/lfHeight.
 *   family: 0/1/2 → "Courier New"; 3 → "Times New Roman";
 *           4 → "Arial"; 5 → "Courier New", italic.
 *           Other → leave lfFaceName empty (the retail jumps over
 *           the name copy via switchD_*_default).
 *
 * On first CreateFontIndirectA failure, the retail code overwrites
 * lfFaceName with the global string at DAT_008a9b6c and retries.
 * That global is set by the launcher dialog ("font face override");
 * we read it via ar_gdi_get_fallback_face_name() — the Win32 build
 * returns the runtime pointer, the stub may return NULL to skip the
 * retry.  We preserve the retry behaviour either way.
 *
 * NB: the retail body has a 4-byte heap leak (operator_new(4) whose
 * result is stored only into the leaked block).  We omit it — the
 * ASan-heavy test build catches lingering leaks and the retail leak
 * has no observable side effect. */
ar_gdi_handle ar_make_font(int32_t width, int32_t height, uint32_t family);

/* FUN_0057a030 — destroy slot, install one fresh HFONT at array[0].
 * Stamps `group` at +0x08.  capacity ends at 1; count stays at 0
 * (the retail body never bumps it — see ar_gdi_slot.count comment). */
void ar_gdi_slot_set_font(ar_gdi_slot *s, uint16_t group,
                          int32_t width, int32_t height, uint32_t family);

/* FUN_0057a1a0 — destroy slot, allocate `capacity`, install one HPEN
 * at array[0], bump count to 1.  Stamps `group` at +0x08.
 * No-op (no HPEN installed) if capacity == 0. */
void ar_gdi_slot_set_pen(ar_gdi_slot *s, int32_t width, uint32_t color,
                         uint16_t group, uint16_t capacity);

/* FUN_0057a260 — destroy slot, allocate `capacity`, install one HBRUSH
 * at array[0], bump count to 1.  Stamps `group` at +0x08.
 * No-op (no HBRUSH installed) if capacity == 0. */
void ar_gdi_slot_set_brush(ar_gdi_slot *s, uint32_t color,
                           uint16_t group, uint16_t capacity);

/* FUN_00582d10 — install a `capacity`-pen gradient ramp into
 * g_ar_gdi_table[index].
 *
 * Pen [0]:               (width_a, color_a)
 * Pens [1..capacity-2]:  (width_a - width_step*i, lerp(color_a, color_mid, i, capacity))
 * Pen  [capacity-1]:     (width_c, color_c)
 *
 * Stamps `group` at +0x08, capacity at +0x06, count at +0x04 (bumped
 * per pen written).  Calls ar_gdi_slot_destroy before re-allocating.
 *
 * Notes vs retail:
 *   - The middle-loop bound is `1 < i < capacity-1` — the loop body
 *     iterates exactly `capacity - 2` pens IFF `capacity >= 3`.
 *     For capacity < 3 only [0] and [capacity-1] get written (if
 *     capacity >= 1 / capacity >= 2 respectively).
 *   - The midpoints' COLORREF uses ar_color_lerp(color_a, color_mid,
 *     i, capacity).  Note: `color_mid` is the SECOND colour param;
 *     the THIRD colour is for the last pen.  The naming in retail's
 *     decompile (param_4 / param_5 / param_6) is opaque; pen 0
 *     uses param_4, midpoints lerp param_4→param_5, the last pen
 *     uses param_6.
 *   - The retail body re-loads `g_ar_gdi_table[index]` on every
 *     iteration (presumably the original C had it as
 *     `g_ar_gdi_table[index]->...` repeatedly).  We don't need that. */
void ar_gdi_slot_set_pen_gradient(uint32_t index, uint16_t group, uint16_t capacity,
                                   uint32_t color_a, uint32_t color_mid,
                                   uint32_t color_c, uint32_t width_a,
                                   uint32_t width_step, uint32_t width_c);

/* ─── top-level driver call ──────────────────────────────────────── */

/* FUN_00579bd0 — boot-driver asset-register batch (fonts).
 *
 * Effects (slot-by-slot):
 *
 *   sprite[0] (DAT_008a76e8): id 0x457, 32×32, type=2, scale=0
 *   sprite[1] (DAT_008a76ec): id 0x455, 32×48, type=2, scale=1
 *
 *   gdi_table[1] (DAT_008a9278): font  6×14 Courier New
 *   gdi_table[2] (DAT_008a927c): font  7×16 Courier New
 *   gdi_table[3] (DAT_008a9280): font  7×18 Courier New
 *   gdi_table[4] (DAT_008a9284): font  4× 8 Courier New
 *   gdi_table[5] (DAT_008a9288): font  7×18 Courier New (family 2, same face)
 *   gdi_table[6] (DAT_008a928c): font 10×20 Courier New
 *   gdi_table[7] (DAT_008a9290): font  5×10 Courier New
 *   gdi_table[8] (DAT_008a9294): font  8×16 Courier New
 *
 *   gdi_table[9]  (DAT_008a9298): font  4× 8 Courier New (via FUN_0057a030)
 *   gdi_table[14] (DAT_008a92ac): pen   width=1 color=0xffffff (via FUN_0057a1a0)
 *   gdi_table[15] (DAT_008a92b0): brush color=0xffffff       (via FUN_0057a260)
 *
 *   gdi_table[12] (DAT_008a92a4): 4-pen gradient (0x200020 → 0x605060 → 0xb090bf), widths 15→4-step→1
 *   gdi_table[10] (DAT_008a929c): 4-pen gradient (0x200000 → 0x804030 → 0xcfa090), widths 15→4-step→1
 *   gdi_table[11] (DAT_008a92a0): 4-pen gradient (0x000030 → 0x405080 → 0x8090bf), widths 15→4-step→1
 *   gdi_table[13] (DAT_008a92a8): 4-pen gradient (0x300020 → 0x804060 → 0xb090a0), widths 15→4-step→1
 *
 * The retail decompile's `FUN_0057a030(4,8,0,param_2)` / a1a0 / a260
 * callsites are thiscall — Ghidra dropped the ECX setup from the C
 * view.  Disassembly confirms: ECX is loaded from [0x8a9298], [0x8a92ac],
 * [0x8a92b0] respectively (table indices 9, 14, 15). */
void ar_register_fonts(void *zdd, uint16_t group, void *settings);

/* ─── GDI primitive wrappers — defined per build target ─────────── */

/* family: see ar_make_font.  italic: 0/1.  face: face name string. */
ar_gdi_handle ar_gdi_create_font(int32_t width, int32_t height,
                                  int italic, const char *face);
/* style 0 = PS_SOLID.  Other styles unused by the boot batch. */
ar_gdi_handle ar_gdi_create_pen(int style, int32_t width, uint32_t color);
ar_gdi_handle ar_gdi_create_brush(uint32_t color);
void ar_gdi_delete(ar_gdi_handle h);
/* Returns the global "font face override" string (DAT_008a9b6c).  Used
 * by ar_make_font as a fallback when the first CreateFontIndirectA
 * fails.  May return NULL or "" — the retail copies it unconditionally,
 * we match that. */
const char *ar_gdi_get_fallback_face_name(void);

#endif /* OPENSUMMONERS_ASSET_REGISTER_H */
