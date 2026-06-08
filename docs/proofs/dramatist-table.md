# Proof — the "Dramatist" character table `DAT_006b6ea8` (the arrival cast, by name)

**Date:** 2026-06-08 (ckpt 92). **Status:** 100%-proven (static binary data + decompile
control-flow + retail render census, three independent sources agree).
**Reproduce:** `python3 tools/dump_dramatist_table.py` (reads the user's own
`vendor/unpacked/sotes.unpacked.exe`; only the derived RE facts are committed).

This settles the long-running "who is the missing party character" question (ckpts 90→91b)
with named ground truth and **corrects ckpt 91b's tangled identities** (see the bottom).

## The mechanism

The engine identifies a *named character* — a **"dramatist"** — by a 32-bit **handle**, not
by its actor code. `FUN_0041f200` (the EFFECT activator) logs the literal string
**`"Get Dramatist Info"`** at `0x41f200:51`, then (`:54-69`) if spawned with a non-zero
handle (`param_9`) it scans the table **`DAT_006b6ea8`** for that handle and:

- sets the actor's **effective code** (`+0x1d4`) = the row's **code** field (the archetype /
  sprite-switch selector), when spawned with code 0 (`param_6==0`);
- carries the row's **bank** field (`+0x30`) as `sVar17`, which **overrides** the
  archetype's default sheet: each archetype `case` does
  `if (sVar17==0) sVar17 = <facing-default>; FUN_00426d70(0, sVar17, 0);`.

So **code = the character archetype** (pose/clip set, shared by many NPCs) and
**bank = the specific sheet** (recolor/variant). The handle picks both.

Row layout (`0x34` bytes = 13 dwords):

| off | type | field |
|-----|------|-------|
| +0x00 | u32 | **handle** (key; the list is handle==0 terminated) |
| +0x04 | u32 | **code** (archetype / `0x41f200` switch selector) |
| +0x08 | char[0x28] | **name** (NUL-terminated ASCII) |
| +0x30 | u16 | **bank** (primary sprite sheet; 0 ⇒ loaded dynamically, e.g. the party leads) |

## The arrival cast — three sources agree

The town-intro cutscene `FUN_004d7d80` case `0x334be` (`:37-40`) spawns the arriving family,
anchor-relative to the wagon (anchor `0x65`):

```
FUN_00431d10(0,        0x1872d, 0x65, 0x3200,     0, 0);  // the wagon (ported)
FUN_0041f0e0(0,        0xc3f0,  0x65, 0x6400,     0, 3, 0, 0);  // Dr. Barnard (code, no handle)
FUN_0041f0e0(0x5f5e1d3,0,       0x65, 8000,       0, 3, 0, 0);  // handle -> Arche's Father
FUN_0041f0e0(0x5f5e1d4,0,       0x65, 0xfffff380, 0, 1, 0, 0);  // handle -> Arche's Mother
```

…then resolves handle **`0x5f5e165` (Arche)** for the dialogue (`0x4d7d80:104,118,…`; Arche is
the persistent party LEADER, created at new-game, not spawned here). Cross-checked against the
retail render census `runs/cutscene-cast` (`0x493ba0`, settled frame 2550, banks + world-x):

| handle | code | dramatist name | bank | census rs_x | port today |
|--------|------|----------------|------|------|------------|
| `0x0c878d35` | `0xc3e6` | **Guard** | `0xe6`→map `0xe5` | 23800 (far left) | renders ✓ |
| **`0x5f5e1d4`** | **`0xc440`** | **Arche's Mother** | **`0xb5`** | **38400 (center)** | **WRONG — port spawns map `0xc440` bank `0xa6`** |
| **`0x5f5e165`** | **`0xc35a`** | **Arche** | **`0`→`0x8b/0x8c/0x8d`** | **41600 (center)** | **MISSING — culls (sprite unregistered)** |
| **`0x5f5e1d3`** | **`0xc3dc`** | **Arche's Father** | **`0xe3`** | 49600 (center-right) | renders ✓ |
| `0x35a4e901` | `0xc3f0` | **Dr. Barnard** | `0xeb` | 67200 (right of horses) | renders ✓ |
| (map object) | `0xc440` | townswoman (no handle) | `0xa6` | 92600 (far right) | renders ✓ |

The family clusters at center — **Mother (38400) · Arche (41600) · Father (49600)** — which is
the "Arche and the woman (her mom)" the USER confirmed on the golden.

### The decompile nails each bank

- **Arche** — `0x41f200` case `0xc35a` (`:793`): installs banks **`0x8b`/`0x8c`/`0x8d`** (+`+0x84`=`0x8e`),
  clip **`0x62a8c8`** (`:908`, == the census clip for `0xc35a`). A 3-row multi-clip protagonist body;
  her dramatist bank is 0, so she has no static sheet — loaded by the (unported) party/new-game path.
- **Arche's Mother** — case `0xc440` (`:1768`, archetype string **`"Woman"`**): default bank
  `0xa6/0xa7/0xa8/0xa5` by facing **when `sVar17==0`**, else the dramatist bank. The **map** townswoman
  (no handle ⇒ `sVar17=0`, facing 1) gets the default **`0xa6`** — exactly the port's `TOWN_EFFECT_DEFS`
  bank and the census's far-right `0xc440`. **Mom** carries dramatist bank **`0xb5`**, so she renders a
  *different* sheet — the one the port never spawns.
- **Arche's Father** — case `0xc3dc` (`:1386`, man archetype); dramatist bank `0xe3` (the port already
  spawns `0xc3dc` bank `0xe3`).

## Corrections to ckpt 91b (USER-confirmed there, refined here)

ckpt 91b's bottom line stands — *the party/character system that loads these sheets is unported* —
but it tangled the identities. With names:

- `0xc3f0` / res `0x477` is **Dr. Barnard** (a man right of the horses), not "the woman". The port
  renders him correctly. (ckpt 91b got "it's a man, renders OK" right; the `CUTSCENE_CAST_DEFS`
  *code comment* still calls him "THE WOMAN / decode bug" — that's the retracted ckpt-91 error.)
- **`0xc35a` is Arche alone** (the protagonist girl), not "Arche + the woman". She is the one true
  *cull* (no static sprite).
- **The "woman (mom)" is `0xc440` bank `0xb5`** — a *distinct* actor the port currently mis-renders
  as the generic map townswoman (`0xc440` bank `0xa6`). Same archetype code, wrong sheet.
- **Arche's Father (`0xc3dc`) renders correctly already.** Only Arche + Mom's-real-sheet are wrong.

## Why this matters for the port (Phase 1)

The chip that makes the cast correct is the **dramatist resolution**: port `DAT_006b6ea8` + the
`0x41f200:54-69` handle→code/bank lookup + the archetype cases (`0xc440` Woman, `0xc3dc` man,
`0xc35a` Arche). Then spawn the cutscene family by handle so Mother gets bank `0xb5` (not `0xa6`),
and register Arche's banks `0x8b`–`0x8e` + her clip `0x62a8c8` to render the protagonist.
