# EN-SE trainer — LLM-operable live game manipulation

A Frida side-car for the retail **English Special Edition** (`sotes_en.exe`, Steam buildid
`23890965`, ImageBase `0x400000`) that lets a human **or an LLM** poke the game to set up test
scenarios fast: god mode, max coins/stats, spawn any monster, dump/diff memory to find new
offsets, and fire synthetic sounds. Built for the JP-voice-patch monster-sound investigation
(`docs/findings/ense-voice-monster-se-drop.md`) but general-purpose.

**The game exe is never modified** — this attaches to a running process over Frida and writes
process memory live.

## Run

Attach to any live `sotes_en.exe` (plain Steam launch, or the voice-patch probe
`tools/ennse_voice/repro_probe.py --patched --interactive --show-window` for input+screenshots):

```
nix develop --command python3 tools/ennse_trainer/trainer.py
```

Line-delimited JSON on stdin, one response per line on stdout. Agent events
(`actor_created`, `sound_test_done`, `freeze_dead`) stream interleaved. It **composes** with the
voice probe: shared engine entries show up in `preHooked` and everything still works (Frida chains
interceptors). When both run, use ONE of them for spawn forcing (both maintain separate queues).

Env: `OPENSUMMONERS_FRIDA_REMOTE` (default `cutestation.soy:27042`).

## Commands

| cmd | args | effect |
|---|---|---|
| `read` | `addr`, `type` (`u32`/`u16`/`u8`/`i32`/`f32`/`ptr`) | read one value |
| `write` | `addr`, `value`, `type` | write one value |
| `readblock` | `addr`, `size` | dword array (offset discovery) |
| `hunt` | `value` | every aligned RW dword == value (heap-wide, cap 20000) |
| `winnow` | `addrs`, `value` | keep those now == value (narrow after a change) |
| `lock` | `name`, `addr`, `value`, `type` | rewrite every 50 ms (freeze) |
| `unlock` / `locks` | `name` / — | drop / list freezes |
| `snap` | `name`, `addr`, `size` | snapshot a block |
| `snapdiff` | `name`, `mode` (`dec`/`inc`/`any`) | fields that changed since snap |
| `actors` | `monsters` (bool) | live actor registry (owner, code, forced) |
| `actorclear` | — | reset the registry |
| `forcespawn` | `target`, `source` (0=any monster), `count` | next N spawns become `target` |
| `forceclear` | — | drop the force queue |
| `soundtest` | `key`, `action` | construct a sound set + play through the real mixer |

## Finding a value you can't address yet (the hunt→winnow loop)

Current HP, coins, etc. are NOT at a fixed VA (heap objects move per run). Nail one live:

1. `{"cmd":"hunt","value":90}` while the stat reads 90 → a few hundred candidates.
2. Change it in-game (take a hit; buy something), then
   `{"cmd":"winnow","addrs":[…previous…],"value":89}` → usually one address.
3. `{"cmd":"lock","name":"hp","addr":"0x…","value":90}` — frozen (god mode / infinite coins).

`snap`+`snapdiff` is the alternative when you know the value lives inside a known object (e.g. an
actor): snapshot the block, act, `snapdiff mode=dec` for a stat that dropped.

**Known:** current HP is NOT inside the actor object (the 0x15b90-byte block); hunt+winnow it.
Coins ("$", cap 100 in MD) hunt the same way. Watch out: an actor field at +0xB638 held the wallet
value in one run — always winnow across a real change before locking.

## Spawning the monster you want, where you are

`forcespawn` rewrites the entity code (arg 6 of the character constructor `0x419b00`) for the next
`count` constructions in the monster band, so **the next monsters that spawn** (walk to the next
room / floor) come out as your target. It can't rewrite a monster that already exists.

Monster sound-set keys (== entity code for these) from the def table `0x65b0e8`:

| monster | code |
|---|---|
| Ghost Warlock | `0xC78B` (51083) → sound key `0xC789` |
| Black Harpy | `0xC794` (51092) → key `0xC792` |
| Babymage | `0xC829` (51241) |

`{"cmd":"forcespawn","target":51083,"source":0,"count":4}` → next 4 monsters are Ghost Warlocks.

## Monster action codes (for `soundtest`)

`a73/a74` = attack, `a9` = damage/hit, `a10` = cast/sing, `a12` = death, `a7` = evasion/backstep.
`{"cmd":"soundtest","key":51081,"action":12}` fires the ghost death scream through the real
DirectSound path — `result:1` = played. (Serviced on the game thread at the next engine sound
call, so it needs the game to be actively running, not paused on a modal.)

## Notes / limits

- Engine entries are byte-signature-validated at init; a build change (different Steam buildid)
  will fail validation loudly. Re-derive against `docs/plans/ennse-voice-patch.md`.
- Teleport (position write) is stubbed pending the player world-x/y offset — discover it with
  `hunt` on the on-screen coordinate then wire a `lock`/`write`, or add a `player` command once
  the offset is pinned.
- Live-probe kills are TARGETED (project rule): stop via `{"cmd":"quit"}`; never sweep by name.
