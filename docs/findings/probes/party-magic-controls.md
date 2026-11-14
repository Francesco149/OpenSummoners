# probe party-magic-controls

run `party-magic-controls`; accepted 2026-11-14 01:06

check PASS (tools/drive/tasks/party-magic-controls.md)
verify MATCH (+ real device)
filed tests/scenarios/party-magic-controls/trace.jsonl (6563 frames)

## driver's report

## party-magic-controls: report

**Run:** `party-magic-controls`, 6563 frames. `rd check` gives **PASS** (step 1 at f902, step 2 at f4001). `rd verify` gives **MISMATCH**, so the trace is not verified. Frame and RNG are the same in all three runs (live session, headless replay, clean session rebuild: frame 6563, rng a5c4bb5b). The live session and the clean rebuild also have the same state hash (semv 7bf2bbaa926f8edc). Only the separate headless replay differs (semv 22696465feff51ac). The evidence diff compares frame 6562 with 6563 and shows just one changing ("volatile") dword, so it is off by one frame and does not show the real difference. Possible cause: the Options "Save and Exit" writes the config file to disk mid-run, and later session rebuilds boot with that file. See tool gaps.

**Start and load:** booted to the title at f576. Continue (D, ent) at f600, then picked slot 6 "Town of Tonkiness, Arche LV4 / Sana LV4, 2:48, 13:21", the newest save. Slots 9–16 on page 2 are empty. I had control at f901 in room 210150, the Tonkiness Inn interior. Messages show #08 "Day 5, From the Weapon Shop", which puts us at walkthrough Day 5 step 1–4. The party is Arche and Sana, with Chiffon as a companion.

### Steps (frames, all looked at)
- **Switch character (Enter):** Sana at f1213 (lid 50011), Arche at f2574 (50010), Sana again at ~f4054, Arche at ~f4511. Both members done.
- **F3 Follow Mode** (`follow`): 0 → 2 at f972 (flame icon, Fight Freely), → 1 at f1003 (anchor icon, Stay There!), → 0 at f1024. All three done.
- **F4 Magic Usage** (`magicuse`): went 3 → 4 → 0 → 1 → 2 → 3 → 4 → 0 between f1044 and f1191. At 0 the icon is the X'd moon, at 1 it is the medpack (looked at f1212 and f1107). Set back to 3 at f6547.
- **F5 spellbook** (`book`):
  - At f1191 and f1243 F5 did nothing, because books B and C were empty.
  - In F1 → Abilities (Sana) I moved Sleep Mist to Book B and Shield to Book C (slot 1), f1652–2111.
  - After that: 1 → 2 at f2195, → 3 at f2221, → 1 at f2247. The top-right spell icons change with the book.
- **F6 camera** (`camera`): 0 → 1 at f1270 (the "LOCK" icon), → 2 at f1296, → 0 at f1322, → 1 at f1348.
- **Sana casts:**
  - **Completed:** Z selected Heal Water, X was held f2293–2412, the circle formed, and the cast went off on release. MP went 88 → 84 by f2452 (strip 2380–2470).
  - **Interrupted:** I released X early, after 40 frames (f2473–2513). The circle fades and MP stays at 84 (looks at 2513 / 2525 / 2545).
  - Sana's AI also cast Diamond Dust in the Mothbee fight (MP 84 → 81, f5438).
  - Not in the final movie: an interruption by a hit, and numpad spell select. On a branch I later cut, `n1` selected Diamond Dust, and Sana's charge was broken by a Mothbee's sleep (she fell asleep, MP unchanged).
  - Arche has no spells yet, and Stella is not in the party.
- **Dash (double tap):** about 432–480 per frame (walking is 240); seen in town f2740+ and in the field at f3356.
- **Crouch:** `stance` = 10 (f3281–3306).
- **Slide:** a running D at f3383; x keeps falling and slows down with stance 10 (looked at f3392).
- **Guard ↑:** stance 11, first with Auto-guard On (f3321) and again with Auto-guard Off (f5973–5993).
- **Auto-guard setting:** F1 → System → Options → Game & Control Setup. On the Auto-guard row X opens an Off/On box, and Save and Exit stores it. Set to Off and saved by f5860 (re-opening at f6204 still showed Off). Set back On around f6250 (the page shows "On") and saved.
- **F2 quick-heal, out of combat:** at f4461 Arche went 158 → 161 instantly. Not done in combat: no fight was near at that point.
- **Combo:** in room 260120 (two Mothbees), `rd search` from f5310 reached `combo` 5 at f5438 and 7 at f5468. The HUD showed "7 Hit +5%" and both bees died (Exp +19 each).

**End state (f6563):** room 260120, Arche leading, `follow` 0, `magicuse` 3, `book` 1 (A), `camera` 1. Arche 153/161 HP, 27/27 MP; Sana 100/100 HP, 81/88 MP. The F1 status panel showed the same HP/MP at f5524.

### Named state (all checked against pictures)
`room` below means `s32(world+0x2784)`; a party slot is `room+0x4030 + 4*i` (slot 0 = Arche, slot 1 = Sana).

| name | expression | values / how checked |
|---|---|---|
| `follow` | `room+0x4050` | 0 Follow Me, 1 Stay There!, 2 Fight Freely; HUD F3 icon |
| `magicuse` | `room+0x4054` | 0–4, F4 adds 1 and wraps; 0 = no magic, 1 = heal only |
| `camera` | `room+0x4058` | 0 / 1 / 2; 1 shows "LOCK" |
| `combo` | `room+0x40e0` | hit count; checked against the HUD "7 Hit" |
| (combo timer, not defined) | `room+0x40e4` | set to about 800 on a hit, falls by 16 per frame, so about 50 frames between hits |
| `lid` | `s32(lslot+4)` | leader id: 50010 Arche, 50011 Sana |
| `p0hp` / `p0mp`, `p1hp` / `p1mp` | `slot+0xa64` / `slot+0xa6c` | name at slot+0xa10; max HP +0xa68, max MP +0xa70; matched the F1 status panel |
| `book` | `s32(lslot+0xb50)` | leader's spellbook, 1/2/3 = A/B/C |
| `stance` | `s32(s32(lslot+0x9f4)+0x14860)` | 0 idle, 10 crouch / slide, 11 guard |

### Game facts and corrections to docs/game for EN-old
- F3 goes Follow → Fight Freely → Stay → Follow. The values are 0 → 2 → 1, so "F3 twice from follow = Stay" is right.
- F4 cycles **5** values, not 6. "Match party" exists only per character in the menu.
- F5 does nothing while books B and C are empty. You fill them in Abilities by picking a book, then a numbered slot (10 per book).
- Auto-guard is changed with an X submenu; ←/→ does nothing on that row.
- Town and field left exits are ↑ at the edge, where a red arrow marker shows; you cannot just walk off the edge. Room 260110 is the field left of town, 260120 the one after it (Mothbees).
- Mothbees put both party members to Sleep, which also stops casting.
- Arche's X barely hurts Green Slimes with the sword sheathed (3 damage, versus 15 with the AI's sword hit).
- A pit fall cost 8 HP here (161 → 153), not the 5 the guide says.
- The `control` variable is still true while the F1 menu is open.

### Tool gaps
- **Search crashed the game:** `rd search` ended in game-exit twice from the slime state in 260110 (around f3662; the game quit near f3814 and f3876). The same command worked in 260120.
- **Marks get lost:** sessions rebuild often and drop `ram mark`; I could not name an Auto-guard variable because of this.
- **Verify evidence is off by one:** it compares the replay at frame 6562 with the live session at 6563, which hides the real difference.
- **Missing conveniences:** no `hex()` in eval, no `--changes` option for `play --watch`, and `actors()` is not mapped, so there is no foe position or HP.

---
model claude-opus-5-5, effort high, 203 turns, $4.41, 69 min

