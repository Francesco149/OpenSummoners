# Asset loader — `sotesd.dll` / `sotesw.dll` / `sotesp.dll` + PE resources

How the engine pulls game-asset bytes out of the three companion DLLs
plus its own embedded `.rsrc` section.

## Companion-DLL layout

All three are **resource-only DLLs** — tiny `.text` (~11 KB DllMain
stub) and `.data`, plus a massive `.rsrc` section holding the assets:

| dll          | `.rsrc` size | actual content (from extractor)               |
|--------------|--------------|-----------------------------------------------|
| `sotesp.dll` | 1.1 MB       | 31 `WAVE` SFX (RIFF) + 1 small `DATA` blob   |
| `sotesw.dll` | 78 MB        | 47 `DATA` entries, every one a WMA (ASF) file|
| `sotesd.dll` | 168 MB       | 759 `DATA` blobs + 436 `WAVE` SFX             |

All entries are tagged with language 1041 = Japanese (0x411).  The
engine's `FindResourceA` calls don't pass a language so the OS picks
the only available variant.

So the "P / W / D" naming convention is:
- **P** = small sound effects pack (`WAVE`)
- **W** = music (`DATA` blobs containing WMA audio)
- **D** = bulk data — game scenarios, sprites, plus larger sound
  effects (`DATA` + `WAVE`)

### sotesd.dll content shape

759 entries of type `DATA` total ~135 MB.  IDs 1000–1004 are each
exactly 676996 bytes (645 KB) — almost certainly a single logical
blob chunked because individual PE resource entries get awkward past
a few hundred KB.  IDs 1011+ are smaller bespoke entries:

```
'DATA' name=1000 size=676996       ← chunked logical-blob part 1
'DATA' name=1001 size=676996       ← part 2
'DATA' name=1002 size=676996       ← part 3
'DATA' name=1003 size=676996       ← part 4
'DATA' name=1004 size=148612       ← part 5 (final, partial)
'DATA' name=1011 size=11382        ← jump (1005-1010 unused) — standalone entries from here
'DATA' name=1012 size=19574
...
```

The 5-part split lines up exactly with the "kind 2 chunked memory
stream" reader pattern (FUN_005b67c0 spans chunks with stride
`stream->[1]`).  Strongly suggests the engine treats 1000-1004 as a
single in-memory blob assembled at boot.

### sotesw.dll content shape

47 entries, all `DATA` type, every one a complete WMA file (ASF
container).  Sizes 1.5–3 MB each.  The launcher reads "Disable
Sound" → settings->[0x21c] gates ZDM (music mgr) init; if music is
disabled, sotesw.dll is loaded but its resources are never queried.

### sotesp.dll content shape

31 `WAVE` entries plus a single 12-byte `DATA` blob and the standard
RT_VERSION.  Small enough that the contents are likely UI feedback
clicks + early-boot SFX.

## Load sequence (from `FUN_005a4770`)

> ⚠ **CORRECTION (ckpt 26, verified in r2):** the handle-store mapping below was
> backwards.  The disassembly shows `LoadLibraryA("sotesd.dll")` is the one
> stored at **`DAT_008a6e74`** (`mov [0x8a6e74], esi` @ **0x5af5fc**, right after
> its `LoadLibraryA` at 0x5af5ca).  `DAT_008a6e74` is the handle the boot driver
> passes to **every** sprite/font/sound registrar as the `settings` arg — and
> every title sprite resource (logo 0x49f, bg 0x91b/0x91c, slot-0 palette seed
> 0x90b) lives in **sotesd.dll**, not sotesp.dll.  See engine-quirks #51.

```
LoadLibraryA("sotesd.dll")  → hMod stored at DAT_008a6e74    ; mov @ 0x5af5fc  (the `settings` handle)
LoadLibraryA("sotesw.dll")  → hMod stored at DAT_008a6e78    ; near 0x575350
LoadLibraryA("sotesp.dll")  → hMod stored at DAT_008a6e7c(?) ; near 0x5af7f2

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
| 2    | **chunked memory stream** via `FUN_005b68f0` → `FUN_005b67c0`  |
| 3    | raw file via `ReadFile(in_ECX[3]=hFile, ...)`            |

The "kind 2" backend is **not a decompressor** — it's a chunked
memory abstraction (the engine's `FUN_005b6520` error string calls
it `"[RAM Disk Error]"`).  Stream descriptor:

- `[0]` = pointer to an array of chunk pointers
- `[1]` = chunk size (e.g. 676996 bytes for the sotesd.dll multi-part
  resource sets)
- `[0x10]` = current cursor offset
- `[0x14]` = state flag (must equal 1 to read)

`FUN_005b67c0(dst, offset, len, dir)` slices `len` bytes from logical
offset `offset` of the chunked stream into `dst`, transparently
spanning chunk boundaries by indexing the chunk array as
`offset / chunk_size`.  Direction 0 = read, !=0 = write (used by
the save system?).

So all assets are stored **uncompressed** in the resource DLLs, just
chunked at 676996-byte boundaries when they exceed a single PE
resource's practical size cap.  Extraction tools can dump raw bytes
with `wrestool` or `pefile` directly — no decoding pass needed for
the bulk-data stream.

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

## Quirk — `sotesw.dll` ALSO carries a (shorter) signature check

Like sotesd.dll, sotesw.dll is integrity-checked at boot — but the
check is **8 bytes against "MUSICWMA"** instead of 60 against the
random string.  The call site (`0x5753af` after the LoadLibraryA) is:

```c
FUN_00579740(hMod_sotesw, 1039, "MUSICWMA", 0x2712, 1, 8);
```

Signature: `FUN_00579740(hMod, resource_id, expected_str, min_version, ?, sig_len)`.

The routine:

1. Reads resource ID `1039 = 0x40F` from sotesw.dll (type='DATA').
2. Reads 4 bytes for `version`, then `sig_len` (8) bytes for sig.
3. Adds `'A'` to each sig byte → decoded string.
4. Compares `version >= min_version (0x2712)` AND decoded sig == `"MUSICWMA"`.
5. Fail → log `"Sotesw.dll is broken."`, but the engine **continues**
   (just sets `DAT_008a6e78 = NULL` so later music loads no-op).

So `"MUSICWMA"` is NOT a PE resource type — it's the **decoded
8-byte expected signature** for sotesw.dll's resource 1039.

The sotesd.dll check uses the same byte-encoding scheme but inlined
(no helper) — see "sotesd.dll 60-byte signature integrity check"
above.  Likely the sotesw check came later, and the engine had
refactored it into the reusable `FUN_00579740` helper by then.

Verified: **`sotesp.dll` has the same scheme too**, in its sole 12-byte
`DATA` entry (resource ID 1031).  Decoding the trailing 8 bytes
(`05 12 0F 00 13 02 07 11` + `0x41` each) yields the string
**`"FSPATCHR"`** ("Fortune Summoners Patcher" probably, given the P
in the DLL name).  Min version is `0x2711` (one less than sotesw's
`0x2712`).

So all three DLLs are signature-checked at boot with the same
byte-encoding scheme.  The signatures themselves are arbitrary ASCII
tags the engine knows about a priori; only the bytes in the
companion DLLs encode them.

| dll          | sig resource ID | sig string                                                   | min ver |
|--------------|-----------------|--------------------------------------------------------------|---------|
| `sotesd.dll` | 0x7DE = 2014    | `JFDGGIUABCVJIEKAUYLPOFDEQBVGSKOLJSCKPIFAXMHGYELSDOBFRKVGBAKB` | 0x2713 |
| `sotesw.dll` | 0x40F = 1039    | `MUSICWMA`                                                   | 0x2712  |
| `sotesp.dll` | 0x407 = 1031    | `FSPATCHR`                                                   | 0x2711  |

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

## Sample DATA content shape

Quick `od` sampling of the standalone (non-chunked) DATA entries in
sotesd.dll (1011+) reveals a recurring pattern:

```
offset 0x00..0x1f : 32-byte header (8 × little-endian u32)
offset 0x20..0x41f: 256-entry palette (4 BGRA bytes each, BMP-style)
offset 0x420..    : pixel data
```

Sample header from sotesd 1011 vs 1012 (offset hex bytes):

```
1011:  5f 42 00 00  8b 36 00 00  5b 33 00 00  57 b7 00 00  ...
1012:  5f 42 00 00  8b 36 00 00  7b 33 00 00  57 b7 00 00  ...
                                  ^^ only the 3rd u32 differs by 0x20
```

The shared first u32 (`0x425f`) and identical palette suggest these
two are different frames of the same sprite (offsets differ but
layout is the same).  The palette starts with the standard 16-color
VGA palette (black, red, green, yellow, blue, magenta, cyan, gray, …)
then a 16-step grayscale ramp at indices 247–254 — looks hand-tuned
for a low-color 2D engine.

So the sotesd.dll DATA format is a **palettized 8-bit Lizsoft sprite**
with a custom 32-byte header.  Spec-it work is deferred to Phase 4
when we need to render sprites; for now the extractor dumps the raw
bytes and downstream tools can do the rest.

The chunked logical-blob (sotesd 1000-1004) has a different header —
`67 ee 00 00 93 66 00 00 f1 57 00 00 …` — and isn't a palettized
sprite shape.  Probably a different asset family (scenario data?
sprite atlas? font glyphs?).  Differential Frida hook on
`IDirectDrawSurface7::Lock` writes during a retail boot will reveal
which.

## Next stops for Phase 2 extraction work

1. ✅ ~~Decompile `FUN_005b68f0`~~ — done.  It's a chunked-memory
   abstraction, NOT a decompressor; assets are stored raw.
2. Build `tools/extract/sotes_resources.py` — dump every resource from
   sotesd/sotesw/sotesp.dll's `'DATA'` type to per-ID files.  Easy
   first pass with `pefile` or `wrestool`; harder pass joins the
   chunked multi-part resources (sotesd 1000-1004) into single blobs.
3. Identify what each chunked resource is by content-sniffing
   (BMP/WAV/RIFF magic, plaintext, custom).  Build extractor scripts
   for the formats we recognize.
4. Spec `sotesp.dll`'s 1.1 MB content (smallest of the three —
   probably small atlases or scenario data; quickest win).
5. Identify what 'MUSICWMA' actually resolves to in sotesw.dll —
   probably just a WMA-format asset that wrestool can't preview
   because it doesn't recognize the type name.  A Frida hook on
   `FindResourceA` during a retail boot will reveal the actual type
   string the engine queries with.
