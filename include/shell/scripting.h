/*
  Copyright (c) 2017 Tom Hancocks
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef SHELL_SCRIPTING
#define SHELL_SCRIPTING

struct shell;
typedef int(*shell_command_imp_t)(struct shell *, int, const char *[]);

struct shell_command;
struct shell_command {
    const char *name;
    shell_command_imp_t impl;
    struct shell_command *next;
};
typedef struct shell_command *shell_command_t;

struct shell_statement;
struct shell_statement {
    int argc;
    const char **argv;
    struct shell_statement *next;
};
typedef struct shell_statement *shell_statement_t;

struct shell_script {
    shell_statement_t first;
    shell_statement_t last;
};
typedef struct shell_script *shell_script_t;


shell_command_t shell_command_create(const char *name, shell_command_imp_t imp);
void shell_command_destroy(shell_command_t cmd);

shell_statement_t shell_statement_create(const char *raw_statement);
void shell_statement_destroy(shell_statement_t statement);

shell_script_t shell_script_open(const char *path);
void shell_script_destroy(shell_script_t script);

void shell_script_execute(struct shell *shell, shell_script_t script);
void shell_statement_resolve(struct shell *shell, shell_statement_t stmt);
int shell_statement_execute(struct shell *shell, shell_statement_t stmt);

#endif
