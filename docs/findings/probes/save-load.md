# probe save-load

run `save-load`; accepted 2026-11-14 10:44

check PASS (tools/drive/tasks/save-load.md)
verify MATCH (+ real device)
filed tests/scenarios/save-load/trace.jsonl (7661 frames)

## driver's report

**Run `save-load`: 7661 frames. `rd check` passes, and `rd verify` matches (headless replay: same frame, rng f8fa0c1b and state hash).** Check step 1 was met at f1664 (control after the first load) and step 2 at f6001. The movie ends in the game, in the Tonkiness inn with control.

**Save digests** (from `rd endsave`)
- `save:16291857da07`, filed at f3831: the first save, into empty slot 8 (`savedata07.sdt`).
- `save:ec077c546372`, filed at f4040: after overwriting slot 8. A final `endsave` at f7661 gives the same digest, because nothing was saved after that.

**What happened, with frames (I looked at a picture for each)**
- **f576, title:** the cursor was remembered on Bonus Menu (`tcur` 2). U, then `ent` on Continue at f623.
- **f703, load screen** "Please choose a save-game to load.", page 1/3, cursor on slot 7 (the newest):
  - 1: Town of Tonkiness, Arche LV1, 0:34
  - 2: Minasa-Ratis Magic School, Arche LV1, 0:57
  - 3: Town of Tonkiness, Arche LV2 / Sana LV1, 1:26
  - 4: Prologue Clear Data, 1:44
  - 5: Mountain Cave, Arche LV3 / Sana LV3, 2:10
  - 6: Town of Tonkiness, Arche LV4 / Sana LV4, 2:48
  - 7: Town of Tonkiness, Arche LV4, 3:25
  - 8: NO DATA
  - All dates read 2012/01/27 12:xx–13:xx.
- **Paging:** R → page 2/3 at f734 (slots 9–16 NO DATA), R → page 3/3 at f765 (17–24 NO DATA), R again wraps to page 1 (f848). The cursor keeps its row across pages.
- **Empty slot (task step 5):** `ent` on slot 23 (f766) and X (f797) did nothing. No message appeared and the screen stayed.
- **f849, loaded slot 7:** waking scene in Arche's room ("Mmmmmm! That was a good sleep!" … Chiffon "Fwoo!"). Control at f1663 in room 210140, gold 832, hp 161, party 1 (Arche alone).
- **Walk to the inn:**
  - Down to the ground floor. The front door is at lx ≈ 26000 in 210140 → room 210120 at f2369. The door at lx ≈ 44800 leads to 210141.
  - From 210120 to 210110 at f2534.
  - The inn door is at lx ≈ 78500 in 210110 (↑) → inn room 210150 at f3176.
- **Innkeeper (lx ≈ 64500), f3472:**
  - "Welcome to our inn, dear. Would you like to rest?" then "I'll make it 3 copper a room, just for you."
  - Menu "Inn - 3CP to stay": Rest / Don't Rest / **Save** (saving needs no rest and costs nothing).
- **Save slot screen** "Please choose a slot to save in.", f3667. The cursor starts on the last-used slot (7).
  - **Empty slot 8:** D, `ent` → "Game saved." (f3710–3750; it waits for a key). No confirm for an empty slot. Label: "8 : Town of Tonkiness / Arche LV4  3:25  2012/01/27 12:17".
  - After the message the slot screen stays open, then returns to the inn menu.
  - **Overwrite:** Save → cursor now on slot 8 → `ent` → "Overwrite this save?" Yes / No, default **Yes** (f3979) → "Game saved." (f4040). The label is unchanged (3:25, 12:17).
  - Don't Rest → "Well, don't overdo it!" → control at f4144.
- **Return to Title, in game (task step 4):**
  - F1 Main Menu: Items / Special Items / Status / Abilities / Party Commands / Messages / System.
  - System: Options / Quick-heal / Money Display / Give Up / Return to Title. Help line: "Returns to the title screen. This option does not save the game."
  - The prompt "Are you sure you wish to return to the title screen?" has Yes / No, with the cursor on **No** (f4298).
  - **No** → back to the System list, still in the game (f4334).
  - **Yes**, picked at f4408 → `world` becomes 0 at f4410 → the studio intro replays → title menu (`tcount` 5) at f4938. That is **528 frames** from world = 0 (the first boot took 576). Later returns took 509 frames from Yes.
  - The title cursor is remembered on Continue (`tcur` 1). An `ent` 16 frames after the title came up was dropped.
- **Reload my own save (task step 2):** the load screen at f5115 has its cursor on slot 8 (the last one saved). Loaded at f5115, control at **f5176, only 61 frames**. Result: room 210150, lx 64000 (saved at 64504; snapped), ly 52000, hp 161, gold 832, party 1. That matches the save state except the x snap.
- **Older slot (task step 3):** Return to Title (f5208–5384), title at f5893, Continue → slot 5 → control at f6112.
  - Room 245165 (Mountain Cave goddess statue room), Arche Lv3 + Sana, party 3, gold 527, hp 144, lx 38400 / ly 36000.
  - I examined the statue (X at lx ≈ 53000): "Water flows from the fountain. Would you like to drink it?" Yes/No → "The cool fountain water restored your health!" → "Would you like to save?" Yes/No → the slot screen.
  - **Backspace on the slot screen cancels** and asks again. I answered No, so no save; control at f6676.
- **Back to my own slot:** Return to Title again, title at f7383. The load screen's cursor is now on **slot 5, the last loaded**. D×3 → slot 8 → control at f7601: room 210150, lx 64000, ly 52000, gold 832, hp 161, party 1. The movie ends at f7661.

**Named state**
- **Current slot:** no address found. The load and save screens' starting cursor equals the last slot loaded or saved (7 → 8 → 5 across this run), so the game keeps it, but I did not find where.
- **Play-time counter:** not found as an in-RAM counter. Over a 60-frame and a 120-frame wait, the module and room/world struct had no dword going up by 60 or by 1. The only clock-like words were host-clock words: `0x50008fd0` (+16 ms per frame; ≈ 1,000,000 + 16·frame) and `0x5002cce0` (+160,000 per frame, 100 ns units). So play time is most likely computed from a stored base plus the host tick. The labels show h:mm, and "3:25" did not change over about 40 game seconds.
- `200a3320` counts frames since entering the room (1 at room entry, +60 in 60 frames). This is a heap address and unvalidated.
- **Save timestamp:** "2012/01/27 12:17" comes from the fixed headless clock. It is earlier than the existing saves' times, so it does not advance as real time.

**Game facts [RD]**
- The inn menu's "Save" saves without resting and for free.
- Saving to an empty slot has no confirm. An occupied slot asks "Overwrite this save?" with Yes as the default.
- "Game saved." waits for a key, then the slot screen stays up until the next key.
- The Return to Title confirm defaults to No.
- Pages are 3 × 8 slots and wrap with ←/→.
- Mountain Cave room 245165 has a statue-fountain: drink → heal → save prompt → slot screen, and Backspace backs out of it.
- A load takes about 61 frames to control. A return to the title takes about 530 frames (the intro replays).

**Tool gaps**
- No RAM value search (find dword == N). This blocked naming the current slot and the play-time base.
- `rd text` returned nothing on the title and load screens and on Yes/No boxes while they were opening, and gave per-glyph fragments on the slot screen.
- I lost 3000 frames because an `ent` pressed right after a sub-menu closed was dropped. I rewound and cut them from the movie.

---
model claude-opus-5-5, effort high, 153 turns, $2.40, 27 min

