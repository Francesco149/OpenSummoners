#!/usr/bin/env python3
"""tools/trace_studio2/pair.py — the Trace Studio v2 tick-join (M6, the identity
JOIN; openrecet E3).

The v1 sync-bug class: frames paired on the FLIP axis with a ±drift pixel search
that *hunts* through content-quiet stretches and absorbs retail's coalesced ticks
as ±1-2 tick error.  Converging two sides was hand-work (push/pull the per-side
traces).

v2 dissolves it.  Every FRAMEBEG carries the deterministic `sim_tick` (the easer
0x43d1d0 call count, reset at game_enter, port mirror g_sim_tick_count) — a STORED
identity, IDENTICAL on both sides for the same logical in-game moment no matter how
far a load stretched the absolute flip count.  Pairing is a JOIN on sim_tick,
computed ONCE here: group each side's frames by sim_tick, take the LAST flip per
tick (the final presented state for that tick — retail coalesces / re-presents,
quirk #99), and align tick-for-tick.  Where a tick exists on only one side it is an
EXPLICIT, named gap — never a silent mispair.

sim_tick resets to 0 at game_enter and the title/menu/prologue boot does NOT tick
the easer, so the ENTIRE pre-game_enter boot collapses to tick 0 (its last flip =
the game_enter establishing frame).  The tick axis is meaningful IN-GAME — which is
the parity frontier (room-render / freeroam).  Boot frames are scrubbed per-side in
osr_view's single-file mode; this join is the in-game port|retail axis.

Both files are read with osr.stream_records (block-buffered, skips the bulky
BLIT/TEXT/SHEET payloads) so this runs on the multi-GB retail.osr without OOM.

Usage (host tools need the nix prefix):
  nix develop --command python3 tools/trace_studio2/pair.py PORT.osr RETAIL.osr
  nix develop --command python3 tools/trace_studio2/pair.py PORT.osr RETAIL.osr --write-pairs pairs.json
"""
from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import osr  # noqa: E402


@dataclass
class Side:
    path: str
    header: osr.Header
    frames: list           # [(ordinal, flip, tick)] in stream order
    by_tick: dict          # tick -> (ordinal, flip) for the LAST flip at that tick
    anchors: dict          # name -> (flip, tick, rng)
    game_enter_flip: int   # the game_enter anchor flip (-1 if absent)


def load_side(path: str) -> Side:
    """One streaming pass: per-frame (ordinal, flip, tick), last-flip-per-tick, and
    the anchors.  ANCHOR + FRAMEBEG are both tiny records, streamed together."""
    header = osr.read_header(path)
    frames: list = []
    by_tick: dict = {}
    anchors: dict = {}
    ordinal = 0
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.ANCHOR}):
        if r.type == osr.FRAMEBEG:
            flip, tick, _ = r.framebeg()
            frames.append((ordinal, flip, tick))
            by_tick[tick] = (ordinal, flip)     # last write wins = last flip per tick
            ordinal += 1
        else:  # ANCHOR
            flip, tick, rng, name = r.anchor()
            anchors[name] = (flip, tick, rng)
    ge = anchors.get("game_enter")
    return Side(path, header, frames, by_tick, anchors,
                ge[0] if ge else -1)


@dataclass
class Pair:
    tick: int
    port: tuple | None      # (ordinal, flip) or None (retail-only gap)
    retail: tuple | None    # (ordinal, flip) or None (port-only gap)

    @property
    def kind(self) -> str:
        if self.port and self.retail:
            return "paired"
        return "port_only" if self.port else "retail_only"


def join(port: Side, retail: Side) -> list[Pair]:
    """JOIN on sim_tick over the sorted union of both sides' ticks.  Matched ticks
    pair; unmatched are honest, named gaps (kept in the timeline, not dropped)."""
    ticks = sorted(set(port.by_tick) | set(retail.by_tick))
    return [Pair(t, port.by_tick.get(t), retail.by_tick.get(t)) for t in ticks]


def naive_flip_drift(port: Side, retail: Side) -> int | None:
    """Contrast: what FLIP-axis pairing reduces to.  game_enter resets the tick on
    both sides but lands at DIFFERENT flips, so pairing the same flip-number offsets
    every in-game moment by (retail_ge_flip - port_ge_flip) flips ≈ that many /2
    ticks of silent drift.  Returns the flip offset, or None if either lacks the
    anchor."""
    if port.game_enter_flip < 0 or retail.game_enter_flip < 0:
        return None
    return retail.game_enter_flip - port.game_enter_flip


def report(port: Side, retail: Side, pairs: list[Pair], *, quiet: bool = False) -> dict:
    def say(*a):
        if not quiet:
            print(*a)

    def side_line(tag: str, s: Side):
        ticks = [t for _, _, t in s.frames]
        flips = [f for _, f, _ in s.frames]
        say(f"{tag:<7}: {osr.SIDE_NAME.get(s.header.side, s.header.side):<6} "
            f"seed=0x{s.header.seed:x} scenario={s.header.scenario!r}")
        if s.frames:
            say(f"         {len(s.frames)} frames  flip {min(flips)}..{max(flips)}  "
                f"sim_tick {min(ticks)}..{max(ticks)}  distinct_ticks={len(s.by_tick)}  "
                f"game_enter@flip={s.game_enter_flip}")

    side_line("port", port)
    side_line("retail", retail)

    paired   = [p for p in pairs if p.kind == "paired"]
    p_only   = [p for p in pairs if p.kind == "port_only"]
    r_only   = [p for p in pairs if p.kind == "retail_only"]
    say(f"\nsim_tick identity-join (last flip per tick):")
    say(f"  timeline ticks   : {len(pairs)}  (union of both sides)")
    say(f"  paired           : {len(paired)} / {min(len(port.by_tick), len(retail.by_tick))} "
        f"(min distinct-tick count)")
    say(f"  port-only  (gaps): {len(p_only)}"
        + (f"  e.g. ticks {[p.tick for p in p_only[:5]]}" if p_only else ""))
    say(f"  retail-only(gaps): {len(r_only)}"
        + (f"  e.g. ticks {[p.tick for p in r_only[:5]]}" if r_only else ""))
    if paired:
        e = paired[len(paired) // 2]   # a mid in-game sample
        say(f"  e.g. tick {e.tick}: port flip {e.port[1]} (frame #{e.port[0]}) "
            f"== retail flip {e.retail[1]} (frame #{e.retail[0]})  "
            f"— {e.retail[1] - e.port[1]:+d} absolute flips, SAME tick")

    # anchor assertions: shared anchors must agree on RNG (the join's validity proof)
    say(f"\nanchor RNG assertions (a mismatch localizes a desync):")
    shared = [n for n in port.anchors if n in retail.anchors]
    if not shared:
        say("  (no shared anchors)")
    for n in sorted(shared, key=lambda n: port.anchors[n][0]):
        pf, pt, pr = port.anchors[n]
        rf, rt, rr = retail.anchors[n]
        ok = "OK " if pr == rr else "FAIL"
        say(f"  [{ok}] {n:<18} rng port=0x{pr:x} retail=0x{rr:x}  "
            f"(flip port={pf} retail={rf}, tick port={pt} retail={rt})")
    rng_ok = all(port.anchors[n][2] == retail.anchors[n][2] for n in shared)

    # the FLIP-axis contrast (what tick-join buys us)
    drift = naive_flip_drift(port, retail)
    if drift is not None:
        say(f"\nflip-axis contrast: game_enter lands at port flip {port.game_enter_flip} "
            f"vs retail flip {retail.game_enter_flip} → naive same-flip pairing drifts "
            f"every in-game moment by {drift:+d} flips (~{drift // 2:+d} ticks) — silent. "
            f"the tick-join above is immune.")

    verdict = "PASS" if (paired and rng_ok) else "CHECK"
    say(f"\nverdict: {verdict}  ({len(paired)} tick-paired frames, "
        f"anchors {'aligned' if rng_ok else 'DIVERGE'})")
    return {
        "verdict": verdict,
        "rng_ok": rng_ok,
        "n_paired": len(paired),
        "n_port_only": len(p_only),
        "n_retail_only": len(r_only),
        "n_timeline": len(pairs),
        "flip_drift": drift,
    }


def write_pairs(path: str, port: Side, retail: Side, pairs: list[Pair]) -> None:
    """Write pairs.json: the ordered timeline keyed by tick, each entry carrying both
    sides' frame ordinal + flip (or null for a gap).  A reference/inspection artifact
    — osr_view computes the same join natively from the two indices."""
    doc = {
        "port":   {"path": port.path,   "scenario": port.header.scenario,
                   "game_enter_flip": port.game_enter_flip},
        "retail": {"path": retail.path, "scenario": retail.header.scenario,
                   "game_enter_flip": retail.game_enter_flip},
        "pairs": [
            {"tick": p.tick, "kind": p.kind,
             "port_ordinal":   p.port[0]   if p.port   else None,
             "port_flip":      p.port[1]    if p.port   else None,
             "retail_ordinal": p.retail[0] if p.retail else None,
             "retail_flip":    p.retail[1]  if p.retail else None}
            for p in pairs
        ],
    }
    with open(path, "w") as f:
        json.dump(doc, f)
    print(f"wrote {path}  ({len(pairs)} timeline entries)")


def main(argv) -> int:
    ap = argparse.ArgumentParser(description="tick-join two .osr captures (port + retail)")
    ap.add_argument("port", help="the PORT-side .osr (src/osr_emit.c)")
    ap.add_argument("retail", help="the RETAIL-side .osr (capture proxy)")
    ap.add_argument("--write-pairs", metavar="PATH", help="also write pairs.json")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args(argv[1:])

    port = load_side(args.port)
    retail = load_side(args.retail)
    # tolerate either order: ensure port is side 0, retail side 1
    if port.header.side == 1 and retail.header.side == 0:
        port, retail = retail, port

    pairs = join(port, retail)
    v = report(port, retail, pairs, quiet=args.quiet)
    if args.write_pairs:
        write_pairs(args.write_pairs, port, retail, pairs)
    return 0 if v["verdict"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
