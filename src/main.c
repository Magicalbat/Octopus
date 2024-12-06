#include <stdio.h>

#include "base/base.h"
#include "platform/platform.h"

int main(void) {
    plat_init();

    mem_arena* perm_arena = arena_create(MiB(1), KiB(64));

    string8 test_str = str8_createf(perm_arena, "Hello %d, and hello %s", 123, "asdf");

    printf("'%.*s'\n", (int)test_str.size, test_str.str);

    arena_destroy(perm_arena);

    return 0;
}

