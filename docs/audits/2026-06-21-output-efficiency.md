# Output-token efficiency audit — 2026-06-21 (OpenSummoners)

> **Q (user):** cut session token burn (output-dominated) without quality loss.
> **Verdict:** output DOES dominate cost, but **~84% of output = REASONING** (redacted
> thinking), not markdown. Prose-compression ceiling ≈ **3%**. Real headroom = reasoning
> **OVERHEAD** (turn count), NOT depth. **Max-thinking stays ON** (user runs it deliberately
> for decomp/parity; depth is load-bearing). Lever = DIRECT thinking, batch turns, delegate.

Strategy ported from `/opt/src/openrecet` (commits f6400ba + b84cc29). This doc = OUR
measured numbers (the openrecet finding reproduced on this project's 193 transcripts ⇒ same
shape, so same levers apply). Tool: `tools/output_token_audit.py`.

## Method
- Parsed 193 transcripts (`~/.claude/projects/-opt-src-OpenSummoners/*.jsonl`) via
  `tools/output_token_audit.py` (reproducible; arg1 = projects-dir, arg2 = recent-window N).
- **Billed output = Σ `usage.output_tokens` DEDUPED by message `id`.** Trap: CC stores 1
  content-block/jsonl-line but stamps EACH line with the whole turn's usage → naive Σ
  multi-counts (~3-5×). Deduped true ≈ **29.4M**.
- **Thinking text REDACTED** (block present, content stripped) yet billed ⇒ thinking tok est
  = `billed − visible` (visible = measured text+tool_use chars → tok @ 3.5-4.0 c/t). Slight
  overcount of thinking (uncounted tool-JSON overhead); conclusion robust even @ 70%.

## Findings (193 sessions, 22,482 turns, 29.4M billed output tok) — BASELINE (pre-convention)
Billed OUTPUT distribution:

| bucket | all | recent-40 |
|---|---|---|
| **THINKING (reasoning)** | **83.7%** | **87.2%** |
| visible response prose | 2.6% | 2.5% |
| doc .md prose (Write/Edit) | 3.2% | 2.2% |
| **compressible prose total** | **5.8%** | **4.7%** |
| code files (.c/.h/…) | 5.0% | 3.4% |
| bash commands | 4.5% | 4.0% |
| misc tool args | 0.9% | 0.7% |

Where thinking goes (turn type):

| type | turns | %out | %row=thinking |
|---|---|---|---|
| mechanical (tools, ≤300ch prose, no write) | 14,654 | 38.8% | 87.3% |
| analysis/response (prose>300ch, no write) | 2,577 | 32.2% | 92.5% |
| authoring (Write/Edit) | 5,251 | 29.0% | 71.1% |

Turn shape:
- **92.7% of turns = exactly 1 tool call; avg 1.07 tools/turn.** (Worse than openrecet's
  89.8% ⇒ batching headroom is even bigger here.)
- Single-tool MECHANICAL turns = **61.0% of turns, 35.2% of output (10.3M tok)**, ~all thinking.
- Halving those (batch pairs) ⇒ ~17.6% of output upper bound (dependency chains can't batch).

Numbers track openrecet near-exactly (THINKING 83.7% vs 83.8%; single-tool-mech 61.0% vs
61.4%; batch upper-bound 17.6% vs 18%) ⇒ the finding generalizes; the levers are the same.

## Non-lossy basis (prose compression) — inherited from openrecet held-out test
openrecet's held-out test (2026-06-21): compressed a real RE-note slab **−47.9%** (all
hex/identifiers/code/paths VERBATIM); a fresh Sonnet sub-agent given ONLY the terse note
recovered ALL facts incl. RELATIONAL ones. ⇒ telegraphic RE-prose is non-lossy for
re-orientation. Cross-project applicable (same RE-prose genre); not re-run here.

## Levers (ranked) — what actually moves the 84%
1. **Fewer/fatter turns** — batch independent probes, front-load plans. Cuts reasoning
   OVERHEAD not DEPTH ⇒ lossless. Upper bound on single-tool-mechanical ~17.6% of output;
   real = a chunk (dependency chains can't batch). **Biggest lever** (92.7% single-tool here).
2. **Cheaper-model sub-agents for mechanical + search** (grep/measure/build/find) — same
   reasoning, ~5-12× cheaper/tok. Targets the 38.8% mechanical slice.
3. **Persist conclusions tersely** ⇒ future-me reads not re-derives (cross-session reasoning
   compression). The real payoff of terse docs.
4. **Terse output house-style** — ~3% direct, free, non-lossy. Adopt as default, not strategy.

**TRAPS (don't):** compress reasoning DEPTH / "think terse" — load-bearing for decomp/parity,
user runs max-thinking deliberately. headroom-style INPUT compression — input is not the cost
driver (though shrinking bulky tool outputs indirectly helps lever 1 by fitting more/turn).

## Adopted convention
`CLAUDE.md` → **"Output-efficiency (TERSE MODE)"** block: max-thinking ON (direct, don't cut);
terse prose (code/hex/ids VERBATIM) + batch turns + delegate mechanical/search + persist terse.

## A/B PROTOCOL (the user's ask: measure post-convention vs this baseline)
- **Baseline frozen here = 193 files / 22,482 turns as of 2026-06-21** (all pre-convention).
- The convention lands in commit adopting the CLAUDE.md block. Sessions AFTER that are the
  treatment group. (This setup session is meta/transition — exclude it; count work-sessions
  from the next.)
- To isolate the post-convention batch of N work-sessions: `nix develop --command python3
  tools/output_token_audit.py "" N` (recent-window = N). Compare its RECENT row vs this
  doc's ALL-SESSIONS baseline row.
- **Success = tools/turn ↑, single-tool-mechanical % ↓, prose-share ↓** at unchanged
  parity/RE quality. **Watch for quality loss** (missed batching dependency, over-terse doc
  that needs re-derivation) ⇒ if seen, REVERT.

## REVERT (if quality loss observed)
- Disable the WRITING style: `git revert <CLAUDE.md TERSE-MODE commit>` (or delete the marked
  block). The batching/delegation levers are behavioral — just stop applying them.
- This doc + `tools/output_token_audit.py` STAY (analysis infra, harmless).
- Re-measure effect anytime: `nix develop --command python3 tools/output_token_audit.py` —
  compare thinking% / tools-per-turn / prose-share before vs after.
