#include <Windows.h>
#include "helpers\helpers.h"

// CGxDeviceOpenGl::CapsWindowSizeInScreenCoords(), check for screen coordinates
#define SCREEN_COORDS_CHECK 0x0059b1b9

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Patch jump for assertion about screen coordinates.
        char* screenCoordsCheck = SCREEN_COORDS_CHECK;
        MakeWritable(screenCoordsCheck);
        *screenCoordsCheck = 0xEB;
    }

    return TRUE;
}
