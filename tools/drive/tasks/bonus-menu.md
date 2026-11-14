---
name: bonus-menu
start: end:rh-water-shrine
steps:
  - in the bonus menu or beyond: frame > 1500
  - looked around for a while: frame > 5000
forbid:
  - crashed back to boot: frame > 3000 and world == 0 and tcount != 5 and frame < 0
---
**Exercise probe: the title's Bonus Menu once the prologue is cleared** (the saves of `rh-water-shrine` include a
"Prologue Clear Data" slot, so the Bonus Menu should be unlocked; on a fresh install it says "You cannot access this menu
yet." — `docs/findings/probes/title-options-bonus.md`). Read `docs/game/side-content.md` / `mechanics.md` for what the
Bonus Menu holds (galleries, music, extra modes?).

1. At the title (do NOT Continue first) pick Bonus Menu. Picture + `rd text` of every page / entry it offers; open each
   sub-screen (gallery viewers, music player, etc.), page through it, back out.
2. If an entry needs something (a clear save, a code), say what it says.
3. If an entry starts a mode (e.g. a bonus dungeon / prologue replay), enter it, look, and come back to the title.
4. End at the title or in a loaded game — NOT inside a sub-screen.

Use DRIVER.md menu pacing. Do not read other runs' movies. Stop around 15k frames. Report: run, frames, every screen with
its frame + text, what unlocks it, game facts [RD], tool gaps.
