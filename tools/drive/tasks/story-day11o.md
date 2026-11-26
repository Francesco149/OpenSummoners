---
name: story-day11o
start: save:da496577f7be
steps:
  - in control: world != 0 and control
  - left fountain 3: room != 450295 and room != 0
forbid:
  - back at the title before the ending: world == 0 and tcount == 5 and frame > 2000 and frame < 3000
---
**Frontier probe: the FINAL leg — Labyrinth of Night 7.10 (junction lower stairs → outside by the gargoyle → lock (Pitch
Black Key) → brick platforms: boost to the top chest [Memory Photo 8] → fancy doors → cutscene → **FINAL BOSS: Dark Witch
Selene**) → the ending (walkthrough Day 11 step 8)**, from story-d11n's end save (`rd new story-d11o --from
save:da496577f7be` — slot 17, **fountain 3** = 450295, full HP/MP). Party: **Arche Lv19** 322 HP (Org Slayer, Iron Armor,
Leather Cape, Kitty Ears; Unison, Wind Thrust, The Full Moon), **Sana Lv19** 240 (Star Rod; Heal Water II `n8`, Force
Field `n9`, Mirror Image), **Stella Lv18** 274 (Magician's Outfit, Celebrity Ring; Interceptor); 30789 CP; **[Pitch Black
Key] ×1**; Better Herb ×5, Best Herb ×2, Revival Drink ×2 + EX, Magic Candy ×2, Mind Chocolate. **Read
`/mnt/ephemeral/opensummoners/drive/agents/story-d11n/report.md` first**, then the walkthrough's Day 11 steps 7.10 + 8
(+ the [RD story-d11j … d11n] notes) and `docs/game/bosses.md` (Selene: summons Skeletal Sergeants + Black Panthers;
dagger ring; dark energy spread; large dark ring; "hammering"; the [MA] hint: Sana's Shield + Mirror Image + Force Field
on Arche → The Full Moon; Stella's Interceptor / Explosion vs the summons; heal with Heal Water II / Heal Mist).

**Route:**
1. 450295 → 450290 → junction 450270 **z2 (102400, 73600) lower stairs → 450300** → "outside by gargoyle → drop right →
   lock (key) → small brick platforms: boost to top chest [Memory Photo 8] → fancy doors". **Save at the last statue /
   fountain before the doors** if one exists (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep
   slots 1–17 and 24) — else fountain 3 (slot 17) is the fallback.
2. **Selene:** full HP/MP; `rd ck save` at the doors and again after the cutscene, before the first hit. Read her HP
   (find her character slot: hostile HP per slot c = `s32(s32(c+0x750)+0x54)` when `u8(s32(c+0x40))` and
   `s32(c+0x1dc) == 2`) and score fight searches on her HP + the summons' HP, NEVER letting any member's HP drop low
   (avoid `phmin < 40`; heal with Sana `n8` / leader swap + `F`; Revival Drinks if someone falls). Buff before/at the
   start (Unison; Sana's Force Field / Mirror Image on Arche if castable). Work in phases with checkpoints; if a phase
   repeatedly kills a member, change the approach (distance, Stella vs summons) rather than brute-forcing.
3. **The ending:** after "Victory!", tap through the scenes + credits (`rd tap` on script / text; never batch `ent` through
   a Yes/No or SAVE prompt — read each prompt with `rd look`). **Note the class president's 4-digit number** (it differs
   per game). At the "save → Game Clear Data" prompt: save to a slot (overwrite the shortest play time, keep 1–17 and 24)
   and report its digest. Stop at the title / when the game returns to a menu, or at ~40k frames past the start if the
   boss is not beaten (end on a pre-boss save then; say what kills you, with `rd look` PNGs).

**Fights:** party HP `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2; dormant foes `c+0x1d0` == 0; slot
numbers move per room; add `room != R` to every in-room search's avoid; ~4 proximity-filtered enemy terms fit the
598-token limit. Arche's sword draw (~45 f) is hittable: draw before approaching. Unison: sword drawn, hold X ~105 f,
release (10 MP), lasts thousands of frames; X tap = Wind Thrust.

**Tools:** `rd zones R --all` (x = LEFT edge; some doors fire only on ↑); `rd map R --crop` (no lifts / planks / blocks);
`rd ck R save/load/drop` (pool ~20–30: drop old ones; `rd stop` clears a wedged pool); `ck load` / `undo`, NOT
`rd rewind`; AVOID `rd strip` / `--at`. `rd search --hold`. A "1 cell only" search = a doomed start state. Menu message
boxes close with X. Searches `-j 1`. Do NOT allow `f2` in searches. Never wrap rd in `timeout`. `RD_HOST=bunker` for
every rd command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment at the end, the boss fight (phases, her HP, attacks, what worked), the president's number,
the ending's sequence (rooms / scenes / prompts), named state, walkthrough corrections, tool gaps.
