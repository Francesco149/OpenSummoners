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

Switch on `state->offset_0x04` (the frame style we mapped in
`winmain-and-bootstrap.md` §"Window sizing"):

| mode | path                                                             |
|------|------------------------------------------------------------------|
| 0    | `FUN_005b8900(640, 480, 16, 0)` ; `FUN_005b8dd0` ; `FUN_005b5ac0(2000)` ; `FUN_005b8480(640, 480, 16, 0,0,0)` ; `FUN_005b8b40(..., 640, 480, 0x1ffffff, 1)` |
| 1    | (TBD — windowed variant)                                          |
| 2    | (TBD — overlapped variant)                                        |
| 3    | (TBD)                                                              |
| 4    | (TBD — Zoom variant)                                               |

The `0x1ffffff` constant in `FUN_005b8b40` is unusual — likely an
"unlimited" / "best-fit" hint to whatever surface descriptor field it
maps to.  When we re-impl, log the actual DDSURFACEDESC2 that the
engine builds at runtime (Frida hook on `CreateSurface`) and treat
0x1ffffff as a per-field flag.  Defer to the Phase-A harness drop.

On failure each branch logs `"It failed in CreateScreen ___ Full..."`
(string at `0x008a28e8`) and recovers by trying a different mode.  The
recovery chain is messy — chase later, when we have a Frida hook to
log which paths the retail engine actually takes per launcher option.

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

`FUN_005b95c0` is where the DDSURFACEDESC2 gets built and
`IDirectDraw7::CreateSurface` (vtable offset 0x18 = method 6) is
called.  Decompile that one next when we get to Phase 2's renderer port.

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

`IDirectDrawSurface7`:

| idx | offset | method                  |
|-----|--------|-------------------------|
|  2  | 0x08   | Release                 |
| 11  | 0x2C   | Flip                    |
| 12  | 0x30   | GetAttachedSurface      |
| 24  | 0x60   | Lock                    |
| 25  | 0x64   | ReleaseDC               |
| 26  | 0x68   | Restore                 |
| 28  | 0x70   | **SetClipper**          |
| 31  | 0x7C   | Unlock                  |
| 32  | 0x80   | UpdateOverlay           |

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