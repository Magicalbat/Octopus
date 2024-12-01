#include <stdio.h>
#include <string.h>

#include "base/base.h"
#include "platform/platform.h"

int main(void) {
    u32 page_size = plat_mem_page_size();
    printf("page_size: %u\n", page_size);

    u8* mem = plat_mem_reserve(page_size * 4);
    plat_mem_commit(mem, page_size);

    memcpy(mem, "Hello World", sizeof("Hello World"));

    printf("%s\n", mem);

    plat_mem_commit(mem + page_size, page_size * 3);
    mem[page_size + 1] = '1';

    plat_mem_release(mem, page_size * 4);

    return 0;
}

