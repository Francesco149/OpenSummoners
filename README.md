# OpenSummoners

An open-source reimplementation of **Fortune Summoners: Secret of the
Elemental Stone** (Lizsoft, 2008 JP / Carpe Fulgur, 2012 EN Steam release).

This is an **educational reverse-engineering and game-preservation project**.
The goal is a drop-in replacement for `sotes.exe` that behaves
indistinguishably from the original for a user who owns a legitimate copy of
the game.

**Not distributed:** no game assets, no decompiled binary code, no copyrighted
content of any kind.  You supply your own copy of the game (Steam).

## Status

Active development — see [`docs/STATUS.md`](docs/STATUS.md) for the current front,
[`docs/PLAN.md`](docs/PLAN.md) for the roadmap, and
[`docs/PROGRESS.md`](docs/PROGRESS.md) for the changelog.

## Downloads

Every build is produced by CI and published to **[Releases](../../releases)** — each
ships **no game assets** (everything reads your own legitimately-owned files at runtime):

- **`opensummoners.exe`** — the port. Drop it beside your Fortune Summoners install.
- **`voice_view.exe`** — a SotES **resource explorer**: open a game DLL, list its
  `WAVE`/`DATA`/… resources, and play/export the audio.
- **Japanese voice patch** — restores **Japanese dialogue voice** to the English special
  edition (English text + JP audio), which the official English release never shipped.
  Install by pasting **one line into PowerShell** — it auto-detects your game + your JP
  `sotesx_s.dll` and installs (re-run to uninstall). The one-liner + a manual zip option
  are in [`tools/ennse_voice/README.md`](tools/ennse_voice/README.md).

The rolling **`nightly`** pre-release always carries the latest build; tagged `vX.Y`
releases are cut at milestones.

## Getting started (NixOS / WSL2)

```fish
nix develop
./tools/setup.sh         # symlinks game into vendor/, runs Steamless, hashes binaries
```

This enters a dev shell with the full RE toolchain (Ghidra, radare2, mingw-w64
32-bit cross-compiler, Frida, Pillow/numpy/scikit-image, …) and prepares the
game files for analysis.

`sotes.exe` is **Steam-DRM packed** — the entry point lives in a `.bind`
section appended by the SteamStub wrapper.  `tools/setup.sh` runs
[Steamless](https://github.com/atom0s/Steamless) to produce a clean
`vendor/unpacked/sotes.unpacked.exe` that Ghidra can decompile directly.

## Layout

```
docs/         design notes, file-format specs, progress log, engine quirks
src/          our C reimplementation (cross-compiled with mingw32)
tests/        unit suite + scenario harness (golden BMP + audio/input traces)
tools/        setup, build, extract, capture, contact-sheet, ghidra-headless,
              Frida agent + capture driver, Job-Object launcher
vendor/       game files (gitignored) — symlinked from the user's Steam install
              + Steamless-unpacked sotes
ghidra/       Ghidra projects (gitignored — derived from the original binary)
runs/         test artifacts (gitignored)
```

## Workflow

Heavy use of a Frida-based harness against the retail exe for ground-truth
probing, with hidden-window + turbo + silent-audio defaults so a harness run
never disrupts the desktop:

- **Hidden window + turbo** from day 1 — the agent rewrites `ShowWindow` →
  `SW_HIDE`, virtualises `timeGetTime`, and no-ops `Sleep`, so the engine
  ticks as fast as the host can churn through it.
- **MessageBox auto-dismiss** in both the harness (Frida-side hook) and the
  drop-in (`dev_hooks.c` patches `user32!MessageBoxA/W` to log → IDOK).  A
  blocked modal dialog never silently stalls the run.
- **Job-Object launcher** wraps every `.exe` spawn so a SIGKILL on the WSL
  side atomically tears down the Windows-side game process — no stray
  processes after a botched run, ever.
- **Bit-exact frame diffs** between retail and our re-impl (once the DDraw7
  surface-lock hook lands), with red-tint overlay on mismatch.
- **Input trace** sparse JSONL format the harness replays into both targets
  for deterministic side-by-side runs.

The harness grows into a **TAS bot** that drives a whole game session at
inhuman speed, exercising different subsystems against retail and our
re-impl in lock-step.

## Sibling projects

OpenSummoners is the third in a series of sibling RE projects, each
distilling lessons from the last:

- **[openrecet](../openrecet)** — Recettear (the first; reference for
  hard-earned conventions)
- **[OpenMare](../OpenMare)** — Patrician III (newer, refined workflow —
  the closer model for OpenSummoners' shape)
- **OpenSummoners** *(this repo)* — Fortune Summoners

## Cross-references & credits

OpenSummoners' reverse engineering is cross-checked against independent community
work — most notably the **Fortune Summoners Fan Discord**
([invite](https://discord.gg/N68c7pt)) and their *SotES Data Formats & Values*
spreadsheet, which documents struct layouts, enums, addresses, and data tables for
`sotes.exe`. Our thanks to that community for their preservation work.

Their spreadsheet is consulted as a **cross-reference / lead**, not as ground truth:
every fact a port depends on is independently re-verified at the byte level against the
decompile + a host test before it is relied upon (the same discipline applied to our own
subsystem survey). Where our function-level RE 100%-proves something the spreadsheet is
missing or marks uncertain, we publish a **human-verifiable proof** others can reproduce
— see [`docs/ods-crossref.md`](docs/ods-crossref.md) and [`docs/proofs/`](docs/proofs).

## License

MIT.  See [`LICENSE`](LICENSE).  The license covers OpenSummoners' own code
only; no rights are granted to the original game.
