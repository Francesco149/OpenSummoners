/*
 * opensummoners-launcher.exe — supervised launcher for retail and port runs.
 *
 * Why this exists
 * ---------------
 * The WSL → Windows dev loop has a real hazard: SIGKILL from `timeout`,
 * a hung-up parent terminal, or any other abrupt teardown can leave the
 * spawned .exe orphaned as a Windows process.  The orphan keeps the PE
 * image mapped, masks subsequent rebuilds (WSL appears to "run yesterday's
 * code"), holds the audio device, and wrecks iterate-build-run.  Same
 * autopsy as OpenMare's launcher — see /opt/src/OpenMare/tools/launcher/
 * for the cross-project reference.
 *
 * Mechanism
 * ---------
 * Wrap the child in a Job Object with JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE.
 * When this launcher's handle to the job closes — for ANY reason,
 * including TerminateProcess by SIGKILL via WSLInterop — the kernel
 * atomically kills every process in the job.  No escape: descendants
 * inherit the job (Win8+), nothing breaks away (BREAKAWAY_OK not set).
 *
 * On top of that:
 *   - Optional --timeout-ms hard ceiling.  Sends WM_CLOSE for graceful
 *     wndproc shutdown, then TerminateJobObject after --grace-ms.
 *   - stdin EOF triggers the same graceful shutdown.  When the parent
 *     WSL bash dies, our stdin pipe closes; we notice and unwind.
 *   - Ctrl-C / Ctrl-Break in the console trigger graceful shutdown.
 *
 * Single-instance enforcement lives in the port itself (src/main.c),
 * not here — that way it also covers .exes started without the launcher.
 *
 * Build: `make -C tools/launcher`  (inside `nix develop`).
 * Usage: `./build/opensummoners-launcher.exe --help`.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static HANDLE g_shutdown_event;
static int    g_quiet;

static void logline(const char *fmt, ...)
{
    if (g_quiet) return;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[launcher] ");
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    fflush(stderr);
    va_end(ap);
}

static BOOL WINAPI ctrl_handler(DWORD ev)
{
    switch (ev) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        SetEvent(g_shutdown_event);
        return TRUE;
    }
    return FALSE;
}

/* Block on stdin; on EOF *during the run* signal shutdown.  This is the
 * WSL hang-up detector: when the parent bash dies mid-run, our stdin pipe
 * closes and ReadFile returns 0.  Caveat: when invoked via `bash -c` with
 * no interactive stdin attached, ReadFile returns EOF immediately at
 * startup — that's not a hang-up, just "no interactive stdin to begin
 * with".  Distinguish by requiring either (a) we've successfully read at
 * least one byte, or (b) we've been alive past a short startup grace
 * window before treating EOF as hang-up.  Anything actually read is
 * discarded — we only care about the EOF edge. */
static DWORD WINAPI stdin_watcher(LPVOID arg)
{
    (void)arg;
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (!h || h == INVALID_HANDLE_VALUE) return 0;

    DWORD start = GetTickCount();
    char  buf[256];
    DWORD got = 0;
    int   ever_read = 0;

    for (;;) {
        if (!ReadFile(h, buf, sizeof buf, &got, NULL)) {
            if (!ever_read && GetTickCount() - start < 500) return 0;
            break;
        }
        if (got == 0) {
            if (!ever_read && GetTickCount() - start < 500) return 0;
            break;
        }
        ever_read = 1;
    }
    SetEvent(g_shutdown_event);
    return 0;
}

/* WM_CLOSE every top-level window owned by the child PID.  Hooks the
 * normal wndproc close path — our main.c handles WM_CLOSE → DestroyWindow
 * → PostQuitMessage → clean shutdown with full stderr flush. */
static BOOL CALLBACK close_top_window(HWND hwnd, LPARAM lparam)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == (DWORD)lparam) {
        PostMessageA(hwnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

static void print_usage(void)
{
    fprintf(stderr,
        "opensummoners-launcher — Job-Object supervisor.\n"
        "\n"
        "Usage:\n"
        "  opensummoners-launcher.exe [LAUNCHER FLAGS] -- <child.exe> [CHILD ARGS...]\n"
        "\n"
        "Launcher flags:\n"
        "  --timeout-ms N    Hard timeout for the child run (default: none).\n"
        "                    On expiry: WM_CLOSE the child, wait --grace-ms,\n"
        "                    then TerminateJobObject.\n"
        "  --grace-ms N      Wait between WM_CLOSE and TerminateJobObject\n"
        "                    (default 2000).\n"
        "  --no-stdin-watch  Don't tear down on stdin EOF.  Use only for\n"
        "                    interactive shells where you want the launcher\n"
        "                    to keep going after the terminal hangs up.\n"
        "  --quiet           Suppress the [launcher] log lines on stderr.\n"
        "\n"
        "Exit codes:\n"
        "  0    child exited cleanly (its return code is propagated)\n"
        "  124  --timeout-ms expired (matches GNU `timeout`)\n"
        "  125  launcher setup failure\n"
        "  130  ctrl-c, stdin EOF, or other shutdown signal\n"
        "\n"
        "Safety guarantee: any way this process dies — clean exit, SIGKILL,\n"
        "taskkill, crash — the kernel kills every descendant via\n"
        "JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE.  No orphans.\n");
}

/* Naive command-line reconstruction.  We only need to handle paths
 * without embedded quotes; the dev recipe controls argv. */
static int build_cmdline(int argc, char **argv, int start,
                         char *out, size_t cap)
{
    size_t off = 0;
    for (int j = start; j < argc; j++) {
        int has_space = strchr(argv[j], ' ') != NULL;
        if (j > start) { if (off + 1 >= cap) return 0; out[off++] = ' '; }
        if (has_space) { if (off + 1 >= cap) return 0; out[off++] = '"'; }
        size_t n = strlen(argv[j]);
        if (off + n >= cap) return 0;
        memcpy(out + off, argv[j], n); off += n;
        if (has_space) { if (off + 1 >= cap) return 0; out[off++] = '"'; }
    }
    if (off >= cap) return 0;
    out[off] = 0;
    return 1;
}

int main(int argc, char **argv)
{
    /* UTF-8 codepage for em-dashes / other non-ASCII in our own log
     * lines; without this the console renders them as CP437 mojibake
     * (e.g. "ΓÇö" for "—"). */
    SetConsoleOutputCP(CP_UTF8);

    DWORD timeout_ms = INFINITE;
    DWORD grace_ms   = 2000;
    int   watch_stdin = 1;
    int   sep = -1;

    int i = 1;
    while (i < argc) {
        const char *a = argv[i];
        if (strcmp(a, "--") == 0)                  { sep = i; break; }
        if (!strcmp(a, "--help") || !strcmp(a, "-h")) { print_usage(); return 0; }
        if (!strcmp(a, "--timeout-ms") && i + 1 < argc) {
            timeout_ms = (DWORD)strtoul(argv[++i], NULL, 10); i++; continue;
        }
        if (!strcmp(a, "--grace-ms") && i + 1 < argc) {
            grace_ms = (DWORD)strtoul(argv[++i], NULL, 10); i++; continue;
        }
        if (!strcmp(a, "--no-stdin-watch")) { watch_stdin = 0; i++; continue; }
        if (!strcmp(a, "--quiet"))          { g_quiet = 1;     i++; continue; }
        fprintf(stderr, "[launcher] unknown flag: %s\n", a);
        print_usage();
        return 125;
    }
    if (sep < 0 || sep + 1 >= argc) {
        fprintf(stderr, "[launcher] missing child command — expected `-- child.exe ...`\n");
        print_usage();
        return 125;
    }

    g_shutdown_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!g_shutdown_event) {
        fprintf(stderr, "[launcher] CreateEventA failed: %lu\n", GetLastError());
        return 125;
    }
    SetConsoleCtrlHandler(ctrl_handler, TRUE);

    HANDLE job = CreateJobObjectA(NULL, NULL);
    if (!job) {
        fprintf(stderr, "[launcher] CreateJobObjectA failed: %lu\n", GetLastError());
        return 125;
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION li = {0};
    li.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
        JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                 &li, sizeof li)) {
        fprintf(stderr, "[launcher] SetInformationJobObject failed: %lu\n",
                GetLastError());
        return 125;
    }

    char cmdline[4096];
    if (!build_cmdline(argc, argv, sep + 1, cmdline, sizeof cmdline)) {
        fprintf(stderr, "[launcher] command line too long\n");
        return 125;
    }

    STARTUPINFOA si = {0};
    si.cb         = sizeof si;
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION pi = {0};

    logline("spawning: %s", cmdline);
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE,
                        CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "[launcher] CreateProcessA failed: %lu\n", GetLastError());
        return 125;
    }
    if (!AssignProcessToJobObject(job, pi.hProcess)) {
        fprintf(stderr, "[launcher] AssignProcessToJobObject failed: %lu\n",
                GetLastError());
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
        return 125;
    }
    if (ResumeThread(pi.hThread) == (DWORD)-1) {
        fprintf(stderr, "[launcher] ResumeThread failed: %lu\n", GetLastError());
        TerminateJobObject(job, 1);
        CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
        return 125;
    }
    logline("child pid=%lu  timeout_ms=%s  grace_ms=%lu",
            pi.dwProcessId,
            (timeout_ms == INFINITE ? "inf" : "(see --timeout-ms)"),
            grace_ms);

    if (watch_stdin) CreateThread(NULL, 0, stdin_watcher, NULL, 0, NULL);

    HANDLE waits[2] = { pi.hProcess, g_shutdown_event };
    DWORD wr = WaitForMultipleObjects(2, waits, FALSE, timeout_ms);

    int rc;
    if (wr == WAIT_OBJECT_0) {
        DWORD child_rc = 1;
        GetExitCodeProcess(pi.hProcess, &child_rc);
        logline("child exited (rc=%lu)", child_rc);
        rc = (int)child_rc;
    } else {
        const char *reason =
            (wr == WAIT_TIMEOUT)        ? "timeout" :
            (wr == WAIT_OBJECT_0 + 1)   ? "shutdown-signal" :
                                          "wait-failed";
        logline("triggering graceful shutdown: %s (grace=%lu ms)",
                reason, grace_ms);

        EnumWindows(close_top_window, (LPARAM)pi.dwProcessId);
        DWORD wg = WaitForSingleObject(pi.hProcess, grace_ms);
        if (wg != WAIT_OBJECT_0) {
            logline("grace expired — TerminateJobObject");
            TerminateJobObject(job, 1);
            WaitForSingleObject(pi.hProcess, 1000);
        } else {
            DWORD child_rc = 1;
            GetExitCodeProcess(pi.hProcess, &child_rc);
            logline("child exited gracefully (rc=%lu)", child_rc);
        }
        rc = (wr == WAIT_TIMEOUT) ? 124 : 130;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    /* Closing the job handle here fires KILL_ON_JOB_CLOSE on any survivor
     * we missed.  Belt-and-braces; the explicit Terminate above usually
     * already finished the job. */
    CloseHandle(job);
    CloseHandle(g_shutdown_event);
    return rc;
}
