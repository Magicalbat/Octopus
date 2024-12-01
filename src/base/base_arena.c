#include "base_arena.h"
#include "platform/platform.h"

#include <string.h>

#define ARENA_ALIGN (sizeof(void*))

static u64 round_up_pow2(u64 n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;

    return n;
}

mem_arena* arena_create(u64 desired_reserve_size, u64 desired_block_size) {
    u32 page_size = plat_mem_page_size();
    u64 block_size = round_up_pow2(ALIGN_UP_POW2(desired_block_size, page_size));
    u64 reserve_size = round_up_pow2(desired_reserve_size);
    mem_arena* out = plat_mem_reserve(reserve_size);

    u64 base_commit = ALIGN_UP_POW2(sizeof(mem_arena), block_size);

    if (out == NULL || !plat_mem_commit(out, base_commit)) {
        return NULL;
    }

    memset(out, 0, sizeof(mem_arena));

    out->current = out;
    out->reserve_size = reserve_size;
    out->block_size = block_size;
    out->pos = sizeof(mem_arena);
    out->commit_size = base_commit;

    return out;
}
void arena_destroy(mem_arena* arena) {
    mem_arena* current = arena->current;

    for (mem_arena* node = current, *prev = NULL; node != NULL; node = prev) {
        prev = node->prev;
        plat_mem_release(node, node->reserve_size);
    }
}
u64 arena_get_pos(mem_arena* arena) {
    mem_arena* current = arena->current;

    return current->base_pos + current->pos;
}
void* arena_push_no_zero(mem_arena* arena, u64 size) {
    mem_arena* current = arena->current;

    // Fast path
    u64 pos_aligned = ALIGN_UP_POW2(current->pos, ARENA_ALIGN);
    u64 new_pos = pos_aligned + size;
    void* out = (u8*)current + pos_aligned;

    if (new_pos <= current->commit_size) {
        current->pos = new_pos;
        return out;
    }

    out = NULL;

    // Need a new arena?
    if (new_pos > current->base_pos + current->reserve_size) {
        mem_arena* new_arena = arena_create(
            // Make sure there is enough space in this arena
            ALIGN_UP_POW2(size + sizeof(mem_arena), current->reserve_size),
            current->block_size
        );

        if (new_arena == NULL) { return NULL; }

        new_arena->base_pos = current->base_pos + current->reserve_size;
        new_arena->prev = current;
        current = new_arena;
        arena->current = new_arena;

        pos_aligned = ALIGN_UP_POW2(current->pos, ARENA_ALIGN);
        new_pos = pos_aligned + size;
    }

    u64 new_commit_size = ALIGN_UP_POW2(new_pos, current->block_size);
    new_commit_size = MIN(new_commit_size, current->reserve_size);
    if (!plat_mem_commit((u8*)current + current->commit_size, new_commit_size - current->commit_size)) {
        return NULL;
    }

    out = (u8*)current + pos_aligned;
    current->pos = new_pos;
    current->commit_size = new_commit_size;

    return out;
}
void* arena_push(mem_arena* arena, u64 size) {
    void* out = arena_push_no_zero(arena, size);

    if (out != NULL) {
        memset(out, 0, size);
    }

    return out;
}
void arena_pop_to(mem_arena* arena, u64 pos) {
    if (pos >= arena_get_pos(arena)) {
        return;
    }

    mem_arena* current = arena->current;

    while (pos < current->base_pos) {
        mem_arena* old_cur = current;
        current = current->prev;
        plat_mem_release(old_cur, old_cur->reserve_size);
    }

    arena->current = current;

    u64 local_pos = pos - current->base_pos;
    local_pos = MAX(sizeof(mem_arena), local_pos);
    current->pos = local_pos;
}
void arena_pop(mem_arena* arena, u64 amount) {
    u64 cur_pos = arena_get_pos(arena);
    u64 set_pos = amount > cur_pos ? 0 : cur_pos - amount;
    arena_pop_to(arena, set_pos);
}

mem_arena_temp arena_temp_begin(mem_arena* arena) {
    return (mem_arena_temp) {
        .arena = arena,
        .pos = arena_get_pos(arena)
    };
}
void arena_temp_end(mem_arena_temp temp_arena) {
    arena_pop_to(temp_arena.arena, temp_arena.pos);
}

THREAD_LOCAL mem_arena* scratch_pool[ARENA_NUM_SCRATCH] = { NULL, NULL };

mem_arena_temp arena_scratch_get(mem_arena** conflicts, u32 num_conflicts) {
    i32 scratch_index = -1;
    for (u32 i = 0; i < ARENA_NUM_SCRATCH; i++) {
        b32 conflict = false;

        for (u32 j = 0; j < num_conflicts; j++) {
            if (scratch_pool[i] == conflicts[j]) {
                conflict = true;
                break;
            }
        }

        if (!conflict) {
            scratch_index = i;
            break;
        }
    }

    if (scratch_index == -1) {
        return (mem_arena_temp){ NULL, 0 };
    }

    if (scratch_pool[scratch_index] == NULL) {
        scratch_pool[scratch_index] = arena_create(ARENA_SCRATCH_RESERVE_SIZE, ARENA_SCRATCH_COMMIT_SIZE);
    }

    return arena_temp_begin(scratch_pool[scratch_index]);
}
void arena_scratch_release(mem_arena_temp scratch) {
    arena_temp_end(scratch);
}

