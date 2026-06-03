# Prologue — the Elemental-Stone intro cutscene (`FUN_0056cd20`)

Survey (ckpt 46) of the timed gem cutscene that plays between the new-game
config menu and the game proper.  This is the **prologue critical path** the
active goal (user @ ckpt 13) calls out: title → new-game menus → **prologue
(stone)** → first frame in-game.

## Where it sits in the boot driver

`FUN_00562ea0` (the post-launch driver, partially ported as `app_flow`), the
title-menu return-code switch, **case `0x1a` (Start / New Game)**:

```c
iVar11 = FUN_00564160();                              // config menu + dismiss anim
if ((iVar11 != 6) && (iVar11 = FUN_0056cd20(), iVar11 != 6)) {  // the STONE INTRO
  FUN_0059ec30(0,0,0x3f2);                            // game proper, map 0x3f2
}
```

So the path is **three sequential blocking calls**:

1. **`FUN_00564160`** — runs the new-game config menu (`FUN_00564780` case `0x24`,
   already ported as `newgame_*`) and, on Start-Game confirm, a brief ~19-frame
   **menu-dismiss animation** loop (`FUN_005642e0` × ≤0x13, `FUN_0056c930` ramp).
   Returns **6** if the config was cancelled (back to title).  `FUN_005642e0`/
   `_564690`/`_5640b0`/`_564110` are the **audio SP_MGR** (sound playback) — not
   the visuals; null-guarded on `DAT_008a7608` (the SP manager), which is null
   until audio (milestone 3) lands, so the dismiss loop is effectively a 19-frame
   wait in the port.

2. **`FUN_0056cd20`** — the gem cutscene proper (this document).  Returns **6** if
   the player pressed **abort (id 0x22)** → skip back to title; otherwise **0** →
   proceed.

3. **`FUN_0059ec30`** — the **game proper** outer loop (`FUN_0059f2c0` map engine +
   resource load/unload).  This is *in-game* and explicitly **out of scope** for
   now ("do NOT extend the trace toward in-game yet").  The scrolling **prologue
   narration text** (the "country of Scotsholm…" story) is part of this game-proper
   opening map, NOT of `0x56cd20` — `0x56cd20` draws **only sprites** (gem + aura +
   sparkles), no GDI text.

**In-scope deliverable: port `FUN_0056cd20` (the gem-on-black cutscene).**

## Sprite banks — ALREADY REGISTERED at boot

The cutscene draws from sprite-pool slots registered by `ar_register_main_sprites`
(= `FUN_005749b0`, group 4) — **already called at boot** (`init_sprite_banks` in
`main.c`).  Pool index = `(retail_BSS_addr − 0x008a7640) / 4`:

| retail global   | port slot              | res id | size (px) | type | role |
|-----------------|------------------------|--------|-----------|------|------|
| `DAT_008a7644`  | `g_ar_sprite_slots[1]` | `0x49f`| 224×224   | 2    | **aura** (big soft glow behind the gem; 2 frames toggled) |
| `DAT_008a7648`  | `g_ar_sprite_slots[2]` | `0x448`| 152×40    | 2    | **sparkles** (24-frame strip; the rising twinkle clusters) |
| `DAT_008a764c`  | `g_ar_sprite_slots[3]` | `0x4a2`| 144×108   | 2    | **gem** (the Elemental Stone; 35-frame `%0x23` glow loop) |

All three are `scale_flag=1`, `type=2`, colorkey 0 — same class as the title's
animated sprites.  So the assets are **decode-ready**; the cutscene resolves a
frame via `ar_sprite_slot_frame(slot, frame)` then blits.

## Rendering — existing infra suffices

- gem + aura + sparkles are drawn with **`FUN_005bd550`** = `zdd_alpha_blit`
  (ported, zdd.c:1797) — the additive/alpha shade blit, last arg an alpha-shade
  object picked from the **`DAT_008a9308` ramp** (20 entries; the title's `ramp_b`).
- when the chosen ramp entry is 0 the code falls back to **`FUN_005b9b70`** =
  positioned color-keyed blit (ported, zdd.c:1640).
- background is **black** (no bg sprite blitted in `0x56cd20`; `FUN_005b9410`
  begin-scene clears, `FUN_005b8fc0` presents).

## The loop structure (`FUN_0056cd20`)

One `do{…}while(true)` with a **fixed-timestep** sub-state machine `iVar9`
(`local_64`), same pattern as the title intro (GetTickCount accumulator `uVar20`,
16 ms = `0x10` steps):

- `iVar9==0` → first tick: pump (`FUN_005b1030`), set `iVar9=2`.
- `iVar9==2` → **UPDATE** branch: advance one animation tick (below), then loop.
- `iVar9==1` → **RENDER** branch: begin → draw gem/aura/sparkles → present.

The accumulator drains 16 ms per update; once `<0x11` left it flips to render.
Mirror with the port's frame_limiter (one update per presented frame), exactly as
the title drive does post-R3.

### UPDATE tick (the `iVar9==2` body, the load-bearing state)

Audio-guarded blocks (`DAT_008a93e4` ZDM music, `DAT_008a7608` SP mgr) are skipped
in the port (both null pre-audio).  The visual state, with **init values**:

| var      | init    | meaning |
|----------|---------|---------|
| `uVar16` | `0x1130`(4400) | **watchdog** — −1/tick; at 0 → finish (return `local_7c`=0). After exit begins, clamped to **200**. |
| `sVar4`  | `10`    | **start delay** — while >0, the tick only decrements it (gem hidden). |
| `local_90`| `0xed8`(3800) | sound-cue countdown (audio; fires `0x5bbdb0(0x1d,5)` at 0 — no-op pre-audio). |
| `uVar8`  | `0`     | **beat count** — +1 per consumed fresh key press; at **>2 (3rd beat)** → begin exit. |
| `bVar5`  | false   | **exit flag** — set on 3rd beat; switches gem to fade-out + clamps watchdog to 200. |
| `sVar6`  | `0`     | **gem fade phase**: 0 = fade-in (`local_bc` 0→600), 1 = hold (`local_9c` 0→0xc7f=3199), 2 = fade-out (`local_bc`−1, or −4 if `bVar5`). |
| `local_bc`| `0`    | gem **alpha level** (0→600); render maps `(bc*800/1000)*0x14/600` → ramp idx 0..0x14. |
| `local_9c`| `0`    | hold counter (phase-1 dwell). |
| `local_a0`| `-4000`| gem/aura **rise position** (fixed pt); `+= local_88/100` each tick. |
| `local_88`| `800`  | rise **velocity**; decays −10 once `local_a0>16000` (ease-out at the top). |
| `local_94`| `0`    | **gem frame** index; advances 0→0x22 then `%0x23` (35-frame loop) via `uVar19` 0..6 gate. |
| `uVar19` | `0`     | gem-frame sub-counter (0..6, then bump `local_94`). |
| `uVar17` | `0`     | **aura frame** (0/1 toggle) via `uVar7` 0..6 gate. |
| `uVar7`  | `0`     | aura sub-counter. |

**Beat input** (only while `!bVar5`): scan the input ring (`mgr+0x108`) newest-
first for the **first fresh press of any id** within 100 ms (= `input_*` ring
semantics, but matching any id, then consume + clear axis arrays + flush ring +
`uVar8++`).  At `uVar8>2` set `bVar5`, `sVar6=2` (start fade-out), clamp `uVar16`
to ≤200, and flip the 6 sparkle entries to their shrink state.

**Abort input** (checked first, every tick): `FUN_0043c110(now, 0x22)` =
`input_poll_consume(mgr, now, 0x22)` → if hit, `local_7c=6`, cleanup, **return 6**
(skip the game, back to title).

### The 6 sparkle entries (`local_46[]`)

6 entries × 6 ushorts each.  Clean field model (per entry):

| field   | init     | role |
|---------|----------|------|
| `state` | 0        | 0=wait, 1=grow, 2=descend, 3=shrink, 4=dead |
| `level` | 0        | brightness band 0..0x14 (grows in st1, shrinks in st3) |
| `sub`   | 0        | 0..10 inner sub-counter gating level changes |
| `y`(int)| **per-entry** | fixed-pt descent position; −0x10/tick in states 1..3 |

Per-entry **`y` start** (staggered so sparkles appear in sequence), overriding the
32000 default: `{0x2a8, 0x3d4, 0x62c, 0x758, 0x884, 0xa46}`.

State machine per tick:
- **0 (wait):** if `sub==0` → state 1; else `sub--`.  (init sub=0 so they start
  immediately; staggering is via the `y` start instead.)
- **1 (grow):** `sub` 0→10 then reset + `level` 0→0x14; at `level==0x14` → state 2.
- **2 (descend):** when `y < 0x2ee1`(12001) → state 3.
- **3 (shrink):** `sub` 0→10 then reset + `level` 0x14→0; at `level==0` → state 4.
- states 1..3 also do `y -= 0x10` each tick.

On the 3rd beat (`bVar5`): each entry's state is forced toward shrink
(`state = 4 - (state!=0)`), so live sparkles die out as the gem fades.

### RENDER tick (the `iVar9==1` body)

Only draws once `sVar4==0` (after the start delay) and `local_bc>0`:

1. **gem** — slot[3] frame `local_94`, at **x = gem.f_0c + 0xf8**, **y = gem.f_10
   + (local_a0/100 + 0x30)**; alpha-shade idx = clamp(`(local_bc*800/1000)*0x14/600`,
   0..0x14) into the `DAT_008a9308` ramp; ramp==0 → plain keyed fallback at
   (0xf8, y).
2. **aura** — slot[1] frame `uVar17`(0/1), at **x = aura.f_0c + 0xd0**, **y =
   aura.f_10 + local_a0/100**; drawn whenever `local_bc>0` (full add, sprite+0x28).
3. **sparkles** — slot[2], a **4-column × 6-row grid** (`uVar21` 0..23 = frame): for
   each of the 6 entries (`local_98`), 4 inner columns at **x = 0x10 + col·0x98**
   (16,168,320,472), **y = entry.y / 100**; alpha-shade idx from
   `(entry.level*0x14)/0x14` (= `entry.level`) into the same ramp; ramp==0 → keyed
   fallback.  Only drawn while the entry's `level != 0`.

(`gem.f_0c/f_10`, `aura.f_0c/f_10` are the decoded sprite's trim offsets — read off
the decoded cell, same `+0xc/+0x10` the title path uses.)

## Port plan (mirrors the title split)

1. **`src/prologue_stone.{c,h}`** (pure) — the state struct + `init` (the inline
   `0x56cd20:50-102` reset + sparkle `sub` stagger) + `update` (one UPDATE tick,
   returning a status:
   RUNNING / ABORT(6) / DONE(0)) + a render-descriptor builder (gem/aura/24
   sparkle draws: frame + x/y + ramp idx).  **Host-tested** for state progression
   and render geometry.  ← this checkpoint.
2. **`src/prologue_drive.{c,h}`** (+ win32) — the timing loop, real `zdd_alpha_blit`/
   keyed-blit draws, `input_poll_consume(0x22)`/`input_any_fresh_press` feed.
3. **`main.c`** — newgame START → `enter_prologue` (instead of
   `leave_newgame_to_title`); one step/frame; on DONE → (game proper deferred:
   re-display title for now, logged); on ABORT → title.
4. **Live verify** — drive retail to the stone intro (the committed trace +
   Start-Game confirm), capture goldens, `differ_px` the gem region.  (Modal-pump
   flip-freeze caveat from the picker may apply — assess when we get there.)

## Open / caveats

- **Audio entanglement.** The cutscene cues BGM/SFX (`DAT_008a93e4`, `DAT_008a7608`,
  `local_90`, `FUN_005bbdb0`); all null-guarded pre-audio, so the visuals run
  standalone.  Wire the cues when audio (milestone 3) lands.
- **Beats are input-driven** (3 presses → exit), with the `uVar16` watchdog as the
  no-input fallback (4400 ticks).  The new-game-flow doc's "(auto-advances)" was
  the watchdog and/or recorded presses; the port reproduces the logic faithfully
  and the drive feeds input from the trace.
- **Game proper (`0x59ec30`) + narration** remain out of scope (in-game).
