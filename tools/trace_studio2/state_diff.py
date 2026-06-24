#!/usr/bin/env python3
"""state_diff.py — wx/hvel state diff on the SIM-TICK axis (frame-lock chase #3).

The camera-independent counterpart to sync_diff.py (which compares body SCREEN-x,
confounded by the follow-camera).  Aligns the PORT's freeroam mover state against a
state-bearing RETAIL re-drive per sim-tick and reports wx (position) + hvel
(horizontal velocity) — the raw physics, with no camera in the loop.

Both sides emit OSR_STATE (port: --osr-state, retail: OSS_OSR_STATE=1).  The field
NAMES differ — the proxy emits the leader-chain values raw (`wx`,`hvel`), the port
emits its freeroam mirror (`fr_wx`,`fr_vel`) — so we normalise both to (wx,hvel).

Position origins can differ (the port's freeroam spawn vs retail's), so wx is
compared RELATIVE to the walk-start tick (the first tick hvel!=0 in-window) — the
accel-ramp SHAPE is what chase #3 tests, camera-free.  hvel is absolute (same units,
the 1/100-px/tick fixed point both engines share).

Usage: state_diff.py <port.osr> <retail.osr> [lo_tick] [hi_tick] [hvel_tol]
"""
import sys
from osr import stream_records, FRAMEBEG, STATE


def load_states(path):
    """tick -> {field_name: ival}, last write wins per tick (1 state/flip)."""
    out, cur = {}, None
    for r in stream_records(path, {FRAMEBEG, STATE}):
        if r.type == FRAMEBEG:
            _, tick, _ = r.framebeg()
            cur = tick
        elif r.type == STATE and cur is not None:
            out[cur] = {f.name: f.ival for f in r.state()}
    return out


def norm(d):
    """(wx, hvel) from either field-naming convention; None if absent."""
    wx = d.get("wx", d.get("fr_wx"))
    hv = d.get("hvel", d.get("fr_vel"))
    return wx, hv


def first_moving_tick(states, lo, hi):
    for t in range(lo, hi + 1):
        if t in states:
            _, hv = norm(states[t])
            if hv:
                return t
    return None


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    port_p, retail_p = argv[1], argv[2]
    lo = int(argv[3]) if len(argv) > 3 else 0
    hi = int(argv[4]) if len(argv) > 4 else 10 ** 9
    tol = int(argv[5]) if len(argv) > 5 else 0

    port = load_states(port_p)
    retail = load_states(retail_p)
    if not port or not retail:
        print(f"port states={len(port)} retail states={len(retail)} — "
              "did both capture --osr-state / OSS_OSR_STATE=1?")
        return 1

    common = sorted(t for t in port if t in retail and lo <= t <= hi)
    if not common:
        print(f"no shared ticks in [{lo},{hi}] (port {min(port)}..{max(port)}, "
              f"retail {min(retail)}..{max(retail)})")
        return 1

    # wx origin per side = wx at each side's own walk-start (first moving tick).
    pstart = first_moving_tick(port, lo, hi)
    rstart = first_moving_tick(retail, lo, hi)
    pbase = norm(port[pstart])[0] if pstart else norm(port[common[0]])[0]
    rbase = norm(retail[rstart])[0] if rstart else norm(retail[common[0]])[0]
    print(f"# port walk-start tick={pstart} wx0={pbase} | "
          f"retail walk-start tick={rstart} wx0={rbase}")
    print(f"# {'tick':>6} {'p_hvel':>7} {'r_hvel':>7} {'dhvel':>6}  "
          f"{'p_relwx':>8} {'r_relwx':>8} {'drelwx':>7}  flag")

    first_div = None
    nmiss = 0
    for t in common:
        pwx, phv = norm(port[t])
        rwx, rhv = norm(retail[t])
        if None in (pwx, phv, rwx, rhv):
            continue
        prel, rrel = pwx - pbase, rwx - rbase
        dh, dw = (phv or 0) - (rhv or 0), prel - rrel
        bad = abs(dh) > tol or abs(dw) > tol
        flag = ""
        if bad:
            nmiss += 1
            flag = "<-- DIVERGE"
            if first_div is None:
                first_div = t
        # print every moving tick or any divergence (skip long idle stretches)
        if bad or phv or rhv:
            print(f"  {t:>6} {phv:>7} {rhv:>7} {dh:>6}  "
                  f"{prel:>8} {rrel:>8} {dw:>7}  {flag}")

    print(f"\n# divergences (tol={tol}): {nmiss}; first at tick {first_div}"
          if first_div is not None else
          f"\n# MATCH within tol={tol} across {len(common)} shared ticks "
          f"[{common[0]}..{common[-1]}] — wx/hvel ramps are bit-equal")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
