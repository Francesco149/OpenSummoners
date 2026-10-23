# When to spawn subagents

The detail behind CLAUDE.md's "Subagents are allowed but used judiciously"
bullet. This file is **only** about that decision — everything else about how we
work (commits, nix develop, the harness, quirks, findings, `/clear` hygiene,
build outputs, reading the decompile) lives in CLAUDE.md, which is the dense
single source. Keep this file minimal so stale process notes don't pile up here.

## The deciding question

**Does the work require judgment, or just careful execution?**

- **Judgment → the main loop.** The Opus orchestrator holds the full reasoning
  history and the multi-pillar parity model in context; a subagent pattern-matches
  shallowly and misses quirks. The routine chip cadence (read Ghidra → write C →
  test → commit) stays single-threaded so that context compounds.
- **Wide, independent, read-only execution → a subagent earns its keep.** When the
  answer means sweeping many files / functions and you only need the conclusion,
  fan it out.

Two real costs gate every spawn: the **debrief/round-trip cost**, and the fact
that the subagent **lacks your reasoning history**. So when you already hold the
thread, do it inline even if it's long — don't be trigger-happy.

## Good fits

- Wide read-only audits: map the binary into subsystems; scout many forward-path
  clusters; scan `docs/decompiled/all.c` for a pattern.
- Pull N facts from a sibling project (`/opt/src/openrecet`, `/opt/src/OpenMare`).
- Run a smoke capture + summarize the diff.
- `tools/workflows/subsystem-survey.js` is the canonical `Workflow` of read-only
  `Explore` agents (seeds `ROADMAP.md` + `engine-quirks.md`). Its output is
  **decompile-grade, not ground truth** — byte-verify every offset before a port
  leans on it.

## Bad fits (stay in the main loop)

- Choosing what to port next; deciding real-bug-vs-harness-quirk.
- Cross-subsystem synthesis; an unscoped "find anything interesting".
- Anything touching `vendor/` or committing.

## These rules are LIVING

When a subagent pattern works well or burns you, edit this file so the lesson
sticks. Don't let a good/bad pattern go unrecorded.

## Wall-clock lessons (2026-10-23 audit: this project had the longest sessions)

Measured, then fixed in tooling: decomp2c looped over ~9,600 data symbols per function compiling a regex each
(O(symbols × text): the 430 KB story scripts took ~15 min; now 108 s, output byte-identical; a 27-module dry regen
54 s); it rescanned the caller's asm per call site and re-read the 10 MB decompile per function (memoized). rhost's
build compiled serially and recompiled rh_hooks.c (~30 s) in every link of the SizeOfImage pad loop (2–4 per build):
75–125 s per `rhost.py run` after any src edit; now 12 s warm / 64 s cold.  The gate itself is ~2.5 min Windows /
~4.3 min wine, bound by rh-combat (57,721 frames).
Loop habits that cost the most:
- **Profile a slow tool before waiting on it again** (`python3 -m cProfile`; for regex-heavy converters attribute
  re.* time per source line) — a 15-minute step is a bug until shown otherwise.
- **`rhost.py run` / `verify` / `corpus` rebuild when src is newer than rhost.exe**: don't edit src / rh_hooks.c
  while one is starting (a half-wired module failed a link mid-run).
- **Bisect a divergence with one multi-frame dump run** (`--dump-at F1,F2,…` up to 128 frames, then `rhs.py` per
  frame), not `--hash-every 1` (hashes the whole state every frame: 528 s for 7.8k frames).
- **Shadow a hot function sampled** (`NAME@N` in VERIFY_HOOKS): full shadow snapshots the state per call — a
  collision test called 84k times by f2100 ran at 2 fps and tripped the 60 s stall detector.

