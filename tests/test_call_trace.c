/*
 * tests/test_call_trace.c — host tests for src/call_trace.c.
 *
 * The port-side call tracer is pure stdio: init opens a file, begin/end
 * frame gate emission, call_trace_enter writes one JSONL row.  These
 * tests drive the public API against a temp file and assert the emitted
 * JSONL matches the schema tools/call_trace_diff.py consumes:
 *
 *   {"va":<u>,"ret_va":<u>,"frame":<u>}          (full-port probe)
 *   {"va":<u>,"ret_va":<u>,"frame":<u>,"stub":true}  (stub probe)
 *
 * Host-side g_module_base is NULL, so ret_va is deterministically 0 —
 * the va/frame/stub fields are what matter for the diff.
 */
#include "../src/call_trace.h"
#include "t.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Read an entire file into a heap buffer (NUL-terminated).  Returns NULL
 * if the file is missing.  Caller frees. */
static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc((size_t)n + 1);
    size_t got = fread(buf, 1, (size_t)n, f);
    buf[got] = '\0';
    fclose(f);
    return buf;
}

/* Unique-ish temp path per test (no Date/rand available; the test runner
 * is single-threaded and unlinks after each, so a per-test literal is
 * sufficient and deterministic). */
static const char *tmp_path(const char *tag)
{
    static char path[256];
    snprintf(path, sizeof(path), "/tmp/os_call_trace_%s.jsonl", tag);
    return path;
}

/* A full-port-probed function — exercises the CALL_TRACE_ENTER macro
 * (which captures __builtin_return_address) rather than the raw API. */
static void probed_full(void) { CALL_TRACE_ENTER(0x5b8fc0); }
static void probed_stub(void) { CALL_TRACE_ENTER_STUB(0x412c10); }

int test_call_trace_emits_row_for_enabled_frame(void)
{
    const char *p = tmp_path("enabled");
    unlink(p);
    call_trace_init_from_cli(p, NULL, 0);   /* no whitelist = every frame */
    call_trace_begin_frame(0);
    probed_full();
    call_trace_end_frame();
    call_trace_shutdown();

    char *txt = slurp(p);
    if (!txt) T_FAIL("no trace file written at %s", p);
    /* 0x5b8fc0 == 6000576 decimal. */
    if (!strstr(txt, "\"va\":6000576"))
        T_FAIL("expected va 6000576 in: %s", txt);
    if (!strstr(txt, "\"frame\":0"))
        T_FAIL("expected frame 0 in: %s", txt);
    if (strstr(txt, "\"stub\":true"))
        T_FAIL("full-port probe must not carry stub marker: %s", txt);
    free(txt);
    unlink(p);
    return 0;
}

int test_call_trace_frame_whitelist_gates(void)
{
    const char *p = tmp_path("whitelist");
    unlink(p);
    unsigned frames[] = { 2 };
    call_trace_init_from_cli(p, frames, 1);

    call_trace_begin_frame(0);   /* not whitelisted → suppressed */
    probed_full();
    call_trace_begin_frame(2);   /* whitelisted → emitted */
    probed_full();
    call_trace_end_frame();
    call_trace_shutdown();

    char *txt = slurp(p);
    if (!txt) T_FAIL("no trace file written");
    if (strstr(txt, "\"frame\":0"))
        T_FAIL("frame 0 should have been gated out: %s", txt);
    if (!strstr(txt, "\"frame\":2"))
        T_FAIL("frame 2 should have been emitted: %s", txt);
    free(txt);
    unlink(p);
    return 0;
}

int test_call_trace_stub_marker(void)
{
    const char *p = tmp_path("stub");
    unlink(p);
    call_trace_init_from_cli(p, NULL, 0);
    call_trace_begin_frame(7);
    probed_stub();
    call_trace_end_frame();
    call_trace_shutdown();

    char *txt = slurp(p);
    if (!txt) T_FAIL("no trace file written");
    /* 0x412c10 == 4271120 decimal. */
    if (!strstr(txt, "\"va\":4271120"))
        T_FAIL("expected va 4271120 in: %s", txt);
    if (!strstr(txt, "\"stub\":true"))
        T_FAIL("stub probe must carry stub marker: %s", txt);
    if (!strstr(txt, "\"frame\":7"))
        T_FAIL("expected frame 7 in: %s", txt);
    free(txt);
    unlink(p);
    return 0;
}

int test_call_trace_disabled_when_no_path(void)
{
    /* init(NULL) must not open a file; subsequent enters are inert and
     * must not crash. */
    call_trace_init_from_cli(NULL, NULL, 0);
    call_trace_begin_frame(0);
    probed_full();
    call_trace_end_frame();
    call_trace_shutdown();
    return 0;
}

int test_call_trace_multiple_rows_one_frame(void)
{
    const char *p = tmp_path("multi");
    unlink(p);
    call_trace_init_from_cli(p, NULL, 0);
    call_trace_begin_frame(1);
    probed_full();
    probed_stub();
    probed_full();
    call_trace_end_frame();
    call_trace_shutdown();

    char *txt = slurp(p);
    if (!txt) T_FAIL("no trace file written");
    /* Three newline-terminated rows. */
    int newlines = 0;
    for (const char *c = txt; *c; c++) if (*c == '\n') newlines++;
    if (newlines != 3)
        T_FAIL("expected 3 rows, counted %d newlines in: %s", newlines, txt);
    free(txt);
    unlink(p);
    return 0;
}

/* seq counts execution order within a frame (0,1,2…) and resets each
 * begin_frame — the chain-alignment key flow_diff.py walks. */
int test_call_trace_seq_orders_and_resets(void)
{
    const char *p = tmp_path("seq");
    unlink(p);
    call_trace_init_from_cli(p, NULL, 0);
    call_trace_begin_frame(0);
    probed_full();              /* seq 0 */
    probed_stub();              /* seq 1 */
    call_trace_begin_frame(1);
    probed_full();              /* seq 0 again (reset) */
    call_trace_end_frame();
    call_trace_shutdown();

    char *txt = slurp(p);
    if (!txt) T_FAIL("no trace file written");
    if (!strstr(txt, "\"seq\":0")) T_FAIL("expected seq 0 in: %s", txt);
    if (!strstr(txt, "\"seq\":1")) T_FAIL("expected seq 1 in: %s", txt);
    /* frame 1's lone row must carry seq 0 (counter reset). */
    if (!strstr(txt, "\"frame\":1,\"seq\":0"))
        T_FAIL("frame 1 should reset seq to 0: %s", txt);
    free(txt);
    unlink(p);
    return 0;
}

/* A field-bearing function: BEGIN, declare typed fields, END.  The emitted
 * row carries an `f:{…}` payload joined to retail by (va, field-name). */
static void probed_fields(void)
{
    CALL_TRACE_BEGIN(0x56aea0);
    CALL_TRACE_I32("cursor", -3);
    CALL_TRACE_U32("count", 42u);
    CALL_TRACE_F32("phase", 1.5f);
    CALL_TRACE_HEX("flags", 0xbeefu);
    CALL_TRACE_END();
}

int test_call_trace_field_bearing_payload(void)
{
    const char *p = tmp_path("fields");
    unlink(p);
    call_trace_init_from_cli(p, NULL, 0);
    call_trace_begin_frame(4);
    probed_fields();
    call_trace_end_frame();
    call_trace_shutdown();

    char *txt = slurp(p);
    if (!txt) T_FAIL("no trace file written");
    /* 0x56aea0 == 5680800 decimal. */
    if (!strstr(txt, "\"va\":5680800"))
        T_FAIL("expected va 5680800 in: %s", txt);
    if (!strstr(txt, "\"frame\":4,\"seq\":0"))
        T_FAIL("expected frame 4 seq 0 in: %s", txt);
    if (!strstr(txt, "\"f\":{\"cursor\":-3"))
        T_FAIL("expected i32 cursor field opening the payload: %s", txt);
    if (!strstr(txt, "\"count\":42"))
        T_FAIL("expected u32 count field: %s", txt);
    if (!strstr(txt, "\"phase\":1.5"))
        T_FAIL("expected f32 phase field: %s", txt);
    if (!strstr(txt, "\"flags\":\"0xbeef\""))
        T_FAIL("expected hex flags field as string: %s", txt);
    if (strstr(txt, "\"stub\":true"))
        T_FAIL("plain BEGIN must not carry stub marker: %s", txt);
    /* exactly one row. */
    int newlines = 0;
    for (const char *c = txt; *c; c++) if (*c == '\n') newlines++;
    if (newlines != 1)
        T_FAIL("expected 1 row, counted %d: %s", newlines, txt);
    free(txt);
    unlink(p);
    return 0;
}

/* A stub-marked field-bearing event still carries its payload AND "stub":true
 * — the declared inputs are diffed even when the body is a subset. */
int test_call_trace_field_bearing_stub(void)
{
    const char *p = tmp_path("fields_stub");
    unlink(p);
    call_trace_init_from_cli(p, NULL, 0);
    call_trace_begin_frame(0);
    CALL_TRACE_BEGIN_STUB(0x56aea0);
    CALL_TRACE_I32("cursor", 7);
    CALL_TRACE_END();
    call_trace_end_frame();
    call_trace_shutdown();

    char *txt = slurp(p);
    if (!txt) T_FAIL("no trace file written");
    if (!strstr(txt, "\"f\":{\"cursor\":7}"))
        T_FAIL("expected closed payload {cursor:7}: %s", txt);
    if (!strstr(txt, "\"stub\":true"))
        T_FAIL("stub BEGIN must carry stub marker: %s", txt);
    free(txt);
    unlink(p);
    return 0;
}

/* A BEGIN with no fields emits a bare event (no `f` object) — and the gate is
 * inert when the frame is not whitelisted (no half-open event leaks). */
int test_call_trace_field_bearing_empty_and_gated(void)
{
    const char *p = tmp_path("fields_empty");
    unlink(p);
    unsigned frames[] = { 5 };
    call_trace_init_from_cli(p, frames, 1);

    call_trace_begin_frame(0);            /* not whitelisted */
    CALL_TRACE_BEGIN(0x56aea0);
    CALL_TRACE_I32("x", 1);               /* must be dropped (gated) */
    CALL_TRACE_END();
    call_trace_begin_frame(5);            /* whitelisted */
    CALL_TRACE_BEGIN(0x56aea0);           /* no fields */
    CALL_TRACE_END();
    call_trace_end_frame();
    call_trace_shutdown();

    char *txt = slurp(p);
    if (!txt) T_FAIL("no trace file written");
    if (strstr(txt, "\"frame\":0"))
        T_FAIL("gated frame 0 must not emit: %s", txt);
    if (strstr(txt, "\"f\":"))
        T_FAIL("fieldless BEGIN must not open an f object: %s", txt);
    if (!strstr(txt, "\"frame\":5,\"seq\":0}"))
        T_FAIL("expected bare frame-5 event: %s", txt);
    free(txt);
    unlink(p);
    return 0;
}
