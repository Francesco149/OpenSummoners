# The standalone — the port running alone (2026-10-23)

**What:** Fortune Summoners played by the C port with NO retail code. The user's own `sotes.exe` + asset DLLs supply
DATA only (strings, tables, maps, sprites, sound); every instruction executed is ours (`src/` + the host). Proof
tooling below. Edition: EN-old (2012) only.

## Two binaries, one host (`tools/rhost/`)
| binary | what | use |
|---|---|---|
| `build/rhost/opensummoners-standalone.exe` | **the product**: `rhost.c` built `-DRH_PRODUCT` → always `--standalone --live`; no code path that maps retail code (`retail_map.h`: the copy loop for code sections is compiled out); GUI subsystem | play it: double-click |
| `build/rhost/rhost.exe --standalone` | the same port, rhost's **deterministic** platform (virtual clock, trace keyboard, heap arena, file overlay) | the corpus gate: replay `tests/scenarios/rh-*` with zero retail code |

Flags (both): `--standalone` (no retail code), `--live` (real platform), `--no-launcher` (skip the launcher dialog:
Windowed + Launch, as every rhost run). rhost's pure / hybrid modes are unchanged (rhost.py `run/corpus/verify` as before)
— the verification loop for new traces stays retail-hosted.

## How the image is built (rhost.c `main`, `retail_map.h`)
1. `retail_copy_data`: the PE headers + every section WITHOUT `IMAGE_SCN_CNT_CODE | MEM_EXECUTE` copied to its VA
   (0x400000-based: port code addresses globals by retail VA, `g_eng.data` = identity); every code section filled
   0xCC (INT3). sotes.exe: `.rdata` `.data` `.rsrc` copied, `.text` never. Each range is noted for the audit.
2. Asset DLLs (`sotesd/sotesw/sotesp.dll`): `LoadLibraryA` → the same copy rule from a read-only file view at the
   fixed 16 MB slot (0x80000000..) — NOT LoadLibrary (they carry a DllMain + CRT code). HMODULE = the slot base;
   FindResource / LoadResource work on it as on 0x400000.
3. `rh_hooks_install`: a 5-byte `jmp rel32` at the entry of every function in the hook set (`g_sa_spec` = rhost.py
   `standalone_hooks()` = the hybrid set + the CRT, embedded by build.sh) to its wrapper (retail calling convention →
   port C function). Calls by address (window/dialog procs, atexit, vector-iterator callbacks), engine-call slots and
   decomp2c `RC_` calls all land there. Anything else in the code range = INT3 → `sa_veh`: "the game reached retail
   code at VA (no port for it yet)", stop (message box when live).
4. The IAT (head of `.rdata`) resolved to the real system DLLs; rhost's overrides on top (deterministic mode: all;
   live: module handle 0x400000, resource redirect, asset loader, the clean command line, ExitProcess, launcher fallback).
5. `sa_seal`: data pages RW (never executable), the code range RX; `SetProcessDEPPolicy(ENABLE)` (data execution →
   AV → "execution of mapped game DATA"). Then the AUDIT (below); a failed audit refuses to run.
6. Entry: `crt_start` (src/msvc_crt.c — retail `start` 0x5c0a8f's port) on rhost's fixed stack: x87 precision 53-bit,
   heap, the C++ static initializers (retail's table 0x854000..0x854040, through their VAs → thunks), WinMain 0x562210.

**The port CRT** (`src/msvc_crt.{c,h}`): the 16 entry points the game calls (new / delete, atexit / exit, fopen /
fclose / fprintf / fscanf / fseek, time / localtime / strftime, setlocale, srand, sprintf, _itoa) — ported from
retail's LIBC functions, every OS effect through retail's import slots (so the deterministic platform stays under
it), observable CRT state at retail's VAs (tz globals, the static `struct tm` 0x8a9d08, day tables, "C" lc_time).
Not reproduced: the heap's block placement (PORT-DEBT(crt-heap)) and the stream buffering (PORT-DEBT(crt-stdio)).

**Stock Steam exe works:** SteamStub (`.bind`) encrypts only `.text`; `.rdata/.data/.rsrc` + the import directory are
byte-identical to the Steamless output (verified 2026-10-23), so data-only mode reads the packed `sotes.exe` as is —
`RETAIL_PACKED` restores the header's SizeOfImage / entry in the in-memory copy. The product looks for
`sotes.unpacked.exe`, then `sotes.exe`, in its own folder (or `--game DIR --exe FILE`).

## The proof — "we aren't copying code"
Runtime (every run): `<rundir>\standalone_audit.txt` (product: `<game>\opensummoners-standalone\`), written after the
seal, before the first port instruction:
- `range …` — every byte range taken from each file: `copied` only for data sections; code sections `CODE-not-copied`
- `thunk VA -> target name` — all 1,360-ish thunks
- `code_range … other 0` — every byte of every not-copied section is 0xCC except the thunks
- `retail_text_windows_compared N identical 0` — no 16-byte window of retail's own `.text` (re-read from the file)
  is present in memory (all-INT3 windows aside)
- `protect …` / `exec_outside_code_range 0` — VirtualQuery over the image: executable = the code range only
- `dep set=1 flags=1 permanent=1`, `verdict OK`

Static (`nix develop --command python3 tools/standalone/prove_no_retail_code.py [--audit RUN/standalone_audit.txt]`):
1. image — the copy rule takes no code: no IDA function (of 1,824) lies in a copied section; copied sections are
   non-executable; `55 8B EC` prologue density .text 66/MB vs 0.0 in .rdata/.data/.rsrc; the DLLs likewise
2. binary — our exes hold no retail code: every byte span shared with retail `.text` (seeded by 32-byte windows)
   over retail FUNCTION bytes is < 128 B and covers no whole retail function; the spans (≤ 73 B) are listed by port
   function — the same C compiled to the same encoding (runs of `mov [reg+disp32], imm32` in the character type inits,
   field tests).  Spans over .text bytes outside every function = library data, reported apart (one: 524 B of
   DirectInput's joystick object array, which both exes take from the SDK)
3. source — no port code reads `.text` bytes as data (code-range `D(va)` only at function entries = callbacks by
   address); every retail entry the port calls or passes by address is a thunk
4. audit — re-checks a run's audit file (verdict, ranges, INT3, identical windows, exec, thunk entries vs IDA)

## Verified (2026-10-23)
`rhost.py corpus --check --standalone` (EN-old scenarios; the SE one skipped): every scenario matches PURE RETAIL in
frame / tick / RNG / **every presented frame's pixels** (`fb` chain; wine also the 600-frame `fb_series`) — Windows
9/9, wine 9/9 (~240k frames). State hashes are not compared (crt-heap: block addresses differ). Live mode (product
exe, the STOCK packed sotes.exe, real DirectInput / DirectSound / window, `--no-launcher`) under headless wine: boots
to the title menu (shots), audit OK incl. `.bind`.

## Playing it
Windows box: `C:\OpenSummoners\` (the user's game files from the Steam folder + the exe) and the desktop / Start Menu
shortcut "OpenSummoners (standalone)" — `tools/standalone/install-play.sh` (re-run after a rebuild to refresh the exe;
never overwrites game files or saves). Log + audit: `C:\OpenSummoners\opensummoners-standalone\`. Elsewhere: copy
`opensummoners-standalone.exe` into any folder holding sotes.exe (stock or unpacked) + the three DLLs. Verified on the user's
Windows 11 (2026-10-24, desktop session via the oss-rhost runner, real DirectInput / DirectDraw, DirectSound created
but the tests ran `--silent`): windowed and fullscreen boot to the title menu (shots); the real launcher dialog (the
user's own launch got past it).  Not yet exercised: audible DirectSound / DirectShow BGM over a long play, window
activation edge cases.

## The real platform (`--live`): what the host adds on Windows (2026-10-24)
The game's HINSTANCE is the hand-mapped image 0x400000 (`GetModuleHandleA(NULL)` → 0x400000: its resources live there).
The loader doesn't know that image as a module; resource loads (LoadIconA, the launcher's DialogBoxParamA, FindResourceA)
work on it as is, but:
- **DirectInput** validates the instance (`GetModuleFileName`) → 0x80070126 → retail's exit "It failed in the
  initialization of ZDI" (0x562ea0) — the user's first launch.  wine's dinput doesn't check, so the wine live test
  passed.  `live_DirectInputCreateEx` hands it the host exe's own handle.
- **Fullscreen** fails as retail's does on Windows 11 (quirk #177: `SetDisplayMode(640,480,16)` DDERR_UNSUPPORTED →
  "It failed in the display mode setting").  The product relaunches itself once under `__COMPAT_LAYER=
  DWM8And16BitMitigation` (appended to any layers already set; the parent waits and returns the child's exit code: two
  processes in Task Manager).  `--no-compat-layer`: no relaunch.  The child logs `product: __COMPAT_LAYER=…`.
- The game's own diagnostics are mirrored into `rhost.log`: `ods: …` (OutputDebugStringA: the ZDD / ZDI error texts, the
  boot steps) and `MessageBoxA [caption] text`; the real box is still shown.
- **Alt-tab in Fullscreen / Zoom** leaves trails, with retail's code as much as the port's (quirk #179: after the
  round trip, Blts from recreated surfaces fail DDERR_UNSUPPORTED).  Applied, verification pending: the format pin.
  When the game creates a surface without a pixel format while the display's format differs from the primary's, the
  host gives it the primary's format and logs `ddraw: surface WxH pinned …`.  It never fires otherwise (no pin in
  normal runs).  Live mode always logs every `Restore` and the first 12 failing surface calls (`ddlog:` lines);
  `--log-ddraw` adds every surface created (caps, size, format) and raises the cap to 300.
- The launcher's result is logged: `launcher: dialog -> screen … sound … launch …` (screen low word 3 Windowed,
  4 Fullscreen, 5 Zoom — Zoom covers the whole desktop, like retail: quirk #178).
- `--screen N` sets the emulated launcher's Screen Mode (3 Windowed = default, 4 Fullscreen, 5 Zoom 1280x960), so a
  test can drive fullscreen without the dialog: `--no-launcher --screen 4`.
- DEP: on that machine `GetProcessDEPPolicy` reports DEP off and permanent for every 32-bit process, including a
  minimal mingw probe with or without NX_COMPAT (system policy OptIn).  So its audits read `dep set=0 flags=0
  permanent=1`.  The other audit checks don't depend on DEP.

## Gaps the standalone surfaced (closed)
- `0x401000 → 0x401010`: the static-init table's FIRST entry (pi/180 into 0x8a9a38) — IDA defines no function there,
  so `coverage --hybrid` never trapped it; it ran as retail code under the hybrid. Ported `si_401000/si_401010`.
- `0x401120` (atexit'd destructor of 0x8a6b50) — exit-time only, never reached by a corpus run. Ported `si_401120`.
- `0x54c5e0` / `0x5851a0` / `0x4017a0` (vector ctor / dtor element callbacks) and `0x4013c0` (the launcher DLGPROC,
  never called under rhost's emulated launcher): ported but without hook rows — reached by address only.
- `ending.c` read MSVC's switch index table `0x49c150` from `.text` (compiler layout): decoded into the switch.
- DirectInput's data formats: retail's `c_dfDIKeyboard` 0x850ec0 / `c_dfDIJoystick` 0x850ef0 (in `.rdata`) point at
  object arrays dinput.lib put INSIDE `.text` (0x5bdae0 / 0x5beb50). rhost's fake DirectInput never read them; real
  DirectInput (live) crashed on 0xCC. The port passes the SDK's own formats (mingw `libdinput`: identical 256 / 44
  objects, compared field by field) — `src/input_dev.c`. The prover's source check now flags any `D(va)` of a
  structure holding a pointer to `.text` bytes outside every function.
- The app teardown `wm_5b10d0` (0x5b10d0, runs at every game exit) took `v1` = the ADDRESS of `dword_8A9B68` where
  retail loads its value (`mov eax, dword_8A9B68`).  So the exit read the box caption / GDI handle off the wrong memory
  and `op_delete`d a global's address.  Visible as the caption "[ Ver]" (retail: "Fortune Summoners Ver1.2").  The
  cause was decomp2c: "pointer local = `dword_X`" always emitted the address; now only when the function's asm takes it
  (`offset`), else the value.  No other instance in `src/` (each `= (int32_t *)D(..)` checked against data_xrefs
  kinds).  The corpus never reached a game exit: `rh-title-exit` (title → Exit, game-exit at frame 762) now does —
  the old line gives a different state hash there under `--hook replace:wm_5b10d0`.
Retail code still reachable but unported = none known; the INT3 trap names any the user's play finds.

## Open
- PORT-DEBT(crt-heap): port MSVC6's small-block heap (0x5c73fd..) faithfully → `--standalone` state hashes = pure
  retail's (then the standalone joins the full corpus gate, not only pixels).
- PORT-DEBT(crt-stdio): buffered streams over retail's `_iob` (only matters with crt-heap: the 4 KB buffers come from
  the heap).
- EN-SE: the hook rows are EN-old VAs (crosswalk later).
