---
name: save-load
start: end:rh-water-shrine
steps:
  - loaded into a room with control: world != 0 and control
  - saved and loaded for a while: frame > 6000
forbid:
  - stuck at the title for long: world == 0 and tcount == 5 and frame > 40000
---
**Exercise probe: saving and loading (docs/game/exercise-checklist.md §A load screen, §G saves).** Start from the
Day 5 save (Continue → the newest slot).

1. Find a goddess statue (walkthrough: the town / shrine statues; the inn's save prompt counts too) and SAVE: once to an
   EMPTY slot and once OVER an existing slot (the overwrite confirm). Note each slot's label (area, level, play time).
   `rd endsave <run>` after each save; report the digests.
2. Return to Title (F1 → System → Return to Title, confirm) and Continue: the load screen with several saves — page /
   scroll through every slot (pictures), load the slot you just saved, and confirm you are where you saved (room, lx/ly,
   hp, gold).
3. Load a DIFFERENT older slot from the title, then load back your own (state changes accordingly: room / party / gold).
4. In game, try F1 → System → Return to Title → "No" (stays) and "Yes" (title; the studio intro replays — count frames).
5. If any slot is empty, select it on the load screen: what happens?

Name what you need: the current slot, the play-time counter (find it: `rd ram mark` → wait → `diff`), and validate them.
Saves write `user\savedataNN.sdt`: the movie must END in the game (not on the title). Use DRIVER.md menu pacing. Do not
read other runs' movies. Stop around 20k frames. Report: run, frames, every screen / message with its frame, the digests,
named state, game facts [RD], tool gaps.
