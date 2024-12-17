#ifdef PROFILE_MODE

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "profile.h"
#include "base/base.h"
#include "platform/platform.h"

typedef struct {
    string8 commit;
    string8 out_filename;
    u32 base_indent;
    u32 indent_num_spaces;
    b32 trailing_comma;

    // Each string is of the format
    // "{category}-{sub category}-{name}-{time}"
    string8_list record;
} profiler;

profiler* prof_init(mem_arena* arena, u32 num_args, string8* args);
void prof_run(mem_arena* arena, profiler* prof);
void prof_save_out(profiler* prof);

int profile_main(int argc, char** argv) {
    plat_init();

    mem_arena* perm_arena = arena_create(MiB(64), KiB(256));

    u32 num_args = (u32)MAX(0, argc);
    string8* args = ARENA_PUSH_ARRAY(perm_arena, string8, argc);
    for (u32 i = 0; i < num_args; i++) {
        args[i] = str8_from_cstr((u8*)argv[i]);
    }

    profiler* prof = prof_init(perm_arena, num_args, args);
    prof_run(perm_arena, prof);
    prof_save_out(prof);

    arena_destroy(perm_arena);

    return 0;
}

// If the current arg starts with the string name,
// the function will return true and put everything after in value.
// Otherwise, it will return false and not modify value
b32 try_get_arg(string8 arg, string8 name, string8* value) {
    if (value == NULL) {
        return false;
    }

    string8 sub = str8_substr(arg, 0, name.size);

    if (str8_equals(sub, name)) {
        *value = str8_substr(arg, name.size, arg.size);
        return true;
    }

    return false;
}

profiler* prof_init(mem_arena* arena, u32 num_args, string8* args) {
    profiler* prof = ARENA_PUSH(arena, profiler);

    *prof = (profiler){
        .commit = STR8_LIT("current"),
        .out_filename = STR8_LIT("profiler_output.json"),
        .base_indent = 1,
        .indent_num_spaces = 4,
        .trailing_comma = true,
    };

    for (u32 i = 0; i < num_args; i++) {
        string8 value = { 0 };

        if (try_get_arg(args[i], STR8_LIT("--commit="), &value)) {
            prof->commit = value;
        }
        if (try_get_arg(args[i], STR8_LIT("--out_filename="), &value)) {
            prof->out_filename = value;
        }
        if (try_get_arg(args[i], STR8_LIT("--base_indent="), &value)) {
            prof->base_indent = str8_to_u64(value);
        }
        if (try_get_arg(args[i], STR8_LIT("--indent_num_spaces="), &value)) {
            prof->indent_num_spaces = str8_to_u64(value);
        }
        if (try_get_arg(args[i], STR8_LIT("--trailing_comma="), &value)) {
            prof->trailing_comma = str8_to_b32(value);
        }
    }

    return prof;
}

#define PROF_RECORD(name) for (u64 __start = plat_time_usec(); __start; \
    str8_list_push(arena, &prof->record, \
    str8_createf(arena, "%s-%s-%s-%" PRIu64, \
                 category, sub_category, name, plat_time_usec()-__start)), \
    __start = 0)

void prof_run_base(mem_arena* arena, profiler* prof) {
    const char* category = "base";
    char* sub_category = "";

    sub_category = "arena";
    {
        mem_arena* test_arena = NULL;
        u64 reserve_size = MiB(1);
        u64 commit_size = KiB(64);

        PROF_RECORD("create") {
            test_arena = arena_create(reserve_size, commit_size);
        }

        PROF_RECORD("push_basic") {
            arena_push(test_arena, KiB(1));
        }
        PROF_RECORD("push_commit") {
            arena_push(test_arena, commit_size * 4);
        }
        PROF_RECORD("push_reserve") {
            arena_push(test_arena, reserve_size * 3);
        }

        PROF_RECORD("pop_reserve") {
            arena_pop(test_arena, reserve_size * 3);
        }

        PROF_RECORD("pop_basic") {
            arena_pop(test_arena, commit_size * 2);
        }

        PROF_RECORD("destroy") {
            arena_destroy(test_arena);
        }

        mem_arena_temp scratch = { 0 };

        PROF_RECORD("scratch_get") {
            scratch = arena_scratch_get(NULL, 0);
        }

        arena_push(scratch.arena, MiB(1));

        PROF_RECORD("scratch_release") {
            arena_scratch_release(scratch);
        }
    }
}

void prof_run_platform(mem_arena* arena, profiler* prof) {
    const char* category = "platform";
    char* sub_category = "";

    sub_category = "io";
    {
        string8 file_name = STR8_LIT("profile_tmp.txt");
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8_list out_list = { 0 };
        for (u32 i = 0; i < (1 << 16); i++) {
            str8_list_push(scratch.arena, &out_list, str8_createf(scratch.arena, "line %u\n", i+1));
        }

        PROF_RECORD("file_write") {
            plat_file_write(file_name, &out_list, false);
        }

        volatile string8 file = { 0 };
        UNUSED(file);

        PROF_RECORD("file_read") {
            file = plat_file_read(scratch.arena, file_name);
        }

        PROF_RECORD("file_delete") {
            plat_file_delete(file_name);
        }

        arena_scratch_release(scratch);
    }
}

void prof_run(mem_arena* arena, profiler* prof) {
    prof_run_base(arena, prof);
    prof_run_platform(arena, prof);
}

typedef struct {
    string8 category;
    string8 sub_category;
    string8 name;
    string8 time;
} prof_item_parts;

void _item_parts_from_str(string8 str, prof_item_parts* out) {
    u64 hyphen_indices[3] = { 0 };
    u64 cur_hyphen_index = 0;

    for (u64 i = 0; i < str.size; i++) {
        if (str.str[i] == '-') {
            hyphen_indices[cur_hyphen_index++] = i;
        }
    }

    out->category = str8_substr(str, 0, hyphen_indices[0]);
    out->sub_category = str8_substr(str, hyphen_indices[0] + 1, hyphen_indices[1]);
    out->name = str8_substr(str, hyphen_indices[1] + 1, hyphen_indices[2]);
    out->time = str8_substr(str, hyphen_indices[2] + 1, str.size);
}

#define WRITE_CSTR(s) str8_list_push(scratch.arena, &out, STR8_LIT(s))
#define WRITE_STR(str) str8_list_push(scratch.arena, &out, str)
#define INDENT(n) for (u32 __i = 0; __i < prof->base_indent + n; __i++) { \
    WRITE_STR(indent_str); }

void prof_save_out(profiler* prof) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string8 indent_str = (string8){
        .size = prof->indent_num_spaces,
        .str = ARENA_PUSH_ARRAY(scratch.arena, u8, prof->indent_num_spaces)
    };
    memset(indent_str.str, ' ', prof->indent_num_spaces);

    string8_list out = { 0 };

    // Initial JSON object setup
    INDENT(0);
    WRITE_CSTR("{\n");
    INDENT(1);
    WRITE_STR(str8_createf(
        scratch.arena,
        "\"commit\": \"%.*s\",\n",
        (int)prof->commit.size, (char*)prof->commit.str
    ));
    INDENT(1);
    WRITE_STR(str8_createf(
        scratch.arena,
        "\"platform\": \"%.*s\",\n",
        (int)plat_get_name().size, (char*)plat_get_name().str
    ));
    INDENT(1);
    WRITE_CSTR("\"categories\": {\n");

    prof_item_parts prev_parts = { 0 };
    prof_item_parts cur_parts = { 0 };

    b8 first_category = true;
    b8 first_sub_category = true;
    b8 first_name = true;

    for (string8_bucket* bucket = prof->record.first; bucket != NULL; bucket = bucket->next) {
        for (u32 i = 0; i < bucket->size; i++) {
            string8 str = bucket->strs[i];
            _item_parts_from_str(str, &cur_parts);

            if (!str8_equals(prev_parts.category, cur_parts.category)) {
                // New category

                if (!first_category) {
                    // Writing the end of the previous sub category
                    WRITE_CSTR("\n");
                    INDENT(3);
                    WRITE_CSTR("}\n");

                    // Write the end of the previous category
                    INDENT(2);
                    WRITE_CSTR("},\n");
                }

                INDENT(2);
                WRITE_STR(str8_createf(
                    scratch.arena,
                    "\"%.*s\": {\n",
                    (int)cur_parts.category.size,
                    (char*)cur_parts.category.str
                ));

                first_category = false;
                first_sub_category = true;
            }

            if (!str8_equals(prev_parts.sub_category, cur_parts.sub_category)) {
                // New sub category

                if (!first_sub_category) {
                    // Writing end of the previous sub category 
                    WRITE_CSTR("\n");
                    INDENT(3);
                    WRITE_CSTR("},\n");
                }

                INDENT(3);
                WRITE_STR(str8_createf(
                    scratch.arena,
                    "\"%.*s\": {\n",
                    (int)cur_parts.sub_category.size,
                    (char*)cur_parts.sub_category.str
                ));

                first_sub_category = false;
                first_name = true;
            }

            if (!first_name) {
                WRITE_CSTR(",\n");
            }

            first_name = false;

            INDENT(4);
            WRITE_STR(str8_createf(
                scratch.arena,
                "\"%.*s\": %.*s",
                (int)cur_parts.name.size, (char*)cur_parts.name.str,
                (int)cur_parts.time.size, (char*)cur_parts.time.str
            ));

            prev_parts = cur_parts;
        }
    }

    // End of sub category
    WRITE_CSTR("\n");
    INDENT(3);
    WRITE_CSTR("}\n");

    // End of category
    INDENT(2);
    WRITE_CSTR("}\n");

    // End of categories object
    INDENT(1);
    WRITE_CSTR("}\n");

    // End of parent object
    INDENT(0);
    if (prof->trailing_comma) {
        WRITE_CSTR("},\n");
    } else {
        WRITE_CSTR("}\n");
    }

    plat_file_write(prof->out_filename, &out, true);

    arena_scratch_release(scratch);
}

#endif // PROFILE_MODE

