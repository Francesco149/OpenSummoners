# In-process retail host (rhost) for SotES — findings + measurements

2026-10-14 (overnight). Tool: `tools/rhost/` (README = usage + the determinism table). Ported
from openrecet's `tools/rhost` (its `docs/findings/in-process-retail-host.md` / 2026-10-13
methodology reset). Decision record: `docs/audits/2026-10-14-rhost-methodology.md`.
All numbers: Win11 native (cutestation, 24 threads), EN-old unless noted.

## 1. Mapping retail into a host process

- `sotes.unpacked.exe` (EN-old, sha256 9e032483b998): ImageBase 0x400000, SizeOfImage 0x3d0f000,
  relocs stripped, MSVC6 linker (6.0), DllCharacteristics 0. `.text` 0x401000+0x1ca795, `.data`
  0x854000..0x8ab2e0 (+bss), `.rsrc` 0x8ac000+0x3862588 (59 MB of game data — quirk #124).
  EN-SE (36a0294df405): SizeOfImage 0x4502000, `.data` 0x8be000..0x92f7a0.
- Imports: KERNEL32 USER32 GDI32 WINMM(mmio only) VERSION ole32 DDRAW(DirectDrawCreateEx)
  DINPUT(DirectInputCreateEx) **DSOUND by ordinal 1** (DirectSoundCreate). **No CreateThread, no
  timeGetTime/QPC** — single-threaded, GetTickCount-paced. 152 imports, all resolved.
- Host = mingw32 exe linked at 0x200000, `--disable-dynamicbase`, with a 0x4c00000-byte `.bss`
  hole covering 0x400000..0x4e21000 (both editions). Sections copied in, imports resolved, the CRT
  entry (`0x5c0a8f` / SE `0x5f0d7b`) called on a fixed stack. Same recipe as openrecet.
- **`FindResourceA(NULL, …)` resolves to the HOST exe**: the map loader passes NULL (`DAT_008a6e7c`
  = 0) → "The map data is not found." on the prologue→town load until NULL-module resource calls
  were pointed at 0x400000.

## 2. Determinism — what had to be pinned, how it was found

| finding | how found | fix |
|---|---|---|
| boot parks at frame 47 / pump spins | `WaitMessage while inactive` | WM_ACTIVATEAPP goes to the GAME window (shown via `SetWindowPos(SWP_SHOWWINDOW)`, quirk #125), not the first (WAIT splash) window |
| GDI handle values in the heap (fonts/brushes/pens: type byte 0x0a/0x10/0x30) | det gate at frame 1: 96 dwords in the CRT SBH region | rh_gdi.c fake handle numbering (+ translate WNDCLASS.hbrBackground); SLOTS reused LIFO + a generation high byte (2026-10-15: retail GetDC/ReleaseDCs every flip ⇒ the never-reused sequence capped at 16384 died at flip ~16.3k) |
| HWND slot 0x8a93ec | det gate (`data` region) | masked (volatile) |
| byte 3 of 256 palette dwords (0x586010, 0x417c40) | det gate: 87+22 heap dwords differing in byte 3 only; `--watch-write` → writer 0x49179d / 0x418216; disasm → stack palette, 3 of 4 bytes written (quirk #123) | external calls run on a separate system stack (rh_scrub.c) |
| BGM positions (music loop) | RE of 0x5bb9b0 (quirk #127) | DirectShow patched onto a virtual playback clock |
| SE: pump flag +0x20 | SE boot spun at frame 62 | edition profile field |

Stack-garbage design notes (the expensive one):
- Zeroing 32 KB below ESP after every external call: 87 → 22 diffs (not enough: quartz
  RenderFile at the map load dirties ~100 KB; the palette sits ~64 KB deep).
- Adaptive page-walk zeroing (zero down until two clean pages): PASS, but 676 fps (−35%).
- **System stack (final):** the IAT/COM thunk copies 16 argument dwords to a separate stack
  [0x58010000, 0x58700000) inside the same TEB bounds and calls there; `post` restores retail's
  ESP + the callee's pop count. PASS at 830 fps. Callbacks into retail during an external call
  (WndProc via DispatchMessage) run on the system stack.
- Watching a dead-stack address with a hardware watchpoint crashes (the exception dispatch itself
  writes there) — find the stack writer by reasoning/`--trap` instead.

Results (3 runs each, semantic hash = all .data + arena dwords except `volatile.<ed>.txt`):
- `rh-boot-title` (1620 flips): PASS, 1 distinct hash.
- `rh-intro-errands` (4320 flips: title → new game → prologue → Tilelia arrival → dialogue → house
  → errands freeroam): PASS, 1 distinct hash (3316 pointer slots calibrated).
- EN-SE boot (1500 flips): PASS, 1 distinct hash (192 slots).
- **Record ≡ replay:** a `--record` run (visible window, realtime 59 fps, real keyboard sampled
  once per flip into the fake device) and the hidden turbo replay of its trace give the same hash.

## 3. Clock model = the cadence the port should match

Every scene loop is a 3-state GetTickCount pace machine (in-game: `0x439690:963-1070`): each render
pass adds the elapsed ms to a budget (cap 100); budget > 16 ⇒ one update, budget −= 16. rhost's
clock is +16 ms per Flip and frozen between Flips ⇒ **exactly 1 update per presented frame**
(measured: easer ticks ≈ flips in-game, 3054 ticks / ~3040 in-game flips). In a real run the
10 ms WM_TIMER and GetTickCount's ~15.6 ms resolution give ~1.6–2 flips per update, wall-clock
dependent (quirk #75) — which is why the port pins `now = sim_tick × 33 ms` for its freeroam replay.
**Open decision:** which deterministic cadence the parity contract uses (rhost 1:1 is the simplest and
matches the port's present loop; the port's 33 ms input clock does not match rhost's 16 ms).

## 4. Speed

| run | fps |
|---|---|
| boot→title, single, raw | ~1000 (600 flips incl. boot in 1.0 s) |
| full intro 4320 flips, single, no system stack | 1037 |
| … with page-walk zeroing | 676 |
| … with the system stack (default) | 830 |
| 3 runs in parallel | ≈337 each (DirectDraw contention) |
| `--record` (human play) | 59–60 realtime |

Whole corpus (3 scenarios, -j8): ~10 s wall including staging. The Frida retail drive took minutes
per re-drive and needed lockstep pins.

## 5. Oracle results (shadow hooks, `rhost.py verify`, 3 scenarios)

- `rand` (port `src/rng.c` vs `FUN_005bf505`): 22,966 calls, 0 divergences.
- `input_poll` (port `input_poll_consume` vs `FUN_0043c110`, compares return + every ring id):
  19,511 calls, 0 divergences.

## 6. Coverage (one-shot INT3 at every IDA function entry, `rhost.py coverage`)

The 3-scenario corpus hits 861/1824 functions; game (non-FLIRT) 729/1550 = **48.7% of game code
bytes**. Joined with the port ledger: of the 866 KB retail executes for boot→intro, port-touched
functions cover 235 KB (27%); 513 hit functions (630 KB) are not referenced by the port — the
largest are 15–28 KB scene/event routines (0x4b4d40, 0x41f200, 0x431e30, 0x4a6b80, …). The intro
renders 1:1 in the port while most of the retail code that produces it is unported
(cast tables etc. stand in) — the planning map for "full structural parity".

## 7. Open / not yet shown

- Port side: no DIK-level input path (the port injects ring ids + held axes), so port↔retail by
  frame number needs the producer `0x46a880` (or a faithful DIK→ring path) ported first.
- JP-SE profile; SE scenarios beyond boot; joystick recording.
- `.osr` draw-stream capture from inside rhost (load `tools/capture_proxy`'s engine hooks
  in-process) so the Trace Studio consumes deterministic retail streams.
- Heap objects outside the arena: none known (all retail allocators are routed).
- Wine/Linux target (untested for sotes; openrecet's works headless).
