#!/usr/bin/env python3
"""
tools/mem_watch.py — memory-access watch driver (structural-parity harness).

Finds the *writer* (or reader) of an engine memory region we can't locate
by reading the decompile — reached via indirect dispatch, or dropped from
Ghidra's output.  Frida's MemoryAccessMonitor write-protects the region in
retail; every access traps with the faulting instruction's address.  That
address → the owning engine function (via the port ledger) → the chip to
port.

The motivating target is the HANDOFF "who fills the +0x108 input ring
buffer" black box: point mem_watch at that region, boot retail to the title
screen, and read off the writer VA.

Usage:

    nix develop --command python3 tools/mem_watch.py \\
        --run-dir runs/memwatch-inputring \\
        --region 0x... :64:input_ring \\
        [--access w] [--max-frames 4000] [--duration-ms 120000]

  --region VA:SIZE[:LABEL]   repeatable.  VA is a Ghidra VA (ImageBase
                             0x00400000).  SIZE in bytes.  Page-granular —
                             a tight SIZE still watches the enclosing
                             page(s); keep it small to limit hot-page noise.

The agent arms the monitor pre-resume (before the engine runs), so an
init-time writer during boot is trapped on its first write.

Output:
    <run_dir>/mem_watch.jsonl    one row per trapped access
    <run_dir>/agent.log          Frida send(log)/errors
    stdout                       a single JSON object: ranked writer table,
                                 each writer mapped to its owning engine
                                 function + port status.

Retail is launched the same way tools/run-retail.sh does it: the
Steamless-unpacked exe is copied into the game directory (so its sibling
DLLs resolve) and run from there.  --analyze-only skips the capture and
re-ranks an existing <run_dir>/mem_watch.jsonl (fully offline).
"""

from __future__ import annotations

import argparse
import bisect
import json
import os
import sys
from pathlib import Path

# Reuse the capture plumbing wholesale.
sys.path.insert(0, str(Path(__file__).resolve().parent))
import frida_capture as fc  # noqa: E402

ROOT          = fc.ROOT
LEDGER_JSON   = ROOT / "docs" / "port-ledger.json"
FUNCTIONS_CSV = ROOT / "docs" / "decompiled" / "functions.csv"

# The Steamless-unpacked exe + game dir, matching tools/run-retail.sh.
UNPACKED_EXE  = ROOT / "vendor" / "unpacked" / "sotes.unpacked.exe"
GAME_DIR      = Path(os.environ.get(
    "OPENSUMMONERS_GAME_DIR",
    "/mnt/c/Program Files (x86)/Steam/steamapps/common/Fortune Summoners"))


# ─── ledger / function-table lookup ────────────────────────────────────────


def _load_function_index() -> list[tuple[int, int, dict]]:
    """Build a sorted [(start_va, end_va, info)] interval list of every
    engine function, so a faulting instruction VA can be mapped to its
    containing function by a single binary search.

    Prefers port-ledger.json (carries port status + src paths); falls back
    to functions.csv (name/size only) if the ledger is absent."""
    intervals: list[tuple[int, int, dict]] = []
    if LEDGER_JSON.exists():
        ledger = json.loads(LEDGER_JSON.read_text())
        for fn in ledger.get("functions", []):
            start = int(fn["va"], 16) if isinstance(fn["va"], str) else int(fn["va"])
            size  = int(fn.get("size", 0)) or 1
            intervals.append((start, start + size, {
                "name":   fn.get("name", ""),
                "status": fn.get("status", "unknown"),
                "src":    fn.get("src", []),
            }))
    elif FUNCTIONS_CSV.exists():
        import csv
        with FUNCTIONS_CSV.open() as f:
            for row in csv.DictReader(f):
                start = int(row["entry"], 16)
                size  = int(row["size"]) or 1
                intervals.append((start, start + size, {
                    "name":   row["name"],
                    "status": "unknown",
                    "src":    [],
                }))
    else:
        raise SystemExit(
            f"neither {LEDGER_JSON} nor {FUNCTIONS_CSV} found — "
            f"regenerate the ledger with tools/gen_port_ledger.py")

    intervals.sort(key=lambda iv: iv[0])
    return intervals


def _owner_of(va: int, intervals: list[tuple[int, int, dict]]) -> dict | None:
    """The function whose [start, end) range contains `va`, or None
    (CRT / library / unmapped code)."""
    starts = [iv[0] for iv in intervals]
    i = bisect.bisect_right(starts, va) - 1
    if i < 0:
        return None
    start, end, info = intervals[i]
    if start <= va < end:
        out = dict(info)
        out["func_va"]  = f"0x{start:08x}"
        out["insn_off"] = va - start   # offset of the faulting insn into fn
        return out
    return None


# ─── region-spec parsing ────────────────────────────────────────────────────


def _parse_region(spec: str) -> dict:
    """`VA:SIZE[:LABEL]` → {va, size, label}."""
    parts = spec.split(":")
    if len(parts) < 2:
        raise argparse.ArgumentTypeError(
            f"--region expects VA:SIZE[:LABEL], got {spec!r}")
    va   = int(parts[0], 0)
    size = int(parts[1], 0)
    label = parts[2] if len(parts) >= 3 and parts[2] else f"0x{va:08x}"
    return {"va": va, "size": size, "label": label}


def _parse_chain(spec: str) -> dict:
    """`ROOTVA:HOP1,HOP2,...:OFF:SIZE[:LABEL[:ARM_AT_FLIP]]` → a chain region.

    Watches a HEAP field reached through a global root pointer, resolved at
    runtime by the agent: read `*(ROOTVA)`, follow each comma-separated `HOP`
    byte offset as a pointer hop, then watch SIZE bytes at the final `OFF`.
    The camera/view x-scroll, for example:
        --watch-chain 0x8a9b50:0x104c:0x60:4:cam_x60:1610
    = *(*(0x8a9b50)+0x104c)+0x60, armed at flip 1610 (just before the pan, to
    conserve the re-arm budget on the hot view page)."""
    parts = spec.split(":")
    if len(parts) < 4:
        raise argparse.ArgumentTypeError(
            "--watch-chain expects ROOTVA:HOPS:OFF:SIZE[:LABEL[:ARM_AT_FLIP]], "
            f"got {spec!r}")
    va   = int(parts[0], 0)
    hops = [int(h, 0) for h in parts[1].split(",") if h]
    off  = int(parts[2], 0)
    size = int(parts[3], 0)
    label = parts[4] if len(parts) >= 5 and parts[4] else f"chain_0x{va:x}+0x{off:x}"
    arm  = int(parts[5], 0) if len(parts) >= 6 and parts[5] else 0
    return {"va": va, "hops": hops, "off": off, "size": size,
            "label": label, "chain": True, "arm_at_flip": arm}


# ─── post-processing: rank trapped accesses by faulting instruction ────────


def _rank(run_dir: Path, regions: list[dict]) -> dict:
    jsonl = run_dir / "mem_watch.jsonl"
    intervals = _load_function_index()
    region_by_idx = {i: r for i, r in enumerate(regions)}

    by_from: dict[int, dict] = {}
    total = 0
    if jsonl.exists():
        for line in jsonl.read_text().splitlines():
            line = line.strip()
            if not line:
                continue
            ev = json.loads(line)
            total += 1
            frm = int(ev["from"])
            g = by_from.setdefault(frm, {
                "from":        frm,
                "ops":         {},
                "n":           0,
                "first_frame": ev.get("frame"),
                "regions":     set(),
                "offsets":     set(),
            })
            g["n"] += 1
            g["ops"][ev["op"]] = g["ops"].get(ev["op"], 0) + 1
            ridx = int(ev.get("region", 0))
            g["regions"].add(ridx)
            reg = region_by_idx.get(ridx, {})
            base = reg.get("va")
            # Chain regions resolve to a heap base unknown python-side; the
            # accessed addr isn't an offset off the Ghidra VA, so skip it.
            if base is not None and not reg.get("chain"):
                g["offsets"].add(int(ev["addr"]) - base)

    writers = []
    for frm, g in sorted(by_from.items(), key=lambda kv: -kv[1]["n"]):
        owner = _owner_of(frm, intervals)
        offs = sorted(g["offsets"])
        writers.append({
            "from_va":        f"0x{frm:08x}",
            "n_accesses":     g["n"],
            "ops":            g["ops"],
            "first_frame":    g["first_frame"],
            "regions":        sorted(g["regions"]),
            "sample_offsets": [(f"+0x{o:x}" if o >= 0 else f"-0x{-o:x}")
                               for o in offs[:12]],
            "owner_func":     owner["func_va"] if owner else None,
            "owner_name":     owner["name"]    if owner else None,
            "owner_status":   owner["status"]  if owner else "unmapped",
            "owner_src":      owner["src"]      if owner else [],
            "insn_offset":    (f"+0x{owner['insn_off']:x}" if owner else None),
        })

    candidates = [w for w in writers
                  if w["owner_status"] in ("unported", "stubbed", "unmapped")]
    return {
        "total_accesses":   total,
        "distinct_writers": len(writers),
        "regions":          [{"index": i, **r} for i, r in enumerate(regions)],
        "writers":          writers,
        "port_candidates":  candidates,
    }


# ─── retail launch (mirrors tools/run-retail.sh's drop) ────────────────────


def _drop_retail_exe() -> tuple[Path, Path]:
    """Copy the unpacked exe into the game dir so its sibling DLLs resolve,
    and return (drop_path, game_dir).  Caller unlinks the drop after the
    run.  Same approach as tools/run-retail.sh."""
    import shutil
    if not UNPACKED_EXE.exists():
        raise SystemExit(f"{UNPACKED_EXE} missing — run ./tools/setup.sh first")
    if not GAME_DIR.is_dir():
        raise SystemExit(f"game dir not found: {GAME_DIR}")
    drop = GAME_DIR / f"sotes-unpacked-memwatch-{os.getpid()}.exe"
    shutil.copy2(UNPACKED_EXE, drop)
    return drop, GAME_DIR


def _capture(args, regions: list[dict]) -> None:
    drop, game_dir = _drop_retail_exe()
    try:
        input_trace = None
        if getattr(args, "input_trace", None) is not None:
            if not args.input_trace.exists():
                raise SystemExit(f"--input-trace: file not found: {args.input_trace}")
            input_trace = fc.parse_input_trace(args.input_trace)
            print(f"[mem_watch] input-trace: {len(input_trace)} entries from "
                  f"{args.input_trace.name}", file=sys.stderr)
        cfg = fc.CaptureConfig(
            exe=drop, cwd=game_dir,
            run_dir=args.run_dir, exact_run_dir=True,
            max_frames=args.max_frames, duration_ms=args.duration_ms,
            remote=args.remote,
            auto_start_server=not args.no_auto_start,
            turbo=not args.no_turbo,
            hide_window=args.hide_window,
            seed_pin=args.seed_pin,
            input_trace=input_trace,
            mem_watch=True,
            mem_watch_regions=[{**r, "access": args.access} for r in regions],
            mem_watch_precise=not args.no_precise,
            mem_watch_flip_rearm=args.rearm_per_flip,
            mem_watch_hw=args.hw,
        )
        rc = fc.run_capture(cfg)
        if rc != 0:
            print(f"[mem_watch] capture returned rc={rc}", file=sys.stderr)
    finally:
        try:
            drop.unlink()
        except OSError:
            pass


# ─── cli ────────────────────────────────────────────────────────────────────


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--run-dir", type=Path, required=True,
                    help="where to write mem_watch.jsonl + agent.log")
    ap.add_argument("--region", type=_parse_region, action="append",
                    default=None, metavar="VA:SIZE[:LABEL]",
                    help="watched STATIC region (Ghidra VA).  Repeatable.")
    ap.add_argument("--watch-chain", type=_parse_chain, action="append",
                    default=None, dest="watch_chain",
                    metavar="ROOTVA:HOPS:OFF:SIZE[:LABEL[:ARM_AT_FLIP]]",
                    help="watched HEAP region reached through a global root "
                         "pointer, resolved at runtime (e.g. the camera/view "
                         "field *(*(0x8a9b50)+0x104c)+0x60).  Repeatable.")
    ap.add_argument("--access", choices=("w", "rw"), default="w",
                    help="trap writes only (default) or reads+writes. "
                         "'rw' floods on hot pages — use only when the "
                         "reader is what's unknown.")
    ap.add_argument("--no-precise", action="store_true",
                    help="raw one-shot-per-page mode.  By default the agent "
                         "re-arms on page-neighbor traps and records only "
                         "accesses inside the watched field.")
    ap.add_argument("--rearm-per-flip", action="store_true",
                    help="re-arm the page monitor once per Flip instead of "
                         "per-access — one trap per frame, no hot-page "
                         "livelock.  REQUIRED for fields on a page the engine "
                         "touches every frame (e.g. the camera/view object).")
    ap.add_argument("--hw", action="store_true",
                    help="use a HARDWARE watchpoint (DR register, write-only on "
                         "the exact bytes) instead of MemoryAccessMonitor — the "
                         "fitting tool for a HOT heap field: zero neighbour "
                         "overhead, hardware auto-rearm, no livelock.  Use with "
                         "--watch-chain + an arm_at_flip gate.")
    ap.add_argument("--remote", default=fc.DEFAULT_REMOTE,
                    help="frida-server host:port (default %(default)s)")
    ap.add_argument("--max-frames", type=int, default=4000,
                    help="stop after this many message-pump iterations "
                         "(default %(default)s)")
    ap.add_argument("--duration-ms", type=int, default=180_000,
                    help="wall-clock ceiling (default %(default)s)")
    ap.add_argument("--no-auto-start", action="store_true",
                    help="skip auto-launching frida-server.exe")
    ap.add_argument("--input-trace", type=Path, default=None,
                    help="drive retail with this input-trace JSONL (needed to "
                         "reach in-game scenes, e.g. the town pan).")
    ap.add_argument("--no-turbo", action="store_true",
                    help="disable turbo (required for live boots that need the "
                         "message pump — engine-quirk #29).")
    ap.add_argument("--show-window", dest="hide_window", action="store_false",
                    default=True, help="show the game window (default hidden).")
    ap.add_argument("--no-seed-pin", dest="seed_pin", action="store_false",
                    default=True, help="don't pin the RNG seed (default pinned).")
    ap.add_argument("--analyze-only", action="store_true",
                    help="skip the capture; just re-rank an existing "
                         "<run_dir>/mem_watch.jsonl (offline)")
    args = ap.parse_args(argv)

    regions = list(args.region or []) + list(args.watch_chain or [])
    if not regions:
        ap.error("at least one --region or --watch-chain is required")
    args.run_dir.mkdir(parents=True, exist_ok=True)

    if not args.analyze_only:
        _capture(args, regions)

    summary = _rank(args.run_dir, regions)
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
