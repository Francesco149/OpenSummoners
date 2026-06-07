# Session handoff — rolling current state (last updated ckpt 83, 2026-06-07)

> **This is a ROLLING file — rewrite the current-state + next-move sections in place
> each checkpoint; do NOT append.** The dated per-checkpoint narrative is the
> append-only `PROGRESS.md` (every ckpt back to 26 is there); the 60-second front is
> `FRONT.md`; durable RE writeups are `findings/`. Keep this to: the current checkpoint,
> the next move, the module layout, and open RE threads.

## Where we are — ckpt 83

**The establishing-hold CAST is PINNED to its producers — Phase 1's complete
producer map.**  RE + live-census milestone (no port yet); resolves the ckpt-82
"pin the cast source next" open item and decomposes the scene for porting.

- **Method.**  Field-spec **band census**: added the 5 non-main band render
  entries (`0x4937c0`/`0x493480`/`0x492fc0`/`0x493230`/`0x493ba0`) + the two emit
  primitives (`0x492670`/`0x4917b0`, with `renderid` on the emitted cel) to
  `tools/flow/retail_fields.json`, captured at the hold (flips 1450/1500/1600,
  `--seed-pin --lockstep --no-turbo`).  The driver `0x48c150` (free-roam branch)
  runs 8 emit passes over the `DAT_008a9b50` bands → one present `0x48eac0`; the
  0x3c draw-node carries cel+pos+mode only (no producer back-ref), so the
  cel↔producer tie is read at EMIT.  The emit-primitive `cel_res` hook is
  authoritative (it caught a VA-arithmetic footgun — see CAVEAT).
- **The 18 visible keyed cels = FOUR map-object bands (all DATA-1022):**
  - **`+0x2560` STRUCTURE → `0x493230`** (single-cel renderer): the **TREE**
    `0xec55` bank `0x15f`→res `0x481` (×2); **bg decorations** `0xec6a` bank
    `0x16c`→`0x403` (×29, layer 8); **fg hedges** `0xec60` bank `0x164`→`0x426`
    (×8; the 5 on-screen are **layer 15** = the bottom row).
  - **`+0x1160` EFFECT → `0x493ba0`** (multi-part char render, built on the ported
    `0x44d160`): the **townsfolk** — 10 distinct `0xc3xx` (1 each) + `0xe29a`×4 +
    `0xe2a5`, banks `0x8b`–`0x146`→res `0x459`/`0x462`/`0x46a`/`0x46b`/`0x472`/
    `0x47b`/`0x426`/`0x3fa`.  layer 12/13.
  - **`+0x11e0` CHARACTER → `0x491ae0`**: collision volumes (bank 0) + props
    (`0x16c`→`0x403`) + the script wagon `0x1872d`.  **Already ported.**
  - **`+0x13e0` → `0x493480`**: 41 animated bank-`0x1aa`→res `0x408` particles
    (layer 6 sky + a square cluster) — blit via alpha/clipped, **NOT in the keyed
    set**; deferred.
- **Map-driven + deterministic (the key result for porting).**  STRUCTURE
  render-state = DATA-1022 record EXACTLY: world pos = map `(x,y)`×100,
  **`frame_base` = map `variant`@+0x18** (verified cel-for-cel: tree {0,1}, hedge
  {0,1,4,5}, deco {16,18,20,21,24,26,28,32,33,35} identical live-vs-map).  The
  code→bank map is the activator's per-type def table (lazy `+0x48` fill, #80):
  `0xec55`→`0x15f`, `0xec60`→`0x164`, `0xec6a`→`0x16c`.  EFFECT townsfolk map 1:1
  by code/count (`map_data --objects`: 13 EFFECT codes incl. `0xe29a`×4) but carry
  a deterministic spawn offset (≈+3000 x) from the `0x41f200` activator.  `0xc35a`
  (×2, also drawn by the party renderer `0x4997b0`), `0xc3dc`, `0xc3f0` are NOT in
  the map → script/party-spawned (like the wagon).
- **Hold = mostly STATIC.**  Across flips 1450/1500/1600 (cam 128000, pre-pan) the
  16 standing townsfolk + 39 structure objects hold a FIXED world pos (only the
  anim frame steps, deterministic per #76); only `0xe29a`×4 translate (RNG wander,
  Phase 2).  Refines #82: the +0x1160 EFFECT band updates during the hold.
- **Corrects the docs' model:** the visible cast is NOT the +0x11e0 band (that's
  collision volumes + props); it splits across +0x1160 (townsfolk) / +0x2560
  (tree/scenery) / +0x11e0 (props) / +0x13e0 (particles).  The "foreground tree"
  is STRUCTURE `0xec55` (res `0x481`) — NOT a banner / `0x5a00c0` overlay / tile
  (all refuted).
- **CAVEAT (footgun):** hand-computing the band VAs for the census analysis was
  wrong (off by 0x200) → bands falsely read "0 active".  Compute VAs in code;
  trust the emit-`renderid` hook over a band-entry census.  quirk #84.
- **State: 896 pass unchanged** (no C touched; docs + `retail_fields.json` only).
  Ledger unchanged.  quirk #84; `findings/in-game-intro.md` "The establishing-hold
  cast is FOUR map-object bands".
- **STRUCTURE band DONE (ckpt 83b) — PORTED + USER-1:1.**  RE'd `0x438a60`
  (per-code bank def table: 0xec55→0x15f, 0xec60→0x164, 0xec6a→0x16c, …) + the
  `0x58d460` dispatch (layer = record +0x30 ? 15 : 8; frame_base = variant +0x18;
  pos = (x,y)×100 — all verified cel-for-cel vs the census).  PORTED
  `actor_spawn_struct_from_map` (60000-range, fully map-driven) +
  `actor_spawn_struct_bank_for_code`; the RENDER reuses `actor_render_static` (the
  `0x493230` static single-cel blit is bit-identical to the default actor arm).
  Wired via `game_actor_walk` (walks g_structs at layers 8/15) + spawned in
  `enter_game`.  **Live: 39 objects spawned + 39 nodes emitted (tree bank 0x15f
  registered); render_diff/position-verified bit-exact** (tree `(0x481,f0)`@(496,64)
  320×320, the 5 hedges, the 4 `0x403` props/deco — all identical port↔retail, zero
  `[rect]`/`[decode]`/`[state]`).  **USER-confirmed on the feed: "the decorations
  are there and positioned 1:1".**  897 pass (+1 `actor_spawn_struct`); ledger
  199/194 unchanged (bare-VA slices); parity-ledger #9.
- **NEXT (Phase 1 cont.) — the EFFECT townsfolk (the people in the square):**
  1. **The multi-part char renderer `FUN_00493ba0`** — built on the ALREADY-PORTED
     `0x44d160` (actor_render_describe) + `0x492670` (draw_pool_emit_actor) + the
     shadow/color-split layers via `0x4917b0`.  Port its core static arm
     (`LAB_004943d7` → describe → the emit loop); the 16 standing townsfolk are
     static (clip-animated frame only, deterministic per #76).
  2. **The EFFECT spawn `FUN_0041f200`** (the 50000-range activator, 0x58d460:151)
     — map pos + the deterministic ≈+3000 x offset (RE it; the static townsfolk
     land at a matched sim-tick).  code→bank def table (banks 0x8b–0x146).
  3. The 4 wandering `0xe29a` need Phase 2 (RNG, deferred).  Then the `0x4962a0`
     off-screen invisibles + the `0x13e0` bank-0x1aa particles (alpha).
- Artifacts (local `/tmp`, ephemeral): `/tmp/cast_census/`, `/tmp/tree_emit/`,
  `/tmp/port_hold_trace.jsonl`, `/tmp/retail_hold_1500.png`; analysis
  `/tmp/census_fixed.py`, `/tmp/parse_variant.py`.  Regen via the field-spec
  capture above.

## Where we are — ckpt 81

**The caravan's HORSES now TROT — the per-tick actor animation is wired and
BIT-VERIFIED live.**  Builds directly on ckpt 80 (the frozen wagon); the trot is
the first thing the per-sim-tick actor UPDATE pass drives in the port.

- **The RE that made it safe (engine-quirk #82).**  `FUN_0054f980`'s case-`0x1872d`
  (`:911-970`) splits cleanly: **`:911-928` is the frame-stepper, run
  UNCONDITIONALLY** (gated only on the clip `+0x6c != 0`; byte-identical to
  `anim_clip_advance`, reads no RNG/clock), and **`:929-970` is the behaviour**,
  which `break`s out unless this is the primary entry AND the global scene-lock
  `*(DAT_008a9b50+0x27a8)==0`, then draws the LCG (`FUN_005bf505`) for idle waits /
  wander — the RNG layer deferred by ckpt 73 / quirk #77.  So the horse-trot is a
  pure deterministic function of sim-ticks and portable in isolation; the wander
  stays deferred.  Driver `FUN_0046cd70:123-169` walks the 0x80-slot `+0x11e0` band
  once per sim-tick calling `0x54f980` per active actor.
- **PORTED (pure + host-tested; +3 tests, 896 pass).**
  - `actor_render_state` gains the anim sub-block `timer` (+0x70) / `done` (+0x74)
    — it already had `clip` (+0x6c) / `frame` (+0x72); the slice the stepper writes
    IS an `anim_state`.
  - **`actor_anim_advance`** (`actor_render.c`) — the per-actor stepper; a thin
    adapter bridging the render-state's anim block to the single ported stepper
    `anim_clip_advance` (one source of truth, host-tested bit-exact ckpt 72).
  - **`actor_pool_update`** (`actor_spawn.c`) — the `0x46cd70` main-band walk:
    advance every active render-state with a clip; the 32 static actors (clip NULL)
    no-op, only the wagon trots.  Returns the count advanced.
  - `main.c game_actor_update` runs it on the SAME sim-tick gate as the camera
    easer (`(g_game_camera_hold & 1)==0`), BEFORE `camera_follow_step` — mirroring
    retail's `0x439690` per-tick body order (`0x46cd70`@:1108 then `0x43d1d0`@:1123).
    `CALL_TRACE_BEGIN(0x46cd70)` port mirror (emits `advanced`).  Reset is automatic
    (`enter_game` re-spawns the pool frame/timer 0 + zeroes the hold counter).
- **LIVE-VERIFIED at the byte level** (port blit trace, settled cam 12800, flips
  2100-2244 = one 144-Flip clip cycle): the wagon (bank `0x175`) is **3 keyed cels
  res `0x3ec`** at screen x 160/288/416 (the −256/−128/0 composite); the body cel
  (x416) steps **5→2→3→4→5** (one body-frame per 36 Flips = 18 sim-ticks) while the
  two fixed wagon cels hold frames 0/1; the `0x46cd70` mirror reports `advanced:1`
  each tick.  **CORRECTION:** the wagon's render_id is **res `0x3ec`** (asset_register
  idx 215), NOT `0x058f` as ckpt-80 noted — fixed in FRONT/quirk #81.  **USER-CONFIRMED
  on the feed:** "the horses' ears animate slightly, looks correct.  The wagon doesn't
  move — the horses are just idling, which is how it's supposed to be."  So `WAGON_CLIP`
  is a SUBTLE IDLE loop, the wagon is PARKED at the settled hold (no locomotion), and the
  "trot" wording is shorthand for that idle cycle (quirk #82).
- **State: 896 pass / 0 fail / 6 skip** (+3).  Ledger **199/194 unchanged** (the
  stepper/walk are bare-VA slices of `0x46cd70`/`0x54f980`; `anim_clip_advance` was
  already counted).  quirk #82; PORT-DEBT `actor-protagonist-clip` narrowed (the
  trot half is done; the RNG behaviour + the cutscene roll-in remain).  Writeup:
  `findings/in-game-intro.md` "The horses TROT".
- **NEXT (corrected ckpt 82 — the "(b) siblings" lead was a DEAD END):** the
  code-adjacent actors `0x1872e`/`0x1872f`/`0x18730` are **OUT-OF-SCENE** (statically
  proven): `0x1872e`←`FUN_00539e80` `case 0x64280` = room 410240 (area 410);
  `0x1872f`←`FUN_005034b0` `case 0x382de` = room 230110 (area 230); `0x18730` = child
  of non-town CHARACTER `0x11350` (not in DATA-1022's 32 town char codes).  All four
  codes (100141-100144) are outside the 70000 map-object range, and the town script
  `FUN_004d7d80` (area-210 rooms `0x334be`…) spawns ONLY the wagon `0x1872d`.  So
  code-adjacency ≠ same scene (engine-quirk #83); the siblings are later-area beats.
  `findings/in-game-intro.md` "The caravan 'siblings' … are OUT-OF-SCENE".

- **GROUND TRUTH (ckpt 82) — the hold residual is the CHARACTER CAST + foreground
  TREE, NOT a "banner".**  Re-captured the retail blit trace + a PNG at the
  scene-LOCKED establishing hold (`game_enter@1434`, flip 1500, cam 128000;
  `--seed-pin --lockstep --no-turbo`, field-spec).  The 108 `blt_keyed` (`0x5b9b70`)
  split into TWO producers:
  - **54 VISIBLE via present `FUN_0048eac0` (`ret_va 0x48ecc2`, 18/frame)** = res
    `0x481` **320×320** @ (496,64) the foreground **TREE** + ~5-7 multi-part townsfolk
    **CHARACTERS** (banks `0x426`×5 / `0x459` / `0x462` / `0x46a` / `0x46b` / `0x472`
    / `0x47b`) + props `0x403` + tiny details `0x3fa`.  The PNG confirms a tree (right)
    + a knot of townsfolk in the square + a flowerbed.
  - **54 INVISIBLE via `FUN_004962a0` (`ret_va 0x49632a`/`5c`/`8b`)** parked at
    **dst y=572 (off the 480 screen)**, NO render-id — a scratch/HUD parked during the
    cutscene; draws nothing visible.  (Identify `0x4962a0` before porting.)
  - **No "Town of Tonkiness" banner blit at the hold** — zero `0x5a00c0`-range
    `ret_va`s → the docs' banner attribution AND the `0x5a00c0`-overlay producer guess
    are BOTH refuted (same as the letterbox turned out to be `0x48c150`).  Any
    area-title card is GDI text / a different time (TBD).
  - The cast is **NOT** the main map-object band (bank `0x16c`/`0x175`, world-x
    88200-176000 = off-screen-LEFT at cam 128000; the 6 drawing main-band actors don't
    present here).  Source = the 8 PARTY actors (`0x59f2c0`→`0x560e60`, `ret_va
    0x59f578`) and/or a scene-actor band — PIN IT NEXT.
  `findings/in-game-intro.md` "The hold residual is the CHARACTER CAST + foreground
  tree".  Artifacts (local `/tmp`): `/tmp/blit_banner_retail/`, `/tmp/spawn_disc/`.

- **USER DIRECTIVE (ckpt 82): the intro scene must render 1:1 on EVERY frame before
  moving to the next scene; THEN pinpoint + port every RNG consumer in the scene 1:1.**
  Per quirk #82 the hold is scene-locked ⇒ deterministic, so this decomposes cleanly:
  - **PHASE 1 (this residual) — the CHARACTER / multi-part static render.**  Pin each
    cast cel to its actor (annotate the emit `FUN_00492670` / the band-walk feeding
    `0x48eac0` with the actor code `+0x1d4`, recapture → cel↔actor map), RE the cast
    spawn (party `0x59f2c0` and/or the scene-actor band), port the multi-part static
    render (generalise the `0x491ae0` `0x1872d` arm past the wagon; the lazy `+0x48`
    sprite-table fill, PORT-DEBT `actor-sprite-table`) → the LOCKED establishing frames
    go differ_px==0.  Also byte-confirm the wagon (`render_diff` keyed `(res 0x3ec,
    frame)`).  The foreground TREE (res `0x481`, a static prop) is the simplest single
    cel to land first.
  - **PHASE 2 — every in-scene RNG consumer.**  Post-unlock the scene-lock clears and
    `0x54f980:929+` draws the LCG for idle/wander; RE + port every consumer and match
    consumption order (rng + rngcalls both sides, the flow trace's unified signal) so
    the post-cutscene frames stay 1:1.  (This RETIRES the ckpt-73 "defer all RNG"
    deferral — it is now scheduled, not indefinite.)
  - **Big foundational arc — best started with a fresh `/clear`** (the character render
    system underlies every scene).  Orient: this file → FRONT → here → the ground-truth
    finding above.

## Where we are — ckpt 80

**The town intro `0x1872d` is PORTED + SPAWN-RE'd + WIRED + USER-CONFIRMED — and
it's the arrival HORSE-DRAWN CARAVAN, not "the protagonist" (corrects #79/#80).**

- **Render arm (commit `af31c69`):** `actor_render_protagonist` = `0x491ae0`'s
  case-`0x1872d` (`0x491ae0:112-192`).  KEY: part 2 (the body) is byte-identical
  to `FUN_0044d160`/`actor_render_describe` (same clip/mirror/angle/link build +
  all three early-return gates); the arm just wraps it with TWO fixed bank-`0x175`
  cels (frame 0 @ x-256, frame 1 @ x-128) → a 3-cel composite at a 128-px pitch.
  Refactored `actor_emit_part`/`actor_emit_layer` out of `actor_render_static`.
- **Spawn, fully RE'd (commit `08fd0be`):** `0x1872d` is NOT a map object (code
  outside 70000), so `actor_spawn_from_map` never makes it.  Chain: the town intro
  cutscene **`FUN_004d7d80`** (`case 0x334be` = room 210110 / area `0xd2`, gated on
  event flags `0x5f76805`/`0x606aa4f`) → **`FUN_00431d10(0, 0x1872d, anchor=0x65,
  x=0x3200, 0, 0)`** (the by-code `+0x11e0` spawn helper: free-slot scan +
  anchor-relative placement) → **`0x431e30` case-`0x1872d`** which sets layer 9,
  facing `+0x2c`=99, resolves the pos (`0x41ee60`), installs the clip
  (`+0x6c = &DAT_00671c48`), and installs the sprite via **`FUN_00426db0(0, 0x175,
  0, 1, 0, 0, 0)`** — the long-missing `+0x48` FILL PRIMITIVE (`426db0(dir, bank,
  frame_base, b, x_off, mirror_x, y_off)` writes one `actor_sprite_row`; RETIRES
  the ckpt-79 "lazy fill not RE'd" unknown).  `actor_spawn_protagonist` ports the
  end state; `main.c game_actor_walk` dispatches code `0x1872d` →
  `actor_render_protagonist`, else `actor_render_static`.
- **The HORSES fix.**  First render froze the body on frame_base 0 → its rightmost
  cel redrew the wagon-left cel ("the right wagon is cut in half" — USER).  The
  body is the **animated HORSES**.  Decoded the clip `&DAT_00671c48` from the
  user's `sotes.exe` `.rdata` (file off `0x271c48`): base_sprite 2, 4 frames, dur
  18, looping, delta {0,1,2,3} → body cels 2..5 = the horses.  Pointed the
  render-state at a reconstructed `WAGON_CLIP` (the 4 RE'd values) → the body draws
  sprite 2 = the horses.  **USER-CONFIRMED on the feed: "that definitely matches
  retail."**
- **State: 893 pass / 0 fail / 6 skip** (+4).  Ledger **199/194 unchanged** (the
  arm + spawn are bare-VA slices of `0x491ae0`/`0x431e30`; `426db0` referenced by
  bare VA).  quirk #81; PORT-DEBT `actor-protagonist-clip` (body frozen on frame 2;
  the per-tick stepper that trots the horses + the cutscene roll-in are deferred).
  Writeup: `findings/in-game-intro.md` "The 0x1872d SPAWN + the arrival WAGON".
- **NEXT:** (a) the per-tick anim (advance `WAGON_CLIP` so the horses trot — needs
  the `0x46cd70`/`0x54f980` update pass or a minimal sim-tick advance); (b) the
  scripted caravan roll-in + anchor-relative spawn (the `0x4d7d80` cutscene);
  (c) the siblings `0x1872e`/`0x1872f` (likely the CHARACTERS — spawned by
  `0x539e80`/`0x5034b0`); (d) byte-confirm via `render_diff` keyed on res `0x058f`
  vs a panned-camera retail capture.

## Where we are — ckpt 79

**The town actor RENDER CENSUS overturns the ckpt-76/78 "32 static actors"
picture — only 6 of 33 main-band actors DRAW — and the minimal CHARACTER SPAWN
is ported + host-tested.**  This is the gating input the ckpt-77 renderer needed,
captured as ground truth (the methodology's "capture each slot's `+0x48` live").

- **The capture.**  Extended `tools/flow/retail_fields.json` `0x491ae0` with the
  `+0x48` sprite-table reads (`row0_bf` = bank|frame_base dir-0 word, `d1_bf`…
  `d7_bf` for dirs 1-7, `dir_e8`, `alpha_f4`/`skip_284`/`angle_ec`, render-state
  `rs_dstx`/`rs_dsty`/`rs_lo284`) — `thisderef`/`thischain` off the actor ECX —
  and ran a field-spec capture at the town hold (flip 1480/1500/1520,
  `--seed-pin --lockstep --no-turbo`, `trace-retail.jsonl`).  33 actors × 3 flips.
- **The result (corrects quirks #78 + #79).**
  - **27 of the 33 are INVISIBLE** — all-zero `+0x48` in every direction, so
    `FUN_0044d160` returns 0 (`bank==0`).  Collision / trigger / spawn volumes
    (`0x111d6`/`0x112e6`/`0x112e2`/`0x11365`/… — the codes whose `0x431e30` arms
    build a physics body, not a sprite).
  - **Only 6 DRAW** (all dir 0, clip 0 = static, skip 0).  Bank `0x16c` (res
    `0x403`) is the town-OBJECTS sheet → these are static **PROPS, not people-NPCs**
    (USER-confirmed against retail: a fountain + a barrel, at the correct spots):
    `0x1129e`×3 (frame 1, layer 9; a barrel), `0x1129f` (frame 2, layer 9),
    `0x112e5` (frame 36, **layer 10**; the fountain).  The one PERSON is
    **`0x1872d`** the **animated protagonist** (bank `0x175`, clip `0x671c48`,
    `+0x2c`=0x63) — **OUTSIDE** the 70000 CHARACTER range (a SEPARATE spawn), needs
    the `0x491ae0` `0x1872d` multi-part arm; its body-part banks (`0x426`/`0x459`/…)
    are **the bulk of the 36-blit residual**.
  - **`0x426620` ZEROES `+0x48`** (the `type*0x80+0x21c04` in it is the
    **cell-indexed collision-grid** lookup #79 misnamed — it writes `+0x288/+0x28c`,
    not a sprite).  The table is filled **LAZILY** by the state-set machinery
    (`0x40afe0`/`0x41e600`) from a type-keyed def table — **not yet RE'd**.
  - **The props sit at a DETERMINISTIC per-code offset from `map_x*100` — NOT RNG**
    (earlier draft wrongly said "wandered").  All three `0x1129e` share the exact
    `+1800x/+1600y`; positions identical across flips 1480/1500/1520 (static).  The
    visible fountain `0x112e5` is `+0/+0` → lands exactly at `map_x*100` and matches
    retail (USER-confirmed).  Delta source TBD (the `0x426620` alignment arm or the
    lazy fill); the port spawns at `map_x*100`.
  - Census artifact: `/tmp/actor_census.json` (ephemeral; regenerate via the
    field-spec capture above).  Engine-quirk #80; `findings/in-game-intro.md`
    "The town actor RENDER CENSUS".
- **Ported + WIRED + LIVE-VERIFIED (+6 tests, 889 pass).**
  - `src/actor_spawn.{c,h}` (pure): `actor_spawn_from_map(pool, map_data)` = the
    `0x58d460`→`0x431e30` slice — walk the object layers, filter to CHARACTER (code
    70000..79999), activate a slot per object (world `(x,y)*100`, dir 0, layer 9,
    static clip NULL), seed the 3 prop codes' dir-0 sprite rows from the captured
    stand-in (`actor_spawn_sprite_for_code`, PORT-DEBT `actor-sprite-table`), leave
    the rest bank 0.  `actor_spawn_pool` = parallel `{actor, render-state}` arrays.
  - `town_render_step_ex` (town_render.c) adds an **actor seam** (a
    `town_actor_walk_fn` called after the tile walk, before the present, into the
    SAME pool) + passes `present_dims_fn` through; `town_render_step` is the
    tile-only wrapper (existing callers unchanged).
  - `main.c`: `g_actors` populated in `enter_game`; `game_actor_walk` walks it →
    `actor_render_static`; `game_cel_dims` (cel `metric_b8/bc`) = the mode-0 cull
    box; `game_present_blit` `PRESENT_KEYED` arm → `zdd_object_blt_keyed` (`0x5b9b70`).
  - **LIVE-VERIFIED:** port logs `game_actor_walk: 5/32 actors emitted (bank 0x16c
    registered)`; the props render at the correct spots (USER-confirmed on the feed
    — fountain right @ screen ~480, barrel left-edge).  Only `0x112e5` (fountain) is
    fully in-window at the hold (cam 128000); the other props are off-screen-left.
  - Tests: `test_actor_spawn` (5) + `town_render_actor_seam` (1).  Ledger 199/194
    (spawn is a bare-VA slice; only the already-counted `FUN_0044d160` appears).
- **NEXT — the `0x1872d` protagonist multi-part animated arm** (`0x491ae0:112-192`):
  the actual PERSON + the bulk of the 36 residual.  Reuses `anim_clip`; spawned by a
  separate (non-`0x431e30`) path (find it — the protagonist isn't in `g_actors`).
  Then `render_diff` vs retail flip 1500 keyed on `(res, frame)` (camera hold-vs-pan
  is a standing deferral, so the signal is actor-blit IDENTITY).  A fuller prop
  verify can drive the port's pan so the off-screen-left props enter the window.

## Where we are — ckpt 78

**The town actor SPAWN is RE'd end-to-end + byte-verified against the map bytes —
no live drive needed.**  This unblocks the (ckpt-77) ported renderer: the spawn's
*inputs* (per-actor code + world x/y) are now known, parsed by `map_data` already.
Docs-only checkpoint (no C touched; 883 pass unchanged).

- **The chain (corrects the ckpt-76 guess `0x42eb20`/"`0x587e00`'s layer pass"):**
  `0x586010:698` → **`FUN_0058d460`** (room object-population pass) → **`FUN_00431e30`**
  (character activator).  `0x58d460` walks the map descriptor's **86 object-placement
  layers** (`mapobj+0x38` headers ×0x3c + `mapobj+0x3c` sub-ptr records ×0x10) and
  dispatches each by the **range of its type code** (`header+0x10`) into four
  pre-allocated bands off `DAT_008a9b50`, each guarded by a named `"<kind> Object
  Count Over"` abort:  EFFECT 50k→`+0x1160` (`0x41f200`), STRUCTURE 60k→`+0x2560`
  (`0x438a60`), **CHARACTER 70k→`+0x11e0` (`0x431e30`)**, DEVICE 80k→`+0x13e0`
  (`0x557550`).  `0x431e30` (`__thiscall`, ECX=free slot) is a per-type switch:
  sets `+0x1d0=1`/`+0x1d4=type`/`+0xfc=9`(layer)/`+0xe8=0`(dir), zeroes the `+0x48`
  sprite table, stores world (x,y), and a per-type helper (`0x426620` + the
  `0x4264xx–0x4273xx` cluster) installs the sprite/anim from a def table
  (`type*0x80+0x21c04`).
- **The byte-level proof (resolves "codes never assigned as constants"):** the town
  behaviour codes ARE the map object type fields.  `tools/extract/map_data.py …
  --objects` decodes DATA 1022's 86 layers → **15 effect + 39 structure + 32
  character + 0 device**; the 32 character codes + multiplicities are IDENTICAL to
  the ckpt-76 live census (0x112e6 ×10, 0x111d6 ×7, 0x1129e ×3, 0x112e2/0x11365 ×2,
  8 more ×1), with world positions.  The 33rd live actor = the 1 animated NPC
  (`0x1872d`=100141, outside the char range → separate path).
  Proof: `docs/proofs/map-object-layer-format.md`; engine-quirk #79;
  `findings/in-game-intro.md` "The town actor SPAWN".
- **The port input that remains:** the **code → `+0x48` sprite table** mapping (the
  only datum NOT in the map record — `0x431e30`'s per-type def-table install).  RE
  the 13 town codes' cases, OR capture each spawned slot's `+0x48` table live (hook
  `0x431e30` onLeave).  Then a minimal spawn (read the 32 objects from `map_data`,
  fill render-state pos + sprite table + dir + layer 9) drives the ported renderer →
  wire into `game_render` → `render_diff` vs retail flip 1500 (the 36-blit residual
  should drop) → human pixel-verify.

## Where we are — ckpt 77

**The town ACTOR RENDER SIDE is PORTED + host-tested** (the default arm that
draws 32/33 town actors), ahead of the spawn.  Pure, no harness; the SPAWN
(band population — RE'd ckpt 78 above) + the `0x1872d` animated arm + the
`game_render` wiring + pixel-verify are the next arc (needs the human for the feed).

- **Ported (commit `0533603`):**
  - `draw_pool_emit_actor` = **`FUN_00492670`** (`src/draw_pool.c`): the actor
    analog of `draw_pool_emit`; same 0x3c node, mode = `bool(alpha!=0)`, alpha in
    the param8 slot, NULL cel emits nothing.
  - **`src/actor_render.{c,h}` (NEW):** `actor_render_describe` = **`FUN_0044d160`**
    (the static/animated/mirrored/angle sprite descriptor over the per-direction
    table `actor+0x48`) + `actor_render_static` = the **`0x491ae0` default arm**
    (skip flag, layer + override, describe, emit).  actor + render-state are
    LOGICAL structs (the spawn fills them); `actor_sprite_row` (0x14) pinned.
  - `map_present` **MODE 0** (`src/map_present.c`): the opaque-actor keyed path
    (project + cel-dims cull via the new `present_dims_fn` → `PRESENT_KEYED`
    `FUN_005b9b70`).  `dims=NULL` keeps the tile-only contract.
- **Validated:** render-state offsets match the ckpt-76 live `0x491ae0` field spec
  exactly (`rs_x`/`rs_y`/`rs_clip`/`rs_frame` = +4/+8/+0x6c/+0x72); logic
  host-tested bit-exact vs the decompile.  **883 pass / 0 fail / 6 skip** (+18).
  Ledger **199/194** (+`FUN_0044d160`, +`FUN_00492670`).  Both GUI builds clean.
- **NEXT (the gating arc — `findings/in-game-intro.md` "The town ACTOR render side"):**
  1. **The SPAWN** (the `+0x11e0` band activator).  Narrowed: NOT `0x560e60`
     (8 party actors) / NOT `0x584710` (refuted) / NOT the `+0x1d0`+`+0x1d4`
     static writers (`0x456a50` find-by-code, `0x487dc0` cell/collision).  It is
     the **entity subsystem** (`0x42eb20`/`0x4282f0`/`0x429060`/27 KB `0x41f200`)
     processing the **DATA 1022 layer entries** (86 — `map_data` parses them) via
     `FUN_00587e00`'s layer pass.  Empirical pin: `mem_watch --hw` an **ACTIVE**
     slot's `+0x1d4` (only the activation writes it; slot 0 is inert — find an
     active index first via a `0x491ae0` ECX log).
  2. **The `0x1872d` animated arm** (1 actor — a 3-element multi-part descriptor,
     `0x491ae0:112-192`); port WITH the spawn so it pixel-verifies.
  3. **Wire** the band walk into `game_render` (between `map_render_walk` and
     `map_present`) + the Win32 keyed sink + cel-dims callback → `render_diff` vs
     retail flip 1500 (the 36-blit residual should drop).

## Where we are — ckpt 76 (the RE that ckpt 77 built on)

**The town NPC/actor RENDER PATH is RE'd live, the trace tooling is hardened +
documented, and the spawn is narrowed to a precise lead.**  (RE + instrumentation
half of "implement the NPCs"; the render-side port landed ckpt 77 above.)

- **Trace tooling (the user's mandate "harden + document the foundation"):**
  - **`thischain`** field source (`tools/frida/opensummoners-agent.js` `ctReadField`):
    like `chain` but ROOTED at the `__thiscall` `this` (ECX) + pointer hops + `off` —
    reads a field BEHIND a this-pointer (an actor's render-state at `*(actor+0x40)+off`).
    The reusable primitive for probing any entity by its live `this`.
  - **Annotated** `0x491ae0` (actor render entry — behaviour `+0x1d4`, the `+0x48`
    sprite table, render-state pos/clip/frame), `0x560e60` (actor reset → spawn caller
    via `ret_va`), `0x584710` (candidate) in `tools/flow/retail_fields.json`.
- **The actor walk.**  Six actor bands off `DAT_008a9b50`; the **MAIN band is
  `+0x11e0` (0x80=128 slots)**, render-emitted by **`FUN_00491ae0`** (from the per-frame
  driver `0x48c150` free-roam branch) and updated by **`FUN_0054f980`** (from the
  per-tick `0x46cd70`); live when `actor+0x1d0 != 0`.
- **Live trace (retail town hold, flip 1500, `--seed-pin --lockstep`): 33 active
  main-band actors — 32 STATIC** (render-state clip `+0x6c`==0), **1 ANIMATED**
  (`+0x1d4`=`0x1872d`, the protagonist/key NPC).  **32/33 behaviour codes are NOT
  explicit `0x491ae0` cases → they hit the DEFAULT arm `caseD_11257` →
  `FUN_0044d160`** (the static-actor descriptor builder) → the emit tail → **`0x492670`**
  (the actor analog of `draw_pool_emit`; node mode 0=keyed / 1=alpha).  The behaviour
  code drives the **AI** (`0x54f980`, RNG motion deferred ckpt-73), NOT the render —
  **one function (`FUN_0044d160`) renders nearly the whole town.**
- **`FUN_0044d160`** reads `actor+0xe8` (dir) → the per-direction sprite table at
  **`actor+0x48` stride 0x14** (bank/frame_base/x_off/y_off) + the render-state
  (`+0x04/+0x08` world pos, `+0x2c` facing, `+0x6c` clip, `+0x72` frame).  Static actor
  (clip==0): cel = `(bank, frame_base+facing)` at the render-state world pos.
- **Render OUTPUT** (the 36 mode-0 keyed `0x5b9b70` blits @1500): res `0x403`/`0x426`
  (villagers) + `0x459`/`0x462`/`0x46a`/`0x47b`/`0x481`/… — **exactly the ckpt-75
  render_diff residual's named NPC banks**.  These ARE the 36 leftover divergences.
- **The band is a PRE-ALLOCATED 128-slot pool** (`0x586010:476-506` calls
  `FUN_0058cf60(0x40)` 0x80× for the main band; `0x58cf60` zeroes a slot, `+0x1d0=0`).
  So the per-room **spawn = ACTIVATE + configure** a subset, running **after
  `0x586010`'s `"Init Objects"` marker** (`:508`).  The behaviour codes are **data-driven**
  (never literal) → an **entity-by-id** subsystem (ROADMAP `0x420000`); NOT `0x560e60`
  (= the 8 PARTY actors, `ret_va=0x59f578`) / NOT `0x584710` (never fired).
- **State: 865 pass / 0 fail / 6 skip** (no C touched).  Engine-quirk #78;
  PORT-DEBT `present-actor-modes` (render-emit half will land here).
- **NEXT (the implement arc — best as one fresh-context session, ends in pixel verify):**
  1. **Find the `+0x11e0` activator** — instrument the code after `0x586010`'s
     "Init Objects" marker (hook the callee that sets `+0x1d0`/`+0x1d4`/`+0x274`, read
     `ret_va`; or `mem_watch --hw` a slot's `+0x1d0`); cross-ref the map DATA 1022 layer
     entries + ROADMAP `0x42eb20`/`0x4282f0`.
  2. **Port the render side** (pure + host-tested): `FUN_0044d160` (static-prop desc) +
     `FUN_00492670` (node emit) + the `0x491ae0` default-arm tail; the `0x1872d` animated
     arm reuses `anim_clip`.
  3. **Wire** `map_present` modes 0 (keyed `0x5b9b70`) / 1 (alpha `0x5bd550`) — cull dims
     from the sprite (`0x48eac0` mode-0/1 arms) — and drive the actor walk from `game_render`.
  4. **Verify** the town NPC blits vs retail flip 1500 (`render_diff` keyed on `(res,frame)`);
     the ckpt-75 residual should drop from 36.

## Where we are — ckpt 75

**The establishing-shot cinematic LETTERBOX is RE'd, ported, and blit-trace 1:1.**
The single biggest missing layer of the town frame (the ckpt-74 diff's 320 `0x583`
draws) is now drawn.

- **The producer — RE'd from the captured retail blit trace, NOT the `0x5a00c0`
  overlay as ckpt-74 guessed.**  The 320 res-`0x583` blits' return addresses
  (`0x8c48a`/`0x8c4fe` + image base 0x400000 = `0x48c48a`/`0x48c4fe`) land inside
  **`FUN_0048c150`** (the per-frame world driver), lines **124-162** — two
  grid-fill loops AFTER the backdrop present pass (`0x48eac0`).  Loop 1 (`in_ECX+0x44`
  = bottom-bar height, ret `0x48c48a`, emitted first) tiles the cel over dy
  416-476; loop 2 (`in_ECX+0x48` = top-bar height, ret `0x48c4fe`) over dy 0-60.
  Each bar rounds its height up to a multiple of the 4px cel height and tiles at
  64px column pitch (10 cols, dx 0-576; inner loop runs while `(dx+0x80)<0x281`).
  Both heights are **64** for the opening town → the quirk-#74 letterbox.
- **The cel** = main sprite-pool **slot 41** (PE resource **`0x583`**, 64×4, opaque
  `ckey=0x1ffffff`), registered by `ar_register_main_sprites` (extras[] idx 41,
  already run at boot, `main.c:718`).  The engine binds it via `FUN_00418470(0)`
  (the plain frame getter — NO `0x417c40` grade) before the `FUN_005b9a40`
  (`blt_onto`) tile blits.
- **PORTED (pure, host-tested + bit-exact vs the trace): `src/letterbox.{c,h}`** —
  `letterbox_render(top_h, bottom_h, sink, ctx)` ports the two loops verbatim
  (4 tests: the 64/64 town grid bit-exact vs the 320-blit trace, zero bars,
  null sink, the round-up-to-4 arithmetic).  Wired in `main.c`:
  `game_letterbox_blit` resolves `&g_ar_sprite_slots[41]` frame 0 →
  `zdd_object_blt_onto`, called in `game_render` AFTER `town_render_step` (on top of
  the backdrop, matching the engine order).  Heights armed to `LETTERBOX_INTRO_BAR`
  (64) in `enter_game`.
- **VERIFIED two ways.**  (1) `render_diff --retail-frame 1500 --port-frame 1200`:
  the town-frame divergences dropped **356 → 36** — all 320 `0x583` blits now match
  retail on identity + geometry + DDraw state (0 `[rect]`/`[decode]`/`[state]`, 0
  port-extra); the 36 left are exactly the deferred RNG-driven actor/banner/tree
  banks (`present-actor-modes`/`ingame-nontile-layers`).  (2) Port frame 1200
  pixel check: rows 0-63 + 416-479 are `(0,0,0)`, row 64 is the sky band — the
  central 640×352 window.  **USER-CONFIRMED on the feed.**
- **State: 865 pass / 0 fail / 6 skip** (+4).  Ledger **197/192 unchanged**
  (`letterbox.c` is a bare-VA slice of the unported `0x48c150` — no new `FUN_`
  token, correct).  parity-ledger #8.  Engine-quirk #74 updated with the proven
  producer.  PORT-DEBT `ingame-letterbox` (the 64/64 heights stand in for the
  unported `0x5a00c0` cutscene op writing the scene-object `+0x44`/`+0x48`; the
  grid-fill geometry is bit-exact).
- **NEXT chip:** the **"Town of Tonkiness" banner + foreground tree/veg** — the
  `0x5a00c0` scripted-scene overlay player (draw-list `stack+0x98` stride-10;
  caption array `stack+0x3a4` stride 0x124 via font bank `DAT_008a7640`).  Also
  where the pan TRIGGER and the letterbox `+0x44`/`+0x48` writer live — porting it
  closes `ingame-nontile-layers`, the trigger half of `ingame-camera-pan`, and the
  source half of `ingame-letterbox`.  Then the NPC actor render/spawn (entity
  system; RNG-driven motion deferred per ckpt-73).

## Where we are — ckpt 73

**The #75-addendum / ckpt-72 OPEN is RESOLVED: the actor-band residual is the RNG
pillar, and the shared LCG stream is non-deterministic run-to-run EVEN UNDER
`--seed-pin`.**  Ran the ckpt-72 directed live check.

- **Experiment.**  Drove retail TWICE (`--seed-pin --lockstep --no-turbo`, the same
  in-game trace `tests/scenarios/in-game-intro/trace-retail.jsonl`), hooking the
  per-sim-tick actor-update boundary `FUN_0046cd70` and snapshotting the LCG state
  word `DAT_008a4f94` there (new `rng` field in `retail_fields.json`, tagged
  with the deterministic `g_sim_tick`, reset at game_enter).  8644 in-game ticks
  common to both runs.
- **Result: `rng` matches 0/8643 sim-ticks.**  The shared stream is at a
  different phase at *every* in-game tick, despite the pinned seed + the
  deterministic sim-tick index.  (`a0_clip`/`a0_frame` matched 8643/8643 but
  TRIVIALLY — main-band slot 0 `+0x11e0` was inert all run, clip=0/frame=0; the
  `rng` divergence is the real signal.  An animating-actor slot was not
  isolated — a follow-up could re-point the chain to a known NPC slot, but the
  shared-stream result already settles the determinism question.)
- **Mechanism — proven at the anchors, not inferred.**  `prologue_enter`: BOTH runs
  on the IDENTICAL flip 946, yet rng differs (`0x84654e6f` vs `0xa79a2d6e`).  At the
  same flip the engine drew a different *number* of LCG values → a per-PRESENT
  consumer × the non-deterministic presents-per-tick count (quirk #75) desyncs the
  stream phase; it never re-converges.  (newgame_enter A@751 rng 0x6a239b8d / B@750
  rng 0x6a239c54; game_enter A@1432 0x84654e6f / B@1434 0xa79a2d6e.)
- **Why it's the actor band.**  `FUN_0054f980` draws this exact LCG `FUN_005bf505`
  ~40× per tick for idle-wait timers (`+0x5c`), the idle→wander branch pick, and
  wander move-offsets (→ `FUN_00450ef0`) — static two-witness.  A divergent stream →
  different waits/dirs/positions run-to-run = the #75-addendum ~6.7k-px residual.
- **CONCLUSION / the fix.**  An RNG-reading subsystem needs its OWN **RNG anchor**
  (snapshot+restore `DAT_008a4f94` at the game_enter sim-tick, both sides; or re-seed
  the actor RNG per tick) — the camera's `g_sim_tick` anchor is insufficient (it
  works only because the camera reads no RNG).  This makes the #75 "anchor each
  subsystem separately" decision MANDATORY (not optional) for the actor layer.  Port
  bar for the band: **data-1:1 given a matched RNG state** — retail-vs-retail isn't
  observed-1:1 here.  Tooling: `tools/rng_tick_diff.py RUN_A RUN_B`.  Engine-quirk
  #77; `findings/in-game-intro.md`.
- **DIRECTION (user, ckpt 73):** **defer ALL RNG-order parity** — it reaches 1:1
  later, once every in-scene RNG consumer is RE'd and we match consumption order
  (rng+rngcalls per anchor on both sides; the flow trace now carries `rngcalls`,
  the unified consumption signal — committed `4c587c0`).  **Do NOT build the
  actor-RNG anchor now.**  The next chips are **implementing all the VISUAL elements
  of this scene**; RNG-driven behaviour parity comes after the consumer census.
- **NEXT (visual elements, simplest first) — now with the blit trace pinpointing them:**
  0. **The town-frame blit diff (ckpt 74) confirmed the backdrop is pixel-faithful**
     (port 250 blits all matched retail on identity+geometry+state; 0 wrong draws).
     The missing 356 draws ARE the chips below — `render_diff --retail-frame 1500
     --port-frame 1200` names them.
  1. **The establishing-shot overlay = bank `0x583`** — **DONE (ckpt 75, see above):
     the producer is `0x48c150:124-162`, ported as `letterbox.{c,h}`; the 320 blits
     now match retail (356→36 diff).**
  2. **"Town of Tonkiness" banner + foreground tree/veg** — the `0x5a00c0`
     scripted-scene overlay player (draw-list `stack+0x98` stride-10; caption array
     `stack+0x3a4` stride 0x124 via font bank `DAT_008a7640`).  Also where the pan
     TRIGGER lives (`ingame-camera-pan`).
  3. **NPC actor RENDER + spawn** (present modes 0/1/2) — the entity/spawn foundation
     (`0x59f2c0` 8-slot init + `0x560e60`); render the actors even though their
     RNG-driven motion won't be observed-1:1 yet (deferred above).

## Where we are — ckpt 72

**The ACTOR ANIMATION cycle is RE'd end-to-end + the frame-stepper ported — and it
rides the existing sim-tick clock, so there is NO separate counter to pin.**  This
closes the ckpt-71 directed next ("RE the NPC/actor system + its animation cycle,
then pin its counter").

- **The UPDATE chain (per sim-tick), distinct from the render/emit pass.**
  `FUN_00439690:1108` calls `FUN_0046cd70(1)` once per sim-tick (when
  `*(param+0x1c)==0`).  `0x46cd70` is the actor-UPDATE master (not the render walk
  `0x48c150`): it walks the pools off `DAT_008a9b50` (active = `actor+0x1d0!=0`) and
  for the main band (`+0x11e0`, 0x80 slots) calls
  `FUN_0054f980(actor+0x40, actor+0x40, 0, 0)` for the primary render-state entry +
  `(entry-0x294, entry, 1, idx)` for each kinematic sub-entry.
- **`0x54f980` (11597 B) = the per-actor behaviour dispatch on `actor+0x1d4`.** It
  shadow-copies the render-state (the body-part chain), then every animating case
  runs the SAME inline frame-stepper on the render-state anim fields (`+0x6c` clip /
  `+0x70` timer / `+0x72` frame / `+0x74` done): `timer++`; at `>=clip.dur` →
  `frame++`,`timer=0`; at `>=clip.count` → loop (`frame=loop_to`) or one-shot hold
  (`frame=count-1`,`done=1`,`timer=1`).
- **The clip is a fixed 0x154-B 32-frame descriptor** (count@`+0x42`/dur@`+0x44`/
  oneshot@`+0x48`/loop_to@`+0x152`/base@`+0x00`/per-frame delta@`+0x02`/x@`+0x50`/
  y@`+0xd0`) — two witnesses: the stepper + the renderer `0x491ae0` case 0x1872d.
  Clip is (re)set on STATE CHANGE by `0x40afe0`/`0x41e600`, reset-on-change only.
- **PORTED (pure, host-tested bit-exact): `src/anim_clip.{c,h}`** —
  `anim_clip_advance` (the stepper) + `anim_state_set` (the change-gated set) +
  `anim_clip_sprite` (base+delta).  `anim_clip` pins the descriptor layout with
  `_Static_assert`.  **8 tests; 854 pass / 0 fail / 6 skip** (+6).  Both GUI builds
  clean (wildcard picks up `anim_clip.c`; unused for now — actors not yet driven).
- **DETERMINISM CONCLUSION:** `+0x70/+0x72` is a pure function of *(sim-ticks since
  clip-set)* — no GetTickCount/Flip/RNG — so it is already deterministic under the
  camera's `g_sim_tick` anchor (game_enter reset).  No new pin.  This REFINES the
  #75-addendum guess that the anim "reads a counter NOT the camera sim-tick".
- **OPEN (the real residual):** the #75 ~6.7k-px actor-band diff under sim-tick
  matching must be a DIFFERENT pillar — the RNG-driven behaviour (which clip plays /
  position).  `0x54f980`'s idle/wander cases draw the LCG `0x5bf505` for random
  waits + spawn offsets; clip-SET timing is downstream.  ANNOTATED for the check
  (`retail_fields.json` `0x46cd70`/`0x54f980` → `a0_clip/a0_timer/a0_frame`): a live
  capture across two sim-tick-matched runs should show `a0_frame` matching while
  `a0_clip`/position drifts.  Engine-quirk #76; `findings/in-game-intro.md` "The
  actor animation cycle".
- **NEXT:** either (a) live-confirm the above (drive retail twice seed-pinned, read
  the `a0_*` anim fields per sim-tick, diff) → pins the residual to the RNG pillar;
  or (b) move to a named visible layer (the cinematic LETTERBOX quirk #74, or the
  `0x5a00c0` banner/foreground-tree overlay player).

## Where we are — ckpt 70

**The intro-PAN camera is WIRED LIVE — the town backdrop now PANS; the scripted
target-setters ported.**

- **The easer is driven by a live camera in `main.c`.** A static
  `camera_view g_game_camera`: `enter_game` sets `map_w/h` (`dim·0xc80`) + the
  640×480 viewport and `camera_apply_snap(128000, 12800)` (spawn origin =
  `MAP_RENDER_CAM_TOWN_3F2`). `game_render` calls `game_camera_step` each frame:
  the `CALL_TRACE_BEGIN(0x43d1d0)` flow-trace mirror (the X-axis easer state per
  `retail_fields.json`) → `camera_follow_step` → `game_camera_to_mr` projects the
  view onto the `mr_camera` subset → the backdrop renders through the *current*
  scroll (replacing the static const). A hold timer fires the scripted pan.
- **The target-setters ported (`0x439690:599-664`).** `camera_apply_snap`
  (`+0x40` command: clamp tgt to `[0, map-vp]`, cap=0/flag=0, JUMP cur=tgt, zero
  vel — spawn positioning) + `camera_apply_pan` (`+0x4c` command: clamp + set tgt
  / cap=speed / flag=0, leave cur/vel — the easer eases). Host-tested bit-exact (2
  new tests). Referenced `0x439690` by **bare VA** (only the setter slice of the
  8866-B fn is ported — no ledger inflation).
- **Visually confirmed on the feed:** hold (cam x=128000, town right) → mid-pan →
  settled (cam x=12800, town left edge / half-timber house). Pan completes ~400
  frames after the hold timer.
- **Added** `MAP_RENDER_CAM_TOWN_3F2_SETTLED` (x=y=12800) — the determinate
  settled camera both sides share for a flip-anchored full-frame diff with NO
  easer in flight.
- **State (ckpt 70): 848 pass / 0 fail / 6 skip** (+2). Ledger **197/1490 touched
  / 192 tested** (unchanged — easer/shake counted ckpt 69; setters are a bare-VA
  slice of `0x439690`). Both GUI builds clean.
- **CADENCE + TRIGGER MEASURED (ckpt 70b) → the pan is TRAJECTORY-1:1.** A retail
  field-spec trace (`--seed-pin --lockstep --no-turbo`, easer `0x43d1d0` + Flip
  hooked, contiguous Flip whitelist) pinned both: the easer fires **once per 2
  Flips** (the sim runs at half the Flip rate; `cam_x60` is a STEP function,
  −300/2flips cruise) and the pan command fires at **`game_enter + 184` Flips**
  (Flip 1616 HOLD, 1617 PAN). `game_camera_step` now gates the sim to every 2nd
  frame (`hold & 1`); `GAME_CAMERA_HOLD_FRAMES=184` (even, so trigger Flip = sim
  tick). VERIFIED: the port passes through the **identical `cam_x60` sequence** as
  retail (128000,127990,127970,…,−300/2flips — diffed the captured `0x43d1d0`
  mirror). RESIDUAL (PORT-DEBT `ingame-camera-pan`): a ~2-3 Flip startup-jitter
  PHASE (retail's wall-clock sim accumulator — a 4-Flip plateau at 1618-1621 a
  clean 2:1 step can't reproduce; ≤1 step ≈ 3px, transient, zero at hold+settled)
  + the cutscene-script TRIGGER source + the spawn-snap origin derivation — all
  downstream of the in-game sim / `0x5a00c0` port. Writeups:
  `findings/in-game-intro.md` "The camera is WIRED LIVE" + "The pan CADENCE +
  TRIGGER measured".

## Where we are — ckpt 69

**The intro-PAN camera EASER located + ported bit-exact; a HW-watchpoint tool +
the annotation methodology reinforced.**

- **The pan is SCRIPTED** (not leader-follow). Live camera-field probe across the
  establishing shot (`game_enter@1434`→3600): the target x snaps to a FIXED
  **12800** (4 cells, town's left edge) + speed **300** once at hold-end
  (~flip 1617, ~183 flips after entry); Y never moves (`+0x5c`=`+0x70`=12800).
- **The easer = `FUN_0043d1d0`** (called from `0x439690:1123`, before `0x499ab0`
  shake/HUD). Per axis: `dist=|tgt-cur|`; `if vel<dist: cur ±= vel(+far-boost
  when flag&&dist>16000); vel=min(vel+10,cap)`; `else cur=tgt (snap);
  vel=max(vel-10,0)`. cap = `+0x20` (=300). Town pan has flag(`+0x1c`)=0.
- **Found via a HARDWARE WATCHPOINT** — it's dispatched through a heap function
  pointer (invisible to static search). New `tools/mem_watch.py --watch-chain
  ROOTVA:HOPS:OFF:SIZE[:LABEL[:ARM_AT_FLIP]] --hw` resolves the view's heap
  `+0x60` and DR-watches it (frida-17 per-thread API, OpenMare pattern). One run:
  1189 writes, single writer insn `0x43d26d`, trajectory 127970→12800; the
  per-tick deltas 30,40,…,300 pin the formula. (MemoryAccessMonitor livelocks on
  the hot view page — the `--hw` path is the fitting tool.)
- **PORT (pure, host-tested bit-exact):** `src/camera_follow.{c,h}` —
  `camera_follow_axis`/`camera_follow_step` (`FUN_0043d1d0`) + `camera_shake_apply`
  (`FUN_0043d340`). 6 tests validate the captured trajectory, the +10/cap-300
  ramp, exact landing, the flag-gated far-boost, shake-inactive=0. **846 pass / 0
  fail / 6 skip.** Ledger **197/1490 touched / 192 tested** (+`0x43d1d0`,`0x43d340`).
- **ANNOTATED** (the user's directive): a named `camera_follow_step` (0x43d1d0)
  entry in `retail_fields.json` with the view fields incl. the now-known
  `vel_x/vel_y` integrator + the formula; the view struct's fields are named at
  their retail offsets in `camera_follow.h`. Port `CALL_TRACE_BEGIN(0x43d1d0)`
  mirror pending the live-camera wiring.
- **METHODOLOGY (reinforced, CLAUDE.md "Annotate as you RE"):** "annotate" = the
  flow-trace field spec (`retail_fields.json` named functions+fields + port
  `CALL_TRACE_BEGIN` mirrors) — CORE step of finishing any RE/port; thiscall/struct
  tagging is a SEPARATE static-readability lane; never an ad-hoc symbol-rename.
- **NEXT (`ingame-camera-pan`):** wire the stepped `camera_view` into
  `main.c game_render`/`game_drive` (replace the static `MAP_RENDER_CAM_TOWN_3F2`)
  + RE the scripted op that sets tgt=12800/speed=300 at hold-end. Then a
  flip-anchored full-frame backdrop/sky diff is meaningful. Writeup:
  `findings/in-game-intro.md` "The camera EASER located".

## Where we are — ckpt 67

**In-game COLOR-GRADE LUT ported → backdrop TILES are `differ_px==0`; the
"establishing shot" proven to be a PAN, not a zoom.**  Drove the menu→in-game
nav trace, diffed the port's town vs a *fresh new-trace* retail hold golden, and
chased the first divergence (the principled "stop at the first divergence, port
the missing thing" loop).

- **Establishing shot = leftward PAN at constant 1:1 scale** (overturns the
  ckpt-65/66 "zoom" framing).  Live-probed flips 1440–2100: viewport `+0x64/+0x68`
  and shear `+0x74` constant; only `+0x60` pans (128000 hold → 59450 by 2100).
  Free-roam render path every frame (`0x490cd0` fires; offscreen/special
  `0x499100`/`0x48c6b0` never); projector `0x490b90` has no scale term.  Port's
  static `MAP_RENDER_CAM_TOWN_3F2` aligns with the golden at **dx=0, same ~64px
  wall pitch**.  PORT-DEBT `ingame-establishing-zoom` **retired**.
- **The missing colour = an in-game per-channel tone-curve LUT** (`DAT_008a9410`),
  built by `FUN_00562ea0` (`0x5639fd-0x563a70`, a cosine curve over two config
  gates) and applied by `0x417c40` (parallax) + `0x490f30` (tiles); the
  title/menu/prologue use the plain getter `0x418470`, so they stay bit-exact.
  It is **NOT** the per-sprite tint (`DAT_008a93fc==0`, identity — ruled out
  live).  Builder **verified bit-exact** vs a live `DAT_008a9410` probe
  (`LUT[64]=35`/`128=100`/`192=175`); gates live-probed `gate1=700 gate2=850`.
  **PORT (`src/color_grade.{c,h}`, host-tested):** `color_grade_build_lut`
  (the formula) + `color_grade_apply_palette` (`0x417c40`'s per-channel RGBQUAD
  remap) + `color_grade_is_active`.  Wired in `main.c`: `enter_game` arms the
  grade before the town banks decode; `title_sheet_format` applies it to each
  **8bpp** sheet's palette *before* the 16bpp pack (retail's order → bit-exact,
  not LUT-after-565).  Scoped so the title sheets (converted earlier) stay
  identity.  **Result: the half-timber wall `(173,170,140)` and ivy
  `(107,105,74)` match retail exactly.**
- **RESIDUAL (open):** the **24bpp parallax** banks (`0x55`/`0x58`/`0x59`, sky+
  mountains) have no palette → the 8bpp grade skips them → the sky still renders
  too bright.  Retail must grade 24bpp by a different path (TBD) AND the port's
  24bpp→16bpp decode is itself brighter than retail's (port raw sky `132,186,255`
  vs back-solved retail raw `~103,165,231`).  PORT-DEBT `render-palette-tint`
  (sharpened: tile half done, 24bpp half + `color-grade-gates` derivation remain).
  Other residuals: NPC actors (blocked on entity/spawn), tree + "Town of
  Tonkiness" banner (`0x5a00c0`), the pan itself (`ingame-camera-snap`).
- **State (ckpt 67): 840 pass / 0 fail / 6 skip** (+4 color_grade).  Ledger
  **194/1490 touched / 189 tested** (+1: the `0x417c40` LUT slice is now
  host-tested).  Both GUI builds clean.  Full writeup:
  `findings/in-game-intro.md` "The in-game COLOR-GRADE LUT".

### (prior, ckpt 66) The PARALLAX far-plane

**PARALLAX FAR-PLANE landed (sky + mountain background).** On top of the ckpt-65
wired backdrop, the port now draws the **parallax far-plane** behind the tiles —
live-verified in-game (port `game_enter@1116`): frame 1200 shows the blue sky band
(layer A bank `0x55`) + the mountains (layers C/B banks `0x58`/`0x59`) under the
town tiles, where it was black before.

- **RE (two-witness, high confidence).** The background producer is `FUN_00490cd0`
  (inline; called FIRST in the per-frame world driver `0x48c150:47`, the free-roam
  path) and its twin `0x499100`→`FUN_00499560` (the establishing-shot/special path
  via `0x48c6b0`).  Both read the SAME 3-layer descriptor from the runtime grid's
  **front-header** (`*(DAT_008a9b50+0x1048)`) via select+blit `0x417c40`→`0x5b9a40`.
  The descriptor is written by the `0x587e00` PROLOGUE's `switch(param_2=room[0x44])`
  / `param_3=room[0x43]`; town (room 210110, area `0xd2`: A=4,C=1) → case 4 → A bank
  `0x55`; C bank `0x58` baseY `0xf8` wrap 8 paraY `0xfa` (0.5×); B bank `0x59` baseY
  `0xe0` wrap 8 paraY 0 (0.25×).  Full writeup: `findings/in-game-intro.md` "The
  PARALLAX far-plane".
- **PORT (pure, host-tested): `src/parallax.{c,h}`** — `parallax_select` (the
  prologue switch), `parallax_render`/`parallax_strip` (`0x490cd0`/`0x499560` math),
  `parallax_to_grid`/`_from_grid` (front-header bytes).  Wired into `town_render`
  (`town_render_parallax`, descriptor selected at load, drawn before the tilemap)
  and `main.c game_render` (sink = `game_parallax_blit` → `zdd_object_blt_onto`).
  9 host tests (8 `test_parallax.c` + 1 `town_render` wiring).
- **Fidelity boundary:** the port uses the plain frame getter `0x418470` (as the
  tiles do) where retail selects via the palette-aware `0x417c40` — the far-plane
  renders with the base palette (time/difficulty tint deferred, PORT-DEBT
  `render-palette-tint`).  Town params (4,1) are hardcoded in `town_render`
  (PORT-DEBT `ingame-nontile-layers`: derive from `game_map`/`game_world`).
- **LIVE-CONFIRMED bit-exact** (retail `--parallax-probe`, the re-synthesised
  trace, `game_enter@1433`): the descriptor `raw32` + the per-tile blit stream
  match the port's `parallax_render` byte-for-byte (incl. layer C y=220 = the
  clamped vertical parallax) → **data-1:1 at the producer**, `MAP_RENDER_CAM_TOWN_3F2`
  confirmed.
- **State (ckpt 66): 836 pass / 0 fail / 6 skip** (+9). Ledger **193/1490 touched /
  188 tested** (+2: `0x490cd0`, `0x499560`). Both GUI builds clean.

### (prior, ckpt 65) The wired backdrop

The backdrop pipeline is **composed (`town_render.{c,h}`) + WIRED into `main.c`**,
rendering the opening **town of Tonkiness backdrop** — the half-timbered house, the
vine trellis, the stone-block walls, ivy + grass — the **same assets at the matching
gameplay scale as the retail golden** (user-confirmed; cross-checked vs golden flip 1800).

- **The composition (pure, host-tested): `src/town_render.{c,h}`.** A thin
  per-room SCENE owning the shared state (parsed `map_data`, the runtime grid,
  the 27-layer `draw_pool`) run in engine order: `town_render_load` =
  `map_data_parse` (`0x587970`) + `map_decode` (`0x587e00` arms);
  `town_render_step` = the backdrop slice of the per-frame driver `0x48c150`
  (`draw_pool_reset` → `map_render_walk` `0x490f30` → `map_present` `0x48eac0`).
  6 host tests (`tests/test_town_render.c`).
- **The Win32 glue (`main.c`).** `load_town_scene(1022)` in `enter_game`:
  `LoadLibraryExA("sotes.exe", AS_DATAFILE)` → the EXE `.rsrc` (the engine-time
  module `DAT_008a6e7c`), `FindResource`/`Lock`(DATA 1022) + `town_render_load`.
  **Live-verified the packed `sotes.exe` `.rsrc` is readable** (Steam-DRM intact;
  no runtime Steamless): DATA 1022 = 152936 B "MSD_SOTES_MAPDATA" 88×19×3.
  The three engine globals are real callbacks: `game_sprite_resolve`
  (`ar_pool_get_slot(bank)` = `&DAT_008a760c[bank]` + `ar_sprite_slot_frame` =
  `0x418470`; bank→pool mapping verified: bank `0x62`→idx 85→res `0x433`, all
  town banks in g5), `game_bank_dims` (slot width/height), `game_present_blit`
  (mode-3 CLIPPED → `zdd_object_blt_clipped` `0x5b9bf0`). `game_render` clears
  black then walks `town_render` through `MAP_RENDER_CAM_TOWN_3F2`.
- **NOT `differ_px==0` yet — named residuals, ALL deferred layers (not logic):**
  the parallax sky/mountain far-plane + foreground trees + dialogue/caption
  overlay (`0x5a00c0`, PORT-DEBT `ingame-nontile-layers`); the NPC actors
  (present modes 0/1/2, PORT-DEBT `present-actor-modes`); retail's zoomed-out
  intro establishing shot at the hold (PORT-DEBT `ingame-establishing-zoom` — the
  camera scale field wasn't in the ckpt-64 probe); and the per-sprite palette
  tint (`render-palette-tint` — the "bit more color" the user noticed, the
  `DAT_008a93fc`/`0x4182d0` difficulty/time ramp, recolors pixels not geometry).
- **State (ckpt 65): 827 pass / 0 fail / 6 skip** (+6 town_render). Ledger
  **191/1490 touched / 186 tested** (pure composition, no new `FUN_`). Both GUI
  builds clean. The backdrop scene is now **driven by `main.c`** (the first
  in-game render module that is).

### (prior, ckpt 64) The camera/view object

- **RE — the camera IS the view object** (`view = *(room_state+0x104c)`, one
  `operator_new(0x78)` struct, allocated in the room-state ctor `0x4017d0:187`).
  Its room-entry init is clean + portable (`586010:854-872` sets viewport
  `+0x64=64000`/`+0x68=48000`, origins `+0x5c/+0x60/+0x74=0`; the two `587d30`
  calls zero the `+0x24`/`+0x3c` sub-blocks holding `+0x34`/`+0x4c`). So the
  ckpt-63 "dynamic-scroll rock, no clean pure init" framing is **refuted**.
- **Live ground truth (the harness, `src:"chain"` field-spec probe).** Added a
  global-deref field src (`*(*(0x8a9b50)+0x104c)+off`) + 9 `cam_*` fields to
  `retail_fields.json`; drove retail to the town twice (`--seed-pin --lockstep`).
  The camera **snaps to `+0x60=128000` (40 cells) / `+0x5c=12800` (4 cells) by
  flip 1093, holds ~83 flips through ~1176** (the town first renders ~1150,
  inside this hold), then runs a **scripted leftward pan** (~−300/flip cruise).
  Viewport matches the static init exactly → the 586010 RE is confirmed.
- **PORT (pure, host-tested):** `map_render_camera_init` (the room-entry zeroed-
  origin state) + the live-verified first-frame constant `MAP_RENDER_CAM_TOWN_3F2`
  (`+0x60=128000`, `+0x5c=12800`, vp 64000×48000; visible window cols 39-60 /
  rows 3-18), both in `src/map_render.{c,h}`. 2 tests. **DEFERRED** (PORT-DEBT
  `ingame-camera-snap`): the spawn-snap that derives the origin from the entry
  params + the intro pan (the dynamic-scroll engine across `0x4710c0`/`0x54f980`
  follow/copy + `0x499ab0`→`view+0x74`).

### (prior, ckpt 63) The in-game PRESENT PASS
**The decode → grid → geometry → draw-list → present chain is CLOSED.**

- **RE — `FUN_0048eac0` is the present pass; `FUN_00490b90` the shared projector.**
  The per-frame driver `0x48c150` resets the 27-layer table (`view+0x54` counts, ==
  `draw_pool_reset`), runs all the per-actor + tilemap emitters (`0x490f30` at :108),
  then calls **`0x48eac0`** to flush. `0x48eac0` walks the 27 layers in order (count at
  `view+0x58`); per node it dispatches on **mode (`+0x18`)** into 4 arms, each projecting
  the node's world pos to screen + culling, then blitting: **mode 0** → `0x5b9b70`,
  **mode 1** → `0x5bd550` (alpha), **mode 2** → the `DAT_008a9274`-palette scaled path,
  **mode 3** → `0x5b9bf0` (clipped color-key) when node `+0x14`==0 else `0x5bd550`. The
  shared projector **`0x490b90`** (used verbatim by modes 1/3, inlined by 0) computes
  `sx = wx/100 - (cam+0x60 + cam+0x34)/100 + offx`, `sy = wy/100 - (cam+0x5c +
  cam+0x74*100 + cam+0x4c)/100 + offy` and a four-corner cull vs `cam+0x64/100` /
  `cam+0x68/100`. `map_render_walk` emits **mode 3, param8=0** → the clipped path.
- **PORT (pure, host-tested):** `src/map_present.{c,h}` — `map_present_project` (`0x490b90`
  arg-for-arg) + `map_present` (the 27-layer walk + mode dispatch). **Mode 3 fully**
  (project w/ node w/h, select CLIPPED/ALPHA by `+0x14`); the cel handle in node `+0x00`
  → a `present_blit_fn` sink (the Win32 layer maps `PRESENT_CLIPPED`→`zdd_object_blt_clipped`,
  `PRESENT_ALPHA`→`zdd_blit_orchestrate`, both already in `zdd.c`). **DEFERRED**
  (PORT-DEBT `present-actor-modes`): modes 0/1/2 — VISITED in faithful order + counted via
  `out_deferred`, not blitted (no ported producer emits them; geometry reads engine sprite
  internals). 9 tests (`test_map_present.c`).

### (prior, ckpt 61) The draw-node layer pool + the backdrop walk driver

- **RE — the layer table is one structure shared by `0x4917b0` + `0x586010`.**
  `FUN_00490f30`'s `0x4917b0` enqueue writes into the render context's DRAW-NODE TABLE at
  **`view + 0x54`** (`view = *(room_state + 0x104c)` — the object `0x490f30` takes as
  `param_1`). `0x586010:510-650` builds it: `operator_new(0xd8)` = **27 (`0x1b`) 8-byte
  layer slots** `{u16 count, u16 cap, ptr node[cap]}`, each given its own
  `operator_new(cap*0x3c)` node array; the 27 caps are literal-stamped (layer1=`0x80`,
  layer2/3=`0x1b8`, layer6=`0x400`, …). **Slot 0 is never given an array (cap 0) → every
  emit to layer 0 fails** — a real quirk, preserved. Present walks the 27 layers in order,
  so the layer index = the draw-order key. `0x4917b0` (106 B) = per-layer bump alloc:
  `node = layer[key & 0xffff]; if (cap <= count) return 0;` else stamp 6 caller dwords
  (`+0x00` sprite, `+0x04/+0x08` dst, `+0x0c/+0x10/+0x14` aux, `+0x18` mode), bump
  `count`, return the node for `490f30` to finish (`+0x2c..+0x38` src rect); the
  `CONCAT22` high-word sort key is masked off (dead in the allocator).
- **PORT (pure, host-tested):** `src/draw_pool.{c,h}` — `draw_pool_init`/`_reset`/`_free`
  (the 27-layer table; `draw_pool_default_caps[]` verbatim) + `draw_pool_emit`
  (`0x4917b0` arg-for-arg; node is exactly 0x3c B, asserted). `map_render_walk` (added to
  `map_render.{c,h}`) — the backdrop-tile core of `490f30.c:55-229`: visible window,
  scan rows-outer/cols-inner, per populated region-A sub-slot resolve the sprite +
  `draw_pool_emit` a node (layer = region-A `+0x4`, mode 3, dst = tile world origin,
  `0x20×0x20` src rect). The sprite manager (`0x418470`/`&DAT_008a760c`) is an
  `mr_sprite_fn` **callback** so the walk stays pure; tile skipped when the resolver
  returns 0. **DEFERRED:** palette tint (`DAT_008a93fc`/`0x4182d0`) + the region-C
  blend/overlay arms (`0x1b58d`/`0x1b5ab`, `490f30.c:230-282`) — registered in
  `port-debt.md`.
  (`0x586010` referenced by bare VA — only its layer-table slice is ported, so the 18 KB
  fn isn't over-counted.) Full writeup: `findings/in-game-intro.md` "The draw-node layer
  pool + the backdrop walk driver".
- **State (ckpt 64):** **821 pass / 0 fail / 6 skip** (+2: camera init + first-frame).
  Ledger **191/1490 touched / 186 tested** (unchanged — the camera init is a slice of the
  bare-VA-referenced `586010`/`587d30`, no new `FUN_` token). Both GUI builds clean; all
  the in-game render modules are in the `src` wildcard but **not yet called by `main.c`**.

## Next move
> The 60-second framing is in `FRONT.md`; this is the detail.

**Camera is wired + pans (ckpt 70); tiles + 24bpp sky matched (ckpt 68, user-confirmed).**
The smallest visible wins, in order:
0. **PAN backdrop diff DONE — verified pixel-1:1 (ckpt 70b).** Captured fresh retail
   pan frames (`--no-turbo --seed-pin --lockstep`) + their `cam_x60`, matched port
   frames by `cam_x60` (sim'd the port camera → port Flips 1304/1344/1384/1422/1462 ↔
   retail 1617/1660/1700/1740/1780, shared cam 127990/125690/120050/114350/108350),
   diffed: **backdrop Δ0** (shift-search sharp min at dx=dy=0; pan-start x=80 col all
   Δ0). Residual = named missing layers ONLY. Parity-ledger #7. **NEW (quirk #74):** the
   establishing shot is **LETTERBOXED** — black bars rows 0-63 + 416-479 (640×352
   window); the "dark top" the user flagged, plus a matching bottom bar; scene-scoped
   (absent in settled play), likely a `0x5a00c0` overlay.
   **Next chips (named layers, simplest first):** the LETTERBOX (quirk #74), then the
   banner + foreground tree (`0x5a00c0`), then the NPC actors (entity/spawn system).
   The settled-end diff (`MAP_RENDER_CAM_TOWN_3F2_SETTLED`, x=y=12800) remains available
   for a no-cadence-question full-frame check.
1. **The 24bpp parallax colour — DONE (ckpt 68).** Retail grades 24bpp banks at
   **DECODE**, not via the palette: `0x417c40` early-exits to the plain getter for
   a palette-less bank, but its **flag-3 branch** (the 24bpp case) first stamps the
   slot's brightness descriptor (`f_08=1`, scales `f_0c/f_10/f_14`=1000 for tint
   case 0, `f_18`=the LUT base when armed); the lazy `ar_sprite_decode` then runs the
   already-ported `ar_sheet_decode_pixels` (LUT-then-scale, magenta skipped). The
   port's parallax sink used the plain getter and never stamped it →
   `game_arm_parallax_grade()` in `main.c` now replicates the stamp in
   `game_parallax_blit`. Verified raw sky `(66,150,255)`→LUT→565=`(33,125,239)`,
   blue `239` == retail main band; **user-confirmed correct on the feed**. The earlier
   "dark top gradient" was camera/scene-dependent (the unported pan revealing a darker
   sky-texture band), **not** an overlay. (NB the old `(132,186,255)`/`(103,165,231)`
   numbers were wrong — actual raw is `(66,150,255)`.) Remaining: a true row-for-row
   sky `differ_px==0` waits on the intro PAN so the cameras align.
2. **The actor renderers** (`0x491ae0` et al.) → present **modes 0/1/2** (the NPCs).
   **BLOCKED:** `0x491ae0` reads a fully-populated entity object from the actor
   pools off `DAT_008a9b50` — the entity/spawn system isn't ported yet (the upstream
   `0x59f2c0` 8-slot actor init + `FUN_00560e60`). Port that foundation first.
3. **The "Town of Tonkiness" banner + the foreground tree** (`0x5a00c0`, the
   scripted-scene overlay player + the `DAT_008a7640` font bank) — PORT-DEBT
   `ingame-nontile-layers`.
4. **The intro PAN — WIRED LIVE (ckpt 70).** The easer + the `0x439690` target-setters
   are ported and stepped each frame; the town pans hold→settled (feed-confirmed).
   REMAINING (PORT-DEBT `ingame-camera-pan`): the pan TRIGGER (the 183-frame hold timer
   stands in for the `0x5a00c0` cutscene-script op — RE'd together with item 3, the
   scripted-scene overlay player) + the easer step CADENCE (port per-frame vs retail
   per-sim-tick; the −147/flip cruise vs cap-300/tick gap → correlate tick↔flip via a
   live trace before claiming a flip-anchored pan diff mid-flight).

**HARNESS — in-game retail drive RESTORED (ckpt 66).** The old `trace-retail.jsonl`
had gone stale (retail's title turns interactive ~150 flips later than it used to, so
the old `Start@615` was eaten and retail sat on the title). Re-synthesised a working
trace (confirm-spam 600..760 → new-game; down×2+confirm → prologue; Z-beats → in-game):
VERIFIED `newgame_enter@750 / prologue_enter@945 / game_enter@1433`. The
`--parallax-probe` then live-confirmed the parallax descriptor + blit stream **bit-exact**
vs the port (see the parallax section). Caveat: the working trace tolerates a stray
confirm landing on the difficulty menu (the down×2 recovers); robust across 3 runs.
NOTE (separate, user-reported): the **PORT** does not take **real keyboard** input when
run interactively (arrows/Enter do nothing) — only `--input-trace` replay drives it;
the windowed DInput/`GetDeviceState` path needs fixing (next task).

**Before a flip-anchored full-frame diff** vs `runs/tas-ingame-1`: pin the
**establishing-shot/zoom** relationship (PORT-DEBT `ingame-establishing-zoom`).
Retail's flip-1150 hold is a zoomed-OUT vista that zooms to 1:1 by ~1800; the
port renders gameplay 1:1 at the hold's scroll origin. So port + golden don't
share a camera at any single flip yet — the backdrop tiles are confirmed by ASSET
+ SCALE match (vs golden 1800), not by a px-exact frame diff. Find the view scale
field (or the `0x5a00c0` overlay projection) that drives the establishing shot.

How to drive the port in-game live (run INSIDE `nix develop` so
OPENSUMMONERS_GAME_DIR is set — else sotesd.dll fails → BLANK render):
`--input-trace tests/scenarios/in-game-intro/trace-port.jsonl --frames 1400`
(`game_enter@1116`), `--capture-frames "1160,1200,1300"` → BMPs in the game dir →
PNG → feed. CLI file paths (`--input-trace`/`--call-trace`) are now absolutized
before the game-dir chdir (main.c `resolve_launch_path`), so a repo-relative path
Just Works — no need to copy the trace into the game dir, and `--call-trace`
writes to (and logs) the launch CWD. The backdrop renders from `game_enter`.

## Module inventory — render + text pipelines complete; in-game data layer ported (not wired)
**Title/menu shell (bit-exact):** pixel_drawer, asset_register, bitmap_session, wnd_proc,
zdd, cs_dispatch, app_pump, title_scene (`0x56aea0`), input (`0x43c110`), obj_container,
menu_list, title_render, title_sink, title_drive, rng (LCG `0x5bf505`/`_5bf4fb`),
title_particles (phase-7 sparkle), app_flow (post-title dispatch).
**Text pipeline (bit-exact):** glyph_text (`0x40fa00`/`0x40fd20` layout builder),
glyph_render (`0x48e200` GDI render), glyph_wrap (`0x40e5e0`/`0x40f040` tooltip
word-wrap).
**New-game config scene (bit-exact + user-confirmed):** newgame_menu (`0x564780` case
0x24 builder), newgame_scene (run-loop model), newgame_box (`0x48cf80` 9-slice panel),
newgame_cursor (`0x48d940` selection cursor), newgame_picker (`0x567ba0` option submenu).
**In-game (milestone 2 — pure + host-tested; the backdrop chain is now WIRED into
`main.c` via `town_render`, ckpt 65):**
game_drive (the in-game run-loop shell), **anim_clip** (the actor animation cycle:
the per-sim-tick frame-stepper `0x54f980` + clip-set `0x40afe0`/`0x41e600` + the
0x154-B 32-frame clip descriptor — ckpt 72, pure + host-tested; not yet driven,
lands with the actor/entity system), **actor_render** (the town ACTOR render side,
ckpt 77 — `actor_render_describe` = `FUN_0044d160` static-actor descriptor +
`actor_render_static` = the `0x491ae0` default arm; pure + host-tested, not yet
WIRED — blocked on the spawn; the `0x1872d` animated arm + spawn + wiring follow),
**camera_follow** (the per-frame camera
ease-to-target `0x43d1d0` + shake sub-applier `0x43d340` + the `0x439690` SNAP/PAN
target-setters `camera_apply_snap`/`_pan`, ckpt 69-70 — pure + host-tested bit-exact;
**WIRED LIVE into `main.c game_render` ckpt 70** — `g_game_camera` stepped each frame,
the town pans hold→settled), **town_render** (composes the backdrop:
`map_data_parse`+`map_decode` load → `draw_pool_reset`+`map_render_walk`+`map_present`
step + `town_render_parallax` → `parallax_render` — driven by `main.c game_render`),
**parallax** (the sky/mountain far-plane `0x490cd0`/`0x499560` + the `0x587e00`-prologue
bank-selection `parallax_select`, ckpt 66), game_world (registry + `0x585000` xref +
`0x561c90` lookup over generated `world_tables_data`), game_map (`0x59f2c0` fresh-entry
arm + `0x4c5350` `0x3f2`→room-210110 key), **map_data** (`0x587970` resource parse),
**map_grid** (runtime render grid + `0x54c970`/`0x58ca80`/`0x58c910` write primitives),
**map_decode** (`0x587e00` per-tile-id placement dispatch — the 9 town tile ids),
**map_render** (`0x490f30` geometry + `map_render_walk` + the camera init `586010:854-872`
/ first-frame constant `MAP_RENDER_CAM_TOWN_3F2`), **draw_pool** (the 27-layer
draw-node pool `0x4917b0`/`0x586010`), **map_present** (`0x48eac0` 27-layer flush +
projector `0x490b90`, mode-3 backdrop path → ported zdd blits). The decode → grid →
geometry → draw-list → present chain is complete + the camera is RE'd; the `0x586010` sim
slice + `main.c` wiring (sprite resolver, EXE-NULL banks) are what remain to drive it with
real data.

## Tooling — Phase B B3 (DDraw blit + state trace) LANDED 2026-06-06
The RENDER drill-in is built + cross-side-verified (`findings/ddraw-blit-trace.md`):
`render_diff.py` names the wrong DRAW (`[sprite]`/`[decode]`/`[rect]`/`[state]`),
`flow_diff.py` names the wrong LOGIC — the two-drill-in coverage requested before NPCs.
- `src/render_id.{c,h}` (host-tested, 7): cel→`(resource_id, frame)` registry (openrecet's
  `tex_name` trick) + `dhash` FNV-1a fingerprint of the DECODED sheet (the improvement: a
  software blitter has CPU pixels at decode → catches RIGHT sprite / WRONG decode). Registered
  at `ar_sprite_slice` (port) / the resolver `0x418470` hook (retail, `installRenderIdHook`).
- Port emits at the 5 blit primitives via `zdd_emit_blit` (`src/zdd.c`); rides the existing
  `call_trace` transport. Retail: blit VAs in `retail_fields.json` + two new agent field
  sources `renderid`/`thisderef` (auto-install, no flag — the `rngcalls` pattern).
- `tools/render_diff.py` (host-tested, 9): aligns each frame's blits by `(va, res, frame)`,
  classifies first divergence; intersection-only field compare (port-only fields never
  false-flag); positional fallback for unnamed cels.
- VERIFIED live (title): retail `res=0x91b` == port `res=0x91b`; ECX/arg reads correct;
  render_diff named all 59 title-phase blits. **Footguns hit:** the port loads `--input-trace`
  BEFORE the game-dir chdir (use an ABSOLUTE path), and `--call-trace` opens its file in the
  launcher CWD (not the game dir); run the launcher INSIDE `nix develop` or sotesd.dll fails
  to load (err 126 → blank render). **Next layers:** retail decode-hash (so `[decode]` fires
  cross-side), the cdecl `0x5bd550` retail spec, a same-scene aligned in-game diff.

## Tooling — Phase B B2 (field-bearing flow trace) LANDED 2026-06-05
The LOGIC drill-in is built + **live-verified on retail** (`docs/plans/trace-tooling-phase-b.md`):
- `src/call_trace.{c,h}`: `seq` (per-frame exec order) + `CALL_TRACE_BEGIN/FIELD/END`
  (`I32/U32/F32/HEX`, + `_STUB`). `tools/flow/retail_fields.json` is the retail spec; the
  Frida agent reads `src: global|arg|argderef` (`retval` = onLeave TODO) into `f:{…}` with a
  per-Flip seq; `frida_capture.py --field-spec[-only]` auto-hooks spec VAs (bounded mode).
  `tools/flow_diff.py` (+ `test_flow_diff.py`, 9 tests) names the first `[chain]`/`[data]`
  divergence; `--field-timeline` localizes per-field state drift.
- **First probe:** `rng` (`DAT_008a4f94`) at the **Flip `0x5b8fc0`** — the shared once-per-
  frame VA. The title runner `FUN_0056aea0` keeps its loop INTERNAL (onEnter once, not per
  frame) so it was the wrong join VA; the Flip is right.
- **NUANCE (next session, don't trip on it):** the port emits `0x5b8fc0` from *two* sites —
  `src/main.c drive_present` (the rng `BEGIN`, runs every frame) and `src/zdd.c:894`
  `CALL_TRACE_ENTER` (the real `zdd_present`, the bare call-coverage probe). Under
  `--hide-window` (always used for parity) `zdd_present` is SKIPPED, so only the rng probe
  fires → clean 1 row/frame. A *non*-hidden run would show 2 rows/frame at `0x5b8fc0`.
- **First result:** title-sparkle RNG is **data-1:1** (port & retail both land on
  `0x404a0a8f`); the per-flip divergence is the R3 title-pace (phase) skew — port anchor
  `subtitle_anim_start` @flip 437 vs retail @897, sparkle compressed into fewer port flips.
  Not a logic bug. Anchor+rate (pace-aware) alignment is the refinement when chased to px.

## How to run / verify live (self-serviceable — Frida host always up, UAC auto-approved)
```
# build (single-TU, full rebuild) + host suite, inside nix develop:
nix develop --command make -C src all && nix develop --command make -C tests run   # 806 pass / 0 fail / 6 skip

# capture port frames (BMPs land in the game dir = Windows C: drive):
cp build/opensummoners-debug.exe /tmp/oss.exe
./build/opensummoners-launcher.exe --timeout-ms 35000 -- /tmp/oss.exe \
    --hide-window --frames 2200 --capture-frames "60,200,400,700"
# then BMP->PNG (PIL, in nix develop) from /mnt/c/.../Fortune Summoners/port_frame_*.bmp

# B2 field-bearing flow trace (the LOGIC drill-in) — retail + port, then diff:
#   retail (bounded: hook ONLY the field-spec VAs, use ABSOLUTE /tmp paths):
OPENSUMMONERS_DURATION_MS=35000 nix develop --command bash tools/run-retail.sh \
    --no-turbo --hide-window --seed-pin --call-trace --field-spec-only \
    --call-trace-frames 900,950,1000,1050 --run-dir /tmp/b2live --exact-run-dir
#   port (drive_present emits rng at the Flip 0x5b8fc0 every frame under --hide-window):
./build/opensummoners-launcher.exe --timeout-ms 80000 -- /tmp/oss.exe \
    --hide-window --frames 1200 --call-trace /tmp/port_ct.jsonl --call-trace-frames 900,950,1000,1050
#   diff (default = per-frame seq-aligned chain+data walk; --field-timeline = per-field):
nix develop --command python3 tools/flow_diff.py \
    --retail /tmp/b2live/call_trace.jsonl --port /tmp/port_ct.jsonl --all
# NB align on an ANCHOR not the raw flip index (title-pace skew: port anim_start@437 vs
# retail@897) — the rng field is data-1:1 (both end 0x404a0a8f) under correct alignment.
```
NB Flip frames advance ~1 per 2 main-loop iterations (pace split), so reaching Flip 700
needs a generous `--frames`/timeout. Retail-side capture + the anchor-aligned pixel diff:
`docs/parity-harness.md`. The per-ckpt probe flags (`--cursor-probe`/`--fade-probe`/
`--pace-probe`/`--seed-pin`/`--textout-probe`/`--menu-trace`) are catalogued in
`PROGRESS.md` and get folded into the unified `scenario-test.py` in the Phase-B harness
work (`docs/plans/`).

## Open RE threads (see ROADMAP subsystem map for the rest)
- **The render rock's deferred arms** (`port-debt.md`): the sprite-resolve palette tint
  (`0x4182d0`), the region-C blend/overlay arms (`0x1b58d`/`0x1b5ab`), the `0x587e00`
  prologue (front-header flags + HUD/border bank selection + `0x1bd82` autotile) + its
  trailing layer pass (`0x58c8c0`/`0x58c8d0`/`0x58cb30`).
- **Camera/view object** — RESOLVED for the static first frame (ckpt 64): the object +
  its room-entry init + the live-probed first-frame value are RE'd/ported (`map_render`
  `MAP_RENDER_CAM_TOWN_3F2`). STILL OPEN (PORT-DEBT `ingame-camera-snap`): the spawn-snap
  that derives `+0x60`/`+0x5c` from the entry params, and the scripted intro pan (the
  dynamic-scroll follow across `0x4710c0`/`0x54f980` + `0x499ab0`→`view+0x74`).
- **Register batches not yet called at boot:** `ar_register_fonts`,
  `ar_register_palette_ramps` (`0x57a330`), the big `0x56e190` (442 sprites), sounds —
  the in-game/prologue scenes need them (all take the sotesd HMODULE).
- **Audio ZDM** `0x5bab10`/`0x5bc150` + SFX `0x411390` — milestone 3.
- **Launcher `config.dat`** `0x5a4770` (46 KB) — milestone 4 (loads sotesd/w/p.dll,
  handles at `DAT_008a6e74/78/7c`).
- **Input producer** (DInput `GetDeviceState`, vtable `[0x24]`) + axis-held flags — black
  box; `mem_watch.py` is the tool.
- **God-object `DAT_008a9b50`/`DAT_008a6e80` layout** — model as we go.

## How to apply (when the user says "continue RE work" or similar)
1. Read `FRONT.md` (60-sec) then this file; `STATUS.md` + `ROADMAP.md` for coverage/next.
2. Pick the recommended next move (or whichever the user redirects to).
3. Port-and-test: small unit → host test → commit. Each ported function gets a
   `FUN_XXXXXX` provenance comment; pin retail offsets via `_Static_assert`. **Reference
   UNPORTED callees by bare VA, never `FUN_`** (it inflates the ledger).
4. **Append any engine quirk** to `findings/engine-quirks.md` (retail behavior only).
   Tag any MVP/synthetic shortcut `PORT-DEBT(...)` + a row in `port-debt.md`.
5. **Regen** `gen_port_ledger.py` + `gen_frontier.py` after a port; check the headline
   didn't move unexpectedly.
6. Verify rendering with `--capture-frames` vs goldens; bit-exact bar (`parity-model.md`).
7. Update `FRONT.md` + this file each meaningful checkpoint; append to `PROGRESS.md`.
8. Suggest a `/clear` at the natural stop point.
