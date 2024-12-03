#include "platform.h"
#ifdef PLATFORM_WIN32

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

static u64 ticks_per_sec = 1;

void plat_init(void) {
    LARGE_INTEGER perf_freq = { 0 };

    if (QueryPerformanceFrequency(&perf_freq)) {
        ticks_per_sec = (u64)perf_freq.QuadPart;
    }
}

u64 plat_time_usec(void) {
    LARGE_INTEGER ticks = { 0 };

    if (!QueryPerformanceCounter(&ticks)) {
        return 0;
    }

    return (u64)ticks.QuadPart * 1000000 / ticks_per_sec;
}

// returns NULL on failure
void* plat_mem_reserve(u64 size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
}
b32 plat_mem_commit(void* mem, u64 size) {
    void* ret = VirtualAlloc(mem, size, MEM_COMMIT, PAGE_READWRITE);
    return ret != NULL;
}
void plat_mem_decommit(void* mem, u64 size) {
    VirtualFree(mem, size, MEM_DECOMMIT);
}
void plat_mem_release(void* mem, u64 size) {
    VirtualFree(mem, size, MEM_RELEASE);
}

u32 plat_mem_page_size(void) {
    SYSTEM_INFO si = { 0 };
    GetSystemInfo(&si);
    return si.dwPageSize;
}

#endif // PLATFORM_WIN32

