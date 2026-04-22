#include <Windows.h>
#include "helpers\helpers.h"

// CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(), address of the jmp after checks for screen coordinates
#define SCREEN_COORDS_CHECK 0x0059b1b9

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // This fixes an assertion crash when "tabbing out" while on the loading screen.
        // Simply binary patch from `0x74`/`JZ` to `0xEB`/`JMP`
        BYTE* screenCoordsCheck = SCREEN_COORDS_CHECK;
        MakeWritable(screenCoordsCheck, sizeof(BYTE));
        *screenCoordsCheck = 0xEB;
    }

    return TRUE;
}
