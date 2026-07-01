# The freeroam status HUD — RE + port scoping (ckpt 152)

The `res=0` freeroam status HUD (USER `osr_notes.jsonl` #7-9 on `port-errands.osr`,
errands tick 2413): the top-left leader panel (portrait + HP/MP bars + numbers +
level + element stars), the bottom-left/right strips, the bottom-right item bar, and
the door indicator.  This is the full drawcall ground truth (off `retail.osr`) + the
render architecture (off the decompile) + the port plan.  See also
`errands-render-gaps.md` §2 (the USER note crops/ticks).

## 1. The drawcall ground truth — retail.osr tick 2413 (the HUD overlay layer)

The HUD draws AFTER the world (seq 0-461 = the room backdrop/tiles); the HUD is the
overlay layer, **seq 462-536** (`tools/trace_studio2/hud_probe.py retail.osr 2413`).
All HUD source blits are `res=0` (system/UI banks, resource_id 0) + GDI TextOutA:

**Top-left leader panel** (panel x-base = 1 when fully slid in):
- seq 462-469 — **HP bar**: 4× `rects` strips `(118,7,168,2)`,`(117,9,..)`,`(116,11,..)`,`(115,13,..)`
  (x −1/row = the italic skew; w=168=`0xa8`; src y 0) + width-0 `alpha` companions (HP full).
- seq 470-475 — **MP bar**: 3× `rects` strips `(118,21,168,2)`,`(117,23)`,`(116,25)` (src y 2).
- seq 478 — **panel frame**: `keyed res=0 (-31,1,286,90)` (the ornate border, drawn over the bars).
- seq 481-494 — **HP text** `' 100 / 100'`: dark `0x202020` outline (x 217-220 × y 4-6) + white
  `0xffffff` center (218-219, 5); font 2; bk=1 (transparent).  Format `"%s / %d"` (cur formatted, max).
- seq 495-508 — **MP text** `'  20 / 20'`: same outline pattern, base (218,18).
- seq 509 — **level glyph**: `keyed res=0 (161,25,8,14)` (the "Lv 1" digit; a UI-bank cel, NOT text).
- seq 510 — **leader portrait**: `keyed res=0 (1,1,82,89)` (Arche's face).

**Bottom-left** (seq 513-516): small `keyed res=0` glyphs `(40,456,13,11)`,`(49,456,10,14)`,
`(112,456,13,11)`,`(121,456,8,14)` — two small numeric/icon clusters (gold / time, TBD).
**Bottom-right item bar** (seq 518-535): **6 slots** at x=440,472,504,536,568,600 (step 32), y=444;
each slot = a 32×32 frame + an item icon (~24-30²) + a 14×10 quantity glyph (3 blits/slot).
**Door indicator** (seq 536): `keyed res=0 (200,415,24,42)` (the exit-door arrow).

## 2. The render architecture (off the decompile)

- **`FUN_00494e60` (0x494e60, 3904 B) = the HUD render orchestrator.**  Called **×2** from the
  in-game render driver `FUN_0048c150:165` (`do { FUN_00494e60(paint_ctx); } while(--i)`), AFTER the
  world layers + fade grid.  Its `this` (ECX) = the scene/room object (`DAT_008a9b50`-rooted); the
  party slots are at **`room+0x4030`** (8 slots, stride 4 → each a `*char` ptr; the char data is
  `*(slot+0x9f4)+0x750`).  It reads a HUD context with fields incl. `+0x10` (slide phase 0..3),
  `+0x1b0` (slide progress 0..1000; x-base `= ((prog*0xb−11000)*0x20)/1000+1` → 1 when in),
  `+0x1b4` (leader id), `+0x1b8`/`+0x1bc` (HP/MP ratios), `+0x340`/`+0x20` (per-member sub-structs),
  `+0x388`/`+0x398`/`+0x3c8`/`+0x3d8` (item/element sub-objects).
- **Sub-renderers** (all called by 0x494e60):
  | VA | role | draw |
  |----|------|------|
  | `0x498680` | HP bar (401 B) | `rects` filled + `alpha` depleted, N rows, x-skew, samples pool `0x2f` gradient (src y = color) |
  | `0x498820` | MP bar | same, 3 rows |
  | `0x498f10` | small gauge/icon bar | `rects`/`alpha` |
  | `0x498620` | element star | keyed blit, per-star |
  | `0x43e250` | **outlined text** (334 B) | dark `0x202020` 3×3 (or 4×4) TextOutA outline + white center; font `DAT_008a9274[idx]` |
  | `0x495e40` | a text/number draw | TextOutA |
  | `0x497f40` | per-member element row | bars + keyed |
  | `0x498960` | per-member | |
  | `0x496560` | multi-party portrait row (party>1) | |
  | `0x497c20` | bottom-left strip | |
  | `0x4963a0` | bottom strip | |
  | `0x4962a0` | **item slot** (×6) — the bottom-right item bar | frame+icon+qty |
  | `0x4975e0` | bottom-right cluster | |
  | `0x496ec0` | bottom strip (gated `DAT_008a6e80+0xcc==0`) | |
  | `0x4969b0` | **door indicator** | keyed blit |
  | `0x495fe0` | screen overlay/fade (×2, conditional) | alpha |
- **Sprite blit mechanism**: `FUN_00418470(N)` returns frame N's descriptor of the current UI bank
  (the decompiler hides the `mov ecx,eax`); `FUN_005b9b70(dst,x,y)` blits THAT descriptor (reads
  `this+0x2c` surface, `+0xc/+0x10` offset, `+0xb8/+0xbc` size, `+0xd4` blend).  The port already
  models this (`ar_pool_get_slot` + `ar_sprite_slot_frame`).
- **The animator** `FUN_0049af40` (the HP/MP-ratio + portrait-fade lerper, retail_fields
  `hud_party_anim_update`) runs 2×/sim-tick from `0x499ab0:180` — it produces the `+0x1b8`/`+0x1bc`
  ratios 0x494e60 renders.  (Not the renderer.)

## 3. Dependencies (why this is multi-checkpoint, not a one-liner)

1. **Source sheets** — every visual element (bars' gradient pool `0x2f`, frame, portrait, star,
   level glyph, item icons, door arrow) reads a `res=0` UI bank.  **Recoverable** from retail.osr:
   the capture grabs source pixels for ALL blits incl. res=0 (`engine_hooks.h sheet_capture_source`,
   keyed by dhash; UI/panel sheets re-grabbed).  So extract each HUD source sheet by dhash from the
   `.osr` SHEET stream → port asset (the captured-asset pattern, like the fire ramp / npc palette).
   OPEN: whether these banks are instead loadable from the user's own files via the existing
   asset_register (find their real pool indices / the HUD init that registers them) — preferred if
   cheap, since it avoids shipping captured pixels.
2. **HUD context / party data** — the values (HP 100/100, MP 20/20, Lv 1, 2 element stars, the 6
   item-bar contents, the leader) come from the party subsystem (`room+0x4030`), which is **unported**
   (the port's freeroam Arche is a standalone `character` mover, not a party slot).  Faithful path:
   a captured **`PORT-DEBT(hud-party-context)`** stand-in with the errands values + a faithful render,
   retired when the party subsystem lands (the cutscene-coords / ERRANDS_CAST precedent).
3. **Render hook** — call the HUD render after the world/effect render in `main.c`, gated on the
   freeroam/errands room (`g_freeroam_active && !room_is_town`), emitting through the port's blit
   primitives so it shows in the port `.osr`.

## 4. Port plan (incremental, verify each slice bit-exact vs retail.osr)

1. **Top-left panel** — HP/MP bars (`0x498680`/`0x498820`) + numbers (`0x43e250`) + frame + portrait
   + level glyph + element stars.  The coherent first visual unit.
2. **Item bar** (bottom-right, `0x4962a0` ×6) + the bottom-left strip (`0x497c20`).
3. **Door indicator** (`0x4969b0`).
4. Retire `PORT-DEBT(hud-party-context)` when the party subsystem is ported.

Verify: `draw_probe`/`hud_probe` (the rects/keyed positions), the TEXT records (the numbers),
`osr_prof` recon (`differ_px==0` per region).  `tools/trace_studio2/hud_probe.py` is the ground-truth
probe (dumps the HUD layer at any tick).

## 5. Slice 1c-1 (ckpt 169) — element STARS bit-exact; LEVEL blocked on the ramp-palette gap

Ground truth re-probed off `sword2.osr` tick 2200 (`hud_probe` + a dhash→SHEET resolve),
the top-left leader panel's data glyphs (seq after the frame at 495):

| seq | element | res | frame | dst | dhash |
|----|----|----|----|----|----|
| 493-494 | EXP gauge | 1102 (0x44e) | 0 | (144,42) w104 | 0x3a65dc81 |
| 496-497 | element stars ×2 | 1103 (0x44f) | 16 | (187,30),(200,30) 12×9 | 0xaedb8faa |
| 526 | level '1' | res0 (ramp0=0x413) | 16 | (161,25,8,14) | 0x192317ef |
| 527 | portrait | res0 | — | (1,1,82,89) | 0xbbf24c22 |
| 528 | portrait sub-blit | 1909 (0x775) | 0 | (92,29,38,16) | 0x72a588ef |

Bank resolution (all "free" — registered by `ar_register_palette_ramps`): stars =
`ar_pool_get_slot(0x31)` = `g_ar_sprite_slots[36]` = res 0x44f (the icon sheet, plain
getter `FUN_004184a0(0)` in `0x498620`); level = `ar_pool_get_slot(1)` =
`g_ar_sprite_ramp_slots[0]` = res 0x413 (the small-font ramp, plain getter in `0x495e40`).

**STARS — PORTED + VERIFIED bit-exact.**  `game_render_hud` blits res 0x44f frame 16 keyed
at `(xbase+0xba+k·0xd, ybase+0x1d)` for `k=0..1` (Arche's 2 affinity stars, PORT-DEBT(hud-
party-context) stand-in count/element).  Added slot 36 to the 8bpp grade skip-list (plain-
getter UI sheet, NOT graded — same class as the bars/frame).  Verified off `port-hud1c.osr`
vs `sword2.osr` tick 2200: both stars res 1103 fr16 dst (187,30)/(200,30) 12×9 **dhash
0xaedb8faa — byte-identical** to the recording.  Geometry host-tested (`hud_star_level_positions`).

**LEVEL — PORTED + VERIFIED bit-exact (slice 1c-2, ckpt 170).**  The geometry is exact (glyph
`c-0x21`, +9px advance, base (161,25) — `hud_glyph_frame` + `HUD_LEVEL_*`, host-tested); the
bank is ramp0/res 0x413 frame 16 ('1').  `game_render_hud` blits the value string left-to-right
via `hud_glyph_frame` from `ar_pool_get_slot(HUD_LEVEL_POOL_IDX=1)`.  Verified off
`port-hud1c2.osr` vs `sword2.osr`: res 1043 fr16 dst (161,25,8,14) **dhash 0x192317ef —
byte-identical** to the GT (was 0x14573bd0 too-dark); the digit is the ONLY ramp draw across the
whole title→newgame→cutscene→errands run (3956 draws, all 0x192317ef), so no other ramp consumer
regressed; bars/frame/stars dhashes unchanged.

### The ramp custom-palette gap — RESOLVED (the LEVEL blocker, slice 1c-2)

The digit rendered **one grade-step too dark** (dhash 0x14573bd0 vs GT 0x192317ef; the '1'
outline read 0x303030 vs retail 0x404040) because the port sliced res 0x413 against its raw
EMBEDDED palette (entry 1 = 0x333333) instead of the installed custom ramp palette
(`entries[0].b` = 0x404040 that `ar_run_palette_ramp` built).

**Root RE'd (the exact retail bind, not curve-fit):** `FUN_004184a0` (the frame-getter decode =
the port's `ar_sprite_decode`) does, right after decode and BEFORE the slice (`:70-73`):
`if (entries[frame].b != 0 && session 8bpp) FUN_005b7bd0(entries[frame].b)`.  **`FUN_005b7bd0`**
overwrites the session's bmiColors (session+0x34, RGBQUAD B,G,R,0) from the installed
PALETTEENTRY buffer (R,G,B,_) — the exact inverse channel-swap of `bs_emit_palette_bgra`.  So a
slot that had `ar_palette_install` run (the 12 ramps + the title seed slot 0) slices against its
INSTALLED palette.  The port omitted this bind → ramp glyphs used the embedded palette.

**Fix (ckpt 170):** `bs_install_palette` (FUN_005b7bd0, `bitmap_session.c`) + a call in
`ar_sprite_decode` right after decode-success, gated exactly as retail (`entries[0].b != NULL &&
8bpp`).  Plain sprite slots never `ar_palette_install` (entries[0].b == NULL) so it's a no-op for
them.  ALSO: the ramp banks are plain-getter (no 0x417c40 grade descriptor) so retail does NOT
grade them — added the `g_ar_sprite_ramp_slots` range to the `title_sheet_format` grade-skip (else
the bound 0x404040 grades back to 0x333333, undoing the bind).  **No regression:** the title seed
(slot 0) is never drawn as a sprite (verified: res 0x90b absent from the run), and res 0x413 is the
only ramp drawn (bit-exact).  `PORT-DEBT(hud-ramp-palette)` RETIRED.  Host: `bs_install_palette`
×2 (swap + roundtrip-vs-emit).

## 6. Slice 1c-2 remainder (ckpt 170 scoping)

### The EXP gauge (`FUN_00498f10`) — PORTED + VERIFIED bit-exact (ckpt 171)

Call site `0x494e60:95`: `FUN_00498f10(ctx, xbase+0x8f, ybase+0x29, 1, 4, cur=0, char+0xe8,
char+0xec /*max*/, 0x68 /*w=104*/, 0x2e /*pool idx → g_ar_sprite_slots[33] = res 0x44e*/, 0, 1)`.
Position **(144,42)** confirmed.  Two spans (ground truth `sword2.osr` tick 2200):
- **seq493 FILLED** — `blt_rects` (mode 2) dst `(144,42,0,2)` src `(104,4)` — **width 0** (Arche's
  errands EXP = 0: `cur=0` and `char+0xe8=0`; there's no combat in the errands, so it stays 0).
  A 0-width no-op → OMITTED, like the HP/MP bars omit their 0-width depleted (ckpt 167).
- **seq494 DEPLETED** — `blt_alpha` (mode 4, `FUN_005bd550`) dst `(144,42,104,2)` src `(0,14)`
  **blend_ref=9** — the full empty gradient.  `498f10`'s display-mode branch
  (`*DAT_008a6e80+0x94==2` → `0x5bd550` alpha, else `0x5b9ae0` rects) takes the ALPHA path here.
  Ported as `zdd_blit_orchestrate(desc, primary, exp_cel, 144,42, 104,2, 0,14, ckey, NULL)`
  (`main.c game_render_hud`, right after the bars — retail's call order has EXP at :95, frame at :97).

**The blend desc — SOURCED (option a, content-match):** retail's `*(frame+0x28)` (the res 0x44e
frame-0 surface's attached blend) = blend_ref 9 in the `sword2.osr` capture, LUT md5 `ed6214bd`.
Dumped the raw LUT bytes for all 20 `g_pd_boot_group_a[]` + 20 `g_pd_boot_group_b[]` entries
(`pd_boot_init_slots(NULL)`, the exact byte layout `oe_blend_register` hashes — mode/shift/mask-derived
per-channel lengths, concatenated ch0‖ch1‖ch2) via a throwaway harness and `md5sum`'d each: **exactly
one full match — `g_pd_boot_group_b[8]`** (md5 `ed6214bd...`, matching the known-good sword-trail
cross-check `g_pd_boot_group_a[19]` → `727d856f` in the SAME sweep).  So `HUD_EXP_RAMP_B_IDX = 8`,
i.e. `g_ramp_b[8]` (mode 0, the banner/fade-family group — NOT group A like the fire/trail, which are
mode-1 dst-reading additive blends; the EXP depleted gauge is a mode-0 straight tint instead).
Added res 0x44e (`HUD_EXP_BANK_SLOT` = `g_ar_sprite_slots[33]`) to the `title_sheet_format` grade-skip
(plain-getter bar sheet, same class as the HP/MP bars idx 34/frame idx 57/stars idx 36).

**VERIFIED** off a fresh `port-hud-exp.osr` (`runs/sync/sword2-nav.jsonl` + `sword2-held.jsonl`,
`tools/run-opensummoners.sh --frames 8500 --osr-state`): tick 2200 seq483 `BLIT alpha res=1102 fr=0
dst=(144,42,104,2) src=(0,14) bmode=1` — dhash **0x3a65dc81 byte-identical** to the retail ground
truth.  1074 host pass (+`test_hud_exp_gauge_position`).  `PORT-DEBT(hud-party-context)` unchanged
(EXP pinned at 0, Arche's errands value).

## 7. The portrait bank-hunt (ckpt 172) — the ×2/frame call resolved; the ckpt-171
## "open contradiction" replaced by a DEEPER, better-evidenced finding: the leader
## slot-match never arms across a FULL scripted replay of the recorded session

**The ×2/frame call SOLVED (static disasm, `objdump -d --start-address=0x48c500
--stop-address=0x48c560`):** `FUN_0048c150` (the per-frame render driver) calls
`0x494e60` from a 2-iteration loop —
```
lea esi,[ebx+0x128]      ; esi = (0x48c150's own this) + 0x128 — array BASE
mov edi,0x2               ; 2 iterations
mov ecx, esi              ; ECX (0x494e60's thiscall this) = esi
call 0x494e60
add esi, 0x3ec            ; stride to the NEXT element
dec edi; jnz ...
```
So `0x494e60` renders a **2-element array of per-member HUD-panel contexts**
(`outer_this+0x128` and `+0x128+0x3ec`), not two unrelated calls — matches the
Ghidra decompile's misleading "`FUN_00494e60(*(...+0x16c))`" (same arg both
iterations; that's `param_1`, the *stack* arg used for blit calls, NOT the
`__thiscall this` Ghidra can't show without thiscall-tagging `0x494e60`).
`array[0]` is the real, populated panel (in the errands: room valid, `mode10=1`,
slot0 active — this is Arche's panel); `array[1]` stays all-zero (no 2nd party
member in the errands). Confirmed live: the two alternating ECX values a naive
per-N-calls throttle was sampling (`0xe51b0d8`/`0xe51b4c4`) differ by EXACTLY
`0x3ec`.

**THE DEEPER FINDING (supersedes the ckpt-171 "open contradiction" — same root
cause, but now proven EXHAUSTIVELY instead of via one placement-sensitive INT3):**
a native capture-proxy detour (`detour_add(0x494e60, ...)`, THROWAWAY — added,
used, then reverted; not in the committed tree) read `in_ECX+0x1b4` ("leader_uid",
the value each slot's `+0x9f4` must equal to match) **at *every single call*, both
array slots, for the ENTIRE replayed session** — `runs/sync/sword2-{nav,held}.jsonl`
replayed start-to-finish under `tools/capture_proxy/run_proxy.sh` (`OSS_TURBO=1`
default, ~840 fps): `game_enter` fired at **flip=1117** (exactly the value
CLAUDE.md's earlier verification cites), `array[0]`'s `room` and `slot0`
(`active=1`, a real `member` pointer) were populated from that flip onward, and
**all 219 nav-trace input injections landed (through the trace's last entry, tick
8014)** — yet `leader_uid` (`in_ECX+0x1b4`) read **exactly `0x0` on every single
call, both array slots, flip 1117 through flip 49478** (well past the entire
recorded session — the replay just idles once the trace is exhausted). This is
not a sampling artifact: the first probe pass DID have a real bug (a `calls%60==0`
throttle has a 50% chance of parity-locking onto ONE of the two alternating array
slots forever — here it locked onto the always-empty `array[1]` after call 20,
which is why the top-level line looked stuck at all-zero) — but the FIX (log on
any `ecx`/`room`/`leader_uid`/`mode10` state CHANGE instead of a call-count
modulus) removed that blind spot and the exhaustive re-run still shows `leader_uid`
never leaving `0x0`, for the panel that unambiguously carries the real character
(active slot0 in `array[0]`).

**Why this specifically blocks the portrait (and NOT the already-ported bars/EXP/
frame/stars):** the leader-match test (`:74-165`) is the single gate around the
*entire* leader-panel block, but bars/EXP/frame/stars are UNCONDITIONAL inside it
once the block runs at all — the port ships them as a **hardcoded `PORT-DEBT
(hud-party-context)` stand-in** (Arche's fixed HP/MP/EXP=0/level values), so
ckpt 167-171's "verified bit-exact" checks only ever confirmed the port's
hardcoded geometry against the CACHED `sword2.osr` (a real human session,
captured long before this ckpt) — none of them exercised a live/replayed RETAIL
run. The **portrait** is different: its bank/frame values are read from
`char+0x50` at the MOMENT the leader-match branch (`:125`) executes, so a stand-in
isn't possible — we need retail's REAL `char+0x50` value, which requires the
match to actually fire in a live capture. It never has, in any replay attempt.

**Conclusion:** the scripted ring-injection replay of the recorded `sword2` inputs
does not reproduce whatever internal event arms `hud_ctx+0x1b4` in the original
human-played session — a genuine **replay-fidelity gap**, not a probe-placement
bug (the ckpt-171 INT3 and the ckpt-172 field probe are two independently-built,
independently-blind-spot-free tools that agree). This is worth remembering for
*any* future investigation leaning on `sword2-{nav,held}.jsonl` for something
that depends on party/leader state specifically (movement/sword/attack chip work
never needed this — only the HUD's per-member data does).

**NEXT (a fresh session, two independent paths — either resolves this):**
(a) **find the actual setter of `+0x1b4`** — grep/decompile whatever writes it
(no direct assignment matched a repo-wide `grep "0x1b4) ="` over
`docs/decompiled/by-address/*.c`, so it's in a not-yet-decompiled function;
start from `FUN_0048c150`'s OWN callers/init path, or search for what else reads/
writes the `outer_this+0x128` 2-element array before the render call); (b) **stop
replaying — drive a REAL, live, human/manual play session** to the errands (no
scripted ring-injection) and probe `array[0]+0x1b4` then, since a human session is
what originally produced the sword2.osr renders in the first place. Either
resolves the bank id in one capture. `PORT-DEBT(hud-party-context)` unchanged;
`tools/flow/hud_portrait_fields.json` (a Frida field-spec attempt, superseded by
the native-proxy probe above because a *separate*, still-unresolved Frida-only
issue — an unmapped `btn=0x22` poll the ring-injection can't satisfy — blocks
`sword2-nav.jsonl` from even reaching `newgame_enter` under `frida_capture.py`,
independent of this session's tick/frame-axis fix below) keeps its field chains
for whoever revisits via Frida once that separate boot-nav issue is understood.

### The 82×89 face PORTRAIT (`FUN_00494e60:125-164`) — mechanism RE'd via STATIC DISASM; an
### open contradiction blocks the bank id (ckpt 171, not landed — next-session pickup)

A per-member descriptor at `char+0x50`: head-state `hud_ctx+0x1c8` selects the frame (`==2`→`+8`,
`==3`→`+10`, else→`+6`); main blit at **(1,1)** 82×89, then a sub-blit at **(92,29)** frame `+0x14`
(= res 0x775, registered idx 56).  The main face is **res=0** in the capture, **dhash 0xbbf24c22** —
a dedicated small-face bank, NOT the res-1000 dialogue bust (confirmed again this ckpt: extracted the
exact 82×89 RGB565 pixels straight off `sword2.osr`'s SHEET stream by dhash — `tools/trace_studio2/
osr.py`'s `Sheet.pixels`, no live capture needed for this part — and it IS a chibi face crop, a girl
with orange/brown hair + green eyes, distinct from every dialogue bust).

**Static disasm (`i686-w64-mingw32-objdump -d -M intel --start-address=0x494e60
--stop-address=0x495e00 vendor/unpacked/sotes.unpacked.exe`) pins the EXACT mechanism** — the
decompile's "`Bank = pool[*(char+0x50 +4)]`" is confirmed byte-for-byte at **0x495204-0x49520f**:
`mov cx,[eax+0x4]` (`eax` = the char+0x50 descriptor, from `mov eax,[edi+0x50]` at 0x4951d8, `edi`=
char, confirmed against the SAME register serving the star-count read `[edi+0xdc]` a few
instructions earlier) → `mov ecx,[ecx*4+0x8a760c]` (**the SAME unified pool table** `DAT_008a760c`
every other HUD element resolves through) → `call 0x418470` at **0x49520f** (MAIN blit, frame
selected by head-state) / **0x49525e** (the cross-FADE variant, `if (in_ECX+0x1d0>0)`) / **0x4952b4**
(the SUB-BLIT, frame `+0x14` = res 0x775 — this one keyed off a DIFFERENT global, `DAT_008a7720`,
right next to the frame's own `DAT_008a7724`, i.e. `+0x8a7720`/`+0x8a7724` are ADJACENT unified-pool
slot pointers).  Cross-check: the FRAME's own call (`0x418470(0)` at decompile :97) sits at
**0x49504d**, immediately preceded by `mov ecx,DWORD PTR ds:0x8a7724` — an EXACT, independent
confirmation that `HUD_FRAME_BANK_SLOT`'s "bank from the unpacked-exe asm" note (ckpt 167) was read
correctly, and that this disasm-reading technique is sound.

**THE OPEN CONTRADICTION:** a live capture-proxy probe (INT3 hook at **0x4951db**, the `test eax,eax`
immediately after `char+0x50` loads, reading `EAX`/`EDI` via the VEH CONTEXT — byte-verified installed,
`orig=0x85` matches `test eax,eax` exactly) **never fired once** across two full replays of
`runs/sync/sword2-nav.jsonl`+`sword2-held.jsonl` (`tools/capture_proxy/run_proxy.sh`, sim_tick reaching
into the tens of thousands, WAY past the confirmed portrait-render tick 1714+) — yet the SAME replay's
`.osr` draw stream shows the portrait rendering at that exact tick (dhash 0xbbf24c22, sliding in from
`dst_x=-333` in lockstep with the frame/bars, i.e. driven by the SAME `xbase` slide formula, so it
cannot be a coincidental unrelated draw).  So either (a) `char+0x50` really is null for Arche and the
observed portrait comes from a DIFFERENT, not-yet-identified code path that happens to track the same
slide-in `xbase` (seems unlikely to be coincidental, but not ruled out), or (b) the "leader-match" party
loop this code sits in (decompile :74-166, gated on `in_ECX+0x1b4 != 0` and a slot's `+0x9f4 ==
in_ECX+0x1b4`) never actually reaches this branch for Arche and the bars/frame/EXP/stars renders I
cross-checked as "confirming this code path executes" were actually misattributed (e.g. the SEPARATE
per-member-row mini-gauge code at decompile :270-297/:323-349, which ALSO calls `FUN_00498f10`/
`FUN_00418470` with constant frame args `0xd/0xe/0xf`, is a plausible alternate source for what looked
like corroborating hits).  **NEEDS (next session):** hook EARLIER still — e.g. at the loop's own
leader-match compare (decompile :80, `*(int*)(in_ECX+0x1b4) == *(int*)(iVar15+0x9f4)`) or at the
`in_ECX+0x1b4`/`in_ECX+0x1b0` reads themselves — to see which branch of the 8-slot loop actually fires
for Arche and reconcile which code literally draws the observed portrait.  A `--osr-state` field dump
(`osr_emit_state_field`, the port/proxy's opt-in named-field pass) at the render-site VAs would remove
the ambiguity in one capture.  `PORT-DEBT(hud-party-context)`: the descriptor (bank + head-state
frames) is Arche's leader stand-in until the party subsystem lands, unchanged either way.
