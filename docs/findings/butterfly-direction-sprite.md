# Butterfly directional sprite — the 8-way flight render (THEME 2, notes #0+#2)

> **Status: RESOLVED + PORTED + drawcall-verified (ckpt 139).** The frame model is
> PROVEN off `retail.osr`, the carrier was pinned by a live Frida read (2452 render
> calls, 0 deviations), and the port now renders the butterfly sprite **bit-exact to
> retail** (draw_probe: 273/280 settled-town ticks frame-identical; the 7 misses are
> a pre-existing horizontal-position lag of the entering butterfly, not a frame bug).
> The fix: `src/actor_spawn.c` reads the per-instance `frame_base` from the map
> variant + corrects the bank flip 4→16. The render half of `butterfly-wander`
> (the "white variant + directions") is retired; the vertical flutter (`butterfly-flutter`)
> + the entering-butterfly position remain the motion debts.

## The gap (USER studio notes #0 + #2)
The town's 4 butterflies (EFFECT code **0xe29a**, bank 0x146, res 0x3fa/1018,
32×32, clip `0x65ddf0`) render with the wrong sprite in the port. Note #0
("butterflies color gap") and note #2 ("butterfly movement different") are ONE
underlying debt: the port reduced the butterfly to a 1-D horizontal patrol with
two facings; retail flies it in **8 directions**.

## The frame model — PROVEN off retail.osr (ground truth)
`tools/trace_studio2/draw_probe.py --res 0x3fa` over the settled town shows every
butterfly cel is `frame = d*4 + flap`:
- **flap ∈ {0,1,2}** — the wing-flap clip `0x65ddf0` (base 0, frame_count 3,
  dur 4, delta {0,1,2}; verified by reading the exe `.rdata` at VA 0x65ddf0).
- **d ∈ 0..7** — an 8-way DIRECTION; the cel base steps by 4 per direction.
  The frame histogram over ticks 80–360 is exactly the 8 groups
  `{0,1,2} {4,5,6} {8,9,10} {12,13,14} {16,17,18} {20,21,22} {24,25,26} {28,29,30}`
  (the 4th slot of each group — 3,7,11,… — is unused).

### d is the WANDER HEADING, not the instantaneous velocity
Tracking individual butterflies across ticks (screen `(x,y)` + frame):
- `d` is **stable for ~50–80 ticks then jumps** (e.g. track-1 d=2 t80–107 →
  d=6 t108–161 → d=2 t162+). That cadence ≈ the flit timer reload (`+0x14236`
  = 0x50 = 80 ticks).
- The actual screen drift is the SAME (rightward ~1–4 px/tick) across butterflies
  carrying **different** `d` (0,2,3,6,7) — so `d` is NOT `atan2(vy,vx)` of the
  realized motion. It is the direction the wander FSM is *aiming* (toward the
  current 2-D wander target), re-picked via RNG each flit.

So porting only the heading FSM's left/right flip (the current
`butterfly_step`, `+0x14244 ∈ {1,3}`) can only ever render d∈{0,1}. The full fix
needs the 8-way wander heading + however it reaches the cel.

## The render path (FUN_0044d160, ported as `actor_render_describe`)
`cel = row.frame_base(+0x4a) + frame_off + sprite_delta`, where
- `sprite_delta` = clip flap {0,1,2},
- `frame_off` = the angle path result (`(angle+360000)/(360000/div) % div`, +flip
  if facing==3) when `angle_anim`(+0xec)≠0, **else** `flip`(=DAT_008a8440[bank]=4)
  if facing==3 **else** 0.

The static reads so far:
- `0x426620` (the EFFECT install primitive the 0xe29a case calls) sets
  `actor+0xec = 0` (angle_anim OFF) — so on the static read the angle path is NOT
  taken, leaving `frame_off ∈ {0,4}` (facing only). That cannot produce d∈2..7.
- The butterfly install is `0x41f200` case 0xe29a (line ~2169):
  `FUN_00426d70(0, 0x146, param_7)` installs ONE sprite row (dir 0); `param_7` =
  the map record's variant field `*(u16)(record+0x18)` (dispatcher `0x58d460:151`).
  `0x427580(&clip,&clip,&clip,0,0)` stores a 3-entry clip table at +0x15914
  (all the same flap clip), NOT 8 rows.
- The 0xe29a per-tick FSM `0x47b990` (lines 769–800) is purely 1-D: heading
  `+0x14244 ∈ {1,3}`, moves toward an X bound via `FUN_0043f880(target_x,…)`.
- The apply/integrate `0x442a70` only TOGGLES facing `+0x2c` between 1/3 and
  advances the clip frame `+0x72` (wrap at clip+0x42=3). Neither it, `0x485fc0`,
  `0x43f880`, nor `0x47b990` writes the sprite-row `+0x4a` or `dir +0xe8`.

## RESOLVED — the live read (ckpt 139, 2452 render calls, 0 deviations)
A live Frida read at FUN_0044d160 entry (ECX=actor, filter code==0xe29a) settled it.
`d` is NOT one field — it splits across the render's existing `cel = sVar3 + sVar6 + sVar5`:
- **`angle_anim`(+0xec)=0 and `angle`(+0x34)=0 ALWAYS** — the angle branch is DEAD for
  butterflies (confirming `0x426620` line 85). `d` is NOT angle-derived.
- **`d` is actually `(d&3)` + `4·(facing==3)`, i.e. a per-instance BASE DIRECTION
  (0..3) + the facing-mirror bit.** Each butterfly oscillates between a heading and
  its mirror only (e.g. bf#0 ∈ {0,4}, bf#1 ∈ {1,5}, …) — the base is constant per
  instance. So my earlier "8-way wander heading that re-picks" reading was the
  base(const) + facing(toggle) decomposed.
- **Low part: `sVar3` = `row[dir].frame_base` (+0x4a)** = the base direction `(d&3)*4`
  ∈ {0,4,8,12}, set ONCE at spawn from the map variant (NOT rewritten per-tick).
- **Mirror: `sVar6`** = `DAT_008a8440[0x146]` = **16** when render-state `+0x2c==3`
  (facing) — frames 16-31 are the left-facing cels; it also reflects off_x (line 57).
  (The port had this flip as 4 — WRONG; corrected to 16.)
- **flap: `sVar5`** = the clip `0x65ddf0` delta {0,1,2}, indexed by render-state +0x72.
- So **`out_frame = frame_base + 16·(facing==3) + flap`**, frame_base = the per-instance
  map variant, facing toggled 1/3 by the (already-ported) heading FSM.

The `+0x2c` that gates the mirror is the **render-state** facing (1/3), not the actor's
+0x2c (which reads 3200, unrelated). The butterfly's frame_base is set at spawn by
`0x426d70(0,0x146,param_7)`, param_7 = `*(u16)(record+0x18)` (the map variant).

## The fix (ported ckpt 139) + verification
1. `src/actor_spawn.c actor_spawn_effect_from_map`: for 0xe29a, `frame_base =
   hdr_u16(h, HDR_OFF_VARIANT)` (was hardcoded 0); standing townsfolk keep 0.
2. `TOWN_EFFECT_DEFS`: butterfly `flip` 4 → 16 (the mirror-cel offset).
3. The facing toggle (1/3) was already ported (`butterfly.c` heading FSM → render-state
   facing), so it supplies the +16 mirror at the right ticks for free.
- **Verified** vs `retail.osr` (`draw_probe.py --res 0x3fa`): butterfly frames now
  bit-exact (e.g. ticks 272-278 upper butterfly 21,21,5,6,6,6,6 == retail; was
  5,5,1,2,2). 273/280 settled-town ticks frame-identical; the 7 misses are a
  pre-existing horizontal-position lag of the entering-from-left butterfly (OLD-port
  has identical positions — my change touched only frames). +host test
  (`test_actor_spawn_effect`: butterfly frame_base=variant, townsfolk ignore it).
- **Residual (separate debts):** the per-tick dst-Y bob (port flat vs retail ±2-5px) is
  `butterfly-flutter` (the vertical sawtooth integrator); the entering-butterfly
  horizontal position is `butterfly-wander`/`butterfly-bounds-writer`. Neither is a
  frame/direction bug.

Cross-refs: `docs/port-debt.md` (`butterfly-wander`, `butterfly-flutter`),
`docs/findings/engine-quirks.md` (#93 the butterfly ID), `src/butterfly.{c,h}`,
`src/actor_render.c` (`actor_render_describe` — the angle path is already ported).
