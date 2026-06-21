# The "res=0" UI sprite banks — RESOLVED (ckpt 153)

The USER-chosen ckpt-152 task: **resolve the `res=0` UI sprite bank** that backs the
dialogue advance indicator, the inline `@@`-code key-cap icons, and the freeroam HUD —
"faithful + LEGAL = load it from the user's files; bank unlocated".  This is the RE that
located it.  **Headline: every one of these banks is a `DATA` resource in the user's own
`sotesd.dll`; the port already loads most of them.  `res=0` was never a special module —
it is a capture-side identity gap** (the `.osr` proxy's resolver only names cels that pass
the `0x418470` registry; the dialogue/HUD cels resolve by direct frame-array index, so the
proxy logs them `res=0 fr=0`).  `render_id.c` even documents the convention: "resource_id 0
is never a real bank".

## Method (Frida ground truth, `runs/ui-bank/`)

Two complementary hooks over one live boot→errands drive (`control-path-gt/nav.jsonl`,
`--no-turbo --field-spec-only --res-probe`):

1. **Pin the specific banks** — a throwaway field spec (`tools/flow/ui_bank_fields.json`)
   `thischain`-reads the bank objects off the `__thiscall this` of the two widget builders
   that consume them:
   - `FUN_00410560` (the advance-indicator init): `this` (ECX) owns the bank at `+0xb8c`.
   - `FUN_00411940` (the dialogue-box build, writes the name colour `0x455f7b`): reads BOTH
     `+0xb88` and `+0xb8c`.
   Each bank object stores its source, per `FUN_004184a0 → bitmap_session::FUN_005b7800(this,
   hModule@+0x3c, resource_id@+0x40, "DATA", …)` (which calls `FindResourceA(hModule,
   resource_id & 0xffff, type)`): the **HMODULE at `+0x3c`** and the **PE resource id (u16)
   at `+0x40`**.  So `thischain hops=[0xb8c] off=0x40` = res, `off=0x3c` = HMODULE.
2. **Enumerate every load** — `--res-probe` hooks `FUN_005b7800` and emits `(module, id,
   type, hmod, ret_va)` per distinct resource load (`res_loads.jsonl`), module names
   resolved via `Process.findModuleByAddress`.

**Result (identical from both VAs):**
| god slot | res (u16) | HMODULE | module | port slot |
|----------|-----------|---------|--------|-----------|
| `god+0xb8c` | **0x455 (1109)** | 0x10000000 | **sotesd.dll** | `AR_SPR_FONT_TEX_455` (43) |
| `god+0xb88` | **0x457 (1111)** | 0x10000000 | **sotesd.dll** | `AR_SPR_FONT_TEX_457` (42) |

`res_loads.jsonl` (83 distinct loads) confirms ALL of them — the dialogue box 9-slice
(0x456), name tab (0x44a), banner (0x449), and every HUD-range DATA resource — come from
`sotesd.dll`.  **No legal blocker exists for any of them: the port loads them from the
user's installed `sotesd.dll` via the existing `asset_register` resource decoder.**

## res 0x455 = the menu/box ornament + cursor + book atlas (port slot 43)

Already RE'd by `tools/extract/cursor_frame_match.py` (ckpt 42, quirk #68) for the newgame
cursor; this pass mapped the WHOLE atlas (render: `tools/extract/lizsoft_sprite.py
runs/ui-bank/sotesd/type=DATA/1109.bin -W 128 -H 288 --flip`; labelled montage
`runs/ui-bank/atlas_0x455_labeled.png`, on the feed).  A **128×288 = 4-col × 6-row grid of
32×48 cells = 24 frames**, sliced BOTTOM-UP, transparent = palette index 0:

| frames | content |
|--------|---------|
| 0-2  | gold ornament (caduceus) |
| 3    | a red ◄► pair (one cell) |
| 4-7  | tan **◄** left chevron (4-frame bob, offy 7,8,9,8) |
| 8-11 | tan **►** right chevron (4-frame bob) |
| 12-15 | bright feather/quill |
| 16-19 | feather cursor — **the newgame selection cursor** (probe-confirmed) |
| 20-23 | the green **BOOK "next" indicator** — the dialogue ADVANCE indicator (base **0x14**) |

## The advance "arrow" = the BOOK indicator (DONE, ckpt 153)

`0x410560` builds it from `god+0xb8c` (= res 0x455 / slot 43): frame base **0x14** + anim
table `{0,1,2,3}` (one step / 10 updates), 1px bob baked into the per-frame cel placement
metric; `0x48d940` blits it only once the body grid leaves typing-state 1.  Position
`0x410560 +0x7c/+0x80` = box-rel **(0x170,0x5c) = (368,92)**.  Verified vs `retail.osr` seq
825 @ tick 1823: the book draws at **(400,284) 32×31** = errands box (32,192)+(368,92). ✓

**Ported (`src/main.c render_dialogue_box`):** the previously-hidden arrow now blits
`g_ar_sprite_slots[AR_SPR_FONT_TEX_455]` frame `dialogue_arrow_frame(d)` at
`box+(DIALOGUE_ARROW_DX,DY)`, gated on `dialogue_awaiting_advance(d)` (main box only), with
a `CALL_TRACE_BEGIN(0x48d940)` mirror.  Retires the ARROW half of
`PORT-DEBT(dialogue-arrow-art)`.

## The inline `@@`-code key-cap icons — BANK RESOLVED (res 0x6fa)

The errands tutorial line 3 (exe string **0x86f388**, read verbatim from the user's exe):
```
To move around, I press @@\x81\xa9 & @@\x81\xa8, and to talk to people and do stuff, I press @@X, right?
```
So the escape is `@@` (0x40 0x40) + a 1-2 byte code: **`\x81\xa9`=← (SJIS), `\x81\xa8`=→
(SJIS), `X`=action**.  At tick 1823 retail draws exactly 3 icons (`draw_probe` seq 537-539,
17×17 keyed): **←@(336,210), →@(378,210), X@(224,266)** — square grey 3D KEY-CAP BUTTONS with
a blue symbol (`osr_prof` recon `runs/ui-bank/retail_icons_body.png` + the per-cel
`runs/ui-bank/icons_src.png`).  These are NOT res 0x455 (those are tan chevrons), a DISTINCT
bank.

**RESOLVED by a PIXEL-EXACT match (279/279 px each):** the `.osr` captured each icon's 17×17
RGB565 source cel (keyed by dhash; res=0 = unmapped).  Sliding those cels over every
sotesd.dll bank that loaded in the errands drive found them in **PE resource `0x6fa` (1786,
sotesd.dll)** — the keyboard KEY-CAP / button-prompt sheet (arrows row 0, Z/X/C row 1, F1-F4
row 2; render `runs/ui-bank/r6fa_flip1.png`).  Layout: **256×128, 32×32 cells = 8 cols × 4
rows = 32 frames**, sliced bottom-up like res 0x455 (frame = row·8 + col of the natural
top-down image).  The 3 errands icons:
| code | symbol | cell (col,row) | **frame** |
|------|--------|----------------|-----------|
| `@@\x81\xa9` | ← | (3,0) | **3** |
| `@@\x81\xa8` | → | (1,0) | **1** |
| `@@X` | X | (1,1) | **9** |
The blit uses a 17×17 sub-window of the 32×32 cell (the cel's own `+0xb8/+0xbc` metric).

**The `@@` body layout** (derived from retail's exact x): the body is monospace at 7px/cell
(`DIALOGUE_ADVANCE`), x0 = `box_x + DIALOGUE_TEXT_DX`.  An `@@<code>` consumes the `@@` + the
1-2 byte code from the source but advances the cursor by **3 cells** (21px), blitting the
key-cap at `(x0 + cell·7, row_y − 2)`.  Verified: ←@cell 24 → x=168+24·7=336 ✓, →@cell 30
(←(3) + " & "(3) after), X@cell 8 → x=168+8·7=224 ✓.  (The grid renderer `0x48e200`
sprite-cell mode blits each cell from `cell[0]` bank + `cell+4` frame; `0x4051d0` is the
upstream raw-SJIS→cell substitution.)

**PORT — DONE + pixel-verified (ckpt 153).**  res 0x6fa is ALREADY registered (slot 55,
`asset_register.c` `{55,0x6fa,0x20,0x20,…}` = `AR_SPR_KEYCAP_6FA`).  `dialogue.c`
`dialogue_keycap_token` + `dialogue_expand_text` count each `@@<code>` as
`DIALOGUE_KEYCAP_CELLS`=3 cells in the word-wrap (not its 3-4 source bytes — the byte
buffer grew to `DIALOGUE_ROW_BYTES`); `main.c` `dialogue_body_row_text` lays out the body
char=1/icon=3 cells, skips the `@@<code>` in the GDI text, and blits the slot-55 frame at
`(x0 + cell·7, row_y − 2)`.  **Verified off `port-icons3.osr` vs `retail.osr`: all 3 icons
==** retail (←@(336,210)f3 / →@(378,210)f1 / X@(224,266)f9), `icons_compare.png` shows the
line pixel-identical.  Retires `PORT-DEBT(dialogue-arrow-art)` entirely (the book indicator +
the inline key-caps).  RESIDUAL: only ←/→/X are mapped (the codes the errands line uses);
other `@@` codes (↑/↓ = SJIS 0x81aa/0x81ab → atlas row 0 cells 0/2; Z=frame 8, C=10) are
unknown-skipped until a line exercises them (then verify the frame + add to the token map).

## The freeroam HUD banks

Same story — every HUD source (`freeroam-hud.md`) is a `sotesd.dll` DATA resource in
`res_loads.jsonl`; loadable from the user's file.  The HUD port is the layout + party-data
work (`freeroam-hud.md` §2-4), not a bank-source blocker.

## Artifacts
`runs/ui-bank/` — `cap/` (the Frida capture: `call_trace.jsonl` pins + `res_loads.jsonl`),
`sotesd/` (extracted resources), `atlas_0x455_labeled.png`, `retail_icons_body.png`,
`retail_t1823.bmp`.  Spec: `tools/flow/ui_bank_fields.json`.
