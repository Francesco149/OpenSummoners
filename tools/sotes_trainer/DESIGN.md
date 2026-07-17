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

## Live findings — real tower save (Arche Lv17, HP 276/301), 2026-07-17

Probing a REAL loaded save (not the demo) surfaced bugs the demo hid:
- **TELEPORT (position write) gets RESET.** Writing `world_x 0xc76c` reverts within a
  few frames to the authoritative value (wrote 109724 → snapped back to 169640).
  `0xc76c`/`0xc770` are a per-frame **derived snapshot**; the game re-integrates
  position from an authoritative source (sub-pixel accumulator / physics field) NOT
  yet pinned (`+0xc790`=168120 is a near-miss candidate; region `+0xc764..+0xc7c0`
  dumped). A 50ms freeze can't beat the ~16ms reset. FIX = RE the position-commit path
  (the EN-SE analogue of the port's `0x442a70` worldX-commit — cross-ref `src/character.c`
  / freeroam-collision findings) and write THAT, or hook the commit. Blocks teleport/warp.
- **`level` offset WRONG.** `stat_block+0xe0` reads 5 but the HUD shows Lv17, and 17 is
  absent from `+0x40..+0x110` → 0xe0 is not level; real level field TBD. (hp/mp/hp_max
  are correct: hp_max=301 is the 3-term sum, confirmed — not a single field.)
- **Anchor is correct** (only ONE live `0xc35a` actor; other hunt hits are garbage).
  hp/mp read + stat writes (god/setstat) work (stat block is not physics-reset).
- **Mob scan WORKS** (`scratchpad/mob_scan.py`): monster-code actors (`0xc742..0xc83e`)
  filtered by validated same-map world coords, excluding the near-Arche **elemental**
  (code 51090 @ ~56px) and stale `(0,0)` demo actors — found the real mob (51052 @ 574px).
  Port this into the DLL as `tpnearest`/`listmobs` once teleport's position field is fixed.

## Probe rig (temporary, delete when done)
- `scratchpad/trainerctl.py` (spawn + repro_agent input/shot + trainer_agent memory,
  poll-file queue), `scratchpad/navto.py` (mode-aware nav), `scratchpad/tctl.py` (sender).
- `tools/ennse_trainer/` Frida trainer_agent has the verified offsets above.
