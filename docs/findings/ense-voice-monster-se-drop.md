# EN-SE JP-voice patch — MONSTER sounds silenced in MYSTERY DUNGEON (open)

**Symptom (USER):** with the `tools/ennse_voice` JP-voice patch installed, monster combat sounds
(Ghost Warlock evasion + death, Black Harpy damage/singing/death, Babymage damage/spellcast) go
SILENT — **but ONLY in Mystery Dungeon / Abyssal Ruins**, the Special-Edition roguelike gamemode
(new SE content). Dialogue voice works. **Base-game monsters are UNAFFECTED** — the user played most
of the game on the (old) patch with zero issues; base Ghosts in Chartreux Catacombs play their
death/evasion sounds correctly WITH the patch.

## STATUS: base-game path was a DETOUR — the real bug is Mystery-Dungeon-specific

An earlier pass mis-diagnosed this as a base-game def-table bug and shipped a 2-byte registrar patch
(retarget `0x59ccce`/`0x59ccd8`). **That was WRONG and is REVERTED** (`version_proxy.c` is back to the
clean bank+manager seed). Proof it was wrong: base-game monster sounds were never broken — same with
or without the patch, and same before/after that "fix". Do NOT re-add it.

## Background — the base-game sound system (works fine WITH the patch; here for reference)

- Static sound-def table @ VA **`0x65b0e8`** (294 recs, stride `0x24`). Per-record: `+0x00` key
  (monster/char id), `+0x04` action code, `+0x08` mgr_idx, `+0x0a` SE clip id, `+0x1c` voice id.
- Consumed by the registrar `0x59cc8c` (preload) + trigger `0x423850` (enqueue by key; 22 callers,
  each hardcodes a monster sound-set key). Per-monster sound object at `monster+0x15b78`; play method
  `0x4075a0`.
- Monster action codes (confirmed by USER listening in res_explorer): **a73/a74/a9 = attack/hit
  (work), a12 = death, a7 = evasion/backstep**. All monster records are `voice_id==0` (unvoiced by
  design; only Arche/Sana/Stella/Chiffon + the Ancient Wyvern boss `0x18754` are voiced). Base monster
  clips live in **`sotesd.dll`** (the default bank; e.g. Ghost death=1936-1941, Merkid=1643-1645).
- Records for working vs "reported-broken" base sounds are BYTE-IDENTICAL, and all clips are present —
  which is *why* nothing base-game reproduced the bug. It never was a base-game bug.

## THE REAL BUG — Mystery Dungeon uses the `sotesx_d2` sound bank, adjacent to the voice bank

The bank block (engine globals; loader `0x5d79c0`+ via `bank_load 0x5d8b10`):

| global | DLL | role |
|---|---|---|
| `0x92af78` | `sotesp.dll` | base SE bank |
| **`0x92af7c`** | **`sotesx_d2.dll`** | **SE / Mystery Dungeon content bank** (88 MB) |
| `0x92af80` | `sotesx_s.dll` | VOICE bank — **what the patch installs** |
| `0x92af84` | `sotesx_d3.dll` | tiny SE patch |

MD sounds come through the `sotesx_d2` (`0x92af7c`) bank, which sits **immediately before** the voice
bank `0x92af80`. A large sound-resource setup routine (entry ~**`0x586af1`**) registers sounds with
**slot counts that change based on which banks are loaded**:
- `0x587f87`: `count 6→8` when the **MD bank** `0x92af7c` is present.
- `0x5880e8`: `count 4→7` when the **voice bank** `0x92af80` is present.

⇒ **Installing the voice bank measurably alters the MD sound-resource/slot allocation.** Leading
hypothesis: the extra voice slots **shift the slot/channel indices** the MD sounds were registered
into, so the MD play reads the wrong/empty slot → silent. (Mechanism CLASS confirmed statically; the
exact slot/routing step is runtime allocation — needs live observation to pin.) Heavy MD-bank consumer
to inspect: the `0x594xxx` cluster (`0x5943e5`+, ~15 reads of `0x92af7c`). Dual-bank consumers
(read BOTH `0x92af7c` and `0x92af80`): the `0x582xxx` and `0x588xxx` (the `0x586af1`) functions.

## PLAN (next session) — drive the EN-SE game into Mystery Dungeon + trace

1. **Repro:** launch the retail EN-SE `sotes_en.exe` WITH the patch installed (`version.dll` +
   `realver.dll` + `sotesx_s.dll` in `…\steamapps\common\sotes\`), **new game → select Mystery
   Dungeon** (a roguelike; immediately available on a new game — no endgame save needed). Reach a MD
   monster; the death/evasion/etc. sound is silent.
2. **Trace (audio can't be "heard" — detect via the call path):** Frida-hook the sound-resource setup
   (`0x586af1`), the play method (`0x4075a0`) / low-level DSound play, and the bank reads, and watch a
   MD monster sound: does the play call fire? does it resolve a valid slot/clip? which bank? Compare
   WITH vs WITHOUT the voice bank loaded (`0x92af80`).
3. **Bisection (localizes bank-vs-manager):** a diagnostic `version.dll` that loads the bank but SKIPS
   the manager creation — if MD still breaks, it's the bank presence (`0x92af80`) perturbing the
   sound-resource setup (the `0x586af1` finding); if MD works, it's the manager.
4. **Fix** goes at the patch level once the exact step is known (e.g. avoid perturbing the MD slot
   allocation, or correct the routing) — NOT a base-game def-table change.

## Driving-setup notes (for next session)
- Game exe: `C:\Program Files (x86)\Steam\steamapps\common\sotes\sotes_en.exe` (EN-SE). Unpacked copy
  for RE: `vendor/unpacked/editions/sotes-ense-en.exe`. Full disasm is regenerable via objdump.
- The live-probe MCP (`mcp__opensummoners__*`) targets the PORT exe (`sotes.unpacked.exe`), NOT
  `sotes_en.exe` — **verify/adapt** it (or use `build/inject.exe` + a Frida agent) to launch + drive +
  observe the EN-SE game. Menu nav = keyboard; screenshots to see the menu; the roguelike likely can't
  be saved mid-run.
- USER will watch the run (they've not played the gamemode). Push visuals to the llm-feed as you go.

Provenance: static RE of `vendor/unpacked/editions/sotes-ense-en.exe`; base-game repro confirmed live
by USER; MD path not yet live-traced.
