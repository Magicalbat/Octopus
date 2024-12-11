#ifdef PROFILE_MODE
#include <stdio.h>

#include "profile.h"
#include "base/base.h"
#include "platform/platform.h"

typedef struct {
    string8 commit;
    string8 out_filename;
    u32 indent;
} profiler;

profiler profiler_init(u32 num_args, string8* args);
void profiler_run(profiler* prof);

int profile_main(int argc, char** argv) {
    plat_init();

    mem_arena* perm_arena = arena_create(MiB(64), KiB(256));

    u32 num_args = (u32)MAX(0, argc);
    string8* args = ARENA_PUSH_ARRAY(perm_arena, string8, argc);
    for (u32 i = 0; i < num_args; i++) {
        args[i] = str8_from_cstr((u8*)argv[i]);
    }

    profiler prof = profiler_init(num_args, args);

    printf(
        "commit: '%.*s'\nout_filename: '%.*s'\nindent: %u\n",
        (int)prof.commit.size, prof.commit.str,
        (int)prof.out_filename.size, prof.out_filename.str,
        prof.indent
    );

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

profiler profiler_init(u32 num_args, string8* args) {
    profiler prof = { 
        .commit = STR8_LIT("current"),
        .out_filename = STR8_LIT("profiler_output.json"),
        .indent = 4,
    };

    for (u32 i = 0; i < num_args; i++) {
        string8 value = { 0 };

        if (try_get_arg(args[i], STR8_LIT("--commit="), &value)) {
            prof.commit = value;
        }
        if (try_get_arg(args[i], STR8_LIT("--out_filename="), &value)) {
            prof.out_filename = value;
        }
        if (try_get_arg(args[i], STR8_LIT("--indent="), &value)) {
            prof.indent = str8_to_u64(value);
        }
    }

    return prof;
}

#endif // PROFILE_MODE

