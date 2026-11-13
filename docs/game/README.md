# docs/game — player knowledge base (for driver agents)

Human-level game knowledge for agents driving RETAIL `sotes.exe` (EN-old, Carpe Fulgur 2012 —
port target) under rhost w/ keyboard only. Goal: new game → full playthrough + exercise every
system. NOT engine RE (that = `docs/findings/`). Facts here = community guides + strings read off the
EN-old exe; verify in rhost before a trace depends on one.

## Files
| file | what |
|---|---|
| `mechanics.md` | controls (keys, fixed vs rebindable), moves, magic casting, party AI, stats/levels, Marks of Heroism, status effects, items, equipment slots, money, saving, game over, menus |
| `walkthrough.md` | ordered route Day 1 → Day 11 → ending → post-game; numbered steps, objectives, puzzles, boss gates, missables |
| `bosses.md` | every boss: where, party, EXP, attacks, strategy |
| `side-content.md` | Help Out chain (→ White Sword), Marks of Heroism table, Memory Photos, Merrin books, capsule machines, skill books (NG+), bonus quest, code door, donations, Nightmare, bonus modes |
| `exercise-checklist.md` | checklist of every system/screen/branch a test campaign should hit + where/how |

## Source tags (used in every file)
| tag | source | good for | URL |
|---|---|---|---|
| **[DK]** | Dragonkyrie, *Guide and Walkthrough* v1.00 (GameFAQs #63757, 2012-03-31, 245 KB) | THE full route (Day 1–11 + end content + bonus quest), item/chest lists per area, shops+prices, skills w/ unlock levels, bestiary drops, books, messages. **Best source.** | https://gamefaqs.gamespot.com/pc/959420-fortune-summoners-secret-of-the-elemental-stone/faqs/63757 (live = Cloudflare; read via Wayback `web.archive.org/web/20190506005827/…/faqs/63757`; mirror Neoseeker 249723) |
| **[ZO]** | Zoelius (Chao Min Wu), *FAQ/Walkthrough* v0.35 (GameFAQs #63717, Neoseeker 249498, 2015) | **stops at Day 8.** Area-graph maps per dungeon, boss EXP table, full Mark of Heroism table (level cap + money cap), Help Out chain table, default keymap (rebindable part), version history | https://gamefaqs.gamespot.com/pc/959420-fortune-summoners-secret-of-the-elemental-stone/faqs/63717 (Wayback 20190506005827) |
| **[EXE]** | strings in EN-old `sotes.unpacked.exe` (`strings -n 6`) | ground truth for EN labels: area names, menu/option names + help text, game-over choices, difficulty names, tutorial text, keybind action names | local: `/mnt/ephemeral/opensummoners/game/enold/sotes.unpacked.exe` |
| **[MA]** | Inosenso, Steam guide *Mastering Arche* | Arche's moves in practice (jump kick, Full Moon, roll i-frames, guard-break/disarm, quick-heal behaviour) | https://steamcommunity.com/sharedfiles/filedetails/?id=1977155459 |
| **[EG]** | Binary Flame, Steam guide *Enemy Guide* (+comments) | per-enemy behaviour + counters, status inflicted | https://steamcommunity.com/sharedfiles/filedetails/?id=161156905 |
| **[MAP]** | Huzzy, Steam guide *Fortune Summoners Maps* | full-area map images for all 19 map groups (chests, traps, switches) — images only; open when lost | https://steamcommunity.com/sharedfiles/filedetails/?id=1783530423 |
| **[HS]** | John Titor, Steam guide *Hot Springs Restoration Patch* (comments) | EN-version censorship difference | https://steamcommunity.com/sharedfiles/filedetails/?id=2999832957 |
| **[PF]** | Toff, Steam guide *Performance Fix [Win10/11]* | DDraw perf issue on modern Windows (irrelevant under rhost) | https://steamcommunity.com/sharedfiles/filedetails/?id=1573821562 |
| **[CN]** | Cbk, Steam guide (Chinese) *关于汉化资源、流程攻略…* | early tip: buy Iron Sword (~750) if Merkids too hard | https://steamcommunity.com/sharedfiles/filedetails/?id=1807411626 |
| **[JW]** | ja.wikipedia *フォーチュンサモナーズ* | editions (orig 2008-09-10, demo 2007-07-07, Deluxe/Plus 2009-06-18), Plus/Deluxe modes, element triangle | https://ja.wikipedia.org/wiki/フォーチュンサモナーズ |
| **[SE]** | Steam store, *Fortune Summoners* SE (app 1381770, 2020) | what SE adds over EN-old | https://store.steampowered.com/app/1381770/ |
| **[SRC]** | speedrun.com `fortunesummoners` (API) | categories Any% (Start Game → Selene "Victory!") + Mysterious Labyrinth; Any% WR 3:10:34 | https://www.speedrun.com/fortunesummoners |
| **[GF]** | GameFAQs cheats page (via search snippet) | Nightmare unlock = beat game once | https://gamefaqs.gamespot.com/pc/959420-fortune-summoners-secret-of-the-elemental-stone/cheats |
| **[REPO]** | this repo: `docs/findings/{new-game-flow,input,game-editions-and-voice}.md`; `/opt/src/kirbys-adventure/tas/transfer/opensummoners/NOTES.md` | verified-in-rhost facts (first rooms, confirm keys, new-game config) | local |

Other places looked, no usable content: TV Tropes (403), Hardcore Gaming 101 (site down/maintenance),
GamerFuzion (index only), Neoseeker live (bot check), fandom wiki (per [CN]: nearly empty), JP 攻略 wiki
(none found by search), Reddit (nothing). Fan Discord `ods` = engine data, not route (`docs/ods-crossref.md`).
Laptop/browser NOT used (Wayback had both GameFAQs FAQs).

## Conventions
- Names in **[Brackets]** = EN item names as the game/[DK] spell them. Area names = [EXE] area table
  (verbatim; note exe typo `Labyrith of Night`).
- "Day N" = the in-game day counter (Messages menu shows `Day %3d`) — the route's spine.
- `UNVERIFIED` = single weak source / inferred; `CONFLICT` = sources disagree (both given).
- Money: 1 Silver = 10 CP, 1 Gold = 100 CP; game displays CP or GP only (System → Money Display) [DK][EXE].

## EN-version differences & known issues
- EN-old (Carpe Fulgur, 2012-01-30/31; Steam app 203510, delisted) = translation of the ORIGINAL 2008 game, w/ Deluxe's richer palette, **no Deluxe voice acting** [ZO][JW]. JP Plus/Deluxe (2009) added Arche's Mysterious Labyrinth + Arche Musou + voices [JW].
- Hot-spring scene: EN publisher **added towels** (JP original had none); fan "Hot Springs Restoration Patch" reverts [HS].
- SE (Steam 1381770, 2020-08-20, Lizsoft self-published after Carpe Fulgur dissolved): Arche's Mystery Dungeon ver. 2.0 (roguelike), Arche's Arena, 6+ new spells/techniques, 30+ items, 50+ outfit colours, new Takeda Mika art [SE]. Not our target; different engine lineage (`docs/findings/game-editions-and-voice.md`).
- Bonus-mode strings exist in the EN-old exe — reachability UNVERIFIED (`side-content.md` §9).
- Bugs: DirectDraw perf collapse on Win8/10/11 (fix = DDrawCompat/dgVoodoo; deep sub-menus still drop FPS) [PF] — irrelevant under rhost. Exe typo: area `Labyrith of Night` [EXE]. No gameplay-breaking EN bugs found in sources.
- Post-game code door number differs per save [DK] — read it from the ending text, don't hardcode 6472.
