#define ERROR_MAX_ERRORS 256
#define ERROR_MAX_MSG_LEN 1024
#define ERROR_CONCAT_CHAR '\n'

typedef enum {
    ERROR_OUTPUT_FIRST,
    ERROR_OUTPUT_LAST,
    ERROR_OUTPUT_CONCAT,
} error_output_type;

void error_frame_begin(void);
string8 error_frame_end(mem_arena* arena, error_output_type output_type);

// Any strings longer than ERROR_MAX_MSG_LEN will be cut off
void error_emit_str(string8 str);
void error_emit(char* cstr);
void error_emitf(char* fmt, ...);

