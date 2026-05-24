# OpenSummoners — Progress log

Append-only changelog.  Newest entries first.  Each entry: date + heading,
then 1–3 short paragraphs.  Cross-link to `docs/findings/*.md` and
specific commits where relevant.

---

## 2026-05-24 — Bootstrap (Phase 0)

Initial commit run.  Set up the project shape: nix flake with mingw-w64
i686 cross compiler + Ghidra + Frida-tools + Python (pillow/numpy/
sk-image/opencv/construct/rich/frida-python), `.editorconfig`,
`.gitignore`, MIT license, README.

`tools/setup.sh` — symlinks the user's Steam install of Fortune Summoners
into `vendor/original/`, detects Steam DRM by checking for a `.bind`
section in `sotes.exe`, runs Steamless via WSLInterop, and stashes the
unpacked binary in `vendor/unpacked/sotes.unpacked.exe`.  First run:
Steamless identified SteamStub Variant 2.1 and unpacked cleanly.
Original SHA: `7d779f2eb02b3c603857fedbc52be6973ac3b0b2c5c1bc696122ddac89fb9f1b`,
unpacked SHA: `9e032483b9981f73cabb83baca17a734fd9e7c41e114703900d9ee82c7969516`.

`tools/launcher/opensummoners-launcher.exe` — Job-Object supervisor copied
verbatim from OpenMare.  Guarantees no orphaned Windows-side .exes after a
SIGKILL'd WSL run.  Same `--timeout-ms` / `--grace-ms` / `--no-stdin-watch`
flags as the sibling.

`src/main.c` + `src/dev_hooks.c` — WinMain skeleton with the four
drop-in defaults the user asked for from day one:
  1. Auto-cd into `OPENSUMMONERS_GAME_DIR` + `SetDllDirectoryA` to the
     same, so any later `LoadLibrary` resolves game-dir DLLs first.
     `OPENSUMMONERS_GAME_DIR` is exported by the flake's shellHook with
     `WSLENV=…/p` so the .exe sees the Windows-form path.
  2. `user32!MessageBoxA/W` prologue patch that redirects every modal
     to stderr with a distinctive `[!!! REDIRECTED MESSAGEBOX !!!]`
     banner and auto-returns IDOK.  Override with `--show-msgbox`.
  3. Single-instance mutex (`OpenSummoners-SingleInstance`) catches
     stray .exes from previous SIGKILL'd runs.
  4. `--hide-window` / `--frames N` flags for harness/smoke runs.
Single-TU build (per the user's preference), two outputs:
`opensummoners.exe` (GUI subsystem) and `opensummoners-debug.exe`
(console subsystem, stderr surfaces in the launching shell).

`tools/ghidra-headless.sh` + `tools/ghidra-scripts/ExportDecompiledC.java`
— batch decompiles `vendor/unpacked/sotes.unpacked.exe` to
`docs/decompiled/` (gitignored).  Java post-script because nixpkgs'
Ghidra isn't built with PyGhidra.  First-run analysis kicked off in
background while we wrote the rest of Phase 0.

`tools/frida/opensummoners-agent.js` + `tools/frida_capture.py` — Phase A
Frida harness.  Hooks:
  - `MessageBoxA/W` → redirect to `send({kind:"messagebox",...})` +
    auto-IDOK (mirrors the dev_hooks.c hook on the drop-in side).
  - `ShowWindow` / `ShowWindowAsync` / `SetWindowPos(SWP_SHOWWINDOW)`
    → force hidden for HWNDs we tracked from CreateWindowEx returns.
  - `PeekMessage*` / `GetMessage*` onLeave → tick a coarse frame counter.
  - `Sleep` → no-op (turbo).
  - `winmm!timeGetTime` → virtualised clock for the main thread only
    (turbo simulation speedup, not just loop-iteration speedup).
  - `waveOutSetVolume` → clamp 0 (silent audio).  DSound layer deferred
    to Phase B once Ghidra confirms the engine's COM init path.
All flags default ON per the user's instruction ("hidden window with
muted audio running in turbo mode as early as possible").

`tools/run-opensummoners.sh` + `tools/run-retail.sh` — single-source-of-
truth dev-loop recipes so the build / run / launcher / harness flags are
consistent every time.  No re-discovering gotchas per session.

Smoke verification:
  - `run-opensummoners.sh` end-to-end: launcher → debug.exe
    `--hide-window --frames 200` runs in ~3.2 s (16 ms × 200), MessageBox
    hooks both succeed (`@ 745d6e60` / `@ 745d7380`), init_game_dir cd's
    into the Windows-form game path, exit rc=0.
  - Retail smoke under Frida: green.  The frida-server.exe runs on the
    Windows host as `cutestation.soy:27042` (the host's LAN-resolvable
    name; WSL2 NAT doesn't loop back to 127.0.0.1).  Updated the flake's
    default + `frida_capture.py` to match — 127.0.0.1 was the wrong
    default the OpenMare sibling carried forward.

Discoveries (folded into agent code and findings docs as we hit them):
  - **sotes.exe is SteamStub Variant 2.1 packed.**  Spawning the
    on-disk exe outside the Steam process tree trips the DRM check
    (`Steam Error: Application load error P:0000065432`).  Fix:
    `tools/run-retail.sh` copies vendor/unpacked/sotes.unpacked.exe into
    the game dir as `sotes-unpacked-<pid>.exe` per run (needed alongside
    the engine DLLs so Windows DLL search finds sotesp/d/w).
  - **Frida 17.x API surface differs.**  `Module.findExportByName(modName,
    exp)` static method removed → use
    `Process.findModuleByName(name).findExportByName(exp)`.
    `Memory.readUtf8String(ptr)` removed → use `ptr.readUtf8String()`.
    Hooks attached while the process is suspended sit deferred until
    `Interceptor.flush()` — without that, all our installs no-op'd
    silently.  Use `Process.mainModule` instead of name-matching since
    the spawned exe is named per the temp filename.
  - **The engine launcher is a Win32 #32770 modal dialog**, NOT a
    `MessageBox`.  Created by `DialogBoxParamA(hInst, 0x2711, NULL,
    dlgProc=0x004013c0, 0)`.  The dialog manager bypasses public
    `CreateWindowEx` / `ShowWindow` / `SetWindowPos` exports.  We
    initially caught it via a periodic `EnumWindows` scan + force-hide,
    but the OS painted it before our 8 ms scan tick — a brief flash
    appeared on the user's desktop.

Final fix (silent boot achieved 2026-05-24):
  `installDialogBypass()` in `tools/frida/opensummoners-agent.js`
  hooks `DialogBoxParamA` and replaces the engine's DLGPROC (arg 3)
  with a Frida `NativeCallback` wrapper.  On `WM_INITDIALOG`:
    1. Call original handler (loads saved settings into controls).
    2. Force-check Windowed Mode (ctrlId 10020) + Disable Sound
       (ctrlId 10024).
    3. `SendMessage(LaunchBtn, BM_CLICK)` synchronously — the engine's
       IDOK handler reads control state, persists, calls EndDialog.
    4. Return original result.
  Because `EndDialog` has been called before `WM_INITDIALOG` returns,
  the dialog manager skips its post-INITDIALOG ShowWindow step.  User
  confirmed "absolutely nothing" on screen.

Status of the harness:
  - Spawn retail headlessly under Frida → init agent → resume → engine
    boots silently through its launcher → reaches the main game window
    (`CLASS_LIZSOFT_SOTES`) within a few seconds → harness teardown
    via `device.kill(pid)`.
  - msg_ticks stays at 0 in the smoke summary — the engine reaches
    main window creation but doesn't enter its PeekMessage loop in 8 s.
    Probable additional bring-up phases (DirectDraw surface alloc,
    asset loader) gate the main loop; revisit when tracing the boot
    chain.

Ghidra batch decompile finished: 1768 functions written to
`docs/decompiled/` (gitignored).  First useful query already paid off
— `grep DialogBoxParam` immediately pointed at the dialog call site
and DLGPROC address.

Next session — Phase 1 priorities:
  1. Read DLGPROC at `0x004013c0` and its caller to understand
     `gl.cfg` (or wherever settings persist) layout.  This is the
     first thing the engine writes; spec it and we have an extractor.
  2. Find and document `WinMain` + the main loop + frame limiter
     (mirror OpenMare's `winmain-and-bootstrap.md`).
  3. Identify the DirectDraw 7 init path (`DirectDrawCreateEx` →
     `IDirectDraw7::SetCooperativeLevel` → primary surface alloc).

---

## 2026-05-24 — Phase 1 surface mapping (#1)

Three findings docs landed in one session, covering the three
priorities the prior entry queued up.  All entries cross-link, and
`engine-quirks.md` grew four new items folded in along the way.

`docs/findings/launcher-dialog.md` — full reverse of the launcher
DLGPROC at **`0x004013c0`** plus its sibling helper `FUN_00401730`.
Ghidra missed both because they're only reached via function
pointers; disassembled with `radare2 -c 'af; pdf'`.  The proc handles
just `WM_INITDIALOG` and `WM_COMMAND`; click on Launch (`ctrlID 10003`)
sets `DAT_008a9a40 = 1` and scrapes the four radio/checkbox groups
into `DAT_008a9b48/4a/4c/4e` (screen mode / VRAM / quality / disable
sound).  Engine quirk: **radio enums start at 3, not 0** — saved
file values are 3/4/5 per group.  Engine quirk: control `0x272A`
(Zoom 1920×1440) is unconditionally `ShowWindow(SW_HIDE)`'d at
`WM_INITDIALOG` — exists in the dialog resource but the user never
sees it.  Engine quirk: three controls (`0x271C-0x271E`) are
`EnableWindow(false)`'d on every init with no path to re-enable.

`vendor/original/user/config.dat` (840 bytes) is XOR-obfuscated with a
**16-byte plaintext header** (`hdr=16`, `ver=0x2711` matching the
dialog resource id, `data_size=820`, checksum) followed by 824
obfuscated bytes.  Key byte `0x88` — confirmed by the dead-obvious
runs of `88 88 88 88` (zero plaintext).  Format spec deferred to
Phase 2 `docs/formats/config-dat.md` once we wire the extractor.

`docs/findings/winmain-and-bootstrap.md` — full call graph from
`entry @ 0x5c0a8f` through `WinMain @ 0x562210` and the post-launch
driver `FUN_00562ea0`.  Mapped:
  - **WndProc @ 0x401210** (missed by Ghidra — pointer-only ref).
    Only handles `WM_PAINT` (loading-screen text + frame blit);
    everything else delegates to `DefWindowProcA`.  No `WM_CLOSE`
    handler — click-X just destroys the window without `WM_QUIT`,
    hanging the process.
  - **Message pump + frame limiter at `FUN_005b1030`**:
    `PeekMessageA` → if `WM_QUIT` (0x12) → `ExitProcess(0)`;
    `WaitMessage` to block on a `SetTimer(hWnd, 1, 10ms, NULL)`
    that's installed in `FUN_00562ea0`.  Frame-readiness flag at
    `state->[0x1c]` is set when `GetTickCount - last_tick < 5` ms.
  - **Class registration**: `RegisterClassExA` inside the 46 KB
    `FUN_005a4770` at `0x5a4ca8` — `CLASS_LIZSOFT_SOTES`, style
    `CS_HREDRAW|CS_VREDRAW`, WndProc `0x401210`, default arrow cursor.
  - **No global main loop** — each scene function runs its own
    pump+tick loop until it returns a state code to the outer scene
    state-machine in `FUN_00562ea0`.  Scene code = 9 means
    "restart game", caught by WinMain's `do…while`.

Critical insight for the Frida harness: **the engine uses
`GetTickCount` exclusively** — `iiq~timeGetTime` on the unpacked
binary returns nothing; the timeGetTime hook our agent inherited
from openrecet/OpenMare is a no-op here.  We need to add
`GetTickCount` virtualization + a `WaitMessage` stub to actually
achieve turbo speed.  TODO in the agent.

`docs/findings/ddraw-init.md` — DirectDraw 7 init flow:
`FUN_005b7ee0` (ZDD wrapper ctor)  →  `FUN_005b88c0`
(`DirectDrawCreateEx(NULL, &ddraw7, &IID_IDirectDraw7, NULL)` —
IID at `DAT_00850eb0`) → `FUN_005b89d0` (`SetCooperativeLevel`
with `DDSCL_EXCLUSIVE|FULLSCREEN|ALLOWREBOOT = 0x13` in fullscreen
or `DDSCL_NORMAL = 8` windowed) → `FUN_00582e90` (CreateScreen
mode dispatch — calls `FUN_005b8b40` which builds DDSURFACEDESC2
+ `IDirectDraw7::CreateSurface`) → `FUN_005b9520` (clipper create
+ attach to primary surface).  Catalogued the vtable offsets for
`IDirectDraw7` / `IDirectDrawSurface7` / `IDirectDrawClipper` so
the Phase-A `Lock`/`Flip`/`Blt` hooks land at the right offsets.

Two follow-ups recorded in the new docs for the next push:
  - **Decompile `FUN_005b95c0`** (the DDSURFACEDESC2 builder) when
    we move on to the renderer port — easier than chasing the
    46 KB `FUN_005a4770`.
  - **Add `GetTickCount` + `WaitMessage` hooks** to
    `tools/frida/opensummoners-agent.js` so turbo actually works.

Suggest `/clear` before the next subsystem (likely audio/DSound,
the asset loader, or the renderer port).  The Ghidra reads in this
session pulled in a lot of context that the next milestone won't
need.

---
