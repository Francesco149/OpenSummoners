# sotes_trainer — standalone EN-SE trainer (WIP)

A **standalone, injected DLL** trainer for the retail EN Special Edition
(`sotes_en.exe`, unpacked ImageBase `0x400000`). In-process, **no Frida**. Two front-ends onto
one core: a **Dear ImGui window** (`trainer_ui.cpp`, opens when the DLL loads) for the human, and
a localhost line-JSON socket (`:7777`) for the LLM/agent (`trainer_mcp.py`). Architecture +
discovered mechanics: **`DESIGN.md`**; SE addresses: **`SE_CODE_MAP.md`**.

Status: **working end-to-end.** Cheats (god = hp+mp 9999, auto-skip, mouse-fly, …), teleport,
player/map/stat reads, **portal-hijack warp** (change a door's destination via fuzzy room-key
search + revert), the room-table walk, save enumeration + menu-drive load/newgame — all in the UI
and the socket. Runs via the generic **mod loader** (drop `mods\sotes_trainer.dll` + launch).
All pokes + engine-fn calls execute engine-side via a thread-safe queue drained at the inputPoll
safepoint. NEXT: force a cross-room/area transition (roadmap #4) + BFS fast-travel.

## Build

```
nix develop --command make -C tools/sotes_trainer      # -> build/sotes_trainer.dll
```

(or the raw line, which links the standalone `.sdt` reader `tools/sotes_save/`:)

```
nix develop --command i686-w64-mingw32-gcc -shared -O2 -s -Wall -static -static-libgcc \
  -Itools/sotes_save -Itools/sotes_trainer/minhook/include -o build/sotes_trainer.dll \
  tools/sotes_trainer/trainer.c tools/sotes_save/sotes_save.c \
  tools/sotes_trainer/minhook/src/{hook,buffer,trampoline}.c \
  tools/sotes_trainer/minhook/src/hde/{hde32,hde64}.c -lws2_32 -luser32
```

`-static -static-libgcc` is REQUIRED — a default mingw build imports
`libgcc_s_sjlj-1.dll`, absent in the game's DLL search path, so `LoadLibrary` fails.
Both build in the mandatory gate `bash tools/ci/build_all.sh`.

**MinHook** (`tools/sotes_trainer/minhook/`, BSD-2, vendored) is the robust inline-hook lib:
its HDE length disassembler copies WHOLE instructions, so it hooks the SEH-scope dialogue-grid
ctor `0x5e59c0` (6-byte `mov eax,fs:0x0` prologue) that the old fixed-5-byte `install_detour`
corrupted. Used by `dlghook` to capture the story/cutscene text-grid `this` at build time.

## Install / run

**The convention: the [mod loader](../mod_loader/).** Drop the DLL into a `mods\` folder beside
the game exe and just launch — no injector, the game is not modified:

```
<game>\version.dll             <- the mod loader (build/version.dll)
<game>\realver.dll             <- copy of C:\Windows\SysWOW64\version.dll
<game>\mods\sotes_trainer.dll  <- this trainer (build/sotes_trainer.dll)
```

Then run the exe normally. On load the trainer opens its ImGui window + binds `:7777`, dismisses
the `#32770` launcher, and keeps the game running unfocused (default boot behaviors below). It
coexists with the JP voice patch (`mods\ennse_voice.dll`).

- **Quick dev loop** (one-shot, no staging): `build/inject.exe <unpacked-exe> <full-path
  sotes_trainer.dll> <cwd>` — CreateProcess-SUSPENDED → remote LoadLibrary → Resume (runs via
  WSLInterop).

On attach it logs to `oss_trainer.log` beside the exe and binds `:7777`, and — so it works
while its window is backgrounded (an agent/UI drives it from elsewhere) — spawns a keepalive
thread with three DEFAULT-ON behaviors: (1) **launcher dismiss** — click through the `#32770`
launcher in-process (no manual click); (2) **keep-active** — re-post `WM_ACTIVATEAPP(TRUE)` to
the game window so it keeps rendering/updating unfocused, without stealing focus; (3) **attract
off** — freeze the title→demo trigger so the title stays up for the menu-drive load. Toggle 2/3
via `keepactive`/`attract`. (Live-verified via `build/inject.exe` into `sotes-ense-en.exe`.)

### Relaunch recipe (agent-operable, this env)

The game (as **`sotes-trainer-oss.exe`**) is staged in `C:\oss-ennse-voice-repro\stock\` (the
unpacked SE exe + all `sotes*.dll` assets + `user\` saves), with the mod loader already in place
(`version.dll` + `realver.dll`). To rebuild + relaunch via the mod loader (runnable from WSL —
`cmd.exe` launches via WSLInterop, no wine):

```
nix develop --command make -C tools/sotes_trainer                 # rebuild the DLL
S=/mnt/c/oss-ennse-voice-repro/stock
cp build/sotes_trainer.dll "$S/mods/sotes_trainer.dll"            # stage the FRESH DLL (else stale)
# kill any prior instance by EXACT pid, then launch (cwd = game dir so it finds sotesd.dll etc.):
PID=$(tasklist.exe /FI "IMAGENAME eq sotes-trainer-oss.exe" | grep -i sotes | awk '{print $2}')
[ -n "$PID" ] && taskkill.exe /PID "$PID" /F
cmd.exe /c start "" /D 'C:\oss-ennse-voice-repro\stock' 'C:\oss-ennse-voice-repro\stock\sotes-trainer-oss.exe'
```

It boots to the TITLE (`player`==null); `load`/`newgame` to enter a scene. The exe name
`sotes-trainer-oss.exe` is deliberate — it's the frida attach target and keeps kills targeted
(never `pkill -f`, which self-matches; kill by exact pid; never touch the shared frida-server).
(Or, for a one-shot without staging into `mods\`, `build/inject.exe <exe> <dll> <cwd>`.)

### RE / probing (throwaway frida)

The mechanics were discovered by **frida** on the live game, then baked into `trainer.c`. Attach
to `sotes-trainer-oss.exe` at `cutestation.soy:27042`; **VA = fileoff + 0x400000** (delta 0 live).
Fingerprint SE code by objdumping the unpacked exe (`vendor/unpacked/editions/sotes-ense-en.exe`)
— SE VAs do NOT match the old Steam base game (see `SE_CODE_MAP.md`). ⚠ Do NOT frida-hook the
scene-rebuild path — it fires mid-teardown and crashes the game; force things via the trainer's
engine-thread safepoint call instead.

## Drive (the LLM / UI interface)

Line-delimited JSON over TCP. From WSL, connect to the Windows host over the LAN
(`cutestation.soy:7777`) — loopback won't cross the WSL2 NAT (the DLL binds
`INADDR_ANY` for this; a shipped build should default to loopback).

**The durable LLM interface is `trainer_mcp.py`** (committed; the LLM counterpart to the
human's ImGui UI, which talks to the same socket). Registered as the `sotes_trainer` MCP in
`.mcp.json` → an agent gets `mcp__sotes_trainer__*` tools (typed `player`/`teleport`/`load`/…
plus a `raw` passthrough for any command, incl. ones added later). Also a CLI for immediate use:

```
python3 tools/sotes_trainer/trainer_mcp.py player
python3 tools/sotes_trainer/trainer_mcp.py teleport x=50000 y=40000
python3 tools/sotes_trainer/trainer_mcp.py raw cmd=state      # arbitrary passthrough
python3 tools/sotes_trainer/trainer_mcp.py --selftest         # ping/state/player
```

Endpoint override: env `OSS_TRAINER_REMOTE=host:port` (a same-host ImGui UI uses
`127.0.0.1:7777`). For raw one-offs, any line-JSON client works (`nc`, a socket snippet).

| cmd | args | effect |
|---|---|---|
| `ping` | — | `{pong,delta,base}` — liveness + ASLR delta |
| `player` | — | `{actor,world_x,world_y,stat_block,hp,hp_max,mp,mp_max,level_base,exp_cur,exp_max}` or null |
| `state` | — | boot/hook diagnostic: `{hooks,main_wnd,launch_clicked,attract_frozen,keepactive,dlgskip,box_open,box_scale,md_state,ti_mgr,pk_mgr}`. `ti_mgr`≠0 ⇒ input poll firing; `box_open` ⇒ a dialogue box is on screen (what dlgskip gates on); `box_scale` = its pop-in 0..1000. |
| `read` | `addr`,`type`(u8/u16/u32),`va`(bool) | read; `va:true` = AP()-relocate a 0x400000 VA |
| `write` | `addr`,`value`,`type`,`va` | write |
| `setstat` | `which`(hp/hp_max/mp/mp_max/level),`value`,`lock`(bool) | set a player stat |
| `god` | `on`(bool) | freeze hp+mp at **9999** each frame (invincibility + free casting). **Default ON.** (Absorbs the old `invincible`.) |
| `teleport` | `x`,`y`(centi-px),`relative`(bool) | move the player via the **phys-box** (`*(actor+0x40)`): X sticks, Y gravity-settles. Absolute = world centi-px; relative = px |
| `mousefly` | `on`(bool) | continuously teleport the player to the cursor over the game window (also **F7**). Freezes the view (RE'd camera `*(root+0x104c)`, top-left cur_x/cur_y) so she stays under the cursor; edge-scrolls to traverse the map |
| `flyto` | `mx`,`my` | map a GIVEN game-window client point → world + teleport there (the mouse-fly mapping for one point — deterministic calibration) |
| `hijack` | `slot`,`target` | overwrite exit-slot N's `target_room` in the live room record → that door warps to `target` (**WARP PRIMITIVE**; within-area). Original stashed for `revert` |
| `revert` | `slot` | restore exit-slot N's original `target_room` |
| `rooms` | `max`(opt) | enumerate the room-record table (valid warp destinations): per room `{key,area,scene}` — the fuzzy-search source for `hijack` |
| `view` | `off`(opt) | mouse-fly camera diagnostic: the resolved view rect (left/top from cur_x/cur_y + span) + the player box + a camera-object field dump |
| `box` | — | debug: the player's collision AABB `{box,tag,x,top,w,h,world_y}` |
| `load` | `slot`(opt),`downs`(opt) | **from the TITLE**: drive Continue→slot-picker→confirm via the game's own menu code. No `slot` = the default-highlighted newest save (= Archmage's Tower Lv17). `slot`:N = select savedataNN via the RE'd picker selection model, then confirm. **VERIFIED live** (slot 1 → HP134/Lv-base3/exp117-1000; slot 6 → HP235/293/exp18406-37000; slot 7/default → HP301/Lv-base5/exp20720-50000 — each == the file). `downs`=N = manual rotate-N fallback. |
| `saves` | — | enumerate + identify EVERY on-disk save (reads `user\savedataNN.sdt` directly, no engine load): per slot `{valid,handle,party:[{name,code,level_base}],file_size,header_grid}`. Use it to pick a slot to `load`. **VERIFIED live.** |
| `saveinfo` | `slot` | full summary of one slot (same shape as a `saves` entry) |
| `newgame` | `to`(opt),`btn`(opt) | **from the TITLE**: rotate to the New Game item then confirm — starts a fresh game. **VERIFIED** (`btn`:2 `to`:1 → fresh Lv1 Arche, HP100/exp0-250). `btn`=title rotate id (2=up/4=down), `to`=rotations from the default Continue. |
| `keepactive` | `on`(bool) | keep the game rendering/updating while its window is unfocused (re-posts WM_ACTIVATEAPP, no focus steal). **Default ON.** |
| `attract` | `freeze`(bool) | patch/unpatch the title idle→demo trigger so the title stays up. **Frozen by default on attach** (so the menu-drive load always has a title). |
| `dlgskip` | `on`(bool) | auto-advance an OPEN dialogue hands-free. PASSIVELY reads the SE dialogue widget (`*(input_mgr+0x374)!=0` ⇒ a box is on screen) and injects the advance ids (`0x24`/`0x27`) **only while a box is up** — in freeroam it injects NOTHING, so it can't auto-trigger a world interaction (the door). **Default ON.** (Instant reveal + a pure UI-state advance are WIP — PORT-DEBT(dlgskip-reveal-ui), see DESIGN.) |
| `dlgbtns` | `b0..b5` | set extra reveal-skip ids dlgskip injects while a box is up (now gated on the passive box-open read, no close-leak). **Default EMPTY** — these ids double as world input; leave off until the reveal is done via a UI-state write (DESIGN). |
| `scan` | `value`,`max`(opt) | find every u32 == value in the game heap (RW private regions) — general "find the object". ⚠ the DirectDraw framebuffer is a volatile RW region that matches any value; verify hits by re-reading. |
| `dlggrid` | — | locate the live dialogue text-container(s) by the pool + body-color `0x3e537d` signature; sets `g_dlg_grid` to the first hit and reports each `{grid,count,line,tm,total,reveal}`. `dlghook` is more robust. |
| `grid` | — | inspect the captured container: walks `grid → widget[+0x48][count-1] → TM(+0x170)` and reports `{grid,cap,count,widgets,line,tm,total,reveal}` (the REAL typewriter reveal state). |
| `dlghook` | `on`(bool) | **MinHook** the container ctor `0x5e59c0` to capture `g_dlg_grid` at BUILD time (robust — catches waiting/instant boxes the scan misses). VERIFIED through the intro (no crash), where the old byte-patch hook + a VEH both crashed. |
| `fastskip` | `on`(bool) | force the active dialogue line's typewriter reveal to total each tick (instant text) — a pure UI-state write (no button ⇒ door-safe). Walks `g_dlg_grid → widget → TM+0x14=TM+0x10` (reveal counter RE'd session 8, DESIGN). Pair with `dlgskip` to fully fast-forward. Default OFF. Chain VERIFIED live. |
| `autoskip` | `on`(bool) | **auto-advance ALL story/cutscene/NPC dialogue** hands-free — the **"hold TAB" skip** (the strongest). NOPs the story-advance gate `je@0x437740` so the stepper advances every frame; skips whole conversations with no keypress. Pure 2-byte code patch, no world input (no stray triggers). **VERIFIED live** (skips Mom's/Dad's/every box, repeatable). **Default ON** — it also auto-advances CHOICE boxes, so `autoskip off` when you need to read or pick. (`dlgskip`=door prompt, `fastskip`=instant text, `autoskip`=advance everything.) |
| `press` | `btn`,`n`(opt) | inject button `btn` into the active input mgr `n` times — a probe to map what each id does in the current context. |
| `call` | `va`,`a0..a7`,`ecx`,`reloc`(bool) | call an engine fn (thiscall via `ecx`); returns `ret`. EXPERIMENTAL (socket thread — unsafe for engine fns) |
| `loadraw` | `slot`,`enter`(bool) | EXPERIMENTAL direct chain (safepoint): 416550 load + 586c60 apply verified; the `enter` transition CRASHES (needs the picker dispatcher `this`) — prefer `load` |
| `unlock_all` | — | drop god + all locks |

Note: `player.level_base` (stat `0xe0`) is NOT the display level — the SE derives the
displayed level from EXP; `exp_cur`/`exp_max` are exposed for a table lookup (DESIGN).

## Verified mechanics (EN-SE, VA = fileoff + 0x400000)

Player anchor = scan for the `0xc35a` actor (ctor `0x419b00`, code @ `+0x1d4`), strong
cross-validation of stat block + world coords. Offsets: `world_x 0xc76c`,
`world_y 0xc770` (DERIVED per-frame snapshot — read-only), `box 0x40` (authoritative
position AABB: `+4`=X, `+8`=top, `+0xc`=w, `+0x10`=h; `world_x=box[+4]`,
`world_y=box[+8]+box[+0x10]-1`, RE'd at commit VA `0x484554`), `stat_block 0x760`;
stat block `hp_cur 0x54`, `hp_base 0x58`, `hp_equip 0x84`, `hp_buff 0x9c` (**max = sum**),
`mp_cur 0x5c`/`0x60`/`0x88`/`0xa0`, `level_base 0xe0` (≠ display level), `exp_cur 0xec`,
`exp_max 0xf0`. HP/MP verified live vs HUD; teleport proven (box write sticks). Full
mechanics + teleport/level RE + menu/load/input RE + the load-recipe plan: `DESIGN.md`.
