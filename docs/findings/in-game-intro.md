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
host-tested unit. What remains for real backdrop pixels (ckpt 64 update — the
**camera is no longer a blocker**, see "The camera/view object" below): **wiring
into `main.c`** — a real sprite resolver (`0x418470`/`&DAT_008a760c`), the
EXE-NULL banks `0x570-0x572`, and the `0x586010` sim slice that populates the
grid + builds the `view+0x54` layer table. The first-frame camera is the
live-verified constant `MAP_RENDER_CAM_TOWN_3F2`; the spawn-snap + intro pan
that produce it are deferred (PORT-DEBT `ingame-camera-snap`).

### The camera/view object — located, init RE'd, first-frame value live-probed (ckpt 64)

The last blocker before real backdrop pixels was the **camera/view object**
(`cam[0x34..0x74]`), described through ckpt 63 as a "dynamic-scroll rock with no
clean pure init point."  That is now **refuted**: the object has a clean,
portable room-entry init, and the opening town's *first-frame* camera is a
determinate constant, live-verified on retail.

**Where the object lives.**  `view = *(room_state + 0x104c)` where `room_state
= DAT_008a9b50`.  Byte `0x104c` = dword index `0x413`; the room-state ctor
`FUN_004017d0:187` allocates it as **`operator_new(0x78)` (120 B)** and stores
it at `in_ECX[0x413]` (zeroing `view+0x54`, the draw-node layer-table count).
So the **camera *is* the view object** (one 0x78-byte struct), spanning exactly
`+0x00..+0x74` — the fields `map_render`/`map_present` read.

**The room-entry init (portable).**  `FUN_00586010:854-872`, right after it
builds the layer table, initialises `view = *(room_state + 0x104c)`:
```
view[0]    (+0x00) = grid[0x2c1038] = dim0*0xc80   (map pixel width)
view[1]    (+0x04) = grid[0x2c103c] = dim1*0xc80   (map pixel height)
view[0x17] (+0x5c) = 0            view[0x18] (+0x60) = 0     (scroll origin)
view[0x19] (+0x64) = 64000        view[0x1a] (+0x68) = 48000 (viewport, 640/480 *100)
view[0x1d] (+0x74) = 0            view[6] (+0x18) = 1
FUN_00587d30(view+9)   -> zeroes the +0x24 sub-block (incl +0x34)
FUN_00587d30(view+0xf) -> zeroes the +0x3c sub-block (incl +0x4c)
```
`FUN_00587d30` (28 B) just zeroes 5 dwords + 2 u16 of a 0x14-byte sub-struct.
Net: at room entry the camera is **origin (0,0), viewport 640x480 (*100), all
scroll components 0** — a pure init.  Ported as `map_render_camera_init`
(`src/map_render.{c,h}`, the mr_camera 7-field projection), host-tested.

**The first town frame is NOT origin-0 — the engine snaps + pans (live ground
truth).**  A new field-spec probe (the agent's `src:"chain"` global-deref:
`*(*(0x8a9b50) + 0x104c) + off`, `tools/flow/retail_fields.json`) read the
camera at the Flip across the in-game window, twice, under `--seed-pin
--lockstep` (`/tmp/camprobe`, `/tmp/camprobe2`):

| flip | +0x60 (x) | +0x5c (y) | +0x64 | +0x68 | +0x34/+0x4c/+0x74 | note |
|------|----------:|----------:|------:|------:|:-----------------:|------|
| 1050 | (null — room_state not yet built; `game_enter@1092`) | | | | | |
| 1093 | 128000 | 12800 | 64000 | 48000 | 0 | snapped; viewport == init |
| 1100..1176 | **128000** | **12800** | 64000 | 48000 | 0 | **stable hold (~83 flips)** |
| 1180 | 127940 | 12800 | … | … | 0 | pan onset (Δ −60) |
| 1200 | 125000–126100 | 12800 | … | … | 0 | accelerating (run-to-run ±~4-flip phase jitter) |
| 1250..1500 | 111650 → 36650 | 12800 | … | … | 0 | leftward cruise ≈ −300/flip |

So between the room build and the first frame the engine **snaps** the origin to
the entry spawn (`+0x60 = 128000 = 40 cells`, `+0x5c = 12800 = 4 cells`; the
entry spawn params `+0x4028=0x65`/`+0x402c=1` from `FUN_004c5350`), holds that
establishing shot ~flip 1093→1176, then runs a **scripted horizontal pan**
(leftward, easing up to ≈ −300/flip).  The town first renders (~flip 1150)
firmly inside the stable hold, so the **first town frame camera is the constant
`{+0x60=128000, +0x5c=12800, +0x64=64000, +0x68=48000, rest 0}`** — ported as
`MAP_RENDER_CAM_TOWN_3F2`.  Its visible window over the 88×19 grid is **cols
39..60 (22), rows 3..18 (16)** (the right-middle of the map; the pan then walks
left).  Viewport `+0x64/+0x68` match the static init exactly, confirming the
586010 RE.

**Parity note (pillar 2 = phase).**  The pan-onset flip drifts a few flips
between otherwise-identical seed-pinned/lockstep runs — the same render-pace
phase skew seen at the title (`parity-ledger.md` R3).  It does not touch the
first-frame camera (deep in the stable hold) but must be anchored (not compared
by raw flip index) once the pan is in scope.

**Still unported (PORT-DEBT `ingame-camera-snap`):** the spawn-snap that sets
the origin from the entry params, and the scripted intro pan (the dynamic-scroll
engine across the `0x4710c0`/`0x54f980` camera-follow/copy routines + `0x499ab0`
which pushes a controller's `+0x4c` into `view+0x74`).  For the static first
frame these are replaced by the live-verified `MAP_RENDER_CAM_TOWN_3F2` constant.

### The backdrop pipeline WIRED — first real in-game pixels (ckpt 65)

The pure pipeline (decode → grid → walk → present, all host-tested) is now
**composed + wired into `main.c`** and produces real town pixels.

**The composition (pure, host-tested): `src/town_render.{c,h}`.**  A thin
per-room SCENE that owns the state the stages share — the parsed `map_data`, the
runtime render grid (`map_grid_alloc`), the 27-layer `draw_pool` — and runs them
in the engine's order: `town_render_load` = the once-per-room
`map_data_parse` (`0x587970`) + `map_decode` (`0x587e00` town arms);
`town_render_step` = the per-frame backdrop slice of the draw driver `0x48c150`
(`draw_pool_reset` → `map_render_walk` `0x490f30` → `map_present` `0x48eac0`).
The three engine globals stay seams (callbacks).  6 host tests
(`tests/test_town_render.c`) drive a real parse+decode of a minimal in-memory
resource through to the exact present op.

**The Win32 glue (`main.c`).**
- `load_town_scene(1022)` (in `enter_game`): `LoadLibraryExA("sotes.exe",
  AS_DATAFILE)` → the EXE's own `.rsrc` (the engine-time module `DAT_008a6e7c`,
  distinct from `sotesd.dll`'s sprite banks), then `FindResource`/`Load`/`Lock`
  of `DATA` scene 1022 + `town_render_load`.  **Live-verified: DATA 1022 loads
  from the *packed* `sotes.exe`** (Steam-DRM intact `.rsrc` — Steamless not
  needed at runtime): 152936 B, name "MSD_SOTES_MAPDATA", 88×19×3, 86 layers.
- `game_sprite_resolve` = `0x490f30`'s resolve: `ar_pool_get_slot(bank)`
  (`&DAT_008a760c[bank]`) + `ar_sprite_slot_frame` (`0x418470`).  The bank→pool
  mapping is internally consistent (`&DAT_008a760c[0x62]` == BSS `0x8a7794` ==
  register table-idx 85 == resource `0x433`); `ar_register_game_sprites` (g5,
  already booted) populates every town bank (0x62/0x63/0x172-0x177/0x17a/0x17b).
- `game_bank_dims` = pool[bank] `+0x20/+0x24` (`ar_sprite_slot.width/height`).
- `game_present_blit` = the mode-3 CLIPPED path → `zdd_object_blt_clipped`
  (`0x5b9bf0`); `PRESENT_ALPHA` can't fire for the backdrop (node `+0x14`==0).
- `game_render` clears black then walks the scene through
  `MAP_RENDER_CAM_TOWN_3F2`.

**Result (live, `--input-trace` in-game-intro, `game_enter@1116`).**  The port
renders a **coherent town backdrop** — the red-roofed half-timbered house, the
vine trellis/arch, the stone-block walls, ivy + grass — the **same assets as the
retail golden**, at the **matching gameplay tile scale** (118k nonblack px, 212
colors).  Cross-checked vs golden flip **1800** (post-establishing-shot, at
gameplay 1:1): the house / stone walls / grass / ivy are pixel-scale identical to
the port → the backdrop tile layer is structurally + asset-correct.

**NOT a `differ_px==0` frame — the named residuals (all deferred layers).**
- The full-screen **parallax sky/mountain far-plane** + the **foreground trees**
  + the **dialogue box & caption overlay** (`0x5a00c0`) are not drawn → the
  upper/right of the port frame is black where they belong.  (PORT-DEBT
  `ingame-nontile-layers`.)
- The **NPC actors** (Arche + co — present modes 0/1/2).  (PORT-DEBT
  `present-actor-modes`.)
- Retail's flip-1150 hold is a **zoomed-OUT establishing shot** (whole-town vista
  + banner) that zooms to 1:1 by ~flip 1800; the port renders gameplay 1:1 at the
  hold's scroll origin.  The camera **scale/zoom field** was not in the ckpt-64
  probe (scroll `+0x34..+0x74` + viewport only).  So a flip-anchored full-frame
  diff at the hold is not yet meaningful — the on-screen relationship between
  `MAP_RENDER_CAM_TOWN_3F2` and the establishing shot is a render-time question
  for the `0x5a00c0` / pan port.  (PORT-DEBT `ingame-establishing-zoom`.)

**NEXT** (smallest visible win first, on top of the now-proven tile layer): the
**parallax far-plane** (the sky/mountain full-screen backdrop — likely a
dedicated bank blit or a `0x5a00c0` slice), then the **actor renderers**
(`0x491ae0` et al. → present modes 0/1/2), then the **dialogue overlay** (the
glyph pipeline again).  Each is diffable against `runs/tas-ingame-1` once the
establishing-shot/zoom relationship is pinned (so the port + golden share a
camera).

### The PARALLAX far-plane — RE'd (producer + descriptor + town values) (ckpt 66)

The sky/mountain full-screen background (the black region above/left of the
ported tile layer; **golden flip 1800** shows blue sky + a hazy blue mountain
ridge upper-left).  It is **NOT a backdrop tile** (region A) and **not** a
`0x5a00c0` slice — it has a **dedicated producer** drawn *before* the tilemap.

**Two producers, identical output.**  The background is drawn by two engine
functions that read the **same descriptor** (the grid front-header) and emit the
same 3 horizontal sprite-strip layers via the select+blit pair
`FUN_00417c40(bank,frame)` (set current cel into ECX) → `FUN_005b9a40(surface,
x,y)` (blit it):
- **`FUN_00490cd0` (603 B)** — the **inline** version, called **first** in the
  per-frame world-render driver `FUN_0048c150` (`:47`, before the actor emitters
  + the tilemap walk `0x490f30` + the present `0x48eac0`).  This is the
  **free-roam / normal-gameplay** path (`0x48c150`'s `in_ECX[7]==0` branch).
- **`FUN_00499100` (1118 B) → `FUN_00499560` (271 B)** — the **helper-based**
  version, called from `0x48c150`'s **else** branch (`in_ECX[7]!=0`, via
  `FUN_0048c6b0(0x3eb,…)`) — the **establishing-shot / special-render** path.
  `0x499100`'s layer-A loop is byte-identical to `0x490cd0`'s; `0x499560(surface,
  view, desc_ptr, factor)` is `0x490cd0`'s B/C-layer math extracted to a helper
  (`grid+0x10`@factor `0xfa`, `grid+0x04`@factor `500`).  The two producers are a
  **mutual cross-check**: they agree on every descriptor offset and the scroll
  algebra.

**The descriptor = the grid front-header** (`*(DAT_008a9b50+0x1048)`, the runtime
render grid built by `0x586010`/`0x587e00`).  Layout (ushort base), and how each
field is consumed:

| byte | field | consumed as |
|------|-------|-------------|
| 0x00 u16 | **layer A** bank | 8 tiles, frame 0..7 at x=`i*0x50`, y=0 — no parallax (top strip) |
| 0x04 u16 | **layer C** bank | 9 tiles; parallax-X **0.5** (`500/1000`) |
| 0x06 u16 | layer C baseY | tile y = baseY + paraY-offset |
| 0x08 u16 | layer C wrap | `frame = col % wrap` |
| 0x0c i32 | layer C paraY | vertical-parallax factor (`*(camY/100)/32 * paraY /1000`, clamp `[-0x1c,0]`) |
| 0x10 u16 | **layer B** bank | 9 tiles; parallax-X **0.25** (`0xfa/1000`) |
| 0x12 u16 | layer B baseY | |
| 0x14 u16 | layer B wrap | |
| 0x18 i32 | layer B paraY | |

The horizontal scroll per layer: `s = ((cam+0x60 + cam+0x34)/100 * factorX)/1000`;
`col0 = s/0x50`, `xoff = -(s % 0x50)`; tiles laid left→right at `xoff + i*0x50`,
`frame = (col0+i) % wrap`.  (Same camera fields as the tilemap projector
`0x490b90`/`map_render_visible_window`.)

**The descriptor is written by the `0x587e00` PROLOGUE** (`587e00.c:104-196`, the
currently-deferred front-header block — `port-debt.md` `ingame-nontile-layers`).
Lines 104-109 zero the parallax fields by default; then a **switch on `param_2`**
(`= room[0x44]`) with `param_3` (`= room[0x43]`, normalised 10/0x28→1, 0x32→2,
0x3c→4, 0x3d→5, else→0) selects the per-room banks.  Cases observed:
- **case 1,5,6,8,10,0xd,0xe,0x12**: A=`0x4e/0x4f/0x50` (by param_3), C bank=`0x57`
  wrap=8 baseY=`0xa0`; no B layer.
- **case 4,9**: A=`0x55/0x56`, C=`0x58` wrap=8 baseY=`0xf8` paraY=`0xfa`,
  B=`0x59` wrap=8 baseY=`0xe0`.
- **case 0xb,0xc,0xf,0x10**: A=`0x55/0x56`, C=`0x5a` … baseY=`0xc8`, B=`0x5b` …
  baseY=`0xd0`.
- **case 0x11**: A=`0x51/0x52` only.  **case 0x13**: A=`0x53`, C=`0x54`.
  **case 3 / default**: sets `in_ECX[0xe]=1` (→ `0x48c150` sees `in_ECX[7]!=0`,
  the special branch) + no parallax.

**The TOWN (room 210110, area `0xd2`).**  `FUN_00585000` fills `room[0x44] = area
A`, `room[0x43] = area C` from the AREA table; area `0xd2` = **A=4, C=1**.  So
`param_2 = 4` → **case 4**, `param_3 = 1` → normalised **0** → A-variant `0x55`.
**Town parallax descriptor:**

| layer | bank | baseY | wrap | paraX | paraY |
|-------|-----:|------:|-----:|------:|------:|
| A | `0x55` | 0 | (8) | — | — |
| C | `0x58` | `0xf8` | 8 | 0.5 | `0xfa` (=250) |
| B | `0x59` | `0xe0` | 8 | 0.25 | 0 |

(Static, two-witness RE; not yet live-probed — see the harness note below.)

**Live-probe — LANDED, descriptor + blit stream confirmed bit-exact (ckpt 66).**
Added a `--parallax-probe` to `frida_capture.py`/the agent (hooks `0x490cd0` + the
inner `0x417c40`/`0x5b9a40`, dumps the descriptor + per-tile blits; reports whether
ECX == grid).  The first probe runs fired **zero** times — a frame-capture
diagnostic showed the saved retail input-trace **no longer navigated** (retail sat
on the title; the menu became interactive ~150 flips later than the old trace
assumed, so its `Start@615` was eaten).  **Re-synthesised a working trace** (spam
confirm across the title-interactive window 600..760 → new-game; down×2 + confirm →
prologue; Z-beats → in-game): VERIFIED `newgame_enter@750`, `prologue_enter@945`,
`game_enter@1433` (`tests/scenarios/in-game-intro/trace-retail.jsonl`).  With it the
probe fires from `game_enter@1433`, and the live descriptor is an **exact match** of
the static RE: `ecx == grid`, `raw32 = 55 00 · 58 00 f8 00 08 00 · 00 00 fa 00 · 59
00 e0 00 08 00 · 00 00 00 00` → A=`0x55`, C=`0x58`/baseY`0xf8`/wrap`8`/paraY`0xfa`,
B=`0x59`/baseY`0xe0`/wrap`8`/paraY`0`.  The live **blit stream also matches the
port's `parallax_render` byte-for-byte**: layer A frames 0-7 @ x=0..560 y=0; layer
B col0=4 (frames 4,5,6,7,0,…) y=224; layer C frames 0-7 @ **y=220** (= baseY 248 +
the clamped −28 vertical-parallax — exactly `test_parallax_render_town_first_frame`).
So the parallax is **data-1:1 at the producer**, and the first-frame camera
`MAP_RENDER_CAM_TOWN_3F2` is confirmed (col0=4 for B at factor 0.25 ⇒ scroll-x
128000).  (The in-game retail harness drive is restored as a side effect — the
re-synthesised trace is the new `trace-retail.jsonl`.)

**PORT PLAN.**  (1) the prologue's parallax-field writes (the `param_2`/`param_3`
case → the 9 grid-header fields) — extend `map_decode`/`map_grid` (retire part of
PORT-DEBT `ingame-nontile-layers`); (2) the shared renderer (`0x490cd0`/`0x499560`
math) as a pure unit reading the descriptor + camera, blitting via a callback
(the `mr_sprite_fn` pattern) — drawn in `game_render` **before** the tilemap walk
(behind it); (3) host tests; (4) capture + compare the sky/mountain vs golden
1800.

## Open questions for the port

- ~~**The establishing-shot zoom**~~ **RESOLVED (ckpt 67): there is NO zoom — it
  is a leftward PAN at constant 1:1 scale.**  Live-probed across the whole shot
  (flips 1440-2100, new trace): the camera viewport `+0x64`/`+0x68` and the
  shear `+0x74` are **constant** (`64000`/`48000`/`0`); only `+0x60` (x scroll
  origin) animates — `128000` hold through ~1600, then panning left ~−147/flip
  to `59450` by 2100.  The per-frame render path is the **free-roam** one
  (`0x490cd0` fires every frame; the offscreen/special path `0x499100`/`0x48c6b0`
  **never** fires), and the projector `0x490b90` has no scale term — so the
  engine cannot zoom via the gameplay render.  Confirmed at the pixel level: the
  port's static `MAP_RENDER_CAM_TOWN_3F2` render aligns with the retail hold
  golden at **dx=0**, same ~64px wall pitch.  PORT-DEBT `ingame-establishing-zoom`
  is retired (the camera animation is the pan, already tracked by
  `ingame-camera-snap`).  See "The in-game COLOR-GRADE LUT" below.
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

## The in-game COLOR-GRADE LUT — the "darker + more saturated" post (ckpt 67)

Driving the nav trace and diffing the port's in-game town against a *fresh*
new-trace retail hold golden (both at the hold camera `+0x60=128000`) revealed
the dominant residual is **colour**, not geometry: every band differed ~99% at
the RGB level even where the backdrop tiles aligned at dx=0, with the port
uniformly **brighter** (content mean 134 vs retail 101).  The user confirmed
from the feed: retail's sprites are *darker and more saturated* — a LUT/post.

**It is NOT the per-sprite palette tint.**  `FUN_00417c40`'s tint switch keys
on `DAT_008a93fc` (= `in_ECX+0x289c`, since `in_ECX = &DAT_008a760c-0xaac`);
live-probed `DAT_008a93fc == 0` across the town intro → case-0 identity, no tint.

**It IS a 256-entry per-channel tone-curve LUT** at `DAT_008a9410`
(`= in_ECX+0x28b0`), applied to every sprite palette's R/G/B bytes.  Both
in-game render paths apply it; the title/menu/prologue paths (plain getter
`0x418470`) do not — which is why those stay bit-exact without it:
- `FUN_00417c40` (the palette-aware getter, used by the parallax producer
  `FUN_00490cd0`) — `417c40.c:259-273`.
- `FUN_00490f30` (the tilemap walk) — applies the same LUT inline,
  `490f30.c:75-213`.
Both gate on `DAT_008a9510 != 0 || DAT_008a9514 != 1000`; live-probed
`DAT_008a9510 = 700`, `DAT_008a9514 = 850` (the in-game defaults) → armed.

**The builder** (`FUN_00562ea0 @ 0x5639fd-0x563a70`, the boot init; the
decompiled FPU was garbled, so read from the raw asm + `.rdata` doubles).  Two
config gates drive it: `gate1 = *(*DAT_008a6e80 + 0x130)` (700),
`gate2 = *(*DAT_008a6e80 + 0x14c)` (850).  For each `i` in 0..255:
```
q      = (i * gate2) / 1000;                              // integer divide
accum  = ( (1000-gate1)*q + (1.0 - cos(q*PI/255))*127.5*gate1 ) * 0.001;
LUT[i] = (uint8_t) min(255.0, accum);                    // x87 ftol = truncate
```
(`.rdata`: PI@0x5cc288, 1/255@0x850dd0, 1.0@0x850dc8, 127.5@0x850dc0,
0.001@0x850d98, 255.0@0x850db8.)  A concave curve: crushes shadows/mids,
preserves highlights = darker + higher contrast/saturation.  `gate1=0 &&
gate2=1000` → identity.  **Verified bit-exact** vs a live probe of
`DAT_008a9410`: `LUT[64]=35`, `LUT[128]=100`, `LUT[192]=175` (and `LUT[0]=0`,
`LUT[255]=233`).

**PORT (ckpt 67): `src/color_grade.{c,h}`** — `color_grade_build_lut`
(the builder, host-tested vs the live samples) + `color_grade_apply_palette`
(the `0x417c40` per-channel RGBQUAD remap) + `color_grade_is_active` (the gate).
Wired in `main.c`: `enter_game` arms the grade (LUT built for 700/850) BEFORE
the town banks decode; the per-sheet conversion hook `title_sheet_format` applies
it to each **8bpp** sheet's palette before the 16bpp pack — matching retail's
order (LUT the palette, *then* 565-quantise), so the result is bit-exact, not
LUT-after-565.  Scoped: the title sheets are converted before `enter_game`, so
they keep `g_color_grade_on==0` and stay bit-exact; only the lazily-decoded
in-game banks are graded.  **Result: the backdrop TILES are now `differ_px==0`
vs retail** (e.g. the half-timber wall `(173,170,140)` and the ivy
`(107,105,74)` match exactly).

**The 24bpp parallax grade — LANDED (ckpt 68).**  The sky/mountain banks
(`0x55`/`0x58`/`0x59`) decode as **24bpp** (no palette), so the 8bpp palette
grade (`title_sheet_format`) can't reach them.  Retail does NOT grade 24bpp via
the palette path — `0x417c40` early-exits to the plain getter when the bank has
no palette (`this->entries->a != 0`).  Instead retail grades 24bpp banks **at
DECODE**: `0x417c40`'s **flag-3 branch** (`*(info+4)==3`, the 24bpp case) stamps
the slot's brightness descriptor — `f_08=1` (the decode-pass gate), per-channel
scales `f_0c/f_10/f_14` (tint case 0 → 1000, the town's `DAT_008a93fc==0`
identity), and `f_18 = in_ECX+0x28b0` (the tone-curve LUT base, set iff the gate
`0x29b0!=0 || 0x29b4!=1000` is armed) — then calls the getter, whose lazy
`ar_sprite_decode` runs **`ar_sheet_decode_pixels`** (already ported, engine-quirk
#46): `p = lut[p]` per non-key channel, then `p = p*scale/1000`.  The port's
parallax sink used the plain getter, so it never stamped these fields and the sky
decoded raw.  **Fix:** `game_arm_parallax_grade()` in `main.c` replicates the
flag-3 stamp before the getter in `game_parallax_blit`.  Verified: the port's raw
sky `(66,150,255)` → `lut` → 565-pack = **`(33,125,239)`**, and the blue channel
**`239` matches retail's main sky band exactly**.  (NB the old finding's raw
`(132,186,255)` / retail `(103,165,231)` numbers were wrong — the actual decoded
raw is `(66,150,255)`; both halves of the residual collapse to the single missing
LUT stamp.)

**Grade USER-CONFIRMED correct (ckpt 68).**  Pushed the graded sky to the feed;
the user confirmed "the grade looks correct".  The 24bpp main-band colour is
matched (blue `239` == retail) and the per-channel LUT path is the right one.

**OPEN (deferred) — the establishing-scene sky GRADIENT.**  The user spotted a
"dark gradient at the top" in one retail frame but *not* in another; the working
hypothesis is that it's a **per-scene CINEMATIC effect tied to the establishing
shot** (the intro pan/hold), NOT the flat per-channel LUT and NOT a constant
overlay — it appears in the establishing-pan frame and is absent once gameplay
settles.  To be confirmed by **probing ground truth** on the establishing cinematic
(the same RE that lands the intro PAN, `ingame-camera-snap`): hook the sky draw +
any per-row/per-scene modulation across the pan, vs decoding raw bank `0x55` to
rule the texture in/out.  Until then the flat LUT grade is the correct base; the
cinematic gradient is an additive scene effect to layer on later.  A row-for-row
sky `differ_px==0` also waits on the pan so port + retail share a camera.  The
other residuals are still-missing content: the NPC actors, the foreground tree, and
the "Town of Tonkiness" banner (`0x5a00c0`).

**Harness note:** `tools/flow/retail_fields.json` gained `tint` (`0x8a93fc`),
`lutgate1/2` (`0x8a9510`/`0x8a9514`), and four LUT samples (`0x8a9410`+) at the
Flip — the field-spec probes that pinned all of the above.

## The intro PAN — the camera-scroll model (ckpt 69)

**Ground truth (retail, `--seed-pin`, the camera field-spec probe across the
establishing shot).** Extended `tools/flow/retail_fields.json` with the camera
**target** + **speed** fields (`cam_x1c`=+0x1c, `cam_x20`=+0x20, `cam_x6c`=+0x6c,
`cam_x70`=+0x70) and drove retail from `game_enter@1434` through flip 3600
(`/tmp/campan*`). The establishing shot is a **scripted leftward pan**, NOT a
leader-follow:

| phase | flips | `+0x60` (cur x) | `+0x6c` (tgt x) | `+0x20` (speed) | `+0x5c`/`+0x70` (y) |
|-------|-------|-----------------|-----------------|-----------------|---------------------|
| **HOLD** | 1434–~1617 | 128000 (40 cells) | 128000 | 0 | 12800 / 12800 |
| **PAN**  | ~1617–~2450 | 128000 → 12800 (eases) | **12800** (4 cells) | **300** | 12800 / 12800 |
| **SETTLED** | ~2450–3600+ | 12800 | 12800 | 300 | 12800 / 12800 |

Key facts, all from the probe:
- **The Y camera never moves** — `+0x5c` = `+0x70` = **12800** the whole time
  (and `+0x34`/`+0x4c`/`+0x74` stay 0). The pan is purely horizontal. So the
  port and retail already share the **same sky rows** at every flip; only the
  visible sky *columns* differ during the pan.
- **The target is a FIXED value (12800), set once** at the hold→pan transition
  (`+0x6c` flips 128000→12800 between flip 1615 and 1620, simultaneously with
  `+0x20` 0→300). Because the target is constant (not tracking a moving entity),
  the pan is **portable without the entity/spawn system** — unlike the actor
  renderers. 12800 = 4 cells = the left edge of the 88-wide town (the
  spawn-snap value `MAP_RENDER_CAM_TOWN_3F2` already uses for Y).
- **The ease is velocity-integrating, accelerate-then-cruise** (NOT a
  proportional ease — the cruise velocity is constant, it does not decelerate
  near the target). Per-flip |Δ+0x60| ramps 6→30→60→90→120→144→**147** over
  ~70 flips (flip 1617→1690), then **cruises at exactly −147/flip**
  (1700→1800 = −14700 over 100 flips, robust across runs), then **clamps at the
  target 12800** (no smooth decel — the approach just saturates at `cur==tgt`).
  Cruise maxvel ≈ speed/2 = 150 (the −147 vs 150 gap is flip↔sim-tick averaging:
  the camera steps per sim-tick, flips advance ~1 per 2 ticks, so the exact
  per-tick step must be read off the function, not curve-fit — CLAUDE.md bar).

**The camera object fields (refined from `586010`/`439690`/`58e6a0`).** The view
= `*(DAT_008a9b50 + 0x104c)` (`operator_new(0x78)`), modelled as `int *`:

| off | idx | role |
|-----|-----|------|
| +0x00/+0x04 | [0]/[1] | map pixel extent (dim0·0xc80 / dim1·0xc80) — the ease clamp ceiling |
| +0x08/+0x0c | [2]/[3] | camera velocity x/y? (zeroed on snap, `439690:619`) — to confirm |
| +0x10/+0x14 | [4]/[5] | look-at target WORLD pos (set by `58e6a0`/`439690` centering) |
| +0x18 | [6] | centering mode flag (1=active) |
| +0x1c | [7] | pan flag (`cam_x1c`; 0 across this pan) |
| +0x20 | [8] | pan SPEED (`cam_x20`; 300 here, 800 in `58e6a0`) |
| +0x34/+0x4c/+0x74 | | projector sub-pixel accumulators (0 across this pan) |
| +0x5c/+0x60 | [0x17]/[0x18] | **current** scroll y/x |
| +0x64/+0x68 | [0x19]/[0x1a] | viewport extent (64000/48000) |
| +0x6c/+0x70 | [0x1b]/[0x1c] | **target** scroll x/y (clamped to `[0, ext-vp]`) |

The **target-setters** are RE'd (clamp to `[0, mapext-viewport]`, then set
`+0x6c/+0x70`/`+0x20` speed/`+0x1c` flag; optionally snap `+0x60=+0x6c`):
`439690:599-642` (snap cmd `param+0x40` / pan cmd `param+0x4c`), `58e6a0`
(look-at-actor, speed 800). **The per-frame EASER** (steps `+0x60` toward `+0x6c`
by the accelerating velocity) is **NOT statically locatable** — no function in
the by-address corpus writes `+0x60`/`view[0x18]` with a gradual RHS (all writes
are `=0`, `=+0x6c` snap, or other objects). It is reached via indirect dispatch
or dropped by Ghidra → the **`mem_watch` use-case**, but the target is a HEAP
address (`*(0x8a9b50)+0x104c+0x60`), so it needs a chain-resolving watch (a
small extension to `tools/mem_watch.py`, which today takes only static VAs).

**OPEN (PORT-DEBT `ingame-camera-pan`, supersedes the pan half of
`ingame-camera-snap`):** locate the easer function (heap mem-watch) → port it
verbatim for a bit-exact per-tick `+0x60` trajectory; and find the scripted op
that sets `+0x6c=12800`/`+0x20=300` at hold-end (~183 flips after `game_enter`).
Until then the port holds at `MAP_RENDER_CAM_TOWN_3F2` (cam x=128000). NB the
**settled** camera (x=12800, y=12800) is a *static* camera the engine genuinely
holds from ~flip 2450 — a second static `MAP_RENDER_CAM_TOWN_3F2_SETTLED`
constant enables a flip-anchored backdrop/sky diff at the settled end **without**
the easer (both sides share the full camera there).

### The camera EASER located + formula (ckpt 69, HW watchpoint)

The per-frame camera-follow easer is **`FUN_0043d1d0`** (366 B) — found with a
**hardware watchpoint** on the resolved heap address `*(*(0x8a9b50)+0x104c)+0x60`
(`tools/mem_watch.py --watch-chain … --hw`; frida-17 per-thread
`Process.getCurrentThread().setHardwareWatchpoint`). It was invisible to static
grep because it is dispatched through a **heap function pointer** (the captured
caller frame is a heap VA, not a code VA). The single writer instruction is
`0x43d26d`; the captured value trajectory `127970 → 12800` (target) and the
**per-tick deltas 30,40,50,…,290,300 then capped at 300** pin the formula
bit-exactly. Called once per frame from `0x439690:1123` (`FUN_0043d1d0()` then
`FUN_00499ab0()` the shake/HUD).

**`FUN_0043d1d0` (the easer), `in_ECX` = the view object (`int *`):**
```
// per axis: X uses [0x1b]=tgt(+0x6c) [0x18]=cur(+0x60) [2]=vel(+0x08)
//           Y uses [0x1c]=tgt(+0x70) [0x17]=cur(+0x5c) [3]=vel(+0x0c)
// [7]=flag(+0x1c)  [8]=vel CAP(+0x20, =300 for the town pan)
dist = |tgt - cur|
if (vel < dist) {                       // still approaching
    extra = 0;
    if (flag != 0 && dist > 16000) extra = (dist - 16000) / 10;  // far-boost
    cur += (cur < tgt ? +1 : -1) * (vel + extra);
    vel  = min(vel + 10, cap);          // ACCELERATE (+10/tick, cap +0x20)
} else {                                // within one step of target
    cur  = tgt;                         // SNAP onto target
    vel  = max(vel - 10, 0);            // DECELERATE
}
// then FUN_0043d340(view+0x24, cur_x, mapw-vpw) / (view+0x3c, cur_y, maph-vph)
```
The town pan has **flag (`+0x1c`) == 0**, so the far-boost arm is NOT taken — it
is the plain `vel += 10` (cap 300) ramp the capture shows. vel/cur start at 0/spawn;
when the scripted op sets `tgt=12800`/`cap(+0x20)=300`, the ramp runs to target then
holds (cur==tgt each frame, vel decays to 0). **`FUN_0043d340`** (299 B) is the
screen-shake sub-applier (writes the projector accumulator `+0x34`/`+0x4c` from a
`{active,duration,counter,phase,out,amp}` shake block + a 0..1000 envelope, clamped to
`[−ext, mapext−vp−cur]`); **inert during the pan** (`*param_1==0` → out=0), which is why
`cam_x34`/`cam_x4c` probe 0 throughout.

**Still PORT-DEBT `ingame-camera-pan`:** the scripted op that SETS `tgt=12800`/`cap=300`
at hold-end (~flip 1617, ~183 flips after `game_enter`) — a one-time write, not per-frame
(target is constant across the pan), so portable as a hold-timer + target set. The easer
formula itself is now fully known + bit-exact-validatable against the captured trajectory.

**Harness win (reusable):** `mem_watch.py` now resolves **chain heap addresses**
(`--watch-chain ROOTVA:HOPS:OFF:SIZE[:LABEL[:ARM_AT_FLIP]]`) and supports a **`--hw`
hardware watchpoint** — the fitting tool for a hot heap field (zero neighbour overhead,
no MemoryAccessMonitor livelock). The `--hw` backtrace names the caller chain.

### The camera is WIRED LIVE + the scripted target-setters ported (ckpt 70)

The easer (ckpt 69) is now **driven by a live camera in `main.c`**, and the two
camera-command arms of the in-game command processor `FUN_00439690` are ported:

- **The target-setters (`0x439690:599-664`).** Two command slots on the scene
  state: the **SNAP** command (`param+0x40`, `:599-642`) clamps the requested
  target to `[0, map_w−vp_w] × [0, map_h−vp_h]`, sets `cap`(`+0x20`)=0 /
  `flag`(`+0x1c`)=0, then **jumps** `cur`(`+0x60`/`+0x5c`)=`tgt` and zeroes
  `vel`(`+0x08`/`+0x0c`) — the room-entry spawn positioning. The **PAN** command
  (`param+0x4c`, `:643-664`) clamps + sets the target and `cap`=speed/`flag`=0 but
  **leaves cur/vel**, so the easer eases there. Ported as
  `camera_apply_snap`/`camera_apply_pan` (`src/camera_follow.c`), host-tested
  bit-exact (clamp, snap-jumps-cur, pan-keeps-cur, full pan lands on 12800).
- **The live wiring (`main.c`).** A static `camera_view g_game_camera`:
  `enter_game` sets `map_w/h` from the loaded map (`dim·0xc80`) + the 640×480
  viewport, then `camera_apply_snap(128000, 12800)` (the spawn origin =
  `MAP_RENDER_CAM_TOWN_3F2`). `game_render` calls `game_camera_step` each frame
  (the `CALL_TRACE_BEGIN(0x43d1d0)` mirror → `camera_follow_step` →
  `game_camera_to_mr` projects the view onto the `mr_camera` subset the backdrop
  walk/parallax read) and renders the backdrop through the *current* scroll
  instead of the static const. A hold timer (`GAME_CAMERA_HOLD_FRAMES=183`) fires
  `camera_apply_pan(12800, 12800, 300)` at hold-end. **Visually confirmed on the
  feed:** the town pans left from the hold (cam x=128000) through mid-pan to the
  settled town-left-edge view (cam x=12800).
- **Two synthetic stand-ins remain (PORT-DEBT `ingame-camera-pan`):** the pan
  **trigger** (the hold timer stands in for the cutscene-script op in the
  unported `0x5a00c0` that writes the `+0x4c` command), and the easer step
  **cadence/phase**. Both were then MEASURED from ground truth (next section) and
  the port adjusted to match. The **settled** end is a determinate static camera
  both sides share (`MAP_RENDER_CAM_TOWN_3F2_SETTLED`, x=y=12800) → a flip-anchored
  full-frame diff is meaningful there regardless.

### The pan CADENCE + TRIGGER measured; the port matches retail's trajectory (ckpt 70b)

A retail field-spec trace (`--seed-pin --lockstep --no-turbo`, the easer
`0x43d1d0` + the Flip `0x5b8fc0` hooked, a contiguous Flip whitelist across the
pan) pinned the two stand-ins to ground truth:

- **TRIGGER = `game_enter + 184` Flips.** Flip 1616 is still HOLD
  (`cam_x6c`=128000, `cam_x20`=0); Flip 1617 = PAN (`cam_x6c`=12800,
  `cam_x20`=300). `game_enter@1433` → `1617 − 1433 = 184`. The target snaps to a
  fixed 12800 and the speed to 300 in a single Flip (one-time write, the
  `0x439690` `+0x4c` command — the unported cutscene script issues it).
- **CADENCE = the easer fires once per 2 Flips.** The in-game sim update
  (`0x439690`, which calls the easer at `:1123`) runs at HALF the Flip rate: the
  `0x43d1d0` events land on every 2nd Flip, and the rendered `cam_x60` is a STEP
  function — flat between sim ticks (e.g. 127990 held across Flips 1618-1621),
  dropping 300 per 2 Flips at cruise (= 150/flip avg, matching the −147/flip the
  ckpt-69 probe saw). The port presents once per frame, so `game_camera_step`
  now gates the sim to **every 2nd frame** (`hold & 1`), phase-aligned so the
  trigger Flip is also a sim tick (`GAME_CAMERA_HOLD_FRAMES`=184, even).
- **RESULT: the port passes through the IDENTICAL `cam_x60` sequence as retail** —
  128000, 127990, 127970, 127940, 127900, 127850, 127790, 127720, 127640, 127550,
  127450, 127340, … then cruise −300/2flips (verified by capturing the port's
  `0x43d1d0` mirror + diffing the distinct-step sequence: identical where both
  are sampled). So at any Flip where the two share a `cam_x60`, the backdrop is
  pixel-identical — the pan is now trajectory-1:1, not just rate-correct.
- **RESIDUAL (irreducible without the sim-clock port):** retail's sim accumulator
  is wall-clock-paced, so its startup has sub-tick JITTER — a 4-Flip plateau at
  1618-1621, a double-tick at 1616 — that a clean fixed 2:1 step doesn't
  reproduce. This leaves a ~2-3 Flip PHASE offset vs retail during the pan (≤1
  step = 300 units = ~3 px horizontal, transient; zero at the hold + settled
  ends). PORT-DEBT `ingame-camera-pan` narrows to: the wall-clock sim-tick
  accumulator (the engine pace clock, map-object `+0x4068`) + the cutscene-script
  trigger SOURCE — both downstream of the in-game sim/`0x5a00c0` port.

## Timestep determinism — the Flip index is non-deterministic; the SIM TICK is the only valid frame-of-reference (2026-06-06)

**Context.** The user pushed back on the ckpt-70b "backdrop Δ0, residual is just
missing layers" framing: in the establishing-pan diff the *whole half-timber
house* (and every rendered foreground tile) shows a ~3 px horizontal **trail**,
while the sky + mountains are Δ0.  Cross-correlating the montage confirmed it:
the house aligns at **dx≈+3**, the background at **dx≈0**.  A pure camera-scroll
error would shift the 0.5×/0.25× parallax mountains by ~1.5/0.75 px — they are
flat 0 — so the camera value matches and only the 1× foreground is off.  That is
the signature of **frame-misalignment**, not a placement bug (the foreground
math `0x490b90`/`0x490f30` is provably identical to retail at equal `cam_x60`).

**Root cause (RE).** The in-game scene driver `FUN_00439690` is **one logical
sim tick per outer iteration**: it renders + **Flips a VARIABLE number of times**
inside a GetTickCount-gated frame limiter (`439690:776-859` — a 3-state machine,
16 ms quantum `local_270=0x10`, re-entered each tick via the
`switchD_0043b615_caseD_5` label so the budget state persists), then steps the
camera easer `FUN_0043d1d0` **exactly once** (`:1123`).  So the **present rate is
decoupled from the sim rate** (~2 Flips per tick avg; the camera is a STEP
function, flat between ticks) and the per-tick Flip count is set by wall-clock
time, not a frame counter.

**Measurement (`--seed-pin --lockstep --no-turbo`, easer + Flip `0x5b8fc0`
hooked, contiguous whitelist).**  Two **identical** retail runs:
- by **Flip index**: first cam step at flip 1619 vs 1616; plateau (a 4-Flip-long
  tick) count/location differ (1 vs 4); **57/121 common flips disagree** on
  `cam_x60` (up to 3 px).  `--lockstep-epsilon-ms 0` is WORSE (95/111) — the
  non-determinism is intrinsic to the wall-clock limiter, not the epsilon creep.
- by **sim tick** (Nth easer call): **bit-identical** — `cam_x60` is a pure
  function of the tick number (128000, 127990, …, −300/tick at cruise).

Visual proof (`tools/sim_tick_diff.py`, retail-vs-retail house crop): FLIP-matched
→ the 3 px house trail (identical to the port "bug"); SIM-STATE-matched → CLEAN
(sub-LSB).  Same engine vs itself.

**The frame-of-reference (the methodology fix).** Never anchor a diff (port↔retail
*or even* retail↔retail) on the Flip index.  Anchor on the **sim tick**:
- The agent now counts easer calls (`g_sim_tick`, `0x43d1d0`) and tags every
  captured frame (`frame_<flip>_t<simtick>.png` + `frames_manifest.jsonl`) and
  call-trace event (`sim_tick` field) with it.
- The counter is **reset at the `game_enter` scene-load anchor** so it is
  cross-run comparable despite the wall-clock-paced pre-game scenes (the
  "synchronize at every non-deterministic load" rule).
- For the pan specifically, `cam_x60` is bijective with the tick and is the
  offset-invariant key — `tools/sim_tick_diff.py --key cam` matches port↔retail
  frames at equal sim state and yields **dx=0** (vs `--key flip` → the 3 px
  trail).

Consequence for `ingame-camera-pan`: there is **no** way to match retail
flip-for-flip — it is not self-consistent run-to-run.  The port's clean fixed
2:1 cadence is fine; diffs just have to be taken at equal sim state.  See
engine-quirk #75 and [[timestep-determinism-pillar]].

## The actor animation cycle — a per-sim-tick stepper (ckpt 72)

Following the ckpt-71 decision (per-subsystem determinism anchoring; **next = RE
the NPC/actor system + its animation cycle, then pin its counter**), the actor
animation path is now reverse-engineered end to end.  The result *refines* the
#75-addendum hypothesis: the animation frame reads neither a global frame-tick
nor an RNG phase — it is a pure per-sim-tick stepper, so it rides the camera's
existing `g_sim_tick` clock with **no separate counter to pin**.

### The call chain (UPDATE pass, distinct from the render/emit pass)

| VA | role |
|----|------|
| `FUN_00439690:1108` | in-game loop → `FUN_0046cd70(1)` once per **sim tick** (when `*(param+0x1c)==0`) |
| `FUN_0046cd70` (1031 B) | per-tick actor-UPDATE master; walks the pools off `DAT_008a9b50` (active = `actor+0x1d0!=0`) |
| `FUN_0054f980` (11597 B) | per-actor behaviour dispatch on `actor+0x1d4`; **runs the inline frame-stepper** |
| `FUN_0040afe0` / `FUN_0041e600` | state-transition handlers; **set** the clip (`rstate+0x6c`) + reset timer/frame/done on change |
| `FUN_00491ae0` (2599 B) | the render/emit pass (READS the frame; quirk #76's third witness for the clip layout) |

`0x46cd70` is **not** `FUN_0048c150` — the latter is the render/emit walk (builds
the draw list, no anim advance).  The animation advances only in the update pass.
For the main actor band (`+0x11e0`, 0x80 slots) it calls
`FUN_0054f980(actor+0x40, actor+0x40, 0, 0)` for the primary render-state entry,
then `FUN_0054f980(entry-0x294, entry, 1, idx)` for each sub-entry (the kinematic
body-part chain — a sub-entry first inherits its parent's transform via the
copy at `0x54f980`'s head).

### The stepper + the clip descriptor (ported: `src/anim_clip.{c,h}`)

Every animating case in `0x54f980` runs the byte-identical idiom (see quirk #76
for the verbatim block): `timer++`; when `timer >= clip.frame_dur`, `frame++` and
`timer=0`; when `frame >= clip.frame_count`, either loop (`frame = clip.loop_to`)
or, for a one-shot clip (`clip.oneshot != 0`), freeze on the last frame
(`frame = count-1`, `done = 1`, `timer = 1`).

The clip `seq` is a fixed **0x154-byte, 32-frame** descriptor — confirmed by two
witnesses (the stepper's `+0x42`/`+0x44`/`+0x48`/`+0x152` reads and the renderer
`0x491ae0` case-0x1872d's `base + per-frame delta`, `per-frame x/y offset` reads):

| off | field | who reads it |
|-----|-------|--------------|
| +0x00 | `base_sprite` (i16) | renderer (`base + delta[f]`) |
| +0x02 | `frame_delta[32]` (i16) | renderer (per-frame sprite id) |
| +0x42 | `frame_count` (u16) | stepper (wrap test) |
| +0x44 | `frame_dur` (u16) | stepper (tick gate) |
| +0x48 | `oneshot` (i32) | stepper (0 = loop) |
| +0x50 | `off_x[32]` (i32) | renderer (per-frame draw x) |
| +0xd0 | `off_y[32]` (i32) | renderer (per-frame draw y) |
| +0x150 | `link` (u16) | renderer (next-clip) |
| +0x152 | `loop_to` (u16) | stepper (loop target) |

The anim STATE lives in the 0x294-byte render-state at `+0x6c` (clip ptr) / `+0x70`
(timer) / `+0x72` (frame) / `+0x74` (done).  `anim_state_set` mirrors the
transition handlers: reset only when the clip pointer changes, so re-asserting the
same state keeps the cycle running.  8 host tests pin the loop trajectory, the
one-shot hold, the duration gate, the NULL guard, the change-gated set, and the
`base+delta` sprite id.

### The determinism conclusion + the open residual

The counter `rstate[+0x70]/[+0x72]` is a pure function of *(sim-ticks since
clip-set)* — no GetTickCount, no Flip index, no RNG.  It is therefore **already
deterministic under the camera's `g_sim_tick` anchor** (game_enter reset); the
actor-anim subsystem needs no new pin.  This is the answer to the ckpt-71 task.

That leaves the #75-addendum's ~6.7k-px actor-band residual (two seed-pinned,
sim-tick-matched runs) attributed to a **different pillar**: the RNG-driven
behaviour (which clip plays / actor position).  `0x54f980`'s idle/wander cases
draw the LCG `FUN_005bf505` for random waits + spawn offsets, and the clip-SET
timing is downstream of those.  Annotated for the verification (`retail_fields.json`
`0x54f980` → `a0_clip`/`a0_timer`/`a0_frame`): a live capture across two
sim-tick-matched runs should show `a0_frame` matching while `a0_clip`/position
drifts — pinning the residual to the RNG/behaviour pillar (anchored separately).
See engine-quirk #76 and [[timestep-determinism-pillar]].

### ckpt 73 — the live check: the shared LCG stream is non-deterministic run-to-run even under `--seed-pin`

Ran that verification, and the mechanism is deeper than "the actor RNG desyncs":
the **shared** LCG stream `DAT_008a4f94` is itself non-deterministic run-to-run.
Drove retail twice (`--seed-pin --lockstep --no-turbo`, same trace), snapshotting
`DAT_008a4f94` at the per-sim-tick actor-update boundary `FUN_0046cd70` (new
`rng` field, tagged with `g_sim_tick`).  **`rng` matched 0 of 8643
in-game sim-ticks.**  The `a0_clip`/`a0_frame` fields matched 8643/8643, but
trivially — main-band slot 0 was inert (clip=0/frame=0) the whole run, so they were
not a real test; `rng` is the signal.

Proof the desync is a stream-phase drift, not just different flip counts to the
anchor: at `prologue_enter` BOTH runs are on the identical flip 946 yet the LCG
state differs (`0x84654e6f` vs `0xa79a2d6e`) — at the same flip the engine drew a
different *number* of values.  A per-present consumer × the non-deterministic
presents-per-tick count (quirk #75) shifts the phase; it never re-converges.

Consequence: the ~6.7k-px actor-band residual is the RNG pillar, and it is **not**
closable by the camera's `g_sim_tick` anchor.  An RNG-reading subsystem needs its
own RNG anchor — snapshot/restore `DAT_008a4f94` at the game_enter sim-tick on both
sides (or re-seed the actor RNG per tick).  Port↔retail bar for the band: data-1:1
given a matched RNG state.  Tool `tools/rng_tick_diff.py`; engine-quirk #77.

## The town ACTORS — render path RE'd LIVE + the spawn narrowed (ckpt 76)

The user directed: "implement the NPCs", "consult the runtime trace to track down
the code paths", "improve the trace tooling … pinpoint the code paths and document
them for future traces."  Did exactly that.  Result: the **render** side is fully
pinned + tractable; the **spawn** side is a data-driven entity subsystem (narrowed,
not yet ported).

### Tooling added (the durable foundation)
- **`thischain` field source** (`tools/frida/opensummoners-agent.js` `ctReadField`):
  like `chain` but **rooted at the `__thiscall` `this` (ECX)** instead of a global —
  start at ECX, follow each `hops` pointer hop, read typed at `off`.  Reaches a field
  *behind* a this-pointer (an actor's render-state at `*(actor+0x40)+off`).  The
  reusable primitive for probing any entity/actor by its live `this`.
- **Annotated** `0x491ae0` (actor render entry), `0x560e60` (actor reset →
  spawn-caller via `ret_va`), `0x584710` (candidate, refuted) in
  `tools/flow/retail_fields.json` — documented for every future trace.

### The actor walk (two passes, six bands, off `DAT_008a9b50`)
The per-frame **render/emit** driver `FUN_0048c150` (free-roam branch,
`in_ECX[7]==0`) and the per-sim-tick **update** driver `FUN_0046cd70` both walk the
same six actor-pool bands; a slot is live when `actor+0x1d0 != 0`:

| band off | slots | render emitter (0x48c150) | update (0x46cd70) |
|----------|------:|---------------------------|-------------------|
| **0x11e0** | **0x80 (128)** | **`FUN_00491ae0`** (the MAIN band — town NPCs/props/party) | `FUN_0054f980` |
| 0x1160 | 0x20 | `FUN_00493ba0` | `FUN_0054e5c0`/`478ba0`/`47b990` |
| 0x1060 | 0x40 | `FUN_004937c0` | `FUN_004710c0` |
| 0x13e0 | 0x400 | `FUN_00493480` | `FUN_0046e510` |
| 0x23e0 | 0x60 | `FUN_00492fc0` | `FUN_0046da00` |
| 0x2560 | 0x80 | `FUN_00493230` | — |

`0x491ae0` is `__thiscall` (ECX = the actor), called `FUN_00491ae0(sub_idx=0, view)`
once per live main-band slot.

### LIVE TRACE (retail, in-game town hold, flip 1500; `--seed-pin --lockstep --no-turbo`)
**33 active main-band actors.**  Their behaviour codes `actor+0x1d4` (the `0x491ae0`
switch key / `0x54f980` dispatch key):

| code | n | code | n | code | n |
|------|--:|------|--:|------|--:|
| 0x112e6 | 10 | 0x111d6 | 7 | 0x1129e | 3 |
| 0x112e2 | 2 | 0x11365 | 2 | 0x112e5/0x1129f/0x111d9/0x111f2/0x1136f/0x11366/0x11367/0x11370 | 1 each |
| **0x1872d** | **1** | | | | |

- **32/33 are STATIC** (render-state `+0x6c` clip == 0, frame 0).  **Exactly ONE is
  animated** (`0x1872d`, clip=`0x671c48`, frame 2, `+0x2c`=0x63 — the protagonist /
  key NPC at world (54400,32000)).
- **CRUCIAL: 32/33 codes are NOT explicit cases in `0x491ae0`'s switch** — they fall
  through to the **default arm `caseD_11257`**, which calls
  **`FUN_0044d160(render_state, &desc)`** then emits ONE node.  So **one function
  renders nearly every town actor.**  (The behaviour code drives the *update/AI* in
  `0x54f980`, NOT the render — static props render identically regardless.  RNG-driven
  motion stays deferred, ckpt 73.)  Only the animated arm `0x1872d` differs (it reads
  the clip per quirk #76).

### `FUN_0044d160` (379 B) — the static-actor descriptor builder (the linchpin)
ECX = actor, `param_1` = primary render-state (`*(actor+0x40)`), `param_2` = out desc.
- `dir = actor+0xe8` (u16) selects the per-direction **sprite table** at
  **`actor+0x48`, stride 0x14**: `+0x00` bank (u16, ==0 ⇒ skip), `+0x02` frame_base
  (s16), `+0x0c` x_off, `+0x10` y_off.  (So table row = `actor + 0x48 + dir*0x14`;
  `+0x4a`/`+0x54`/`+0x58` are that row's frame_base/xoff/yoff.)
- clip (`render_state+0x6c`): **0 ⇒ static** → desc off=(0,0), `sprite_delta=0`;
  non-0 ⇒ animated → off = `clip.off_x[frame]`/`off_y[frame]` (`+0x50`/`+0xd0`),
  `sprite_delta = clip.base + clip.frame_delta[frame]`, and `clip+0x150` may override dir.
- facing: if `render_state+0x2c == 3`, mirror via the flip table `&DAT_008a8440[bank]`.
- out desc (the 10-short element the emit tail reads): off_x, off_y, **bank**,
  **frame** (= `row.frame_base + facing + sprite_delta`), alpha=0.

### The emit (`0x491ae0` tail `LAB_004923eb`)
Per descriptor element: dst world = `render_state+0x04/+0x08`; offset
`(off_x+rs+0x40, off_y+rs+0x44)`; `FUN_004927c0(bank,frame,…)` marks tile occlusion
(the `grid+0x190030` buffer `0x48c150` pre-clears — **deferrable**); cel =
`FUN_00417c40(bank, frame)` (the palette-aware getter + the in-game LUT grade,
`color_grade`); then **`FUN_00492670`** emits the node.

**`FUN_00492670` (118 B)** = the actor analog of `draw_pool_emit` (`0x4917b0`):
writes `node = (*(view+0x54))[layer].array + count*0x3c` — `node[0]=cel`, `[1]/[2]`=
world x/y, `[3]/[4]`=dx/dy (= `param6`/`param7`), `[5]`=alpha, **`[6]=mode=bool(alpha)`**
(`param7!=0`).  `layer = actor+0xfc` (the live trace: layer 9-10).  So actors emit
into the SAME `view+0x54` draw_pool `map_present` walks, as **mode 0 (keyed, opaque,
`FUN_005b9b70`)** or **mode 1 (alpha, `FUN_005bd550`)** — exactly the deferred
PORT-DEBT `present-actor-modes`.

### Render OUTPUT (live, the 36 mode-0 keyed `0x5b9b70` blits at flip 1500)
res **0x403** (×4 villagers, frames 16/1/36), **0x426** (×5, frames 0-5),
0x459/0x462/0x46a/0x46b/0x472/0x47b/0x481/0x3fa — the town's villagers + props,
ckey `0xf81f` (magenta).  **0x403 + 0x426 are exactly the ckpt-75 render_diff
residual's named NPC banks** → these blits ARE the 36 leftover divergences.

### The town actor SPAWN — RE'd + byte-verified (ckpt 78)
The ckpt-76 narrowing ("a data-driven entity-by-id subsystem, NOT a single
function, via `0x587e00`'s layer pass") was the right *idea* but the wrong
functions.  The real chain is **`0x586010:698` → `FUN_0058d460` → `FUN_00431e30`**
— NOT `0x42eb20`/`0x4282f0`, NOT inside `0x587e00`.  Established by static read +
**proven against the map bytes** (no live drive needed):
`docs/proofs/map-object-layer-format.md`.

**`FUN_0058d460` — the room object-population pass** (3341 B; called from
`0x586010:698`, immediately after `FUN_00587970` parses the map + `FUN_00587e00`
decodes the tiles; `param_2` = that map-descriptor object).  It walks the map's
**`count` (=86) object-placement layers** — the `0x3c`-byte layer headers at
`mapobj+0x38` (the `puVar14` cursor, stride `0x3c`) paired with the layer
sub-pointer records at `mapobj+0x3c` (`local_268`, stride `0x10` = the four
sub-array pointers {a,b,c,d}).  The 0x3c header **is the object's placement
record**: `+0x04` x, `+0x08` y (both ×100 → world), **`+0x10` the type code**,
`+0x18` u16 sub-type; sub-array `a` (n_a=12 dwords) is the per-instance config
(mostly zero for the town props).  Each object is dispatched by the **range of its
type code** into one of four pre-allocated bands off `DAT_008a9b50` (a free-slot
scan, then a named `"<kind> Object Count Over"` abort if the band is full):

| type range | kind | band | spawn fn | DATA 1022 |
|------------|------|------|----------|----------:|
| 50000–59999 | EFFECT | `+0x1160` | `FUN_0041f200` | 15 |
| 60000–69999 | STRUCTURE | `+0x2560` | `FUN_00438a60` | 39 |
| **70000–79999** | **CHARACTER** | **`+0x11e0`** | **`FUN_00431e30`** | **32** |
| 80000–89999 | DEVICE | `+0x13e0` | `FUN_00557550` | 0 |

**`FUN_00431e30` — the character activator** (25293 B, `__thiscall` ECX = the free
`+0x11e0` slot `0x58d460` just found).  A giant per-type `switch(param_4 = type)`;
every case sets **`actor+0x1d0 = 1`** (active), **`actor+0x1d4 = type`** (the
behaviour code), **`actor+0xfc = 9`** (draw layer — matching the live trace's layer
9-10), `actor+0xe8 = 0` (dir), zeroes the 8-row `+0x48` sprite table, saves the
world (x,y) from params 1-2 into the render-state, and calls per-type init helpers
(`FUN_00426620` & the `0x4264xx–0x4273xx` cluster — the "entity" region ckpt-76
guessed at `0x42eb20`) that look up a **per-type entity-def table** (base +
`type*0x80 + 0x21c04`, stride 0xc, `426620:168`) to install the sprite/anim/
collision.  So the **appearance is keyed by the type code, NOT carried in the map
record** — the code→sprite-table mapping is the one remaining port input.

**The byte-level proof (resolves "codes never assigned as constants").**  The town
codes are not constants at all — they ARE the map's object type fields.  Decoding
`header[+0x10]` for DATA 1022's 86 layers
(`tools/extract/map_data.py … --objects`) yields exactly **32 CHARACTER objects**
whose codes + multiplicities are **identical** to the ckpt-76 live census
(0x112e6 ×10, 0x111d6 ×7, 0x1129e ×3, 0x112e2 ×2, 0x11365 ×2, and
0x112e5/0x1129f/0x111d9/0x111f2/0x1136f/0x11366/0x11367/0x11370 ×1), with their
world positions (e.g. layer[1] `0x111d6` @ (2144,416), layer[49] `0x112e6` @
(624,288)).  The 33rd live actor is the one animated NPC (`0x1872d` = 100141,
outside the character range → a separate spawn path).  The port's `map_data`
already parses these layers (`map_layer.hdr[0x3c]`), so the spawn's *inputs* (code,
x, y per object) are in hand.
- The 8 PARTY actors remain a separate set (`0x560e60`-reset at `0x59f578` inside
  `0x59f2c0`, `map+0x4030`) — unrelated to this room-object band.
- The band stays a PRE-ALLOCATED fixed pool (`0x586010:476-506`,
  `FUN_0058cf60(0x40)` ×128 → `+0x1d0=0`); `0x58d460` ACTIVATES a subset.  After it,
  `0x586010:707-718` walks the now-active `+0x11e0` slots and sets each one's clip
  (`FUN_0041e600`) — the static town props get clip 0 (no anim), the animated NPC
  its clip.
- **NEXT (the port input):** the **code → `+0x48` sprite table** mapping — RE the
  relevant `FUN_00431e30` cases + the `type*0x80+0x21c04` def table for the 13 town
  codes, OR capture each spawned slot's `+0x48` table live (hook `0x431e30` onLeave,
  dump `actor+0x48`).  Then a minimal spawn (read the 32 objects from `map_data`,
  fill render-state pos + sprite table + dir + layer) drives the ported renderer.

### The town actor RENDER CENSUS — only 6 of 33 draw (ckpt 79; corrects #78/#79)
The spawn's one missing datum was the code → `+0x48` sprite-table mapping.  Per
the methodology ("capture each slot's `+0x48` live"), I extended the `0x491ae0`
field spec with the `+0x48` table reads (`row0_bf` = bank|frame_base word, `d1_bf`
…`d7_bf` for dirs 1-7, `dir_e8`, alpha/skip/angle, the render-state dst-base +
layer-override) and ran a field-spec capture at the town hold (flip 1480/1500/1520,
`--seed-pin --lockstep --no-turbo`).  **The result overturns the "32 static
scenery actors" framing (#78):**

- **27 of the 33 active main-band actors are INVISIBLE** — their `+0x48` table is
  all-zero in every direction, so `FUN_0044d160` returns 0 (`bank==0`).  These are
  the `0x111d6`/`0x112e6`/`0x112e2`/`0x11365`/… codes — collision / trigger / spawn
  volumes (their `FUN_00431e30` arms build a physics body, never a sprite).
- **Only 6 draw**, all `dir==0` / `clip==0` (static) / `skip==0`.  Bank `0x16c`
  (res `0x403`) is the town-OBJECTS sheet — these are static **PROPS, not
  people-NPCs** (USER-confirmed against retail: the fountain + a barrel, at the
  correct positions).  The one PERSON is the animated protagonist `0x1872d`:

  | code | n | bank | frame_base | layer | what | world (x,y) at flip 1500 |
  |------|--:|------|-----------:|------:|------|--------------------------|
  | `0x1129e` | 3 | `0x16c` | 1  | 9  | prop (a barrel) | (88200,46400)(93000,46400)(126600,46400) |
  | `0x1129f` | 1 | `0x16c` | 2  | 9  | prop | (120000,46400) |
  | `0x112e5` | 1 | `0x16c` | 36 | 10 | prop (the fountain) | (176000,41600) |
  | `0x1872d` | 1 | `0x175` | 0  | 9  | **the protagonist** (animated) | (54400,32000) |

- **`0x426620` ZEROES `+0x48`; the table is filled LAZILY** (state-set
  `0x40afe0`/`0x41e600` from a type-keyed def table — the actual def table, not the
  `type*0x80+0x21c04` collision lookup #79 misnamed; that one is cell-indexed and
  writes `+0x288/+0x28c`).  The def table is **not yet RE'd**; captured ground truth
  stands in (PORT-DEBT `actor-sprite-table`).
- **The props sit at a DETERMINISTIC per-code offset from `map_x*100` — NOT RNG**
  (earlier draft wrongly called it "wander").  All three `0x1129e` share the exact
  same `+1800x/+1600y`; positions are identical across flips 1480/1500/1520 (static).
  The visible fountain `0x112e5` has `+0/+0` → it lands exactly at `map_x*100` and
  matches retail (USER-confirmed).  So the delta is a fixed placement/anchor nuance
  (the `0x426620` alignment arm or the lazy fill — TBD), reproducible; the port
  spawns at `map_x*100`.  Census artifact: `/tmp/actor_census.json` (regenerate via
  the field-spec capture).  Engine-quirk #80.

### Port plan (corrected by the census above)
1. **Spawn (DONE, ckpt 79):** `src/actor_spawn.c` — read the 32 CHARACTER objects
   from `map_data` (`0x58d460` filter: code 70000..79999), fill `{actor,
   render-state}` at `(x,y)*100`, dir 0 / layer 9 / static; the 3 visible prop codes
   get their dir-0 sprite row from the captured stand-in map, the other 29 stay
   invisible (bank 0).  Host-tested.  PORT-DEBT `actor-sprite-table`.
2. **Render (DONE, ckpt 77):** `actor_render_static` (`FUN_0044d160` + the
   `0x491ae0` default arm) draws a spawned static actor — `map_present` MODE 0 keyed.
3. **Wire (DONE, ckpt 79):** `town_render_step_ex` adds an actor seam (emit between
   the tile walk and present); `main.c game_actor_walk` walks `g_actors` →
   `actor_render_static`; `game_cel_dims` (cel `metric_b8/bc`) gives the mode-0 cull
   box; `game_present_blit`'s `PRESENT_KEYED` arm → `zdd_object_blt_keyed`.
   **LIVE-VERIFIED:** the port logs `5/32 actors emitted (bank 0x16c registered)`
   and the props render at the correct spots (USER-confirmed on the feed — fountain
   right, barrel left).  +1 host test (`town_render_actor_seam`).
4. **The protagonist (the BULK of the 36) is the NEXT arc:** `0x1872d` is OUTSIDE
   the CHARACTER range (a separate spawn) AND needs the `0x491ae0` `0x1872d`
   multi-part animated arm (reads the clip; reuses `anim_clip`).  Its body-part
   banks (`0x426`/`0x459`/…) are most of the 36 — the actual person.
5. **Verify:** `render_diff` vs retail flip 1500 keyed on `(res, frame)`; the camera
   (port hold vs retail panned) is a standing deferral, so the signal is the
   actor-blit IDENTITY appearing.

Engine-quirk #78/#79 (corrected by #80) + #80; PORT-DEBT `present-actor-modes`,
`actor-sprite-table`.

### The town ACTOR render side — PORTED (ckpt 77)

Ported the render half of the arc above (steps 1+2), pure + host-tested, ahead of
the spawn.  32/33 town actors render through the **default arm**, now in the port:

- **`draw_pool_emit_actor` = `FUN_00492670`** (`src/draw_pool.c`).  The actor analog
  of `draw_pool_emit` (`0x4917b0`): the SAME 0x3c draw node, but the node MODE is
  derived `= bool(alpha != 0)` (opaque → mode 0 keyed, translucent → mode 1 alpha)
  and `alpha` lands in the param8 slot; a NULL cel emits nothing (`492670.c:12`).
- **`actor_render.{c,h}` (NEW).**  `actor_render_describe` = **`FUN_0044d160`**
  bit-exact: the per-direction sprite-table row (`actor+0x48`, stride 0x14, 8 dirs,
  indexed by `actor+0xe8`) → the static / animated / mirrored / angle frame +
  placement offset, into the 10-short descriptor.  `actor_render_static` = the
  **`0x491ae0` default arm (`caseD_11257`)**: skip flag (`actor+0x284`), layer
  (`actor+0xfc`, overridden by the render-state's `+0x284` sub-object `+0x100`),
  describe, then `draw_pool_emit_actor`.  The actor + render-state are LOGICAL
  structs (the spawn fills them, like `anim_clip`'s `anim_state`); only
  `actor_sprite_row` (0x14) is a pinned data layout.
- **`map_present` MODE 0** (`src/map_present.c`).  The opaque-actor keyed path
  (`FUN_0048eac0` case 0): project like a tile (`map_present_project`), but the cull
  box comes from the CEL dims (`cel+0x1c/+0x20`) via a new `present_dims_fn`
  callback; a visible node → `PRESENT_KEYED` (`FUN_005b9b70`, whole-sprite
  color-keyed blit, no src rect).  `dims == NULL` keeps the tile-only contract
  (`town_render_step` passes NULL today → actor nodes deferred, never dropped).
  `present-actor-modes` narrowed to modes 1/2.

**Validation.**  The render-state offsets the port models (`world_x@+0x04`,
`world_y@+0x08`, `facing@+0x2c`, `clip@+0x6c`, `frame@+0x72`) match the ckpt-76
live-RE'd `0x491ae0` field spec exactly (`retail_fields.json`: `rs_x` off 4, `rs_y`
off 8, `rs_kind2c` off 44, `rs_clip` off 108, `rs_frame` off 114) — so the struct
modelling is already pinned to live retail data; the *logic* is host-tested
bit-exact vs the decompile (18 new tests, `test_actor_render.c` +
`test_draw_pool.c` + `test_map_present.c`).  883 pass.

**Still open (the next arc — needs the harness + then the human for pixel-verify):**
1. **The SPAWN** (the band-population activator) — the gating input.  Narrowed:
   it is NOT `0x560e60` (the 8 PARTY actors, `ret_va` in `0x59f2c0`) nor `0x584710`
   (refuted ckpt 76); the static candidates that write both `+0x1d0`+`+0x1d4`
   (`0x456a50` find-by-code, `0x487dc0` cell/collision) are NOT the activator
   either.  It is the **entity subsystem** (`0x42eb20`/`0x4282f0`/`0x429060`/the
   27 KB `0x41f200`) processing the **DATA 1022 layer entries** (86 of them, the
   room object list `map_data` already parses) via `FUN_00587e00`'s layer pass — a
   multi-function unit.  **The empirical pin (next):** the slots are pre-allocated
   inert (`0x58cf60` sets `+0x1d0=0`); `FUN_0058cf60` allocs each slot's
   render-state array (`param_1 * 0x294` entries — main band `0x40`=64 sub-entries
   per slot), it does NOT activate.  Slot 0 of the main band is INERT all run
   (ckpt 73), so a `mem_watch --hw` must target an ACTIVE slot's `+0x1d4` (only the
   activation writes it; `0x58cf60` never touches `+0x1d4`) — find an active slot
   index first (hook `0x491ae0`, log the ECX actor ptr, cross-ref the band array).
2. **The `0x1872d` animated arm** (1 actor, the key NPC) — a 3-element multi-part
   descriptor (`0x491ae0:112-192`); port it WITH the spawn so it can be pixel-verified.
3. **Wire** the actor band walk into `game_render` (between `map_render_walk` and
   `map_present`) + the Win32 mode-0 keyed sink + cel-dims callback; then
   `render_diff` vs retail flip 1500 (the 36-blit residual should drop).

### The 0x1872d SPAWN + the arrival WAGON (ckpt 80) — corrects "the protagonist"

Ported the `0x491ae0` case-`0x1872d` arm (`actor_render_protagonist`, a 3-cel
composite over the same `FUN_0044d160` body build the default arm uses — part 2
is byte-identical to `actor_render_describe`, wrapped with two fixed bank-`0x175`
cels at x-256 / x-128).  Then RE'd how `0x1872d` gets onto the `+0x11e0` band at
all — it is NOT a map object (its code is outside the 70000 CHARACTER range that
`0x58d460`/`actor_spawn_from_map` walk), so the census's "spawned with the 32
characters" framing was incomplete.  The real chain (static, two-witness):

- **`FUN_004d7d80` is the per-room cutscene/event SCRIPT.**  It early-outs unless
  the area is `0xd2` (210, the town) and then `switch`es on the room id
  (`**(DAT_008a9b50+0x1038)`).  **`case 0x334be`** (= 210110, the town) is the
  town intro script; on its first step (gated on event flags `0x5f76805` /
  `0x606aa4f` being unset) it spawns the scene and sets the camera hold
  (`in_ECX[0x11]=0x1f400`=128000 — the same hold the camera RE measured).
- **The spawn call:** `FUN_00431d10(0, 0x1872d, 0x65, 0x3200, 0, 0)`.
  `FUN_00431d10` is the **by-code main-band spawn helper**: scan
  `DAT_008a9b50+0x11e0` (0x80 slots) for a free slot (`+0x1d0==0`); if an anchor
  code is given (`param_3=0x65`), find the active slot whose `+0x274==0x65` and
  offset the spawn (x,y) by that anchor's render-state position (so `0x1872d` is
  placed RELATIVE to actor 0x65); then call `FUN_00431e30(x, y, 3, code=0x1872d,
  …, 99, …)`.
- **`FUN_00431e30` case-`0x1872d`** (the activator arm) installs, on the slot:
  layer 9 (`in_ECX[0x3f]`), zeroes the `+0x48` sprite table (8 rows × 0x14), runs
  `FUN_0041ee60(x,y,…)` to resolve the world position, fills `in_ECX[0x11]`
  render-state sub-entries (each `+0x2c` = `param_11` = 99), then:
  - **`FUN_00426db0(0, 0x175, 0, 1, 0, 0, 0)`** — installs sprite-table **row 0**:
    bank `0x175`, frame_base 0, x_off 0, mirror_x 0, y_off 0.  `FUN_00426db0(dir,
    bank, frame_base, b, x_off, mirror_x, y_off)` writes one `actor_sprite_row`
    (byte offsets +0x48/+0x4a/+0x4c/+0x54/+0x50/+0x58 — exactly the
    `actor_render.h` row layout).  **This RETIRES the ckpt-79 "lazy +0x48 fill not
    RE'd" unknown** for this actor.
  - render-state **clip = `&DAT_00671c48`** (resets frame/timer/done) — confirms
    the ckpt-72 clip pointer.
- **Siblings:** `0x1872e` (spawned from `FUN_00539e80`), `0x1872f` (from
  `FUN_005034b0`), `0x18730` (from inside `0x431e30` via `FUN_00556ef0`) are the
  same family — `0x431e30` has explicit case arms for all four.

**IT IS A HORSE-DRAWN WAGON, not a person (ckpt-80 LIVE, USER-confirmed).**  Wired
`actor_spawn_protagonist` + the `game_actor_walk` dispatch and drove the port
through the pan to the settled camera (cam 12800, frame 2200).  A with-`0x1872d`
vs no-`0x1872d` rebuild diff isolates exactly its pixels (bbox x180-543,
y195-383) — bank `0x175` (res `0x058f`) is a **covered WAGON / caravan**, NOT a
character.  So the census/ckpt-79 "the one PERSON / the protagonist" label is
**corrected — `0x1872d` is the town intro's arrival CARRIAGE** (USER-confirmed on
the feed: "the left caravan is correct and the position looks correct …
that looks correct").  The decoded bank `0x175` is the user's own asset (same
both sides) → ground truth.

**The 3-cel composite = wagon-left | wagon-body | HORSES.**  The first render
froze the body on frame_base 0, so its rightmost cel redrew the wagon's left cel
("the right wagon is cut in half" — USER).  The body is actually the **animated
HORSES** (USER: "the right part … is supposed to be horses").  Decoding the clip
**`&DAT_00671c48`** from the user's `sotes.exe` `.rdata` (file off `0x271c48`, for
analysis) gives: **base_sprite 2, frame_count 4, frame_dur 18, LOOPING,
frame_delta {0,1,2,3}** → the body cel cycles sprite frames **2..5** (the horses
trotting), zero per-frame offset.  `actor_spawn_protagonist` now points the
render-state clip at a reconstructed `WAGON_CLIP` (those 4 RE'd values — timing
metadata, not the asset), so the body draws sprite 2 = the horses.  Re-captured
+ USER-confirmed correct on the feed.

### The horses TROT — the per-tick anim wired (ckpt 81)

The ckpt-80 wagon was frozen on the clip's first frame.  Reading the per-actor
update `FUN_0054f980` case-`0x1872d` (`:911-970`) shows its two halves are cleanly
separable (engine-quirk #82):

- **`:911-928` is the frame-stepper, UNCONDITIONAL** (gated only on the clip
  `+0x6c != 0`) — byte-identical to `anim_clip_advance` (quirk #76), reads no
  RNG/clock.  So the wagon's body cel **always** cycles, deterministically.
- **`:929-970` is the behaviour**, which `break`s out entirely unless this is the
  primary entry AND the global scene-lock `*(DAT_008a9b50+0x27a8)==0`, then draws
  the LCG (`FUN_005bf505`) for idle waits / wander — the deferred RNG layer (#77).

So the trot is portable in isolation.  Driver `FUN_0046cd70:123-169` walks the
0x80-slot `+0x11e0` band once per sim-tick, calling `0x54f980` per active actor.

**PORTED:**
- `actor_render_state` gains the anim sub-block `timer` (+0x70) / `done` (+0x74)
  (it already had clip +0x6c / frame +0x72) — the slice the stepper writes IS an
  `anim_state`.
- `actor_anim_advance` (`actor_render.c`) — the per-actor stepper; a thin adapter
  that bridges the render-state's anim block to the single ported stepper
  `anim_clip_advance` (one source of truth, host-tested bit-exact, ckpt 72).
- `actor_pool_update` (`actor_spawn.c`) — the `0x46cd70` main-band walk: advance
  every active render-state with a clip (the 32 static actors no-op on clip NULL,
  only the wagon trots).  Returns the count advanced.
- `main.c game_actor_update` calls it on the SAME sim-tick gate as the camera
  easer (`(g_game_camera_hold & 1)==0`), BEFORE `camera_follow_step` — mirroring
  retail's `0x439690` per-tick body order (`0x46cd70`@:1108 then `0x43d1d0`@:1123).
  A `CALL_TRACE_BEGIN(0x46cd70)` port mirror emits the advanced count.  Reset is
  automatic: `enter_game` re-spawns the pool (frame/timer 0) + zeroes the hold.

**LIVE-VERIFIED (port blit trace, settled cam 12800, flips 2100-2244 spanning one
144-Flip clip cycle):** the wagon (bank `0x175`) renders as **3 keyed cels (res
`0x3ec`, NOT `0x058f` as ckpt-80 mis-noted)** at screen x 160/288/416 (the
−256/−128/0 composite); the body cel (x416) steps **5→2→3→4→5** (one body-frame
per 36 Flips = 18 sim-ticks) while the two fixed wagon cels hold frames 0/1, and
the `0x46cd70` mirror reports `advanced:1` each tick.  **USER-confirmed on the feed
(ckpt 81): "I can see the horses' ears animating slightly, looks correct.  The
wagon doesn't move — the horses are just idling, which is how it's supposed to be."**
So `WAGON_CLIP` is a SUBTLE IDLE loop (ear flicks), the wagon is PARKED at the
settled position, and "trot" above is shorthand for that idle cycle — no locomotion
at the hold (retail ground truth, quirk #82).  Host tests:
`actor_anim_advance_matches_stepper` / `_null_is_noop`, `actor_pool_update_trots`
(the body cel cycles 2→5, the static actor stays frozen).

**Still open / PORT-DEBT(actor-protagonist-clip), narrowed:** the RNG-driven
behaviour (`0x54f980:929+`, deferred ckpt 73).  The spawn pos is the settled census
const (54400,32000) — USER-confirmed correct as the PARKED position; whether the
caravan visibly ROLLS IN earlier in the intro (the cutscene's anchor-relative
`0x431d10(…, anchor 0x65, x 0x3200, …)` arrival) is a separate `0x4d7d80` question,
not observed at the settled hold.  Cross-check: `render_diff` vs a panned-camera
retail capture keyed on `(res 0x3ec, frame)`.

### The caravan "siblings" `0x1872e`/`0x1872f`/`0x18730` are OUT-OF-SCENE (ckpt 82)

The ckpt-80/81 handoff floated the code-adjacent actors `0x1872e`/`0x1872f`/
`0x18730` as the "same family … likely the CHARACTERS" and the next town chip.
**Statically refuted — none is spawned in the town intro (area 210 / room
210110 = `0x334be`).**  Code-adjacency of actor IDs does NOT imply same scene
(engine-quirk #83):

- **`0x1872e`** is spawned at `FUN_00539e80:2859` —
  `FUN_00431d10(0x35a4e930,0x1872e,0x66,0xffffb1e0,0,0)` — inside
  **`case 0x64280` (room 410240, area 410)**.  `0x539e80` is a per-room SCRIPT
  handler (sibling of the wagon's `0x4d7d80`) dispatched every frame from the
  story-FSM `FUN_0040c380` (gated on progress flag `0x5f76805 ∈ [1000,0xb55)`);
  it `switch`es on the live room id `**(DAT_008a9b50+0x1038)` and its cases are
  ALL area-410 rooms (`0x641fe`/`0x64212`/`0x6423a`/`0x64276`/`0x64280`).  None
  is the town `0x334be`, so it is inert in the town.
- **`0x1872f`** is spawned at `FUN_005034b0:964` —
  `FUN_00431d10(0,0x1872f,2,0xc80,0xffffda80,0)` — inside **`case 0x382de`
  (room 230110, area 230)**.  `0x5034b0`'s room cases are all area-230
  (`0x382de`/`0x382ed`/`0x382f2`/`0x38306`).
- **`0x18730`** is spawned as a CHILD of CHARACTER code **`0x11350`** (via
  `FUN_00556ef0` from `0x431e30`'s `case 0x11350`, `:2192`).  `0x11350` is NOT
  one of the town's 32 character map-objects (`map_data DATA-1022 --objects`:
  `0x112e6`×10, `0x111d6`×7, `0x1129e`×3, `0x112e2`×2, `0x11365`×2, +
  `0x112e5`/`0x11370`/`0x11366`/`0x11367`/`0x1136f`/`0x111f2`/`0x111d9`/`0x1129f`).
  So its parent never spawns in the town either.

Two further structural facts confirm it: (1) all four codes (100141-100144) are
OUTSIDE the 70000-79999 CHARACTER range, so the map-object pass
`0x58d460`→`actor_spawn_from_map` never makes any of them (same reason the wagon
`0x1872d` is script-spawned, ckpt 80); (2) the town cutscene script
`FUN_004d7d80` (cases `0x334be`…`0x33522`, all area 210) issues exactly ONE
`FUN_00431d10` call — the wagon `0x1872d` in room `0x334be`.  This matches the
ckpt-79 live census (33 active actors at the town hold = 32 map-objects + the
wagon; no siblings).  **Conclusion:** the siblings belong to later story beats in
other areas; they are not part of the town-intro parity arc and were dropped from
the NEXT list.

### The hold residual is the CHARACTER CAST + foreground tree, NOT a "banner" (ckpt 82, blit-trace ground truth)

Re-captured the retail blit trace + a PNG at the establishing hold
(`game_enter@1434`, cam 128000, the scene-LOCKED cinematic; flips 1450/1500/1600,
`--seed-pin --lockstep --no-turbo`, field-spec).  This **refines/corrects** the
ckpt-74 "the 36 residual = NPC actors + tree + banner" characterization with
exact producers + a visual:

- **108 `blt_keyed` (`0x5b9b70`) at the hold split into two producers:**
  - **54 via the present `FUN_0048eac0` (`ret_va 0x48ecc2`), 18/frame — THE
    VISIBLE residual** (the cast + tree).  Banks: `0x481` (1× **320×320** @ dst
    (496,64) = the **foreground TREE**, right edge), `0x426` (5× frame 5 @ y≈344-352
    = a row of similar townsfolk), `0x459`/`0x462`/`0x46a`/`0x46b`/`0x472`/`0x47b`
    (1× each, 25-50 × 53-90 px = distinct standing CHARACTERS), `0x403` (props),
    `0x3fa` (2× tiny 14×8 details).  The PNG confirms: a foreground tree on the
    right + ~5-7 townsfolk standing in the square + a flowerbed.
  - **54 via `FUN_004962a0` (`ret_va 0x49632a`/`0x49635c`/`0x49638b`, 3 stacked
    layers × 6 columns), all at dst **y=572 (BELOW the 480 screen → OFF-SCREEN /
    invisible)**, NO render-id (a scratch/dynamically-built cel).  Not part of the
    visible frame — likely the party HUD parked off-screen during the cutscene.
    Identify `FUN_004962a0` before porting (it draws nothing visible at the hold).**
- **There is NO "Town of Tonkiness" banner blit at the hold** — zero keyed blits
  with `ret_va` in `0x5a00c0`'s range, refuting both the docs' banner attribution
  AND the `0x5a00c0`-overlay-player producer hypothesis (the same way the letterbox
  turned out to be `0x48c150`, not `0x5a00c0`).  If an area-title card exists it is
  GDI text and/or at a different time (TBD — not in 1450-1600).
- **The visible cast is NOT the main map-object band.**  The `0x491ae0` main-band
  census at flip 1500 shows banks `0x16c`/`0x175` only (the 5 props + the wagon),
  and at cam 128000 those sit world-x 88200-176000 = OFF-SCREEN-LEFT, so they never
  present here.  The 18 cast cels (banks `0x426`/`0x459`/…) come from a **different
  render source** — the 8 PARTY actors are reset by `0x59f2c0`→`0x560e60` (`ret_va
  0x59f578`, spawn-discovery probe), and/or a separate scene-actor band.  **Pin the
  exact source next: annotate the actor emit `FUN_00492670` (or the band-walk that
  feeds `0x48eac0`) with the actor code/`+0x1d4`, recapture → each cast cel ties to
  its actor** (the methodical loop, mirror of how `0x48ecc2` pinned the present).
- **Determinism (quirk #82):** at the establishing hold the scene-lock
  `*(DAT_008a9b50+0x27a8)!=0`, so EVERY actor's behaviour half `break`s out — no LCG
  drawn — so the whole hold frame is **deterministic**.  This decomposes the user's
  "1:1 on every frame, then the RNG consumers" exactly into two phases:
  - **Phase 1 (this residual):** RE the cast's spawn + multi-part static render
    (positions + sprite tables + clips at the locked pose) → the establishing-shot
    frames go differ_px==0.  This is the protagonist/character render system (the
    `0x491ae0` multi-part arm generalised past the wagon; the lazy `+0x48` sprite
    fill, PORT-DEBT `actor-sprite-table`).
  - **Phase 2 (the RNG consumers):** their post-unlock wander — RE + port every
    in-scene LCG consumer (`0x54f980:929+` idle/wander, etc.) and match consumption
    order (rng+rngcalls both sides) so the post-cutscene frames stay 1:1.
- Artifacts (local, `/tmp`, ephemeral): `/tmp/blit_banner_retail/call_trace.jsonl`,
  `/tmp/spawn_disc/` (+ the hold PNG, pushed to the feed).  Regenerate via the
  field-spec capture above.

### The establishing-hold cast is FOUR map-object bands — the producer map (ckpt 83)

Pinned every visible cel of the establishing hold to its producer, closing the
ckpt-82 "pin the cast source next" open item.  The decisive tool was a
field-spec **band census** (`retail_fields.json` got the 5 non-main band render
entries `0x4937c0`/`0x493480`/`0x492fc0`/`0x493230`/`0x493ba0` + the two emit
primitives `0x492670`/`0x4917b0` with `renderid` on the emitted cel).  The driver
`FUN_0048c150` (free-roam branch, `in_ECX[7]==0`) runs **8 emit passes** over the
bands off `DAT_008a9b50`, then one present `0x48eac0` flushes — so the cel↔producer
tie lives at EMIT, not present (the 0x3c node carries cel+pos+mode only, no
back-ref).  Hooking the emit primitives with the resolved `cel_res` is authoritative.

**The 18 visible keyed cels @ the hold (cam 128000) decompose into 4 bands, all
DATA-1022 map objects:**

| band off 8a9b50 | render fn | type range | what (codes → bank → res) |
|---|---|---|---|
| `+0x11e0` CHARACTER | `0x491ae0` | 70000–79999 | mostly INVISIBLE collision volumes (bank 0) + 3 props (`0x1129e`/`0x1129f`/`0x112e5` bank `0x16c`→res `0x403`) + the script wagon `0x1872d` (bank `0x175`→`0x3ec`). **Already ported.** |
| `+0x2560` STRUCTURE | `0x493230` | 60000–69999 | **the TREE** `0xec55` bank `0x15f`→res **`0x481`** (×2); **bg decorations** `0xec6a` bank `0x16c`→`0x403` (×29, layer 8); **fg hedges** `0xec60` bank `0x164`→**`0x426`** (×8; the 5 on-screen are **layer 15** = the bottom row). |
| `+0x1160` EFFECT | `0x493ba0` | 50000–59999 | the **townsfolk** — 10 distinct standing NPCs `0xc3be`/`0xc3dd`/`0xc3e6`/`0xc3f2`/`0xc404`/`0xc422`/`0xc42c`/`0xc440`/`0xc441`/`0xc468` (1 each, banks `0x8b`–`0xf9`→res `0x459`/`0x462`/`0x46a`/`0x46b`/`0x472`/`0x47b`/…) + `0xe29a` ×4 (bank `0x146`→`0x426`/`0x3fa`) + `0xe2a5`. layer 12/13. |
| `+0x13e0` | `0x493480` | (script `0x18704`/`0x18708`) | 41 animated bank `0x1aa`→res **`0x408`** at layer 6 (sky) + a layer-11 cluster around the square — **NOT in the keyed set** (blits via alpha/clipped, not `0x5b9b70`); ambient particles/effects, deferred. |

The other bands (`+0x1060`/`+0x23e0`) are empty at the hold.  Backdrop tiles come
from the tile walk `0x490f30` (via `0x4917b0`, res `0x403`/`0x408`/`0x428`/`0x432`/
`0x433`/`0x436`/`0x437`/`0x8ba`).

**Map-driven + deterministic (the key portability result).**  For STRUCTURE the
census render-state matches the map record EXACTLY: **world pos = map (x,y)×100**,
**`frame_base` = map `variant`@+0x18** (verified cel-for-cel: tree variant {0,1},
hedge {0,1,4,5}, deco {16,18,20,21,24,26,28,32,33,35} — identical to the live
`frame_base`).  So the tree + hedges + decorations are a pure function of DATA-1022.
The EFFECT townsfolk also map 1:1 by code/count (`map_data --objects`: 13 EFFECT
codes incl. `0xe29a`×4) but their render-state pos carries a deterministic spawn
offset (≈ +3000 x) over the map pos — the `0x41f200` activator's placement (RE
next).  A few census actors are NOT in the map (`0xc35a` ×2 — also drawn by the
party renderer `0x4997b0`/`ret 0x4997f1` = a PARTY member; `0xc3dc`/`0xc3f0`) →
**script/party-spawned**, like the wagon.

**The hold is mostly STATIC + 4 wanderers.**  Across flips 1450/1500/1600 (all
cam 128000, pre-pan) the 16 standing townsfolk + all 39 structure objects hold a
FIXED world pos (only the anim `frame` advances — deterministic per quirk #76);
only `0xe29a` ×4 translate (RNG wander, Phase 2).  This refines quirk #82: the
scene-lock stops the +0x11e0 behaviour, but the +0x1160 EFFECT band has its own
update that lets `0xe29a` roam during the hold.

**Corrects the docs' model:** the visible cast is NOT the +0x11e0 "town actor"
band (that's collision volumes + props); it is split across +0x1160 (townsfolk),
+0x2560 (tree/scenery), +0x11e0 (props), +0x13e0 (particles).  And the ckpt-82
"foreground TREE" is the STRUCTURE object `0xec55` (res `0x481`) — NOT a banner,
NOT the `0x5a00c0` overlay (both already refuted), and NOT a tile.

**CAVEAT (a footgun I hit):** hand-computing the band VAs for the census analysis
was wrong (off by 0x200) → the bands falsely read "0 active".  The emit-primitive
hook (`cel_res` via `renderid`) was the cross-check that caught it.  Compute VAs in
code, and trust the emit hook over a band-entry census.  engine-quirk #84.

**Port plan (Phase 1, simplest-first):**
1. **STRUCTURE band** (tree + hedges + bg deco) — fully map-driven, single-cel
   (`0x493230` is a no-loop renderer), static.  Spawn the 39 structure objects
   (pos=map×100, frame_base=variant, code→bank {`0xec55`:`0x15f`, `0xec60`:`0x164`,
   `0xec6a`:`0x16c`}, layer per the `0x438a60` activator — RE next), render via a
   port of `0x493230`, wire into `game_render`.  Lands the prominent TREE first.
2. **EFFECT band** (townsfolk) — multi-part `0x493ba0` (built on the ported
   `0x44d160`) + the `0x41f200` spawn (pos offset).  The 16 static land at a matched
   sim-tick; the 4 `0xe29a` need Phase 2 (RNG).
Artifacts: `/tmp/cast_census/`, `/tmp/tree_emit/` (ephemeral; regen via the
field-spec capture at flips 1450/1500/1600).

**PORTED (ckpt 83b) — the STRUCTURE band is in + USER-1:1.**  RE'd the activator
`FUN_00438a60` (60000-range: a per-code switch, each case ends with a fill
primitive that installs the sprite bank — 0xec55→0x15f, 0xec60→0x164, 0xec6a→0x16c,
… — the def table) and the dispatcher `FUN_0058d460:233-269` (the layer = the map
record's foreground flag @+0x30: `==1 ? 15 : 8`; frame_base = variant @+0x18; pos =
record (x,y)×100 — all verified cel-for-cel against the census).  Ported
`actor_spawn_struct_from_map` (`src/actor_spawn.c`, fully map-driven — no capture
stand-in, so not PORT-DEBT) + `actor_spawn_struct_bank_for_code`; the render REUSES
`actor_render_static` because `0x493230`'s static single-cel blit is bit-identical
to the `0x491ae0` default arm (same cel `(bank, frame_base)`, same `(world +
dst_base + row off)`, same mode-0 keyed node).  Wired in `main.c` (`g_structs`
spawned in `enter_game`; `game_actor_walk` walks it at the structure layers 8/15,
which the layer-ordered present interleaves with the cast).  **Live-verified:** 39
objects spawn + emit; the tree bank `0x15f` registers; `render_diff` (port flip 1200
↔ retail 1500) shows the tree `(0x481,f0)`@(496,64) 320×320, the 5 hedges `(0x426)`,
and the 4 `0x403` props/deco all matching on identity + frame + dst position (zero
`[rect]`/`[decode]`/`[state]`); the only residuals are the 8 EFFECT townsfolk cels
+ the 18 off-screen `0x4962a0` invisibles.  **USER-confirmed on the feed: "the
decorations are there and positioned 1:1."**  parity-ledger #9; 897 pass (+1).

### The EFFECT townsfolk PORTED — positions 1:1, RNG residual identified (ckpt 84)

Landed the EFFECT band (the standing townsfolk in the square) positioned 1:1,
frozen on the idle clip's frame 0 — the wagon/STRUCTURE precedent (position-first,
animate next).  Builds directly on the ckpt-83 census.

**The render REDUCES to `actor_render_static`** (like STRUCTURE).  For a plain
townsperson `FUN_00493ba0`'s static arm (`LAB_004943d7` → `FUN_0044d160` describe
→ the emit loop) emits exactly ONE mode-0 keyed cel — verified against the hold
blit trace (`0x5b9b70` carried `res`+`frame`; 18 keyed blits, one per townsperson,
**no `0x4917b0` shadow/split, no `DAT_008a9358` color-remap fired** — the node
stayed keyed mode-0, so neither path changed the output).  The spell-text / HP-bar
/ effect-list arms (`0x493ba0:78-378`) all gate on render-state `+0x66`/view
`+0x562a` (0 for a townsperson) → skipped.  So the EFFECT band reuses
`actor_render_static`, exactly as STRUCTURE does.

**The placement is FULLY MAP-DRIVEN** (derived from the census + the map, NOT a
27 KB switch read): `world = (map (x,y) − dst) × 100`, where `dst` is the per-code
render anchor (render-state `+0x40/+0x44`).  Proof: the census `rs_x` = `(map_x −
dstx) × 100` cel-for-cel (e.g. `0xc3e6` map (208,384) dst (−30,−32) → (23800,41600),
matching the live render-state exactly; the `+30`-px world offset exactly cancels
the `−30` render dst at the screen, so `screen = map − cam`).  The 11 map townsfolk
= 10 distinct `0xc3xx` (×1) + `0xe2a5`.

**Ported (898 pass, +1):** `actor_spawn_effect_from_map` + `actor_spawn_effect_def_
for_code` (`actor_spawn.c`, PORT-DEBT `effect-sprite-table` — the captured `{code →
bank, dst, layer}` stand-in for `0x41f200`); `main.c` `g_effects` pool spawned in
`enter_game`, walked by `game_actor_walk` via `actor_render_static` at layer 13 (the
layer-ordered present slots the townsfolk in front of the bg deco / behind the fg
hedges).  Live: 11 townsfolk spawn + 11 nodes emit (bank `0xf9` registered).

**VERIFIED + the RNG residual (USER, ckpt 84).**  render_diff (port 1200 ↔ retail
1500): the on-screen townsfolk match retail on **resource + position** (zero
`[rect]`/`[state]`).  **USER-confirmed: "the NPCs are rendering at the correct
positions."**  The residual is now pinned to the **RNG pillar** — three RNG-driven
elements that make the scene NOT yet 1:1 on every frame, exactly the USER's Phase-2:
1. **Townsfolk FACING (mirror)** — some townsfolk render flipped (USER: "the
   orientation of some of them is flipped").  `FUN_0044d160`'s `rs->facing == 3`
   arm reflects `off_x` + picks the mirror cel (`frame_off = DAT_008a8440[bank]`);
   the port spawns `facing 0` + passes `flip_table NULL`, so NO townsperson
   mirrors.  The facing is set per-actor (likely an RNG draw at spawn) → Phase 2.
2. **Townsfolk idle PHASE** — frozen on frame 0; the idle clip `0x6290e0` (base 0,
   20 frames, dur 14, looping, delta {0,1,2,1,0,1,2,1,…}) IS decoded + the stepper
   IS ported, but the per-actor START phase is staggered run-deterministically
   (`0xc3e6` f0 / `0xc404` f18 / …; NOT a map-record field → likely RNG at spawn).
3. **The FOUNTAIN PARTICLE SPRAY** — the entire `+0x13e0` band (`0x493480`, res
   `0x408` bank `0x1aa`, the "layer-11 cluster around the square") is MISSING in the
   port (USER pointed it out: a purple/blue/white sparkle spray erupting from the
   fountain + green leafy particles right).  Clearly RNG (particle positions /
   velocities).  Deferred at ckpt 83 as "ambient particles"; now a named Phase-2
   consumer.

PORT-DEBT `effect-sprite-table` / `effect-anim-phase` / `effect-wanderers`.  **NEXT
(USER directive): the scene-wide RNG-CONSUMER CENSUS** — hook the LCG (`FUN_005bf505`
/ `DAT_008a4f94`) in the retail town scene, log every draw's consumer site (`ret_va`)
+ `rngcalls` + value, enumerate WHO consumes RNG and in what ORDER (the townsfolk
facing/phase, the `0xe29a` wander, the fountain particles `0x13e0`, the `0x54f980`
behaviour, …), then match consumption order both sides → the scene goes 1:1 on
every frame.  This RETIRES the ckpt-73 "defer all RNG" deferral.

### The scene-wide RNG-consumer census (ckpt 84, Phase 2 begins)

USER directive after the townsfolk landed positioned 1:1: "drill into RNG
consumption next … track down every RNG consumer in the scene," and "integrate the
RNG tracing into the flow-trace tooling" (not a bespoke probe).  Done — and it
RETIRES the ckpt-73 defer-all-RNG.

**Integration (the flow-trace-native way).**  Added `0x5bf505` (`FUN_005bf505`, the
engine LCG = MSVC rand) as a flow-trace entry in `tools/flow/retail_fields.json`
(field `rng_state` = `DAT_008a4f94` read onEnter; the auto-captured `ret_va` NAMES
the consumer site).  Capture with `--field-spec --field-spec-only` + a tight
`--call-trace-frames` whitelist (the LCG is hot — a burst at room-load, then per
tick); analyse with the new `tools/rng_consumer_census.py` (maps each `ret_va` +
0x400000 → its function via `functions.csv`, splits at a frame boundary).  Supersedes
the ad-hoc `--rand-probe` for the in-scene census.

**The capture.**  `frida_capture.py --no-turbo --lockstep --seed-pin --input-trace
…/trace-retail.jsonl --field-spec retail_fields.json --field-spec-only
--call-trace-frames <1420-1445,1450-1465,1500,1550,1600>` → **1142 LCG draws** across
the town-scene window (game_enter@1434).  Split at 1434 (`--split-frame 1434`):

| consumer fn | spawn (≤1434) | hold (>1434) | sites | what |
|---|---:|---:|---:|---|
| `FUN_0041f200` | **134** | 0 | 8 | the EFFECT activator — the townsfolk FACING + idle PHASE (8 sites = its 8 static rand calls; the USER's "flipped orientation"). |
| `FUN_00426ec0` | 38 | 0 | 2 | spawn helper (sprite/anim install via the activator). |
| `FUN_00427670` | 20 | 0 | 5 | spawn helper. |
| `FUN_00426fd0` | 19 | 0 | 1 | spawn helper. |
| `FUN_0054f980` | 5 | **425** | 12 | the per-actor BEHAVIOUR dispatch — idle waits + wander (the `0xe29a` wander; quirk #77). |
| `FUN_0047b990` | 15 | **320** | 4 | the `+0x1160` EFFECT-band UPDATE fn (actor-walk table) — the per-tick effect/particle/wander RNG (the FOUNTAIN SPRAY lives here / in `0x453960`). |
| `FUN_00453960` | 2 | **154** | 2 | per-tick consumer (particle step / sub-update). |
| `FUN_00467380` / `FUN_004099a0` / `FUN_005531b0` / `FUN_00427360` / `FUN_00431cb0` / `FUN_00439690` / `FUN_004c5e00` | — | — | 1-2 | room/camera setup incidentals. |

Cross-checked against the static decompile: `0x41f200` has exactly **8** `FUN_005bf505`
calls (= the 8 sites), `0x47b990` has 10, `0x453960` has 2, `0x426ec0` has 2 — the
census matches the code.

**Two clean groups (the USER's two residuals, now pinpointed):**
- **SPAWN (one-shot, room-load):** `FUN_0041f200` + its 3 helpers (211 draws) set every
  townsperson's facing + idle phase + (probably) spawn jitter.  Porting these in the
  right ORDER → the townsfolk face the right way + start at the right idle frame.
- **HOLD (per-tick):** `FUN_0054f980` / `FUN_0047b990` / `FUN_00453960` (~900 draws) drive
  the `0xe29a` wander + the fountain particle spray + idle behaviour.

**NEXT (the matching half):** (1) RE `FUN_0041f200`'s 8 rand sites → which sets facing
(the `+0x2c`==3 mirror) vs idle phase (`+0x72`) vs jitter; port them in order.  (2) An
**RNG ANCHOR at game_enter** both sides (snapshot/restore `DAT_008a4f94` — the ckpt-73
fix; the port can't replay the whole boot RNG chain, so re-seed the town scene to a
known state on both sides), then match consumption order (rng+rngcalls) tick-for-tick.
(3) Annotate each producer with a `rngcalls` field + a port `CALL_TRACE_BEGIN` mirror so
`flow_diff` localizes the first divergence.  Then the fountain particles (`0x47b990`/
`0x453960`) + the `0xe29a` wander land 1:1.  Artifacts (ephemeral): `runs/rng-census/` /
`/tmp/rng_census/`; `tools/rng_consumer_census.py`.

### Townsfolk facing is a MAP field (PORTED + USER-1:1); idle phase + fountain are RNG (ckpt 85)

Phase-2 "matching half", first chip.  RE'd the three ckpt-84 RNG residuals and found
the facing is RNG-FREE (a map field) while idle-phase + fountain are genuine LCG
consumers — so the facing landed standalone, USER-confirmed, with no RNG anchor.

**FACING = the map sub-record field `puVar1[4]` (NOT RNG).**  The room-object
dispatcher `FUN_0058d460:96` computes `cVar12 = (-(puVar1[4] != 0) & 2) + 1` →
**1 (normal) / 3 (mirrored)**, where `puVar1 = *(*(mapobj+0x3c) + i*0x10)`, and
forwards it as **param_8** to the EFFECT activator `0x41f200` (`:151`) + the CHARACTER
activator `0x431e30` (`:227`).  `0x41f200:861` stores param_8 at render-state `+0x2c`.
The renderer `FUN_0044d160` mirrors only on `facing == 3`: cel `frame += flip`,
`off_x = mirror_x - off_x`, where `flip = *(short*)(DAT_008a8440[bank])`.  Read live,
**`0x8a8440` is a pointer array** whose entries deref to heap sprite-group descriptors;
the first short is the group's **frames-per-direction** (4 or 16 for the town banks).
So the mirrored cel = `frame_base + frames_per_dir`.

**Live census ground truth** (`0x493ba0` field spec + a new `rs_facing` field, plus a
one-shot read of `DAT_008a8440[bank]`; `--seed-pin --lockstep --no-turbo`, flips
1450/1500/1550/1600):

| code | bank | facing | flip (frames/dir) |
|---|---|---|---|
| 0xc3be | 0xd4 | **3** | 16 |
| 0xc3dd | 0xe1 | **3** | 4 |
| 0xc3e6 | 0xe5 | **3** | 4 |
| 0xc422 | 0x93 | **3** | 16 |
| 0xc42c | 0x99 | **3** | 16 |
| 0xc441 | 0xa9 | **3** | 16 |
| 0xc468 | 0xd0 | **3** | 4 |
| 0xc3f2 | 0xf0 | 1 | (4) |
| 0xc404 | 0xf9 | 1 | (4) |
| 0xc440 | 0xa6 | 1 | (16) |
| 0xe2a5 | 0x14c | 1 | (16) |

(The census also re-confirms `0xc3dc`/`0xc3f0` are the non-map script-spawned pair —
correctly absent from the port's 11 map townsfolk; `0xc440` is multi-part, banks
0xa6+0xb5 — a separate render gap, facing 1.)

**PORTED (898 pass, builds clean):** `TOWN_EFFECT_DEFS` gains `facing`+`flip` columns;
`actor_spawn_effect_def_for_code` returns them; the spawn sets `rs->facing`;
`actor_spawn_effect_fill_flip_table` fills a bank-indexed stand-in for the global
`DAT_008a8440`, wired in `main.c` and passed to every `game_actor_walk`
`actor_render_static` call.  **USER-confirmed on the feed: "npc orientation matches
retail yes."**  (Frozen-frame still — the idle anim PHASE differs; that's the next
residual.)  PORT-DEBT `effect-sprite-table` extended (facing+flip captured; proper
source = map `puVar1[4]` via the unported `0x587e00`, + `DAT_008a8440` frames/dir).

**IDLE PHASE is RNG.**  `FUN_00426ec0` (spawn phase-randomizer): with a clip,
`+0x72 = (rand() * clip.frame_count@+0x42) >> 15` (uniform start frame) + a 2nd rand
for the timer.  Matching it 1:1 needs the **game_enter RNG anchor** + replaying the
spawn draws in order.

**The 8 rand draws in `0x41f200` are position-jitter + a particle sub-spawn** (`:294`/
`:301` → `0x426e00` motion `+0x58`/`+0x60`; `:326-334` → `0x427b70` particle emitter),
NOT facing/phase.  Helper `0x427670` (20 draws) is the fountain particle init; with
the per-tick `0x47b990`/`0x453960` it drives the FOUNTAIN SPRAY (band `+0x13e0` /
`0x493480`) — all RNG.

**NEXT (the remaining two residuals → the RNG anchor):** snapshot/restore
`DAT_008a4f94` to a fixed value at the `game_enter` anchor on BOTH sides (retail: a new
harness re-pin; port: re-seed in `enter_game`), then port the spawn RNG consumers in
order (`0x41f200` jitter + `0x426ec0` phase + `0x427670`/`0x427b70` particles) so the
idle phases + fountain align; then the per-tick `0x47b990`/`0x453960` + `0xe29a`
wander.  Tooling: `rng_consumer_census.py`, `flow_diff.py`, the `rng_state`/`rngcalls`
fields.  Artifacts (ephemeral): `runs/facing-census/`, `runs/read_fliptable.py`.

### The town SPAWN RNG anchor — LANDED + verified (ckpt 86)

The keystone that makes the two ckpt-85 RNG residuals (townsfolk idle PHASE + the
fountain SPRAY) portable: re-align `DAT_008a4f94` on both sides at the town spawn so
the per-object spawn draws (idle frame, particle jitter) are reproducible despite the
non-deterministic title->town consumption (quirk #77).  RE + tooling milestone (no
visual change yet — that lands when the spawn consumers are ported, Chip 2).

**The spawn RNG structure (retail ground truth, engine-quirk #86).**  The seed-pinned
`0x5bf505` census (`runs/rng-census-repin`) shows the town-LOAD frame draws a fixed
238-draw burst over **19 EFFECT objects** (`0x58d460` -> `0x41f200`, map-layer order).
Per object: `0x426fd0`(1) + `0x41f200`(7 = jitter + particle params) + optionally
`0x427670`(5, 4 objects) + `0x426ec0`(2 = idle frame `+0x72` / timer `+0x70`); two
objects carry a type-switch one-off (`0x431cb0`/`0x427360`).  The port renders 11
standing townsfolk but ALL 19 consume RNG, so the replay must process all 19 in order
(rendering only the 11) to keep the idle phases aligned.

**The re-pin point — first `0x41f200`, not `game_enter`.**  A pre-spawn one-off draw
`0x4c5e00`(1) fires between the `game_enter` entry (`0x59f2c0`) and the first
`0x41f200`, so pinning at `game_enter` would desync the port by one draw.  The anchor
is therefore: arm at the `game_enter` agent anchor, write the seed at the FIRST
`0x41f200` onEnter (`installRngAnchor`, a per-map latch; emits the `rng_anchor`
event).  The port mirrors it by re-seeding `rng_srand(game_rng_seed())` at the top of
`enter_game` — faithful because all pre-effect-spawn `enter_game` code
(load_town_scene, camera, the CHARACTER/STRUCTURE spawns) is RNG-free, exactly as
retail's `0x431e30`/`0x438a60` dispatches are.

**Verified live.**  `town SPAWN RNG re-pinned @ frame 1419 (game_spawn):
DAT_008a4f94 0x71cc78f1 -> 0x004f5347`.  The `before` (0x71cc78f1) differs from the
`game_enter` seed (0x46fe3f46) — proving the intervening pre-spawn draw — and the
spawn burst's draw counts are byte-identical pre/post re-pin (134/38/20/19), so the
re-pin resets seed VALUES only, not control flow.  The spawn is now deterministic from
`0x4f5347` on both sides.

**Code.**  Agent: `installRngAnchor()` + the `g_rng_anchor_armed`/`g_rng_anchored`
latch armed in the `game_enter` scene-anchor handler (`opensummoners-agent.js`);
`frida_capture.py` logs the `rng_anchor` event.  Port: `game_rng_seed()` helper
(shared by boot + `enter_game`) + the `rng_srand` re-seed at `enter_game`
(`main.c`).  **898 pass, ledger 199/194 unchanged** (no function ported — a harness +
seam change).

**NEXT (Chip 2):** port `0x41f200`'s per-object RNG consumption in map order (the
19-object burst above), give the 11 townsfolk the idle clip `0x6290e0` + set
`+0x72`/`+0x70` from the aligned `0x426ec0` draws, then verify the idle phases vs
retail's render-state `+0x72` per townsperson.  Then the fountain (Chip 3).

### The townsfolk idle PHASE — PORTED (ckpt 87)

Chip 2 of the spawn-RNG arc, on top of the ckpt-86 anchor.  The 11 standing villagers
now run the idle breathing clip from a per-actor RNG start frame (retail staggers them),
instead of frozen on frame 0.

**The model (decompile- + census-verified).**  Per map EFFECT object in dispatch order,
`0x41f200` draws `8 + extra` before `0x426ec0`: `0x426fd0`(1) + prologue(7); extra = 5
for the 4 `0xe29a` wanderers (`0x427670` case 2, :2181), 1 for `0xe2a5` (`0x431cb0`,
:2272), else 0.  Then `0x426ec0`(2): `frame=(rand*20)>>15`, `timer=(rand*14)>>15` over
the shared idle clip `0x6290e0` (base 0, 20 frames, dur 14, looping; decoded from the
exe).  All 11 rendered townsfolk are in the first 15 (map) effects — the 4 script
effects + the conditional `:2849` draw spawn after, so they do not perturb the phases.

**Ported.**  `actor_spawn_effect_from_map` replays every map EFFECT object's draws in
order (consume-to-advance; the `0xe29a` wanderers + unknown codes consume but are not
spawned; only the rendered townsfolk use the `0x426ec0` pair), embeds `IDLE_CLIP` +
`effect_prefix_draws(code)`, sets `rs->clip`/`frame`/`timer`.  `game_actor_update`
advances `g_effects` per sim-tick.  Host test `actor_spawn_effect` locks the replay to an
inline reference LCG; 898 pass.  Offline from `0x4f5347`: start frames
{1,17,17,17,3,14,4,16,18,12,10}.

**Validation pending (not yet differ_px==0).**  The chain is complete (Chip-1 seed +
census draw counts + decompile shapes + decoded clip + host test), but the live bit-exact
cross-check of `+0x72` per townsperson vs retail has not run.  Cleanest: a `0x426ec0`
onLeave field read (the field-spec is onEnter-only today) or render_diff at a matched
sim-tick.

### The FOUNTAIN SPRAY — the particle subsystem RE'd (ckpt 88, Chip 3)

Chip 3 of the in-game-intro arc: the purple/blue/white water spray erupting from the
fountain (+ leafy particles), the last visible-but-missing element of the establishing
scene.  This is a full RE of the engine's **particle subsystem** — read end-to-end from
the decompile; the live ground-truth capture + parity bar are the next step.

**Corrects the ckpt-84 census misattribution.**  The RNG census guessed "the FOUNTAIN
SPRAY lives in `0x47b990`/`0x453960`".  Read at the byte level, `0x47b990` is the
**`+0x1160` EFFECT-band behaviour/AI dispatcher** (player aura, the `0xe29a` wander,
`0xc35a`…) keyed on `+0x1d4`; it does **not** handle any fountain code.  `0x453960` is a
generic **2-draw scatter helper** (`rs[+0x18]=rand_range(p3,+p4)`, `rs[+0x28]=
rand_range(p1,+p2)`), used by many particle spawns — not the fountain owner.  The census
counted RNG draws by function and lumped these in; the real owners are below.

**The architecture (5 parts).**
1. **The band.**  `DAT_008a9b50 + 0x13e0` is the **DEVICE / particle pool — 1024 slots**
   (`0x46cd70:103` walks `0x13e0` for `iVar10=0x400`).  Each slot is a render-state-style
   record (the same `+0x40`→sub-record shape as actors: `+4/+8` world x/y, `+0x18`/`+0x28`
   velocity, `+0x2c` facing, `+0x5c` lifetime counter, `+0x58` sub-phase, `+0x6c` clip,
   `+0x70/+0x72/+0x74` anim, `+0x1d4` code, `+0x1d0` active, `+0x280` age/priority).
2. **The emitter** = the **fountain prop `0x112e5`** (a CHARACTER, `+0x11e0` band —
   **already spawned by the port since ckpt-79**, bank `0x16c` res `0x403` frame 36, at
   map pos `+0/+0`).  Its per-tick behaviour `0x54f980` **case `0x112e5`** (`:218-235`):
   when primary (`param_3==0`) and not paused (`in_ECX[3]!=1`), every sim-tick it draws
   2 LCG values (`y = (rand*400>>15) - 3000`; `x = (rand*800>>15) - 400`) and calls
   `0x557370(this, 0x18708, parent, x, y, 99, 1, 0,0,2,…)` to spawn one **`0x18708`**
   water droplet, then draws ~2 more for a splash sound.  (The neighbouring `0x112e2`
   case `:150-172` spawns `0x18704` every 6th tick — a second emitter; confirm live which
   are active in the town.)
3. **The allocator** `0x557370` (452 B): round-robin scan of the 1024 `+0x13e0` slots for
   a free one (`+0x1d0==0`, index `*(mapctl+0xce) & 0x3ff`); if none, evict the lowest-
   `+0x280` (oldest) slot below the caller's priority `param_13`.  Computes the spawn
   position parent-relative by anchor mode `param_7` (1=center, 2=top, 3=center-top,
   4=center-bottom), then `0x557550(code, x, y, …)`.
4. **The config** `0x557550` (21 KB, a per-code switch — the DEVICE activator).  Fountain
   cases install bank **`0x1aa`** (= res **`0x408`**, the census bank):
   - **`0x18708`** (`:670`): `0x426d70(0,0x1aa,6)` frame 6 + clip `&DAT_006449c0` +
     `0x4385c0(DAT_008a9330)`.  No velocity scatter (velocity comes from the stepper's
     gravity).  **The main fountain water.**
   - **`0x18704`** (`:630`): frame 8 + clip `&DAT_00644b58` + `0x453960(-10000,5000,
     -1000,1000)` → launch `rs[+0x28]∈[-10000,-5000)`, `rs[+0x18]∈[-1000,0)` (upward).
   - **`0x18707`** (`:661`): frame 8 + clip `&DAT_00644cf0` + `0x453960(-25000,10000,
     -5000,10000)`.  **`0x18709`** (`:676`): frame 0 + clip `&DAT_00644e88` + `rs[+0x58]=
     rand%2` (a leaf?).  All bank `0x1aa`.
   - `0x186ca` (`:37`) is an **invisible controller** (no sprite; sets timer `+0x96=0x3c`,
     `rs[+0x28]=8000`) — distinct from the visible droplets.
5. **The per-tick stepper** `0x46e510` (10.7 KB, called once/slot from `0x46cd70:107`): a
   giant switch on `+0x1d4` with per-type physics.  Common shape: integrate
   `rs[+4] += ±rs[+0x28]/100` (x, signed by facing `+0x2c`), `rs[+8] += rs[+0x18]/100`
   (y), age `rs[+0x18]` toward a clamp (gravity), advance/fade the clip via the alpha LUT
   `&DAT_008a9308[phase]` into `in_ECX+0xf4/+0xf8`, and expire (`*pcVar3=0` free, or
   `0x556180`) on lifetime or a collision-grid hit (`(x/0xc80,y/0xc80)` → `mapctl+0x21c04`).
   - **`0x18708`** (`:529`): `rs[+0x18] = min(rs[+0x18]+8000, 80000)` (accelerate DOWN),
     x/y integrate, cycle 8 frames (`+0x58`), expire on water/ground grid hit.
   - **`0x18704`** (`:683`): `rs[+0x18] = max(rs[+0x18]-500, -5000)` (decelerate UP),
     integrate, fade after lifetime `+0x5c > 0x28`.
6. **The render** `0x493480` (the `+0x13e0` band renderer, called from `0x48c150`).  For a
   plain particle it takes the **default (`else`) arm** `:93-117`: `0x44d160` describe →
   `0x4917b0` **alpha-blit** (NOT the keyed `0x5b9b70` path the port uses for opaque
   actors — particles are alpha/clipped; this is a new sink for the port).  The `0x186ca`
   arm `:33-91` is a separate horizontal-cel-string renderer (the controller), not a
   single particle.

**Scope / parity question (drives the next step).**  The emitter draws RNG **every
sim-tick, continuously** (≈2-4 draws/tick + per-particle draws).  The establishing-hold
gates the wander RNG via the scene-lock `*(0x8a9b50+0x27a8)==0`, but the fountain case is
**NOT** gated on it (so the spray runs during the hold — consistent with the USER seeing
it).  Whether the per-tick fountain stream is deterministic run-to-run under the ckpt-86
re-pin (quirk #77's per-present desync vs the deterministic sim-tick clock during the
locked hold) is the **open question that sets the parity bar** (bit-exact vs
"data-1:1 given a matched RNG state").  **Verify live next** (two seed-pinned retail runs,
compare `+0x13e0` particle positions tick-for-tick) before porting — do not guess.

**Clips to decode from the exe** (like the wagon `0x671c48` / idle `0x6290e0`):
`&DAT_006449c0` (0x18708), `&DAT_00644b58` (0x18704), `&DAT_00644cf0` (0x18707),
`&DAT_00644e88` (0x18709); + the alpha LUTs `&DAT_008a9308` / `&DAT_008a92e0`.

#### Live ground truth + decoded clips (ckpt 88)

Mined from the ckpt-86 capture `runs/rng-census-repin` (`--seed-pin --lockstep
--no-turbo`, re-pinned at the spawn), which already hooks `0x493480` (the particle
renderer, 5924 events) and `0x5bf505` (the LCG, 3630 draws) over the hold.

**Two particle systems render at the hold** (both bank `0x1aa` = res `0x408`):
- **`0x18708` — the fountain WATER spray** (2668 render calls): frame_base 6, clip
  `0x6449c0`, **layer 11**, clustered at the fountain (`rs_x` 169661-185544 ≈ ±80px
  around the prop's 177000; `rs_y` 35659-46103), `rs_frame` cycling 0↔1.  Emitter =
  the fountain prop **`0x112e5`** (`0x54f980:218`, spawns 1/primary-tick).
- **`0x18704` — the SKY particles** (3256 render calls): frame_base 8, clip `0x644b58`,
  **layer 6** (the sky), wider spread.  Emitter = **`0x112e2`** (`0x54f980:150`, spawns
  every 6th tick; ckpt-78 census had `0x112e2 ×2`).
- **~58-69 particles alive per frame** at steady state (the ckpt-83 "41" was a single-
  frame undercount).  Both emitter props are CHARACTER objects already spawned by the
  port (ckpt 79) — they only lack the per-tick spawn behaviour.

**Decoded clips** (decoder validated against IDLE_CLIP — exact match):
- `0x6449c0` (0x18708): base 0, **count 2, dur 2, looping**, deltas `{0,1}` → with
  frame_base 6, sprites 6↔7 every 2 ticks.
- `0x644b58` (0x18704): base 0, **count 6, dur 20, ONESHOT**, deltas `{0,1,2,3,4,5}` →
  with frame_base 8, sprites 8→13 over 120 ticks (holds the last).

**The parity bar (corrected — the spray is deterministic in lockstep).**  My first
analysis wrongly grouped ALL hooked events (incl. the blit primitives `0x5b95c0` etc.)
and saw "render-pass RNG bursts".  Filtering to `va==0x5bf505` (the actual LCG) shows the
truth: during the hold the LCG is drawn in a **regular per-sim-tick pattern** — 238 at the
spawn tick (quirk #86), then a clean **period-6 cycle `[6,14,6,14,14,14]`** — consumed
ENTIRELY by per-sim-tick updaters `0x54f980`(1585)/`0x47b990`(1199)/`0x453960`(592, the
particle-velocity scatter, ≈1 spawn/tick).  **The render pass draws NO LCG.**  Because the
harness captures in `--lockstep` (1 update/present per flip, = the port's cadence), there
is no per-present variance, so under the re-pin the scene RNG is **deterministic** and the
spray is **bit-exact portable** — CONTINGENT on the port reproducing the per-tick
consumption ORDER (the fountain shares the stream with `0x47b990`/the other `0x54f980`
cases, so frame-exact alignment is entangled with porting those co-resident per-tick
consumers — the broader Phase-2 work).  This refines quirk #77: its "non-determinism" was
the pre-re-pin seed drift, not a per-present LCG consumer.  (A second seed-pinned run to
prove run-to-run determinism is the clean confirmation.)

### The SKY-AMBIENT particles — CHIMNEY SMOKE (ckpt 89)

The second of the town's two particle systems (the first was the fountain water): code
**`0x18704`**, which the live capture + the USER's retail run identify as **chimney
smoke** — a translucent white plume rising from a house chimney, visible at the very top
of the screen after the camera pans left from the fountain.  Built directly on the ckpt-88
particle subsystem (same 1024-slot `+0x13e0` pool, same `0x493480` alpha render).

**The pieces (decompile-read, all confirmed against the trace below).**
- **Emitter** = the prop `0x112e2` (`0x54f980:150-172`), an INVISIBLE CHARACTER trigger the
  port already spawns (ckpt 79); census has 2 in the town (world `62800,4800` + `113600,
  18400` — LEFT of the fountain at `176000`, so they pan into view after it).  Every **6th**
  sim-tick (`if (5 < +0x5c+1)`) it draws 2 LCG (a y then x spawn jitter) and spawns one
  `0x18704` via `0x557370`.  NOT gated on primary/paused (unlike the fountain).
- **Config** `0x557550:630-640`: bank `0x1aa` frame_base **8**, clip **`0x644b58`** (decoded:
  base 0, 6 frames, dur 20, **ONESHOT**, deltas `{0,1,2,3,4,5}`), layer **6** (the sky), the
  velocity scatter `0x453960(-10000,5000,-1000,1000)` → vel_x ∈ [-10000,-5000), vel_y ∈
  [-1000,0), and the base brightness `in_ECX[0x3d/0x3e]=DAT_008a9350` (= ramp_b[18]).
- **Step** `0x46e510:683-710`: vel_y decelerates by 500 toward a -5000 floor (accelerate UP),
  integrate x (signed by facing +0x2c) + y, **expire when the oneshot clip finishes** (the
  +0x74 done flag), lifetime++, and a ramp_b fade `(&DAT_008a9308)[18 - (life-40)/4]` (clamp
  [2,18]) once life > 40.
- **Render** `0x493480` default arm → `0x4917b0` ALPHA-blit, exactly like the fountain — the
  node alpha is the render-state +0xf4 descriptor (sky → **ramp_b** `0x8a9308`; fountain →
  ramp_a `0x8a92e0`).  Port: `particle_pool_render` emits param8 = `(ramp_sel<<8) | idx`;
  `game_present_blit` PRESENT_ALPHA decodes ramp_a vs ramp_b.

**Trace verification (the USER directive "tracing to verify correctness").**  Mined the
ckpt-88 `runs/rng-census-repin` (`0x493480`, 5924 events) + a fresh `runs/sky-facing`
field-spec capture.  Ground truth that pins the port:
- The town's `+0x13e0` band renders **only** `0x18704` (3256) + `0x18708` (2668) — both now
  ported; **nothing else particle-wise remains** in the town.
- A sample `0x18704` render: **layer 6, bank 0x1aa, frame_base 8, clip 0x644b58** — exactly
  the port.  World **Y [-1734..18397]** ≈ port **[-1643..18225]** (the rise physics + emitter
  Y match nearly exactly).
- **Two RNG-independent port bugs the trace caught (now fixed):**
  1. **Anchor** (`0x557370` mode-1 = `parent_render_state[+0xc]/2`).  The `0x112e2` trigger is
     invisible → +0xc == 0 → the faithful anchor is **0** (the particle spawns at the prop's
     exact world pos); the port had a **hardcoded +1600** (USER caught it).  Removed — the sky
     now spawns at the prop.  (The fountain's +0xc ≈ 2810 → +1405; NOT its display-cel width,
     which measures +1700, so +0xc's setter is still un-RE'd → fountain keeps PORT-DEBT
     `fountain-anchor`, the calibrated +1245.)
  2. **Facing** (render-state +0x2c, the `0x46e510` x-integration sign).  The `runs/sky-facing`
     capture shows **all particles (sky AND water) have +0x2c == 1**, so x integrates
     `+= +vel_x/100` (no flip): the sky's negative vel_x → it **drifts LEFT** (retail world X
     [50690..114356], extending LEFT of the prop).  The port spawned facing 0 → sign-flipped →
     drifted RIGHT (invisible on the symmetric fountain spray, hence the fountain passed, but
     wrong for the always-left sky).  Fixed: `particle_spawn_{water,sky}` set `facing = 1`.
- After both fixes the port sky distribution is **world X [51440..113369]** ≈ retail
  **[50690..114356]**, Y matching — the shape now matches retail (exact per-frame positions
  still await the Phase-2 RNG phase-match, PORT-DEBT `fountain-rng-phase`).

**USER-confirmed:** "smoke looks 1:1, no alpha needed" + independently spotted the same
chimney smoke in retail ("smoke on a chimney at the very top of the screen … after some
left panning from the fountain").  During the establishing shot most of the plume is behind
the top letterbox bar; only the base (at the chimney) peeks below it — consistent with retail.

**Full-intro side-by-side video (ckpt 89).**  Frame-matched (anchor-aligned: port newgame
690/prologue 826/game_enter 1116 vs retail 748/945/1430) retail|port capture across
title→newgame→prologue→town, 64 pairs (`/tmp/intro_sidebyside.mp4` + a feed montage).
Verdict: **title/menu 1:1, prologue aligned, town establishing 1:1** (backdrop + fountain +
decorations + townsfolk + chimney smoke all match).  The one clear divergence the full
sequence surfaces: retail's **"Town of Tonkiness" area banner** (appears ~retail flip 1600+)
is MISSING in the port — the long-known `0x5a00c0` scripted-overlay debt
(PORT-DEBT `ingame-nontile-layers`); it is a TIMED scene element, not present at the hold
(consistent with ckpt 82's "no banner at the hold").

### The establishing REVEAL is a per-cell FADE-GRID transition — NOT the letterbox bars (ckpt 90)

**The chase.**  The golden-video review (#89) read the opening-town reveal as "a vertical
fade opening from the middle of the screen outward."  Measured off the dense golden
(`runs/video60-retail`, strict pure-`(0,0,0)`-row count): from game_enter the black region
is a clean center-out ramp — top bar `480 → ~240 → 64`, **−8 px / sim-tick**, floor 64,
settling at sim-tick ~25; bottom symmetric.  The obvious hypothesis was the quirk-#74
LETTERBOX bars (`0x48c150:124/143`, `in_ECX[0x11]`/`[0x12]` = +0x44/+0x48) ramping from
full-screen (240, meeting at center) down to 64.

**Both letterbox-bar hypotheses REFUTED by live capture** (the "don't guess, probe" payoff).
Annotated the candidates in `retail_fields.json` (`thisderef` off ECX) and field-spec-captured
the reveal window (`runs/letterbox-reveal{,2}`, `--seed-pin --lockstep`, the
`trace-retail.jsonl` drive):
- **`0x499ab0`** (the scene-cinematic step, 1×/sim-tick from `0x439690:1124`) — its
  this+0x44/+0x48 are **constant 64** (mode 1, already at target), scroll +0x4c **constant 0**,
  the WHOLE reveal.  Its `±2/call toward 64` ramp is real code but the bars are pre-settled.
- **`0x48c150`** (the grid-fill itself, the *authoritative* visible bar source) — its own
  this+0x44/+0x48 are **also constant 64** every flip of the reveal.
So the letterbox is a fixed 64 px the entire establishing shot, and the scroll never moves
(no vertical pan).  The reveal is a **separate producer over a static scene.**

**The real producer — `FUN_0048e920`, a per-cell fade-grid.**  Traced the black cel (res
`0x583`) blits via `0x5b9a40` over the reveal (`runs/reveal-blit`): three producers of the
black tile — `0x48c48a` + `0x48c4fe` (the constant 64 px letterbox bars, 160 tiles each) and
**`0x48e9c3`**, which emits **~1010 → 0** black tiles across the reveal (both bars ~202 px at
game_enter, **gone by the settled hold** sim-tick ~33).  `ret_va 0x48e9c3` lands in
**`FUN_0048e920`** (403 B), the **scene-transition FADE-GRID render** (called from
`0x48c150:175`, after the letterbox): it walks a grid of 64×4 cells (`this[0]` array, `this[2]`
count, `this[3]` mode; stride 0xc = state/timer/col/row) and per cell either ALPHA-blits the
black cel via `0x5bd550` (alpha `0x1f-(timer<<5)/1000`, partial fade), OPAQUE-blits via
`0x5b9a40` (fully black), or skips (clear).  The center-out iris is the per-cell clear PATTERN.
The per-cell state/timer **UPDATE is `FUN_0049af40`** (3313 B), called **2×/sim-tick** from the
cinematic step `0x499ab0:180-183` — that 2× is what turns the code's small per-call step into
the measured −8 px/sim-tick envelope.

**So the long-open "dark establishing-shot TOP GRADIENT" (ckpt 66/67) is finally explained:**
it is this fade-grid transition mid-animation (the top cells still opaque before the iris
opens), NOT a static tint and NOT the letterbox.

### The establishing REVEAL is PORTED — `src/scene_fade.{c,h}` (ckpt 95)

**Correction first (the ckpt-90 `0x49af40` was WRONG).**  Reading `FUN_0049af40` (3313 B): it
is the per-frame **HUD/portrait/HP-bar animator** (walks the 8 party slots `room+0x4030`, lerps
the fill bars + portrait fade timers, returns a counter) — it never touches a 64×4 grid.  The
real per-cell **update is the INLINE loop at `0x499ab0:125-177`** (`1×/sim-tick`, advance each
fading cell's timer), and the iris **pattern** is set by **`0x49a890`** (variant 0, center-out)
/ `0x49a740` (1, edges-in) / `0x49aae0`+`0x49aa00` (2, sweep).  So the measured −8 px/sim-tick =
mode-1's **2 rows/tick** × the 4 px row pitch (not the ckpt-90 "`0x49af40` 2×").

**The subsystem (the grid object `*(0x8a9b50+0x1040)`):**
- **arm** `0x439690:555-583` (the frame-FSM, gated on a transition request): `mode = req+0x28`,
  **`variant = (rand*3)>>15` ∈ {0,1,2}** (one LCG draw — the iris shape is RNG-chosen),
  `speed = req+0x2c`, `radius=0`, `done=0`, then fill all W×H cells `{state 0, timer 0, col, row}`.
- **render** `0x48e920` (after the letterbox, `0x48c150:175`): per cell by mode+state — state 0 →
  opaque black res `0x583` (slot 41, `0x5b9a40`); state 1 → alpha black res `0x458` (slot 40,
  frame[`0x1f-(timer<<5)/1000`], `0x5bd550`); state 2 → skip.
- **live params** (`runs/reveal-grid`, the `0x48e920` field spec): W=10, H=120, count=1200,
  mode=1, speed=1000, **variant=0** (center-out, this RNG run).

**Ported + wired** (`scene_fade.{c,h}` + `main.c`): `scene_fade_arm` at `enter_game` (after the
spawn burst, mirroring retail's order), `scene_fade_step` once/sim-tick after the camera easer
(`0x439690:1124` order), `scene_fade_render` after `letterbox_render`.  The opaque sink reuses
the letterbox cel (res `0x583`).

**The alpha edge — the true `0x5bd550` composite (the descriptor Ghidra dropped).**  The first cut
KEYED-blitted res `0x458` frame[level], which drew the gray MASK opaquely (USER: "white on the
outside, black on the inside, no transparency").  Disassembling `0x48e920` (`0x48eaa9`, the
`0x5bd550` call) recovered the thiscall ECX Ghidra omitted: **the blend descriptor is `*(0x8a93b8)`
= the `[19]`/full entry of the group-E ramp table `0x8a936c`** (`0x55f150` indexes it; the live
pointer is unique vs ramp_a[19]/ramp_b[19] — `runs/reveal-desc`).  The port already builds it:
**`g_pd_boot_group_e[19]`** (weight 1000, **mode-2 subtract-blend** = darken the dest by the source).
So res `0x458` frame[level] is a per-level GRAY MASK (light = opaque, black = clear) and the
subtract-blend darkens the town by it — just-marked cells (level 31, light) go near-black,
almost-cleared cells (level 1, black) leave the town.  `game_scene_fade_alpha` now does
`zdd_blit_orchestrate(&g_pd_boot_group_e[19], primary, res0x458[level], x, y, 0x40, 4, 0, 0,
0x1ffffff, NULL)` — exactly retail's call (gdi_ctx `*(0x8a6ec0)`≈NULL → the simple path; colorkey
`0x1ffffff` > 16-bit → no keying).  **VERIFIED on the composited capture** (`port_frame_01160`): the
blue town sky shows through and darkens smoothly to near-black across the receding edge (true
translucency), not the opaque gray.

**VERIFIED** (port blit trace): black tiles 1490 → 650 → 320 over frames 1118→1200, the center-out
iris opening from dead center and settling to the 64px letterbox by ~sim-tick 25 — matching
retail's 240→64 envelope.  **USER: "the iris looks reasonable."**  Host-tested (`test_scene_fade.c`,
5 cases); 919 pass.

**PORT-DEBT(scene-fade-rng-phase):** the iris variant is RNG, but the port's post-spawn LCG phase
isn't retail's yet (the effect spawn doesn't consume the full 238-draw burst — only the idle phases
are ported, ckpt 87), so the drawn variant is wrong (2/sweep) — pinned to the live town value 0
until the spawn-RNG is complete.  And the port arms at `enter_game` (no black-load window), so the
reveal's absolute start tick is offset from retail's by the skipped load frames.  Both resolve in
Phase 2 (whole-scene RNG + load-window timing).  A self-contained subsystem — it will serve every
other room-enter fade.

### USER flag (ckpt 90): 4 characters arrive in front of the wagon at the end of the pan

The USER spotted a parity gap: at the settled town (pan end) **4 characters stand in front of
the wagon + horses** that the port lacks.  Scoped to the town-intro cutscene script
**`FUN_004d7d80`** (`case 0x334be` = room 210110, the same handler that spawns the wagon
`0x1872d`, gated on event flags `0x5f76805`/`0x606aa4f`): right after the wagon it spawns the
arriving party via `FUN_0041f0e0` (a by-code/by-handle spawn helper, distinct from the map
`0x41f200` and the wagon's `0x431d10`):
- `FUN_0041f0e0(0, 0xc3f0, 0x65, 0x6400, 0, 3, …)` — character `0xc3f0`, facing 3.
- `FUN_0041f0e0(0x5f5e1d3, 0, 0x65, 8000, 0, 3, …)` — a HANDLE-spawned actor (code 0, ref `0x5f5e1d3`).
- `FUN_0041f0e0(0x5f5e1d4, 0, 0x65, -3200, 0, 1, …)` — a HANDLE-spawned actor (ref `0x5f5e1d4`), facing 1.
These are the story/party characters arriving with the caravan (the ckpt-83 "`0xc3f0`/`0xc35a`/
`0xc3dc` are script/party-spawned, not in the map" note).  PORT path: the cutscene-spawn helper
`0x41f0e0` (+ the `0x5f5e1dx` handle resolution) → render via the existing actor machinery
(`0x493ba0`/`0x44d160`, already ported).  NB the wagon was the FIRST `0x4d7d80` spawn we
ported (ckpt 80); the party spawns are the rest of the same cutscene.

**PORTED (ckpt 90, partial) — `actor_spawn_cutscene_cast` (`src/actor_spawn.c`).**  Appends
the 3 EFFECT-band script spawns (`0xc35a`/`0xc3dc`/`0xc3f0`) to `g_effects` after the
townsfolk, captured from the settled census (`runs/cutscene-cast`, flip 2400): world pos /
bank / dst / facing / idle phase, rendered by the same `actor_render_static` (layer 13).
**Outcome (USER-reviewed):**
- **`0xc3dc` (bank `0xe3`) + `0xc3f0` (bank `0xeb`) RENDER** — the two town-NPC members
  (near the wagon/horse).  Their facing==3 was initially wrong (mirror flip 0); fixed with
  the in-scene `DAT_008a8440[bank]` read (`flip = 4` for both; chain-field capture
  `runs/flip-probe`).  USER-confirmed the 2 appear; facing corrected.
- **`0xc35a` (bank `0x8b`) CULLS** + the **little girl is absent** — these two are the
  **PLAYER-PARTY characters** (the "woman + little girl on the wagon", USER).  `0xc35a`'s
  bank `0x8b` is NOT in `game_sprites[]` (`ar_register_game_sprites`) — it registers through
  the unported party/character loader, so `game_sprite_resolve` returns NULL and the actor
  culls; the little girl isn't an EFFECT actor at all (a different band — the party renderer
  `0x4997b0` / `0x560e60`).  Confirmed via a port emit probe (`emitted=0` for `0xc35a`, =1
  for the other two).  **DEFERRED — PORT-DEBT(cutscene-party-chars):** rendering the woman +
  girl needs the party-character bank registration (+ likely the party render path), a
  separate arc from the town-NPC EFFECT spawn.  `911 pass`.

### The PARTY-character render path is SCOPED — it reuses `0x493ba0` (ckpt 90)

USER directive: render the woman + little girl (the player party).  Scoped, not yet ported.

- **The party renderer `FUN_004997b0` (150 B) is tiny and reuses the EFFECT renderer.**  It
  walks the **8 party actors** at `room_state+0x4030` (`i=7..0`); for each active one
  (`+0x9c4==1` && `+0x9f4!=0`, skipping the leader `+0x200c`) it calls **`FUN_00493ba0`** —
  the SAME multi-part char renderer the townsfolk + cutscene cast use (the port already
  reuses it as `actor_render_static`).  `param_2!=0` renders just the leader `+0x200c`.  The
  8 party slots are reset by `FUN_00560e60` (42 B) at `0x59f578` inside `game_enter`
  (`0x59f2c0`).  So the party RENDER side is mostly in hand; the work is the SPAWN (where the
  party actors + their banks come from) + wiring `0x4997b0` into the walk.
- **The two front-of-wagon characters (golden crop confirmed = a tall WOMAN in red + a short
  GIRL in pink holding a slate):**
  - **The woman** is the single keyed-blit (`0x5b9b70`) character at the wagon front:
    **res `0x477` frame 5, 40×89, screen dx≈234** (`runs/party-res`, flip 2240).  A plain
    opaque cel → renders fine once her sheet is registered.
  - **The little girl** has **NO keyed blit** at the wagon front — she renders via a richer
    path (alpha/shadow/multi-part, i.e. the protagonist drawn by `0x4997b0`→`0x493ba0`), which
    is why she's absent from the keyed-blit trace.  She IS the controllable-character actor.
- **Bank registration is the real blocker + has a MISMATCH to resolve.**  Banks register from
  `game_sprites[]` (`ar_register_game_sprites`); the rendering NPC banks are there as full
  character entries (`0xe3`→res `0x479` 0x50×0x60 type 2; `0xeb`→`0x79f`).  But the census
  bank for `0xc35a` (`row0`=`0x8b`) maps to a SHORT-form `{139, 0x4fb, …}` (res `0x4fb`, not
  the rendered `0x477`) — so `0x8b` is NOT the woman's render bank; `0xc35a`'s `+0x48` is a
  multi-part/party indirection, and the true bank is whichever `game_sprites[]` id = `0x477`.
  **NEXT (the party arc):** (1) find the bank whose `game_sprites[]` id = `0x477` (the woman)
  + confirm it's registered → spawn her with it (she's already in `g_effects`, just culling on
  the wrong bank); (2) RE the party SPAWN (`0x560e60`-reset slots at `room_state+0x4030`, filled
  during `0x59f2c0` game_enter) + wire `0x4997b0` into `game_actor_walk` for the little girl /
  protagonist; (3) register the protagonist's sheet.  PORT-DEBT(cutscene-party-chars).  This is
  the gateway to the controllable-character milestone (Phase C).  Artifacts: `runs/party-res`
  (the keyed-blit res capture), `runs/cutscene-cast` (the EFFECT census), `runs/flip-probe`.

### CORRECTION (ckpt 91): the ckpt-90 party scope mis-mapped bank→res — the woman is `0xc3f0` (already emits, but as the WRONG sheet) (2026-06-08)

> **⚠ SUPERSEDED on the IDENTITY by "DEFINITIVE (ckpt 91b)" below.** The `bank = idx + 13`
> finding here is correct and stands.  But the woman identity is WRONG: res `0x477` is the
> **MAN** (the NPC right of the horses, rendered CORRECTLY by the port), NOT the woman, and
> there is **no decode bug**.  The woman (Arche's mom) + Arche herself are at `0xc35a`'s
> position (center) — `0xc35a` CULLS (ckpt-90 was right).  This section's "woman = `0xc3f0`,
> decode bug" was a cross-run-flip-misalignment artifact (the three retail runs boot at
> different game_enter flips, so "flip 2300" is a different cutscene moment in each).  Read
> ckpt 91b for the USER-confirmed truth.

The ckpt-90 "PARTY render path" scope above is built on `bank == registration_idx`.
**That is wrong: the runtime bank = registration `idx` + 13.**  Re-verifying every claim
empirically (port blit traces + sheet dumps + retail goldens) overturns the character map.

**The offset, PROVEN.**  `ar_register_group3_sprites`/`ar_register_game_sprites` write the
slot at `g_ar_sprite_slots[idx]` (raw `idx`); the resolver `ar_pool_get_slot(bank)` returns
`g_ar_sprite_slots[bank - (AR_SPRITE_RAMP_COUNT+1)]` = `[bank-13]`.  So **a runtime bank
resolves to the table row whose `idx == bank-13`**.  Ground truth: the foreground TREE
(render_diff-verified bit-exact, ckpt 83b) is `STRUCT_BANK_DEFS` bank `0x15f`=351, and res
`0x481` registers at **idx 338** — `351-338 = 13`.  Confirmed live for the cast banks too
(below).  The ckpt-90 note's `bank=idx` reads (`0xe3`→`0x479`, `0xeb`→`0x79f`, `0x8b`→`0x4fb`)
are all artifacts of the missing `+13` (and one was matched against the *sound* table —
`{139,0x4fb,4}` is `game_sounds[]`, not a sprite).

**The corrected cutscene-cast map** (census `runs/cutscene-cast` banks + the proven offset,
re-confirmed by the port's own settled blit trace at flip 2300):

| actor | census bank | →idx (−13) | res | renders in port? |
|------|------|------|------|------|
| `0xc3dc` | `0xe3`=227 | 214 | **0x473** | yes — a young-man NPC, dx 338 |
| `0xc3f0` | `0xeb`=235 | 222 | **0x477** | **yes — but as the WRONG sheet (see below), dx 514** |
| `0xc35a` | `0x8b`=139 | **126** | — (idx 126 has NO sprite registration) | **no — this is the actual culler** |

So **the woman is `0xc3f0` (res `0x477`), and she ALREADY emits** (via the keyed primitive
`0x5b9b70`, like every other character) — the ckpt-90 note's "the woman is `0xc35a`, culling"
is **inverted**.  `0xc35a` is a *different* character (idx 126 unregistered) that genuinely
culls.

**BUT the woman still doesn't render correctly — a real port DECODE bug.**  Dumping the port's
res `0x477` sheet (env `OPENSUMMONERS_DUMP_BANK`, a static frames-0..7 row) shows **all frames
are a portly MUSTACHED man** (brown vest / orange scarf / green pants).  Retail's res `0x477`
is the **WOMAN in red** — golden-confirmed at flip 2300 (`runs/video60-retail/frame_02300`,
dx 321, the little girl in pink beside her) and by the ckpt-90 predecessor's own party-res
crop.  Retail's res `0x461` is the mustached man (idx 152, bank `0xa5`, far right by the INN
@ dx 575) — and **the port's res `0x477` content == retail's res `0x461` content**.  So the
port's slot 222 (res `0x477`) holds the *man's* pixels, not the woman's.  Slot 222 has the
correct `resource_id` (`0x477`, render_id-confirmed) but the wrong decoded image.

**Leading hypothesis (UNVERIFIED — needs the decompile or a Frida decode dump):** res `0x477`'s
*native* DLL bitmap is the man, and retail **clones the woman's frames INTO slot 222** via an
SS_MGR slot-clone (`FUN_004179b0`, dst pool `0xeb`) — keeping the `0x477` id but swapping the
pixels.  The port's `group3_clones[]` is **incomplete** (the comment claims 94 distinct dsts;
only ~79 rows are present) and has **no dst `0xeb`** → the port keeps the native (man) decode.
To confirm: regenerate the decompile (`tools/ghidra-headless.sh`) and grep `0x57ca40` for a
`FUN_004179b0(0xeb, …)` clone, OR Frida-dump retail's decoded res `0x477` (the
`bs_decode_resource` hook).  The alternative — a resource-id→DLL-bitmap loader error specific
to `0x477` — is less likely (the tree/townsfolk resources decode fine).

**The little girl (Arche)** is NOT an EFFECT actor and has NO keyed blit at the wagon front —
golden-confirmed she stands in pink beside the woman (dx ~345), rendered via the richer party
path (`0x4997b0`→`0x493ba0`, alpha/shadow/multi-part).  Genuinely absent in the port.

**The scene is a party-arrival DIALOGUE cutscene**, not a static tableau: the portrait+textbox
("Arche's Father — Ahh, here we…", the `0x5a00c0` overlay, PORT-DEBT `ingame-nontile-layers`)
plus the party **walking in** (res `0x477` walks dx 234→321 over flips 2240→2300) → the cast
positions are time-varying.  The port's `CUTSCENE_CAST_DEFS` are a single mid-walk snapshot
(`runs/cutscene-cast` flip 2400, rs_x 67200 → screen ~514), so even the *correctly-identified*
actors sit at the wrong (frozen) spots vs the settled dialogue state.

**So the corrected NEXT for the woman + girl:**
1. **The woman** needs the res `0x477` DECODE fixed (the missing clone, most likely), not a new
   bank — she already spawns/emits as `0xc3f0`.  Then her settled position (the walk-in end
   state, not the flip-2400 snapshot).
2. **The little girl** needs the party renderer `0x4997b0` + her sheet registration (Phase C
   gateway), as ckpt 90 said.
3. The `0x5a00c0` dialogue overlay is the other large visible gap (separate debt).

Artifacts (this session, ephemeral): `/tmp/blit_port_settled.jsonl` (port settled cast),
`/tmp/dump_row8.png` (the res `0x477`=man sheet dump), `/tmp/retail2300_band.png` (retail
golden, woman+girl @ dx 321).  Tool: the `OPENSUMMONERS_DUMP_BANK=<bank>` env (needs
`WSLENV` forwarding) spawns a frames-0..7 row of `<bank>` — reusable for sprite-identity Qs
(reverted from `main.c` after use; re-add the ~20-line block in `enter_game` if needed).

### DEFINITIVE (ckpt 91b): res `0x477` is the MAN (no decode bug) — the woman (Arche's mom) + Arche are at `0xc35a`'s spot, which CULLS (2026-06-08, USER-confirmed)

> **⚠ IDENTITIES REFINED by "THE DRAMATIST TABLE (ckpt 92)" below.**  The cull/gap
> conclusion stands, but the cast is now named from ground truth: `0xc35a` is **Arche
> alone** (not "Arche + mom"); the "woman (mom)" is the *distinct* actor `0xc440` bank
> `0xb5`; `0xc3f0`/res `0x477` is **Dr. Barnard**; **Arche's Father (`0xc3dc`) already
> renders**.  See the ckpt-92 section + `docs/proofs/dramatist-table.md`.

The ckpt-91 "decode bug / woman = `0xc3f0`" above is WRONG.  Pinned the cast with
**single-run-consistent** evidence (the prior error was cross-run flip misalignment — the
`cutscene-cast` census, `party-res` trace, and `video60-retail` golden each boot at a
different game_enter flip, so the same flip number is a different cutscene moment in each;
align by the SETTLED state — the cast is stationary once arrived — not by flip).

**The settled cast (census `runs/cutscene-cast`, all positions constant once arrived; bank →
idx = bank−13 → res):**

| code | bank | idx | res | port | settled spot | who |
|------|------|-----|-----|------|------|-----|
| `0xc3e6` | `0xe5` | 216 | `0x475` | renders | far left | townsman |
| `0xc440` | `0xb5` | 168 | `0x467` | renders | left-center | townsman |
| **`0xc35a`** | **`0x8b`** | **126** | **— (idx 126 UNREGISTERED)** | **CULLS** | **center** | **Arche + the woman (mom)** |
| `0xc3dc` | `0xe3` | 214 | `0x473` | renders | center-right | townsman |
| **`0xc3f0`** | **`0xeb`** | **222** | **`0x477`** | **renders (correctly)** | **far right (right of the horses)** | **a MAN (NPC)** |

**USER-confirmed on the feed:** the crop at `0xc35a`'s center spot "correctly shows Arche and
the woman (her mom)"; the crop at `0xc3f0`/res `0x477` (far right) is "the guy to the right of
the horses, correct".  So:
- **res `0x477` = the MAN.**  The port already renders him correctly.  **There is NO decode
  bug** — `sotesd.dll` res `0x477` is the man (port in-game dump + offline `lizsoft_sprite`
  decode both agree), and `sotesd.dll` is the only sprite source (sotesp.dll has no res 1143;
  the EXE's res 1143 is a different `MPED2DT` map-data format, not a sprite).
- **The woman (Arche's mom) + Arche (the protagonist girl) are the missing party characters.**
  `0xc35a` (bank `0x8b` → idx 126) CULLS because idx 126 has no sprite registration — the
  port never loads these party-character sheets.  ckpt-90's "woman = `0xc35a`, culls, a
  player-party character" was RIGHT.

**So the task is exactly the approved plan — port the party/character system (the dramatist
registry + per-character sprite loading + the party band + the cutscene), NO decode fix.**
`0xc35a` is the keyed party actor (the woman, and/or Arche's keyed body); Arche's richer
multi-part render (`0x493ba0` shadow/remap arms) is the other half.  Refine the exact
`0xc35a`-vs-Arche split + each one's true sprite source (bank/resource/module) as Phase-1's
first step (a clean single-run capture hooking the party spawn + `0x556eb0` handle resolve).

**EXE-embedded resources — loading strategy (USER directive, ckpt 91b).**  If a needed
party/character sheet turns out to live in `sotes.exe`'s own `.rsrc` (retail loads some banks
via `FindResourceA(NULL, …)` — see `main.c` init_sprite_banks note, banks `0x570`-`0x572`),
the port must **NOT embed the asset**.  Load it from the user's own files at runtime: either
extract on the fly from the (packed) retail `sotes.exe` — `LoadLibraryEx(sotes.exe,
LOAD_LIBRARY_AS_DATAFILE)` then `FindResourceA`/`bs_decode_resource` (the `.rsrc` survives the
Steam `.bind` DRM, which packs `.text` not resources) — or extract once and **cache under
`%APPDATA%`**.  This keeps the legal line (the port reads the user's binary, never
redistributes assets).  Most town/party sheets are in `sotesd.dll` already; this only applies
to the EXE-`NULL`-module banks.

Artifacts: `runs/extract/{sotesd,sotesp,sotesexe}` (offline PE-resource dumps);
`runs/cutscene-cast` (the settled cast census); feed crops `cs_0xc35a_dx288` (Arche+mom) /
`cs_0xc3f0_dx544` (the man).

### THE DRAMATIST TABLE (ckpt 92): the arrival cast resolved BY NAME — `DAT_006b6ea8` is the handle→character map (2026-06-08, 100%-proven)

Full proof + reproduction: **`docs/proofs/dramatist-table.md`** (`tools/dump_dramatist_table.py`).
Three independent sources agree (static table, decompile control-flow, retail census), so this
is settled ground truth — it NAMES every arrival character and supersedes the census's
"townsman" guesses.

**The mechanism — character identity is a HANDLE, resolved through a "dramatist" registry.**
`FUN_0041f200` (the EFFECT activator) logs `"Get Dramatist Info"` at `:51`, then (`:54-69`) when
spawned with a non-zero **handle** (`param_9`) + code 0 it scans **`DAT_006b6ea8`** (rows of
`0x34` B = `{+0x00 handle, +0x04 code, +0x08 name[0x28], +0x30 bank}`) for the handle and:
- sets the actor's **effective code** `+0x1d4` = the row's **code** (the archetype / sprite-switch
  selector — shared by many NPCs); and
- carries the row's **bank** `+0x30` as `sVar17`, which **overrides** the archetype default sheet
  (each `case`: `if (sVar17==0) sVar17 = <facing default>; FUN_00426d70(0, sVar17, 0)`).

So **code = archetype** (pose/clip set), **bank = the specific sheet**; the handle picks both.
(This is the dramatist/handle registry the plan scoped via `0x556eb0`/`0x555f00` — `0x556eb0`
resolves a handle to a *live actor* in `DAT_008a9b50+0x2790`; `DAT_006b6ea8` is the *static
character definition*.  The cutscene uses BOTH: spawn-by-handle resolves the def, later dialogue
resolves the live actor.)

**The arrival family — cutscene `0x4d7d80:334be` spawns (anchor-relative to the wagon `0x65`):**

```
FUN_00431d10(0,         0x1872d, 0x65, 0x3200,     0,0);            // the wagon (PORTED)
FUN_0041f0e0(0,         0xc3f0,  0x65, 0x6400,     0,3,0,0);        // Dr. Barnard (by code, no handle)
FUN_0041f0e0(0x5f5e1d3, 0,       0x65, 8000,       0,3,0,0);        // handle -> Arche's Father
FUN_0041f0e0(0x5f5e1d4, 0,       0x65, 0xfffff380, 0,1,0,0);        // handle -> Arche's Mother
```

…then runs a walk-in/camera step machine (`in_ECX[…]` writes gated on `FUN_00439680`
yield-until-done) and dialogue (`FUN_00556eb0(0x5f5e1d3)` → `FUN_0049d6e0` "Ahh, here we are at
last!…"), referencing **Arche** (`0x5f5e165`, the persistent party LEADER created at new-game,
NOT spawned here).

**The cast, named + cross-checked vs `runs/cutscene-cast` (settled frame 2550):**

| handle | code | name (dramatist) | bank | rs_x | port today |
|--------|------|------|------|------|------------|
| `0x0c878d35` | `0xc3e6` | **Guard** | `0xe5` | 23800 | renders ✓ |
| **`0x5f5e1d4`** | **`0xc440`** | **Arche's Mother** | **`0xb5`** | 38400 | **WRONG sheet — port spawns the map `0xc440` bank `0xa6`** |
| **`0x5f5e165`** | **`0xc35a`** | **Arche** (clip `0x62a8c8`, banks `0x8b`/`0x8c`/`0x8d`) | `0`→dyn | 41600 | **MISSING — culls (sprite unregistered, party render unported)** |
| **`0x5f5e1d3`** | **`0xc3dc`** | **Arche's Father** | `0xe3` | 49600 | renders ✓ |
| `0x35a4e901` | `0xc3f0` | **Dr. Barnard** | `0xeb` | 67200 | renders ✓ |
| (map obj) | `0xc440` | townswoman (no handle) | `0xa6` | 92600 | renders ✓ |

Decompile-confirmed banks: case `0xc35a` (`:793`) installs `0x8b`/`0x8c`/`0x8d` + clip `0x62a8c8`
(`:908`); case `0xc440` (`:1768`, archetype string **`"Woman"`**) defaults to `0xa6` (facing 1,
no dramatist bank — the map townswoman) but uses `0xb5` for Mom; case `0xc3dc` (`:1386`) is the
man archetype, bank `0xe3`.

**Corrections to ckpt 91b** (its cull conclusion stands; identities were tangled):
- `0xc3f0`/res `0x477` = **Dr. Barnard** (renders), NOT "the woman".  The `CUTSCENE_CAST_DEFS`
  code comment's "0xc3f0 = THE WOMAN / decode bug" is the retracted ckpt-91 error — now fixed.
- **`0xc35a` = Arche alone**, the one true cull.  The "woman (mom)" is the *separate* actor
  `0xc440` bank `0xb5` (same archetype as the map townswoman `0xa6`, different sheet).
- **Arche's Father `0xc3dc` already renders correctly.**  So the wrong/missing arrival
  characters are exactly **Arche** (missing) + **Mom's real sheet** (`0xb5`, mis-rendered).

**Phase-1 port chips made concrete by this:**
1. Port `DAT_006b6ea8` + the `0x41f200:54-69` handle→code/bank resolution + the archetype cases
   `0xc440` (Woman) / `0xc3dc` (man) / `0xc35a` (Arche).
2. Spawn the cutscene family by handle so **Mother gets bank `0xb5`** (not the map default `0xa6`).
3. Register **Arche's banks `0x8b`–`0x8e` + her clip `0x62a8c8`** and render the protagonist (the
   party-leader render — gateway to controllable Arche).

The full table is the game's entire named-NPC roster (Arche/Sana/Stella/Chiffon party leads at
banks 0 = dynamic; teachers, shopkeepers, monsters…) — see the proof doc.

### ARCHE RENDERS — her body banks are EXE-embedded sprites res `0x570`–`0x573` (ckpt 94, USER-confirmed)

The ckpt-93 gap ("all characters except Arche") is **closed**: Arche the party leader now renders
her own sheet.  The whole Phase-2 "party band" framing turned out to be **unnecessary for the
arrival scene** — a live census (`runs/cutscene-cast`, the `0x493ba0` field spec) showed Arche is
drawn by the SAME `0x493ba0` EFFECT path as the rest of the cast, not a separate band:

| field | Arche `0xc35a` (live, settled town) |
|-------|-------------------------------------|
| `row0` bank | **`0x8b`** (sprite slot **126** = bank−13) |
| clip | `0x62a8c8` (decoded byte-identical to the idle clip `0x6290e0`) |
| world (x,y) | (41600, 45600) — x = the wagon anchor; dst (−30,−24); facing 1 |
| layer | 13 (the townsfolk layer) |

So her ONLY blocker was **bank registration**: slot 126 had no sprite (only the numeric-collision
WAVE sound; see below).  A field-spec **chain read** of the live retail slots
(`g_ar_sprite_table[idx]` @ `0x8a7640+idx*4` → slot → `+0x40` resource_id; `runs/arche-res`,
`runs/arche-params`, validated against known slots 168/222/338) pinned her 4 banks:

| bank | slot | resource | w×h | grp/scale/type |
|------|-----:|----------|-----|----------------|
| `0x8b` | 126 | **`0x570`** | 80×80 | 3 / 1 / 2 |
| `0x8c` | 127 | **`0x571`** | 80×80 | 3 / 1 / 2 |
| `0x8d` | 128 | **`0x572`** | 80×96 | 3 / 1 / 2 |
| `0x8e` | 129 | **`0x573`** | 80×80 | 3 / 1 / 2 |

**The key engine fact — these are EXE-embedded sprites with a numeric collision.**  Resources
`0x570`–`0x573` are type **`WAVE` (sounds) in sotesd.dll** but type **`DATA` (sprites) in
sotes.exe's own `.rsrc`**.  The town NPC sheets (res `0x459`–`0x47b`, slots ~161–236) live in
sotesd.dll; the **playable-character** sheets live in the EXE and retail loads them via
`FindResourceA(NULL=the exe, …, "DATA")` (the `init_sprite_banks` note's "EXE-embedded banks
`0x570`-`0x572`").  This collision is exactly what misled ckpt 90 (`0x8b→0x4fb` was the sound
table `game_sounds[139]`, retracted).

**Ported (ckpt 94).**  `ar_register_party_exe_sprites` (asset_register.c) registers slots 126–129
with `settings = g_sotes_exe` (the sotes.exe datafile handle `ar_sprite_decode` reads from — never
embeds the asset, the USER directive), called in `enter_game` after `load_town_scene` opens the
exe.  `actor_spawn_cutscene_cast` gains an Arche row (handle `0x5f5e165`; her dramatist bank is 0 so
`party_resolve_spawn` yields 0 → an explicit `bank_override` `0x8b`, her `0x41f200` case's row-0
install).  She renders via `actor_render_static` (one keyed cel).  **Bit-level confirm** (port blit
trace, settled frame 2200): res `0x570` frame 1 emits at screen (258, 304) — exactly world
(41600,45600) − settled cam + dst (−30,−24).  **USER-confirmed on the live port window: "everyone is
rendering correctly now."**  914 tests pass.  PORT-DEBT(cutscene-party-chars): the static-cast Arche,
not yet the party-band leader (`0x4997b0`) — her multi-part body banks `0x8c`–`0x8e`, the walk-in
roll-in, and the live-actor handle registry (dialogue) remain Phase 2/3.

### The town BUTTERFLIES — `0xe29a` is NOT "wandering villagers" (ckpt 96, USER-confirmed)

USER (golden review #89): tiny butterflies flit by the flowerbeds at the settled town — "over the
dark wood, below the sword/ARMS sign, above the dog" (retail flips ~2028 + 2138; ~3–5 px).  The port
lacked them.  Full ground truth + the mislabel post-mortem are in engine-quirks **#93**; the chase
and port:

- **The chase (live, the methodology's "RE via a render trace at the settled town").**  No existing
  capture hooked the particle band at the settled town, so I drove retail (`--seed-pin --lockstep
  --no-turbo`, input `tests/scenarios/in-game-intro/trace-retail.jsonl`) and captured PNGs +
  field-spec traces at flips 2028/2138.  The particle band (`0x493480`) rendered ONLY the ported
  `0x18704`+`0x18708`; the EFFECT band (`0x493ba0`) only townsfolk/cast — so the butterfly was in
  neither known band.  A **blit trace** (`blt_keyed` `res`/`dx`/`dy`) found it at the butterfly's
  screen pos: **res `0x3fa`, 14×8 cels**.  An **emit trace** (`0x492670` `cel_res`+`ret_va`) named
  the producer: `FUN_00493ba0`, at world positions that match the **`0xe29a`** census 1:1 → the
  "wandering villagers" ARE the butterflies (`runs/butterfly-{census,allbands,blits,emit}`).
- **The asset.**  res `0x3fa` = bank `0x146` (sprite-pool slot **313** = bank−13; **32×32**, ck 0,
  scale 1, type 2, group 3, in **sotesd.dll** — a plain DATA sprite, already group3-registered in the
  port's `group3_sprites[]` at idx 313, just never used).  Clip **`0x65ddf0`** decoded (base 0,
  **3 frames, dur 4, looping, delta {0,1,2}** — wing-flap).  Two colour variants (yellow + white) =
  per-instance frame_base/facing (0/4/8/12).
- **The port (`src/actor_spawn.c`).**  Added `0xe29a` to `TOWN_EFFECT_DEFS` (bank `0x146`, dst (0,0),
  layer 12, facing 1, flip 4) + a `BUTTERFLY_CLIP`; the spawn now selects the per-code clip (butterfly
  vs IDLE_CLIP) BEFORE the `0x426ec0` phase draws (the draw COUNT is unchanged, so the shared LCG
  stays aligned — no townsfolk-phase regression).  `0xe29a` was previously *excluded* (draws consumed,
  not spawned); now it spawns + renders via the existing `actor_render_static` path.
- **Verified.**  919 host tests pass (`test_actor_spawn` updated: `0xe29a`→def found, count 2→3, the
  butterfly slot + flap clip).  Port blit trace: res `0x3fa` emits frames 0/1/2 on-screen as the
  camera pans (frame 1600 @dx 116/180; frame 1850 @dx 491/555) — two yellow butterflies flapping by
  the ARMS sign / dark wood / flowers / dog, matching retail's placement.
- **PORT-DEBT(butterfly-wander):** the per-instance direction/colour (frame_base 0/4/8/12 → the white
  variant) and the RNG wander drift (the 5 `0x427670` draws are consumed but the motion isn't applied)
  — Phase 2, alongside the other scene RNG residuals.

### The area-title BANNER — "Town of Tonkiness" is `FUN_00494a60`, NOT the `0x5a00c0` overlay player (ckpt 100, RE complete, ground-truthed)

The long-known missing "Town of Tonkiness" area banner (the golden-video review #89; PORT-DEBT
`ingame-nontile-layers`).  Fully RE'd + captured.  **It is NOT `0x5a00c0`** (that's the scrolling
story-text / dialogue overlay player — the 3-state `GetTickCount` loop + caption array; it renders the
prologue narration, not the area card).  The area-title banner is a separate, small producer.

**Producer — `FUN_00494a60` (918 B), the area-title banner renderer.**  Called **3× per frame** from
the in-game render driver `FUN_0048c150:176-178` — once per banner slot, ECX = `*(view+0x11c)` /
`*(view+0x120)` / `*(view+0x124)` (view = `*(room_state+0x104c)`, the 0x78-byte view object, quirk #73).
The 3 calls sit **right after the scene-fade REVEAL grid** `FUN_0048e920` (`:175`, the ported
`scene_fade`) and before `FUN_0048c820` — that is the render ORDER for the port (banner draws after the
iris/letterbox).  **Only slot0 (`view+0x11c`) is the area card; slots 1,2 stay `enable=0`** (other
banner instances, unused in the town).

**The banner state object (ECX) layout** (pinned live, `runs/banner-state` field-spec on `0x494a60`):
- `[0]` (`+0`)   = **mode**: `1` = GDI-text card (the area title); `2/3/4` = sprite banner (a scroll
  sprite from the `DAT_008a7718` pool, idx `1/0/2`) — *unused for the town*.
- `[1]` (`+4`)   = `{phase: u16 low, hold_ctr: u16 high}` — the animation phase (0/1/2/3) + hold counter.
- `[2]` (`+8`)   = **alpha** 0..1000.
- `[3]` (`+0xc`) = text-struct ptr (`+0xa` = string length).
- `[4]` (`+0x10`)= **hold duration** = **400** (live).
- `[5]/[6]` (`+0x14/+0x18`) = dst x/y (0/0 for mode 1 — case 1 hard-codes the position; see render).
- `[8]` (`+0x20`)= **enable**.

**Animation — `FUN_00499ab0` (the per-sim-tick cinematic step; runs once per 2 flips, the sim-tick gate;
also updates the `scene_fade` grid).**  For slot0 when `enable!=0`, `switch(mode)`; mode-1 sub-machine on
`phase`:
- **phase 0** — compose the text (`FUN_0049a050` "text ready?" → `FUN_004118b0` free-old), `phase++`.
- **phase 1 (FADE IN)** — `alpha += 0x14` (cap 1000); when `>=1000`, `phase++`.
- **phase 2 (HOLD)**   — `hold_ctr += 1` until `hold_ctr >= [4]` (400), then `phase++`.
- **phase 3 (FADE OUT)** — `alpha -= 0x14` (clamp ≥0); when `<1`, set `enable=0` (done).
- (`0x14`/tick × ½ flip-rate = **+10 alpha/flip** — matches the live ramp 20→60→260→…→1000.)

**Render — `FUN_00494a60` case 1 (mode 1).**  Gets the scroll cel `ppVar5 = FUN_00418470(0)` (res
**`0x449`**, 314×108; loaded lazily via `0x4184a0` when the banner first renders — `runs/banner-probe`
res_load @flip 1511).  If the text isn't composed yet (`*(int*)*DAT_008a7714 == 0`), it **GDI-composes
the area name onto the cel ONCE** (cached): `paint_ctx::FUN_005b94e0` (GetDC) → draw → `FUN_005b9500`
(ReleaseDC).  Then blit at the **hard-coded (0xa0, 0x40) = (160, 64)**: `alpha<1000` → `FUN_00494e10`
(alpha blit via `0x5bd550`, ramp `(&DAT_008a9308)[alpha*0x14/1000]`); `alpha==1000` → `FUN_005b9b70`
(opaque keyed).  Confirmed in `runs/banner-blits`: res `0x449` keyed @ (160,64) wh (314,108) once alpha
hits 1000 (flips ≥1620); during the fade it's the unhooked alpha primitive.

**The GDI text** (`runs/banner-probe/textout.jsonl` — the "Courier New" textout earlier mistaken for a
debug overlay IS this real composition):
- Font `(&DAT_008a9274)[iVar6]`, `iVar6` chosen by string length: `len 11-14`→adv 12; `15-22`→**font idx
  6, adv 10**; `23-28`→idx 8 adv 9; `≥29`→idx 8 adv 8; `≤10`→adv 0xe.  "Town of Tonkiness" = len 17 →
  font 6, **advance 10**.  Live LOGFONT: **face "Courier New", h 20, w 10, weight 400 (normal), italic 0,
  charset 1**.  The bold/outlined look is the multi-offset shadow, NOT a bold weight.
- x centred: `iVar2 = -((len*adv)/2)`, `iVar7 = iVar2 + 0xa0`.  For len 17: iVar2 −85, iVar7 75
  (cel-relative; matches the textout x 73-236).
- **Shadow** (`SetTextColor 0x404040`): the whole string drawn **12×** at x ∈ `iVar7+{-2,-1,0,1}`,
  y ∈ `{13,14,15}` (3 rows × 4 cols, the outline) via `FUN_0048e860` (= the ported `glyph_row_draw`).
- **Fill** (`SetTextColor 0xffffff`): drawn **2×** at x ∈ `{iVar7-1, iVar7}`, y 14.
- The 2nd font pass (`FUN_0048e6d0` / font idx 7) is the **furigana/ruby** pass — **no-op for English**
  (every record's ruby flag is 0; the port's `glyph_ruby_draw` already models this as a no-op).
- `bkmode TRANSPARENT`.  Drawn onto the scroll cel's DC; cel then blits at (160,64) → on-screen text
  centred ≈ x 235-405 (screen centre 320).

**Timing (live, game_enter@~1434, seed-pinned lockstep).**  Banner **arms ~flip 1513** (`= game_enter +
~78`; `enable` 0→1, `mode` 0→1 between flips 1511 and 1515).  Fade-in ≈100 ticks → **alpha 1000 by ~flip
1614**.  HOLD until `hold_ctr` reaches 400 → fades out ~**flip 2422** (well past the settled town 2138 —
the banner is up the whole intro).  Decoded f1 confirmed the model exactly: phase 2, hold_ctr 12→42→139
across flips 1640/1700/1900.

**Port plan (a self-contained `src/banner.{c,h}` producer, like `scene_fade`/`letterbox`).** The port
has ALL the building blocks: `zdd_object_get_dc`/`_release_dc` (= `0x5b94e0`/`0x5b9500`), `glyph_row_draw`
(= `0x48e860`), `ar_make_font` (CreateFontIndirectA), the sprite/alpha blits, and `g_ramp_a` (= the
`0x8a9308` ramp).  Mirror the object + the phase machine + the render; arm in `enter_game` on the measured
timer; render after `scene_fade` in `game_render`.  **Open items for the port:** (a) ~~res `0x449`'s
bank~~ **RESOLVED (disasm):** the mode-1 block `0x494b5a` fetches the scroll cel via `FUN_00418470(0)`
**`__thiscall` with ECX = `DAT_008a7714`** — a DEDICATED bank slot (NOT the main `DAT_008a760c` pool),
frame 0 = res `0x449` (sotesd.dll DATA, 314×108).  `*DAT_008a7714` is the "text composed?" cache flag
(`0x494b62`: `if (*(int*)*DAT_008a7714 == 0)` compose).  `DAT_008a7718` is the separate sprite-banner
pool (modes 2/3/4, unused).  → the port registers a dedicated slot for res `0x449` (like the font slots
`AR_SPR_FONT_TEX_457`), gets frame 0, composes the text onto it once, blits.
(b) the ARM trigger SOURCE (who sets `enable=1`/`mode=1`/the text — measured timer + PORT-DEBT for now,
precedent = the camera-pan/letterbox triggers, all the same scene-script source); (c) the area-name
source (area table `0xd2` → "Town of Tonkiness" — read from the table or hard-code for the town with a
PORT-DEBT).  Ground truth: `runs/banner-probe` (PNGs/textout/res), `runs/banner-state` (3-slot config),
`runs/banner-blits` (the res-0x449 blit), `tools/flow/banner_fields.json` (the reusable field-spec).

**PORTED + BIT-EXACT (ckpt 101, USER-confirmed "banner looks good").**  `src/banner.{c,h}` (NEW, pure +
host-tested: `banner_arm`/`banner_step` = the `0x499ab0` mode-1 phase machine, `banner_text_layout`/
`banner_alpha_ramp_index` = the `0x494a60` case-1 length+alpha math) + `main.c` wiring (the scroll-cel
fetch via slot 53, the GDI text compose `zdd_object_get_dc`→`SetTextColor`/`SelectObject`/`TextOutA`→
`zdd_object_release_dc`, the keyed/alpha blit at (160,64), armed at game_enter+78, stepped + rendered in
the sim-tick block after `scene_fade`).  **VERIFIED `differ_px=0/36720`** over the whole banner region
(scroll + vines + GDI text + sky behind), camera-matched (port frame 1300 vs retail `runs/banner-verify`
1614, both `cam_x60=128000`); amplified diff all-black.
- **The decisive fix — the scroll decodes UNGRADED.**  res `0x449` is bound by retail's PLAIN getter
  `0x418470(0)` (NO `0x417c40` grade descriptor), so its 8bpp palette is NOT colour-graded.  The port's
  8bpp grade was global → the parchment rendered ~10% too dark (a uniform per-channel offset; the GDI
  text + the sky behind were already bit-exact).  Skipping the grade for slot 53 (the decode hook) →
  parchment `differ_px=0`.  General gate (PORT-DEBT `banner-grade`): grade only via the `0x417c40`
  getter; the slot-53 skip is the stand-in.
- **Residuals (named, outside the bit-exact window):** the arm trigger + the text + the hold are
  measured constants (PORT-DEBT `banner-trigger`, the same scene-script source as the camera-pan +
  letterbox); only the font-6 length band is built (PORT-DEBT `banner-font-table`).
- 5 host tests (`tests/test_banner.c`: arm / fade-in / hold→fade-out / layout ladder / alpha-ramp gate)
  — 933 pass.  parity-ledger #11; engine-quirk #96.  Artifacts: `runs/banner-verify` (the matched
  retail cap).

## The DIALOGUE BOX subsystem (ckpt 102 RE) — chips/plan in `plans/dialogue-cutscene.md`

The town-arrival **dialogue** (portrait + name + textbox, Z-advanced; golden ~flip 2200+)
is driven by a **linear cutscene coroutine**, fully RE'd here.  All render primitives it
needs are ALREADY in the port.

- **Script** `0x4d7d80` case `0x334be` (scene-id `*(0x8a9b50+0x1038)`, area `0xd2`; called
  by the dispatcher `0x40c380`).  Linear sequence of beats: configure a beat on the
  scene-controller, then `FUN_00439680()` (9-byte thunk → the beat-runner `0x439690`) which
  BLOCKS until the beat completes; `ret == 6` = Escape/abort → the script unwinds.
- **Beat-runner** `0x439690` (8866 B): on the dialogue dirty flag rebuilds the box widget
  tree, runs a `GetTickCount`-paced frame-pump (each frame → `0x48c150` render + `0x5b8fc0`
  flip + `0x5b1030` pump), and returns when the beat's completion holds.  Beat-type switch
  @ `:1128` — `1`=dialogue (advance via `0x43b980` Z-poll), `2`=flag `+0x24`, `3`=camera at
  target (`*(0x8a9b50+0x104c)` cur==tgt), `4`=entity flag, `6`=timer `+0x57c`≤0, `7`=portrait
  anim/`+0x57c`, `9`=entity `+0x20`.
- **Dialogue-line setup** `0x49d6e0`: writes the controller's dialogue fields — text→`+0x8a`,
  name (actor `+0x750`)→`+0x28a`, voice→`+0x2e8`, dirty `+0x78=1`, beat `+0x20=1`; resolves
  the PORTRAIT id into `+0x84` via the face table **`DAT_006b6568`** (stride 0x10, key
  `[actor head-state +0x1d8, face_id]`, frame `+8/+10/+12` by the actor anim-state).
- **Render path (primitives all ported):** `0x48c150` → `0x48c820` (widget-tree walk, 873 B,
  NOT yet ported) → `0x48cf80` (9-slice FRAME = `src/newgame_box.c`, box res **`0x456`** slot
  `DAT_008a7708`, 0x20×0x20) + `0x48e200` (GDI text = `src/glyph_render.c`, **Courier New**
  via `0x579f40`, 7×16 slot `DAT_008a927c`, +1/+1 shadow, 7-px advance).  Portrait =
  `(&DAT_008a760c)[+0x84]` set by `0x415860`, faded by `0x49c910`, positioned by `0x49c640`.
- **Live speaker** `0x556eb0(handle)` scans the actor registry `*(0x8a9b50+0x2790)` for
  `actor+0x1d8 == handle`.  Town handles: Father `0x5f5e1d3`, Arche `0x5f5e165`, Mother
  `0x5f5e1d4`, Sana `0x5f5e166`.
- **Text = story content** in the user's exe `.rdata` (line 1 @ VA `0x86d58c`, voices
  `0x3eb`–`0x3f4`).  The port reads these from the user's `sotes.exe` at runtime by VA —
  NEVER embedded in source (dramatist-table precedent + legal line).
- **Object model (to confirm):** `0x439680` tail-calls with no stack arg ⇒ the runner's
  `param_1` and `in_ECX` are almost certainly the SAME `this` (a single scene-controller
  holding beat + dialogue + widget state).
