# Title / main-menu state machine (EN-SE) + input injection

RE'd against `sotes-trainer-oss.exe` (unpacked EN-SE) — the title screen's runner, its input path, and
the button-injection mechanism the sotes-mod-loader `mod.game.input.press` / the trainer's menu-drive
use. Motivation: drive the title menu programmatically (auto-load a save; arbitrary UI navigation) and
fix the "auto-load starts a NEW game when the title defaults to Start" bug.

> ## ⚠️ CORRECTION (2026-07-20 pm) — parts of the ORIGINAL text below are BASE-GAME addresses, WRONG for EN-SE
> The "generic `menu_ctrl` controller" section cross-referenced `menu-list.md`, which was RE'd on the
> BASE-GAME unpack (`sotes.unpacked.exe`, `9e0324..`) — a DIFFERENT lineage from the EN-SE shipping exe
> (`sotes_en.exe` == `sotes-trainer-oss.exe`, `bed4e1..`).  Those function VAs do NOT match EN-SE and
> **crashed the loader** when hooked (`0x43ce50` is a mid-instruction *struct-init* in EN-SE; MinHook's
> 5-byte patch corrupted `mov %edx,0x14878(%esi)` @ `0x43ce4b` → wild `WRITE @ 0x15F3B468`, the logged
> fault).  Disasm-verified CORRECTIONS (all in `bed4e1..`), used by the WORKING loader auto-load:
>
> | claim below | ❌ base-game | ✅ EN-SE (verified) |
> |---|---|---|
> | menu nav/action fn (capture menu_ctrl here) | `menu_list_latch 0x43ce50` / `menu_list_nav 0x43ca40` | **`0x5e7fe0`** (thiscall; ecx=menu_ctrl, arg=action 0/1 nav, **9 confirm**) → mode-1 handler `0x5e7ad0` |
> | title input-poll (button consume) | `input_poll_consume 0x43c110` / `0x56b8cc` | **`0x437c70`** (== the loader safepoint; `poll(mgr, button, now)`, scans `mgr+0x108` down) |
> | **title COMMIT button** | `0x24` | **`0x25`** (`push 0x25;call 0x437c70` @ `0x583714` → action 9). The title does NOT poll `0x24`. It polls `0x22` back, `0x02`/`0x04` nav, `0x25` confirm. |
> | save-DATA list confirm poll | (assumed `~0x436xxx`) | **`0x4378d0`** called from `0x586a13` (ret `0x586a18`) — the save-list scene handler |
>
> WHAT IS STILL CORRECT for EN-SE: the input-record **ring** mechanism (`mgr+0x0c`, slot 63 first, `{id,
> now,state}` + `(poll_now−rec_now)<=0x64` freshness); the menu_ctrl **layout** (cursor=`*(*(ctrl+0x174)+
> 0x14)`, rows@`ctrl+0x17c` ×0x10 action@+0x04, count@`list+0x10`) — base-game and EN-SE SHARE the layout,
> only the FUNCTION VAs differ; and the title **rows** `1a/1c/1e/1d/8` (Start/**Continue**/Option/Special/
> Exit, built via `0x5e88c0`), Continue = row 1.  The title-scene runner VAs (`0x582c40`, `0x581ba0`,
> phase table `0x5842f8`) ARE EN-SE (found live).  See sotes-mod-loader `core/sotes_bindings.c` (the
> working injection-only drive) + the loader's memory `sotes-live-re-approach.md`.

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
- **button ids** (SotES title — disasm-verified at the poll→latch sequence `0x56b80f..0x56b8e3`,
  2026-07-20): **`0x24` confirm/commit** (`input_poll_consume(0x24)` @ `0x56b8cc` → `menu_list_latch`
  dir 9 → nav returns 3 → commit `rows[cursor]`), `0x22` back/abort, **`1` up / `3` down** (`2`/`4` are
  page-up/down = no-ops on the single-page title). **`0x25` is NEVER polled by the title** (byte-scanned
  the whole title fn — zero `push 0x25`); the earlier "`0x25` confirm" note was wrong — the loader's old
  `save.load` injected `0x25` at the title, which `input_poll_consume`'s exact-id match ignored.

`mod.game.input.press(id[,mgr])` writes one such record; `save.load` uses the same path for confirm.

## The selection CURSOR — RESOLVED (2026-07-20)

The cursor is **NOT in the input manager** (injecting rotate ids and diffing `mgr[0..0x400]` showed it
static).  It lives in the title's **`menu_ctrl`** — the generic selection controller of
`findings/menu-list.md` — which the SE title **does reuse** (the base-game analog was the right lead):
the title drives it via `menu_list_latch` (`0x43ce50`) / `menu_list_nav` (`0x43ca40`).

- **cursor** = `*(*(menu_ctrl+0x174)+0x14)` (list header `+0x14`); `count` = `list+0x10`; `stride` =
  `list+0x0c`; `sel2` (page-top) = `list+0x18`.  **rows** = `*(menu_ctrl+0x17c)` (each `menu_row`
  `0x10` B, `action` @ `+0x04`).  The title's rows are `0x1a,0x1c,0x1e,0x1d,8`
  (Start/**Continue**/Option/Special/Exit), so **Continue = the row whose `action == 0x1c`** (index 1).
- **getting `menu_ctrl` live:** it is the `ecx` of `menu_list_latch` (`0x43ce50`).  Entry proven by
  disasm: `mov esi,[ecx]; cmp [esi+0x54],0x3e8` (the `sub->ready==1000` gate), then branch on `[ecx+8]`
  (mode) and load `[ecx+0x170]` (list2) — exactly the `menu_ctrl` layout.
- **Caveat (why the hook, not pointer-chasing):** `menu_ctrl` is not a findable global (the all-memory
  scan for a pointer == the input mgr returned zero hits), AND `menu_list_latch` runs **only when a
  button is consumed** (`input_poll_consume` @ `0x43c110` returns non-zero) — so at an idle title
  nothing calls it.  The live recipe: **inject a nav (id `3`) to provoke one latch call, capture the
  `ecx`, then write the cursor directly and commit with `0x24`.**  (This also explains the old note that
  "nav-injection at the idle title did nothing": nav only moves once `sub->ready==0x3e8`, and its effect
  is on the `menu_ctrl` — never on the static input mgr that was being diffed.)

## The auto-load "defaults to Start" bug — FIXED (2026-07-20)

`mod.game.save.load` freezes the attract/demo then drives the title into a save.  The old code injected
`0x25` at the title (a no-op — see button ids above) with no cursor control, so it could only warn.
**Now** (`sotes-mod-loader core/sotes_bindings.c`) it:

1. hooks `menu_list_latch` (`0x43ce50`) to capture the live title `menu_ctrl`, validated (mode 1 + a
   Start `0x1a` row present) so it never grabs an unrelated menu that happens to be latching;
2. injects nav id `3` until that capture lands (the latch only runs on a consumed button);
3. writes the cursor (`*(list+0x14)`) to the **Continue** row (`action == 0x1c`), keeping `sel2`
   (`list+0x18`) = `floor(cursor/stride)*stride`;
4. once `sub->ready == 0x3e8`, injects **`0x24`** (the real commit) → commits Continue → the save-data
   list, regardless of what the title defaulted to.

If the title has a Start row but **no** Continue row (no save exists), it refuses to commit (which would
start a NEW game) and stops with a clear log line.  The picker step (`0x4378d0`, a separate input path)
is unchanged.  **Remaining:** end-to-end in-game confirmation of the full drive (capture → cursor →
commit → picker → load).
