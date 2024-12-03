#include <stdio.h>

#include "base/base.h"
#include "platform/platform.h"

int main(void) {
    plat_init();

    u64 start_time = plat_time_usec();

    mem_arena* perm_arena = arena_create(MiB(1), KiB(64));

    string8_list list = { 0 };

    str8_list_push(perm_arena, &list, STR8_LIT("String 1\n"));
    str8_list_push(perm_arena, &list, STR8_LIT("String 2\n"));
    str8_list_push(perm_arena, &list, STR8_LIT("String 3\n"));
    str8_list_push(perm_arena, &list, STR8_LIT("String 4"));

    string8 out = str8_concat(perm_arena, &list);
    
    printf("%.*s\n", (int)out.size, out.str);

    arena_destroy(perm_arena);

    u64 end_time = plat_time_usec();

    printf("main took %llu us\n", end_time - start_time);

    return 0;
}

