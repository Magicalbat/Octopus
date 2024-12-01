#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#include "base_defs.h"

#define ARENA_NUM_SCRATCH 2
#define ARENA_SCRATCH_RESERVE_SIZE MiB(64)
#define ARENA_SCRATCH_COMMIT_SIZE KiB(256)

#define ARENA_PUSH(arena, T) (T*)arena_push((arena), sizeof(T))
#define ARENA_PUSH_NZ(arena, T) (T*)arena_push_no_zero((arena), sizeof(T))
#define ARENA_PUSH_ARRAY(arena, T, num) (T*)arena_push((arena), sizeof(T) * (num))
#define ARENA_PUSH_ARRAY_NZ(arena, T, num) (T*)arena_push_no_zero((arena), sizeof(T) * (num))

typedef struct mem_arena {
    struct mem_arena* prev;
    struct mem_arena* current;

    // Both of these are rounded up to be powers of two
    u64 reserve_size;
    // Memory is commited in factors of block_size
    u64 block_size;

    // Do not access these directly, use arena_get_pos instead
    u64 base_pos;
    u64 pos;
    u64 commit_size;
} mem_arena;

typedef struct {
    mem_arena* arena;
    u64 pos;
} mem_arena_temp;

mem_arena* arena_create(u64 desired_reserve_size, u64 desired_block_size);
void arena_destroy(mem_arena* arena);
u64 arena_get_pos(mem_arena* arena);
void* arena_push_no_zero(mem_arena* arena, u64 size);
void* arena_push(mem_arena* arena, u64 size);
void arena_pop_to(mem_arena* arena, u64 pos);
void arena_pop(mem_arena* arena, u64 amount);

mem_arena_temp arena_temp_begin(mem_arena* arena);
void arena_temp_end(mem_arena_temp temp_arena);

mem_arena_temp arena_scratch_get(mem_arena** conflicts, u32 num_conflicts);
void arena_scratch_release(mem_arena_temp scratch);

#endif // BASE_ARENA_H

