# probe story-day11f

run `story-d11f`; accepted 2026-11-26 21:08

check PASS (tools/drive/tasks/story-day11f.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11f/trace.jsonl (741344 frames)

## driver's report

## story-d11f report

**Run:** `story-d11f` (--from story-d11e), RD_HOST=bunker throughout. It starts at f715348 in 440130 (lower floor) and ends at **f741344** in **440240**, the pot puzzle room. At the end the game is in free control with Stella as leader, standing on top of block I. That is 26.0k frames.

**End save: `0c5856a7a482`.** It is in slot 8, which held "White Rose Inn, Stella LV2, 4:15", the shortest play time outside slots 1–7 and 24. I saved on the 440220 bed (95200) at f731511, after the nap's full heal and **before** the puzzle.
- `--from save:0c5856a7a482` restarts at the bed.
- `--from story-d11f` continues inside the half-solved puzzle.

### Where it blocks: block II
- II sits at (67200,7600) in rose vines (code 70400). It rests on the vine piece at (70000,14000).
- Vine pieces on both sides of II overlap it: (62000,4400/9200) on the left and (74800,4400/9200/12400) on the right.
- Arche can reach II from the upper-right ledge (lx 74112, push pose 114), but II will not move while those side pieces exist.
- Every Stella shot that burned the side pieces also burned the piece under II, so II dropped straight into III's cell (67200,54400). I reloaded after each.

What each shot burned:

| Stella's shot | Burned | Effect on II |
|---|---|---|
| ↓X from the upper-right ledge facing left, lx 77248 | only (70000,14000) | falls into III's cell |
| ↑X from lx 77248 or 81304 | (74800,12400), (70000,14000), (62000,4400), (62000,9200) | falls into III's cell |
| jump + →X from block I's right edge (two heights) | (62000,4400), (62000,9200), (70000,14000) | falls into III's cell |
| ←X on the upper-right ledge | nothing, no projectile seen | — |

- Vine bodies are 4000×4000 (+0xc/+0x10); state 1 means burned. Fire spreads to other pieces in a chain whose rule I could not pin down.
- **Suggested next step:** reverse-engineer the code-70400 fire spread. Leads: `0x40afe0` (engine-quirks.md:4193) and the 70400 branches in all_ida.c around lines 5338, 7615, 8099, 8737. Then:
  1. Clear only the side pieces.
  2. Push II left 6400 to x 60800; it stays on the (70000,14000) piece.
  3. Burn the piece under it; II drops to (60800,54400).
  4. Push I off the upper ledge's right edge onto IV.
  5. Place III with the guide's balance-platform method.

### Route (frames)
| frame | what |
|---|---|
| 715348–717413 | 440130 fight; climb outside: platform 3 (stand 84000) → platform 2 (58400) → chest ledge (45600) → platform 1 (32800) → upper floor (23200) |
| 717621 | ↑ (163200,25600) → 440150 |
| 719542 | ↑ (96000,35200) → 440160 |
| 720776–721588 | 440160 lower floor: Kobold Ace ×2 + Baby Aquamage (51241) killed |
| 721649 | door (83200,89600) → 440190 bottom |
| 722095–725600 | left shaft → middle corridor (Young Harpy, Kobold Ace, Aquamage killed) → right shaft → top floor |
| 727904 | ↑ (108800,32000) → 440200, arriving at (25600,87200) |
| 730054 | ↑ (38400,28800) → 440210 |
| 730678 | → 440220 bedroom |
| 731511 | nap → full heal → save, slot 8 |
| 732091–732450 | 440210 pot 70450 (69000,42400), V → scene → "Do you want to start fixing the pot?" → Yes |
| 732765 | → **440240** "Fixing The Broken Pot" ("break any block = reset") |
| 733729 | VII pushed to the far left (25600) |
| 734478 | IX off the right ledge's left edge → (67200,67200) ✓ |
| 734930 | VIII off the left ledge's right edge → (54400,67200) |
| 735348 | V onto VIII |
| 735533 | VIII+V stack pushed left to 51200 |
| 735973 | Arche set to 'Stay There!' (F1 → Party Commands → Follow Mode) |
| 737060 | Stella on top of I |
| 737276 | Stella →X: the vines under VI burn, VI drops onto the stack |
| 738299 | stack pushed right to 60700; VI rides along |
| 738443 | VI pushed onto IX |
| 738958 | VII pushed back to 54200 |
| 739616 | IV pushed off the upper ledge's left edge onto the left ledge |
| 740188 | **lever 70216 (16000,64000), V facing it: snaps every block to the 6400 grid** |
| 740778 | IV off the left ledge's right edge → (54400,60800) ✓ |
| 741231 | Arche reaches the upper-right ledge via the right jump-through steps |
| 741344 | Stella ↑X from I's top burns the vine cluster next to her (frees her); end |

### Puzzle board (440240), first time seen
- **Target cells** (top-left corners): row 1 I/II/III at x 54400/60800/67200, y 54400. Row 2 IV/V/VI at y 60800. Row 3 VII/VIII/IX at y 67200.
- **Well:** the board is a well three cells wide (x 54400–73600).
  - Left ledge: x 9600–54400, top 48000; the part x 9600–25600 is one 3200 step higher.
  - Right ledges: 73600–92800 and 99200–112000, top 48000.
  - Upper-mid ledge: x 35200–54400, top 25600. Upper-right ledge: x 73600–99200, top 22400.
  - Floor under the well: top 73600; x 3200–25600 and 99200–112000 are a 3200 step higher (70400).
- **Getting around:** left-wall jump-through ledges at x 3200–6400, y 35200/48000/60800 (↓+C drops through). Right jump-through steps at x ~101000–105600, y 38400/25600, lead up to the upper-right ledge.

Blocks 70910 (position = top-left corner):

| handle | numeral | start position |
|---|---|---|
| 501 | VII | (83200,67200), right floor |
| 502 | VIII | (46400,41600), left ledge |
| 503 | IX | (73600,41600), right ledge |
| 504 | IV | (35200,19200), upper-mid ledge |
| 505 | V | (32000,41600), left ledge |
| 506 | VI | (54400,6800), in the vines |
| 507 | I | (46400,19200), upper-mid ledge |
| 508 | II | (67200,7600), in the vines |
| 509 | III presumably (numeral not seen) | (118400,67200), far-right floor |

Other objects: balance platforms 70420 at (92800,60800) and (112000,60800); lever 70216; torches 70371.

**Mechanics confirmed:**
- A block pushed off an edge moves a full cell and lands snapped to that column.
- Pushing the bottom block of a stack moves the whole stack.
- A push stops about 100 short of the next block. The lever fixes that, and you need it before "IV off right edge": without it, IV landed on V's corner at (54400,54400).
- Arche can push from a floor one 3200 step higher than the block.
- Followers do not push blocks.

The guide's [DK] order works as written up to IV, apart from that lever step.

**End state:** VII, VIII, IX, IV, V, VI are placed. I is still at (46400,19200), II at (67200,7600) held by its vines, III at (118400,67200).

### Party at end
- **Arche** Lv16 (new this leg) 281/281 HP, 51 MP, Atk 73, Def 67: Org Slayer, Scale Armor, Leather Cape, Kitty Ears. Follow Mode is 'Stay There!'.
- **Sana** Lv15 209/209: Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv15 237/237: Lady's Dagger, Scale Armor, Woven Gloves, Magic Bracelet.
- 18856 CP; no items used.
- No upgrades available on this route (no shops or chests). Abilities were not reassigned.

### Walkthrough corrections (EN-old)
- **5.7 save:** the 440220 bed at 95200 (V; "Looks like you can take a nap" Yes, then "Would you like to save?").
- **5.7 repair:** starts at the pot 70450 in **440210** (69000,42400) and leads to the puzzle room **440240**.
- **Way back to the bed from 440130's lower floor** (about 15k frames with fights): outside platforms → 440130 upper floor ↑ (163200,25600) → 440150 → ↑ (96000,35200) → 440160 lower door (83200,89600) → 440190 left shaft → middle corridor → right shaft → top floor ↑ (108800,32000) → 440200 (arrive 25600,87200) → climb → ↑ (38400,28800) → 440210 → 440220.
- **440160 Baby Aquamage (51241):** casts Sleep Mist on the whole party, plus a party-wide spell that took Arche 66 → 16. Kill it first.

### Named state (run-local)
- Party: `h0..h2`, `hmin`.
- Enemy HP: `eh0` (short 8-slot form), `el` (slots 0–2), `e34`, `e67`, `hk`.
- Blocks: `b1x..b9y`; `b6ax/b6ay` = VI, `b6bx/b6by` = II, `b3x/b3y` = presumed III.
- Platforms: `plx/ply/prx/pry`.
- Party positions: `ax/ay` (Arche), `m1x/m1y` (Sana), `m2x/m2y` (Stella).

### Tool gaps and lessons
- `rd strip` replays from boot when no savestate is near. One strip at the bed took about 6 minutes; meanwhile the main session idled out and was rebuilt (41 min), and every checkpoint was lost. `rd verify` also rebuilt it (36 min). Avoid strip and `--at` on long movies.
- The savestate pool holds about 30. When it is full, search prints only the error, which is easy to miss.
- Searches return long paths (1000–2000 frames); hand-written jump sequences were far shorter.
- `ent` right after a menu closes gets dropped; check `lid` after each leader switch.
- There is no probe for vine 70400 burn and spread relations.
