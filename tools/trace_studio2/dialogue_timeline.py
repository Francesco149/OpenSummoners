#!/usr/bin/env python3
"""tools/trace_studio2/dialogue_timeline.py — reconstruct the DIALOGUE reveal
timeline from an .osr draw stream (the typewriter ground truth).

The in-game dialogue box draws its body text per-glyph with GDI TextOutA
(quirk #63): font ref 3 (Courier New 7px), the BODY in main colour 0x3e537d
(DIALOGUE_BODY_MAIN) + shadow 0xa8b9cc, the speaker NAME in white 0xffffff +
shadow 0x455f7b.  Counting the body MAIN glyphs per frame and sorting them by
(y, x) reconstructs exactly what was revealed that tick — so this module reads
the *real* typewriter curve straight off a capture (retail OR port), with no
guessing about the engine's internal grade logic.

It streams (works on the 1.9 GB retail.osr) and groups frames into LINES: within
a line the revealed body text only grows (each tick's string starts-with the
previous); when that prefix relation breaks (or the box clears, or the name
changes) a new line begins.  Per line it reports the start tick, the
fully-revealed tick, the last (advance) tick, and the per-tick reveal length —
the inputs for THEME 1 (the reveal rate / skip) and the matched-cadence nav (the
per-line advance ticks).

Usage:
    python3 tools/trace_studio2/dialogue_timeline.py <file.osr> [tick_lo] [tick_hi]
    python3 tools/trace_studio2/dialogue_timeline.py <file.osr> --curve <line_idx>
"""
from __future__ import annotations

import sys

sys.path.insert(0, __file__.rsplit("/", 1)[0])
import osr  # noqa: E402

BODY_MAIN = 0x3E537D    # DIALOGUE_BODY_MAIN (the revealed body glyphs)
NAME_MAIN = 0xFFFFFF    # the speaker name (white main copy)
DLG_FONT = 3            # Courier New 7px (the dialogue font_ref)


def frame_dialogue(texts):
    """From one frame's TEXT records, reconstruct (name, body) strings.
    Body = the main-colour body glyphs sorted by (row y, x); name likewise.
    Filter by COLOUR only (the cross-side-stable identity) — NOT font_ref, which
    each side numbers independently in its own FONT table (retail ref 3, port 11)."""
    body = [(t.y, t.x, t.text) for t in texts if t.color == BODY_MAIN]
    name = [(t.y, t.x, t.text) for t in texts if t.color == NAME_MAIN]
    # group body glyphs into rows (y clusters ~28 apart), then sort by x in-row
    body.sort(key=lambda g: (g[0], g[1]))
    name.sort(key=lambda g: (g[0], g[1]))
    bstr = "".join(g[2] for g in body)
    nstr = "".join(g[2] for g in name)
    return nstr, bstr, len(body)


def timeline(path, tick_lo=0, tick_hi=10**9):
    """Yield (tick, flip, name, body, nbody) per frame in [tick_lo, tick_hi]
    that carries dialogue body or name glyphs (collapses nothing — caller does)."""
    cur = None
    bucket = []
    for r in osr.stream_records(path, {osr.FRAMEBEG, osr.TEXT}):
        if r.type == osr.FRAMEBEG:
            if cur is not None and tick_lo <= cur[1] <= tick_hi:
                name, body, nb = frame_dialogue(bucket)
                yield cur[1], cur[0], name, body, nb
            flip, tick, _ = r.framebeg()
            cur = (flip, tick)
            bucket = []
            if tick > tick_hi:
                break
        elif r.type == osr.TEXT and cur is not None:
            bucket.append(r.text())
    if cur is not None and tick_lo <= cur[1] <= tick_hi:
        name, body, nb = frame_dialogue(bucket)
        yield cur[1], cur[0], name, body, nb


class Line:
    __slots__ = ("idx", "name", "full", "start_tick", "full_tick",
                 "advance_tick", "curve")

    def __init__(self, idx, name, tick, flip, body):
        self.idx = idx
        self.name = name
        self.full = body
        self.start_tick = tick
        self.full_tick = None
        self.advance_tick = tick
        self.curve = [(tick, len(body))]   # (tick, revealed_len)


def lines(path, tick_lo=0, tick_hi=10**9):
    """Group the timeline into dialogue LINES (see module doc)."""
    out = []
    cur = None
    for tick, flip, name, body, nb in timeline(path, tick_lo, tick_hi):
        if not body:
            # box empty this tick — close any open line (a gap = advance done)
            cur = None
            continue
        is_new = (
            cur is None
            or name != cur.name
            or not body.startswith(cur.full[:len(body)])  # not a prefix-extension
        )
        # a shrink that is still a prefix of the same target = same line still
        # revealing? no — within a line text only grows.  Treat any non-extension
        # (including a shorter different string) as a new line.
        if is_new:
            cur = Line(len(out), name, tick, flip, body)
            out.append(cur)
        else:
            if len(body) > len(cur.full):
                cur.full = body
            cur.advance_tick = tick
            cur.curve.append((tick, len(body)))
            if cur.full_tick is None and body == cur.full and len(body) > 0:
                # mark the first tick fully revealed (best-effort; refined below)
                pass
    # second pass: full_tick = first tick whose reveal == the final full length
    for ln in out:
        fl = len(ln.full)
        ln.full_tick = next((t for t, n in ln.curve if n >= fl), ln.advance_tick)
    return out


def skip_tick(ln):
    """The tick a line was SKIPPED (the typewriter reveal jumps by >=2 in one
    tick — impossible for the 1-char/tick typewriter, so it is a confirm press),
    or None if the line typed to completion with no skip."""
    prev = None
    for t, n in ln.curve:
        if prev is not None and n - prev >= 2:
            return t
        prev = n
    return None


def cmd_nav(retail, boot_nav, tick_lo, tick_hi, offset, boot_max, runoff_leads=None):
    """Emit a MATCHED-CADENCE nav (stdout): the flip-keyed boot prefix of
    `boot_nav` (entries with frame <= boot_max) then tick-keyed confirms (id
    0x24) at each retail line's SKIP tick + ADVANCE tick, +offset.  The port,
    sharing the sim-tick axis, then advances the dialogue at retail's exact
    cadence (plans/intro-cutscene-1to1.md, the THEME 1 prerequisite).

    runoff_leads: {line_idx: lead_ticks} — a CONCURRENT beat-gap (the L7->L8
    "Arche runs ahead" run-off) where the line's dialogue beat COMPLETES
    lead_ticks before its body clears: retail fires the camera pan + 0x402730
    run-off on the beat completion (caravan pans tick 977) while the box holds
    its full text to its full+24 auto-hold.  So press its advance at
    advance_tick - lead_ticks (the port lingers the box; cutscene.c
    ARRIVAL_RUNOFF_BOX_HOLD).  The room-transition beat-gap (L9->house) keeps
    advance_tick (its fades run AFTER the advance), so this is opt-in per line."""
    runoff_leads = runoff_leads or {}
    import json
    # boot prefix: keep the flip-keyed menu nav, drop the old dialogue spam
    print(f"# matched-cadence nav: boot prefix (frame<= {boot_max}) of "
          f"{boot_nav} + tick confirms from {retail} ticks [{tick_lo},{tick_hi}] "
          f"offset {offset:+d}")
    with open(boot_nav) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            e = json.loads(line)
            if "frame" in e and e["frame"] <= boot_max:
                print(json.dumps(e, separators=(",", ":")))
    lns = lines(retail, tick_lo, tick_hi)
    confirms = []
    for i, ln in enumerate(lns):
        st = skip_tick(ln)
        if st is not None:
            confirms.append((st + offset, f"skip L{ln.idx} {ln.name!r}"))
        # SPEAKER-CHANGE advances fire 8 ticks BEFORE the body-last-shown tick.
        # retail processes the advance ~2 ticks BEFORE the new box opens (it arms
        # the OLD box's reverse-ramp portrait dissolve immediately, the new box's
        # re-pop has a ~2-tick latency — engine-quirk #108).  The box-frame OPENS
        # at advance_tick−6 (the new box spawns in front while the old box lingers
        # full ~6t then closes — quirk #107), so the press = box-open − 2 =
        # advance_tick − 8 (the port delays its re-pop 2t back to advance_tick−6,
        # keeping the 28/28 box-frame match while the portrait fade-out leads it by
        # 2).  A SAME-speaker advance has no overlap → fires at advance_tick.  The
        # last line in range has no known next (a same-speaker page or a THEME-3
        # beat gap, not an overlap) → no lead.
        # The -8 lead is the speaker-change box OVERLAP (#108) — it ONLY applies to
        # a QUICK reopen (the new box pops in ~10t after the advance).  A LONG gap to
        # the next line means a NON-dialogue beat ran between them (the L7→L8 run-off,
        # or the L9→house room transition with its exit/entry fades): the box closes
        # FULLY (no overlap), so the advance fires at advance_tick with no lead.
        nxt = lns[i + 1] if i + 1 < len(lns) else None
        gap = (nxt.start_tick - ln.advance_tick) if nxt is not None else 0
        overlap = (nxt is not None) and (nxt.name != ln.name) and (gap <= 20)
        runoff = runoff_leads.get(ln.idx)
        if runoff is not None:
            adv = ln.advance_tick - runoff
            tag = f" [run-off lead -{runoff}]"
        else:
            adv = ln.advance_tick - (8 if overlap else 0)
            tag = (" [spkr-change -8]" if overlap
                   else (" [beat-gap]" if (nxt is not None and gap > 20) else " [same]"))
        confirms.append((adv + offset, f"adv  L{ln.idx} {ln.name!r}" + tag))
    confirms.sort()
    for tick, why in confirms:
        print(f"# {why}")                        # standalone comment line (parser skips)
        print(f'{{"tick":{tick},"ids":[36]}}')
    print(f"# {len(lns)} lines, {len(confirms)} confirms", flush=True)
    return 0


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    if argv[1] == "NAV":
        # NAV <retail.osr> <boot_nav.jsonl> <tick_lo> <tick_hi> [offset] [boot_max]
        #     [runoff_leads]   runoff_leads = "L:T,L:T" (e.g. "7:6" for the L7->L8 run-off)
        retail, boot_nav = argv[2], argv[3]
        tlo, thi = int(argv[4]), int(argv[5])
        offset = int(argv[6]) if len(argv) > 6 else 0
        boot_max = int(argv[7]) if len(argv) > 7 else 1100
        runoff_leads = {}
        if len(argv) > 8 and argv[8]:
            for pair in argv[8].split(","):
                k, v = pair.split(":")
                runoff_leads[int(k)] = int(v)
        return cmd_nav(retail, boot_nav, tlo, thi, offset, boot_max, runoff_leads)
    path = argv[1]
    if len(argv) >= 4 and argv[2] == "--curve":
        li = int(argv[3])
        tlo = int(argv[4]) if len(argv) > 4 else 0
        thi = int(argv[5]) if len(argv) > 5 else 10**9
        lns = lines(path, tlo, thi)
        if li >= len(lns):
            print(f"only {len(lns)} lines")
            return 1
        ln = lns[li]
        print(f"line {li}  name={ln.name!r}  full({len(ln.full)})={ln.full!r}")
        print(f"  start_tick={ln.start_tick} full_tick={ln.full_tick} "
              f"advance_tick={ln.advance_tick}")
        prev = None
        for t, n in ln.curve:
            d = "" if prev is None else f"  (+{n - prev})"
            print(f"    tick {t}: {n}{d}")
            prev = n
        return 0
    tick_lo = int(argv[2]) if len(argv) > 2 else 0
    tick_hi = int(argv[3]) if len(argv) > 3 else 10**9
    lns = lines(path, tick_lo, tick_hi)
    print(f"== {len(lns)} dialogue lines in tick [{tick_lo},{tick_hi}] ==")
    for ln in lns:
        nticks = len(ln.curve)
        span = ln.advance_tick - ln.start_tick
        reveal_span = ln.full_tick - ln.start_tick
        print(f"L{ln.idx:<2} {ln.name:<18} start={ln.start_tick:<5} "
              f"full={ln.full_tick:<5} adv={ln.advance_tick:<5} "
              f"(reveal {reveal_span}t, hold {ln.advance_tick - ln.full_tick}t, "
              f"len={len(ln.full)})  {ln.full[:48]!r}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
