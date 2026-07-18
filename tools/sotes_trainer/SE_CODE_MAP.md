# SE_CODE_MAP — EN Special Edition binary map + base-game equivalence

Durable registry of RE'd **EN-SE** (`sotes-ense-en.exe`, unpacked ImageBase `0x400000`)
addresses, struct offsets, and their relationship to the **old Steam base game**
(`sotes.exe` → `vendor/unpacked/sotes.unpacked.exe`, the OpenSummoners port target), so we
don't rediscover it every session. Companion to `DESIGN.md` (mechanics + roadmap). Update
this whenever an SE VA/offset is verified.

## Conventions + tooling
- **VA = fileoff + 0x400000** for `.text`/`.rdata`/`.data` alike (both exes; verified vs section
  headers). Live delta is **0** (the exe loads at its preferred base) → a Ghidra/objdump VA == the
  live address. Any VA disassembles straight from the file at `off = VA − 0x400000`.
- **SE `.text` = [0x401000, 0x5fcc75)**, `.rdata` 0x5fd000, `.data` 0x8be000..0x930000, `.rsrc`
  0x930000 (≈67 MB **embedded map/asset DATA** — map codes like 90010 appear here as *data*, not
  code, so filter constant-searches to `.text`).
- **Base `.text` = [0x401000, 0x5cb795)**, `.rdata` 0x5cc000, `.data` 0x854000. Base render-root
  global (Ghidra) = `DAT_008a9b50`.
- Full-.text objdump (regenerate any session; the grep surface for fingerprints/xrefs):
  `dd if=…/sotes-ense-en.exe bs=1 skip=$((0x1000)) count=$((0x1fbc75)) | \`
  `i686-w64-mingw32-objdump -D -b binary -m i386 -M intel --adjust-vma=0x401000`.
- Live probe: **frida** to `sotes-trainer-oss.exe` @ `cutestation.soy:27042` (dump/disasm/hook —
  throwaway); the **trainer socket** (`trainer_mcp.py`) for read/write/scan/call.

## ⚠ THE base↔SE EQUIVALENCE RULE (read this first)
**Base and SE are INDEPENDENT RECOMPILES of ~the same source. VAs do NOT transfer, and the bytes
differ at *every* VA** (byte-diff base-vs-SE at 24 known VAs = 0 matching bytes across the board —
menu/save/dialogue/map/player ALL differ). So:
- **NEVER assume base VA == SE VA.** A base VA that "works" in SE is a *coincidence* to be verified,
  never a given. Counter-example proven: base `FUN_00587e00` = the map tile decoder; SE `0x587e00`
  = an unrelated config-string getter (`mov ecx,0x92ac68; call 0x40d100`). The SE map decoder is a
  DIFFERENT VA (≈0x5b2xxx–0x5b3xxx, found by fingerprint — see Map subsystem).
- **What DOES transfer: the MODEL** — struct field offsets, the algorithm/control-flow, distinctive
  constants (map codes, colors, alloc sizes), and cross-object relationships. Use the port
  (`src/`) + base decompile (`docs/decompiled/`, boundary ~0x5bdab0, curated subset) as the *guide*,
  then find the SE analog by **fingerprint** (search SE `.text` for the shared constant / structure),
  then **verify live**. This is how DESIGN cracked the dialogue reveal counter and the load chain.
- **Some struct offsets carry, some don't.** Actor/stat-block offsets carry (verified live). The
  MAP OBJECT is restructured (base is `operator_new(0x4120)`; SE has no 0x4120 alloc → different
  size/layout) — base map-object offsets do NOT reliably apply.
- Tools: `scratchpad/eqcheck.py` (byte-diff base↔SE at a VA — a MISMATCH just confirms recompile,
  it does NOT disprove role-equivalence; roles must be checked structurally, not by bytes).

## SE VA registry (verified on the SE exe — authoritative for SE)
All from live RE / SE objdump (DESIGN sessions + this one). "base" column = the port/decompile
analog where known; **the base VA is the MODEL, not necessarily the SE VA**.

### Player / actor / stats  (offsets carry to SE — verified live)
| SE VA | role | notes / base analog |
|---|---|---|
| `0x419b00` | player/actor ctor (14-arg) | code=arg6@esp+0x18 → `actor+0x1d4`; handle `+0x1d8`. Player code `0xc35a`. |
| `0x484554` | world_x/y position-commit | writes `actor+0xc76c/0xc770` from phys-box `*(actor+0x40)`: x=box[+4], y=box[+8]+box[+0x10]−1 |
- actor offsets: `+0x40` phys-box (AABB: +4 X, +8 topY, +0xc W, +0x10 H), `+0x760` stat_block,
  `+0xc76c/0xc770` world_x/y (derived snapshot), `+0xc7a4` → input-mgr chain (`g_ti_mgr = *(*(actor+0xc7a4))`).
- stat_block: `+0x54` hp_cur `+0x58` hp_base `+0x84` hp_equip `+0x9c` hp_buff (max=+0x58+0x84+0x9c);
  mp `+0x5c/0x60/0x88/0xa0`; `+0xe0` **combat_level_max** (MAX COMBAT LEVEL — the "N" in the stat
  window's "combat level M/N" + the HUD stars, USER live SE; the displayed char level = `+0xe0` +
  `+0xd8` level_bonus per `494e60:123`, NOT `+0xe0` alone); `+0xd8` level_bonus, `+0xdc` star_count;
  `+0xec` exp_cur `+0xf0` exp_max.

### ✅ Input / keyboard subsystem — RESOLVED live (movement-inject PROVEN; door-enter foreground-gated)
Char input mgr = `*(*(actor+0xc7a4))` (`OFF_INPUT_CHAIN`; live Arche `0x085fc4f4`). HELD-AXIS (base
layout carries; idle=0): `mgr+0x114` UP · `+0x118` DOWN · `+0x11c` LEFT · `+0x120` RIGHT · `+0x124`
JUMP · `+0x128` ATTACK (held-tick counters; UP → door-enter cmd `[3]=0xb`).

**THE KEYBOARD OBJECT:** global `0x92d5bc` → keyboard device obj (`kb_this`; live `0x85235b8`).
`+0x4`=`IDirectInputDevice8*`, `+0x10`=buffered event count, `+0x14`=`DIDEVICEOBJECTDATA[]` event
array (0x10 B/entry: dwOfs,dwData,dwTS,dwSeq), **`+0x18`=256-B IMMEDIATE DIK buffer** (`buf[dik]&0x80`).

| SE VA | role |
|---|---|
| `0x5e2a10` | `kb_poll(this)`: `Poll()`(vt+0x64) → `GetDeviceState(0x100,this+0x18)`(vt+0x24) = the immediate buffer |
| `0x5e2a70` | `keydown(this,dik)` = `this[0x18+dik]&0x80` |
| `0x5e2820` | `buffered_read(this)`: `GetDeviceData(0x10,this+0x14,&this[0x10],0)`(vt+0x28) = the EVENT queue |
| ~`0x468e00` | HELD-AXIS builder: `keydown` the 4 HARDCODED arrows **UP=0xC8 DOWN=0xD0 LEFT=0xCB RIGHT=0xCD** → `mgr+0x114/118/11c/120`; jump/attack/menu keys CONFIGURABLE via options `0x92af98[0]+0x6a8/0x6c4/0x718` |
| `0x497050` | per-frame input refresh: `kb_poll`+`buffered_read` on `0x92d5bc`, then event consumers `0x5e2ba0`/`0x497180` |

**✅ MOVEMENT INJECTION PRIMITIVE — PROVEN.** Write `kb_this[0x18+DIK]=0x80` every frame (hook
`kb_poll 0x5e2a10` onLeave) → builder reads it → real movement. VERIFIED: RIGHT (0xCD) walked Arche
+300px; LEFT/UP likewise. **MUST maintain the FULL movement set each frame** (held DIKs→0x80, rest→0):
a key left at 0x80 STICKS (`GetDeviceState` only clears the buffer when FOREGROUND — see below), and
stuck UP+RIGHT = parry-lock that freezes her. Holding UP overrides horizontal walk (UP=defensive pose).

**⚠ THE FOREGROUND GATE — why the auto DOOR-ENTER fails.** MOVEMENT uses the IMMEDIATE buffer
(`keydown`), so continuous injection drives it regardless of focus. But DOOR-ENTER is a discrete ACTION
off the BUFFERED press-EVENT path (`GetDeviceData 0x5e2820` → ring). DirectInput is FOREGROUND-
cooperative: window NOT foreground ⇒ `GetDeviceState`/`GetDeviceData` return `DIERR_INPUTLOST
0x8007001e`, the immediate buffer FREEZES (last-injected values stick — PROVEN: `buf[UP]=0x80` written,
never cleared) AND no real buffered events flow ⇒ door-enter never fires. PROVEN: a full-floor sweep
(teleport + clean UP tap ∀ x∈[4000,150000]) with a `0x5a6010` transition hook NEVER fired — via
immediate-buffer UP, ring-press, OR a plain injected buffered event. The USER's teleport-to-door+UP
worked because their window was FOCUSED.

### ✅✅ DOOR-DRIVER SOLVED (foreground-independent) + WITHIN-AREA HIJACK-WARP PROVEN (2026-07-18)
The plain buffered-event inject failed because — NOT foreground — `buffered_read 0x5e2820` returns
**0 (failure)** on INPUTLOST, so the consumer (`0x497050`) SKIPS the events. **FIX: hook
`0x5e2820` onLeave and FORCE the return value to 1** (`retval.replace(1)`) while injecting the event,
so the consumer processes it regardless of focus. The auto DOOR-ENTER then fires with NO foreground.
Recipe (per frame while "pressing UP"), inject into the keyboard obj `*(0x92d5bc)` only:
```
kb_this[0x14] (event array)[0] = { dwOfs=0xC8(DIK_UP), dwData=0x80(press)/0x00(release),
                                   dwTimeStamp=GetTickCount(), dwSequence=++seq }
kb_this[0x10] (count) = 1 ;  buffered_read RETVAL := 1
```
Do one 'P' (press) event then one 'R' (release) ~0.3 s later = a clean tap. Immediate-buffer keys
(movement) kept CLEARED meanwhile.  **PROVEN reproducible + hijack-composed:** teleport to the door
zone + this tap → real transition: 440220→440210 (door x≈110000-112000), 440210→440220 (x≈158500);
and with `hijack slot=0 target=440230` the SAME door warped to **440230** (a room it doesn't naturally
reach) — a full WITHIN-AREA warp to any resident room. The door ZONE is a range (~2000 cpx wide), so a
coarse teleport sweep (step ≤1500) reliably lands on it; a single teleport can miss the edge. Each
transition does a fresh REBUILD (render_root changes) and fires **`0x5ac6b0`** (the door/scene entry;
observed args `[0,0x2738,0,ptr,0x2,TARGET_ROOM_RECORD,ptr,1]` — a5 = the target room record), NOT
`0x5a6010`/`0x5c83c0`.

### ✅✅✅ BAKED + CROSS-REGION WARP WORKS (2026-07-18) — the deliverable
BAKED into the trainer (`trainer.c`, MinHook post/around detours on `kb_poll 0x5e2a10` +
`buffered_read 0x5e2820`, installed LAZILY on first use — hooking during the game's boot-time
input-device activation CRASHES it, so NOT at boot): commands **`doorenter`** (fire the door-enter),
**`hold mask=N`** (1=UP 2=DOWN 4=LEFT 8=RIGHT movement) / **`release`**.  Driver **`warp.py <room>`**
= the CROSS-REGION router: BFS the live `rooms` graph, per hop — same-AREA hop = hijack ALL exits →
next + teleport-sweep + `doorenter` (direct, resident); cross-AREA hop = sweep WITHOUT hijack + take
the door into the next area (a wrong same-area door → warp back + resume; the REAL boundary portal
loads the next area's W-map, so NO crash).  Re-plans from the actual room each hop.  **VERIFIED live
end-to-end:** Archmage's Tower 440230 → 440210 → 440110 (within-area) → the real 440110→**420240**
boundary portal = a DIFFERENT AREA (420), no crash.  Cross-region fast-travel to any graph-reachable
room.  (Slow: each hop is a teleport-sweep; a future speedup = read exit door POSITIONS from the scene
to skip the sweep.)

### ✅ DOOR ANCHORS — the portal POSITIONS (2026-07-18) — warp SPEEDUP, live-PROVEN
The room-transition DOORS are invisible-volume **CHARACTER-band actors** in the pool at
**`*(render_root)+0x11e0`** (128 pointer slots; base+SE both — the base pool offsets CARRY: EFFECT
`+0x1160`/32, CHARACTER `+0x11e0`/128, DEVICE `+0x13e0`/1024, STRUCTURE `+0x2560`/128).  A door
anchor = a slot that is **active `+0x1d0`==1** with **exit_key at `+0x274`** (== a room-record exit
slot's key; `+0x278`=valid) and a live phys-box; its world AABB is `*(anchor+0x40)` → **`+0x04`=x
`+0x08`=y(top) `+0x0c`=w `+0x10`=h `+0x14`=baseline** (the SAME AABB as the player box).  Door
CENTER x = `box[+4]+box[+0xc]/2`; FEET (world_y) = `box[+8]+box[+0x10]`.  Door type-codes (`+0x1d4`):
70101-3 (0x111d5/6/7), 70440 (0x11328), 70610/1 (0x113d2/3), 70620 (0x113dc).  **Link to the exit:**
`anchor+0x274` == the room-record exit slot's key → its `target_room` (`rr+0x1c+slot*0xc +4`).  So the
portal position for an exit = the CHARACTER-band anchor whose `+0x274` == that exit's `exit_key`.
**VERIFIED live** (room 420240): slot-0 code 70101 gkey=1 @center(1600,113600) → exit 1 → 420230;
slot-2 code 70102 gkey=2 @center(166400,94400) → exit 2 → 440110.  Teleport onto the center-x/feet +
`doorenter` fires THAT portal (proven — went 420240→420230/440110 by picking the anchor).  Baked:
`tc_get_map` fills each exit's `door_x`(center)/`door_y`(feet); the **`door` cmd** (`{slot,enter}`)
teleports straight onto it + optionally fires; the UI Portals "go" button; `warp.py` uses it (no more
sweep).  Base model: door-USE handler `FUN_0059a1f0`, overlap+code test `FUN_0059a7c0`, gate-anchor
key tag `FUN_00438670` (`+0x274`/`+0x278`).  Trainer code: `find_exit_anchor` (`trainer.c`).
**⚠ STALE-DUP fix (same session):** after a cross-region warp two code-0xc35a actors coexist — the
LIVE in-scene Arche (box tracks world_x: `box[+4]==+0xc76c`) + a stale/roster ghost (garbage box).
`find_player`/teleport was grabbing the ghost.  `actor_valid` now REJECTS an actor whose
`*(actor+0x40)[+4] != world_x` (the 0x484554 commit invariant), so teleport hits the on-screen Arche.

### ⚠ THE TRANSITION GATES — why a door won't fire in combat / when unseen (base `FUN_0059a1f0`)
The door-USE handler only fires INSTANTLY when the leader stands on a door AND **no combat is near**:
the instant-exit branch (base 59a1f0:177-186) needs `FUN_0059a880()==0` (no active entry in the
projectile/combat pool `+0x23e0` within 32000x/16000y of the leader) AND area flags `+0x2770==0 &&
+0x2764!=1`.  Two gates otherwise (USER-confirmed, matches the decompile):
- **In combat + portal SEEN before** → a HOLD-RAMP: `bVar3` (the seen-list scan `ctrl+8` stride 8 vs
  `target_room`, count `ctrl+0x2008`) true → `in_ECX[6] += 30`/frame (only +200 out of combat), cap
  10000, transitions at **`>9999` (0x270f)** ⇒ "hold UP a few secs".
- **Portal NEVER used** → `bVar3` false → the ramp path is SKIPPED (59a1f0:241) → `return 0`; only the
  combat-free instant path can pass ⇒ "can't enter until out of combat".
VERIFIED behaviorally: holding UP 10 s at a door in a mob room did NOT transition (hard block).  The
trainer FIX (next: a `warpgate` toggle, a code patch like `autoskip`) forces the instant transition
on any door-change, ignoring combat/seen/hold.  SE analogs (objdump): ramp `cmp eax,0x270f` @`0x5a646b`
(door region, near the door path `0x5a6e12`); door-code test @ `0x5c31a3`/`0x5c3801`/`0x5b4013`.

### ✅ FORCED WARP (no door handler) — the mechanism, for the DIRECT-warp (RE'd, not yet baked)
The room-load is **EDGE-triggered on StartArea's RETURN, not a passive poll** of `map+0x4024`.  Recipe
(base): stage the next key via `FUN_00401d40(target,return_key,exit_key)` (→ `ctrl+0x900/4/8`) then
commit `FUN_00402030` (→ `map+0x4024/+0x4028/+0x402c`; == `FUN_004e61a0` SetNextArea), also set the
restore copy `map+0x40d0/+0x40d4/+0x40d8`, THEN make `FUN_0058f360` return a transition code (the only
"kick" — the door handler / a menu / the cutscene fade).  Loads ANY room cross-area (StartArea loads
the target's own W-map by its scene index; `FUN_00561c90` linear-scans the whole room table, no area
filter).  Map object (holds `+0x4024`) = base `*(render_root+0x2784)` (SE `*(render_root+0x1044)` is
one indirection off — verify live).  Cutscene town→house→errands uses exactly this stage/commit-under-
fade (`0x4d7d80` dispatch).  ⇒ the direct warp needs the SE stage/commit analogs + a non-door trigger.

### Menu / title / load / save  (role-verified on SE; base VAs NOT in the exported decompile)
| SE VA | role |
|---|---|
| `0x437c70` | input poll `(ecx=mgr,arg1=now,arg2=btn_id)` — per-frame safepoint; the TITLE polls this |
| `0x4378d0` | generic-menu controller — the save-slot PICKER polls this |
| `0x581ba0` | title/load dispatcher (switch: 0x1a Start / 0x1c Continue / 0x1d Special / 0x1e Option) |
| `0x582c40` / `0x584ac0` | SE title routine / classic title routine |
| `0x585cf0` | Continue-load chain → writes `*out_tgt=handle`, returns 0xc |
| `0x416550` | save load `(this=S,handle,slot,0,0)` reads `savedataNN.sdt` (→ archive `0x5dee40`) |
| `0x586c60` | save apply `(this=DAT_0092ac68,handle,sel)` (acts iff handle==0x2738 Main Quest) |
| `0x5cb460` | deferred scene transition `(this,tgt,x,y)` — the load-INTO-game enter (tgt=save handle, NOT a map id); builds a cmd via `0x5a6010` |
| `0x5ef121` | engine alloc(size) [SE-only .text region ≥0x5cb795] |
| `0x5e8e80/0x5e8ea0/0x5e8ec0` | picker getters (handle/slot/handle-for-i) |
| `0x40d100` | config/resource getter `(ecx=0x92ac68, idx)` |
- globals: `0x92ac68` config/game-state singleton (embeds game-dir str + save-key table ptr; apply's `this`);
  `0x92dd4c` title/load dispatcher `this` (embeds save-key table `0x5fd290` — NOT the live room);
  `0x92af98` game/options obj (`+0x24` res enum, `+0x158` classic/SE title selector); `0x92af7c` title cond.

### Attract / dialogue  (verified on SE; the reveal fns are SE-only .text, found by fingerprint)
| SE VA | role |
|---|---|
| `0x583866` | attract idle→demo trigger `jl 0x5832e1` (NOP→`jmp` freezes the title) → demo `0x408150` |
| `0x437740` | story-dialogue advance GATE `je 0x437752` (reads flag `[mgr+0x12c]`@`0x437738`); NOP = autoskip |
| `0x5e59c0` | dialogue text-grid ctor (thiscall, 7 args, ret 0x1c) — grid `+0x48` widget arr, `+0x4c` {cap,count} |
| `0x5e84f0` | TM_init (grade-setter, found by the ×2/×3/×5 lea fingerprint) |
| `0x5e7ad0` | dialogue skip/step (cmd 9 = snap reveal→end); `0x5e7fe0` = cmd 8 auto-advance |
- reveal chain: `grid+0x48[count−1]` → widget `+0x170` → TM (`+0x10` total, `+0x14` reveal cursor).
  Save `.sdt` codec: archive `0x5dee40`, keystr build `0x5df030`, perm table `0x5fd290` (.rdata).

### Map / scene subsystem  (RE IN PROGRESS — this session; SE VAs ≠ base)
The SE map decoder is NOT at base `0x587e00`. Located by the region-E anchor constant `0x15f9a`
(=90010, the ONLY `.text` use is `0x5b348b`):
| SE VA | role |
|---|---|
| ~`0x5b2xxx`–`0x5b34c3` | SE map DECODER (object/tile dispatch; region-E loop @`0x5b3460`) = base `FUN_00587e00` analog |
| `0x5b3840` | map/layer container `.count()` (thiscall ecx=container) |
| `0x54c8c0` | map/layer container `.get(idx, &out_obj, &out2)` (thiscall) |
| `0x5b3a70` | region-E link-anchor handler (base `FUN_0058cb30` analog) |
| `0x5b39c0`, `0x5b3850` | map object/tile emitters |
- **map object CODE @ object+0x10** (90010/90011 = region-E link anchors).
- `0x4c1ca0` SE map_id→room_key setter (base `FUN_004c5350`, thiscall ecx=controller; map_obj=`[ecx+4]`;
  reads `map_obj+0x4104`=map_id; switches 1 / 0x3f2(town) / 0x3fc; writes room_key `+0x4024`, spawn
  `+0x4028/+0x402c`). **Intro-map only** — normal door transitions do NOT hit it. Callers: 0x5a6418,
  0x5abeb2, 0x5cac2f, 0x5cb0a3. map_id write sites incl. `0x4e1d72 mov [eax+0x4104],0x3f2`.

### ✅ THE CURRENT-MAP CHAIN (render-root) — RESOLVED live (this session)
Found in the map-decode caller (`0x5ad443: mov ecx,ds:0x92dd38; mov eax,[ecx+0x1038]`):
```
render_root = *(u32*)0x92dd38                    // SE render-root global (base DAT_008a9b50 analog)
room_record = *(u32*)(render_root + 0x1038)      // base +0x1038 CARRIES
  room_record[0]  = ROOM KEY   (+0x00)   e.g. 0x334dd=210141 storage room
  room_record[1]  = AREA KEY   (+0x04)   e.g. 0xd2=210
  room_record[3]  = DATA/SCENE (+0x0c)   e.g. 1026  ← the FindResource("DATA") map id
  room_record[0x43]=tileset, [0x44]=parallax
  EXITS (portals): dword 7 stride 3 (+0x1c, +0x28, …), 20 slots:
     dw(7+3k)=exit_key, dw(8+3k)=TARGET ROOM key, dw(9+3k)=return/entry key
```
VERIFIED live: storage room (0x334dd/DATA 1026) → 2 exits both target 0x334dc (the shop), keys 2/3.
The base room-record layout (room_key[0]/area[1]/scene[3]/exits@dw7-stride3) CARRIES to SE. NOTE
`render_root+0x1044` (base map_obj) does NOT carry (reads junk) — use the ROOM RECORD for identity,
not the map object. Render-root also: `+0x1048` (used @0x5ad4a2), room_record `+0x148/+0x14c`.
Map DATA loader path: universal resource opener `0x5de520` (FindResource wrapper; maps AND sprites)
← map parser `0x54cxxx` (@0x54cae9); map DECODER `0x5b2xxx` (region-E @0x5b348b) ← builder `0x5ad4xx`
(reads render_root+0x1038, @0x5ad443) ← 0x4b5c8d ← 0x526144. The MASTER room table (ALL records) is a
contiguous heap block, **0x158 B stride** (record 0x150 + 8-B header), found by the longest-valid-run
scan (thread #3 ✅; 427 rooms this save). **OPEN**: the SE transition fn to force a room change (roadmap #4).

### ✅ THE CAMERA / VIEW OBJECT — RESOLVED live (mouse-fly; base camera_follow.h layout carries)
The view/camera object = **`*(render_root + 0x104c)`** (base analog: `src/camera_follow.h` +
`FUN_0043d1d0` the easer). Field offsets carry from base, VERIFIED live by two-point teleport
tracking + a cal dump (the trainer's mouse-fly maps the cursor through these):
```
cam = *(*(0x92dd38) + 0x104c)
  cam+0x00 = map_w (cpx)        cam+0x04 = map_h (cpx)      // map pixel size, for scroll clamping
  cam+0x5c = cur_y              cam+0x60 = cur_x            // EASED scroll ORIGIN = the view TOP-LEFT
  cam+0x64 = vp_w (64000=640px) cam+0x68 = vp_h (48000=480px)  // viewport span (world cpx)
  cam+0x6c = tgt_x             cam+0x70 = tgt_y            // the follow TARGET the easer chases
  cam+0x14 = a vertical LOOK-AT mirror (= player box_top; tracks the player INSTANTLY — do NOT use
             it as the view top: it creates a mouse-fly feedback runaway.  cur_y is the real top.)
```
So world↔screen: `screen_px = world_px − cam_top_px` (dialogue.c:312); a cursor at client fraction
f maps to `world = (cur_x, cur_y) + f·(vp_w, vp_h)`.  The camera EASES cur toward tgt (`0x43d1d0`),
clamped to `[0, map − viewport]`.  Freezing the view = pin cur+tgt; the trainer mouse-fly does this
(+ edge-scroll the frozen latch) so the player stays under the cursor yet can traverse the map.
The resolution enum (`*0x92af98+0x24`: 0→640×480 / 2→1280×960 / 3→1920×1440) only ZOOMS the window;
the world span (vp_w/vp_h) is constant, so read it live (zoom-safe).

### Scene-load STEPS + failure strings (found by the "map data not found" message-box)
The rebuild fn `0x5ac830` logs each load step via a label pushed to a debug/step fn (labels @0x924xxx):
`Start StartArea`(0x92480c @`0x5ac869`) → `Load Map Data`(0x9247b4 @`0x5ad4ed`) → `InitAreaGate`(0x924754)
→ `Setup View Manager`(0x92477c) → `Init Objects`(0x9247ec) → `Start Area Loop`(0x9246cc @`0x5ae047`) →
`Start Event`. **FAILURE strings** (fatal message-box → then a CRASH): `The map data is not found.`
(0x924790 @**`0x5ad656`**), `The gate is not found.`(0x924764 @`0x5adcc4`/`0x5adeeb`), `It failed in
reading the W-map file.`(0x9247c4 @`0x5ad433`), `Structure/Device/Effect/Character Object Count Over.`
The "Load Map Data" step (`0x5ad4ed`..`0x5ad656`) reads globals `0x92c828` (load-mode obj: switches on
`[+4]/[+8]/[+0xc]`, case `3` → `0x5ad8b2`), `0x92b9b4`, `0x92ac68` (config/game-dir); it builds a path
(`0x411930`/`0x411c10`) and loads a **W-map file** — i.e. maps come from a per-AREA W-map file, not just
a room key. **⇒ cross-AREA door-hijack FAILS at 0x5ad656** ("map data not found" → crash): PROVEN live —
hijacking a tower door's `target_room` to 110110 (Tonkiness, area 110) IS read + acted on (the door tried
to load it), but area 110's W-map data isn't resident from the tower save → fatal. So the door-hijack
warp PRIMITIVE works, but cross-REGION needs the target area's W-map loaded first (the 0x92c828 load-mode
/ W-map path is the lead).

### ✅ WARP PRIMITIVE — PROVEN live (USER-driven)
**Overwrite a door's `target_room` in the current room record's exit slot(s) → use the door → it warps
to the chosen room.** VERIFIED: hijacked room 440220's exits (normally 440210/440230) to **440150**;
walking through a door loaded room 440150 / scene 1324 (a room that door doesn't normally reach). The
hijack is written to the LIVE room record `*(*0x92dd38 + 0x1038)`, exit slots @ `rr+0x1c+k*0xc`
(`target` @ +4). **⇒ full auto-warp recipe (within a loaded area): hijack the door's target_room →
teleport Arche to the door → trigger UP → wait for `map` room_key to change.** LIMIT: target must be in
a resident area (cross-AREA crashes at `0x5ad656`, "map data not found" — needs the area's W-map loaded;
that's the next RE for cross-region). Tools: `scratchpad/hijack.py` (hijack exits), `find_rooms.py`
(enumerate the 730-room table by area — the fast-travel destination list).

**Cross-region PLAN (USER):** don't need direct cross-area warp — warp WITHIN-area to the room that
holds the real AREA-BOUNDARY portal, then use that real portal to cross (it loads the next area's
W-map correctly). So pathfinding = within-area hijack-warps chained through real boundary portals.
**Direct cross-area LEAD (still worth solving):** the load-request is `*(u32*)0x92c828` (read 5×; the
Load Map Data step `0x5ad586` reads `[req+4]`(switch, case 3 @0x5ad8b2)/`[req+8]`/`[req+0xc]`); the
`0x5a1bba` reader (fn ~0x5a1xxx) is the likely SETTER. Cross-area needs this request pointed at the
target area's W-map (loaded like a real boundary transition does) BEFORE the rebuild — else `0x5ad656`.

## Base-game MODEL (the reference to find SE analogs against)
From the port + `docs/decompiled` (base VAs — the algorithm/structs, NOT SE addresses):
- **3 ids on 2 objects**: `map_obj+0x4104` MAP ID (town 0x3f2=1010) · `map_obj+0x4024` ROOM KEY
  (town 0x334be=210110) · `room_record[3]` = the DATA/scene resource (`FindResourceA(module,
  scene&0xffff,"DATA")`; town 1022 / house 1023 / errands 1025).
- **render-root** global `DAT_008a9b50` → `map_obj = *(root+0x1044)`, `room_record = *(root+0x1038)`,
  camera `+0x104c`, actor band `+0x11e0` (128), effect pool `+0x13e0`, STRUCTURE bank `+0x27a4`,
  region-E grid `+0x1d1030`.
- **map object** (base `operator_new(0x4120)`, ctor `FUN_0059f2c0`): `+0x4104` map id, `+0x4024`
  room key, `+0x4028/+0x402c` spawn, `+0x4030` 8 actor slots, `+0x4084` active, `+0x4068` tick.
- **room record** (0x54 dwords): `[0]` room id/key, `[1]` area key, `[3]` DATA scene id, `[0x43]/[0x44]`
  tileset/parallax params, exits at **dword 7 stride 3**: `dw(7+3k)` exit key, **`dw(8+3k)` TARGET
  ROOM** (portal destination), `dw(9+3k)` return key. Built by `FUN_00585000` (port `gw_cross_reference`).
- **transition**: wrapper `FUN_0059ec30(mode,_,map_id)` → world ctor/loader `FUN_0059f2c0` (writes
  +0x4104, calls `FUN_004c5350` map_id→room_key→+0x4024) → selector-dispatcher `FUN_0059ee50` (reload
  codes 0x2724/0x272e/0x2738). DATA loader `FUN_00587970`(parse)/`FUN_005b62a0`(FindResource)/
  `FUN_00587e00`(decode). In-cutscene light room swap: `FUN_00401d40`(stage)/`FUN_00402030`(commit→+0x4024).
- **map_id → room_key** table `FUN_004c5350` (jump on map+0x4104): 0x3f2→0x334be(sp 0x65/1),
  0x3fc→0x30db3. Town-intro room chain: arrival 0x334be → house 0x334c8 → errands 0x334dc.
- map DATA format + tile dispatch: `src/map_data.{c,h}`, `src/map_decode.{c,h}`, `tools/extract/map_sweep.py`
  (376 maps); engine-quirks #119–121 (object bands: Effect 50000s / Structure 60000s / Character
  70000s / Device 80000s; region-E 90010/90011).
- Decimal ref: 0x3f2=1010, 0x334be=210110, 0x334c8=210120, 0x334dc=210140, 0x2738=10040, 0x15f9a=90010.

## Open threads (next)
1. ✅ DONE — SE render-root chain `*0x92dd38 → +0x1038 room_record` (room_key/area/scene/exits). The
   trainer `map` query reads this. (Found via the decode-caller `0x5ad443`, verified live vs a real
   door entry into the storage room = 0x334dd / DATA 1026.)
2. **SE transition fn to FORCE a room change** (roadmap #4) — MECHANISM TRACED, no clean call yet.
   The render-root REBUILD fn is **`0x5ac830`** (`__thiscall`, SEH-scoped, 0x1bd20 stack frame;
   `alloc(0x37c4)` new root → `*0x92dd38`; commits room_record @`+0x1038` via `0x5ac9ce`, map_obj
   @`+0x1044`). Its ONLY two callers (found by `grep 'call 0x5ac830'` — reliable, unlike the FPO
   backtrace that falsely fingered `0x4a1c60`):
   - **`0x5a6e12`** (fn ~0x5a6xxx) = the DOOR/portal path. Setup reads `[ebp+0x4104]` (the MAP
     OBJECT's map_id) + `esi+0x10/+0x14`, builds ~13 args, `this`=eax (a transition context from
     `0x5ac6b0`), pushes ebp(map_obj) last. So the transition is KEYED OFF THE MAP OBJECT'S STATE,
     not a plain room key.
   - **`0x5c861e`** (fn ~0x5c8xxx, near the load transition `0x5cb460`) = the save-LOAD path.
   ⚒ NO simple "enter room(key)" exists — a direct call would need the whole context reconstructed
   (the base `0x5cb460` direct-call crashed for exactly this reason, DESIGN session 3). ⚠ Also DO NOT
   frida-hook the rebuild/commit (mid-teardown free+realloc → CRASH). **Three realistic force paths
   (next session):** (a) set the MAP OBJECT's target (`map_obj+0x4104` map_id / `+0x4024` room_key,
   both writable) + find & set the pending-transition TRIGGER the door handler flips, then let the
   game rebuild itself (cleanest, needs the trigger flag); (b) SIMULATE the door input — teleport
   onto a door tile + inject UP (needs DINPUT-level injection, the door handler then does everything
   right); (c) reverse the full `0x5a6xxx` arg setup + `0x5ac6b0` context and call `0x5ac830` on the
   safepoint (highest crash risk). VERIFIED live (crash-free, USER-driven): tower 440220→440210→…
   is a clean chain (each room's exits link neighbors); `map` re-reads `*0x92dd38` so it's always the
   current room. LIGHT hooks on the dispatcher are safe; only the rebuild/commit hook crashes.

   **UPDATE — the transition is MULTI-PATH (caching), so there is no single "the transition fn":**
   render-root global `0x92dd38` has 6 writers — TEARDOWN `0x581a30` (frees old root, sets global 0);
   the full REBUILD `0x5ac6f2/0x5ac93e/0x5ac98d` (inside `0x5ac830`); and `0x5c84e1/0x5c89e4` (inside
   the transition fn `0x5c83c0`, whose 5 callers = 0x5c7a5d/0x5ca2db/0x5ca6b5/0x5caca4/0x5cb124). The
   rebuild `0x5ac830` has 2 direct callers: `0x5a6e12` (in `0x5a6010`, called by the deferred
   transition `0x5cb460`) and `0x5c861e` (in `0x5c83c0`). BUT hopping between ALREADY-LOADED adjacent
   rooms (tower 440210<->440220) takes a lightweight CACHED/POOLED fast-path that reuses a pooled
   render-root and hits NONE of {0x5a6010, 0x5cb460, 0x5c83c0, rebuild} — only the FIRST uncached load
   of a room hits the rebuild (proven: storage-room entry hit `0x5ac9ce`; the tower hops fired no
   hook). So a single direct-call "go to room X" is hard — the door-use handler dispatches
   cached-vs-rebuild and reads UP internally.

   **VALIDATED fast-travel PRIMITIVE (USER-driven, crash-free): teleport onto a door spot + press UP =
   correct transition** (tested 440210->440220 from a TELEPORTED position). So the robust auto path =
   teleport to the target door, then make the game's OWN door handler fire. Two ways without reversing
   the multi-path transition: (i) force the freeroam UP INPUT STATE briefly at the door (the handler
   then dispatches everything) — needs the DINPUT held-keys field; NOTE freeroam movement is DINPUT,
   NOT the button ring (injecting ring ids 1..20 moved Arche 0px and one opened the PAUSE MENU);
   (ii) reverse the door-use handler + call its transition branch (harder). NEXT: find the DINPUT UP
   field (path i) — the cleanest auto-fast-travel.
3. ✅ **DONE — the MASTER room table walk (all rooms + the map GRAPH).** The old 1-room symptom had
   TWO causes: (a) the walk used the wrong STRIDE — records are **0x158** apart (0x150 payload + an
   8-B alloc header/pad), not 0x150 (VERIFIED live: 440205/440210/440220 sit exactly 0x158 apart);
   (b) the LIVE record `*(*0x92dd38+0x1038)` is a per-area COPY (a short ~11-room contiguous run),
   NOT the master.  The MASTER is a SEPARATE contiguous 0x158-strided block = **by far the LONGEST
   such run in RW memory** (this save: **427 rooms**, base ≈`0x14a868`, all 39 areas, sorted, no
   dupes, an 8-B zeroed header before base bounds it).  No fixed global points at it (heap addr
   varies per launch), so `tc_get_rooms` FINDs it via `room_table_scan` = the longest-run scan (the
   `tc_get_chars` pattern) + a cached, cheaply-revalidated base.  The `rooms` cmd now emits
   `{key,area,scene,exits:[target_room,...]}` for all 427 = the cross-region GRAPH (`area:A` filters).
   **Graph facts** (this save): 946 edges, 64 cross-area; the ONLY dangling target is **999999** = the
   OVERWORLD/world-map sentinel (47 edges — filter it, it is NOT a room); BFS over real edges reaches
   **324/427 rooms (26 areas)** from the tower 440220, the rest (110/120/130/200/450/980-990) being
   separate components reached via the 999999 overworld, not a room portal.  So within-a-component
   cross-region routing = BFS the graph; crossing components needs the overworld transition (#2).
   Code: `trainer.c` `rec_ok`/`room_table_scan`/`room_table_get`/`room_table_rec`.  **REMAINS = #2**
   (the warp EXECUTION: auto door-enter along a route).
