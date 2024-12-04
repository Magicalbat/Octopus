#include <stdio.h>

#include "base/base.h"
#include "platform/platform.h"

int main(void) {
    plat_init();

    mem_arena* perm_arena = arena_create(MiB(1), KiB(64));

    string8_list list = { 0 };
    str8_list_push(perm_arena, &list, STR8_LIT("Line 1\n"));
    str8_list_push(perm_arena, &list, STR8_LIT("Line 2\n"));
    str8_list_push(perm_arena, &list, STR8_LIT("Line 3\n"));
    str8_list_push(perm_arena, &list, STR8_LIT("Line 4\n"));
    str8_list_push(perm_arena, &list, STR8_LIT("Line 5\n"));

    plat_file_write(STR8_LIT("out.txt"), &list);
    
    arena_destroy(perm_arena);

    return 0;
}

