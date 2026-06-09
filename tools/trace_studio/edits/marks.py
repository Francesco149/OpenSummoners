"""edits/marks.py — the mark-type registry (single source of truth).

A "mark" is a viewer annotation at a frame (kind + optional note/box) stored in
the session's edits.jsonl.  ALL kinds here are WORKLIST kinds — unlike
openrecet there are no auto-applied trace pins (this project's RNG is globally
seed-pinned on both sides; per-trace {rngseed}/{phasepin} ops don't exist in
the flat input-trace format).  `apply` renders them into worklist.md with
frame context for Claude to chase.

Served at GET /api/registries; the SPA MarkBar renders one button per entry,
so adding a kind here surfaces end-to-end with zero JS edits.
"""
from __future__ import annotations

MARK_TYPES = [
    {"kind": "note", "label": "✎ divergence note", "applies": False,
     "hint": "flag a divergence at this frame — free text + optional box; "
             "Claude chases these from the worklist"},
    {"kind": "feature", "label": "★ feature", "applies": False,
     "hint": "a thing to RE/port that this frame shows (missing overlay, "
             "wrong sprite, …)"},
    {"kind": "rng", "label": "🎲 rng suspect", "applies": False,
     "hint": "looks RNG-phased (different random outcome, same logic)"},
    {"kind": "phase", "label": "⟲ phase suspect", "applies": False,
     "hint": "looks phase-shifted (same content, offset in time)"},
]

APPLY_KINDS: frozenset = frozenset()
WORKLIST_KINDS = frozenset(m["kind"] for m in MARK_TYPES)


def registry() -> list[dict]:
    return MARK_TYPES
