---
name: story-day10b
start: story-d10a
steps:
  - in control in the tower: world != 0 and control
  - left the torch room: room != 410130 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: Weathervane Tower, walkthrough Day 10 steps 2.2 (rest) → 2.6 (star lever, fountain, MoH 14), from the
end of run `story-d10a`** (its movie replayed: f373565 in **410130**, the torch room, **all 8 torches out and the gate
70230 (x 131800) open**, **Sana leading** at lx ≈ 139776, control). Party **Lv12**: **Arche** 217/255 (Org Slayer,
**Scale Armor**, Leather Cape, Kitty Ears), **Sana** 164/173, ~173 MP (Mahogany Staff, **Scale Armor**, Chest Sash, Green
Smock, Anti-Poison Ring), **Stella** 130/218, 156 MP (Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale;
Phoenix slot 3; learned Inferno); 1349 CP; Magic Candy ×3, Better Herb ×7, Ghost Marshmallow ×3, Revival Drink,
Venom-Away S ×3, Recall Lamp, Doll Hand. Last save: White Rose Inn, slot 24, `eaee1a401f88`. Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d10a/report.md` and the walkthrough's **Day 10** ([RD story-d10a])
first. **Sana's water attack = forward + X together** (`LX*4` / `RX*4`; quirk #184) — a plain X is a staff swing.

**Tools:** `rd zones R --all` (doors, objects + states, "zone hold"); `rd map R --crop`; `rd ck R save/load/drop`.
Keep `{…}` inputs short. Searches: `-j 1` (a second bunker session runs far slower); give per-platform goals and bound
how far left / down a search may go. Every `--score` search needs the current room in its `--avoid`. Never `rd tap
ent` through dialogue that may end in a Yes/No or save prompt; do NOT allow `f2` in searches.

Next:
1. Switch the leader back to Arche (`ent`, check `lid`). 🔑 Through the gate → Boys; stairs up; ⚠ spike traps.
2. 💎 2.3: far right stairs down: moving blocks over spikes → [Rune Staff] (equip on Sana if better).
3. 🔑 2.4: ride the platform up, push the block → lever; far right [Revival Drink]; zigzag → [Water of Life].
4. 🔑 2.5: moving blocks → 2 blocks: push the right one to the floor left of the left ledge's edge; push the 2nd off onto
   it; push the stack to the lever. Switch; ⚠ mid-hall spike trap; 2nd switch; right, stairs; drop off the right edge →
   hidden stairs down.
5. 🔑 2.6: left: Arche pushes the platform right; Sana / Stella stays on it ('Stay There!' per member); push back → they
   reach the ledge → **star lever**. Down, stairs up: [Stronggirl Juice]; drop → **fountain** (save: `rd endsave`); right
   → **MoH 14**. Stop there (or at ~40k frames).
Save at every save point and `rd endsave <run>` after each; note each digest.

Fights: harpies grab and drop you a floor; harpy song confuses. `--hold` searches; keep an avoid on each member's HP.
Do not grind; do not read other runs' movies. **Run `rd check` and `rd verify` when you reach ~$6 spent (mandatory)**;
verify again before `finish()` if the budget allows. Stop when the run is ~40k frames past its start, or at a point you
cannot get past after real attempts (say what blocks you, with an `rd map` PNG path). Report: run, frames, route steps
with frames, end-save digests, each character's level + equipment at the end, upgrades taken / skipped (why), shop
lists + prices seen, named state, walkthrough corrections, tool gaps.
