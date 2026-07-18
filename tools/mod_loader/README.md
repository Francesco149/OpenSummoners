# mod_loader — generic Fortune Summoners mod loader (`version.dll`)

A drop-in **`version.dll`** that loads whatever DLLs you drop into a **`mods\`** folder
beside the game exe. The same proven proxy-`version.dll` injection the [JP voice
patch](../ennse_voice/) pioneered, generalized from one hard-coded patch to a folder of
mods. **The game exe is never modified.**

The loader is **generic** — it carries no game addresses, so a game update never breaks
it (only the individual mods pin addresses). Mods share one process: drop the trainer and
the voice patch together and both run.

## How it works

The game imports `version.dll`, so Windows loads ours in its place. On attach it:

1. forwards all 17 `version.dll` exports to `realver.dll` (a renamed copy of your own
   `C:\Windows\SysWOW64\version.dll`, via `version.def`) so the exe's imports resolve;
2. spawns a loader thread that `LoadLibraryEx`es every `mods\*.dll` (with
   `LOAD_WITH_ALTERED_SEARCH_PATH`, so a mod may ship co-located deps in `mods\`).

The load runs on a worker thread, never in `DllMain` (calling `LoadLibrary` from `DllMain`
risks a loader-lock deadlock). It starts once the loader lock frees — after the exe's
static imports resolve — so mods come up right as the game bootstraps: early enough to
install hooks, late enough to be safe. Each mod's own `DllMain` then runs.

A bare `version.dll` reference resolves to `System32\version.dll`, so the drop-in must be
**named `version.dll` AND sit beside the exe** (both true for a game-folder install).

## Install (any game folder with `sotes_en.exe` / `sotes.exe`)

```
<game>\version.dll        <- this (build/version.dll)
<game>\realver.dll        <- copy of C:\Windows\SysWOW64\version.dll
<game>\mods\<yourmod>.dll <- one or more mods
```

Then launch normally. `oss_modloader.log` (beside the exe) records each mod it loaded.

## Build

```
nix develop --command make -C tools/mod_loader     # -> build/version.dll
```

Also built by the mandatory gate `bash tools/ci/build_all.sh`.

## Shipping mods

- **[EN-SE trainer](../sotes_trainer/)** — `mods\sotes_trainer.dll` (teleport / god / map +
  a Dear ImGui UI). Its own README covers the dev-loop injector too.
- **[JP voice patch](../ennse_voice/)** — `mods\ennse_voice.dll` (+ `sotesx_s.dll` in the
  game dir). Installed together with this loader by the patch's one-liner.

Writing a new mod: any DLL with a `DllMain` that spawns a thread and does its thing. Pin
game addresses against the unpacked exe (see [`SE_CODE_MAP.md`](../sotes_trainer/SE_CODE_MAP.md));
resolve them at runtime against `GetModuleHandle(NULL)` so ASLR/rebase is fine.
