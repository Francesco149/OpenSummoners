# probe story-day9d

run `story-d9d`; accepted 2026-11-19 05:38

check PASS (tools/drive/tasks/story-day9d.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day9d/trace.jsonl (282638 frames)

## driver's report

# story-d9d: Catacombs 4.4–4.5 → Memory Photo 6, Crypt Key, Doll Hand, high lever (400155) → 400160

**Run:** `story-d9d` (`rd new --from story-d9c`; every rd command used `RD_HOST=bunker`; no Windows box; no DISK LOW).
- **Start:** f264278 in 400146, control, party 7 (Arche, Sana, Stella).
- **End:** f282638 in **400160**, the 4.6 room (tombstones), control, Arche leading, sword sheathed, Follow Mode back to Follow Me!.
- Used 18.4k of the 40k frame budget.
- I stopped once the budget gates were done (I reached ~$6). 4.6–4.8 are still open (tombstones, Master Ghost, MoH 13, Thieves' Key).

**Gates (final movie f282638):**
- `rd check` **PASS**: step 1 at f264279, step 2 at f264601.
- `rd verify` **MATCH**: rng 9aa36819, semv caff1b4669773fb6, fb 0cf6d3a84597fbd9.
- An earlier verify at f282441 also matched.

**End saves:** none. There is no save point on this route. The last save is still `a2a16253c9bb` (White Rose Inn, slot 22). The next leg must continue with `--from story-d9d`.

## Route (frames)
- **400146 → 400145 → 400140** (f264563–265121). Leave the MoH room by zone 1, then 400145 zone 1. In 400140 you arrive at the pot door (12800, 116000). Press ↑ only when standing still; a ↑ while moving is ignored.
- **400140**
  - The gap at x 36–52k on the bottom floor is a **pit** (−11 HP, back to the ledge). It is not a tunnel. Run-jump over it.
  - The stair vines 70400 (x 78–88k) had **reset** on re-entry. Stella burned 6 of 8 again (f265376–265831):
    - Z toggles her to fire mode.
    - `RX` burns the low pieces; `U*3 UX` burns the high ones.
  - Up the slope into the middle room. The 2 Skeletal Soldiers (51027) held the far-right exit; zone hold showed them next to the stranded Arche / Sana. Arche killed them with a search (f267280).
  - Zone 2 (227200, 64000, walk-in) → **400141** at f267564.
- **400141** (the "right, use key" room)
  - A Ghost Mage, a Ghost and 51052 were here (fight f267604–269413).
  - Keyhole **70217 (88000, 30400)**: V with the Crypt Key opened gate 70230 (95000) at f269589.
  - Zone 2 (112000, 38400, ↑) → **400150** at f269737.
- **400150** (water area; 116 × 32 cells)
  - Two water channels shaped like a U, with pits at the surface. Only Sana swims. **Sana loses HP slowly while underwater** (105 → 56 over ~2000 f).
  - Chest **70220 (228800, 92800)**, bottom of the right channel = **[Memory Photo 6]** (Sana, f270462).
  - Chest **70220 (116800, 92800)**, bottom of the left channel = **[Crypt Key]** (f272503).
  - Chest **70220 (353600, 32000)**, upper right past gate 70230 (274200) and wall lever 70216 (281600) = **[Doll Hand]** (f275313). It opened only with V from its right side, facing left.
    - Upper-right enemies: 2 Skeletal Soldiers and a Ghost Mage.
    - During the search the lever / gate went back to 0. I flipped the lever again with V at lx ≈ 283900 facing left.
  - Left gate 70230 (80600) / keyhole 70217 (84800, 46400): V → "You used Crypt Key. The Crypt Key has broken." (f277185).
  - 2 Ghost Mages (51082) held zone 2:
    - Arche's sword fights failed: the mages heal each other (pair HP 181 → 330), and their spells nearly KO'd Sana. I undid that.
    - **Stella in fire mode (`UX`, `C*8 X`, `LX`) killed both** (f278500).
    - Phoenix (`n3`, hold X 150) fired but hit nothing; the mages were out of range / idle.
  - Zone 2 (38400, 51200) → **400155** at f278580.
- **400155** (4.5: 2 baskets + high lever)
  - Objects: baskets 70460 at x 52800 (left) / 84800 (right), lever 70216 (91200, 12400), gate 70230 (114200). One Skeletal Soldier and one Ghost Mage were killed (f280078).
  - **The baskets are a balance.** Each basket's body y: 0 v 0 → both 30400; 1 v 0 → the loaded one goes to 41600 (floor) and the other to 19200; 1 v 1 → both 30400; 2 v 1 → the 2 side goes down. A rider stands at basket y − 5600.
  - **A stone ledge left of the left basket** (lx ≈ 41700) stands at **ly 26400**. From it, a run-jump `RC*20 C*15` reaches the left basket even when it is fully up (stand 13600).
  - Follow Mode: All Members 'Stay There!' and each member set to **Match party**. The rows showed 'Follow Me!' before, which overrides the party setting.
  - Order that works:
    1. Arche into the right basket; it sinks to the floor.
    2. Sana to the ledge, then onto the left basket (f281725); the baskets level out at 30400.
    3. Stella from the floor `C*30 L*10` onto the left basket (f282000); the left goes down and the right goes up to 19200.
    4. Switch to Arche (stand 13600), `R*10 V` → lever 0 → 1, gate open (f282122).
  - Follow Mode set back to All Members 'Follow Me!' (f282372). Zone 2 (134400, 41600) → **400160** at f282598.
- **400160** (end): 5 zones.
  - 1 → 400155; 2 (28800, 96000) → 400175; 3 (153600, 60800) and 4 (128000, 22400) → 400170; 5 (70440, (208000, 54400), state 1) → 400180.
  - The two **70800 at (192000, 28800) and (225600, 27200)** have handles 400160501/502. They are probably the 4.6 pushable tombstones (not confirmed).
  - Foes: Vile Bat (51062), 51052 ×4, Ghost Mage, Ghosts.

## Party at the end (f282638)
- **Arche** Lv11, 225/225 HP, 39/39 MP: Org Slayer, Thick Leather, Leather Cape, Kitty Ears.
- **Sana** Lv11, 157/158 HP, 71/176 MP: Mahogany Staff, Leather Armor, Chest Sash, Green Smock, Anti-Poison Ring.
- **Stella** Lv11, 143/195 HP, 86/140 MP: Lady's Dagger, Scale Armor, Woven Gloves, Salamander Scale.
- No level-ups or gear changes on this leg.
- **8379 CP** (+586 from drops).
- New items: Memory Photo 6, Doll Hand. Crypt Keys: 0 held (2 used, 1 found).
- Other items as at the start, plus the Ghost Marshmallow etc. No herbs were used; healing was Sana's slot-2 heal (4 MP each).
- **Upgrades:** none were available (no shop, no gear chest). Doll Hand was not equipped; I did not check what it is. No stat items were on this stretch.

## Game facts / walkthrough corrections
- 4.4 "right, use the key, tunnel" = 400140's middle room → upper-right walk-in exit → **400141** keyhole → **400150** water area. In EN-old the order inside 400150 is: Memory Photo 6 (right channel) → Crypt Key (left channel, the "pit after next") → Doll Hand (upper-right chest, needs the 274200 gate / 281600 lever) → left keyhole gate → zone 2.
- Sana takes slow damage while underwater.
- Sana's slot-2 spell heals the nearest low member or herself. Hold X ~90 frames, press `.` first; one cast heals ~100 HP for 4 MP.
- Stella's fire mode (`UX` diagonal, `C*8 X` air shots) kills Ghost Mages that Arche's sword cannot out-damage, because the mages heal each other.
- Party leader swap: `ent` order is Arche → Sana → Stella. `lid` changes ~16 frames after the press. Two quick presses sometimes register only once, so wait about 20 frames between them.
- Object 70800 = gravestone-like objects with handles (400140, 400145, 400150, 400160).
- 70460 = hanging balance basket. Its y = `s32(s32(s32(world+0x11e0+4*slot)+0x40)+8)`.

## Named state (this run's vars.json)
- `p0 p1 p2 pmin nfoe fhp`: copied from d9c.
- `sk`: slots 5+6 HP.
- `f3`: slots 0–2 HP sum.
- `f2`: slots 0–1 HP sum.
- `gm`: slots 3+4 HP sum.
- `pl pr`: the 400155 basket y values (object slots 0 / 1, body +8).
- Helper script (scratchpad) listing actor slots as code / hp / x / y / targetable: `(s32(c+0x1d4), s32(s32(c+0x750)+0x54), s32(s32(c+0x40)+4), s32(s32(c+0x40)+8), c+0x1d0)` with `c = s32(world+0x1160+4i)`. **Enemy positions are body +4 / +8 (`s32(c+0x40)`), not `c+4`.**

## Tool gaps / notes
- **BUG: `rd ck R drop NAME` wipes the live session.** `cmd_ck` "drop" never calls `r.attach()`, so `r.sess` is None. It then calls `r.save_sess()`, which writes `null` into session.json. The live process is orphaned on bunker and keeps using the shared 5 GB pool until it idles out. The next command replays from boot (~12–15 min, 3 times this run). The drop also always reports "it had no live savestate" and frees nothing. A fix is to attach (`cached_only=True`) before popping, or to skip `save_sess` when `sess` is None.
- Because of that bug and other bunker sessions, searches sometimes ran at 40–110 ticks/s instead of ~1400. "savestate pool exhausted" hit after ~13 checkpoints; dropping them all (one replay) recovered.
- Hold-search goals for "standing on X" must pin `ly` to the surface (`abs(ly - (pl - 5600)) < 600`). Otherwise mid-jump frames pass the hold.
- A search goal of "near the chest" lets momentum carry the leader past it. A goal of "chest state == 1" with `V` actions worked better.
- No game-over and no forbid hit.

## Repo edits
None. No commits.

