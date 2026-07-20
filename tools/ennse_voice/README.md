# Fortune Summoners EN-SE — Japanese voice patch

Adds the **Japanese dialogue + deluxe combat voices** to the retail **English special
edition** (the Steam `sotes` build) — English text, Japanese voice acting (both the story
dialogue lines *and* the characters' battle grunts) — which the official English release
never shipped. Drop-in DLL; **the game exe is not modified.**

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

The English engine (`sotes_en.exe`) still contains the *entire* voice subsystem — the
code that plays a voice clip is byte-for-byte identical to the Japanese engine. The
English localizer removed exactly one line: the call that loads `sotesx_s.dll` and
remembers its handle. With that handle left null the engine can't play any voice, so
dialogue is silent and the characters' battle grunts drop to the plain (non-"deluxe") set.

Just re-loading the bank isn't enough, though. The special edition introduced a bug in the
boot code that registers every sound effect: when the voice bank *is* present, it registers
the **deluxe** version of each sound — but enemies have no deluxe variant, so it registers
**nothing** for them and their hit / scream / death SFX go silent. (This is a real engine
bug — even the native Japanese special-edition exe loses monster sounds if the bank is
loaded early. It's why simply dropping the voice DLL in doesn't just work.)

So this patch — a **standalone drop-in `version.dll`**, loaded before the exe's entry point
because the game imports `version.dll` normally, forwarding the real version-info calls to a
renamed copy of your own system DLL (`realver.dll`) — makes two tiny in-memory changes and
nothing else:

1. **Loads `sotesx_s.dll` early**, right before that boot registrar runs, so the characters
   get their deluxe voices back and the engine builds its own voice manager for the dialogue.
2. **Flips one byte** in the registrar so enemies with no deluxe voice keep their normal
   sound instead of being dropped — restoring the monster SFX.

The exe on disk is never touched; both changes live only in the running process's memory.

## What you need

- The **English special edition** on Steam (`…\steamapps\common\sotes\`, has
  `sotes_en.exe`) —
  **[buy it here and support Lizsoft](https://store.steampowered.com/app/1381770/Fortune_Summoners_Special_Edition/)**.
- **`sotesx_s.dll`** — the Japanese voice bank (~253 MB). It ships only with the
  voiced Japanese release (below); copy it from your own JP install/CD. (It is not
  redistributed here — you must own it.)

## Where the voice bank comes from

No Steam edition ships the dialogue voice bank — not even the Japanese language of
the Special Edition. The only sources are the **out-of-print voiced Japanese
releases** by Lizsoft; metadata to hunt one down second-hand:

- **フォーチュンサモナーズ ～アルチェの精霊石～ Deluxe** — the 2009-06-18 boxed
  retail release (publisher ジャングル / Jungle, distr. Access Media International;
  Amazon JP ASIN `B0026EQRVE`). Added professional voice acting for the main story
  scenes: 田村ゆかり (Arche), 植田佳奈 (Sana), 藤村歩 (Stella).
- The Lizsoft **JP retail-CD special edition** (disc dated 2008–2009; installs to
  `C:\Program Files (x86)\lizsoft\FortuneSummoners`) — the build matrix in
  `docs/findings/game-editions-and-voice.md`. Vector's download listing now reads
  取扱い終了 (discontinued), and lizsoft.jp only points at Steam.

Identify the right file: `sotesx_s.dll`, **253,206,528 bytes**, 1,448 `WAVE`
resources (PCM 22050 Hz 16-bit mono), SHA-256
`ad15512c6810caeb877de5c6237439e53df3b72c04898d5115d7a6e73762b0a6`.
(Browse/play it with the repo's [resource explorer](../res_explorer/README.md).)

## Exact patch target (for future game updates)

The patch seeds engine globals by address, so it is built against ONE exe build.
Verified against:

| field | value |
|---|---|
| game | Fortune Summoners Special Edition (Steam app `1381770`) |
| Steam buildid | `23890965` |
| file | `sotes_en.exe`, 72,529,416 bytes |
| SHA-256 | `668f7e1a12e70b36acf60859c3ee34385daa826839cdde3d93f2929a5c51232e` |
| ImageBase | `0x400000` (unpacked; runtime-relocated, ASLR-safe) |

Patched addresses (`ennse_voice.c`): the seed inline-hooks the bank-load wrapper
`0x5d8b10` to set the voice-bank global `0x92af80` early (before the boot registrar), and
flips one byte at `0x59ccce` — the registrar's deluxe-skip branch — so enemy sounds still
register. If Steam ships a new build (different buildid/hash), re-derive these per
`docs/findings/ense-voice-combat-init.md` before trusting the patch.

## Manual install (offline — no one-liner)

From [`ennse-voice-patch.zip`](https://github.com/Francesco149/OpenSummoners/releases/download/nightly/ennse-voice-patch.zip),
run **`Install.bat`** — it auto-detects the game folder and your JP `sotesx_s.dll`
(file-picker fallback) and copies everything in.

Or place these files in `…\steamapps\common\sotes\` by hand:
1. `version.dll`  — this patch (from the zip, or `build/version.dll`).
2. `realver.dll`  — a copy of `C:\Windows\SysWOW64\version.dll`.
3. `sotesx_s.dll` — the JP voice bank (from your Japanese special edition).

Then launch normally (Steam). The first voiced line (Arche's dad, right at the start)
should be spoken in Japanese.

## Uninstall

Re-run the one-liner and choose **uninstall** — or run `Uninstall.bat` from the zip, or
delete `version.dll`, `realver.dll`, `sotesx_s.dll` (and `oss_voice.log`) by hand. That's
a full revert to vanilla. The game is otherwise untouched.

## Troubleshooting

- `oss_voice.log` (written next to the DLL) logs each step — a healthy run shows
  `[reg] deluxe-skip patched…` then `[seed] voice bank -> <non-zero address>`.
- No voice but no crash → make sure `sotesx_s.dll` is present in the game folder.
- Crash on launch about a missing `version.dll` function → `realver.dll` is missing.

## Note for maintainers

`ennse_voice.c` hard-codes addresses for the **current** `sotes_en.exe` build (unpacked
ImageBase `0x400000`; resolved at runtime against the module base, so ASLR/rebase is fine).
The hooks are signature-gated, so a mismatched build fails safe (no patch) rather than
corrupting the game. If Steam updates it, re-verify the addresses in
`docs/findings/ense-voice-combat-init.md`.

Build: `nix develop --command make -C tools/ennse_voice` → `build/version.dll` (the patch).
