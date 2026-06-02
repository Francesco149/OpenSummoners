# OpenSummoners — Progress log

Append-only changelog.  Newest entries first.  Each entry: date + heading,
then 1–3 short paragraphs.  Cross-link to `docs/findings/*.md` and
specific commits where relevant.

---

## 2026-06-02 (ckpt 24) — port-side input replay (`--input-trace`)

Ported `input_trace.{c,h}` (commit `50b348d`) — the port-side counterpart of
the Frida harness's deterministic input injection. It parses the *same* sparse
`{"frame":N,"ids":[..]}` JSONL the harness writes (`tools/frida_capture.py`) and
replays it into the title-scene drive's input ring as fresh presses
(`{id, flag=1, ts=now}`, round-robin slots), keyed on the present/Flip count.
So a scripted scene walk (e.g. the new-game path in
`docs/findings/new-game-flow.md`) can drive the *port* deterministically — the
port-side half of the replay → port-frame-capture → golden-diff pipeline.

`main.c` gains `--input-trace <file>`: loads the trace once the drive stands up
and injects due entries each iteration before the scene polls; `g_present_frame`
(bumped in `drive_present`) is the Flip-anchored frame axis. No-op without the
title-scene drive. Pure C, host-tested (7 tests, 636 total: 630 pass / 0 fail /
6 skip): parse basic / tolerant (comments, blanks, key-order, missing ids) /
out-of-order / malformed; replay injects-once-at-frame / catch-up / NULL+empty
guards. Both cross-builds clean; ledger unchanged at 131/1490 (port-side
tooling, no retail FUN). Live validation (does an injected press actually skip
the splash / nav the menu) is deferred to next session; the port-frame-capture
+ diff half is still gated on 8d pixels.

## 2026-06-02 (ckpt 23) — 8d's opaque-trim scanner ported; the whole 8d call graph decoded

Ported `bs_trim_opaque_rect` (`FUN_005b6f80`, commit `2372a3f`) — the
trim-metadata builder the per-cell sprite-surface builder (8d, `0x5b9630`) runs
to size each cell: it scans a W×H window of the decoded bottom-up DIB for the
tight bounding box of opaque (non-colour-key) pixels and reports `found_opaque`
/ `found_key` (the builder skips a fully-transparent cell → a metrics-only
ZDDObject, and drops the colour key on a fully-opaque cell → the `0x1ffffff`
sentinel). The leaf helpers `0x5b6f00` (depth) / `0x5b6ec0` (bottom-row) are
one-liners, inlined.

**RE landed a real asymmetry → engine-quirk #48:** the 24bpp scan gates its
y-bounds on the *global* `x_left < W` (so `y_bottom` runs to `H-1` once any
opaque pixel exists anywhere), while the 8bpp scan uses a *per-row* opaque flag
(tight `y_bottom`). The headline test pair trims the same opaque shape at 24bpp
(`y_bottom`=7) vs 8bpp (`y_bottom`=4); +6 host tests (629 total, 623 pass / 0
fail / 6 skip). Ledger 131/1490 (8.1%), 128 tested (+1).

**Also decoded (no port — for the live session):** the rest of 8d's call graph
(`0x5b9280` build = new+ctor+`0x5b9630`+dtor; `0x5b9630` orchestrator =
trim-gate → `create_surface_pair` → `0x5b9910`; `0x5b9910` = Lock(`0x5b9490`) +
clip + zero + per-format copy via the `0x5b7310/_74f0/_7270` + `0x5b7bd0`
converters), with arg mappings, in `docs/findings/sprite-pipeline.md`. The
pure-logic 8d pieces are now all ported; what remains is DDraw-bound +
interdependent (Lock, pixel copy, format converters) — best ported + verified
together in the live session, where a registered bank + the real display depth
let the produced pixels be diffed against the harness goldens. So next session's
8d work is *wire + verify*, not RE-from-scratch.

## 2026-06-02 (ckpt 22) — the title scene is driven from `main.c`: the loop runs live

Wired the milestone-0 capstone: the ported title runner (`FUN_0056aea0` =
`title_scene_step`) is now **driven by the drop-in's `main.c`**, with the render
sink bound to the live primary surface. The scene that was a tested-in-isolation
unit is now the drop-in's actual per-frame loop.

**`src/title_drive.{c,h}` (ckpt 22a, commit `c90f834`)** — the *caller* side of
`FUN_0056aea0`, the plumbing its retail caller `FUN_00562ea0` owns, factored
Win32-free so it host-tests. `title_drive_init` allocates the scene's object
graph (a `sel_list` menu-tree owner + its 0x1b0 `menu_node` at `entry[0]` — the
slot the lazy phase-8 `title_menu_spawn` configures — and the `input_mgr` with a
**fully-populated idle ring**, since the poll/scan paths deref `ring[i]` with no
NULL guard), binds the render sink (`title_sink_ctx` → `title_render_sink_hook`),
and zeroes the FSM via `title_scene_init`. `title_drive_step` runs one
`title_scene_step` and latches the result on `TITLE_SCENE_DONE` (idempotent
after). `title_drive_shutdown` unbinds + frees the graph (mirrors the test
harness's `free_spawn`). **6 host tests, 617 pass / 0 fail / 6 skip** (+6):
init-allocates-and-binds, FLIP→present on a render frame, NULL-primary headless
no-op, abort-poll→DONE + idempotency, menu spawn + clean teardown (LSan),
shutdown safety on never-spawned / uninitialised drives.

**`main.c` wiring (ckpt 22b, commit `d9c75d9`)** — after DDraw init in mode 2,
`init_title_drive` binds the sink to `g_zdd->primary_obj` with `drive_present`
(→ `zdd_present`) and `drive_log_flip` thunks, installs `ar_sprite_decode_hook`
= `ar_sprite_decode` (banks self-decode once registered), and allocates the
drive. `main_loop_body` runs one `title_scene_step` per iteration; a render
iteration presents through the sink's `TITLE_DRAW_FLIP`; scene completion logs
the menu-action result and stops (the outer action dispatch lands when later
scenes are ported). `--no-title-scene` falls back to the legacy minimal present
loop. Both 32-bit cross-builds clean.

**The 8d per-cell surface builder (`0x5b9280`, `ar_frame_build_hook`) is still
NULL**, so every sprite resolves to NULL ⇒ the scene renders a **cleared +
flipped window with no sprites** — HANDOFF "move B", the prove-the-loop-live
state. Alpha ramps (`0x8a92b8`/`0x8a9308`) + the compositor display group are
unfilled/unmodeled at a cold boot, so they pass through as NULL (plain blits /
no compose — faithful). **Wants live verification next session** (Frida
self-serviceable): confirm zero DDERR + a flipping window, then wire 8d for real
sprites. Cadence note: `main.c`'s 16 ms `frame_limiter` throttles one step per
iteration (so ~1 render per ~2 steps); the scene's pacer adapts. Tune against
the live window next session if the visible rate is off. Ledger unchanged at
**130/1490 (8.1%), 127 tested** (the drive composes already-counted functions;
ref-bumps only).

## 2026-06-02 — Sprite-sheet decoder `FUN_004184a0` + slicer `FUN_004188b0`: the genuine pixel source is ported, ckpt 20

Ported the sprite-sheet decoder chip — the `ar_sprite_decode_hook` target that
turns a bank's PE "DATA" resource into the per-frame surface array the frame
getter returns.  The chip shrank dramatically once we found the whole
resource-load + DIB/decompress layer was **already ported** as `bitmap_session`
(`bs_decode_resource` etc.), so this checkpoint is the remaining decode logic:

- **`FUN_004184a0` → `ar_sprite_decode`** (asset_register.c) — entry-0 decoder:
  re-decode cleanup (release old frames + free the array) → `bs_decode_resource`
  (compressed) → gated 24bpp brightness pass → `ar_sprite_slice` → `bs_release`.
  On resource failure the drop-in leaves the bank unloaded (frames NULL, the
  getter's "still undecoded" path) instead of retail's process-exit.
- **`ar_sheet_decode_pixels`** — the pure 24bpp colour transform (the genuine
  new pixel logic, fully host-tested): per-channel `ch * scale / 1000` with an
  optional gamma LUT (`slot->f_18`), magenta `0xff00ff` left untouched, and the
  **reversed** byte→field mapping (byte0·f_14, byte1·f_10, byte2·f_0c) —
  engine-quirks **#46**.  Reads 3 bytes/pixel (not retail's dword) so the last
  pixel never reads past the buffer under ASan.
- **`FUN_004188b0` → `ar_sprite_slice`** — frame-grid geometry
  (`cols=sheet_w/cell_w`, `rows=sheet_h/cell_h`), `slot->f_38=count`, allocate +
  fill `entries[0].frames` via the per-cell surface-builder hook.  The
  builder's colour-key is hardwired to magenta unless the `0x1ffffff`
  sentinel — engine-quirks **#47**.

The DDraw leaf layer stays behind nullable hooks (`ar_frame_build_hook` =
`0x5b9280`, `ar_frame_free_hook` = `FUN_005b9390`; the trim-metadata `0x5b6f80`,
format switch `0x5b7310/_74f0/_7270`, and 8bpp palette `0x5b7bd0` are deferred).
Headless sizes + zero-fills the frames array but leaves surfaces NULL.

**605 host tests (599 pass, 0 fail, 6 skip; +12 this ckpt)**; both 32-bit
cross-builds clean.  Ledger **130/1490 touched (8.1%), 127 tested** (+2:
`FUN_004184a0` + `FUN_004188b0`, both inventoried).  Added a shared
`tests/bs_fixture.h` so the end-to-end decode test can register a synthetic
compressed 24bpp "DATA" resource without duplicating the builder.  Full decode
in `docs/findings/sprite-pipeline.md`.

## 2026-06-02 — Clipped Blt `FUN_005b9bf0` + sparkle wrapper: the whole title compositor/wrapper layer is ported, ckpt 19

Ported the last two render-bridge chips, completing every `TITLE_DRAW_*` arm:

- **`FUN_005b9bf0` → `zdd_object_blt_clipped`** (zdd.c) — the third blt
  sibling: a color-keyed Blt whose source rect is clipped against the src
  object's placement metrics (metric_0c/_10 origin, metric_14/_18 extent)
  with an explicit source sub-origin, the dest rect shifting to compensate
  for any left/top clip.  Pinned from the Ghidra decomp at
  `docs/decompiled/by-address/5b9bf0.c` (the hand stack-trace was ambiguous;
  the decomp made the clip algebra unambiguous).  Reuses the existing
  `zdd_surface_blt` primitive.  Returns 1 (no surface) / 0 (collapsed
  region) / HRESULT like its siblings.

- **`FUN_0056c580` → `title_draw_sparkle`** (title_render.c) — the sparkle/
  trail wrapper, gated directly on a caller-supplied descriptor (not a ramp
  lookup): desc != NULL → `zdd_blit_orchestrate` alpha with an **explicit
  source sub-rect** (the only wrapper that doesn't use src 0,0 + the
  sprite's full metric_14/_18); desc == NULL → `zdd_object_blt_clipped`.
  The alpha path metric-offsets the dest origin; the clipped path passes the
  raw origin (the clip applies the metric internally) — a retail asymmetry
  preserved literally.

**8 new host tests** (6 blt_clipped: null-src, no-clip rect forward,
left/top clip, extent clamp, collapsed→0, null-dest defensive; 2 sparkle:
alpha vs clipped path selection + arg mapping).  **587 host tests pass, 0
fail, 6 skip.**  Both 32-bit cross-builds clean.  Ledger **128/1490 touched
(7.9%), 125 tested** (+1 = blt_clipped; the four 0x56cxxx wrappers are
sub-helper labels — real ports not in functions.csv, so the headline holds).

The compositor + all four wrappers + the three blt primitives behind them
are now ported.  Next render chip = the sprite-sheet **decoder `0x4184a0` +
slicer `0x4188b0`** (the genuine pixel source; needs the sheet binary format
pinned), then the **sink + drive from `main.c`**.  See
`docs/findings/sprite-pipeline.md`.

---

## 2026-06-02 — Compositor `FUN_0056c180` ported into a new render-bridge module, ckpt 17

Ported the per-frame sprite-display-list **compositor `FUN_0056c180`** as
**`title_compositor_draw`** in a NEW module **`src/title_render.{c,h}`** — the
first to bridge both `asset_register.h` (the sprite pool + frame getter) and
`zdd.h` (the blit primitive + surfaces), exactly the home the next render
chips (the wrappers + the sink) need.

The valuable per-entry logic is factored into a pure helper
**`title_compositor_resolve`**: the animation frame-index math
(`p = min((anim_num*frame_count)/anim_div, frame_count-1)`,
`frame = (uint16_t)(frame_base - p + frame_count - 1)`), the asset-pool +
frame-surface lookup (`ar_pool_get_slot` → `ar_sprite_slot_frame`), the
blend-ramp index clamp (`(alpha_level*20)/1000`, clamped to [0,19] — retail's
`[0x8a9304]` default literally `== &ramp[19]`), and the centi-pixel geometry
(`metric_0c + x_num/100`).  All of retail's magic-reciprocal divides
(0x51eb851f → ÷100, 0x10624dd3 → ÷1000, the signed idiv ÷anim_div) are plain
C int division; disasm-verified at 0x56c19b..0x56c28d.

The display-list entry (0x1c B) + group header are modeled as
`title_sprite_entry` / `title_sprite_group` with `_Static_assert`-pinned
offsets.  The 20-entry blend-descriptor ramp `0x8a92b8` is passed in as a
parameter (NULL ⇒ all blits no-op), mirroring the existing `title_fade_ramp`
decoupling — in retail it IS pixel_drawer's `g_pd_boot_group_a` viewed as a
pointer table.  Headless-safety skips: `anim_div == 0` (retail faults) and a
NULL resolved sprite (retail derefs NULL) both drop the entry.

**10 new host tests** (resolve: basic geometry/frame/ramp, min-clamp, u16
frame mask, ramp lo/hi/mid clamp, div-0 invalid, NULL sprite/entry, NULL ramp;
draw: iterate+skip-invalid+arg-forward via a capture hook, NULL/empty group
no-op).  **572 host tests pass, 0 fail, 6 skip.**  Both 32-bit cross-builds
clean.  Ledger **127/1490 touched (7.9%), 124 tested** (+1 = the compositor).

Decode + the ckpt-16 getter: `docs/findings/sprite-pipeline.md` (compositor
section now marked PORTED).

---

## 2026-06-02 — Sprite frame getter `FUN_00418470` ported; the render sink's asset/sprite pipeline mapped, ckpt 16

Scouted move #1 ("build the render sink + drive the runner") and found it is
**gated on an unported asset/sprite subsystem** — every sprite draw resolves a
*frame surface* out of the asset pool `DAT_008a760c[bank_id]`, lazily decoded
from a sprite sheet.  Fully decoded the chain and wrote
**`docs/findings/sprite-pipeline.md`**: pool → `ar_sprite_slot` (the "bank") →
`bank->entries[0].frames[frame]` → `zdd_object*` surface → `zdd_blit_orchestrate`;
the compositor `0x56c180` walks the scene's display list and blits each entry;
the wrappers `0x56c610/_4e0/_470/_580` each resolve one sprite the same way.

**Reuse find:** the "bank" is the **already-pinned `ar_sprite_slot`** (0x44 B)
indexed by the existing `ar_pool_get_slot` — caught myself starting a duplicate
`zdd_sprite_bank` model and reverted to build on `asset_register`.

**Ported:** the frame getter **`FUN_00418470` → `ar_sprite_slot_frame`** in
`asset_register.c` — the two-level `slot->entries[0].frames[id]` lookup with
lazy decode routed through the nullable `ar_sprite_decode_hook` (the decoder
`0x4184a0` is a later chip).  Widened `ar_sprite_entry.a` (opaque `uint32_t`) →
**`void *frames`** to pin its role + make the getter host-testable (still 4 B
on the 32-bit build, so the 8-byte record holds).

**562 host tests pass, 0 fail, 6 skip** (4 new: null slot/entries, loaded-index
without hook, lazy-decode-fires-once, headless-no-hook).  Both 32-bit
cross-builds clean.  Ledger **126/1490 touched (+1), 123 tested (+1)**.  Next:
the compositor `0x56c180` (decoded, wants a new render-bridge module), then the
sheet decoder `0x4184a0`, then the sink + drive from `main.c`.

---

## 2026-06-02 — Blit orchestrator `FUN_005bd550` + `FUN_005b9ae0` ported; complex path proven dead, ckpt 15

Ported the **blit orchestrator `0x5bd550`** (`zdd_blit_orchestrate`) and its
sibling **`0x5b9ae0`** (`zdd_object_blt_rects`) — render task #8 from the ckpt-13
list.  `0x5bd550` is the single chokepoint every title-screen sprite draw funnels
through: the per-frame compositor `0x56c180` and the sprite wrappers
`0x56c470/_4e0/_580` all call it.  It is `__thiscall` on a `zdd_blend_desc`, with
10 stack args (`dest, src, dst_x, dst_y, w, h, src_x, src_y, colorkey, gdi_ctx`).

**Scout result (ckpt-13 move #1, "does the basic title render via the plain
path?"):** answered by disassembling all four sprite wrappers + the compositor.
`0x56c610` (plain) and `0x56c4e0` (leveled, at full brightness — ramp entry 0 per
quirk #40) forward to the already-ported `0x5b9b70` (`zdd_object_blt_keyed`); but
the **cursor `0x56c470` always** and the **per-frame compositor `0x56c180`
always** funnel through `0x5bd550`.  So the static logo+menu-text can draw plain,
but the cursor and the compositor can't — `0x5bd550` was the real next chip, not
a shortcut to move #2.

**`0x5bd550`'s simple path is the only live one.**  An exhaustive write-search of
the image (all `mov [0x8a6ec0], *` encodings) shows `DAT_008a6ec0` — the global
every caller passes as `gdi_ctx` — is **written only to zero**, never to a
surface.  So the complex path (GDI `BitBlt` into a scratch surface + hardware
`Blt` back via `0x5b9ae0`) **never executes**, and `0x5b9ae0` is reachable only
from it → also dead.  New **engine-quirk #45**.  The simple path is just
`zdd_object_lock(dest)` → `zdd_alpha_blit` → `zdd_object_unlock(dest)`, all
already-ported primitives; the complex path is still ported faithfully (new
`zdd_dc_blit` GDI seam in `zdd_win32.c`) but exercised only by a host test.

**558 host tests pass, 0 fail, 6 skip** (5 new: blt_rects null-src/null-dest/
rect-math, orchestrate simple lock→blit→unlock, orchestrate complex GDI
round-trip incl. the 16px min clamp).  Both 32-bit cross-builds clean.  Ledger
**125/1490 touched (+2), 122 tested**.  (Caught the recurring stray-`FUN_`
inflation again: `FUN_0056c180`/`_0056c470` in a docstring bumped the count until
rewritten as bare VAs.)

## 2026-06-02 — Software alpha blitter `FUN_005bd680` ported, ckpt 14

Ported the 1072-byte **software alpha/colorize blitter at `0x5bd680`** — the
heart of the title sprite-draw subsystem and the cleanest first chip of the
milestone-0 render bridges (ckpt-13 render task #7).  It is `__thiscall` on a
small blend descriptor (new `zdd_blend_desc`: `mode` + three `{shift,mask,LUT}`
channel records at `+0x04`/`+0x18`/`+0x2c`).  Three blend modes, all skipping
source pixels equal to the colorkey: **0** `out_ch = lut_ch[(src&mask)>>shift]
<< shift` (1-D remap), **1** `out_ch = lut_ch[(src_lvl<<5)+dst_lvl] << shift`
(2-D src×dst blend, reads the dest pixel), **2** `g = (ch0+ch1+ch2)/3; out_ch =
lut_ch[g] << shift` (colorize).  Split into a pure host-testable core
(`zdd_alpha_blit_pixels`, raw 16bpp buffers) + the retail wrapper
(`zdd_alpha_blit`) that reads dest geometry directly (caller pre-locks dest),
locks/unlocks only the **source**, and no-ops on a failed source Lock — all
mirroring retail (orchestrator `0x5bd550` locks dest first).  Clipping mirrors
retail exactly: right edge clamps to dst stride-in-words, bottom to dst height,
negative origins shift the source and pin dest to 0.

New **engine-quirk #44**: mode 1 hardcodes the src-level stride at 32
(`shl ebp,5`) for every channel, even 6-bit green; literal mirrored, the LUT
layout + descriptor ctor are a later chip.  11 host tests (3 modes, colorkey
skip, LUT transform, both clip axes, invalid-mode no-op, null guards, wrapper
lock/unlock + lock-fail).  **553 pass / 0 fail / 6 skip**; both 32-bit
cross-builds clean.  Ledger **122→123 touched, 119→120 tested**.  Commit
`cd95935`.

Scouted the rest of the alpha subsystem for the next checkpoint: orchestrator
`0x5bd550` (302 B; simple path = lock-dest → `0x5bd680` → unlock-dest, complex
path adds GDI BitBlt + a hardware Blt) calls `0x5b94e0`/`0x5b9500` (already
ported = `zdd_object_get_dc`/`_release_dc`) and the **unported** `0x5b9ae0`
(140 B Blt-with-explicit-rects, sibling of `zdd_object_blt_keyed`; its 9-arg
order is pinned by the `0x5bd550` call site).

---

## 2026-06-02 — Skip-splash early-out ported; update half complete, ckpt 12

Ported `FUN_0056aea0`'s **skip-splash early-out** (`0x56b0e8..0x56b150`, "press
a button during the intro to jump straight to the menu") — the last
documented-deferred slice of the title scene's update half.  It now sits in
`title_scene_step` right after the `0x22` abort poll: scan the input ring for any
fresh press (`input_any_fresh_press`); on a hit, zero the fade, fire the BGM
`SetNextSegment` cue when still before phase 3, flush the ring + axis state
(`input_mgr_reset`), and force phase 8.  The gate honours the scene's new
`skip_intro` field (`param_1`): clear → a press is ignored at phase 0 (a first
boot plays the studio fade in full), set → it skips from phase 0 too; phases 1..7
always skip on a press.  No engine subsystem pulled in — the BGM cue reuses the
existing `set_next_segment` hook.

The flush block (`0x56b25e..0x56b29a`) mapped the input manager past the ring:
the two "axis-held" flags at `+0x114`/`+0x118` the title menu reads are actually
`array_A[0]`/`array_A[1]` of an **11-dword array**, with a parallel array B at
`+0x140`, plus `+0x10c`/`+0x110`/`+0x16c` scalars — all cleared by the skip.
`input_mgr` was extended to model them (offsets pinned by `_Static_assert` on the
32-bit build) and the two `axis_held_v/h` reads became `axis_held[0]/[1]`.  New
finding → **engine-quirks #41** (let the *reset* code, not the read code, reveal a
struct's true array shape).  The one piece left out is retail's scene-local
sparkle-counter reset (`var_3eh_2`), which belongs to the still-deferred
sparkle-trail subsystem (spawned/advanced outside the runner via a hook).

**542 host tests pass, 0 fail, 6 skip (of 548)** — 8 new (the two input
primitives: any-fresh-press match/empty/flag+age, reset flushes all fields; and
the skip-splash path: jump-to-menu with fade reset + ring flush, the BGM cue
gating, the phase-0 `skip_intro` gate both ways, and the no-press no-op).  Both
32-bit cross-builds clean.  Ledger **122/1490 touched (7.5%)** unchanged — the
early-out is a slice of the already-counted `FUN_0056aea0`; its new input helpers
reference the slice by bare VA.  See `findings/input.md` (wider manager model),
`findings/title-scene.md` (skip-splash now ported).

## 2026-06-02 — Parity harness live-verified against retail under Frida, ckpt 12

Ran the structural-parity call-trace harness against **live retail under
Frida** for the first time (Frida is always up + UAC auto-approved on this
host, so the "human-verification gate" is really self-serviceable — noted in
memory).  Result: it **works end-to-end**.  A `--no-turbo` capture with the
*full* 1743-VA candidate set hooked at once booted retail to its title window
and emitted **1.8M call-trace events over 1914 Flip frames with zero
crashes** — so `tools/bisect_call_trace_vas.py` proved **unnecessary** for
this boot path; `engine_vas_frida_safe.json` was written directly from the
full set.

Two hard live findings.  (1) **Everything live must be `--no-turbo`:** turbo
freezes the splash before the engine reaches its message pump (quirk #29) —
a turbo boot reports `msg_count=0` / no Flips, so the bisect's turbo default
made every subset read as a crash (the calibration trap the file header
warned about).  No-turbo boots cleanly (`msg_count` ~750-1000, 1914 frames).
Fixed the bisect to default `--no-turbo`.  (2) Mining the trace for the title
render path **confirms the ported control flow and the next port target
against live retail**: `0x56c180` compose / `0x5b8fc0` Flip / `0x5b1030` pump
fire once per frame; `0x56c930` post-update fires ≈ half as often (validating
the pacing-FSM update/render split); and the **software alpha blitter
`0x5bd680` (+ orchestrator `0x5bd550`) is the hot per-frame draw primitive
(4279 calls)** — the recommended next render-bridge chip.  The call_trace
*diff* is now blocked only on the port side (main.c must drive
`title_scene_step`).  Details + the per-frame table in `docs/parity-harness.md`.

## 2026-06-02 — Title scene runner wired into one orchestrated loop, ckpt 11

Composed the ckpt 1–10 units (the pacing FSM, the fade FSM, the menu spawn,
the per-frame input dispatch, the render step, the teardown) into the one
running title-scene runner — `title_scene_step` / `title_scene_init` in
`src/title_scene.c`, ported from `FUN_0056aea0`'s outer `do { … } while(1)`
body (`0x56b002..0x56ba75`).  One call = one loop iteration: sample
GetTickCount → `title_pace_step` (pump fires through a hook when requested) →
on a `TITLE_PACE_RENDER` iteration `title_render_step` draws+presents and
loops; on a `TITLE_PACE_UPDATE` iteration the update half runs (pre-update
side effects → the `0x22` abort poll → the phase switch = `title_fade_step`,
with `title_menu_spawn` on first menu entry + `title_menu_input_step`, and
`title_menu_teardown` before the phase-10 fade-out → the per-frame tail:
watchdog increment, post-update, per-owner-entry update).

This is the piece "between a pile of units and a running title scene": the
whole milestone-0 control flow now exists as one composable, testable unit.
Every still-unported per-frame engine call (`0x5b1030` pump, `0x43e140` +
`0x40fe00` + `0x566250` pre-update, `0x56c930` post-update, `0x43c2e0`
per-entry, the BGM SetNextSegment cue, `0x56c070` sparkle spawn) is routed
through a nullable `title_scene_hooks` struct; the menu-input side effects
keep using the existing `title_menu_*_hook` globals; the render bridges keep
using `title_render_sink_hook`.  So the runner assembles and tests now without
pulling in audio / DInput / DDraw / the god object.

**Anatomy confirmed while wiring** (folded into `findings/title-scene.md`):
the scene returns *only* out of the update half — the `0x22` abort poll
(result 6) or the phase-10 fade-out completing (result = the committed menu
action, or 0 on a watchdog timeout); the render half never returns, it loops
(reinforcing the ckpt-10 finding that Ghidra's "jump table as call+return"
was an artifact).  The idle watchdog (`local_50`) increments on *every* update
frame across *all* phases — so the ~75 s timeout counts the intro, not just
menu-idle time.  The menu both spawns *and* runs its first input poll on the
same frame, with the input gate still closed (so the first menu frame can't
latch).

**Deferred (documented seam):** the **skip-splash early-out**
(`0x56b0e8..0x56b150`, "press any button during the intro to jump to the
menu") is not ported here — it walks the input-mgr ring directly and fires a
second SetNextSegment cue; it's a separable intro-convenience reading
input-mgr internals the poll port doesn't model yet.  Without it the intro
always plays in full (the headless `param_1==0` default).

**534 host tests pass, 0 fail, 6 skip (of 540)** — 7 new (runner init/bind,
a render iteration presents without touching the update half, the abort poll
returns 6, the full intro walk to the spawned menu, the watchdog forcing the
fade-out exit, the full intro→menu→commit→exit "money path" returning the
selected action 0x1a, and NULL-hooks safety; ASan/UBSan clean).  Both 32-bit
cross-builds clean.  Ledger **122/1490 touched (7.5%), 119 tested**
(unchanged — an extension of the already-counted `FUN_0056aea0`; unported
callees referenced by bare VA).  The commit-path test had to point the
controller's gate (`ctrl->sub`) at a real `menu_input_sub` rather than the
node it aliases on 32-bit — the same host/target struct-padding divergence
quirk #38 documents.

---

## 2026-06-01 — Title scene render half ported, ckpt 10

Ported `FUN_0056aea0`'s **render branch** (`0x56bb04..0x56bf1a`) into
`src/title_scene.c` as **`title_render_step`** — the path the frame pacer
dispatches to on a `sub==1` (`TITLE_PACE_RENDER`) frame.  This is the last
piece of the title scene: with the update half done at ckpt 9, the runner's
whole control flow (fade FSM + pacing FSM + update + render) is now ported.
The fade→alpha helper `0x448c80` is ported alongside as the pure
`title_fade_ramp`.

The render half draws one frame for the current phase: a prologue (phase 0 →
surface reset `0x5b9410`; phases 2–3 → surface clear `0x5b9b70`; phase > 10 →
skip to frame-end), then the **recovered 11-entry jump table** at `0x56bfa4`
dispatching `jmp [phase*4+...]` to one of 7 inline handlers (studio/title-logo
alpha blits, the two "press button" sprite pairs, the sparkle trail, the menu
bg+sprite+cursor, the menu fade-out), then the universal frame-end at
`0x56bec4`: compose `0x56c180` → "Title Menu - Flipping" log (once, unless the
`DAT_008a6b54` quiet flag) → Flip `0x5b8fc0` — the documented "title menu drew
a frame" event.

It is heavily DDraw/asset/object-model-coupled — every leaf the handlers call
(`0x494e10`, `0x418470`, `0x56c610/_4e0/_580/_470`, `0x56c180`, `0x5b8fc0`, …)
is unported.  Per the handoff's guidance, rather than a hook per bridge, they
are reported as an ordered stream of tagged `title_draw_cmd`s through a single
`title_render_sink_hook` (no-op by default) — the render half's *purpose* is
exactly that ordered draw stream, so the sink is its testable core: the
dispatch decision, per-handler draw sequence, fade→alpha ramp, sparkle-trail
geometry, and selected-row cursor placement are all asserted in it without
pulling in any black-box draw subsystem.

**Findings — new quirk #40:** `0x448c80`'s ramp returns **0 at both ends** —
`fade == 1000` lands on the excluded `idx == 20` (`>= 0x14` cap) and returns 0,
*not* the top entry, so a saturated hold composites by a different path than
the ramp; and its 20-dword table at `0x8a9308` reads **all-zero statically**
(DDraw fills it at run time), so a headless port correctly sees alpha 0
everywhere (modelled as a NULL/empty `ramp` input).  Also: the two intro logos
are **container fields at +4/+8**, not `0x418470` assets; and Ghidra's
"call+return" rendering of the `0x56bb55` jump table hid that the 7 handlers
are inline labels all converging on the one frame-end.

This is an extension of the already-counted `FUN_0056aea0`, with all unported
callees referenced by bare VA, so the ledger is **unchanged: 122/1490 touched
(7.5%), 119 tested**.  **9 host tests** (fade-ramp index/clamp, prologue gating,
logo clear-vs-blit, press-button asset pairs, sparkle trail count/geometry,
menu bg+cursor, fade-out, flip-log-once, no-sink safety); **527 pass / 0 fail /
6 skip (of 533)**, ASan/UBSan clean.  Both 32-bit cross-builds clean.

## 2026-06-01 — Title-menu per-frame input dispatch assembled, ckpt 9

Assembled the `0x56aea0` default-branch **per-frame menu input dispatch**
(`0x56b807..0x56ba39`) into `src/title_scene.c` as **`title_menu_input_step`**
— the last piece of the title scene's *update* half (commit `f8f76bc`).  Like
ckpt 8 this is an *assembly* of already-ported leaves (`input_poll_consume`,
`menu_list_latch`), so the ledger is unchanged (**122/1490 touched, 119
tested**); the value is the wiring + the disasm-resolved findings.

The step polls the five menu buttons + two interleaved axis-held syntheses,
feeds each into the latch, then runs the action switch (move / confirm /
denied / cancel SFX), the enabled-row commit (joystick lazy-attach → save-data
table walk + notify → phase-10 + result latch), and the idle watchdog.  The
four unported side effects — SFX `0x411390`, joystick `0x5ba120/_290`, notify
`0x41bb80`, watchdog `0x40a5d0` — route through no-op-by-default hooks (the
`menu_cell_layout_hook` pattern).  The save-data lookup itself is ported
faithfully against a caller-supplied model of the god object's table slice.
The `+0x114/+0x118` axis-held flags were added to `input_mgr` (with 32-bit
offset asserts) — they live in the input manager past the poll ring.

**Findings (the decompile hid both, so resolved against the raw r2 disasm) —
new quirk #39:** the action `switch` keys on the *latch return code*, not the
button, and the engine's cancel-returns-3 / confirm-returns-4 convention
inverts the intuitive reading: the physical **commit button is `0x24`**
(latch dir 9 → nav returns 3 → `case 3`), `case 3` gates on the selected
*row*'s `flag8` (enabled) — **not** on `action == 0x1d` as an earlier summary
claimed (that's a separate, later save-data guard) — and `case 4` (cancel
SFX 7) is **dead** in the title flow (needs latch dir 10, never sent).  Also:
the page dirs (`2`/`3`) are no-ops on the single-page menu (`stride 6 ≥ count
5`), so only up/left navigate.  Corrected the stale "back/cancel" / "0x1d →
SFX 6" notes in `title-scene.md`.

**9 host tests** (nav SFX, page-button no-op, commit enabled/disabled,
save-data match + 0x1d skip, idle frame, axis-held synth, watchdog idle
threshold); **518 pass / 0 fail / 6 skip (of 524)**, ASan/UBSan clean.  Both
32-bit cross-builds clean.  Remaining for the whole title scene: only the
**render half** (`0x56bb04`, jump-table draws + Flip) — the milestone-0 update
half is now complete.

## 2026-05-29 — Title-menu spawn block assembled, ckpt 8

Assembled the `0x56aea0` default-branch **spawn block** (`0x56b5cd..0x56b807`)
into `src/title_scene.c` as **`title_menu_spawn`** (+ `title_menu_teardown`
for the phase-10 path) — the last piece of the title scene's *update* half.
No new function is ported here (it composes already-ported leaves), so the
ledger is unchanged; the value is the assembly + the structural finding.  The
block: configure the owner `sel_list`'s next entry as the menu tree node with
one child (`menu_node_build`), bump + `sel_list_mark_last`, acquire that lone
child as the controller, `menu_ctrl_build` a 6×1 stride-6 grid, append the
five fixed rows `0x1a,0x1c,0x1e,0x1d,8` (each `menu_row_finalize`d — a no-op
on fresh NULL cells), then seek the cursor to the row matching the saved
selection key and `menu_list_scroll_into_view`.

**Headline finding (new quirk #38):** the 0x1b0 menu node wears **four**
overlaid identities — container header, embedded `menu_ctrl`, `sel_entry`,
*and* `obj_pool`.  The controller (`local_60`) is the node's lone **child**,
handed out by reinterpreting the node as a pool: retail does
`obj_pool_acquire(node)` (ECX = node), confirmed in the disasm
(`0x56b623 call 0x412c10` with the node still in ECX from
`[owner->entries + count*4]`).  The acquire stamps the child's `+0x00` —
which is the controller's `menu_ctrl.sub` — with the node pointer, **wiring
the controller's input-ready gate to the node** (the latch reads `node+0x54`
ready / `node+0x04` enabled).  The node's child-array/count/capacity at
`+0x48`/`+0x4e`/`+0x4c` alias `obj_pool`, and `node+0x08` aliases
`sel_entry.selected` — but **only on the 32-bit target**; the node's 8-byte
`owner` pointer shifts those fields on the 64-bit host.  So the port applies
`obj_pool_acquire`'s semantics to the node's own `menu_node` fields (identical
to the cast on win32) and the test checks selection via the `sel_entry` view.

5 new host tests (full five-row build, cursor-seek-to-match, no-match keeps
cursor 0, teardown clears the node's `+0x50` flag, teardown-noop-when-unset;
ASan/LSan clean).  **509 pass / 0 fail / 6 skip (of 515)**; both 32-bit
cross-builds clean (all `menu_node`/`sel_list`/`pool_slot` offset asserts
hold).  Ledger **122/1490 touched (7.5%), 119 tested** (unchanged — assembly,
not a new port).  See `findings/title-scene.md`, `findings/menu-list.md`
"The spawn block", and quirk #38.

---

## 2026-05-29 — Menu-node builder + the menu-tree structure, ckpt 7

Ported `FUN_0040f3e0` (434 B) into `menu_list` as **`menu_node_build`** — the
0x1b0 menu-item / page builder.  It (re)configures one menu node from its
params and (re)builds the node's child-node array, freeing any stale children
via `menu_ctrl_clear` first.  This is the last sub-function the title-menu
spawn block needed; only the cheap inline row appends remain to finish the
update half.

**Headline finding (new quirk #37):** the engine's menus are a **tree of
uniform 0x1b0-byte nodes**, and Ghidra mis-typed this builder's `__thiscall`.
The decompiled call `FUN_0040f3e0(piVar11,0,0,100,100,1,0)` reads as "operates
on `piVar11`", but the disasm (`0x40f3ec mov ebx,ecx`; call site `0x56b606
mov ecx,[owner->entries + count*4]`) shows the ECX `this` is the **node** being
configured and `piVar11` is `param_1` (the owning `sel_list`) — Ghidra dropped
the ECX node and rendered it as the first arg.  So the earlier "page-container"
reading in `findings/menu-list.md` / HANDOFF was off by one, now corrected.

Each node overlays two views on one buffer: a **container header**
(`+0x00..+0x84`, child-pointer array at `+0x48`, u16 count at `+0x4c`) and an
**embedded `menu_ctrl`** at `+0x00` (so `+0x164..+0x17c` are
`field_164/list2/list/entries/rows`) followed by `0x30 B` of **display config**
at `+0x180..+0x1ac` (text/shadow colours `0x3e537d`/`0xa8b9cc`/`0xf08080` +
label VAs `&DAT_00677b98`/`&DAT_008090a9`).  That dual identity is why the
builder tears a stale child down with `menu_ctrl_clear`.  Modelled `menu_node`
(0x1b0) in `menu_list.h`, pinned by guarded 32-bit `_Static_assert`s.

5 new host tests (title call, config-blob copy, per-child display config,
rebuild-frees-old-children under LSan, zero-children).  **504 pass / 0 fail /
6 skip (of 510)**; both cross-builds clean (the new 0x1b0/offset asserts and
the `sizeof(menu_node) >= sizeof(menu_ctrl)` cast-safety assert all hold).
Ledger **122/1490 touched (7.5%), 119 tested**.  See commit `a78073a`,
`findings/menu-list.md`, and quirk #37.

---

## 2026-05-29 — Menu grid-cell finalizer + dead-alloc quirk, ckpt 6

Ported the menu spawn block's **grid-cell finalizer** into `menu_list` as
`menu_row_finalize` (`FUN_00411f40`, 444 B).  `__thiscall(ctrl, row)`: it
walks the row's cell array (bounded by `hdr->alloc_b`) and, per cell,
refreshes whichever lazily-built sub-objects are present — `obj0` (re-lays
its glyph text via `0x40fa00`), `obj54` and `obj20` (re-zeroes their
modelled fields when `row < hdr->count`; `obj20` also recomputes
`+0x1c = max(+0x14, min(+0x18, 0))`).

**The headline finding (new quirk #36):** the decompile reads as a classic
lazy get-or-create, but disassembling `0x411f40` shows the per-sub-object
`if (ptr == 0) operator_new(...)` sits **inside** an outer `ptr != 0`
guard reading the same slot with no intervening write (verified at
`0x411fbf` / `0x412046`).  So the allocation is **statically dead** — the
finalizer never allocates, it only re-zeroes sub-objects built elsewhere.
The earlier `findings/menu-list.md` note that it "lazily operator_new's"
the sub-objects was **wrong** and is corrected.  On the fresh title menu
every cell pointer is NULL, so the whole function is a no-op there.

`0x40fa00` (the cell's 800-B SJIS/colour-escape/font-metric text-layout
builder) is its own subsystem and stays unported; the finalizer's call to
it routes through an observable hook (`menu_cell_layout_hook`) so the
dispatch is testable without pulling in the text layer.  Its string arg
`&DAT_008a9b6c` is the god object's engine-name buffer (god+0x1c).

Modelled `menu_cell_obj54` (0x54 B) / `menu_cell_obj20` (0x20 B) with
guarded `_Static_assert`s; both cross-builds clean.  **6 new host tests**
(fresh no-op, obj54 re-zero, obj20 re-zero+clamp, row-outruns-count guard,
obj0 layout-hook dispatch, all-cells iteration; ASan/LSan clean); **499
pass / 0 fail / 6 skip (of 505)**.  Ledger **121/1490 touched (7.4%), 118
tested** — unchanged, because `0x411f40` had been provisionally counted via
a ckpt-5 header comment using the `FUN_` token; this port legitimises it.
What remains of the update half: the menu-item builder `0x40f3e0` + the
spawn-block assembly.  See `findings/menu-list.md`, commit `1ba5827`.

---

## 2026-05-29 — Menu-controller geometry ctor+dtor, ckpt 5

Ported the menu-spawn block's **allocate/free pair** into `menu_list` as
`menu_ctrl_build` (`FUN_0040f5c0`, 563 B) and `menu_ctrl_clear`
(`FUN_0040e0c0`, 555 B) — one commit, since build calls clear (slots are
recycled from an object pool, not zeroed; the ctor tears down stale state
first — new quirk **#35**).

`menu_ctrl_build` allocates the `0x24` list header plus the controller's
**two parallel geometry arrays**: the row array (`alloc_a` × `menu_row`
0x10, at `+0x17c`) with a per-row cell array (`alloc_b` × `menu_cell`
0x18), and the per-column metadata array (`alloc_b` × `menu_entry` 0x24,
at `+0x178`) stamped `pos=index*0x20` / `extent=0x20`.  `menu_ctrl_clear`
frees it all in retail order (confirm graph → `+0x164` → entries → each
row's cells + their three lazy sub-objects → row array → header **last**,
since its dims size the free loops).

Modelled the controller's extended layout (`menu_row`/`menu_cell`/
`menu_entry` + the ctor-touched scalars `field_c/_10/_20/_84/_140/_164`)
and extended `confirm_src`/`confirm_caprec` with the owned-pointer slots
the teardown frees.  Every new offset pinned by guarded `_Static_assert`;
both cross-builds clean.  `operator_new → calloc` (zero-init divergence
documented: retail leaves `row.flag8` + each cell's trailing 8 bytes
indeterminate; the spawn block always writes `flag8` before reading).

**9 new host tests** (build header/params/grid/entries, fresh-clear no-op,
recycle-rebuild, and the confirm-graph + cell-subobject + `+0x164`
teardowns — ASan/LSan verify no leak/double-free/UAF); **493 pass / 0 fail
/ 6 skip (of 499)**.  Ledger **121/1490 touched (7.4%), 118 tested**.  The
menu controller's geometry is now grounded; what remains of the spawn
block is the cheap inline row-populate and the two lazy cell finalizers
(`0x40f3e0`, `0x411f40`).  See `findings/menu-list.md`, commit `a380457`.

## 2026-05-29 — Menu input→action chain, ckpt 4 (scroll + nav + latch)

Ported the entire **menu input → action chain** the title menu's update
half depends on, into a new **`src/menu_list.{c,h}`** (three logical
commits 4a/4b/4c, port-and-test):

- **4a `menu_list_scroll_into_view`** (`FUN_004192b0`, 52 B) — recompute
  the page-top `sel2 = floor(cursor/stride)*stride`, return 1 if it moved.
  Factored the step-search into `page_top()` (the nav engine reuses it).
- **4b `menu_list_nav`** (`FUN_0043ca40`, 970 B) — the cursor-navigation
  engine. Its inner jump table at `0x43ce1c` was **Ghidra-unrecovered**
  (duplicate targets); recovered with `radare2 -c 'pxw 44 @ 0x43ce1c'` —
  dir 0..10 → 7 handlers (prev/next/page-up/page-down/no-op/cancel/
  confirm). Ported branch-for-branch including the three list-type scroll
  models (0 linear-wrap / 2 grid / 3 trailing-page, whose shared fields
  change meaning per type) and the two-rate auto-repeat (300 ms initial →
  100 ms steady). The internal `GetTickCount` is injected as `now`.
- **4c `menu_list_latch`** (`FUN_0043ce50`, 220 B) — the input gate:
  refuses unless `sub->ready==1000 && sub->enabled!=0`, then dispatches
  mode 1 → nav, mode 2 → confirm/message box (two-press reveal-then-
  dismiss). Modelled the `sub` ready-gate and the `confirm_list`
  `src→caprec→cap` u16 chain.

Together with the ckpt-3 poll this closes `input_poll_consume →
menu_list_latch → menu_list_nav`. Verified the common tail + cancel/
confirm + latch offsets against the r2 disasm at `0x43ccf7` / `0x43cae9`
/ `0x43ce50`. **43 new host tests** (6 scroll + 26 nav + 11 latch), all
hand-derived; **484 pass / 0 fail / 6 skip**. Both cross-builds clean.
New quirks **#32** (jump table + per-type field reuse), **#33** (two-rate
auto-repeat), **#34** (1000-ready gate + two-press confirm). Ledger
**118/1490 touched (7.2%)**. See `findings/menu-list.md`.

Next: the menu-spawn block (port leaves `0x40f3e0`/`0x411f40`/`0x40f5c0`,
then assemble the 5-row populate) to finish the update half, then the
render half (`0x56bb04`). The input-ring **producer** (DInput
`GetDeviceState`) is now the only black box left in the input subsystem.

---

## 2026-05-29 — Title-menu update-half leaves (ckpt 3): input poll + container primitives

Knocked out the pure, zero-dependency leaves the title-menu update half
(`FUN_0056aea0` default branch) depends on, per the HANDOFF "next move" —
port-and-test rhythm to shrink the surface before assembling the menu.

**Input ring poll `FUN_0043c110`** → new `src/input.{c,h}`,
`input_poll_consume` (opens milestone 1).  The read side of the input
manager's 64-entry event ring at `+0x108`: scan newest-first, match
`id + flag==1 + age<=100 ms` (unsigned, rollover-safe), and on a hit zero
the record id (consume-on-read).  10 host tests; see `findings/input.md`
and engine-quirks #30.  This is the consumer end of the ring `mem_watch.py`
is meant to find live (producer still black-box).

**Container leaves `FUN_00412c10` + `FUN_00414080`** → new
`src/obj_container.{c,h}`.  `obj_pool_acquire` (check out the next free
slot from a fixed-capacity pool, stamp owner/index/+8, NULL when full —
note the index is a 16-bit store into a dword, quirk #31) and
`sel_list_mark_last` (single-selection: mark the last list entry, clear the
rest).  Both ~10× across the engine; in the menu-spawn block they run back
to back (append → mark-last → acquire controller).  8 host tests; the
`sel_list` `+4`/`+6` layout is cross-validated by the title-scene caller.

Deferred (still object-model-coupled / unported deps): the action latch
`FUN_0043ce50` + the 970 B cursor-nav engine `FUN_0043ca40` (jump table
unrecovered), and the menu-spawn assembly itself (needs `0x40f3e0`/
`0x40f5c0`/`0x411f40`/`0x4192b0`).  The sound-effect player at `0x411390`
is audio-subsystem-coupled (milestone 3), not the "action switch" the old
HANDOFF guessed.

Two commits.  **441 host tests pass / 0 fail / 6 skip** (of 447), both
cross-build exes clean.  Ledger **113→115 touched (7.0%), 110→112 tested**.

## 2026-05-29 — Structural-parity harness (offline foundation)

Detour from milestone 0 to build the call-graph-diff + mem-watch machinery
that `../openrecet` credits for fast rendering-path convergence, adapted to
SoTE's DirectDraw stack. Design: `docs/plans/parity-harness.md`; how-to:
`docs/parity-harness.md`. Built the pieces that need no live retail (the
"offline foundation"); the live retail-under-Frida runs are the next-session
human-verification gate.

Landed **[offline, all tested]**: `src/call_trace.{c,h}` — the port-side
`CALL_TRACE_ENTER(0xVA)` probe (one null-check when off), wired into
`main.c` (`--call-trace <path>` + `--call-trace-frames`, begin/end frame
brackets, boot-frame-0 bracket) and probed into `zdd_create`/`zdd_present`/
`zdd_window_paint`/`cs_dispatch_create_screen`; 5 new host tests (**423
pass / 0 fail / 6 skip** of 429, both exes clean). `tools/call_trace_diff.py`
— per-frame overlap / retail-only (= port gap) / port-only (= divergence)
diff with `--align-on-first 0xVA` load-skew anchoring; 9 pytest cases.
`tools/gen_engine_vas.py` — `functions.csv` → 1743-VA candidate hook list.
`tools/mem_watch.py` ranking (offline `--analyze-only` verified: a faulting
insn maps to its owning function + port status via the ledger).

Code-complete, **live verification deferred**: `opensummoners-agent.js`
call-trace + mem-watch modes, anchored on a hook of the DDraw Flip
(`FUN_005b8fc0`) for the per-frame boundary — the DirectDraw analog of
openrecet's D3D-Present anchor (frame axis matches the port's per-`zdd_present`
`g_frame_counter`). `frida_capture.py` driver fields + CLI. `mem_watch.py`
capture + `bisect_call_trace_vas.py` (boots retail via `run-retail.sh`,
bisects out Frida-unsafe VAs → `engine_vas_frida_safe.json`; its
`--boot-threshold` needs calibration on the first live boot). First real use
will target the `+0x108` input-ring writer — resolving a standing HANDOFF
black box. Ledger headline unchanged (112/1490); the probes sit in
already-ported files.

---

## 2026-05-29 — Title scene runner, checkpoint 2: frame-pacing FSM

Second code chip of milestone 0.  Ported the `local_28` frame-pacing sub-state
machine + the `FUN_005b1030` pump call sites at the top of `FUN_0056aea0`'s outer
loop, as `title_pace_*` in `src/title_scene.{c,h}` (`title_pace_step`).  It's a
pure fixed-16 ms-timestep accumulator: each iteration it runs the *update* half
(input + the ckpt-1 `local_64` phase FSM) burning the wall-clock budget in 16 ms
slices, or the *render* half (jump-table draw + Flip), refilling the budget from
the real `GetTickCount` delta (clamped 100 ms) and pumping on the way into update.
`now` is passed in (app_pump style); the pump request and update/render decision
are reported via `title_pace_step_out`, so the unit stays Win32-free and
link-dependency-free.  12 new host tests, all green (**418 pass / 0 fail / 6
skip** of 424; both cross-build exes clean).

Decoded byte-for-byte from r2 disasm `0x56b002..0x56b0c8` (raw stack offsets).
**Resolved the open ckpt-1 thread:** the `E` counter at `[esp+0x5c]` (which r2
showed but Ghidra dropped) is a **dead** consecutive-sub-second-frame tally — a
full-function disassembly scan finds it written-only/never-read, and Ghidra
dead-store-eliminated it.  Its window anchor `D = local_20` is read only to gate
that dead update, so the *entire* `S==1` post-arm is observably inert and is
omitted — behaviourally exact.  Only the `S==2` arm (`A = now`) is load-bearing.
The pacer also explains the `--turbo` "splash doesn't animate" symptom: with a
frozen clock the budget never refills past one slice, so after the first update
the loop renders every frame with the phase FSM frozen.  See
`findings/title-scene.md` "Frame-pacing sub-state machine" and engine-quirk #29.
Ledger headline unchanged (112/1490) — `FUN_0056aea0` was already "touched";
progress shows as new provenance refs.

---

## 2026-05-29 — Title scene runner, checkpoint 1: intro-phase/menu-fade FSM

First code chip of milestone 0 (title screen renders).  Ported the pure
arithmetic core of `FUN_0056aea0` (the 3441-byte title scene runner): the
`switch(local_64)` intro-phase / menu-fade state machine, as
`src/title_scene.{c,h}` (`title_fade_step`).  19 host tests, all green
(406 pass / 0 fail / 6 skip total; cross-build clean).

Verified the control flow against raw radare2 disasm before porting — the
`PTR_DAT_0056bfa4` 11-entry phase jump table (`0x56bb5c..0x56be85`, 7 distinct
handlers) and the `switch` table at `0x56bf68` both match `findings/title-scene.md`,
and every fade constant/threshold in cases 0..10 was confirmed against
`0x56b153..0x56b5c1`.  The FSM is kept **side-effect-free**: the two outward
signals (the studio→title BGM "SetNextSegment" cue, and the phase-7 sparkle
spawn at intensity `(fade*0xe0)/900+0xc0`) are reported through a per-frame
`title_fade_step_out` descriptor instead of calling the still-unported engine
helpers — so `title_scene.c` has zero link dependencies and drops straight into
both the host suite and the mingw `.exe`.  New quirk **#28** (single reused
0..1000 fade ramp across 8 phases + the menu "breathing" oscillator).

**Deferred to later checkpoints** (seams documented in `title_scene.h`): the
`local_28` frame-pacing FSM + `FUN_005b1030` pump call sites; the phase-8/9
menu-controller spawn (`0x412c10`) + 5-slot populate + input poll/latch
(`0x43c110`/`0x43ce50`) + action switch (`0x411390`); the render-half jump-table
draw handlers + frame-end flip (`0x56c180` + `FUN_005b8fc0`); the `param_1`
skip-intro + ring-buffer "press to skip splash" early-out; joystick lazy-attach
(`0x5ba120`); the `local_50` watchdog.  Ledger headline unchanged (112 touched —
`FUN_0056aea0` was already counted; the ledger is binary, so partial progress
isn't reflected in the count, only in the new provenance refs).

## 2026-05-29 — Project-wide audit + workflow tooling (no code port)

Brought OpenSummoners up to the sibling-project (OpenMare / OpenLords2)
workflow standard and mapped the entire binary so future sessions don't
re-analyze the same code.  No engine functions ported this checkpoint —
this is infrastructure + intelligence.

**Derived progress tooling** (adapted from OpenMare):
`tools/gen_port_ledger.py` + `tools/gen_frontier.py`.  Engine-proper
boundary pinned to `0x5bdab0` (last engine fn `FUN_005bd680` before the
MSVC CRT tail — import thunks, `operator_new`, `_malloc`, the `entry`
startup, etc.).  Generates `STATUS.md`, `port-ledger.{md,json}`,
`port-frontier.md` from `FUN_<va>` provenance comments in `src/` +
`functions.csv` — idempotent, `--check`-able for a pre-commit hook.
Baseline: **112/1490 engine-proper touched (6.8%), 9.5% of code bytes**,
109 host-tested, 52 portable-today frontier leaves.

**Subsystem-survey workflow** (`tools/workflows/subsystem-survey.js`):
22 read-only `Explore` agents (16 mapping address bands + 6 scouting the
forward path), ~1.9 M tokens.  Output seeded `docs/ROADMAP.md` (11-milestone
order + full subsystem map of every band + 5 port-readiness cards) and
surfaced **136 quirks** — the load-bearing/charming subset went into
`findings/engine-quirks.md` #15–#27 (god-object `DAT_008a9b50`, the
universal frame-pump `return 6` quit convention, the hash-id asset
directory *with recovered character names* — Arche/Sana/Sophia, the LCG
RNG, struct strides 0x294/0x300, WMA-temp-file BGM, lazy gamepad attach,
the "object perpetuity state area" overflow log).  Raw structured result
archived at `docs/audit/subsystem-survey-2026-05-29.json` (mine it; don't
re-run).  This is the sanctioned read-only-Explore carve-out to the
no-subagents rule — recorded in PLAN.md §7 + AGENT-WORKFLOW.md.

**Docs reorg:** new `STATUS.md`, `ROADMAP.md`, `port-ledger.{md,json}`,
`port-frontier.md`, `findings/INDEX.md`.  Updated `AGENT-WORKFLOW.md`
(session-lifecycle checklist; co-author trailer → Opus 4.8), `PLAN.md` §7,
`memories/HANDOFF.md`.  The forward target is unchanged: ROADMAP milestone
0, the title scene runner `FUN_0056aea0`.

---

## 2026-05-25 — Main pump / frame waiter port (FUN_005b1030)

Ported the 156-byte message-pump / frame-waiter
(`FUN_005b1030` → `app_pump_frame`) as a new `app_pump` module.  The
function is the inner gate the scene runner calls twice per loop
iteration; with this in place, porting the title-menu scene runner
no longer has a "missing prerequisite" gap.

Field-named the `app_ctx` struct (was `wp_app_ctx`, opaque pad after
`+0x08`).  The pump touches three previously-unnamed slots:
- `+0x0c limiter_enable` — master frame-limiter on/off.
- `+0x10 last_tick_ms` — `GetTickCount` sample from the previous
  pump exit.
- `+0x1c pump_throttle` — set by the limiter when re-arming, cleared
  by `WM_TIMER` (0x113) — the periodic tick paces the pump.  This is
  the same slot the WndProc already named `timer`; renamed to make
  the limiter coupling explicit.

The throttle re-arm condition pinned to UNSIGNED `prev - now < 5`
(disasm at 0x5b10b3 uses `jae`).  Equivalent to "GetTickCount hasn't
ticked since the previous sample" — a sub-tick spin guard that holds
the throttle until `WM_TIMER` clears it.  16 host tests cover the
limiter boundaries (==4 sets / ==5 skips), first-frame path, master-
disable, WM_QUIT short-circuit, drain-then-exit, and NULL ctx defense.

Refactoring done as part of the port: `wp_app_ctx` → `app_ctx`,
`g_wp_app_ctx` → `g_app_ctx`, `g_wp_active_flag` → `g_app_active_flag`.
The struct moved from `wnd_proc.h` to a new `src/app_pump.h` since
the pump is now the canonical owner; WndProc just `#include`s it.
`wp_state_init` no longer touches the shared globals — tests call
`app_pump_state_init` alongside.

Tests: 372 → 387 pass (+15 net; -1 layout test moved from wp_ to
app_).  Live boot still zero DDERR through 10 frames mode-2.

The pump is NOT yet wired into `main.c`'s per-frame loop — the
drop-in keeps its own minimal `main_loop_body` until the scene
runner ports and calls `app_pump_frame` retail-style.

---

## 2026-05-25 — Surface-paint leaves chip session (14 ports)

Marathon chip session.  Four checkpoints landed all the remaining
"small leaf" ZDD ports the title-menu scene runner needs.  +43 tests
(329 → 372), every commit cross-builds with mingw clean, live boot
still zero DDERR.

**Commit 55708b3** — Surface Lock/Unlock + clear + color descriptor +
keyed blit:
- `zdd_object_lock` (FUN_005b9490, vtable[25]).
- `zdd_object_unlock` (FUN_005b94d0, vtable[32]).
- `zdd_object_clear` (FUN_005b9410) — Lock + zero-fill + Unlock.
  Uses a new `zdd_object_get_locked_info` Win32 primitive to bridge
  the 4-byte-vs-8-byte pointer mismatch between retail and host.
- `zdd_object_blt_keyed` (FUN_005b9b70) — variant of `blt_onto` with
  positioned dest origin + DDBLT_KEYSRC.
- `zdd_bind_pixel_format` (FUN_005b8a20) — GetSurfaceDesc + mask
  extract.  New `zdd_color_descriptor` struct (22 bytes, replaces the
  unobserved `_pad000[0x18]` at zdd+0x00).
- `zdd_color_convert` (FUN_005b8b00) — RGB888 → surface-native pack.
- GetSurfaceDesc / Lock / Unlock Win32 primitives.

**Commit ed82ce6** — Mode-4 upscaler + create-screen wiring:
- `zdd_object_upscale_16bpp` (FUN_005b8ea0, 285B) — 2x software
  scaler.  Faithful to retail's hardcoded outer-stride caveat.
- Wired upscaler into `zdd_present` mode 4 (Zoom).
- Wired `zdd_bind_pixel_format` into `zdd_create_screen`'s 16bpp
  branch.

**Commit 4d6c590** — Wire color_convert into 16bpp set_color_key.
The TODO in `zdd_object_set_color_key`'s 16bpp branch is now an
actual `zdd_color_convert` call — magenta 0x00FF00FF correctly packs
into RGB565 0xF81F.

**Commit 4db9980** — Lost-surface recovery (FUN_005b91d0/_9240/_9ab0/
_9ac0).  Four functions: `zdd_object_is_lost`, `_restore_surface`,
`zdd_check_any_surface_lost`, `zdd_restore_all_surfaces`.  Backed by
two new Win32 primitives (IsLost vtable[24] / Restore vtable[27]).
Needed by the post-activate hook (FUN_005b14c0, unported) and any
later code that handles DDERR_SURFACELOST.

All ZDD leaves the title-menu scene runner needs (`FUN_0056aea0`)
are now ported.  Next checkpoint should start porting the runner
itself.

---

## 2026-05-25 — WM_PAINT handler ported + wired

Ported `FUN_005b9130` — the 151-byte WM_PAINT consumption handler — as
the new `zdd_window_paint(zdd *self, void *hwnd)` in `src/zdd.c`.
Sequence is retail-faithful: `if (mode != 2) return 0;
BeginPaint(hwnd) → zdd_object_get_dc(primary_obj) → BitBlt(window_hdc,
screen_pos_x, screen_pos_y, screen_width, screen_height, src_hdc, 0,
0, SRCCOPY) → zdd_object_release_dc(primary_obj) → EndPaint`.  Returns
1 iff consumed (caller's WndProc must then return 0 to the OS, since
we own the dirty region's validation via EndPaint).

Three new Win32 primitives in `zdd_win32.c`: `zdd_window_paint_begin`
(BeginPaint with a heap-allocated PAINTSTRUCT so its lifetime spans
the separate begin/end calls — the pure-logic body can't hold a
stack-frame PAINTSTRUCT the way retail does), `zdd_window_paint_end`
(EndPaint + free the cookie), and `zdd_window_blit_copy` (GDI BitBlt
with SRCCOPY on caller-supplied HDCs).  The blit primitive is distinct
from `zdd_desktop_present` — the latter wraps `GetDC(NULL)` +
`ReleaseDC(NULL)` internally, while this one trusts the caller to
hand it both HDCs.

Wired into `main.c`'s minimal `wndproc` via a new WM_PAINT case that
calls `zdd_window_paint(g_zdd, hwnd)`; if it returns 1 the message is
consumed, otherwise the case falls through to `DefWindowProcA` which
validates the update region itself.  Wired directly here (not via the
ported `wp_handle_message`) because the WndProc module is still in
isolation — `wnd_proc_win32.c`'s `wp_paint_check` remains a no-op
stub pending the input-subsystem ports needed to wire `wp_handle_message`
into main.

A retail-faithful quirk fell out of porting: the BitBlt uses
`screen_pos_x/y` (the same +0x138/+0x13c fields the per-frame present
dispatcher reads) as destination coordinates, but BeginPaint returns
an HDC whose origin is at the window's CLIENT-area top-left in CLIENT
coordinates.  For a window at e.g. screen (200, 300), the WM_PAINT
BitBlt destination becomes (200, 300) within the client area — off-
screen.  Retail does this too.  In practice the per-frame
`zdd_present` mode 2 fires every frame and re-paints via `GetDC(NULL)`
at the correct screen coords, so this misalignment is overwritten
before any frame fully composites.  See `zdd.h` `zdd_window_paint`
docstring for the full reasoning.

5 new host tests added (110 total in test_zdd.c): NULL-self guard,
non-mode-2 short-circuit (no primitives fire), NULL-primary_obj
defensive guard, mode-2 full sequence (verifies the exact retail
ordering — begin(seq=1) → get_dc(2) → blit(3) → release_dc(4) →
end(5) — plus HDC/cookie threading and coord passing), and a "doesn't
touch present-dispatcher primitives" anti-regression.  329 host tests
pass / 0 fail / 6 skip (was 324/0/6).  Cross-build with mingw clean;
`tools/run-opensummoners.sh --frames 10 --hide-window` still boots
through zero DDERR (hidden window doesn't see WM_PAINT damage; the
visible-window case will exercise the new handler on uncover).

---

## 2026-05-25 — Present dispatcher ported, smoke loop replaced

Ported `FUN_005b8fc0` — the 5-mode per-frame present dispatcher — and
its three leaves (`FUN_005b94e0` GetDC, `FUN_005b9500` ReleaseDC,
`FUN_005b9a40` blit_onto) as the new `zdd_present` /
`zdd_object_get_dc` / `zdd_object_release_dc` / `zdd_object_blt_onto`
functions in `src/zdd.c`.  Added three new Win32 primitives
(`zdd_surface_flip`, `zdd_surface_blt`, `zdd_desktop_present`) in
`zdd_win32.c`.  The drop-in's hand-rolled smoke-present loop
(`present_smoke_frame` in main.c) is removed; main.c now calls
`zdd_present(g_zdd)` directly each frame.

Two non-trivial discoveries fell out:

1. **`paint_ctx::FUN_005b94e0` / `_9500` are misnamed by Ghidra** — the
   ECX in every live callsite is a `zdd_object*` (specifically
   `zdd.primary_obj`), not a separate "paint_ctx" class.  Verified by
   r2 disasm at `0x5b9158` (`mov ecx, [esi + 0x16c]` in the WM_PAINT
   handler `FUN_005b9130`) and `0x5b90a1` (case 2 of `FUN_005b8fc0`).
   The `paint_ctx` typedef in `src/wnd_proc.h` is a misnomer for `zdd`
   itself — same `+0x16c primary_obj` / `+0x138..+0x144` rect /
   `+0x164 pixel_format_mode` offsets; its `+0x2c zdd_device`
   docstring is incorrect (that offset falls inside `zdd.log_buf` and
   isn't read by anything in the WM_PAINT path).

2. **`FUN_005b9a40` arg order**: `(this=src, dest_obj, dest_x, dest_y)`.
   The `this` parameter is the SOURCE surface; the receiver of the
   Blt vtable call is `dest_obj->com_primary`.  Confirmed by tracing
   case 4's push order at `0x5b900b..0x5b9010` against the function
   body's slot usage at `0x5b9a93` (the COM-dereferenced arg slot is
   the middle stack arg, not the first).  Also discovered: r2 names
   stack args by physical-offset (`arg_1ch` ≠ "first C arg") rather
   than by C-arg-index — caused a brief detour.

Mode 2's desktop-DC technique was also a discovery: case 2 uses
`GetDC(NULL)` (the *desktop* DC, set explicitly at function start via
`mov dword [hWnd], 0`) and `BitBlt` at `(screen_pos_x, screen_pos_y)`,
NOT `GetDC(hWnd)`.  This means `screen_pos_x/y` must be the window's
CLIENT-area top-left in *screen* coordinates for the BitBlt to land
inside the window.  Drop-in now keeps these in sync via a new
`sync_window_position()` helper, called once after `init_ddraw` (with
`ClientToScreen(g_hwnd, &pt)`) and again on every `WM_MOVE`.  Retail
must do this from somewhere we haven't traced — `FUN_005b9130` or a
WM_MOVE handler in the WndProc subsystem; deferred for now.

Mode 4 (Zoom) is partially ported: the dispatcher correctly fans
into `zdd_object_blt_onto` + Flip, but the upstream software
upscaler `FUN_005b8ea0` (285-byte 16bpp pixel-copy via Lock/Unlock)
is NOT ported.  Mode 4 in this port skips the upscale stage —
`back_obj_b` stays blank, so the Flip presents `back_obj_a`'s last
contents.  Mode 4 isn't the live boot mode; this only bites on a
Zoom-launcher selection.

`--no-smoke-present` flag renamed to `--no-present`.  Smoke-failure
counters / log lines in main.c removed (we're done with the
hand-rolled diagnostic; the dispatcher's own DDERR logging via
`zdd_log_dderr` → `OutputDebugStringA` → stderr is the new
diagnostic surface).

17 new host tests added (105 total in test_zdd.c): 4 for
`zdd_object_get_dc`, 2 for `zdd_object_release_dc`, 3 for
`zdd_object_blt_onto`, 8 for `zdd_present`'s mode dispatch.  324 host
tests pass / 0 fail / 6 skip (was 307/0/6).  Cross-build with mingw
clean; `tools/run-opensummoners.sh --frames 10 --hide-window` boots
through the dispatcher with zero DDERR.

---

## 2026-05-25 — Mode-2 smoke-present + two RE bug fixes it surfaced

Drop-in `main.c` now runs a per-frame smoke-present in launcher mode 2
(Windowed): `BltColorFill(0xF800)` on the offscreen primary, then
`GetDC` → `BitBlt` → `ReleaseDC` onto the window HDC.  Mirrors the
windowed branch of retail's `FUN_005b8fc0` (the engine's "Title Menu -
Flipping" path) but without porting `paint_ctx` yet.  Counts per-step
failures; the harness sees zero DDERR across all frames.  Gated behind
`--no-smoke-present` so the flag can be flipped off when the real
title-scene runner lands.

The first run of the smoke loop failed every frame with DDERR_NOCLIPLIST
(0x887600CD) — which surfaced two real RE bugs in the prior port:

1. **`zdd_object_create_surface_pair` was passing the wrong args to
   `zdd_object_stamp_metrics`.**  Retail's `FUN_005b95c0` calls
   `FUN_005b98c0` with `(p1, p2, p6, p7, p8/width, p9/height)` — NOT
   `(p1..p6)`.  Confirmed by r2 disasm at `0x5b95ff–0x5b9617` (push
   order on the stack before the call).  The wrong mapping left
   `metric_b8`/`metric_bc` holding the count flag (`1, 0`) instead of
   the surface dimensions (`640, 480`).

2. **`zdd_object_attach_clipper` passed a NULL pointer where retail
   builds a real `RGNDATA` on the stack.**  `FUN_005b9520` builds
   `{RGNDATAHEADER, RECT}` bounding the full surface from
   `self->metric_b8/metric_bc` and hands the struct address to
   `IDirectDrawClipper::SetClipList` (vtable[7] / byte 0x1c —
   confirmed by r2 at `0x5b95a7`, resolving the prior open
   ambiguity).  Renamed the Win32 primitive
   `zdd_clipper_set_clip_list_null` → `_rect` and threaded the
   dimensions from the now-correct metric slots.  Under
   `DDSCL_NORMAL`, an empty cliplist makes every subsequent `Blt` fail
   `NOCLIPLIST`; the fix unblocks all windowed-mode drawing.

DDERR debugging was also helped by dual-sinking
`zdd_output_debug_string` to stderr in `zdd_win32.c` (in addition to
`OutputDebugStringA`).  Without DbgView attached the engine's DDERR
builder output is invisible — the dup makes the harness output
line-oriented and self-contained.

307 host tests pass / 0 fail; mingw cross-build clean; `tools/
run-opensummoners.sh --frames 5` boots and runs the smoke loop with
zero DDraw errors.  See commit `5d82301`.

---

## 2026-05-25 — DDraw init WIRED into drop-in `WinMain` (mode-2 Windowed)

The boot-time graphics init chain is now end-to-end inside the drop-in.
`src/main.c` now calls (after window creation):

1. `zdd_create(&g_zdd)`           — DirectDrawCreateEx (FUN_005b7ee0)
2. `zdd_set_coop_level(g_zdd, hwnd, fullscreen)` — SetCooperativeLevel
   (FUN_005b89d0).  `fullscreen = (launcher_mode != 2)` — only Windowed
   runs DDSCL_NORMAL; the other modes use DDSCL_EXCLUSIVE|FULLSCREEN|
   ALLOWREBOOT.
3. `cs_dispatch_create_screen(g_zdd, launcher_mode, 1280, 960)`
   — the FUN_00582e90 driver that fans into per-mode CreateScreen.
   Zoom-target args (1280×960) mirror retail's `*(int*)(in_ECX +
   0x14/0x18)`.

Cleanup at process exit calls `zdd_destroy(g_zdd)`.  Until the
launcher settings record parser (FUN_005a4770, 45 KB) is ported,
the launcher mode is hardcoded to **2 (Windowed)** — overridable
per-run via `--launcher-mode=N`.  `--skip-ddraw` keeps the prior
"window-only" boot path for harness comparison.

Live boot via `tools/run-opensummoners.sh` runs clean to the success
path:

```
[opensummoners] init_ddraw: launcher_mode=2 (0=Full 1=Safe 2=Wind 3=DB 4=Zoom)
[opensummoners] init_ddraw: SetCooperativeLevel ok (fullscreen=0)
[opensummoners] init_ddraw: CreateScreen dispatch returned (success path)
[opensummoners] OpenSummoners exiting after 120 frames (1920 ms wall)
[launcher] child exited (rc=0)
```

No surface drawing yet — the title menu / scene loop (FUN_0056aea0)
is still unported, so 120 frames of empty main-loop run through with
no visible output.  This is the surface-creation plumbing, not a
renderer.

Tests unchanged at 307/0/6 (no port changes; just wiring).  Cross-build
clean.

Next: port the title-menu scene runner OR a "draw something to the
primary surface" smoke (BltColorFill on com_a) so we can confirm the
surface is actually writable in flight.  The 16bpp pixel-format
binding (FUN_005b8a20) gap may surface here.

---

## 2026-05-25 — `cs_dispatch_create_screen` PORTED (FUN_00582e90, 3560B outer mode dispatcher)

The outer mode-dispatch driver around `zdd_create_screen` lands.
3560 bytes of retail mostly-inlined strcpy/strcat collapse to a
single ~350-line port + a tiny Win32 leg + a setjmp-aware host
harness.  Reaches CreateScreen end-to-end from the launcher mode
arg.

New files:
- `src/cs_dispatch.h` — public driver + module globals
  (`cs_primary_pair`, zoom overrides, log buf, engine-name header).
  Test-only function-pointer hooks for `zdd_create_screen` and
  `zdd_object_new` so the host harness can verify per-mode dispatch
  without configuring the full ZDD surface-create stub chain.
- `src/cs_dispatch.c` — pure-logic dispatch.  Per-mode handlers
  (cs_mode_full / _safe / _windowed / _db / _zoom), error-log
  builder (`cs_log_append_failure` — the inlined strcat dance
  collapsed to one bounded helper), and a `cs_compute_zoom_rect`
  helper for mode 4's centre-rect math.
- `src/cs_dispatch_win32.c` — Win32 primitives
  (`cs_log_get_last_error`, `cs_fatal_log`,
  `cs_fatal_log_with_lasterror`, `cs_exit`) backing the dispatcher
  for the real build via `GetLastError` + `FormatMessageA` +
  `OutputDebugStringA` + `ExitProcess`.

Per-mode behaviour (matches retail):
- **mode 0 (Full)**: SetDisplayMode(640,480,16,0) → hide_cursor →
  busy_wait(2000) → zdd_create_screen(...mode=0, vmem=0) →
  zdd_object_new(&cs_primary_pair, 640, 480, 0x1ffffff, 1).
- **mode 1 (Safe)**: same fullscreen preamble; create_screen with
  mode=1, vmem=1; NO primary-pair alloc.
- **mode 2 (Windowed)**: NO preamble; create_screen with mode=2,
  vmem=1.  Only branch that doesn't touch the display mode.
- **mode 3 (DB)**: fullscreen preamble; create_screen with mode=3,
  vmem=1.  CreateScreen failure uses the FUN_00426110 "with
  lasterror" variant instead of the standard log builder.
- **mode 4 (Zoom)**: resolve desktop dims via
  `cs_zoom_override_*` or `zdd_get_display_mode`; bounds-check
  against launcher zoom target; SetDisplayMode + preamble; compute
  centre-rect blob; create_screen with mode=4, vmem=1, rect.
  Trailing `zdd_hide_cursor` on success (matches retail's literal
  end-of-mode-4 call; idempotent).

Error strings re-verified against `vendor/unpacked/sotes.unpacked.exe`
via `r2 ps @ 0x8a28e8/0x8a28bc/...` at port time.  The "\n" between
log entries (DAT_00854570) is confirmed: it's literally one byte
"\n", not " -- " or " — " as the variable name suggested.

Tests now: **307 pass, 0 fail, 6 skip** (up from 286; 21 new — 4
zoom-rect math, 3 log-builder, 1 prior-pair release, 5 happy-path
per-mode dispatch, 5 fail-path per-mode dispatch, 1 default-mode
no-op, 2 mode-4 GetDisplayMode/exact-match edges).  Cross-build
with mingw clean.

Open RE thread closed: `DAT_008a6ec0` (the primary-pair ZDDObject*)
is now named `cs_primary_pair` and owned by the cs_dispatch module.
`DAT_008a9534`/_9530 (the 0x638-byte rolling error log + dirty
flag) likewise.  `DAT_008a6eac`/_eb0 (zoom-mode display override
globals) modelled as `cs_zoom_override_width`/`_height`.

Remaining gap: `FUN_005b8a20` (16bpp pixel-format binding inside
`zdd_create_screen`'s post-success hook) is the one un-ported call
on the boot path.  ECX identity ambiguous (open RE thread); the
boot path may need this once the harness runs live.

Next: wire `cs_dispatch_create_screen` into the drop-in's WinMain
(currently retail still owns engine init) — the natural next
visible-output checkpoint.

---

## 2026-05-25 — `zdd_create_screen` PORTED (FUN_005b8480, 1088B, full 5-mode dispatch)

The big inner CreateScreen body lands.  Single-checkpoint port over
the leaves that came in this session: release_children prologue
(already had it), primary-surface DDSD builder + CreateSurface (new
Win32 leg), 8bpp palette setup (just ported), back-buffer attach
(just ported), and the existing orchestrator + clipper attach.

Per-mode wiring (mirrors retail exactly):
- **mode 0 (Full)**: CreateSurface primary (flippable, optional
  VRAM) → alloc primary_obj → attach_backbuffer with the videomem
  flag → clipper attach.
- **mode 1 (Safe)**: CreateSurface primary (non-flippable, no
  backbuffer count) → alloc primary_obj → create_surface_pair with
  videomem flag → clipper attach.
- **mode 2 (Windowed)**: SKIP primary CreateSurface (release any
  prior com_a) → alloc primary_obj → create_surface_pair with
  videomem flag → clipper attach.  Same code path as Safe minus
  the primary alloc.
- **mode 3 (DB)**: CreateSurface primary (flippable, no-VRAM-fold
  at the primary layer; videomem honoured at orchestrator) → alloc
  primary_obj → alloc back_obj_a + attach_backbuffer to it (forced
  no-VRAM) → create_surface_pair on primary_obj with videomem flag
  → clipper attach.
- **mode 4 (Zoom)**: like DB but adds a third ZDDObject — back_obj_a
  attaches the display-sized back-buffer (rect[0/1]), back_obj_b
  gets a source-sized orchestrator-created surface (rect[5/6]),
  primary_obj gets a source-sized orchestrator-created surface.
  Both orchestrator calls force VRAM (hardcoded p5=1).

Failure cleanup mirrors retail's "release just the latest failure"
pattern — prior ZDDObject slots leak on per-mode failure since
the caller (FUN_00582e90) exits the process via FUN_005bf5db(0)
shortly after.

**New struct fields**: `zdd_t` now has explicit `screen_pos_x` /
`screen_pos_y` (zeroed slots at +0x138/+0x13c — likely a paired
origin set elsewhere in the windowed-mode path), `screen_width` /
`screen_height` (+0x140/+0x144), and `screen_rect[7]` (+0x148..
+0x163).  `pixel_format_mode` doc updated to clarify the dual
role: it's the launcher's mode_arg (0..4), and FUN_005b8c00's
"== 2" check (Windowed) doubles as "needs explicit DDPIXELFORMAT".

**TODO**: 16bpp pixel-format binding via `FUN_005b8a20` is a one-
line TODO inside the post-success hook.  The boot path is bpp 16
so this gap may matter for visible output.  ECX identity ambiguous
(pixel-format descriptor object, not the calling ZDDObject) —
needs Frida verification at the first live call.

Tests now: **286 pass, 0 fail, 6 skip** (up from 272; 14 new — 5
primary-DDSD builder branch tests, 9 create_screen mode + edge
tests).  Cross-build with mingw clean (the 32-bit cross compile
exercises the new `_Static_assert` offsets for the 6 new fields).

Next: port `FUN_00582e90` (the outer dispatcher) — straight
transcription over `zdd_create_screen` now that the body is in.

---

## 2026-05-25 — back-buffer attach + 8bpp palette setup PORTED (closes 2 FUN_005b8480 leaves)

Two more leaves of `FUN_005b8480` (the big mode-aware surface-init)
land before tackling its body.

**`zdd_object_attach_backbuffer`** (FUN_005b9740, 153 bytes) — the
back-buffer fetch helper used by `FUN_005b8480` mode 0 (Full),
mode 3 (DB Mode), and the first leg of mode 4 (Zoom).  Pure-logic
orchestration over a new Win32 primitive `zdd_get_attached_surface`
that wraps `IDirectDrawSurface7::GetAttachedSurface` (vtable[12] /
byte 0x30).  The Ghidra decomp was misleading — Ghidra lost track
of the stack frame and emitted bogus `unaff_retaddr` params for the
trailing `stamp_metrics` call.  Disassembly via r2 confirmed the
real signature: `(self, primary_surface, width, height,
force_videomem)`.  Sequence: prefill_desc(0,0) → build DDSCAPS2 with
caps[0] = DDSCAPS_BACKBUFFER (4) or |DDSCAPS_VIDEOMEMORY (0x804) →
GetAttachedSurface into `self->com_primary` → stamp_metrics(w, h,
0, 0, w, h) → set_color_key(0x1ffffff sentinel).

**`zdd_setup_8bit_palette`** (FUN_005b8e00, 157 bytes) — 8bpp palette
allocator called by `FUN_005b8480` when bpp == 8.  Pure-logic
orchestration over two new Win32 primitives: `zdd_create_system_palette`
(GetSystemPaletteEntries + IDirectDraw7::CreatePalette vtable[5] /
byte 0x14 with DDPCAPS_8BIT, stash in `self->com_b`), and
`zdd_surface_set_palette` (IDirectDrawSurface7::SetPalette vtable[31]
/ byte 0x7c).  The orchestrator null-checks `self->com_a` before
SetPalette — matches retail exactly.

**Open RE thread closed**: `self->com_a` (ZDD +0x128) is the primary
display IDirectDrawSurface7.  Was previously listed as "Roles
unknown — likely IDirectDrawPalette + IDirectDrawClipper or similar"
in `zdd.h`.  FUN_005b8e00 calls SetPalette on it, and FUN_005b8480
mode 0/3/4 allocates it via `ddraw7->CreateSurface(..., &this+0x128,
NULL)`.  `com_a` doc comment updated to reflect the pinned role.

Tests now: **272 pass, 0 fail, 6 skip** (up from 265/6; 7 new — 3
attach_backbuffer, 4 setup_8bit_palette).  Cross-build clean.

Next: port `FUN_005b8480` itself.  All leaf deps are now in place
except `FUN_005b8a20` (16bpp pixel-format binding, ECX identity
ambiguous — likely a global pixel-format descriptor, not the
calling ZDDObject).  The body can land with the 16bpp branch
stubbed/TODO since the boot path (mode 0, bpp 16) is the
hot one and depends on that final stub being roughed in.

---

## 2026-05-25 — CreateScreen mode dispatch MAPPED + 4 leaf helpers PORTED

Setup checkpoint before tackling the big mode-aware surface-alloc
inside `FUN_00582e90`.  Two things landed:

**Documentation pass** — `docs/findings/ddraw-init.md` "FUN_00582e90
mode-dispatch CreateScreen" section now has the full 5-mode table
filled in (Full / Safe / Windowed / DB Mode / Zoom), the prologue
(release prior DAT_008a6ec0 ZDDObject), the centre-rect math used by
mode 4, and the 7 fixed error-string mappings (which retail's
`s_It_failed_in_CreateScreen___*` strings get logged per branch).
Mode 0 is the boot path (640×480, 16bpp) and the only branch that
calls our already-ported `zdd_object_new` factory.  Modes 1–4 each
dispatch through the still-unported `FUN_005b8480` (1088 bytes —
big internal-mode-aware surface-init that owns the three ZDD
slots at +0x16c/+0x18/+0x1c).

**Four leaf helpers PORTED** — the simple bits that FUN_00582e90's
prologue + each per-mode branch touch (none of which actually do
surface allocation; that's `FUN_005b8480`'s job in the next
checkpoint):

  1. `zdd_hide_cursor` (FUN_005b8dd0, 33 bytes) — inverse of the
     existing `zdd_restore_cursor_on_release`.  Gates on
     `cursor_state == 1` (currently shown) and calls
     `zdd_show_cursor(0)`.  Idempotent on already-hidden.

  2. `zdd_set_display_mode` (FUN_005b8900, 74 bytes) —
     IDirectDraw7::SetDisplayMode wrapper.  Vtable index 21 / byte
     offset 0x54.  Args: (w, h, bpp, refresh, 0=dwFlags).  Logs
     "DirectDraw.SetDisplayMode" DDERR on failure.

  3. `zdd_get_display_mode` (FUN_005b8950, 126 bytes) —
     IDirectDraw7::GetDisplayMode wrapper.  Vtable index 12 / byte
     offset 0x30.  Builds a stack DDSURFACEDESC2 with dwFlags=0x41006
     (HEIGHT|WIDTH|PITCH|PIXELFORMAT) + a pre-stamped ddpf
     (dwSize=0x20, DDPF_RGB).  Returns (w, h, pitch) via out-pointers
     — any of which may be NULL (mode 4 passes pitch as NULL).  No
     DDERR log; retail's caller picks the message itself.

  4. `zdd_busy_wait_ms` (FUN_005b5ac0, 39 bytes) — GetTickCount
     busy-spin.  All fullscreen branches pause 2000 ms between
     SetDisplayMode and surface-create — looks like a "let the mode
     transition settle" workaround.  Host stub instantly returns +
     records the argument so tests don't actually sleep.

**Open RE thread closed**: `FUN_005b9390` is exactly our existing
`zdd_object_dtor` — same body, byte-for-byte.  The pair
`FUN_005b9390 + FUN_005bef0e` in FUN_00582e90's prologue is exactly
`zdd_obj_destroy(&DAT_008a6ec0)`.  No new helper needed for that
slot.

Tests now: **265 pass, 0 fail, 6 skip** (up from 256/6; 9 new — 3
for hide-cursor, 2 for SetDisplayMode, 3 for GetDisplayMode, 1 for
busy-wait).  Cross-build with mingw still clean.

Next: port `FUN_005b8480` itself — the 1088-byte internal mode-
aware surface-init that all 5 branches funnel through.  Once that's
in, FUN_00582e90 becomes a thin dispatcher we can port in one shot.

---

## 2026-05-25 — Clipper attach PORTED (FUN_005b9520)

Closes out the ZDDObject's COM-attach lifecycle.  `zdd_object_attach_clipper`
(FUN_005b9520, 157 bytes per .text) does the canonical "release prior,
create new, configure, attach" dance:

  1. zdd_com_release(&self->com_back)
  2. self->parent->ddraw7->CreateClipper(0, &self->com_back, NULL)   vtable[4]
  3. self->com_back->SetClipList(&stack_NULL, 0)                     vtable[7]
  4. self->com_primary->SetClipper(self->com_back)                   vtable[28]

Pure-logic orchestration in zdd.c over three new Win32 primitives
(zdd_create_clipper, zdd_clipper_set_clip_list_null,
zdd_surface_set_clipper).  Splitting the primitives makes the
sequence verifiable on host: 4 new tests assert call order
(create -> list -> attach), pre-existing-com_back release, and the
defensive null-skip on CreateClipper failure + missing primary.

Two structural notes captured as open RE threads:
- The +0xac field on ZDDObject is dual-role: back-buffer
  IDirectDrawSurface7 for normal surface objects, IDirectDrawClipper
  for clipper-only objects.  Both implement IUnknown so the dtor's
  release path doesn't care, but the field name "com_back" is now
  misleading.
- The vtable[7] (SetClipList) call passes a stack-local NULL pointer
  as the LPRGNDATA, which is invalid input.  ddraw-init.md flagged
  this offset as potentially-mis-decompiled (could be SetHWnd at
  vtable[8]).  Our port mirrors the literal vtable+0x1c recovery;
  Frida verification recommended.

Tests now: **256 pass, 0 fail, 6 skip** (up from 252/6; 4 new host
tests for the clipper-attach call sequence).

---

## 2026-05-25 — CreateSurfacePair factory PORTED (FUN_005b8b40)

Wraps the surface-alloc stack up to the public engine entry point.
`zdd_object_new` (FUN_005b8b40, 184 bytes) does the canonical "allocate
+ ctor + orchestrator + cleanup-on-failure" sequence:

```c
zdd_object *zdo = calloc(1, sizeof(zdd_object));
if (!zdo) return 0;
zdd_object_ctor(zdo, parent);
if (!zdd_object_create_surface_pair(zdo, w, h, 0, colorkey,
                                    count, 0, 0, w, h)) {
    zdd_object_dtor(zdo); free(zdo); return 0;
}
*out = zdo; return 1;
```

Mirrors retail's `operator_new(0xd8) → FUN_005b9350 → FUN_005b95c0 →
on-fail FUN_005b9390 + FUN_005bef0e` byte-for-byte.  We swap operator_new
for calloc (deterministic zero-init; the subsequent ctor stamps every
observable field so the observable behaviour is identical) and the
heap-free primitive for free.

Side change: `zdd_object_create_surface_pair` (the orchestrator) gains
an int return type.  Retail's Ghidra decomp shows it as void but the
assembly leaves the last callee's EAX value as the implicit return;
FUN_005b8b40 reads that as int to decide cleanup.  The mapping:
0 means "CreateSurface failed OR (SetColorKey failed AND key was
non-sentinel)"; 1 means "everything OK or the sentinel path
short-circuited SetColorKey".  All 4 existing orchestrator tests still
pass since none of them was checking the return value.

Tests now: **252 pass, 0 fail, 6 skip** (up from 246/6; 6 new — 3 for
the orchestrator's new return-int behaviour across sentinel-success,
create-fail, and setkey-fail branches; 3 for the factory: happy path,
create-fail teardown, setkey-fail teardown).

---

## 2026-05-25 — Surface-alloc stampers + orchestrator PORTED (4 functions)

Closes out the ZDDObject surface-alloc sub-tree at the leaf level.
Four new functions land in the same `zdd.{c,h}` family:

  - FUN_005b97e0  `zdd_object_prefill_desc`              66 bytes
  - FUN_005b98c0  `zdd_object_stamp_metrics`             73 bytes
  - FUN_005b9830  `zdd_object_set_color_key`            138 bytes
  - FUN_005b95c0  `zdd_object_create_surface_pair`      110 bytes

Pure logic + tests on the host; the Win32 leg adds
`zdd_surface_set_color_key` (IDirectDrawSurface7::SetColorKey via
vtable[29] with DDCKEY_SRCBLT) to `zdd_win32.c`.

The 0x1ffffff colorkey sentinel was confirmed to be load-bearing:
the orchestrator's boot call site passes it (per FUN_005b8b40 →
0x5b95c0), so the SetColorKey vtable call is skipped entirely for
the primary surface alloc and `state_flag` stays 0.  Non-sentinel
keys stamp state_flag = 0x8000 and call through to the vtable.  The
16bpp branch inside FUN_005b9830 is a TODO — it expects to call
FUN_005b8b00 (a channel-shift converter that takes a pixel-format
descriptor in ECX, identity unresolved), but that branch is dead at
boot because the sentinel wins.  Once the descriptor's owner is
pinned, wire FUN_005b8b00's converted output in place of the raw key.

ZDDObject's struct now names 21 fields (up from 8): the three
self-pointers at +0x00..+0x08 (each points at a specific sub-field of
the embedded DDSURFACEDESC2 — lpSurface @ DDSD+0x24, lPitch @
DDSD+0x10, dwHeight @ DDSD+0x08), the six metric slots at +0x0c..
+0x20, the colorkey pair at +0x24/+0x28, the four secondary metrics
at +0xb0..+0xbc, and the two cached create-time args (caps_in,
force_videomem_in) at +0xcc/+0xd0.  Only `embedded_ddsd` (the
124-byte scratch DDSURFACEDESC2 at +0x30..+0xab) remains opaque
`uint8_t[]`.  Field-naming reflects byte offsets, not semantics — the
metric clusters likely hold src/dst rect TL/BR pairs based on the
orchestrator's argument shape, but no consumer reads them yet.

Tests now: **246 pass, 0 fail, 6 skip** (up from 234/6; 12 new
host tests across prefill / metrics / set_color_key / orchestrator
plus a "passes caps from field" test that verifies the deliberate
prefill→roundtrip→CreateSurface read-from-field path retail uses).

---

## 2026-05-25 — DDSURFACEDESC2 builder + CreateSurface PORTED (FUN_005b8c00)

Lands the meatiest "actually create a DDraw surface" function — 372
bytes of DDSURFACEDESC2 construction + vtable-call into
`IDirectDraw7::CreateSurface`.  Split clean: pure-logic descriptor
build (`zdd_build_surface_desc`) in zdd.c, Win32-side surface call
(`zdd_create_surface`) in zdd_win32.c.

Pure-logic dispatch verified by 10 tests covering each pixel-format
branch:

  - dwCaps = (caps_base | OFFSCREENPLAIN), |= VIDEOMEMORY when
    force_videomem OR self->videomem_flag
  - pixel_format_mode != 2 → short-circuits (no DDPF), even if a bpp
    is set
  - pixel_format_mode == 2 → DDPF block with:
      bpp 8:  PALETTEINDEXED8, bitcount=0, masks=0
      bpp 16: RGB, bitcount=16, RGB565 masks
      bpp 24/32: RGB, bitcount=N, masks = 0xFF0000/00FF00/0000FF
        (retail's switch literally falls 24 → 32 — the "engine quirk"
         from docs/findings/ddraw-init.md is preserved literally)
      other bpp: defensive — leaves bitcount and masks at 0

The Win32 wrapper additionally binds a palette (vtable[31] SetPalette)
when self->com_b is non-NULL — confirming the open-RE thread guess
that com_b is likely the IDirectDrawPalette (zdd-init.md hypothesis).
Failure path: zdd_log_dderr("DirectDraw", "CreateSurface", hr) + return 0.

Adds ZDD field `videomem_flag` at +0x134 (formerly pad).  Read by the
descriptor builder, not yet written — the higher-level mode-dispatch
FUN_00582e90 is what stamps it during fullscreen-mode init.

Tests now: **234 pass, 0 fail, 6 skip** (up from 224/6; 10 new
pure-logic descriptor tests).

---

## 2026-05-25 — ZDDObject ctor + dtor + pixel-buf release PORTED

Three more leaf ports landing on top of the ZDD wrapper checkpoint
that closes the "ZDDObject cleanup chain" open thread:

  - FUN_005b9350  `zdd_object_ctor`              50 bytes
  - FUN_005b9390  `zdd_object_dtor`              75 bytes
  - FUN_005b93e0  `zdd_object_release_pixel_buf` 42 bytes

ZDDObject struct shape pinned at 0xd8 bytes with the 6 lifecycle-
pair fields named (`com_primary` at +0x2c, `com_back` at +0xac,
`parent` at +0xc0, `pixel_buf` at +0xc4, `pixel_buf_flag` at +0xc8,
`state_flag` at +0xd4).  The embedded DDSURFACEDESC2 + window-fit
metrics regions (`_pad030[0xac-0x30]` and `_pad0b0[0xc0-0xb0]` +
`_pad0cc[0xd4-0xcc]`) stay as opaque pad until the surface-alloc
helpers (FUN_005b97e0 / _98c0) get ported alongside FUN_005b95c0.

The previously-placeholder `zdd_obj_destroy` in zdd_win32.c gets
replaced — it's now pure logic in zdd.c that walks
`zdd_object_dtor` then heap-frees the allocation.  Only the
`zdd_object_local_free` primitive remains on the Win32 side (wraps
LocalFree).

Release order in `zdd_object_dtor` matches retail's
FUN_005b9390 byte-for-byte: pixel buf first, then com_back (+0xac),
then com_primary (+0x2c), then parent->open_objects decrement.  The
"com_back BEFORE com_primary" order is the load-bearing detail — it
keeps the COM refcount graph clean when com_back is an
IDirectDrawSurface7 fetched via GetAttachedSurface off com_primary
(open thread per docs/findings/ddraw-init.md
"FUN_005b9520 — Clipper attach" notes).

7 new host tests; the two pre-existing release-children + dtor tests
were updated to use real malloc'd ZDDObjects instead of synthetic
pointers (zdd_obj_destroy now dereferences).

Tests now: **224 pass, 0 fail, 6 skip** (up from 217/5; 7 new
including a 32-bit-only zdd_object layout skip).

---

## 2026-05-25 — ZDD wrapper first slice PORTED (8 functions)

First slice of the DirectDraw 7 wrapper class HANDOFF flagged as the
recommended "next move" — the eight leaf functions in
`docs/findings/ddraw-init.md`'s call graph that together cover the
class lifecycle + DDraw init + DDERR error logging.  Lands in
`src/zdd.{c,h}` + `src/zdd_win32.c` + `tests/test_zdd.c`.

Ports (in size-ascending order):

  - FUN_005b8da0  `zdd_restore_cursor_on_release`     33 bytes
  - FUN_005b88c0  `zdd_directdraw_create_ex`          57 bytes
  - FUN_005b89d0  `zdd_set_coop_level`                71 bytes
  - FUN_005b7fe0  `zdd_dtor`                          90 bytes
  - FUN_005b7f80  `zdd_ctor`                          94 bytes
  - FUN_005b8040  `zdd_release_children`             139 bytes
  - FUN_005b7ee0  `zdd_create`                       153 bytes
  - FUN_005b80d0  `zdd_log_dderr`                    826 bytes

Pure-logic split matches the established bitmap_session / wnd_proc
pattern: ctor, dtor decision tree, DDERR-to-string mapping, and the
log-message builder live in zdd.c; the six Win32 primitives
(ShowCursor, OutputDebugStringA, IUnknown::Release via vtable[2],
DirectDrawCreateEx, IDirectDraw7::SetCooperativeLevel via vtable+0x50,
placeholder ZDDObject destroyer) live in zdd_win32.c.  Host tests
exercise the pure logic with controllable stubs.

The DDERR log message format was a small detective exercise: r2
pszj on each of the seven strings the helper concatenates (verified
against `docs/decompiled/by-address/5b80d0.c` and a fresh
`vendor/unpacked/sotes.unpacked.exe` read) showed retail uses commas-
in-place-of-periods in "Warning,exists ZDD errors," and " failed,Error
Code " — not typos, intentional.  The 18-entry HRESULT → DDERR_xxx
table in `k_dderr_table[]` mirrors the switch ladder in FUN_005b80d0
verbatim; format output is fully exercised by 4 host tests covering
known/empty-prefix/unknown-hresult/null-input paths.

Open follow-ups now:
- ZDDObject (`FUN_005b9350` ctor + the FUN_005b9390 cleanup chain) is
  still unported; `zdd_obj_destroy` in zdd_win32.c is a `free()`
  placeholder that will dispatch through the cleanup chain once
  ZDDObject lands.  Host tests don't touch it.
- Vtable indices for the COM Release calls (+0x128 / +0x12c) match
  IUnknown's standard but the semantic role of those two com pointers
  hasn't been pinned — `com_a` / `com_b` are deliberately vague.
  Likely IDirectDrawPalette + IDirectDrawClipper given the surrounding
  code, confirm when their setters land.
- `pixel_format_mode` / `pixel_format_bpp` (+0x164/+0x168) are written
  by paths we haven't ported yet (FUN_005b8c00 reads them when building
  DDSURFACEDESC2 in `mode == 2` paths).  Modelled as fields for size
  correctness; no consumer in this checkpoint.

Tests now: **217 pass, 0 fail, 5 skip** (up from 200 pass / 4 skip;
the new skip is `zdd_layout_matches_retail_offsets`, 32-bit only).
Real mingw build adds `-lddraw -ldxguid`.

---

## 2026-05-25 — Pixel-Drawer boot-time slot tables PORTED

Picks up an open-thread item from HANDOFF (the 5 fixed-size sprite-slot
allocator loops inside `FUN_00562ea0` lines 462-576).  All five groups
(`DAT_008a92b8` ×20, `DAT_008a9308` ×20, `DAT_008a9358` ×5,
`DAT_008a93bc` ×4, `DAT_008a936c` ×20 — total 69 slots) plus the four
special-colour writes that populate group D land in
`src/pixel_drawer.c` as `pd_boot_init_slots(fmt)` + a companion
`pd_boot_release_slots()` for host-test teardown.  All primitives this
calls into (`pd_blend_init`, `pd_blend_set_color`, `pd_blend_commit`)
were already ported in the first Pixel-Drawer pass — the boot driver
is purely orchestration.

Also corrects a finding doc: `winmain-and-bootstrap.md` claimed the
group-D 4 slots were "filled in later by code we haven't yet mapped" —
disassembly of FUN_00562ea0:0x5637f1-0x5638b6 (via radare2) shows the
4 special-colour writes ARE in the same boot phase, just written
inline as 4 explicit `mov ecx, [addr]` thiscalls that Ghidra's source
view collapsed into ambiguous untyped calls.  Targets are
D[0]/D[1]/D[3]/D[2] (in that order; D[2] and D[3] also get
commit_flag=1).  Boot driver replays this exact sequence.

Storage choice: static `PdBlend g_pd_boot_group_*[]` arrays rather
than retail's `PdBlend *DAT_X[]` heap-pointer-arrays.  Retail's slots
are process-lifetime allocations from `operator_new(0x50)` that are
never freed — static storage gives the same observable end-state with
zero malloc and is ASan-quiet under repeated boot.  If a future
consumer ever needs the pointer-array layout, add a parallel
`PdBlend *g_pd_boot_*_ptrs[]` view then.

Tests now: **200 pass, 0 fail, 4 skip** (up from 192).  8 new tests:
per-group weight/mode/state checks (A weight ramp /20 mode 1,
B weight ramp /22 mode 0, C grey-ramp R=G=B = 1100..1740,
E weight ramp /20 mode 2), group D 4 special-colour assignments
including the D[3]→D[2] retail-quirky order, full-coverage check
that every slot in every group commits its RGB masks from `fmt`,
custom-format propagation (RGB555 spot-check), and idempotency
re-run.  The idempotency test caught a real bug — `pd_blend_init`
zeroes channel.lut without freeing it first, which leaks on re-init
of a static slot — fixed by having `pd_boot_init_slots` call
`pd_boot_release_slots` at entry.  This is a host-build concern only
(retail allocates a fresh slot each boot via operator_new, so the
issue doesn't manifest in the real engine).

---

## 2026-05-25 — FUN_0057ca40 6th pass: 9 inline slot-clones PORTED (function functionally complete)

Closes the last deferred subsystem of FUN_0057ca40.  The 9 inline
FUN_00582b80 calls (the ones taking ECX = a `paVar1 = DAT_X` source
slot rather than going through the SS_MGR table) are extracted by
`tools/extract/57ca40_inline_clone_table.py` and replayed in retail
issue order by `ar_apply_group3_inline_clones`, called from the tail
of `ar_register_group3_sprites` after the SS_MGR clone pass.  3
distinct source pool indices (383, 390, 402) — all themselves
populated by the 1st-pass slot-register table — fan out into 9
disjoint targets (257..261, 384..385, 391..392).

No new primitive needed: each replay is just
`ar_sprite_slot_clone(pool[dst], pool[src])`.  The info-entry side
of each cluster (zero + marker/flag-copy + data-ptr for the 4 early
clusters; 20-byte STRUCT_COPY for the 5 late ones) is already
covered by the 4th-pass `ar_apply_group3_info_events` — verified by
re-running the audit tool `57ca40_pool_map.py` (0 orphans across all
443 pool writes).

With this pass landed, FUN_0057ca40's six retail-observable
subsystems (slot register, info events, SS_MGR clones, inline
clones, plus the two thiscall primitives) all replay in the port.
The next consumer of this state is `FUN_00586010` (palette-draw with
flag dispatch — see rabbit-hole §6); porting it will pin the
per-prefix flag semantics from the read side.

Tests now: **192 pass, 0 fail, 4 skip** (up from 187).  5 new tests
cover: target population after register, late-cluster shared-source
metadata propagation, early-cluster metadata propagation, apply
idempotency, src/dst-set disjointness.  Updated 2 existing tests
to reflect the new slot-count expectation (327 → 336).

See `docs/findings/0057ca40-rabbit-hole.md` §4 for the cluster
source/target table.

---

## 2026-05-25 — FUN_0057ca40 5th pass: 94 SS_MGR slot-clones PORTED

Last of the sprite-slot work in FUN_0057ca40.  The 94 FUN_004179b0
calls inside the function are extracted by
`tools/extract/57ca40_clone_table.py` and replayed in retail issue
order by `ar_apply_group3_clones`, called from the tail of
`ar_register_group3_sprites` after the 4th-pass info-events apply.
Total clones: 94 (54 distinct sources, 94 distinct destinations;
sources span main_slot 134..321, destinations span main_slot 135..322,
all within the 233-slot register region populated by the same
function's earlier pass).

The primitive `ar_ss_mgr_clone_slot(dst_pool_idx, src_pool_idx)`
reuses the existing `ar_sprite_slot_clone` (slot-metadata copy) and
`ar_info_entry_clear` (info-entry zero) primitives, since
FUN_004179b0's body is structurally identical to those primitives'
bodies — only the indirection differs.  Modeling sidesteps the SS_MGR
`this` pointer via a new unified-pool accessor `ar_pool_get_slot`
that maps pool indices 1..12 → ramp slots and 13..908 → main slots
(see rabbit-hole §7: SS_MGR == input_mgr at 0x008a6b60, so the host
already owns both tables as globals).

Tests now: **187 pass, 0 fail, 4 skip** (up from 176).  11 new tests
cover: pool accessor on sentinel/ramp/main ranges, primitive-level
clone (metadata propagation, info marker+flag copy, info data/palette
stay null, dst-entries destruction under ASan), table-walker (apply
idempotency, dst pool range, first-clone metadata propagation,
integration with register_group3_sprites).  Integration count test
updated: register pass writes 233 slots + clone pass writes 94 more
= 327 unique populated slots.

The remaining FUN_0057ca40 deferred work is now strictly on the **9
FUN_00582b80 cluster wiring** — open-coded template-slot init per
cluster, not table-extractable.  See HANDOFF "Next move" #3.

---

## 2026-05-25 — FUN_0057ca40 4th pass: 443 info-entry pool writes PORTED

Mechanical follow-on to the per-call-site indexing confirmation:
extracted the full info-entry event stream and replayed it as a
443-row static table walked by `ar_apply_group3_info_events`,
called from the tail of `ar_register_group3_sprites`.  Every write
the function performs to the parallel info-entry pool now lands in
the host model: 138 marker, 194 flag, 98 data-ptr, 5 struct-copy,
4 marker-copy, 4 flag-copy events spanning pool indices 92..437.

Extractor at `tools/extract/57ca40_info_table.py` mirrors the
`57ca40_sprite_table.py` model — re-run after re-export to catch
drift.  It captures 4 short-typed data-ptr writes (lines 2142,
2147, 2286, 2291) that the `57ca40_pool_map.py` audit's regex
missed — taking the real total from 439 to 443.  Sanity check
verifies all dst indices fall in [0, 909); the 5 struct copies'
sources (pool[139..145]) are not produced inside FUN_0057ca40, but
they're alloc-zeroed by the pool allocator and read at zero —
matching retail's allocator-zeroed pool semantics.

DATA_SET payloads (98 events; 25 distinct PE rdata addresses, e.g.
0x006748d0) are stored as opaque uintptr_t markers.  No consumer
reads them as bytes yet — the first FUN_00586010-style palette
draw with flag dispatch will need extracted PE bytes; this port is
observability-only on the data side.  8 new spot-check tests cover
the kinds (FLAG_SET, MARKER_SET, DATA_SET, MARKER_COPY, FLAG_COPY,
STRUCT_COPY) plus the bounded-region invariant (events only touch
pool[92..437]) plus the wiring through `ar_register_group3_sprites`.
Tests now: **176 pass, 0 fail, 4 skip** (up from 168).

The remaining FUN_0057ca40 deferred work is now strictly on the
**sprite-slot side**: 94 SS_MGR clones (FUN_004179b0) plus the 9
inline-clone clusters (FUN_00582b80 sprite-slot ops).  Info-entry
side is closed.

---

## 2026-05-24 — FUN_0057ca40 per-call-site indexing confirmed

Walked all 466 info-entry references inside FUN_0057ca40 and matched
them against slot decls + clone targets in the same function.  The
implicit "pool[i] shadows slot[i]" model is fully confirmed: 0 of
434 pool writes are orphaned (i.e. every info-entry write at retail
BSS `0x8a8440 + i*4` corresponds to a slot at `0x8a760c + i*4`,
where the slot is either declared inline, declared via the helper,
or produced by SS_MGR clone / inline-template clone).  Audit script:
`tools/extract/57ca40_pool_map.py` — rerun after re-export to catch
drift.

The walk also surfaced a Ghidra rendering gotcha for `DAT_008a8XXX`
references: different DAT vars carry different inferred C types
(byte-typed vs short-typed), so source-level offsets like `+2` vs
`+4` can both denote the same disasm byte offset (+4 = flag).
Verified by disasm at 0x57cad7 (byte-typed, mov [eax+4]) vs
0x57cf3d (short-typed, mov [ecx+4] but Ghidra source says +2).
Rabbit-hole §2 rewritten with the correction; the "pad +2..+3 is
never touched" claim from §4 is reaffirmed (no Ghidra +2 source
write actually targets byte +2 in retail).

This unblocks the 434-write port — it's now mechanical (compose
slot-idx → flag/marker/data tuples in retail issue order, replay
into `g_ar_info_table[i]`).  Deferred because no consumer reads the
info-entry pool yet; the first FUN_00586010-style palette draw with
flag-dispatch will need it.

---

## 2026-05-24 — ar_info_entry pool (909 entries) + allocator finding

Followed the "where do the parallel-table pointer slots come from"
thread to its root and unblocked the full pool model.  The allocator
is **FUN_00562ea0:225-253** — a single 909-iteration loop that runs
right before the "SS_MGR_Preparation" log line.  It heap-allocates
two parallel pools side-by-side: a 0x44-byte sprite slot AND a
0x14-byte info entry per index, stored in adjacent BSS pointer
tables at 0x8a760c..0x8a8440 and 0x8a8440..0x8a9270.

That pins three corrections we'd been waving past:

  - **`ar_info_entry` is 20 bytes, not 16.**  The allocator zeros
    five dwords (the last being `+0x10`); the existing `clear`
    routine only touches the first 14 bytes.  Struct + static
    asserts updated.
  - **`+0x0c` is a palette pointer**, not the "f_0c semantics
    unknown" placeholder.  FUN_00586010:755 reads it as
    `*(int ***)(DAT + 0xc)`, uses it directly as the source of a
    256-entry palette modifier loop when non-NULL, or falls back to
    `ar_palette_session_begin` + `FUN_00417bc0(entry->data, ...)`
    when NULL.  Renamed `f_0c` → `palette`.
  - **The "parallel pool" is 909 entries, not ~357.**  Retail BSS
    range 0x8a8578..0x8a8b14 (the rabbit-hole's "extends to ~357
    entries" estimate) is just pool indices 78..437 of the full
    909-entry table.

`g_ar_sprite_flags[14]` (flat uint32) replaced by
`g_ar_info_entries[909]` + `g_ar_info_table[909]`.  `ar_state_init`
wires the table.  `ar_register_palette_ramps` now writes through the
table: `g_ar_info_table[AR_INFO_RAMP_FLAGS_BASE + i]->flag = N`,
matching retail's `*(int *)(DAT_008a85xx + 4) = N` pattern.

One new pool-init test (168 pass / 0 fail / 4 skip, up from 167).
Two existing info-entry tests refactored for the new field name
(`f_0c` → `palette`) and the +0x10 field's leave-untouched
guarantee.  `docs/findings/0057ca40-rabbit-hole.md` extended with
sections 5 (allocator finding) and 6 (FUN_00586010 + FUN_00587e00
consumer evidence).  HANDOFF's "Open RE threads" entries on
g_ar_sprite_flags and the parallel-pool are now obsolete.

---

## 2026-05-24 — FUN_00582b80 (slot clone) + FUN_00582d00 (info entry clear)

Ported the next two functions from FUN_0057ca40's deferred subsystem
list: `ar_sprite_slot_clone` (the `__thiscall` slot metadata clone)
and `ar_info_entry_clear` (a 14-byte clear of a parallel-info-table
entry).  Together they form the "clone-and-detach pair" that appears
9× in FUN_0057ca40 — see `docs/findings/0057ca40-rabbit-hole.md`
section 4 for the disasm walk and the new struct discovery below.

The big finding is **`ar_info_entry`**, the 16-byte parallel-info-
table entry shape that HANDOFF previously called out as "each retail
entry is itself a POINTER to a struct."  Disasm at 0x57fa98 confirms
FUN_00582d00's `this` is loaded from `[0x008a8a40]` — a pointer in
the parallel table — and the writes pin the layout: u16 marker @+0,
pad @+2, u32 flag @+4, const void* data @+8 (later set to PE rdata
pointers like &DAT_006752f8), u32 f_0c @+12.  Struct + static
asserts now live in `src/asset_register.h`.

`ar_sprite_slot_clone` reuses `ar_sprite_slot_destroy` for its
free-old-state prologue, then stamps every metadata field from src
to dst in retail order, allocates a fresh 1-entry `entries[]`, and
deep-copies src's `aux_buf` (24-byte stride entries, count from
src->f_38).  Retail quirk preserved: `dst->f_38` stays at 0 even
when the aux deep-copy runs — the count isn't propagated; we match.

7 new unit tests (167 pass, 0 fail, 4 skip — up from 160).  Both
functions tagged in `TagThiscallFunctions.java` (26 tags now);
parse+tag stage confirmed `ar_info_entry=16` parses cleanly and
both new functions land in their class namespaces.  Module-isolation
still holds: no real caller is ported yet — the functions are
available primitives for the eventual FUN_0057ca40 wiring once
SS_MGR and the parallel-info-table array land.

---

## 2026-05-24 — Headless Parse C Source automation

Ported the `ParseCSource.java` script from sibling OpenMare to
`tools/ghidra-scripts/`, plus a `tools/ghidra-cpp-shim/` directory
with minimal `stdint.h` / `stddef.h` / `stdbool.h` shims for
Ghidra's bundled CPP (it has no libc).  ParseCSource passes the
shim dir as an include path and `-D_Static_assert(c,m)=` to strip
the C11 keyword from our headers, so `src/asset_register.h`,
`src/bitmap_session.h`, and `src/wnd_proc.h` parse cleanly into
Ghidra's DTM in headless mode.

`tools/ghidra-tag-and-export.sh` upgraded to a 3-stage pipeline
running in one analyzeHeadless session: ParseCSource → TagThiscall
Functions → ExportDecompiledC.  Verified end-to-end: all 14 structs
parsed (sizes match — paint_ctx=368, zdm_entry=56, log_singleton=1284,
etc.), 24/24 tags applied, 1768 functions re-exported.

Immediate payoff: the bodies of the 7 WndProc-dep thiscalls now
show typed `this->field` accesses.  paint_ctx::FUN_005b9130 reads
`if (this->state == 2) { BitBlt(hdc, this->blit_x, this->blit_y,
this->blit_w, this->blit_h, ...); FUN_005b94e0(this->back_ctx, ...); }`.
The WndProc itself reads as a clean class-dispatched function
(`input_mgr::FUN_0058ffa0((input_mgr *)&DAT_008a6b60, 1)`,
`zdm::FUN_005bbd20(DAT_008a93e4, ...)`, etc.).  No more
"open Ghidra GUI + Parse C Source" manual step — `nix develop -c
./tools/ghidra-tag-and-export.sh` is the single command for
struct-edit → typed-decomp.

See `docs/findings/cpp-recovery-workflow.md` "Automated parsing +
tagging" section for the full how-to + new-header discipline.

---

## 2026-05-24 — WndProc dependency formalization

Modeled the 5 "deep engine" struct shapes the WndProc reaches through
its 5 thiscall callees, and tagged each callee in Ghidra so its
prototype and class namespace get applied to the decomp.  The structs
live in `src/wnd_proc.h`'s new "deep-engine struct shapes" section
and pin only the offsets observed in the disasm:

  - **paint_ctx** (DAT_008a93cc) — +0x2c zdd_device, +0x138 blit
    rect (x/y/w/h), +0x164 state.  `this` for FUN_005b9130 (the
    BitBlt-from-backbuffer paint helper), FUN_005b94e0 (begin-frame
    vtable trampoline at zdd_device->vtable[0x44]), and FUN_005b9500
    (end-frame at vtable[0x68]).
  - **input_dev** (DAT_008a93d8, DAT_008a93dc[2]) — +0x04 dev_obj
    (vtable[0x1c] = Acquire), +0x08 acquired flag.  `this` for
    FUN_005ba290.
  - **zdm** (DAT_008a93e4) — +0x18 entries pointer, +0x1c count,
    +0x2c inline name string.  `this` for FUN_005bbd20 (the
    multiplexer set-active fan-out).  Per-entry struct **zdm_entry**
    has stride 0x38 with +0x00 dev, +0x08/+0x0c sub-device pointers
    (each with own vtable), +0x20 active, +0x24 state2, +0x28
    8-byte cookie.
  - **input_mgr** (singleton at &DAT_008a6b60) — +0x2884 zdm_ptr.
    `this` for FUN_0058ffa0 (input pause-on-deactivate; just NULL-
    guards and forwards to FUN_005bbd20).
  - **log_singleton** (singleton at &DAT_008a6620) — +0x404 path
    CHAR buffer.  `this` for FUN_00408b90 (the engine's
    OutputDebugString + log-file writer).

The wnd_proc.h externs (`g_wp_paint_check_this`, `g_wp_input_dev_extra`,
`g_wp_input_devs[2]`, `g_wp_zdm`) were upgraded from `void *` to the
typed pointer forms, and `wp_input_acquire`'s parameter became
`input_dev *` accordingly.

7 new tags added to `tools/ghidra-scripts/TagThiscallFunctions.java`
(now 24 total).  Headless tag step verified: 24/24 applied.  The
re-export was kicked off — the 7 new functions now show in the decomp
with class namespace + explicit `this` arg at every call site
(typed-body upgrade requires Parse C Source on wnd_proc.h in the
Ghidra GUI, then re-running the script — see HANDOFF "Next move" #1).

Tests unchanged at **160 pass, 0 fail, 4 skip** — the WndProc test
suite still binds `(void *)0xN` literal addresses into the new typed
globals via implicit `void *` → `T *` conversion.  Cross-build clean.

---

## 2026-05-24 — ar_register_group3_sprites (FUN_0057ca40 partial)

Ported the 233 sprite-slot register operations inside FUN_0057ca40 —
the "Ghidra-fails 24884 B body" from the prior HANDOFF turned out to
decompile cleanly once the typed-struct workflow from the previous
checkpoint was in scope (the `cpp-recovery-workflow` infra silently
fixed it).  3124-line decomp is at `docs/decompiled/by-address/57ca40.c`.

But the body isn't just "register N sprites" — it has FOUR
subsystems.  Only the first is ported here:

  1. **233 sprite-slot registers** (91 inlined + 142 helper-style
     calls).  Slot indices 79..423, all using uniform (zdd, settings,
     group) routing from the caller.  Table-driven through
     `ar_sprite_slot_register`.  **PORTED** as
     `ar_register_group3_sprites` and wired into `ar_boot_register_all`
     between aux_sounds and game_sounds (matches retail issue order).
  2. ~380 parallel-info-table writes (0x008a8578..0x008a8b14).
     **DEFERRED** — needs `g_ar_sprite_flags[]` refactored from flat-u32
     to pointer-to-struct array (~357 entries).
  3. 94 FUN_004179b0 SS_MGR slot-clones.  **DEFERRED** — needs SS_MGR.
  4. 9 FUN_00582b80 + 1 FUN_00582d00 tail.  **DEFERRED**.

See `docs/findings/0057ca40-rabbit-hole.md` for the full breakdown.
Generator at `tools/extract/57ca40_sprite_table.py` — re-run after
re-exporting the decomp to spot drift.

Tests: +7 new tests in `tests/test_asset_register.c` (distinct-slot
canary, group-tag stamping, uniform routing, three spot-checks for
specific entries, no-overlap-with-other-batches assertion).  Plus
the existing `boot_register_all_touches_every_batch_signature_slot`
test now also pins the group-3 spot-check on sprite[79].
**160 pass, 0 fail, 4 skip** (was 153 / 0 / 4).  Cross-build clean.

Asset-register module is now **functionally complete for the title-
scene boot path** — every `ar_register_*` batch that the boot driver
calls is ported, and no ported consumer reads the deferred FUN_0057ca40
state.

---

## 2026-05-24 — ar_boot_register_all wired

The 8 ported `ar_register_*` batches are no longer modules in isolation
— a new `ar_boot_register_all` in `asset_register.c` calls them in
retail issue order, matching FUN_00562ea0:613-624 byte-for-byte modulo
the one un-ported batch (FUN_0057ca40, group 3, 24884 B Ghidra-fails).
This is the "register every asset slot at boot" entry point: pass ZDD,
ZDS, settings, sotesp_module, and a locale state, and every ramp /
sprite / sound / GDI slot the title scene depends on lands populated.

API shape `ar_boot_register_all(zdd, zds, settings, sotesp_module,
locale)` keeps the conceptual settings-vs-sotesp split (in retail both
are the same DAT_008a6e74 pointer at boot; we accept them separately so
unit tests can distinguish "this register batch reads settings" from
"this batch reads sotesp.dll"). `locale == NULL` skips the locale-tail
batch entirely — useful for testing other batches in isolation; retail
always passes a valid struct.

The FUN_0057ca40 gap is marked with an inline comment at the exact
position it'd slot into (between aux_sounds and game_sounds).  No hook
mechanism — once Ghidra-fail is resolved and the function is ported,
the call is dropped in.

Tests: +6 new tests in `tests/test_asset_register.c` covering group-tag
routing, ZDD-vs-ZDS plumbing, the sotesp-module split (idx 0 +
ramp_slots use sotesp; everything else uses settings), locale state
plumbing, NULL-locale skip behaviour, and a "did every batch run"
canary check.  **153 pass, 0 fail, 4 skip** (was 147 / 0 / 4).  The
palette-install side of palette_ramps is a no-op in these tests
because the bs_load_pe_resource stub's resource table starts empty
when asset tests run; the install path is already covered separately
by test_bitmap_session.c's palette_ramps_* tests.

Cross-build clean.

---

## 2026-05-24 — ar_register_palette_ramps (FUN_0057a330) ported

Ported the 3919-byte sprite-batch palette function as
**`ar_register_palette_ramps`** — second-biggest sprite-register call
at boot.  Three observable sections, all wired in this checkpoint:

**12 palette-ramp blocks** — each registers a small (24×24 or 32×32)
type-2 sprite at one of 12 new `g_ar_sprite_ramp_slots[i]` (retail
BSS 0x008a7610..0x008a763c, a 12-pointer table that precedes the
main sprite pool's 0x008a7640 base), runs the same 3-color palette
ramp scheme as `ar_register_main_sprites`
(palette[1]=bg, [41..50]=mid, [51..70]=lerp(mid→fg, i/20)) with
per-ramp colors, then installs.  All 12 share the
`ar_palette_session_begin` / `ar_palette_install` path that landed
in the previous checkpoint — no new decoder code needed; the family
is now reused.  When the resource decoder fails (wrong bit depth
or missing resource) the install is skipped, matching the main
sprites ramp behaviour.

**23 trailing sprite registers** — main-pool indices 33..61 with
mixed icon / panel shapes.  Two of these (idx 36 at retail 0x76d0
and idx 38 at retail 0x76d8) are spelled inline as the
destructor-plus-field-writes pattern in retail; same observable end
state as `ar_sprite_slot_register`, so all 23 flow through the
helper here.  One entry (idx 37 at retail 0x76d4) is the only
register-call in the file that passes `settings=NULL` instead of
the launcher settings — special-cased in the iteration loop.

**14 portrait blocks** — each is a register-call at retail
0x8a7744..0x8a7778 (main pool indices 65..78, portrait/character art
80×{352,480,320,144,400}) followed by a write of a flag value (0 or
3) into a new parallel `g_ar_sprite_flags[]` table (14 entries
modelling the retail BSS region 0x008a8578..0x008a85ac).  The flag's
semantic meaning is unknown — likely a frame-count or facing-direction
override; no consumer is ported yet so we capture just the observable
+4 write into a flat uint32 array (the retail pointer-to-struct
indirection is unmodelled).

Function-level stack-local `bitmap_session` in retail is a vestigial
SEH-protected RAII placeholder — `bs_release_no_free`'d at entry,
`bs_release`'d at exit, never used.  No observable effect; not
modelled.

Tests: +9 new tests in `tests/test_bitmap_session.c` covering all
three sections.  **147 pass, 0 fail, 4 skip** (was 138 / 0 / 4).
Cross-build clean.  Ghidra TAGS array also gained the two thiscall
helpers it uses (FUN_004178e0 / FUN_00491770) so the re-exported
decomp shows typed `this->field` access through the family.

---

## 2026-05-24 — bitmap_session module + palette ramp wired end-to-end

New module **src/bitmap_session.[ch] + src/bitmap_session_win32.c** —
the 7-method `__thiscall` class behind the PE-resource bitmap decoder
in FUN_004178e0's palette-session front half, plus FUN_005b7c10
(compressed-resource header parser, a free function despite living in
the same family).  Lifecycle is entirely stack-managed in
FUN_004178e0 — the bitmap_session is `[esp+8]` over a 0x444-byte
frame.  Win32-free body; `bs_local_alloc_zeroed` / `bs_local_free` /
`bs_load_pe_resource` are externs supplied per build target.

The blocking ECX puzzle from the prior session's deferral (which
`this` does FUN_005b7800 actually run on?) was resolved by r2 disasm
of FUN_004178e0 — every callsite does `lea ecx, [esp+8]` before the
call, confirming the stack-local interpretation.  Outer this
(sprite_slot * in ESI) is read only for the HMODULE+resource_id pair
passed to FindResourceA.  Resource type string is "DATA", not "BMP"
as the prior findings draft assumed.

**ar_palette_session_begin** (FUN_004178e0, ar_sprite_slot method)
lands in asset_register.c — builds the stack session, calls
`bs_decode_resource(..., "DATA", 1)`, emits the BGRA palette into a
caller buffer iff the source was 8bpp.  Then
**ar_register_main_sprites' palette-ramp section** (previously
deferred) is now wired: allocate a 1024-byte buffer, seed via
ar_palette_session_begin from sotesp.dll/0x90b, override palette[1]=0,
palette[41..50]=0x383838, palette[51..70]=lerp(0x383838→0xffffff,
i=1..20 / 20), install onto slot[0] via ar_palette_install.  No-op
when the decoder can't return a palette.

Ghidra workflow improvement: TagThiscallFunctions.java's TAGS array
gained the 7 bitmap_session methods; re-export shows
`__thiscall bitmap_session::FUN_…(bitmap_session *this, …)`
throughout the family and `bitmap_session local_444[1080]` as the
typed stack local in FUN_004178e0.

Tests: +21 new bitmap_session tests (basic state, init/release,
compressed-header signature mismatch + happy path, raw + compressed
decode paths, ar_palette_session_begin BGRA emit + 24bpp skip, and
end-to-end ar_register_main_sprites integration).  **138 pass, 0
fail, 4 skip** (was 117 / 0 / 3 — the new skip is the
bitmap_session layout test, 32-bit-only).  Win32 cross build clean.
Commits: `8cb9fd8` (struct+tags+findings), `4f89867` (port+tests).

---

## 2026-05-24 — Asset-Register: palette-trio leaves (FUN_005b5d90 + FUN_00491770)

Ported the two leaf halves of the FUN_005749b0 palette-ramp trio
that don't need the PE-resource decoder:

**`ar_palette_pack_entry`** (FUN_005b5d90, 33 B) — pack a Win32
`COLORREF` (`0x00BBGGRR`) into one `PALETTEENTRY` (peRed, peGreen,
peBlue, peFlags=0).  Independent of any container.  Used between
the seed step and install step to override or lerp individual
palette entries.

**`ar_palette_install`** (FUN_00491770, 52 B) — lazy-install a
256-entry (1024-byte) palette onto a sprite slot's first entry.
Allocates `s->entries[0].b` on first call; the existing
`ar_sprite_slot_destroy` already frees it iff non-zero, so the
round trip is leak-clean.  The Ghidra decomp's
`*(int *)(*in_ECX + 4)` pattern is `(*this)+4` →
`entries[0].b` of `ar_sprite_entry` — `entries[0].b` is the
owned-pointer half of the entry record.

The third piece — `FUN_004178e0`, "begin palette session" — is NOT
ported.  It needs the whole PE-resource decoder chain
(`FUN_005b7800` + `FUN_005b71f0` + `FUN_005b7c10` + the small
release-helper group + `FUN_005b7b90` RGBA↔BGRA swap).  Blocking
question: which `this` does `FUN_005b7800` actually run on?  The
offsets `+0x3c` / `+0x40` in FUN_004178e0 match `ar_sprite_slot`
(`settings`, `resource_id`), but the bitmap-session layout
FUN_005b7800 needs (pixel buffer at +0x00, palette at +0x34..+0x434)
doesn't fit overlaid on an `ar_sprite_slot` (which has `entries`
at +0x00 and `aux_buf` at +0x34).  Most likely the actual ECX is
reset before the FUN_005b7800 call — the Ghidra decomp drops
__thiscall ECX setups for un-tagged callsites.  Full layout
analysis and resolution path are in
`docs/findings/palette-session.md`.

Tests: +6 (pack basic / top-byte ignore / overwrite; install
lazy-alloc / reuse / destroy round trip).  **117 pass, 0 fail,
3 skip**.  Win32 cross build clean.  Commits: `d3e8a00`, `6db790d`.

---

## 2026-05-24 — Asset-Register: FUN_0057b280 tail (ar_register_locale_sounds + ar_register_aux_sounds)

Closed FUN_0057b280's deferred backlog from the previous checkpoint
— two distinct ports landing in the same session:

**`ar_register_aux_sounds`** — the 4 inline `ar_sound_slot::FUN_00563ef0`
calls the boot driver (`FUN_00562ea0:617-620`) issues between
`FUN_0057a330` and `FUN_0057ca40`.  Hardcoded indices 22..25 with
IDs 0x4cb / 0x4ca / 0x4c8 / 0x4c9 (issue order), count 2 each,
group 2.  Same `ar_sound_slot_init` semantics as the rest of the
sound batches (`load_flag = 0`).  Tests: +3.  Commit: `d4198b0`.

**`ar_register_locale_sounds`** — the conditional locale-table loop
at the tail of retail FUN_0057b280.  Walks the 283-entry rdata
table at `0x00691018` (terminator = first +0x00 == 0 row) and
dispatches into the W_MGR sound pool keyed on three launcher-
settings globals (DAT_008a6e68 / _6e70 / _6e80+0x1c8), now exposed
as an `ar_locale_state` struct.  Two paths:
  - **PATH A (fallback)** when override==0 OR no current locale OR
    launcher flag suppresses it.  Resource id = entry.primary_id;
    settings = (flag==-1) ? locale.fallback : caller's settings.
  - **PATH B (locale override)** otherwise.  override == 0x7fff
    is the skip-when-active sentinel.  Resource id = entry.override;
    settings = locale.current.

Touched indices span 160..464 (267 distinct) — required bumping
`AR_SOUND_SLOT_COUNT` 256 → 512 to fit the 465-entry retail W_MGR
pool (allocated 0x1d1 entries in FUN_00562ea0's SS_MGR_Preparation
block).  Added `AR_SOUND_POOL_COUNT = 465` as the documented exact
retail capacity.

Table data extracted via `r2 px @ 0x691018` + a Python parser on
the resulting hex.  Only the five fields the loop actually reads
are kept in the static const C array; the magic / sequence /
metadata fields are summarised in the table-extraction comment.

Field shape observations from the parsed data (vs the previous
HANDOFF notes that pegged magic as 0xc35a):
- 23 distinct magic values appear in live entries (0xc35a..0xc35d,
  0xc4ae, 0xc754, 0xc756, 0xc760, 0xc77f, 0xc789, 0xc792, 0xc79c,
  0xc80b, 0xc829, 0xc83d, 0xe2a4..0xe2a8, 0x1874e, 0x18755, 0x18759).
  Magic is NEVER read by the loop — likely a zone/area tag for some
  other subsystem.
- field4 (`u32` at +0x04) is a per-locale group selector 1..73 (with
  gaps), monotonic per magic — looks like a "scene_id" the locale
  pre-loader can filter on.
- 15 entries have primary_id == 0 (sentinels skipped by the loop's
  `if (resource_id != 0)` early-out).  15 have override == 0x7fff.
  29 have flag == -1.
- count_add (`i16` at +0x14) is only ever 0 or 2; flag (`i32` at
  +0x18) is only ever 0 or -1; pad16 / field1e are always 0.

Tests: +7 (no-locale path → primary_id + fallback-or-settings,
primary_id==0 skip semantics, launcher_flag forces fallback,
override path under live locale, 0x7fff skip sentinel, coexistence
with `ar_register_game_sounds` at the 160..244 overlap, lazy-load
buffer pointer preservation).  **111 pass, 0 fail, 3 skip**.  Win32
cross build clean.  Commit: `aec8f15`.

---

## 2026-05-24 — Asset-Register: FUN_0057b280 (ar_register_game_sounds)

The "game sounds" boot-register batch — the sixth call in
`FUN_00562ea0`'s asset-register sequence (right after `FUN_0057ca40`,
called as `FUN_0057b280(ZDS, 3, settings)`).  Populates **174
single-slot sound-bank entries** in `g_ar_sound_table[]` covering
pool indices 12..244 (with 59 sparse gaps in that range).  Same
six-field write pattern as `ar_register_sounds` — every entry routes
through `ar_sound_slot_init` since the retail compiler's choice
between inline blocks (122 entries) and `FUN_00563ef0` thiscall
dispatches (52 entries) is observably identical (load_flag=0,
buffer untouched).

The pool-pointer table at `(&DAT_008a6ec4)[i]` only ran out to idx 11
in the previous port (the original AR_SOUND_SLOT_COUNT cap of 12 came
from `ar_register_sounds`).  Bumped `AR_SOUND_SLOT_COUNT` to **256**
(covers FUN_0057b280's max idx 244 with headroom) and renamed the
old 12-cap constant to `AR_SOUND_MAIN_COUNT` so `ar_register_sounds`
still loops over its exact 12-entry roster.  No retail BSS size for
the contiguous pool past 0x008a6ec4 is documented yet; bump again if
a later batch overruns.

Entry data lifted from the Ghidra decomp via a quick regex sweep
(122 `puVar2 = DAT_…; … puVar2[6] = ID;` blocks + 52 thiscall calls);
issue order preserved so any future call-trace test matches without
renormalisation.

**Deferred** — NOT in this port:

1. The 4 inline `FUN_00563ef0` calls the caller (`FUN_00562ea0:617-620`)
   issues at indices 22..25 with group=2 (IDs 0x4c8..0x4cb).  These
   sit between FUN_0057a330 and FUN_0057b280 in the boot sequence and
   write three slots in the "gap" of FUN_0057b280's range; need their
   own tiny helper when the boot driver gets ported.
2. The conditional locale-table loop at the tail of retail
   FUN_0057b280 (walks the 0x24-stride table at `&DAT_00691018`,
   dispatches into the pool keyed on locale state at DAT_008a6e68 /
   _6e70 / _6e80).  This is the language-pack / per-locale sound
   override path — needs reading the structured rdata table at
   0x00691018 and modelling the launcher-settings struct fields the
   branch reads.

Tests: +6 (total-entry-count 174, index range bounds + sample gaps,
field-write spot check across all 5 count buckets {1,2,4,6,8,16},
all-pairs distinct resource IDs, coexistence with `ar_register_sounds`
without group-tag stomping, lazy-load `buffer` preservation on re-
register).  Total **101 pass, 0 fail, 3 skip**.  Win32 cross build
clean.

---

## 2026-05-24 — WndProc: FUN_005b12e0 (wp_handle_message)

Ported the main game window's WndProc — the message handler
RegisterClassExA wires up for the engine's primary window.  The
function is small in code (441 bytes / 84 decomp lines) but
load-bearing: it owns `DAT_008a952c`, the "WM_ACTIVATEAPP wParam
mirror" the engine's outer pump (`FUN_005b1030`) spins waiting for.
The current Frida agent posts a fake `WM_ACTIVATEAPP(TRUE)` to flip
this flag because hidden retail windows don't naturally see the
message from the shell; a correctly-ported WndProc unblocks dropping
that workaround once we own the window registration.

Split into three TUs following the asset-register pattern:

- **`src/wnd_proc.c`** — pure logic, Win32-free.  Decodes the 9
  message classes the dispatch cares about (WM_DESTROY/MOVE/SIZE/
  PAINT/CLOSE/ACTIVATEAPP/KEYDOWN/TIMER + default→DefWindowProc),
  with the WM_ACTIVATEAPP activation half being the meaty branch —
  acquires the "extra" input device (with CP1/CP2 log surround),
  iterates the 2-slot device array, emits the unconditional CP3 log,
  flips the ZDM activation state, then runs the post-activate hook.

- **`src/wnd_proc_win32.c`** — Win32 adapters.  `wp_def_window_proc`
  → DefWindowProcA, `wp_app_exit` → ExitProcess, `wp_log_cp` →
  OutputDebugStringA.  The five "deep engine" hooks (paint_check,
  app_pause, input_acquire, zdm_set_active, post_activate) are
  placeholder no-ops — none of those subsystems are ported yet, but
  swapping each for a real call is a one-line change once they are.

- **`src/wnd_proc.h`** — typedefs Win32 message types as pointer-
  sized integers so the pure logic compiles + tests on Linux.
  Models `wp_app_ctx` with just the fields FUN_005b12e0 reads
  (`f00` head of the device-init pointer chain, `loaded`, `timer`).

The "device init flag" subtlety: retail's activation path computes
`bVar1 = !(ctx->f00 && *ctx->f00 && (*ctx->f00)[+0x18])` then passes
`!bVar1` to ZDM.  I.e. the ZDM arg = "the chain is fully wired".
Disasm at 0x5b13b5..0x5b13c8 + 0x5b1462..0x5b146a confirms this is
literal pointer-deref-pointer-deref + +0x18 read.  Modelled with two
test cases that build the chain explicitly with stack-local int
buffers + a sub-pointer.

Tests: +20 (harmless messages, close→exit, paint short-circuit
combinations, ACTIVATEAPP flag-write semantics, full call-order
spec for the activation path, log-quiet gate, sparse loop, ZDM
arg = init_flag both true and false, timer field-clear, state
reset, layout assert).  Total **95 pass, 0 fail, 3 skip**.  Win32
cross build clean (single-TU mingw picks the new .c files up
automatically).

Not done in this commit: tagging the WndProc's dependency thiscalls
(FUN_005b14c0 / _0058ffa0 / _005ba290 / _005bbd20 / _005b9130 /
_00408b90) in Ghidra.  The script needs each class struct in the
DTM via Parse C Source, and we only modelled one (`wp_app_ctx`) —
the rest are opaque void* in the port.  Defer to a follow-up that
formalizes the input/ZDM/paint-context layouts.

---

## 2026-05-24 — Asset-Register: FUN_0056e190 (ar_register_game_sprites)

The "hundreds of sprites" boot-register batch — the fifth call in
`FUN_00562ea0`'s asset-register sequence (right after
`ar_register_main_sprites`, called as `FUN_0056e190(ZDD, 5, settings)`).
By far the biggest sprite batch at boot: **442 single-entry sprite
registers** packed into a table-driven port that iterates
`ar_sprite_slot_register` once per entry.

The retail decomp is 2782 lines structured as:

- **93 inline blocks** at idx 425..517 (BSS 0x008a7ce4..0x008a7e54)
  — the compiler chose to open-code the destructor + field-write
  sequence rather than emit a call, because the `this` pointer was
  visible as a global.  Resource IDs are sequential 0x592..0x5fb.
  72 use shape (0xa0×0xb0, scale=1, type=0); 21 (resource IDs
  0x71f..0x733, idx 467..487) use (0xb0×0x90, scale=1, type=0).

- **349 trailing FUN_005748c0 thiscalls** with implicit-ECX slot
  pointers that Ghidra dropped from the C view (only the 8
  non-ECX args show up).  Re-extracted the ECX setups via
  `r2 -c 'pD 0x672c @ 0x56e190' | awk '/mov ecx, dword \[0x8a/{last=...} /call 0x5748c0/{print last}'`
  → paired one-to-one with the 349 C-decomp arg lists by file
  order.  Three sprite shapes: 0xa0×0xb0 / 0xb0×0x90 (type=0, scale=1)
  and 0x80×0x80 (type=2, scale=0 — small icon).  Touches 346
  indices in idx 518..863 plus the low-index stragglers at idx 62/63/64
  (resource IDs 0x608/0x609/0x60a, the 0x80×0x80 icon shape).

**Pool refactor**: `AR_SPRITE_SLOT_COUNT` bumped 64 → 1024 to fit
the new high-water-mark (idx 863) with headroom for the remaining
batches (FUN_0057a330, _57ca40, _57b280 likely add a few dozen more
slots).  Retail's contiguous BSS region past 0x8a7640 is plenty
large; storage cost is ~70 KB BSS.

Tests: +6 (inline-block field-map at shape-shift boundaries,
trailing-call shape spot-check across all three shapes + low-idx
stragglers, total slot count = 442, resource-id uniqueness pin
across the whole batch, untouched-indices stay zero, coexistence
with `ar_register_main_sprites`).  Total **75 pass, 0 fail, 2
skip**.  32-bit cross build clean — pool capacity bump verified at
compile time.

---

## 2026-05-24 — Asset-Register: FUN_005749b0 (ar_register_main_sprites)

UI/menu sprite-register batch — the fourth call in `FUN_00562ea0`'s
asset-register sequence (after `FUN_00579bd0`, `FUN_00579a00`, and
the four `FUN_00563ef0` sound-bank loads).  Populates 34 sprite
slots: 9 inline registers, 1 special transient register (idx 0, id
0x90b loaded from sotesp.dll instead of the launcher settings
record), and 24 trailing single-call registers.  Most slots are
640×480 full-screen backgrounds and 368×276 UI panels; the
stragglers at indices 46/47/50/55 are small icons (32×32 / 64×64).

**The ECX-hidden mystery**: the C decomp shows one `FUN_005748c0`
call without an obvious `this` pointer — Ghidra dropped the thiscall
ECX setup.  radare2 disasm at 0x00574e0a reveals
`mov ecx, dword [0x8a7640]` — the slot at DAT_008a7640 (idx 0 in
our unified pool).  Same slot is also the target of the palette
ramp that follows.  So this one slot is BOTH register-populated
AND palette-decorated, while the inline slots and trailing calls
get only the register pass.

**Refactor**: `g_ar_sprite_slots` went from a 2-entry array
(FUN_00579bd0-specific) to a 64-entry unified pool indexed by
`(retail_BSS_addr - 0x008a7640) / 4`.  FUN_00579bd0's two
font-texture slots now live at `AR_SPR_FONT_TEX_457` (= 42) and
`_455` (= 43).  Existing tests updated mechanically.  Future
batches (FUN_0057a330, FUN_0056e190) plug into the same pool —
bumping `AR_SPRITE_SLOT_COUNT` as needed.

**Skipped**: the palette ramp section between the slot-5 and slot-9
inline writes — builds a 256-entry palette via the palette-session
trio (FUN_004178e0 / _005b5d90 / _00491770) and installs it onto
the idx-0 slot.  Documented in the driver docstring; will land
when the palette-session trio + PE-resource decoder do.

Tests: +6 (inline-slots field map, transient sotesp slot, trailing
IDs in index order, untouched indices stay zero, total slot count,
coexistence with `ar_register_fonts`).  Total 69 pass, 0 fail, 2
skip.  32-bit cross build clean.

---

## 2026-05-24 — Asset-Register: FUN_005748c0 (ar_sprite_slot_register)

Exposes the single-entry sprite-slot register as a public helper —
the same shape used by FUN_005749b0, FUN_0057a330, and the hundreds-
of-sprites mega-register FUN_0056e190.  Previously this lived as a
static helper (`ar_sprite_slot_register_init`) inside the module,
parametrized over `entry_count`.  All known retail callers pass
entry_count=1, so the public form hardcodes it — matches FUN_005748c0
exactly (operator_new(8) + 1-entry zero-loop + named field writes).

`ar_register_fonts` now calls the public helper instead.  Field-init
behaviour is unchanged; the test `register_fonts_sprite_slots` still
passes and pins the same slot state.

**Pivot vs handoff recommendation**: deferred the FUN_00563ef0 wave-
load second half.  Per-resource WAVE loading at boot is dead code
(boot batch passes load_flag=0 everywhere), and the dep chain pulls
in DSound vtable mocks + mmio + PE-resource — sizable test scaffolding
for code that doesn't run.  Instead picked the highest-leverage
building block on the title-menu critical path: the per-slot register
that the next three boot-driver calls (5749b0/57a330/56e190) all share.

Tests: +4 (full field-init map, destroy-on-reregister with aux_buf
+ multi-entry array, uint16 truncation, retail call-shape spot check
against FUN_0057a330's first arg list).  Total 63 pass, 0 fail, 2
skip.  32-bit cross build clean — `ar_sprite_slot` still 0x44 B.

---

## 2026-05-24 — Asset-Register: FUN_00579a00 (sound batch)

Second port in the asset-register batch — `FUN_00579a00` registers 12
sound-bank slots at DAT_008a6ec4..6ef0 ("W_MGR" pool).  Adds the
`ar_sound_slot` type (0x18 B; layout asserted) and the matching field-
init helper `ar_sound_slot_init` — which is also the first half of
`FUN_00563ef0` (the boot batch passes `load_flag = 0` so the wave-load
second half is dead code at boot).

The roster is a 12-entry table of (resource_id, count/kind) — 8 kind-2
slots, 4 kind-4 slots, hitting IDs in two ranges (0x506..0x510 and
0x4d8..0x4d9, plus one outlier at 0x903).  Eleven are written inline
in retail; the twelfth (table[8], id 0x50c) dispatches through
FUN_00563ef0 with load_flag=0 — disasm confirms the field-writes are
identical, so the port routes all 12 through the shared helper.

The `buffer` field at +0x04 is the lazy-load "already loaded?"
sentinel.  The init helper deliberately leaves it untouched; the test
`register_sounds_buffer_pointer_preserved` pins this so a future
refactor can't accidentally clobber it.

Tests: +4 (field-init, state clear, full 12-slot roster verification,
buffer preservation).  Total 59 pass, 0 fail, 2 skip.  32-bit cross
build clean.  Cumulative across the session: 13 functions ported into
the Asset-Register module across the FUN_00579bd0 and FUN_00579a00
boot batches.

---

## 2026-05-24 — Asset-Register module: FUN_00579bd0 family

Second ported module lands.  `FUN_00579bd0` is the first asset-register
batch call from the boot driver (after Pixel-Drawer set, before "The
resource was set" log line — see `docs/findings/asset-loader.md`).
Pulls in 11 functions total: the top-level batch, the 9 supporting
slot-setter / GDI-primitive helpers it calls, and the delete-array
thunk underneath everything.

**Module shape (`c4d2da0`)**: two struct types modelling the retail
in-memory slot layouts.  `ar_gdi_slot` (12 B) holds a fixed-capacity
HGDIOBJ array — the shape used by DAT_008a9274[idx] entries 1..15
in the boot batch.  `ar_sprite_slot` (0x44 B) is the sprite-slot
shape from FUN_0056e190 ("hundreds of similar blocks", per asset-
loader.md) — two are touched here (DAT_008a76e8 / _76ec for the font
texture slots).  Layouts asserted with `_Static_assert` blocks live
on the 32-bit cross build.

**Win32 isolation**: `asset_register.c` is pure logic.  The four GDI
primitive wrappers (`ar_gdi_create_font/pen/brush`, `ar_gdi_delete`)
are externs supplied by `asset_register_win32.c` (real build picks
it up via `src/Makefile` wildcard) or by the test harness (recording
stubs that log every call into a per-kind table).  Tests then assert
on call order + arguments without touching real GDI.

**Retail quirks preserved as comments rather than code**: the
`operator_new(4)` leak in `FUN_00579f40` (omitted, ASan-clean and
no observable effect); the asymmetry where `set_font` leaves
`count=0` but `set_pen`/`set_brush`/`set_pen_gradient` bump it;
the middle-loop bound in `FUN_00582d10` that makes gradient
capacities 0/1/2 skip the middle entirely.

**Ghidra recovery gap closed via radare2**: the bottom-block calls
`FUN_0057a030(4,8,0,group)` / a1a0 / a260 had their ECX setup
dropped from the C decompile.  Disasm at `0x579df8 / 0x579e05 /
0x579e1a` shows ECX = `[0x8a9298]` / `[0x8a92ac]` / `[0x8a92b0]`,
which decode to table indices 9, 14, 15.

**Tests**: +24 new (lerp arithmetic incl. descending channels and
alpha skip, GDI destruct order incl. NULL-hole handling, all 7
slot setters individually, `ar_register_fonts` end-to-end on sprite
+ GDI slot indices + call-order verification, layout parity).
Total: 55 pass, 0 fail, 2 skip (skips are the 32-bit-only layout
asserts; they fire at compile time on the cross build).  32-bit
cross build verifies layout parity, both `opensummoners.exe` and
`opensummoners-debug.exe` build clean.

Module is in isolation — not yet wired into the drop-in's boot
path.  Wiring waits until `FUN_00579a00` / `FUN_0057a330` /
`FUN_0056e190` land so calling it has a visible effect.

---

## 2026-05-24 — Test harness scaffold + first ported module (Pixel-Drawer)

Pivoted from "extract assets to spec the format" → "RE the init sequence
and reimplement methodically with tests" (the openrecet model).  Six
commits across one session.

**Harness fix (`8d6855c`)**: the `--max-frames` cap formula in
`frida_capture.py` was `msg_ticks * 250 >= max_frames`, which at the
default `max_frames=600` fired at just 3 emitted batches (~750
drained messages) and pre-empted any `--duration-ms` longer than
~12 s.  Now compares against the true running count carried on each
`msg` event; default bumped to 30000.

**Sprite format spec (`a2e5cb0`)**: archaeology pass on the
`sotesd.dll` DATA blobs spec'd the `0x425f` sprite family layout
(32 B outer magic + 1024 B BGRA palette + 64 B sub-table + 14 B
BMFH-style preamble + 8 B sub-sig + W×H 8bpp pixels).  213 of 759
DATA blobs parse cleanly.  W/H aren't in the file — they come from
`FUN_0056e190`-family registration calls.  Extractor at
`tools/extract/lizsoft_sprite.py`.  User redirected away from
chasing the asset extraction further: "we will load sprites the
same way as retail exe does it" once the init replay catches up.

**Title-scene runner (`07088a7`)**: `FUN_0056aea0` mapped fully.
8-phase intro animation (studio-logo fade → title fade → "press
button" → particle hand-off), pump+frame-budget cadence, the
default-branch menu-action latch via `FUN_0043ce50`, lazy DInput
pad attach on first menu-confirm.  Also extended
`winmin-and-bootstrap.md` with the state-code → next-scene map for
the outer driver's 0/6/8/9/0x1a..0x1e codes, and the Pixel-Drawer
slot-table allocation phase (69 slots in 5 fixed-size groups).
This is the first-rendered-frame bridge from boot done to actual
DDraw Flip.

**Test harness scaffold + Pixel-Drawer leaf primitives (`a53c141`)**:
`tests/{t.h, test_main.c, Makefile}` mirroring openrecet's pattern —
host gcc + ASan/UBSan, X-macro registry, `T_ASSERT_*` macros,
name-filter via `$F`.  Ported the 5 leaf primitives of `FUN_005bd*`:
mask→shift encoder, channel ctor, channel free-LUT, slot ctor, slot
SetColor.  13 tests; layout parity enforced via `_Static_assert`
blocks active on the 32-bit cross build.

**Pixel-Drawer LUT builder (`bb8c706`)**: `FUN_005bd040` (801 B,
four blend formulas + shared-LUT short-circuit).  Modes: 1=add,
2=sub, 3=lerp-variant-A, 4=channel-weight-coupled, default=lerp.
Floor-correction terms preserved literally even where they're
always-zero for valid weights, in case the engine ever passes
out-of-range inputs.  10 new tests with hand-computed expectations.

**Pixel-Drawer slot commit + mask reader (`aa0e62c`)**: `FUN_005bd3d0`
ties the leaf primitives together (free LUTs → resolve format from
PdFormat or RGB565 default → encode masks → resolve slot.state →
rebuild LUTs in R/G/B order with B sharing R-not-G).  The 8-byte
sub-detail that "B can share with R but never with G" preserved
verbatim from the retail call sequence at 5bd456/45f/468.
Pixel-Drawer module is now complete: 7 functions ported, 30 tests
passing on host (1 layout-test host-skip), 32-bit cross build clean.

Status: test harness is established and the first complete module
is in.  Next sessions should look at the remaining boot-driver
phases that have clear consumer relationships — likely the
ZDD wrapper (DDraw surface mgmt — Win32 heavy, needs mock layer or
just port + verify via the smoke harness) or the asset register
batch (`FUN_00579bd0` fonts, `FUN_0056e190` sprite slots et al —
consumes Pixel-Drawer slots so it integrates our work directly).
Skip the SS_MGR/W_MGR/GD_MGR boot pools until we know their
consumer semantics — they're just `operator_new` loops in
isolation.

---

## 2026-05-24 — Phase 1+2 push: audio, asset loader, config.dat, DDraw surface builder

Long unattended session.  Six commits, four new findings docs, two
new extractors.  Highlights:

**Audio + Input init (`docs/findings/audio-init.md`):** corrected the
prior mis-labeling of `FUN_005b9fc0` as "wave audio" — it's actually
the DInput keyboard sub-device, following `FUN_005b9cf0` (ZDI main /
`DirectInputCreateEx` with version `0x0700`).  DSound primary buffer
is created with `DSBCAPS_CTRLVOLUME` so the engine can master-attenuate
via `SetVolume`.  The launcher's "Disable Sound" gates ZDM (music
mgr) init only — DSound still inits either way.  ZDM allocates 50 ×
576-byte voice slots.  DInput is loaded by `Ordinal_1()` (= legacy
`DirectSoundCreate`), not by name — a quirk in `FUN_005bb180`.

**DDraw surface alloc (`docs/findings/ddraw-init.md`):** decompiled
`FUN_005b95c0` + `FUN_005b8c00` — the actual `IDirectDraw7::CreateSurface`
path.  Identified the engine's `0x01FFFFFF` "no color key" sentinel
(was previously mis-read as an "unlimited hint"), the 24bpp→32bpp
case fallthrough in the pixelformat switch, and corrected the
IDirectDrawSurface7 vtable cheat sheet (Lock at offset 0x64 not 0x60,
Unlock at 0x80 not 0x7c — 0x7c is SetPalette).

**Asset loader (`docs/findings/asset-loader.md` + `tools/extract/sotes_resources.py`):**
the three companion DLLs are pure resource-only PEs.  Wrote a
zero-dep PE-resource walker that dumps every entry to `type=<T>/<ID>.bin`
with a manifest.  Real content map:

- **sotesp.dll**: 31 WAVE SFX + signature blob
- **sotesw.dll**: 47 WMA music files (in `DATA` type, despite the
  earlier "MUSICWMA" speculation)
- **sotesd.dll**: 759 DATA blobs (~135 MB) + 436 WAVE SFX (~26 MB)

`FUN_005b6340`'s "kind 2 source" turns out to be a chunked-memory
abstraction (`FUN_005b67c0` spans 676996-byte chunks) — NOT
decompression as initially guessed.  This means sotesd 1000–1004
(each exactly 676996 bytes) is one logical 3.4 MB blob assembled
at boot.  Assets are stored RAW.  Sample DATA inspection shows
Lizsoft sprites have a 32-byte header + 256-entry BGRA palette
+ pixel data.

**Signature integrity checks (new engine-quirk §13):** all three
DLLs carry the same byte-encoded ASCII signature scheme.  Each
DLL has a resource that decodes (byte + `0x41`) to a known string:

| dll          | resource ID  | signature                                                    | min ver |
|--------------|--------------|--------------------------------------------------------------|---------|
| `sotesd.dll` | 0x7DE (2014) | `JFDGGIUABCVJIEKAUYLPOFDEQBVGSKOLJSCKPIFAXMHGYELSDOBFRKVGBAKB` | 0x2713 |
| `sotesw.dll` | 0x40F (1039) | `MUSICWMA`                                                   | 0x2712  |
| `sotesp.dll` | 0x407 (1031) | `FSPATCHR`                                                   | 0x2711  |

Our drop-in port can no-op these — they're integrity seals for the
ship-time DLLs, irrelevant when the user provides their own legit
copies.

**config.dat extractor (`tools/extract/config_dat.py` +
`docs/formats/config-dat.md`):** 16-byte plaintext header + 820-byte
XOR-obfuscated body (key `0x88`, confirmed by abundance of
`88 88 88 88` runs).  Body parses as one leading u32 + 102 `(u32,u32)`
pairs (`field_id`, `value`).  Field-ID semantics TBD but pattern is
clearly a typed key/value store matching the engine's `FUN_005afb90`
schema-registration with 101 fields.

**Harness turbo fixes (`tools/frida/opensummoners-agent.js`):**
GetTickCount virtualization (gated on first PeekMessage entry to
avoid pre-pump init livelock), WaitMessage stub (main-thread only),
Sleep → Sleep(0) (yield not noop, so background threads don't
starve), and PostMessage WM_ACTIVATEAPP(TRUE) to the main game
window as soon as the periodic scan finds it (without this the
pump spins on `DAT_008a952c == 0` forever because hidden windows
don't naturally receive the activation message).  Also corrected
the WndProc/class doc — `0x401210` is `CLASS_LIZSOFT_WAIT` (the
"Please wait." splash), the main game window is
`CLASS_LIZSOFT_SOTES` with WndProc `0x5b12e0`.

Engine quirks file grew from 8 entries to 14, with the most
load-bearing additions being §10 (WM_ACTIVATEAPP gating) and §13
(the three-DLL signature scheme).

Status: phase 1 surface mapping complete.  Phase 2 file-format
extraction started with config.dat and the resource walker.  Next
session is likely the Lizsoft sprite format spec + the chunked
sotesd 1000-1004 blob identification (needs DDraw Lock-hook capture
of a known sprite, then byte-diff against the extracted DATA bytes).

---

## 2026-05-24 — Harness turbo fixes + WndProc-class correction

Phase 1 surface mapping (the previous entry) flagged three TODOs that
this push addressed in `tools/frida/opensummoners-agent.js`:

1. **`GetTickCount` virtualization.**  Replaces `timeGetTime` as the
   simulation clock — Fortune Summoners never imports `timeGetTime`.
   The hook is gated by `g_pump_entered`: pre-pump init has busy-waits
   that livelock if the clock jumps 17 ms per call instead of advancing
   with real wall-clock.  After first `PeekMessageA` entry from main
   thread, the virtualization activates.
2. **`WaitMessage` stub** (main-thread only).  The pump uses
   `WaitMessage` to yield between frames; with virtual clock the OS
   timer never fires, so `WaitMessage` would hang.  Stub returns 1
   immediately on the main thread; background threads keep real OS
   semantics (audio mixer, file I/O may use `WaitMessage` for real
   waits).
3. **`Sleep` → `Sleep(0)`** instead of true no-op.  True-noop starves
   background threads of CPU, and the main thread often polls flags
   set by exactly such threads.  `Sleep(0)` yields the timeslice
   without actually sleeping — fast enough for turbo, correct enough
   for background work.

Discovered in the process: a hidden window never naturally gets
`WM_ACTIVATEAPP` from the OS, and `FUN_005b1030`'s spin loop only
breaks when `DAT_008a952c != 0` — which is set by the WndProc on
`WM_ACTIVATEAPP`.  Fix: agent posts `WM_ACTIVATEAPP(TRUE)` to the
main game window as soon as the periodic scan finds it
(`installPeriodicWindowScan`).  Without this, `msg_ticks` stayed at
0 forever with `--turbo --hide-window`; with it, pump enters and
the engine progresses into per-scene loops.

Also folded in a WndProc-class correction.  The Phase 1 notes claimed
the main game window's WndProc was `0x401210`; that's actually the
**`CLASS_LIZSOFT_WAIT`** ("Please wait." splash) WndProc.  The main
game window uses **`CLASS_LIZSOFT_SOTES`** with WndProc `0x5b12e0`
— a 441-byte handler that includes the load-bearing `WM_ACTIVATEAPP`
case plus `WM_CLOSE → ExitProcess(0)`.  Both classes are registered
in `FUN_005a4770`, sites `0x5a4ca8` and `0x5af314` respectively.
The `0x5b12e0` site does `mov dword [esp+0x50], 0x5b12e0` (lpfnWndProc
slot in WNDCLASSEXA at offset 8) — visible at `0x5af2c7`.

Quirks doc grew §9 (two WndProcs / two classes), §10 (WM_ACTIVATEAPP
as load-bearing pump-unlock), §11 (function-pointer-only callbacks
that Ghidra misses).  Engine-bootstrap doc updated to document both
WndProcs and the harness fix recipe.

Status: harness now reaches per-frame ticks in `--turbo --hide-window`
mode.  Steady-state frame rate still partial — `msg_ticks` reaches
~250 in some runs and 0 in others within a 30 s window, suggesting
init-phase race conditions remain (likely asset loading from
`sotesd.dll` / `sotesw.dll` — Phase 2 work).  Good enough to land as
a checkpoint; remaining bring-up TBD as the asset-loader RE goes.

---

## 2026-05-24 — Bootstrap (Phase 0)

Initial commit run.  Set up the project shape: nix flake with mingw-w64
i686 cross compiler + Ghidra + Frida-tools + Python (pillow/numpy/
sk-image/opencv/construct/rich/frida-python), `.editorconfig`,
`.gitignore`, MIT license, README.

`tools/setup.sh` — symlinks the user's Steam install of Fortune Summoners
into `vendor/original/`, detects Steam DRM by checking for a `.bind`
section in `sotes.exe`, runs Steamless via WSLInterop, and stashes the
unpacked binary in `vendor/unpacked/sotes.unpacked.exe`.  First run:
Steamless identified SteamStub Variant 2.1 and unpacked cleanly.
Original SHA: `7d779f2eb02b3c603857fedbc52be6973ac3b0b2c5c1bc696122ddac89fb9f1b`,
unpacked SHA: `9e032483b9981f73cabb83baca17a734fd9e7c41e114703900d9ee82c7969516`.

`tools/launcher/opensummoners-launcher.exe` — Job-Object supervisor copied
verbatim from OpenMare.  Guarantees no orphaned Windows-side .exes after a
SIGKILL'd WSL run.  Same `--timeout-ms` / `--grace-ms` / `--no-stdin-watch`
flags as the sibling.

`src/main.c` + `src/dev_hooks.c` — WinMain skeleton with the four
drop-in defaults the user asked for from day one:
  1. Auto-cd into `OPENSUMMONERS_GAME_DIR` + `SetDllDirectoryA` to the
     same, so any later `LoadLibrary` resolves game-dir DLLs first.
     `OPENSUMMONERS_GAME_DIR` is exported by the flake's shellHook with
     `WSLENV=…/p` so the .exe sees the Windows-form path.
  2. `user32!MessageBoxA/W` prologue patch that redirects every modal
     to stderr with a distinctive `[!!! REDIRECTED MESSAGEBOX !!!]`
     banner and auto-returns IDOK.  Override with `--show-msgbox`.
  3. Single-instance mutex (`OpenSummoners-SingleInstance`) catches
     stray .exes from previous SIGKILL'd runs.
  4. `--hide-window` / `--frames N` flags for harness/smoke runs.
Single-TU build (per the user's preference), two outputs:
`opensummoners.exe` (GUI subsystem) and `opensummoners-debug.exe`
(console subsystem, stderr surfaces in the launching shell).

`tools/ghidra-headless.sh` + `tools/ghidra-scripts/ExportDecompiledC.java`
— batch decompiles `vendor/unpacked/sotes.unpacked.exe` to
`docs/decompiled/` (gitignored).  Java post-script because nixpkgs'
Ghidra isn't built with PyGhidra.  First-run analysis kicked off in
background while we wrote the rest of Phase 0.

`tools/frida/opensummoners-agent.js` + `tools/frida_capture.py` — Phase A
Frida harness.  Hooks:
  - `MessageBoxA/W` → redirect to `send({kind:"messagebox",...})` +
    auto-IDOK (mirrors the dev_hooks.c hook on the drop-in side).
  - `ShowWindow` / `ShowWindowAsync` / `SetWindowPos(SWP_SHOWWINDOW)`
    → force hidden for HWNDs we tracked from CreateWindowEx returns.
  - `PeekMessage*` / `GetMessage*` onLeave → tick a coarse frame counter.
  - `Sleep` → no-op (turbo).
  - `winmm!timeGetTime` → virtualised clock for the main thread only
    (turbo simulation speedup, not just loop-iteration speedup).
  - `waveOutSetVolume` → clamp 0 (silent audio).  DSound layer deferred
    to Phase B once Ghidra confirms the engine's COM init path.
All flags default ON per the user's instruction ("hidden window with
muted audio running in turbo mode as early as possible").

`tools/run-opensummoners.sh` + `tools/run-retail.sh` — single-source-of-
truth dev-loop recipes so the build / run / launcher / harness flags are
consistent every time.  No re-discovering gotchas per session.

Smoke verification:
  - `run-opensummoners.sh` end-to-end: launcher → debug.exe
    `--hide-window --frames 200` runs in ~3.2 s (16 ms × 200), MessageBox
    hooks both succeed (`@ 745d6e60` / `@ 745d7380`), init_game_dir cd's
    into the Windows-form game path, exit rc=0.
  - Retail smoke under Frida: green.  The frida-server.exe runs on the
    Windows host as `cutestation.soy:27042` (the host's LAN-resolvable
    name; WSL2 NAT doesn't loop back to 127.0.0.1).  Updated the flake's
    default + `frida_capture.py` to match — 127.0.0.1 was the wrong
    default the OpenMare sibling carried forward.

Discoveries (folded into agent code and findings docs as we hit them):
  - **sotes.exe is SteamStub Variant 2.1 packed.**  Spawning the
    on-disk exe outside the Steam process tree trips the DRM check
    (`Steam Error: Application load error P:0000065432`).  Fix:
    `tools/run-retail.sh` copies vendor/unpacked/sotes.unpacked.exe into
    the game dir as `sotes-unpacked-<pid>.exe` per run (needed alongside
    the engine DLLs so Windows DLL search finds sotesp/d/w).
  - **Frida 17.x API surface differs.**  `Module.findExportByName(modName,
    exp)` static method removed → use
    `Process.findModuleByName(name).findExportByName(exp)`.
    `Memory.readUtf8String(ptr)` removed → use `ptr.readUtf8String()`.
    Hooks attached while the process is suspended sit deferred until
    `Interceptor.flush()` — without that, all our installs no-op'd
    silently.  Use `Process.mainModule` instead of name-matching since
    the spawned exe is named per the temp filename.
  - **The engine launcher is a Win32 #32770 modal dialog**, NOT a
    `MessageBox`.  Created by `DialogBoxParamA(hInst, 0x2711, NULL,
    dlgProc=0x004013c0, 0)`.  The dialog manager bypasses public
    `CreateWindowEx` / `ShowWindow` / `SetWindowPos` exports.  We
    initially caught it via a periodic `EnumWindows` scan + force-hide,
    but the OS painted it before our 8 ms scan tick — a brief flash
    appeared on the user's desktop.

Final fix (silent boot achieved 2026-05-24):
  `installDialogBypass()` in `tools/frida/opensummoners-agent.js`
  hooks `DialogBoxParamA` and replaces the engine's DLGPROC (arg 3)
  with a Frida `NativeCallback` wrapper.  On `WM_INITDIALOG`:
    1. Call original handler (loads saved settings into controls).
    2. Force-check Windowed Mode (ctrlId 10020) + Disable Sound
       (ctrlId 10024).
    3. `SendMessage(LaunchBtn, BM_CLICK)` synchronously — the engine's
       IDOK handler reads control state, persists, calls EndDialog.
    4. Return original result.
  Because `EndDialog` has been called before `WM_INITDIALOG` returns,
  the dialog manager skips its post-INITDIALOG ShowWindow step.  User
  confirmed "absolutely nothing" on screen.

Status of the harness:
  - Spawn retail headlessly under Frida → init agent → resume → engine
    boots silently through its launcher → reaches the main game window
    (`CLASS_LIZSOFT_SOTES`) within a few seconds → harness teardown
    via `device.kill(pid)`.
  - msg_ticks stays at 0 in the smoke summary — the engine reaches
    main window creation but doesn't enter its PeekMessage loop in 8 s.
    Probable additional bring-up phases (DirectDraw surface alloc,
    asset loader) gate the main loop; revisit when tracing the boot
    chain.

Ghidra batch decompile finished: 1768 functions written to
`docs/decompiled/` (gitignored).  First useful query already paid off
— `grep DialogBoxParam` immediately pointed at the dialog call site
and DLGPROC address.

Next session — Phase 1 priorities:
  1. Read DLGPROC at `0x004013c0` and its caller to understand
     `gl.cfg` (or wherever settings persist) layout.  This is the
     first thing the engine writes; spec it and we have an extractor.
  2. Find and document `WinMain` + the main loop + frame limiter
     (mirror OpenMare's `winmain-and-bootstrap.md`).
  3. Identify the DirectDraw 7 init path (`DirectDrawCreateEx` →
     `IDirectDraw7::SetCooperativeLevel` → primary surface alloc).

---

## 2026-05-24 — Phase 1 surface mapping (#1)

Three findings docs landed in one session, covering the three
priorities the prior entry queued up.  All entries cross-link, and
`engine-quirks.md` grew four new items folded in along the way.

`docs/findings/launcher-dialog.md` — full reverse of the launcher
DLGPROC at **`0x004013c0`** plus its sibling helper `FUN_00401730`.
Ghidra missed both because they're only reached via function
pointers; disassembled with `radare2 -c 'af; pdf'`.  The proc handles
just `WM_INITDIALOG` and `WM_COMMAND`; click on Launch (`ctrlID 10003`)
sets `DAT_008a9a40 = 1` and scrapes the four radio/checkbox groups
into `DAT_008a9b48/4a/4c/4e` (screen mode / VRAM / quality / disable
sound).  Engine quirk: **radio enums start at 3, not 0** — saved
file values are 3/4/5 per group.  Engine quirk: control `0x272A`
(Zoom 1920×1440) is unconditionally `ShowWindow(SW_HIDE)`'d at
`WM_INITDIALOG` — exists in the dialog resource but the user never
sees it.  Engine quirk: three controls (`0x271C-0x271E`) are
`EnableWindow(false)`'d on every init with no path to re-enable.

`vendor/original/user/config.dat` (840 bytes) is XOR-obfuscated with a
**16-byte plaintext header** (`hdr=16`, `ver=0x2711` matching the
dialog resource id, `data_size=820`, checksum) followed by 824
obfuscated bytes.  Key byte `0x88` — confirmed by the dead-obvious
runs of `88 88 88 88` (zero plaintext).  Format spec deferred to
Phase 2 `docs/formats/config-dat.md` once we wire the extractor.

`docs/findings/winmain-and-bootstrap.md` — full call graph from
`entry @ 0x5c0a8f` through `WinMain @ 0x562210` and the post-launch
driver `FUN_00562ea0`.  Mapped:
  - **WndProc @ 0x401210** (missed by Ghidra — pointer-only ref).
    Only handles `WM_PAINT` (loading-screen text + frame blit);
    everything else delegates to `DefWindowProcA`.  No `WM_CLOSE`
    handler — click-X just destroys the window without `WM_QUIT`,
    hanging the process.
  - **Message pump + frame limiter at `FUN_005b1030`**:
    `PeekMessageA` → if `WM_QUIT` (0x12) → `ExitProcess(0)`;
    `WaitMessage` to block on a `SetTimer(hWnd, 1, 10ms, NULL)`
    that's installed in `FUN_00562ea0`.  Frame-readiness flag at
    `state->[0x1c]` is set when `GetTickCount - last_tick < 5` ms.
  - **Class registration**: `RegisterClassExA` inside the 46 KB
    `FUN_005a4770` at `0x5a4ca8` — `CLASS_LIZSOFT_SOTES`, style
    `CS_HREDRAW|CS_VREDRAW`, WndProc `0x401210`, default arrow cursor.
  - **No global main loop** — each scene function runs its own
    pump+tick loop until it returns a state code to the outer scene
    state-machine in `FUN_00562ea0`.  Scene code = 9 means
    "restart game", caught by WinMain's `do…while`.

Critical insight for the Frida harness: **the engine uses
`GetTickCount` exclusively** — `iiq~timeGetTime` on the unpacked
binary returns nothing; the timeGetTime hook our agent inherited
from openrecet/OpenMare is a no-op here.  We need to add
`GetTickCount` virtualization + a `WaitMessage` stub to actually
achieve turbo speed.  TODO in the agent.

`docs/findings/ddraw-init.md` — DirectDraw 7 init flow:
`FUN_005b7ee0` (ZDD wrapper ctor)  →  `FUN_005b88c0`
(`DirectDrawCreateEx(NULL, &ddraw7, &IID_IDirectDraw7, NULL)` —
IID at `DAT_00850eb0`) → `FUN_005b89d0` (`SetCooperativeLevel`
with `DDSCL_EXCLUSIVE|FULLSCREEN|ALLOWREBOOT = 0x13` in fullscreen
or `DDSCL_NORMAL = 8` windowed) → `FUN_00582e90` (CreateScreen
mode dispatch — calls `FUN_005b8b40` which builds DDSURFACEDESC2
+ `IDirectDraw7::CreateSurface`) → `FUN_005b9520` (clipper create
+ attach to primary surface).  Catalogued the vtable offsets for
`IDirectDraw7` / `IDirectDrawSurface7` / `IDirectDrawClipper` so
the Phase-A `Lock`/`Flip`/`Blt` hooks land at the right offsets.

Two follow-ups recorded in the new docs for the next push:
  - **Decompile `FUN_005b95c0`** (the DDSURFACEDESC2 builder) when
    we move on to the renderer port — easier than chasing the
    46 KB `FUN_005a4770`.
  - **Add `GetTickCount` + `WaitMessage` hooks** to
    `tools/frida/opensummoners-agent.js` so turbo actually works.

Suggest `/clear` before the next subsystem (likely audio/DSound,
the asset loader, or the renderer port).  The Ghidra reads in this
session pulled in a lot of context that the next milestone won't
need.

---

## Checkpoint 13 — TAS framework: retail capture + deterministic input injection

Mirrored openrecet's TAS harness for OpenSummoners. Two new self-serviceable
Frida capabilities, both **validated live against retail**, plus the scenario
layout and the recovered new-game scene flow.

**Frame capture** (`tools/run-retail.sh --no-turbo --capture-frames "…"`):
at the Flip hook, walk the engine god object to the DDraw render-target
surface (`*(0x8a93cc)->[0x16c] paint_ctx ->[0x2c]`), GetDC + BitBlt into a
24bpp top-down DIB (GDI does the RGB565→BGR convert), read the bits, send to
the driver which writes lossless PNG (`runs/<dir>/frames/frame_NNNNN.png`).
Surface chain + vtable indices pinned by r2 disasm of the GetDC wrapper
`FUN_005b94e0` (`mov eax,[ecx+0x2c]; call [[eax]+0x44]`). First validation:
8 frames of the title boot (studio splash → logo → full menu), 640×480, colors
correct.

**Input injection** (`--input-trace <file.jsonl>`): a hidden window makes
DInput silent, so the engine's 64-slot event ring is ours to fill. Hook the
poll consumer `FUN_0043c110` (ecx = current scene's input mgr); per Flip frame,
write synthetic records `{id, ts, flag=1}` into the newest ring slots for the
ids a sparse `{frame, ids}` trace schedules. Validated: a single trace clicks
the entire **NEW GAME** path — title `Start` → difficulty config menu →
DOWN×2 → `Start Game` → confirm → Elemental Stone intro → prologue narration —
fully deterministic, captured frame-for-frame and confirmed visually.

Three findings landed (engine-quirks **#42/#43**; `input.md` corrected):
  - title nav button ids ≠ their latch-dir names: **up = id 1, down = id 3,
    confirm = id 0x24**; ids 2/4 are page up/down (no-ops in single-column
    menus). Diagnosed by hooking `menu_list_latch` (`0x43ce50`) live and
    seeing `ready=1000` yet no cursor move on id 2.
  - the injected record's `ts` must be the engine's **per-frame cached `now`**
    (the poll's first arg), not a fresh `GetTickCount()` — else `(now-ts)`
    underflows the 100 ms recency window and the press is silently dropped.
  - each scene has its **own input-manager instance**; inject into the current
    poll's `ecx`, never a cached one (the difficulty menu uses a different mgr
    than the title, and polls a different id set `0x22,1,3,0x24,0x27`).

New: `docs/plans/tas-framework.md`, `docs/findings/new-game-flow.md`,
`tests/scenarios/{title-idle,new-game-through}/` (openrecet layout).
Visuals pushed to llm-feed (title boot montage; new-game click-through montage).

> ⚠ Always launch retail via `tools/run-retail.sh` (unpacked exe), never
> `frida_capture.py` directly (the default `vendor/original/sotes.exe` is the
> Steam-DRM-packed image → stalls + orphan window).

Gaps for next time: prologue→first-playable-map needs a recorded human trace
(distil to sparse) or RE of the prologue sequencer; port-side
`input_trace.{c,h}` + port frame capture are latent (blocked on milestone-0
rendering). The harness is now the **yardstick** for the render-bridge port
(HANDOFF Next move #1): once the port renders, capture port frames the same way
and diff vs these retail goldens.

---

## 2026-06-02 (ckpt 21) — the title render sink: cmd stream → real ZDD blits

`src/title_sink.{c,h}` (+ `tests/test_title_sink.c`, 13 tests): the runtime
bridge that turns `title_render_step`'s abstract `TITLE_DRAW_*` command stream
(the render half of `FUN_0056aea0`, behind `title_render_sink_hook`) into the
retail render half's actual ZDD calls, against a bound primary surface. This
is the "sink" half of HANDOFF Next move #1 (the drive half is ckpt 22).

**RE landed this ckpt** (render half `0x56bb04..0x56bf1a`, r2): every per-phase
draw resolves its source frame out of ONE of two fixed sprite banks, then blits
onto `DAT_008a93cc->[0x16c]` (= the ZDD `primary_obj` at +0x16c):
- **MAIN bank** = `0x8a7658` = pool slot **19** (`ar_pool_get_slot(19)`; main
  pool base `0x8a7640` + 6·4). Carries the studio/title logos (frames 1/2),
  the press-button sprites (2/3/4), the sparkle (4/5), the menu background +
  menu sprite (5/6). The cmd `asset` is the `ar_sprite_slot_frame` frame id.
- **CURSOR bank** = `0x8a765c` = pool slot **20**. The menu-selection
  highlight; frame id = the selected row index.

Both self-decode via the ported `ar_sprite_slot_frame` chain (ckpt 16/20) —
but stay NULL until the slot is registered with a real "DATA" resource AND the
8d surface builder (`ar_frame_build_hook`) is wired, so every sprite op no-ops
faithfully (the "still-undecoded" path). ⇒ the drive will render a cleared +
flipped window with no sprites yet (move B), giving 8d a frame-diff harness.

**Faithful + host-tested:** `SURFACE_RESET` (→ `zdd_object_clear`),
`SURFACE_CLEAR`/`SPRITE` (keyed blit of `frame(main,asset)`→primary),
`SPRITE_LEVEL` (→ `title_draw_sprite_level`, ramp_b plain/alpha both proven),
`FRAME_END` (→ `title_compositor_draw` of the bound display-list group),
`FLIP` (present cb), `LOG_FLIPPING` (log cb) — the whole intro + menu-background
+ fade-out path.

**Deferred behind ctx callbacks** (no-op default): `LOGO`, `SPARKLE`,
`MENU_CURSOR` — the alpha-ramp draws whose blend-descriptor *pointer* rides the
32-bit `alpha` field of `title_draw_cmd` (can't round-trip on a 64-bit host) or
whose level numerator (`[esp+0x20]`) + fixed src geometry the command drops.
They only fire once the run-time ramp tables (`0x8a92b8`/`0x8a9308`) are
populated, which never happens at a cold headless boot, so deferring costs no
intro/menu-background fidelity. Wired + validated against live goldens in the
drive checkpoint.

**Fidelity fix to `title_render_step`:** `TITLE_DRAW_SURFACE_CLEAR` now carries
the source frame index in `asset` (prologue background = 0; the logo handler's
alpha-0 path = `frames[1]` studio / `frames[2]` title). The alpha-0 logo blit
is `frames[1/2]` (`0x56bba0`/`0x56bc19`), NOT the phase-2..3 background
`frames[0]` — the abstract op was previously lossy across the two.

**617 host tests (611 pass, 0 fail, 6 skip; +12 this ckpt).** Both 32-bit
cross-builds clean. Ledger unchanged at **130/1490 (8.1%), 127 tested** — the
sink bridges already-counted functions (no new `FUN_` port tokens).

---

## ckpt 25 — 8d ported in full (per-cell builder + format converters + slicer) + wired live (2026-06-02)

The genuine sprite pixel source is **ported, host-tested, and wired into the
live drive** in three commits:

**25  (`d82af11`) — 8d core in `src/zdd.c`:**
- `zdd_object_new_cell` (FUN_005b9280) — operator_new + ctor + orchestrate + publish.
- `zdd_object_build_cell` (FUN_005b9630) — trim-gate (count>1 tightens to the
  opaque bbox; found_key==0 drops the colorkey; found_opaque==0 → metrics-only,
  no surface) → `create_surface_pair` → pixel writer.
- `zdd_object_copy_cell_pixels` (FUN_005b9910) — **raw bottom-up byte blit**
  (NOT a format converter — see below); zero-fills the locked dest then row-copies
  the WxH window, clamping the per-row span to dest pitch + source stride.
- Verified against `docs/decompiled/by-address/5b9280.c`, `5b9630.c`, all.c 5b9910.
- 10 host tests. Ledger 131→134.

**25b (`16e3a18`) — format converters in `src/bitmap_session.c`:**
- `bs_convert_to_16bpp` (FUN_005b7310), `bs_convert_8bpp_to_24bpp` (_74f0),
  `bs_convert_24bpp_to_32bpp` (_7270), `bs_load_palette_from` (_7bd0).
- The slicer picks one by display depth ([zdd+0x168]); windowed = 16bpp, so a
  24bpp title sheet packs to RGB565 via the ZDD shift descriptor. 7 host tests.
  Ledger 134→138.

**25c (`03d33c1`) — slicer body + live wiring:**
- `ar_sprite_slice` expanded to the full FUN_004188b0: trim-scan loop (into
  `slot->aux_buf`) + format switch (new `ar_sheet_format_hook`) + trim-aware
  build loop. Verified vs `by-address/4188b0.c`.
- `main.c` adapters bind the three hooks to the live ZDD: `title_frame_build`
  → `zdd_object_new_cell`, `title_frame_free` → `zdd_obj_destroy`,
  `title_sheet_format` → `bs_convert_*` by `g_zdd->pixel_format_bpp`.

**Two corrected findings → engine-quirks #49 (format converters live in the
SLICER, not the pixel writer — the writer is a raw byte copy) and #50 (the
slicer passes (cell_w, cell_h) as the trim scanner's (height, width) args).**

**LIVE (self-serviced):** port boots windowed at **depth=16bpp**, drives the
title scene, flips, **zero DDERR, zero crashes through 900 frames**, clean
60fps. 8d is crash-clean against real DDraw.

**Gap to *visible* sprites (next session, NOT 8d):** the title banks (pool
19/20) are never registered at boot, so `ar_sprite_slot_frame` returns NULL
(entries==NULL) and the decode→slice→8d chain never fires.
`ar_register_main_sprites` exists but needs the **launcher settings record**
(the PE resource source) + sotesp module — a separate
asset-registration/launcher subsystem. Plus the sink's LOGO/SPARKLE/MENU_CURSOR
arms are still deferred no-ops and the cold-boot intro must reach the menu phase.

**647 host tests (0 fail, 6 skip; +17).** Both 32-bit cross-builds clean.
Ledger **138/1490 (8.6%), 135 tested (+7)**.

---

## ckpt 26 (2026-06-02) — THE TITLE SCREEN RENDERS (bank registration + frame capture)

**The payoff checkpoint.** Registering the title sprite banks at boot lit up the
whole ported render pipeline. The drop-in now decodes the real Fortune Summoners
title art from sotesd.dll and blits it to screen.

**The fix** (`src/main.c`, commit `e00718b`): `init_sprite_banks()` —
`LoadLibraryA("sotesd.dll")` + `ar_state_init()` +
`ar_register_main_sprites(g_zdd, /*group=*/4, hSotesd, hSotesd)`, called between
`init_ddraw` and `init_title_drive`. `FreeLibrary` on shutdown. The pool slots
19/20 the sink reads (`AR_SPR_TITLE_MAIN`/`_CURSOR` → `g_ar_sprite_slots[6]`/`[7]`
= ids 0x91b/0x91c) are exactly the ones `ar_register_main_sprites` populates, so
the getter stops short-circuiting to NULL and the decode→slice→8d→blit chain runs.

**RE findings that unblocked it (engine-quirks #51):**
- `bs_decode_resource`/FUN_005b7800 takes the HMODULE **directly** — no `+0x3c`
  record indirection. `settings` is literally an HMODULE.
- That HMODULE is **sotesd.dll** = `DAT_008a6e74` (stored @ 0x5af5fc right after
  its `LoadLibraryA`). The asset-loader doc mapped the three DLL handles to the
  wrong DAT_ slots (said sotesp.dll) — corrected.
- Every title resource (logo 0x49f, bg banks 0x91b/0x91c, slot-0 palette seed
  0x90b) lives in sotesd.dll's `DATA` type; none in sotesp.dll. Slot 0's
  `sotesp_module` arg is the SAME sotesd handle (misnamed param).

**Verified 1:1 against retail goldens** via new port-side frame capture
(`--capture-frames "60,200,…"` → `port_frame_NNNNN.bmp` of the composed primary;
BMP→PNG→read-image). Port frame 60 = full title art + "FORTUNE SUMMONERS" logo;
port frame 200 = the title menu (Start/Continue/Bonus Menu/Options/Exit +
"Secret of the Elemental Stone" + copyright), **pixel-identical to retail
`runs/title-idle/frame_01900.png`**. Frame capture committed `dd4ef08` (roadmap
task #10 core).

**Three remaining fidelity gaps (NEXT layer, not rendering bugs):**
1. Intro pacing — port reaches the menu by Flip ~200; retail still on the
   Lizsoft splash there (~1900 to menu). The pace pump `0x5b1030` is a stubbed
   hook, so phases aren't time-gated. Port it to match retail's timeline.
2. Menu cursor highlight absent — the CURSOR bank (pool 20) is registered + live
   but the `MENU_CURSOR` sink arm is a deferred no-op. Wire it.
3. Lizsoft studio splash not drawn — `LOGO`/`SPARKLE` arms deferred (need the
   runtime alpha ramps `0x8a92b8`/`0x8a9308` populated).

**647 host tests (0 fail, 6 skip).** Ledger **138/1490 (8.6%)** unchanged —
wiring, not a new function port. Added `SINK_RESOLVE_DEBUG` compile-gated probe.

---
