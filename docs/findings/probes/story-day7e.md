# probe story-day7e

run `story-d7e`; accepted 2026-11-18 00:30

check PASS (tools/drive/tasks/story-day7e.md)
verify MATCH (+ real device)
filed tests/scenarios/story-day7e/trace.jsonl (38431 frames)

## driver's report

# story-d7e: Chartreux → Carudale Tunnels, homeward (Day 7 end) — partial

**Run:** `story-d7e`, from `save:5e899fce8699` (slot 13, Dancing Piglet Inn). Ends **f38431** (37.6k frames after load @f868; ~40k budget hit — an extra 2.5k-frame search in 330110 was rewound, it only cost HP). All RD_HOST=bunker; nothing touched Windows.

**Gates:** `rd check story-d7e tools/drive/tasks/story-day7e.md` **PASS** (step 1 f869, step 2 f3001). `RD_HOST=bunker rd verify story-d7e` **MATCH** (f38431, rng f5d711a3, semv 31e63475379c5c3f, fb 52003a7ac370b2b0).

**End state:** f38431, room **330110** (Carudale Tunnels, first cave room from the Loch side), just arrived at its west entry (lx 0, ly 125600), control. Arche 163/197, Sana 86/104 (MP 35). Gold 1172. `story` 1210. **No new save / end-save** — no inn/statue on this stretch. Next leg: `rd new NEXT --from story-d7e:r330110` (or `story-d7e@38431`). Exit to Loch Carudale = 330110 zone 1 **press-up at (195200, 32000)** → 260230; route = zig-zag up the right side (map `/mnt/ephemeral/opensummoners/drive/runs/story-d7e/view/map_330110_f38431.png`); 1 Merkid Mage (51041) + Lead Merkid (51042) on the way.

**Not reached:** Tonkiness / home save / Day 8. Load (Lead Merkid fights + the boat) ate the budget.

## Route (frames)
- f868 loaded 300240 (title: U, Continue, R → page 2, cursor lands on slot 15 → U U → slot 13).
- f1248 → 300120; f1830 → 265110 (exit = ↑ at x 281600); f2654 → 265120.
- 265120 (→265130 walk-in at 201600,12800): the rising cliff at x 166400 can't be jumped from the low ground (pit at 164–170k = −9 HP). Route: climb to the upper ledge (x ≈108000, ly 26400, f4895), then run-jump + air jumpkicks right. f5164 → 265130.
- f5486 Heal Water (Sana) → Arche full. 265130 → 265180 is ↑ at (44800, 38400); a Blue Slime beside the door holds `zrest`; killed → f7345 265180. f8132 → 265190.
- 265190 exit (150400, 12800) held by 2 Lead Merkids + Merkid Mage (attacking Sana at the west side): fight search killed all 3 (f9550) → f9934 265200. Heal ×2 (f10438).
- f10638 → **330140 (lake)**. Sunken boat 70600 at (196800,108800) again (reset since d7c).
  - Sana (leader) dives onto it (f11741) → rises (by 108800→~51200, f12001); a **Lead Merkid boards with her** and knocks her off / kills her if she just rides. Working recipe: face left, Heal Water while it moves → left stop bx ≈131200 (f12389, Sana 95).
  - Arche never boarded at the left stop (she was off fighting a Merkid Mage + Lead Merkid on the west shore, ax ≈ 92–130k). **Sana crossed alone**: face right → boat to right stop bx ≈ 384000 → step off → right shore (f13724). Room exits take the whole party along (Arche followed).
  - Right-exit (457600, 25600, walk-in) blocked by a Lead Merkid (zrest 30). Sana killed it with **Diamond Dust** (n1, hold X 130) + Heal Water (f14291–17548; ~3.3k frames, the costliest part). f17967 → 330135.
- 330135: heal (f18407), Arche crossed the platform cavern by search (f24222 — 5.8k frames; went past the door to x 327572) → ↑ at (259200, 32000) → **330130 f24613**.
- 330130 (exit ↑ 329600,25600): shaft at x ≈295–318k up to the exit ledge. Sana fell behind into the bottom pit (≈ x 227–250k, y 80000) with 2 Lead Merkids and nearly died twice; fixes: **F1 → Items → Herb → Sana** (f27246, cast bar "Herb-Sana", +~99), Arche went down and killed both (f28881), killed slimes in the shaft / at the door → f30923 330128.
- 330128 (↑ at 182400,25600, straight up a zig-zag): climb f30963–34371; 2 Lead Merkids at the door (killed f34991) → f35052 330120.
- 330120: **Magic Candy → Sana** (+61 MP, f35404), Heal Water ×3; climb to exit (201600,19200; walk-in, but held by a Lead Merkid on the ledge above — killed with jump attacks f38221) → **f38391 330110**.

## Party at end (unchanged gear)
- **Arche** Lv7, 163/197 HP, 31 MP: Iron Sword, Thick Leather, Woven Gloves, Accessory empty.
- **Sana** Lv5, 86/104 HP, 35/108 MP: Iron Staff, Leather Armor, Chest Sash, Green Smock.
- Items: Herb ×2, Better Herb ×7 (by menu rows), Revival Drink, Magic Candy ×2, Mentama Drop, Merrin Card, Kitty Ears, Underwear, Taiyaki Sweet Sticks. Used: 1 Herb (Sana), 1 Magic Candy (Sana).
- Upgrades: none available on this stretch. **Skipped [Life Dew] / [Cloth Gloves]** (330140 water chests): Sana alone cannot hold the lake against the Lead Merkids (she nearly died twice just riding the boat); no real attempt made. No shops passed.

## Game facts / walkthrough corrections
- Boat return crossing (330140 east-bound): Sana dives on the sunken boat (x 196800) → it rises; facing left takes it to the left stop (bx ≈131200), facing right to the right stop (≈384000). Turning needs a fresh ←/→ press while not casting (input during the Heal Water animation is dropped). A Lead Merkid tends to board at surfacing.
- Sana can do the whole crossing alone: the party follows through the exit zone.
- Sana's Diamond Dust kills a Lead Merkid (112 HP) on land; ~10–20 per hit.
- Items menu: Magic Candy is used at once from F1 → Items (no cast bar; "Sana recovered 61 MP"); Herb used from the menu on Sana = a timed cast bar "Herb-Sana" (~200 frames) then +~99 HP. "On who?" lists Arche / Sana.
- Party Commands submenu: Magic Usage / Follow Mode / Formation (no regroup / warp).
- 265120 east-bound: the low route ends at a pit before the cliff (x 164–170k); the way up is the upper ledge from the west steps.
- The Carudale Tunnels respawn all Lead Merkids on re-entry; they block the ↑ exits (zrest) and are the main time sink (each 110–130 HP, guard a lot; jump attacks / Arcing Slash land).

## Named state (run vars)
ahp / shp / smp (party slot 0/1 HP, Sana MP: `s32(s32(s32(world+0x2784)+0x4030/0x4034)+0xa64/0xa6c)`), ax ay / sx sy (party bodies via slot+0x9f4→+0x40), nfoe (targetable faction-2 in char slots 0–9), bx by bst (zone/object actor slot 7 = boat in 330140), m7 / m3 / e2 / e3 / e4 / e8 (per-slot foe HP, 0 when not targetable).

## Tool gaps
- `actors()` is unmapped; counting hostiles over 32 slots exceeds the 598-token stop-condition limit (had to cap at 10 slots).
- `rd search --score` on absolute coords crossed into the neighbouring room (zone ↑ near the entry) and maximised there; needs `room` in the avoid every time.
- Dropping checkpoints triggered a session rebuild (115 s replay) — same as d7c/d7d.
- No way to see which object/actor slot is which without ad-hoc eval loops (bx/by were tied to "slot 7" and changed meaning per room).

## Repo edits
None. No commits. Helper scripts in the session scratchpad (arrive.sh).

