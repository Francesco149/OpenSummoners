# probe story-barness-hills

run `story-bh`; accepted 2026-11-14 07:02

check PASS (tools/drive/tasks/story-barness-hills.md)
verify MATCH (+ real device)
filed tests/scenarios/story-barness-hills/trace.jsonl (27157 frames)

## driver's report

## story-bh: Barness Day 6, from the inn to the Barness Hills field (280110). The dungeon was not reached.

**Result:** `rd check` PASSES for tools/drive/tasks/story-barness-hills.md (step 1 at frame 764, step 2 at frame 3001, the forbid never fired). `rd verify` says MATCH: frame 27157, rng 3130683c. The live session had drifted by one volatile dword at f27157 (evidence is in runs/story-bh/verify-drift-f27157); the clean replay matches. The movie is **27157 frames** long and ends in room 280110 at lx 0, ly 42400, with HP 117/172 and 799 CP.

I stopped at about 27k frames, far short of 150k, because the API budget was running out. I was blocked in 280110: after about 12 minutes of `rd search` across four searches, no cave entrance in that field was found.

**Route reached (frames):**
- **f764, loaded:** I chose Continue and slot 7 ("Barness Village, Arche LV5, 3:43"). That puts you in room 270200, the "Weathervane Inn" in Barness. Arche is alone: 172/172 HP, 27/27 MP, 893 CP, no healing items.
- **Messages:** the newest is No.10 "The Mayor Says…", Day 6. The mayor says the tower crest key was stolen and another crest is "in the caves near Barness". So walkthrough step 5 was already done; I was at step 6.
- **f14100–14628:** stayed at the inn (6 CP), which heals fully, and saved there (it went to slot 7). End-save digest **85acd213a421** (frame 14628): `rd new NEXT --from save:85acd213a421`. I passed no goddess statue.
- **f22810:** first game over, coverage seen once on purpose. Mothbees kept putting Arche to sleep in 260160 and she had no herbs. "Return To Safety" put her back in **270110 at lx 377600 (the Barness east gate), not at the last field entry**, with full HP and 887→799 CP.
- **About f24600:** found the 🔑 "street with a guard". It is the inn street **270120**: a vine archway at lx ≈ 95000 with a soldier beside it. ↑ there leads to **280110** (Barness Hills field), where Arche arrives at lx 268800.
- **f24734–25371:** new scene in 280110. Chiffon says "This pwace seems awfully... familiar", then Arche: "the second crest is somewhere in here". Close it with `ent`; the box text only shows up after a press.
- **f26497–27157:** walked west through 280110, past Cocorats and Mothbees, to its west edge (lx 0, ly 42400). ↑ at that edge does nothing.

**Where it is stuck:** the walkthrough's "1st cave / 2nd cave / end cave" doors in 280110 were not found. I can see dark cave arches in the pictures (strip at f25400–27060), but searches with ↑ in their actions never left the room. My guess, not confirmed: room exits are locked while an enemy is engaged (the HUD's "ESCAPE" icon shows crossed out), and the Mothbees in the field keep that lock on. The same thing blocked the 260170→260160 exit until a Mothbee left. The next probe should start from my end state or checkpoint `hills` (f25371), clear the nearby enemies, then press ↑ at the arches.

**Named state** (defined in this run's vars):
- `gold` = `s32(s32(world+0x2784)+0x4014)`. It matched the F1 panel (893 CP) and followed the inn fee (887) and the game-over loss (799).
- `hp` / `mp` = `s32(lslot+0xa64)` / `s32(lslot+0xa6c)`. They matched the F1 panel (172/172, 27/27), dropped with damage, and refilled at the inn. `lslot+0xa68` reads 168, which is base max HP; the shown max adds the equipment bonus.
- `party` = a bitmask over the four party slots `s32(s32(world+0x2784)+0x4030+4*i)+0x9c4 == 1`. It reads 1 (Arche only). Sana's slot reads **2**, not 0, probably meaning "sick / away".
- **Day counter: not found.** I had no day change to `ram diff` against. A candidate from the decompile: story flag `100100101` (the game checks the values 1110 / 1120 / 1140). Flags are 12-byte {id, value, ?} records at `s32(rmo+0x40ac)+0x1804`, with the count at `+0x7804`, where `rmo` = `s32(world+0x2784)`. That is not validated, and the expression language cannot loop over the records.

**Game facts and walkthrough corrections (EN-old):**
- Barness room ids: 270110 is the east street with the crops (the far-left windmill end has no exit), 270120 is the inn street, 270200 is the inn interior. 260170 and 260160 are Barness road, and 260160 has the cave the guide says to ignore. Signs in the decompile: 260150 is the junction (↑ Barness, → Tonkiness, ← Weathervane Tower & Chartreux), 260130 is the bridge guard's room, 260210 is the Weathervane Tower door ("The door is closed tightly"), 260230 is Loch Carudale.
- **Step 6 "Street w/ guard → left" is wrong for EN-old.** The guard stands at an archway in the inn street 270120, at lx ≈ 95000, and you press ↑ under the arch. You don't go left.
- The cave left of the inn (270120, lx ≈ 14000, an old man beside it) leads to 280115. That cave is gated on its west side (≈ lx 254000, a diamond sign) and its east side. From the decompile: the 290110 diamond switch sets flag 280115501, and the 280150 switch sets 280115502 ("…sound of machinery activating").
- Area 280 is the caves/fields near Barness and area 290 is Barness Hills Dungeon. The decompile has a Sana + Arche scene keyed on room 280110, probably for a later day.
- Movement: walking builds up speed and slides. Two presses of ←/→ in a row turn into a dash (480 per frame), even with a few idle frames between them. That made my `{toward()}` positioning overshoot badly.
- Mothbees put Arche to sleep repeatedly. Solo at Lv5 with no herbs, she dies in the Barness road fields.
- The inn's item shop is in the inn: a Barness kid NPC says so.

**Tool gaps:**
- `rd search` doesn't know about the room-exit lock while enemies are engaged, so ↑-exits look unreachable. An `escape_ok` name (from the HUD state) would help.
- No way to list a room's exits or doors.
- `ram str` on D() addresses was the only way to read sign and NPC text. A helper that maps an actor to its text would save a lot.
- `eval` has no list comprehension or loop, so the flag table can't be searched.
- The `read` and `grep` tools cut long walkthrough lines (line 86 etc.) at about 250 characters.
- Menus opened during `script == 1` silently drop F1. A `menu_open` name would help.

---
model claude-opus-5-5, effort high, 272 turns, $6.21, 101 min

