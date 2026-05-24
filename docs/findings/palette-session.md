# Palette session — leaf helpers + deferred PE-resource decoder

Status as of 2026-05-24: the two **leaf** halves of the palette-ramp
trio (`ar_palette_pack_entry` / `ar_palette_install`) are ported.  The
**front** half — `FUN_004178e0`, which seeds the palette from a PE
bitmap resource via `FUN_005b7800` — is NOT ported.  This doc captures
what's known and what blocks finishing.

Caller of the trio is `FUN_005749b0` (ar_register_main_sprites)
between the slot-5 and slot-9 inline writes — see the comment at
`src/asset_register.c:954`.  Shape:

```c
FUN_004178e0(palette_storage);                       // seed from PE bitmap
for (10x) FUN_005b5d90(palette + idx*4, COLORREF);   // override bg entries
for (20x) {                                          // lerp gray → white
    COLORREF c = FUN_005b5f50(0x383838, 0xffffff, i, 20);
    FUN_005b5d90(palette + (10+i)*4, c);
}
FUN_00491770(palette);                               // install onto slot[0]
```

The lerp helper (`ar_color_lerp`, FUN_005b5f50) and the two leaves are
ported.  The seed step is what's missing.

## What `ar_palette_pack_entry` / `ar_palette_install` actually do

**FUN_005b5d90** (33 bytes) — pack a Win32 COLORREF (`0x00BBGGRR`) into
a `PALETTEENTRY` (4 bytes: `peRed, peGreen, peBlue, peFlags`):

```c
out[0] = colorref & 0xff;        // peRed   = low byte
out[1] = (colorref >>  8) & 0xff;// peGreen = mid byte
out[2] = (colorref >> 16) & 0xff;// peBlue  = high byte
out[3] = 0;                      // peFlags = 0
```

Independent of any container — caller owns the destination buffer.
Used to override individual palette entries between the seed step and
the install step.

**FUN_00491770** (52 bytes) — lazy-install a 256-entry (1024-byte)
palette onto a sprite slot's first entry:

```c
if (this->entries[0].b == NULL)
    this->entries[0].b = operator_new(0x400);
memcpy(this->entries[0].b, palette, 0x400);
```

The Ghidra decomp's `*(int *)(*in_ECX + 4)` deref pattern is
`(*this)+4` — `*this` is `entries` (the ar_sprite_entry array
pointer), `+4` of that is `entries[0].b` (the owned-pointer half of
`ar_sprite_entry`).  Matches: the sprite destructor already frees
`entries[i].b` iff non-zero, so this install is leak-clean on slot
teardown.

## The bitmap-session struct (inferred from `FUN_005b71f0` + `FUN_005b6f10`)

`FUN_005b71f0` (117 B) is the constructor-ish init for what looks like
a small bitmap-resource wrapper.  Writes via `in_ECX`:

| offset  | type   | content                                                     |
|---------|--------|-------------------------------------------------------------|
| +0x00   | HLOCAL | LocalAlloc'd pixel buffer (LMEM_ZEROINIT, biSizeImage)      |
| +0x04   | u32    | stride (`bytesPerPixel * width`, written by FUN_005b6f10)   |
| +0x08   | -      | unused?                                                     |
| +0x0c   | u32    | BITMAPINFOHEADER.biSize = 0x28                              |
| +0x10   | u32    | BITMAPINFOHEADER.biWidth                                    |
| +0x14   | u32    | BITMAPINFOHEADER.biHeight                                   |
| +0x18   | u16    | BITMAPINFOHEADER.biPlanes = 1                               |
| +0x1a   | u16    | BITMAPINFOHEADER.biBitCount                                 |
| +0x1c   | u32    | BITMAPINFOHEADER.biCompression = 0                          |
| +0x20   | u32    | BITMAPINFOHEADER.biSizeImage                                |
| +0x24   | u32    | BITMAPINFOHEADER.biXPelsPerMeter = 0                        |
| +0x28   | u32    | BITMAPINFOHEADER.biYPelsPerMeter = 0                        |
| +0x2c   | u32    | BITMAPINFOHEADER.biClrUsed = 0                              |
| +0x30   | u32    | BITMAPINFOHEADER.biClrImportant = 0                         |
| +0x34   | RGBQUAD[256] | bmiColors palette (1024 bytes)                        |

The total inferred size is `0x434` bytes.  No constructor we've seen
yet sizes the heap allocation that produces an instance.

The three "release" helpers all operate on offset +0x00:

| function       | body                                                         |
|----------------|--------------------------------------------------------------|
| FUN_005b6e70   | `*this = 0` (zero the pixel buffer pointer — no free)        |
| FUN_005b6e90   | `if (*this) { LocalFree(*this); *this = 0; }`                |
| thunk_FUN_005b6e80 | tail-jump to FUN_005b6e90 (same body)                    |
| FUN_005b6f00   | `return *(u16*)(this+0x1a)` (depth getter)                   |
| FUN_005b6f10   | `*(u16*)(this+0x1a) = depth; *(u32*)(this+4) = stride`       |

## The PE-resource decoder (`FUN_005b7800`, 359 B)

Two decode paths gated by the 4th argument `lazy_or_decompress`:

**Path 1 — `lazy_or_decompress == 0`** (raw / pre-decoded resource):

```c
data = LockResource(LoadResource(FindResourceA(hModule, id, type)));
header = data + data[0];      // header offset stashed in data[0]
biWidth  = data[1];
biHeight = data[2];
biBitCount = *(u16*)(data + 0x0e);
FUN_005b71f0(biWidth, biHeight, biBitCount);   // alloc pixel buf + init BIH
if (this->biBitCount == 8) {
    memcpy(this->palette, header, 0x400);
    pixels_src = header + 0x100*4;   // pixels after palette
} else if (this->biBitCount == 24) {
    pixels_src = header;             // pixels at header
} else {
    return 0;                        // unsupported depth
}
memcpy(this->pixels, pixels_src, biHeight * stride);
```

**Path 2 — `lazy_or_decompress != 0`** (compressed-resource header
parse via `FUN_005b7c10`, 186 B):

The resource payload starts with a packed pointer-table header
(0x2711-byte signature at `data[+0x438] - data[+0x448]`).  `FUN_005b7c10`
unpacks header fields, then a palette of 256 RGBQUADs is copied from
inside `data` (offset chain that's been rebased per the header
pointers) and pixel data starts at `data + offset + 0x458`.

I haven't decoded the compressed-resource header packer enough to
match it against any known sprite-pack format.  Worth checking
`tools/extract/lizsoft_sprite.py` for the matching unpacker — the
LizSoft sprite pack format may be exactly this.

## The blocking puzzle: which `this` is FUN_005b7800 called on?

`FUN_004178e0` (the begin-palette wrapper, 194 B) opens with:

```c
FUN_005b6e70();   // bitmap-session release-but-don't-free
iVar1 = FUN_005b7800(this->[+0x3c],            // HMODULE
                     this->[+0x40],            // u16 resource id
                     "BMP",                    // resource type
                     1);                       // compressed path
if (iVar1 != 0) {
    if (FUN_005b6f00() == 8) FUN_005b7b90(param_1);  // RGBA->BGRA swap
    FUN_005b6e90();                            // release pixel buf
}
```

The offsets `+0x3c` and `+0x40` MATCH `ar_sprite_slot.settings` and
`ar_sprite_slot.resource_id` — the slot's first
`ar_sprite_slot_register` call already populates them.  So
`FUN_004178e0`'s `this` is plausibly an `ar_sprite_slot`.

But `FUN_005b7800`'s `this` needs the bitmap-session layout — pixel
buffer at +0x00, palette at +0x34 (1024 bytes).  On an `ar_sprite_slot`:

- `+0x00` is `entries` (the pointer to ar_sprite_entry[] — a heap
  pointer, owned).  Overwriting it with the LocalAlloc'd pixel buffer
  would corrupt the slot.
- `+0x34` is `aux_buf` (one void pointer, NOT 1024 bytes of palette).
  Overwriting +0x34..+0x434 would walk past the end of the sprite slot
  (sized 0x44) into adjacent BSS.

Either:

1. **FUN_005b7800 runs on a different ECX than FUN_004178e0.**  In
   x86 __thiscall ECX is callee-clobberable.  The Ghidra decomp's
   `in_ECX` is a heuristic — it represents "whatever was in ECX at
   function entry", and downstream calls may have set ECX explicitly
   to some other object before invoking `FUN_005b7800`.
   If true, the actual ECX is probably a bitmap-session struct that
   lives elsewhere — maybe as a static, maybe pointed to by some
   other field of the sprite slot (e.g. an unmodelled field past
   +0x44, or a global accessed via `[g_decoder_session]`).

2. **The sprite slot is bigger than 0x44 and overlays a
   bitmap-session region.**  Our 0x44 size is determined by the
   register-time write set; the actual class allocation in retail
   could be wider.  In that case `entries` (+0x00) is actually a
   union with the pixel buffer pointer, `aux_buf` (+0x34) is actually
   the palette start, etc.  This would mean both ports we've already
   landed (`ar_sprite_slot_destroy` freeing entries, etc.) are
   misinterpreting the field — possible but would force a rework.

Resolving this requires reading the ECX setup in `FUN_004178e0`'s
disassembly directly — the Ghidra C view drops the `mov ecx, …`
instructions for __thiscall callsites the analyzer didn't tag.  See
the WndProc dependency formalization track (HANDOFF "Next move" #2):
the right move is to model the bitmap_session struct in a header,
add it to `TagThiscallFunctions.java`'s TAGS array, and re-export the
decomp via `./tools/ghidra-tag-and-export.sh`.  After that the
typed `this->field` reads will reveal which class FUN_005b7800
actually operates on.

## Win32 stub plan (for when the decoder gets ported)

Pattern follows `asset_register_win32.c`:

- `bitmap_session.c` — pure: struct definition, raw-decode path,
  compressed-decode path, palette-swap.
- `bitmap_session_win32.c` — real: thin wrappers around
  `LocalAlloc/Free` and `FindResourceA/LoadResource/LockResource`,
  declared as externs in the pure header.
- Test stubs replace the Win32 wrappers — `LocalAlloc → calloc`,
  `LocalFree → free`, resource trio returns a per-test byte buffer
  provided by the harness.

## Pointers into existing code

- Two ported leaves: `src/asset_register.c` "FUN_005b5d90 — pack" and
  "FUN_00491770 — install" sections.
- Tests: `tests/test_asset_register.c` `palette_pack_entry_*` and
  `palette_install_*`.
- Wiring point (deferred): `src/asset_register.c:954` in
  `ar_register_main_sprites`.
- Original Ghidra decomps: `docs/decompiled/by-address/4178e0.c`,
  `491770.c`, `5b5d90.c`, `5b71f0.c`, `5b6e70.c`, `5b6e80.c`,
  `5b6e90.c`, `5b6f00.c`, `5b6f10.c`, `5b7800.c`, `5b7b90.c`,
  `5b7c10.c`.
