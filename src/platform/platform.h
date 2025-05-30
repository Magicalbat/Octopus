#if defined(PLATFORM_WIN32)

#define WIN32_MEAN_AND_LEAN
#define UNICODE
#include <Windows.h>

#elif defined(PLATFORM_LINUX)

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#endif

void plat_init(void);

// Do not modify the string 
string8 plat_get_name(void);

void plat_fatal_error(const char* msg);

u64 plat_time_usec(void);
void plat_sleep_ms(u32 ms);

u64 plat_file_size(string8 file_name);
string8 plat_file_read(mem_arena* arena, string8 file_name);
b32 plat_file_write(string8 file_name, const string8_list* list, b32 append);
b32 plat_file_delete(string8 file_name);

void plat_get_entropy(void* data, u64 size);

// returns NULL on failure
void* plat_mem_reserve(u64 size);
b32 plat_mem_commit(void* mem, u64 size);
void plat_mem_decommit(void* mem, u64 size);
void plat_mem_release(void* mem, u64 size);

u32 plat_mem_page_size(void);

