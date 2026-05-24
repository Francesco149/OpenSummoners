# OpenSummoners — Progress log

Append-only changelog.  Newest entries first.  Each entry: date + heading,
then 1–3 short paragraphs.  Cross-link to `docs/findings/*.md` and
specific commits where relevant.

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
