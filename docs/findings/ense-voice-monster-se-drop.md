# EN-SE JP-voice patch — monster combat SE go SILENT (root cause + fix)

**Symptom (USER, 2026-07-11):** with the `tools/ennse_voice` JP-voice patch installed, some
MONSTER combat sounds stop playing — Ghost Warlock evasion/death, Black Harpy
damage/singing/death, Babymage (Baby Aquamage/Blazemage) damage/spellcast — tested vs
Abyssal Ruins mobs. Dialogue voice + party combat voices are fine. "probably other monsters too."

## Root cause — the SFX registrar drops SE-only defs once the voice bank is set

The engine has a static **sound-def table** in `.rdata` @ **VA `0x65b0e8`** (294 records, stride
**`0x24`**; `sotes-ense-en.exe`, SHA in `game-editions-and-voice.md`). Per-record fields:

| off | field | note |
|----|----|----|
| +0x00 | `key` (u32) | monster/char id (== `base_stat_table` id); 0 terminates the table |
| +0x04 | slot idx (u16) | active-sound slot |
| +0x08 | `mgr_idx` (u16) | index into the manager table `0x92afe0[]` |
| +0x0a | **SE clip id** (u16) | WAVE id in the SE bank (`sotesp.dll`); 0 ⇒ record skipped |
| +0x18 | bank sel (s32) | −1 ⇒ SE bank `0x92af78`; 1 ⇒ null; 0 ⇒ default bank `edi` |
| +0x1c | **voice id** (u16) | dialogue-voice clip in `sotesx_s.dll`; **0 / 0x7fff ⇒ no voice** |
| +0x20 | voice param (s32) | pan/vol for the voice |

Two fns consume the table: the **trigger** `0x4238xx` (play-by-key; on voice_id==0 sets the
voice-param to 0 and STILL plays — harmless) and the **registrar** **`0x59cc8c`** (registers each
def's clip into its manager at scene/battle init — the buggy one). Registrar per-def branch:

```
59cc9d  cmp [eax+0x65b0f2],0     ; SE id==0? -> skip (nothing to play)   je 0x59cd55
59ccaa  cmp ds:0x92af80,0        ; VOICE BANK loaded?
59ccb0  je  0x59cd08             ;   no  -> SE branch (register SE clip)
59ccb2  mov ecx,ds:0x92af98      ; sound ctrl obj
59ccba  cmp [[ecx]+0x238],0      ; voice-mode gate
59ccc0  jne 0x59cd08             ;   gate!=0 -> SE branch
59ccc2  mov cx,[eax+0x65b104]    ; voice id
59cccc  je  0x59cd55  <-- BUG    ; voice_id==0   -> SKIP (NO SE fallback)
59ccd7  je  0x59cd55  <-- BUG    ; voice_id==0x7fff -> SKIP
        ...register VOICE clip from 0x92af80 via mgr_table[mgr_idx]...
59cd08  SE branch: register SE clip (field_0a) from sel-bank via mgr_table[mgr_idx]
59cd55  loop tail (i++)
```

Stock EN-SE: voice bank `0x92af80`==null ⇒ `0x59ccb0` always jumps to the SE branch ⇒ every def
registers its SE clip ⇒ all monster SE play. **The patch sets `0x92af80` (to voice dialogue) ⇒ the
registrar now takes the voice branch, and any def with `voice_id==0` hits `je 0x59cd55` and is
SKIPPED — its SE clip is never registered ⇒ that sound is silent for the session.** Its manager slot
stays empty, so the later trigger `0x4238xx` plays nothing.

## Why only monsters — they are UNVOICED by design (voice_id==0)

Table dump: **58 of 294 records are SE-only (`voice_id==0`, `SE-id!=0`)** and they are the MONSTER
sound sets; party/NPC sets (`0xc35a`=Arche, `0xc35b`=Sana, `0xc35c`, `0xc35d`) carry real voice ids
(2228–2456) and keep playing. SE-only keys (== `base_stat_table` ids):

`0xc756` Kobold · `0xc760` Merkid · `0xc77f` Cocorat · `0xc789` **Ghost** · `0xc792` **Young Harpy**
· `0xc79c` Org Swordmaster · `0xc80b` Sabercat · `0xc829` **Baby Aquamage** · `0xc83d` Dragon Pup ·
`0xc4ae` · `0xc754` · `0xe2a4-8` · `0x1875b` · `0x1875f` (+ 4 special `22001-4` clips under `0xc35b`).

The USER's monsters are variant palette-swaps that share a base family's sound set (the trigger keys
the base): **Ghost Warlock `0xc78b` / Ghost Mage `0xc78a` → Ghost `0xc789`**; **Black Harpy `0xc794`
/ Harpy `0xc793` → Young Harpy `0xc792`**; **Baby Blazemage `0xc82a` → Baby Aquamage `0xc829`**. All
three base sets are voice_id==0 ⇒ silent under the patch. Matches the report exactly.

## The affected roster — 17 enemy sound-sets (the sound-set is per COMBAT CLASS)

The combat callers HARDCODE the sound-set key as an immediate `push 0x…; …; call 0x423850` — so a
sound-set is per combat-CLASS, and variant monsters that reuse a class share its key (that IS the
inheritance).  **22 sound-sets are trigger-hardcoded** (all callers of `0x423850`); of them **5 are
VOICED (unaffected)** — `0xc35a` Arche, `0xc35b` Sana, `0xc35c` Stella, `0xc35d` Chiffon, and notably
**`0x18754` Ancient Wyvern** (28 voiced records, a boss that DOES speak) — and **17 are UNVOICED ⇒
AFFECTED**:

| key | class / family (members share the class → all affected) |
|---|---|
| `0xc760` | **Merkid** — Merkid, Merkid Mage `0xc761`, Lead Merkid `0xc762`/`0x18749` |
| `0xc77f` | **Cocorat** — Cocorat, Vicious Cocorat `0xc780` |
| `0xc756` | **Kobold** — Kobold, Kobold Ace `0xc757`/`0x1874b` |
| `0xc789` | **Ghost** — Ghost, Ghost Mage `0xc78a`, Ghost Warlock `0xc78b`, Master Ghost `0x18759` |
| `0xc792` | **Young Harpy** — Young Harpy, Harpy `0xc793`, Black Harpy `0xc794` |
| `0xc79c` | **Org** — Org `0xc7b0`, Org Archer/Bowman `0xc7a6/7`, Org Swordmaster, Org Centurion `0xc7b2`, Boss Org `0x1874c`, Org Lackey `0x1874d` |
| `0xc80b` | **feline** — Sabercat, Saber-Panther `0xc80c`, Black Panther `0xc80d` |
| `0xc829` | **Babymage** — Baby Aquamage `0xc829`/`0x1874a`, Baby Blazemage `0xc82a` |
| `0xc83d` | **Dragon Pup** — Dragon Pup `0xc83d`/`0x1875c`, Ice Dragon Pup `0xc83e`/`0x1875d` |
| `0x1875b` | **Cerberus** |
| `0xc754` | class-id (not a roster stat id) — **likely the Skeleton class** (adjacent to Skeletal Soldier `0xc753`; Skeletal Corpora/Sergeant/Vile/Hopeful) |
| `0xc4ae` | class-id — UNIDENTIFIED (isolated `0xc4xx`; possibly early trash or a specific creature) |
| `0xe2a4` `0xe2a5` `0xe2a6` `0xe2a8` | class-ids in `0xe2xx` — **likely story bosses** (`0xe2a7` is defined but NOT hardcode-triggered) |
| `0x1875f` | class-id in `0x187xx` — **likely a late/postgame boss** |

Monster→class dispatch NOT yet RE'd, so the 7 class-ids (`0xc4ae`/`0xc754`/`0xe2a4-8`/`0x1875f`) are
mapped by id-adjacency + game knowledge, not proven.  Slimes/bats/snakes (`0xc742-4`, `0xc74e/f`…)
have their OWN stat ids but no triggered sound-set here ⇒ they appear to have no vocalization (generic
impact SE only, unaffected) — unconfirmed pending the dispatch RE.  **Earliest confirmed-affected:
Merkid / Cocorat / Kobold (early-game staples, ~100-150 HP)** — a first-dungeon repro, not the
postgame Abyssal Ruins.

## JP == EN — data & code identical; JP's fix is a runtime gate

`sotes-ense-jp.exe` has the **byte-identical** def table (base VA `0x6590e8`, same 294 recs, monster
keys voice_id==0) AND a **byte-identical registrar** (same forward `je` skip: JP `0x59b2e9`→`0x59b375`
== EN `0x59ccc9`→`0x59cd55`, +0x19e0 shift). So JP does NOT differ in data or code; it must rely on
the runtime **voice-mode gate `[[*0x92af98]+0x238]`** (or voice-bank load TIMING) to route combat SFX
through the SE branch. The patch reproduces neither ⇒ EN takes the voice branch and drops monster SE.

## The gate `Q[0x238]` — fully bounded; does NOT need mirroring (safety analysis)

`Q = *(*0x92af98)` = the **voice-subsystem settings/state object** (confirmed: sibling fields
`Q[0x28c]` read by sound-init `0x5821fa` + the Voice-Manager UI `0x588161`; `Q[0x2c4]` gates a voice
call in dialogue `0x435708`). `Q[0x238]` is the combat-SFX voice toggle. **Every reader of the gate,
whole engine:** exactly TWO — the registrar `0x59ccb2` and the trigger `0x423982` (verified by scanning
all `0x92af98` double-derefs; the many `0x46xxxx/0x47xxxx/[esp]+0x238` hits are a different
"sound-descriptor" struct type, same offset, not `Q`).

- **EN's effective gate value is 0** (voice mode) — PROVEN by the symptom itself: the registrar only
  skips monsters when `bank!=0 AND gate==0`, and the USER observes the skip, so `gate==0`.
- **The trigger `0x4238xx` never SKIPS** on the gate/voice-id — it only sets the voice-param (`ebp`) to
  0 and still enqueues. So the only silence-causing consumer is the registrar, fully covered by Fix A.
- **No missing-clip silence:** the voiced records reference voice_ids **2228–2456 (213 distinct)**, and
  `sotesx_s.dll` holds **1448 WAVE clips, ids 1003–2459 — ALL 213 present** (PE parsed flat: the
  Lizsoft asset DLLs are obfuscated `raddr=0` flat-mapped PEs, so file-off==RVA). So voiced defs never
  hit an absent clip; nothing else goes silent under voice mode.

⇒ **Fix A is robust to ANY gate value.** gate==0: voiced→voice, unvoiced→SE(Fix A). gate!=0: all→SE.
Either way every def registers a playable clip. Mirroring the gate is unnecessary AND undesirable: the
gate only picks voice-vs-SE for VOICED (party) defs — both audible — so forcing it (Fix B) would only
LOSE the party's JP combat voice. Not touching it keeps EN in voice mode, so the patch also restores
**party combat voices** (a bonus) alongside dialogue voice, while Fix A restores the unvoiced monster SE.

(Still OPEN, non-blocking: the gate's SETTER / why EN defaults to 0 — a `Q[0x238]` write via the
`0x92af98` double-deref wasn't located; likely the config/Voice-Manager load. Confirm live if desired
by logging `*(int*)((*(char**)(*(char**)AP(0x92af98)))+0x238)` during the Abyssal Ruins battle.)

## FIX A (recommended) — restore the SE fallback (2 bytes, runtime code patch)

Retarget the two `voice_id==0/0x7fff → skip` jumps to the **SE branch `0x59cd08`** (identical to
stock bank-null behavior; register state at the jumps is unchanged — eax/edi/esi intact). SE branch
uses default bank `edi` (proven = the always-present SE/effects bank: it is written as `[mgr+0x10]`
for boot/UI clips `0x4ed`… that play before any voice bank exists), or SE bank `0x92af78` for sel==−1.

| VA | file off | before | after | effect |
|----|----|----|----|----|
| `0x59ccce` | `0x19ccce` | `83` | `36` | `je 0x59cd55` → `je 0x59cd08` (voice_id==0) |
| `0x59ccd8` | `0x19ccd8` | `7c` | `2f` | `je 0x59cd55` → `je 0x59cd08` (voice_id==0x7fff) |

Voiced defs (voice_id!=0) are untouched ⇒ dialogue + party combat voice still play; monster SE-only
defs now register SE ⇒ restored. Apply in `version_proxy.c` seed_thread BEFORE (or with) the bank
seed; **guard on the current bytes (0x83 / 0x7c) so a different Steam build is not corrupted** — the
patch is build-locked (see the README's "Exact patch target"; add these 2 VAs to that list).

```c
static void patch_sfx_se_fallback(void){           // je 0x59cd55 -> je 0x59cd08 (x2)
    unsigned char *p1=(unsigned char*)AP(0x59ccce), *p2=(unsigned char*)AP(0x59ccd8); DWORD o;
    if(*p1==0x83){ VirtualProtect(p1,1,PAGE_EXECUTE_READWRITE,&o); *p1=0x36; VirtualProtect(p1,1,o,&o);}
    if(*p2==0x7c){ VirtualProtect(p2,1,PAGE_EXECUTE_READWRITE,&o); *p2=0x2f; VirtualProtect(p2,1,o,&o);}
}
```

**FIX B (alt, no code patch):** force the gate `[[*0x92af98]+0x238]` nonzero so the registrar always
takes the SE branch — simpler, but ALL table defs lose voice, so party COMBAT voices become SE too
(dialogue is a separate path `0x437077`/mgr `0x92b76c`, unaffected). Inferior fidelity; Fix A is
better. Needs the gate offset confirmed live either way.

## Safety verdict (answers "does Fix A break anything / are other sounds dropped?")
- **No other skips.** The registrar's only skips are SE-id==0 (nothing to play) and voice_id==0/0x7fff
  (both covered by Fix A). The gate is read at only 2 sites (registrar + trigger); the trigger never
  skips. So the gate mismatch causes NO silence beyond the monster registrar skip.
- **No missing-clip silence.** All 213 referenced voiced ids are in `sotesx_s.dll` (proven above).
- **Fix A can't regress.** For the affected records it reproduces the byte-exact stock SE-branch path
  (`edi` bank identical stock-vs-patched — set at registrar entry, independent of the bank globals).
- Net after Fix A: dialogue voice ✓, party combat voice ✓ (bonus restore), monster SE ✓ — nothing mute.

## Open / validate (non-blocking)
- Live-repro the silence + Fix A on retail EN-SE in Abyssal Ruins (USER setting up an endgame save);
  optionally log `Q[0x238]` live to confirm gate==0 empirically.
- The gate SETTER / why EN defaults to 0 (config vs Voice-Manager load) — effect already bounded.
- Confirm the variant→base key mapping in the combat trigger (`0x4238xx` caller / key calc).
- PORT note: when OpenSummoners adds EN-text+JP-voice (ROADMAP), its voice path MUST fall back to the
  SE clip on voice_id==0 — do not replicate this drop.

Provenance: static RE of `vendor/unpacked/editions/sotes-ense-{en,jp}.exe` (objdump/py); no live run yet.
