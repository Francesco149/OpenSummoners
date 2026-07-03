#!/usr/bin/env python3
"""rng_seq_diff.py — compare the ORDERED RNG-state SEQUENCE of two OSR_STATE captures.

The RNG census is confounded by the port/retail dialogue-timing skew (the two
advance the town-intro dialogue at different sim-ticks, so a tick-for-tick join
is misaligned even when the RNG draw ORDER is identical).  But RNG draws are
INPUT-driven, not wall-clock-driven: under the same nav both sides issue the same
logical sequence of LCG draws.  So we compare the ordered sequence of DISTINCT
per-tick `rng` state words (dedup consecutive repeats — idle ticks that draw
nothing, and the multi-flip-per-tick real-time captures, both collapse), and
report the first index where the sequences diverge = the first divergent draw
batch.  Each entry keeps its (tick, rngcalls) for localisation.

Usage: rng_seq_diff.py <port.osr> <retail.osr> [context]
  context = how many aligned entries to print before the divergence (default 6).
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


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    ctx = int(argv[3]) if len(argv) > 3 else 6
    mt = int(argv[4]) if len(argv) > 4 else 6000
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
