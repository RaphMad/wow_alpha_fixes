#include <Windows.h>

void MakeWritable(LPVOID memory) {
    DWORD oldProtect;
    VirtualProtect(memory, 1024, PAGE_EXECUTE_READWRITE, &oldProtect);
}
