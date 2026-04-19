#include "minhook_134\include\MinHook.h"
#include <Windows.h>
#include <psapi.h>
#include <timeapi.h>

typedef unsigned long addr_t;

// OsTimeManager::TimeKeeper()
#define TIMEKEEPER_PROC_RVA 0x0045c100

// cpuTicksPerSecond
#define CPU_TICKS_PER_SECOND_RVA 0x00cc45a8

// CGxDevice::CpuFrequency()
#define CPU_FREQUENCY_RVA 0x005951b0

// frequency
#define FREQUENCY_RVA 0x00E1324C

static addr_t moduleBase;

static volatile HINSTANCE hinstDll = NULL;
static volatile DWORD64 tscFrequency = 0;
static volatile cpuFrequencyWritten = FALSE;

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

    // Save original thread settings
    DWORD_PTR oldAffinity = SetThreadAffinityMask(hThread, 1 << GetCurrentProcessorNumber());
    int oldPriority = GetThreadPriority(hThread);

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
        _mm_lfence();

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
            if (tickDelta >= expectedTicks - 50 && tickDelta <= expectedTicks + 50) {
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
        tscFreq = (double)bestTscDelta * (double)qpcFreq.QuadPart / (double)bestQpcDelta;
    }

    // Restore settings
    timeEndPeriod(1);
    SetThreadAffinityMask(hThread, oldAffinity);
    SetThreadPriority(hThread, oldPriority);

    return (DWORD64)round(tscFreq);
}

static DWORD WINAPI HookedTimeKeeperProc(LPVOID lpParameter) {
    tscFrequency = CalibrateTSC();

    DWORD64* cpuTicksPerSecondAddr = moduleBase + CPU_TICKS_PER_SECOND_RVA;
    *cpuTicksPerSecondAddr = tscFrequency;

    while(!cpuFrequencyWritten) {
		Sleep(1);
	}

    // Unload dll and remove hooks
	FreeLibraryAndExitThread(hinstDll, 0);
}

static float WINAPI HookedCpuFrequency() {
    // Wait for TSC calibration to complete
    while (tscFrequency == 0) {
        Sleep(1);
    }

    // The game assumes the TSC frequency is equal to the CPU frequency
    float frequency = (float)tscFrequency;

    // Update memory location read by original function, so it will still be used when dll gets unloaded
    float* frequencyAddr = moduleBase + FREQUENCY_RVA;
    *frequencyAddr = frequency;

    cpuFrequencyWritten = TRUE;

    return frequency;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    hinstDll = h;

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            if (MH_Initialize() != MH_OK) return FALSE;

            moduleBase = GetBaseAddress();
            addr_t timekeeperProc = moduleBase + TIMEKEEPER_PROC_RVA;
            addr_t cpuFrequencyFunc = moduleBase + CPU_FREQUENCY_RVA;

            if (MH_CreateHook(timekeeperProc, HookedTimeKeeperProc, NULL) != MH_OK) return FALSE;
            if (MH_CreateHook(cpuFrequencyFunc, HookedCpuFrequency, NULL) != MH_OK) return FALSE;

            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return FALSE;

            break;
        case DLL_PROCESS_DETACH:
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
    }

    return TRUE;
}
