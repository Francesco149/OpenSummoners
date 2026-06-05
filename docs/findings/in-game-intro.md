# In-game intro — the scene past the prologue (map 0x3f2)

Milestone 2 (the game proper).  The user opted to **extend the trace in-game**
(ckpt 48): spam Z after the prologue begins, capture ~1 min of retail frames,
then port to match.  This doc is the living plan + engine survey.

**Status (ckpt 54):** the foundational plumbing is in place — the **game-entry
anchor** (both sides), the **port seam** (`PROLOGUE_DONE → enter_game`), **plan 3a
RESOLVED** (the town's resource banks identified + the boot batches g2/g3/g5 wired),
the **`game_drive` scaffold is stood up** (plan 3b's first bullet), the **static
world tables are extracted + decoded** (ckpt 53), and now (ckpt 54) the **world-
table layer is PORTED + host-tested** (`src/game_world.{c,h}`): the AREA/ROOM
registry build, the `FUN_00585000` cross-reference (area defaults + reciprocal
room exits), and the `FUN_00561c90` id lookup, over generated table bytes
(`src/world_tables_data.{c,h}`).  The **`0x3f2`→opening-room resolution is
RESOLVED** (map 0x3f2 → key `0x334be` = room **210110 "Town of Tonkiness"**,
scene 1022 — see the ROOM-lookup section).  The in-game **engine** (`0x59f2c0`'s
map-object + actor slots + sim `0x586010` + render `0x5a00c0`) is surveyed but
still **unported** — the remaining body of milestone 2 (plan 3b: build the map
object reading these tables, then a slice of `0x5a00c0` for the static town
backdrop into `game_render`, register the EXE-NULL banks at the engine-time site,
diff vs `runs/tas-ingame-1`).

## How you reach it (the Z-spam exit)

The prologue cutscene `0x56cd20` exits on the **3rd "beat"** — any fresh key
press (`prologue_stone`: `beats++`, 3rd → exit fade → ~200-tick watchdog →
`PROLOGUE_DONE`).  **Z = the confirm id `0x24` (36)** = a beat (the NORMAL exit);
`0x22` would *abort* to the title instead.  On `PROLOGUE_DONE` retail's boot
driver calls **`0x59ec30(0, 0, 0x3f2)`** (app_flow.c chain: `0x1a → 0x564160 …
0x56cd20 … 0x59ec30(0,0,0x3f2)`), which loads + runs the opening map.

Trace (RETAIL flips, lockstep): Start@615 → down@715/740 → confirm@795 → fade →
prologue_enter@815 → Z beats @850/870/890/910/930 → exit → in-game.  Then Z every
20 flips advances the story dialogue.  Saved: `tests/scenarios/in-game-intro/`.

## The RETAIL golden (runs/tas-ingame-1 — local only, runs/ is gitignored)

Captured under `--lockstep` (240 s, 250 events; anchors subtitle@432 /
newgame@667 / prologue@815, consistent with all prior runs).  Frame-content
sweep (mean brightness / distinct colors):

| flip | content |
|------|---------|
| 900–1050 | **black** (cutscene exit-fade + the post-3rd-beat watchdog + map load) |
| 1100 | transition begins (mean 0.7, 99 colors) |
| **1150** | **first in-game frame — the opening TOWN of Tilelia** (houses, NPCs, hill backdrop, a banner across the top) |
| 1200–1700 | the town scene (mean ~70, 2–5k colors) |
| ~2200–2500 | **story DIALOGUE** — a character portrait (the girl) + an empty dialogue box; Z advances it |
| 3000 | dialogue continues |

So the in-game intro is: **opening town map → intro story dialogue** (Z-advanced).
Montage pushed to llm-feed (frames 1150/1200/1400/1800/2200/2500).

**The game proper renders from ~flip 1150** (retail).  900–1100 is black (the
fade-out + the map resource load — `0x59ec30` logs `"Unload Resource From…
Terminate"` between scenes and loads the new map's banks).

## Plumbing (DONE, ckpt 50)

### Game-entry anchor — both sides
- **Retail:** `SCENE_ANCHORS` in `tools/frida/opensummoners-agent.js` gains
  `{ va: 0x59f2c0, name: 'game_enter' }`.  `0x59f2c0(map,…)` is the per-map run
  loop the wrapper `0x59ec30` calls — once for the first map 0x3f2, again per map
  transition (0x2724/0x272e).  It fires on EVERY entry; the flip stamp
  disambiguates re-entries.  The cutscene exit-fade + map load make the
  commit→first-game-frame flip offset variable, so a map-loop-entry anchor is
  required (a fixed offset off prologue_enter would NOT hold).
- **Port:** `enter_game()` in `src/main.c` emits `emit_anchor("game_enter")`
  (logs `anchor: game_enter flip=<N> rng=0x<hex>`), matching the retail name so
  `tas_diff` can align the in-game frames once they render.

### Port seam — PROLOGUE_DONE → enter_game
`main_loop_body`'s prologue arm now routes the NORMAL exit (3rd beat →
`PROLOGUE_DONE`) to **`enter_game()`** instead of `leave_prologue_to_title`.
`enter_game` tears down the prologue drive, emits the anchor, logs the seam
(`enter_game: 0x59ec30(0,0,0x3f2) — opening map (in-game engine 0x59f2c0
unported)`), and — since the engine is unported — re-displays the title (like
the other unported sub-scene stubs: DEMO_START etc.).  **The ABORT path (id
0x22) still goes to `leave_prologue_to_title`.**  When `0x59f2c0` is ported,
`enter_game`'s body becomes a real `game_drive` init/step loop.

## game_drive scaffold (plan 3b, DONE ckpt 52)

`enter_game` no longer re-displays the title — it stands up a **`game_drive`**
(`src/game_drive.{c,h}`), the milestone-2 counterpart of `prologue_drive`: it
owns the in-game input ring, and `main_loop_body` runs one `game_drive_step` per
presented frame.  The in-game engine is unported, so a step has **no sim/render
model behind it yet** — its render callback (`game_render` in `main.c`) clears
the primary to **black**, the faithful map-load frame retail shows from
`game_enter` (flip ~1092) until the town first renders (~flip 1150) while
`0x59f2c0` allocates the world + loads map 0x3f2 and the entry fade runs (golden:
flips 900-1100 black).  Presenting black here is correct and replaces the prior
stub's wrong title re-display.

**Verified live** (`trace-port.jsonl`, `--frames 1300`): `game_enter@1116`
rng `0x40d00581` (matches retail `game_enter@1092`); the port then runs the
`game_drive` to frame 1300 without re-displaying the title.  Captures: frame 400
(title, phase 6) = full content (title unaffected); frames 1160/1200 (in-game,
`phase=-1`) = **fully black** (extrema 0, nonblack=0) → the early in-game frames
now match retail's black entry window.  3 host tests (`test_game_drive.c`),
753 pass.

`game_status` reserves **`GAME_EXIT`** for the ported engine's scene-transition
codes (`0x59f2c0` return 4/5 → `0x59ec30` map reload 0x2724/0x272e); for now a
step always stays `GAME_RUNNING`.  When `0x5a00c0` is ported, `game_render` grows
from the clear-to-black into the real town render walk (it reuses the already-
ported `ar_sprite_decode`/zdd/ramps), and a `game_world`/`game_scene` model joins
`input` in the drive.

## Entry-function map (the milestone-2 units)

```
0x59ec30(p1, p2, map=0x3f2)   — scene LOAD/UNLOAD WRAPPER (531 B)
  do {                            (outer loop over map (re)loads)
    *in_ECX = operator_new(0x18)  — the scene object (fields [0..5])
    result  = 0x59f2c0(map, p2, …, &local_18, …)   ← THE ENGINE (3522 B)
    unload the scene object's resources
      (0x5a4320 / 0x5bef0e ×3 / 0x43e3a0(2) / 0x5a4440;
       OutputDebugStringA "Unload Resource From… Terminate")
    if result==4||5:  next = 0x59ee50(local_18, …)
                      0x16 → reload map 0x2724,  0x17 → reload map 0x272e
    else:             teardown (free local_14 list, local_10 block) + return
  } while (true)
```

`0x59f2c0(map,…)` — the IN-GAME ENGINE (3522 B): **allocates the world, loads
the map, runs the per-room loop.**  Decomposition:

1. **Zero a ~0xeb1c-B stack scratch frame** (`0x59f2c0:44-87`): a 0x29-dword
   header, a 0x800-dword tilemap, **8× (0x47-dword + 0x108-dword) entity blocks**
   (the party/actor scratch), a 0x600-dword block, a 0x1800-dword block, a final
   0x800-dword block.  These are the per-room working buffers.
2. **Allocate the world objects:**
   - `pvVar1 = operator_new(0x4120)` — the **map object** (the big one).  Its
     `+0x4030..+0x404c` get **8× `operator_new(0xeec)`** sub-objects (per-actor
     state).  The **map-id field is `+0x4104`** — defaults to **`0x3f2`** when
     the caller passes 0 (`0x59f2c0:378-381`).  `+0x4068` = a `GetTickCount`
     stamp (the in-game pace clock).
   - `scene[4] = operator_new(0x5400c)` — a large world buffer (tile/cell data;
     ~1014× 0x54-dword entries).
   - `scene[5] = operator_new(0x7808)` — a second large world buffer.
   - `scene[1..3] = operator_new(0x88)` ×3 — three small sub-objects.
3. **Init the actor/cell registry** (`0x59f2c0:122-144`): copy the global table
   **`&DAT_006940c8`** (0x54-dword stride, zero-terminated) into `scene[4]`
   entry-by-entry, calling **`0x585000`** (413 B) per entry.  `scene[4]` head =
   `&DAT_00693848`.  This is the static map/tile-type or actor-type manifest.
4. **Two init arms** keyed on `in_stack_0000eb2c` (the second scene param):
   - `== 0` (the **fresh new-game entry — our path**): init the 8 actor slots
     via `0x560e60`, set map-object header fields (`+0x4018/+0x4054=3/+0x4064…`),
     stamp `+0x4068 = GetTickCount`, set `+0x4104 = map id`, then `0x4c5350` /
     `0x4e59a0` → fall into the room loop at `LAB_0059fb8a`.
   - `!= 0` (a **save/continue load**): `0x5b6060`/`0x41cfd0` + the save-buffer
     path (`0x5b6200`/`0x5b6300`… — the resource-0x2711 save reader, the same
     subsystem `newgame_start_save_salt` salts) then the same room-loop entry.
5. **The per-room loop** (`LAB_0059fb8a` → `LAB_0059fd85`):
   - `0x5611d0` / `0x5851e0` / `0x59e1a0` — room setup.
   - `local_8 = 0x561c90()` — fetch the active room/scene record; NULL → exit.
   - `0x560e90()` — room step.
   - `0x586010()` → **the per-step driver** (see below); its return code
     `local_c` selects: **1/2/3/0xa → `0x4e61a0`** (continue), **4/5** → a scene
     transition (`0x5a3bf0`/`0x5a3c10`/`0x4e1950` + a 58000-B alloc room rebuild,
     sets `*scene_done=1`), **default** → exit.
   - then a SECOND switch on `puVar13`: **3 → `switchD_caseD_3`** which calls
     **`0x5a00c0()`** = **the in-game RENDER dispatch**; `0x5a00c0` returning 6
     exits, else loop back to `LAB_0059fd85`.  **1/2/0xa → loop** (re-step
     without render).  So the loop is *render-on-demand*: `0x586010` steps the
     sim and only state 3 triggers a `0x5a00c0` present.
```

### The two giant children (the real milestone-2 body)
- **`0x586010` (6133 B) — the per-step driver.**  Allocates the global
  room-state object **`DAT_008a9b50 = operator_new(0x27b8)`** (`0x4017d0` ctor),
  wires the scene pointers into it (`+0x1044=scene`, `+0x103c/+0x1038` = the
  caption/param ptrs), references the string **`"Start StartArea"`**
  (`s_Start_StartArea_008a2dd0` → `&DAT_008a9b6c`, the start-room name).  Per
  `docs/findings/0057ca40-rabbit-hole.md` §6 it also drives **palette/sprite
  rendering** off the info-entry pool (`DAT_008a866c` = pool index 61, fields
  +4/+8/+0xc).  Net: this is the room **state-setup + simulation/draw step** that
  returns the loop's dispatch code.  Hybrid load+update — port it as a unit, but
  expect to stub large arms first.
- **`0x5a00c0` (13690 B) — the in-game RENDER dispatch.**  Walks the world and
  blits tiles/entities/UI.  **Reuses the already-ported sprite primitives:**
  `ar_sprite_decode` (`0x4184a0`), the zdd blits (`0x5b9b70`/`0x5b9410`/…), the
  alpha ramp leaf (`0x5bd550`), and the pace pump (`0x5b1030`).  It is recursive
  (calls itself — a scene-tree walk).  This is where the town tilemap + NPCs +
  banner + dialogue box are drawn → **the smallest visible win lives here**, on
  top of primitives the port already has.

## Porting plan (the same loop as title/new-game/prologue)

1. ~~**A game-entry ANCHOR**~~ **DONE (ckpt 50)** — retail `0x59f2c0` +
   port `enter_game` emit `game_enter`.
2. ~~**Wire the port seam**~~ **DONE (ckpt 50)** — `PROLOGUE_DONE → enter_game`.
3. **Survey + port `0x59f2c0`** in units, port-and-test each vs the golden,
   smallest visible win first:
   - ~~**(a) The map-0x3f2 resource/bank load.**~~  **RESOLVED (ckpt 51) — see
     "Resource banks (plan 3a)" below.**  No per-map resource file: the town
     lazily decodes pre-registered sprite banks via the normal `ar_sprite_decode`
     path.  Wired the deferred boot batches (`ar_register_palette_ramps` g2 /
     `ar_register_group3_sprites` g3 / `ar_register_game_sprites` g5) into
     `init_sprite_banks`; verified the title still renders `differ_px=0`.
   - **(b) The static town backdrop/tilemap render.**  ~~Stand up a `game_drive`
     (mirror `prologue_drive`)~~ **DONE (ckpt 52)** — `src/game_drive.{c,h}`,
     wired into `enter_game`/`main_loop_body`, renders the black map-load frame.
     REMAINING: port a slice of `0x5a00c0` into `game_render` (it needs the world
     populated by `0x59f2c0` setup + the `0x586010` sim step first — the map
     object 0x4120, the 0x5400c/0x7808 world buffers, the `&DAT_006940c8`
     registry).  Target the static tilemap + backdrop FIRST (flip ~1150 golden),
     diff vs `runs/tas-ingame-1`.
   - **(c) Entities/NPCs**, then **(d) the dialogue box** (portrait + textbox,
     ~flip 2200 — likely the glyph pipeline again).
4. **Diff** each ported piece vs `runs/tas-ingame-1` at the matching tick
   (anchor on `game_enter`).

## Resource banks (plan 3a) — RESOLVED (ckpt 51)

**Question:** which banks does the opening town (map 0x3f2) pull, and from where?

**Method — live res-probe (ground truth).**  New `frida_capture.py --res-probe`
hooks the generic PE-resource decoder `bs_decode_resource` (`FUN_005b7800`) and
logs every distinct `(module, id, type)` load with the flip it first fired (agent
`installResProbe`, dedup by module|id|type).  Drove retail through the full
prologue → Z-spam → in-game town under `--lockstep`
(`tests/scenarios/in-game-intro/trace-retail.jsonl`); analysed the loads with
`frame >= game_enter@1092`.

**Findings:**
- **No per-map resource file.**  Across the in-game window the ONLY resource
  loads are **`type="DATA"` sprite-sheet decodes**, all via the SAME path the
  title uses: `ar_sprite_decode` (`0x4184a0`, caller RVA 0x18582) + the slot
  palette-load `ar_sprite_slot::load_palette` (`0x4178e0`, caller RVA 0x1796a).
  `0x586010`/`0x5a00c0` never `FindResource` a map/tile file — **map layout is
  compiled-in static data** (the `&DAT_006940c8` 0x54-stride actor/cell registry,
  the "StartArea" tables), not a loaded resource.  (`sotesw.dll` = 47 WMA music;
  `sotesp.dll` = 1 DATA — neither holds graphics.  The map id `0x3f2` colliding
  with a `sotesw` WMA id is a red herring: it's the area's BGM, not tile data.)
- **74 distinct sprite banks** decode for the town + intro dialogue: **71 from
  `sotesd.dll`** + **3 EXE-embedded** (`hModule=NULL` → `FindResourceA(NULL,…)`).
  Cross-referenced against the register tables:
  | source batch | retail fn | count | examples |
  |---|---|---|---|
  | group 2 ramps/portraits | `ar_register_palette_ramps` | a few | 0x3ea/0x43a/0x6ba (dialogue faces) |
  | group 3 | `ar_register_group3_sprites` | ~38 | 0x3ec,0x481,0x769-76b,0x8b7-8bb |
  | group 4 main | `ar_register_main_sprites` (already wired) | ~15 | 0x449-451,0x456,0x583,0x6fa,0x775 |
  | group 5 game | `ar_register_game_sprites` | ~10 | 0x594,0x59e,0x5a3,0x7ef-7f9 |
  | **EXE-NULL (unregistered)** | — | **3** | **0x570,0x571,0x572** |
- **Load path = boot registration + lazy decode.**  Retail's
  `ar_boot_register_all` (`FUN_00562ea0:613`) registers groups 1-5 at boot; the
  port's `init_sprite_banks` had only done g4 + fonts.  **Wired g2 + g3 + g5
  (ckpt 51)**, all with `settings=g_sotesd` (every town bank id is a sotesd DATA
  resource).  Banks decode lazily on first render, so this is inert until a
  `game_drive` exists — verified the title is still `differ_px=0` (the title uses
  none of these banks).
- **The EXE-NULL set (`0x570-0x572`) is the one residual.**  These ids exist
  ONLY in `sotes.exe`'s own `.rsrc` (absent from sotesd) and load with
  `hModule=NULL`.  They are in NO ported sprite-register batch (the
  `locale_sounds[]` rows with the same numeric ids are a different
  resource-type namespace, not these sprites).  Retail registers them with
  `settings=NULL`, most likely **at engine time** (the map's local tileset/actor
  banks) — a `game_drive`/engine registration unit, deferred to plan 3b.
  - **Confirmed present (ckpt 52):** `sotes.unpacked.exe`'s `.rsrc` holds
    `type=DATA` ids `0x56e…0x577` (and 387 DATA entries total), so retail's
    `FindResourceA(NULL, MAKEINTRESOURCE(0x570), "DATA")` hits the EXE itself.
    (Tool: `tools/extract/sotes_resources.py vendor/unpacked/sotes.unpacked.exe`.)
  - **Port resource source (NOT settings=NULL).**  The port ships as its own
    `opensummoners.exe`, so `FindResourceA(NULL,…)` would search the PORT's
    `.rsrc` (which lacks these) — `settings=NULL` is wrong for the port.  The
    port must instead `LoadLibraryExA("sotes.exe", NULL, LOAD_LIBRARY_AS_DATAFILE)`
    (the original EXE is still in the game-dir CWD; Steamless leaves `.rsrc`
    readable) and pass THAT handle as `settings` — the same pattern as the
    `LoadLibraryA("sotesd.dll")` → `g_sotesd` the title banks use.
  - **Why this can't be wired yet (the coupling).**  Registration needs the
    **pool slot indices** these banks occupy, and those are set by the
    engine-time registration site inside the unported `0x586010`/`0x5a00c0` (the
    res-probe captured the `(module,id,type)` load, not the destination slot).
    Registering them into guessed slots would be unverifiable until the render
    walks them.  So this stays bundled with the `0x5a00c0` port: when that slice
    lands and reveals the slot indices, add a boot/engine registration of
    `0x570-0x572` with `settings=` a `g_sotes_exe` datafile handle.

## The static world tables (plan 3b groundwork, ckpt 53)

The world the engine builds is driven by **two compiled-in tables** in the EXE's
`.rdata` (read-only static data — confirming plan 3a's "map layout is compiled-in
static data, not a resource").  `0x59f2c0:122-144` copies them into the world's
`scene[4]` object on entry: it sets `scene[4][0] = &DAT_00693848` (the AREA
table) and copies every `&DAT_006940c8` entry (the ROOM registry) into
`scene[4][1..]`, cross-referencing the two via `FUN_00585000`.

**Extractor (committed):** `tools/extract/game_world_tables.py` decodes both out
of `vendor/unpacked/sotes.unpacked.exe` (PE VA→file-offset map; `.rdata` at VA
0x5cc000).  Run it for the full listing; `--raw N` hex-dumps ROOM entry N.

### AREA table — `&DAT_00693848`, 0x40-byte stride, zero-terminated (33 entries)
Each entry = an English **area name** + a few small fields `FUN_00585000` fans
out into the room record (it fills `room[0x43/0x44/0x45/0x50/0x51/0x146]` from
the matching area's dwords `[0xd]/[0xb]/[0xc]/[0xe]/[0xf-lo]/[0xf-hi]` — i.e. the
area supplies per-room defaults when the room leaves them 0).

| +off | field | role |
|------|-------|------|
| 0x00 | dword `id` | area key (matched by `ROOM.area`) |
| 0x04 | char[] `name` | ASCII English area name |
| 0x2c | dword `A` | → `room[0x44]` |
| 0x30 | dword `B` | → `room[0x45]` |
| 0x34 | dword `C` | → `room[0x43]` |
| 0x38 | dword `D` | → `room[0x50]` |
| 0x3c | u16 `E` / u16 `F` | → `room[0x51]` / `room[0x146]` |

The 33 areas ARE Fortune Summoners' world: `0xd2` **"Town of Tonkiness"** (the
**opening town** — map 0x3f2 resolves here, see the ROOM-lookup section below;
NOT `0x6e` "Town of Tolkien", a later town — the earlier "Tilelia"/"Tolkien"
guesses were both wrong), `0x6e` Town of Tolkien, Silver Dungeon, Minasa-Ratis
Magic School, Mountain Cave, Shrine of the Wind, Barness Village, Chartreux City,
… Labyrith of Night.  (Several ids have an empty name = an unnamed sub-area.)

### ROOM registry — `&DAT_006940c8`, 0x150-byte (0x54-dword) stride
Entry `[0]` is a **header sentinel** (dword0 = `0x000f423f` = 1000001, rest 0).
Real rooms are `[1..N-1]`; the table ends at the first entry with `dword0 == 0`
(`0x59f2c0:128-130` counts exactly this way).  **N-1 = 416 rooms across the 33
areas.**  Most of each 0x150-byte entry is zero (sparse — set live by the sim);
the populated fields:

| dword | field | role |
|-------|-------|------|
| 0 | `id` | packed room id, e.g. 110110 (= area 110, room 110) |
| 1 | `area` | area key → AREA table (verified: 110→"Town of Tolkien") |
| 3 | `scene` | sequential scene/record index (1002, 1004, 1005, …) |
| 7 | `d7` | link/flag |
| 7..0x42 | `exits[0x14]` | 0x14 exit slots, stride 3 dwords: {key (lo16 @dw7+3k), target `ROOM.id` (@dw8+3k), return field (lo16 @dw9+3k), 0 (hi16 @dw9+3k)}.  `FUN_00585000` part 2 fills the *reciprocal* slots from other rooms' exits.  (The earlier "dword8=parent / dword9=d9" labels were exit-slot[0]'s {target,return}.) |
| 0x43/0x44/0x45/0x50/0x51 | area defaults | filled by `FUN_00585000` part 1 from the room's AREA entry (C/A/B/D, then E=lo16 & F=hi16 of dword 0x51) — only where the room left them 0 |
| 0x46 (+0x118) | `sjis` | Shift-JIS room name (cp932), e.g. room 110110 = "トルーキンの町 １丁目" (Town of Tolkien, district 1) |

### ROOM lookup — `FUN_00561c90` (ckpt 54)
`FUN_00561c90(scene[4], key)` is a **linear search over the copied room region by
`ROOM.id` (dword0)**: it walks `scene[4] + 4 + i*0x150` for `i < scene[4]+0x54004`
(the room count, header sentinel included) and returns the entry whose dword0 ==
`key`, else NULL.

**RESOLVED — how `0x3f2` selects the opening room (ckpt 54).**  The engine is
entered with map `0x3f2` stored into map-object `+0x4104`.  The room loop
(`0x59f2c0` `LAB_0059fd85`) calls `FUN_00561c90` with the key in **`+0x4024`**
(disasm `59fd8b: mov edi,[ebx+0x4024]; push edi`), NOT `+0x4104`.  `+0x4024` is
set by **`FUN_004c5350`** — a jump-table on `*(map+0x4104)`: `map==1→0x4c5614`,
**`map==0x3f2→0x4c5516`** (writes `+0x4024 = 0x334be`, `+0x4028 = 0x65`,
`+0x402c = 1`), `map==0x3fc→0x30db3`.  So the lookup key for map 0x3f2 is
**`0x334be`**.

`0x334be` decodes as decimal **210110** (`0x3·0x10000 + 0x34be`), which **is** a
real room: registry entry **[61]**, `id 210110`, area key **`0xd2`**
(= **"Town of Tonkiness"**), **`scene 1022`**.  So the opening map renders room
**210110 "Town of Tonkiness"**, NOT room 110110 "Town of Tolkien" — *that earlier
identification (ckpt 53) was wrong; Tolkien (area 0x6e) is a later, different
town.*  (`+0x4028 = 0x65` / `+0x402c = 1` are the entry spawn params, not the
room key.)  Seven other rooms (62,63,66,68,70,77,129) carry exits whose target is
210110 → it is a town hub.  Host-tested: `tests/test_game_world.c`
`game_world_map_3f2_opening_room`.  (Caveat: the engine *resolution* to room
210110 is proven from the binary; that this room is what renders at golden flip
~1150 — vs an immediate scripted transition — is a render-time fact to re-confirm
when `0x5a00c0` is ported.)

The `exits`/area-default cross-reference + the lookup are ported, pure +
host-tested, in **`src/game_world.{c,h}`** (ckpt 54) over the generated table
bytes (`src/world_tables_data.{c,h}`, emitted by
`game_world_tables.py --emit-c`).

## The MAP OBJECT — `FUN_0059f2c0` fresh-entry arm (ckpt 55)

`src/game_map.{c,h}` ports the **runtime map object** the engine builds on a
fresh new-game entry — the `in_stack_0000eb2c == 0` arm of `FUN_0059f2c0`
(lines 160-218) plus the `FUN_004c5350` room-key resolution it calls.  This is
the object the sim (`0x586010`) and render (`0x5a00c0`) read; it sits on top of
the `game_world` table layer (ckpt 54).

**What it builds** (verbatim from the decomp):
- `operator_new(0x4120)` — the map object (modeled as a zero-initialised
  `buf[0x4120]`), plus **8× `operator_new(0xeec)`** actor sub-objects (kept in
  `actors[8]`; host pointers don't fit the 4-byte `map+0x4030` slots).
- the 8-actor loop (`0x59f2c0:162-170`): stamp each slot's index at `slot+0xa0c`,
  run **`FUN_00560e60`** (zeros `slot` dwords `[0],[1],[2],[0x271],[0x27c..0x27e]`
  + u16 `[0x272]`), set the per-slot active flag `map+0x4084+4*i = 1`.
- the header field writes (`0x59f2c0:171-214`): `map+0x40a4=1`, `+0x4018=1`,
  **`+0x4054=3`**, the `+0x405c..+0x4064` u16 run (last `+0x4064` *dword* write
  wins → `+0x4064=0`, `+0x4062` stays u16 1), the 3 `{1,0}` dword pairs at
  `+0x4108` (which fill the object's tail exactly to `+0x4120`), the
  `GetTickCount` stamp at `+0x4068` (port: a `tick` arg, default 0 for
  deterministic builds), `+0x406c=0`.
- `map+0x4104 = map id` (= `0x3f2`), then **`FUN_004c5350`** resolves the room
  key.  Its `map==0x3f2` arm (`4c5350:72-109`) writes the **room-lookup key
  `map+0x4024 = 0x334be`** (= room 210110), the entry spawn params
  `+0x4028=0x65` / `+0x402c=1`, `+0x401c=0`, `+0x40d0=0`, and ramps `+0x4014`
  toward the ceiling `+0x4020` by +5 (inert under zero-init — see below).

`game_map_active_room(m, w)` = `game_world_find_room(w, map+0x4024)` →
**room 210110 "Town of Tonkiness"** (area `0xd2`, scene 1022).

**Fidelity boundaries (documented in the header, not hidden):**
- The map buffer is **zero-initialised**; retail's `operator_new` returns raw
  memory and relies on the explicit writes + a few opaque sub-inits
  (`0x5612b0`/`0x5611d0`/`0x4e59a0`) for the rest.  `map+0x4020` (the `+0x4014`
  ramp ceiling) is set by one of those sub-inits, so under our zero-init the
  ramp is inert — it does **not** affect the room-key resolution we verify.
- `FUN_004c5350`'s opaque sprite/anim sub-calls (`0x408dc0`/`0x413b20`/
  `0x4c5e00`/`0x412db0`/…) manage the sprite registry, not these map fields, and
  are skipped.  The `map==0x3fc` arm (key `0x30db3`) is gated by unported global
  save-flag state (`0x4c57f0` lookups) and is not reproduced; the `map==1` arm's
  pure writes (`+0x401c=1`, `+0x40d0=0`) are.
- An **ordering note** the port preserves: the `map==0` → `0x3f2` default lives
  at `LAB_0059fb8a` (`0x59f2c0:378-381`), *after* `FUN_004c5350`.  So a fresh
  entry with `map id == 0` gets `+0x4104=0x3f2` but **no `+0x4024` key** from the
  resolver — the real fresh path always passes `0x3f2` explicitly (the caller
  `0x59ec30(0,0,0x3f2)`).

Host-tested in `tests/test_game_map.c` (6 tests: the 0x3f2 room key, the header
fields, the actor slots, the tail-pair fill, the end-to-end room-210110
resolution, the map==0 default).  `game_map` is **not yet wired into `main.c`**
— like `game_world` it is the world-runtime foundation the unported sim/render
units read.  **NEXT:** a slice of `0x586010` (sim) → `0x5a00c0` (render) that
walks this map object + room 210110 to draw the static town backdrop, diffed vs
`runs/tas-ingame-1` anchored on `game_enter`.

## The RUNTIME MAP DATA — `FUN_00587970` + DATA resource 1022 (ckpt 56)

Surveying the next unit (`0x586010` sim → `0x5a00c0` render) revealed that the
per-room **visual map** (the tilemap cell grid + an object/layer list — the
actual town backdrop) is **loaded from a PE DATA resource keyed by the room's
scene index**, and that resource lives in the **main EXE**, not sotesd.dll.

### Load path
`FUN_00586010` (the sim setup), after building the room-state object
`DAT_008a9b50`, resolves the map data (`586010:675-697`):
```
local_920 = DAT_008a6e7c;                       // the EXE module handle (default)
if (scene+0x103c has an override handle at +0x54008) local_920 = that;
iVar9 = FUN_00587970(local_920, (u16)room[3]);  // room[3] = the SCENE index
if (iVar9 == 0)  -> "The map data is not found. NO %d"  (fatal)
FUN_00587e00(pvVar3, room[0x44], room[0x35], room[0x43]);   // the decoder
```
`FUN_00587970` opens the resource via **`FUN_005b62a0`**, which is plain
`FindResourceA(module, scene & 0xffff, "DATA")` + `LoadResource` +
`LockResource`, then copies it out sequentially with **`FUN_005b6340`** (mode 1
= memcpy from the locked pointer).  For the opening town: room 210110's **scene
= 1022**, so the map is **`FindResourceA(EXE, 1022, "DATA")`** =
**DATA resource 1022 in the EXE** (152,936 bytes, name "MSD_SOTES_MAPDATA").

`DAT_008a6e7c` is one of a bank of six boot module-handle slots
(`0x8a6e68..0x8a6e7c`, zero-inited in `FUN_00562210`); it holds the **EXE's own
module handle**, distinct from `g_sotesd`.

### This REFINES plan 3a (ckpt 51 was incomplete, not wrong)
Plan 3a's res-probe hooked only the **sprite** decoder `bs_decode_resource`
(`0x5b7800`); `FUN_005b62a0` is a **separate FindResource path** the probe never
observed, so plan 3a's "no per-map resource file, map layout is compiled-in
static data" is true only of the **ROOM REGISTRY** (the room graph / names /
scene indices, compiled into `.rdata` — `game_world_tables.py`).  The per-room
**visual map** (tiles + object layers) **is** a loaded resource, sourced from
the **EXE** (a module plan 3a didn't check for map data) and keyed by scene
index.  This pairs with the ckpt-51/52 **EXE-NULL banks `0x570-0x572`**: the
EXE's `.rsrc` holds engine-time DATA resources, and the port must load them from
the original `sotes.exe` as a datafile (`LoadLibraryExA(..., AS_DATAFILE)` → a
`g_sotes_exe` handle) — **the same handle serves both the EXE-NULL sprite banks
and the scene-indexed map data.**

### Map-data format (decoded + bit-exactly validated)
`FUN_00587970` reads the resource sequentially from offset 0:

| bytes | field | role |
|-------|-------|------|
| `[0x00:0x04]` | magic | dword (observed `0x30`) |
| `[0x04:0x34]` | header | 0x30-byte opaque block |
| `[0x34:0x68]` | maphdr | `+0x00` char[0x20] name (space-padded) · `+0x20/+0x24/+0x28` dims · `+0x2c` count |
| `[0x68:..]` | cells | `dim0*dim1*dim2` cells × `0x1c` bytes |
| then ×`count` | layers | each: `0x3c`-byte header (sub counts `+0x1c`/`+0x20`/`+0x24`/`+0x28`, strides 4/0xc/0x100/8) followed by those four sub-arrays |

For DATA 1022: magic `0x30`, name **"MSD_SOTES_MAPDATA"**, **dims 88 × 19 × 3**
(width × height × planes), **count 86** layer entries (all uniform here:
`+0x1c=12`, `+0x20=3`).  The parse consumes the resource **EXACTLY**
(`consumed == 152936`, zero remainder) — the invariant that makes the decode
trustworthy.

### Port (pure, host-tested)
- **`tools/extract/map_data.py`** — extract + decode a map DATA resource from a
  PE (default 1022 from the EXE); asserts exact consumption.  Re-runnable ground
  truth (the ckpt-53 pattern, now for the runtime map resource).
- **`src/map_data.{c,h}`** — ports `FUN_00587970`'s parse as pure C: the caller
  supplies the locked resource bytes (FindResource/LockResource stays Win32 in
  `main.c`), and `map_data_parse` decodes into owned host allocations (magic,
  header, maphdr, dims, count, the cell array, and the `count` layer entries
  with their four sub-arrays).  Adds an overrun guard vs `len` (retail trusts
  the bundled data; inert on a well-formed map).  4 host tests
  (`tests/test_map_data.c`, synthetic blobs) → **770 pass / 0 fail / 6 skip**.
- The `0x1c`-byte cell record + the four layer sub-array element layouts are
  decoded by the (still unported) **`FUN_00587e00`**; this parser preserves their
  raw bytes for it.

### The map-descriptor object (what `FUN_00587970` builds, ckpt 57)

`FUN_00587970`'s `in_ECX` (== `FUN_00587e00`'s `param_1`) is the map descriptor
the engine re-uses across room loads.  The 0x34-byte maphdr is read *into the
object's first 0x34 bytes*, so the object layout is:

| off    | field                                                              |
|--------|--------------------------------------------------------------------|
| +0x00  | char[0x20] name (== maphdr name)                                   |
| +0x20  | dim0 (width / cols, 88)                                            |
| +0x24  | dim1 (height / rows, 19)                                           |
| +0x28  | dim2 (planes / z, 3)                                               |
| +0x2c  | layer count (86)                                                   |
| +0x34  | → cell array (`dim0*dim1*dim2` × 0x1c, `operator_new`)             |
| +0x38  | → layer-header array (count × 0x3c)                               |
| +0x3c  | → layer sub-pointer table (count × 0x10 = 4 ptrs {a,b,c,d}/layer)  |

The prologue frees a previously-parsed map (`+0x34/+0x38/+0x3c` and, per layer,
the four sub-arrays) before re-parsing — the `map_data_free` then `map_data_parse`
sequence in the port.

### The cell record (0x1c B) — semantics from the `FUN_00587e00` consumer (ckpt 57)

`FUN_00587e00`'s per-cell decode loop (`587e00.c:586-601`) reveals the field
semantics and the **linearization**:

```
idx = (dim1*z + y) * dim0 + x          // z-major: plane, then row, then col
```

| off    | field      | use in FUN_00587e00                                      |
|--------|------------|---------------------------------------------------------|
| +0x00  | co-id      | a second id set on exactly the same cells as the tile   |
| +0x04  | **tile id**| the big nested switch key (`iVar6`)                      |
| +0x08  | aux        | bank/animation selector                                 |
| +0x0c  | arg (uVar23)| low u16 forwarded as a sprite index                    |
| +0x10  | **shape**  | footprint/orientation selector (0..0xc)                 |
| +0x14  | arg (uVar25)| placement param (0 across DATA 1022)                   |
| +0x18  | arg (uVar21)| placement param (0 across DATA 1022)                   |

An **empty** cell is all-zero (`tile id == 0`).  Ported as the pure host-tested
`map_cell` + `map_data_cell(m,x,y,z)` / `map_data_cell_index` accessors in
`src/map_data.{c,h}` (this is the structure the eventual `FUN_00587e00` port and
the render walk index into).

### DATA 1022 decoded (ground truth, `--cells`)

`tools/extract/map_data.py … --id 1022 --cells` decodes the opening town:
**160 of 5016 cells populated**, forming a coherent backdrop — **z=2** is the
near plane (the long ground/floor strips along the bottom rows), **z=0** the far
plane (scattered rooftop/building runs), **z=1** a few mid-plane details.  Tile
ids cluster in the **`0x1b58b` family** (0x1b58b ×78, 0x1b58f ×33, 0x1b58c ×19,
0x1b58d, 0x1b59f, 0x1b5a0…) plus **`0x29ff4` ×21**; shape selectors used:
`{0,2,10,11,12,14,15}`; `+0x14`/`+0x18` are always 0 here.

### `FUN_00587e00` is an 18 KB multi-checkpoint rock (NOT 3 KB — ckpt 57 correction)

Earlier handoffs estimated `FUN_00587e00` at "3282 B"; it is in fact **18055 B**
(`587e00.c`, 3283 decompiled lines).  Its shape:
- a prologue (`:44-506`) that computes the runtime-grid header from the map dims,
  normalises `param_3` (10/0x28→1, 0x32→2, 0x3c→4, 0x3d→5) and switches on
  `param_2`/`param_4` to select HUD/border/frame **sprite-bank ids** (writing the
  `in_ECX[0..0x16]` header + ~30 `FUN_00417870` bank-refresh blocks over the
  `DAT_008a76xx..86xx` pool-pointer tables);
- a grid-clear pass (`:507-584`, `FUN_0054c970` per cell);
- then the bulk: a **per-tile-id dispatch** (a deep nested switch on cell+0x04)
  where *each* known tile id has a hand-written placement recipe selected by the
  cell's shape (`+0x10`), emitting tile placements via **`FUN_0058ca80`** /
  **`FUN_0058c910`** into the runtime grid `in_ECX`; plus a final layer pass
  (`:3185-3204`, `FUN_0058c8c0`/`8d0`/`cb30` over the parsed layer entries for
  ids 0x15f9a/0x15f9b).
It is deeply coupled to engine globals (`DAT_008a9b50`, the bank-pointer pool)
and ~8 helpers (`0058ca80`, `0058c910`, `0058c8c0`, `0058c8d0`, `0058cb30`,
`0054c970`, `0056df10`, `004118b0`).  **Scoping win for the eventual port:** the
town (DATA 1022) uses only tile ids `< 0x1bd82`, so the giant `0x1bd82` autotile
block and the `0x1d8ab`/`0x1ffbc…`/decoration switches are *dead code for this
map* — the town backdrop exercises only the generic per-tile-id arms + a handful
of shapes.

**NEXT:** the `FUN_00587e00` per-tile-id placement arms + the emit helpers
(`0058ca80`/`0058c910`) for the ~9 town tile ids, then the matching slice of
`0x5a00c0` to render the backdrop, diff vs `runs/tas-ingame-1`.  `0x586010`/
`0x5a00c0` remain multi-checkpoint rocks (the full engine sim + a self-contained
scripted-scene render loop with its own pace machine).

### The runtime render grid + its write primitives (ckpt 58)

`FUN_00587e00`'s `in_ECX` is the **runtime render grid**: a large flat
engine-owned buffer (≳2.9 MB) that the in-game render dispatch `0x5a00c0` later
walks to blit the room.  It is NOT the parsed `map_data` — it is the *decoded,
placed* form the per-tile-id dispatch writes into.  Cells are addressed with a
fixed **row pitch of 0x80 (128)** regardless of the map width:

```
idx = p1 * 0x80 + p2          (p1 along the dim0/0x2c1030 axis, p2 along dim1)
```

(distinct from `map_data`'s z-major `(dim1*z+y)*dim0+x`).  At fixed byte offsets
the buffer holds a small front header (`in_ECX[0..0x16]`, prologue flags) plus
**four parallel per-cell regions** and a **dim header**:

| byte      | region | per-cell | written by   | layout                                   |
|-----------|--------|----------|--------------|------------------------------------------|
| 0x000030  | A      | 0x40 (4×0x10) | `0058c910` | per sub-slot s @ 0x30+s·0x10+c·0x40: +0 u16 bank, +2 u16, +4 u16 flag, +8 i32 dx, +c i32 dy |
| 0x140030  | B      | 0x10     | `0058ca80`   | +0 u16, +4 dword, +8 dword, +c dword     |
| 0x195030  | C      | 0x0c     | `0054c970`   | +0 dword, +4 dword, +8 u16               |
| 0x2c1030  | dims   | —        | prologue     | i32 dim0 (cols)                          |
| 0x2c1034  | dims   | —        | prologue     | i32 dim1 (rows)                          |
| 0x2c1038  | dims   | —        | prologue     | i32 dim0·0xc80 (pixel extent)            |
| 0x2c103c  | dims   | —        | prologue     | i32 dim1·0xc80                           |
| 0x2c1040  | D      | 2        | `0058ca80`   | u16                                      |

(`587e00.c` addresses these via a `ushort*` base, e.g. `in_ECX[0x160818]` ==
byte 0x2c1030.)  The three **write primitives** — the smallest pure units the
18 KB dispatch calls — are now ported (pure, host-tested) in `src/map_grid.{c,h}`:

- **`FUN_0054c970`** (84 B) → `map_grid_clear_cell` — writes region C's 0xc-byte
  entry; bounds-checked against the *pixel* dims (0x2c1038/0x2c103c).
- **`FUN_0058ca80`** (167 B) → `map_grid_emit_obj` — fills a `p3×p4` (rows×cols)
  block into region B (the 0x10-byte record) + region D (the 2-byte grid).
- **`FUN_0058c910`** (347 B) → `map_grid_emit_tile` — places a tile (sprite bank)
  in sub-slot `slot` of region A, footprint either an explicit span or derived
  from the bank's pixel size (`pool[bank]+0x20`/`+0x24` rounded up to 32-px
  tiles, clamped to the grid).  The bank pool (`&DAT_008a760c`) is an engine
  global; the port takes a `mg_bank_dims_fn` callback to stay pure.

`map_grid_set_dims` ports the prologue's four dim-header writes so the
primitives can be exercised standalone.  6 host tests assert the exact bytes
each primitive deposits (`tests/test_map_grid.c`).  Everything else the prologue
does (the HUD/border sprite-bank selection over the `DAT_008a76xx` pool, the
front-header flags) stays unported — the engine-coupled body of the rock.  These
primitives are the *write* side; the per-tile-id arms that decide *what* to emit
for each town tile id, and the `0x5a00c0` *read* side that blits the grid, are
the remaining units.

### The per-tile-id placement dispatch (ckpt 59)

`FUN_00587e00`'s body (`587e00.c:572-3183`, past the deferred prologue) is one
big loop over every cell that, keyed on the cell's **tile id (+0x04)** and
**shape selector (+0x10)**, issues a recipe of `map_grid_*` write-primitive
calls — the *arms* that decide **what** each cell paints into the runtime grid.
The dispatch is a hand-written binary search over thousands of tile ids; almost
all of it is dead code for any one map.

**Ground truth — DATA 1022 exercises exactly 9 tile ids** (`map_data.py --cells`
now prints the `(id, shape) → count` cross-tab):

| id        | shapes (count)                       | recipe (587e00.c arm)                                   |
|-----------|--------------------------------------|---------------------------------------------------------|
| `0x1b58b` | 0 (×45), 2 (×33)                     | shape-sized obj block + base tile (bank `0x62`, slot 3) |
| `0x1b58c` | 0 (×10), 2 (×9)                      | same arm as `0x1b58b`                                    |
| `0x1b58d` | 2 (×1)                               | 6-call obj cluster (2 carry the `&DAT_005cc430` blend ptr in region B +0x8, region D = 2) + base tile (bank `0x63`) |
| `0x1b58f` | 0 (×21),10,11,12,15                  | optional fg tile (bank `0x17a`, frame = shape) + base (bank `0x176`); slot/flag from `z` |
| `0x29ff4` | 0 (×15), 14 (×6)                     | same as `0x1b58f` but base bank `0x177`                 |
| `0x1b5a0` | 0 (×4)                               | one base tile (bank `0x17b`, slot 2, flag `0xa`)        |
| `0x1b5a9` | 0 (×2)                               | one base tile (bank `0x172`, slot 1)                    |
| `0x1b5aa` | 0 (×1)                               | one base tile (bank `0x173`, `z`-dependent slot/flag)   |
| `0x1b5ab` | 0 (×1)                               | a tile (bank `0x174`, flag `0x14`) + a 2×9 obj block; **no** base tile |

Every other id (the `0x1bd82` autotile pre-pass, the HUD/border families that
read the `DAT_008a76xx`/`DAT_008a7bfc` bank pool inline, the `0x1d8ab`/`0x1ffbc`
decoration switches) is **dead code for the town** and is not ported.  All nine
town arms are pure compositions of the already-ported `map_grid` primitives —
*none* of them touch an engine global directly, which is exactly why the town
backdrop is tractable ahead of the engine-coupled rest of the rock.

**PORT (pure, host-tested): `src/map_decode.{c,h}`.**
- `map_decode_cell(m, grid, x, y, z, dims, ctx)` — reads the cell record and
  runs its arm (no-op for an empty/unhandled cell, matching the retail
  fall-through to `switchD_005887cc_caseD_271d`).
- `map_decode(m, grid, dims, ctx)` — the loop body: the four dim-header writes
  (via `map_grid_set_dims`), the region-C pre-clear over `dim0×dim1`
  (`587e00.c:572-584`), then the z-major per-cell dispatch + the per-cell
  region-E "co-id" zero (`587e00.c:3175`, a 5th per-cell region @ byte
  `0x1d1030`, stride `0x30`).
- The two `0x1b58d` blend pointers (`&DAT_005cc410`/`&DAT_005cc430`, engine
  .rdata blend descriptors) are written verbatim as their retail VAs
  (`MD_BLEND_*`) into region B +0x8 — the render port translates them later.

**Faithfulness mapping.**  `FUN_0058ca80`/`FUN_0058c910` calls transcribe
argument-for-argument to `map_grid_emit_obj`/`map_grid_emit_tile` (emit_tile
drops the two unused decomp params 7/8 — see `map_grid.c`); the shared tail
`LAB_0058c3b9` is the final base `emit_tile`, `LAB_0058c3b4` just zeroes the
span before it.

**STATE.** 10 host tests (`tests/test_map_decode.c`, exact-byte assertions per
arm) → **788 pass / 0 fail / 6 skip**.  Integration smoke-test: decoding the
real 88×19×3 DATA 1022 (160 populated cells) hits **0 unhandled ids** and runs
ASan-clean — the 9-id port is complete for the town.  The auto-ledger holds at
**187/1490 touched / 182 tested** (0x587e00 was already name-counted as
touched+tested via `map_data.c`'s references since ckpt 56; this checkpoint is
the genuine dispatch port behind that line).  Deferred (the engine-coupled body
of the rock): the prologue's front-header flags + HUD/border bank selection, the
`0x1bd82` autotile pre-pass, and the trailing layer pass (`587e00.c:3185-3204`,
the `0x58c8c0`/`0x58c8d0`/`0x58cb30` helpers over `0x15f9a`/`0x15f9b` entries).
`map_decode.c` is in the `src` wildcard but not yet called by `main.c` — it is
the decode foundation the `0x5a00c0` read/blit slice will drive.

### The static tilemap render walk — `FUN_00490f30` (ckpt 60)

**Model correction.**  Ckpts 50-59 called `FUN_005a00c0` "the in-game RENDER
dispatch" that "reads the decoded grid and blits the town backdrop".  Surveying
it for the read/blit slice showed it does **not** touch the render grid:
`0x5a00c0` references **none** of the grid region offsets
(`0x2c1030`/`0x140030`/`0x195030`/region A).  It is the scripted-scene **overlay
player** — a 3-state `GetTickCount` pace machine (`5a00c0.c:1646-1690`), a
sprite draw-list built on its own stack (`stack+0x98`, stride 10 dwords:
`[3]`=bank-pool index, `[4]`=mode, `[7]`=range gate, `[8]`=alpha numerator), and
a **0x124-stride caption-text array** (`stack+0x3a4`, glyph-index strings drawn
through the font bank `DAT_008a7640`).  It blits via the ported primitives
(`ar_sprite_decode`/`0x4184a0`, zdd `0x5b9b70`, alpha-ramp `0x5bd550`).  That is
the intro banner / dialogue / caption layer, **not** the tilemap.

**The tilemap is rendered by `FUN_00490f30` (2002 B).**  Found by intersecting
the ~30 grid-readers (those touching dims `0x2c1030`) with the bank pool
`DAT_008a760c`: `0x490f30` is the only render-sized one.  It is called
`FUN_00490f30(view, 1)` with the **render grid in ECX** from the per-frame draw
walk (`0x48c150:108` and `0x499100:185`, both passing the view object
`*(room_state + 0x104c)`).  What it does (`490f30.c`):

1. **Visible-cell window** (`490f30.c:40-54`) from the view/camera object +
   the grid dims.  First visible column `col0 = clampNeg((cam[0x60] +
   cam[0x34]) / 0xc80 - 1)`; `ncols = min(dim0 - col0, cam[0x64]/0xc80 + 2)`
   (a 2-cell render margin).  Row axis sums three components, one ×100:
   `row0 = clampNeg((cam[0x5c] + cam[0x74]*100 + cam[0x4c]) / 0xc80 - 1)`;
   `nrows = min(dim1 - row0, cam[0x68]/0xc80 + 2)`.  (`clampNeg(v)=max(v,0)`,
   the `((int)v<0)-1 & v` idiom.)
2. **Scan the window** (rows outer, cols inner).  Per cell, grid index
   `idx = col*0x80 + row` (the **read-side** confirmation of ckpt-58's fixed
   `0x80` row pitch — the inner loop advances the region-A pointer by `0x80`
   cells per column).  Walk region A's 4 sub-slots (`0x30 + slot*0x10 +
   idx*0x40`); for each populated one (bank `+0x0` != 0) resolve the sprite
   (`0x418470` on frame `+0x2`), apply the difficulty/time palette tint
   (`DAT_008a93fc` → the `0x4182d0` ramp), and enqueue a draw node via
   `0x4917b0`.  Node geometry: dest = the tile's world origin
   `(col*0xc80, row*0xc80)`; source-rect = a `0x20×0x20` window at offset
   `(dx*0xc80/100, dy*0xc80/100)` where `dx,dy` (region A `+0x8/+0xc`) are the
   sub-tile coordinates within the bank's atlas (so a multi-cell sprite's
   footprint cells each draw their own 32-px sub-tile).  The **layer key** is
   region A `+0x4` (the "flag" `map_grid_emit_tile` writes).
3. **Region-C objects** (`490f30.c:230-282`): the per-cell blend/overlay record
   (`+0x195038`, what `map_grid_clear_cell` and the `0x1b58d`/`0x1b5ab` arms
   write) → more nodes via `0x417c40` / `0x48c6b0`.

**The draw node (`0x4917b0`, 106 B)** is a per-LAYER bump allocator: layer table
at `render_ctx + 0x54` (8 B/layer = u16 count, u16 cap, ptr to a 0x3c-byte node
array).  It stores node `+0x00` sprite, `+0x04/+0x08` dest x/y, `+0x18` mode,
`+0x14`/`+0x10`/`+0x0c` aux; `490f30` then fills `+0x2c/+0x30` source offset and
`+0x34/+0x38` source size.  Nodes are flushed/blitted by the present pass (the
zdd path) — the consumer side of the pipeline.

**PORT (pure, host-tested): `src/map_render.{c,h}`** — the GEOMETRY of the walk,
the part that is pure arithmetic over the view object and the grid, decoupled
from the engine-coupled draw machinery:
- `map_render_visible_window(cam, dim0, dim1, *out)` — `490f30.c:40-54`.
- `map_render_grid_index(col, row)` = `col*0x80 + row`.
- `map_render_tile(grid, col, row, slot, *out)` — reads one region-A sub-slot
  and returns the draw-node geometry `{bank, frame, layer, dst_x, dst_y, src_x,
  src_y, w=0x20, h=0x20}`, or 0 for an empty sub-slot.  The grid it reads is
  exactly what `map_decode` produced — closing the decode→read loop.

**STATE.** 8 host tests (`tests/test_map_render.c`): the window arithmetic
(incl. both branches of the count cap + the negative-origin clamp + the ×100 row
component), the index, and the writer↔reader agreement (emit a tile via
`map_grid_emit_tile`, read it back) → **796 pass / 0 fail / 6 skip**.  Ledger
**188/1490 touched / 183 tested** (+1: `0x490f30`; the deferred helpers
`0x4917b0`/`0x418470`/`0x417c40`/`0x48c6b0`/`0x4182d0` are referenced by bare VA,
not `FUN_`, so the derived ledger doesn't over-count).  Both GUI builds compile
clean; `map_render.c` is in the `src` wildcard, not yet called by `main.c`.

**DEFERRED (the rest of the render rock):** the sprite resolve
(`0x418470`/`0x417c40`), the palette tint (`DAT_008a93fc`/`0x4182d0`), the
draw-node pool enqueue (`0x4917b0`, needs `render_ctx+0x54`) and the zdd
blit/present, plus the region-C blend/overlay arms.  Those + the camera/view
object's construction (where `cam[0x34..0x74]` come from) are the next units on
the path to actual town-backdrop pixels.

### The draw-node layer pool + the backdrop walk driver (ckpt 61)

The next slice down the render rock: the **draw-node accumulator** the walk
emits into, plus the driver that turns `map_render_tile`'s per-cell geometry
into a populated draw list.

**The layer table (`0x586010:510-650`).**  Right after the sim allocs the
room-state object it builds the render context's **draw-node table** at
`view + 0x54` (`view = *(room_state + 0x104c)` — the same object `0x490f30`
takes as `param_1` and `0x4917b0` reads as `in_ECX`).  It is `operator_new(0xd8)`
= **27 (`0x1b`) 8-byte layer slots**, each `{u16 count, u16 cap, ptr node[cap]}`,
and each slot is given its own `operator_new(cap * 0x3c)` node array.  The 27
caps are literal-stamped in the sim and reproduced verbatim in
`draw_pool_default_caps[]` (e.g. layer 1 = `0x80`, layer 2/3 = `0x1b8`, layer 6 =
`0x400`, …).  **Slot 0 is only zero-initialised and never given an array**
(cap 0), so emits to layer 0 always fail — a real engine quirk preserved by the
port.  A later present pass walks the 27 layers in order, so the layer index
doubles as the draw-order key (how the tilemap, sprites and HUD interleave into
one sorted frame).

**`0x4917b0` (106 B) = the per-layer bump allocator.**  `node = layer[key &
0xffff]`; `if (cap <= count) return 0;` else stamp the six caller dwords
(`+0x00` sprite, `+0x04/+0x08` dst, `+0x0c/+0x10/+0x14` aux, `+0x18` mode),
bump `count`, return the node for the caller (`0x490f30`) to finish with the
`+0x2c/+0x30/+0x34/+0x38` source rect.  The high word of `param_1` (a `dy`-derived
sort key `0x490f30` packs via `CONCAT22`) is **discarded** by the mask — dead in
the allocator.

**PORT (pure, host-tested): `src/draw_pool.{c,h}`** —
- `draw_pool_init` allocates the 27 node arrays per `draw_pool_default_caps`;
  `draw_pool_reset` zeroes the counts (the per-frame "begin draw list");
  `draw_pool_free` releases them.
- `draw_pool_emit(pool, layer_key, mode, sprite, dst_x, dst_y, p6,p7,p8)` is
  `0x4917b0` arg-for-arg (caller-order args), returning the node or NULL on a
  full layer / out-of-range key.  The node is exactly **0x3c bytes** (asserted),
  matching the `cap*0x3c` array sizing and the `+0x2c..+0x38` src-rect offsets.

**PORT (pure, host-tested): `map_render_walk`** (added to `map_render.{c,h}`) —
the backdrop-tile core of `0x490f30`'s main loop (`490f30.c:55-229`): compute
the visible window, scan it (rows outer, cols inner), and for each populated
region-A sub-slot resolve the sprite and emit a node into the pool with
`layer = region-A +0x4`, `mode 3`, `dst = tile world origin`, and the
`0x20×0x20` source rect.  The engine sprite manager (`0x418470` / `&DAT_008a760c`)
is taken as an `mr_sprite_fn` **callback** (the `mg_bank_dims_fn` pattern) so the
walk stays pure; a tile is skipped when the resolver returns 0 (retail's
`iVar4 != 0` emit gate, `490f30.c:216`).  **Deferred** (documented in the code):
the difficulty/time palette tint (`DAT_008a93fc`/`0x4182d0`, recolors pixels not
geometry) and the per-cell region-C blend/overlay arms (`0x1b58d`/`0x1b5ab`,
`490f30.c:230-282`).

**STATE.** 10 new host tests (`tests/test_draw_pool.c` ×7: node size/offsets,
the 27-cap table + array sizing, emit field layout, the `& 0xffff` layer mask,
fill-to-cap overflow, layer-0-always-full + out-of-range key, reset-keeps-arrays;
`tests/test_map_render.c` ×3: the walk over a populated grid + a full-window
camera asserting node count/layer/geometry, the resolver gate, and window
clipping) → **806 pass / 0 fail / 6 skip** (+10).  Ledger **189/1490 touched /
184 tested** (+1: `0x4917b0`; `0x586010` is referenced by **bare VA** since only
its layer-table slice is ported, so the derived ledger doesn't over-count the
18 KB function).  Both GUI builds compile clean; `draw_pool.c` is in the `src`
wildcard, not yet called by `main.c`.

**This closes the decode → grid → geometry → draw-list chain.**  What remains for
actual pixels: the **camera/view object construction** (`cam[0x34..0x74]` are
updated dynamically by the gameplay scroll across many functions — a rock, no
clean pure init) and the **present pass** (walk the 27 layers → resolve each
node's sprite → zdd blit).  Producing the *real* DATA 1022 draw list needs the
camera; the host tests use synthetic cameras + grids (the window math is exact,
so a chosen camera deterministically selects the visible cells).

### The present pass — `FUN_0048eac0` + the projector `FUN_00490b90` (ckpt 63)

The CONSUMER that turns the accumulated draw list into a blit stream — the last
pure stage of the in-game render pipeline.

**The per-frame driver `FUN_0048c150`** is the frame's render orchestrator: it
**resets** the 27-layer table (`view+0x54` counts zeroed, `0x48c150:18-26` ==
`draw_pool_reset`), runs every emitter that *builds* the draw list (the per-actor
sprite emitters `0x491ae0`/`0x4937c0`/… over the `0x11e0..0x2760` actor slots,
then the tilemap walk `FUN_00490f30(view,1)` at `:108`), and finally calls
**`FUN_0048eac0`** at `:109` to **flush** it.

**`FUN_0048eac0` (1131 B) — the present pass.** Walks the 27 layers in order
(count at `view+0x58`; layer index = draw-order key), and for each node
dispatches on the node's **mode (`+0x18`)** into four arms, each projecting the
node's world position to screen space + culling, then blitting via a different
zdd primitive:

| mode | blit primitive | ported as | who emits it |
|------|----------------|-----------|--------------|
| 0 | `FUN_005b9b70` (keyed onto) | `zdd_object_blt_keyed`-onto | actor sprites (deferred) |
| 1 | `FUN_005bd550` (alpha orchestrate) | `zdd_blit_orchestrate` | paint_ctx sprites (deferred) |
| 2 | scaled/palette (`DAT_008a9274` + paint_ctx clone) | — | scaled draws (deferred) |
| 3 | `+0x14`==0 → `FUN_005b9bf0`; else → `FUN_005bd550` | `zdd_object_blt_clipped` / `zdd_blit_orchestrate` | **`map_render_walk` (the backdrop tiles)** |

**The shared projector `FUN_00490b90` (307 B)** is used verbatim by modes 1 and 3
(inlined by mode 0): from the camera object it computes
`sx = wx/100 - (cam+0x60 + cam+0x34)/100 + offx`,
`sy = wy/100 - (cam+0x5c + cam+0x74*100 + cam+0x4c)/100 + offy` (`offx/offy` =
node `+0x0c/+0x10`), then a four-corner viewport cull: `cw+sx >= 0 && sy+ch >= 0
&& sx < cam+0x64/100 && sy < cam+0x68/100` (`cw/ch` = the node's `+0x34/+0x38`
w/h for mode 3, the sprite dims `+0x1c/+0x20` for mode 0/1). This is the **same
camera transform** as `map_render_visible_window` — the cull box just shifts
from whole-window to per-node. `map_render_walk` emits **mode 3 with param8=0**,
so the town backdrop's every tile takes the clipped color-key path
`FUN_005b9bf0(sprite_this, dest, sx, sy, w, h, src_x, src_y)`.

**PORT (pure, host-tested): `src/map_present.{c,h}`.**
- `map_present_project` — `FUN_00490b90` arg-for-arg (`param_9==0` arm, the only
  one the present pass uses); truncating int division matches the MSVC codegen.
- `map_present` — the 27-layer walk + mode dispatch. **Mode 3 is fully ported:**
  project with the node's own w/h, then select `PRESENT_CLIPPED` (`+0x14`==0) or
  `PRESENT_ALPHA` (`+0x14`!=0) and hand a resolved `present_op` (the cel handle
  from node `+0x00` becomes the blit `this`, plus the projected dst + the
  `+0x2c/+0x30/+0x34/+0x38` src rect) to a `present_blit_fn` sink. The Win32
  layer maps the kind → the matching ported zdd blit (`zdd.c`), keeping the walk
  pure (the `mr_sprite_fn`/`mg_bank_dims_fn` pattern). **DEFERRED** (PORT-DEBT
  `present-actor-modes`): modes 0/1/2 are VISITED in faithful order and counted
  via `out_deferred` (never silently dropped) but not blitted — no ported
  producer emits them yet, and their geometry reads engine sprite/paint_ctx
  internals. They land with the actor renderers (`0x491ae0` et al.).

**STATE.** 9 host tests (`tests/test_map_present.c`): the projector (placement +
camera offset + each of the four cull edges, boundary-inclusive), the walk
(layer-then-node present order + projected geometry, the CLIPPED/ALPHA kind
selection, the off-screen cull, the mode-0/1/2 defer-and-count, a dry NULL-sink
count) → **819 pass / 0 fail / 6 skip**. Ledger **191/1490 touched / 186 tested**
(+2: `0x48eac0`, `0x490b90`; the unported callees `0x48c150`/`0x491ae0` are
referenced by bare VA so the derived ledger doesn't over-count).

**This closes the `decode → grid → geometry → draw-list → present` chain** —
every stage from the map DATA resource to the per-node blit op is a pure,
host-tested unit. What remains for real backdrop pixels: the **camera/view
object construction** (`cam[0x34..0x74]`, the dynamic-scroll rock) and **wiring
into `main.c`** (a real sprite resolver `0x418470`/`&DAT_008a760c`, the EXE-NULL
banks `0x570-0x572`, and the `0x586010` sim slice that populates the grid +
builds the `view+0x54` layer table).

## Open questions for the port

- **`0x586010`'s true split** load-vs-draw: it both allocates `DAT_008a9b50`
  (looks once-per-room) yet is called every loop iteration — confirm whether the
  alloc/teardown is guarded (it frees `DAT_008a9b50` at the top if non-NULL,
  `586010:97-102`), i.e. it re-creates the room-state each step, or the loop only
  re-enters on room change.  Trace the loop cadence live (a `0x586010` entry hook
  + flip stamp) to pin whether the engine loop is per-frame or per-room.
- **The dialogue system** (portrait + textbox + the story script for map 0x3f2)
  — a sub-system of the map loop; the text is likely the glyph pipeline again.
- **Whether the intro is scripted** (an event track auto-advanced by Z) or free
  movement — the golden shows a banner + dialogue, suggesting a scripted intro.
