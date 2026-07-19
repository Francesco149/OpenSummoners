# EN-SE JP-voice patch — COMBAT voice silent — ROOT-CAUSED + fix implemented (2026-07-19)

**Symptom (USER):** with `tools/ennse_voice`, dialogue voice plays but **combat** voice
(Arche's attack grunts — `sotesx_s.dll` clips **2235/2236**) is wrong/silent; the native JP
`sotes.exe` plays them, and they vanish if `sotesx_s.dll` is removed. Awaiting the user's
live confirmation of the fix build.

## Root cause — combat-voice id is baked at ACTOR-CREATION, gated on the bank
Combat voice does NOT go through the dialogue path (`FUN_00437db0`, one caller only). It is
per-actor: the action-table builder **`FUN_00423850`** (EN; == JP `FUN_00423890`, byte-1:1)
runs when each character actor is created and, for every attack action, bakes the voice id
into the actor's action slot — but only behind this gate (EN `FUN_00423850`):
```c
if ((DAT_0065b104[act]==0) || (DAT_0092af80==0) || (*(*DAT_0092af98+0x238)!=0))
    voice_id = 0;                 // combat voice OFF for this action
else
    voice_id = DAT_0065b108[act]; // bake the sotesx_s combat-voice clip id
```
`DAT_0092af80` = the voice-bank HMODULE. **If the bank is null when the actor is built, the
id bakes 0 and that actor is PERMANENTLY silent in combat** (re-checked only on the next
actor rebuild). Dialogue is immune because the dialogue trigger reads the bank *live*.

## Why the old patch failed it — seed too LATE
The EN localizer removed exactly ONE line: the voice-bank load. In JP the bank loads at CRT
init — `entry → FUN_0057f180 → FUN_005c94f0` line 3951 `DAT_00926170 = bank_load("sotesx_s.dll")`
— long before any actor. The EN bank-loader `FUN_005cb880` (== JP `FUN_005c94f0`) simply omits
`DAT_0092af80 = bank_load(...)` (loads sotesd/MD/extra/sotesp, never voice), so the bank stays
null. The old patch re-seeded it, but LATE (worker thread → main-thread WndProc, ~1.2 s into
boot) — **after the party actors are built** → dialogue worked, combat baked-silent. Neither
thread nor manager was the issue; only *when* the bank global becomes non-null.

## The fix — seed from the engine's own message pump (shipped in `ennse_voice.c`; USER-verified windowed)
`do_seed()` revives the two globals with the engine's OWN functions: `0x92af80 =
bank_load("sotesx_s.dll")`, then `0x92b76c = new; manager_init(sounddev,0x10)`. It must run
(a) on the MAIN thread (so operator_new serializes with the engine — the old worker seed
raced it, dropping monster SFX in fullscreen), (b) AFTER SteamStub decrypts `.text` and sound
init, (c) BEFORE the first party actor, and (d) **AFTER the engine's boot gate** — because
seeding the bank BEFORE that gate (`if (0x92af80!=0){ mgr=new; manager_init… }` in
`FUN_00581ba0`) makes the gate build a manager that (observed) does NOT voice dialogue; letting
the gate see a null bank + skip leaves OUR proven manager as the one used.

The trigger that satisfies all four: **IAT-hook the engine's own pump `USER32!PeekMessageA`**
(import slot **`0x5fd20c`**). A tiny waiter swaps the slot to our `my_peek` once the loader/
SteamStub resolves it (resolves at the OEP, the first pump is ~260 MB-of-bank-loads later — a
wide margin); the seed then fires deterministically on the engine's first pump frame — main
thread, post-decryption, post-sound, post-gate, pre-actor. IAT-hooking is a pointer swap, so
it is prologue-independent (Win10/11 dropped the `mov edi,edi` hot-patch prologue that inline
USER32 hooks relied on). Combat is bulletproofed twice: the frame-0 seed sets the bank before
any party actor, AND we inline-hook the actor builder `0x423850` (known prologue
`83 ec 08 53 55`) so `do_seed` runs before every actor's voice-id bake. Process memory only;
exe on disk untouched. (An earlier attempt redirected the `0x58113e` boot CALL to seed before
the gate — it fixed combat but the gate's manager didn't voice dialogue; superseded.)

## Cross-edition VAs (voice subsystem) — see docs/vamap/
| role | JP-SE | EN-SE (patch target) | EN-old |
|------|-------|------|--------|
| bank_load (LoadLibraryA wrapper) | `0x5d6880` | `0x5d8b10` | `0x5b0890` |
| bank INIT (clears cluster) | `0x57f180` | `0x580ec0` | `0x562210` |
| bank LOADER (voice load cut in EN) | `0x5c94f0` | `0x5cb880` | `0x5a4770` |
| boot / main-loop (voice-MGR gate) | `0x57fe50` | `0x581ba0` | — |
| **pump IAT slot (the trigger)** | — | **`0x5fd20c`** (PeekMessageA) | — |
| **actor builder (COMBAT bake + 2nd hook)** | `0x423890` | **`0x423850`** | `0x4289f0` |
| dialogue voice trigger | `0x435050` | `0x435000` | `0x439690` |
| voice PLAY (clip→mgr slot) | `0x437db0` | `0x437dc0` | `0x43c1b0` |
| dialogue-voice config gate | `0x5e3f50` | `0x5e6320` | — |
| voice bank global (THE gate) | `0x926170` | **`0x92af80`** | (none) |
| voice manager global | `0x926958` | `0x92b76c` | (none) |
| config obj | `0x926184` | `0x92af98` | — |
| sound device | `0x9288c8` | `0x92d5b8` | — |

EN bank cluster (4-byte HMODULE slots): `0x92af78` sotesp, `0x92af7c` MD, **`0x92af80` voice**,
`0x92af84` extra, `0x92af88` sotesd (== JP `0x926168+4k`).

## Status / open
- **USER-verified (2026-07-19, windowed):** combat grunts, dialogue voice, AND mob/monster SFX
  all correct. Fullscreen re-test pending (expected fine — the seed is main-thread via the pump,
  so the old worker-thread heap race that dropped fullscreen monster SFX cannot occur).
- Config gate `*(config+0x238)` (a combat-voice-disable) is 0 in normal play (it is in JP, same
  byte-identical code) — combat voice confirmed live, so no action needed.
- **Timing is deterministic, not raced:** the seed fires on the engine's own first `PeekMessageA`
  frame. Only the IAT-slot waiter thread races, and with a wide margin (slot resolves at the OEP;
  first pump is after the ~260 MB bank load). If a future build shifts addresses, the actor-sig /
  IAT checks fail safe (no patch) — regen `docs/vamap/`.
- **Loader v2 migration:** the sibling `../sotes-mod-loader` (v2) SKIPS DllMain-only mods (needs an
  `OssModInit` export) — it explicitly names "the voice patch". This targets the shipped in-repo
  `tools/mod_loader` (loads every `mods\*.dll`). Under v2, export `OssModInit` and run `do_seed` +
  the actor hook from the first safepoint (`0x437c70`, main thread) instead of the pump IAT hook.

Supersedes the `0x58113e` boot-CALL-redirect attempt (fixed combat but the engine gate's manager
did not voice dialogue). The fullscreen monster-SFX drop of `ense-voice-monster-se-drop.md` is the
same root cause (worker-thread seed) — this main-thread pump seed resolves it too.
