# probe fields-enemies

run `fields-enemies`; accepted 2026-11-14 14:30

check PASS (tools/drive/tasks/fields-enemies.md)
verify MATCH (+ real device)
filed tests/scenarios/fields-enemies/trace.jsonl (20444 frames)

## driver's report

## fields-enemies: report

**Run:** `fields-enemies`, 20,444 frames. `rd check` gives **PASS**: step 1 (control after loading) at f810, step 2 (frame > 8000) at f8001, and the forbid never fired. `rd verify` gives **MATCH** (f20444, rng 32a6e064). The live session first differed from the replay in one volatile dword; the dump is kept in `runs/fields-enemies/verify-drift-f20444`. A clean rebuild matched.

**Not covered:** I did not go back to town and out of the west exit, so Mothbee (51025, a type seen in another run's 260110) is not in this probe. I stopped at f20444 in 225120 on the way back, to stay within the API budget. I also did not see Blue Slimes, which in the walkthrough are a scripted Day 2 tutorial fight.

**Start:** the title came up at f576. Continue → the save list had its cursor on slot 1, so I picked slot 3 by hand (the newest by date): "Town of Tonkiness, Arche LV2 / Sana LV1, 1:26". Play starts in the Inn (210150) with 44 CP and HP 115/115. Sana is in the party, `story` = 401, difficulty is Easy (`cdiff` = 10) and Auto-guard is ON.

**Route:** Inn → town 210110 → ↑ at the tower right of the boutique (f1587) → 225110 → 225120 (f3740) → 225130 (f5033) → school grounds 230110 (f6189; exit ↑ at the tower on the right edge, lx≈244000) → 235110 (f6809) → 235120 (f7949) → Mountain Cave 240110 (f9539) → 240120 (f11317) → 240125 (f11959). Then back: 240120 → 240110 → 235120 → 235110 → 230110 → 225130 → 225120 (f20444).

### Enemy table
| Enemy | Code (+0x1d4) | Rooms | Max HP (Easy) | Damage to Arche | Hits to kill / drop | Frames |
|---|---|---|---|---|---|---|
| Green Slime | 51010 | 225110, 225120, 235120, 240110 | 24–28 (24, 25, 26, 27, 28) | 1 when guarded (f2124, "1" in the picture); 2–3 when hit mid-attack (f9153: 60→57, f9233: 57→55) | 2–3 hits of about 6–14 from Arche; once Sana's Diamond Dust finished one for 20. Drop: Exp +3 and coins (about 4 CP) | hit f2124; kills f2170 (with Sana), f9285, f9327 (Arche), f4827 (Sana) |
| Snake | 51023 | 225110, 225120, 225130, 235110 | 12–14 | Lunging bite, picture f2746. 1 when guarded (f2746, f5297); 2 mid-attack (f5317); 4 unguarded (f19519). Manual ↑ guard took 0 (green spark at f4082) | 1 Wide Slash (13) at f4167, or 2 slashes (14→4→0). Drop: Exp +2 and 1–2 CP | kills f2828 ×2, f4167, f5381, f19584, f20007 |
| Huge Bat | 51022 | 240125 only (see the gating fact below) | 15 / 17 | Diving swoop, picture f12079. 1 when guarded (f12079); 4–5 unguarded (f12439, f12879, f13129, f13204; pose 98/99) | About 2 hits; my last slash hit for 10 (17→10→0). Hard to hit from the ground: a search with `C*4 X` / `U X` found the kill. Drop: Exp +4 and coins | kills f13804 (Sana or Arche), f14010 (Arche, picture f14050) |
| Crate (an object, not an enemy) | 58010 | 225110, 225130 | — | — | — | — |
| Guard NPC | 50150 | 235120 | — | — | — | — |

**Respawn:** I left 240125 at f14352 and came back at f14454. Both bats were back with full HP (15 / 17). Slimes in 240110 and snakes in 225130 were also back on the return trip. So leaving a room resets its enemies.

### Game facts [RD]
- **The enemy table is 32 slots** at `world+0x1160` (`struct wl_char *chars[32]` in `src/world_layout.h`), not 4.
  - A slot is in use when `u8(s32(c+0x40))` ≠ 0, and targetable when `+0x1d0` ≠ 0.
  - Arche and Sana sit in the same table, after the enemies.
  - Enemy HP is at `s32(s32(c+0x750)+0x54)` and **max HP at `+0x58`**.
  - Enemy faction (`+0x1dc`) is 2; crates read 0.
- **Huge Bats only exist under some conditions.** `char_init` (`gated()`, `src/char_init.c:621`) builds a 51022 only when the time of day (`s32(0x8a6b60+0x289c)`, defined as `tod`) is 2–5, or when the grid's +0x1c or `world+0x27b4` is set.
  - `tod` was 0 the whole run, so the bats in 240110 and 240120 are invisible stubs: they have an HP value but `alive` = 0.
  - Only the 240125 bats are alive. Presumably that is the grid flag, but I did not read it.
  - 58010 crates are gated the other way: they exist only when `tod` is not 1–4.
- **Damage on Easy is small:** 1 when Auto-guard or a guard is up, 2–5 unguarded. Snake 4, bat 5, slime 3 were the largest hits seen. Manual ↑ guard blocks a snake bite completely (f4082).
- **Pits in the 225110 field cost 5 HP** (f1870, f2337) and put you back on the ledge.
- **HP jumped to full twice without a level-up** (max HP stayed 111):
  - 55→115 on entering the Mountain Cave (f9539);
  - 57→115 in 240125 around f14050–14140, while Sana had a spell circle up.
  - I don't know the cause of either.
- **The HUD HP number lags** the `hp` value for a few dozen frames after damage.
- **Room exits in this run:** idle on the ground and press ↑. While a hostile is within range, `zrest` stays at 30 and the ESCAPE icon shows; that happened at the 235120, 240110 and 225130 exits, and I had to kill nearby slimes or snakes first. Bats in 240125 did not hold the exit.
- Holding ↑ to guard while standing on an exit zone takes the exit (I did this once at 225120 and undid it).
- One `L` frame does not turn Arche; 2 or more are needed.
- **Names I defined in the run:**
  - `ec0..3`: enemy code for slots 0–3;
  - `eh0..7`: enemy HP for slots 0–7 (`eh6` only partly);
  - `ex0..7`: enemy x minus `lx`;
  - `ey1`: slot 1's y minus `ly`;
  - `tg`: the x of whichever of slots 5 / 4 is still alive (was too long to use in stop conditions);
  - `tod`: time of day.

### Tool gaps
- The 598-token limit per stop condition is the biggest problem. There is no way to say "nearest live enemy" or "loop over slots", and `actors()` is not mapped for this game, so I aimed attacks at one slot at a time. Named variables are inlined, so defining one does not save tokens.
- `rd scan` replays from boot: 72 s for a 140-frame window.
- Live strip / scan sessions were refused by host admission until another host took them.
- `{toward(x - lx)}` overshoots after a dash because of momentum.
- There is no named variable for the guard hit or the damage taken per hit. I read damage from `hp` changes and pictures.

---
model claude-opus-5-5, effort high, 207 turns, $4.47, 50 min

