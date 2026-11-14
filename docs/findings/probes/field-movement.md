# probe field-movement

run `field-movement`; accepted 2026-11-14 08:58

check PASS (tools/drive/tasks/field-movement.md)
verify MATCH (+ real device)
filed tests/scenarios/field-movement/trace.jsonl (10074 frames)

## driver's report

**Run `field-movement`: 10074 frames. `rd check` PASSES. `rd verify` gives MATCH, but the live session had drifted by one volatile dword (kept in runs/field-movement/verify-drift-f10074); a clean replay rebuilt the session and matched.**

I stopped at about 10k frames instead of 25k to stay within budget. Every item was exercised except pushing a crate.

**Check steps:** step 1 (loaded into a room with control) at frame 1148; step 2 (frame > 6000) at frame 6001. The forbid never fired.

**Route:** title at frame 576 (cursor on Start) → ↓ to Continue, `ent` → the load screen has a single save, slot 1 "Town of Tonkiness, Arche LV1, 0:34" → control at frame 1147 in room 210140, the house's top floor.

**Which day it is:** unclear.
- Dad's shop offers Buy / Sell / Help Out. Help Out gives a Taiyaki fetch quest.
- Mom only says "Mom, I'm going out to play!"
- The parents' closet still held the Stronggirl Juice.
- It is not Day 1 before the errands (the doors are open). Sana is not in the party (`party` = 1).

**Items, each with its frame and measurements:**

1. **Movement** (pictures looked at for 1366/1380, 1480/1629 and 8232)
   - **Walk:** 240 lx per frame after about 10 frames of speeding up. `lstate` = 1.
   - **Double-tap dash** (`L . L*30`, from frame 1197): 480 per frame, reached about 15–20 frames after starting. `lstate` stays 1.
     - A double-tap whose first press is long (`R*30 R . R`) gives no dash.
     - Pressing the opposite direction during a dash gives `lpose` = 2 (turn / skid).
   - **Slide** (↓ during a dash, frame 1370): `lstate` = 2 and `stance` = 10. Speed falls from about 470 to about 160 per frame over 34 frames and covers about 12,000 units. The picture at 1380 shows Arche lying flat.
   - **Crouch** (↓ while standing, frame 1273): `lstate` = 2, `stance` = 10. While ↓ is held, `stance` flickers 10/0 about every 14 frames.
   - **Jump:** for about 4 frames `lstate` is 3 while `ly` has not changed yet; then `lstate` is 3 while rising and 4 while falling. The card's "3 jump start" is really "rising".
     - Short jump (`C*2`, frame 1420): peak about 4,760 above the floor (ly 13600 → about 8840), about 36 frames in the air.
     - Long jump (`C*12`, frame 1462): peak about 8,420 (ly 13600 → about 5180), about 44 frames.
     - Indoors there may be a ceiling. In the field, `RC*14` reached about 9,260 (ly 26400 → 17140).
   - **Dash-jump** (frames 1611 and 8316): keeps the dash speed of about 480 per frame in the air and covers about 17,800 units. At frame 8316–8358 it cleared the field pit.
   - **Stairs:** walk onto them; the slope is 1:1 at walking speed.
     - The top floor → landing drop is at the left wall (lx 6400; ly 13600 → 16800).
     - The middle floor → ground floor stairs need ↓ plus a direction at the top. Coming back up needed jumps (found by search).
2. **Doors and exits**
   - ↑ at doors: 210140 → 210141 (parents' bedroom, frame 3342; back at 3989).
   - 210141 also contains the ground-floor storage room. Its door is at lx 57600 in 210140 (entered at 4252, left at 4929).
   - Front door at lx about 21400 → 210120 (frame 5811).
   - Walking off the right edge of 210120 (lx about 152000) → 210110 (frame 6097).
   - The town exit is ↑ at the tower right of the boutique (lx 269976) → field 225110 (frame 8025).
   - The field's right edge is also a ↑ marker (lx 254000) → 225120 (frame 9497). Its left edge (lx 0) shows an "ESCAPE" ↑ marker.
     - The first ↑ there failed while a Snake was attacking; after killing the Snake, ↑ took me back to 225110 (frame 10074).
3. **Sword and breakables**
   - Z draws the sword (`lpose` 3, about 50 frames) and Z again sheathes it (`lpose` 4). Inputs during the draw animation are dropped.
   - The storage-room barrel and pots are scenery: a Slash did nothing to them at frame 4608.
   - The barrel outside the weapon store (210110, lx about 127000) breaks: Snap Thrust (`lpose` 10105) at frame 6362 → "You found 5 copper coins", `gold` 193 → 198 at 6476. Picture at 6427.
   - No crate to push: the errand crate is gone.
4. **Examine and talk**
   - V on the parents' closet (frame 3576) → "You found Strongirl Juice". After that, X and V on the empty closet do nothing.
   - On the shop poster (lx about 7144 in 210140):
     - V → "Welcome to the Plumfield Shop!" (frame 5133).
     - X with the sword sheathed → the same text (5198).
     - V with the sword drawn → examines (5389).
     - X with the sword drawn → Slash 10101, no text (5595).
     - At frame 5489, with Arche facing the poster, X examined; that looked like "sword drawn", but the picture at 5506 showed it sheathed.
   - **Talking**, each with X:
     - Dad (frame 2652). Careful: tapping `ent` in his shop bought items and took `gold` 193 → 3. I undid it.
     - Mom (3172; needed facing her within about 4800 units).
     - "Man" in 210110 (6549: "so many monsters…").
5. **Camera** (pictures at 6645, 6743, 6816, 6887, 7057, 7117, 7209)
   - F6 cycles `cam` 0 → 1 → 2 → 0. It starts at 0 after loading.
     - 0: plain. While dashing left, Arche sat about 90 px behind centre.
     - 1: HUD icon "LOCK".
     - 2: HUD icon "LONG". While dashing right, Arche sat at about x 125, so the view leads far ahead.
   - PgUp (`pu*40`) moves the view up about 70 px, and it stays after release (7057).
   - PgDn brings it back.
   - Home (`home*40`) pans the view left about 220 px (Arche shifts to x about 530) and stays.
   - After `pd*30` plus dashing, the view was normal again (7209).
6. **Pit** (field 225110, lx about 39000): fell in at frame 8161. HP 100 → 95 at 8212, and Arche reappeared at the near ledge edge (lx 36272, ly 26399); picture at 8232. The search path later cost one more fall (HP 90).

**Named state:** `cam` = `s32(s32(world+0x2784)+0x4058)` (camera mode 0/1/2), defined in this run. `lpose` values found: 2 turn / skid, 3 draw sword, 4 sheathe, 10105 Snap Thrust (direction then X), 10101 Slash.

**Game facts [RD] (corrections for docs/game):**
- Field area exits are ↑ markers at the room edge, not walk-offs. The house front → town street exit is a walk-off.
- The town exits to the fields with ↑ at the tower right of the boutique (210110 → 225110).
- Pits cost 5 HP and put you back at the ledge you fell from.
- X examines unless the sword is drawn; V always examines.
- Dad's shop has Help Out in this save, and `ent` in his menu buys items.
- Snakes in 225120 hit for 1 while auto-guard is on (`lstate` 5).

**Tool gaps:**
- `rd look --at` with several frames gives tiny thumbnails; a full-size option per frame would help.
- No `end` key in the input spec (POV right).
- An `ent` tap loop with no menu guard can spend money: a "stop when a menu appears" option for `tap` would help.
- The search's best path took hits and a pit fall (`--avoid hp < 85` allowed that).
- The `outside` checkpoint went stale without any warning until I used `eval --at-frame`.

---
model claude-opus-5-5, effort high, 165 turns, $2.61, 28 min

