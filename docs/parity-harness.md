# Structural-parity harness — how to run

Diff **which engine functions retail called per frame** against **which our
port reached**, to drive rendering-path porting. Design + rationale:
[`docs/plans/parity-harness.md`](plans/parity-harness.md). This file is the
operational cheat-sheet.

## Pieces

| Tool | What it does |
|---|---|
| `tools/gen_engine_vas.py` | `functions.csv` → `tools/frida/data/engine_vas.json` (candidate hook list, thunks/tiny stubs dropped). **[offline]** |
| `tools/bisect_call_trace_vas.py` | Boots retail with the candidate set hooked, bisects out VAs that crash → `engine_vas_frida_safe.json`. **[live]** |
| `tools/frida_capture.py --call-trace` | Hooks the safe VA list in retail, emits `<run_dir>/call_trace.jsonl` (per-Flip-frame). **[live]** |
| `src/call_trace.{c,h}` + `opensummoners.exe --call-trace <path>` | Port-side emitter; `CALL_TRACE_ENTER(0xVA)` probes. **[offline]** |
| `tools/call_trace_diff.py` | Diffs the two `call_trace.jsonl` → overlap / retail-only (= port gap) / port-only (= divergence). **[offline]** |
| `tools/flow_diff.py` + `tools/flow/retail_fields.json` | The LOGIC drill-in: names the first call whose inputs matched but whose output/state diverged (field-bearing call trace). **[offline]** |
| `tools/render_diff.py` + `src/render_id.{c,h}` | The RENDER drill-in: names the first divergent BLIT and classifies it (`[sprite]`/`[decode]`/`[rect]`/`[state]`), keyed on the cross-side `(resource_id, frame)` identity + a decoded-sheet `dhash`. See `findings/ddraw-blit-trace.md`. **[offline]** |
| `tools/mem_watch.py` | Write-protects a region in retail; ranks trapping instructions → owning function via the ledger. **[live capture / offline rank]** |

**The two drill-ins pair up:** `render_diff` says *which draw* is wrong (and how —
wrong sprite / wrong decoded pixels / wrong rect / wrong DDraw state); `flow_diff`
says *which logic* produced it. Both consume the same `call_trace.jsonl`.

> **Don't add a bespoke `--foo-probe` flag.** A new datum is almost always a
> `fields[]` entry on the right VA in `retail_fields.json` with the right `src`
> (`global`/`arg`/`argderef`/`chain`/`rngcalls`/`renderid`/`thisderef`); a new
> *kind* of datum is one new `src:` type in the agent's `ctReadField` (one place →
> every VA can read it, and `flow_diff`/`render_diff` classify it for free). The
> field spec is the unifier — bespoke flags don't compose with the diffs and rot
> once answered (the repo carries a graveyard of `--cursor-probe`/`--fade-probe`/…).
> Reach for a standalone flag only when the thing genuinely can't be a per-VA/
> per-frame field (e.g. an interactive recorder). `renderid`/`rngcalls` are the
> model: a new `src:` that auto-installs its own hook, no flag.

Frame axis: both sides count **Flips**. The agent bumps on each
`FUN_005b8fc0` (DDraw Flip) entry; the port bumps `g_frame_counter` once per
`zdd_present`. `--align-on-first 0xVA` absorbs boot/load skew.

## First-run order (the live gate)

These need retail under Frida on the Windows host. **Live-verified working
2026-06-02** — Frida is always up and UAC is auto-approved on this host, so
these are self-serviceable (no human gate). Run inside `nix develop`.

> ⚠ **Everything live MUST be `--no-turbo`.** Turbo freezes the splash before
> the engine reaches its message pump (engine-quirks #29): the window appears
> but `msg_count`/Flips stay 0. No-turbo boots cleanly to the title and renders
> (a 16 s no-turbo capture saw **1914 Flip frames**); turbo saw **0**.

```sh
# 1. (offline) candidate VA list
python3 tools/gen_engine_vas.py

# 2. (live, usually SKIP) vet the Frida-safe subset.  Already done: the full
#    1743-VA candidate set was verified Frida-safe by the direct capture in
#    step 3 (no crash, 1.8M events), so engine_vas_frida_safe.json is the full
#    set.  Only re-run if a new candidate set adds a crashing VA.
python3 tools/bisect_call_trace_vas.py    # now defaults to --no-turbo

# 3. (live) capture retail's per-frame call trace for the title scene
python3 tools/frida_capture.py --no-turbo --call-trace \
    --call-trace-vas-file tools/frida/data/engine_vas_frida_safe.json \
    --run-dir runs/calltrace-title --exact-run-dir \
    --duration-ms 16000
#    → runs/calltrace-title/call_trace.jsonl  (≈135 MB for ~1900 frames;
#      add --call-trace-frames <f1,f2,...> to whitelist specific Flip frames
#      and keep it small.  runs/ is gitignored.)

# 4. (offline) capture the port's trace for the same scene — BLOCKED until
#    main.c drives title_scene_step (the drop-in's minimal main_loop_body
#    doesn't run the real scene yet).  See HANDOFF "Next move".
make -C src
./build/opensummoners.exe --frames 600 --call-trace /tmp/port_calltrace.jsonl

# 5. (offline) diff, anchored on the title-scene entry
python3 tools/call_trace_diff.py \
    --retail runs/calltrace-title/call_trace.jsonl \
    --port   /tmp/port_calltrace.jsonl \
    --align-on-first 0x56aea0 --verbose
```

The **retail-only** list in step 5 is the prioritized port queue.

### What the first live retail capture already showed (2026-06-02)

Mining `runs/calltrace-title` (no-turbo, 1914 frames) for the title render
path — the per-frame call counts confirm the ported control flow and the
next port target against live retail:

| VA | role | calls | frames | note |
|----|------|------:|--------|------|
| `0x56aea0` | title scene | 1 | 0 | one entry; the do/while loops inside ✓ |
| `0x5b1030` | pump | 1915 | all | once/frame ✓ |
| `0x56c180` | frame compose | 1915 | all | once/frame ✓ |
| `0x5b8fc0` | Flip | 1914 | all | once/frame ✓ |
| `0x56c930` | post-update | 984 | all | ≈ half (UPDATE frames only) — validates the pacing-FSM split ✓ |
| `0x5bd550`→`0x5bd680` | blit orch → **software alpha blitter** | 4279 each | all | the hot per-frame draw primitive — **confirms the right next render-bridge target** |
| `0x5b9b70` | color-key blit (ported) | 1937 | 87+ | post-logo ✓ |
| `0x494e10` | logo blit | 359 | 0–471 | intro only ✓ |
| `0x5b9410` | surface reset (ported) | 90 | 0–89 | early phases ✓ |
| `0x418470` | asset get | 2800 | 515+ | menu phase ✓ |

(The title sprite *wrappers* `0x56c4e0`/`56c610`/`56c470`/`56c580` show 0 — they
were tiny-stub-filtered from the candidate set, not absent; the blitter they
funnel into is what's captured.)

## mem_watch (find a region's writer)

```sh
# live capture + rank (e.g. the +0x108 input ring writer — a HANDOFF black box)
python3 tools/mem_watch.py --run-dir runs/memwatch-inputring \
    --region 0xADDR:64:input_ring

# offline re-rank an existing capture (no retail needed)
python3 tools/mem_watch.py --analyze-only --run-dir runs/memwatch-inputring \
    --region 0xADDR:64:input_ring
```

Output is a JSON writer table: each trapping instruction → owning engine
function + port status + src files. `port_candidates` are the
unported/unmapped writers = the chips to port.

## Adding port-side probes

One line at the top of a ported function that corresponds to an engine VA:

```c
#include "call_trace.h"
void my_ported_fn(...) {
    CALL_TRACE_ENTER(0x56aea0);        /* fully ported */
    /* ... */
}
/* or, for a stub / partial body: */
    CALL_TRACE_ENTER_STUB(0x412c10);
```

`CALL_TRACE_ENTER` is one null-check when `--call-trace` is off. The
`_STUB` form marks the row `"stub":true`; the diff shows it as `≈`
(count-parity but body incomplete) vs `=` (full parity). Currently probed:
`zdd_create` (`0x5b7ee0`), `zdd_present` (`0x5b8fc0`), `zdd_window_paint`
(`0x5b9130`), `cs_dispatch_create_screen` (`0x582e90`).
