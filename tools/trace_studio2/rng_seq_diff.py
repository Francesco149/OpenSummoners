#!/usr/bin/env python3
"""rng_seq_diff.py — compare the per-sim-tick RNG stream of two OSR_STATE captures.

Since ckpt 192 both sides emit OSR_STATE once per SIM-TICK, each record carrying
its own `tick` (retail at the easer 0x43d1d0, port at drive_present), so the two
tick axes align 1:1 and the PRIMARY comparison is TICK-FOR-TICK (`bubble_diff`):
it reports every divergence BUBBLE (a contiguous run of diverging ticks), whether
each self-heals (the LCG re-converges — a benign draw-TIMING wobble, same total
draws redistributed across ticks) or is PERMANENT (the port's RNG goes static
while retail keeps drawing = a room-load spawn burst the port skips).  This
reveals structure the old subsequence match hid — it broke at the FIRST bubble and
called it a total divergence even when the streams re-converged one tick later.

A small constant sampling-phase lead is auto-absorbed (best_offset).  For a LEGACY
per-flip capture (no `tick` field on a side) it falls back to the DISTINCT-state
subsequence match (`load_seq`), which tolerated the lockstep flip-batching confound
that per-tick emission now eliminates.

Usage: rng_seq_diff.py <port.osr> <retail.osr> [context] [max_tick]
  context  = ticks of context printed around the permanent-divergence onset (6).
  max_tick = stop reading past this tick (default 6000; covers town→errands).
"""
import sys
from osr import stream_records, FRAMEBEG, STATE


def load_seq(path, max_tick=None):
    """Ordered [(tick, rng, rngcalls)], one per SIM-TICK (last state wins), then
    dedup CONSECUTIVE identical rng (idle/no-draw ticks + multi-flip repeats).

    Each STATE is keyed by its OWN `tick` field when present (ckpt 192: the retail
    proxy emits one STATE per sim-tick at the easer, so N>1 STATEs can sit under one
    FRAMEBEG under lockstep — a FRAMEBEG-association would collapse them all to the
    flip's tick).  Falls back to the enclosing FRAMEBEG tick for legacy per-flip
    captures with no `tick` field.  Stops once a tick exceeds max_tick (ticks are
    game_enter-relative + monotonic, so this skips a turbo capture's idle tail)."""
    per_tick = {}          # tick -> (rng, rngcalls)
    order = []
    cur = None
    for r in stream_records(path, {FRAMEBEG, STATE}):
        if r.type == FRAMEBEG:
            _, tick, _ = r.framebeg()
            if max_tick is not None and tick > max_tick and per_tick:
                break
            cur = tick
        elif r.type == STATE:
            fields = {f.name: f for f in r.state()}
            rng = fields["rng"].ival & 0xffffffff if "rng" in fields else None
            rc = fields["rngcalls"].ival if "rngcalls" in fields else -1
            tick = int(fields["tick"].ival) if "tick" in fields else cur
            if rng is None or tick is None:
                continue
            if max_tick is not None and tick > max_tick and per_tick:
                break
            if tick not in per_tick:
                order.append(tick)
            per_tick[tick] = (rng, rc)
    seq = []
    prev = None
    for t in order:
        rng, rc = per_tick[t]
        if rng != prev:
            seq.append((t, rng, rc))
            prev = rng
    return seq


def load_by_tick(path, max_tick=None):
    """Per-tick {tick: (rng, rngcalls)} — NOT dedup'd, tick-for-tick.  Also returns
    whether the capture carried its own `tick` field (per-sim-tick, ckpt 192) vs
    falling back to the enclosing FRAMEBEG (legacy per-flip).  Under per-tick
    emission the two sides' ticks align 1:1, so a direct tick-keyed compare reveals
    the true structure — self-healing bubbles + the permanent-divergence onset —
    that the subsequence match (built for the lockstep confound) hides."""
    per = {}
    has_tick = False
    cur = None
    for r in stream_records(path, {FRAMEBEG, STATE}):
        if r.type == FRAMEBEG:
            _, tick, _ = r.framebeg()
            cur = tick
        else:
            fields = {f.name: f for f in r.state()}
            if "rng" not in fields:
                continue
            if "tick" in fields:
                has_tick = True
                t = int(fields["tick"].ival)
            elif cur is not None:
                t = cur
            else:
                continue
            if max_tick is not None and t > max_tick:
                if per:
                    break
                continue
            per[t] = (fields["rng"].ival & 0xffffffff,
                      fields["rngcalls"].ival if "rngcalls" in fields else -1)
    return per, has_tick


def best_offset(pd, rd, lo=1, hi=600):
    """The constant tick offset o (retail_tick = port_tick + o) maximizing
    tick-for-tick rng matches over an early window — absorbs a small constant
    sampling-phase lead (usually 0 under per-tick emission)."""
    best_o, best_n = 0, -1
    ptw = [(t, v[0]) for t, v in pd.items() if lo <= t <= hi]
    for o in range(-3, 4):
        n = sum(1 for t, rng in ptw if rd.get(t + o, (None,))[0] == rng)
        if n > best_n:
            best_n, best_o = n, o
    return best_o


def bubble_diff(pd, rd, ctx):
    """Tick-for-tick RNG diff: report divergence BUBBLES (contiguous diverging
    ticks) + whether each self-heals, and the PERMANENT-divergence onset (the last
    matching tick → the errands RNG-free gap)."""
    o = best_offset(pd, rd)
    common = sorted(t for t in pd if (t + o) in rd)
    if not common:
        print("no overlapping ticks to compare")
        return 1
    print(f"tick-for-tick diff (retail_tick = port_tick + {o}); "
          f"{len(common)} overlapping ticks {common[0]}..{common[-1]}")
    idx = {t: i for i, t in enumerate(common)}
    diffs = [t for t in common if pd[t][0] != rd[t + o][0]]
    print(f"  match {len(common) - len(diffs)}/{len(common)} ticks, "
          f"{len(diffs)} diverge")
    if not diffs:
        print("  → RNG is tick-for-tick IDENTICAL across the whole window.")
        return 0
    # contiguous diverging runs = bubbles
    bubbles, run = [], [diffs[0]]
    for t in diffs[1:]:
        if idx[t] == idx[run[-1]] + 1:
            run.append(t)
        else:
            bubbles.append((run[0], run[-1]))
            run = [t]
    bubbles.append((run[0], run[-1]))
    last_match = max((t for t in common if pd[t][0] == rd[t + o][0]), default=None)
    heal = [b for b in bubbles if last_match is not None and b[1] < last_match]
    perm = [b for b in bubbles if last_match is None or b[1] >= last_match]
    print(f"  {len(bubbles)} bubble(s): {len(heal)} self-heal, "
          f"{len(perm)} permanent; last MATCHING tick = {last_match}")
    for bi, (a, b) in enumerate(bubbles):
        if bi >= 24:
            print(f"    ... ({len(bubbles) - bi} more bubbles)")
            break
        span = idx[b] - idx[a] + 1
        tag = "self-heals" if (last_match is not None and b < last_match) \
              else "PERMANENT — no re-converge (RNG-free gap onset)"
        print(f"    bubble {bi:2}: ticks {a}..{b} ({span}t) — {tag}")
    # the permanent onset = first diverging tick with no later match: the two LCG
    # streams split for good (a draw-COUNT divergence — one side draws a different
    # total from here — or a scene-timing split that breaks the tick correspondence).
    if perm:
        onset = perm[0][0]
        print(f"\n  PERMANENT divergence onset ~tick {onset} "
              f"(the LCG streams split — a draw-COUNT difference or a scene-timing "
              f"split; watch rc to tell which).  Context around it:")
        lo = max(common[0], onset - ctx)
        for t in [x for x in common if lo <= x <= onset + ctx]:
            pr, prc = pd[t]
            rr = rd[t + o][0]
            mark = "  <-- differ" if pr != rr else ""
            print(f"    tick {t:<5} port=0x{pr:08x} rc={prc:<6} "
                  f"retail=0x{rr:08x}{mark}")
    return 0


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    ctx = int(argv[3]) if len(argv) > 3 else 6
    mt = int(argv[4]) if len(argv) > 4 else 6000
    # Per-tick captures (ckpt 192): both sides carry a `tick` field, ticks align
    # 1:1 → the tick-for-tick bubble diff is exact + reveals self-healing.  Fall
    # back to the subsequence match only for a legacy per-flip capture.
    pd, ph = load_by_tick(argv[1], mt)
    rd, rh = load_by_tick(argv[2], mt)
    if ph and rh:
        return bubble_diff(pd, rd, ctx)
    print("legacy per-flip capture (no `tick` field on a side) — subsequence match:")
    p = load_seq(argv[1], mt)
    r = load_seq(argv[2], mt)
    print(f"port seq: {len(p)} distinct rng states (ticks {p[0][0]}..{p[-1][0]})")
    print(f"retail  : {len(r)} distinct rng states (ticks {r[0][0]}..{r[-1][0]})")
    # Turbo-robust: the retail capture (turbo) batches multiple sim-ticks per flip
    # so it emits a SUBSEQUENCE of the true per-tick rng stream, while the port
    # (1:1 flip:tick) emits the full stream.  So through the aligned span retail
    # is an in-order SUBSEQUENCE of port.  Two-pointer: for each retail value,
    # advance the port pointer to it; if port exhausts first, retail drew a value
    # the port never produces from that state = the first REAL divergence (or, at
    # the port's end, the errands spawn burst the port skips entirely).
    pv = [x[1] for x in p]
    i = 0
    matched = 0
    firstbad = None
    for j in range(len(r)):
        i0 = i
        while i < len(pv) and pv[i] != r[j][1]:
            i += 1
        if i >= len(pv):
            firstbad = (j, i0)
            break
        i += 1
        matched += 1
    print(f"\nsubsequence match: {matched}/{len(r)} retail states found in-order in port")
    if firstbad is None:
        print("retail is a full in-order subsequence of port (aligned; port is the "
              "superset — no retail draw is unexplained).")
        return 0
    j, i0 = firstbad
    rt, rr, rc = r[j]
    print(f"port EXHAUSTED matching retail[{j}] tick={rt} rng=0x{rr:08x} "
          f"(port ran out at its last state, port ticks end {p[-1][0]}).")
    print(f"  → retail keeps drawing past the port's last produced state = the "
          f"divergence point (errands spawn burst if port ticks ended ~town-end).")
    print(f"  port last {ctx} states:")
    for k in range(max(0, len(p) - ctx), len(p)):
        print(f"    port[{k}] tick={p[k][0]:<5} rng=0x{p[k][1]:08x} rc={p[k][2]}")
    print(f"  retail around the divergence (idx {j}):")
    for k in range(max(0, j - 2), min(len(r), j + ctx)):
        mark = "  <-- port can't match from here" if k == j else ""
        print(f"    retail[{k}] tick={r[k][0]:<5} rng=0x{r[k][1]:08x}{mark}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
