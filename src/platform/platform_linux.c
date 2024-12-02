#include "platform.h"
#ifdef PLATFORM_LINUX

#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

void plat_init(void) { }

u64 plat_time_usec(void) {
    struct timespec ts = { 0 };
    if (-1 == clock_gettime(CLOCK_MONOTONIC, &ts)) {
        return 0;
    }

    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void* plat_mem_reserve(u64 size) {
    void* out = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (out == MAP_FAILED) {
        out = NULL;
    }
    return out;
}
b32 plat_mem_commit(void* mem, u64 size) {
    i32 ret = mprotect(mem, size, PROT_READ | PROT_WRITE);
    return ret == 0;
}
void plat_mem_decommit(void* mem, u64 size) {
    mprotect(mem, size, PROT_NONE);
    madvise(mem, size, MADV_DONTNEED);
}
void plat_mem_release(void* mem, u64 size) {
    munmap(mem, size);
}

u32 plat_mem_page_size(void) {
    return (u32)sysconf(_SC_PAGESIZE);
}

#endif // PLATFORM_LINUX

