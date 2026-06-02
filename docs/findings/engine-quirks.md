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

---

## 9. The "main game window" uses a different WndProc than the "Please wait" window

The engine registers **two** window classes inside `FUN_005a4770`:

- `CLASS_LIZSOFT_WAIT` (reg site `0x5a4ca8`, WndProc `0x401210`) — the
  splash window that paints "Please wait." while the bootstrap runs.
- `CLASS_LIZSOFT_SOTES` (reg site `0x5af314`, WndProc `0x5b12e0`) — the
  actual main game window the player sees.

Both register the same WNDCLASSEX layout (style `CS_HREDRAW|CS_VREDRAW`,
default cursor, no menu) just with different class names + WndProcs.
An earlier read assumed both used `0x401210`; the WAIT one does, but
the main game window's WndProc is `0x5b12e0`, which is what handles
WM_PAINT-for-the-real-game, WM_CLOSE→ExitProcess, WM_ACTIVATEAPP, and
WM_TIMER.

> 📍 See `docs/findings/winmain-and-bootstrap.md` "Two WndProcs, two
> classes" section.

---

## 10. The pump only breaks out when `DAT_008a952c != 0` (WM_ACTIVATEAPP flag)

`FUN_005b1030`'s outer spin loop:

```c
while (1) {
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { ...dispatch... }
    if (DAT_008a952c != 0 && state->[0x1c] == 0) break;
    WaitMessage();
}
```

`DAT_008a952c` is set by the main-window WndProc on `WM_ACTIVATEAPP`
(see quirk §9).  A hidden window in our harness doesn't naturally
get focus-activated by the OS, so the flag stays 0 and the pump
spins forever the first time the engine calls into it.  The Frida
harness fixes this by `PostMessageA(hwnd, WM_ACTIVATEAPP, TRUE, 0)`
to the main game window as soon as the periodic window scan finds
it.

This is also why early bring-up runs (before this fix) saw
`msg_ticks == 0` even with `--turbo --hide-window`: not that the
engine couldn't pump, but that pump entered, found no activation
flag, and spun.

> 📍 See `tools/frida/opensummoners-agent.js installPeriodicWindowScan`
> "main window appearance" branch.

---

## 11. `0x01FFFFFF` is the engine's "no color key" sentinel

Throughout the DDraw surface code (`FUN_005b8b40`, `FUN_005b95c0`,
`FUN_005b9830`) the value `0x01FFFFFF` is passed where you'd expect
a color key.  It's a sentinel: `FUN_005b9830` checks
`if (param_1 == 0x01FFFFFF)` and skips the `SetColorKey` call,
storing a flag in `self->[0xd4]` to mark the surface as "no key
attached".

An earlier read of this constant treated it as some kind of
"unlimited / best-fit hint" to DDraw; that was wrong.  It's not
passed to DDraw at all — it's a marker the engine uses internally.

> 📍 `docs/findings/ddraw-init.md` § "FUN_005b9830".

---

## 12. The engine's 24-bit pixelformat case falls through to 32-bit

In `FUN_005b8c00`'s surface-format switch:

```c
switch (bpp) {
  case 8:  ddpf.dwFlags |= DDPF_PALETTEINDEXED8; break;
  case 16: dwRBitMask = 0xF800; dwGBitMask = 0x07E0; dwBBitMask = 0x001F; break;
  case 24:                                   // ← falls through
  case 32: dwRBitMask = 0xFF0000; dwGBitMask = 0xFF00; dwBBitMask = 0xFF; break;
}
```

Case 24 and case 32 share the same masks (XRGB8888 layout) even
though a real 24-bit DDraw surface is packed RGB888 with no padding.
Probably harmless — DDraw ignores `dwRGBBitMasks` for 24/32-bit
surfaces in practice — but our port should faithfully reproduce the
fallthrough to keep the DDSURFACEDESC2 byte-identical for diffing.

> 📍 `docs/findings/ddraw-init.md` § "FUN_005b8c00".

---

## 13. `sotesd.dll` carries a 60-byte hand-rolled signature check

The first thing `FUN_005a4770` does after `LoadLibraryA("sotesd.dll")`
is verify that resource ID `0x7DE` (2014) contains a recognizable
60-byte ASCII signature.  Each byte in `sotesd.dll`'s resource (at
offsets 60..119) is treated as an index into `A..Z` by adding `0x41`,
producing a 60-character string compared against the engine's
hardcoded constant:

```
JFDGGIUABCVJIEKAUYLPOFDEQBVGSKOLJSCKPIFAXMHGYELSDOBFRKVGBAKB
```

If the comparison fails the engine logs `"Necessary resource (B) is
not found."` and `ExitProcess(0)`.  Looks like a manual integrity
seal — the author typed a random key once, baked it into both the
engine and the data DLL, and shipped them together.

Implication for the drop-in port: we must either reproduce the
identical check (and refuse to run if the user's sotesd.dll has been
tampered with) or just no-op it.  No-op is the cleaner choice — the
check serves no purpose in a behavior-fidelity port that links
against the user's own legit sotesd.dll.

> 📍 `docs/findings/asset-loader.md` § "sotesd.dll signature check".

---

## 14. PE has 1768 Ghidra-recovered functions but several "load-bearing" ones are reached only via function pointer

Ghidra's auto-analysis misses functions that are never the target of a
direct call — common when the engine uses C++-style vtables or stores
callbacks in globals.  Confirmed misses so far:

- `0x4013c0` — launcher DLGPROC (passed to `DialogBoxParamA`)
- `0x401210` — `CLASS_LIZSOFT_WAIT` WndProc (stored in WNDCLASSEX)
- `0x401730` — VRAM-group enable helper (called from the missed DLGPROC)
- `0x5b12e0` — `CLASS_LIZSOFT_SOTES` WndProc — actually Ghidra DID pick
  this up (decompiled output exists) but it has no callers; only
  reachable via a stored function pointer in the second WNDCLASSEX.

Workflow: when grepping Ghidra output for callers of a callback type
(WndProc, DLGPROC, vtable entry) and getting zero hits, fall back to
radare2 (`af @ <addr>; pdf`) on the address you find by scanning for
the function-pointer write site, not by Ghidra's call graph.

> 📍 Pattern documented under `docs/AGENT-WORKFLOW.md` (Reading the
> decompiled output) — radare2 is the second-line tool.

---

> **Entries #15 onward were harvested by the `opensummoners-subsystem-survey`
> workflow (2026-05-29; 16 band-mappers + 6 forward-path scouts).** They are
> *decompile-grade* — strong leads, byte-verify offsets before a port leans on
> them. The survey reported 136 quirks total; these are the load-bearing /
> charming subset. The full set + the subsystem map is archived at
> `docs/audit/subsystem-survey-2026-05-29.json` (mine it instead of re-running).

## 15. `DAT_008a9b50` is the whole engine's god-object singleton

Almost every gameplay subsystem reaches state through one global pointer,
`DAT_008a9b50` (appears 32+ times across the `0x4a****`–`0x5b****` survey
alone).  It is effectively a single large C++ class instance holding *all*
the pools and sub-managers at fixed offsets:

- `+0x1038` → scene-state struct (`[0]`=map/area id, `[1]`=screen/mode id
  like `0xd2`/`0xf0`/`0x136`/`0x300`, `[4]`=event-specific context).
- `+0x1160` → 32-slot on-screen entity registry.
- `+0x2780` → difficulty mode; `+0x2784` game-mode; `+0x2798` boss-AI state.
- `+0x27a0` → global frame/spawn counter (see #18).
- `+0x2790` → NPC list.

This is a load-bearing architectural choice: the port must preserve a single
shared instance (or thread-wrap it) — scene handlers are *not* passed their
state via ctor, they read the singleton.

> 📍 survey band `0x4a0000-0x4d0000` + `0x5a0000-0x5bdab0`.

## 16. The universal frame pump and the `return 6` quit convention

Every scene handler — title, dialogue, dungeon, battle — is written as a
single big function that, hundreds of times, sets some state then calls a
frame-pump (`FUN_00439680` for gameplay scenes; `FUN_005b1030`
/`app_pump_frame` for the title path) and checks the return: **`6` means
"user cancelled / quit"** and unwinds the entire scene immediately.  There is
no exception machinery and no explicit dtor calls — cancellation is just this
magic return code propagated up by hand, and cleanup is assumed automatic.

Port-critical: the pump *must* yield to the Win32 message loop and poll input
each call.  A blocking reimplementation will hang the whole game, because the
scene functions never return to an outer loop on their own.

> 📍 survey band `0x4a0000-0x4d0000`, `0x4d0000-0x540000`.

## 17. Asset / entity directory is keyed by opaque 32-bit hash IDs — and we know some names

Characters, items, and NPCs are resolved through `FUN_00556eb0(<hash>)`, a
string-id → resource-pointer resolver.  The hashes are baked 32-bit constants
with no enum in the binary.  The survey recovered a few mappings worth their
weight in gold for future RE:

- `0x5f5e165` → main PC **Arche**
- `0x5f5e166` → companion **Sana**
- `0x5f5e168` → dummy/placeholder party slot sentinel (party-validity check
  compares `char+0x9f0` against this to skip empty slots)
- `0x35a4e902` → teacher **Sophia**

Scene/mode IDs are a *separate* family of sparse magic constants
(`0x11300`, `0x1124c`, `0x186a1`–`0x186b0` magic-move classes, `0x186ca`+
scene ids) that appear only as switch cases.  Recovering the full ID→name
tables (from the data section or by Frida-tracing the resolver) is a standing
task; record every mapping you confirm here.

> 📍 survey bands `0x4a0000-0x4d0000`, `0x4d0000-0x540000`, `0x540000-0x560000`.

## 18. Global frame/spawn counter resets at `0xffffff`, not a power-of-two wrap

The counter at `DAT_008a9b50+0x27a0` increments per entity state-change /
spawn and, instead of a cheap power-of-two mask, *resets to 0 when it reaches
`0xffffff`* (16,777,215).  Most engines wrap with `& (N-1)`; this deliberate
decimal-ish ceiling smells like avoidance of an arithmetic edge case in the
sprite-scheduling math.  Preserve it — code downstream compares against it.

> 📍 survey bands `0x450000-0x470000`, `0x540000-0x560000`.

## 19. The "object perpetuity state area has been fully used" crash log

When the object-spawn pool (`base+0x1804`, capacity `base+0x7804`) hits
`0x800` entries, the allocator doesn't realloc — it `GetLastError()`s,
`FormatMessageA`s, and `OutputDebugString`s the literal string *"The object
perpetuity state area has been fully used"* into `DAT_008a9534` right before
falling over.  A hard 2048-entry ceiling that was clearly runtime-tuned, with
charmingly bureaucratic telemetry on the way down.

> 📍 `FUN_004a63d0:35`, `FUN_004b3b20:61`.

## 20. The "scene function" is a degenerate coroutine: `in_ECX[5]` is the resume counter

Every large scene/dialogue function uses object field `in_ECX[5]` (i.e.
`+0x14`) as a plain phase counter: `0`=init, `1`=loop, `2`=outro, etc.  On
first entry it initialises; on every later frame it re-enters and an `if`
ladder jumps to the matching phase.  No actual continuation/stack state is
saved — it's a hand-rolled coroutine where the only persisted thing is *which
branch to run next*.  This makes the functions enormous, re-entrant per
frame, and extremely fragile to edit (insert a phase and every later index
shifts).

> 📍 survey bands `0x4a0000-0x4d0000`, `0x4d0000-0x540000`.

## 21. Character/sprite struct stride is `0x294` (660 B), entities `0x300`

Sprite/character arrays are indexed `base + idx*0x294` everywhere in the
render path, with hot offsets `+0x04` x, `+0x08` y, `+0x40` w, `+0x44` h,
`+0x48` sprite-state enum, `+0x5c` duration, `+0x66` frame-count, `+0x2c`
rotation-mode (0–3).  Gameplay *entities* are a different pool at `0x300`-byte
stride, and actor state at `0x140`.  These strides are load-bearing: a single
misread offset (Ghidra sometimes shows `0x290`/`0x298`) silently corrupts the
whole renderer.  Byte-verify before porting any sprite code.

> 📍 survey bands `0x480000-0x490000`, `0x490000-0x4a0000`, `0x420000-0x430400`.

## 22. RNG is the classic MS-rand LCG, seeded once globally

`FUN_005bf505` is `DAT_008a4f94 = DAT_008a4f94*0x343fd + 0x269ec3; return
(seed>>0x10)&0x7fff` — the textbook Microsoft `rand()` LCG.  It is used
*pervasively*: sprite-effect timing, ability cooldowns, damage variance,
difficulty scaling, even per-animation frame jitter (it's called multiple
times per render in some animators, not seeded per frame).  Faithful play
requires reproducing the **seed initialisation order**, since the whole game's
randomness is one shared stream.  (One survey agent guessed "Mersenne
Twister" elsewhere — the constants prove it's the simple LCG; trust the
constants.)

> 📍 `FUN_005bf505` (band `0x5a0000-0x5bdab0`).

## 23. Input auto-repeat is hand-rolled: 300 ms first delay, then 100 ms

`FUN_0043ca40` implements key/pad repeat entirely in C with no DirectInput
repeat setting: first press freezes for `GetTickCount()+300`, then switches to
a 100 ms repeat window.  Slow first repeat, fast thereafter — the classic
arcade/console menu feel, deliberately reproduced.  Input events live in a
consume-on-read ring buffer at `+0x108` (3-dword slots `[button_id, timestamp,
state]`, backward linear search, head index at `+0x0c`); a poll that matches
`state==1` clears the slot.

> 📍 `FUN_0043ca40`, `FUN_0043c110`, `FUN_0043ce50` (band `0x430000`).

## 24. Gamepads are lazily attached on the first menu *confirm*, never at boot

DInput joysticks are not enumerated during init.  `FUN_005ba120` (acquire up
to 2 pads into `DAT_008a93dc[0..1]`) is called only when the title menu sees
action `==3` (confirm) for the first time.  A headless/turbo smoke run that
exits before "press start" therefore never attaches a pad — and that's
correct behaviour, not a bug.  Keyboard works throughout; only pad acquisition
is deferred.

> 📍 `FUN_0056aea0:528-542`, `FUN_005ba120`.

## 25. BGM is played by writing the WMA to a temp file and RenderFile-ing it

Music (WMA resources in `sotesw.dll`) is *not* streamed from memory.
`FUN_005bc150` extracts the resource bytes to a `GetTempFileNameA` path, then
`FUN_005bab10` builds a DirectShow/WMF graph via `CoCreateInstance`
(`IGraphBuilder`, IIDs in `DAT_00850f08/28/58`) and `RenderFile`s the temp
file; the temp file is deleted after.  Most engines use an in-memory IStream;
this one round-trips through disk for every track.  SFX, by contrast, go
through DirectSound (ZDS) with a **50-voice** polyphony ceiling (`0x32`).

> 📍 `FUN_005bc150`, `FUN_005bab10`, `FUN_005bbeb0` (band `0x5a0000-0x5bdab0`).

## 26. "Disable Sound" gates the music manager (ZDM) only, not DirectSound (ZDS)

The launcher "Disable Sound" checkbox sets `settings[0x21c]=1`, which skips
**ZDM** (music) init but leaves **ZDS** (DirectSound primary buffer + SFX)
fully initialised.  Practical upshot for the harness: you can exercise the SFX
path with music disabled, isolating audio bring-up.

> 📍 `FUN_005ba6e0` boot driver; cross-ref `findings/audio-init.md`.

## 27. Title menu item order is hardcoded `0x1a, 0x1c, 0x1e, 0x1d, 8`

The five title entries (New Game / Load / Options / ? / Exit) are populated in
that fixed action-id sequence, and the loop matches each against
`DAT_008a6e80+0xa60` — a **save-game validity flag** — to pick the initial
highlighted slot (so "Continue" only auto-selects when a save exists).  Reorder
or omit a slot and menu init breaks.

> 📍 `FUN_0056aea0:401-464`.

## 28. The title intro is one reused 0..1000 fade ramp across 8 phases, then a "breathing" menu oscillator

The whole studio-logo → title-logo → "press button" → sparkle intro is driven by
a **single** fade accumulator (`uVar15`, range 0..1000) that is reset and
re-purposed at each phase boundary, with a *different per-phase increment*
(`+20`, `+10`, `+100`, `+10`, `+20`) and *two distinct saturation idioms*: the
fade-*in* phases clamp with a plain `if (v > 1000) v = 1000` after the add, while
the fade-*out* phases (2 and 10) and the menu cross-fade use a branchless
`max(v - d, 0)` (`setl bl; dec ebx; and ebx, eax`).  Phase advance is gated
sometimes by the fade reaching its rail and sometimes by a *separate* hold-timer
(`local_68`) — e.g. phase 5 saturates the fade in 10 frames but holds for 40.

Once the menu is up (phases 8/9) the same `uVar15` first ramps to 1000, then a
*second* ramp `local_58` oscillates **up to 1000 (phase 8) then back down to 0
(phase 9), forever** — this is the menu highlight's pulsing "breathing" glow, a
two-state loop with no exit except a menu action.  The phase-7 sparkle intensity
is `(fade * 0xe0) / 900 + 0xc0` and is only spawned while `fade < 850`, so the
sparkles fade out *before* the ramp tops out.

> 📍 `FUN_0056aea0` `switch(local_64)` @ `0x56b153..0x56b5c1`; ported as the pure
> FSM in `src/title_scene.c` (checkpoint 1).  See `findings/title-scene.md`.

## 29. The frame pacer is a fixed-16 ms-timestep accumulator with a dead vestigial FPS counter

The outer loop of every scene runner (mapped here in `FUN_0056aea0`) is paced by
a 3-state machine (`local_28` = 0/1/2) that is the textbook
**accumulate-then-render fixed-timestep loop**, not a simple sleep-to-vsync.
`sub==2` (update) burns the accumulated wall-clock budget (`local_30`, ms) in
**16 ms slices** — one slice per iteration — running the input + phase FSM each
time; when the budget is down to its last slice it flips to `sub==1` (render),
which draws+flips, then **refills** the budget from the real elapsed
`GetTickCount` delta (clamped to **100 ms** so a hitch can't stack an unbounded
catch-up burst) and pumps the OS queue (`FUN_005b1030`) on the way back to
update.  The pump is called only on the `sub==0`/`sub==1` (render→update)
transitions, never on a pure update slice.  Consequence: on a machine fast
enough that `GetTickCount` doesn't advance between iterations (turbo / very fast
host), the budget never refills past one slice, so after the first update the
loop **renders every frame** with the phase FSM frozen — exactly the "splash
doesn't animate under `--turbo`" symptom noted earlier.

Hidden inside the pacer's post-transition block is a **dead** counter: on every
`sub==1` frame the engine increments `[esp+0x5c]` while <1000 ms have elapsed
since a 1-second-window anchor `local_20` (and resets both otherwise) — a
consecutive-sub-second-frame tally, i.e. an FPS / uptime counter.  A
full-function disassembly scan finds `[esp+0x5c]` **written only, never read**;
Ghidra dead-store-eliminates it; and `local_20`'s sole read merely gates that
dead update, so the *entire* `sub==1` post-arm is observably inert (it changes
no sub-transition, pump call, or render/update decision).  The port drops both
locals — behaviourally exact — keeping only the load-bearing `sub==2` arm
(`local_2c = now`, the anchor the budget-exhaust test measures against).

> 📍 `FUN_0056aea0` pacing FSM @ `0x56b002..0x56b0c8` (r2, raw stack offsets);
> ported as `title_pace_*` in `src/title_scene.c` (checkpoint 2).  See
> `findings/title-scene.md` "Frame-pacing sub-state machine".

## 30. Input is polled as a consume-on-read 100 ms recency window

The engine doesn't read a debounced key state — it scans a **64-entry ring
of timestamped event records** (`FUN_0043c110`, the manager's `+0x108`
ring) and asks "is there a *pressed* record for button N whose timestamp is
within 100 ms of `now`?".  Three quirks fall out of the 84-byte routine:

- **Consume-on-read.** A hit doesn't just return 1 — it **zeroes the matched
  record's id** (`record[0] = 0`).  So a single physical press is "used up"
  by the first poller that matches it; a second poll for the same button in
  the same frame misses.  This is how the menu code can poll several buttons
  in sequence without one press registering twice.

- **100 ms staleness gate, unsigned.** The age test is
  `(uint32_t)(now - record.ts) <= 100`.  Because it's unsigned, a record
  whose timestamp is *ahead* of `now` (a `GetTickCount` rollover, or a stale
  slot left over from 49.7 days ago) underflows to a huge delta and is
  rejected — the rollover is handled for free, exactly like the frame
  pacer's anchors (quirk #29).

- **Newest-slot-wins scan.** The scan starts at the **top** slot (index 63,
  address `+0x108`) and walks **down**, so when two slots hold a matching
  event the higher-indexed one is consumed first.  The ring is evidently
  filled such that the newest event sits at the highest index.

The *producer* — whatever fills the ring from `IDirectInputDevice7::
GetDeviceState` — is still a black box (milestone 1); this is only the read
side.  `mem_watch.py --region <+0x108 addr>:64:input_ring` is the tool meant
to catch the writer live.

> 📍 `FUN_0043c110` @ `0x43c110` (84 B); ported as `input_poll_consume` in
> `src/input.c` (checkpoint 3).  See `findings/input.md`.

## 31. Pool-acquire writes the slot index as a 16-bit store into a 32-bit field

`FUN_00412c10` (object-pool checkout) stamps a freshly-handed-out slot with
its index via `*(uint16_t *)(slot + 4) = count` — a **16-bit** write into
what is otherwise treated as a dword field, leaving the slot's `+6` half
untouched.  Easy to miss in a port (a naive `slot->index = count` as a
32-bit store would also clobber `+6`); the high half evidently carries
something the pool doesn't reset on reuse.  Modelled as an explicit
untouched `_hi6` and pinned by a test.

> 📍 `FUN_00412c10` @ `0x412c10` (46 B); ported as `obj_pool_acquire` in
> `src/obj_container.c` (checkpoint 3).

## 32. The cursor-nav engine packs four list behaviours behind one jump table whose fields change meaning per type

`FUN_0043ca40` dispatches the menu direction code through an indirect jump
table at `0x43ce1c` that **Ghidra refuses to recover** (it has duplicate
targets — five of the eleven entries point at the same "return 0" stub).
`radare2 -c 'pxw 44 @ 0x43ce1c'` reads it cleanly: dir 0..10 →
`{cb0b, cbd2, cc7c, cce2, cdfe×5, cae9, cafa}`, i.e. 7 distinct handlers
(prev / next / page-up / page-down / no-op / cancel / confirm).

The genuinely confusing part is that the list-header fields **mean
different things depending on `hdr->type`** (`+0x00`):

- **type 0 (linear wrap):** `+0x18` (`sel2`) is a *wrap floor* — prev from
  the page top jumps to `sel2 + stride − 1`, next from the bottom wraps
  back to `sel2`.
- **type 2 (grid):** `+0x18` is the *visible page-top*, recomputed in the
  shared tail as `floor(cursor/stride)*stride`; prev/next stay inside the
  current row and only page-up/down cross rows.
- **type 3 (trailing page):** `+0x18` *trails* the cursor — the tail forces
  `sel2 = cursor − stride + 1` whenever the cursor passes the old window
  bottom, so the selection sits at the bottom of the viewport as you
  descend.

So the same four dwords (`stride/count/cursor/sel2`) drive three quite
different scroll models. Ported branch-for-branch as `menu_list_nav`.

> 📍 `FUN_0043ca40` @ `0x43ca40` (970 B); ported in `src/menu_list.c`
> (checkpoint 4b).  See `findings/menu-list.md`.

## 33. Menu auto-repeat is a two-rate timer stored in the list header, in the GetTickCount domain

The nav engine's "axis held" codes (dir 4/6) don't move the cursor on every
call.  The first call **arms** a deadline `header[+0x1c or +0x20] =
GetTickCount() + 300`; subsequent calls do nothing until `now` reaches the
deadline, at which point the engine **re-fires** as the equivalent press
(dir 4→1 "next", dir 6→0 "prev") and **re-arms at +100** — so the repeat
rate steps from a 300 ms initial delay to a 100 ms steady cadence, the
classic key-repeat feel.  Releasing the axis (dir 5/7) zeroes the deadline.
Two independent deadlines (`+0x1c`, `+0x20`) let the two axes repeat
independently.  All comparisons are unsigned `GetTickCount` deltas, so the
49.7-day rollover is a non-issue (cf. quirks #29, #30).

> 📍 `FUN_0043ca40` @ `0x43ca40`; the timer arms live at handler entry
> (dir 4/6 cases).  In the port `GetTickCount()` is injected as `now`.

## 34. The menu input latch is gated on a "1000 == fully faded in" sub-object, and confirm boxes need two presses

`FUN_0043ce50` (the action latch) refuses to act unless its input
sub-object reports `sub[+0x54] == 1000` **and** `sub[+0x04] != 0`.  The
magic `1000` is the same `0..1000` ramp the title fades run on (title-scene
phases) — input is dead until the transition-in animation has fully
completed, which is why button mashing during a menu fade does nothing.

When the controller is in mode 2 (a confirm / scrolling-message box rather
than a cursor list), the latch implements the familiar **two-press**
behaviour directly: the first confirm/cancel **reveals all** remaining text
(fast-forwards `pos` to the cap reached via the `src → [+0xc] → [+8]` u16
chain, returns 6), and only the *second* press — once `pos >= cap` —
**dismisses** it (latches `action = 8`, returns 8).  A latched `action == 8`
blocks further input until reset, so a held button can't skip past the box.

> 📍 `FUN_0043ce50` @ `0x43ce50` (220 B); ported as `menu_list_latch` in
> `src/menu_list.c` (checkpoint 4c).  See `findings/menu-list.md`.

## 35. The menu-controller constructor calls its own destructor first — slots are recycled, not freshly zeroed

`FUN_0040f5c0` (the menu-controller geometry constructor) opens by calling
`FUN_0040e0c0` — its **matching destructor** — *before* writing anything.
This is not redundant: the controller is checked out of a fixed-capacity
object pool (`FUN_00412c10`, quirk #31), which stamps only the slot's
owner/index/`+8` and leaves the rest holding **whatever the previous
occupant left there**.  So the ctor cannot assume zeroed memory; it runs
the full teardown to free any stale header / row-array / cell sub-objects /
confirm graph from the prior life of that slot, *then* rebuilds.  On a
genuinely first-use slot every pointer happens to be NULL and the teardown
no-ops — but the engine never relies on that.  A re-entry to the title
scene's spawn block therefore silently recycles the same controller in
place.

Two layout subtleties the alloc/free pair pins down:

- The controller carries **two parallel arrays** sized off *different*
  header dimensions.  The **row** array (`+0x17c`, one `0x10`-byte slot per
  menu line) is sized by `alloc_a` (hdr `+0x04`); the **per-column entry**
  array (`+0x178`, `0x24` bytes each) *and* every row's **cell** array
  (`0x18` bytes each) are both sized by `alloc_b` (hdr `+0x08`).  For the
  title menu that's `alloc_a=6, alloc_b=1` → up to 6 rows × 1 cell, with a
  single column-metadata entry.  Each entry is stamped `pos = index*0x20`,
  `extent = 0x20` (a vertical layout stride).
- The destructor frees the header **last**, because its row/cell free loops
  read their bounds (`alloc_a`/`alloc_b`) out of the still-live header.

> 📍 `FUN_0040f5c0` @ `0x40f5c0` (563 B) / `FUN_0040e0c0` @ `0x40e0c0`
> (555 B); ported as `menu_ctrl_build` / `menu_ctrl_clear` in
> `src/menu_list.c` (checkpoint 5).  See `findings/menu-list.md`.

## 36. The grid-cell finalizer's lazy allocation is dead code — it only ever re-zeroes

`FUN_00411f40` walks a menu row's cell array and, per cell, "refreshes" up
to three sub-objects (`obj0` text/glyph, the `0x54` object, the `0x20`
object).  For the `0x54` and `0x20` objects the decompile reads as a classic
lazy get-or-create:

```c
if (cell.obj54 != 0 && i < alloc_b && row < count) {       // outer guard
    if (cell.obj54 == 0) cell.obj54 = operator_new(0x54);  // ← never taken
    /* zero a handful of fields */
}
```

The inner `== 0` allocation sits **inside** the outer `!= 0` guard and reads
the *same* slot with no write in between — verified against the disasm
(`0x411fbf` for the `0x54` object, `0x412046` for the `0x20`).  So the
`operator_new` branches are statically unreachable: the function never
allocates, it only re-zeroes sub-objects that some *other* path already
built (and, for the `0x20` object, recomputes `+0x1c = max(+0x14,
min(+0x18, 0))` — which, reading the zeros it just wrote, settles at 0).

Consequence for the port: `menu_row_finalize` omits the dead alloc (matching
how the dead null-check on `cl` was handled in `menu_list_latch`).  And the
earlier note in `findings/menu-list.md` that this function "lazily
`operator_new`s" the sub-objects was **wrong** — corrected there.  On the
fresh title menu every cell pointer is NULL, so the whole finalizer is a
no-op; the sub-objects are populated only when real menu items with
text/icons are configured (a still-unmapped path, distinct from the cheap
inline row appends of the spawn block).

> 📍 `FUN_00411f40` @ `0x411f40` (444 B); ported as `menu_row_finalize` in
> `src/menu_list.c` (checkpoint 6).  See `findings/menu-list.md`.

## 37. The menu is a tree of uniform 0x1b0 nodes — each node *is* a menu_ctrl plus display config, and Ghidra mis-typed the node builder's `this`

`FUN_0040f3e0` (the menu-item / page builder, ported as `menu_node_build`)
reveals that the engine's menus are a **tree of uniform 0x1b0-byte nodes**.
A single node layout is reused at every level and overlays *two views* on the
same buffer:

- a **container header** (`+0x00..+0x84`): an owner back-pointer (`+0x00`),
  scalars, and at `+0x48`/`+0x4c` a heap array of child-node pointers and its
  u16 count;
- an **embedded `menu_ctrl`** at `+0x00` (so `+0x164..+0x17c` are exactly
  `menu_ctrl.field_164/list2/list/entries/rows`), followed by `0x30 B` of
  **display config** at `+0x180..+0x1ac` (text/shadow colours `0x3e537d` /
  `0xa8b9cc` / `0xf08080` and two label-string VAs `&DAT_00677b98` /
  `&DAT_008090a9`).

So one object is simultaneously a tree container *and* a selectable
controller — which is why the builder frees a stale child with
`menu_ctrl_clear` (`0x40e0c0`) before `operator delete`.  The owning list
(`param_1`) is a `sel_list` (the `obj_container.h` single-select list) whose
entries are these nodes; `node+0x08` is the `sel_entry` "selected" flag that
`sel_list_mark_last` (`0x414080`) toggles right after the build.

**Ghidra trap.** Ghidra mis-typed `FUN_0040f3e0`'s `__thiscall`: it rendered
the ECX `this` as the call's *first stack arg* and dropped the real `this`.
The decompiled call `FUN_0040f3e0(piVar11,0,0,100,100,1,0)` therefore reads as
"operates on `piVar11`", which led the earlier `findings/menu-list.md` /
HANDOFF notes to call it a "page-container" function.  The disassembly
corrects this:

- prologue `0x40f3ec  mov ebx, ecx` — the function works on ECX;
- call site `0x56b606  mov ecx, [edi + ecx]` (= `owner->entries[count]`, with
  `edi = count*4`) sets ECX to a **node**, while `0x56b609  push esi` makes
  `owner` (= `piVar11`) the first *stack* param.

So `this` is the node being configured and `piVar11` is its owner — off by
one from the decompile.  `ret 0x1c` (7 dwords) confirms the seven stack
params `(owner, f_c, f_10, f_14, f_18, n_children:u16, config)`.

> 📍 `FUN_0040f3e0` @ `0x40f3e0` (434 B); ported as `menu_node_build` in
> `src/menu_list.c` (checkpoint 7).  Always confirm a `__thiscall`'s ECX in
> the disasm before trusting the decompile's argument list.

## 38. The menu node has *four* overlaid identities — and the obj_pool / sel_entry aliases coincide only on the 32-bit target

Assembling the title-menu spawn block (`0x56b5cd..0x56b807`, ported as
`title_menu_spawn`) shows the 0x1b0 menu node from quirk #37 wears **four**
hats on one buffer, each a different engine primitive reinterpreting the same
address:

| view | fields used | who uses it |
|------|-------------|-------------|
| **container header** | owner `+0x00`, child array `+0x48`, child count `+0x4c` | `menu_node_build` (`0x40f3e0`) |
| **embedded `menu_ctrl`** | `+0x00..+0x17c` + display config `+0x180..+0x1ac` | `menu_ctrl_build` / nav / latch |
| **`sel_entry`** | selected flag `+0x08` | `sel_list_mark_last` (`0x414080`) |
| **`obj_pool`** | slots `+0x48`, capacity `+0x4c`, count `+0x4e` | `obj_pool_acquire` (`0x412c10`) |

The spawn is one elegant chain exploiting this: `menu_node_build` gives the
node **one child** and sets its child array (`+0x48`/`+0x4c`/`+0x4e`); then
`obj_pool_acquire(node)` — *the same node, reinterpreted as a pool* — hands
out `children[0]` as the menu controller and stamps that child's `+0x00` with
the node pointer.  Since `+0x00` is the controller's `menu_ctrl.sub`, this
**wires the controller's input-ready gate to the node**: `menu_list_latch`
later reads `sub->ready` (`node+0x54`) and `sub->enabled` (`node+0x04`),
gating menu input on the node's own `+0x54` ramp (the same 0..1000 transition
the fades use — quirk #34).  The controller built on `children[0]` thus draws
its "am I accepting input yet?" state straight from its parent node.

**Porting trap (32- vs 64-bit).** These overlays are byte-exact *only on the
32-bit target*.  Two of them depend on field offsets that the 0x1b0 node and
the small primitive structs share only when pointers are 4 bytes:

- `obj_pool` puts slots/capacity/count at `+0x48`/`+0x4c`/`+0x4e`; the node's
  `children`/`child_count`/`field_4e` line up there on win32 but **not** on
  the 64-bit host (the node's leading `void *owner` and the pool's `0x48`-byte
  opaque head widen differently), so a `(obj_pool *)node` reinterpret reads the
  pool header at the wrong offsets and `obj_pool_acquire` returns NULL.
- likewise `sel_entry.selected` at `+0x08` is `menu_node.selected` at `+0x08`
  on win32, but the node's 8-byte `owner` pushes its modelled `selected` field
  to `+0x0c` on the host — so `sel_list_mark_last` (writing the `sel_entry`
  view at `+0x08`) and a read through `menu_node.selected` disagree on the host.

The drop-in is win32, so the retail reinterpret-casts are correct there.  The
host **port** can't use them: `title_menu_spawn` therefore applies
`obj_pool_acquire`'s semantics to the node's own `menu_node` fields (identical
to the cast on win32), and the test checks the selection flag through the
`sel_entry` view that `sel_list_mark_last` actually writes.  This is the same
"layout-exact only on the 32-bit target" discipline as the per-child
`menu_ctrl_clear` cast in `menu_node_build` (quirk #37).

> 📍 spawn block of `FUN_0056aea0` @ `0x56b5cd`; ported as `title_menu_spawn`
> / `title_menu_teardown` in `src/title_scene.c` (checkpoint 8).  When a
> retail function reinterpret-casts one object as another primitive, the cast
> is only portable to the host if both structs are pointer-free up to the
> aliased fields — otherwise replicate the semantics on the real struct.

## 39. The title menu's "confirm" is the *cancel* latch code — action means the nav return value, not the button

The title menu's per-frame input dispatch (`FUN_0056aea0` default branch,
`0x56b807..0x56ba39`, ported as `title_menu_input_step`) polls five buttons,
feeds each into the action latch `menu_list_latch(ctrl, dir, now)`, and then
`switch`es on the latch's **return code** (`esi` / retail `iVar14`).  It is
easy to misread the switch as keying on the button — it does not, and the two
do not line up:

| button | latch `dir` | nav meaning | nav **returns** | switch case → effect |
|--------|-------------|-------------|-----------------|----------------------|
| 1 (up)     | 0 | prev      | 1 (moved)  | 1 → move SFX 9 |
| 3 (left)   | 1 | next      | 1 (moved)  | 1 → move SFX 9 |
| 2 (down)   | 2 | page-up   | 0/2        | 2 → move SFX 9 (or 0 → nothing) |
| 4 (right)  | 3 | page-down | 0/2        | 2 → move SFX 9 (or 0 → nothing) |
| **0x24**   | **9** | **cancel** | **3**   | **3 → commit** (joystick + leave menu) |

So the physical **commit/start button is `0x24`**, and it reaches the
"commit" arm *because the cancel handler returns 3*.  An earlier findings note
called `0x24` "back/cancel" — that was reading the `dir` passed in, not the
outcome.  Conversely the switch's `case 4` (which plays a distinct "cancel"
SFX 7) is **dead in the title flow**: the only latch dir that returns 4 is
`dir 10` (confirm), which the title dispatch never sends.

Two more easy-to-miss details in the same block:

- `case 3` then gates on the **selected row**'s `flag8` (enabled), at
  `rows[cursor]+8` — *not* on the action id.  A disabled row plays a "denied"
  SFX (6) and changes nothing; an enabled row plays "confirm" (5) and commits.
  (A summary that said "if action == 0x1d push SFX 6" was conflating this
  enable-check with the later `act != 0x1d` save-data guard.)
- on the single-page title menu (`stride 6 ≥ count 5`) the page-up/down dirs
  (`2`/`3`) are **no-ops** — nav's page handlers require `stride < count` — so
  "down"/"right" do nothing; only "up"/"left" (prev/next) actually move the
  cursor, wrapping through the five rows.

> 📍 `title_menu_input_step` in `src/title_scene.c` (checkpoint 9).  When a
> port `switch`es on a callee's return value, label the cases by what the
> callee *returns*, not by the input that produced them — the engine's
> cancel-returns-3 / confirm-returns-4 convention inverts the intuitive
> button→meaning reading here.

## 40. The render half's fade→alpha ramp is a runtime-filled 20-entry table that returns 0 at *both* ends — and full saturation maps to 0, not the top entry

`FUN_0056aea0`'s render branch (`0x56bb04..0x56bf1a`, ported as
`title_render_step`) blends every fading sprite through `0x448c80`
(`title_fade_ramp`), called everywhere as `0x448c80(fade, 1000)`:

```
idx = (fade * 20) / 1000          ; = fade / 50, signed truncating
if (idx < 0 || idx >= 20) return 0
return ramp[idx]                  ; ramp = the 20-dword table at 0x8a9308
```

Three things bite:

- **Full fade returns 0.** `fade == 1000` gives `idx == 20`, which trips the
  `>= 0x14` cap and returns **0**, not `ramp[19]`.  So a *fully* saturated fade
  blends at alpha 0, the same as `fade == 0`.  The ramp only produces a
  non-zero blend in the open interval — `fade` in `[50, 999]` (`idx` 1..19).
  The intro phases exploit this: they ramp `fade` to 1000 then *hold* it there,
  and the logo handlers (`0x56bb5c`/`0x56bbd4`) read the ramp result to choose
  between an alpha blit (`0x494e10`, ramp ≠ 0) and a plain surface clear
  (`0x5b9b70`, ramp == 0) — i.e. at the saturated hold the logo is composited
  by a *different* path than during the ramp.

- **The table is empty in the static image.** `0x8a9308` (20 dwords) reads
  **all zero** in `sotes.unpacked.exe`; DDraw/asset init fills it at run time.
  A faithful host port that has no live palette therefore sees every ramp
  lookup return 0 → the logo handlers always take the clear branch and sparkles
  draw at alpha 0.  That is the correct *pre-init* behaviour, not a bug;
  `title_fade_ramp` takes the table as a parameter (NULL ⇒ all-zero) so a test
  can supply a populated ramp to exercise the blit path.

- **The two intro logos are not `0x418470` assets.** Press-button / sparkle /
  menu sprites are fetched by id through `0x418470(id)` (asset ids 2..6), but
  the studio and title logos are read as **fields at +4 and +8** of the asset
  container object (`*(*(*0x8a7658))`), distinguishing them only by that
  offset.  The port records the logo draw with `asset = 4` (studio) or `8`
  (title) to keep the two handlers' single shared blit (`0x494e10` at
  `0x56bc37`) distinguishable.

Also worth flagging structurally: Ghidra rendered the jump-table dispatch at
`0x56bb55` (`jmp [phase*4 + 0x56bfa4]`) as a **call that returns**, making the
seven per-phase handlers look like separate functions.  They are inline labels
*inside* `FUN_0056aea0`; every one `jmp`s to the shared frame-end at `0x56bec4`
(compose `0x56c180` → "Title Menu - Flipping" log → Flip `0x5b8fc0`), so the
render half always presents exactly one frame regardless of which handler ran.

> 📍 `title_render_step` / `title_fade_ramp` in `src/title_scene.c`
> (checkpoint 10).  When a small "scale" helper indexes a table with a
> hard `>= N` cap, check the endpoint: here the natural "max input" lands
> exactly on the excluded index and silently returns 0.  And a data table
> that reads all-zero statically is a signal it is populated at run time —
> model it as an input, not a constant.

## 41. The input manager's "axis-held" flags are slots [0] and [1] of an 11-dword array, with a parallel array beside it

The title-menu input dispatch reads two "axis held?" flags at input-mgr
`+0x114` (vertical) and `+0x118` (horizontal) to synthesise auto-repeat / release
nav events (quirk #33 / `title_menu_input_step`).  They looked like two
standalone fields — but the **skip-splash field flush** (`0x56b25e..0x56b29a`,
the "press a button to skip the intro" early-out) reveals their real shape: it
zeroes `+0x114..+0x140` and `+0x140..+0x16c` as **two parallel 11-dword arrays**
(`mov [eax+0x2c],ebx; mov [eax],ebx; add eax,4` ×11), plus the `+0x16c` half-word
and the `+0x10c`/`+0x110` dwords.  So `+0x114` is `array_A[0]`, `+0x118` is
`array_A[1]` — the vertical/horizontal flags are just the first two of eleven
per-direction(?) entries, and a second array B sits at `+0x140`.  B's semantics
aren't recovered yet (the flush zeroes it alongside A); the title path only ever
reads A[0]/A[1].

The flush also walks the 64-entry ring-pointer table at `+0xc..+0x108` and zeros
each slot's *id* dword (`*ring[i] = 0`) — a wholesale consume of every pending
event — then forces the scene to the menu phase.  This subsumes the redundant
single-slot zero the scan path did on the matched slot at `0x56b18f`.

The skip-splash gate (`0x56b107..0x56b117`) is itself a quirk: it is active for
phases 1..7 always, but at phase 0 only when the scene's `param_1` ("start at
menu / skip intro", set on re-entry from a submenu) is non-zero — so a *first*
boot (`param_1 == 0`) cannot skip the very first frame of the studio fade, only
phases 1+.  Below phase 3 the skip also fires the same BGM `SetNextSegment` cue
the phase-2→3 transition would have (so skipping early still advances the music).

> 📍 `input_mgr` + `input_any_fresh_press` / `input_mgr_reset` in
> `src/input.{h,c}`; skip-splash wiring in `title_scene_step`
> (checkpoint 12).  When two adjacent scalar fields turn out to be cleared
> by a counted loop, they're probably `array[0]`/`array[1]` — let the *reset*
> code, not the *read* code, tell you a struct's true array shape.

## 42. The title-menu nav button ids don't mean what their latch-dir names say — id 3 (not id 2) moves the cursor *down*

Confirmed **live** by injecting single ring events into retail and watching the
cursor (the TAS harness, checkpoint 13).  `title_menu_input_step` polls five
button ids and feeds each through `menu_list_latch(dir)` → `menu_list_nav(dir)`.
The nav engine's dir dispatch is: **0 = prev (cursor up), 1 = next (cursor
down), 2 = page-up, 3 = page-down**, 9 = cancel/commit, 10 = confirm.  The
title's id→latch-dir wiring is:

| ring id | latch dir | nav effect | so the button is… |
|---------|-----------|-----------|-------------------|
| `1`     | 0         | prev      | **UP**            |
| `3`     | 1         | next      | **DOWN**          |
| `2`     | 2         | page-up   | (no-op: single column, `stride ≥ count`) |
| `4`     | 3         | page-down | (no-op: single column) |
| `0x24`  | 9         | commit (returns 3 → enabled-confirm in the title flow, see #39) | **CONFIRM** |
| `0x22`  | —         | abort poll → scene state 6 | **QUIT/ABORT** |

So the actionable truth for scripting input: **up = id 1, down = id 3,
confirm = id 0x24.**  ids 2 and 4 are page nav and do nothing in the 5-item
single-column title menu (the page handlers early-out unless `stride < count`).
This corrects the provisional `docs/findings/input.md` labels (which read
`0x02 = down`, `0x03 = left` off the latch-dir numbers — but id 2 is page-up
and id 3 is the real down).  Pressing id 2 ("down" per the old label) latches
dir 2 (page-up) and visibly does nothing; the regression was diagnosed by
hooking `menu_list_latch` (`0x43ce50`) live and seeing `ready=1000 enabled=1`
yet no cursor move.

> 📍 The injected ring record is the abstract action id the poll compares
> against (`rec.id == button_id`), so injection sidesteps the DInput key→id
> map entirely — the table above is what to put in a trace, regardless of
> which physical key produces each id at runtime.

## 43. Each scene has its *own* input-manager instance — a once-cached `this` injects into the wrong ring

The poll consumer `FUN_0043c110` is `__thiscall`, so `ecx = this` is the input
manager.  Discovered live (checkpoint 13): the title scene and the new-game
**difficulty config menu** are different scenes with **different manager
instances** — caching `this` from the title's first poll and injecting there
makes the difficulty menu's presses vanish (the record sits unconsumed in the
title manager's ring while the sub-scene polls its own, empty ring).  The fix
is to inject into the *current* poll's `ecx` every time, not a cached pointer.

The two menus also poll **different id sets**: the title polls
`0x22, 2, 4, 1, 3, 0x24`; the difficulty menu polls `0x22, 1, 3, 0x24, 0x27`
(no page ids; adds `0x27`, presumably the left/right value-change for the
"Game Difficulty" row).  Down is `id 3` in both.

> 📍 TAS injection in `tools/frida/opensummoners-agent.js`
> (`installInputInjection`); when replaying input across a scene transition,
> resolve the manager per-poll, never once.

## 44. The software blend LUT is indexed with a hardcoded src-level stride of 32 — even for the 6-bit green channel

The software alpha blitter `FUN_005bd680` has three blend modes selected by
its descriptor's `+0x00` field; mode 1 is a true src×dst blend that indexes a
per-channel byte LUT by `(src_level << 5) + dst_level` (`shl ebp, 5` at
0x5bd8f8 / 0x5bd934 / 0x5bd95c).  The `<< 5` (×32) src stride is **hardcoded
for all three channels**, but in RGB565 the green channel is 6-bit
(`0x07E0 >> 5` ⇒ levels 0..63).  So for green, `dst_level` (0..63) exceeds the
32-entry "row" and bleeds into the next src row's region of the table.

Either retail's green blend LUT is laid out to absorb this (rows sized to the
real dst range, with `src_level*32` only nominal), or the green channel in
*this* descriptor is actually fed a ≤5-bit mask — TBD when the descriptor's
constructor is ported.  The literal port mirrors the `<< 5`; the LUT contents
and the descriptor build are a separate (future) chip.

Modes 0 and 2 are 1-D (`lut[src_level]` and `lut[gray]` respectively, where
`gray = (ch0+ch1+ch2)/3` via the `0xaaaaaaab` reciprocal-multiply), so the
stride quirk is mode-1-only.  All three modes skip any source pixel equal to
the colorkey arg (`cmp colorkey, src; je`).

> 📍 `zdd_alpha_blit_pixels` in `src/zdd.c`; descriptor layout in
> `zdd_blend_desc` (`src/zdd.h`).

## 45. The blit orchestrator's GDI/hardware-Blt "complex path" is dead code — `DAT_008a6ec0` is only ever written to zero

`FUN_005bd550` (the blit orchestrator every title-screen sprite draw funnels
through — the per-frame compositor `FUN_0056c180` plus the sprite wrappers
`FUN_0056c470/_4e0/_580` all call it) has two paths, branching on its last
argument (`param_10`):

- **simple** (`param_10 == 0`): lock the dest → software alpha blit
  (`FUN_005bd680`) → unlock the dest.
- **complex** (`param_10 != 0`): GetDC both surfaces, GDI-`BitBlt` the dest
  region into the scratch surface `param_10`, ReleaseDC, alpha-blit the source
  onto the scratch, then hardware-`Blt` (`FUN_005b9ae0`) the scratch back onto
  the dest — a read-modify-write so a mode-1 (src×dst) blend sees fresh dest
  pixels through GDI rather than a DDraw Lock.

**Every** caller passes the global `DAT_008a6ec0` as `param_10`.  An exhaustive
write-search of the image (`mov [0x8a6ec0], *` across all encodings: `a3`,
`c705`, `890d/15/1d/35/3d/25/2d`) finds **exactly three writes, all storing
zero**:

- `0x5623d6` `mov [0x8a6ec0], esi` inside the engine-init zero-block at
  `0x5623a0` (esi = 0 there; it's the same value stored to a dozen pointer
  globals including `0x8a93cc`),
- `0x582eb4` `mov dword [0x8a6ec0], 0` (explicit),
- (the third hit at `0x5626e3` is a `c705` whose operand also resolves in the
  same init zeroing region).

There is **no** write that stores a surface/`paint_ctx` pointer.  So
`DAT_008a6ec0` is always NULL and the complex path **never executes** in this
binary — `FUN_005b9ae0` (the hardware Blt with explicit rects) is reachable
*only* from that dead path and is therefore dead too.  `0x8a6ec0` reads as a
debug/feature toggle ("composite blends through a GDI scratch surface") that
ships permanently off.

Consequence for the port: `zdd_blit_orchestrate`'s simple path is the only one
that needs live verification; it reuses entirely-ported primitives
(`zdd_object_lock`/`zdd_alpha_blit`/`zdd_object_unlock`).  The complex path is
still ported for fidelity (`zdd_object_blt_rects` = `FUN_005b9ae0`, plus the
`zdd_dc_blit` GDI seam) but is exercised only by a host test, never at runtime.

> 📍 `zdd_blit_orchestrate` / `zdd_object_blt_rects` in `src/zdd.c`;
> `zdd_dc_blit` in `src/zdd_win32.c`.

## 46. The sprite-sheet decoder's brightness pass is per-channel ×scale÷1000 with a *reversed* byte→field mapping, gated on a slot flag

`FUN_004184a0` (the sprite-sheet decoder, ported as `ar_sprite_decode`) runs an
in-place colour transform over the freshly-decoded 24bpp sheet **before**
slicing it into frames — but only when **two** conditions hold:

1. `slot->f_08` (`in_ECX[2]`, byte +0x08) is non-zero — a per-slot "apply
   brightness" gate stamped by the registrar, **not** a global.
2. the sheet is genuinely 24bpp (`bs_get_bit_count == 0x18`).

For every pixel whose low 24 bits ≠ the magenta key `0xff00ff` (which is left
untouched — it's the transparent colour), each channel is, in order:

```
if (slot->f_18) ch = ((uint8_t*)slot->f_18)[ch];   // optional gamma/remap LUT
ch = (uint8_t)( (int)ch * scale / 1000 );           // signed idiv, trunc → 0
```

The surprise is the **byte→field mapping is reversed** relative to the field
order:

| DIB byte | channel | scale field |
|----------|---------|-------------|
| `p[0]`   | B       | `slot->f_14` (+0x14) |
| `p[1]`   | G       | `slot->f_10` (+0x10) |
| `p[2]`   | R       | `slot->f_0c` (+0x0c) |

i.e. byte 0 uses the *highest* of the three field offsets, byte 2 the lowest.
So `f_0c/f_10/f_14` are **not** a naïve "channel0/1/2" triple — read them as
(R-scale, G-scale, B-scale).  Scales are out of 1000 (1000 = ×1.0); the divide
is the same `0x10624dd3 / sar 6` signed-÷1000 magic the compositor uses for
its alpha math.

Retail reads each pixel as `*(uint*)p & 0xffffff` for the key test, which reads
one byte past the buffer on the final pixel (the LocalAlloc'd `biSizeImage`
buffer has heap slack so retail tolerates it).  The port reads the three bytes
individually instead — identical result, no out-of-bounds read under ASan.

> 📍 `ar_sheet_decode_pixels` in `src/asset_register.c`; the field roles are
> pinned in `ar_sprite_slot` (`asset_register.h`).

## 47. The slicer normalises every colour-key to magenta unless it's the `0x1ffffff` "no-key" sentinel

`FUN_004188b0` (`ar_sprite_slice`) takes the bank's colour-key (`slot->colorkey`,
`in_ECX[10]`) but **does not pass it through verbatim** to the per-frame surface
builder.  There are two arms of the format-setup switch:

- `colorkey == 0x1ffffff` — the "no colour-key" sentinel.  The builder is
  handed `0x1ffffff` unchanged and the format-setup calls use key `0`.
- any other value — after the format-setup switch the code does
  `param_5 = 0xff00ff`, so the builder **always** receives the magenta key
  regardless of what `slot->colorkey` actually held.

So a sprite bank's stored colour-key only ever selects *whether* keying is on
(`!= 0x1ffffff`), not *which* colour — the keyed colour is hardwired to magenta
`0xff00ff` (the same key the decoder's brightness pass skips, quirk #46).

> 📍 `ar_sprite_slice` in `src/asset_register.c`.

## 48. The per-cell trim scanner bounds the bottom edge differently for 8bpp vs 24bpp

`FUN_005b6f80` (`bs_trim_opaque_rect`) scans a sprite cell for the tight
bounding box of opaque (non-colour-key) pixels.  It has two depth-specific scan
loops, and they are **not symmetric** in how they decide which rows extend the
box's bottom edge:

- **24bpp** gates its per-row right-edge scan *and* its `y_top`/`y_bottom`
  update on the **global** `x_left < W` test (`cmp var_14h, edi; jge`).  `x_left`
  is a running minimum that, once any row has lowered it below `W`, **stays**
  below `W` for the rest of the scan.  So every row processed *after* the first
  opaque row — including fully-transparent ones — passes the gate and extends
  `y_bottom`.  Net effect: 24bpp `y_bottom` is always `H-1` whenever the cell
  has any opaque pixel at all (the top edge `y_top` is still tight — it's a
  `min`, set on the first qualifying row).
- **8bpp** uses a **per-row** opaque flag (`ebx`, `xor`'d to 0 at the top of
  each row, set to 1 only when that row has an opaque pixel) as the gate.  So
  its `y_bottom` is the last row that *actually* contains an opaque pixel —
  tight.

`x_left`/`x_right`/`y_top` are tight in both paths; only the bottom edge
differs.  The downstream surface builder (`0x5b9630`) derives the cell surface
height from `y_bottom - y_top + 1`, so a 24bpp cell's surface spans down to the
cell's physical bottom regardless of where its art ends, while an 8bpp cell's is
cropped to its art.  Whether this is deliberate (8bpp art is index-packed and
trimming saves more) or an oversight, the port reproduces it byte-for-byte.

> 📍 `bs_trim_opaque_rect` in `src/bitmap_session.c`; host-tested by the
> `trim_24bpp_loose_ybottom_quirk` vs `trim_8bpp_tight_ybottom` pair (same
> opaque shape, `y_bottom` = 7 vs 4).

## 49. The sprite format conversion lives in the *slicer*, not the pixel writer — the per-cell builder is a raw byte blit

The ckpt-23 sprite-pipeline note placed the display-depth format converters
(`0x5b7310`/`_74f0`/`_7270` + the 8bpp palette `0x5b7bd0`) inside the per-cell
pixel writer `0x5b9910`.  Re-reading the Ghidra decompilations (ckpt 25) shows
that is wrong: **`0x5b9910` is a plain `rep movs` byte copy** with no converter
dispatch at all.  The format switch is in the **slicer `0x4188b0`**
(`0x4189f2..0x418b45`): it converts the *whole decoded sheet* in place to the
god-object display depth (`switch [zdd+0x168]`) **once, before** the per-cell
build loop runs.  By the time `0x5b9910` copies a cell, source and dest already
share a pixel format, so the raw copy is correct.

Consequences for the port:
- The converters are `bitmap_session` methods (`bs_convert_to_16bpp` etc.),
  ported in `bitmap_session.c` — not `zdd`.
- `zdd_object_copy_cell_pixels` takes bytes-per-pixel from the **source**
  depth (`biBitCount>>3`) and clamps the per-row span to both the dest pitch
  and the source stride.
- The switch has two arms differing only in the converter's `key_color`
  argument: `0` when `slot->colorkey == 0x1ffffff` (no key), `0xff00ff`
  otherwise (and `param_5` is then forced to `0xff00ff` for the build loop —
  the same normalisation as quirk #47).

> 📍 `bs_convert_*` in `src/bitmap_session.c`; `zdd_object_copy_cell_pixels` in
> `src/zdd.c`; the switch in `ar_sprite_slice` (`src/asset_register.c`) routed
> through `ar_sheet_format_hook` (adapter `title_sheet_format` in `main.c`).

## 50. The slicer passes (cell_w, cell_h) as the trim scanner's (height, width) args

In the slicer's trim-scan loop (`0x4189a9`), `FUN_005b6f80` (the opaque-rect
scanner) is invoked as `(key, base_x, base_y, param_3, param_4, out)` where
`param_3` is the **cell width** and `param_4` the **cell height** — but the
scanner's 4th/5th parameters are `height` then `width` (it iterates
`row < arg4`, `x < arg5`).  So a non-square cell is scanned transposed
(cell_w rows × cell_h cols).  For the title banks the cells are square or the
whole sheet, so the transposition is invisible there; whether it is a latent
retail bug or the two dimensions are simply interchangeable for the bbox math,
the port mirrors the literal argument order.

> 📍 `ar_sprite_slice` trim loop in `src/asset_register.c` (passes `cell_w` as
> `bs_trim_opaque_rect`'s `height`, `cell_h` as its `width`).
