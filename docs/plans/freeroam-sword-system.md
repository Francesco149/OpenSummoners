# Plan — the freeroam SWORD / ATTACK system (Z unsheathe + X attack)

> **Status: SCOPED (ckpt 154), porting NOT started.**  USER (game owner, ckpt 154):
> "she DOES have a sword available here [the errands] so let's do the sword stuff."
> So Arche's sword IS exercisable in the errands freeroam — the next moveset chip after
> the SLIDE (ckpt 154, done).  This doc is the durable scope so the arc survives a `/clear`.
> Orient: `CLAUDE.md` → `FRONT.md` → here.  Sibling: `findings/freeroam-pose-commands.md`
> (the U/D-pose + slide), `plans/controllable-arche-faithful.md` (the freeroam base).

## The USER ground-truth (engine-quirks #3311, the game owner)
- **Z = unsheathe / sheathe the sword** (a TOGGLE; a ring action).  **Two distinct
  POSE/anim SETS for everything by sword-out vs sword-in** — i.e. a SECOND full cel set
  (walk/idle/crouch/jump…) for the sword-out stance.
- **X = ATTACK when the sword is OUT, INTERACT when not** (e.g. talk to mom).  Different
  attacks by holding FORWARD / UP / DOWN while attacking.

## The command map — RE'd off `0x478ba0` (the char-AI builder; ckpt 154 initial pass)
The builder snapshots `entity+0x14854` cmd block + re-derives each cmd per tick:
- `cmd[0]` `+0x14854` — walk/dash (1/5 L, 2/6 R) — PORTED.
- `cmd[2]` `+0x1485c` — jump (7/8/9) — PORTED.
- `cmd[3]` `+0x14860` — U/D pose (10 down / 0xb up) — PORTED (ckpt 153/154).
- **`cmd[4]` `+0x14864` — ATTACK (0xe / 0xf)** — THIS arc.  Built from:
  - `:296-300` AI/auto attack (`+0x158a4`+0x128 set, 200ms gate) → `0x479f90` + cmd[4]=0xe.
  - **`:301-303` the X-button attack:** `(param_1+0x66 != 0)` [SWORD OUT] AND
    `(param_1+0x68 == 0)` [not mid-swing] AND ring **id 9** (X) in 400ms → cmd[4]=**0xf**.
  - `:459-460` `(param_1+0x66 != 0) || cmd[4]==0xe` → cmd[4]=0xf (normalize).

## The key STATES (to confirm by capture + RE)
- **`param_1+0x66` (s16) = the SWORD-OUT flag** (≠0 ⇒ X attacks; gates the attack arm).
  `param_1` = the body/anim struct (the `0x442a70` `puVar4`?; +0x48/+0x58/+0x66/+0x68 are
  anim/state fields).  CONFIRM the offset + that Z toggles it.
- **`entity+0x750` = the WEAPON/COMBAT struct** (`+0x140` mode, `+0x138[]` attack-slot
  array, `+0x144`, `+0x466`).  `FUN_00479d30(+0x750)`=get stance / `FUN_00479da0(+0x750,n)`
  =set stance (0-4); `FUN_0047a420(dir,flag)` cycles `+0x138[]` attack slots (modular %10);
  `FUN_0047a300`.  This is the combat/weapon state machine.
- **`entity+0x14898` = the stance-arm mode** (`:336`; ==1 enables the 0x18-0x1c stance
  selects; code 0xc35a [Arche] + `+0x750`+0x466==0 forces it to 2 at :337-339).
- **`entity+0x14894` (s16)** — an attack/combo gate (`:435`, `:472`).
- The control scheme `*(*0x8a6e80+0x104)` (0/1/2) branches the bind set (`:322/399/421`).

## The stance / unsheathe arms (`:341-374`, ids 0x18-0x1c)
Five ring binds (ids 0x18,0x19,0x1a,0x1b,0x1c, 200ms) each: if pressed AND current
stance (`0x479d30`) != target AND `0x4071f0(+0x750, +0x750[0x140], n)` ok → set stance n
(`0x479da0`).  Plus `0x47a7f0(0x1d..0x21, …, 5..9)` (`:376-394`) and the 0xa/0xc/0xd +
0xe/0xf arms (`:397-455`) via `0x47a300`/`0x47a420`.  **The Z unsheathe/sheathe toggle is
in here** (likely `0x47a420`/`0x47a300` flipping `+0x66` / the `+0x750` mode) — pin the
exact id + fn by capture.

## OPEN QUESTIONS (resolve capture-first, like the slide)
1. **Which ring id does Z post** for unsheathe/sheathe? (X=id 9 confirmed; Z=? — depends
   on the `*0x8a6e80` keybind + scheme `+0x104`.)  Ground-truth: inject Z's scancode ring
   in the errands freeroam, read which `+0x14864`/`+0x66`/`+0x750` field changes.
2. **What IS `+0x66` (sword-out)** + which fn toggles it — confirm `0x47a420`/`0x47a300`.
3. **The SECOND pose set (sword-out cels):** the bank/frame range for sword-out
   walk/idle/crouch/jump (the USER's "two distinct anims").  Off `retail.osr` res 0x570
   (draw_probe) with the sword drawn — like the ckpt-153b pose cels.
4. **The ATTACK cels + the apply** (`0x442a70`'s cmd[4]=0xe/0xf handling = a new body
   state): the swing anim + hitbox (the hitbox is gameplay, defer; the ANIM is the port).
5. **Directional attacks** (forward/up/down + X) — the variant selection.
6. **X=INTERACT when sword in** (talk to mom) — the non-combat X path (proximity + NPC
   dialogue trigger).  May overlap the door-enter / interact subsystem.

## The PLAN (capture-first, the slide/pose pattern)
1. **Capture the GROUND TRUTH** in the errands freeroam (the proven `ids:[4,4]`+held +
   the field-spec / proxy `.osr`): press Z (find its ring id by scanning), read `+0x66` /
   `+0x750`+0x140 / `+0x14898` per tick → the sword-out toggle.  Then press X (id 9) with
   sword out → cmd[4]=0xf, the apply body state, the swing cels (res 0x570 draw stream).
   Capture sword-out WALK/IDLE/CROUCH cels (the second pose set).
   - Field spec: extend `pose_consts_fields.json` (or a new `sword_fields.json`) with
     `+0x66`, `+0x750`→`+0x140`/`+0x466`, `+0x14894`/`+0x14898`, `cmd4`=+0x14864.
   - Nav: the proven dash-ring2 prefix → freeroam, then Z (scan ids) + X.
2. **Port the STANCE TOGGLE** (`character_resolve_attack`/`_sword` sibling to
   `character_resolve_pose`): Z ring → toggle a `sword_out` field; X ring → cmd[4] when out.
3. **Port the SWORD-OUT POSE SET** (`arche_*_clip` variants keyed on `sword_out`) +
   the ATTACK anim (a new `arche_attack_clip`, the cmd[4] body state in `character_step`).
4. **Verify bit-exact** off a port `.osr` vs retail (the swing cels + the sword-out
   walk/idle, tick-for-tick) — `draw_probe --res 0x570`.
5. **Document** (finding + quirk + ledger) + retire the relevant moveset debt.

## Risks / notes
- BIG arc — likely 2-3 chips (toggle+pose, attack anim, directional + interact).  The
  HITBOX/damage is gameplay (defer / PORT-DEBT); the ANIM + state are the faithful port.
- `0x442a70` is the 12KB shared integrator — RE the STRUCTURE but PIN values by capture
  (the ckpt-117/153 lesson; the cmd[4] body state will reuse stack slots).
- X=interact (sword-in) may need the NPC-proximity + dialogue-trigger subsystem (overlaps
  `char-up-door-probe` / the questline) — scope separately if so.
