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

**TOOLING NEXT (the unblock):** emit OSR_STATE once per SIM-TICK, not per flip — hook the
easer `0x43d1d0` in the proxy (`engine_hooks.h`) and emit `rng` there (mirror the port's
`g_sim_tick_count` site).  Then every tick's `rng` is captured on both sides, lockstep
batching is invisible, and `rng_seq_diff.py` gives a clean first-divergence tick.  A
real-time `turbo=0 lockstep=0` capture (the sword2.osr mode) is the alternative but is slow
to reach the errands (the town intro plays in real time).

## The fix path (once the census is clean)

1. Confirm the RNG is aligned coming INTO the errands (or find + fix the first real
   divergence — a town/house per-tick consumer the port draws differently).
2. Model the errands room-load RNG burst — `0x431e30`'s per-case draws for each animated
   CHARACTER-band actor (the `0x426ec0` pair + any prefix), IN MAP ORDER, the way
   `actor_spawn_effect_from_map` already replays the town's `0x41f200` burst (quirk #86).
3. Model the family scene-script spawn RNG (`0x4dc510` case-7/8 → `0x41ec20` →
   `0x426ec0`), and any per-tick house/errands anim RNG retail draws.
4. Then Mom/Father's spawn phase DERIVES from the LCG; verify `differ_px==0` at the clamp
   (family render fr2/fr6 == retail), retiring `effect-anim-phase`.

This is a genuine multi-checkpoint Phase-2 arc (the errands analogue of the town quirk #86
work), blocked first on the per-sim-tick census tooling above.
