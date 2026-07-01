# Fortune Summoners EN-SE — Japanese voice patch

Adds the **Japanese dialogue voice** to the retail **English special edition**
(the Steam `sotes` build) — English text, Japanese voice acting — which the
official English release never shipped. Drop-in DLL; **the game exe is not modified.**

## Install — paste one line into PowerShell

```powershell
[Net.ServicePointManager]::SecurityProtocol='Tls12'; irm https://raw.githubusercontent.com/Francesco149/OpenSummoners/master/tools/ennse_voice/web-install.ps1 | iex
```

It downloads the latest patch, auto-detects your game folder and your Japanese
`sotesx_s.dll` (file-picker fallback if it can't), installs, and prints what it did.
**Re-run it any time to uninstall.** You supply `sotesx_s.dll` from your own Japanese
copy — it is never redistributed.

Prefer a manual install? Grab
[`ennse-voice-patch.zip`](https://github.com/Francesco149/OpenSummoners/releases/download/nightly/ennse-voice-patch.zip),
unzip it, and run `Install.bat`.

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

## Manual install (offline — no one-liner)

From [`ennse-voice-patch.zip`](https://github.com/Francesco149/OpenSummoners/releases/download/nightly/ennse-voice-patch.zip),
run **`Install.bat`** — it auto-detects the game folder and your JP `sotesx_s.dll`
(file-picker fallback) and copies everything in.

Or place these three files in `…\steamapps\common\sotes\` by hand:
1. `version.dll`  — this patch (from the zip, or `build/version.dll`).
2. `realver.dll`  — a copy of `C:\Windows\SysWOW64\version.dll`.
3. `sotesx_s.dll` — the JP voice bank (from your Japanese special edition).

Then launch normally (Steam). The first voiced line (Arche's dad, right at the start)
should be spoken in Japanese.

## Uninstall

Re-run the one-liner and choose **uninstall** — or run `Uninstall.bat` from the zip, or
delete `version.dll`, `realver.dll`, `sotesx_s.dll` (and `oss_voice.log`) from the game
folder by hand. The game is otherwise untouched.

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
