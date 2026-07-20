# EN-SE JP-voice patch — monster-SFX silence in FULLSCREEN — ✅ ROOT-CAUSED + FIXED (2026-07-18)

> **UPDATE 2026-07-20 — the "heap race" root cause below was a MISDIAGNOSIS. Real cause: a boot
> registrar deluxe-SKIP.** Deeper RE (decode of the `0x65b0e8` action table + disasm of
> `FUN_0059b520`) proved the mob silence is deterministic, not a race: the boot sound-def registrar
> `FUN_0059b520`, when the voice bank is set at the moment it runs, takes its DELUXE branch and does
> `goto skip` for every row lacking a deluxe id (`DAT_0065b104==0`) — which is exactly the 64 MONSTER
> rows (harpy key `0xc789` ids `0x790-0x795`, etc.), so their sound-defs never register → silent.
> Bank NULL at that instant → non-deluxe branch registers them → mob plays. This is an SE regression
> present in the JP-SE exe too (byte-identical), and it is gated on WHEN the bank is set relative to
> `FUN_0059b520`, NOT on which thread sets it. The 2026-07-18 "worker→silent / main-thread→audible"
> A/B was really "seed landed before vs after `FUN_0059b520`" (frame-pace-dependent in fullscreen).
> **Fix:** party combat ALSO comes from this registrar (not the actor bake), so retiming alone trades
> mob for combat. The working fix seeds the bank BEFORE the registrar (party → deluxe) AND flips one
> byte at `0x59ccce` so the registrar's `deluxe_id==0` skip (`je 0x59cd55`) becomes `je 0x59cd08` (the
> non-deluxe path) → monster rows register from `sotesd`. Full account + table data:
> `ense-voice-combat-init.md` (§DEFINITIVE root cause, §THE FIX, quirk #78). The heap-race analysis
> below is retained for provenance but is SUPERSEDED.

**Symptom (USER-refined 2026-07-18):** with `tools/ennse_voice`, monster REACTION sounds (harpy
hit/scream/damage, ghost evade/death, babymage cast…) go SILENT — but **only in FULLSCREEN** (with OR
without a ddraw wrapper), NOT windowed; gone if the patch is removed; fine in the native JP release.
Music/BGM/JP-voice/player-SFX all keep playing. Live repro: Weathervane Tower harpies — **base game,
NOT MD-specific** (the original "Mystery Dungeon only" framing was incomplete). USER was FULLSCREEN;
main-thread-seed build restored the harpy hit sound (+ JP voice for dad plays simultaneously).

## ✅ ROOT CAUSE — a worker-thread SEED RACE (USER's original hypothesis, PROVEN by the fix)
`ennse_voice`'s `seed_thread` did `bank_load` + `operator_new`×2 (voice mgr `0xc` + slot array
`count*0x18`) + `manager_init` from a WORKER thread ~1.2 s into boot — CONCURRENT with the engine's own
sound init on the MAIN thread. The engine is an old single-threaded MSVC build (non-thread-safe CRT
heap), so the worker-thread allocations race the engine's main-thread allocator and corrupt a nearby
monster-SFX resource → it plays silence. **FULLSCREEN-specific because it is a TIMING race:** fullscreen
(esp. + a ddraw wrapper) runs a high, STABLE frame rate whose pacing lands the race on the SFX resource;
windowed's slower/variable loop dodges it. The seed is a one-shot at boot, so the damage is done before
you reach any monster (which is why the earlier windowed-only campaign never saw it, and why nulling the
voice globals LIVE does not undo it — see below).

## The FIX (shipped in `ennse_voice.c`) — seed on the ENGINE MAIN thread
The worker thread now finds the game HWND, subclasses its `WndProc`, and `PostMessage(WM_OSS_SEED)`; the
WndProc — which runs on the window-owning (= MAIN/engine) thread — does `do_seed()` then un-subclasses
(one-shot). So `bank_load`/`manager_init` serialize with the engine's sound work and cannot race. Compile
toggle `OSS_SEED_MAIN_THREAD` (default 1; 0 = old worker-thread seed, for A/B); falls back to the
worker-thread seed if no window is found. **VERIFIED live (USER, fullscreen):** harpy hit sounds AUDIBLE
+ JP voice plays, simultaneously; `oss_voice.log` shows `[seed] running on MAIN thread (WndProc, tid=…)`
on a *different* tid than the worker that armed it. A/B-clean: worker seed → silent, main-thread seed →
audible, everything else equal.

## Why every EARLIER theory was wrong (all live-DISPROVEN 2026-07-18)
- **DirectSound focus-mute** (the `0xe2` SFX buffers lack `DSBCAPS_GLOBALFOCUS`): RED HERRING. The game
  IS foreground during play; forcing GLOBALFOCUS onto the SFX buffers live did NOT restore them; and the
  JP-VOICE buffers are ALSO `0xe2` (no GLOBALFOCUS, confirmed live: a 22050 Hz 334 KB voice buffer) yet
  play fine — focus-mute would kill both.
- **The MD sound-channel pool** (`0x587f87`/`0x5880e8` counts 4→7 when a bank loads): those are
  OPTIONS-MENU row counts, NOT a channel pool (`scratchpad/se-sound-pool-static.md`). No shared pool, no
  concurrent-buffer cap on modern Windows.
- **The per-frame voice service `0x437cd0`** (dispatched when mgr `0x92b76c` is set): nulling `0x92b76c`
  live did NOT restore the harpy → not the ongoing service.
- **The voice globals' presence:** nulling both `0x92b76c` + `0x92af80` live did NOT restore it — the SFX
  path never reads them; the patch's effect is INDIRECT (a boot-time heap-race corruption).

## The instrument that cracked it (`scratchpad/`, throwaway)
`detector.js` (Frida) + `attach.py` (loop-reattaches across the user's windowed→fullscreen relaunch):
hooks `soundPlay 0x4075a0` + `bufferStart 0x5e3e10` and, **per distinct IDirectSoundBuffer vtable**,
`Play`(12)/`GetStatus`(9)/`GetVolume`(6); logs each `CreateSoundBuffer` DSBUFFERDESC flags + a DDraw-coop
(`0x5e0d30`) fullscreen marker. THE KEY OBSERVATION: the harpy `Play` FIRES in fullscreen (`cls:OK`, vol
−200…−500, playing, not-lost) yet is silent — **API-transparent**, which is why every status-only probe
before saw a "healthy dispatch." `poke.py` = live u32 read/write (the null tests). ⚠ the single-Play-hook
first read of "Play skipped in fullscreen" was a MEASUREMENT ARTIFACT — fullscreen buffers use a
DIFFERENT DSOUND buffer-impl vtable (`0x73ec3890`) than windowed (`0x73ec3990`), so you must hook EVERY
class's Play, not just the first buffer's.

## Open (minor): exact corrupted structure not byte-pinned
The fix proves the *race* (serialize the seed → sounds return); the precise heap block the race clobbers
(which SFX resource / which of `bank_load` vs the two `operator_new`s) is not byte-RE'd — unnecessary
for the fix, a possible follow-up. Strong candidate = the non-thread-safe CRT `operator_new` (`0x5ef121`)
contended between the worker seed and the engine's concurrent sound-resource allocs.

---
## HISTORICAL — 2026-07-17 WINDOWED campaign (SUPERSEDED by the root cause above; kept for provenance)
The original report named Mystery Dungeon (Abyssal Ruins) monsters (Ghost Warlock evade/death, Black
Harpy damage/singing/death, Babymage damage/cast). The 2026-07-17 campaign below could not reproduce it
in 6 contexts — **all WINDOWED**, where the race does not trigger. That is the whole reason it looked
un-reproducible; the fullscreen variable was the missing key.

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
