# Title / main-menu state machine (EN-SE) + input injection

RE'd against `sotes-trainer-oss.exe` (unpacked EN-SE) — the title screen's runner, its input path, and
the button-injection mechanism the sotes-mod-loader `mod.game.input.press` / the trainer's menu-drive
use. Motivation: drive the title menu programmatically (auto-load a save; arbitrary UI navigation) and
fix the "auto-load starts a NEW game when the title defaults to Start" bug.

## The routines

| SE VA | role |
|---|---|
| `0x581ba0` | title/load **dispatcher** — sets the title up per resolution (`*0x92af98 → [0x24]` enum: writes render size at `this+0x14/0x18` = 0x280×0x1e0 / 0x500×0x3c0 / 0x780×0x5a0), then runs the menu; switches on the chosen action (0x1a Start / 0x1c Continue / 0x1d Special / 0x1e Option). |
| `0x582c40` | the **title-scene runner** (3.4 KB). Allocs its scene objects (`0x5ef121` alloc), builds the UI, and runs a per-phase state machine. `ecx` at entry = the title scene `this` (`title_this`). |
| `0x437c70` | the per-frame **input poll** (the loader's "safepoint"). The title calls it as `poll(mgr, now, button_id)` — e.g. `push 0x22; push ebp(now); call 0x437c70` at `0x5830b3` (button 0x22 = back). `mgr = *(title_this + 4)`. |
| jump table `0x5842f8` | 13 dwords indexed by the title's phase var (`edi`, a stack local at `0x14(esp)`): `0x583168, 583333, 583363, 583383, 5833a3, 5833e6, 584323, 58344a, 58348e, 5834d2, 58356f, 58356f, 583896`. This is the phase dispatch (`jmp *0x5842f8(,%edi,4)` at `0x583161`, guarded by `cmp 0xc,%edi; ja …`). |

Related globals: `0x92dd4c` = a scene/save `this` (references the save-cipher keystr table `0x5fd290`
at `+0x54`); `0x92af98` = options obj (`[0x24]` resolution); `0x92af7c` title condition flag;
`0x92af98[0]+0x158` classic/SE title selector.

## Input injection (VERIFIED — the mechanism `mod.game.input.press` exposes)

The engine polls buttons through a **64-slot record ring on the input manager**:
- `mgr = *(title_this + 4)` — the object `0x437c70` gets as `ecx`. The loader captures it live at the
  safepoint (`exec_ti_mgr`; this session `0x0f1097c0`, moves per run).
- ring at **`mgr + 0x0c`**, 64 dword slots; **slot 63 (`mgr+0x108`) is polled first** (the title scans
  `mgr+0x108` downward for 0x3f entries, `0x583125`+).
- each slot points at a record **`{ id@+0, now@+4, state@+8 }`**; the poll matches `id` + `state==1` +
  **`(poll_now − record_now) <= 0x64`** (freshness).  `poll_now` = the poll's `now` arg (`exec_sp_now`).
- **button ids** (SotES title): **`0x25` confirm** (PROVEN — the loader's `save.load` injects it and
  reaches the save picker), `0x22` back/abort. Rotate up/down is `2`/`4` or `1`/`3` (the two RE notes
  disagree; unresolved — see below).

`mod.game.input.press(id[,mgr])` writes one such record; `save.load` uses the same path for confirm.

## The selection CURSOR — NOT located yet (the open problem)

Driving the title needs to know / set which item is highlighted (so confirm picks Continue, not
Start). **The cursor is NOT in the input manager**: injecting every rotate id (1/2/3/4) at the title
and diffing `mgr[0..0x400]` every frame showed **the input manager is completely static** — only our
own ring writes at `+0x108` change. So the menu cursor / selected-row lives in the **title scene
object (`title_this`) or a menu-list controller it owns**, not the input device wrapper.

Leads to pin it (a follow-up):
- Locate `title_this` (the object with `*(title_this+4) == mgr`) and diff IT across a rotate injection.
- OR trace `0x582c40` past `0x583200` (the menu-build + the phase handlers off `0x5842f8`) to the
  field that holds the selected row — the base-game analog is the menu-list controller cursor at
  `ctrl+0x174 → header+0x14` (`docs/findings/menu-list.md`); confirm whether the SE title reuses it.
- Probing must happen in the **fresh interactive phase** (right at boot, while the menu polls nav) —
  an idle title with the attract/demo frozen appears to sit in a non-nav phase (nav injections there
  produced no state change at all, while confirm at boot does advance).

## The auto-load "defaults to Start" bug

`mod.game.save.load` freezes the attract/demo then injects `0x25` confirm at the title every frame
until the save picker opens (`0x4378d0` hook) — which works **only because the title defaults to
Continue** in the common case. If the last boot started a NEW game, the title defaults to **Start**,
so the injected confirm commits Start → a new game, and the drive then sees a scene load and wrongly
reports success.

**Fix path (needs the cursor):** before confirming, set the cursor to the Continue row (or inject
rotate until the selected action is `0x1c`). The building block — button injection — is in place
(`mod.game.input.press`); the missing piece is reading/writing the cursor (above). Until then the
drive should at least DISTINGUISH "reached the picker" (real load) from "scene loaded without the
picker" (new game) and warn — a cheap safety net.
