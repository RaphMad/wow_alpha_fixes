// Credits to https://github.com/hannesmann/vanillafixes/blob/main/src/dll/tsc.c

#include <Windows.h>
#include <math.h>

typedef NTSTATUS (NTAPI *PNT_QUERY_TIMER_RESOLUTION)(PULONG, PULONG, PULONG);
typedef NTSTATUS (NTAPI *PNT_SET_TIMER_RESOLUTION)(ULONG, BOOLEAN, PULONG);

static BOOL increaseTimerResolution(ULONG maxRequestedResolution) {
	static PNT_QUERY_TIMER_RESOLUTION fnNtQueryTimerResolution = NULL;
	static PNT_SET_TIMER_RESOLUTION fnNtSetTimerResolution = NULL;

	// If the NT timer functions are not already loaded, attempt to load them
	if(!fnNtQueryTimerResolution || !fnNtSetTimerResolution) {
		HMODULE hNtDll = GetModuleHandle(L"ntdll");
		fnNtQueryTimerResolution = (PNT_QUERY_TIMER_RESOLUTION)GetProcAddress(hNtDll, "NtQueryTimerResolution");
		fnNtSetTimerResolution = (PNT_SET_TIMER_RESOLUTION)GetProcAddress(hNtDll, "NtSetTimerResolution");

		// Use timeBeginPeriod if NtSetTimerResolution is unavailable
		if(!fnNtQueryTimerResolution || !fnNtSetTimerResolution) {
			UINT resolution = max(1, maxRequestedResolution / 10000);
			return timeBeginPeriod(resolution) == TIMERR_NOERROR;
		}
	}

	ULONG minSupportedResolution, maxSupportedResolution, currentResolution;
	fnNtQueryTimerResolution(&minSupportedResolution, &maxSupportedResolution, &currentResolution);

	ULONG desiredResolution = max(maxSupportedResolution, maxRequestedResolution);

	if(fnNtSetTimerResolution(desiredResolution, TRUE, &currentResolution) != 0) {
		// Fall back to timeBeginPeriod if NtSetTimerResolution failed
		UINT resolution = max(1, maxRequestedResolution / 10000);
		return timeBeginPeriod(resolution) == TIMERR_NOERROR;
	}

	return TRUE;
}

typedef BOOL (WINAPI *PSET_PROCESS_INFORMATION)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);

static BOOL setPowerThrottlingState(ULONG feature, BOOL enable) {
	static PSET_PROCESS_INFORMATION fnSetProcessInformation = NULL;

	// If the SetProcessInformation function is not already loaded, attempt to load it
	if(!fnSetProcessInformation) {
		HMODULE hKernel32 = GetModuleHandle(L"kernel32");
		fnSetProcessInformation = (PSET_PROCESS_INFORMATION)GetProcAddress(hKernel32, "SetProcessInformation");

		// If getting the function address failed, return FALSE to indicate an error
		if(!fnSetProcessInformation) {
			return FALSE;
		}
	}

	PROCESS_POWER_THROTTLING_STATE powerThrottlingState = {0};
	powerThrottlingState.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
	powerThrottlingState.ControlMask = feature;
	powerThrottlingState.StateMask = enable ? feature : 0;

	return fnSetProcessInformation(
		GetCurrentProcess(),
		ProcessPowerThrottling,
		&powerThrottlingState,
		sizeof(PROCESS_POWER_THROTTLING_STATE)
	);
}

static inline void timeSample(PLARGE_INTEGER pQpc, PDWORD64 pTsc) {
	// Try to request a new timeslice before sampling timestamp counters
	Sleep(0);

	QueryPerformanceCounter(pQpc);
	*pTsc = __rdtsc();

	// Wait for QPC and RDTSC to finish
	_mm_lfence();
}

DWORD64 CalibrateTSC() {
    // Increase the process timer resolution up to a cap of 0.5 ms
    increaseTimerResolution(5000);

    // Disable Windows 11 power throttling for the process
    setPowerThrottlingState(PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION, FALSE);
    setPowerThrottlingState(PROCESS_POWER_THROTTLING_EXECUTION_SPEED, FALSE);

	int oldPriority = GetThreadPriority(GetCurrentThread());
	// Pin on current core and run with high priority
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	SetThreadAffinityMask(GetCurrentThread(), 1 << (GetCurrentProcessorNumber()));

	// Obtain a baseline time for comparison
	LARGE_INTEGER baselineQpc;
	DWORD64 baselineTsc;
	timeSample(&baselineQpc, &baselineTsc);

	Sleep(500);

	// Obtain new value after sleep
	LARGE_INTEGER diffQpc;
	DWORD64 diffTsc;
	timeSample(&diffQpc, &diffTsc);

	// Calculate the relative time spent in sleep (>= 500 ms)
	diffQpc.QuadPart -= baselineQpc.QuadPart;
	diffTsc -= baselineTsc;

	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

	double elapsedTime = diffQpc.QuadPart / (double)performanceFrequency.QuadPart;
	double estimatedTscFrequency = diffTsc / elapsedTime;

	// Restore old priority and affinity
	SetThreadPriority(GetCurrentThread(), oldPriority);
	SetThreadAffinityMask(GetCurrentThread(), 0);

	return (DWORD64)round(estimatedTscFrequency);
}
