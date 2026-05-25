# OpenSummoners — Progress log

Append-only changelog.  Newest entries first.  Each entry: date + heading,
then 1–3 short paragraphs.  Cross-link to `docs/findings/*.md` and
specific commits where relevant.

---

## 2026-05-25 — ZDDObject ctor + dtor + pixel-buf release PORTED

Three more leaf ports landing on top of the ZDD wrapper checkpoint
that closes the "ZDDObject cleanup chain" open thread:

  - FUN_005b9350  `zdd_object_ctor`              50 bytes
  - FUN_005b9390  `zdd_object_dtor`              75 bytes
  - FUN_005b93e0  `zdd_object_release_pixel_buf` 42 bytes

ZDDObject struct shape pinned at 0xd8 bytes with the 6 lifecycle-
pair fields named (`com_primary` at +0x2c, `com_back` at +0xac,
`parent` at +0xc0, `pixel_buf` at +0xc4, `pixel_buf_flag` at +0xc8,
`state_flag` at +0xd4).  The embedded DDSURFACEDESC2 + window-fit
metrics regions (`_pad030[0xac-0x30]` and `_pad0b0[0xc0-0xb0]` +
`_pad0cc[0xd4-0xcc]`) stay as opaque pad until the surface-alloc
helpers (FUN_005b97e0 / _98c0) get ported alongside FUN_005b95c0.

The previously-placeholder `zdd_obj_destroy` in zdd_win32.c gets
replaced — it's now pure logic in zdd.c that walks
`zdd_object_dtor` then heap-frees the allocation.  Only the
`zdd_object_local_free` primitive remains on the Win32 side (wraps
LocalFree).

Release order in `zdd_object_dtor` matches retail's
FUN_005b9390 byte-for-byte: pixel buf first, then com_back (+0xac),
then com_primary (+0x2c), then parent->open_objects decrement.  The
"com_back BEFORE com_primary" order is the load-bearing detail — it
keeps the COM refcount graph clean when com_back is an
IDirectDrawSurface7 fetched via GetAttachedSurface off com_primary
(open thread per docs/findings/ddraw-init.md
"FUN_005b9520 — Clipper attach" notes).

7 new host tests; the two pre-existing release-children + dtor tests
were updated to use real malloc'd ZDDObjects instead of synthetic
pointers (zdd_obj_destroy now dereferences).

Tests now: **224 pass, 0 fail, 6 skip** (up from 217/5; 7 new
including a 32-bit-only zdd_object layout skip).

---

## 2026-05-25 — ZDD wrapper first slice PORTED (8 functions)

First slice of the DirectDraw 7 wrapper class HANDOFF flagged as the
recommended "next move" — the eight leaf functions in
`docs/findings/ddraw-init.md`'s call graph that together cover the
class lifecycle + DDraw init + DDERR error logging.  Lands in
`src/zdd.{c,h}` + `src/zdd_win32.c` + `tests/test_zdd.c`.

Ports (in size-ascending order):

  - FUN_005b8da0  `zdd_restore_cursor_on_release`     33 bytes
  - FUN_005b88c0  `zdd_directdraw_create_ex`          57 bytes
  - FUN_005b89d0  `zdd_set_coop_level`                71 bytes
  - FUN_005b7fe0  `zdd_dtor`                          90 bytes
  - FUN_005b7f80  `zdd_ctor`                          94 bytes
  - FUN_005b8040  `zdd_release_children`             139 bytes
  - FUN_005b7ee0  `zdd_create`                       153 bytes
  - FUN_005b80d0  `zdd_log_dderr`                    826 bytes

Pure-logic split matches the established bitmap_session / wnd_proc
pattern: ctor, dtor decision tree, DDERR-to-string mapping, and the
log-message builder live in zdd.c; the six Win32 primitives
(ShowCursor, OutputDebugStringA, IUnknown::Release via vtable[2],
DirectDrawCreateEx, IDirectDraw7::SetCooperativeLevel via vtable+0x50,
placeholder ZDDObject destroyer) live in zdd_win32.c.  Host tests
exercise the pure logic with controllable stubs.

The DDERR log message format was a small detective exercise: r2
pszj on each of the seven strings the helper concatenates (verified
against `docs/decompiled/by-address/5b80d0.c` and a fresh
`vendor/unpacked/sotes.unpacked.exe` read) showed retail uses commas-
in-place-of-periods in "Warning,exists ZDD errors," and " failed,Error
Code " — not typos, intentional.  The 18-entry HRESULT → DDERR_xxx
table in `k_dderr_table[]` mirrors the switch ladder in FUN_005b80d0
verbatim; format output is fully exercised by 4 host tests covering
known/empty-prefix/unknown-hresult/null-input paths.

Open follow-ups now:
- ZDDObject (`FUN_005b9350` ctor + the FUN_005b9390 cleanup chain) is
  still unported; `zdd_obj_destroy` in zdd_win32.c is a `free()`
  placeholder that will dispatch through the cleanup chain once
  ZDDObject lands.  Host tests don't touch it.
- Vtable indices for the COM Release calls (+0x128 / +0x12c) match
  IUnknown's standard but the semantic role of those two com pointers
  hasn't been pinned — `com_a` / `com_b` are deliberately vague.
  Likely IDirectDrawPalette + IDirectDrawClipper given the surrounding
  code, confirm when their setters land.
- `pixel_format_mode` / `pixel_format_bpp` (+0x164/+0x168) are written
  by paths we haven't ported yet (FUN_005b8c00 reads them when building
  DDSURFACEDESC2 in `mode == 2` paths).  Modelled as fields for size
  correctness; no consumer in this checkpoint.

Tests now: **217 pass, 0 fail, 5 skip** (up from 200 pass / 4 skip;
the new skip is `zdd_layout_matches_retail_offsets`, 32-bit only).
Real mingw build adds `-lddraw -ldxguid`.

---

## 2026-05-25 — Pixel-Drawer boot-time slot tables PORTED

Picks up an open-thread item from HANDOFF (the 5 fixed-size sprite-slot
allocator loops inside `FUN_00562ea0` lines 462-576).  All five groups
(`DAT_008a92b8` ×20, `DAT_008a9308` ×20, `DAT_008a9358` ×5,
`DAT_008a93bc` ×4, `DAT_008a936c` ×20 — total 69 slots) plus the four
special-colour writes that populate group D land in
`src/pixel_drawer.c` as `pd_boot_init_slots(fmt)` + a companion
`pd_boot_release_slots()` for host-test teardown.  All primitives this
calls into (`pd_blend_init`, `pd_blend_set_color`, `pd_blend_commit`)
were already ported in the first Pixel-Drawer pass — the boot driver
is purely orchestration.

Also corrects a finding doc: `winmain-and-bootstrap.md` claimed the
group-D 4 slots were "filled in later by code we haven't yet mapped" —
disassembly of FUN_00562ea0:0x5637f1-0x5638b6 (via radare2) shows the
4 special-colour writes ARE in the same boot phase, just written
inline as 4 explicit `mov ecx, [addr]` thiscalls that Ghidra's source
view collapsed into ambiguous untyped calls.  Targets are
D[0]/D[1]/D[3]/D[2] (in that order; D[2] and D[3] also get
commit_flag=1).  Boot driver replays this exact sequence.

Storage choice: static `PdBlend g_pd_boot_group_*[]` arrays rather
than retail's `PdBlend *DAT_X[]` heap-pointer-arrays.  Retail's slots
are process-lifetime allocations from `operator_new(0x50)` that are
never freed — static storage gives the same observable end-state with
zero malloc and is ASan-quiet under repeated boot.  If a future
consumer ever needs the pointer-array layout, add a parallel
`PdBlend *g_pd_boot_*_ptrs[]` view then.

Tests now: **200 pass, 0 fail, 4 skip** (up from 192).  8 new tests:
per-group weight/mode/state checks (A weight ramp /20 mode 1,
B weight ramp /22 mode 0, C grey-ramp R=G=B = 1100..1740,
E weight ramp /20 mode 2), group D 4 special-colour assignments
including the D[3]→D[2] retail-quirky order, full-coverage check
that every slot in every group commits its RGB masks from `fmt`,
custom-format propagation (RGB555 spot-check), and idempotency
re-run.  The idempotency test caught a real bug — `pd_blend_init`
zeroes channel.lut without freeing it first, which leaks on re-init
of a static slot — fixed by having `pd_boot_init_slots` call
`pd_boot_release_slots` at entry.  This is a host-build concern only
(retail allocates a fresh slot each boot via operator_new, so the
issue doesn't manifest in the real engine).

---

## 2026-05-25 — FUN_0057ca40 6th pass: 9 inline slot-clones PORTED (function functionally complete)

Closes the last deferred subsystem of FUN_0057ca40.  The 9 inline
FUN_00582b80 calls (the ones taking ECX = a `paVar1 = DAT_X` source
slot rather than going through the SS_MGR table) are extracted by
`tools/extract/57ca40_inline_clone_table.py` and replayed in retail
issue order by `ar_apply_group3_inline_clones`, called from the tail
of `ar_register_group3_sprites` after the SS_MGR clone pass.  3
distinct source pool indices (383, 390, 402) — all themselves
populated by the 1st-pass slot-register table — fan out into 9
disjoint targets (257..261, 384..385, 391..392).

No new primitive needed: each replay is just
`ar_sprite_slot_clone(pool[dst], pool[src])`.  The info-entry side
of each cluster (zero + marker/flag-copy + data-ptr for the 4 early
clusters; 20-byte STRUCT_COPY for the 5 late ones) is already
covered by the 4th-pass `ar_apply_group3_info_events` — verified by
re-running the audit tool `57ca40_pool_map.py` (0 orphans across all
443 pool writes).

With this pass landed, FUN_0057ca40's six retail-observable
subsystems (slot register, info events, SS_MGR clones, inline
clones, plus the two thiscall primitives) all replay in the port.
The next consumer of this state is `FUN_00586010` (palette-draw with
flag dispatch — see rabbit-hole §6); porting it will pin the
per-prefix flag semantics from the read side.

Tests now: **192 pass, 0 fail, 4 skip** (up from 187).  5 new tests
cover: target population after register, late-cluster shared-source
metadata propagation, early-cluster metadata propagation, apply
idempotency, src/dst-set disjointness.  Updated 2 existing tests
to reflect the new slot-count expectation (327 → 336).

See `docs/findings/0057ca40-rabbit-hole.md` §4 for the cluster
source/target table.

---

## 2026-05-25 — FUN_0057ca40 5th pass: 94 SS_MGR slot-clones PORTED

Last of the sprite-slot work in FUN_0057ca40.  The 94 FUN_004179b0
calls inside the function are extracted by
`tools/extract/57ca40_clone_table.py` and replayed in retail issue
order by `ar_apply_group3_clones`, called from the tail of
`ar_register_group3_sprites` after the 4th-pass info-events apply.
Total clones: 94 (54 distinct sources, 94 distinct destinations;
sources span main_slot 134..321, destinations span main_slot 135..322,
all within the 233-slot register region populated by the same
function's earlier pass).

The primitive `ar_ss_mgr_clone_slot(dst_pool_idx, src_pool_idx)`
reuses the existing `ar_sprite_slot_clone` (slot-metadata copy) and
`ar_info_entry_clear` (info-entry zero) primitives, since
FUN_004179b0's body is structurally identical to those primitives'
bodies — only the indirection differs.  Modeling sidesteps the SS_MGR
`this` pointer via a new unified-pool accessor `ar_pool_get_slot`
that maps pool indices 1..12 → ramp slots and 13..908 → main slots
(see rabbit-hole §7: SS_MGR == input_mgr at 0x008a6b60, so the host
already owns both tables as globals).

Tests now: **187 pass, 0 fail, 4 skip** (up from 176).  11 new tests
cover: pool accessor on sentinel/ramp/main ranges, primitive-level
clone (metadata propagation, info marker+flag copy, info data/palette
stay null, dst-entries destruction under ASan), table-walker (apply
idempotency, dst pool range, first-clone metadata propagation,
integration with register_group3_sprites).  Integration count test
updated: register pass writes 233 slots + clone pass writes 94 more
= 327 unique populated slots.

The remaining FUN_0057ca40 deferred work is now strictly on the **9
FUN_00582b80 cluster wiring** — open-coded template-slot init per
cluster, not table-extractable.  See HANDOFF "Next move" #3.

---

## 2026-05-25 — FUN_0057ca40 4th pass: 443 info-entry pool writes PORTED

Mechanical follow-on to the per-call-site indexing confirmation:
extracted the full info-entry event stream and replayed it as a
443-row static table walked by `ar_apply_group3_info_events`,
called from the tail of `ar_register_group3_sprites`.  Every write
the function performs to the parallel info-entry pool now lands in
the host model: 138 marker, 194 flag, 98 data-ptr, 5 struct-copy,
4 marker-copy, 4 flag-copy events spanning pool indices 92..437.

Extractor at `tools/extract/57ca40_info_table.py` mirrors the
`57ca40_sprite_table.py` model — re-run after re-export to catch
drift.  It captures 4 short-typed data-ptr writes (lines 2142,
2147, 2286, 2291) that the `57ca40_pool_map.py` audit's regex
missed — taking the real total from 439 to 443.  Sanity check
verifies all dst indices fall in [0, 909); the 5 struct copies'
sources (pool[139..145]) are not produced inside FUN_0057ca40, but
they're alloc-zeroed by the pool allocator and read at zero —
matching retail's allocator-zeroed pool semantics.

DATA_SET payloads (98 events; 25 distinct PE rdata addresses, e.g.
0x006748d0) are stored as opaque uintptr_t markers.  No consumer
reads them as bytes yet — the first FUN_00586010-style palette
draw with flag dispatch will need extracted PE bytes; this port is
observability-only on the data side.  8 new spot-check tests cover
the kinds (FLAG_SET, MARKER_SET, DATA_SET, MARKER_COPY, FLAG_COPY,
STRUCT_COPY) plus the bounded-region invariant (events only touch
pool[92..437]) plus the wiring through `ar_register_group3_sprites`.
Tests now: **176 pass, 0 fail, 4 skip** (up from 168).

The remaining FUN_0057ca40 deferred work is now strictly on the
**sprite-slot side**: 94 SS_MGR clones (FUN_004179b0) plus the 9
inline-clone clusters (FUN_00582b80 sprite-slot ops).  Info-entry
side is closed.

---

## 2026-05-24 — FUN_0057ca40 per-call-site indexing confirmed

Walked all 466 info-entry references inside FUN_0057ca40 and matched
them against slot decls + clone targets in the same function.  The
implicit "pool[i] shadows slot[i]" model is fully confirmed: 0 of
434 pool writes are orphaned (i.e. every info-entry write at retail
BSS `0x8a8440 + i*4` corresponds to a slot at `0x8a760c + i*4`,
where the slot is either declared inline, declared via the helper,
or produced by SS_MGR clone / inline-template clone).  Audit script:
`tools/extract/57ca40_pool_map.py` — rerun after re-export to catch
drift.

The walk also surfaced a Ghidra rendering gotcha for `DAT_008a8XXX`
references: different DAT vars carry different inferred C types
(byte-typed vs short-typed), so source-level offsets like `+2` vs
`+4` can both denote the same disasm byte offset (+4 = flag).
Verified by disasm at 0x57cad7 (byte-typed, mov [eax+4]) vs
0x57cf3d (short-typed, mov [ecx+4] but Ghidra source says +2).
Rabbit-hole §2 rewritten with the correction; the "pad +2..+3 is
never touched" claim from §4 is reaffirmed (no Ghidra +2 source
write actually targets byte +2 in retail).

This unblocks the 434-write port — it's now mechanical (compose
slot-idx → flag/marker/data tuples in retail issue order, replay
into `g_ar_info_table[i]`).  Deferred because no consumer reads the
info-entry pool yet; the first FUN_00586010-style palette draw with
flag-dispatch will need it.

---

## 2026-05-24 — ar_info_entry pool (909 entries) + allocator finding

Followed the "where do the parallel-table pointer slots come from"
thread to its root and unblocked the full pool model.  The allocator
is **FUN_00562ea0:225-253** — a single 909-iteration loop that runs
right before the "SS_MGR_Preparation" log line.  It heap-allocates
two parallel pools side-by-side: a 0x44-byte sprite slot AND a
0x14-byte info entry per index, stored in adjacent BSS pointer
tables at 0x8a760c..0x8a8440 and 0x8a8440..0x8a9270.

That pins three corrections we'd been waving past:

  - **`ar_info_entry` is 20 bytes, not 16.**  The allocator zeros
    five dwords (the last being `+0x10`); the existing `clear`
    routine only touches the first 14 bytes.  Struct + static
    asserts updated.
  - **`+0x0c` is a palette pointer**, not the "f_0c semantics
    unknown" placeholder.  FUN_00586010:755 reads it as
    `*(int ***)(DAT + 0xc)`, uses it directly as the source of a
    256-entry palette modifier loop when non-NULL, or falls back to
    `ar_palette_session_begin` + `FUN_00417bc0(entry->data, ...)`
    when NULL.  Renamed `f_0c` → `palette`.
  - **The "parallel pool" is 909 entries, not ~357.**  Retail BSS
    range 0x8a8578..0x8a8b14 (the rabbit-hole's "extends to ~357
    entries" estimate) is just pool indices 78..437 of the full
    909-entry table.

`g_ar_sprite_flags[14]` (flat uint32) replaced by
`g_ar_info_entries[909]` + `g_ar_info_table[909]`.  `ar_state_init`
wires the table.  `ar_register_palette_ramps` now writes through the
table: `g_ar_info_table[AR_INFO_RAMP_FLAGS_BASE + i]->flag = N`,
matching retail's `*(int *)(DAT_008a85xx + 4) = N` pattern.

One new pool-init test (168 pass / 0 fail / 4 skip, up from 167).
Two existing info-entry tests refactored for the new field name
(`f_0c` → `palette`) and the +0x10 field's leave-untouched
guarantee.  `docs/findings/0057ca40-rabbit-hole.md` extended with
sections 5 (allocator finding) and 6 (FUN_00586010 + FUN_00587e00
consumer evidence).  HANDOFF's "Open RE threads" entries on
g_ar_sprite_flags and the parallel-pool are now obsolete.

---

## 2026-05-24 — FUN_00582b80 (slot clone) + FUN_00582d00 (info entry clear)

Ported the next two functions from FUN_0057ca40's deferred subsystem
list: `ar_sprite_slot_clone` (the `__thiscall` slot metadata clone)
and `ar_info_entry_clear` (a 14-byte clear of a parallel-info-table
entry).  Together they form the "clone-and-detach pair" that appears
9× in FUN_0057ca40 — see `docs/findings/0057ca40-rabbit-hole.md`
section 4 for the disasm walk and the new struct discovery below.

The big finding is **`ar_info_entry`**, the 16-byte parallel-info-
table entry shape that HANDOFF previously called out as "each retail
entry is itself a POINTER to a struct."  Disasm at 0x57fa98 confirms
FUN_00582d00's `this` is loaded from `[0x008a8a40]` — a pointer in
the parallel table — and the writes pin the layout: u16 marker @+0,
pad @+2, u32 flag @+4, const void* data @+8 (later set to PE rdata
pointers like &DAT_006752f8), u32 f_0c @+12.  Struct + static
asserts now live in `src/asset_register.h`.

`ar_sprite_slot_clone` reuses `ar_sprite_slot_destroy` for its
free-old-state prologue, then stamps every metadata field from src
to dst in retail order, allocates a fresh 1-entry `entries[]`, and
deep-copies src's `aux_buf` (24-byte stride entries, count from
src->f_38).  Retail quirk preserved: `dst->f_38` stays at 0 even
when the aux deep-copy runs — the count isn't propagated; we match.

7 new unit tests (167 pass, 0 fail, 4 skip — up from 160).  Both
functions tagged in `TagThiscallFunctions.java` (26 tags now);
parse+tag stage confirmed `ar_info_entry=16` parses cleanly and
both new functions land in their class namespaces.  Module-isolation
still holds: no real caller is ported yet — the functions are
available primitives for the eventual FUN_0057ca40 wiring once
SS_MGR and the parallel-info-table array land.

---

## 2026-05-24 — Headless Parse C Source automation

Ported the `ParseCSource.java` script from sibling OpenMare to
`tools/ghidra-scripts/`, plus a `tools/ghidra-cpp-shim/` directory
with minimal `stdint.h` / `stddef.h` / `stdbool.h` shims for
Ghidra's bundled CPP (it has no libc).  ParseCSource passes the
shim dir as an include path and `-D_Static_assert(c,m)=` to strip
the C11 keyword from our headers, so `src/asset_register.h`,
`src/bitmap_session.h`, and `src/wnd_proc.h` parse cleanly into
Ghidra's DTM in headless mode.

`tools/ghidra-tag-and-export.sh` upgraded to a 3-stage pipeline
running in one analyzeHeadless session: ParseCSource → TagThiscall
Functions → ExportDecompiledC.  Verified end-to-end: all 14 structs
parsed (sizes match — paint_ctx=368, zdm_entry=56, log_singleton=1284,
etc.), 24/24 tags applied, 1768 functions re-exported.

Immediate payoff: the bodies of the 7 WndProc-dep thiscalls now
show typed `this->field` accesses.  paint_ctx::FUN_005b9130 reads
`if (this->state == 2) { BitBlt(hdc, this->blit_x, this->blit_y,
this->blit_w, this->blit_h, ...); FUN_005b94e0(this->back_ctx, ...); }`.
The WndProc itself reads as a clean class-dispatched function
(`input_mgr::FUN_0058ffa0((input_mgr *)&DAT_008a6b60, 1)`,
`zdm::FUN_005bbd20(DAT_008a93e4, ...)`, etc.).  No more
"open Ghidra GUI + Parse C Source" manual step — `nix develop -c
./tools/ghidra-tag-and-export.sh` is the single command for
struct-edit → typed-decomp.

See `docs/findings/cpp-recovery-workflow.md` "Automated parsing +
tagging" section for the full how-to + new-header discipline.

---

## 2026-05-24 — WndProc dependency formalization

Modeled the 5 "deep engine" struct shapes the WndProc reaches through
its 5 thiscall callees, and tagged each callee in Ghidra so its
prototype and class namespace get applied to the decomp.  The structs
live in `src/wnd_proc.h`'s new "deep-engine struct shapes" section
and pin only the offsets observed in the disasm:

  - **paint_ctx** (DAT_008a93cc) — +0x2c zdd_device, +0x138 blit
    rect (x/y/w/h), +0x164 state.  `this` for FUN_005b9130 (the
    BitBlt-from-backbuffer paint helper), FUN_005b94e0 (begin-frame
    vtable trampoline at zdd_device->vtable[0x44]), and FUN_005b9500
    (end-frame at vtable[0x68]).
  - **input_dev** (DAT_008a93d8, DAT_008a93dc[2]) — +0x04 dev_obj
    (vtable[0x1c] = Acquire), +0x08 acquired flag.  `this` for
    FUN_005ba290.
  - **zdm** (DAT_008a93e4) — +0x18 entries pointer, +0x1c count,
    +0x2c inline name string.  `this` for FUN_005bbd20 (the
    multiplexer set-active fan-out).  Per-entry struct **zdm_entry**
    has stride 0x38 with +0x00 dev, +0x08/+0x0c sub-device pointers
    (each with own vtable), +0x20 active, +0x24 state2, +0x28
    8-byte cookie.
  - **input_mgr** (singleton at &DAT_008a6b60) — +0x2884 zdm_ptr.
    `this` for FUN_0058ffa0 (input pause-on-deactivate; just NULL-
    guards and forwards to FUN_005bbd20).
  - **log_singleton** (singleton at &DAT_008a6620) — +0x404 path
    CHAR buffer.  `this` for FUN_00408b90 (the engine's
    OutputDebugString + log-file writer).

The wnd_proc.h externs (`g_wp_paint_check_this`, `g_wp_input_dev_extra`,
`g_wp_input_devs[2]`, `g_wp_zdm`) were upgraded from `void *` to the
typed pointer forms, and `wp_input_acquire`'s parameter became
`input_dev *` accordingly.

7 new tags added to `tools/ghidra-scripts/TagThiscallFunctions.java`
(now 24 total).  Headless tag step verified: 24/24 applied.  The
re-export was kicked off — the 7 new functions now show in the decomp
with class namespace + explicit `this` arg at every call site
(typed-body upgrade requires Parse C Source on wnd_proc.h in the
Ghidra GUI, then re-running the script — see HANDOFF "Next move" #1).

Tests unchanged at **160 pass, 0 fail, 4 skip** — the WndProc test
suite still binds `(void *)0xN` literal addresses into the new typed
globals via implicit `void *` → `T *` conversion.  Cross-build clean.

---

## 2026-05-24 — ar_register_group3_sprites (FUN_0057ca40 partial)

Ported the 233 sprite-slot register operations inside FUN_0057ca40 —
the "Ghidra-fails 24884 B body" from the prior HANDOFF turned out to
decompile cleanly once the typed-struct workflow from the previous
checkpoint was in scope (the `cpp-recovery-workflow` infra silently
fixed it).  3124-line decomp is at `docs/decompiled/by-address/57ca40.c`.

But the body isn't just "register N sprites" — it has FOUR
subsystems.  Only the first is ported here:

  1. **233 sprite-slot registers** (91 inlined + 142 helper-style
     calls).  Slot indices 79..423, all using uniform (zdd, settings,
     group) routing from the caller.  Table-driven through
     `ar_sprite_slot_register`.  **PORTED** as
     `ar_register_group3_sprites` and wired into `ar_boot_register_all`
     between aux_sounds and game_sounds (matches retail issue order).
  2. ~380 parallel-info-table writes (0x008a8578..0x008a8b14).
     **DEFERRED** — needs `g_ar_sprite_flags[]` refactored from flat-u32
     to pointer-to-struct array (~357 entries).
  3. 94 FUN_004179b0 SS_MGR slot-clones.  **DEFERRED** — needs SS_MGR.
  4. 9 FUN_00582b80 + 1 FUN_00582d00 tail.  **DEFERRED**.

See `docs/findings/0057ca40-rabbit-hole.md` for the full breakdown.
Generator at `tools/extract/57ca40_sprite_table.py` — re-run after
re-exporting the decomp to spot drift.

Tests: +7 new tests in `tests/test_asset_register.c` (distinct-slot
canary, group-tag stamping, uniform routing, three spot-checks for
specific entries, no-overlap-with-other-batches assertion).  Plus
the existing `boot_register_all_touches_every_batch_signature_slot`
test now also pins the group-3 spot-check on sprite[79].
**160 pass, 0 fail, 4 skip** (was 153 / 0 / 4).  Cross-build clean.

Asset-register module is now **functionally complete for the title-
scene boot path** — every `ar_register_*` batch that the boot driver
calls is ported, and no ported consumer reads the deferred FUN_0057ca40
state.

---

## 2026-05-24 — ar_boot_register_all wired

The 8 ported `ar_register_*` batches are no longer modules in isolation
— a new `ar_boot_register_all` in `asset_register.c` calls them in
retail issue order, matching FUN_00562ea0:613-624 byte-for-byte modulo
the one un-ported batch (FUN_0057ca40, group 3, 24884 B Ghidra-fails).
This is the "register every asset slot at boot" entry point: pass ZDD,
ZDS, settings, sotesp_module, and a locale state, and every ramp /
sprite / sound / GDI slot the title scene depends on lands populated.

API shape `ar_boot_register_all(zdd, zds, settings, sotesp_module,
locale)` keeps the conceptual settings-vs-sotesp split (in retail both
are the same DAT_008a6e74 pointer at boot; we accept them separately so
unit tests can distinguish "this register batch reads settings" from
"this batch reads sotesp.dll"). `locale == NULL` skips the locale-tail
batch entirely — useful for testing other batches in isolation; retail
always passes a valid struct.

The FUN_0057ca40 gap is marked with an inline comment at the exact
position it'd slot into (between aux_sounds and game_sounds).  No hook
mechanism — once Ghidra-fail is resolved and the function is ported,
the call is dropped in.

Tests: +6 new tests in `tests/test_asset_register.c` covering group-tag
routing, ZDD-vs-ZDS plumbing, the sotesp-module split (idx 0 +
ramp_slots use sotesp; everything else uses settings), locale state
plumbing, NULL-locale skip behaviour, and a "did every batch run"
canary check.  **153 pass, 0 fail, 4 skip** (was 147 / 0 / 4).  The
palette-install side of palette_ramps is a no-op in these tests
because the bs_load_pe_resource stub's resource table starts empty
when asset tests run; the install path is already covered separately
by test_bitmap_session.c's palette_ramps_* tests.

Cross-build clean.

---

## 2026-05-24 — ar_register_palette_ramps (FUN_0057a330) ported

Ported the 3919-byte sprite-batch palette function as
**`ar_register_palette_ramps`** — second-biggest sprite-register call
at boot.  Three observable sections, all wired in this checkpoint:

**12 palette-ramp blocks** — each registers a small (24×24 or 32×32)
type-2 sprite at one of 12 new `g_ar_sprite_ramp_slots[i]` (retail
BSS 0x008a7610..0x008a763c, a 12-pointer table that precedes the
main sprite pool's 0x008a7640 base), runs the same 3-color palette
ramp scheme as `ar_register_main_sprites`
(palette[1]=bg, [41..50]=mid, [51..70]=lerp(mid→fg, i/20)) with
per-ramp colors, then installs.  All 12 share the
`ar_palette_session_begin` / `ar_palette_install` path that landed
in the previous checkpoint — no new decoder code needed; the family
is now reused.  When the resource decoder fails (wrong bit depth
or missing resource) the install is skipped, matching the main
sprites ramp behaviour.

**23 trailing sprite registers** — main-pool indices 33..61 with
mixed icon / panel shapes.  Two of these (idx 36 at retail 0x76d0
and idx 38 at retail 0x76d8) are spelled inline as the
destructor-plus-field-writes pattern in retail; same observable end
state as `ar_sprite_slot_register`, so all 23 flow through the
helper here.  One entry (idx 37 at retail 0x76d4) is the only
register-call in the file that passes `settings=NULL` instead of
the launcher settings — special-cased in the iteration loop.

**14 portrait blocks** — each is a register-call at retail
0x8a7744..0x8a7778 (main pool indices 65..78, portrait/character art
80×{352,480,320,144,400}) followed by a write of a flag value (0 or
3) into a new parallel `g_ar_sprite_flags[]` table (14 entries
modelling the retail BSS region 0x008a8578..0x008a85ac).  The flag's
semantic meaning is unknown — likely a frame-count or facing-direction
override; no consumer is ported yet so we capture just the observable
+4 write into a flat uint32 array (the retail pointer-to-struct
indirection is unmodelled).

Function-level stack-local `bitmap_session` in retail is a vestigial
SEH-protected RAII placeholder — `bs_release_no_free`'d at entry,
`bs_release`'d at exit, never used.  No observable effect; not
modelled.

Tests: +9 new tests in `tests/test_bitmap_session.c` covering all
three sections.  **147 pass, 0 fail, 4 skip** (was 138 / 0 / 4).
Cross-build clean.  Ghidra TAGS array also gained the two thiscall
helpers it uses (FUN_004178e0 / FUN_00491770) so the re-exported
decomp shows typed `this->field` access through the family.

---

## 2026-05-24 — bitmap_session module + palette ramp wired end-to-end

New module **src/bitmap_session.[ch] + src/bitmap_session_win32.c** —
the 7-method `__thiscall` class behind the PE-resource bitmap decoder
in FUN_004178e0's palette-session front half, plus FUN_005b7c10
(compressed-resource header parser, a free function despite living in
the same family).  Lifecycle is entirely stack-managed in
FUN_004178e0 — the bitmap_session is `[esp+8]` over a 0x444-byte
frame.  Win32-free body; `bs_local_alloc_zeroed` / `bs_local_free` /
`bs_load_pe_resource` are externs supplied per build target.

The blocking ECX puzzle from the prior session's deferral (which
`this` does FUN_005b7800 actually run on?) was resolved by r2 disasm
of FUN_004178e0 — every callsite does `lea ecx, [esp+8]` before the
call, confirming the stack-local interpretation.  Outer this
(sprite_slot * in ESI) is read only for the HMODULE+resource_id pair
passed to FindResourceA.  Resource type string is "DATA", not "BMP"
as the prior findings draft assumed.

**ar_palette_session_begin** (FUN_004178e0, ar_sprite_slot method)
lands in asset_register.c — builds the stack session, calls
`bs_decode_resource(..., "DATA", 1)`, emits the BGRA palette into a
caller buffer iff the source was 8bpp.  Then
**ar_register_main_sprites' palette-ramp section** (previously
deferred) is now wired: allocate a 1024-byte buffer, seed via
ar_palette_session_begin from sotesp.dll/0x90b, override palette[1]=0,
palette[41..50]=0x383838, palette[51..70]=lerp(0x383838→0xffffff,
i=1..20 / 20), install onto slot[0] via ar_palette_install.  No-op
when the decoder can't return a palette.

Ghidra workflow improvement: TagThiscallFunctions.java's TAGS array
gained the 7 bitmap_session methods; re-export shows
`__thiscall bitmap_session::FUN_…(bitmap_session *this, …)`
throughout the family and `bitmap_session local_444[1080]` as the
typed stack local in FUN_004178e0.

Tests: +21 new bitmap_session tests (basic state, init/release,
compressed-header signature mismatch + happy path, raw + compressed
decode paths, ar_palette_session_begin BGRA emit + 24bpp skip, and
end-to-end ar_register_main_sprites integration).  **138 pass, 0
fail, 4 skip** (was 117 / 0 / 3 — the new skip is the
bitmap_session layout test, 32-bit-only).  Win32 cross build clean.
Commits: `8cb9fd8` (struct+tags+findings), `4f89867` (port+tests).

---

## 2026-05-24 — Asset-Register: palette-trio leaves (FUN_005b5d90 + FUN_00491770)

Ported the two leaf halves of the FUN_005749b0 palette-ramp trio
that don't need the PE-resource decoder:

**`ar_palette_pack_entry`** (FUN_005b5d90, 33 B) — pack a Win32
`COLORREF` (`0x00BBGGRR`) into one `PALETTEENTRY` (peRed, peGreen,
peBlue, peFlags=0).  Independent of any container.  Used between
the seed step and install step to override or lerp individual
palette entries.

**`ar_palette_install`** (FUN_00491770, 52 B) — lazy-install a
256-entry (1024-byte) palette onto a sprite slot's first entry.
Allocates `s->entries[0].b` on first call; the existing
`ar_sprite_slot_destroy` already frees it iff non-zero, so the
round trip is leak-clean.  The Ghidra decomp's
`*(int *)(*in_ECX + 4)` pattern is `(*this)+4` →
`entries[0].b` of `ar_sprite_entry` — `entries[0].b` is the
owned-pointer half of the entry record.

The third piece — `FUN_004178e0`, "begin palette session" — is NOT
ported.  It needs the whole PE-resource decoder chain
(`FUN_005b7800` + `FUN_005b71f0` + `FUN_005b7c10` + the small
release-helper group + `FUN_005b7b90` RGBA↔BGRA swap).  Blocking
question: which `this` does `FUN_005b7800` actually run on?  The
offsets `+0x3c` / `+0x40` in FUN_004178e0 match `ar_sprite_slot`
(`settings`, `resource_id`), but the bitmap-session layout
FUN_005b7800 needs (pixel buffer at +0x00, palette at +0x34..+0x434)
doesn't fit overlaid on an `ar_sprite_slot` (which has `entries`
at +0x00 and `aux_buf` at +0x34).  Most likely the actual ECX is
reset before the FUN_005b7800 call — the Ghidra decomp drops
__thiscall ECX setups for un-tagged callsites.  Full layout
analysis and resolution path are in
`docs/findings/palette-session.md`.

Tests: +6 (pack basic / top-byte ignore / overwrite; install
lazy-alloc / reuse / destroy round trip).  **117 pass, 0 fail,
3 skip**.  Win32 cross build clean.  Commits: `d3e8a00`, `6db790d`.

---

## 2026-05-24 — Asset-Register: FUN_0057b280 tail (ar_register_locale_sounds + ar_register_aux_sounds)

Closed FUN_0057b280's deferred backlog from the previous checkpoint
— two distinct ports landing in the same session:

**`ar_register_aux_sounds`** — the 4 inline `ar_sound_slot::FUN_00563ef0`
calls the boot driver (`FUN_00562ea0:617-620`) issues between
`FUN_0057a330` and `FUN_0057ca40`.  Hardcoded indices 22..25 with
IDs 0x4cb / 0x4ca / 0x4c8 / 0x4c9 (issue order), count 2 each,
group 2.  Same `ar_sound_slot_init` semantics as the rest of the
sound batches (`load_flag = 0`).  Tests: +3.  Commit: `d4198b0`.

**`ar_register_locale_sounds`** — the conditional locale-table loop
at the tail of retail FUN_0057b280.  Walks the 283-entry rdata
table at `0x00691018` (terminator = first +0x00 == 0 row) and
dispatches into the W_MGR sound pool keyed on three launcher-
settings globals (DAT_008a6e68 / _6e70 / _6e80+0x1c8), now exposed
as an `ar_locale_state` struct.  Two paths:
  - **PATH A (fallback)** when override==0 OR no current locale OR
    launcher flag suppresses it.  Resource id = entry.primary_id;
    settings = (flag==-1) ? locale.fallback : caller's settings.
  - **PATH B (locale override)** otherwise.  override == 0x7fff
    is the skip-when-active sentinel.  Resource id = entry.override;
    settings = locale.current.

Touched indices span 160..464 (267 distinct) — required bumping
`AR_SOUND_SLOT_COUNT` 256 → 512 to fit the 465-entry retail W_MGR
pool (allocated 0x1d1 entries in FUN_00562ea0's SS_MGR_Preparation
block).  Added `AR_SOUND_POOL_COUNT = 465` as the documented exact
retail capacity.

Table data extracted via `r2 px @ 0x691018` + a Python parser on
the resulting hex.  Only the five fields the loop actually reads
are kept in the static const C array; the magic / sequence /
metadata fields are summarised in the table-extraction comment.

Field shape observations from the parsed data (vs the previous
HANDOFF notes that pegged magic as 0xc35a):
- 23 distinct magic values appear in live entries (0xc35a..0xc35d,
  0xc4ae, 0xc754, 0xc756, 0xc760, 0xc77f, 0xc789, 0xc792, 0xc79c,
  0xc80b, 0xc829, 0xc83d, 0xe2a4..0xe2a8, 0x1874e, 0x18755, 0x18759).
  Magic is NEVER read by the loop — likely a zone/area tag for some
  other subsystem.
- field4 (`u32` at +0x04) is a per-locale group selector 1..73 (with
  gaps), monotonic per magic — looks like a "scene_id" the locale
  pre-loader can filter on.
- 15 entries have primary_id == 0 (sentinels skipped by the loop's
  `if (resource_id != 0)` early-out).  15 have override == 0x7fff.
  29 have flag == -1.
- count_add (`i16` at +0x14) is only ever 0 or 2; flag (`i32` at
  +0x18) is only ever 0 or -1; pad16 / field1e are always 0.

Tests: +7 (no-locale path → primary_id + fallback-or-settings,
primary_id==0 skip semantics, launcher_flag forces fallback,
override path under live locale, 0x7fff skip sentinel, coexistence
with `ar_register_game_sounds` at the 160..244 overlap, lazy-load
buffer pointer preservation).  **111 pass, 0 fail, 3 skip**.  Win32
cross build clean.  Commit: `aec8f15`.

---

## 2026-05-24 — Asset-Register: FUN_0057b280 (ar_register_game_sounds)

The "game sounds" boot-register batch — the sixth call in
`FUN_00562ea0`'s asset-register sequence (right after `FUN_0057ca40`,
called as `FUN_0057b280(ZDS, 3, settings)`).  Populates **174
single-slot sound-bank entries** in `g_ar_sound_table[]` covering
pool indices 12..244 (with 59 sparse gaps in that range).  Same
six-field write pattern as `ar_register_sounds` — every entry routes
through `ar_sound_slot_init` since the retail compiler's choice
between inline blocks (122 entries) and `FUN_00563ef0` thiscall
dispatches (52 entries) is observably identical (load_flag=0,
buffer untouched).

The pool-pointer table at `(&DAT_008a6ec4)[i]` only ran out to idx 11
in the previous port (the original AR_SOUND_SLOT_COUNT cap of 12 came
from `ar_register_sounds`).  Bumped `AR_SOUND_SLOT_COUNT` to **256**
(covers FUN_0057b280's max idx 244 with headroom) and renamed the
old 12-cap constant to `AR_SOUND_MAIN_COUNT` so `ar_register_sounds`
still loops over its exact 12-entry roster.  No retail BSS size for
the contiguous pool past 0x008a6ec4 is documented yet; bump again if
a later batch overruns.

Entry data lifted from the Ghidra decomp via a quick regex sweep
(122 `puVar2 = DAT_…; … puVar2[6] = ID;` blocks + 52 thiscall calls);
issue order preserved so any future call-trace test matches without
renormalisation.

**Deferred** — NOT in this port:

1. The 4 inline `FUN_00563ef0` calls the caller (`FUN_00562ea0:617-620`)
   issues at indices 22..25 with group=2 (IDs 0x4c8..0x4cb).  These
   sit between FUN_0057a330 and FUN_0057b280 in the boot sequence and
   write three slots in the "gap" of FUN_0057b280's range; need their
   own tiny helper when the boot driver gets ported.
2. The conditional locale-table loop at the tail of retail
   FUN_0057b280 (walks the 0x24-stride table at `&DAT_00691018`,
   dispatches into the pool keyed on locale state at DAT_008a6e68 /
   _6e70 / _6e80).  This is the language-pack / per-locale sound
   override path — needs reading the structured rdata table at
   0x00691018 and modelling the launcher-settings struct fields the
   branch reads.

Tests: +6 (total-entry-count 174, index range bounds + sample gaps,
field-write spot check across all 5 count buckets {1,2,4,6,8,16},
all-pairs distinct resource IDs, coexistence with `ar_register_sounds`
without group-tag stomping, lazy-load `buffer` preservation on re-
register).  Total **101 pass, 0 fail, 3 skip**.  Win32 cross build
clean.

---

## 2026-05-24 — WndProc: FUN_005b12e0 (wp_handle_message)

Ported the main game window's WndProc — the message handler
RegisterClassExA wires up for the engine's primary window.  The
function is small in code (441 bytes / 84 decomp lines) but
load-bearing: it owns `DAT_008a952c`, the "WM_ACTIVATEAPP wParam
mirror" the engine's outer pump (`FUN_005b1030`) spins waiting for.
The current Frida agent posts a fake `WM_ACTIVATEAPP(TRUE)` to flip
this flag because hidden retail windows don't naturally see the
message from the shell; a correctly-ported WndProc unblocks dropping
that workaround once we own the window registration.

Split into three TUs following the asset-register pattern:

- **`src/wnd_proc.c`** — pure logic, Win32-free.  Decodes the 9
  message classes the dispatch cares about (WM_DESTROY/MOVE/SIZE/
  PAINT/CLOSE/ACTIVATEAPP/KEYDOWN/TIMER + default→DefWindowProc),
  with the WM_ACTIVATEAPP activation half being the meaty branch —
  acquires the "extra" input device (with CP1/CP2 log surround),
  iterates the 2-slot device array, emits the unconditional CP3 log,
  flips the ZDM activation state, then runs the post-activate hook.

- **`src/wnd_proc_win32.c`** — Win32 adapters.  `wp_def_window_proc`
  → DefWindowProcA, `wp_app_exit` → ExitProcess, `wp_log_cp` →
  OutputDebugStringA.  The five "deep engine" hooks (paint_check,
  app_pause, input_acquire, zdm_set_active, post_activate) are
  placeholder no-ops — none of those subsystems are ported yet, but
  swapping each for a real call is a one-line change once they are.

- **`src/wnd_proc.h`** — typedefs Win32 message types as pointer-
  sized integers so the pure logic compiles + tests on Linux.
  Models `wp_app_ctx` with just the fields FUN_005b12e0 reads
  (`f00` head of the device-init pointer chain, `loaded`, `timer`).

The "device init flag" subtlety: retail's activation path computes
`bVar1 = !(ctx->f00 && *ctx->f00 && (*ctx->f00)[+0x18])` then passes
`!bVar1` to ZDM.  I.e. the ZDM arg = "the chain is fully wired".
Disasm at 0x5b13b5..0x5b13c8 + 0x5b1462..0x5b146a confirms this is
literal pointer-deref-pointer-deref + +0x18 read.  Modelled with two
test cases that build the chain explicitly with stack-local int
buffers + a sub-pointer.

Tests: +20 (harmless messages, close→exit, paint short-circuit
combinations, ACTIVATEAPP flag-write semantics, full call-order
spec for the activation path, log-quiet gate, sparse loop, ZDM
arg = init_flag both true and false, timer field-clear, state
reset, layout assert).  Total **95 pass, 0 fail, 3 skip**.  Win32
cross build clean (single-TU mingw picks the new .c files up
automatically).

Not done in this commit: tagging the WndProc's dependency thiscalls
(FUN_005b14c0 / _0058ffa0 / _005ba290 / _005bbd20 / _005b9130 /
_00408b90) in Ghidra.  The script needs each class struct in the
DTM via Parse C Source, and we only modelled one (`wp_app_ctx`) —
the rest are opaque void* in the port.  Defer to a follow-up that
formalizes the input/ZDM/paint-context layouts.

---

## 2026-05-24 — Asset-Register: FUN_0056e190 (ar_register_game_sprites)

The "hundreds of sprites" boot-register batch — the fifth call in
`FUN_00562ea0`'s asset-register sequence (right after
`ar_register_main_sprites`, called as `FUN_0056e190(ZDD, 5, settings)`).
By far the biggest sprite batch at boot: **442 single-entry sprite
registers** packed into a table-driven port that iterates
`ar_sprite_slot_register` once per entry.

The retail decomp is 2782 lines structured as:

- **93 inline blocks** at idx 425..517 (BSS 0x008a7ce4..0x008a7e54)
  — the compiler chose to open-code the destructor + field-write
  sequence rather than emit a call, because the `this` pointer was
  visible as a global.  Resource IDs are sequential 0x592..0x5fb.
  72 use shape (0xa0×0xb0, scale=1, type=0); 21 (resource IDs
  0x71f..0x733, idx 467..487) use (0xb0×0x90, scale=1, type=0).

- **349 trailing FUN_005748c0 thiscalls** with implicit-ECX slot
  pointers that Ghidra dropped from the C view (only the 8
  non-ECX args show up).  Re-extracted the ECX setups via
  `r2 -c 'pD 0x672c @ 0x56e190' | awk '/mov ecx, dword \[0x8a/{last=...} /call 0x5748c0/{print last}'`
  → paired one-to-one with the 349 C-decomp arg lists by file
  order.  Three sprite shapes: 0xa0×0xb0 / 0xb0×0x90 (type=0, scale=1)
  and 0x80×0x80 (type=2, scale=0 — small icon).  Touches 346
  indices in idx 518..863 plus the low-index stragglers at idx 62/63/64
  (resource IDs 0x608/0x609/0x60a, the 0x80×0x80 icon shape).

**Pool refactor**: `AR_SPRITE_SLOT_COUNT` bumped 64 → 1024 to fit
the new high-water-mark (idx 863) with headroom for the remaining
batches (FUN_0057a330, _57ca40, _57b280 likely add a few dozen more
slots).  Retail's contiguous BSS region past 0x8a7640 is plenty
large; storage cost is ~70 KB BSS.

Tests: +6 (inline-block field-map at shape-shift boundaries,
trailing-call shape spot-check across all three shapes + low-idx
stragglers, total slot count = 442, resource-id uniqueness pin
across the whole batch, untouched-indices stay zero, coexistence
with `ar_register_main_sprites`).  Total **75 pass, 0 fail, 2
skip**.  32-bit cross build clean — pool capacity bump verified at
compile time.

---

## 2026-05-24 — Asset-Register: FUN_005749b0 (ar_register_main_sprites)

UI/menu sprite-register batch — the fourth call in `FUN_00562ea0`'s
asset-register sequence (after `FUN_00579bd0`, `FUN_00579a00`, and
the four `FUN_00563ef0` sound-bank loads).  Populates 34 sprite
slots: 9 inline registers, 1 special transient register (idx 0, id
0x90b loaded from sotesp.dll instead of the launcher settings
record), and 24 trailing single-call registers.  Most slots are
640×480 full-screen backgrounds and 368×276 UI panels; the
stragglers at indices 46/47/50/55 are small icons (32×32 / 64×64).

**The ECX-hidden mystery**: the C decomp shows one `FUN_005748c0`
call without an obvious `this` pointer — Ghidra dropped the thiscall
ECX setup.  radare2 disasm at 0x00574e0a reveals
`mov ecx, dword [0x8a7640]` — the slot at DAT_008a7640 (idx 0 in
our unified pool).  Same slot is also the target of the palette
ramp that follows.  So this one slot is BOTH register-populated
AND palette-decorated, while the inline slots and trailing calls
get only the register pass.

**Refactor**: `g_ar_sprite_slots` went from a 2-entry array
(FUN_00579bd0-specific) to a 64-entry unified pool indexed by
`(retail_BSS_addr - 0x008a7640) / 4`.  FUN_00579bd0's two
font-texture slots now live at `AR_SPR_FONT_TEX_457` (= 42) and
`_455` (= 43).  Existing tests updated mechanically.  Future
batches (FUN_0057a330, FUN_0056e190) plug into the same pool —
bumping `AR_SPRITE_SLOT_COUNT` as needed.

**Skipped**: the palette ramp section between the slot-5 and slot-9
inline writes — builds a 256-entry palette via the palette-session
trio (FUN_004178e0 / _005b5d90 / _00491770) and installs it onto
the idx-0 slot.  Documented in the driver docstring; will land
when the palette-session trio + PE-resource decoder do.

Tests: +6 (inline-slots field map, transient sotesp slot, trailing
IDs in index order, untouched indices stay zero, total slot count,
coexistence with `ar_register_fonts`).  Total 69 pass, 0 fail, 2
skip.  32-bit cross build clean.

---

## 2026-05-24 — Asset-Register: FUN_005748c0 (ar_sprite_slot_register)

Exposes the single-entry sprite-slot register as a public helper —
the same shape used by FUN_005749b0, FUN_0057a330, and the hundreds-
of-sprites mega-register FUN_0056e190.  Previously this lived as a
static helper (`ar_sprite_slot_register_init`) inside the module,
parametrized over `entry_count`.  All known retail callers pass
entry_count=1, so the public form hardcodes it — matches FUN_005748c0
exactly (operator_new(8) + 1-entry zero-loop + named field writes).

`ar_register_fonts` now calls the public helper instead.  Field-init
behaviour is unchanged; the test `register_fonts_sprite_slots` still
passes and pins the same slot state.

**Pivot vs handoff recommendation**: deferred the FUN_00563ef0 wave-
load second half.  Per-resource WAVE loading at boot is dead code
(boot batch passes load_flag=0 everywhere), and the dep chain pulls
in DSound vtable mocks + mmio + PE-resource — sizable test scaffolding
for code that doesn't run.  Instead picked the highest-leverage
building block on the title-menu critical path: the per-slot register
that the next three boot-driver calls (5749b0/57a330/56e190) all share.

Tests: +4 (full field-init map, destroy-on-reregister with aux_buf
+ multi-entry array, uint16 truncation, retail call-shape spot check
against FUN_0057a330's first arg list).  Total 63 pass, 0 fail, 2
skip.  32-bit cross build clean — `ar_sprite_slot` still 0x44 B.

---

## 2026-05-24 — Asset-Register: FUN_00579a00 (sound batch)

Second port in the asset-register batch — `FUN_00579a00` registers 12
sound-bank slots at DAT_008a6ec4..6ef0 ("W_MGR" pool).  Adds the
`ar_sound_slot` type (0x18 B; layout asserted) and the matching field-
init helper `ar_sound_slot_init` — which is also the first half of
`FUN_00563ef0` (the boot batch passes `load_flag = 0` so the wave-load
second half is dead code at boot).

The roster is a 12-entry table of (resource_id, count/kind) — 8 kind-2
slots, 4 kind-4 slots, hitting IDs in two ranges (0x506..0x510 and
0x4d8..0x4d9, plus one outlier at 0x903).  Eleven are written inline
in retail; the twelfth (table[8], id 0x50c) dispatches through
FUN_00563ef0 with load_flag=0 — disasm confirms the field-writes are
identical, so the port routes all 12 through the shared helper.

The `buffer` field at +0x04 is the lazy-load "already loaded?"
sentinel.  The init helper deliberately leaves it untouched; the test
`register_sounds_buffer_pointer_preserved` pins this so a future
refactor can't accidentally clobber it.

Tests: +4 (field-init, state clear, full 12-slot roster verification,
buffer preservation).  Total 59 pass, 0 fail, 2 skip.  32-bit cross
build clean.  Cumulative across the session: 13 functions ported into
the Asset-Register module across the FUN_00579bd0 and FUN_00579a00
boot batches.

---

## 2026-05-24 — Asset-Register module: FUN_00579bd0 family

Second ported module lands.  `FUN_00579bd0` is the first asset-register
batch call from the boot driver (after Pixel-Drawer set, before "The
resource was set" log line — see `docs/findings/asset-loader.md`).
Pulls in 11 functions total: the top-level batch, the 9 supporting
slot-setter / GDI-primitive helpers it calls, and the delete-array
thunk underneath everything.

**Module shape (`c4d2da0`)**: two struct types modelling the retail
in-memory slot layouts.  `ar_gdi_slot` (12 B) holds a fixed-capacity
HGDIOBJ array — the shape used by DAT_008a9274[idx] entries 1..15
in the boot batch.  `ar_sprite_slot` (0x44 B) is the sprite-slot
shape from FUN_0056e190 ("hundreds of similar blocks", per asset-
loader.md) — two are touched here (DAT_008a76e8 / _76ec for the font
texture slots).  Layouts asserted with `_Static_assert` blocks live
on the 32-bit cross build.

**Win32 isolation**: `asset_register.c` is pure logic.  The four GDI
primitive wrappers (`ar_gdi_create_font/pen/brush`, `ar_gdi_delete`)
are externs supplied by `asset_register_win32.c` (real build picks
it up via `src/Makefile` wildcard) or by the test harness (recording
stubs that log every call into a per-kind table).  Tests then assert
on call order + arguments without touching real GDI.

**Retail quirks preserved as comments rather than code**: the
`operator_new(4)` leak in `FUN_00579f40` (omitted, ASan-clean and
no observable effect); the asymmetry where `set_font` leaves
`count=0` but `set_pen`/`set_brush`/`set_pen_gradient` bump it;
the middle-loop bound in `FUN_00582d10` that makes gradient
capacities 0/1/2 skip the middle entirely.

**Ghidra recovery gap closed via radare2**: the bottom-block calls
`FUN_0057a030(4,8,0,group)` / a1a0 / a260 had their ECX setup
dropped from the C decompile.  Disasm at `0x579df8 / 0x579e05 /
0x579e1a` shows ECX = `[0x8a9298]` / `[0x8a92ac]` / `[0x8a92b0]`,
which decode to table indices 9, 14, 15.

**Tests**: +24 new (lerp arithmetic incl. descending channels and
alpha skip, GDI destruct order incl. NULL-hole handling, all 7
slot setters individually, `ar_register_fonts` end-to-end on sprite
+ GDI slot indices + call-order verification, layout parity).
Total: 55 pass, 0 fail, 2 skip (skips are the 32-bit-only layout
asserts; they fire at compile time on the cross build).  32-bit
cross build verifies layout parity, both `opensummoners.exe` and
`opensummoners-debug.exe` build clean.

Module is in isolation — not yet wired into the drop-in's boot
path.  Wiring waits until `FUN_00579a00` / `FUN_0057a330` /
`FUN_0056e190` land so calling it has a visible effect.

---

## 2026-05-24 — Test harness scaffold + first ported module (Pixel-Drawer)

Pivoted from "extract assets to spec the format" → "RE the init sequence
and reimplement methodically with tests" (the openrecet model).  Six
commits across one session.

**Harness fix (`8d6855c`)**: the `--max-frames` cap formula in
`frida_capture.py` was `msg_ticks * 250 >= max_frames`, which at the
default `max_frames=600` fired at just 3 emitted batches (~750
drained messages) and pre-empted any `--duration-ms` longer than
~12 s.  Now compares against the true running count carried on each
`msg` event; default bumped to 30000.

**Sprite format spec (`a2e5cb0`)**: archaeology pass on the
`sotesd.dll` DATA blobs spec'd the `0x425f` sprite family layout
(32 B outer magic + 1024 B BGRA palette + 64 B sub-table + 14 B
BMFH-style preamble + 8 B sub-sig + W×H 8bpp pixels).  213 of 759
DATA blobs parse cleanly.  W/H aren't in the file — they come from
`FUN_0056e190`-family registration calls.  Extractor at
`tools/extract/lizsoft_sprite.py`.  User redirected away from
chasing the asset extraction further: "we will load sprites the
same way as retail exe does it" once the init replay catches up.

**Title-scene runner (`07088a7`)**: `FUN_0056aea0` mapped fully.
8-phase intro animation (studio-logo fade → title fade → "press
button" → particle hand-off), pump+frame-budget cadence, the
default-branch menu-action latch via `FUN_0043ce50`, lazy DInput
pad attach on first menu-confirm.  Also extended
`winmin-and-bootstrap.md` with the state-code → next-scene map for
the outer driver's 0/6/8/9/0x1a..0x1e codes, and the Pixel-Drawer
slot-table allocation phase (69 slots in 5 fixed-size groups).
This is the first-rendered-frame bridge from boot done to actual
DDraw Flip.

**Test harness scaffold + Pixel-Drawer leaf primitives (`a53c141`)**:
`tests/{t.h, test_main.c, Makefile}` mirroring openrecet's pattern —
host gcc + ASan/UBSan, X-macro registry, `T_ASSERT_*` macros,
name-filter via `$F`.  Ported the 5 leaf primitives of `FUN_005bd*`:
mask→shift encoder, channel ctor, channel free-LUT, slot ctor, slot
SetColor.  13 tests; layout parity enforced via `_Static_assert`
blocks active on the 32-bit cross build.

**Pixel-Drawer LUT builder (`bb8c706`)**: `FUN_005bd040` (801 B,
four blend formulas + shared-LUT short-circuit).  Modes: 1=add,
2=sub, 3=lerp-variant-A, 4=channel-weight-coupled, default=lerp.
Floor-correction terms preserved literally even where they're
always-zero for valid weights, in case the engine ever passes
out-of-range inputs.  10 new tests with hand-computed expectations.

**Pixel-Drawer slot commit + mask reader (`aa0e62c`)**: `FUN_005bd3d0`
ties the leaf primitives together (free LUTs → resolve format from
PdFormat or RGB565 default → encode masks → resolve slot.state →
rebuild LUTs in R/G/B order with B sharing R-not-G).  The 8-byte
sub-detail that "B can share with R but never with G" preserved
verbatim from the retail call sequence at 5bd456/45f/468.
Pixel-Drawer module is now complete: 7 functions ported, 30 tests
passing on host (1 layout-test host-skip), 32-bit cross build clean.

Status: test harness is established and the first complete module
is in.  Next sessions should look at the remaining boot-driver
phases that have clear consumer relationships — likely the
ZDD wrapper (DDraw surface mgmt — Win32 heavy, needs mock layer or
just port + verify via the smoke harness) or the asset register
batch (`FUN_00579bd0` fonts, `FUN_0056e190` sprite slots et al —
consumes Pixel-Drawer slots so it integrates our work directly).
Skip the SS_MGR/W_MGR/GD_MGR boot pools until we know their
consumer semantics — they're just `operator_new` loops in
isolation.

---

## 2026-05-24 — Phase 1+2 push: audio, asset loader, config.dat, DDraw surface builder

Long unattended session.  Six commits, four new findings docs, two
new extractors.  Highlights:

**Audio + Input init (`docs/findings/audio-init.md`):** corrected the
prior mis-labeling of `FUN_005b9fc0` as "wave audio" — it's actually
the DInput keyboard sub-device, following `FUN_005b9cf0` (ZDI main /
`DirectInputCreateEx` with version `0x0700`).  DSound primary buffer
is created with `DSBCAPS_CTRLVOLUME` so the engine can master-attenuate
via `SetVolume`.  The launcher's "Disable Sound" gates ZDM (music
mgr) init only — DSound still inits either way.  ZDM allocates 50 ×
576-byte voice slots.  DInput is loaded by `Ordinal_1()` (= legacy
`DirectSoundCreate`), not by name — a quirk in `FUN_005bb180`.

**DDraw surface alloc (`docs/findings/ddraw-init.md`):** decompiled
`FUN_005b95c0` + `FUN_005b8c00` — the actual `IDirectDraw7::CreateSurface`
path.  Identified the engine's `0x01FFFFFF` "no color key" sentinel
(was previously mis-read as an "unlimited hint"), the 24bpp→32bpp
case fallthrough in the pixelformat switch, and corrected the
IDirectDrawSurface7 vtable cheat sheet (Lock at offset 0x64 not 0x60,
Unlock at 0x80 not 0x7c — 0x7c is SetPalette).

**Asset loader (`docs/findings/asset-loader.md` + `tools/extract/sotes_resources.py`):**
the three companion DLLs are pure resource-only PEs.  Wrote a
zero-dep PE-resource walker that dumps every entry to `type=<T>/<ID>.bin`
with a manifest.  Real content map:

- **sotesp.dll**: 31 WAVE SFX + signature blob
- **sotesw.dll**: 47 WMA music files (in `DATA` type, despite the
  earlier "MUSICWMA" speculation)
- **sotesd.dll**: 759 DATA blobs (~135 MB) + 436 WAVE SFX (~26 MB)

`FUN_005b6340`'s "kind 2 source" turns out to be a chunked-memory
abstraction (`FUN_005b67c0` spans 676996-byte chunks) — NOT
decompression as initially guessed.  This means sotesd 1000–1004
(each exactly 676996 bytes) is one logical 3.4 MB blob assembled
at boot.  Assets are stored RAW.  Sample DATA inspection shows
Lizsoft sprites have a 32-byte header + 256-entry BGRA palette
+ pixel data.

**Signature integrity checks (new engine-quirk §13):** all three
DLLs carry the same byte-encoded ASCII signature scheme.  Each
DLL has a resource that decodes (byte + `0x41`) to a known string:

| dll          | resource ID  | signature                                                    | min ver |
|--------------|--------------|--------------------------------------------------------------|---------|
| `sotesd.dll` | 0x7DE (2014) | `JFDGGIUABCVJIEKAUYLPOFDEQBVGSKOLJSCKPIFAXMHGYELSDOBFRKVGBAKB` | 0x2713 |
| `sotesw.dll` | 0x40F (1039) | `MUSICWMA`                                                   | 0x2712  |
| `sotesp.dll` | 0x407 (1031) | `FSPATCHR`                                                   | 0x2711  |

Our drop-in port can no-op these — they're integrity seals for the
ship-time DLLs, irrelevant when the user provides their own legit
copies.

**config.dat extractor (`tools/extract/config_dat.py` +
`docs/formats/config-dat.md`):** 16-byte plaintext header + 820-byte
XOR-obfuscated body (key `0x88`, confirmed by abundance of
`88 88 88 88` runs).  Body parses as one leading u32 + 102 `(u32,u32)`
pairs (`field_id`, `value`).  Field-ID semantics TBD but pattern is
clearly a typed key/value store matching the engine's `FUN_005afb90`
schema-registration with 101 fields.

**Harness turbo fixes (`tools/frida/opensummoners-agent.js`):**
GetTickCount virtualization (gated on first PeekMessage entry to
avoid pre-pump init livelock), WaitMessage stub (main-thread only),
Sleep → Sleep(0) (yield not noop, so background threads don't
starve), and PostMessage WM_ACTIVATEAPP(TRUE) to the main game
window as soon as the periodic scan finds it (without this the
pump spins on `DAT_008a952c == 0` forever because hidden windows
don't naturally receive the activation message).  Also corrected
the WndProc/class doc — `0x401210` is `CLASS_LIZSOFT_WAIT` (the
"Please wait." splash), the main game window is
`CLASS_LIZSOFT_SOTES` with WndProc `0x5b12e0`.

Engine quirks file grew from 8 entries to 14, with the most
load-bearing additions being §10 (WM_ACTIVATEAPP gating) and §13
(the three-DLL signature scheme).

Status: phase 1 surface mapping complete.  Phase 2 file-format
extraction started with config.dat and the resource walker.  Next
session is likely the Lizsoft sprite format spec + the chunked
sotesd 1000-1004 blob identification (needs DDraw Lock-hook capture
of a known sprite, then byte-diff against the extracted DATA bytes).

---

## 2026-05-24 — Harness turbo fixes + WndProc-class correction

Phase 1 surface mapping (the previous entry) flagged three TODOs that
this push addressed in `tools/frida/opensummoners-agent.js`:

1. **`GetTickCount` virtualization.**  Replaces `timeGetTime` as the
   simulation clock — Fortune Summoners never imports `timeGetTime`.
   The hook is gated by `g_pump_entered`: pre-pump init has busy-waits
   that livelock if the clock jumps 17 ms per call instead of advancing
   with real wall-clock.  After first `PeekMessageA` entry from main
   thread, the virtualization activates.
2. **`WaitMessage` stub** (main-thread only).  The pump uses
   `WaitMessage` to yield between frames; with virtual clock the OS
   timer never fires, so `WaitMessage` would hang.  Stub returns 1
   immediately on the main thread; background threads keep real OS
   semantics (audio mixer, file I/O may use `WaitMessage` for real
   waits).
3. **`Sleep` → `Sleep(0)`** instead of true no-op.  True-noop starves
   background threads of CPU, and the main thread often polls flags
   set by exactly such threads.  `Sleep(0)` yields the timeslice
   without actually sleeping — fast enough for turbo, correct enough
   for background work.

Discovered in the process: a hidden window never naturally gets
`WM_ACTIVATEAPP` from the OS, and `FUN_005b1030`'s spin loop only
breaks when `DAT_008a952c != 0` — which is set by the WndProc on
`WM_ACTIVATEAPP`.  Fix: agent posts `WM_ACTIVATEAPP(TRUE)` to the
main game window as soon as the periodic scan finds it
(`installPeriodicWindowScan`).  Without this, `msg_ticks` stayed at
0 forever with `--turbo --hide-window`; with it, pump enters and
the engine progresses into per-scene loops.

Also folded in a WndProc-class correction.  The Phase 1 notes claimed
the main game window's WndProc was `0x401210`; that's actually the
**`CLASS_LIZSOFT_WAIT`** ("Please wait." splash) WndProc.  The main
game window uses **`CLASS_LIZSOFT_SOTES`** with WndProc `0x5b12e0`
— a 441-byte handler that includes the load-bearing `WM_ACTIVATEAPP`
case plus `WM_CLOSE → ExitProcess(0)`.  Both classes are registered
in `FUN_005a4770`, sites `0x5a4ca8` and `0x5af314` respectively.
The `0x5b12e0` site does `mov dword [esp+0x50], 0x5b12e0` (lpfnWndProc
slot in WNDCLASSEXA at offset 8) — visible at `0x5af2c7`.

Quirks doc grew §9 (two WndProcs / two classes), §10 (WM_ACTIVATEAPP
as load-bearing pump-unlock), §11 (function-pointer-only callbacks
that Ghidra misses).  Engine-bootstrap doc updated to document both
WndProcs and the harness fix recipe.

Status: harness now reaches per-frame ticks in `--turbo --hide-window`
mode.  Steady-state frame rate still partial — `msg_ticks` reaches
~250 in some runs and 0 in others within a 30 s window, suggesting
init-phase race conditions remain (likely asset loading from
`sotesd.dll` / `sotesw.dll` — Phase 2 work).  Good enough to land as
a checkpoint; remaining bring-up TBD as the asset-loader RE goes.

---

## 2026-05-24 — Bootstrap (Phase 0)

Initial commit run.  Set up the project shape: nix flake with mingw-w64
i686 cross compiler + Ghidra + Frida-tools + Python (pillow/numpy/
sk-image/opencv/construct/rich/frida-python), `.editorconfig`,
`.gitignore`, MIT license, README.

`tools/setup.sh` — symlinks the user's Steam install of Fortune Summoners
into `vendor/original/`, detects Steam DRM by checking for a `.bind`
section in `sotes.exe`, runs Steamless via WSLInterop, and stashes the
unpacked binary in `vendor/unpacked/sotes.unpacked.exe`.  First run:
Steamless identified SteamStub Variant 2.1 and unpacked cleanly.
Original SHA: `7d779f2eb02b3c603857fedbc52be6973ac3b0b2c5c1bc696122ddac89fb9f1b`,
unpacked SHA: `9e032483b9981f73cabb83baca17a734fd9e7c41e114703900d9ee82c7969516`.

`tools/launcher/opensummoners-launcher.exe` — Job-Object supervisor copied
verbatim from OpenMare.  Guarantees no orphaned Windows-side .exes after a
SIGKILL'd WSL run.  Same `--timeout-ms` / `--grace-ms` / `--no-stdin-watch`
flags as the sibling.

`src/main.c` + `src/dev_hooks.c` — WinMain skeleton with the four
drop-in defaults the user asked for from day one:
  1. Auto-cd into `OPENSUMMONERS_GAME_DIR` + `SetDllDirectoryA` to the
     same, so any later `LoadLibrary` resolves game-dir DLLs first.
     `OPENSUMMONERS_GAME_DIR` is exported by the flake's shellHook with
     `WSLENV=…/p` so the .exe sees the Windows-form path.
  2. `user32!MessageBoxA/W` prologue patch that redirects every modal
     to stderr with a distinctive `[!!! REDIRECTED MESSAGEBOX !!!]`
     banner and auto-returns IDOK.  Override with `--show-msgbox`.
  3. Single-instance mutex (`OpenSummoners-SingleInstance`) catches
     stray .exes from previous SIGKILL'd runs.
  4. `--hide-window` / `--frames N` flags for harness/smoke runs.
Single-TU build (per the user's preference), two outputs:
`opensummoners.exe` (GUI subsystem) and `opensummoners-debug.exe`
(console subsystem, stderr surfaces in the launching shell).

`tools/ghidra-headless.sh` + `tools/ghidra-scripts/ExportDecompiledC.java`
— batch decompiles `vendor/unpacked/sotes.unpacked.exe` to
`docs/decompiled/` (gitignored).  Java post-script because nixpkgs'
Ghidra isn't built with PyGhidra.  First-run analysis kicked off in
background while we wrote the rest of Phase 0.

`tools/frida/opensummoners-agent.js` + `tools/frida_capture.py` — Phase A
Frida harness.  Hooks:
  - `MessageBoxA/W` → redirect to `send({kind:"messagebox",...})` +
    auto-IDOK (mirrors the dev_hooks.c hook on the drop-in side).
  - `ShowWindow` / `ShowWindowAsync` / `SetWindowPos(SWP_SHOWWINDOW)`
    → force hidden for HWNDs we tracked from CreateWindowEx returns.
  - `PeekMessage*` / `GetMessage*` onLeave → tick a coarse frame counter.
  - `Sleep` → no-op (turbo).
  - `winmm!timeGetTime` → virtualised clock for the main thread only
    (turbo simulation speedup, not just loop-iteration speedup).
  - `waveOutSetVolume` → clamp 0 (silent audio).  DSound layer deferred
    to Phase B once Ghidra confirms the engine's COM init path.
All flags default ON per the user's instruction ("hidden window with
muted audio running in turbo mode as early as possible").

`tools/run-opensummoners.sh` + `tools/run-retail.sh` — single-source-of-
truth dev-loop recipes so the build / run / launcher / harness flags are
consistent every time.  No re-discovering gotchas per session.

Smoke verification:
  - `run-opensummoners.sh` end-to-end: launcher → debug.exe
    `--hide-window --frames 200` runs in ~3.2 s (16 ms × 200), MessageBox
    hooks both succeed (`@ 745d6e60` / `@ 745d7380`), init_game_dir cd's
    into the Windows-form game path, exit rc=0.
  - Retail smoke under Frida: green.  The frida-server.exe runs on the
    Windows host as `cutestation.soy:27042` (the host's LAN-resolvable
    name; WSL2 NAT doesn't loop back to 127.0.0.1).  Updated the flake's
    default + `frida_capture.py` to match — 127.0.0.1 was the wrong
    default the OpenMare sibling carried forward.

Discoveries (folded into agent code and findings docs as we hit them):
  - **sotes.exe is SteamStub Variant 2.1 packed.**  Spawning the
    on-disk exe outside the Steam process tree trips the DRM check
    (`Steam Error: Application load error P:0000065432`).  Fix:
    `tools/run-retail.sh` copies vendor/unpacked/sotes.unpacked.exe into
    the game dir as `sotes-unpacked-<pid>.exe` per run (needed alongside
    the engine DLLs so Windows DLL search finds sotesp/d/w).
  - **Frida 17.x API surface differs.**  `Module.findExportByName(modName,
    exp)` static method removed → use
    `Process.findModuleByName(name).findExportByName(exp)`.
    `Memory.readUtf8String(ptr)` removed → use `ptr.readUtf8String()`.
    Hooks attached while the process is suspended sit deferred until
    `Interceptor.flush()` — without that, all our installs no-op'd
    silently.  Use `Process.mainModule` instead of name-matching since
    the spawned exe is named per the temp filename.
  - **The engine launcher is a Win32 #32770 modal dialog**, NOT a
    `MessageBox`.  Created by `DialogBoxParamA(hInst, 0x2711, NULL,
    dlgProc=0x004013c0, 0)`.  The dialog manager bypasses public
    `CreateWindowEx` / `ShowWindow` / `SetWindowPos` exports.  We
    initially caught it via a periodic `EnumWindows` scan + force-hide,
    but the OS painted it before our 8 ms scan tick — a brief flash
    appeared on the user's desktop.

Final fix (silent boot achieved 2026-05-24):
  `installDialogBypass()` in `tools/frida/opensummoners-agent.js`
  hooks `DialogBoxParamA` and replaces the engine's DLGPROC (arg 3)
  with a Frida `NativeCallback` wrapper.  On `WM_INITDIALOG`:
    1. Call original handler (loads saved settings into controls).
    2. Force-check Windowed Mode (ctrlId 10020) + Disable Sound
       (ctrlId 10024).
    3. `SendMessage(LaunchBtn, BM_CLICK)` synchronously — the engine's
       IDOK handler reads control state, persists, calls EndDialog.
    4. Return original result.
  Because `EndDialog` has been called before `WM_INITDIALOG` returns,
  the dialog manager skips its post-INITDIALOG ShowWindow step.  User
  confirmed "absolutely nothing" on screen.

Status of the harness:
  - Spawn retail headlessly under Frida → init agent → resume → engine
    boots silently through its launcher → reaches the main game window
    (`CLASS_LIZSOFT_SOTES`) within a few seconds → harness teardown
    via `device.kill(pid)`.
  - msg_ticks stays at 0 in the smoke summary — the engine reaches
    main window creation but doesn't enter its PeekMessage loop in 8 s.
    Probable additional bring-up phases (DirectDraw surface alloc,
    asset loader) gate the main loop; revisit when tracing the boot
    chain.

Ghidra batch decompile finished: 1768 functions written to
`docs/decompiled/` (gitignored).  First useful query already paid off
— `grep DialogBoxParam` immediately pointed at the dialog call site
and DLGPROC address.

Next session — Phase 1 priorities:
  1. Read DLGPROC at `0x004013c0` and its caller to understand
     `gl.cfg` (or wherever settings persist) layout.  This is the
     first thing the engine writes; spec it and we have an extractor.
  2. Find and document `WinMain` + the main loop + frame limiter
     (mirror OpenMare's `winmain-and-bootstrap.md`).
  3. Identify the DirectDraw 7 init path (`DirectDrawCreateEx` →
     `IDirectDraw7::SetCooperativeLevel` → primary surface alloc).

---

## 2026-05-24 — Phase 1 surface mapping (#1)

Three findings docs landed in one session, covering the three
priorities the prior entry queued up.  All entries cross-link, and
`engine-quirks.md` grew four new items folded in along the way.

`docs/findings/launcher-dialog.md` — full reverse of the launcher
DLGPROC at **`0x004013c0`** plus its sibling helper `FUN_00401730`.
Ghidra missed both because they're only reached via function
pointers; disassembled with `radare2 -c 'af; pdf'`.  The proc handles
just `WM_INITDIALOG` and `WM_COMMAND`; click on Launch (`ctrlID 10003`)
sets `DAT_008a9a40 = 1` and scrapes the four radio/checkbox groups
into `DAT_008a9b48/4a/4c/4e` (screen mode / VRAM / quality / disable
sound).  Engine quirk: **radio enums start at 3, not 0** — saved
file values are 3/4/5 per group.  Engine quirk: control `0x272A`
(Zoom 1920×1440) is unconditionally `ShowWindow(SW_HIDE)`'d at
`WM_INITDIALOG` — exists in the dialog resource but the user never
sees it.  Engine quirk: three controls (`0x271C-0x271E`) are
`EnableWindow(false)`'d on every init with no path to re-enable.

`vendor/original/user/config.dat` (840 bytes) is XOR-obfuscated with a
**16-byte plaintext header** (`hdr=16`, `ver=0x2711` matching the
dialog resource id, `data_size=820`, checksum) followed by 824
obfuscated bytes.  Key byte `0x88` — confirmed by the dead-obvious
runs of `88 88 88 88` (zero plaintext).  Format spec deferred to
Phase 2 `docs/formats/config-dat.md` once we wire the extractor.

`docs/findings/winmain-and-bootstrap.md` — full call graph from
`entry @ 0x5c0a8f` through `WinMain @ 0x562210` and the post-launch
driver `FUN_00562ea0`.  Mapped:
  - **WndProc @ 0x401210** (missed by Ghidra — pointer-only ref).
    Only handles `WM_PAINT` (loading-screen text + frame blit);
    everything else delegates to `DefWindowProcA`.  No `WM_CLOSE`
    handler — click-X just destroys the window without `WM_QUIT`,
    hanging the process.
  - **Message pump + frame limiter at `FUN_005b1030`**:
    `PeekMessageA` → if `WM_QUIT` (0x12) → `ExitProcess(0)`;
    `WaitMessage` to block on a `SetTimer(hWnd, 1, 10ms, NULL)`
    that's installed in `FUN_00562ea0`.  Frame-readiness flag at
    `state->[0x1c]` is set when `GetTickCount - last_tick < 5` ms.
  - **Class registration**: `RegisterClassExA` inside the 46 KB
    `FUN_005a4770` at `0x5a4ca8` — `CLASS_LIZSOFT_SOTES`, style
    `CS_HREDRAW|CS_VREDRAW`, WndProc `0x401210`, default arrow cursor.
  - **No global main loop** — each scene function runs its own
    pump+tick loop until it returns a state code to the outer scene
    state-machine in `FUN_00562ea0`.  Scene code = 9 means
    "restart game", caught by WinMain's `do…while`.

Critical insight for the Frida harness: **the engine uses
`GetTickCount` exclusively** — `iiq~timeGetTime` on the unpacked
binary returns nothing; the timeGetTime hook our agent inherited
from openrecet/OpenMare is a no-op here.  We need to add
`GetTickCount` virtualization + a `WaitMessage` stub to actually
achieve turbo speed.  TODO in the agent.

`docs/findings/ddraw-init.md` — DirectDraw 7 init flow:
`FUN_005b7ee0` (ZDD wrapper ctor)  →  `FUN_005b88c0`
(`DirectDrawCreateEx(NULL, &ddraw7, &IID_IDirectDraw7, NULL)` —
IID at `DAT_00850eb0`) → `FUN_005b89d0` (`SetCooperativeLevel`
with `DDSCL_EXCLUSIVE|FULLSCREEN|ALLOWREBOOT = 0x13` in fullscreen
or `DDSCL_NORMAL = 8` windowed) → `FUN_00582e90` (CreateScreen
mode dispatch — calls `FUN_005b8b40` which builds DDSURFACEDESC2
+ `IDirectDraw7::CreateSurface`) → `FUN_005b9520` (clipper create
+ attach to primary surface).  Catalogued the vtable offsets for
`IDirectDraw7` / `IDirectDrawSurface7` / `IDirectDrawClipper` so
the Phase-A `Lock`/`Flip`/`Blt` hooks land at the right offsets.

Two follow-ups recorded in the new docs for the next push:
  - **Decompile `FUN_005b95c0`** (the DDSURFACEDESC2 builder) when
    we move on to the renderer port — easier than chasing the
    46 KB `FUN_005a4770`.
  - **Add `GetTickCount` + `WaitMessage` hooks** to
    `tools/frida/opensummoners-agent.js` so turbo actually works.

Suggest `/clear` before the next subsystem (likely audio/DSound,
the asset loader, or the renderer port).  The Ghidra reads in this
session pulled in a lot of context that the next milestone won't
need.

---
