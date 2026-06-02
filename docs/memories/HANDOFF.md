# Session handoff — last updated 2026-06-02 (post-title dispatch backbone, ckpt 33)

> **ckpt 33 — POST-TITLE DISPATCH BACKBONE: the title menu is re-enterable;
> Exit exits.** Until now the port hard-shut-down on *any* `TITLE_SCENE_DONE`,
> so committing a menu row just quit. Ported the result→action mapping of
> retail's boot-driver outer loop `FUN_00562ea0` (562ea0.c:684-734) as the pure,
> Win32-free `app_flow_dispatch` (**`src/app_flow.{c,h}`**): `6/8→EXIT`,
> `9→EXIT_9`, `0x1a→NEW_GAME`, `0x1b→DEMO_START`, `0x1c→CONTINUE`,
> `0x1d→OPTIONS`, `0x1e→BONUS`, `0`/default→`REENTER_TITLE`. Wired into
> `main.c`: Exit sets `g_shutdown`; every (still-UNPORTED) sub-scene arm logs +
> calls the new **`reenter_title()`** which tears down the finished drive and
> rebuilds it (`build_title_drive(skip_intro=1)`), so the menu loops like
> retail's. **Verified live**: a trace to **Exit** → `result=8` → clean exit (no
> re-enter line); a trace confirming **Start** → `result=26` → stub log →
> drive rebuilds → menu reappears. Captures confirm the re-entered title
> **replays the intro from phase 0** (quirk **#60**: the `local_164`/`param_1`
> re-display arg does NOT skip the intro — it only enables a phase-0 skip-press;
> 56aea0.c:177/:182). 668 host tests pass (+1 `app_flow_dispatch_codes`).
> Ledger **145/1490 (8.9%)** unchanged (`0x562ea0` was already counted; this is
> a partial port of its tail → status `tested`).
>
> **The new-game/continue/options/bonus sub-scenes are stubs (re-enter title).**
> They are gated on the next big rock: the **glyph/text pipeline** (`0x40fa00`
> SJIS + `#`-colour escapes, `0x40f800`) + **font/sprite-batch registration**
> (`ar_register_fonts`, `0x57a330` palette ramps, `0x56e190` 442-sprite batch).
> That pipeline is the shared prerequisite for every menu AND the prologue
> narration — see **Next move**.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 32 — THE TITLE MENU IS INTERACTIVE (milestone 1): injected nav moves
> the cursor + commits.** Live-validated the `--input-trace` path and found the
> menu was **dead to input** despite rendering bit-exact: `menu_list_latch`
> gates all nav on `sub->ready==1000` (quirk #34), where `sub->ready` is the
> spawned node's `+0x54` ramp — `menu_node_build` zeroes it, so the gate starts
> **closed**. The driver that opens it is the title scene's **post-update**
> side effect **`FUN_0056c930`** (was stubbed NULL), **NOT** the per-entry
> update `0x43c2e0` (which only *reads* `+0x54`). `0x56c930`'s **mode-1** arm
> ramps the active node's `+0x54` **+50/frame to 1000** (node built mode 1,
> `+0x50`=1) → menu navigable **~20 update frames after spawn**. Ported the
> mode-1 arm as **`menu_owner_transition_step`** (`src/menu_list.c`; modes 0/2
> are submenu-slide paths the title never uses — deferred + documented), wired
> as the drive's `post_update` (`src/main.c` `drive_post_update`). Quirk **#59**.
>
> **Verified live** (new `--menu-trace` cursor-row diagnostic in
> `src/title_sink.c`): DOWN×4 walks the cursor `0→1→2→3→4`, UP walks back, and
> confirm (`0x24`) on row N returns that row's action id — `result=26` (`0x1a`
> Start) on row 0, `result=8` (Exit) on row 4. The ► arrow + row-highlight
> visibly track the selection (port `Start`-vs-`Options` capture pushed to
> llm-feed). 667 host tests pass (+7 ramp tests); ledger **145/1490 (8.9%)**
> (+1: `0x56c930`). NB the input gate (`+0x54`, +50/frame, open ~flip 547) opens
> *before* the cursor becomes visible (`fade==1000`, +20/frame, ~flip 577), so a
> press lands before the highlight appears — time demos after ~flip 577.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 31 — THE TITLE INTRO IS FULLY BIT-EXACT: phase-7 sparkle particles
> ported + `differ_px=0` (user-confirmed 1:1 incl. particles).** Closed
> parity-ledger R4, the last intro-content gap. The `FUN_0056c070` particle
> twinkles are **four sites over one cap-500 pool** (quirk #58): spawn
> `0x56c070`, **per-frame update `0x56ba69`** (rise/age), cull `0x56c030`,
> draw `0x56c180`. The twinkles **spawn at the reveal edge, rise upward
> (accelerating via the `+0x08` velocity field), animate frames 0→7 as they
> age, then cull at lifetime end — they "evaporate upwards", they do NOT
> accumulate.**
>
> The first cut spawned+drew but **froze** them (omitted the update) → an
> over-bright piled-up smear (8277 px diff). Finding + porting the update
> (`y_num -= vel; vel += 2; anim_num-- else cull`, inline at `0x56ba69` +
> swap-remove `0x56c030`) fixed it. **Determinism:** the engine LCG seed
> `DAT_008a4f94` is `srand(time())` (`0x56227a`), so retail's twinkles are
> wall-clock-random; the port pins a fixed seed (`OSS_RNG_DEFAULT_SEED
> 0x4f5347`) and the harness `--seed-pin` (default ON) writes the same value
> into retail at the first spawn. **Verified `differ_px=0`** at a matching
> update-tick: port Flip 465 vs `sparkle-align/frame_00939` (parity-ledger #4,
> user-confirmed). Off-tick frames differ only by the R3 render-rate sub-tick
> jitter (retail renders each update-state ~2.2×) + run-to-run intro-pacing
> jitter (first spawn flip 886/895/896) — align by the new `subtitle_anim_start`
> TAS anchor + tick, never a fixed flip. 660 host tests pass (+10); ledger
> 144/1490 (8.8%, +6 real ports: LCG/srand + spawn/update/cull).
>
> New port modules: `src/rng.{c,h}` (the MSVC LCG), `src/title_particles.{c,h}`
> (pool + spawn + update + cull). New harness: `--seed-pin`/`--seed-value` +
> the `subtitle_anim_start` anchor (`installSparkleAnchor`).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 30 — title intro CONTENT parity: both logos BIT-EXACT, sparkle sweep
> bit-exact.** Wired the last two deferred render-half arms (the ckpt-29
> recommended move). RE collapsed both into already-validated paths:
> - **LOGO**: the quirk-#40 "+4/+8 container fields" are just MAIN-bank
>   `frames[1]` (studio) / `frames[2]` (title) — `*(*slot)` is the frames array
>   `0x418470` indexes. The logo handler (`0x56bb5c`/`0x56bbd4`, alpha leaf
>   `0x494e10`) is **bit-identical to the sprite-level wrapper** `0x56c4e0`
>   (same `ramp_b`, same fade<=0/idx>=20/empty→plain-keyed rules; the only
>   `0x5bd550` a10-global difference is pixel-irrelevant). So `title_render_logo`
>   now emits one `TITLE_DRAW_SPRITE_LEVEL` (frame 1/2, raw fade). **Fixed a real
>   bug**: the old branch keyed on the scene `ramp`/`fade_ramp` param, never
>   populated by `main.c` → logos rendered **opaque, unfaded**. Now they fade via
>   the sink's `ramp_b`. Quirk **#56**.
> - **SPARKLE**: `0x56bcf7` copies 4×48 slivers of the menu-bg sprite (MAIN frame
>   5) src `(x,416)`→dst `(x,416)`, revealing the "Secret of the Elemental Stone"
>   subtitle column-by-column. Cmd now carries the raw clamped level + column
>   (the sink indexes `ramp_b` + calls `title_draw_sparkle`). Quirk **#57**.
>
> **Verified (R1 fade-matched method).** New `frida_capture.py --fade-probe`
> (hooks `0x448c80`, logs the per-flip logo fade in phases 0–4). **Studio logo
> phase 0 fade 640 → `differ_px=0`; title logo phase 3 fade 820 → `differ_px=0`**
> (parity-ledger #2/#3, **user-confirmed 1:1**). Sparkle full reveal (fade 1000):
> logo + subtitle match exactly; only residual = retail's **additive particle
> twinkles** from the separate, still-deferred `FUN_0056c070` spawn (parity-ledger
> R4 — a **noted gap**, user-acknowledged, not a sweep bug). 650 host tests pass
> (+2 sink sparkle tests); ledger 138/1490 unchanged (wiring). Fade-probe caveat:
> phase 7 logs the first *sparkle* level, not the raw fade — match by reveal
> extent there.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 29 — R3 (intro pacing) diagnosed + fixed; hidden-window flicker fixed.**
> The "port rushes the intro" framing was **wrong**. Measured both sides with
> the real clock (new `frida_capture.py --pace-probe` + a `pace:` phase log in
> `src/main.c`): **wall-clock to the menu already matched retail (~9.2 s)**.
> The gap was render-rate: retail renders ~127 flips/s, duplicating each scene
> state ~2.2× (cursor probe: each `menu_fade` value spans ~2 flips); the port's
> fixed-timestep accumulator (`title_pace_step`) was driven **one pace-step per
> 16 ms-throttled main-loop iteration**, so `now` advanced per *update*, the
> budget refill ran away to ~6 updates/render, and the port **DROPPED ~5/6 of
> the intro's fade frames** (rendered 90 of ~528 update ticks; choppy).
> **Fix (`src/main.c` `main_loop_body`):** spin pace-steps (updates ~free) until
> one *present*, then `frame_limiter` gates the presented-frame rate — like
> retail's tight outer loop. Now 1 update/render: phase curve = canonical
> 51/102/153/254/275/316/437/**528**, every fade value rendered, wall-clock
> unchanged. R1 re-verified post-fix at `menu_fade=750` → **differ_px=0**.
> Flip-index-exact match to a golden = the capture rig's refresh (~127 Hz),
> **not portably reproducible**; the distinct-content sequence is, and now
> matches. Quirk **#54**; parity-ledger R3 resolved.
>
> Also fixed a **hidden-window screen flicker** the user reported: the port's
> mode-2 present BitBlts the *desktop* (`GetDC(NULL)`) every frame regardless of
> window visibility — now skipped under `--hide-window` (captures read
> `primary_obj` first, so lossless). Live diag showed **retail paints its
> *window* (`GetDC(hwnd)`), not the desktop → retail doesn't flicker; only the
> port did.** Quirk **#55**; likely port mismodel (desktop vs window present
> target) → follow-up task to confirm via disasm + correct `zdd_present` case 2.
> Commits `f886d10` (pace) + `5ba8f37` (flicker). 648 host tests pass; ledger
> 138/1490 unchanged (driving fixes + instrumentation, no new FUN).
>
> ─────────────────────────────────────────────────────────────────────────────
>
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

## ⭐ Current state (ckpt 33): intro bit-exact, menu INTERACTIVE, and the title is now RE-ENTERABLE (Exit exits, commits dispatch)

The title is feature-complete as a *loop*: intro bit-exact, menu interactive
(ckpt 32), and now the menu-commit return code is **dispatched** like retail's
boot driver instead of force-quitting (ckpt 33, `src/app_flow.c` +
`reenter_title`/`build_title_drive` in `main.c`). **Exit (8) → clean shutdown;
every other commit → the sub-scene (all UNPORTED stubs for now) → re-display the
title** (which replays the intro from phase 0, faithful — quirk #60). The
sub-scenes themselves do not render yet; that's the next rock (glyph/text
pipeline — see Next move).

The whole intro chain is bit-exact, AND the menu responds to input: injected
`--input-trace` nav moves the cursor (both directions) and confirm returns the
selected row's action code (ckpt 32: `FUN_0056c930`'s mode-1 `+0x54` ramp,
`menu_owner_transition_step`, wired as `post_update`, quirk #59). Use
`--menu-trace` to log cursor-row changes. **The intro render chain:**

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

**Verified BIT-EXACT** (`differ_px==0`) against retail goldens for: the title
menu + selected-row cursor (R1, parity-ledger #1, ckpt 28); the **studio logo**
(phase 0, fade 640, #2) and **title-art logo** (phase 3, fade 820, #3) — both
ckpt 30, **user-confirmed 1:1**; the **phase-7 subtitle-reveal sweep** at full
reveal; and (ckpt 31) the **phase-7 particle twinkles** (#4, port Flip 465 vs
`sparkle-align/frame_00939`, seed-pinned `0x4f5347`, **user-confirmed 1:1 incl.
particles**). **Intro pacing (R3) is correct** (ckpt 29): every fade tick
renders at ~60 Hz, wall-clock to menu matches retail (~9.2 s).

**parity-ledger R4 is CLOSED (ckpt 31).** The intro has **no known content
divergence left**. The only non-bit-exact thing about the intro is the
**flip-INDEX** at which a given state renders (R3 render-rate ~2.2× + run-to-run
intro-pacing jitter) — which is **not portably reproducible by design** and not
chased; the distinct-content sequence + every random-particle frame match at
tick alignment. Seed pinning (`OSS_RNG_DEFAULT_SEED 0x4f5347` port-side,
`--seed-pin` harness-side, default ON) is what makes the RNG-driven twinkles
comparable; align frames by the `subtitle_anim_start` anchor + update-tick.

## R3 is resolved — what "pacing" did and didn't mean (read before re-opening)

The port's *wall-clock* pacing already matched retail; the Flip-INDEX gap is a
render-rate artifact (retail ~127 flips/s duplicating each state ~2.2×; the port
now renders each state once at ~60 Hz). **Flip-index-exact** match to a specific
golden = the capture rig's refresh and is **not portably reproducible** — do not
chase it. The achievable, meaningful target is the *distinct-content sequence*,
which now matches (every fade value rendered, no drops). The pace pump `0x5b1030`
and the pre/post side-effect hooks stay stubbed **on purpose** — they are NOT the
pacing key (that was the driving cadence, fixed in `main_loop_body`); they matter
only for the BGM cue / per-entry updates, port them when those subsystems land.

## Next move (pick one — recommendation first)

> Context: the title is a complete loop now — intro bit-exact, menu interactive,
> commits dispatched + re-enterable (ckpt 33). The active goal (user, ckpt 13)
> is **1:1 parity with retail** for title + new-game + prologue. The front is
> the **new-game config submenu** (confirm "Start" → it should render) →
> prologue. The dispatch backbone is in (`app_flow_dispatch`); the `NEW_GAME`
> arm is a stub. What gates rendering it is the **glyph/text pipeline**.

1. **(recommended) Port the glyph/text pipeline + font registration — the
   shared gate for every menu AND the prologue narration.** The new-game config
   menu (`FUN_00564780` **case 0x24**: builds "Game Difficulty / Auto-guard /
   Start Game" via `FUN_00411940` owner + `FUN_00412160` entries +
   `FUN_00566570` value-row text, then a shared run loop) cannot render text
   until **`0x40fa00`** (the 800 B cell glyph/text-layout builder: SJIS,
   `#`-colour escapes; `menu_list.c` already routes its call site through a hook
   awaiting this) + **`0x40f800`** land. It also needs banks registered at boot
   the way `init_sprite_banks` does it: `ar_register_fonts`,
   `ar_register_palette_ramps` (`0x57a330`), the big `FUN_0056e190` (442
   sprites), sounds — all take the sotesd HMODULE. **Verify glyph output against
   retail before wiring any scene** (render a known string, diff). This is a
   meaty standalone unit; do it first, THEN the config-scene controller.
2. **Then: the new-game config scene runner.** Once text renders, port
   `FUN_00564780` case 0x24 + the shared run loop as a new scene/drive (mirror
   `title_drive`), and route the `app_flow` `NEW_GAME` arm to it instead of the
   stub. The transition `FUN_00564160` plays first; the **Elemental-Stone intro
   anim** is `FUN_0056cd20` (a timed particle/gem cutscene, NOT a menu). Then
   `FUN_0059ec30` starts the game proper. Reference trace:
   `tests/scenarios/new-game-through/trace.jsonl` (retail Flip axis — re-time for
   the port). Difficulty menu = a **mode-2** controller polling `0x22,1,3,0x24,
   0x27`; `id 0x27` = value left/right, **unverified directionally**
   (new-game-flow.md "Open"). The dispatch already keeps the same `--input-trace`
   across re-entry, so when NEW_GAME stops re-entering the title and runs the
   scene, the trace's post-Start events feed it.
3. ~~Dispatch the title return code instead of exiting~~ **DONE (ckpt 33,
   `app_flow_dispatch` + `reenter_title`, quirk #60).** Exit exits; commits
   dispatch; unported arms re-display the title.

## Tooling added ckpt 32

- **`--menu-trace`** (`src/title_sink.c`, `title_sink_menu_trace`) — logs a
  stderr line whenever the highlighted menu row changes
  (`[sink] menu cursor row 1 -> 2 (y=80)`), so injected nav is verified at the
  cursor-state level, not by eyeballing pixels. A CLI flag, **not** an env var:
  WSLInterop does not forward arbitrary Linux env vars to the Windows child
  (only nix-shell-exported ones like `OPENSUMMONERS_GAME_DIR` reach `getenv`).
- **Menu-nav trace recipe** (self-serviceable, no Frida): write a
  `{"frame":N,"ids":[..]}` JSONL into the **game dir** (the child's CWD; a
  Windows exe can't read `/tmp`), then
  `OSS=/tmp/oss.exe; $OSS --hide-window --menu-trace --frames 720 --input-trace trace.jsonl`.
  Button ids: **1=up, 3=down**, 2/4=page, **0x24(=36)=confirm**, 0x22=abort.
  Time presses **after ~flip 577** (cursor visible); the input gate opens
  earlier (~flip 547). Confirm on row N → scene returns that row's action id
  (Start `0x1a`=26, Continue `0x1c`, Bonus `0x1e`, Options `0x1d`, Exit `8`).

## Tooling added this ckpt (31)

- **`frida_capture.py --seed-pin` / `--seed-value`** (default ON, `0x4f5347`) —
  the agent hooks `FUN_0056c070` and writes `DAT_008a4f94` to the fixed seed at
  the first spawn, so retail's phase-7 twinkles match the port's pinned-seed
  build. One-shot.
- **`subtitle_anim_start` TAS anchor** — the same first-spawn hook always emits
  `{kind:'anchor', name:'subtitle_anim_start', frame}` (recorded in the run
  summary's `anchors`), independent of seed-pin. Use it as tick 0 to align
  captures, since retail's intro pacing jitters the flip index run-to-run.
- **`/tmp` diff scripts** (not committed): per-tick `differ_px` sweep of port
  frames vs a dense retail flip window — how `differ_px=0` was found at
  port-465/retail-939. Re-derive with PIL `ImageChops`/numpy if needed.

## Tooling added ckpt 30

- **`frida_capture.py --fade-probe`** — hooks `FUN_00448c80` (the fade→alpha
  ramp), logs the first `(value,div)` per Flip → `<run>/fade_level.jsonl`. In
  phases 0–4 the first call's value IS the studio/title logo fade, so this gives
  retail's logo fade per flip → match a port frame at the same fade and diff (how
  logos #2/#3 were verified `differ_px=0`). **Caveat:** in phase 7 the first call
  is the first *sparkle* (`min(7·fade,1000)`), not the raw fade; phases 5–6 don't
  call `0x448c80` at all (the gap in the jsonl pinpoints them). Generalises the
  `--cursor-probe` pattern (`installFadeProbe` in the agent).

## Tooling added ckpt 29

- **`frida_capture.py --pace-probe`** (+ `--pace-every N`) — timestamps Flips →
  `<run>/pace.jsonl` + a live `flips/s` print, and stamps the cursor-onset
  event with wall-clock ms. This is how R3 was measured (retail: ~127 flips/s,
  menu onset Flip 1172 @ 9.23 s). Use `--no-turbo`.
- **Port `pace:` phase log** (`src/main.c`) — logs each phase transition with
  Flip count + wall-clock (`pace: phase A -> B @ flip=N t=Mms`). The port-side
  counterpart of `--pace-probe`; how the port's wall-clock-to-menu was checked.
- **`/tmp/pace_sim.py`** (not committed) — a Python replica of `title_pace_step`
  + `title_fade_step` used to validate the driving fix offline (ratio 1.00, 0
  missed fade values) before touching C. Re-create from quirk #54 if needed.
- **`--hide-window` now skips the desktop present** (`drive_present`) — kills
  the screen flicker (quirk #55); captures unaffected (read `primary_obj` first).

## Tooling from ckpt 28

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

## Module inventory (16 modules) — render pipeline COMPLETE

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper, cs_dispatch,
app_pump, title_scene (`FUN_0056aea0`, fully ported+wired+driven), input
(`FUN_0043c110`), obj_container, menu_list, **title_render** (compositor +
wrappers), **title_sink** (cmd→ZDD bridge, banks 19/20), **title_drive** (caller
side of the runner), **rng** (the MSVC LCG `FUN_005bf505`/`_5bf4fb`, ckpt 31),
**title_particles** (phase-7 sparkle pool: spawn `0x56c070` + update `0x56ba69` +
cull `0x56c030`, ckpt 31), **app_flow** (post-title dispatch = `FUN_00562ea0`
tail switch, ckpt 33). **8d** (`zdd_object_new_cell/_build_cell/_copy_cell_pixels`
+ `bs_convert_*` + slicer) ported ckpt 25, **now firing live** (banks registered
ckpt 26). `main.c` drives the scene against the live ZDD with the 8d hooks +
`init_sprite_banks` wired; on a menu commit it `app_flow_dispatch`es the result
(Exit→shutdown, else→`reenter_title`). `--no-title-scene` restores the legacy
present loop.

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

The title screen is now **bit-exact** end-to-end: menu + cursor (R1, ckpt 28),
both intro logos + the sparkle subtitle-reveal sweep (ckpt 30), pacing (R3,
ckpt 29), and the phase-7 particle twinkles (R4, ckpt 31, seed-pinned). **No
known intro-content residual remains.** Next: drive the new-game menus (the
`--input-trace` path) and confirm they render, then the prologue.

## Open RE threads (see ROADMAP subsystem map for the rest)

- **Title render-half arms — ALL WIRED + bit-exact** (`title_sink.c`):
  `MENU_CURSOR` (ckpt 28), `LOGO` (folded into `SPRITE_LEVEL`, ckpt 30),
  `SPARKLE` subtitle-reveal sweep (ckpt 30), and the **`FUN_0056c070` particle
  twinkles** (spawn/update/cull/draw, ckpt 31, quirks #57/#58 — DONE, bit-exact).
  (`TITLE_DRAW_LOGO` sink case + the `draw_logo`/`draw_sparkle`/`draw_cursor` ctx
  callbacks are now vestigial fallbacks; nothing emits LOGO.)
- **Outer-loop side-effect hooks** (stubbed in `title_scene_hooks`): `0x5b1030`
  (message pump), `0x43e140`/`0x40fe00`/`0x566250` (pre), `0x43c2e0` (per-entry).
  **`0x56c930` (post) is now WIRED** (ckpt 32) — its mode-1 `+0x54` ramp opens
  the menu-input gate (`drive_post_update` → `menu_owner_transition_step`); modes
  0/2 (submenu slide) are still deferred inside that port. **NB the rest are NOT
  the intro-pacing key** (that was the driving cadence, fixed in `main_loop_body`
  ckpt 29 — see quirk #54); `0x43e140`/`0x40fe00` are audio/joystick updates,
  port when those land. `0x43c2e0` animates a node's *child* widgets (gated on
  `+0x54>=1000`) — needed for in-row sub-widget animation, not basic nav.
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
