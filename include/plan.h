#ifndef DEU_PLAN_H
#define DEU_PLAN_H
#include <stddef.h>
#define PLAN_MAX_STAGES 16
#define PLAN_MAX_ARGS 64
#define PLAN_MAX_TOKENS 256
#define PLAN_TEXT_CAP 4096

#define PLAN_MAX_REDIRS 16
typedef enum { REDIR_INPUT, REDIR_OUTPUT, REDIR_APPEND, REDIR_DUP_STDERR } RedirType;
typedef struct { RedirType type; int fd; char *path; } Redirection;
typedef struct {
    Redirection redirs[PLAN_MAX_REDIRS];
    size_t redir_count;
    char *argv[PLAN_MAX_ARGS + 1];
    size_t argc;
    char *input_path;
    char *output_path;
    int append_output;
} Stage;
typedef struct {
    Stage stages[PLAN_MAX_STAGES];
    size_t count;
    int background;
} Plan;
int plan_parse(const char *line, Plan *out, char *error, size_t error_size);
int plan_parse_status(const char *line, Plan *out, char *error, size_t error_size, int last_status);
void plan_release(Plan *plan);
void plan_print(const Plan *plan);
#endif
