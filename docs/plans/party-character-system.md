# Plan — Port the party-character system (render the arriving party + the arrival cutscene)

**Status:** approved 2026-06-08 (ckpt 91). Scope chosen by USER: *render + arrival cutscene*
(controllable Arche = a scoped future phase). Multi-session.

> **UPDATE (ckpt 92) — Phase-1 RE substantially DONE; the character→sprite map is the
> "dramatist table" `DAT_006b6ea8`.** Proof: `docs/proofs/dramatist-table.md`; quirk #91.
> Character identity is a 32-bit HANDLE → `DAT_006b6ea8` (`{handle, code, name, bank}`);
> `0x41f200:54-69` maps handle→archetype **code** (`+0x1d4`) + sheet **bank** (`sVar17` →
> `0x426d70`). The arrival family is NAMED: handle `0x5f5e1d3`→`0xc3dc` bank `0xe3` **Father**
> (renders), `0x5f5e1d4`→`0xc440` bank `0xb5` **Mother**, `0xc3f0` bank `0xeb` **Dr. Barnard**
> (renders), and the LEADER `0x5f5e165`→`0xc35a` **Arche** (banks `0x8b`–`0x8e`, clip
> `0x62a8c8`). **Corrected gap:** only Arche culls + Mom's `0xb5` sheet is mis-rendered (port
> spawns the map `0xc440` bank `0xa6` default townswoman); Dad already renders. So Phase-1's
> "RE the character→resource→bank map" (below) is answered — the remaining Phase-1 work is the
> PORT: the dramatist resolve + the archetype cases + Arche's sprite registration. The
> persistent-party-band (`0x4997b0`) framing below still holds for the *controllable* leader,
> but the *arrival* cast (incl. Arche standing at center) is the EFFECT-band dramatist path.

## Context

The ckpt-90 "PARTY render path" scope was built on a wrong `bank == idx` assumption.
ckpt 91/91b corrected it: runtime `bank = registration_idx + 13` (proven). **There is NO
decode bug** (ckpt 91 briefly claimed one — a cross-run-flip artifact, retracted in 91b):
res `0x477` is the **MAN right of the horses, which the port renders CORRECTLY**, and it's
the only sprite source (sotesd.dll; sotesp.dll lacks it; the EXE's res 1143 is map data).
The real situation (USER-confirmed, `findings/in-game-intro.md` "DEFINITIVE (ckpt 91b)"):
**the woman (Arche's mom) + Arche (the protagonist girl) are party characters fed by an
entirely unported subsystem** — a handle/"dramatist" registry + the party band (`0x4997b0`)
+ per-character sprite loading. They stand at center where `0xc35a` (bank `0x8b` → idx 126,
**UNREGISTERED → CULLS**) sits; the port never loads their sheets, so they're absent. The
port also fakes the arriving cast with `CUTSCENE_CAST_DEFS` (a flip-2400 mid-walk snapshot
of EFFECT-band actors), positions are frozen, and there is no dialogue.

**Confirmed settled cast** (census `runs/cutscene-cast`; bank → idx−13 → res): `0xc3e6`
`0xe5`→`0x475`, `0xc440` `0xb5`→`0x467`, **`0xc35a` `0x8b`→idx 126 (CULLS) = Arche + the
woman (mom), center**, `0xc3dc` `0xe3`→`0x473`, **`0xc3f0` `0xeb`→`0x477` = the MAN, far
right (renders OK)** — all townsmen except the `0xc35a` party pair.

**Goal:** render the woman + girl **1:1** and play the **arrival cutscene** (walk-in +
dialogue + portrait/textbox). The fully *controllable* Arche (Phase C — in-game
movement/physics FSM) is a scoped **future** phase here (entry points only; its FSM needs
its own RE pass). No MVPs — port the real system, retire the `CUTSCENE_CAST_DEFS` snapshot
+ PORT-DEBT(cutscene-party-chars).

## The system (RE map, with VAs)

- **Character identity = a handle/dramatist registry.** `DAT_008a9b50+0x2790` is a
  dynamic array `{cap@+0, ptr@+4, count@+8}` of actor pointers, keyed by the actor's
  handle `+0x1d8`. Add/remove: `0x555f00` (also the sibling registries `+0x2788`/`+0x278c`
  via `0x555fc0`). Resolve handle→actor: **`0x556eb0`**. The town-intro cutscene spawns
  party members by **handle** (`0x5f5e1d3`/`0x5f5e1d4`) — these resolve only if the actors
  were created+registered earlier (story/new-game setup), which the port never does.
- **The party band.** `room_state+0x4030` = 8 slots (`operator_new(0xeec)` each, allocated
  +reset in `game_enter 0x59f2c0:96-103,164-170` via `0x560e60`) + the **leader** at
  `room_state+0x200c`. Renderer **`0x4997b0`**: walks slots `i=7..0` (gate
  `+0x9c4==1 && +0x9f4!=0`, skip the leader), then the leader (param_2=1). The actor's
  render-state is **indirect**: `slot+0x9f4`→entity, `entity+0x40`→render-state. Called
  from the town render driver **`0x48c150:101-102`** *after* the EFFECT/STRUCTURE/CHARACTER
  bands (party draws on top), once per room instance (stride 0x4120).
- **The renderer body `0x493ba0`** (3 KB) is the same multi-part char renderer the port
  already reuses as `actor_render_static` (`src/actor_render.c`) — but only its static arm.
  A party/protagonist needs the **unported arms**: the shadow blit `0x4917b0`, the
  color-remap (`DAT_008a9358`), and the effect-list / multi-part walk (render-state
  `+0x562a`/`+0x562c`).
- **Per-character sprite loading.** `ar_sprite_slot_register` (`0x5748c0`, **already
  ported**, marks the slot for lazy decode of a resource id) is the bank-overwrite. The
  per-character sprite-state (frame bases per pose) comes via the dialogue/setup path
  `0x49d6e0` + table `DAT_006b6568` (`{handle, frameA, frameB, frameC}`). None of this is
  called for party members in the port → their banks keep the group3 placeholders.
- **The cutscene `0x4d7d80`** (case `0x334be`, gated on flags `0x5f76805`/`0x606aa4f`):
  spawns the wagon (`0x431d10`, ported) + 3 party actors (`0x41f0e0` → EFFECT band
  `0x41f200`), then runs a **walk-in movement script** (actor FSM `0x402730`/`0x402330`)
  and **dialogue** (`0x49d6e0`) with portraits, drawing the **portrait/textbox via the
  `0x5a00c0` overlay** (unported, PORT-DEBT `ingame-nontile-layers`).

## Plan (phased)

### Phase 0 — Ground-truth the cast — DONE (ckpt 91b, USER-confirmed)
Resolved: res `0x477` = the MAN (renders OK, no decode bug); `0xc35a` (bank `0x8b`→idx 126,
CULLS) = Arche + the woman (mom) at center; the party/character sprite loading is the gap.
See the cast table above + `findings/in-game-intro.md` "DEFINITIVE (ckpt 91b)".  One detail
deferred to Phase 1's first step: the exact `0xc35a`-vs-Arche split + each one's true sprite
source (bank/resource/**module**), via a clean single-run capture hooking the party spawn +
`0x556eb0`.  *(Original Phase-0 framing below, for reference.)*
The one real ambiguity was: which visible character is which actor. Positions suggest the
woman+girl are the **handle** actors (`0x5f5e1d3`/`0x5f5e1d4`, anchor-relative x `8000`/
`-3200`, landing center) and `0xc3f0` (x `+0x6400`→screen ~514) is the separate far-right
man. Confirm with one seed-pinned `--lockstep` capture at the settled town that annotates,
per visible keyed/party blit: actor **code**, **handle `+0x1d8`**, **band** (EFFECT
`+0x1160` vs party `+0x4030` vs dramatist), and **bank/res**. Also dump retail's decoded
**bank `0xeb`/slot 222** to confirm whether the woman's sheet overwrites res `0x477` or she
uses a different bank. Deliverable: a definitive cast table (character → actor → band →
bank → resource → settled pos), which fixes the exact work in Phase 1/2. (Extend
`tools/flow/retail_fields.json` with party-band + `+0x1d8`/`+0x9f4`-chain entries rather
than a bespoke probe.)

### Phase 1 — The dramatist registry + per-character creation & sprite loading
New module **`src/party.{c,h}`**.
- Port the handle registry: the `+0x2790` dynamic array + `0x555f00` (add/remove) +
  **`0x556eb0`** (resolve). Model the registry on a small growable array of actor refs.
- Port the **party-member creation** at new-game/scene-load: RE where the story party
  (the woman, Arche) are instantiated with their handles (`+0x1d8`) and registered. RE the
  **character→sprite-resource→bank** mapping and call `ar_sprite_slot_register` per member
  so their **real sheets** load into their banks (replacing the group3 placeholders). This
  is the chip that makes the woman render as the woman.
- Wire into `enter_game` (`src/main.c`) alongside the existing `0x560e60` party-slot reset.
- **EXE-embedded sheets (USER directive):** if a party/character sheet lives in `sotes.exe`'s
  own `.rsrc` (retail loads some banks via `FindResourceA(NULL,…)` — e.g. `0x570`-`0x572`,
  per the `init_sprite_banks` note in `main.c`), the port must **NOT embed the asset**. Load
  it from the user's own files at runtime: `LoadLibraryEx("<gamedir>/sotes.exe",
  LOAD_LIBRARY_AS_DATAFILE)` + `FindResourceA`/`bs_decode_resource` (the `.rsrc` survives the
  Steam `.bind` DRM, which packs `.text` not resources), **or** extract once and cache under
  `%APPDATA%/OpenSummoners/`. Keeps the legal line. Most sheets are in `sotesd.dll` already;
  this applies only to the EXE-`NULL`-module banks.

### Phase 2 — The party band + the rich renderer arms
- Port **`0x4997b0`** (the 8-slot + leader walk, the `+0x9f4`→`+0x40` indirect
  render-state) into `src/party.c`; wire it into `game_actor_walk` (`src/main.c`) **after**
  the EFFECT/STRUCTURE/CHARACTER band walks (matching `0x48c150`'s order) so the party
  draws on top. Reuse the existing `draw_pool`/`game_present_blit` (PRESENT_KEYED/ALPHA).
- Extend **`src/actor_render.{c,h}`** with the unported `0x493ba0` arms the party needs:
  the **shadow blit** (`0x4917b0`), the **color-remap** (`DAT_008a9358`), and the
  multi-part/effect-list walk (`+0x562a`). Keep `actor_render_static` as the static subset;
  add an `actor_render_character` (or extend) for the full path. Host-test the new logic
  bit-exact where pure.
- Replace `actor_spawn_cutscene_cast` (the snapshot hack) with real party spawns resolved
  through Phase 1; retire PORT-DEBT(cutscene-party-chars).

### Phase 3 — The arrival cutscene dynamics
- Port the **walk-in movement** (the cutscene actor FSM `0x402730`/`0x402330` driving the
  party from spawn to their settled positions) — replaces the frozen census positions.
- Port the **dialogue runner `0x49d6e0`** (text lines `0x3eb`/`0x3ec`/`0x3ed…` + portrait
  resolution via `0x556eb0`).
- Port the **`0x5a00c0` portrait/textbox overlay** (PORT-DEBT `ingame-nontile-layers`);
  this also unblocks the "Town of Tonkiness" area-title banner. Likely its own sub-module
  `src/dialogue.{c,h}` / `src/overlay.{c,h}`.

### Phase 4 — Controllable Arche (FUTURE, scoped — not detailed here)
Entry points exist: `game_drive.input` (poll, `src/input.c`), the leader party actor
(`room_state+0x200c`), `camera_follow` (`src/camera_follow.c`, already drives the pan). The
in-game character **movement/physics FSM** (the `0x59f2c0`-region in-game sim) is unexplored
and needs its own RE pass → a separate plan (the Phase-C / trace-studio gateway).

## Files

- **New:** `src/party.{c,h}` (dramatist registry + party band walk + party spawn),
  `src/dialogue.{c,h}` + `src/overlay.{c,h}` (Phase 3).
- **Modify:** `src/actor_render.{c,h}` (rich `0x493ba0` arms: shadow/remap/multi-part),
  `src/actor_spawn.{c,h}` (drop `CUTSCENE_CAST_DEFS`; real party spawn),
  `src/asset_register.c` (per-character sprite registration helper if needed),
  `src/main.c` (`enter_game` dramatist+party setup; `game_actor_walk` party pass),
  `tools/flow/retail_fields.json` (Phase-0 annotations). `src/Makefile` for new TUs.
- **Docs:** this plan, `findings/in-game-intro.md` (the cast table + writeups per phase),
  `engine-quirks.md` (retail facts: handle/dramatist registry, party band layout),
  `port-debt.md` (retire cutscene-party-chars; note the Phase-3 overlay).

## Verification

- **Phase 0:** the Frida cast table is internally consistent (every visible character
  mapped to an actor/band/bank).
- **Phase 1–2:** `render_diff` (port vs a seed-pinned retail capture at a matched sim-tick)
  shows the woman = res `0x477` **woman** + the girl present, both `[rect]`/`[decode]`-clean;
  a settled-town frame diff vs `runs/video60-retail` golden → the cast matches. Push port
  vs retail crops to the llm-feed for USER confirmation.
- **Phase 3:** the full arrival plays 1:1 in an anchor-aligned side-by-side video
  (walk-in + dialogue + portrait), extending the ckpt-89 intro video.
- Throughout: `nix develop --command make -C tests run` stays green for new pure logic;
  build both GUI + debug exes; regen the derived ledger when functions are ported.

## Reuse (don't re-implement)

`actor_spawn_pool` / `actor`/`actor_render_state` (`src/actor_spawn.h`),
`actor_render_describe`/`actor_render_static`/`actor_emit_part`/`anim_clip`
(`src/actor_render.c`), `ar_sprite_slot_register`/`ar_pool_get_slot` (`src/asset_register.c`),
`draw_pool`/`game_present_blit`/`zdd_object_blt_*`, `camera_follow`, `input_mgr`, and the
`game_actor_walk`/`game_actor_update` wiring (`src/main.c`).
