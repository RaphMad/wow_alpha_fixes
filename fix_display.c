#include "minhook_134\include\MinHook.h"
#include <WinUser.h>
#include <Windows.h>

// Based on https://github.com/Mates1500/VanillaMultiMonitorFix/blob/master/src/VanillaMultiMonitorFix/dllmain.cpp
typedef BOOL(WINAPI* EnumDisplayDevicesA_t)(LPCSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICEA lpDisplayDevice, DWORD dwFlags);

static DWORD MAX_DISPLAY_DEVICES;
static DWORD PREFERRED_DISPLAY_INDEX;

static EnumDisplayDevicesA_t originalEnumDisplayDevicesA;

static void enumerateDisplayDevices() {
    DWORD counter = 0;
    DWORD maxWidth = 0;

    DISPLAY_DEVICEA res;
    res.cb = sizeof(DISPLAY_DEVICEA);

    DEVMODE dev;

    while (EnumDisplayDevicesA(NULL, counter, &res, 0)) {
        EnumDisplaySettingsA(res.DeviceName, NULL, &dev);

        if (dev.dmPelsWidth > maxWidth) {
            maxWidth = dev.dmPelsWidth;
            PREFERRED_DISPLAY_INDEX = counter;
        }

        counter++;
    }

    MAX_DISPLAY_DEVICES = counter;
}

static BOOL WINAPI hookedEnumDisplayDevicesA(LPCSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICEA lpDisplayDevice, DWORD dwFlags)
{
    if (iDevNum < MAX_DISPLAY_DEVICES) {
        return originalEnumDisplayDevicesA(lpDevice, PREFERRED_DISPLAY_INDEX, lpDisplayDevice, dwFlags);
    }

    return originalEnumDisplayDevicesA(lpDevice, iDevNum, lpDisplayDevice, dwFlags);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            if (MH_Initialize() != MH_OK) return FALSE;

            enumerateDisplayDevices();

            if (MH_CreateHookApi(L"user32", "EnumDisplayDevicesA", &hookedEnumDisplayDevicesA, &originalEnumDisplayDevicesA) != MH_OK) return FALSE;
            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return FALSE;

            break;
        case DLL_PROCESS_DETACH:
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
    }

    return TRUE;
}
