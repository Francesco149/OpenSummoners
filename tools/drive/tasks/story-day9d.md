---
name: story-day9d
start: story-d9c
steps:
  - in control in the Catacombs: world != 0 and control
  - left the MoH 12 room: room != 400146 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Chartreux Catacombs, walkthrough Day 9 steps 4.4–4.8 (water area, platforms + high lever,
tombstones, BOSS Master Ghost, MoH 13, the guard → [Thieves' Key], save), from the end of run `story-d9c`** (its movie
replayed: f264278 in **400146**, the MoH 12 room, control, Arche leading, sword sheathed; MoH 12 taken). Party **Lv11**:
**Arche** 225 HP (Org Slayer, Thick Leather, Leather Cape, Kitty Ears), **Sana** 158 HP / 176 MP (Mahogany Staff,
Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring), **Stella** 195 HP / 140 MP (Lady's Dagger, **Scale Armor**,
Woven Gloves, Salamander Scale; Phoenix = spell slot 3; learned Blade); 7793 CP; Better Herb ×8, Magic Candy ×4, Recall
Lamp, Revival Drink, Venom-Away S ×3, Ghost Marshmallow, **Crypt Key ×1**. Last save: White Rose Inn, slot 22,
`a2a16253c9bb` (none since). Read `/mnt/ephemeral/opensummoners/drive/agents/story-d9c/report.md` and the walkthrough's
**Day 9** step 4 ([RD story-d9c]: object codes, Follow Mode per member) first.

**Tools:** `rd zones R` (doors + "zone hold"); object state `s32(s32(world+0x11e0+4*slot)+0xc)`; `rd map R --crop`
(world units); `rd ck R drop NAME` early (a lost savestate pool = a ~9-minute replay). Prefer fixed key counts over
per-frame `{…}` inputs. Every `--score` search needs the current room in its `--avoid`; bound `lx`. Never mash `ent`
through Yes/No or save prompts; do NOT allow `f2` in searches. Ghosts often ignore the sword: Stella's Phoenix.

Next:
1. 🔑 4.4: back to the pot area (400140), right, use the key, tunnel → water area: [Doll Hand]; Sana into the right
   water crevice → [Memory Photo 6]; skip the next pit, take the one after → [Crypt Key]; left door.
2. 🔑 4.5: 2 platforms + a high lever: one member on the right platform, two on the left → the right one reaches the
   lever (Follow Mode 'Stay There!' per member).
3. 🔑 4.6: drop, right, up: 2 pushable tombstones → right one far right, left one far left → secret door; ride the
   spike trap → [Crypt Key]; left door → left pit hugging right → [Crypt Key], [Guardian Drop]; [Power Drop]; barrels
   [Good Child Drop] (optional loot only if cheap).
4. 🔑 4.7: the fountain → fancy doors → **BOSS: Master Ghost** → [Pocketwatch]. Full HP/MP first.
5. 🔑 4.8: stairs down: [Crypt Key] + **MoH 13**; key right → the "moaning" guard → give [Pocketwatch] → [Thieves'
   Key] → **save** (`rd endsave`). Stop there (or at ~40k frames).
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: `--hold` searches; keep an avoid on each member's HP. Do not grind; do not read other runs' movies. **Run `rd
check` and `rd verify` when you reach ~$6 spent (mandatory)**; verify again before `finish()` if the budget allows. Stop
when the run is ~40k frames past its start, or at a point you cannot get past after real attempts (say what blocks you,
with an `rd map` PNG path). Report: run, frames, route steps with frames, end-save digests, each character's level +
equipment at the end, upgrades taken / skipped (why), shop lists + prices seen, named state, walkthrough corrections,
tool gaps.
