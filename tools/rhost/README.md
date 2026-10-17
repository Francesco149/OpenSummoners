# tools/rhost — in-process retail host (Fortune Summoners)

Runs the user's own Steamless-unpacked `sotes.exe` **inside our process**, deterministically, so
the retail side of every comparison is a reproducible function of (edition, save, input trace).
Ported from openrecet's `tools/rhost` (its 2026-10-13 methodology reset) to SotES: MSVC6,
DirectDraw7, DirectInput7, DirectSound + DirectShow BGM, single-threaded, GetTickCount-paced.
Findings + measurements: `docs/findings/in-process-retail-host.md`.

## Quick use (from maximus; runs on the Windows box)

```sh
python3 tools/rhost/rhost.py setup                     # one-time: build, stage, runner task, game copies
python3 tools/rhost/rhost.py boot --frames 1500 --shot-at 300,1499     # smoke: boot → title
python3 tools/rhost/rhost.py run rh-intro-errands --shot-every 150      # replay a scenario
python3 tools/rhost/rhost.py det rh-intro-errands -n 3                  # determinism gate
python3 tools/rhost/rhost.py corpus                                     # every rh-* scenario
python3 tools/rhost/rhost.py verify                                     # every scenario × every shadow hook
python3 tools/rhost/rhost.py coverage                                   # first-hit coverage of all IDA functions
python3 tools/rhost/rhost.py record my-scenario [--save USERDIR|TGZ]    # HUMAN PLAY → tests/scenarios/<name>/
python3 tools/rhost/rhost.py boot --edition ense-en                     # the EN Steam SE build
python3 tools/rhost/rhost.py run X --extra="--watch-write 2005cb90@0"   # who writes this address?
python3 tools/rhost/rhost.py run X --extra="--trap 4181df"              # registers at an instruction
#   (+ `trap-count VA hits=N` per trap at exit; --trap-max 0 = counts only: path coverage)
python3 tools/rhost/rhost.py run X --extra="--trap-census 46e510:1d4"   # histogram of [ecx+0x1d4] at each hit
#   (which particle codes reach 0x46e510; one VA:OFF per flag, shares the 8 trap slots)
```
Results land in `/mnt/ephemeral/opensummoners/rhost-runs/<batch>/<run>/` (`rhost.log`, last line
`RESULT key=value…`; shots `shot_fNNNNNN.png`; dumps `state_*.rhs`). The driver rebuilds +
restages automatically when `tools/rhost/*.c|h|sh` or the linked port sources change.
Runs fan out on the Windows box (`-j 8` default).

Speed (Win11 native, single run): boot→title ~1000 fps raw; full intro (4320 flips, in-game) ~830
fps with the system-stack layer; 3 parallel runs ≈ 340 fps each (contention: DirectDraw).

## Scenarios — `tests/scenarios/<name>/trace.jsonl`

```
# comments
{"meta": {"edition": "enold"}}              optional; also "savefile": "<rel path to a user\ dir or .tgz>"
{"frame": 620, "keys": [28]}                from flip-frame 620 on, exactly DIK 0x1c (Enter) is held
{"frame": 624, "keys": []}                  …released
```
A LEVEL format (sticky until the next line) of DirectInput scancodes, keyed on the flip count.
Default keymap (retail producer 0x46a880): arrows 0xc8/0xd0/0xcb/0xcd → ring ids 1/3/2/4,
**Enter 0x1c → confirm (ids 20 + 0x24)**, Esc 0x01 / Backspace 0x0e → 39, F1..F6, digits; the
action keys come from config.dat (+0x548…). `rhost.py record` writes the same format.
Saves are the game's `user\` dir (savedataNN.dat…): user data, gitignored (`_saves/`).

## What makes a run deterministic

| source of run-to-run difference | rhost layer |
|---|---|
| wall clock (GetTickCount — the only clock sotes reads; GetLocalTime/GetSystemTime/TZ for logs + `srand(time())`) | virtual: +16 ms per Flip, frozen between Flips; creeps 1 ms/call after 2000 calls without a Flip (busy-wait breaker, a pure function of the call sequence) ⇒ every 16 ms pace machine runs 1 update/present |
| the pump's 10 ms `SetTimer` (WM_TIMER clears the wait flag state+0x1c / SE +0x20) | fake SetTimer; `WaitMessage` clears the flag and returns |
| OS messages to the WndProc (activation pauses music + re-acquires input; paint; keys) | WndProc filter drops activation/timer/paint/input; one synthetic WM_ACTIVATEAPP(1) when the game window is shown |
| the launcher dialog (DialogBoxParamA 0x2711) | not shown: globals set as a Windowed+Launch click (exact WM_INITDIALOG→Launch round trip) |
| `srand(time(0))` in WinMain | pinned: `srand(--seed)`, default 0x4f5347 = the port's OSS_RNG_DEFAULT_SEED |
| heap placement (NT heap/LFH; Global/LocalAlloc) | fixed-address arena 0x20000000.. (rh_heap.c) |
| kernel handle numbers (CRT `__pioinfo`, mutex) | sequence-numbered fake handles (rh_handles.c) |
| GDI handle numbers kept in the heap (fonts/brushes/pens) | fake GDI handles (rh_gdi.c): slots reused LIFO like GDI's table, generation in the high byte (stale handle ⇒ invalid); retail does GetDC/ReleaseDC every flip. Periodic log: `kh=` kernel / `gdi=issued/live/hw` |
| stack placement + **system-DLL stack garbage read by retail** (uninitialized palette byte 3 at 0x586010 / 0x417c40) | fixed stack 0x58800000..0x59000000; every external call (IAT + DirectDraw7/DirectShow/fake DSound COM) runs on a separate system stack 0x58010000..0x58700000 (rh_scrub.c) |
| command line / environment / startup info / std handles | fixed values |
| temp file names (BGM .wma extraction), user's saves/config | `GetTempPathA` = `<game>\tmp\` (run-independent string) + deterministic names; overlay: game dir read-only, writes copy-on-write into the run dir, `user\` + `temp\` whited-out unless the run dir provides them |
| DirectInput devices / focus | fake DirectInput7: keyboard = the trace (GetDeviceState level + GetDeviceData edges stamped with virtual ms), no joysticks |
| BGM playback position (the music manager loops the track off IMediaSeeking::GetPositions) | real FilterGraph (exact durations), but Run/Pause/Stop/SetPositions/GetPositions patched onto a virtual playback clock; silent unless `--audible`/`--record` |
| DirectSound | silent fake with a fixed-address buffer pool unless `--audible`/`--record` |
| single-instance mutex (would collide with parallel runs / a real game) | unnamed |
| resources of "the exe" (`FindResourceA(NULL, …)` — the map loader 0x5b62a0) | NULL module → the mapped image 0x400000 |

Residual run-to-run differences: only slots holding system-DLL/COM pointers + the HWND, listed in
`volatile.<edition>.txt` (calibrated by `rhost.py det SCEN --calibrate`, masked in `state_sem` by
ADDRESS; `state_full` hashes every byte). `det` fails on any other class of difference.
**Record ≡ replay:** `--record` feeds the game through the same fake keyboard (sampled once per
flip), so the recorded trace replays to the identical semantic hash (verified 2026-10-14).
**Stable across rhost rebuilds:** everything retail may point at that rhost owns lives at a fixed VA
(fake COM objects, command line / environment buffers: `rh_fixed_alloc`, 0x5f080000), and
`SetUnhandledExceptionFilter` hands retail NULL as the previous filter (the CRT keeps it at
0x8a9ef8). Verified: a deliberate host layout shift leaves every state hash unchanged. (Per-boot
ASLR of ddraw/dinput/dsound/ole32/quartz does not move the hash either: `--perturb-dlls`.)

**The render gate (`--fb-hash`, 2026-10-16):** at each Flip entry the composed back buffer (visible width,
pitch padding left out) is hashed and chained: RESULT `fb=` / `fb_frames=`, `fb=` on the `--hash-every`
lines. `corpus --bless/--check` always pass it (expected `fb`; wine also `fb_series`): pure ≡ hybrid in pixels
on every presented frame. Deterministic per platform (wine ≠ Windows pixels, each stable run to run).

**semv's external map is an envelope:** a gap of up to 4 MB between non-deterministic mappings counts as
external too. Wine places a 2 MB system block in whichever gap is free when its thread asks (two layouts
across runs), and a plain integer (a byte tuple like 0x09090908) then fell inside the map in one run only:
rh-combat's wine series flaked f13200..f51000 (pure runs too). Re-blessed the wine series with the envelope.
The whole system zone between rhost's image and its arena (0x05c00000..0x20000000, minus rhost's own pages)
counts as external too: wine's heaps grow there to a high-water mark that varies by thread timing (2026-10-17:
0x0ac8.. vs 0x0bc5.. in two identical hybrids).

**Probes:** `--trap VA` lines carry 11 stack words (`args`, after the return address); `--trap-frames A-B` logs
only inside a frame window; `--heap-trace A-B` logs every HeapAlloc / HeapFree there (size, block, ebp chain:
diff pure vs hybrid sequences for an allocation-order divergence); every access violation is logged (`AV fN at VA (read/write addr)`, registers, the
ebp chain — symbolize port frames with `nm -n build/rhost/rhost.exe`); a foreign `HeapFree` logs its callers.

**REPLACE reaches port→port calls too (2026-10-17):** a hook redirects retail's `call` sites to its wrapper; a
ported caller reaches its callee through an engine-call slot (`g_eng.hXXXXXX` = retail's VA) or decomp2c's `RC_`
(`g_eng.retail(va)`) instead — before, those ran retail's even when the callee was replaced (the ported room
loop's render 0x48c150 never ran as port code).  After installing the hooks rhost retargets every slot holding a
replaced VA to the hook's wrapper (retail's convention) and `g_eng.retail` returns it — except a port's
deliberate delegation to retail's own version (the integrator's unported special poses: `integrate_442a70`).

## Hooks — port functions vs retail on identical state (`--hook shadow:NAME[@N],replace:NAME`)

`rh_hooks.c` registry; port code is linked in, compiled with the port's own `-std`. RET hooks
compare return + listed state (`rand`: the seed word); FIELD hooks compare the exact fields a
function may write (`input_poll`: every ring record id). `replace:` runs the port INSTEAD of retail
(hybrid mode). A port function never calls its own retail VA. Add one = a wrapper + a table row,
then add it to `VERIFY_HOOKS` in rhost.py.

**Shadow guard:** a full-state shadow runs retail, restores the pre-state, then runs the port —
anything EXTERNAL the call does (outside the arena snapshot) would happen twice. The engine-call slots
with such effects are bound to guard stubs (`gd_*` in rh_hooks.c: the surface free 0x5b9390, the
DirectSound buffer release 0x5bb3a0, DeleteObject, the DirectInput calls, the input re-poll 0x40fe00,
and the script engine 0x40c380 for modes other than 3/4 — room-start / touch scripts drive DirectShow /
DirectDraw): in the PORT phase they skip the call and `rh_shadow_abort()`; the shadow keeps retail's
result and counts the call `skipped`. Those calls are proven by `replace:` + `corpus --check`. Check
that a set of shadows does not perturb the run with `corpus --check --hook shadow:…`.

**Caller-stack objects (2026-10-16):** the arena snapshot does not cover the stack, so a function writing
through a pointer into its CALLER's frame (the body copies the integrator hands its per-type handlers, a
menu context, a text buffer) used to leak the retail phase's writes into the port phase (the port then
ran on retail's result — found by `slash_step` 0x461240: ret 1/0 with "equal" state).  The GX hooks
(TC_HOOK / ST_HOOK / TC_HOOK_R) now go through `full_shadow_gx`: every argument (or `this`) pointing into
the fixed stack above the wrapper's frame gets a 0x300-byte window saved before the retail phase, put
back for the port phase, compared after it ("the caller-stack object differs at …"), and left as retail
had it.  Custom wrappers with stack objects do the same by hand (`key_token_replace`).

**Hybrid proof (2026-10-14):** with `rand` + `input_poll` either SHADOWED or REPLACED, all 6 scenarios
(EN-old + EN-SE) end byte-identical (semantic) to pure retail (`corpus --check --hook …` PASS); verify:
rand 69,420 calls / 0 diverge, input_poll 42,125 / 0. This needs retail's uninitialized stack reads
(quirk #123 class) neutralized, otherwise any port code on the retail stack changes what they copy
into the heap: `uninit-fix` (rhost.c `UNINIT[]`, per edition) zeroes byte 3 at every call of the
palette copy (EN-old 0x491770 / SE 0x48c300, 15–16 callers), the inline palette before the 1 KB copy
(0x4181df / 0x41233f) and the move-command pad (0x440dd1 / 0x43bccc); site D (quirk #130) zeroes every
heap block on allocation (MSVC6 `___sbh_alloc_block` 0x5c73fd / 0x5f7afa + the rh_heap arena) — reused
blocks otherwise leak stale DirectDraw pointers into partially-initialized nodes. `--no-uninit-fix` = raw retail.
**Windows binary = `oss-rhost.exe`** (openrecet's restage kills processes named `rhost` by name).
Census recipe for the next site: `run X --dump-at end` pure vs hooked → `rhs.py A B` →
`--watch-write VA@0` on a differing address (every value change + writer EIP) → read it in all_ida.asm.

## Editions (`--edition`, or `{"meta":{"edition":…}}` in the trace)

`enold` (EN Steam 2012, the port's target) and `ense-en` (EN Steam SE build 24635888,
2026-08-10). Each is a row in `EDITIONS` (rhost.c): the engine VAs the host touches. SE values come
from `tools/ida/crosswalk.py`. Game data: `C:\oss-rhost\game\{enold,ense}\` (pristine copies,
`win/stage-game.ps1`).

## Windows plumbing (separate from openrecet — both can run at once)

- `ssh win` = Windows OpenSSH (PowerShell). sshd runs in session 0 (no desktop ⇒ no
  DirectDraw), so batches go through **`win/runner.ps1`**, a FIFO queue runner in the interactive
  logon session: scheduled task **`oss-rhost-runner`**, queue **`C:\oss-rhost\queue`**.
  openrecet uses `orc-rhost-runner` + `C:\orc-rhost\queue`: no shared files, names, mutexes or
  windows; each rhost process is hermetic (unnamed mutex, off-screen non-activating window).
- `C:\oss-rhost\{bin,game,runs,queue}`.

## Wine target (maximus, headless) — `--target wine` (or `RHOST_TARGET=wine`)

Same batches, run locally in parallel under `xvfb-run` (16-bit screen) + wine 11: a prefix whose
`drive_c/oss-rhost` → `/mnt/ephemeral/opensummoners/wine/root` (bin/, runs/, game/<ed> → local game
data: `game/ense` = links into the Linux-Steam SE install + the unpacked exes; `game/enold` needs a
copy of `C:\oss-rhost\game\enold`). **A valid LOGIC oracle:** frame/tick/RNG identical to Windows
(SE boot f900 rng 9d661dcf; rh-se-newgame rng 7ba076a4 / tick 1663 both), deterministic run-to-run;
state hashes differ from Windows only environmentally, so wine keeps its own `expected.wine.json`.
Pixels + speed are Windows truth (wined3d on llvmpipe: ~190–260 fps). One Xvfb per batch is shared by
all workers (one wineserver per prefix; per-run displays die with BadWindow). Known flake: under
6-way parallel load one rh-errands-walk wine run in ~5 ended with a different state hash (frame/tick/
rng equal) — not reproduced by `det --target wine`. Use it when the Windows box is unavailable or
for wide parallel sweeps.

## Not yet

port-side DIK input path + determinism knobs (port↔retail by frame number), the in-game cadence
check (port pins 33 ms/sim tick, rhost runs 16 ms/flip), JP-SE profile, D3D-free `.osr` capture
from rhost (load `capture_proxy` in-process), joystick recording, EN-old game data on maximus for wine.
