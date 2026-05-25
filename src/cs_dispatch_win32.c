/*
 * src/cs_dispatch_win32.c — Win32 primitives backing cs_dispatch.c.
 *
 * Supplies the primitive set declared in cs_dispatch.h for the real
 * (mingw) build.  The host harness in tests/test_cs_dispatch.c
 * provides recording stubs and does NOT link this file.
 *
 * Real-build primitive map:
 *   cs_log_get_last_error              FUN_00560900 — Win32 GetLastError +
 *                                       FormatMessageA + OutputDebugStringA
 *   cs_fatal_log                       FUN_00406440 — engine fatal logger
 *   cs_fatal_log_with_lasterror        FUN_00426110(..., 1, ...) — DB Mode
 *                                       variant w/ GetLastError prepended
 *   cs_exit                            FUN_005bf5db(0) — ExitProcess(0)
 *
 * Note: the OUT-side log-file machinery the engine uses (FUN_005bf4e8
 * fopen-equivalent + FUN_005bf496 fprintf-equivalent + FUN_005bf440
 * fclose-equivalent) is NOT wired in.  The drop-in's logging design
 * defers to OutputDebugStringA for now — captured by Frida or
 * DebugView at runtime — so the cleaner "stamp message into the
 * Win32 debug stream" subset is sufficient for boot diagnosis.  Wire
 * the on-disk log later if title-menu errors prove hard to triage
 * without it.
 */
#include "cs_dispatch.h"

#include <windows.h>
#include <stdio.h>

/* ─── helper: format a single GetLastError-flavoured line ─────────── */

/* Retail emits "[%08d] %s" via FUN_005bf3ee (a sprintf wrapper) where
 * %s is the FormatMessageA-decoded text.  We mirror with snprintf. */
static void emit_last_error_line(void)
{
    DWORD code = GetLastError();
    if (code == 0) return;

    CHAR  raw[256];
    raw[0] = '\0';
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS
                       | FORMAT_MESSAGE_MAX_WIDTH_MASK,  /* 0x12ff */
                   NULL, code, 0, raw, sizeof(raw) - 1, NULL);

    CHAR  line[512];
    line[0] = '\0';
    _snprintf(line, sizeof(line) - 1, "[%08lu] %s", (unsigned long)code, raw);
    line[sizeof(line) - 1] = '\0';

    OutputDebugStringA(line);
    if (cs_engine_name_header != NULL) OutputDebugStringA(cs_engine_name_header);
    OutputDebugStringA("\n");
}

void cs_log_get_last_error(void)
{
    emit_last_error_line();
}

void cs_fatal_log(const char *fixed_msg, const char *header)
{
    if (fixed_msg != NULL) OutputDebugStringA(fixed_msg);
    if (header    != NULL) OutputDebugStringA(header);
    OutputDebugStringA("\n");
}

void cs_fatal_log_with_lasterror(const char *fixed_msg, const char *header)
{
    emit_last_error_line();
    cs_fatal_log(fixed_msg, header);
}

void cs_exit(int code)
{
    ExitProcess((UINT)code);
}
