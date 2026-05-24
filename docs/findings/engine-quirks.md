# Engine quirks

A running collection of small things the original Fortune Summoners
engine does that are weird, charming, or inexplicable.  Each entry is
short — one paragraph or two, optional code snippet, source pointer.

> See `docs/AGENT-WORKFLOW.md` for the convention.  Add an entry whenever
> something makes you blink at the decompiler output.  Bias toward
> recording it before you forget *why* it surprised you.

---

## 1. The bulk of the .exe lives in `.rsrc`, not `.text`

`sotes.exe` is 64 MB on disk, but `.text` is only 1.85 MB.  56 MB is in
`.rsrc` — the PE resource directory.  This is an unusual choice for a
2012-era Windows game; most contemporaries shipped assets in separate
archive files (`.pak`, `.dat`, `.cpr`) and kept the .exe lean.

Lizsoft may have done this to keep all assets in a single tamper-proof
file (the Steam DRM wrap *only* covers the `.text` section; the
resource directory's bytes survive Steamless unmodified, so a comparison
of pre- and post-Steamless sotes.exe is a fast way to confirm where the
asset payload lives).

Possibly relevant: `sotesd.dll` (168 MB) and `sotesw.dll` (82 MB) are
also disproportionately large for code; they may be additional
resource-style asset bundles.  TBD which is which until we look at the
imports of both DLLs and the `LoadLibrary` calls in `sotes.exe`.

> 📍 See `docs/PLAN.md` §4 for the section table.  Confirm by extracting
> .rsrc with `wrestool -x vendor/original/sotes.exe` once we set up an
> extractor.

---

## 3. The startup launcher is a Win32 dialog with class `#32770`

Before sotes.exe ever reaches `WinMain`'s main-window creation, it pops
a modal Win32 dialog ("Fortune Summoners Ver1.2 - Product Ver. -",
class `#32770` = the standard system dialog class) with:

| ctrlID | text                                     |
|--------|------------------------------------------|
| 10019  | "Screen Mode" (group label)              |
| 10020  | "Windowed Mode" (radio)                  |
| 10022  | "Fullscreen Mode" (radio)                |
| 10025  | "Zoom Mode(1280x960)" (radio)            |
| 10026  | "Zoom Mode(1920x1440)" (radio)           |
| 10021  | "Graphics Quality" (group label)         |
| 10006  | "High" (radio)                           |
| 10007  | "Medium" (radio)                         |
| 10008  | "Low" (radio)                            |
| 10018  | "VRAM Use" (group label)                 |
| 10015  | "Minimal" (radio)                        |
| 10016  | "Normal Use" (radio)                     |
| 10017  | "Use All" (radio)                        |
| 10023  | (warning text about VRAM + high quality) |
| 10024  | "Disable Sound" (checkbox)               |
| 10003  | "Launch" (button)                        |

Most engines of this era ship a separate `*config.exe` for this; here it's
baked into the main exe and gates the actual game launch.  This means
the harness can't reach any engine code without first dismissing the
dialog — see `tools/frida/opensummoners-agent.js`
`installPeriodicWindowScan` for the auto-handler that EnumChildWindows
the dialog, ticks "Disable Sound", and PostMessages `BM_CLICK` to the
Launch button.

The dialog appears even on the Steamless-unpacked exe — it's not part of
the DRM layer.  It's the engine's own first-run-style config dialog,
shown every startup, with selections persisted somewhere (TBD which file).

> 📍 First captured 2026-05-24.  See PROGRESS.md for the dialog-bypass
> harness work and `tools/frida/opensummoners-agent.js` for the
> implementation.

---

## 4. Main game window class is `CLASS_LIZSOFT_SOTES`

Once the launcher dialog is dismissed, the engine creates its main game
window with class name `CLASS_LIZSOFT_SOTES` — a Lizsoft-specific class
they apparently use across their products.  Title matches the dialog
("Fortune Summoners Ver1.2 - Product Ver. -") which means a window-
class-only filter (without a title check) would be the safest way to
distinguish "the game window" from "the launcher dialog" in our
harness.

> 📍 Captured 2026-05-24.  See the agent's `hwnd_seen` events.

---

## 2. Steam DRM is SteamStub Variant 2.1 — Steamless unpacks cleanly

The on-disk `sotes.exe` is wrapped with SteamStub Variant 2.1 (the
classic 2011-2013 era variant Lizsoft would have had access to when they
went up on Steam in Jan 2012).  `Steamless.CLI.exe` identifies the
variant on first sight and reconstructs the original entry point and
code section without any manual help: original entry RVA `0x03d0f2ee`
inside the appended `.bind` section becomes `0x004c0a8f` inside `.text`
after unpacking.

The Steam DRM only covers the `.text` section.  Post-Steamless, all
non-code sections (`.rdata`, `.data`, `.rsrc`) byte-match the pre-Steamless
file.

> 📍 `tools/setup.sh` does the detection (objdump section list) +
> unpacking.  See `docs/PROGRESS.md` 2026-05-24 for the SHAs.

---

## 5. `Zoom Mode(1920x1440)` is in the dialog resource but unconditionally hidden

Control `0x272A` (10026) — "Zoom Mode(1920x1440)" — exists in the launcher
dialog resource (§3 enumerates it) but the DLGPROC calls
`ShowWindow(GetDlgItem(hDlg, 0x272A), SW_HIDE)` on every WM_INITDIALOG.
The scrape path at `0x00401424+` doesn't read its state either.

So Zoom mode in this build means only 1280x960, despite the .rc suggesting
two choices.  Best guess: 1920x1440 was either too expensive on 2012-era
hardware or never finished; the dev took the toggle out of the UX without
ripping the control out of the resource file.

> 📍 See `docs/findings/launcher-dialog.md` for the WM_INITDIALOG flow.

---

## 6. Launcher radio enums start at 3, not 0

Every saved radio in the launcher dialog uses **3 / 4 / 5** as the three
possible enum values; the "Disable Sound" checkbox writes 3 or 4 (not
0/1).  The same +3 offset appears in `config.dat`'s on-disk layout — when
we ship our extractor + writer, we have to faithfully reproduce this
offset.  Looks like the enum was re-numbered at some point and they
didn't rebase to zero.

> 📍 `DAT_008a9b48/4a/4c/4e` in `docs/findings/launcher-dialog.md`.

---

## 7. Three launcher controls are permanently EnableWindow(false)'d

IDs `0x271C / 0x271D / 0x271E` (10012/10013/10014) are greyed out at every
`WM_INITDIALOG` with no path to ever re-enable them.  Visible but
non-interactive in the dialog — probably correspond to abandoned options.

> 📍 `docs/findings/launcher-dialog.md` §"WM_INITDIALOG".

---

## 8. `config.dat` is XOR-obfuscated with a plaintext header

`vendor/original/user/config.dat` (840 bytes) carries a 16-byte plaintext
header followed by 824 bytes of XOR-obfuscated body.  The key byte 0x88
is dead obvious because runs of zero plaintext decode to `88 88 88 88`
sequences in the file, and the file is *full* of them.

```
000000 10 00 00 00 11 27 00 00 34 03 00 00 db 56 00 00
       [hdr=16   ][ver=0x2711][datsz=820][checksum   ]
000010 34 51 bc 3d 8c 88 88 88 f5 05 63 88 8a e3 67 88 …obfuscated…
```

`0x2711` is the `DialogBoxParamA` resource ID for the launcher dialog —
nice cross-confirmation that this file is "the launcher's settings".
Format details TBD; defer to Phase 2's `docs/formats/config-dat.md`
when we write the extractor.

> 📍 Hex peek: `od -A x -t x1z -v vendor/original/user/config.dat`.
