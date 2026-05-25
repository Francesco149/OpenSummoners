# DirectDraw 7 init path

How the engine spins up DDraw, from `DirectDrawCreateEx` through the
primary/back-buffer surfaces.  Companion to
`docs/findings/winmain-and-bootstrap.md`.

Conventions: the engine wraps DDraw in a "ZDD" C++ object (the field
naming in the log strings is `ZDDCore`, `ZDDObject`, etc.).  In the
decompiled output, the global `DAT_008a93cc` holds the pointer to that
wrapper, and `DAT_008a93cc + 0x124` is the `IDirectDraw7*` itself.

## Call graph (in bootstrap order)

```
FUN_00562ea0  (post-launch driver)
  ├─ FUN_005b7ee0(&DAT_008a93cc)         ; create ZDD wrapper
  │    ├─ operator_new(0x170)            ; ZDD struct (~368 bytes)
  │    └─ FUN_005b7f80                   ; in-place ctor (zero fields)
  │    └─ FUN_005b88c0                   ; ← DirectDrawCreateEx
  ├─ FUN_005b89d0(hWnd, fullscreen?1:0)  ; SetCooperativeLevel
  ├─ FUN_00582e90                        ; mode-dispatch: CreateScreen
  │    ├─ FUN_005b8900(640, 480, 16, 0)  ; FUN_005b8480-friends: SetDisplayMode (fullscreen path only)
  │    └─ FUN_005b8b40(&DAT_008a6ec0, 640, 480, 0x1ffffff, 1)
  │         └─ FUN_005b9350              ; ZDDObject ctor
  │         └─ FUN_005b95c0              ; primary + back-buffer alloc
  └─ FUN_005b9520                        ; create + attach IDirectDrawClipper
```

## `FUN_005b88c0` — DirectDrawCreateEx

```c
HRESULT DDCreate(ZDD *self) {                  // 0x5b88c0
    HRESULT hr = DirectDrawCreateEx(
        NULL,                                  // lpGUID — primary device
        (void**)&self->ddraw7,                 // self->[0x124] = IDirectDraw7*
        &IID_IDirectDraw7 /* DAT_00850eb0 */,  // riid
        NULL);                                 // pUnkOuter
    if (FAILED(hr)) { LogErr("DirectDrawCreate", hr); return 0; }
    return 1;
}
```

`DAT_00850eb0` is the in-binary IID for `IDirectDraw7`.  Verify in
Phase 2 by parsing the 16 bytes at that address.

## `FUN_005b89d0` — SetCooperativeLevel

```c
HRESULT DDSetCoop(HWND hWnd, BOOL fullscreen) {   // 0x5b89d0
    DWORD flags = fullscreen
        ? (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT)   // 0x13 = 1|2|0x10
        : DDSCL_NORMAL;                                              // 0x08
    HRESULT hr = self->ddraw7->lpVtbl->SetCooperativeLevel(self->ddraw7, hWnd, flags);
    if (FAILED(hr)) { LogErr("DirectDraw", "SetCooperativeLevel", hr); return 0; }
    return 1;
}
```

Vtable call: `(**(code **)(*ddraw7 + 0x50))(...)` — offset 0x50 = method
index 20 = `IDirectDraw7::SetCooperativeLevel`.

> Engine quirk: `DDSCL_ALLOWREBOOT` is set in fullscreen.  Means "let
> ALT-CTRL-DEL boot the system if it has to" — paranoid 2012 flag for
> exclusive-mode mishaps.

## `FUN_00582e90` — mode-dispatch CreateScreen (3560 bytes)

**Prologue (runs unconditionally before the switch):**
```c
if (DAT_008a6ec0 != 0) {                       // prior boot-time ZDDObject
    FUN_005b9390(DAT_008a6ec0);                // release com_back + com_primary + decrement parent ref
    FUN_005bef0e(DAT_008a6ec0);                // operator_delete on the ZDDObject
    DAT_008a6ec0 = 0;
}
```

Then switch on `*(int*)(this + 0x04)` (the frame style we mapped in
`winmain-and-bootstrap.md` §"Window sizing").  `this` (in_ECX) is the
launcher-settings record at `DAT_008a93e4` — its +0x04 holds the radio
selection from the launcher dialog.  Two more state fields are read by
mode 4: `+0x14` (zoom target width) and `+0x18` (zoom target height).

| mode | name           | call sequence                                                                                                                                       |
|------|----------------|-----------------------------------------------------------------------------------------------------------------------------------------------------|
| 0    | "Full"         | `FUN_005b8900(640,480,16,0)` → `FUN_005b8dd0` → `FUN_005b5ac0(2000)` → `FUN_005b8480(640,480,16, 0, 0, NULL)` → `FUN_005b8b40(&DAT_008a6ec0,640,480,0x1ffffff,1)` |
| 1    | "Safe"         | `FUN_005b8900(640,480,16,0)` → `FUN_005b8dd0` → `FUN_005b5ac0(2000)` → `FUN_005b8480(640,480,16, 0, 1, NULL)`                                        |
| 2    | "Windowed"     | `FUN_005b8480(640,480,16, 2, 1, NULL)`                                                                                                              |
| 3    | "DB Mode"      | `FUN_005b8900(640,480,16,0)` → `FUN_005b8dd0` → `FUN_005b5ac0(2000)` → `FUN_005b8480(640,480,16, 3, 1, NULL)`                                        |
| 4    | "Zoom Mode"    | get-display-mode (or override from `DAT_008a6eac`/`DAT_008a6eb0`) → validate `>= state[0x14]/0x18` → `FUN_005b8900(W,H,16,0)` → `FUN_005b8dd0` → `FUN_005b5ac0(2000)` → centre-rect math → `FUN_005b8480(640,480,16, 4, 1, &local_21c)` → `FUN_005b8dd0` |

Notes on each mode:
- **Mode 0** is the only branch that calls `FUN_005b8b40` (CreateSurfacePair)
  *after* `FUN_005b8480` — the comment in `ddraw-init.md` originally treated
  this as the only branch.  In reality the `FUN_005b8480` mode-arg differs:
  0 means primary-only, 1=safe variant, 2=windowed, 3=DB, 4=zoom.
- **Mode 2 (Windowed)** is the only branch that skips `SetDisplayMode`
  entirely — it stays in the desktop's current mode and just builds a
  windowed surface.  Per `FUN_005b8480` body: when its mode arg == 2,
  the primary-surface alloc is skipped and only the GDI back-buffer is
  built.
- **Mode 3 (DB Mode)** uses `FUN_00426110` instead of `FUN_00406440` for
  its error log — likely a "fatal w/ launcher restore" path.
- **Mode 4 (Zoom Mode)** computes a 7-int "rect" blob laid out as:
  `{display_w, display_h, 2, centre_x, centre_y, src_w, src_h}` where
  `centre_x = max(0, (display_w - state[0x14]) / 2)` etc.  This blob
  becomes `FUN_005b8480`'s 6th arg — copied into `this+0x148..+0x164`.

On failure each branch builds a global error-string into `DAT_008a9534`
(a 0x638-byte rolling log buffer gated by `DAT_008a9530`), calls
`FUN_00406440(fail_msg, DAT_008a9b6c /* engine-name header */)`, and
exits via `FUN_005bf5db(0)`.  The two pre-mortem fixed strings:

| string                                                  | meaning                          |
|---------------------------------------------------------|----------------------------------|
| `s_It_failed_in_CreateScreen___Full` @ 0x8a28e8         | mode 0 primary-create failed     |
| `s_It_failed_in_CreateScreen___Safe` @ 0x8a28bc         | mode 1 safe-create failed        |
| `s_It_failed_in_CreateScreen___Wind` @ 0x8a2918         | mode 2 windowed-create failed    |
| `s_It_failed_in_CreateScreen___DB_M` @ 0x8a29c8         | mode 3 / mode 4 DB-create failed |
| `s_It_failed_in_the_display_mode_se` @ 0x8a29f0         | `FUN_005b8900` (SetDisplayMode) failed |
| `s_It_failed_in_the_display_mode_ac` @ 0x8a2a18         | `FUN_005b8950` (GetDisplayMode) failed |
| `s_Zoom_Mode_is_only_playable_in_re` @ 0x8a2944         | zoom rect < required size      |

The `0x1ffffff` constant in `FUN_005b8b40` is the **"no color key"
sentinel** (`FUN_005b9830` interprets it as "skip SetColorKey").  See
`FUN_005b95c0` notes below.

## `FUN_005b8b40` — primary + back-buffer alloc

```c
int CreateSurfacePair(ZDDObject **out, w, h, pixelFmtFlags, count) { // 0x5b8b40
    ZDDObject *zdo = new ZDDObject(...);              // operator_new(0xd8)
    if (zdo) zdo = ZDDObject::ctor(zdo);              // FUN_005b9350
    if (!CreateSurfaceImpl(w, h, 0, pixelFmtFlags, count, 0,0, w, h)) {
        // FUN_005b95c0 — fills in DDSURFACEDESC2 and calls IDirectDraw7::CreateSurface
        delete zdo; return 0;
    }
    *out = zdo;
    return 1;
}
```

`FUN_005b95c0` is the per-surface entry point — see below.

## `FUN_005b95c0` — surface alloc orchestrator (110 bytes)

```c
void CreateSurfaceImpl(w, h, ignored_x_y, x, y, dstW, dstH, bpp, colorkey) { // 0x5b95c0
    FUN_005b97e0(/*y*/, /*bpp*/);                     // pre-fills DDSD lpSurface ptrs in self
    if (!FUN_005b8c00(self->[0x2c]/*IDDSurface7**/,
                      /*w*/, /*h*/, self->[0xcc]/*caps*/, /*bpp*/)) return;
    FUN_005b98c0(w, h, ignored, x, dstW, dstH);       // stash params on self
    FUN_005b9830(colorkey);                           // SetColorKey if not 0x1ffffff
}
```

`0x1ffffff` is the **"no color key" sentinel** — `FUN_005b9830` interprets
it as "skip SetColorKey" and stashes a flag, otherwise it calls
`IDirectDrawSurface7::SetColorKey(DDCKEY_SRCBLT, key)`.  The earlier
note in `FUN_005b8b40` calling this an "unlimited / best-fit hint" was
wrong — it's a sentinel.

## `FUN_005b8c00` — the actual `IDirectDraw7::CreateSurface` call (372 bytes)

This is where the DDSURFACEDESC2 is built and `CreateSurface` (vtable
offset 0x18) actually invoked.  Stack-local DDSURFACEDESC2:

```c
DDSURFACEDESC2 ddsd = {0};
ddsd.dwSize   = 0x7C;                               // 124, sizeof(DDSURFACEDESC2)
ddsd.dwFlags  = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;  // 7
ddsd.dwHeight = h;
ddsd.dwWidth  = w;
ddsd.ddsCaps.dwCaps = caps_in | DDSCAPS_OFFSCREENPLAIN;  // |= 0x40
if (forceVRAM || self->[0x134]/*videomem_flag*/)
    ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;          // |= 0x800

// If self->[0x164] == 2 (explicit-pixelformat mode):
if (self->[0x164] == 2) {
    int bpp = self->[0x168];
    ddsd.dwFlags |= DDSD_PIXELFORMAT;                    // |= 0x1000
    ddsd.ddpfPixelFormat.dwSize  = 0x20;                 // 32
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;             // 0x40
    switch (bpp) {
      case 8:
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;  // 0x60
        break;
      case 16:
        ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
        ddsd.ddpfPixelFormat.dwRBitMask    = 0xF800;       // RGB565
        ddsd.ddpfPixelFormat.dwGBitMask    = 0x07E0;
        ddsd.ddpfPixelFormat.dwBBitMask    = 0x001F;
        break;
      case 24: case 32:
        ddsd.ddpfPixelFormat.dwRGBBitCount = bpp;
        ddsd.ddpfPixelFormat.dwRBitMask    = 0xFF0000;     // XRGB8888 / RGB888
        ddsd.ddpfPixelFormat.dwGBitMask    = 0x00FF00;
        ddsd.ddpfPixelFormat.dwBBitMask    = 0x0000FF;
        break;
    }
}

hr = ddraw7->lpVtbl->CreateSurface(ddraw7, &ddsd, &lpDDS, NULL);

// If we own a palette (self->[0x12c] != 0), bind it to the new surface.
if (succeeded && self->[0x12c] != 0) {
    lpDDS->lpVtbl->SetPalette(lpDDS, self->[0x12c]);    // vtbl[0x7c] = SetPalette (method 31)
}
```

Note: `ddpfPixelFormat.dwRGBBitCount` doesn't appear explicitly assigned
in the 8bpp branch but the field stays zero — DirectDraw treats that as
"infer from palette size".

> Engine quirk: the 24bpp branch sets the SAME masks as the 32bpp branch
> (case 0x18 falls through to case 0x20).  Likely harmless because DDraw
> ignores RGBBitMasks when bit count is 24/32 anyway, but still
> sloppy.

## `FUN_005b9520` — Clipper attach

```c
void AttachClipper(ZDDObject *self) {              // 0x5b9520
    IDirectDrawClipper **pp = &self->clipper;       // self->[0xac]
    if (*pp) { (*pp)->Release(); *pp = NULL; }      // vtbl[8] = Release

    IDirectDraw7 *dd = self->[0xc0]->ddraw7;        // dd = ZDD::ddraw7
    dd->lpVtbl->CreateClipper(dd, 0, pp, NULL);     // vtbl[0x10] = CreateClipper (method 4)
    (*pp)->lpVtbl->SetHWnd(*pp, &hWnd, 0);          // vtbl[0x1c] = SetClipList (method 7) — see note
    self->[0x2c]->lpVtbl->???(self->[0x2c], *pp);   // attach clipper to primary surface (method 28 / SetClipper)
}
```

> Vtable offset 0x1c on the clipper is **SetClipList** by the standard
> `IDirectDrawClipper` order.  But the second arg here is a pointer
> that the engine plausibly fills with hWnd elsewhere — re-check via
> Frida by hooking `IDirectDrawClipper::SetHWnd` *and* `SetClipList`
> and seeing which one fires at boot.  Don't trust the static offset
> read until the runtime trace confirms it.

The third call attaches the clipper to a surface via vtbl[0x70] of
that surface object — `IDirectDrawSurface7::SetClipper` is method 28
(byte offset `28*4 = 0x70`).  ✓

## Vtable cheat-sheet for the harness

`IDirectDraw7` (sorted by method index → byte offset for x86 32-bit):

| idx | offset | method                  |
|-----|--------|-------------------------|
|  2  | 0x08   | Release                 |
|  4  | 0x10   | CreateClipper           |
|  5  | 0x14   | CreatePalette           |
|  6  | 0x18   | **CreateSurface**       |
|  8  | 0x20   | EnumDisplayModes        |
| 20  | 0x50   | **SetCooperativeLevel** |
| 21  | 0x54   | SetDisplayMode          |
| 22  | 0x58   | WaitForVerticalBlank    |

`IDirectDrawSurface7` (sorted by method index — these are the offsets
the engine actually dereferences in `FUN_005b8c00`, `FUN_005b9830`,
`FUN_005b9520`, plus the ones we'll need to hook for frame capture):

| idx | offset | method                  |
|-----|--------|-------------------------|
|  2  | 0x08   | Release                 |
| 11  | 0x2C   | Flip                    |
| 12  | 0x30   | GetAttachedSurface      |
| 24  | 0x60   | IsLost                  |
| 25  | 0x64   | **Lock**                |
| 26  | 0x68   | ReleaseDC               |
| 27  | 0x6C   | Restore                 |
| 28  | 0x70   | **SetClipper**          |
| 29  | 0x74   | **SetColorKey**         |
| 30  | 0x78   | SetOverlayPosition      |
| 31  | 0x7C   | **SetPalette**          |
| 32  | 0x80   | **Unlock**              |
| 33  | 0x84   | UpdateOverlay           |

> ⚠ An earlier read of this table had `Lock` at offset 0x60 and
> `Unlock` at 0x7c.  Wrong.  `Lock` is at 0x64 (method 25) and
> `Unlock` is at 0x80 (method 32); 0x60 is `IsLost`, 0x7c is
> `SetPalette`.  Confirmed by reading `FUN_005b8c00`'s palette-bind
> call site (`vtbl[0x7c]` is called with `(surface, palette*)` — only
> `SetPalette` has that signature).

`IDirectDrawClipper`:

| idx | offset | method                  |
|-----|--------|-------------------------|
|  2  | 0x08   | Release                 |
|  4  | 0x10   | GetHWnd                 |
|  7  | 0x1C   | SetClipList             |
|  8  | 0x20   | SetHWnd                 |

These match what we observed in the disassembly above.

## Files referenced

- `docs/decompiled/by-address/5b7ee0.c` — `CreateZDDCore` wrapper.
- `docs/decompiled/by-address/5b7f80.c` — ZDD ctor (zero fields).
- `docs/decompiled/by-address/5b7fe0.c` — ZDD dtor.
- `docs/decompiled/by-address/5b88c0.c` — DirectDrawCreateEx call site.
- `docs/decompiled/by-address/5b89d0.c` — SetCooperativeLevel.
- `docs/decompiled/by-address/582e90.c` — CreateScreen mode dispatch.
- `docs/decompiled/by-address/5b8b40.c` — surface allocator wrapper.
- `docs/decompiled/by-address/5b95c0.c` — surface alloc orchestrator
  (pre-fill, CreateSurface, stash params, SetColorKey).
- `docs/decompiled/by-address/5b8c00.c` — DDSURFACEDESC2 builder +
  `IDirectDraw7::CreateSurface` call + palette bind.
- `docs/decompiled/by-address/5b97e0.c` — pre-fill DDSD lpSurface ptrs.
- `docs/decompiled/by-address/5b98c0.c` — stash create params on ZDDObject.
- `docs/decompiled/by-address/5b9830.c` — `SetColorKey` (0x1ffffff sentinel
  means "no color key").
- `docs/decompiled/by-address/5b9520.c` — clipper create + attach.
- `docs/decompiled/by-address/5b80d0.c` — DDERR_*-to-string log helper.

## Next-step recommendations for the harness

1. **Hook `DirectDrawCreateEx`** to log the IID + the returned interface
   pointer.  Confirms `DAT_00850eb0 == IID_IDirectDraw7`.
2. **Hook `IDirectDraw7::CreateSurface`** (vtable+0x18) — dump the
   DDSURFACEDESC2 to confirm pixel format, dimensions, flags, back-buffer
   count.  This is what `FUN_005b95c0` builds; it's faster to log it than
   decompile the 46 KB build path.
3. **Hook `IDirectDrawSurface7::Lock` / `Unlock`** to capture the
   pixel buffer for frame-diffing — the Phase-3 scenario harness needs
   these.
4. **Hook `IDirectDrawSurface7::Flip`** to drive the "frame done" event
   in the harness so the input-trace stepper has a clean per-frame tick.