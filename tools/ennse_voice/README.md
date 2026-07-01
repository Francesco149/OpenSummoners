# Fortune Summoners EN-SE — Japanese voice patch

Adds the **Japanese dialogue voice** to the retail **English special edition**
(the Steam `sotes` build) — English text, Japanese voice acting — which the
official English release never shipped. Drop-in DLL; **the game exe is not modified.**

## Download

**➡ [Download `ennse-voice-patch.zip`](https://github.com/Francesco149/OpenSummoners/releases/download/nightly/ennse-voice-patch.zip)** — always the latest build.

Unzip it anywhere and run **`Install.bat`**. You also need `sotesx_s.dll` from your own
Japanese copy — the installer finds it automatically (see [Install](#install) below).

## How it works (short version)

The English engine (`sotes_en.exe`) still contains the *entire* voice subsystem —
the code that plays a voice clip per dialogue line is byte-for-byte identical to the
Japanese engine. The English localizer only removed the **loader** (the one call that
loads `sotesx_s.dll` and remembers its handle). So the play path is all there, just
switched off because two globals are never set.

This patch is a proxy `version.dll`: the game already loads `version.dll`, so ours gets
loaded too, forwards the real version-info functions to a renamed copy of your own
system `version.dll` (`realver.dll`), and — once the audio system is up — calls the
engine's *own* functions to load `sotesx_s.dll` and create the voice manager, setting
those two globals. From there the engine plays the voice on its own, using its own
per-line mapping (the voice-id data is already present in the shared `sotesd.dll`).

## What you need

- The **English special edition** on Steam (`…\steamapps\common\sotes\`, has
  `sotes_en.exe`).
- **`sotesx_s.dll`** — the Japanese voice bank (~253 MB). It ships only with the
  **Japanese special edition**; copy it from your own JP install/CD. (It is not
  redistributed here — you must own it.)

## Install

**Easy:** run `install.bat` (it copies your system `version.dll`→`realver.dll`, drops
in `version.dll`, and copies `sotesx_s.dll` from a JP install if it finds one). Pass the
game folder as an argument if it isn't at the default Steam path.

**Manual** — put these three files in `…\steamapps\common\sotes\`:
1. `version.dll`  — this patch (from `build/version.dll`).
2. `realver.dll`  — a copy of `C:\Windows\SysWOW64\version.dll`.
3. `sotesx_s.dll` — the JP voice bank (from your JP special edition).

Then launch the game normally (Steam). The first voiced line (Arche's dad, right at the
start) should be spoken in Japanese.

## Uninstall

Delete `version.dll`, `realver.dll`, `sotesx_s.dll` (and `oss_voice.log`) from the game
folder — or run `uninstall.bat`. The game is otherwise untouched.

## Troubleshooting

- `oss_voice.log` (written next to the DLL) logs each step — a healthy run ends with
  `[seed] DONE bank=… mgr=…` (both non-zero).
- No voice but no crash → make sure `sotesx_s.dll` is present in the game folder.
- Crash on launch about a missing `version.dll` function → `realver.dll` is missing.

## Note for maintainers

`version_proxy.c` hard-codes addresses for the **current** `sotes_en.exe` build (unpacked
ImageBase `0x400000`; resolved at runtime against the module base, so ASLR/rebase is fine).
If Steam updates the game, re-verify the addresses in `docs/plans/ennse-voice-patch.md`.

Build: `nix develop --command make -C tools/ennse_voice` → `build/version.dll`.
