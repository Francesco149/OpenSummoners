# Input subsystem — the event ring and its poll

> Milestone 1.  This is the *read* side, recovered and ported (checkpoint
> 3).  The *producer* (`FUN_0046a880`, the DInput path that fills the ring) is
> now RECOVERED — see "The producer" below.

## The event ring (`input-manager + 0x108`)

The input-manager object owns a fixed **64-entry ring of pointers to event
records**, laid out at `manager + 0x0c .. manager + 0x108` (64 dwords, the
last at `+0x108`).  Each pointed-to record is (at least) three dwords:

| record off | meaning                                              |
|------------|------------------------------------------------------|
| `+0x00`    | button / key id (zeroed when the event is consumed)  |
| `+0x04`    | `GetTickCount()` timestamp of the event              |
| `+0x08`    | state flag — `1` means "pressed"/active              |

The ring is evidently written so that the **newest event sits at the highest
index** (the poll scans top-down and takes the first match — see below).

## The poll (`FUN_0043c110`, ported `input_poll_consume`)

84-byte `__thiscall(now, button_id)`.  "Was `button_id` pressed within the
last 100 ms, and if so consume it."

```
for (i = 63; i >= 0; i--)                       // newest slot first
    rec = ring[i];
    if (rec.id == button_id && rec.flag == 1
        && (uint32_t)(now - rec.ts) <= 100) {   // unsigned: rollover-safe
        rec.id = 0;                             // consume-on-read
        return 1;
    }
return 0;                                        // no recent press
```

Three load-bearing behaviours — all covered by quirk #30:

1. **Consume-on-read** — a hit zeroes `rec.id`, so one press satisfies at
   most one poll.  The menu polls several buttons per frame off the same
   ring without a press double-registering.
2. **100 ms unsigned recency window** — `(uint32_t)(now - ts) <= 100`;
   future/stale timestamps underflow and are rejected, so the 49.7-day
   `GetTickCount` rollover is handled for free.
3. **Newest-index-wins** — scan starts at slot 63 (`+0x108`) and decrements.

Ported pure in `src/input.{c,h}` (zero engine callees); 10 host tests.
Retail offsets pinned via `_Static_assert` on the 32-bit build.

## The wider manager fields + the skip-splash flush (`FUN_0056aea0`)

The poll only reads the ring, but the title scene's **skip-splash early-out**
(`0x56b0e8..0x56b150`, "press a button to skip the intro") touches more of the
manager — and its field-clear block (`0x56b25e..0x56b29a`) maps the layout past
the ring:

| manager off    | shape           | meaning / use                                    |
|----------------|-----------------|--------------------------------------------------|
| `+0x10c`       | dword           | flushed by skip-splash (semantics tbd)           |
| `+0x110`       | dword           | flushed by skip-splash (semantics tbd)           |
| `+0x114`       | dword `array_A[11]` | A[0] = vertical axis-held, A[1] = horizontal; rest tbd |
| `+0x140`       | dword `array_B[11]` | parallel to A; semantics tbd (flushed together)  |
| `+0x16c`       | half-word       | flushed by skip-splash (semantics tbd)           |

So the two "axis-held" flags the title menu reads at `+0x114` / `+0x118` are just
`array_A[0]` / `array_A[1]` — see **engine-quirks #41**.  The flush also zeroes
every ring slot's id (a wholesale event-consume) before forcing phase 8.

Two primitives port the skip-splash's input side (`src/input.c`):

- **`input_any_fresh_press(m, now)`** — the scan (`0x56b119..0x56b144`): same
  newest-first walk as the poll, but matching *any* pressed id (`rec.id != 0`),
  read-only.  "Did the player press anything?"
- **`input_mgr_reset(m)`** — the flush (`0x56b25e..0x56b29a`): zero every ring
  slot id, both 11-dword arrays, and the `+0x10c`/`+0x110`/`+0x16c` fields.

## Button ids the title menu polls

From the `FUN_0056aea0` default branch (see `findings/title-scene.md`
"Input dispatch"), each menu frame polls these ids and feeds the hits into
the action latch `FUN_0043ce50` → nav engine `FUN_0043ca40`.

> ⚠ **The latch-dir name ≠ the cursor effect** (engine-quirks #42, confirmed
> live by ring injection).  The nav engine dispatches dir **0 = up, 1 = down,
> 2 = page-up, 3 = page-down**.  So the *cursor* effect of each polled id is:

| id     | latch dir | **cursor effect**             | for scripting |
|--------|-----------|-------------------------------|---------------|
| `0x01` | 0         | **up** (prev)                 | UP            |
| `0x03` | 1         | **down** (next)               | **DOWN**      |
| `0x02` | 2         | page-up (no-op, single column)| —             |
| `0x04` | 3         | page-down (no-op)             | —             |
| `0x24` | 9         | commit → title-confirm (#39)  | **CONFIRM**   |
| `0x22` | —         | abort poll → scene state 6    | QUIT          |
| (none) | 4/5/6/7   | axis-held synth via `array_A[0]/[1]` (`+0x114/+0x118`) | (held repeat) |

**To script the title menu: up = id 1, down = id 3, confirm = id 0x24.**
(The old labels here read "0x02 = down / 0x03 = left" straight off the latch
dir numbers; id 2 is actually page-up and id 3 is the real down.)

The new-game **difficulty config** sub-menu is a *separate scene with its own
input-manager instance* (engine-quirks #43) and polls a different id set —
`0x22, 1, 3, 0x24, 0x27` (down is still id 3; 0x27 is the value left/right).

## The producer (`FUN_0046a880`) — RESOLVED (ckpt 132)

The ring WRITER.  It walks the currently-pressed DInput keys (each `uVar9` a
pressed-key handle) and, per key, reads its scancode (`FUN_005ba3a0`), pressed
flag (`FUN_005ba3d0`) and timestamp (`FUN_005ba3f0`), then for each ACTION the
key is bound to **pushes a ring event**: shift the 64-slot `+0x108` ring down one,
write the new record at the head (`+0xc`) with `id`, `ts`, `flag` (1=press, the
`+0x00`/`+0x04`/`+0x08` shape this doc already documents).  So ONE physical key
can post SEVERAL ring ids in a frame (the fan-out below).

Two binding halves:

- **FIXED keyboard scancodes** (always active, literal `if (scancode == k)`):

  | DIK scancode        | ring id | action  |
  |---------------------|---------|---------|
  | `0x1c` **RETURN**   | `0x24`  | CONFIRM |
  | `0x01` ESCAPE       | `0x27`  | cancel  |
  | `0x0e` BACKSPACE    | `0x27`  | cancel  |
  | `0x0b` `0`          | `0x21`  | (quit?) |
  | `0xc8/0xd0/0xcb/0xcd` arrows | 1/3/2/4 | dir |

- **CONFIGURABLE buttons** (scancode read from the `*DAT_008a6e80` launcher
  config, each gated by its `+enable` dword).  Each button fans out to several
  ring ids:

  | config key off | enable off | ring ids posted          | default key (port) |
  |----------------|------------|--------------------------|--------------------|
  | `+0x558`       | `+0x548`   | `8`, `0x25`, **`0x24`**  | **X** (attack)     |
  | `+0x574`       | `+0x564`   | **`7`**, `0x27`          | **C** (jump)       |
  | `+0x5ac`       | `+0x59c`   | `0x26`, `0x24`           | —                  |
  | `+0x590`       | `+0x580`   | `9`, …                   | **Z** (sheathe)    |

So the dialogue/menu **CONFIRM (ring `0x24`) is ENTER _or_ X** — ENTER via the
fixed binding, X via the `+0x558` config button (which is also the attack key).
**Z is NOT a confirm** (USER ckpt 132): by elimination it is the `+0x590` button
(→ ring 9, the sheathe action), not the `0x24`-posting `+0x5ac`.  Ported to the
port's reduced live producer `src/input_live.c` (`KEYMAP`): ENTER + X → ring
`0x24`, C → ring 7, Z has no confirm role.  The per-button SCANCODE *defaults*
(which physical key fills each config slot) remain `PORT-DEBT(keybind-config)` —
the `*0x8a6e80` launcher config isn't modeled; the X/C/Z assignments above are
the port's existing annotations (`input_live.h`), USER-confirmed for the confirm.

## Open

- ~~**Action latch `FUN_0043ce50`** and the **cursor-nav engine
  `FUN_0043ca40`**~~ — **DONE** (checkpoint 4).  The jump table Ghidra
  mis-rendered was recovered via radare2; both ported to `src/menu_list.c`.
  The poll → latch → nav chain is complete.  See `findings/menu-list.md`.
- **Lazy joystick attach** — DInput pads are enumerated on first menu
  *confirm* (`FUN_005ba120`), not at boot.  A headless run that never
  reaches "press start" never calls it; that's expected, not a bug.
