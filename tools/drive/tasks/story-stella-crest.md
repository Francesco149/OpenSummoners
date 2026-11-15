---
name: story-stella-crest
start: story-stella-barness
steps:
  - loaded into a room with control: world != 0 and control
  - left the start room: room != 0 and frame > 3000
forbid:
  - back at the title after loading: world == 0 and tcount == 5 and frame > 6000
---
**Frontier probe: finish the Stella interlude from the end of `story-stella-barness`** (start: that scenario's movie
replayed — Day 6, **Stella solo** (Lv3, HP ~30/107, MP ~29/53, 1 Herb + 1 Better Herb, 973 CP) in **290230** of the
Barness Hills Dungeon (Stella route), beside 2 chests (lx ≈ 175000–180000) with 2 Cocorats + a Killer Bat near; last
save = the Weathervane Inn, slot 9, digest `0ef7e721d142`). Read `docs/findings/probes/story-stella-barness.md` and the
walkthrough's [RD story-stella-barness] note first (Stella's two attack modes on Z; ↑+X burns vines; ↓+X lights
braziers; Fire Wall = hold X). HP is low: heal first (F1 → Items, away from enemies = instant) and fight with care
(`--hold` searches; add `room == 290230` to enemy-HP goals — the slots reset on a room change). Next, per the
walkthrough: the chests, stairs down → right: [Moldy Bread]; left: **clover lever**; top-left → the talking doors → Stella
gets the **Wind Crest** → control returns to **Arche** → enter the doors → **BOSS: Stella** (`docs/game/bosses.md`) →
[Wind Crest 2] → out with Stella (her shortcut) → heading home: the boys, the doctor (medicine for Sana) → the Barness
herbalist → Herb Farm Cave (Arche + Stella).

**Upgrades (DRIVER.md "keep the party strong"):** equip anything better from the chests (Defense Ring etc.); use stat items
per the guide (Good Child Drop → Stella; Power Drop → Arche); when Arche returns, check his gear / level vs the
walkthrough before the Stella boss (Downstab helps there) and at the Barness general store (270190: Shortsword 2800, Iron
Sword 780, Leather Armor 560) on the way home if the money allows.

1. `rd new <run> --from story-stella-barness` replays that movie into a live session. Confirm where you are: `rd look`,
   `rd text`, `rd status`.
2. Follow the walkthrough's 🔑 steps from there, as far as you can. Save at every goddess statue / inn save you pass and
   run `rd endsave <run>` after each save, so later probes can start there; note each digest.
3. Fights: read `docs/game/bosses.md` before a boss; use `--hold` searches (DRIVER.md); keep HP up. A game over is fine
   to see once (coverage) — then `rd undo` / `ck load`.
4. Run `rd check` and `rd verify` before your budget runs low (the last driver ran out before verifying).
5. Name the state you need as you go (`rd ram R mark` → act → `rd ram R diff` → `rd ram R define`): anything a
   predicate needs (torch / vine object flags, the boss's HP).

Do not grind; do not read other runs' movies. Stop when the run is ~40k frames past its start (the start is at ~154k), or at a point you cannot get past
after real attempts (say what blocks you). Report: run, frames, the route steps reached with their frames, the
end-save digests, named state, walkthrough corrections (what the guide got wrong for EN-old), tool gaps.
