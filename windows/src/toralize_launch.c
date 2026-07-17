#include <stdio.h>
#include <stdlib.h>
#include "socks4.h"
#include "utils.h"
//Win Socket
#include <winsock2.h>
#include <ws2tcpip.h>

static int inject(HANDLE proc, const char *dllPath) {
    SIZE_T len = strlen(dllPath) + 1;

    void *mem = VirtualAllocEx(proc, NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) { fprintf(stderr, "VirtualAllocEx failed: %lu\n", GetLastError()); return -1; }

    if (!WriteProcessMemory(proc, mem, dllPath, len, NULL)) {
        fprintf(stderr, "WriteProcessMemory failed: %lu\n", GetLastError());
        return -1;
    }

    HMODULE k32 = GetModuleHandleA("kernel32.dll");                       // kernel32, not your dll
    LPTHREAD_START_ROUTINE loadlib =
        (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryA");      // LoadLibraryA
    if (!loadlib) { fprintf(stderr, "GetProcAddress(LoadLibraryA) failed\n"); return -1; }

    HANDLE t = CreateRemoteThread(proc, NULL, 0, loadlib, mem, 0, NULL);  // runs LoadLibraryA(mem) in curl
    if (!t) { fprintf(stderr, "CreateRemoteThread failed: %lu\n", GetLastError()); return -1; }

    WaitForSingleObject(t, INFINITE);

    DWORD remoteModule = 0;
    GetExitCodeThread(t, &remoteModule);   // 0 => LoadLibrary returned NULL => dll failed to load
    CloseHandle(t);
    VirtualFreeEx(proc, mem, 0, MEM_RELEASE);

    if (remoteModule == 0) {
        fprintf(stderr, "LoadLibraryA in target failed (dll not loaded)\n");
        return -1;
    }
    return 0;
}

#define MAX_CMD_LINE 4096

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args ....]", argv[0]);
        return EXIT_FAILURE;
    }
    /* 1. join argv[1..] into a command line (curl + its args) */
    char cmdline[MAX_CMD_LINE] = {0};
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat_s(cmdline, sizeof(cmdline), " ");
        if (strchr(argv[i], ' ')) {                       // quote args with spaces
            strcat_s(cmdline, sizeof(cmdline), "\"");
            strcat_s(cmdline, sizeof(cmdline), argv[i]);
            strcat_s(cmdline, sizeof(cmdline), "\"");
        } else {
            strcat_s(cmdline, sizeof(cmdline), argv[i]);
        }
    }

    /* absolute path to toralize.dll — right next to this launcher exe */
    char dllPath[MAX_PATH];
    GetModuleFileNameA(NULL, dllPath, MAX_PATH);          // full path of THIS exe
    char *slash = strrchr(dllPath, '\\');
    if (slash) slash[1] = '\0';                           // strip exe name, keep folder
    strcat_s(dllPath, sizeof(dllPath), "toralize.dll");

    HANDLE hooksReady = CreateEventA(NULL, TRUE, FALSE, "ToralizeHooksReady");
    /* 2. start the child suspended */
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                        CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "CreateProcess failed: %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    /* 3. inject */
    if (inject(pi.hProcess, dllPath) != 0) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return EXIT_FAILURE;
    }

    /* 4. wait for the DLL to finish installing hooks, then resume */
    WaitForSingleObject(hooksReady, 5000);
    ResumeThread(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);               // pass through curl's exit code
    if (hooksReady) CloseHandle(hooksReady);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return (int)code;
}
