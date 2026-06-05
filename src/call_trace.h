/* Port-side call tracer — per-frame JSONL of which port functions ran.
 *
 * Counterpart of the Frida `call_trace.jsonl` emitter in
 * tools/frida/opensummoners-agent.js.  Together they let
 * tools/call_trace_diff.py walk both sides of a title-screen / scene
 * frame and report:
 *
 *   • which engine functions retail called that we never reached
 *     (= unported functions our scene-state code skips today — the
 *     prioritized port queue)
 *   • which we called that retail didn't (= structural divergence)
 *   • which both sides called (= overlap, candidates for I/O diff)
 *
 * Schema (matches the Frida agent's call_trace event, modulo ts):
 *
 *   {"va": <ghidra_va>, "ret_va": <module_relative>, "frame": <N>}
 *
 *   • va        — the engine Ghidra-VA the port function corresponds
 *                 to.  Annotation-driven (CALL_TRACE_ENTER(0x4xxxxx))
 *                 rather than auto-discovered: the manual probe is
 *                 explicit, lossless, and doubles as port↔engine
 *                 documentation.  False positives impossible.
 *   • ret_va    — caller's PC, module-relative.  Add IMAGE_BASE
 *                 (0x00400000) to map to a Ghidra VA.  Identical
 *                 convention to the Frida agent.
 *   • frame     — sim-frame index.  Both sides count *flips* (one per
 *                 present): the port bumps g_frame_counter once per
 *                 zdd_present in main_loop_body; the agent bumps on each
 *                 FUN_005b8fc0 (the DDraw Flip dispatcher) entry.  So the
 *                 two frame axes are directly comparable, modulo the
 *                 boot/load skew that call_trace_diff's --align-on-first
 *                 absorbs.
 *   • seq       — per-frame execution-order counter, stamped on every
 *                 emitted row (legacy ENTER + the BEGIN/FIELD/END field-
 *                 bearing events).  Reset each begin_frame.  Lets
 *                 tools/flow_diff.py align the call CHAIN in order, not
 *                 just the set (the data-blind, order-blind view is
 *                 call_trace_diff.py's Counter).
 *
 * Wiring (in src/main.c, alongside the frame loop):
 *
 *   1. parse_cmdline absorbs --call-trace <path> + optional
 *      --call-trace-frames i,j,k.
 *   2. call_trace_init_from_cli(...) is called once, after parse_cmdline.
 *   3. main_loop_body calls call_trace_begin_frame(g_frame_counter) at
 *      the top (before any traced work) and call_trace_end_frame() after.
 *   4. On shutdown, call_trace_shutdown() closes the file.
 *
 * Cost when not enabled: every CALL_TRACE_ENTER is a single null-check
 * on a static FILE pointer.  Output saturates fast when traced functions
 * are hot, so pair with --call-trace-frames for non-title scenarios.
 * Probe annotations live alongside the port functions: each declares the
 * Ghidra VA it implements.  Adding a new probe is one line per ported
 * function.
 */

#ifndef OPENSUMMONERS_CALL_TRACE_H
#define OPENSUMMONERS_CALL_TRACE_H

#include <stddef.h>
#include <stdint.h>

void call_trace_init_from_cli(const char *path,
                              const unsigned *frames, size_t n_frames);
void call_trace_begin_frame(unsigned frame);
void call_trace_end_frame(void);
void call_trace_shutdown(void);

/* Emit one JSONL row.  ret_addr is captured by the probe macro so the
 * value is the caller's PC, not the macro expansion's.  `stub` carries
 * forward into the emitted JSON as `"stub": true` when nonzero — used
 * by tools/call_trace_diff.py to distinguish "matched-count-AND-fully-
 * ported" rows from "matched-count-but-port-body-is-a-stub" rows.
 * Without the marker, pure call-count parity can hide a stubbed body
 * (= the engine fires the function, our port also fires SOMETHING at
 * the same VA, but our SOMETHING returns immediately or only emits the
 * preamble). */
void call_trace_enter(uint32_t ghidra_va, const void *ret_addr, int stub);

/* Probe macro for a FULLY PORTED function — body matches the engine's
 * behavioural contract end-to-end (subject to documented divergences).
 * Compiles to a single null-check + fprintf gate when --call-trace is
 * off.  `ghidra_va` is the engine VA the function corresponds to. */
#define CALL_TRACE_ENTER(ghidra_va) \
    call_trace_enter((uint32_t)(ghidra_va), __builtin_return_address(0), 0)

/* Probe macro for a PARTIALLY PORTED or STUB function.  Use when:
 *   - Function body is a stub that returns immediately (or only fires a
 *     state-write preamble) and the real work is deferred to a future
 *     chip.
 *   - Function body is partially ported — wraps real work AND
 *     unimplemented sub-stubs, AND the unimplemented portion is
 *     load-bearing for the current scene's behaviour.
 * Do NOT use when:
 *   - Body is fully ported but happens to hit a no-op branch in the
 *     current scenario (e.g. a counter==0 early-out where the body is
 *     complete and the gate is just BSS-zero today).
 * The marker propagates into JSONL as `"stub": true`; call_trace_diff
 * surfaces those rows as `≈` (count-parity but body-not-complete)
 * distinct from `=` (full parity) and `≠` (count mismatch). */
#define CALL_TRACE_ENTER_STUB(ghidra_va) \
    call_trace_enter((uint32_t)(ghidra_va), __builtin_return_address(0), 1)

/* ── field-bearing event (BEGIN/FIELD/END) ─────────────────────────────────
 * Emit a call event carrying a DECLARED PAYLOAD — the salient inputs/state the
 * function used — so tools/flow_diff.py can match the data moved, not just that
 * the function ran.  The retail side declares the same-named fields in
 * tools/flow/retail_fields.json (joined by va + field-name); the Frida agent
 * reads them off the live process.  See docs/plans/trace-tooling-phase-b.md.
 *
 * Usage (at function entry, BEFORE any traced sub-call):
 *     CALL_TRACE_BEGIN(0x56aea0);
 *     CALL_TRACE_I32("cursor", cursor);
 *     CALL_TRACE_F32("phase",  phase);
 *     CALL_TRACE_END();
 *
 * Each field is captured at the call site (exact C values — free + precise).
 * Like the ENTER probes these compile to a cheap gated no-op when --call-trace
 * is off.  CALL_TRACE_ENTER(va) remains the no-payload form.  One event is
 * assembled in a static buffer and fwritten atomically at END so the stream is
 * never interleaved; emit fields at entry, before any traced sub-call, then
 * END (the discipline that keeps events non-nested). */
void call_trace_begin(uint32_t ghidra_va, const void *ret_addr);
void call_trace_begin_stub(uint32_t ghidra_va, const void *ret_addr);
void call_trace_field_i32(const char *name, int32_t v);
void call_trace_field_u32(const char *name, uint32_t v);
void call_trace_field_f32(const char *name, float v);
void call_trace_field_hex(const char *name, uint32_t v);
void call_trace_end(void);

#define CALL_TRACE_BEGIN(ghidra_va) \
    call_trace_begin((uint32_t)(ghidra_va), __builtin_return_address(0))
/* Field-bearing BEGIN that also marks the row "stub":true (partially-ported
 * body; the declared INPUTS are still diffed by flow_diff, the stub mark keeps
 * call_trace_diff's coverage view honest). Pairs with CALL_TRACE_END(). */
#define CALL_TRACE_BEGIN_STUB(ghidra_va) \
    call_trace_begin_stub((uint32_t)(ghidra_va), __builtin_return_address(0))
#define CALL_TRACE_I32(name, v) call_trace_field_i32((name), (int32_t)(v))
#define CALL_TRACE_U32(name, v) call_trace_field_u32((name), (uint32_t)(v))
#define CALL_TRACE_F32(name, v) call_trace_field_f32((name), (float)(v))
#define CALL_TRACE_HEX(name, v) call_trace_field_hex((name), (uint32_t)(v))
#define CALL_TRACE_END()        call_trace_end()

#endif /* OPENSUMMONERS_CALL_TRACE_H */
