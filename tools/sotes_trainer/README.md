# sotes_trainer — standalone EN-SE trainer (WIP)

A **standalone, injected DLL** trainer for the retail EN Special Edition
(`sotes_en.exe`, unpacked ImageBase `0x400000`). In-process, **no Frida** — hosts a
localhost line-JSON socket that is BOTH the optional LLM/agent interface and the
future stats-UI backend. Architecture + discovered mechanics: **`DESIGN.md`**.

Status: **P0+P1 core proven end-to-end** (inject → socket → player anchor →
read/write/setstat/god/teleport). Menu/load/attract + traversal/combat features are
next (roadmap in DESIGN.md).

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

## Inject

- **Dev loop** (composes with the throwaway Frida probe rig): set
  `TRAINERCTL_INJECT_DLL='C:\...\sotes_trainer.dll'` before launching
  `scratchpad/trainerctl.py` — it `LoadLibrary`s the DLL in-target after resume.
  Or one-shot: Frida `Module.getGlobalExportByName('LoadLibraryA')` with the DLL's
  **Windows** path (`scratchpad/inject_test.py`).
- **Ship**: `build/inject.exe <unpacked-exe> <full-path sotes_trainer.dll> <cwd>`
  (CreateProcess-SUSPENDED → remote LoadLibrary → Resume) or a `version.dll` proxy (the
  ennse_voice pattern). Hands-free — see the default boot behaviors below.

On attach it logs to `oss_trainer.log` beside the exe and binds `:7777`, and — so it works
while its window is backgrounded (an agent/UI drives it from elsewhere) — spawns a keepalive
thread with three DEFAULT-ON behaviors: (1) **launcher dismiss** — click through the `#32770`
launcher in-process (no manual click); (2) **keep-active** — re-post `WM_ACTIVATEAPP(TRUE)` to
the game window so it keeps rendering/updating unfocused, without stealing focus; (3) **attract
off** — freeze the title→demo trigger so the title stays up for the menu-drive load. Toggle 2/3
via `keepactive`/`attract`. (Live-verified via `build/inject.exe` into `sotes-ense-en.exe`.)

## Drive (the LLM / UI interface)

Line-delimited JSON over TCP. From WSL, connect to the Windows host over the LAN
(`cutestation.soy:7777`) — loopback won't cross the WSL2 NAT (the DLL binds
`INADDR_ANY` for this; a shipped build should default to loopback). Helper:
`scratchpad/sock.py '{"cmd":"player"}'`.

| cmd | args | effect |
|---|---|---|
| `ping` | — | `{pong,delta,base}` — liveness + ASLR delta |
| `player` | — | `{actor,world_x,world_y,stat_block,hp,hp_max,mp,mp_max,level_base,exp_cur,exp_max}` or null |
| `state` | — | boot/hook diagnostic: `{hooks,main_wnd,launch_clicked,attract_frozen,keepactive,dlgskip,box_open,box_scale,md_state,ti_mgr,pk_mgr}`. `ti_mgr`≠0 ⇒ input poll firing; `box_open` ⇒ a dialogue box is on screen (what dlgskip gates on); `box_scale` = its pop-in 0..1000. |
| `read` | `addr`,`type`(u8/u16/u32),`va`(bool) | read; `va:true` = AP()-relocate a 0x400000 VA |
| `write` | `addr`,`value`,`type`,`va` | write |
| `setstat` | `which`(hp/hp_max/mp/mp_max/level),`value`,`lock`(bool) | set a player stat |
| `god` | `on`(bool) | freeze hp+mp at max every 50 ms |
| `teleport` | `x`,`y`(centi-px),`relative`(bool) | move the player via the **phys-box** (`*(actor+0x40)`): X sticks, Y gravity-settles |
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
