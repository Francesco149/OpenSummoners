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
Stella); `level_base` @code+4.  A fixed-604 metadata block ⇒ a 16-u32 party-header grid always
at body 0x260 (fields grow over a playthrough — gold/playtime/progress candidates, exposed RAW,
UNLABELED: not pinned, and we don't ship guessed names).

**`tools/sotes_save/`** — dep-free (libc-only) `sotes_save.{h,c}` + `sotes_save_dump` CLI
(headless: dumps every real save; my verification tool).  REUSABLE outside the trainer — a
save editor, the port's save subsystem.  Both build in `tools/ci/build_all.sh`.

### Trainer commands (know what each save is)
The trainer compiles `sotes_save` in and reads `<exedir>\user\savedataNN.sdt` directly (no
engine load needed to identify a save):
- `saves` → every present slot's `{valid,handle,party:[{name,code,level_base}],file_size,
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
exp_max=[code−0xc], level_base=[code+4]).

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

**The clean fix (next):** skip the typewriter by writing the dialogue box's reveal-progress field
directly (base: box `+0x54` <1000 while typing, ==1000 done) — NO button, so no world re-trigger.
Needs the SE dialogue-box object: find it via a live Frida watchpoint on the injected probe record
(whoever clears record[+0] is the dialogue advance-poll; its `this`/args = the box).  Then dlgskip
forces `+0x54=done` each poll while a box is active → instant reveal, and 0x24/0x27 auto-advance.
Commands added for this arc: `newgame`, `dlgskip`, `dlgbtns` (tune reveal ids), `press` (id probe),
`state` (boot/hook diag), `keepactive`.

## Probe rig (temporary, delete when done)
- `scratchpad/trainerctl.py` (spawn + repro_agent input/shot + trainer_agent memory,
  poll-file queue), `scratchpad/navto.py` (mode-aware nav), `scratchpad/tctl.py` (sender).
- `tools/ennse_trainer/` Frida trainer_agent has the verified offsets above.
- **Live-run recipe** (proven this session): `build/inject.exe <unpacked-exe> <full-path
  sotes_trainer.dll> <cwd>` (drop the unpacked exe into the SE game dir so its cwd finds the SE
  assets + `user/`), then connect `cutestation.soy:7777` and drive `state`/`saves`/`load`.  The
  DLL is hands-free (launcher dismiss + keep-active + attract-off on attach).  `OSS_NO_REPRO=1`
  if repro_agent is also up (its 0x437c70/0x4378d0 hooks collide — DESIGN session 3b).
