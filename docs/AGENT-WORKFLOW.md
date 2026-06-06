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
