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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <shell/shell.h>
#include <shell/prompt.h>
#include <shell/commands.h>
#include <shell/scripting.h>
#include <shell/parser.h>

#include <vfs/vfs.h>

#define SHELL_BUFFER_LEN 1024


#pragma mark - Main Shell

static shell_t main_shell = NULL;

shell_t shell_init(shell_variable_t vars,
                   shell_script_t script,
                   const char *image_path)
{
    if (main_shell) {
        return main_shell;
    }
    
    // Create a new shell
    main_shell = calloc(1, sizeof(*main_shell));
    
    // Configure the shell
    main_shell->buffer_size = SHELL_BUFFER_LEN;
    main_shell->script = script;
    main_shell->image_path = image_path;
    main_shell->first_variable = vars;
    shell_register_commands(main_shell);
    
    return main_shell;
}

void shell_do(shell_t shell)
{
    assert(shell);

    // If there is a script to run then we should run it before launching the
    // user prompt shell.
    if (shell->script) {
        shell_script_execute(shell, shell->script);
    }

    // Start the shell run loop
    char *buffer = malloc(main_shell->buffer_size * sizeof(*buffer));
    int err = 0;
    while (err != SHELL_TERMINATE) {

        // Clear the input buffer and ensure it is clean for the next input
        memset(buffer, 0, main_shell->buffer_size * sizeof(buffer));

        // Display the input prompt and get input from the user
        char *prompt = calloc(255, sizeof(*prompt));
        strcpy(prompt, vfs_pwd(shell->device_filesystem));
        strcpy(prompt + strlen(prompt), " # ");
        shell_get_input(prompt, buffer, main_shell->buffer_size);

        // Construct a statement from the input and then execute the statement.
        shell_statement_t stmt = shell_statement_create(buffer);
        if (stmt) {
            shell_statement_resolve(main_shell, stmt);
            err = shell_statement_execute(main_shell, stmt);
            shell_statement_destroy(stmt);
            stmt = NULL;
        }

        // Clean up the prompt and wrap up this command ready for the next one.
        free(prompt);
        printf("\n");
    }

    // Clean up
    free(buffer);
}


#pragma mark - Shell Commands

void shell_add_command(shell_t shell, shell_command_t command)
{
    assert(shell);
    assert(command);

    shell_command_t tmp = shell->first_command;
    command->next = tmp;
    shell->first_command = command;
}


#pragma mark - Shell Variables

void shell_add_variable(shell_t shell, shell_variable_t variable)
{
    assert(shell);
    assert(variable);
    
    // We're going to add the variable to the head of the variable list,
    // shifting everything along by one.
    variable->next = shell->first_variable;
    if (shell->first_variable) {
        shell->first_variable->prev = variable;
    }
    shell->first_variable = variable;
}

shell_variable_t shell_find_variable(shell_t shell, const char *symbol)
{
    assert(shell);
    assert(symbol);
    
    shell_variable_t variable = shell->first_variable;
    while (variable) {
        if (strcmp(variable->symbol, symbol) == 0) {
            break;
        }
        variable = variable->next;
    }
    return variable;
}

