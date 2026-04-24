#include <Windows.h>
#include "helpers\helpers.h"

// CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(), address of the jmp after checks for screen coordinates
#define SCREEN_COORDS_CHECK 0x0059b1b9

// call to SoundFileCache::Shutdown on game exit (5 bytes)
#define SOUND_FILE_CACHE_SHUTDOWN 0x007b52c7

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // This fixes an assertion crash when "tabbing out" while on a full screen loading screen.
        // Simply binary patch from `0x74`/`JZ` to `0xEB`/`JMP`
        BYTE* screenCoordsCheck = SCREEN_COORDS_CHECK;
        MakeWritable(screenCoordsCheck, sizeof(BYTE));
        *screenCoordsCheck = 0xEB;

        // Shutting down the soundfile cache will sometimes get stuck when closing the game.
        // This prevents a proper exit and requires to kill the game via taskmgr.
        // Simply overwrite call to shutdown with nops.
        BYTE nops[5] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
        MakeWritable(SOUND_FILE_CACHE_SHUTDOWN, sizeof(nops));
        memcpy(SOUND_FILE_CACHE_SHUTDOWN, nops, sizeof(nops));
    }

    return TRUE;
}
