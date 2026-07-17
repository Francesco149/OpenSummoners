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
nix develop --command i686-w64-mingw32-gcc -shared -O2 -s -Wall -static -static-libgcc \
  -o build/sotes_trainer.dll tools/sotes_trainer/trainer.c -lws2_32
```

`-static -static-libgcc` is REQUIRED — a default mingw build imports
`libgcc_s_sjlj-1.dll`, absent in the game's DLL search path, so `LoadLibrary` fails.

## Inject

- **Dev loop** (composes with the throwaway Frida probe rig): set
  `TRAINERCTL_INJECT_DLL='C:\...\sotes_trainer.dll'` before launching
  `scratchpad/trainerctl.py` — it `LoadLibrary`s the DLL in-target after resume.
  Or one-shot: Frida `Module.getGlobalExportByName('LoadLibraryA')` with the DLL's
  **Windows** path (`scratchpad/inject_test.py`).
- **Ship**: `build/inject.exe` (CreateProcess-SUSPENDED → remote LoadLibrary → Resume)
  or a `version.dll` proxy (the ennse_voice pattern).

On attach it logs to `oss_trainer.log` beside the exe and binds `:7777`.

## Drive (the LLM / UI interface)

Line-delimited JSON over TCP. From WSL, connect to the Windows host over the LAN
(`cutestation.soy:7777`) — loopback won't cross the WSL2 NAT (the DLL binds
`INADDR_ANY` for this; a shipped build should default to loopback). Helper:
`scratchpad/sock.py '{"cmd":"player"}'`.

| cmd | args | effect |
|---|---|---|
| `ping` | — | `{pong,delta,base}` — liveness + ASLR delta |
| `player` | — | `{actor,world_x,world_y,stat_block,hp,hp_max,mp,mp_max,level}` or null |
| `read` | `addr`,`type`(u8/u16/u32),`va`(bool) | read; `va:true` = AP()-relocate a 0x400000 VA |
| `write` | `addr`,`value`,`type`,`va` | write |
| `setstat` | `which`(hp/hp_max/mp/mp_max/level),`value`,`lock`(bool) | set a player stat |
| `god` | `on`(bool) | freeze hp+mp at max every 50 ms |
| `teleport` | `x`,`y`(centi-px),`relative`(bool) | write world_x/y |
| `unlock_all` | — | drop god + all locks |

## Verified mechanics (EN-SE, VA = fileoff + 0x400000)

Player anchor = scan for the `0xc35a` actor (ctor `0x419b00`, code @ `+0x1d4`), strong
cross-validation of stat block + world coords. Offsets: `world_x 0xc76c`,
`world_y 0xc770`, `stat_block 0x760`; stat block `hp_cur 0x54`, `hp_base 0x58`,
`hp_equip 0x84`, `hp_buff 0x9c` (**max = sum**), `mp_cur 0x5c`/`0x60`/`0x88`/`0xa0`,
`level 0xe0`. Verified live vs HUD (Lv3 / HP140 / MP34). Full mechanics table +
menu/load/attract RE: `DESIGN.md`.
