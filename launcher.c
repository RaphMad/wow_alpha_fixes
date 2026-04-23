#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static void exitErr(LPCTSTR msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    fprintf(stderr, "\nUsage: launcher.exe WoWClient.exe <fix1.dll fix2.dll ...>");

    exit(EXIT_FAILURE);
}

static LPTSTR getLastErrorMessage() {
	LPTSTR errorMessage = NULL;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

	if (FormatMessage(flags, NULL, GetLastError(), 0, errorMessage, 0, NULL)) {
		return errorMessage;
	}

	return "Unknown error";
}

static void injectDll(HANDLE remoteProcess, LPCTSTR dllFile) {
    LPVOID remoteBuffer;
    LPVOID kernel32_LoadLibraryA;
    HANDLE injectorRemoteThread;

    printf("dll file: %s\n", dllFile);

    size_t bytesToAlloc = strlen(dllFile) + 1;

    if (!(remoteBuffer = VirtualAllocEx(remoteProcess, NULL, bytesToAlloc, MEM_COMMIT, PAGE_READWRITE)))
        exitErr("Could not allocate remote memory");
    puts("Allocated remote memory");

    if (!WriteProcessMemory(remoteProcess, remoteBuffer, dllFile, bytesToAlloc, NULL))
        exitErr("Could not write target dll name to remote process");
    puts("Wrote target dll name to remote process");

    if (!(kernel32_LoadLibraryA = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA")))
        exitErr("Could not find LoadLibraryA()");
    printf("Found kernel32.dll!LoadLibraryA() at address %p\n", kernel32_LoadLibraryA);

    if (!(injectorRemoteThread = CreateRemoteThread(remoteProcess, NULL, 0, kernel32_LoadLibraryA, remoteBuffer, 0, NULL)))
        exitErr("Could not create remote thread");

    WaitForSingleObject(injectorRemoteThread, INFINITE);
    puts("\nSUCCESFULLY INJECTED DLL!");

    VirtualFreeEx(remoteProcess, remoteBuffer, 0, MEM_RELEASE);
    CloseHandle(injectorRemoteThread);
}

int main(int argc, char** argv) {
    if (argc == 1)
        exitErr("");

    STARTUPINFO startupInfo = {0};
    PROCESS_INFORMATION processInfo;

    if (!CreateProcess(argv[1], NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startupInfo, &processInfo))
        exitErr("Error creating target process: %s", getLastErrorMessage());

    for (unsigned int i = 2; i < argc; i++) {
        injectDll(processInfo.hProcess, argv[i]);
    }

    ResumeThread(processInfo.hThread);

    CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

    return EXIT_SUCCESS;
}
