# OpenSummoners — porting roadmap (milestones + subsystem map)

> Hand-maintained. The mechanical "what's reachable from ported code" frontier
> lives in [`port-frontier.md`](port-frontier.md) (derived from the call
> graph); this file adds the *semantic* layer the call graph can't give: the
> user-facing milestone order, a map of which subsystem lives where in
> `sotes.exe`, and per-milestone port-readiness cards.
>
> The subsystem map + cards below were **seeded by the
> `opensummoners-subsystem-survey` workflow** (2026-05-29; 16 read-only Explore
> agents mapping address bands + 6 scouting the forward path — see
> `tools/workflows/subsystem-survey.js`). Findings are **decompile-grade**:
> strong leads, but verify exact offsets/strides/VAs at the byte level before a
> port depends on them (a few VAs in the survey were mis-transcribed). The raw
> structured result is archived at
> [`audit/subsystem-survey-2026-05-29.json`](audit/subsystem-survey-2026-05-29.json) —
> mine it rather than re-running. Quirks the survey found are in
> [`findings/engine-quirks.md`](findings/engine-quirks.md) #15–#27.

## Milestone order (user-facing)

Fortune Summoners is a 2D side-view action RPG. The port advances along the
path a player walks, because each milestone is a visually-verifiable checkpoint
the harness can frame-diff against retail (PLAN.md §5, Phases 4–5).

| #  | milestone | state | gated on |
|----|-----------|-------|----------|
| 0  | **Title screen renders** (first visible frame) | 🔲 **next** | `FUN_0056aea0` scene runner — all leaves ported. Card below. `hard` (size, not deps) |
| 1  | Title menu is **interactive** (highlight New Game/Options) | 🔲 | input system `FUN_0043c110`/`_43ce50` + DInput `FUN_005ba120`. Card below. `moderate` |
| 2  | Options menu navigates (gfx/sound/keys/difficulty) | 🔲 | config subsystem `FUN_00566a80` + menu controller `FUN_00412c10`. `moderate` |
| 3  | **Title BGM** plays | 🔲 | ZDM/WMF audio bring-up `FUN_005bab10`/`_5bc150`. Card below. `hard` (COM/WMF) |
| 4  | Real `config.dat` parse (replaces `--launcher-mode`) | 🔲 | `FUN_005a4770` 46 KB launcher init. Card below. `hard` (size) |
| 5  | **New Game → first scene loads + renders** | 🔲 | scene dispatcher `FUN_00546670` + map/tile render `0x590000` + sprite system `0x57ca40`. `hard` |
| 6  | Player **moves + collides** in a map | 🔲 | entity FSM `FUN_00442a70` + collision `0x54c640`/`0x483040` + camera `0x552c70`. `hard` |
| 7  | Dialogue / cutscene playback | 🔲 | narrative scene FSM (`FUN_004d13a0` prologue classroom). `hard` |
| 8  | Battle / combat loop | 🔲 | battle state machine `FUN_004710c0` (20 KB) + action dispatcher `FUN_00442a70`. `very-hard` |
| 9  | Save / load | 🔲 | save-slot init `FUN_00438a60` + serializer `FUN_0043c610` + path build `FUN_0041a890`. `moderate` |
| 10 | **Tutorial dungeon playthrough indistinguishable from retail** | 🔲 | everything above + the dungeon driver `0x5034b0`. Phase-5 goal. |

Milestones 0–4 are the title-path shell; 5–8 are the gameplay core (the bulk of
the eventual port); 9–10 prove fidelity end-to-end.

## Subsystem map — `sotes.exe` (engine-proper, below `0x5bdab0`)

Where each subsystem lives, by address band. Representative VAs are entry/driver
functions, not exhaustive. **The forward path is non-contiguous**: the title/menu
shell straddles `0x40****`–`0x41****` and `0x56****`–`0x59****`; the bootstrap +
renderer + pump sit at `0x5b****` (mostly already ported); and the *gameplay
simulation* (the bulk of the port) is the big block `0x42****`–`0x55****`.

| band | dominant subsystems | representative VAs |
|------|---------------------|--------------------|
| `0x401000` | **master object-pool ctor** (`FUN_004017d0`, all pools), the **main game-loop state machine**, sprite anim state, the **font/glyph system** + message formatter | `0x4017d0` (god-ctor), `0x40c380` (game loop), `0x4034f0` (font, 7 KB), `0x4051d0` (msg formatter) |
| `0x410000` | **menu/dialog controller** (`FUN_00412c10` allocator), character-stats init (`0x41f200`, 27 KB), shop/NPC dialog, item grid/list UI, **save-file path build** | `0x412c10` (menu alloc), `0x41f200` (char init), `0x41c390` (inn/healer), `0x41a890` (save path) |
| `0x420000` | scene/level init + entity spawning (data-driven setters), NPC/character setup, **entity-definition lookup by id** | `0x430400` (boss scene init), `0x42eb20` (spawn-by-id switch), `0x4282f0` (enemy def lookup), `0x426620` (sprite anchor) |
| `0x430000` | **battle scenario init** (`0x431e30`, 25 KB), battle turn engine (`0x439690`), grid/collision + pathfind, game-state/mode init, **menu/input** (`0x43c110` poll), save manager | `0x431e30` (scenario), `0x439690` (turn FSM), `0x43c110` (input poll), `0x438a60` (save slot), `0x439680` (frame pump) |
| `0x440000` | **entity per-frame FSM** (`0x442a70`, action-code dispatcher), entity arena alloc, sprite/anim, character action handlers, dialog/voice trigger, damage/status/items, pathfinding | `0x442a70` (entity FSM), `0x4412d0` (entity alloc), `0x446900` (dialog handler), `0x44a740` (skill dispatch) |
| `0x450000` | **master dialogue runner** (`0x461790`, 19 KB), entity-action dispatch, sprite-frame sequencing/culling, render-time dialogue sequencing, sprite-batch accumulation | `0x461790` (dialogue runner), `0x45a300` (action dispatch), `0x468b10` (cull), `0x46a880` (sfx accum) |
| `0x470000` | **master battle phase controller** (`0x4710c0`, 20 KB), NPC/monster AI decision tree, sprite/particle effects, wave coordination, UI text/popup-damage render, update-tick dispatcher | `0x4710c0` (battle ctl), `0x47b990` (NPC AI), `0x478ba0` (turn finalizer), `0x476d30` (damage numbers) |
| `0x480000` | character animation sequencer (`0x4895c0`, 8.6 KB), AABB hit-test + knockback, **GDI glyph blit** (`0x48e200`), entity movement physics, **SFX trigger** (`0x489280`), combat-effect anim | `0x4895c0` (char anim), `0x480260` (hit test), `0x48e200` (glyph blit), `0x489280` (sfx) |
| `0x490000` | **tile/sprite grid renderer** (`0x490f30`), spell/effect render, char-anim player, battle-status UI, shop/dialogue menu, anim-table lookups, palette/color-mode | `0x490f30` (tile render), `0x493ba0` (spell fx), `0x494e60` (battle UI), `0x4917b0` (frame lookup) |
| `0x4a0000`–`0x4cffff` | **scene-event dispatch** (narrative scripting; huge 20–28 KB per-screen handlers keyed on scene id), party/inventory state, inn/shop/puzzle scenes, object-spawn allocator | `0x4b4d40` (28 KB scene), `0x4c6b40` (battle-loop scene), `0x4c2dc0` (inn/shop), `0x4cc250` (party swap) |
| `0x4d0000`–`0x53ffff` | **narrative scene FSM** (prologue/tutorial/guild/dungeon scripted events), character status flags, **dungeon/encounter driver**, quest/item persistence (sparse band) | `0x4d13a0` (classroom prologue), `0x4d7d80` (guild), `0x4dc510` (home dungeon), `0x5034b0` (dungeon floor) |
| `0x540000` | **primary cutscene/scene dispatcher** (`0x546670`, 22 KB, 100+ scene ids), main **sprite copy/animate** (`0x54f980`, 11.6 KB), **tile-map collision** (`0x54c640`), camera follow/clamp, enemy spawner, battle animator, level-up UI | `0x546670` (scene driver), `0x54f980` (sprite copy), `0x54c640` (collision), `0x552c70` (camera) |
| `0x560000` | **title scene runner** (`0x56aea0`) + **gameplay scene runner** (`0x56cd20`), options/settings menus, sprite/wave loaders, **engine init** (`0x562ea0` ZDD/ZDS/ZDM), input-device init/config | `0x56aea0` (title), `0x56cd20` (gameplay), `0x562ea0` (engine init), `0x564780` (options), `0x56bfd0` (input init) |
| `0x570000`–`0x58ffff` | **master sprite-group register** (`0x57ca40`, 24 KB), scene refresh, **audio/music init** (`0x5752e0` 17 KB, sotesw.dll load), SFX-slot batch, **animation frame pump** (`0x58f360`, ~40 FPS), GDI font slots | `0x57ca40` (sprite reg), `0x5752e0` (audio init), `0x58f360` (anim pump), `0x57a030` (GDI font) |
| `0x590000` | inventory/item menus, **audio cue/wave manager**, party status/skill menus, **primary render dispatch** (`0x5a00c0` 13.6 KB) + **scene render pump** (`0x5a4770`-adjacent), scene transition/load | `0x593f60` (item menu), `0x590230` (wave mgr), `0x591120` (title main-menu loop), `0x59ec30` (scene load) |
| `0x5a0000`–`0x5bdaaf` | **launcher config parser** (`0x5a4770`, 46 KB, config.dat XOR), ability/spell effect system, sprite/particle lifecycle, **bitmap/palette session**, **ZDD DirectDraw wrapper**, string/mem/RNG (`0x5bf505` LCG) — **much already ported** | `0x5a4770` (config), `0x5b28a0` (spell fx), `0x5b8480` (ZDD present), `0x5bf505` (RNG) |

> `0x5bdab0`+ is the statically-linked MSVC CRT (operator_new, _malloc,
> _strcmp, the `entry` startup, RtlUnwind, …) — linked by the drop-in, not
> ported (see `STATUS.md`).

## Port-readiness cards (from the forward-path scouts)

### Milestone 0 — title scene runner `FUN_0056aea0` (`hard`, 3441 B)

The first visible frame: studio/title fade-ins, "press button" prompt, then the
top-level menu. A 7-phase state machine with frame-budget sync. **All its leaf
dependencies are ported** — the difficulty is its size + the
**`PTR_DAT_0056bfa4` indirect jump table** (Ghidra gave up; radare2 recovers 7
handler VAs covering 11 phase indices 0–10 — recover by hand). Fade math uses
unsigned 0..1000 ramps with per-phase clamps (quirk #—); menu slot order is
hardcoded (#27). Still-unported callees it touches: `0x41bbe0`, `0x43e140`,
`0x40a5d0`, `0x40fe00`, `0x41f3e0`, `0x414080`, `0x411f40` (most are small
leaves, already on the frontier). See `findings/title-scene.md`.

### Milestone 1 — input system `FUN_0043c110` + `FUN_0043ce50` (`moderate`)

Keyboard + gamepad polling and action latching. `FUN_0043c110` scans a
consume-on-read ring buffer at `+0x108` (3-dword slots, head at `+0x0c`);
`FUN_0043ce50` latches confirm/cancel/nav into device-independent action codes
(keyboard → `FUN_0043ca40`, gamepad branch reads device fields). **Open black
box:** the VA that calls `IDirectInputDevice::GetDeviceState` (vtable `[0x24]`)
to *fill* the ring buffer isn't visible in the decompile — Frida-hook
`GetDeviceState` to find the caller; this is load-bearing for input at all.
DInput pads attach lazily on first confirm (#24). Deps: `0x43c650`, `0x43c920`,
`0x5ba070`, `0x5ba540`.

### Milestone 3 — audio bring-up `FUN_005bab10` / `FUN_005bc150` (`hard`)

ZDS (DirectSound, SFX) is largely reached by boot already; **ZDM** (music) is
new. BGM path: extract WMA from `sotesw.dll` → temp file → DirectShow/WMF graph
via `CoCreateInstance(IGraphBuilder)` → `RenderFile` (#25). GUIDs live in the
data section (`DAT_00850f08/28/58`) and must be recovered. 50-voice polyphony
(#25). "Disable Sound" gates ZDM only (#26) — harness can test SFX without
music. Smallest milestone: play one title BGM track. Deps: `0x5bbb50` (graph
pump), `0x5bc0c0`, `0x5bbdb0` (vol/pan), `0x5bad80` (COM release).

### Milestone 4 — launcher settings `FUN_005a4770` (`hard`, 46 KB)

Catch-all init: launcher dialog, `config.dat` round-trip, window-class
registration, main-window creation. `config.dat` is an 840-byte file —
16-byte plaintext header `[size=16][ver=0x2711][data=820][checksum]` then 820
bytes XOR'd with single key **`0x88`** (#— / engine-quirks #8). Boot registers
**101 schema fields** into a 2828-byte struct via repeated `FUN_005afb90`
calls, then `FUN_005afc90` unpacks. Radio enums use 3/4/5 not 0/1/2 (#6). First
plaintext dword `0xB534D9BC` is an unexplained hash/salt — preserve on
round-trip. Recover the field schema via radare2 on `FUN_005a4770` or a Frida
hook on the register path. See `findings/launcher-dialog.md`,
`formats/config-dat.md`.

### Milestone 5 (support) — wave-load / sprite+sound pools `FUN_00563ef0` (`hard`)

First half (sound-slot field init) is ported as `ar_sound_slot_init`; second
half (wave decode + DirectSound buffer fill) is unported and needs DSound
vtable dispatch (`CreateSoundBuffer`/`DuplicateSoundBuffer`). Feeds the sprite +
sound pools `DAT_008a760c` / `DAT_008a92b4`. Assets are **uncompressed**, just
chunked at 676996-byte PE-resource boundaries; `FUN_005b67c0` spans chunks
transparently. Loads are **lazy** — buffers are NULL until first play. Deps:
`0x5bb250`, `0x5bb2f0`, `0x5bb3d0`, `0x5bb5c0`, `0x5b6340` (polymorphic asset
reader). See `findings/asset-loader.md`.

## Next-chip triage

The **semantic** next move is **milestone 0** (title scene runner) — it's the
first new visible frame and all its leaves are ported. After that, milestone 1
(input) makes the menu respond, which unblocks every interactive screen.

The **mechanical** next chips (unported functions already called by ported
code, zero-dependency leaves) are in [`port-frontier.md`](port-frontier.md):
114 frontier functions, 52 portable-today leaves. Several of the title runner's
own callees (`0x412c10` menu alloc, `0x43c110` input poll, `0x414080`,
`0x411f40`) already show up there — porting the runner will naturally pull them
in.
