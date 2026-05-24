# OpenSummoners — Project Plan

> Living document.  Update as decisions change.  Linked from `PROGRESS.md`.
> Last revised: 2026-05-24.

## 1. Goal

Produce a **drop-in replacement** for `sotes.exe` (the main game executable
of *Fortune Summoners: Secret of the Elemental Stone*, Lizsoft 2008 JP /
Carpe Fulgur 2012 EN Steam release) that is **behaviorally
indistinguishable** from the original for a user who owns a legitimate copy
of the game.

Educational reverse-engineering + game preservation.  Not byte-identical
(specific MSVC era / CRT codegen would be a goose chase — see §3).  Not
redistributed.  No piracy enablement.

## 2. Hard constraints

1. **Never** redistribute game assets.  The vendor symlink to the user's
   Steam install, extracted asset bytes, decompiled C, golden screenshots
   produced from the original — all live under `vendor/` or sibling
   gitignored paths.
2. The repo ships only: source code, format specifications (text), test
   harness, build tooling, documentation.  Anyone cloning the repo gets
   nothing they couldn't write themselves.
3. **Behavioural fidelity > byte fidelity.**  Save-file compatibility is
   required; pixel-perfect rendering is the default goal but tiny
   GDI/DDraw rasterization deltas are tolerated.
4. **Faithfully reproduce engine quirks** — load-bearing bugs, hand-rolled
   hash variants, weird off-by-ones — rather than "fixing" them in our
   port.  See `docs/findings/engine-quirks.md` for the running catalogue.

## 3. Tech stack (decided 2026-05-24)

| concern              | choice                                             |
|----------------------|----------------------------------------------------|
| language             | C (C11)                                            |
| build toolchain      | mingw-w64 i686 cross compiler (32-bit Win32)       |
| graphics             | DirectDraw 7 (engine imports `DDRAW.dll` /         |
|                      | `DirectDrawCreateEx`)                              |
| audio                | DirectSound (engine imports `DSOUND.dll` /         |
|                      | `DirectSoundCreate`)                               |
| input                | DirectInput (engine imports `DINPUT.dll` /         |
|                      | `DirectInputCreateEx`)                             |
| windowing / system   | Win32 GDI (`GDI32.dll`, `USER32.dll`), winmm,      |
|                      | ole32 for COM, VERSION for file-info reads         |
| portability          | phase 1: Win32 only; phase 2: abstract backend     |
| license              | MIT                                                |

Why C: matches Ghidra's decompiler output verbatim, minimal translation
friction.  Can always uplift later.

Why DDraw direct rather than wrapping to a modern API: the engine calls
DDraw7.  Wrapping DDraw-for-DDraw makes the test harness simple — same
lock path, same surface format, simple frame diffs.  SDL2 / DXGI / etc.
would force us to tolerate rasterization differences from day one.  Phase 6
abstracts this; until then we link the DDraw runtime the Windows host
already provides.

Why we **don't** chase byte-equivalence: the original was built with a
2012-era MSVC; we'd need to dig out the exact CRT/STL versions and PDB
strip pass to even attempt it.  The drop-in test is "does the Steam
install boot and play identically when this binary is in `sotes.exe`'s
place", not "does `fc /b` print zero differences".

## 4. Target binary — what we know

The on-disk `sotes.exe`:

- 32-bit PE, Machine 0x014C, Subsystem 0x0002 (Windows GUI).
- 5 sections; preferred ImageBase `0x00400000`.
- TimeDateStamp `Fri Jan 27 02:04:32 2012` UTC — the Steam (EN) release.
- **Steam DRM (SteamStub Variant 2.1).**  Entry-point RVA `0x03d0f2ee`
  lands inside the `.bind` section appended by the SteamStub wrapper.
  `tools/setup.sh` detects this (objdump section check) and runs Steamless
  (`Steamless.CLI.exe`) to produce `vendor/unpacked/sotes.unpacked.exe`,
  whose entry point relocates to `0x004c0a8f` inside `.text` — clean MSVC
  PE that Ghidra can decompile directly.

`vendor/unpacked/sotes.unpacked.exe` sections (post-Steamless):

| section  | RVA range            | size       | role           |
|----------|----------------------|------------|----------------|
| `.text`  | 0x401000 – 0x5cb795  | ~1.85 MB   | code           |
| `.rdata` | 0x5cc000 – 0x853536  | ~2.53 MB   | r/o data       |
| `.data`  | 0x854000 – 0x8a7000  | ~0.32 MB   | r/w data       |
| `.rsrc`  | 0x8ac000 – 0x410e588 | ~56 MB     | PE resources   |

The bulk of the .exe size lives in `.rsrc` — likely a Win32-resource-backed
asset archive (sprites? music? scenario data?).  TBD which.

DLL imports (verified on disk, post-Steamless):

| dll            | role                                                     |
|----------------|----------------------------------------------------------|
| `DDRAW.dll`    | DirectDraw 7 — rendering (`DirectDrawCreateEx`)           |
| `DSOUND.dll`   | DirectSound — audio                                       |
| `DINPUT.dll`   | DirectInput — keyboard/gamepad input                      |
| `WINMM.dll`    | Multimedia timer + `mmioRead` (likely WAV/MIDI loading)   |
| `GDI32.dll`    | GDI palette / blits (`GetSystemPaletteEntries`)           |
| `USER32.dll`   | window / message pump (`DefWindowProcA`, …)               |
| `KERNEL32.dll` | filesystem + threads + memory                              |
| `ole32.dll`    | COM (`CoCreateInstance`) — likely for DSound 8 / DInput 8 |
| `VERSION.dll`  | file-version reads (`GetFileVersionInfoA`)                |

Companion binaries (in the game dir alongside `sotes.exe`):

| binary             | role                                                |
|--------------------|-----------------------------------------------------|
| `sotesd.dll`       | 168 MB — likely a data archive disguised as a DLL   |
| `sotesw.dll`       | 82 MB — likely a data archive disguised as a DLL    |
| `sotesp.dll`       | 1.1 MB — small enough to be actual code; TBD        |
| `lizsoft.spl`      | 40 KB — Lizsoft proprietary format; TBD             |
| `html/`            | game-help HTML files                                |
| `readme-manual.htm`| user-facing manual                                  |

## 5. Phased roadmap

### Phase 0 — Bootstrap (THIS PHASE)
- [x] Decisions: language, target, license, layout.
- [x] Flake with full toolchain (mingw32 + Ghidra + Frida + Python tooling).
- [x] `tools/setup.sh` — symlinks Steam install into `vendor/`, detects
      Steam DRM, runs Steamless, hashes key files.
- [x] `tools/launcher/opensummoners-launcher.exe` — Job-Object supervisor
      so no spawned .exe ever orphans.
- [x] `src/main.c` skeleton — WinMain, window class, PeekMessage loop,
      MessageBox redirect, single-instance mutex, auto-cd into the game
      dir.  Builds two outputs (GUI + console subsystem).
- [x] `tools/ghidra-headless.sh` — batch decompile
      `vendor/unpacked/sotes.unpacked.exe` to `docs/decompiled/`.
- [x] `tools/frida_capture.py` + `tools/frida/opensummoners-agent.js`
      Phase A — MessageBox redirect, hide-window, turbo, silent-audio,
      message-pump counter.
- [x] `tools/run-opensummoners.sh` and `tools/run-retail.sh` — canonical
      dev-loop recipes.
- [ ] First findings doc — `docs/findings/winmain-and-bootstrap.md` after
      the first Ghidra pass + Frida boot-trace.

### Phase 1 — Surface mapping
Goal: understand the binary's overall structure without writing
replacement code yet.

- Run Ghidra auto-analysis.  Export full decompiled C tree to
  `docs/decompiled/` (gitignored).
- Identify entry point, `WinMain`, the main loop, the message pump.
- Enumerate imported DLLs; confirm DDraw / DSound / DInput version flavor
  (likely 7-era).
- Map high-level subsystems: window/init, input, asset loader (the
  `.rsrc` resource path and any `sotesd.dll` / `sotesw.dll` reads),
  renderer, audio, scripting/event loop, save system.
- Document each subsystem under `docs/findings/<subsystem>.md`.

### Phase 2 — File format extractors
Goal: parse every game asset format from outside the engine.  Each
extractor is independent code + a spec doc + tests against the user's
files.

Priority order (informed by what the binary reads first):

1. **PE resource directory** (`sotes.exe.rsrc`) — wrestool / icotool can
   already dump.  Spec the on-disk layout; identify what's image vs
   audio vs scenario.
2. **`sotesd.dll` / `sotesw.dll`** — likely asset archives.  TBD layout.
3. **`sotesp.dll`** — likely supplementary code; TBD.
4. **`lizsoft.spl`** — Lizsoft proprietary; TBD.
5. **Save files** (location TBD — likely under `%APPDATA%`).

Each format gets a `docs/formats/<name>.md` spec and
`tools/extract/<name>.py`.

### Phase 3 — Harness everywhere
Goal: a Frida-driven harness that spins retail up headless, exercises any
boot path or scenario, and produces bit-exact ground-truth captures
(frames + audio events + simulation state) for any scenario.  **This is
the foundation that everything else hangs off.**

- Frida agent against `sotes.exe` with the openrecet/OpenMare pattern
  already started in Phase 0.
- DDraw `IDirectDrawSurface7::Lock` / `Blt` hook to grab the front-buffer
  contents as a normalised 32-bit BGRA payload per `Flip`.
- Pure-fn differential test infrastructure (`NativeFunction` over Frida)
  for anything we port — RNG, hash, decoder, simulator step.
- Scenario format (yaml + sparse JSONL input trace + golden BMPs + audio
  event log).  Same shape as OpenMare's so the same downstream
  scenario-test pipeline can consume both projects' outputs.
- TAS bot — drive scripted gameplay via input-trace replay, capture
  per-frame, frame-diff against golden, fail-fast on mismatch with a
  red-tint overlay.

### Phase 4 — Drop-in renderer + input + audio
Goal: replace `sotes.exe` with `opensummoners.exe` that boots into the
title screen, accepts input, plays sound effects + BGM.

### Phase 5 — Gameplay loop fidelity
Goal: the drop-in plays through Chiffon's tutorial dungeon
indistinguishably from retail.

### Phase 6 — Source-port abstraction
Optional, post-fidelity.  Abstract the DDraw / DSound / DInput backends so
the engine can target SDL2 / DXGI / etc.

## 6. Where to look first (Phase 1 reading list)

When the first Ghidra pass completes, prioritize understanding:

- `WinMain` and the immediate boot chain — `RegisterClass`,
  `CreateWindowEx`, `DirectDrawCreateEx`, `DirectSoundCreate`,
  `DirectInputCreateEx`.
- The main message loop and frame limiter.
- Asset loader — how the engine pulls bytes from `.rsrc` and the
  oversized DLL companions.

## 7. Subagent policy (carry-forward)

**Subagents are forbidden by default** for this RE project.  The only
carve-outs are pure mechanical batch tasks with no judgment surface.
Anything that involves reading Ghidra output, deciding what to port next,
or chasing a bug stays inline.

## 8. Sibling projects

| repo                       | game / engine                    | role                                    |
|----------------------------|----------------------------------|-----------------------------------------|
| `/opt/src/openrecet`       | Recettear (EasyGameStation 2007) | first project; reference for conventions|
| `/opt/src/OpenMare`        | Patrician 3 (Ascaron 2003)       | refined workflow; closer model to copy  |
| **OpenSummoners** (this)   | Fortune Summoners (Lizsoft 2008) | third in the series                      |

Cross-reference findings across the three when in doubt; the three games
all hit DirectDraw / DirectSound / DirectInput, so a quirk in one is
often a quirk in another.
