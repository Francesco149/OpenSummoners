# Engine quirks

A running collection of small things the original Fortune Summoners
engine does that are weird, charming, or inexplicable.  Each entry is
short — one paragraph or two, optional code snippet, source pointer.

> Add an entry whenever something makes you blink at the decompiler output;
> bias toward recording it before you forget *why* it surprised you.  This is
> **retail ground-truth behaviour ONLY** (offsets/formats/control-flow/UI facts
> read off the original) — port state / "deferred"/"stub" notes go in
> FRONT/HANDOFF/port-debt, per CLAUDE.md "Log engine quirks as you find them".
> Format: a numbered entry, 1–3 short paragraphs, an optional code snippet, and
> a `> 📍` pointer to the source (Ghidra address, our port file, or a
> `findings/*.md` section).  Cite the entry number in commit messages.

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

> 📍 When Ghidra's call graph shows zero callers for a callback type, fall back
> to radare2 (`af @ <addr>; pdf`) on the function-pointer write site — the
> second-line tool. Decompile-reading conventions are in CLAUDE.md "Disassembly".

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

## 51. `settings` is a bare HMODULE (sotesd.dll), not a record — and it's `DAT_008a6e74`, not sotesp

The asset registrars (`ar_register_main_sprites`/`_fonts`/`_sounds` =
FUN_005749b0 etc.) take a `settings` argument that ends up in
`slot->settings` (+0x3c) and is handed **directly** to the PE-resource
decoder `bs_decode_resource` (FUN_005b7800) as its `hModule` param —
`FindResourceA(hModule, …)`.  There is **no `+0x3c` record indirection**: the
"settings record" framing in earlier notes was wrong.  `settings` is literally
an `HMODULE`.

And that HMODULE is **sotesd.dll**: the boot driver FUN_00562ea0 passes
`DAT_008a6e74` to every registrar (0x562ea0:620-631), and `DAT_008a6e74` is
assigned the result of `LoadLibraryA("sotesd.dll")` at **0x5af5fc** — *not*
sotesp.dll.  (`docs/findings/asset-loader.md` had the three DLL handles mapped
to the wrong DAT_ slots; corrected there.)  Confirmed by resource enumeration:
all title sprite IDs (0x49f logo, 0x91b/0x91c title bg banks, the slot-0
palette seed 0x90b) exist in sotesd.dll's `DATA` type and nowhere in
sotesp.dll (which holds only `WAVE` SFX + its 12-byte signature blob 0x407).

Practical consequence for the drop-in: registering the title banks is just
`ar_register_main_sprites(zdd, /*group=*/4, hSotesd, hSotesd)` after
`LoadLibraryA("sotesd.dll")` — the `sotesp_module` parameter (slot 0) takes the
*same* sotesd handle (retail's FUN_005748c0 for idx 0 also passes
`DAT_008a6e74`); the parameter name is a historical misnomer.

> 📍 `init_sprite_banks` in `src/main.c`; `ar_register_main_sprites` in
> `src/asset_register.c`; verified against `vendor/unpacked/sotes.unpacked.exe`
> @ 0x5af5fc and the sotesd/sotesp `.rsrc` directories.

## 52. The menu cursor *breathes* — its `level_num` is a phase-8↔9 triangle, peaking at idx 16 (not 19)

The title menu's selection cursor (`FUN_0056c470` @ call site 0x56be74) is
drawn every menu frame with `level_div = 0x4b0` (1200, a constant) but
`level_num = [esp+0x20]` — which is **`local_58` in `FUN_0056aea0`, an animated
triangle wave**, not a constant.  The phase FSM oscillates it
(`56aea0.c`:366-384): phase **8** does `local_58 += 0x32` (50) each *update*
step until it saturates at 1000, then flips to phase **9**, which does
`local_58 -= 0x32` (clamped ≥0) until it hits 0, then flips back to 8.  So the
cursor brightness ramps `0→1000→0` in steps of 50, one step per update step
(it appears ~2× per value in a Flip-indexed trace because of the pacing FSM's
update/draw split — see the `0x56c930` half-rate).

Because `idx = (local_58 * 20) / 1200`, the cursor's blend index sweeps **0..16,
peaking at 16** — it never reaches 19, and at `local_58 ≤ 0` the draw
early-returns (the cursor is invisible at the bottom of each breath).  Driving
it to a static idx-19 full-add (the pre-ckpt-28 port) made the cursor uniformly
**over-bright** vs retail at every phase — the parity-ledger R1 residual.

Measured live: `tools/frida_capture.py --cursor-probe` hooks `FUN_0056c470` and
logs the per-Flip `level_num`/`level_div`; the 0→1000→0 step-50 sequence is in
`runs/cursor-probe/cursor_level.jsonl`.  The port already computed the value as
`title_fade_state.menu_fade`; ckpt 28 just threads it into the cursor draw
(`cmd->level = level_num`, `cmd->alpha = level_div`).  At equal `menu_fade` /
`local_58` the port menu is **bit-exact** vs retail (parity-ledger #1).

> 📍 `title_render_menu` / `title_render_step` in `src/title_scene.c`;
> `TITLE_DRAW_MENU_CURSOR` arm in `src/title_sink.c`; probe in
> `tools/frida/opensummoners-agent.js` (`installCursorProbe`).

## 53. Frida harness must spawn the *unpacked, co-located* exe — packed `sotes.exe` stalls (0 frames)

`vendor/original` is a symlink to the game dir, so `vendor/original/sotes.exe`
is the **packed Steam-DRM** exe — spawning it under Frida stalls in the launcher
(message pump never ticks, `last_frame=-1`, 0 Flips).  The DRM-free PE is the
Steamless output `vendor/unpacked/sotes.unpacked.exe`.  But the engine resolves
its asset paths (`config.dat`, `sotesd.dll`, …) relative to its **own module
directory** (`GetModuleFileName`), *not* the cwd — so the unpacked exe must sit
*inside* the game dir next to those files, or it pops "The file is not found.
…sotesd.dll" and stalls.  Fix: `setup.sh`/the harness copy the unpacked exe to
`vendor/original/sotes.unpacked.exe` (= game dir) and spawn that; that is now
`frida_capture.py`'s default `RETAIL_EXE`.

> 📍 `tools/frida_capture.py` `RETAIL_EXE`/`ASSET_CWD`; the working golden run
> `runs/calltrace-title/run.json` used `sotes-unpacked-880680.exe` in the game
> dir — same principle.

## 54. The title pace machine is a fixed-timestep accumulator that must be spun in a tight loop — one render *per present*, not one pace-step per throttled frame

`FUN_0056aea0`'s outer `do/while` (ported as `title_pace_step`) is a classic
fixed-timestep accumulator: each iteration either does one *update* (advance the
phase FSM, ~free) or one *render* (present + refill the budget with elapsed
wall-clock, `b += now − anchor`, clamp 100 ms).  Retail spins this loop **as
fast as the CPU allows**, blocking only on the present (Flip → display vblank).
So updates run at the budget-gated ~60 Hz and renders run at the display refresh
(measured **~127 flips/s**), giving **~2.2 duplicate flips per scene state**
(`--cursor-probe`: each `menu_fade` value spans ~2 consecutive flips).

The port (`src/main.c`) originally called the pace machine **once per
`frame_limiter`-throttled (16 ms) main-loop iteration**.  That violates the
accumulator's contract: it stretches the ~free update spin to 16 ms each, so
`now` advances 16 ms *per update*, the refill `b += now − anchor` compounds, and
the loop settles at **~6 updates per render** — the port rendered only **90 of
the intro's ~528 update ticks** (dropping ~5/6 of the fade frames; choppy).
Critically this is *not* a wall-clock rush: the port reached the menu at ~9.9 s
vs retail's ~9.2 s — the divergence was purely in *which* frames got rendered
(and the Flip index).

**Fix:** drive it like retail — spin `title_drive_step` (no per-step sleep,
detect a present via `g_present_frame` change) until one frame is presented,
*then* `frame_limiter` gates the presented-frame rate.  → 1 update per render,
every scene state rendered, phase curve = canonical 51/102/153/254/275/316/437/
528, wall-clock unchanged (~9.2 s).  Flip-index-exact parity with a golden is the
capture rig's refresh (~127 Hz) and is **not portably reproducible**; the
distinct-content sequence is, and now matches.

> 📍 `main_loop_body` in `src/main.c` (the spin-until-present loop);
> measurement: `--pace-probe` in `tools/frida/opensummoners-agent.js` +
> `tools/frida_capture.py`, and the `pace:` phase log in `src/main.c`.
> General lesson: a fixed-timestep accumulator ported into a *host* loop must
> keep the engine's call cadence (tight spin, present-gated), not be re-paced by
> the host's own frame limiter — the two throttles compound.

## 55. Windowed (mode 2) present — the port BitBlts the DESKTOP (`GetDC(NULL)`); retail BitBlts its WINDOW (`GetDC(hwnd)`)

`--hide-window` strips `WS_VISIBLE` so the game window never shows, yet a
640×480 rectangle still flickered on the real desktop every frame.  Cause: the
port's `zdd_present` mode 2 (`src/zdd.c` case 2 → `zdd_desktop_present`) does
`BitBlt(GetDC(NULL) /* desktop */, screen_pos_x, screen_pos_y, w, h, surfaceDC,
…)` — it paints straight onto the desktop at the window's logical position,
independent of window visibility.  So a hidden window doesn't stop the paint.

A live diagnostic (Frida: hook `GetDC`/`GetDCEx`/`GetWindowDC`/`BeginPaint`
during the Flip dispatcher `FUN_005b8fc0` under `--hide-window`) showed
**retail's mode-2 present calls `GetDC(hwnd=<real window>)`, never `GetDC(NULL)`**
(the desktop-DC redirect never fired across 1500 flips).  So retail paints into
its *own window* DC — hidden ⇒ nothing on the visible desktop ⇒ **retail does
not flicker**; only the port did.

**RESOLVED (ckpt 31) — disasm-confirmed + fixed.** `FUN_005b8fc0`'s mode-2 path
decoded:
```
0x5b90ad  call 0x5b94e0           ; surface GetDC → src HDC
0x5b90b7  push ebx ; call [0x5cc1c4]   ; GetDC(ebx) — ebx from [esp+0x14] = the
                                       ;   window handle, NOT push 0 (desktop)
0x5b90ea  call [0x5cc034]         ; BitBlt(GetDC(hwnd), [esi+0x138],[esi+0x13c],
                                  ;   w,h, srcHDC, 0,0, SRCCOPY)
0x5b90f2  push edi;push ebx;call [0x5cc1c0]  ; ReleaseDC(hwnd, dc)
```
So retail BitBlts into the **window DC** at `screen_pos_x/y` (= `[esi+0x138/13c]`,
0 in windowed mode → client origin). The port's `GetDC(NULL)` desktop blit was a
mismodel (the old "`hWnd=NULL` @0x5b8fc5" reading was wrong — `0x5b8fc5` zeroes
the surface-GetDC *out-slot* `[esp+8]`, not the GetDC arg). A `GetDC(NULL)`
desktop blit on a **visible/focused** window fights DWM's compositor (and the
`COLOR_WINDOW` erase brush flashed on activate) → a periodic flicker the user
hit after clicking to focus.

**Fix (ckpt 31, user-confirmed flicker gone):** `zdd_window_present`
(`zdd_win32.c`) = `GetDC(hwnd)`+`BitBlt(0,0,w,h)`+`ReleaseDC(hwnd)`;
`zdd_present` case 2 uses it when a present hwnd is bound
(`zdd_set_present_hwnd`, set in `main.c` after window+DDraw init) and falls back
to the desktop blit when unbound (headless/host). Window class `hbrBackground =
NULL` + `WM_ERASEBKGND` returns 1 (the present repaints the whole client each
frame). The ckpt-29 `--hide-window` present-skip stays as belt-and-braces.

> 📍 `zdd_window_present`/`zdd_desktop_present` (`src/zdd_win32.c`),
> `zdd_present` case 2 + `zdd_set_present_hwnd` (`src/zdd.c`), `drive_present` +
> window class + `WM_ERASEBKGND` (`src/main.c`).

## 56. The two intro logos are NOT mystery container fields — they're MAIN frames[1]/[2], and the logo handler IS the sprite-level wrapper

Quirk #40 read the studio/title logos as "fields at +4/+8 of `*(*(*0x8a7658))`,
not pool assets". The r2 disasm of the logo handlers (`0x56bb5c` studio /
`0x56bbd4` title) shows what that walk actually is. With `S = [0x8a7658]` the
MAIN sprite slot (pool 19): the handler does `D = *S; F = *D; logo = *(F + 4)`
(studio) or `*(F + 8)` (title). But `F = *(*S)` is exactly the slot's **frames
array base** that `FUN_00418470(id)` indexes (`eax=[*S]; eax[id*4]`), so `+4`
== `frames[1]` and `+8` == `frames[2]`. **The logos are just MAIN-bank
frames 1 and 2** — the same bank/decode path as the press-button (2..4) and
menu (5/6) sprites, reached by an open-coded `frames[]` index instead of the
`0x418470` call (with the same lazy `FUN_004184a0(0)` decode guard).

And the whole handler is **bit-identical to the sprite-level wrapper**
(`0x56c4e0`, `TITLE_DRAW_SPRITE_LEVEL`): `fade<=0` → skip; else
`0x448c80(fade,1000)` over the **same** ramp table (`0x8a9308` = ramp_b);
ramp 0 (idx<0 / idx>=20 / empty slot) → plain keyed blit (`0x5b9b70`), nonzero
→ alpha blit at dst `(metric_0c, metric_10)`, src `(0,0)`, full w/h. The only
difference is the `0x5bd550` 10th arg — the logo's alpha leaf (`0x494e10`)
passes `[0x8a6b60+0x360]` where `0x56c4e0` passes `[0x8a6ec0]` — and that arg is
**pixel-irrelevant**: a fade-matched diff of the studio logo (phase 0, fade 640)
and the title logo (phase 3, fade 820) against retail goldens is **`differ_px=0`**
(parity-ledger #2/#3, ckpt 30).

So `title_render_logo` now emits one `TITLE_DRAW_SPRITE_LEVEL` (frame 1 or 2,
raw fade) instead of a bespoke `TITLE_DRAW_LOGO`. This **fixed a real bug**: the
old code branched on the scene-side `ramp` param (`fade_ramp`), which `main.c`
never populated (always NULL), so it always took the alpha-0 clear branch and
the logos popped in fully **opaque with no fade**. Routing through the sink's
populated `ramp_b` restores the fade.

> 📍 `title_render_logo` in `src/title_scene.c`; the `TITLE_DRAW_SPRITE_LEVEL`
> case in `src/title_sink.c`. Lesson: when a decompile shows a struct walk like
> `*(*(*g) + k)`, check whether `*(*g)` is an array you already index elsewhere
> — the "field" may just be `array[k/4]`. And a "special" handler that turns out
> to share a generic wrapper's exact control flow should be folded onto it, not
> reimplemented (reuses the bit-exact-validated path).

## 57. The phase-7 "sparkle" is two systems: a subtitle-reveal sweep (render half) + an additive particle spawn (update half)

`FUN_0056bcf7` (phase 7) is usually called "the sparkle", but it is **only the
render-half subtitle reveal**: it copies 4×48 vertical slivers of the menu-bg
sprite (MAIN frame 5) at src `(x,416)`→dst `(x,416)` via `0x56c580`, `x` stepping
192..<416 by 4, alpha = `ramp_b[idx]` of `min(7·fade − 100·i, 1000)` (opaque
once that saturates). This wipes the "Secret of the Elemental Stone" subtitle in
column-by-column. Wired ckpt 30 as `TITLE_DRAW_SPARKLE`; **verified bit-exact**
at full reveal (the subtitle banner matches the golden exactly).

The *twinkling* white sparkle dots scattered over the lower art are a
**separate** subsystem — the update-half `FUN_0056c070` particle spawn (phase
7's `title_scene_hooks` call, still stubbed). At fade 1000 the only port↔retail
residual is those particles (1208 px, 96.6 % retail-brighter — additive dots the
port lacks). Don't conflate the two: the reveal sweep is done; the particle
twinkle is the open `0x56c070` thread.

> 📍 `title_render_sparkle` / `TITLE_DRAW_SPARKLE` (`src/title_scene.c`,
> `src/title_sink.c`). The sparkle wrapper `0x56c580` already had the right
> signature in `title_render.c`; the fix was the cmd encoding (carry the raw
> clamped level + column, let the sink index `ramp_b`) — the old encoding tried
> to round-trip a 64-bit blend-descriptor pointer through a 32-bit field.

## 58. The phase-7 particle twinkles "evaporate upwards" — spawn, draw, and a per-frame rise/age/cull are three separate sites; the seed is `srand(time())`

The `FUN_0056c070` particle subsystem (quirk #57's update half) is **four** call
sites inside `FUN_0056aea0`, over one cap-500 pool (`DAT_008a92b4`, allocated
`operator_new(8)` header + `operator_new(14000)` = 500×`0x1c` array):

- **spawn** `0x56c070` (called `0x56b51b`, phase-7 case while `uVar15 < 0x352`):
  append one particle at the reveal edge — `x = (rand·0x10/32768 + (uVar15·0xe0/900
  + 0xc0))·100`, `y = (rand·0x18/32768 + 0x1a0)·100`, **`+0x08 = rand·200/32768`
  = upward velocity**, `+0x0c = +0x0e = rand·0x14/32768 + 0x14` (∈[20,39]) =
  lifetime/anim. Four `rand()` draws, in order x/y/vel/anim.
- **update** `0x56ba69` (runs **every** update tick — both watchdog branches at
  `0x56ba0e` fall into it): for each live particle, back-to-front:
  `y_num -= vel; vel += 2` (rises, accelerating up → "evaporate upwards"),
  then `anim_num != 0 ? anim_num-- : cull`.
- **cull** `0x56c030` (called `0x56baae`): swap-remove — `count--; entries[i] =
  entries[count]`. Index-safe because the update iterates back-to-front with the
  count latched once (`edi = count-1`).
- **draw** `0x56c180` (called `0x56bed5`): blits each particle at frame
  `frame_base − clamp((anim_num·frame_count)/anim_div, fc−1) + fc − 1`. Since
  `anim_num` starts `== anim_div` (frame 0) and counts down to 0, the frame walks
  **0→7** over the lifetime — the sparkle's fade/shrink animation. Bank `0x15`
  (=pool 21 = `g_ar_sprite_slots[8]` = resource `0x91d`), blend `ramp_a[16]`
  (`alpha_level 800`).

So a twinkle: bursts at the sweep edge, rises (accelerating) while animating
frames 0→7, and is culled at lifetime end — it does **not** accumulate. The
first cut omitted the update entirely → frozen, piled-up, over-bright (8277 px
diff); porting the update closed it to `differ_px=0` (parity-ledger #4).

**Determinism:** the engine RNG seed `DAT_008a4f94` is `srand(time(NULL))` at
boot (`FUN_00562210` `0x56227a`; `FUN_005bf6df` is `time()`), so retail's twinkle
stream is **wall-clock-random and not reproducible run-to-run**. The port pins a
fixed seed by default (`OSS_RNG_DEFAULT_SEED 0x4f5347`, `OPENSUMMONERS_RNG_SEED`
overrides) and the harness pins retail's seed to match at the first spawn
(`--seed-pin`, default on). With both pinned, the twinkles are bit-exact at a
matching update-tick. NB the *flip* at which a given tick renders is **not**
reproducible (intro pacing jitters run-to-run + the R3 ~2.2× render-rate), so
align captures by the `subtitle_anim_start` TAS anchor (first spawn) + tick.

> 📍 `src/title_particles.{c,h}` (pool + spawn + update + cull), `src/rng.{c,h}`
> (the LCG), `src/title_scene.c` (`update_particles` hook), `src/main.c`
> (`g_particles` + seed pin), `tools/frida/opensummoners-agent.js`
> (`installSparkleAnchor`).

## 59. The menu-input gate is opened by the post-update side effect `FUN_0056c930` (the node `+0x54` ramp), NOT the per-entry update `0x43c2e0`

The title menu renders bit-exact but is **dead to input** until one specific
side effect runs.  `menu_list_latch` (`FUN_0043ce50`, quirk #34) refuses every
nav action while `sub->ready != 1000`, where `sub` is the controller's `+0x00`
back-pointer to its **parent node** (quirk #34's spawn-overlay) and `ready` is
that node's `+0x54` ramp.  `menu_node_build` zeroes `+0x54`, so the gate starts
**closed** — the menu cannot latch a selection on the frame it spawns.

What ramps `+0x54` to 1000 is the title scene's **post-update** side effect
`FUN_0056c930` (the `local_64`==8/9 tail's `0x56c930` call), NOT the per-owner-
entry update `0x43c2e0` that runs right after it.  `0x43c2e0` only *reads*
`+0x54` (it animates the active node's *child* widgets, gated on `+0x54>=1000`
or `+0x58!=0`); it never writes it.  `0x56c930` is the menu-node **transition**
updater: a `for i in [0, owner->count)` loop over `owner->entries[i]` that, for
each active node (`+0x04 != 0`), dispatches on the node's transition mode
(`+0x1c`):

- **mode 1** (`+0x1c==1`, what `menu_node_build` sets) — the steady input-gate
  fade: `+0x54 += 50`/frame toward 1000 while `+0x50` is set ("in"), or
  `-= 40`/frame toward 0 while clear ("out", tearing the node down at <=0 via
  `0x56cc10`).  So a freshly-spawned menu becomes navigable **~20 update frames
  after it appears** (0 → 1000 at +50/frame).
- **mode 0 / mode 2** — submenu *dismiss* / *slide-to-target* animations (snap
  or lerp + `0x49a340`/`0x49a2f0`/`0x49a470`); the title's single node never
  uses them.

This is why an injected `--input-trace` DOWN/confirm did nothing in the port
until `0x56c930` was wired: the latch gate was permanently closed.  Porting the
mode-1 ramp (and routing it as the drive's `post_update`) made the title menu
**interactive** — injected nav moves the cursor (`0->1->2->3...`, both
directions) and confirm on row N returns that row's action id (Start `0x1a`,
… Exit `8`).  Note the gate (`+0x54`, +50/frame, open ~flip 547) opens *before*
the cursor becomes visible (the main `fade==1000` draw gate, +20/frame, ~flip
577), so presses land before the highlight appears — align demos accordingly.

> 📍 `FUN_0056c930` @ `0x56c930` (607 B); ported (mode-1 arm) as
> `menu_owner_transition_step` in `src/menu_list.c`.  Wired as the drive's
> `post_update` (`src/main.c` `drive_post_update`).  Diagnostic:
> `--menu-trace` logs cursor-row changes (`src/title_sink.c`).  See quirk #34
> (the latch gate) and `findings/new-game-flow.md` (the reference trace).

## 60. The title runner's re-display arg (`local_164`/`param_1`) does NOT skip the intro — the studio/title intro REPLAYS on every return to the title

The boot driver `FUN_00562ea0`'s outer `do { } while(true)` loop (562ea0.c:658-735)
calls the title runner `FUN_0056aea0(local_164)` each iteration, where `local_164`
is non-zero (set to 1) on every iteration after the first.  It is tempting to
read this re-display arg as "skip the intro, jump to the menu" — **it is not.**

`FUN_0056aea0` re-enters with its phase FSM local `local_64 = 0` every call, so
it **restarts at phase 0 and replays the full studio→title intro** (logos +
subtitle sweep + sparkles) each time you return from a sub-scene.  `param_1`'s
*only* effect is at the skip-to-menu block (56aea0.c:177): the gate is
`local_64 < 8 && (local_64 != 0 || param_1 != 0)`, and the jump itself is still
**conditioned on a fresh recent button press** (56aea0.c:182: bails unless the
input record is a press `[2]==1` newer than 100 ms).  So `param_1 != 0` merely
lets a press *during phase 0* skip the replay; with `param_1 == 0` only phases
1..7 honour a skip-press.  No press ⇒ the intro plays in full either way.

The port models this faithfully: `title_scene` `skip_intro` IS `param_1`
(`title_scene.c:729` gates the same skip-on-`input_any_fresh_press`).  The
post-title dispatch (`src/app_flow.c`, ckpt 33) rebuilds the title drive with
`skip_intro=1` on every re-entry (matching `local_164=1`); a captured re-entered
frame shows the title art **fading in again from black** (phase 0→8), confirming
the replay.  This is correct retail behaviour, not a port regression.

> 📍 `FUN_0056aea0` @ `0x56aea0`; the skip gate at 56aea0.c:177/:182.  Ported:
> `title_scene.c` (`skip_intro`), `src/app_flow.c` + `reenter_title`
> (`src/main.c`).  See quirk #54 (intro pacing) and `findings/new-game-flow.md`.

## 61. Dynamic menu text is GDI `TextOutA`, not a sprite font — and the glyph layout builder's first two Ghidra params are swapped

Two findings from porting the text/glyph layout builder (`FUN_0040fa00`):

**(a) Text is rendered through Win32 GDI.**  `ar_register_fonts`
(`FUN_00579bd0`) builds 8 real `HFONT`s (`CreateFontIndirectA`); the renderer
`FUN_0048e200` `SelectObject`s one into the back-buffer HDC and `TextOutA`s
each glyph (monospace 7 px/char advance, with a 2-pass drop shadow).  So the
drop-in renders dynamic menu/narration text by calling **real GDI** — no
glyph rasteriser needs porting.  (The title top-level menu does NOT use this:
its labels are baked into the menu-bg sprite.  Dynamic text — new-game config,
options, prologue narration — is what needs the pipeline.)  The builder
(`0x40fa00`/`0x40f800`) and renderer (`0x48e200`) all operate on the SAME
`menu_ctrl`/`menu_node` object already modelled in `menu_list.h` (descriptor
`+0x174`, per-column `entries` `+0x178`, `rows` `+0x17c`, colour config
`+0x180`), so the text system is not a new container — just a builder + a GDI
draw hung off the existing menu object.

**(b) `FUN_0040fa00`'s param_1/param_2 are ROW/COL, not the COL/ROW Ghidra
shows.**  The decompile reads `FUN_0040fa00(uint col, int row, char *str)`,
but the caller `FUN_0040f800:31` passes `(new_row, col_iterator, &text)` and
the body indexes `rows[param_1].cells[param_2]` (param_1 strides by `0x10` =
`menu_row`, param_2 by `0x18` = `menu_cell`).  So **param_1 = row, param_2 =
col**.  The bounds check `param_2 < hdr[+8]` is therefore `col < column-count
(alloc_b)` and `param_1 < hdr[+0x10]` is `row < row-count (count)` — which only
makes sense with the corrected naming.  The port (`glyph_cell_layout`) uses
`(row, col)` directly.

> 📍 `FUN_0040fa00` @ `0x40fa00`, `FUN_0040fd20` @ `0x40fd20` (ported,
> `src/glyph_text.{c,h}`, ckpt 34); renderer `0x48e200`, font reg
> `FUN_00579bd0` (`asset_register.c`).  See `findings/text-glyph-pipeline.md`.

## 62. The GDI text renderer's `this` is the CHILD node; the parent supplies the x/y base; and three "label" display-config fields are reinterpreted as COLORREFs on dead paths

Three findings from porting the renderer `FUN_0048e200` (`glyph_grid_render`):

**(a) `this` is the child controller node, not the parent.**  The paint walker
`FUN_0048c820` (the menu render loop) calls `0x48e200` with **ECX = the child
node** (`iVar1->children[k]`, the node whose `+0x08` mode == 1) and passes
**param_4/param_5 = the *parent* node's `+0x0c`/`+0x10`** (its on-screen x/y).
So the renderer reads the *child's* list/rows/entries (`+0x174..+0x17c`) and
display config (`+0x180..`), but positions everything relative to the *parent's*
origin.  `menu_node_build` re-zeroes each child's `field_14`/`field_18`
(`+0x14`/`+0x18`), so the ruby pass (gated on `field_14 != 0`) is OFF for the
basic menus — the new-game/options menus draw a single GDI text pass + drop
shadow, no furigana.

**(b) The drop shadow is two offset copies of the glyph row.**  Before the main
`0x48e860` pass, the renderer inlines that same per-glyph `TextOutA` loop
**twice** in the shadow colour (`node+0x184`): once at `(x, y+1)` and once at
`(x+1, y)`.  Net effect: a 1px down-right drop shadow under every selectable /
focused row.  Disabled rows (`row.flag8 == 0`, type != 1) skip the shadow.

**(c) `node+0x188`/`+0x194`/`+0x198` hold POINTERS but are read as COLORREFs.**
The builder `FUN_0040f3e0` stores label/string VAs there (`&DAT_00677b98`,
`&DAT_008090a9`), yet `0x48e200` reads them as `local_38`/`local_34` colours in
the **disabled-row** branch (`+0x194`/`+0x198`) and as the **ruby secondary**
colour (`+0x188`).  Both are dead for the current menus (selectable rows have
`flag8 == 1`; `field_14 == 0` disables the ruby pass), so the pointer-as-colour
reinterpret never reaches a visible `SetTextColor` — only `+0x180` (normal
text), `+0x184` (shadow), `+0x18c`/`+0x190` (focused text/secondary) are live.
The port reads all of them faithfully by offset; the dead ones simply don't draw.

NB for host-testing: on the 32-bit target `menu_node` and its embedded
`menu_ctrl` alias at `+0x00` (so `+0x174..+0x17c` name both views), but the
64-bit host layouts diverge (8-byte pointers) — the tests keep a `menu_ctrl`
(owns the built container + laid-out text) and a `menu_node` (references its
arrays + carries the display config) as **separate** objects, which is exactly
what the renderer sees on-target.

> 📍 `FUN_0048e200` (`glyph_grid_render`), `FUN_0048e860` (`glyph_row_draw`),
> `FUN_0048e6d0` (`glyph_ruby_draw`) — ported `src/glyph_render.{c,h}` +
> `glyph_render_win32.c` (ckpt 35).  Caller `FUN_0048c820`.  See
> `findings/text-glyph-pipeline.md`.

## 63. The GDI text renderer is verified bit-exact against retail's LIVE TextOutA stream — every parameter matches; and retail has a debug HUD overlay using a *different* (3×3-outline) text routine

Closed the ckpt-35 "render a string, diff" gate by hooking **`gdi32!TextOutA`**
in retail (`frida_capture.py --textout-probe`) and capturing the **real
per-glyph draw stream** of the **new-game config menu** — the first scene that
exercises the GDI text path (`glyph_grid_render` `0x48e200`).  Drove retail
there by pressing **Start (id 0x24) at flip ~400** (the title menu is
interactive ~flip 200–700, then auto-enters a gameplay **demo** by ~flip 900,
so the old `new-game-through` trace's flip-2050 press landed in the demo — see
`trace-retimed.jsonl`).

Retail's menu renders **"Game Difficulty  1:Easy" / "Auto-guard  On" /
"Start Game"** + a help box, and every parameter the port's renderer uses
matches retail's measured output **bit-for-bit**:

| parameter            | retail (measured)                      | port |
|----------------------|----------------------------------------|------|
| font                 | Courier New **7×18** (slot 3)          | ✓ `ar_register_fonts` font[3] = `ar_make_font(7,0x12,0)` |
| bk mode              | **TRANSPARENT** (1)                     | ✓ `SetBkMode(TRANSPARENT)` |
| per-glyph advance    | **7 px** (label glyphs at Δx=7)         | ✓ 7 px/byte |
| draw primitive       | one **`TextOutA`** per glyph            | ✓ `glyph_row_draw` |
| drop shadow          | 2 copies **(x+1,y)** and **(x,y+1)**    | ✓ quirk #62(b) |
| shadow colour        | **0xa8b9cc**                            | ✓ `node+0x184` |
| normal text colour   | **0x3e537d**                            | ✓ `node+0x180` |
| focused text colour  | **0xf08080**                            | ✓ `node+0x18c` |
| row pitch            | 28 px (y = 56/84/112)                   | — (builder geometry) |
| cell origin x        | 72                                      | — (builder geometry) |

Because GDI rasterization is deterministic given an identical `HFONT`
(`CreateFontIndirectA(LOGFONTA)` is byte-identical both sides via `ar_make_font`)
+ identical (x, y, colour, bk mode, glyph bytes), this is a **bit-exact**
result for the glyph pixels — the renderer port is correct.  The remaining
end-to-end *stream* diff (port emits the same `TextOutA` calls) waits on the
**new-game menu BUILDER** port, which supplies the cells/geometry the renderer
walks (row pitch 28, origin x=72, the value-column offsets, the focused row).

**Bonus discovery — a debug HUD overlay.**  During the title's attract **demo**
the engine paints a GDI **debug HUD** (a column of stat numbers + `"Bonus Mode"`
+ `"The Game Play by Computer operator only."` + `"- Please Hit Any Key -"`) in
fonts **6×14 / 7×16 / 7×18 Courier New**.  Its text routine is **NOT**
`glyph_grid_render`: it draws each string as a **full 3×3 outline** — 9 shadow
copies (all 8 neighbours + centre) in `0x202020`, then the centre once in
`0xffffff` (or `0x808080`) — i.e. an *embossed/outlined* helper, distinct from
the menu renderer's 2-copy drop shadow.  Not parity-relevant to the title/menu
port (the port doesn't render the demo HUD), but a useful specimen of a second
GDI-text path if the demo mode is ever ported.

> 📍 Tool: `frida_capture.py --textout-probe [--textout-frames LO,HI]` →
> `<run>/textout.jsonl` (deduped distinct glyph draws: x/y/bytes/colour/bkmode +
> selected `LOGFONTA`).  Ground-truth capture:
> `runs/textout-start/.../frames/frame_00450.png` (the retail config menu).
> Caveat: under the hidden-window turbo harness retail runs ~15 flips/s (vs
> ~127 native), so the menu is at flip ~400–1500, not the trace's old 2000+.

## 64. The new-game config menu BUILDER reproduces retail's TextOutA stream — the whole text pipeline is now closed end-to-end (build → render → bit-exact stream)

Ported the construction half of the new-game ("Start") config scene
(`FUN_00564780` **case 0x24** + the grid setup `FUN_00411940` performs) into
`src/newgame_menu.{c,h}`, supplying the cells/geometry the (already bit-exact,
quirk #63) GDI renderer walks.  Run through `glyph_grid_render` at the box
base **(x=32, y=32)**, the built grid emits the captured retail `TextOutA`
stream **draw-for-draw** — all **129** menu-region glyph draws (3 rows ×
{col0 label, col1 value} × {shadow-down, shadow-right, main}, Start-Game's
empty value column skipped) match
`goldens/retail-newgame-config-textout.jsonl` exactly.  This closes the
"render a string, diff" gate **end-to-end**: the text pipeline now builds AND
renders the new-game menu bit-identically to retail.

**The geometry, fully reconciled** (the ckpt-36 "builder geometry" TODO):

```
cx = entry[col].pos + x + node->field_c        cy = entry[col].field4 + node->field_10 + 28*disp + y
```

| source                      | value | feeds                                  |
|-----------------------------|-------|----------------------------------------|
| box node position (param_4/5 of `0x48e200`) | x=32, y=32 | render base |
| `menu_ctrl_build(grid,0x28,0x18,…)` | field_c=**40**, field_10=**24** | text inset |
| ctor `entry[0].pos`         | 0     | col 0 origin → **x=72** |
| case-0x24 `*(entries+0x24)=0xa0` | entry[1].pos=**160** | col 1 origin → **x=232** |
| `menu_node_build` child `+0x1ac` | 0x1c=**28** | row pitch → y=56/84/112 |

So the menu builder is **not** a new container — it is `menu_ctrl_build`
(3×2 linear grid, type 0, stride 3) + a one-field entry override
(`entry[1].pos = 0xa0`) + three `menu_grid_append`s + a value-fill loop, over
the same `menu_ctrl`/`menu_node` model the renderer already reads.

**`menu_grid_append` (`FUN_00412160`) is a thin append:** count++, stamp the
row's kind/action/`flag8=1`, then its per-column refresh loop is *byte-for-byte*
`FUN_00411f40`'s body (re-layout obj0 / re-zero obj54 / re-zero+clamp obj20),
so it delegates to the ported `menu_row_finalize` — and on a **fresh** append
every cell sub-object is NULL, making that refresh a **no-op** (the guarded
bodies + their dead `operator_new` are all skipped, quirk #36 again).  The only
real work is the final `0x40fa00` laying the label into column 0; the value
column is filled afterwards by the run loop's first block (kind-0 rows only).

**Row kinds:** `field0` 0 = an *option* row (label col 0 + a value the build
fills into col 1, e.g. Difficulty/Auto-guard); `field0` 3 = an *action button*
(col 0 label only, no value — "Start Game").  The value strings come from
`FUN_00566a80(id, setting)` keyed on the **current setting** retail reads from
the settings record `*DAT_008a6e80 + 0xc + id*0x1c`; the golden was captured at
the defaults difficulty **10**→"1:Easy", auto-guard **1**→"On".

Scope: this is the **builder + grid** only.  The interactive run loop
(`FUN_00565810`/`0x565d10` nav, value toggles via id 0x27, the tooltip text
node `0x566850`, the box widget tree `0x411940` builds, and the Start→game
transition `0x564160`/`0x59ec30`) is **not** ported — the next rock once the
scene is wired as a drive.  Partial ports: `FUN_00564780` (case 0x24 only),
`FUN_00566570`/`FUN_00566a80` (the id 3/4 arms).

## 65. The new-game config run loop's input contract: button `0x24`→confirm (0xc), `0x27`→BACK (0xb) — there is NO in-place value toggle; option values change only via the picker submenu

Ported the Win32-free heart of the case-0x24 run loop (`FUN_00564780`'s
post-build loop, 564780.c:597-669) into **`src/newgame_scene.{c,h}`** — the
focused-row tooltip selection, the pump-result dispatch, and the value-refill —
mirroring the `title_scene` (pure) vs `title_drive` (Win32) split.  The real
per-frame pump (`0x565d10`, with its GDI present + the `0x43bca0` input scan)
stays in the drive (next unit).

**The pump→action contract.**  `0x565d10` collapses every per-frame outcome
into one of three codes that `FUN_00564780` dispatches on:

| pump code | meaning                  | run-loop action                          |
|-----------|--------------------------|------------------------------------------|
| `0xd`     | cursor moved / page      | re-render (re-resolve the tooltip)       |
| `0xc`     | confirm/OK button        | act on focused row: kind 0 → open picker; kind 3 (`0x1e`) → start the game |
| `0xb`     | back/cancel button       | `local_434=0xb`, teardown → return 0xb → caller (`0x564160`) sets iVar3=6 → back to title |

**This corrects new-game-flow.md's earlier "id 0x27 = value left/right" guess**
(which that doc itself flagged as directionally unverified).  Tracing the
chain: `0x43bca0` polls `FUN_0043c110(t, 0x24)` → `menu_list_latch(9)` and
`FUN_0043c110(t, 0x27)` → `menu_list_latch(10)`.  The nav engine (quirk in
`menu_list.c`) names dir 9 "cancel" (returns 3) and dir 10 "confirm"
(returns 4); `0x43bca0`'s tail + `0x565d10` then map **return 3 → `0xc`**
(the scene's *confirm*) and **return 4 → `0xb`** (the scene's *back*).  So the
nav engine's internal 9/10 labels are inverted relative to this scene's meaning,
but the net is unambiguous: **`0x24` confirms, `0x27` backs out.**  There is
**no in-place value toggle** — an option's value changes only by confirming
into its **picker submenu** (`FUN_00567ba0` default arm for id 3/4: a nested
grid + its own `0x565d10` loop).  `newgame_scene_dispatch` returns
`NEWGAME_OPEN_PICKER` for a kind-0 confirm; once the (deferred) picker commits,
`newgame_scene_set_option` re-lays that row's value column (the run loop's
value-refill block, 564780.c:367-385).

**Start Game (kind 3, `0x1e`):** confirm → `FUN_00568b40(0x1e)` is a **no-op**
for `0x1e` (its body only handles `0x1c`/`0x1d` Yes/No confirmations), so the
action switch falls straight to case 0x24's `goto teardown` → `FUN_00564780`
returns **0** → `0x564160` proceeds to the Elemental-Stone intro
(`FUN_005642e0`) then `0x59ec30` (game proper).

**Open for live confirm (Frida):** only the *physical-key identity* of ids
`0x24`/`0x27` (what `FUN_0043c110` maps them to) — the decompile→code-path
mapping above is solid, but which controller/keyboard input each id reads is
worth a `--input-trace` sanity check when the drive is wired.

Ported: `FUN_00566850` (option tooltip, id 3/4 arms) into `newgame_menu.c`;
the case-0x24 run-loop body + tooltip switch + value-refill into
`newgame_scene.c`.  Still bare-VA (unported): `0x565d10`, `0x43bca0`,
`0x567ba0`, `0x568b40`, `0x564160`, the box widgets `0x411940`.  698 host tests
(+4); ledger 155/1490 (+1: `0x566850`).

## 66. The new-game config menu's retail rendering: Courier New 7×18 (a heavier font than the title menu), a cream bordered box panel, and a focus-arrow sprite beside the selected row

Three retail-side rendering facts about the case-0x24 config menu, read off the
captured golden (`tests/scenarios/new-game-through/goldens/`) while validating
the menu render.

**The config menu uses a different font from the title menu.**  The golden's
`TextOutA` stream selects LOGFONTA `{face "Courier New", h 18, w 7}` — the
`{w=7, h=0x12, family=2}` entry of the 8 fonts `ar_register_fonts` builds
(`FUN_00579f40`).  The title menu uses the smaller `{w=7, h=0x10, family=0}`
default-family font.  So the config scene picks the heavier Courier New 7×18 for
its rows.

**The menu sits on a cream bordered box panel.**  The text is drawn over a
solid **cream RGB(239,227,214)** bordered sub-rect (the box widget
`0x411940`→`0x40f3e0`) with **gold corner art**, not over the bare scene — it is
a panel over the prior screen, NOT a full-screen wash.  The box bg is what the
glyph anti-aliasing blends toward (so the same glyphs read softer here than on a
dark bg).

**Retail draws a focus/selection arrow sprite beside the selected row.**  To the
**left** of the focused row's text (the menu text proper starts at x=72), at
**x≈60–64**, retail paints a small arrow sprite in **dark-gold RGB(107,93,49)**
+ **tan RGB(206,186,173)** marking the current selection — a separate sprite
from the text, moving with the cursor.

## 67. The new-game config box is a 9-slice sprite panel (bank res 0x457, 32×32 cells, frames tl0/top1/tr2/l3/c4/r5/bl6/b7/br8) plus a separate animated sparkle corner (bank res 0x3e8, frames 16–19)

The bordered cream panel behind the config menu (and the tooltip) is **not** a
procedural fill — it is a **9-slice sprite box** rendered by `FUN_0048cf80` (the
tiled variant; `FUN_0048cb90` is the fade-scaled twin).  Read live off the
actively-rendering bank (`--box-probe`), the panel's bank is **PE resource
`0x457`** (sotesd.dll) and its nine slice frames sit at box-node `+0x60..+0x72`
in order **tl=0, top=1, tr=2, lmid=3, center=4, rmid=5, bl=6, bottom=7, br=8**,
each a **32×32** cell.  Frame 4 (center) is the **cream RGB(239,227,214)** fill,
1/3/5/7 the bevel edges, 0/2/6/8 the ornate gold corners.  Edges + center are
**tiled** (repeat at 32px), not stretched; the panel fades in through the node
`+0x54` ramp fed as the blit alpha.  `FUN_00411940` builds two such boxes for
case-0x24: the **menu** box at (32,32) size **400×124** and the **tooltip** box
at (32,392) size **576×80**.

On top of the static panel sits a **separate, animated** decoration: a single
type-1 cell node rendered by `FUN_0048d940`, **base frame 16** cycling its
frame-list `[0,1,2,3]` → sprite frames **16–19**, positioned at the box's
**top-left** corner (≈dst (44,29), ~22×41) — the **selection cursor** (a drooping
gold feather/quill + soft white shadow), not a twinkle.  **NB: the bank is
`0x455`, NOT `0x3e8`** — the `--box-probe`'s `slot+0x40` read of **`0x3e8`** for
this node is a reused/garbage marker (see **#68** for the proof + the bottom-up
decode finding).

Both bank fields are scene-object members (`*(this+0xb88)` = the 9-slice bank,
`*(this+0xb8c)` = the cursor bank); neither is written as a literal offset
anywhere in the decompiled corpus (an embedded sub-object ctor sets them via a
pool/clone path).  `slot+0x40` is the reliable resource id for the **9-slice**
bank (0x457) but NOT for the cursor bank — for the cursor the reliable signal is
the per-frame trim size read via `entries[frameSel]→frec+0x14/+0x18` (#68).

## 68. The new-game selection cursor is bank res 0x455 frames 16–19, and Lizsoft sprite atlases are decoded BOTTOM-UP (the `0x3e8` probe readout is a reused marker)

The new-game config menu's **selection cursor** — the drooping gold feather/quill
+ soft white shadow that hangs from the box's top-left corner toward the focused
row (`FUN_0048d940` type-1 arm, `*(scene+0xb8c)`) — is **PE resource `0x455`**
(sotesd.dll), the **sibling** of the 9-slice box bank `0x457` (#67), **frames
16–19** (`base 16` + frame-list `[0,1,2,3]`).  Both `0x455` and `0x457` are
registered together by `ar_register_fonts` (`FUN_00579bd0`): `0x457` as a 32×32
type-2 slot, `0x455` as a **32×48 type-2 slot with `scale_flag=1`**.

**Two traps make this hard to see:**

1. **`slot+0x40` lies for this node.**  The `--box-probe` reads the cursor bank's
   `res_id` (slot+0x40) as **`0x3e8`** — but PE resource `0x3e8` is an 80×352
   *portrait* in sotesd, a *WMV* in sotesw, and *absent* in sotesp; it is never a
   22×41 vine.  `slot+0x40` is a reused/garbage marker for this node type.  The
   **reliable** per-frame signal is the trimmed frame size, read via the
   `entries[frameSel] → frec+0x14/+0x18` (w/h) + `+0xc/+0x10` (offx/offy) chain —
   which the probe *also* records, and which reads a clean, stable **22×41**.

2. **The pixel data is BOTTOM-UP.**  The Lizsoft DATA blob carries a BMP-style
   preamble and its 8bpp pixel area is a **bottom-up** bitmap; the engine's slicer
   walks cells bottom-up.  The `0x455` atlas is 128×288 = a 4-col × 6-row grid of
   32×48 cells (24 frames).  Read **top-down**, frames 16–19 land on the row-4 ►
   chevrons (9×17) — which look nothing like a vine, producing a false "0x455
   matches nothing".  Read **bottom-up**, frames 16–19 are the feather, and their
   trimmed bboxes match the live probe **exactly**: frame 17 = 22×41 @ (4,3),
   18 = 22×40 @ (4,4), 19 = 22×41 @ (4,3).  (Tool:
   `tools/extract/cursor_frame_match.py`.)

The transparent background is palette **index 0** (RGB ≈ (0,0,3)); magenta is
index 253.  At blit time the near-black background is colour-keyed (slot
`colorkey=0`), the same mechanism the box bank 0x457 uses.

**Port status (NOT a retail quirk — recorded here only to close the trail):**
the bank/slot/frames in `src/newgame_cursor.c` were correct all along.  Enabling
the render first exposed a *separate* bug — initially mis-diagnosed as a
"`scale_flag=1` videomem cell-build path" fault, but actually the transposed trim
scan of **#69**.  With that fixed (ckpt 43) the cursor renders **bit-exact**
(menu-box `differ_px=0` at the matching animation phase); `g_newgame_cursor_enable`
is now ON.

## 69. `bs_trim_opaque_rect` (`FUN_005b6f80`) takes (cell_w, cell_h) — its arg4 is the COLUMN/x loop, arg5 the ROW/y loop; a non-square cell scanned with them swapped comes out transposed

The per-cell opaque-bbox scanner `FUN_005b6f80` is called by the slicer
`FUN_004188b0` as `FUN_005b6f80(key, base_x, base_y, cell_w, cell_h, out)`.
Inside, **arg4 (`cell_w`) is the inner/column loop bound and the x-axis range**,
and **arg5 (`cell_h`) is the outer/row loop bound and the y-axis range** (verify:
`iVar5 = param_4` seeds the inner loop; the outer loop runs `while (counter <
param_5)`).  So the two trailing args are NOT interchangeable.

This is invisible on a **square** cell (the 32×32 box bank `0x457`, #67) — rows
and columns iterate the same count — which is why every square-cell sprite
(title art, box chrome) rendered bit-exact while the bug lay dormant.  It only
bites a **non-square** cell: the 32×48 cursor bank `0x455` (#68).  Scanned
transposed (32 rows × 48 cols instead of 48×32), the scan reads past the 32-wide
cell into the neighbouring column, yields a wrong bbox/offset, and the built cell
surface is the wrong size at the wrong placement — live, the cursor blitted as an
opaque-black ~16×24 rect at x≈72 instead of the gold feather at x44.

**The port bug:** `src/bitmap_session.c`'s `bs_trim_opaque_rect` named its two
size params `(height, width)` (and the body correctly uses `height` for the row
loop, `width` for the column loop) — but the caller passes `(cell_w, cell_h)`, so
`height` bound to `cell_w` and the cell was iterated transposed.  Fix: rename the
params to `(width, height)` to match the caller's order; the body is unchanged.
Verified offline against the real `0x455` blob via
`tools/extract/cursor_trim_probe.c` (frame 17 → 22×41 @ (4,3), matching the live
`--box-probe`) and live (menu-box `differ_px=0`).  Regression test:
`tests/test_bitmap_session.c::test_trim_8bpp_nonsquare_quirk69` (a deliberately
non-square 4×8 cell).

## 70. The tooltip/help text is a standalone WORD-WRAPPING text node (FUN_0040e360 → parse/justify/commit), distinct from the menu grid: one free-form string greedily wrapped into rows at a configured width, with `%n`/`%m`/`%w` escapes

The new-game / options scenes draw their bottom-of-screen help line through a
**separate text-node widget** (`this+0x170`), NOT the menu grid (`+0x174/+0x178/
+0x17c`).  A menu grid is a fixed rows×cols cell layout (one glyph buffer per
cell); a text node takes ONE string and word-wraps it.  Each frame
`FUN_00564780` recomputes the focused row's help string and calls
**`FUN_0040e360(text)`** on the node, which runs three steps over the node's twin
record arrays — a `0xc`-byte **position record** `{u16 col@+0, s16 row@+2, u32
color@+4, u8 linebreak@+8, u8 wraptype@+9}` per glyph, parallel to a `0x24`-byte
glyph-bytes record:

1. **substitution** (gated on `node+0x164 != 0`, a key→value table; **empty for
   the English tooltips** → skipped, text passes through);
2. **`FUN_0040f040` parse** — split the string into glyph records, folding the
   engine escapes: **`%n`** sets the next glyph's `linebreak` flag (a forced row
   break), **`%m<digits>%`** sets a per-glyph colour, **`%w`** a wrap hint; an
   unknown `%X` drops both bytes;
3. **`FUN_0040e5e0` justify** — greedy word-wrap assigning each glyph a
   `(col,row)`.  A **word** is classified by its lead byte (`sVar3`): **1** =
   alpha run `[A-Za-z']+`, **2** = digit run `[0-9.,]+`, **3** = SJIS lead
   (kinsoku rules over the `DAT_008548xx` 2-byte punctuation table), **0** = a
   lone other glyph.  An alpha/digit word also absorbs **one** trailing
   `{space ! , - . ; ?}` / space.  The row width accumulator `uVar13` and the
   wrap test `(word_w + uVar13) > param_1` break to a new row when the next word
   would overflow the configured **width = `param_1`** (the `FUN_0040dee0` ctor
   arg `0x44` = **68 glyph-columns** for these menus); forced `%n` breaks also
   bump the row;
4. **`FUN_004031c0` commit** copies the wrapped bytes into the renderer's
   `0x24`-byte records.

The node's text origin is the box inset: tooltip box `(32,392)` + `FUN_0040dee0`
insets `(40,24)` → first glyph at **(72,416)**, rows step by the font line height
**28**, drawn with the SAME monospace Courier-New-7×18 + 2-copy drop shadow +
colours (text `0x3e537d`, shadow `0xa8b9cc`) as the menu grid.  Verified bit-exact
against the captured golden: the difficulty-row tooltip wraps **65 / 52** glyphs
across y=416/444 — the break is the width-68 word-wrap (the source string has no
`%n`), reproduced exactly by the port (`src/glyph_wrap.c`; `differ_px`: **0
text-colored pixels** in the tooltip region differ).  The SJIS kinsoku path
(`sVar3==3`) is deferred in the port — English never reaches it.

## 71. The option PICKER submenu (FUN_00567ba0) is a nested modal value-grid; its build args are decompiler-lost and reconstructed, and a live golden is unreachable (the flip counter freezes in 0x565d10's modal pump)

Confirming (`0xc`) on a **kind-0 option row** of the new-game config menu (Game
Difficulty / Auto-guard) opens that option's **picker submenu** — `FUN_00567ba0`,
a nested 1-column grid of the option's value choices with its own `0x565d10` nav
loop.  It is a **blocking modal call** from the parent's run loop (564780.c:612):
the parent frame loop is suspended until the picker returns `0` (cancel) or `0xc`
(a value was committed → the parent sets `local_42c=1` → re-lays every option's
value cell, 564780.c:367-385).

**The picker's grid (567ba0.c:29-45, default arm):**
- `uVar2 = FUN_00568320(id, vals)` — the value-code list + count.  id 3
  (difficulty): `{10,20,30,40}` (4) or `{..,50}` (5) gated on the unlock flag
  `*(*DAT_008a6e80 + 0xaa4)`; id 4 (auto-guard): `{0,1}`.  `uVar2==0` → `return 0`.
- `FUN_00411940(this, 0x120,0x80,0x100, 0, 2)` — a value grid box at **(288,128)**
  width **256** (cf. the menu box's `(32,32)` w=400).
- per value: `FUN_00566a80(id,val)` (the value label) → `FUN_00412160(...)` (append).
- `FUN_00419900(0, current_value)` — **seek the cursor to the current value's row**
  (the picker opens on the active selection, not row 0).
- nav loop: `0xb` → `return 0` (cancel); `0xc` → `FUN_005657f0(...)` (write
  `settings[id]=selected_value`), `return 0xc`; `0xd` → re-iterate.

**Decompiler-lost args (reconstructed, documented in `src/newgame_picker.h`):**
Ghidra dropped the register/stack args of the picker's `__thiscall` calls
(`FUN_00412160` row kind, `FUN_00419900` seek, `FUN_005657f0` commit).  They are
rebuilt from the callees' own contracts: `FUN_00419900(field0,action)` seeks the
row whose `(kind,id)` match, so the seek is `(0, current_value)` over kind-0 value
rows; `FUN_005657f0(id,value)` writes `*(settings + 0xc + id*0x1c) = value`, so the
commit is `(option_id, selected_value)`.

**Verification limit (an OPEN gate):** entering the new-game scene, retail's Flip
counter **freezes** (the modal pump `0x565d10` doesn't advance the hooked DDraw
Present — see quirk #67 box-probe caveat), and BOTH the harness's frame capture
and its input injection are keyed on the Flip index.  So the open picker is
**unreachable** by the current harness: it can neither be driven to (inject confirm
inside the frozen-flip loop) nor captured by flip index.  Therefore the picker's
**render geometry (288,128 / 256) and the reconstructed args are NOT pixel-verified
against retail** — closing that gate needs a harness that drives/captures inside
the modal pump (hook `0x565d10`'s own present + feed its input directly).

The port models the picker as `src/newgame_picker.{c,h}` (pure: value list + build
+ seek + nav/commit/cancel, host-tested) wired into `newgame_drive` as a frame-
stepped modal SUBMODE (the equivalent of the blocking call): on a kind-0 confirm
the drive opens the picker and pumps input into it instead of the parent; on COMMIT
it calls `newgame_scene_set_option(id, chosen)` (the parent's value-refill).
Rendered port-side at (288,128) over the menu; **user-confirmed visually correct**
(open → nav → commit re-lays the parent's value cell, 1:Easy→2:Normal).

## 72. "Start Game" does NOT enter the cutscene on the next flip: FUN_00564160 runs a ≤20-frame fade-out of the new-game scene first (the box node's mode-1 closing alpha ramp)

When the new-game config menu commits **Start Game**, `FUN_00564780(0x24)` returns
0 and its caller `FUN_00564160` does NOT jump straight to the prologue cutscene
`0x56cd20`.  It first **clears the menu box node's `+0x50`** (564160.c:30,
`*(box+0x50)=0`) and runs a **fade-out loop** (564160.c:36-54):

```
FUN_00564690();                       // init the pace machine (step 0x10 ms)
while (true) {
    do { while (FUN_0055f550()==1) present; } while (slice != 2);  // 1 present/slice (lockstep)
    if (FUN_005642e0()==6 || ++n > 0x13) break;   // ≤ 0x13+1 = 20 iterations
    FUN_0056c930();                   // mode-1 CLOSING alpha ramp (field_50==0 ⇒ field_54 -= 0x28)
    for each child: FUN_0043c2e0();   // per-entry animation
}
0x56cd20(...);                        // THEN the cutscene
```

So between the Start commit and `prologue_enter` retail spends **exactly ~20
presented flips** (measured under `--lockstep`: confirm@795 → prologue_enter@815).
`FUN_005642e0` returns `6` (early break) only if it finds an **abort event** (id
`0x22`) within the fade — the Start path never aborts, so the full 20 frames run.
With `0x56c930`'s mode-1 close stepping `field_54` down by `0x28` (40) per frame,
20 frames take the box alpha 1000→200 (not to 0); the screen finishes black because
the cutscene `0x56cd20` opens on a black background, not because the box fully fades.

**TAS consequence (was open thread #1 in tas-harness.md):** a port that enters the
cutscene 1 flip after the commit reaches `prologue_enter` ~14-20 flips earlier than
retail, and the cutscene **fade-in** does not align at a single anchor offset
(residuals 246–662 px = one fade step at ckpt 47).  Porting the loop's TIMING +
alpha-ramp state (newgame_drive's fade-out submode, NEWGAME_FADEOUT_FRAMES=20)
makes the port spend the same ~20 flips, and the cutscene fade-in then diffs
`differ_px=0` at a constant offset (verified: 63/64 dense gem-rise frames bit-exact,
the only residual being the pre-existing tick-0 entry frame).  The fade-out frames
themselves are NOT yet bit-exact in the port — the per-frame fade *render* (the box-
panel alpha blit, `0x48cf80`'s alpha arm via `0x5bd550`, + the GDI menu-text fade)
is the deferred box-alpha arm; the port re-renders the menu opaque during the fade.
That render is a separate open item; this quirk closed the TIMING gap.

## 73. The in-game camera IS the view object — `*(room_state+0x104c)`, a 0x78-byte struct with a fixed 640×480 viewport (`+0x64=64000`, `+0x68=48000`); the opening-town intro holds an establishing shot ~83 flips, then scripted-pans left

The render camera and the "view object" `FUN_00490f30`/`FUN_0048eac0` project
through are the **same 0x78-byte struct**: `view = *(room_state + 0x104c)`
(`room_state = DAT_008a9b50`), allocated as `operator_new(0x78)` by the
room-state ctor `FUN_004017d0:187` (byte 0x104c = dword index 0x413).  Its
room-entry init is `FUN_00586010:854-872`:

```
view[0]/[1] = dim0*0xc80 / dim1*0xc80    (map pixel extent)
view+0x5c = view+0x60 = view+0x74 = 0    (scroll origin / shear, zeroed)
view+0x64 = 64000   view+0x68 = 48000    (viewport = 640*100 / 480*100, FIXED)
FUN_00587d30(view+9) / (view+0xf)        (zero the +0x24 / +0x3c sub-blocks,
                                          which hold +0x34 / +0x4c)
```

So the viewport is a hard 640×480 (in the engine's ×100 fixed point) and the
origin starts at (0,0).  **Live ground truth (`--seed-pin --lockstep`, two runs):**
on the opening town (map 0x3f2, room 210110) the engine then **snaps** the origin
to the entry spawn — `view+0x60 = 128000` (40 cells), `view+0x5c = 12800`
(4 cells) — **by flip 1093, and HOLDS it stable ~83 flips through ~1176** (the
town first renders ~flip 1150, inside this hold).  After ~1176 it runs a
**scripted leftward pan** (`view+0x60` decreasing, easing up to ≈ −300/flip),
while `view+0x5c` (y) stays fixed at 12800 and `+0x34`/`+0x4c`/`+0x74` stay 0.

**TAS/parity consequence:** the opening town's *first* rendered frame uses a
determinate camera (window cols 39-60 / rows 3-18 of the 88×19 grid), so the
static backdrop is comparable without porting the dynamic scroll.  The pan-onset
flip jitters a few flips run-to-run (the R3 render-pace phase pillar) — anchor on
`game_enter`, do not compare the pan by raw flip index.  (Probe method: the
field-spec `src:"chain"` global-deref `*(*(0x8a9b50)+0x104c)+off`, the `cam_*`
fields in `tools/flow/retail_fields.json`; writeup `in-game-intro.md` "The
camera/view object".)

## 74. The opening-town establishing shot is LETTERBOXED — solid-black bars over the top 64 rows (0-63) and bottom 64 rows (416-479), leaving a 640×352 cinematic window

During the intro establishing shot (the scripted leftward pan, map 0x3f2 / room
210110), retail draws **opaque black bars** across the full width: **rows 0-63
at the top and rows 416-479 at the bottom** (`(0,0,0)`, full 640px wide,
verified across pan flips 1617-1780 under `--seed-pin --lockstep`).  The visible
scene is the central **640×352** band (rows 64-415).  The bars are **stable**
(same extent every frame) through the pan — a fixed cinematic frame, not an
animating iris.  This is the "dark top" the user noticed in the establishing
frame (there is a matching *bottom* bar too); it is **scene-scoped** — absent
from settled gameplay — so it is a per-scene CINEMATIC overlay tied to the intro.

**The producer (proven, ckpt 75 — NOT the `0x5a00c0` overlay as first guessed):**
the per-frame world driver `FUN_0048c150:124-162`, immediately after the backdrop
present pass (`0x48eac0`).  Two grid-fill loops tile a single 64×4 opaque cel
(main sprite-pool slot 41 = PE resource **`0x583`**, registered by
`ar_register_main_sprites`) across the full screen width — the BOTTOM bar first
(`in_ECX+0x44` rows, ret `0x48c48a`, dy 416-476) then the TOP bar (`in_ECX+0x48`
rows, ret `0x48c4fe`, dy 0-60).  Each bar's height is rounded up to a multiple of
4 (the cel height) and tiled at 64px column pitch (10 columns, dx 0-576); the
inner column loop runs while `(dx+0x80) < 0x281`.  Both bar-height fields are 64
for the opening town.  They live on the scene-controller object (`in_ECX`) and are
written by the (unported) `0x5a00c0` cutscene script — the same source as the
camera-pan trigger.  Verified bit-exact against the retail blit trace
(`/tmp/blit_town_retail`, flip 1500: 160 bottom-bar + 160 top-bar `blt_onto`
calls of res `0x583`).

**TAS/parity consequence:** a flip-anchored full-frame diff of the establishing
shot will show the top/bottom 64px as a solid difference until the letterbox is
ported (the backdrop *inside* the window is otherwise pixel-1:1 — verified ckpt
70b by matching `cam_x60` and diffing: the buildings/walls/sky/mountains are
Δ0 except the banner/NPCs/foreground-tree layers).  Probe: capture retail with
`--capture-frames` under `--no-turbo --seed-pin --lockstep`, scan each row for
`(0,0,0)` across the width.  Writeup `in-game-intro.md` "The pan CADENCE +
TRIGGER measured" → the diff verification.

## 75. The in-game frame is a WALL-CLOCK GetTickCount limiter that presents a VARIABLE number of Flips per sim tick — so the Flip index is NON-DETERMINISTIC; the sim-tick index is the only deterministic frame of reference

The in-game scene driver `FUN_00439690` runs ONE logical sim tick per outer
iteration: it renders + **Flips repeatedly inside a GetTickCount-gated frame
limiter** (`439690:776-859`, a 3-state machine with a 16 ms quantum
`local_270=0x10`, re-entered each tick via the `switchD_0043b615_caseD_5`
label so the state persists), then steps the camera easer `FUN_0043d1d0`
**exactly once** (`:1123`).  The limiter presents the *same* frame (camera is
constant within a tick) 1-or-more times until enough wall-clock time has
accrued, then the sim advances.  So **the present/Flip rate is decoupled from
the sim rate** (≈2 Flips per sim tick on average, ~31 Hz sim / ~61 Hz present).

**Measured (2026-06-06, `--seed-pin --lockstep --no-turbo`, easer + Flip
`0x5b8fc0` hooked, contiguous flip whitelist 1600-1720):** two **identical**
retail runs DIVERGE when indexed by Flip — first cam step at flip 1619 vs 1616;
plateau (a 4-Flip-long sim-tick) count and location differ (1 plateau at
1699-1703 in one run vs 4 plateaus at 1616/1674/1675/1702 in the other);
**57 of 121 common flips disagree** on `cam_x60` (up to 3 px).  `--lockstep
-epsilon-ms 0` is WORSE (95/111 disagree), so the non-determinism is intrinsic
to the wall-clock limiter, not just the lockstep epsilon creep.  **Indexed by
sim tick (the Nth easer `0x43d1d0` call) the two runs are BIT-IDENTICAL** — the
camera sequence 128000, 127990, 127970, … (vel ramp −10,−20,…,−300) is a pure
deterministic function of the tick number.

**TAS/parity consequence (the methodology pillar):** never anchor a port↔retail
(or even retail↔retail) diff on the **Flip index** — it wanders run-to-run by
several frames / px.  Anchor on the **sim tick** (easer-call index, equivalently
`cam_x60` during the monotonic pan).  A "few-px offset on the whole foreground
while the background is Δ0" is the signature of Flip-misalignment, NOT a
placement bug (the 0.5×/0.25× parallax hides the same camera error that the 1×
foreground exposes).  When a residual wanders and no anchor fixes it,
investigate the timestep determinism FIRST.  Writeup `in-game-intro.md`
"Timestep determinism".

### #75 addendum — the non-determinism is NOT harness-tunable; per-subsystem anchoring is required (2026-06-06)

Follow-up experiments narrowing the cause:
- **GetTickCount is the engine's ONLY clock** — no `timeGetTime` /
  `QueryPerformanceCounter` / `rdtsc` anywhere in the decompile.  So the limiter's
  variability is not an un-virtualized time source; lockstep's virtual GetTickCount
  (16 ms/Flip, main-thread) is the clock it reads.
- **No lockstep setting removes the jitter.**  `--lockstep-epsilon-ms 0` →
  95/111 flips disagree (worse).  `--lockstep-step-ms 100` → still ~2 flips/tick
  with a 1..5 jitter and two runs' flips-per-tick distributions differ
  ({5:1,2:61,3:1,1:1} vs {4:2,1:2,5:1,2:58}).  So flip-level determinism is not
  reachable by config — the per-tick Flip count is set by the limiter's
  interaction with the OS message pump (`FUN_005b1030`), not the clock alone.
- **`--turbo` is unusable in-game:** plain turbo freezes the sim (GetTickCount
  doesn't advance → the pacer budget never refills, the FSM + pump stall — the
  classic "splash doesn't animate under turbo", quirk #29).  `--turbo --lockstep`
  is not faster (turbo_ticks=0) and the (no-turbo-timed) input trace misses, so
  it never reaches in-game.  Determinism must come from the SIM-TICK anchor, not
  from a faster clock.
- **Each sim SUBSYSTEM has its own clock and must be anchored separately.** The
  camera easer is sim-tick-deterministic (game_enter-anchored).  The **NPC/actor
  animation is on a DIFFERENT clock** — matched two runs by the deterministic
  sim-tick (54 ticks, camera Δ0) and the actor band still diffs ~6.7k px.  So the
  actor anim reads a counter (global frame tick or a spawn-time RNG phase) that is
  NOT the camera sim-tick and is non-deterministic run-to-run.  Finding + pinning
  it (à la the RNG seed pin) is the next determinism chip — it lives in the
  unported actor/entity system (`0x491ae0` reads the frame; the advance is in the
  per-tick actor update).  **CONFIRMED per-tick, not per-Flip:** within any single
  sim tick (its 2-3 duplicate Flips) the actor-band pixels are IDENTICAL
  (intra-tick diff 0) — so the actor anim is a per-tick clock and is **pinnable**
  at an anchor (camera/RNG style), NOT tangled in the non-deterministic flip rate.
  Plan: RE the actor anim cycle, then pin its counter for determinism.

Practical rule until then: **diff one frame per sim tick** (dedup the multi-Flip
presents), match camera/backdrop on the sim tick, and treat the actor layer as a
separate not-yet-pinned subsystem.

### #76 — the actor animation frame is a pure per-sim-tick STEPPER (rides the camera clock; no separate counter) (2026-06-06)

RE'ing the actor anim cycle (the #75-addendum "next chip") **refines** that
addendum's guess.  The animation frame does NOT read "a global frame tick or a
spawn-time RNG phase that is not the camera sim-tick" — it is advanced by a small
inline stepper that reads ONLY the clip's own duration/count and the per-actor
timer.

**The call chain (per SIM-TICK, two-witness from the decompile):**
- In-game loop `FUN_00439690:1108` calls `FUN_0046cd70(1)` once per tick (when
  `*(param+0x1c)==0`).  This is the actor-UPDATE master driver — distinct from the
  render/emit pass `FUN_0048c150`.
- `0x46cd70` walks the actor pools off `DAT_008a9b50` (active = `actor+0x1d0!=0`);
  for the main band (`+0x11e0`, 128 slots) it calls `FUN_0054f980(actor+0x40,
  actor+0x40, 0, 0)` for the primary render-state entry (and `(entry-0x294, entry,
  1, idx)` for each kinematic sub-entry).
- `0x54f980` (the per-actor behaviour dispatch on `actor+0x1d4`) runs, in EVERY
  animating case, the byte-identical inline frame-stepper on the render-state's
  anim fields:

  ```
  seq = rstate[+0x6c];                              // current clip ptr
  if (seq && (++rstate[+0x70] >= seq[+0x44])) {     // timer++ >= duration
      rstate[+0x72]++;  rstate[+0x70] = 0;          // frame++, reset timer
      if (rstate[+0x72] >= seq[+0x42]) {            // past the last frame
          if (seq[+0x48] == 0) rstate[+0x72] = seq[+0x152];        // loop -> loop_to
          else { rstate[+0x70]=1; rstate[+0x74]=1; rstate[+0x72]=seq[+0x42]-1; } // one-shot hold
      }
  }
  ```

  The clip descriptor (`seq`) is a fixed 0x154-byte 32-frame clip: count@`+0x42`,
  duration@`+0x44`, one-shot@`+0x48`, loop-to@`+0x152`, base sprite@`+0x00`,
  per-frame sprite-delta@`+0x02`, per-frame x/y offset@`+0x50`/`+0xd0` (the last
  three confirmed by the renderer `FUN_00491ae0` case 0x1872d).  The clip is
  (re)assigned only on a STATE CHANGE (`FUN_0040afe0`/`FUN_0041e600`), which resets
  timer/frame/done — and only when the clip pointer actually changes, so
  re-asserting the same state lets the cycle keep running.

**Consequence — the frame counter needs NO separate pin.**  `rstate[+0x70]/[+0x72]`
is a pure function of *(sim-ticks since the clip was set)* — it reads no
GetTickCount, no Flip index, no RNG.  So it is already deterministic under the
**same `g_sim_tick` anchor** the camera uses (game_enter scene-load reset).  This
is the per-subsystem anchoring the #75 decision called for: the actor-anim
subsystem anchors on the existing sim-tick clock; there is no new counter to find.
Ported (host-tested bit-exact): `src/anim_clip.c` `anim_clip_advance` +
`anim_state_set`.

**So what was the #75 ~6.7k-px actor-band residual?**  Since the frame-stepper is
sim-tick-deterministic, a divergence between two seed-pinned, sim-tick-matched
runs cannot be the frame *within* a clip — it must be a DIFFERENT pillar: the
RNG-driven BEHAVIOUR (which clip is playing / the actor's position).  `0x54f980`'s
idle/wander cases draw the LCG `FUN_005bf505` for random wait timers and spawn
offsets (e.g. case 0x11365/0x1872d pick a random `+0x5c` wait before the next
state), and the clip-SET timing is downstream of those draws.  If the actor RNG
consumption desyncs run-to-run, the actor STATE (not the intra-clip frame) drifts.
**To confirm:** live-capture `a0_clip`/`a0_frame` (now annotated in
`retail_fields.json` for `0x54f980`) across two seed-pinned sim-tick-matched runs —
expect `a0_frame` to match at equal sim-tick while `a0_clip`/position diverges,
pinning the residual to the RNG/behaviour pillar (anchored separately, à la the
RNG seed pin).  **CONFIRMED — see #77** (the live experiment; the mechanism turned
out to be deeper than "actor RNG desyncs": the *shared* LCG stream phase is itself
non-deterministic run-to-run despite the seed pin).

### #77 — the shared LCG stream PHASE is non-deterministic run-to-run even under `--seed-pin` (a per-flip consumer × the non-deterministic flip rate desyncs `DAT_008a4f94`) → RNG-driven subsystems need a per-subsystem RNG anchor, not just the sim-tick (2026-06-06, ckpt 73)

The live experiment the #76 "To confirm" called for.  Drove retail **twice**,
`--seed-pin --lockstep --no-turbo`, the same in-game input trace, hooked the
per-sim-tick actor-update boundary `FUN_0046cd70` and snapshotted the LCG state
word **`DAT_008a4f94`** there (field `rng`, tagged with the deterministic
`g_sim_tick`, reset at `game_enter`).  8644 in-game sim-ticks common to both runs.

**Result: `rng` matches on 0 of 8643 sim-ticks** — the shared LCG stream is
at a *different* state at every single in-game tick, even though the seed was
pinned to the same value at boot and the sim-tick index is the deterministic
frame-of-reference (quirk #75).  (The actor anim fields `a0_clip`/`a0_frame` did
match 8643/8643, but **trivially** — actor slot 0 of the main band `+0x11e0` was
inert the whole run, `clip=0 frame=0`; that only shows an inert actor stays
identically inert, not that the stepper is deterministic for an animating actor.
The `rng` divergence is the real signal.)

**The mechanism — proven at the scene anchors, not just inferred.**  The TAS
anchors carry the LCG state at entry:
- `newgame_enter`: run A flip **751** rng `0x6a239b8d` · run B flip **750** rng `0x6a239c54`
- `prologue_enter`: run A flip **946** rng `0x84654e6f` · run B flip **946** rng `0xa79a2d6e`
- `game_enter`: run A flip **1432** rng `0x84654e6f` · run B flip **1434** rng `0xa79a2d6e`

The decisive pair is **`prologue_enter`: both runs are on the IDENTICAL flip 946,
yet the LCG state differs** (`0x84654e6f` vs `0xa79a2d6e`).  So the desync is not
merely "the two runs reached the anchor at different flip counts" — at the *same*
flip the engine has drawn a *different number* of RNG values.  Some consumer draws
the LCG at a rate that is not locked to the flip or the sim-tick (the title/menu
sparkle and per-frame effects draw per *present*; the present/Flip count per sim
tick is itself non-deterministic, quirk #75).  Once the stream phase diverges it
never re-converges, so every downstream RNG-driven subsystem inherits a
non-deterministic stream.

**Consequence (closes the #75-addendum / #76 OPEN, refines the fix).**  The actor
BEHAVIOUR dispatch `FUN_0054f980` draws this exact LCG (`FUN_005bf505`) dozens of
times per tick for idle-wait timers, the idle→wander branch pick, and wander
move-offsets (static two-witness: ~40 call sites; e.g. behaviour `0x11365` draws a
`+0x5c` wait `≈ rand·300/0x8000`, a branch selector, then a move offset `≈
rand·1000/0x8000 − 500` into `FUN_00450ef0`).  Feeding a divergent stream into
those draws makes the actors choose different waits / directions / positions
run-to-run → that **is** the #75-addendum ~6.7k-px actor-band residual.  It is the
RNG pillar (parity-model pillar 3), and crucially it is **NOT closable by the
camera's `g_sim_tick` anchor** (which works only because the camera reads no RNG).
A subsystem that reads RNG needs its own **RNG anchor**: snapshot/restore
`DAT_008a4f94` at the `game_enter` sim-tick (or re-seed the actor RNG per tick) on
*both* sides so the per-tick actor draws are reproducible — the per-subsystem
anchoring the #75 decision called for, now shown to be mandatory (not optional) for
the actor layer.  For port↔retail parity the bar is therefore "data-1:1 given a
matched RNG state," compared under a pinned actor-RNG anchor — retail-vs-retail
itself is not observed-1:1 in this band.

Reproduce: `tools/run-retail.sh --no-turbo --hide-window --seed-pin --lockstep
--input-trace tests/scenarios/in-game-intro/trace-retail.jsonl --call-trace
--field-spec-only` ×2 into two run dirs, then `tools/rng_tick_diff.py runA runB`
(the `rng` field is on `0x46cd70` in `tools/flow/retail_fields.json`).

### #78 — the opening town's "NPCs" are 32 STATIC scenery actors + 1 animated; the main actor band renders 32/33 via the renderer's DEFAULT arm (2026-06-07, ckpt 76)

> **CORRECTED by #80 (ckpt 79):** the live `+0x48`-sprite-table capture shows
> only **6 of the 33** main-band actors actually DRAW (3 villager codes + the 1
> animated protagonist); the other **27 are invisible** collision/trigger volumes
> (all-zero sprite table → the renderer's `bank==0 => skip`).  "32 static scenery
> actors" overstates it — they are 32 character-band *entities*, mostly invisible.

The in-game world has **six actor-pool bands** off the room-state god-object
`DAT_008a9b50`, each walked by the per-frame render driver `FUN_0048c150`
(free-roam branch) and the per-sim-tick update driver `FUN_0046cd70`; a slot is
live when `actor+0x1d0 != 0`.  The **MAIN band is `+0x11e0` (0x80 = 128 slots)**,
rendered by `FUN_00491ae0` and updated by `FUN_0054f980`.  (The others: `+0x1160`
0x20 → `0x493ba0`; `+0x1060` 0x40 → `0x4937c0`/`0x4710c0`; `+0x13e0` 0x400 →
`0x493480`; `+0x23e0` 0x60 → `0x492fc0`; `+0x2560` 0x80 → `0x493230`.)

Live trace of the town hold (retail flip 1500, `--seed-pin --lockstep`):
**33 active main-band actors.**  **32 are STATIC** (render-state `+0x6c` clip == 0,
frame 0 — fixed scenery/villager sprites); **exactly ONE is animated** (`+0x1d4` =
`0x1872d`, clip set, the protagonist/key NPC).  The behaviour code `+0x1d4`
(0x112e6 ×10, 0x111d6 ×7, 0x1129e, the 0x1136x run, …) is the dispatch key of BOTH
`0x491ae0` (render) and `0x54f980` (update/AI) — but **for 32/33 codes it is NOT an
explicit case in `0x491ae0`'s switch**, so they fall through to the **default arm**
(`caseD_11257` → `FUN_0044d160` → emit one node).  Net: the behaviour code selects
the actor's *AI/motion* (in `0x54f980`), while **rendering is code-agnostic for
static actors** — one function (`FUN_0044d160`) draws nearly the whole town
population.  Each actor's appearance is its **per-direction sprite table** at
`actor+0x48` (stride 0x14: bank/frame_base/x_off/y_off), indexed by `actor+0xe8`
(dir); the node is emitted (`FUN_00492670`) into the `view+0x54` draw_pool as
**mode 0 (keyed) / 1 (alpha)** — the present-pass actor modes.  Render output banks:
res `0x403`/`0x426` (villagers) + `0x459`/`0x462`/`0x46a`/`0x47b`/`0x481`/… (the
ckpt-75 render_diff residual's named NPC banks).

The behaviour codes are **never assigned as literals** in the decompile because
they are not constants — they are read from the room's map resource (see #79).
(The 8 PARTY actors are a separate set: `FUN_00560e60`-reset at `0x59f578` inside
`0x59f2c0`, stored at `map+0x4030`.)  Consequence for the port: the render side
(`FUN_0044d160` + `0x492670` + present modes 0/1) is small + tractable and renders
the whole static town; the spawn (band population) is RE'd in #79.
Tool: the `thischain` field source + the `0x491ae0` annotation in
`tools/flow/retail_fields.json`; `findings/in-game-intro.md` "The town ACTORS".

### #79 — the room object/actor spawn: a type-RANGE dispatch over the map's object layers, into four pre-allocated bands (2026-06-07, ckpt 78)

> **CORRECTION (ckpt 79, see #80):** the closing paragraph's claim that
> `FUN_00426620` "installs the sprite from a per-type def table `type*0x80 +
> 0x21c04`" is **wrong** — that formula is **cell-indexed** (`*(grid+0x1048)` at
> `(cell_x*0x80 + 0x21c04 + cell_y)*0xc`) and writes the actor's **collision/region
> flags** (`+0x288/+0x28c`), NOT a sprite.  `FUN_00426620` in fact **ZEROES** the
> `+0x48` sprite table; the table is filled **lazily** by the state machine
> (`FUN_0040afe0`/`FUN_0041e600`) — see #80.  The "appearance keyed by type"
> *intent* stands; the *mechanism* was misattributed.

A room's actors/props/effects are **placed by the map resource, not by code**.  The
room-object pass **`FUN_0058d460`** (called from `FUN_00586010:698`, right after the
map is parsed + the tiles decoded) walks the map's **object-placement layers** — the
`count` `0x3c`-byte layer headers at `mapobj+0x38` (DATA 1022 has **86**).  Each
header IS an object placement: `+0x04` x, `+0x08` y (×100 → world), **`+0x10` the
type code**, `+0x18` a u16 sub-type.  Each object is dispatched **by the integer
RANGE of its type code** into one of four actor-pool bands off `DAT_008a9b50`, each
band a fixed pool pre-allocated empty by `FUN_00586010:476-506`
(`FUN_0058cf60(0x40)`); the pass scans the band for a free slot (`+0x1d0==0`) and
aborts with a named `"<kind> Object Count Over"` debug string if the band is full:

| type-code range | kind | band | spawn fn |
|-----------------|------|------|----------|
| 50000–59999 | EFFECT | `+0x1160` | `FUN_0041f200` |
| 60000–69999 | STRUCTURE | `+0x2560` | `FUN_00438a60` |
| **70000–79999** | **CHARACTER** | **`+0x11e0`** | **`FUN_00431e30`** |
| 80000–89999 | DEVICE | `+0x13e0` | `FUN_00557550` |

The **CHARACTER** band (`+0x11e0`) is the entity band `FUN_00491ae0` renders (#78);
for the opening town it holds static **props** + invisible volumes + the one
animated protagonist — NOT townsperson-NPCs (#80).  The character activator
**`FUN_00431e30`** (`__thiscall`, ECX = the free slot) is a per-type `switch` that
sets `actor+0x1d0=1` (active), **`actor+0x1d4 = type`** (so the placement type code
becomes the behaviour code verbatim), `actor+0xfc=9` (draw layer), `actor+0xe8=0`
(dir), **ZEROES the `+0x48` sprite table**, and stores the world (x,y).  Its
per-type helper `FUN_00426620` fills the **collision/region** flags (`+0x288/+0x28c`
via the cell-indexed `*(grid+0x1048)` lookup at `(cell_x*0x80 + 0x21c04 + cell_y)*0xc`
— this is what the original draft misnamed a "sprite def table").  The **sprite**
table `+0x48` is filled LAZILY later (`FUN_0040afe0`/`FUN_0041e600`, #80) — so an
actor's **appearance is still keyed by its type code**, just not here, and not from
the map record (the layer sub-arrays are ~empty for the town props).  For DATA 1022
the object layers
decode to **15 effects + 39 structures + 32 characters + 0 devices = 86**, and the
32 character codes + multiplicities match the live actor census exactly (proof:
`docs/proofs/map-object-layer-format.md`; `tools/extract/map_data.py … --objects`).

### #80 — the town CHARACTER band is MOSTLY INVISIBLE (only 3 of 13 codes draw); the `+0x48` sprite table is filled LAZILY (not by the spawn), keyed by type (2026-06-07, ckpt 79)

A live `+0x48`-sprite-table capture of every active `+0x11e0` main-band actor at
the town hold (retail flip 1480/1500/1520, `--seed-pin --lockstep --no-turbo`;
hooked `FUN_00491ae0`, `__thiscall` ECX = actor, reading its `+0x48` table per the
field spec) corrects #78 + #79:

- **Only 6 of the 33 active main-band actors actually DRAW.**  The renderer
  (`FUN_0044d160`) reads the per-direction sprite row at `actor + 0x48 + dir*0x14`
  and returns 0 (draws nothing) when its `+0x00` **bank is 0**.  Capture: **27 of
  the 33 actors have an ALL-ZERO `+0x48` table** in every direction — they are
  invisible **collision / trigger / spawn volumes** (the `0x111d6`/`0x112e6`/… codes
  whose `FUN_00431e30` arms set up a physics/kinematic body, not a sprite).  Only
  these draw (all `dir==0`, `clip==0` = static, `skip==0`):

  These are static **PROPS, not people-NPCs** — bank `0x16c` (res `0x403`) is the
  town-OBJECTS sheet (USER-confirmed against retail on the feed: the fountain
  `0x112e5` + a barrel `0x1129e`; they render at the correct positions).  The only
  person in the band is the animated protagonist `0x1872d`.

  | code | n | bank | frame_base | draw layer | note |
  |------|--:|------|-----------:|-----------:|------|
  | `0x1129e` | 3 | `0x16c` | 1  | 9  | prop (a barrel), res `0x403` |
  | `0x1129f` | 1 | `0x16c` | 2  | 9  | prop |
  | `0x112e5` | 1 | `0x16c` | 36 | 10 | prop (the fountain), draw layer 10 not 9 |
  | `0x1872d` | 1 | `0x175` | 0  | 9  | **the arrival WAGON** (NOT a person — corrected by #81; clip `0x671c48`, `+0x2c`=0x63; **outside** the 70000 CHARACTER range — a SEPARATE cutscene spawn; the `0x491ae0` `0x1872d` multi-part arm) |

  So the town's mode-0 keyed-blit residual (#74/#75's 36 blits) is **5 static
  prop blits (bank `0x16c`) + the multi-part protagonist** (bank `0x175` +
  its body-part banks `0x426`/`0x459`/… — the bulk), NOT 32 visible scenery actors.

- **The `+0x48` table is filled LAZILY, not by the spawn.**  `FUN_00431e30`
  (via `FUN_00426620`) **zeroes** `+0x48`; yet at flip 1500 the 6 visible actors
  have a non-zero bank/frame there.  So the table is populated **after** the spawn,
  by the per-state animation set machinery (`FUN_0040afe0`/`FUN_0041e600`) reading
  a **type-keyed entity-def table** (the actual def table, distinct from the
  collision lookup #79 misnamed).  That def table is **not yet RE'd** — captured
  ground truth stands in for it (PORT-DEBT `actor-sprite-table`).

- **The props render at a DETERMINISTIC per-code offset from their map coords —
  NOT RNG.**  (Earlier draft wrongly called this RNG "wander".)  A prop's captured
  render-state pos differs from `map_x*100` by a fixed per-CODE delta — all three
  `0x1129e` share the *exact* same `+1800x/+1600y`, and the positions are identical
  across flips 1480/1500/1520 (static, not moving).  The clearly-visible fountain
  `0x112e5` has delta `+0/+0`, so it lands *exactly* at `map_x*100` (USER-confirmed
  it matches retail).  So the delta is a deterministic placement/anchor nuance (the
  spawn's `0x426620` alignment arm or the lazy fill — TBD), fully reproducible; the
  port spawns at `map_x*100` today (correct for the fountain; the `+1800/+1600`
  props are off-screen/edge at the hold, so their small delta is a refinement).

Field spec: `tools/flow/retail_fields.json` `0x491ae0` (the `row0_bf`/`d1_bf`..
`d7_bf`/`dir_e8`/… `thisderef` fields).  Port: `src/actor_spawn.c` (the spawn +
the visible-code stand-in map).  Writeup: `findings/in-game-intro.md` "The town
actor RENDER CENSUS".

### #81 — the town intro's `0x1872d` is the arrival WAGON (not "the protagonist"), spawned by the cutscene script `0x4d7d80`; the `+0x48` fill primitive is `0x426db0` (2026-06-07, ckpt 80)

The one drawing actor `0x80`-band code that is NOT a static prop, `0x1872d`, is
spawned and rendered by a path entirely separate from the map's CHARACTER objects:

- **Spawn = the per-room cutscene script `FUN_004d7d80`.**  It runs only when the
  area is `0xd2` (210) and `switch`es on the room id (`**(DAT_008a9b50+0x1038)`);
  **`case 0x334be`** (= 210110, the town) is the intro script.  On its first step
  (gated on event flags `0x5f76805`/`0x606aa4f`) it calls
  **`FUN_00431d10(0, 0x1872d, anchor=0x65, x=0x3200, 0, 0)`** — the by-code
  `+0x11e0`-band spawn helper (free-slot scan; with a non-zero anchor it finds the
  active slot whose `+0x274` == the anchor and places the new actor RELATIVE to
  that actor's render-state pos) — and sets the camera hold to 128000
  (`in_ECX[0x11]=0x1f400`, the value the camera RE measured).  `0x431d10` calls
  `FUN_00431e30` (the big activator) with the code; its **case-`0x1872d`** arm sets
  layer 9, runs `FUN_0041ee60` to resolve the world pos, fills `in_ECX[0x11]`
  render-state sub-entries (`+0x2c`=99), installs the clip
  (render-state `+0x6c = &DAT_00671c48`), and installs the sprite via:
- **`FUN_00426db0(dir, bank, frame_base, b, x_off, mirror_x, y_off)`** — the
  per-direction sprite-row writer (the lazy `+0x48` fill #80 flagged as un-RE'd):
  writes `row+0x00=bank`, `+0x02=frame_base`, `+0x04=b`, `+0x08=mirror_x`,
  `+0x0c=x_off`, `+0x10=y_off` at `actor + 0x48 + dir*0x14`.  For `0x1872d` it is
  `FUN_00426db0(0, 0x175, 0, 1, 0, 0, 0)` → **row 0 only: bank `0x175`, frame_base 0**.
- **The asset is a covered WAGON, not a person.**  Bank `0x175` (res `0x3ec` —
  the port's render_id blit trace ground truth, ckpt 81; the ckpt-80 note of
  `0x058f` was unverified and matches no registered bank) decodes to a
  wagon/caravan sheet.  Proven port-side by a with-`0x1872d`
  vs without-`0x1872d` settled-frame diff (cam 12800), which isolates the exact
  `0x1872d` pixels as a wagon composite — and since bank `0x175` is the user's own
  decoded asset (identical both sides), this is ground truth.  The `0x491ae0`
  case-`0x1872d` arm draws it as a **3-cel composite tiled at a 128-px pitch**:
  **wagon-body-left (frame 0 @ x-256) | wagon-body (frame 1 @ x-128) | the HORSES
  (the clip-driven body cel @ x+0)**.  The body's clip is `&DAT_00671c48`
  (decoded: base_sprite 2, frame_count 4, frame_dur 18, looping, frame_delta
  {0,1,2,3}) so the body cycles sprite frames **2..5** = the horses trotting.  So
  #80's "the one PERSON / the protagonist" label for `0x1872d` is **corrected: it
  is the intro arrival CARRIAGE** (USER-confirmed "matches retail").  (Its siblings
  `0x1872e`/`0x1872f`/`0x18730`, spawned by `0x539e80`/`0x5034b0`/`0x431e30`, are
  the rest of the arrival set — likely the characters; not yet rendered.)

Port: `src/actor_render.c` (`actor_render_protagonist`), `src/actor_spawn.c`
(`actor_spawn_protagonist`).  Writeup: `findings/in-game-intro.md` "The 0x1872d
SPAWN + the arrival WAGON".

### #82 — `0x54f980`'s per-actor update splits into an UNCONDITIONAL anim stepper + a GATED behaviour; the wagon's horses always idle-animate, its wander is RNG/cutscene-gated (2026-06-07, ckpt 81)

Reading the case-`0x1872d` arm of the per-actor update `FUN_0054f980` (`:911-970`,
called once per sim-tick by `0x46cd70` for each active `+0x11e0` actor) shows the
two halves are cleanly separable — which is *why* the horses' idle animation can be
ported now while the RNG layer stays deferred:

- **Half 1, the frame-stepper (`:911-928`) runs UNCONDITIONALLY** — gated only on
  `render-state +0x6c` (the clip) being non-zero.  It is the byte-identical
  `timer++ / dur-gate / frame++ / loop-or-hold` idiom of quirk #76 (port:
  `anim_clip_advance`).  No GetTickCount / Flip / RNG → a pure function of
  (sim-ticks since clip-set).  So the wagon's body cel **always cycles** sprite
  2..5 for as long as the actor is active — at the hold, during the pan, at the
  settled camera, regardless of input or RNG.  **USER-confirmed retail behaviour
  (ckpt 81): at the settled position the wagon is PARKED (stationary) and the
  horses just IDLE-animate — a subtle 4-frame loop (ear flicks), not locomotion.**
  So `WAGON_CLIP` is an idle cycle; the body-cel "trot" elsewhere in these notes is
  shorthand for this idle loop, not movement.  (Whether the caravan visibly ROLLS
  IN earlier in the intro is a separate question for the `0x4d7d80` cutscene — not
  this stepper, and not observed at the settled hold.)
- **Half 2, the behaviour (`:929-970`) is GATED then RNG-driven.**  It first
  `break`s out entirely if `param_3 != 0` (a kinematic sub-entry, not the primary)
  **or** `*(DAT_008a9b50+0x27a8) != 0` (a global "scene/cutscene busy" lock).
  Only past that gate does it draw the LCG `FUN_005bf505` for the idle-wait timer
  (`+0x5c`), the idle→wander branch pick, and the move direction — the
  non-deterministic motion deferred by #77 / ckpt 73.

So an actor's APPEARANCE-animation (clip stepper) and its AI/MOTION (behaviour)
are independent subsystems sharing one `0x54f980` call: the former is deterministic
and portable in isolation; the latter is the RNG layer.  The port drives only
half 1 (`actor_pool_update` → `actor_anim_advance`, once per sim-tick on the camera
cadence); LIVE-VERIFIED on the port blit trace — at the settled camera the wagon's
body cel (res `0x3ec`) steps 2→3→4→5→2 every 36 Flips (18 sim-ticks) while the two
fixed wagon cels stay frames 0/1.  Writeup: `findings/in-game-intro.md` "The horses
TROT — the per-tick anim wired".

### #83 — actor-CODE adjacency does NOT imply same-scene; the per-room SCRIPT handlers are area-gated on the live room id (2026-06-07, ckpt 82)

The town intro's scripted wagon is code `0x1872d` (100141).  The code-adjacent
`0x1872e`/`0x1872f`/`0x18730` (100142-100144) look like a "family" and `0x431e30`
has explicit activator arms for all four — but they are **spawned by DIFFERENT
scripts in DIFFERENT areas**, never in the town:

- The narrative scene SCRIPTS are a long sequential family
  (`0x4d7d80`/`0x5034b0`/`0x539e80`/… — dozens), each called every frame from the
  story-FSM `FUN_0040c380` (gated on the story-progress flag `FUN_0041e2f0(0x5f76805)`
  in `[1000, 0xb55)`).  A handler is a `switch` on the **live room id**
  `**(DAT_008a9b50+0x1038)` and does nothing unless the current room matches one of
  ITS cases — so a handler is inert in every room it doesn't own.
- The high digits of a room id ARE its area: town = area 210, room `0x334be`
  (210110).  `FUN_00539e80` owns area-410 rooms (`0x641fe`/…/`0x64280`) and spawns
  `0x1872e` in `0x64280` (410240); `FUN_005034b0` owns area-230 rooms
  (`0x382de`/…) and spawns `0x1872f` in `0x382de` (230110); `0x18730` is a child of
  CHARACTER `0x11350`, which is not one of the town's 32 map-object char codes.
- Cross-check on the spawn paths: all four codes are OUTSIDE the 70000-79999
  CHARACTER range, so the map-object layer pass `0x58d460`→`0x431e30` (which only
  walks 70000-range objects, #79) never makes any of them — they reach `0x431e30`
  ONLY via a script's `FUN_00431d10(code, …)` call.  The town script `0x4d7d80`
  (area-210 rooms) issues exactly one such call: the wagon `0x1872d`.

**Lesson for the RE loop:** to decide whether actor code X belongs to scene S,
find X's `FUN_00431d10(…,X,…)` call site and read the room-id `case` guarding it —
do NOT infer scene membership from code adjacency or from `0x431e30` having an arm
for it (every code in the game has an arm).  `findings/in-game-intro.md` "The
caravan 'siblings' … are OUT-OF-SCENE".

### #84 — the in-game scene's visible objects are FOUR map-object bands, each with its own per-frame renderer; STRUCTURE scenery is fully map-driven (pos = map×100, frame_base = map variant@+0x18) (2026-06-07, ckpt 83)

The per-frame world driver `FUN_0048c150` (free-roam branch, `in_ECX[7]==0`) walks
several actor-pool bands off `DAT_008a9b50`, each by a DEDICATED render/emit fn,
then one present `FUN_0048eac0` flushes the shared 27-layer draw_pool.  For the
opening town hold (cam 128000) the visible objects partition by the band's
type-RANGE (the #79 spawn dispatch):

| band | render fn | type range | role at the hold |
|---|---|---|---|
| `+0x11e0` | `0x491ae0` | 70000 CHARACTER | collision volumes (bank 0, invisible) + props (`0x16c`) + script wagon |
| `+0x2560` | `0x493230` | 60000 STRUCTURE | the **TREE** (`0xec55`→bank `0x15f`/res `0x481`), bg decorations (`0xec6a`→`0x16c`), fg hedges (`0xec60`→`0x164`/res `0x426`, layer 15) |
| `+0x1160` | `0x493ba0` | 50000 EFFECT | the **townsfolk** (multi-part chars; banks `0x8b`–`0x146`) |
| `+0x13e0` | `0x493480` | (script) | animated bank-`0x1aa` particles (res `0x408`, alpha — not in the keyed set) |

`0x493230` is a SINGLE-cel renderer (reads dir at `+0xe8`, sprite row at
`+0x48+dir*0x14`, render-state pos `+0x04`/`+0x08`, clip `+0x6c`; emits one cel via
`0x4917b0`).  `0x493ba0` is the multi-part character renderer (built on the ported
`0x44d160` descriptor + `0x492670`/`0x4917b0` emits, with shadow/color-split
layers).

**STRUCTURE is a pure function of DATA-1022:** the live render-state world pos =
the map record's `(x,y)`@+0x04/+0x08 ×100, and the sprite `frame_base` = the map
record's **variant @ +0x18** — verified cel-for-cel (tree {0,1}, hedge {0,1,4,5},
deco {16,18,20,21,24,26,28,32,33,35} all identical live-vs-map).  The code→bank
map is the activator's per-type def table (the lazy `+0x48` fill, #80):
`0xec55`→`0x15f`, `0xec60`→`0x164`, `0xec6a`→`0x16c`.  EFFECT townsfolk also map
1:1 by code/count but carry a deterministic spawn offset (≈+3000 x) from the
`0x41f200` activator.  Across the pre-pan hold the 16 standing townsfolk + 39
structure objects are FIXED (only the anim frame steps, #76); only `0xe29a` ×4
roam (RNG, refines #82 — the EFFECT band updates during the hold).  The "foreground
tree" is thus a STRUCTURE map-object, not a banner/`0x5a00c0` overlay/tile.
`findings/in-game-intro.md` "The establishing-hold cast is FOUR map-object bands".

### #85 — townsfolk FACING is a deterministic MAP field (not RNG); idle PHASE + fountain ARE RNG; the 8 rand draws in `0x41f200` are position-jitter + a particle sub-spawn (2026-06-07, ckpt 85)

Resolving the three ckpt-84 RNG residuals (facing / idle-phase / fountain) by RE +
a live census, the facing turns out to be RNG-FREE while the other two are genuine
LCG consumers — correcting the ckpt-84 guess that all three were RNG.

**FACING (the "flipped orientation") is the map sub-record field `puVar1[4]`.** The
room-object dispatcher `FUN_0058d460:96` computes the facing as
`cVar12 = (-(puVar1[4] != 0) & 2) + 1` → **1 (normal) or 3 (mirrored)**, where
`puVar1 = *(*(mapobj+0x3c) + i*0x10)` is the per-object sub-record (built by the
unported decoder `0x587e00`).  It forwards `cVar12` as **param_8** to BOTH the EFFECT
activator `0x41f200` (`:151`) and the CHARACTER activator `0x431e30` (`:227`);
`0x41f200:861` stores it at render-state `+0x2c` and passes it to every `0x426620`
sprite install.  The renderer `FUN_0044d160` mirrors only on `facing == 3`: it picks
the mirror cel `frame += flip` and reflects `off_x = mirror_x - off_x`, where
`flip = *(short*)(DAT_008a8440[bank])` — **`0x8a8440` is a pointer array** (confirmed
live: each cell derefs to a heap sprite-group descriptor) whose first short is the
group's **frames-per-direction** (read live for the town banks: 4 or 16).  So the
flip cel = `frame_base + frames_per_dir`.  No RNG anywhere on this path.

**IDLE PHASE is RNG.** `FUN_00426ec0` (the spawn phase-randomizer, called per actor
in the `0x41f200` chain) does, when the render-state has a clip (`+0x6c != 0`):
`+0x72 = (rand() * clip.frame_count@+0x42) >> 15` (a uniform start frame in
`[0, frame_count)`), then a 2nd `rand()` for the timer phase via `clip+0x44`.  The
live census confirms every townsperson runs the idle clip `0x6290e0` with a scattered
per-actor start frame.  So matching the idle phase 1:1 needs the **game_enter RNG
anchor** (snapshot/restore `DAT_008a4f94` both sides) + replaying the spawn draws in
order — it is NOT deterministic.

**The 8 `FUN_005bf505` calls in `0x41f200` are position-jitter + a particle
sub-spawn, NOT facing/phase.** Two (`:294`/`:301`) feed `FUN_00426e00` writing the
motion sub-struct `+0x58`/`+0x60`; five (`:326-334`) feed `FUN_00427b70` (the particle
sub-emitter).  Helper `0x427670` draws 20× (the fountain particle init).  These + the
per-tick `0x47b990`/`0x453960` are the FOUNTAIN SPRAY (band `+0x13e0`/`0x493480`),
also RNG.

Ground truth (live `0x493ba0` census, `--seed-pin --lockstep --no-turbo`, flips
1450-1600): of the 11 map townsfolk, **7 are facing 3** (`0xc3be`/`0xc3dd`/`0xc3e6`/
`0xc422`/`0xc42c`/`0xc441`/`0xc468`), 4 facing 1 (`0xc3f2`/`0xc404`/`0xc440`/`0xe2a5`);
flip (frames/dir) per bank read live from `DAT_008a8440`.  PORTED: facing + the flip
table (`actor_spawn.c` defs + `actor_spawn_effect_fill_flip_table`, wired in
`main.c`); the townsfolk now mirror RNG-free.  Idle-phase + fountain remain (need the
RNG anchor).  `findings/in-game-intro.md` "Townsfolk facing is a map field".

### #86 — the town SPAWN draws a fixed RNG burst of 19 EFFECT objects at the game_enter load frame; the re-pin point is the FIRST `0x41f200` (a pre-spawn `0x4c5e00` draw sits between game_enter and it) (2026-06-07, ckpt 86)

Ground truth from the seed-pinned `0x5bf505` flow-trace census (the new
`runs/rng-census-repin` capture, `--seed-pin --lockstep --no-turbo`).  At the
single town-LOAD frame (`game_enter`, flip 1419/1434 — it VARIES run-to-run, quirk
#77), the room object-population pass `0x58d460` dispatches the map's EFFECT objects
to `0x41f200` in map-layer order, drawing a deterministic **238-draw burst over
exactly 19 EFFECT objects** (the port renders only 11 standing townsfolk, but all 19
consume RNG — the 4 wandering `0xe29a` + 4 more script/particle effects draw too).

**The per-object draw pattern** (run-length, from the trace) is, in order:
`0x426fd0`×1 (the `+0xf4` field init, `0x41f200:237`) → `0x41f200`×7 (the prologue:
2 position-jitter `:294`/`:301` + 5 particle-param `:326-334`) → **optionally
`0x427670`×5** (the per-case particle init — fires for 4 of the 19, the shape-2
objects) → `0x426ec0`×2 (the idle anim PHASE: start frame `+0x72` + timer `+0x70`).
So an object is **10 draws** (shape-1, no `0x427670`) or **15 draws** (shape-2).  Two
objects carry an extra per-case one-off INSIDE the type switch (`0x431cb0`×1,
`0x427360`×1), and one fires the conditional `0x41f200:2849` draw (`0x425caa`).  The
totals are stable across runs: `0x41f200` 134, `0x426ec0` 38 (=19×2), `0x426fd0` 19,
`0x427670` 20 (=4×5) — the re-pin changes seed VALUES, not the draw COUNT/ORDER
(control flow is keyed on the object CODE, not the seed).

**The re-pin point is the first `0x41f200`, NOT the `game_enter` anchor.** A lone
pre-spawn draw `0x4c5e00`×1 (plus the engine's own `0x439690`/`0x5531b0`/… ticks)
fires BETWEEN the `game_enter` entry (`0x59f2c0`) and the first `0x41f200`, so pinning
`DAT_008a4f94` at `game_enter` would leave the spawn one draw out of phase vs a port
that replays only the `0x41f200` effect burst.  Verified live: at the first
`0x41f200` the natural seed was `0x71cc78f1` while `game_enter`'s was `0x46fe3f46`
(different → there ARE intervening draws).  The harness re-pins at `0x41f200` onEnter
(armed at `game_enter`); the port re-seeds at the top of `enter_game` (mirror — all
pre-effect-spawn code is RNG-free).  `findings/in-game-intro.md` "The town SPAWN RNG
anchor".

### #87 — the engine's PARTICLE subsystem: a 1024-slot `+0x13e0` DEVICE pool, round-robin alloc (`0x557370`), per-code config (`0x557550`) + per-tick physics (`0x46e510`), alpha-blit render (`0x493480` default arm); the fountain spray is emitter `0x112e5` → `0x18708` (2026-06-07, ckpt 88)

Read end-to-end from the decompile (live capture pending).  The visible particle
effects (fountain spray, leaves, combat hits, dust) are one subsystem distinct from the
actor bands:

- **Band.**  `DAT_008a9b50 + 0x13e0` is a **1024-slot** pool (`0x46cd70:103-112` walks it,
  `iVar10 = 0x400`, calling `0x46e510` per active slot).  Slots reuse the actor render-
  state shape (`+0x40`→record: `+4/+8` world x/y, `+0x18`/`+0x28` y/x velocity, `+0x2c`
  facing, `+0x58` sub-phase, `+0x5c` lifetime, `+0x6c/+0x70/+0x72/+0x74` clip/anim,
  `+0x1d0` active, `+0x1d4` code, `+0x280` age/priority).
- **Alloc** `0x557370`: round-robin to the first free slot (`+0x1d0==0`, cursor
  `*(mapctl+0xce) & 0x3ff`); on full, evict the oldest (lowest `+0x280`) below the
  caller's priority.  Position is parent-relative by anchor mode (1=center / 2=top /
  3=center-top / 4=center-bottom), then → `0x557550`.
- **Config** `0x557550` (a 21 KB per-code switch).  Each particle CODE installs its bank/
  frame (`0x426d70`), clip (`0x407b80`), optional launch-velocity scatter (`0x453960`, 2
  draws), and body (`0x426620`).  The fountain droplets are bank **`0x1aa`** (res `0x408`):
  `0x18708` frame 6 (main water), `0x18704` frame 8 (upward), `0x18707` frame 8, `0x18709`
  frame 0.  `0x186ca` is a sprite-less controller (timer `+0x96=0x3c`).
- **Per-tick** `0x46e510` (a 10.7 KB switch on `+0x1d4`): integrate `x += ±vel/100`,
  `y += vel_y/100`; age the y-velocity toward a clamp (gravity — `0x18708` +8000/tick→
  down, `0x18704` −500/tick→up); cycle the clip; fade via the alpha LUT `&DAT_008a9308`
  into `in_ECX+0xf4/+0xf8`; expire on lifetime or a collision-grid hit
  (`(x,y)/0xc80 → mapctl+0x21c04`).
- **Render** `0x493480` (the `+0x13e0` renderer, from `0x48c150`): the **default arm**
  (`:93-117`) is `0x44d160` describe → `0x4917b0` **ALPHA-blit** (clipped/translucent —
  NOT the keyed `0x5b9b70` opaque path; a new sink the port lacks).  The `0x186ca` arm
  is a separate horizontal cel-string renderer (the controller, not a particle).

**The fountain.**  The emitter is the fountain prop **`0x112e5`** (CHARACTER `+0x11e0`,
already spawned — quirk #80), whose per-tick behaviour `0x54f980:218` spawns one `0x18708`
droplet each primary sim-tick (2 RNG for spread + ~2 for sound).  The spray is therefore a
**continuous per-tick RNG consumer**, NOT gated by the scene-lock `+0x27a8` (so it runs
during the establishing hold).  Corrects the ckpt-84 census guess that the spray "lives in
`0x47b990`/`0x453960`" — `0x47b990` is the `+0x1160` behaviour/AI dispatcher (no fountain
code), `0x453960` is a generic 2-draw scatter helper.  `findings/in-game-intro.md` "The
FOUNTAIN SPRAY".

### #88 — particle render-state +0x2c (facing) is **1** for every particle (sky AND water) → x integrates `+= +vel_x/100` (no sign flip); the `0x112e2` sky emitter is an INVISIBLE trigger so its anchor (`0x557370` mode-1 = +0xc/2) is 0 (2026-06-07, ckpt 89)

Two retail ground-truth facts that pin particle placement (live-captured, `runs/sky-facing`
+ `runs/rng-census-repin`):

- **Facing.**  Every active `+0x13e0` particle has render-state **`+0x2c == 1`** (capture:
  34/34 `0x18704` + 63/63 `0x18708`).  `0x46e510`'s x-integration is
  `iVar = (+0x2c != 1) ? -0x51eb851f : +0x51eb851f; x += iVar*vel_x>>… (= ±vel_x/100)` — so
  with +0x2c==1 the sign is **+** (no flip).  Consequence: the sky particle's negative scatter
  vel_x makes it **drift LEFT** (world X extends LEFT of the emitter prop: retail
  `[50690..114356]` vs props `62800`/`113600`).  The fountain's 3-way cycle (left-strong/right/
  left-weak) is likewise relative to +vel_x/100 — a port that spawns facing 0 mirrors every
  particle's horizontal motion (invisible on the ~symmetric fountain spray, wrong for the sky).
- **Anchor.**  `0x557370` mode-1 spawns at `parent.world_x + parent_render_state[+0xc]/2 +
  jitter`.  +0xc is the emitter's display half-extent in world units: an INVISIBLE emitter (the
  `0x112e2` sky trigger, no cel) has +0xc == 0 → anchor 0 (particles spawn at the prop's exact
  world pos; trace: `0x18704` fresh ≈ prop ± jitter).  A VISIBLE emitter (the fountain prop)
  has +0xc ≈ 2810 → +1405; note this is NOT the prop's display-cel width (that measures 3400/
  +1700), so +0xc is a distinct field whose setter is still un-RE'd.

- **Sky vs fountain particle systems** (both bank `0x1aa`, both render via the `0x493480`
  alpha arm): `0x18704` = **chimney SMOKE** (emitter `0x112e2`, layer 6, clip `0x644b58`
  6-frame ONESHOT → expires on completion, fade via **ramp_b** `0x8a9308`, drifts up+left);
  `0x18708` = fountain WATER (emitter `0x112e5`, layer 11, clip `0x6449c0` 2-frame loop, fade
  via **ramp_a** `0x8a92e0`, gravity-down).  The town's `+0x13e0` band renders ONLY these two
  codes.  `findings/in-game-intro.md` "The SKY-AMBIENT particles".

### #89 — the establishing-scene REVEAL is a VERTICAL FADE opening from the MIDDLE of the screen (corrects the "dark top gradient" read); + ground BUTTERFLIES and a Start-Game SHRINK-DOWN are port-missing elements (2026-06-07, ckpt 89, golden video review)

Spotted in the ckpt-89 dense golden capture (`runs/video60-retail`) + the USER's real-time
side-by-side review.  Three retail behaviors the port lacks:

- **The establishing REVEAL** — CONFIRMED off the golden (frame-stepped at game_enter): the
  scene is revealed by a **vertical fade that opens from the MIDDLE of the screen outward**.
  At game_enter the frame is fully BLACK (sim_tick ~6), then a horizontal band of the scene
  grows from the vertical center (t~13 → t~23), the black top + bottom shrinking, until it
  settles into the cinematic LETTERBOX (the quirk-#74 thin top/bottom bars).  So the
  long-open "dark establishing-shot TOP GRADIENT" (ckpt 66/67) is NOT a static tint — it is
  this reveal mid-animation (the top is still black before the iris fully opens).  A
  vertical-curtain/iris reveal.  The port jumps straight to the letterboxed scene (no reveal).
- **BUTTERFLIES** flit near the GROUND in the town square — by the flowerbeds / the townsfolk
  (USER-pinpointed: next to the girl in pink, above the flowers, over the dog; small, easy to
  mistake for birds).  Absent in the port.  KEY: they appear at the **settled town** (~retail
  flip 2150), which is AFTER the establishing-hold range my particle census covered.
  **RESOLVED ckpt 96 (quirk #93) — and the particle hypothesis here was WRONG.** They are NOT a
  `0x557550` particle (the `+0x13e0` band renders only `0x18704`+`0x18708` at any frame); they are
  the 4 **`0xe29a` EFFECT** objects (res `0x3fa`, bank `0x146`, clip `0x65ddf0`, `0x493ba0` layer 12)
  that every prior checkpoint mis-labelled "wandering villagers" and the port excluded.  Ported.
- **The Start-Game menu has a SCALE transition BOTH WAYS** (USER): it scales **IN/UP** when it
  APPEARS (a grow-from-small reveal) and the reverse — scales **OUT/DOWN** — when dismissed
  (confirming Start Game, id `0x24`), before the next scene.  The port pops it in/out with no
  scale.  (A scale-anim on the menu panel — likely the same blit-scale path as other UI; RE the
  menu show/hide transition.)

(Separately, the PORT's menu-cursor pulse looked fast in the review — a port-side timing item
to verify at matched timing, possibly the dev-build's uncapped flip rate or the 2x video
speed-up; NOT a retail quirk → tracked as a TODO, not here.)

### #90 — the establishing REVEAL is a per-cell FADE-GRID transition (`FUN_0048e920` render / `FUN_0049af40` update), NOT the letterbox bars; the letterbox is a constant 64 px the whole shot (2026-06-08, ckpt 90)

Live-RE'd the #89 reveal.  Pixel-measured envelope (golden `runs/video60-retail`, strict
pure-black rows): top/bottom black ramps `~240 → 64` at **−8 px/sim-tick**, floor 64, settling
~sim-tick 25 — a center-out iris over a STATIC scene.

The intuitive cause (the quirk-#74 LETTERBOX bars ramping 240→64) is **refuted by field
capture** (`runs/letterbox-reveal{,2}`): both the scene-cinematic step `0x499ab0`'s and the
grid-fill `0x48c150`'s `this+0x44`/`+0x48` (the bar heights) read **constant 64** the entire
reveal, and the scroll `+0x4c` is **constant 0**.  The letterbox is fixed 64 px; the reveal is
a separate producer.

A `0x5b9a40` black-cel (res `0x583`) blit trace (`runs/reveal-blit`) finds the producer:
besides the constant letterbox bars (`ret_va 0x48c48a`/`0x48c4fe`, 160 tiles each), **`ret_va
0x48e9c3` emits ~1010 → 0 black tiles across the reveal** (gone by the settled hold).  That is
**`FUN_0048e920`** (403 B), a **scene-transition FADE-GRID**: a grid of 64×4 cells (`this[0]`
array / `this[2]` count / `this[3]` mode; stride 0xc = state/timer/col/row) each ALPHA-blitted
(`0x5bd550`, alpha `0x1f-(timer<<5)/1000`), OPAQUE-blitted (`0x5b9a40`), or skipped per cell —
the center-out clear pattern is the iris.  Rendered from `0x48c150:175` (after the letterbox).
This also explains the ckpt-66/67 "dark establishing TOP GRADIENT": the fade-grid
mid-animation, not a tint.  Full writeup: `findings/in-game-intro.md` "The establishing REVEAL
is a per-cell FADE-GRID".

**CORRECTION (ckpt 95) — the update is NOT `0x49af40`; the grid is `*(0x8a9b50+0x1040)`.**
Reading `FUN_0049af40` (3313 B), it is the per-frame **HUD/portrait/HP-bar animator** (walks
the 8 party slots `room+0x4030`, lerps the HP/MP fill bars + portrait fade timers, returns a
counter) — it never touches a 64×4 grid.  The real per-cell **update is the INLINE loop at
`0x499ab0:125-177`** (advance each fading cell's timer, `1×/sim-tick`), and the iris **pattern**
is set by **`FUN_0049a890`** (variant 0, center-out) / `0x49a740` (1, edges-in) / `0x49aae0`+
`0x49aa00` (2, sweep).  The grid object is **`*(0x8a9b50+0x1040)`**; it is **armed at
`0x439690:555-583`** (mode = request +0x28, **variant = `(rand*3)>>15`** ∈ {0,1,2} — one LCG
draw, the iris shape is RNG-chosen, speed = request +0x2c, then fill W×H cells).  The measured
−8 px/sim-tick = mode-1's **2 rows/tick** × the 4 px row pitch, `1×/sim-tick` (not the ckpt-90
"`0x49af40` 2×").  **PORTED ckpt 95: `src/scene_fade.{c,h}`.**  Live town params
(`runs/reveal-grid`): W=10, H=120, count=1200, mode=1, speed=1000, variant=0.

### #91 — character identity is a 32-bit HANDLE resolved through the "Get Dramatist Info" table `DAT_006b6ea8` (`{handle, code, name, bank}`); `0x41f200` maps handle→archetype code + sheet bank at spawn (2026-06-08, ckpt 92)

Retail ground truth (static `.rdata` + decompile + the `runs/cutscene-cast` census; proof
`docs/proofs/dramatist-table.md`).  A *named character* (a "dramatist") is keyed by a 32-bit
**handle**, not by its actor code.  **`FUN_0041f200`** (the EFFECT activator) logs the literal
`"Get Dramatist Info"` (`:51`), then (`:54-69`) when spawned with a non-zero handle (`param_9`)
+ code 0 it linear-scans the table **`DAT_006b6ea8`** (rows of `0x34` bytes =
`{+0x00 u32 handle, +0x04 u32 code, +0x08 char[0x28] name, +0x30 u16 bank}`, handle==0
terminated) and: sets the actor's effective **code** `+0x1d4` = the row's code (the ARCHETYPE
/ sprite-switch selector — shared by many NPCs), and carries the row's **bank** as `sVar17`,
which **overrides** the archetype's facing-default sheet (each `case`:
`if (sVar17==0) sVar17 = <default>; FUN_00426d70(0, sVar17, 0)`).  So **code = archetype
(pose/clip set), bank = the specific sheet**; the handle picks both.  E.g. case `0xc440` is the
**"Woman"** archetype (string `s_Woman_00855174`): facing-1 default bank `0xa6` (the generic map
townswoman) but `0xb5` for Arche's Mother (handle `0x5f5e1d4`); case `0xc35a` is **Arche** (banks
`0x8b`/`0x8c`/`0x8d`, clip `0x62a8c8`).  The town-intro cutscene `0x4d7d80:334be` spawns the
arriving family BY HANDLE (`0x41f0e0(0x5f5e1d3=Father, …)`, `0x41f0e0(0x5f5e1d4=Mother, …)`) +
Dr. Barnard (`0xc3f0`) by code, then resolves the persistent LEADER **Arche** (`0x5f5e165`, code
`0xc35a`) — created at new-game — for dialogue.  Distinct from the *live-actor* handle registry
(`0x556eb0` resolve / `0x555f00` add-remove over `DAT_008a9b50+0x2790`, matching an actor's
`+0x1d8`): `DAT_006b6ea8` is the static character DEFINITION, the registry is the runtime lookup.
The full table is the game's entire named-NPC roster (party leads Arche/Sana/Stella/Chiffon at
bank 0 = dynamically loaded; teachers, shopkeepers, monsters, …).

### #92 — the playable-character body sheets are EXE-EMBEDDED sprites with a numeric COLLISION: res `0x570`–`0x573` are `DATA` (sprites) in sotes.exe but `WAVE` (sounds) in sotesd.dll (2026-06-08, ckpt 94)

Retail ground truth (PE `.rsrc` manifests of both modules + a live slot read,
`runs/arche-res`/`runs/arche-params`).  Arche the party leader (code `0xc35a`) installs a 4-bank
body **`0x8b`–`0x8e`** (sprite slots **126–129** = bank−13; `0x41f200` case `0xc35a` `:899`,
`FUN_00426db0(row, 0x8b..0x8d, …)` + `+0x21 = 0x8e`).  Those banks resolve to PE resources
**`0x570`/`0x571`/`0x572`/`0x573`** (group 3, type 2, scale 1, ck 0; dims 80×80/80×80/80×96/80×80).
The trap: **the same numeric resource id is two different assets in two different modules** —
`0x570`–`0x573` are type **`WAVE`** (sound effects) in **sotesd.dll** but type **`DATA`** (the
lizsoft sprite container) in **sotes.exe**'s own `.rsrc`.  The town *NPC* sheets (res `0x459`–`0x47b`,
slots ~161–236) live in sotesd.dll; the **playable-character** sheets (Arche/Sana/Stella…) live in
the EXE, and retail loads them via `FindResourceA(NULL = the exe module, …, "DATA")` (= the
`init_sprite_banks` "EXE-embedded banks `0x570`-`0x572`" note).  A sprite slot's source module is its
`settings` field (`FUN_005748c0` arg2; `ar_sprite_decode` reads it as the `FindResource` HMODULE), so
registering these slots with `settings = NULL/the-exe-handle` is what makes them decode from the EXE.
This collision is what derailed ckpt 90 (the `0x8b→0x4fb` read was the *sound* table
`game_sounds[139]`, not a sprite — retracted in 91b).  **The character's identity is its `0x493ba0`
render path + bank, NOT the bare resource number** — always pin which MODULE a resource id means.

### #93 — the town `0xe29a` EFFECT objects are the BUTTERFLIES, not "wandering villagers" (corrects #84–#89; res `0x3fa` / bank `0x146`, clip `0x65ddf0`) (2026-06-09, ckpt 96, USER-confirmed)

The 4 map `0xe29a` (58010) EFFECT objects were called "wandering villagers" from ckpt 83 onward
(the spawn excluded them, consuming only their RNG).  They are actually the **4 small BUTTERFLIES**
that flit by the flowerbeds at the settled town (USER-pinpointed: over the dark wood beam, below
the **ARMS** weapon-shop sign, above the dog — retail flips ~2028 + 2138; tiny ~3–5 px, easy to
mistake for birds).

**Ground truth (live, seed-pinned + lockstep, `runs/butterfly-{census,allbands,blits,emit}`):**
- They render via **`0x493ba0`** (the EFFECT multi-part renderer) at **layer 12**, sprite **res
  `0x3fa`** (bank **`0x146`**, sprite-pool slot **313** = bank−13; **32×32**, group-3 registered in
  sotesd.dll — a DATA resource, NOT exe-embedded), animated by clip **`0x65ddf0`** (decoded: base 0,
  **3 frames, dur 4, looping, delta {0,1,2}** — a fast wing-flap, vs the villagers' 20-frame breathe).
- The butterfly emit world positions match the `0xe29a` `0x493ba0` render positions **1:1** (e.g.
  @flip 2138: 0xe29a at (103850,44750)/(107990,45140)/(187360,42810)/(173050,44990); res `0x3fa`
  emits at (104250,44110)/(107960,44500)/(186960,42450)/(173450,44350)).
- **Two colour variants** (yellow + white/gray) = different cel ranges in the sheet, selected by the
  per-instance **frame_base / facing** (census `row0_bf` high bits = 0/4/8/12 → cels 0-2 / 4-6 /
  8-10 / 12-14, plus higher variants; live `cel_fr` 0/4/5/8/12/17/25/30).  The wander is real (the
  5 `0x427670`-case-2 draws #86 attributed to "the wanderers" ARE the butterfly's RNG flit).

**Why it hid for ~13 checkpoints:** the particle/cast censuses ran at the establishing HOLD
(flip ~1500), where the butterflies are off-screen LEFT (they live at world x 104k–187k = the
inn/arms half, which the camera only pans to during the arrival); and the EFFECT band census read
only the actor `code` + `row0_bf` bank, never the rendered cel's **resource id**.  **Lesson:** to
identify a small/ambient actor, capture the rendered RESOURCE (the blit `res` / emit `cel_res`), not
just the code+bank — a "wandering NPC" can be a butterfly.  Ported ckpt 96 (`src/actor_spawn.c`:
`0xe29a` added to `TOWN_EFFECT_DEFS` + `BUTTERFLY_CLIP`); per-instance direction/variant + RNG
wander drift are PORT-DEBT(butterfly-wander).

### #94 — the town room-load RNG burst is **19 EFFECT objects** (15 MAP + 4 SCRIPT cutscene cast), and the establishing-REVEAL iris-variant draw fires AFTER the whole burst (2026-06-09, ckpt 97)

The first in-game frame (the seed-pinned `0x5bf505` census `runs/rng-census-repin`, sim-tick 0,
238 draws) consumes the LCG in this exact order — the keystone for the scene-wide RNG-phase work:

1. **`0x4c5e00`** — 1 pre-spawn draw (BEFORE the re-pin point; the port re-seeds `enter_game`
   AFTER it, matching ckpt 86: the re-pin is the first `0x41f200` onEnter, and `0x426fd0` is called
   *inside* `0x41f200` so it lands post-re-pin).
2. **The 19-object EFFECT spawn burst = 213 draws**, each object via `0x58d460`/`0x4d7d80` →
   `0x41f200`, shape `0x426fd0`(1) + `0x41f200`(7) + [per-type extra] + `0x426ec0`(2):
   - **15 MAP objects (171 draws)** — `0x58d460` room population, map-layer order: 10 plain
     townsfolk (10 draws each) + **4 butterflies** `0xe29a` (15 each: `+0x427670` case-2 ×5) + 1
     `0xe2a5` (11: `+0x431cb0` ×1).  Ported by `actor_spawn_effect_from_map`.
   - **4 SCRIPT objects (42 draws)** — `0x4d7d80` (the town intro cutscene, case `0x334be`) spawns
     the arrival cast via `0x41f0e0`→`0x41f200`: **Arche** (`0xc35a`) is obj16 and draws **12** (her
     `case 0xc35a` is the ONLY one that calls `0x427360` (+1) and trips the conditional
     `0x41f200:25caa` (+1)); Barnard/Father/Mother draw **10** each.  Ported by
     `actor_spawn_cutscene_cast` (ckpt 97 — previously consumed 0 → the bug below).
3. **`0x439690`@`0x43a941`** — **1 draw = the establishing-REVEAL iris VARIANT** `(rand*3)>>15`
   (`0x439690:555-583`, quirk #90 / `src/scene_fade.c`).  It fires AFTER all 19 spawns, so its value
   depends on the FULL 213-draw advance.  From the pinned seed `0x4f5347`: after 213 draws the draw
   = 7211 → variant **0 (center-out)** = retail's live town value.  The pre-ckpt-97 port consumed
   only the 15 map objects (171) → the draw = 31664 → variant **2 (sweep)** — the
   PORT-DEBT(scene-fade-rng-phase) bug.  **Only the draw COUNT matters** (the MSVC LCG state after N
   steps is independent of how the values are used), so the cutscene cast just consumes 42 to advance
   the phase; the iris is now DRAWN, not pinned.
4. Then the first per-tick update `0x46cd70`: `0x47b990` ×4 (the butterflies — the ONLY EFFECT-band
   per-tick RNG consumer; townsfolk go to the RNG-free `0x478ba0`), then the `0x54f980`/`0x453960`
   particle emitters.  (Per-tick model: quirk #95.)

Verified: host test `actor_spawn_cutscene_iris` (the 42-draw contract + variant 0 vs the bug's 2) +
the live port log (`scene_fade_arm … variant=0 … DRAWN at the post-spawn LCG phase`).  This retires
the RNG-phase half of PORT-DEBT(scene-fade-rng-phase); the residual is only the skipped black-load
WINDOW (the reveal's absolute start-tick offset).

### #95 — the town's PER-TICK LCG stream is a clean COUNT model (butterflies even + fountain 6 + sky every-6); the ckpt-73 "non-deterministic RNG" was the missing butterfly draws, NOT true nondeterminism (2026-06-09, ckpt 98)

Under the seed pin, the town's per-sim-tick LCG consumption is fully deterministic and reproducible —
resolving the ckpt-73/#77 "the stream desyncs run-to-run even under `--seed-pin`" mystery: that was
the *port* missing the EFFECT-band draws + the non-deterministic presents-per-tick (quirk #75), not
nondeterminism in retail's per-tick logic.  Ground truth: the seed-pinned `0x5bf505` per-tick census
(`runs/rng-census-repin`), validated bit-exact by replaying the MSVC LCG against the `0x46cd70`
onEnter rng at **293 of 298 ticks** (the 5 misses are named irregulars, below).

**The per-tick driver `0x46cd70` walks the bands in this order; only two draw RNG in the town:**
1. **EFFECT band `0x47b990`** — called ONLY for update-mode-1 actors (`actor+0x200==1`), which in the
   town is just the **4 BUTTERFLIES** (`0xe29a`); the 11 standing townsfolk + the 4 cutscene cast take
   the RNG-free arm `0x478ba0`.  So the butterflies ARE the EFFECT-band per-tick stream.
   - **Every OTHER sim-tick** (the 1-bit gate `0x14232`: work when 0, set 1; next tick dec to 0 +
     return).  Fresh-spawned they share phase → all 4 work on EVEN ticks.
   - On a work tick: the flit-pick timer `0x14236` (work-tick countdown reloaded to `0x50`) gates the
     wander draws — when 0 it draws the move test `(rand*1000)>>15 < 0xc874` (+ a 2nd draw, the flit
     offset, if it passes) then reloads `0x50`.  Init 0 → fires on the spawn tick, then every `0x50`
     = 80 work-ticks = **160 sim-ticks** (observed re-fire: tick 162).
   - The `0xe29a` case (`:768-801`) then ALWAYS draws twice: heading `0xc890 = (rand*0xc80>>15)+0x640`
     and the flutter flag `(rand*1000)>>15 < 100`.
   - So a butterfly draws **2** per work-tick (heading+flag), or **3-4** when the flit pick fires.
     Each one's `0xc874` (~650-749) is set at spawn by `0x427670` case 2 (= `(rand*100>>15)+0x28a`,
     the 5th of its 5 draws).  The drawn values feed the flit MOTION (`0x43f880`, the 5.5 KB
     movement/collision FSM) + the facing/bounds — DEFERRED, so the butterflies hold position but the
     stream advances (consume-to-advance).
2. **CHARACTER band `0x54f980`** — the particle EMITTERS: the fountain `0x112e5` (6 draws/tick: jitter
   y/x + splash×2 + velocity×2, the velocity either inline `0x550bf8/0x550c22` or via `0x453960`,
   always 6) and the 2 sky `0x112e2` emitters (4 draws each — jitter + `0x453960` velocity — every 6th
   tick, at ticks 5/11/17/23/…).  Already ported (`src/particle.c`); the cadence matches.

**The clean count model** (reproduces retail's per-tick rng exactly):
`tick 0 = 23` (14 butterfly + 6 fountain + 3 first-tick init); then per tick N≥1:
`6 (fountain) + 8·[N even] + 8·[N≡5 mod 6]`.

**The irregular consumers — RESOLVED (ckpt 99, `src/ambient.{c,h}`; corrects the earlier "5 misses /
all 0x5531b0" guess).**  A seed-pinned timer-state capture (`runs/ambient-timer`, the
`0x5531b0`/`0x467380`/`0x54f980` field-spec reads of each one's `+0x5c`/`+0x20c` countdown) pinned the
residual to **FOUR self-clocked ambient/event timers** (+ the butterfly re-fire that `butterfly.c`
already models), ALL clean **unit-decrement** (1/sim-tick) — not the suspected "all `0x5531b0`":
| timer | mechanism / band | init | fires | draws |
|---|---|---|---|---|
| `0x11370` | `0x5531b0` ambient SOUND (CHARACTER) | `(rand*300)>>15` = 33 @t0 | tick **33** | +3 |
| wagon `0x1872d` | `0x54f980:932` idle-wander (CHARACTER) | `(rand*300)>>15` = 134 @t0 | tick **134** | +3 |
| `0x467380` | `0xe2a5` event timer (EFFECT, via `0x442a70`) | `+0x20c`=184 (spawn-set) | tick **183** | +4 |
| `0x1136f` | `0x5531b0` ambient SOUND (CHARACTER) | `(rand*300)>>15` = 189 @t0 | tick **189** | +3 |
A 0x5531b0/0x54f980 timer fires when its countdown hits ≤0 (so init `C` → fires at tick `C`); the
`0x467380` fires at `+0x20c==1`.  **The census's earlier C-values (141/189/33) were off-by-one** — the
`0x5bf505` `rng_state` field is the state *before* the draw, so the returned value is
`rval(step(state))`, not `rval(state)`; reading the `cd` directly gives 189/33/134.  **Order in the
`0x46cd70` walk** (proven by the capture seq order): EFFECT `butterfly_step → 0x467380`, then CHARACTER
`fountain → sky → 0x1136f → 0x11370 → wagon`.  Ported as four consume-to-advance timers
(`ambient_effect_step` + `ambient_character_step`); the values feed sounds / the wagon wander / an
`0xe2a5` sub-effect (none ported) but the COUNTS + TIMING keep the stream aligned.  **The whole
settled-town per-tick stream is now bit-exact** (offline replay 0/297 vs the capture; live port
bit-exact ticks 0-248 through all four fires 33/134/183/189; host test `ambient_pertick`).  Retires the RNG residual of
PORT-DEBT(fountain-rng-phase) + the RNG half of PORT-DEBT(actor-protagonist-clip); the only synthetic
remainder is the `0x467380` `cd`-init (seed-pinned 184, PORT-DEBT(ambient-event-cd)).

**Ported (ckpt 98, `src/butterfly.{c,h}`):** `butterfly_step` runs the EFFECT-band draw model once per
sim-tick (in `game_actor_update`, BEFORE the emitters), with each butterfly's `0xc874` captured by
`actor_spawn_effect_from_map` from the spawn replay; `game_actor_update` consumes the 3 tick-0 init
draws.  **Validated:** the offline LCG replay (the 4 freqs 653/686/735/698, state after spawn =
`0x9c2b551d`) reproduces retail's `0x46cd70` onEnter rng for **all 34 ticks 0-33** with zero
mismatches; host test `butterfly_pertick` locks the gate / flit-timer / count model.  **LIVE-CONFIRMED:**
the running port's per-tick LCG state (a `0x46cd70` debug read) matches retail tick-for-tick —
`0x9c2b551d, 0xb92fc6fa, 0x5c22a348, 0x9bf8e1ee, 0x1027c41c, 0x22322222, 0x056084f8, 0x7bc49e1e,
0xd2b528cc, 0xa60bc952, 0xede48fe0, 0x1886fdc6` for ticks 0-11, exactly retail's `runs/rng-census-repin`.

### #96 — the "Town of Tonkiness" area-title banner is `FUN_00494a60` (a 3-slot card renderer), NOT the `0x5a00c0` overlay player; mode-1 = a scroll sprite (res `0x449`) with the area name GDI-composed onto it and faded in (2026-06-09, ckpt 100)

The area-title card the player sees on entering an area.  Long mis-attributed to the `0x5a00c0`
"scripted overlay player" (which is actually the scrolling story-TEXT / dialogue runner — the
3-state `GetTickCount` loop + the 0x124-stride caption array; the ckpt-82 blit trace already found
"no `0x5a00c0` banner blit at the hold" — correct, the card is a different producer, shown later).

- **Renderer `FUN_00494a60` (918 B)** — called **3× per frame** from the render driver
  `FUN_0048c150:176-178`, ECX = `*(view+0x11c)`/`*(view+0x120)`/`*(view+0x124)` (3 banner slots;
  view = `*(room_state+0x104c)`).  Drawn **right after** the scene-fade reveal grid `FUN_0048e920`.
  Only **slot0** (`view+0x11c`) is the area card; the other 2 stay `enable=0`.
- **Object:** `[0]`=mode (1=GDI-text card; 2/3/4=sprite-pool banner, unused here), `[1]`={phase
  u16, hold_ctr u16}, `[2]`=alpha 0..1000, `[3]`=text ptr, `[4]`=hold (400), `[5/6]`=dst, `[8]`=enable.
- **Animation `FUN_00499ab0`** (per-sim-tick = every 2 flips): phase 0 compose → 1 fade-in
  (`alpha+=0x14`→1000) → 2 hold (`hold_ctr→400`) → 3 fade-out (`alpha-=0x14`→`enable=0`).
- **Render (case 1):** scroll cel = `FUN_00418470(0)` = **res `0x449`** (314×108, lazy-decoded @flip
  1511); the area name is **GDI-composed onto the cel ONCE** (cached via `DAT_008a7714`) — `GetDC`
  (`0x5b94e0`) → `FUN_0048e860` text → `ReleaseDC` (`0x5b9500`) — then blit at hard-coded **(160,64)**:
  `alpha<1000` → alpha blit (`0x5bd550`, ramp `(&DAT_008a9308)[alpha·0x14/1000]`), `=1000` → keyed
  (`0x5b9b70`).
- **GDI text:** font `DAT_008a9274[len-keyed idx]` (len 17 "Town of Tonkiness" → idx 6, advance 10);
  LOGFONT **Courier New, h20 w10 weight 400, italic 0, charset 1**; shadow `0x404040` drawn 12× (x
  ±2px / y 13-15) + white `0xffffff` 2× — the outline IS the multi-offset shadow, not a bold weight;
  centred `x = 160 − (len·adv)/2`.  The 2nd font pass (`FUN_0048e6d0`/font 7) is the furigana pass,
  no-op for English.  (The "Courier New" GDI textout seen in a probe is THIS, not a debug overlay.)
- **Timing** (seed-pinned, game_enter@~1434): arms ~flip **1513** (`game_enter+78`), alpha 1000 by
  ~1614, holds (hold_ctr→400) to ~flip **2422**, then fades.  Up the whole intro.
- Ground truth: `runs/banner-probe` / `runs/banner-state` / `runs/banner-blits`;
  `tools/flow/banner_fields.json`.  Full writeup: `findings/in-game-intro.md` "The area-title BANNER".

### #97 — the in-game DIALOGUE BUBBLE is one widget node + satellite cells: a 9-slice POP-IN (scale `+0x54`, content gated until 1000), a speaker-anchored TAIL pair, a portrait cross-fade that snaps OPAQUE at fade 500, and a 3-pass per-char typewriter (2026-06-10, ckpt 104)

The town-intro speech bubble (`0x439690` builder + `0x48c820` walk), live-verified bit-exact
against trace-studio `intro-1`:

- **Pop-in = the node scale mode** (`0x48c820` `+0x1c==1`): the 9-slice frame (`0x48cf80`, box
  bank `DAT_008a7708` = res `0x456`, 32×32 cells, frames 0-8) is drawn at `w·scale/1000 ×
  h·scale/1000`, CENTERED in the full rect (integer division); `+0x54` steps **+50 per widget
  update** (one update ≈ 2 flips = the sim-tick cadence) → 0→1000 in 20 updates ≈ 40 flips.
  ALL content cells are gated on `+0x54 < 1000 && +0x58 == 0` — the box pops in EMPTY, then the
  name/tab/portrait/text appear on the next update.
- **Geometry** (`0x439690:395-482`, the portrait layout `param_1+0x84 != 0`): box W=0x198 (408)
  H=0x70 (112); body text cell at box+(0x88,0x14) = (136,20), **36 chars × 3 rows**, row pitch
  `+0x1ac` = 28, per-glyph advance 7; name cell at (long-name: W−0xc0+5, −9) with colors
  **white main + `0x455f7b` shadow** (`:464-465`); name TAB cel = bank `DAT_008a7710` (res
  `0x44a`) frame **0 for names > 12 chars / 1 for short** at (W−0xc0, −0x20); TWO portrait
  cells (cross-fade pair) at (−0x18,−0x48).  Box position from the SPEAKER via `0x49c640`:
  x = clamp(speaker_center − W/2, 0x20, 0x260−W), y ≈ speaker_top − H − 0x30 (town line 1 →
  (174,148), Father center 378).
- **The bubble TAIL** (`0x49c640:70-115`): TWO cels from the BOX bank — frame **9** (the notch
  over the border) at (tail_x, H−0x20) and frame **10** (the spike below) at (tail_x, H),
  where `tail_x = clamp(speaker_center_boxrel, 0x20, W−0x20) − 0x10`; frames **11/12** =
  the flipped-below variants, **13/14** = the twin-layout variants.  The pair hangs at the
  box BOTTOM at the speaker's x (mostly behind the portrait bust on line 1).
- **Portrait cross-fade** (`0x49c910`, gated on box scale==1000): fade `[0x21]` += `[0x22]`=50
  per update; the NEW cel blends via `ramp_b[(fade·0x14)/500]` while the index ≤ 0x13, and the
  moment it exceeds 0x13 (fade ≥ 500) the desc is cleared to 0 = the **plain keyed blit — the
  incoming portrait is FULLY OPAQUE from fade 500**; the second half only fades the OLD cel out
  (`ramp_b[((1000−fade)·0x14)/500]`).  A hold-at-19 model measurably lags retail.
- **Typewriter** (`0x439690:499-514` config; the body renderer `0x48da70`): interval =
  `*(*DAT_008a6e80+0x248)` widget updates per char (measured 5 ≈ 10 flips), grade slots
  `2i/3i/5i`; VOICED lines override to `6/0x12/0x18/0x24`.  Reveal cadence fitted from the
  line-1 trace: space ≈ 1 update, `,` ≈ 3i, a row close adds ≈ +i.  Each revealed row draws
  **3 full GDI passes** (`0x48da70`: shadow (x,y+1) + shadow (x+1,y) in `+0x184`, then main
  (x,y) in `+0x180`), per-char TextOutA at `col·7 + cell.x + node.x`; body colors `0x3e537d`
  main / `0xa8b9cc` shadow, font Courier New **7×18** charset 1 (live LOGFONT — not the 7×16
  menu slot).
- **The advance ARROW** (`0x410560` config on the text cell): bank from the widget manager
  god+`0xb8c` (module unresolved — the probe's res_id 1000 collides with sotesd's parallax
  mountain sheet, see #92/#98), frame base `+0x2c`=0x14 + anim table {0,1,2,3} (`+0x2e..`),
  one step per `+0x70`=10 updates, pos = (boxW−cell.x)−0x20−8, (boxH−cell.y)−0x20+0xc →
  (542,240); the 1px bob is baked in the per-frame cel placement metrics.  **Hidden while the
  typewriter runs** (`0x48d940`'s `+0x174[0]==1` early-out; confirmed: no arrow pixels at
  flips 2800/3100 mid-typing) — it shows only when the finished line waits for Z.
- **The UI sheets decode UNGRADED**: the bubble/tab cels resolve through the plain getter
  family (`0x4184a0`/`0x418470`), skipping the in-game `0x417c40` palette grade — same as the
  banner scroll (#96).  Proven by exact-pixel matches of the raw palettes against live frames.

### #98 — the 24bpp resource blobs (dialogue portraits, parallax planes) carry a plain 24bpp BMP whose "palette" slot is UNINITIALIZED packer memory (XP-era heap droppings), and land on the 16bpp 565 surface — the on-screen pixels are the sheet through one RGB565 quantize+bit-replicate round trip (2026-06-10, ckpt 104)

- The standard sprite container (32B magic + 1024B palette + 64B + BMFH + pixels) holds, for
  24bpp resources, a **BM with `data_off 0x36` and no palette** — the 1024-byte palette slot
  contains build-machine memory (pointer-rich `0x7c95xxxx` XP DLL addresses in sotesd's blobs).
  Pixels start at `+0x458 + pixel_off` (the self-rebasing header, `FUN_005b7c10`); the Father
  portrait res `0x7ef` = exactly 160×176×3 BGR bottom-up, magenta `0xff00ff` key.
- **The screen math:** the engine's surfaces are 16bpp RGB565; a 24bpp sheet quantizes at
  upload and the readback expands by bit replication — `R8=(r5<<3)|(r5>>2)`,
  `G8=(g6<<2)|(g6>>4)`.  The live retail bust matches the raw decoded sheet **exactly** through
  that round trip alone (no grade, no blend) at 1:1 scale, position (150,76).
- **Numeric-collision warning (extends #92):** res id 1000 (`0x3e8`) in sotesd.dll is a 640×352
  24bpp PARALLAX MOUNTAIN plane (registered twice: pool slot 65 as 80×352 columns), while the
  dialogue arrow bank that a probe reported as "res 1000" lives on another module/slot —
  always record the slot's `settings` HMODULE alongside the id.

### #99 — the town-arrival script schedule on the SIM-TICK axis (easer-call counts from game_enter): pan command at tick 92 (camera first moves t93), banner first alpha step t42, dialogue bubble first visible change t645; presents run ~2 flips/tick with single-tick coalesces (2026-06-10, ckpt 105)

- **The deterministic comparison axis is the easer-call count** (`0x43d1d0` onEnter
  count = the capture `_t` stamp), NOT the Flip index: under `--lockstep` retail
  still presents each tick a VARIABLE number of times (mostly 2 flips/tick, with
  occasional 1-flip ticks and 3-flip stretches — e.g. intro-1 ticks 41 and 491 were
  never presented at all).  Flip-axis trigger calibrations silently absorb these
  coalesces as ±1-tick errors; two independent earlier reads of the same triggers
  disagreed by 1-2 ticks for exactly this reason.
- **The town-arrival schedule (intro-1 nav, ticks counted from the first in-game
  easer call):** the camera-pan command lands on the controller at **tick 92** (the
  eased camera first MOVES on the t93 present — the easer's first step with a fresh
  target produces no displacement); the area-banner's first composed alpha step
  (value `0x14`) renders at **tick 42**; the dialogue bubble's first visible change
  (pop-in onset) renders at **tick 645**.  The dialogue pop/portrait-fade/typewriter
  change SEQUENCE from t645 is pixel-identical to the port's machine run 1:1 —
  retail's whole bubble timeline is rigid once the start tick matches.
- **Fade-phase probes through the alpha ramps PLATEAU:** the banner blit's ramp
  index is `alpha*20/1000` (one index per 50 alpha = 2.5 ticks at ±0x14/tick), so
  tick-shift probes that diff pixels at `dt` offsets return differ_px==0 for 2-3
  consecutive dt — a 2-tick offset can read as 1.  Calibrate fade phases off the
  per-present VALUE sequence (each distinct level's first tick), not a dt scan.
- **The typewriter's steady cadence is 5 ticks/char on BOTH sides** (confirming the
  ckpt-104 fit), but the ROW-TRANSITION pause structure differs from the port's
  fitted grades: across the row-1→row-2 boundary retail paused {5, 14, 5} ticks
  between changes where the fitted model produced {1, 5, 16} (net −3) — the real
  char→grade map is still unread (PORT-DEBT dialogue-pause-grades).

### #100 — the fade-grid's render cels decode through the PLAIN getter (UNGRADED), retail's tick-stamped present shows the post-update grid, and the grid object carries an overlay AUDIO-fade level — three reveal facts that closed R6 (2026-06-10, ckpt 106)

- **res `0x458` (the 32-frame alpha mask ramp) + res `0x583` (the opaque black
  cel) bind through the plain getter `FUN_004184a0(0)`** (`0x48e920:37/66`
  lazy-bind) — NO `0x417c40` palette grade, the same ungraded family as quirk
  #96 (banner scroll / dialogue bubble / name tab).  The mask sheet is a pure
  linear gray ramp: 32 vertical 64×4 cells, storage bottom-up white→black, so
  visual cell v has exactly `gray5 == v` after the 565 round trip; the
  composite `out5 = dst5 − mask5` (group-E weight-1000 mode-2 LUT is exact:
  `lut[s][d] = clamp(d−s)`).  A graded decode reads one 5-bit step weak across
  most of the ramp — the whole R6 "level map mismatch" was this plus a
  one-tick stamp offset stacked.
- **Stamp registration: a retail frame tick-stamped u presents the
  POST-update-u cinematic state.**  Proven by mask-level extraction at forced
  stamp equality (effective `s5(a) == a` exactly, 11 indexes, both 5-bit
  wobble points included).  The port must run its cinematic step BEFORE
  rendering on the same tick the stamp increments — an extra "arm latency"
  fence reads as exactly one aging step of frontier lag.
- **The fade-grid object carries a second, AUDIO ramp at +0x20/+0x24/+0x28**
  ([8] mode: 1=zero, 2=ramp to 1000, 3=ramp to 0; [9] speed; [10] level
  0..1000; updated at `0x499ab0:104-124` BEFORE the iris block at
  `rate = speed*20/1000` per tick; armed by the beat-runner `+0x30` request,
  `0x439690:585+`).  Town entry runs mode 2 speed 500 → level += 10/tick, 0 →
  1000 over the first 100 ticks.  Consumed by ~12 SOUND-position updaters
  (`0x46d180`/`0x46d4a0`/`0x46e510:1771`/`0x554570`/`0x557060`/`0x54d090`/
  `0x489280`/`0x407ba0`/`0x40a010`/`0x40ac90`/`0x40bb90`) as
  `vol += (level−1000)*3000/1000` clamped to [−10000, 0] — fed to
  `FUN_005bb870/80/90`, which are vtable thunks into DirectSound-land
  (live-resolved targets in a loaded sound module; param ranges = DSound
  volume hundredths-dB / pan / `(p+10000)*22050/10000` frequency).  It is the
  town's ambient-audio fade-in, NOT a video term — pixels never read [10].
  (Field spec: the `0x499ab0` `ovl_*`/`r40..r80` chain fields dump the whole
  object; `runs/r6-grid` is the seed-pinned reveal-window ground truth.)
