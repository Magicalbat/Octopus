#include <stdio.h>

#include "base/base.h"
#include "platform/platform.h"

int main(void) {
    plat_init();

    mem_arena* perm_arena = arena_create(MiB(1), KiB(64));
    
    string8 file_name = STR8_LIT("README.md");
    u64 file_size = plat_file_size(file_name);
    string8 file = plat_file_read(perm_arena, file_name);

    printf("%lu\n", file_size);
    printf("%.*s\n", (int)file.size, file.str);

    arena_destroy(perm_arena);


    return 0;
}

