# Audio + Input subsystem init

How the engine spins up DSound + DInput from the post-launch driver
`FUN_00562ea0`.  This doc supersedes earlier notes in
`winmain-and-bootstrap.md` that mis-labeled `FUN_005b9fc0` as the
"wave audio" init — it's actually a DInput sub-device, not audio.

## Correct boot order

```
FUN_00562ea0 (post-launch driver)
  ├─ FUN_005b9cf0(&DAT_008a93d0, hInst)     ; ZDI  — DirectInput7 main
  │    ├─ operator_new(0x498c)              ; the ZDI wrapper struct (~18 KB)
  │    └─ FUN_005b9f80(hInst)
  │         └─ DirectInputCreateEx(hInst, 0x0700, &IID_IDirectInput7,
  │                                &self->[1] /* lpDI */, NULL)
  │         ; IID at DAT_00850ea0 — verify in Phase 2 (16 bytes)
  ├─ FUN_005b9fc0(&DAT_008a93d8, hWnd)      ; ZDI keyboard sub-device
  │    ├─ operator_new(0x118)               ; per-device struct
  │    ├─ FUN_005ba220(zdi_ptr)             ; stash parent ZDI ptr, refcount
  │    └─ FUN_005ba410(hWnd)
  │         ├─ lpDI->CreateDeviceEx(GUID_SysKeyboard /* DAT_00850df0 */,
  │         │                       IID_IDirectInputDevice7 /* DAT_00850e90 */,
  │         │                       &lpdid, NULL)   ; vtbl[0x24] = method 9
  │         ├─ lpdid->SetDataFormat(c_dfDIKeyboard /* DAT_00850ec0 */)
  │         │                                       ; vtbl[0x2c] = method 11
  │         ├─ lpdid->SetCooperativeLevel(hWnd, 6
  │         │                = DISCL_FOREGROUND|DISCL_NONEXCLUSIVE)
  │         │                                       ; vtbl[0x34] = method 13
  │         └─ FUN_005ba330(0x20)            ; → lpdid->Acquire (vtbl[0x1c])
  │  "ZDI was set"
  ├─ FUN_005baed0(&DAT_008a93d4, hWnd)      ; ZDS — DirectSound
  │    ├─ operator_new(0x10c)               ; ZDS wrapper struct (268 bytes)
  │    ├─ FUN_005bafa0()                    ; ctor: zero fields, copy engine name
  │    └─ FUN_005bb180(hWnd)
  │         ├─ Ordinal_1(NULL, &lpds, NULL) ; dsound!Ordinal 1 = DirectSoundCreate
  │         ├─ lpds->SetCooperativeLevel(hWnd, 1 = DSSCL_NORMAL)
  │         │                                       ; vtbl[0x18] = method 6
  │         └─ lpds->CreateSoundBuffer(&DSBUFFERDESC{
  │                size  = 0x24,
  │                flags = 0x81 = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME,
  │                bytes = 0,
  │                ... zeros ...
  │              }, &lpdsb_primary, NULL)   ; vtbl[0xc] = method 3
  │  "ZDS was set"
  └─ if (settings->[0x21c] != 1):           ; "sound NOT disabled by other path"
       ├─ operator_new(0x22c)               ; ZDM wrapper struct (556 bytes)
       └─ FUN_005bbb10()                    ; ctor: zero fields, copy engine name
       └─ FUN_005bbeb0(0x32, 8)             ; allocate 50 × 576-byte music slots
                                            ; (max-voices = 50, max-channels = 8)
       "ZDM was set"
```

## Wrapper-struct globals

| global         | type     | role                                  |
|----------------|----------|---------------------------------------|
| `DAT_008a93cc` | `ZDD*`   | DirectDraw wrapper                    |
| `DAT_008a93d0` | `ZDI*`   | DirectInput7 main                     |
| `DAT_008a93d4` | `ZDS*`   | DirectSound wrapper                   |
| `DAT_008a93d8` | `ZDI*`   | keyboard device (sub-ZDI)             |
| `DAT_008a93dc` | `ZDI*`?  | second sub-device (mouse?) — see below|
| `DAT_008a93e4` | `ZDM*`   | music manager (conditional)           |
| `DAT_008a93ec` | `HWND`   | main game window                      |
| `DAT_008a6e80` | `void**` | settings struct pointer (in-memory config.dat) |

`FUN_005b12e0`'s `WM_ACTIVATEAPP` handler iterates `DAT_008a93dc` +
`DAT_008a93e0` (2 entries) calling `FUN_005ba290` on each — looks like
"acquire all DInput devices on app activate".  That suggests there's
a second DInput sub-device after the keyboard, likely mouse, set up
elsewhere (or by `FUN_0058ffa0` at `WM_ACTIVATEAPP` deactivation).

## DSound details

### Wrapper struct (`ZDS`, 0x10C bytes)

| offset | type      | purpose                            |
|--------|-----------|------------------------------------|
| `0x00` | `LPDIRECTSOUND` | DirectSound interface (`*self`)   |
| `0x04` | `LPDIRECTSOUNDBUFFER` | primary buffer ptr        |
| `0x08` | `char[256]` | engine name copy (from `DAT_008a9b6c`) |
| `0x108` | `???`    | TBD                                |

### Primary buffer description

```c
DSBUFFERDESC dsbd = {0};
dsbd.dwSize         = 0x24;                    // 36
dsbd.dwFlags        = DSBCAPS_PRIMARYBUFFER    // 0x01
                    | DSBCAPS_CTRLVOLUME;      // 0x80
dsbd.dwBufferBytes  = 0;                       // PRIMARYBUFFER must be 0
                                               // (other fields zero — no format)
```

`DSBCAPS_CTRLVOLUME` lets the engine `IDirectSoundBuffer::SetVolume`
on the primary buffer to control master volume.  Note this is the
PRIMARY buffer — secondary buffers (per-effect / per-music) are
created later by the asset loader / music manager.

### Error strings (helpful for harness log filtering)

The DSound init logs failures via `FUN_005bb040` with these tagged
strings:

- `s_DirectSound_008a4db4` = `"DirectSound"`
- `s_DirectSoundCreate_008a4dc0` = `"DirectSoundCreate"`
- `s_SetCooperativeLevel_008a4b40` = `"SetCooperativeLevel"`
- `s_CreateSoundBuffer_PrimaryBuffer__008a4d90` = `"CreateSoundBuffer(PrimaryBuffer)"`

### Quirk: DSound is loaded by ordinal, not name

`Ordinal_1()` in `FUN_005bb180` calls `dsound!Ordinal 1`.  In
DSOUND.dll, ordinal 1 is `DirectSoundCreate` — the legacy DSound 1.0
entry point.  The engine doesn't use the named export.  Probably done
to support older DSound runtimes that didn't have name decoration.

> 📍 Verify with `objdump -p /mnt/c/Windows/SysWOW64/dsound.dll | grep -A2 'Ordinal'`
> or via Frida hook on the import.

## DInput details

### Two devices, two structs

The engine creates **two** DInput devices via the same per-device
wrapper class:

- `DAT_008a93d8` — keyboard (set up by `FUN_005b9fc0`)
- `DAT_008a93dc` — TBD (mouse?) — call site not yet identified

The `WM_ACTIVATEAPP` handler loops over both, calling
`FUN_005ba290` (which calls `vtbl[0x1c] = Acquire`).

### Vtable cheat sheet — `IDirectInputDevice7` (32-bit)

| idx | offset | method                  |
|-----|--------|-------------------------|
|  2  | 0x08   | Release                 |
|  7  | 0x1C   | **Acquire**             |
|  8  | 0x20   | Unacquire               |
|  9  | 0x24   | GetDeviceState          |
| 10  | 0x28   | GetDeviceData           |
| 11  | 0x2C   | **SetDataFormat**       |
| 13  | 0x34   | **SetCooperativeLevel** |
| 18  | 0x48   | Poll                    |

### Vtable cheat sheet — `IDirectInput7`

| idx | offset | method                  |
|-----|--------|-------------------------|
|  3  | 0x0C   | CreateDevice            |
|  4  | 0x10   | EnumDevices             |
|  9  | 0x24   | **CreateDeviceEx**      |

## ZDM (Music Manager) details

### Wrapper struct (`ZDM`, 0x22C bytes)

The struct is allocated in `FUN_00562ea0` but ctor work is split
between `FUN_005bbb10` (ctor: zero `[0]`, `[6]`, `[8]`; copy engine
name to `[0xb]`) and `FUN_005bbeb0(0x32, 8)` (which appears to be a
**resize** routine that allocates the per-voice slots):

```c
in_ECX[8] = new uint8_t[50 * 576];  // 50 voices × 576 bytes/voice = 28800 bytes
in_ECX[9] = 50;                      // max voices
in_ECX[7] = 8;                       // max channels per voice?
```

That's 50 simultaneous voices — generous for a 2008 indie 2D game;
suggests heavy layering of music + sfx + ambience.

### Disable-sound gating

The `settings->[0x21c] != 1` test gates whether ZDM is initialized at
all.  The launcher's "Disable Sound" checkbox writes
`DAT_008a9b4e ∈ {3, 4}` (see `engine-quirks.md` §6), but the in-memory
settings struct field at offset `0x21c` is different — TBD how it's
populated.  Best guess: `0x21c` is set to `1` when the launcher's
"Disable Sound" is checked, propagating through the launcher
settings-write path in `FUN_005a4770`.

Even when ZDM is skipped, ZDS (DirectSound) is still initialized — so
the launcher's "Disable Sound" really means "disable music", not "no
audio at all".  Sound effects still play through DSound, just no BGM.

> Verify with a Frida hook on `FUN_005bbb10` while toggling the
> launcher checkbox.

## Notes for the harness

1. The agent's `installSilentAudioHooks` currently only clamps
   `winmm!waveOutSetVolume` — but the engine doesn't appear to use
   winmm waveOut at all, going straight to DSound.  Add a DSound
   wrap: hook `dsound!Ordinal 1` (DirectSoundCreate) and wrap the
   returned `IDirectSound` interface so `IDirectSoundBuffer::SetVolume`
   on the primary buffer always clamps to `DSBVOLUME_MIN` (-10000).

   Easier alternative: the harness sets `auto_disable_sound = true`
   so the launcher's checkbox is ticked, which makes the engine skip
   ZDM init — covers music.  DSound primary buffer still inits at
   volume 0 unless `SetVolume` is called with a non-zero value
   somewhere in scene code; instrument once we see one.

2. The `Acquire` call (`FUN_005ba330(0x20)`) is what binds the
   keyboard device to our process foreground.  When the hidden
   window has no foreground status, Acquire may return
   `DIERR_OTHERAPPHASPRIO` and the engine logs the error but
   continues — input just doesn't work in the harness, which is
   fine for headless smoke runs but not for input-trace replay.
   For replay we'll need to either (a) inject events via
   `keybd_event` / `SendInput` (DInput re-polls via `GetDeviceState`)
   or (b) hook `IDirectInputDevice::GetDeviceState` directly and
   feed our trace there.

## Files referenced

- `docs/decompiled/by-address/562ea0.c` (lines 365–430) — boot driver,
  authoritative call order.
- `docs/decompiled/by-address/5b9cf0.c` — ZDI alloc + DInputCreate.
- `docs/decompiled/by-address/5b9f80.c` — `DirectInputCreateEx` call.
- `docs/decompiled/by-address/5b9fc0.c` — ZDI keyboard sub-device alloc.
- `docs/decompiled/by-address/5ba220.c` — sub-device ctor.
- `docs/decompiled/by-address/5ba290.c` — `Acquire` wrapper.
- `docs/decompiled/by-address/5ba410.c` — keyboard CreateDeviceEx +
  SetDataFormat + SetCooperativeLevel + Acquire.
- `docs/decompiled/by-address/5baed0.c` — ZDS alloc + DSound init.
- `docs/decompiled/by-address/5bafa0.c` — ZDS ctor.
- `docs/decompiled/by-address/5bb180.c` — DirectSoundCreate +
  SetCooperativeLevel + primary buffer.
- `docs/decompiled/by-address/5bbb10.c` — ZDM ctor.
- `docs/decompiled/by-address/5bbeb0.c` — ZDM voice-slot resize.
