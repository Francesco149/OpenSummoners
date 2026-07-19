# EN-SE JP-voice patch — plan

**Goal:** a redistributable **DLL** that makes the retail **EN special edition** play the
JP dialogue voice (from `sotesx_s.dll`), with the **exe left unmodified** (still packed;
Steam-launchable). No port integration (that's a separate future track).

## Status (2026-07-01)

**DONE — voice plays (user-confirmed: JP voice on the EN SE, English text + JP audio).**
`tools/ennse_voice/` is a mod-loader mod that seeds the voice bank so the engine plays voice
with its own per-line mapping. `oss_voice.log` traces each step.
**Next:** ship via CI nightly release (see repo `.github/workflows/`).

### ✅ ROOT-CAUSED + fix implemented (2026-07-19) — COMBAT voice was silent (seed too LATE)
Combat voice (Arche's attack grunts = `sotesx_s.dll` clips **2235/2236**) is NOT the dialogue
path — it's baked per-actor: the action-table builder `FUN_00423850` writes each attack's
voice id into the actor **only if the voice bank `0x92af80` is set AT ACTOR-CREATION**; else
0 = permanently silent. The old seed set the bank LATE (worker→WndProc, after the party
actors were built) → dialogue worked, combat baked-silent. **Fix:** an early main-thread hook
that restores the ONE line the localizer removed at the JP-equivalent point — redirect the
`CALL FUN_00581ba0` at **`0x58113e`** (inside app-init, after banks load, before the boot's
manager gate + first actor) through a thunk that runs `0x92af80 = bank_load("sotesx_s.dll")`.
The engine's own retained gate then builds the manager (dialogue) AND actors bake combat
voice — identical to JP. No manual manager creation; process memory only. Legacy late seed
kept behind `OSS_ENNSE_LATE_SEED=1`. Full RE + the cross-edition VA map:
**`docs/findings/ense-voice-combat-init.md`** + **`docs/vamap/`** (a BinDiff-style edition
matcher: jpse↔ense 99.6%, the localizer-edited audio-init trio auto-recovered). USER to
confirm live.

### ✅ RESOLVED (2026-07-18) — monster SFX silenced in FULLSCREEN (a worker-thread SEED RACE)
The silence was **fullscreen-specific, not MD-specific**: the patch seeded (`bank_load` + `operator_new`×2
+ `manager_init`) from a WORKER thread ~1.2 s into boot, racing the engine's single-threaded sound init on
the main thread and corrupting a monster-SFX resource. Fullscreen's high, stable FPS lands the race;
windowed's slower loop dodges it (which is why the whole 2026-07-17 windowed campaign never repro'd).
**FIX: seed on the engine MAIN thread** (the worker subclasses the game window's WndProc + posts a message;
the WndProc runs on the main/engine thread → serialized, no race). Compile toggle `OSS_SEED_MAIN_THREAD`
(default 1). USER-verified live in fullscreen: harpy hit sounds + JP voice both play. The old MD-pool
(`0x5880e8` = an Options-MENU row count, not a channel pool) and focus-mute (voice buffers are also `0xe2`
no-GLOBALFOCUS yet play) theories were live-DISPROVEN. Full root cause + disproven theories + the Frida
instrument (`scratchpad/detector.js`): **`docs/findings/ense-voice-monster-se-drop.md`**.
⚠ An earlier base-game "registrar 2-byte fix" was a MISDIAGNOSIS — **reverted** (base was never broken).

## The key finding (why this is easy)

`sotes_en.exe` (EN-SE engine) **retains the ENTIRE voice subsystem code** — the localizer
only removed the **loader** (the `LoadLibrary("sotesx_s.dll")` + bank-handle store). Proof:
the voice **trigger** (`cmp di,0x7fff`, `66 81 ff ff 7f`) and the **play fn** slot-pool loop
(`8d 04 40 83 3c c6 00`) are present **byte-for-byte in both engines** (EN just ~0x10–0x5a
shifted). The play path is intact but **dead**, gated on two globals that are never set:
- voice-**bank** handle — never written in EN (no `a3` store among its xrefs) ⇒ always null.
- voice-**manager** — created by sound-init *only if* the bank is non-null ⇒ never created.

⇒ **Patch = re-seed those two globals.** The engine then plays voice using its OWN per-line
mapping (the per-line voice id at `[esp+0x3c]` is read from the byte-identical `sotesd.dll`
scenario data — so no id↔line RE, no dialogue hook needed).

## Voice subsystem map (VA = fileoff + 0x400000 in both unpacked engines)

| thing | JP engine (`sotes.exe`) | EN engine (`sotes_en.exe`) |
|---|---|---|
| asset-manager `this` | `0x925e58` | `0x92ac68` |
| **bank_load(this,name,1,1)** fn | `0x5d6880` | `0x5d8b10` |
| **voice-bank handle global** | `0x926170` | **`0x92af80`** |
| **voice-manager global** | `0x926958` | **`0x92b76c`** |
| sotesp (SE) handle global | `0x926168` | `0x92af78` |
| dialogue voice **trigger** block | `0x437020`+ | `0x437077`+ |
| per-line voice id | `[esp+0x3c]` (skip if `0` / `0x7fff`) | same |
| config gate (Dialogue-Voice ON?) | `0x5e3f50` | `0x5e6320` |
| **play(mgr,bank,id,param)** fn | `0x437db0` | `0x437dc0` |
| play→ channel alloc → load+DSound | `0x5e13d0` | (mirror TBD) |
| manager-create (in sound-init) | `0x580bed`–`0x580c1f` | ~`0x581xxx` (mirror TBD) |
| manager_init(this,sounddev,0x10) | `0x582d50` | (mirror TBD) |
| engine `operator new` | `0x5ecd71` | (mirror TBD) |

Manager is a 0xC struct `{ptr field0; u16 count@+4; ptr slotarray@+8}`; slots are 0x18 B.
Voice clips = `WAVE` resources **1003+**, PCM/mono/22050/16-bit. `DATA/1002` = a validation
header (`u32=10001` magic + obfuscated bytes), NOT a line→clip table.

JP sound-init voice branch (the sequence to replicate/trigger):
```
if (voice_bank != 0) {                       // 0x580bed
    mgr = operator_new(0xc); *(int*)mgr = 0;  // 0x580bf6..0x580c04
    voice_mgr_global = mgr;                    // 0x580c19
    manager_init(mgr, sound_device_0x9288c8, 0x10);  // 0x582d50
}
```

## Patch approach (DLL)

1. **Inject vehicle — a proxy DLL the packed exe auto-loads** (exe untouched). Candidate:
   **`version.dll`** proxy in the game dir (classic; forwards the ~15 real exports to
   `System32\version.dll`, runs our code on attach). ⚠️ MUST verify it actually loads —
   ckpt-164 found a bare `ddraw.dll` app-dir drop loses to `System32\ddraw.dll`; version.dll
   is usually safe (not KnownDLL / not pre-loaded) but TEST it. Fallback: a launcher exe that
   `CreateProcess(SUSPENDED)`+remote-`LoadLibrary`+`Resume` (like `build/inject.exe`) — keeps
   the exe unmodified but the user runs the launcher, not the exe directly.
2. **Seed, once, when the engine is ready** (asset-mgr `0x92ac68` initialized). Do it all by
   calling the engine's OWN functions (so values/layout are exactly right):
   - `0x92af80 = bank_load(0x92ac68, "sotesx_s.dll", 1, 1)`  (call EN `0x5d8b10`)
   - create the manager: `mgr = operator_new(0xc); *(int*)mgr=0; 0x92b76c = mgr;
     manager_init(mgr, <EN sound-device global>, 0x10)` (EN mirrors of `0x5ecd71`/`0x582d50`)
   - Timing options: (a) hook the EN bank-loader's return (asset-mgr ready, banks loaded),
     set the bank there, let sound-init create the mgr IF loader precedes sound-init; else
     (b) lazy: hook a per-frame fn, seed on first call when `0x92af80==null && mgr ready`.
3. **Config gate.** The trigger calls `0x5e6320` and skips if it returns 0 = the "Dialogue
   Voice" setting. Must ensure it's ON (set the underlying config, or patch the gate). VERIFY
   what it reads.

## Open items (blocking a prototype)

- [x] **Inject feasibility — CONFIRMED.** `sotes_en.exe` imports exactly 3 fns from
      `version.dll` (`GetFileVersionInfoA` / `…SizeA` / `VerQueryValueA`). A proxy
      `version.dll` in the game dir **loads into the engine** (verified marker: `self=…\
      VERSION.dll` = ours) and forwards the 3 to a `realver.dll` (renamed copy of the
      user's own `SysWOW64\version.dll`, to dodge base-name-match recursion). The
      ckpt-164 `ddraw`→System32 gotcha does NOT apply to `version.dll`; the packed exe
      keeps the same static import ⇒ works on the shipped game. `tools/ennse_voice/`.
- [ ] Which exe Steam launches (is `sotes.exe` a launcher that spawns `sotes_en.exe`?).
- [ ] EN mirrors of: `operator new` (JP `0x5ecd71`), `manager_init` (JP `0x582d50`),
      the sound-device global (JP `0x9288c8`), the play-resource fn (JP `0x5e13d0`).
- [ ] What `0x5e6320` (config gate) reads — and how to force it ON.
- [ ] Verify the EN engine actually POPULATES `[esp+0x3c]` (voice id) from scenario data
      (near-certain — dialogue fn + `sotesd.dll` are byte-identical — but confirm live).
- [ ] Decompile both engines (JP running; EN queued) for the above mirrors + init args.

## Deliverable & future

- **This track:** `tools/ennse_voice/` → a proxy/inject DLL + a small installer/readme; drop
  into `…\steamapps\common\sotes\`, launch, hear JP voice over EN text.
- **DONE (2026-07-02):** `tools/voice_view` grew into **`tools/res_explorer`** — the full
  SotES resource explorer (ImGui/DX11; sprites/maps/audio/BGM/strings, preview + export,
  self-contained `res_explorer.exe` in every release). voice_view is retired; its
  `--list`/`--dump` CLI carried over.
