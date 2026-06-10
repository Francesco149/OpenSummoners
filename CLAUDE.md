# OpenSummoners — Claude entry point

C reimplementation of **Fortune Summoners: Secret of the Elemental Stone** (Lizsoft,
2008 JP / Carpe Fulgur, 2012 EN Steam) as a drop-in replacement for `sotes.exe`. Goal:
**full structural parity with the retail engine, matched 1:1 frame by frame** — not an
MVP. Legal line: **never redistribute game assets / decompiled binary code** (the user
owns the game; the port reads the user's own files / extracts at runtime). Not
byte-identical — MSVC-era CRT codegen is a goose chase; correctness is proven
**behaviorally** against the original exe (`docs/PLAN.md` §3).

This file is dense on purpose and auto-loads every session. It is the **single durable
source of orientation**: all process knowledge that used to live in `~/.claude`
auto-memory is consolidated here, so even if the uncommitted `.claude` dir is lost you
can orient from the repo alone. A fresh session needs only this file → `docs/FRONT.md`
to be productive in ~60 seconds. **The repo is the source of truth.**

## Current front
→ **`docs/FRONT.md`** — the ONE hand-edited "what we're doing right now" block (a
60-second read), injected verbatim into `docs/STATUS.md` by `tools/gen_port_ledger.py`
so status can never drift. `docs/memories/HANDOFF.md` is the rolling current-checkpoint
detail (module layout + open RE threads); `docs/PROGRESS.md` is the append-only
changelog. Active multi-session plans: `docs/plans/`.

## How we work here (conventions)
- **Knowledge in the repo; status is derived.** Durable knowledge → `docs/`. Live status
  is derived: `docs/FRONT.md`→`STATUS.md`, and `docs/port-ledger.{md,json}` +
  `docs/port-frontier.md`. After anything with a `FUN_<va>` provenance comment, regen:
  `nix develop --command python3 tools/gen_port_ledger.py && nix develop --command python3 tools/gen_frontier.py`.
  Don't hand-track status in prose. **Ledger footgun:** the generator counts *any*
  `FUN_<va>` token in `src/` as a "ported" signal (engine-proper boundary `0x5bdab0`;
  `--check` exits 3 if stale). So when a comment must name an **unported** callee, write
  it as a **bare VA** (`0x412c10`), never `FUN_00412c10`, or it falsely inflates the
  touched count. Use the `FUN_<va>` form only for functions actually ported.
- **Bit-exact is the bar — never hand-wave a diff.** A frame is "confirmed 1:1" only
  when `differ_px == 0` (or the residual is a *named, understood* delta). Any non-zero
  residual is an OPEN item with concrete hypotheses in `docs/parity-ledger.md`, never
  "close enough". **RE the responsible code + probe retail; don't eyeball or curve-fit a
  constant** — pinpoint the divergence with the diff lenses (`render_diff` = which draw,
  `flow_diff` = which logic; add a `retail_fields.json` field, not a bespoke probe flag),
  read the algorithm in the decompile, port *that logic*, then validate bit-exact at
  matching animation phase. Standing accepted residuals:
  documented benign deviations + occasional ≤1-LSB texture-sampling noise only.
- **Attribute every divergence to a PILLAR before suspecting logic.** (1) **logic/data→
  output** (the pure contract we port), (2) **phase** (load-dependent counter/anim origin
  — e.g. the title render-rate ×2.2 + flip-skew, `parity-ledger.md` R3), (3) **RNG** (LCG
  `FUN_005bf505`/`DAT_008a4f94` consumption order), (4) **upstream inputs** (fix frame 0
  forward). Pin phase+RNG+inputs, THEN compare; record data-1:1 vs observed-1:1. Full
  model: **`docs/parity-model.md`**.
- **Pin the RNG seed on both sides; compare by anchor/tick, not flip index.** Port boots
  fixed **`OSS_RNG_DEFAULT_SEED = 0x4f5347`** (`src/rng.h`; env `OPENSUMMONERS_RNG_SEED`
  overrides); retail harness **`--seed-pin`** (default ON) writes the same value into
  `DAT_008a4f94` (`srand` `FUN_005bf4fb`, seeded `srand(time())` at boot `0x56227a`, so
  retail's stream is wall-clock-random unless pinned). Both emit named anchors at
  scene/phase boundaries (e.g. `subtitle_anim_start` = first `0x56c070`); align on an
  anchor, march tick-for-tick. Deviation under a pinned seed is a real bug.
- **Full port, not MVP.** Tag every synthetic/MVP shortcut with a `PORT-DEBT(tag, …)`
  comment + a row in **`docs/port-debt.md`**. Retire them; don't let a fake silently cap
  parity. "Deferred" prose belongs in that registry, not buried in a finding.
- **Log engine quirks as you find them** in `docs/findings/engine-quirks.md` — **retail
  ground-truth behavior ONLY** (offsets/formats/control-flow/UI facts read off goldens).
  NOT for our port: drive/module names, host-test counts, ledger deltas, "deferred"/
  "stub" seams, capture reassurances — those go in FRONT/HANDOFF/port-debt/`TODO(...)`
  comments.
- **Lean on the harness for ground truth — don't guess.** When the decompile is
  ambiguous, Frida-hook the real values; when two impls seem equivalent, frame-diff a
  side-by-side. The Frida host is always up + **UAC auto-approved**, so the live gates
  (`frida_capture.py`, `bisect_call_trace_vas.py`, `mem_watch.py`) are
  **self-serviceable** — not blocked on the human.
- **Annotate as you RE — two SEPARATE durable practices; never invent a third.** When you
  reverse a function/global, record it in BOTH lanes (not an ad-hoc symbol-rename / parallel
  names DB — that's the outdated trap):
  (1) **Trace annotation** (execution tracing — *the* meaning of "annotate" here, the
  openrecet standard): give the VA a **named entry + named fields** in
  `tools/flow/retail_fields.json` (`name` + `_note` role + the payload it reads:
  `global`/`arg`/`argderef`/`chain`), mirrored port-side by `CALL_TRACE_BEGIN(va)` +
  `CALL_TRACE_FIELD` in the matching `src/` module. Then `tools/flow_diff.py` can name the
  first call whose inputs matched but whose output/field **diverged** — and coverage
  **compounds**: every annotated function makes the next divergence faster to pinpoint. Do
  this as a core step of finishing any RE/port, not an afterthought.
  (2) **thiscall/struct tagging** (static analysis — decompile *readability* only, unrelated
  to tracing): add `(addr, class, prototype)` rows to
  `tools/ghidra-scripts/TagThiscallFunctions.java` + struct shapes to `src/*.h`, re-applied
  by `tools/ghidra-tag-and-export.sh` so `docs/decompiled/*.c` read with typed `this->field`.
- **Show visuals on the llm-feed** (`/opt/src/llm-feed/feed.py`, :8777;
  `curl -s localhost:8777/healthz` → `ok`, start it if down). Push as you go, then keep
  working — fire-and-forget, not a checkpoint:
  `python3 /opt/src/llm-feed/feed.py image PATH --title T --note N`, or the
  **`comparison`** subcommand for port|retail amplified diffs (montage also available).
  Never eog/explorer. Port frames: `--capture-frames "60,200,…"` → BMPs in the game dir
  → convert BMP→PNG (PIL, in `nix develop`) → push.
- **Cross-reference, but verify at the byte level.** The Fortune Summoners Fan Discord's
  *SotES Data Formats & Values* spreadsheet (`docs/ods-crossref.md`) + the 2026-05-29
  subsystem survey (`docs/audit/subsystem-survey-2026-05-29.json`, quirks distilled in
  `engine-quirks.md` #15–#27) are *leads*, not ground truth — confirm exact offsets/VAs
  in the decompile + a test before a port depends on them. When we 100%-prove something
  the sheet lacks, publish a human-verifiable proof under `docs/proofs/`.
- **Single-TU compile, full rebuild every time** — one `gcc` per output binary, all `.c`
  passed directly, no `.o`/`.d`. C compiles sub-second here; the simplicity is free.
- **Commits: commit in logical units as you go, without waiting to be asked.** Direct to
  `master` (no per-feature branches — they fight the linear checkpoint workflow); one
  chip/feature per commit; build + test first. There are **no git hooks** here, so add
  the trailer by HEREDOC and end the message with it:
  `Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`. Regenerate the
  derived ledger in the same commit when you ported anything. No `git add -A`. **Push**
  only when asked.
- **Subagents are allowed but used judiciously** — wide read-only audits yes; judgment /
  bug-chasing / porting in the main loop. Weigh the debrief cost + the subagent's missing
  context. The rules are LIVING — update `docs/AGENT-WORKFLOW.md` when a pattern helps or
  burns you.
- **Everything runs inside `nix develop`** — `python3`/`frida`/`ghidra-analyzeHeadless`/
  `i686-w64-mingw32-gcc`/`make` are flake-provided; `command not found` ⇒ you forgot the
  dev shell (the #1 recurring footgun). Wrap as `nix develop --command <cmd>`.
- **Session hygiene — suggest `/clear` at milestone breakpoints + orient fast.** When a
  self-contained arc lands and the next is fresh scope, offer a `/clear` (large Ghidra
  reads bloat context). Orientation path: this file → `FRONT.md` → "Where to read next".

## Run / build (host tools need the `nix develop --command` prefix)
- **Build:** `nix develop --command make -C src` (mingw32; **don't** override `CC`) →
  `build/opensummoners.exe` (+ `-debug.exe`, console build — prefer it for interactive
  runs so `fprintf` log lines surface). **Tests:** `nix develop --command make -C tests run`
  (host unit suite, ASan/UBSan; currently **806 pass / 0 fail / 6 skip** — run before
  committing C).
- **Disassembly:** read `docs/decompiled/by-address/<addr>.c` (cite by address — names
  rename, addresses are stable). Index: `docs/decompiled/functions.csv`. Regenerate via
  `tools/ghidra-headless.sh` (re-runs in ~seconds if the project already exists).
  Decompiled C is gitignored (derived from the user's binary).
- **TAS / parity harness (THE divergence loop):** for every divergence — **pinpoint →
  attribute to a pillar → fix the logic (RE it, don't curve-fit) → re-verify to
  `differ_px==0` → log the quirk + ledger row.** Three diff lenses, all over one
  seed-pinned, anchor-aligned `call_trace.jsonl` per side:
  - `tools/render_diff.py` — **the RENDER lens: names the wrong/missing BLIT and how**
    (`[sprite]`/`[decode]`/`[rect]`/`[state]`), keyed on the cross-side
    `(resource_id, frame)` identity + a decoded-sheet `dhash` (`src/render_id.{c,h}`;
    port emits at the 5 blit primitives via `zdd_emit_blit`, retail via the resolver
    `0x418470` registry). `findings/ddraw-blit-trace.md`.
  - `tools/flow_diff.py` — **the LOGIC lens: names the first call whose inputs matched
    but whose output/state diverged** (field-bearing `CALL_TRACE_BEGIN/FIELD/END` +
    `tools/flow/retail_fields.json`). render_diff says which draw is wrong; flow_diff
    says which logic produced it.
  - `tools/tas_diff.py` — anchor-aligned pixel diff (drift search); `call_trace_diff.py`
    — coarse which-fns-each-side-reached.
  Capture: `tools/frida_capture.py` (retail; `--seed-pin --lockstep` deterministic;
  `--field-spec[-only]` auto-hooks the spec VAs; `--hide-window --turbo --silent-audio`
  defaults, but live boots needing the message pump MUST be `--no-turbo`, quirk #29) /
  `run-retail.sh`. Port: `opensummoners.exe --call-trace <path> --call-trace-frames i,j`
  **run INSIDE `nix develop`** (else `sotesd.dll` won't load → blank), `--input-trace
  <repo-relative-path>` (paths are absolutized pre-chdir). **Add a new datum as a
  `retail_fields.json` field, NOT a bespoke `--foo-probe` flag** (`src:` global/arg/
  argderef/chain/rngcalls/renderid/thisderef; a new *kind* = one `src:` in the **Frida
  agent**'s `ctReadField`, `tools/frida/opensummoners-agent.js`). How-to:
  `docs/parity-harness.md`.
- **TRACE STUDIO — the scrub-and-mark review loop (ckpt 103, supersedes plan B1+C).**
  `tools/trace_studio.py capture <scenario>` drives BOTH targets, pairs the flip axes
  anchor-segmented (sticky ±drift), and builds a session (frames/videos/state/verdict);
  `serve` (:8779) scrubs port|retail|diff in lockstep where the **USER flags divergences
  as MARKS** → `worklist.md` is the hand-off Claude works from; `recapture --only port` =
  the fast fix loop. **Read the live session's worklist/edits before picking new work.**
  **Keep `serve` UP every session so the USER can review/mark at any time — start it if
  down** (the OpenSummoners studio is `:8779`; sibling projects hold 8777/8778): `nohup nix
  develop --command python3 tools/trace_studio.py serve --session <name> &` (most-recent
  session; `ls runs/trace-studio/`). Verify: `ss -tlnp | grep :8779`.
  **Chase marks on the SIM-TICK axis, never the flip axis** (ckpt 105): both sides'
  frames are `frame_<flip>_t<tick>` (tick = easer `0x43d1d0` call count; port mirror
  `g_sim_tick_count`), state/worklist carry both — tick mismatch at a mark = phase
  pillar, tick match with pixels differing = real logic. Flip-axis trigger readings
  absorb retail's coalesced ticks (±1-2 tick errors) and fade dt-probes plateau on the
  alpha-ramp quantization — calibrate fades off per-present VALUE sequences (quirk #99).
  How-to: `docs/trace-studio.md`; sessions in `runs/trace-studio/`.

## The binary & paths
- **Game install:** `/mnt/c/Program Files (x86)/Steam/steamapps/common/Fortune Summoners/`.
  `sotes.exe` (32-bit PE, MSVC ~2012, **Steam-DRM `.bind` section** → unpacked by
  Steamless to `vendor/unpacked/sotes.unpacked.exe`). Companion DLLs are oversized →
  embedded assets: `sotesd.dll` (168 MB), `sotesw.dll` (82 MB, BGM/WMA), `sotesp.dll`
  (1.1 MB); plus `lizsoft.spl`. Imports KERNEL32/USER32/GDI32/ole32/**DDRAW**/**DSOUND**/
  **DINPUT**/WINMM → **DirectDraw 7** software-blit renderer + DirectSound + DirectInput.
- **Devtools** (`/mnt/c/Users/headpats/Documents/_devtools/`): `Steamless.v3.1.0.5…/
  Steamless.CLI.exe` (DRM unpack, runs via WSLInterop, no wine), `frida-server-…-
  windows-x86_64.exe` (started via `Start-Process -Verb runAs`; same instance serves the
  sibling projects).
- **Frida host:** `cutestation.soy:27042` (Windows LAN name — **not** `127.0.0.1`; WSL2
  NAT doesn't loop back). Override via env `OPENSUMMONERS_FRIDA_REMOTE` → falls back to
  `OPENRECET_FRIDA_REMOTE` → `127.0.0.1:27042`.
- **Sibling RE projects** (read-only references, hard-earned conventions):
  `/opt/src/openrecet` (Recettear — the reorg this project's rigor is modeled on),
  `/opt/src/OpenMare` (Patrician III — closest workflow shape).

## Where to read next (by need)
- **Is FUN_x ported / coverage:** `docs/STATUS.md` + `docs/port-ledger.{md,json}` (derived).
- **What's confirmed pixel-1:1 (regression guard):** `docs/parity-ledger.md`.
- **Why a frame differs / how to chase it:** `docs/parity-model.md` (the pillars) +
  `docs/parity-harness.md` (the tools).
- **Which DRAW / d3d-state is wrong (vs which LOGIC):** `docs/findings/ddraw-blit-trace.md`
  (`render_diff` = the blit/state lens; `flow_diff` = the logic lens).
- **Changelog:** `docs/PROGRESS.md`. **RE writeups:** `docs/findings/INDEX.md`.
- **What to port next:** `docs/port-frontier.md` (mechanical next chip) +
  `docs/ROADMAP.md` (semantic milestone order + subsystem map).
- **Synthetic shortcuts owed back:** `docs/port-debt.md`.
- **Orchestration / when to spawn subagents:** `docs/AGENT-WORKFLOW.md`.
- **External RE cross-reference + community proofs:** `docs/ods-crossref.md`,
  `docs/proofs/`. **Strategic frame:** `docs/PLAN.md`.
