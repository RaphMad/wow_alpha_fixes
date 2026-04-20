#include "tsc\tsc.h"

typedef unsigned long addr_t;

// OsTimeManager::TimeKeeper() - supposed to read and populate CPU_TICKS_PER_SECOND, but actually never called?
#define TIMEKEEPER_PROC 0x0045c100

// OsGetAsyncClocksPerSecond() - accesses and initializes CPU_TICKS_PER_SECOND, but we initialize it earlier with our calibrated value
#define GET_ASYNC_CLOCKS_PER_SECOND 0x0045b970

// cpuTicksPerSecond (DWORD64)
#define CPU_TICKS_PER_SECOND 0x00cc45a8

// CGxDevice::CpuFrequency() - accesses and initializes FREQUENCY, but we initialize it earlier with our calibrated value
#define CPU_FREQUENCY_FUNC 0x005951b0

// frequency (float 32bit)
#define CPU_FREQUENCY 0x00E1324C

volatile static DWORD64 tscCalibration;

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        tscCalibration = CalibrateTSC();

        // Patch cpuTicksPerSecond and cpuFrequency in place (before game functions get a chance to initialize them with less accurate values).
        DWORD64* cpuTicksPerSecondAddr = CPU_TICKS_PER_SECOND;
        *cpuTicksPerSecondAddr = tscCalibration;

        float* cpuFrequency = CPU_FREQUENCY;
        *cpuFrequency = (float)tscCalibration;
    }

    return TRUE;
}
