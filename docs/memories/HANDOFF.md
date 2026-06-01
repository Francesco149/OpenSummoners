# Session handoff — last updated 2026-06-01 (title-scene render half, ckpt 10)

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint, don't append.
`docs/PROGRESS.md` is the append-only changelog; this file is "where
to pick up *right now*".

## Where we are

**Milestone 0 (title screen renders) — the whole `FUN_0056aea0` control
flow is now ported.** The 3441 B title scene runner has had its fade FSM,
pacing FSM, update half (input poll + menu chain + spawn + per-frame input
dispatch), **and now its render half** all ported across ckpts 1–10.

**Checkpoint 10 just landed** (this session): the **render half**
(`0x56bb04..0x56bf1a`, the pacer's `sub==1` arm) → `src/title_scene.c` as
**`title_render_step`**, plus the fade→alpha helper `0x448c80` as the pure
**`title_fade_ramp`**. The render step draws one frame for the current phase:
prologue gating (phase 0 → surface reset `0x5b9410`; phases 2–3 → clear
`0x5b9b70`; phase > 10 → skip) → the **recovered 11-entry jump table** at
`0x56bfa4` (`jmp [phase*4+...]`) to 7 inline handlers → the shared frame-end
at `0x56bec4` (compose `0x56c180` → "Title Menu - Flipping" log → Flip
`0x5b8fc0`).

It's heavily DDraw/asset/object-model-coupled — every leaf the handlers call
is unported. Rather than a hook per bridge, they're reported as an **ordered
stream of tagged `title_draw_cmd`s through a single `title_render_sink_hook`**
(no-op by default). The render half's purpose *is* that ordered draw stream,
so the sink is its testable core (dispatch, per-handler sequence, ramp alpha,
sparkle-trail geometry, cursor placement — all asserted without any black-box
draw subsystem).

**Headline finding — new quirk #40:** `0x448c80`'s ramp returns **0 at both
ends** — `fade == 1000` lands on the excluded `idx == 20` (`>= 0x14` cap),
returning 0 not the top entry, so a saturated hold composites via a different
path than the ramp; and its 20-dword table at `0x8a9308` is **all-zero
statically** (DDraw fills it at run time), so a headless port correctly sees
alpha 0 everywhere (modelled as a NULL/empty `ramp` input). Also: the two
intro logos are **container fields at +4/+8**, not `0x418470` assets; and
Ghidra's "call+return" rendering of the `0x56bb55` jump table hid that the 7
handlers are inline labels all converging on the one frame-end.

**527 host tests pass, 0 fail, 6 skip (of 533)** — 9 new (ramp index/clamp,
prologue gating, logo clear-vs-blit, press-button asset pairs, sparkle trail,
menu bg+cursor, fade-out, flip-log-once, no-sink safety; ASan/UBSan clean).
Both 32-bit cross-builds clean. Ledger **122/1490 touched (7.5%), 119 tested**
(unchanged — an extension of the already-counted `FUN_0056aea0`; unported
callees referenced by bare VA).

**Orientation docs (read for the bigger picture):**

- `docs/STATUS.md` — coverage headline (DERIVED). 122/1490 touched, 9.7%
  of bytes, 119 host-tested.
- `docs/ROADMAP.md` — 11-milestone order + subsystem map + port-readiness.
- `docs/findings/title-scene.md` — the title runner's full anatomy; the
  dispatch-table section now has the ckpt-10 per-handler draw table.
- `docs/findings/menu-list.md` — the menu controller: scroll/nav/latch,
  geometry ctor/dtor, the grid-cell finalizer, the menu-node builder + tree.
- `docs/findings/input.md` — the input ring + poll; only the ring
  *producer* remains black-box (it also fills `+0x114/+0x118`).
- `docs/port-frontier.md` — DERIVED "what to port next".
- `findings/engine-quirks.md` #15–**#40**.

**Tooling (run after every port that lands):**

```
python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py
```

**Static disasm:** the canonical unpacked image for r2 is
`vendor/unpacked/sotes.unpacked.exe` (the `vendor/original/sotes.exe`
symlink target is the **packed** Steam build — its `.text` is encrypted, so
r2 reads garbage there). Recipe:
`nix develop --command bash -c "r2 -q -e scr.color=0 -c 'af @ <va>; pdf @ <va>' vendor/unpacked/sotes.unpacked.exe"`.
For a jump table: `pxw <n> @ <table-va>`.

**Structural-parity harness (offline foundation landed 2026-05-29):**
call-graph diff + mem-watch, mirroring `../openrecet`. How-to:
`docs/parity-harness.md`; design: `docs/plans/parity-harness.md`. Offline
pieces done + tested. The agent + `frida_capture.py` call-trace/mem-watch
modes + `tools/bisect_call_trace_vas.py` are code-complete but **need a
live retail-under-Frida run to verify** (human-verification gate).

NB: only put a `FUN_<va>` token in `src/` for a function you have
actually ported — the ledger generator treats any `FUN_<va>` in src as a
port signal. Reference *unported* callees by bare VA (`0x494e10`,
`0x56c610`), not `FUN_...`, or you'll inflate the headline. **Always regen
the ledger and check the count after a port** (ckpt 9 bit us: four unported
callees written as `FUN_` bumped 122→126 until corrected).

## Module inventory (11 modules)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper,
cs_dispatch, app_pump, title_scene (`FUN_0056aea0` — fade FSM + pacing FSM +
the **whole update half** + the **whole render half** (`title_render_step` /
`title_fade_ramp`)), input (`FUN_0043c110`), obj_container (`FUN_00412c10` +
`FUN_00414080`), menu_list (`FUN_004192b0` + `FUN_0043ca40` + `FUN_0043ce50`
+ `FUN_0040f5c0` + `FUN_0040e0c0` + `FUN_00411f40` + `FUN_0040f3e0`) + the
title-menu **spawn block** + **input dispatch** in `title_scene.c`. Live boot
zero DDERR through 10 frames in mode 2. The ported title FSMs + menu chain +
render step are **not yet wired** into a real scene loop in main.c (the
drop-in still uses its own minimal `main_loop_body`).

## Active goal

**`FUN_0056aea0` is fully ported as a pile of tested units** (fade FSM,
pacing FSM, spawn, input dispatch, render step). The remaining milestone-0
work is **wiring those units into one running title-scene runner** and
installing the hooks against the real engine so the scene becomes observable
live (and eventually draws).

## Next move (pick one — recommendation first)

1. **(recommended) Wire the units into a real scene loop** (`main.c` /
   a new `title_scene_run`). Compose, per the outer `do/while` of
   `FUN_0056aea0`: `title_pace_step` → on a `TITLE_PACE_UPDATE` frame run
   the `0x22`→return-6 abort poll then `title_fade_step` (and, in the menu
   phase, `title_menu_spawn` on first entry + `title_menu_input_step`) → on a
   `TITLE_PACE_RENDER` frame run `title_render_step`. Install the hooks
   (SFX/joystick/notify/watchdog from ckpt 9 + the new `title_render_sink_hook`)
   against the real audio/DInput/god-object/DDraw, and drive it from the
   drop-in. The render sink is where the still-unported draw bridges
   (`0x494e10`, `0x418470`, `0x56c610/_4e0/_580/_470`, `0x56c180`,
   `0x5b8fc0`) actually get implemented — start by deciding which to port
   vs. stub for a first "title frame composited" milestone. **This is the
   thing between a pile of units and a running title scene.**

2. **Port a draw bridge** to make the render sink do real work. Best
   first targets: `0x56c180` (frame compose) + `0x5b8fc0` (the DDraw Flip,
   `IDirectDrawSurface7::Flip`, vtable 0x2C — see `ddraw-init.md`) give a
   real "frame committed" event; `0x418470` (asset get) + one of the sprite
   draws (`0x56c610`/`0x56c4e0`) give a real composited sprite. Each is its
   own RE sub-task against the asset/DDraw model.

3. **Live harness gate** — run `bisect_call_trace_vas.py` /
   `mem_watch.py --region <+0x108>:64:input_ring` under Frida to verify the
   call-trace + mem-watch machinery and catch the input-ring producer (the
   DInput `GetDeviceState` writer, vtable `[0x24]`), which also fills the
   `+0x114/+0x118` axis-held flags. Human-in-the-loop.

## Open RE threads (see ROADMAP subsystem map for the rest)

- **`FUN_0056aea0`** title scene runner — **fully ported as units.**
  Remaining: wire into a real runner + implement the draw bridges behind
  the render sink.
- **Render-half draw bridges** (now stubbed behind `title_render_sink_hook`):
  `0x494e10` (logo alpha blit), `0x418470` (asset get), `0x56c610` (plain
  sprite), `0x56c4e0` (leveled sprite), `0x56c580` (sparkle), `0x56c470`
  (cursor highlight), `0x56c180` (compose), `0x5b8fc0` (Flip), `0x5b9410`
  (surface reset), `0x5b9b70` (surface clear). All DDraw/asset/object-model.
- **`0x40fa00`** the cell text-layout / glyph builder (800 B; SJIS parse,
  `#`-colour escapes, font-metric table) — its own text subsystem;
  `menu_row_finalize` calls it via a hook until it lands.
- **SFX `0x411390`** + **joystick `0x5ba120/_290`** + **save-data notify
  `0x41bb80`** + **watchdog `0x40a5d0`** — the four update-half side effects
  `title_menu_input_step` stubs through hooks; port when their subsystems
  come up (audio = milestone 3; DInput pad attach; god-object dispatch).
- **Input** poll + latch + nav **DONE**. Remaining: the **producer** that
  fills the `+0x108` ring (DInput `GetDeviceState`) and `+0x114/+0x118`
  axis-held flags — black box; `mem_watch.py` is the tool.
- **Audio ZDM** `FUN_005bab10`/`_5bc150` + SFX player `FUN_00411390` —
  milestone 3 (WMF/COM, hard).
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4.
- **Hash-id asset directory** `FUN_00556eb0` — recover the ID→name table.
- God-object `DAT_008a9b50`/`DAT_008a6e80` layout (engine-quirks #15) —
  model as we go (the render half reads `DAT_008a93cc->[0x16c]` = primary
  surface, `DAT_008a93ec` = hWnd, `0x8a9308` = the 20-dword alpha ramp).
- Frida turbo: add `GetTickCount` + `WaitMessage` hooks to the agent
  (quirk #29 explains why turbo currently freezes the splash).

## How to apply

When the user says "continue RE work" (or similar):

1. Read this file first, then `STATUS.md` + `ROADMAP.md`.
2. Pick the recommended next move (or whichever the user redirects to).
3. Work port-and-test style: small unit → tests → commit. Each ported
   function gets a `FUN_XXXXXX` provenance comment (the ledger keys on it)
   and a test spot-checking behaviour vs hand-computed expectations. Pin
   retail struct offsets via `_Static_assert` guarded by
   `#if UINTPTR_MAX == 0xFFFFFFFFu`. For a Ghidra-unrecovered jump table,
   recover it in r2 first (`pxw <n> @ <table-va>`). When a decompiled branch
   or a `__thiscall` arg list looks contradictory, **disassemble it** (r2
   `pdf`) to resolve — that's how ckpt 6 caught the dead-alloc, ckpt 7 the
   mis-typed `this`, ckpt 9 the commit/latch inversion, and ckpt 10 the
   jump-table-is-really-a-jmp + the ramp-saturation-to-zero (quirk #40).
4. **Append any engine quirk** you find to `findings/engine-quirks.md`.
5. **Regenerate the derived artifacts** (`gen_port_ledger.py` +
   `gen_frontier.py`) when a port lands — and **check the headline count
   didn't move unexpectedly** (a stray `FUN_` for an unported callee inflates
   it; reference unported callees by bare VA).
6. Update THIS file at each meaningful checkpoint; append to PROGRESS.md.
7. **Suggest a `/clear`** at the natural stop point — the docs are the
   durable memory, not context.
