/* inject.c — launch the (unpacked) retail exe SUSPENDED, force-load our proxy DLL
 * by FULL PATH via a remote LoadLibraryA, then resume.  This sidesteps the ddraw
 * DLL-search-order problem entirely (app-dir shadowing of ddraw.dll is unreliable —
 * System32 ddraw wins, ckpt 164): our DLL loads regardless of its name, BEFORE the
 * main thread runs, so engine_hooks_install patches the mapped sotes code and the
 * harness can dismiss the launcher in-process.  32-bit (matches sotes.exe); kernel32
 * is at a fixed base across 32-bit processes so our LoadLibraryA addr is valid in the
 * target.  Usage: inject.exe <exe> <dll-fullpath> <cwd> */
#include <windows.h>
#include <stdio.h>
int main(int argc, char **argv){
    if (argc < 4){ fprintf(stderr, "usage: inject <exe> <dll-fullpath> <cwd>\n"); return 2; }
    const char *exe = argv[1], *dll = argv[2], *cwd = argv[3];
    STARTUPINFOA si; PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof si); si.cb = sizeof si; memset(&pi, 0, sizeof pi);
    if (!CreateProcessA(exe, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, cwd, &si, &pi)){
        fprintf(stderr, "CreateProcess(%s) failed %lu\n", exe, GetLastError()); return 1;
    }
    size_t n = strlen(dll) + 1;
    void *rem = VirtualAllocEx(pi.hProcess, NULL, n, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!rem || !WriteProcessMemory(pi.hProcess, rem, dll, n, NULL)){
        fprintf(stderr, "WriteProcessMemory failed %lu\n", GetLastError()); TerminateProcess(pi.hProcess,1); return 1;
    }
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE ll = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryA");
    HANDLE th = CreateRemoteThread(pi.hProcess, NULL, 0, ll, rem, 0, NULL);
    if (!th){ fprintf(stderr, "CreateRemoteThread failed %lu\n", GetLastError()); TerminateProcess(pi.hProcess,1); return 1; }
    WaitForSingleObject(th, 15000);
    DWORD loaded = 0; GetExitCodeThread(th, &loaded);
    CloseHandle(th);
    printf("injected '%s' -> remote HMODULE-low=0x%lx; resuming pid=%lu\n", dll, loaded, pi.dwProcessId);
    ResumeThread(pi.hThread);
    printf("pid=%lu\n", pi.dwProcessId);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return loaded ? 0 : 1;
}
