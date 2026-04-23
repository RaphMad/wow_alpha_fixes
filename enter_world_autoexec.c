#include <Windows.h>
#include "minhook_134\include\MinHook.h"
#include <stdio.h>

#define CONSOLE_COMMAND_EXECUTE 0x0062cef0
#define CG_GAMEUI_ENTER_WORLD 0x004e65f0

volatile static FILE *autoExecFile;
static char line[256];

static void(__fastcall *consoleCommandExecute)(char*, int) = CONSOLE_COMMAND_EXECUTE;

static void executeConsoleCommand(char* command) {
    // 2nd param = 'addToHistory' -> no
    consoleCommandExecute(command, 0);
}

void(*origEnterWorld)(void);

static void enterWorld() {
    origEnterWorld();

    // Execute commands line by line
    while (fgets(line, sizeof(line), autoExecFile) != NULL) {
        // Remove trailing newlines
        line[strcspn(line, "\r\n")] = 0x0;

        if (strlen(line) > 0) {
            executeConsoleCommand(line);
        }
    }

    // Reset cursor to start of file for subsequent calls
    rewind(autoExecFile);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            autoExecFile = fopen("WTF\\autoexec.wtf", "r");

            if (autoExecFile) {
                if (MH_Initialize() != MH_OK) return FALSE;

                if (MH_CreateHook(CG_GAMEUI_ENTER_WORLD, enterWorld, &origEnterWorld) != MH_OK) return FALSE;
                if (MH_EnableHook(CG_GAMEUI_ENTER_WORLD) != MH_OK) return FALSE;
            }

            break;
        case DLL_PROCESS_DETACH:
            if (autoExecFile) {
                fclose(autoExecFile);
                MH_DisableHook(MH_ALL_HOOKS);
                MH_Uninitialize();
            }
    }

    return TRUE;
}
