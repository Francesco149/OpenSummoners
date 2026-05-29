# Input subsystem — the event ring and its poll

> Milestone 1.  This is the *read* side, recovered and ported (checkpoint
> 3).  The *producer* (the DInput `GetDeviceState` path that fills the
> ring) is still a black box — see "Open" below.

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

## Button ids the title menu polls

From the `FUN_0056aea0` default branch (see `findings/title-scene.md`
"Input dispatch"), each menu frame polls these ids and feeds the hits into
the action latch `FUN_0043ce50`:

| id     | meaning                              | latch arg |
|--------|--------------------------------------|-----------|
| `0x02` | down                                 | 2         |
| `0x04` | right                                | 3         |
| `0x01` | up                                   | 0         |
| `0x03` | left                                 | 1         |
| `0x24` | back / cancel                        | 9         |
| (none) | axis-held synthesis via `+0x114/+0x118` flags | 4/5/6/7 |

## Open (still black box)

- **Producer.** Who calls `IDirectInputDevice7::GetDeviceState` (vtable
  `[0x24]`) and writes the `+0x108` ring?  Point
  `mem_watch.py --region <+0x108 addr>:64:input_ring` at a live retail run
  to catch the writer (the milestone-1 human-verification gate).
- **Action latch `FUN_0043ce50` (220 B)** and the **cursor-nav engine it
  calls, `FUN_0043ca40` (970 B)** — object-model-coupled and partly
  unrecovered (Ghidra mis-rendered `FUN_0043ca40`'s jump table).  Deferred;
  these turn a raw button hit into a menu action / cursor move.
- **Lazy joystick attach** — DInput pads are enumerated on first menu
  *confirm* (`FUN_005ba120`), not at boot.  A headless run that never
  reaches "press start" never calls it; that's expected, not a bug.
