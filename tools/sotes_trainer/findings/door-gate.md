# The door-transition gate — full RE (base `FUN_0059a1f0` ↔ SE `0x5c2af0`)

The door-USE handler decides, every frame the leader overlaps a door anchor, whether to fire a
room transition.  RE'd off the port's base decompile (`docs/decompiled/by-address/59a1f0.c`,
`FUN_0059a1f0`) cross-referenced 1:1 to the SE exe (`0x5c2af0`) + confirmed live with the trainer
call-trace harness.  This is the authority for the **warpgate** feature (instant doors) + the
**unseen gate**.  Base and SE are independent recompiles: the ALGORITHM carries, the struct offsets
DO NOT — the SE `O` (door-state) offsets below are the SE ones.

## Signature
`__thiscall (ret 0x18)`; `ecx = O` (the per-door STATE object).  Stack args (SE, after prologue):
`[esp+0x38]=param_2` = the leader controller · `[esp+0x3c]=param_3` / `[esp+0x40]=param_4` /
`[esp+0x24 local]=param_5`-ish OUT (target_room / entry_key / exit_key) · `[esp+0x48]=param_6` = the
FORCED flag.  Returns **1 = fire the transition** (writes the OUT triple @ `0x5c314e`), else 0.
Called per-frame from `0x5b6f97` (the scene update loop).  VERIFIED live: the handler runs ~60×/s
while the leader stands at a door; when idle it bails early (before the changed-check), so the
combat/ramp code only runs on an actual door-enter attempt.

## The SE `O` (door-state) struct — RE'd off `0x5c2af0`
| off | role | base analog |
|---|---|---|
| `O+0x00` | current room key (commit sets `=O+0x08`) | `in_ECX[0]` |
| `O+0x04` | committed flag (set 1) | `in_ECX[1]` |
| `O+0x08` | the door ANCHOR / target | `in_ECX[2]` |
| `O+0x0c` | the anchor (dup) | `in_ECX[3]` |
| `O+0x10` | **bVar3 = SEEN flag** (target room in the leader's seen-list) | seen-scan result |
| `O+0x14` | an unseen sub-flag (non-overworld unseen) | — |
| `O+0x18` | mode word (0/1/2; **2 = blocked**) | `*(ushort*)(in_ECX+4)` |
| `O+0x1c` | combat COOLDOWN countdown (set 0x1e=30 on combat; dec/frame) | `in_ECX[5]` |
| `O+0x20` | **the HOLD RAMP** (0→10000; the "hold prompt") | `in_ECX[6]` |
| `O+0x24` | can-enter flag (leader grounded+still at the door) | `in_ECX[7]` |
| `O+0x28` | free-running frame counter (`&0x800000ff`) | `in_ECX[8]` |
| `O+0x2c` | SHORT ramp (word, 0→40) on the not-changed path | `*(ushort*)(in_ECX+9)` |
| `O+0x30` | a per-frame flag | `in_ECX[10]` |

## The flow (SE addresses)
1. **Prologue** `0x5c2af0`: bump `O+0x28`; dec `O+0x1c` (the cooldown) if >0.
2. **Leader** `0x5c2b29`: `leader = *(param_2+0x200c)`; its body `= leader+0xa14`; null → reset+ret 0.
3. **Can-enter** `0x5c2b53`: `O+0x24 = 1` iff the leader is grounded + near-still at the door
   (box check `0x56cc00` = base `FUN_0054e5c0`; velocity `|body+0x28| ≤ 0x5dc0`).
4. **Overlap scan** `0x5c2b9b`: walk the CHARACTER band `*(0x92dd38)+0x11e0` (128 slots, active =
   `+0x1d0!=0`), overlap test `0x5c3190` (base `FUN_0059a7c0`) → the door anchor `esi` → `O+0x08`.
   None → reset+ret 0.
5. **Resolve exit** `0x5c2c05`: `exit_key = anchor+0x274`; find the room-record exit slot
   (`*(*(0x92dd38)+0x1038)+0x1c`, stride 0xc, ≤0x14 slots) whose key matches → `local_4`.  Its `+4`
   = **TARGET ROOM**, `+8` = entry key.
6. **SEEN gate (bVar3)** `0x5c2c67`: scan the leader's seen-list `param_2+0x08` (stride 8, count
   `param_2+0x2008`) for an entry `==local_4[+4]` (**the target room / map id**) with `+4==1`.
   Hit → `O+0x10=1`.  **This is the unseen gate — keyed on the target MAP ID** (USER live: hijack a
   portal to an unseen map → the portal reads unseen; matches this scan exactly).  `999999`
   (`0xf423f`, the overworld sentinel) + `render_root[0x1030]==2` are special-cased at `0x5c2d53`.
7. **COMBAT gate (bVar4)** — proximity test `0x478980` (base `FUN_0047b7c0`) over the EFFECT band
   `+0x1160` (32 slots): SEEN path `0x5c2cae` uses 48000×20000; UNSEEN path `0x5c2d7c` uses
   56000×20000 + a party-slot scan (`param_2+0x4030`, 8) + the faction match `0x43bf70`.  Combat
   found → `[esp+0x14]=1`.
8. **Dispatch** `0x5c2ed7`:
   - `param_6` (forced) → set `O+0x18=2`, ret 0.
   - **`bVar4 (combat) → jmp 0x5c30b6`** (the HOLD RAMP).  ← the mobs-nearby path.
   - `O+0x1c (cooldown) > 0 → jmp 0x5c30bd` (into the ramp).
   - else (no combat, no cooldown): `iVar6 = anchor+0x278` valid-flag switch; `==1` → the GATE-1
     instant check `0x5c2f41`; the **changed-check** `0x5c2f5c cmp O[0],O[8]`: equal (standing) →
     `0x5c3039` (the not-changed SHORT-ramp path), else → `0x5c2f64` (the combat-ZONE scan +
     area-flags, then commit `0x5c301f`).
9. **The HOLD RAMP** `0x5c30b6`:
   - `0x5c30cf`: **`if bVar3(O+0x10)!=1 → 0x5c317d`** = the UNSEEN HARD BLOCK (`O+0x18=2`, ret 0).
   - `0x5c30f5`: **`if leader-input+0x114 (UP held) == 0 → 0x5c3134`** (decrement the ramp).  ←
     THE ANTI-SELF-FIRE GATE: the ramp only climbs while UP is held.
   - `0x5c30ff`: (UP held) area-flags → `O+0x20 += 60` (or 200), cap 10000.
   - `0x5c3144`: `if O+0x20 < 10000 → 0x5c3183` (ret 0); else → **`0x5c314e` FIRE**.
10. **`0x5c314e`** = the transition trigger: write `*param_3=target_room`, entry/exit keys, ret 1.

## The gates, summarized
- **INSTANT** (GATE 1): no combat, no cooldown, changed door, combat-ZONE clear + area-flags →
  commit `0x5c301f`.  **warpgate's existing patch** (`0x5c2f64`→`jmp 0x5c301f`) skips the zone-scan
  + flags for THIS path only.
- **HOLD** (seen + combat): hold UP, `O+0x20` ramps 0→10000 (~1–3 s) → fire.  ← the "hold prompt"
  the USER hits with mobs.  **warpgate's patch never runs here** (combat jumps to `0x5c30b6` at
  `0x5c2f0d`, before `0x5c2f64`).
- **HARD BLOCK** (unseen + combat): `0x5c317d` sets `O+0x18=2`, never fires until combat clears.

## warpgate v2 — the ROBUST fix (the right patch points)
The old warpgate only patched GATE 1, so it does nothing with mobs nearby.  The correct, self-fire-
safe interventions (all gated by the game's own UP-held check, so STANDING never fires):
1. **Keep** GATE-1: `0x5c2f64` → `jmp 0x5c301f` (no-combat instant).
2. **`0x5c30ff` → `jmp 0x5c314e`** (rel `e9 4a 00 00 00`): in the hold-ramp path, the instant UP is
   held (past the `0x5c30f5` anti-self-fire gate) fire the transition — skips the ramp.  This is
   strictly BETTER than the dropped `0x5c314c`→nop, which self-fired because it sat AFTER the
   decrement path merged (fired even when UP was NOT held).
3. **`0x5c30cf` `jne 0x5c317d` → `nop`×6**: let an UNSEEN door take the ramp path (→ instant via #2)
   instead of hard-blocking.  ⚠ same-area only — a cross-AREA unseen target still needs its W-map
   loaded (the `0x5ad656` "map data not found" crash), which is the warp-router's job, not this gate.

All three applied/removed together by `warpgate_set`, AUTO-GATED to a settled in-scene state
(`scene_present() + g_scene_settle`), same as before.  USER-VERIFIED live: instant enter with mobs
(hold-prompt flashes then skips); warped a whole dungeon of unseen maps; unseen+combat works.

## SHELVED (2026-07-18) — warpgate breaks SCRIPTED sequences → GAME-side crash (NOT a trainer bug)
Re-entering the "mystery dungeon" entrance (target map **999999** / `0xf423f`) after clearing the
dungeon **without the boss** (warpgate skipped the scripted sequence) CRASHES — reproducible, and
autoskip-OFF still crashes (autoskip exonerated).  The **crash-resilient trace TAIL is decisive**
(`f0000001` heartbeat = settle+scene_present, `f0000010` = god write, `f0000020` = warpgate set):
```
god-write (settle=0x4118, old scene)   ← god writing the VALID old leader
heartbeat (settle=0x411a)  [78ms stall]← the transition begins
heartbeat (settle=0x0001)              ← render-root CHANGED -> settle reset -> god GATED OFF
heartbeat×4 (settle 2..5)              ← NO god-writes (correctly off through the cutscene)
warpgate set apply=0 (remove)          ← last trainer action, COMPLETED
[crash]                                ← a few frames into the game's own cutscene/entrance script
```
⇒ the scene-state gating WORKED (god off + warpgate removed the instant the cutscene loaded; the
trainer is NOT writing during it).  `scene_present` stayed **1** the whole time (render-root went
old→new WITHOUT passing through 0), which is exactly why the **settle-on-root-CHANGE** gate catches
it and `scene_present` alone would not.  ROOT CAUSE: warpgate CREATED an inconsistent story state
(cleared without the boss / skipped the entrance script); the game faults on it later.  The tool is
correct; the *consequence* of the bypass is the problem.  Deferred robustness (USER: shelf): a
guard so warpgate won't bypass a **scripted/special** door (999999 is one signal; the boss/area-lock
`+0x3770`/`+0x3764` another) + RE what **999999** actually is (locked/scripted/mystery-dungeon
placeholder — NOT "overworld", USER-corrected: a weapon-shop door shows target 999999).

## OPEN (next session) — cross-region SINGLE-portal warp to ANY map (USER-set direction)
Goal: one portal jump to any map, not the current chained boundary-portal BFS (`warp.py`).  This is
the DIRECT WARP thread: SE_CODE_MAP "FORCED WARP" — stage `0x401d40`(target,return,exit) / commit
`0x402030` (→ map_obj `+0x4024/+0x4028/+0x402c`), the cross-area W-map load request `*0x92c828`
(`[req+4]` case-3 @`0x5ad586`), and a non-door TRIGGER, WITHOUT hitting the `0x5ad656` "map data not
found" fault (needs the target area's W-map resident first).  Cross-ref the base model
`FUN_0059ec30`/`FUN_0059f2c0`/`FUN_004c5350`.  Use the live trace harness (this session) to trace a
real cross-area boundary transition + diff it vs a same-area hop to see where the W-map load fires.
