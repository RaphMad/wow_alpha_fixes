#include <Windows.h>
#include "tsc\tsc.h"
#include "minhook_134\include\MinHook.h"

// OsTimeManager::TimeKeeper() - supposed to read and populate CPU_TICKS_PER_SECOND, but actually never called?
#define TIMEKEEPER_PROC 0x0045c100

// OsGetAsyncClocksPerSecond() - accesses and initializes CPU_TICKS_PER_SECOND, but we initialize it earlier with our calibrated value
#define OS_GET_ASYNC_CLOCKS_PER_SECOND 0x0045b970

// OsGetAsyncThreadTimeMs(), OsGetAsyncTimeMs(), OsGetAsyncTimeMsPrecise() - will all be patched to work based on our calibrated value and rdtsc()
#define OS_GET_ASYNC_THREAD_TIME_MS 0x0045bd20
#define OS_GET_ASYNC_TIME_MS 0x0045bab0
#define OS_GET_ASYNC_TIME_MS_PRECISE 0x0045bac0
#define OS_GET_ASYNC_TIME_S 0x0045bb40

// cpuTicksPerSecond (DWORD64)
#define CPU_TICKS_PER_SECOND 0x00cc45a8

// CGxDevice::CpuFrequency() - accesses and initializes CPU_FREQUENCY, but we initialize it earlier with our calibrated value
#define CPU_FREQUENCY_FUNC 0x005951b0

// frequency (float 32bit)
#define CPU_FREQUENCY 0x00E1324C

volatile static DWORD64 tscCalibration;
volatile static float scaleTscToMs;

static unsigned int dummyTimeKeeper() {
    return 0;
}

static unsigned long long threadTimeMs() {
    return (unsigned long long)(__rdtsc() * scaleTscToMs);
}

static unsigned long timeMs() {
    return (unsigned long)(__rdtsc() * scaleTscToMs);
}

static unsigned long timeMsPrecise() {
    return (unsigned long)(__rdtsc() * scaleTscToMs);
}

static float timeS() {
    return (float)(__rdtsc() / tscCalibration);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            if (MH_Initialize() != MH_OK) return FALSE;

            tscCalibration = CalibrateTSC();
            scaleTscToMs = 1000.0 / (float)tscCalibration;

            // Patch cpuTicksPerSecond and cpuFrequency in place (before game functions get a chance to initialize them with less accurate values).
            DWORD64* cpuTicksPerSecondAddr = CPU_TICKS_PER_SECOND;
            *cpuTicksPerSecondAddr = tscCalibration;

            float* cpuFrequency = CPU_FREQUENCY;
            *cpuFrequency = (float)tscCalibration;

            // Prevent timekeeper from interferring with our changes
            if (MH_CreateHook(TIMEKEEPER_PROC, dummyTimeKeeper, NULL) != MH_OK) return FALSE;

            // Hook timing functions to work based off tsc
            if (MH_CreateHook(OS_GET_ASYNC_TIME_MS_PRECISE, timeMsPrecise, NULL) != MH_OK) return FALSE;
            if (MH_CreateHook(OS_GET_ASYNC_TIME_S, timeS, NULL) != MH_OK) return FALSE;
            if (MH_CreateHook(OS_GET_ASYNC_THREAD_TIME_MS, threadTimeMs, NULL) != MH_OK) return FALSE;

            // This function is used by movement calculations, hooking it will cause glitchy movement
            //if (MH_CreateHook(OS_GET_ASYNC_TIME_MS, timeMs, NULL) != MH_OK) return FALSE;

            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return FALSE;

            break;
        case DLL_PROCESS_DETACH:
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
    }

    return TRUE;
}
