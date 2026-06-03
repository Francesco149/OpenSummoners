# In-game intro — the scene past the prologue (map 0x3f2)

Milestone 2 (the game proper).  The user opted to **extend the trace in-game**
(ckpt 48): spam Z after the prologue begins, capture ~1 min of retail frames,
then port to match.  This doc is the living plan + engine survey.

**Status (ckpt 53):** the foundational plumbing is in place — the **game-entry
anchor** (both sides), the **port seam** (`PROLOGUE_DONE → enter_game`), **plan 3a
RESOLVED** (the town's resource banks identified + the boot batches g2/g3/g5 wired),
the **`game_drive` scaffold is stood up** (plan 3b's first bullet), and now the
**static world tables are extracted + decoded** (ckpt 53 — see "The static world
tables" below): the AREA name table + the 416-room registry the engine copies in,
with field meanings and the opening-town entry identified.  The in-game **engine**
(`0x59f2c0` + its two giant children `0x586010`/`0x5a00c0`) is surveyed but
**unported** — that is the remaining body of milestone 2 (plan 3b: port the world
construction reading these tables, then a slice of `0x5a00c0` for the static town
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

The 33 areas ARE Fortune Summoners' world: `0x6e` **"Town of Tolkien"** (the
opening town — the handoff's earlier "Tilelia" was a wrong guess; the binary
says Tolkien / SJIS トルーキン), Silver Dungeon, Minasa-Ratis Magic School,
Mountain Cave, Shrine of the Wind, Barness Village, Chartreux City, … Labyrith
of Night.  (Several ids have an empty name = an unnamed sub-area.)

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
| 8 | `parent` | a `ROOM.id` (prev/parent room; 999999 = root sentinel) |
| 9 | `d9` | ordinal within parent |
| 0x46 (+0x118) | `sjis` | Shift-JIS room name (cp932), e.g. room 110110 = "トルーキンの町 １丁目" (Town of Tolkien, district 1) |

The `parent`/`d9` links form the room-transition graph (the per-area room list).
Entries 70-75 of some rooms hold packed non-id blobs (e.g. room 1 = the town
entry has 6 dwords there) — per-room extra data, decode deferred to the sim port.

**OPEN — how `0x3f2` (=1010) selects the first room.**  The engine is entered
with map `0x3f2` = 1010, stored into map-object `+0x4104`.  But **no room has
`scene` == 1010 or 1011** (they jump 1009→1012), so `0x3f2` is NOT a `ROOM.scene`
value — it's a separate scene-load id the room loop (`FUN_00561c90`, "fetch the
active room/scene record") resolves to the town's first room (110110, `scene`
1002).  Pinning that resolution (is `0x3f2` an absolute scene-resource id? a
`scene`+offset? a separate index?) is the **first task of the room-loop port** —
trace `0x561c90` with the world populated, map 0x3f2 → the selected `ROOM.id`.

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
