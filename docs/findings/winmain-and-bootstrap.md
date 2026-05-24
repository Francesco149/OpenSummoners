# WinMain, window, message pump, frame limiter

How the engine boots from PE entry to the first frame.  Companion to
`docs/findings/launcher-dialog.md` (the launcher dialog that gates
the post-WinMain init path).

## Call graph

```
entry @ 0x5c0a8f                              ; CRT startup
  └─ WinMain @ 0x562210                       ; FUN_00562210
       ├─ log "The application was started"
       ├─ DeleteFileA("fs_boot.log")
       ├─ set locale + atexit-style cleanup
       ├─ allocate game-state struct DAT_008a9b68 (0xe8 bytes, zeroed)
       ├─ FUN_005a4770(hInstance)             ; ~46 KB; init path
       │   ├─ launcher dialog (FUN_005b0d70 → DialogBoxParamA)
       │   ├─ RegisterClassExA  CLASS_LIZSOFT_SOTES → WndProc @ 0x401210
       │   └─ CreateWindowExA   (FUN_005b0ee0)
       └─ do { /* per-game-run loop */
            ├─ allocate per-run struct (FUN_005b6580 etc.)
            ├─ FUN_00562ea0          ; post-launch driver (see below)
            └─ FUN_005624c0 + free struct
          } while (FUN_00562ea0 returned 9)   ; 9 = restart game
       └─ FUN_005b10d0()                       ; final cleanup
```

`FUN_005a4770` is **46 KB** so the Ghidra decompiler times out on it; we
have to read the relevant slices in radare2.  It's the catch-all init —
parses command line, reads `config.dat`, runs the launcher dialog,
registers the window class, creates the main window.  We'll spec the
parts we need as Phase 1 demands rather than chase the whole 46 KB.

## Entry point details

```c
void entry(void) {                                 // 0x5c0a8f
    DWORD ver = GetVersion();                      // …populate DAT_008a9bac etc.
    if (!FUN_005c6394(0)) FUN_005c0baa(0x1c);     // CRT heap init or noreturn
    FUN_005c61e9();                                 // CRT init globals
    DAT_008ab2c4 = GetCommandLineA();
    DAT_008a9c14 = FUN_005c60b7();                  // GetEnvironmentStringsA wrapper
    FUN_005c5e6a(); FUN_005c5db1(); FUN_005bf5ae(); // more CRT init
    GetStartupInfoA(&si);
    nShowCmd = (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT;
    HINSTANCE hInst = GetModuleHandleA(NULL);
    int rc = WinMain(hInst, NULL, lpCmdLine_garbage, nShowCmd);
    FUN_005bf5db(rc);                               // ExitProcess(rc)
}
```

Standard MSVC CRT shim; nothing surprising.

## Window class registration

In `FUN_005a4770` around `0x5a4c28`:

```asm
mov  dword [WNDCLASSEXA.cbSize],          0x30
mov  dword [WNDCLASSEXA.style],           3            ; CS_HREDRAW|CS_VREDRAW
mov  dword [WNDCLASSEXA.lpfnWndProc],     0x401210     ; WndProc
mov  dword [WNDCLASSEXA.hInstance],       hInst
...                                                    ; everything else zero
call LoadCursorA(NULL, IDC_ARROW)                      ; → hCursor field
call RegisterClassExA(&wndclass)
```

Class name `CLASS_LIZSOFT_SOTES` (string at `0x8a4360`).

There are **two** `RegisterClassExA` call sites in `FUN_005a4770`
(0x5a4ca8 and 0x5af314).  TBD if the second one is a different class
(e.g., a sub-window) — defer.

Main window is created later in `FUN_005b0ee0`:

```c
CreateWindowExA(0,
    "CLASS_LIZSOFT_SOTES",
    /* lpWindowName: built from "Fortune Summoners ", version, "Ver1.2 ", " - Product Ver. -" */,
    WS_POPUP,                                       // 0x80000000
    0, 0, 100, 100,                                  // tiny initial rect
    NULL, NULL, hInst, NULL);
```

The post-creation `SetWindowPos` / `SetWindowLongA` calls in
`FUN_00562ea0` resize and apply the correct frame style based on the
launcher's screen-mode selection.  See "Window sizing" below.

## Two WndProcs, two classes

The engine registers **two** window classes inside `FUN_005a4770`:

| reg site   | class string         | WndProc      | what it is                |
|------------|----------------------|--------------|---------------------------|
| `0x5a4ca8` | `CLASS_LIZSOFT_WAIT` | `0x401210`   | "Please wait." splash     |
| `0x5af314` | `CLASS_LIZSOFT_SOTES`| `0x5b12e0`   | main game window          |

Earlier this doc claimed both used `0x401210`; that was wrong — confirmed
by harness runs where `CreateWindowExA` returns the `CLASS_LIZSOFT_WAIT`
HWND first, with `0x401210` as its WndProc, and the main `CLASS_LIZSOFT_SOTES`
window appears later with `0x5b12e0` (set at `0x5af2c7`:
`mov dword [esp+0x50], 0x5b12e0`).

## CLASS_LIZSOFT_WAIT WndProc @ 0x401210

Disassembled from radare2 (Ghidra missed it; function pointer only).
Only handles **WM_PAINT** specially:

```c
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static const char wait[16] = "Please wait.";
    GameState *g = DAT_008a6b3c;

    if (uMsg == WM_PAINT /* 0x0f */) {
        HDC hdc = GetDC(hWnd);
        if (g->offset_0x108 != 0) {
            // Real game frame ready: blit via the renderer (FUN_005b6ee0/5ba390/5b6f30).
        } else {
            // Loading screen: draw two filled rects + text rows
            FUN_005b5ef0(hdc, 0, 0, g->w-1, g->h-1, ...);
            FUN_005b5ef0(hdc, 4, 4, g->w-9, g->h-9, ...);
            SetTextColor(hdc, 0);
            TextOutA(hdc, 80, 20, &g->offset_0x114, strlen(...));   // status string
            TextOutA(hdc, 80, 40, wait, 12);                         // "Please wait."
        }
        ReleaseDC(hWnd, hdc);
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);              // everything else
}
```

That's the entire WAIT-window WndProc — its only job is to draw a
"Please wait." splash while the main bootstrap runs.  All other
messages delegate to `DefWindowProcA`.

## CLASS_LIZSOFT_SOTES WndProc @ 0x5b12e0

The main game window's WndProc — 441 bytes, Ghidra picked it up.
Handles these messages explicitly; everything else delegates:

| msg              | handler                                                         |
|------------------|-----------------------------------------------------------------|
| `WM_DESTROY`(0x2) `WM_MOVE`(0x3) `WM_SIZE`(0x5) | `break` (no-op)                       |
| `WM_PAINT` (0xf) | call render path if `state` + DDraw + DAT_008a93ec gates pass   |
| `WM_CLOSE` (0x10)| `ExitProcess(0)` — direct, immediate                            |
| `WM_ACTIVATEAPP` (0x1c) | **`DAT_008a952c = wParam`** — see below                  |
| `WM_KEYDOWN` (0x100), default → DefWindowProcA                                     |
| `WM_TIMER` (0x113)| clear `state[7]` (timer ack)                                   |

**`WM_ACTIVATEAPP` is load-bearing.**  The pump (`FUN_005b1030`) only
breaks out of its outer spin loop when `DAT_008a952c != 0` — i.e. only
when the engine has been told the app is in foreground.  A hidden
window in our harness never gets that activation from the OS, so the
pump spins forever and the engine never advances past whatever called
it.  Fix: the Frida agent posts `WM_ACTIVATEAPP(TRUE)` to the main
window as soon as it appears (see `installPeriodicWindowScan` in
`tools/frida/opensummoners-agent.js`).

When activation hits, the WndProc additionally activates DInput
devices via `FUN_005ba290` and logs `ActivateInputDevice_CPx` —
but only if `DAT_008a9b64[2]` is non-zero (i.e. game state struct
is set up).  Early in init the gate is zero, so the activation
handler just sets the flag and returns immediately.

There is **no `WM_CLOSE` polite handler** — the only handler is
`ExitProcess(0)`.  Steam-overlay-induced WM_CLOSE is swallowed by
that.  Clicking the X on the title bar destroys the window and
the pump (`PeekMessage(NULL,…)` returns 0 forever), hanging the
process unless the engine's own `ExitProcess` call fires first.

## Main message pump + frame limiter — `FUN_005b1030`

```c
void PumpAndYield(void) {                          // 0x5b1030
    MSG msg;
    while (1) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT /* 0x12 */) {
                ExitProcess(0);                    // FUN_005bf5db
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (DAT_008a952c != 0 && state->offset_0x1c == 0) break;
        WaitMessage();                             // block until next message
    }
    if (state->offset_0xc /* hTimer */ != 0) {
        DWORD now = GetTickCount();
        if (state->offset_0x10 == 0
         || state->offset_0x10 - now < 5)        // ≈ "5 ms or less remaining"
        {
            state->offset_0x1c = 1;                // mark tick ready
        }
        state->offset_0x10 = now;                  // record last-frame time
    }
}
```

`state->offset_0x0c` is the `SetTimer` handle (id=1, 10 ms interval — set
in `FUN_00562ea0` at line "An intermittent timer was set").  The pump
yields via `WaitMessage` and unblocks when the timer fires; the timer
maps to **~100 Hz**.

`state->offset_0x1c` is the "tick ready" flag, polled by per-scene
loops (e.g. `FUN_0056aea0`) to decide whether enough time has passed
to advance the simulation.

`state->offset_0x10` is the wall-clock `GetTickCount` of the last
processed tick.

> Engine uses **`GetTickCount`** exclusively — no `timeGetTime`,
> `QueryPerformanceCounter`, or `RDTSC`.  Confirmed by `iiq~` import
> check on the unpacked binary.  Our Frida agent's `timeGetTime`
> virtualization (carried over from openrecet) does **nothing** here —
> we need to virtualize `GetTickCount` instead, and stub `WaitMessage`
> for turbo (it's the main "sleep" mechanism).

## Scene driver — `FUN_00562ea0` (post-launch loop)

Called by WinMain in a `do…while(rc==9)` outer loop, so returning 9
restarts the whole game from this entry.  Bootstrap order (one-time
init):

| log string                              | function                                              |
|-----------------------------------------|-------------------------------------------------------|
| `AS Start`                              | (start banner)                                        |
| sizes screen by mode (see below)        |                                                       |
| `Area information check end`            | (sanity check on `DAT_006940c8` table)                |
| allocates `0x38d` × tile / `0x1d1` × sprite / `0x10` × ?? structs |                          |
| `SS_MGR Preparation`                    | `operator_new` loop, sprite-system mgr                |
| `W_MGR Preparation`                     | `operator_new` loop, world mgr                        |
| `GD_MGR Preparation`                    | `operator_new` loop, game-data mgr                    |
| `The environmental setting was read`    | reads `*DAT_008a6e80` (settings struct, in-memory `config.dat`); writes `state->offset_0x04` ∈ {0,1,2,3,4} = frame style |
| `An intermittent timer was set`         | `SetTimer(hWnd, 1, 10, NULL)`                          |
| `The window was set`                    | `SetWindowLong + SetWindowPos` switch on mode          |
| (no log)                                | `FUN_005b9cf0(&DAT_008a93d0, hInst)` — **DInput init (ZDI)** |
| (no log)                                | `FUN_005b9fc0(&DAT_008a93d8, hWnd)` — **wave audio (ZDW?) init** |
| `ZDI was set`                           | input subsystem ready                                  |
| `FUN_005baed0(&DAT_008a93d4, hWnd)`     | DSound init                                            |
| `ZDS was set`                           | audio subsystem ready                                  |
| (conditional, if `*(*settings + 0x21c) != 1`) | `FUN_005bbb10()` — music mgr (ZDM)              |
| `ZDM was set`                           | music subsystem ready (only if not disabled)           |
| `FUN_005b7ee0(&DAT_008a93cc)`           | **DDraw7 core** (ZDD)                                  |
| `ZDD was set`                           | DDraw subsystem ready                                  |
| `FUN_005b89d0(hWnd, fullscreen?1:0)`    | `IDirectDraw7::SetCooperativeLevel`                   |
| `CooperativeLevel was set`              |                                                        |
| `FUN_005b8b40(...)` + `FUN_005b9520`    | surface alloc + screen mode set                        |
| `The screen was set`                    |                                                        |
| (large palette / color table fills)     |                                                        |
| `Pixel Drawer was set`                  |                                                        |
| `FUN_00579bd0/00579a00/0057a330/...`    | load asset blobs from `sotesd.dll`/`sotesw.dll` (TBD)  |
| `The resource was set`                  |                                                        |
| (conditional `FUN_005752e0`)            | special-resource init                                  |
| `Music was set`                         |                                                        |
| `FUN_00583c90(1)` + `FUN_00583ee0`      | wire input devices, enumerate gamepads                 |
| `The input device connection setting`   |                                                        |
| `Preparation completion`                | bootstrap done                                          |

After bootstrap, `FUN_00562ea0` enters the **scene state machine**:

```c
do {
    if (DAT_008a93e4) FUN_005bcb30();   // ZDM tick (music mgr)
    if (DAT_008a7608) FUN_005640b0();   // SP mgr tick
    FUN_00583c90(1);                    // input poll
    if (state->offset_0x10c == 0)
        FUN_005a4760();                 // (zeroes state->[0], [4])
    int next = FUN_0056aea0(lvl);       // Title menu — runs its own pump+tick
    switch (next) {
        case 0:  /* normal next-scene rotation */
        case 6: case 8: return result;  /* exit */
        case 9:  return 9;              /* restart game (back to WinMain) */
        case 0x1a: FUN_00564160(); FUN_0056cd20(); FUN_0059ec30(0,0,0x3f2); break;
        case 0x1b: FUN_0059ec30(0x2724, 0, 0); break;
        case 0x1c: ... call FUN_0056a670 ...; break;
        case 0x1d: FUN_0040a5d0(0,0,0,0,1); FUN_00568de0(0); break;
        case 0x1e: FUN_00583fe0(); break;
    }
} while (1);
```

So there is **no single "main loop"** in the classic sense — each scene
function (title menu, world, battle, dialog, …) runs its **own** message
pump + tick loop until it returns a state code that tells the outer
driver which scene to enter next.  This is a state machine of "scenes",
not a single PeekMessage/Dispatch/Tick loop.

The pump (`FUN_005b1030`) is called from **at least 10 places** in
the decompiled output (every scene function has its own pump call).

## Window sizing — `state->offset_0x04` modes

Set early in `FUN_00562ea0` from the launcher's `DAT_008a9b48` (3=Windowed,
4=Fullscreen, 5=Zoom), with additional `+0x40` `VRAM` weighting:

| mode | flag-style              | ex-style          | size       | how positioned                  |
|------|-------------------------|-------------------|------------|----------------------------------|
| 0    | WS_POPUP / no caption   | WS_EX_TOPMOST     | 640×480    | top-left (0,0)                  |
| 1    | WS_POPUP / no caption   | WS_EX_TOPMOST     | 640×480    | top-left (0,0)                  |
| 2    | overlapped + caption    | none              | 640×480 client | centered on desktop          |
| 3    | WS_POPUP / no caption   | WS_EX_TOPMOST     | 640×480    | top-left (0,0)                  |
| 4    | WS_POPUP / no caption   | WS_EX_TOPMOST     | size-of-DESKTOP (Zoom) | top-left          |

Engine native resolution is **640×480** for cases 0/1/2/3.  Zoom (case 4)
hits the desktop resolution.  The Zoom-1280 / Zoom-1920 launcher options
(`DAT_008a9b48 == 5`) get folded into the `+0x24` and `+0x40` settings
struct fields, which `FUN_00562ea0` consults to pick 640×480, 1280×960,
or 1920×1440 client size.

> 640×480 confirms what `engine-quirks §3` hinted at — the "Zoom" toggle
> is a *scaler*, not a resolution change at the engine level.  All
> rendering is at 640×480 and the OS resizes the front buffer.

## Files referenced

- `docs/decompiled/by-address/5c0a8f.c` — `entry`
- `docs/decompiled/by-address/562210.c` — WinMain
- `docs/decompiled/by-address/562ea0.c` — post-launch driver / scene
  state machine
- `docs/decompiled/by-address/5b1030.c` — pump + frame limiter
- `docs/decompiled/by-address/5b0ee0.c` — CreateWindowExA wrapper
- WndProc @ `0x401210` (radare2 only; Ghidra missed)
- Class-registration site at `0x5a4ca8` inside the 46 KB
  `FUN_005a4770` (radare2 only; Ghidra decompile timed out)

## Notes for the harness

1. **Virtualize `GetTickCount` for turbo**, not `timeGetTime`.  Confirmed:
   the engine never imports `timeGetTime`.  *Gate the virtualization
   on first PeekMessage entry* — pre-pump init has busy-waits keyed
   off GetTickCount that livelock if the clock jumps 17 ms per call.
2. **Stub `WaitMessage`** to return immediately *on the main thread
   only*.  The pump yields there between frames; stubbing it lets
   PeekMessage spin and tick frames as fast as the OS can deliver
   `WM_TIMER` messages.  Background threads (audio mixer, file I/O)
   may use WaitMessage for real OS waits — keep their semantics.
3. **`Sleep` → `Sleep(0)`** (yield) instead of true no-op.  A true
   no-op starves background threads of CPU, and the main thread
   may be polling a flag set by exactly such a thread.
4. **Post `WM_ACTIVATEAPP(TRUE)`** to the main game window as soon as
   it appears.  Without this the pump spins on `DAT_008a952c == 0`
   forever — see CLASS_LIZSOFT_SOTES WndProc section above.
5. The pump's `WM_QUIT` handler is `ExitProcess(0)`.  If we post
   `WM_QUIT` from the harness, the process exits cleanly *only if*
   PeekMessage sees it before the engine's own state-machine exits.

## Notes for the port

- Allocate `state` (currently `DAT_008a9b68`, 0xe8 bytes); zero it.
- Allocate per-run sub-struct (`FUN_00562210` allocates a 0x20 struct
  with three 0x124 children — TBD shape; the children look like
  per-asset-manager handles).
- Re-running a game = re-allocating and entering the do-while.  Return
  code 9 means "restart".
- The WndProc-handles-only-WM_PAINT pattern is fine to reproduce; we
  don't need to add a proper `WM_CLOSE` handler unless we want to make
  the port more polite than retail.