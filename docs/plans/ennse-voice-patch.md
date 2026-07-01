# EN-SE JP-voice patch — plan

**Goal:** a redistributable **DLL** that makes the retail **EN special edition** play the
JP dialogue voice (from `sotesx_s.dll`), with the **exe left unmodified** (still packed;
Steam-launchable). No port integration (that's a separate future track).

## Status (2026-07-01)

**DONE — voice plays (user-confirmed: JP voice on the EN SE, English text + JP audio).**
`tools/ennse_voice/` builds a proxy `version.dll` (all 17 exports forwarded to
`realver.dll`) that, once the sound device is up, calls the engine's own
`bank_load("sotesx_s.dll")` → sets `0x92af80`, and `operator_new`+`manager_init` →
sets `0x92b76c`. The engine then plays voice with its own per-line mapping.
Redistributable: `version.dll` + `Install.bat`/`install.ps1` (auto-detect game +
JP `sotesx_s.dll`) + `Uninstall.bat` + `README.md`. `oss_voice.log` traces each step.
**Next:** ship via CI nightly release (see repo `.github/workflows/`).

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
- **QUEUED (future session, user request):** grow `tools/voice_view` into a full **SotES
  resource explorer** covering every format we've RE'd/ported, packaged as a **self-contained
  redistributable exe**. (Not this track.)
