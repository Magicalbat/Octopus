#ifdef PROFILE_MODE

#include <stdio.h>
#include <string.h>

#include "profile.h"
#include "base/base.h"
#include "platform/platform.h"

typedef struct {
    string8_list output;

    string8 commit;
    string8 out_filename;
    u32 base_indent;
    u32 indent_num_spaces;
    b32 trailing_comma;
} profiler;

profiler prof_init(u32 num_args, string8* args);
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

    profiler prof = prof_init(num_args, args);
    prof_run(perm_arena, &prof);
    prof_save_out(&prof);

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

profiler prof_init(u32 num_args, string8* args) {
    profiler prof = { 
        .commit = STR8_LIT("current"),
        .out_filename = STR8_LIT("profiler_output.json"),
        .base_indent = 1,
        .indent_num_spaces = 4,
        .trailing_comma = true,
    };

    for (u32 i = 0; i < num_args; i++) {
        string8 value = { 0 };

        if (try_get_arg(args[i], STR8_LIT("--commit="), &value)) {
            prof.commit = value;
        }
        if (try_get_arg(args[i], STR8_LIT("--out_filename="), &value)) {
            prof.out_filename = value;
        }
        if (try_get_arg(args[i], STR8_LIT("--base_indent="), &value)) {
            prof.base_indent = str8_to_u64(value);
        }
        if (try_get_arg(args[i], STR8_LIT("--indent_num_spaces="), &value)) {
            prof.indent_num_spaces = str8_to_u64(value);
        }
        if (try_get_arg(args[i], STR8_LIT("--trailing_comma="), &value)) {
            prof.trailing_comma = str8_to_b32(value);
        }
    }

    return prof;
}

void prof_run(mem_arena* arena, profiler* prof) {

#define PROF_WRITE(s) str8_list_push(arena, &prof->output, STR8_LIT(s))
#define PROF_WRITE_EXISTING(s) str8_list_push(arena, &prof->output, (s))
#define PROF_INDENT(n) for (u32 __i = 0; __i < (n) + prof->base_indent; __i++) {\
    PROF_WRITE_EXISTING(indent_str); }

    string8 indent_str = {
        .size = prof->indent_num_spaces,
        .str = ARENA_PUSH_ARRAY(arena, u8, prof->indent_num_spaces)
    };
    memset(indent_str.str, ' ', indent_str.size);

    // Initial JSON Setup
    PROF_INDENT(0);
    PROF_WRITE("{\n");
    PROF_INDENT(1);
    PROF_WRITE("\"commit\": \"");
    PROF_WRITE_EXISTING(prof->commit);
    PROF_WRITE("\",\n");
    PROF_INDENT(1);
    PROF_WRITE("\"categories\": {\n");

    // Running Profiles
    {
    }

    // Closing JSON Object
    PROF_INDENT(1);
    PROF_WRITE("}\n"); // Closing categories
    PROF_INDENT(0);
    PROF_WRITE("}"); // Closing main object
    if (prof->trailing_comma) {
        PROF_WRITE(",");
    }
    PROF_WRITE("\n");
}

void prof_save_out(profiler* prof) {
    plat_file_write(prof->out_filename, &prof->output, true);
}

#endif // PROFILE_MODE

