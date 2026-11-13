# Exercise checklist — every system / screen / branch a test campaign should reach

Tick per campaign. "Where" = earliest point reachable. Sources per item in `mechanics.md` / `walkthrough.md`.
Probe items marked (probe) — existence/behaviour UNVERIFIED in EN-old; confirm in rhost and update.

## A. Boot / title / options
- [ ] Launcher dialog (`#32770`, Launch) → game window — `docs/findings/launcher-dialog.md`.
- [ ] Studio/title intro plays; replays on every return to title (quirk #60).
- [ ] Title: New Game · Load · Options · Bonus Menu (locked text "You cannot access this menu yet." before prologue clear) · Exit.
- [ ] New Game config: change `Game Difficulty` (picker 1:Easy…4:Expert; 5:Nightmare only after a clear) · toggle `Auto-guard` · `Start Game` (≤20-frame fade).
- [ ] Options → every sub-screen: Keyboard Setup (rebind a key; try clearing Jump → "Jump, Attack, and Magic Select are not bound."; bind duplicate → "This button is already bound."), Gamepad Setup, Analog Gamepad Setup, Sound Settings, Graphics Settings (Display FPS, Brightness, Contrast, Graphics Quality), Game & Control Setup (Difficulty, Auto-guard, Dash Input Sensitivity, Message Speed, Magic Quick Select both modes, Combo Meter, Magic Circle Display). Exit via `Save Changes & Exit`, `Exit Without Saving` (confirm), `Reset to Defaults` (confirm).
- [ ] Load screen w/ no saves / w/ saves; "This data cannot be loaded." path (probe: corrupt slot).

## B. Movement & field
- [ ] Walk, double-tap dash, slide (↓ while running), crouch, short/long jump, dash-jump.
- [ ] Enter door (↑), area exits L/R, stairs.
- [ ] Fall into a field pit (−5 HP) — Day 2 fields.
- [ ] Push crate (Day 1 storage room), destroy crate by accident → re-enter resets.
- [ ] Draw / sheathe sword (Z); smash barrels/pots/crates for loot (Day 1 bedroom).
- [ ] Examine with V vs X; talk to NPC; dog follows (Day 1 2nd screen).
- [ ] Camera: F6 modes; POV keys PgUp/PgDn/Home/End.
- [ ] Moving platform, falling platforms, chain platforms (Mountain Cave Day 2–3).
- [ ] Spike trap as elevator (Day 4).
- [ ] Swimming as Sana (Day 3), Mermaid Robe party underwater (Sana lv6), drowning when bubble ends (probe).
- [ ] Boat: Sana fetches / Arche pushes (Carudale Tunnels, Day 7).
- [ ] Steam vent burn + hot water burn (Korat Caves Day 8).
- [ ] Harpy grab-and-drop to lower floor (Weathervane Day 10).
- [ ] Unison triple jump (Day 11+).
- [ ] Teleport-trap chest (Labyrinth of Night) → "You triggered a teleport trap!".
- [ ] Poison-gas trap chest (Day 4 Mountain Cave left chest), spike-trap chest.

## C. Combat
- [ ] Each Arche move (table in mechanics §2) incl. Jumpkick unarmed, Downstab (lv2), Combo Slash (lv3), Wheel Slash (lv5) … Full Moon (lv17).
- [ ] Manual guard (↑) with Auto-guard OFF; Auto-guard ON.
- [ ] Get disarmed (absorb many hits) → fight unarmed → re-arm (probe).
- [ ] Unblockable hit (big damage numbers); shielded enemy (Org Centurion).
- [ ] Combo ≥5 hits → combo meter; party-sustained combo.
- [ ] Interrupt an enemy cast (Merkid Mage, Day 5).
- [ ] Every status on party: Poison (Viper Day 5/6), Sleep (Mothbee pollen Day 5), Confused (Harpy Day 10), Paralyzed (Red Slime / Rattlesnake), Frozen (enemy ice: Merkid Mage Diamond Dust / Ice Dragon Pup), DEF down. Each recovery message ("%s woke up." etc.).
- [ ] Party member falls ("%s has fallen...") → revive w/ Revival Drink.
- [ ] Level up → `Level-up Info` / `New Abilities`; hit level cap (before next Mark) → EXP stalls.
- [ ] Level 23 Arche → special music.
- [ ] Monster drop pickup; rare drop ([Org Sword]).

## D. Magic & party
- [ ] Switch character (Enter) — Day 3.
- [ ] Cast: select (Z / Num key / quickslot), hold X, release → spell. Interrupted cast. Shielded cast (no interrupt).
- [ ] Spellbooks A/B/C (F5); assign spells (Abilities → Use In Book A/B/C); verify AI ignores book C.
- [ ] Toggle an ability's AI-use in Abilities menu.
- [ ] Party Commands: Follow Mode ×3 (+F3 cycle), Magic Usage ×6 (+F4 cycle), Formation, Battle Order; per-character overrides.
- [ ] Stay There! puzzles: weight (Day 3), 3 buttons (Day 9 Catacombs), platforms (Archmage's Tower).
- [ ] Fire Handling: burn vines, light torches (Day 6 Stella). Water: extinguish torches (Weathervane).
- [ ] Teleportation spell (Sana lv19) + blocked-area message "For some reason, you cannot teleport from here."
- [ ] Mirror Image / Force Field / Shield on Arche.
- [ ] Solo-character segments: Sana-only water rooms (Day 5), Stella solo (Day 6), Arche solo (Day 6).

## E. Menus (F1) & items
- [ ] Main Menu every top-level entry: Status (per char), Equipment (4 slots + costumes; "Cannot Equip"; confused-member block), Items (Use → "Who do you want to use this on?"; "This item cannot be used in combat."; Discard + rare-item confirm; Z re-arrange), Special Items (view a Memory Photo; Stone Tablets; crests), Abilities, Party Commands, Messages (read a day's letter), System (Give Up, Return to Title, Money Display GP/CP, quick-heal config ×3, Options).
- [ ] `Adventure Stats` screen (Max Damage, Longest Combo, Party Deaths, …).
- [ ] Quick-heal F2 out of combat (instant) & in combat (charge bar; cancel by pressing again); F healing key; options Auto Poison Recovery / Minor Wound Recovery / Rare Item Usage.
- [ ] Recall Lamp use + blocked place.
- [ ] Bag full → "Your bags are full, so you can not take the contents of the chest."
- [ ] Money cap → "You have all the money you can carry."
- [ ] Permanent stat items (Power/Tough/Good Child/Guardian Drop, Stronggirl/Mystery Juice).

## F. Towns, shops, services
- [ ] Shop Buy / Sell (rare-item sell confirm) / "You have nothing to sell." — Tonkiness arms (Day 2+), item (Dad, Day 3+), boutique; Barness item shop; Chartreux bakery/boutique/item/candy/arms (arms closed Day 6, open Day 7, night stock Day 9); Korat souvenir/arms/item; White Rose Inn; Mountain Cat Inn (Guardpost); traveling merchant (Day 8); bonus-quest merchant; school nurse; goat girl milk.
- [ ] Dad's `Help Out` — accept, decline/give-up dialog, deliver, chain to White Sword.
- [ ] Storage chest deposit/withdraw (Arche's room; item shops).
- [ ] Inn rest (pay) / Don't Rest; save prompt; insufficient money ("Awwwww, I don't have enough money...").
- [ ] Wandering healer `Get Healed` / `No Thanks` (Day 8 road; bonus quest).
- [ ] Save statue / fountain (drink = heal + save) — first at Mountain Cave Day 4.
- [ ] Capsule machines 20 CP + 200 CP (win "yay" + "aww" outcomes; too-much-stuff refusal).
- [ ] Magic rock-paper-scissors (school kids) win 3 in a row (probe exact NPC).
- [ ] Nun donation (min 100; 100,000 → Silver Rosary).
- [ ] Fortune teller Mark check.
- [ ] Sword lessons NPC (Tonkiness weapon store, Day 2+): each topic (Swordsmanship Basics, Thrust, Wide Slash, Arcing Slash, Snap Thrust, Combos, Unblockable Attacks, Guarding, Other Techniques, All Done).
- [ ] Books: readable (Merrin), too-hard (skill books pre-clear), skill-granting (post-clear NG).
- [ ] Day/night variants (evening Day 1, night Day 9 Chartreux).

## G. Story progression gates (each = a branch to hit)
- [ ] Each Day 1–11 transition (save prompt at day end).
- [ ] Prologue clear → title → load `Prologue Clear Data` → `Quest Selection` (Main Quest vs Bonus Quest).
- [ ] Bonus quest 'The Other, Other Cave' (one-shot; loot carries back).
- [ ] Gargoyle quiz: wrong answer path (probe) + 12.
- [ ] Each boss (bosses.md) incl. Stella duel, Wyvern edge-gust.
- [ ] Archmage pot puzzle: break a block → reset; lever neaten.
- [ ] Ending + credits; `Game Clear Data` save ("Would you like to save your victory data?"; "Returning to the title screen without saving victory data" path).
- [ ] Post-game: code door (enter right AND wrong number — probe), teleport-out lever.
- [ ] NG after clear: Nightmare selectable; read the 3 skill books.

## H. Failure states
- [ ] Party wipe in field → `Try Again`.
- [ ] Party wipe in dungeon → `Return To Safety (-10% Money)` (check money −10%, location).
- [ ] `Give Up` from System in a dungeon → game over; in town → "You cannot give up here."
- [ ] `Return to Title` w/o save; re-load.
- [ ] Single member fallen while others live (AI continues).
- [ ] Drowning (Mermaid Robe expiry underwater) (probe).
- [ ] Save error paths ("Unable to save!") — host-level, optional.

## I. Bonus Menu (probe — see side-content §9)
- [ ] Bonus Menu after prologue clear / after game clear: list entries; try Arche's Wondrous Labyrinth, Arche's Arena (stage, enemy count, Instant Kill Mode), Auto-Play Mode, Versus Mode (Player vs AI, AI vs AI) — record which exist in EN-old.

## J. Collection totals (completion audit)
- [ ] 20/20 Marks of Heroism (fortune teller confirms) · 8/8 Memory Photos · 12/12 Merrin books · 3/3 Wind Crests · White Sword · Legendary Sword · Merrin's Staff · Silver Rosary · Saint's Brooch · Rapier · Celebrity Ring · Skeleton Strap.
