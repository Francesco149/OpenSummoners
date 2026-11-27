# probe story-day11d

run `story-d11d`; accepted 2026-11-26 15:15

check PASS (tools/drive/tasks/story-day11d.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day11d/trace.jsonl (675176 frames)

## driver's report

# story-d11d: Archmage's Tower 5.1 → 5.2 (done)

Run `story-d11d` (--from story-d11c), RD_HOST=bunker throughout (Windows box untouched). Start f639326 (440110) →
end **f675176** in **440220** (headmaster's bedroom), control, leader Arche. 35.85k frames.

Gates: `rd check` PASS (step 1 f639327, step 2 f642081, forbid clear). `rd verify` **MATCH** after rebuild: frame 675176
rng 7744137d semv 2a476cce74275e67 fb 2cf5da7e4d29dec1. Caveat: the live session's semv (0cf714f8dab67567) differed from
the headless replay (frame + RNG equal) = session drift; evidence collection failed (bunker session ended mid-dump,
SessionEnded); clean-replay rebuild matched the replay. Movie is good.

End save **8b171418cd3b** — slot 7 (was "Chartreux City, Stella LV2, 4:06" = shortest outside 1–6/24) now
"Archmage's Tower 9:36, Arche/Sana/Stella LV15". Saved via the bedroom bed (nap → "Would you like to save?").

## Route (frames)
| frame | what |
|---|---|
| 639326–640143 | heal: 3 Better Herbs (Arche/Sana/Stella full) |
| 640660–641567 | 440110: Pink Slime fight (search) |
| 642080 | 440110 zone 2 ↑ → **440120** (step 2) |
| 642556–644109 | 440120: Better Herb → Sana; fought left (Arche → Lv15) |
| 644209–644333 | 440120 far-left chest 70220 (30400, 32000) **[Mind Chocolate]** |
| 645832 | 440120 zone 2 ↑ (131200, 44800) → **440130** |
| 645872–646693 | 440130: outside-left platforms → chest ledge |
| 647245–647439 | 440130 chest (35200, 48000) **[Tough Drop]** → Arche (Def +2, f647886–648136) |
| 648489–649786 | 440130: up into tower upper floor, killed Org Swordmaster/Bowman |
| 649833 | 440130 zone 2 ↑ (163200, 25600) → **440150** |
| 649873–652141 | 440150: fights; zone 2 ↑ (96000, 35200) → **440160** |
| 654424–654588 | 440160 chest (166400, 35200) **[Good Child Drop]** → Sana (Spi +2); Mind Chocolate → Sana, Magic Candy → Stella |
| 656199–657597 | 440160: drop off left end to lower floor, kill Kobold Ace + Baby Aquamage, door ↑ (83200, 89600) → **440190** |
| 657637–665344 | 440190: climb left shaft → middle corridor → right shaft → top floor; fights; zone 2 ↑ (108800, 32000) → **440200** |
| 665613 | 440200 **MoH 16** (object 70100 at (72000, 78015), walk into it; full heal; Congratulations ~850 f) |
| 666468–668320 | 440200: climb to top, zone 2 ↑ (38400, 28800) → **440210** |
| 668472–671701 | former-headmaster scene (event zone 101 x 68800; pot + Arche/Stella squabble) |
| 672062–672478 | 440210 rightmost shelf 70353 (122800) 'Magical Girl Merrin Part 12/12' (read: Yes, 3 pages) |
| 672698–672852 | 440210 far-right drawers 70363 (147200, handle 440210601) **[Archmage's Tower Key]** |
| 672973 | 440210 zone 2 ↑ (156800, 48000) → **440220** bedroom |
| 673348–673492 | 440220 wardrobe 70353 (66800, handle 440220601) **[Magic Bracelet]** (V facing right) |
| 673880 | Magic Bracelet → **Stella** |
| 674167–675156 | bed 70352 (81600): nap → full heal → save prompt → slot 7 |

## Party at end
- **Arche** Lv15 280/280 HP, 48 MP: Org Slayer, Scale Armor, Leather Cape, Kitty Ears. Tough Drop (Def +2).
- **Sana** Lv15 209/209, 268 MP: Rune Staff, Scale Armor, Chest Sash, Green Smock, Anti-Poison Ring. +Good Child Drop (Spi +2).
- **Stella** Lv15 237/237, 206 MP: Lady's Dagger, Scale Armor, Woven Gloves, **Magic Bracelet** (Spi 63→83).
- Gold 15987 CP (cap now 90000). MoH 16.
- Bag (39/48): Better Herb ~7 (one ×3 stack + 6 singles), Best Herb ×4, Life Dew, Revival Drink ×3, Venom-Away S ×3,
  Sweet Sticks, Magic Candy ×3, Recall Lamp, Orange Beret, Salamander Scale (unequipped), Chest Sash, Underwear,
  Blessed Earring, Merrin Card, Doll Hand.

## Choices / skipped
- Magic Bracelet → Stella: equip preview: Arche Spi 37→57 but Atk −6/Def −6 (replaces Kitty Ears); Sana Spi 86→106 but
  Def −6 and loses Anti-Poison Ring; Stella Spi 63→83 with no loss (replaces Salamander Scale, fire resist only).
- Blessed Earring (magic resist + anti-confusion) still in bag: Kitty Ears give Arche Atk/Def; kept them.
- Good Child Drop → Sana (healer). Abilities from Lv15 not reassigned (no menu visit).
- 440150 chest 70220 at (27200, 19200) on an outside pillar top-left: not on the walkthrough route, not reached (skipped).
- 440190 gate 70230 (31000, 22400) + "?" object 70217 (36800, 28800) at the top-floor left end: V on 70217 did nothing.
- No shops on this leg.

## Walkthrough corrections (EN-old rooms)
- 5.1 room chain: 440110 → (zone 2 ↑ 150400) **440120** (Mind Chocolate far left) → (↑ 131200,44800) **440130** ("up;
  left ledges [Tough Drop]" = OUTSIDE the tower: from the entry floor jump left onto floating platforms (x 89600–102400,
  stand ly 84000 / 58400 / 32800), the stone block (stand 71200) and the chest ledge x 32000–76800 (stand 45600); "right,
  stairs by locked wall" = jump from the top platform into the tower's upper floor, zone 2 ↑ (163200, 25600); Org
  Swordmaster + Org Bowman there) → **440150** ("left, stairs": zone 2 ↑ (96000, 35200)) → **440160** ("right [Good Child
  Drop]" chest (166400, 35200); "left, drop": drop off the upper floor's left end outside, land on the lower floor
  (stand ly 84000–87200, x ≥ 28000); "stairs" = wooden door ↑ (83200, 89600)) → **440190** ("climb": left shaft with 3
  jump-through ledges x 16000–22400 — jump up-left around the ledge above, then right; middle corridor right; right
  shaft ledges x 188000–195000 to the top floor ly 29600; "stairs" = zone 2 ↑ (108800, 32000)) → **440200** (MoH 16 at
  (72000, 78015) on the lower floor; wooden door = zone 3 ↑ (172800, 60800) → 440205; "up → fancy doors" = zone 2 ↑ (38400,
  28800) → **440210** headmaster's room).
- 5.2: 440210 rightmost shelf = 70353 at x 122800 (Merrin 12/12, Yes/No to read); drawers = 70363 at x 147200 (key);
  wooden door = zone 2 ↑ (156800, 48000) → **440220** bedroom: wardrobe 70353 (66800) = Magic Bracelet; beds 70352 (81600,
  95200): "take a nap" Yes → full heal → "Would you like to save?" → slot list. 440220 zone 2 ↑ (19200, 35200) → 440230
  (stairs to roof, presumably).
- Scene wording: Stella "I believe this is a magical pot"; Arche turns it ("I'm just turning it a little") — it breaks.
- story flag stays 1600 through this leg.

## Named state (run-local)
- `h0..h2`, `hmin` = min(h0,h1,h2).
- `ne`: count of targetable enemies (faction byte +0x1dc == 2 and +0x1d0 != 0) over slots 0–9 + 14–21 (slimes spawn into
  high slots). Enemy slots come first, then the party (party started at slot 6 / 8 / 12 depending on room).
- `eh8` = sum of HP of targetable enemies in slots 0–7 — a good `--score -eh8` for fights; per-room `eM`, `eT`, `eK`
  = HP of named slots. Enemy position: `s32(s32(c+0x40)+4/+8)` (c+4 reads 0 for enemies).

## Tool gaps / lessons
- Postfix limit 598 tokens: an enemy-HP sum over 16 slots is too long; 8 slots fits.
- Fight searches with `--goal ne == 0` stall; `--score -eh8` + cell `(eh8 // 60, lx // 6400)` works, staged goals.
- After movement, a release leaves a slide (~3500 units) and a following same-direction press within ~20 f can dash:
  wait ~40 f before precise walking. Door ↑ zones: ±~2500 around zone x works; U must come after standing idle.
- F1 menu: `f1 .*50 ent` still drops the ent sometimes; open, look, then press.
- Using a stat drop on the LEADER becomes a timed cast; the extra `ent` after it swaps leader. Healing items on others
  are also timed casts by the leader ("Sweet Sticks - Sana") — the game keeps running; don't heal from the menu mid-fight.
- `--watch`/stop-expression reads fine; `rd map` scale 9–12 crops were essential for the shafts in 440190.
- verify: live session semv drifted (frame/RNG equal) — evidence lost because the bunker session ended during `dump`;
  every bunker session death = ~33 min replay (3 replays this leg).

