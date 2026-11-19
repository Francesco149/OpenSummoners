# probe story-day8g

run `story-d8g`; accepted 2026-11-18 16:30

check PASS (tools/drive/tasks/story-day8g.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day8g/trace.jsonl (108586 frames)

## driver's report

# story-d8g: Korat Caves 370120 → 370110 → 370130 → 370140 → 370145 → 370150 → 370155 → 370156 → 370157 (stopped before the clover puzzle; boss NOT reached)

**Run:** `story-d8g`, created with `RD_HOST=bunker ./rd new story-d8g --from story-d8f`. It starts at f71259 in 370120 with control, a 3-member party (`party` = 7) and Arche leading. It ends at **f108586** in **370157** (Korat Caves, the moving-block / clover-lever room). The party has control, everyone is alive and no enemies are in the room. That is about 37.3k frames. Every rd command used `RD_HOST=bunker`. Nothing ran on the Windows box, and no DISK LOW was seen.

**Gates:**
- `rd check` **PASS**: step 1 at f71260, step 2 at f73830.
- `rd verify` **MATCH** at f108586 (rng 8b8c5d0f, semv 966e81393de26c8e, 0 differing dwords).

**End save:** none new. No save point was passed, so the last save is still **4ce2d13efdf6** (Korat inn, slot 21, before the caves). To keep this progress, continue with `rd new NEXT --from story-d8g`. Checkpoints are r157@108586, r156@107006, r155@104066, r150@96139, among others.

## Route (frames)
- **f71300–71640:** F1 → Items. **[Salamander Scale]** = accessory, "reduces fire damage taken". Equipped on **Stella** (her accessory slot was empty).
  - Orange Beret: Sana-only and cosmetic (no stats). Left off.
  - Spare Chest Sash on Stella: Defense −2. Not equipped.
- **f72158–72398:** Sana leading, Heal Water (`n2`, X×150) twice brought the party to full.
- **Cave graph,** read from the area table (`s32(world+0x103c)`, 336-byte records from +4, zones 20×12 B at +0x1c):
  - 370110: → 360140 / 370115 / 370120 / 370130
  - 370130: → 370110 / 370140 / 370150
  - 370140: → 370130 / 370145
  - 370145: → 370140 / 370150
  - 370150: → 370130 / 370145 / 370155 (z4) / 370160 (z3)
  - 370155 → 370156 → 370157 (dead end: the clover branch)
  - 370160 → 370170 → 370175 → 370180 → 370190 → 370200 → 370210 (the boss line, not visited)
- **370120 → out:** the bottom is cut off from the upper loop except by the shaft over the hot-water pit (x 83–90k).
  - Small jump-through ledges hang over the pit at y ≈ 157600, 144800, 132000 and so on.
  - From the edge at x 97360, a short hop lands on the lowest ledge. The search found `LC*4 .*4 LC*6 L*6 C*6`.
  - From there, full jumps go up ledge to ledge into the middle corridor (f73535). Then up the left side of the loop to the walk-in zone (0, 44800) → **370110 at f73829**.
- **370110:** down the east slope to zone 2 (150400, 102400; press ↑), clearing Red Slimes and Salamandras → **370130 at f75024**.
- **370130:**
  - Top-right lever 370130511 (code 70216, at 244800, 40000), **V at f77917**. It switched off both horizontal steam blowers (70810). The steam around the lever burns hard (Arche fell to 62).
  - Then went back west along the top, down the west side and east along the bottom corridor to zone 3 (252800, 153600).
  - The zone table says "walk in", but it only fired on **↑ while idle** → **370140 at f81798**.
- **370140:** the right-side chests (233600 and 241600, y 108800):
  - **f84398: [Lady's Dagger]**
  - **f84759: [Tough Drop]**
  - The walkthrough's 7.2 loot is in **370140**, not 370120.
  - Then down and west to zone 2 (0, 134400) → **370145 at f91671**.
  - It took a Stella Phoenix on a Dragon Pup and an Arche kill on a Salamandra. A party member near a foe holds the zone (see below).
- **370145 (f91711–92791):** with no enemy near:
  - Venom-Away S on Sana and on Arche. From the F1 menu it acts at once; no cast bar.
  - Equipped Lady's Dagger on Stella: Attack 44 → 60.
  - Stella's Phoenix killed the Dragon Pup.
  - West to zone 2 → **370150 at f96099**.
- **370150:** right entry → west along the middle → down to the bottom-left. Two Salamandras killed.
  - **f97595: chest 370150501 (16000, 185600) = [Anti-Poison Ring]**
  - **f97869: chest 370150502 (38400, 192000) = [Mystery Juice]**
  - Then:
    - **Arche reached Lv10.**
    - Ring equipped on **Sana** (empty slot, Defense 39 → 45).
    - **Tough Drop → Arche** ("Defense increased by 2"; the run's first use, at f85155, was lost to a checkpoint load).
    - **Mystery Juice → Stella** (Max MP +4 → 118).
  - The bottom chasm (x 147k–202k) was crossed by search. The block-over-vent step was not needed.
  - Zone 4 (220800, 185600; press ↑) → **370155 at f104026**. Stella was leading, because her AI could not cross the chasm and kept the zone held.
- **370155:** no enemies.
  - The last Magic Candy went on Sana; her AI then healed everyone to full.
  - East under the ceiling vents (70700 ×3) and over the pits → walk-in zone (201600, 19200) → **370156 at f106967**.
- **370156:** a Dragon Pup and a Salamandra were attacking Arche and Sana back at x 53–60k, holding the exit. Arche killed both (f108417) → walk-in zone (99200, 22400) → **370157 at f108546**.
- **Stopped at f108586** in 370157 (0, 23200). It matches walkthrough 7.3:
  - Start/stop lever 370157601 (code 70216) at (53200, 34800)
  - Moving stone block 370157602 (code 70210) at (62800, 41600)
  - Ceiling vents 70700 at x 65600, 77200 and 88000
  - **Clover lever 370157511** (code 70216) at (136000, 33600), behind a horizontal blower 70810 at (136000, 27200)

**Not done (blockers):**
- The clover puzzle, the lower path (Guardian Drop, 3000 CP, block stack, upper-left vent), **BOSS New Age Myconid**, MoH 11 and the hot spring.
- The reason was the ~40k-frame budget. Fights against flying Dragon Pups (51261) and Salamandras (51043) cost far more than expected: Arche's sword rarely reaches a flying pup, Stella's Phoenix does, and MP ran out.

## Party at the end (f108586)
- **Arche:** **Lv10**, 112/216 HP, 39/39 MP. Org Slayer, Thick Leather, Leather Cape, Kitty Ears. Defense +2 from the Tough Drop.
- **Sana:** **Lv9**, 74/145 HP, 47/150 MP. Mahogany Staff, Leather Armor, Chest Sash, Green Smock, **Anti-Poison Ring**.
- **Stella:** **Lv9**, 182/182 HP, 2/118 MP. **Lady's Dagger**, Woven Gloves, **Salamander Scale**. Still no body armour.
- **Gold:** 2044 CP.
- **Items:**
  - Used up: Magic Candy (0 left).
  - Left: Mind Chocolate ×1, Holy Water ×2, Hot Springs Bun ×3, Handmade Tart, Sweet Sticks ×5, Better Herb ×5, Venom-Away S (2+3+3 doses), Revival Drink, **Recall Lamp**.
  - Spare equipment: Rondel, Iron Staff, Chest Sash, Underwear, Orange Beret.
- `story` is still 1320.

**Upgrades skipped:**
- **Stella body armour:** none found or affordable.
- **Orange Beret:** no stats.
- **Spare Chest Sash on Stella:** lowers Defense.
- **The 370150 chest at (214400, 108800):** has no handle, so it may be the walkthrough's poison chest. Left closed.

## Game facts / walkthrough corrections
- **[Lady's Dagger] + [Tough Drop] are in 370140's right-hand pair of chests.** [Anti-Poison Ring] + [Mystery Juice] are in **370150's bottom-left** chests. Step 7.3's "bottom area" is 370150.
- **370120 exit from the bottom:** use the jump-through ledges above the hot-water pit (x 83–90k). A short hop from x ≈ 97.4k reaches the lowest one; full jumps go up the rest. Walking off the edge drops you into the pit (−10 HP, back to the ledge).
- **370130:**
  - The top-right lever (V) turns off both horizontal steam blowers.
  - Zone 3 (→ 370140) is listed as kind "walk in" but fired only on **↑ while idle** at (254000, 164000).
- **Zone hold applies to party members too:** for a not-done zone, every active party member within 48000 of a foe holds `zrest` at 30. This is in `zone_pass`, `src/gimmick.c`.
  - A stranded follower stops the exit. Seen twice: Stella left of the 370150 chasm, and Arche/Sana fighting in 370156.
  - Fix: switch leader to the stranded member (or kill the foe near them).
- **Salamandra in a "dormant" state:** `c+0x1d0` = 0 but HP > 0, and swords don't hit it. Seen at 370150 slot 8.
- **Steam blowers (70810 / 70700 horizontal) burn for 30–40 per tick.** Sana died standing in 370140's right-wall blower (251200, 97600). Don't park the party at x > 240k there.
- **Leader switch (`ent`) is often ignored:** right after a room entry, and while a cast or menu just closed. A state-gated tap (`{key('ent') if lid != X and frame % 20 < 2 else 0}`) works reliably.
- **Venom-Away S from F1 acts at once** (message box, no cast bar). Tough Drop / Mystery Juice / Magic Candy do have the ~250-frame cast bar.
- **The F1 menu opens on "Level-up Info"** first after a level-up.

## Named state (run vars)
- `p0 p1 p2 pmin`: party HP by party slot.
- `e0..e13`, `x0..x13`: char-slot HP (targetable hostile) and x.
- `ehp`: the sum of e0..e7.
- `cha chb` (370140 chests), `c501 c502` (370150 chests), `lev130` (370130 lever).
- `s8hp`: slot-8 HP, ignoring the targetable flag.
- `sx`: slot 16 x. Mis-named: slot 16 is Sana in 370150.

## Tool gaps
- No var for "which foe holds `zrest`". A `zholder` var (or `rd zones` printing the threat, including party-member checks) would have saved several searches.
- `rd zones` reports kind 1 "walk in" for zones that only fired on ↑ (370130 z3).
- Searches in fights run at 150–250 ticks/s, so 150–200 s fight searches often end with `--apply-best` progress only.
- A follower stranded across a gap is not visible in `rd status`. Party-member x/y in status would help.

## Repo edits
None. No commits.

