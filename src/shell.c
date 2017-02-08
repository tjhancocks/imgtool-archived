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

#include "shell.h"
#include "shell_prompt.h"
#include "shell_commands.h"
#include "shell_scripting.h"
#include "shell_parser.h"

#include "vfs.h"

#define SHELL_BUFFER_LEN 1024

static shell_t main_shell = NULL;

shell_t shell_init(vfs_t vfs)
{
    if (main_shell) {
        return main_shell;
    }
    
    // Create a new shell
    main_shell = calloc(1, sizeof(*main_shell));
    
    // Configure the shell
    main_shell->buffer_size = SHELL_BUFFER_LEN;
    main_shell->running = 1;
    main_shell->filesystem = vfs;
    shell_register_commands(main_shell);
    
    return main_shell;
}

void shell_do(shell_t shell)
{
    assert(shell);

    // Start the shell run loop
    char *buffer = malloc(main_shell->buffer_size * sizeof(*buffer));
    while (main_shell->running) {

        // Clear the input buffer and ensure it is clean for the next input
        memset(buffer, 0, main_shell->buffer_size * sizeof(buffer));

        // Display the input prompt and get input from the user
        char *prompt = calloc(255, sizeof(*prompt));
        strcpy(prompt, vfs_pwd(shell->filesystem));
        strcpy(prompt + strlen(prompt), " # ");
        shell_get_input(prompt, buffer, main_shell->buffer_size);

        // Parse the input out into a list of arguments. First argument is the
        // command name.
        int argc = 0;
        char **argv = NULL;
        shell_parse(buffer, &argc, &argv);

        // Execute the arguments.
        shell_execute(main_shell, argc, (const char **)argv);

        // Clean up the prompt and wrap up this command ready for the next one.
        free(prompt);
        free(argv);
        printf("\n");
    }
}

void shell_add_command(shell_t shell, shell_command_t command)
{
    assert(shell);
    assert(command);

    shell_command_t tmp = shell->first_command;
    command->next = tmp;
    shell->first_command = command;
}
