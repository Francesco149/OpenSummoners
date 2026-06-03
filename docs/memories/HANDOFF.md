# Session handoff — last updated 2026-06-04 (ckpt 56 — RUNTIME MAP-DATA LOAD PATH + FORMAT RESOLVED, and `FUN_00587970` PORTED + host-tested.  Surveying the next unit (`0x586010` sim → `0x5a00c0` render) showed both are multi-checkpoint rocks (the full engine sim; a self-contained scripted-scene render loop with its own GetTickCount pace machine + caption/sprite arrays + resource-unload teardown).  The tractable next data-layer unit is the **runtime MAP DATA**: the per-room VISUAL map (tilemap cell grid + 86 object/layer entries — the town backdrop) is **loaded from a PE DATA resource keyed by the room's SCENE index, sourced from the main EXE** (`DAT_008a6e7c` = the EXE module handle, NOT sotesd).  `0x586010:690` calls **`FUN_00587970(EXE, room.scene)`** → **`FUN_005b62a0`** = `FindResourceA(EXE, scene&0xffff, "DATA")` + LoadResource/LockResource → **`FUN_005b6340`** mode-1 sequential copy.  Opening town: room 210110 scene **1022** → **DATA resource 1022 in the EXE** (152,936 B, name **"MSD_SOTES_MAPDATA"**, **dims 88×19×3**, **86 layers**).  This REFINES plan 3a (ckpt 51 was incomplete): the res-probe hooked only the sprite decoder `0x5b7800`, so it never saw this separate FindResource path — "compiled-in" is true only of the ROOM REGISTRY (.rdata graph/names/scene-ids); the per-room visual map IS a loaded EXE resource.  Pairs with the EXE-NULL banks 0x570-0x572: the port loads both from the original `sotes.exe` as a datafile (`g_sotes_exe`).  PORT: new tool **`tools/extract/map_data.py`** (decode any map DATA resource; asserts EXACT consumption) + pure host-tested **`src/map_data.{c,h}`** porting `FUN_00587970`'s parse (caller supplies locked bytes; decodes magic+0x30 header+0x34 maphdr{name,dims,count} + `dim0*dim1*dim2`×0x1c cell array + `count` layer entries each 0x3c header + 4 sized sub-arrays into owned allocations; overrun-guarded).  4 new host tests → **770 pass / 0 fail / 6 skip**.  Ledger **184/1490** (`0x587970` now tested).  `map_data.c` is in the src wildcard, not yet wired into main.c (foundation for the FUN_00587e00 decoder).  Full writeup: docs/findings/in-game-intro.md "The RUNTIME MAP DATA".  NEXT: port **`FUN_00587e00`** (3282 B, the map-data → world decode reading this parsed structure) + the matching `0x5a00c0` render slice, diff vs `runs/tas-ingame-1`.  Clean /clear point.)

> **ckpt 56 — THE RUNTIME MAP-DATA LOAD PATH + FORMAT ARE RESOLVED; the
> `FUN_00587970` parser is PORTED + host-tested.**  Grounding + a small,
> verifiable data-layer unit on the way to the town backdrop — after surveying
> the sim/render and finding both are multi-checkpoint rocks.
>
> **THE SURVEY (why this unit, not the whole render).**  `0x586010` (6 KB) is the
> full engine sim — it allocs the `0x27b8` room-state object `DAT_008a9b50`,
> creates the party actors, loads the map, runs the event system (`0x40b8f0`) +
> per-frame step (`0x58f360`).  `0x5a00c0` (13.7 KB) is NOT a simple backdrop
> blitter: it's a self-contained **blocking scripted-scene player** with its own
> 3-state GetTickCount pace machine, a sprite-descriptor array, a caption/text
> line array (0x124 stride, glyph-index strings), and a full resource-unload
> teardown path.  Both are multi-checkpoint.  The tractable next unit is the
> **map-data load** the sim performs.
>
> **KEY RE FINDING — the town backdrop is a PE DATA resource in the EXE, keyed by
> scene index.**  `0x586010:675-697`: `local_920 = DAT_008a6e7c` (the **EXE
> module handle**, one of the boot slots `0x8a6e68..7c`), then
> `FUN_00587970(local_920, (u16)room[3])` where `room[3] = the SCENE index`.
> `FUN_00587970` opens via **`FUN_005b62a0`** = `FindResourceA(module,
> scene&0xffff, "DATA")` + LoadResource + LockResource, and copies it out with
> **`FUN_005b6340`** mode 1 (memcpy from the locked pointer).  Room 210110's
> **scene = 1022**, so the opening town map = **`FindResourceA(EXE, 1022,
> "DATA")` = DATA resource 1022 in the EXE** (152,936 B, name
> **"MSD_SOTES_MAPDATA"**).
>
> **REFINES plan 3a (ckpt 51 incomplete, not wrong).**  The ckpt-51 res-probe
> hooked only the **sprite** decoder `bs_decode_resource` (`0x5b7800`);
> `FUN_005b62a0` is a **separate FindResource path** it never observed.  So "map
> layout is compiled-in static data" holds only for the **ROOM REGISTRY** (the
> room graph / names / scene indices in `.rdata` — `game_world_tables.py`); the
> per-room **visual map** (tiles + object layers) **is** a loaded resource,
> sourced from the **EXE** (a module plan 3a didn't check) and keyed by scene
> index.  Pairs with the ckpt-51/52 **EXE-NULL banks `0x570-0x572`**: the port
> must load both the EXE-NULL sprite banks AND the scene-indexed map data from
> the original `sotes.exe` as a datafile (one `g_sotes_exe` handle).
>
> **MAP-DATA FORMAT (decoded + bit-exactly validated).**  Sequential from
> offset 0: `[0:4]` magic (`0x30`); `[4:0x34]` 0x30-byte header; `[0x34:0x68]`
> maphdr (`+0x00` char[0x20] name · `+0x20/+0x24/+0x28` dims · `+0x2c` count);
> `[0x68:..]` cells = `dim0*dim1*dim2` × `0x1c` B; then `count` layer entries,
> each a `0x3c` header (sub counts `+0x1c`/`+0x20`/`+0x24`/`+0x28`, strides
> 4/0xc/0x100/8) + those four sub-arrays.  DATA 1022 = dims **88×19×3**, **86**
> layers; the parse consumes the resource **EXACTLY** (152936, zero remainder) —
> the trust invariant.
>
> **PORT (pure, host-tested):** new **`tools/extract/map_data.py`** (extract +
> decode any map DATA resource; asserts exact consumption — re-runnable ground
> truth, the ckpt-53 pattern) + new **`src/map_data.{c,h}`** porting
> `FUN_00587970`'s parse (caller supplies the locked bytes — FindResource stays
> Win32 in `main.c`; `map_data_parse` decodes into owned allocations; overrun
> guard vs `len`, inert on a well-formed map).  The `0x1c`-byte cell record +
> the layer sub-array element layouts are decoded by the unported
> **`FUN_00587e00`** (3282 B); this parser preserves their raw bytes.
>
> **STATE:** 4 new host tests (`tests/test_map_data.c`, synthetic blobs) →
> **770 pass / 0 fail / 6 skip** (+4).  Ledger **184/1490** (`0x587970` now
> *tested*).  `map_data.c` is in the `src` wildcard but **not yet wired into
> `main.c`** (the foundation the `FUN_00587e00` decoder + render read).  Full
> writeup: `docs/findings/in-game-intro.md` "The RUNTIME MAP DATA".  **NEXT:**
> port **`FUN_00587e00`** (the map-data → world decode reading this structure) +
> the matching slice of `0x5a00c0` to render the town backdrop, diff vs
> `runs/tas-ingame-1` anchored on `game_enter`.  Clean **/clear point**.
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 55 — IN-GAME MAP OBJECT PORTED + host-tested.  New pure, host-tested **`src/game_map.{c,h}`** ports the runtime map object the in-game engine builds on a fresh new-game entry: the `in_stack_0000eb2c==0` arm of **`FUN_0059f2c0`** (lines 160-218) + the **`FUN_004c5350`** room-key resolution.  Allocates the `operator_new(0x4120)` map object (zero-init `buf[0x4120]`) + **8× `operator_new(0xeec)`** actor slots, runs the actor init loop (slot index @+0xa0c, **`FUN_00560e60`** field-zero, active flag `map+0x4084+4*i=1`), writes the header fields (`+0x40a4=1`,`+0x4018=1`,**`+0x4054=3`**, the `+0x405c..+0x4064` u16 run, the 3 `{1,0}` tail pairs @+0x4108 that fill the object to +0x4120, the `GetTickCount` stamp @+0x4068), sets `map+0x4104=0x3f2`, then `FUN_004c5350`'s `map==0x3f2` arm writes the **room key `map+0x4024=0x334be`** (+ spawn `+0x4028=0x65`/`+0x402c=1`).  `game_map_active_room` resolves it via `game_world_find_room` → **room 210110 "Town of Tonkiness"** (area 0xd2, scene 1022) — the end-to-end opening-room build over the ckpt-54 table layer.  Fidelity boundaries documented in the header (zero-init baseline; `+0x4020` ramp ceiling set by an opaque sub-init so the `+0x4014` ramp is inert; sprite-registry sub-calls + the `0x3fc` arm skipped; the `map==0→0x3f2` default runs AFTER `FUN_004c5350` so a 0-map gets no key — real path passes 0x3f2).  6 new host tests → **766 pass / 0 fail / 6 skip**.  Ledger **180/1490** (`0x4c5350`,`0x560e60` now tested).  Both port builds compile clean; `game_map.c` is in the src wildcard but NOT yet called by main.c (foundation for the sim/render units).  Full writeup: docs/findings/in-game-intro.md "The MAP OBJECT".  NEXT: a slice of `0x586010` (sim) → `0x5a00c0` (render) that walks this map object + room 210110 to draw the static town backdrop, diff vs `runs/tas-ingame-1`.  Clean /clear point.)

> **ckpt 55 — THE IN-GAME MAP OBJECT IS PORTED + HOST-TESTED.**  The runtime
> world object the engine builds on a fresh new-game entry, on top of the
> ckpt-54 table layer; the next unit (the sim/render slice) reads it.
>
> **PORT:** new **`src/game_map.{c,h}`** — the `in_stack_0000eb2c==0` arm of
> **`FUN_0059f2c0`** (lines 160-218) + the **`FUN_004c5350`** room-key resolver:
> - `operator_new(0x4120)` map object (zero-init `buf[0x4120]`) + **8×
>   `operator_new(0xeec)`** actor sub-objects (kept in `actors[8]` — host
>   pointers don't fit the 4-byte `map+0x4030` slots).
> - the 8-actor loop (`0x59f2c0:162-170`): slot index @`slot+0xa0c`,
>   **`FUN_00560e60`** zeros the slot bookkeeping, per-slot active flag
>   `map+0x4084+4*i = 1`.
> - the header writes (`:171-214`): `+0x40a4=1`, `+0x4018=1`, **`+0x4054=3`**,
>   the `+0x405c..+0x4064` u16 run (last `+0x4064` *dword* wins → `=0`), the 3
>   `{1,0}` dword pairs @`+0x4108` (which fill the object exactly to `+0x4120`),
>   the `GetTickCount` stamp @`+0x4068` (port: a `tick` arg, default 0).
> - `map+0x4104 = 0x3f2`, then **`FUN_004c5350`**'s `map==0x3f2` arm writes the
>   **room-lookup key `map+0x4024 = 0x334be`** (= room 210110), spawn params
>   `+0x4028=0x65`/`+0x402c=1`, `+0x401c=0`, `+0x40d0=0`.
> `game_map_active_room(m,w)` = `game_world_find_room(w, map+0x4024)` →
> **room 210110 "Town of Tonkiness"** (area 0xd2, scene 1022).
>
> **FIDELITY BOUNDARIES (in the header, not hidden):** map buffer is
> zero-initialised (retail's `operator_new` is raw + relies on the explicit
> writes + opaque sub-inits `0x5612b0`/`0x5611d0`/`0x4e59a0`); `map+0x4020` (the
> `+0x4014` ramp ceiling) is one of those sub-inits' outputs, so the ramp is
> inert under zero-init and does NOT affect the verified room key.
> `FUN_004c5350`'s sprite-registry sub-calls (`0x408dc0`/`0x413b20`/…) + its
> `0x3fc` arm (gated by unported save-flag state, `0x4c57f0`) are skipped; the
> `map==1` arm's pure writes are kept.  **Ordering note preserved:** the
> `map==0 → 0x3f2` default (`0x59f2c0:378-381`) runs *after* `FUN_004c5350`, so a
> `0`-map fresh entry gets `+0x4104=0x3f2` but NO `+0x4024` key from the resolver
> — the real path always passes `0x3f2` (`0x59ec30(0,0,0x3f2)`).
>
> **STATE:** 6 new host tests (`tests/test_game_map.c`) → **766 pass / 0 fail /
> 6 skip** (+6).  Ledger **180/1490** (`0x4c5350`, `0x560e60` now *tested*;
> `0x59f2c0` was already touched via the ckpt-54 `game_world` header).  Both port
> builds (mingw GUI + debug) compile clean — `game_map.c` is in the `src`
> wildcard but **not yet called by `main.c`** (it is the world-runtime
> foundation the unported sim/render units read, same status as `game_world`).
> Full writeup: `docs/findings/in-game-intro.md` "The MAP OBJECT".  **NEXT:** a
> slice of **`0x586010`** (sim) → **`0x5a00c0`** (render) that walks this map
> object + room 210110 to draw the static town backdrop, diff vs
> `runs/tas-ingame-1` anchored on `game_enter`.  Clean **/clear point**.
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 54 — WORLD-TABLE LAYER PORTED + host-tested, and the `0x3f2`→opening-room resolution RESOLVED (correcting ckpt 53).  New pure, host-tested **`src/game_world.{c,h}`** ports the data half of the in-game engine's fresh-entry world construction: the AREA/ROOM registry build (`0x59f2c0:122-144`'s zero-terminated copy), **`FUN_00585000`** (per-room cross-reference — part 1 fills area defaults `room[0x43/0x44/0x45/0x50/0x51]` from the matching AREA entry; part 2 builds the **reciprocal room-transition exits** scanning every other room's 0x14 exit slots), and **`FUN_00561c90`** (room lookup = linear search of the copied region by `ROOM.id`/dword0).  Tables ship as generated bytes `src/world_tables_data.{c,h}` (new `game_world_tables.py --emit-c` lifts them verbatim from the EXE `.rdata`).  **KEY RE FINDING — map `0x3f2` → room 210110 "Town of Tonkiness", NOT 110110 "Town of Tolkien" (ckpt 53 was WRONG).**  Disasm of the room loop (`59fd8b: mov edi,[ebx+0x4024]; push edi; call 0x561c90`) shows the lookup key is map-object **`+0x4024`** — set by **`FUN_004c5350`** (jump table on `*(map+0x4104)`: `map==0x3f2 → 0x4c5516` writes `+0x4024=0x334be`, `+0x4028=0x65`, `+0x402c=1`).  **`0x334be` = decimal 210110** = registry entry **[61]**: id 210110, area `0xd2` **"Town of Tonkiness"**, **scene 1022**.  Tolkien (area 0x6e, room 110110) is a *different, later* town.  `+0x4028`/`+0x402c` are entry spawn params; 7 rooms carry exits targeting 210110 (a hub).  6 new host tests → **760 pass / 0 fail / 6 skip**.  Ledger **178/1490** (`0x585000`,`0x561c90` now tested).  Port builds clean (game_world.c + world_tables_data.c pulled into the src wildcard, not yet called by main.c).  Full writeup: docs/findings/in-game-intro.md "ROOM lookup" + "ROOM registry".  NEXT: build the map object (`0x59f2c0` fresh-entry arm — the 0x4120 alloc + field init + 8 actor slots) on this table layer, then a `0x586010`/`0x5a00c0` slice to render room 210110's static backdrop, diff vs `runs/tas-ingame-1`.  Clean /clear point.)

> **ckpt 54 — THE WORLD-TABLE LAYER IS PORTED + HOST-TESTED; map `0x3f2`
> resolves to room 210110 "Town of Tonkiness" (ckpt-53 "Tolkien" was WRONG).**
> The data foundation the in-game engine reads, ported as pure host-tested C,
> plus the room-lookup chain pinned from disassembly.
>
> **PORT:** new **`src/game_world.{c,h}`** — the data half of `0x59f2c0`'s
> fresh-entry world construction:
> - **registry build** (`0x59f2c0:122-144`): count the zero-terminated rooms
>   (417 incl. the `[0]` header sentinel `0xf423f`), copy them, copy the AREA
>   table pointer, run the cross-reference per room.
> - **`FUN_00585000`** (the cross-reference): *part 1* finds the room's AREA
>   (by `ROOM.area == AREA.id`) and fills still-zero defaults
>   `room[0x43/0x44/0x45/0x50/0x51-lo/0x51-hi]` from area dwords C/A/B/D/E/F;
>   *part 2* builds the **reciprocal room-transition exits** — scans every other
>   room's 0x14 exit slots (stride 3 dwords from dw7) for one targeting this
>   room and mirrors it back in.
> - **`FUN_00561c90`** (`game_world_find_room`): linear search of the copied
>   region by `ROOM.id` (dword0), header included.
> Tables ship as generated `src/world_tables_data.{c,h}` (verbatim `.rdata`
> bytes; new `game_world_tables.py --emit-c`).  Pure + host-tested; main.c does
> NOT call it yet (it's the foundation the map-object/sim/render units read).
>
> **KEY RE FINDING (corrects ckpt 53) — how `0x3f2` selects the opening room.**
> `0x3f2` is NOT a room id and NOT a scene index.  The room loop
> (`0x59f2c0` `LAB_0059fd85`) passes `FUN_00561c90` the key in map-object
> **`+0x4024`** (disasm `59fd8b: mov edi,[ebx+0x4024]; push edi`), which
> **`FUN_004c5350`** sets via a jump table on `*(map+0x4104)`:
> `map==1 → 0x4c5614`, **`map==0x3f2 → 0x4c5516`** (writes `+0x4024 = 0x334be`,
> `+0x4028 = 0x65`, `+0x402c = 1`), `map==0x3fc → 0x30db3`.  **`0x334be` =
> decimal 210110** = ROOM registry entry **[61]**: id 210110, area key **`0xd2`
> = "Town of Tonkiness"**, **scene 1022**.  So the opening map renders room
> **210110 "Town of Tonkiness"** — the ckpt-53 "Town of Tolkien" (area 0x6e,
> room 110110) and earlier "Tilelia" identifications were BOTH wrong (Tolkien is
> a different, later town).  `+0x4028`/`+0x402c` are the entry spawn params, not
> the room key.  Seven rooms (62,63,66,68,70,77,129) carry exits targeting
> 210110 → it is a town hub.  Host-tested: `game_world_map_3f2_opening_room`.
> **Caveat:** the engine *resolution* to room 210110 is proven from the binary;
> that this is what RENDERS at golden flip ~1150 (vs an immediate scripted
> transition) is a render-time fact to re-confirm when `0x5a00c0` is ported.
>
> **STATE:** 6 new host tests (`tests/test_game_world.c`) → **760 pass / 0 fail /
> 6 skip** (+7).  Ledger **178/1490** (`0x585000`, `0x561c90` now *tested*).
> Port builds clean.  Full writeup: `docs/findings/in-game-intro.md` ("ROOM
> lookup", "ROOM registry", "The static world tables").  **NEXT:** port the
> `0x59f2c0` fresh-entry arm's **map object** (the `operator_new(0x4120)` alloc +
> field init at `+0x4018/+0x4054/+0x4068/+0x4104` + the 8× `0xeec` actor slots),
> wiring it to this `game_world`; then a slice of `0x586010` (sim) + `0x5a00c0`
> (render) for room 210110's static backdrop, diff vs `runs/tas-ingame-1`
> anchored on `game_enter`.  Clean **/clear point**.
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 53 — STATIC WORLD TABLES extracted + decoded.  Two compiled-in `.rdata` tables `0x59f2c0:122-144` copies into `scene[4]`: AREA name table (`&DAT_00693848`, 0x40-stride, 33 areas) + ROOM registry (`&DAT_006940c8`, 0x150-stride, 416 rooms + header sentinel `0xf423f`).  Tool `tools/extract/game_world_tables.py`.  NOTE: this ckpt's "opening town = Town of Tolkien / room 110110" was CORRECTED at ckpt 54 → the opening room is 210110 "Town of Tonkiness".  Full writeup: docs/findings/in-game-intro.md.)

> **ckpt 53 — THE STATIC WORLD TABLES ARE EXTRACTED + DECODED** (plan 3b
> groundwork).  Verifiable RE: the complete compiled-in map database the in-game
> engine reads.  No runtime C ported — this de-risks + grounds the world-
> construction port that comes next (like the ckpt-49 golden / ckpt-50 survey).
>
> **WHAT.**  `0x59f2c0:122-144` builds the world's `scene[4]` object from two
> `.rdata` tables: `scene[4][0] = &DAT_00693848` (AREA) and a copy of every
> `&DAT_006940c8` entry (ROOM) into `scene[4][1..]`, cross-referenced by
> `0x585000`.  Decoded both out of `vendor/unpacked/sotes.unpacked.exe`:
> - **AREA table `&DAT_00693848`** — 0x40-byte stride, zero-terminated, **33
>   entries** = Fortune Summoners' areas: `0x6e` "Town of Tolkien", `0x82` Silver
>   Dungeon, `0xe6` Minasa-Ratis Magic School, … `0x1c2` Labyrith of Night.  Each
>   carries 6 small dwords `0x585000` fans into the room (per-room defaults).
> - **ROOM registry `&DAT_006940c8`** — 0x150-byte (0x54-dword) stride, ends at
>   the first `dword0==0`; entry `[0]` is a header sentinel (`0xf423f`), so
>   **416 real rooms**.  Sparse (most of each entry set live by the sim); the
>   populated fields = {`id` packed e.g. 110110, `area`→AREA name, `scene`
>   sequential index 1002/1004/…, `parent` (a room id — the transition graph),
>   `d9` ordinal, **SJIS name @+0x118** e.g. room 110110 = "トルーキンの町 １丁目"}.
>
> **TOOL (committed):** `tools/extract/game_world_tables.py` — PE VA→offset map,
> decodes + lists both tables, `--raw N` hex-dumps ROOM entry N.  Re-runnable
> ground truth.
>
> **DOC FIX:** the opening town is **"Town of Tolkien"** (SJIS トルーキン), NOT
> "Tilelia" — that earlier name was an unverified guess, now corrected from the
> binary.
>
> **OPEN THREAD (the first room-loop-port task):** the engine is entered with map
> `0x3f2` = 1010, but **no room has `scene` 1010/1011** (the sequence jumps
> 1009→1012).  So `0x3f2` is a separate scene-load id the room loop `0x561c90`
> ("fetch the active room/scene record") resolves to the town's first room
> (110110, `scene` 1002).  Trace `0x561c90` with the world populated to pin how
> `0x3f2` → `ROOM.id`.
>
> **STATE:** pure groundwork — 753 host tests pass UNCHANGED (no `src/` change),
> ledger **175/1490 UNCHANGED**.  Full writeup: `docs/findings/in-game-intro.md`
> "The static world tables (plan 3b groundwork, ckpt 53)".  **NEXT:** port the
> `0x59f2c0` fresh-entry arm (world construction) into a `game_world` model
> reading these tables + the 8 actor slots; then the `0x3f2`→room resolution;
> then a slice of `0x5a00c0` for the static town backdrop, diff vs
> `runs/tas-ingame-1`.  Clean **/clear point**.
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 52 — GAME_DRIVE SCAFFOLD stood up (plan 3b's first bullet).  `enter_game` no longer re-displays the title — it stands up a new `game_drive` (`src/game_drive.{c,h}`, the milestone-2 counterpart of prologue_drive): owns the in-game input ring, and `main_loop_body` runs one `game_drive_step` per presented frame.  The in-game ENGINE (0x59f2c0 setup + 0x586010 sim + 0x5a00c0 render) is still unported, so a step renders the faithful BLACK map-load frame (`game_render`=zdd_object_clear) — the state retail shows from game_enter (~flip 1092) to the first town frame (~1150) while the engine loads the map + fades (golden flips 900-1100 black).  VERIFIED LIVE (trace-port, --frames 1300): game_enter@1116 rng 0x40d00581 (matches retail @1092); port runs game_drive to 1300 without re-displaying title; captures frame 400 (title phase6)=full content, frames 1160/1200 (in-game phase=-1)=fully black → early in-game frames now match retail's black entry.  3 host tests (test_game_drive.c) → 753 pass.  Ledger 175/1490 unchanged (scaffold/seam, no new FUN).  Also recorded: EXE-NULL banks 0x570-0x572 CONFIRMED present in sotes.unpacked.exe .rsrc (type=DATA); port must load via LoadLibraryExA(sotes.exe,AS_DATAFILE) not settings=NULL; their registration stays coupled to the engine-time slot indices (deferred with 0x5a00c0).  NEXT: port a slice of 0x5a00c0 into game_render for the static town backdrop — needs the world populated by 0x59f2c0 setup + 0x586010 sim first (the 0x4120 map object, 0x5400c/0x7808 buffers, &DAT_006940c8 registry).  Clean /clear point.)

> **ckpt 52 — THE IN-GAME GAME_DRIVE SCAFFOLD IS STOOD UP** (plan 3b's first
> bullet; the structural seam the render slice drops into).  No engine code
> ported — this is the drive plumbing, like ckpt 49/50's phase-kickoff units.
>
> **PORT:** new **`src/game_drive.{c,h}`** (mirror of `prologue_drive`): owns the
> in-game `input_mgr` + ring, `game_drive_init/step/shutdown`, a `game_drive_cfg`
> {render,present,user}.  `enter_game` (`main.c`) now stands it up (render=
> `game_render`, present=`drive_present`) instead of `reenter_title`;
> `main_loop_body` gains a `g_game_active` branch that injects trace input + runs
> one `game_drive_step` per frame.  **`game_render` = `zdd_object_clear` (black)**
> — the faithful map-load frame (engine unported).  `game_status::GAME_EXIT` is
> reserved for the engine's scene-transition codes (0x59f2c0 ret 4/5 → 0x59ec30
> reload); a step stays `GAME_RUNNING` for now.
>
> **VERIFIED LIVE** (`tests/scenarios/in-game-intro/trace-port.jsonl`,
> `--frames 1300`): subtitle@436 / newgame@691 / prologue@826 / **game_enter@1116
> rng 0x40d00581** (matches retail game_enter@1092).  The port runs the game_drive
> to frame 1300 with NO title re-display.  Captures (C:\osscap): frame 400 (title,
> phase 6) = 307200 nonblack px (title unaffected); frames 1160/1200 (in-game,
> phase=-1) = **extrema 0, nonblack=0** (fully black) → the early in-game frames
> now match retail's black entry window (golden: flips 900-1100 black).
>
> **EXE-NULL banks (recorded, in-game-intro.md "Resource banks"):** `0x570-0x572`
> CONFIRMED present in `sotes.unpacked.exe`'s `.rsrc` as `type=DATA` (387 DATA
> ids total; tool `sotes_resources.py`).  The port ships as its own exe, so
> `FindResourceA(NULL,…)`→`settings=NULL` is WRONG for the port — it must
> `LoadLibraryExA("sotes.exe", LOAD_LIBRARY_AS_DATAFILE)` and pass that handle as
> `settings` (same pattern as `g_sotesd`).  Registration needs the pool **slot
> indices** these banks occupy, set by the engine-time site inside the unported
> 0x586010/0x5a00c0 → stays bundled with the 0x5a00c0 port.
>
> **NEXT (plan 3b body):** port a slice of **`0x5a00c0`** (the 13.7 KB render
> dispatch) into `game_render` for the static town backdrop — but it reads world
> state the **0x59f2c0 setup** + **0x586010 sim step** must populate first (the
> 0x4120 map object, the 0x5400c/0x7808 world buffers, the `&DAT_006940c8`
> 0x54-stride actor/cell registry).  So the real first unit is the world
> construction (0x59f2c0 fresh-entry arm, host-tested against the decomp), THEN a
> minimal render slice.  3 host tests (753 pass).  Ledger 175/1490.  Clean
> **/clear point**.
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 51 — PLAN 3a RESOLVED: the in-game town's resource banks are identified by a NEW live res-probe (`frida_capture.py --res-probe` → hooks `bs_decode_resource` 0x5b7800, logs every distinct (DLL,id,type) load).  Drove retail prologue→Z-spam→town under lockstep: the opening map (0x3f2) loads NO per-map resource file — it lazily decodes **74 sprite banks** via the normal `ar_sprite_decode` path, **71 from sotesd.dll + 3 EXE-embedded** (hModule=NULL).  Map layout = compiled-in static data, NOT a resource.  Cross-referenced: the banks are exactly the deferred boot batches g2(ramps/portraits)+g3+g5 that `init_sprite_banks` skipped (g4 already wired).  **WIRED g2/g3/g5 at boot** (all settings=g_sotesd); **verified title still differ_px=0** (A/B post-vs-pre, inert as predicted — title uses none of them).  The ONE residual = EXE-NULL banks 0x570-0x572 (settings=NULL, engine-time registration → plan 3b).  750 host tests pass; ledger unchanged (tooling + boot wiring).  NEXT: plan 3b — stand up `game_drive`, register the EXE-NULL banks, port a slice of `0x5a00c0` for the static town backdrop, diff vs runs/tas-ingame-1.  Clean /clear point.)

> **ckpt 51 — PLAN 3a (in-game resource banks) RESOLVED + the boot batches WIRED.**
> Built a new ground-truth probe, captured the town's exact bank set, and wired
> the missing registration — the prerequisite for the engine port (plan 3b).
>
> **NEW TOOLING — `frida_capture.py --res-probe [--res-frames LO,HI]`.**  Hooks
> the generic PE-resource decoder `bs_decode_resource` (`FUN_005b7800`) and logs
> each DISTINCT `(module, id, type)` load with its first flip (agent
> `installResProbe`, dedup by module|id|type → `res_loads.jsonl`).  This is the
> "which DLL banks does a scene pull" ground truth — every sprite-bank decode +
> any world/audio resource read funnels through `FindResourceA` here.
>
> **FINDING (RESOLVED plan 3a).**  Drove retail prologue → Z-spam → in-game town
> under `--lockstep` (`trace-retail.jsonl`), analysed loads at `flip >=
> game_enter@1092`:
> - **No per-map resource file.**  The town's ONLY loads are `type="DATA"`
>   sprite-sheet decodes via the SAME path the title uses (`ar_sprite_decode`
>   0x4184a0 + palette-load 0x4178e0).  `0x586010`/`0x5a00c0` never FindResource a
>   map/tile file → **map layout is compiled-in static data** (the
>   `&DAT_006940c8` registry, "StartArea" tables).  `sotesw.dll`=WMA music,
>   `sotesp.dll`=1 ramp — neither holds graphics (map-id 0x3f2 colliding with a
>   sotesw WMA id is a BGM red herring).
> - **74 distinct sprite banks**: 71 `sotesd.dll` + 3 EXE-embedded (hModule=NULL).
>   Cross-referenced to the register tables = the deferred boot batches **g2**
>   (`ar_register_palette_ramps`: ramps + dialogue face portraits 0x3ea/…),
>   **g3** (`ar_register_group3_sprites`: bulk town 0x423-0x481/0x769-76b/0x8b7-8bb),
>   **g4** (already wired, ~15 banks), **g5** (`ar_register_game_sprites`:
>   character sprites 0x592-5fb/0x7ef-7f9).
>
> **PORT CHANGE.**  `init_sprite_banks` (`src/main.c`) now also calls
> `ar_register_palette_ramps(2)` + `ar_register_group3_sprites(3)` +
> `ar_register_game_sprites(5)` (all `settings=g_sotesd`), matching retail's
> `ar_boot_register_all`.  Banks decode lazily, so it's inert until a
> `game_drive` renders in-game.  **VERIFIED no regression:** A/B capture
> (post-vs-pre binary, port title flips 60/200) → **differ_px=0** both frames;
> port boots clean, log shows "+ in-game batches g2/g3/g5".  750 host tests pass
> (main.c not host-linked).  Ledger unchanged (tooling + boot wiring, no new FUN).
>
> **ONE RESIDUAL → plan 3b.**  The EXE-NULL banks **0x570-0x572** (present only in
> `sotes.exe`'s own .rsrc, absent from sotesd; loaded with `hModule=NULL`) are in
> NO ported sprite batch — retail registers them with `settings=NULL`, most
> likely at engine time (the map's local tileset).  Register them with
> `settings=NULL` when standing up the render.  Full writeup:
> `docs/findings/in-game-intro.md` "Resource banks (plan 3a)".
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 50 — IN-GAME SEAM WIRED: the prologue's NORMAL exit (3rd beat → PROLOGUE_DONE) now routes to a new enter_game seam that emits the game_enter TAS anchor + logs 0x59ec30(0,0,0x3f2), and the retail agent gains the matching game_enter anchor @0x59f2c0. VERIFIED BOTH SIDES: port game_enter@1116 / retail game_enter@1092, rng 0x40d00581 MATCHES exactly (no RNG desync across prologue→in-game). The engine 0x59f2c0 is surveyed/decomposed (its two giant children 0x586010=6KB sim/draw step + 0x5a00c0=13.7KB render dispatch are the next port). Plan: docs/findings/in-game-intro.md)

> **ckpt 50 — THE IN-GAME SEAM IS WIRED + ANCHORED (both sides), engine
> decomposed.**  Foundational plumbing for milestone 2 (the game proper); no
> engine code ported yet (that's the next, multi-checkpoint rock).
>
> **PORT SEAM:** `main_loop_body`'s prologue arm now routes `PROLOGUE_DONE` (the
> 3rd-beat NORMAL exit) to a new **`enter_game()`** (`src/main.c`) instead of
> `leave_prologue_to_title`.  `enter_game` tears down the prologue drive, emits
> the **`game_enter`** anchor (`emit_anchor`), logs the `0x59ec30(0,0,0x3f2)`
> entry, and — engine unported — re-displays the title (like the other stubbed
> sub-scenes; DEMO_START etc.).  **ABORT (id 0x22) still → `leave_prologue_to_title`.**
>
> **RETAIL ANCHOR:** `SCENE_ANCHORS` (`tools/frida/opensummoners-agent.js`) gains
> `{ va: 0x59f2c0, name: 'game_enter' }` — the per-map run-loop entry `0x59ec30`
> calls (once for map 0x3f2, again per transition).  Fires on every entry; the
> flip stamp disambiguates.  A fixed offset off prologue_enter would NOT hold (the
> cutscene exit-fade + map load make the commit→first-frame offset variable), so a
> map-loop-entry anchor is required.
>
> **VERIFIED BOTH SIDES (live).**  Port (`--input-trace` new committed
> `tests/scenarios/in-game-intro/trace-port.jsonl`, `--frames 1400`):
> newgame_enter@691 → prologue_enter@826 → **game_enter@1116 rng=0x40d00581**.
> Retail (Frida `--lockstep`, `trace-retail.jsonl`): **game_enter@1092
> rng=0x40d00581**.  **RNG matches exactly** at the seam on both sides → no rand
> desync across prologue→in-game (the transition consumes nothing the port
> misses); the anchor absorbs the +24-flip offset for `tas_diff`.
>
> **ENGINE SURVEYED (`docs/findings/in-game-intro.md`).**  `0x59f2c0` (3522 B) =
> world alloc (the **map object `operator_new(0x4120)`**, map-id field **+0x4104**
> defaulting to **0x3f2**, 8× 0xeec actor sub-objects; two big world buffers
> 0x5400c/0x7808 on scene[4]/[5]; the **`&DAT_006940c8` 0x54-stride actor/cell
> registry** copied in via `0x585000`) + a **per-room loop** (`LAB_0059fd85`)
> calling the two giant children: **`0x586010` (6 KB)** = the room state-setup +
> sim/draw step (allocs `DAT_008a9b50` 0x27b8, refs `"Start StartArea"`; returns
> the loop dispatch code 1/2/3/0xa/4/5), and **`0x5a00c0` (13.7 KB)** = the
> in-game RENDER dispatch (state 3 → present).  **`0x5a00c0` reuses the
> ALREADY-PORTED sprite primitives** (`ar_sprite_decode 0x4184a0`, zdd blits,
> ramps `0x5bd550`) → the smallest visible win (the town tilemap at retail flip
> ~1150) sits on top of existing code.  **NEXT: plan 3a/3b in in-game-intro.md** —
> find map-0x3f2's resource banks + load path (likely the deferred `ar_register_*`
> batches at boot, like the title banks ckpt 26), stand up a `game_drive` (mirror
> `prologue_drive`), and port a slice of `0x5a00c0` to render the static town
> backdrop, diffing vs `runs/tas-ingame-1`.  750 host tests pass (no src logic
> change); ledger 175/1490 unchanged (seam+anchor+survey).  Clean **/clear point**
> before the engine port begins.
>
> ─────────────────────────────────────────────────────────────────────────────

> **ckpt 49 — IN-GAME PHASE KICKOFF: retail golden of the opening map captured;
> the in-game engine (`0x59f2c0`) is the next port.**  Per the ckpt-48 decision,
> the user said: *"yes extend the trace.  start by just spamming Z after the
> prologue begins, give the trace ~1 min of frames, then continue matching
> retail."*  Done the FIRST half (establish the target); the in-game ENGINE is
> unported (the next big rock).
>
> **HOW YOU GET IN-GAME:** the prologue cutscene `0x56cd20` exits on the **3rd
> beat** (any fresh press); **Z = confirm id `0x24`** = a beat = the NORMAL exit
> (`0x22` would *abort* to title).  On `PROLOGUE_DONE` retail calls
> **`0x59ec30(0,0,0x3f2)`** → loads + runs the opening map.  Trace saved:
> `tests/scenarios/in-game-intro/` (retail flips: Start@615 … prologue_enter@815,
> Z beats @850/870/890, then Z every 20 flips).
>
> **RETAIL GOLDEN (runs/tas-ingame-1 — LOCAL ONLY, runs/ gitignored):** captured
> under `--lockstep`.  Flips **900–1100 are black** (cutscene exit-fade + map
> load); **the game renders from ~flip 1150** = the **opening TOWN of Tilelia**
> (houses/NPCs/banner), and spamming Z advances into the **story DIALOGUE**
> (portrait + textbox, ~flip 2200–2500).  6-frame montage pushed to llm-feed.
>
> **THE NEXT ROCK — `0x59f2c0` (3522 B), the map run loop.**  `0x59ec30` (531 B)
> is just the scene LOAD/UNLOAD wrapper; `0x59f2c0(map,…)` is the in-game engine
> (zeroes a ~0xeb1c-B stack frame of tile/entity arrays + a 0x4120 map object,
> loads map 0x3f2, runs the per-frame update + render).  **Plan (full writeup:
> `docs/findings/in-game-intro.md`):** (1) add a game-entry ANCHOR (retail
> `0x59ec30`/`0x59f2c0` + port `enter_game`) so tas_diff aligns the in-game
> frames; (2) wire the port seam (`PROLOGUE_DONE` → `enter_game` stub, instead of
> re-display title); (3) port `0x59f2c0` in units vs the golden (town tilemap →
> entities → dialogue box), smallest visible win first.  **No port code this
> ckpt** — phase kickoff (golden + plan).  751 host tests still pass; ledger
> 175/1490.  This is a clean **/clear point** before the in-game port begins.
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 48 — the NEW-GAME→PROLOGUE FADE-OUT is ported: the cutscene-offset gap (open thread #1) is CLOSED — the port now spends retail's ~20 fade flips before 0x56cd20, so prologue_enter moved 801→821 (+20) and the cutscene FADE-IN diffs differ_px=0 at a constant offset, 63/64 dense gem frames bit-exact; the fade-out frames' RENDER (box-alpha arm) is the remaining sub-item)

> **ckpt 48 — THE NEW-GAME→PROLOGUE FADE-OUT IS PORTED (cutscene-offset gap
> CLOSED).**  Closed TAS open thread #1: the port skipped retail's post-config
> fade-out, entering the gem cutscene **1 flip** after the "Start Game" commit
> while retail spends **~20 flips**, so the cutscene fade-in couldn't align at a
> single anchor offset (ckpt-47 residuals 246–662 px = one fade step).
>
> **KEY RE FINDING (quirk #72):** `FUN_00564160`, on a Start commit, clears the
> menu box node's `+0x50` (564160.c:30) and runs a **≤20-frame fade-out loop**
> (`0x5642e0` scene update + `FUN_0056c930` **mode-1 CLOSING** alpha ramp +
> `0x43c2e0` per-entry) before `0x56cd20` — **exactly 20 presented flips** under
> `--lockstep` (confirm@795 → prologue_enter@815).  `0x5642e0` early-breaks
> (returns 6) only on an abort (id 0x22); the Start path runs all 20.
>
> **PORT:** a fade-out submode in `newgame_drive` (`NEWGAME_FADEOUT_FRAMES=20`).
> A Start-Game commit no longer returns `NEWGAME_START` immediately — it sets
> `fading`, clears `node.field_50`, seeds `node.field_54` from the open gate,
> and for 20 frames ramps `field_54` down by `0x28`/frame (the `0x56c930`
> mode-1 close, already ported as `menu_owner_transition_step`), re-renders +
> presents, then returns `NEWGAME_START` so `main.c` enters the prologue at
> retail's offset.
>
> **VERIFIED (tas_diff, `prologue_enter`, port@821 vs retail@815,
> runs/tas-retail-gem, window 2):** prologue_enter moved **801→821** (+20,
> matching retail); `prologue_enter` rng still **0x40d00581** (the fade consumes
> no `rand()`).  The cutscene **fade-in (ticks 1-28) is now entirely
> `differ_px=0`** at a near-constant drift (−2, ±1 lockstep wobble) — the
> ckpt-47 fade-in residuals are GONE.  **63/64** dense gem-rise frames bit-exact;
> the only non-zero frame is tick 0 (84684 px) = the **pre-existing** entry-frame
> issue (port renders the first gem tick while retail's first present is still
> black — documented ckpt 47, NOT introduced here).  Comparison pushed to
> llm-feed.
>
> **STILL OPEN — the fade-out frames' RENDER (deferred box-alpha arm).**  The
> port re-renders the new-game menu **opaque** during the 20 fade frames; retail
> fades the box-panel alpha (`0x48cf80`'s alpha arm via `0x5bd550`) + the GDI
> menu text, so those ~20 transition frames (port ~801-820 vs retail ~795-814)
> are NOT yet bit-exact.  That fade *render* is a separate item — needs a retail
> capture of the fade-out frames to model how the GDI text fades.  This ckpt
> closed the **timing/offset** gap (the cutscene now diffs at a constant offset).
> 751 host tests pass (+1).  Ledger **175/1490** (+1: `0x564160` partial).  Full
> writeup: **`docs/findings/tas-harness.md`** (open thread #1 → CLOSED).
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 47 — the TAS DETERMINISTIC TRACE-DIFF SYSTEM is built + validated: the `--lockstep` retail clock makes retail render 1 update/present like the port, bilateral RNG-stamped ANCHORS align the flip axes, and `tools/tas_diff.py` diffs frame-for-frame; INTRO 28/28 BIT-EXACT and PROLOGUE CUTSCENE CONTENT BIT-EXACT (63/64 dense) through the pipeline; two real port gaps surfaced — see ckpt 47)

> **ckpt 47 — THE TAS SYSTEM WORKS: deterministic port↔retail frame-for-frame
> diff.**  Built the determinism stack the user asked for.  **KEY RE FINDING:**
> retail drives its whole update cadence off **`GetTickCount` deltas** (no
> timeGetTime/QPC) — every scene loop (title, the new-game modal pump `0x565d10`,
> the prologue cutscene `0x56cd20`) runs the same 3-state pace machine spending a
> budget in 16 ms update slices.  The old turbo clock bumped the virtual clock
> per-CALL → several updates banked per Flip → retail rendered only ~1/N of the
> update stream (subtitle anchor @ flip **73**), un-diffable vs the port's
> 1-update/present.  New **`--lockstep`** (agent) freezes the virtual clock
> between Flips and banks exactly **one update quantum per present** (stall-breaker
> creep defeats load busy-waits without polluting the budget) → retail renders
> 1 update/present like the port (subtitle anchor @ **432**, two runs
> byte-identical, all consecutive frames distinct = exactly 1:1).
>
> **ALSO PROVEN:** retail is already **fully deterministic run-to-run** under
> turbo+seed-pin (two runs, different wall-clock seeds, byte-identical frames
> through the dynamic intro AND the auto-demo).  So lockstep is for *cadence*
> (every update rendered), not determinism.
>
> **ANCHORS (both sides, RNG-stamped):** `subtitle_anim_start` / `newgame_enter`
> / `prologue_enter` — port logs `anchor: <name> flip=<N> rng=0x<hex>`
> (`emit_anchor` in main.c), retail sends `{kind:anchor,…,rng}` (agent
> `installSceneAnchors` hooks `0x564780`/`0x56cd20`; sparkle hook covers the
> first).  Within a scene both march tick-for-tick; the anchor absorbs the
> per-binary flip skew, and the RNG stamp flags an unaccounted `rand()` consumer.
>
> **`tools/tas_diff.py`** aligns a port capture set to a retail run on an anchor
> and reports per-tick `differ_px`, best-matching each port frame within ±W (so
> an occasional **port 0-update duplicate present** that slips the offset by ±1
> is absorbed without hiding a real divergence).
>
> **RESULTS:** INTRO (`subtitle_anim_start`, port@438/retail@432): **28/28
> phase-7 frames `differ_px=0`** through the harness (one port dup at tick 22 →
> drift −1).  PROLOGUE cutscene (`prologue_enter`, port@801/retail@815): dense
> gem-rise cross-align → **63/64 port frames have a `differ_px=0` retail match**
> (only the scene-entry frame lacks one) → the **gem/aura/narration render is
> frame-for-frame BIT-EXACT** (ckpt-46 eyeball verification now quantified).
> Comparison pushed to llm-feed.
>
> **TWO REAL PORT GAPS surfaced by the anchors (open RE threads, NOT render
> bugs):**  (1) **the port SKIPS the new-game→prologue transition** — port:
> confirm "Start Game" → `prologue_enter` in **1 flip**; retail: **~20 flips**
> (the `0x564160`/`0x5642e0` fade-out/load before `0x56cd20`).  This shifts
> retail's cutscene timeline ~14 flips later, so a single constant anchor offset
> doesn't hold across the fade-in (±1-tick fade-cadence, residuals 246–662 px =
> one fade step).  (2) **RNG desync at `prologue_enter`** — both sides match at
> newgame_enter (`0x404a0a8f`) but differ at prologue_enter (port `0x404a0a8f`
> vs retail `0x40d00581`): the retail transition consumes `rand()` the port
> doesn't.  The gem is RNG-independent so unaffected, but it's the canonical
> "unaccounted rand consumer between two anchors".
>
> **GAP #2 NOW CLOSED (user chose "port the transition").**  New **`--rand-probe`**
> (tally rand() callers between two anchors) pinned it to **exactly 2 rand()
> calls** (`0x5b6acc`/`0x5b6ae9`) inside **`FUN_005b6990`** — the **initial
> save-file write** (resource `0x2711`) retail runs on "Start Game", salting its
> buffer with 2 rand draws.  Save deferred (milestone 4), but its RNG side
> effect is on the TAS path, so **`newgame_start_save_salt()`** (main.c) consumes
> the 2 draws on the START→prologue handoff → port prologue_enter rng now
> **`0x40d00581`**, matching retail.  **GAP #1 (the 20-flip fade-out) STILL OPEN:**
> retail's `FUN_00564160` post-config loop runs ≤20 frames of the new-game scene
> **fading to black** (box panels ramp alpha down — `FUN_005642e0` +
> `FUN_0056c930` modes 0/2 + `FUN_0043c2e0`) before `0x56cd20`; the port enters
> the cutscene 1 flip after commit, so the cutscene offset drifts during the
> fade-in.  **NEXT: port that fade-out so the timelines align at a constant
> offset.**  Full writeup: **`docs/findings/tas-harness.md`**.  749 host tests
> pass (tooling + main.c).  Ledger unchanged (harness + 1 RNG side-effect line).
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 46 — the ELEMENTAL-STONE PROLOGUE CUTSCENE (FUN_0056cd20) is ported + wired into the Start path + RENDERS LIVE (gem + aura + scrolling narration), USER-CONFIRMED visually; next is the bit-exact retail diff, then the game proper 0x59ec30 — in-game, deferred)

> **ckpt 46 — THE ELEMENTAL-STONE PROLOGUE CUTSCENE IS PORTED, WIRED, AND
> RENDERS LIVE.**  Confirming "Start Game" in the new-game menu now runs the gem
> cutscene (`FUN_0056cd20`, the prologue critical path): the glowing purple
> Elemental Stone rises on black with a soft aura, while the **story NARRATION
> scrolls** ("Elemental Stones: stones imbued with the power of an Elemental
> Spirit, which grant the wielder of one the ability to control that element via
> 'magic'.…").  New pure, host-tested **`src/prologue_stone.{c,h}`** (the visual
> half of `0x56cd20`): the per-tick UPDATE state machine (start delay, watchdog,
> gem fade-in/hold/fade-out, gem-frame %0x23 + aura toggle cadence, the rise
> curve, the 6 caption-line state machines, abort/beat input) + the render-
> descriptor build.  New **`src/prologue_drive.{c,h}`** is the Win32-free caller
> (steps one tick/frame, renders + presents; no input-gate ramp — the cutscene
> reads the raw ring).  `main.c`: `prologue_render` clears to black + blits gem
> (slot[3]/0x4a2 via ramp_b) → aura (slot[1]/0x49f via ramp_a) → 24 caption tiles
> (slot[2]/0x448 via ramp_b); the new-game START commit now calls `enter_prologue`
> instead of re-displaying the title; on DONE the game proper is unported (re-
> display title, logged), on ABORT (0x22) → title.  19 host tests (**749 pass /
> 0 fail / 6 skip**).  Ledger **174/1490 (+1: `0x56cd20`)**.
>
> **KEY LIVE FINDING:** the **prologue NARRATION is part of `0x56cd20`**, NOT the
> game proper — it is **pre-baked sprite tiles** (bank `0x448` = slot[2], a 24-
> tile strip = 6 lines × 4 horizontal tiles), the grid the survey first mislabeled
> "sparkles".  `0x56cd20` uses **no GDI text** at all.  The banks were **already
> registered at boot** (`ar_register_main_sprites` group 4), and the alpha blit
> (`zdd_alpha_blit`) + ramps (`g_ramp_a`/`g_ramp_b`) were already ported — so the
> cutscene needed only the state model + drive + wiring.  The aura's blend ramp
> (ramp_a, idx `local_bc/30`) was recovered from the **disasm** (`0x56d38d`); the
> decompiler had dropped `FUN_005bd550`'s `__thiscall` ECX = the ramp entry.
>
> **USER-CONFIRMED visually** ("that cutscene looks good… on first inspection it
> looks right").  Montage pushed to llm-feed (frames 950–3300 of one run).
>
> **OPEN gate — NO bit-exact retail diff yet.**  No retail golden of the stone
> intro has been captured.  NEXT: drive **retail** to the cutscene (the committed
> trace + Start-Game confirm), capture goldens, and `differ_px`-diff the gem +
> narration vs the port — the bit-exact bar.  (Caveat to assess: like the picker,
> `0x56cd20`'s modal loop may freeze the hooked Flip counter — if so, capture
> needs a non-flip-keyed harness.)  Until then the cutscene is eyeball-verified.
> See Next move #1.
>
> ─────────────────────────────────────────────────────────────────────────────

# Session handoff — earlier (ckpt 45 — the new-game OPTION PICKER submenu is ported + wired + rendered + USER-CONFIRMED (FUN_00567ba0, quirk #71))

> **ckpt 45 — THE NEW-GAME OPTION PICKER SUBMENU IS PORTED, WIRED, RENDERED,
> AND USER-CONFIRMED.**  Confirming on a kind-0 option row (Game Difficulty /
> Auto-guard) now opens that option's **picker submenu** — the nested value-grid
> `FUN_00567ba0` runs.  New pure, host-tested **`src/newgame_picker.{c,h}`**:
> `newgame_picker_values` (`FUN_00568320` — id 3 `{10,20,30,40}`/`{..,50}` unlock-
> gated, id 4 `{0,1}`) + the run-loop model (build the 1-col value grid, **seek
> the cursor to the current value** via `FUN_00419900`, nav/commit/cancel via the
> `NEWGAME_PUMP_*` codes → RUNNING/COMMIT(value)/CANCEL).  Wired into
> **`newgame_drive`** as a frame-stepped modal SUBMODE (the equivalent of retail's
> blocking `FUN_00567ba0` call): a kind-0 CONFIRM opens the picker and pumps input
> into `picker.grid` instead of the parent; on COMMIT the drive calls
> `newgame_scene_set_option(id, chosen)` (the parent value-refill — re-lays the
> value cell); on CANCEL the option is unchanged.  `main.c` draws the picker box
> (9-slice, **(288,128) w=256**) + its value rows over the menu when active.
> **Verified LIVE** (port trace Start→confirm Difficulty→down→confirm): the
> difficulty picker opens `{1:Easy,2:Normal,3:Hard,4:Expert}` with the current
> value focused; nav moves the highlight; commit re-lays the parent cell **1:Easy
> → 2:Normal**.  Montage pushed to llm-feed; **USER-CONFIRMED** ("screenshots look
> good, those menus render correctly").  Quirk **#71**.  10 new host tests
> (**736 total: 730 pass / 0 fail / 6 skip**).  Ledger **173/1490 (10.7%)** (+5:
> `0x568320`,`0x567ba0`,`0x419900`,`0x5657f0`).
>
> **OPEN gate (NOT a content bug — a harness limitation): no bit-exact retail
> diff of the open picker.**  Entering the new-game scene, retail's **Flip counter
> freezes** (the modal pump `0x565d10` doesn't advance the hooked DDraw Present —
> quirk #67 caveat), and **both** the harness's frame capture **and** its input
> injection are flip-keyed.  So the open picker can be neither driven-to nor
> captured by flip index → its render geometry (288,128/256) and the decompiler-
> reconstructed args (the `FUN_00412160` row kind, `FUN_00419900`/`FUN_005657f0`
> arg lists, all documented in `newgame_picker.h`) are **NOT pixel-verified vs
> retail**.  Closing it needs a harness that hooks `0x565d10`'s own present +
> feeds its input directly (not flip-keyed) — a tooling task, deferred.  Until
> then the picker is user-eyeball-verified, which is the verification available.
>
> **NEXT: the Start→game path (the prologue critical path).**  The committed
> reference trace navigates to "Start Game" and confirms — it never opens the
> picker — so the Start→game transition (`0x564160`→`0x5642e0`/`0x56cd20` timed
> Elemental-Stone cutscene → `0x59ec30` game proper) is the item actually on the
> path to "first frame in-game".  START is a stub today (re-displays title).
> See Next move #1d.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 44 — THE NEW-GAME TOOLTIP TEXT NODE RENDERS BIT-EXACT.**
> The bottom-of-screen help line is a standalone **word-wrapping text node**
> (`this+0x170`), NOT the menu grid — one free-form string greedily wrapped into
> rows.  Ported the layout core **`FUN_0040e5e0`** (justify) + the **`%n`/`%m`/`%w`
> parse `FUN_0040f040`** as the pure, host-tested **`src/glyph_wrap.{c,h}`**: a
> word = alpha `[A-Za-z']+` / digit `[0-9.,]+` (+ one absorbed trailing
> `{space ! , - . ; ?}`) / lone glyph; the row-width accumulator + wrap test
> mirror retail's `uVar13`/`param_1` (width = the `FUN_0040dee0` ctor arg
> `0x44` = **68 glyph-columns**); `%n` forces a break.  SJIS kinsoku (`sVar3==3`,
> the `DAT_008548xx` table) is deferred (English never reaches it).  Quirk **#70**.
> `newgame_render` (`main.c`) now picks the focused row's help string
> (`newgame_scene_tooltip`), wraps it, and draws each row at **(72, 416+r·28)**
> with the menu's 2-copy shadow (`0xa8b9cc`) + text (`0x3e537d`).
> **Verified LIVE** (port Flip 760 vs `goldens/retail-newgame-config-menu.png`):
> the difficulty-row tooltip wraps **65 / 52** glyphs across y=416/444 — the break
> is the width-68 word-wrap (the source string has **no** `%n`), reproduced
> exactly.  Tooltip region: **0 text-colored pixels differ** (every glyph +
> shadow matches; proven by a text-presence XOR == 0).  6 new host tests
> (`tests/test_glyph_wrap.c`, **720 pass / 0 fail / 6 skip**).  Ledger **168/1490
> (+5: `0x40e5e0`,`0x40f040`,`0x4031c0` touched)**.  Comparison pushed to llm-feed.
>
> **OPEN (small, pre-existing, NOT the tooltip text): a 9px box-panel RGB565
> 1-LSB rounding.**  The only residual in the tooltip region is **9 background
> (cream/gold) pixels off by exactly 8 in a 5-bit (R or B) channel, green always
> exact** — i.e. one RGB565 quantization step on the 9-slice box-panel sprite
> (`newgame_box`, ckpt 40, bank `0x457`), NOT the text.  Same class as the
> delta-8 px in the menu box (there masked by the deferred top-left sparkle).
> **Hypothesis:** an 8→5-bit decode rounding (round vs truncate) in the box-frame
> sprite decode (`bs_convert_to_16bpp`) lands a few edge/gradient pixels on the
> opposite side of a 5-bit boundary.  A separate sprite-decode investigation —
> compare the port's decoded `0x457` frame RGB565 vs retail at those px; deferred
> (the box was user-accepted at ckpt 40).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 43 — THE NEW-GAME SELECTION CURSOR RENDERS BIT-EXACT (`differ_px=0`).**
> The ckpt-42 "scale_flag=1 videomem cell-build path" diagnosis was **WRONG**.
> The real bug: `bs_trim_opaque_rect` (`FUN_005b6f80`) named its two size params
> `(height, width)`, but retail's arg4 = `cell_w` (the inner/column loop + x-range)
> and arg5 = `cell_h` (the outer/row loop + y-range); the slicer passes
> `(cell_w, cell_h)`, so a **non-square** cell was scanned **transposed**.
> Invisible on the square 32×32 box bank `0x457` (every square sprite stayed
> bit-exact), it scrambled the 32×48 cursor bank `0x455` into a wrong-size,
> wrong-offset, un-keyed cell (the "opaque-black 16×24 @ x72" symptom).  Fix: swap
> the param names to `(width, height)` in `src/bitmap_session.{c,h}` (body
> unchanged); the port's `0x455` frame-17 trim is now **22×41 @ (4,3)**, matching
> the live `--box-probe` (proven offline by **`tools/extract/cursor_trim_probe.c`**).
> Quirk **#69**.  `g_newgame_cursor_enable` flipped **ON**; verified LIVE — port
> Flip 761 vs `goldens/retail-newgame-config-menu.png` → menu-box **`differ_px=0`**
> (panel + text + cursor).  Off-phase frames 760/762 differ only by the cursor's
> animation phase (frames 17≡19 = 22×41@(4,3) is the phase the golden froze; 16/18
> are the off-phase) — the same caveat as the intro twinkles, not a content gap.
> Closes the ckpt-40 307px residual.  Parity-ledger **#5**.  **714 pass / 0 fail /
> 6 skip** (+1 regression: `test_trim_8bpp_nonsquare_quirk69`).  Ledger 163/1490
> (bug-fix, no new FUN).  **NEXT: the tooltip TEXT node (word-wrap) → the option
> picker submenu (`0x567ba0`) → the Start→game path (`0x564160`→`0x59ec30`).**
> See Next move #1b.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 42 — THE SELECTION-CURSOR SPRITE BANK IS SOLVED: it is bank `0x455`
> (sotesd.dll, slot 43 = `AR_SPR_FONT_TEX_455`), frames 16–19 — the SAME
> bank/slot/frames the ckpt-41 geometry port already targeted.**  The ckpt-41
> blocker ("0x455 sweep matches nothing") was a decode-**ORIENTATION** error: the
> Lizsoft DATA blob is a BMP-style **bottom-up** bitmap, and the engine slices
> cells bottom-up.  Read top-down, frames 16–19 land on the row-4 ► chevrons
> (9×17 — not a vine); read **bottom-up**, frames 16–19 are the drooping gold
> **feather/quill + soft white shadow**, and their trimmed bboxes match the live
> `--box-probe` **EXACTLY** (frame 17 = 22×41 @ (4,3), 18 = 22×40 @ (4,4),
> 19 = 22×41 @ (4,3)).  Proof tool: **`tools/extract/cursor_frame_match.py`**.
> The probe's `res_id=0x3e8` (slot+0x40) is a **reused/garbage marker** (PE 0x3e8
> = an 80×352 portrait in sotesd / a WMV in sotesw / absent in sotesp) — the
> reliable per-frame signal is the trim size via `entries[frameSel]→frec+0x14/
> +0x18`.  Quirk **#68**; #67 corrected.  No `src/` logic change (decode-
> orientation + bank-ID finding), so **713 pass / 0 fail / 6 skip** unchanged;
> ledger unchanged.
>
> **STILL OPEN (the NEXT render task) — a scale_flag=1 cell-build bug, NOT a bank
> problem.**  I flipped `g_newgame_cursor_enable` ON and captured live (port frame
> 760): the cursor blits as an **opaque-black 16×24 rect at x72–87** (golden
> feather is at x44–66) — the gold corner ornament is the box's own 9-slice
> corner.  `0x455` is the **only** registered bank with **`scale_flag=1`** (box
> `0x457` is 0), so its cell takes the **untested videomem cell-build path**
> (`zdd_object_build_cell` `videomem` arg → caps `0x804`).  Symptoms: the slicer
> computes the **wrong trim offset** (base (40,26)+fdx≈32 → x72 vs correct fdx=4 →
> x44) AND the transparent area **fails to colour-key**.  differ_px went 307→493
> (a regression), so the gate is reverted to **OFF**.  **NEXT: dump the port's
> decoded slot-43 frame-17 trim rect + surface, compare to the probe's 22×41 @
> (4,3), and fix the scale_flag=1 videomem trim/keying — then flip the gate ON
> and diff vs the golden.**  See Next move #1a''.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 41 — THE SELECTION-CURSOR (gold vine) GEOMETRY IS PORTED + VALIDATED,
> but its sprite BANK is unidentified — render is GATED OFF pending a reliable
> retail probe.**  New pure, host-tested **`src/newgame_cursor.{c,h}`** ports
> `FUN_0048d940`'s type-1 arm (the menu selection cursor): frame = base(16) +
> frames{0,1,2,3} → 16-19 (from the RELIABLE `FUN_00411ec0` decomp args, not the
> probe); blit base x = box_x + (node+0x7c=-32 → +8), y = box_y + (node+0x80=-30
> → -6) + (cursor-sel2)*pitch(28).  **Position VALIDATED**: row-0 base (40,26)
> matches the live `--box-probe` golden AND independently derives from the text
> col0/row0 origins (72-32=40, 56-30=26).  4 new host tests (**713 pass / 0 fail
> / 6 skip**).  `main.c` has the adapter + render wired behind `g_newgame_cursor_enable`
> (OFF).
>
> **THE BLOCKER (open RE thread): the cursor's sprite BANK is unidentified.**  The
> `--box-probe` deref chain (`bank=*(node+0x28); slot=*bank`) reads **garbage** at
> `slot+0x20`(width=a pointer) / `slot+0x38`(f_38=0xf800) for THIS node type, so
> its **`res_id=0x3e8` and 22×41 frame readouts are NOT trustworthy** (only the
> POSITION, computed from node fields, is).  Ruled out: port's **0x3e8** (slot 65,
> sotesd) decodes to a **640×352 background landscape** (8 frames, not a vine);
> **0x3e8 is absent** in sotesp/sotesw (decode fails); a full **24-frame sweep of
> the sibling box atlas 0x455** (slot 43, the `+0xb8c`-vs-`+0xb88` guess) at (40,26)
> **matches nothing** (frames are 44×30 feathers/◄►arrows/caduceus/books, but the
> golden's element is a thin **22×41 drooping stem+bud+soft-shadow**).  The real
> bank = `*(god+0xb8c)` — whose WRITE site I could not find in the decomp (only
> reads).  **NEXT: find the real cursor bank** — either locate the `god+0xb8c`
> store (the box-widget god-object's sprite-bank init, near the `+0xb88`=0x457
> box-bank store), or write a Frida probe that dumps the actual blitted frame
> surface PIXELS from retail's `0x48d940` (the only fully reliable ground truth).
> The 307px menu-box top-left residual is this still-unported vine.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 40 — THE NEW-GAME CONFIG BOX PANEL RENDERS (9-slice chrome; menu box
> bit-exact except the deferred sparkle corner).**  The bordered cream panel
> behind the menu + tooltip is now drawn.  First a new live **`frida_capture.py
> --box-probe`** (hooks the sprite-cell render `0x48d940` + the 9-slice renderers
> `0x48cb90`/`0x48cf80`, gated to the menu flip window) captured the exact box
> composition (golden `goldens/retail-newgame-box-cells.jsonl`, **quirk #67**):
> the panel is a **9-slice sprite box** (`0x48cf80`), bank **PE resource 0x457**
> (already registered by `ar_register_fonts` as `AR_SPR_FONT_TEX_457`, 32×32),
> frames **tl0/top1/tr2/l3/c4/r5/bl6/b7/br8** (center 4 = cream RGB(239,227,214));
> two instances — **menu box (32,32)400×124** + **tooltip box (32,392)576×80** —
> matching the golden's measured bounds.  A **separate** animated sparkle corner
> (`0x48d940`, bank **0x3e8**, frames 16–19) sits at the top-left.
>
> Ported `0x48cf80`'s opaque arm as the pure, host-tested **`src/newgame_box.{c,h}`**
> (the 9-slice tiling walk over a `newgame_box_ops` vtable: corner→tiled edge→
> remainder→corner per row, top/full-middle/partial-middle/bottom rows); the real
> blit (`ar_sprite_slot_frame` + `zdd_object_blt_clipped` = the keyed clipped blit
> `FUN_005b9bf0`) is wired in **`main.c`**.  `newgame_render` now: clear primary →
> draw both boxes via DDraw → GetDC + `glyph_grid_render` the menu text on top
> (replacing the placeholder `PatBlt(BLACKNESS)`).  **Verified LIVE** (port frame
> 760): menu box **differ_px=307/50800 (0.6%)** vs the golden — and **all 307 are
> in the top-left corner (x44–65,y29–69)**, exactly the deferred sparkle overlay;
> the 9-slice panel + menu text are **bit-exact everywhere else** (interior cream
> RGB(239,227,214) matches exactly).  Comparison pushed to llm-feed.  4 new host
> tests (coverage-grid: slices tile each box exactly once, no gap/overlap/OOB) →
> **709 pass / 0 fail / 6 skip**.  Ledger **161/1490 (+4: `0x48cf80`, `0x48d670`,
> `0x48d3d0` ported; the keyed blit was already in zdd.c)**.
>
> **NEXT: the remaining chrome + the Start→game path.**  (a) the **animated
> sparkle corner** (`0x48d940`, bank 0x3e8 — needs that bank registered + the
> single-cell animated render; the 307px residual); (b) the **tooltip TEXT node**
> (y=416/444, word-wrapped — `newgame_scene_tooltip` computes the text, the box is
> already drawn, need the word-wrap split into rows); (c) the **option picker
> submenu** (`0x567ba0`); (d) the **Start→game path** (Elemental-Stone intro
> `0x564160`→`0x5642e0`/`0x56cd20`→`0x59ec30`).  Also deferrable polish: the box
> **fade-in** (`0x48cf80`'s alpha arm via `0x5bd550`).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 39 — THE NEW-GAME CONFIG SCENE RUNS LIVE (visible + interactive).**
> Wired the new-game config scene as a runnable **drive** — the last piece after
> the builder (ckpt 37), renderer (bit-exact, ckpt 36), and run-loop model
> (ckpt 38).  New **`src/newgame_drive.{c,h}`** is the Win32-free caller (the
> `FUN_00565d10` / case-0x24 frame-pump side), factored like `title_drive` vs
> `title_scene`: owns the scene + input manager, ramps the input gate
> **+50/frame to 1000** (the title's `menu_owner_transition_step` mode-1 ramp),
> polls the buttons and collapses them via `menu_list_latch` into the pump result
> the scene dispatches on, then renders + presents through cfg callbacks.  The
> input pump realises `0x565d10`'s scan + `0x43bca0`'s id→latch map (quirk #65):
> 1=up/3=down/2,4=page→MOVE(`0xd`); `0x24`=confirm→latch(9)→3→CONFIRM(`0xc`);
> `0x27`=back→latch(10)→4→BACK(`0xb`).  `main.c`: `app_flow`'s NEW_GAME arm now
> routes to **`enter_newgame`** (was the re-enter-title stub); `newgame_render`
> GDI-renders the menu grid onto the primary (`glyph_grid_render` base (32,32),
> **Courier New 7×18 = font slot 5**); a `g_newgame_active` branch runs one
> `newgame_drive_step` per frame.  **Verified LIVE**: confirm Start @flip 620 →
> `result=26` → enter_newgame → the menu renders "Game Difficulty 1:Easy /
> Auto-guard On / Start Game" (row 0 focused); DOWN moves focus 0→1 (pixel-
> sampled colours bit-exact, periwinkle `0xf08080` + tan `0xa8b9cc` shadow);
> `0x27` backs out → title replays.  Quirk **#66**.  7 new host tests
> (**705 pass / 0 fail / 6 skip**).  Ledger **157/1490 (+2: `0x565d10`,
> `0x43bca0`, both partial)**.
>
> **NEXT: the deferred chrome + the Start→game path** (Next move #1).  The scene
> is live but bare: no **box widget tree** (`0x411940`→`0x40f3e0` bordered box +
> gold corners; plain black fill now), no **tooltip text node** (y=416/444,
> word-wrapped — `newgame_scene_tooltip` computes the text, render needs the
> box/word-wrap builder), no **option picker submenu** (`0x567ba0`; a kind-0
> confirm yields NEWGAME_OPEN_PICKER, surfaced but inert).  START is a stub
> (re-displays title); the real path is the **Elemental-Stone intro**
> (`0x564160`→`0x5642e0`/`0x56cd20`→`0x59ec30`).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 38 — THE NEW-GAME CONFIG RUN-LOOP MODEL IS PORTED (the Win32-free
> heart of `FUN_00564780` case 0x24); the `0x27` input semantics are
> RE-corrected.** Ported the run-loop heart of the new-game config scene into
> new **`src/newgame_scene.{c,h}`**, mirroring the `title_scene` (pure) vs
> `title_drive` (Win32) split.  The pure state machine host-tests; the real
> per-frame pump stays in the drive (next unit).  Three pieces:
> **`newgame_scene_tooltip`** (the per-frame focused-row tooltip:
> `newgame_option_tooltip` = **`FUN_00566850`** for option rows id 3/4 + the
> kind-3 action-tooltip switch — Save/Default/Cancel/Start-Game help, strings
> verified from the binary); **`newgame_scene_dispatch`** (the pump-result
> switch, 564780.c:597-669: `0xd`→re-render, `0xc`→confirm-on-row,
> `0xb`→back); **`newgame_scene_set_option`** (the picker's value-refill,
> `FUN_00566a80`→`glyph_cell_layout`).  Quirk **#65**.  4 new host tests
> (**698 pass / 0 fail / 6 skip**).  Ledger **155/1490 (+1: `0x566850`)**.
>
> **KEY RE FINDING (quirk #65) — the `0x27` input semantics:** new-game-flow.md's
> earlier "id 0x27 = value left/right" was a **wrong** guess (it self-flagged as
> unverified).  Tracing `FUN_00564780`'s loop: `0x565d10`→`0x43bca0` maps button
> `0x24`→`menu_list_latch(9)` and `0x27`→`menu_list_latch(10)`, which net out to
> **`0x24` = confirm (`0xc`)** and **`0x27` = back/cancel (`0xb`)**.  There is
> **NO in-place value toggle** — an option's value changes only by confirming
> (`0x24`) into its **picker submenu** (`FUN_00567ba0` default arm for id 3/4).
> Only the *physical-key identity* of `0x24`/`0x27` is left for a live Frida
> `--input-trace` confirm.
>
> **NEXT: wire the new-game config scene as a runnable DRIVE** (Next move #1).
> The builder (ckpt 37), the renderer (bit-exact, ckpt 36), and now the
> run-loop model (ckpt 38) are done.  What's missing is the Win32 drive that
> makes it VISIBLE + interactive: a `newgame_drive` mirroring `title_drive` that
> (a) renders each frame — box widget bg + `glyph_grid_render` of the menu node
> + the tooltip node — onto the primary surface and presents; (b) runs the
> input pump (`0x565d10` + `0x43bca0` scan, or a port-bespoke equivalent feeding
> `newgame_scene_dispatch`); (c) ramps the input gate (`sub.ready` 0→1000); and
> (d) routes `app_flow`'s `NEW_GAME` arm to it instead of the re-enter-title
> stub.  Deferred sub-rocks: the **box widget tree** (`0x411940`'s `0x40f3e0`
> box + tooltip box `0x4124d0`/`0x40dee0`) and the **option picker submenu**
> (`0x567ba0`) and the **Elemental-Stone intro** (`0x564160`→`0x5642e0`→`0x59ec30`).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 37 — THE NEW-GAME CONFIG MENU BUILDER IS PORTED; the text pipeline is
> now closed END-TO-END (build → render → bit-exact `TextOutA` stream).**
> Ported the construction half of the new-game ("Start") config scene —
> `FUN_00564780` **case 0x24** + the grid setup `FUN_00411940` performs — into
> new **`src/newgame_menu.{c,h}`**.  Run through the (already bit-exact, quirk
> #63) renderer `glyph_grid_render` at the box base **(x=32, y=32)**, the built
> grid emits retail's captured `TextOutA` stream **draw-for-draw**: all **129**
> menu-region glyph draws match
> `goldens/retail-newgame-config-textout.jsonl` exactly
> (`tests/test_newgame_menu.c`).  **Geometry fully reconciled** (the ckpt-36
> open TODO): col 0 origin **x=72** (entry[0].pos 0 + base 32 + `field_c` 40),
> col 1 **x=232** (case-0x24's `entry[1].pos=0xa0` override), row pitch **28**
> (`node+0x1ac`), rows y=56/84/112; focus row 0 in 0xf08080, others 0x3e537d,
> shadow 0xa8b9cc.  Ported functions: `menu_grid_append` (`FUN_00412160`, a thin
> append whose per-column refresh == `FUN_00411f40` → delegated to
> `menu_row_finalize`, no-op on fresh rows, quirk #36); the option string
> providers `newgame_option_label`/`newgame_option_value` (`FUN_00566570`/
> `FUN_00566a80` arms id 3/4); `newgame_config_build` (the case-0x24 sequence).
> Quirk **#64**.  3 new host tests (**694 pass / 0 fail / 6 skip**).  Ledger
> **154/1490 touched (9.5%, 148 tested)** (+4: `0x412160`, `0x564780`,
> `0x566570`, `0x566a80` — last three partial).
>
> **NEXT: wire the new-game config scene as a runnable DRIVE** (Next move #1).
> The grid renders bit-exact; what's missing is the live scene — the run loop
> (`0x565810`/`0x565d10` nav), the value toggle (id 0x27), the tooltip text node
> (`0x566850`), the box widget tree (`0x411940` geometry/title sub-nodes), and
> the Start→game transition (`0x564160`→`0x59ec30`).  Route `app_flow`'s
> `NEW_GAME` arm to it instead of the re-enter-title stub.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 36 — THE TEXT-RENDERER PIXEL GATE IS CLOSED: every GDI parameter
> matches retail's LIVE `TextOutA` stream, bit-for-bit.** Part 1 wired
> `ar_register_fonts` at boot + a `--render-glyph-test` offscreen DIB path
> (port side). Part 2 (this checkpoint) closed the verification gate from the
> retail side: a new **`frida_capture.py --textout-probe`** hooks
> `gdi32!TextOutA`/`ExtTextOutA` and records every glyph draw
> (x/y/bytes/colour/bk-mode + the selected `LOGFONTA`). Drove retail to the
> **new-game config menu** — the first GDI-text scene — and diffed its real
> output against the port's renderer. **Every parameter matches:** font
> (Courier New **7×18**, port slot 3), bk mode **TRANSPARENT**, **7 px/glyph**
> advance, **per-glyph `TextOutA`**, the **2-copy shadow** `(x+1,y)/(x,y+1)`,
> and all three colours — normal **0x3e537d**, shadow **0xa8b9cc**, focused
> **0xf08080**. Since GDI rasterizes deterministically from an identical
> `HFONT` + identical draw args, the glyph pixels are **bit-identical** → the
> renderer port (`glyph_grid_render`/`glyph_row_draw`, ckpt 35) is **correct**.
> Quirk **#63**. Ground truth saved:
> `tests/scenarios/new-game-through/goldens/retail-newgame-config-menu.png` +
> `…-textout.jsonl`. 691 host tests pass (no `src/` change — verification +
> tooling). Ledger **150/1490** unchanged.
>
> **Two key findings while driving there:** (1) under the hidden-window turbo
> harness retail runs **~15 flips/s** (vs ~127 native), and the title menu
> auto-enters a **gameplay demo by ~flip 900** — so the old `new-game-through`
> trace's Start-at-flip-2050 landed in the demo; **`trace-retimed.jsonl`**
> presses Start at **flip ~400** to catch the live title menu. (2) That demo
> paints a GDI **debug HUD** (stat numbers + "Bonus Mode" + "Please Hit Any
> Key") via a **different** text routine — a full **3×3 outline** (9 shadow
> copies + centre), NOT the menu's 2-copy shadow. Not parity-relevant (the
> port doesn't render the demo), documented in quirk #63.
>
> **NEXT: the new-game config scene + its menu BUILDER** (Next move #2). The
> renderer is proven; what's missing is the code that BUILDS the cells it walks
> (row pitch 28, origin x=72, the value columns "1:Easy"/"On", the focused
> row). Once the port builds these, it emits the identical `TextOutA` stream →
> the end-to-end stream/pixel diff is trivially 0. Route `app_flow`'s
> `NEW_GAME` arm to it instead of the re-enter-title stub.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 35 — TEXT/GLYPH PIPELINE, PART 2: the GDI text renderer is ported +
> host-tested.** Ported the **render half** of the dynamic-text system into new
> **`src/glyph_render.{c,h}`** (+ **`glyph_render_win32.c`**), the visual
> counterpart of ckpt 34's build half. Three functions: **`glyph_row_draw`**
> (`0x48e860`, per-glyph `TextOutA`, 7 px/byte advance → 14 px per SJIS pair),
> **`glyph_ruby_draw`** (`0x48e6d0`, the furigana pass — gated on
> `node->field_14`==0 for basic menus + a no-op on raw text, faithful but
> unexercised), and **`glyph_grid_render`** (`0x48e200` GDI branch — walk rows
> `[sel2..+stride)` × cols, position each cell, pick text/shadow COLORREFs by
> selection state, monospace right-align, draw the **2-copy drop shadow** +
> glyphs + optional ruby).
>
> GDI is injected through a **`glyph_gdi_ops`** vtable
> (`select_font`/`set_text_color`/`text_out`), so the walk + colour selection
> are **pure + host-tested** with a recording stub; the real back-buffer GDI is
> in `glyph_render_win32.c` (`glyph_gdi_ops_win32`), the project's `_win32.c`
> split (host harness never links it). Modelled `menu_cell` +0x10/+0x14 as the
> per-cell colour overrides. **Three findings (quirk #62):** (a) the renderer's
> `this` is the **child** node while the **parent** supplies the x/y base; (b)
> the shadow is two offset copies (`(0,+1)`,`(+1,0)`) in `node+0x184`; (c)
> `node+0x188`/`+0x194`/`+0x198` hold label **pointers** read as COLORREFs but
> only on dead paths — only +0x180/+0x184/+0x18c/+0x190 are live menu colours.
> The retail **sprite-cell mode** (`param_1==0`, ZDD blits) is deferred. 11 new
> host tests (**691 pass / 0 fail / 6 skip**). Ledger **150/1490 (9.2%)** (+3:
> `0x48e200`, `0x48e860`, `0x48e6d0`).
>
> **OPEN VERIFICATION GATE (human/Frida): the pixel diff.** The walk is
> host-tested but the glyphs are **not yet diffed vs retail**. Next move:
> wire `ar_register_fonts` at boot, render a known string offscreen via
> `glyph_gdi_ops_win32` into a DIB-section DC, and `differ_px`-diff the glyph
> region vs retail (font-probe Frida hook on `0x48e200`, or the new-game menu
> once it builds). THEN the new-game config scene (`0x564780` case 0x24) +
> row-append `0x40f800`. See **Next move** below.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 34 — TEXT/GLYPH PIPELINE, PART 1: the glyph layout builder is ported +
> host-tested.** Surveyed the dynamic-text subsystem
> (**`docs/findings/text-glyph-pipeline.md`**), two findings (**quirk #61**):
> (a) text renders through **Win32 GDI** (`ar_register_fonts` builds 8 `HFONT`s,
> `0x48e200` `TextOutA`s each glyph — no rasteriser to port); (b) builder
> (`0x40fa00`), row-append (`0x40f800`), renderer (`0x48e200`) all operate on the
> **same `menu_ctrl`/`menu_node`** in `menu_list.h`. Ported the **build half**
> into **`src/glyph_text.{c,h}`**: `glyph_token_search` (`0x40fd20`) +
> `glyph_cell_layout` (`0x40fa00`, string → `cell.obj0` records; escape pass
> hooked NULL = faithful for escape-free English). Corrected the **swapped
> Ghidra params** (`0x40fa00` param_1/param_2 are ROW/COL). Ledger then
> **147/1490 (9.0%)**.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 33 — POST-TITLE DISPATCH BACKBONE: the title menu is re-enterable;
> Exit exits.** Until now the port hard-shut-down on *any* `TITLE_SCENE_DONE`,
> so committing a menu row just quit. Ported the result→action mapping of
> retail's boot-driver outer loop `FUN_00562ea0` (562ea0.c:684-734) as the pure,
> Win32-free `app_flow_dispatch` (**`src/app_flow.{c,h}`**): `6/8→EXIT`,
> `9→EXIT_9`, `0x1a→NEW_GAME`, `0x1b→DEMO_START`, `0x1c→CONTINUE`,
> `0x1d→OPTIONS`, `0x1e→BONUS`, `0`/default→`REENTER_TITLE`. Wired into
> `main.c`: Exit sets `g_shutdown`; every (still-UNPORTED) sub-scene arm logs +
> calls the new **`reenter_title()`** which tears down the finished drive and
> rebuilds it (`build_title_drive(skip_intro=1)`), so the menu loops like
> retail's. **Verified live**: a trace to **Exit** → `result=8` → clean exit (no
> re-enter line); a trace confirming **Start** → `result=26` → stub log →
> drive rebuilds → menu reappears. Captures confirm the re-entered title
> **replays the intro from phase 0** (quirk **#60**: the `local_164`/`param_1`
> re-display arg does NOT skip the intro — it only enables a phase-0 skip-press;
> 56aea0.c:177/:182). 668 host tests pass (+1 `app_flow_dispatch_codes`).
> Ledger **145/1490 (8.9%)** unchanged (`0x562ea0` was already counted; this is
> a partial port of its tail → status `tested`).
>
> **The new-game/continue/options/bonus sub-scenes are stubs (re-enter title).**
> They are gated on the next big rock: the **glyph/text pipeline** (`0x40fa00`
> SJIS + `#`-colour escapes, `0x40f800`) + **font/sprite-batch registration**
> (`ar_register_fonts`, `0x57a330` palette ramps, `0x56e190` 442-sprite batch).
> That pipeline is the shared prerequisite for every menu AND the prologue
> narration — see **Next move**.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 32 — THE TITLE MENU IS INTERACTIVE (milestone 1): injected nav moves
> the cursor + commits.** Live-validated the `--input-trace` path and found the
> menu was **dead to input** despite rendering bit-exact: `menu_list_latch`
> gates all nav on `sub->ready==1000` (quirk #34), where `sub->ready` is the
> spawned node's `+0x54` ramp — `menu_node_build` zeroes it, so the gate starts
> **closed**. The driver that opens it is the title scene's **post-update**
> side effect **`FUN_0056c930`** (was stubbed NULL), **NOT** the per-entry
> update `0x43c2e0` (which only *reads* `+0x54`). `0x56c930`'s **mode-1** arm
> ramps the active node's `+0x54` **+50/frame to 1000** (node built mode 1,
> `+0x50`=1) → menu navigable **~20 update frames after spawn**. Ported the
> mode-1 arm as **`menu_owner_transition_step`** (`src/menu_list.c`; modes 0/2
> are submenu-slide paths the title never uses — deferred + documented), wired
> as the drive's `post_update` (`src/main.c` `drive_post_update`). Quirk **#59**.
>
> **Verified live** (new `--menu-trace` cursor-row diagnostic in
> `src/title_sink.c`): DOWN×4 walks the cursor `0→1→2→3→4`, UP walks back, and
> confirm (`0x24`) on row N returns that row's action id — `result=26` (`0x1a`
> Start) on row 0, `result=8` (Exit) on row 4. The ► arrow + row-highlight
> visibly track the selection (port `Start`-vs-`Options` capture pushed to
> llm-feed). 667 host tests pass (+7 ramp tests); ledger **145/1490 (8.9%)**
> (+1: `0x56c930`). NB the input gate (`+0x54`, +50/frame, open ~flip 547) opens
> *before* the cursor becomes visible (`fade==1000`, +20/frame, ~flip 577), so a
> press lands before the highlight appears — time demos after ~flip 577.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 31 — THE TITLE INTRO IS FULLY BIT-EXACT: phase-7 sparkle particles
> ported + `differ_px=0` (user-confirmed 1:1 incl. particles).** Closed
> parity-ledger R4, the last intro-content gap. The `FUN_0056c070` particle
> twinkles are **four sites over one cap-500 pool** (quirk #58): spawn
> `0x56c070`, **per-frame update `0x56ba69`** (rise/age), cull `0x56c030`,
> draw `0x56c180`. The twinkles **spawn at the reveal edge, rise upward
> (accelerating via the `+0x08` velocity field), animate frames 0→7 as they
> age, then cull at lifetime end — they "evaporate upwards", they do NOT
> accumulate.**
>
> The first cut spawned+drew but **froze** them (omitted the update) → an
> over-bright piled-up smear (8277 px diff). Finding + porting the update
> (`y_num -= vel; vel += 2; anim_num-- else cull`, inline at `0x56ba69` +
> swap-remove `0x56c030`) fixed it. **Determinism:** the engine LCG seed
> `DAT_008a4f94` is `srand(time())` (`0x56227a`), so retail's twinkles are
> wall-clock-random; the port pins a fixed seed (`OSS_RNG_DEFAULT_SEED
> 0x4f5347`) and the harness `--seed-pin` (default ON) writes the same value
> into retail at the first spawn. **Verified `differ_px=0`** at a matching
> update-tick: port Flip 465 vs `sparkle-align/frame_00939` (parity-ledger #4,
> user-confirmed). Off-tick frames differ only by the R3 render-rate sub-tick
> jitter (retail renders each update-state ~2.2×) + run-to-run intro-pacing
> jitter (first spawn flip 886/895/896) — align by the new `subtitle_anim_start`
> TAS anchor + tick, never a fixed flip. 660 host tests pass (+10); ledger
> 144/1490 (8.8%, +6 real ports: LCG/srand + spawn/update/cull).
>
> New port modules: `src/rng.{c,h}` (the MSVC LCG), `src/title_particles.{c,h}`
> (pool + spawn + update + cull). New harness: `--seed-pin`/`--seed-value` +
> the `subtitle_anim_start` anchor (`installSparkleAnchor`).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 30 — title intro CONTENT parity: both logos BIT-EXACT, sparkle sweep
> bit-exact.** Wired the last two deferred render-half arms (the ckpt-29
> recommended move). RE collapsed both into already-validated paths:
> - **LOGO**: the quirk-#40 "+4/+8 container fields" are just MAIN-bank
>   `frames[1]` (studio) / `frames[2]` (title) — `*(*slot)` is the frames array
>   `0x418470` indexes. The logo handler (`0x56bb5c`/`0x56bbd4`, alpha leaf
>   `0x494e10`) is **bit-identical to the sprite-level wrapper** `0x56c4e0`
>   (same `ramp_b`, same fade<=0/idx>=20/empty→plain-keyed rules; the only
>   `0x5bd550` a10-global difference is pixel-irrelevant). So `title_render_logo`
>   now emits one `TITLE_DRAW_SPRITE_LEVEL` (frame 1/2, raw fade). **Fixed a real
>   bug**: the old branch keyed on the scene `ramp`/`fade_ramp` param, never
>   populated by `main.c` → logos rendered **opaque, unfaded**. Now they fade via
>   the sink's `ramp_b`. Quirk **#56**.
> - **SPARKLE**: `0x56bcf7` copies 4×48 slivers of the menu-bg sprite (MAIN frame
>   5) src `(x,416)`→dst `(x,416)`, revealing the "Secret of the Elemental Stone"
>   subtitle column-by-column. Cmd now carries the raw clamped level + column
>   (the sink indexes `ramp_b` + calls `title_draw_sparkle`). Quirk **#57**.
>
> **Verified (R1 fade-matched method).** New `frida_capture.py --fade-probe`
> (hooks `0x448c80`, logs the per-flip logo fade in phases 0–4). **Studio logo
> phase 0 fade 640 → `differ_px=0`; title logo phase 3 fade 820 → `differ_px=0`**
> (parity-ledger #2/#3, **user-confirmed 1:1**). Sparkle full reveal (fade 1000):
> logo + subtitle match exactly; only residual = retail's **additive particle
> twinkles** from the separate, still-deferred `FUN_0056c070` spawn (parity-ledger
> R4 — a **noted gap**, user-acknowledged, not a sweep bug). 650 host tests pass
> (+2 sink sparkle tests); ledger 138/1490 unchanged (wiring). Fade-probe caveat:
> phase 7 logs the first *sparkle* level, not the raw fade — match by reveal
> extent there.
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 29 — R3 (intro pacing) diagnosed + fixed; hidden-window flicker fixed.**
> The "port rushes the intro" framing was **wrong**. Measured both sides with
> the real clock (new `frida_capture.py --pace-probe` + a `pace:` phase log in
> `src/main.c`): **wall-clock to the menu already matched retail (~9.2 s)**.
> The gap was render-rate: retail renders ~127 flips/s, duplicating each scene
> state ~2.2× (cursor probe: each `menu_fade` value spans ~2 flips); the port's
> fixed-timestep accumulator (`title_pace_step`) was driven **one pace-step per
> 16 ms-throttled main-loop iteration**, so `now` advanced per *update*, the
> budget refill ran away to ~6 updates/render, and the port **DROPPED ~5/6 of
> the intro's fade frames** (rendered 90 of ~528 update ticks; choppy).
> **Fix (`src/main.c` `main_loop_body`):** spin pace-steps (updates ~free) until
> one *present*, then `frame_limiter` gates the presented-frame rate — like
> retail's tight outer loop. Now 1 update/render: phase curve = canonical
> 51/102/153/254/275/316/437/**528**, every fade value rendered, wall-clock
> unchanged. R1 re-verified post-fix at `menu_fade=750` → **differ_px=0**.
> Flip-index-exact match to a golden = the capture rig's refresh (~127 Hz),
> **not portably reproducible**; the distinct-content sequence is, and now
> matches. Quirk **#54**; parity-ledger R3 resolved.
>
> Also fixed a **hidden-window screen flicker** the user reported: the port's
> mode-2 present BitBlts the *desktop* (`GetDC(NULL)`) every frame regardless of
> window visibility — now skipped under `--hide-window` (captures read
> `primary_obj` first, so lossless). Live diag showed **retail paints its
> *window* (`GetDC(hwnd)`), not the desktop → retail doesn't flicker; only the
> port did.** Quirk **#55**; likely port mismodel (desktop vs window present
> target) → follow-up task to confirm via disasm + correct `zdd_present` case 2.
> Commits `f886d10` (pace) + `5ba8f37` (flicker). 648 host tests pass; ledger
> 138/1490 unchanged (driving fixes + instrumentation, no new FUN).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 28 — R1 CLOSED: title menu is bit-exact (`differ_px==0`), cursor pulse
> RE'd.** The 955px cursor residual was the **cursor pulse**. Retail animates
> the cursor `level_num` (`FUN_0056c470` arg3 = `[esp+0x20]`) as a triangle wave
> — it's `local_58` in `FUN_0056aea0`, driven by the phase FSM (phase 8:
> `+50`/update to 1000; phase 9: `-50`/update to 0; `56aea0.c`:366-384). With
> the fixed `level_div=0x4b0` (1200), `idx=(local_58*20)/1200` peaks at **16**
> (NOT 19) and breathes to 0 (invisible). The port had pinned the cursor to a
> static idx-19 full-add → uniformly over-bright. The port *already* computed
> the value as `title_fade_state.menu_fade`; ckpt 28 threads it into the cursor
> draw. **Method (RE, not eyeballing — per user):** `frida_capture.py
> --cursor-probe` (new) hooks `0x56c470` and logged the 0→1000→0 step-50
> triangle; read `FUN_0056aea0`'s FSM to find `local_58`; wired it; validated
> `differ_px==0` by matching port frames (now log `menu_fade`) to retail goldens
> captured WITH `--cursor-probe` (known `local_58`) at equal pulse phase: port
> Flip 209 (mf=750) vs retail 1300 (l58=750) → 0px; port 203 (450) vs 1420/1460.
> **User-confirmed fully 1:1 bit-identical.** Commits `<this ckpt>` (tools +
> render fix). 648 host tests pass. Also fixed the Frida harness default exe
> (was spawning the packed DRM exe → 0 frames; now the co-located unpacked PE —
> engine-quirks #53). Ledger unchanged 138/1490 (wiring, no new FUN).
>
> ─────────────────────────────────────────────────────────────────────────────
>
> **ckpt 26 — THE PAYOFF: the port renders the real title screen.** Registering
> the title sprite banks at boot lit up the entire ported pipeline. The drop-in
> now decodes the actual Fortune Summoners title art from sotesd.dll and blits
> it: **full character art + background + "FORTUNE SUMMONERS" logo, then the
> title menu** (Start / Continue / Bonus Menu / Options / Exit + "Secret of the
> Elemental Stone" + copyright). The art/logo/menu-text region matched the
> retail golden (port frame 200 vs `runs/title-idle/frame_01900.png`) with the
> only diff being the (then-unwired) selection cursor — NOT a full bit-exact
> match (see ckpt 27 + parity-ledger R1). Self-verified via the new **port-side
> frame capture** (`--capture-frames`, BMP→PNG→read-image).
>
> The fix (commit `e00718b`): `init_sprite_banks()` in `main.c` —
> `LoadLibraryA("sotesd.dll")` + `ar_state_init()` +
> `ar_register_main_sprites(g_zdd, 4, hSotesd, hSotesd)`, called between
> `init_ddraw` and `init_title_drive`. **Key RE finding (engine-quirks #51):**
> `settings` is a *bare HMODULE* (no +0x3c record), and it is **sotesd.dll** =
> `DAT_008a6e74` (stored @ 0x5af5fc) — the asset-loader doc had it as sotesp.dll
> and was WRONG. All title resources live in sotesd.dll. Bank→slot map:
> `ar_pool_get_slot(19)`=`g_ar_sprite_slots[6]`=id 0x91b=MAIN;
> pool 20=slots[7]=0x91c=CURSOR — both populated by `ar_register_main_sprites`.
>
> 647 host tests pass (0 fail, 6 skip). Ledger **138/1490 (8.6%)** unchanged
> (wiring, not a new port). Frame capture (commit `dd4ef08`) is roadmap task #10.

**This is the first thing to read at the start of every session.**

Rolling state — REWRITE on each meaningful checkpoint. `docs/PROGRESS.md` is the
append-only changelog; this file is "where to pick up *right now*".

## ⭐ Current state (ckpt 55): title bit-exact loop; new-game scene bit-exact (box/text/cursor/tooltip + picker); prologue gem cutscene content bit-exact (63/64) + the fade-out timing ported. Milestone 1 (title+new-game+prologue) is 1:1 for rendered content. **Milestone 2 (the game proper) IN PROGRESS**: the in-game seam is wired + anchored (game_enter, both sides, rng-matched), the engine 0x59f2c0 is surveyed/decomposed, **plan 3a is RESOLVED** (town banks identified + boot batches g2/g3/g5 wired, title differ_px=0), **the `game_drive` scaffold is stood up** (ckpt 52, black map-load frame), the **static world tables are extracted** (ckpt 53), the **world-table layer is PORTED + host-tested** (ckpt 54) — `src/game_world.{c,h}` (registry build + `FUN_00585000` cross-reference + `FUN_00561c90` lookup) over generated `src/world_tables_data.{c,h}`, with the **`0x3f2`→opening-room resolution RESOLVED** (map 0x3f2 → key `0x334be` = room **210110 "Town of Tonkiness"**, scene 1022 — NOT Tolkien/110110), and (ckpt 55) the **MAP OBJECT is PORTED + host-tested** — `src/game_map.{c,h}` ports the `0x59f2c0` fresh-entry arm (the 0x4120 map object + 8 actor slots + header field init) + the `FUN_004c5350` room-key resolution (`map+0x4024=0x334be`), with `game_map_active_room` resolving end-to-end to room 210110 over the `game_world` layer. STILL UNPORTED (plan 3b): the sim 0x586010, the render 0x5a00c0 (and the map object's opaque sub-inits 0x5612b0/0x5611d0/0x4e59a0). Neither `game_world` nor `game_map` is wired into main.c yet (they're the foundation those units read). Deferred: the EXE-NULL town banks 0x570-0x572 (confirmed in sotes.exe .rsrc; port loads via LoadLibraryExA(sotes.exe,AS_DATAFILE); registration coupled to engine-time slot indices, plan 3b); the fade-out frames' RENDER (box-alpha arm).

> **User @ ckpt 44:** "can confirm 1:1 except for sparkle on cursor."  → The
> whole new-game screen (box + menu text + cursor + tooltip help text) is
> **user-confirmed 1:1**.  The "sparkle on cursor" they see is the **cursor's
> animated gold feather/glint at a different animation phase** than the single
> frozen golden frame — the SAME animation-phase sampling caveat as the intro
> twinkles: at the matching tick (Flip 761, frame 17/19) it is `differ_px=0`
> (ckpt 43), off-phase frames differ only by the feather's phase (gold pixels
> shifted, roughly balanced golden-brighter/port-brighter — NOT a missing
> additive sparkle, NOT a content gap).  **Nothing to fix here.**  (The other
> tiny residual — 9px box-panel RGB565 1-LSB rounding — is non-text, pre-existing
> box decode, see ckpt-44 OPEN.)  **The new-game scene RENDER is parity-complete;
> next is BEHAVIOUR: the option picker submenu + the Start→game transition —
> Next move #1c/#1d.**

The **new-game config scene** is live + interactive (ckpt 39) AND now renders the
**bordered cream box panel** behind the menu (ckpt 40).  `newgame_render`
(`main.c`) composes the frame retail-style: clear primary → draw the two 9-slice
box panels via DDraw (**menu box (32,32)400×124** + **tooltip box (32,392)576×80**)
→ GetDC + `glyph_grid_render` the menu text on top (base (32,32), Courier New
7×18 = font slot 5).  The 9-slice render is the pure, host-tested
**`src/newgame_box.{c,h}`** (port of `0x48cf80`'s opaque arm, quirk #67): a tiling
walk over a `newgame_box_ops` vtable, bank **PE resource 0x457** (=
`AR_SPR_FONT_TEX_457`, already registered by `ar_register_fonts`, 32×32 cells,
frames tl0/top1/tr2/l3/c4/r5/bl6/b7/br8, center = cream RGB(239,227,214)); the
real blit (`ar_sprite_slot_frame` + `zdd_object_blt_clipped` = `FUN_005b9bf0`) is
the main.c adapter.

The scene composes from the already-ported builder (`newgame_menu.c`, ckpt 37),
text renderer (`glyph_render.c`, bit-exact ckpt 36), run-loop model
(`newgame_scene.c`, ckpt 38), drive (`newgame_drive.c`, ckpt 39, quirk #66), and
now the box (`newgame_box.c`, ckpt 40).  **Verified LIVE** (port frame 760): menu
box **differ_px 307/50800 (0.6%)** vs the golden, with **all 307 residual pixels
in the top-left corner** = the **deferred animated sparkle overlay** (`0x48d940`,
bank **0x3e8**, frames 16–19, at (44,29)); the 9-slice panel + menu text are
**bit-exact everywhere else**.

What's left for the scene is the **remaining chrome + transitions**: the sparkle
corner (`0x3e8`), the tooltip TEXT node (y=416/444, word-wrapped — box drawn, text
computed by `newgame_scene_tooltip`, needs the word-wrap split), the option
picker submenu (`0x567ba0` — a kind-0 confirm yields NEWGAME_OPEN_PICKER, surfaced
but inert), the box fade-in (`0x48cf80`'s alpha arm), and the Start→game path (the
Elemental-Stone intro `0x564160`→`0x5642e0`→`0x59ec30`).  See **Next move #1**.

The title is feature-complete as a *loop*: intro bit-exact, menu interactive
(ckpt 32), and now the menu-commit return code is **dispatched** like retail's
boot driver instead of force-quitting (ckpt 33, `src/app_flow.c` +
`reenter_title`/`build_title_drive` in `main.c`). **Exit (8) → clean shutdown;
every other commit → the sub-scene (all UNPORTED stubs for now) → re-display the
title** (which replays the intro from phase 0, faithful — quirk #60). The
sub-scenes themselves do not render yet; that's the next rock (glyph/text
pipeline — see Next move).

The whole intro chain is bit-exact, AND the menu responds to input: injected
`--input-trace` nav moves the cursor (both directions) and confirm returns the
selected row's action code (ckpt 32: `FUN_0056c930`'s mode-1 `+0x54` ramp,
`menu_owner_transition_step`, wired as `post_update`, quirk #59). Use
`--menu-trace` to log cursor-row changes. **The intro render chain:**

```
title_scene_step → title_sink → resolve_frame(bank 19/20)
   → ar_sprite_slot_frame(slot, id)
        → ar_sprite_decode (0x4184a0)  [NOW FIRES — banks registered]
             → bs_decode_resource(sotesd.dll, 0x91b, "DATA", compressed)
             → ar_sprite_slice (0x4188b0): trim + format switch + build
                  → ar_sheet_format_hook → bs_convert_to_16bpp (RGB565)
                  → ar_frame_build_hook → zdd_object_new_cell (8d) → real surface
        → title_draw_sprite → keyed blit onto primary → present → VISIBLE
```

**Verified BIT-EXACT** (`differ_px==0`) against retail goldens for: the title
menu + selected-row cursor (R1, parity-ledger #1, ckpt 28); the **studio logo**
(phase 0, fade 640, #2) and **title-art logo** (phase 3, fade 820, #3) — both
ckpt 30, **user-confirmed 1:1**; the **phase-7 subtitle-reveal sweep** at full
reveal; and (ckpt 31) the **phase-7 particle twinkles** (#4, port Flip 465 vs
`sparkle-align/frame_00939`, seed-pinned `0x4f5347`, **user-confirmed 1:1 incl.
particles**). **Intro pacing (R3) is correct** (ckpt 29): every fade tick
renders at ~60 Hz, wall-clock to menu matches retail (~9.2 s).

**parity-ledger R4 is CLOSED (ckpt 31).** The intro has **no known content
divergence left**. The only non-bit-exact thing about the intro is the
**flip-INDEX** at which a given state renders (R3 render-rate ~2.2× + run-to-run
intro-pacing jitter) — which is **not portably reproducible by design** and not
chased; the distinct-content sequence + every random-particle frame match at
tick alignment. Seed pinning (`OSS_RNG_DEFAULT_SEED 0x4f5347` port-side,
`--seed-pin` harness-side, default ON) is what makes the RNG-driven twinkles
comparable; align frames by the `subtitle_anim_start` anchor + update-tick.

## R3 is resolved — what "pacing" did and didn't mean (read before re-opening)

The port's *wall-clock* pacing already matched retail; the Flip-INDEX gap is a
render-rate artifact (retail ~127 flips/s duplicating each state ~2.2×; the port
now renders each state once at ~60 Hz). **Flip-index-exact** match to a specific
golden = the capture rig's refresh and is **not portably reproducible** — do not
chase it. The achievable, meaningful target is the *distinct-content sequence*,
which now matches (every fade value rendered, no drops). The pace pump `0x5b1030`
and the pre/post side-effect hooks stay stubbed **on purpose** — they are NOT the
pacing key (that was the driving cadence, fixed in `main_loop_body`); they matter
only for the BGM cue / per-entry updates, port them when those subsystems land.

## Next move (pick one — recommendation first)

> Context: the title is a bit-exact loop; the new-game config scene renders bit-
> exact (box + text + cursor + tooltip) with the picker user-confirmed; the
> prologue gem cutscene renders live + is **content bit-exact** (ckpt 47, 63/64
> dense), AND the new-game→prologue **fade-out transition timing is now ported**
> (ckpt 48) so the cutscene fade-in diffs `differ_px=0` at a constant offset.
> The active goal (user, ckpt 13) is **1:1 parity** for title + new-game +
> prologue — which is now essentially MET for the rendered content.  Two things
> remain: the fade-out frames' RENDER (a known sub-item) and the user-decision on
> extending the trace in-game (we now HAVE prologue + main-menu rendering).

1. **★ (recommended — the user opted in @ ckpt 48) PORT THE IN-GAME ENGINE
   `0x59f2c0` to match the golden.**  The trace is extended in-game, the RETAIL
   golden is captured (`runs/tas-ingame-1`, opening town from ~flip 1150), and
   (ckpt 50) the **game-entry anchor + port seam are DONE + verified both sides**
   (port game_enter@1116 / retail @1092, rng 0x40d00581 matches).  The engine is
   **surveyed/decomposed** (see `docs/findings/in-game-intro.md` "Entry-function
   map").  Remaining = **plan 3** in that doc, smallest visible win first:
   - ~~(a) game-entry anchor~~ **DONE (ckpt 50)** — retail `0x59f2c0` + port
     `enter_game` emit `game_enter`.
   - ~~(b) port seam~~ **DONE (ckpt 50)** — `PROLOGUE_DONE → enter_game`.
   - ~~(3a) Find map-0x3f2's resource banks + load path~~ **DONE (ckpt 51).**
     No per-map file: the town lazily decodes pre-registered sprite banks via
     `ar_sprite_decode`; map layout is compiled-in static data.  Wired the
     deferred boot batches g2/g3/g5 (title still `differ_px=0`).  Residual: the
     EXE-NULL banks `0x570-0x572` (register with `settings=NULL` in 3b).  Probe:
     `frida_capture.py --res-probe`.  Full map: in-game-intro.md "Resource banks".
   - ~~(3b-i) **Stand up a `game_drive`**~~ **DONE (ckpt 52)** —
     `src/game_drive.{c,h}` mirrors `prologue_drive`; `enter_game` stands it up,
     `main_loop_body` steps it once/frame; `game_render`=`zdd_object_clear`
     (black map-load frame).  Verified live (game_enter@1116, in-game frames black,
     title unaffected).
   - ~~(3b-ii-α) **The world-table layer**~~ **DONE (ckpt 54)** —
     `src/game_world.{c,h}`: the `&DAT_006940c8` registry copy + `0x585000`
     cross-reference + `0x561c90` lookup, with the `0x3f2`→room-210110
     resolution proven.
   - ~~(3b-ii-β) **The map object**~~ **DONE (ckpt 55)** —
     `src/game_map.{c,h}`: the `0x59f2c0` fresh-entry arm (the 0x4120 map object
     alloc + 8 actor slots + header field init) + the `FUN_004c5350` room-key
     resolution (`map+0x4024=0x334be`).  `game_map_active_room` resolves
     end-to-end to room 210110 over `game_world`.  Host-tested
     (`tests/test_game_map.c`).
   - (3b-ii-γ) **The sim + a render slice (NEXT).**  `game_render` today clears
     black; to draw the town, port the **0x586010 sim step** (enough to populate
     what the backdrop reads — it allocs `DAT_008a9b50` 0x27b8, refs
     `"Start StartArea"`), then a slice of **`0x5a00c0`** (13.7 KB; reuses ported
     `ar_sprite_decode`/zdd/ramps), feeding them the ckpt-54/55 `game_world` +
     `game_map`.  Target the **static town backdrop/tilemap FIRST** (golden flip
     ~1150), diff vs `runs/tas-ingame-1` anchored on `game_enter`.  Also wire
     `game_map`/`game_world` into `main.c`'s `game_drive` (today neither is
     called).  Register the EXE-NULL banks 0x570-0x572 at the engine-time site
     once the slot indices surface (settings=a `sotes.exe` datafile handle).
     Then entities, then the dialogue box (~flip 2200, likely the glyph pipeline
     again).  Use `--res-probe` on the PORT too to confirm it decodes the same
     bank set as retail.
   The two giant children — **`0x586010` (6 KB, sim/draw step)** + **`0x5a00c0`
   (13.7 KB, render dispatch)** — are the real body; `0x59ec30`/`0x59f2c0` are
   the wrapper + world-setup/loop around them.

2. **(deferred polish) The fade-out frames' RENDER — the box-alpha arm.**  The
   transition TIMING is aligned (ckpt 48) but the port re-renders the new-game
   menu **opaque** during the 20 fade frames, while retail fades the box-panel
   alpha (`0x48cf80`'s alpha arm via `0x5bd550`) + the GDI menu text.  So those
   ~20 transition frames (port ~801-820 vs retail ~795-814) are NOT yet bit-
   exact.  To close: port `0x48cf80`'s alpha arm (the 9-slice box blit blended by
   `node.field_54`) and capture **retail's fade-out frames** (~795-814 — drive
   retail to the commit, capture that window) to model how the GDI menu text
   fades during the transition (likely the scene is composited to an offscreen
   then alpha-blitted — needs the capture to confirm).  Self-contained; doesn't
   need new scope.  The fade STATE (`field_54` ramp) is already tracked, so this
   is purely the render half.

3. **(verification, cheap) Re-capture retail's cutscene under lockstep + re-run
   `tas_diff` to re-confirm 63/64 after any prologue change.**  The recipe is in
   `docs/findings/tas-harness.md` (port@821 vs retail@815, runs/tas-retail-gem,
   window 2).  The pre-existing **tick-0 entry-frame** residual (84684 px — the
   port renders the first gem tick while retail's first present at prologue_enter
   is still black) is a small prologue_drive first-present item worth closing too
   (defer the first prologue render by one tick so tick 0 is black like retail).

3. **(deferred polish, optional) Finish the new-game scene's remaining CHROME.**
   The box panel + menu text + selection cursor render bit-exact; port the rest in
   roughly this order (cheapest visual win first):
   ~~(a) the box widget panel~~ **DONE (ckpt 40, quirk #67, `src/newgame_box.c`).**
   ~~(a') the selection cursor~~ **DONE + BIT-EXACT (ckpt 43, quirk #69).**
   Geometry ported ckpt 41 (`src/newgame_cursor.{c,h}`, `0x48d940` type-1, base 16
   + frames{0,1,2,3}→16-19, row-0 base (40,26)); bank ID'd ckpt 42 (0x455 slot 43,
   bottom-up, quirk #68); the render bug fixed ckpt 43 (a transposed trim scan, NOT
   the mis-diagnosed videomem path — `bs_trim_opaque_rect` arg order, quirk #69).
   `g_newgame_cursor_enable=1`.  Verified: port Flip 761 vs golden → menu-box
   `differ_px=0`.  Proof tool for the trim: `tools/extract/cursor_trim_probe.c`.
   ~~(b) the tooltip TEXT node~~ **DONE + BIT-EXACT (ckpt 44, quirk #70,
   `src/glyph_wrap.{c,h}`).**  The help line is a standalone WORD-WRAPPING text
   node (`this+0x170`), not the menu grid; the break is greedy word-wrap at
   width 68 (NOT a `%n` — the difficulty source string has none).  Ported the
   justify `FUN_0040e5e0` (ASCII path) + the `%n`/`%m`/`%w` parse `FUN_0040f040`;
   `newgame_render` picks `newgame_scene_tooltip`, wraps at 68, draws each row at
   (72,416+r·28) with the menu's shadow+colours.  Verified: difficulty tooltip
   wraps 65/52 across y=416/444 → **0 text-colored pixels differ**.  (The build
   chain `FUN_0040e360`→parse/justify/commit + the substitution table —
   `this+0x164`, empty for English — is documented in quirk #70; the port fuses
   parse+justify and renders directly, as main.c already does for the menu grid.)
   **REMAINING tooltip residual = 9px box-panel RGB565 1-LSB rounding (pre-existing,
   NOT text)** — see ckpt-44 OPEN above; a separate sprite-decode item.
   ~~(c) the option picker submenu~~ **DONE + USER-CONFIRMED (ckpt 45, quirk #71,
   `src/newgame_picker.{c,h}`).**  `FUN_00567ba0` ported pure (value list
   `FUN_00568320` + build + seek `FUN_00419900` + nav/commit/cancel), wired into
   `newgame_drive` as a modal submode, rendered at (288,128) over the menu; commit
   calls `newgame_scene_set_option`.  Bit-exact retail diff is an OPEN gate (the
   flip counter freezes in `0x565d10`'s modal pump — capture/inject are flip-keyed;
   needs a non-flip-keyed harness).  **NEXT — the transition (recommended below):**
   (d) the **Start→game path**: the Elemental-Stone intro (`0x564160` →
   `0x5642e0`/`0x56cd20` timed cutscene → `0x59ec30` game proper).  START is a
   stub today (re-displays title).  **This is the prologue critical path** — the
   committed trace navigates to "Start Game" + confirms (never opens the picker),
   so this is the item actually on the path to "first frame in-game".
   Deferrable polish: the box **fade-in** (`0x48cf80`'s alpha arm via `0x5bd550`);
   the picker bit-exact gate (a harness that drives/captures inside the modal pump).
   How to drive there (PORT side, self-serviceable): trace `{"frame":620,"ids":[36]}`
   into the **game dir**, then `./build/opensummoners-launcher.exe --timeout-ms 45000
   -- /tmp/oss.exe --hide-window --frames 1100 --input-trace ng_trace.jsonl
   --capture-frames "700,760,840" --capture-dir=C:\osscap` (use a **no-space**
   capture dir — the launcher splits the game-dir path on its space).  Frames
   after ~660 are the newgame scene (`phase=-1`).  RETAIL side: `--box-probe`
   (Flip freezes at 422 in the modal pump — flip-gated probes see only the
   title→menu transition, which is when the box first renders).
2. ~~The new-game config menu BUILDER.~~ **DONE (ckpt 37, quirk #64).**
   `src/newgame_menu.{c,h}` builds the case-0x24 grid (`FUN_00564780` case 0x24 +
   `FUN_00411940` setup); host-tested to emit retail's `TextOutA` stream
   draw-for-draw (`tests/test_newgame_menu.c`).  Geometry: col0 x=72, col1 x=232,
   pitch 28.  Remaining chips for the broader text system: the escape expander
   (`0x4034f0`/`0x4051d0`, hooked NULL — English labels don't need it), the
   sprite-cell render mode (`0x48e200` `param_1==0`), and the screen-settings
   row-append twin `0x40f800` (used by case 0x20, not case 0x24).
3. ~~Dispatch the title return code instead of exiting~~ **DONE (ckpt 33,
   `app_flow_dispatch` + `reenter_title`, quirk #60).** Exit exits; commits
   dispatch; unported arms re-display the title.

## Tooling added ckpt 36

- **`frida_capture.py --textout-probe [--textout-frames LO,HI]`** — hooks
  `gdi32!TextOutA`/`ExtTextOutA` in retail and logs each glyph draw
  (x/y/glyph-bytes/text-colour/bk-mode + the **selected `LOGFONTA`** via
  `GetCurrentObject(OBJ_FONT)`+`GetObjectA`) to `<run>/textout.jsonl`, deduped
  to the distinct draw set. The flip window skips the intro/demo debug-text
  flood (the probe returns before any GDI query outside `[LO,HI]`). This is the
  **GDI-text ground-truth** capture: which `ar_register_fonts` HFONT a scene
  picks, the colours, and the per-glyph advance. The `installTextOutProbe` /
  `ensureTextOutQueryFns` / `readSelectedFont` pattern in the agent generalises
  to any GDI primitive.
- **`tests/scenarios/new-game-through/trace-retimed.jsonl`** — presses Start at
  flip ~400 (the live title menu under the harness), vs the old `trace.jsonl`'s
  flip-2050 which lands in the auto-demo. Use this to drive retail to the
  new-game config menu.
- **`tests/scenarios/new-game-through/goldens/`** — `retail-newgame-config-menu.png`
  (the captured menu) + `retail-newgame-config-textout.jsonl` (the full
  per-glyph stream) — the parity ground truth for the menu builder port.

## Tooling added ckpt 32

- **`--menu-trace`** (`src/title_sink.c`, `title_sink_menu_trace`) — logs a
  stderr line whenever the highlighted menu row changes
  (`[sink] menu cursor row 1 -> 2 (y=80)`), so injected nav is verified at the
  cursor-state level, not by eyeballing pixels. A CLI flag, **not** an env var:
  WSLInterop does not forward arbitrary Linux env vars to the Windows child
  (only nix-shell-exported ones like `OPENSUMMONERS_GAME_DIR` reach `getenv`).
- **Menu-nav trace recipe** (self-serviceable, no Frida): write a
  `{"frame":N,"ids":[..]}` JSONL into the **game dir** (the child's CWD; a
  Windows exe can't read `/tmp`), then
  `OSS=/tmp/oss.exe; $OSS --hide-window --menu-trace --frames 720 --input-trace trace.jsonl`.
  Button ids: **1=up, 3=down**, 2/4=page, **0x24(=36)=confirm**, 0x22=abort.
  Time presses **after ~flip 577** (cursor visible); the input gate opens
  earlier (~flip 547). Confirm on row N → scene returns that row's action id
  (Start `0x1a`=26, Continue `0x1c`, Bonus `0x1e`, Options `0x1d`, Exit `8`).

## Tooling added this ckpt (31)

- **`frida_capture.py --seed-pin` / `--seed-value`** (default ON, `0x4f5347`) —
  the agent hooks `FUN_0056c070` and writes `DAT_008a4f94` to the fixed seed at
  the first spawn, so retail's phase-7 twinkles match the port's pinned-seed
  build. One-shot.
- **`subtitle_anim_start` TAS anchor** — the same first-spawn hook always emits
  `{kind:'anchor', name:'subtitle_anim_start', frame}` (recorded in the run
  summary's `anchors`), independent of seed-pin. Use it as tick 0 to align
  captures, since retail's intro pacing jitters the flip index run-to-run.
- **`/tmp` diff scripts** (not committed): per-tick `differ_px` sweep of port
  frames vs a dense retail flip window — how `differ_px=0` was found at
  port-465/retail-939. Re-derive with PIL `ImageChops`/numpy if needed.

## Tooling added ckpt 30

- **`frida_capture.py --fade-probe`** — hooks `FUN_00448c80` (the fade→alpha
  ramp), logs the first `(value,div)` per Flip → `<run>/fade_level.jsonl`. In
  phases 0–4 the first call's value IS the studio/title logo fade, so this gives
  retail's logo fade per flip → match a port frame at the same fade and diff (how
  logos #2/#3 were verified `differ_px=0`). **Caveat:** in phase 7 the first call
  is the first *sparkle* (`min(7·fade,1000)`), not the raw fade; phases 5–6 don't
  call `0x448c80` at all (the gap in the jsonl pinpoints them). Generalises the
  `--cursor-probe` pattern (`installFadeProbe` in the agent).

## Tooling added ckpt 29

- **`frida_capture.py --pace-probe`** (+ `--pace-every N`) — timestamps Flips →
  `<run>/pace.jsonl` + a live `flips/s` print, and stamps the cursor-onset
  event with wall-clock ms. This is how R3 was measured (retail: ~127 flips/s,
  menu onset Flip 1172 @ 9.23 s). Use `--no-turbo`.
- **Port `pace:` phase log** (`src/main.c`) — logs each phase transition with
  Flip count + wall-clock (`pace: phase A -> B @ flip=N t=Mms`). The port-side
  counterpart of `--pace-probe`; how the port's wall-clock-to-menu was checked.
- **`/tmp/pace_sim.py`** (not committed) — a Python replica of `title_pace_step`
  + `title_fade_step` used to validate the driving fix offline (ratio 1.00, 0
  missed fade values) before touching C. Re-create from quirk #54 if needed.
- **`--hide-window` now skips the desktop present** (`drive_present`) — kills
  the screen flicker (quirk #55); captures unaffected (read `primary_obj` first).

## Tooling from ckpt 28

- **`frida_capture.py --cursor-probe`** — hooks retail `FUN_0056c470` (menu
  cursor draw), logs per-Flip `level_num`/`level_div` → `<run>/cursor_level.jsonl`
  + a distinct-value summary. The pattern (read 8 stack slots, find the known
  `0x4b0` div to anchor the arg layout, tag by `g_flip_frame`) **generalises to
  any FUN_ whose args you want to measure live** — copy `installCursorProbe` in
  the agent + the message handler in `frida_capture.py`. Use `--no-turbo`.
- **Port capture now logs pulse state** (`src/main.c`): each `--capture-frames`
  line prints `phase=… fade=… menu_fade=…` so a port frame can be matched to a
  retail golden at the same cursor-pulse phase (capture goldens WITH
  `--cursor-probe` to know their `local_58`, then diff at equal value → 0 px).
- **Harness default exe fixed** — `frida_capture.py` now spawns
  `vendor/original/sotes.unpacked.exe` (the unpacked PE co-located in the game
  dir), NOT the packed `sotes.exe` (which stalls at 0 frames). engine-quirks #53.
- (still here) `--capture-frames`, `SINK_RESOLVE_DEBUG`, `push_comparison.py`.
- **`docs/parity-ledger.md`** — entry **#1 is now CONFIRMED bit-exact** (title
  menu, phase-matched, `differ_px==0`). Re-diff + update after render changes.

## Module inventory (25 modules) — render pipeline COMPLETE; text pipeline CLOSED end-to-end; new-game config scene = box + menu text + cursor + tooltip text all bit-exact + the option picker submenu ported/user-confirmed (Start→game pending)

Pixel-Drawer, Asset-Register, Bitmap-Session, WndProc, ZDD wrapper, cs_dispatch,
app_pump, title_scene (`FUN_0056aea0`, fully ported+wired+driven), input
(`FUN_0043c110`), obj_container, menu_list, **title_render** (compositor +
wrappers), **title_sink** (cmd→ZDD bridge, banks 19/20), **title_drive** (caller
side of the runner), **rng** (the MSVC LCG `FUN_005bf505`/`_5bf4fb`, ckpt 31),
**title_particles** (phase-7 sparkle pool: spawn `0x56c070` + update `0x56ba69` +
cull `0x56c030`, ckpt 31), **app_flow** (post-title dispatch = `FUN_00562ea0`
tail switch, ckpt 33), **glyph_text** (the cell-grid text/glyph layout builder:
`glyph_cell_layout`/`glyph_token_search` = `0x40fa00`/`0x40fd20`, ckpt 34 —
build half; escape expander still hooked-NULL), **glyph_render** (the GDI text
renderer: `glyph_grid_render`/`glyph_row_draw`/`glyph_ruby_draw` =
`0x48e200`/`0x48e860`/`0x48e6d0`, ckpt 35 — pure walk over a `glyph_gdi_ops`
vtable + real GDI in `glyph_render_win32.c`; sprite-cell mode deferred),
**newgame_menu** (the new-game config menu builder: `menu_grid_append` =
`0x412160`, `newgame_option_label`/`_value`/`_tooltip` =
`0x566570`/`0x566a80`/`0x566850` arms, `newgame_config_build` = `0x564780`
case 0x24, ckpt 37/38 — emits retail's `TextOutA` stream draw-for-draw),
**newgame_scene** (the run-loop model = `0x564780` case-0x24 loop body, ckpt 38:
`newgame_scene_tooltip`/`_dispatch`/`_set_option` — pure state machine; the
Win32 pump `0x565d10`/`0x43bca0`, picker `0x567ba0`, box widgets `0x411940` NOT
ported, the drive's job),
**newgame_box** (the 9-slice box panel = `0x48cf80` opaque arm, ckpt 40),
**newgame_cursor** (the selection-cursor geometry = `0x48d940` type-1, ckpt 41 —
bank 0x455 frames 16-19 bottom-up, render bit-exact ckpt 43, quirk #68/#69),
**glyph_wrap** (the tooltip text-node word-wrap = `0x40e5e0` justify + `0x40f040`
`%n`/`%m`/`%w` parse, ckpt 44, quirk #70 — pure + host-tested; SJIS kinsoku
deferred; rendered directly in `newgame_render`),
**newgame_picker** (the option picker submenu = `0x567ba0` default arm, ckpt 45,
quirk #71 — value list `0x568320` + build + cursor-seek `0x419900` + nav/commit;
wired into `newgame_drive` as a modal submode, rendered at (288,128); bit-exact
retail diff an OPEN gate — flip-frozen modal pump).
**In-game (milestone 2):** **game_drive** (the in-game drive scaffold, mirror of
`prologue_drive`; ckpt 52 — renders the black map-load frame, engine unported),
**game_world** (the world-table layer = registry build + `FUN_00585000`
cross-reference + `FUN_00561c90` lookup over generated `world_tables_data`; ckpt
54), **game_map** (the runtime map object = `0x59f2c0` fresh-entry arm + the
`FUN_004c5350` `0x3f2`→`map+0x4024=0x334be` room-key resolution + 8 actor slots;
ckpt 55 — resolves end-to-end to room 210110).  `game_world`/`game_map` are pure
+ host-tested but NOT yet wired into `main.c` (the foundation the sim 0x586010 /
render 0x5a00c0 will read).
**8d** (`zdd_object_new_cell/_build_cell/_copy_cell_pixels`
+ `bs_convert_*` + slicer) ported ckpt 25, **now firing live** (banks registered
ckpt 26). `main.c` drives the scene against the live ZDD with the 8d hooks +
`init_sprite_banks` wired; on a menu commit it `app_flow_dispatch`es the result
(Exit→shutdown, else→`reenter_title`). `--no-title-scene` restores the legacy
present loop.

## How to run / verify live (self-serviceable — [[reference_frida]])

```
# build (single-TU, full rebuild) inside nix develop:
make -C src all && make -C tests run        # 647 pass / 0 fail / 6 skip

# capture title frames (writes BMPs into the game dir = Windows C: drive):
cp build/opensummoners-debug.exe /tmp/oss.exe
./build/opensummoners-launcher.exe --timeout-ms 35000 -- /tmp/oss.exe \
    --hide-window --frames 2200 --capture-frames "60,200,400,700"
# then BMP->PNG (PIL) from /mnt/c/.../Fortune Summoners/port_frame_*.bmp and read it
```

NB Flip frames advance ~1 per 2 main-loop iterations (pace split), so reaching
Flip 700 needs a generous `--frames`/timeout. `run-opensummoners.sh` rebuilds
the debug exe with default flags — use the launcher directly if you need a
`-DSINK_RESOLVE_DEBUG` build to survive.

## Active goal (UPDATED, user @ ckpt 48/49 — the trace is now extended in-game)

**The title + new-game + prologue scenes are 1:1 (title bit-exact; new-game box/
text/cursor/tooltip/picker bit-exact + user-confirmed; prologue content
bit-exact 63/64 + the new-game→prologue transition timing aligned, ckpt 48).
The ckpt-13 milestone is MET.**

**NEW directive (user @ ckpt 48): EXTEND THE TRACE IN-GAME.**  *"yes extend the
trace.  start by just spamming Z after the prologue begins, give the trace ~1
min of frames, then continue matching retail."*  Done the first half (ckpt 49):
the trace spams Z (id 0x24, a prologue beat) → `0x59ec30(0,0,0x3f2)` → the
opening town of Tilelia + story dialogue; RETAIL golden captured
(`runs/tas-ingame-1`, game from ~flip 1150).  **Now: port the in-game engine
`0x59f2c0` to 1:1-match that golden** — same port-and-diff loop, scene by scene.
See `docs/findings/in-game-intro.md` + Next move #1.

## Open RE threads (see ROADMAP subsystem map for the rest)

- **Title render-half arms — ALL WIRED + bit-exact** (`title_sink.c`):
  `MENU_CURSOR` (ckpt 28), `LOGO` (folded into `SPRITE_LEVEL`, ckpt 30),
  `SPARKLE` subtitle-reveal sweep (ckpt 30), and the **`FUN_0056c070` particle
  twinkles** (spawn/update/cull/draw, ckpt 31, quirks #57/#58 — DONE, bit-exact).
  (`TITLE_DRAW_LOGO` sink case + the `draw_logo`/`draw_sparkle`/`draw_cursor` ctx
  callbacks are now vestigial fallbacks; nothing emits LOGO.)
- **Outer-loop side-effect hooks** (stubbed in `title_scene_hooks`): `0x5b1030`
  (message pump), `0x43e140`/`0x40fe00`/`0x566250` (pre), `0x43c2e0` (per-entry).
  **`0x56c930` (post) is now WIRED** (ckpt 32) — its mode-1 `+0x54` ramp opens
  the menu-input gate (`drive_post_update` → `menu_owner_transition_step`); modes
  0/2 (submenu slide) are still deferred inside that port. **NB the rest are NOT
  the intro-pacing key** (that was the driving cadence, fixed in `main_loop_body`
  ckpt 29 — see quirk #54); `0x43e140`/`0x40fe00` are audio/joystick updates,
  port when those land. `0x43c2e0` animates a node's *child* widgets (gated on
  `+0x54>=1000`) — needed for in-row sub-widget animation, not basic nav.
- **Other register batches** not yet called at boot: `ar_register_fonts`,
  `ar_register_palette_ramps` (FUN_0057a330), the big `FUN_0056e190` (442
  sprites), sounds. The title path doesn't need them, but the new-game/prologue
  scenes will — register them the same way (all take the sotesd HMODULE).
- **Dynamic text pipeline — build + GDI render PORTED** (ckpt 34/35,
  `glyph_text.c` + `glyph_render.c`). Remaining chips: the **escape expander**
  (`0x4034f0`/`0x4051d0`, hooked NULL — port when an escape-bearing string needs
  it), the **row-append `0x40f800`**, and the **sprite-cell render mode**
  (`0x48e200` `param_1==0`, ZDD blits). **Pixel-diff vs retail still pending**
  (Next move #1).
- **SFX `0x411390`** / joystick `0x5ba120/_290` / save-notify `0x41bb80` /
  watchdog `0x40a5d0` — the four `title_menu_input_step` side effects.
- **Audio ZDM** `FUN_005bab10`/`_5bc150` + SFX `FUN_00411390` — milestone 3.
- **Launcher `config.dat`** `FUN_005a4770` (46 KB) — milestone 4. We now know it
  loads sotesd/w/p.dll and stores their handles at DAT_008a6e74/78/7c.
- **Input producer** (DInput `GetDeviceState`, vtable `[0x24]`) + axis-held
  flags (quirk #41) — black box; `mem_watch.py` is the tool.
- God-object `DAT_008a9b50`/`DAT_008a6e80` layout (quirk #15) — model as we go.

## How to apply

When the user says "continue RE work" (or similar):
1. Read this file first, then `STATUS.md` + `ROADMAP.md`.
2. Pick the recommended next move (or whichever the user redirects to).
3. Port-and-test style: small unit → host test → commit. Each ported function
   gets a `FUN_XXXXXX` provenance comment; pin retail offsets via
   `_Static_assert` under `#if UINTPTR_MAX == 0xFFFFFFFFu`. Reference UNPORTED
   callees by bare VA, never `FUN_` (it inflates the ledger).
4. **Append any engine quirk** to `findings/engine-quirks.md` (now at #51).
5. **Regen** `gen_port_ledger.py` + `gen_frontier.py` after a port; check the
   headline didn't move unexpectedly.
6. **Verify rendering with `--capture-frames`** vs the goldens — self-serviceable.
7. Update THIS file each meaningful checkpoint; append to PROGRESS.md.
8. Suggest a `/clear` at the natural stop point.
