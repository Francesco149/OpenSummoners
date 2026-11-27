# probe story-postgame-a

run `story-pga`; accepted 2026-11-27 17:49

check PASS (tools/drive/tasks/story-postgame-a.md)
verify: by the batch (the leaf movie of its chain: `rhost.py legs`)
filed tests/scenarios/story-postgame-a/trace.jsonl (39084 frames)

## driver's report

# story-pga report: inside the post-game code room, no save made

Run `story-pga` went from Game Clear Data (slot 18) to the Labyrinth of Night, opened the code door and took three of the code room's chests. It did **not** save at fountain 1 and did **not** get the Saint's Brooch: the code room nearly wiped the party and the ~40k-frame budget ran out. The run ends at **f39084** (about 38.2k frames past the load), mid-fight with an Ice Dragon Pup, everyone alive.

- **Gates:**
  - `rd check`: **PASS** (step 1 at f916, step 2 at f2001, no forbid hit).
  - `rd verify`: **MATCH** (frame 39084, rng 070393e4, semv 600ab4090bbfb96e, fb 71ef392773297615).
- **End save:** none. The next leg must start from the run itself (`--from story-pga`), not a save.
- **Session:** stopped. No commits.

## The door code is 4596 in this save, not 9792
- I entered 9792 first. It was wrong, and the door triggered a teleport trap that dropped the party in 450150. I rolled back to before that.
- Retail stores the code at **`cfg+0xa88`** (the config object, `u32(u32(0x8a6e80))`), and it reads **4596** after this save loads.
- The check is in the decompile near `"What will you set dial %d to?"` (all_ida.c ~140407): the four dials are joined into a string and compared with `"%04d"` of the stored code. If the stored code is 0, it rolls 4 random digits (no two adjacent digits equal) and saves the config.
- The filed save's `config.dat` (840 B) holds neither 9792 nor 4596 as plain int32s, so it is probably encoded.
- **I can't explain the mismatch with story-d11o's 9792.** Either the president's line in the ending doesn't come from `cfg+0xa88`, or the end-save filing didn't capture the config d11o wrote. Worth a probe, and the walkthrough's "9792" needs a note: read `s32(cfg+0xa88)` in your own run.

## How the code door works (EN-old)
- The door is room **450400**, zone 2 at x 64000, between two gargoyles. Press ↑. You reach 450400 from 450180's far top-left zone 4 (x 0, y 19200), a walk-in exit.
- **First visit:** "The door is sealed shut.", then a party conversation (four dials, Stella: "might be trapped", Sana: "look somewhere else until we know the number"). It ends with no prompt.
- **Second ↑:** "The door is locked fast. You have no way of forcing it." → (ent) "There are four dials with numbers 0 through 9 on them. Would you like to try setting the dials?" → Yes/No, cursor on Yes.
- **Dial entry:** four times "What will you set dial N to?" with a 10-row list ("Set to 0" … "Set to 9"). The cursor starts on 0 each time; press D N times, then ent.
  - Rhythm that worked: confirm with `ent .*110`, then `ent .*40` opens the next list.
- **Right code:** "The door opened!". ↑ then enters **450410**.
- **Wrong code:** "You seem to have put in the wrong numbers... you suddenly feel very light!" → "You triggered a teleport trap!" → room **450150**. That covers the checklist's wrong-number probe.
- Extra `ent` presses after the conversation swapped the leader through Sana and Stella and back.

## Route
| frame | what |
|---|---|
| 576 | Title. The cursor remembered Bonus Menu (tcur 2); U → Continue |
| ~713 | The load screen cursor starts on **slot 7**, not the newest; R R to page 3, U×5 to slot 18 |
| 915 | Free roam in Tonkiness 210110 at x 12800 |
| 1497 | 210110 zone 7 (265600) → 225110. A snake held the exit until I killed it |
| 2926 → 3232 → 4320 | 225120 → 225130 → school yard 230110. No darkness cutscene in this state |
| 5116 | 230110 door (stop fully at x ~159000, then ↑) → **450110** |
| 6049 / 6564 / 7382 | → 450120 → 450130 (no plate scene) → 450140 |
| 8298–8655 | 450140: Unison at x ~119000, then a triple jump onto the brick platform (ly 20000). Walking low falls into the pit |
| 10424 | High route right; the rattlesnake and bat at the exit killed → 450145 |
| 13827 | 450145 (Centurions) → 450150 |
| 17013 | 450150 bottom; Centurion and 51241 killed; zone 3 → 450170 |
| 18013–21740 | 450170: up the right-hand stairs to the top corridor. The followers got stuck at the bottom: I led Sana up, then regrouped with Arche |
| 22447–24149 | Two Skeletal Corporals killed; gap crossed; Blazemage killed; zone 2 → **450180**. Arche Lv20 |
| 25738 → 25819 | 450180: left shaft up to the top corridor; zone 4 (x 0) → **450400**. Fountain 1 skipped (budget) |
| 26135–31347 | Code door: two ↑ visits, wrong-code probe rolled back, **4596** → opened |
| 31737 | → **450410**, the code room |
| ~33430 | **[Stronggirl Juice]**, chest (27200, 115200). The game spells it "Strongirl Juice" |
| ~36870 | **[Rapier]**, chest (36800, 73600), after riding the spike traps and killing the Org Centurion standing on the chest |
| ~38300 | **[Mind Chocolate]**, chest (75200, 179200), between two Ice Dragon Pups |
| 38696–39084 | Fled right, then a search found a heal path (a Better Herb on Arche). **End** |

## Code room 450410
- **Layout:** 72×112 cells. A top corridor (y 16–35k) with the door at x 86400. A central shaft (x ~140–157k) from top to bottom. Exits to **450420** on the right edge at y 19200, 131200, 198400 and 329600 (zones 2–5); 450420 is unvisited.
- **Chests:**

| chest | where | item |
|---|---|---|
| 70220 | (36800, 73600), upper-left passage | Rapier. Reached by riding the hanging spike traps 70430 (x 54–80k, y 76800) up from the corridor below |
| 70220 | (27200, 115200), lower-left corridor (reached via the stairs off the shaft at y ~107000) | Stronggirl Juice |
| 70220 | (75200, 179200), passage left of the shaft | Mind Chocolate |

- **Not in this room's chests:** the Saint's Brooch, the Mystery Juice and the 4000 / 1000 CP. They are probably in 450420.
- **Lever** 70216 at (59200, 339200), bottom left, with floor spikes 70245 at x 10–17k and 177–196k, y 346400. A fall onto the left spikes killed Arche from 45 HP. I could not trigger the lever with V; its state stays at `s32(actor+0xc)`.
- **Traps:** plates 70800 at (28800, 19200) (it went to state 1 when the party walked over it) and (86400, 249600).
- **Monsters:**
  - Black Harpy 51092: sleep song.
  - Black Panther 51213: 860 HP.
  - Ice Dragon Pup 51262: ~680 HP, 2 at the Mind Chocolate chest.
  - Org Centurion 51122.
  - Skeletal Corporal 51252.
  - Ghost Warlock 51083.
  - Vile Bat 51062, Purple Slime 51015.
- **Shaft falls wrap vertically:** Sana fell from ly ~259k and reappeared at ly 93600, and a search landed Arche at the passage level from below.
- **Chest flags:** the object flag count (`flags+0x7804` = 417) did not change when chests opened, so it can't be used as a chest goal. I used `script == 1` plus `rd text` instead.

## Party at the end (f39084)
- **HP / levels:** Arche **Lv20** (241/331 HP, 47/59 MP), Sana 258 HP / 168 MP, Stella 255 HP / 151 MP. Sana and Stella's levels were not re-read.
- **Money:** 41477 CP.
- **New items:** Rapier, Stronggirl Juice, Mind Chocolate.
- **Not done:**
  - Rapier not equipped on Stella.
  - Stronggirl Juice not used.
  - No save made.
  - Saint's Brooch, Mystery Juice and the CP chests not taken.
- **Items used:** a quick-heal item (the cast bar), then a Better Herb (Arche +166).

## Walkthrough corrections (EN-old)
- **The code number:** read `s32(cfg+0xa88)` in your own run rather than relying on the president's line (see the 4596 section).
- **450400 is the code-door room**, the "keep heading left" step from 450180. Fountain 1 (450185) is a separate detour right along the 450180 top corridor (z5 at 166400, 38400).
- **450170:** the top corridor is reached by the right-hand diagonal stairs (from x ~200k), then a shaft with small ledges (x ~230–240k). The followers do not climb it on their own: lead each of them up.
- **450140:** the safe crossing is the high route (Unison triple jump at the left ledge).
- **The guide's split of the code room is wrong for EN-old:** Mind Chocolate is in the **first** visit's reach (passage at y ~176800 left of the shaft), not only after re-entering. The Brooch and CP are not in 450410's three chests.

## Named state (run-local)
- `m0`–`m2`, `phmin`: party HP.
- `e0`–`e15`: hostile HP per slot.
- `px0`–`px2`, `py0`–`py2`: positions of party slots 0–2.
- `lever`: the 450410 lever actor's state.

## Tool gaps
- **Searches over 180 s go to the background,** which costs a wait round-trip each time.
- **Search score has no waypoints:** multi-stage scores were awkward and often converged on dead ends, such as 450180's lower room under the fountain door.
- **AI followers strand in vertical rooms** (450170) and fall through shafts. There is no "gather the party" helper.
- **Nothing reads which chest you opened:** `flags+0x7804` stayed the same.
- **`rd map` shows pits / spikes poorly:** it doesn't separate floor spikes, so a search fell onto them. An avoid zone for spike cells would help.

## Files
- Final frame: `/mnt/ephemeral/opensummoners/drive/runs/story-pga/view/look_f039084.png`
- Code entry (dial list): `/mnt/ephemeral/opensummoners/drive/runs/story-pga/view/look_f030271.png`
- Yes/No prompt: `/mnt/ephemeral/opensummoners/drive/runs/story-pga/view/look_f030109.png`
- Code room map: `/mnt/ephemeral/opensummoners/drive/runs/story-pga/view/map_450410_f31776.png`
- Rapier chest ledge: `/mnt/ephemeral/opensummoners/drive/runs/story-pga/view/look_f036464.png`

## Next leg
Start `--from story-pga`, heal, then:
1. Equip the Rapier on Stella and use the Stronggirl Juice.
2. Take the right-side exits to **450420** for the Saint's Brooch, the Mystery Juice and the 4000 / 1000 CP.
3. Pull the bottom lever, which teleports you outside the door.
4. Walk 450400 → 450180 → fountain 1 (z5) and save.
