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
- **`level` — RESOLVED (display level is EXP-DERIVED, not a stored int).** `stat_block+0xe0`
  (=5) is `level_base`, NOT the display level; the demo's "Lv3" match was a coincidence.
  17 is absent as a u32/u16/u8 anywhere in the stat block, the actor, or near the player
  handle (58 handle refs scanned). But `sb+0xf0 = 50000` == **exactly Arche's level-17
  `exp_max`** in the base-stat table (`{0xc35a,17,290,52,{68,60,38,44},50000}`), and
  `sb+0xec = 23167` = exp progress ⇒ the char IS Lv17, derived from EXP. TRAINER: expose
  `exp_cur (0xec)` + `exp_max (0xf0)` + `level_base (0xe0)`; the true display level needs a
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

## Menu / load / input — RE for the "load this save" recipe (2026-07-17 session 2)

Goal (USER): drive the title menu reliably + a repeatable recipe to (re)load this save.
Live disasm (SE VAs; some funcs carry `jmp 0x6b1xxxx` = pre-existing Frida hooks, benign):

- **Direct-load chain is STATEFUL — needs the menu UI object as `ecx`.** `FUN_005e8e80`
  (get handle) / `FUN_005e8ea0` (get slot) read `this->[0x174]→+0x14` (selected index) into
  `this->[0x17c]` array of 16-byte entries (handle@+0, slot@+4). So `FUN_00585cf0`'s
  terminal load (`0x416550` load + `0x586c60` apply + `0x5cb460` transition) can't be called
  from arbitrary state without reconstructing the title/menu object. ⇒ prefer INPUT injection.
- **Input model (the reliable path — RE'd, not yet drive-tested).**
  - **Find the active mgr:** IN-SCENE it is `mgr = *(*(actor+0xc7a4))` (proven — one of the 4
    process-wide refs to the live mgr is `*(actor+0xc7a4)`; the actor's input sub-object holds
    it).  AT THE MENU there is no player actor ⇒ the mgr must be found another way (a heap-scan
    for the 64-ptr signature, or a poll hook) — the OPEN piece, needs the menu state to nail.
  - **Layout:** `mgr[0xc + i*4]` = 64 button-record pointers (i=0..63; poll scans i=0x3f→0).
    Record (clean atomic snapshot, gameplay): `+0` = **type/category** (=2 for every gameplay
    record — NOT the button id), `+4` = timestamp, `+8` = **state** (0/1), `+0xc..` = aux.
  - **Poll `FUN_00437c70`/`FUN_004378d0`:** consume a record whose `+0` matches the poller's
    category (`437c70` wants `+0==0`, `4378d0` wants `+0==2`) AND `+8==1` AND `now-ts <= 0x64`
    (100ms), then clear `+0` and **dispatch via `FUN_005e7fe0`** (its arg = an `[esp]` value /
    `mgr->0x11c` fallback → push 6/7).  ⇒ the button→action map is the DISPATCHER, not a record
    id.  INJECT = set a record's `+8=1`, `+4=GetTickCount`, correct `+0` category — but which
    record-index→which menu action needs live menu observation (RE `0x5e7fe0` at the menu).
  - Menu button ids (DESIGN §"Mode signal"): {2,4,34,37} are the polled CATEGORIES at a menu
    (title + slot-picker); 4=rotate, 37=confirm, 34=abort.

## Recipe PLAN — "load this save" (next session; needs menu access + screen feedback)
The direct-load chain is stateful and the menu input needs live observation, so DON'T drive it
blind.  Fastest reliable route next session:
1. Spawn a FRESH instance for menu RE with SCREENSHOTS + input — reuse `tools/ennse_voice/
   repro_probe.py` (sandbox unpacked exe, DirectDraw capture, 64-slot input ring).  Leaves the
   user's live game untouched (separate pid).  Screens give the visual feedback this needs.
2. At the title, find the active input-mgr (heap-scan the 64-ptr signature or hook the poll),
   dump its records, then map slot→action by injecting `+8=1`/`+4=tick`/`+0=category` and
   watching the screen (Continue → slot-picker → confirm).  Detect load success HEADLESSLY via
   the trainer `player` cmd (a valid `0xc35a` actor appears in-scene).
3. Encode the working sequence into the DLL as `press <slot>` (writes the mgr record) + a `load
   <slot>` convenience (either the input drive, or the direct chain `0x585cf0` via the new
   `call` cmd once the title object is the `ecx`).  Then the trainer's `load` reloads this save.
4. The tower save's SLOT: `user/savedataNN.sdt` — pick by newest mtime (the loaded one), or read
   it from the load-menu object at confirm time (`0x5e8ea0` returns slot).

## Probe rig (temporary, delete when done)
- `scratchpad/trainerctl.py` (spawn + repro_agent input/shot + trainer_agent memory,
  poll-file queue), `scratchpad/navto.py` (mode-aware nav), `scratchpad/tctl.py` (sender).
- `tools/ennse_trainer/` Frida trainer_agent has the verified offsets above.
