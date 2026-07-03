# The errands RNG census — diagnosing the family anim-phase residual (ckpt 191)

**Goal.** Un-MVP the errands FAMILY anim-phase (`effect-anim-phase`): retail seeds
Mom/Father's idle-clip START frame via `0x426ec0` (`frame = (rand·clip.frame_count)>>15`
+ a 2nd rand for the timer — 2 LCG draws/actor); the port renders them at a HARDCODED
`ERRANDS_CAST.clip_phase` (Mom fr0 vs retail fr2, Father fr5 vs fr6).  A faithful fix
must DERIVE the phase from the LCG, which requires the port's RNG stream to match retail
at the family-spawn tick.

**Method.** The OSR_STATE rng census: capture the port (`--osr-state`) + retail
(`OSS_OSR_STATE=1`) over the SAME nav (`nav-errands-spam` + `hold-right-clamp`),
seed-pinned `0x4f5347`, and compare the `rng` state word.  New tool
`tools/trace_studio2/rng_seq_diff.py` compares the ORDERED sequence of distinct per-tick
`rng` states (draws are input-driven, so the sequence is robust to the port/retail
dialogue-timing skew — a tick-for-tick join is not).  Captures:
`C:\oss-osr\port-rngcensus.osr` (`--osr-state`, 8500 frames) +
`C:\oss-osr\retail-rngcensus.osr` (`OSS_OSR_STATE=1`, turbo=0 lockstep=1, 150 s).

## Findings

1. **Aligned into gameplay — EXACT.** port==retail `rng` at every boot anchor:
   newgame `0x404a0a8f`, prologue `0x40d00581`, game_enter `0x40d00581` (both re-pin to
   `0x4f5347` at the first `0x41f200`, quirk #86).  The per-tick sequence matches EXACTLY
   through tick 40 (same index, same value).

2. **The port draws almost NO RNG after the town.**  Port `rngcalls` grows steadily
   through the town (tick 0→~1268, →14845: the game_enter 19-object spawn burst +237 at
   tick 1, then per-tick +6/+14 butterfly/fountain draws + a periodic +20 every 162t),
   then **FLATLINES** — the town→house→errands transition + the whole errands draw
   essentially zero (a lone +2 at tick ~1710).  Port distinct-rng states: **1271** (ticks
   0..1735).

3. **Retail draws RNG CONTINUOUSLY.**  Over the same window retail has **2602** distinct
   states through tick 3000 with **no >40-tick gap** — where the port has a 442-tick
   RNG-free house/transition (tick 1268→1710), retail keeps drawing.  And the port's
   town-end state `0x51c212a8` (tick 1268) does **not** appear in retail's stream at all.
   So retail consumes RNG in the house + transition + errands (per-tick anim/effect draws
   + the `0x431e30`→`0x426ec0` per-actor spawn burst) that the port's
   `actor_spawn_from_map` (errands CHARACTER band) + ERRANDS_CAST **skip entirely**
   (`actor_spawn_from_map` never calls `rng_rand`; the family use a hardcoded `clip_phase`).
   `0x431e30:745` confirms the errands char-band activator calls `0x426ec0(iVar2)` (the
   fire; clock/pot too) — the port draws none of it.

## The confound — the census must emit on the SIM-TICK axis, not the FLIP axis

The retail capture ran under **lockstep** (`turbo=0 lockstep=1`, `step=16ms`).  Lockstep
virtualises the clock per FLIP, and OSR_STATE emits per flip (`eh_flip_cb`), but a lockstep
flip can advance MORE than one 16 ms sim-tick — so retail's per-flip `rng` stream BATCHES /
shifts sim-ticks (the tick-41 wobble: retail[41]==port[42], a one-tick draw lead that the
subsequence match absorbs for 581 states then breaks at retail tick 584).  This makes the
fine-grained per-tick comparison unreliable — I cannot cleanly separate "a real town RNG
divergence" from "retail's flip-emission batched a tick".  This is exactly CLAUDE.md's
**sim-tick-axis** rule (ckpt 105): chase divergences on the easer/sim-tick count, never the
non-deterministic Flip index (quirk #75).

**TOOLING NEXT (the unblock) — LANDED ckpt 192.** OSR_STATE now emits once per SIM-TICK,
not per flip: the proxy moved the whole state block from `eh_flip_cb` to `eh_sim_tick_cb`
(the easer `0x43d1d0`, quirk #75) and each STATE carries its own `tick` (+`flip`) field; the
port added the matching `tick`/`flip` fields at its `drive_present` emit.  `rng_seq_diff.py`
`load_seq` keys each STATE by its own `tick` (falling back to the enclosing FRAMEBEG for
legacy per-flip captures — backward compatible), so under lockstep the K per-tick STATEs
under one flip are recovered instead of collapsing to the flip-time tick.  The easer fires
during the sim update — after FRAMEBEG, before the frame's blits — so `osr_scrub_frame_state`
(the osr_view engine-state panel) still resolves each frame's state unchanged.  Verified:
port+proxy build clean, 1104 host pass, and a synthetic lockstep stream (one FRAMEBEG over 3
easer ticks) round-trips through `load_seq` as 3 distinct ticks.  **NEXT: re-capture both
sides per-sim-tick** (retail `run_proxy.sh` with `OSS_OSR_STATE=1`, port `--osr-state`; same
`nav-errands-spam` + `hold-right-clamp`) and run `rng_seq_diff.py` for the clean
first-divergence tick.  A real-time `turbo=0 lockstep=0` capture (the sword2.osr mode) is the
alternative but is slow to reach the errands (the town intro plays in real time).

## Census RESULT (ckpt 192 — per-sim-tick, tick-for-tick)

Re-captured both sides per-sim-tick (port `port-rngcensus2.osr` 8498 frames; retail
`retail-rngcensus2.osr` turbo=0 lockstep=1, both `nav-errands-spam` + `hold-right-clamp`,
seed-pin 0x4f5347) and ran the upgraded `rng_seq_diff.py`.  With the lockstep confound gone
the two tick axes align **1:1 (offset 0)**, so the tool now does a TICK-FOR-TICK diff and the
picture is sharp — and it MOVES the first divergence:

- **Port == retail RNG tick-for-tick through the TOWN, ticks 1..973** (match 967/973 town
  ticks) — the town RNG model is essentially bit-exact.
- **Two self-healing bubbles precede the split:** ticks **584..588** (5t) and **972** (1t).
  Each DIVERGES then RE-CONVERGES to the same LCG state — the *same total draws redistributed
  across ticks* (a draw-TIMING wobble, not a count error).  rc across 584-588 = +58 total on
  both sides.  The old subsequence match BROKE at the first bubble (584) and mislabeled it a
  total divergence; the tick-for-tick diff shows it heals one tick later.
- **First PERMANENT divergence: tick 974** (last matching tick 973) — and the port is STILL
  DRAWING there (rc keeps climbing 11498→…), so it is a draw-COUNT / scene-timing split, NOT
  the port going static.  This is in the TOWN, ~300 ticks BEFORE the house/errands — so the
  ckpt-191 "the gap is the errands spawn burst" framing was imprecise (a subsequence-match
  artifact): the town RNG itself splits first and must align before the errands can.
- **Locus — the periodic +20 event, RE target pinned.**  The port's town `rngcalls` deltas
  show a periodic burst at **tick 163 + 162·k** (163, 325, 487, 649, 811, **973** — spacing
  EXACTLY 162, each **+20/+21** draws), after the tick-1 **+237** game_enter spawn burst
  (quirk #86).  All of k=0..5 (through tick 973) MATCH retail tick-for-tick; **the split opens
  at tick 974, the tick immediately after the k=5 event (973 = the last matching tick).**  Then
  the port fires an **ANOMALOUS +18 burst at tick 984** — 11 ticks after 973, OFF the 162 grid
  (the 7th on-grid event is at 973+162=1135, which is also present) — the prime suspect for the
  extra/misordered draw.  A post-974 re-alignment scan finds NO single offset recovering the
  match (975-1100 = 1/126; a partial 79/169 at offset −10 by 1100-1268) ⇒ a real COMPOUNDING
  divergence, not a redistributable tick-shift.  So the consumer to RE is the **162-tick periodic
  event + whatever fires the tick-984 burst** (the EFFECT-band event timer `0x467380` / the
  butterfly-slot band, `game_actor_update` in `main.c`), tracing why the port draws at 974/984
  what retail does not.

## Count-vs-timing RESOLVED — the omitted consumer is FUN_0043f880:415 (ckpt 193)

Landed the retail `rngcalls` counter: proxy `eh_rand_cb`, an E9-trampoline @ `0x5bf505`
(head `a1 94 4f 8a 00` = `mov eax,[8a4f94]`, a clean 5-byte PIC prologue) that counts
every rand + mirrors to an `rngcalls` STATE field.  Re-captured `retail-rngcensus3.osr`.
Two new count-readers:
- `tools/trace_studio2/lcg_walk.py` — derives retail's per-tick DRAW COUNT purely from the
  `rng` state seq (walk the LCG `s'=s*0x343fd+0x269ec3` between adjacent per-tick states),
  so it read the count off the OLD rngcensus2 (no counter needed).  Port walk == port rc
  EXACTLY ⇒ method validated.
- proxy `OSS_RAND_TRACE_LO/HI` (windowed `[esp]` ret_va log in `eh_rand_cb`) +
  `tools/trace_studio2/randtrace_attrib.py` — maps each rand consumer site → fn via
  `functions.csv`.  Plus a windowed `0x43f880` entry hook (`[mvtrace]`) to ID the actor.

**VERDICT: a COUNT split — retail draws +1 rand()/tick from census tick 972** (NOT 974 —
see below).  port 6/14-alternating (the every-other-tick gate phase) → retail 7/15.
`rng_seq_diff.py` now prints the verdict + dp/dr (draws/tick, port/retail); `dr` (the
rngcalls delta) == the LCG-walk == the ret_va count — triple-confirmed.

**The omitted consumer = `FUN_0043f880` line 415 (VA `0x440301`, +0xa81)** — the SOLE rand
in that fn (the actor MOVE-COMMAND builder): a probabilistic "push command 3" roll
`(rand*1000)>>15 < in_ECX[0x3212]` (a per-actor wander-freq, halved when HP<300‰), gated
`local_b8[6]!=0 && param_5==0`.  0.000 draws/tick before tick 972, exactly **1.000/tick**
after ⇒ the whole +1/tick.

**The actor = a grounded town NPC** (body `0xe8767d8` @ world 41600,45600).  The `[mvtrace]`
window shows 4 EVEN-tick bodies (the gated butterflies, st=3) + THIS one every tick from
971, with state `0→1` (idle→WALK) exactly at census 972, then walking right accelerating
(wx 41632→43024).  So a town pedestrian starts a walk at census 972 and its walk move-cmd
rolls line 415 per tick; the port's town RNG model (butterflies `0x47b990` + the `0x54f980`
ambient actor) has NO such walking NPC → omits the draw.

**Why census 972 not 974** (the ckpt-192 bubble_diff mislabeled 972 self-healing / 974
permanent): the draw STARTS at 972; the butterfly flit-pick at 973 (`butterfly.c:92
if r<wander_freq`) is a STATE-dependent conditional draw that draws 1 FEWER on retail's
+1-perturbed state, coincidentally re-converging at 973 (same 35-draw total over 972-973),
then diverging for good from 974.  ONE root cause (the missing `0x43f880` per-tick draw),
not two.  The ckpt-191/192 "periodic +20 / butterfly consumer at k=6" framing is SUPERSEDED
— the +20 tick is just the butterflies' periodic flit-pick (which the port already models);
the real gap is the unmodeled NPC.

**Secondary (defer):** the ±3 spikes (census 979/999) = `FUN_00489280+0x21/+0x77` @
~0.077/tick (≈1 per 13t) — a lower-freq consumer; chase after the primary NPC lands.

## The fix path (once the census is clean)

1. **IDENTIFIED (ckpt 193):** the first real divergence is TOWN census tick 972 = the port
   OMITTING `FUN_0043f880:415` (a walking town NPC's per-tick move-cmd "push command 3"
   roll; body `0xe8767d8` @ 41600,45600 goes idle→walk there).  FIX = model that NPC's walk
   AI in the port (the town analogue of `butterfly.c`/`ambient.c`): spawn/track it, run the
   idle→walk transition at census tick 972, and reproduce the line-415 roll (1 draw/tick
   while walking, `(rand*1000)>>15 < wander_freq`).  Open sub-steps: (a) identify the NPC's
   resource id + spawn provenance (draw stream / town scene script); (b) find what triggers
   the walk at tick 972; (c) port the `0x43f880` state-1 command-set incl. line 415.  The
   town LCG must reach the house/errands with retail's exact state or nothing downstream can
   align.  (Then chase the secondary `0x489280` ±3 spikes.)
2. Model the errands room-load RNG burst — `0x431e30`'s per-case draws for each animated
   CHARACTER-band actor (the `0x426ec0` pair + any prefix), IN MAP ORDER, the way
   `actor_spawn_effect_from_map` already replays the town's `0x41f200` burst (quirk #86).
3. Model the family scene-script spawn RNG (`0x4dc510` case-7/8 → `0x41ec20` →
   `0x426ec0`), and any per-tick house/errands anim RNG retail draws.
4. Then Mom/Father's spawn phase DERIVES from the LCG; verify `differ_px==0` at the clamp
   (family render fr2/fr6 == retail), retiring `effect-anim-phase`.

This is a genuine multi-checkpoint Phase-2 arc (the errands analogue of the town quirk #86
work), blocked first on the per-sim-tick census tooling above.
