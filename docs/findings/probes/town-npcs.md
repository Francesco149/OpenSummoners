# probe town-npcs

run `town-npcs`; accepted 2026-11-14 12:14

check PASS (tools/drive/tasks/town-npcs.md)
verify MATCH (+ real device)
filed tests/scenarios/town-npcs/trace.jsonl (19666 frames)

## driver's report

**Run `town-npcs`**: 19,666 frames, from `end:rh-water-shrine`. `rd check` PASSES (step 1 at f1223, step 2 at f6001; the forbid never fired) and `rd verify` MATCHES (rng e8ee5d59). Gold stayed at 832 the whole run: nothing was bought and no quest was accepted.

**The save is Day 6, not Day 5, so I could not talk to anyone as Sana.** Arche wakes up alone (party=1). Sana's Mother says Sana has a fever, and an Old Man asks where the Shrine of the Wind guard went, so the shrine is already done. Enter in free roam left lid at 50010 (Arche). The task card's start note needs fixing.

**Loading the save:** after the studio intro the title cursor sat on Bonus Menu (tcur=2), not Continue. `U` then `ent`, then `ent` on the load screen loads the newest slot (slot 3, Town of Tonkiness). It opens with a wake-up scene and the player has control at f1222.

**Who and what I talked to (talk frame = the frame the conversation started):**
| room | lx | who | first line | frame | notes |
|---|---|---|---|---|---|
| 210140 bedroom | 64024 | bunny (X examine) | "Arche's precious bunny, Chocolat." | 1226 | |
| 210140 2F | 62560 | Arche's Mother | Arche: "Mom, I'm off to school!" → "Have fun, dear, and mind the slimes!" | 1686 / 1776 / 18566 | portrait (picture f1870); same line all 3 times |
| 210140 1F | 54000 | Arche's Father | "Dad, I'm off!" → "Be careful, Arche!" | 2214 | the 2nd talk (f17707) CHANGED: "...buy it with your allowance" → Buy/Sell/Help Out, Buy list shown (f17811), backed out |
| 210140 1F | 25552 | Capsule Machine (V) | "Yay, a Capsule Machine!...20CP... Should I try it?" | 2622 | Yes/No choice (picture f2770), picked No |
| 210180 (house left of home) | 49200 | Old Lady | "Dearie me, it's so difficult to get out of the house at my age..." | 3873 / 3927 | same twice |
| 210110 town | 160664 | Little Boy | "Mister butterfly, waaaaait!" | 4764 / 4958 | same twice |
| 210110 | 160664 | Woman | "The North Gate, to the left, leads to Chartreux, but something damaged the bridge!" | 4880 | |
| 210110 | 193240 | Old Man + Old Lady | "Pardon me, madame... rendez-vous at Loch Carudale?" / "Hmm..." | 5290 / 5370 | 2-speaker, same twice |
| 210110 | 244356 | Old Man | party magic-order tutorial (inline key icon) | 5870 | |
| 210110 | 223252 | Girl | "There's been a lot of monsters on the way to school lately." | 6141 | |
| 210170 boutique | 21152 | Old Lady | "Hmm, do these clothes have the wrong tag?" | 6569 | |
| 210170 | 50584 | Woman (clerk) | "Welcome! This is a clothing store..." | 6837 | Buy/Sell; Buy list (picture f7050), not bought |
| 210110 | 129368 | Man | "There've been so many monsters in the wild lately..." | 7791 / 7897 | |
| 210110 | 129936 | Little Girl | "When I grow up, I'm gonna go to the Minced Rats Magic School..." | 7995 | |
| 210110 | 107776 | Dog | Arche: "You wanna come with us, Mr. Puppy?" → "Woof, woof!" | 8203 / 10491 | |
| 210160 weapon store | 44136 | Swordswoman | "What would you like to learn about?" | 8634 | 3-way choice (picture f8932); I heard "Unblockable Attacks", then picked "All Done" |
| 210160 | 60160 | Weapon Shop Owner | "Welcome! I make and sell weapons here..." | 9064 | Buy/Sell, cancelled |
| 210110 | 17808 | Guard (North Gate) | "Yet another boring day at the gate..." | 9952 | |
| 210110 | 88216 | Woman (by the inn) | "The old headmaster of Minasa-Ratis was a terribly frightening man." | 10668 | |
| 210150 inn 1F | 34368 | Old Man | "...an elemental stone slumbered in the cave behind the school." | 11026 | |
| 210150 | 65128 | Innkeeper | "Welcome to our inn, dear..." | 11380 | Rest / Don't Rest / Save menu (picture f11596), picked Don't Rest |
| 210150 2F | 28432 | Female Mage | "...press the number keys on your keyboard to select spells." | 12489 | |
| 210130 residential street | 55264 | Sana's Mother | "Good morning, Arche!" → Sana has a fever | 14189 / 14542 | story scene with portraits (picture f14420), identical on repeat; ↑ at her door (f15420) only gives "Hello, Arche!" and does not open the house |
| 210130 | 19272 | Old Man | "Huh, wonder where the guard who was at the Shrine of the Wind got off to?..." | 15018 | |
| 210140 bedroom desk | 49492 | book (examine) | "'Book of Sword Secrets'. Would you like to read it?" | 19274 | Yes/No → too complex |
| 210140 bedroom shelf | 45844 | book (examine) | "'Magical Girl Merrin Part 1/12'" | 19446 | book reader, 2 pages (picture f19612) |

Other rooms visited: 210120 (house front), and 210200 (the first house in 210130), which had no NPCs. The "Liens" house sign, the scarecrow, the poster in the shop and the painting in 210200 do not respond to V. I did not find an item store, and I did not open the upstairs back rooms of the boutique or the inn.

**Game facts [RD]:**
- **Rooms and doors:** 210180 = the house left of home (door ↑ at lx ≈ 70400 in 210120). 210160 = weapon store (door at lx 114800 in 210110). 210150 = inn (door at 79000). 210170 = boutique (door at 258500). 210130 = residential street, reached by ↑ under the vine arch at 210110 lx ≈ 216400. 210200 = house off 210130 (door at ≈ 137000). 210110 → 210120 is the arch at lx ≈ 152000.
- **The shrine path is a walk-in exit:** the left edge of 210130 (lx < ~5000) is a kind-1 zone to 210210. It fires a few frames after you stop inside it, even when you then press right.
- **Talking:** V talks as well as examines. It works only from standing still, facing the NPC, at the right distance; X often did nothing.
- **Floors in 210140:** 52000 / 32800 / 13600; the inn 2F is at 32800.
- **Shops:** in the Buy list `C` backs out to the Buy/Sell choice. That choice appears only after `ent` advances the "anything else?" line; then `bk` closes the shop.
- **Choice menus take `ent` at once:** tapping `ent` through a conversation spent 20 CP on the capsule machine once; I rewound it.
- **Dad's line changes during the day:** in the morning he only says goodbye; later he runs his shop (Buy/Sell/Help Out). No one else changed their line when talked to again.

**Tool gaps:**
- `dlg` stayed 0 even while dialogue boxes were open, so I used `script`.
- There is no state variable for "a choice menu is open"; one would make `tap ent` safe.
- `rd text --text` dumps every partial character pair of type-on lines (title, load screen, book reader).
- I found NPCs from pictures because there is no NPC/actor position list.
- Dash momentum makes short presses misleading (`R*12` reduced lx).
- The best way to talk was `rd search --goal "script == 1"` with actions `L*3 .*6`, `R*3 .*6`, `V .*12`.

---
model claude-opus-5-5, effort high, 280 turns, $5.28, 55 min

