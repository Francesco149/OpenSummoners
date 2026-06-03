# Session handoff — last updated 2026-06-03 (ckpt 46 — the ELEMENTAL-STONE PROLOGUE CUTSCENE (FUN_0056cd20) is ported + wired into the Start path + RENDERS LIVE (gem + aura + scrolling narration), USER-CONFIRMED visually; next is the bit-exact retail diff, then the game proper 0x59ec30 — in-game, deferred)

> **ckpt 46 — THE ELEMENTAL-STONE PROLOGUE CUTSCENE IS PORTED, WIRED, AND
> RENDERS LIVE.**  Confirming "Start Game" in the new-game menu now runs the gem
> cutscene (`FUN_0056cd20`, the prologue critical path): the glowing purple
> Elemental Stone rises on black with a soft aura, while the **story NARRATION
> scrolls** ("Elemental Stones: stones imbued with the power of an Elemental
> Spirit, which grant the wielder of one the ability to control that element via
> 'magic'.…").  New pure, host-tested **`src/prologue_stone.{c,h}`** (the visual
> half of `0x56cd20`): the per-tick UPDATE state machine (start delay, watchdog,
> gem fade-in/hold/fade-out, gem-frame %0x23 + aura toggle cadence, the rise
> curve, the 6 caption-line state machines, abort/beat input) + the render-
> descriptor build.  New **`src/prologue_drive.{c,h}`** is the Win32-free caller
> (steps one tick/frame, renders + presents; no input-gate ramp — the cutscene
> reads the raw ring).  `main.c`: `prologue_render` clears to black + blits gem
> (slot[3]/0x4a2 via ramp_b) → aura (slot[1]/0x49f via ramp_a) → 24 caption tiles
> (slot[2]/0x448 via ramp_b); the new-game START commit now calls `enter_prologue`
> instead of re-displaying the title; on DONE the game proper is unported (re-
> display title, logged), on ABORT (0x22) → title.  19 host tests (**749 pass /
> 0 fail / 6 skip**).  Ledger **174/1490 (+1: `0x56cd20`)**.
>
> **KEY LIVE FINDING:** the **prologue NARRATION is part of `0x56cd20`**, NOT the
> game proper — it is **pre-baked sprite tiles** (bank `0x448` = slot[2], a 24-
> tile strip = 6 lines × 4 horizontal tiles), the grid the survey first mislabeled
> "sparkles".  `0x56cd20` uses **no GDI text** at all.  The banks were **already
> registered at boot** (`ar_register_main_sprites` group 4), and the alpha blit
> (`zdd_alpha_blit`) + ramps (`g_ramp_a`/`g_ramp_b`) were already ported — so the
> cutscene needed only the state model + drive + wiring.  The aura's blend ramp
> (ramp_a, idx `local_bc/30`) was recovered from the **disasm** (`0x56d38d`); the
> decompiler had dropped `FUN_005bd550`'s `__thiscall` ECX = the ramp entry.
>
> **USER-CONFIRMED visually** ("that cutscene looks good… on first inspection it
> looks right").  Montage pushed to llm-feed (frames 950–3300 of one run).
>
> **OPEN gate — NO bit-exact retail diff yet.**  No retail golden of the stone
> intro has been captured.  NEXT: drive **retail** to the cutscene (the committed
> trace + Start-Game confirm), capture goldens, and `differ_px`-diff the gem +
> narration vs the port — the bit-exact bar.  (Caveat to assess: like the picker,
> `0x56cd20`'s modal loop may freeze the hooked Flip counter — if so, capture
> needs a non-flip-keyed harness.)  Until then the cutscene is eyeball-verified.
> See Next move #1.
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 45 — the new-game OPTION PICKER submenu is ported + wired + rendered + USER-CONFIRMED (FUN_00567ba0, quirk #71))

> **ckpt 45 — THE NEW-GAME OPTION PICKER SUBMENU IS PORTED, WIRED, RENDERED,
> AND USER-CONFIRMED.**  Confirming on a kind-0 option row (Game Difficulty /
> Auto-guard) now opens that option's **picker submenu** — the nested value-grid
> `FUN_00567ba0` runs.  New pure, host-tested **`src/newgame_picker.{c,h}`**:
> `newgame_picker_values` (`FUN_00568320` — id 3 `{10,20,30,40}`/`{..,50}` unlock-
> gated, id 4 `{0,1}`) + the run-loop model (build the 1-col value grid, **seek
> the cursor to the current value** via `FUN_00419900`, nav/commit/cancel via the
> `NEWGAME_PUMP_*` codes → RUNNING/COMMIT(value)/CANCEL).  Wired into
> **`newgame_drive`** as a frame-stepped modal SUBMODE (the equivalent of retail's
> blocking `FUN_00567ba0` call): a kind-0 CONFIRM opens the picker and pumps input
> into `picker.grid` instead of the parent; on COMMIT the drive calls
> `newgame_scene_set_option(id, chosen)` (the parent value-refill — re-lays the
> value cell); on CANCEL the option is unchanged.  `main.c` draws the picker box
> (9-slice, **(288,128) w=256**) + its value rows over the menu when active.
> **Verified LIVE** (port trace Start→confirm Difficulty→down→confirm): the
> difficulty picker opens `{1:Easy,2:Normal,3:Hard,4:Expert}` with the current
> value focused; nav moves the highlight; commit re-lays the parent cell **1:Easy
> → 2:Normal**.  Montage pushed to llm-feed; **USER-CONFIRMED** ("screenshots look
> good, those menus render correctly").  Quirk **#71**.  10 new host tests
> (**736 total: 730 pass / 0 fail / 6 skip**).  Ledger **173/1490 (10.7%)** (+5:
> `0x568320`,`0x567ba0`,`0x419900`,`0x5657f0`).
>
> **OPEN gate (NOT a content bug — a harness limitation): no bit-exact retail
> diff of the open picker.**  Entering the new-game scene, retail's **Flip counter
> freezes** (the modal pump `0x565d10` doesn't advance the hooked DDraw Present —
> quirk #67 caveat), and **both** the harness's frame capture **and** its input
> injection are flip-keyed.  So the open picker can be neither driven-to nor
> captured by flip index → its render geometry (288,128/256) and the decompiler-
> reconstructed args (the `FUN_00412160` row kind, `FUN_00419900`/`FUN_005657f0`
> arg lists, all documented in `newgame_picker.h`) are **NOT pixel-verified vs
> retail**.  Closing it needs a harness that hooks `0x565d10`'s own present +
> feeds its input directly (not flip-keyed) — a tooling task, deferred.  Until
> then the picker is user-eyeball-verified, which is the verification available.
>
> **NEXT: the Start→game path (the prologue critical path).**  The committed
> reference trace navigates to "Start Game" and confirms — it never opens the
> picker — so the Start→game transition (`0x564160`→`0x5642e0`/`0x56cd20` timed
> Elemental-Stone cutscene → `0x59ec30` game proper) is the item actually on the
> path to "first frame in-game".  START is a stub today (re-displays title).
> See Next move #1d.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 44 — THE NEW-GAME TOOLTIP TEXT NODE RENDERS BIT-EXACT.**
> The bottom-of-screen help line is a standalone **word-wrapping text node**
> (`this+0x170`), NOT the menu grid — one free-form string greedily wrapped into
> rows.  Ported the layout core **`FUN_0040e5e0`** (justify) + the **`%n`/`%m`/`%w`
> parse `FUN_0040f040`** as the pure, host-tested **`src/glyph_wrap.{c,h}`**: a
> word = alpha `[A-Za-z']+` / digit `[0-9.,]+` (+ one absorbed trailing
> `{space ! , - . ; ?}`) / lone glyph; the row-width accumulator + wrap test
> mirror retail's `uVar13`/`param_1` (width = the `FUN_0040dee0` ctor arg
> `0x44` = **68 glyph-columns**); `%n` forces a break.  SJIS kinsoku (`sVar3==3`,
> the `DAT_008548xx` table) is deferred (English never reaches it).  Quirk **#70**.
> `newgame_render` (`main.c`) now picks the focused row's help string
> (`newgame_scene_tooltip`), wraps it, and draws each row at **(72, 416+r·28)**
> with the menu's 2-copy shadow (`0xa8b9cc`) + text (`0x3e537d`).
> **Verified LIVE** (port Flip 760 vs `goldens/retail-newgame-config-menu.png`):
> the difficulty-row tooltip wraps **65 / 52** glyphs across y=416/444 — the break
> is the width-68 word-wrap (the source string has **no** `%n`), reproduced
> exactly.  Tooltip region: **0 text-colored pixels differ** (every glyph +
> shadow matches; proven by a text-presence XOR == 0).  6 new host tests
> (`tests/test_glyph_wrap.c`, **720 pass / 0 fail / 6 skip**).  Ledger **168/1490
> (+5: `0x40e5e0`,`0x40f040`,`0x4031c0` touched)**.  Comparison pushed to llm-feed.
>
> **OPEN (small, pre-existing, NOT the tooltip text): a 9px box-panel RGB565
> 1-LSB rounding.**  The only residual in the tooltip region is **9 background
> (cream/gold) pixels off by exactly 8 in a 5-bit (R or B) channel, green always
> exact** — i.e. one RGB565 quantization step on the 9-slice box-panel sprite
> (`newgame_box`, ckpt 40, bank `0x457`), NOT the text.  Same class as the
> delta-8 px in the menu box (there masked by the deferred top-left sparkle).
> **Hypothesis:** an 8→5-bit decode rounding (round vs truncate) in the box-frame
> sprite decode (`bs_convert_to_16bpp`) lands a few edge/gradient pixels on the
> opposite side of a 5-bit boundary.  A separate sprite-decode investigation —
> compare the port's decoded `0x457` frame RGB565 vs retail at those px; deferred
> (the box was user-accepted at ckpt 40).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 43 — THE NEW-GAME SELECTION CURSOR RENDERS BIT-EXACT (`differ_px=0`).**
> The ckpt-42 "scale_flag=1 videomem cell-build path" diagnosis was **WRONG**.
> The real bug: `bs_trim_opaque_rect` (`FUN_005b6f80`) named its two size params
> `(height, width)`, but retail's arg4 = `cell_w` (the inner/column loop + x-range)
> and arg5 = `cell_h` (the outer/row loop + y-range); the slicer passes
> `(cell_w, cell_h)`, so a **non-square** cell was scanned **transposed**.
> Invisible on the square 32×32 box bank `0x457` (every square sprite stayed
> bit-exact), it scrambled the 32×48 cursor bank `0x455` into a wrong-size,
> wrong-offset, un-keyed cell (the "opaque-black 16×24 @ x72" symptom).  Fix: swap
> the param names to `(width, height)` in `src/bitmap_session.{c,h}` (body
> unchanged); the port's `0x455` frame-17 trim is now **22×41 @ (4,3)**, matching
> the live `--box-probe` (proven offline by **`tools/extract/cursor_trim_probe.c`**).
> Quirk **#69**.  `g_newgame_cursor_enable` flipped **ON**; verified LIVE — port
> Flip 761 vs `goldens/retail-newgame-config-menu.png` → menu-box **`differ_px=0`**
> (panel + text + cursor).  Off-phase frames 760/762 differ only by the cursor's
> animation phase (frames 17≡19 = 22×41@(4,3) is the phase the golden froze; 16/18
> are the off-phase) — the same caveat as the intro twinkles, not a content gap.
> Closes the ckpt-40 307px residual.  Parity-ledger **#5**.  **714 pass / 0 fail /
> 6 skip** (+1 regression: `test_trim_8bpp_nonsquare_quirk69`).  Ledger 163/1490
> (bug-fix, no new FUN).  **NEXT: the tooltip TEXT node (word-wrap) → the option
> picker submenu (`0x567ba0`) → the Start→game path (`0x564160`→`0x59ec30`).**
> See Next move #1b.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 42 — THE SELECTION-CURSOR SPRITE BANK IS SOLVED: it is bank `0x455`
> (sotesd.dll, slot 43 = `AR_SPR_FONT_TEX_455`), frames 16–19 — the SAME
> bank/slot/frames the ckpt-41 geometry port already targeted.**  The ckpt-41
> blocker ("0x455 sweep matches nothing") was a decode-**ORIENTATION** error: the
> Lizsoft DATA blob is a BMP-style **bottom-up** bitmap, and the engine slices
> cells bottom-up.  Read top-down, frames 16–19 land on the row-4 ► chevrons
> (9×17 — not a vine); read **bottom-up**, frames 16–19 are the drooping gold
> **feather/quill + soft white shadow**, and their trimmed bboxes match the live
> `--box-probe` **EXACTLY** (frame 17 = 22×41 @ (4,3), 18 = 22×40 @ (4,4),
> 19 = 22×41 @ (4,3)).  Proof tool: **`tools/extract/cursor_frame_match.py`**.
> The probe's `res_id=0x3e8` (slot+0x40) is a **reused/garbage marker** (PE 0x3e8
> = an 80×352 portrait in sotesd / a WMV in sotesw / absent in sotesp) — the
> reliable per-frame signal is the trim size via `entries[frameSel]→frec+0x14/
> +0x18`.  Quirk **#68**; #67 corrected.  No `src/` logic change (decode-
> orientation + bank-ID finding), so **713 pass / 0 fail / 6 skip** unchanged;
> ledger unchanged.
>
> **STILL OPEN (the NEXT render task) — a scale_flag=1 cell-build bug, NOT a bank
> problem.**  I flipped `g_newgame_cursor_enable` ON and captured live (port frame
> 760): the cursor blits as an **opaque-black 16×24 rect at x72–87** (golden
> feather is at x44–66) — the gold corner ornament is the box's own 9-slice
> corner.  `0x455` is the **only** registered bank with **`scale_flag=1`** (box
> `0x457` is 0), so its cell takes the **untested videomem cell-build path**
> (`zdd_object_build_cell` `videomem` arg → caps `0x804`).  Symptoms: the slicer
> computes the **wrong trim offset** (base (40,26)+fdx≈32 → x72 vs correct fdx=4 →
> x44) AND the transparent area **fails to colour-key**.  differ_px went 307→493
> (a regression), so the gate is reverted to **OFF**.  **NEXT: dump the port's
> decoded slot-43 frame-17 trim rect + surface, compare to the probe's 22×41 @
> (4,3), and fix the scale_flag=1 videomem trim/keying — then flip the gate ON
> and diff vs the golden.**  See Next move #1a''.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 41 — THE SELECTION-CURSOR (gold vine) GEOMETRY IS PORTED + VALIDATED,
> but its sprite BANK is unidentified — render is GATED OFF pending a reliable
> retail probe.**  New pure, host-tested **`src/newgame_cursor.{c,h}`** ports
> `FUN_0048d940`'s type-1 arm (the menu selection cursor): frame = base(16) +
> frames{0,1,2,3} → 16-19 (from the RELIABLE `FUN_00411ec0` decomp args, not the
> probe); blit base x = box_x + (node+0x7c=-32 → +8), y = box_y + (node+0x80=-30
> → -6) + (cursor-sel2)*pitch(28).  **Position VALIDATED**: row-0 base (40,26)
> matches the live `--box-probe` golden AND independently derives from the text
> col0/row0 origins (72-32=40, 56-30=26).  4 new host tests (**713 pass / 0 fail
> / 6 skip**).  `main.c` has the adapter + render wired behind `g_newgame_cursor_enable`
> (OFF).
>
> **THE BLOCKER (open RE thread): the cursor's sprite BANK is unidentified.**  The
> `--box-probe` deref chain (`bank=*(node+0x28); slot=*bank`) reads **garbage** at
> `slot+0x20`(width=a pointer) / `slot+0x38`(f_38=0xf800) for THIS node type, so
> its **`res_id=0x3e8` and 22×41 frame readouts are NOT trustworthy** (only the
> POSITION, computed from node fields, is).  Ruled out: port's **0x3e8** (slot 65,
> sotesd) decodes to a **640×352 background landscape** (8 frames, not a vine);
> **0x3e8 is absent** in sotesp/sotesw (decode fails); a full **24-frame sweep of
> the sibling box atlas 0x455** (slot 43, the `+0xb8c`-vs-`+0xb88` guess) at (40,26)
> **matches nothing** (frames are 44×30 feathers/◄►arrows/caduceus/books, but the
> golden's element is a thin **22×41 drooping stem+bud+soft-shadow**).  The real
> bank = `*(god+0xb8c)` — whose WRITE site I could not find in the decomp (only
> reads).  **NEXT: find the real cursor bank** — either locate the `god+0xb8c`
> store (the box-widget god-object's sprite-bank init, near the `+0xb88`=0x457
> box-bank store), or write a Frida probe that dumps the actual blitted frame
> surface PIXELS from retail's `0x48d940` (the only fully reliable ground truth).
> The 307px menu-box top-left residual is this still-unported vine.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 40 — THE NEW-GAME CONFIG BOX PANEL RENDERS (9-slice chrome; menu box
> bit-exact except the deferred sparkle corner).**  The bordered cream panel
> behind the menu + tooltip is now drawn.  First a new live **`frida_capture.py
> --box-probe`** (hooks the sprite-cell render `0x48d940` + the 9-slice renderers
> `0x48cb90`/`0x48cf80`, gated to the menu flip window) captured the exact box
> composition (golden `goldens/retail-newgame-box-cells.jsonl`, **quirk #67**):
> the panel is a **9-slice sprite box** (`0x48cf80`), bank **PE resource 0x457**
> (already registered by `ar_register_fonts` as `AR_SPR_FONT_TEX_457`, 32×32),
> frames **tl0/top1/tr2/l3/c4/r5/bl6/b7/br8** (center 4 = cream RGB(239,227,214));
> two instances — **menu box (32,32)400×124** + **tooltip box (32,392)576×80** —
> matching the golden's measured bounds.  A **separate** animated sparkle corner
> (`0x48d940`, bank **0x3e8**, frames 16–19) sits at the top-left.
>
> Ported `0x48cf80`'s opaque arm as the pure, host-tested **`src/newgame_box.{c,h}`**
> (the 9-slice tiling walk over a `newgame_box_ops` vtable: corner→tiled edge→
> remainder→corner per row, top/full-middle/partial-middle/bottom rows); the real
> blit (`ar_sprite_slot_frame` + `zdd_object_blt_clipped` = the keyed clipped blit
> `FUN_005b9bf0`) is wired in **`main.c`**.  `newgame_render` now: clear primary →
> draw both boxes via DDraw → GetDC + `glyph_grid_render` the menu text on top
> (replacing the placeholder `PatBlt(BLACKNESS)`).  **Verified LIVE** (port frame
> 760): menu box **differ_px=307/50800 (0.6%)** vs the golden — and **all 307 are
> in the top-left corner (x44–65,y29–69)**, exactly the deferred sparkle overlay;
> the 9-slice panel + menu text are **bit-exact everywhere else** (interior cream
> RGB(239,227,214) matches exactly).  Comparison pushed to llm-feed.  4 new host
> tests (coverage-grid: slices tile each box exactly once, no gap/overlap/OOB) →
> **709 pass / 0 fail / 6 skip**.  Ledger **161/1490 (+4: `0x48cf80`, `0x48d670`,
> `0x48d3d0` ported; the keyed blit was already in zdd.c)**.
>
> **NEXT: the remaining chrome + the Start→game path.**  (a) the **animated
> sparkle corner** (`0x48d940`, bank 0x3e8 — needs that bank registered + the
> single-cell animated render; the 307px residual); (b) the **tooltip TEXT node**
> (y=416/444, word-wrapped — `newgame_scene_tooltip` computes the text, the box is
> already drawn, need the word-wrap split into rows); (c) the **option picker
> submenu** (`0x567ba0`); (d) the **Start→game path** (Elemental-Stone intro
> `0x564160`→`0x5642e0`/`0x56cd20`→`0x59ec30`).  Also deferrable polish: the box
> **fade-in** (`0x48cf80`'s alpha arm via `0x5bd550`).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 39 — THE NEW-GAME CONFIG SCENE RUNS LIVE (visible + interactive).**
> Wired the new-game config scene as a runnable **drive** — the last piece after
> the builder (ckpt 37), renderer (bit-exact, ckpt 36), and run-loop model
> (ckpt 38).  New **`src/newgame_drive.{c,h}`** is the Win32-free caller (the
> `FUN_00565d10` / case-0x24 frame-pump side), factored like `title_drive` vs
> `title_scene`: owns the scene + input manager, ramps the input gate
> **+50/frame to 1000** (the title's `menu_owner_transition_step` mode-1 ramp),
> polls the buttons and collapses them via `menu_list_latch` into the pump result
> the scene dispatches on, then renders + presents through cfg callbacks.  The
> input pump realises `0x565d10`'s scan + `0x43bca0`'s id→latch map (quirk #65):
> 1=up/3=down/2,4=page→MOVE(`0xd`); `0x24`=confirm→latch(9)→3→CONFIRM(`0xc`);
> `0x27`=back→latch(10)→4→BACK(`0xb`).  `main.c`: `app_flow`'s NEW_GAME arm now
> routes to **`enter_newgame`** (was the re-enter-title stub); `newgame_render`
> GDI-renders the menu grid onto the primary (`glyph_grid_render` base (32,32),
> **Courier New 7×18 = font slot 5**); a `g_newgame_active` branch runs one
> `newgame_drive_step` per frame.  **Verified LIVE**: confirm Start @flip 620 →
> `result=26` → enter_newgame → the menu renders "Game Difficulty 1:Easy /
> Auto-guard On / Start Game" (row 0 focused); DOWN moves focus 0→1 (pixel-
> sampled colours bit-exact, periwinkle `0xf08080` + tan `0xa8b9cc` shadow);
> `0x27` backs out → title replays.  Quirk **#66**.  7 new host tests
> (**705 pass / 0 fail / 6 skip**).  Ledger **157/1490 (+2: `0x565d10`,
> `0x43bca0`, both partial)**.
>
> **NEXT: the deferred chrome + the Start→game path** (Next move #1).  The scene
> is live but bare: no **box widget tree** (`0x411940`→`0x40f3e0` bordered box +
> gold corners; plain black fill now), no **tooltip text node** (y=416/444,
> word-wrapped — `newgame_scene_tooltip` computes the text, render needs the
> box/word-wrap builder), no **option picker submenu** (`0x567ba0`; a kind-0
> confirm yields NEWGAME_OPEN_PICKER, surfaced but inert).  START is a stub
> (re-displays title); the real path is the **Elemental-Stone intro**
> (`0x564160`→`0x5642e0`/`0x56cd20`→`0x59ec30`).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 38 — THE NEW-GAME CONFIG RUN-LOOP MODEL IS PORTED (the Win32-free
> heart of `FUN_00564780` case 0x24); the `0x27` input semantics are
> RE-corrected.** Ported the run-loop heart of the new-game config scene into
> new **`src/newgame_scene.{c,h}`**, mirroring the `title_scene` (pure) vs
> `title_drive` (Win32) split.  The pure state machine host-tests; the real
> per-frame pump stays in the drive (next unit).  Three pieces:
> **`newgame_scene_tooltip`** (the per-frame focused-row tooltip:
> `newgame_option_tooltip` = **`FUN_00566850`** for option rows id 3/4 + the
> kind-3 action-tooltip switch — Save/Default/Cancel/Start-Game help, strings
> verified from the binary); **`newgame_scene_dispatch`** (the pump-result
> switch, 564780.c:597-669: `0xd`→re-render, `0xc`→confirm-on-row,
> `0xb`→back); **`newgame_scene_set_option`** (the picker's value-refill,
> `FUN_00566a80`→`glyph_cell_layout`).  Quirk **#65**.  4 new host tests
> (**698 pass / 0 fail / 6 skip**).  Ledger **155/1490 (+1: `0x566850`)**.
>
> **KEY RE FINDING (quirk #65) — the `0x27` input semantics:** new-game-flow.md's
> earlier "id 0x27 = value left/right" was a **wrong** guess (it self-flagged as
> unverified).  Tracing `FUN_00564780`'s loop: `0x565d10`→`0x43bca0` maps button
> `0x24`→`menu_list_latch(9)` and `0x27`→`menu_list_latch(10)`, which net out to
> **`0x24` = confirm (`0xc`)** and **`0x27` = back/cancel (`0xb`)**.  There is
> **NO in-place value toggle** — an option's value changes only by confirming
> (`0x24`) into its **picker submenu** (`FUN_00567ba0` default arm for id 3/4).
> Only the *physical-key identity* of `0x24`/`0x27` is left for a live Frida
> `--input-trace` confirm.
>
> **NEXT: wire the new-game config scene as a runnable DRIVE** (Next move #1).
> The builder (ckpt 37), the renderer (bit-exact, ckpt 36), and now the
> run-loop model (ckpt 38) are done.  What's missing is the Win32 drive that
> makes it VISIBLE + interactive: a `newgame_drive` mirroring `title_drive` that
> (a) renders each frame — box widget bg + `glyph_grid_render` of the menu node
> + the tooltip node — onto the primary surface and presents; (b) runs the
> input pump (`0x565d10` + `0x43bca0` scan, or a port-bespoke equivalent feeding
> `newgame_scene_dispatch`); (c) ramps the input gate (`sub.ready` 0→1000); and
> (d) routes `app_flow`'s `NEW_GAME` arm to it instead of the re-enter-title
> stub.  Deferred sub-rocks: the **box widget tree** (`0x411940`'s `0x40f3e0`
> box + tooltip box `0x4124d0`/`0x40dee0`) and the **option picker submenu**
> (`0x567ba0`) and the **Elemental-Stone intro** (`0x564160`→`0x5642e0`→`0x59ec30`).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 37 — THE NEW-GAME CONFIG MENU BUILDER IS PORTED; the text pipeline is
> now closed END-TO-END (build → render → bit-exact `TextOutA` stream).**
> Ported the construction half of the new-game ("Start") config scene —
> `FUN_00564780` **case 0x24** + the grid setup `FUN_00411940` performs — into
> new **`src/newgame_menu.{c,h}`**.  Run through the (already bit-exact, quirk
> #63) renderer `glyph_grid_render` at the box base **(x=32, y=32)**, the built
> grid emits retail's captured `TextOutA` stream **draw-for-draw**: all **129**
> menu-region glyph draws match
> `goldens/retail-newgame-config-textout.jsonl` exactly
> (`tests/test_newgame_menu.c`).  **Geometry fully reconciled** (the ckpt-36
> open TODO): col 0 origin **x=72** (entry[0].pos 0 + base 32 + `field_c` 40),
> col 1 **x=232** (case-0x24's `entry[1].pos=0xa0` override), row pitch **28**
> (`node+0x1ac`), rows y=56/84/112; focus row 0 in 0xf08080, others 0x3e537d,
> shadow 0xa8b9cc.  Ported functions: `menu_grid_append` (`FUN_00412160`, a thin
> append whose per-column refresh == `FUN_00411f40` → delegated to
> `menu_row_finalize`, no-op on fresh rows, quirk #36); the option string
> providers `newgame_option_label`/`newgame_option_value` (`FUN_00566570`/
> `FUN_00566a80` arms id 3/4); `newgame_config_build` (the case-0x24 sequence).
> Quirk **#64**.  3 new host tests (**694 pass / 0 fail / 6 skip**).  Ledger
> **154/1490 touched (9.5%, 148 tested)** (+4: `0x412160`, `0x564780`,
> `0x566570`, `0x566a80` — last three partial).
>
> **NEXT: wire the new-game config scene as a runnable DRIVE** (Next move #1).
> The grid renders bit-exact; what's missing is the live scene — the run loop
> (`0x565810`/`0x565d10` nav), the value toggle (id 0x27), the tooltip text node
> (`0x566850`), the box widget tree (`0x411940` geometry/title sub-nodes), and
> the Start→game transition (`0x564160`→`0x59ec30`).  Route `app_flow`'s
> `NEW_GAME` arm to it instead of the re-enter-title stub.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 36 — THE TEXT-RENDERER PIXEL GATE IS CLOSED: every GDI parameter
> matches retail's LIVE `TextOutA` stream, bit-for-bit.** Part 1 wired
> `ar_register_fonts` at boot + a `--render-glyph-test` offscreen DIB path
> (port side). Part 2 (this checkpoint) closed the verification gate from the
> retail side: a new **`frida_capture.py --textout-probe`** hooks
> `gdi32!TextOutA`/`ExtTextOutA` and records every glyph draw
> (x/y/bytes/colour/bk-mode + the selected `LOGFONTA`). Drove retail to the
> **new-game config menu** — the first GDI-text scene — and diffed its real
> output against the port's renderer. **Every parameter matches:** font
> (Courier New **7×18**, port slot 3), bk mode **TRANSPARENT**, **7 px/glyph**
> advance, **per-glyph `TextOutA`**, the **2-copy shadow** `(x+1,y)/(x,y+1)`,
> and all three colours — normal **0x3e537d**, shadow **0xa8b9cc**, focused
> **0xf08080**. Since GDI rasterizes deterministically from an identical
> `HFONT` + identical draw args, the glyph pixels are **bit-identical** → the
> renderer port (`glyph_grid_render`/`glyph_row_draw`, ckpt 35) is **correct**.
> Quirk **#63**. Ground truth saved:
> `tests/scenarios/new-game-through/goldens/retail-newgame-config-menu.png` +
> `…-textout.jsonl`. 691 host tests pass (no `src/` change — verification +
> tooling). Ledger **150/1490** unchanged.
>
> **Two key findings while driving there:** (1) under the hidden-window turbo
> harness retail runs **~15 flips/s** (vs ~127 native), and the title menu
> auto-enters a **gameplay demo by ~flip 900** — so the old `new-game-through`
> trace's Start-at-flip-2050 landed in the demo; **`trace-retimed.jsonl`**
> presses Start at **flip ~400** to catch the live title menu. (2) That demo
> paints a GDI **debug HUD** (stat numbers + "Bonus Mode" + "Please Hit Any
> Key") via a **different** text routine — a full **3×3 outline** (9 shadow
> copies + centre), NOT the menu's 2-copy shadow. Not parity-relevant (the
> port doesn't render the demo), documented in quirk #63.
>
> **NEXT: the new-game config scene + its menu BUILDER** (Next move #2). The
> renderer is proven; what's missing is the code that BUILDS the cells it walks
> (row pitch 28, origin x=72, the value columns "1:Easy"/"On", the focused
> row). Once the port builds these, it emits the identical `TextOutA` stream →
> the end-to-end stream/pixel diff is trivially 0. Route `app_flow`'s
> `NEW_GAME` arm to it instead of the re-enter-title stub.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 35 — TEXT/GLYPH PIPELINE, PART 2: the GDI text renderer is ported +
> host-tested.** Ported the **render half** of the dynamic-text system into new
> **`src/glyph_render.{c,h}`** (+ **`glyph_render_win32.c`**), the visual
> counterpart of ckpt 34's build half. Three functions: **`glyph_row_draw`**
> (`0x48e860`, per-glyph `TextOutA`, 7 px/byte advance → 14 px per SJIS pair),
> **`glyph_ruby_draw`** (`0x48e6d0`, the furigana pass — gated on
> `node->field_14`==0 for basic menus + a no-op on raw text, faithful but
> unexercised), and **`glyph_grid_render`** (`0x48e200` GDI branch — walk rows
> `[sel2..+stride)` × cols, position each cell, pick text/shadow COLORREFs by
> selection state, monospace right-align, draw the **2-copy drop shadow** +
> glyphs + optional ruby).
>
> GDI is injected through a **`glyph_gdi_ops`** vtable
> (`select_font`/`set_text_color`/`text_out`), so the walk + colour selection
> are **pure + host-tested** with a recording stub; the real back-buffer GDI is
> in `glyph_render_win32.c` (`glyph_gdi_ops_win32`), the project's `_win32.c`
> split (host harness never links it). Modelled `menu_cell` +0x10/+0x14 as the
> per-cell colour overrides. **Three findings (quirk #62):** (a) the renderer's
> `this` is the **child** node while the **parent** supplies the x/y base; (b)
> the shadow is two offset copies (`(0,+1)`,`(+1,0)`) in `node+0x184`; (c)
> `node+0x188`/`+0x194`/`+0x198` hold label **pointers** read as COLORREFs but
> only on dead paths — only +0x180/+0x184/+0x18c/+0x190 are live menu colours.
> The retail **sprite-cell mode** (`param_1==0`, ZDD blits) is deferred. 11 new
> host tests (**691 pass / 0 fail / 6 skip**). Ledger **150/1490 (9.2%)** (+3:
> `0x48e200`, `0x48e860`, `0x48e6d0`).
>
> **OPEN VERIFICATION GATE (human/Frida): the pixel diff.** The walk is
> host-tested but the glyphs are **not yet diffed vs retail**. Next move:
> wire `ar_register_fonts` at boot, render a known string offscreen via
> `glyph_gdi_ops_win32` into a DIB-section DC, and `differ_px`-diff the glyph
> region vs retail (font-probe Frida hook on `0x48e200`, or the new-game menu
> once it builds). THEN the new-game config scene (`0x564780` case 0x24) +
> row-append `0x40f800`. See **Next move** below.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 34 — TEXT/GLYPH PIPELINE, PART 1: the glyph layout builder is ported +
> host-tested.** Surveyed the dynamic-text subsystem
> (**`docs/findings/text-glyph-pipeline.md`**), two findings (**quirk #61**):
> (a) text renders through **Win32 GDI** (`ar_register_fonts` builds 8 `HFONT`s,
> `0x48e200` `TextOutA`s each glyph — no rasteriser to port); (b) builder
> (`0x40fa00`), row-append (`0x40f800`), renderer (`0x48e200`) all operate on the
> **same `menu_ctrl`/`menu_node`** in `menu_list.h`. Ported the **build half**
> into **`src/glyph_text.{c,h}`**: `glyph_token_search` (`0x40fd20`) +
> `glyph_cell_layout` (`0x40fa00`, string → `cell.obj0` records; escape pass
> hooked NULL = faithful for escape-free English). Corrected the **swapped
> Ghidra params** (`0x40fa00` param_1/param_2 are ROW/COL). Ledger then
> **147/1490 (9.0%)**.
>
> ─────────────────────────────────────────────────────────────────────────────
>
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

## ⭐ Current state (ckpt 45): title is a bit-exact loop; the new-game config scene runs live + renders its BOX PANEL (ckpt 40), selection cursor BIT-EXACT (ckpt 43), TOOLTIP TEXT bit-exact (ckpt 44), AND the OPTION PICKER submenu ported + user-confirmed (ckpt 45, quirk #71); next is the Start→game path (0x564160→0x59ec30 — the prologue critical path)

> **User @ ckpt 44:** "can confirm 1:1 except for sparkle on cursor."  → The
> whole new-game screen (box + menu text + cursor + tooltip help text) is
> **user-confirmed 1:1**.  The "sparkle on cursor" they see is the **cursor's
> animated gold feather/glint at a different animation phase** than the single
> frozen golden frame — the SAME animation-phase sampling caveat as the intro
> twinkles: at the matching tick (Flip 761, frame 17/19) it is `differ_px=0`
> (ckpt 43), off-phase frames differ only by the feather's phase (gold pixels
> shifted, roughly balanced golden-brighter/port-brighter — NOT a missing
> additive sparkle, NOT a content gap).  **Nothing to fix here.**  (The other
> tiny residual — 9px box-panel RGB565 1-LSB rounding — is non-text, pre-existing
> box decode, see ckpt-44 OPEN.)  **The new-game scene RENDER is parity-complete;
> next is BEHAVIOUR: the option picker submenu + the Start→game transition —
> Next move #1c/#1d.**

The **new-game config scene** is live + interactive (ckpt 39) AND now renders the
**bordered cream box panel** behind the menu (ckpt 40).  `newgame_render`
(`main.c`) composes the frame retail-style: clear primary → draw the two 9-slice
box panels via DDraw (**menu box (32,32)400×124** + **tooltip box (32,392)576×80**)
→ GetDC + `glyph_grid_render` the menu text on top (base (32,32), Courier New
7×18 = font slot 5).  The 9-slice render is the pure, host-tested
**`src/newgame_box.{c,h}`** (port of `0x48cf80`'s opaque arm, quirk #67): a tiling
walk over a `newgame_box_ops` vtable, bank **PE resource 0x457** (=
`AR_SPR_FONT_TEX_457`, already registered by `ar_register_fonts`, 32×32 cells,
frames tl0/top1/tr2/l3/c4/r5/bl6/b7/br8, center = cream RGB(239,227,214)); the
real blit (`ar_sprite_slot_frame` + `zdd_object_blt_clipped` = `FUN_005b9bf0`) is
the main.c adapter.

The scene composes from the already-ported builder (`newgame_menu.c`, ckpt 37),
text renderer (`glyph_render.c`, bit-exact ckpt 36), run-loop model
(`newgame_scene.c`, ckpt 38), drive (`newgame_drive.c`, ckpt 39, quirk #66), and
now the box (`newgame_box.c`, ckpt 40).  **Verified LIVE** (port frame 760): menu
box **differ_px 307/50800 (0.6%)** vs the golden, with **all 307 residual pixels
in the top-left corner** = the **deferred animated sparkle overlay** (`0x48d940`,
bank **0x3e8**, frames 16–19, at (44,29)); the 9-slice panel + menu text are
**bit-exact everywhere else**.

What's left for the scene is the **remaining chrome + transitions**: the sparkle
corner (`0x3e8`), the tooltip TEXT node (y=416/444, word-wrapped — box drawn, text
computed by `newgame_scene_tooltip`, needs the word-wrap split), the option
picker submenu (`0x567ba0` — a kind-0 confirm yields NEWGAME_OPEN_PICKER, surfaced
but inert), the box fade-in (`0x48cf80`'s alpha arm), and the Start→game path (the
Elemental-Stone intro `0x564160`→`0x5642e0`→`0x59ec30`).  See **Next move #1**.

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

> Context: the title is a bit-exact loop; the new-game config scene renders bit-
> exact (box + text + cursor + tooltip) with the picker user-confirmed; and now
> the **prologue gem cutscene renders live + user-confirmed visually** (ckpt 46).
> The active goal (user, ckpt 13) is **1:1 parity** for title + new-game +
> prologue.  What's left for the prologue is the **bit-exact retail diff**.

1. **(recommended) Capture a RETAIL golden of the stone intro + `differ_px`-diff
   it vs the port.**  The cutscene (`FUN_0056cd20`) renders correctly by eyeball
   (gem + aura + scrolling narration; user-confirmed ckpt 46) but has **no bit-
   exact verification** yet — no retail golden captured.  Drive **retail** to the
   cutscene: the committed new-game trace + a Start-Game confirm (down ×2 from
   row 0, then `0x24`), capture frames during the gem rise, and diff the gem +
   narration region vs the port at a matched animation tick (the gem fade/rise +
   the caption `sub`-stagger make the tick alignment derivable, like the intro
   twinkles).  **Caveat to assess first:** `0x56cd20` is a modal loop like the
   picker — it may freeze the hooked Flip counter (`0x565d10` quirk #67 class),
   in which case flip-keyed capture/inject won't reach it and a non-flip-keyed
   harness (hook `0x56cd20`'s own `FUN_005b8fc0` present) is needed.  Port-side
   driving recipe (self-serviceable, WORKS): write a trace into the game dir —
   `{"frame":620,"ids":[36]}` (title Start), `{"frame":720,"ids":[3]}`,
   `{"frame":745,"ids":[3]}` (down ×2 → Start Game), `{"frame":800,"ids":[36]}`
   (confirm) — then `./build/opensummoners-launcher.exe --timeout-ms 120000 --
   /tmp/oss.exe --hide-window --frames 3600 --input-trace prologue_trace.jsonl
   --capture-frames "950,1550,2300,2800" --capture-dir=C:/osscap` (**forward
   slashes** in the capture dir — the launcher eats a backslash → `C:osscap`).
   Frames after ~800 are the cutscene; the narration's 2nd line appears ~flip
   2300.  Modules: `src/prologue_stone.{c,h}` (pure state+render),
   `src/prologue_drive.{c,h}` (the caller), `prologue_render` in `main.c` (blits).

2. **Then the game proper (`0x59ec30`, map 0x3f2) — the next big rock, but
   OUT-OF-SCOPE until the user extends the trace.**  On PROLOGUE_DONE the port
   re-displays the title (logged seam).  Entering `0x59ec30` is the whole in-game
   engine (`0x59f2c0` map loop + resource load/unload) — the active goal says "do
   NOT extend the trace toward in-game yet; once we have prologue and main menu
   rendering we extend the trace."  We now HAVE prologue + main menu rendering, so
   this is the moment to ask the user whether to extend the trace toward in-game.

3. **(deferred polish, optional) Finish the new-game scene's remaining CHROME.**
   The box panel + menu text + selection cursor render bit-exact; port the rest in
   roughly this order (cheapest visual win first):
   ~~(a) the box widget panel~~ **DONE (ckpt 40, quirk #67, `src/newgame_box.c`).**
   ~~(a') the selection cursor~~ **DONE + BIT-EXACT (ckpt 43, quirk #69).**
   Geometry ported ckpt 41 (`src/newgame_cursor.{c,h}`, `0x48d940` type-1, base 16
   + frames{0,1,2,3}→16-19, row-0 base (40,26)); bank ID'd ckpt 42 (0x455 slot 43,
   bottom-up, quirk #68); the render bug fixed ckpt 43 (a transposed trim scan, NOT
   the mis-diagnosed videomem path — `bs_trim_opaque_rect` arg order, quirk #69).
   `g_newgame_cursor_enable=1`.  Verified: port Flip 761 vs golden → menu-box
   `differ_px=0`.  Proof tool for the trim: `tools/extract/cursor_trim_probe.c`.
   ~~(b) the tooltip TEXT node~~ **DONE + BIT-EXACT (ckpt 44, quirk #70,
   `src/glyph_wrap.{c,h}`).**  The help line is a standalone WORD-WRAPPING text
   node (`this+0x170`), not the menu grid; the break is greedy word-wrap at
   width 68 (NOT a `%n` — the difficulty source string has none).  Ported the
   justify `FUN_0040e5e0` (ASCII path) + the `%n`/`%m`/`%w` parse `FUN_0040f040`;
   `newgame_render` picks `newgame_scene_tooltip`, wraps at 68, draws each row at
   (72,416+r·28) with the menu's shadow+colours.  Verified: difficulty tooltip
   wraps 65/52 across y=416/444 → **0 text-colored pixels differ**.  (The build
   chain `FUN_0040e360`→parse/justify/commit + the substitution table —
   `this+0x164`, empty for English — is documented in quirk #70; the port fuses
   parse+justify and renders directly, as main.c already does for the menu grid.)
   **REMAINING tooltip residual = 9px box-panel RGB565 1-LSB rounding (pre-existing,
   NOT text)** — see ckpt-44 OPEN above; a separate sprite-decode item.
   ~~(c) the option picker submenu~~ **DONE + USER-CONFIRMED (ckpt 45, quirk #71,
   `src/newgame_picker.{c,h}`).**  `FUN_00567ba0` ported pure (value list
   `FUN_00568320` + build + seek `FUN_00419900` + nav/commit/cancel), wired into
   `newgame_drive` as a modal submode, rendered at (288,128) over the menu; commit
   calls `newgame_scene_set_option`.  Bit-exact retail diff is an OPEN gate (the
   flip counter freezes in `0x565d10`'s modal pump — capture/inject are flip-keyed;
   needs a non-flip-keyed harness).  **NEXT — the transition (recommended below):**
   (d) the **Start→game path**: the Elemental-Stone intro (`0x564160` →
   `0x5642e0`/`0x56cd20` timed cutscene → `0x59ec30` game proper).  START is a
   stub today (re-displays title).  **This is the prologue critical path** — the
   committed trace navigates to "Start Game" + confirms (never opens the picker),
   so this is the item actually on the path to "first frame in-game".
   Deferrable polish: the box **fade-in** (`0x48cf80`'s alpha arm via `0x5bd550`);
   the picker bit-exact gate (a harness that drives/captures inside the modal pump).
   How to drive there (PORT side, self-serviceable): trace `{"frame":620,"ids":[36]}`
   into the **game dir**, then `./build/opensummoners-launcher.exe --timeout-ms 45000
   -- /tmp/oss.exe --hide-window --frames 1100 --input-trace ng_trace.jsonl
   --capture-frames "700,760,840" --capture-dir=C:\osscap` (use a **no-space**
   capture dir — the launcher splits the game-dir path on its space).  Frames
   after ~660 are the newgame scene (`phase=-1`).  RETAIL side: `--box-probe`
   (Flip freezes at 422 in the modal pump — flip-gated probes see only the
   title→menu transition, which is when the box first renders).
2. ~~The new-game config menu BUILDER.~~ **DONE (ckpt 37, quirk #64).**
   `src/newgame_menu.{c,h}` builds the case-0x24 grid (`FUN_00564780` case 0x24 +
   `FUN_00411940` setup); host-tested to emit retail's `TextOutA` stream
   draw-for-draw (`tests/test_newgame_menu.c`).  Geometry: col0 x=72, col1 x=232,
   pitch 28.  Remaining chips for the broader text system: the escape expander
   (`0x4034f0`/`0x4051d0`, hooked NULL — English labels don't need it), the
   sprite-cell render mode (`0x48e200` `param_1==0`), and the screen-settings
   row-append twin `0x40f800` (used by case 0x20, not case 0x24).
3. ~~Dispatch the title return code instead of exiting~~ **DONE (ckpt 33,
   `app_flow_dispatch` + `reenter_title`, quirk #60).** Exit exits; commits
   dispatch; unported arms re-display the title.

## Tooling added ckpt 36

- **`frida_capture.py --textout-probe [--textout-frames LO,HI]`** — hooks
  `gdi32!TextOutA`/`ExtTextOutA` in retail and logs each glyph draw
  (x/y/glyph-bytes/text-colour/bk-mode + the **selected `LOGFONTA`** via
  `GetCurrentObject(OBJ_FONT)`+`GetObjectA`) to `<run>/textout.jsonl`, deduped
  to the distinct draw set. The flip window skips the intro/demo debug-text
  flood (the probe returns before any GDI query outside `[LO,HI]`). This is the
  **GDI-text ground-truth** capture: which `ar_register_fonts` HFONT a scene
  picks, the colours, and the per-glyph advance. The `installTextOutProbe` /
  `ensureTextOutQueryFns` / `readSelectedFont` pattern in the agent generalises
  to any GDI primitive.
- **`tests/scenarios/new-game-through/trace-retimed.jsonl`** — presses Start at
  flip ~400 (the live title menu under the harness), vs the old `trace.jsonl`'s
  flip-2050 which lands in the auto-demo. Use this to drive retail to the
  new-game config menu.
- **`tests/scenarios/new-game-through/goldens/`** — `retail-newgame-config-menu.png`
  (the captured menu) + `retail-newgame-config-textout.jsonl` (the full
  per-glyph stream) — the parity ground truth for the menu builder port.

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

## Module inventory (22 modules) — render pipeline COMPLETE; text pipeline CLOSED end-to-end; new-game config scene = box + menu text + cursor + tooltip text all bit-exact + the option picker submenu ported/user-confirmed (Start→game pending)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper, cs_dispatch,
app_pump, title_scene (`FUN_0056aea0`, fully ported+wired+driven), input
(`FUN_0043c110`), obj_container, menu_list, **title_render** (compositor +
wrappers), **title_sink** (cmd→ZDD bridge, banks 19/20), **title_drive** (caller
side of the runner), **rng** (the MSVC LCG `FUN_005bf505`/`_5bf4fb`, ckpt 31),
**title_particles** (phase-7 sparkle pool: spawn `0x56c070` + update `0x56ba69` +
cull `0x56c030`, ckpt 31), **app_flow** (post-title dispatch = `FUN_00562ea0`
tail switch, ckpt 33), **glyph_text** (the cell-grid text/glyph layout builder:
`glyph_cell_layout`/`glyph_token_search` = `0x40fa00`/`0x40fd20`, ckpt 34 —
build half; escape expander still hooked-NULL), **glyph_render** (the GDI text
renderer: `glyph_grid_render`/`glyph_row_draw`/`glyph_ruby_draw` =
`0x48e200`/`0x48e860`/`0x48e6d0`, ckpt 35 — pure walk over a `glyph_gdi_ops`
vtable + real GDI in `glyph_render_win32.c`; sprite-cell mode deferred),
**newgame_menu** (the new-game config menu builder: `menu_grid_append` =
`0x412160`, `newgame_option_label`/`_value`/`_tooltip` =
`0x566570`/`0x566a80`/`0x566850` arms, `newgame_config_build` = `0x564780`
case 0x24, ckpt 37/38 — emits retail's `TextOutA` stream draw-for-draw),
**newgame_scene** (the run-loop model = `0x564780` case-0x24 loop body, ckpt 38:
`newgame_scene_tooltip`/`_dispatch`/`_set_option` — pure state machine; the
Win32 pump `0x565d10`/`0x43bca0`, picker `0x567ba0`, box widgets `0x411940` NOT
ported, the drive's job),
**newgame_box** (the 9-slice box panel = `0x48cf80` opaque arm, ckpt 40),
**newgame_cursor** (the selection-cursor geometry = `0x48d940` type-1, ckpt 41 —
bank 0x455 frames 16-19 bottom-up, render bit-exact ckpt 43, quirk #68/#69),
**glyph_wrap** (the tooltip text-node word-wrap = `0x40e5e0` justify + `0x40f040`
`%n`/`%m`/`%w` parse, ckpt 44, quirk #70 — pure + host-tested; SJIS kinsoku
deferred; rendered directly in `newgame_render`),
**newgame_picker** (the option picker submenu = `0x567ba0` default arm, ckpt 45,
quirk #71 — value list `0x568320` + build + cursor-seek `0x419900` + nav/commit;
wired into `newgame_drive` as a modal submode, rendered at (288,128); bit-exact
retail diff an OPEN gate — flip-frozen modal pump).
**8d** (`zdd_object_new_cell/_build_cell/_copy_cell_pixels`
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
- **Dynamic text pipeline — build + GDI render PORTED** (ckpt 34/35,
  `glyph_text.c` + `glyph_render.c`). Remaining chips: the **escape expander**
  (`0x4034f0`/`0x4051d0`, hooked NULL — port when an escape-bearing string needs
  it), the **row-append `0x40f800`**, and the **sprite-cell render mode**
  (`0x48e200` `param_1==0`, ZDD blits). **Pixel-diff vs retail still pending**
  (Next move #1).
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
