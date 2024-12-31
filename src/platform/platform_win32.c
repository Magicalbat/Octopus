#include "platform.h"
#ifdef PLATFORM_WIN32

#define WIN32_MEAN_AND_LEAN
#define UNICODE
#include <Windows.h>

#define _DWORD_MAX (~(DWORD)0)

static u64 ticks_per_sec = 1;

void plat_init(void) {
    LARGE_INTEGER perf_freq = { 0 };

    if (QueryPerformanceFrequency(&perf_freq)) {
        ticks_per_sec = (u64)perf_freq.QuadPart;
    }
}

string8 plat_get_name(void) {
    return STR8_LIT("win32");
}

void plat_fatal_error(const char* msg) {
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
    ExitProcess(1);
}

u64 plat_time_usec(void) {
    LARGE_INTEGER ticks = { 0 };

    if (!QueryPerformanceCounter(&ticks)) {
        return 0;
    }

    return (u64)ticks.QuadPart * 1000000 / ticks_per_sec;
}

u64 plat_file_size(string8 file_name) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = str16_from_str8(scratch.arena, file_name, true);
    WIN32_FILE_ATTRIBUTE_DATA attribs = { 0 };
    b32 ret = GetFileAttributesExW(file_name16.str, GetFileExInfoStandard, &attribs);

    arena_scratch_release(scratch);

    if (ret == false) {
        return 0;
    }

    return ((u64)attribs.nFileSizeHigh << 32) | (u64)attribs.nFileSizeLow;
}

string8 plat_file_read(mem_arena* arena, string8 file_name) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = str16_from_str8(scratch.arena, file_name, true);

    HANDLE file_handle = CreateFileW(
        (LPCWSTR)file_name16.str, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
    );

    arena_scratch_release(scratch);

    if (file_handle == INVALID_HANDLE_VALUE) {
        return (string8){ 0 };
    }

    LARGE_INTEGER file_size = { 0 };
    if (!GetFileSizeEx(file_handle, &file_size)) {
        return (string8){ 0 };
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);

    string8 out = { 
        .size = file_size.QuadPart,
        .str = ARENA_PUSH_ARRAY(maybe_temp.arena, u8, file_size.QuadPart)
    };

    u64 total_read = 0;
    while (total_read < out.size) {
        u64 to_read = out.size - total_read;
        DWORD to_read_capped = (DWORD)MIN(to_read, (u64)_DWORD_MAX);

        DWORD cur_read = 0;
        if (!ReadFile(file_handle, out.str + total_read, to_read_capped, &cur_read, NULL)) {
            arena_temp_end(maybe_temp);
            out = (string8){ 0 };

            break;
        }

        total_read += cur_read;
    }

    CloseHandle(file_handle);

    return out;
}

b32 plat_file_write(string8 file_name, const string8_list* list, b32 append) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = str16_from_str8(scratch.arena, file_name, true);
    HANDLE file_handle = CreateFileW(
        (LPCWSTR)file_name16.str,
        append ? FILE_APPEND_DATA : GENERIC_WRITE,
        0, NULL,
        append ? OPEN_ALWAYS : CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    arena_scratch_release(scratch);

    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    b32 out = true;

    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8 full_file = str8_concat(scratch.arena, list);

        u64 total_written = 0;
        while (total_written < full_file.size) {
            u64 to_write = full_file.size - total_written;
            DWORD to_write_capped = (DWORD)MIN(to_write, (u64)_DWORD_MAX);

            DWORD written = 0;
            if (!WriteFile(file_handle, full_file.str + total_written, to_write_capped, &written, NULL)) {
                out = false;

                break;
            }

            total_written += written;
        }

        arena_scratch_release(scratch);
    }

    CloseHandle(file_handle);

    return out;
}

b32 plat_file_delete(string8 file_name) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = str16_from_str8(scratch.arena, file_name, true);
    b32 ret = DeleteFileW((LPCWSTR)file_name16.str);

    arena_scratch_release(scratch);

    return ret;
}

void plat_get_entropy(void* data, u64 size) {
    BCryptGenRandom(NULL, data, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
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

