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

## The blocking puzzle — RESOLVED 2026-05-24

The bitmap_session is a **stack-local in FUN_004178e0**, not a member
of the sprite_slot.  Sniffed via radare2 `pdf @ 0x4178e0` — the Ghidra
C view dropped the load-bearing `mov ecx, [esp+8]` setups (and there's
no class info on the bitmap-session functions to make the analyzer
restore them in the decomp).  The function prologue is:

```asm
sub  esp, 0x438        ; reserve the 0x434-byte bitmap_session + 4 SEH guard
push esi
mov  esi, ecx          ; ESI = outer this (sprite_slot *)
push edi
lea  ecx, [esp+8]      ; ECX = &bitmap_session (the stack local)
call FUN_005b6e70      ; bitmap_session::release_no_free
```

And every subsequent bitmap_session method call repeats the
`lea ecx, [esp+8]` immediately before its `call`.  The outer slot is
addressed via ESI (`mov ax, [esi+0x40]` for resource_id, `mov ecx,
[esi+0x3c]` for HMODULE).

So `FUN_004178e0` is a sprite_slot member function that operates on
TWO `this`-style pointers: the implicit `this` (sprite slot, ESI-saved)
and an emphemeral bitmap_session built on the stack.

Other corrections vs the earlier draft of this section:

- The resource-type string at `DAT_00854c98` is **"DATA"**, not "BMP"
  (verified via `r2 -q -c 'ps @ 0x854c98'`).  The retail engine uses a
  custom PE resource type called "DATA" for indexed bitmaps; standard
  `RT_BITMAP` (== 2 cast to LPCSTR) is not in play here.
- `[+0x3c]` of the outer this is used as the `HMODULE` arg to
  `FindResourceA` — for the idx-0 palette-ramp callsite that's the
  sotesp.dll module handle the boot driver stashes into the slot's
  `settings` field (see `ar_register_main_sprites` comment in
  asset_register.h).  For other slots, settings is the launcher
  settings record, which would NOT be a valid HMODULE — so this
  function is specific to the sotesp-loaded slot.

The bitmap_session lifecycle inside `FUN_004178e0`:

```c
bool ar_palette_session_begin(ar_sprite_slot *slot, uint8_t out_palette[1024]) {
    bitmap_session session;
    bs_release_no_free(&session);          // FUN_005b6e70 — zero pixels ptr
    bool ok = bs_decode_resource(&session,
                                 slot->settings,    // HMODULE
                                 slot->resource_id, // resource id (u16)
                                 "DATA", 1);        // FUN_005b7800
    bool emit_palette = false;
    if (ok) {
        emit_palette = (bs_get_bit_count(&session) == 8); // FUN_005b6f00
        if (emit_palette) bs_emit_palette_bgra(&session, out_palette); // FUN_005b7b90
        bs_release(&session);              // FUN_005b6e90 — LocalFree pixels
    }
    bs_release(&session);                  // SEH-style idempotent cleanup
                                           // (thunk_FUN_005b6e90)
    return emit_palette;
}
```

Return value: the disasm has `mov eax, edi` at the end with `edi`
seeded as `1` only inside the `bit_count == 8` branch (otherwise it
stays 0 from the `xor edi, edi` at function entry).  So
`FUN_004178e0` returns `true` IFF the decoded bitmap was 8bpp AND the
palette emit happened.  (The early `if (iVar1 == 0)` path returns 0.)

The two `bs_release` calls at the end mirror MSVC's try/finally
codegen: the `bit_count == 8` branch did the LocalFree, but the SEH
unwind handler does it again as a safety net — `bs_release` is
idempotent on a NULL pixel pointer (FUN_005b6e90 / thunk_FUN_005b6e80
both null-check).

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
