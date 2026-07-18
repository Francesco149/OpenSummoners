# sotes_trainer — standalone EN-SE trainer (design + mechanics)

Goal: a **shippable, standalone trainer** for the retail EN Special Edition
(`sotes_en.exe`, Steam buildid 23890965, ImageBase `0x400000`) — rich enough to
set up any test scenario fast and skip the slog of getting there. NOT a Frida
script: Frida (`tools/ennse_trainer/`, `tools/ennse_voice/`) is **throwaway
probing scaffolding** used only to discover the mechanics recorded below.

## Architecture (target)

```
  sotes_en.exe (game process)
    └─ sotes_trainer.dll   (injected; in-process; NO Frida)
         ├─ build-sig validate  (fail loud on a different buildid)
         ├─ player-anchor hook   (ctor 0x419b00, code 0xc35a)
         ├─ 50ms tick thread     (freezes / locks / keep-awake)
         ├─ feature engine        (god / teleport / speed / noclip / spawn / warp / menu)
         └─ localhost JSON server (127.0.0.1:PORT, line-delimited JSON)
                ├── the OPTIONAL LLM interface  (agent connects, reads state, sends cmds)
                └── the UI backend
  trainer_ui  (separate; reads the JSON server; shows ALL tracked stats live + toggles)
```

- **In-process DLL**, not an external RPM/WPM tool: the anchor needs a ctor hook,
  several features need to call engine fns / write the per-frame input record, and
  menu/scene control is only *reliable* from inside (write the input record the game
  polls, vs racing external injection). Matches the project's shipped `version.dll`
  (`tools/ennse_voice/`) inject pattern + `build/inject.exe`
  (CreateProcess-SUSPENDED → remote LoadLibrary → Resume).
- **One JSON server, two consumers.** The UI and the LLM speak the same protocol.
  Line-delimited JSON over TCP (127.0.0.1). Every feature = one command; a
  once-per-frame state snapshot streams for the UI/agent to render.
- **UI** (later phase): a native window (ImGui/DX11, like `tools/osr_view`) or a
  served web page — either just renders the JSON snapshot + posts commands. Kept
  out of the DLL so the core ships headless-testable.

## Discovered mechanics (EN-SE, VA = fileoff + 0x400000)

> **The durable SE VA registry + base-game equivalence is `SE_CODE_MAP.md`** — read it before
> re-deriving any offset. Key rule proven there: base (old Steam `sotes.exe`) and SE are
> INDEPENDENT RECOMPILES — VAs do NOT transfer (bytes differ at every VA), only the MODEL
> (struct offsets, algorithm, constants) carries; find SE analogs by fingerprint, verify live.

### Player anchor + stats — VERIFIED live vs HUD (Lv3 / HP140 / MP34)
- ctor `0x419b00` (14-arg): entity code = arg6 @ `esp+0x18`; final code @ `actor+0x1d4`,
  handle @ `actor+0x1d8`. **Player = code `0xc35a`** (Arche, dramatist row 0).
- actor offsets: `world_x 0xc76c` (centi-px = px*100), `world_y 0xc770` (candidate —
  verify it moves on JUMP/fall not L/R), `stat_block ptr 0x760`.
- stat_block offsets: `hp_cur 0x54`, `hp_base 0x58`, `hp_equip 0x84`, `hp_buff 0x9c`
  (**max HP = 0x58+0x84+0x9c**, party.h), `mp_cur 0x5c`, `mp_base 0x60`, `mp_equip 0x88`,
  `mp_buff 0xa0`, `level 0xe0`. (base==sum when no gear; equip/buff read 0 for lvl-3 Arche.)
- monster code ranges (for spawn-force / one-hit filters): `0xc742..0xc83e`,
  `0x18744..0x1875d`. Sound-def table `0x65b0e8` (294×0x24).

### Mode / scene state signal (the "wait on memory" primitive)
- The engine runs ONE input-manager object at a time; it polls a fixed button-id set
  that identifies the mode:
  - **{2,4,34,37} = a MENU** (title screen AND save-slot picker both use this).
  - **{19} = the autoplay/attract DEMO** ("Press Any Key To Exit").
  - gameplay = other ids.
- Managers alternate ~**75 s title → ~65 s demo → repeat** (the attract cycle).
- To detect mode from the DLL: read the active input manager + its polled set (hook the
  input poll `0x437c70` / `0x4378d0`, or read the manager singleton). This is the clean
  gate for "act only when the title menu is up."

### Title / menu / load dispatch (decompile: FUN_00581ba0)
- dispatcher `FUN_00581ba0` picks the title routine: SE `0x582c40` unless
  `(DAT_0092af7c==0 || *(*DAT_0092af98+0x158)!=0)` → classic `0x584ac0`.
- title routine returns a code; dispatcher `switch`:
  - `0x1a` Start → `FUN_005a4a40` (scenario-select, when MD bank present) else new game.
  - `0x1c` **Continue** → `iVar=FUN_00585cf0(&tgt,&x,&y)`; if `iVar==0xc` →
    `FUN_005cb460(tgt,x,y)` transitions into the loaded scene.
  - `0x1d` Special/bonus → `FUN_0058b1f0(0)`.  `0x1e` Option → `FUN_005c6c30`.
- **load chain** inside `FUN_00585cf0`: builds the slot list, runs `FUN_005866c0`
  (list input loop: 0xb cancel / 0xc confirm / 0xd keep-looping); on confirm
  `handle=FUN_005e8e80()`, `slot=FUN_005e8ea0()`, `FUN_00416550(handle,slot,0,0)` = LOAD
  savedata, `FUN_00586c60(handle,slot)` = apply → returns `0xc`.
- **Direct-load option** for the trainer: reproduce that terminal sequence
  (`FUN_00416550` + `FUN_00586c60` + `FUN_005cb460`) with a chosen slot to skip the whole
  title/menu — RE the exact args live before relying on it.

### Attract (demo) — the "attract off" toggle
- The title idle counter is a **stack local** (`local_84`→`0x1193`) inside `0x582c40`; it
  forces the demo transition. NOT a global → can't freeze by poke.
- Options: (a) **byte-patch** the `0x1193 <` compare / demo-launch branch (need the
  instruction addr from disasm); (b) **keep-awake**: from the 50ms thread, while the menu
  manager ({2,4,34,37}) is active, reset the idle counter (nudge a menu input / write the
  local via the frame's stack) each ~N s.
- Menu ring ids: `4`=rotate+1, `37`=confirm, `34`=abort, `2`=? ; "first press = wake,
  eaten". Frida ring-inject only *wakes* reliably (confirm flaky) — the in-process trainer
  should instead **write the input record directly** or **poke the selection index +
  trigger confirm** (find the selection var live).

### Config / options object
- `*DAT_0092af98` = ptr→game/options object; `+0x24` = resolution/scale enum
  (0→640×480 default, 2→1280×960, 3→1920×1440). `+0x158` = the classic/SE title selector.
- Save files: `user/savedataNN.sdt` (slots 00-07 present).

## Feature roadmap (each: discover → verify live → encode in the DLL → expose as a cmd)

- **P0 core**: inject + build-validate; player-anchor ctor hook; 50ms tick thread;
  localhost line-JSON server; cmds `ping/player/read/write`.
- **P1 verified**: `god` (freeze HP/MP), `setstat`, `teleport` (write world_x/y),
  `tpnearest`. (offsets already verified.)
- **P2 get-into-scene**: mode read; `menu`/`load <slot>` (input-record write or direct
  load chain); `attract off`. Unblocks fast scenario setup — load the tower save w/ mobs.
- **P3 traversal**: `speed <mult>` (game-speed / frame limiter), `movespeed`, `noclip`.
- **P4 combat**: `onehit` (freeze enemy HP≈0), robust `god`.
- **P5 UI**: native/web panel over the JSON server showing all tracked stats + toggles.

## Next session — map introspection + fast travel (planned; USER-set 2026-07-18)
**LANDED 2026-07-18 — steps 2+3 (portal enumeration + the map GRAPH).** The `rooms` cmd now
enumerates the MASTER room table = ALL rooms (427 this save) + each room's exits
(`{key,area,scene,exits:[target,...]}`), i.e. the full cross-region portal GRAPH; the old bug
(returned only the current room) was the wrong table + stride — see SE_CODE_MAP thread #3 (✅):
the table stride is **0x158** and the master is a separate block found by a longest-valid-run scan.
BFS reaches 324/427 rooms (26 areas) from the tower via real portals; the rest cross via the
overworld (the `999999` exit sentinel).
**✅ LANDED 2026-07-18 — steps 4-6 (the warp EXECUTION) DONE, incl. CROSS-REGION.**  The USER
picked the door-enter route (not a direct-jump call — the transition fn `0x5a6010` is a 60 KB-frame
full rebuild, too heavy to call).  RE'd the SE keyboard subsystem (kb obj `*(0x92d5bc)`, immediate
buffer +0x18, buffered event queue +0x14) + baked the raw-DINPUT AUTO DOOR-ENTER (`doorenter`) +
movement (`hold`/`release`) into the DLL — the door-enter forces `buffered_read 0x5e2820` to return
success while injecting an UP press-event, so it fires foreground-independent (the missing piece).
`warp.py <room>` = the adaptive BFS router (per hop: same-area = hijack-all + teleport-sweep +
doorenter; cross-area = the real boundary portal, no hijack).  **VERIFIED cross-region: tower 440230
→ … → 420240 (a different AREA).**  See SE_CODE_MAP "Input / keyboard subsystem" + "BAKED +
CROSS-REGION WARP WORKS".  Still open: step 1's MOBS/NPCs; a warp SPEEDUP (read exit door positions
from the scene to skip the per-hop teleport-sweep).
The goal arc, in order (each step: RE → verify live → trainer cmd):
1. **Query the CURRENT map + its contents.**  Which map/scene we're in (id/name) + every object.  **The
   map STRUCTURE/props/portals are LARGELY DONE in the port** (USER — reuse, don't re-RE): `res_explorer`
   renders any map + its props, the port renders town/house/errands, `map_decode` + the 587e00 tile dispatch
   (100% cell coverage, quirks #119-121) + the region-E link anchors give the format.  So the map-DATA query
   = parse that (statically from the SE map resource, or live off the loaded scene) — cross-ref.  **The GAP
   (new RE): MOBS / NPCS + maps beyond the house.**  The port's CHARACTER band is only captured for town/
   house/errands (`ERRANDS_CAST`, `CHAR_BANK_DEFS`); mob/NPC spawn tables elsewhere are stubbed and no map
   past the house is rendered.  So live-read the SE actor pool for mobs/NPCs (monster codes `0xc742..`/
   `0x18744..` + NPC codes; anchor = the actor ctor `0x419b00`, already hooked) and RE the per-map spawn.
2. **Enumerate every PORTAL** (map-transition triggers / region links — the port's region-E link anchors,
   map codes 90010/90011, `FUN_0058cb30`, already RE'd → cross-ref).  Each portal → position + DESTINATION
   map (+ entry point).
3. **Query the maps reachable from here** (the portal destination set) → build a map GRAPH.
4. **Test a DIRECT map-change FIRST (USER — before building any pathfinding).**  Try triggering the
   transition STRAIGHT to a destination map (call the map/scene-transition fn with a target map id — the SE
   analog of the load-transition `0x5cb460`, or the map-change a portal itself invokes) and check it is
   WELL-BEHAVED enough for testing: does the destination load with party + state intact, correct spawn, no
   softlock / missing-flag break?  If a direct jump is clean, **fast-travel = just call it** and the portal
   graph + pathfind (5-6) become UNNECESSARY (or a fallback only for maps the direct jump can't reach cleanly).
5. **Pathfind map→map** (only if the direct jump breaks state portals set up): given a save's starting map +
   a target (searched by "what needs testing"), BFS the portal graph → the sequence of maps/portals to walk.
6. **Auto fast-travel:** drive it — teleport Arche to each portal (the phys-box teleport, already working)
   + auto-ENTER it (the transition trigger — UP-into-portal per the door model, or call the transition fn)
   → hop map-to-map to the destination.
Foundation in hand: teleport (`*(actor+0x40)` phys-box), the player anchor, **autoskip** (blast through any
dialogue en route), load/newgame (pick the start save).  Prereq RE: the map/scene singleton + object-pool
layout (SE offsets) and the portal record format.

## Standing PHILOSOPHY — proper RE over fragile SCANS (USER, 2026-07-18)
**Scanning the heap is a PROBING-stage tool only** (RE + finding addresses across versions later), NOT
a runtime mechanism.  The trainer currently full-heap-SCANS to find the player/party (`find_player`,
`scan_actor`, `tc_get_chars`, `god_freeze_party`) — every rd32 is a `VirtualQuery` syscall, so a
per-frame scan LAGS the game (the party-god freeze regression, 2026-07-18: an unconditional 4x/sec
scan + 3x `actor_valid`/frame; mitigated to scan-only-when-a-member-is-missing + a cached statblock,
but still a scan).  **Going forward: reach the player + party roster by a POINTER CHAIN / HOOK from a
known global**, not a scan — e.g. the leader controller's party slots (base door handler reads
`*(param_2+0x4030 + i*4)`, i<8 = the 8 party actor slots), or the render-root CHARACTER band, or a
party global.  RE it (next session, with the tracing harness below), cache the chain, revalidate cheap.
Reserve the scan for first-boot discovery + version-porting.

## ✅ LANDED 2026-07-18 — the SE LIVE TRACING HARNESS (USER-requested; VERIFIED live)
The SE analogue of the port's retail call-trace, BAKED + live-toggleable from the trainer socket.
Command (`raw` passthrough or `scratchpad/tr.py`): **`{"cmd":"trace","op":"on|off|dump|clear|pause|
resume|status","vas":[…],"max":N}`**.  `op:on` with `vas` (hex strings `"0x5c2af0"` or decimals)
MinHooks each VA with a **generic ENTRY thunk** + starts recording; `dump` drains the ring since the
last dump (`{s,t,va,ecx,edx,ret,a:[6 args]}` per call); `off` stops + unhooks; `clear` resets the read
cursor; `pause`/`resume` toggle recording without unhooking.
- **Mechanism (`trainer.c` "LIVE CALL-TRACE HARNESS"):** per-hooked-VA a raw-bytes thunk `pushad;
  pushfd; push esp; push va; call trace_entry_c; add esp,8; popfd; popad; jmp [tramp]`.  ENTRY-ONLY,
  so it's convention-agnostic + stack-safe: the thunk tail-JMPs to the MinHook trampoline, so the
  original runs its own `ret N` back to ITS caller (no return-addr juggling → cdecl/stdcall/thiscall
  all work without knowing the arg count).  The logger only reads the thunk's own stack frame (regs +
  raw arg values — no ptr deref), so a bad game pointer can't fault our hook.  Records → an 8192-slot
  ring the socket drains (atomic write index; a torn body is acceptable for a diagnostic).  MinHook's
  HDE copies WHOLE prologue instructions, so SEH-scope fns hook cleanly (the 6-byte `mov eax,fs:0x0`
  that broke the fixed-5-byte install_detour — VERIFIED: hooked `0x581ba0` crash-free).
- **VERIFIED live (title screen):** hooking the per-frame title-loop leaf **`0x5e57e0`** (called 2×/
  frame from `0x582c40`'s loop @ `0x5839ef`/`0x583a10`) captured ~133 rec/s with the two call-sites
  cleanly split by `ret` (`0x5839f4` vs `0x583a16`), ecx = the title obj, args stable.  Hooks install
  AND remove clean; game stays smooth.  (Per-frame title leaves for a liveness check: `0x5e57e0`/
  `0x5e57f0`.  Title fns that are ONE-SHOT loop-bodies — entry fires once — incl. `0x581ba0`,
  `0x582c40`; the gameplay input refresh `0x497050` is TITLE-COLD.)
- **v2 (not yet built):** return-value capture (needs a per-thread shadow-stack of the caller retaddr)
  + guarded per-VA FIELD snapshots (the `retail_fields.json` analog — for "which fields RAMP", e.g. the
  door hold-ramp `in_ECX[6]`).  Entry inputs already answer "which fn fired, order, ecx/args".
- **Next USER goals for the harness** (now UNBLOCKED): (1) trace the door-hold code path (`0x5c2af0`
  the door-USE handler + its callees `0x4393d0`/`0x4495b0`/`0x4495f0`; see SE_CODE_MAP "GATES"); (2)
  DIFF a cross-region vs a same-area door (where the W-map load diverges); (3) crack the DIRECT WARP
  (stage/commit `0x401d40`/`0x402030` + the edge-trigger).  Also feeds the RE-over-scan cleanup (find
  the party roster by a traced pointer-chain, not a scan) + the regression triage (load→newgame,
  instant-doors) noted 2026-07-18.  This is general RE infra — every traced call grows `SE_CODE_MAP`.

## Live findings — real tower save (Arche Lv17), 2026-07-17 (SESSION 2 — RESOLVED)

Probing the REAL loaded save (Archmage Tower) resolved the two bugs the demo hid.
Method: HW write-watchpoint (frida-17 `thread.setHardwareWatchpoint`, instance method,
per-thread — arm ALL threads) on `world_x`, then disasm the committing instruction.

- **TELEPORT — SOLVED (write the phys-box, not the snapshot).** `world_x 0xc76c` /
  `world_y 0xc770` are a per-frame **DERIVED snapshot**, re-committed every frame (even
  idle) from the actor's collision AABB. The commit is at **VA 0x484554** (`__thiscall`,
  `ret 4`; ecx=actor, ebx=box):
  ```
  0x484554  mov eax,[ebx+4]           ; world_x = box[+4]            (direct copy)
  0x484557  mov [ecx+0xc76c],eax
  0x48455d  mov eax,[ebx+8]           ; world_y = box[+8]+box[+0x10]-1
  0x484560  mov edx,[ebx+0x10]        ;         = top + height - 1   (= bottom edge)
  0x484563  lea eax,[eax+edx-1]
  0x484567  mov [ecx+0xc770],eax
  ```
  The AABB: `box+0` tag, `box+4`=left X (=world_x), `box+8`=top Y, `box+0xc`=width,
  `box+0x10`=height. **The box ptr is `*(actor+0x40)`** (the sole process-wide reference;
  stable while idle). PROVEN LIVE: writing `box[+4] += 30000` moved Arche +300px and it
  **STUCK** (world_x followed next frame, no snap-back). `box[+8]` (top Y) is authoritative
  too but **gravity-settled** — writing it up made her rise then fall back with an accel
  curve (physics, not a reset bug), so Y teleport lands on the ground below the target.
  IMPLEMENTED: `teleport` now does `box=*(actor+0x40)`, `box[+4]=x`, `box[+8]=y-h+1`.
- **`level` — `stat_block+0xe0` is the MAX COMBAT LEVEL (USER, live SE), NOT the display level.**
  Poking `+0xe0` moves the "N" in the stat window's "combat level M/N" and the HUD element STARS
  (3 lit of 5 → 3 of 21 when poked to 21); the demo's "Lv3" match was a coincidence.  The port RE'd
  the displayed char level as `party_stat_level = +0xe0 + +0xd8` (combat_level_max + level_bonus,
  `494e60:123`; = 1 for Arche Lv1) — the strongest lead for the true display level (below).
  17 is absent as a u32/u16/u8 anywhere in the stat block, the actor, or near the player
  handle (58 handle refs scanned). But `sb+0xf0 = 50000` == **exactly Arche's level-17
  `exp_max`** in the base-stat table (`{0xc35a,17,290,52,{68,60,38,44},50000}`), and
  `sb+0xec = 23167` = exp progress ⇒ the char IS Lv17, derived from EXP. TRAINER: expose
  `exp_cur (0xec)` + `exp_max (0xf0)` + `combat_level_max (0xe0)`; the true display level needs a
  table lookup (SE exp table not yet dumped) or RE of the HUD level-drawer's read. (Two
  fields sum to 17 — `+0xdc`=3 + `+0xe4`=14 — but that's unconfirmed; NOT shipped.)
- **Anchor + stats correct.** ONE live `0xc35a` actor. hp/mp read + god/setstat write work.
  hp_max=301 = `+0x58`(286)+`+0x84`(0)+`+0x9c`(15) — the 3-term sum, confirmed.
- **Mob scan** (loose scanner `scratchpad/posfind.py scanmobs`): monster-code actors
  (`0xc742..0xc83e` / `0x18744..0x1875d`) do exist in memory (e.g. code 0xc746 @ wx=34024,
  0xc829 @ wx=25600) but NONE near Arche in the current idle scene, and their box-link
  (`box[+4]==world_x`) does NOT validate like the player's (mob box relationship TBD).
  `tpnearest`/`listmobs` DEFERRED — needs a scene with live near-player mobs + the mob
  box/pos model. The DESIGN's earlier "51052 @574px" was last session's (now-changed) state.

## LOAD VERIFIED END-TO-END — menu-drive loads the tower save (2026-07-17 session 3b)

Proven LIVE (trainer DLL injected via `scratchpad/trainer_test.py`): the Archmage's Tower
save (slot 8, Arche/Sana/Stella **Lv17**) LOADS and the game enters it — HUD "Arche Lvl 17
301/301 HP 62/62 MP", party+gold, tower interior on screen; trainer `player` = hp301/mp62/
**exp_max 50000** (= Arche Lv17).  The winning path is **MENU-DRIVE**, not the direct call.

**IMPLEMENTED + VERIFIED in the trainer:** `{"cmd":"load"}` now drives it autonomously — one
command freezes the attract, injects confirm at the title (0x437c70 ring) → Continue → the
picker, then re-injects confirm at the picker (0x4378d0 ring) until the load fires; returns
`{loaded:true, player:{…Lv17…}}`.  Two inline hooks (0x437c70 + 0x4378d0), each capturing its
poll esp+manager so the record is timed to the poll's own `now`; `inject_record` writes
{+0=id,+4=now,+8=1} into `mgr[0xc+slot*4]` (63=first-polled), exactly the 0x437c70 contract.
**TEST CAVEAT:** `repro_agent.js` ALSO frida-hooks 0x437c70 AND 0x4378d0 (its press/menu_press),
which COLLIDES with the trainer's inline hooks (reads frida's patched prologue → crash).  Test
the trainer's menu-drive with `OSS_NO_REPRO=1` (no repro_agent; verify headlessly via `player`).
A shipped trainer won't have frida on those VAs, so no collision.  The picker confirm is
re-injected every poll (a single early confirm is dropped before the cursor settles).

### The working recipe (input injection through the game's own menus)
1. At the TITLE (polls `0x437c70`): inject **confirm=37** → the DEFAULT selection is
   **Continue** → opens the save-slot picker.  (title input = repro `seq`/`presswhen` /
   trainer 0x437c70 ring — WORKS; the earlier "title won't respond" was the demo, not a bug.)
2. The slot PICKER (polls `0x4378d0`, the GENERIC controller — NOT 0x437c70): inject
   **confirm=37** via the 0x4378d0 path (repro `menu_confirm`).  The picker DEFAULT-highlights
   the **newest** save = slot 8 = Archmage's Tower ⇒ tower load = **confirm + menu_confirm**,
   no navigation.  (Arbitrary slot N needs picker rotate injection — TODO.)
3. Detect success headlessly: trainer `player` returns a valid 0xc35a actor with the save's
   stats (or `game_state` shows a scene).  The transition is deferred (~a few frames).

### Ground-truth terminal chain (frida-observed on the real Continue-load)
- enumeration (building the list): `416550(tmp, 0x2738, id=0x46..0x4f, 0x1428c4, 1)` per row.
- TERMINAL load of the chosen row (list index **7** = slot 8): `416550(S, 0x2738, **7**, 0, 0)`
  → `586c60(0x92ac68, 0x2738, **7**)` → `5cb460(this=**picker dispatcher** 0x6b0181f, 0x2738,
  **7**, 0)`.  So the "slot" is the **0-based picker list index**, and 586c60/5cb460's extra
  arg is that same index.
- **Why the direct call crashed (`enter=1`):** the load(416550) + apply(586c60) args were
  RIGHT (matched ground truth), but the transition `5cb460` needs the **PICKER's** dispatcher
  `this` + arg2=list-index — NOT the title's `*0x92dd4c` + 0 that the trainer passed.  The
  picker `this` only exists after navigating to the picker ⇒ the direct call from the title is
  impractical.  `load` (direct) is kept EXPERIMENTAL (`enter=1`); the real `load` is the
  menu-drive.  Verified: attract-freeze, the 0x437c70 safepoint, engine-thread calls, load
  (416550 ret=1) and apply all work standalone; only the standalone transition is unreachable.

## Menu / load / input — FULLY RE'd + attract freeze (2026-07-17 session 3)

Session-2's "load chain is STATEFUL / can't call it / active-mgr is the OPEN piece / +0 is a
category" is **SUPERSEDED** — the whole chain + input contract + attract trigger are now RE'd
off the EN-SE exe (offline `objdump -M intel`; live `repro_probe.py` for mode/screens).
Uniform mapping: **VA = fileoff + 0x400000** for .text/.rdata/.data alike (verified vs section
headers), so any VA disassembles/reads straight from the file at `off = VA-0x400000`.

### Direct-load chain — THE RECIPE (bypasses the menu AND the attract entirely)
Reproduces the Continue-confirm terminal (`FUN_00585cf0`, the code-`0xc` branch @0x585f16). To
load main-quest save slot N:
1. `S = FUN_005ef121(0xea94)` — engine-alloc the ~60 KB save struct; zero it (585cf0 memsets
   +0x804.. via `rep stos`; a zeroed VirtualAlloc buffer is equivalent for the read path).
2. `FUN_00416550(this=S, handle=0x2738, slot=N, 0, 0)` — reads `user\savedataNN.sdt` into S;
   returns nonzero on success (thiscall, `ret 0xc`).
3. `FUN_00586c60(this=DAT_0092ac68, handle=0x2738)` — applies S into the game-state singleton
   (thiscall, `ret 8`).
4. dispatcher `FUN_00581ba0` then calls `FUN_005cb460(tgt=0x2738, x=0, y=0)` — scene transition
   into the loaded game.  (585cf0 writes `*out_tgt=handle`, `*out_x=0`, returns `0xc`; the
   caller does the 5cb460.)  S is transient (freed @58615a `FUN_00580e40(S,1)`); the loaded
   state lives in the singleton after apply.

**`handle` is a save-CATEGORY enum, not a per-save id — PROVEN by the code (not guessed):**
416550 switches arg1(handle) → filename FORMAT: `1`→`savedata%02d.bak`(0x8c66d0),
**default→`savedata%02d.sdt`(0x8c66e4)**, `0x2724`/`0x272e`→special (no sprintf, ebx=0/1).
586c60 acts ONLY if handle==`0x2738` (`cmp eax,0x2738; jne ret`).  The UNIQUE value that both
reads `.sdt` (=default branch, not 1/0x2724/0x272e) AND applies (==0x2738) is **`0x2738` = Main
Quest** (cf. strings "Main Quest" @0x9225fc / "Bonus Quest" @0x9225d4; 0x2724/0x272e = the
bonus/"Disk" saves).  `slot` = the numeric savedataNN index formatted into the filename.

getters (menu path, ref): `FUN_005e8e80(mgr)`=handle=`*( *(mgr+0x17c) + (*(*(mgr+0x174)+0x14))*0x10 + 0 )`;
`FUN_005e8ea0`= same `+4` = slot.  Menu slot-list = 16-byte entries `{handle@0, slot@4}` at
`mgr+0x17c`; selected idx = `*(mgr+0x174)+0x14`.  `FUN_005e8ec0(i)` = handle for an arbitrary i.

**ENGINE-THREAD SAFETY (mandatory):** 416550/586c60/5cb460 do file IO + singleton mutation +
DirectDraw scene teardown → they MUST run on the ENGINE thread at a per-frame safepoint, NOT the
trainer's socket thread.  Encode `load` as: queue the request → a safepoint hook (hook 0x437c70
inputPoll, the trainer analogue of repro's inputPoll-drained soundtest/spawn) drains + runs the
3 calls.  The bare off-thread `call` primitive is UNSAFE for these (fine for pure getters).

### Input-record contract — `FUN_00437c70` (the reliable menu-drive), FULLY RE'd
`FUN_00437c70(ecx=mgr, arg1=now, arg2=button_id)` scans a 64-slot ring, returns 1 (+clears
record[+0]) on hit:
- ring = `mgr[0x0c .. 0x108]` = 64 record-POINTERS; polled i=0x3f→0 (mgr+0x108 down to mgr+0xc).
- MATCH: `record[+0]==arg2(button_id) && record[+8]==1 && (now - record[+4]) <= 0x64` (100-unit
  window).  record layout: **`+0`=button id, `+4`=timestamp, `+8`=state(=1)**.  (Session-2's
  "+0 = category 0/2" was WRONG — +0 IS the polled button id.)
- INJECT = write a record `{+0=id, +4=now, +8=1}` then `mgr[0x0c + slot*4] = &record` (slot 63 =
  first polled).  This is EXACTLY what repro's `injectInputRecord` / `press` / `sequence`
  (presswhen) do — the contract is satisfied; the mechanism is correct.

**Active-mgr discovery — SOLVED** (was session-2's OPEN piece): the active mgr = the `ecx` of
every 0x437c70 call.  Live: `status.input_manager` (repro captures it).  Trainer: hook 0x437c70
(or read the global 0x582c40 loads it from) — no player actor needed at the menu.

**Which primitive where:** the TITLE (`FUN_00582c40`) polls its buttons via **0x437c70
DIRECTLY** → drive with inputPoll-injection (`press`/`sequence`), **NOT `menu_press`**.
`menu_press` hooks `0x4378d0` (the GENERIC controller) → that's for other menus (in-game /
likely the save-slot picker).  Title poll set = **{2,4,34,37}** (34 polled 2×/frame),
4/2=rotate, 37=confirm, 34=abort; demo/attract poll = **{19}** ("hit any key").

### Attract (demo) — FREEZE it (idle→demo trigger LOCATED)
`FUN_00582c40` idle counter = stack local `[esp+0x4c]`; inc'd each frame @0x5839d9 (`inc eax`,
capped 0x1194).  Trigger @0x583866: `jl 0x5832e1` (counter<0x1194 → keep looping); ELSE falls
through to `FUN_00408150(DAT_0092ac68,0,0,0,0,1)` @0x58387b = **LAUNCH DEMO**.  (DESIGN's
"0x1193" was off-by-one — it is **0x1194** = 4500 frames ≈ 75 s.)
**FREEZE** = patch @0x583866 `0f 8c 75 fa ff ff`(jl) → `e9 76 fa ff ff 90`(jmp+nop): the demo
call is unreachable, title stays up forever.  (Needs a `.text` write with `Memory.protect` RWX
first — repro's `write`/`typedWrite` does NOT protect; the trainer would `VirtualProtect` then
patch.)  Live-proven: repro `press[19]` DOES exit the demo (flaky only under the backlog below).

### Live-drive tooling note (repro_probe.py --interactive)
Safe rig (sandbox EN-SE, exact-pid kill), but a single-threaded stdin loop: slow commands
(`sequence`/`shot` with multi-second timeouts) **BACKLOG** it → responses lag commands by whole
batches (the misalignment that ate session-3's live turns).  Clean protocol next time: short
timeouts, ONE action per batch, FULLY drain resp before reading (or spawn per-experiment).  The
real live LOAD verification wants the **trainer DLL** (mem + a safepoint call), not the
sound-probe.  Driver used this session: `scratchpad/mdrive.py` (file-queue over repro stdin).

PREREQS (still current):
- Unpacked SE exe `vendor/unpacked/editions/sotes-ense-en.exe`; saves
  `…/steamapps/common/sotes/user/savedataNN.sdt` (00-07 present; newest savedata07 @2026-07-16;
  repro `--copy-saves` copies them into the sandbox).  Offline disasm helper: slice the exe at
  `off=VA-0x400000` → `i686-w64-mingw32-objdump -D -b binary -m i386 -M intel --adjust-vma=VA`.
- Frida-17: `ptr.readU32()` (not `Memory.readU32`); HW watchpoints are per-thread INSTANCE
  methods, arm ALL threads.

## Save-file format + know-what-each-save-is + any-slot load — session 4 (2026-07-17)

### The `.sdt` container — FULLY RE'd + shipped as a standalone lib (`tools/sotes_save/`)
The whole savedataNN.sdt codec is cracked (full byte-level RE in
`docs/findings/sdt-save-format.md`; verified 8/8 real saves).  Header = plaintext
`[u32 len=0x10][magic=0x2711][u32 bodysize][u32 val3][u32 seed]`; body = obfuscated by a
**subtract-key + inverse-permutation substitution** (loader `FUN_00416550` → archive
`FUN_005dee40` @0x5def06): `key=(seed>>8)&0xff`, `plain[i]=INV_KEYSTR[(cipher[i]-key)&0xff]`,
where INV_KEYSTR inverts the 256-byte permutation @**0x5fd290** (built by `FUN_005df030`).
Decoded body = a record stream (record 0 = 604-byte metadata; magic@meta+0x22c, category
handle@+0x230 == 0x2738 Main Quest).  Party roster = scan for codes 0xc35a/b/c (Arche/Sana/
Stella); `combat_level_max` @code+4.  A fixed-604 metadata block ⇒ a 16-u32 party-header grid always
at body 0x260 (fields grow over a playthrough — gold/playtime/progress candidates, exposed RAW,
UNLABELED: not pinned, and we don't ship guessed names).

**`tools/sotes_save/`** — dep-free (libc-only) `sotes_save.{h,c}` + `sotes_save_dump` CLI
(headless: dumps every real save; my verification tool).  REUSABLE outside the trainer — a
save editor, the port's save subsystem.  Both build in `tools/ci/build_all.sh`.

### Trainer commands (know what each save is)
The trainer compiles `sotes_save` in and reads `<exedir>\user\savedataNN.sdt` directly (no
engine load needed to identify a save):
- `saves` → every present slot's `{valid,handle,party:[{name,code,combat_level_max}],file_size,
  header_grid}`.  The agent/UI picks a slot from this, then `load`s it.
- `saveinfo {slot}` → one slot, same shape.

### `load {slot:N}` — arbitrary-slot load (VERIFIED live)
Picker selection getters `FUN_005e8e80`/`0x5e8ea0`: the save-list MANAGER holds the slot list
at `*(mgr+0x17c)` (0x10-byte entries `{handle@0, slot@4}`, slot = the savedataNN index) and the
selected index at `*( *(mgr+0x174) + 0x14 )`.  **CORRECTION (was wrong): the MANAGER is NOT the
0x4378d0 controller `ecx` — it is the controller's arg1 (poll `esp+4`).**  The controller `ecx`'s
`+0x17c` is garbage (found live: `s0`=huge).  `picker_select_slot` probes `ecx` + stack args 1..5
for the object whose `+0x17c` list's first entry has a small slot id, then writes the index of the
entry with `slot==N`; no match ⇒ safe no-op ⇒ the default (newest) highlight loads.

**VERIFIED live** (inject.exe → `sotes-ense-en.exe`): `slot:1` → the loaded actor is HP134 / Lv-
base3 / exp 117-1000; `slot:6` → HP235/293 / exp 18406-37000; `slot:7`/default → HP301 / Lv-base5
/ exp 20720-50000 — **each == the value `sotes_save` decodes from that file** (double proof: the
right slot loads AND the decoder's exp/level offsets are correct: exp_cur=body[code−0x10],
exp_max=[code−0xc], combat_level_max=[code+4]).

Two robustness fixes were needed and made (both live-verified):
- **Robust title-confirm.**  The single-shot title confirm (state 1→2 after ONE inject) was
  sometimes dropped before the title menu settled ⇒ the picker never opened ⇒ hang.  Now
  `poll_title_cb` RE-injects the confirm every poll until the picker opens (its poll 0x4378d0
  sets `g_pk_mgr`; the title only polls 0x437c70, so `g_pk_mgr`==0 until then), then advances.
- **Default boot behaviors** (keepalive thread): keep-active (`WM_ACTIVATEAPP(TRUE)`, no focus
  steal — the game pauses unfocused otherwise), attract-off (freeze the demo trigger so the title
  stays up), launcher dismiss (click `#32770` in-process).  Without keep-active the game froze
  while its window was backgrounded; without attract-off it cycled to the demo mid-drive.

## New game + dialogue-skip — session 5 (2026-07-17)

### `newgame` — VERIFIED
From the title, rotate to the "New Game"/Start item then confirm (the title's own menu).  The
default title selection is Continue; New Game is one rotate UP: `newgame {btn:2, to:1}` starts a
fresh game (VERIFIED: fresh Lv1 Arche, HP100/MP30, exp 0/250 at the intro spawn 41600,51199).
`btn` = the title rotate id (2=up / 4=down), `to` = rotations from Continue.  Drive: poll_title_cb
injects `to` rotates (one every `g_ng_gap` polls) then re-injects confirm until the scene leaves
the title; the command returns when `find_player` sees the fresh actor.

### Dialogue-skip — the button model (partly solved; fast-reveal is WIP)
The retail dialogue is a state machine: the ADVANCE-poll (base `FUN_0043b980`) matches ring ids
**0x24/0x27** and moves to the next box; the TYPEWRITER step (base `FUN_0043bca0`) matches 2/4.
Ground truth from live testing:
- **0x24/0x27 are dialogue-only** (inert in gameplay + menus).  Injecting them every gameplay poll
  AUTO-ADVANCES each box hands-free with NO re-trigger — this is `dlgskip`'s shipped default.
- **The reveal fast-forward** (what "pressing Enter" does — instantly finish the typewriter) is NOT
  0x24/0x27; it's a button that ALSO maps to a WORLD input.  Injecting it fast-skips the reveal but
  re-fires world interactions — e.g. it walked Arche into the house-exit door, re-triggering the
  "Dad needs your help" story gate in a loop (that gate triggers on UP-into-the-door, per the USER).
- A **consumption-gate** (watch whether the injected 0x24 probe gets cleared → a dialogue is up)
  was built to fire the reveal-skip only during dialogue, but 0x24 is consumed in the WAITING state
  (not while typing) and has a 1-poll lag, so it both mistimes the skip AND leaks one world-input on
  close.  So button injection can't cleanly fast-forward the reveal.

### Dialogue-skip — PASSIVE GATE (session 6; the SE dialogue-box object FOUND)
The consumption-gate above is SUPERSEDED.  It INJECTED to *detect* (a 0x24 probe every poll) and
lagged 1 poll — so it ran in freeroam and leaked a world input on close, AUTO-TRIGGERING the door.
Now dlgskip PASSIVELY reads the dialogue widget and injects ONLY while a box is on screen: freeroam
⇒ nothing injected ⇒ the door can't be auto-triggered (the USER fix: distinguish an already-open
box by READING it, act only if open).

SE dialogue widget = embedded in the input manager (`g_ti_mgr` = `*(*(actor+0xc7a4))`):
- **+0x374** = first content-cell ptr — **0 = no box**, 8 cell ptrs when a box exists (open/opening/
  popping-out).  This is `dialogue_box_open()`'s test.
- **+0x3a4** = pop-in scale (0..1000; 1000 = fully open), **+0x3a8** = scale step 50.
- box object = `*(mgr+0x374)`, parity layout `+0x4` active / `+0x54` scale.

RE'd LIVE (`scratchpad/diffdump.py`: a freeroam↔dialogue differential off the input mgr) + the
build-independent body-text color constants **0x3e537d/0xa8b9cc**, which located the SE dialogue
text code at **0x5e6668** in `vendor/unpacked/editions/sotes-ense-en.exe`.  NOTE the SE binary has
its OWN layout — parity VAs (0x439690 etc.) do NOT map; only shared CONTENT constants + struct
offsets carry over (function VAs coincide by luck, e.g. 0x43b980 is collision code here).
`state` now reports `box_open`/`box_scale`.

**Still WIP — PORT-DEBT(dlgskip-reveal-ui):** advance is still the 0x24/0x27 ring inject (dialogue-
only, now gated).  Per USER "forcing UI state to advance the dialogue is better than forcing input
state", the reveal-skip + advance should be a UI-STATE write on the box, not a button.  Two facts
to build on: (1) the DOOR prompt does NOT respond to 0x24/0x27 (a special choice on the same
widget) — verified live; (2) pinning the reveal/advance FIELD needs a NORMAL-dialogue TYPING-state
capture (reveal < total), which the waiting-state capture can't show.
**Scene-settle debounce (session 6b) — fixes a PRE-EXISTING newgame crash.**  dlgskip-ON through
`newgame` crashed the game DURING the title→scene transition.  PROVEN pre-existing (not the passive
gate): rebuilt commit e92abf9 (the old probe-consumption dlgskip) and it crashes IDENTICALLY;
dlgskip-OFF newgame survives in both.  Cause: dlgskip resumes the instant the menu-drive
(`g_ng_state`) clears and touches the freshly-loaded scene's dialogue widget before it settles →
fault.  FIX: `DLG_SETTLE_POLLS`=120 (~2s) — hold dlgskip off for a beat after ANY drive
(`g_md_state`/`g_ng_state`).  VERIFIED: newgame + dlgskip-ON now SURVIVES (fresh Arche 41600,51199,
box_open stays false, no loop, game alive through the whole test).  (The USER reached the door with
the old build in their env — this crash is timing/env-sensitive, but the debounce removes it.)
Commands for this arc: `newgame`, `dlgskip` (now passive-gated + settle-debounced), `dlgbtns`,
`press` (id probe), `state` (+box_open/box_scale), `keepactive`, `dlgtrace` (RE aid).

### Fast-skip (skip the TYPEWRITER) — RE'd; blocked on a SAFE grid capture (session 6c)
"dlgskip isn't fast": it ADVANCES boxes (0x24/0x27) but does not skip the typewriter (that is the
INTERACT key = the port's `dialogue_skip_reveal`, i.e. `reveal = total`).  Clean fix = force
reveal=total via a UI-STATE write (no keypress → can't trip the door; the USER's door=UP, skip=
INTERACT are different keys, but a UI write avoids ALL world input).
FOUND — the SE dialogue body TEXT-GRID ctor **`0x5e59c0`** (thiscall, `ecx`=grid; reached from the
body colors `0x3e537d`/`0xa8b9cc` @`0x5e6668`, ctor call @`0x5e5982`).  Grid layout: **+0x4c** =
char total, **+0x48** = per-char cell array, **+0x4** = active, **+0x8** = REVEAL counter (init 0,
climbs to +0x4c).  Shared by intro + in-game dialogue.  ⇒ fast-skip = while a grid is active +
`+0x8 < +0x4c`, write `grid+0x8 = grid+0x4c`.
BLOCKER — capturing the grid `this` safely.  Hooking `0x5e59c0` **CRASHES newgame**: it is an
SEH-scope function (prologue installs an exception frame, handler `0x5fcc0b`); the prologue-
relocating detour breaks its unwind and the intro (which throws/catches during scene build) faults.
PROVEN: adding the ctor hook crashes newgame even with dlgskip OFF; removing it restores newgame.
TWO DIALOGUE PATHS — in-game (door/NPC) dialogue is on the input-mgr widget (`g_ti_mgr+0x374`,
mapped + passive-gated); the INTRO/cutscene dialogue is on a DIFFERENT object (the tracer saw 0
frames + scans found no box on `g_ti_mgr` during the intro — that is also why dlgskip stalls in the
intro).  A shared grid capture would cover both.
NEXT (safe grid capture, no SEH detour): (a) hook the per-tick typewriter STEPPER that increments
`grid+0x8` (likely a small non-SEH fn) instead of the ctor; (b) reach the grid via a pointer chain —
the ctor caller `0x5e5982` has `grid = *(edi + *(esi))`, `esi` = the grid manager (find esi's
global); or (c) in-game-only via the box cells (`g_ti_mgr+0x374` → the text-grid cell).  Infra
ready: the `dlgtrace` cmd + the `g_dlg_grid` tracer (idle until a safe hook/chain sets `g_dlg_grid`).
`PORT-DEBT(dlgskip-reveal-ui)`.

**Update (session 6c, cont.) — the walls, mapped.**  Pursuing the in-game path surfaced that the
dialogue system has (at least) TWO renderers:
- **Input-mgr widget** `*(input_mgr+0x374)` (scale +0x3a4) — this is the **special DOOR PROMPT**
  (the box the USER first triggered; it does NOT advance on 0x24/0x27).  The passive gate + the
  `box_open`/`box_scale` diagnostics key on THIS.  It is what makes freeroam inject-free (door fix),
  but it is NOT where normal story dialogue lives — so dlgskip does not auto-advance those.
- **Cutscene / story dialogue** (the intro AND "Dad" NPC dialogue, initiated by interact) — a
  SEPARATE object, NOT on `input_mgr+0x374` (verified live: box_open=false, +0x374 holds unrelated
  "options" ASCII while Dad's box is up).  Its text grid (built by the shared 0x5e59c0, body color
  0x3e537d) was NOT reachable by scanning `scene_this`(*0x92dd4c) / `game_opts`(*0x92af98) within 2
  pointer levels, nor by the cell body-color signature.
Both 0x5e59c0 AND its caller 0x5e5890 are **SEH-scope functions** (`mov %esp,%fs:0x0` prologues) —
the trainer's prologue-relocating `install_detour` breaks their unwind → newgame crash.  So the
clean-fix needs NEW trainer infra before it can proceed: (1) a NON-prologue-relocating hook (a
mid-function/trampoline-free detour, or a VEH / HW-breakpoint capture) to grab the grid `this`
safely; or (2) a memory-SCAN command (like find_player) to locate the active grid by the 0x3e537d
cell-color signature + walk to grid+0x8/+0x4c; then force reveal=total.  The two shipped fixes
(passive gate, settle debounce) are unaffected; the fast-skip is DEFERRED pending that infra.

## Session 7 (2026-07-17) — debug-infra: scanner works, VEH is a dead end, MinHook is the path

Built the "scan for the grid, then debug the game to find the accessor code" infra the USER asked
for.  Results (all live-verified on the SE exe):

- **install_detour's REAL bug (RE'd — it is NOT "SEH unwind").**  install_detour copies a FIXED
  5 bytes; 0x5e59c0's first insn is the **6-byte** `mov eax,fs:0x0` (`64 a1 00 00 00 00`).  Saving
  5 SPLITS it: the trampoline runs a truncated `mov` that eats the jmp-back opcode as its disp32 →
  corrupted control flow (the "SEH crash" — WER put the fault mid-instruction).  The title/picker
  hooks work only because their first 5 bytes are whole insns (`53 8b 5c 24 0c`).  ⇒ the fix is a
  **length-disassembler** hook, not "avoid SEH-scope functions".
- **VEH / HW-breakpoint engine = DEAD END here.**  A registered vectored exception handler CRASHES
  the newgame intro even doing nothing (returns `CONTINUE_SEARCH`) — the intro scene build is
  C++/SEH-exception-heavy and a process-wide VEH perturbs it.  PROVEN by bisection: newgame crashes
  with the VEH registered + NO bp armed; the committed build's newgame is fine.  The engine code is
  kept but gated behind `OSS_TRAINER_ENABLE_VEH` (default OFF) — do NOT rely on it / on HW watch.
- **The newgame crash under dlgskip-ON is a PRE-EXISTING race, not the new code.**  A larger DLL
  perturbs the settle-debounce timing so the pre-existing dlgskip/intro-transition race fires.
  dlgskip-OFF newgame survives (both builds — bisected).  Work around: keep dlgskip OFF across the
  title→scene transition; the robust fix is to not touch the widget until the scene fully settles.
  (Also: the intro's early **voiced** lines auto-advance themselves — dlgskip was never skipping
  those; the real target is WAITING dialogue, e.g. Dad / the "new house" cutscene box.)
- **Scanner (`scan`/`dlggrid`/`grid`) — shipped, WORKS on a TYPING grid** (active==1): found the
  intro grid (rev=6/512).  MISSES a WAITING/instant box (active!=1 when done typing).  Two caveats
  that make raw scanning fragile: (1) the **DirectDraw framebuffer** is a big volatile RW private
  region whose pixels are rewritten every frame → a value-scan matches framebuffer NOISE (e.g.
  0x06fac744 matched every value probed); verify hits by re-reading for stability.  (2) **+0x4c
  (512) is the cell-array CAPACITY, NOT the per-line text length** — `reveal` (+0x8) climbs to a
  per-line textlen field (TBD), so fast-skip must force `reveal = <textlen field>`, NOT +0x4c.
  `fastskip` is therefore WIP (default OFF) pending that field + a robust capture.
- **Cell layout (RE'd live):** a cell = `[glyph coverage bitmap (ascending alpha bytes)][color
  palette: +0x00 body 0x3e537d, +0x04 shadow 0xa8b9cc, +0x08.. more]`, ~0x1b0 B; the grid holds the
  cell array at +0x48.
- **THE PATH FORWARD = MinHook** (USER suggestion; `TsudaKageyu/minhook`, BSD-2, MinGW-buildable,
  uses the HDE length disassembler → copies WHOLE instructions, fixing the 6-byte-prologue bug for
  good).  Plan: vendor it, `MH_CreateHook(0x5e59c0, detour, &orig)` with a **naked-jmp** detour
  (`mov ecx→g_dlg_grid; jmp *orig` — convention-agnostic, no thiscall double-`ret N` cleanup),
  `MH_EnableHook`.  Captures EVERY grid at build time (typing OR waiting) → retires the scanner
  signature.  Then RE the reveal-target field off a KNOWN captured grid, wire fast-skip + auto-
  advance (gate the advance on the grid, not the widget).  Alternatives surveyed: PolyHook2 (C++11,
  heavier), subhook/funchook (lighter C, BSD-2).

### LANDED — MinHook grid capture VERIFIED + the reveal fields (NOT +0x8)
- `dlghook` MinHooks the ctor 0x5e59c0 (__thiscall detour, 7 args, `ret 0x1c`) → `g_dlg_grid` at
  BUILD time.  **VERIFIED live:** dlghook-ON + newgame (dlgskip-OFF) survives the WHOLE intro with
  `captures`=4→13 ctor hits, game alive — exactly where install_detour + the VEH both crashed.
  Retires the dlggrid scan for capture (it also catches waiting/instant boxes the scan missed).
- **The reveal counter is NOT +0x8** — the DESIGN's old (unverified) guess is DISPROVEN: +0x8 stays
  1 (or 6) while a line types.  `dlgtrace` (engine-speed grid[0..0xC0] snapshots) on a hook-captured
  TYPING grid shows THREE climbing fields: **+0x0c 0→190, +0x10 0→192, +0x54 0→1000**.  +0x4c is NOT
  a stable text length (reads 512 / 65538 / 458762 across grids = capacity/uninit junk).  Working
  model: +0x0c = revealed char count, +0x10 = total chars (leads +0x0c by ~2), +0x54 = reveal
  progress 0..1000 (same range as the widget box-scale).
- **NEXT (fast-skip, un-blocked):** determine the DRIVER field (force each on a CONTROLLED typing box
  — a non-voiced NPC/Dad box, not the auto-advancing voiced intro — and verify via a window
  screenshot or field-stability re-read: the driver stays forced, a derived field gets recomputed).
  Then fast-skip = force the driver; wire auto-advance gated on the GRID being active (not the
  input-mgr widget).  Keep dlgskip OFF across the title→scene transition (the pre-existing race).

### Reveal-offset RE — the base-game MODEL + why the SE offsets differ (byte-scan next)
Cross-ref'd the PORTED dialogue (USER: "we already ported this — get the offset from the decompile,
then scan the SE binary for the same function").  Base-game (sotes.exe) model from the decompile:
- dialogue obj: **scale +0x54** (CONFIRMED shared with SE = 1000 done), text-machine ptr at +0x174.
- TM = *(dialogue+0x174) (43ca40's `piVar1`, int*): mode +0x00 (0/2), cols/divisor +0x0c (`[3]`),
  **total +0x10 (`[4]`)**, **cursor/reveal +0x14 (`[5]`)**, +0x18 (`[6]`).  Case 9 snaps +0x14 to the
  end (== the port's `dialogue_skip_reveal` reveal=total).  `dialogue.c` provenance: typewriter
  **0x439690** (writes the pause-grade table {i, i*2, i*3, i*5} to grid +0x10/+0x20/+0x24/+0x28,
  i = `*(*DAT_008a6e80+0x248)`), skip **0x43ca40**, text-expand 0x43cf90.
- LIVE-ELIMINATED on the SE box (hooked grid 0x5e59c0, Dad's dialogue): reveal is NOT +0x8 (stays 1),
  NOT +0x0c (forced 149→60, render UNCHANGED — screenshot-verified), NOT +0x10/+0x4c, and +0x54 =
  scale.  The SE grid's sub-object ptrs DON'T match base (+0x74=0x20, +0x170/+0x174 = garbage) — so
  the **SE dialogue struct differs from base at the sub-object level** (only +0x54 scale is shared);
  the ported base offsets can't be applied blind.
- **NEXT (the USER's plan):** disasm the base-game typewriter 0x439690 (distinctive: the grade-table
  {i, i*2, i*3, i*5} stores + the `*(*DAT+0x248)` interval load), scan `sotes-ense-en.exe` for the
  same function (structurally similar bytes/immediates), then RE which object+offset the SE typewriter
  reveals — that's the SE reveal counter to force (or call the SE analog of 43ca40 case 9 via `call`).
- **Screenshot verifier (works):** the game window is capturable — Get-Process(sotes-trainer).
  MainWindowHandle → GetClientRect+ClientToScreen → `Graphics.CopyFromScreen` (captures the displayed
  DDraw pixels; PrintWindow / FindWindow-by-class both failed).  `scratchpad/shot.ps1`.

### Reveal counter FOUND — byte-scan RESOLVED (session 8, 2026-07-17)
The base grade-table fingerprint = the **{×2,×3,×5}-of-one-register lea triple** (base 43a720:
`lea [c+c*1]`/`[c+c*2]`/`[c+c*4]` → 2i/3i/5i pause grades).  UNIQUE in SE .text (`find_grade2.py`
over the full objdump; VA=fileoff+0x400000 both exes) → the SE typewriter grade-setter **0x5e84f0**.
Chased the object graph statically → the reveal offset the port needs:
- **The reveal counter is TM+0x14 (== base), TWO pointer-hops BELOW the captured grid:**
  `grid(g_dlg_grid) --+0x48[i]--> widget(0x1b0 B) --+0x170--> TM(0x2c B) --+0x14 reveal`.
  **TM+0x10 = total; SKIP = write TM+0x14 = TM+0x10** (== port `dialogue_skip_reveal` reveal=total,
  dialogue.c:199).  The old DESIGN probes failed b/c they poked grid+0x8 / widget fields directly —
  the reveal lives on the TM sub-object, not the grid or the widget.
- **g_dlg_grid IS the container** (ctor 0x5e59c0 PROVEN: `[+0x4c]=cap` word, `alloc cap*4`→`[+0x48]`=
  widget-ptr array, each widget `alloc 0x1b0` B [== DESIGN's "cell"], `[+0x4e]`=count word init 0).
  ⇒ active widget = `(*(u32*)(grid+0x48))[ *(u16*)(grid+0x4e) - 1 ]` (last-allocated line) — reachable
  from the EXISTING ctor hook, **NO new hook**.
- **TM sub-struct (0x2c B, alloc'd in start-line 0x5e6cd0 @ `widget+0x170`):** +0x00 glyph buf(0x200),
  +0xc step/mode(=1), **+0x10 total**, **+0x14 reveal cursor**, +0x1c pacing timer(GetTickCount+300/
  +100), +0x20/24/28 pause grades 2i/3i/5i.  Init by TM_init **0x5e84f0**(a1→+0xc, a2=i→+0x10, a3→
  +0x14 cursor-init, a4→+0x18).
- **FAITHFUL skip = call 0x5e7ad0(ecx=box, cmd=9)** — the SE analog of base `FUN_0043ca40` (PROVEN:
  same GetTickCount now+300/+100 timers @TM+0x1c, same case-4..7 switch, 11-entry command jump table;
  reads TM via `box+0x174`).  Cmd 9 = snap reveal→end.  ⚠ NOTE `box+0x174` (skip) vs `widget+0x170`
  (typewriter) — the SAME ±4 as base; verify live which the widget exposes (likely box==widget with
  the TM ptr cached at both, or box=widget's parent).
- **Key SE VAs:** grid-ctor 0x5e59c0; add-line 0x5e6530 → get-widget 0x5e62c0 → start-line 0x5e6cd0;
  TM_init 0x5e84f0; skip/step 0x5e7ad0.  Tools: `scratchpad/find_grade2.py`, `se_text.asm` (full objdump).
- **VERIFIED LIVE + IMPLEMENTED (session 8).**  Walked the chain on a live SE (dlghook-captured
  container 0x3021a468): **cap=10 / count=7**; widgets [0-5] = DONE lines (+0x170=0, finalized TM at
  +0x174), the **ACTIVE line = widget[6] +0x170 → TM total=4 reveal=1** (mid-type).  Writing
  **TM+0x14 = total STICKS** (the typewriter stops at reveal>=total); writing 0 gets re-driven to 1
  (PROVES +0x14 IS the live reveal counter, not a derived field).  `fastskip`/`grid`/`dlggrid`
  REWRITTEN to walk `grid → widget[+0x48][count-1] → TM(+0x170) → +0x14=+0x10` (`dlg_active_tm` /
  `dlg_force_reveal`, trainer.c); pure UI-state, no input → door-safe; builds clean.  ⚠ PENDING: the
  on-screen VISUAL confirm — the BACKGROUNDED game's sim pauses (the typewriter ticks once per socket
  wake, then stalls; a foregrounded/focused run is needed to watch text flow) + a re-inject smoke test
  of the new build.  Optional follow-up: wire the faithful `call 0x5e7ad0(cmd=9)` as an alternative to
  the direct write.  Retires `PORT-DEBT(dlgskip-reveal-ui)` at the memory level.

### The "hold TAB" fast-skip = the auto-advance flag [mgr+0x12c] (session 9, 2026-07-18) — DONE
USER: holding TAB skips dialogue even faster than fastskip; find the code + use it.  RE'd with a frida
DEBUGGER (Interceptor + Backtracer + a code-patch toggle — the reusable rig below):
- WHY fastskip alone wasn't enough: fastskip (reveal=total) only INSTANTS the text, it does not ADVANCE
  the box; dlgskip (inject 0x24/0x27 into g_ti_mgr) only advances the DOOR-prompt widget
  (`g_ti_mgr+0x374`).  Story/cutscene/NPC dialogue is a SEPARATE object (the `g_dlg_grid` container) on a
  SEPARATE input path, so dlgskip never advances it — THAT is the gap TAB fills.
- Hooked the SE skip/advance fns while the USER held TAB: **wrap 0x5e7fe0(cmd=8) fired 73x**, and
  **NOTHING** appeared in the g_ti_mgr input ring (`monitor_tab.py`) — so TAB drives the story stepper's
  AUTO-ADVANCE, not a button.  cmd 8 = base `FUN_0043ca40(8)` auto-advance.  Backtrace:
  `0x437752 <- 0x43719b <- 0x434ff8 <- cutscene script 0x4dd290`.
- THE GATE (SE analog of base `FUN_0043b980:75`): `0x437738 mov eax,[ebx+0x12c]` (ebx = the story
  dialogue mgr; **+0x12c = the auto-advance flag**) → `0x437740 je 0x437752` skips the advance when the
  flag==0; else `0x43774d call wrap(cmd=8)`.  Holding TAB sets `[mgr+0x12c]!=0`.
- **FIX = patch the je: `0x437740` `74 10` → `90 90` (nop nop)** → the stepper advances EVERY frame → ALL
  story dialogue skips itself, hands-free.  **VERIFIED LIVE** (frida patch, USER-confirmed): skips Mom's +
  Dad's + every new box, NO stray world input, repeats on each dialogue.  SHIPPED as **`autoskip`**
  (`dlg_autoskip` + `patch_bytes`; `VA_DLGADV_JE`).  Pure 2-byte code patch, no input injection.  **Default
  ON** (USER; applied at attach in `keepalive_thread`, alongside attract-freeze — a boot code-patch, so no
  scene-settle debounce needed).  Also auto-advances CHOICE boxes → `autoskip off` to read/pick.  Builds clean.
- **The frida DEBUGGER rig is reusable** (the USER-requested "actual debugger"): `scratchpad/dbg.py`
  (Interceptor onEnter hooks + `Thread.backtrace` on ANY VA — absolute addr, delta=0; logs ecx/args/bt),
  `patch_test.py` (`Memory.patchCode` je→nop toggle + wrap-call counter), `monitor_tab.py` (input-ring
  dump over the socket `read`).  Attach: frida remote `cutestation.soy:27042` → PID (sotes-trainer-oss.exe).
  For a future path-diff RE, frida **Stalker** can block-trace with/without an input and diff — not needed
  here (the gate fell straight out of the Interceptor backtraces).

## Probe rig (temporary, delete when done)
- `scratchpad/trainerctl.py` (spawn + repro_agent input/shot + trainer_agent memory,
  poll-file queue), `scratchpad/navto.py` (mode-aware nav), `scratchpad/tctl.py` (sender).
- `tools/ennse_trainer/` Frida trainer_agent has the verified offsets above.
- **Live-run recipe** (proven this session): `build/inject.exe <unpacked-exe> <full-path
  sotes_trainer.dll> <cwd>` (drop the unpacked exe into the SE game dir so its cwd finds the SE
  assets + `user/`), then connect `cutestation.soy:7777` and drive `state`/`saves`/`load`.  The
  DLL is hands-free (launcher dismiss + keep-active + attract-off on attach).  `OSS_NO_REPRO=1`
  if repro_agent is also up (its 0x437c70/0x4378d0 hooks collide — DESIGN session 3b).
