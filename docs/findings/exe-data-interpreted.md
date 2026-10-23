# Exe / DLL data that is INTERPRETED — survey (2026-10-23, enold)

Question: which bytes the port reads from the retail image (`.rdata` 0x5cc000.., `.data` 0x854000.., `.rsrc`
0x8ac000..) or the asset DLLs (`sotesd/sotesw/sotesp.dll`, resource-only) get dispatched on as a
program / command stream (bytecode, command lists, opcode tables, text markup, data-driven SMs) vs plain data.
Context: standalone exe copies those sections (NOT `.text` 0x401000..0x5cb795); user req = RE + document any
proprietary script format.

**TL;DR — no bytecode / script VM exists in the exe or DLL data.** All story / event / cutscene / AI / skill
logic is compiled x86 (ported as `src/chapter_scripts.c` 39k lines + `src/story_scripts.c` 55k lines + the
gameplay modules). What IS interpreted: (1) a small **text-markup language** inside strings; (2) **declarative
data** (maps, anim clips, a composite-sprite frame table, flag-gated row tables) whose ids/codes select native
handlers (data-keyed dispatch, no program counter). Plus one side hazard: the port reads a compiler switch table
in `.text` (§5).

## 1. Method (reproducible)
- PE resource census of exe + 3 DLLs (parse resource dir; classify every blob by magic / decoder signature).
- Resource readers: the only `FindResourceA` sites in all_ida.c = `0x5b62a0` (DATA stream), `0x5b7800` (typed
  DIB), `0x5bb740` (WAVE), `0x5bc150` (BGM). Callers of `0x5b62a0`: `0x42b5f0` (res 1986), `0x579740` (sigs),
  `0x587970` (maps), `0x5a4770` (sig 0x7de). Callers of `0x5b7800`: `0x4178e0`, `0x4184a0` (sprites).
- Interpreter-shape scan over all 1,824 IDA functions: `switch` + `*p++`/`*++p` deref in one fn → only CRT
  (`__output` 0x5c1d41, `__input` 0x5c503a, `___strgtold12` 0x5c95a6, `__store_winword` 0x5bfdfd) + 9 game fns
  whose `p++` is an inlined strcpy/strcmp/memset (0x4593e0 0x4a6b80 0x565d10 0x567ba0 0x56aea0 0x56cd20
  0x59afd0 0x59f2c0 0x5a00c0). Switches on static-table entries: only `0x584710` (start-party code @0x854080)
  + `byte_49C150` (compiler index). ⇒ no byte-stream dispatcher.
- Code-pointer scan of `.rdata`/`.data` (dword ∈ .text): only static-init thunks 0x854004 (15), CRT tables
  0x854048 / 0x8a4f78 / 0x8a51b0; 0x5cc018.. & 0x852890.. are IAT/INT RVAs (false +). `vtables_ida.tsv` empty
  (/GR-). ⇒ no data-resident dispatch tables of game code.
- String census (.rdata 828 / .data 6,094 text runs): control content = `%n` (164 / 1,161), printf `%s %d %0Nd
  %%`, `@@` key tokens, SJIS glyphs (↑→↓← ♪ …). No other escape bytes (no `\x01`-style codes, no `\n` in game text).

## 2. Resource census (every blob classified)
| module | type | n | format | consumer |
|---|---|---|---|---|
| sotes.exe | DATA 0x3fe..0x5aa | 376 | MPED2DT map (§3 row M) | `0x587970` map parse |
| sotes.exe | DATA 0x570..0x577, 0x57c, 0x5ab | 10 | scrambled DIB 8bpp (party body sheets, #92) | `0x5b7800` |
| sotes.exe | DATA 0x4e1 | 1 | cp932 text: JP web-activation notice + EULA (3,062 B) | none found (1249 refs = face id / sprite field) |
| sotes.exe | ICON/GROUP_ICON/DIALOG 0x2711,0x2775/VERSION | 7 | Win32 std | launcher `0x4013c0` |
| sotesd.dll | DATA 0x3e8..0x91d | 756 | scrambled DIB (493×24bpp, 263×8bpp) | `0x5b7800` |
| sotesd.dll | DATA 0x7c2 (1986) | 1 | composite-sprite frame table (§3 row P) | `0x42b5f0` |
| sotesd.dll | DATA 0x49c, 0x7de | 2 | signature: u32 0x2712 / 0x2713 + 60 B | `0x579740`, `0x5a4770` |
| sotesd.dll | WAVE 0x4a3..0x907 | 436 | RIFF WAVE | `0x5bb740` |
| sotesw.dll | DATA 0x3e8..0x417 | 46 | ASF/WMA BGM | `0x5bc150` (temp file `1%07d.wma` → DirectShow, #25) |
| sotesw.dll | DATA 0x40f | 1 | signature: u32 0x2712 + 8 B | `0x579740` via `0x5752e0` |
| sotesp.dll | WAVE 0x3e8..0x406 | 31 | RIFF WAVE | `0x5bb740` |
| sotesp.dll | DATA 0x407 | 1 | signature: u32 0x2711 + 8 B | `0x579740` |

No unclassified blob. No resource holds a script. (Outside the image, not in scope: `config.dat` XOR #8,
`sotes.ini`, `savedataNN.sdt` [sdt-save-format.md], `lizsoft.spl` = plain BMP splash.)

## 3. Candidates
Kinds: **markup** (in-string control codes) · **TSM** (table-driven: data keys/conditions select native code;
no instruction stream) · **decl** (declarative data, interpreted only as parameters) · **plain**.

| # | data | consumer (VA → port fn, file) | kind | format notes | documented? |
|---|---|---|---|---|---|
| T1 | story / UI / description strings: `.data` (e.g. 0x86d58c town line 1), `.rdata` skill/item descs (0x682784..) | `0x40f040` text_parse (`src/ui_text.c:132`), `0x40e5e0` text_wrap (`:191`), `0x413ed0` text_layout_set, `0x40e360` ctrl_text_set; typewriter `0x43c650` ctrl_tick (`src/ui_run.c:193`) | markup | `%n` forced break · `%mN%` (≤10 digits) = next glyph's value (key-icon id) · `%w` = break class 5 · any other `%x` dropped (2 B) · byte ≥0x80 = SJIS lead (2-byte glyph); per-glyph break class (blank 4, `,` 2, `!.;?` 3, SJIS ・1 、2 。！？3, ）」』 →4) drives wrap + typewriter delay (×2/3/5 base; voiced 6/18/24/36) | yes — engine-quirks #148, #150, #154 |
| T2 | key tokens `@@ZK @@XK @@CK @@↑K.. @@ENK @@F1K..@@F5K @@AB @@SB @@+M @@-M @@Z @@X @@C @@EN @@F1..F5 ↑ → ←` (33; strings in .data, table BUILT IN CODE) | `0x410610` sys_msg_keys (`src/ui_text.c:427`) → `0x411230` ctrl_key_add (`:397`); expand `0x43cf90` ctrl_keys_expand (`:419`), `0x412800` key_token_replace | markup (macro) | plain byte search (no SJIS stepping); hit → `%m<10000+i>%` + blanks via fmt `%%m%d%%%s` (0x85485c); id → key-icon sprite sys+0xbbc frame/dx/dy | yes — #148 |
| T3 | `#`-prefixed word; furigana table 0x5cd978..0x6286c4 (2,267 × 164 B: word, readings +0x10+9k, context id +0xa0) | `0x4031c0` glyph_recs_set (`src/ui_text.c:92`), `0x40fa00` grid_cell_text, search `0x40fd20`, context test `0x4034f0` sj_4034f0 (`src/sjis_text.c:94`), writer `0x4051d0` | markup + TSM | `#` blanks a word's first 2 glyph records; auto-ruby on every record set; context id 1..12 = `switch` selecting neighbour-string lists built in code (prefix/suffix compare) | partly — #148 (ids 1..12 semantics not; JP-only, EN never matches) |
| T4 | printf / strftime formats in `.data` (e.g. 0x8a0aa8 `You used %s.%%nThe %s has broken.`, 0x8a0b70 `%Y/%m/%d %H:%M`) | `0x5bf3ee` sprintf → CRT `__output` 0x5c1d41 (port: `g_eng.h5bf3ee` → retail's CRT under rhost, `crt_sprintf` src/msvc_crt.c in the standalone) | CRT format lang (standard) | game escapes its `%n` as `%%n` in all 5 mixed formats (0x85c618 0x85c76c 0x85e14c 0x8a0aa8 0x8a4658) ⇒ CRT `%n` never fires; only `%s %d %0Nd %%` + strftime used | no (this doc) |
| T5 | Messages / quest table 0x6b7ee8 (29 × 1212 B, 0-term): +0 done flag, +4 order, +8 gate flag, +0xc gate min, +0x10 author handle, +0x14 title, +0x94 body | gate: `0x58d3b0` quest_state (`src/room_load.c:258`), `0x49ce70` quest_count (`src/room_tick.c:366`); view `0x597aa0` fm_597aa0 / `0x5985f0` fm_5985f0 (`src/field_menu.c:718`, `:1214`), clip `0x5984c0` | TSM + markup | row open ⇔ flag[+8] ≥ [+0xc] ∧ flag[+0] unset (areas 1010/1020 only); body: **single `@` = page break** (split into 512-B pages, `src/field_menu.c:1439`), `%n` inside | gate yes (#138, #147, #173); **`@` page break NOT documented** |
| T6 | fetch-quest table 0x6e7a38 (20 B rows {wanted item id, ?, quest flag, min flag-100100101, ?}, 0-term) | `0x49e6c0` sc_49e6c0 (`src/script_helpers.c:1436`) | TSM | first row with flag ∈ {0,10} ∧ chapter ≥ min → item row of 0x6c0cd0 → native dialogue; sets flag 10 | no |
| M | MPED2DT maps: exe DATA ×376 | parse `0x587970` (`src/map_data.c:63`, retail-layout `src/room_load.c:734`); cells → `0x587e00` map_decode (`src/map_decode.c:913`); objects → `0x58d460` room_populate (`src/room_load.c:369`) | decl + TSM | hdr: u32 0x30 · `MPED2DT\0`+8 sp · u32 ver 0x2712 · record sizes {0x34,0x1c,0x3c,4,0xc,0x100,8} (self-describing) · name `MSD_SOTES_MAPDATA` · dims · cells 0x1c B (tile id +4 → per-id NATIVE emit recipe) · objects: 0x3c hdr (code +0x10, x/y, u16 variant +0x18) + A (4-B params) / B (12-B points) / C (0x100 B: **0 in all 376 maps**) / D (8 B). 12,351 objects: 5xxxx 1,668 (char → `0x41f200`), 6xxxx 3,553 (structure `0x438a60`), 7xxxx 5,081 (device `0x431e30`), 8xxxx 0 (effect `0x557550`), 9xxxx 2,049 (90010/90011 link anchors → `0x58cb30`) | yes — docs/proofs/map-data-format.md, map-object-layer-format.md (its band labels — and #119's — are stale: per `0x58d460`'s own overflow strings 0x8a2e64/48/2c/0c, 5xxxx = Character, 6xxxx = Structure, 7xxxx = Device, 8xxxx = Effect, as `src/room_load.c:369` has it), #79 #84 #119–#121 #124 #131 #138; header record-size block + A/B/D field map not consolidated |
| W | map-defined trigger wiring: object A-params id1/id2 (<1000 → +1000×map), switch codes 70100..70850, link handles; actor event ids (map record +0x1d8) | `0x59b170` gimmick_step, `0x40afe0` switch_set, `0x40be30` switch_link (`src/gimmick.c:381`, recursive); `0x40b8f0` event_run mode 3 → `0x40c380` script_engine (`src/script_engine.c:593`) | TSM | data declares WHICH actors are linked / which id fires; every effect + every script branch is native `switch` on code / record id / area (world+0x105c) / flag 100100101 range | yes — #137, #144 |
| C | anim clips `.rdata` ~0x6290e0..0x676748 (~700 × 0x194 B; arrays stride 0x194, lone clips in 0x198 slots; IDA 66/338 = label split at +0x42) | stepper `0x54f980` (`src/anim_clip.c`), renderer `0x491ae0`; step tags `0x485fc0` @0x486cf1 (`src/char_apply.c:287-299`) | decl + per-frame event tags | +0 base sprite, +2 i16 delta[32], +0x42 count, +0x44 dur, +0x48 oneshot, +0x4c abs, +0x50/+0xd0 off_x/off_y[32], +0x150 link, +0x152 loop_to, **+0x154 u16 step_tag[32]**: on frame entry (timer 0) tag k≠0 → footstep(kind k) by ground surface; 11 → kind 1 + a 2nd step 5 ticks later. 14 clips tagged, values {1,2} | partly — anim_clip.h (asserts sizeof 0x154: truncated; +0x154 track + 0x194 size undocumented) |
| P | composite-sprite frame table: sotesd DATA 1986 (366,120 B) | load `0x42b5f0` ci_42b5f0 (`src/char_types.c:1229`) from `0x42a9a0` (codes 100179/100180 = 0x18753/0x18754); read `0x44d020` entity_part_descs (`src/char_status.c:98`), `0x4505c0` entity_sprite_descs (char_attack.c), `0x445e50`/`0x455c90` (part by id) | decl (keyframes) | u32 8 · u32 rec 52 · u32 hdr 28 · hdr i32[7] {22 parts/frame (+0x1c), 160 left-facing offset (+0x18), 20, 8, 320 frames, 480, 384} · 7,040 cells × 52 B {i32 part key, x, y, sx 1000, sy 1000, ?, part id 1001..1005 → bank 304..308 (map via `0x42b550`), frame, ?angle, ?, ?, dx, dy}; frame idx = clip base+delta (+160 facing left); part id 0 = hidden | no |
| S | sprites: sotesd DATA ×756, exe DATA ×10 | `0x5b7800` bs_5b7800 (`src/frame_load.c:1195`) + `0x5b7c10` bs_5b7c10 (`:1400`) | plain (codec) | sig u32@0x438 − u32@0x448 == 10001; key-rebased DIB header + 1 KB palette; bottom-up | yes — docs/formats/lizsoft-sprite.md, sprite-pipeline.md |
| A | WAVE (sotesd 436, sotesp 31); WMA (sotesw 46) | `0x5bb740` (`src/sound_dev.c:998`); `0x5bc150` sd_5bc150 (`:1577`) | plain | RIFF / ASF | yes — asset-loader.md, #25, #127 |
| G | signature blobs sotesd 0x49c / 0x7de, sotesp 0x407, sotesw 0x40f | `0x579740` al_579740; `0x5a4770` (`src/win_main.c:2804`, `:2857`) | plain (integrity) | u32 ≥ 0x2711..0x2713, then byte+65 == `.data` key (0x8a423c 60 B, 0x8a42a4 60 B, `Fspatchrk`..) | yes — #13 |
| D | param tables `.rdata`/`.data`: skills 0x6826c0 (74 × 768 B; desc @+0xc4 w/ T1/T2 markup), skill-by-code 0x6907c0, base stats 0x67ac58 (220 B), status 0x628708 (18 × 140 B), voice groups 0x690ca0 / lines 0x691018, voice rows 0x6b6568, dramatist 0x6b6ea8, items 0x6c0cd0 (736 B; desc markup), shop stock id lists (e.g. 0x6e7f24, 0-term), AREA 0x693848 / ROOM 0x6940c8, level rows 0x690b40, room-variant rows 0x693808, palette remaps 0x672440..0x676748 (520 B), slope profiles 0x5cc410/0x5cc430, shape polylines 0x5cc450 (12 × 33 dw), start party 0x854080 (24 B) | char_def.c, base_stat_table, char_status.c, sound_fx, scene_cmd, script_helpers, game_world.c, map_decode, render_draw.c, title_extra | plain (keys + params for native `switch`) | yes — char_def.h, #91 #145 #151, game_world.h, docs/proofs/dramatist-table.md |

**Not candidates (runtime command records, built by native code, never loaded from the image):** scene command
block runner+0x1c..+0xb28 run by `0x439690` scene_run (#154); cast command char+0x15b64 (#153); char command
block +0x14854 (char_ai); window / overlay / dissolve state (#155); ending credits (`en_*`, strings passed by code);
title intro phases; particle (`0x557550`), shot, AI, skill-stage behaviour = per-code `switch` in code; attract
demo = AI ("The Game Play by Computer operator only.") on in-memory bank kind 10030 (`0x41b670`), no recorded input.

## 4. Native "scripts" — confirmation
`0x40c380` script_engine: by area world+0x105c (3 → `0x4e1d10`; 1020 → `0x4e23d0`; 1010 → 8 fns while flag
100100101 ∈ 0..500, 28 fns while 1000..2900) then 28 fns for every area (`0x49d930`..`0x4c45a0`) — each a
compiled C++ coroutine (#20) calling the scene builders (`src/scene_cmd.h`) with immediate args + string VAs.
No table of steps, no opcode stream. #109 "flat beat sequence" = native call sequence, not data.

## 5. Side finding — `.text`-resident data the port reads (standalone hazard) — RESOLVED 2026-10-23
Status: `0x49c150` decoded into the switch (`src/ending.c`); every by-address callback below has a hook row → the
standalone's entry thunk (docs/standalone.md; `prove_no_retail_code.py` source checks enforce both).
- `0x49c150`: 25-B MSVC switch byte-index table, read as data by `0x49bf90` en_49bf90 (`src/ending.c:264`).
  `.text` is not copied ⇒ wrong dispatch in the standalone. Fix: inline as a C `switch` / local table.
- Code VAs taken as callbacks via `D(va)`: `0x401210` (`src/win_main.c:430`, WndProc), `0x5b12e0`
  (`:2706`, WndProc), `0x4013c0` (`src/win_app.c:919`, DlgProc), `0x4017a0` (`src/engine_rest.c:4430`,
  `src/title_extra.c:548`, vec-dtor), `0x5851a0`/`0x54c5e0` (`src/title_extra.c:3388-3389` …), `0x5b10d0`
  (`src/win_main.c:3007`, atexit), `0x5ba200` (`src/input_dev.c:286`, DInput enum cb) ⇒ must map to port fns.
- `.data` static-init table 0x854004 + CRT tables hold `.text` pointers: never call through them standalone.

## 6. Verdict
- **No proprietary script / bytecode format** in sotes.exe or the asset DLLs. Story, events, cutscenes, AI,
  skills, particles, ending = native code (already ported as C). No resource is a script; no data-resident
  dispatch table of game code; no byte-stream dispatcher outside the CRT.
- **Interpreted content = the text-markup mini-language** (T1–T5: `%n %mN% %w`, `@@` tokens, `#` ruby, SJIS
  break classes, Messages `@` page break, `%%n` format convention) + **data-keyed dispatch** over declarative
  formats (M, W, C, P, T5, T6).
- Need full RE + a format doc:
  1. **Text markup spec** (`docs/formats/text-markup.md`): consolidate #148/#150/#154 + `@` page break (T5,
     undocumented) + `%%n` convention (T4) + furigana context ids 1..12 (T3: RE `0x4034f0` neighbour lists).
  2. **MPED2DT map format** (`docs/formats/mped2dt-map.md`): merge the two proofs; add header version/record-size
     block, object A/B/D field map (from room_populate / char_init / device_init args), C unused, engine kind
     names (fix the stale band labels in the proof + #119), region-E links.
  3. **Anim clip record** (0x194 B incl. +0x154 step-tag track) — format doc; anim_clip.h note.
  4. **Composite-sprite frame table** (sotesd 1986; codes 100179/100180) — undocumented; finish 52-B cell
     field RE via `0x44d020`/`0x4505c0`, then format doc.
  5. Not a format but blocking for standalone: §5 `.text` reads.
