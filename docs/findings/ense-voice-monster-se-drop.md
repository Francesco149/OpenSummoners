# EN-SE JP-voice patch — reported monster-sound silence (Mystery Dungeon) — NOT REPRODUCED

**Report:** with the `tools/ennse_voice` JP-voice patch: Ghost Warlock evasion+death, Black Harpy
damage/singing/death, Babymage damage/spellcast SILENT, "tested with monsters in Mystery Dungeon's
Abyssal Ruins, there are probably other monsters with the same issue."

## STATUS 2026-07-17: extensive live campaign — NO repro in 6 contexts. Dispatch chain proven healthy under the patch.

Instrumented retail EN-SE (probe `tools/ennse_voice/repro_probe.py` + repro_agent.js; side tracer
scratchpad `se_trace.py` hooking `bufferStart 0x5e3e10` + `soundPlay 0x4075a0` w/ caller VAs,
DirectSound `GetStatus`/`Play` verified). All PATCHED (voice bank live, gate 0):

| context | method | result |
|---|---|---|
| title synthetic | construct+play all 7 reported (keys 0xC789/0xC792/0xC829) | ALL play, DS hresult 0 (also stock — identical; codex 2026-07-16) |
| Arche's Arena (bonus menu) natural | force-spawn `0x419b00` arg6 → real fights, ~312 kills | attack a73 + Babymage cast a10 play; **real Ghost Warlock death a12 dispatched+played** (caller `0x4498d8`) |
| story field (Archmage's Tower save) | live Babymages | cast a10 plays |
| **MD floor 1, forced reported trio** | force-spawn 4×0xC78B/4×0xC794/4×0xC829, USER LISTENING (visible window, HP frozen) | **USER: "sounds are all playing correctly — harpy singing and screaming"** |
| MD synthetic (in-mode) | all 7 reported | ALL play, DS-verified |
| pool pressure | 72 retained synthetic sound sets (~300 buffers) | zero degradation |

USER also confirms EARLY MD mobs sound fine on the patch (own playthrough of MD dungeon 1), and
base-game monsters were always fine (whole-game playthrough on the patch).

## Key mechanics learned (durable)

- Monster damage/death/evasion sounds DO go through per-owner `soundPlay 0x4075a0`
  (death a12 caller `0x4498d8`; party damage a9 caller `0x4530e3`, evade a7 `0x459fa9`). Most arena
  trash has NO death-scream rows — 1 death dispatch in 300 kills is normal, not a bug signal.
- Sound-def table `0x65b0e8` (294×0x24): the reported trio = the TAIL rows (274-278, 282-284,
  286-292). Babymage rows have `bank_selector=-1` (multi-bank); ghost/harpy `0`; only 4 rows use
  bank 1; NONE reference the MD bank statically.
- Loader state machine `0x586a63..0x588xxx` (state entry `0x586af1`): sound-channel pool counts —
  `0x587f87` 6→8 if MD bank `0x92af7c`, `0x5880e8` 4→7 if voice bank `0x92af80` (+3 slots when
  patched). Still the best mechanistic lever for a scale/depth-dependent drop (tail rows lose
  first) — but 72-set spam did NOT exhaust anything.
- MD mode ("Arche's Mystery Dungeon ver2.0") = title → Start → Scenario Select (its prologue reuses
  the story opening w/ Challenge tutorial panels; runs on the MAIN-GAME input loop — `0x5b66bd` is
  NOT MD-exclusive).
- Title drive (headless): title routine classic `0x584ac0` / SE-variant `0x582c40` chosen by
  `(0x92af7c==0 || opt+0x158!=0)` in dispatcher `FUN_00581ba0`; menu polls ONLY {2,4,34,37}
  (4=rotate+1, 37=confirm, 34=abort; first accepted press = wake, eats the rotate). Dispatcher
  switch on title return: 0x1a=Start (→`FUN_005a4a40` scenario select when MD bank present),
  0x1c=Continue (`0x585cf0`), 0x1d=Special/bonus (`0x58b1f0`), 0x1e=Option, 6/8/9=exit. Story
  demos poll gameplay ids and EAT queued ring presses — self-sync `press_when` is unreliable at
  the title; forcing the title's return code (hook onLeave, inject 34) is the robust bypass.
- God mode: cur HP is NOT in the actor (0x15b90 obj); heap-hunt value→winnow after damage → single
  addr; freeze @0.25s. (Actor+0xB638 was the WALLET in one run — verify before freezing.)
- 2× crash at the MD town weapon-shop NPC (codex 2026-07-16 + USER 2026-07-17 first visit;
  second visit clean) — patched+instrumented both times, nondeterministic, no crash log. OPEN.

## Where this leaves the bug

Not reproducible on a fresh MD run with the exact reported monsters. Surviving hypotheses:
1. **Depth/scale**: Abyssal Ruins deep floors (bigger rosters / long sessions) + the +3 voice-slot
   shift → tail-row sound sets lose slots. Needs either a deep save or the constrained pool
   identified statically (the `0x40ce80` allocation cluster `0x587f00-0x588300`).
2. **Reporter-side environment**: NOT a bad interim build — `origin/master` never included Fix A
   (2874247/717cfb2 are local-only), so the reporter ran the initial clean seed, i.e. the SAME
   logic we failed to repro against. Remaining reporter variables: their JP `sotesx_s.dll`
   variant, sound device/config, MD progress. **NEXT: ask the reporter** for `oss_voice.log`,
   how deep in Abyssal Ruins, session length (fresh boot vs hours in), mob density on screen,
   and whether party/attack sounds stayed fine while ONLY those monsters were silent.
3. Plain SE channel-stealing under mob crowds (would repro stock too — not a patch bug).

Do NOT re-add the registrar retarget (`0x59ccce/0x59ccd8`) — see revert 717cfb2; base game + MD
floor 1 provably fine without it.

Provenance: live Frida campaign 2026-07-17 (runs/ennse-voice-repro/startup-*-2026071{6,7}-*),
static decompile of `vendor/unpacked/editions/sotes-ense-en.exe` via DecompileAt.java.
