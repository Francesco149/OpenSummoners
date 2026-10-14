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
| GDI handle numbers kept in the heap (fonts/brushes/pens) | sequence-numbered fake GDI handles (rh_gdi.c) |
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

## Hooks — port functions vs retail on identical state (`--hook shadow:NAME[@N],replace:NAME`)

`rh_hooks.c` registry; port code is linked in, compiled with the port's own `-std`. RET hooks
compare return + listed state (`rand`: the seed word); FIELD hooks compare the exact fields a
function may write (`input_poll`: every ring record id). `replace:` runs the port INSTEAD of retail
(hybrid mode). A port function never calls its own retail VA. Add one = a wrapper + a table row,
then add it to `VERIFY_HOOKS` in rhost.py.

**Hybrid proof (2026-10-14):** `corpus --check --hook replace:rand,replace:input_poll` → all 5
scenarios end byte-identical (semantic) to pure retail. Shadow runs compare per call (0 divergences)
but their END state may drift from pure retail: the wrapper + port code run on the retail stack and
retail has uninitialized stack reads (quirk #123 class) that copy such garbage into the heap.
Known sites are neutralized (`uninit-fix`, rhost.c: palette byte 3 at 0x586f79 + 0x4181df, move
command pad at 0x440dd1; `--no-uninit-fix` = raw retail); a third palette copy remains (heap
0x20bc835c… in rh-intro-errands under shadow). Next step: a transparent shadow hook (return-address
hijack: retail runs at its exact stack position, the port runs on the system stack afterwards).
Census recipe: `run X --dump-at end` pure vs hooked → `rhs.py A B` → `--watch-write VA@0` on a
differing address (logs every value change + writer EIP) → read the writer in all_ida.asm.

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

## Not yet

port-side DIK input path + determinism knobs (port↔retail by frame number), the in-game cadence
check (port pins 33 ms/sim tick, rhost runs 16 ms/flip), JP-SE profile, D3D-free `.osr` capture
from rhost (load `capture_proxy` in-process), joystick recording, wine target.
