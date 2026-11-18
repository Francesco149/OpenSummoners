# probe story-day7f

run `story-d7f`; accepted 2026-11-18 00:41

check PASS (tools/drive/tasks/story-day7f.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day7f/trace.jsonl (76416 frames)

## driver's report

# story-d7f: Carudale Tunnels → Tonkiness home save (Day 7 end) → Day 8 → Sana kidnapped → Org side street

**Run:** `story-d7f`, from `story-d7e@38431` (330110). Ends **f76416** (~38k frames after start; budget stop). All RD_HOST=bunker; nothing touched Windows.

**Gates:** `rd check story-d7f tools/drive/tasks/story-day7f.md` **PASS** (step 1 f38432, step 2 f40751). `RD_HOST=bunker rd verify story-d7f` **MATCH** at f76416 (rng 2d50c6e4, semv cfa86812df580fd7, fb 7ca109c15286f397). An earlier verify at f55754 also MATCHed.

**End saves:**
- **f2e506cb158b** (f55754): home save, slot 14, "Minasa-Ratis Magic School", start of Day 8. `rd new NEXT --from save:f2e506cb158b`
- **7722d878b57d** (f66114): wandering healer save, slot 15, Korat road 340120, Day 8 before the kidnapping (Arche + Sana, full HP/MP). `rd new NEXT --from save:7722d878b57d`

**End state:** f76416, room **340150** (the Org side street), Arche alone (Sana kidnapped, `party` 1). Arche 124/197 is stuck in a low alcove at (258256, 41140) under the right-hand cliff. 5 hostiles are up: Kobolds 51030, 51024, and 51071 on the top ledge. Gold 1234. `story` 1310. The exit to 340160 is a walk-in at (278400, 0), top right. It is reached along the upper ledge band (rows 5–6), not from the low alcove. Map: `/mnt/ephemeral/opensummoners/drive/runs/story-d7f/view/map_340150_f76416.png`. For the next leg I'd resume from `story-d7f:r340150` (f72375, room entry), or `story-d7f@75209` (healed, 196 HP, in the alcove).

## Route (frames)
- f38431 330110: Sana's Heal Water healed Sana (86→104). With Arche leading, a search went up the zig-zag. The Merkids did not have to die. f40750 → **260230** (Loch Carudale, cave ledge).
- 260230: **Old Man** (50180) by the cave: "use m'boat… it moves in the direction you're facing". The boat is object 70600 at (136000, 32015), right of the cliff.
  - ⚠ Arche leading, Arche boarded: Sana stayed behind, fell in the lake and died to the Merkids by the time Arche reached the exit. This was rolled back.
  - Working recipe: **switch to Sana** (`ent`, 30 f after the dialogue closes), Heal Water, walk right, run-jump onto the boat facing right. The boat runs at 200 lx/f to the pier stop (bx 246399, ~600 f). Then Sana walks to the walk-in exit (348800, 9600). Arche stayed on the cliff, but the exit pulls her along. f42540 → 260220.
- 260220 (Mothbees, Merkid Mage, Merkids): f43818 → 260215 → f44515 260210 (avoid its ↑ zone 3 = Weathervane Tower) → f44895 260200 (downhill; the crate did not matter) → f45439 260190 → f45672 260180 → f46172 260150 → f46610 260140 → f47667 260130 (Heal Water on Sana) → f48738 260120 → f49156 260110 → ↑ (153600, 32000) → **f50055 Tonkiness 210110**.
- 210110: scene at the gate (Sana goes home; `story` 1210→1220 around here). Walked to ↑ 150400 → 210120 → ↑ 118400 → 210140.
- 210140: talking to Dad (ground-floor counter, X sheathed) gave a scene about the sweet sticks. Mom (middle floor, cooking) does not respond to X. The **bedroom trigger fires on walking right along the top floor** (~lx 27888). Prompt "Should I change into my pajamas?" (Yes) → Chiffon talk → night. HP/MP refilled. "Would you like to save?" Yes → slot 14 (cursor starts on 13, one D) → f55754 end-save.
- **Day 8:** class scene 230140, then Sana talk (Stella left) → Sana joins. Route: 230140 ↑ 92800 → 230130 ↑ 83200 → 230110 (schoolyard) ↑ 240000 → **235110** (Korat road) → 235120.
  - ⚠ In 235120 the **↑ at (249600, 28800) = Mountain Cave 240110 (wrong)**. The Korat road is **↑ (150400, 38400) → 340110**.
- 340110 → 340120: walking into the **wandering healer** (Female Mage, 340120 x≈155000) with the sword drawn slashes her bag ("Don't break those…"). Sheathed + V talks to her. Heal 30 CP (no Yes/No: confirm on the status panel), then "save as well?" Yes → slot 15 → f66114 end-save.
- 340120 → 340125 → **340130**: kidnapping cutscene (Boss Org + Org Lackey, "Bosslady Selene"), f67918–70008. `story` → **1310**.
- Back in 340125: classmates (Class Prez, Colm, Leon) go to tell Sophia. 340125 → 340120 → **↑ (131200, 28800) → 340150 side street** f72335. Two fight searches cost HP; Quick-heal F2 ×2.

## Party at end
- **Arche** Lv7, 124/197 HP, 31 MP: Iron Sword, Thick Leather, Woven Gloves, Accessory empty (unchanged).
- **Sana** (kidnapped): Iron Staff, Leather Armor, Chest Sash, Green Smock (unchanged). Her max HP reads 112 and MP 117 after Day 8 began. A "Level UP!" HUD icon showed in 340110, so she is probably Lv6 (not checked in the menu).
- Items: 2 Quick-heals were used (F2: Better Herb or Herb, not checked). Revival Drink, Magic Candy ×2, Mentama Drop, Merrin Card, Kitty Ears, Underwear and Taiyaki Sweet Sticks were not used.
- Gold 1172 → 1234: +62 from drops, −30 for the healer.
- **Upgrades:** no shop was passed with money for one. I did not visit the Tonkiness weapon shop (Iron Sword is already the best there). Skipped the side street's top-left [Herb] chest (70220): too little value for the frame budget.

## Game facts / walkthrough corrections
- **Loch Carudale return (260230 → 260220):** the old man's boat sits at the cave-side cliff. Only the leader rides, so the follower is left behind. Have Sana lead (she survives the lake and the exit pulls Arche along); Arche leading strands Sana in the lake.
- **Day 7 end:** Mom is not interactive. The pajamas/save scene fires walking along the bedroom (top) floor of 210140. To reach that floor, take the middle floor's left stairs to the top, then run-jump right (walking right follows the stairs back down). The home save happens after the night scene; HP/MP are refilled before it.
- **Day 8 route:** school 230140 → 230130 → 230110 → 235110 → 235120 → **↑ x 150400 → 340110** → 340120 (healer, 30 CP heal + save) → 340125 → 340130 (kidnapping, `story` 1310). Side street = 340120 ↑ (131200, 28800) → 340150. Its exit (278400, 0) → 340160, presumably the cave with bones / Org Lair.
- 340150 has Kobolds (51030), 51024 and 51071 on the ledges. They hit hard (Arche lost ~90–120 HP over the searches).

## Named state (run vars)
`ahp shp smp` (party slot 0/1 HP, slot-1 MP), `ax ay sx sy` (party bodies), `nfoe` (targetable faction-2 in char slots 0–9, sum of 10 explicit terms; `sum(... for ...)` comprehensions are rejected), `bx by` (zone/object actor slot 6 = the 260230 boat: `s32(s32(s32(world+0x11e0+24)+0x40)+4/+8)`).

## Tool gaps
- The savestate pool ran out after ~21 checkpoints. Dropping them caused a 164 s session rebuild; `rd rewind` also replays (>120 s).
- `rd search` `--goal ... or script == 1` fires on 1-frame transient scripts at room entry. `dlg != 0` worked better as a "something happened" goal.
- After a cutscene that leaves `script` 0, the first `ent` can be eaten (no leader swap). Wait ~30 f.
- Search actions with `U` entered doors in the house (210140 → 210141). Leave U out of non-door searches.

## Repo edits
None. No commits. Helper scripts are in the session scratchpad (arrive.sh, go.sh).

