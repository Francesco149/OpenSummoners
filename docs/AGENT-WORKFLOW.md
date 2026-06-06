# Agent workflow — autonomy conventions

How Claude (the model running this RE project) should structure its work.
Read at the start of every new session.

## Session lifecycle (do this every session)

**At the start:**

1. Read `docs/memories/HANDOFF.md` — "where to pick up *right now*".
2. Glance at `docs/STATUS.md` for the headline coverage % and current front.
3. Skim this file's TL;DR.
4. `docs/port-frontier.md` answers "what's the next mechanical chip";
   `docs/ROADMAP.md` answers "what's the next *semantic* milestone".

**At the end of a meaningful checkpoint:**

1. Commit in a logical unit (co-author trailer, no `git add -A`).
2. **Regenerate the derived progress artifacts** if you ported anything:
   `python3 tools/gen_port_ledger.py && python3 tools/gen_frontier.py`
   (this refreshes `STATUS.md`, `port-ledger.{md,json}`, `port-frontier.md`).
   The signal is the `FUN_<va>` provenance comment in your src — keep it.
3. **Append any engine quirk you found** to `findings/engine-quirks.md`
   (see "Note engine quirks" below) — even a one-paragraph hunch.
4. Rewrite `docs/memories/HANDOFF.md` for the next session.
5. **Suggest a `/clear` point** if the milestone is a clean unit and the
   next is fresh scope (see "Suggest `/clear`" below). The durable memory
   is the docs, not the context; large Ghidra reads bloat the window.

## TL;DR

- **Stay autonomous.**  The user expects to be a periodic
  visual-confirmation checkpoint, not a per-step approver.  Make choices
  and report them.
- **Subagents are allowed — but used judiciously** (same rules as
  openrecet, 2026-06-05).  The deciding question is openrecet's: **does
  this require judgment or just careful execution?**  Judgment → main
  loop (Opus orchestrator, continuous context).  Wide, independent,
  read-only execution → a subagent earns its keep.  Two real costs gate
  every spawn: the **debrief/round-trip cost**, and the fact that a
  subagent **lacks your full reasoning history** (it pattern-matches
  shallowly and can miss a quirk).  So **don't be trigger-happy**: when
  you already hold the thread, do it inline even if it's long.
  - ✅ **Good fits:** wide read-only audits (map the binary into
    subsystems, scout many forward-path clusters), scan `all.c` for a
    pattern, run a smoke run + summarize a diff, pull N facts from a
    sister project.  `tools/workflows/subsystem-survey.js` is the
    canonical `Workflow` of read-only `Explore` agents (seeds
    `ROADMAP.md` + `engine-quirks.md`); its output is *decompile-grade* —
    byte-verify offsets before a port leans on them.
  - ❌ **Bad fits (stay in the main loop):** choosing what to port next,
    deciding real-bug-vs-harness-quirk, cross-subsystem synthesis,
    unscoped "find anything interesting", anything touching `vendor/` or
    committing.  The routine chip cadence (read Ghidra → write C → test →
    commit) runs single-threaded so the orchestrator holds context.
  - **These rules are LIVING — update them as you go.**  When a subagent
    pattern works well or burns you, edit this section (and the
    `feedback_autonomy` memory) so the lesson sticks.  Don't let a
    good/bad pattern go unrecorded.
- **Everything runs inside `nix develop`.**  `python3`, `frida`,
  `ghidra-analyzeHeadless`, `i686-w64-mingw32-gcc` are all flake-provided;
  outside the dev shell they will not be on PATH.  This is the #1
  recurring footgun across sibling RE projects — when `command not
  found` appears, the first hypothesis is "I forgot the dev shell".
- **Single-TU compile, full rebuild every time.**  No `.o`/`.d` files.
  C compiles fast enough.  Don't add intermediate build artifacts.
- **Lean on the harness for ground truth.**  When the decompiled output
  is ambiguous, write a Frida hook to log the actual values at runtime.
  When two implementations seem equivalent, frame-diff a side-by-side
  scenario.  Don't guess.

## Stop-and-report points

The orchestrator should pause when:

- A planning decision is needed (license, scope, format choice).
- A risky action is about to happen (git commit, force-push, anything
  destructive to `vendor/`).
- A subsystem milestone is complete and the next milestone is a fresh
  scope worth talking about.
- The harness asks for visual confirmation (a side-by-side image is
  generated; relay the path to the user).

These are *natural* stop points — the user doesn't have to interrupt;
the orchestrator pauses on its own and reports.

### Suggest `/clear` at natural milestones

The `PROGRESS.md` log + findings docs are the durable memory of this
project; the conversation context is a working set that gets *bloated*
by every file read of large Ghidra dumps (`docs/decompiled/all.c` can
easily exceed 10 MB).  **At every natural stop-and-report point, suggest
the user run `/clear`** if the just-finished milestone was a clean unit
and the next milestone is a fresh scope.

The right phrasing is short:

> Good stop point — this milestone is committed and documented in
> PROGRESS.md / `docs/findings/<file>.md`.  Consider `/clear` before
> starting the next subsystem so the context doesn't carry the Ghidra
> reads we just did.

Skip the `/clear` suggestion when:

- The current task is mid-flight (a hook is half-implemented, a test is
  failing, a finding is partial).
- The next planned step depends specifically on a fact just learned this
  session that hasn't yet been committed to a doc.

## Commits

**Commit in logical units as you go**, don't let the working tree pile
up.  A good unit is "one subsystem ported", "one extractor + its format
spec", "one harness fix + the test that proves it", "one finding
documented with its supporting Ghidra reads".  When in doubt, commit at
every natural stop-and-report point.

Conventions:

- Use the user's global git identity (`headpats`).  Never
  `git config --local user.email` something else.
- **Every commit Claude makes must include a co-author trailer**:

      Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>

  Use a HEREDOC to pass the message so the trailer lands on its own line.
- Commit only what was asked.  Never `git add -A` and sweep in stray
  files (we do this every session; stay disciplined).
- Never `git push` unless the user explicitly asks.
- Never amend (`--amend`) or rebase (`-i`) without explicit request — a
  new commit is almost always safer.
- Never bypass hooks (`--no-verify`) without explicit request — fix the
  underlying issue.

## Note engine quirks as you find them

When porting a function or chasing a bug, you will routinely stumble on
small things the original engine does that are weird, charming, or
inexplicable — non-standard hash variants, hand-rolled obfuscation,
hard-coded retry loops, deliberate-looking buffer over-allocations,
load-bearing-by-accident behavior, etc.

**Append a short entry to `docs/findings/engine-quirks.md`** before
moving on.  Even one paragraph.  Even if you're not sure it's
interesting — it almost certainly is once you write it down.

Why this lives here, not in code comments: each quirk is a one-paragraph
story about *why* the engine does the weird thing, and that story rots
quickly if scattered across `// note:` comments next to faithful ports.
Collecting them in one file gives the user a fun read, signposts future
debugging ("oh right, this is the function with the
subtract-instead-of-XOR hash"), and lets you cite a single anchor in
commit messages instead of restating the quirk every time it's relevant.

Format: numbered entry, 1–3 short paragraphs, optional code snippet, and
a `> 📍` pointer to the source location (Ghidra address, our port file,
or the relevant `docs/findings/*.md` section).

## Findings docs

Per-subsystem findings live in `docs/findings/<subsystem>.md`.  Each doc
captures the **why** behind our implementation choices: which Ghidra
addresses cover which behavior, what the calling convention is, which
struct field is what offset, where retail's quirks influence our port.

Don't worry about being polished — a findings doc that drifts from the
code as we port is *less* useful than no doc; bias toward keeping these
fresh, even at the cost of being terse.

## How to use the Frida harness

`tools/frida_capture.py` is the canonical entry point.  It accepts:

- `--hide-window` / `--show-window` — almost always hidden; only show
  when the user wants to watch the screen.
- `--turbo` / `--no-turbo` — almost always on; off only if you need
  wall-clock-accurate audio (we have `--silent-audio` for everything
  else).
- `--silent-audio` / `--no-silent-audio` — on by default; off if you're
  debugging audio.
- `--show-msgbox` — disable the agent's MessageBox redirect to see real
  popups (only when debugging the harness itself).
- `--turbo-step-ms N` — advance the virtual clock by N ms per main-loop
  tick.  Default 17 ≈ 60 Hz; adjust once Ghidra resolves the engine's
  actual frame cadence.
- `--max-frames N` / `--duration-ms N` — caps to keep smoke runs bounded.

When you add a new hook to `tools/frida/opensummoners-agent.js`, expose
it via the `init({...})` RPC and add a matching CLI flag to
`frida_capture.py`.  Keep the surface symmetric.

## Reading the decompiled output

`tools/ghidra-headless.sh` emits:

- `docs/decompiled/all.c` — flat file with every function.
- `docs/decompiled/by-address/<addr>.c` — one file per function by entry.
- `docs/decompiled/by-name/<name>.c` — same, by Ghidra-resolved name.
- `docs/decompiled/functions.csv` — index (name, addr, size, thunk, cc).

When citing Ghidra functions in findings docs, use the
`docs/decompiled/by-address/<addr>.c` form — names rename over time but
addresses are stable until we re-import.

For fast iteration: edit a function's name in Ghidra GUI on the same
project, then re-run `ghidra-headless.sh` with the project already
present (it skips re-analysis and just regenerates the C tree, ~seconds).

## Build outputs

`make -C src` produces **two** PE binaries from the same `.c` set:

- `build/opensummoners.exe` — GUI subsystem (`-mwindows`).  The
  shippable binary.  No console pops up on launch; `fprintf` to
  stdout/stderr is silently dropped (Windows GUI subsystem doesn't wire
  stdio).
- `build/opensummoners-debug.exe` — console subsystem.  Same code,
  different PE header.  Windows hooks stdin/stdout/stderr to the
  launching shell's TTY, so any `fprintf` line surfaces in real time.

**Prefer the debug binary for interactive runs** when you want to read
log lines.  The shippable binary is fine for the harness since it pipes
stdio explicitly via Popen / Frida.

## Persistent analysis tooling

Inline one-off Python (PE-string lookups, VA → file-offset conversions,
fixed-table dumps from the exe) lives at `tools/analyze/`.  Grow it
instead of pasting throwaway scripts; one-purpose-per-file is fine.

### Before adding a permanent ad-hoc flag, fit it into the existing unified systems

When you reach for a new harness probe, **first ask whether it belongs in a
system we already have** — the TAS/seed-pin harness, the **flow trace**
(`retail_fields.json` field spec + the agent's `src:` field sources), the
call-trace, `mem_watch`, the sim-tick anchor.  A bespoke `--foo-probe` flag is
debt: it needs its own plumbing (CLI arg → cfg → agent opt → bespoke send/print),
doesn't compose with `flow_diff`, and rots once the question is answered (the repo
already carries a graveyard of `--cursor-probe`/`--fade-probe`/`--pace-probe`/…).
The flow trace is the unifier: a new datum is usually just a `fields[]` entry on
the right VA with the right `src` (`global`/`arg`/`argderef`/`chain`/`rngcalls`),
and if a new *kind* of datum is needed, add a `src:` type to the agent's
`ctReadField` (one place) so EVERY VA can read it and `flow_diff` classifies it for
free.  Example (ckpt 73→74): RNG-consumption tracking became `src:"rngcalls"` (the
cumulative LCG-draw count, auto-installing the single `0x5bf505` hook) rather than
a `--rng-count` flag — modelled on openrecet's identical `rngcalls` source.  Reach
for a standalone flag only when the thing genuinely can't be expressed as a
per-VA/per-frame field (e.g. an interactive recorder).

## Always run inside `nix develop`

If you find yourself typing `python3 …` and getting `command not found`
or the wrong version, you forgot to enter the dev shell.  Wrap with:

    nix develop --command python3 tools/whatever.py …

…or prefix the whole session: `nix develop` once, then run tools
normally.  This is the #1 most common failure across sibling RE
projects — call it out aggressively in your own notes.

## Bringing up frida-server

The Windows-side `frida-server.exe` runs on the host reachable as
`cutestation.soy:27042` (the Windows machine's LAN-resolvable name).
**Do not default to `127.0.0.1:27042`** — WSL2's NAT doesn't loop back
to the Windows host's localhost binding, so the LAN hostname is the
only reliable path from inside WSL.  Override with
`$OPENSUMMONERS_FRIDA_REMOTE` if you ever need a different host/port.

`tools/frida_capture.py` will auto-spawn `frida-server.exe` via
`Start-Process -Verb runAs` if the TCP port isn't already open and the
exe is reachable locally.  In the common case the server is already
running and the script just connects.

If you need to start it manually on the Windows side:

    powershell.exe Start-Process -Verb runAs \
      -FilePath C:\Users\headpats\Documents\_devtools\frida-server-…\frida-server-…-windows-x86_64.exe \
      -ArgumentList '-l','0.0.0.0:27042'
