#!/usr/bin/env python3
"""lcg_walk.py — derive RETAIL's per-tick rand() draw COUNT purely from the rng
STATE sequence, by walking the MSVC LCG between consecutive per-tick states.

The LCG (FUN_005bf505) is deterministic: state' = state*0x343fd + 0x269ec3 (mod
2^32).  So the number of draws made ON a tick = the LCG "distance" from the prior
tick's ending state to this tick's ending state (walk forward up to MAXW and see
how many steps land on it).  This gives retail's per-tick draw count from a capture
that carries only `rng` (no rngcalls) — e.g. the ckpt-192 retail-rngcensus2.osr —
so the tick-974 divergence can be read as COUNT-vs-TIMING WITHOUT the new proxy
counter.  A distance of >MAXW (no match) means either a big burst or a NON-rand
seed write (srand/reseed) broke the pure-LCG chain — itself diagnostic.

Usage: lcg_walk.py <port.osr> <retail.osr> [lo_tick] [hi_tick]
"""
import sys
from osr import stream_records, FRAMEBEG, STATE

MUL, ADD, MASK = 0x343fd, 0x269ec3, 0xffffffff
MAXW = 4000   # max draws/tick to search (a room-load spawn burst can be large)


def step(s):
    return (s * MUL + ADD) & MASK


def lcg_dist(a, b, maxw=MAXW):
    """draws to go from state a to state b (a already applied), or None if >maxw."""
    if a == b:
        return 0
    s = a
    for n in range(1, maxw + 1):
        s = step(s)
        if s == b:
            return n
    return None


def load_states(path, lo, hi):
    """{tick: (rng, rngcalls_or_-1)} for ticks in [lo-1, hi] (need lo-1 as the walk base)."""
    per = {}
    cur = None
    for r in stream_records(path, {FRAMEBEG, STATE}):
        if r.type == FRAMEBEG:
            _, cur, _ = r.framebeg()
        else:
            f = {x.name: x for x in r.state()}
            if "rng" not in f:
                continue
            t = int(f["tick"].ival) if "tick" in f else cur
            if t is None:
                continue
            if t > hi and per:
                break
            if lo - 1 <= t <= hi:
                per[t] = (f["rng"].ival & MASK,
                          f["rngcalls"].ival if "rngcalls" in f else -1)
    return per


def per_tick_draws(per):
    """{tick: draws_on_tick} via LCG-walk from the previous present tick's state."""
    out = {}
    ticks = sorted(per)
    for i, t in enumerate(ticks):
        if i == 0:
            continue
        pt = ticks[i - 1]
        if pt != t - 1:
            continue  # gap — can't attribute
        out[t] = lcg_dist(per[pt][0], per[t][0])
    return out


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    lo = int(argv[3]) if len(argv) > 3 else 960
    hi = int(argv[4]) if len(argv) > 4 else 1000
    pp = load_states(argv[1], lo, hi)
    rr = load_states(argv[2], lo, hi)
    pd = per_tick_draws(pp)
    rd = per_tick_draws(rr)
    # also read the port's OWN rngcalls delta (ground truth) to validate the walk
    print(f"tick   port_rng     p_draws(walk/rc)  retail_rng   r_draws(walk)  note")
    for t in range(lo, hi + 1):
        if t not in pp or t not in rr:
            continue
        pr, prc = pp[t]
        rrng, rrc = rr[t]
        pw = pd.get(t)
        rw = rd.get(t)
        # port rc delta (ground truth draw count) if available
        prc_d = (prc - pp[t - 1][1]) if (t - 1 in pp and prc >= 0 and pp[t - 1][1] >= 0) else None
        same = "" if pr == rrng else "  <-- STATE differs"
        cnt = ""
        if pw is not None and rw is not None and pw != rw:
            cnt = f"  COUNTΔ={pw - rw}"
        pws = "?" if pw is None else str(pw)
        rws = "?" if rw is None else str(rw)
        prcs = "" if prc_d is None else f"/{prc_d}"
        print(f"{t:<6} 0x{pr:08x}   {pws:>4}{prcs:<5}         "
              f"0x{rrng:08x}   {rws:>4}          {same}{cnt}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
