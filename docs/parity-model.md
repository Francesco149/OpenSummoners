# The multi-pillar parity model

The discipline that keeps 1:1 work honest: **before you suspect ported logic, attribute
the divergence to a pillar.** An observed difference between the port and retail comes
from exactly one of four sources. Three of them are *not* logic bugs — they are origin
offsets you normalize away. If you skip this step you will "fix" correct code to chase a
phase or seed artifact, and break parity elsewhere.

This is the SotES-specific statement of the model openrecet formalized
(`../openrecet/CLAUDE.md`). Cross-referenced from `CLAUDE.md` and `parity-ledger.md`.

---

## The four pillars

### 1. Logic / data → output  *(the only one we actually port)*
The pure contract: **same inputs ⇒ same output.** This is what a ported function must
reproduce bit-for-bit. A real logic divergence survives after pillars 2–4 are pinned. If
it does, it is a genuine port bug (wrong constant, wrong branch, wrong struct offset,
wrong trig fn, …) — fix it and re-verify to `differ_px == 0`.

### 2. Phase  *(load-dependent origin — normalize, don't fix)*
A counter or animation cycle whose **origin** depends on boot/load timing, not on logic.
Same per-tick math, different starting point. Examples already documented:
- **Render-rate / flip-skew** (`parity-ledger.md` R3): retail renders at display refresh
  (~127 Hz on the capture rig) and duplicates each scene state ~2.2×; the port renders
  one flip per scene state. The *wall-clock pacing matches* (~9.2 s to the menu both
  sides) and the *distinct-content sequence* matches — but **flip indices do not align**.
  Compare a port flip against the retail flip showing the **same scene state**
  (phase/fade/menu_fade), never the same index.
- Any free-running counter sampled at a scene boundary whose absolute value is set by how
  long the boot/intro took.

Phase offsets present as a **constant** offset (CONST-OFFSET), not growing drift. Accept
them; record which counter and why. They are normalized by anchoring (below).

### 3. RNG  *(same stream, possibly different origin)*
The engine LCG is **`FUN_005bf505`**, state word **`DAT_008a4f94`**, seeded
`srand(time())`-style at boot (so retail's stream is wall-clock-random run-to-run). Same
*consumption order/count* ⇒ same values **for a given seed**. Pin the seed on both sides:
- Port boots a fixed **`OSS_RNG_DEFAULT_SEED`** (`src/rng.h`).
- Retail harness **`--seed-pin`** writes the same value into `DAT_008a4f94` at the first
  consumer.

A divergence that survives a pinned seed is a real consumption-order bug (we draw a
different number of randoms, or in a different order) — that IS pillar 1. A divergence
that only appears unpinned is just the seed origin; not a bug.

### 4. Upstream inputs  *(fix in order, frame 0 forward)*
If a frame's *inputs* already diverge, the divergence you see this frame is not this
frame's code. Don't blame it. Walk the chain in order and fix the earliest diverging
frame first; later frames often resolve for free.

---

## The procedure

1. **Pin phase + RNG + inputs.** Seed-pin both sides; drive the same anchored TAS trace
   (`docs/parity-harness.md`); align on the chosen **anchor**, not a flip index.
2. **Compare at equal phase/tick.** `tools/tas_diff.py` aligns the two flip axes on the
   anchor and matches each port frame to the best retail frame within a small drift
   window (absorbing the ±1 duplicate-frame cadence wrinkle) — so a real divergence is
   never hidden, and a phase offset is never mistaken for a bug.
3. **Read the verdict:**
   - **ALIGNED** (`differ_px == 0` at equal phase) → logic is **confirmed 1:1 given same
     data**, even if raw unpinned output differs. Record it in `parity-ledger.md`.
   - **CONST-OFFSET** (identical content, shifted origin) → pillar 2/3, an *accepted*
     phase/seed offset. Record which counter; do **not** touch the logic.
   - **DRIFT** (growing difference) → a real pillar-1 divergence. Chase it: narrow to the
     first diverging frame, then the first diverging function/field/blit, fix, re-verify.
4. **Record the distinction.** Note data-1:1 vs observed-1:1 and which pillars are
   off-but-accepted, so a known phase/seed origin is never re-litigated as wrong logic.

---

## What counts as an accepted residual
Only two classes, and both must be **named and understood** in `parity-ledger.md`:
- A documented **benign deviation** (a structural difference with zero pixel effect, or a
  retail artifact the port deliberately doesn't reproduce — recorded with its cause).
- Occasional **≤1-LSB texture-sampling noise** on a rotated/scaled quad (sub-texel
  rounding differs across capture GPUs) — only after the quad's geometry/UVs/diffuse are
  proven bit-identical, so it is provably the rasterizer, not our math.

Everything else with `differ_px > 0` is an OPEN investigation item with hypotheses
([[feedback_bit_exact]]) — never "close enough".
