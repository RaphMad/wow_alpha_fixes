#include "minhook_134\include\MinHook.h"
#include <Windows.h>
#include <psapi.h>
#include <timeapi.h>

typedef unsigned long long addr_t;

#define TIMEKEEPER_RVA 0x0045c100
#define CPU_TICKS_PER_SECOND_RVA 0x00cc45a8

static addr_t moduleBase;

static addr_t GetBaseAddress() {
    // Enumerating a single module is enough, since the first module always is the executable itself.
    HMODULE module;
    DWORD numModules;

    if (EnumProcessModulesEx(GetCurrentProcess(), &module, sizeof(HMODULE), &numModules, LIST_MODULES_ALL)) {
        // HMODULE actually represents the base address of a module
        return module;
    }

    return 0;
}

static DWORD64 CalibrateTSC() {
    HANDLE hThread = GetCurrentThread();
    DWORD_PTR oldAffinity;
    int oldPriority;

    // Save original thread settings
    oldAffinity = SetThreadAffinityMask(hThread, 1ULL << 0);
    oldPriority = GetThreadPriority(hThread);

    // Boost priority
    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

    // Increase timer resolution
    timeBeginPeriod(1);

    // Multiple samples for accuracy
    DWORD64 samples[5];
    DWORD ticks[5];
    DWORD64 qpcSamples[5];
    LARGE_INTEGER qpcFreq;
    QueryPerformanceFrequency(&qpcFreq);

    for (int i = 0; i < 5; i++) {
        // Request timeslice before sampling
        Sleep(0);

        QueryPerformanceCounter((LARGE_INTEGER*)&qpcSamples[i]);
        samples[i] = __rdtsc();
        ticks[i] = GetTickCount();

        Sleep(100);  // 100ms between samples
    }

    // Find most consistent window around calibration target
    int bestStart = 0;
    int bestCount = 0;
    DWORD64 bestQpcDelta = 0;
    DWORD64 bestTscDelta = 0;

    for (int i = 0; i < 5; i++) {
        for (int j = i + 2; j <= 5; j++) {
            DWORD64 qpcDelta = qpcSamples[j-1] - qpcSamples[i];
            DWORD64 tscDelta;
            if (samples[j-1] >= samples[i]) {
                tscDelta = samples[j-1] - samples[i];
            } else {
                // Handle TSC wraparound
                tscDelta = samples[i] - samples[j-1];
            }
            DWORD tickDelta = ticks[j-1] - ticks[i];

            // Check if window is within acceptable range
            int windowSize = j - i;
            int expectedTicks = windowSize * 100;  // Each sleep is 100ms
            if (tickDelta >= expectedTicks - 50 &&
                tickDelta <= expectedTicks + 50) {
                // Found good window
                if (qpcDelta > bestQpcDelta) {
                    bestQpcDelta = qpcDelta;
                    bestTscDelta = tscDelta;
                    bestStart = i;
                    bestCount = j - i;
                }

                break;
            }
        }
    }

    // Calculate frequency using QPC as reference
    double tscFreq = 0;

    if (bestQpcDelta > 0 && qpcFreq.QuadPart > 0) {
        tscFreq = (double)bestTscDelta * (double)qpcFreq.QuadPart /
                  (double)bestQpcDelta;
    }

    // Restore settings
    timeEndPeriod(1);
    SetThreadAffinityMask(hThread, oldAffinity);
    SetThreadPriority(hThread, oldPriority);

    return (DWORD64)round(tscFreq);
}

static DWORD WINAPI HookedTimeKeeper() {
    DWORD64 g_tsc_frequency = CalibrateTSC();

    // Calculate derived values
    if (g_tsc_frequency > 0) {
        double g_timer_to_ms = 1000.0 / (double)g_tsc_frequency;

        // Calculate offset to align with GetTickCount
        DWORD64 currentTsc = __rdtsc();
        DWORD currentTick = GetTickCount();
        INT64 g_timer_offset = (INT64)currentTick - (INT64)(currentTsc * g_timer_to_ms);
    }

    // Write to global variable (CpuTicksPerSecond)
    DWORD64* cpuTicksPerSecond = moduleBase + CPU_TICKS_PER_SECOND_RVA;
    *cpuTicksPerSecond = g_tsc_frequency;

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            if (MH_Initialize() != MH_OK) return FALSE;

            moduleBase = GetBaseAddress();
            addr_t timekeeperFunc = moduleBase + TIMEKEEPER_RVA;

            if (MH_CreateHook(timekeeperFunc, HookedTimeKeeper, NULL) != MH_OK) return FALSE;

            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return FALSE;

            break;
        case DLL_PROCESS_DETACH:
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
    }

    return TRUE;
}
