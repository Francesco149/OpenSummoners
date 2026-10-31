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

## Methodology (2026-10-14 rhost reset — `docs/audits/2026-10-14-rhost-methodology.md`)
**Verification foundation = rhost** (`tools/rhost/`, README): the user's retail `sotes.exe` runs
INSIDE our process on the Windows box, deterministically (flip-locked virtual clock, fake pump/
DInput/BGM clock, fixed heap/handles/stack, external calls on a system stack). Same trace ⇒ same
state hash; ~830–1000 fps; no Frida, no pins. Editions: `enold` (port target) + `ense-en`.
**The product = the standalone** (`docs/standalone.md`): `build/rhost/opensummoners-standalone.exe`
runs the port with NO retail code (data sections only); `rhost.exe --standalone` is its deterministic twin.
- **Porting loop (per function):** read `/mnt/ephemeral/opensummoners/decompiled-ida/enold/all_ida.c`
  (+ `all_ida.asm`, `functions_ida.tsv`, `data_xrefs.tsv`, `classes_ida.tsv`) → write the C port →
  wrapper + row in `tools/rhost/rh_hooks.c` → `rhost.py verify` (shadow, per call) + `rhost.py corpus
  --check --hook replace:NAME` (hybrid end state) → commit with the evidence line
  (`rhost shadow: N calls / K scenarios, 0 divergences; replace: corpus --check PASS`).
- **Gates:** `rhost.py det SCEN -n 3` (only `volatile.<edition>.txt` pointer slots may differ);
  `rhost.py corpus --check` (expected.json per `tests/scenarios/rh-*`; re-`--bless` only for an
  intended rhost change). Scenarios: DIK-level traces; humans record with `rhost.py record NAME` (`--from PREV`: continue PREV's play
  from its final save — re-derived from PREV's trace on every replay, nothing committed but the trace).
  **The hybrid gate** `corpus --check --hybrid` = every ported function REPLACING retail; on wine it also
  compares the state hash every 600 frames (`expected.wine.json` `sem_series`, `semv`) — transient
  divergences a room reload erases show there; `hookbisect SCEN --frames N` names the breaking hook.
  **The render gate** (on by default in `corpus --bless/--check`): `--fb-hash` chains a hash of every Flip's
  back buffer (`fb`, + `fb_series` on wine) — pure ≡ hybrid in PIXELS on every presented frame, the
  verification for render ports (REPLACE; a shadow can't re-blit). **The standalone gate** `corpus --check
  --standalone` = frame / tick / RNG / pixels vs PURE retail (state hashes: PORT-DEBT(crt-heap)).
  `rhost.py KNOWN_GAPS` = USER-accepted residuals (parity-ledger R9): hybrid / standalone checks compare such a
  scenario only before its gap frame — add one ONLY on a USER decision.
  Big functions: `tools/ida/decomp2c.py VA:name ...` first pass (keeps IDA's control flow / types / real class
  layouts, stack locals at their ebp offsets, ecx/edx call-site fixes; unported callees as retail's own `RC_`,
  Win32 calls through retail's import slots `API_x`; `--typed` keeps the KB structs from `src/` headers) → review
  → wire → verify. Only port functions in rhost-linked modules (`tools/rhost/build.sh` MODS) are bound by name.
  **The IDA export must be decompiled AFTER class recovery with a cleared cfunc cache** (`oss_ida_export.py`,
  2026-10-16): before, ~19.7k lines showed callers of now-thiscall methods WITHOUT their `this`, and decomp2c's
  "caller's this" guess was wrong in places (e.g. `beh_47adb0(c, …)` vs retail's `ecx = v43`). Re-export with
  `ida.py export --kb --fresh` after touching the exporter.
- **Probes:** `--watch-write VA@FRAME` (who writes an address), `--trap VA` (registers; `--trap-mem
  REG±OFF:LEN` dumps memory at each hit), `--shadow-from F` (shadows only from flip F), `coverage` (first-hit
  over all IDA functions; `--hybrid`: what retail code still runs), `--dump-at` + `rhs.py` (state diffs),
  `--shot-at` / `--shot-every` (frames as images).
- **Windows:** `ssh win` + task `oss-rhost-runner` + `C:\oss-rhost\` — separate from openrecet's
  `orc-rhost-runner`/`C:\orc-rhost\` (verified running simultaneously); the binary is `oss-rhost.exe` there
  (openrecet kills `rhost.exe` by name — never kill anything by exe NAME on that box). Wine (`--target wine`,
  local headless) = the logic oracle. IDA 9.4 headless: `python3 tools/ida/ida.py export [--kb] [--fresh]`
  (`C:\oss-ida\`).
- **Removed (USER, 2026-10-31):** the pre-rhost loop — the from-scratch front end `opensummoners.exe`
  (`src/main.c` + its title / newgame / prologue / town drive modules + their host tests), the Frida retail
  drive + live-probe MCP, the Ghidra export tooling, the diff lenses (render / flow / tas / call_trace diff),
  Trace Studio v1 / v2 + the capture proxy, the WSL launcher. Last commit with them: `57b8ccb`. What they
  found about retail stays in `docs/findings/`; don't resurrect them — extend rhost instead.

## How we work here (conventions)
- **Output-efficiency (TERSE MODE — added 2026-06-21; REVERTIBLE: `git revert` the commit or
  delete this bullet). Lever = DIRECT max-thinking, never CUT it.** Max-thinking stays ON
  (decomp/parity needs deepest reasoning — user policy); reasoning depth is load-bearing. Session
  output cost is ~84% reasoning, visible prose only ~6% ⇒ cut reasoning OVERHEAD + output tokens,
  NOT depth. Data + revert + A/B proc: `docs/audits/2026-06-21-output-efficiency.md`; audit:
  `tools/output_token_audit.py` (re-measure before/after to judge quality loss).
  1. **Write ALL prose terse** (responses + docs/findings/journal/commits): telegraphic — drop
     articles/copulas/hedges/filler, fragments, symbols (→ ⇒ ∧ ¬ @ ==). **VERBATIM:** code, hex
     (FUN_/DAT_/0x…), identifiers, paths, numbers, gate exprs, tables. Non-lossy (openrecet
     held-out test 2026-06-21: −48% chars, fresh agent recovered all facts incl. relational).
  2. **Batch independent probes into ONE turn; front-load plans** (baseline 1.07 tool-calls/turn,
     92.7% single-tool ⇒ ~14k single-tool-mechanical turns of wasted re-orientation preambles).
     Don't serialize independent reads/greps/builds. The real ~10-18% lever (here ~17.6% upper bound).
  3. **Delegate MECHANICAL + SEARCH to a Sonnet/Haiku sub-agent** (grep sweeps, measurements,
     build/test runs, file-finding) — same reasoning ~5-12× cheaper/tok; reserve Opus
     max-thinking for decomp/parity. (Squares with the existing subagent-judiciously rule below.)
  4. **Persist conclusions tersely** so future-me READS not RE-DERIVES (cross-session
     reasoning-compression — the real payoff of terse docs).
- **Knowledge in the repo; status is derived.** Durable knowledge → `docs/`. Live status
  is derived: `docs/FRONT.md`→`STATUS.md`, and `docs/port-ledger.{md,json}` +
  `docs/port-frontier.md`. After anything with a `FUN_<va>` provenance comment, regen:
  `nix develop --command python3 tools/gen_port_ledger.py && nix develop --command python3 tools/gen_frontier.py`.
  Don't hand-track status in prose. **Ledger footgun:** the generator counts *any*
  `FUN_<va>` token in `src/` as a "ported" signal (engine-proper boundary `0x5bdab0`;
  `--check` exits 3 if stale). So when a comment must name an **unported** callee, write
  it as a **bare VA** (`0x412c10`), never `FUN_00412c10`, or it falsely inflates the
  touched count. Use the `FUN_<va>` form only for functions actually ported.
- **Bit-exact is the bar — never hand-wave a diff.** Confirmed = rhost says so: shadow 0 divergences,
  `corpus --check` PASS (frame / tick / RNG / state / every frame's pixels). Any residual is an OPEN item
  with concrete hypotheses in `docs/parity-ledger.md`, never "close enough"; it becomes a KNOWN_GAPS entry
  only by USER decision. **RE the responsible code + probe retail; don't eyeball or curve-fit a constant**
  — name the diverging function (`verify` / `hookbisect` / `--shadow-from`), read the algorithm in the
  decompile, port *that logic*, re-gate.
- **NEVER ship an approximation — this is a faithful port, DRAWCALL PER DRAWCALL (USER,
  ckpt 134).** A linear-fit curve, a guessed z-order, a "reasonable" rate — all forbidden. Retail's own
  code is the ground truth and it runs in rhost: reproduce its exact values, order and state. **If a probe
  can't surface the exact truth, the TOOL is the gap: IMPROVE rhost's probes until it can** — "not cleanly
  probeable" is NEVER license to approximate, curve-fit, or guess. **And LOOK at it yourself before
  reporting:** a frame or animation you did not actually inspect (shots / state dumps) is NOT done.
- **Attribute every divergence to a PILLAR before suspecting logic.** (1) **logic/data→
  output** (the pure contract we port), (2) **phase** (load-dependent counter/anim origin),
  (3) **RNG** (LCG `FUN_005bf505`/`DAT_008a4f94` consumption order), (4) **upstream inputs**
  (fix frame 0 forward), (5) **undefined data** retail reads (uninitialized stack / heap: quirks
  #130, #182 — rhost's neutralization sites). rhost pins phase + RNG + inputs by construction (same
  trace, `--seed`); the hybrid / standalone diverging from pure = a real bug or pillar 5. Full model:
  **`docs/parity-model.md`**.
- **Full port, not MVP.** Tag every synthetic/MVP shortcut with a `PORT-DEBT(tag, …)`
  comment + a row in **`docs/port-debt.md`**. Retire them; don't let a fake silently cap
  parity. "Deferred" prose belongs in that registry, not buried in a finding.
- **Log engine quirks as you find them** in `docs/findings/engine-quirks.md` — **retail
  ground-truth behavior ONLY** (offsets/formats/control-flow/UI facts). NOT for our port: module
  names, host-test counts, ledger deltas, "deferred"/"stub" seams, gate reassurances — those go in
  FRONT/HANDOFF/port-debt/`TODO(...)` comments.
- **Trace the CODE to replicate it — don't measure-and-approximate (USER, ckpt 137).**
  "Don't guess" means: find the EXACT retail instructions that PRODUCE the behaviour — read the
  decompile + follow the execution in rhost (`--trap`, `--watch-write`, which gate fires, who writes
  which field) — and port THAT logic, so the timing/shape EMERGES. A measured value is for VERIFYING
  the ported logic or RESOLVING a decompile ambiguity — **NEVER a curve-fit constant shipped IN PLACE of
  the logic.** Sole exception: a value standing in for a not-yet-ported subsystem, clearly
  `PORT-DEBT`-tagged with a path to derive it.
- **Annotate as you RE — the IDA knowledge pack.** Names / roles: `tools/ida/retail_fields.json`
  (curated engine names + globals) and the port's own function names (provenance); thiscall rows
  `(addr, class, prototype)` in the TAGS array of `tools/ida/TagThiscallFunctions.java`; struct shapes in
  `src/*.h`. `tools/ida/gen_knowledge.py` folds them into the KB → `ida.py export --kb` re-decompiles with
  typed `this->field`. Plus the hook row's name in `rh_hooks.c` (what `verify` / `hookbisect` print).
- **Visuals: llm-feed UNWIRED (USER, 2026-10-14).** Don't push anywhere; write PNGs under `runs/` (or the
  scratchpad) from rhost shots, cite the path, LOOK at them yourself (Read tool). Never eog/explorer.
  USER visual review = playing the standalone (`tools/standalone/install-play.sh`: the desktop shortcut
  "OpenSummoners (standalone)").
- **Cross-reference, but verify at the byte level.** The Fortune Summoners Fan Discord's
  *SotES Data Formats & Values* spreadsheet (`docs/ods-crossref.md`) + the 2026-05-29
  subsystem survey (`docs/audit/subsystem-survey-2026-05-29.json`, quirks distilled in
  `engine-quirks.md` #15–#27) are *leads*, not ground truth — confirm exact offsets/VAs
  in the decompile + a gate before a port depends on them. When we 100%-prove something
  the sheet lacks, publish a human-verifiable proof under `docs/proofs/`.
- **Builds:** rhost's `build.sh` compiles every TU in parallel, objects cached by a hash of the PREPROCESSED
  source + flags (no stale-dependency risk; story_scripts ~40 s, rh_hooks.c ~30 s cold, warm ~12 s). The host
  test suite (`tests/Makefile`) and res_explorer are their own makes.
- **Build EVERYTHING before you push — `bash tools/ci/build_all.sh` (MANDATORY).** `res_explorer` LINKS
  the port's own `src/*.c` (the decoders: bitmap_session, map_*, asset_register, actor_*, party, …), so a new
  `src/` dependency silently DRIFTS it — `party.c` gaining a `base_stat_table.c` dep broke the `res_explorer`
  link *after* a push. `build_all.sh` supersets `ci.yml` (standalone + res_explorer + ennse_voice + host suite
  + asset scan) + the save inspector and trainer. All mingw, no game assets, no Windows. Green there ⇒ green
  in CI.
- **Releases:** CI ships the standalone, res_explorer, the voice patch (`ci.yml` / `nightly.yml` /
  `release.yml`). The rolling `nightly` pre-release is MANUAL (`gh workflow run nightly.yml`, when enough
  changed; USER 2026-10-31) — never re-add a cron. Download links in README / `tools/ennse_voice/` point at it.
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
- **Everything runs inside `nix develop`** — `python3`/`i686-w64-mingw32-gcc`/`make` are
  flake-provided; `command not found` ⇒ you forgot the dev shell (the #1 recurring footgun).
  Wrap as `nix develop --command <cmd>` (its banner goes to stdout: don't pipe its output into a file
  you then execute).
- **Session hygiene — suggest `/clear` at milestone breakpoints + orient fast.** When a
  self-contained arc lands and the next is fresh scope, offer a `/clear` (large decompile
  reads bloat context). Orientation path: this file → `FRONT.md` → "Where to read next".

## Run / build (host tools need the `nix develop --command` prefix)
- **Build:** `nix develop --command bash tools/rhost/build.sh` → `build/rhost/rhost.exe` (the host) +
  `build/rhost/opensummoners-standalone.exe` (the product). **Tests:** `nix develop --command make -C tests
  run` (host unit suite, ASan/UBSan; currently **363 pass / 0 fail / 2 skip** — run before committing C).
- **Disassembly:** the IDA 9.4 export `/mnt/ephemeral/opensummoners/decompiled-ida/
  <edition>/all_ida.c` (knowledge pack + C++ class recovery applied; `plain/` = untouched IDA),
  regenerate with `python3 tools/ida/ida.py export --kb`. Cite by address (IDA names keep the VA
  suffix). **IDA pointer-typing trap:** an int IDA typed as `_DWORD *` makes `v86 += 6000` mean +24000
  in the asm (`char_physics` boost bug, 2026-10-16) — check `+=`/`-=` on pointer-typed locals (esp.
  `vN = (_DWORD *)this->field_X`) against the asm. Edition VA mapping: `tools/ida/crosswalk.py`.
  Never commit decompiled C.
- **Play / look:** `rhost.py run SCEN --show` (a visible replay), `--shot-at` / `--shot-every` (images);
  the standalone on the Windows box via `tools/standalone/install-play.sh`; the no-retail-code proof
  `tools/standalone/prove_no_retail_code.py`.

## The binary & paths
- **Game install:** `/mnt/c/Program Files (x86)/Steam/steamapps/common/Fortune Summoners/`.
  `sotes.exe` (32-bit PE, MSVC ~2012, **Steam-DRM `.bind` section** → unpacked by
  Steamless to `vendor/unpacked/sotes.unpacked.exe`; `tools/setup.sh`). Companion DLLs are oversized →
  embedded assets: `sotesd.dll` (168 MB), `sotesw.dll` (82 MB, BGM/WMA), `sotesp.dll`
  (1.1 MB); plus `lizsoft.spl`. Imports KERNEL32/USER32/GDI32/ole32/**DDRAW**/**DSOUND**/
  **DINPUT**/WINMM → **DirectDraw 7** software-blit renderer + DirectSound + DirectInput.
  rhost's copies: `C:\oss-rhost\game\<edition>\` (Windows), `/mnt/ephemeral/opensummoners/game/` (wine).
- **Devtools** (`/mnt/c/Users/headpats/Documents/_devtools/`): `Steamless.v3.1.0.5…/
  Steamless.CLI.exe` (DRM unpack, runs via WSLInterop, no wine).
- **Sibling RE projects** (read-only references, hard-earned conventions):
  `/opt/src/openrecet` (Recettear — the reorg this project's rigor is modeled on; its rhost is this one's
  model), `/opt/src/OpenMare` (Patrician III — closest workflow shape).
- **SotES mod tooling — SAME `sotes.exe`, a cross-reference for port RE.** The author's
  standalone **mod loader** (proxy `version.dll` + LuaJIT + native-mod C ABI) lives in
  `../sotes-mod-loader` (orient from ITS `docs/HANDOFF.md`; sessions on it START there), with
  the author's mods + registry in `../sotes-mods`; the injected EN-SE **trainer** is
  `tools/sotes_trainer` (RE map `SE_CODE_MAP.md`; MCP `sotes_trainer` in `.mcp.json`). All RE the SAME
  binary family, so findings transfer BOTH ways — cite them when a VA/struct is already reversed: safepoint
  `0x437c70`, kb_poll `0x5e2a10` (kbd dev `*(0x92d5bc)`), main-window class
  `CLASS_LIZSOFT_SOTES`, launcher `#32770` (Launch 10003), roster/coordinates structs, base-stat
  table (`FUN_00426fd0`). The loader's `mod.mem`/`mod.game` + hook registry = a live RE probe on
  the real game (Lua or native), complementary to rhost.

## Where to read next (by need)
- **Is FUN_x ported / coverage:** `docs/STATUS.md` + `docs/port-ledger.{md,json}` (derived);
  `rhost.py coverage` for what the corpus reaches.
- **What's confirmed / open residuals:** `docs/parity-ledger.md`.
- **Why a run diverges / how to chase it:** `docs/parity-model.md` (the pillars) + `tools/rhost/README.md`
  (the host, probes, gates, recipes).
- **The standalone (product, proof, real-platform notes):** `docs/standalone.md`.
- **Changelog:** `docs/PROGRESS.md`. **RE writeups:** `docs/findings/INDEX.md`.
- **What to port next:** `docs/port-frontier.md` (mechanical next chip) +
  `docs/ROADMAP.md` (semantic milestone order + subsystem map).
- **Synthetic shortcuts owed back:** `docs/port-debt.md`.
- **Orchestration / when to spawn subagents:** `docs/AGENT-WORKFLOW.md`.
- **External RE cross-reference + community proofs:** `docs/ods-crossref.md`,
  `docs/proofs/`. **Strategic frame:** `docs/PLAN.md`.
