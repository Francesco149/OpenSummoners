---
name: story-day9e
start: story-d9d
steps:
  - in control in the Catacombs: world != 0 and control
  - left room 400160: room != 400160 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Chartreux Catacombs, walkthrough Day 9 steps 4.6–4.8 (tombstones → spike trap → keys, BOSS Master
Ghost, MoH 13, the guard → [Thieves' Key], save), from the end of run `story-d9d`** (its movie replayed: f282638 in
**400160**, control, Arche leading, sword sheathed; party 7). Party **Lv11**: **Arche** 225 HP (Org Slayer, Thick
Leather, Leather Cape, Kitty Ears), **Sana** 157/158 HP, 71/176 MP (Mahogany Staff, Leather Armor, Chest Sash, Green
Smock, Anti-Poison Ring; slot-2 heal: hold X ~90 f, ~100 HP for 4 MP), **Stella** 143/195 HP, 86/140 MP (Lady's
Dagger, Scale Armor, Woven Gloves, Salamander Scale; Phoenix slot 3; fire mode `UX` / `C*8 X`); 8379 CP; Better Herb
×8, Magic Candy ×4, Recall Lamp, Revival Drink, Venom-Away S ×3, Ghost Marshmallow, Memory Photo 6, Doll Hand; **no
Crypt Key**. Last save: White Rose Inn, slot 22, `a2a16253c9bb` (none since). Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d9d/report.md` and the walkthrough's **Day 9** step 4 ([RD story-d9c],
[RD story-d9d]) first.

**Tools:** `rd zones R` (doors + "zone hold"); object state `s32(s32(world+0x11e0+4*slot)+0xc)`; `rd map R --crop`;
**`rd ck R drop NAME` is fixed** (it now frees the savestate and keeps the session) — drop old checkpoints early. Prefer
fixed key counts over per-frame `{…}` inputs. Every `--score` search needs the current room in its `--avoid`; bound
`lx`. Never mash `ent` through Yes/No or save prompts; do NOT allow `f2` in searches. Leader swap `ent`: wait ~20 frames
between presses. Pushing: sword sheathed.

Next:
1. 🔑 4.6: 2 pushable tombstones (70800?) → right one far right, left one far left → secret door; ride the spike trap
   → [Crypt Key]; the right door needs another key → left door (fall into skeletons) → left pit hugging right →
   [Crypt Key], [Guardian Drop]; bottom [Power Drop]; left-area barrels [Good Child Drop] (optional loot only if cheap).
2. 🔑 4.7: back to the spike area, far right, unlock; right of the fancy doors = **fountain** (heal?). Doors → **BOSS:
   Master Ghost** → [Pocketwatch]. Full HP/MP first (Magic Candy ×4).
3. 🔑 4.8: stairs down: left [Crypt Key] + **MoH 13**; key right, stairs → the "moaning" guard → give [Pocketwatch] →
   [Thieves' Key] → **save** (`rd endsave`). Stop there (or at ~40k frames).
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP. Do not grind; do not read other runs' movies. **Run `rd
check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows. Stop
when the run is ~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you,
with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level +
equipment at the end, upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections,
tool gaps.
