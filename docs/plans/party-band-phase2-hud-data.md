# Plan — Party band Phase 2, slice A: the HUD leader-panel DATA (un-MVP `hud-party-context`)

**Started 2026-07-03 (post-ckpt-188b). USER-picked arc** (party band Phase 2) over the RNG
census + errands questline. Continues `docs/plans/party-character-system.md` (Phase 1 = the
dramatist table, DONE in `party.c`). Scope of THIS slice: make the freeroam HUD leader panel's
**HP / MP / level / element-star-count / EXP** derive from a real party-member stat record
(Arche lvl 1) instead of the hardcoded `hud-party-context` constants. The item-bar room fields,
the portrait (leader-match `+0x1b4`, replay-blocked), and the door subsystem stay debted.

## The RE'd data contract (retail, off the decompile — GROUND TRUTH)

**Hierarchy** (all confirmed): map obj `0x4120` → 8 slot ptrs `+0x4030..+0x404c`, leader ptr
`+0x200c`. slot (`0xeec`): active `+0x9c4`==1, handle `+0x9f0` (!= sentinel `0x5f5e168`), member
ptr `+0x9f4`, slot idx `+0xa0c`. member entity (`0xc788`+): **stats block at `+0x750`**.

**Stats block (`member+0x750`) — the fields the HUD reads:**
| field | off (rel +0x750) | meaning |
|---|---|---|
| hp_cur | +0x54 | current HP |
| hp_base/equip/buff | +0x58 / +0x84 / +0x9c | **max HP = sum of the 3** (`0x497b40`, `494e60:109`) |
| mp_cur | +0x5c | current MP |
| mp_base/equip/buff | +0x60 / +0x88 / +0xa0 | **max MP = sum** (`0x497bb0`/`0x496970`) |
| star_count | +0xdc | element-star COUNT (loop bound, `494e60:100`) — Arche=2 |
| level_bonus | +0xd8 | added to level; the star renderer's 5th arg but **IGNORED** by `0x498620` |
| combat_level_max | +0xe0 | **level = +0xe0 + +0xd8** (`494e60:123`) — Arche=1 |
| exp_cur | +0xe8 | `494e60:96` EXP gauge cur — Arche=0 |
| exp_max | +0xec | EXP gauge max (next-level threshold) |
| item_mode | +0x140 | item-bar slot4 (gated) + animator `+0x140` change-detect |
| status[18] | +0x448 (u16×18) | per-status timers (animator low-HP/status → head-state) |
| status_word | +0x44a | u16 status (item-bar / door gates) |

**Number formulas** (so the displayed text derives, not hardcoded):
- `hp_max = base+equip+buff` (clamp ≥1). `hp_ratio = cur*1000/hp_max`. Displayed cur HP
  (`0x497b40(stats, ratio)`): `ratio == cur*1000/hp_max ? cur : hp_max*ratio/1000`. Errands is
  steady (cur==max, ratio settles 1000) ⇒ shows cur==max. Text "%4d / %d" = "100 / 100".
- MP identical over the +0x60/+0x88/+0xa0 / +0x5c fields → "20 / 20".
- level text = `combat_level_max + level_bonus`. element = draw the fixed star cel `star_count`×.
  EXP gauge: filled span 0-width when cur==0 (omitted, only depleted draws) — already ported.

**The animator `0x49af40`** lerps HP/MP ratios into the HUD ctx (`+0x1b8`/`+0x1bc` for the
leader panel; per-slot pairs at `+0x344+2i`) at ±5..±(diff/10)/tick toward `cur*1000/max`. At
settled full HP the ratio == 1000 (no transient). So the port can compute ratio directly from
stats at steady state (the errands never damages Arche) — the lerp itself lands with combat.

**The leader-match gate (`494e60:74`, `+0x1b4`!=0)** wraps the ENTIRE per-member panel in
retail; the port draws it unconditionally with stand-ins. `+0x1b4` (leader_uid) reads 0 in
scripted replay (replay-fidelity gap, `freeroam-hud.md §7`) → the PORTRAIT stays blocked (needs
a live play or the `+0x1b4` setter). This slice does NOT touch the portrait.

## Port design

- Extend `src/party.{c,h}`: a `party_stats` struct mirroring the `+0x750` read-fields; a
  `party_member` {handle, active, stats}; a `party` {member slots[8], int leader}. Pure C,
  host-tested. Accessors: `party_stat_hp_max/mp_max`, `party_stat_hp_cur` (the `0x497b40`
  formula), `party_stat_level`, `party_stat_star_count`, `party_stat_exp_cur/max`.
- `party_init_errands(party*)`: put Arche's lvl-1 member in slot 0 + leader=0, stats from her
  REAL level-1 def (SOURCE = pending the stat-source RE — a base-stat table extracted like the
  dramatist table, or the computed init; NEVER a relocated 100/20/1 constant).
- `main.c`: a global `g_party`; init at `freeroam_begin`; `game_render_hud` reads
  `party_leader(&g_party)` for HP/MP/level/star-count/EXP, dropping the `HUD_*_VALUE` /
  `hp_cur=100` constants. Retire those specific `hud-party-context` stand-ins; the item-bar
  icons + portrait + door stay in the debt row (narrow it).

## Verify
- Host tests for the stat formulas (max=sum, the `0x497b40` cur-select, level=base+bonus).
- `--osr-emit` a freeroam clamp capture (the Z-spam recipe) → the HUD panel still renders
  `differ_px==0` vs `sword2.osr` tick 2200 (same geometry, now real-sourced values).
- `make -C tests run` green; both exes build; regen ledger if any `FUN_` ported.
