"""tools/trace_studio — the scrub-and-mark studio for port↔retail trace parity.

Design + how-to: docs/plans/trace-studio.md (architecture) and docs/trace-studio.md
(operational cheatsheet). Modeled on openrecet's tools/trace_studio (the proven
record→view→mark→apply→re-capture loop), adapted to this project's harness:

  - both sides are driven by per-side FLAT input traces ({"frame":N,"ids":[…]}),
    seed-pinned (OSS_RNG_DEFAULT_SEED both sides) + retail under --lockstep;
  - alignment is anchor-segmented tick-for-tick pairing (tas_diff's model) with a
    sticky ±drift best-match for the port's occasional duplicate frame;
  - marks/notes are the USER's divergence-flagging channel; `apply` turns them
    into worklist.md (no auto-pins — RNG is globally pinned here).
"""
