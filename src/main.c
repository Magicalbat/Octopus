#include <stdio.h>
#include <stdlib.h>

#include "base/base.h"
#include "platform/platform.h"
#include "profile/profile.h"

int main(int argc, char** argv) {
#ifdef PROFILE_MODE
    return profile_main(argc, argv);
#else

    UNUSED(argc);
    UNUSED(argv);

    plat_init();

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264));

    string8 str = plat_get_name();
    string8_list list = { 0 };
    str8_list_push(perm_arena, &list, str);

    plat_file_write(STR8_LIT("tmp.txt"), &list, false);
    string8 file = plat_file_read(perm_arena, STR8_LIT("tmp.txt"));

    printf("%.*s\n", (int)file.size, file.str);

    plat_file_delete(STR8_LIT("tmp.txt"));

    arena_destroy(perm_arena);

    return 0;

#endif
}

