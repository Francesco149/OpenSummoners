# probe story-day9b

run `story-d9b`; accepted 2026-11-19 03:24

check PASS (tools/drive/tasks/story-day9b.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day9b/trace.jsonl (234522 frames)

## driver's report

# story-d9b: Mayberk Manor (dinner → night moans) → kitchen back door → Chartreux Graveyard → gravestone → Catacombs 1st room

**Run:** `story-d9b` (resumed after the earlier driver's OOM kill; no `rd new`). Start f209786 (310130). **End f234522** in **400110 = Chartreux Catacombs first room**, lx 30400 ly 29600, `control` 1, sword drawn, party 7 (Arche leads), 2 Ghosts (51081) ahead. 24.7k frames used of the 40k budget; stopped where the task says.
All rd via `RD_HOST=bunker`; no Windows box; no DISK LOW.

**Gates:** `rd check` **PASS** (step 1 f209787, step 2 f210218). `rd verify` **MATCH** at f234522 (rng f7da67f8, semv 22b1b09b2cfbb301, fb 50bbf0ae35ffbc44).

**End save:** none new — no save point on the route (manor / graveyard / catacombs entrance have none seen). Last save still `a2a16253c9bb` (White Rose Inn slot 22). The next leg must continue from the run (`--from story-d9b`), not a save.

## Route (frames)
- f209786–f218678 (previous driver, reconstructed from checkpoints + its looks): 310140 top floor = **Stella's room** (ck stellaroom f210727), hall, ground floor talks (maid, cook in kitchen 310150, a butler "malter" ck f216757), evening → Rowen → dinner. On resume (session rebuilt by a 527 s replay) the game was in the **dinner scene** (310120, caviar talk), `story` 1420.
- f219264: dinner done, `story` **1420 → 1430**. Bedroom scene (Stella's 310120 upper floor, `tod` 1 → **4** = night). Arche wakes alone (party 1), moans, then Stella: "entrance to the graveyard behind the mansion". Control f222968, party 5 (Arche + Stella).
- 310140 top floor: the box at x 103600 (code 70220) = **Item Storage** (deposit / withdraw, 128 slots), not a chest. Nothing taken.
- Hall 310130 top → down: walk L to the far-left landing (12800, 32800), then R down the stairs to the middle floor, R to the landing (177200, 58400), L down to the ground (f225437). Zone 7 (140800) → **310150 = kitchen** → zone 4 (x 99200, the **back door**) → **310160** (outside, behind the manor) → walk R → zone 2 (192000) → **390110 Chartreux Graveyard** f226797.
- 390110: scene (noises), **Sana joins** → party 7, control f228800. Walk left; Vile Bats (51062). Exit zone 2 (x 51200, press up) is held by bats: fight search (`zrest` in cell) → **390120** f230612.
- 390120: bats cleared by a `foe == 0 --hold 40` search (f231765). **The pushable gravestone** = the first one at x ≈ 108000 (sword sheathed; pushing it from the left does nothing). Jump onto it (`L*20 .*5 RC*20`), step off right, hold L: it slides left (lpose 114) until lx 87248 → zone 2 (70440, handle 390120503) rises y 48000 → 30360, **state 1 → 0**: a crypt doorway appears at **x 92800** (f232615).
- U at x 92800 → scene → U again → **400110** (f233221). Scene ("let's show them our violence"), control f234492 facing the 2 Ghosts.

## Party at the end (f234522)
All Lv10. HP: Arche 137/216 (MP 39), Sana 104/149 (156/161), Stella 130/189 (120/132). Gear unchanged from d9a: Arche Org Slayer / Thick Leather / Leather Cape / Kitty Ears; Sana Mahogany Staff / Leather Armor / Chest Sash / Green Smock / Anti-Poison Ring; Stella Lady's Dagger / Woven Gloves / Salamander Scale (**no body armour**). **5261 CP** (+122 from bats). Items as d9a (Better Herb ×9, Magic Candy ×4, Recall Lamp, Revival Drink, Venom-Away S ×2).

## Upgrades taken / skipped
- **Ghost Blaster (night weapon store): not visited.** 5261 CP ≪ 12,500; a trip from the manor to 300220 and back is ~5k+ frames for a look only. Night stock not confirmed.
- **Bronze Armor for Stella: skipped** — the free [Scale Armor] is in Catacombs step 2, a few rooms ahead.
- No chests on the route (390110/390120 had none; 400110 has a chest 70230 at x 114200 = the Crypt Key chest after the ghosts — next leg).

## Game facts / walkthrough corrections
- Dinner → bedroom → night is automatic after the manor talks; `story` 1430 from dinner on; night `tod` = 4.
- Kitchen = **310150** (hall ground zone 7 → 310150, back door = zone 4 at x 99200 → 310160 → 390110 at x 192000).
- Sana joins in 390110 as soon as you arrive (scene), not deeper.
- Gravestone: in **390120**, the first stone right of the entrance spot (≈108000). Push it from its RIGHT side, sheathed; it slides ~20k left; doorway at 92800 (press ↑).
- 310140 top-floor box (70220) = Item Storage (store items / see money max 40000 CP).
- 400110 objects: 70230 at (114200, 22400) handle 400110501 (chest), 70217 at (108800, 27200) handle 400110502 (locked door?), 70220 at (65600, 32000) handle 400110601 (another storage box?). Exits: zone 1 back (22400), zone 2 (182400, 48000) → 400120.

## Named state (this run)
- `p0 p1 p2` = HP of party slots 0–2 (`s32(s32(s32(world+0x2784)+0x4030+4i)+0xa64)`), `pmin` = min over active members.

## Tool gaps / notes
- **The bunker session died / was rebuilt twice** (527 s and 540 s replays): checkpoints dropped "no live savestate". Probably host memory pressure.
- **Very slow host:** state-reading inputs (`{toward(..)}*150`) took > 3 min for 75 frames; one search ran at 8 ticks/s. Prefer fixed key counts.
- `rd check` and `rd verify` each replay ~10 min.
- Chained `Z` right after a combo did not sheathe (sword stayed 1); a `C` press while pushing did not jump — release first.

## Repo edits
None. No commits.

