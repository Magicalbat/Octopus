#include "base_error.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    string8 msg;
    u32 stack_pos;
    u64 arena_pos;
} _error_elem;

typedef struct {
    mem_arena* arena;
    _error_elem* errors;
    u32 num_errors;

    // Num errors at start of current stack frame
    u32 cur_stack_pos;
    // Arena pos at start of current stack frame
    u64 cur_arena_pos;
} _error_thread_context;

static THREAD_LOCAL _error_thread_context _context = { 0 };

void error_frame_begin(void) {
    if (_context.arena == NULL) {
        u64 min_reserve_size = (ERROR_MAX_ERRORS + 1) *
            (sizeof(_error_elem) + ERROR_MAX_MSG_LEN);

        _context.arena = arena_create(
            min_reserve_size, KiB(64), false
        );

        _context.errors = ARENA_PUSH_ARRAY(
            _context.arena, _error_elem, ERROR_MAX_ERRORS
        );
    }

    _context.cur_stack_pos = _context.num_errors;
    _context.cur_arena_pos = arena_get_pos(_context.arena);
}
string8 error_frame_end(mem_arena* arena, error_output_type output_type) {
    u32 num_frame_errors = MAX(0, (i64)_context.num_errors - _context.cur_stack_pos);

    if (num_frame_errors == 0) {
        return (string8){ 0 };
    }

    _error_elem* frame_errors = &_context.errors[_context.cur_stack_pos];

    string8 out = { 0 };

    switch (output_type) {
        case ERROR_OUTPUT_FIRST: {
            out = str8_copy(arena, frame_errors[0].msg);
        } break;
        case ERROR_OUTPUT_LAST: {
            out = str8_copy(arena, frame_errors[num_frame_errors - 1].msg);
        } break;
        case ERROR_OUTPUT_CONCAT: {
            u64 out_size = 0;
            for (u32 i = 0; i < num_frame_errors; i++) {
                // +1 for separator
                out_size += frame_errors[i].msg.size + 1;
            }

            // No need to separator at the end
            if (out_size) { out_size--; }

            out.size = out_size;
            out.str = ARENA_PUSH_ARRAY(arena, u8, out_size);

            u64 out_pos = 0;
            for (u32 i = 0; i < num_frame_errors; i++) {
                memcpy(out.str + out_pos, frame_errors[i].msg.str, frame_errors[i].msg.size);
                out_pos += frame_errors[i].msg.size;

                out.str[out_pos++] = ERROR_CONCAT_CHAR;
            }
        } break;
    }

    _context.num_errors = _context.cur_stack_pos;
    arena_pop_to(_context.arena, _context.cur_arena_pos);

    if (_context.num_errors) {
        _error_elem last_error = _context.errors[_context.num_errors-1];

        _context.cur_stack_pos = last_error.stack_pos;
        _context.cur_arena_pos = last_error.arena_pos;
    }

    return out;
}

void _error_emit_impl(string8 str) {
    if (_context.num_errors >= ERROR_MAX_ERRORS) {
        return;
    }

    _error_elem cur_error = {
        .msg = str,
        .stack_pos = _context.cur_stack_pos,
        .arena_pos = _context.cur_arena_pos,
    };

    _context.errors[_context.num_errors++] = cur_error;
}

void error_emit_str(string8 str) {
    string8 msg = str8_copy(_context.arena, str8_substr(str, 0, ERROR_MAX_MSG_LEN));

    _error_emit_impl(msg);
}

void error_emit(char* cstr) {
    if (cstr == NULL) {
        return;
    }

    u64 size = 0;
    u8* ptr = (u8*)cstr;
    for (u32 i = 0; i < ERROR_MAX_MSG_LEN && *(ptr++); i++) {
        size++;
    }

    string8 msg = {
        .str = ARENA_PUSH_ARRAY(_context.arena, u8, size),
        .size = size
    };

    memcpy(msg.str, cstr, size);

    _error_emit_impl(msg);
}

void error_emitf(char* fmt, ...) {
    if (fmt == NULL) {
        return;
    }

    va_list args = { 0 };
    va_start(args, fmt);

    u8* str_data = ARENA_PUSH_ARRAY(_context.arena, u8, ERROR_MAX_MSG_LEN);
    i32 size = vsnprintf((char*)str_data, ERROR_MAX_MSG_LEN, fmt, args);

    if (size < 0) {
        arena_pop(_context.arena, ERROR_MAX_MSG_LEN);
        return;
    }

    size = MIN(size, ERROR_MAX_MSG_LEN);

    if (size < ERROR_MAX_MSG_LEN) {
        arena_pop(_context.arena, ERROR_MAX_MSG_LEN - size);
    }

    _error_emit_impl((string8){
        .str = str_data,
        .size = size
    });

    va_end(args);
}

