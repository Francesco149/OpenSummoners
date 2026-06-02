# Session handoff — last updated 2026-06-02 (title menu BIT-EXACT, ckpt 28)

> **ckpt 28 — R1 CLOSED: title menu is bit-exact (`differ_px==0`), cursor pulse
> RE'd.** The 955px cursor residual was the **cursor pulse**. Retail animates
> the cursor `level_num` (`FUN_0056c470` arg3 = `[esp+0x20]`) as a triangle wave
> — it's `local_58` in `FUN_0056aea0`, driven by the phase FSM (phase 8:
> `+50`/update to 1000; phase 9: `-50`/update to 0; `56aea0.c`:366-384). With
> the fixed `level_div=0x4b0` (1200), `idx=(local_58*20)/1200` peaks at **16**
> (NOT 19) and breathes to 0 (invisible). The port had pinned the cursor to a
> static idx-19 full-add → uniformly over-bright. The port *already* computed
> the value as `title_fade_state.menu_fade`; ckpt 28 threads it into the cursor
> draw. **Method (RE, not eyeballing — per user):** `frida_capture.py
> --cursor-probe` (new) hooks `0x56c470` and logged the 0→1000→0 step-50
> triangle; read `FUN_0056aea0`'s FSM to find `local_58`; wired it; validated
> `differ_px==0` by matching port frames (now log `menu_fade`) to retail goldens
> captured WITH `--cursor-probe` (known `local_58`) at equal pulse phase: port
> Flip 209 (mf=750) vs retail 1300 (l58=750) → 0px; port 203 (450) vs 1420/1460.
> **User-confirmed fully 1:1 bit-identical.** Commits `<this ckpt>` (tools +
> render fix). 648 host tests pass. Also fixed the Frida harness default exe
> (was spawning the packed DRM exe → 0 frames; now the co-located unpacked PE —
> engine-quirks #53). Ledger unchanged 138/1490 (wiring, no new FUN).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 26 — THE PAYOFF: the port renders the real title screen.** Registering
> the title sprite banks at boot lit up the entire ported pipeline. The drop-in
> now decodes the actual Fortune Summoners title art from sotesd.dll and blits
> it: **full character art + background + "FORTUNE SUMMONERS" logo, then the
> title menu** (Start / Continue / Bonus Menu / Options / Exit + "Secret of the
> Elemental Stone" + copyright). The art/logo/menu-text region matched the
> retail golden (port frame 200 vs `runs/title-idle/frame_01900.png`) with the
> only diff being the (then-unwired) selection cursor — NOT a full bit-exact
> match (see ckpt 27 + parity-ledger R1). Self-verified via the new **port-side
> frame capture** (`--capture-frames`, BMP→PNG→read-image).
>
> The fix (commit `e00718b`): `init_sprite_banks()` in `main.c` —
> `LoadLibraryA("sotesd.dll")` + `ar_state_init()` +
> `ar_register_main_sprites(g_zdd, 4, hSotesd, hSotesd)`, called between
> `init_ddraw` and `init_title_drive`. **Key RE finding (engine-quirks #51):**
> `settings` is a *bare HMODULE* (no +0x3c record), and it is **sotesd.dll** =
> `DAT_008a6e74` (stored @ 0x5af5fc) — the asset-loader doc had it as sotesp.dll
> and was WRONG. All title resources live in sotesd.dll. Bank→slot map:
> `ar_pool_get_slot(19)`=`g_ar_sprite_slots[6]`=id 0x91b=MAIN;
> pool 20=slots[7]=0x91c=CURSOR — both populated by `ar_register_main_sprites`.
>
> 647 host tests pass (0 fail, 6 skip). Ledger **138/1490 (8.6%)** unchanged
> (wiring, not a new port). Frame capture (commit `dd4ef08`) is roadmap task #10.

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint. `docs/PROGRESS.md` is the
append-only changelog; this file is "where to pick up *right now*".

## ⭐ Current state (ckpt 28): title menu is BIT-EXACT; intro pacing + LOGO/SPARKLE remain

The whole chain runs live, every frame, producing correct pixels:

```
title_scene_step → title_sink → resolve_frame(bank 19/20)
   → ar_sprite_slot_frame(slot, id)
        → ar_sprite_decode (0x4184a0)  [NOW FIRES — banks registered]
             → bs_decode_resource(sotesd.dll, 0x91b, "DATA", compressed)
             → ar_sprite_slice (0x4188b0): trim + format switch + build
                  → ar_sheet_format_hook → bs_convert_to_16bpp (RGB565)
                  → ar_frame_build_hook → zdd_object_new_cell (8d) → real surface
        → title_draw_sprite → keyed blit onto primary → present → VISIBLE
```

**Verified BIT-EXACT** (`differ_px==0`, parity-ledger #1) against retail goldens
for the title art + menu layout + the selected-row cursor brightness (the cursor
pulse was R1, closed ckpt 28). The two remaining divergences are the **next
layer** (NOT rendering bugs — the pipeline is correct):

1. **Intro pacing (R3)** — the port reaches the menu by Flip ~200; retail is
   still on the Lizsoft studio splash there (retail ~1300+ to menu). The port
   **rushes/skips the intro fade**: the pace pump `0x5b1030` (and the pre/post
   side-effect hooks) are still stubbed in `title_scene_hooks`, so phases tick
   per-frame instead of being time-gated. To 1:1-match retail's timeline
   frame-for-frame these must apply the real per-phase delays/frame-counters.
   (NB the *cursor pulse* now matches at equal phase regardless; this is about
   the port Flip index == retail Flip index.)
2. **`LOGO` / `SPARKLE` arms not drawn** — the Lizsoft studio splash (`LOGO`)
   and the menu sparkle (`SPARKLE`) sink arms are still deferred no-ops. The
   alpha ramps `0x8a92b8`/`0x8a9308` are now live (ckpt 27), so both are
   wirable; each carries a blend-descriptor pointer per command. Wire like
   `MENU_CURSOR` was: resolve the bank frame + draw via the ramp.

## Next move (pick one — recommendation first)

> Context: rendering is bit-exact for the static menu. The active goal (user,
> ckpt 13) is **1:1 parity with retail** for title + new-game + prologue.

1. **(recommended) Intro pacing fidelity (R3)** — make the port's phase timeline
   match retail so a given Flip frame shows the same content as the golden at
   that index (right now port Flip N ≠ retail Flip N — only the *content*
   matches at equal phase). Port the pace pump `0x5b1030` + the pre/post
   side-effect hooks (`0x43e140`/`0x40fe00`/`0x566250` pre, `0x56c930` post)
   stubbed in `title_scene_hooks`, OR find the frame-counter/timer that gates
   each fade phase. This is what makes end-to-end frame-for-frame golden diffing
   meaningful and is the gate the user set for the rest. The `--cursor-probe`
   pattern generalises: add similar arg-logging hooks to measure the per-phase
   timers on retail.
2. **Wire `LOGO` + `SPARKLE`** — the remaining title-screen fidelity; the alpha
   ramps are live. Verify with `--capture-frames` against the goldens
   (self-serviceable, BMP→PNG→read; match pulse/fade phase like R1).
3. **Live-validate `--input-trace`** (ckpt 24, still unverified) + drive the
   menu nav so the CURSOR highlight moves — pairs naturally with #1. Does an
   injected DOWN actually move the selection on the port? Frida self-serviceable,
   no-turbo. Then extend toward the new-game menus (the user gates trace
   extension on "once we have prologue and main menu rendering").

## Tooling added this ckpt (28)

- **`frida_capture.py --cursor-probe`** — hooks retail `FUN_0056c470` (menu
  cursor draw), logs per-Flip `level_num`/`level_div` → `<run>/cursor_level.jsonl`
  + a distinct-value summary. The pattern (read 8 stack slots, find the known
  `0x4b0` div to anchor the arg layout, tag by `g_flip_frame`) **generalises to
  any FUN_ whose args you want to measure live** — copy `installCursorProbe` in
  the agent + the message handler in `frida_capture.py`. Use `--no-turbo`.
- **Port capture now logs pulse state** (`src/main.c`): each `--capture-frames`
  line prints `phase=… fade=… menu_fade=…` so a port frame can be matched to a
  retail golden at the same cursor-pulse phase (capture goldens WITH
  `--cursor-probe` to know their `local_58`, then diff at equal value → 0 px).
- **Harness default exe fixed** — `frida_capture.py` now spawns
  `vendor/original/sotes.unpacked.exe` (the unpacked PE co-located in the game
  dir), NOT the packed `sotes.exe` (which stalls at 0 frames). engine-quirks #53.
- (still here) `--capture-frames`, `SINK_RESOLVE_DEBUG`, `push_comparison.py`.
- **`docs/parity-ledger.md`** — entry **#1 is now CONFIRMED bit-exact** (title
  menu, phase-matched, `differ_px==0`). Re-diff + update after render changes.

## Module inventory (13 modules) — render pipeline COMPLETE

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper, cs_dispatch,
app_pump, title_scene (`FUN_0056aea0`, fully ported+wired+driven), input
(`FUN_0043c110`), obj_container, menu_list, **title_render** (compositor +
wrappers), **title_sink** (cmd→ZDD bridge, banks 19/20), **title_drive** (caller
side of the runner). **8d** (`zdd_object_new_cell/_build_cell/_copy_cell_pixels`
+ `bs_convert_*` + slicer) ported ckpt 25, **now firing live** (banks registered
ckpt 26). `main.c` drives the scene against the live ZDD with the 8d hooks +
`init_sprite_banks` wired. `--no-title-scene` restores the legacy present loop.

## How to run / verify live (self-serviceable — [[reference_frida]])

```
# build (single-TU, full rebuild) inside nix develop:
make -C src all && make -C tests run        # 647 pass / 0 fail / 6 skip

# capture title frames (writes BMPs into the game dir = Windows C: drive):
cp build/opensummoners-debug.exe /tmp/oss.exe
./build/opensummoners-launcher.exe --timeout-ms 35000 -- /tmp/oss.exe \
    --hide-window --frames 2200 --capture-frames "60,200,400,700"
# then BMP->PNG (PIL) from /mnt/c/.../Fortune Summoners/port_frame_*.bmp and read it
```

NB Flip frames advance ~1 per 2 main-loop iterations (pace split), so reaching
Flip 700 needs a generous `--frames`/timeout. `run-opensummoners.sh` rebuilds
the debug exe with default flags — use the launcher directly if you need a
`-DSINK_RESOLVE_DEBUG` build to survive.

## Active goal (unchanged, user @ ckpt 13)

**Make the PORT render the scenes the new-game trace covers — title menu +
new-game menus + prologue (stone/narration) — to 1:1-match retail, using the
harness goldens as the pixel target. Do NOT extend the trace toward in-game yet;
"once we have prologue and main menu rendering we extend the trace."**

Title menu is now **bit-exact** (parity-ledger #1, R1 closed ckpt 28 — the
cursor pulse). Remaining for full title parity: intro pacing (R3, frame-for-
frame index alignment) + the `LOGO`/`SPARKLE` arms. Then drive the new-game
menus (the `--input-trace` path) and confirm they render, then the prologue.

## Open RE threads (see ROADMAP subsystem map for the rest)

- **Deferred sink arms** `LOGO`/`SPARKLE` (`title_sink.c`) — the remaining
  title-screen fidelity. `MENU_CURSOR` is now wired + bit-exact (ckpt 28). The
  alpha ramps `0x8a92b8`/`0x8a9308` are live, so LOGO/SPARKLE are wirable now.
- **Outer-loop side-effect hooks** (stubbed in `title_scene_hooks`): `0x5b1030`
  (pace pump — the intro-pacing key), `0x43e140`/`0x40fe00`/`0x566250` (pre),
  `0x56c930` (post), `0x43c2e0` (per-entry). Porting the pump is what fixes
  intro-pacing fidelity (Next move #2).
- **Other register batches** not yet called at boot: `ar_register_fonts`,
  `ar_register_palette_ramps` (FUN_0057a330), the big `FUN_0056e190` (442
  sprites), sounds. The title path doesn't need them, but the new-game/prologue
  scenes will — register them the same way (all take the sotesd HMODULE).
- **`0x40fa00`** cell text-layout / glyph builder (SJIS, `#`-colour escapes) —
  the menu text currently renders (pre-baked into the menu-bg sprite?), but
  dynamic text will need this.
- **SFX `0x411390`** / joystick `0x5ba120/_290` / save-notify `0x41bb80` /
  watchdog `0x40a5d0` — the four `title_menu_input_step` side effects.
- **Audio ZDM** `FUN_005bab10`/`_5bc150` + SFX `FUN_00411390` — milestone 3.
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4. We now know it
  loads sotesd/w/p.dll and stores their handles at DAT_008a6e74/78/7c.
- **Input producer** (DInput `GetDeviceState`, vtable `[0x24]`) + axis-held
  flags (quirk #41) — black box; `mem_watch.py` is the tool.
- God-object `DAT_008a9b50`/`DAT_008a6e80` layout (quirk #15) — model as we go.

## How to apply

When the user says "continue RE work" (or similar):
1. Read this file first, then `STATUS.md` + `ROADMAP.md`.
2. Pick the recommended next move (or whichever the user redirects to).
3. Port-and-test style: small unit → host test → commit. Each ported function
   gets a `FUN_XXXXXX` provenance comment; pin retail offsets via
   `_Static_assert` under `#if UINTPTR_MAX == 0xFFFFFFFFu`. Reference UNPORTED
   callees by bare VA, never `FUN_` (it inflates the ledger).
4. **Append any engine quirk** to `findings/engine-quirks.md` (now at #51).
5. **Regen** `gen_port_ledger.py` + `gen_frontier.py` after a port; check the
   headline didn't move unexpectedly.
6. **Verify rendering with `--capture-frames`** vs the goldens — self-serviceable.
7. Update THIS file each meaningful checkpoint; append to PROGRESS.md.
8. Suggest a `/clear` at the natural stop point.
