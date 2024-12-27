#include "platform.h"
#ifdef PLATFORM_WIN32

#define WIN32_MEAN_AND_LEAN
#define UNICODE
#include <Windows.h>

static u64 ticks_per_sec = 1;

static string16 _utf16_from_utf8(mem_arena* arena, string8 str) {
    i32 out_size = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, (LPCCH)str.str, str.size, NULL, 0);

    if (out_size <= 0) {
        return (string16) { 0 };
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);
    // +1 for null terminator
    u16* out_data = ARENA_PUSH_ARRAY(maybe_temp.arena, u16, out_size + 1);
    i32 ret = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, (LPCCH)str.str, str.size, out_data, out_size);
    
    if (ret != out_size) {
        arena_temp_end(maybe_temp);
        return (string16){ 0 };
    }

    return (string16){ .str = out_data, .size = out_size };
}

void plat_init(void) {
    LARGE_INTEGER perf_freq = { 0 };

    if (QueryPerformanceFrequency(&perf_freq)) {
        ticks_per_sec = (u64)perf_freq.QuadPart;
    }
}

string8 plat_get_name(void) {
    return STR8_LIT("windows");
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

    string16 file_name16 = _utf16_from_utf8(scratch.arena, file_name);
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

    string16 file_name16 = _utf16_from_utf8(scratch.arena, file_name);

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
        DWORD to_read_clamped = (DWORD)MIN(to_read, (u64)(~(DWORD)0));

        DWORD cur_read = 0;
        if (!ReadFile(file_handle, out.str + total_read, to_read_clamped, &cur_read, NULL)) {
            CloseHandle(file_handle);
            arena_temp_end(maybe_temp);
            return (string8){ 0 };
        }

        total_read += cur_read;
    }

    CloseHandle(file_handle);

    return out;
}

void plat_file_write(string8 file_name, const string8_list* list, b32 append) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = _utf16_from_utf8(scratch.arena, file_name);

    arena_scratch_release(scratch);
}

b32 plat_file_delete(string8 file_name) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 file_name16 = _utf16_from_utf8(scratch.arena, file_name);
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

