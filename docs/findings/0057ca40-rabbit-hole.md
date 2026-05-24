# FUN_0057ca40 — the group-3 sprite batch is multi-subsystem

**Status:** partial port landed 2026-05-24, then extended later that day
with the FUN_00582b80 + FUN_00582d00 thiscalls (see section 4).  Full
body still NOT ported — the parallel-info-table indexing, SS_MGR
slot-clones, and FUN_00582b80 + ar_info_entry_clear *call clusters*
are the three remaining gaps.

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

### 2. ~380 parallel-table writes — **DEFERRED**

The function writes to a parallel pool at retail BSS
`0x008a8578..0x008a8b14`:

| prefix    | writes | offsets       | values  | meaning                  |
| --------- | ------ | ------------- | ------- | ------------------------ |
| 0x008a85xx | 15     | +4            | 1       | extends `g_ar_sprite_flags` past the 14-entry boundary the previous batch reached |
| 0x008a86xx | 39     | +4, +8        | 1, 2    | flag-style |
| 0x008a87xx | 34     | +4            | 2       | always-2 flag |
| 0x008a88xx | 22     | +4            | 2       | always-2 flag |
| 0x008a89xx | 45     | +4, +8        | 1, 2    | mixed |
| 0x008a8axx | 35     | +4, +8        | 1, 2    | mixed |
| 0x008a8bxx | 4      | +4            | 2       | tail |

Plus 98 const-data-pointer writes at offset +8 (or +16, depending on
the entry width — needs disasm to confirm) pointing into the PE rdata
section (`&DAT_006748d0`, `&DAT_00674ad8`, `&DAT_006752f8`, etc.).

Plus 88 prologue-style `*DAT = N` writes at the same addresses (the
"+0 = 0 prologue" pattern we first saw in `ar_register_palette_ramps`
at flag idx 19).

Modeling this needs:

- Extending `g_ar_sprite_flags[]` from a flat 14-entry uint32 array
  to a ~357-entry pointer-to-struct array (matching retail's actual
  shape — see HANDOFF note "Each retail entry is itself a POINTER to
  some still-unidentified struct").
- Identifying the struct's full field layout (current observed:
  +0 u16, +4 u32, +8 ptr — at least 12 bytes).
- Identifying the per-prefix semantics (the values 1 vs 2 must mean
  *something*).

No consumer of these writes is ported yet, so the deferral is
behaviourally invisible to downstream code today.

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
**`ar_info_entry` (16 bytes)** — the parallel-info-table entry the
HANDOFF previously called out as "each retail entry is itself a
POINTER to a struct."  Disasm at 0x57fa98..0x57fa9e:

```
mov  ecx, [0x008a8a40]     ; load entry pointer from parallel table
call FUN_00582d00          ; zero 14 bytes of *entry
```

The clear writes `word@+0` then `dword@+4/+8/+12` (note: `ax` not
`eax` at +0 — the pad bytes at +2..+3 stay untouched).  The follow-up
chain at 0x57faa3..0x57facb copies a u16 from `[0x008a8a34]+0` and a
u32 from `[0x008a8a34]+4` into the cleared entry, then writes a
const PE rdata pointer (e.g. `&DAT_006752f8`) at `entry + 8`.

That pins the **ar_info_entry layout**:
  - +0x00 (u16): marker — values 1/2 in the prefix-table writes; the
    `entry->marker` field the prior section's "+4: 1/2 flag" hand-wave
    was actually +0 (the +4 writes are this field's `flag` neighbour).
  - +0x02 (u16): pad — never touched.
  - +0x04 (u32): flag — 0/1/2 in observed copies.
  - +0x08 (void*): const PE rdata pointer (e.g. &DAT_006752f8,
    &DAT_006748d0, &DAT_00674ad8).
  - +0x0c (u32): cleared; semantics unknown.

The struct is now defined in `src/asset_register.h` (search
`ar_info_entry`); both functions are ported and unit-tested.  The
ports stand alone — there is no caller of FUN_0057ca40 in the
drop-in yet, so the new functions sit idle awaiting the SS_MGR +
parallel-info-table-array landing.  Tag rows added to
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
