# Plan — structural-parity harness (call-graph diff + mem-watch + input injection)

Status: **OFFLINE FOUNDATION LANDED** (2026-05-29) — phases marked
`[offline]` below are built + tested; `[live]` phases are code-complete but
await the first retail-under-Frida run (the human-verification gate). See
`docs/parity-harness.md` for the run cheat-sheet and PROGRESS.md for the
landing entry. Mirrors the machinery in `../openrecet`
that "accelerated converging to the right rendering-path ports". Goal: be
able to drive **retail `sotes.exe`** and **our port** through the *same*
scenario and mechanically answer "what did retail call this frame that we
didn't?" — turning rendering-path parity from guesswork into a diff.

---

## 1. What openrecet built (studied 2026-05-29)

A four-part loop, each piece independently useful:

| Piece | File(s) | Role |
|---|---|---|
| **Capture driver** | `tools/frida_capture.py` (39 KB) | Spawns retail under a Windows frida-server, loads the agent, lays down a run dir. Knobs: hide-window, turbo (virtual clock), silent-audio, force-input, **call-trace**, **mem-watch**, d3d-trace, auto-z-spam, force-resolution. |
| **Frida agent** | `tools/frida/openrecet-agent.js` (124 KB) | All the in-process hooks. Per-frame anchor = D3D `Present`. Emits batched JSONL events (`frame`, `call_trace_batch`, `mem_access_batch`, …). |
| **Port-side probe** | `src/call_trace.{c,h}` | `CALL_TRACE_ENTER(0xVA)` macro at the top of each ported function. Compiles to one null-check when off; emits `{va,ret_va,frame}` JSONL matching the agent's schema. `CALL_TRACE_ENTER_STUB` marks not-yet-complete bodies. |
| **Diff** | `tools/call_trace_diff.py` | Per-frame set diff of retail vs port VAs → **overlap** / **retail-only (= port gap, next thing to port)** / **port-only (= structural divergence / wrong tag)**. Frame alignment via `--align-on-first 0xVA` (anchors each side to the first frame a marker fires, so load-time skew doesn't matter). |
| **Mem watcher** | `tools/mem_watch.py` + agent `MemoryAccessMonitor` | Write-protect a region in retail; every trap → faulting-insn VA → owning engine function (via `port-ledger.json`) → the chip to port. The unblock path for "this region draws stale because nobody fills it". |
| **VA-list vetting** | `tools/bisect_call_trace_vas.py` → `engine_function_vas_frida_safe.json` | Some function-entry hooks crash the engine on boot; bisect finds the safe subset to attach. |

Supporting inputs both diff tools consume: `docs/decompiled/functions.csv`
(VA→name) and `docs/port-ledger.json` (VA→port-status+src).

---

## 2. What OpenSummoners already has

- `tools/frida_capture.py` (17 KB) + `tools/frida/opensummoners-agent.js`
  (979 lines): **boot-only Phase A**. Hooks: MessageBox redirect, HWND
  ownership tracking, hide-window, turbo (Sleep no-op + virtual
  timeGetTime + **GetTickCount** + WaitMessage no-block), silent-audio,
  launcher dialog auto-click + bypass, a **manual frame counter bumped on
  PeekMessage** (explicitly "until we have a real end-of-frame anchor").
- `docs/decompiled/functions.csv` — 1768 functions, **identical schema**
  to openrecet (`name,entry,size,is_thunk,calling_conv`).
- `docs/port-ledger.json` — **identical schema** (`va,name,size,status,src`).
- `src/main.c` — `main_loop_body()` drains PeekMessage, calls
  `zdd_present` (the DDraw `FUN_005b8fc0` 5-mode dispatcher), then
  `frame_limiter`; `g_frame_counter++` per loop. Title-scene FSMs ported.
- Build: `SRCS := $(wildcard *.c)` — drop a new `.c` in `src/` and it's in
  both the exe and debug builds automatically.

So the **driver + ledger + functions.csv + a working boot agent already
exist**. `call_trace_diff.py` and `mem_watch.py` are schema-compatible and
port near-verbatim. The missing pieces are the **emitters** (retail-side
agent modes + port-side probe) and a **real frame anchor**.

---

## 3. Adaptation deltas (SoTE ≠ Recettear)

1. **DirectDraw, not Direct3D8.** openrecet's `d3d_trace` (IDirect3DDevice8
   vtable hooks) has no direct analog. But the *primary* ask — call-graph
   parity — is renderer-agnostic: it's about *which engine functions ran*,
   not D3D state. A DDraw Blt/Lock/Flip state-trace ("Phase R") is
   deferred; call-graph diff + mem-watch come first, exactly the pieces
   openrecet credits for fast convergence.

2. **Frame anchor = the Flip.** Replace the PeekMessage proxy with a hook
   on `FUN_005b8fc0` (the DDraw Flip / present dispatcher) `onEnter`:
   bump an authoritative `g_frame`, flush per-frame event batches there.
   The port side already increments `g_frame_counter` once per
   `zdd_present`, so both sides count *flips* → directly comparable.
   `--align-on-first` absorbs the boot/load skew.

3. **Engine VA list must be generated.** openrecet ships a vetted
   `engine_function_vas_frida_safe.json`; we generate ours from
   `functions.csv` (drop thunks + sub-~8-byte stubs) and **bisect** for the
   Frida-safe subset. 1768 functions is small enough to attach broadly,
   then carve out crashers.

4. **Input is a black box (and that's a feature here).** SoTE reads input
   via DInput `GetDeviceState` (vtable `[0x24]`) into a `+0x108` ring
   buffer whose *writer is unidentified* (open thread in HANDOFF). Two
   injection strategies:
   - **Interim (no RE needed):** hook `IDirectInputDevice8::GetDeviceState`
     `onLeave` and rewrite the returned 256-byte keyboard-state buffer to
     synthesize key presses per a scenario trace. Drives title→menu
     unattended (the `auto_z_spam` analog).
   - **Precise (after mem-watch):** point `mem_watch` at the `+0x108` ring
     buffer → find the writer → overwrite there. **This also resolves the
     HANDOFF "who fills the input ring" black box** — the harness pays for
     itself on its first real use.

---

## 4. Implementation phases (dependency order)

Each phase ends at a committable, independently-useful checkpoint. Phases
marked **[offline]** are buildable + unit-testable with no retail/Frida/UAC
— I can land these solo. Phases marked **[live]** need a retail-under-Frida
run (UAC prompt, game drives on the Windows host) → **human-verification
gate**.

### Phase 0 — Retail frame anchor  **[live]**
- Agent: hook `FUN_005b8fc0` `onEnter` → authoritative `g_frame++`; expose
  it to all emitters. Keep PeekMessage counter as a labelled fallback.
- Verify: a smoke run logs a monotonic frame count that tracks visible
  animation cadence.

### Phase E.1a — Engine VA list + bisect  **[offline gen / live vet]**
- `tools/gen_engine_vas.py`: `functions.csv` → `tools/frida/data/engine_vas.json`
  (filter `is_thunk`, size < 8, and a hand-maintained denylist).
- `tools/bisect_call_trace_vas.py` (port of openrecet's): binary-search the
  set of hooks that destabilize boot → `engine_vas_frida_safe.json`.

### Phase E.1b — Retail call-trace emitter  **[live]**
- Agent `call_trace` mode: `Interceptor.attach` `onEnter` over the safe VA
  list, buffer `{va, ret_va, frame}`, flush the batch on each Flip. Honour
  a `call_trace_frames` whitelist (output saturates fast otherwise).
- Driver: `CaptureConfig` fields + CLI `--call-trace`, `--call-trace-frames`,
  `--call-trace-vas-file`, writing `<run_dir>/call_trace.jsonl`.

### Phase E.1c — Port-side call-trace probe  **[offline]**
- Copy `src/call_trace.{c,h}` from openrecet (rename guards
  `OPENRECET_`→`OPENSUMMONERS_`). Picked up by the wildcard build for free.
- Wire into `main.c`: `parse_cmdline` absorbs `--call-trace <path>` +
  `--call-trace-frames`; `call_trace_begin_frame(g_frame_counter)` /
  `call_trace_end_frame()` bracket `main_loop_body`; `call_trace_shutdown`
  at exit.
- Sprinkle `CALL_TRACE_ENTER(0xVA)` into the already-ported functions
  (`title_scene` FSMs, `app_pump_frame`, `zdd_*`, `cs_dispatch`, …),
  `CALL_TRACE_ENTER_STUB` where bodies are partial.
- Unit-test in the host suite: a test drives a couple of ported functions
  with `--call-trace` to a temp file and asserts the JSONL rows.

### Phase E.2 — call_trace_diff.py  **[offline]**
- Port verbatim (functions.csv-compatible). Default anchor candidate:
  title-scene entry `FUN_0056aea0` for `--align-on-first`.
- Smoke-test with two hand-written JSONL fixtures.

### Phase D.7 — mem_watch.py + agent MemoryAccessMonitor  **[offline script / live capture]**
- Port `mem_watch.py` near-verbatim (ledger-compatible; only the `fc.`
  config field names change).
- Agent `mem_watch` mode: `MemoryAccessMonitor` over regions, precise
  re-arm, batched `mem_access_batch`.
- First targets: the `+0x108` input ring buffer (→ writer = the input
  black box) and any title-screen region that draws stale.

### Phase I — Input injection / scenario driver  **[live]**
- Agent: `GetDeviceState` `onLeave` keyboard-buffer rewrite from a
  per-frame scenario trace; plus an `auto_z_spam` analog.
- Define starter scenarios (`tools/scenarios/`): `title-idle`,
  `title-press-start`, `menu-navigate` — each a `{frame, buttons}` trace
  that drives a reproducible rendering path for the diff.

### Phase R — DDraw render/state trace  **[deferred]**
- Analog of `d3d_trace` for DDraw `Blt`/`BltFast`/`Lock`/`Flip`. Only once
  call-graph parity plateaus and we need pixel-level divergence.

### Wrap — docs
- `docs/parity-harness.md` how-to; update `HANDOFF.md` "Tooling" + the
  Frida-turbo open thread; engine-quirks for anything found.

---

## 5. Recommended execution order & gates

1. **E.1c + E.2 first** (both `[offline]`): port-side probe + diff script +
   host tests. Zero retail dependency → lands + commits solo, and gives the
   port half of the loop immediately.
2. **Phase 0 + E.1a/b** next (`[live]`): frame anchor + retail emitter. This
   is the **first human-verification gate** — needs a real retail Frida run
   (UAC, the game window on the Windows host). I'll pause and ping here.
3. **First real diff**: title-idle scenario, `--align-on-first 0x56aea0`.
   This is the payoff — the retail-only list is the prioritized port queue.
4. **D.7 mem-watch** + **Phase I input injection** to reach deeper scenes.

Open decision for the user: **scope of this pass.** Build the whole stack,
or stop after the first working call-graph diff (steps 1–3) and evaluate?
Recommendation: steps 1–3, because that's the minimum that proves the loop
end-to-end and is what openrecet says moved the needle.
