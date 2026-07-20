# EN-SE JP-voice patch — combat/dialogue/mob voice: root cause + the JP-faithful fix

Target: **`sotes_en.exe`** (retail English Special Edition, SteamStub-packed). `tools/ennse_voice`
= standalone proxy **version.dll** that re-adds the JP **dialogue** + **deluxe combat** voices
(`sotesx_s.dll`) the EN localizer stripped. exe untouched, memory-only.

## The one-line bug
EN localizer removed exactly ONE line from the audio-bank loader `FUN_005cb880`: the voice-bank
load. So the voice-bank global **`DAT_0092af80` stays null**. Downstream, null bank ⇒:
- boot sound-def registrar `FUN_0059b520` bakes every combat grunt to the **non-deluxe** bank,
- per-actor builder `FUN_00423850` bakes combat-voice id 0 (silent),
- boot manager gate `FUN_00581ba0` (`if(0x92af80!=0){build voice mgr}`) skips ⇒ **dialogue silent**.
But `0x92af80` alone can't win: `FUN_0059b520` registers BOTH party-combat and monster defs in one pass,
and its DELUXE branch (bank set) gives party rows deluxe voice but SKIPS the monster rows (no deluxe
variant). Seed before it → deluxe combat but silent mob; seed after it → mob but non-deluxe combat. The
fix needs the early seed **plus** a 1-byte patch to the registrar's deluxe-skip — see "DEFINITIVE root
cause of mob silence" and "THE FIX" below.

## The gate (two boot-time bakers, SAME condition) — EN VAs, verbatim
Gate: `bank(0x92af80)==0  OR  *(*0x92af98+0x238)!=0  →  non-deluxe / silent`.

`FUN_0059b520` (boot sound-def registrar; JP `FUN_00599b40`; reads bank ONCE, bakes permanently):
```c
if (*(short*)((int)&DAT_0065b0f2 + iVar3) != 0) {                 // voiced action?
  if ((DAT_0092af80==0) || (*(int*)(*DAT_0092af98+0x238)!=0)) {   // NON-deluxe
    ... iVar4 = DAT_0092af78/param_3;  sVar8 = *(short*)(&DAT_0065b0f2+iVar3);   // sotesd/sotesp id
  } else {                                                        // DELUXE
    sVar8 = *(short*)(&DAT_0065b104+iVar3);                       // deluxe id
    if (sVar8==0 || sVar8==0x7fff) goto skip;
    iVar4 = DAT_0092af80;                                         // bank = sotesx_s
  }
  FUN_00584850(DAT_0092d5b8, iVar4/*bank*/, sVar8/*id*/, uVar5, param_2, 0);   // register def
}
// next: iVar3=(++uVar6 & 0xffff)*0x24;  end when (&DAT_0065b0e8)[idx*9]==0
```
`FUN_00423850` (per-actor builder; byte-1:1 w/ JP `FUN_00423890`; 22 callers via `FUN_00419af0`):
```c
if (((*(short*)(&DAT_0065b104+iVar11)==0) || (DAT_0092af80==0)) ||
    (*(int*)(*DAT_0092af98+0x238)!=0))  uVar12 = 0;                        // combat voice OFF
else                                    uVar12 = *(u32*)(&DAT_0065b108+iVar11);  // bake id
```
Monster/mob SFX **ARE registered by `FUN_0059b520`** (they are rows in the same 0x65b0e8 table — see
the mob trap below); they PLAY via SE players `FUN_00409320` (`FUN_005e37a0(slot, bank=*(this+0x20),
id=*(this+0x18))`) reading the registered def's bank+id. Dialogue is a separate path: play
`FUN_00437dc0` (ONE caller = trigger `FUN_00435000`), reads bank live per line (timing-immune).
`config+0x238` (a combat-voice-disable) is real (=0 in JP normal play, so the deluxe branch fires) but
NOT the operative blocker for the retimed fix; the deciding variable is whether `0x92af80` is null when
`FUN_0059b520` runs.

## DEFINITIVE root cause of MOB silence — the deluxe-branch SKIP (proven in table data, 2026-07-20)
`FUN_0059b520` walks all 294 rows of 0x65b0e8. In the DELUXE branch (bank set, `config+0x238==0`) it
does `sVar8 = DAT_0065b104[row] (deluxe id); if (sVar8==0 || sVar8==0x7fff) goto skip;` — i.e. **a row
with no deluxe variant registers NOTHING**. Decoding the EN table: **64 rows have a non-deluxe id but
`deluxe_id==0`, and they are exactly the MONSTER rows** — key `0xc789` = Black Harpy (ids 0x790-0x795,
in sotesd), ghosts, babymage `0xc792`, `0xc829`, rows 240-289. So:
- bank **NULL** when `FUN_0059b520` runs → non-deluxe branch registers them (bank=sotesd) → **mob plays**
- bank **SET** when `FUN_0059b520` runs → deluxe branch **SKIPS** them (no def ever registered) → **silent**
Disasm of the skip (0x59ccc2): `mov cx,[0x65b104+eax]; cmp cx,bx(0); je 0x59cd55; cmp cx,0x7fff; je
0x59cd55`. This is an **SE-introduced bug**: byte-identical `FUN_00599b40` in the JP-SE build (jpse) →
the native JP-SE exe ALSO loses mob SFX when sotesx_s is loaded early (USER-confirmed). A separate
build (v1.4) that natively loads sotesx_s plays all sounds → its registrar lacks this skip.

**Party combat comes from THIS registrar's deluxe def, NOT the actor bake** (build #12 proved it: seed
AFTER the registrar → mob ✓ + dialogue ✓ but combat went NON-deluxe; the per-actor bake `FUN_00423850`
exists but is not what drives the audible attack grunt). So the registrar does ONE pass with ONE bank
state, and you cannot get deluxe party rows AND registered monster rows from timing alone — retiming is
insufficient. The fix needs BOTH: seed the bank BEFORE the registrar (party rows → deluxe branch →
deluxe voice) AND patch the deluxe-skip so `deluxe_id==0` rows fall to the non-deluxe path (mob → sotesd)
instead of being dropped.

## Sound/voice IDs (bank membership settled)
| id dec(hex) | bank (runtime HMODULE) | category |
|---|---|---|
| **1003**(0x3eb), 1004 | **sotesx_s** | dialogue (reads bank live; play ret `0x437e82`) |
| **2235**(0x8bb), **2236**(0x8bc), 2239/2240/2252/2291/2341/2353/2399/2429/2444/2446… | **sotesx_s** | **DELUXE** combat grunts (SE players ret `0x409381/407af7/4052a4/40c966`) |
| **1342**(0x53e), 2230(0x8b6) | **sotesd** | **non-deluxe** combat grunts (USER: "1342 is 100% a non-deluxe combat line in sotesd") |
| 1289/1221/1285/1279/1293/1216/1191/1651-1654/1985/… | **sotesd** | mob/monster + player SE (non-voice) |

`sotesx_s.dll` = 253,206,528 B, 1,448 WAVE clips, PCM mono 22050/16-bit, lang 0x411(JP); **absent
from both EN editions**, identical JP-disc vs Steam-JP. Runtime HMODULE varies per boot (`0x1f050000`,
`0x23050000`, `0x23240000`). sotesd HMODULE `0x10000000`.

Id tables (static, **stride 0x24** = 9 dwords/action-row), EN addrs (JP = `-0x2000`):
`DAT_0065b0e8` key (end when key==0) · `DAT_0065b0f2`(short) non-deluxe id · `DAT_0065b104`(short)
deluxe id + gate value · `DAT_0065b108`(dword) actor-baked id · `DAT_0065b0f0`(ushort) SS_MGR idx into
`DAT_0092afe0[]` · `DAT_0065b0f4/fc`, `UNK_0065b0fe` other fields. JP mirrors: `0065908e8/659104/659108`.

## Cross-edition VA map (voice subsystem) — see `docs/vamap/`
jpse=`sotes-ense-jp.exe` (bundled JP, voiced ref) · ense=`sotes_en.exe` (patch target) · enold=Carpe
Fulgur 2012 (`docs/decompiled/`). All ImageBase 0x400000.

| role | jpse | **ense** | enold |
|------|------|------|-------|
| bank_load wrapper (LoadLibraryA wrap; 3 direct calls) | 0x5d6880 | **0x5d8b10** | 0x5b0890 |
| bank INIT (clears cluster, calls loader) | 0x57f180 | 0x580ec0 | 0x562210 |
| bank LOADER (voice load CUT in EN) | 0x5c94f0 | **0x5cb880** (50452 B) | 0x5a4770 |
| app-init→loader CALL | — | 0x580fde (`call 0x5cb880`) | — |
| app-init→boot CALL | — | 0x58113e (`call 0x581ba0`) | — |
| boot / main-loop (manager gate) | 0x57fe50 | 0x581ba0 | — |
| **boot sound-def registrar** | 0x599b40 | **0x59b520** | — |
| sound-def register fn `(dev,bank,id,cnt,p,0)` | — | 0x584850 | — |
| **actor builder** (combat-voice bake) | 0x423890 | **0x423850** | 0x4289f0 |
| dialogue voice trigger | 0x435050 | 0x435000 | 0x439690 |
| dialogue voice PLAY (1 caller) | 0x437db0 | 0x437dc0 | 0x43c1b0 |
| clip loader (bank,id→DSound) | 0x5e13d0 | **0x5e37a0** (prologue `6a ff 68 ab cb 5f 00`) | — |
| SE players (combat SFX) | — | 0x409320 | — |
| manager_init(mgr,dev,0x10) | 0x582d50 | 0x584a70 | — |
| operator_new (cdecl) | — | 0x5ef121 | — |
| **voice bank GLOBAL (the gate)** | 0x926170 | **0x92af80** | (none) |
| voice manager GLOBAL | 0x926958 | 0x92b76c | (none) |
| config obj ptr (`*=0x6a91540` live) | 0x926184 | 0x92af98 | — |
| sound device | 0x9288c8 | 0x92d5b8 | — |
| asset-mgr `this` | 0x925e58 | 0x92ac68 | — |
| SS_MGR array | 0x9261cc | 0x92afe0 | — |
| PeekMessageA IAT slot | — | 0x5fd20c | — |
| LoadLibraryA IAT slot (**IAT hook DEAD, see below**) | — | 0x5fd14c | — |

## Bank-load ORDER (the crux) — JP loads voice mid-sequence; EN restructured
JP `FUN_005c94f0` (voice = 3rd load, first wrapper call, BEFORE extra+sotesp):
```
sotesd.dll      → DAT_00926178   (inline LoadLibraryA)
sotesx_d2.dll   → DAT_0092616c   (inline LoadLibraryA)
sotesx_s.dll    → DAT_00926170   (wrapper FUN_005d6880)   ← VOICE — the ONE line EN removed
extra           → DAT_00926174   (wrapper)
sotesp          → DAT_00926168   (wrapper)
```
EN `FUN_005cb880` (voice slot 0x92af80 never touched; 2nd inline bank changed to English data):
```
sotesd.dll      → DAT_0092af88   (inline, ~0x5cbXXX)
sotesd_en.dll   → DAT_0092af94   (inline)                 ← EN-only; JP had sotesx_d2 here
[VOICE would load HERE in JP]
MD              → DAT_0092af7c   (wrapper call @ 0x5d79c0)
extra           → DAT_0092af84   (wrapper call @ 0x5d7a29)
sotesp          → DAT_0092af78   (wrapper call @ 0x5d7a8a)
```
Cluster: `0x92af78` sotesp · `af7c` MD · **`af80` VOICE** · `af84` extra · `af88` sotesd · `af94` sotesd_en.
All bank loads finish before the boot CALL (0x58113e) ⇒ before `FUN_0059b520` + manager gate + actors.

## THE FIX — early seed + 1-byte deluxe-skip patch (both needed; build #13)
Party combat and monster SFX BOTH come from `FUN_0059b520`'s one pass, so a single bank state can't
give deluxe party rows AND registered monster rows by timing. Two patches, armed together by a DllMain
waiter once SteamStub decrypts `.text`:
1. **Seed EARLY** — inline-hook the bank_load wrapper `0x5d8b10` (called directly by the loader on the
   main thread, before `FUN_0059b520`); on its first call set `0x92af80 = LoadLibraryA(sotesx_s)`. Steal
   the 6-byte prologue `81 EC 10 07 00 00` (`sub esp,0x710`) → trampoline → jmp `0x5d8b16`. So the party
   rows take the registrar's DELUXE branch (deluxe voice) and the manager gate builds the dialogue mgr.
2. **Patch the deluxe-skip** — one byte at `0x59ccce`: the registrar's `je 0x59cd55` (skip on
   `deluxe_id==0`) is `0f 84 83 00 00 00`; change the rel32 low byte `0x83 → 0x36` so it becomes `je
   0x59cd08` — the NON-deluxe path. Now `deluxe_id==0` rows (every monster) register from `sotesd` with
   their non-deluxe id instead of being dropped. Verified: at `0x59cccc` the regs (`eax`=row off,
   `edi`=sotesd, `ebx`=0) equal what `0x59cd08`'s real predecessors (`bank==0` / `config!=0`) pass, so
   the redirect is register-safe. The `0x7fff` sentinel `je` (`0x59ccd7`) is left intact (intentional
   deluxe suppression). rel32 math: `0x59cd08 − (0x59cccc+6) = 0x36`.

Why not just fill `deluxe_id` on the monster rows: the deluxe branch hardcodes `bank = 0x92af80`
(sotesx_s), which has no monster sounds → they'd resolve in the WRONG dll and stay silent. The
path-redirect keeps them on `sotesd`. Both patches are opcode-signature-gated ⇒ a future shift fails
safe. **Not the IAT:** `KERNEL32!LoadLibraryA` slot `0x5fd14c` hook doesn't stick on this SteamStub build
(attempt #8) — a direct `.text` inline hook is immune.

**Both editions, one DLL (USER-verified 2026-07-20).** The JP-SE `sotes.exe` (jpse) loads sotesx_s
natively, so dialogue + deluxe combat already work — but its registrar `FUN_00599b40` is byte-identical
to EN's, so the mob-silence deluxe-skip is there too. `FUN_00599b40` starts at `0x599b40`, so the skip
`je` sits at the SAME offset into the function: **EN `0x59cccc` → JP `0x59b2ec`**; both are
`0f 84 83 00 00 00` (`je <skip>`) and both take the identical byte patch `[je+2] 0x83→0x36` (redirect
delta `0x36`, → JP non-deluxe path `0x59b328`). So the DLL detects the edition by exe name — `sotes_en`
→ seed (`0x5d8b10`) + deluxe-skip (`0x59cccc`); `sotes` → deluxe-skip only (`0x59b2ec`), no seed —
every hook signature-gated so a wrong guess no-ops. JP mirrors for the diag: bank `0x926170`, clip
loader `0x5e13d0`, sotesp `0x926168`, sotesd `0x926178`.

## Attempt history (why each failed; keep to avoid re-treading) — ONE mechanism explains it all
Both party combat AND mob SFX come from `FUN_0059b520`; its deluxe branch gives party rows deluxe voice
but SKIPS `deluxe_id==0` rows (monsters). Deciding variables: is `0x92af80` set when the registrar runs
(party deluxe vs not; mob skipped vs not) and is it set before the manager gate (dialogue). The old
"heap race / worker-vs-main-thread" reads were a MISDIAGNOSIS of the same registrar timing.
| # | vehicle / timing | result |
|---|---|---|
| 0/9 | worker seed `Sleep(600)` + our mgr | deluxe combat; mob broke fullscreen (seed vs registrar by frame pace) |
| 1 | `0x58113e` boot-CALL redirect | combat ✓ mob ✓ **dialogue ✗** (our path bypassed engine mgr) |
| 2 | PeekMessageA pump + our mgr + actor hook | windowed all ✓; **fullscreen combat ✗** |
| 8 | version.dll LoadLibraryA IAT hook | no seed — IAT slot `0x5fd14c` dead on SteamStub |
| 10/11 | seed BEFORE registrar (worker poll / wrapper hook `0x5d8b10`) | combat ✓ dialogue ✓ **mob ✗** — deluxe-skip drops monsters |
| 12 | seed AFTER registrar (`0x5828e0`) | mob ✓ dialogue ✓ **combat non-deluxe** — proved party combat = registrar def, NOT the actor bake |
| **13** | **THIS: seed before registrar (`0x5d8b10`) + 1-byte deluxe-skip patch (`0x59ccce`)** | pending USER test (expect all three ✓, both modes) |

USER vehicle: standalone **version.dll**, mod-loading disabled, exe untouched. Reporter: a v1.4 build
natively loads sotesx_s with ALL sounds working ⇒ its registrar lacks the deluxe-skip; the SE introduced it.

## Status / open
- Build #13 staged (`…\steamapps\common\sotes\version.dll`, md5 `16f0bd24`, `-DOSS_VOICE_DIAG`). Awaiting
  USER windowed + fullscreen test: **deluxe combat (2235/2236) + JP dialogue (1003) + mob/harpy SFX**, all
  three. DIAG log should show BOTH `2235/2236 [VOICE_x_s]` AND harpy `0x790-0x795 [SOTESD]`.
- The deluxe-skip patch replicates what a clean v1.4 registrar does (register the non-deluxe sound when no
  deluxe variant exists). It touches ONE byte of engine `.text`; everything else is the pointer seed.
- **Loader v2 migration:** `../sotes-mod-loader` v2 skips DllMain-only mods (needs `OssModInit`). Under v2,
  arm both patches from the earliest native-init that precedes the bank loader; the standalone version.dll
  is the proven vehicle.
- JP disc `sotes.exe` = Themida-protected (deprioritized).

Provenance: static decompile of `vendor/unpacked/editions/sotes-ense-{en,jp}.exe` (Ghidra), objdump of
`FUN_00581ba0`/`FUN_0059b520` (full gate 0x59cc93-0x59cd58), decode of the 0x65b0e8 deluxe-id column,
live clip-trace + USER builds #10-13 (2026-07-19→20).
