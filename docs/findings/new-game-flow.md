# New-game flow — scene sequence (retail ground truth)

Recovered by the TAS harness (checkpoint 13): a deterministic input trace
injected into retail's input ring, captured frame-by-frame. This is the
reference path the port must reproduce for "structural parity + pixel accuracy
on clicking through new game → first frame in-game".

## The click-through (verified, deterministic)

| stage | how you get there | what's on screen |
|-------|-------------------|------------------|
| **Title menu** | boot, ~frame 1900 menu interactive | Start / Continue / Bonus Menu / Options / Exit. Cursor (►) starts on **Start** (row 0, action `0x1a`). |
| **New-game config** | press **0x24** (confirm) on Start | boxed menu: **Game Difficulty** (1:Easy) / **Auto-guard** (On) / **Start Game**, with a tooltip box. Cursor starts on Game Difficulty. *Separate scene → separate input-manager instance (engine-quirks #43).* |
| **(nav)** | **id 3** (down) ×2 → cursor on **Start Game** | tooltip: "Confirm options and begin the game. (Options can be altered after the game starts.)" |
| **Elemental Stone intro** | press **0x24** on Start Game | a glowing purple gem (the "Secret of the Elemental Stone") on black. |
| **Prologue narration** | (auto-advances) | scrolling story text: "…the country of Scotsholm… Elemental stones are now mass-produced…", gem centered. |
| **first in-game frame** | *(not yet reached — past the prologue)* | TBD — the prologue is a timed/advance-on-input cutscene; best captured from a **recorded human trace** (distil to a sparse trace) or by REing the prologue sequencer. |

## Button ids per menu (injection truth, engine-quirks #42/#43)

- **Title menu** polls `0x22, 2, 4, 1, 3, 0x24`. Effects: id 1 = up, **id 3 =
  down**, id 2 = page-up (no-op), id 4 = page-down (no-op), **id 0x24 =
  confirm**, id 0x22 = abort.
- **Difficulty config** polls `0x22, 1, 3, 0x24, 0x27`. id 1 = up, **id 3 =
  down**, **id 0x24 = confirm/OK**, **id 0x27 = back/cancel** (NOT a value
  toggle — see the run-loop note below; quirk #65). Pressing confirm (`0x24`)
  on an option row opens that option's **picker submenu** (`FUN_00567ba0`); on
  the **Start Game** row it begins the game.

> **Correction (ckpt 38, quirk #65):** the earlier "id 0x27 = value left/right"
> reading above was a guess and is **wrong**.  Tracing `FUN_00564780`'s run
> loop: `0x565d10` → `0x43bca0` maps button `0x24` → `menu_list_latch(9)` and
> `0x27` → `menu_list_latch(10)`, which net out (via the 9/10→3/4→`0xc`/`0xb`
> mapping) to **`0x24` = confirm (`0xc`)** and **`0x27` = back (`0xb`)**.  There
> is **no in-place value toggle**; an option's value changes only by confirming
> into its picker submenu.  Only the *physical-key identity* of `0x24`/`0x27`
> (what `FUN_0043c110` reads them as) is still worth a live `--input-trace`
> confirm.

## The trace

`tests/scenarios/new-game-through/trace.jsonl` (sparse `{frame, ids}`). Frame
numbers are **Flip frames** (same axis the capture uses). They're timing-
sensitive to the menu fade-in gates (`menu_input_sub.ready` must reach 1000 —
the "+0x54 ramp"); the committed trace uses generous margins. Re-time if the
boot path length changes.

## Builder ported (ckpt 37)

The **config-menu grid builder** for the difficulty scene (`FUN_00564780`
case 0x24) is ported in `src/newgame_menu.{c,h}` and verified to emit retail's
exact `TextOutA` stream (quirk #64): a 3×2 linear grid (Game Difficulty /
Auto-guard / Start Game) at box base (32,32), col origins x=72/232, row pitch
28.  Still a **stub in `app_flow`** — the NEW_GAME arm re-enters the title; the
scene is not yet wired as a runnable drive.  Remaining for the live scene: the
Win32 frame pump (`0x565d10` + the `0x43bca0` input scan), the option picker
submenu (`0x567ba0` default arm), the box widget tree (`0x411940`), and the
Start→game transition (`0x564160`→`0x59ec30`).

## Run-loop model ported (ckpt 38)

The **Win32-free heart** of the case-0x24 run loop is ported in
`src/newgame_scene.{c,h}` (quirk #65), mirroring the `title_scene`/`title_drive`
split: the focused-row **tooltip resolution** (`newgame_option_tooltip` =
`FUN_00566850` for option rows + the kind-3 action-tooltip switch), the
**pump-result → action dispatch** (`0xd`→re-render, `0xc`→confirm-on-row,
`0xb`→back), and the **value-refill** (`newgame_scene_set_option`, the picker's
commit effect).  Host-tested (4 tests).  The pump itself (`0x565d10`), the
picker submenu, and the box widgets are the drive's job — the next unit.

## Box chrome — retail ground truth (ckpt 40, live `--box-probe`)

The bordered cream panels behind the menu + tooltip are a **9-slice sprite box**,
captured live by hooking the render path (new `frida_capture.py --box-probe`,
gated to the menu flip window).  Two distinct systems compose the chrome:

**1. The 9-slice panel** — renderer **`FUN_0048cf80`** (the tiled 9-slice; sibling
`0x48cb90` is the fade-scaled variant), bank **PE resource `0x457`** (= 1111,
sotesd.dll; scene field `*(this+0xb88)`, embedded at box-node `+0x5c`).  The 9
slice frame ids live at node `+0x60..+0x72` and are, in order, **tl=0, top=1,
tr=2, lmid=3, center=4, rmid=5, bl=6, bottom=7, br=8** — each a **32×32** cell
(`+0x74`/`+0x78` = corner cell w/h).  The center (frame 4) is the **cream
RGB(239,227,214)** fill, the edges (1/3/5/7) the bevel, the corners (0/2/6/8) the
ornate gold filigree.  Edges + center are **tiled** (repeat 32px), not stretched.
The panel fades in via the node `+0x54` ramp passed as the blit alpha (captured
at `fade=50`).  Two instances, both bank `0x457`, both built by `FUN_00411940`
(case-0x24 calls it twice):
  - **menu box**:    rect **(32,32)** size **400×124**
  - **tooltip box**: rect **(32,392)** size **576×80**
These match the golden's measured bounds (top box x32–431/y29–155; bottom box
x32–607/y392–471).

**2. The animated sparkle corner** — renderer **`FUN_0048d940`** (single-sprite
cell), bank **PE resource `0x3e8`** (= 1000; scene field `*(this+0xb8c)`, box-node
`+0x28`).  A type-1 node at the box's **top-left** corner (≈dst (44,29), ~22×41),
**base frame 16** cycling frame-list **[0,1,2,3] → sprite frames 16–19** (the
twinkle animation, idx in `+0x72`).  A decorative overlay on top of the static
corner — secondary to the panel.

Ground truth saved: `goldens/retail-newgame-box-cells.jsonl` (the `box_frame` +
`box_cell` capture).  Neither bank field (`+0xb88`/`+0xb8c`) is written as a
literal offset anywhere in the 1768-fn corpus — they're set by an embedded
sub-object ctor — so the **live probe is the only way** to learn the resource
ids; the slot's `resource_id` (`slot+0x40`) was read straight off the actively-
rendering bank.

**Harness caveat:** entering the new-game scene, the **Flip counter freezes**
(the modal pump `0x565d10` doesn't advance the hooked Present), so flip-gated
probes only see the **title→menu transition** (flips ~410–422) — which is exactly
when the box first renders, so it was captured.  Steady-state re-renders are
invisible to a flip-gated probe here.

**Port plan (next):** register banks `0x457` + `0x3e8` at boot (the
`ar_register_main_sprites` pattern, sotesd HMODULE), port the `0x48cf80` tiled
9-slice render, build the two boxes, draw them behind the menu/tooltip text
(replacing `newgame_render`'s placeholder `PatBlt(BLACKNESS)`), then `differ_px`
the text region → 0.

## Option picker submenu ported (ckpt 45, quirk #71)

Confirming on a kind-0 option row (Game Difficulty / Auto-guard) opens that
option's **picker submenu** — `FUN_00567ba0`, a nested 1-column value-grid with
its own `0x565d10` nav loop, a **blocking modal call** from the parent's run loop
(564780.c:612).  Ported pure + host-tested in **`src/newgame_picker.{c,h}`**:
`newgame_picker_values` (`FUN_00568320` — id 3 `{10,20,30,40}`/`{..,50}` unlock,
id 4 `{0,1}`), build the value grid, seek the cursor to the current value
(`FUN_00419900`), nav/commit/cancel.  Wired into `newgame_drive` as a frame-
stepped modal submode; on COMMIT it calls `newgame_scene_set_option(id, chosen)`
(the parent's value-refill).  Rendered port-side at (288,128) over the menu;
**user-confirmed visually correct** (open → nav → commit re-lays 1:Easy→2:Normal).

The picker's `__thiscall` arg lists (the `FUN_00412160` row kind, `FUN_00419900`
seek, `FUN_005657f0` commit) were decompiler-lost → reconstructed from the
callees' contracts (see `newgame_picker.h`).  A live **retail** golden is
**unreachable**: the flip counter freezes in `0x565d10`'s modal pump and both the
harness's capture + input injection are flip-keyed.  So the box geometry
(288,128/256) and the args are an OPEN pixel-verification gate.

## Open

- **Picker bit-exact gate.** The open picker can't be captured/driven in retail
  (flip-frozen modal pump, quirk #71).  Closing it needs a harness that hooks
  `0x565d10`'s own present + feeds its input directly (not flip-keyed).
- **Prologue → first playable map.** The opening cutscene (stone + narration)
  needs either a recorded human trace (advance/skip presses with real timing)
  distilled to a sparse trace, or RE of the prologue sequencer. This is the
  remaining gap to "first frame in-game rendered".
- The difficulty/auto-guard value toggles (id 0x27) are unverified directionally
  (left vs right vs cycle).
