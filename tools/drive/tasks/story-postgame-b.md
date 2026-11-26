---
name: story-postgame-b
start: story-pga
steps:
  - in control: world != 0 and control
  - left the code room: room != 450410 and room != 0
forbid:
  - back at the title: world == 0 and tcount == 5 and frame > 2000
---
**Frontier probe: the post-game code room's rest (450420: [Saint's Brooch] (Arche), [Mystery Juice], 4000 / 1000 CP; the
lever → teleport outside the door) → fountain 1 save**, from the end of run `story-pga` (`rd new story-pgb --from
story-pga` — the RUN: f39084 in **450410**, mid-fight with an Ice Dragon Pup near the Mind Chocolate chest (75200,
179200), everyone alive, Arche Lv20 241/331). **There is no save in this chain since the Game Clear Data** — saving at
fountain 1 is the main goal. **Read `/mnt/ephemeral/opensummoners/drive/agents/story-pga/report.md` first** and the
walkthrough's POST-GAME section (+ the [RD story-pga] note: the room layout, the exits to 450420, the lever).

**Route:**
1. Finish / escape the Ice Dragon Pup fight with the HP guard below; heal (Sana `n8`). Equip the **Rapier → Stella** and
   use the **Stronggirl Juice** (Arche) if the previews agree.
2. 450410's right-edge exits → **450420** (unvisited): "next passage right side, drop hugging left wall → [Saint's
   Brooch] (Arche) + 4000 CP; drop → 1000 CP; [Mystery Juice] by the spikes on the right" — map it, take what is
   reachable; equip the Brooch on Arche if the preview agrees.
3. The **lever** ("teleports you back outside the door"; 450410's at (59200, 339200) bottom left, V facing it; or one in
   450420) → outside the door (450400) → 450180 → its top corridor → z5 (166400, 38400) → **fountain 1** (450185):
   drink + save (`rd endsave`; all 24 slots full: overwrite the shortest play time, keep slots 1–18 and 24).
4. Stop after the fountain 1 save, at ~40k frames past the start, or at a point you cannot get past after real attempts
   (say what blocks you, with an `rd look` PNG path). Do NOT fight Selene.

**Fights:** party HP `s32(s32(s32(world+0x2784)+0x4030+4*i)+0xa64)` for i = 0..2 — heal BEFORE a fight search; avoid
`phmin < 40`; hostile HP per character slot c: `s32(s32(c+0x750)+0x54) if u8(s32(c+0x40)) and s32(c+0x1dc) == 2 else
0`; ≤ ~4 enemy terms per score; add `room != R` to every in-room search's avoid. Black Harpies sing sleep; Black
Panthers ~860 HP. Floor spikes 70245 kill: keep searches off them (`ly` / x bands in the avoid). Shaft falls wrap
vertically in 450410.

**Tools:** `rd zones R --all` (x = LEFT edge; some doors fire only on ↑); `rd map R --crop` (spikes / lifts poorly drawn);
`rd ck R save/load/drop` (pool ~16–30); `ck load` / `undo`, NOT `rd rewind`; AVOID `rd strip` / `--at`. Short search
segments + a scripted heal phase beat one long search. Followers strand in vertical rooms: lead each up (leader swap).
Chests / levers with V facing them; chest opens show as `script == 1` + `rd text`. Never step scripts through a Yes/No /
save prompt. Searches `-j 1`. Do NOT allow `f2` in searches. Never wrap rd in `timeout`. `RD_HOST=bunker` for every rd
command; one at a time.

Do not grind; do not read other runs' movies. Run `rd check` + `rd verify` ONCE (mandatory; at the end is fine). Your
report goes in your FINAL MESSAGE (the orchestrator files it). Report: run, frames, route steps with frames, end-save
digests, levels + equipment, 450420's layout + loot, named state, walkthrough corrections, tool gaps.
