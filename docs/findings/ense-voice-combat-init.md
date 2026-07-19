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

## The fix — restore the removed load EARLY, JP-faithful (implemented in `ennse_voice.c`)
EN app-init `FUN_00580ec0` runs: clear the bank cluster (`DAT_0092af80=0`) → `CALL FUN_005cb880`
(0x580fde, loads the other banks) → `CALL FUN_00581ba0` (0x58113e, boot/main-loop). The boot
still holds the retained voice gate: `if (DAT_0092af80!=0){ mgr=new; manager_init(sounddev,0x10);} `.
So the patch **redirects the CALL at `0x58113e`** through a tiny thunk that runs
`DAT_0092af80 = bank_load("sotesx_s.dll")` and jumps into the real boot. Net: the bank is live
**after all banks load, before the manager gate and before the first actor** — the engine then
builds the manager (dialogue) AND actors bake combat-voice ids, all through its OWN retained
code, exactly like the JP engine. Process memory only; exe on disk untouched. The `0x58113e`
byte signature (`E8 5d 0a 00 00`) positively IDs this EN-SE build, so the bundled JP `sotes.exe`
and unknown builds are never mis-patched (legacy late seed still available via
`OSS_ENNSE_LATE_SEED=1`).

## Cross-edition VAs (voice subsystem) — see docs/vamap/
| role | JP-SE | EN-SE (patch target) | EN-old |
|------|-------|------|--------|
| bank_load (LoadLibraryA wrapper) | `0x5d6880` | `0x5d8b10` | `0x5b0890` |
| bank INIT (clears cluster) | `0x57f180` | `0x580ec0` | `0x562210` |
| bank LOADER (voice load cut in EN) | `0x5c94f0` | `0x5cb880` | `0x5a4770` |
| boot / main-loop (voice-MGR gate) | `0x57fe50` | `0x581ba0` | — |
| **boot CALL site (the hook)** | — | **`0x58113e`** | — |
| actor builder (COMBAT bake) | `0x423890` | `0x423850` | `0x4289f0` |
| dialogue voice trigger | `0x435050` | `0x435000` | `0x439690` |
| voice PLAY (clip→mgr slot) | `0x437db0` | `0x437dc0` | `0x43c1b0` |
| dialogue-voice config gate | `0x5e3f50` | `0x5e6320` | — |
| voice bank global (THE gate) | `0x926170` | **`0x92af80`** | (none) |
| voice manager global | `0x926958` | `0x92b76c` | (none) |
| config obj | `0x926184` | `0x92af98` | — |
| sound device | `0x9288c8` | `0x92d5b8` | — |

EN bank cluster (4-byte HMODULE slots): `0x92af78` sotesp, `0x92af7c` MD, **`0x92af80` voice**,
`0x92af84` extra, `0x92af88` sotesd (== JP `0x926168+4k`).

## OPEN / verify live
- Confirm combat voice plays on the fix build (USER boot). If still silent with the bank set,
  suspect the config gate `*(config+0x238)` (a combat-voice-disable; must be 0 — it is in JP,
  same code) — read `*(*0x92af98+0x238)` live.
- The mod must attach before `0x58113e` executes. It does: the shipped `tools/mod_loader`
  `LoadLibraryEx`'s us on a worker thread spawned from its DllMain (pre-entry), and `0x58113e`
  is after the engine maps ~260 MB of bank DLLs — we win the race by a wide margin. Guards:
  a callsite-signature mismatch (wrong build) applies no patch; sound-device-already-up (loaded
  late) falls back to the legacy seed. If a future build shifts addresses the signature fails
  safe — regen `docs/vamap/`.
- **Loader v2 migration:** the sibling `../sotes-mod-loader` (v2) SKIPS DllMain-only mods (it
  requires an `OssModInit` export) — it explicitly names "the voice patch". This fix targets
  the shipped in-repo `tools/mod_loader` (loads every `mods\*.dll`). To run under v2, export
  `OssModInit` and do the seed at the first safepoint (`0x437c70`, main thread, before party
  actors) via `do_seed` (build the manager manually — the boot gate already ran); the inline
  hook is unnecessary there. Keep both paths if shipping for both loaders.

Supersedes the fullscreen worker-thread-race theory of `ense-voice-monster-se-drop.md` for the
COMBAT-voice symptom (that doc's monster-SFX drop is a separate, already-fixed issue).
