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
 *   - FUN_005748c0   ar_sprite_slot_register  (single-entry sprite register)
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

/* ─── ar_sound_slot — Type C, sound-bank descriptor (0x18 B) ─────── */

/* Used by the SOUND family (FUN_00579a00, FUN_00563ef0 et al).  Each
 * slot is a pointer-target in the table at DAT_008a6ec4 (the "W_MGR
 * boot-pool" — see HANDOFF "Open RE threads").  Allocation of the
 * slot bodies themselves is some other init function's job; this
 * module reads `g_ar_sound_table[i]` as already-pointing-at storage.
 *
 * Layout reverse-engineered from FUN_00579a00 (inline write pattern)
 * and FUN_00563ef0 (thiscall slot setter + lazy wave-loader):
 *
 *   +0x00 (u16): "count" — number of buffer entries (kind 2 or 4 in
 *                the boot batch).  Doubles as a channel-count-ish
 *                value when FUN_00563ef0 actually loads waves.
 *   +0x02 (u16): zero — possibly a state bit; always cleared.
 *   +0x04 (u32): "buffer ptr" — set by the wave-load path to a
 *                count*8 B array of DSound buffer descriptors.
 *                NULL means "not yet loaded".  Boot-time inits leave
 *                this at 0.
 *   +0x08 (u32): ZDS (DirectSound manager) pointer.
 *   +0x0c (u16): PE resource ID for the wave bytes (sotesd.dll).
 *   +0x0e (u16): pad — never written.
 *   +0x10 (u32): settings pointer (launcher settings record).
 *   +0x14 (u16): asset-group tag.
 *   +0x16 (u16): pad — never written. */
typedef struct ar_sound_slot {
    uint16_t  count;          /* +0x00 */
    uint16_t  state;          /* +0x02 */
    void     *buffer;         /* +0x04 — lazily allocated by wave loader */
    void     *zds;            /* +0x08 */
    uint16_t  resource_id;    /* +0x0c */
    uint16_t  _pad0e;
    void     *settings;       /* +0x10 */
    uint16_t  group;          /* +0x14 */
    uint16_t  _pad16;
} ar_sound_slot;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(ar_sound_slot)          == 0x18, "ar_sound_slot must be 24 bytes");
_Static_assert(offsetof(ar_sound_slot, state)       == 0x02, "sound state offset");
_Static_assert(offsetof(ar_sound_slot, buffer)      == 0x04, "sound buffer offset");
_Static_assert(offsetof(ar_sound_slot, zds)         == 0x08, "sound zds offset");
_Static_assert(offsetof(ar_sound_slot, resource_id) == 0x0c, "sound resource_id offset");
_Static_assert(offsetof(ar_sound_slot, settings)    == 0x10, "sound settings offset");
_Static_assert(offsetof(ar_sound_slot, group)       == 0x14, "sound group offset");
#endif

/* ─── globals — mirror retail BSS slot tables ────────────────────── */

/* Sprite slot pool — models the retail BSS region
 * 0x008a7640..0x008a771c.  Logical index = (retail_addr - 0x008a7640) / 4.
 * Multiple register batches (FUN_00579bd0, FUN_005749b0, FUN_0057a330,
 * FUN_0056e190, ...) all populate slots in this same pool — keying by
 * index keeps the field-write behaviour observable across batches
 * without modelling each retail BSS sub-range separately.
 *
 * Capacity 64 covers all currently-known register batches with
 * headroom (FUN_0056e190 will need more — bump when that lands).
 *
 * Named index constants for the slots whose semantic role is known
 * are defined below; everything else is referenced by raw index in
 * the batch driver and its tests. */
#define AR_SPRITE_SLOT_COUNT 64
extern ar_sprite_slot  g_ar_sprite_slots[AR_SPRITE_SLOT_COUNT];
extern ar_sprite_slot *g_ar_sprite_table[AR_SPRITE_SLOT_COUNT];

/* FUN_00579bd0 font-texture sprites.  Retail addresses 0x008a76e8 and
 * 0x008a76ec → indices 42 and 43 in the pool. */
#define AR_SPR_FONT_TEX_457  42  /* 32×32 type=2 (DAT_008a76e8) */
#define AR_SPR_FONT_TEX_455  43  /* 32×48 type=2 (DAT_008a76ec) */

/* The retail table is at &DAT_008a9274.  Index 0 lives there; index 1
 * is DAT_008a9278; ...; index 13 (FUN_00582d10 max in this batch) is
 * DAT_008a92a8.  We over-allocate to 32 for headroom — the rest of
 * the asset-register batch (FUN_0057ca40, FUN_0057b280, etc.) will
 * touch more entries. */
#define AR_GDI_SLOT_COUNT 32
extern ar_gdi_slot  g_ar_gdi_slots[AR_GDI_SLOT_COUNT];
extern ar_gdi_slot *g_ar_gdi_table[AR_GDI_SLOT_COUNT];   /* table[i] == &slots[i] post-init */

/* Sound slots — DAT_008a6ec4..6ef0, 12 entries (4-byte stride =
 * 12 pointers in BSS, each pointing at an ar_sound_slot).  The "W_MGR"
 * pool referenced in HANDOFF.  FUN_00579a00 touches all 12. */
#define AR_SOUND_SLOT_COUNT 12
extern ar_sound_slot  g_ar_sound_slots[AR_SOUND_SLOT_COUNT];
extern ar_sound_slot *g_ar_sound_table[AR_SOUND_SLOT_COUNT];

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

/* FUN_005748c0 — register one sprite slot (single 8-byte entry).
 *
 * Calls ar_sprite_slot_destroy first, then allocates a fresh 1-entry
 * `entries` array, zero-fills it, and stamps the named fields.
 * entry_count is always 1 in retail callers — every known dispatcher
 * (FUN_005749b0 inline blocks, FUN_0057a330 batch, FUN_0056e190
 * hundreds-of-sprites mega-register, and the FUN_00579bd0 sprites in
 * this module) passes the same single-entry shape.
 *
 * Fields touched (writes match retail order; observable end state):
 *   entries     ← new calloc(1, 8)
 *   entry_count ← 1
 *   zdd         ← zdd
 *   settings    ← settings
 *   resource_id ← resource_id (low 16 of param_3)
 *   width, height, colorkey, scale_flag, type ← as passed
 *   group       ← group (low 16 of param_9)
 *   f_08 = f_18 = f_38 ← 0
 *
 * Fields left untouched: f_0c, f_10, f_14 (BSS or prior state); aux_buf
 * is freed in the prologue (via ar_sprite_slot_destroy).
 *
 * Returns nothing (retail returns 1; the value is dead in every caller). */
void ar_sprite_slot_register(ar_sprite_slot *s, void *zdd, void *settings,
                              uint16_t resource_id,
                              uint32_t width, uint32_t height,
                              uint32_t colorkey, uint32_t scale_flag,
                              uint32_t type, uint16_t group);

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

/* ─── sound-bank slot setter (used by ar_register_sounds) ────────── */

/* FUN_00563ef0 first half — the pure field-write half (no wave load).
 *
 * Equivalent to FUN_00563ef0 called with `load_flag = 0` (the form
 * the boot-time register passes).  Writes the 6 named fields and
 * clears `state`.  Leaves `buffer` untouched — the lazy wave loader
 * (the FUN_00563ef0 `if (param_6 != 0 && ...)` branch — not ported)
 * is the only thing that ever populates it.
 *
 * Provenance: matches the inline 11-slot pattern in FUN_00579a00 and
 * the field-init body of FUN_00563ef0. */
void ar_sound_slot_init(ar_sound_slot *s, void *zds, void *settings,
                        uint16_t resource_id, uint16_t count, uint16_t group);

/* FUN_00579a00 — register 12 sound-bank slots at boot.
 *
 * Each entry has the form (id, kind):
 *   table[ 0]: id=0x50f kind=2
 *   table[ 1]: id=0x50e kind=2
 *   table[ 2]: id=0x508 kind=2
 *   table[ 3]: id=0x510 kind=2
 *   table[ 4]: id=0x903 kind=2
 *   table[ 5]: id=0x509 kind=4
 *   table[ 6]: id=0x506 kind=4
 *   table[ 7]: id=0x507 kind=2
 *   table[ 8]: id=0x50c kind=4   ← only entry written via FUN_00563ef0
 *   table[ 9]: id=0x50d kind=4
 *   table[10]: id=0x4d8 kind=2
 *   table[11]: id=0x4d9 kind=2
 *
 * Retail writes entries 0..7, 9..11 inline (sharing one register-
 * passed group/settings/zds across them all), then dispatches entry
 * 8 through FUN_00563ef0(zds, settings, 0x50c, 4, group, 0).  We
 * route all 12 through the same `ar_sound_slot_init` helper since the
 * field-writes are identical and `load_flag = 0` means the lazy-load
 * branch is dead code at boot.  Verified equivalent via disasm at
 * 0x579b89-0x579bb9.
 *
 * Note the kind=2 / kind=4 alternation: 0x506 / 0x509 / 0x50c / 0x50d
 * are the kind-4 slots.  No obvious pattern in the resource IDs;
 * likely the 4-channel sound effects (footsteps?  combat hits?  unknown
 * without deeper RE). */
void ar_register_sounds(void *zds, uint16_t group, void *settings);

/* ─── top-level driver calls ─────────────────────────────────────── */

/* FUN_005749b0 — UI/menu sprite-register batch.
 *
 * Called by the boot driver (FUN_00562ea0) right after the third
 * `FUN_00563ef0` sound-bank load and before `FUN_0056e190`.  Retail
 * caller: `FUN_005749b0(ZDD, 4, settings)` — `group` = 4.
 *
 * Populates 34 sprite slots in `g_ar_sprite_slots[]`:
 *
 *   - idx 0   (DAT_008a7640): id 0x90b 32×32 — special, loaded from
 *                              sotesp.dll (passed as `sotesp_module`
 *                              instead of `settings`).  Retail loads
 *                              this via `FUN_005748c0` with `settings
 *                              = DAT_008a6e74` (the sotesp HMODULE),
 *                              presumably so the small SFX-pack
 *                              companion DLL is queried for this
 *                              one font texture.  Same slot is also
 *                              the target of the palette ramp (see
 *                              "Palette ramp section" below).
 *   - idx 1..9  (DAT_008a7644..0x7664): 9 inline registers — IDs
 *                              0x49f, 0x448, 0x4a2, 0x49d, 0x913,
 *                              0x91b, 0x91c, 0x91d, 0x8df.  Sizes /
 *                              types vary; see the driver body.
 *                              Note retail writes them in shuffled
 *                              order (1,2,3,4,6,7,8,5,9) — output is
 *                              order-independent so we batch them.
 *   - idx 10..29 (DAT_008a7668..0x76b4): 20 trailing FUN_005748c0
 *                              calls.  Mostly the 368×276 panel set
 *                              (0x8df-family follow-ups) plus 640×480
 *                              full-screen backgrounds.
 *   - idx 46, 47, 50, 55 (DAT_008a76f8, _76fc, _7708, _771c): 4
 *                              straggler small-icon registers.  The
 *                              gap between 30..45 and 44..49 is where
 *                              FUN_00579bd0's font textures live
 *                              (indices 42/43) and other batches will
 *                              fill in.
 *
 * Palette ramp section (NOT PORTED): retail constructs a 256-entry
 * palette between the slot-5 inline write and the slot-9 inline
 * write — calls FUN_004178e0 (palette session begin), then
 * FUN_005b5d90 ten times to write background entries, then
 * FUN_005b5f50 + FUN_005b5d90 twenty times to lerp from 0x383838
 * to 0xffffff, then FUN_00491770 (install palette onto the
 * sprite slot at idx 0).  Depends on the palette-session trio
 * (FUN_004178e0 / _005b5d90 / _00491770) — deferred.  The slot's
 * resource-id / dimension fields ARE written by the line-230
 * FUN_005748c0 call (already ported below); only the palette
 * upload step is skipped.  When the PE-resource decoder lands,
 * port the palette ramp and call it from this driver between the
 * inline-slot writes and the trailing-call batch. */
void ar_register_main_sprites(void *zdd, uint16_t group, void *settings,
                              void *sotesp_module);

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
