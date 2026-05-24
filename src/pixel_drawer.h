/*
 * src/pixel_drawer.h — Pixel Drawer module (partial port).
 *
 * Ported from `sotes.unpacked.exe` functions FUN_005bd020 / FUN_005bd380 /
 * FUN_005bcff0 / FUN_005bd3b0 / FUN_005bd4d0 — the leaf primitives of
 * the "ZDPixelDrawer" subsystem (the engine's RGB color-blend slot
 * manager used for fades, tints, and UI text colour binding).  See
 * `docs/findings/winmain-and-bootstrap.md` "Pixel Drawer slot tables"
 * for the boot-time use of these.
 *
 * The slot is a 0x50-byte struct holding three 20-byte sub-blocks (one
 * per R/G/B channel) plus slot-level fields (overall weight, blend
 * mode, invert flag, …).  The drop-in needs this for:
 *
 *   1. Allocating the 69 boot-time slots that the asset register
 *      populates with sprite metadata.
 *   2. Setting per-slot tint colours (`FUN_005bd3b0(R, G, B)`).
 *   3. Committing those colours into hardware-encoded form
 *      (`FUN_005bd3d0` — the commit path; LUT builder still TBD,
 *      see TODO).
 *
 * Not yet ported (next pass):
 *   - FUN_005bd040 (LUT builder, 801 B, four blend modes)
 *   - FUN_005bd3d0 (slot commit — depends on the LUT builder)
 *   - FUN_005b8ae0 (read RGB masks from the ZDD wrapper — Win32-side
 *     hook into DDraw surface descriptor)
 */
#ifndef OPENSUMMONERS_PIXEL_DRAWER_H
#define OPENSUMMONERS_PIXEL_DRAWER_H

#include <stddef.h>
#include <stdint.h>

/* ─── PdChannel — one per R/G/B channel (20 bytes) ───────────────────── */

typedef struct PdChannel {
    /* +0x00 (4B): "channel shift" — encoded position of the channel's
     * MSB in a 16-bit packed pixel (FUN_005bd380(mask) result; see
     * pd_channel_mask_to_shift below).  For RGB565: R=11, G=6, B=0. */
    int32_t  shift;
    /* +0x04 (4B): raw RGB bitmask (e.g. 0xF800 for R in RGB565).
     * Set by pd_blend_commit (TBD) from the engine's surface format. */
    uint32_t mask;
    /* +0x08 (4B): pointer to the lookup-table buffer (lazily allocated
     * by the LUT builder, size 0x400 for mode 1 or 0x20 for mode 2).
     * NULL when not allocated. */
    uint8_t *lut;
    /* +0x0C (2B): per-channel weight (0..1000 — pixel intensity input). */
    uint16_t weight;
    /* +0x0E (2B): padding (Ghidra shows the next u32 starts at +0x10). */
    uint16_t _pad0e;
    /* +0x10 (4B): non-zero when `lut` was allocated by us and needs
     * pd_channel_free_lut to free it on reset. */
    uint32_t lut_allocated;
} PdChannel;

/* Compile-time size sanity: we want the same layout the retail engine
 * uses so allocation sizes and offsets are interchangeable.  20 bytes
 * matches the boot-driver's 5×u32 reads on each channel.  Only valid
 * on 32-bit builds (the drop-in is i686-mingw); host x86_64 tests
 * have 8-byte pointers and skip these. */
#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(PdChannel) == 20, "PdChannel must be 20 bytes");
#endif

/* ─── PdBlend — a slot (80 bytes), 3 channels + slot-level fields ───── */

typedef struct PdBlend {
    /* +0x00 (4B): state machine — 0=idle/identity, 1=set, 2=set+inverted
     * (driven by the lut_allocated flag at +0x4c and the slot-level
     * weight at +0x40; see pd_blend_commit TBD). */
    uint32_t state;
    /* +0x04 .. +0x3f (60B): three R/G/B channel sub-blocks. */
    PdChannel r;
    PdChannel g;
    PdChannel b;
    /* +0x40 (2B): slot-level weight 0..1000.  Set to 1000 (full intensity)
     * by pd_blend_init; the boot driver overrides it per-slot
     * immediately after init for the per-group ramps. */
    uint16_t  weight;
    /* +0x42 (2B): padding. */
    uint16_t  _pad42;
    /* +0x44 (4B): blend mode (1..4).  Set externally after pd_blend_init
     * — pd_blend_init does NOT touch this.  The LUT builder uses this
     * to pick one of four blend formulas. */
    uint32_t  mode;
    /* +0x48 (4B): "invert" flag — when non-zero, FUN_005bd040 inverts the
     * input axis in the LUT formulas (`iVar6 = local_8` instead of
     * `iVar6 = param_2`). */
    uint32_t  invert;
    /* +0x4C (4B): "commit requested" flag — when set to 1, commit will
     * force state to 2 (the "always apply" branch). */
    uint32_t  commit_flag;
} PdBlend;

#if UINTPTR_MAX == 0xFFFFFFFFu
_Static_assert(sizeof(PdBlend) == 0x50, "PdBlend must be 80 bytes");
_Static_assert(offsetof(PdBlend, r)           == 0x04, "PdBlend r offset");
_Static_assert(offsetof(PdBlend, g)           == 0x18, "PdBlend g offset");
_Static_assert(offsetof(PdBlend, b)           == 0x2c, "PdBlend b offset");
_Static_assert(offsetof(PdBlend, weight)      == 0x40, "PdBlend weight offset");
_Static_assert(offsetof(PdBlend, mode)        == 0x44, "PdBlend mode offset");
_Static_assert(offsetof(PdBlend, invert)      == 0x48, "PdBlend invert offset");
_Static_assert(offsetof(PdBlend, commit_flag) == 0x4c, "PdBlend commit_flag offset");
_Static_assert(offsetof(PdChannel, weight)        == 0x0c, "PdChannel weight offset");
_Static_assert(offsetof(PdChannel, lut_allocated) == 0x10, "PdChannel lut_allocated offset");
#endif

/* ─── functions ──────────────────────────────────────────────────────── */

/* FUN_005bd380.  Returns the bit shift needed to align the MSB of a
 * channel mask to bit 11 of a 16-bit packed pixel.
 *
 *   RGB565 R=0xF800 → 11   (MSB at bit 15, shifted right by 4 to land at bit 11)
 *   RGB565 G=0x07E0 →  6
 *   RGB565 B=0x001F →  0
 *   RGB555 R=0x7C00 → 10
 *
 * The return value is "11 - leading_zeros_from_bit_15"; negative is
 * possible (mask MSB at bit < 4 — never happens with real DDraw masks). */
int32_t pd_channel_mask_to_shift(uint32_t mask);

/* FUN_005bd020.  Zero a channel and seed `weight = 1000`. */
void pd_channel_init(PdChannel *c);

/* FUN_005bcff0.  Free the channel's LUT buffer if allocated. */
void pd_channel_free_lut(PdChannel *c);

/* FUN_005bd4d0.  Slot constructor — zeroes state, inits all three
 * channels (per pd_channel_init), and sets slot-level weight to 1000.
 * Does NOT touch `mode` (the caller sets it externally — see
 * `winmain-and-bootstrap.md` Pixel Drawer slot-tables). */
void pd_blend_init(PdBlend *b);

/* FUN_005bd3b0.  Set the per-channel weights.  Does NOT encode/allocate
 * LUTs — that's a separate commit step.  Most slots stay in [0..1000]
 * but the boot-time "grey-ramp" and "special-color" slots deliberately
 * use values up to ~2000 to produce over-bright tints — the field is a
 * u16 specifically to allow this. */
void pd_blend_set_color(PdBlend *b, uint16_t r, uint16_t g, uint16_t b_chan);

/* PdFormat — the three RGB bitmasks the commit needs from the active
 * DDraw surface.  Matches the offsets the retail engine reads from the
 * ZDD wrapper (FUN_005b8ae0 reads `this[1]`, `this[2]`, `this[3]` —
 * byte offsets 4, 8, 0xc).  We don't pull in the whole ZDD wrapper for
 * the leaf module's tests; the renderer-side glue can adapt.
 *
 * Pass NULL to pd_blend_commit for the engine's default RGB565
 * (R=0xF800, G=0x07E0, B=0x001F) — what the retail code uses when its
 * ZDD ptr argument is NULL. */
typedef struct PdFormat {
    uint32_t r_mask;
    uint32_t g_mask;
    uint32_t b_mask;
} PdFormat;

/* FUN_005bd3d0.  Slot "commit" — re-encodes the per-channel masks from
 * the surface's pixel format, updates the slot state machine, and
 * (re)builds the three channel LUTs.
 *
 * Sequence:
 *   1. Frees any existing LUTs on R/G/B (`pd_channel_free_lut`).
 *   2. Reads R/G/B masks: from `fmt` if non-NULL, else RGB565 defaults.
 *   3. For each channel: stores the mask and its
 *      `pd_channel_mask_to_shift`-encoded shift.
 *   4. Resolves slot->state from the slot-level fields:
 *        commit_flag == 1                          → state 2 (forced apply)
 *        weight == 1000 AND mode == 0              → state 0 (identity)
 *        otherwise                                 → state 1 (full LUT)
 *   5. Builds the three channel LUTs in order R, G, B, passing
 *      R as `prev` to both G and B (matches the retail quirk —
 *      B can share-alias with R but never with G).
 *
 * After this returns, the slot is "live": surface blits use the
 * channel LUTs to translate pixel values. */
void pd_blend_commit(PdBlend *slot, const PdFormat *fmt);

/* FUN_005b8ae0.  Read the active surface's RGB masks into three output
 * locations.  The retail signature is `void(this, *r, *g, *b)` with the
 * ZDD wrapper as `this` — we model that as a PdFormat argument.  Kept
 * as a standalone export so a future ZDD-wrapper module can expose it
 * verbatim. */
void pd_format_get_masks(const PdFormat *fmt,
                         uint32_t *r_out, uint32_t *g_out, uint32_t *b_out);

/* ─── Boot-time slot tables (FUN_00562ea0:462-576) ──────────────────────
 *
 * Five fixed-size slot groups (20 + 20 + 5 + 4 + 20 = 69 slots) plus
 * four special-colour writes into the 4-slot group D.  The retail
 * driver allocates 69 zero-initialized 0x50-byte slot objects via
 * `operator_new(0x50)` and stores pointers in five global arrays:
 *
 *   group  retail addr   count   per-slot fields set in the loop
 *   ──────────────────────────────────────────────────────────────────────
 *   A      DAT_008a92b8  20      weight = iVar20/20,  mode = 1
 *   B      DAT_008a9308  20      weight = iVar20/22 (mode untouched, = 0)
 *   C      DAT_008a9358   5      set_color(c,c,c) with c = 0x44c + iVar20/5
 *   D      DAT_008a93bc   4      none in loop; filled by 4 special writes
 *   E      DAT_008a936c  20      weight = iVar20/20,  mode = 2
 *
 * `iVar20` in groups A/B/E walks 1000, 2000, …, 20000.  In group C it
 * walks 0, 800, 1600, 2400, 3200 (so c = 1100, 1260, 1420, 1580, 1740).
 *
 * The four special-colour writes (FUN_00562ea0:0x5637f1-0x5638b6,
 * disassembly-verified — Ghidra's source view omits the explicit
 * `mov ecx, [addr]` before each thiscall, so the target slot of each
 * pair is ambiguous from the decomp alone):
 *
 *   target   commit_flag  set_color args
 *   ───────────────────────────────────────────────
 *   D[0]     (stays 0)    ( 600,  600,  600)
 *   D[1]     (stays 0)    (1000,  600,  600)
 *   D[3]     ← 1          (1000, 1800, 2000)
 *   D[2]     ← 1          ( 900,  920,  930)
 *
 * Each slot then gets pd_blend_commit invoked with the active surface
 * format (the retail `ZDD` ptr at DAT_008a93cc).
 *
 * For the port we use *static* PdBlend arrays rather than heap allocations:
 * the retail engine never frees these (process-lifetime), and static
 * storage keeps the host tests deterministic and ASan-quiet across
 * repeated runs of the boot driver.  If a future consumer ever needs
 * pointer arrays matching the retail BSS layout, expose a parallel
 * `PdBlend *g_pd_boot_*_ptrs[]` view then. */

enum {
    PD_BOOT_GROUP_A_COUNT = 20,
    PD_BOOT_GROUP_B_COUNT = 20,
    PD_BOOT_GROUP_C_COUNT = 5,
    PD_BOOT_GROUP_D_COUNT = 4,
    PD_BOOT_GROUP_E_COUNT = 20,
};

extern PdBlend g_pd_boot_group_a[PD_BOOT_GROUP_A_COUNT];   /* DAT_008a92b8 */
extern PdBlend g_pd_boot_group_b[PD_BOOT_GROUP_B_COUNT];   /* DAT_008a9308 */
extern PdBlend g_pd_boot_group_c[PD_BOOT_GROUP_C_COUNT];   /* DAT_008a9358 */
extern PdBlend g_pd_boot_group_d[PD_BOOT_GROUP_D_COUNT];   /* DAT_008a93bc */
extern PdBlend g_pd_boot_group_e[PD_BOOT_GROUP_E_COUNT];   /* DAT_008a936c */

/* Allocate-and-commit every boot slot in the five groups.  Re-runnable:
 * each call drops any LUTs left over from a previous run (pd_blend_commit
 * does this internally) and re-encodes against the current `fmt`.
 *
 * `fmt` is the active DDraw surface format, or NULL to use the engine's
 * RGB565 defaults (same convention as pd_blend_commit). */
void pd_boot_init_slots(const PdFormat *fmt);

/* Free every LUT currently held by the boot slots.  The retail engine
 * leaves these allocated for the process lifetime (the OS reclaims at
 * exit), so the real drop-in does not need to call this — it exists
 * purely so host-side ASan/unit tests can release after each run. */
void pd_boot_release_slots(void);

/* FUN_005bd040.  Build (or share) the channel's LUT.
 *
 * `prev` is the previously-built channel (or NULL).  When `prev` is
 * non-NULL and has the same channel weight as `chan`, this is the
 * **shared-LUT short-circuit**: `chan->lut` is aliased to `prev->lut`
 * and lut_allocated stays at 0 (we don't own it, the previous channel
 * does — pd_channel_free_lut is correctly a no-op on shared aliases).
 *
 * Otherwise allocates and fills one of two LUT shapes based on
 * `slot->state`:
 *
 *   state == 0 or 2 → 32-byte 1-D LUT: lut[k] = clamp(w*k / 1000),
 *                     where w is the channel weight and k is either
 *                     the index or its bit-reversed twin (32-k) when
 *                     `slot->invert` is set.  This is the "identity
 *                     scaling" LUT used when the slot is idle or in
 *                     the simple-attenuation state.
 *
 *   state == 1     → 1024-byte 2-D LUT (32 × 32 rows of bytes), filled
 *                     by one of five blend formulas selected by
 *                     `slot->mode`:
 *                     • 1 = "add": out = i + W*s/1000, clamped ≥ i
 *                     • 2 = "sub": out = i - W*s/1000, clamped ≤ i
 *                     • 3 = "lerp variant A": (1-W)*i + (s-i)*W
 *                     • 4 = "channel-weight-coupled lerp" (mixes
 *                       channel weight `w` into the formula directly
 *                       and skips the final `*w/1000` scaling)
 *                     • other = identity unless s != i, then case-3 form
 *                     For modes 1/2/3/default the result is post-scaled
 *                     by channel weight before clamping to [0, 31].
 *                     `s` is either `inner` (column) or `32 - inner`
 *                     when `slot->invert` is set.
 *
 *   anything else  → no-op (no allocation).
 *
 * After this returns `chan->lut` may be either a fresh allocation
 * (lut_allocated=1, free with pd_channel_free_lut) or a shared
 * pointer into `prev->lut` (lut_allocated=0). */
void pd_blend_build_channel_lut(PdBlend  *slot,
                                PdChannel *chan,
                                PdChannel *prev);

#endif /* OPENSUMMONERS_PIXEL_DRAWER_H */
