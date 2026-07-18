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
  mp `+0x5c/0x60/0x88/0xa0`; `+0xe0` level_base (≠ display Lv, EXP-derived); `+0xec` exp_cur `+0xf0` exp_max.

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
(reads render_root+0x1038, @0x5ad443) ← 0x4b5c8d ← 0x526144. Room table (all records) in the heap
(0x1f39xxxx block; each record 0x150 B). **OPEN**: the SE transition fn to force a room change (roadmap #4).

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
2. **SE transition fn to FORCE a room change** (roadmap #4) — the door/portal handler that takes a
   TARGET ROOM key (from the exit slot) and loads it. Leads: the room BUILDER `0x5ad4xx` rebuilds
   whatever `render_root+0x1038` points at, so setting the room record + retriggering, OR the
   higher-level "enter room" fn (up the builder's caller chain 0x4b5c8d/0x526144/0x5e84aa). Also the
   intro path `0x4c1ca0`/callers 0x5cac2f/0x5cb0a3 (near the known transition 0x5cb460).
3. Room-record TABLE walk (all rooms, 0x150 B each, 0x1f39xxxx heap block) → the full map GRAPH for
   BFS pathfinding (roadmap #5).
