#!/usr/bin/env python3
"""
tools/test_call_trace_diff.py — unit tests for call_trace_diff.py.

Run inside `nix develop`:
    python3 -m pytest tools/test_call_trace_diff.py -q
"""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

import call_trace_diff as ctd


def _write_jsonl(path: Path, rows: list[dict]) -> None:
    path.write_text("".join(json.dumps(r) + "\n" for r in rows))


# ── load_trace ──────────────────────────────────────────────────────────────


def test_load_trace_counts_per_frame(tmp_path):
    p = tmp_path / "t.jsonl"
    _write_jsonl(p, [
        {"va": 0x100, "ret_va": 0, "frame": 0},
        {"va": 0x100, "ret_va": 0, "frame": 0},   # same VA twice in frame 0
        {"va": 0x200, "ret_va": 0, "frame": 1},
    ])
    by_frame, stubs = ctd.load_trace(p)
    assert by_frame[0][0x100] == 2
    assert by_frame[1][0x200] == 1
    assert stubs == {}                              # no stub rows


def test_load_trace_tracks_stub_rows(tmp_path):
    p = tmp_path / "t.jsonl"
    _write_jsonl(p, [
        {"va": 0x100, "ret_va": 0, "frame": 0, "stub": True},
        {"va": 0x100, "ret_va": 0, "frame": 0},     # one stubbed, one not
    ])
    by_frame, stubs = ctd.load_trace(p)
    assert by_frame[0][0x100] == 2
    assert stubs[0][0x100] == 1


def test_load_trace_tolerates_blank_lines(tmp_path):
    p = tmp_path / "t.jsonl"
    p.write_text('{"va":1,"frame":0}\n\n   \n{"va":2,"frame":0}\n')
    by_frame, _ = ctd.load_trace(p)
    assert set(by_frame[0]) == {1, 2}


def test_load_trace_rejects_malformed(tmp_path):
    p = tmp_path / "t.jsonl"
    p.write_text('{"va":1,"frame":0}\nNOT JSON\n')
    try:
        ctd.load_trace(p)
    except SystemExit as e:
        assert "malformed JSONL" in str(e)
    else:
        raise AssertionError("expected SystemExit on malformed line")


# ── diff_frames ──────────────────────────────────────────────────────────────


def test_diff_frames_partitions():
    retail = Counter({0x10: 1, 0x20: 2, 0x30: 1})   # 0x30 retail-only
    port   = Counter({0x10: 1, 0x20: 1, 0x40: 1})   # 0x40 port-only
    d = ctd.diff_frames(retail, port)
    assert d["overlap"] == [(0x10, 1, 1), (0x20, 2, 1)]  # 0x20 count mismatch
    assert d["retail_only"] == [(0x30, 1, 0)]
    assert d["port_only"] == [(0x40, 0, 1)]


def test_first_frame_with_va():
    by_frame = {5: Counter({0xaa: 1}), 9: Counter({0xbb: 1, 0xaa: 1})}
    assert ctd.first_frame_with_va(by_frame, 0xaa) == 5
    assert ctd.first_frame_with_va(by_frame, 0xbb) == 9
    assert ctd.first_frame_with_va(by_frame, 0xcc) is None


# ── end-to-end main() ────────────────────────────────────────────────────────


def test_main_align_on_first(tmp_path, capsys):
    """retail anchors at frame 900, port at frame 5; align-on-first pairs
    them and reports overlap / retail-only / port-only correctly."""
    retail = tmp_path / "retail.jsonl"
    port   = tmp_path / "port.jsonl"
    _write_jsonl(retail, [
        {"va": 0x56aea0, "ret_va": 0, "frame": 900},   # anchor
        {"va": 0x5b8fc0, "ret_va": 0, "frame": 900},
        {"va": 0x412c10, "ret_va": 0, "frame": 900},   # retail-only (gap)
    ])
    _write_jsonl(port, [
        {"va": 0x56aea0, "ret_va": 0, "frame": 5},      # anchor
        {"va": 0x5b8fc0, "ret_va": 0, "frame": 5},
        {"va": 0x582e90, "ret_va": 0, "frame": 5},      # port-only
    ])
    rc = ctd.main([
        "--retail", str(retail), "--port", str(port),
        "--align-on-first", "0x56aea0", "--verbose",
        "--functions-csv", str(tmp_path / "nonexistent.csv"),
    ])
    out = capsys.readouterr().out
    assert rc == 0
    assert "retail anchor: frame 900" in out
    assert "port anchor:   frame 5" in out
    assert "overlap (called on both):    2" in out
    assert "retail-only (port missing):  1" in out
    assert "port-only (retail skipped):  1" in out
    assert "0x412c10" in out          # the gap shows in retail-only section
    assert "0x582e90" in out          # the divergence shows in port-only


def test_main_frame_offset_alignment(tmp_path, capsys):
    """--retail-frame-offset shifts retail frames before matching."""
    retail = tmp_path / "retail.jsonl"
    port   = tmp_path / "port.jsonl"
    _write_jsonl(retail, [{"va": 0x1000, "ret_va": 0, "frame": 100}])
    _write_jsonl(port,   [{"va": 0x1000, "ret_va": 0, "frame": 2}])
    # port_frame == retail_frame + offset → 2 == 100 + (-98)
    rc = ctd.main([
        "--retail", str(retail), "--port", str(port),
        "--retail-frame-offset", "-98",
        "--functions-csv", str(tmp_path / "nope.csv"),
    ])
    out = capsys.readouterr().out
    assert rc == 0
    assert "retail frame 100 vs port frame 2" in out
    assert "overlap (called on both):    1" in out


def test_main_errors_on_empty_trace(tmp_path, capsys):
    retail = tmp_path / "retail.jsonl"
    port   = tmp_path / "port.jsonl"
    retail.write_text("")
    _write_jsonl(port, [{"va": 1, "frame": 0}])
    rc = ctd.main(["--retail", str(retail), "--port", str(port)])
    assert rc == 2
    assert "no events" in capsys.readouterr().err
