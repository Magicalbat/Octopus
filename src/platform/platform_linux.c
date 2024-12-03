#include "platform.h"
#ifdef PLATFORM_LINUX

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

void plat_init(void) { }

u64 plat_time_usec(void) {
    struct timespec ts = { 0 };
    if (-1 == clock_gettime(CLOCK_MONOTONIC, &ts)) {
        return 0;
    }

    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

u64 plat_file_size(string8 file_name) {
    struct stat file_stats = { 0 };

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u8* name_cstr = str8_to_cstr(scratch.arena, file_name);
    i32 ret = stat((char*)name_cstr, &file_stats);

    arena_scratch_release(scratch);

    if (ret == -1) {
        return 0;
    }

    return file_stats.st_size;
}

string8 plat_file_read(mem_arena* arena, string8 file_name) {
    i32 fd = -1;

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u8* name_cstr = str8_to_cstr(arena, file_name);
    fd = open((char*)name_cstr, O_RDONLY);

    arena_scratch_release(scratch);

    if (fd == -1) {
        return (string8){ 0 };
    }

    string8 out = { 0 };

    struct stat file_stats = { 0 };
    if (fstat(fd, &file_stats) == -1) {
        goto end;
    }

    if (!S_ISREG(file_stats.st_mode)) {
        goto end;
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);

    out.size = file_stats.st_size;
    out.str = ARENA_PUSH_ARRAY(maybe_temp.arena, u8, out.size);

    u64 str_pos = 0;

    while (str_pos < out.size) {
        i64 bytes_read = read(fd, out.str + str_pos, out.size - str_pos);

        if (bytes_read == -1) {
            arena_temp_end(maybe_temp);
            out = (string8){ 0 };
            goto end;
        }

        str_pos += bytes_read;
    }

end:
    close(fd);
    return out;
}

// TODO: benchmark multiple small writes vs one large write
void plat_file_write(mem_arena* arena, string8 file_name, const string8_list* list);

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

