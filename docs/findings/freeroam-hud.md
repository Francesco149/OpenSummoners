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

**Scoping correction (ckpt 172, after actually reading these decompiles — bump the estimate):**
none of slices 2-3 are simple static icons; each is its own multi-checkpoint RE, same class as the
portrait, not a quick add-on:
- **`0x4969b0` (door indicator) is a FULL off-screen multi-exit compass-arrow system**, not a fixed
  (200,415) icon: scans a 32-actor band for "exit" types matching a room/param relation, world→screen
  projects each against a 72000×56000 view bound, DEDUPES/stacks actors landing within 5px of each
  other (offsetting ±12px by which screen edge), picks 1-of-8 frames (4 directions × normal/
  highlighted — highlighted when `param_4==`the actor, i.e. "the door you're standing at"), and
  alpha-fades near the bound via the `DAT_008a9308` ramp table (the ckpt-163/171 blend-LUT family).
  ckpt-167's single observed draw (200,415) is just this scene's ONE passing actor — the general
  mechanism needs the full multi-door case understood before it's faithfully portable.
- **`0x497c20` (bottom-left strip) is a "quick item" HUD slot**: a mode selector at `param_2+0x4070`
  (0=nothing / 1="Quick heal" text / 2=an inventory item name+count via `param_2+0x4078/0x407c`) drives
  an optional TEXT row, ALWAYS drawing a 3-segment animated bar (`FUN_005b9ae0`×3) + frame + icon
  underneath regardless of mode. In solo-Arche errands mode is presumably 0 (no quick item bound) —
  but "the base bar always renders" means this is NOT the omit-when-0 pattern the EXP gauge used;
  needs the `param_2+0x40xx` quick-item struct RE'd even for the always-on part.
- **`0x4975e0` (bottom-right cluster) looks like COMBAT UI** (a small gauge via the shared
  `FUN_00498f10`, a weapon-range/hitbox visualization via `FUN_004505c0` + a clipped blit, and a
  cooldown-percent icon via `FUN_00497a00` keyed off `in_ECX+0x3c4`) — likely inert in the peaceful
  errands but its OWN unconditional tail draw (`FUN_005b9b70` at the end) still needs the exact
  position math traced, not assumed.
- **`0x4962a0` (item slot, ×6) is the simplest of the four (242 B)** — a slide-in-positioned keyed
  blit (frame) + a conditional 2nd blit (icon present) from two DIFFERENT pool slots
  (`DAT_008a76d0`/`DAT_008a76f0`) — but its caller/loop (`0x496ec0`? unread this ckpt) still needs
  confirming for the per-slot item-icon/qty source.
None of these are "next chip" material for a quick add-on pass; each wants its own dedicated
checkpoint (RE the data struct, ground-truth the OFF state in `sword2.osr`/`retail.osr`, port, verify)
the same way the portrait does. Picking back up here: start with `0x4962a0`'s wrapper (find what loops
it ×6 and what per-slot icon/qty data it reads) since it's the smallest surface area.

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

## 8. The ITEM BAR (`0x4962a0` x6) — PORTED + verified bit-exact (ckpt 173)

Picked up from the ckpt-172 scoping note ("start with `0x4962a0`'s wrapper... since it's the
smallest surface area").

**The wrapper question, answered:** there is no `×6` loop anywhere — `FUN_00494e60` (the SAME HUD
orchestrator as every other slice) calls `FUN_004962a0` **six times inline**, once per slot
(`:377-449`), each with a different literal slot index and a per-slot frame-selector expression.

**The mechanism (static disasm — Ghidra's decompile hides all three thiscall/pool reads here, same
trap as the ckpt-171/172 portrait hunt: `objdump -d -M intel --start-address=0x4962a0
--stop-address=0x496392 vendor/unpacked/sotes.unpacked.exe`):** `FUN_004962a0(dst, slot, frame,
vslide, hslide)` issues **three keyed blits at the SAME (x,y)**:
1. **BG (ALWAYS):** bank `DAT_008a76f0` = pool idx **0x39** (retail addr = `0x8a760c+4*57`) = **res
   0x450**.  NOT frame 0 — `FUN_004184a0(P,0)`'s literal `0` is the lazy-decode **entry_idx** (it's
   the SAME function as `ar_sprite_decode`/`FUN_00418470`'s sibling, confirmed by reading
   `FUN_004184a0`'s own decompile: `piVar1 = *in_ECX + (param_1&0xffff)*8` — an `entries[]` index, not
   a frame select).  The ACTUAL drawn frame is a **hardcoded `info+0x30` cache slot = `frames[0x30/4]
   = frames[12]`**, baked into the call site itself (both `FUN_004962a0` and the SEPARATE multi-party
   portrait-row renderer `FUN_00496560`/`FUN_00498960` share this exact bank+offset — a common "slot
   background" asset).  Ground-truthed empirically (a brute-force dhash sweep, see below) after a
   naive `ar_sprite_slot_frame(bg,0)` produced a WRONG, differently-shaped cel (see the "wrong asset"
   pitfall below) — reading the asm's `[edx+0x30]` correctly predicts frame 12.
2. **ICON (`if (param_3 != 0)`):** bank `DAT_008a76d0` = pool idx **0x31** — **the SAME bank
   `HUD_STAR_POOL_IDX` already decodes** (res 0x44f, confirmed via `FUN_0057a330`'s registration
   table: `{36, 0x44f, 0x20, 0x20, 0, 0, 2}` = `g_ar_sprite_slots[36]`, pool_idx 49 = idx 36 after the
   13-slot ramp-count offset `ar_pool_get_slot` applies).  Frame = `param_3` directly (array-indexed:
   `array[param_3]`, `array = *(*P)`).
3. **LABEL (ALWAYS):** the SAME bank, frame = **`slot_index + 4`** (`array[(slot+4)&0xffff]`) — an
   F1..F6 key-cap glyph, confirmed by dhash-extracting the sheet: literal "F1"/"F2".../"F6" bitmaps at
   frames 4-9.  Independent of any game/party state — no PORT-DEBT needed for this draw.

Position (`0x4962a0`'s own math): `x = slot*0x20 - (hslide*200)/1000 + 0x1b8`, `y = (1000-vslide)*
0x80/1000 + 0x1bc`.  `vslide`/`hslide` are the caller's `room+0x3c8`/`room+0x388` — traced into
`FUN_0049af40` (`hud_party_anim_update`, already annotated in `retail_fields.json` but with no
`fields` yet — ckpt 90/95 left it "for the eventual HUD port"):
- **`room+0x3c8` (`in_ECX[0xf2]`)** is a plain **room-active ramp**: `+20/tick` toward 1000 while
  `*room != 0` (HUD context active), `-20/tick` toward 0 otherwise.  A slower sibling of the panel's
  own `+0x1b0` progress (`+50/tick`) — SEPARATE counter, not reused.  Ported as `hud_item_slide_step`
  (`hud.c`), armed at the SAME control hand-off as the panel (`main.c`, `g_hud_item_slide_prog`).
- **`room+0x388` (`in_ECX[0xe2]`)** is the **door-proximity GLOW ramp**: `+40/tick` while a tracked
  door actor (`room+0x398` = `in_ECX[0xe6]`) is within a screen-space distance bound, `-10/tick`
  otherwise (`-20/tick` if the room itself is inactive).  This is the SAME subsystem backing the
  (deferred, ckpt 172) door indicator — `PORT-DEBT(hud-item-hslide)`, pinned at its observed floor
  (0 — Arche is not near a door in the errands ground truth: the captured item-bar x's are exactly
  `slot*32+440`, zero offset).

**Ground-truthing the 6 per-slot data sources** (all via `room+0x4030`'s 8-slot party array or plain
room fields, decompiled `FUN_004961a0`/`FUN_00496240`/`FUN_004961e0`/`FUN_00496170`):
- slot0 = `0x2c + (FUN_004961a0()!=0)` (an 8-slot party scan bool)
- slot1 = `0x30`/`0x31` keyed on `room+4->+0x4070` (the SAME "quick item bound" flag `FUN_00497c20`
  reads — ckpt 172's scoping note)
- slot2 = `0x28 + clamp(room+0x4050, 0, 2)`
- slot3 = `0x21 + (room+0x4054, switch 0..4)`
- slot4 = `0x38 + (leader char+0x750+0x140, mapped 1/2/3->0/1/2, else 3)`, gated on `FUN_00496170()
  == 1` (itself `room+0x14898` unless `room+0x1d4==0xc35a && *(leader+0x750+0x466)==0`)
- slot5 = `0x50 + clamp(room+0x4058, 0, 2)`

**Brute-force dhash sweep (the verification method — no live/replay capture needed, sidestepping the
§7 replay-fidelity trap entirely):** since all the frame-selector logic funnels through TWO already-
registered, already-loadable pool banks, a temporary debug pass (`--hud-item-probe`, reverted after
use) blitted pool 0x39 frames 0-29 and pool 0x31 frames 0-89 in an on-screen grid at BOOT (before the
title scene — `ar_register_group3_sprites` registers both banks unconditionally in `init_sprite_banks`,
so no errands navigation is needed), captured via `--osr-emit`, then dhash-matched every grid cell
against the 13 target dhashes extracted from `sword2.osr` tick 2200 (`osr.py`'s `Sheet.pixels`, same
technique as the ckpt-172 portrait pixel pull).  **Exact 1:1 match, first sweep:** icons `{44, 48, 40,
36, 59, 80}`, labels `{4,5,6,7,8,9}`, BG **12** — confirming both the mechanism reading AND the
retail-side default field values (a fresh new-game errands: `room+0x4054==3` was the one surprise,
the rest defaulted to their 0 floor) in one shot.

**Pitfall avoided — the grade-skip trap (ckpt 147/171's class of bug, hit again):** the FIRST full
in-game verification (a real errands playthrough via `sword2-nav/held.jsonl`, not the boot-time
probe) showed the ICON+LABEL draws dhash-exact but the BG draw WRONG (`0x34adc1b6` vs retail
`0xc6faa77e`) — the boot-time probe decoded the bank BEFORE the port's global 8bpp colour-grade ever
touched it, but the real in-game path grades it (plain-`info+0x30`-cache sheets are NOT graded by
retail, same class as the frame/stars/EXP — `title_sheet_format`'s grade-skip list, `main.c`).  Fixed
by adding `HUD_ITEM_BG_BANK_SLOT` (44) to the skip list.  Lesson restated: a bank passing ONE
verification context (boot-time synthetic blit) can still fail the REAL render path — the full
errands capture is the only trustworthy verification, the boot probe is for frame-ID discovery only.

**VERIFIED end-to-end:** a fresh `port-hud-item.osr` (`tools/run-opensummoners.sh --frames 8500
--osr-state --osr-emit ... --input-trace runs/sync/sword2-nav.jsonl --held-trace
runs/sync/sword2-held.jsonl`, `OPENSUMMONERS_TIMEOUT_MS=220000` to let the real-time sim reach the
errands hand-off) vs `sword2.osr` at tick 2200: **all 18 blits** (BG+icon+label x 6 slots) match
retail on dst position, size, AND dhash — **0 mismatches**.  A manual RGB565 composite of both sides'
item-bar region is visually identical (pushed to the feed).  1077 host pass (+3:
`hud_item_slot_position`, `hud_item_slide_step`, `hud_item_icon_frames`).  `PORT-DEBT(hud-party-
context)` extended (the 6 icon frames); NEW `PORT-DEBT(hud-item-hslide)` (the door-glow floor) +
`PORT-DEBT(hud-slide)` registered properly in `port-debt.md` (previously code-commented only).

## 9. The DOOR INDICATOR (`0x4969b0`) — ALGORITHM ported bit-exact; actor SOURCE is new PORT-DEBT (ckpt 174)

Picked up ckpt-172's own scoping note (§4): "`0x4969b0` (door indicator) is a FULL off-screen
multi-exit compass-arrow system, not a fixed (200,415) icon."  Confirmed and RE'd completely via
`objdump -d -M intel --start-address=0x4969b0 --stop-address=0x496ec0
vendor/unpacked/sotes.unpacked.exe` (Ghidra's decompile hides FOUR separate ECX-hiding traps here —
more than any prior HUD slice — so static disasm was load-bearing, not optional).

### The mechanism (fully RE'd)

`FUN_004969b0(dst, ref, cam, highlight)` scans `DAT_008a9b50+0x1160` (a 32-slot actor-pointer band —
the SAME EFFECT-type band `actor_spawn.h` documents for the town's townsfolk, engine-quirk #84,
though whether errands "doors" and town "townsfolk" are the SAME kind of EFFECT actor is unconfirmed).
Per candidate `iVar7`:

1. **Validity filter:** `iVar7[+0x1d0]==1` (active) AND `body[0]==1` (body-valid, `body=*(iVar7+0x40)`)
   AND `body[0x7e]==0` (not suppressed) AND `*(char+0x750)+0x44a==0` (status clear) — else skip.
2. **Zone gate:** both `ref[+0x1dc]` and `cand[+0x1dc]` non-zero, AND (either is the wildcard `3`, or
   they differ) — else skip.  (`ref` = param_2 = `hud_ctx+0x24`, a per-member field NOT yet identified
   elsewhere; likely the leader's actor pointer, cached at the SAME leader-match block ckpt 171/172
   found never fires under scripted replay — moot here since sword2.osr is a real human capture.)
3. **Reach pre-filter:** `abs(ref_cx - cand_cx) < 72000 && abs(ref_cy - cand_cy) < 56000` (world units;
   `cx = x + w/2`, `cy = y + (h-baseline)/2 + baseline` — a baseline-relative Y, not a plain center).
4. **On-screen exclusion:** `FUN_004766a0(cand.x, cand.y, cand.w, cand.h, margin=0)` against the
   camera (`this=param_3=DAT_008a9b50+0x104c`, the SAME `camera_view`/`mr_camera` object
   `camera_follow.h`/`map_render.h` already model) — if the candidate's world rect intersects the
   viewport, skip (the indicator is OFF-screen-only, by design).
5. **Fade depth:** `depth = min((72000-adx)*1000/32000, (56000-ady)*1000/24000)` — small near the
   reach-box edge (far away), large near the viewport (about to come on-screen).
6. **Project + clamp:** `(cand_cx,cand_cy)` through the camera (`/100` each term BEFORE subtracting —
   matters under truncating division), clamp to `[0,vp_w/100] x [0,vp_h/100]`.  Which axis clamped
   picks an EDGE direction (0=TOP default, 2=BOTTOM, 1=RIGHT, 3=LEFT — three sequential overriding
   `if`s, not else-if, so LEFT beats RIGHT beats BOTTOM beats TOP).
7. **Dedup/stack:** up to 20 buckets, matched within `<5px` on both axes; a match increments the
   bucket's count (0..4 draw, 5+ silently drop the draw but still update the bucket); a bucket miss
   with the table already full **aborts the entire 32-actor scan** (retail's bare `return;`, not a
   per-candidate skip).  A match's draw position is nudged `stack*12px` PERPENDICULAR to its edge,
   INTO the screen (TOP: +Y, BOTTOM: -Y, RIGHT: -X, LEFT: +X) — spreading stacked arrows toward the
   interior rather than along the edge (avoids corner overflow).
8. **Highlight:** if `highlight==iVar7` (pointer identity — param_4 = `hud_ctx+0x398`, the SAME
   tracked-door field `hud-item-hslide`'s glow ramp reads), the edge index +4 (8 total dir+hilite
   frames).  Final `frame_index = dir + 4` (retail's own unconditional `+4` on top — frames 0-3 of the
   bank are unused by this renderer).
9. **Draw:** `ramp_idx = depth*20/1000` (unreachable-negative case omitted, proven unreachable by step
   3's gate); `ramp_idx>=20` → **OPAQUE** keyed blit (`FUN_005b9b70`, this=frame, offset internal);
   else → **ALPHA** blit through `g_ramp_b[ramp_idx]` (`FUN_005bd550`/`zdd_blit_orchestrate`, desc=ECX
   NOT `frame+0x28` — that's the *colorkey* arg; the offset (`frame.metric_0c/_10`) must be added by
   the CALLER for this path, unlike the keyed path).  `DAT_008a9308` = **`g_ramp_b`**
   (`g_pd_boot_group_b`, already ported/used by the EXP gauge/sword-trail/particle sky-fade —
   `pixel_drawer.h`), so NO new asset work was needed.  `DAT_008a76f4` = pool idx **0x3a** (58) = the
   ALREADY-REGISTERED `g_ar_sprite_slots[45]` = **res 0x451, 64x64** (`asset_register.c:1727` — no new
   registration needed either; it registers unconditionally at boot, same as every sibling HUD bank).

**The 4 ECX-hiding traps** (verified byte-for-byte in the disasm, none visible in Ghidra's pseudocode):
`FUN_0044e640`/`FUN_0044e680` (candidate center x / y) run `this=iVar7` (the loop actor); the SECOND
`FUN_0044e680` call (reference center y) runs `this=param_2` — textually identical
`FUN_0044e680(0)` calls in the decompile, but `mov ecx,edi` vs `mov ecx,ebx` between them; `FUN_004766a0`
(on-screen test) runs `this=param_3` (the camera); the final blit's blend descriptor is `mov ecx,esi`
(`esi` = the ramp-table lookup) immediately before `call 0x5bd550` — NOT `frame+0x28` as the
positional-arg reading would suggest (that's `param_9`/colorkey).

### PORT-DEBT(hud-door-actors) — the algorithm is ported; the actor source is not

Ported `hud_door_process` (`hud.c`/`.h`) as a pure, host-tested function taking one candidate + the
scan-wide dedup state; `main.c game_render_hud` loops it.  **What's NOT portable yet:** the `+0x1160`
EFFECT-band actor SPAWN itself — a whole unported subsystem, not a missing constant.  Recon (a
throwaway `g_town.map.layers` dump, reverted after use) found the errands room's map data has exactly
**2** EFFECT-band (50000-59999) objects: code 50240 @ world (62400,28800), code 50140 @ world
(48000,48000) — real, data-sourced positions, not guesses.  But **both stay on-screen at every
reachable camera position** in the errands (108800x64000 map, `[0,44800]` horizontal-only scroll,
64000x48000 viewport) — verified independently in Python across the full `sword2-nav.jsonl` camera
trajectory (screen-x never leaves `[0,640]` for either).  So this port-debt's OBSERVABLE effect is
"zero door-indicator draws in the errands," which matches ground truth exactly: a full-session scan of
`sword2.osr` (real human play, the SAME session ckpt 167-173 ground-truth everything else against)
finds **zero** `res=0x451` blits too.  ckpt-167's original single "(200,415,24,42)" observation (§1)
must have come from a session this project no longer has a capture for; the two data-sourced EFFECT
objects found this ckpt are not proven to be THAT observation's source (may be something else
entirely — a shop trigger, not an exit — or the real exit position derives from the room-registry's
reciprocal-exit table (`game_world.c gw_cross_reference`, `{key,target_room_id,return_field}` slots)
rather than a plain map object; that table gives CONNECTIVITY, not a world position, so if it IS the
source, the position must come from cross-referencing its `key` against a map object some other way —
unresolved, next-session territory).

**VERIFIED (the strongest check available without a positive-case capture):** a fresh
`port-hud-door.osr` (same recipe as §8, `--osr-state`) over the full errands replay: **0** `res=0x451`
blits (matches `sword2.osr`'s 0), and the item-bar (`res=0x450`)/star (`res=0x44f`) dhashes are
UNCHANGED from §8's ground truth (`0xc6faa77e` / `0xaedb8faa`) — the new `title_sheet_format` grade-skip
entry (`HUD_DOOR_BANK_SLOT`, added proactively since the door bank binds via the SAME
`FUN_004184a0(0)` plain-getter dispatch as the item-bar BG, `0x496e0e`) caused no regression.  The
ALGORITHM is host-tested (`test_hud_door_edges/_filters/_dedup_stack/_highlight/_dedup_exhaustion`, 5
tests covering every branch — all 4 edges, every filter/gate, dedup clustering + the 5-stack cap +
12px perpendicular offset, the highlight bump, and the 20-bucket exhaustion abort) against fixtures
independently cross-checked in Python (fresh reimplementation of the tdiv/center-x/center-y formulas,
not calls into the C code under test) before being baked into the C assertions.  1082 host pass (+5).
