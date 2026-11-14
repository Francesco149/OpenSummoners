# Mechanics — controls, combat, magic, party, progression

Tags: see `README.md`. EN labels in `quotes` are verbatim [EXE] strings.

## 1. Controls (keyboard)

### 1a. Default keys
| key | action | src |
|---|---|---|
| ← → | move; **double-tap = dash/run** (`Dash Input Sensitivity` option sets the double-tap window) | [DK][EXE] |
| ↓ | crouch; while running → slide to stop | [DK] |
| ↑ | enter door / area exit; **guard** (when Auto-guard off — guard is ↑ either way) | [DK][EXE][REPO] |
| X | attack / select / talk / examine / **confirm** | [DK][REPO] |
| C | jump (hold = higher) / cancel / closes dialogue | [DK][REPO] |
| Z | draw/sheathe sword; for casters = spell select; in item lists = re-arrange items | [DK][EXE] |
| V | examine only (no attack) | [DK][ZO] |
| F | use healing item | [DK][ZO] |
| Enter | `Change Character/Skip Text` — switch controlled party member; advances / skips dialogue; menu confirm | [DK][EXE][REPO] |
| Esc / Backspace | cancel (fixed scancodes) | [REPO input.md] |
| F1 | Main Menu (status/equipment/items/…) | [DK][EXE "accessed with F1"] |
| F2 | quick-heal (item on member w/ lowest % HP) | [DK][EXE] |
| F3 | cycle Follow Mode (Follow Me! / Stay There! / Fight Freely!) — HUD icon bottom-left; anchor = stay | [DK] |
| F4 | cycle party Magic Usage (full moon=all-out, half=moderate, waning=little, X'd moon=none, medpack=heal only) | [DK] |
| F5 | change active spellbook (A/B/C) | [DK][EXE] |
| F6 | change camera mode (anchor=centred, red lock-on=target enemy, long=lead ahead) | [DK] |
| Num0–9 | `Select Spell 0–9` (direct spell select) | [ZO][EXE] |
| PgUp/PgDn/Home/End | Move POV up/down/left/right | [ZO][EXE] |

- **CONFLICT (resolved):** [ZO] lists Change Character/Open Menu/Follow/Magic-use/Scroll mode as "None"
  and Change Spellbook = NumEnter. Explanation: the keybind screen only accepts **A–Z, Space, Numpad**
  ([EXE] "Selectable keys are A-Z, Space and Numpad keys") — Enter/F-keys are FIXED hardwired bindings
  ([REPO] input.md: RETURN fixed → confirm), the rebindable slots for those actions default empty. Both
  sets work. [DK]'s table = what a default install responds to.
- Options → Keyboard Setup: `Jump, Attack, and Magic Select are not bound.` = error if those cleared [EXE].
- [REPO] verified (rhost, EN-old): X talks/confirms; C jumps + cancels dialogue; ↑ enters doors; Enter
  confirms + advances dialogue but does NOT talk; Z does nothing near an NPC.

### 1b. Command notation (in-game Abilities descriptions) [EXE tutorial]
- `A+B` = press together. `A-B` = in sequence, fast. Descriptions assume facing RIGHT; facing left swap ←/→.
- Combo: land ≥5 hits in quick succession → combo meter; damage scales w/ combo length; any party member
  hitting anything keeps it alive [EXE].

## 2. Arche's sword techniques ([DK] list, [ZO][EXE] confirm; level = Arche's Combat Level)
| lv | move | input (facing right) | note |
|---|---|---|---|
| 1 | Slash | X | |
| 1 | Thrust | ↓+X | faster than slash |
| 1 | Roll | ↓ then → | moves through enemies, i-frames [MA] |
| 1 | Cartwheel | ↓ then ← | backwards evade, i-frames [MA] |
| 1 | Wide Slash | →+X | steps forward, long recovery |
| 1 | Snap Thrust | ←+X | instant turn + thrust behind |
| 1 | Jumpkick | → then ↑ then X | works unarmed; "best move" [MA] |
| 1 | Arcing Slash | ↑+X | anti-air; extra dmg to airborne (last anim cycle) [EG] |
| 2 | Downstab | while falling ↓+X | guard-break; breaks crates from above |
| 3 | Combo Slash | after Slash, ↑+X | |
| 5 | Wheel Slash | → then ↓ then X | rolls through enemies |
| 7 | Combo Wide Slash | after Slash, →+X | |
| 9 | Snap Slash | ← then X | |
| 10 | Knight's Thrust | while running ↓+X | pierces guards/multiple |
| 12 | Leaping Slash | ↑ then → then X | unblockable |
| 14 | Twirling Slash | ← then ↓ then X | hits behind |
| 17 | The Full Moon | while jumping ↑+X | huge air spin; best late move [MA][EG] |
| 20 | Buster Tornado (3 MP) | spell, during Unison | |
| story | Unison (10 MP) | spell; after seal broken (Day 11) | **triple jump** (wind boost jump; "boost TWICE in the air") — needed for many late chests |
| story | Wind Thrust | during Unison | ranged sword blades |
- Learned automatically on level-up (`New Abilities` in Level-up Info) [EXE]. Books (NG+) can grant early (side-content.md).
- [RD combat-moves + combat-party, EN-old] `lpose` ids, each on an enemy with a picture: Slash 10101 (X; also the jump attack:
  X in the air), Thrust 10102 (↓+X), Downstab 10103 (↓+X falling), Arcing Slash 10104 (↑+X), Snap Thrust 10105 (←+X), Wide
  Slash 10109 (→+X), **Jumpkick 10110** (`R U X`, fresh 1-frame presses; also unarmed), Combo Slash 10111 (`X .*10 UX`),
  Roll / Cartwheel 10108 (`D*3 DR*6`, ↓ then toward / away). In-game command strings: Cartwheel "↓ ▶ ←", Jumpkick "→ ▶ ↑ ▶ X".
  Wheel Slash needs Combat Level 5 (Day 5 save: 3/5). An attack key pressed during recovery is dropped; jump needs a fresh
  press; a hit stuns input ~30 frames. Arche's move ids: 10101–10118, 10120, 10122. Sana: Diamond Dust 20101 (`n1`, hold X
  ~130 frames, release), Heal Water 20102 (`n2`).

## 3. Combat rules
- Don't mash: slow recovery; enemies dodge/guard/counter. Guard then punish [DK][ZO][EXE].
- **Auto-guard** (new-game option + Options) blocks automatically when idle; never while attacking/jumping [EXE]. Recommended ON [DK]; [ZO] calls it unreliable.
- [RD combat-party] Mothbee Sleep passes ↑ guard; lasts ~280 frames unless a hit wakes ("Arche has fallen asleep." / "Arche woke up."); both members can sleep at once. F2 in combat = "Quick-heal" bar then the herb bar (~250 frames, not cancelled by a hit); F / F2 do nothing without a healing item.
- Guard can't stop: magic, huge-weapon hits, big-monster body hits, jump attacks (unblockable → bigger damage numbers) [EXE][MA]. Shield-bearing enemies guard anything frontal [EXE].
- Taking too many hits (blocked or not) in a short time → **disarmed + knocked down** [MA]; Jumpkick still works unarmed.
- Interrupt casters by hitting them while the magic circle forms (applies to all enemy mages) [EG][ZO].
- Pits in fields: fall = −5 HP and respawn [DK]. [RD combat-moves: −8 HP at Barness road 260170 (Lv5, Day 6), back at the last ledge.]
- Harpies grab + drop you to earlier screens (Weathervane Tower) [EG][DK].

## 4. Magic
- Elements: wind (Arche/Chiffon, 'Silpheed'), water (Sana, 'Eolamest'), fire (Stella, 'Royal Scarlet'), earth (classmates). Triangle (magic janken): wind>water>earth>fire>wind [JW]. Elemental weakness damage modifiers: UNVERIFIED (no source).
- **Casting** [EXE tutorial]: Z (or Num0–9, or quickslots) selects spell → **hold X** → magic circle forms → **release X** when complete → casts. Getting hit during the circle interrupts (Shield prevents interruption).
- Option `Magic Quick Select`: `Magic Book Select Mode` vs `Direct Magic Select Mode`; Z+dir assigns quickslots 1–6 [EXE].
- **Spellbooks A/B/C** (F5 switches): Abilities menu → `Use In Book A/B/C`; max N spells per book; **AI never uses spells in book C** [EXE]. Abilities menu also toggles each ability used/not-used by AI.
- Sana/Stella non-spell attacks: X=melee; ↓+X / →+X / ↑+X = short / long / air elemental shot (Water Shoot / Fire Handling) [DK]. Fire Handling burns dead vines + lights torches (puzzles). Sana swims (water areas, levers underwater); Mermaid Robe (lv6) lets whole party breathe underwater.
- Sana spells (lv: name MP): 1 Diamond Dust 3 (can freeze) · 1 Heal Water 4 (60–100) · 3 Shield 2 (½ dmg, no interrupt, 1 target) · 4 Sleep Mist 3 · 5 Splash 2 · 6 Mermaid Robe 2 · 7 Turbid Squall 3 (−DEF) · 8 Leap Back (move) · 9 Heal Water II 6 (180–240) · 11 Force Field 5 (+ATK, 1 target) · 13 Antidote 4 · 14 Diamond Dust II 6 · 15 Shield II 5 · 17 Mirror Image 5 (2 copies replicate physical attacks) · 19 Teleportation 6 (party → last safe place; blocked in some areas) · 21 Water Blow 8 · 23 Heal Mist 12 (party 180–240) [DK]. [EXE] also has `Revival` (revive w/ 180–240 HP) — learn level UNVERIFIED (not in [DK] list).
- Stella (joins at lv3): 3 Stab · 3 Fire Handling · 3 Leap Back · 3 Fire Wall 3 · 4 Chaser 4 (homing) · 7 Phoenix 4 · 9 Disintegrate 3 (fails vs stronger targets) · 11 Blade 3 · 12 Inferno 5 · 14 Life Force 4 (60–100 heal) · 16 Interceptor 5 (anti-air) · 17 Fire Wall II 6 · 19 Phoenix II 7 · 20 Chaser II 8 · 21 Explosion 9 · 23 Cerberus 11 (HP ∝ Spirit) [DK].
- "Mirror Moon" late-game cheese: Sana casts Shield + Mirror Image + Force Field on Arche → spam Full Moon [MA].

## 5. Party & AI
- Members: **Arche** (sword; Atk A/Def B/Spi F/Res D [ZO]), **Sana** (water healer, joins Day 2 walk/Day 3 playable), **Stella** (fire mage; playable solo Day 6 interlude, permanent from Day 8 Org Lair onward). Chiffon (wind spirit, Day 4+) = companion, wears costumes, not a fighter [DK].
- Switch control: Enter [DK]. AI drives the others ("FOR-D System" [JW]).
- Main Menu → `Party Commands` [EXE]: `Follow Mode` ('Follow Me!' / 'Stay There!' / 'Fight Freely!'; per-character "match party" option), `Magic Usage` (Cast like mad / Normal use / Be conservative / Heal only / Don't use magic / Match party), `Formation`, `Battle Order`.
- **Stay There!** = puzzle tool: park members on pressure plates/weights/platforms (F3 twice from follow) [DK][EXE].
- Only Arche pushes blocks/crates/statues/boats (sword sheathed: Z) [DK][ZO]; Stella cannot push [DK].
- Members left behind reappear when you change rooms [DK].

## 6. Stats, levels, Marks of Heroism
- Stats: HP, MP, Attack, Defense, Spirit (magic dmg), Resist (magic def + ailment resist) [DK][ZO].
- Two levels: `Combat Level` + `Adventurer Level` [EXE][JW]. Level-ups: random stat gains [ZO] (save before levelling if min-maxing).
- **Level cap**: starts 3; each **Mark of Heroism** +1 cap and raises money cap; 20 Marks → max level 23, max 140,000 CP [DK][ZO]. EXP gain falls near the cap [ZO]. EXP: lv2 250, lv3 500, lv23 132,000 [DK]. Lv23 Arche → special music [DK]. Table: `side-content.md` §1.
- Permanent stat items: [Power Drop] Atk+1–2, [Tough Drop] Def+1–2, [Good Child Drop] Spi+1–2, [Guardian Drop] Res+1–2, [Stronggirl Juice] MaxHP+2–4, [Mystery Juice] MaxMP+2–4 [DK]. Power→Arche, Good Child→Sana/Stella [DK].
- Difficulty: `1:Easy 2:Normal 3:Hard 4:Expert 5:Nightmare` [EXE]; Nightmare after one clear [DK][GF]; change any time (System/Options), takes effect on next area change [EXE]; harder = stronger enemies + more EXP [EXE][DK]. New-game default `1:Easy` [REPO].
- `Adventure Stats` screen: Max Damage Dealt, Longest Combo, Total Party Deaths, EXP/Money Gained, Monsters Defeated, Marks Gained [EXE].

## 7. Status effects [DK][EG][EXE]
| status | effect | cure |
|---|---|---|
| Poison | DoT; persists | [Venom-Away]/[Venom-Away S], Sana Antidote (lv13), quick-heal w/ `Auto Poison Recovery`; [Anti-Poison Ring] |
| Sleep | immobile long; hits shorten | ends if causer dies; [Sleepless Ring] |
| Confused | attacks friend+foe | timeout or causer dies (Harpy song) |
| Frozen | immobile; hit ends it | — |
| Paralyzed | can't move (Red Slime, Rattlesnake) | timeout |
| DEF down | Turbid Squall etc. | timeout |
| ATK up | Force Field | — |
| Fallen | KO'd member | [Revival Drink]/[Revival Drink EX], Revival spell |
- Status prevention items exist except vs freeze [DK].

## 8. Items, inventory, equipment
- **Items never stack** (each copy = one slot) [DK]. Bag full → can't take chest contents [EXE].
- **Storage chest** (128 items): Arche's room; every item shop; Org-Lair merchant [DK]. North Chartreux Guardpost inn has none.
- Equipment slots [EXE]: `Weapon`, `Armor #1` (body), `Armor #2` (gloves/capes/sash), `Accessory`; plus **costumes** (colour/vanity: dresses, hairbands, smocks, berets; Chiffon can wear Green Clothes/Hat). Weapons per character: swords=Arche, staves (+[Wooden Sword])=Sana, daggers/[Rapier]=Stella [DK]. Confused member: can't change equipment [EXE].
- Key/special items: `Special Items` menu (crests, stones, keys, Memory Photos — viewable) [EXE][DK].
- Healing consumables (HP): Melshee Choc 25–30, Mentama Drop 40–50, Taiyaki 50–60, Herb 70–100, Slime Bean 90–120, Fresh Milk 90–110, Walnut Choc 120–170, Better Herb 160–200, Handmade Tart 180–240, Honey 180–240, Hot Springs Bun 200–250, Best Herb 320–400; party: Holy Water 140–160, Life Dew 160–200; MP: Merrin Gum 5–10, Magic Candy 50–70, Mind Chocolate 120–140; revive: Revival Drink 120–160 HP, EX 240–320 [DK].
- [Recall Lamp]: instant return to last safe place; single use; blocked in some places [DK].
- Quick-heal (F2) options (System menu): `Auto Poison Recovery`, `Minor Wound Recovery` (heal members ≥75% HP?), `Rare Item Usage` (use monster-drop items like Honey/cake) [EXE][MA]. In battle quick-heal charges a bar then heals; pressing again cancels; out of battle instant [MA].
- Sell: rare items get an extra confirm; discard likewise [EXE].

## 9. Money
- Coins found as Copper/Silver/Gold; 1 GP = 100 CP; display GP or CP [EXE][DK]. Cap rises w/ Marks (1000 → 140,000) [ZO]. At cap: "You have all the money you can carry." [EXE].

## 10. Saving, healing, death
- Save: inns (pay to rest: Tonkiness 3 CP, Barness 6, Chartreux 8), school nurse (free rest + save), dungeon **statues/fountains** (drink = heal + save), wandering healer in fields (`Healer - %s`, `Get Healed`), plus story prompts at day ends [DK][ZO][EXE].
- Slots: `savedata%02d.sdt` (+.bak/.dat) [EXE]; format → `docs/findings/sdt-save-format.md`. Special saves: `Prologue Clear Data` (after Day 4 boss; game returns to title, load it to continue) and `Game Clear Data` (post-ending) [EXE][DK].
- **Game over** (all fallen) → choice `Try Again` / `Return To Safety (-10% Money)` [EXE]. Dungeon death w/ Return To Safety = redo dungeon [DK] (UNVERIFIED exact Try Again semantics: likely retry from area entry / last checkpoint).
- `Give Up` (System menu): instant game over → return to last safe place; disallowed in towns ("You cannot give up here.") [EXE].
- `Return to Title` (System menu): no save [EXE].

## 11. Menus (F1 Main Menu) [EXE]
`Main Menu` → `Status` (per char: stats, `Equipment`, `Abilities`), items (use/`Discard`/`Remove`), `Special Items`, `Abilities` (spellbook assignment, AI toggles), `Party Commands`, `Messages` (dated hint letters per Day — "what to do next"), `System` (`Give Up`, `Return to Title`, `Money Display`, quick-heal config, `Options`). Exact top-level order UNVERIFIED — probe in rhost.
- Options (title + in-game): `Keyboard Setup`, `Gamepad Setup`, `Analog Gamepad Setup`, `Sound Settings` (music/sound/voice/message volume), `Graphics Settings` (`Graphics Quality`, `Brightness`, `Contrast`, `Display FPS`), `Game & Control Setup` (`Game Difficulty`, `Auto-guard`, `Dash Input Sensitivity`, `Message Speed`, `Magic Quick Select`, `Display Combo Meter`, `Magic Circle Display`); buttons `Save Changes & Exit` / `Reset to Defaults` / `Exit Without Saving` [EXE].
- Title: New Game / Load / Options / Bonus Menu / Exit (order per `docs/findings/engine-quirks.md` #27) [REPO]. New Game → config box `Game Difficulty` / `Auto-guard` / `Start Game` [REPO new-game-flow.md].
- Shop UI: Buy / Sell / (Dad only) `Help Out` [DK][EXE].
- In-world: inn rest prompt `Inn - %s to stay` / `Don't Rest`; healer `Get Healed` / `No Thanks` [EXE].

- **[RD] verified in rhost (probe `title-options-bonus`, 2026-11-13) — supersedes the guide lines above where they differ:**
  - Title: **Start / Continue / Bonus Menu / Options / Exit** (not New Game / Load). Continue is enabled with no save:
    load screen "Please choose a save-game to load.", 3 pages × 8 slots, all "N : NO DATA".
  - Options list: Game & Control Setup, Graphics Settings, Sound Settings, Keyboard Setup, Gamepad Setup, Analog Gamepad
    Setup (both greyed without a pad), Back. Page buttons: **Default Settings / Cancel Changes / Save and Exit**.
  - Game & Control: Game Difficulty (1 Easy .. 4 Expert on a fresh install — no Nightmare), Auto-guard, Magic Quick
    Select (Magic Book Select Mode / Direct Magic Select Mode / Don't Use), Dash Input Sensitivity (1 Short .. 5 Long,
    default 4).
  - Graphics ("Options: Screen Settings"): Graphics Quality (High/Normal/Low), Contrast + Brightness (0..10, def 7),
    **Message Speed** (1..5, def 3), **Display Combo Meter** (Yes), **Magic Circle Display** (No), Display FPS (No); no
    Save row — `C` saves and exits without a prompt (so does C on Sound).
  - Sound: Music / Sound / Voice / Message volume (def 3 / 5 / 3 / 3; Voice + Message have Off).
  - Keyboard: 3 pages (Left/Right); pressing a row's own key unbinds it ("----").
  - Bonus Menu opens on a fresh install with **Versus Mode, Auto-Play Mode, Back** only (no Labyrinth, no Arena in this
    edition); both modes: "You cannot access this menu yet."
  - Saving options draws the RNG (0x8a4f94).

## 12. Environment / puzzle vocabulary [DK]
Push blocks/crates/gargoyle statues/gravestones onto **blue floor buttons**; **red buttons** drain water / hold doors (need a body: park a member); levers marked with symbols (sun, moon, heart, diamond, clover, scales, crescent, star) open walls with the same symbol; weights need party members as counterweight; falling platforms; spike traps (some ride up as lifts); **trap chests** (poison gas, spike, teleport) — walkthrough flags each; steam vents (Korat) burn; hot water burns; moving stone blocks; boats (Sana fetches, Arche pushes). Leave + re-enter a room resets most puzzles; breaking a block resets the Archmage pot puzzle.
