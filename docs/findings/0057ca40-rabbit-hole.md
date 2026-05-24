# FUN_0057ca40 — the group-3 sprite batch is multi-subsystem

**Status:** partial port landed 2026-05-24, extended later that day
with the FUN_00582b80 + FUN_00582d00 thiscalls (see section 4), then
extended again on 2026-05-24 with the **info_entry pool allocator
finding** (see section 5) which corrects the entry size from 16 →
20 bytes and pins the pool capacity at exactly 909 entries.  Full
body still NOT ported — the per-call-site indexing inside
FUN_0057ca40, the SS_MGR slot-clones, and the FUN_00582b80 +
ar_info_entry_clear *call clusters* are the three remaining gaps.

## What HANDOFF said

The HANDOFF flagged FUN_0057ca40 as Ghidra-fails (24884 B body, decompile
response buffer exceeded) and recommended attacking it via Frida watch-
points or chunked radare2 disasm.  The expectation was "register N
sprites boilerplate."

## What we actually found

Ghidra DOES decompile the function — the typed-struct workflow that
landed with `ar_register_palette_ramps` (the previous checkpoint)
resolved the buffer issue.  3124-line decomp is at
`docs/decompiled/by-address/57ca40.c`.

But the body is not just "register N sprites."  It contains FOUR
distinct subsystems:

### 1. 233 sprite-slot registers — **PORTED**

- 91 **inlined** blocks (`paVar1 = DAT_xxxx; FUN_00417b50(...); zdd=; ...`)
- 142 **helper-style** calls (`ar_sprite_slot::FUN_005748c0(...)`)
- Slot indices span 79..423 in `g_ar_sprite_slots[]`.
- All entries use uniform `(zdd, settings, group)` from the caller.
- Order between registers is observably irrelevant.

This is what `ar_register_group3_sprites` ports.  Table-driven; the
generator that extracted the table is `tools/extract-57ca40.py`.

### 2. ~380 ar_info_entry-pool writes — **POOL MODELED, INDEXING CONFIRMED 2026-05-24**

**Update 2026-05-24 (allocator):** the "parallel pool" was elevated
from a 14-entry flat-uint32 model to a full 909-entry `ar_info_entry`
pool — see section 5 below for the allocator finding.

**Update 2026-05-24 (per-call-site indexing — CONFIRMED):** walked
all 466 info-entry references inside FUN_0057ca40 and matched them
to slot decls / clone targets in the same function.  The result:
**pool[i] one-to-one shadows slot[i]** across all 909 entries.  The
per-call-site mapping is implicit — every info-entry write at retail
BSS address `0x8a8440 + i*4` corresponds to the slot at retail BSS
address `0x8a760c + i*4`.  Evidence (from `tools/extract/57ca40_pool_map.py`):

- 243 slot decls (inline + helper) in the function.
- 229 of those have ≥1 info-entry write in the cluster between
  this slot decl and the next.
- 226 of the 229 (98.7%) have their FIRST info-entry write land
  at pool[slot_idx] — i.e., the slot's own shadow entry.
- The 3 "deviations" are not real: they're clusters where the
  FUN_00582b80 inline-clone primitive runs BEFORE the cluster's
  own info-write, and its clone-target's info-entry is touched
  first.  Once those clone targets are counted as slot
  declarations (the clone IS the decl, since it produces a new
  populated slot), 0 / 466 info-entry writes are orphaned.
- 27 slot decls write NO info-entry — these slots leave their
  shadow entry in the allocator-zeroed state.  No anomaly.
- Distinct slot indices produced (decl + SS_MGR clone target +
  inline-clone target): 346, range 92..437.
- Distinct pool indices touched: 319, range 92..437.  All 319
  are within the 346 produced slot indices.

The natural port model is therefore:

  pool_idx == slot_idx,  for both byte-direct and SS_MGR-table-mediated
  access (since `input_mgr->info_entry_table[i]` at +0x18e0 + i*4
  IS the same pointer as the BSS-address-per-write retail uses).

**Ghidra rendering gotcha (NEW finding):** different `DAT_008a8XXX`
variables get DIFFERENT C types in Ghidra — some are byte-typed
(source-level offset = byte offset), others are short-typed
(source-level offset = byte offset / 2).  Disasm confirms:

  | Ghidra source                              | DAT type | byte offset | field          |
  | ------------------------------------------ | -------- | ----------- | -------------- |
  | `*(undefined4*)(DAT_008a85b0 + 4) = 1`     | byte*    | +4          | flag           |
  | `*(undefined4*)(DAT_008a85c4 + 2) = 1`     | short*   | +4          | flag (same!)   |
  | `*DAT_X = 0x1c`                            | (either) | +0          | marker (u16)   |
  | `*(undefined**)(DAT_X + 8) = &DAT_006752f8`| byte*    | +8          | data ptr       |
  | `*(undefined**)(DAT_X + 4) = &DAT_006752f8`| short*   | +8          | data ptr (same)|

So the source-level offsets in the prefix table above (`+4`, `+8`) are
NOT meaningful — what matters is the disasm byte offset, which is
always +4 (flag) or +8 (data) regardless of Ghidra rendering.  The
~115 source `+2` writes the previous version of this section called
"info_pad2" are actually flag writes at byte offset +4 (i.e. pad
+2..+3 is genuinely never touched).

Final event-kind breakdown across all clusters (after the offset
correction):

  - 138 marker writes (`*DAT = N`)  — byte offset +0
  - 194 flag writes (Ghidra source `+4` for byte* or `+2` for short*)
    — byte offset +4
  - 94 data-ptr writes (Ghidra source `+8` for byte* or `+4` for
    short*) — byte offset +8
  - 94 SS_MGR clone calls (`FUN_004179b0(dst, src)`)
  - 9 inline-clone calls (`FUN_00582b80(target_slot)`)
  - 4 clear-entry calls (`FUN_00582d00()`)
  - 4 marker-copy-from-template + 4 flag-copy-from-template (the
    FUN_00582d00 follow-up that populates the cleared entry from
    a template entry)

**DONE 2026-05-24:**

- `g_ar_sprite_flags[]` (flat 14-entry uint32) replaced by
  `g_ar_info_entries[909]` + `g_ar_info_table[909]` — modelled
  one-to-one against retail's pool at 0x8a8440..0x8a9270 (sized by
  the allocator in §5).
- Per-entry struct layout extended to 20 bytes: marker u16@0,
  flag u32@4, data (const void *)@8, palette (void *)@0xc,
  f_10 u32@0x10.  Reads from FUN_00586010 confirm both `+0xc` as a
  256-entry palette pointer and `+4` as a 0/1/2/3 dispatch value.
- Per-call-site indexing confirmed — pool[i] shadows slot[i].
- Ghidra rendering gotcha (byte-typed vs short-typed DAT) characterised.

**Still deferred:**

- The port itself.  The mapping is clean now (write at pool[slot_idx]),
  but no consumer of these writes is ported yet, so the deferral is
  behaviourally invisible to downstream code today.  Port becomes
  natural once the first FUN_00586010-style consumer (palette draw
  with flag-dispatch) is in scope.
- Per-prefix semantics (values 1 vs 2 in the flag).  FUN_00586010's
  switch on `flag == 0/1/2/3` is the strongest evidence so far that
  these are functional state values rather than just "live" markers.
  Mapping each pool-index range to its flag's meaning is open work.

### 3. 94 FUN_004179b0 calls — **DEFERRED**

`FUN_004179b0(dst_idx, src_idx)` is a `__thiscall` on the SS_MGR
singleton.  It clones one slot's metadata (`zdd`, `settings`, resource
id, dims, etc.) into another slot, plus copies a 24-byte aux buffer
and a parallel-table entry at `+0x18e0 + idx*4`.

54 distinct source indices, each with 1-5 clone targets — looks like
a "sprite-frame-variant" expansion (each "source" sprite has N
visual variants that share metadata but get separate slot identities).

Modeling needs the SS_MGR struct (which we haven't mapped beyond
"the singleton at DAT_008a8440 lives in the SS_MGR boot-pool, see
HANDOFF").

### 4. 9 FUN_00582b80 + 1 FUN_00582d00 — **PORTED 2026-05-24** (subset)

`FUN_00582b80(target_slot)` is a `__thiscall` on the source slot — it
clones source metadata into `target_slot` (zdd, width, height,
colorkey, scale_flag, type, settings, resource_id, group), allocates
a fresh 1-entry `entries[]` on the target, and deep-copies the
source's `aux_buf` (24-byte stride entries, count from src->f_38).
Slightly different shape from FUN_004179b0 (no SS_MGR table
indirection).

`FUN_00582d00()` is a `__thiscall` on a NEW struct shape:
**`ar_info_entry` (20 bytes — see §5 for the size correction)** —
the pool entry the HANDOFF previously called out as "each retail
entry is itself a POINTER to a struct."  Disasm at 0x57fa98..0x57fa9e:

```
mov  ecx, [0x008a8a40]     ; load entry pointer from the pool
call FUN_00582d00          ; zero 14 bytes of *entry
```

The clear writes `word@+0` then `dword@+4/+8/+12` (note: `ax` not
`eax` at +0 — the pad bytes at +2..+3 stay untouched).  The follow-up
chain at 0x57faa3..0x57facb copies a u16 from `[0x008a8a34]+0` and a
u32 from `[0x008a8a34]+4` into the cleared entry, then writes a
const PE rdata pointer (e.g. `&DAT_006752f8`) at `entry + 8`.

That pins the **ar_info_entry layout** (with subsequent corrections
folded in from §5 and §6):
  - +0x00 (u16): marker — values 1/2 in the prefix-table writes; the
    `entry->marker` field the prior section's "+4: 1/2 flag" hand-wave
    was actually +0 (the +4 writes are this field's `flag` neighbour).
  - +0x02 (u16): pad — never touched.
  - +0x04 (u32): flag — 0/1/2/3 in observed reads (see §6).
  - +0x08 (void*): const PE rdata pointer (e.g. &DAT_006752f8,
    &DAT_006748d0, &DAT_00674ad8).
  - +0x0c (void*): **palette buffer pointer** — 256-entry, 1024-byte
    palette read by FUN_00586010 (see §6).  Was "f_0c / semantics
    unknown" before.
  - +0x10 (u32): zeroed by the allocator (§5); not touched by the
    14-byte clear; semantics unknown.

The struct is defined in `src/asset_register.h` (search `ar_info_entry`);
both functions are ported and unit-tested.  Tag rows added to
`tools/ghidra-scripts/TagThiscallFunctions.java` (26 tags now).

**Clone-and-detach pair**: at every FUN_00582b80 + FUN_00582d00
cluster in FUN_0057ca40, the inline-template slot (in ECX before
the call) gets cloned to the target sprite slot, and then the
parallel-table entry that "shadows" the target slot gets zeroed and
re-populated from a template parallel entry plus a const data
pointer.  The template slot ECX is dropped (its fields read; its
fresh `entries` alloc not re-used by the target — the target
allocs its own).

The tail of FUN_0057ca40 also has 5×20-byte memcpy loops into a
`+0xae0`-base region — another parallel table we haven't named.

### 5. Pool allocator — **FUN_00562ea0:225-253** (found 2026-05-24)

The "where do the pool slot pointers come from" question that the
2026-05-24 port left open is answered by a single loop in the boot
driver, run BEFORE the "SS_MGR_Preparation" log:

```c
puVar15 = &DAT_008a8440;
iVar11  = 0x38d;                /* 909 iterations */
do {
    puVar3 = operator_new(0x44);     /* 68 B sprite slot */
    if (puVar3 == NULL) puVar3 = NULL;
    else {
        puVar3[0] = 0;               /* +0x00 */
        *(short *)(puVar3 + 1) = 0;  /* +0x04 (u16) */
        puVar3[7]  = 0;              /* +0x1c */
        puVar3[13] = 0;              /* +0x34 */
    }
    puVar15[-0x38d] = puVar3;        /* store in sprite-slot table */

    puVar4 = operator_new(0x14);     /* 20 B info_entry */
    if (puVar4 == NULL) puVar4 = NULL;
    else {
        *puVar4                = 0;  /* +0x00 (u16) */
        *(int *)(puVar4 + 2)   = 0;  /* +0x04 */
        *(int *)(puVar4 + 4)   = 0;  /* +0x08 */
        *(int *)(puVar4 + 6)   = 0;  /* +0x0c */
        *(int *)(puVar4 + 8)   = 0;  /* +0x10 */
    }
    *puVar15 = puVar4;               /* store in info-entry table */

    puVar15++;
    iVar11--;
} while (iVar11 != 0);
```

That one loop runs TWO parallel pools side-by-side, allocating one
slot and one info entry per index for 909 iterations:

| pool          | BSS range            | stride | entry size | retail var |
| ------------- | -------------------- | ------ | ---------- | ---------- |
| sprite-slot   | 0x8a760c..0x8a8440   | 4      | 0x44 (68)  | (none)     |
| info-entry    | 0x8a8440..0x8a9270   | 4      | 0x14 (20)  | (none)     |

The sprite-slot pool's BSS range is the same one the
`g_ar_sprite_ramp_slots` (idx 1..12) and `g_ar_sprite_slots` (idx 13..)
viewers already model, just unified into one 909-entry table whose
first entry (0x8a760c) we hadn't accounted for and whose tail
extends further than the previous AR_SPRITE_SLOT_COUNT=1024 over-
estimate (the real count is 909, capped exactly here).

The info-entry pool's BSS range ends at 0x8a9270, which is the start
of the W_MGR pool (the 16-entry GDI slot pool at &DAT_008a9274 from
section "W_MGR_Preparation").  The 4-byte gap is unexplained but
probably alignment / a single sentinel slot.

**Consequence for ar_info_entry:** size is 20 bytes (not 16), with a
`+0x10` field zeroed by the allocator but NOT touched by
`ar_info_entry_clear` (which clears 14 bytes from +0x00).  The host
model in `g_ar_info_entries[909]` + `g_ar_info_table[909]` (added
this checkpoint) matches retail's shape one-to-one.

### 7. SS_MGR singleton == WndProc `input_mgr` (both at 0x008a6b60)

The two pool tables the allocator (§5) fills are NOT standalone globals
in retail — they're fields of the same struct that the WndProc port
already models as `input_mgr` (see `src/wnd_proc.h`).  The FUN_004179b0
disasm proves this:

```
0x0058040c   mov  ecx, 0x8a6b60        ; load singleton 'this'
0x00580411   call fcn.004179b0          ; __thiscall slot-clone
```

And FUN_004179b0 indexes its tables off ECX as:

```c
*(int **)(in_ECX + 0x0aac + idx*4)   /* sprite slot ptr table */
*(int **)(in_ECX + 0x18e0 + idx*4)   /* info-entry ptr table  */
```

Both offsets land within the input_mgr's 0x2884-byte opaque head:

| this + ofs | absolute     | meaning                                     |
| ---------- | ------------ | ------------------------------------------- |
| +0x0aac    | 0x008a760c   | sprite-slot pointer table (909 × 4 B)       |
| +0x18e0    | 0x008a8440   | info-entry pointer table (909 × 4 B)        |
| +0x2884    | 0x008a93e4   | zdm pointer (input_mgr::zdm_ptr — modelled) |

So one struct owns the sprite slot pool, the info-entry pool, and the
zdm pointer.  Naming-wise: "input_mgr" matches the WndProc role,
"SS_MGR" matches the asset-register role.  We keep both names in the
codebase since each is used in the relevant context; the underlying
struct is the same.

Implication for porting FUN_004179b0 et al: any "SS_MGR singleton"
port needs to plumb the unified pool index (not the main-pool or
ramp-pool index our standalone globals use).  Pool index i ↔
ramp_slots[i-1] for i=1..12, ↔ main_slots[i-13] for i=13..908, and
i=0 is a sentinel slot at 0x008a760c with no observed consumer.

### 6. Reader/writer consumers — **FUN_00586010, FUN_00587e00**

Two functions outside FUN_0057ca40 touch info-entries by absolute
BSS address; they tell us what the fields mean downstream:

**FUN_00586010** — a 1035-line draw routine that picks a *single*
info entry (`DAT_008a866c` = pool index 61) and uses three of its
fields to drive palette rendering:

```c
if (*(int *)(DAT_008a866c + 4) == 3)     { /* solid-color dispatch */ }
palette = *(int ***)(DAT_008a866c + 0xc);
if (palette == NULL) {
    ar_sprite_slot::FUN_004178e0(DAT_008a7838, &local_91c);
    if (*(int *)(DAT_008a866c + 8) != 0)
        FUN_00417bc0(*(undefined4 *)(DAT_008a866c + 8), &local_91c);
    palette = &local_91c;
}
if (*(int *)(DAT_008a866c + 4) == 1) { /* lerp dispatch */ }
else if (*(int *)(DAT_008a866c + 4) == 2) { /* desaturate dispatch */ }
```

So `+0x04 (flag)` is a 0/1/2/3 dispatch value, `+0x08 (data)` is a
secondary palette-modifier source, and `+0x0c (palette)` is a
pre-computed 256-entry palette buffer that, when non-NULL, overrides
the slot's own palette.

**FUN_00587e00** — a 3282-line "refresh" routine that *writes*
`entry->data` (pool pointer + 8) for many indices.  It iterates
multiple entry ranges (0x8a85d0..0x8a85e8, 0x8a8608..0x8a8638, ...)
and sets `*(undefined **)(DAT + 8) = puVarX` if the current value
differs from `puVarX`.  Looks like a runtime swap-in of new data
buffers — possibly the "scene reload" path that picks a different
const PE rdata pointer when the locale or area changes.

Neither function is ported.  Their evidence is what justified the
size + palette-field corrections in §5 and §6 — pinning the field
semantics from independent read sites confirms the disasm-level
heuristics from the clear/copy chain.

## Why partial-port

A full port of this function would require porting all four subsystems
at once — every subsystem touches state no other ported batch has
needed, so the scaffolding doesn't exist.  Doing them in one commit
would be a multi-thousand-line change with a lot of speculative
struct modeling.

The slot-register subset (233 ops) is the well-understood part: each
write is independent of the others, the helper (`ar_sprite_slot_register`)
already exists, and the observable end-state on `g_ar_sprite_slots[]`
is exactly what the existing pixel-drawer consumers read.

The deferred subsystems write to state that NO ported consumer reads
yet.  Downstream impact: zero.  Future work: each deferred subsystem
becomes its own checkpoint once we have a consumer that demands it
(or once the scaffolding for SS_MGR / parallel-info-table modeling
lands for another reason).

## Cross-references

- `tools/extract-57ca40.py` — generator for the table in
  `asset_register.c` (re-run if the decomp ever changes).
- `docs/decompiled/by-address/57ca40.c` — full Ghidra decomp (3124 lines).
- `src/asset_register.c` — `ar_register_group3_sprites` and the
  233-entry `group3_sprites[]` table.
- `src/asset_register.h` — full docstring on the partial-port scope.
- HANDOFF.md "Open RE threads" — entries on g_ar_sprite_flags
  parallel-pool and SS_MGR boot-pool that this finding renames as
  blockers for the deferred subsystems.
