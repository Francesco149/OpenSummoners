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
| `tools/mem_watch.py` | Write-protects a region in retail; ranks trapping instructions → owning function via the ledger. **[live capture / offline rank]** |

Frame axis: both sides count **Flips**. The agent bumps on each
`FUN_005b8fc0` (DDraw Flip) entry; the port bumps `g_frame_counter` once per
`zdd_present`. `--align-on-first 0xVA` absorbs boot/load skew.

## First-run order (the live gate)

These need retail under Frida on the Windows host (UAC prompt for
frida-server; the game runs). Run inside `nix develop`.

```sh
# 1. (offline) candidate VA list
python3 tools/gen_engine_vas.py

# 2. (live) vet the Frida-safe subset — ~20-40 min, writes
#    tools/frida/data/engine_vas_frida_safe.json.
#    ⚠ --boot-threshold + timing constants need calibration on this first run
#    (see the file header): confirm a known-good boot's msg_count first.
python3 tools/bisect_call_trace_vas.py

# 3. (live) capture retail's per-frame call trace for the title scene
python3 tools/frida_capture.py --call-trace \
    --run-dir runs/calltrace-title --exact-run-dir \
    --duration-ms 12000 --max-frames 4000
#    → runs/calltrace-title/call_trace.jsonl

# 4. (offline) capture the port's trace for the same scene
make -C src
./build/opensummoners.exe --frames 600 --call-trace /tmp/port_calltrace.jsonl

# 5. (offline) diff, anchored on the title-scene entry
python3 tools/call_trace_diff.py \
    --retail runs/calltrace-title/call_trace.jsonl \
    --port   /tmp/port_calltrace.jsonl \
    --align-on-first 0x56aea0 --verbose
```

The **retail-only** list in step 5 is the prioritized port queue.

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
