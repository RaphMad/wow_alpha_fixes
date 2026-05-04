#include <Windows.h>

void MakeWritable(LPVOID memory, SIZE_T size) {
    DWORD oldProtect;
    VirtualProtect(memory, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}
