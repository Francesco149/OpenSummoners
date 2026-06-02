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

#include <stdbool.h>
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
    /* +0x00: the frame-surface array — a pointer to an array of per-frame
     * zdd_object* surfaces, populated lazily by the sprite-sheet decoder
     * (0x4184a0) and read by the frame getter (FUN_00418470) as
     * `entries[0].frames[frame_id]`.  NULL ⇒ the bank's sheet is not yet
     * decoded (the getter's "needs load" flag).  Never freed by the slot
     * destructor — the surfaces are owned by the ZDD wrapper.  (Was the
     * opaque `uint32_t a`; widened to a real pointer so the getter is
     * host-testable — on the 32-bit build it is still 4 bytes, preserving
     * the 8-byte record size.) */
    void    *frames;   /* +0x00 */
    void    *b;        /* +0x04 — owned aux buffer; ar_sprite_slot_destroy frees */
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
    /* +0x08 (4B): decode-transform gate.  Cleared to 0 by the register;
     * set non-zero by the per-sprite registrars that want a brightness
     * pass.  The decoder (FUN_004184a0) only runs the 24bpp
     * colorkey/brightness transform when this is non-zero. */
    uint32_t  f_08;
    /* +0x0c (4B): brightness scale for pixel byte 2 (R in the BGR DIB),
     * applied as `byte = byte * f_0c / 1000` by the decoder
     * (FUN_004184a0, 0x418690).  Cleared to 0 by the register. */
    uint32_t  f_0c;
    /* +0x10 (4B): brightness scale for pixel byte 1 (G), `byte * f_10 /
     * 1000` (FUN_004184a0, 0x418674). */
    uint32_t  f_10;
    /* +0x14 (4B): brightness scale for pixel byte 0 (B), `byte * f_14 /
     * 1000` (FUN_004184a0, 0x418659). */
    uint32_t  f_14;
    /* +0x18 (4B): optional gamma/remap LUT base.  When non-zero the
     * decoder maps each channel through `byte = ((uint8_t*)f_18)[byte]`
     * before the brightness scale (FUN_004184a0, 0x418633).  Stored as a
     * raw 32-bit pointer in retail; cleared to 0 by the register. */
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

/* ─── ar_info_entry — SS_MGR-pool entry (20 B) ───────────────────── */

/* Reverse-engineered from FUN_00582d00 (the 14-byte clear routine),
 * the surrounding copy chain in FUN_0057ca40 (around 0x57fa90), and
 * the pool allocator at FUN_00562ea0:225-253 which pins the 20-byte
 * size and the +0x10 field that the clear doesn't touch.
 *
 * Allocator (FUN_00562ea0:225-253) runs a single 909-iteration loop
 * that initialises TWO parallel pools side-by-side:
 *   - 0x8a760c..0x8a8440 — 909 × 0x44-byte sprite slot bodies
 *     (`g_ar_sprite_ramp_slots` + `g_ar_sprite_slots` view the same pool).
 *   - 0x8a8440..0x8a9270 — 909 × 0x14-byte ar_info_entry bodies.
 * Each pool slot in BSS holds a heap pointer.  The "parallel-info-
 * table" the rabbit-hole originally described as ~357 entries at
 * 0x8a8578..0x8a8b14 is just a subset of this 909-entry pool.
 *
 * Disasm sequence at one call site (0x57fa93..0x57facb):
 *   call FUN_00582b80          ; clone sprite slot (this=src, arg=dst)
 *   mov  ecx, [0x8a8a40]       ; load entry-ptr from the pool
 *   call FUN_00582d00          ; clear *entry  (word@+0, dwords@+4/+8/+12)
 *   ; then: copy *[0x8a8a34] (template-entry) → *entry at offsets 0 and 4
 *   ; then: *(entry + 8) = &DAT_006752f8   (const PE rdata pointer)
 *
 * Layout:
 *   +0x00 (u16):  live/active marker — values 1 or 2 in the prefix
 *                 table breakdown (see 0057ca40-rabbit-hole.md table).
 *                 Cleared to 0 by ar_info_entry_clear.
 *   +0x02 (u16):  pad — never touched by the clear or the template-
 *                 copy chain.  Untouched in every observed retail write.
 *   +0x04 (u32):  flag/state — 0/1/2/3 in observed writes/reads
 *                 (FUN_00586010:727-822 dispatches on the value).
 *   +0x08 (void*):const PE rdata pointer (e.g. &DAT_006752f8,
 *                 &DAT_006748d0, &DAT_00674ad8 — the 98 const-data-
 *                 pointer writes).  Re-written at runtime by
 *                 FUN_00587e00 for many entries.
 *   +0x0c (void*):palette buffer pointer (256-entry, 1024 B).  Read
 *                 by FUN_00586010:755 as `*(int ***)(entry + 0xc)` —
 *                 if non-NULL it overrides the ar_palette_session_begin
 *                 result; entries iterate it in parallel with the
 *                 local palette for ar_color_lerp-style modifiers.
 *                 Cleared to NULL by ar_info_entry_clear.
 *   +0x10 (u32):  zeroed by the allocator; semantics unknown.  NOT
 *                 touched by ar_info_entry_clear (14 B clear stops at
 *                 +0x0e), so bytes 14..19 retain alloc-time zero plus
 *                 whatever runtime writes happen elsewhere. */
typedef struct ar_info_entry {
    uint16_t    marker;     /* +0x00 */
    uint16_t    _pad02;     /* +0x02 — untouched by the routines we model */
    uint32_t    flag;       /* +0x04 */
    const void *data;       /* +0x08 — const PE rdata pointer */
    void       *palette;    /* +0x0c — 1024 B palette buffer (256 RGBA) */
    uint32_t    f_10;       /* +0x10 — alloc-zeroed; runtime semantics unknown */
} ar_info_entry;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(ar_info_entry)             == 0x14, "ar_info_entry must be 20 bytes");
_Static_assert(offsetof(ar_info_entry, marker)   == 0x00, "info_entry marker offset");
_Static_assert(offsetof(ar_info_entry, flag)     == 0x04, "info_entry flag offset");
_Static_assert(offsetof(ar_info_entry, data)     == 0x08, "info_entry data offset");
_Static_assert(offsetof(ar_info_entry, palette)  == 0x0c, "info_entry palette offset");
_Static_assert(offsetof(ar_info_entry, f_10)     == 0x10, "info_entry f_10 offset");
#endif

/* ─── globals — mirror retail BSS slot tables ────────────────────── */

/* Sprite slot pool — models the retail BSS region starting at
 * 0x008a7640.  Logical index = (retail_addr - 0x008a7640) / 4.
 * Multiple register batches (FUN_00579bd0, FUN_005749b0, FUN_0057a330,
 * FUN_0056e190, ...) all populate slots in this same pool — keying by
 * index keeps the field-write behaviour observable across batches
 * without modelling each retail BSS sub-range separately.
 *
 * Capacity 1024 covers FUN_0056e190's hundreds-of-sprites batch
 * (touches idx 62..863) with headroom for the remaining un-ported
 * batches (FUN_0057a330, FUN_0057ca40, FUN_0057b280 — likely a few
 * dozen more slots).  Retail's BSS region is contiguous past 0x8a7640
 * for at least 0x1000 bytes; bump again if a later batch exceeds 1024.
 *
 * Named index constants for the slots whose semantic role is known
 * are defined below; everything else is referenced by raw index in
 * the batch driver and its tests. */
#define AR_SPRITE_SLOT_COUNT 1024
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

/* Palette-ramp slot pool — models the 12-entry retail BSS region at
 * 0x008a7610..0x008a763c (preceding the main sprite pool's base
 * 0x008a7640).  These 12 slots are exclusively touched by
 * `ar_register_palette_ramps` (FUN_0057a330) — one per ramp — and
 * never cross-indexed with the main pool, so they live in their own
 * 12-entry array.
 *
 * Index → retail address map (slots laid out in retail BSS order):
 *
 *   ramp[ 0] → 0x008a7610   ramp[ 4] → 0x008a7620   ramp[ 8] → 0x008a7630
 *   ramp[ 1] → 0x008a7614   ramp[ 5] → 0x008a7624   ramp[ 9] → 0x008a7634
 *   ramp[ 2] → 0x008a7618   ramp[ 6] → 0x008a7628   ramp[10] → 0x008a7638
 *   ramp[ 3] → 0x008a761c   ramp[ 7] → 0x008a762c   ramp[11] → 0x008a763c
 *
 * Index = (retail_addr - 0x008a7610) / 4. */
#define AR_SPRITE_RAMP_COUNT 12
extern ar_sprite_slot  g_ar_sprite_ramp_slots[AR_SPRITE_RAMP_COUNT];
extern ar_sprite_slot *g_ar_sprite_ramp_table[AR_SPRITE_RAMP_COUNT];

/* ar_info_entry pool — models the 909-entry retail BSS pointer table
 * at 0x008a8440..0x008a9270.  The pool is allocated by FUN_00562ea0:225-253
 * in lockstep with the sprite-slot pool (see g_ar_sprite_slots above):
 * a single 909-iteration loop heap-allocs one 68 B sprite slot AND one
 * 20 B info_entry per index, storing both pointers in adjacent BSS
 * regions.  Our host model mirrors retail's pointer-indirected access:
 * `g_ar_info_table[i]->field` is the equivalent of retail's
 * `(*(struct **)(0x008a8440 + i*4))->field` and surfaces every write
 * the disassembly performs through the +4 / +8 / +0xc offsets.
 *
 * Index → retail BSS pointer-slot map (each slot is 4 bytes; the
 * pointed-to entry is 20 bytes):
 *
 *   pool[  0] → 0x008a8440   pool[  1] → 0x008a8444   ...
 *   pool[ 78] → 0x008a8578   (first "g_ar_sprite_flags" entry — the
 *                             portrait-flag pairs the palette-ramp
 *                             register batch writes)
 *   pool[ 91] → 0x008a85ac   (last "g_ar_sprite_flags" entry)
 *   pool[ 92] → 0x008a85b0   (first FUN_0057ca40 inline-template write)
 *   pool[437] → 0x008a8b14   (last FUN_0057ca40 write — the "+4 = 2"
 *                             tail entry; rabbit-hole table row 7)
 *   pool[908] → 0x008a926c   (pool end; pool[909] would be 0x008a9270
 *                             which is the start of the W_MGR pool —
 *                             see g_ar_gdi_slots below)
 *
 * Most pool entries past 437 are not touched at boot by any function
 * we've decompiled.  Capacity matches retail exactly. */
#define AR_INFO_ENTRY_COUNT 909
extern ar_info_entry  g_ar_info_entries[AR_INFO_ENTRY_COUNT];
extern ar_info_entry *g_ar_info_table[AR_INFO_ENTRY_COUNT];

/* Portrait-flag region — 14 entries written by ar_register_palette_ramps
 * with values 0 or 3 (paired one-to-one with the 14 trailing
 * FUN_005748c0 register calls).  Retail pool indices 78..91. */
#define AR_INFO_RAMP_FLAGS_BASE  78
#define AR_INFO_RAMP_FLAGS_COUNT 14

/* Sound slots — start at DAT_008a6ec4.  The first 12 entries
 * (DAT_008a6ec4..6ef0) are the "main sounds" populated by FUN_00579a00.
 * FUN_0057b280 (the game-sounds batch, group 3) extends the pool out
 * to index 244 (retail address 0x008a7294), and its conditional locale
 * tail (`ar_register_locale_sounds`) reaches as far as idx 464 — the
 * retail pool capacity, allocated as a 0x1d1-entry pointer table at
 * `&DAT_008a6ec4` by FUN_00562ea0's SS_MGR_Preparation block.  Round
 * capacity up to 512 to cover the locale loop's max idx with headroom. */
#define AR_SOUND_MAIN_COUNT  12
#define AR_SOUND_POOL_COUNT  465   /* retail W_MGR pool allocation (0x1d1) */
#define AR_SOUND_SLOT_COUNT  512
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

/* FUN_00582b80 — clone metadata from one ar_sprite_slot into another.
 *
 * Thiscall on `src` (retail puts the source in ECX); `dst` is the
 * stack arg.  Like `ar_sprite_slot_register` but pulls every field
 * from an existing slot rather than taking them as primitives:
 *
 *   1. Frees dst's old `aux_buf` and walks dst's old `entries[]`
 *      freeing each entry's owned `b` pointer, then frees the
 *      entries array.  Same prologue shape as ar_sprite_slot_destroy.
 *   2. Allocates a fresh single-entry `entries` array on dst,
 *      zeroed.  dst->entry_count = 1.
 *   3. Copies metadata from src to dst: zdd, settings, resource_id,
 *      width, height, colorkey, scale_flag, type, group.  Clears
 *      f_08, f_18, f_38 to 0 on dst.
 *   4. If src->aux_buf is non-NULL, allocates dst->aux_buf =
 *      malloc(src->f_38 * 24) and memcpy's that many bytes from
 *      src->aux_buf.  Retail quirk: dst->f_38 is left at 0 (cleared
 *      in step 3 and never re-stamped) — the count is NOT
 *      propagated to dst.  We match retail.
 *
 * Source's `entries` array is NOT transferred.  src retains its own
 * pointer; dst gets a brand-new 1-entry alloc.  Callers (FUN_0057ca40)
 * sometimes follow up by zeroing src->entries/entry_count/f_08/f_0c
 * via `ar_info_entry_clear`-style ops on the template slot — see
 * `docs/findings/0057ca40-rabbit-hole.md` "clone-and-detach pair".
 *
 * Module-isolation: no real caller is ported yet.  Available for the
 * FUN_0057ca40 wiring once SS_MGR and the parallel-info-table land. */
void ar_sprite_slot_clone(ar_sprite_slot *dst, const ar_sprite_slot *src);

/* FUN_00582d00 — zero a parallel-info-table entry.
 *
 * Thiscall on `entry` (retail puts the entry pointer in ECX after
 * loading it from the table at e.g. `[0x8a8a40]`).  Writes:
 *   word @+0  = 0   (low half of `marker`)
 *   dword@+4  = 0   (flag)
 *   dword@+8  = 0   (data)
 *   dword@+12 = 0   (f_0c)
 * Leaves the pad bytes at +2..+3 untouched (the retail body writes
 * `ax` to +0, not `eax`).
 *
 * Used in the FUN_0057ca40 cluster immediately after
 * ar_sprite_slot_clone, before the copy-from-template-entry chain
 * populates the cleared entry.  Module-isolation: no consumer ported. */
void ar_info_entry_clear(ar_info_entry *entry);

/* Unified sprite-slot pool accessor — maps a retail pool index (0..908,
 * matching `0x008a760c + i*4`) to the host slot pointer.
 *
 *   pool[0]      → NULL (the allocator-zeroed sentinel at 0x008a760c;
 *                  no decompiled consumer touches it; see rabbit-hole §5).
 *   pool[1..12]  → &g_ar_sprite_ramp_slots[pool_idx - 1].
 *   pool[13..908]→ &g_ar_sprite_slots[pool_idx - 13].
 *
 * Mirrors the SS_MGR singleton's `this->sprite_table[i]` (at +0x0aac,
 * see docs/findings/0057ca40-rabbit-hole.md §7).  Use whenever a
 * ported function indexes the unified pool (e.g. FUN_004179b0); the
 * standalone batches keep using their own range-specific arrays. */
ar_sprite_slot *ar_pool_get_slot(uint16_t pool_idx);

/* ─── sprite-bank frame getter (FUN_00418470) ────────────────────────
 *
 * Resolves frame `frame_id` of a sprite bank (`ar_sprite_slot`) to its
 * surface, lazy-decoding the bank's sheet on first use.  Retail's
 * two-level indirection: bank+0 is the `entries` array; entries[0].frames
 * is the per-frame surface array.  A NULL frames pointer means "sheet not
 * yet decoded" — retail then calls the decoder 0x4184a0 unconditionally
 * and dereferences the result.
 *
 *   if (slot->entries[0].frames == NULL) ar_sprite_decode_hook(slot);
 *   return ((void**)slot->entries[0].frames)[frame_id & 0xffff];
 *
 * The decoder (0x4184a0 — PE-resource read + 24bpp decode + per-channel
 * brightness LUT + surface slice via 0x4188b0) is a separate, larger
 * chip; it is routed through the nullable hook below so the getter ports
 * and tests now.  Returns the surface as a void* (a zdd_object* the render
 * side casts).  Returns NULL when the bank is still unloaded after the
 * (possibly absent) hook — headless safety; retail would dereference NULL.
 * Pinned from the disasm at 0x418470. */
typedef void (*ar_sprite_decode_fn)(ar_sprite_slot *slot);
extern ar_sprite_decode_fn ar_sprite_decode_hook;

void *ar_sprite_slot_frame(ar_sprite_slot *slot, uint16_t frame_id);

/* ─── sprite-sheet decoder + slicer (FUN_004184a0 / FUN_004188b0) ─────
 *
 * The genuine pixel source: turn a sprite bank's PE "DATA" resource into
 * the per-frame surface array `entries[0].frames` that ar_sprite_slot_frame
 * returns.  This is the `ar_sprite_decode_hook` target — install
 * `ar_sprite_decode` into the hook to make the frame getter self-load.
 *
 * The chain (faithful to retail):
 *   ar_sprite_decode (0x4184a0)
 *     ├─ free any previously-decoded frames (re-decode path)
 *     ├─ bs_decode_resource(settings, resource_id, "DATA", compressed=1)
 *     │     [already ported in bitmap_session.c]
 *     ├─ if (f_08 && bpp==24) ar_sheet_decode_pixels(...)   ← brightness pass
 *     └─ ar_sprite_slice(...)                                ← cut into frames
 *
 * The leaf that turns one cell into a real DDraw surface (0x5b9280) and
 * the optional per-cell trim-metadata builder (0x5b6f80) are the
 * DDraw layer — routed through the nullable hooks below so the decode +
 * slice logic ports and host-tests now (headless: frames array is sized
 * and zero-filled, but the surfaces stay NULL, exactly as the frame
 * getter's "still unloaded" path already tolerates).  The format-setup
 * switch on the god-object display depth ([zdd+0x168] →
 * 0x5b7310/_74f0/_7270) and the 8bpp indexed-palette apply
 * (0x5b7bd0) are likewise deferred — see docs/findings/sprite-pipeline.md. */

/* Forward decl so this Win32-free header needn't pull in bitmap_session.h;
 * the full definition lives there and is included by asset_register.c. */
struct bitmap_session;

/* The 24bpp colorkey/brightness transform (FUN_004184a0 inner loop,
 * 0x4185b1..0x4186be), applied in place to a freshly-decoded sheet.
 *
 * For each pixel whose low 24 bits are NOT the magenta key 0xff00ff
 * (B=0xff, G=0x00, R=0xff):
 *   if (lut) ch = lut[ch]        for each of the 3 bytes   (optional gamma)
 *   byte0 (B) = (uint8_t)((int)byte0 * m_b / 1000)         (slot->f_14)
 *   byte1 (G) = (uint8_t)((int)byte1 * m_g / 1000)         (slot->f_10)
 *   byte2 (R) = (uint8_t)((int)byte2 * m_r / 1000)         (slot->f_0c)
 * Division truncates toward zero (retail signed idiv).  Magenta pixels are
 * left untouched (they are the transparent key).  No-op if the sheet is
 * not 24bpp.  `lut` may be NULL.  Reads 3 bytes per pixel (not a dword) so
 * the last pixel never reads past the buffer. */
void ar_sheet_decode_pixels(struct bitmap_session *sheet,
                            uint32_t m_b, uint32_t m_g, uint32_t m_r,
                            const uint8_t *lut);

/* Per-cell surface builder (0x5b9280) — the DDraw leaf.  Returns the
 * created frame surface (a zdd_object* the render side casts) or NULL.
 * NULL hook ⇒ headless: ar_sprite_slice leaves that frame NULL. */
typedef void *(*ar_frame_build_fn)(ar_sprite_slot *slot,
                                   const struct bitmap_session *sheet,
                                   int src_x, int src_y, int cell_w, int cell_h,
                                   uint32_t colorkey, void *aux_entry);
extern ar_frame_build_fn ar_frame_build_hook;

/* Per-frame surface release (FUN_005b9390 + the FUN_005bef0e delete that
 * follows it) — frees one surface created by ar_frame_build_hook.  Used by
 * the re-decode cleanup.  NULL hook ⇒ the surface pointer is dropped
 * without a free (headless: the build hook returned a non-owned token). */
typedef void (*ar_frame_free_fn)(void *surface);
extern ar_frame_free_fn ar_frame_free_hook;

/* Slice a decoded sheet into `entries[entry_idx].frames` (FUN_004188b0).
 *   cell_w/cell_h == 0 default to the whole sheet (one frame).
 *   cols = sheet_w / cell_w, rows = sheet_h / cell_h, count = cols*rows.
 * Returns 0 (and leaves frames untouched) when count == 0; otherwise sets
 * slot->f_38 = count, allocates the count-entry frames array, and fills it
 * left-to-right, top-to-bottom via ar_frame_build_hook.  Returns 1 iff every
 * cell produced a surface (0 if any build returned NULL — that slot stays
 * NULL but the array is still installed, matching retail's local_c). */
int ar_sprite_slice(ar_sprite_slot *slot, uint16_t entry_idx,
                    const struct bitmap_session *sheet,
                    int cell_w, int cell_h, uint32_t colorkey);

/* The decoder body (FUN_004184a0), entry index 0 (retail's only caller,
 * the frame getter, always passes 0).  Signature matches
 * ar_sprite_decode_fn so it can be installed directly:
 *   ar_sprite_decode_hook = ar_sprite_decode; */
void ar_sprite_decode(ar_sprite_slot *slot);

/* FUN_004179b0 — SS_MGR thiscall slot-clone via pool indices.
 *
 * Retail prototype: `void __thiscall SS_MGR::clone_slot(SS_MGR *this,
 * uint16_t dst_pool_idx, uint16_t src_pool_idx)`.  Both indices are
 * unified pool indices (see `ar_pool_get_slot`); the SS_MGR singleton
 * is the same struct WndProc models as `input_mgr` at 0x008a6b60 (see
 * rabbit-hole §7).
 *
 * Behaviour:
 *   1. Slot side — identical to `ar_sprite_slot_clone` (FUN_00582b80):
 *      destroys dst, stamps metadata (zdd, settings, resource_id, dims,
 *      colorkey, scale_flag, type, group), allocates a fresh 1-entry
 *      `entries[]`, deep-copies src->aux_buf when present.
 *   2. Info-entry side — zeros 14 bytes of dst (same shape as
 *      `ar_info_entry_clear`), then copies `marker` (word@+0) and
 *      `flag` (dword@+4) from src.  `data`/`palette`/`f_10` stay zero.
 *
 * Used by FUN_0057ca40 in 94 calls to expand 54 distinct sources into
 * 94 distinct sprite-frame-variant targets — see
 * `ar_apply_group3_clones` and rabbit-hole §3 for the call-site
 * breakdown.  No other ported caller wires it yet. */
void ar_ss_mgr_clone_slot(uint16_t dst_pool_idx, uint16_t src_pool_idx);

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

/* ─── sprite palette helpers (palette-session trio, leaf half) ─────
 *
 * Two of the three FUN_005749b0 palette-ramp primitives.  The third
 * piece — FUN_004178e0, the "begin palette session" wrapper that
 * pulls the source palette out of a PE bitmap resource via
 * FUN_005b7800 — is NOT ported.  It needs the full PE-resource
 * decoder, which is a separate (much bigger) checkpoint; see
 * `docs/findings/palette-session.md` for the rabbit-hole notes.
 *
 * Both helpers here are usable standalone: the caller can construct
 * a 256-entry PALETTEENTRY buffer in user memory using
 * `ar_palette_pack_entry`, then commit it to a sprite slot using
 * `ar_palette_install`.  Wiring them into ar_register_main_sprites'
 * idx-0 palette ramp waits on the PE decoder. */

/* FUN_005b5d90 — pack a Win32 COLORREF into one 4-byte PALETTEENTRY.
 *
 * PALETTEENTRY layout (wingdi.h): peRed, peGreen, peBlue, peFlags.
 * COLORREF layout:                0x00BBGGRR — low byte = R, mid = G,
 *                                 high = B.  So the retail body's
 *                                 `out[0] = colorref & 0xff;
 *                                  out[1] = (colorref >> 8) & 0xff;
 *                                  out[2] = (colorref >> 16) & 0xff;
 *                                  out[3] = 0`
 * is exactly the R/G/B/peFlags=0 PALETTEENTRY shape.
 *
 * Independent of any container — caller owns `out` (must point to at
 * least 4 bytes). */
void ar_palette_pack_entry(uint8_t *out, uint32_t colorref);

/* FUN_004178e0 — begin a palette session: decode the sprite slot's PE
 * bitmap resource and emit a 256-entry BGRA palette into a caller
 * buffer.
 *
 * Reads `s->settings` (used as HMODULE — the sotesp.dll handle when
 * called on idx 0 from ar_register_main_sprites) and `s->resource_id`,
 * then runs `bs_decode_resource(…, "DATA", 1)` on a stack-local
 * bitmap_session.  If the decoded bitmap is 8bpp, emits its palette
 * via bs_emit_palette_bgra into `out_palette` and returns true.  Any
 * other depth (or a resource-load failure) returns false WITHOUT
 * touching out_palette — caller's buffer is undefined on false.
 *
 * Caller is responsible for the 1024-byte storage at out_palette.
 * The pixel data decoded into the session is discarded — only the
 * palette is exported.
 *
 * Behaviour vs retail: matches FUN_004178e0 modulo C++-exception
 * cleanup.  The retail body uses MSVC SEH (`mov fs:[0], …`) to
 * guarantee bs_release runs on unwind; our port has no exceptions
 * and the explicit bs_release in the success path covers normal
 * teardown.  The defensive `bs_release` retail does TWICE on the
 * success path (once inside the if, once after) is also unnecessary
 * here — bs_release is idempotent and the second call has no
 * additional effect we'd lose. */
bool ar_palette_session_begin(ar_sprite_slot *s, uint8_t out_palette[1024]);

/* FUN_00491770 — lazy-install a 256-entry (1024-byte) palette onto a
 * sprite slot's first entry.
 *
 * Retail body:
 *   if (this->entries[0].b == NULL)
 *       this->entries[0].b = operator_new(0x400);
 *   memcpy(this->entries[0].b, palette, 0x400);
 *
 * The `b` field is the owned-pointer half of an ar_sprite_entry; the
 * sprite slot destructor (ar_sprite_slot_destroy) already frees it
 * iff non-zero, so this install is leak-clean on slot teardown.
 *
 * Caller is responsible for ensuring `s->entries` is non-NULL and
 * `s->entry_count >= 1` — typically by having run
 * ar_sprite_slot_register first.  The retail call sites all do, so
 * this matches observable retail behaviour. */
void ar_palette_install(ar_sprite_slot *s, const uint8_t palette[1024]);

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

/* FUN_0057b280 — register 174 game-sound slots at boot (group 3).
 *
 * Called by the boot driver as `FUN_0057b280(ZDS, 3, settings)` —
 * right after FUN_0057ca40 (deferred) and before FUN_005749b0
 * (ar_register_main_sprites).  Populates pool indices 12..244 with
 * 174 single-slot entries (59 gaps in that range), all with
 * `ar_sound_slot_init` semantics — same six writes ar_register_sounds
 * uses.  No lazy wave-load path (every retail dispatch passes
 * `load_flag = 0`).
 *
 * Retail body is structured as:
 *
 *   1. 122 inline blocks — open-coded `s->zds = …; s->count = …;
 *      s->state = 0; s->group = …; s->resource_id = …; s->settings = …`
 *      sequences (the same six fields, varying issue order).
 *   2.  52 thiscall calls — `ar_sound_slot::FUN_00563ef0(slot, zds,
 *      settings, id, count, group, 0)` for indices the compiler
 *      preferred to route through the helper rather than open-code.
 *      Observable end state is identical to the inline path; all 174
 *      entries flow through `ar_sound_slot_init` here.
 *
 * The conditional locale-table loop at the tail of retail FUN_0057b280
 * is exposed separately as `ar_register_locale_sounds` (walks the
 * 0x24-stride table at &DAT_00691018 — depends on the locale-state
 * globals; boot driver must wire them in).  The 4 inline `FUN_00563ef0`
 * calls the caller (FUN_00562ea0:617-620) issues at indices 22..25 with
 * group=2 are exposed separately as `ar_register_aux_sounds`. */
void ar_register_game_sounds(void *zds, uint16_t group, void *settings);

/* 4 inline `FUN_00563ef0` calls at FUN_00562ea0:617-620.
 *
 * Sit in the boot-driver register sequence BETWEEN FUN_0057a330
 * (group 2 sprite register) and FUN_0057ca40 (group 3) — the four
 * calls share group 2 with FUN_0057a330 and effectively tail it on
 * the sound side.  Not part of FUN_0057b280 even though they
 * populate four indices in the same pool range (22..25).
 *
 * Retail call shape, exactly as observed in the boot driver:
 *
 *   ar_sound_slot::FUN_00563ef0(&pool[22], zds, settings, 0x4cb, 2, 2, 0);
 *   ar_sound_slot::FUN_00563ef0(&pool[23], zds, settings, 0x4ca, 2, 2, 0);
 *   ar_sound_slot::FUN_00563ef0(&pool[24], zds, settings, 0x4c8, 2, 2, 0);
 *   ar_sound_slot::FUN_00563ef0(&pool[25], zds, settings, 0x4c9, 2, 2, 0);
 *
 * All four pass `load_flag = 0`, so the lazy wave-load branch is
 * dead — `ar_sound_slot_init` semantics apply (same six writes as
 * ar_register_sounds / ar_register_game_sounds).
 *
 * Group is parameterised for symmetry with the other register
 * batches, but retail always passes 2. */
void ar_register_aux_sounds(void *zds, uint16_t group, void *settings);

/* Locale state — the three globals the FUN_0057b280 locale loop
 * dereferences (DAT_008a6e68 / _6e70 / _6e80+0x1c8).  Passed in by
 * the boot driver so this module stays Win32-free and per-locale state
 * is testable.
 *
 *   fallback_settings — the "default-locale" settings record (retail
 *                       DAT_008a6e68).  Used as the slot's settings
 *                       pointer for entries with flag == -1, regardless
 *                       of which path the loop takes.
 *   current_settings  — the "active-locale" settings record (retail
 *                       DAT_008a6e70).  NULL means no locale loaded —
 *                       forces the fallback path for every entry.
 *                       When the override path runs, this becomes the
 *                       slot's settings pointer.
 *   launcher_flag     — `*(int*)(*DAT_008a6e80 + 0x1c8)`, a status flag
 *                       in the launcher's settings struct.  NON-ZERO
 *                       suppresses the override path (acts the same as
 *                       no locale being loaded).  Specific meaning
 *                       unknown without modelling the launcher struct
 *                       further. */
typedef struct ar_locale_state {
    void *fallback_settings;
    void *current_settings;
    int   launcher_flag;
} ar_locale_state;

/* FUN_0057b280 tail — the conditional locale-table loop.
 *
 * Walks the 283-entry rdata table at retail address 0x00691018 and
 * dispatches into the W_MGR sound pool keyed on locale state.  Each
 * live entry (primary_id != 0) routes through `ar_sound_slot_init`
 * with settings + resource_id chosen per:
 *
 *   PATH A (fallback) — taken when one of:
 *       override == 0
 *       OR locale->current_settings == NULL
 *       OR locale->launcher_flag != 0
 *     id        = entry.primary_id
 *     count     = entry.count_add + 2
 *     settings  = (entry.flag == -1) ? locale->fallback_settings
 *                                    : caller's `settings`
 *
 *   PATH B (locale override) — taken otherwise (override != 0
 *     AND current_settings != NULL AND launcher_flag == 0):
 *       if entry.override == 0x7fff: skip the entry entirely
 *       else:
 *         id        = entry.override
 *         count     = entry.count_add + 2
 *         settings  = locale->current_settings
 *
 *   ar_sound_slot_init(pool[entry.idx], zds, settings, id, count, group)
 *
 * 15 entries have `primary_id == 0` — they're skipped in both paths
 * (probably parked rows that another subsystem reads via the magic
 * field).  15 entries have `override == 0x7fff`.  29 entries have
 * `flag == -1`.  Touched indices range 160..464 (267 distinct).
 *
 * In the boot driver this is called as the tail of FUN_0057b280 with
 * the same zds / group / settings as the inline+thiscall halves —
 * call after `ar_register_game_sounds` to match retail issue order. */
void ar_register_locale_sounds(void *zds, uint16_t group, void *settings,
                                const ar_locale_state *locale);

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
 * Palette ramp section (PORTED 2026-05-24): between the idx-0 inline
 * register and the trailing-call batch, retail builds a 256-entry
 * palette via ar_palette_session_begin (sotesp.dll 0x90b as the
 * 8bpp source), overrides palette[1]=0, palette[41..50]=0x383838,
 * palette[51..70]=lerp(0x383838→0xffffff, 1/20..20/20), then installs
 * it onto the idx-0 sprite slot via ar_palette_install.  The whole
 * ramp is a no-op when ar_palette_session_begin returns false (e.g.
 * the resource isn't 8bpp). */
void ar_register_main_sprites(void *zdd, uint16_t group, void *settings,
                              void *sotesp_module);

/* FUN_0057a330 — palette-ramp + portrait sprite-register batch.
 *
 * Called by the boot driver (FUN_00562ea0) AFTER ar_register_aux_sounds
 * and BEFORE FUN_0057ca40 (Ghidra-fail deferred function).  Retail call
 * shape: `FUN_0057a330(ZDD, 2, settings)` — `group` = 2.
 *
 * The fourth parameter `sotesp_module` is the sotesp.dll HMODULE — the
 * same DAT_008a6e74 value `ar_register_main_sprites` consumes.  In
 * retail the value is loaded directly from `DAT_008a6e74` at the call
 * site; we parameterise so the boot driver port owns the module
 * pointer.
 *
 * The function does three things in sequence:
 *
 *   1. 12 palette-ramp blocks — each registers a small (24×24 or 32×32)
 *      type-2 sprite at one of the 12 g_ar_sprite_ramp_slots and runs
 *      a per-sprite palette ramp via ar_palette_session_begin /
 *      ar_palette_install.  The ramp overrides:
 *          palette[1]      = bg_color
 *          palette[41..50] = mid_color (10 entries)
 *          palette[51..70] = ar_color_lerp(mid_color, fg_color, i, 20)
 *                            for i=1..20
 *      All 12 sprites use the SAME PE resource ID for their palette
 *      source — but the resource IDs and dimensions differ per ramp
 *      (0x412 32×32 OR 0x413 24×24).  See the ramps[] table in the
 *      port for the full color tuples.  When the decoder fails (no
 *      such resource, wrong bit depth) the ramp body is a no-op,
 *      matching ar_register_main_sprites' palette ramp.
 *
 *   2. 23 trailing FUN_005748c0 calls — sprite slots in the main pool
 *      at retail addrs 0x8a76c4..0x8a7734 (indices 33..61).  Two of
 *      these (idx 36 and 38) are spelled inline as the
 *      destructor-plus-field-writes pattern in retail; same observable
 *      end state as ar_sprite_slot_register, so all 23 flow through
 *      the helper here.  Mix of small icons (32×32 type=2),
 *      medium (96×64 type=2), and panel-sized (192×128 type=2) shapes.
 *
 *   3. 14 portrait-sprite blocks — each registers a tall portrait
 *      sprite (typically 80×352 / 80×480 / 80×320 / 80×144 type=0) at
 *      retail addrs 0x8a7744..0x8a7778 (indices 65..78 in main pool)
 *      AND writes a "flag" value (0 or 3) into
 *      `g_ar_info_table[AR_INFO_RAMP_FLAGS_BASE + i]->flag`.  See
 *      AR_INFO_ENTRY_COUNT for the pool layout.
 *
 * The function-level stack-local `bitmap_session` in retail is a
 * vestigial SEH-protected RAII placeholder — bs_release_no_free'd at
 * entry, bs_release'd at exit, never otherwise touched.  No
 * observable effect; we don't model it. */
void ar_register_palette_ramps(void *zdd, uint16_t group, void *settings,
                               void *sotesp_module);

/* FUN_0056e190 — "hundreds of sprites" register batch.
 *
 * Called by the boot driver as `FUN_0056e190(ZDD, 5, settings)` —
 * immediately after `ar_register_main_sprites` (group 4) and before the
 * "The resource was set" log line.  Group passed is 5.
 *
 * Populates 442 sprite slots in `g_ar_sprite_slots[]` — by far the
 * biggest sprite batch at boot.  Resource IDs span 0x453..0x908; pool
 * indices span 62..863.  The retail body is structured as:
 *
 *   1. 93 INLINE BLOCKS — slots at indices 425..517 (retail addresses
 *      0x8a7ce4..0x8a7e54).  Sequential indices, sequential resource
 *      IDs (0x592..0x5fb).  Each block is the open-coded
 *      destructor-plus-field-writes that the compiler emits when
 *      pre-Ghidra-thiscall info was available — same observable end
 *      state as ar_sprite_slot_register.  72 are 0xa0×0xb0 (scale=1,
 *      type=0) and 21 (resource IDs 0x71f..0x733) are 0xb0×0x90.
 *
 *   2. 349 TRAILING CALLS to FUN_005748c0 — slot pointers come from
 *      `mov ecx, [DAT_]` thiscall setups that Ghidra dropped from the
 *      C view.  Pool indices span 62..863 NON-sequentially (the
 *      compiler/source clearly reordered or grouped by content).  Three
 *      sprite shapes appear:
 *        - 0xa0×0xb0 scale=1 type=0  (portrait/character-sized)
 *        - 0xb0×0x90 scale=1 type=0  (wider UI/character art)
 *        - 0x80×0x80 scale=0 type=2  (small icon-sized — matches the
 *                                     UI-icon pattern from
 *                                     ar_register_main_sprites' tail)
 *
 * No palette ramps, no branching, no other helpers — purely 442
 * single-entry sprite-slot registers.  Implementation is table-driven:
 * one static const entry per retail register call, iterated through
 * `ar_sprite_slot_register`.
 *
 * Provenance: the trailing slot indices come from radare2 disasm of
 * FUN_0056e190 (the inline section comes straight from the Ghidra
 * decomp; the trailing thiscall ECX setups were re-extracted from
 * raw bytes via `pD 0x672c @ 0x56e190 | awk`). */
void ar_register_game_sprites(void *zdd, uint16_t group, void *settings);

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

/* FUN_0057ca40 — group-3 sprite-register batch (PARTIAL PORT).
 *
 * Called by the boot driver as `FUN_0057ca40(ZDD, 3, settings)` — between
 * ar_register_aux_sounds and ar_register_game_sounds.  Group is 3.
 *
 * The retail body (24884 B) is decompilable post the typed-struct
 * Ghidra-recovery workflow (the HANDOFF's "Ghidra-fails" note was made
 * before that infra landed).  But the body contains FOUR distinct
 * subsystems, only the first of which is ported in this checkpoint:
 *
 *   1. **233 sprite-slot registers** — 91 inlined (the open-coded
 *      destructor + field-writes pattern) + 142 helper-style
 *      (ar_sprite_slot::FUN_005748c0 calls).  All registers use the
 *      caller's `(zdd, settings, group)` uniformly; every slot index is
 *      distinct (range 79..423 in the main sprite pool).  Order between
 *      registers is observably irrelevant (each writes to a different
 *      slot).  **PORTED HERE** as a 233-entry static table iterated
 *      through ar_sprite_slot_register.
 *
 *   2. **443 ar_info_entry writes** at retail BSS 0x008a85b0..0x008a8b14
 *      — `entry->marker` / `entry->flag` / `entry->data` writes plus 5
 *      tail struct-copies (pool[257..261] = struct-copy pool[139..145]),
 *      4 marker-copies, and 4 flag-copies across pool indices 92..437.
 *      **PORTED HERE** as the 4th pass: a 443-entry static event table
 *      walked by `ar_apply_group3_info_events`, called from the tail of
 *      `ar_register_group3_sprites`.  DATA_SET payloads are retail PE
 *      rdata addresses stored as opaque uintptr_t — observability-only
 *      until the first consumer (FUN_00586010-style palette draw) is
 *      ported and needs the real bytes.  Audited by
 *      `tools/extract/57ca40_pool_map.py` (0 orphans).
 *
 *   3. **94 FUN_004179b0 calls** — SS_MGR thiscall "clone slot from
 *      another slot" operations (dst_pool_idx, src_pool_idx).  Clones
 *      one slot's metadata to another, plus zeroes the destination
 *      info-entry and copies the source info-entry's marker (word@+0)
 *      and flag (dword@+4).  **PORTED HERE** as the 5th pass: a
 *      94-entry static table walked by `ar_apply_group3_clones`, called
 *      from the tail of `ar_register_group3_sprites` (after the 4th
 *      info-events pass).  Both indices flow through the unified pool
 *      accessor `ar_pool_get_slot`; the SS_MGR singleton == input_mgr
 *      identity (rabbit-hole §7) means we don't need to plumb a
 *      `this` pointer through the call.  Sources span 54 distinct
 *      pool indices (147..335) — all within the 233 slots the
 *      register pass populates, which is why the clones come AFTER.
 *
 *   4. **9 FUN_00582b80 calls + 4 FUN_00582d00** at the tail —
 *      another SS_MGR-style thiscall pattern that clones fields from
 *      a "this" slot into a target slot.  **PORTED HERE** as the 6th
 *      pass: a 9-entry static table (sources pool 383/390/402; targets
 *      257-261, 384-385, 391-392) walked by
 *      `ar_apply_group3_inline_clones`, called from the tail of
 *      `ar_register_group3_sprites` after the SS_MGR clones.  Each
 *      replay calls `ar_sprite_slot_clone(pool[dst], pool[src])` —
 *      info-entry side of each cluster (zero + marker/flag-copy +
 *      data-ptr for the 4 early ones; 20-byte STRUCT_COPY for the 5
 *      late ones) is already covered by the 4th-pass event table.
 *
 * With all six passes ported, FUN_0057ca40 is functionally complete:
 * pixel-drawer and the title-scene boot path observe
 * `g_ar_sprite_slots[79..437]` as fully populated with the right
 * (rid, w, h, ck, sf, type) tuples, and the parallel info-entry
 * table mirrors retail's pool state including all derived
 * (cloned/copied) entries.  Per-prefix semantics on info-entry flag
 * values (the 0/1/2/3 dispatch FUN_00586010 reads) remain open work
 * — the value bytes are correct, but their per-pool-index meaning
 * isn't catalogued yet.
 *
 * Provenance: docs/decompiled/by-address/57ca40.c (3124 lines).
 * Generated by tools/extract-57ca40.py from the Ghidra C export. */
void ar_register_group3_sprites(void *zdd, uint16_t group, void *settings);

/* 4th pass of FUN_0057ca40 — apply 443 info-entry pool writes in
 * retail issue order.  Called from the tail of `ar_register_group3_sprites`
 * (so boot-driver paths get it for free); also exposed standalone so
 * tests can observe the pool transitions in isolation from the slot
 * loop.  All writes target `g_ar_info_table[i]` indices that the
 * pool allocator already wired up in `ar_state_init`. */
void ar_apply_group3_info_events(void);

/* 5th pass of FUN_0057ca40 — replay 94 SS_MGR slot-clone calls
 * (FUN_004179b0) in retail issue order via `ar_ss_mgr_clone_slot`.
 * Called from the tail of `ar_register_group3_sprites` (after the
 * info-events pass), also exposed standalone for unit tests.
 *
 * Each clone reads (slot + info) at the source pool index and writes
 * (slot + info) at the destination pool index.  All 94 sources are
 * distinct from the 94 destinations, so the order between clones is
 * observably irrelevant — we replay in retail issue order anyway for
 * provenance.
 *
 * Sources MUST be populated before this runs.  In the boot driver,
 * that's true: the slot-register pass populates the sources first
 * (sources span pool indices 147..335 which overlap the 233-slot
 * register region), and the info-events pass primes the source info
 * entries' marker+flag fields. */
void ar_apply_group3_clones(void);

/* 6th pass of FUN_0057ca40 — replay 9 inline FUN_00582b80 slot-clone
 * calls in retail issue order via `ar_sprite_slot_clone`.  Called from
 * the tail of `ar_register_group3_sprites` after the SS_MGR clones;
 * also exposed standalone for unit tests.
 *
 * Each clone reads the source slot and overwrites the target slot
 * (allocating a fresh `entries[]` and deep-copying `aux_buf`).  All 3
 * source pool indices (383/390/402) are themselves populated by the
 * slot-register pass at the head of `ar_register_group3_sprites`; all
 * 9 target pool indices (257-261, 384-385, 391-392) are disjoint from
 * the source set, so the order between clones is observably
 * irrelevant.  Info-entry side of each cluster lives in the
 * group3_info_events[] table — no info writes happen here.
 *
 * The 4 "early" clusters (target 384/385, 391/392) pair each clone
 * with a `FUN_00582d00 + marker/flag-copy + DATA_SET` info-event
 * triple; the 5 "late" clusters (target 257..261) pair each clone
 * with a 20-byte STRUCT_COPY info-event.  Both info-side patterns
 * land in the 4th pass; this 6th pass closes the slot side. */
void ar_apply_group3_inline_clones(void);

/* FUN_00562ea0:613-624 — boot-driver asset-register wiring.
 *
 * Replays the engine's "register every asset slot at boot" sequence in
 * the same issue order retail uses.  Each ported register batch lands
 * on the same slot pool, so a single call here populates the full
 * sprite/sound/GDI state the title scene expects.
 *
 * Issue order (from FUN_00562ea0):
 *
 *   ar_register_fonts          (zdd, 1, settings)            // FUN_00579bd0
 *   ar_register_sounds         (zds, 1, settings)            // FUN_00579a00
 *   ar_register_palette_ramps  (zdd, 2, settings, sotesp)    // FUN_0057a330
 *   ar_register_aux_sounds     (zds, 2, settings)            // 4× inline FUN_00563ef0
 *   ar_register_group3_sprites (zdd, 3, settings)            // FUN_0057ca40 (PARTIAL)
 *   ar_register_game_sounds    (zds, 3, settings)            // FUN_0057b280 head
 *   ar_register_locale_sounds  (zds, 3, settings, locale)    // FUN_0057b280 tail
 *   ar_register_main_sprites   (zdd, 4, settings, sotesp)    // FUN_005749b0
 *   ar_register_game_sprites   (zdd, 5, settings)            // FUN_0056e190
 *
 * Retail dispatches every batch with `param_3 = DAT_008a6e74` — the
 * sotesp.dll HMODULE the launcher loaded.  In retail that one global
 * doubles as both "settings" and "sotesp_module" because the engine's
 * launcher-settings record IS the sotesp.dll handle at this point in
 * boot.  We keep the conceptual split (`settings` vs `sotesp_module`)
 * so unit tests can distinguish them.  Pass the same pointer for both
 * to match retail behaviour byte-for-byte; pass distinct pointers to
 * exercise the split.
 *
 * Group tags (1..5) are baked in — they're parameters of each ported
 * batch but retail always passes the same constants in this sequence.
 *
 * `locale` plumbs the three locale-state globals (DAT_008a6e68 / _6e70
 * / *DAT_008a6e80+0x1c8) into `ar_register_locale_sounds`.  Pass NULL
 * to skip the locale tail entirely (useful for testing the rest of the
 * sequence in isolation; retail always passes a valid struct).
 *
 * `ar_register_group3_sprites` is the PARTIAL port of FUN_0057ca40 —
 * 233 sprite-slot register operations plus 443 info-entry pool writes
 * (the 4th pass; via `ar_apply_group3_info_events` called from its
 * tail).  The remaining ~100 operations (94 SS_MGR clone calls and
 * the 9 inline-clone clusters' sprite-slot ops) are deferred — see
 * the ar_register_group3_sprites docstring for the full breakdown.
 * No consumer of the deferred clones is ported yet. */
void ar_boot_register_all(void *zdd, void *zds, void *settings,
                          void *sotesp_module,
                          const ar_locale_state *locale);

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
