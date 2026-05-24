# Asset loader — `sotesd.dll` / `sotesw.dll` / `sotesp.dll` + PE resources

How the engine pulls game-asset bytes out of the three companion DLLs
plus its own embedded `.rsrc` section.

## Companion-DLL layout

All three are **resource-only DLLs** — tiny `.text` (~11 KB DllMain
stub) and `.data`, plus a massive `.rsrc` section holding the assets:

| dll          | `.rsrc` size | role                                 |
|--------------|--------------|--------------------------------------|
| `sotesd.dll` | 168 MB       | "D" group — sprite/scenario/text data |
| `sotesw.dll` | 78 MB        | "W" group — wave / music             |
| `sotesp.dll` | 1.1 MB       | "P" group — small misc data          |

Reach: the engine refers to them as the "B" / "W" / "P" resource
groups in error messages.  TBD which letter maps to which DLL — the
"Necessary resource (B) is not found." string fires when sotesd.dll
fails, suggesting "B" = "data" (basic? bulk?  Or just a poor naming).

## PE resource layout (sotesd.dll example)

Two resource types, both unnamed-but-typed:

- `'DATA'` — bulk assets, 759 entries with integer IDs starting at 1000
- `'BITMAP'` (?) — 436 entries (TBD which type; wrestool reports them
  as the unnamed second branch)
- one `--type=16` (RT_VERSION) entry — standard PE version info

All entries are tagged with language 1041 = Japanese (0x411).  The
engine's `FindResourceA` calls don't pass a language so DDU's default
language-selection picks the only available variant.

Many DATA entries are exactly 676996 bytes (chunked uniformly):

```
'DATA' name=1000 size=676996
'DATA' name=1001 size=676996
'DATA' name=1002 size=676996
'DATA' name=1003 size=676996
'DATA' name=1004 size=148612      ← first non-multiple
'DATA' name=1011 size=11382       ← jump (1005-1010 not used)
...
```

Suggests 1000–1004 are a single huge logical blob (maybe a sprite
atlas?) chunked at 676996 bytes (≈ 645 KB) per resource — possibly
hitting a Windows PE resource-size cap (older Windows versions
had ~1 MB practical limits on single `.rsrc` entries, so engines
that needed bigger blobs broke them into 1 MB-ish chunks).

## Load sequence (from `FUN_005a4770`)

```
LoadLibraryA("sotesp.dll")  → hMod stored at DAT_008a6e74    ; near 0x5af7f2
LoadLibraryA("sotesw.dll")  → hMod stored at DAT_008a6e78    ; near 0x575350
LoadLibraryA("sotesd.dll")  → hMod stored somewhere similar  ; near 0x5af5cf

For sotesd.dll only — 60-byte signature check.  See "Quirk" below.

FUN_00579740(hMod, ?, type_str, resource_id, ...)
  └─ FUN_005b62a0(hMod, resource_id)
      ├─ FindResourceA(hMod, MAKEINTRESOURCE(id), type_str)
      ├─ LoadResource(hMod, hRes)
      └─ LockResource(hRes) → pointer stored at zdr->[1] and [2]
```

`FUN_005b62a0` is the **lowest-level PE resource read** — only ever
uses type `"DATA"` (string literal at `0x8a4b40` IIRC).  The "DATA"
hardcode + the integer-ID-only PE layout means every asset lookup is
keyed by `(dll, id)` — no string lookups.

After load, asset bytes are read through `FUN_005b6340`, a
polymorphic reader keyed off `source_kind`:

| kind | meaning                                                  |
|------|----------------------------------------------------------|
| 1    | PE resource (cursor advances through LockResource ptr)   |
| 2    | compressed/decoded stream via `FUN_005b68f0`             |
| 3    | raw file via `ReadFile(in_ECX[3]=hFile, ...)`            |

So the engine has a unified reader abstraction over PE resources,
files, and a third compressed-source kind.  TBD what `FUN_005b68f0`
actually decodes — best guess from naming patterns is a Lizsoft
proprietary RLE or LZSS-style decoder.  When we move on to actually
extracting assets (`tools/extract/sotesd_dat.py` etc.), this is the
next stop.

## "Asset register" calls in the boot driver

The boot driver `FUN_00562ea0` calls these between "Pixel Drawer was
set" and "The resource was set":

```
FUN_00579bd0(ZDD, 1, settings)   ; → CreateFontIndirectA × 8 + UI sprites
FUN_00579a00(ZDD, 1, settings)
FUN_0057a330(ZDD, 2, settings)
FUN_00563ef0(ZDS, settings, 0x4cb, 2, 2, 0)  ; sound bank 0x4cb (1227)
FUN_00563ef0(ZDS, settings, 0x4ca, 2, 2, 0)  ; sound bank 0x4ca (1226)
FUN_00563ef0(ZDS, settings, 0x4c8, 2, 2, 0)
FUN_00563ef0(ZDS, settings, 0x4c9, 2, 2, 0)
FUN_0057ca40(ZDD, 3, settings)
FUN_0057b280(ZDD, 3, settings)
FUN_005749b0(ZDD, 4, settings)
FUN_0056e190(ZDD, 5, settings)   ; ~26 KB function, registers ~hundreds of sprites
"The resource was set"
```

What looked like "load asset blobs" earlier is actually mostly
**sprite registration** — populating in-memory sprite descriptors
with `(resource_id, width, height, colorkey, scale_flag, type)`
without actually decoding the bytes.  Decoding happens lazily
when a sprite is first painted (or in batch at scene entry).

Specifically, `FUN_0056e190` walks through hundreds of `DAT_008a7ce4`,
`DAT_008a7ce8`, `DAT_008a7cec`, … global slots, each filled with:

```c
slot->[7]  = ZDD_ptr;
slot->[1]  = 1;                  // count
slot->[0]  = new uint8_t[8];     // entry array (zeroed)
slot->[2]  = 0;
slot->[6]  = 0;
slot->[0xf] = settings;
slot->[0x10] = 0x592;            // PE resource ID
slot->[8]  = 0xa0;               // width
slot->[9]  = 0xb0;               // height
slot->[0xa] = 0xff00ff;          // colorkey (magenta = transparent)
slot->[0xb] = 1;                 // scale flag
slot->[0xc] = 0;                 // type
slot->[0x42] = (uint16_t)group;  // asset-group tag
```

So a sprite slot is **48 bytes** with a small per-entry suballocation.
Resource IDs cluster in the 0x500+ range for sotesd.dll's sprite
group.

`FUN_00563ef0(ZDS, settings, 0x4cb, ...)` is the **sound-bank load**
call — passes a resource ID through to DSound's `IDirectSound::CreateSoundBuffer`
+ Lock + memcpy + Unlock.  IDs 0x4c8–0x4cb (1224–1227) look like the
core boot sound effects (UI click, menu select, etc.); larger banks
load lazily on scene entry.

## Quirk — `sotesd.dll` 60-byte signature integrity check

`sotesd.dll` carries a 60-byte signature in resource ID `0x7DE` (2014),
read at boot in `FUN_005a4770` near `0x5af6ac`:

1. Load resource `0x7DE` (`type='DATA'`) via `FUN_005b62a0`.
2. Read 4 bytes at offset 4 → check `version >= 0x2713` (10003).
3. Read 60 bytes at offset 60 (decimal `0x3C`).
4. For each byte `b`, compute `b + 0x41`.
5. Compare the resulting 60-character string against the hardcoded
   constant at `0x8a42a4`:

   ```
   JFDGGIUABCVJIEKAUYLPOFDEQBVGSKOLJSCKPIFAXMHGYELSDOBFRKVGBAKB
   ```

6. If mismatch → log `"Necessary resource (B) is not found."` and
   `ExitProcess(0)`.

So sotesd.dll's bytes 60..119 of resource 0x7DE encode that string as
indices into A-Z (each byte ∈ [0, 25] decodes to the corresponding
letter).  Looks like a hand-rolled integrity check rather than any
recognizable hash — probably typed by the author at random as a marker
so a tampered sotesd.dll would fail boot.

Implication for our port: **we must reproduce this check or skip it**.
Either reuse the original sotesd.dll (which is the only legit case
anyway — we don't redistribute it) and just call the same routine, or
no-op the verifier in our drop-in.  No-oping is cleaner since the
user might run our drop-in alongside an updated sotesd.dll someday.

> See `docs/findings/engine-quirks.md` (will add §14 for this).

## Quirk — DSound primary buffer reads from sotesw.dll resource ID 0x2712 with type "MUSICWMA"

The post-LoadLibrary path for sotesw.dll calls:

```c
FUN_00579740(hMod, 1039, "MUSICWMA", 0x2712, 1, 8);
```

That requests resource `0x2712 = 10002` with type `"MUSICWMA"` from
sotesw.dll.  Failure logs `"Sotesw.dll is broken."`.

wrestool doesn't surface a `"MUSICWMA"` type in sotesw.dll (only
'DATA' and the version info entry), so either:
- the type is enumerated by wrestool under a different label, or
- the engine probes for the type and falls back gracefully when it's
  absent, or
- the type IS present but wrestool can't see custom 8-byte type names

Verify via Frida hook on `FindResourceA` during a retail boot.

## Files referenced

- `docs/decompiled/by-address/562ea0.c` (lines 613–625) — asset
  register batch.
- `docs/decompiled/by-address/579bd0.c` — font slot register.
- `docs/decompiled/by-address/579f40.c` — CreateFontIndirectA wrapper.
- `docs/decompiled/by-address/56e190.c` — sprite slot register
  (26 KB; hundreds of similar blocks).
- `docs/decompiled/by-address/5b62a0.c` — FindResource + LoadResource
  + LockResource.
- `docs/decompiled/by-address/5b6340.c` — polymorphic byte reader (PE
  resource / compressed / file).
- `docs/decompiled/by-address/5b68f0.c` — compressed-stream decoder
  (TBD format).
- `0x5af6ac` (radare2 only) — sotesd.dll 60-byte signature check.

## Next stops for Phase 2 extraction work

1. Decompile `FUN_005b68f0` and identify its compression scheme.
   It's the gating step for any extractor that needs to operate on
   the "kind 2" sources.
2. Spec the 759-entry sotesd.dll DATA structure: chunk the sprites
   first (676996-byte aligned IDs 1000-1004), then enumerate the
   smaller entries.
3. Build `tools/extract/sotesd_dat.py` to dump the raw resource
   bytes per ID + a thumbnail BMP for the visual ones.
4. Spec `sotesp.dll` (small enough to enumerate by hand).
5. Identify what 'MUSICWMA' actually resolves to in sotesw.dll.
