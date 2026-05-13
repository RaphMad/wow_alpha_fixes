#include <Windows.h>
#include "helpers\helpers.h"

#define WRITE_APP_IS_ACTIVE 0x0046b01a
#define FSOUND_SetHWND 0x007b4daa

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Prevent write to s_AppIsActive
        // This makes the game think it's always focused (s_AppIsActive stays 1)
        // To check: Might cause unwanted side-effects, but fix works with just the patch below?
        //BYTE nops[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
        //MakeWritable(WRITE_APP_IS_ACTIVE, sizeof(nops));
        //memcpy(WRITE_APP_IS_ACTIVE, nops, sizeof(nops));

        // Prevents FMOD from detecting window focus loss
        // (uses cdecl calling convention, so need to clean parameters from stack - 0x83, 0xC4, 0x04 = "ADD ESP, 4")
        BYTE patch[5] = { 0x83, 0xC4, 0x04, 0x90, 0x90 };
        MakeWritable(FSOUND_SetHWND, sizeof(patch));
        memcpy(FSOUND_SetHWND, patch, sizeof(patch));
    }

    return TRUE;
}
